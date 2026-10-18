#!/usr/bin/env python3
"""Verify v11 option/facility assumptions against the pinned upstream sources.

Usage: python3 tools/audit_v11_tables.py /path/to/polishedcrystal
Optional: append /path/to/old.gbc /path/to/new.gbc to verify assembled sets too.
The ROMs must be built from the pinned revisions with matching .sym files.
No checkout, network access, or writes are performed.
"""

import re
import subprocess
import sys
from pathlib import Path

from generate_v11_maps import OLD_REV, NEW_REV, read


def tiers(source):
    return [[line.strip() for line in re.findall(r"^\s+dp (.+)$", part, re.M)]
            for part in source.split("db -1")[:4]]


def option_constants(source):
    source = source.split("; wOptionsMenuDescriptionState::")[0].split("; wForgettingMove::")[0]
    return [line.split(";")[0].strip() for line in source.splitlines()
            if line.split(";")[0].strip()
            and not any(name in line for name in ("NICKNAMES_ALWAYS", "NICKNAMES_NEVER", "NO_TEXT_PAUSE_F"))]


def rom_tiers(path):
    rom = path.read_bytes()
    symbol = re.search(r"^([0-9a-f]+):([0-9a-f]+) BattleTowerMons$",
                       path.with_suffix(".sym").read_text(), re.M)
    bank, address = (int(value, 16) for value in symbol.groups())
    offset = bank * 0x4000 + address - 0x4000
    result = []
    for _ in range(4):
        records = []
        while rom[offset] != 255:
            records.append(rom[offset:offset + 9])
            offset += 9
        result.append(records)
        offset += 1
    return result


def main():
    repo = Path(sys.argv[1])
    before, after = [tiers(read(repo, rev, "data/battle_tower/parties.asm")) for rev in (OLD_REV, NEW_REV)]
    assert [len(tier) for tier in before] == [61, 140, 143, 88]
    assert [len(tier) for tier in after] == [61, 140, 144, 89]
    insertions = {2: 101, 3: 51}
    for tier in range(4):
        expected = list(before[tier])
        if tier in insertions:
            expected.insert(insertions[tier], "URSALUNA, MALE | URSALUNA_BLOODMOON_FORM")
        assert expected == after[tier], f"Unreviewed set change in tier {tier}"
    print("Facility sets: all 432 old identities preserved; insert B:101 and A:51")

    trainers = []
    for rev in (OLD_REV, NEW_REV):
        trainers.append([(name.rstrip("@"), kind) for name, kind in
                         re.findall(r'rawchar "([^"]+)", (\w+)', read(repo, rev, "data/battle_tower/classes.asm"))])
    assert trainers[0] == trainers[1] and len(trainers[0]) == 134
    print("Facility trainers: all 134 indexes unchanged (name padding/class IDs live in ROM)")

    assert option_constants(read(repo, OLD_REV, "constants/ram_constants.asm")) == option_constants(
        read(repo, NEW_REV, "constants/ram_constants.asm"))
    print("Options: existing encodings unchanged; only nickname and NO_TEXT_PAUSE bits added")

    # Discovered via gh pr list --state merged --label version/minor. Check
    # ancestry, not merge dates: #1301 predates the tag date but is absent from it.
    for number, commit, in_old in (
        (1566, "9b25b5273f11942ef7013b28ce04272035a24da8", False),
        (1313, "88efa64840fd0b0f36386ae0f36db8af78616ff9", False),
        (1301, "c91896377b05bea2d1fa5e55770e506b366b6a22", False),
        (1137, "16d44d82d30921785362ba5af668724b8c16199e", True),
        (993, "462a1329f1dff7791ea681b4ded81781faeaaca7", True),
    ):
        for rev, expected in ((OLD_REV, in_old), (NEW_REV, True)):
            status = subprocess.run(["git", "-C", str(repo), "merge-base", "--is-ancestor", commit, rev]).returncode
            assert status in (0, 1) and (status == 0) == expected, (number, rev, status)
        print(f"PR #{number}: {'already in v3.2.3' if in_old else 'requires v11 audit'}")

    if len(sys.argv) == 4:
        old_rom, new_rom = map(rom_tiers, map(Path, sys.argv[2:]))
        changes = []
        for tier in range(4):
            assert len(old_rom[tier]) == len(before[tier]) and len(new_rom[tier]) == len(after[tier])
            for index, record in enumerate(old_rom[tier]):
                new_index = index + int(tier in insertions and index >= insertions[tier])
                target = new_rom[tier][new_index]
                if record[0] == 24 and not record[1] & 0x20 and 3 <= record[1] & 0x1f <= 5:
                    record = record[:1] + bytes((record[1] + 1,)) + record[2:]  # Arbok form insertion
                assert record[:8] == target[:8], (tier, index, record, target)
                if record != target:
                    assert before[tier][index] == "RAICHU, FEMALE"
                    assert record[8] & 0x1f == target[8] & 0x1f  # same nature
                    assert (record[8] & 0x60, target[8] & 0x60) == (0x60, 0x40)
                    changes.append(before[tier][index])
        assert changes == ["RAICHU, FEMALE"], changes  # Lightning Rod moved from hidden to slot 2
        print("Built ROMs: all 432 migrated sets retain species/form identity, item, moves and DVs")
        print("Ability-only differences:", changes)


if __name__ == "__main__":
    main()
