#!/usr/bin/env python3
"""lane3_filesyms.py -- list symbols of one source file from a lane3_srcmap CSV.

Usage: lane3_filesyms.py <src.csv> <srcfile-regex> [--kind bss|text|rodata|data] [--min N] [--top N]
"""
import csv
import re
import sys
import collections


def main():
    csvp, rx = sys.argv[1], re.compile(sys.argv[2])
    kind = None
    lo = 0
    top = 400
    a = sys.argv
    if '--kind' in a:
        kind = a[a.index('--kind') + 1]
    if '--min' in a:
        lo = int(a[a.index('--min') + 1])
    if '--top' in a:
        top = int(a[a.index('--top') + 1])
    rows = []
    with open(csvp, newline='') as f:
        for r in csv.DictReader(f):
            if not rx.search(r['srcfile']):
                continue
            if kind and r['kind'] != kind:
                continue
            if int(r['size']) < lo:
                continue
            rows.append(r)
    rows.sort(key=lambda r: -int(r['size']))
    tot = collections.Counter()
    for r in rows:
        tot[r['kind']] += int(r['size'])
    for r in rows[:top]:
        print('%-70s %8d %-10s %-6s %s:%s' % (r['name'], int(r['size']), r['section'], r['kind'], r['srcfile'].split('/')[-1], r['line']))
    print('--- totals', dict(tot), 'n=', len(rows))


if __name__ == '__main__':
    main()
