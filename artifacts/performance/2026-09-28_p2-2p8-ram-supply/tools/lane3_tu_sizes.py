#!/usr/bin/env python3
"""lane3_tu_sizes.py -- per-translation-unit byte totals from a lane3_maptab JSON.

Usage: lane3_tu_sizes.py <map.json> [top_n] [--include-overlay] [--match REGEX]
Prints text/rodata/data/bss per object file (archive members collapsed to the
archive name). Overlay (.ovl.frontend*) rows are skipped unless asked for.
"""
import json
import re
import sys
import collections


def obj_name(o):
    o = o.replace('\\', '/')
    m = re.match(r'^(.*/)?([^/(]+)\(([^)]+)\)$', o)
    if m:
        return m.group(2) + '(' + m.group(3) + ')'
    return o.split('/')[-1]


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    flags = [a for a in sys.argv[1:] if a.startswith('--')]
    rows = json.load(open(args[0]))
    top = int(args[1]) if len(args) > 1 else 60
    inc_ovl = '--include-overlay' in flags
    match = None
    for i, a in enumerate(sys.argv):
        if a == '--match':
            match = re.compile(sys.argv[i + 1])
    tu = collections.defaultdict(collections.Counter)
    for r in rows:
        if not inc_ovl and r['out'].startswith('.ovl.frontend'):
            continue
        o = obj_name(r['obj'])
        if match and not match.search(o):
            continue
        tu[o][r['cls']] += r['size']
    items = sorted(tu.items(), key=lambda kv: -sum(kv[1].values()))
    print('%-52s %9s %9s %9s %9s %9s' % ('TU', 'text', 'rodata', 'data', 'bss', 'total'))
    gt = collections.Counter()
    for k, c in items[:top]:
        print('%-52s %9d %9d %9d %9d %9d' % (k, c['text'], c['rodata'], c['data'], c['bss'], sum(c.values())))
    for k, c in items:
        gt.update(c)
    print('%-52s %9d %9d %9d %9d %9d' % ('ALL', gt['text'], gt['rodata'], gt['data'], gt['bss'], sum(gt.values())))


if __name__ == '__main__':
    main()
