# R2-03 E14 — the submit is the fighter, and the 3D hardware is asleep

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** Measurement. The unnamed residual is
`ndsRendererExecuteNativeFighterOwnerProduction` at **279,617 ticks/frame**, and
**there is no GX backpressure at all** — the geometry engine is idle while the
ARM9 spends 447 ticks per triangle deciding what to send it.

## 1. The residual, named

E13 left 336,030 ticks/frame — 67% of the fighter draw — charged to a single
remainder, because E3's split stopped where the owner inputs get built. Two
brackets close it. Control build, 479 frames, 958 draw calls, both fighters,
per presented frame:

| phase | ticks/frame | share of draw |
|---|---:|---:|
| Walk | 3,368 | 0.7% |
| Validate | 10,595 | 2.1% |
| Reset | 6,632 | 1.3% |
| OwnerPrep (matrices + materials) | 143,684 | 28.6% |
| Build production inputs | 37,292 | 7.4% |
| **Execute against GX** | **279,617** | **55.7%** |
| tail (by arithmetic) | 20,436 | 4.1% |
| **total** | **501,624** | |

`ndsRendererExecuteNativeFighterOwnerProduction` alone is **25% of the entire
1.12M frame budget** and 2.3x everything R2-03 has spent four experiments
optimising.

## 2. Which side of the FIFO is slow

The cut that follows depends entirely on this, and the two answers take opposite
actions: if the CPU is being throttled by a full command FIFO, only submitting
less geometry helps, and that is a visual-fidelity trade. If the geometry engine
is starved, the cost is ours and a cheaper emitter fixes it without touching a
pixel.

`GXSTAT` answers it directly — bits 16-24 are the command FIFO's 40-bit entry
count, bit 24 is full, bit 26 is empty, bit 27 is geometry-engine-busy. Sampled
either side of the submission, 946 samples:

| reading | value |
|---|---:|
| FIFO entries entering the submit | **0** |
| FIFO entries leaving the submit | **0** |
| max entries ever seen leaving | **0** |
| geometry engine busy on exit | **0 of 946** |
| OR of every raw GXSTAT word | **0x06009F00** |

**The FIFO is empty at both ends of every fighter submission this match, and the
geometry engine is never busy when we finish handing it a fighter.** There is no
backpressure to find. The ARM9 is the slow side by a wide margin, and the DS's
3D hardware — the part that is actually good at this — is idle waiting for it.

### The positive control, and why it was needed

"Every FIFO field reads zero" is also precisely what a probe pointed at nothing
produces, and this cycle had already been fooled once by exactly that shape
(E13 §6). So the raw register word is OR-ed across every sample: **bit 26,
command-FIFO-empty, is set**, along with bit 25 (less than half full) and a
plausible matrix-stack level in bits 8-12. A dead read cannot set bit 26. The
register is live and the zeros are the measurement.

## 3. What this means for R2-03

447 ticks of ARM9 per hardware triangle, with the geometry engine idle.

- **Cutting fighter polygons is the wrong lever.** It would help — the cost is
  per-triangle — but it spends visual fidelity to work around a CPU that is
  failing to feed hardware with headroom to spare. `PROJECT_GOAL.md` permits
  that trade; this measurement says we have not yet earned it.
- **The lever is a cheaper emitter**, and the precedent is already in this repo
  and already won: R2-02 E2 put the stage's rigid replay on GXFIFO DMA. The
  fighter emitter still walks and packs per triangle on the CPU. This is E12's
  lesson again at a larger scale — *a fast path exists that the specialized
  fighter path never took*.
- The two shipped R2-03 cuts (-47,486 combined) both landed in OwnerPrep, 28.6%
  of the draw. The 55.7% was never instrumented, so it was never a candidate.

## 4. Incidental, recorded not chased

`GXSTAT` bit 15 — matrix stack overflow/underflow error — is set in the OR, so
it latched at least once during the run. It is a sticky flag and may well date
from init or teardown rather than the match. It is not this experiment's
business and nothing observable is wrong, but it is an error bit that is on, and
`AGENTS.md` does not permit leaving that unrecorded. Board item, not a detour.

## 5. Next

**R2-03 E15: price a DMA/precompiled command stream for the fighter emitter**,
against the 279,617. The fighter's geometry is already a generated program
(`sNdsNativeFighterRuns[67]`, `sNdsNativeFighterDenseVertices[541]`), which is
the same starting position the stage was in before E2.
