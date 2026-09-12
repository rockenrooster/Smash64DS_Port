# Metal Mario — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 1. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The donor file constructs three victory scripts with timed STEP_FGM events, edits menu actions, and disables specific menu scripts with the no-script sentinel. Zero loose moveset binaries does not mean zero script work.

The current DS production manifest already identifies MMario Main/MainMotion/Model roots as a nonselectable campaign variant. That is a reuse opportunity, not evidence that the selectable Remix METAL behavior is already implemented.

### Evidence and read boundary

- [src/MetalMario/MetalMario.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/MetalMario/MetalMario.asm#L1-L45) — inspected source slice, lines 1–45.

The small donor file is not a complete census of shared Metal behavior. No playable-versus-campaign equivalence has been proved.

## Work sequence

### 1. Identity and scope

Treat the requested donor as the pinned Remix METAL selection. Diff its effective attributes, actions, shared Mario hooks, AI, copy policy and mode-specific behavior against P2 MMario. Reuse equivalent P2 machinery, but do not silently substitute a different playable variant.

### 2. Separate campaign behavior

Keep campaign opponent setup distinct from selectable-fighter rules. Preserve the existing campaign encounter, unlock/progression logic and all legacy IDs. Adding a CSS entry must not make nonplayable variants appear in random selection.

### 3. Finish presentation

Convert the actual timed victory events, menu poses, stock/HUD assets and selected costumes/materials. Derive item and copy support from effective donor tables, rather than a parameter-only estimate.

## Directed acceptance witnesses

- [ ] Playable Metal Mario and the campaign opponent retain their separate mode contracts.
- [ ] All three victory-event schedules play the intended sound without leaking into later scenes.
- [ ] Four mirrors, Mario/Metal Mario mixed selection, item holding, respawn and source-defined Kirby behavior.
- [ ] Old save records and campaign selections keep their existing meaning.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

Complete after Falco proves admission and P2 MMario is qualified. A source-diff task can begin earlier.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
