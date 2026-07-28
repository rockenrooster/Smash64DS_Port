# R2-02 E1a — the stage prepares 21 runs a frame and reuses all 21

**Date:** 2026-07-27
**Phase:** R2-02 (`Smash64DS_Runtime2_SwitchPlan.md` §7), first landed cut.
**Flag:** `NDS_R2_STAGE_DIRECT` (default `0`).
**Verdict: KEEP.** `STG` P50 −94,784, down on **128 of 128 frames**, Boundary
green, stage screenshot unchanged.

---

## 1. What was changed

`ndsRendererPrepareNativeStageOwner` called `ndsRendererNativeStagePrepareRun`
once per stage run, 21 times per frame, to derive a `NDSNativeStagePreparedRun`
from three generated ROM tables plus live `stats`. E0 established that every
input except `stats` is already baked, and that Task 44's steady-state admission
already proves `stats` does not change.

The cut caches the 21 prepared records in `sNdsNativeStageOwnerExecution` and
skips the whole loop while a reuse key holds. The key is
`frame->config` plus the eight asset base pointers, not the Task 44 guard —
E0 §5a required the reuse to key on identity rather than lean on a guard whose
purpose is something else. `epoch_mask` is cached and restored with the record
because the Task 36 replay capture and the segment-0 hash consume it after the
loop. Any `accepted == FALSE` path invalidates the cache.

This is the "cheaper intermediate" E0 §5 said to resist as an endpoint and to
run as a measurement step first. It has now done that job: the saving is where
§3 said it was, and larger. The generator bake (E0 §5 item 1) is recorded below
as the deletion follow-up.

## 2. Engagement, before any tick is read

565 reuses, 2 builds, over 566 presented frames — read from the same run that
produced the buckets, via the new `-ExtraGlobals` switch on
`scripts/sample-tick-hud-buckets.ps1`.

This is reported first on purpose. A flag that silently never fires is
indistinguishable from a flag that fired and saved nothing, and this campaign
has already shipped that mistake once: Task 52 found the Task 36 replay
structurally disabled *after* it had been measured. The two builds are the
first frame and one re-arm; the elision is live on 99.6% of frames.

## 3. Measurement

One tree, two builds, flag off versus on (Task 79: vary the build, not the run).
128-frame ring dump, frames 439–566, `smash64ds-battle-playable-tickhud-hwtri`,
melonDS-Accurate `DE80E46B…`, git `c2b92c3d6c`.

| bucket | off P50 | on P50 | ΔP50 | off P95 | on P95 | ΔP95 |
|---|---|---|---|---|---|---|
| `ALL` | 1,680,064 | 1,680,000 | −64 | 2,240,384 | 1,680,768 | −559,616 |
| `FTR` | 543,104 | 545,472 | +2,368 | 986,304 | 994,560 | +8,256 |
| **`STG`** | **351,488** | **256,704** | **−94,784** | **357,376** | **264,384** | **−92,992** |
| `SRC` | 324,224 | 325,888 | +1,664 | 664,384 | 683,584 | +19,200 |
| `MISC` | 48,512 | 47,936 | −576 | 157,440 | 157,440 | 0 |
| `WAIT` | 358,784 | 441,792 | +83,008 | 466,944 | 517,568 | +50,624 |
| **`WORK`** | **1,326,080** | **1,239,296** | **−86,784** | **1,799,360** | **1,669,440** | **−129,920** |
| `WORK-H` | 1,321,728 | 1,228,608 | −93,120 | 1,719,360 | 1,643,328 | −76,032 |

`BG`, `AUD` and `HUD` moved by ≤ 704 ticks and are omitted.

**Paired per-frame, candidate minus control, matched by ring slot:**

| bucket | frames down | frames up | median Δ |
|---|---|---|---|
| **`STG`** | **128** | **0** | **−94,688** |
| `WORK` | 114 | 14 | −89,216 |
| `WAIT` | 23 | 105 | +88,448 |
| `FTR` | 25 | 103 | +2,240 |
| `SRC` | 25 | 103 | +1,184 |

`STG` down on every single frame, at 13× the 5,000–7,000 build-placement noise
floor (Task 100), is a mechanism and not placement. `FTR` and `SRC` move by
about a thousand ticks with an inconsistent sign — that *is* placement, and it
is a tenth the size of the win.

`WAIT` rising by almost exactly what `WORK` loses is the expected shape: a
saving smaller than one 560,190-tick VBlank period becomes idle rather than
frame rate. That is why `ALL` P50 is flat at −64 and why `ALL` is not the
series to read (`TASK_STANDING_RULES.md`).

**VBlank interval histogram** (the pacing signal AGENTS.md requires, not min FPS):

| | 2 | 3 | 4 | 5+ | max | slips |
|---|---|---|---|---|---|---|
| off | 0 | 511 | **50** | 5 | 18 | 0 |
| on | 1 | 548 | **12** | 5 | 18 | 0 |

Four-VBlank frames fall from 50 to 12 out of 566. That is the win reaching
pacing, not just the tick counter.

## 4. Against the prediction

E0 sized the `PrepareRun` head bracket at 67,119 ticks/frame. Realized 94,784 —
**141% of prediction**, against a kill line of ~40,000.

I do not claim to have measured why it over-delivered. The plausible reading is
that the bracket counts the instructions and not the memory traffic: R2-00b put
62.1% of frame work in stall, and a 21-iteration loop over three generated
tables costs D-cache traffic that a tick bracket around the call attributes
nowhere. That is a hypothesis, not a result. It is worth naming because if it is
right, every remaining table-walk in the frame is under-sized by the same
mechanism.

## 5. Correctness

- **Boundary green** on the candidate configuration (`battle_playable_realtime`,
  mode 163, one-minute), ROM built `NDS_R2_STAGE_DIRECT=1`.
- **Automated screenshot analysis:** top-screen required-region detail
  **62.792%** (4521/7200) candidate versus **62.778%** (4520/7200) control —
  one pixel in 7200.
- **Screenshots:** `artifacts/visibility/r2-02-e1a-on-boundary-20260727.png` and
  `…-off-…png`. Tree, trunks, bushes, flowers, fence, pond, both platforms and
  the sky are unchanged. The fighters differ in animation phase because the two
  Boundary captures land on different game frames; that is capture timing, not
  rendering. **Owner visual approval is still outstanding** — this is the
  fidelity-budget report, not the sign-off.
- 0 cadence violations on both arms; `vbiTotal` 566 on both, so neither arm
  dropped or duplicated a presented frame.

## 6. Where this leaves the budget

`STG` is 256,704 against the frozen 180K budget — gap 76,704, down from 171,488.
`WORK` P95 is 1,669,440 against the 1.12M gate; 549,440 to go.

The next block in this phase is `ndsRendererAdapterPrepareNativeStageMatrices`,
bracketed at 55,077 ticks/frame on one call. In steady state it composes 16
dynamic bindings through `ndsRendererAdapterPrepareInitialMatrices` plus
`ndsRendererAdapterComposeNativeRootMatrix`. Unlike `PrepareRun` this is **not**
frame-invariant — the camera moves, so every composed matrix is genuinely new —
so it needs a different attack than a reuse key, and E0's sizing for it should
not be assumed to transfer.

## 7. Evidence

`artifacts/` is outside git, so the run records are anchored by hash:

| SHA-256 (first 16) | file |
|---|---|
| `CB3122EE44E14627` | `artifacts/performance/r2-02-e1a-off-438.json` |
| `75591E1CFA079E28` | `artifacts/performance/r2-02-e1a-on-438.json` |
| `A9D638F6606D69D8` | `artifacts/visibility/r2-02-e1a-off-boundary-20260727.png` |
| `A3F9A58E6D68109C` | `artifacts/visibility/r2-02-e1a-on-boundary-20260727.png` |

One caveat on the tree the arms were built from: it also carried another agent's
uncommitted bug #10 lab probes, including a `poly_fmt` local in
`ndsRendererNativeBeginHierarchyBatch` that is a no-op at `NDS_LAB_NO_CULL=0`.
**Both** arms carried it identically, so the A/B delta is unaffected, but the
absolute numbers are from a tree one no-op refactor away from this commit.

## 8. Follow-ups this cut created

- **E1b, deletion:** bake `NDSNativeStagePreparedRun` for all 21 runs in
  `generate_nds_native_stage.py` and resolve `texture_entry` at match load. That
  removes the ROM-side derivation and the first-frame build entirely; the
  runtime gain over E1a is small, the code deletion is not. Fits R2-08.
- `scripts/sample-tick-hud-buckets.ps1` gained `-ExtraGlobals`. It throws rather
  than reporting zeros when the read produces no line, because a missing read
  and a counter that never incremented must not look alike.
