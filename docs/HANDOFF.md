# Handoff

Use this file for the active handoff only. Keep it under 150 lines. The harness
registry decides the current Boundary/Latest profile; this file summarizes what
to do next.

## Start Here

1. Check the registry view:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

2. Current pair is expected to be:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

3. Read `docs/STATUS.md` for current truth, `docs/DIAGNOSTIC_REFERENCE.md` for
full marker strings, and `docs/PORTING.md` for history.

## Current Boundary

Modes `161/162` are the active legacy regression anchor. They prove selected
Fox Jab2 live-hit damage lifecycle plus selected damage-status follow-through
on the Pupupu Mario/Fox battle root. The current short marker summary is:

```text
status=17->52/45, hitlag=6->0, callbacks=1/6/1,
search=0xf, repeat=1/1, gate=0x3f, catchSearch=0xffffffff/s3
```

Latest verified source-backed detail: TaruCannon kind `3` now routes through
the source-ordered setup shell, installs status `61`, and runs one original
physics tick copying fighter root position from the barrel root. Continuous
TaruCannon update/shoot runtime still waits for Jungle barrel helpers and map
throw-hit data.

Latest renderer detail: `src/nds/nds_renderer.c` now handles real `G_MTX` /
`G_VTX` traversal state. It unpacks packed-N64 `Mtx` values to DS 20.12,
keeps separate modelview/projection state, composes the transform matrix,
restores modelview state for `G_POPMTX`, and decodes/transforms vertex payloads
during shared display-list execution. `G_TRI1` / `G_TRI2` traversal now counts
triangles whose vertices are transformed-ready for the next GX submission
slice. The fixture script gates F3DEX2 VTX/TRI/MTX/POPMTX packing, composed
vertex transforms, modelview stack restore, and transformed triangle readiness.

## Process Change

Do not continue one-bit proof-mask increments. Future gameplay slices should
import one whole BattleShip TU, or a coherent adjacent group of TUs, prove it at
the boundary, then graduate it to live runtime by removing the guarded seam.

New harness modes are exceptional. Fold new proofs into the current pair unless
the work reaches a scene-level boundary such as `battle_playable` or
`results_screen`.

## Recommended Next Work

1. Mechanical split: follow the `src/port/reloc_backend.c` split plan in
   `docs/ARCHITECTURE.md`. This is a pure move, no behavior change.
2. Renderer stage 1 continuation: route proven Mario/Fox and Pupupu samples
   through the shared matrix/vertex/triangle-ready state, then submit those
   transformed triangles through the DS 3D path.
3. Runtime slice 1: full `ft/ftmain.c` plus `gm/gmcollision.c`, gated by a
   continuous-runtime verifier on the current Mario/Fox scene.

Do the mechanical split before broad runtime edits unless the user redirects.

## Verification

For docs-only edits:

```powershell
.\scripts\check-docs.ps1
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
```

For mechanical split chunks:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For shared runtime/common fighter/renderer/harness registry changes, add the
smallest broader check that matches the touched area:

```powershell
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
.\scripts\verify-regression.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, inspect status, optionally commit, then run snapshot
as the last project command:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

## Avoid

- one-bit proof-mask increments;
- per-branch seed/restore proofs when the whole function can run naturally;
- permanent state restore around already-proven code;
- new harness modes for individual feature bits;
- broad gameplay rewrites outside original BattleShip source;
- broad compatibility headers to make imports easier;
- editing `decomp/`, generated outputs, emulator configs, logs, or runner slots.
