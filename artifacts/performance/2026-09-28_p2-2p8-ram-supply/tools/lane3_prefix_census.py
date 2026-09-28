#!/usr/bin/env python3
"""lane3_prefix_census.py -- byte totals by symbol-name prefix, per section kind.

Usage: lane3_prefix_census.py <nm-sysv.txt> [--min BYTES] [--overlay]
Strips a leading g/s/d/k and 'Nds' style markers, then groups by the first
CamelCase words (up to 3). Overlay symbols are excluded unless --overlay.
Purpose: find the renderer-domain landscape before assigning families.
"""
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_symtab as st  # noqa: E402


def group(name):
    n = re.sub(r'\.(part|constprop|isra|cold)\.\d+$', '', name)
    n = re.sub(r'(\.(constprop|isra|part)\.\d+)+$', '', n)
    m = re.match(r'^([gsdk]?)(Nds)([A-Z][a-z0-9]+)([A-Z][a-z0-9]+)?([A-Z][a-z0-9]+)?', n)
    if m:
        return ('Nds' + (m.group(3) or '') + (m.group(4) or '') + (m.group(5) or ''))
    m = re.match(r'^(nds)([A-Z][a-z0-9]+)([A-Z][a-z0-9]+)?([A-Z][a-z0-9]+)?', n)
    if m:
        return ('nds' + (m.group(2) or '') + (m.group(3) or '') + (m.group(4) or ''))
    m = re.match(r'^([a-z]+)([A-Z][a-z0-9]+)?', n)
    if m:
        return m.group(1) + (m.group(2) or '')
    return n


def main():
    syms = st.load(sys.argv[1])
    lo = 0
    ovl = '--overlay' in sys.argv
    if '--min' in sys.argv:
        lo = int(sys.argv[sys.argv.index('--min') + 1])
    tot = collections.defaultdict(collections.Counter)
    for s in syms:
        if s['size'] <= 0 or s['name'].startswith('$'):
            continue
        if s['sec'].startswith('.ovl') and not ovl:
            continue
        if s['sec'] in ('.debug', ):
            continue
        tot[group(s['name'])][s['kind']] += s['size']
    rows = sorted(tot.items(), key=lambda kv: -sum(kv[1].values()))
    print('%-46s %9s %9s %9s %9s %9s' % ('prefix', 'text', 'rodata', 'data', 'bss', 'total'))
    for k, c in rows:
        t = sum(c.values())
        if t < lo:
            break
        print('%-46s %9d %9d %9d %9d %9d' % (k, c['text'], c['rodata'], c['data'], c['bss'], t))


if __name__ == '__main__':
    main()
