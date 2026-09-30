"""Flag fighters that leave the ground right after a platform switch-off.

Reads plat-<tag>.log files written by platprobe.ps1: an OFF block (the stage
switched a yakumono off) and the TICK blocks after it. For each fighter that
was grounded at OFF and is airborne on a following tick, print the y it stood
at and the y on each following tick; a jump of more than one tick of gravity is
the port's (0, 0) edge teleport.
"""
import re
import sys

SP = sys.argv[1]
ROW = re.compile(r"\s+P(\d) ga=(-?\d+) fl=(-?\d+) st=(-?\d+) x=(-?[\d.]+) y=(-?[\d.]+)")

for tag in sys.argv[2:]:
    blocks = []
    cur = None
    for line in open(f"{SP}/plat-{tag}.log", encoding="utf-8", errors="replace"):
        if line.startswith("OFF") or line.startswith("TICK"):
            cur = {"head": line.strip(), "rows": {}}
            blocks.append(cur)
            continue
        m = ROW.match(line)
        if m and cur is not None:
            p, ga, fl, st, x, y = m.groups()
            cur["rows"][int(p)] = (int(ga), int(fl), int(st), float(x), float(y))
    events = 0
    for i, b in enumerate(blocks):
        if not b["head"].startswith("OFF") or not b["rows"]:
            continue
        events += 1
        follow = []
        for nb in blocks[i + 1:]:
            if nb["head"].startswith("OFF"):
                break
            follow.append(nb)
        for p, (ga, fl, st, x, y) in sorted(b["rows"].items()):
            if ga != 0:
                continue
            nxt = [nb["rows"].get(p) for nb in follow if p in nb["rows"]]
            if not nxt or nxt[0][0] != 1:
                continue
            ys = " ".join(f"{r[4]:.1f}" for r in nxt)
            print(f"{tag} {b['head']} P{p} fl={fl} st={st} stood y={y:.1f} -> {ys}  "
                  f"first drop {y - nxt[0][4]:.1f}")
    print(f"{tag}: {events} switch-off events with fighters present")
