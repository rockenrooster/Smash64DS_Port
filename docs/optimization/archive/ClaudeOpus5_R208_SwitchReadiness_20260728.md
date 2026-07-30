# R2-08 switch readiness — §6 acceptance gates evaluated

**Date:** 2026-07-28
**Status:** **NOT READY.** Gate 3 fails by 1.37x. Gates 4 and 5 are unrun.
**Nothing was switched**: no ROM republished, no public-build pin moved. This is
the measurement §6 requires *before* the switch, not the switch.

## Why this exists

The switch plan's §6 lists five acceptance gates, all required. Whether the
switch can proceed is a measurable question, and it had been getting answered by
argument. This answers it from evidence already collected this cycle.

## The five gates

### 1. Boundary verifier green on the Runtime 2 battle path — **PASS**

`.\scripts\verify-all.ps1 -Profile Boundary` passed on the shipping
configuration with both graduated cuts enabled (`NDS_R2_FIGHTER_HW_MTX := 1`,
`NDS_R2_FIGHTER_HW_LIGHT := 1`), including
`battle_playable_realtime` and the published ROM contract check.

### 2. Visual gate — **PARTIAL**

Owner approved both rendering-side changes on 2026-07-28: E17 (split matrix
load) and E16 (hardware lighting). Geometry is bit-identical in both
(`P0 = 181,440`, `P1 = 173,502` in candidate and control over the same
480-frame window).

**Not satisfied**: §6 asks for *synchronized screenshot diffs*, and no
frame-locked cross-build capture exists. `capture-melonds.ps1 -ExactFirstFrame`
is gated to the Cut G GO-text window; live captures drift because the faster arm
reaches a later match clock at the same wall delay. Recorded as a harness gap.

### 3. Performance gate: P95 <= 1.12M ticks/presented frame — **FAIL, 1.37x**

Tick-HUD ring dump, 128 samples, frames 439..566, shipping configuration:

| series | P50 | **P95** | gate |
|---|---:|---:|---|
| WORK | 1,065,664 | **1,530,816** | <= 1,120,000 |
| WORK-H | 1,058,240 | 1,493,120 | |
| ALL | 1,119,872 | 1,680,384 | |

**WORK P95 is 1,530,816 against a 1,120,000 budget — 410,816 over, 1.37x.**

`PROJECT_GOAL.md` gates the milestone on WORK's P95 specifically, which is why
that row is decisive and not ALL (VBlank-quantized) or P50.

Subsystem attribution: `FTR` P50 456,512 against R2-03's provisional 250,000
budget. The fighter draw is the single subsystem over budget; `STG` P50 173,120
is inside R2-02's 180,000.

VBlank interval histogram, per the device-reporting rule:
**2:420 3:134 4:10 5+:2, max 18.**

### 4. Stability: full 3600-tick soak, zero flashes/corruption/hangs — **UNRUN**

Not attempted this cycle. Cheap to run and should be, but it cannot rescue gate 3.

### 5. Owner play test on retail hardware, recorded — **UNRUN**

Requires the owner and physical hardware. Not attempted.

## Verdict

**1 pass, 1 partial, 1 hard fail, 2 unrun.** The switch cannot proceed, and the
blocker is not procedural — it is 410,816 ticks of P95, which is what R2-03
exists to remove and has removed 52,672 of.

Performing the switch anyway would republish `smash64ds.nds` and
`smash64ds-battle-playable-hwtri.nds` and move the public-build pin to a
configuration that misses the project's own headline performance requirement by
37%, while §6 states all five gates are required. That is the specific outcome
the gates exist to prevent.

## What closes gate 3

R2-03's remaining coupled cost is 107,307 ticks/frame (state replay 65,026 +
`PrepareProductionRun` 42,281), specified for removal in
`ClaudeOpus5_R203_E26_Spec_GeneratedEpochState_20260728.md`. That is roughly a
quarter of the 410,816 overshoot. R2-04's presentation-rate pose work and further
geometry reduction are also required; the plan's own §7 sequences them after
R2-03 for that reason.

**This document should be re-run and updated at each phase gate**, so the switch
decision is always a table rather than a judgement call.
