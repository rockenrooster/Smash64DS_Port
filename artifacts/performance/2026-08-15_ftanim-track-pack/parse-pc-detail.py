#!/usr/bin/env python3
"""Top PCs inside a function, annotated with the disassembly line."""
import csv, sys, re

PC_CSV, DIS, LO, HI, N = sys.argv[1], sys.argv[2], int(sys.argv[3], 16), int(sys.argv[4], 16), int(sys.argv[5])

dis = {}
rx = re.compile(r"^\s*([0-9a-f]+):\s+[0-9a-f ]+\t(.*)$")
with open(DIS, errors="replace") as fh:
    for line in fh:
        m = rx.match(line)
        if m:
            a = int(m.group(1), 16)
            if LO <= a <= HI:
                dis[a] = m.group(2).strip()

rows = []
tot = 0
with open(PC_CSV, newline="") as fh:
    for r in csv.DictReader(fh):
        pc = int(r["pc"], 16)
        if not (LO <= pc <= HI):
            continue
        mt = int(r["marg_total_cycles"]) - int(r["marg_halt_wait"])
        tot += mt
        rows.append((mt, pc, int(r["marg_instructions"]), int(r["marg_dcache_fill"]),
                     int(r["marg_icache_fill"]), int(r["marg_interlock"])))
rows.sort(reverse=True)
print("total marginal ticks/frame in range: %.0f" % (tot / 160.0))
print("%-11s %8s %6s %7s %7s %7s  %s" % ("pc", "tk/fr", "%", "ex/fr", "dfill", "ilock", "insn"))
for mt, pc, mi, df, icf, il in rows[:N]:
    print("0x%08x %8.0f %5.1f%% %7.1f %7.0f %7.0f  %s" %
          (pc, mt / 160.0, 100.0 * mt / tot, mi / 80.0, df / 80.0, il / 80.0,
           dis.get(pc, "?")))
