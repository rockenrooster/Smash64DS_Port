# Ganondorf — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 2. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The inspected idle stream writes 0xD0013F33, described by its source as changing animation speed without changing command execution speed. The distinction between animation time and event time is therefore not optional.

Up-special scripts inject their own throw-data pointer and have grab/release streams. The shared Captain source also contains a Ganondorf down-special animation-structure patch outside the character folder; the shared-file dependency was located, not fully audited.

### Evidence and read boundary

- [src/Ganondorf/Ganondorf.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Ganondorf/Ganondorf.asm#L1-L135) — inspected source slice, lines 1–135.
- [src/captainshared.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/captainshared.asm) — path/dependency located; complete file not audited.

Full shared-hook coverage and victim-offset tables still require source resolution. No assembly-size effort estimate is retained.

## Work sequence

### 1. Captain-family dependency audit

Resolve the effective Captain-derived tables and read the reached portions of captainshared.asm. Map every required patch to an existing or new DS owning seam. Count shared dependencies when estimating work; absence of a large local Special.asm proves little.

### 2. Capture and event clocks

Port up-special capture/release offsets and state transitions as attacker-and-victim behavior. Represent animation-rate and script-rate changes independently. Resolve disabled action sentinels, inherited actions and raw numeric references exactly.

### 3. Native presentation

Bake source model/hand/face selections and attached effects into native owners. Do not leave effect anchoring or copied neutral-special behavior on a generic renderer. Include all source material alternatives in residency.

## Directed acceptance witnesses

- [ ] Idle animation-rate edits do not unintentionally accelerate event waits or loops.
- [ ] Up-special catches each supported victim archetype, releases correctly, and cleans up on interruption/KO.
- [ ] Down-special airborne/landing/flip transitions preserve collision and timing.
- [ ] Captain/Ganondorf mixed matches and Kirby-copy matches have no shared-state leakage.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

Follow Falco; use the existing Captain implementation only as an implementation reference, not as the new fighter specification.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
