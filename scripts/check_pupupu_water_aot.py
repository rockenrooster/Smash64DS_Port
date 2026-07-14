#!/usr/bin/env python3
"""Focused deterministic checks for generate_pupupu_water_aot.py."""

from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path
from typing import Sequence

import generate_pupupu_water_aot as generator


DEFAULT_FIXTURE = Path(__file__).resolve().parent / "fixtures" / (
    "pupupu_water_aot_expected.json"
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def load_fixture(path: Path) -> dict[str, object]:
    if not path.is_file():
        raise AssertionError(f"expected fixture is absent: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def check_missing_and_corrupt_corpus(repo_root: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="pupupu-water-aot-negative-") as temp:
        empty_root = Path(temp) / "empty"
        try:
            generator.load_source_corpus(empty_root)
        except generator.Falsifier as exc:
            message = str(exc)
            require(message.startswith("FALSIFIER:"), "missing corpus lost falsifier tag")
            require("corpus is absent" in message, "missing corpus diagnostic is unclear")
        else:
            raise AssertionError("missing O2R corpus was accepted")

        corrupt_root = Path(temp) / "corrupt"
        corrupt_path = corrupt_root / generator.BANK103_RELATIVE
        corrupt_path.parent.mkdir(parents=True, exist_ok=True)
        source = bytearray((repo_root / generator.BANK103_RELATIVE).read_bytes())
        source[-1] ^= 0x01
        corrupt_path.write_bytes(source)
        try:
            generator.load_source_corpus(corrupt_root)
        except generator.Falsifier as exc:
            message = str(exc)
            require(message.startswith("FALSIFIER:"), "corrupt corpus lost falsifier tag")
            require("SHA256" in message, "corrupt corpus diagnostic omitted its hash")
        else:
            raise AssertionError("corrupt O2R corpus was accepted")


def check_artifacts(repo_root: Path, fixture_path: Path) -> generator.GeneratedArtifacts:
    expected = load_fixture(fixture_path)
    first = generator.generate(repo_root)
    require(
        first.fixture_summary() == expected,
        "generated census/artifact hashes differ from the pinned fixture",
    )

    # A second complete source simulation/conversion catches accidental host
    # state, iteration-order, or output-directory dependence.
    second = generator.generate(repo_root)
    require(first.files() == second.files(), "two complete generations differ")
    require(
        second.fixture_summary() == expected,
        "second generation differs from the pinned fixture",
    )

    with tempfile.TemporaryDirectory(prefix="pupupu-water-aot-output-") as temp:
        output_dir = Path(temp)
        generator.write_artifacts(first, output_dir)
        require(
            sorted(path.name for path in output_dir.iterdir())
            == sorted(first.files()),
            "output directory contains an unexpected artifact set",
        )
        for name, expected_payload in first.files().items():
            require(
                (output_dir / name).read_bytes() == expected_payload,
                f"written artifact {name} differs from its in-memory bytes",
            )
    generator.verify_index(first.index, first.payload)
    return first


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="checkout containing the pinned decomp/BattleShip-main O2R corpus",
    )
    parser.add_argument(
        "--fixture",
        type=Path,
        default=DEFAULT_FIXTURE,
        help="pinned compact expected-result JSON",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    repo_root = args.repo_root.resolve()
    try:
        check_missing_and_corrupt_corpus(repo_root)
        artifacts = check_artifacts(repo_root, args.fixture.resolve())
    except (AssertionError, generator.Falsifier, OSError, ValueError) as exc:
        print(f"PUPUPU_WATER_AOT_FIXTURES_FAIL: {exc}")
        return 1

    fixture = artifacts.fixture_summary()
    census = fixture["census"]
    artifact_hashes = fixture["artifacts"]
    assert isinstance(census, dict)
    assert isinstance(artifact_hashes, dict)
    print(
        "PUPUPU_WATER_AOT_FIXTURES_OK "
        f"frames={census['simulated_frames']} cycle={census['cycle_frames']} "
        f"keys={census['total_keys']} outputs={census['total_outputs']} "
        f"oracle_pixels={census['oracle_pixels']} payload_bytes={artifact_hashes['payload_bytes']} "
        f"payload_sha256={artifact_hashes['payload_sha256']} "
        f"index_sha256={artifact_hashes['index_sha256']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
