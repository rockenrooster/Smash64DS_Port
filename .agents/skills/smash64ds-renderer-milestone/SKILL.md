---
name: smash64ds-renderer-milestone
description: Implement or review the current Smash64DS renderer milestone M2 fighter owner, M3 complete Dream Land stage owner, or M4 pre-GO texture residency. Use only after a measured work packet selects one milestone. Encodes the live complete-owner contracts and known failed architectures so Codex removes generic frame work instead of repeating micro-optimizations. Do not use for isolated triangle tweaks or broad renderer cleanup.
---

# Objective

Produce the first bounded device result for one complete renderer milestone,
with exact host contracts and a design that removes a named class of gameplay
work.

# 1. Select exactly one milestone

Read:

- the renderer row in `docs/P1_EXECUTION_BOARD.md`;
- `docs/optimization/NATIVE_RENDERER_PLAN.md`;
- the matching entries in `docs/OPTIMIZATION_ROADMAP.md` and
  `docs/PERF_LEDGER.md`;
- the current generated data/module and generic source owner;
- relevant BattleShip display/material/matrix source.

The live documents set numerical gates. The orientation references in this skill
summarize the July 15 snapshot but do not override newer measurements.

# 2. Common complete-owner rules

For M2 or M3:

1. Generate immutable topology and static metadata outside the frame loop.
2. Bind live matrices, materials, lights, textures, selected events, and dynamic
   callbacks directly.
3. Preflight every condition that could reject before the first GX mutation.
4. Once committed, execution must be non-failing by contract; never partially
   submit then replay generic output.
5. Preserve exact source order, opaque/translucent ordering, depth, matrix
   ownership, material animation, selected display lists, persistent vertex
   cache, and owner entry/exit state.
6. Use one coarse owner call or the few source-required interleaved segments, not
   a new per-root/per-list/per-run interpreter.
7. Keep one whole-owner exact fallback before mutation for unsupported live
   conditions.
8. Expose bounded laboratory control/candidate selection at an owner boundary;
   remove it from production after acceptance.
9. Prove cardinality and state with host fixtures before spending an emulator
   window.

Never cache a composed gameplay frame or replace BattleShip selection with a
handwritten visual approximation.

# 3. M2 — native Mario/Fox owner

Follow `references/m2-fighter-owner.md`.

The material difference from rejected work must be joint removal of generic
matrix/light/production work. A hierarchy-only, transport-only, or another
per-root executor is not M2.

Mandatory current design constraints include:

- exact source local-matrix/fixed-point order and active-animlock rejection;
- generated 25/27-joint schedules and owner-qualified live bindings;
- exact CPU lighting sidecar unless a newer independent oracle supersedes it;
- eligible synthesized texture matrices only with live MObj/profile-2 parity;
- no physical GX palette alias with the traversal stack;
- one owner transaction across the generated epochs/runs;
- no whole-owner CPU copy/DMA packet whose preparation costs remain in-frame;
- no 121-small-list design and no generic triangle/vertex helpers in the common
  production loop.

# 4. M3 — complete Dream Land stage owner

Follow `references/m3-stage-owner.md`.

Treat all eight source callbacks and 57 DObjs as one validated owner, while
retaining callback-sized segments where fighters/weapons interleave in global
order. Preserve frame-global projected/no-Z depth sequencing across those
interleavings. Every live material/texture epoch must be shadow-validated before
arming the owner.

Do not count a host packet with `production_linked=0` as runtime progress. The
first device falsifier must remove generic work and meet both the live absolute
and saving thresholds.

# 5. M4 — zero gameplay texture preparation

Follow `references/m4-texture-residency.md` and invoke
`$smash64ds-memory-vram-audit` for byte/bank/reserve proof.

Build a full logical-key manifest after original battle setup and before GO.
Convert/prepare/upload supported assets before the violation fence arms. During
play, allow lookup/bind only; record first failure for conversion, allocation,
replacement, refresh, decompression, file I/O, GL create/upload/delete, eviction,
or manifest fallback.

Do not claim M4 from static prewarm alone: animated water exceeds simple finite
VRAM residency in the current corpus and needs a distinct exact representation.

# 6. Fast implementation sequence

1. Prove generated cardinality and byte budgets on host.
2. Add the smallest additive production seam owned by the lane.
3. Link without changing the default canonical path.
4. Run the live eight-frame same-ROM first falsifier.
5. REVERT runtime activation immediately if the structural gate fails; retain
   exact host fixtures only when they remain useful.
6. Widen through `$smash64ds-perf-experiment` and
   `$smash64ds-verifier-router` only for a candidate that earns it.

# 7. Output

Return the completed performance evidence packet plus:

- exact generic operations removed from the hot path;
- generated owner/epoch/run/vertex/triangle/key counts;
- preflight and no-partial-submit proof;
- fallback reasons/counts;
- host versus device status (`production_linked`, active selector, default path);
- remaining single residual wall;
- KEEP / REWORK / REVERT.
