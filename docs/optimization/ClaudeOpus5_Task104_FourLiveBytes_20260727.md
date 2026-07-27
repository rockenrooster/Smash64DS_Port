# Task 104 — A 1,292-byte copy that moved four live bytes

**Date:** 2026-07-27
**Status:** **KEEP, default on.** `STG` P50 −22,016, `WORK-H` P50 −26,240,
P95 −28,352, VBlank 4-interval 39 → 28. Boundary green.
**Flag:** `NDS_TASK104_STAGE_STATS_ELISION`, default `1`, kept so the A/B stays
reproducible.
**Inputs:** `artifacts/task104-control.json`, `artifacts/task104-cand.json`.
**Follows:** Task 103's partition of `STG`, Task 84 E1/E2's pricing of
`ndsRendererInitStats`, and Task 81's closure of the stage texture memo.

## 1. What was there

`ndsRendererPrepareNativeStageOwner` walks 8 stage segments. Each iteration
opened with a full clear of a 1,292-byte `NDSRendererStats`:

```c
ndsRendererInitStats(&sNdsNativeStageOwnerExecution.preflight_stats);
sNdsNativeStageOwnerExecution.preflight_stats.geometry_mode =
    segment->initial_geometry;
ndsRendererInitTraversalState(state, frame->config, &...preflight_stats, ...);
```

and then, on the three segments the Task 36 replay serves (mask `{0, 5, 7}`),
immediately overwrote every one of those bytes:

```c
*stats = owner->segment_stats[segment_index];   /* 1,292 bytes */
*epoch_mask |= owner->segment_epoch_mask[segment_index];
return TRUE;                                    /* caller: continue */
```

So a hit segment cleared 1,292 bytes, wrote one field, then copied 1,292 bytes
over the top — and `continue`d straight to the next iteration, which cleared the
struct again.

## 2. The liveness proof

Every access to `sNdsNativeStageOwnerExecution.preflight_stats` in the tree is
inside `ndsRendererPrepareNativeStageOwner`. The commit path never reads it.
(`nds_renderer.c:17951` also names a `preflight_stats`, but that is
`sNdsNativeFighterOwnerExecution` — a different struct instance.)

Within the loop, nothing runs between the copy and the next segment's clear.
After the loop, exactly one member is read:

```c
stats->sync_command_count =
    sNdsNativeStageOwnerExecution.preflight_stats.sync_command_count;
```

`NDS_NATIVE_STAGE_SEGMENT_COUNT` is 8 and the replay mask includes segment 7, so
the last iteration can be a hit — which is the only way any part of a copied
snapshot reaches that read.

**Therefore the 1,292-byte copy transports 4 live bytes, and the clear that
precedes it is entirely dead on hit segments.** The candidate carries
`sync_command_count` and hoists the eligibility test above the clear. That test
reads nothing out of the incoming stats — only the replay owner's flags and the
segment mask — so moving it is order-independent.

Equivalence for the one live field: if segment 7 is a hit, the field comes from
`segment_stats[7]` exactly as before; if it is a miss, it accumulates naturally
exactly as before. Every miss segment still opens with a full clear, so no miss
path can observe a field left dirty by a hit.

## 3. The measurement

Matched A/B, one source file, only `NDS_TASK104_STAGE_STATS_ELISION` differing.
32 samples, frames 439–470, ring dump.

| bucket | control P50 | candidate P50 | Δ P50 | control P95 | candidate P95 | Δ P95 |
|---|---|---|---|---|---|---|
| **STG** | 369,280 | 347,264 | **−22,016** | 375,808 | 352,064 | **−23,744** |
| **WORK-H** | 1,301,504 | 1,275,264 | **−26,240** | 1,675,776 | 1,647,424 | **−28,352** |
| WORK | 1,307,200 | 1,276,224 | −30,976 | 2,023,680 | 1,710,976 | −312,704 |
| FTR | 544,704 | 544,000 | −704 | 546,944 | 547,072 | +128 |
| SRC | 315,776 | 316,864 | +1,088 | 685,760 | 684,160 | −1,600 |
| MISC | 46,528 | 46,976 | +448 | 47,296 | 47,552 | +256 |
| WAIT | 367,616 | 404,032 | +36,416 | | | |
| ALL | 1,679,936 | 1,680,064 | +128 | | | |

VBlank intervals: **3: 428 → 439, 4: 39 → 28, 5+: 3 → 3, max 18 both.** Eleven
frames moved from four VBlanks to three.

Four things make this a KEEP rather than a hopeful reading:

- `STG` −22,016 is 3–4× the measured 5,000–7,000 noise floor.
- `FTR` is flat (−704). No re-addressing collateral — the failure mode that
  closed Tasks 87–89, 94, 95 and 103 E7.
- `SRC` is flat (+1,088), so the native owner is still succeeding. A broken
  epoch mask would fail preflight and push work into the generic fallback, which
  would raise `SRC`, not lower `STG`.
- The removed work reappears as `WAIT` (+36,416) — slack, not cost — while `ALL`
  moves +128, which is flat under VBlank quantization.

`WORK` P95 −312,704 is not claimed as the win. P95 resolution is 1/32 at this
sample count and that column is dominated by the HUD outlier frame; `WORK-H`
P95 −28,352 is the defensible tail number.

## 4. Why Task 103 E7 only realised 28%, answered

E7 removed the clear on hit segments and left the copy. It predicted 9,831
(3 × 3,277) and measured −2,752 — 28%.

Task 84 E1.4 had already named the mechanism and it was not instrument error:

> "the clear also warms the cache for the writes that immediately follow it.
> Delete it and some of those misses move to the first real write of each field
> rather than disappearing."

The copy wrote the same ~41 cache lines the clear had been warming, so removing
the clear relocated the misses into the copy instead of eliminating them. Remove
both accesses and there is nothing left for the misses to move into — the same
three segments now yield 22,016 instead of 2,752, an 8× difference from deleting
the second access as well as the first.

This also retires the explanation Task 103 E7 gave for its own miss. The span was
not over-attributed: Task 84's independent duplication measurement (3,544
ticks/call) agrees with E6's bracket (3,277), and the standing rule "trust a span
in proportion to its length" did not apply here.

## 5. What this says about the three closed `InitStats` routes

Task 84 left three routes and closed two. The 28% realisation ratio explains why
the survivors looked so unpromising: **any route that removes one access to bytes
that are still touched by a second access is capped at roughly a quarter of its
nominal size**, which puts every "clear less" variant under the noise floor.

Route 3 ("reduce the 11.7 calls/frame") is what this task did, but the reason it
paid is not that a call was removed. It is that *every* access to those bytes on
those segments was removed together. That distinction is the transferable part:

> Size a memory-traffic lever by the bytes that stop being touched, not by the
> instructions that stop executing.

Route 1 (skip `texture_tiles`) and Route 2 (persistent buffer) remain correctly
closed, and Route 1's counterexample — the unguarded read in
`ndsRendererSyncTextureTile` — is unaffected by this change, which never leaves a
miss segment with an uncleared struct.

## 6. What remains in the stage

Task 103's partition, updated. `STG` P50 is now 347,264.

| block | ticks/frame | note |
|---|---|---|
| `ndsRendererPrepareNativeStageOwner` | ~138,600 | was 160,588; this task took 22,016 |
| generic emit — 21 runs, 103 triangles | 63,607 | unowned |
| `ndsRendererAdapterPrepareNativeStageMatrices` | 55,077 | one call/frame, unowned |
| replay word push — 3,916 words @ 9.51 | 37,233 | |

Inside the prepare owner, the `PrepareRun` head remains the largest unowned
block at **67,119 ticks/frame over 21 calls**. It sits on a long span so its
measurement is trustworthy, and Task 81's closed memo does not cover it: that
one was a texture-identity memo at the bind seam, and Task 81 measured the stage
making **zero** texture binds during battle.

The `ApplyStateSpan` block (30,895 over 21 calls) is the other untouched one.

## 7. Verification

- `.\scripts\verify-boundary.ps1` — **passed**. Battle-only stage work, so
  Boundary is the widest relevant verifier; Latest is not stacked on it.
- `M3_NATIVE_STAGE_CHECK_OK callbacks=8 dobjs=57 bindings=42 runs=54 epochs=49
  triangles=202 state_deltas=148 state_events=423 replay_commands=886` —
  identical stage output. The geometry the stage submits did not change; only the
  diagnostic bytes copied alongside it did.
