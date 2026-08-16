"""Item C. Is the over-gate set one population or several, and what is each worth?

Basis: build-c219-animitcm-ship, artifacts/performance/2026-08-16_anim-itcm/ship-rows.csv,
whole match, gate arm (NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1),
1,600 samples, frames 439-2038, DLDI ON, slips=0. Byte-reproduced by two further
emulator sessions this cycle (io-rows.csv, io8-rows.csv are identical to it).

Bucket tree read from the accumulation sites (closure asserted exact below):
  WORK-H = FTR+STG+BG+AUD+MISC+(OTHR-WAIT)+SRC
  SRC    = GCRA + SWRM + SRCRES         GCRA = SINT+SPHD+SPHC+SCAT+SHDT+SPRM+GCRARES
  SINT   = SCPU + SITR

INSTRUMENT CORRECTION: cpuGetTiming() intermittently reports a span exactly
2^22 = 4,194,304 ticks too large (see IO_AUDIT.md section 2). Frames whose ALL
exceeds 2^22 carry it; every affected bucket is corrected by one subtraction.
"""
import csv, sys
from collections import Counter
import numpy as np

PATH = sys.argv[1] if len(sys.argv) > 1 else \
    'artifacts/performance/2026-08-16_anim-itcm/ship-rows.csv'
TOPN = int(sys.argv[2]) if len(sys.argv) > 2 else 80
TWO22 = 1 << 22
GATE, APP = 1120380, 24947

rows = [{k: int(v) for k, v in x.items()} for x in csv.DictReader(open(PATH))]
BUCK = [c for c in rows[0] if c != 'frame']
bad = [r for r in rows if r['ALL'] >= TWO22]
for r in bad:
    for c in BUCK:
        if r[c] >= TWO22:
            r[c] -= TWO22
print(f'instrument correction applied to {len(bad)} frames: '
      f'{[r["frame"] for r in bad]}')

LEAF = ['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
        'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']
SIM = {'SRCRES','SWRM','GCRARES','SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM'}

def leaves(r):
    six = r['SINT']+r['SPHD']+r['SPHC']+r['SCAT']+r['SHDT']+r['SPRM']
    return [r['FTR'], r['STG'], r['BG'], r['AUD'], r['MISC'], r['OTHR']-r['WAIT'],
            r['SRC']-r['GCRA']-r['SWRM'], r['SWRM'], r['GCRA']-six,
            r['SINT']-r['SCPU'], r['SCPU'], r['SPHD'], r['SPHC'], r['SCAT'],
            r['SHDT'], r['SPRM']]

L = np.array([leaves(r) for r in rows], dtype=np.int64)
W = np.array([r['WORK-H'] for r in rows], dtype=np.int64)
F = np.array([r['frame'] for r in rows], dtype=np.int64)
assert (L.sum(1) - W == 0).all(), 'leaf closure failed'
print(f'CLOSURE exact on all {len(rows)} rows (leafsum == WORK-H)')

med = np.median(L, axis=0)
srt = np.sort(W)[::-1]
def rank80(w): return np.sort(w)[::-1][TOPN-1]
base = rank80(W)
print(f'\ncorrected rank-{TOPN} {base:,} raw / {base-APP:,} net   gate {GATE:,}   '
      f'GAP {base-APP-GATE:+,}')
print('band 41-120: ' + ' '.join(f'{srt[i]:,}' for i in
      (40, 49, 59, 69, 79, 89, 99, 109, 119)))

order = np.argsort(-W)
sel = order[:TOPN]
E = np.maximum(L[sel] - med, 0)
tot = E.sum(1)
insim = np.array([E[:, LEAF.index(k)] for k in SIM]).sum(0)
q = np.percentile(insim / tot * 100, [0, 5, 25, 50, 75, 95, 100])
print('\nshare of each over-gate frame\'s excess that lies INSIDE the 60 Hz sim (SRC):')
print('  pct 0/5/25/50/75/95/100 = ' + ' / '.join(f'{v:.1f}%' for v in q))
out = np.where(insim / tot < 0.5)[0]
print(f'  majority-OUTSIDE frames: {len(out)}/{TOPN} -> '
      f'{sorted(int(F[sel][i]) for i in out)}')

dom = [LEAF[i] for i in E.argmax(1)]
owners = [k for k, _ in Counter(dom).most_common()]
cols = ['SHDT','SITR','SPHD','SPRM','SCPU','SCAT','GCRARES','SRCRES','MISC','FTR','STG','OTHRW']
print('\nCONDITIONAL PROFILE -- median EXCESS of every leaf on each owner\'s frames.')
print(f'{"owner":8s} {"n":>3s} ' + ' '.join(f'{c:>8s}' for c in cols))
for k in owners:
    idx = [i for i in range(TOPN) if dom[i] == k]
    m = np.median(E[idx], axis=0)
    print(f'{k:8s} {len(idx):>3d} ' +
          ' '.join(f'{int(m[LEAF.index(c)]):>8,}' for c in cols))
print(f'{"runP50":8s} {len(rows):>3d} ' +
      ' '.join(f'{int(med[LEAF.index(c)]):>8,}' for c in cols))

print('\nEXACT RE-RANK -- delete the named work, re-sort all 1,600, read rank-80.')
print(f'{"intervention":42s} {"rank-80":>10s} {"moved":>8s} {"gap":>9s}')
def show(n, w2):
    r = rank80(np.asarray(w2, dtype=np.int64))
    print(f'{n:42s} {r:>10,} {base-r:>8,} {r-APP-GATE:>+9,}')
show('(control) untouched', W)
show(f'calibration: flat -{base-APP-GATE:,} everywhere', W - (base-APP-GATE))
for k in owners:
    idx = np.array([sel[i] for i in range(TOPN) if dom[i] == k])
    cut = np.array([E[i].sum() for i in range(TOPN) if dom[i] == k], dtype=np.int64)
    w2 = W.copy(); w2[idx] -= cut
    show(f'delete ALL excess on the {k} cluster ({len(idx)} fr)', w2)
w2 = W.copy(); w2[sel] -= E.sum(1).astype(np.int64)
show(f'delete ALL excess on all {TOPN}', w2)
for k in ['SHDT','SITR','SPHD','SPRM','SCPU','GCRARES','FTR','MISC','STG']:
    j = LEAF.index(k)
    show(f'halve {k} on EVERY frame', W - L[:, j] // 2)
