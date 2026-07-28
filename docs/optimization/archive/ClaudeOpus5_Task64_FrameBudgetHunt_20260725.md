# Task 64 — Where the frame actually goes: the hunt after the stage axis closed

**Date:** 2026-07-25
**Status:** Census + reframe. No code changed, no flag added.
**Supersedes:** Revision 2's Task 64 ("gate the repaired path"), void since Task 63 STOPped.

Task 63 closed the stage-geometry axis. This document answers "where next?" —
and finds that the campaign's *measurement* has been mis-set, not just its
targets.

## 1. The headline: `ALL` is a quantized wall clock, not a work meter

`ALL` is computed in `ndsBattlePlayableFinalizePresentedIteration`
(`src/port/taskman_seam.c:4896`):

```c
u32 all = cpuGetTiming() - sNdsBattlePlayableTickHudLoopStartTick;
```

That is **wall time for the whole presented iteration, including the VBlank
wait** — `gNdsRendererProfileVBlankWaitTicks` is one of the loop's own known
spans, and `OTHR` is defined as the residual `all - named`, so the wait lands
in `OTHR`.

A presented iteration therefore costs a whole number of VBlank periods. At the
ARM9's 33.513982 MHz against a 59.8261 Hz refresh, one VBlank is **560,190
cycles**. Task 56's control measured `ALL` P50 = **1,680,000** — three VBlank
periods to within 0.03% — with the VBlank histogram **2:0, 3:474, 4:80, 5+:12**.

Zero frames have ever hit 2 VBlanks. The frame is pinned at 3 → **20 FPS**.

## 2. What that does to every prior verdict

| task | work removed | `ALL` | recorded verdict |
|---|---|---|---|
| 53 | STG −187,648 | flat (OTHR +174,720) | KEEP, "ALL flat" |
| 55 | −20.6% stage words | flat | STOP |
| 56 | **−47% fighter vertices** | flat (−64) | **KILL** |
| 63 §5b | −18.8% stage GX words | flat | closed |

Every one of those is a correct measurement of a **quantized** quantity. A
change that removes real work but does not cross a 560,190-tick step *cannot*
move `ALL` — the reclaimed time simply becomes VBlank wait, which is exactly
what Task 53 observed when STG fell 187,648 and OTHR rose 174,720.

So "ALL flat" never meant "this lever removed nothing." It meant "this lever
removed less than one VBlank." The inference drawn from it — that the lever was
worthless — does not follow, and it was drawn four times.

Task 56 is the sharpest case: a **47% fighter-vertex reduction** was KILLed on
an `ALL`-flat reading. That is the largest single geometry reduction the
campaign ever produced.

**Correction to carry forward:** `ALL` is the right *acceptance* gate (it is
what the player experiences) but it is a terrible *search* gate. Searching must
be done against work, in ticks, and only the accumulated total gets tested
against `ALL`.

## 3. The budget, and the size of the gap

Task 56 control, tick-HUD ROM, frame 600, 111–128 samples:

| bucket | P50 ticks | share of `ALL` | attacked so far |
|---|---|---|---|
| FTR | 575,360 | 34.2% | geometry only (Task 56) |
| STG | 382,720 | 22.8% | exhaustively (Tasks 51–63) |
| SRC | 320,256 | 19.1% | **never** |
| OTHR | 275,008 | 16.4% | partially characterized (Task 54) |
| BG + AUD + HUD + MISC | ~126,656 | ~7.5% | Tasks 30/38/42 (audio) |
| **ALL** | **1,680,000** | 100% | |

`SRC` is `nNDSTickHudBucketSourceUpdate` (`include/nds/nds_startup.h:4051`) —
the BattleShip game simulation, fed from `gNdsTickHudSourceTicks`. It is the
third-largest bucket in the frame and **no task in this campaign has ever
looked at it.**

**The gap to 30 FPS:** the frame must fit two VBlanks, 1,120,000 ticks.
From 1,680,000 that is **560,000 ticks to remove — one third of the frame.**
No single lever measured so far is close; the largest was Task 53's 187,648,
and that one relocated rather than removed.

Note the ambiguity that must be resolved first: `OTHR` = `all - named` mixes
**VBlank idle wait** with **GX pipe-full stalls** (Task 54 proved the latter is
real and substantial). Until those are separated, the true work total is
somewhere between 1,405,000 (named only) and 1,680,000, and the required cut is
somewhere between 285,000 and 560,000 ticks. Resolving that is step one.

## 4. Ranked targets

1. **FTR non-vertex work — 575,360 ticks (34%).** Task 56's own ledger names
   the cost: *"matrix arithmetic, lighting, dense-vertex preparation, hierarchy
   traversal rather than geometry-engine transform."* Every fighter task so far
   attacked geometry submission, which that sentence says is not where the time
   is. This is the largest bucket and its cost is already named and unattacked.
2. **SRC — 320,256 ticks (19%).** Entirely unexamined. Highest uncertainty,
   which cuts both ways: could be irreducible decomp-faithful simulation, could
   contain the same kind of soft-float and `mem*` traffic the old census ranked
   at 9.49% and 7.84% globally.
3. **OTHR — 275,008 ticks (16%).** Decompose into wait vs stall before
   treating any of it as recoverable.

## 5. The instrument to use — already built, never pointed here

`include/nds/nds_task37_profile.h` documents a per-program-counter ARM9
profiler in the repo's own melonDS fork: it attributes emulated cycles by PC
**including cache fills, cache streaming, write-buffer drains, bus waits,
interlocks, and pipeline refills**, driven by `MELONDS_ARM9_PROFILE_CSV` with a
CP15 trace-ID control channel to window it (`NDS_TASK37_PROFILE`, default 0,
compiles to nothing when off).

That is precisely the instrument for "what are FTR's 575,360 ticks made of,"
and it was built for Task 37 and never reused. It answers the question directly
instead of by the subtract-a-lever-and-look-at-ALL method that has now produced
four uninformative results.

## 6. Recommended next task

**Task 65 — PC-attributed census of FTR and SRC.** Window the ARM9 profiler on
the canonical Boundary configuration at frames 438+, dump the CSV, and rank
functions by inclusive cycles within the fighter draw and the source update.
Deliverable: a ranked table of the top ~30 functions with their share of the
frame, plus a decomposition of `OTHR` into VBlank wait vs GX stall.

Gate the *next* optimization on ticks removed against the 285,000–560,000
target, and reserve `ALL` for the final acceptance test once enough has
accumulated to plausibly cross a VBlank step.

Also worth reconsidering on the new reading: **Task 56 mode 2 deserves to be
un-KILLed as a banked contribution** (its code exists, default-off, on branch
`codex/task56-fighter-stripify`). It was rejected for failing a test it could
not have passed alone. It should be re-evaluated as part of an accumulated
budget, not on its own `ALL` reading — though note its measured FTR *rose*
1.0%, so its contribution to CPU ticks may be nil even if its GX-vertex cut is
real. That is exactly the kind of question the PC profiler settles.
