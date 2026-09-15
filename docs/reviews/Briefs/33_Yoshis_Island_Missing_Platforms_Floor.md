# 33 — Yoshi's Island: restore main platforms and floor/path

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Missing structural presentation / gameplay readability.
**Existing owner:** P2-4 Yoster stage geometry.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Main platforms and main floor/path geometry are  completely missing, you can see the background right through it.

## Current source findings

Clouds are largely correct, while the main platforms AND main path/floor are reported missing. No current capture or decoded required-root list for those structural groups was available here. A documented source-approved haze omission is not permission to omit structural geometry.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [S12](research/SOURCE_LEDGER.md#s12), [R90](research/SOURCE_LEDGER.md#r90), [R91](research/SOURCE_LEDGER.md#r91).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Enumerate the structural roots from the readonly Yoster map and compare them with generated bindings/runs and native submission. Restore any accidentally omitted root or wrong segment mapping at the producer. If emitted triangles are rejected, fix the owning camera/matrix/cull/depth/material-state divergence while retaining source opaque coverage. Keep authored transform-only parents. Add independent expected structural-root/triangle coverage and source camera-view checks, not a test that simply trusts the producer’s own output. Do not restore unrelated approved haze or replace geometry with a wallpaper.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md](research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md).

## Disprove this candidate before stacking patches

Collision parity, total stage triangles or “no native failures” cannot show that these particular surfaces reached pixels. Separate absent data from present-but-fully-transparent or off-camera data before choosing a patch.

## Acceptance for this symptom

All main platforms AND the main floor/path are visible, correctly textured, positioned and layered, without background showing through intended opaque surfaces. Fighters visibly stand on the corresponding collision geometry; camera sweeps and stage re-entry retain them. Clouds and transparent decorations remain correct.

## Required regression scope

Collision/pass-through behavior, blast zones, source culling/depth, clouds/heart/card alpha, other stage packets and gameplay cadence.

## Coordination

Serialize with stage-wide material/alpha changes to avoid one brief re-hiding surfaces restored by the other.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
