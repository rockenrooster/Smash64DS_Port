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

The original fighter runtime is live: `gmcollision.c`, `ftmain.c`,
`ftmanager.c`, `ftanim.c`, `ftanimend.c`, `ftkey.c`, common/Mario/Fox status
tables, normal moves, weapons, effects, and Mario/Fox specials run through
imported BattleShip code. `FTStruct` and `FTData` retain source-layout guards;
mode `163` proves natural combat, KO/rebirth, HUD, audio parsing, one-track
Pupupu BGM, and the current hardware-rendered battle scene.

The fighter renderer now imports BattleShip `ftdisplaymain.c`,
`ftdisplaylights.c`, and `guMtxCatF`. The live DS path enters the original
display preamble, uses its lighting/geometry state, and follows its hidden,
no-texture, single-`dl`, and ordered `dls[]` part-selection contract. Only the
selected display lists cross a narrow DS hardware-submission seam; the old
manual all-DObj collector remains only as the CPU/software fixture oracle.

The source camera matrix/projection path is prepared before fighter visibility
selection. Source-selected events retain their matrix/material owner and
per-draw geometry/prim/env/light state; `dls[0]` remains in parent matrix state
as in `ftdisplaymain.c:789-805,883-899`. Canonical proof reports
`gxram=658/2010`, geometry mode `0x222005`, selected parts `14/18`, submitted
parts, and zero CPU-oracle mismatches. The current late capture is
`artifacts/visibility/2026-07-09_fighter-display-contract-hudoff-final.png`;
Mario and Fox
are improved over the manual collector, but close compatibility spawns,
lower-body fragments, and incomplete materials/textures still prevent final
visual acceptance.

Canonical screenshot gates remain strict: `44489/49152` non-clear,
`13454/49152` dominant-green, `30474/49152` detail, `3465/5616`
fighter-region color, and `928/49152` adjacent-frame delta. Raw DS
matrix/depth and cached submission remain renderer debt; the source-correct
full fighter body currently reduces canonical presentation to about `3.1fps`.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc
cache eviction. Mode `163` reports headroom `237948`, resident reloc `681632`
bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. The BGM stream adds a separate
64 KiB resident buffer; after subtracting it, `172412` bytes remain against
the 128 KiB reserve.

Renderer hardware is default for all-DL modes `33/34`, stage MP modes
`59-124`, and Boundary pair `161/162`; global normal builds keep software
preview. Texture-format/lane, stage-carry, GX RAM, oracle, fighter-contract,
and screenshot gates cover the canonical renderer.

## Current Notes

Infrastructure checkpoint 2026-07-08: stable flags stay in
`nds_build_config.h`; per-mode harness ID/Inishie scale live in
`nds_scene_harness_config.h`. `RegressionCore -Force` measured `1893.130s`,
no-op `39.377s`, shared HW-tri switch `29.590s`, and full prebuild
`4773.933s`.

The config-header mode `161` regression is fixed without verifier expectation
changes: the broad `ftmanager.c` skip-entry guard is gone, and the VSBattle
wrapper preserves the source-correct setup from `decomp/.../scvsbattle.c:468`.
Normal boot again exposes one controller; the neutral second pad is now limited
to `NDS_DEV_LIVE_INPUT_PREVIEW=1` canonical builds.
The taskman allocator now tries `0x140000` and `0x130000` before its legacy
1 MiB fallback, preventing source-display builds from overflowing after a
failed `0x150000` allocation while preserving the 128 KiB reserve.

Canonical realtime + live-input + HW-tri renders through `gcDrawAll`, polls
live pads before each update, and has hard GX RAM, oracle, display-contract,
and screenshot gates. The input bridge maps B/X/Y/L/R plus arrows/A/START.
The fast scripted mode-163 ROM uses a distinct `-fast-hwtri` filename so
Boundary/Regression builds cannot overwrite the shipped realtime ROM.

The active `161/162` boundary is still bounded proof scaffolding, while
`battle_playable` is the first scene-level unbounded stock/KO anchor.
Legacy bounded modes are migrate-or-delete: obsolete mode/verifier stacks get
deleted with one `[coverage-reduced]` `KNOWN_ISSUES` line. Modes `57/58` and
`159/160` have already been deleted.

Follow-ups: remaining fighter material/part fidelity, source-spawn entry/floor
runtime, raw DS matrix/depth, wallpaper/SObj composition, renderer-cache 60fps
cutover, FGM/voice, and the original sequence player.

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
