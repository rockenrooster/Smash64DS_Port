#!/usr/bin/env python3
"""lane3_symtab.py -- parse `arm-none-eabi-nm --format=sysv -S` into a symbol table.

Library + CLI.
  CLI:  lane3_symtab.py <nm-sysv.txt> <regex> [--top N] [--sec SECTION]
        prints symbols whose name matches regex, with size / section / class,
        sorted by size, and the byte total per (section, kind).

Symbol kinds (derived, since .main mixes code and rodata):
  text   : FUNC (any section) or anything in a code-only output section
  rodata : OBJECT in .main / .text.* / .ovl.frontend  (r/R/t on an OBJECT)
  data   : OBJECT in .main.rw / .dtcm / .itcm-data
  bss    : OBJECT in .main.bss / .dtcm.bss / .ovl.frontend.bss
"""
import re
import sys
import collections

RE_LINE = re.compile(
    r'^(?P<name>[^|]+)\|(?P<val>[0-9a-f]*)\|\s*(?P<cls>\S)\s*\|\s*(?P<typ>\S*)\|(?P<size>[0-9a-f]*)\|[^|]*\|(?P<sec>.*)$')


def kind_of(typ, sec):
    if sec in ('.main.bss', '.dtcm.bss', '.ovl.frontend.bss', '.bss'):
        return 'bss'
    if sec in ('.main.rw', '.dtcm', '.data', '.ovl.frontend.rw'):
        return 'data'
    if typ == 'FUNC':
        return 'text'
    if typ == 'OBJECT':
        return 'rodata'
    return 'other'


def load(path):
    syms = []
    with open(path, errors='replace') as f:
        for line in f:
            m = RE_LINE.match(line.rstrip('\n'))
            if not m:
                continue
            name = m.group('name').strip()
            val = m.group('val')
            size = m.group('size')
            if not val:
                continue
            sec = m.group('sec').strip()
            typ = m.group('typ').strip()
            syms.append({
                'name': name,
                'addr': int(val, 16),
                'size': int(size, 16) if size else 0,
                'cls': m.group('cls'),
                'typ': typ,
                'sec': sec,
                'kind': kind_of(typ, sec),
            })
    return syms


def main():
    syms = load(sys.argv[1])
    rx = re.compile(sys.argv[2])
    top = 9999
    sec = None
    args = sys.argv[3:]
    for i, a in enumerate(args):
        if a == '--top':
            top = int(args[i + 1])
        if a == '--sec':
            sec = args[i + 1]
    sel = [s for s in syms if rx.search(s['name']) and s['size'] > 0 and (sec is None or s['sec'] == sec)]
    sel.sort(key=lambda s: -s['size'])
    tot = collections.Counter()
    for s in sel:
        tot[(s['sec'], s['kind'])] += s['size']
    for s in sel[:top]:
        print('%-64s %8d %-12s %-6s %s' % (s['name'], s['size'], s['sec'], s['kind'], s['typ']))
    print('--- totals')
    for k in sorted(tot):
        print('%-14s %-7s %9d' % (k[0], k[1], tot[k]))
    print('all', sum(tot.values()), 'n=', len(sel))


if __name__ == '__main__':
    main()
