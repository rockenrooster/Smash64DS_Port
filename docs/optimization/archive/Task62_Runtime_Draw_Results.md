# Task 62 — Runtime `DreamLand_DrawStatic3D` Path: Results

The first runtime task. Adds a flag-gated runtime path that draws the generated
c120 mesh directly from the baked blob, bypassing the segment0 program. Touches
`src/nds/nds_renderer.c` (one writer). No gameplay/collision change.

## 2026-07-25 owner gate — REVERT

The owner rejected the first flag-on ROM: “i see just mario and fox on
invisible platforms, no stage, must be invisible or really small.” A corrected
runtime lifecycle draw and a working local capture then exposed the complete
failure rather than rescuing the candidate:

- `task62_v5.png`: all 119 generated triangles were counted, but the static
  mesh was absent. The generated candidate had hard-coded every triangle to
  raw-Z class 0 even though the source static mesh contains 66 raw-Z, 99
  projected no-Z, and 10 rejected triangles.
- `task62_v7.png`: restoring source depth classes made the geometry visible,
  but the result is opaque white screen-covering cards, not a recognizable
  Dream Land island or three-platform silhouette.
- `task62_raw_core_probe.png`: retaining only the source raw-Z portion again
  produces no visible stage.
- `task62_raw_core_noz_probe.png`: forcing that raw core to neutral depth
  reveals only two thin horizontal surface bands. It cannot provide the
  promised island silhouette by itself.

The root problem is upstream of the draw loop. Task 59's candidate IR writes
`s`, `t`, and `rgba` as zero and writes `run_index=-1`,
`texture_epoch=-1`, and `submit_class=0`. The Task 60 primitive stream
retains topology and binding only. Most Dream Land scenery in this mesh is
alpha-textured no-Z card geometry; drawing those cards flat white necessarily
fills their rectangular bounds. The Task 58 oracle also rasterizes every
projected triangle without clip/depth/material/alpha semantics, so its high IoU
does not qualify a runtime-visible stage.

**Verdict: REVERT.** Keep `NDS_DREAMLAND_DS_MESH=0`; do not enable or publish
this path. The measured CPU/GX reduction remains useful rejected-experiment
evidence, but it cannot override the owner visual gate. Any future attempt must
preserve material epochs, UVs, vertex color/alpha, source depth classes, and
runtime clipping through simplification and primitive compilation, then pass a
visual gate before performance promotion.

The flag-on experiment had overwritten the root published battle ROM. On
2026-07-25 it was rebuilt with `NDS_DREAMLAND_DS_MESH=0` as
`smash64ds-battle-playable-hwtri.nds`, SHA-256
`4d795b4e83b335598b20a3b5953fdb1821797cc5e0a825fa96a0643abba4a090`.
Boundary then passed against that publication; its synchronized capture restored
the full textured stage (`artifacts/visibility/latest.png`, 37.642% dominant
green and 58.181% non-white/non-green detail). `check-published-roms.ps1` now
rejects the known Task 62 payload signature so this failed candidate cannot
silently replace the published ROM again.

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
| draw function renders the silhouette correctly | ❌ rejected by owner and follow-up captures |
| perf improvement (ALL/P95) | ✅ measured, but rejected on fidelity |

## Retired path

Do not run another A/B on this representation. A future compiler must retain
the omitted material/depth semantics and earn a fresh owner visual gate before
performance measurement.
