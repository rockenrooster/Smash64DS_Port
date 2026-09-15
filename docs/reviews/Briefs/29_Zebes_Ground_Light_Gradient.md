# 29 — Zebes ground lights: taper to transparency

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Stage material / lighting presentation.
**Existing owner:** P2-4 Zebes visual acceptance.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Stage lights on the ground floor have a flat/hard transparency (hard upsidown trapezoid shape) instead of looking like a real light source with a gradient that tapers to fully transparent towards the top.

## Current source findings

The hard trapezoid can be a lost source vertex-alpha gradient, lost texture alpha or both. The existing stage generator collapses varying vertex alpha to one value per triangle; that mechanism is directly visible, but the exact light root was not independently decoded here.

Source keys: [R88](research/SOURCE_LEDGER.md#r88), [R91](research/SOURCE_LEDGER.md#r91), [HW01](research/SOURCE_LEDGER.md#hw01), [HW02](research/SOURCE_LEDGER.md#hw02).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Identify the light roots and alpha equation first. For untextured SHADE-alpha lights, test E01 isobands or a source-derived alpha-ramp texture mapped to the source beam extent; for textured lights, retain the texture coverage and combine it with the varying shade factor rather than replacing one with the other. Keep the bottom bright and the top source-transparent using the original material colors and geometry. Emit native translucent polygons with the correct compare/write policy and no opaque rectangular backing.

### Code delivery boundary

[E01 alpha-isoband implementation](experiments/alpha_isobands.py) is executable offline experiment code, with analytic tests. It is **not integrated into the stage producer or linked into a ROM**.

Implementation contract and code-oriented integration details: [experiments/README.md](experiments/README.md).

## Disprove this candidate before stacking patches

Do not extend the acid root allowlist to guessed light root offsets. If light alpha already resides in texels, additional geometric subdivision is not the first repair.

## Acceptance for this symptom

Ground lights retain the intended source shape/brightness near the base, taper smoothly upward and disappear into the background without a visible card outline or diagonal alpha seam. Show captured/profile evidence at several camera positions and with a fighter passing in front/behind.

## Required regression scope

Acid's already-correct color/depth, other stage lights, shared material/texture atlas, geometry budgets and cadence.

## Coordination

May reuse a proven gradient representation from other effects; maintain its own source-derived fixtures.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
