# Grab hold pose — Euler-extract gimbal-lock exact-equality precision bug — RESOLVED (2026-05-23)

## Symptom

During a standard grab hold (`CatchWait` on grabber, `CapturePulled` / `CaptureWait` on victim), the victim character would render **tilted ~50° forward and ~50° rolled to the side**, sometimes reading as "upside-down with feet on grabber's floor line and torso clipped through the floor" for tall victims. Most reliably reproducible **Pikachu → Samus on Dreamland**; Windows and Linux reporters most affected, macOS could reproduce but less consistently. Did **not** reproduce with DK as either grabber or victim despite TC's handoff doc highlighting DK as the canary case.

## What it isn't

This bug is **distinct** from (and was misdiagnosed as) the stale FTParts transform cache hypothesis covered in [`grab_pose_stale_root_transform_2026-05-23.md`](grab_pose_stale_root_transform_2026-05-23.md). That earlier fix landed the same day (PR #204) but turned out to be a no-op for the offline reproducer — runtime tracing (`SSB64_GRAB_TRACE=1`) showed `root_cache_pre = 0` and `hand_cache_pre = 0` on every single tick across 975 frames of a held grab, meaning the invalidate had nothing to clear. The invalidate is retained as defensive coverage for hypothetical save-state / rollback restore paths.

## Root cause

`func_ovl2_800EDA0C` in `gm/gmcollision.c` decomposes a 4×4 rotation matrix into XYZ Euler angles. It handles the gimbal-lock case (where the matrix's X-axis row aligns with world ±Z, making yaw ±90° and roll undefined) with an **exact float-equality check**:

```c
if ((dst[0][2] == -1.0F) || (dst[0][2] == 1.0F))
{
    // gimbal-lock branch: rotate.z = 0, compute rotate.x from row 1
}
else
{
    rotate->y = syUtilsArcSin(-dst[0][2]);
    rotate->x = syUtilsArcTan2(dst[1][2], dst[2][2]);
    rotate->z = syUtilsArcTan2(dst[0][1], dst[0][0]);   // garbage when row 0 is near-aligned
}
```

For the grab path, `ftCommonCapturePulledRotateScale` extracts the grabber's heavy-item hand-joint world matrix into Euler form, then writes it as the victim's root rotate. The hand matrix is built by `func_ovl0_800C9A38` composing the full joint chain (grabber root → spine → arm → hand local). When the chain happens to produce a near-axis-aligned result — common for "palm-down hand pose" with a `lr=1, rotate.y = π/2` grabber facing — the X-axis row comes out *close* to `(0, 0, 1)` but the small chain-multiply rounding leaves residual noise:

```
mtx_row0  = (0.0025434317, -0.0029893226, 0.9999909997)
norm_row0 = (0.0025434350, -0.0029893266, 0.9999923110)
```

`0.9999923 != 1.0F` on x86_64 / arm64 with clang, so the equality check fails, the general extraction branch runs, and `atan2(-0.00299, 0.00254) ≈ -0.866` is extracted as `rotate.z` (≈ -49.6° roll) from pure floating-point quantization noise. Combined with the expected -90° yaw and the animation's natural pitch this composes into a visibly tilted held victim. Tall characters (Samus) make the tilt obvious; squat characters (Kirby, DK) mask it.

**Why N64 was unaffected:** IDO 7.1 / MIPS float math at the relevant chain depth apparently rounds the analogous accumulation to exactly `1.0F` often enough that the gimbal-lock branch fires reliably. The vanilla check was always fragile — it depended on luck-of-rounding — but the lucky-rounding regime held on hardware. Different toolchain, different rounding, different luck. Same source of bug as `audio_envmix_ramp_overflow_2026-05-08.md`: arithmetic shape that worked on N64 RSP/CPU is precision-sensitive when reimplemented on modern FPUs.

## Fix

Replace the exact-equality gimbal-lock check in `func_ovl2_800EDA0C` with a tolerance under `#ifdef PORT`:

```c
#ifdef PORT
if ((dst[0][2] <= -0.9999F) || (dst[0][2] >= 0.9999F))
#else
if ((dst[0][2] == -1.0F) || (dst[0][2] == 1.0F))
#endif
```

Threshold `0.9999F` means "within ~0.81° of pure ±90° yaw." Genuinely-rotated grab poses won't be inside this window; only chains that the original code *intended* to detect as gimbal-locked. The inner `dst[0][2] == -1.0F` branch selector is updated to `<= -0.9999F` to match. N64 codepath unchanged.

After fix, the same matrix on the Pikachu → Samus repro produces `victim_out_rot = (-0.000457, -1.570796, 0.000000)` — clean upright, observed across 975 traced frames. Visual confirms.

## Diagnostic trail

Process to nail this down (reusable for similar precision-sensitive port bugs):

1. **Wire trace** in `ftCommonCapturePulledRotateScale` under `SSB64_GRAB_TRACE=1` env var. First pass logged what we *thought* was the hand world matrix — read from `attach_dobj->user_data.p->mtx_translate` — but the US path uses `func_ovl0_800C9A38` which writes only to a local `Mtx44f`, not the cached `mtx_translate` (only the animlocks branch caches), so we saw all-zero matrices. Wrong field.
2. **Re-trace** with a `f32 (*trace_mtx)[4]` pointer assigned to the actual local matrix in each `#if defined(REGION_US)` branch. Got real basis vectors.
3. **Cache hypothesis falsified**: `root_cache_pre=0, hand_cache_pre=0` on every frame. The earlier-PR invalidate was provably a no-op.
4. **Re-trace** with high-precision (`%.10f`) matrix entries and explicit re-computation of the normalized basis vector + the `dst[0][2] == ±1.0F` check inside the trace. Saw `0.9999923110` value never hitting equality; saw the `atan2(-0.00299, 0.00254) → -0.866` chain end-to-end matching the observed `victim_out_rot.z`. End of investigation.

## Audit hook

Any other matched-decomp function that compares a derived float against an exact constant (`== 1.0F`, `== 0.0F`, `== ±constant`) is a port-stability landmine. Two classes that matter:
- **Branch selectors at gimbal-lock / discontinuity boundaries** (this bug, anywhere the rotation/normal handling switches branches at exact axis alignment).
- **Counter-shaped equality checks** with values produced by float arithmetic (e.g. comparing accumulated sums to a target).

Both want tolerance-based comparison. Audit candidates: any `gm/gmcollision.c` matrix-decomp routine, anything that does `if (f == 0.0F)` after a multiplication chain, anything in physics integration comparing time-step accumulators.

## Sweep follow-up (2026-05-23, same day)

After confirming the offline reproducer was fixed solely by the gimbal-lock tolerance change here (PR #205), we ran the audit hook against the rest of the codebase. JR's repro on `main` (which had PR #204's invalidate machinery applied) **still produced the inverted victim** — so PR #204's `ftParamInvalidateFighterRootChain`, `ftMainApplySlopeContourFlags`, per-frame stale-FULL guard, NULL guards on capture/throw paths, and `mpCommonUpdateFighterSlopeContour` invalidate were demonstrably not load-bearing for the offline bug they were diagnosed against (and the runtime trace had already shown `root_cache_pre = 0, hand_cache_pre = 0` on every one of 975 frames, confirming the invalidate had nothing to clear). Reverted in `398f02978` on the same branch as part of this follow-up.

Sweep scope: `decomp/src/{gm,ft,mp,wp,it,gr,sc,lb,ef,mn,sys/audio}/` and `port/`. libultraship out of scope (separate codebase). Three confirmed sibling sites with the same root-cause class:

| Site | Shape | Visible blast |
|---|---|---|
| `gr/grcommon/grsector.c:620` `func_ovl2_801070A4` | Matrix-to-Euler extractor, `(vec3->z == ±1.0F)` exact-equality gimbal-lock check. `vec3` from `lbCommonCross3D` + `syVectorNorm3D`. | Sector Z Arwing laser orientation can be wrong by up to 90° |
| `ft/ftparam.c:2794` (two-bone IK) | Same matrix-to-Euler shape, `(inverse_xy_2 == ±1.0F)`. Value from `sqrtf` chain. Drives `child1_dobj` rotate. | Arm IK chain (grab/throw target tracking) produces corrupted joint rotations |
| `gm/gmcollision.c:591` `func_ovl2_800EE050` | Different shape: `square == 1.0F` chooses between identity matrix and a general formula whose else branch divides by `(1.0F - SQUARE(temp))` and `(1.0F + dist.x)` — both collapse to zero exactly when the equality check is supposed to fire. Original inline comment: "JUST BARELY matches" (the IDO author was aware). | Collision capsule rotation matrix gets inf/nan when the input vector is near-±X-axis-aligned — likely cause of any "stretched / missing hitbox" report at near-horizontal angles |

All three fixed with the same `<= -0.9999F || >= 0.9999F` (or `>= 0.9999F` for the single-sided `square` check) tolerance pattern under `#ifdef PORT`. ~0.81° band around the singularity, narrow enough that genuine rotations don't trip it.

**Dismissed as a different class** (defensive divide-by-zero guards, not branch-mis-selection): `lb/lbparticle.c:1822 tm == 0.0F`, `lb/lbcommon.c:411 magnitude == 0.0F`, `lb/lbcommon.c:2173 magnitude == 0.0F`. IEEE-754 `sqrtf` preserves exact zero so these equality checks fire correctly for natural inputs; the worst case is a subnormal-flushing edge case which is a separate hardening discussion. `wp/wplink/wplinkboomerang.c:386 vel == 10.0F` was flagged as possibly broken but is self-correcting on the next physics tic via the `< 10.0F` clamp in `wpLinkBoomerangSubVelSqrt`.

**Audit coverage:** `gm/ ft/ mp/ wp/ it/ gr/ sc/ lb/ ef/ mn/ sys/audio/ port/`. Not yet swept: `mv/`, `db/`, libultraship.
