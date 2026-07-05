# Handoff

Use this file for the active handoff only. Keep it under 150 lines. The harness
registry decides the current Boundary/Latest profile; this file summarizes what
to do next.

## Start Here

1. Check the registry view:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

2. Current Boundary/Latest entries are expected to be:

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
`ftmain.c`, and `gmcollision.c`. The current short marker summary is:

```text
ftmanager natural-combat: wait=354/372, walk=8/8,
dash=7/6, run=8/9, attack=11, hitbox=4,
damage=0->4 status=40, guard=2/10/6, updates=427, mask=0xfffff,
hwsubmit=42, hwtri=192, hwftr=2/582
```

Mode `163` is the scene-level `battle_playable` Boundary/Latest anchor. It runs
Pupupu Mario/Fox stock battle with the imported battle camera, Dead, and
Rebirth live by default, then proves natural attack/damage, KO, stock
decrement, falls increment, RebirthDown -> RebirthStand -> RebirthWait, return
to Wait, and a DS 3D hardware stage + fighter frame.

Latest renderer detail: DS 3D hardware submission defaults to all-DL modes
`33/34`, stage draw/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge/wall/stale/live-stale/motion-stale/cliff-status/cliff-tick/fall-map/fall-landing/ceiling/ceiling-status/cliff-catch modes `59-100`, and Boundary/Latest pair
`161/162`; pass `-SoftwarePreview` to those wrappers for comparison runs. The
Pupupu stage-inclusive gate proves matrix, material, texture, depth/fog/alpha,
primitive-Z, and texture-perspective HW submission with zero rejects. The strict
direct/menu Mario/Fox all-DL hardware defaults now pass on live
manager-created fighters: all 14/18 selected DObjs are clean and hardware
submits 284/298 fighter triangles. The all-DL proof carries the
source-equivalent segment `0xE` material register, RSP vertex/render state,
original fighter-part MObjs, and CI TLUT seeds from the current material
palette. All-DL now reports `bind119/upload8/ready119/reject0`. The
stage-inclusive `gcDrawAll`/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge/wall/stale/live-stale/motion-stale/cliff-status/cliff-tick/fall-map/fall-landing/ceiling/ceiling-status/cliff-catch defaults now submit the Pupupu stage plus
both selected live fighters in one frame: `hwsubmit=252`, `hwtri=1152`,
`hwftr=2/582`, and `bind582/upload66/ready582/reject0`. The active boundary
wrappers assert that stage + both-fighter DS 3D replay after the imported
manager combat chain passes. Latest captures include
`artifacts\boundary-combat-hwtri.png`, the stage MP hardware captures,
menu-chain all-DL HW, and `artifacts\renderer-stage-gcdrawall-hw-fighters.png`.
Global normal builds still use the software preview.

Latest runtime detail: `gm/gmcollision.c` is imported as a whole BattleShip TU
via `src/import/battleship_gmcollision.c`, replacing the local
matrix/world-position helper copies. The shared `FTStruct` source region in
`include/ft/fighter.h` now matches BattleShip `fttypes.h` through
`display_mode`; `joints` is at `2280`, callback slots start at `2516`, the
source region is `2896` bytes, and DS/proof-only fields live after that
boundary. Static layout guards freeze the source offsets and extension boundary.
Full BattleShip `ft/ftmain.c` is now imported by default through
`src/import/battleship_ftmain.c`; the duplicate local `ftMain*` seams are gone
or routed through the imported original once. The default ladder, boundary,
continuous live-hit verifier, and four-way sharded Regression passed after a
fresh Regression prebuild, and all four Regression shards were rerun green on
current `master` after the renderer follow-ups. The regression-cycle fix
preserves the first selected cross-floor target match so later wall/cliff MP
updates cannot erase motion-stale proof evidence.

Runtime slice 2 graduated the original manager/status/animation path. Default
builds import `ft/ftmanager.c`, the full original common/Mario/Fox status
descriptor tables, and live `ftanim.c`/`ftanimend.c`/`ftkey.c`. Modes `39/40`,
`53/54`, and `161/162` now rebuild movement, attack, live-hit,
damage/recover, and guard coverage on that natural runtime. Legacy standalone
gcDrawAll modes `57/58` and selected Fox Jab2 modes `159/160` were deleted
instead of resurrecting their motion-extract and synthetic marker seams.

`battle_playable` default: `NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1` now links
original `gm/gmcamera.c`, `ftcommondead.c`, `ftcommonrebirth.c`, and
battle-critical `if/ifcommon.c` HUD paths plus original `if/ifscreenflash.c`.
The mode-163 proof reports
`stock2->1`, `falls0->1`, Dead/Rebirth/return-control frames,
`hud=dmg4/digits0x40a stock3->2`, and `hwsubmit=42`, `hwtri=192`,
`hwftr=2/582`. Timer, pause/end UI, magnify/arrows, tags, effects/items, and
broader SObj/RDP helper coverage remain follow-up.
It also gates the memory ledger: current arena headroom is `235396`, resident
reloc payloads are `618448` bytes (`stage=202816`, `fighter=206960`,
`if=208672`), and stale menu/opening payload bytes are `0/0`.

## Process Change

Future gameplay slices are runtime-first subsystem groups aimed at scene-level
capability: import original TUs, wire narrow seams, prove with the continuous
natural-runtime verifier plus captures, then graduate live.

Legacy bounded modes are migrate-or-delete. When a slice obsoletes an old
marker stack, delete its mode/verifier and leave one `[coverage-reduced]`
`KNOWN_ISSUES` ledger line instead of reproducing old markers.

New harness modes are only for scene-level capabilities such as `battle_playable`.

## Recommended Next Work

1. Finish the non-critical interface perimeter around timer, pause/end UI,
   magnify/arrows, tags, effects/items, and broader SObj/RDP helpers.
2. As subsystem slices obsolete old marker stacks, migrate-or-delete their
   modes/verifiers and record one-line `[coverage-reduced]` follow-ups.
3. Renderer follow-up: broaden source-scene coverage, then plan cutover.

## Verification

For docs-only edits, run `.\scripts\check-docs.ps1`. For mechanical split
chunks:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For shared runtime/common fighter/renderer/harness registry changes, add the
smallest broader check that matches the touched area. For shared fighter
runtime, use sharded regression, not the serial 45-minute run:

```powershell
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4
.\scripts\build-verify-profile.ps1 -Profile Regression -Force
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex N -RunnerSlot N -NoBuild
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, inspect status, optionally commit, then run
`.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the last project command.
