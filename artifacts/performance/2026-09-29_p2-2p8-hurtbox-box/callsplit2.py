"""Call counts per caller for a set of callee symbols, split into over-gate
frames (work = region cycles - armWaitForIrq > gate) and the rest.

usage: python callsplit2.py <dis> <profile-dir> <gate_work_cycles> callee [callee...]
Needs <profile-dir>/symreg.pkl (cf95.py pass 1).
"""
import collections
import pickle
import re
import sys

dis, pdir, gate = sys.argv[1], sys.argv[2], int(sys.argv[3])
callees = sys.argv[4:]
pat = re.compile(r'\s(?:bl|blx|b)\s+[0-9a-f]+ <(' + '|'.join(re.escape(c) for c in callees) + r')>')
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
per = pickle.load(open(pdir + '/symreg.pkl', 'rb'))
regions = sorted({r for v in per.values() for r in v})
idle = per.get('armWaitForIrq', {})
work = collections.Counter()
for s, v in per.items():
    for r, c in v.items():
        work[r] += c
over = set()
for r in regions:
    if work[r] - idle.get(r, 0) > gate:
        over.add(str(r))
nreg = len(regions)
nover = len(over)
nctl = nreg - nover
cnt_o = collections.Counter()
cnt_c = collections.Counter()
with open(pdir + '/arm9-profile.csv') as f:
    next(f)
    for line in f:
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
keys = set(cnt_o) | set(cnt_c)
rows = []
for k in keys:
    o = cnt_o[k] / max(nover, 1)
    c = cnt_c[k] / max(nctl, 1)
    rows.append((o - c, o, c, k))
rows.sort(reverse=True)
print(f'regions {nreg} over {nover} control {nctl}; sites {len(sites)}')
tot_o = sum(cnt_o.values()) / max(nover, 1)
tot_c = sum(cnt_c.values()) / max(nctl, 1)
print(f'calls/frame: over {tot_o:.1f} control {tot_c:.1f}')
print(' premium    over/fr  ctrl/fr  caller -> callee')
for d, o, c, k in rows[:70]:
    print(f'{d:8.1f} {o:9.1f} {c:8.1f}  {k[0]} -> {k[1]}')
print('--- by control-frame count (median cost) ---')
rows.sort(key=lambda t: -t[2])
for d, o, c, k in rows[:40]:
    print(f'{d:8.1f} {o:9.1f} {c:8.1f}  {k[0]} -> {k[1]}')
