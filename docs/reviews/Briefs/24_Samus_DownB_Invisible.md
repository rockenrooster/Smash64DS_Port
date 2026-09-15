# 24 — Samus down-B: morph ball and Bomb visibility

**Version:** runtime-candidate research revision, 2026-09-14.
**Inspected base:** `a5c5bc08d8e8661658865216798d600462db948e` (`master`).
**Priority / scope:** Fighter/weapon visibility.
**Existing owner:** P2-3 Samus morph closure + weapon/effect owner.
**Status:** OPEN — candidate code/proposals, not a ROM-verified fix.

Read [COMMON.md](COMMON.md). Reconcile the actual working tree, source-derived
assets and current board before changing code. Preserve unrelated edits.

## Owner report (preserved)

> down B is invisible.

## Current source findings

Samus body morph and the spawned Bomb object are different obligations. Existing body morph programs must be retained; “down B invisible” does not establish which of them is absent.

Source keys: [S02](research/SOURCE_LEDGER.md#s02), [S11](research/SOURCE_LEDGER.md#s11), [R18](research/SOURCE_LEDGER.md#r18), [R71](research/SOURCE_LEDGER.md#r71).
These distinguish directly inspected code from repository-recorded proof; none
is a new ROM run by this package's author.

## Potential runtime fix

Use human ground/air Bomb to separate body-program admission from bomb weapon draw. Repair the source-derived morph-program selection or image binding only if it diverges, then close the bomb spawn/motion/material/explosion child owner independently. Keep the source collision, fuse and cleanup; restore Samus’s canonical body on completion and interruptions. Include repeated bombs and resource reuse so fixing the first spawn cannot leave stale texture or child pointers.

### Code delivery boundary

No preimage-matched target-source diff is supplied for this report: the remaining change depends on an unobserved runtime divergence or unavailable generated asset closure. The implementation above is a concrete conditional repair proposal, not a fabricated completed patch.

Implementation contract and code-oriented integration details: [research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md](research/FIGHTER_ROOT_AND_EFFECT_CLOSURE.md).

## Disprove this candidate before stacking patches

Drawing a bomb while Samus stays hidden, or drawing Samus with no bomb/explosion, is partial only. CPU nonengagement remains insufficient.

## Acceptance for this symptom

Samus's source-correct morph body, bomb and required explosion effects are all visible at the proper phases in ground/air natural use. Repeated uses and damage interruption retain correct visibility and resource safety. Do not close on body proof alone if the bomb remains absent.

## Required regression scope

Shield rolls/cliff escapes, Catch, charge shot attachment, bomb collision/jump interactions, pools and scene cleanup.

## Coordination

Share morph seam with roll brief; separate detached weapon and explosion acceptance.

## Closure

Apply COMMON.md's natural-path, positive-engagement, source/pixel/audio, resource,
native-only, cadence and widest-relevant-verifier requirements. Record candidate
identity and actual coverage in the existing BUG_NOTES owner; preserve BUGS wording
and unrelated dirty edits. Report any unrun/failed gate and owner acceptance still
owed. No brief or host-only test closes the runtime bug. Report unverified portions explicitly.
