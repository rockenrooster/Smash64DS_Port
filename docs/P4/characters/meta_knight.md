# Meta Knight — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `joaorb64/smashremix-plus-extra` at `96621afea26a83305abaf81add07dcf5a9c5fe3e`.  
**Default production wave:** 3. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The config selects JIGGLYPUFF as base but supplies its own multi-jump parameters, six costumes, wing-part cloaking data and Kirby bonus-stage assignments. The main source includes both MetaKnightSpecial.asm and CPU.asm; it also declares tornado start/loop/end states and assembly-built item streams.

The four special-file slots being zero is not proof of no special resources: animations, model parts, scripts and reached shared effects still matter.

### Evidence and read boundary

- [extra_characters/MetaKnight/config.yaml](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/extra_characters/MetaKnight/config.yaml#L1-L155) — inspected source slice, lines 1–155.
- [extra_characters/MetaKnight/main.asm](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/extra_characters/MetaKnight/main.asm#L1-L125) — inspected source slice, lines 1–125.

Config values are known, but full jump-helper, CPU and special behavior remain to be validated. Bonus-stage assignments are not playability proof.

## Work sequence

### 1. First EXTRA adapter canary

Resolve config, request lists, generated symbols, sound substitutions and binary assets in a disposable donor build. Compare base/submodule pins and source hashes. Emit the same native production metadata as the main Remix adapter, not another runtime loader.

### 2. Jumps and special states

Read the configured jump helper and MetaKnightSpecial.asm. Port actual jump consumption/height rules and tornado/recovery/other reachable states. Convert source loops and item streams; include CPU.asm behavior rather than inheriting Jigglypuff AI by default.

### 3. Wings and modes

Compile wing/sword/model visibility and source cloaking changes natively. For a later campaign/bonus tier, start from the explicit Kirby stage assignments and test traversal; do not claim new bonus-stage design is necessarily required.

## Directed acceptance witnesses

- [ ] All supported jump transitions consume and restore the correct state, including recovery interruption.
- [ ] Tornado start/loop/end and other source-special exits clean up visual and gameplay flags.
- [ ] Six-costume inventory, wing visibility, item swings, reflected facing, capture and copied-power policy.
- [ ] EXTRA import produces stable hashes on rebuild and reaches the same downstream checks as Falco.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

Run EXTRA source/asset admission in P4.0; complete the character after the common native pipeline is proven.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
