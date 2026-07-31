---
name: smash64ds-optimize-gx-3d
description: Analyze, optimize, or debug the Nintendo DS geometry engine and 3D renderer in Smash64DS_Port, including GXFIFO submission, matrices, vertices, primitives, polygons, clipping, culling, depth, texture and palette state, command capture/replay, stage and fighter native rendering, GX stalls, or missing/wrong geometry. Use for renderer performance, stage/fighter draw cost, missing underside or camera-angle geometry defects, command-stream work, or DS-native 3D specialization.
---

# Optimize GX 3D

## Mission

Build the fastest correct DS-native 3D path. Distinguish CPU command construction, FIFO/backpressure, geometry work, and visible raster cost before changing representation.

## Owning seams

Start with CodeGraph and inspect:

- `src/nds/nds_renderer.c`;
- `include/nds/nds_renderer.h`;
- `src/nds/nds_task49_gx_differ.c`;
- `src/nds/nds_platform.c` renderer status and presentation;
- generated stage/fighter data and their generators under `scripts/`;
- Makefile renderer/profile/task flags;
- current GX/stage/fighter evidence in `artifacts/performance`;
- BattleShip display-list behavior and comparable DS-native renderers in both local DS references.

Do not use archived task conclusions as current code truth without checking the live path.

## First classification

For each symptom, classify the dominant mechanism:

1. **CPU preparation** — traversal, material resolution, transforms, state decoding, stream construction.
2. **CPU submission** — MMIO writes, loop overhead, synchronization.
3. **GX backpressure** — FIFO stalls or geometry-engine limits.
4. **Geometry load** — matrix transforms, vertex count, clipping, primitive setup.
5. **Raster/visibility** — screen coverage, overdraw, depth, translucency.
6. **State correctness** — matrix stack, winding, culling, texture parameters, polygon attributes, latches.
7. **Memory** — stream, texture, palette, or prepared-owner residency.

Do not assume fewer stream words, triangles, or pixels means fewer ARM9 ticks. Measure the currency.

## Workflow

1. **Reproduce on a fixed camera/window.**
   - Use the canonical battle unless a visual bug needs a more precise harness.
   - Capture synchronized control/candidate images.
   - For intermittent geometry, use an in-place toggle when possible; separate captures can miss the defect.

2. **Trace source semantics.**
   - Identify the original display-list commands, matrix ownership, material state, winding, culling, and animation dependencies.
   - Confirm whether geometry is rigid, dynamic, skinned, billboarded, translucent, or screen-space.
   - Never patch a shared defect with arbitrary per-model offsets.

3. **Record the stream/cost structure.**
   - owner/segment/run counts;
   - FIFO words by command;
   - matrix loads/multiplies;
   - vertices and primitive groups;
   - texture/palette binds;
   - state changes and redundant writes;
   - clipping/cull outcomes where measurable;
   - time split around preparation, push loop, and tail.

4. **Select a DS-native transformation.**
   Prefer:
   - offline generated C or packed streams;
   - stage/fighter-specific code;
   - capture-once/replay for immutable work;
   - dense binding lists;
   - precomputed matrices or model-space streams;
   - primitive strips/quads when they reduce the measured bottleneck;
   - state sorting only where source-visible order permits;
   - event/generation invalidation instead of per-frame rebuilding.

5. **Preserve exact named quantities.**
   Use the GX differ or targeted comparison to name what is exact: command sequence, matrix values, vertex positions, polygon state, or visible result. Do not use a mixed aggregate hash as proof of all renderer behavior.

6. **Build a one-flag A/B.**
   - Same tree and configuration.
   - Confirm candidate engagement with a cheap counter.
   - Avoid profile instrumentation in the candidate unless both arms pay it.
   - Search on `WORK-H` P50; gate on P95; use `ALL` only for VBlank crossing.

## Correctness checklist

Verify:

- matrix mode and stack balance;
- view/model/projection multiplication order;
- fixed-point range and rounding;
- vertex component order;
- primitive begin/end boundaries;
- strip winding parity;
- backface culling and two-sided exceptions;
- polygon ID, alpha, depth/write mode;
- texture format, size, wrap, flip, and palette address;
- color/normal/texcoord persistence;
- material animation and visibility invalidation;
- clipping at screen edges;
- no persistent vertex/state cache leakage between parts or owners.

For missing underside or camera-angle geometry, prove whether the cause is absent source polygons, culling/winding, transform/matrix ownership, clipping, depth, or stale state before adding geometry.

## Fidelity gate

Rendering may approximate only within the project contract:

- synchronized A/B screenshots;
- changed-pixel count and mean delta;
- no structural artifact, missing geometry, wrong texture, or flicker;
- owner's visual approval.

Agents cannot self-approve.

## Known traps

- `ALL` can remain flat when real work is removed.
- A vertex reduction can fail to move ticks when another fixed cost dominates.
- DMA to GXFIFO is not automatically faster and may be device-only.
- A capture/replay path can be invalidated by a seemingly unrelated dynamic state.
- Removing runs can disable a replay path and increase cost.
- Redundant persistent-state writes are only valuable if submission is the bottleneck.
- Screen coverage may be nearly free relative to per-run/per-vertex scaffolding.

## Required result

Report:

- exact owner and call path;
- cost classification;
- control/candidate stream structure;
- correctness quantity held exact or visual budget consumed;
- `WORK-H` P50/P95 and owner bucket deltas;
- screenshot/differ evidence;
- engagement counters;
- device classification;
- KEEP, REVERT, STOP, or WIP.
