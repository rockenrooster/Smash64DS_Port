# Bowser — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 3. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The checked-in filename is lowercase bowser.asm. Its tables remap many common, damage, recovery and ledge animations to Bowser-specific files. The script inventory includes multi-part forward throw streams, grounded/airborne/landing down special and an aerial-landing event.

That surface makes Bowser a useful non-Fox asset/topology test. A Yoshi setup parent, by itself, would not prove matching skeletons or collision behavior.

### Evidence and read boundary

- [src/Bowser/bowser.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Bowser/bowser.asm#L1-L145) — inspected source slice, lines 1–145.

Skeleton size, actual DS geometry, texture demand and complete special semantics have not been measured.

## Work sequence

### 1. Early asset canary

During P4.0, decode model topology, effective detail selection, materials, joint references, hurtboxes, action counts and closure sizes before porting the complete moveset. Feed these into the same native owner pipeline used for Falco; do not build a Bowser-only importer.

### 2. Full behavior

Resolve BowserSpecial.asm and reached common patches. Implement the source neutral-special emissions, up special, down-special ground/air/landing transitions, grabs and the complete forward-throw sequence. Determine actual article caps and timing from source.

### 3. Geometry and fidelity

Measure whole-scene polygons/vertices and texture placement in four-way encounters. Convert source geometry first; any lower-detail substitution must preserve required parts and follow measured-conflict/owner-approval policy.

## Directed acceptance witnesses

- [ ] Every emitted gameplay joint/hurtbox index is valid in the generated topology.
- [ ] Forward throw traverses all required streams, with victim alignment across the roster.
- [ ] Down special and aerial landing handle edges, moving/pass-through floors, interruptions and stock loss.
- [ ] Four Bowser costumes and mixed heavy geometry/item/effect scenes remain admitted and native.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

Run the asset/topology canary in P4.0; finish the full character after the simpler admitted slices.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
