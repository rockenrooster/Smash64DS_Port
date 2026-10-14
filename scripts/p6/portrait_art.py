#!/usr/bin/env python3
"""P6 A9: an authored content's select portrait from the owner's art.

The owner supplied a portrait for each P6 fighter (2026-10-14, p6d: "style it
to match the rest of the roster"; local, under builds/p6-art/<name>/owner_art/,
like the kit). This trims the image viewer's frame from it, keys its
background out (the pixels of the background's colour class connected to the
image's edge), crops it to the roster's framing (the face about half the
picture or more, as Remix's portraits) and grades it, and writes the
transparent `builds/p6-art/<name>/portrait.png` that p6_present.py frames
over the select's fire plate.

    portrait_art.py --name palutena [--preview out.png]

Owner-derived: the outputs stay local (D10); the recipe is committed.
"""
from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
# The image viewer's frame line around the owner's screenshots.
VIEWER_FRAME = (39, 126, 127)
# The working width the source is area-averaged to (its art is about 64
# pixels wide, shown enlarged).
WORK = 128
# Per content: the background's colour class (hue range in degrees, which
# may wrap past 360; saturation and value ranges, 0-1), the square crop at
# the working size (x, y, side) and the grade (gamma, gain, saturation).
ART = {
    # A teal backdrop behind green hair (hue 125-136) and a gold crown.
    "palutena": {"key": {"h": (148, 190), "s": (0.15, 1.0), "v": (0.35, 1.0)},
                 "crop": (12, 12, 104), "gamma": 1.25, "gain": 0.95, "sat": 1.15},
    # A pale pink backdrop (hue 12) behind blue hair; his skin is hue 30.
    "ike": {"key": {"h": (335, 22), "s": (0.08, 0.45), "v": (0.45, 1.0)},
            "crop": (27, 16, 100), "gamma": 1.1, "gain": 1.0, "sat": 1.1},
}


class ArtError(SystemExit):
    pass


def trim_frame(a: np.ndarray) -> np.ndarray:
    """The image without the viewer's frame rows and columns."""
    frame = np.array(VIEWER_FRAME)

    def framed(line: np.ndarray) -> bool:
        return bool((np.abs(line.astype(int) - frame).max(axis=-1) < 12).mean() > 0.5)

    h, w = a.shape[:2]
    top, bot, left, right = 0, h, 0, w
    while top < h // 4 and framed(a[top]):
        top += 1
    while bot > 3 * h // 4 and framed(a[bot - 1]):
        bot -= 1
    while left < w // 4 and framed(a[:, left]):
        left += 1
    while right > 3 * w // 4 and framed(a[:, right - 1]):
        right -= 1
    return a[top:bot, left:right]


def hsv(rgb: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Hue (degrees), saturation and value of 0-255 RGB."""
    a = rgb / 255.0
    mx, mn = a.max(-1), a.min(-1)
    d = mx - mn
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    h = np.zeros(mx.shape)
    some = d > 1e-6
    hr = some & (mx == r)
    hg = some & ~hr & (mx == g)
    hb = some & ~hr & ~hg
    h[hr] = ((g - b)[hr] / d[hr]) % 6
    h[hg] = (b - r)[hg] / d[hg] + 2
    h[hb] = (r - g)[hb] / d[hb] + 4
    return h * 60.0, np.where(mx > 0, d / np.maximum(mx, 1e-6), 0.0), mx


def background(rgb: np.ndarray, key: dict) -> np.ndarray:
    """The pixels of the key's colour class connected to the image's edge."""
    h, s, v = hsv(rgb)
    lo, hi = key["h"]
    hue = ((h >= lo) & (h <= hi)) if lo <= hi else ((h >= lo) | (h <= hi))
    cls = hue & (s >= key["s"][0]) & (s <= key["s"][1]) & (v >= key["v"][0]) & (v <= key["v"][1])
    rows, cols = cls.shape
    bg = np.zeros_like(cls)
    todo = deque([(y, x) for y in range(rows) for x in (0, cols - 1)] +
                 [(y, x) for x in range(cols) for y in (0, rows - 1)])
    while todo:
        y, x = todo.popleft()
        if bg[y, x] or not cls[y, x]:
            continue
        bg[y, x] = True
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ny, nx = y + dy, x + dx
            if 0 <= ny < rows and 0 <= nx < cols and cls[ny, nx] and not bg[ny, nx]:
                todo.append((ny, nx))
    return bg


def make(name: str) -> np.ndarray:
    """The keyed, cropped and graded portrait (RGBA, 0-255)."""
    from PIL import Image
    if name not in ART:
        raise ArtError(f"no portrait recipe for {name!r}")
    src = REPO / "builds" / "p6-art" / name / "owner_art" / "portrait_source.png"
    if not src.exists():
        raise ArtError(f"{src}: missing (the owner's art, local)")
    art = ART[name]
    body = trim_frame(np.asarray(Image.open(src).convert("RGB")))
    height = int(round(body.shape[0] * WORK / body.shape[1]))
    rgb = np.asarray(Image.fromarray(body).resize((WORK, height), Image.BOX)).astype(float)
    alpha = (~background(rgb, art["key"])).astype(float)
    x, y, side = art["crop"]
    if x + side > WORK or y + side > height:
        raise ArtError(f"{name}: crop {art['crop']} leaves the {WORK}x{height} image")
    rgb, alpha = rgb[y:y + side, x:x + side] / 255.0, alpha[y:y + side, x:x + side]
    rgb = np.clip(rgb, 0.0, 1.0) ** art["gamma"] * art["gain"]
    grey = rgb.mean(-1, keepdims=True)
    rgb = np.clip(grey + (rgb - grey) * art["sat"], 0.0, 1.0)
    return np.dstack([rgb * 255.0, alpha * 255.0]).round().astype(np.uint8)


def main() -> int:
    from PIL import Image
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--name", required=True)
    ap.add_argument("--preview", type=Path, help="the select's portrait and flash, enlarged")
    args = ap.parse_args()
    out = REPO / "builds" / "p6-art" / args.name / "portrait.png"
    Image.fromarray(make(args.name), "RGBA").save(out)
    print(f"portrait art: {args.name} -> {out}")
    if args.preview:
        import sys
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        import p6_present
        pic, flash = p6_present.portrait_pair(out)
        both = np.concatenate([pic, flash], axis=1)[..., :3].astype(np.uint8)
        Image.fromarray(both).resize((both.shape[1] * 6, both.shape[0] * 6), Image.NEAREST).save(args.preview)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
