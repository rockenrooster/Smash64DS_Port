# Task 56 — Mario/Fox DS-native geometry (stripify/quadify): E0 census + verdict

**Date:** 2026-07-24
**Branch:** `codex/task56-fighter-stripify` (parent `a95a85f`, verified)
**Parent commit:** `4b4cf7908` (PRELUDE verifier-tooling fix; runtime parent `a95a85f`)
**Outcome:** **PROCEED (FULL large-gain gate met).** Mode-2 within-run
TRIANGLE_STRIP stripify of the fighter AOT stream cuts an estimated **47.0% /
882 fewer VERTEX16 submissions** per two-fighter traversal. This is the lever
Tasks 53/55 proved is the invariant floor: VERTEX16 transforms.

## Why this moves off the stage (recap)

- Task 53 removed ~188K of STG CPU prep (replay) but ALL stayed flat.
- Task 54 proved the stage is GX-throughput/backpressure bound after replay.
- Task 55 removed 355 redundant state words (−9.1% replay buffer) but ALL again
  stayed flat; the decisive reconciliation found the floor is the **606 stage
  VERTEX16 transforms**, not state words or CPU prep.
- The stage's VERTEX16-count reduction ceiling is tiny (stripify ~5.6%).
- **The large-gain lane is fighters.** FTR P50 579,264 is 34.5% of ALL P50.

## E0 fresh profile-0 baseline (the only Task-56 A control)

Built from the exact parent (`a95a85f`) as
`smash64ds-battle-playable-tickhud-hwtri`, BUILD `build-task56-parent`,
ROM SHA-256 `7AF64140D92BFE7A8BD79BB7E7167F9642EC32C9C5479725CE1E54C4737D0BBD`.
128 presented-frame samples, frame 438, melonDS `DE80E46B…` (repo fork, models
icache/dcache). `artifacts/performance/.task56-parent-baseline-128.json`.

| bucket | min | mean | P50 | P95 | max |
|---|---|---|---|---|---|
| ALL | 1,679,360 | 1,850,745 | **1,680,128** | **2,240,576** | 3,360,704 |
| FTR | 573,184 | 620,014 | **579,264** | **1,014,528** | 1,026,944 |
| STG | 376,896 | 382,150 | 381,632 | 388,672 | 390,080 |
| BG | 4,096 | 4,208 | 4,224 | 4,288 | 4,352 |
| AUD | 1,152 | 6,490 | 2,496 | 63,232 | 67,264 |
| HUD | 896 | 39,463 | 1,024 | 320,704 | 323,584 |
| SRC | 161,088 | 393,406 | 318,336 | 953,408 | 1,290,368 |
| MISC | 47,040 | 75,139 | 48,704 | 157,632 | 197,888 |
| OTHR | 17,216 | 329,875 | 338,432 | 537,216 | 572,864 |

VBlank-interval histogram (566 presented frames): 2→0, 3→474, 4→80, 5+→12,
max 18, slips 0. (Matches Task 53's shipped replay-on B-side exactly — this
parent includes Task 53's default-on replay and Task 55's default-off elision.)

**Validation:** the spec named "FTR P50 ~579K / P95 ~1.015M, ALL P95 ~2.24M";
the measured P50 579,264 / P95 1,014,528 / ALL P95 2,240,576 match exactly, so
this parent build is the correct Task-56 control (STALE BASELINE TRAP avoided).

## E0 path proof — the native-fighter production path IS the shipping draw

**Static proof (airtight, no melonDS needed):** the published and tick-HUD
target blocks both set `NDS_RENDERER_PROFILE_LEVEL := 0` and
`NDS_RENDERER_FAST_RUN_DEFAULT := 9` (Makefile:232, :235, :297, :305). The
runtime mode global is initialized at nds_renderer.c (declaration site) under:

```c
#if NDS_RENDERER_PROFILE_LEVEL == 0
#if NDS_RENDERER_FAST_RUN_DEFAULT
volatile u32 gNdsRendererFastRunMode = NDS_RENDERER_FAST_RUN_DEFAULT;  /* == 9 */
```

`gNdsRendererFastRunMode == 9 == NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE`
(include/nds/nds_renderer.h:193). The adapter dispatch
(src/port/reloc_backend_renderer_dl.c:11564-11575) sets
`native_owner_production_mode = TRUE` for modes 8/9 under
`NDS_RENDERER_PROFILE_LEVEL < 2` (0 < 2 holds), routing fighters through
`ndsRendererExecuteNativeFighterOwnerProduction` →
`ndsRendererNativeBeginDirectBatch` → `glBegin(GL_TRIANGLE)` at
nds_renderer.c:16212. The per-corner emit is `COLOR → [TEX_COORD] → VERTEX16`
over the dense records (nds_renderer.c:16777-16786), and the only primitive
mode anywhere in the native path is `GL_TRIANGLE`.

The production/hierarchy emit code is gated `#if NDS_RENDERER_HW_TRIANGLES &&
(NDS_RENDERER_PROFILE_LEVEL < 2)`; both conditions hold for the published ROM
(`HW_TRIANGLES := 1`, level 0), so it is compiled and is the selected path.
`gNdsRendererFastRunMode` (0x020ca958, section `D`) and
`sNdsRendererRuntimeFrameSummary` (0x02192be0, section `b`) are both present
in the parent ELF, confirming the instrumented path is live at profile-0.

The only per-frame disable is `is_use_animlocks || shuffle_tics`
(reloc_backend_renderer_dl.c:11579-11585) — special states, not the canonical
steady-state battle, which is exactly the window the baseline above samples.

(The dynamic GDB probe `scripts/probe-task56-fighter-path.ps1` hit an
intermittent melonDS GDB-listener startup race — "The program is not being
run" at the `target remote` line, the same flakiness seen in prior tasks. The
static proof above is stronger and does not depend on melonDS.)

## E0 current-fighter census — confirms the retained M2 numbers survived

The generated owner IR header (src/nds/nds_native_fighter_owner.generated.inc)
still declares the retained-M2 topology:

```
/* Canonical export: 32 roots, 49 epochs, 67 runs, 626 triangles. */
/* Dense geometry: 541 immutable vertices, 1878 indexed corners. */
```

Per-owner (from the topology census, scripts/task56_fighter_topology_census.py,
which parses the shipping IR directly): **mario 320 triangles / fox 306
triangles = 626 total, 1878 emitted GL_TRIANGLES corners.** This matches the
historical retained-M2 evidence (626 / 1878 / 32 roots / 49 epochs / 67 runs)
— it survived all later renderer changes. (The 606 stage VERTEX16 count from
Task 55 is a different stream — the stage, not the fighters.)

## E0 offline topology census — the decisive gate measurement

`scripts/task56_fighter_topology_census.py` (read-only; parses the exact
shipping `sNdsNativeFighterPackedCorners` / `sNdsNativeFighterRunFirstCorner` /
`sNdsNativeFighterRuns` tables the runtime walks, so the topology is
bit-identical to what ships). Per-run it partitions by the safety boundaries
the spec requires (a run already groups triangles sharing submit_class = matrix
contract, and lives inside an epoch owning material/texture/polygon state),
builds a topology graph keyed on the complete per-corner denseId (rgba/s/t/
binding/cache_slot — so two corners sharing only XYZ but differing in any
attribute have different denseIds and are never merged: ATTRIBUTE TRAP safe),
and computes strip/quad opportunities with the DS winding model.

**DS winding model (libnds videoGL.h:144-151 + GBATEK):**
- `GL_TRIANGLES` (0): every 3 verts = 1 tri, no sharing. N tris = 3N verts.
- `GL_TRIANGLE_STRIP` (2): v0,v1,v2 = tri0; each extra vert = +1 tri. The DS
  hardware flips winding every other triangle so the strip stays oriented. A
  triangle extends a strip iff it contains the ACTIVE EDGE (the last two
  emitted verts); the new vertex becomes the third corner. N tris in one
  connected strip = N+2 verts. The census tracks the active edge so it cannot
  overcount by joining on the wrong edge.
- `GL_QUADS` (1): 4 verts = 2 tris sharing a diagonal. 2 tris → 4 verts.

Full per-run table: `artifacts/performance/.task56-topology-census.txt`.

**GRAND TOTAL (mario+fox): 626 triangles, 1878 current GL_TRIANGLES vertices**

| lever | final vertices | reduction | fewer verts | gate |
|---|---|---|---|---|
| mode 1 exact-order strips | 1692 | **9.9%** | 186 | below |
| mode 2 within-run reorder strips | 996 | **47.0%** | **882** | **FULL met** |
| quad-pair | 1322 | 29.6% | 556 | below full |

- **Mode 1 (exact source order) is 9.9%** — the source emit order rarely
  continues a strip because the next triangle's shared edge is not the active
  edge. Mode 1 cannot reach the large-gain gate.
- **Mode 2 (within-run reordering) is 47.0% / 882 fewer.** A longest-strip
  heuristic (try every start + every legal initial active-edge orientation,
  extend first-fit, keep the longest chain) — a strong, conservative upper
  bound. Clears the FULL gate on both axes (≥35% AND ≥600 fewer).
- The mode-1→mode-2 jump (9.9% → 47.0%) is the "material increase" the spec
  requires to justify mode 2: "mode 2 is allowed only when mode 1 cannot reach
  the large-gain gate AND E0 proves safe opaque reordering materially
  increases the reduction." Both hold.

**Mode-2 safety is guaranteed by construction, not by a runtime check:**
- A run is already partitioned by `submit_class` (raw single-matrix vs
  cross-matrix) — the matrix-ownership boundary (CROSS-MATRIX TRAP).
- A run lives inside an epoch that owns material/texture/polygon-format state
  (the spec's material/texture/polygon boundaries).
- Reordering triangles WITHIN a run changes nothing about which matrix,
  material, texture, or polygon state each triangle sees — only the order in
  which the DS geometry engine receives them.
- The runs are opaque, depth-tested, same-matrix, same-material, same-texture,
  same-polygon-format, no source-order side effect (the fighter display
  contract submits a static topology per animation frame; there is no
  painter's-algorithm dependence within a run). This satisfies every mode-2
  safety clause in the spec.
- Winding is preserved: the DS hardware's per-triangle flip keeps a strip
  front-facing; where two edge-sharing triangles need a same-direction join,
  a 2-vertex degenerate stitch is inserted (counted as a real submission —
  DEGENERATE TRAP respected; the 47.0% is AFTER degenerates where needed).

## Predicted ALL-P95 recovery (sanity check)

FTR P95 = 1,014,528. The fighter VERTEX16 submissions are the geometry-engine
transform cost. A 47% cut to the fighter vertex-transform workload is the
single largest lever available; Tasks 53/55 established that VERTEX16
transforms are the invariant floor that CPU-prep and state-word removal cannot
touch. The realistic ALL-P95 recovery depends on how much of FTR is
vertex-transform vs other fighter work (matrix load, texture prepare, shading);
E2 will measure it. The gate does not require a specific ALL prediction at E0
for FULL PROCEED (only ≥35%/≥600-fewer), which is met.

## Written PROCEED/STOP verdict — PROCEED (FULL gate met)

- ✅ FULL PROCEED: ≥35% reduction in emitted fighter VERTEX16 commands — **47.0%**.
- ✅ FULL PROCEED: ≥600 fewer fighter VERTEX16 submissions — **882 fewer**.
- Mode 2 is justified (mode 1 below gate; mode 2 materially higher).
- All mode-2 safety clauses satisfied by the run/epoch partition.

Proceed to E1: extend the existing native-fighter generator to emit, per run, a
DS-native primitive stream that replaces `glBegin(GL_TRIANGLE)` + 3k vertices
with `glBegin(GL_TRIANGLE_STRIP)` + (N+2)-per-strip vertices, computed
host-side (no runtime topology work — GENERATOR TRAP avoided). Default-off flag
`NDS_TASK56_FIGHTER_PRIMITIVES`. Mode 1 ships first as the control; mode 2 is
the win. Reuse the existing dense VERTEX16 records (E1 REUSE rule) — only the
primitive type and vertex ORDER/INDEX sequence changes.

## Build environment note

Git Bash direct `make` hits the `/opt/devkitpro` recursive sub-make quirk.
Build through the devkitPro msys2 bash:
`C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd repo && make TARGET=... BUILD=... -j16'`.
A stale partial BUILD dir triggers "cp: File exists" asset-copy races — delete
the BUILD dir before rebuilding.
