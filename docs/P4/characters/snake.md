# Snake — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `joaorb64/smashremix-plus-extra` at `96621afea26a83305abaf81add07dcf5a9c5fe3e`.  
**Default production wave:** 6. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The config groups C4, Nikita, up-smash and Cypher hitbox files as Snake-only resources, while placing grenade_hitbox separately for Kirby. Corresponding GFX files are appended as dependencies. It selects Captain as base, seven costumes, explicit hurtboxes and symbolic sound remaps.

These are five named resource families, not measured simultaneous article counts. Neither the config nor a remembered later-game limit establishes how many may coexist.

### Evidence and read boundary

- [extra_characters/Snake/config.yaml](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/extra_characters/Snake/config.yaml#L1-L220) — inspected source slice, lines 1–220.

Article callback semantics, fuses/caps, maximum pool pressure and runtime cost have not been audited or measured in this review.

## Work sequence

### 1. Early risk census

Before full porting, inspect creators, update callbacks, destruction rules, collision ownership and source limits for all five families. Measure the DS representation and native geometry/effects of their reachable assets. Record persistent versus action-local state and possible overlap across stock transitions.

### 2. Gameplay articles

Implement the actual grenade fuse/throw/pickup behavior, C4 placement/detonation, Nikita control/termination, up-smash article and Cypher recovery as specified by the remaining source. Do not assume reflectability, absorbability, one-C4/two-grenade limits or ownership transfer without a witness.

### 3. Kirby and native resources

Generate a grenade ability fragment separate from Snake-only resources, while following any additional copy dependencies. Prepare the entire reachable match union before GO; this is not runtime power loading. Include explosion variants, voices, loops, radio presentation where in scope and all required joint origins.

## Directed acceptance witnesses

- [ ] Prime every source-permitted article combination, then trigger simultaneous effects/collisions.
- [ ] Owner KO, capture, interruption, stock replacement and match teardown preserve/remove each article as specified.
- [ ] Three Kirbys plus Snake can use the source grenade ability without loading all Snake-only data or sharing mutable state.
- [ ] Four Snakes with costumes, items and the measured hard stage satisfy separate RAM/VRAM/pool/CPU gates.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

Run source/resource feasibility in P4.0, finish full production last by default. Early risk work prevents discovering a systemic article problem at the end.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
