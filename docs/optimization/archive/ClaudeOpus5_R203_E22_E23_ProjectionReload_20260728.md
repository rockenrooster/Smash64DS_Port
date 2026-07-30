# R2-03 E22/E23 — 96.7% of the projection loads are redundant, and removing them is worth 0.27%

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** E22 measurement stands. **E23 implementation REVERTED** — engaged on
93.8% of loads, worth −3,008 FTR P50, under the placement floor. The per-root
matrix bracket is not redundancy, and its remainder is not the matrix load.

## 1. E22 — what the per-root matrix work actually is

The per-root bracket was the last unpriced item in the phase (~40,000/frame on
E15's inflated bracket, less E17's shipped 17,600). E21's rule pointed the
question: not *how often does it run* but *what does it re-write*.

Counters in `ndsRendererLoadHardwareSplitMatrices`, 480 presented frames,
frames 439..919, both fighters, `NDS_R2_FIGHTER_HW_MTX=1`:

| counter | per frame |
|---|---:|
| matrix loads called | 30.0 |
| **elided by the generation check** | **0.0** |
| loads performed | 30.0 |
| of those, identical projection **and** modelview | 0.0 (6 in 480 frames) |

The existing generation counter **never once elides a load**. Every root has a
distinct generation and a genuinely distinct matrix. On that reading the per-root
work has no redundancy at all, and the phase is simply 30 necessary loads.

## 2. That reading was wrong, in E21's own way

The loader writes **two** matrices per call, and only one of them is per-root.
Scoring them jointly reports "nothing is redundant" while one half may be
redundant every single time. Splitting the compare:

| counter | per frame | share of loads |
|---|---:|---:|
| loads performed | 30.0 | |
| **identical projection** | **29.0** | **96.7%** |
| identical modelview | 0.0 (6 in 480 frames) | 0.04% |

The modelview is genuinely per-root. **The projection is per-frame camera data
riding along with it**, re-pushed 29 extra times a frame. The single non-identical
projection each frame is the first load after the stage's own matrix work, which
is exactly right.

E21 warned about counting identity of the target rather than identity of the
write. This is the same error one level down: counting identity of the *call*
rather than of each *field the call writes*.

## 3. E23 — building it

Skipping the `GL_PROJECTION` half when unchanged. The memo is valid inside one
owner execute and cleared at its start; a frame-scoped memo would be unsound,
because `ndsRendererHardwareEmitIFCommonClouds`,
`ndsRendererNativeStageTask36LoadNoZProjection` and the mip debug pass all write
`GL_PROJECTION` without touching `sNdsRendererHardwareMatrixMode`, so the shared
protocol cannot see them. Nothing writes it between the first and last root of
one execute, which is the whole span the memo needs.

Both arms `NDS_R2_FIGHTER_HW_MTX=1`, tick-HUD ROM, ring dump, 128 samples,
frames 439..566:

| bucket | A (control) | B (candidate) | delta |
|---|---:|---:|---:|
| FTR P50 | 489,536 | 486,528 | **−3,008** |
| WORK P50 | 1,099,328 | 1,094,528 | −4,800 |
| WORK-H P50 | 1,093,504 | 1,092,032 | −1,472 |
| ALL P50 | 1,120,000 | 1,119,872 | −128 |

Engagement, read from the same run that produced the buckets:
**`gNdsR2ProjHoistSkipped=16,750`, `gNdsR2ProjHoistLoaded=1,114`** — the arm
skipped 93.8% of projection loads, matching E22's 96.7% within the memo's
per-execute reset.

**−3,008 is under the 5,000–7,000 build-placement floor.** It is also almost
exactly what the work is worth from first principles: 29 loads × (a 16-word
conversion + ~18 FIFO writes) ≈ 2,900 ticks. The two agree, which is why the
result is trusted rather than re-run — a third A samples emulator noise, and
guest ticks are deterministic, so it could not resolve a placement-floor
question.

## 4. Why it was reverted rather than kept

The gain is real and unmeasurable at once. Keeping it would put a permanent
64-byte matrix and a hot-path `memcmp` into the fighter loop in exchange for a
number the instrument cannot resolve from zero. That is E8's shape — the
local-matrix memo that looked obviously beneficial and cost **+16,301** — and
E8's lesson is that a compare on the hot path is not free merely because its hit
rate is high.

The standing rule to keep every repeatable correctness-preserving gain applies to
gains the instrument can *see*. This one cannot be distinguished from zero.

## 5. The durable finding, which is larger than the cut

**A redundancy's share is not its cost.** 96.7% of these writes were redundant and
eliminating all of them was worth 0.27% of the frame, because the work being
repeated is GX FIFO traffic — and E14 already established the FIFO is never
backpressured on this path (empty at both ends of 946/946 submissions). FIFO
writes are stores. Stores are cheap.

In this renderer, **"how often" is a bad proxy for "how much" whenever the
repeated work is FIFO traffic.** Three of the last four candidate cuts died on
some form of this, and it is now the first question to ask of any redundancy
count: *what does one of them cost?*

The corollary matters for R2-03's remaining work: the per-root bracket's ~40,000
is **not** the matrix load, which is now measured at roughly 3,000 for its
redundant half and cannot be much more than 6,000–8,000 in total. The rest is
`ndsRendererNativeBindProductionRoot`, `glStoreMatrix`, and
`ndsRendererNativeApplyRootLightPreamble` — and the light preamble is E16's
territory. **The last unpriced item resolves into E16, not beside it.**

## 6. Also established, for free

Over the same 480-frame window the candidate and control arms of **E17** emit
`gNdsFighterDLAllDrawP0HardwareTriangleCount = 136,640` — identical to the digit.
E17 changes no geometry whatsoever, which is a stronger structural check than the
one it shipped with. (E20/E21's "320/frame" was that quantity over a different
window; the rate over frames 439..919 is 284.7. Neither is wrong; the control
must simply come from the same window, which is now what E19's rule requires.)

## 7. R2-03's queue after this

| cut | size | status |
|---|---:|---|
| E17 split matrix load | −17,600 | built, Boundary green both arms, geometry now proven identical, **awaiting visual approval** |
| E16 hardware lighting | 35,000–50,000 | ceiling measured 53,760; four-part change |
| ~~E20 state-delta guard~~ | ~~25,000–30,000~~ | REFUTED (E21), ~3,920 |
| ~~E23 projection hoist~~ | ~~part of ~40,000~~ | **REVERTED, −3,008, sub-floor** |
| ~~per-root matrix work~~ | ~~~40,000 unpriced~~ | **resolved: the load is ~6,000–8,000; the balance is the light preamble** |

Nothing unpriced remains. **E16 is the only cut left in the phase**, and E22 has
now folded the last open item into it.
