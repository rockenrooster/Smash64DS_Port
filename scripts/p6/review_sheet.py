#!/usr/bin/env python3
"""Join a look's review renders (scripts/p6/blender/review.py) into one sheet
(P6 A6): the four close-up views in a row, then the battle-size view as the
DS shows it (256x192, nearest-neighbour) enlarged 3x.

    review_sheet.py <render dir> --out sheet.png [--title "..."]
"""
from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw


def sheet(src: Path, out: Path, title: str = "") -> Path:
    views = [Image.open(src / f"{v}.png").convert("RGB") for v in ("front", "three_quarter", "side", "back")]
    battle = Image.open(src / "battle_raw.png").convert("RGB").resize((768, 576), Image.NEAREST)
    w = sum(v.width for v in views)
    h = views[0].height + battle.height + 40
    out_img = Image.new("RGB", (w, h), (32, 32, 32))
    x = 0
    for v in views:
        out_img.paste(v, (x, 30))
        x += v.width
    out_img.paste(battle, (0, 30 + views[0].height + 10))
    d = ImageDraw.Draw(out_img)
    d.text((8, 8), title or src.name, fill=(230, 230, 230))
    d.text((780, 30 + views[0].height + 20), "battle size: 256x192 DS screen, 3x", fill=(200, 200, 200))
    out_img.save(out)
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("src", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--title", default="")
    args = ap.parse_args()
    print(sheet(args.src, args.out, args.title))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
