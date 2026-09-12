# Wario — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 2. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

Wario's idle sequence is built directly in assembly from waits, blink subroutines and model changes. Both neutral-special entries create a concurrent NSP_TRAIL stream. Victory and selection sequences add their own loops and waits.

These are concrete tests for the event importer; the local .bin list is not the whole character. Do not substitute a bike/Waft moveset from another game.

### Evidence and read boundary

- [src/Wario/Wario.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Wario/Wario.asm#L1-L140) — inspected source slice, lines 1–140.

The actual special callbacks and all source collision hooks still need a complete review; no modern-game mechanics are assumed.

## Work sequence

### 1. Event graph completeness

Export assembly-built idle, concurrent neutral-special trail and victory streams. Preserve the lifetime of secondary streams across source-defined action changes. Reject orphaned or multiply executed presentation streams.

### 2. Behavioral delta

Read actual neutral-special recoil and up/down-special callbacks, plus shared Mario hooks. Separate damage/collision flags from cosmetic trails; terminating a trail must not terminate an active hitbox or vice versa.

### 3. Parts and lifecycle

Compile blink/eye/hand/modelpart alternatives and entry/results assets through native generation. Audit body-slam/collision hooks wherever they actually live; do not infer completeness from the main action table.

## Directed acceptance witnesses

- [ ] Idle runs long enough to traverse all assembled waits/eye states; no missing subroutine tail.
- [ ] Concurrent neutral-special trails terminate correctly on recoil, damage, landing, KO and scene exit.
- [ ] Ground/air/landing special variants and source collision/clang interactions.
- [ ] Four Warios and Mario/Wario/Kirby mixtures, items, selected costumes and repeated results loops.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After the assembly-built/concurrent event importer is qualified with Falco and shared tests.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
