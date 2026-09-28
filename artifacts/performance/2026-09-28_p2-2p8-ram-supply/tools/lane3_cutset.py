#!/usr/bin/env python3
"""lane3_cutset.py -- what becomes unreachable when a set of symbols is cut.

Usage: lane3_cutset.py <graph.pkl> <cut-regex> [--roots a,b,c] [--list N] [--sum-only]
                       [--extra-cut regex2]

Computes reach(roots) and reach(roots with every symbol matching cut-regex
removed); the difference is the set that ONLY the cut symbols keep alive.
Prints bytes per (section, kind) and lists the largest members.
Roots default: main,crt0Startup (plus everything address-taken from them).

The result is a static UPPER BOUND on what a deletion frees: it ignores gc-sections
granularity and counts a shared helper as freed only if nothing else references it.
"""
import collections
import re
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402


def main():
    pkl = sys.argv[1]
    cut = re.compile(sys.argv[2])
    roots = ['main', 'crt0Startup']
    lst = 40
    if '--roots' in sys.argv:
        roots = sys.argv[sys.argv.index('--roots') + 1].split(',')
    if '--list' in sys.argv:
        lst = int(sys.argv[sys.argv.index('--list') + 1])
    L = rg.Loaded(pkl)
    full = L.reach(roots)
    if '--extra-cut' in sys.argv:
        cut2 = re.compile(sys.argv[sys.argv.index('--extra-cut') + 1])
        cut = re.compile('(' + cut.pattern + ')|(' + cut2.pattern + ')')
    part = L.reach(roots, cut=cut)
    # the cut symbols themselves are excluded from `part`, so they belong to the diff
    diff = full - part
    tot = collections.Counter()
    for i in diff:
        s = L.syms[i]
        tot[(s['sec'], s['kind'])] += s['size']
    print('reach(full)=%d  reach(cut)=%d  only-via-cut=%d' % (len(full), len(part), len(diff)))
    for k in sorted(tot):
        print('  %-24s %-7s %9d' % (k[0], k[1], tot[k]))
    print('  TOTAL', sum(tot.values()))
    if '--sum-only' in sys.argv:
        return
    items = sorted(diff, key=lambda i: -L.syms[i]['size'])
    for i in items[:lst]:
        s = L.syms[i]
        print('%-70s %8d %-10s %s' % (s['name'], s['size'], s['sec'], s['kind']))


if __name__ == '__main__':
    main()
