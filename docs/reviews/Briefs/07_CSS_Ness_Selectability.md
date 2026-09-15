# 07 — VS CSS: Ness selection through the normal shell

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Roster/gameplay access.
**Existing owner:** P2-3f47 + P2-1 CSS.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Ness not selectable

## Current source findings

The board already records native Ness output. That scoped result does not establish CSS admission, all selected poses or complete move closure in this exact build.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [S10](research/SOURCE_LEDGER.md#s10), [R102](research/SOURCE_LEDGER.md#r102).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Keep the existing Ness owner. Resolve the current selected/idle preview program and admitted resource set, then change the canonical roster capability only after its native completeness and resource tests pass. R03 supplies the remaining shared reset-order repair, not a Ness rendering implementation. Verify source-kind -> menu-cell -> compact pack -> native owner mapping, then both detail modes, natural selection and return/reacquire. Use the same capability definition for producer and runtime admission.

### Executable candidate diffs

[R03_css_reset_image_lifetime.patch](runtime/R03_css_reset_image_lifetime.patch) — **DEFENSIVE CANDIDATE / not a complete CSS fix**.

Check preimages with `python Briefs/tools/check_candidates.py --repo . --candidate R03`. Applying a patch and passing a host test do not establish natural-path correctness.

## Disprove this candidate before stacking patches

A disabled CSS row with a working Ness battle owner is not evidence to rewrite the battle renderer. Do not conflate build admission with the source game unlock flag.

## Acceptance for this symptom

Eligible Ness is selectable and produces the correct visible preview and actual Ness fighter in a normal match. The route survives repetition, preserves source unlock rules and has no growing arena usage, collision refusal or native failure. Existing diagnostic rendering is credited only for its actual coverage.

## Required regression scope

Kirby/Purin admission, Yoshi shared asset 0x152 ownership, source selection sounds and current menu cadence.

## Coordination

Share infrastructure with Kirby/Jigglypuff; do not unnecessarily rerun their unrelated move suites to prove Ness routing.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
