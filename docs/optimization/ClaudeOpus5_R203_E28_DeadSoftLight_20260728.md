# R2-03 E28 — the software light preparation E16 left behind

**Date:** 2026-07-28
**Verdict:** **KEEP.** FTR −31,488 ticks/frame (median of 128 paired frames),
geometry bit-identical, every measured frame improved.

## What it is

E16 moved the fighter's diffuse term onto the geometry engine and skipped the
per-dense-vertex shading loop with a `hardware_lit` flag. It did **not** remove
the work that produced that loop's inputs. Every lit epoch still ran:

- `ndsRendererHardwarePrepareLitDirection` — nine 32x32->64 multiplies to push
  the light into modelview space, three 64-bit squares, an `sqrtf`, and three
  float divides — whenever `state->prepared_light_direction_valid` was clear,
  which the matrix change at every root bind clears;
- `ndsRendererHardwareGetLightShadeLut` — a linear scan of the shade cache, and
  a 128-entry build on a miss.

Both results are read **only** inside the loop `hardware_lit` skips. The single
other reader, `ndsRendererM2ShadeCensusEpoch` at `nds_renderer.c:24451`, is
under `NDS_RENDERER_M2_DETAILED_LEDGER` and is not in any shipping or tick-HUD
configuration, so the cut is conditioned on that flag being off.

The change is a `#if`: `NDS_R2_SHADE_SKIP_SOFT_LIGHT`, plus one build-independent
`epoch_lit` predicate replacing the two `prepared_direction != NULL` tests that
were standing in for "is this epoch lit".

## Why it was not found earlier

E24 read this function, concluded "the action walk isn't the cost", and moved
on. The action walk is indeed not the cost — but the walk is not what the E16
flag left running. The dead work is in the **preamble above** the walk, inside
the condition that decides whether the epoch is lit at all. The bracket that
priced the shade (`gNdsR2ExecShadeTicks`) covered both, so the preamble's share
was never separated from the walk's.

**Standing lesson:** when a cut skips a loop with a runtime flag, price what
computes the loop's *inputs* separately from the loop. A flag that skips a
consumer does not skip its producers, and a single bracket around both cannot
tell you which one you actually removed.

## Evidence

Two arms from one tree. Control = `NDS_R2_FIGHTER_SOFT_LIGHT_KEEP=1`, candidate
= default. Same target (`smash64ds-battle-playable-tickhud-hwtri`), same
emulator, same window, ring dump, 128 samples, frames 439..566.

### Paired per-frame deltas (candidate − control, same frame = same game state)

| bucket | better | worse | median delta | worst regression |
|---|---:|---:|---:|---:|
| **FTR** | **128/128** | **0** | **−31,488** | −13,824 (still an improvement) |
| WORK | 113 | 15 | −31,680 | +375,232 |
| ALL | 60 | 59 | 0 | +560,640 (one VBlank) |

`FTR` improved on every one of the 128 frames, and the *least* improved frame
still fell 13,824. That is well clear of the 5,000–7,000 build-placement noise
floor.

**WORK frames over the 1,120,000 gate: 52/128 -> 40/128.**

### VBlank interval histogram (the required device-report form)

| arm | 2 | 3 | 4 | 5+ | max |
|---|---:|---:|---:|---:|---:|
| control | 409 | 148 | 7 | 2 | 18 |
| candidate | **438** | **117** | 9 | 2 | 18 |

29 presented frames moved from a 3-VBlank interval to a 2-VBlank interval.

### On the WORK P95 that rose

The sorted-column table read `WORK P95 1,496,064 -> 1,569,728`, +73,664, which
contradicts every other number here. It is an artifact: each column's P95 is a
*different frame*, and the P95 frame is an excursion frame — the settled
"renderer fast-path dropout" — whose placement moves between arms. `WAIT` P95
fell by almost exactly the same amount (−97,664) in the same table, which is the
tell: inside a VBlank-quantized frame the wall time is fixed, so WORK and WAIT
trade against each other and neither is a cost signal on its own.

The paired comparison is the correct instrument here and it is unambiguous.
Recorded so the next cut does not re-litigate it: **compare arms frame-by-frame
by frame number, not by sorted percentile.** Both arms run the same deterministic
ROM from the same start frame, so frame N is the same game state in both, and
the pairing is free.

## Correctness

- **Structural (E19's rule).** `gNdsFighterDLAllDrawP0HardwareTriangleCount` =
  136,640 and `...P1...` = 146,880 over frames 439..919 (480 frames) in **both**
  arms. Identical, not merely close.
- **Mechanically:** the removed values had no consumer in this configuration, so
  bit-identical output is the predicted result rather than a lucky one. This is
  not an approximation and does not spend fidelity budget.
- Boundary verifier: see the board entry for the result.

## Where R2-03 stands after this

Fighter execute partition, shipping configuration, before E28:

| phase | ticks/frame |
|---|---:|
| Preflight | 3,272 |
| Root | 44,785 |
| State replay | 72,798 |
| Shade | 57,715 |
| Submit | 105,630 (of which prepare 41,928) |
| sum | 284,200 |

E28 takes ~31,500 out of the shade. Graduated R2-03 total is now
E17 17,600 + E16 35,072 + E28 31,488 = **84,160**.

The next ranked items are unchanged: the state replay (72,798) coupled to the
prepare (41,928), specified in
`ClaudeOpus5_R203_E26_Spec_GeneratedEpochState_20260728.md`, and the per-root
44,785.

## E27 is dead

The same session measured `gNdsR2MaterialOnlyInvalidations` at **2.0/frame**
against 28.0 material applications: 26 of every 28 material invalidations of the
texture prepare hit a prepare that the before-span deltas had *already*
invalidated. Splitting the prepare's validity would therefore reach 2.0 of the
46.4 full prepares a frame — roughly 1,800 ticks, below the noise floor.
Refuted; do not re-propose it as a standalone cut. It remains a necessary
*component* of E26, where the deltas no longer invalidate first.
