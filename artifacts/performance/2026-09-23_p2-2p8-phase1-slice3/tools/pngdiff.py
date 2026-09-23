"""Pixel diff of two melonDS window captures (slice 3 Results comparison).

Usage: pngdiff.py A.png B.png [OUT_DIFF.png]
Prints the size, the count of differing pixels and their bounding box, and
writes a diff mask (white = differs) when OUT_DIFF.png is given.
"""
import sys

import numpy as np
from PIL import Image


def main():
    a = np.asarray(Image.open(sys.argv[1]).convert('RGB')).astype(np.int16)
    b = np.asarray(Image.open(sys.argv[2]).convert('RGB')).astype(np.int16)
    print('sizes', a.shape, b.shape)
    if a.shape != b.shape:
        print('DIFFERENT SIZES')
        return 1
    d = np.abs(a - b).max(axis=2)
    n = int((d > 0).sum())
    print('pixels', a.shape[0] * a.shape[1], 'differing', n, 'max channel delta', int(d.max()))
    if n:
        ys, xs = np.nonzero(d)
        print('bbox x %d..%d y %d..%d' % (xs.min(), xs.max(), ys.min(), ys.max()))
    if len(sys.argv) > 3:
        Image.fromarray(((d > 0) * 255).astype(np.uint8)).save(sys.argv[3])
    print('IDENTICAL' if n == 0 else 'DIFFER')
    return 0


if __name__ == '__main__':
    sys.exit(main())
