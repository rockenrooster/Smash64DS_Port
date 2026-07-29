# R2-03 E32 — fold the hitlag shuffle instead of surrendering the native owner

**Date:** 2026-07-29
**Status:** **KEEP candidate. Performance and engagement verified; the visual
gate needs the owner.** Flag `NDS_R2_FIGHTER_SHUFFLE_FOLD`, default **0** — not
graduated into the published or tick-HUD blocks yet.

## The defect

`reloc_backend_renderer_dl.c` disabled the entire native fighter owner whenever

```c
(fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u)
```

dropping the fighter to the generic DObj/display-list interpreter for the
duration of **every hit**. E31 measured it: 5 `AnimLock` fallbacks over frames
460..500, one per burst frame, and a split counter attributed **5 to
`shuffle_tics` and 0 to `is_use_animlocks`**.

`shuffle_tics` is SSB64's hitlag shuffle — `fttypes.h:1146` "Model shift timer",
set from `ftParamGetHitLag` in `ftparam.c:236`. So the renderer was giving up its
fast path precisely when the game is at its most active.

## Why it never needed to

`ftdisplaymain.c:1205`:

```c
if (fp->shuffle_tics != 0) {
    syMatrixAdvanceW(m, gSYTaskmanGraphicsHeap);
    syMatrixTra(m, shuffle.x, shuffle.y, 0.0F);
    gSPMatrix(..., G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
}
ftDisplayMainDrawAll(fighter_gobj);
if (fp->shuffle_tics != 0) { gSPPopMatrix(..., G_MTX_MODELVIEW); }
```

One push, one whole-model translate by `(x, y, 0)`, one pop. It touches no
geometry, no material, no animation, no per-joint transform. And `lbcommon.c:1627`
gives the same effect in the form the port actually needs:

```c
f[3][0] += dFTDisplayMainShufflePositions[fp->is_shuffle_electric][fp->shuffle_frame_index].x;
f[3][1] += ...y;
```

— an add into the translation row of the part's **world** matrix, before the
camera is applied.

## The change

`ndsRendererAdapterPrepareNativeOwnerMatrices` already builds exactly that:

```c
ndsRendererAdapterBuildDObjWorldMatrix(bindings[i], &world);
/* E32: world.m[3][0] += shuffle_x;  world.m[3][1] += shuffle_y; */
ndsRendererMtxMulAffine20p12(&world, &camera_modelview, &modelviews[i]);
```

The offset goes in at the same point, in the same space, as the source's own
attached-DObj path — so this is **mechanically equivalent by construction**, not
an approximation of the effect. `shuffle_tics` then comes out of the eligibility
disjunction. `is_use_animlocks` stays in it: the census measured it firing zero
times in the Boundary scene, so leaving it conservative costs nothing.

Conversion is `(s32)(offset * 4096.0F)`: `NDS_R2_RS20P12` converts 16.16 to
20.12, so one float world unit is 4096, and the table's ±50/±100 become
±204,800/±409,600.

## Evidence

Paired 128-frame A/B against E30, `NDS_TICK_HUD_DRAW=0` on both arms.

**Engagement, read from the same run** (`-ExtraGlobals`):
`gNdsR2ShuffleFoldedFrames = 20` — two fighters across ten burst frames, exactly
as predicted. An optimisation behind a flag that silently never fires is
indistinguishable from one that fires and saves nothing; this one fired.

**The bimodal distribution collapsed:**

| | FTR P50 | FTR P95 | FTR max | frames > 600k | WORK P95 |
|---|---:|---:|---:|---:|---:|
| E30 | 404,672 | 913,920 | 918,976 | **11** | 1,467,840 |
| **E32** | 408,512 | **412,992** | **414,656** | **0** | **1,381,120** |

`FTR` P95 −500,928. Whole-frame `WORK` P95 **1,467,840 -> 1,381,120**, and
frames over the 1,120,000 gate **35/128 -> 27/128**. VBlank histogram
`2:472 3:87 4:4 5+:2` -> **`2:489 3:72 4:4 5+:1`**.

**The cost on ordinary frames is real but small.** `FTR` median +3,456 on
106/128 frames: `ndsRendererAdapterSetShuffleOffset` runs per fighter per frame
and the two adds run per binding (~32/frame). That is at the 5,000–7,000 noise
floor and is bought back many times over by the tail.

## VERDICT 2026-07-29: DO NOT GRADUATE — visual regression, mechanism named

The visual gate below was answered by measurement rather than by eye, and **the
answer is no.** The flag stays default 0 and E32 is not a graduation candidate in
its current form.

**Method.** Both arms built to their own `NDS_OUTPUT_ROOT` under `builds/`, then
frame-locked captures of the *same* presented frames from each:
`capture-melonds.ps1 -ExactFirstFrame N -ExactSecondFrame N+1 -FoxCpuMode 1
-SoftwareRenderer`. Hitlag frames 480/481 (inside the `FTR` burst), control
frames 510/511 (`FTR` at median, so neither arm falls back).

**The control proves the comparison is sound.** On non-hitlag frames the two arms
differ by **188 pixels of 276,224 (0.068%)**, and the bounding box is confined to
the bottom-screen `FPS`/`UP` readout, which necessarily differs because the arms
run at different speeds. Everything else is pixel-identical.

**The gameplay state is bit-identical.** The `CUTG_EXACT` rows agree byte for
byte between arms at every captured frame, state hash included. E32 is
render-side only, exactly as designed.

**The hitlag frames are not.**

| frame | differing pixels | share |
|---|---:|---:|
| 480 (hitlag) | 1,826 | 0.661% |
| 481 (hitlag) | 1,536 | 0.556% |
| 510 (control) | 188 | 0.068% |
| 511 (control) | 188 | 0.068% |

**What changed is the hurt flash, not the shake.** `artifacts/visibility/e32-compare-480.png`
and `e32-compare-481.png` stack the arms side by side, magnified, on the struck
fighter. The reference arm renders Fox **light grey and legible**; the candidate
renders the same frame **dark maroon**, losing the body's readability.

**The mechanism follows from E34.** `prim_color` and `env_color` are the *only*
per-epoch state that varies at runtime, and what varies them is Task 39's hurt
flash writing `input->materials[]` live. During animlock the reference arm falls
back to the generic path (E31) and gets that path's flash handling; the candidate
keeps the native owner and applies the flash material differently. The fold's
arithmetic is fine — `gNdsR2ShuffleFoldedFrames = 20` and the state hashes match
— but keeping the fighter on the native owner during hitlag exposes a
colour-handling difference the fallback was hiding.

**So E32 is really two changes and only one of them was measured.** It was framed
as "fold the shuffle into the world matrix", but its actual effect is "stop
falling back during animlock", and the fallback was also masking a material
seam. Fix the native owner's hurt-flash colour to match the generic path, then
re-run these four captures; the tick win is real and worth returning for.

`PROJECT_GOAL.md` requires the result stay "recognizable, readable during
gameplay". A struck fighter turning dark maroon is a readability change on
exactly the frames a player is reading most closely, so this is not a
budget-and-approve cosmetic delta.

## What was NOT verified when this was written

**The visual gate.** A zero offset would flatten `FTR` exactly the same way, by
simply not shuffling — so "the burst disappeared" is *not* evidence the effect
survived. The engagement counter proves the code ran and the arithmetic is
consistent with the port's 20.12 world scale, but **only a screenshot or play
test can confirm the fighter still visibly shakes on hit, by the right amount and
in the right direction.** This is rendering-side; it gates on the owner's
approval per the standing rules, and the flag stays default-0 until then.

Specifically worth watching for:
- the shake is present at all during hitlag;
- its amplitude matches Runtime 1 (build the same ROM with
  `NDS_R2_FIGHTER_SHUFFLE_FOLD=0` for the comparison arm — that arm is the
  generic path, which is correct by construction);
- electric hits shake horizontally rather than vertically
  (`is_shuffle_electric` selects the second table row).

**Boundary on the enabled arm: PASSED.** `verify-all.ps1 -Profile Boundary` with
`NDS_R2_FIGHTER_SHUFFLE_FOLD` defaulted to 1 for the duration of the run —
"Boundary verification profile passed."

A first attempt at this was thrown away, and the reason is worth keeping: I
started that run with the default flipped to 1 and then reverted it to 0 while
the run was still building. `make` re-reads the Makefile on every invocation and
the profile runs several, so the flag state was not knowable and the (passing)
result could not be trusted either way. **Never edit a build flag while a
verifier is running — the tree a verifier reads has to be still.** The run above
was left completely alone until it finished, and the default reverted after.

The committed state has the flag **default 0**, where every hunk is inside
`#if NDS_R2_FIGHTER_SHUFFLE_FOLD` and the eligibility condition falls through to
its original `#else`, so the shipping configuration is unchanged by construction.

## Where the gate stands

| | P50 | P95 | gate |
|---|---:|---:|---:|
| WORK, instrument off, E32 | ~1,007,000 | **1,381,120** | 1,120,000 |

**1.23x, a gap of 261,120** — from 1.37x when the R2-08 readiness table was first
written. The remaining tail is `SRC` asset loading (Task 75 E0, sized ~103,488)
plus `OTHR`/`MISC`.
