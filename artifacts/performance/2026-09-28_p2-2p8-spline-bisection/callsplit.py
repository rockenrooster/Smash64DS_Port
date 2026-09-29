"""Call counts per caller for a set of callee symbols, split into over-gate
frames and the rest, from a per-frame-region task37 profile.

usage: python callsplit.py <dis> <profile-dir> <gate_cycles> callee [callee...]
"""
import collections
import re
import sys

dis, pdir, gate = sys.argv[1], sys.argv[2], int(sys.argv[3])
callees = sys.argv[4:]
pat = re.compile(r'\s(?:bl|blx)\s+[0-9a-f]+ <(' + '|'.join(re.escape(c) for c in callees) + r')>')
sites = {}
func = None
for line in open(dis, encoding='utf-8', errors='replace'):
    m = re.match(r'^([0-9a-f]{8}) <(.+)>:', line)
    if m:
        func = m.group(2)
        continue
    m = pat.search(line)
    if m:
        pc = int(line.split(':')[0].strip(), 16)
        sites[pc] = (func, m.group(1))
over = set()
nreg = 0
with open(pdir + '/arm9-profile.regions.csv') as f:
    next(f)
    for line in f:
        p = line.split(',')
        nreg += 1
        if int(p[2]) > gate:
            over.add(p[0])
nover = len(over)
nctl = nreg - nover
cnt_o = collections.Counter()
cnt_c = collections.Counter()
with open(pdir + '/arm9-profile.csv') as f:
    next(f)
    for line in f:
        # region,pc,mode,opcode,instructions,...
        p = line.split(',', 5)
        pc = int(p[1], 16)
        s = sites.get(pc)
        if s is None:
            continue
        n = int(p[4])
        if p[0] in over:
            cnt_o[s] += n
        else:
            cnt_c[s] += n
byf_o = collections.Counter()
byf_c = collections.Counter()
for s, n in cnt_o.items():
    byf_o[s] += n
for s, n in cnt_c.items():
    byf_c[s] += n
keys = set(byf_o) | set(byf_c)
rows = []
for k in keys:
    o = byf_o[k] / max(nover, 1)
    c = byf_c[k] / max(nctl, 1)
    rows.append((o - c, o, c, k))
rows.sort(reverse=True)
print(f'regions {nreg} over {nover} control {nctl}; sites {len(sites)}')
tot_o = sum(byf_o.values()) / max(nover, 1)
tot_c = sum(byf_c.values()) / max(nctl, 1)
print(f'calls/frame: over {tot_o:.1f} control {tot_c:.1f}')
print(' premium    over/fr  ctrl/fr  caller -> callee')
for d, o, c, k in rows[:60]:
    print(f'{d:8.1f} {o:9.1f} {c:8.1f}  {k[0]} -> {k[1]}')
