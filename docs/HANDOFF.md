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
blank/dead-input, but the visual frame is still not demo-fidelity. Latest:
`frames=62 fps=32/32 ticks=639899904 gxram=372/1152`, `tri=609`,
`texFmt=conv0x100/bind0x100/pal0x100/rej0x0/why0x0`,
`texLane=layout0x2/byte290/half290`, and
`stageCarry=2646/2646/tex2016/tile2142/short378/378/seg189`. The late capture
shows `42335/49152` non-clear, `22557/49152` green,
`19640/49152` detail, and adjacent-frame delta `0/49152`; Dream Land is
recognizable with visible fighters, but fighter assembly and remaining draw
classes are still visually wrong. The input bridge maps B/X/Y/L/R in addition
to arrows/A/START; the debug HUD/`LIVE_PAD` marker shows held keys -> live
pad0 -> original P0 controller/root-x. Raw DS matrix/depth remain renderer
debt; canonical HW still uses the projected fallback and 60fps still needs
cached draw-state.

## Process Change

Future gameplay slices are runtime-first subsystem groups aimed at scene-level
capability: import original TUs, wire narrow seams, prove with the continuous
natural-runtime verifier plus captures, then graduate live.

Legacy bounded modes are migrate-or-delete. When a slice obsoletes an old
marker stack, delete its mode/verifier and leave one `[coverage-reduced]`
`KNOWN_ISSUES` ledger line instead of reproducing old markers.

New harness modes are only for scene-level capabilities.

## Recommended Next Work

1. Fix renderer fidelity: fighter assembly, per-tile/TMEM sampling, and raw DS matrix/depth.
2. Add wallpaper/SObj background composition for Dream Land.
3. Land renderer-cache submission for canonical realtime + live-input + HW-tri
   so textured stage/fighters move from the measured sub-60fps smoke to 60fps.
4. Build the FGM/voice backend slice on top of the parsed assets.
5. Continue non-critical interface/particle perimeter.
6. Migrate-or-delete obsolete modes and record `[coverage-reduced]` follow-ups.

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
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
