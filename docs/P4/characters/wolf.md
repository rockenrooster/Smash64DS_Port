# Wolf — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 2. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The action edits name separate ground/air neutral-special streams, two up-special streams and initial/loop/end reflector states. Wolf also overrides many item pickup, throw and swing animations.

This is evidence of a wider asset/state surface than a Fox costume. The excerpt does not establish the exact projectile parameters or every special callback.

### Evidence and read boundary

- [src/Wolf/Wolf.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Wolf/Wolf.asm#L1-L110) — inspected source slice, lines 1–110.

Projectile caps, source callback timings and precise skeleton-sharing opportunities are not yet established.

## Work sequence

### 1. Resolve effective actions

Read the complete Wolf special and shared Fox paths. Distinguish inherited execution from replaced data and callbacks. Resolve normal attacks, shield transitions, throws and item actions, not just the three special buttons.

### 2. Projectile and reflector integration

Determine emission timing, projectile lifetime, collision and ownership changes from actual callbacks. Fit those resources into the existing weapon/reflector seams with independent per-instance state.

### 3. Share selectively

Deduplicate verified identical immutable Fox-family assets. Keep Wolf-specific model/animation/material sequences and late item constructors resident when reachable. Falco is not evidence that Wolf neutral-special behavior is equivalent.

## Directed acceptance witnesses

- [ ] All item pickup/drop/grounded and aerial throw/swing actions select valid Wolf animations.
- [ ] Wolf alone loads inherited dependencies without Fox; four Wolves preserve independent emit/reflect state.
- [ ] Projectile reflection, teams and any source-supported absorption/collision interactions.
- [ ] Mixed Fox/Falco/Wolf/Kirby stress, source copy policy, entries/results and repeated rematches.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After Falco and shared Fox-family source mapping; full production remains gated on resource admission.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
