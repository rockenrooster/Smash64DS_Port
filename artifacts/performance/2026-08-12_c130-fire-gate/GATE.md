# R2-07 rows 2 and 3 — the performance check the fixes owed

`BUG_FIXING_PROCESS.md` owes a performance A/B when active-frame work changed.
One of these two fixes changes it: admitting texture 12 to the quad atlas means
FlameLR particles now **draw** instead of taking `lbparticle.c:3698`'s
draw-nothing `continue`. That is new per-frame work, it is what the source does,
and it must be priced rather than assumed free.

## The measurement

`builds/build-c130-fire-bothcpu`, `NDS_R2_BOTH_CPU=1` (**R2-07's gate arm**),
1600 samples from frame 438, `-RingDump`, DLDI **ON**, `slips=0`,
`git=458df8ad8e0+dirty(4)`.

| bucket | P50 | P95 |
|---|---:|---:|
| `ALL` | 1,118,208 | 1,678,784 |
| `WORK` | 953,280 | 1,390,016 |
| **`WORK-H`** | **944,256** | **1,217,472** |
| `FTR` | 303,232 | 329,472 |
| `SRC` | 323,648 | 580,992 |

VBI 2:1697 3:313 4:15 5+:13, max 26, total 2038.
Raw: `c130-gate.json`, `c130-gate-rows.csv`, `c130-gate.log`.

## Against the bank, and why the difference is not the fix

The comparable figure is the **both-CPU** bank, not the Boundary one:
`build-c126-bothcpu` measured `WORK-H` P50 938,368 / P95 **1,207,616**
(`…/2026-08-12_c126-armcheck/ARM_MISLABEL.md`). So c130 reads **P95 +9,856
(+0.8%)** and **P50 +5,888 (+0.6%)**.

**Both differences are inside this arm's own measurement floor.** The arm-check
run recorded three `BOTH_CPU 1` readings of **1,197,952 / 1,207,168 / 1,207,616**
— a 9,664 spread with no source change at all — and the campaign's cross-build
placement floor is ±8,544. +9,856 sits on top of both.

**The mechanism is an order of magnitude too small to be the cause**, which is
the stronger argument, because "inside the noise" alone would leave it open.
Counted rather than estimated: across the six flame creations both arms produce,
`gNdsParticleQuadEmitCount` advanced **3324 → 3332 on the control and 3324 →
3350 on the candidate** — **18 extra quads over roughly twelve frames**, about
1.5 additional textured quads per burning frame. At any plausible per-quad cost
that is hundreds of ticks on the frames that carry a burn, not ~10,000 on the
95th percentile of the whole match. A burn also occupies a small minority of the
1600-frame window, so it cannot move P50 by 5,888 either.

**Verdict: no meaningful P1 performance regression.** The gate arm's standing
failure is unchanged and unrelated — it was **+87,236 over** before these fixes
and is +97,092 over at this reading, still inside the same floor. `HANDOFF.md`'s
banner and the owner's "bugs first, then P95" ordering both still apply.
