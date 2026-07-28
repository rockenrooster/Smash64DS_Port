# R2-03 E1 — the fighter shade loop is not a memo, and now we know

**Date:** 2026-07-28
**Phase:** R2-03 sizing, first candidate from
`ClaudeOpus5_R203_E0_FrameRebaseline_20260728.md` §4. No runtime change.
**Flag:** `NDS_R2_FIGHTER_SHADE_PROOF` (lab, default `0`).
**Verdict: REFUTED.** The shade loop's inputs change on **1,796 of 1,835
frames** and its outputs change on exactly the same 1,796. There is nothing to
memoise. One build and one run to find out.

---

## 1. The hypothesis

`ndsRendererNativeShadeProductionActions` is **48,422 ticks/frame**, the largest
non-idle, non-soft-float function in the frame. It re-lights every fighter dense
vertex every frame: a lit shade (LUT or per-channel), a material scale, a
modulate, and a pack to RGB15. R2-02 E3 had just shown that a whole class of
"this is dynamic" work in the stage had been constant since Task 51, so the
obvious question was whether this loop is the same shape.

Two hashes, because they imply different cuts:

- **input** — `epoch_policy`, `packet_mode`, the epoch's action span,
  `stats->prim_color`, `geometry_mode`, `light_dir_mask`, `light_color_mask`,
  `light_color_1/2`, `state->color_modulate`, and the prepared light direction.
  Everything the shaded colour is a function of besides the constant tables.
- **output** — `shaded_rgba` and `packed_color` across all 541 entries of
  `sNdsNativeFighterPreparedDense`.

Input constant → the loop is a memo on a dozen words. Input moving but output
constant → RGB15's 5 bits a channel are absorbing the motion, and the memo wants
a quantised key. Both moving → nothing to memoise.

## 2. The result

Whole one-minute Boundary match, `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1
NDS_R2_STAGE_ACTORS=1 NDS_R2_FIGHTER_SHADE_PROOF=1`, sampled at frame 1700.

```text
gNdsR2ShadeFrameCount        = 1,835
gNdsR2ShadeInputChangeCount  = 1,796   (97.9% of frames)
gNdsR2ShadeOutputChangeCount = 1,796   (97.9% of frames)
gNdsR2ShadeCallCount         =    49   (calls in the last frame)
```

**Both move, and they move together.** The prepared light direction follows each
fighter's root modelview, so it changes whenever a fighter or the camera moves —
which is nearly every frame of a match. And the output change count is not one
frame lower than the input's: RGB15 quantisation absorbs none of it, so a
quantised key would not help either.

Only 39 frames in 1,835 had a constant shade. A memo would hit 2.1% of the time
and add a key comparison to the other 97.9%.

**Both memo variants are dead.** This is the null the standing rules ask to be
written down: the biggest-looking function in the frame is big because of what
it computes, not because it recomputes it.

## 3. What that leaves

49 calls a frame at ~988 ticks each. The lever is the per-vertex arithmetic, not
its frequency:

- `ndsRendererHardwareLitShadeColorLut` / `...LitShadeColorPrepared`
- `ndsRendererHardwareScaleMaterialChannel5`, three times per vertex on the
  material path
- `RGB15` and `ndsRendererHardwareModulatePackedColor`

At 2.44 cycles per instruction this is compute-bound, not stall-bound, which
also means it will not respond to placement or traffic work — the two levers the
campaign has been leaning on. Task 90 already built and sized a light-shade LUT
(`sNdsRendererHardwareLightShadeCache`, 4,192 bytes); whether the remaining
per-vertex tail is worth a second pass should be sized against that history
before anything is written.

**The queue's second candidate is now first:** `ndsFighterMarioFoxDLAllDrawForSlot`,
**37,206 ticks/frame at 5.55 cycles per instruction** — the highest stall ratio
of any large function in the frame, and the generic tree walk plus per-frame
display-list revalidation that §7 names as R2-03's first deletion.
`NDS_TASK91_DRAW_PHASE_CENSUS` already splits it into
`gNdsTask91WalkTicks` / `gNdsTask91ValidateTicks`; size with that first.

## 4. Evidence

| SHA-256 (first 16) | file |
|---|---|
| `71EDB3DA6CCB312E` | `artifacts/performance/r2-03-e1-shade-falsifier-1700.json` |

ROM `builds/build-r2-03-e1-shadeproof`. The instrument stays behind
`NDS_R2_FIGHTER_SHADE_PROOF` (default 0) — it is two hashes and a frame hook,
and it is the right thing to re-run before anyone proposes memoising this loop
again.
