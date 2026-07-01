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

Renderer stage 1 now carries real `G_MTX` / `G_VTX` traversal state in
`src/nds/nds_renderer.c`: BattleShip/N64 packed `Mtx` data is unpacked to DS
20.12, modelview/projection state is composed, modelview `G_POPMTX` stack
restore is tracked, and vertex payloads are decoded/transformed during shared
display-list execution. The shared traversal also counts `G_TRI1` / `G_TRI2`
triangles whose vertices are transformed-ready for the next GX submission
slice. The fixture script gates F3DEX2 VTX/TRI/MTX/POPMTX packing, composed
vertex transforms, modelview stack restore, and transformed triangle readiness.

Latest gameplay proof remains the TaruCannon status `61` setup/physics tick.

## Current Blocker

The active boundary is still bounded proof scaffolding, not continuous gameplay.
The next useful work is not another proof bit. It is one of:

- mechanical split of `src/port/reloc_backend.c` by the plan in
  `docs/ARCHITECTURE.md`;
- DS hardware renderer stage 1 continuation: feed proven Mario/Fox and Pupupu
  display-list samples through the shared matrix/vertex state;
- renderer stage 2: submit transformed `G_TRI1` / `G_TRI2` output to the DS
  3D path;
- full-TU runtime import slice for `ft/ftmain.c` plus `gm/gmcollision.c`;
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
