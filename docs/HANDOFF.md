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

Only `./emulators/melonds/melonDS.exe` and repo-owned
`./emulators/melonds-runners/slotN/melonDS.exe` may launch; external paths fail.
All TOMLs use one 488x675 vertical/equal/native-aspect, zero-gap, unswapped,
unfiltered, OSD-off window profile. Scripted launches apply the central profile
to their selected config, and DevFast checks every classified emulator TOML in
every registered worktree. `Set-MelonDSWindowConfig.ps1 -AllWorktrees` is only
for creation/repair, not a recurring manual audit.
Manual melonDS stays limited to 60 FPS with OpenGL 6x, `Volume = 256`, GDB off,
and ports `3333/3334`. Automation is unthrottled interpreter/software, muted
only at the host (`Volume = 0`), and uses root `4333/4334`, slot 0 `4323/4324`,
FGM slot 1 `3343/3344`, capture slot 3 `3363/3364`, audio slot 4 `3373/3374`,
M4 slot 8 `3413/3414`, or countdown slot 2 `4463/4464`. ROM audio stays live.

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
zero generic foreground staging/BG3 copies, live native-OAM commits, one affine
queue per live frame, one extra seed apply, no coverage failure, a nonidentity
transform, and at most 35,000 affine-update ticks. Live frames retain both
fighters and the exact 626-triangle fighter contract. Cumulative stage totals
reconcile as 42 lists/202 triangles per traversal plus the exact source-weapon
ledger; unmarked setup traffic is rejected.

Exact BattleShip Sprite manifests plus layered-SObj 4c/CI4/I8, TLUT, and
TEXSHUF decoding restore the countdown traffic light and GO art on the top
screen. The approved lower text HUD shows FPS, timer, Mario/Fox labels, stock,
and damage; it updates only when state changes and clears on VS Results.

The countdown path is now setup-converted main bitmap OAM. Integrated profile-1
frames 187–194 measured 11,584/11,584 median/P95 versus 1,863,232 foreground
ticks, with zero gameplay conversion/upload, final clear at frame 511, no
frame-600 idle tax, and complete captures in `artifacts/visibility`.

Exact `ftParamLockPlayerControl`/`ftParamUnlockPlayerControl` behavior now keeps
both fighters and Fox CPU inactive during Wait while the timer remains 3,600,
then unlocks controls and starts the timer at GO. The same canonical ROM passed
coherent pre-GO and post-GO windows.

Canonical/shipped ROM:

```text
smash64ds-battle-playable-hwtri.nds
14,362,624 bytes
SHA-256 57B85DDC6B2919D8962589188D6066F6CE6D0FD83B2F729175C9F339C8CCFAFD
```

Exact completed frames 438/439 in
`artifacts/visibility/2026-07-14_canonical_fast_frame438-439_200444-1174022-p660.png`
and its `_next` pair pass GO/timer/control/OAM state, full top-screen coverage,
green/detail, motion, named-region, horizontal-detail, and sky gates. The first
frame is published as `latest.png`. This is melonDS, not hardware, acceptance.

## Performance And Remaining Milestones

- Milestone 1: complete. Native BG2 affine update beats the 35K ceiling.
- Milestone 2: in progress. The generated AOT Mario/Fox owner exists, but its
  synchronized combined cost is about 431K versus 170–250K. The 17,704-byte
  transaction packet is tooling only until live device gates pass.
- Milestone 3: open. The current stage owner is about 801K versus 150–250K.
- Milestone 4: an exact RGB256 host generator qualifies for an eight-frame
  device falsifier; runtime palette mapping and zero gameplay preparation stay open.

Whole-frame presentation is about 10.3 FPS in the latest synchronized
`laboratory-profile-1` M2 window, not a canonical phase baseline. The accepted
slices are fidelity/ownership wins, not a 60 FPS claim. On exact ROM
`57B85D...FAFD`, natural Fox up-smash restores all 11 damage colliders with
zero mismatch and clears the no-damage flag. Manual repeat-hit confirmation
and a continuous natural-hit gate remain open.

The isolated one-minute state/memory gate passes from exact locked 1:00 through
Time Up and Results: logic=3892, timer=3600→0/3600, Fox CPU=7203 updates,
scene=22→24, safety=0, stale=0/0, and 171,916 bytes conservative reserve after
the resident BGM buffer. It is unthrottled lifecycle evidence, not a realtime
or exact canonical-duration qualification.

The source-backed 64,848-byte AOT FGM pack passes countdown and natural combat:
PublicExcited/3/2/1/GO play once, Mario KO plays exact `439/292/154`, all five
KO IDs are observed, handles recycle, and included failures are zero. Fox winner
16 naturally transitions to Results 22 with zero stream/cleanup faults. Pitch,
fork voice 685, other voices, and 24 observed unsupported calls remain.

## Execution Ownership

Use `P1_EXECUTION_BOARD.md` for the active lanes, worktrees, file locks,
dated gates, and acceptance decisions. This handoff does not maintain a second
task queue. Shared renderer-core work stays serialized in the live tree;
gameplay and audio return isolated commits plus reproduction evidence.

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

GBI/audio/renderer-parity fixtures, architecture, registry, canonical
runtime/parity, exact-frame Cut G capture, focused profile-1 pre/post-GO,
one-minute expiry/Results, and canonical profile-0 pre/post-GO pass.
Boundary/Latest were migrated because exact source locks invalidate their old
pre-GO motion assumption. Full Regression remains follow-up.

After successful verified progress, run
`.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final project action.
