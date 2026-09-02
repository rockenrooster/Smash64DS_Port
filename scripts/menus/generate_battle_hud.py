#!/usr/bin/env python3
"""Bake the P2-2 lower-screen battle HUD into DS 4bpp OBJ cells.

The runtime HUD is a presentation sink only.  BattleShip's imported ifCommon
objects remain authoritative for timer, damage and stock state; this bake merely
turns their source artwork into a format the DS sub 2D engine can consume with
no runtime decode/conversion.

All normal interface/menu sprites are decoded through the same RELO reader used
by generate_mn_ui_kit.py.  Mario/Fox model files use the older 0x58-byte RELO
header variant in the checked BattleShip_o2r corpus, so their tiny CI4 stock
icons are read through a deliberately bounded legacy reader with source offsets
cross-checked against the decomp's Sprite declarations.
"""

from __future__ import annotations

import argparse
import importlib.util
import re
import struct
import sys
from pathlib import Path


DAMAGE_SYMBOLS = [f"llIFCommonPlayerDamageDigit{i}Sprite" for i in range(10)] + [
    "llIFCommonPlayerDamageSymbolPercentSprite"
]
TIMER_SYMBOLS = [f"llIFCommonTimerDigit{i}Sprite" for i in range(10)] + [
    "llIFCommonTimerSymbolColonSprite"
]
STOCK_DIGIT_SYMBOLS = [f"llIFCommonDigits{i}Sprite" for i in range(10)] + [
    "llIFCommonDigitsCrossSprite"
]
PORTRAIT_SYMBOLS = [
    "llMNPlayersPortraitsMarioSprite",
    "llMNPlayersPortraitsFoxSprite",
    "llMNPlayersPortraitsLuigiSprite",
    "llMNPlayersPortraitsDonkeySprite",
    "llMNPlayersPortraitsCaptainSprite",
    "llMNPlayersPortraitsSamusSprite",
    "llMNPlayersPortraitsLinkSprite",
    "llMNPlayersPortraitsPikachuSprite",
    "llMNPlayersPortraitsYoshiSprite",
    "llMNPlayersPortraitsNessSprite",
    "llMNPlayersPortraitsPurinSprite",
    "llMNPlayersPortraitsKirbySprite",
]

# The checked corpus contains two O2R header revisions.  The original
# Mario/Fox/Luigi model exports use the legacy 0x58-byte form (data size at
# +0x54); newer split reloc files such as DkIcon use the compact 0x50-byte form
# (data size at +0x4c).  Both carry the same RELO magic at +4.  Detect the
# revision from the self-consistent payload extent instead of making fighter
# admission depend on a per-file loader fork.
O2R_RESOURCE_HEADER_SIZE = 0x40
O2R_PAYLOAD_LAYOUTS = (
    (0x58, 0x54),
    (0x50, 0x4C),
)
MODEL_STOCK = {
    "MARIO": {
        "file": "MarioModel",
        "sprite": 0x72D0,
        "texture": 0x71A8,
        "palettes": [0x7200, 0x7228, 0x7250, 0x7278, 0x72A0],
    },
    "FOX": {
        "file": "FoxModel",
        "sprite": 0x7C28,
        "texture": 0x7B28,
        "palettes": [0x7B80, 0x7BA8, 0x7BD0, 0x7BF8],
    },
    # BattleShip dLuigiMain_sprites.stock_sprite points to LuigiModel+0x7CD8
    # and dLuigiMain_stock_luts points to the four 0x28-byte-stride CI4 LUT
    # frames at 0x7C30/58/80/A8.  The stock texture immediately precedes the
    # first LUT (0x7BD8..0x7C2F).  Keep these source offsets explicit for the
    # same bounded legacy-reader reason as Mario/Fox above; the O2R model uses
    # the old 0x58-byte RELO header and is not handled by RelocFile.
    "LUIGI": {
        "file": "LuigiModel",
        "sprite": 0x7CD8,
        "texture": 0x7BD8,
        "palettes": [0x7C30, 0x7C58, 0x7C80, 0x7CA8],
    },
    # BattleShip relocData/319_DkIcon.c keeps DK's stock art in the shared
    # DkIcon dependency rather than in DonkeyModel.  The source layout is
    # texture@0x08, LUTs@0x60/88/B0/D8/100, bitmap@0x120, Sprite@0x130.
    # Feeding that ordinary O2R payload through the same stock conversion is
    # the pipeline fix: future fighters may point `file` at whichever reloc
    # actually owns their FTSprites without teaching the runtime a new path.
    "DONKEY": {
        "file": "DkIcon",
        "sprite": 0x130,
        "texture": 0x08,
        "palettes": [0x60, 0x88, 0xB0, 0xD8, 0x100],
    },
    # Falcon keeps his stock art in CaptainModel like Mario/Fox/Luigi.
    # reloc_data_symbols.us.txt:4362 gives llCaptainModelStockSprite = 0xc6a8;
    # 236_CaptainMain.c:215's dCaptainMain_stock_luts names SIX LUTs --
    # dCaptainModel_palette_0xC5B0 then the five 0x28-stride frames in the
    # 0xC5D0 gap -- which is the first six-costume stock set on the roster and
    # matches dFTParamCostumeIDs[nFTKindCaptain]'s six distinct indices.
    # The 88-byte CI4 texture immediately precedes the first LUT.
    "CAPTAIN": {
        "file": "CaptainModel",
        "sprite": 0xC6A8,
        "texture": 0xC558,
        "palettes": [0xC5B0, 0xC5D8, 0xC600, 0xC628, 0xC650, 0xC678],
    },
    # 217_SamusMain.c's FTSprites points stock_sprite at SamusModel+0xE2F0
    # and its five stock_luts at 0xE220/48/70/98/C0.  320_SamusModel.c pins
    # the 8x10 CI4 texture immediately before those LUTs at 0xE1C8.
    "SAMUS": {
        "file": "SamusModel",
        "sprite": 0xE2F0,
        "texture": 0xE1C8,
        "palettes": [0xE220, 0xE248, 0xE270, 0xE298, 0xE2C0],
    },
    # 225_LinkMain.c's dLinkMain_sprites points at LinkModel's Stock sprite and
    # exactly four stock LUTs. 324_LinkModel.c pins the 8x10 CI4 texture at
    # 0x11C48, the first palette at 0x11CA0, then three 0x28-stride palette
    # frames after the source's 8-byte pads. reloc_data_symbols.us.txt binds the
    # Sprite itself at 0x11D48. Keep the source physical layout explicit just as
    # the earlier model-owned stock icons do.
    "LINK": {
        "file": "LinkModel",
        "sprite": 0x11D48,
        "texture": 0x11C48,
        "palettes": [0x11CA0, 0x11CC8, 0x11CF0, 0x11D18],
    },
    # 243_PikachuMain.c:199 dPikachuMain_stock_luts names five LUTs: the
    # dPikachuModel_palette_0x9930 block then four 0x28-stride frames in the
    # 0x9950 gap (0x9958/0x9980/0x99A8/0x99D0). 341_PikachuModel.c:4370 pins
    # the 88-byte 8x10 CI4 texture immediately before the first LUT, and
    # reloc_data_symbols.us.txt:4388 binds llPikachuModelStockSprite = 0x9a00.
    "PIKACHU": {
        "file": "PikachuModel",
        "sprite": 0x9A00,
        "texture": 0x98D8,
        "palettes": [0x9930, 0x9958, 0x9980, 0x99A8, 0x99D0],
    },
    # 247_YoshiMain.c dYoshiMain_stock_luts names six LUTs at a 0x28 stride
    # from dYoshiModel_palette_0xA9B0 (0xA9B0/0xA9D8/0xAA00/0xAA28/0xAA50/
    # 0xAA78): Yoshi is the one fighter with six costumes. 338_YoshiModel.c
    # pins the 88-byte 8x10 CI4 texture at 0xA958 immediately before the
    # first LUT and llYoshiModelStockSprite at 0xAAA8.
    # Ness: stock texture 88 bytes before the first LUT, LUTs from dNessMain_stock_luts,
    # sprite llNessModelStockSprite (admit_fighter.py).
    # Purin: stock texture 88 bytes before the first LUT, LUTs from dPurinMain_stock_luts,
    # sprite llPurinModelStockSprite (admit_fighter.py).
    # Kirby: stock texture 88 bytes before the first LUT, LUTs from dKirbyMain_stock_luts,
    # sprite llKirbyModelStockSprite (admit_fighter.py).
    "KIRBY": {
        "file": "KirbyModel",
        "sprite": 0x1D5E0,
        "texture": 0x1D4B8,
        "palettes": [0x1D510, 0x1D538, 0x1D560, 0x1D588, 0x1D5B0],
    },
    "PURIN": {
        "file": "PurinModel",
        "sprite": 0x7BB0,
        "texture": 0x7A88,
        "palettes": [0x7AE0, 0x7B08, 0x7B30, 0x7B58, 0x7B80],
    },
    "NESS": {
        "file": "NessModel",
        "sprite": 0xC188,
        "texture": 0xC088,
        "palettes": [0xC0E0, 0xC108, 0xC130, 0xC158],
    },
    "YOSHI": {
        "file": "YoshiModel",
        "sprite": 0xAAA8,
        "texture": 0xA958,
        "palettes": [0xA9B0, 0xA9D8, 0xAA00, 0xAA28, 0xAA50, 0xAA78],
    },
}


class BakeError(RuntimeError):
    pass


def load_ui_generator(repo_root: Path):
    path = repo_root / "scripts" / "menus" / "generate_mn_ui_kit.py"
    spec = importlib.util.spec_from_file_location("nds_mn_ui_bake", path)
    if spec is None or spec.loader is None:
        raise BakeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    # Python 3.13's dataclass resolver expects the module to be registered
    # while decorators execute.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def scale_raster(ui, raster, num: int = 4, den: int = 5):
    src_h = len(raster)
    src_w = len(raster[0]) if src_h else 0
    dst_w = max(1, (src_w * num + den // 2) // den)
    dst_h = max(1, (src_h * num + den // 2) // den)
    return ui.box_scale(raster, dst_w, dst_h), dst_w, dst_h


def intensity_indices(raster, content_w: int, content_h: int,
                      cell_w: int, cell_h: int,
                      origin_x: int = 0, origin_y: int = 0) -> list[int]:
    out = [0] * (cell_w * cell_h)
    for y in range(content_h):
        for x in range(content_w):
            r, g, b, a = raster[y][x]
            if a == 0:
                continue
            # The source I/IA sprites are intensity masks.  Preserve every
            # non-zero-alpha texel (OBJ4 has only transparent/opaque) and keep
            # fifteen intensity levels so the runtime can reproduce the source
            # primitive-colour modulation by changing only the palette.
            luma = max(r, g, b)
            out[(origin_y + y) * cell_w + origin_x + x] = \
                max(1, min(15, (luma * 15 + 127) // 255))
    return out


def pack_obj4(indices: list[int], width: int, height: int) -> bytes:
    if width % 8 or height % 8 or len(indices) != width * height:
        raise BakeError(f"OBJ4 cell must be 8x8 tiled, got {width}x{height}")
    blob = bytearray()
    for tile_y in range(0, height, 8):
        for tile_x in range(0, width, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    lo = indices[(tile_y + y) * width + tile_x + x] & 0xF
                    hi = indices[(tile_y + y) * width + tile_x + x + 1] & 0xF
                    blob.append(lo | (hi << 4))
    return bytes(blob)


def quantize_portrait(ui, raster, width: int, height: int):
    """Deterministic weighted median-cut: transparent + at most 15 colours."""
    counts: dict[tuple[int, int, int], int] = {}
    for y in range(height):
        for x in range(width):
            r, g, b, a = raster[y][x]
            if a < 32:
                continue
            key = (r, g, b)
            counts[key] = counts.get(key, 0) + max(1, a)

    if not counts:
        raise BakeError("portrait became fully transparent")

    boxes = [list(counts.keys())]
    while len(boxes) < 15:
        split_at = -1
        split_channel = 0
        split_span = -1
        split_weight = -1
        for i, box in enumerate(boxes):
            if len(box) < 2:
                continue
            spans = [max(c[ch] for c in box) - min(c[ch] for c in box)
                     for ch in range(3)]
            channel = max(range(3), key=lambda ch: spans[ch])
            weight = sum(counts[c] for c in box)
            if (spans[channel], weight) > (split_span, split_weight):
                split_at = i
                split_channel = channel
                split_span = spans[channel]
                split_weight = weight
        if split_at < 0:
            break

        box = sorted(boxes.pop(split_at), key=lambda c: (c[split_channel], c))
        total = sum(counts[c] for c in box)
        accum = 0
        cut = 1
        for j, color in enumerate(box[:-1], 1):
            accum += counts[color]
            if accum * 2 >= total:
                cut = j
                break
        boxes.append(box[:cut])
        boxes.append(box[cut:])

    palette_rgb = []
    for box in boxes:
        weight = sum(counts[c] for c in box)
        palette_rgb.append(tuple(
            sum(c[ch] * counts[c] for c in box) // weight for ch in range(3)
        ))

    palette = [0] + [ui.rgba8_to_ds(r, g, b, 255) for r, g, b in palette_rgb]
    palette += [0] * (16 - len(palette))
    indices = [0] * (16 * 16)
    for y in range(height):
        for x in range(width):
            r, g, b, a = raster[y][x]
            if a < 32:
                continue
            best = min(range(len(palette_rgb)),
                       key=lambda i: ((r - palette_rgb[i][0]) ** 2 +
                                      (g - palette_rgb[i][1]) ** 2 +
                                      (b - palette_rgb[i][2]) ** 2))
            indices[y * 16 + x] = best + 1
    return pack_obj4(indices, 16, 16), palette


def read_o2r_payload(path: Path) -> bytes:
    raw = path.read_bytes()
    if len(raw) < min(header for header, _ in O2R_PAYLOAD_LAYOUTS):
        raise BakeError(f"{path.name}: RELO too short")
    if struct.unpack_from("<I", raw, 4)[0] != 0x52454C4F:
        raise BakeError(f"{path.name}: missing RELO magic")
    # The 0x58/0x50 forms above are the extern-count 4 and 0 cases of one
    # header: 0x40 resource bytes, file id, a word, the extern count, that many
    # u16 externs, then the data size. YoshiModel carries ten externs (0x64),
    # so parse the table the way generate_nds_native_owners.load_o2r_payload
    # does instead of enumerating extents.
    extern_count = struct.unpack_from("<I", raw, O2R_RESOURCE_HEADER_SIZE + 8)[0]
    size_offset = O2R_RESOURCE_HEADER_SIZE + 12 + extern_count * 2
    if size_offset + 4 <= len(raw):
        size = struct.unpack_from("<I", raw, size_offset)[0]
        if size_offset + 4 + size == len(raw):
            return raw[size_offset + 4:]
    raise BakeError(f"{path.name}: unsupported RELO payload extent ({len(raw):#x})")


def stock_asset(ui, repo_root: Path, spec: dict):
    path = (repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" /
            "reloc_fighters_main" / spec["file"])
    payload = read_o2r_payload(path)
    sprite = spec["sprite"]
    if sprite + 68 > len(payload):
        raise BakeError(f"{spec['file']}: stock Sprite out of range")
    width, height = struct.unpack_from(">hh", payload, sprite + 4)
    attr = struct.unpack_from(">H", payload, sprite + 20)[0]
    bmfmt, bmsiz = payload[sprite + 48], payload[sprite + 49]
    if (width, height, bmfmt, bmsiz) != (8, 10, ui.G_IM_FMT_CI, ui.G_IM_SIZ_4b):
        raise BakeError(
            f"{spec['file']}: stock Sprite drifted: {width}x{height} "
            f"fmt={bmfmt} siz={bmsiz}")
    if (attr & ui.SP_TEXSHUF) == 0:
        raise BakeError(f"{spec['file']}: source stock sprite lost SP_TEXSHUF")

    tex = spec["texture"]
    # Bitmap width_img is 16: eight visible pixels plus eight padded pixels.
    # Undo the exact SP_TEXSHUF odd-row qword swap before sampling x=0..7.
    source = []
    for y in range(10):
        row = bytes(payload[tex + y * 8:tex + (y + 1) * 8])
        row = ui.deswizzle_row(row, y, True, 8)
        pixels = []
        for byte in row:
            pixels.extend([(byte >> 4) & 0xF, byte & 0xF])
        source.append(pixels[:8])

    # 320x240 -> 256x192.  The 8x10 source icon becomes 6x8, but lives in an
    # 8x8 DS cell.  Nearest sampling preserves CI4 index identity so every
    # costume palette remains valid for the one shared glyph.
    dst_w, dst_h = 6, 8
    indices = [0] * 64
    for y in range(dst_h):
        sy = min(9, (y * 10) // dst_h)
        for x in range(dst_w):
            sx = min(7, (x * 8) // dst_w)
            indices[y * 8 + x] = source[sy][sx]

    palettes = []
    for pal_off in spec["palettes"]:
        if pal_off + 32 > len(payload):
            raise BakeError(f"{spec['file']}: stock palette out of range")
        palette = []
        for i in range(16):
            n64 = struct.unpack_from(">H", payload, pal_off + i * 2)[0]
            palette.append(ui.rgba8_to_ds(*ui.rgba16_to_rgba8(n64)))
        # DS 4bpp OBJ always treats index 0 as transparent, matching these CI4
        # source assets' transparent palette entry.
        palette[0] &= 0x7FFF
        palettes.append(palette)
    return pack_obj4(indices, 8, 8), palettes


def c_array_u8(name: str, rows: list[bytes]) -> list[str]:
    width = len(rows[0]) if rows else 0
    lines = [f"static const u8 {name}[{len(rows)}][{width}] = {{"]
    for row in rows:
        values = ", ".join(f"0x{v:02x}" for v in row)
        lines.append(f"    {{ {values} }},")
    lines.append("};")
    return lines


def c_array_u16(name: str, rows: list[list[int]]) -> list[str]:
    width = len(rows[0]) if rows else 0
    lines = [f"static const u16 {name}[{len(rows)}][{width}] = {{"]
    for row in rows:
        values = ", ".join(f"0x{v:04x}" for v in row)
        lines.append(f"    {{ {values} }},")
    lines.append("};")
    return lines


def c_metric_u8(name: str, rows: list[tuple[int, int]]) -> list[str]:
    lines = [f"static const u8 {name}[{len(rows)}][2] = {{"]
    for width, height in rows:
        lines.append(f"    {{ {width}u, {height}u }},")
    lines.append("};")
    return lines


def bake(repo_root: Path, output: Path) -> None:
    ui = load_ui_generator(repo_root)
    offsets = ui.load_reloc_offsets(repo_root)
    o2r = repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r"

    interface_sets = [
        ("reloc_interface/IFCommonPlayerDamage", DAMAGE_SYMBOLS),
        ("reloc_interface/IFCommonTimer", TIMER_SYMBOLS),
        ("reloc_interface/IFCommonDigits", STOCK_DIGIT_SYMBOLS),
    ]
    glyph_groups: list[list[bytes]] = []
    glyph_metrics: list[list[tuple[int, int]]] = []
    for group_index, (rel_path, symbols) in enumerate(interface_sets):
        fileobj = ui.RelocFile(o2r / rel_path)
        rows = []
        metrics = []
        for symbol in symbols:
            if symbol not in offsets:
                raise BakeError(f"missing reloc offset for {symbol}")
            _, raster = ui.decode_sprite_raster(fileobj, symbol, offsets[symbol])
            raster, width, height = scale_raster(ui, raster)
            if width > 16 or height > 16:
                raise BakeError(f"{symbol}: scaled glyph {width}x{height} exceeds 16x16")
            if group_index == 0:
                # Damage digits are the one animated glyph family.  BattleShip
                # scales them around their per-character centre during damage
                # bounce; centring the AOT art in a transparent 32x32 OBJ cell
                # gives the DS affine unit enough bounds for the complete
                # source scale range used by normal attacks without a runtime
                # resample or a per-glyph pivot correction.
                cell_w = cell_h = 32
                origin_x = (cell_w - width) // 2
                origin_y = (cell_h - height) // 2
            else:
                cell_w = cell_h = 16
                origin_x = origin_y = 0
            rows.append(pack_obj4(
                intensity_indices(raster, width, height, cell_w, cell_h,
                                  origin_x, origin_y),
                cell_w, cell_h))
            metrics.append((width, height))
        glyph_groups.append(rows)
        glyph_metrics.append(metrics)

    portrait_file = ui.RelocFile(o2r / "reloc_menus" / "MNPlayersPortraits")
    portrait_gfx = []
    portrait_palettes = []
    for symbol in PORTRAIT_SYMBOLS:
        _, raster = ui.decode_sprite_raster(portrait_file, symbol, offsets[symbol])
        # 45x43 -> 16x15.  This is the smallest square OBJ cell that keeps a
        # recognizable portrait while leaving enough sub OBJ budget for the
        # four-player damage/stock presentation.
        raster = ui.box_scale(raster, 16, 15)
        gfx, palette = quantize_portrait(ui, raster, 16, 15)
        portrait_gfx.append(gfx)
        portrait_palettes.append(palette)

    mario_gfx, mario_palettes = stock_asset(ui, repo_root, MODEL_STOCK["MARIO"])
    fox_gfx, fox_palettes = stock_asset(ui, repo_root, MODEL_STOCK["FOX"])
    luigi_gfx, luigi_palettes = stock_asset(ui, repo_root, MODEL_STOCK["LUIGI"])
    donkey_gfx, donkey_palettes = stock_asset(ui, repo_root, MODEL_STOCK["DONKEY"])
    captain_gfx, captain_palettes = stock_asset(
        ui, repo_root, MODEL_STOCK["CAPTAIN"])
    samus_gfx, samus_palettes = stock_asset(ui, repo_root, MODEL_STOCK["SAMUS"])
    link_gfx, link_palettes = stock_asset(ui, repo_root, MODEL_STOCK["LINK"])
    pikachu_gfx, pikachu_palettes = stock_asset(
        ui, repo_root, MODEL_STOCK["PIKACHU"])
    kirby_gfx, kirby_palettes = stock_asset(ui, repo_root, MODEL_STOCK["KIRBY"])
    purin_gfx, purin_palettes = stock_asset(ui, repo_root, MODEL_STOCK["PURIN"])
    ness_gfx, ness_palettes = stock_asset(ui, repo_root, MODEL_STOCK["NESS"])
    yoshi_gfx, yoshi_palettes = stock_asset(ui, repo_root, MODEL_STOCK["YOSHI"])

    # Shared intensity palette for timer/stock-count glyphs.  Damage gets the
    # same fifteen intensity indices but its four palettes are generated live
    # from BattleShip's per-player damage colour curve.
    white_palette = [0]
    for i in range(1, 16):
        c = (i * 255 + 7) // 15
        white_palette.append(ui.rgba8_to_ds(c, c, c, 255))

    lines = [
        "/* GENERATED by scripts/menus/generate_battle_hud.py -- do not edit. */",
        "#ifndef NDS_BATTLE_HUD_GENERATED_INC",
        "#define NDS_BATTLE_HUD_GENERATED_INC",
        "",
        "#define NDS_BATTLE_HUD_DAMAGE_GLYPHS 11u",
        "#define NDS_BATTLE_HUD_TIMER_GLYPHS 11u",
        "#define NDS_BATTLE_HUD_STOCK_DIGIT_GLYPHS 11u",
        f"#define NDS_BATTLE_HUD_PORTRAITS {len(PORTRAIT_SYMBOLS)}u",
        f"#define NDS_BATTLE_HUD_STOCK_OWNERS {len(MODEL_STOCK)}u",
        "#define NDS_BATTLE_HUD_DAMAGE_GFX_BYTES 512u",
        "#define NDS_BATTLE_HUD_TIMER_GFX_BYTES 128u",
        "#define NDS_BATTLE_HUD_STOCK_DIGIT_GFX_BYTES 128u",
        "#define NDS_BATTLE_HUD_PORTRAIT_GFX_BYTES 128u",
        "#define NDS_BATTLE_HUD_STOCK_GFX_BYTES 32u",
        "#define NDS_BATTLE_HUD_STOCK_CONTENT_W 6u",
        "#define NDS_BATTLE_HUD_STOCK_CONTENT_H 8u",
        "",
    ]
    lines += c_array_u8("kNdsBattleHudDamageGfx", glyph_groups[0])
    lines += [""]
    lines += c_array_u8("kNdsBattleHudTimerGfx", glyph_groups[1])
    lines += [""]
    lines += c_array_u8("kNdsBattleHudStockDigitGfx", glyph_groups[2])
    lines += [""]
    lines += c_metric_u8("kNdsBattleHudDamageMetric", glyph_metrics[0])
    lines += [""]
    lines += c_metric_u8("kNdsBattleHudTimerMetric", glyph_metrics[1])
    lines += [""]
    lines += c_metric_u8("kNdsBattleHudStockDigitMetric", glyph_metrics[2])
    lines += [""]
    lines += c_array_u8("kNdsBattleHudPortraitGfx", portrait_gfx)
    lines += [""]
    lines += c_array_u8("kNdsBattleHudStockGfx", [
        mario_gfx, fox_gfx, luigi_gfx, donkey_gfx, captain_gfx, samus_gfx,
        link_gfx, pikachu_gfx, yoshi_gfx, ness_gfx, purin_gfx, kirby_gfx
])
    lines += [""]
    lines += c_array_u16("kNdsBattleHudPortraitPalette", portrait_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudMarioStockPalette", mario_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudFoxStockPalette", fox_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudLuigiStockPalette", luigi_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudDonkeyStockPalette", donkey_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudCaptainStockPalette", captain_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudSamusStockPalette", samus_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudLinkStockPalette", link_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudPikachuStockPalette", pikachu_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudYoshiStockPalette", yoshi_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudNessStockPalette", ness_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudPurinStockPalette", purin_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudKirbyStockPalette", kirby_palettes)
    lines += [""]
    lines += c_array_u16("kNdsBattleHudWhitePalette", [white_palette])
    lines += ["", "#endif /* NDS_BATTLE_HUD_GENERATED_INC */", ""]

    text = "\n".join(lines)
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_text(errors="replace") != text:
        output.write_text(text)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()
    root = args.repo_root.resolve()
    output = args.output or (root / "src" / "nds" / "generated" /
                             "battle_hud.generated.inc")
    try:
        bake(root, output)
    except (BakeError, Exception) as exc:
        # Keep one concise line in make output; ConvertError from the shared UI
        # decoder is intentionally surfaced here too.
        print(f"generate_battle_hud: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
