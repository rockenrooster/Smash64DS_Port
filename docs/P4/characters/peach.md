# Peach — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 4. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

Peach defines explicit pan/racket/club/parasol model states and crown/head/hand swaps. Forward-smash variants select different weapon models. FLOAT is a source script; neutral and up specials use concurrent streams.

Up-special open/float/close/fall states hide held items and select different parasol states. The file also constructs many item-action scripts directly in assembly.

### Evidence and read boundary

- [src/Peach/Peach.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Peach/Peach.asm#L1-L145) — inspected source slice, lines 1–145.

Float duration, item probabilities/populations and full special behavior were not established by the inspected action/script section.

## Work sequence

### 1. Float and input rules

Resolve the actual float state machine, remaining duration, input thresholds, attack transitions, ground/air exits and resets. CPU support must understand the resulting movement state, not merely select a special button.

### 2. Items and special behavior

Inspect source-generated item selection, creation, ownership, timers and randomness. Determine every reachable item variant and resource even when the global item switch disables random item spawns. A fighter-generated item is not automatically disabled by that switch.

### 3. Parts and attachments

Compile all listed model alternatives, held-item hiding/restoration and concurrent special streams into native state changes. Load the reachable union before GO; choosing a pan/parasol/crown must not trigger texture conversion or global owner invalidation.

## Directed acceptance witnesses

- [ ] Float begins/ends correctly around jumps, attacks, landing, damage, grabs and stock transitions.
- [ ] Each source item outcome has valid collision, render and sound resources; deterministic random outcomes.
- [ ] Parasol opening/closing while holding an item restores visibility and ownership correctly.
- [ ] Four Peaches with costume and article pressure; source Kirby behavior; CSS/results/entry parts reset.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After item lifecycle and deterministic resource-union support, without delaying its initial asset census.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
