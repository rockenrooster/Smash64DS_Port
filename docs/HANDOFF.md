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

Mode `163` is the scene-level `battle_playable` Boundary/Latest anchor. It runs
Pupupu Mario/Fox stock battle with imported camera/Dead/Rebirth live by default,
then proves natural attack/damage, KO/rebirth, normal moves, Mario fireball,
Fox blaster, guard, reflector, grab/throw, Mario/Fox specials, audio asset
parsing, one-track Pupupu BGM playback, and a DS 3D hardware stage + fighter
frame.

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

Runtime slice 2 graduated the original manager/status/animation path. Default
builds import `ft/ftmanager.c`, the full original common/Mario/Fox status
descriptor tables, and live `ftanim.c`/`ftanimend.c`/`ftkey.c`. Modes `39/40`,
`53/54`, and `161/162` now rebuild movement, attack, live-hit,
damage/recover, and guard coverage on that natural runtime. Legacy standalone
gcDrawAll modes `57/58` and selected Fox Jab2 modes `159/160` were deleted
instead of resurrecting their motion-extract and synthetic marker seams.

`battle_playable` default: `NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1` now links
original `gm/gmcamera.c`, `ftcommondead.c`, `ftcommonrebirth.c`,
battle-critical `if/ifcommon.c` HUD paths, original `if/ifscreenflash.c`, the
normal moveset imports, the weapon manager, Mario fireball, Fox blaster, the
original effect manager, Fox reflector, Mario Super Jump Punch, Mario Tornado,
Fox Fire Fox, original audio asset parsing, and one-track Pupupu BGM playback.
The mode-163 proof reports `stock8->3`, `falls0->5`,
`moveset=0x7ff phase=15`, `tilt=23/17/17`, `smash=13`, `aerial=19`,
`landing=26`, `grab=3/1`, `throw=12/5/175`, `throwDmg=0->12`,
`hud=dmg16/digits0x1060a stock9->4`, `projectile=... dmg=13`,
`reflector=0xff proc=1 vx=49809->-49809`, `specials=0xfff phase=7`,
`audio=seq47 bank1=1/42/117@32000 bank2=1/1/322@44100 fgm=100/464/695
raw=4422960 resident=0 scratch=64416`,
`bgm=track0 play=1 stop=1 refills=32 read=1114112 rate=44099 loop=0 hwloop=0 resident=65536`, and
`hwsubmit=42`, `hwtri=192`, `hwftr=2/582`. FGM/voice playback, original
sequence-player import, and non-critical HUD/SObj/particle perimeter remain
follow-up.
It also gates the memory ledger: current arena headroom is `237948`, resident
reloc payloads are `681632` bytes (`stage=202816`, `fighter=175440`,
`if=208672`), stale menu/opening payload bytes are `0/0`, and the separate
64 KiB BGM stream buffer leaves `172412` bytes against the 128 KiB reserve.

Canonical realtime + live-input + HW-tri shows recognizable Dream Land with
separated but not yet accepted Mario and Fox bodies. The HUD-off capture is
`artifacts/visibility/2026-07-09_fighter-vertex-cache-hudoff-final.png`; the
pre-fix baseline is
`artifacts/visibility/2026-07-09_fighter-lit-material-hudoff-final.png`.
Source map-object kinds `0..3` decode exactly, and the original manager grounds
Mario/Fox on lines `3/2` at X `0/-1397`. Fighter `MObjSub` attachment now
normalizes O2R mixed-width lanes before original `gcAddMObjForDObj` copies the
record. The HW combiner also preserves the proven one-cycle
`PRIMITIVE * SHADE` formula, so Mario remains red/blue and Fox now uses his
source olive/brown palette. Persistent source vertex slots restore 44 previously
dropped cross-joint triangles and visibly connect more limb strips. Residual
fragments, incomplete textures, direction-light decoding, and raw DS
matrix/depth remain debt; full fighter submission measures about `3.0fps`.
The scripted fast mode-163 target is
`smash64ds-battle-playable-fast-hwtri.nds`; the user-facing realtime ROM is
`smash64ds-battle-playable-hwtri.nds`, so verifier builds no longer collide
with the shipped artifact.
Normal builds expose one controller; the canonical live-input build alone
exposes the connected-neutral second pad. Intermediate taskman arena fallbacks
keep fighter-runtime modes above the 128 KiB memory reserve.

## Process Change

Future gameplay slices are runtime-first subsystem groups aimed at scene-level
capability: import original TUs, wire narrow seams, prove with the continuous
natural-runtime verifier plus captures, then graduate live.

Legacy bounded modes are migrate-or-delete. When a slice obsoletes an old
marker stack, delete its mode/verifier and leave one `[coverage-reduced]`
`KNOWN_ISSUES` ledger line instead of reproducing old markers.

New harness modes are only for scene-level capabilities.

## Recommended Next Work

1. Finish fighter part/texture/light fidelity; handle source entry behavior separately.
2. Cache source-selected stage/fighter draw state and restore canonical 60fps.
3. Add wallpaper/SObj background composition for Dream Land.
4. Replace projected-submit with source-correct raw DS matrix/depth submission.
5. Build the FGM/voice backend slice on top of the parsed assets.

## Verification

For docs-only edits, run `.\scripts\check-docs.ps1`. For mechanical chunks:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For shared-TU changes, use `RegressionCore` during the session. Tyler runs the full Regression sweep overnight with `scripts/start-overnight-regression.ps1`.
Current mode `161` stalls at `NAT_ATTACK=1,0`; clean `b1a9d839a` reproduces it
before renderer draw. Repair it before Boundary/RegressionCore; full stays overnight.
Detach prebuilds expected to exceed 90 seconds and confirm by stamp:

```powershell
.\scripts\build-verify-profile.ps1 -Profile RegressionCore -Detach
.\scripts\build-verify-profile.ps1 -Profile RegressionCore -VerifyStamp
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
