# Current Status

This is the short current-truth document. Use `docs/DIAGNOSTIC_REFERENCE.md`
for full marker strings; append history in `docs/PORTING.md`.
## Direction

Target remains a full 1:1 playable Nintendo DS source port of BattleShip Smash
64. Gameplay work now moves by runtime-first subsystem slices aimed at
scene-level capability: import coherent original TU groups, prove with the
continuous natural-runtime verifier plus captures, then graduate live.

Keep `decomp/` read-only. Do not hand-author gameplay when BattleShip source can
be ported.

## Current Boundary

The registry decides the active Boundary/Latest set:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Current Boundary/Latest entries:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-battle-playable-harness.ps1 -DelaySeconds 3
```

Modes `161/162` remain the bounded natural-combat pair. They keep the
Pupupu/Dream Land Mario/Fox battle root stable, create Mario/Fox through the
original manager, and drive Wait -> Walk -> Dash -> Run -> RunBrake -> Turn,
Fox Attack11, live hitbox search, Mario damage/recover, and GuardOn/Guard/
GuardOff through imported `ftanim.c`/`ftkey.c`, original status descriptors,
`ftmain.c`, and `gmcollision.c`.

Mode `163` is the scene-level `battle_playable` anchor. It runs Pupupu Mario/Fox
stock battle with imported camera/Dead/Rebirth live by default, then proves
natural attack/damage, KO/rebirth, normal moves, Mario fireball, Fox blaster,
guard, reflector, grab/throw, Mario/Fox specials, audio asset parsing, one-track
Pupupu BGM playback, and a DS 3D hardware stage + fighter frame.

## Latest Proof

Runtime slice 1 landed full BattleShip `gm/gmcollision.c`, replacing the local
matrix/world-position collision helpers. The shared `FTStruct` source region in
`include/ft/fighter.h` matches BattleShip `fttypes.h` through `display_mode`
(`joints=2280`, callbacks at `2516+`, source region `2896`), with DS/proof
fields moved to the tail extension and compile-time guards freezing layout.

Full BattleShip `ft/ftmain.c` is imported by default; duplicate local
`ftMain*` seams are gone or call the original once. Current layout and coverage
notes are in `docs/FTSTRUCT_PARITY.md` and `docs/KNOWN_ISSUES.md`.

Runtime slice 2 graduated the manager/status/animation path. Default builds now
import `ft/ftmanager.c`, the original common/Mario/Fox status descriptor
tables, and live `ftanim.c`/`ftanimend.c`/`ftkey.c`. Mario/Fox are created
through original manager descriptors and status-buffer payload loading. The
natural-combat proof now rebuilds movement, attack, live-hit, damage/recover,
and guard coverage on that runtime for modes `39/40`, `53/54`, and `161/162`.
The old cliffmotion restore hook is deleted after the direct and menu-chain
cliff-family Regression modes stayed green. The remaining stage compat-replay
seam in `ftMainSetStatus` is still documented as follow-up.

`battle_playable` graduated to default for `gm/gmcamera.c`,
`ftcommondead.c`, `ftcommonrebirth.c`, battle-critical `if/ifcommon.c` HUD,
original `if/ifscreenflash.c`, normal moveset TUs, the weapon manager, Mario
fireball, Fox blaster, the original effect manager, Fox reflector, Mario Super
Jump Punch, Mario Tornado, Fox Fire Fox, original audio asset parsing, and
one-track Pupupu BGM playback. Mode `163` proves stock/KO, natural combat,
normal moves, projectiles, reflector, grab/throw, remaining Mario/Fox specials,
HUD percent/stock, audio asset parsing, BGM playback, and HW stage/fighter
submission. FGM/voice playback, original sequence-player import, and
non-critical HUD/SObj/particle perimeter remain follow-up.

Mode `163` has fast verifier and canonical realtime + live-input + HW-triangle
paths. Canonical HW is pixel-gated and stable but not fidelity-complete:
the current capture still has white/misplaced Dream Land surfaces and broken
fighter assembly. Latest smoke after active-tile, implicit texture, and input
work: `frames=65 fps=34/34 ticks=635312768 gxram=368/1138`, `tri=438`,
`43849/49152` non-clear, `11619/49152` green, `13766/49152` detail,
`1136/5616` fighter-region pixels, and adjacent-frame delta `1181/49152`.
Raw DS matrix/depth still needs repair,
so canonical HW uses the CPU-oracle projected-submit fallback until a renderer
fidelity slice replaces it with source-correct raw or cached submission.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc
cache eviction. Mode `163` reports headroom `237948`, resident reloc `681632`
bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. The BGM stream adds a separate
64 KiB resident buffer; after subtracting it, `172412` bytes remain against
the 128 KiB reserve.

Renderer hardware is default for all-DL modes `33/34`, stage MP family modes
`59-124`, and Boundary/Latest pair `161/162`; global normal builds still use
software preview. The Pupupu gate submits the stage plus both fighters:
`hwsubmit=42`, `hwtri=192`, `hwftr=2/582`, `bind97/upload11/ready97/reject0`.
Canonical realtime adds texture-format/reject markers
(`conv0x100/bind0x100/pal0x100/rej0x0/why0x0`) and strict screenshots; raw
matrix/depth fidelity and cached 60fps cutover remain follow-up.

## Current Notes

Infrastructure checkpoint 2026-07-08: stable flags stay in
`nds_build_config.h`; per-mode harness ID/Inishie scale live in
`nds_scene_harness_config.h`. `RegressionCore -Force` measured `1893.130s`,
no-op `39.377s`, shared HW-tri switch `29.590s`, and full prebuild
`4773.933s`.

The config-header mode `161` regression is fixed without verifier expectation
changes: the broad `ftmanager.c` skip-entry guard is gone, and the VSBattle
wrapper preserves the source-correct setup from `decomp/.../scvsbattle.c:468`.

Canonical realtime + live-input + HW-tri renders through `gcDrawAll`, polls
live pads before each update, and has hard GX RAM/screenshot gates. The input
bridge now maps B/X/Y/L/R in addition to arrows/A/START and the HUD/markers
show held keys, live pad0, original controller state, and P0 root-x. Latest:
`combine=4655/2917/lit0/mat0/proj43684`,
`texFmt=conv0x100/bind0x100/pal0x100/rej0x0/why0x0`; it remains below 60fps
and visually incomplete.

The active `161/162` boundary is still bounded proof scaffolding, while
`battle_playable` is the first scene-level unbounded stock/KO anchor.
Legacy bounded modes are migrate-or-delete: obsolete mode/verifier stacks get
deleted with one `[coverage-reduced]` `KNOWN_ISSUES` line. Modes `57/58` and
`159/160` have already been deleted.

Follow-ups: raw DS matrix/depth and fighter assembly, wallpaper/SObj
composition, renderer-cache 60fps cutover, FGM/voice, and sequence player.

## Verification

Quick iteration uses `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.
Docs-only changes also run `check-docs`; registry/script changes run
`check-harness-registry`. Shared architecture changes graduate to:

```powershell
.\scripts\verify-all.ps1 -Profile RegressionCore -NoBuild -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For prebuilds over 90 seconds, detach, confirm with `-VerifyStamp`, and run the
daily full Regression sweep overnight.

After verified progress, commit if requested, then run the Lean snapshot last:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```
