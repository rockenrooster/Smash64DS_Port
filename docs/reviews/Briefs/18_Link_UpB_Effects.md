# 18 — Link Up-B: complete spin-attack effects

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Attack telegraph / VFX correctness.
**Existing owner:** P2-3f33.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Up B effects aren't rending properly

## Current source findings

The repository records a partial Link Spin implementation and outstanding natural/source-default qualification. The native entry-effect path already recognizes relevant owners; do not re-add an owner solely because an old board row says missing.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [R64](research/SOURCE_LEDGER.md#r64), [R65](research/SOURCE_LEDGER.md#r65), [R66](research/SOURCE_LEDGER.md#r66), [R78](research/SOURCE_LEDGER.md#r78).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Verify grounded Spin body/sword root changes and the air effect-only sibling separately. Bind the source sword/Spin child to its live joint and correct foreign material/image owner; preserve source frame, lifetime and UV animation. Audit the entry-effect group alpha/depth contract and use R02 only after identifying its source geometry-state inheritance. Generate any genuinely missing reachable root sibling into both detail images rather than changing source motion or discarding an unrecognized child.

### Executable candidate diffs

[R02_effect_initial_z.patch](runtime/R02_effect_initial_z.patch) — **CONDITIONAL CANDIDATE / sibling audit required**.

Check preimages with `python Briefs/tools/check_candidates.py --repo . --candidate R02`. Applying a patch and passing a host test do not establish natural-path correctness.

Implementation contract and code-oriented integration details: [research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md](research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md).

## Disprove this candidate before stacking patches

A grounded diagnostic pose with triangles does not prove airborne Spin or body restoration. A depth fix does not close missing geometry/material frames.

## Acceptance for this symptom

Ground and air spin effects appear with source-derived shape, texture, transparency, orientation, attachment and timing, without invisibility or bad occlusion. Provide full-move natural-input captures for both facings, positive per-component native engagement and restoration afterward.

## Required regression scope

Link Neutral-B/Catch, sword equipment, Kirby CopyLink, other translucent combat effects and gameplay timing/cadence.

## Coordination

Share only proven common Link or material defects; retain independent air/ground proof.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
