#!/usr/bin/env python3
"""lane3_scan_counters.py -- scan recorded tick-HUD run JSONs for counter values.

Usage: lane3_scan_counters.py <artifacts/performance> <since-YYYY-MM-DD> <regex> [--files]

Walks every *.json under the given root whose top-level folder name starts with
a date >= since and that carries an "extras" list of {name,value}. For every
extras name matching the regex it prints: name, #files seen, #files nonzero,
max value, and one file with the max value.  With --files it also lists every
nonzero file (first 8).
Also prints how many run files were scanned and the distinct 'target' values.
"""
import json
import os
import re
import sys
import collections


def main():
    root, since, rx = sys.argv[1], sys.argv[2], re.compile(sys.argv[3])
    show_files = '--files' in sys.argv
    seen = collections.Counter()
    nonzero = collections.defaultdict(list)
    maxv = {}
    maxf = {}
    nfiles = 0
    targets = collections.Counter()
    gits = collections.Counter()
    for top in sorted(os.listdir(root)):
        if not re.match(r'^\d{4}-\d{2}-\d{2}', top):
            continue
        if top[:10] < since:
            continue
        base = os.path.join(root, top)
        if not os.path.isdir(base):
            continue
        for dp, dn, fn in os.walk(base):
            for f in fn:
                if not f.endswith('.json'):
                    continue
                p = os.path.join(dp, f)
                try:
                    d = json.load(open(p, encoding='utf-8', errors='replace'))
                except Exception:
                    continue
                if not isinstance(d, dict):
                    continue
                ex = d.get('extras')
                if not isinstance(ex, list):
                    continue
                nfiles += 1
                targets[str(d.get('target'))] += 1
                gits[str(d.get('gitShort'))] += 1
                for e in ex:
                    if not isinstance(e, dict):
                        continue
                    n = e.get('name')
                    if n is None or not rx.search(n):
                        continue
                    v = e.get('value')
                    if not isinstance(v, (int, float)):
                        continue
                    seen[n] += 1
                    if v != 0:
                        nonzero[n].append(os.path.relpath(p, root))
                    if n not in maxv or v > maxv[n]:
                        maxv[n] = v
                        maxf[n] = os.path.relpath(p, root)
    print('run files scanned:', nfiles)
    print('targets:', dict(targets.most_common(12)))
    print('%-46s %6s %8s %14s  %s' % ('counter', 'files', 'nonzero', 'max', 'file@max'))
    for n in sorted(seen):
        print('%-46s %6d %8d %14s  %s' % (n, seen[n], len(nonzero[n]), maxv[n], maxf[n] if nonzero[n] else ''))
        if show_files and nonzero[n]:
            for pth in nonzero[n][:8]:
                print('      ', pth)


if __name__ == '__main__':
    main()
