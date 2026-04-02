#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="Verify golden save fixtures against the CLI patcher.")
    parser.add_argument(
        "--patcher",
        type=Path,
        default=repo_root / "build" / "polished_save_patcher",
        help="Path to the CLI patcher binary.",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=repo_root / "tests" / "fixture_manifest.json",
        help="Path to the fixture manifest JSON file.",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=repo_root,
        help="Repository root used to resolve manifest paths.",
    )
    return parser.parse_args()


def first_diff_offset(expected: bytes, actual: bytes) -> int | None:
    for offset, (expected_byte, actual_byte) in enumerate(zip(expected, actual)):
        if expected_byte != actual_byte:
            return offset
    if len(expected) != len(actual):
        return min(len(expected), len(actual))
    return None


def run_case(case: dict[str, object], patcher: Path, repo_root: Path) -> None:
    case_name = str(case["name"])
    input_path = repo_root / str(case["input"])
    expected_path = repo_root / str(case["expected"])
    target_version = str(case["target_version"])

    with tempfile.TemporaryDirectory(prefix="fixture-verify-") as temp_dir:
        output_path = Path(temp_dir) / "patched_save.sav"
        completed = subprocess.run(
            [str(patcher), str(input_path), str(output_path), target_version],
            capture_output=True,
            text=True,
            check=False,
        )

        if completed.returncode != 0:
            raise AssertionError(
                f"{case_name}: patcher exited with {completed.returncode}\n"
                f"stdout:\n{completed.stdout}\n"
                f"stderr:\n{completed.stderr}"
            )

        if not output_path.exists():
            raise AssertionError(f"{case_name}: patcher did not produce {output_path}")

        expected_bytes = expected_path.read_bytes()
        actual_bytes = output_path.read_bytes()
        if expected_bytes != actual_bytes:
            diff_offset = first_diff_offset(expected_bytes, actual_bytes)
            raise AssertionError(
                f"{case_name}: output mismatch at offset {diff_offset}\n"
                f"input={input_path}\n"
                f"expected={expected_path}\n"
                f"actual={output_path}\n"
                f"expected_size={len(expected_bytes)} actual_size={len(actual_bytes)}"
            )


def main() -> int:
    args = parse_args()
    patcher = Path(args.patcher).resolve()
    manifest_path = Path(args.manifest).resolve()
    repo_root = Path(args.repo_root).resolve()

    if not patcher.is_file():
        print(f"Patcher binary not found: {patcher}", file=sys.stderr)
        return 1

    if not manifest_path.is_file():
        print(f"Fixture manifest not found: {manifest_path}", file=sys.stderr)
        return 1

    cases = json.loads(manifest_path.read_text(encoding="utf-8"))
    if not isinstance(cases, list) or not cases:
        print(f"Fixture manifest is empty: {manifest_path}", file=sys.stderr)
        return 1

    for case in cases:
        run_case(case, patcher, repo_root)
        print(f"PASS {case['name']}")

    print(f"Verified {len(cases)} fixture cases.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
