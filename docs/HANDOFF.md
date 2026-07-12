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

Latest renderer detail: BattleShip `ftdisplaymain.c`, `ftdisplaylights.c`, and
`guMtxCatF` are imported. The live fighter path now uses the original display
preamble, light state, visibility flags, and `dl`/ordered-`dls[]` selection;
only those selected DLs cross the narrow DS hardware-submission seam. The
manual all-DObj collector remains only as the software fixture oracle.

The source camera matrix/projection is prepared before fighter visibility
selection. Each selected event retains its matrix/material owner plus per-draw
geometry/prim/env/light and cycle/render state; `dls[0]` uses parent state.
The adapter now preserves the 32-slot input/transformed RSP vertex cache across
the selected part sequence, as BattleShip's common `gSYTaskmanDLHeads[0]`
stream does. All-DL HW output is the full `320/306` Mario/Fox triangle set with
zero rejects; canonical reports `gxram=742/2251`, geometry `0x222005`, cycle/
render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Initial diffuse/ambient state comes from the first selected source `MObjSub`
(`0xffffff00/0x4c4c4c00`); per-part overrides carry and fallback use is zero.

Runtime slice 2 imports the original manager, common/Mario/Fox status tables,
and live animation/key runtime. Modes `39/40`, `53/54`, and `161/162` rebuild
movement, attack, damage/recover, and guard naturally; obsolete gcDrawAll modes
`57/58` and selected Jab2 modes `159/160` were deleted.

`battle_playable` default: `NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1` now links
original `gm/gmcamera.c`, `ftcommondead.c`, `ftcommonrebirth.c`,
battle-critical `if/ifcommon.c` HUD paths, original `if/ifscreenflash.c`, the
normal moveset imports, the weapon manager, Mario fireball, Fox blaster, the
original effect manager, Fox reflector, Mario Super Jump Punch, Mario Tornado,
Fox Fire Fox, `ftcomputer.c`, original audio parsing, and Pupupu BGM playback.
The lifecycle CPU gate records `36394` original process/target frames, all
A/B/Z inputs, `303` live-hitbox frames, `6366` guard frames, recovery selection,
and `124%` maximum Mario damage.
The mode-163 proof reports `stock8->6`, `falls0->2`,
`moveset=0x7ff phase=15`, `grab=18/1`, `throw=12/5/618`,
`throwDmg=13->25`, `hud=dmg25/digits0x2050a stock9->7`,
`projectile=spawn1/ok1/dmg7`, `reflector=0xff proc=1
vx=49809->-49809 owner=Fox`, `specials=0xfff phase=7`,
`audio=seq47 bank1=1/42/117@32000 bank2=1/1/322@44100 fgm=100/464/695
raw=4422960 resident=0 scratch=16`,
`bgm=track0 play=1 stop=1 rate=44099 resident=65536`, and
`hwsubmit=42`, `hwtri=202`, `hwftr=2/626`. FGM/voice, original sequence-player,
and non-critical HUD/SObj/particle work remain follow-up.
The memory ledger reports headroom `236100`, resident reloc `681632` bytes
(`stage=202816`, `fighter=175440`, `if=208672`), stale bytes `0/0`, and
`172412` bytes after the 64 KiB BGM buffer against the 128 KiB reserve.

Canonical realtime + live-input + HW-tri shows recognizable Dream Land;
source-separated Mario/Fox bodies are broadly accepted on DS.
Source map-object kinds `0..3` decode exactly, and the original manager grounds
Mario/Fox on lines `3/2` at X `0/-1397`. A source-shaped `gcAddMObjAll` wrapper
uses loaded-file/asset-generation provenance for local O2R lane restoration.
Canonical observes at least four water/Whispy swaps, no native/failure cases,
and first flags `0x0200 -> 0x006b`. The renderer independently resolves
tile-6/TMEM-0x40 TEXEL1 and tile-7/TMEM-0 TEXEL0, recognizes exact
`G_CC_TEMPLERP`, and precomposes a DS RGBA5551/A1 approximation. Its CI4 path
builds all 256 pair values per change; addressing and Bayer A1 stay exact. A 184-frame
gate proves positive scene-lifetime compatible-state refresh, zero eviction/
reject/oracle drift, and terminal `12/12` matches. Pond detail is
`46.053%/23px` versus white `27.997%/105px`.
Profile-0 canonical/shipped retains all `828` triangles; `648` ordinary source-Z
triangles use corrected GX. Exceptions are `44` cross-matrix, `126` no-Z, and `10` range; snapshot/
decal/prim/reject remain zero. Matrix-keyed batches are `121/707/121`, loads are
`53`, and projected divisions fall `7,074 -> 1,242`. Profile 2 retains device
`PosTest 32/0/e2/w0/c0/mw1/drop0`, oracle `2484/0/0`, and all depth contracts.
The 32 slots retain matrix IDs in per-traversal 64-entry tables. Profile 0 turns
`821` loads into `282` lazy transforms and `258` hits; create/reuse/overflow is
`67/7/0`. All 44 stale-slot triangles are genuinely mixed. Warm median/p95 is
`15,543,456/15,804,544`, another `1.9%` reduction, at `2.1fps`. Capture: `artifacts/visibility/2026-07-11_canonical_fast_200725-1819819-p34716.png`.
Imported DObj/MObj/CObj AObj32 attachments normalize complete N64 MSB-first
command graphs once per reloc generation; fighter AObj16 bypasses that path.
Original timing remains live, and a post-step corrects packed RGBA. Persistent
fighter vertex slots retain 44 cross-joint triangles. Source depth shares one
clip vertex. Projected no-Z depth now has background/foreground phases: layer 0
remains far before source-Z; layer 3 moves near afterward, restoring the fence
over layer-1 floor/path. Stage traversal carries its 32-slot RSP vertex cache
and applies `G_MWO_POINT_ST`; File3's five flower groups add ten source textured
triangles (`192 -> 202`). Masked-clamp axes through 128 texels, six CI4 stars,
and the DXT tail stride remain fixed. Debt: Whispy face acceptance, water phase/
shifts/other TEXEL1, fog/color animation, speed, and Mario light A/B.
The scripted target is `smash64ds-battle-playable-fast-hwtri.nds`; shipped realtime is `smash64ds-battle-playable-hwtri.nds`. Canonical live input alone
exposes a connected-neutral second pad; normal builds expose one controller.

## Recommended Next Work
1. Cut software 2D to exact final-resolution output; keep live source transforms.
2. Re-profile before packet replay; soak only at architecture milestones.

Do not restore the rejected five-address load-time `MObjSub` probe; the accepted seam is the generic original attachment boundary and proves live output.
The corrected tile-origin equation is source parity, but its fixed-camera probe changed only `18/49152` pixels; do not cite it as the remaining ribbon fix.
Do not translate N64 `CLAMP` as clamp-only when a nonzero mask owns a smaller physical period; preserve logical tile clamp and masked repeat/mirror separately.
Do not revisit the high-bit fighter branch for current fragments (`0/0` active Mario/Fox descriptors), or queue Dream Land by generic head assumptions (`layer_mask=0`, `42/0` lists, `0/49152` changed pixels).

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

Fresh P1Gate/Boundary pass in `213.9s/143.6s`. This is not the five-minute P1
soak; keep historical harnesses only for localization and skip Full Regression.

After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
