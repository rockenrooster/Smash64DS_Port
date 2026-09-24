"""Slice 6: classify route 0 / route 1 capture differences in the 3D scene.

Usage: capclass6.py ARM0 ARM1 [ARM0 ARM1 ...]
For each pair of captures (captures/ARM-*.png), takes the top screen's scene
rows (window y 56..347: the rows below carry the tick-HUD FPS/UP line, which
differs by route by design) and groups the differing window pixels into
8-connected components. One DS pixel is 1-2 window pixels a side (the guest
view is 400x600 for 256x384), so an edge flip of one DS pixel is a component of
at most 4 window pixels; a content difference is a larger component. Prints,
per frame: differing scene pixels, components, the largest component and its
box, the worst channel delta, and the share of differing pixels that sit on an
edge of the route 0 picture (a neighbour within 2 window pixels differs from it
by more than 24 in some channel): a rounding flip lies on an edge, a changed
texture or colour shows up in flat interiors too.
"""
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAP = os.path.join(HERE, 'captures')
X0, X1, Y0, Y1 = 8, 408, 56, 348


def components(mask):
    seen = np.zeros(mask.shape, dtype=bool)
    comps = []
    ys, xs = np.nonzero(mask)
    for y, x in zip(ys, xs):
        if seen[y, x]:
            continue
        stack = [(y, x)]
        seen[y, x] = True
        pts = []
        while stack:
            cy, cx = stack.pop()
            pts.append((cy, cx))
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    ny, nx = cy + dy, cx + dx
                    if (0 <= ny < mask.shape[0]) and (0 <= nx < mask.shape[1]) and \
                            mask[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        stack.append((ny, nx))
        comps.append(pts)
    return comps


def main():
    args = sys.argv[1:]
    for i in range(0, len(args), 2):
        a_arm, b_arm = args[i], args[i + 1]
        names = sorted(n[len(a_arm) + 1:] for n in os.listdir(CAP)
                       if n.startswith(a_arm + '-') and n.endswith('.png'))
        print('== %s vs %s' % (a_arm, b_arm))
        for name in names:
            pb = os.path.join(CAP, b_arm + '-' + name)
            if not os.path.exists(pb):
                continue
            a = np.asarray(Image.open(os.path.join(CAP, a_arm + '-' + name)).convert('RGB')).astype(int)
            b = np.asarray(Image.open(pb).convert('RGB')).astype(int)
            d = np.abs(a - b).max(axis=2)[Y0:Y1, X0:X1]
            mask = d > 0
            comps = components(mask)
            big = max(comps, key=len) if comps else []
            box = ''
            if big:
                ys = [p[0] + Y0 for p in big]
                xs = [p[1] + X0 for p in big]
                box = 'x %d..%d y %d..%d' % (min(xs), max(xs), min(ys), max(ys))
            sa = a[Y0:Y1, X0:X1]
            grad = np.zeros(mask.shape, dtype=int)
            for dy in (-2, -1, 0, 1, 2):
                for dx in (-2, -1, 0, 1, 2):
                    if dy == 0 and dx == 0:
                        continue
                    shifted = np.roll(np.roll(sa, dy, axis=0), dx, axis=1)
                    grad = np.maximum(grad, np.abs(sa - shifted).max(axis=2))
            on_edge = int((mask & (grad > 24)).sum())
            share = (100.0 * on_edge / mask.sum()) if mask.any() else 100.0
            print('  %-12s scene px %4d  components %3d  largest %3d px %-22s max delta %3d  on edges %5.1f%%' % (
                name, int(mask.sum()), len(comps), len(big), box, int(d.max()) if mask.any() else 0, share))


if __name__ == '__main__':
    main()
