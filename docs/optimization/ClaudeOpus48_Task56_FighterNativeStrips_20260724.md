# Task 56 — Mario/Fox DS-native geometry: stripify + quadify the fighter AOT stream

**Status:** E0 PROCEED (FULL large-gain gate met). E1/E2 pending.
**Branch:** `codex/task56-fighter-stripify` (parent `a95a85f`)
**Standing rules apply** (docs/optimization/TASK_STANDING_RULES.md).

> The full task brief is the owner's paste (recorded verbatim in the session
> that chartered this task). This file records the implementation, evidence,
> and verdict. Filename follows the `ClaudeOpus48_` convention.

## Objective

Reduce the number of fighter VERTEX16 transforms the DS geometry engine
performs, by compiling the immutable Mario/Fox topology offline into DS-native
`GL_TRIANGLE_STRIP` / `GL_QUAD` primitive streams instead of the current
`GL_TRIANGLES` (3 vertices per triangle, no sharing). New flag
`NDS_TASK56_FIGHTER_PRIMITIVES ?= 0`:
- 0 = current native-fighter `GL_TRIANGLES` emission (control).
- 1 = exact-order strip/quad representation (source order preserved).
- 2 = within-run opaque-run DS-native topology reorder (the large-gain lever).

Default 0 until a measured KEEP. Gameplay/animation/collision/hitboxes/state
unchanged — only how the already-selected visual geometry is submitted to GX.

## E0 result (2026-07-24) — PROCEED

Full evidence: `artifacts/performance/2026-07-24_task56-fighter-stripify-e0.md`.
Census output: `artifacts/performance/.task56-topology-census.txt`.
Baseline JSON: `artifacts/performance/.task56-parent-baseline-128.json`.

**Path proof:** the shipping ROM sets `NDS_RENDERER_FAST_RUN_DEFAULT := 9` +
`PROFILE_LEVEL := 0`; `gNdsRendererFastRunMode` initializes to 9
(`NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE`); the adapter routes modes 8/9
to `ndsRendererExecuteNativeFighterOwnerProduction` (compiled since `0 < 2`),
whose only primitive mode is `glBegin(GL_TRIANGLE)` (nds_renderer.c:16212).
The native-fighter production path is the shipping fighter draw.

**Baseline (parent tick-HUD ROM `7AF64140…`, 128 samples, frame 438):**
FTR P50 579,264 / P95 1,014,528; ALL P50 1,680,128 / P95 2,240,576; STG P50
381,632 (Task 53 replay live). VBI 2:0 3:474 4:80 5+:12 max 18 slips 0.

**Topology census (parses shipping IR, 626 tris / 1878 corners):**

| lever | vertices | reduction | fewer | gate |
|---|---|---|---|---|
| mode 1 exact-order strips | 1692 | 9.9% | 186 | below |
| mode 2 within-run reorder | 996 | **47.0%** | **882** | **FULL** |
| quad-pair | 1322 | 29.6% | 556 | below full |

**Verdict: PROCEED (FULL gate met).** Mode 2 clears ≥35% AND ≥600-fewer. Mode
1 cannot reach the gate; mode 2's jump (9.9%→47.0%) is the "material increase"
justifying mode 2. Mode-2 safety is structural: a run is already partitioned by
submit_class (matrix) inside an epoch (material/texture/polygon), so within-run
reorder changes no state — only geometry-engine receive order. Winding is
preserved by the DS hardware's per-triangle flip + degenerate stitches where
needed (counted as real submissions).

## E1 plan (pending)

Extend the existing native-fighter generator (scripts/generate_nds_native_owners.py)
to emit, per run, a primitive descriptor (type + vertex-reference index
sequence) instead of the flat corner list. Runtime: one `GFX_BEGIN` per
primitive group + the generated vertex sequence. No runtime topology work
(strip finding/adjacency is host-side only). Reuse the existing dense
VERTEX16 records (gx_xy/gx_z unchanged); only primitive type + vertex
order/index changes. Build a semantic primitive differ (expand both A and B to
canonical triangles, compare triangle multiset + winding + matrix/material
state) — a raw GX word differ cannot certify stripify since the stream changes
by design.

## E2 plan (pending)

A = `NDS_TASK56_FIGHTER_PRIMITIVES=0`, B = `=1`, C = `=2` (if justified). Same
parent/tree/target/emulator/input/window, ≥128 samples. Report ALL/FTR/OTHR/
SRC/STG P50/P95 + VBlank histogram + per-traversal vertex/strip/quad/VERTEX16/
FIFO-word counts. The real gate is ALL (Tasks 53/55 proved FTR can fall while
OTHR rises and ALL stays flat). Visual A/B mandatory across MOVING animations
(idle/run/jump/attack/shield/hitstun/special/death/close+wide camera/pause
orbit) — the owner is the visual oracle. Task 9 state hash EXACT.

## Traps honored

TASK55 (test moving animations, not one static frame), OWNER-BUCKET (ALL
decides, not FTR), STRIP WINDING (alternates; active-edge tracked), DEGENERATE
(counted as real verts), ATTRIBUTE (denseId is the complete per-corner key),
CROSS-MATRIX (no strip across matrix ownership), TRANSLUCENCY (no reorder of
order-sensitive geometry), GENERATOR (all topology host-side), PROFILE (use the
profile-0 shipping path), OVERRIDE (prove the flag reached the ROM), VERIFIER
(127u PowerShell parse bug fixed in PRELUDE commit `4b4cf7908`), STALE BASELINE
(fresh parent baseline, matches spec's named numbers exactly).
