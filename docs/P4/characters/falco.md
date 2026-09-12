# Falco — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 1. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The pinned action table assigns Phantasm callbacks to neutral-special actions 0xE1 and 0xE2. Fire Bird and reflector states are named separately. This is not a Fox-blaster replacement inferred from another Smash game.

The assembly appends GO_TO commands to inserted binary streams and injects THROW_DATA pointers. USP_GROUND_MOVE deliberately has no END and falls through into USP_LOOP; the airborne stream also reaches that loop. Inherited fields use -1, which must be resolved rather than treated as a valid final value. The same source includes AI, menu, costume, crowd and Kirby-inhale setup.

### Evidence and read boundary

- [src/Falco/Falco.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Falco/Falco.asm#L1-L245) — inspected source slice, lines 1–245.

Exact callback semantics, complete inherited closure, geometry, loaded bytes and performance remain unmeasured.

## Work sequence

### 1. Resolved import

Export inherited and overridden action parameters and callback slots, including scripts assembled outside .bin files. Preserve the fall-through and return/loop structure in a typed event graph. Compare the graph against the linked donor before any hand-port is called complete.

### 2. Native behavior

Read src/Falco/Phantasm.asm and all reached shared Fox hooks. Port grounded/airborne entry, movement, interruption and collision separately. Keep Fire Bird and reflector source differences explicit; reuse a Fox callback only after checking those differences.

### 3. Residency and integration

Bring required Fox-derived resources without requiring Fox to be selected. Share immutable blobs, not temporary action flags or costume state. Resolve the source Kirby mapping into a complete reachable ability package. Produce CSS/results, audio, AI and all item-state resources through the normal pipeline.

## Directed acceptance witnesses

- [ ] A Falco-only match loads every inherited dependency; four Falcos keep independent state.
- [ ] Ground and air Phantasm: interrupted startup, collision, landing, ledge approach, damage and stock loss.
- [ ] Ground Fire Bird moveset falls through exactly once into the intended loop; no out-of-bounds decoder read.
- [ ] Reflector ownership/team interactions; Fox plus Falco with different costumes; source-defined Kirby copy acquisition, use and loss.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

P4.1 end-to-end prover. Do not open the full roster implementation until this slice closes.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
