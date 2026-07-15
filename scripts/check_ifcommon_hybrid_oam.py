#!/usr/bin/env python3
"""Prove the exact BattleShip IFCommon hybrid OBJ packing contract."""

from __future__ import annotations

import hashlib
import re
import struct
import sys
from pathlib import Path


EXPECTED_O2R_SHA256 = (
    "aee5419f4515b03c06985f4d697db7e002604b617c7fbd5eca9bdc8207ad4efe"
)
EXPECTED_PALETTE = (
    0x0000, 0x9084, 0xEF7B, 0xA108, 0x8000, 0x98C6, 0xF7BD,
    0xFFFF, 0xA94A, 0x8842, 0xB18C, 0xE739, 0xCE73, 0xD6B5,
    0xC631, 0xB9CE, 0xDEF7, 0xA084, 0x9C63, 0x9863, 0xA4A5,
    0xA8A5, 0xA8C6, 0xACE7, 0x843F, 0x829F, 0xFD89, 0x9CFF,
    0xFD84,
)
EXPECTED_STATS = {
    "assets": 16,
    "tiles": 59,
    "bitmap_bytes": 26880,
    "indexed_bytes": 33472,
    "alignment_waste_bytes": 64,
    "obj_span_bytes": 60416,
    "parity_pixels": 46912,
}
OBJ_ALIGNMENT = 128
OBJ_BANK_E_BYTES = 64 * 1024
PALETTE_ENTRIES = 256


def fail(message: str) -> None:
    raise AssertionError(message)


def number(token: str) -> int:
    return int(token.rstrip("uU"), 0)


def parse_manifest(source: str) -> list[dict[str, object]]:
    begin = source.index("static const NDSIFCommonAssetSpec")
    end = source.index("\n};\n\n#undef TILE", begin) + 3
    block = source[begin:end]
    token = r"(?:0x[0-9a-fA-F]+|[0-9]+)u?"
    asset_pattern = re.compile(
        r"\{\s*(" + token + r")\s*,\s*"
        + r"\s*,\s*".join([r"(" + token + r")"] * 7)
        + r"\s*,\s*\{(.*?)\}\s*\}",
        re.DOTALL,
    )
    assets: list[dict[str, object]] = []
    for match in asset_pattern.finditer(block):
        fields = [number(match.group(i)) for i in range(1, 9)]
        tiles = []
        for tile_text in re.findall(r"TILE\(([^)]*)\)", match.group(9)):
            tile = tuple(number(item.strip()) for item in tile_text.split(","))
            if len(tile) != 8:
                fail("hybrid OAM TILE does not have eight fields")
            tiles.append(tile)
        tile_count = fields[7]
        if len(tiles) != tile_count:
            fail(
                f"asset {fields[0]:#x} declares {tile_count} live tiles, "
                f"parsed {len(tiles)}"
            )
        assets.append(
            {
                "offset": fields[0],
                "prim": tuple(fields[1:4]),
                "env": tuple(fields[4:7]),
                "tiles": tiles,
            }
        )
    if len(assets) != EXPECTED_STATS["assets"]:
        fail(f"expected 16 hybrid assets, parsed {len(assets)}")
    return assets


def load_o2r(path: Path) -> bytes:
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED_O2R_SHA256:
        fail(f"IFCommonGameStatus SHA-256 changed: {digest}")
    if raw[4:8] != b"OLER":
        fail("IFCommonGameStatus is not an O2R resource")
    if int.from_bytes(raw[0x40:0x44], "little") != 0x52:
        fail("IFCommonGameStatus O2R file id is not 0x52")
    extern_count = int.from_bytes(raw[0x48:0x4C], "little")
    size_offset = 0x40 + 12 + (extern_count * 2)
    data_size = int.from_bytes(raw[size_offset:size_offset + 4], "little")
    data = raw[size_offset + 4:size_offset + 4 + data_size]
    if len(data) != data_size:
        fail("IFCommonGameStatus O2R data payload is truncated")
    return data


def be_s16(data: bytes, offset: int) -> int:
    return struct.unpack_from(">h", data, offset)[0]


def reloc_target(data: bytes, offset: int) -> int:
    return (struct.unpack_from(">I", data, offset)[0] & 0xFFFF) * 4


def sprite_at(data: bytes, offset: int) -> dict[str, int]:
    if offset + 68 > len(data):
        fail(f"Sprite at {offset:#x} exceeds O2R data")
    sprite = {
        "width": be_s16(data, offset + 4),
        "height": be_s16(data, offset + 6),
        "attr": struct.unpack_from(">H", data, offset + 20)[0],
        "nbitmaps": be_s16(data, offset + 40),
        "bmheight": be_s16(data, offset + 44),
        "format": data[offset + 48],
        "size": data[offset + 49],
        "bitmap": reloc_target(data, offset + 52),
    }
    if sprite["nbitmaps"] <= 0 or not (sprite["attr"] & 0x0200):
        fail(f"Sprite at {offset:#x} is not the expected TEXSHUF manifest")
    return sprite


def bitmap_at(data: bytes, sprite: dict[str, int], index: int) -> dict[str, int]:
    offset = sprite["bitmap"] + (index * 16)
    if offset + 16 > len(data):
        fail(f"Bitmap {index} at {offset:#x} exceeds O2R data")
    return {
        "width": be_s16(data, offset),
        "width_img": be_s16(data, offset + 2),
        "buffer": reloc_target(data, offset + 8),
        "height": be_s16(data, offset + 12),
    }


def rgb15(red: int, green: int, blue: int) -> int:
    return 0x8000 | (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)


def lerp_rgb15(prim: tuple[int, ...], env: tuple[int, ...], intensity: int) -> int:
    inverse = 255 - intensity
    color = tuple(
        ((prim[channel] * intensity) + (env[channel] * inverse) + 127) // 255
        for channel in range(3)
    )
    return rgb15(*color)


def decode_pixel(
    data: bytes,
    sprite: dict[str, int],
    prim: tuple[int, ...],
    env: tuple[int, ...],
    source_x: int,
    source_y: int,
) -> int:
    out_y = 0
    for bitmap_index in range(sprite["nbitmaps"]):
        bitmap = bitmap_at(data, sprite, bitmap_index)
        width = bitmap["width"]
        width_img = bitmap["width_img"] or width
        height = bitmap["height"] or sprite["bmheight"]
        advance = sprite["bmheight"] or height
        if width == 0:
            break
        if not (out_y <= source_y < out_y + height and source_x < width):
            out_y += advance
            continue

        local_y = source_y - out_y
        shuffled_x = source_x
        if local_y & 1:
            if sprite["size"] == 0:
                shuffled_x ^= 8
            elif sprite["size"] == 1:
                shuffled_x ^= 4
            elif sprite["format"] == 0 and sprite["size"] == 3:
                shuffled_x ^= 1

        buffer = bitmap["buffer"]
        if sprite["format"] == 0 and sprite["size"] == 3:
            offset = buffer + ((local_y * width_img + shuffled_x) * 4)
            rgba = struct.unpack_from(">I", data, offset)[0]
            return rgb15(rgba >> 24, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF) \
                if rgba & 0xFF else 0
        if sprite["format"] == 3 and sprite["size"] == 1:
            value = data[buffer + (local_y * width_img) + shuffled_x]
            return lerp_rgb15(prim, env, (value >> 4) * 17) if value & 0xF else 0
        if sprite["format"] == 4 and sprite["size"] == 1:
            value = data[buffer + (local_y * width_img) + shuffled_x]
            return rgb15(*prim) if value else 0
        if sprite["format"] == 4 and sprite["size"] == 0:
            row_bytes = (width_img + 1) // 2
            packed = data[buffer + (local_y * row_bytes) + (shuffled_x >> 1)]
            value = packed >> 4 if not (shuffled_x & 1) else packed & 0xF
            return rgb15(*prim) if value else 0
        fail(
            f"unsupported IFCommon format {sprite['format']}/{sprite['size']}"
        )
    return 0


def tile_offset(width: int, x: int, y: int) -> int:
    return (((y >> 3) * (width >> 3) + (x >> 3)) * 64) + \
        ((y & 7) * 8) + (x & 7)


def check_runtime_contract(root: Path, source: str) -> None:
    header = (root / "include/nds/nds_ifcommon_oam.h").read_text()
    platform = (root / "src/nds/nds_platform.c").read_text()
    if "#ifndef NDS_IFCOMMON_HYBRID_OAM\n#define NDS_IFCOMMON_HYBRID_OAM 0" not in header:
        fail("NDS_IFCOMMON_HYBRID_OAM is not default-off in the public seam")
    for fragment in (
        "SpriteMapping_Bmp_1D_128",
        "SpriteColorFormat_256Color",
        "NDS_IFCOMMON_OBJ_GFX_ALIGNMENT",
        "memcpy(SPRITE_PALETTE, palette_storage",
    ):
        if fragment not in source:
            fail(f"hybrid owner is missing runtime contract: {fragment}")
    for fragment in (
        "#if NDS_IFCOMMON_HYBRID_OAM",
        "vramSetBankF(VRAM_F_TEX_PALETTE_SLOT0);",
        "vramSetBankG(VRAM_G_TEX_PALETTE_SLOT1);",
        "vramSetBankF(VRAM_F_MAIN_SPRITE_0x06410000);",
        "vramSetBankG(VRAM_G_MAIN_SPRITE_0x06414000);",
    ):
        if fragment not in platform:
            fail(f"platform is missing hybrid F/G ownership: {fragment}")
    gameplay = source[source.index("void ndsIFCommonNativeOamBeginFrame") :]
    for forbidden in (
        "ndsIFCommonDecodePixel(",
        "SPRITE_GFX",
        "SPRITE_PALETTE",
        "dmaFill",
        "dmaCopy",
    ):
        if forbidden in gameplay:
            fail(f"post-prepare IFCommon path contains forbidden work: {forbidden}")


def verify(root: Path) -> None:
    source = (root / "src/nds/nds_ifcommon_oam.c").read_text()
    assets = parse_manifest(source)
    data = load_o2r(
        root / "decomp/BattleShip-main/BattleShip_o2r/reloc_interface/"
        "IFCommonGameStatus"
    )
    sprites = [sprite_at(data, int(asset["offset"])) for asset in assets]

    palette = [0]
    palette_index = {0: 0}
    for asset_index, asset in enumerate(assets[3:], start=3):
        for source_x, source_y, width, height, *_ in asset["tiles"]:
            for y in range(height):
                for x in range(width):
                    color = decode_pixel(
                        data, sprites[asset_index], asset["prim"], asset["env"],
                        source_x + x, source_y + y,
                    )
                    if color not in palette_index:
                        if len(palette) >= PALETTE_ENTRIES:
                            fail("IFCommon indexed colors exceed one OBJ palette")
                        palette_index[color] = len(palette)
                        palette.append(color)
    if tuple(palette) != EXPECTED_PALETTE:
        fail("deterministic IFCommon RGB15 palette changed")

    stats = {
        "assets": len(assets),
        "tiles": 0,
        "bitmap_bytes": 0,
        "indexed_bytes": 0,
        "alignment_waste_bytes": 0,
        "obj_span_bytes": 0,
        "parity_pixels": 0,
    }
    cursor = 0
    for asset_index, asset in enumerate(assets):
        sprite = sprites[asset_index]
        for tile in asset["tiles"]:
            source_x, source_y, width, height, cell_width, cell_height, pad_x, pad_y = tile
            if cell_width % 8 or cell_height % 8:
                fail("standard OBJ cell is not an 8-pixel multiple")
            aligned = (cursor + OBJ_ALIGNMENT - 1) & ~(OBJ_ALIGNMENT - 1)
            stats["alignment_waste_bytes"] += aligned - cursor
            cursor = aligned
            if cursor % OBJ_ALIGNMENT:
                fail("hybrid OBJ tile start is not 128-byte aligned")

            reference = [0] * (cell_width * cell_height)
            for y in range(height):
                for x in range(width):
                    reference[(pad_y + y) * cell_width + pad_x + x] = decode_pixel(
                        data, sprite, asset["prim"], asset["env"],
                        source_x + x, source_y + y,
                    )

            if asset_index < 3:
                size = len(reference) * 2
                reconstructed = reference
                stats["bitmap_bytes"] += size
            else:
                size = len(reference)
                encoded = bytearray(size)
                for y in range(cell_height):
                    for x in range(cell_width):
                        encoded[tile_offset(cell_width, x, y)] = palette_index[
                            reference[y * cell_width + x]
                        ]
                reconstructed = [
                    palette[encoded[tile_offset(cell_width, x, y)]]
                    for y in range(cell_height)
                    for x in range(cell_width)
                ]
                stats["indexed_bytes"] += size
            if reconstructed != reference:
                fail(f"RGB15 parity failed for asset {asset['offset']:#x}")
            cursor += size
            stats["tiles"] += 1
            stats["parity_pixels"] += len(reference)

    stats["obj_span_bytes"] = cursor
    if stats != EXPECTED_STATS:
        fail(f"hybrid OBJ byte contract changed: {stats}")
    if cursor > OBJ_BANK_E_BYTES:
        fail(f"hybrid OBJ span {cursor} exceeds 64 KiB bank E")
    check_runtime_contract(root, source)

    print("IFCommon hybrid OAM: PASS")
    print(f"  O2R SHA-256: {EXPECTED_O2R_SHA256}")
    print(
        f"  assets/tiles/parity pixels: {stats['assets']}/"
        f"{stats['tiles']}/{stats['parity_pixels']}"
    )
    print(
        f"  bitmap/indexed/alignment bytes: {stats['bitmap_bytes']}/"
        f"{stats['indexed_bytes']}/{stats['alignment_waste_bytes']}"
    )
    print(
        f"  OBJ span/headroom: {cursor}/{OBJ_BANK_E_BYTES - cursor} bytes"
    )
    print(
        f"  standard OBJ palette: {len(palette)} used entries "
        f"({len(palette) - 1} visible), {PALETTE_ENTRIES * 2} upload bytes"
    )
    print("  post-prepare conversion/upload: 0/0")


if __name__ == "__main__":
    try:
        verify(Path(__file__).resolve().parents[1])
    except (AssertionError, OSError, ValueError, struct.error) as error:
        print(f"IFCommon hybrid OAM: FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
