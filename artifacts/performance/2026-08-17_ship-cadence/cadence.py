"""Cadence arithmetic for the shipping-configuration reads.

Reads the `PCADHIST ... DONE` line out of each probe-present-cadence.ps1
capture in this directory and prints the two-VBlank fraction against the
owner's >=95% arm. No emulator, no build: run it from the repo root as

    python artifacts/performance/2026-08-17_ship-cadence/cadence.py

The denominator is the guest's own `pres` counter read at
`mnVSResultsStartScene`, i.e. after the battle loop has fully exited, so it
covers every presented frame of the match rather than a sampler window.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

ARMS = [
    ("control-c240-cadence.txt",
     "c240 tick-HUD DRAW=0  (control, banked 94.90%)"),
    ("c241-cadence.txt",
     "c241 TICK_HUD=0, BATTLEPACK=1 KEEP_CACHE=1"),
    ("c242-cadence.txt",
     "c242 TICK_HUD=0, shipping defaults (pack off)"),
    ("c244-cadence.txt",
     "c244 shipping defaults, BOTH_CPU=0 (Boundary arm)"),
    ("c245-cadence.txt",
     "c245 PUBLISHED target, BOTH_CPU=1 (the gate)"),
    # 2026-08-17, after the owner-approved default flip. Same target, same
    # stress arm, same invocation as c245 -- the generated configs differ from
    # the published target's by exactly one line, NDS_R2_BOTH_CPU 0 -> 1 -- on a
    # tree where NDS_R2_BATTLEPACK and ..._KEEP_CACHE now default to 1. So
    # c245 -> c247 is the battlepack pair isolated on the SHIPPING binary.
    ("c247-cadence.txt",
     "c247 PUBLISHED target, BOTH_CPU=1, PACK ON (flipped)"),
]

DONE = re.compile(
    r"PCADHIST DONE .*?2=(\d+) 3=(\d+) 4=(\d+) 5=(\d+) max=(\d+) min=(\d+) "
    r"viol=(\d+) vbl=(\d+) pres=(\d+)")


def read(path):
    if not os.path.exists(path):
        return None
    with open(path, errors="ignore") as handle:
        for line in handle:
            m = DONE.search(line)
            if m:
                return [int(g) for g in m.groups()]
    return None


def main():
    rows = []
    print("%-46s %6s %5s %4s %4s %7s %9s %7s %6s %5s" % (
        "arm", "2", "3", "4", "5+", "total", "two-VBlank", "need", "margin",
        "max"))
    for name, label in ARMS:
        got = read(os.path.join(HERE, name))
        if got is None:
            print("%-46s  (no capture)" % label)
            continue
        two, three, four, five, mx, mn, viol, vbl, pres = got
        total = two + three + four + five
        if total != pres:
            print("  WARNING: bucket sum %d != presented %d (%s)" %
                  (total, pres, name))
        need = -(-95 * pres // 100)
        rows.append((label, two, three, four, five, pres, need, mx, viol))
        print("%-46s %6d %5d %4d %4d %7d %8.2f%% %7d %+6d %5d" % (
            label, two, three, four, five, pres, 100.0 * two / pres, need,
            two - need, mx))
        if viol != 0:
            print("  WARNING: %d cadence violations (interval < 2)" % viol)

    if len(rows) >= 2:
        print("\ndeltas against the c240 control, in frames:")
        base = rows[0]
        for row in rows[1:]:
            print("  %-44s two %+5d   3+ %5d -> %-5d" % (
                row[0], row[1] - base[1],
                base[2] + base[3] + base[4], row[2] + row[3] + row[4]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
