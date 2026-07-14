# Handoff
Use this file for the active handoff only. Keep it under 150 lines. The harness
registry decides the current Boundary/Latest profile; this file summarizes what
to do next.

## Start Here

1. Check the registry view:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

2. Expected Boundary/Latest entries:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-battle-playable-harness.ps1 -DelaySeconds 3
```

3. Read `docs/STATUS.md` for current truth, `docs/DIAGNOSTIC_REFERENCE.md` for
full marker strings, and `docs/PORTING.md` for history.

## Current Boundary

Modes `161/162` are the bounded natural-combat pair. In the default
original-manager build they prove natural Mario/Fox combat on the Pupupu
Mario/Fox battle root: Wait -> Walk -> Dash -> Run -> RunBrake -> Turn,
Fox Attack11, live hitbox search, Mario damage/recover, and GuardOn/Guard/
GuardOff through imported `ftanim.c`/`ftkey.c`, original status descriptors,
`ftmain.c`, and `gmcollision.c`.

Mode `163` is the scene-level `battle_playable` Boundary/Latest anchor. Its fast
configuration retains the scripted two-human stock chain. Canonical realtime/
live-input presents the source five-minute, items-off Mario human versus Fox
level-3 CPU match. `battle_playable_match_lifecycle` uses a one-minute test
limit and runs the same source timer, Time Up, taskman cleanup, and original
`VSBattle(22) -> VSResults(24)` transition. Imported
`mnvsresults.c`, `lbtransition.c`, `scsubsysfighter.c`, and `scsubsysdata.c`
now own Results by default. The gate reaches tick `120+`, loads all eight files,
creates two fighters and 12 SObjs, and installs source Win/Lose statuses.

The renderer imports BattleShip display/light/matrix source and keeps selected
source DL order, RSP vertex carry, materials, depth, lighting, and live camera.
Profile 2 remains the exact oracle; production Mode 8 owns the proven native
fighter path. Geometry stays `828 = 202+320+306` source triangles with exact
`648/44/126/10` submit classes. See `docs/PERF_LEDGER.md` for the detailed
optimization history and counters.

Mode 163 links original camera, death/rebirth, HUD, normal moves, weapons,
effects, level-3 CPU, parsed audio, and Pupupu BGM. Its lifecycle gate covers
stock/KO/rebirth, combat, one-minute source expiry, cleanup, and Results while
retaining the protected 128 KiB reserve. The canonical five-minute runtime and
live DS input remain the user-facing gameplay configuration.

The coarse target now intrinsically forces realtime presentation, live input,
HW triangles, profile 1, and natural Mode 8. A plain target build reproduces
ROM `DC2871F3...52E4E3AD` (12,036,096 bytes). Its exact eight-frame Mode-8 run
retains all 828 triangles, `70/686` runs, `60+320+306` owners, and `29/0/0`
fallbacks at about 15.4 FPS. Fresh exact-ROM captures
`2026-07-14_dc287-mode8-direct*.png` pass both-frame visibility, named-region,
texture-detail, and motion gates; a same-ROM no-build run passes original Fox
CPU, natural BGM/refills, and memory/ITCM checks.

The rejected `39CD1397...B508CEF` cleanup ROM came from an under-specified
manual build with live input disabled. The reported partial second PNGs were
complete on disk; a multi-image inspection view displayed unchanged regions
as black. The user's separate no-audio/no-stage report for the exact `DC287...`
ROM remains a manual-environment retest item, so do not claim hardware/manual
acceptance. Screenshot coverage exists only for Mode 0, Mode 8, and canonical
HW-tri; Modes 1-7 have no capture set, and screenshots cannot prove audio or
hardware.

Cut F's scene raster cache is rejected and removed: its first eight frames cut
draw to `467,968/468,992`, but the seed omitted wallpaper pixels and the real
source camera exceeded cache bounds after 232 frames, forcing generic fallback.
Do not count its captures as working output. Remaining presentation debt includes
Whispy face, HUD/weapon detail, platform crossing, and Mario facing/light.

The per-callback typed stage executor is also rejected and removed. Candidate
stage time was about `877,248` ticks versus `873,344` generic, with 799/828
fast triangles and one fallback. Its whole-draw delta came from the already
accepted fighter owner, so it cannot supply a hundreds-of-thousands stage cut.

The full-source scanline-affine wallpaper path is rejected and removed. On one
ROM, mode 1 versus the candidate measured draw `1,489,696 -> 1,250,304` and
wallpaper `274,144 -> 34,112`: savings of only `239,392/240,032`, below the
300K keep gate, with a worse upload P95. Exact ROM `DC287...` is restored.

Cut G is paused as a WIP lab, not a production or hardware-accepted path. Its
three cached 128x128 scene mips plus a 2x2 projective grid reduce steady draw to
about `448K-450K` ticks with `628` live fighter triangles and recognizable
Dream Land/Mario/Fox captures. Original interface-camera traversal adds only
about `7K-10K` ticks, but the HUD is still absent because the layered SObj
adapter does not decode the source interface's CI4 sprites. Dynamic stage/front
effects are also absent, and pacing remains about 29.8 FPS. Resume with general
CI4 SObj support, then immediately measure and keep/revert against fidelity and
one-VBlank cost; do not count the current Cut G PNGs as complete screenshots.
## Recommended Next Work
1. Preserve the exact `DC287...` target and ask for manual retest before changing
   ROM code for the external launch report. Keep title and VS-setup ROMs outside
   this fast path.
2. Require a distinct source-backed representation with
   a measured/modelled exclusive saving of at least 300K ticks and a credible
   live-camera/fidelity path. Get its first timing in one build or 60 minutes.
3. Kill it immediately below that bound. Do not revive mode 9, Cut D texture
   residency/overlap, Cut F raster capture, matrix-only, affine-wallpaper,
   full-source scanline/HBlank wallpaper, prepared-VM, record-executor, or
   local no-Z/raw variants.

Do not restore the rejected five-address load-time `MObjSub` probe; the accepted seam is the generic original attachment boundary and proves live output.
The corrected tile-origin equation is source parity, but its fixed-camera probe changed only `18/49152` pixels; do not cite it as the remaining ribbon fix.
Do not translate N64 `CLAMP` as clamp-only when a nonzero mask owns a smaller physical period; preserve logical tile clamp and masked repeat/mirror separately.
Do not revisit the high-bit fighter branch for current fragments (`0/0` active Mario/Fox descriptors), or queue Dream Land by generic head assumptions (`layer_mask=0`, `42/0` lists, `0/49152` changed pixels).

## Optimization Decision

Cut D is closed: 322 keys require 3,739,648 bytes, reuse distance is 216, and
ARM7 overlap needs a second prepass inside an impossible 77,376-tick budget.
Cut F is also closed for the camera/wallpaper failures above despite its large
short-window saving. Both temporary implementations are removed. The accepted
workspace performance baseline remains Mode 8. Automated launch, draw, motion,
CPU, audio, and memory gates pass on the exact artifact; only the separate
manual environment report remains open.

## Verification

For docs-only edits, run `.\scripts\check-docs.ps1`. Use the visible canonical
loop per meaningful edit and the additive shadow checkpoint for shared-runtime
work:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-battle-playable-renderer-forensic.ps1 -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

This checkpoint passes the exact coarse Mode-8 benchmark, paired visual gates,
realtime CPU/audio/memory verifier, all four static checks, and fresh DevFast
canonical build/capture/parity. The prior broad checkpoint passed the eight-
frame forensic oracle, P1Gate, and Boundary `161/162/163`; rerun those after
further shared-runtime work. Full Regression remains intentionally skipped.
After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
