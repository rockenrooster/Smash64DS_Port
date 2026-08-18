#!/usr/bin/env python3
"""P2-1h. Bake the DS system-menu banner icon from the original title emblem.

WHY THIS EXISTS
---------------
The owner's 2026-08-18 ruling is that this is a port, so the original branding
ships -- and the banner is the first branding a player sees, before the ROM
even boots.  Without this the ROM carries devkitPro's default calico icon,
which is neither the original's nor ours.

THE SOURCE ASSET is `llMNTitleLogoAnimFullSprite` in `reloc_menus/MNTitle`:
the 128x124 I4 Smash Bros emblem that `mnTitleMakeLogoNoOpening` draws on the
title screen (mntitle.c:1051).  It is an intensity STENCIL -- the source's own
combiner takes colour from the primitive and alpha from the texel -- so the
icon is that stencil at the primitive the title gives it, resolved into the
sixteen colours a DS banner has.

THE OUTPUT FORMAT is what `ndstool -b` reads: a 32x32 Windows BMP with a
sixteen-entry palette.  Structurally identical to devkitPro's own
`calico/share/nds-icon.bmp` (8 bits per index, 16 RGBQUADs, 118-byte offset),
because that file is the existing proof of what the tool accepts.  Palette
entry 0 is the DS banner's transparent index, so the emblem sits on nothing
and takes the system menu's own background.

Run with --preview-dir to write a PNG of the result before spending a build.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from generate_mn_ui_kit import (  # noqa: E402
    ConvertError, RelocFile, box_scale, decode_sprite_raster,
    load_reloc_offsets, write_png,
)

ICON_W = ICON_H = 32
PALETTE_ENTRIES = 16
BMP_HEADER_BYTES = 14 + 40 + (PALETTE_ENTRIES * 4)

SOURCE_O2R = "MNTitle"
SOURCE_SYMBOL = "llMNTitleLogoAnimFullSprite"
# mnTitleMakeLogoNoOpening, mntitle.c:1073 -- the emblem's own primitive.
SOURCE_PRIM = (0xFF, 0x00, 0x00)


def build_icon(repo_root: Path) -> tuple[list[int], list[tuple[int, int, int]]]:
    """The emblem as 32x32 palette indices plus its sixteen-entry palette."""
    offsets = load_reloc_offsets(repo_root)
    if SOURCE_SYMBOL not in offsets:
        raise ConvertError(f"{SOURCE_SYMBOL} has no offset in either header")
    path = (repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" /
            "reloc_menus" / SOURCE_O2R)
    fileobj = RelocFile(path)
    _, raster = decode_sprite_raster(fileobj, SOURCE_SYMBOL,
                                     offsets[SOURCE_SYMBOL], (255, 255, 255),
                                     alpha_ramp=True)
    raster = box_scale(raster, ICON_W, ICON_H)

    # Fifteen opaque steps of the emblem's own primitive, plus the transparent
    # index at 0.  A ramp rather than a threshold because a 32x32 downscale of
    # a 128x124 stencil is mostly partial coverage: thresholding it produces a
    # jagged blob, and the banner has fifteen colours going spare.
    palette = [(0, 0, 0)]
    for step in range(1, PALETTE_ENTRIES):
        scale = step / (PALETTE_ENTRIES - 1)
        palette.append((int(SOURCE_PRIM[0] * scale),
                        int(SOURCE_PRIM[1] * scale),
                        int(SOURCE_PRIM[2] * scale)))

    indices = []
    for row in raster:
        for (_, _, _, alpha) in row:
            # Alpha IS the coverage here: the stencil's intensity ramp survived
            # the downscale as alpha, and colour is the flat primitive.
            step = (alpha * (PALETTE_ENTRIES - 1) + 127) // 255
            indices.append(step)
    return indices, palette


def write_bmp(path: Path, indices: list[int],
              palette: list[tuple[int, int, int]]) -> None:
    """32x32 8-bit BMP, bottom-up, sixteen palette entries."""
    if len(indices) != ICON_W * ICON_H:
        raise ConvertError("icon is not 32x32")
    body = bytearray()
    for y in range(ICON_H - 1, -1, -1):  # BMP rows run bottom-up
        body += bytes(indices[y * ICON_W:(y + 1) * ICON_W])
    table = bytearray()
    for (r, g, b) in palette:
        table += bytes((b, g, r, 0))
    header = struct.pack("<2sIHHI", b"BM",
                         BMP_HEADER_BYTES + len(body), 0, 0, BMP_HEADER_BYTES)
    info = struct.pack("<IiiHHIIiiII", 40, ICON_W, ICON_H, 1, 8, 0, 0,
                       0, 0, PALETTE_ENTRIES, PALETTE_ENTRIES)
    data = bytes(header + info + table + body)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() == data:
        return
    path.write_bytes(data)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".", type=Path)
    parser.add_argument("--preview-dir", type=Path, default=None)
    args = parser.parse_args(argv)

    repo_root = args.repo_root.resolve()
    indices, palette = build_icon(repo_root)
    out = repo_root / "assets" / "banner" / "smash64ds_icon.bmp"
    write_bmp(out, indices, palette)

    if args.preview_dir is not None:
        # Checkerboard behind index 0 so the transparent index is visible.
        rgb = bytearray()
        for y in range(ICON_H):
            for x in range(ICON_W):
                index = indices[y * ICON_W + x]
                if index == 0:
                    shade = 0x60 if (((x >> 2) ^ (y >> 2)) & 1) else 0x30
                    rgb += bytes((shade, shade, shade))
                else:
                    rgb += bytes(palette[index])
        write_png(args.preview_dir.resolve() / "nds_banner_icon.png",
                  ICON_W, ICON_H, bytes(rgb))

    opaque = sum(1 for index in indices if index != 0)
    print(f"nds banner icon: {out.name}, {opaque}/{ICON_W * ICON_H} "
          f"opaque texels, {PALETTE_ENTRIES} palette entries")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ConvertError as exc:
        print(f"generate_nds_banner_icon: {exc}", file=sys.stderr)
        sys.exit(1)
