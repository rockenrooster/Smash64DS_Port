# 12 — Yoshi Up-B: egg and shell presentation closure

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Attack telegraph / missing VFX.
**Existing owner:** P2-3f52.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Up B egg shells are not rendering.

## Current source findings

The Yoshi shell fragment, egg weapon and fighter pose are distinct source consumers. The older EggBreak/weak-stub and atlas-size diagnoses must be reconciled with current landed code; no missing-script ID was independently decoded here.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [R52](research/SOURCE_LEDGER.md#r52), [R61](research/SOURCE_LEDGER.md#r61), [R64](research/SOURCE_LEDGER.md#r64).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Derive the Up-B source move trigger and break/throw children, then extend the existing native weapon/effect producer for every referenced shell/egg root and its material frames. Bind the shell child to the source egg transform, update lifetime through the source callback, and dispatch it on its actual source link. If the trigger and owner engage but the fragment is absent, patch atlas cell selection/alpha or parent-generation lifetime, not the move command. Integrate with the same egg resource closure used by egg lay/entry only where the source actually shares assets.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md](research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md).

## Disprove this candidate before stacking patches

Re-enabling a formerly weak function or adding an already-present atlas sheet is not evidence. Positive source spawn plus positive native draw plus visible shell fragments are all required.

## Acceptance for this symptom

Ground/air Up-B visibly shows every source-required egg/shell phase through both facings, collision and expiration, at the correct attachment and timing. Positive native engagement for each child, no missing frames, leaks or altered projectile/hitbox behavior.

## Required regression scope

Yoshi neutral-B egg, entry egg, shield, catch/throw body visibility, repeated eggs and four-fighter resource pressure.

## Coordination

Share generated source closure with the other Yoshi briefs; keep each natural trigger and visual check separate.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
