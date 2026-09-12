# King Dedede — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 4. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

Dedede's up-special source maintains a four-byte player-indexed button buffer, documented as a single-frame buffer for shortening the move. The move also consumes available jumps using a maximum-jumps attribute and applies phase-dependent movement.

This creates a concrete per-slot state and input-timing requirement. Rendering once per two source ticks must not accidentally make the shortening window twice as long.

### Evidence and read boundary

- [src/Dedede/DededeSpecial.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Dedede/DededeSpecial.asm#L1-L135) — inspected source slice, lines 1–135.

Opening up-special code is inspected; full inhale/article behavior and simultaneous population are not proved.

## Work sequence

### 1. Movement and input ownership

Move the source buffer semantics into explicit per-instance/slot state with defined initialization and reset. Port the exact shortening, lateral/vertical movement, gravity and landing transitions from the remaining callbacks.

### 2. Capture and articles

Resolve inhale, capture, release, generated allies/projectiles and all donor-defined interactions. Determine where source resources live and whether copied abilities share a subset. Do not assume inhale implies copying powers.

### 3. Large native owner

Measure geometry/texture/pose demand alongside article and hammer-related parts. Precompute or specialize gameplay attachments without deriving collision from a stale visual-only skeleton. Keep four mirrors independent.

## Directed acceptance witnesses

- [ ] Shorten input just before, on and after the source window; equivalent behavior across render scheduling.
- [ ] Multi-jump consumption and recovery interruption restore the correct state.
- [ ] Capture/release of every victim class, target KO and stage/ledge interaction.
- [ ] Maximum source-supported article population, four Dededes, costume variants and repeated matches.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After capture and article seams; record precise buffer semantics before optimizing frame scheduling.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
