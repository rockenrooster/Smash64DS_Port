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

Mode `163` has three verifier-covered configurations. The fast harness keeps
its scripted two-human stock chain; canonical realtime/live-input presents the
source five-minute, items-off Mario human versus Fox level-3 CPU match. The
fast lifecycle configuration runs that source match through Time Up and the
original VSBattle-to-VSResults scene transition. Together they cover natural
combat/KO/rebirth, normal moves, specials, audio, and a DS 3D hardware frame.

## Latest Proof

The original fighter runtime is live: `gmcollision.c`, `ftmain.c`,
`ftmanager.c`, `ftcomputer.c`, animation/key, common/Mario/Fox statuses, normal
moves, weapons, effects, and specials run through imported BattleShip code.
The lifecycle gate records `36394` source CPU process/target frames, all A/B/Z
inputs, `303` live-hitbox frames, `6366` guard frames, recovery selection, and
`124%` maximum Mario damage. Original `ifcommon.c:2472-2529,3144-3152,3342-3345`
consumes all `18000` timer ticks and requests `LoadScene`; imported
`scvsbattle.c:513-560` returns through taskman cleanup and changes scene
`VSBattle(22) -> VSResults(24)`. The actual VS Results scene remains stubbed.

The fighter renderer imports BattleShip `ftdisplaymain.c`, `ftdisplaylights.c`,
and `guMtxCatF`. Its display preamble, lighting state, visibility flags, and
single-`dl`/ordered-`dls[]` selection run live; only selected lists cross the DS
submission seam. The manual all-DObj collector remains a software fixture.

Selected events retain source matrix/material and geometry/prim/env/light
state; pre-matrix `dls[0]` keeps parent state as in
`ftdisplaymain.c:789-805,883-899`. The DS bridge also carries the RSP input and
transformed vertex cache across those per-part lists, matching BattleShip's
single `gSYTaskmanDLHeads[0]` stream. Exact Mario cross-joint fixtures now pass,
all-DL HW triangles rise from `284/298` to `320/306`, and rejects fall to zero.
Canonical proof reports `gxram=729/2209`, geometry `0x222005`, selected parts
`14/18`, and zero CPU-oracle mismatches.

O2R `MObjSub` mixed-width lanes, aligned Dream Land starts, original floor
adoption, and the observed one-cycle `PRIMITIVE * SHADE` material path remain
live. Capture
`artifacts/visibility/2026-07-09_fighter-vertex-cache-hudoff-final.png` shows
more connected limbs for red/blue Mario and olive/brown Fox. Residual fragments,
incomplete textures, and direction-light fidelity still block visual acceptance.

Canonical screenshot gates report `32687/49152` non-clear, `15627/49152`
dominant-green, `16920/49152` detail, `758/5616` fighter-region color, and
`595/49152` adjacent-frame delta. Raw DS matrix/depth and cached submission
remain debt; full fighter presentation currently runs at about `2.9fps`.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc
cache eviction. Mode `163` reports headroom `237948`, resident reloc `681632`
bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. Audio `.ctl` parsing now peaks at
`16` bytes of scratch. The separate 64 KiB BGM buffer leaves `172412` bytes
against the 128 KiB reserve.

Renderer hardware is default for all-DL modes `33/34`, stage MP modes
`59-124`, and Boundary pair `161/162`; global normal builds keep software
preview. Texture-format/lane, stage-carry, GX RAM, oracle, fighter-contract,
and screenshot gates cover the canonical renderer.

## Current Notes

Stable flags stay in `nds_build_config.h`; per-mode harness ID/Inishie scale
live in `nds_scene_harness_config.h`. The lifecycle shared-header rebuild took
`1406.40s`; its stamp validated in `0.38s` and RegressionCore passed in
`434.7s`.

The config-header mode `161` regression is fixed without verifier expectation
changes: the broad `ftmanager.c` skip-entry guard is gone, and the VSBattle
wrapper preserves the source-correct setup from `decomp/.../scvsbattle.c:468`.
Normal boot again exposes one controller; the neutral second pad is now limited
to `NDS_DEV_LIVE_INPUT_PREVIEW=1` canonical builds.
The taskman allocator now tries `0x140000` and `0x130000` before its legacy
1 MiB fallback, preventing source-display builds from overflowing after a
failed `0x150000` allocation while preserving the 128 KiB reserve.
Pupupu map-object kinds `0..3` now decode as `(0,6)`, `(-1397,906)`,
`(1,1545)`, and `(1421,909)` with no duplicates or unaligned reads. Mario and
Fox enter Wait grounded on lines `3/2` at X `0/-1397`.

Canonical realtime + live-input + HW-tri renders through `gcDrawAll`, polls
live pads before each update, and has hard GX RAM, oracle, display-contract,
and screenshot gates. The input bridge maps B/X/Y/L/R plus arrows/A/START.
The fast scripted mode-163 ROM uses a distinct `-fast-hwtri` filename so
Boundary/Regression builds cannot overwrite the shipped realtime ROM.

The active `161/162` boundary is still bounded proof scaffolding, while
`battle_playable` is the scene-level battle anchor.
Legacy bounded modes are migrate-or-delete: obsolete mode/verifier stacks get
deleted with one `[coverage-reduced]` `KNOWN_ISSUES` line. Modes `57/58` and
`159/160` have already been deleted.

Follow-ups: import the actual VS Results scene, natural CPU offstage recovery,
fighter fidelity, source entry behavior, raw DS matrix/depth, renderer-cache
60fps cutover, FGM/voice, and the original sequence player.
The former mode-161 Attack11 and mode-163 reflector blockers are closed;
direct/menu Boundary and RegressionCore pass without expectation changes.

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
