# Campaign 05 — GX Hierarchy / Matrix Offload

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Move **render-only** fighter hierarchy composition from ARM9 to the DS geometry/matrix pipeline.

Do not redo triangle stripification; Task 56 primitive streams are already shipped.

Do not simply flip an old `GX_COMPOSE` flag. Re-derive against the current fixed-camera, shipping `GX_COMPOSE=0` path.

**The sized prize** (`FTR_LANE.md`, shipping-config per-PC attribution):
per-joint matrix build is **61,848 tk/fr (21.3% of `FTR`)**, the
`BuildDObjLocalMatrix → BuildDObjXObjMatrix → TraRotRpyDirect` chain is 42,017
inclusive, and `ndsRendererMtxMulAffine20p12` alone is 18,549 with **72.9%
icache stall** (its 616 B body is re-fetched from main RAM essentially every
call). Offload wins twice: the multiply work moves to GX and the resident code
that thrashes the I-cache shrinks. Two prior failure modes to design against:
the per-joint GX hierarchy experiment that regressed **+33K on transport** (84
matrix restores — archived), and `FTR` already holding **52.6% of the whole
run's GX-FIFO stall**, so added GX commands must be watched for FIFO
backpressure.

## Core classification

For each Mario/Fox joint classify:

1. **Gameplay-authoritative** — read by collision, hitboxes, effects, attachments, physics, etc. Keep CPU-authoritative unless another campaign replaces those consumers.
2. **Render-only dynamic** — compose parent/local transform on GX.
3. **Static/identity/AOT-foldable** — eliminate runtime composition when legal.

## Current repo anchors

- `scripts/fighters/generate_nds_native_owners.py`
- `scripts/fighters/check_nds_native_owner_hierarchy.py`
- `scripts/census-fighter-gameplay-joints.ps1`
- `scripts/probe-attachment-position.ps1`
- `scripts/probe-fox-gun-matrix.ps1`
- `scripts/run-task49-gx-differ.ps1`
- `scripts/analyze-task49-gx-differ.ps1`
- `src/nds/nds_renderer.c`
- `src/port/reloc_backend_renderer_dl.c`
- shipped Task 56 primitive checker/data

## Phase 0 — Re-census joint consumers

For every joint:

- enumerate static and observed CPU readers;
- classify reader as simulation/collision/hitbox/effect/camera/debug/render;
- record whether it needs world transform, local transform, origin, or subset;
- record call/frame frequency.

Do not call a joint render-only based on one trace alone.

**Deliverable:** generated joint-ownership JSON.

## Phase 1 — Generate hierarchy/matrix-stack oracle

Extend native-owner generation with:

- parent index;
- traversal order;
- primitive groups per node;
- local transform source;
- push/pop boundaries;
- CPU anchor nodes;
- GX-compose eligible nodes.

Add a host simulation of push/mult/pop and compare node-world transforms with CPU oracle poses.

## Phase 2 — One render-only subtree

Pick a subtree with no gameplay transform readers and meaningful CPU matrix cost.

For it:

1. CPU supplies parent anchor/local fixed transforms.
2. GX loads/multiplies in generated hierarchy order.
3. Existing primitive groups render under current matrix.
4. CPU stops building descendant world matrices used only for drawing.

Do not change topology/material/binding in this first slice.

## Phase 3 — GX semantic differ

Use GX differ tooling to compare:

- matrix mode;
- load/multiply sequence;
- push/pop depth;
- primitive/state ordering;
- visible output.

Statically verify maximum matrix stack depth and restoration.

## Phase 4 — Expand by ownership class

Migrate additional class-2 subtrees by descending CPU cost.

For mixed trees, permit generated CPU anchor points for gameplay-read descendants while GX still composes render-only siblings.

## Phase 5 — Feed fixed-native matrices directly

Coordinate with Campaign 13.

Desired render-only chain:

`fixed animation/local pose -> native local matrix -> GX multiply`

not:

`fixed -> float -> CPU world matrix -> fixed -> GX`.

## Phase 6 — Integrate with Campaign 11

Campaign 11's generated renderer should ultimately own the hierarchy program. Campaign 05 should produce data/code describing:

- runtime CPU matrices required;
- GX local matrices;
- push/pop points;
- primitive groups under each node.

Avoid building a throwaway second hierarchy VM.

## Verification

- gameplay state hashes;
- attachment/effect positioning;
- targeted Mario/Fox attacks/recovery;
- owner hierarchy checker;
- GX differ;
- stack depth/restore assertions;
- fixed-camera pixel parity;
- FTR matrix/build/submit buckets;
- WORK-H P50/P95;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- GX stall counters.

## Keep / kill gates

Keep only if CPU matrix work disappears without equivalent GX stall/command cost and no gameplay reader loses its authoritative transform.

Kill if CPU still computes the same world matrices, stack behavior becomes fragile, attachments drift, or the change depends on redoing strips.

## Completion criteria

Every render-only Mario/Fox hierarchy segment is classified. Profitable segments are GX-composed from native local transforms, gameplay-authoritative transforms remain correct, and the result is directly usable by Campaign 11.
