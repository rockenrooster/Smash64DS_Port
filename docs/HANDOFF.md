# Handoff

Use this file for exact current commands and artifact handoff only. Keep it
under 150 lines. `P1_EXECUTION_BOARD.md` is the only dynamic queue, the harness
registry decides Boundary/Latest membership, and `PORTING.md` owns history.

## Start Here

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-battle-playable-one-minute-match.ps1 -RunnerSlot 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

Single-run Codex verification reserves GDB ARM9/ARM7 ports `4333/4334`, leaving
`3333/3334` free for a manually opened melonDS instance. Runner slots retain
their isolated per-slot port mapping and stay host-muted (`Volume = 0`); ROM
audio remains enabled and the user's manual emulator config is untouched.

As of 2026-07-14, P1 and all automated iteration/stability soaks use the source
one-minute (`3600` tick) expiry-to-Results rule. Do not launch the obsolete
five-minute configuration.

Boundary and BoundaryDirect now select `battle_playable_realtime` (mode 163),
the canonical one-minute Mario-human/Fox-CPU scene. Modes 161/162 remain
registered for diagnosis but are no longer active Boundary/Latest entries:
their bounded input driver assumes pre-GO movement, which conflicts with the
restored BattleShip control lock. Do not add an unlock seed to revive them.

## Cut G Result

Cut G milestone 1 is accepted and graduated to the canonical/shipped profile-0
ROM. One complete source-rendered Dream Land wallpaper is retained in the
256x192 BG2 layer. Each live frame applies `grWallpaperCalcPersp` state through
native BG2 affine registers while the original display graph continues to draw
the stage, both fighters, effects, and foreground/interface SObjs.

The focused gate requires exactly one seed/capture, zero texture uploads,
failures, or fallbacks for the retained wallpaper, 49,152 initial BG2 pixels,
live BG3 traffic, one affine queue per live frame, one extra seed apply, no
coverage failure, a nonidentity transform, and at most 35,000 affine-update
ticks. Live frames retain both fighters and the exact 626-triangle fighter
contract.

Exact BattleShip Sprite manifests plus layered-SObj 4c/CI4/I8, TLUT, and
TEXSHUF decoding restore the countdown traffic light and GO art on the top
screen. The approved lower text HUD shows FPS, timer, Mario/Fox labels, stock,
and damage; it updates only when state changes and clears on VS Results.

Exact `ftParamLockPlayerControl`/`ftParamUnlockPlayerControl` behavior now keeps
both fighters and Fox CPU inactive during Wait while the timer remains 3,600,
then unlocks controls and starts the timer at GO. The same canonical ROM passed
coherent pre-GO and post-GO windows.

Canonical/shipped ROM:

```text
smash64ds-battle-playable-hwtri.nds
12,043,264 bytes
SHA-256 385B9F051C5CBB801089C69E13D49F9E0D19C07F1E4DA19DA943772B5553FC21
```

Fresh canonical capture `artifacts/visibility/latest.png` passes full top-screen
coverage, green/detail, paired motion, named-region, horizontal-detail, and sky
gates. This is melonDS/verifier acceptance, not physical-hardware acceptance.

## Performance And Remaining Milestones

- Milestone 1: complete. Native BG2 affine update is within the 5–35K target.
- Milestone 2: in progress. The generated AOT Mario/Fox owner exists, but its
  synchronized combined cost is about 431K versus the 170–250K target.
- Milestone 3: open. AOT DS-native complete-stage target is 150–250K ticks.
- Milestone 4: open. Sampled gameplay still converts textures and uploads
  `2 / 36,864` bytes; conversion must move entirely before gameplay.

Whole-frame presentation is about 10.3 FPS in the latest synchronized
`laboratory-profile-1` M2 window, not a canonical phase baseline. The accepted
slices are fidelity/ownership wins, not a 60 FPS claim. On exact ROM
`385B9F...FC21`, natural Fox up-smash restores all 11 damage colliders with
zero mismatch and clears the no-damage flag. Manual repeat-hit confirmation
and a continuous natural-hit gate remain open.

The isolated one-minute state/memory gate passes from exact locked 1:00 through
Time Up and Results: logic=3892, timer=3600→0/3600, Fox CPU=7203 updates,
scene=22→24, safety=0, stale=0/0, and 171,916 bytes conservative reserve after
the resident BGM buffer. It is unthrottled lifecycle evidence, not a realtime
or exact canonical-duration qualification.

## Execution Ownership

Use `P1_EXECUTION_BOARD.md` for the active lanes, worktrees, file locks,
dated gates, and acceptance decisions. This handoff does not maintain a second
task queue. Shared renderer-core work stays serialized in the live tree;
gameplay and audio return isolated commits plus reproduction evidence.

The rejected separate projection/modelview cut saved only 36,192 fighter ticks
in exact same-ROM A/B/A and was removed. A compile-time Mode-8 TRIANGLE_NOOP
floor then proved submission-only work can save at most 100K: fighters still
cost 331K with all run preparation/emission removed. The next M2 design must
also remove a large share of the measured ~178K matrix-preparation wall.

Keep comparisons on identical ROM hashes and synchronized windows. Require
counters, screenshots, semantic traces, and runtime state to agree.

## Focused M2 Commands

```powershell
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 8 `
  -RendererBenchmarkSamples 8 -RendererBenchmarkStartFrame 600 -GdbPort 4333
```

Profile-1 laboratory targets never publish the shipped filename. Refresh the
user ROM only through the canonical Makefile parity rule.

## Verification State

GBI fixtures, docs, architecture, registry, canonical DevFast build/capture/
parity, focused profile-1 pre/post-GO, and canonical profile-0 pre/post-GO pass.
Boundary/Latest were migrated because exact source locks invalidate their old
pre-GO motion assumption. Full Regression remains follow-up.

After successful verified progress, run
`.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final project action.
