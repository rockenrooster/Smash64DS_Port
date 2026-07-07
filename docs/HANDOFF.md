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
`ftmain.c`, and `gmcollision.c`.

Mode `163` is the scene-level `battle_playable` Boundary/Latest anchor. It runs
Pupupu Mario/Fox stock battle with imported camera/Dead/Rebirth live by default,
then proves natural attack/damage, KO/rebirth, normal moves, Mario fireball,
Fox blaster, guard, reflector, grab/throw, Mario/Fox specials, audio asset
parsing, one-track Pupupu BGM playback, and a DS 3D hardware stage + fighter
frame.

Latest renderer detail: DS 3D hardware submission defaults to all-DL modes
`33/34`, stage draw/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge/wall/stale/live-stale/motion-stale/cliff-status/cliff-tick/fall-map/fall-landing/ceiling/ceiling-status/cliff-catch/cliff-wait/cliff-attack/cliff-attack-action/cliff-common2/cliff-escape-action/common2/cliff-climb floor/action/common2/finish/cliff-wait damage/MP Passive modes `59-124`, and Boundary/Latest pair
`161/162`; pass `-SoftwarePreview` to those wrappers for comparison runs. The
Pupupu stage-inclusive gate proves matrix, material, texture, depth/fog/alpha,
primitive-Z, and texture-perspective HW submission with zero rejects. The strict
direct/menu Mario/Fox all-DL hardware defaults now pass on live
manager-created fighters: all 14/18 selected DObjs are clean and hardware
submits 284/298 fighter triangles. The all-DL proof carries the
source-equivalent segment `0xE` material register, RSP vertex/render state,
original fighter-part MObjs, and CI TLUT seeds from the current material
palette. All-DL now reports `bind119/upload8/ready119/reject0`. The
stage-inclusive `gcDrawAll`/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge/wall/stale/live-stale/motion-stale/cliff-status/cliff-tick/fall-map/fall-landing/ceiling/ceiling-status/cliff-catch/cliff-wait/cliff-attack/cliff-attack-action/cliff-common2/cliff-escape-action/common2/cliff-climb floor/action/common2/finish/cliff-wait damage/MP Passive defaults now use a stage-side original-manager smoke proof
(`mask=0x24f`) and submit the Pupupu stage plus both selected live fighters in
one frame: `hwsubmit=42`, `hwtri=192`, `hwftr=2/582`, and
`bind97/upload11/ready97/reject0`. The active boundary wrappers and mode `163`
keep fuller movement/live-hit/combat ownership. Latest captures include
`artifacts\boundary-combat-hwtri.png`, the stage MP hardware captures through MP Passive,
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
current `master`.

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
`bgm=track0 play=1 stop=1 refills=88 read=2949120 rate=44046 loop=1 resident=65536`, and
`hwsubmit=42`, `hwtri=192`, `hwftr=2/582`. FGM/voice playback, original
sequence-player import, and non-critical HUD/SObj/particle perimeter remain
follow-up.
It also gates the memory ledger: current arena headroom is `237836`, resident
reloc payloads are `770896` bytes (`stage=202816`, `fighter=264704`,
`if=208672`), stale menu/opening payload bytes are `0/0`, and the separate
64 KiB BGM stream buffer still leaves `172300` bytes above the reserve.

## Process Change

Future gameplay slices are runtime-first subsystem groups aimed at scene-level
capability: import original TUs, wire narrow seams, prove with the continuous
natural-runtime verifier plus captures, then graduate live.

Legacy bounded modes are migrate-or-delete. When a slice obsoletes an old
marker stack, delete its mode/verifier and leave one `[coverage-reduced]`
`KNOWN_ISSUES` ledger line instead of reproducing old markers.

New harness modes are only for scene-level capabilities such as `battle_playable`.

## Recommended Next Work

1. Build the FGM/voice backend slice on top of the parsed assets.
2. Continue non-critical interface/particle perimeter.
3. As subsystem slices obsolete old marker stacks, migrate-or-delete their
   modes/verifiers and record one-line `[coverage-reduced]` follow-ups.
4. Renderer follow-up: broaden source-scene coverage, then plan cutover.

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
