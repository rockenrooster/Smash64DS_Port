#!/usr/bin/env python3
"""lane3_srcmap.py -- attach a source file:line (DWARF, via nm -l) to every symbol.

Usage: lane3_srcmap.py <nm-sysv.txt> <nm-l.txt> <out.csv>
       lane3_srcmap.py summary <out.csv> [--match REGEX] [--top N]

nm -l lines:  <addr> [<size>] <type> <name>\t<file>:<line>
Joined to the sysv table on (addr, name).  Output CSV columns:
  name,addr,size,section,kind,srcfile,line
'srcfile' is the path relative to src/ or include/ when possible, else the basename.
"""
import collections
import csv
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_symtab as st  # noqa: E402

RE_NML = re.compile(r'^([0-9a-f]{8})\s+(?:([0-9a-f]{8})\s+)?(\S)\s+(\S+)(?:\t(.*))?$')


def norm(path):
    p = path.replace('\\', '/')
    for marker in ('/Smash64DS_Port/src/', '/Smash64DS_Port/include/', '/Smash64DS_Port/decomp/'):
        i = p.find(marker)
        if i >= 0:
            return p[i + len('/Smash64DS_Port/'):]
    if '/src/' in p and 'Smash64DS' in p:
        return p[p.index('/src/') + 1:]
    return p.split('/')[-1]


def build(sysv, nml, outp):
    syms = st.load(sysv)
    where = {}
    with open(nml, errors='replace') as f:
        for line in f:
            m = RE_NML.match(line.rstrip('\n'))
            if not m or not m.group(5):
                continue
            addr = int(m.group(1), 16)
            name = m.group(4)
            loc = m.group(5)
            mm = re.match(r'^(.*):(\d+)$', loc)
            if mm:
                where[(addr, name)] = (norm(mm.group(1)), int(mm.group(2)))
    n = 0
    miss = 0
    with open(outp, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['name', 'addr', 'size', 'section', 'kind', 'srcfile', 'line'])
        for s in syms:
            if s['size'] <= 0 or s['name'].startswith('$'):
                continue
            loc = where.get((s['addr'], s['name']))
            if loc is None:
                miss += 1
                loc = ('?', 0)
            w.writerow([s['name'], s['addr'], s['size'], s['sec'], s['kind'], loc[0], loc[1]])
            n += 1
    print('rows', n, 'without source', miss)


def summary(csvp, match=None, top=60):
    tot = collections.defaultdict(collections.Counter)
    with open(csvp, newline='') as f:
        for r in csv.DictReader(f):
            if r['section'].startswith('.ovl'):
                continue
            if match and not re.search(match, r['srcfile']):
                continue
            tot[r['srcfile']][r['kind']] += int(r['size'])
    rows = sorted(tot.items(), key=lambda kv: -sum(kv[1].values()))
    print('%-64s %9s %9s %9s %9s %9s' % ('srcfile', 'text', 'rodata', 'data', 'bss', 'total'))
    for k, c in rows[:top]:
        print('%-64s %9d %9d %9d %9d %9d' % (k, c['text'], c['rodata'], c['data'], c['bss'], sum(c.values())))


def main():
    if sys.argv[1] == 'summary':
        match = None
        top = 60
        if '--match' in sys.argv:
            match = sys.argv[sys.argv.index('--match') + 1]
        if '--top' in sys.argv:
            top = int(sys.argv[sys.argv.index('--top') + 1])
        summary(sys.argv[2], match, top)
    else:
        build(sys.argv[1], sys.argv[2], sys.argv[3])


if __name__ == '__main__':
    main()
