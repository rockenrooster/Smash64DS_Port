# Current Status

This is the short current-truth document for active development. Keep it under 150 lines.
Use `docs/DIAGNOSTIC_REFERENCE.md` for full marker strings; append history in `docs/PORTING.md`.

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

Mode `163` now has two presentation paths: the default fast verifier keeps the
deep proof chain unthrottled, while realtime presentation runs one battle
update, one draw, and one DS vblank per frame. The realtime smoke reports
`frames=600 fps=598/598 ticks=335878400`; normal/manual builds use that
realtime path.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc
cache eviction. Mode `163` reports headroom `237948`, resident reloc `681632`
bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. The BGM stream adds a separate
64 KiB resident buffer; after subtracting it, `172412` bytes remain against
the 128 KiB reserve.

Renderer hardware is default for all-DL modes `33/34`, stage MP family modes
`59-124`, and Boundary/Latest pair `161/162`; global normal builds still use
software preview. The Pupupu stage-inclusive gate submits the stage plus both
selected fighters in one hardware frame: `hwsubmit=42`, `hwtri=192`,
`hwftr=2/582`, and `bind97/upload11/ready97/reject0`. Full visual fidelity
still needs broader source-scene coverage and cutover work.

## Current Notes

Infrastructure checkpoint 2026-07-08: stable flags stay in force-included
`nds_build_config.h`; per-mode harness ID and Inishie scale now live in
`nds_scene_harness_config.h`. `RegressionCore -Force` measured `1893.130s`,
no-op `39.377s`, and shared HW-tri mode switch `29.590s` for three scene-aware
objects plus relink. Full Regression prebuild now stamps in `4773.933s`.

The config-header mode `161` regression is fixed without verifier expectation
changes: the broad `ftmanager.c` skip-entry guard is gone, and the VSBattle
wrapper preserves the source-correct setup from `decomp/.../scvsbattle.c:468`.

Canonical realtime + live-input + HW-tri ROM investigation is open. The
realtime loop does draw once per vblank, but mode-163 proof preparation is
`NDS_HARNESS_FAST_LOGIC`-gated and the HW presentation still routes through the
stage-gcDrawAll proof helper, not a canonical full-scene renderer path.

The active `161/162` boundary is still bounded proof scaffolding, while
`battle_playable` is the first scene-level unbounded stock/KO anchor.
Legacy bounded modes are migrate-or-delete: obsolete mode/verifier stacks get
deleted with one `[coverage-reduced]` `KNOWN_ISSUES` line. Modes `57/58` and
`159/160` have already been deleted.

- follow-ups: renderer source-scene/HW cutover; FGM/voice, original sequence
  player, and non-critical HUD/SObj/particle perimeter.

## Verification

For normal 30-60 minute work, run
`.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

Docs-only changes also run `.\scripts\check-docs.ps1`; harness registry/script
changes also run `.\scripts\check-harness-registry.ps1`.

Runtime/subsystem changes that touch shared architecture should graduate to:

```powershell
.\scripts\verify-all.ps1 -Profile RegressionCore -NoBuild -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For prebuilds expected to exceed 90 seconds, run
`.\scripts\build-verify-profile.ps1 -Profile <Profile> -Detach` and confirm
completion with `.\scripts\build-verify-profile.ps1 -Profile <Profile>
-VerifyStamp`. Run one full fresh Regression prebuild plus four sharded
`-NoBuild` runs at the end of a shared-TU session.

After verified progress, inspect status, optionally commit, then run the Lean
snapshot as the final project command:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```
