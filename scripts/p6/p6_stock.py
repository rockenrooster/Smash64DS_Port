#!/usr/bin/env python3
"""P6 A9: a look's stock icon over its base's (the HUD's per-costume icon).

The base's stock sprite (FTAttributes.sprites: a CI4 sprite, one LUT a
costume) is rewritten in place in the look's model copy: the kit's stock
icon row (the sheet's one row a fighter, one column a kit costume) is
resized to the sprite's size, its default costume quantized to the shared
index image (index 0 clear), and each costume's LUT is the mean colour of
that costume's icon under each index (p6_costumes.py's costume picks, as
many as the base has LUTs). The output is model_writer.py --textures input.

    p6_stock.py --name palutena --base-export builds/p4-staging/export/peach \\
        --out builds/p6-art/palutena/costumes/stock.json [--preview stock.png]

Kit-derived: local output (D10).
"""
from __future__ import annotations

import argparse
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
SPRITE_W, SPRITE_H = 0x04, 0x06       # Sprite.width/height (s16)
SPRITE_BITMAP = 52                    # Sprite.bitmap
BITMAP_BUF = 8                        # Bitmap.buf


class StockError(SystemExit):
    pass


def sprites(fm: N.FighterModel) -> tuple[int, list[int], int, int]:
    """The base's stock sprite: its texel buffer offset, its LUT offsets
    (model file), width and height."""
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
    buf = fm.model.ptr(bitmap[2] + BITMAP_BUF)
    return buf[2], luts, w, h


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


def build(name: str, base_export: Path) -> tuple[list, list]:
    fm = N.FighterModel.from_export(base_export)
    buf, luts, w, h = sprites(fm)
    picks = p6_costumes.COSTUMES[name]["picks"][:len(luts)]
    imgs = icons(name, picks, (w, h))
    sys.path.insert(0, str(HERE))
    import look_blend
    idx, pal0 = look_blend.quantize(imgs[0].astype(np.uint8), 16, True)
    idx = np.array(idx)
    writes = []
    pals = []
    for k, img in enumerate(imgs):
        pal = []
        for i in range(16):
            m = (idx == i) & (img[..., 3] >= 128)
            if i == 0 or not m.any():
                pal.append(list(pal0[i]))
            else:
                pal.append([int(round(c)) for c in img[m][:, :3].mean(axis=0)] + [255])
        pals.append(pal)
        writes.append({"image": buf, "texels": idx.tolist(), "palette": pal, "palette_at": [luts[k]]})
    return writes, pals


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--name", required=True, choices=sorted(ROWS))
    ap.add_argument("--base-export", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--preview", type=Path)
    args = ap.parse_args()
    writes, pals = build(args.name, args.base_export)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes((json.dumps(writes) + "\n").encode("utf-8"))
    print(f"p6 stock: {len(writes)} costumes -> {args.out}")
    if args.preview:
        from PIL import Image
        idx = np.array(writes[0]["texels"])
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
