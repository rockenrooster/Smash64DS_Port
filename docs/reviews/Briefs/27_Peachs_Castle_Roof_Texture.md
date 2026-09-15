# 27 — Peach's Castle: restore textures on recovered roof geometry

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Stage texture fidelity; preserve recovered geometry.
**Existing owner:** P2-4 Castle visual acceptance.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> peaches castle: Foreground castle roof renders ALL geometry now but the texture is missing on the now visible geometry. A continuous tiled roof surface is almost achieved.

## Current source findings

The owner explicitly says all foreground roof geometry is now visible. Reopening an earlier geometry-admission defect without checking the repaired packet would mis-target this report.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [R90](research/SOURCE_LEDGER.md#r90), [R91](research/SOURCE_LEDGER.md#r91).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Compare a newly visible untextured roof run with its adjoining correctly tiled run. Follow texture-enable, active tile/load tile, TLUT, source image, material segment, UV origin/scale and inherited state through the packet generator and runtime epoch cache. If a run has no explicit texture command but inherits one in the source, carry that state across the same source boundary in the native packet; if UVs or an external texture owner are omitted, repair producer ownership/relocation. Invalidate cached prepared texture state on every input that changes its meaning. Keep the now-complete roof topology and continuous repeat phase.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md](research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md).

## Disprove this candidate before stacking patches

Do not turn texture on for the entire stage or copy a neighboring roof material by visual guess. Inspect the exact source material on the affected triangles; some geometry may intentionally be untextured.

## Acceptance for this symptom

All already recovered roof geometry remains visible and textured with a continuous source-derived tiled surface; no flat untextured faces, material seams, wrong wrap or new backface overdraw. Capture close/far camera positions and the prior failing region, with packet/texture identity and native engagement.

## Required regression scope

Castle foreground/background ordering, sloped surfaces and collision, other stage packet users, material cache/residency and stable gameplay cadence.

## Coordination

Share proven UV/material producer fixes with Mushroom Kingdom only after checking their actual root causes.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
