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
selection. Each selected event retains its source matrix/material owner and
per-draw geometry/prim/env/light state; pre-matrix `dls[0]` uses parent state.
The adapter now preserves the 32-slot input/transformed RSP vertex cache across
the selected part sequence, as BattleShip's common `gSYTaskmanDLHeads[0]`
stream does. All-DL HW output is the full `320/306` Mario/Fox triangle set with
zero rejects; canonical proof reports `gxram=729/2209`, geometry `0x222005`,
selected parts `14/18`, and zero oracle mismatches.

Runtime slice 2 imports the original manager, common/Mario/Fox status tables,
and live animation/key runtime. Modes `39/40`, `53/54`, and `161/162` rebuild
movement, attack, damage/recover, and guard naturally; obsolete gcDrawAll modes
`57/58` and selected Jab2 modes `159/160` were deleted.

The local `ftParamsUpdateFighterPartsTransform*` seams now follow
`ftparam.c:2161-2349`; mode `163` walks both fighters toward stage center and
uses the source-derived jostle plus reflector reach before firing.

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
`hwsubmit=42`, `hwtri=192`, `hwftr=2/626`. FGM/voice playback, original
sequence-player import, and non-critical HUD/SObj/particle perimeter remain
follow-up.
The memory ledger reports headroom `237948`, resident reloc `681632` bytes
(`stage=202816`, `fighter=175440`, `if=208672`), stale bytes `0/0`, and
`172412` bytes after the 64 KiB BGM buffer against the 128 KiB reserve.

Canonical realtime + live-input + HW-tri shows recognizable Dream Land with
separated but not yet accepted Mario/Fox bodies. Imported `grwallpaper.c`
owns the original 300x220 wallpaper behind the 3D stage and HUD. Latest capture:
`artifacts/visibility/2026-07-10_source-fighter-light-hudoff-final.png`.
Source map-object kinds `0..3` decode exactly, and the original manager grounds
Mario/Fox on lines `3/2` at X `0/-1397`. Fighter `MObjSub` attachment now
normalizes O2R mixed-width lanes before original `gcAddMObjForDObj` copies the
record. The HW combiner also preserves the proven one-cycle
`PRIMITIVE * SHADE` formula. Imported DObj/MObj/CObj AObj32 attachments now
normalize complete N64 MSB-first command graphs once per reloc generation;
fighter AObj16 streams bypass that path. Original timing/state stays live, and
a host-independent post-step corrects packed RGBA byte arithmetic. Persistent
source vertex slots restore 44 cross-joint triangles. Source-depth X/Y/Z share one
clip vertex; no-Z layers use signed 20.12 NDC. Dream Land texture varyings stay
linear; masked-clamp axes through 128 texels materialize source addressing inside
the logical tile. Its six CI4 decorations now render as recognizable stars; the
192-wide island remains bounded debt. Latest capture: `artifacts/visibility/2026-07-11_masked-clamp-linear-sampler-hudoff-stability-next.png`.
Source scene light and N64 RGBA order are live. Harsh contrast, fragments, shifts,
TEXEL1/water, DL-head order, raw GX matrices, and slow output remain debt.
The scripted target is `smash64ds-battle-playable-fast-hwtri.nds`; the shipped
realtime ROM is `smash64ds-battle-playable-hwtri.nds`, avoiding collisions.
Normal builds expose one controller; the canonical live-input build alone
exposes the connected-neutral second pad. Intermediate taskman arena fallbacks
keep fighter-runtime modes above the 128 KiB memory reserve.

## Recommended Next Work

1. Prove fighter anim-lock matrices and semantic render/fog/alpha state against fixed-pose crops.
2. Prove nonzero shift, TEXEL1/water, DXT-zero, and unmasked POT-padding texture semantics.
3. Make wallpaper commits atomic and separate audio producer/consumer timing.
4. Cache corrected draw/SObj state for stable audio and 60fps.
5. Add Dream Land shadows, CPU recovery, and FGM/voice playback.

Do not repeat the exact five-record stage `MObjSub` normalization as a visual fix: its canonical probe changed `0/49152` pixels and was fully reverted.
The corrected tile-origin equation is source parity, but its fixed-camera probe changed only `18/49152` pixels; do not cite it as the remaining ribbon fix.
Do not translate N64 `CLAMP` as clamp-only when a nonzero mask owns a smaller physical period; preserve logical tile clamp and masked repeat/mirror separately.
Do not revisit the high-bit fighter branch for current fragments (`0/0` active Mario/Fox descriptors), or queue Dream Land by generic head assumptions (`layer_mask=0`, `42/0` lists, `0/49152` changed pixels).

## Verification

For docs-only edits, run `.\scripts\check-docs.ps1`. For mechanical chunks:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For shared-TU changes, use `RegressionCore` during the session. Tyler runs the full Regression sweep overnight with `scripts/start-overnight-regression.ps1`.
Detach prebuilds expected to exceed 90 seconds and confirm by stamp:

```powershell
.\scripts\build-verify-profile.ps1 -Profile RegressionCore -Detach
.\scripts\build-verify-profile.ps1 -Profile RegressionCore -VerifyStamp
.\scripts\verify-all.ps1 -Profile RegressionCore -NoBuild -DelaySeconds 3
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
