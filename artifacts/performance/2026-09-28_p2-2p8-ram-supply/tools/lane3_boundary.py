#!/usr/bin/env python3
"""lane3_boundary.py -- the symbols an old-path unit calls that must STAY (shared).

Usage: lane3_boundary.py <graph.pkl> <src.csv> --cut REGEX [--also-cut REGEX] [--cut-edge 'SRC>DST' ...] [--min N] [--csv out.csv]
Computes the unit U = reach(main) - reach(main minus cut).  Boundary B = symbols outside U
referenced by some member of U.  Reports B sorted by size, with the U members that use them
and the surviving referrers (who keeps it alive).  Deleting U must keep B.
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
    cut = re.compile(a[a.index('--cut') + 1])
    if '--also-cut' in a:
        cut = re.compile('(' + cut.pattern + ')|(' + a[a.index('--also-cut') + 1] + ')')
    lo = int(a[a.index('--min') + 1]) if '--min' in a else 150
    src = {}
    with open(a[2], newline='') as f:
        for r in csv.DictReader(f):
            src[(r['name'], int(r['addr']))] = (r['srcfile'], r['line'])
    cut_edges = []
    i2 = 0
    while '--cut-edge' in a[i2:]:
        j = a.index('--cut-edge', i2)
        sr, dr = a[j + 1].split('>', 1)
        cut_edges.append((re.compile(sr), re.compile(dr)))
        i2 = j + 2
    full = L.reach(['main', 'crt0Startup'])
    part = L.reach(['main', 'crt0Startup'], cut=cut, cut_edges=cut_edges)
    U = full - part
    B = {}
    for u in U:
        for v in L.out.get(u, ()):
            if v not in U:
                B.setdefault(v, []).append(u)
    rows = sorted(B.items(), key=lambda kv: -L.syms[kv[0]]['size'])
    out = []
    for v, us in rows:
        s = L.syms[v]
        if s['size'] < lo:
            continue
        keep = [L.syms[c]['name'] for c in L.inn.get(v, ()) if c not in U]
        sf = src.get((s['name'], s['addr']), ('?', 0))
        out.append((s['size'], s['name'], s['sec'], s['kind'], sf[0].split('/')[-1], sf[1],
                    ';'.join(L.syms[u]['name'] for u in us[:3]), ';'.join(keep[:3])))
    for o in out:
        print('%7d %-56s %-10s %-6s %s:%s  usedBy[%s] keptBy[%s]' % o)
    print('boundary symbols >= %d B: %d, total %d B' % (lo, len(out), sum(o[0] for o in out)))
    if '--csv' in a:
        with open(a[a.index('--csv') + 1], 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['size', 'name', 'section', 'kind', 'srcfile', 'line', 'used_by_unit', 'kept_by'])
            for o in out:
                w.writerow(o)


if __name__ == '__main__':
    main()
