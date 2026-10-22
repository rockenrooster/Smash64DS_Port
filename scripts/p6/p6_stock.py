#!/usr/bin/env python3
"""P6 A9: a look's stock icon over its base's (the HUD's per-costume icon).

The base's stock sprite (FTAttributes.sprites: a CI4 sprite, one LUT a
costume) is rewritten in place in the look's model copy. Its bitmap's rows
are `width_img` texels apart (Roy's and Peach's: 16, of which the sprite
shows 8) and laid out for a block load: each odd row's two 32-bit halves are
swapped (TMEM's odd-line interleave), so an odd row's 8 shown texels are its
second half. Every written row is padded to the stride with index 0 (clear)
and odd rows swapped. Two ways:

- From the kit (Palutena): the kit's stock icon row (the sheet's one row a
  fighter, one column a kit costume) is resized to the sprite's size, its
  default costume quantized to the shared index image (index 0 clear), and
  each costume's LUT is the mean colour of that costume's icon under each
  index (p6_costumes.py's costume picks, as many as the base has LUTs), a
  costume's hair recolour (p6_costumes.HAIR_RECOLORS) on its hair entries.
- A recolour of the base's own icon (Ike, owner 2026-10-14: "you can just
  recolor roy's stock sprites"): the base's texels stay; each costume's LUT
  is the base's first costume's, its hair indices moved to the look's hair
  hue and its headband index set to the costume's headband colour
  (RECOLORS).

The output is model_writer.py --textures input.

    p6_stock.py --name palutena --base-export builds/p4-staging/export/peach \\
        --out assets/p6-art/palutena/costumes/stock.json [--preview stock.png]
    p6_stock.py --name ike --base-export builds/p4-staging/export/roy \\
        --out assets/p6-art/ike/costumes/stock.json

Kit-derived and base-derived: local output (D10).
"""
from __future__ import annotations

import argparse
import colorsys
import json
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "p4"))
import n64_model as N  # noqa: E402
import p6_costumes  # noqa: E402

SHEET = (REPO / "decomp" / "alt_assets" / "Fighters" / "Super Smash Bros 3DS" /
         "3DS - Super Smash Bros. for Nintendo 3DS - Fighters - Stock Icons (Small).png")
CELL = 15                       # the sheet's pitch: a 14-pixel icon and a grid line
ROWS = {"palutena": 28}
KIT_COLUMN = {f"c0{k}": k for k in range(8)}
# A recoloured base icon: the base's indices that draw the hair (taken from
# its first costume's LUT to the look's hair hue, `sat` and `val` scaling
# the colour's own) and the one that draws the headband (each costume's
# colour, RGB hex). Ike: Roy's icon (indices 2, 4, 5, 6 his red hair, 3 his
# headband) with Ike's navy-blue hair on every costume (his look keeps it)
# and the headbands of his six costumes (olive, red, green, silver; gold for
# the gold costume, which keeps the olive cloth on the model, so the icon
# tells it from the default; grey for the black one).
RECOLORS = {
    "ike": {"hair": {"indices": [2, 4, 5, 6], "to_hue": 228, "sat": 0.9, "val": 0.62},
            "band": {"index": 3,
                     "colours": ["5e7a2e", "b02818", "3c9c1c", "d8dcd8", "c8a030", "8c9090"]}},
}
SPRITE_W, SPRITE_H = 0x04, 0x06       # Sprite.width/height (s16)
SPRITE_BITMAP = 52                    # Sprite.bitmap
BITMAP_WIDTH_IMG = 2                  # Bitmap.width_img (the row stride)
BITMAP_BUF = 8                        # Bitmap.buf


class StockError(SystemExit):
    pass


def sprites(fm: N.FighterModel) -> tuple[int, list[int], int, int, int]:
    """The base's stock sprite: its texel buffer offset, its LUT offsets
    (model file), width, height and row stride (texels)."""
    lay = fm.lay
    ref = fm.main.ptr(fm.attr + lay["FTAttributes.sprites"])
    if ref is None or ref[0] != "intern":
        raise StockError("FTAttributes.sprites is not an internal pointer")
    base = ref[2]
    stock = fm.main.ptr(base)
    luts = []
    arr = fm.main.ptr(base + 4)
    at = arr[2]
    while at < base and fm.main.ptr(at) is not None:
        luts.append(fm.main.ptr(at)[2])
        at += 4
    sprite = stock[2]
    w, h = struct.unpack_from(">hh", fm.model.data, sprite + SPRITE_W)
    bitmap = fm.model.ptr(sprite + SPRITE_BITMAP)
    stride = struct.unpack_from(">h", fm.model.data, bitmap[2] + BITMAP_WIDTH_IMG)[0]
    if stride < w or stride % 2:
        raise StockError(f"stock bitmap stride {stride} for a {w}-texel sprite")
    buf = fm.model.ptr(bitmap[2] + BITMAP_BUF)
    return buf[2], luts, w, h, stride


def padded(idx: np.ndarray, stride: int) -> list:
    """Rows padded to the bitmap's stride with index 0 (clear), each odd row
    with its 32-bit halves (8 CI4 texels each) swapped."""
    out = []
    for y, row in enumerate(idx):
        row = list(map(int, row)) + [0] * (stride - len(row))
        if y % 2:
            row = [v for k in range(0, stride, 16) for v in row[k + 8:k + 16] + row[k:k + 8]]
        out.append(row)
    return out


def shown(rows: list, width: int) -> np.ndarray:
    """The sprite's shown texels from its stored rows (padded()'s inverse)."""
    out = []
    for y, row in enumerate(rows):
        if y % 2:
            row = [v for k in range(0, len(row), 16) for v in row[k + 8:k + 16] + row[k:k + 8]]
        out.append(row[:width])
    return np.array(out)


def icons(name: str, picks: list[str], size: tuple[int, int]) -> list[np.ndarray]:
    from PIL import Image
    sheet = Image.open(SHEET).convert("RGBA")
    r = ROWS[name]
    out = []
    for pick in picks:
        c = KIT_COLUMN[pick]
        cell = sheet.crop((c * CELL + 1, r * CELL + 1, c * CELL + CELL, r * CELL + CELL))
        out.append(np.asarray(cell.resize(size, Image.BOX)).astype(np.float32))
    return out


def build_kit(name: str, fm: N.FighterModel) -> tuple[list, list, np.ndarray]:
    buf, luts, w, h, stride = sprites(fm)
    picks = p6_costumes.COSTUMES[name]["picks"][:len(luts)]
    imgs = icons(name, picks, (w, h))
    import look_blend
    idx, pal0 = look_blend.quantize(imgs[0].astype(np.uint8), 16, True)
    idx = np.array(idx)
    writes = []
    pals = []
    recolors = p6_costumes.HAIR_RECOLORS.get(name, {})
    for k, img in enumerate(imgs):
        pal = []
        for i in range(16):
            m = (idx == i) & (img[..., 3] >= 128)
            if i == 0 or not m.any():
                pal.append(list(pal0[i]))
            else:
                pal.append([int(round(c)) for c in img[m][:, :3].mean(axis=0)] + [255])
        if k in recolors:
            pal = [list(pal[0])] + p6_costumes.recolor(pal[1:], recolors[k],
                                                       p6_costumes.hair_top([pal[1:]]))
        pals.append(pal)
        writes.append({"image": buf, "texels": padded(idx, stride), "palette": pal, "palette_at": [luts[k]]})
    return writes, pals, idx


def _rgba5551(v: int) -> list[int]:
    return [((v >> 11) & 31) * 255 // 31, ((v >> 6) & 31) * 255 // 31, ((v >> 1) & 31) * 255 // 31,
            255 if v & 1 else 0]


def build_recolor(name: str, fm: N.FighterModel) -> tuple[list, list, np.ndarray]:
    buf, luts, w, h, stride = sprites(fm)
    spec = RECOLORS[name]
    data = fm.model.data[buf:buf + stride * h // 2]
    rows = []
    for y in range(h):
        line = data[y * stride // 2:(y + 1) * stride // 2]
        rows.append([v for b in line for v in (b >> 4, b & 15)])
    idx = shown(rows, w)
    base = [_rgba5551(int.from_bytes(fm.model.data[luts[0] + 2 * i:luts[0] + 2 * i + 2], "big"))
            for i in range(16)]
    hair = spec["hair"]
    band = spec["band"]
    if len(band["colours"]) < len(luts):
        raise StockError(f"{name}: {len(band['colours'])} headband colours for {len(luts)} LUTs")
    writes = []
    pals = []
    for k, lut in enumerate(luts):
        pal = [list(c) for c in base]
        for i in hair["indices"]:
            r, g, b, a = pal[i]
            _, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            nr, ng, nb = colorsys.hsv_to_rgb(hair["to_hue"] / 360, min(1.0, s * hair["sat"]),
                                             min(1.0, v * hair["val"]))
            pal[i] = [int(round(nr * 255)), int(round(ng * 255)), int(round(nb * 255)), a]
        c = band["colours"][k]
        pal[band["index"]] = [int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16), 255]
        pals.append(pal)
        writes.append({"image": buf, "texels": rows, "palette": pal, "palette_at": [lut]})
    return writes, pals, idx


def build(name: str, base_export: Path) -> tuple[list, list, np.ndarray]:
    fm = N.FighterModel.from_export(base_export)
    if name in RECOLORS:
        return build_recolor(name, fm)
    return build_kit(name, fm)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--name", required=True, choices=sorted(set(ROWS) | set(RECOLORS)))
    ap.add_argument("--base-export", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--preview", type=Path)
    args = ap.parse_args()
    writes, pals, idx = build(args.name, args.base_export)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes((json.dumps(writes) + "\n").encode("utf-8"))
    print(f"p6 stock: {len(writes)} costumes -> {args.out}")
    if args.preview:
        from PIL import Image
        tiles = [Image.fromarray(np.array(p, dtype=np.uint8)[idx], "RGBA").resize((idx.shape[1] * 8, idx.shape[0] * 8),
                                                                                   Image.NEAREST) for p in pals]
        sheet = Image.new("RGBA", (sum(t.width + 4 for t in tiles), tiles[0].height), (60, 60, 60, 255))
        x = 0
        for t in tiles:
            sheet.paste(t, (x, 0), t)
            x += t.width + 4
        sheet.save(args.preview)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
