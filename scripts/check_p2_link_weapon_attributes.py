#!/usr/bin/env python3
"""Byte-exact Link weapon WPAttributes oracle for the DS relocation fixup."""

from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

import generate_battle_playable_texture_census as census


SPECS = (
    (
        "spin",
        census.InputSpec(
            Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/LinkMain"),
            "8a771a6e2b9e9d8e4b1b103d4c2be332e7290c36adf47c324822c2957797a005",
            225,
        ),
        0x0C,
        (0x00044642, 0x0005442A, 0x00064664, 0x00104690),
        (0x01900780, 0x0C814000, 0x01A10184, 0x07800000),
    ),
    (
        "boomerang",
        census.InputSpec(
            Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/LinkSpecial1"),
            "ffa3b112d4d57eb1c66604b16da7de72a7a538100de8278dd4ca50be5e1fa84d",
            226,
        ),
        0x00,
        (0x00020184, 0x00000000, 0xFFFF01B0, 0x00000000),
        (0x00C81180, 0x07824000, 0x01A0F9B4, 0x0DC00000),
    ),
)

WPATTR_BYTES = 52
S16_RUN_FIRST = 0x10
S16_RUN_END = 0x24
EXPECTED_S16 = (0, 0, 0, 0, 0, 0, 150, 0, -150, 150)


def ds_normalize_wpattributes(source: bytes) -> bytes:
    """Replay word-byte swap, then the runtime's mixed-s16 lane repair."""
    if len(source) != WPATTR_BYTES:
        raise RuntimeError(f"WPAttributes slice is {len(source)} bytes, expected 52")
    result = bytearray(source)
    for offset in range(0, len(result), 4):
        result[offset : offset + 4] = result[offset : offset + 4][::-1]
    for offset in range(S16_RUN_FIRST, S16_RUN_END, 4):
        result[offset : offset + 4] = (
            result[offset + 2 : offset + 4] + result[offset : offset + 2]
        )
    return bytes(result)


def main() -> None:
    for label, spec, offset, expected_ptrs, expected_words in SPECS:
        resource = census.load_o2r(ROOT, spec)
        if offset + WPATTR_BYTES > len(resource.payload):
            raise RuntimeError(f"{label} WPAttributes exceeds asset {spec.file_id}")
        normalized = ds_normalize_wpattributes(
            resource.payload[offset : offset + WPATTR_BYTES]
        )
        pointers = struct.unpack_from("<4I", normalized, 0)
        s16_values = struct.unpack_from("<10h", normalized, S16_RUN_FIRST)
        words = struct.unpack_from("<4I", normalized, S16_RUN_END)
        if pointers != expected_ptrs:
            raise RuntimeError(f"{label} pointer words {pointers!r} != {expected_ptrs!r}")
        if s16_values != EXPECTED_S16:
            raise RuntimeError(f"{label} s16 lanes {s16_values!r} != {EXPECTED_S16!r}")
        if words != expected_words:
            raise RuntimeError(f"{label} packed words {words!r} != {expected_words!r}")
    print(
        "P2_LINK_WEAPON_ATTRIBUTES_OK "
        "spin_asset=225@0x0c boomerang_asset=226@0x00 "
        "offsets=0/0/0,0/0/0 map=150/0/-150/150"
    )


if __name__ == "__main__":
    main()
