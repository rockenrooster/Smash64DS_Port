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
`33/34`, stage MP family modes `59-124`, and Boundary/Latest pair `161/162`;
pass `-SoftwarePreview` to those wrappers for comparison runs. The current
Pupupu gate submits the stage plus both selected live fighters in one frame:
`hwsubmit=42`, `hwtri=192`, `hwftr=2/582`, and
`bind97/upload11/ready97/reject0`. Global normal builds still use software
preview.

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
`bgm=track0 play=1 stop=1 refills=32 read=1114112 rate=44099 loop=0 hwloop=0 resident=65536`, and
`hwsubmit=42`, `hwtri=192`, `hwftr=2/582`. FGM/voice playback, original
sequence-player import, and non-critical HUD/SObj/particle perimeter remain
follow-up.
It also gates the memory ledger: current arena headroom is `237948`, resident
reloc payloads are `681632` bytes (`stage=202816`, `fighter=175440`,
`if=208672`), stale menu/opening payload bytes are `0/0`, and the separate
64 KiB BGM stream buffer leaves `172412` bytes against the 128 KiB reserve.

Canonical realtime + live-input + HW-tri is verifier-covered and no longer
blank/dead-input. Latest: `frames=67 fps=35/35 ticks=639162944 gxram=375/1163`,
`oracle=1080/0/0`, `texFmt=conv0x100/bind0x100/pal0x100/rej0x0/why0x0`, and
`combine=4723/2959/lit0/mat0/proj44330`. Canonical and shipped HUD-off captures
both show `44723/49152` non-clear, `10301/49152` green,
`10239/49152` non-white/non-green detail, and `968/5616` fighter-region
pixels. Raw DS matrix/depth still misses fighter/platform pixels, so canonical
HW uses the CPU-oracle projected-submit fallback; 60fps still needs cached
draw-state.

## Process Change

Future gameplay slices are runtime-first subsystem groups aimed at scene-level
capability: import original TUs, wire narrow seams, prove with the continuous
natural-runtime verifier plus captures, then graduate live.

Legacy bounded modes are migrate-or-delete. When a slice obsoletes an old
marker stack, delete its mode/verifier and leave one `[coverage-reduced]`
`KNOWN_ISSUES` ledger line instead of reproducing old markers.

New harness modes are only for scene-level capabilities.

## Recommended Next Work

1. Fix raw DS matrix/depth now that Dream Land/fighter pixels are pixel-proven.
2. Land renderer-cache submission for canonical realtime + live-input + HW-tri
   so textured stage/fighters move from the measured sub-60fps smoke to 60fps.
3. Build the FGM/voice backend slice on top of the parsed assets.
4. Continue non-critical interface/particle perimeter.
5. Migrate-or-delete obsolete modes and record `[coverage-reduced]` follow-ups.
6. Renderer follow-up: broaden source-scene coverage, then plan cutover.

## Verification

For docs-only edits, run `.\scripts\check-docs.ps1`. For mechanical split
chunks:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For shared-TU changes, use `RegressionCore` during the session and one full
fresh Regression prebuild plus sharded `-NoBuild` runs at the end. Detach
prebuilds expected to exceed 90 seconds and confirm by stamp:

```powershell
.\scripts\build-verify-profile.ps1 -Profile RegressionCore -Detach
.\scripts\build-verify-profile.ps1 -Profile RegressionCore -VerifyStamp
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4
.\scripts\build-verify-profile.ps1 -Profile Regression -Force
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex N -RunnerSlot N -NoBuild
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
