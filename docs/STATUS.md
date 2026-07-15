# Current Status

This is the short current-truth document. `P1_EXECUTION_BOARD.md` is the only
dynamic queue. Use `DIAGNOSTIC_REFERENCE.md` for marker definitions and append
history to `PORTING.md`.

## Direction

The target remains a 1:1 playable Nintendo DS source port of BattleShip Smash
64. Keep `decomp/` read-only, import coherent original TU groups, and graduate
only source-backed behavior proven in continuous runtime and captures.

## Current Boundary

The registry is authoritative:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Boundary/BoundaryDirect now select canonical `battle_playable_realtime`, mode
163. Latest keeps runtime, Title, and that same natural one-minute
Mario-human/Fox-level-3 scene. Exact BattleShip start locks made the former
161/162 bounded pre-GO input driver invalid; those modes remain diagnostic-only
and their selected live-hit coverage reduction is recorded in KNOWN_ISSUES.

The public/manual battle ROM temporarily keeps Fox classified as the original
level-3 CPU but pauses only decision/input. Reactions and gameplay stay live;
proofs enable the loop, and final P1 requires Tyler's request plus a CPU-on run.

Mode 163 imports the original fighter manager/main/CPU, animation/key/status
runtime, collision, camera, death/rebirth, IFCommon, normal moves, weapons,
effects, audio, and Results chain. Other mode-163 configurations still cover
stock/KO/rebirth and the one-minute Time Up -> VS Results lifecycle.

## Accepted Cut G Milestone

Milestone 1 is complete. Cut G retains exactly one full Dream Land wallpaper
seed in native 256x192 BG2 and drives it with live `grWallpaperCalcPersp`
transforms through DS affine registers. Stage geometry, animated foreground,
fighters, effects, and interface traversal remain live rather than flattened.

The profile-1 proof requires one seed/capture, no retained-wallpaper upload,
failure, or fallback, 49,152 BG2 pixels, zero generic foreground staging/BG3
copies, live native OAM, exact affine frame conservation, zero coverage failure,
nonidentity motion, and an affine update no greater than 35,000 ticks. Both
fighters retain the 626-triangle contract; cumulative stage totals are 42/202
per traversal plus the exact source-weapon ledger, with unmarked setup traffic
rejected.

BattleShip Sprite manifests and general 4c/CI4/I8 layered-SObj decode restore
the countdown traffic light and GO art on the top screen. The user-approved
lower text HUD shows FPS, timer, Mario/Fox labels, stock, and damage, updates on
state changes, and clears at VS Results.

The live countdown SObjs now use setup-converted main bitmap OAM instead of
full-layer software composition. Integrated frames 187–194 measured
11,584/11,584 native median/P95 versus 1,863,232 foreground ticks, with zero
gameplay conversion/upload, complete captures, final clear at frame 511, and no
frame-600 idle tax.

BattleShip's exact player-control gate is restored. A synchronized pre-GO
sample proves Wait, 3,600 remaining, timer stopped, both fighters locked, and
zero Fox CPU processing. The explicit CPU-on post-GO proof shows Go, remaining
+ passed = 3,600, timer running, both unlocked, and natural CPU activity.

The canonical target enables the retained path at profile 0 and publishes the
user-facing ROM only through the Makefile parity rule:

```text
smash64ds-battle-playable-hwtri.nds
14,368,768 bytes
SHA-256 E08C6C9EA29F671EE5AA9D9D6491B1B12E80A1DBC348AF99468CA72BE072425F
```

Completed frames 438/439 in the dated
`artifacts/visibility/2026-07-15_canonical_fast_frame438-439_044100-8463313-p20112.png`
pair pass exact GO/timer/control/OAM state, visibility, detail, named regions,
motion, and sky coverage; frame 438 is `latest.png`. Acceptance is melonDS-only.
All generated screenshots belong under `artifacts/visibility`.

## P1 Release Matrix

| Area | Current state |
|---|---|
| Natural one-minute battle and Results | CPU-on 3,600→0/Time Up/22→24 gate passes; public/manual default is temporarily CPU-paused, and final CPU-on qualification remains |
| Gameplay | Fireball early submission/rebound automation passes but full-lifetime visuals and independent `0x47` parity remain open; the platform gate has a next-frame landing blind spot; isolated LIVE `mpprocess` closure passes after endpoint/common repair, but sparse DamageFall runtime, moving-wall/project-floor parity, and coherent `mpcommon` remain open |
| Renderer | M1/native countdown pass; M2 Mode 8 is correct but above target and Mode 7 is rejected; M3 exact 12,663-byte core plus partial adapter compile; M4 exact 167,936-byte residency and 138-triangle draw packets await live integration; pause ±33.6° source parity is unresolved |
| HUD/countdown | User-approved lower HUD and top countdown pass |
| Audio | Production phase/KO and isolated crowd-ACK gates pass; the user ROM has no blocking trace; ID626 PNT=1/LEN=3527 model passes with 2 guard samples/cycle; audible qualification remains open |
| Stability/memory | One full match passes with 172,024-byte conservative reserve and zero safety faults; repetition pending |
| Release evidence | Cut G exact-frame capture passes; final dated capture, executable focused/DevFast/Boundary checks, and exact-ROM retest pending |

## Performance And Open Work

The latest detailed-ledger Mode-8 A0/A1 is 477,152/477,376 ticks and renders
both fighters correctly. Rejected Mode 7 is 518,336/518,784 and produces blank
fighters; its runtime path must be removed. The ledger-off accepted reference
remains about 413.5K. A direct-contract design may remove an estimated 62–75K,
but it is not implemented or measured. The last control smoke window is about
17.7 FPS, draw 1.646M, and loop 2.241M ticks. Therefore:

- M2 has no accepted cut meeting its 170–250K target.
- M3's exact 12,663-byte / 8-callback / 57-DObj / 42-binding packet and
  renderer core compile. The partial adapter helpers compile, but display-loop
  interception, link, timing, counters, and screenshot remain unfinished.
- M4's exact one-pass residency is 167,936 bytes. Its 68-cell/138-triangle draw
  helper has zero draw-time upload, I/O, or allocation in the ARM gate, but
  live prepare/draw/teardown, reserve, and post-GO fence proof remain open.

Detailed renderer measurements and falsification gates live only in
`OPTIMIZATION_ROADMAP.md`.

The 715-frame platform route pins exact live geometry and crossing-frame
rejections, but it does not require continued ascent or a descending plane
crossing before accepting the next landing. It therefore does not close the
reported early-landing/jump-suppression symptom. Fireball selects the current
custom-`0x47` path for 40/40 early draws with zero internal mismatch,
1,757 units of natural travel, source rebound `55→46.75`, 80 triangles, and
222,736-byte reserve. The gate lacks an independent BattleShip matrix oracle,
full 140-tick collision trace, and Fireball ROI capture; the manual report stays open.
The natural DamageFly gate currently times out without a verified sample, and
throw/release remains candidate-only. ID626's no-growth AOT PNT=1/LEN=3527
body passes a state-latch/restore model that rejects missing restore/wrong
PNT/LEN and exposes two guard samples per cycle; audible proof remains open. Camera state synchronization passes across
normal/front/±16.8°/±33.6° windows, but both ±33.6° screenshots
reproduce the reported pause-only wide-view occlusion. Normal camera remains
contained; identical BattleShip/N64 comparison is still needed before calling
the wide view a renderer defect.

A BattleShip ABI mismatch had made Fox's up/down-smash restore command disable
his damage colliders. The user confirms damage works on the repaired path;
natural Fox up-smash restores all 11 active colliders to Normal with zero
mismatch and clears the no-damage flag. Continuous natural-hit coverage remains.

## Verification

Checkpoint commands:

```powershell
.\scripts\check-docs.ps1
.\scripts\check-architecture.ps1
.\scripts\verify-mpprocess-private-import.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -NoBuild -DelaySeconds 3 -RunnerSlot 0
```

`Full`, `Regression*`, and `P1Gate` are list-only; never run or prebuild them. Remaining blockers live on the execution board; run the Lean snapshot last.
