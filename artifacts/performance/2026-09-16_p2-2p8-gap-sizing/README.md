# P2-2p8 gap sizing against the locked-30 gate

Measured 2026-09-16 on the clean, post-deletion canonical baseline
(`build-p2-fourcpu-tickhud`, 1,972 samples, four fighters, Dream Land).
This is a **sizing record, not a candidate**. It exists because the campaign has
been selecting leaf-level levers without a written statement of how far they can
possibly go, and the answer changes what should be worked on next.

## The gate

`1,120,000` ticks is two VBlank intervals. This run's `ALL` P50 is 1,678,016
across exactly three intervals, so one interval is **559,339 ticks** and two is
**1,118,678** — the 1,120,000 figure is the locked-30 frame budget, not an
arbitrary constant. A frame fits 30 FPS when its work fits two intervals.

## Where the four-fighter arm actually sits

| | ticks | vs gate |
|---|---|---|
| WORK-H P50 | 1,575,296 | **1.41x** |
| WORK-H P95 | 2,311,616 | **2.06x** |
| WORK-H max | 6,373,568 | 5.69x |

**1,821 of 1,972 frames — 92.3% — exceed the gate. 151 frames, 7.7%, fit.**

Closing it needs **-455,296** ticks from the median frame (28.9% of it) and
**-1,191,616** from P95 (51.6% of it).

## What the known levers are worth

Measured on this same build, per frame:

| lane | tk/fr | share of the P50 gap |
|---|---|---|
| Entire `__aeabi_fadd` + `__aeabi_fmul` class | 90,169 | 19.8% |
| …of which the collision matrix family | ~49,900 | 11.0% |
| …of which `func_ovl2_800ED490` alone | 16,940 | 3.7% |
| Pose subsystem (Play + Parse + Update) | ~63,800 | 14.0% |
| `memset` + `memcpy` | ~43,300 | 9.5% |
| Every N04.0x code lever so far, combined | ~20,000 | 4.4% |
| The 2026-09-16 clean rebuild (build hygiene) | 50,432 | 11.1% |

**Deleting every soft-float operation in the frame closes under a fifth of the
median gap and under a tenth of the P95 gap.** Campaign 12's whole measured
simulation reservoir, 87,085 tk/fr on the strictly-simulation subsystems, is the
same order. No combination of the lanes above reaches 455,296, let alone
1,191,616.

## What follows

This is not an argument to stop optimizing; the levers above are real and several
have banked. It is an argument that **P2-2p8 as currently specified cannot be
closed by the kind of work the campaign has been selecting**, and that continuing
to pick the next-largest leaf will keep producing 5,000-to-15,000-tick results
against a 455,296-tick requirement.

Three directions, and the choice is the owner's:

1. **Structural reduction in per-fighter work at four players.** The two-fighter
   shipping shell measures 26.4 FPS on the same build, close to target, while the
   four-fighter arm is 1.41x over at the median. The cost is not uniformly
   distributed overhead — it scales with fighter count. A lever that changes what
   each additional fighter costs (draw-call structure, per-fighter LOD, shared
   pose/packet work) is the only class sized to the gap.
2. **Re-scope the four-fighter target.** 1,120,000 is the locked-30 budget and
   the natural product target, but whether *four-player* SSB64 is required to
   hold 30 on DS hardware is a scope question, not a measurement. The source
   game's own four-player behaviour is the reference, and this repo has not
   recorded it. Worth establishing before spending more cycles against a number
   that may not be the right one.
3. **Accept P2-2p8 RED as a known state** and let P2-3/4/5/6/7 acceptance
   proceed, with four-player performance tracked rather than gating.

## Method

`artifacts/verification/p2-2-fourcpu-tickhud.csv`, the per-frame WORK-H column
from the run that produced the current checkpoint. Frame counts are direct, not
modelled. Lane figures come from
`artifacts/performance/2026-09-16_p2-2p8-n0409-profile/` with ticks computed as
`cycles / (2 * regions)`.
