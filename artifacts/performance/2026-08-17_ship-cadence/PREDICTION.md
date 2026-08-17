# Prediction — the shipping configuration's cadence, written before the run

**Date:** 2026-08-17 · **Branch:** `codex/r2-runtime2` · **base HEAD `87056b30353`**

This file is written **before** `build-c241-shipcadence` is measured, so the
result cannot be reverse-fitted onto it. `CADENCE_ARM.md` §7 named this
measurement as the next cycle's step 0 and left the answer open.

## What is being measured, and why it is not the same ROM as c240

`build-c240-cadence-draw0` is the tick-HUD ROM with the console render turned
off (`NDS_TICK_HUD_DRAW=0`). It reads **94.90%** two-VBlank. It still pays the
tick-HUD *apparatus* — the eleven 128-entry rings, the per-iteration
`ndsPlatformTickHudSample()`, the bucket accumulators — measured at a ~24,947
tick constant that the published ROM does not carry.

`build-c241-shipcadence` is `smash64ds-battle-playable-proof-hwtri`, which the
Makefile pins at **`NDS_TICK_HUD := 0`** and `NDS_SHIP_TELEMETRY := 1`
(`Makefile:1890-1896`). That is the published battle configuration
(`Makefile:1653-1664`) plus the debugger publications, and nothing else: both
blocks agree on scene harness, live input, HW triangles, profile level 0, fast
run 9, HW compose 2, GX compose 1, stage stride 8, HW matrix, HW light, shuffle
fold and fixed cubic.

**The direction of the residual is known, and it is one-sided.** The proof ROM
does *strictly more* per-frame work than the published ROM — the extra
`cpuGetTiming()` pairs around draw/HUD, `ndsRendererProfileFrameBegin` and
`ndsRendererProfileFramePublish`. It does not do less. So the number this run
produces is a **lower bound** on the published ROM's cadence, not an estimate
that could fall on either side of it.

The pacing counter itself is not conditional: `taskman_seam.c:5069` increments
`gNdsBattlePlayablePacingPresentIntervalBucket[]` in every configuration,
including the published one. Only its *publication* to the debugger needs
`NDS_SHIP_TELEMETRY`, which is exactly the reason the proof target exists.

The reader is `scripts/probe-present-cadence.ps1`, which reads the guest's own
cumulative histogram over gdb at `ndsBattlePlayableFrameCompleteMarker`. It has
no tick-HUD dependency of any kind — that is the reader §7 asked for, and it
was already in the tree.

## The prediction

Board model: subtracting a uniform 24,947 from c240's rows moves **13** more
in-window frames under the bracketed boundary `B = 1,118,496`, giving
1,935 + 13 = **1,948 / 2,039 = 95.54%**.

Three things push against taking that as the answer:

1. The proof ROM adds `NDS_SHIP_TELEMETRY` work back. Estimated small (a
   handful of `cpuGetTiming()` reads and one publish per frame) but unmeasured,
   so it subtracts an unknown 0–2 frames.
2. It is a **different binary**, so placement moves. The c239/c240 pair was one
   flag apart on the same source and its in-window over-boundary counts still
   differed by 2. A whole different target can differ by more.
3. The 24,947 constant was measured on the tick-HUD ROM's own `HUD` bucket. The
   proof ROM removes the sampling path entirely, which may be worth slightly
   more than that constant, not less.

**Point estimate: 95.4% two-VBlank whole match. Range: 94.6% – 96.3%.**

Secondary predictions:

- Total presented frames **2,039 ± 3** (the match length is fixed at 3,600
  ticks; c237/c239/c240 all read exactly 2,039).
- `slips = 0` — no cadence violation, i.e. no interval below 2.
- Max interval **17–20**, still owned by presented frame 2 (the stage's and
  fighters' first full draw). The tick HUD is not implicated in that frame, so
  removing it should not move the maximum much.
- Entry window (frames 1–439) at or above **97%**, gameplay window below it.

## What this run cannot settle

- It does not measure the published ROM directly. It bounds it from below.
- It does not re-bank the tick arm. `c239` stays the tick basis; this target's
  `WORK-H` is not comparable to it (no tick HUD, so no bucket instrument).
- **The ≥95% verdict is the owner's.** A measurement at 95.x% is a
  measurement, not an acceptance.
