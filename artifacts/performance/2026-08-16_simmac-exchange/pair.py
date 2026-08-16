# Paired per-frame WORK-H deltas between shadow arms of ONE binary.
#
# Same binary, one .data word apart, so there is no placement term and the
# >=14,080 rank-80 / ~5,700 P50 cross-build floors do not apply. What DOES
# survive is the cartridge-read frames CAMERA_Q20_12.md section 3.2 measured:
# the paired delta's extremes reach +/-150k on frames whose cost is not
# reproducible between two emulator sessions, and rank-80 sits inside that
# contaminated band. The paired per-frame MEDIAN is immune to it by
# construction and is the statistic this prints first.
#
# Usage: python pair.py <arm0.rows.csv> <armN.rows.csv> [label]
import csv, math, statistics, sys


def load(path):
    rows = {}
    with open(path, newline='') as handle:
        for row in csv.DictReader(handle):
            rows[int(row['frame'])] = {k: int(v) for k, v in row.items()
                                       if k != 'frame'}
    return rows


def pct(values, q):
    values = sorted(values)
    k = (len(values) - 1) * q
    lo, hi = int(math.floor(k)), int(math.ceil(k))
    return values[lo] if lo == hi else values[lo] + (values[hi] - values[lo]) * (k - lo)


def main():
    a_path, b_path = sys.argv[1], sys.argv[2]
    label = sys.argv[3] if len(sys.argv) > 3 else f"{a_path} -> {b_path}"
    A, B = load(a_path), load(b_path)
    frames = sorted(set(A) & set(B))
    print(f"=== {label}   paired frames n={len(frames)} ===")
    print(f"{'bucket':10} {'A P50':>11} {'B P50':>11} {'dP50':>9} "
          f"{'pairedMed':>10} {'pairedMean':>11} {'B>A':>6} {'B<A':>6} {'B=A':>6}")
    for bucket in ['WORK-H', 'WORK', 'ALL', 'SRC', 'FTR', 'STG', 'GCRA',
                   'SINT', 'SPHD', 'MISC', 'OTHR']:
        if bucket not in A[frames[0]]:
            continue
        a = [A[f][bucket] for f in frames]
        b = [B[f][bucket] for f in frames]
        d = [y - x for x, y in zip(a, b)]
        print(f"{bucket:10} {statistics.median(a):11,.0f} {statistics.median(b):11,.0f} "
              f"{statistics.median(b) - statistics.median(a):9,.0f} "
              f"{statistics.median(d):10,.0f} {statistics.mean(d):11,.1f} "
              f"{sum(1 for x in d if x > 0):6d} {sum(1 for x in d if x < 0):6d} "
              f"{sum(1 for x in d if x == 0):6d}")
    a = [A[f]['WORK-H'] for f in frames]
    b = [B[f]['WORK-H'] for f in frames]
    d = sorted(y - x for x, y in zip(a, b))
    print()
    print("WORK-H paired-delta distribution: "
          + "  ".join(f"P{int(q*100)} {pct(d, q):+,.0f}"
                      for q in (0.05, 0.25, 0.5, 0.75, 0.95)))
    print(f"WORK-H paired-delta min {d[0]:+,}  max {d[-1]:+,}  "
          f"trimmed mean (drop 40 each tail) {statistics.mean(d[40:-40]):+,.1f}")
    for tag, series in (('A', a), ('B', b)):
        s = sorted(series, reverse=True)
        print(f"{tag}: P50 {pct(series,0.5):,.0f}  P90 {pct(series,0.9):,.0f}  "
              f"rank-80 {s[79]:,}  mean {statistics.mean(series):,.0f}  "
              f"over-gate(>1,120,380 net of 24,947 apparatus) "
              f"{sum(1 for v in series if v - 24947 > 1120380)}")
    ra = sorted(a, reverse=True)[79]
    rb = sorted(b, reverse=True)[79]
    print(f"rank-80 WORK-H: A {ra:,}  B {rb:,}  B-A {rb-ra:+,}")


main()
