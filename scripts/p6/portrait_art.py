#!/usr/bin/env python3
"""P6 A9: an authored content's select portrait from the owner's art.

The owner supplied a pixel portrait for each P6 fighter (2026-10-14:
"backgrounds will have to be changed to match the portrait style of the
existing roster"; local, builds/p6-art/<name>/owner_art/portrait_source.png,
like the kit): a framed square of about 32 pixels, drawn large on black, its
backdrop dark. This trims the art's frame and the black gap inside it, keys
the backdrop out (the dark pixels that are not the fighter's hair colour,
connected to the art's edge) and writes the transparent
`builds/p6-art/<name>/portrait.png` that p6_present.py frames over the
select's fire plate, as the roster's portraits are.

    portrait_art.py --name palutena [--preview out.png]

Owner-derived: the outputs stay local (D10); the recipe is committed.
"""
from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
# The art's frame line (brown) and the black gap inside it.
FRAME_RGB = (85, 70, 55)
FRAME_TOLERANCE = 30
GAP_MAX = 12
# The working size the art is area-averaged to before keying.
WORK = 112
# Per content: the backdrop is the pixels darker than `dark` (value, 0-1)
# that are not the fighter's own hue (hue range in degrees, which may wrap
# past 360, at saturations above `keep_s`: the hair's shadows stay).
ART = {
    # A dark teal backdrop behind green hair and a gold crown.
    "palutena": {"dark": 0.26, "keep_h": (70, 165), "keep_s": 0.35},
    # A dark red backdrop behind blue hair and his red cape.
    "ike": {"dark": 0.24, "keep_h": (195, 265), "keep_s": 0.30},
}


class ArtError(SystemExit):
    pass


def art_box(img: np.ndarray) -> np.ndarray:
    """The art inside its frame: the outer box of everything lit, then the
    frame's rows and columns and the black gap trimmed from each side, the
    result's centre square."""
    lit = img.max(-1) > 60
    rows = np.nonzero(lit.mean(1) > 0.3)[0]
    cols = np.nonzero(lit.mean(0) > 0.3)[0]
    if len(rows) == 0 or len(cols) == 0:
        raise ArtError("no framed art found")
    box = img[rows[0]:rows[-1] + 1, cols[0]:cols[-1] + 1]

    def edge(lines: np.ndarray) -> int:
        k = 0
        while k < len(lines) // 4:
            line = lines[k].astype(int)
            gap = (line.max(-1) < GAP_MAX).mean() > 0.6
            frame = (np.abs(line - np.array(FRAME_RGB)).max(-1) < FRAME_TOLERANCE).mean() > 0.6
            if not (gap or frame):
                break
            k += 1
        return k

    top, bottom = edge(box), edge(box[::-1])
    left, right = edge(box.transpose(1, 0, 2)), edge(box.transpose(1, 0, 2)[::-1])
    art = box[top:box.shape[0] - bottom, left:box.shape[1] - right]
    side = min(art.shape[:2])
    y0, x0 = (art.shape[0] - side) // 2, (art.shape[1] - side) // 2
    return art[y0:y0 + side, x0:x0 + side]


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


def backdrop(rgb: np.ndarray, art: dict) -> np.ndarray:
    """The backdrop's pixels: dark, not the fighter's hue, connected to the
    edge."""
    h, s, v = hsv(rgb)
    lo, hi = art["keep_h"]
    hue = ((h >= lo) & (h <= hi)) if lo <= hi else ((h >= lo) | (h <= hi))
    cls = (v < art["dark"]) & ~(hue & (s > art["keep_s"]))
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
    """The keyed portrait (RGBA, 0-255, WORK square)."""
    from PIL import Image
    if name not in ART:
        raise ArtError(f"no portrait recipe for {name!r}")
    src = REPO / "builds" / "p6-art" / name / "owner_art" / "portrait_source.png"
    if not src.exists():
        raise ArtError(f"{src}: missing (the owner's art, local)")
    art = art_box(np.asarray(Image.open(src).convert("RGB")))
    rgb = np.asarray(Image.fromarray(art).resize((WORK, WORK), Image.BOX)).astype(float)
    alpha = np.where(backdrop(rgb, ART[name]), 0.0, 255.0)
    return np.dstack([rgb, alpha]).round().astype(np.uint8)


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
