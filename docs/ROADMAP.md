# Roadmap

This roadmap tracks milestone status and active focus. Keep detailed proof in
`docs/DIAGNOSTIC_REFERENCE.md`, append-only history in `docs/PORTING.md`, and
current handoff state in `docs/STATUS.md` / `docs/HANDOFF.md`.

Status labels:

- Done: verified in current ROM/runtime.
- In Progress: started, but not enough for the next real gameplay/menu result.
- Deferred: known work, not started.

## Milestones

| # | Milestone | Status | Next Gate |
|---|---|---:|---|
| 1 | Clean devkitPro/libnds NDS project skeleton | Done | Keep `make -j16` green. |
| 2 | Add BattleShip as read-only source | Done | Keep imports through `src/import`. |
| 3 | Identify original startup/game-loop path | Done | Preserve `syMainLoop -> syMainThread5 -> scManagerRunLoop`. |
| 4 | DS platform/backend layer | In Progress | Keep DS behavior in `src/nds` or `src/port`. |
| 5 | Stub required libultra/platform seams | In Progress | Replace stubs only when a source boundary needs them. |
| 6 | Boot an `.nds` ROM | Done | Keep melonDS/no$gba smoke paths working. |
| 7 | Run original game-state/update loop | In Progress | Move from bounded proofs to whole-TU runtime slices. |
| 8 | Render original startup/title/menu assets | In Progress | Preserve proven previews while building DS hardware renderer. |
| 9 | DS hardware renderer for N64 display lists | In Progress | Matrix/vertex GX pipeline fixtures, then triangle submission. |
| 10 | Load one stage and two fighters | In Progress | Mario/Fox on Pupupu already proven in bounded form. |
| 11 | `battle_playable` continuous runtime | Deferred | Unbounded VSBattle Mario vs Fox, live input, no state restore. |
| 12 | Breadth: fighters, stages, menus, items, audio | Deferred | Repeat whole-subsystem slice recipe after `battle_playable`. |
| 13 | 1:1 full game port | Deferred | Requires renderer/audio/save/overlay/memory maturity. |

## Active Focus

The target is the full 1:1 port. The process has changed: one-bit proof-mask
increments are banned for future gameplay progress because they do not scale to
the full BattleShip codebase.

Current active proof boundary remains modes `161/162`:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

Use it as a regression anchor while migrating to larger source-backed slices.

## Execution Queue

1. Process/docs reset: keep AGENTS, STATUS, HANDOFF, and this roadmap aligned
   with whole-TU slices, proof graduation, harness restraint, and docs diet.
2. Mechanical split of `src/port/reloc_backend.c` by the plan in
   `docs/ARCHITECTURE.md`: relocation/assets, fighter model/struct proofs,
   movement proofs, MP collision proofs, cliff/ledge proofs, renderer/DL
   helpers, and diagnostic recorders.
3. DS hardware renderer stage 1: N64 `Mtx`/`Vtx` plus `gSPMatrix` /
   `gSPVertex` semantics to DS GX fixed-point matrix/vertex pipeline. The
   packed-matrix unpack and point-transform helper is in place; next wire it
   into real display-list command state.
4. Runtime slice 1: full `ft/ftmain.c` plus `gm/gmcollision.c`, replacing
   bounded hit-search/stat hubs with original code.
5. DS hardware renderer stage 2: `gSP1Triangle` / `gSP2Triangles` to GX vertex
   FIFO with Pupupu and Mario/Fox screenshot gate.
6. Animation/status runtime: `ft/ftmanager.c`, status tables,
   `ft/ftanim.c`, `ft/ftanimend.c`, and `ft/ftkey.c`.
7. Remaining common fighter runtime: unimported `ft/ftcommon/*`,
   `ft/ftphysics.c`, `ft/ftparam.c`, and `ft/ftpublic.c`.
8. Battle flow/HUD/camera: `gm/gmcamera.c`, `gm/gmcommon.c`, `if/ifcommon.c`,
   KO/respawn/rules logic.
9. Memory plan gate before breadth: DS main RAM is 4 MiB; document overlay and
   per-scene asset-streaming budget before adding many fighters/stages.

## Renderer Track

Build the F3DEX GBI to DS 3D hardware translator in `src/nds`, using
`include/nds/nds_gbi_decode.h`, decode fixtures, and proven decoded display
lists as fixtures. Keep the software path as a reference comparator until the
proven scenes pass on the hardware path.

Renderer stages:

1. Matrix/vertex pipeline fixtures for Mario/Fox display lists.
2. Triangle submission on DS 3D hardware, screenshot-gated in melonDS.
3. Texture conversion and VRAM cache for N64 TMEM formats.
4. Render state mapping for combiner/blender approximations.
5. Per-scene cutover behind a flag, with software reference retained.

Document fidelity limits in `docs/ARCHITECTURE.md`: DS has no full N64
per-pixel combiner, and hardware budgets are finite.

## Runtime Track

Each gameplay slice should:

1. inspect the BattleShip TU(s);
2. import the original file(s) through `src/import/battleship_*.c`;
3. add only narrow compatibility seams in project-owned code;
4. prove the whole path at a current boundary;
5. remove the guarded proof seam when the imported subsystem can run live.

The next major runtime milestone is `battle_playable`: continuous VSBattle
Mario vs Fox on Pupupu/Dream Land, live input, `gcRunAll` and `gcDrawAll` every
frame, no proof-local state restore around proven code.

## Verification Direction

Keep the tiered cadence:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
.\scripts\verify-regression.ps1
```

Add a continuous-runtime verifier for unbounded battle frames: scripted input,
valid statuses, finite positions, no crash/assert, and frame-time budget.

Renderer slices also require:

```powershell
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\capture-melonds.ps1
.\scripts\sample-runtime-speed.ps1
.\scripts\verify-opening-movie-speed.ps1
```

Run only the checks that match the touched area, then snapshot last.

## Avoid

- single-bit proof-mask increments;
- per-branch seed/restore proofs when a whole function or TU can run;
- proof code that reruns one already-imported branch with synthetic state;
- permanent state restore around proven runtime;
- new harness modes below scene-level boundaries;
- full title/menu/gameplay rewrites outside BattleShip source;
- broad compatibility headers or broad imports that hide missing seams;
- renderer breadth without fixture/screenshot gates;
- deleting historical harnesses only to reduce apparent bloat.
