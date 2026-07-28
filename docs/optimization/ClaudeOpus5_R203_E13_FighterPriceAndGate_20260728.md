# R2-03 E13 — what a fighter actually costs, and where R2-03 has not been looking

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** Measurement. R2-03's gate is **missed by 2.00x**, and **67% of the
fighter draw has never been split**.

## 1. Why this ran

The owner reported, unprompted: *"when i knock fox out of screen so he doesn't
render, FPS goes up to 29 FPS, a huge jump."*

Nothing in the fighter draw path is visibility-conditional, so on the campaign's
own model that observation should have been impossible. Either the model was
wrong about where fighter cost goes, or the frame is not bound by what the
campaign has been measuring. Both are worth more than another cut inside
`MatrixPrep`.

## 2. What one fighter costs

Two arms, identical source, `NDS_R2_DRAW_SUPPRESS_MASK` 0 versus 2, tick-HUD ROM,
128 presented frames each.

| | A: both fighters | C: Fox draw suppressed | delta |
|---|---:|---:|---:|
| WORK P50 | 1,160,704 | 889,280 | **-271,424** |
| WORK P95 | 1,573,312 | 1,423,872 | -149,440 |
| FTR P50 | 553,600 | 284,416 | -269,184 |
| WAIT P50 | 246,720 | 271,232 | +24,512 |
| ALL P50 | 1,679,680 | 1,119,808 | -559,872 (one VBlank) |
| VBlank interval | 2:217 3:333 4:13 5+:3 max 18 | 2:458 3:102 4:5 5+:1 max 17 | |

Engagement is self-proving: `gNdsFighterDLAllDrawP1HardwareTriangleCount` goes
173,502 -> 0 and `gNdsTask91DrawCalls` 1,134 -> 567.

**The owner's observation reproduces exactly.** The median frame moves from three
VBlank intervals to two -- 20 FPS to 30 FPS -- and the 2-interval share goes from
38% of frames to 81%. One fighter's draw is 271,424 ticks of a 1.68M frame.

## 3. The frame is CPU-bound, and this is the measurement that says so

The obvious reading of the owner's report is that the DS rasterizer is the
bottleneck and an off-screen fighter stops costing pixels. The A/C pair refutes
it without needing a separate arm:

**WAIT went up when Fox stopped drawing** (246,720 -> 271,232). A rasterizer-bound
frame that loses a quarter of its pixel load waits *less*. A CPU-bound frame that
loses 271,424 ticks of ARM9 work finishes earlier and waits *longer*, which is
what happened, and the whole 559,872-tick fall in `ALL` is one VBlank period --
the frame crossed a quantum boundary because the CPU got there sooner.

## 4. R2-03's gate: missed by 2.00x

Fighter draw partitioned on the control arm, 479 frames, 958 draw calls, both
fighters, per presented frame:

| phase | ticks/frame | share |
|---|---:|---:|
| Walk | 3,338 | 0.7% |
| Validate | 11,520 | 2.3% |
| Reset | 6,700 | 1.3% |
| MatrixPrep | 120,407 | 24.0% |
| — of which world matrices | 101,683 | 20.3% |
| MaterialPrep | 22,092 | 4.4% |
| **Submit (residual)** | **336,030** | **67.1%** |
| **total** | **500,833** | |

The switch plan's §7 budget for fighter rendering is **250,000 for both
fighters combined**. Measured: **500,833**. Over by 250,833, a factor of 2.00.

Per fighter: Mario 237,219 per draw call measured directly on arm C, Fox
therefore ~267,988. Either fighter alone very nearly exhausts the budget written
for the pair.

That budget was set in R2-00b without a measured per-fighter cost. It is not
wrong to keep it -- it is what 30 FPS needs -- but it should be recorded that
nothing ever validated it against an actual fighter, and the actual fighter is
twice it.

## 5. The 67% nobody has measured

R2-03 has shipped two cuts, E9+E10 (-14,762) and E12 (-32,724): **-47,486
against a 250,833 gap, 19% of it.** Both landed in `MatrixPrep`, which is 24% of
the fighter draw.

The submit residual -- everything after owner preparation closes -- is
**336,030 ticks/frame, 2.8x MatrixPrep**, and has never been bracketed. E2/E3
split the outer function and stopped at the point the owner inputs are built;
everything past that has been charged to a single unnamed remainder ever since.

For scale: ~626 hardware triangles per frame across both fighters (Mario 320,
Fox 306). 336,030 ticks over 626 triangles is **537 ticks per triangle** of
submission. That number is not credible as pure FIFO traffic and is the reason
the next experiment is a split rather than a cut.

This is R2-02 E3's lesson recurring at the next level down, and E12's lesson
restated: *when the measured phases do not add up to the bracket, the answer is
in the residual.* R2-03 has been optimizing the 24% it had already instrumented
because it was instrumented.

## 6. A refuted instrument, recorded because it nearly wasn't caught

The first attempt at splitting CPU cost from pixel cost translated the fighter
20,000 units out of the view volume, so the ARM9 submission would stay
byte-identical while the rasterizer's share disappeared. It reported -13,632
WORK P50 -- a tidy "the rasterizer is 5% of a fighter" result, just above the
5,000-7,000 build-placement floor, entirely plausible, and **false**.

The screenshot showed Fox still standing next to Mario. Writing
`root->translate` does not reach the hardware: the per-frame DObj world matrix
cache serves the matrix it already built. Moving the injection to the prepared
modelviews in `ndsRendererAdapterPrepareNativeOwnerMatrices` did not reach it
either -- Fox drew again -- so the production submit path does not consume those
either, and the arm was deleted rather than debugged, because arms A and C
already answered the question §3 asks.

Two rules follow, and both are in `TASK_STANDING_RULES.md`:

- **A probe that moves geometry proves engagement with a screenshot, not with a
  tick delta.** A delta near the placement floor is exactly what a
  silently-inert probe produces, and it reads as a finding.
- Arm C's engagement proof cost nothing because it was structural -- a triangle
  count that must go to zero. **Prefer a probe whose engagement is a counter
  that cannot be ambiguous.**

## 7. Disposition

`NDS_R2_DRAW_SUPPRESS_MASK` is kept, default 0. It is how a whole fighter gets
priced, R2-05 will want it for the next fighter, and it is five lines with a
self-proving engagement counter. The offscreen mask is deleted.

Next: **R2-03 E14 — bracket the 336,030-tick submit.** No cut should be proposed
inside the fighter until its two-thirds is named.
