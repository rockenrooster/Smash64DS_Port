#!/usr/bin/env python3
"""Pack baked 1P-intro fighter stills into DS texture files.

Input: raw 256x192 RGB15 dumps of the 3D layer (DS display capture, source A =
3D, little-endian u16, bit 15 set where a polygon was drawn), one per still,
from scripts/menus/bake_1p_intro_stills.ps1. A team member's dump also has a
<name>.depth sidecar: the member's distance from the stage camera (s32, x16).
Output: one `.s1i` file per still: the drawn pixels' bounding box as a 256-colour
texture (index 0 transparent) padded to power-of-two sides, plus where the box
sits on the 256x192 screen.

    u32 magic 'S1I1'; u16 x, y, w, h, tex_w, tex_h, colors, reserved;
    u16 palette[colors] (RGB15, entry 0 unused/transparent);
    u8 texels[tex_h][tex_w]

The three teams were baked one member per boot (a whole team does not fit the
intro's scene heap); their stills are composited here far to near, which is
the depth order within the source's one draw because the members never
interpenetrate:
    o01         the eighteen Yoshis (o01m00..17)
    o01_06_<c>  the same with every member i % 6 == c shaded (player Yoshi c)
    o08         the eight Kirbys with their hats (o08m00..07)
    o08_08_0    the recoloured team (player Kirby in costume 0)
    o12         the ten polygons (o12m<kind - nFTKindNStart>)
"""
from __future__ import annotations

import argparse
import pathlib
import struct
import sys

import numpy as np
from PIL import Image

MAGIC = 0x31493153  # "S1I1" little-endian
W, H = 256, 192

POLYGON_MEMBERS = (0, 1, 2, 3, 5, 6, 7, 8, 9, 11)


def team_composites() -> dict[str, list[str]]:
    teams = {
        'o01': ['o01m%02d' % i for i in range(18)],
        'o08': ['o08m%02d' % i for i in range(8)],
        'o08_08_0': ['o08m%02dk' % i for i in range(8)],
        'o12': ['o12m%02d' % i for i in POLYGON_MEMBERS],
    }
    for costume in range(4):
        teams['o01_06_%d' % costume] = [
            ('o01m%02ds' % i) if (i % 6) == costume else ('o01m%02d' % i)
            for i in range(18)]
    return teams


def is_member(stem: str) -> bool:
    return (len(stem) >= 5 and stem[0] == 'o' and stem[3] == 'm' and
            stem[1:3] in ('01', '08', '12'))


def load_raw(path: pathlib.Path) -> np.ndarray:
    raw = path.read_bytes()
    if len(raw) < W * H * 2:
        raise ValueError("%s: short capture (%d bytes)" % (path, len(raw)))
    return np.frombuffer(raw[:W * H * 2], dtype="<u2").reshape(H, W)


def composite(raw_dir: pathlib.Path, members: list[str]) -> np.ndarray:
    layers = []
    for stem in members:
        depth_path = raw_dir / (stem + '.depth')
        depth = struct.unpack('<i', depth_path.read_bytes()[:4])[0]
        if depth <= 0:
            raise ValueError("%s: no member depth" % stem)
        layers.append((depth, stem, load_raw(raw_dir / (stem + '.raw'))))
    out = np.zeros((H, W), dtype=np.uint16)
    for _depth, _stem, px in sorted(layers, key=lambda layer: -layer[0]):
        drawn = (px & 0x8000) != 0
        out[drawn] = px[drawn]
    return out


def pot(value: int) -> int:
    size = 8
    while size < value:
        size *= 2
    return size


def pack_one(px: np.ndarray) -> bytes:
    drawn = (px & 0x8000) != 0
    ys, xs = np.nonzero(drawn)
    if len(xs) == 0:
        raise ValueError("capture has no drawn pixel")
    x0, x1 = int(xs.min()), int(xs.max()) + 1
    y0, y1 = int(ys.min()), int(ys.max()) + 1
    box = px[y0:y1, x0:x1]
    mask = drawn[y0:y1, x0:x1]
    rgb = np.zeros((box.shape[0], box.shape[1], 3), dtype=np.uint8)
    rgb[..., 0] = ((box & 0x1F) << 3).astype(np.uint8)
    rgb[..., 1] = (((box >> 5) & 0x1F) << 3).astype(np.uint8)
    rgb[..., 2] = (((box >> 10) & 0x1F) << 3).astype(np.uint8)
    colors = np.unique(box[mask] & 0x7FFF)
    if len(colors) <= 255:
        lut = {int(c): i + 1 for i, c in enumerate(colors)}
        palette = [0] + [int(c) for c in colors]
        idx = np.zeros(box.shape, dtype=np.uint8)
        flat = box & 0x7FFF
        for c, i in lut.items():
            idx[(flat == c) & mask] = i
    else:
        img = Image.fromarray(rgb, "RGB")
        quant = img.quantize(colors=255, method=Image.Quantize.MEDIANCUT,
                             dither=Image.Dither.NONE)
        q = np.array(quant, dtype=np.uint16) + 1
        pal = quant.getpalette()[:255 * 3]
        palette = [0]
        for i in range(255):
            r, g, b = pal[i * 3:i * 3 + 3]
            palette.append((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10))
        idx = np.where(mask, q, 0).astype(np.uint8)
    w, h = x1 - x0, y1 - y0
    tw, th = pot(w), pot(h)
    tex = np.zeros((th, tw), dtype=np.uint8)
    tex[:h, :w] = idx
    head = struct.pack("<I8H", MAGIC, x0, y0, w, h, tw, th, len(palette), 0)
    return (head + struct.pack("<%dH" % len(palette), *palette) +
            tex.tobytes())


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("raw_dir", type=pathlib.Path)
    ap.add_argument("out_dir", type=pathlib.Path)
    args = ap.parse_args(argv)
    args.out_dir.mkdir(parents=True, exist_ok=True)
    stills = {}
    for raw in sorted(args.raw_dir.glob("*.raw")):
        if not is_member(raw.stem):
            stills[raw.stem] = load_raw(raw)
    for name, members in team_composites().items():
        if all((args.raw_dir / (stem + '.raw')).is_file() for stem in members):
            stills[name] = composite(args.raw_dir, members)
        else:
            print("%-24s missing members" % name)
    total = 0
    for name in sorted(stills):
        blob = pack_one(stills[name])
        (args.out_dir / (name + ".s1i")).write_bytes(blob)
        total += len(blob)
        print("%-24s %6d B" % (name, len(blob)))
    print("total %d B in %d stills" % (total, len(stills)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
