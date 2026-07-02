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

Renderer stage 3b keeps the hardware path opt-in behind
`NDS_RENDERER_HW_TRIANGLES=1` and adds source-shaped DObj matrix seed coverage
for BattleShip billboard kinds `33-40` and recalc kinds `41-50`. The DS path now
uses consistent `v16` world scaling for textured/untextured triangles and can
submit stage DObj display lists reached through the existing Pupupu `gcDrawAll`
traversal. The renderer now recognizes BattleShip `gSPMvpRecalc` /
`G_MW_MATRIX` display-list streams, seeds fighter-parts matrix kind `0x4B`,
seeds BattleShip battle-camera matrix kind `0x4C`,
converts cached fighter-parts `Mtx44f` seeds with fixed-W semantics matching
`syMatrixF2LFixedW`, and composes selected DObj parent chains from root to child
before the camera modelview. The stage-inclusive hardware capture
`artifacts\renderer-stage-gcdrawall-hw.png` shows the Pupupu platform plus
fighter geometry using the captured BattleShip camera GObj and its `0x4C`
matrix seed. The Mario/Fox all-DL hardware capture
`artifacts\renderer-chain-hw-battle.png` now restores the same battle camera
around the direct display calls, so it no longer depends on the bounded
hardware fallback camera. The hardware texture upload path now records
`LOADTLUT` palette state and converts CI4/CI8 textures through the same
RGBA5551 scratch upload used by RGBA16/I16, but these captures still submit raw
DObj geometry without material display-list emission, so they remain
flat-shaded.
Hardware all-DL stats are `HW=316/314 oracle=316/314 rejects=18/8 seeds=14/18`
and `MW=0/0/0/0`, proving the direct all-DL scene reaches DS hardware triangle
submission and per-DObj matrix seeds but not matrix-word streams. Full fighter
visual fidelity still needs material display-list submission before raw DObj
DLs, then combiner, depth/material state, broader texture proof, and cutover
work. Default builds still use the software preview.

Latest gameplay proof remains the TaruCannon status `61` setup/physics tick.

## Current Blocker

The active boundary is still bounded proof scaffolding, not continuous gameplay.
The next useful work is not another proof bit. It is one of:

- mechanical split of `src/port/reloc_backend.c` by the plan in
  `docs/ARCHITECTURE.md`;
- renderer follow-up: route material display lists into the opt-in hardware
  draw, then prove textured CI/TLUT output, combiner/material/depth state, and
  renderer cutover work;
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
