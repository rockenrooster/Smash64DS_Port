# R2-03 E5 — the fighter's per-run prepared facts are a two-valued table

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** CONFIRMED — the facts are memoisable. 1.9% of frames change them.

## 1. The question

`Smash64DS_Runtime2_SwitchPlan.md` §7 asks R2-03 for a "per-epoch generated
submit consuming only baked facts (`poly_fmt`, texture slot, palette, matrix
binding, corner stream) — no `PrepareProductionRun` policy re-checks, no
traversal-state/stats dependency, no per-frame texture identity proof."

That is R2-02 E1a's cut moved to the fighter, and E1a was worth 94,784
ticks/frame on a table of the same shape. It is only worth building if those
facts actually hold still. The fighter differs from Dream Land in a way that
could decide it: its materials are live where the stage's are not.

E1 already refuted the neighbouring memo — the shade loop changed on 1,796 of
1,835 frames. So this is a real question, not a formality.

## 2. The first instrument was hooked to a dead path

The falsifier was first placed at the tail of
`ndsRendererNativePreflightFighterHierarchy`, which fills
`execution->hierarchy_runs[]`. It reported, over 628 frames:

```
calls=0 epochs=0 full=0x811c9dc5 stable=0x811c9dc5 light=0x811c9dc5
```

`0x811c9dc5` is the FNV offset basis. The hashes were "constant" because the
hook never ran once.

`reloc_backend_renderer_dl.c:12095` picks between two owner entry points:

```c
production_result = (native_owner_hierarchy_mode != FALSE) ?
    ndsRendererExecuteNativeFighterOwnerHierarchy(...) :
    ndsRendererExecuteNativeFighterOwnerProduction(...);
```

and `native_owner_hierarchy_mode` is TRUE only for
`NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS` (mode 7). Canonical is **mode 9**,
`NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE`, which sets
`native_owner_production_mode` instead. `hierarchy_runs[]` is mode 7's table and
the shipping build never writes it.

**A hash that never moved because the code never ran is indistinguishable, in
the hash alone, from a hash that never moved because the data held still.** The
call counter is what separated them, and it is the only reason this was caught
rather than banked as a spectacular KEEP.

## 3. The instrument that measures the live path

Rehooked inside `ndsRendererNativePrepareProductionRun`, immediately before its
single `return TRUE`. That is the function mode 9 actually calls, once per run
per frame, and it is called with `hierarchy_run == NULL`: the facts are
recomputed and consumed on the spot and never stored anywhere. That recompute is
the cost R2-03 exists to remove.

Three FNV-1a accumulators, folded per run and compared per hardware frame:

| hash | covers |
|---|---|
| `STABLE` | run index, texture name/params/format/width/height, `poly_fmt`, UV scale/origin/offset, vertex flags, textured |
| `MATERIAL` | run index, `texture_prepare_material_color` |
| `FULL` | both, plus the `NDSRendererHardwareTextureCacheEntry *` |

`FULL` carries the cache pointer because it rotates for reasons unrelated to
what is drawn; `STABLE` omits it deliberately, which is the distinction E1a
turned into a fix.

The hash function is `noinline, noclone, cold, optimize("Os")` in its own
`.text.r2_run_proof` section. `PrepareProductionRun` is
`NDS_RENDERER_NATIVE_FIGHTER_CODE`, i.e. ITCM, and inlining the hash there
overflowed the region by exactly 100 bytes.

## 4. Result

Canonical Boundary configuration, one-minute match, sampled every 50 presented
frames from 250 to 1750 — 1,500 frames of live combat against the level-3 Fox.

| accumulator | changes | rate |
|---|---|---|
| `STABLE` | 28 / 1500 | **1.9%** |
| `FULL` | 28 / 1500 | 1.9% |
| `MATERIAL` | 28 / 1500 | 1.9% |

The table takes only **two distinct values** in the entire match:

| state | runs | `STABLE` | `FULL` |
|---|---|---|---|
| A | 67 | `0x50f2e180` | `0x0ae15780` |
| B | 37 | `0x80b47807` | `0x4c44bf00` |

A denser sweep (every 4 frames, 500→628) confirms the shape: long constant runs
with occasional flips, never per-frame churn.

Compare E1, on the neighbouring loop: **97.9% changed**. This is 98.1%
*unchanged*. The two results are mirror images and the difference is real, not
instrument noise.

### 4a. Per-run, the facts never change at all

The per-frame digests fold every prepared run in submission order, so they
cannot tell "a run's facts changed" from "a different set of runs ran". Keeping
one digest per run index separates them:

| counter | value |
|---|---|
| fills (first sight of a run index) | 67 |
| **hits** (facts identical to the stored copy) | **112,300** |
| **misses** (facts differed) | **0** |
| out-of-range run indices | 0 |

67 is the whole of `sNdsNativeFighterRuns[]`. Every run is filled once and never
changes again. The 29 whole-frame "changes" were the *set* moving between 67 and
37 prepared runs, never the data.

This also settles §5's material caveat: colour is baked per run, and Task 39's
hurt flash reaches the fighter through `state->color_modulate` in the vertex
path (`ndsRendererHardwareModulatePackedColor`), which is downstream of these
facts and untouched by a memo of them.

### 4b. The function never rejects

A miss counter hooked before `return TRUE` cannot see a rejected call, and a run
that is sometimes accepted and sometimes rejected must never be baked. Counting
entries separately:

```
entry = 112,367        hit + miss + fill = 112,300 + 0 + 67 = 112,367
```

Exactly equal. `ndsRendererNativePrepareProductionRun` returned FALSE zero times
in the whole match, so the 67-vs-37 split is runs that were never *called*, not
runs that were *rejected*.

Taken together: for the canonical configuration this function is a pure lookup
keyed on `run_index`. Every policy check, geometry-mode validation, texture
resolution and UV derivation in it recomputes an answer that cannot differ.

## 5. What this does and does not license

**Licensed.** The per-run prepared facts are a memo keyed on `run_index` alone,
with no per-frame revalidation required: 0 misses and 0 rejections in 112,367
calls. R2-03's baked-facts submit is viable on the live path, and the table can
be *generated* rather than discovered — which is what §7 asks for.

**Not licensed.** Limits worth stating before someone builds on this.

- Run *count* is not a valid key. Frame 1200 reads 67 runs carrying state B's
  whole-frame hash. Key on the index.
- This measures the *facts*, not the pose. The matrices were deliberately not
  hashed. They are Task #11's 91,338 ticks/frame and they move every frame.
- Zero misses over one match is not a proof of immutability for all inputs. The
  texture cache entry pointer held still here because nothing evicted a fighter
  texture; a different scene, a paused camera, or a stage with more texture
  pressure could rotate it. A shipped memo should either carry the cache's
  `key_generation` as a cheap validity tag or be regenerated at load, not
  silently assume the pointer is eternal.
- One configuration, one stage, two fighters. The result is strong for canonical
  mode 9 on Dream Land and is not evidence about modes 7 or 8.

## 6. Size of the prize

Owner prep is 113,199 ticks/frame (E2/E3), split MatrixPrep 91,338 (E4) +
MaterialPrep 21,504. This result is about the 21,504 — roughly 1.9% of the
1.12M gate, and it does not close the 460K gap on its own. The pose half is
four times larger and is a different problem.

The value here is that it is the *enabling* step for R2-03's direct submit, and
it is now proven rather than assumed.

## 7. Reproduce

```
make BUILD=build-r2-03-e5-runproof \
  TARGET=smash64ds-battle-playable-tickhud-hwtri NDS_R2_FIGHTER_RUN_PROOF=1
```

ROM `BDE87945FD209763`. Probe reads `gNdsR2Run{Full,Stable,Material}Hash` and
their change counters at a breakpoint on
`ndsBattlePlayableFrameCompleteMarker`, stepping the presented-frame counter.
`gNdsR2RunCallCount` must be non-zero — see §2.
