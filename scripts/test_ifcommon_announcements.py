#!/usr/bin/env python3
"""Focused host checker for native TIME UP / GAME SET OAM banks.

Parses the actual src/nds/nds_ifcommon_oam.c tables (no duplicated
constants), asserts the BattleShip source contract, the fixed 64 KiB bank
plan and per-glyph tile coverage. Actual C bank/phase execution is covered
by menus/test_ifcommon_end_bank.py.

ROM build/emulator free. Visual closure is NOT claimed here.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
C_PATH = ROOT / "src" / "nds" / "nds_ifcommon_oam.c"
RELOC_PATH = ROOT / "src" / "port" / "reloc_backend_assets.c"
IFCOMMON_PATH = (
    ROOT
    / "decomp"
    / "BattleShip-main"
    / "decomp"
    / "src"
    / "if"
    / "ifcommon.c"
)

VRAM_BYTES = 64 * 1024

# BattleShip source contract (measured, not proposed).
EXPECTED_END_OFFSETS = {
    "T": 0xE4A8,
    "I": 0xF740,
    "M": 0x127E0,
    "E": 0x144E0,
    "U": 0x16EB8,
    "P": 0x18FE8,
    "S": 0x1B5F8,
    "A": 0x1DE68,
    "G": 0x20788,
}
EXPECTED_END_SOURCE = {
    "T": (36, 56),
    "I": (17, 57),
    "M": (50, 56),
    "E": (32, 56),
    "U": (41, 58),
    "P": (36, 56),
    "S": (39, 58),
    "A": (43, 56),
    "G": (41, 57),
}
EXPECTED_END_DS = {
    "T": (29, 45),
    "I": (14, 46),
    "M": (40, 45),
    "E": (26, 45),
    "U": (33, 46),
    "P": (29, 45),
    "S": (31, 46),
    "A": (34, 45),
    "G": (33, 46),
}
TIME_UP_ORDER = ["T", "I", "M", "E", "U", "P"]
GAME_SET_ORDER = ["G", "A", "M", "E", "S", "T"]
GAME_SET_SOBJS = ["G", "A", "M", "E", "S", "E", "T"]

LEGAL_CELLS = {
    (8, 8), (16, 16), (32, 32), (64, 64), (16, 8), (32, 8),
    (32, 16), (64, 32), (8, 16), (8, 32), (16, 32), (32, 64),
}

FAILURES: list[str] = []


def fail(message: str) -> None:
    FAILURES.append(message)
    print(f"FAIL: {message}")


def check(condition: bool, message: str) -> None:
    if not condition:
        fail(message)
    else:
        print(f"ok: {message}")


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_defines(text: str) -> dict[str, int]:
    defines: dict[str, int] = {}
    for match in re.finditer(
        r"#define\s+(NDS_IFCOMMON_\w+)\s+(\(?\d+[uU]*\)?|\d+u?)", text
    ):
        raw = match.group(2).rstrip("uU").strip("()")
        try:
            defines[match.group(1)] = int(raw, 0)
        except ValueError:
            continue
    return defines


def parse_asset_specs(text: str) -> list[dict]:
    """Parse the sNdsIFCommonAssetSpecs initializer in file order."""
    start = text.index("sNdsIFCommonAssetSpecs[")
    brace = text.index("{", text.index("=", start))
    depth = 0
    end = brace
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                end = i
                break
    body = text[brace : end + 1]
    entries = re.findall(
        r"\{\s*(0x[0-9a-fA-F]+)u?\s*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"
        r"\s*(\d+)u?\s*,\s*\{(.*?)\}\s*\}",
        body,
        re.DOTALL,
    )
    specs = []
    for offset_hex, tile_count, tiles_body in entries:
        tiles = re.findall(
            r"TILE\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,"
            r"\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)",
            tiles_body,
        )
        specs.append(
            {
                "offset": int(offset_hex, 16),
                "tile_count": int(tile_count),
                "tiles": [tuple(int(v) for v in tile) for tile in tiles],
            }
        )
    return specs


def parse_end_letters(text: str) -> dict[str, dict]:
    rows = re.findall(
        r"\{\s*nNDSIFCommonAssetEnd([A-Z])\s*,\s*(0x[0-9a-fA-F]+)u?\s*,"
        r"\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}",
        text,
    )
    return {
        letter: {
            "offset": int(offset, 16),
            "source": (int(sw), int(sh)),
            "ds": (int(dw), int(dh)),
        }
        for letter, offset, sw, sh, dw, dh in rows
    }


def parse_slots(text: str, name: str) -> list[tuple[str, int]]:
    block = re.search(
        r"sNdsIFCommon" + name + r"Slots\[\d+\]\s*=\s*\{(.*?)\};",
        text,
        re.DOTALL,
    )
    if block is None:
        return []
    return [
        (letter, int(offset.rstrip("u"), 0))
        for letter, offset in re.findall(
            r"nNDSIFCommonAssetEnd([A-Z])\s*,\s*(\d+u?)", block.group(1)
        )
    ]


def tile_bytes(tile: tuple) -> int:
    return tile[4] * tile[5] * 2


def check_coverage(name: str, content: tuple[int, int],
                   tiles: list[tuple]) -> int:
    width, height = content
    grid = [[0] * width for _ in range(height)]
    total = 0
    for (sx, sy, cw, ch, cell_w, cell_h, _px, _py) in tiles:
        total += cell_w * cell_h * 2
        if (cell_w, cell_h) not in LEGAL_CELLS:
            fail(f"{name}: illegal cell {cell_w}x{cell_h}")
        if sx + cw > width or sy + ch > height:
            fail(f"{name}: tile ({sx},{sy},{cw},{ch}) overflows "
                 f"{width}x{height}")
            continue
        for y in range(sy, sy + ch):
            for x in range(sx, sx + cw):
                grid[y][x] += 1
    for y in range(height):
        for x in range(width):
            if grid[y][x] != 1:
                fail(f"{name}: content pixel ({x},{y}) covered "
                     f"{grid[y][x]}x")
                return total
    print(f"ok: {name} covers {width}x{height} once, {total} bytes")
    return total


def main() -> int:
    for path in (C_PATH, RELOC_PATH, IFCOMMON_PATH):
        check(path.is_file(), f"input exists: {path.relative_to(ROOT)}")
    if FAILURES:
        return 1
    c_text = read_text(C_PATH)

    defines = parse_defines(c_text)
    for key, expected in (
        ("NDS_IFCOMMON_ASSET_COUNT", 25),
        ("NDS_IFCOMMON_GO_BANK_BASE", 0),
        ("NDS_IFCOMMON_GO_BANK_BYTES", 17408),
        ("NDS_IFCOMMON_END_BANK_BASE", 17408),
        ("NDS_IFCOMMON_END_BANK_BYTES", 20736),
        ("NDS_IFCOMMON_SPARK_BANK_BASE", 38144),
        ("NDS_IFCOMMON_TAG_BANK_BASE", 60672),
        ("NDS_IFCOMMON_USED_BYTES", 63744),
    ):
        check(defines.get(key) == expected, f"define {key} == {expected}")

    specs = parse_asset_specs(c_text)
    check(len(specs) == 25, f"spec table has 25 entries (got {len(specs)})")
    if len(specs) != 25:
        return 1

    # Source spelling and offsets against the reloc manifest.
    reloc_text = read_text(RELOC_PATH)
    reloc_offsets = [int(v, 16) for v in re.findall(
        r"NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS,\s*(0x[0-9a-fA-F]+)u?",
        reloc_text,
    )]
    letter_order = ["T", "I", "M", "E", "U", "P", "S", "A", "G"]
    for pos, letter in enumerate(letter_order):
        offset = EXPECTED_END_OFFSETS[letter]
        check(offset in reloc_offsets,
              f"reloc manifest contains end letter {letter} {hex(offset)}")
        check(specs[16 + pos]["offset"] == offset,
              f"OAM spec[{16 + pos}] is end letter {letter} {hex(offset)}")

    end_letters = parse_end_letters(c_text)
    check(len(end_letters) == 9, "end metadata has nine letters")
    for letter in EXPECTED_END_OFFSETS:
        entry = end_letters.get(letter)
        check(entry is not None, f"end metadata has {letter}")
        if entry is None:
            continue
        check(entry["offset"] == EXPECTED_END_OFFSETS[letter],
              f"{letter} offset {hex(entry['offset'])}")
        check(entry["source"] == EXPECTED_END_SOURCE[letter],
              f"{letter} source {entry['source']}")
        check(entry["ds"] == EXPECTED_END_DS[letter],
              f"{letter} DS {entry['ds']}")
        sw, sh = entry["source"]
        dw, dh = entry["ds"]
        check(dw == round(sw * 0.8) and dh == round(sh * 0.8),
              f"{letter} DS is round(source*0.8)")

    # Source announce tables: TIME UP has 6 SObjs, GAME SET has 7 (E twice).
    ifcommon_text = read_text(IFCOMMON_PATH)
    time_up = re.search(
        r"dIFCommonAnnounceTimeUpSpriteData.*?{(.*?)};",
        ifcommon_text, re.DOTALL)
    game_set = re.search(
        r"dIFCommonAnnounceGameSetSpriteData.*?{(.*?)};",
        ifcommon_text, re.DOTALL)
    check(time_up is not None and
          len(re.findall(r"BlueLetter", time_up.group(1))) == 6,
          "source TIME UP has 6 BlueLetter SObjs")
    check(game_set is not None and
          len(re.findall(r"BlueLetter", game_set.group(1))) == 7,
          "source GAME SET has 7 BlueLetter SObjs (E twice)")

    # Indexed entries own no OBJ bytes but keep metadata.
    for index in range(3, 16):
        check(specs[index]["tile_count"] == 0,
              f"indexed spec {index} owns no OBJ tiles")
        check(len(specs[index]["tiles"]) == 0,
              f"indexed spec {index} has no tile rows")

    # GO coverage: 50x58, 56x59, 19x58 -> 7168 + 7168 + 3072.
    go_bytes = 0
    for index, content in ((0, (50, 58)), (1, (56, 59)), (2, (19, 58))):
        spec = specs[index]
        check(spec["tile_count"] == len(spec["tiles"]),
              f"GO spec {index} tile_count matches rows")
        check(spec["tile_count"] <= 9, f"GO spec {index} within 9 tiles")
        go_bytes += check_coverage(f"GO[{index}]", content, spec["tiles"])
    check(go_bytes == 17408, f"GO bank bytes {go_bytes} == 17408")

    # End coverage per DS content size.
    letter_index = {"T": 16, "I": 17, "M": 18, "E": 19, "U": 20,
                    "P": 21, "S": 22, "A": 23, "G": 24}
    end_bytes: dict[str, int] = {}
    for letter, content in EXPECTED_END_DS.items():
        spec = specs[letter_index[letter]]
        check(spec["offset"] == EXPECTED_END_OFFSETS[letter],
              f"end spec {letter} offset matches metadata")
        check(spec["tile_count"] == len(spec["tiles"]),
              f"end spec {letter} tile_count matches rows")
        check(spec["tile_count"] <= 9, f"end spec {letter} within 9 tiles")
        end_bytes[letter] = check_coverage(
            f"END[{letter}]", content, spec["tiles"])

    def subset_total(order: list[str]) -> int:
        return sum(end_bytes[letter] for letter in order)

    time_up_bytes = subset_total(TIME_UP_ORDER)
    game_set_bytes = subset_total(GAME_SET_ORDER)
    check(time_up_bytes == 18432, f"TIME UP subset {time_up_bytes} == 18432")
    check(game_set_bytes == 20736, f"GAME SET subset {game_set_bytes} == 20736")
    check(game_set_bytes == defines.get("NDS_IFCOMMON_END_BANK_BYTES"),
          "end bank sized for GAME SET max")

    # Fixed slot packing: contiguous, in-bank, no partial overlap.
    for name, order in (("TimeUp", TIME_UP_ORDER),
                        ("GameSet", GAME_SET_ORDER)):
        slots = dict(parse_slots(c_text, name))
        check(len(slots) == 6, f"{name} slot table has 6 entries")
        cursor = 0
        intervals = []
        for letter in order:
            check(letter in slots, f"{name} slot has {letter}")
            base = slots.get(letter, -1)
            size = end_bytes[letter]
            check(base == cursor,
                  f"{name} {letter} packed at {base} (cursor {cursor})")
            check(base + size <= 20736, f"{name} {letter} inside end bank")
            intervals.append((base, base + size))
            cursor += size
        for i in range(len(intervals)):
            for j in range(i + 1, len(intervals)):
                overlap = min(intervals[i][1], intervals[j][1]) - max(
                    intervals[i][0], intervals[j][0])
                check(overlap <= 0, f"{name} slots {i}/{j} do not overlap")

    # 64 KiB bounds including tags.
    go_base = defines["NDS_IFCOMMON_GO_BANK_BASE"]
    end_base = defines["NDS_IFCOMMON_END_BANK_BASE"]
    spark_base = defines["NDS_IFCOMMON_SPARK_BANK_BASE"]
    tag_base = defines["NDS_IFCOMMON_TAG_BANK_BASE"]
    banks = [
        ("GO", go_base, 17408),
        ("END", end_base, 20736),
        ("SPARK", spark_base, 22528),
        ("TAG", tag_base, 3072),
    ]
    for name, base, size in banks:
        check(base + size <= VRAM_BYTES, f"{name} bank inside 64 KiB")
    for i in range(len(banks)):
        for j in range(i + 1, len(banks)):
            overlap = min(banks[i][1] + banks[i][2],
                          banks[j][1] + banks[j][2]) - max(
                              banks[i][1], banks[j][1])
            check(overlap <= 0,
                  f"{banks[i][0]}/{banks[j][0]} banks do not overlap")
    total = 17408 + 20736 + 22528 + 3072
    check(total == 63744 and total == defines["NDS_IFCOMMON_USED_BYTES"],
          f"total {total} == 63744 resident")

    # Actual C phase/bake execution lives in menus/test_ifcommon_end_bank.py.

    if FAILURES:
        print(f"{len(FAILURES)} failure(s)")
        return 1
    print("test_ifcommon_announcements: all checks passed "
          "(source/layout only, no visual closure)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
