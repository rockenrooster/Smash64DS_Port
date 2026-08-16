"""What an FTR saving is worth at rank-80, and how concentrated the lane is.

An FTR candidate is not an event-frame candidate: it touches EVERY frame, so
../2026-08-16_sitr-attach-lane/convert.py's 288-frame re-rank is the wrong
instrument for it.  This is the same exact re-sort of the c220 basis's own 1,600
rows, with a uniform D subtracted from every frame and capped at that frame's own
FTR value, plus the conversion curve the brief asks for rather than one point.

Basis build-c220-camship, ../2026-08-16_camera-ship/ship220-rows.csv, apparatus
24,947, gate 1,120,380, rank-80 1,210,624 raw, REQUIREMENT +65,297.  No emulator.
"""
import csv
import statistics as st
import sys

RING = 'artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
D = 'artifacts/performance/2026-08-16_ftr-attribution/'
APP, GATE, TOPN = 24947, 1120380, 80

rows = [{k: int(v) for k, v in r.items()} for r in csv.DictReader(open(RING))]
W = [r['WORK-H'] for r in rows]
F = [r['FTR'] for r in rows]
base = sorted(W, reverse=True)[TOPN - 1]
band = sum(sorted(W, reverse=True)[40:120]) / 80.0
print('CONTROL  rank-80 %d raw / %d net   level %+d   band 41-120 %.0f'
      % (base, base - APP, base - APP - GATE, band))
print('         (expect 1,210,624 / 1,185,677 / +65,297 / 1,218,356)')
print('FTR P50 %d, band 41-120 %.0f, ratio %.3f -- the lane is FLAT, which is '
      'why a\nuniform cut converts 1:1 here and would not in SITR or SHDT.'
      % (st.median(F), sum(F[i] for i in
                           sorted(range(len(W)), key=lambda i: -W[i])[40:120])
         / 80.0,
         (sum(F[i] for i in sorted(range(len(W)), key=lambda i: -W[i])[40:120])
          / 80.0) / st.median(F)))
over = sum(1 for w in W if w >= base)
print('touched frames: ALL %d.  At or above rank-80: %d.  A cut that reaches '
      'every\nframe has no sub-population that can sink below the rank and '
      'stop paying.' % (len(W), over))


def rerank(d):
    ser = sorted((W[i] - min(d, F[i]) for i in range(len(W))), reverse=True)
    return ser[TOPN - 1]


print('')
print('=== CONVERSION CURVE: a uniform D on the FTR lane ===')
print('%-56s %8s %9s %8s %7s %11s'
      % ('candidate', 'D', 'rank-80', 'moved', 'ratio', 'level'))
CAND = [
    ('gmCameraLookAtFuncMatrix hoisted to once per frame', 5143),
    ('ndsRendererMtxMulAffine20p12 -> ITCM (616 B)', 2989),
    ('ftDisplayMainDrawDefault + LoadHardwareSplitMatrices -> ITCM', 3231),
    ('the whole 908 B ITCM pool, best FTR tenants', 4000),
    ('cold-path out-of-lining, REALISTIC third-to-half', 4900),
    ('cold-path out-of-lining, PERFECT-COMPACTION CEILING', 9714),
    ('cross-build placement floor, for reference', 14080),
    ('the CAPTURE pass deleted entirely (ceiling)', 44600),
    ('the whole PER-JOINT MATRIX build deleted (ceiling)', 61848),
    ('the whole NATIVE PRODUCTION emit deleted (ceiling)', 71448),
    ('FTR cut by 22%, the lanes.txt figure that closes the gate', 63895),
]
for name, d in CAND:
    r = rerank(d)
    print('%-56s %8d %9d %8d %7.3f %+11d'
          % (name[:56], d, r, base - r, (base - r) / d, r - APP - GATE))

print('')
print('=== the curve itself, so nobody quotes one point ===')
for d in (1000, 2989, 5143, 9714, 14080, 20000, 44600, 65297, 90000, 120000):
    r = rerank(d)
    print('  D=%-7d rank-80 %d  moved %6d  ratio %.3f  level %+d'
          % (d, r, base - r, (base - r) / d, r - APP - GATE))
print('The ratio holds at 1.000 until D exceeds the FTR value of the frames')
print('near the rank; the lane is flat so that does not happen below ~120,000.')
