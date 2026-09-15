# 09 — VS CSS: missing Yoshi 3D preview

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Missing presentation / roster preview.
**Existing owner:** P2-1 / P2-3r7.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> fighter 3d previews not visible for:
>     -Yoshi

## Current source findings

The raw YoshiModel 0x152 shared-pin collision was already repaired. The current reset-order gaps remain a separate candidate; neither fact establishes the cause of the presently invisible Yoshi preview.

Source keys: [S10](research/SOURCE_LEDGER.md#s10), [R71](research/SOURCE_LEDGER.md#r71), [R102](research/SOURCE_LEDGER.md#r102).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Use R03 for valid lifetime hardening. Trace Yoshi compact Model registration, native owner image generation/detail, live drawable roots and selected-pose model parts. Correct a mismatched source kind/detail or stale table at that seam; ensure preparation runs inside the correct resettable block and publication occurs only after valid roots and materials exist. If only Selected fails, extend the source-derived preview program/pose closure rather than changing all fighters or importing battle-only assets into every CSS slot. Keep the old disjoint shared-pin guard.

### Executable candidate diffs

[R03_css_reset_image_lifetime.patch](runtime/R03_css_reset_image_lifetime.patch) — **DEFENSIVE CANDIDATE / not a complete CSS fix**.

Check preimages with `python Briefs/tools/check_candidates.py --repo . --candidate R03`. Applying a patch and passing a host test do not establish natural-path correctness.

Implementation contract and code-oriented integration details: [research/CSS_TRANSACTIONAL_PREVIEW.md](research/CSS_TRANSACTIONAL_PREVIEW.md).

## Disprove this candidate before stacking patches

Repeat release/reacquire and Results->CSS. A missing native owner, wrong selected root and a camera that projects the mesh off-screen require different changes; collect the first differing value.

## Acceptance for this symptom

Yoshi's animated idle and selected preview are visible, correctly framed and textured through hover, confirm, cancel and repeat entry. All relevant resource counters show positive engagement, bounded lifetime and no collisions; BGM continues and menu cadence meets the contract.

## Required regression scope

Other previews, selected-pose warm-up, 0x152 pin disjointness, Results -> CSS particle initialization and shared palette/VRAM lifetime.

## Coordination

Independent of Yoshi battle egg/root programs; do not conflate preview visibility with special-move closure.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
