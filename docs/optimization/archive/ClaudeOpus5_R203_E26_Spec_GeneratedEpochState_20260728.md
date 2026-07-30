# R2-03 E26 — implementation spec: generated per-epoch resolved state

**Date:** 2026-07-28
**Status:** specified, not started. This is R2-03's remaining work and the switch
plan's own R2-03 bullet.
**Target:** the coupled 107,307 ticks/frame (state replay 65,026 + prepare
42,281). Nothing smaller reaches it — see §1.

## 1. Why this and not something cheaper

Four prior cuts read null and E25b explained all four at once: the state replay
and `PrepareProductionRun` are one mechanism. The replay applies 194.4 state
deltas a frame, 127.3 of which call
`NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE`, which forces the full prepare on 46.4
of 62.8 runs — even though E12's texture memo hits 99.5% and E5 measured the
prepare's outputs at 1.9% churn.

E25c then ruled out the cheap repair. Of the 127.3 invalidating deltas, **70.2
move the 20-word tile state** against 57.1 cheap scalars, so no per-run value key
is affordable: a generation counter bumps more often than there are runs, and
hashing the tile costs 20 loads against 781 saved.

**There is no cheap validity key for a value legitimately rewritten more often
than it is read.** The replay writes constantly because that is what a replay
does. The draw needs only the *post-replay* state, and that is stable.

## 2. The change

Emit, per epoch, the **resolved** state the draw actually reads, and install it
directly instead of replaying deltas to reconstruct it.

### Generator (`scripts/generate_nds_native_owners.py`)

`append_state_span` already receives `(phase, root_index, epoch_index, first,
count)` and resolves each `sequence[i] -> state[delta_index] -> (w0, w1,
effect)`. Everything needed is in scope; the change is a fold rather than new
data.

1. Maintain a model of the renderer state while walking the program in
   submission order — the same order `append_state_span` is already called in.
   Apply each delta exactly as `ndsRendererNativeApplyStateDelta` does
   (`nds_renderer.c`, the `switch (delta->effect)`), for effects 2..14.
2. After each epoch's `before` span, snapshot the fields
   `ndsRendererNativePrepareProductionRun` reads:
   - `geometry_mode`, `othermode_l`
   - `texture_combine_w0`, `texture_combine_w1`
   - `env_color`, `prim_color`
   - `texture_state_flags`, `texture_tile`
   - the **active** `NDSRendererTileState` (20 words) — only the active tile,
     not all eight
   - `light_color_1`, `light_color_2`, `light_dir_x/y/z` (E16 reads these)
3. Emit `sNdsNativeFighterEpochResolvedState[49]`. Deduplicate: if the 49 epochs
   share few distinct states the table collapses, and either way 49 x ~32 words
   is ~6 KB of ROM, which `PROJECT_GOAL.md` explicitly prefers over CPU time.
4. Keep emitting the delta table unchanged. It stays the oracle for the
   differ and the fallback path, and it is what proves the fold correct.

### Runtime

Behind `NDS_R2_FIGHTER_EPOCH_STATE`, default 0.

- In `ndsRendererExecuteNativeFighterOwnerProduction`, replace the two
  `ndsRendererNativeApplyStateSpan` calls with one install of the epoch's
  resolved state into `stats`.
- Install **without** calling `NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE`. That is
  the entire point: the prepare's inputs are being set to a known value, so the
  prepare's outputs for that epoch are also known.
- Bake the prepare's outputs per epoch too (`poly_fmt`, `scale_s/t`,
  `origin_s/t`, `offset`, `vertex_flags`, `textured`) and install them into
  `state->texture_prepare_*` with `texture_prepare_valid = TRUE`, so
  `PrepareProductionRun` short-circuits to the texture bind and
  `ndsRendererNativeBeginDirectBatch`. Those two have live GX side effects and
  must still run.

## 2a. CORRECTION — the epoch state is not purely static

§2 as first written claimed the generator knows each epoch's final state at build
time. **That is wrong**, and the error is load-bearing enough to have wasted a
build. Reading the runtime epoch loop:

```
ApplyStateSpan(before_state_first, before_state_count)   <- static
if (material_slot != NONE) ApplyMaterial(input->materials[slot])  <- RUNTIME INPUT
ApplyStateSpan(after_state_first, after_state_count)     <- static
shade
runs
```

Two consequences:

1. **The material is a per-frame input injected between the two spans.**
   `input->materials[]` is live — Task 39's hurt flash writes colour — so the
   fold cannot simply produce one resolved state per epoch. It must produce
   **two** snapshots (post-before and post-after) plus a mask of which fields the
   after-span writes, so the runtime can install snapshot A, apply the material,
   then re-apply only the after-span's fields. Ordering is then exact.

2. **`ndsRendererNativeApplyMaterial` invalidates the texture prepare too** — four
   `NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE` sites in its body, one of them
   beside `stats->prim_color = material->prim_w1`. E16's census measured
   `gNdsR2ShadeMaterialEpochs` at **27.7/frame, 57% of epochs**. So baking the
   deltas removes 127.3 invalidations a frame but leaves **27.7**, and
   `PrepareProductionRun` still takes its full path on every epoch carrying a
   material.

**Revised expectation:** E26 removes the 194.4 delta applications and most of the
invalidations, but not all of the 42,281 prepare cost. Size it as the replay
(65,026) plus the share of prepare attributable to non-material epochs, not the
full 107,307.

### E27 falls out of this and is much smaller

`prim_color` does not affect `poly_fmt`, the texture identity, or the UV scale /
origin / offset — those depend on texture state, not the primitive colour. It is
carried separately as `state->texture_prepare_material_color`. So the material's
invalidation of the *whole* prepare is broader than the data requires.

**Split the prepare's validity**: a material-only change should refresh
`texture_prepare_material_color` and leave the texture/UV half valid. That is a
narrow change to the invalidation macro plus one field, it needs no generator
work, and it targets the 27.7 material invalidations that E26 cannot reach.

Measure first, per the standing rules: count how many of the 46.4 full prepares a
frame are triggered by a material application with no accompanying texture-state
delta. If that number is large, E27 is a cheap cut and should land before E26.

## 3. Correctness gates, in order

1. **Fold equivalence, offline.** Before any runtime change: have the generator
   emit both the resolved state and a replay of its own delta table, and assert
   they match for all 49 epochs. A mismatch here is a generator bug and costs
   nothing to find.
2. **Structural counter** (E19's rule): `P0/P1HardwareTriangleCount` must equal
   the control's 181,440 / 173,502 over the same window.
3. **Engagement counter** (E15's rule): count installs per frame; expect 46.4,
   and expect `gNdsR2RunTexPrepCount` to fall toward the 16.3 reuse rate.
4. Boundary with the flag **on** (the arm that would ship), then a screenshot
   pair and the owner's visual approval — it is rendering-side.

## 4. Traps already paid for

- **ITCM has 1,024 bytes free** (E16 left it at 31,744/32,768). Anything on this
  chain needs `noinline` outside `.itcm.native_fighter`. The census and
  run-proof instruments can no longer be built into one ROM — measure with one.
- **Convert every path, not the one you are reading.** The fighter has four
  production emit paths; E16 was caught by editing two.
- **`stats` fields feed more than the prepare.** E24's early return skipped the
  action walk and Boundary caught the missing
  `gNdsRendererProfileSourceVertexLoadCount` even though geometry was identical.
  Anything the replay used to write that something else reads must still be
  written.
- **Do not weaken E12's texture staleness protocol.** The cache entry can rotate
  under a run; `gNdsR2TexMemoStaleCount` exists to catch it.

## 5. Expected result

The 194.4 delta applications, the 127.3 invalidations and the 46.4 re-prepares
go together. Against R2-03's remaining ~198,000 that is roughly half. It does not
close the phase on its own, and the honest position stays that R2-04's pose work
and further geometry reduction are also required.
