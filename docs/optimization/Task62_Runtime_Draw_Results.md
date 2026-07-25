# Task 62 — Runtime `DreamLand_DrawStatic3D` Path: Results

The first runtime task. Adds a flag-gated runtime path that draws the generated
c120 mesh directly from the baked blob, bypassing the segment0 program. Touches
`src/nds/nds_renderer.c` (one writer). No gameplay/collision change.

## Deliverables

- **Commit 1 (`3ebb30aae`)** — generated data blob: `scripts/generate_dreamland_ds_mesh.py` + `src/nds/dreamland_ds_mesh.generated.inc`. c120 baked as 71 groups (48 GL_QUAD + 23 GL_TRIANGLE_STRIP) + 261 rebased s10.3 vertices + VERTEX10 rebasis constants + FNV1a certificate. `--check` rebuilds deterministically.
- **Commit 2 (`202b4c442`)** — flag `NDS_DREAMLAND_DS_MESH ?= 0` wired through Makefile + header validation (mirrors NDS_TASK51_STAGE_NATIVE). Default-off.
- **Commit 3 (`a7d6f4be3`)** — `ndsRendererDreamLandDrawStatic3D` + dispatch hook + engagement counters, all `#if NDS_DREAMLAND_DS_MESH`.
- **Commit 4 (`2c2daf0cd`, original)** — results doc (this file). Originally mis-attributed a flag=1 build failure to a "pre-existing nitrofs build-environment failure"; see correction below.
- **Commit 5 (counter relocation fix)** — the engagement counters were accidentally nested inside the `#if NDS_RENDERER_PROFILE_LEVEL == 1` block at `nds_renderer.c:2041`, so they compiled out in the published profile-0 hwtri target and the draw function's writes to them were undeclared. Moved them under the feature-flag block alongside the draw function. Override-trap preserved (proven byte-identical at flag=0 with and without the fix).

## Runtime design

`ndsRendererDreamLandDrawStatic3D(projection, camera_modelview, stats)`:
1. Loads projection (GL_PROJECTION) + camera_modelview (GL_MODELVIEW) via `glMatrixMode`/`glLoadMatrix4x4` (column-major, mirrors the segment0 path).
2. Pushes the VERTEX10 rebasis as a compensating `MTX_MULT4x3` (scale on diagonal, origin in column 3) — reuses the Task 51 EnsureWorld pattern. The hardware reconstructs world space from the blob's rebased s10.3 vertices.
3. Emits each of 71 groups: `GFX_BEGIN = prim`, then per-vertex `GFX_COLOR` + the vertex via its encoded opcode (`GFX_VERTEX10` / `GFX_VERTEX_XZ` / `GFX_VERTEX_YZ` / `GFX_VERTEX16` fallback) directly to the GBATEK registers.
4. `glPopMatrix(1)` — balances the modelview stack so subsequent owners are unaffected.

Dispatch: in `ndsRendererPrepareNativeStageOwner`, when the flag is on, call the draw function and skip the segment0 program (single gated `goto done`). Dynamic actors (Whispy/flowers) still draw via their own owners.

First integration emits **flat white (no textures)** as a geometry proof-of-concept — the owner's visual A/B (the KEEP gate) confirms the silhouette renders before textures/materials are layered in.

## Build status (verified)

Both gates now pass. The earlier "nitrofs build-environment failure" was a misdiagnosis — a clean build succeeds; the real blocker was (a) invoking `make` against the bare default target (which has `NDS_RENDERER_HW_TRIANGLES=0`, so the validator correctly rejected the flag) instead of the published `smash64ds-battle-playable-hwtri` target, and (b) the misplaced engagement counters (fixed in Commit 5).

- **flag=0 byte-identity: PROVEN.** `make TARGET=smash64ds-battle-playable-hwtri NDS_DREAMLAND_DS_MESH=0` ROM sha256 = `4d795b4e83b335598b20a3b5953fdb1821797cc5e0a825fa96a0643abba4a090`, identical with and without the counter fix (stash A/B). The override-trap holds: at default 0 the entire Task 62 change compiles out and the published ROM is unchanged.
- **flag=1 clean compile + link: PROVEN.** `make TARGET=smash64ds-battle-playable-hwtri NDS_DREAMLAND_DS_MESH=1` produces `smash64ds-battle-playable-hwtri.nds` (sha256 = `c84d279545139d36ec437fa1138e62007b26074747f96a0c0cac792c6e0a191a`), differing from flag=0 as expected (gated code links in). Config header confirms `NDS_DREAMLAND_DS_MESH 1`, `NDS_RENDERER_HW_TRIANGLES 1`, `NDS_RENDERER_PROFILE_LEVEL 0`.
- **ELF symbols present (flag=1):** verified via `arm-none-eabi-nm`:
  - Counters (BSS): `gNdsDreamLandDSGroups`, `gNdsDreamLandDSSubmittedVertices`, `gNdsDreamLandDSWords`.
  - Generated data (file-scoped statics): `sNdsDreamLandDSGroupFirstVertex`, `sNdsDreamLandDSGroupPrim`, `sNdsDreamLandDSGroupVertexCount`, `sNdsDreamLandDSVertexOpcode`, `sNdsDreamLandDSVertexX/Y/Z`.
  - The draw function itself is inlined into `ndsRendererPrepareNativeStageOwner` (its sole call site) under optimization, so it has no standalone symbol — expected.

## What is and isn't proven

| claim | status |
|-------|--------|
| flag=0 byte-identical to shipping | ✅ proven (stash A/B, hash match) |
| flag compiles out at default | ✅ proven (override-trap holds) |
| flag=1 clean compiles + links | ✅ proven (ROM + ELF built) |
| generated data + counters present in ELF | ✅ proven (nm) |
| host blob/checker determinism | ✅ proven (all 6 host checkers pass) |
| draw function renders the silhouette correctly | ❌ not yet (needs flag=1 ROM run + owner visual A/B) |
| perf improvement (ALL/P95) | ❌ not yet (needs the KEEP-gate A/B) |

## Next step (owner-gated KEEP)

128-frame synchronized A/B (flag 0 vs 1) on the canonical Boundary configuration (`battle_playable_realtime`, mode 163, Dream Land, 3600-tick Time mode). Report ALL/STG/FTR/OTHR P50/P95 + the engagement counters on the shared HUD row. **Visual A/B mandatory** (owner is the oracle): normal gameplay side-by-side, no holes, platform silhouettes correct, main island recognizable, pause-orbit swimming check. KEEP only if visual quality acceptable AND frame-level ALL/P95 improves materially (AGENTS rule: do not keep on STG reduction alone).

No claim of a KEEP this cycle — the clean-compile gate is met; the KEEP is explicitly deferred to the owner-gated visual + perf measurement.
