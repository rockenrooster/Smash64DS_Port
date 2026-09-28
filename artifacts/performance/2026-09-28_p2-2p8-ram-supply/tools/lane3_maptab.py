#!/usr/bin/env python3
"""lane3_maptab.py -- parse a GNU ld linker map into per-input-section rows.

Usage: lane3_maptab.py <map> <out.json>

Each row: {out, in, sym, addr, size, obj, cls}
  out  : output section (.itcm, .main, .main.rw, .main.bss, .ovl.frontend ...)
  in   : input section name (.text.foo, .rodata.str1.4, .bss.gFoo ...)
  sym  : symbol derived from the input-section suffix ('' if merged/anonymous)
  cls  : text | rodata | data | bss  (from the input-section prefix)
Only the output sections that occupy target memory are kept.
"""
import json
import re
import sys

KEEP_OUT = {
    '.itcm', '.dtcm', '.dtcm.bss', '.text.hot', '.text.hot.draw',
    '.text.frontend_resident', '.main', '.main.rw', '.main.bss',
    '.ovl.frontend', '.ovl.frontend.bss', '.crt0', '.eh_frame', '.init_array',
    '.fini_array', '.tinfo',
}

RE_OUT = re.compile(r'^(\.[A-Za-z0-9_.]+)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)')
RE_OUT_NAME_ONLY = re.compile(r'^(\.[A-Za-z0-9_.]+)\s*$')
RE_IN_INLINE = re.compile(r'^ (\S+)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(.+?)\s*$')
RE_IN_NAME = re.compile(r'^ (\S+)\s*$')
RE_IN_CONT = re.compile(r'^\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(.+?)\s*$')


def classify(insec):
    n = insec
    if n.startswith('.text') or n == '.itcm' or n.startswith('.itcm') \
            or n.startswith('.init') or n.startswith('.fini') \
            or n.startswith('.crt0'):
        return 'text'
    if n.startswith('.rodata') or n.startswith('.ARM.ex') or n.startswith('.eh_frame'):
        return 'rodata'
    if n.startswith('.bss') or n.startswith('.sbss') or n.startswith('COMMON') or n.startswith('.dtcm.bss'):
        return 'bss'
    if n.startswith('.data') or n.startswith('.sdata') or n.startswith('.dtcm') or n.startswith('.tdata'):
        return 'data'
    return 'other'


def sym_of(insec):
    m = re.match(r'^\.(?:text|rodata|data|bss|sbss|sdata)\.(.+)$', insec)
    if not m:
        return ''
    s = m.group(1)
    # strip gcc mergeable/anonymous suffixes
    if s.startswith('str1.') or s.startswith('cst') or s.startswith('str'):
        return ''
    s = re.sub(r'\.(part|constprop|isra|cold|lto_priv)\.\d+$', '', s)
    return s


def main():
    src, dst = sys.argv[1], sys.argv[2]
    rows = []
    out = None
    in_map = False
    pending = None
    with open(src, 'r', errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            if not in_map:
                if line.startswith('Linker script and memory map'):
                    in_map = True
                continue
            if line and not line[0].isspace():
                m = RE_OUT.match(line)
                if m:
                    out = m.group(1)
                    pending = None
                    continue
                m = RE_OUT_NAME_ONLY.match(line)
                if m:
                    out = m.group(1)
                    pending = None
                    continue
                # other column-0 lines (e.g. PROVIDE) -- keep out
                continue
            if out not in KEEP_OUT:
                continue
            m = RE_IN_INLINE.match(line)
            if m and m.group(1) not in ('*fill*',) and not m.group(1).startswith('*'):
                name, addr, size, obj = m.groups()
                rows.append({'out': out, 'in': name, 'sym': sym_of(name),
                             'addr': int(addr, 16), 'size': int(size, 16),
                             'obj': obj, 'cls': classify(name)})
                pending = None
                continue
            m = RE_IN_NAME.match(line)
            if m and not m.group(1).startswith('*') and not m.group(1).startswith('0x'):
                pending = m.group(1)
                continue
            m = RE_IN_CONT.match(line)
            if m and pending is not None:
                addr, size, obj = m.groups()
                rows.append({'out': out, 'in': pending, 'sym': sym_of(pending),
                             'addr': int(addr, 16), 'size': int(size, 16),
                             'obj': obj, 'cls': classify(pending)})
                pending = None
                continue
    json.dump(rows, open(dst, 'w'))
    tot = {}
    for r in rows:
        k = (r['out'], r['cls'])
        tot[k] = tot.get(k, 0) + r['size']
    for k in sorted(tot):
        print('%-26s %-7s %10d' % (k[0], k[1], tot[k]))
    print('rows', len(rows))


if __name__ == '__main__':
    main()
