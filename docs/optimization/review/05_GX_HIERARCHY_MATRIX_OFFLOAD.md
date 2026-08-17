# Campaign 05 — GX Hierarchy / Matrix Offload

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Status — the core mechanism SHIPPED 2026-08-16

**`NDS_R2_FIGHTER_GX_COMPOSE=1` is now the published default** (owner policy
2026-08-16: accepted optimisations ship enabled). The shipped route is the
accepted c185 bank: fighter joint composition on the GX matrix stack
(`RESTORE(parent)` / `MTX_MULT_4x4(chain)` / `STORE(i)` in preorder, matrix
palette as parent store, requires `NDS_R2_FIGHTER_HW_MTX=1`). The Slice 43
one-frame-blink defect was a GX matrix-stack leak, since fixed full-match; the
owner accepted the matched-tic pixel masks (0.0358–0.1742% battle-screen
variance, GXSTAT `0x06000000`, gameplay invariants unchanged). Its historical
DRAW=0 cadence sibling read 90.731% two-VBlank — the cadence arm is still
open. Mode 2 (compose-and-compare per binding) remains the verification arm;
`gNdsR2GxComposeVerifyFail` must stay zero.

## Remaining objective

The flag flip is done; the campaign's remaining value is everything the flag
did **not** change:

1. **Joint-consumer classification (Phase 0)** — still unowned; needed by
   Campaign 11 and for AOT folding.
2. **AOT-fold static/identity transforms** so neither CPU nor GX composes
   what never changes.
3. **Feed fixed-native local matrices directly** (Phase 5): CPU still builds
   every local matrix each frame. The named live seam is
   `ndsRendererAdapterBuildDObjLocalMatrix` — 62 `bl __aeabi_*` sites in one
   per-joint-per-frame function whose **interior is already fixed point**; the
   soft float is its f32 boundary, concentrated in the MVP-recalc scale path
   gated on `has_mvp_recalc_rpy_0x47` (board). Coordinate with Campaign 13.
4. **CPU anchor minimization**: stop building CPU world matrices that only
   feed drawing now that GX composes; gameplay-authoritative joints keep their
   CPU transforms.

Do not redo triangle stripification; Task 56 primitive streams are already shipped.

Sizing note: the old prize figure — per-joint matrix build 61,848 tk/fr,
`MtxMulAffine20p12` 18,549 with 72.9% icache stall (`FTR_LANE.md`) — is
**GX=0-era and pre-ITCM-repack** (that kernel is now ITCM-resident and the
compose is on GX). Re-attribute on the new shipping census before sizing any
slice. `FTR` held 52.6% of the run's GX-FIFO stall at GX=0; with compose ON,
watch FIFO backpressure counters even more closely.

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

*Overtaken as written: the shipped c185 route already composes the whole joint
hierarchy on GX. Keep this phase's shape as the template for the remaining
slices (AOT folds, CPU-anchor removal, fixed local-matrix feed): one bounded
change, differ, measure.*

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
