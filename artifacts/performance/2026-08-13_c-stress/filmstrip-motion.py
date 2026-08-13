#!/usr/bin/env python3
"""Per-frame motion of a soak filmstrip, top screen and bottom screen apart.

WHY THIS EXISTS. `soak-freeze-watch.ps1` hashes the WHOLE client area, so any
pixel that changes keeps the verdict at NO-FREEZE -- and the tick HUD on the
bottom screen changes its digits every presented frame by construction. A guest
whose gameplay has stopped while presentation continues therefore hashes as
"moving" forever. Splitting the two screens is what separates "the game is
running" from "the instrument is running".

Reads the PNGs written by -SaveFramesTo (window captures, chrome included) and
reports, per consecutive pair, the fraction of pixels that differ by more than
a channel threshold. Geometry is the canonical 416x664 profile: content origin
(8, 56), each screen 400x300 (256x192 at the enforced 1.5625 scale).
"""
import sys
import pathlib
import numpy as np
from PIL import Image

TOP = (8, 56, 408, 356)
BOTTOM = (8, 356, 408, 656)
THRESHOLD = 12


def changed_fraction(a, b):
    delta = np.abs(a.astype(np.int16) - b.astype(np.int16)).max(axis=2)
    return float((delta > THRESHOLD).mean())


def main(directory):
    frames = sorted(pathlib.Path(directory).glob("f*.png"))
    if len(frames) < 2:
        raise SystemExit(f"need at least two frames in {directory}")
    print("elapsed,name,topChanged,bottomChanged,topColors")
    previous = None
    for path in frames:
        image = Image.open(path).convert("RGB")
        top = np.asarray(image.crop(TOP))
        bottom = np.asarray(image.crop(BOTTOM))
        # Distinct colours in the game picture, as a corruption floor: a frame
        # that lost its textures, its palette or its geometry collapses toward a
        # handful of colours, and that is checkable over a whole filmstrip
        # without anyone looking at every frame. A live Dream Land frame is in
        # the thousands; a Results screen is lower but never single digits.
        colors = len(np.unique(top.reshape(-1, 3), axis=0))
        if previous is not None:
            elapsed = int(path.name[1:6])
            print("%d,%s,%.6f,%.6f,%d" % (
                elapsed, path.name,
                changed_fraction(top, previous[0]),
                changed_fraction(bottom, previous[1]),
                colors))
        previous = (top, bottom)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else ".")
