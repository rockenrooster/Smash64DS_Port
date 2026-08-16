# POSITION.md section 3: paired per-frame bucket deltas for the two GX_COMPOSE
# build pairs, plus the Task 103 stage-phase counters.
# Run from anywhere: paths resolve relative to this file.
import csv, os, statistics, math

def load(p):
    rows = {}
    with open(p, newline='') as f:
        for row in csv.DictReader(f):
            rows[int(row['frame'])] = {k: int(v) for k, v in row.items() if k != 'frame'}
    return rows

_here = os.path.dirname(os.path.abspath(__file__))
b1 = os.path.join(_here, '..', '2026-08-16_gxcompose-bank-basis')
b2 = os.path.join(_here, '..', '2026-08-16_gx-transfer-locate')

pairs = [
    ("UNINSTRUMENTED  c206-shipgx0 (GX0) -> c207-gx1 (GX1)",
     b1 + '/c206-shipgx0-rows.csv', b1 + '/c207-gx1-rows.csv'),
    ("TASK103-INSTRUMENTED  c208-t103-gx0 -> c209-t103-gx1",
     b2 + '/c208-t103-gx0-rows.csv', b2 + '/c209-t103-gx1-rows.csv'),
]

def pct(v, q):
    v = sorted(v); k = (len(v)-1)*q
    lo, hi = int(math.floor(k)), int(math.ceil(k))
    return v[lo] if lo == hi else v[lo] + (v[hi]-v[lo])*(k-lo)

for label, pa, pb in pairs:
    A, B = load(pa), load(pb)
    fr = sorted(set(A) & set(B))
    print(f"\n=== {label}   n={len(fr)} ===")
    print(f"{'bucket':16} {'A P50':>10} {'B P50':>10} {'dP50':>8} {'pairedMed':>10} {'B>A':>6} {'B<A':>6}")
    def dv(d):
        e = dict(d)
        e['DRAW=F+S+B+H+M'] = d['FTR']+d['STG']+d['BG']+d['HUD']+d['MISC']
        e['SIM=SRC'] = d['SRC']
        return e
    for c in ['FTR','STG','MISC','DRAW=F+S+B+H+M','SIM=SRC','GCRA','SINT','WORK-H','ALL']:
        a = [dv(A[f])[c] for f in fr]; b = [dv(B[f])[c] for f in fr]
        d = [y-x for x, y in zip(a, b)]
        print(f"{c:16} {statistics.median(a):10,.0f} {statistics.median(b):10,.0f} "
              f"{statistics.median(b)-statistics.median(a):8,.0f} {statistics.median(d):10,.0f} "
              f"{sum(1 for x in d if x>0):6d} {sum(1 for x in d if x<0):6d}")
    r80a = sorted([A[f]['WORK-H'] for f in fr], reverse=True)[79]
    r80b = sorted([B[f]['WORK-H'] for f in fr], reverse=True)[79]
    print(f"rank-80 WORK-H: A {r80a:,}  B {r80b:,}  B-A {r80b-r80a:+,}")

# Task103 whole-match counters, 2038 frames, cumulative
FRAMES = 2038
A103 = dict(PrepareTicks=125618688, PrepareCount=2038, TraversalTicks=2938048, TraversalCount=8,
            DisplayTicks=326960576, DisplayCount=55890, FinishTicks=950784, FinishCount=2037,
            BeginTicks=57934912, BeginEndBatchTicks=5395584, PushTicks=21618240,
            WordCount=7972976, RunCount=67188, TailTicks=7662720,
            CommitTicks=314566848, CommitCount=16296)
B103 = dict(PrepareTicks=130706304, PrepareCount=2038, TraversalTicks=2959872, TraversalCount=8,
            DisplayTicks=318491392, DisplayCount=55890, FinishTicks=963392, FinishCount=2037,
            BeginTicks=54151104, BeginEndBatchTicks=5833472, PushTicks=21638016,
            WordCount=7972976, RunCount=67188, TailTicks=7516288,
            CommitTicks=305463424, CommitCount=16296)
print(f"\n=== Task 103 stage-phase counters, cumulative over {FRAMES} presented frames ===")
print(f"{'counter':22} {'GX0 total':>16} {'GX1 total':>16} {'GX0 /frame':>12} {'GX1 /frame':>12} {'d/frame':>10}")
for k in A103:
    a, b = A103[k], B103[k]
    print(f"{k:22} {a:16,} {b:16,} {a/FRAMES:12,.1f} {b/FRAMES:12,.1f} {(b-a)/FRAMES:+10,.1f}"
          + ("   IDENTICAL" if a == b else ""))
sa = A103['PrepareTicks']+A103['TraversalTicks']+A103['DisplayTicks']+A103['FinishTicks']
sb = B103['PrepareTicks']+B103['TraversalTicks']+B103['DisplayTicks']+B103['FinishTicks']
print(f"\nfour STG sites summed: GX0 {sa/FRAMES:,.0f} tk/fr   GX1 {sb/FRAMES:,.0f} tk/fr   d {(sb-sa)/FRAMES:+,.0f}")
print(f"STG bucket own mean:   GX0 223,084          GX1 221,492          d {221492-223084:+,}")
