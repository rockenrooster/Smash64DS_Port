# Crash Bandicoot — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 5. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

Crash's neutral-special startup creates/attaches a spin effect, installs a shield-hit callback and applies different horizontal velocity multipliers for grounded and airborne entry. It also distinguishes Crash and Kirby actions/physics constants.

Spin-effect setup reaches Size.link.adjust_usp_gfx_size_.render_routine_, a dependency outside the character file. Port the required visible behavior, not the entire unrelated donor size system.

### Evidence and read boundary

- [src/Crash/CrashSpecial.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Crash/CrashSpecial.asm#L1-L125) — inspected source slice, lines 1–125.

Only startup and initial attachment behavior were inspected; complete special and platform-object rules remain source tasks.

## Work sequence

### 1. Spin and collision hooks

Resolve shield-hit, damage, collision and effect teardown callback lifetimes. Implement ground/air startup differences and copy-specific behavior at the owning DS seams. Never allow a stale callback to survive a state exit.

### 2. Remaining specials and stage interaction

Inspect all effective dig/recovery/platform-related behavior named by the donor inventory. Determine whether created objects are gameplay collision surfaces, effects or both. Their removal and ownership rules decide resource and collision requirements.

### 3. Native attachment

Replace the reached generic donor render path with an appropriate native effect owner while keeping source attachment, scale and visibility semantics. Any optional-presentation degradation must remain local and cannot affect hitboxes.

## Directed acceptance witnesses

- [ ] Spin against shield, hit, miss, aerial landing and interruption; callback state clears at the correct time.
- [ ] Spin effect stays attached across scale/facing/camera changes without generic rendering.
- [ ] Any source-created platform/object interacts and disappears correctly during owner KO and scene exit.
- [ ] Crash/Kirby copies and four mirrors stress independent effects and gameplay pools.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After shared article/collision and attached-effect infrastructure is qualified.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
