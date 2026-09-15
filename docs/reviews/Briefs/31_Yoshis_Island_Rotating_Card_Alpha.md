# 31 — Yoshi's Island: transparent rotating texture cards

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Stage alpha / conspicuous missing coverage.
**Existing owner:** P2-4 Yoster visual acceptance.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Yoshi's Island: Rotating textures still expose opaque hard edge white texture-card backgrounds instead of using transparency.

## Current source findings

Rotating white cards have a coverage problem; white RGB alone does not imply transparency. Current IA conversion fixes must not be overwritten by old swizzle advice.

Source keys: [R61](research/SOURCE_LEDGER.md#r61), [R64](research/SOURCE_LEDGER.md#r64), [R90](research/SOURCE_LEDGER.md#r90), [R91](research/SOURCE_LEDGER.md#r91), [HW02](research/SOURCE_LEDGER.md#hw02).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Trace each rotating material frame’s original coverage channel and effective combiner. Preserve graded alpha or source cutout zeroes through palette/texel conversion and runtime blend state, and carry sampler wrap/clamp and frame selection into texture-cache identity. If source I textures use intensity as coverage, bake that alpha equation; if CI uses transparent palette entries, preserve those entries rather than deleting white. Clear atlas gutters or use correct clamping only where source sampling requires it. Keep the source rotation and visible white art.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md](research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md).

## Disprove this candidate before stacking patches

A global RGB-white color key destroys legitimate highlights/clouds. A blanket polygon-opacity reduction leaves the entire card visible and is not a coverage fix.

## Acceptance for this symptom

Rotating art shows transparent backgrounds rather than opaque white rectangles through the full cycle, without clipping legitimate white pixels or creating halos/atlas bleed. Preserve source animation/color and correct world layering. Cloud platforms remain unchanged.

## Required regression scope

Cloud appearance, heart sparkle material, other atlas cells, stage geometry visibility and stage re-entry/cadence.

## Coordination

Share alpha producer work with sparkles only if the source formats and actual first divergence match.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
