# Lanky Kong — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 5. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

Lanky's neutral special is a Grape Shooter. Both inspected entry paths initialize ammo to 3 for that special's startup; this is not evidence for three shots per stock. Emission is gated by a moveset flag which is then cleared.

Lanky and Kirby use different weapon joints and origins. The Kirby X-offset instruction loads 0x43960000 (300.0), while its comment says 200. Decode numeric bits and consumers rather than treating prose as executable truth.

### Evidence and read boundary

- [src/Lanky/LankySpecial.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Lanky/LankySpecial.asm#L1-L120) — inspected source slice, lines 1–120.

Exact ammo-consumption loop, all attachment transformations and remaining specials have not been fully inspected.

## Work sequence

### 1. Exact numeric and flag semantics

Resolve ammunition decrement/repeat/cancel behavior and projectile spawn arguments from code. Add a unit fixture for the 0x43960000 discrepancy and for source signed/float conversions. Translate verified values into the chosen DS representation, with acceptable mechanical-equivalence tolerances.

### 2. Joint-dependent gameplay

Audit long-arm, weapon, grab and other changing-joint consumers. Generate efficient gameplay-relevant transforms or verified trajectories; do not use an old visual pose as the projectile/grab origin. Separate Kirby attachment definitions.

### 3. Complete donor move set

Resolve all remaining balloon/trombone/stretch-related inventory against the actual action table and callbacks. Compile all modelpart/effect variants, ordinary item states and AI; no guessed mechanics from the character name.

## Directed acceptance witnesses

- [ ] Ammo initializes/decrements/repeats exactly as source; aerial and grounded cancels do not reuse stale flags.
- [ ] Lanky and Kirby origins match the source instructions, including the decoded 300.0 fixture.
- [ ] Long-reach captures/hits across facing, animation-rate changes and victim archetypes.
- [ ] Four mirrors plus article pressure, ordinary items, stock transitions and rematches.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After gameplay-joint and article support; include the numeric fixture in the importer immediately.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
