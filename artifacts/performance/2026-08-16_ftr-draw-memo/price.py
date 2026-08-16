"""What the fighter draw-contract memo is worth, same binary, two arms.

build-c223-ftrmemo, one ELF (romSha DE80E46BDCF1FD98), 1,600 samples, frames
440-2039, gate arm NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 KEEP_CACHE=1, mode 163,
DLDI ON, slips=0 on both arms. Arms differ in exactly one poked .data word,
gNdsFtrDrawMemoRoute.route: 0 = the walk runs every capture (control), 1 = the
memo (candidate). No cross-build placement term.

Apparatus 24,947, gate 1,120,380, as on the c220 basis. The banked basis level
is +65,297 at rank-80 1,210,624; a same-binary move is applied to that level the
way ../2026-08-16_camera-ship/CAMERA_SHIP.md applied its own -8,896.
UNITS: 2 profile cycles = 1 project tick.
"""
import csv
import statistics as st

APP, GATE, TOPN = 24947, 1120380, 80
BASIS_RANK80, BASIS_LEVEL = 1210624, 65297
D = 'artifacts/performance/2026-08-16_ftr-draw-memo/'


def load(name):
    return [{k: int(v) for k, v in r.items()}
            for r in csv.DictReader(open(D + name))]


off = load('memo-off-rows.csv')
on = load('memo-on-rows.csv')
assert len(off) == len(on) == 1600, (len(off), len(on))


def rank(rows, key, n=TOPN):
    return sorted((r[key] for r in rows), reverse=True)[n - 1]


def band(rows, key, lo=40, hi=120):
    return sum(sorted((r[key] for r in rows), reverse=True)[lo:hi]) / (hi - lo)


print('%-10s %11s %11s %11s' % ('', 'CONTROL', 'CANDIDATE', 'delta'))
for k in ('WORK-H', 'FTR', 'STG', 'SRC', 'ALL'):
    a, b = rank(off, k), rank(on, k)
    print('%-10s %11d %11d %11d   rank-80' % (k, a, b, b - a))
for k in ('WORK-H', 'FTR'):
    a = st.median(r[k] for r in off)
    b = st.median(r[k] for r in on)
    print('%-10s %11.0f %11.0f %11.0f   P50' % (k, a, b, b - a))
    a, b = band(off, k), band(on, k)
    print('%-10s %11.0f %11.0f %11.0f   band 41-120' % (k, a, b, b - a))

# Paired: same ROM, same seed, deterministic instrument, so row i is the same
# presented frame on both arms. Compare FRAMES, not ranks.
for k in ('WORK-H', 'FTR'):
    d = [on[i][k] - off[i][k] for i in range(1600)]
    wins = sum(1 for x in d if x < 0)
    print('%-10s paired median %+d, mean %+.0f, wins %d/1600'
          % (k, st.median(d), sum(d) / 1600.0, wins))

c80, k80 = rank(off, 'WORK-H'), rank(on, 'WORK-H')
moved = c80 - k80
print('')
print('CONTROL   rank-80 %d raw / %d net   level %+d   band 41-120 %.0f'
      % (c80, c80 - APP, c80 - APP - GATE, band(off, 'WORK-H')))
print('CANDIDATE rank-80 %d raw / %d net   level %+d   band 41-120 %.0f'
      % (k80, k80 - APP, k80 - APP - GATE, band(on, 'WORK-H')))
print('MOVED     %d at rank-80 on this binary' % moved)
print('')
print('Applied to the banked c220 basis (rank-80 %d, level +%d):'
      % (BASIS_RANK80, BASIS_LEVEL))
print('  new rank-80 %d, NEW REQUIREMENT +%d'
      % (BASIS_RANK80 - moved, BASIS_LEVEL - moved))
