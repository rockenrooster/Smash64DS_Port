"""Over-gate cluster decomposition on the CURRENT basis (build-c220-camship).

Reproduces ../2026-08-16_match-io-audit/overgate.py's method on the new basis
rows, which the sampler already wrap-corrected live (WRAPFIX column), so no
offline 2^22 subtraction is applied here.

Leaf tree, read from the accumulation sites (include/nds/nds_startup.h:4283-4392,
src/port/reloc_backend_diagnostic_recorders.c:5996-6017):
  WORK-H = FTR + STG + BG + AUD + MISC + (OTHR-WAIT) + SRC
  SRC    = GCRA + SWRM + SRCRES
  GCRA   = SINT + SPHD + SPHC + SCAT + SHDT + SPRM + GCRARES
  SINT   = SCPU + SITR
"""
import csv, sys, statistics as st

PATH = sys.argv[1] if len(sys.argv) > 1 else \
    'artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
rows = [{k: int(v) for k, v in r.items()} for r in csv.DictReader(open(PATH))]
print('rows=%d  wrapfix_rows=%d' % (len(rows), sum(1 for r in rows if r.get('WRAPFIX'))))

LEAF = ['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
        'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six = r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return [r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
            r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,
            r['SINT']-r['SCPU'],r['SCPU'],r['SPHD'],r['SPHC'],r['SCAT'],
            r['SHDT'],r['SPRM']]

# closure assertion, no clamping
bad = 0; neg = {}
for r in rows:
    L = leaves(r)
    if sum(L) != r['WORK-H']: bad += 1
    for n, v in zip(LEAF, L):
        if v < 0: neg[n] = neg.get(n, 0) + 1
print('closure leafsum==WORK-H violations: %d of %d' % (bad, len(rows)))
print('negative-leaf frames:', neg if neg else 'none (nesting proof holds)')

med = {n: st.median([leaves(r)[i] for r in rows]) for i, n in enumerate(LEAF)}
print('\nrun medians:', {n: int(med[n]) for n in LEAF})

order = sorted(rows, key=lambda r: -r['WORK-H'])
top = order[:80]
rank80 = top[-1]['WORK-H']
print('\nrank-80 = %d   net = %d   level vs 1,120,380 gate = %+d'
      % (rank80, rank80-24947, rank80-24947-1120380))
print('band ranks 41-120 mean = %d' % (sum(r['WORK-H'] for r in order[40:120])//80))

# method 1: dominant excess owner
lab = {}
for r in top:
    L = leaves(r)
    exc = [(L[i]-med[n], n) for i, n in enumerate(LEAF)]
    lab[r['frame']] = max(exc)[1]
from collections import Counter, defaultdict
cnt = Counter(lab.values())
print('\ncluster sizes:', dict(cnt.most_common()))

groups = defaultdict(list)
for r in top: groups[lab[r['frame']]].append(r)
print('\n%-8s %3s %14s %14s %14s' % ('owner','n','median own exc','median own','run median'))
for name, g in sorted(groups.items(), key=lambda kv: -len(kv[1])):
    i = LEAF.index(name)
    vals = [leaves(r)[i] for r in g]
    print('%-8s %3d %14d %14d %14d   ratio own/runmed = %.2fx'
          % (name, len(g), int(st.median(vals)-med[name]), int(st.median(vals)),
             int(med[name]), st.median(vals)/max(med[name],1)))

# exact re-rank: delete each cluster's own-leaf excess
allw = sorted((r['WORK-H'] for r in rows), reverse=True)
print('\n%-30s %12s %9s %9s' % ('delete own excess on...','rank-80','moved','gap'))
print('%-30s %12d %9d %+9d' % ('(control)', rank80, 0, rank80-24947-1120380))
for name, g in sorted(groups.items(), key=lambda kv: -len(kv[1])):
    i = LEAF.index(name)
    fr = {r['frame'] for r in g}
    ser = []
    for r in rows:
        w = r['WORK-H']
        if r['frame'] in fr: w -= max(0, leaves(r)[i]-med[name])
        ser.append(w)
    ser.sort(reverse=True)
    print('%-30s %12d %9d %+9d' % (name+' (%d frames)'%len(g), ser[79], rank80-ser[79],
                                   ser[79]-24947-1120380))

for name in ('SITR','SHDT'):
    if name in groups:
        print('\n%s cluster frames: %s' % (name, sorted(r['frame'] for r in groups[name])))
