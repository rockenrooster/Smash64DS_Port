"""P50/P95/P99 of WORK (frames >= 64) plus chosen extras for sweep arms."""
import json
import sys

BASE = "D:/Stuff/DevFolder/Smash64DS_Port/artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/"
WANT = sys.argv[2].split(",") if len(sys.argv) > 2 else []


def pct(values, p):
    s = sorted(values)
    k = max(0, min(len(s) - 1, int(round(p / 100.0 * len(s) + 0.5)) - 1))
    return s[k]


for arm in sys.argv[1].split(","):
    d = json.load(open(BASE + arm + ".json"))
    names = d["bucketNames"]
    wi = 1 + names.index("WORK")
    work = [r[wi] for r in d["rows"] if r[0] >= 64]
    ex = {e["name"]: e["value"] for e in d["extras"]}
    shown = " ".join(f"{n.replace('gNds', '')}={ex.get(n)}" for n in WANT if n in ex)
    print(f"{arm:10s} n={len(work)} P50={pct(work, 50):,} P95={pct(work, 95):,} "
          f"P99={pct(work, 99):,} mean={sum(work) / len(work):,.0f} "
          f"vbi2={d['vbi2']} vbi3={d['vbi3']} {shown}")
