#!/usr/bin/env python3
"""lane3_fn_referrers.py -- functions of selected source files with their referrers.

Usage: lane3_fn_referrers.py <graph.pkl> <src.csv> <srcfile-regex> [--min N] [--kind text|bss|rodata|data]
For each symbol (default: text) of the matching source files, prints size, name,
file:line, and up to 4 referrers (name, size).  Flags: [ONLY-OLD] if every referrer
chain leads only into the old fighter entry (uses the cut set of the DrawForSlot entry).
"""
import csv
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402


def main():
    L = rg.Loaded(sys.argv[1])
    csvp = sys.argv[2]
    rx = re.compile(sys.argv[3])
    lo = 400
    kind = 'text'
    if '--min' in sys.argv:
        lo = int(sys.argv[sys.argv.index('--min') + 1])
    if '--kind' in sys.argv:
        kind = sys.argv[sys.argv.index('--kind') + 1]
    full = L.reach(['main', 'crt0Startup'])
    part = L.reach(['main', 'crt0Startup'], cut=re.compile(r'^ndsFighterMarioFoxDLAllDrawForSlot'))
    only_old = full - part
    bykey = {}
    for i, s in enumerate(L.syms):
        bykey[(s['name'], s['addr'])] = i
    rows = []
    with open(csvp, newline='') as f:
        for r in csv.DictReader(f):
            if not rx.search(r['srcfile']) or r['kind'] != kind or int(r['size']) < lo:
                continue
            if r['section'].startswith('.ovl'):
                continue
            rows.append(r)
    rows.sort(key=lambda r: -int(r['size']))
    for r in rows:
        i = bykey.get((r['name'], int(r['addr'])))
        refs = []
        if i is not None:
            for c in sorted(L.inn.get(i, ())):
                cs = L.syms[c]
                refs.append('%s(%d)' % (cs['name'], cs['size']))
        flag = 'ONLY-OLD' if (i is not None and i in only_old) else ''
        print('%7d %-60s %s:%s %-8s <- %s' % (int(r['size']), r['name'], r['srcfile'].split('/')[-1], r['line'], flag, ', '.join(refs[:4]) + (' +%d' % (len(refs) - 4) if len(refs) > 4 else '')))


if __name__ == '__main__':
    main()
