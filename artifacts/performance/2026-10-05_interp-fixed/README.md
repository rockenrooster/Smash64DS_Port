# 2026-10-05 Path arc length in fixed point; the float kernel and its tables deleted

Owner, 2026-10-05: "Software floating point should not exist, fixed point
only ... Delete Old machinery." Ruling D13 allows a digest re-baseline; none
was needed. Clean four-CPU lab (`NDS_LAB_FOURCPU_WORDS=1`), items on, Sector Z
(`gNdsLabFourCpuGkind=1`); WORK-H from the ring dump, frames >= 64
(`runsum.py`); paired figures from `pairab.py` (WORK, frames >= 60).

## What changed

syInterpGetFracFrame's Bezier/Catrom arm -- the arc-length reparametrisation
of a moving path: Sector Z's Arwing (a platform, so gameplay state on every
tick it flies), Samus's rolls, the Board the Platforms boards -- was the
largest soft-float site in the Sector Z census (`ndsIxQuart` 302 calls a frame,
~23K ticks of library calls plus the square roots). It ran as a bit-exact
binary32 kernel over library multiplies and adds, backed by a 103 KB captured
Arwing flight table (it needed 201 KB free at stage setup) and a scene-heap
result table of up to 64 KB.

`include/nds/nds_interp_fixed.h` keeps the source's algorithm -- bisection on
[0,1], nine-sample Simpson integrals of sqrt(quartic), the 1e-5 tolerances and
both exit tests -- in integers: Q22 frames (every node and sample exact), one
even power-of-two scale for the five coefficients (Q30 Horner on SMULL), the
math unit's 64-bit sqrt started before the next sample's Horner and collected
after it, one int64 unit for integrals, time_scale and the tolerance (no
divide in the loop), and the final (id + frac) / (points_num - 1) rounded to
nearest even from the hardware divider. From depth 5 a node spans at most
2^-6 of the segment, where one Simpson panel matches four to ~1e-9, so deep
nodes take three samples (one or two new).

Host proof `scripts/test_interp_fixed.c`: the quotient matches the float
divide on 126,000 frames; random Bezier segments at scales 2^-8..2^16 match the
float source bit for bit on 99.73-99.99% of calls (walked as the game walks
them); a captured Sector Z Arwing path (not committed: ROM data) 99.75%, the
rest within the source's own tolerance. The match rate is the same with and
without the three-sample deep nodes.

Deleted: `include/nds/nds_interp_exact.h` (539 lines) and its host proof, the
float replica, the run-time oracle and capture buffer, the Arwing flight table
(`assets/stages/sector_arwing_frac.bin`, its generator, loader and Makefile
rule) and the scene-heap result table.

## Measurements

| Run | Build | P50 | P95 | over |
|---|---|---|---|---|
| `sz-q7-p0` | float kernel + tables (word 0) | 892,608 | 1,216,576 | 186 |
| `sz-q7` / `sz-q7b` | fixed, first cut | 886,976 / 887,488 | 1,209,152 / 1,198,208 | 167 / 165 |
| `sz-q9` | + pipelined sqrt, result table kept | 886,144 | 1,186,880 | 160 |
| `sz-q9-nm` | same, result table off | 884,800 | 1,186,240 | 160 |
| `sz-q10` | + three-sample deep nodes, table deleted | 884,480 | 1,188,032 | 155 |
| `gate-q9` | Dream Land | 799,232 | 1,088,512 | 76 |
| `gate-q10` | Dream Land | 799,744 | 1,086,016 | 75 |

- Every Sector Z run's digest is identical to the float kernel's, and the
  gate's to Q6's.
- Float kernel -> q10: paired P95 1,228,416 -> 1,201,536 (-26.9K), P50 -8.8K.
  A fresh flight's frames cost ~60K less; a repeated flight's, where the old
  flight table answered, ~8K more (q9 without deep nodes: ~10K more).
- The result table bought nothing in front of the fixed arm (P95 +4.9K with
  it, mean +0.4K) and cost up to 64 KB of scene heap.
- Gate q6 -> q10: digest identical, P95 -2.1K (paired).

## Sector Z over the gate, after (profile `artifacts/task37-census/sz-szprof09`)

384 frames from frame 200, 34 over two VBlanks. Their premium over the other
350 (ARM9 cycles, idle wait excluded) is texture resolve and upload
(`ndsRendererHardwareResolveOrBindTexture` +71K in 18 frames), lean
re-materialisations (~100K in 7 frames) and status-change setup (pose parse
and bind, AObj normalisation, reloc lookups); soft float is ~15K of it.
