#!/usr/bin/env python3
"""lane3_callees.py -- list what a symbol references (out-edges) with sizes.

Usage: lane3_callees.py <graph.pkl> <symbol-regex> [--kind call|word|data] [--min N]
For each matching symbol, prints its out-edges (targets) sorted by target size, with
the edge kind, and marks targets that have NO other referrer than this symbol
(sole referrer) -- those die with it.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402


def main():
    L = rg.Loaded(sys.argv[1])
    rx = re.compile(sys.argv[2])
    kind = None
    lo = 0
    if '--kind' in sys.argv:
        kind = sys.argv[sys.argv.index('--kind') + 1]
    if '--min' in sys.argv:
        lo = int(sys.argv[sys.argv.index('--min') + 1])
    for i, s in enumerate(L.syms):
        if not rx.search(s['name']):
            continue
        print('== %s (%d B, %s)' % (s['name'], s['size'], s['sec']))
        rows = []
        for t in L.out.get(i, ()):
            ks = L.kinds.get((i, t), [])
            if kind and kind not in ks:
                continue
            ts = L.syms[t]
            if ts['size'] < lo:
                continue
            sole = (len(L.inn.get(t, ())) == 1)
            rows.append((ts['size'], ts['name'], ts['sec'], ','.join(ks), sole, len(L.inn.get(t, ()))))
        rows.sort(reverse=True)
        for r in rows:
            print('   %8d %-64s %-10s %-10s %s (referrers=%d)' % (r[0], r[1], r[2], r[3], 'SOLE' if r[4] else '', r[5]))


if __name__ == '__main__':
    main()
