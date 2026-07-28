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

## 5. What this does and does not license

**Licensed.** The per-run prepared facts are a memo. R2-03's baked-facts submit
is viable on the live path, in E1a's exact shape: keep the table, and recompute
only what moves.

**Not licensed.** Three limits worth stating before someone builds on this.

- State B is not simply "one fighter missing". Frame 1200 reads 67 runs with
  state B's hash, so run *count* is not a valid memo key. The key has to be the
  facts themselves or whatever selects them, not the count.
- `MATERIAL` never moved independently of `STABLE` — the two counters are equal
  at every one of the 31 samples. So this run does **not** demonstrate that the
  hurt flash perturbs `texture_prepare_material_color`; it may write colour
  elsewhere, or it may not have fired in a way that reached these runs. The
  falsifier's own comment predicted live materials and the measurement did not
  confirm it. A memo must still handle a colour change; it just is not the
  common case.
- This measures the *facts*, not the pose. The matrices were deliberately not
  hashed. They are Task #11's 91,338 ticks/frame and they move every frame.

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
