#!/usr/bin/env python3
"""Host-convert VS battle and Results wallpapers to DS-native NitroFS assets.

WHAT THIS CONVERTS AND WHY IT IS A BUILD STEP
---------------------------------------------
BattleShip draws each stage's backdrop as a 300x220 N64 sprite
(`gMPCollisionGroundData->wallpaper`, gr/grwallpaper.c:147) and the VS
Results backdrop as `llMNVSResultsWallpaperSprite` (mn/mnvsmode/mnvsresults.c:764).
Both are RELO sprite containers the DS must never decode at runtime
(PROJECT_GOAL: compile-time conversion; AGENTS.md: loading time is cheap,
gameplay CPU is not). This script resolves the containers once, on the host,
and emits NitroFS payloads plus a C manifest the runtime compiles in.

SOURCE IDENTITIES (all measured, none guessed)
  Battle (9 VS stages, 1P paused): the stage-select preview draws
    `sMNMapsGroundInfo->wallpaper` (mnmaps.c:956), and each GR map header
    names its own wallpaper container -- Castle borrows the opening movie's
    room (Makefile P2-4 stage 2: GRCastleMap references
    dMVOpeningRoomWallpaper_sprite_0x26C88), Hyrule borrows StageCastle,
    Inishie borrows StageHyruleWallpaper, Yamabuki borrows StagePokemon,
    the rest borrow their same-named Stage container. Every container below
    was verified to hold a 300x220 RGBA16 sprite at payload offset 0x26C88
    with the listed file id; anything else is a ConvertError.
  Results: `llMNVSResultsWallpaperSprite` at payload offset 0xD5C8 in
    MNVSResults (file 0x22), 300x220 I4. The source combiner is
    PRIMITIVE/ENVIRONMENT over TEXEL0 (mnvsresults.c:679
    `mnVSResultsWallpaperProcDisplay`), so the bake stores intensity indices
    as u8 and the live prim/env palette stays dynamic.

NATIVE FORMAT
  Native dimensions are source dimensions times 4/5 (300x220 -> 240x176),
  nearest pixel-center sampling: dest texel (dx, dy) samples source texel
    sx = ((2*dx+1)*src_w)//(2*dst_w), sy = ((2*dy+1)*src_h)//(2*dst_h).
  Battle pixels are little-endian opaque DS RGB555 with bit 15 set. A source
    texel whose alpha is not 255 is a ConvertError -- never silently forced.
  Results pixels are one u8 intensity index each (source nibble * 17).

OUTPUTS (both chosen by CLI flags)
  <output-dir>/native_wallpaper_<key>.bin   -- NitroFS payloads
  <header>                                   -- generated C table of asset
    file_id, source bitmap payload offset, source width/height, native
    width/height, format, and NitroFS filename.

PIXEL DECODE IS REUSED, NOT REIMPLEMENTED: RelocFile/decode_sprite_raster
come from scripts/menus/generate_mn_ui_kit.py. There is no runtime source
graphics decoder.
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: F401,E402

from generate_mn_ui_kit import (  # noqa: E402
    ConvertError,
    RelocFile,
    decode_sprite_raster,
)

G_IM_FMT_RGBA, G_IM_FMT_YUV, G_IM_FMT_CI, G_IM_FMT_IA, G_IM_FMT_I = range(5)
G_IM_SIZ_4b, G_IM_SIZ_8b, G_IM_SIZ_16b, G_IM_SIZ_32b = range(4)

SCALE_NUM, SCALE_DEN = 4, 5

FORMAT_BATTLE_RGB555 = 0
FORMAT_RESULTS_I8 = 1


@dataclass(frozen=True)
class WallpaperSource:
    key: str          # NitroFS stem suffix and manifest name
    o2r: str          # container path under BattleShip_o2r
    file_id: int      # expected RELO file id
    sprite_offset: int  # source Sprite record offset (not its Bitmap table)
    battle: bool      # False selects the Results I4 path
    runtime_asset_id: int | None = None  # DS registry identity can alias file_id


# The nine VS stages (1P paused: no Last/Metal/small/bonus/training
# wallpapers) plus VS Results. Container choice per map header extern list,
# quoted in the module docstring; order follows dMNMapsFileInfos.
SOURCES: tuple[WallpaperSource, ...] = (
    WallpaperSource("pupupu", "reloc_stages/StageDreamLand", 0x58, 0x26C88, True, 0x10058),
    WallpaperSource("zebes", "reloc_stages/StageZebes", 0x59, 0x26C88, True, 0x10059),
    WallpaperSource("jungle", "reloc_stages/StageJungle", 0x5C, 0x26C88, True, 0x1005C),
    WallpaperSource("yoster", "reloc_stages/StageYoshi", 0x5D, 0x26C88, True, 0x1005D),
    WallpaperSource("yamabuki", "reloc_stages/StagePokemon", 0x5E, 0x26C88, True, 0x1005E),
    WallpaperSource("castle", "reloc_movies/MVOpeningRoomWallpaper", 0x5A, 0x26C88, True),
    WallpaperSource("sector", "reloc_stages/StageSector", 0x63, 0x26C88, True, 0x10063),
    WallpaperSource("hyrule", "reloc_stages/StageCastle", 0x5F, 0x26C88, True, 0x1005F),
    WallpaperSource("inishie", "reloc_stages/StageHyruleWallpaper", 0x5B, 0x26C88, True, 0x1005B),
    WallpaperSource("results", "reloc_menus/MNVSResults", 0x22, 0xD5C8, False),
)


@dataclass
class ConvertedAsset:
    source: WallpaperSource
    src_w: int
    src_h: int
    native_w: int
    native_h: int
    format: int
    filename: str
    payload: bytes
    bitmap_offset: int


def native_size(src: int) -> int:
    """Source dimension times 4/5; non-divisible dimensions are a falsifier."""
    if (src * SCALE_NUM) % SCALE_DEN != 0:
        raise ConvertError(
            f"source dimension {src} is not divisible by {SCALE_DEN}")
    return (src * SCALE_NUM) // SCALE_DEN


def sample_index(dst: int, src_len: int, dst_len: int) -> int:
    """Nearest pixel-center source index for dest index `dst`."""
    return ((2 * dst + 1) * src_len) // (2 * dst_len)


def rgba8_to_ds_opaque(red: int, green: int, blue: int, alpha: int,
                       where: str) -> int:
    """RGBA8888 -> little-endian DS RGB555. Non-opaque alpha is rejected."""
    if alpha != 255:
        raise ConvertError(
            f"{where}: unexpected alpha {alpha}, refusing to force opaque")
    return ((1 << 15) | ((blue >> 3) << 10) | ((green >> 3) << 5) |
            (red >> 3))


def convert_source(repo_root: Path, source: WallpaperSource) -> ConvertedAsset:
    path = repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" / source.o2r
    fileobj = RelocFile(path)
    if fileobj.file_id != source.file_id:
        raise ConvertError(
            f"{source.key}: file id {fileobj.file_id:#x} != "
            f"expected {source.file_id:#x}")
    sprite, raster = decode_sprite_raster(fileobj, source.key,
                                          source.sprite_offset)
    src_w, src_h = sprite.width, sprite.height
    if (src_w, src_h) != (300, 220):
        raise ConvertError(
            f"{source.key}: {src_w}x{src_h} is not the 300x220 wallpaper")
    if len(raster) != src_h or len(raster[0]) != src_w:
        raise ConvertError(f"{source.key}: raster shape does not match sprite")
    native_w, native_h = native_size(src_w), native_size(src_h)

    if source.battle:
        if sprite.bmfmt != G_IM_FMT_RGBA or sprite.bmsiz != G_IM_SIZ_16b:
            raise ConvertError(
                f"{source.key}: expected RGBA16, got fmt={sprite.bmfmt} "
                f"siz={sprite.bmsiz}")
        out = bytearray()
        for dy in range(native_h):
            sy = sample_index(dy, src_h, native_h)
            for dx in range(native_w):
                sx = sample_index(dx, src_w, native_w)
                red, green, blue, alpha = raster[sy][sx]
                out += struct.pack("<H", rgba8_to_ds_opaque(
                    red, green, blue, alpha, f"{source.key}@({sx},{sy})"))
        filename = f"native_wallpaper_{source.key}.bin"
        return ConvertedAsset(source, src_w, src_h, native_w, native_h,
                              FORMAT_BATTLE_RGB555, filename, bytes(out),
                              sprite.bitmap)

    if sprite.bmfmt != G_IM_FMT_I or sprite.bmsiz != G_IM_SIZ_4b:
        raise ConvertError(
            f"{source.key}: expected I4, got fmt={sprite.bmfmt} "
            f"siz={sprite.bmsiz}")
    out = bytearray()
    for dy in range(native_h):
        sy = sample_index(dy, src_h, native_h)
        for dx in range(native_w):
            sx = sample_index(dx, src_w, native_w)
            red, green, blue, _alpha = raster[sy][sx]
            if not (red == green == blue):
                raise ConvertError(
                    f"{source.key}@({sx},{sy}): intensity channels disagree "
                    f"({red},{green},{blue})")
            out.append(red)
    filename = f"native_wallpaper_{source.key}.bin"
    return ConvertedAsset(source, src_w, src_h, native_w, native_h,
                          FORMAT_RESULTS_I8, filename, bytes(out), sprite.bitmap)


def render_header(assets: list[ConvertedAsset]) -> str:
    lines = [
        "// Generated by scripts/stages/generate_native_wallpapers.py --",
        "// do not edit. Asset file_id, source bitmap payload offset, source",
        "// width/height, native width/height, format, NitroFS filename.",
        "// Format 0: little-endian opaque DS RGB555 (bit 15 set),",
        "// row-major, native_w*native_h halfwords.",
        "// Format 1: u8 intensity indices (source nibble * 17), row-major,",
        "// native_w*native_h bytes; the live prim/env palette stays dynamic.",
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "typedef struct NDSNativeWallpaper {",
        "    uint32_t asset_id; // DS registry identity, including aliases",
        "    uint32_t file_id;",
        "    uint32_t src_offset;",
        "    uint16_t src_w;",
        "    uint16_t src_h;",
        "    uint16_t native_w;",
        "    uint16_t native_h;",
        "    uint16_t format;",
        "    const char *nitro_filename;",
        "} NDSNativeWallpaper;",
        "",
        "static const NDSNativeWallpaper kNDSNativeWallpapers[] = {",
    ]
    for asset in assets:
        runtime_id = (asset.source.file_id if asset.source.runtime_asset_id is None
                      else asset.source.runtime_asset_id)
        lines.append(
            f'    {{ 0x{runtime_id:X}, 0x{asset.source.file_id:02X}, '
            f'0x{asset.bitmap_offset:05X}, '
            f'{asset.src_w}, {asset.src_h}, '
            f'{asset.native_w}, {asset.native_h}, '
            f'{asset.format}, "{asset.filename}" }}, // {asset.source.key}')
    lines += [
        "};",
        "",
        f"static const uint32_t kNDSNativeWallpaperCount = {len(assets)};",
        "",
    ]
    return "\n".join(lines)


def generate(repo_root: Path, output_dir: Path,
             header_path: Path) -> list[ConvertedAsset]:
    """Convert every source and write the NitroFS payloads plus the C table."""
    output_dir.mkdir(parents=True, exist_ok=True)
    assets = [convert_source(repo_root, source) for source in SOURCES]
    for asset in assets:
        (output_dir / asset.filename).write_bytes(asset.payload)
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(render_header(assets), encoding="utf-8")
    return assets


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Host-convert VS/Results wallpapers to DS-native assets.")
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output-dir", default="builds/native_wallpapers",
                        help="directory for the NitroFS .bin payloads "
                             "(repo-relative or absolute)")
    parser.add_argument("--header", default=None,
                        help="generated C table path "
                             "(default: <output-dir>/native_wallpapers.generated.inc)")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    repo_root = args.repo_root.resolve()
    output_dir = Path(args.output_dir)
    if not output_dir.is_absolute():
        output_dir = repo_root / output_dir
    header_path = Path(args.header) if args.header is not None else \
        output_dir / "native_wallpapers.generated.inc"
    if not header_path.is_absolute():
        header_path = repo_root / header_path
    try:
        assets = generate(repo_root, output_dir, header_path)
    except ConvertError as exc:
        print(f"generate_native_wallpapers: {exc}", file=sys.stderr)
        return 1
    for asset in assets:
        print(f"{asset.filename}: src {asset.src_w}x{asset.src_h} "
              f"native {asset.native_w}x{asset.native_h} "
              f"format {asset.format} {len(asset.payload)} bytes")
    print(f"header: {header_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
