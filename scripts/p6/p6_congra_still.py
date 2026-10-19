"""P7 C1: an authored content's congratulations still (owner 2026-10-19).

Ike and Palutena have no Remix congratulations plates; the owner chose a
still rendered from their own model by the DS renderer on a plain backdrop.
The intro still bake renders it (scripts/menus/bake_1p_intro_stills.ps1
-Studio: the player card's camera aimed at the fighter, the 3D layer alone
display-captured with a transparent clear, studio_c<NN>.raw); this composes
that capture over the backdrop as the 300x220 picture the congratulations
screen shows in its two plates (mncongra.c: top at (10,10), bottom at
(10,120)), written as assets/congra/c<NN>.png. scripts/p4/p4_onep_art.py
turns it into the plate files at build time. Like the intro stills, the
pictures are renders of ROM-derived models and stay local.

    p6_congra_still.py --raw-dir artifacts/intro_bake/raw --out assets/congra 19 20
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

W, H = 300, 220
CAPTURE_W, CAPTURE_H = 256, 192
# The fighter's height in the picture, and the gap below its feet.
FIGURE_H = 196
FOOT_GAP = 8
# A plain backdrop: a soft vertical blend, light above to deeper below.
BACK_TOP = (238, 232, 216)
BACK_BOTTOM = (176, 196, 222)


def load_capture(path: Path) -> np.ndarray:
    """The bake's RGB15 capture as RGBA8 (alpha from the capture bit)."""
    raw = np.frombuffer(path.read_bytes(), dtype="<u2")[:CAPTURE_W * CAPTURE_H]
    raw = raw.reshape(CAPTURE_H, CAPTURE_W)
    rgba = np.zeros((CAPTURE_H, CAPTURE_W, 4), dtype=np.uint8)
    for i, shift in enumerate((0, 5, 10)):
        c = (raw >> shift) & 0x1F
        rgba[..., i] = (c << 3) | (c >> 2)
    rgba[..., 3] = np.where((raw & 0x8000) != 0, 255, 0)
    return rgba


def compose(capture: np.ndarray) -> Image.Image:
    ys, xs = np.nonzero(capture[..., 3])
    if len(xs) == 0:
        raise SystemExit("capture holds no fighter")
    figure = Image.fromarray(capture[ys.min():ys.max() + 1, xs.min():xs.max() + 1], "RGBA")
    scale = FIGURE_H / figure.height
    if figure.width * scale > W - 16:
        scale = (W - 16) / figure.width
    figure = figure.resize((max(1, round(figure.width * scale)),
                            max(1, round(figure.height * scale))), Image.LANCZOS)
    t = np.linspace(0.0, 1.0, H)[:, None]
    back = np.zeros((H, W, 3), dtype=np.float64)
    for i in range(3):
        back[..., i] = BACK_TOP[i] + (BACK_BOTTOM[i] - BACK_TOP[i]) * t
    picture = Image.fromarray(back.round().astype(np.uint8), "RGB").convert("RGBA")
    x = (W - figure.width) // 2
    y = H - FOOT_GAP - figure.height
    picture.alpha_composite(figure, (x, max(0, y)))
    return picture.convert("RGB")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--raw-dir", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("contents", type=int, nargs="+")
    args = ap.parse_args(argv)
    args.out.mkdir(parents=True, exist_ok=True)
    for content in args.contents:
        raw = args.raw_dir / f"studio_c{content:02d}.raw"
        picture = compose(load_capture(raw))
        picture.save(args.out / f"c{content:02d}.png")
        print(f"c{content:02d}: {W}x{H} from {raw.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
