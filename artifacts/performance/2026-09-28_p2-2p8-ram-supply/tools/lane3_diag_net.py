#!/usr/bin/env python3
"""lane3_diag_net.py -- net bytes freed by deleting ndsResetStartupDiagnostics.

Usage: lane3_diag_net.py <lane3_members_diag_<elf>.csv> <script-refs.csv> <taskman_seam_core.c>

Inputs
  members csv   : symbols only ndsResetStartupDiagnostics keeps alive (lane3_report_tables.py)
  script-refs   : lane3_script_refs.py --out (symbols named by a script/tool)
  source        : src/port/taskman_seam_core.c (to find the non-zero initial values)
The function zero-fills BSS globals that crt0 already zeroes.  What has to survive a
deletion:  (a) counters some script/probe reads by name (keep the symbol), and
(b) globals the function sets to a NON-ZERO value (0xffffffff, 1, -1, an enum): they must
become initialised data.  Everything else is freed.
"""
import collections
import csv
import re
import sys


def base(n):
    return re.sub(r'(\.(constprop|isra|part|cold|lto_priv)\.\d+)+$', '', n)


def main():
    members, refs, src = sys.argv[1], sys.argv[2], sys.argv[3]
    named = set(r['symbol'] for r in csv.DictReader(open(refs, newline='')))
    s = open(src, encoding='utf-8', errors='replace').read()
    i = s.index('void ndsResetStartupDiagnostics(void)')
    depth = 0
    k = s.index('{', i)
    while True:
        c = s[k]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                break
        k += 1
    body = s[i:k + 1]
    nz = set(re.findall(r'NDS_DIAG_WORD\(\s*(\w+)\s*,\s*[12]u\s*\)', body))
    zero_direct = ('0', '0u', 'FALSE', 'NULL', '0.0F', '0.0f')
    for n, _, v in re.findall(r'^\s+(g\w+|s\w+)(\[[^\]]*\])?\s*=\s*([^;]+);', body, re.M):
        if v.strip() not in zero_direct:
            nz.add(n)
    tot = collections.Counter()
    keep_named = collections.Counter()
    keep_nz = collections.Counter()
    freed = collections.Counter()
    for r in csv.DictReader(open(members, newline='')):
        kind, sz, nm = r['kind'], int(r['size']), base(r['name'])
        tot[kind] += sz
        if nm in named and kind in ('bss', 'data'):
            keep_named[kind] += sz
        elif nm in nz and kind in ('bss', 'data'):
            keep_nz[kind] += sz
        else:
            freed[kind] += sz
    print('function source lines: %d..%d' % (s[:i].count('\n') + 1, s[:k].count('\n') + 1))
    print('non-zero-initialised globals:', len(nz))
    print('unit total          ', dict(tot), sum(tot.values()))
    print('keep: script-named  ', dict(keep_named), sum(keep_named.values()))
    print('keep: non-zero init ', dict(keep_nz), sum(keep_nz.values()))
    print('net freed           ', dict(freed), sum(freed.values()))


if __name__ == '__main__':
    main()
