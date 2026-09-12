# Sheik — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 4. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The inspected up-special code distinguishes ground and air initialization, stages begin/move transitions, scales existing horizontal velocity and handles different collision transitions. Its movement constant includes 0x438C0000 (280.0).

These are reasons to test high-speed collision and event timing, not reasons to multiply all parameters by two for a 30-FPS renderer. The original draft's needles/chain inventory still needs full source resolution.

### Evidence and read boundary

- [src/Sheik/SheikSpecial.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Sheik/SheikSpecial.asm#L1-L115) — inspected source slice, lines 1–115.

Only the opening up-special slice was inspected here. Complete needle/secondary-special behavior and pool maxima remain tasks.

## Work sequence

### 1. Persistent ability state

Resolve needle storage/release and every reached secondary special from the pinned tables and callbacks. Identify state lifetimes, cancellation, emission counts and required joint/material resources. Do not infer a dependency on global charge-smash rules from a similarly named field.

### 2. Vanish/recovery transitions

Port ground/air begin, move and exit collision behavior with its source velocity changes, visibility and damage-state transitions. Separate render invisibility from hurtbox/collision flags.

### 3. Fast-joint and article work

Compute gameplay-relevant origins/hitboxes at the rate their events require. Use existing native transform/baked trajectory machinery rather than full generic traversal for a projectile origin. Determine source article bounds before assigning pools.

## Directed acceptance witnesses

- [ ] Needle store/cancel/release and any copied version persist/reset in the correct source states.
- [ ] Recovery near edges, walls, ceilings, platforms and moving ground, including interruption during invisibility.
- [ ] Held items, grabs and damage restoration after each special exit.
- [ ] Four Sheiks and mixed projectile/copy encounters preserve cadence and deterministic event order.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After persistent-state, event and article seams are qualified; exact ordering within this wave follows measured resource dependencies.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
