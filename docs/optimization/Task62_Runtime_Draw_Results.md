# Task 62 — Runtime `DreamLand_DrawStatic3D` Path: Results

The first runtime task. Adds a flag-gated runtime path that draws the generated
c120 mesh directly from the baked blob, bypassing the segment0 program. Touches
`src/nds/nds_renderer.c` (one writer). No gameplay/collision change.

## Deliverables (3 commits)

- **Commit 1 (`3ebb30aae`)** — generated data blob: `scripts/generate_dreamland_ds_mesh.py` + `src/nds/dreamland_ds_mesh.generated.inc`. c120 baked as 71 groups (48 GL_QUAD + 23 GL_TRIANGLE_STRIP) + 261 rebased s10.3 vertices + VERTEX10 rebasis constants + FNV1a certificate. `--check` rebuilds deterministically.
- **Commit 2 (`202b4c442`)** — flag `NDS_DREAMLAND_DS_MESH ?= 0` wired through Makefile + header validation (mirrors NDS_TASK51_STAGE_NATIVE). Default-off.
- **Commit 3 (`a7d6f4be3`)** — `ndsRendererDreamLandDrawStatic3D` + dispatch hook + engagement counters, all `#if NDS_DREAMLAND_DS_MESH`.

## Runtime design

`ndsRendererDreamLandDrawStatic3D(projection, camera_modelview, stats)`:
1. Loads projection (GL_PROJECTION) + camera_modelview (GL_MODELVIEW) via `glMatrixMode`/`glLoadMatrix4x4` (column-major, mirrors the segment0 path).
2. Pushes the VERTEX10 rebasis as a compensating `MTX_MULT4x3` (scale on diagonal, origin in column 3) — reuses the Task 51 EnsureWorld pattern. The hardware reconstructs world space from the blob's rebased s10.3 vertices.
3. Emits each of 71 groups: `GFX_BEGIN = prim`, then per-vertex `GFX_COLOR` + the vertex via its encoded opcode (`GFX_VERTEX10` / `GFX_VERTEX_XZ` / `GFX_VERTEX_XY` / `GFX_VERTEX_YZ` / `GFX_VERTEX16` fallback) directly to the GBATEK registers.
4. `glPopMatrix(1)` — balances the modelview stack so subsequent owners are unaffected.

Dispatch: in `ndsRendererPrepareNativeStageOwner`, when the flag is on, call the draw function and skip the segment0 program (single gated `goto done`). Dynamic actors (Whispy/flowers) still draw via their own owners.

First integration emits **flat white (no textures)** as a geometry proof-of-concept — the owner's visual A/B (Commit 5 KEEP gate) confirms the silhouette renders before textures/materials are layered in.

## Build status (honest)

- **flag=0: BYTE-IDENTICAL** to the baseline without this change. Proven via stash comparison: ROM hash `707afa93...` with and without the renderer edit. The override-trap holds — at default (0), the entire Task 62 change (blob + flag + draw function + hook) compiles out and the published ROM is unchanged.
- **flag=1 clean compile: BLOCKED** by a **pre-existing nitrofs build-environment failure** (`Cannot open builds/build/nitrofs` at the `ndstool` step). Confirmed identical with **all Task 62 changes stashed** — this is not caused by the Task 62 code. Compilation/linking of the draw function must be re-verified once the build environment is healthy (the nitrofs directory population step isn't running in this environment).

## What is and isn't proven

| claim | status |
|-------|--------|
| flag=0 byte-identical to shipping | ✅ proven (stash A/B) |
| flag compiles out at default | ✅ proven (override-trap holds) |
| host blob/checker determinism | ✅ proven (all 6 host checkers pass) |
| draw function C is syntactically valid / links | ⚠️ not yet verified (nitrofs build-env failure blocks the compile) |
| visual correctness (silhouette renders) | ❌ not yet (needs flag=1 ROM + owner visual A/B) |
| perf improvement (ALL/P95) | ❌ not yet (needs the KEEP-gate A/B) |

## Next steps (owner-gated)

1. **Resolve the nitrofs build-environment failure** so a flag=1 ROM can be produced. This is environmental, not Task 62 code.
2. **Commit 4 (clean-compile gate)**: build flag=1, confirm the new symbols appear in the ELF and the ROM links.
3. **Commit 5 (KEEP gate)**: 128-frame synchronized A/B (flag 0 vs 1) on the canonical Boundary configuration. Report ALL/STG/FTR/OTHR P50/P95 + engagement counters. **Visual A/B mandatory** (owner is the oracle). KEEP only if visual quality acceptable AND frame-level ALL/P95 improves materially.

No claim of a KEEP this cycle — that's explicitly deferred to the owner-gated measurement once the build environment is healthy.
