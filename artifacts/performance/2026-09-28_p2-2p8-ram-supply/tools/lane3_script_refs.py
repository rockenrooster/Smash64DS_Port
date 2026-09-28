#!/usr/bin/env python3
"""lane3_script_refs.py -- which checkers/tests/probes name the symbols of a unit.

Usage: lane3_script_refs.py <unit.csv> <repo-root> [--dirs scripts,tools,docs/p2] [--top N]
                             [--names-file extra_names.txt] [--out refs.csv]

Reads the CSV written by lane3_unit.py (column 'name'; '.constprop.N' style suffixes stripped),
tokenises every text file under the given directories into identifiers, and reports:
  * per file: how many unit symbols it mentions (top N) with a few examples
  * per symbol: number of files that mention it (only symbols with >= 1 file)
Nothing is written into the repo; --out writes a CSV wherever asked.
"""
import collections
import csv
import os
import re
import sys

IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]{3,}')
EXTS = ('.ps1', '.py', '.gdb', '.txt', '.json', '.md', '.sh', '.cmd', '.mk', '.h', '.c', '.inc', '.ld', '.csv', '.log')
SKIP_DIR = {'.git', 'node_modules', '__pycache__', 'artifacts', 'builds', 'build', 'backups', 'logs', 'sessions'}


def base(n):
    n = re.sub(r'(\.(constprop|isra|part|cold|lto_priv)\.\d+)+$', '', n)
    return n


def main():
    a = sys.argv
    names = set()
    with open(a[1], newline='') as f:
        for r in csv.DictReader(f):
            names.add(base(r['name']))
    if '--names-file' in a:
        for line in open(a[a.index('--names-file') + 1]):
            if line.strip():
                names.add(line.strip())
    root = a[2]
    dirs = ['scripts', 'tools']
    if '--dirs' in a:
        dirs = a[a.index('--dirs') + 1].split(',')
    top = int(a[a.index('--top') + 1]) if '--top' in a else 40
    perfile = collections.defaultdict(set)
    persym = collections.defaultdict(set)
    nfiles = 0
    for d in dirs:
        for dp, dn, fn in os.walk(os.path.join(root, d)):
            dn[:] = [x for x in dn if x not in SKIP_DIR]
            for f in fn:
                if not f.lower().endswith(EXTS):
                    continue
                p = os.path.join(dp, f)
                try:
                    if os.path.getsize(p) > 6_000_000:
                        continue
                    text = open(p, encoding='utf-8', errors='replace').read()
                except Exception:
                    continue
                nfiles += 1
                for tok in set(IDENT.findall(text)):
                    if tok in names:
                        rel = os.path.relpath(p, root).replace('\\', '/')
                        perfile[rel].add(tok)
                        persym[tok].add(rel)
    print('files scanned', nfiles, ' unit symbols', len(names), ' symbols named somewhere', len(persym))
    rows = sorted(perfile.items(), key=lambda kv: -len(kv[1]))
    for rel, syms in rows[:top]:
        ex = ', '.join(sorted(syms)[:4])
        print('%5d  %-70s %s' % (len(syms), rel, ex))
    if '--out' in a:
        with open(a[a.index('--out') + 1], 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['symbol', 'files'])
            for s in sorted(persym):
                w.writerow([s, ';'.join(sorted(persym[s]))])


if __name__ == '__main__':
    main()
