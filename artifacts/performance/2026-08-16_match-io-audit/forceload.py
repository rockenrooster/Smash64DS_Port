"""Items A and B. What the seven in-match card reads and the 134 force-loads cost.

Buckets come from the RING dump (`../2026-08-16_anim-itcm/ship-rows.csv`), which is
internally exact -- ALL == WORK+WAIT and ALL-named == OTHR on all 1,600 rows.
Counters come from the per-frame-stop run (`pf-rows.csv`) on the same ROM.

ALIGNMENT, measured not assumed: the -PerFrameGlobals path emits a TORN row --
`ALL` from iteration f, every other bucket from iteration f+1 -- so 1,526 of its
1,600 rows fail ALL == WORK+WAIT. Its counters align with ring frame f+1; the
association is +212,620 at that offset and -6,751 / -882 at offsets 0 and -1.
Only the counter columns are taken from it.

INSTRUMENT CORRECTION: frames whose ALL exceeds 2^22 carry a cpuGetTiming()
overflow-correction failure worth exactly 4,194,304 ticks; corrected here.
"""
import csv
from collections import Counter
import numpy as np

TWO22 = 1 << 22
GATE, APP = 1120380, 24947
ring = {int(x['frame']): {k: int(v) for k, v in x.items()}
        for x in csv.DictReader(open('artifacts/performance/2026-08-16_anim-itcm/ship-rows.csv'))}
pf = [{k: int(v) for k, v in x.items()}
      for x in csv.DictReader(open('artifacts/performance/2026-08-16_match-io-audit/pf-rows.csv'))]

for r in ring.values():
    if r['ALL'] >= TWO22:
        for c in r:
            if c != 'frame' and r[c] >= TWO22:
                r[c] -= TWO22

def delta(col):
    v = [r[col] for r in pf]
    return {pf[i]['frame'] + 1: (v[i] - v[i-1] if i else 0) for i in range(len(pf))}
FL, RD = delta('gNdsR204AnimForceLoadTotal'), delta('gNdsRelocAssetPayloadReadCount')

LEAF = ['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
        'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six = r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return [r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
            r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,
            r['SINT']-r['SCPU'],r['SCPU'],r['SPHD'],r['SPHC'],r['SCAT'],
            r['SHDT'],r['SPRM']]

frames = sorted(f for f in ring if f in FL)
L = np.array([leaves(ring[f]) for f in frames], dtype=np.int64)
W = np.array([ring[f]['WORK-H'] for f in frames], dtype=np.int64)
fl = np.array([FL[f] for f in frames]); rd = np.array([RD[f] for f in frames])
F = np.array(frames)
assert (L.sum(1) == W).all()
allW = np.array([ring[f]['WORK-H'] for f in sorted(ring)], dtype=np.int64)
srt = np.sort(allW)[::-1]; base = srt[79]
rank = {f: int(np.searchsorted(-srt, -ring[f]['WORK-H']) + 1) for f in ring}
print(f'window ring frames {F[0]}..{F[-1]}  n={len(F)}   '
      f'rank-80 {base:,} raw / {base-APP:,} net  gap {base-APP-GATE:+,}')
print(f'force-loads {fl.sum()}  on {(fl>0).sum()} frames   card reads {rd.sum()}')
print('\nTHE SEVEN IN-MATCH CARD READS')
for i in np.where(rd > 0)[0]:
    print(f'   ring frame {F[i]}   WORK-H {W[i]:>9,}   rank {rank[F[i]]:>4} of 1,600   '
          f'SITR {L[i][LEAF.index("SITR")]:>9,}')

print('\nDOES THE COST SCALE WITH THE COUNT?  (the falsifier: more of the cause)')
cols = ['SITR','SCPU','SPHD','SHDT','SPRM','GCRARES','SRCRES','FTR','MISC','STG','OTHRW']
b = fl == 0
print(f'{"dFL":>4} {"n":>5} {"WORK-H mean":>12} {"lift":>10} {"per-FL":>9} ' +
      ' '.join(f'{c:>8}' for c in cols))
mb = W[b].mean()
print(f'{0:>4} {b.sum():>5} {mb:>12,.0f} {"":>10} {"":>9} ' +
      ' '.join(f'{L[b][:,LEAF.index(c)].mean():>8,.0f}' for c in cols))
for k in (1, 2, 3):
    m = (fl == k) & (rd == 0)
    if m.sum() < 3: continue
    print(f'{k:>4} {m.sum():>5} {W[m].mean():>12,.0f} {W[m].mean()-mb:>+10,.0f} '
          f'{(W[m].mean()-mb)/k:>+9,.0f} ' +
          ' '.join(f'{L[m][:,LEAF.index(c)].mean():>8,.0f}' for c in cols))
m = rd > 0
print(f'{"read":>4} {m.sum():>5} {W[m].mean():>12,.0f} {W[m].mean()-mb:>+10,.0f} {"":>9} ' +
      ' '.join(f'{L[m][:,LEAF.index(c)].mean():>8,.0f}' for c in cols))

print('\nMATCHED: a card read against a force-load frame that did no I/O')
a = (fl > 0) & (rd == 0)
print(f'   plain force-load  n={a.sum():>4}  P50 {np.median(W[a]):>10,.0f}  mean {W[a].mean():>10,.0f}')
print(f'   + card read       n={m.sum():>4}  P50 {np.median(W[m]):>10,.0f}  mean {W[m].mean():>10,.0f}')
print(f'   card-read premium over a plain force-load: '
      f'{np.median(W[m])-np.median(W[a]):+,.0f} median / {W[m].mean()-W[a].mean():+,.0f} mean')
for c in cols:
    j = LEAF.index(c); d = L[m][:, j].mean() - L[a][:, j].mean()
    if abs(d) > 5000: print(f'      {c:8s} {d:>+10,.0f}')

print('\nOVER-GATE CROSS-TAB (top-80 of the ring series, dominant excess owner)')
med = np.median(L, axis=0)
sel = np.argsort(-W)[:80]; E = np.maximum(L[sel]-med, 0)
dom = [LEAF[i] for i in E.argmax(1)]
print(f'{"owner":8s} {"n":>3s} {"withFL":>7s} {"withRead":>9s} {"median own excess":>18s}')
for k, _ in Counter(dom).most_common():
    idx = [i for i in range(80) if dom[i] == k]
    print(f'{k:8s} {len(idx):>3d} {sum(1 for i in idx if fl[sel][i]>0):>7d} '
          f'{sum(1 for i in idx if rd[sel][i]>0):>9d} '
          f'{np.median(E[idx][:,LEAF.index(k)]):>18,.0f}')
print(f'{"TOTAL":8s} {80:>3d} {sum(1 for i in range(80) if fl[sel][i]>0):>7d} '
      f'{sum(1 for i in range(80) if rd[sel][i]>0):>9d}')
print(f'base rate: {(fl>0).sum()}/{len(F)} = {(fl>0).sum()/len(F)*100:.1f}% of frames carry a force-load')

print('\nEXACT RE-RANK')
def show(n, w2):
    r = np.sort(np.asarray(w2, dtype=np.int64))[::-1][79]
    print(f'  {n:52s} {r:>10,}  moved {base-r:>8,}  gap {r-APP-GATE:>+9,}')
show('(control) untouched', W)
w = W.copy(); w[rd > 0] = np.int64(np.median(W[a])); show('7 card reads -> a plain force-load frame', w)
w = W.copy(); w[rd > 0] = np.int64(np.median(W[b])); show('7 card reads -> a quiet frame', w)
w = W.copy(); w[a] -= np.int64(np.median(W[a]) - np.median(W[b])); show('every force-load premium deleted (109 fr)', w)
w = W.copy(); w[fl > 0] = np.int64(np.median(W[b])); show('every force-load frame -> a quiet frame', w)
j = LEAF.index('SITR'); show('all SITR excess above its own median deleted', W - np.maximum(L[:, j]-med[j], 0))
