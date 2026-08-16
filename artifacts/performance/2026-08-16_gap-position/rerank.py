# POSITION.md sections 2, 4 and 6: exact re-rank of build-c206-shipgx0's own
# 1,600 rows under each candidate deletion. No build or emulator run needed.
# Run from anywhere: paths resolve relative to this file.
import csv, os

def load(p):
    rows = []
    with open(p, newline='') as f:
        for row in csv.DictReader(f):
            rows.append({k: int(v) for k, v in row.items()})
    return rows

base = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..',
                    '2026-08-16_gxcompose-bank-basis')
A = load(base + '/c206-shipgx0-rows.csv')   # GX=0 = the shipping renderer
APPARATUS = 24947
GATE = 1120380

def rank80(vals):
    return sorted(vals, reverse=True)[79]

base_r80 = rank80([r['WORK-H'] for r in A])
print(f"baseline rank-80 raw {base_r80:,}  net {base_r80-APPARATUS:,}  gap {base_r80-APPARATUS-GATE:+,}")
print()
print(f"{'intervention':46} {'new r80 raw':>12} {'moved':>9} {'new gap':>9} {'closes?':>8}")
print('-'*90)

def report(label, f):
    new = [f(r) for r in A]
    r = rank80(new)
    gap = r - APPARATUS - GATE
    print(f"{label:46} {r:12,} {base_r80-r:9,} {gap:+9,} {'YES' if gap<=0 else 'no':>8}")
    return base_r80 - r

# calibration: a strictly flat cut must move rank-80 by exactly its size
for k in (22608, 94481):
    report(f"FLAT cut of {k:,} on every frame", lambda r, k=k: r['WORK-H'] - k)

# the 60 Hz -> 30 Hz rung, derived on this tree instead of from Task 106
for frac, name in ((0.5, 'half'),):
    report(f"SRC {name} (simulation at 30 Hz, no compensation)",
           lambda r, frac=frac: r['WORK-H'] - int(r['SRC'] * frac))
    report(f"GCRA {name} (gcRunAll bodies only at 30 Hz)",
           lambda r, frac=frac: r['WORK-H'] - int(r['GCRA'] * frac))
    report(f"GCRA {name} but SCPU+SINT kept at 60 Hz",
           lambda r, frac=frac: r['WORK-H'] - int((r['GCRA'] - r['SCPU'] - r['SINT']) * frac))
    report(f"SINT {name} only (interrupt/physics half at 30 Hz)",
           lambda r, frac=frac: r['WORK-H'] - int(r['SINT'] * frac))
    report(f"SCPU {name} only (level-3 AI at 30 Hz)",
           lambda r, frac=frac: r['WORK-H'] - int(r['SCPU'] * frac))

# draw-side rungs, as level cuts of their measured size
for k, name in ((22608, 'stage no-Z band'), (12595, 'particle draw kernels'),
                (4736, 'camera Q20.12 chain (measured paired median)'),
                (9273, 'draw-side fixed point at the measured 1.70x'),
                (53215, 'ALL fidelity-neutral inventory at 100%'),
                (30480, 'fidelity-neutral AVAILABLE TODAY at 100%')):
    report(f"level cut {k:,} = {name}", lambda r, k=k: r['WORK-H'] - k)

# what fraction of SRC would have to go to close
lo, hi = 0.0, 1.0
for _ in range(40):
    mid = (lo + hi) / 2
    r = rank80([x['WORK-H'] - int(x['SRC'] * mid) for x in A])
    if r - APPARATUS - GATE <= 0: hi = mid
    else: lo = mid
print()
print(f"SRC fraction needed to close the gate alone: {hi*100:.1f}%")
print(f"  (that is {hi*2*100:.0f}% of the two logic updates a presented frame runs)")

# concentration of SRC on the frames that set the percentile
order = sorted(A, key=lambda r: r['WORK-H'], reverse=True)
print()
print("the rank-80 FRAME itself (the frame whose WORK-H is the gate reading):")
f80 = order[79]
for c in ['frame','WORK-H','SRC','GCRA','SCPU','SINT','SPHD','SHDT','FTR','STG','MISC','OTHR','WAIT']:
    print(f"   {c:8} {f80[c]:>12,}")
