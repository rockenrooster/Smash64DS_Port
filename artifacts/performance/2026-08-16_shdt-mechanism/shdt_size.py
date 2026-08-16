"""What a PARTIAL cut of SHDT is worth, and what SHDT actually is.

The 2026-08-16 audit (../2026-08-16_match-io-audit/IO_AUDIT.md section 5) priced the
SHDT cluster by DELETING ALL its excess: -57,152 at rank-80, 88% of the +64,977
requirement. That is a ceiling, not a lever -- live hitbox hit detection must run
every simulation tick and must produce identical results, so no implementation
deletes its excess. This script prices the fractions an implementation could
actually reach, and states the band rather than the rank-80 point.

Basis: build-c219-animitcm-ship, ../2026-08-16_match-io-audit/io-rows.csv
(byte-identical to ../2026-08-16_anim-itcm/ship-rows.csv and to io8-rows.csv --
three emulator sessions, one ROM), whole match, gate arm
NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1,
1,600 samples, frames 439-2038, DLDI ON, slips=0.

Bucket tree and the 2^22 instrument correction are taken verbatim from the audit's
overgate.py so the cluster reproduces exactly.
"""
import csv, sys
from collections import Counter
import numpy as np

PATH = sys.argv[1] if len(sys.argv) > 1 else \
    'artifacts/performance/2026-08-16_match-io-audit/io-rows.csv'
TOPN, TWO22, GATE, APP = 80, 1 << 22, 1120380, 24947

rows = [{k: int(v) for k, v in x.items()} for x in csv.DictReader(open(PATH))]
BUCK = [c for c in rows[0] if c != 'frame']
bad = [r for r in rows if r['ALL'] >= TWO22]
for r in bad:
    for c in BUCK:
        if r[c] >= TWO22:
            r[c] -= TWO22
print(f'2^22 instrument correction applied to {len(bad)} frames: '
      f'{[r["frame"] for r in bad]}')

LEAF = ['FTR','STG','BG','AUD','MISC','OTHRW','SRCRES','SWRM','GCRARES',
        'SITR','SCPU','SPHD','SPHC','SCAT','SHDT','SPRM']

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
med = np.median(L, axis=0)
SH = LEAF.index('SHDT')

def rank80(w): return np.sort(np.asarray(w, np.int64))[::-1][TOPN-1]
def band(w):
    s = np.sort(np.asarray(w, np.int64))[::-1]
    return s[40:120].mean()
base, bandbase = rank80(W), band(W)
print(f'CLOSURE exact on all {len(rows)} rows (leafsum == WORK-H)')
print(f'corrected rank-80 {base:,} raw / {base-APP:,} net   gate {GATE:,}   '
      f'REQUIREMENT {base-APP-GATE:+,}')
print(f'ranks 41-120 band mean {bandbase:,.0f}')

# ---- the cluster, reproduced by the audit's own dominant-excess-owner rule ----
order = np.argsort(-W)
sel = order[:TOPN]
E = np.maximum(L[sel] - med, 0)
dom = [LEAF[i] for i in E.argmax(1)]
cl = np.array([sel[i] for i in range(TOPN) if dom[i] == 'SHDT'])
clE = np.array([E[i, SH] for i in range(TOPN) if dom[i] == 'SHDT'], dtype=np.int64)
print(f'\nSHDT cluster reproduces at n={len(cl)}   frames {sorted(F[cl].tolist())}')
print(f'  median own SHDT excess {int(np.median(clE)):,}   '
      f'total SHDT excess on the cluster {clE.sum():,}')

# ---- section 1: what SHDT IS across the whole run ------------------------------
sh = L[:, SH]
print(f'\n--- the SHDT lane, whole match, 1,600 frames ---')
print(f'  P50 {int(np.median(sh)):,}   mean {sh.mean():,.0f}   '
      f'P95 {int(np.percentile(sh,95)):,}   max {sh.max():,}   '
      f'concentration mean/P50 {sh.mean()/np.median(sh):.1f}x')
print(f'  lane TOTAL over the match {sh.sum():,} tk   '
      f'flat baseline (P50 x 1600) {int(np.median(sh))*1600:,} tk   '
      f'excursion {sh.sum()-int(np.median(sh))*1600:,} tk')
eng = sh > 30000
print(f'  frames over 30,000: {eng.sum()}   they carry '
      f'{sh[eng].sum()/sh.sum()*100:.1f}% of the lane')

# ---- section 2: the fraction curve, cluster-only vs every frame ----------------
print(f'\n--- EXACT RE-RANK: delete a FRACTION of SHDT, two shapes ---')
print(f'{"f":>5s} | {"cluster-only (33 fr)":>34s} | {"every frame (1,600)":>34s}')
print(f'{"":>5s} | {"rank-80":>10s} {"moved":>8s} {"band41-120":>12s} | '
      f'{"rank-80":>10s} {"moved":>8s} {"band41-120":>12s}')
for f in (0.10, 0.20, 0.25, 0.333, 0.50, 0.667, 0.75, 1.00):
    wc = W.copy(); wc[cl] -= (clE * f).astype(np.int64)
    # "every frame" cuts f of the SHDT bucket wherever it is, which is what an
    # implementation that makes the chain cheaper actually does.
    we = W - (sh * f).astype(np.int64)
    print(f'{f:>5.3f} | {rank80(wc):>10,} {base-rank80(wc):>8,} {band(wc):>12,.0f} | '
          f'{rank80(we):>10,} {base-rank80(we):>8,} {band(we):>12,.0f}')

# ---- section 3: how much of the gap each shape can reach at ALL ----------------
print(f'\n--- ceilings ---')
for name, w2 in (
        ('delete ALL SHDT excess on the 33-frame cluster', (lambda: (
            lambda w: (w.__setitem__(cl, w[cl]-clE), w)[1])(W.copy()))()),
        ('delete the WHOLE SHDT bucket on every frame', W - sh),
        ('delete SHDT down to its own P50 on every frame',
         W - np.maximum(sh - int(np.median(sh)), 0)),
):
    r = rank80(w2)
    print(f'  {name:52s} rank-80 {r:>10,}  moved {base-r:>7,}  '
          f'gap {r-APP-GATE:>+8,}  band {band(w2):>11,.0f}')

# ---- section 4: the shape a real implementation has ----------------------------
# NOT a component-share model. The engaged-frame premium measured on the profile
# (c191-engaged57-leafattr.txt, +268,255 tk/fr over the 57 engaged regions) spans
# EVERY bucket -- the animation rebuild lands in SPRM, the asset I/O in SITR, the
# hit SFX in SITR -- so its component shares cannot be mapped onto the SHDT
# bucket. Only the fighter collision chain is inside SHDT, and on the engaged set
# it is +102,988 tk/fr (self 35,277 + leaf 67,712) = 38.4% of that premium.
#
# What CAN be priced against the gate series is the shape: a cheaper chain makes
# SHDT cheaper wherever SHDT runs, in proportion to the chain's share of it. The
# excursion-only shape below is the right one -- the flat 4,608 baseline is the
# ground-hazard scan and the k-flag scan, which no chain change touches.
exc = np.maximum(sh - int(np.median(sh)), 0)
print(f'\n--- the shape a real implementation has: cut a FRACTION of the SHDT')
print(f'    EXCURSION (bucket minus its own P50) on every frame ---')
print(f'{"f":>5s} {"rank-80":>10s} {"moved":>8s} {"band41-120":>12s} {"gap":>9s}')
for f in (0.10, 0.20, 0.25, 0.333, 0.50, 0.667, 0.75, 1.00):
    w2 = W - (exc * f).astype(np.int64)
    print(f'{f:>5.3f} {rank80(w2):>10,} {base-rank80(w2):>8,} {band(w2):>12,.0f} '
          f'{rank80(w2)-APP-GATE:>+9,}')
print(f'\n  the fraction of the SHDT EXCURSION that must go to close +{base-APP-GATE:,}:')
for f in np.arange(0.80, 1.001, 0.02):
    w2 = W - (exc * f).astype(np.int64)
    if rank80(w2) - APP - GATE <= 0:
        print(f'    f = {f:.2f}  -> rank-80 {rank80(w2):,}  gap {rank80(w2)-APP-GATE:+,}')
        break
else:
    print('    NONE. Deleting 100% of the SHDT excursion does not close the gap.')
