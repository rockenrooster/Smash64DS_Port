# 35 — Saffron City: restore the garage-door state/animation cycle

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Stage hazard state/timing.
**Existing owner:** P2-4 Yamabuki stage actor.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> Saffron city: the pokemon garage door hazard is always open for some reason. It should close and open periodically.

## Current source findings

The original imported state machine already owns the cycle. In Open, it calls SetClosedWait only when monster_gobj becomes NULL; SetClosedWait resets waits, position and closing animation. The gate has its own gcPlayAnimAll process. A baked open pose alone is not evidence that runtime animation is frozen.

Source keys: [R73](research/SOURCE_LEDGER.md#r73), [R106](research/SOURCE_LEDGER.md#r106), [R74](research/SOURCE_LEDGER.md#r74).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

First compare gate_status, monster_gobj, animation frame and the submitted live gate matrix. If state remains Open after the monster should retire, repair the actual source monster-clear/ejection callback connection. If state and animation advance but pixels stay open, repair actor identity/binding and matrix memo invalidation so the native gate consumes that live DObj rather than a static map pose. If no animation advances, repair process/animation ownership, not source timers. Preserve the source timer/proximity opening behavior and collision yakumono slot synchronization.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md](research/MATERIAL_DEPTH_AND_STAGE_REPAIRS.md).

## Disprove this candidate before stacking patches

Do not implement a new fixed-period door oscillator or forcibly NULL the monster on a render frame. Source closing depends on monster lifecycle, so the two are not interchangeable.

## Acceptance for this symptom

Door visibly closes and reopens according to source timers/proximity/monster lifecycle, with correct animation and sounds. Several cycles, near/away cases and repeated stage entry pass. State traces and pixels agree; the door is not merely animated independently of the hazard logic.

## Required regression scope

Saffron monster spawn/cleanup, collision/detect line, VFX and sound, actor transforms, pause/resume and native packet lifetime.

## Coordination

Saffron monster VFX has its own brief; door closure and effect visibility need distinct evidence.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
