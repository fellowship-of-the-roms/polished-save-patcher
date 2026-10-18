#!/usr/bin/env python3
"""Cross-check scene encodings throughout the pinned v11 migration range.

Usage: python3 tools/audit_v11_scenes.py /path/to/polishedcrystal
Reads both revisions with git archive; no checkout or network access is needed.
This is a source audit, not a proof of gameplay correctness. Coordinate changes
are deliberately ignored here; the migration requires the unchanged upstairs.
"""

import io
import re
import subprocess
import sys
import tarfile
from collections import Counter
from pathlib import Path

from generate_v11_maps import OLD_REV, NEW_REV, RENAMES, read


def sources(repo, rev):
    archive = subprocess.check_output(["git", "-C", str(repo), "archive", rev, "maps"])
    with tarfile.open(fileobj=io.BytesIO(archive)) as files:
        return {Path(member.name).stem: files.extractfile(member).read().decode()
                for member in files if member.isfile() and member.name.endswith(".asm")}


def scene_constants(maps):
    values = {"SCENE_ALWAYS": -1}
    for source in maps.values():
        index = 0
        for line in source.splitlines():
            match = re.match(r"\s+scene_(const|script) ([^;]+)", line)
            if match:
                args = match[2].split(",")
                if match[1] == "const" or len(args) == 2:
                    values[args[-1].strip()] = index
                index += 1
    return values


def value(text, constants):
    text = text.strip()
    return constants[text] if text in constants else int(text.replace("$", "0x"), 0)


def commands(source, constants):
    result = Counter()
    for line in source.splitlines():
        match = re.match(r"\s+(setscene|setmapscene|coord_event) (.+)", line.split(";")[0])
        if not match:
            continue
        args = [arg.strip() for arg in match[2].split(",")]
        if match[1] == "setscene":
            result[("setscene", value(args[0], constants))] += 1
        elif match[1] == "setmapscene":
            result[("setmapscene", RENAMES.get(args[0], args[0]), value(args[1], constants))] += 1
        else:
            result[("coord_event", value(args[2], constants))] += 1
    return result


def first_scene_assignment(source, label, constants):
    body = source.split(label + ":", 1)[1]
    return value(re.search(r"^\s+setscene (\S+)", body, re.M)[1], constants)


def main():
    repo = Path(sys.argv[1])
    old, new = [sources(repo, rev) for rev in (OLD_REV, NEW_REV)]
    constants = scene_constants(new)
    aliases = {
        "FarawayIsland": ["FarawayIslandSouth"],
        "UndergroundPathSwitchRoomEntrances": ["GoldenrodUndergroundSwitchRoom"],
        "Route23": ["Route23North", "Route23South"],
    }
    differences = {}
    checked = 0
    for name, source in old.items():
        targets = aliases.get(name, [name])
        before = commands(source, {})
        if not all(target in new for target in targets):
            assert not before, f"Unreviewed removed map with scene commands: {name}"
            continue
        after = sum((commands(new[target], constants) for target in targets), Counter())
        checked += bool(before or after)
        if before != after:
            differences[name] = before - after, after - before
    # The remaining changes were traced in the audit document. This also
    # includes debug-only state setup in PlayersHouse2F, not just scene_script.
    assert set(differences) == {
        "BurnedTower1F", "OlivineCity", "PlayersHouse2F", "CeruleanCity",
        "Route24", "RuinsOfAlphOutside", "UndergroundPathSwitchRoomEntrances",
    }, differences
    for name, (removed, added) in sorted(differences.items()):
        print(f"{name}: removed {dict(removed)}; added {dict(added)}")

    # Follow the same script's meaning across revisions, rather than assuming
    # that an unchanged RAM offset implies an unchanged scene encoding.
    for name, label, expected in (
        ("BurnedTower1F", "BurnedTowerRivalBattleScript", (2, 3)),
        ("OlivineCity", "OlivineCityRivalGymScript", (1, 2)),
    ):
        actual = (first_scene_assignment(old[name], label, {}),
                  first_scene_assignment(new[name], label, constants))
        assert actual == expected, (name, actual)
    for source, values, expected in ((old["BurnedTower1F"], {}, 1),
                                     (new["BurnedTower1F"], constants, 2)):
        match = re.search(r"^\s+coord_event\s+\d+,\s*\d+,\s*([^,]+), BurnedTowerRivalBattleScript$", source, re.M)
        assert value(match[1], values) == expected
    assert constants["SCENE_BURNEDTOWER1F_MEET_EUSINE"] == 0
    assert constants["SCENE_OLIVINECITY_RIVAL_ENCOUNTER"] == 0

    # Every existing map-scene binding retains its saved field. Added aliases
    # share existing state; they do not allocate uninitialized scene bytes.
    old_vars, new_vars = [dict(re.findall(r"^\s+scene_var (\w+),\s*(\w+)",
                            read(repo, rev, "data/maps/scenes.asm"), re.M)) for rev in (OLD_REV, NEW_REV)]
    ram_aliases = {
        "wFarawayIslandSceneID": "wFarawayIslandSouthSceneID",
        "wUndergroundPathSwitchRoomEntrancesSceneID": "wGoldenrodUndergroundSwitchRoomSceneID",
    }
    for name, field in old_vars.items():
        assert new_vars[RENAMES.get(name, name)] == ram_aliases.get(field, field), name
    assert new_vars["ROUTE_23_SOUTH"] == new_vars["ROUTE_23_NORTH"] == "wRoute23SceneID"
    assert new_vars["CERULEAN_CITY"] == new_vars["ROUTE_24"] == "wRoute24SceneID"
    print(f"Checked {checked} maps with scene commands and all {len(old_vars)} existing scene bindings")
    print("Required scene conversions: Burned Tower 1->2, 2->3; Olivine 1->2")


if __name__ == "__main__":
    main()
