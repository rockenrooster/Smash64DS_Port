#!/usr/bin/env python3
"""lane3_unit_summary.py -- per-source-file and per-section totals of a lane3_members_*.csv.

Usage: lane3_unit_summary.py <members.csv> [--top N]
"""
import collections
import csv
import os
import sys


def main():
    path = sys.argv[1]
    top = int(sys.argv[sys.argv.index('--top') + 1]) if '--top' in sys.argv else 30
    by = collections.defaultdict(collections.Counter)
    tot = collections.Counter()
    for r in csv.DictReader(open(path, newline='')):
        by[r['srcfile']][r['kind']] += int(r['size'])
        tot[r['kind']] += int(r['size'])
    print('%s: text %d rodata %d data %d bss %d = %d' % (
        os.path.basename(path), tot['text'], tot['rodata'], tot['data'], tot['bss'], sum(tot.values())))
    for f, c in sorted(by.items(), key=lambda kv: -sum(kv[1].values()))[:top]:
        print('  %-66s text %6d ro %6d data %5d bss %6d = %6d' % (
            f, c['text'], c['rodata'], c['data'], c['bss'], sum(c.values())))


if __name__ == '__main__':
    main()
