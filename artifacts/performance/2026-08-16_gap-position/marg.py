# POSITION.md sections 4 and 5: per-bucket P50 and top-80 median on both
# uninstrumented arms, and the SRC concentration behind the 30 Hz rung.
# Run from anywhere: paths resolve relative to this file.
import csv, os, statistics

def load(p):
    rows = {}
    with open(p, newline='') as f:
        for row in csv.DictReader(f):
            rows[int(row['frame'])] = {k: int(v) for k, v in row.items() if k != 'frame'}
    return rows

base = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..',
                    '2026-08-16_gxcompose-bank-basis')
A = load(base + '/c206-shipgx0-rows.csv')   # GX=0, the shipping renderer
B = load(base + '/c207-gx1-rows.csv')

APPARATUS = 24947
GATE = 1120380

for name, D in (('c206-shipgx0 (GX=0, SHIPPING)', A), ('c207-gx1 (GX=1)', B)):
    fr = sorted(D)
    order = sorted(fr, key=lambda f: D[f]['WORK-H'], reverse=True)
    top80 = order[:80]
    r80 = D[order[79]]['WORK-H']
    print(f"\n=== {name} ===  frames {fr[0]}..{fr[-1]} n={len(fr)}")
    print(f"rank-80 WORK-H raw {r80:,}  net {r80-APPARATUS:,}  gap {r80-APPARATUS-GATE:+,}")
    print(f"{'bucket':10} {'P50':>10} {'top80 med':>10} {'top80/P50':>9} {'x gap(94,481)':>14}")
    for c in ['FTR','STG','BG','AUD','HUD','SRC','MISC','OTHR','WAIT','ALL','WORK-H',
              'GCRA','SCPU','SINT','SPHD','SCAT','SPRM','SHDT','SWRM','SPHC']:
        v = [D[f][c] for f in fr]
        t = [D[f][c] for f in top80]
        p50 = statistics.median(v); tm = statistics.median(t)
        print(f"{c:10} {p50:10,.0f} {tm:10,.0f} {tm/p50 if p50 else 0:9.2f} {tm/94481:14.3f}")

# The 30 Hz derivation: SRC covers TWO 60 Hz logic updates per presented frame.
fr = sorted(A)
order = sorted(fr, key=lambda f: A[f]['WORK-H'], reverse=True)
top80 = order[:80]
src_t = statistics.median([A[f]['SRC'] for f in top80])
gcra_t = statistics.median([A[f]['GCRA'] for f in top80])
print(f"\n30 Hz derivation on the SHIPPING arm, top-80 medians:")
print(f"  SRC  {src_t:,.0f}   half = {src_t/2:,.0f}  = {src_t/2/94481:.3f}x gap")
print(f"  GCRA {gcra_t:,.0f}   half = {gcra_t/2:,.0f}  = {gcra_t/2/94481:.3f}x gap")
print(f"  LADDER's Task 106 mechanism figure 119,744 = {119744/94481:.4f}x gap")

# whole-match means for the same two, for the record
print(f"  whole-match SRC mean {statistics.mean([A[f]['SRC'] for f in fr]):,.0f}"
      f"  median {statistics.median([A[f]['SRC'] for f in fr]):,.0f}")
