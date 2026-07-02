# Current Status

This is the short current-truth document for active development. Keep it under
150 lines. Use `docs/DIAGNOSTIC_REFERENCE.md` for full marker strings and
`docs/PORTING.md` for append-only increment history.

## Direction

Target remains a full 1:1 playable Nintendo DS source port of BattleShip Smash
64. The process is changing because one-bit proof-mask increments are too
expensive for that goal. Future gameplay work should move by whole original
translation units, or coherent adjacent TU groups, then graduate proven code to
live runtime.

Keep `decomp/` read-only. Do not hand-author gameplay when BattleShip source can
be ported.

## Current Boundary

The registry decides the active Boundary/Latest set:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Current pair:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

Modes `161/162` are the legacy live-hit regression anchor. They keep the
Pupupu/Dream Land Mario/Fox battle root stable and prove a bounded selected Fox
Jab2 live-hit lifecycle through damage-status follow-through. They inherit the
current MP, cliff/ledge, passive/recover, wall/rebound, catch/throw, shield,
damage setup/recover, and TaruCannon hazard setup/physics proofs.

The current public summary is:

```text
status=17->52/45, hitlag=6->0, callbacks=1/6/1,
search=0xf, repeat=1/1, gate=0x3f, catchSearch=0xffffffff/s3
```

Full diagnostic marker strings live in `docs/DIAGNOSTIC_REFERENCE.md`, not here.

## Latest Proof

Runtime slice 1 landed the full BattleShip `gm/gmcollision.c` translation unit
through `src/import/battleship_gmcollision.c`. The old project-owned
`gmCollisionGetFighterPartsWorldPosition`, `func_ovl2_800EDA0C`, and
`gmCollisionGetWorldPosition` copies were removed, so matrix/world-position
collision helpers now resolve to the original source. `ft/ftmain.c` was tested
as a whole-TU import but did not land; it fails before duplicate cleanup on
full item/weapon/effect/audio/ground header compatibility, tracked in
`docs/KNOWN_ISSUES.md`.

Renderer hardware work remains opt-in behind `NDS_RENDERER_HW_TRIANGLES=1`.
Stage 3b added BattleShip billboard/recalc DObj matrix seed coverage, consistent
`v16` world scaling, stage DObj submission through the existing Pupupu
`gcDrawAll` traversal, BattleShip `gSPMvpRecalc` / `G_MW_MATRIX` stream support,
fighter-parts matrix kind `0x4B`, and battle-camera matrix kind `0x4C`. The
latest increment routes source-shaped `gcDrawMObjForDObj` material branch tables
into that same traversal by emitting segment `0x0E` branch lists and material
packets from the taskman graphics heap, including palette/TLUT, texture image,
load-block, tile-size, texture, color, and light-color state packets. Updated
captures:
`artifacts\renderer-chain-hw-battle.png` and
`artifacts\renderer-stage-gcdrawall-hw.png`. The stage-inclusive capture shows
the Pupupu platform plus hardware-submitted fighter geometry using the captured
BattleShip camera path. The all-DL verifier now has an opt-in hardware texture
gate; run `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`.
It proves `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, and the current capture is
`artifacts\battle-mariofox-dl-draw-all-hwtri.png`. The hardware texture cache
key now includes the source render/load tile, TMEM, palette, and tile-origin
state for that opt-in path. The hardware triangle path now applies the recorded
primitive/environment material color and alpha from the current combine state,
maps recorded F3DEX2 front/back cull geometry mode to DS polygon cull bits, and
uses the sm64-nds decal-combine, non-shade white tint, blend alpha-memory, and
texture-filter coordinate-bias rules for that gate.
Full visual fidelity still needs remaining combiner behavior, depth
policy, broader texture-format coverage, and renderer cutover work. Default
builds still use the software preview.

Latest gameplay proof remains the TaruCannon status `61` setup/physics tick.

## Current Blocker

The active boundary is still bounded proof scaffolding, not continuous gameplay.
The next useful work is not another proof bit. It is one of:

- mechanical split of `src/port/reloc_backend.c` by the plan in
  `docs/ARCHITECTURE.md`;
- renderer follow-up: finish opt-in hardware combiner/depth/material policy,
  broaden texture-format/state coverage beyond the first all-DL CI/TLUT upload, and
  then plan renderer cutover work;
- compatibility-header split needed before full-TU runtime import of
  `ft/ftmain.c`;
- continuous-runtime verifier for unbounded battle frames.

## Runtime Target

Next major gameplay milestone is `battle_playable`: continuous unbounded
VSBattle Mario vs Fox on Pupupu/Dream Land with live input, `gcRunAll` and
`gcDrawAll` every frame, no state restore around proven code, and the DS
hardware renderer path active enough for the scene.

## Verification

For normal 30-60 minute work:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
```

Docs-only changes also run:

```powershell
.\scripts\check-docs.ps1
```

Harness registry/script changes also run:

```powershell
.\scripts\check-harness-registry.ps1
```

Runtime/subsystem changes that touch shared architecture should graduate to:

```powershell
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

Run `verify-current` or `verify-regression` only for shared runtime behavior,
common fighter code, scene-manager flow, allocator/linker behavior, harness
registry behavior, or broad renderer changes.

After verified progress, inspect status, optionally commit, then run the Lean
snapshot as the final project command:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```
