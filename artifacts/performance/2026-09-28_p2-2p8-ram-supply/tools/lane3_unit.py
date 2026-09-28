#!/usr/bin/env python3
"""lane3_unit.py -- evaluate a deletion unit: the set only kept alive by a set of entry symbols.

Usage:
  lane3_unit.py <graph.pkl> <src.csv> --cut REGEX [--also-cut REGEX] [--roots a,b]
                [--cut-edge 'SRC_RE>DST_RE' ...]   (ignore reset-hook edges src->dst)
                [--csv out.csv] [--by-file] [--list N] [--exclude-unit REGEX]

Computes only_via = reach(roots) - reach(roots minus symbols matching (cut|also-cut)).
Prints bytes by (section, kind) and, with --by-file, by source file.  --csv writes every
member symbol (name, size, section, kind, srcfile:line).

--exclude-unit REGEX : additionally remove from the printed/CSV set any symbol whose name
matches (used to subtract sub-units already accounted elsewhere).
"""
import collections
import csv
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402


def load_src(csvp):
    d = {}
    with open(csvp, newline='') as f:
        for r in csv.DictReader(f):
            d[(r['name'], int(r['addr']))] = (r['srcfile'], r['line'])
    return d


def main():
    a = sys.argv
    pkl, csvp = a[1], a[2]
    cut = re.compile(a[a.index('--cut') + 1])
    if '--also-cut' in a:
        cut = re.compile('(' + cut.pattern + ')|(' + a[a.index('--also-cut') + 1] + ')')
    roots = ['main', 'crt0Startup']
    if '--roots' in a:
        roots = a[a.index('--roots') + 1].split(',')
    L = rg.Loaded(pkl)
    src = load_src(csvp)
    cut_edges = []
    i2 = 0
    while '--cut-edge' in a[i2:]:
        j = a.index('--cut-edge', i2)
        spec = a[j + 1]
        src_re, dst_re = spec.split('>', 1)
        cut_edges.append((re.compile(src_re), re.compile(dst_re)))
        i2 = j + 2
    full = L.reach(roots)
    part = L.reach(roots, cut=cut, cut_edges=cut_edges)
    diff = full - part
    excl = re.compile(a[a.index('--exclude-unit') + 1]) if '--exclude-unit' in a else None
    members = []
    for i in diff:
        s = L.syms[i]
        if excl and excl.search(s['name']):
            continue
        members.append(i)
    tot = collections.Counter()
    byfile = collections.defaultdict(collections.Counter)
    for i in members:
        s = L.syms[i]
        tot[(s['sec'], s['kind'])] += s['size']
        f = src.get((s['name'], s['addr']), ('?', 0))[0]
        byfile[f][s['kind']] += s['size']
    print('members %d' % len(members))
    for k in sorted(tot):
        print('  %-24s %-7s %9d' % (k[0], k[1], tot[k]))
    print('  TOTAL %d' % sum(tot.values()))
    if '--by-file' in a:
        rows = sorted(byfile.items(), key=lambda kv: -sum(kv[1].values()))
        for f, c in rows:
            print('  %-58s text %7d ro %7d data %6d bss %7d = %7d' % (
                f, c['text'], c['rodata'], c['data'], c['bss'], sum(c.values())))
    if '--csv' in a:
        with open(a[a.index('--csv') + 1], 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['name', 'size', 'section', 'kind', 'srcfile', 'line'])
            for i in sorted(members, key=lambda i: -L.syms[i]['size']):
                s = L.syms[i]
                sf = src.get((s['name'], s['addr']), ('?', 0))
                w.writerow([s['name'], s['size'], s['sec'], s['kind'], sf[0], sf[1]])
    if '--list' in a:
        n = int(a[a.index('--list') + 1])
        for i in sorted(members, key=lambda i: -L.syms[i]['size'])[:n]:
            s = L.syms[i]
            sf = src.get((s['name'], s['addr']), ('?', 0))
            print('%8d %-64s %-10s %-6s %s:%s' % (s['size'], s['name'], s['sec'], s['kind'], sf[0].split('/')[-1], sf[1]))


if __name__ == '__main__':
    main()
