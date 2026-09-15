# 01 — Impact wave: restore fighter occlusion

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** VFX correctness; shared depth-state risk.
**Existing owner:** Existing effects/native-renderer package; preserve the separate shield report.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> green Impact wave effect is not drawing/occluding properly. It should draw "around" a fighter, so the part of the effect that is behind the fighter gets occluded properly. right now it always draws over a fighter, even the part that should be behind the fighter

## Current source findings

The native owner computes clip coordinates, then discards clip Z in favor of NextProjectedDepth for every triangle. The original display callback selects AA_ZB_XLU_SURF. The exact asset command decode is user-supplied, not independently repeated here. Current projected source-depth and painter-transition helpers were inspected.

Source keys: [S05](research/SOURCE_LEDGER.md#s05), [S07](research/SOURCE_LEDGER.md#s07), [R76](research/SOURCE_LEDGER.md#r76), [R78](research/SOURCE_LEDGER.md#r78), [R105](research/SOURCE_LEDGER.md#r105).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Apply/review R01: cache SourceDepthToV16(z * PROJECTED_VERTEX, w) for the 18 unique vertices and emit that array per corner. Keep the current XY, UV, material, topology and source order. Independently review R02 so effect models inherit the source G_ZBUFFER state while their own clear/set masks still apply. Check the effective render-mode compare/write contract, not G_ZBUFFER alone. Full-alpha DS depth writes and near clipping are separate requirements; do not force every effect to alpha 30 as a shortcut.

### Executable candidate diffs

[R01_impact_wave_source_depth.patch](runtime/R01_impact_wave_source_depth.patch) — **DIRECT CANDIDATE / native runtime**.

[R02_effect_initial_z.patch](runtime/R02_effect_initial_z.patch) — **CONDITIONAL CANDIDATE / sibling audit required**.

Check preimages with `python Briefs/tools/check_candidates.py --repo . --candidate R01`. Applying a patch and passing a host test do not establish natural-path correctness.

## Disprove this candidate before stacking patches

If the observed ring never engages this native owner, follow its actual owner before attributing the live symptom. A passing ring/native A/B against the old equally broken control is not an N64-reference proof.

## Acceptance for this symptom

Capture the effect before, across and after fighter intersection from both facings and several camera distances. Independently identify rear ring pixels that must be hidden and front pixels that must remain visible. Check the initial full-opacity frame, the fade, simultaneous rings, stage intersections and disappearance. Log positive native draw engagement (use gNdsImpactWaveNativeDrawCount if still present), effective depth policy and zero native failures. The source/generic arm may share this bug: an A/B against it alone is not an oracle.

## Required regression scope

Fighters and stage real-depth geometry; shield, Fox reflector, respawn halo, KO burst, Arwing and affected effect siblings. Audit ndsRendererSubmitNativeRebirthHalo rather than automatically applying the same policy. Preserve P50/P95/cadence and retained 18-vertex reuse.

## Coordination

Coordinate shared depth edits with Samus charge occlusion and all material fixes; do not merge their acceptance claims.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
