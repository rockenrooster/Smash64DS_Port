# Marth — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 3. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The inspected actions identify Dolphin Slash, Counter and Dancing Blade. The declared Dancing Blade graph has stages 1, 2 and 3, with high/mid/low branches on later stages and grounded/airborne versions. Do not invent a fourth stage from familiarity with another game.

Counter-attack prepends RANDOM_SFX with a source voice table. Victory uses a concurrent stream, and item attacks invoke shared event subroutines.

### Evidence and read boundary

- [src/Marth/Marth.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Marth/Marth.asm#L65-L185) — inspected source slice, lines 65–185.

All input windows, hit-region values, counter math and RNG-call behavior still need callback-level source resolution.

## Work sequence

### 1. Sword-family foundation

Resolve all shared resources and callbacks, keeping sword attachment/hit regions independent of purely visual pose updates. Preserve the source hitbox ordering and priority, not an inferred later-game tipper specification.

### 2. Branching and counter

Port the actual Dancing Blade input windows, ground/air transitions and end conditions. Read counter trigger filters, attack result, timing, damage and interruption semantics. Preserve the distinction between effect RNG and authoritative gameplay state under the adopted deterministic policy.

### 3. Reusable outputs for Roy

Publish immutable asset and explicitly reusable callback fragments; do not make Roy require Marth to be selected. Keep distinct action parameters and behavior deltas even when a shared helper is used.

## Directed acceptance witnesses

- [ ] Each declared stage-2/stage-3 branch is reached with boundary-timed inputs on ground and in air.
- [ ] Counter activation, misses, invalid/valid attacks, interruption and post-counter cleanup.
- [ ] Sword hitbox positions versus rendered sword at movement and animation-rate boundaries.
- [ ] Marth-only and Marth/Roy resource sharing, Kirby source mapping, items and all result streams.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

Complete before Roy full implementation; source/asset census for Roy can run earlier.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
