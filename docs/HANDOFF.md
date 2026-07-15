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
to their selected config. DevFast validates the shared profile and repo-local
launcher policy without auditing mutable emulator TOMLs. Use
`Set-MelonDSWindowConfig.ps1 -AllWorktrees` only for explicit creation/repair;
`check-melonds-policy.ps1 -AuditLocalConfigs` is the opt-in drift audit.
All generated screenshots must resolve under `artifacts/visibility`.
Manual melonDS stays limited to 60 FPS with OpenGL 6x, `Volume = 256`, GDB off,
and ports `3333/3334`. Automation is unthrottled interpreter/software, muted
only at the host (`Volume = 0`), and uses root `4333/4334`, slot 0 `4323/4324`,
FGM slot 1 `3343/3344`, capture slot 3 `3363/3364`, audio slot 4 `3373/3374`,
M4 slot 8 `3413/3414`, or countdown slot 2 `4463/4464`. ROM audio stays live.

As of 2026-07-15, P1 and all automated iteration/stability soaks use the source
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
256x192 BG2 layer and updated through live `grWallpaperCalcPersp` affine state;
the original display graph still draws stage, fighters, effects, and interface.
The gate proves one 49,152-pixel seed, no retained-path upload/failure/fallback,
nonidentity motion at <=35K ticks, both fighters / 626 triangles, and cumulative
stage 42/202 per traversal plus exact source-weapon traffic.

Source countdown assets use setup-converted main bitmap OAM at 11,584/11,584
median/P95 with zero gameplay conversion/upload and clean teardown. Countdown
stays top-screen; the approved FPS/timer/player/stock/damage text stays on the
bottom. Source Wait locks both fighters/CPU at 3,600 and GO unlocks control and
starts the timer.

Canonical/shipped ROM:

```text
smash64ds-battle-playable-hwtri.nds
14,368,768 bytes
SHA-256 F8EFEE10ED15457CD79A9B71B9766B5247BE870C332FB12316431F8301A0A94A
```

Exact completed frames 438/439 in
`artifacts/visibility/2026-07-15_canonical_fast_frame438-439_035112-0572048-p44488.png`
and its `_next` pair pass GO/timer/control/OAM state, full top-screen coverage,
green/detail, motion, named-region, horizontal-detail, and sky gates. The first
frame is published as `latest.png`. This is melonDS, not hardware, acceptance.

## Performance And Remaining Milestones

- Milestone 1: complete. Native BG2 affine update beats the 35K ceiling.
- Milestone 2: ~431K versus 170–250K; evidence supports about 50–75K from the
  first hierarchy cut. Keep only ≥80K with slots Mario 16–23 / Fox 16–17.
- Milestone 3: strict eight-callback slab must save >=300K, reach <=500K, stay
  <=16 KiB resident, and add no BSS/heap.
- Milestone 4: static prewarm is feasible; exact visible full-water residency is
  not (903,168 bytes minimum versus 524,288 total texture VRAM). Require a new
  representation checkpoint plus zero post-GO conversion/allocation/upload/I/O.

The latest focused profile-1 M2 A/B/A (`03950839...BEEF09B`, frames 600–607)
measures about 10.1–10.2 FPS, not a canonical phase baseline. The accepted
slices are fidelity/ownership wins, not a 60 FPS claim. The user confirms Mario
can damage Fox after the repair; continuous natural-hit coverage remains open.

The isolated one-minute state/memory gate passes from exact locked 1:00 through
Time Up and Results: logic=3892, timer=3600→0/3600, Fox CPU=7203 updates,
scene=22→24, safety=0, stale=0/0, and 172,024 bytes conservative reserve after
the resident BGM buffer. It is unthrottled lifecycle evidence, not a realtime
or exact canonical-duration qualification.

Platform semantics pass all three lines in 715 frames: exact live geometry, Mario-only
mask `0x7`, three upward passes/zero accepts, nine reverse hits/landings, two side cycles, and three Pass rejections. Fireball passes 40/40 custom
`0x47` source-MVP draws, zero mismatch/reject/translation drift, 1,757 units of
natural travel, rebound `55→46.75`, and reserve 222,736; its dated capture is in
`artifacts/visibility`. Their original manual reports remain open. Natural DamageFly currently
times out; throw/release remains candidate-only. The live damage/default map
policies are floor-only; BattleShip wall/ceiling Run state and connected-floor
adjacency are the next coherent `mpprocess`/`mpcommon` graduation. Isolated crowd ACK diagnostics pass; the user ROM has no blocking trace.
ID626 now uses a no-growth AOT PNT=1/LEN=3527 body. Its stateful host model
rejects missing restore/wrong PNT/LEN and records two guard samples of alignment
debt per 28,216-sample cycle; runtime passes but acoustic fidelity is manual. Synchronized
camera state passes, normal/front/±16.8° images are clean, and both ±33.6° pause
views reproduce the reported wide-view occlusion. Source parity is unresolved;
do not change renderer behavior without an identical BattleShip/N64 comparison.

The source-backed 64,848-byte AOT FGM pack passes countdown and natural combat:
PublicExcited/3/2/1/GO play once, Mario KO plays exact `439/292/154`, all five
KO IDs are observed, handles recycle, and included failures are zero. Fox winner
16 naturally transitions to Results 22 with zero stream/cleanup faults. Pitch,
fork voice 685, other voices, and 24 unsupported calls remain. Retest the
opening crowd audibly on the rebuilt exact canonical ROM before closing it.

## Execution Ownership

Use `P1_EXECUTION_BOARD.md` for the active lanes, worktrees, file locks,
dated gates, and acceptance decisions. This handoff does not maintain a second
task queue. Shared renderer-core work stays serialized in the live tree;
gameplay and audio return isolated commits plus reproduction evidence.

Keep comparisons on identical ROM hashes and synchronized windows. Require
counters, screenshots, semantic traces, and runtime state to agree.

## Focused Commands

```powershell
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 8 `
  -RendererBenchmarkSamples 8 -RendererBenchmarkStartFrame 600 -GdbPort 4333
.\scripts\verify-battle-playable-platform-semantics.ps1 -NoBuild
.\scripts\verify-battle-playable-fireball-render.ps1 -NoBuild
.\scripts\verify-battle-playable-damagefall-recovery.ps1 -NoBuild
.\scripts\verify-battle-playable-throw-release-recovery.ps1 -NoBuild
.\scripts\verify-battle-playable-camera-containment.ps1 -NoBuild
.\scripts\verify-battle-playable-crowd-envelope-timing.ps1 -NoBuild
```

Profile-1 laboratory targets never publish the shipped filename; refresh the user ROM only through the canonical Makefile parity rule.

## Verification State

GBI/audio/renderer-parity fixtures, architecture, registry, canonical
runtime/parity, exact-frame Cut G capture, focused profile-1 pre/post-GO,
one-minute expiry/Results, canonical profile-0 pre/post-GO, platform, exact Cut
G, 40-draw Fireball source-MVP, and crowd command-ACK gates pass. DamageFly
timed out; throw/camera are candidate-only and crowd acoustic fidelity is open.
A fresh RegressionCore prebuild/stamp/runtime and active mode-163 Boundary pass;
Full Regression follows. After successful verified progress, run
`.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final project action.
