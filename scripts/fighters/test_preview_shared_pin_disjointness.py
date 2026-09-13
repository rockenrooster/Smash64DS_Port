#!/usr/bin/env python3
"""Keep compact CSS shared pins disjoint from every FPC1 section asset ID."""

from __future__ import annotations

import re
import tempfile
from pathlib import Path

import generate_preview_core_packs as packs


ROOT = Path(__file__).resolve().parents[2]
PLAYERS_VS = ROOT / "src/import/battleship_mnplayersvs.c"


def compact_shared_pins() -> set[int]:
    source = PLAYERS_VS.read_text(encoding="utf-8")
    match = re.search(
        r"sNdsPlayersVSSharedResidentAssetIDs\[\]\s*=\s*\{(.*?)\n\};",
        source,
        re.S,
    )
    if match is None:
        raise AssertionError("shared CSS resident pin list was not found")

    pins: set[int] = set()
    include = True
    for raw in match.group(1).splitlines():
        line = raw.strip()
        if line == "#if !NDS_PLAYERS_VS_COMPACT_PREVIEW":
            include = False
            continue
        if line == "#endif":
            include = True
            continue
        if include:
            pins.update(int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]+)u", line))
    if not pins:
        raise AssertionError("compact CSS resident pin list is empty")
    return pins


def main() -> int:
    pins = compact_shared_pins()
    collisions: list[str] = []
    with tempfile.TemporaryDirectory(prefix="preview-pin-check-") as temp:
        output = Path(temp) / "preview-core"
        if packs.main(["--output-dir", str(output)]) != 0:
            raise AssertionError("preview-pack generation failed")
        paths = sorted(output.glob("*.fpc"))
        if len(paths) != len(packs.KIND_ORDER):
            raise AssertionError(
                f"expected {len(packs.KIND_ORDER)} preview packs, got {len(paths)}"
            )
        for path in paths:
            decoded = packs.decode_pack(path.read_bytes())
            for index, section in enumerate(decoded["sections"]):
                asset_id = int(section[0])
                if asset_id in pins:
                    collisions.append(
                        f"{path.name}:section{index}=0x{asset_id:03x}"
                    )
    if collisions:
        raise AssertionError(
            "compact CSS shared pin collides with FPC1 section asset IDs: "
            + ", ".join(collisions)
        )
    print(
        f"Preview shared-pin disjointness: PASS ({len(pins)} pins, "
        f"{len(packs.KIND_ORDER)} packs)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
