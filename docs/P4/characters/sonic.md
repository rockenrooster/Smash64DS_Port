# Sonic — P4 character subplan

**Status:** planned; source slices inspected, no DS implementation or performance pass claimed.  
**Source:** `JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.  
**Default production wave:** 5. Parent: [master](../New_Characters.md).

## Source observations that determine this plan

The neutral-special source has target state, range/speed/turn/recoil constants and separate locked/unlocked durations. It also shares a helper with Super Sonic and Kirby/JKirby variants.

A mention of Super Sonic in a helper is not permission to expand the planned roster or include every variant's assets. Reachability must retain normal Sonic's actual behavior and the selected Kirby paths without blindly following irrelevant variant branches.

### Evidence and read boundary

- [src/Sonic/SonicSpecial.asm](https://github.com/JSsixtyfour/smashremix/blob/5e04fe7fcd023cd43c71f25f89bb6e810d254d55/src/Sonic/SonicSpecial.asm#L1-L105) — inspected source slice, lines 1–105.

The full target-selection routine and remaining specials are not audited; source constants alone are not a complete motion specification.

## Work sequence

### 1. Targeted neutral special

Resolve actual target selection, tie-breaking, target validity, range, turning and end conditions. Use stable entity references/generations or an equivalent safe mechanism so KO/removal cannot leave a stale target. Preserve source-ordered authoritative decisions.

### 2. Remaining move graph

Inspect spin/charge/spring/recovery paths that the effective Sonic tables actually reach. Determine charge state, source article limits, reuse of spring objects and cleanup; no assumed later-game moveset.

### 3. Fast movement and native visuals

Test gameplay collision at the adopted simulation cadence. Bake roll/modelpart changes and trajectories where valid, while keeping gameplay positions current. Restrict Super Sonic-specific branches through an explicit feature/profile decision, not textual deletion of shared code.

## Directed acceptance witnesses

- [ ] Neutral special with no target, one target, ties, target crossing, target KO and team restrictions.
- [ ] Charge/release and source spring/recovery states near small platforms and ceilings.
- [ ] Four Sonics and fast mixed opponents stress collision and whole-frame CPU, not just render polygon count.
- [ ] Kirby source copy paths and stock/rematch cleanup do not retain target or charge state.

These supplement, rather than replace, the [shared completeness and verification gate](../shared/04_Verification.md). Normal states, source-required CPU/copy/items/audio/UI behavior, all supported costumes, save identity and scene teardown remain mandatory for the declared release tier. No fallback to a donor parent, omitted effect, smaller gameplay pool or restricted matchup can be silently called complete.

## Scheduling and next deliverable

After deterministic targeting and fast-movement tests exist; perform the asset/resource census early.

The first output is a source-qualified action/callback/resource inventory with unresolved entries explicit. Record artifact hashes and actual measurements when available; never put zero in an unknown budget field. Shared mechanisms belong in the existing DS owning subsystem or generator; this card does not authorize a character-specific parallel asset loader.
