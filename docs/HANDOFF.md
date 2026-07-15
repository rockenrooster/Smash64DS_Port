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

Temporary automation policy (2026-07-15): the public/manual battle ROM keeps
Fox classified as the original level-3 CPU but defaults only his decision/input
loop off. Reactions, damage, hitstun, animation, physics, collision, scoring,
and HUD/results CPU semantics remain live. CPU/lifecycle proofs explicitly set
`FoxCpuMode=1`; final P1 qualification remains blocked until Tyler asks to turn
the default back on and the CPU-on canonical ROM passes.

Restart pause point: source, verifier, docs, and repo-local skills are preserved
and focused static checks pass, but the corrected ROM has not been rebuilt. The
current 1:30-timestamp ROM is a manual-reference artifact only. Resume with `verify-battle-playable-realtime-harness.ps1 -SkipScreenshot`, then refresh its
hash/docs and snapshot; no repo process remains (external SuperMarioWar melonDS PID 36352 was left untouched).

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
SHA-256 E08C6C9EA29F671EE5AA9D9D6491B1B12E80A1DBC348AF99468CA72BE072425F
```

Exact completed frames 438/439 in
`artifacts/visibility/2026-07-15_canonical_fast_frame438-439_044100-8463313-p20112.png`
and its `_next` pair pass GO/timer/control/OAM state, full top-screen coverage,
green/detail, motion, named-region, horizontal-detail, and sky gates. The first
frame is published as `latest.png`. This is melonDS, not hardware, acceptance.

## Performance And Remaining Milestones

- Milestone 1: complete. Native BG2 affine update beats the 35K ceiling.
- Milestone 2: clean same-ROM Mode-8 is 413,504 ticks versus 170–250K. The
  whole-owner FIFO packet was 537,792 (+124,288), failed the <=337,472 gate,
  and is being removed; retain its exact host fixtures, not the runtime path.
- Milestone 3: the exact host packet is 10,076 bytes / 57 DObjs / 42 bindings /
  54 runs / 202 triangles, but production linkage is zero. Device acceptance
  still requires >=300K saving, <=500K, <=16 KiB, and dynamic invariance.
- Milestone 4: the exact water host fixture is 181,408 bytes and remains
  production-unlinked; the proposed pre-GO NitroFS payload is 258,048 bytes.
  Estimated reserve remains above 128 KiB, but M4 still requires measured
  reserve and zero post-GO conversion/allocation/upload/I/O/eviction/palette DMA.

The latest focused clean M2 A/B/A (`13506F55...B98589B`, frames 600–607)
reproduces A0=A1 exactly at 413,504 median / 413,632 P95 and a 17.1 FPS smoke
marker; rejected B is 537,792 / 537,856 and 16.7 FPS. This is laboratory
evidence, not a canonical phase baseline or a 60 FPS claim. The user confirms
Mario can damage Fox after the repair; continuous natural-hit coverage remains
open.

The isolated CPU-on one-minute state/memory gate passes from exact locked 1:00 through
Time Up and Results: logic=3892, timer=3600→0/3600, Fox CPU=7203 updates,
scene=22→24, safety=0, stale=0/0, and 172,024 bytes conservative reserve after
the resident BGM buffer. It is unthrottled lifecycle evidence, not a realtime
or exact canonical-duration qualification.

The platform gate proves only its exact upward-crossing rejection and can miss
an incorrect next-frame landing; Tyler's report remains open and its automation
needs continued-ascent/descending-crossing assertions. Fireball's early
submission/rebound automation passes, but its full-lifetime visual and
independent `0x47` matrix parity remain open. The isolated BattleShip
`mpprocess.c` LIVE build now has exclusive exact symbol closure after the
endpoint-world/common-local repair. It is not graduated: the first natural
DamageFall run stalled in breakpoint churn before any attack, the sparse
source-input verifier is newly repaired but unrun, and moving-wall/project-floor
providers plus coherent `mpcommon` remain open. Camera ±33.6° source parity and
opening-crowd acoustic fidelity also remain open. Exact evidence and owners live
on `P1_EXECUTION_BOARD.md`.

## Execution Ownership

Use `P1_EXECUTION_BOARD.md` for the active lanes, worktrees, file locks,
dated gates, and acceptance decisions. This handoff does not maintain a second
task queue. Shared renderer-core work stays serialized in the live tree;
gameplay and audio return isolated commits plus reproduction evidence.

Acceleration workflows are repo-local under `.agents/skills`; restart to discover them.

Keep comparisons on identical ROM hashes and synchronized windows. Require
counters, screenshots, semantic traces, and runtime state to agree.

## Focused Commands

```powershell
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 8 `
  -RendererBenchmarkSamples 8 -RendererBenchmarkStartFrame 600 -GdbPort 4333
.\scripts\verify-battle-playable-damagefall-recovery.ps1 -NoBuild
.\scripts\verify-mpprocess-private-import.ps1
```
Profile-1 laboratory targets never publish the shipped filename; refresh the user ROM only through the canonical Makefile parity rule.

## Verification State

`Full`, `Regression*`, and `P1Gate` are list-only; use focused checks, DevFast,
and Boundary, then run
`.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final project action.
