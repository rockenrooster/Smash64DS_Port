# Roy — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 3. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The define_character row uses CAPTAIN as the setup parent, while Roy's action table references many MARTH animation files. The setup macro only accepts original-cast parents, so a single clone-parent field cannot express the real dependency graph.

Roy's declared actions include Double Edge Dance and Flare Blade start/charge/attack/strong-attack variants on ground and in air. He is not simply Marth with Counter retained and different sword colors.

### Evidence and read boundary

- [src/Roy/Roy.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Roy/Roy.asm#L160-L285) — inspected source slice, lines 160–285.
- [src/Character.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Character.asm) — path/dependency located; complete file not audited.

The excerpt names charge states but does not establish their thresholds, damage, or all callback-sharing relationships.

## Work sequence

### 1. Explicit donors

Maintain separate setup/action inheritance and asset dependencies. Share verified Marth animation, shield and entry resources through canonical identities. Prove Roy alone loads them without admitting Marth as a fighter.

### 2. Distinct mechanics

Inspect Roy specials and reached Marth helpers. Port the Flare Blade charge/release graph and every source-defined damage/self-effect/maximum-charge rule, without assuming any such value from other Smash games. Resolve Double Edge Dance independently from Marth branch timing.

### 3. Collision and effects

Keep source hit-region differences, fire effects, sound events and state exits explicit. Charge state must reset or persist exactly where the source dictates; four instances never share it.

## Directed acceptance witnesses

- [ ] Roy without Marth selected has a complete immutable dependency closure.
- [ ] All charge entry/release thresholds, strongest branch, interruption, landing, KO and rematch reset.
- [ ] Double Edge Dance branch windows and transitions differ only where the pinned source says.
- [ ] Roy/Marth mixed costumes, sword/effect pressure, ordinary items, CPU and source-defined Kirby behavior.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After Marth native asset/family support. Being asset-dependent is not a requirement for Marth to occupy a match slot.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
