"""Item C, method 2: Ward clustering of the over-gate frames' excess-share vectors.

Method 1 (dominant-excess-owner labelling), the closure assertion, the conditional
profile and the re-rank arithmetic are in overgate.py. This file exists to show that
a second, independent grouping method finds the same five clusters.

Basis and instrument correction: see overgate.py.
"""
import csv
from collections import Counter
import numpy as np
from sklearn.cluster import AgglomerativeClustering
from sklearn.metrics import silhouette_score

TWO22 = 1 << 22
rows = [{k: int(v) for k, v in x.items()} for x in csv.DictReader(
    open('artifacts/performance/2026-08-16_anim-itcm/ship-rows.csv'))]
for r in rows:
    if r['ALL'] >= TWO22:
        for c in r:
            if c != 'frame' and r[c] >= TWO22:
                r[c] -= TWO22

LEAF = ['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
        'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
def leaves(r):
    six = r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return [r['FTR'],r['STG'],r['BG'],r['AUD'],r['MISC'],r['OTHR']-r['WAIT'],
            r['SRC']-r['GCRA']-r['SWRM'],r['SWRM'],r['GCRA']-six,
            r['SINT']-r['SCPU'],r['SCPU'],r['SPHD'],r['SPHC'],r['SCAT'],
            r['SHDT'],r['SPRM']]

L = np.array([leaves(r) for r in rows], dtype=np.int64)
W = np.array([r['WORK-H'] for r in rows], dtype=np.int64)
F = np.array([r['frame'] for r in rows])
assert (L.sum(1) == W).all()
med = np.median(L, axis=0)
sel = np.argsort(-W)[:80]
E = np.maximum(L[sel] - med, 0)
S = E / np.maximum(E.sum(1, keepdims=True), 1)

print('WARD ON THE TOP-80 EXCESS-SHARE VECTORS (corrected series)')
for k in range(2, 9):
    lab = AgglomerativeClustering(n_clusters=k, linkage='ward').fit_predict(S)
    print(f'  k={k}  silhouette {silhouette_score(S, lab):.3f}  '
          f'sizes {sorted(Counter(lab).values(), reverse=True)}')
lab = AgglomerativeClustering(n_clusters=5, linkage='ward').fit_predict(S)
print(f'\nk=5, silhouette {silhouette_score(S, lab):.3f}')
for cl in sorted(set(lab), key=lambda c: -(lab == c).sum()):
    i = np.where(lab == cl)[0]
    p = S[i].mean(0); t = np.argsort(-p)[:4]
    print(f'  n={len(i):>2}  WORK-H median {int(np.median(W[sel][i])):>9,}   ' +
          '  '.join(f'{LEAF[x]} {p[x]*100:.0f}%' for x in t))
    print(f'        frames {sorted(int(F[sel][x]) for x in i)}')
