"""Re-bank the tick arm from a sample-tick-hud-buckets rows CSV.

    python artifacts/performance/2026-08-17_ship-cadence/rebank.py \
        artifacts/performance/2026-08-17_ship-cadence/c246-rows.csv \
        [artifacts/performance/2026-08-17_itcm-repack2/c239-rows.csv]

The first CSV is the candidate, the second (optional) the basis it is compared
against. Reports the percentiles the board banks -- P50, P90, rank-80 raw and
net, band 41-120, top-1%, max, and the over-gate count -- plus the requirement
against the 1,120,380 tick gate.

`WORK-H` is `WORK` minus the tick HUD's own draw. `net` additionally subtracts
the 24,947 apparatus constant that only the tick-HUD instrument pays, which is
the number the board banks. rank-80 is the 80th-largest row of the 1,600-sample
window, i.e. its P95.
"""
import csv
import sys

GATE = 1_120_380
APPARATUS = 24_947


def load(path):
    rows = []
    with open(path, newline="") as handle:
        for row in csv.DictReader(handle):
            try:
                rows.append(int(row["WORK-H"]))
            except (KeyError, ValueError):
                continue
    return sorted(rows, reverse=True)


def report(label, sorted_desc):
    n = len(sorted_desc)
    if n == 0:
        print("%s: no rows" % label)
        return None
    rank80 = sorted_desc[79] if n >= 80 else sorted_desc[-1]
    band = sorted_desc[40:120] if n >= 120 else sorted_desc
    # Even n: the board's P50 is the mean of the two central rows (reproduced
    # against ITCM_REPACK2.md's 841,024 from 841,088 and 840,960).
    p50 = (sorted_desc[(n - 1) // 2] + sorted_desc[n // 2]) // 2
    p90 = sorted_desc[max(0, n // 10 - 1)]
    top1 = sorted_desc[max(0, n // 100 - 1)]
    # The board counts over-gate on RAW WORK-H, not on the net figure: the
    # apparatus subtraction is applied to the banked percentile, not to the
    # per-frame test. Reproduced against ITCM_REPACK2.md's 88.
    over = sum(1 for v in sorted_desc if v > GATE)
    print("%-28s n=%d" % (label, n))
    print("  P50            {:>12,}".format(p50))
    print("  P90            {:>12,}".format(p90))
    print("  rank-80 raw    {:>12,}".format(rank80))
    print("  rank-80 net    {:>12,}   (raw - {:,} apparatus)".format(
        rank80 - APPARATUS, APPARATUS))
    print("  band 41-120    {:>14.1f}".format(sum(band) / float(len(band))))
    print("  top-1%         {:>12,}".format(top1))
    print("  max            {:>12,}".format(sorted_desc[0]))
    print("  over gate      {:>12,}  of {}   (raw > {:,})".format(
        over, n, GATE))
    print("  REQUIREMENT    {:>+12,}  (rank-80 net - gate)".format(
        rank80 - APPARATUS - GATE))
    return {"p50": p50, "p90": p90, "rank80": rank80,
            "net": rank80 - APPARATUS, "top1": top1, "max": sorted_desc[0],
            "over": over}


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    cand = report(argv[1].split("/")[-1], load(argv[1]))
    if len(argv) > 2:
        print()
        base = report(argv[2].split("/")[-1], load(argv[2]))
        if cand and base:
            print("\ndelta candidate - basis:")
            for key in ("p50", "p90", "rank80", "net", "top1", "max", "over"):
                print("  {:<8} {:>+12,}".format(key, cand[key] - base[key]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
