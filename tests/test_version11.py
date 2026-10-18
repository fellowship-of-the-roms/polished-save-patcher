#!/usr/bin/env python3
"""Behavioral/save-integrity regression tests for the v3.3.0 migration.

Uses real v10 saves plus controlled boundary cases. Expected values are checked
independently of the C++ patch, including all stored Pokemon formats and maps.
Run after building the CLI: python3 tests/test_version11.py [patcher-path]
"""

import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATCHER = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else ROOT / "build/polished_save_patcher"


def symbols(path):
    result = {}
    for line in path.read_text().splitlines():
        match = re.fullmatch(r"([0-9a-fA-F]+):([0-9a-fA-F]+) (\S+)", line)
        if match:
            result[match[3]] = (int(match[1], 16), int(match[2], 16))
    return result


SYM = symbols(ROOT / "resources/version11/version11.sym")


def sram(name):
    bank, address = SYM[name]
    return bank * 0x2000 + address - 0xa000


def saved(name):
    for base, end, destination in (
        ("wOptions", "wOptionsEnd", "sOptions"),
        ("wPlayerData", "wPlayerDataEnd", "sPlayerData"),
        ("wCurMapData", "wCurMapDataEnd", "sMapData"),
        ("wPokemonData", "wPokemonDataEnd", "sPokemonData"),
    ):
        if SYM[base][1] <= SYM[name][1] < SYM[end][1]:
            return sram(destination) + SYM[name][1] - SYM[base][1]
    raise ValueError(name)


def getflag(data, start, index):
    return bool(data[start + index // 8] & (1 << (index % 8)))


def setflag(data, start, index, value=True):
    mask = 1 << (index % 8)
    data[start + index // 8] = (data[start + index // 8] & ~mask) | (mask if value else 0)


def checksum_save(data):
    for base, end, checksum in (
        ("sGameData", "sGameDataEnd", "sChecksum"),
        ("sBackupGameData", "sBackupGameDataEnd", "sBackupChecksum"),
    ):
        start = sram(checksum)
        data[start:start + 2] = (sum(data[sram(base):sram(end)]) & 0xffff).to_bytes(2, "little")


def box_checksum(mon):
    return (127 + sum((i + 1) * mon[i] for i in range(32))
            + sum((i + 2) * (mon[i] & 127) for i in range(32, 49))) & 0xffff


def sign_box(mon):
    checksum = box_checksum(mon)
    for index in range(17):
        bit = (checksum >> (15 - index)) & 1 if index < 16 else 0
        mon[32 + index] = (mon[32 + index] & 127) | (bit << 7)
    return mon


class Version11Tests(unittest.TestCase):
    def test_story_scene_insertions(self):
        # These bytes did not move, but new scenes changed their meaning.
        # Sweep even invalid values so the migration cannot reset other progress.
        for before in range(256):
            source = self.base()
            source[saved("wBurnedTower1FSceneID")] = before
            source[saved("wOlivineCitySceneID")] = before
            scene_names = [name for name in SYM if name.startswith("w") and name.endswith("SceneID")
                           and SYM[name][0] == SYM["wPlayerData"][0]
                           and SYM["wPlayerData"][1] <= SYM[name][1] < SYM["wPlayerDataEnd"][1]
                           and name not in ("wBurnedTower1FSceneID", "wOlivineCitySceneID", "wPokecenter2FSceneID")]
            for index, name in enumerate(scene_names):
                source[saved(name)] = index % 9
            checksum_save(source)
            output = self.patch(source)
            self.assertEqual(output[saved("wBurnedTower1FSceneID")],
                             before + 1 if before in (1, 2) else before)
            self.assertEqual(output[saved("wOlivineCitySceneID")], 2 if before == 1 else before)
            for name in scene_names:
                self.assertEqual(output[saved(name)], source[saved(name)], name)

    def test_saved_layout_matches_version10(self):
        old = symbols(ROOT / "resources/version10/version10.sym")
        aliases = {
            "wUndergroundSwitchPositions": "wFollowInSync",
            "wFarawayIslandSceneID": "wFarawayIslandSouthSceneID",
            "wUndergroundPathSwitchRoomEntrancesSceneID": "wGoldenrodUndergroundSwitchRoomSceneID",
        }
        for name, (bank, address) in old.items():
            if name.startswith("s") and 0xa000 <= address < 0xc000:
                self.assertEqual(SYM[name], (bank, address), name)
            for first, last in (("wOptions3", "wOptionsEnd"), ("wPlayerData", "wPlayerDataEnd"), ("wCurMapData", "wCurMapDataEnd"),
                                ("wPokemonData", "wPokemonDataEnd")):
                if bank == old[first][0] and old[first][1] <= address <= old[last][1]:
                    destination = aliases.get(name, name)
                    self.assertEqual(address - old[first][1], SYM[destination][1] - SYM[first][1], name)

    def base(self):
        return bytearray((ROOT / "tests/verified-saves/MrKat-Save/v10/patched_save.sav").read_bytes())

    def patch(self, data, target="11", success=True):
        with tempfile.TemporaryDirectory() as directory:
            source, output = Path(directory) / "in.sav", Path(directory) / "out.sav"
            source.write_bytes(data)
            args = [str(PATCHER), str(source), str(output)]
            if target is not None:
                args.append(target)
            result = subprocess.run(args, capture_output=True, text=True)
            logs = result.stdout + result.stderr
            self.assertEqual(result.returncode == 0, success, logs)
            self.assertEqual(source.read_bytes(), data, "Input was modified")
            if not success:
                self.assertFalse(output.exists(), logs)
                return None
            self.assertNotIn("error:", logs.lower(), logs)
            patched = output.read_bytes()
            self.assertEqual(len(patched), len(data))
            self.assertEqual(patched[0x8000:], data[0x8000:], "RTC/emulator trailer changed")
            self.assertEqual(int.from_bytes(patched[sram("sSaveVersion"):sram("sSaveVersion") + 2], "big"), 11)
            for base, end, checksum in (
                ("sGameData", "sGameDataEnd", "sChecksum"),
                ("sBackupGameData", "sBackupGameDataEnd", "sBackupChecksum"),
            ):
                self.assertEqual(sum(patched[sram(base):sram(end)]) & 0xffff,
                                 int.from_bytes(patched[sram(checksum):sram(checksum) + 2], "little"))
            self.assertEqual(patched[sram("sGameData"):sram("sGameDataEnd")],
                             patched[sram("sBackupGameData"):sram("sBackupGameDataEnd")])
            return patched

    def test_all_existing_versions_chain_to_11(self):
        for player in (ROOT / "tests/verified-saves").iterdir():
            expected = self.patch((player / "v10/patched_save.sav").read_bytes())
            for version in (7, 8, 9):
                path = next((player / f"v{version}").glob("*.sav")) if version == 7 else player / f"v{version}/patched_save.sav"
                with self.subTest(player=player.name, version=version):
                    self.assertEqual(self.patch(path.read_bytes(), target=None), expected)

    def test_reject_invalid_inputs_and_repeat_or_downgrade(self):
        base = self.base()
        for offset in (sram("sGameData"), sram("sBackupGameData")):
            data = base.copy()
            data[offset] ^= 1
            self.patch(data, success=False)
        data = base.copy()
        data[sram("sWritingBackup")] = 1
        self.patch(data, success=False)
        data = base.copy()
        data[saved("wMapGroup")] = 1
        checksum_save(data)
        self.patch(data, success=False)
        self.patch(b"too short", success=False)
        self.patch(base, target="10", success=False)
        self.patch(base, target="9", success=False)
        self.patch(base, target="12", success=False)
        self.patch(self.patch(base), success=False)

    def test_dex_and_fly_bit_insertions(self):
        data = self.base()
        for name in ("wPokedexSeen", "wPokedexCaught"):
            address = saved(name)
            # Different pattern for seen/caught, with all boundary positions set.
            for bit in range(400):
                setflag(data, address, bit, bit % (3 if name.endswith("Seen") else 5) == 0
                        or bit in (318, 319, 320, 321, 322, 392, 399))
        for bit in range(40):
            setflag(data, saved("wVisitedSpawns"), bit, bit % 2 == 0 or bit == 33)
        checksum_save(data)
        result = self.patch(data)
        for name in ("wPokedexSeen", "wPokedexCaught"):
            address = saved(name)
            expected = [getflag(data, address, i) for i in range(393)]
            expected.insert(319, False)
            self.assertEqual([getflag(result, address, i) for i in range(400)], expected + [False] * 6)
        address = saved("wVisitedSpawns")
        expected = [getflag(data, address, i) for i in range(34)]
        expected.insert(15, expected[14])
        self.assertEqual([getflag(result, address, i) for i in range(40)], expected + [False] * 5)

    def test_forms_in_every_saved_pokemon_format(self):
        data = self.base()
        records = []
        # Include normal, special, egg/female, and extended-species lookalikes.
        for i, form in enumerate((1, 2, 3, 4, 0xc5, 0x23)):
            address = saved("wPartyMon1") + 48 * i
            data[address], data[address + 21] = 24, form
            records.append((address + 21, form + 1 if i in (2, 3, 4) else form))
        for name in ("wBreedMon1", "wBreedMon2", "wLevelUpMon", "wContestMon"):
            address = saved(name)
            data[address], data[address + 21] = 24, 0x85
            records.append((address + 21, 0x86))
        for name in ("wRoamMon1", "wRoamMon2", "wRoamMon3"):
            address = saved(name)
            data[address], data[address + 9] = 24, 3
            records.append((address + 9, 4))
        address = saved("wHiddenGrottoContents")
        data[address:address + 6] = bytes((1, 24, 5, 2, 24, 5))
        records.extend(((address + 2, 6), (address + 5, 5)))
        for team in range(10):
            for slot in range(6):
                address = sram("sHallOfFame") + 98 * team + 1 + 16 * slot
                data[address], data[address + 4] = 24, 4
                records.append((address + 4, 5))
        boxes = []
        for bank in (1, 2):
            for part in "ABC":
                start, end = sram(f"sBoxMons{bank}{part}Mons"), sram(f"sBoxMons{bank}{part}End")
                for address in (start, end - 49):
                    mon = bytearray((i * 7) & 127 for i in range(49))
                    mon[0], mon[21] = 24, 0xc3
                    data[address:address + 49] = sign_box(mon)
                    boxes.append(address)
        bad = sram("sBoxMons1AMons") + 49
        data[bad:bad + 49] = data[boxes[0]:boxes[0] + 49]
        data[bad + 1] ^= 1
        checksum_save(data)
        result = self.patch(data)
        for address, expected in records:
            self.assertEqual(result[address], expected, hex(address))
        for address in boxes:
            expected = bytearray(data[address:address + 49])
            expected[21] = 0xc4
            self.assertEqual(result[address:address + 49], sign_box(expected))
        self.assertEqual(result[bad:bad + 49], data[bad:bad + 49])

    def test_events_defaults_puzzle_and_new_scratch_space(self):
        data = self.base()
        events = saved("wEventFlags")
        for bit in range(606, 621):
            setflag(data, events, bit)
        for bit in range(2259, 2304):
            setflag(data, events, bit)
        setflag(data, events, 1070)  # old Firebreather Dick, now Cyd
        for start, end in (("wOvercastRandomDay", "wCandyAmounts"), ("wPaletteSwapAddress", "wTimeOfDayPalFlags")):
            data[saved(start):saved(end)] = b"\xa5" * (saved(end) - saved(start))
        data[saved("wFollowInSync")] = 0xf3
        data[saved("wDailyTrainerHouseOpponent")] = 4
        data[sram("sOptions3")] = 7
        checksum_save(data)
        result = self.patch(data)
        for bit in range(606, 621):
            self.assertEqual(getflag(result, events, bit), bit == 620)
        for bit in range(2259, 2287):
            self.assertEqual(getflag(result, events, bit), bit in (2259, 2261, 2270))
        self.assertTrue(getflag(result, events, 1070))
        self.assertFalse(getflag(result, events, 2268))  # new Dick is a different trainer
        self.assertEqual(result[saved("wFollowInSync")], 0)
        self.assertEqual(result[saved("wDailyTrainerHouseOpponent")], 5)
        self.assertEqual(result[sram("sOptions3")], 1)
        self.assertEqual(result[saved("wOvercastRandomDay")], 255)
        self.assertEqual(set(result[saved("wOvercastRandomDay") + 1:saved("wCandyAmounts")]), {0})
        self.assertEqual(result[saved("wCandyAmounts"):saved("wWingAmounts")], data[saved("wCandyAmounts"):saved("wWingAmounts")])

    def test_map_translation_all_changed_pairs_and_sentinels(self):
        # Generated map table is source-derived, not the C++ translation logic.
        header = (ROOT / "include/patching/Version11Maps.h").read_text().split("struct CenterWarp")[0]
        changes = [tuple(map(int, row)) for row in re.findall(r"\{(\d+), (\d+), (\d+), (\d+)\}", header)]
        changes += [(0, 0, 0, 0), (255, 255, 255, 255), (1, 1, 1, 1)]
        names = ("wLastSpawnMapGroup", "wYanmaMapGroup", "wDunsparceMapGroup",
                 "wRoamMon1MapGroup", "wRoamMon2MapGroup", "wRoamMon3MapGroup")
        for index in range(0, len(changes), len(names)):
            data = self.base()
            batch = list(zip(names, changes[index:index + len(names)]))
            for name, (a, b, _, _) in batch:
                data[saved(name):saved(name) + 2] = bytes((a, b))
            checksum_save(data)
            result = self.patch(data)
            for name, (_, _, a, b) in batch:
                self.assertEqual(result[saved(name):saved(name) + 2], bytes((a, b)), name)

    def test_all_option_bytes_primary_and_backup(self):
        # Exercise every byte value, including dirty reserved bits, independently
        # in the two copies. No reset of initial choices is needed for v11.
        for value in range(256):
            data = self.base()
            expected = []
            for copy, (block, extra) in enumerate((("sOptions", "sOptions3"),
                                                  ("sBackupOptions", "sBackupOptions3"))):
                for offset in range(SYM["wOptionsEnd"][1] - SYM["wOptions"][1]):
                    address = sram(block) + offset
                    original = (value + offset * 31 + copy * 113) % 256
                    data[address] = original
                    is_textbox_flags = offset == SYM["wTextboxFlags"][1] - SYM["wOptions"][1]
                    expected.append((address, original & ~0x20 if is_textbox_flags else original))
                address = sram(extra)
                data[address] = (value + copy * 113) % 256
                expected.append((address, data[address] & ~0x06))
            result = self.patch(data)
            for address, expected_byte in expected:
                self.assertEqual(result[address], expected_byte, (value, hex(address)))

    def test_existing_legendary_roamers_gain_plain_form(self):
        # The entire old form byte was uninitialized, including extspecies.
        for stale in (0, 1, 0x20, 0xff):
            data = self.base()
            for i in range(3):
                address = saved(f"wRoamMon{i + 1}")
                data[address] = 243 + i
                data[address + 9] = stale
                data[address + 2:address + 4] = bytes((1, 1))  # unchanged map
            checksum_save(data)
            result = self.patch(data)
            for i in range(3):
                address = saved(f"wRoamMon{i + 1}")
                self.assertEqual(result[address:address + 9], data[address:address + 9])
                self.assertEqual(result[address + 9], 1)
        data = self.base()
        for i, species in enumerate((0, 255, 25)):
            address = saved(f"wRoamMon{i + 1}")
            data[address], data[address + 9] = species, 0xe5
        checksum_save(data)
        result = self.patch(data)
        for i in range(3):
            self.assertEqual(result[saved(f"wRoamMon{i + 1}") + 9], 0xe5)

    def test_facility_set_ids_all_tiers_and_slots(self):
        # Independent enumeration of old tables: D=61, C=140, B=143, A=88.
        # Check all 432 valid IDs in the rental party AND every history slot.
        # Include empty/random sentinels and out-of-range values unchanged.
        pairs = [(tier, index) for tier, size in enumerate((61, 140, 143, 88)) for index in range(size)]
        pairs += [(tier, index) for tier in range(4) for index in (254, 255)]
        pairs += [(2, 143), (3, 88), (4, 51), (255, 101), (255, 255)]
        parties = ("sBT_MonParty", "sBT_OTMonParty1", "sBT_OTMonParty2", "sBT_OTMonParty3")
        for first in range(0, len(pairs), 3):
            data = self.base()
            data[sram("sBattleTowerChallengeState")] = 0x0a  # rental, in progress
            data[sram("sBT_CurTrainerAndSwap")] = 0x2b
            data[sram("sBTTrainers"):sram("sBTTrainers") + 7] = bytes((0, 3, 51, 101, 131, 132, 133))
            batch = pairs[first:first + 3]
            for name in parties:
                for slot, pair in enumerate(batch):
                    data[sram(name) + slot * 2:sram(name) + slot * 2 + 2] = bytes(pair)
            result = self.patch(data)
            self.assertEqual(result[sram("sBattleTowerChallengeState"):sram("sBT_MonParty")],
                             data[sram("sBattleTowerChallengeState"):sram("sBT_MonParty")])
            for name in parties:
                for slot, (tier, index) in enumerate(batch):
                    expected = index + int(tier == 2 and 101 <= index < 143 or tier == 3 and 51 <= index < 88)
                    self.assertEqual(result[sram(name) + slot * 2:sram(name) + slot * 2 + 2],
                                     bytes((tier, expected)), (name, tier, index))
            for name in ("wBattleTowerCurStreak", "wBattleTowerTopStreak", "wBattleFactoryCurStreak",
                         "wBattleFactoryTopStreak", "wBattleFactorySwapCount"):
                self.assertEqual(result[saved(name):saved(name) + 2], data[saved(name):saved(name) + 2])
        # Same bytes alias party selection data when rental mode is not set.
        for state in range(5):
            data = self.base()
            data[sram("sBattleTowerChallengeState")] = state
            address = sram("sBT_PartySelections")
            data[address:address + 6] = bytes((2, 101, 3, 51, 2, 142))
            result = self.patch(data)
            self.assertEqual(result[address:address + 6], data[address:address + 6])

    def test_return_warps_including_removed_snowtop_stairs(self):
        # Old Indigo map 16:3 moved to 16:5; Snowtop 36:5 lost its upstairs.
        for before, after, warp in (((16, 3), (16, 5), 3), ((36, 5), (36, 5), 1), ((0, 0), (11, 24), 4)):
            data = self.base()
            data[saved("wJohtoBadges")] |= 4
            data[saved("wBackupMapGroup"):saved("wBackupMapGroup") + 2] = bytes(before)
            data[saved("wBackupWarpNumber")] = 250
            checksum_save(data)
            result = self.patch(data)
            self.assertEqual(result[saved("wBackupMapGroup"):saved("wBackupMapGroup") + 2], bytes(after))
            self.assertEqual(result[saved("wBackupWarpNumber")], warp)
        data[saved("wJohtoBadges")] = 0
        checksum_save(data)
        result = self.patch(data)
        self.assertEqual(result[saved("wBackupMapGroup"):saved("wBackupMapGroup") + 2], bytes((26, 6)))
        self.assertEqual(result[saved("wBackupWarpNumber")], 3)

    def test_preserve_unrelated_save_data(self):
        data = self.base()
        result = self.patch(data)
        for start, end in (("wPlayerID", "wObjectFollow_Leader"), ("wMoney", "wPlayerState"),
                           ("wCandyAmounts", "wCurMapSceneScriptPointer"), ("wPartyMonOTs", "wPokedexFlags")):
            # Daily trainer ID is zero in this fixture and remains zero.
            self.assertEqual(result[saved(start):saved(end)], data[saved(start):saved(end)], start)
        for start, end in (("sPartyMail", "sMailbox10BackupEnd"), ("sNewBox1", "sNewBoxEnd")):
            self.assertLess(sram(start), sram(end))
            self.assertEqual(result[sram(start):sram(end)], data[sram(start):sram(end)], start)


if __name__ == "__main__":
    unittest.main()
