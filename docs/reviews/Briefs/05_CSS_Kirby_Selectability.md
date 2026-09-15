# 05 — VS CSS: enable Kirby after proving the native closure

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Roster/gameplay access.
**Existing owner:** P2-3f47 + P2-1 CSS.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Kirby not selectable

## Current source findings

Kirby availability is not just a portrait flag. The board records native root/copy-hat work and outstanding natural proofs. CSS shared-pin and arena repairs already landed. A later source review still identifies reset lifetime seams.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [S10](research/SOURCE_LEDGER.md#s10), [R71](research/SOURCE_LEDGER.md#r71), [R83](research/SOURCE_LEDGER.md#r83), [R102](research/SOURCE_LEDGER.md#r102).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Use R03 only for the remaining reset-order gaps. Complete Kirby native owner-image residency, hidden-part/root programs and copy-hat ownership before changing the CSS admission mask. Publish a fighter kind only after its FPC1 data, both reachable detail images and preview pose are valid in the same generation. Treat the copy-hat runtime tables as owned by their per-battle-slot image and retire them with their backing range. Once current full-moveset/preview evidence qualifies Kirby, update the existing canonical roster admission owner and associated generated menu cells, not save unlock bits or an independent duplicate mask.

### Executable candidate diffs

[R03_css_reset_image_lifetime.patch](runtime/R03_css_reset_image_lifetime.patch) — **DEFENSIVE CANDIDATE / not a complete CSS fix**.

Check preimages with `python Briefs/tools/check_candidates.py --repo . --candidate R03`. Applying a patch and passing a host test do not establish natural-path correctness.

## Disprove this candidate before stacking patches

R03 is defensive and does not make Kirby selectable by itself. A valid CSS idle pose does not qualify Cutter, copied specials or required hat swaps.

## Acceptance for this symptom

Kirby can be hovered, selected, cancelled, reselected and used in a natural match when eligible. Preview and gameplay remain visible, source unlock behavior is respected, copied appearance restores correctly and repeated scene loops have bounded memory with zero native failures. A full roster bitmask alone is not proof.

## Required regression scope

Ness/Jigglypuff, other preview residents, Kirby copy hats, owner-image epoch invalidation and the existing CSS pin/loader tests.

## Coordination

Share infrastructure with the other two roster briefs; keep Kirby-specific acceptance separate.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
