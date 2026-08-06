# Grab hold pose — victim upside-down, torso in floor (stale root transform cache) — RESOLVED (2026-05-23)

## Symptom

During a standard grab hold (grabber in `CatchWait`, victim in `CaptureWait` / `CapturePulled`), the victim could appear **upside-down** with feet still roughly on the grabber's floor line and the upper body **clipped into the ground**. First and most obviously reported as **Ness → Donkey Kong** on Windows and Linux builds; once the fix landed it visibly corrected for other pairs too (Fox, Mario, DK cargo, …).

Not reliably reproducible locally — depends on draw / cache timing.

## Root cause

Hand-joint world transform reads were sampling a **stale grabber root DObj matrix**.

`ftCommonCapturePulledRotateScale` derives the victim's X/Z + rotation from the grabber's heavy-item hand joint world matrix via `func_ovl2_800EDBA4(hand)` (`gm/gmcollision.c:336`). That function lazily walks `dobj->parent` upward, stopping at the first ancestor whose `FTParts.unk_dobjtrans_0x5 != 0` and using its `mtx_translate` as the base for re-derivation.

Each frame `ftMainProcPhysicsMap` calls `ftParamsUpdateFighterPartsTransformAll(fp->joints[nFTPartsJointTopN])` (`ft/ftmain.c:1935`), which clears the cache word on TopN **and its descendants** — but **not** the root DObj sitting above TopN (`DObjGetStruct(fighter_gobj)`).

If the root's `unk_dobjtrans_0x5` was set on a previous frame, the next walk-up short-circuits at the root with that frame's stale matrix. X/Z/rotation reflect the previous body pose (often inverted relative to the current animation) while Y is independently snapped to the grabber's floor by `ftCommonCapturePulledProcMap` — hence "feet on ground, torso in floor."

A secondary leak: `mpCommonUpdateFighterSlopeContour` (`mp/mpcommon.c:401`) writes a fresh `rotate.x` from the floor angle when `FTSLOPECONTOUR_FLAG_FULL` is set, but didn't invalidate the FTParts caches afterward, so the new pitch couldn't reach downstream joint matrices on the same frame.

A third gap: scripts that cleared the FULL flag without zeroing `root_dobj->rotate.vec.f.x` left a body-tilt pitch that bled into unrelated grab/throw transforms.

This is class-of-bug latent on N64 because hardware/ucode timing implicitly refreshes the upstream caches via display walks; the port's `func_ovl2_800EDBA4` faithfully reproduces the lazy walk but doesn't get that incidental refresh.

## Fix

Add a single port-side helper, `ftParamInvalidateFighterRootChain(GObj *)`, that walks **TopN → root** clearing `transform_update_mode` and `unk_dobjtrans_word`. Then call it at the three places where a stale root cache or pitch can reach a downstream hand-matrix read:

| Site | File | Why |
|------|------|-----|
| Before `func_ovl2_800EDBA4(grabber_hand)` | `ft/ftcommon/ftcommoncapturepulled.c` (`ftCommonCapturePulledRotateScale`) | Core fix. Ensures the walk-up reaches the live root. |
| After `mpCommonUpdateFighterSlopeContour` writes root rotate.x | `mp/mpcommon.c` (`mpCommonUpdateFighterSlopeContour`) | Lets the new pitch propagate same-frame. |
| Inside a new helper `ftMainApplySlopeContourFlags` whenever FULL clears | `ft/ftmain.c` (motion-event `SetSlopeContour`, `ftMainSetStatus`, per-frame guard in `ftMainProcPhysicsMap`) | Pairs every FULL-flag clear with a pitch-zero + invalidate. |

Also added defensive NULL guards on `capture_gobj` / `capture_fp` / `throw_desc` in `ftCommonCapturePulledRotateScale`, `ftCommonCapturePulledProcMap`, `ftCommonCaptureWaitProcMap`, and `ftCommonThrownReleaseThrownUpdateStats` — all paths that can be reached after rollback resim or mid-frame coupling tear-down with a NULL capture pointer.

All changes are `#ifdef PORT` guarded; N64 codepaths are unchanged.

## Provenance

Bug diagnosed by Alex Vanderveen (TechnicallyComputers fork) in the netplay rollback context — see [`grab_coupling_geometry_handoff_2026-05-23.md`](https://github.com/TechnicallyComputers/BattleShip/blob/main/docs/grab_coupling_geometry_handoff_2026-05-23.md). This commit ports the **engine-only** subset of TC's `port-patches` commits `f71ef4b6c` and `f8ae7c9b5` (plus the NULL guards from `c47d17625`), with the following deviations:

- Renamed `ftParamInvalidateFighterTransformFromRoot` → `ftParamInvalidateFighterRootChain` (the function walks TopN → root, not from-root-downward; the original name reads the wrong direction).
- Dropped redundant explicit `unk_dobjtrans_0x5 = 0` / `unk_dobjtrans_0x7 = 0` writes — `unk_dobjtrans_word = 0` already covers them via the union (`ft/fttypes.h:996-1006`).
- Removed the misattributed `// 0x800EB528` ROM-address comment on the new helper (that address belongs to `ftParamsUpdateFighterPartsTransformAll`).
- Excluded TC's `mpCommonVec3fPointerUsable` magic-number pointer filter — papers over a separate bug rather than fixing it, and is not part of the grab pose chain.
- Excluded TC's pause-menu netplay / SSB64_NETMENU / character-specific rollback hunks from `ftmain.c` and `f8ae7c9b5` — those are not load-bearing for the offline grab pose bug and would pull in unrelated state.

Netplay rollback grab geometry refresh (`syNetRbSnapshotRefreshGrabCouplingGeometry`, finalize-order, per-tick refresh during forward sim, throw-desc snapshot) remains TC-fork-only and is not ported here — this codebase has no rollback netplay layer.

## Verification

- `cmake --build .claude/worktrees/grab-coupling-fix/build --target ssb64 -j 4` — clean build, no new warnings.
- Manual repro from the offline side requires creating the cache-aliasing window between victim and grabber physics, which only happens reliably under specific frame timing; both the JRickey main tree and this branch fail to reproduce locally. Multiple players have reported the pre-fix behavior on Windows/Linux builds (issue tracked in TC fork doc).
