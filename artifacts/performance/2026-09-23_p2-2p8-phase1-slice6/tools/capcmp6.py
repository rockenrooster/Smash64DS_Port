"""Slice 6: route 0 / route 1 capture pairs (capture-s6.ps1 output).

Usage: capcmp6.py ARM0 ARM1 [--diff]
Pairs ARM0-*.png with ARM1-*.png of the same stop name in captures/, crops the
guest viewport of the canonical 416x664 melonDS window (x 8..407, y 56..655:
the title bar carries a live FPS readout and the frame is desktop pixels) and
reports, per pair, the differing viewport pixels, the largest channel delta
and the bounding box, for the top screen (the 3D battle view, y 56..355) and
the whole viewport. --diff writes a white-on-black mask per differing pair.
"""
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAP = os.path.join(HERE, 'captures')
X0, X1, Y0, Y1, YTOP = 8, 408, 56, 656, 356


def load(path):
    return np.asarray(Image.open(path).convert('RGB')).astype(np.int16)


def main():
    a_arm, b_arm = sys.argv[1], sys.argv[2]
    write_diff = '--diff' in sys.argv
    names = sorted(n[len(a_arm) + 1:] for n in os.listdir(CAP)
                   if n.startswith(a_arm + '-') and n.endswith('.png'))
    total = 0
    for name in names:
        pa = os.path.join(CAP, a_arm + '-' + name)
        pb = os.path.join(CAP, b_arm + '-' + name)
        if not os.path.exists(pb):
            print('%-14s missing in %s' % (name, b_arm))
            continue
        a, b = load(pa), load(pb)
        if a.shape != b.shape:
            print('%-14s size differs %s %s' % (name, a.shape, b.shape))
            continue
        d = np.abs(a - b).max(axis=2)
        view = d[Y0:Y1, X0:X1]
        top = d[Y0:YTOP, X0:X1]
        n_view = int((view > 0).sum())
        n_top = int((top > 0).sum())
        total += n_view
        bbox = ''
        if n_view:
            ys, xs = np.nonzero(view)
            bbox = 'bbox x %d..%d y %d..%d' % (xs.min() + X0, xs.max() + X0, ys.min() + Y0, ys.max() + Y0)
            if write_diff:
                Image.fromarray(((d > 0) * 255).astype(np.uint8)).save(
                    os.path.join(CAP, 'diff-%s-%s-%s' % (a_arm, b_arm, name)))
        print('%-14s top screen %6d px, viewport %6d px of %d, max delta %3d %s' % (
            name, n_top, n_view, view.size, int(view.max()), bbox))
    print('TOTAL differing viewport pixels:', total, '(IDENTICAL)' if total == 0 else '')


if __name__ == '__main__':
    main()
