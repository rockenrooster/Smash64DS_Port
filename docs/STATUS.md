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
zero Fox CPU processing. A post-GO sample proves Go, remaining + passed =
3,600, timer running, both unlocked, and natural CPU activity.

The canonical target enables the retained path at profile 0 and publishes the
user-facing ROM only through the Makefile parity rule:

```text
smash64ds-battle-playable-hwtri.nds
14,368,768 bytes
SHA-256 F8EFEE10ED15457CD79A9B71B9766B5247BE870C332FB12316431F8301A0A94A
```

Completed frames 438/439 in the dated
`artifacts/visibility/2026-07-15_canonical_fast_frame438-439_035112-0572048-p44488.png`
pair pass exact GO/timer/control/OAM state, visibility, detail, named regions,
motion, and sky coverage; frame 438 is `latest.png`. Acceptance is melonDS-only.
All generated screenshots belong under `artifacts/visibility`.

## P1 Release Matrix

| Area | Current state |
|---|---|
| Natural one-minute battle and Results | Natural 3,600→0/Time Up/22→24 gate passes; exact canonical-duration qualification remains |
| Gameplay | All-platform and 40-draw Fireball gates pass, but manual reports remain open; damage/default live policies are floor-only, DamageFly has no verified sample, and throw recovery is candidate-only |
| Renderer | M1/native countdown pass; M2 active; M3 slab specified; M4 static prewarm is feasible but exact full-water residency needs a new representation; pause ±33.6° source parity is unresolved |
| HUD/countdown | User-approved lower HUD and top countdown pass |
| Audio | Production phase/KO and isolated crowd-ACK gates pass; the user ROM has no blocking trace; ID626 PNT=1/LEN=3527 model passes with 2 guard samples/cycle; audible qualification remains open |
| Stability/memory | One full match passes with 172,024-byte conservative reserve and zero safety faults; repetition pending |
| Release evidence | Cut G exact-frame capture passes; final dated qualification capture, Full Regression, and exact-ROM retest pending |

Detailed owners, gates, blockers, and evidence live only on the execution board.

## Performance And Open Work

The latest focused profile-1 M2 A/B/A (`03950839...BEEF09B`, frames 600–607)
measures about 10.1–10.2 FPS, not a canonical phase baseline. Sampled lab gameplay
still reports positive texture conversion and two uploads totaling 36,864 bytes. Therefore:

- Milestone 2 remains ~431K versus 170–250K. Retained evidence supports about
  50–75K from the first hierarchy cut; the ≥80K keep gate remains unchanged.
- Milestone 3's strict eight-callback slab must save >=300K, reach <=500K, stay
  <=16 KiB resident, and add no BSS/heap; otherwise remove it.
- Milestone 4 can prewarm static Dream Land/fighter/Fireball/effect misses, but
  exact full-water residency is impossible in the retained layouts: the
  smallest exact visible RGB256 corpus is 903,168 bytes versus 524,288 bytes of
  total DS texture VRAM. M4 must reach an explicit representation checkpoint;
  post-GO conversion/allocation/upload/I/O still remain hard failures.

The 715-frame all-platform gate pins exact live geometry, Mario-only mask `0x7`,
three upward passes/zero accepts, nine reverse hits/landings, two side cycles, and three source Pass rejections. Fireball now
passes the BattleShip custom-`0x47` MVP path for 40/40 draws with zero mismatch,
1,757 units of natural travel, source rebound `55→46.75`, 80 triangles, and
222,736-byte reserve. Its dated capture is under `artifacts/visibility`. Both
original manual reports remain open.
The natural DamageFly gate currently times out without a verified sample, and
throw/release remains candidate-only. ID626's no-growth AOT PNT=1/LEN=3527
body passes a state-latch/restore model that rejects missing restore/wrong
PNT/LEN and exposes two guard samples per cycle; audible proof remains open. Camera passes
synchronized normal/front/±16.8°/±33.6° windows, but both ±33.6° screenshots
reproduce the reported pause-only wide-view occlusion. Normal camera remains
contained; identical BattleShip/N64 comparison is still needed before calling
the wide view a renderer defect.

A BattleShip ABI mismatch had made Fox's up/down-smash restore command disable
his damage colliders. The user confirms damage works on the repaired path;
natural Fox up-smash restores all 11 active colliders to Normal with zero
mismatch and clears the no-damage flag. Continuous natural-hit coverage remains.

## Verification

Passing checks for this checkpoint:

```powershell
.\scripts\check-docs.ps1
.\scripts\check-architecture.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\build-verify-profile.ps1 -Profile RegressionCore -VerifyStamp
.\scripts\verify-all.ps1 -Profile RegressionCore -NoBuild -DelaySeconds 3 -RunnerSlot 0
.\scripts\verify-boundary.ps1 -NoBuild -DelaySeconds 3 -RunnerSlot 0
```

Focused profile-1 and canonical profile-0 pre/post-GO runs pass. The one-minute
gate passes logic=3892, timer=3600→0/3600, scene=22→24, safety/stale=0, and
reserve=172,024. Fresh RegressionCore prebuild/stamp/runtime and mode-163 Boundary
pass; Full Regression follows. Platform/Fireball pass; DamageFly timed out,
throw/camera remain candidate-only, and crowd command telemetry is not acoustic proof.
Run the Lean snapshot only after all final checks and status inspection; run no project command after it.
