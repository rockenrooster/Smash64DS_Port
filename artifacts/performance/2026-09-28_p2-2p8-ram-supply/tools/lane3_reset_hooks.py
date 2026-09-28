#!/usr/bin/env python3
"""lane3_reset_hooks.py -- data kept alive only by a few small (reset/forget) functions
besides the old-path unit.

Usage: lane3_reset_hooks.py <graph.pkl> <src.csv> [--cut REGEX] [--min N] [--small N]
Candidates: data/bss/rodata symbols NOT in the cut-set, whose referrers are all either
in the cut-set or small functions (<= --small bytes, default 260) -- typical 'invalidate /
forget / reset' hooks that keep dead state linked.  Prints size, the small referrers
and their own referrers so a human can confirm they only clear/forget.
"""
import csv
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402


def main():
    a = sys.argv
    L = rg.Loaded(a[1])
    cut = re.compile(a[a.index('--cut') + 1] if '--cut' in a else r'^ndsFighterMarioFoxDLAllDrawForSlot')
    lo = int(a[a.index('--min') + 1]) if '--min' in a else 200
    small = int(a[a.index('--small') + 1]) if '--small' in a else 260
    src = {}
    with open(a[2], newline='') as f:
        for r in csv.DictReader(f):
            src[(r['name'], int(r['addr']))] = (r['srcfile'], r['line'])
    full = L.reach(['main', 'crt0Startup'])
    part = L.reach(['main', 'crt0Startup'], cut=cut)
    old = full - part
    rows = []
    for i, s in enumerate(L.syms):
        if s['kind'] not in ('bss', 'data', 'rodata') or s['size'] < lo:
            continue
        if i in old or i not in full:
            continue
        refs = L.inn.get(i, set())
        if not refs:
            continue
        live = [c for c in refs if c not in old]
        if not live:
            continue
        if all(L.syms[c]['size'] <= small for c in live):
            rows.append((s['size'], i, live))
    rows.sort(reverse=True)
    for size, i, live in rows:
        s = L.syms[i]
        sf = src.get((s['name'], s['addr']), ('?', 0))
        print('%7d %-56s %-9s %s:%s' % (size, s['name'], s['kind'], sf[0].split('/')[-1], sf[1]))
        for c in sorted(live):
            cs = L.syms[c]
            up = ', '.join('%s(%d)' % (L.syms[x]['name'], L.syms[x]['size']) for x in sorted(L.inn.get(c, ()))[:4])
            print('          via %s (%d B) <- %s' % (cs['name'], cs['size'], up))


if __name__ == '__main__':
    main()
