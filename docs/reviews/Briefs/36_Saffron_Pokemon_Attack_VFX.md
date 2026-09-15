# 36 — Saffron City: native Pokemon attack effects

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Hazard attack telegraph / missing VFX.
**Existing owner:** P2-5i2 / P2-4 Yamabuki actors.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Pokemon are missing the VFX for their attacks.

## Current source findings

Saffron stage Pokémon are item/ground-monster actors, not interchangeable with Poké Ball Pokémon. Their bodies, weapons and effects have distinct native owners; a complete body maker mask does not prove attack VFX.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [R73](research/SOURCE_LEDGER.md#r73), [R74](research/SOURCE_LEDGER.md#r74), [R106](research/SOURCE_LEDGER.md#r106), [R64](research/SOURCE_LEDGER.md#r64).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Enumerate each source ground-monster attack’s weapon/effect children, including shared dust/explosion families, and generate the required native owners/materials/scripts. Bind children to the source ground-monster transform and actual display callback/link. Preserve spawn, attack and retire callbacks so fixing VFX does not keep the garage door open indefinitely. Share genuinely common native effect implementations with items but retain the stage actor’s source attributes and lifetime. Verify every reachable ground-monster attack, not only Electrode or one body draw.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md](research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md).

## Disprove this candidate before stacking patches

Zero missing-body owners cannot qualify weapon/effect children. Do not use generic substitute particles, skip a monster kind, or change the source spawn law to avoid unimplemented effects.

## Acceptance for this symptom

Every source-reachable Saffron monster attack has its required visible effects during a natural hazard cycle, correctly positioned, colored, blended and timed, with cleanup afterward. Include an explicit monster/phase coverage matrix and positive native output; leave unexercised species/phases open rather than extrapolating from one explosion.

## Required regression scope

Door cycle, monster collision/damage/audio, common particle/effect consumers, Poke Ball monsters sharing a repaired primitive, resource pressure and scene exit.

## Coordination

Coordinate with Saffron door and common VFX producers; do not broaden this report to every unrelated Poke Ball feature.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
