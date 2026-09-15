# 22 — Pikachu strong side-A: missing attack effect

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Attack telegraph / move-specific VFX.
**Existing owner:** P2-3 fighter/electric effects owner.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> strong side a effects not rendering.

## Current source findings

“Strong side A” does not uniquely identify tilt versus smash. Preserve the report and cover both source move branches rather than inventing a status ID.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [R61](research/SOURCE_LEDGER.md#r61), [R64](research/SOURCE_LEDGER.md#r64).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Trace the source side-tilt and side-smash effect commands and identify which produces the reported electrical telegraph. Wire a missing native effect/particle family through the existing generator, exact joint binding and active frame/lifetime. If the maker already fires, repair its material/frame or attachment owner. Keep hitbox timing, RNG and move strength unchanged; do not use extra synthetic damage sparks as the telegraph.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md](research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md).

## Disprove this candidate before stacking patches

A hit spark after contact is not proof of the attack VFX before contact. Capture whiff and hit, both facings, and the relevant tilt/smash variants.

## Acceptance for this symptom

Each affected source move shows the correct effect at its original event time and attachment in both directions, with the correct electrical/transparent appearance and cleanup. Hitboxes, active frames and movement are unchanged. Report the resolved move names and remaining ambiguity explicitly.

## Required regression scope

Other Pikachu normal attacks, neutral/down-B, shared electric material/atlas, damage overlays and event dispatch timing.

## Coordination

Do not count electric-damage victim proof as proof of this attacker-side move effect.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
