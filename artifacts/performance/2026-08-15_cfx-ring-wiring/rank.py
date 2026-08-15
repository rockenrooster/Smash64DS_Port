"""Rank the WHOLE distribution of two (or three) tick-HUD row CSVs.

Memory `rank-the-whole-distribution`: a top-14 view once said a change landed
the gate while all 128 frames said 34 -> 26 over-gate. P95 is a position in a
sorted list, so the list is what gets printed.

Usage:  python rank.py label=path.csv [label=path.csv ...]
"""
import csv
import sys


def load(path):
    work = []
    with open(path, newline="") as handle:
        for row in csv.DictReader(handle):
            work.append(int(row["WORK-H"]))
    return work


def pct(sorted_desc, p):
    """The harness's own convention, so these numbers are comparable to the
    bank: ascending index int(n*p) - 1, i.e. for n=1600 P95 is the 81st-largest
    value. Verified against build-c147-ctl, whose published P95 is 1,210,944 and
    whose 80th-largest -- the figure plan.md section 0 ranks against -- is
    1,212,224. Both are printed; they differ by one rank and 1,280 ticks."""
    n = len(sorted_desc)
    asc_idx = max(0, int(n * p) - 1)
    return sorted_desc[n - 1 - asc_idx]


def report(label, work):
    s = sorted(work, reverse=True)
    n = len(s)
    print(f"\n== {label}  n={n}")
    print(f"   P50 {pct(s,0.50):>10,}   P90 {pct(s,0.90):>10,}   "
          f"P95 {pct(s,0.95):>10,}   top-1% {pct(s,0.99):>10,}   "
          f"max {s[0]:>10,}")
    print(f"   rank-80 (the 80th-largest, plan.md section 0's figure) "
          f"{s[79]:>10,}")
    print("   rank  1..12 : " + " ".join(f"{v:,}" for v in s[:12]))
    print("   rank 70..90 : " + " ".join(f"{v:,}" for v in s[69:90]))
    return s


def main():
    arms = {}
    for arg in sys.argv[1:]:
        label, path = arg.split("=", 1)
        arms[label] = report(label, load(path))

    labels = list(arms)
    if len(labels) < 2:
        return
    base = labels[0]
    print("\n== deltas vs " + base + " (positive = the arm is CHEAPER)")
    print(f"   {'arm':<10}{'P50':>12}{'P90':>12}{'P95':>12}"
          f"{'rank-80':>12}{'top-1%':>12}{'max':>12}")
    for label in labels:
        a, b = arms[base], arms[label]
        row = [pct(a, 0.50) - pct(b, 0.50), pct(a, 0.90) - pct(b, 0.90),
               pct(a, 0.95) - pct(b, 0.95), a[79] - b[79],
               pct(a, 0.99) - pct(b, 0.99), a[0] - b[0]]
        print(f"   {label:<10}" + "".join(f"{v:>+12,}" for v in row))


if __name__ == "__main__":
    main()
