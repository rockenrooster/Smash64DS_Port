# Banjo & Kazooie — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 5. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The neutral-special source declares forward EGG_DURATION = 30 and BACKWARD_EGG_DURATION = 120. Its startup/main paths distinguish grounded/airborne states and select forward/backward behavior from input and facing. It explicitly maps Banjo and Kirby to different actions.

Some neutral-special setup is documented as based on Samus routines. This is another reason to track actual callback dependencies separately from a character's setup parent.

### Evidence and read boundary

- [src/Banjo/BanjoSpecial.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Banjo/BanjoSpecial.asm#L1-L105) — inspected source slice, lines 1–105.

Durations are observed constants, not a proven pool count. Complete emissions, caps and other specials require callback inspection.

## Work sequence

### 1. Egg behavior and pools

Read the emission, movement, bounce/collision and lifetime consumers before converting those constants into capacities. Derive live-population bounds from minimum emission interval, duration, ownership rules and any explicit source cap. Include back-fired eggs; do not size from the shorter duration only.

### 2. Remaining specials and state

Resolve every effective special, including any source-declared persistent counters. Do not invent a Wonderwing-use limit from another game. Implement independent state and correct stock/reset behavior.

### 3. Multipart native actor

Preserve source body/partner/weapon attachment and visibility. Share generated data where safe, but keep the gameplay egg origin and hurtboxes distinct from visual pose throttling. Prepare the smaller Kirby ability closure explicitly.

## Directed acceptance witnesses

- [ ] Forward/backward selection across input deadzone and facing changes, on ground and in air.
- [ ] Simultaneous back-fired eggs survive their source lifetimes and interact correctly with stage/fighters.
- [ ] All source special counters and effects reset at the right lifecycle boundary.
- [ ] Three Kirbys plus Banjo, multiple costumes and item-heavy mixed scenes.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After projectile/ability-fragment support; source-only census includes both egg lifetime classes.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
