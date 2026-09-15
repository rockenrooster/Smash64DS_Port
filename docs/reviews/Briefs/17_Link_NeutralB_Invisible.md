# 17 — Link neutral-B: throw/catch body visibility

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Fighter visibility during a core move.
**Existing owner:** P2-3f33.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> neutral B makes link invisible when throwing and catching the boomerang

## Current source findings

A newly identified conditional defect is present in nds_renderer_assets.c: Link SpecialN foreign boomerang table/preamble routing exists inside the Kirby-image branch but is absent from its #else helpers. The host harness reproduces the old wrong-table return; the patched helpers pass high/low/bounds tests. The reported ROM configuration remains unknown.

Source keys: [R71](research/SOURCE_LEDGER.md#r71), [R83](research/SOURCE_LEDGER.md#r83), [R84](research/SOURCE_LEDGER.md#r84).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Apply R04 only when its preimages/configuration match. It copies the existing Link source-owner routing into the disabled-Kirby helpers, including the matching light-preamble source. It does not alter the already-implemented SpecialN program. Independently inspect empty-hand/catch-frame root coverage, source-owner indices and canonical restoration; if the failing ROM already uses the Kirby-enabled arm, R04 does not explain that live failure. Keep Catch and Kirby CopyLink proofs scoped and intact.

### Executable candidate diffs

[R04_link_foreign_tables_without_kirby.patch](runtime/R04_link_foreign_tables_without_kirby.patch) — **DIRECT CONDITIONAL CANDIDATE / configuration-dependent**.

Check preimages with `python Briefs/tools/check_candidates.py --repo . --candidate R04`. Applying a patch and passing a host test do not establish natural-path correctness.

## Disprove this candidate before stacking patches

The host fixture uses the exact helper bodies with stand-in tables, not full generated images. Verify foreign epoch indices against the actual boomerang image and exercise natural ground/air throw-return-catch in the build that failed.

## Acceptance for this symptom

Link remains source-correctly visible while throwing, waiting and catching; hands/equipment and boomerang are correct, with no flicker or stuck hidden parts. Natural source-default move proof covers ground/air and repeat use; diagnostic-only poses and counters do not close it.

## Required regression scope

Retained Link Catch proof, shield/sword equipment, Up-B, Kirby CopyLink, boomerang collision and move timing.

## Coordination

Coordinate shared Link owner-program edits with Up-B but keep separate acceptance.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
