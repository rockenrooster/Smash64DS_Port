# Mr. Game & Watch — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `joaorb64/smashremix-plus-extra` at `96621afea26a83305abaf81add07dcf5a9c5fe3e`.  
**Default production wave:** 6. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The selected folder is MRGAW, not MRGAWPLUS or MRGAWTHREED. The source has Chef, Fire and Judge callback wiring; Judge has distinct grounded/airborne action definitions for outcomes 1 through 9. Do not assume a later game's down-special bucket belongs to this donor.

A shared render-update hook flips X scale using facing for the top, weapon and grab joints. The config describes numerous prop/cloaking variants. Kirby's inhale entry writes hat ID 0x08: that mapping must be resolved, not replaced by an invented unique Chef copy.

### Evidence and read boundary

- [extra_characters/MRGAW/config.yaml](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/extra_characters/MRGAW/config.yaml#L1-L150) — inspected source slice, lines 1–150.
- [extra_characters/MRGAW/GameAndWatchSpecial.asm](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/extra_characters/MRGAW/GameAndWatchSpecial.asm#L1-L145) — inspected source slice, lines 1–145.
- [extra_characters/MRGAW/main.asm](https://github.com/joaorb64/smashremix-plus-extra/blob/96621afea26a83305abaf81add07dcf5a9c5fe3e/extra_characters/MRGAW/main.asm#L345-L550) — inspected source slice, lines 345–550.

The source establishes action families and transform hooks; outcome probabilities, history and Kirby hat meaning remain unresolved until their consumers are read.

## Work sequence

### 1. Correct variant and randomness

Pin MRGAW only in the profile. Resolve Judge selection/history/RNG rules and all nine outcome payloads from the special code. Generate directed fixtures for each outcome; random CPU play is not coverage. Inspect Chef and Fire behavior and source CPU tables.

### 2. 2D-shaped native rendering

Preserve facing-dependent signed scaling, outline visibility, prop selection, item and grab attachment semantics. A DS-native model/billboard solution must still render correctly under the supported camera/pause behavior; do not declare sprites automatically equivalent.

### 3. Copy and resources

Resolve hat 0x08 through the pinned base tables and implement the actual copied or no-copy behavior. Include every outcome’s gameplay/presentation dependencies and shared file 0x164 where reached. Do not import PLUS/THREED assets merely because conditional variant entries mention them.

## Directed acceptance witnesses

- [ ] All nine Judge outcomes in air and on ground, with source selection/history semantics and reproducible RNG.
- [ ] Facing changes while holding items, grabbing, attacking, paused/orbiting and entering/results.
- [ ] Chef emission/lifetime/collision, Fire transitions, all active prop/material states and source Kirby mapping.
- [ ] Four MRGAWs and mixed item/effect scenes exercise whole-scene transparency/geometry and pool limits.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After the EXTRA adapter and event/part systems are proven; its transform canary may run during early asset admission.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
