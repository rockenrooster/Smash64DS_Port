# The four-CPU build carries 8,458 B of entry effects for fighters it cannot run

**Sized, not implemented.** This is a lever with a measured prize and a stated
mechanism; no code has been written for it.

## Why this was measured

The Yoshi egg owner (`…_p2-3f53-yoshi-egg-owner/`) built clean, checked green,
and turned Boundary RED anyway: about 1 KB of new resident packet data pushed
`gNdsTaskmanArenaChosenSize` down one 4,096-byte page, `AllocFailCount` 84 → 85,
and 14 draws lost their texture bind. The arena is requested at
`NDS_TASKMAN_ARENA_SIZE` and steps **down** until a `calloc` succeeds, so the
four-CPU build is sitting exactly on a page edge.

`diagnostics_mp_taskman_state.c:740-756` already states the rule: *"NEVER raise
this without returning at least as much static image first."* So the question is
where static image can be returned — and the entry-effect packet answers it.

## The measurement

The four-CPU stress roster is **Donkey / Samus / Link / Kirby**. The entry-effect
packet is emitted once and linked into every build, so it also carries Mario's,
Fox's, Captain's and the Poke Ball entry effects, which cannot play there.

Root ordinals come from the emitted `*_ROOT_FIRST` / `*_ROOT_COUNT` macros; each
group row's texture index (column 3) and root index (column 4) were identified
by structure, not assumption — column 4 is the only non-decreasing column
spanning 0..61, and column 3 is the only one whose values are 0..63 plus the
255 "no texture" sentinel.

**25 of 62 roots belong to fighters absent from this roster**: Mario 2, Fox 8,
Captain entry 10, Falcon Kick 1, Falcon Punch 1, Fox's reflector 1, MBallRays 2.

| owner | texel bytes used only by it |
|---|---|
| FOX | 4,373 |
| CAPTAIN | 2,201 |
| REFLECTOR (Fox's) | 1,349 |
| FALCON_KICK | 331 |
| MARIO | 158 |
| MBALLRAYS | 46 |
| **total returnable** | **8,458** |
| shared with a present owner | **0** |

**The partition is clean — not one texel array is shared between an absent and a
present owner**, so nothing has to be kept for a fighter that is playing.

**8,458 bytes is 2.1 arena pages**, against the one page the egg needed.

Two reasons this is a *lower* bound:

- It counts **texels only**. The group, position, corner-S/T, corner-colour,
  corner-position and root rows for those 25 roots are additional, and the
  corner arrays alone are 10,116 bytes across all owners.
- `FALCON_PUNCH` scores 0 because its three texture variants bind through the
  material slot at runtime rather than through the group's static texture
  index, so they are invisible to this attribution.

For scale: entry-effect symbols total **61,008 bytes** across 146 symbols in
`builds/build-p2-fourcpu-tickhud/…-hwtri.elf`, of which the texel arrays are
17,247.

## The mechanism to return it

Per-roster conditional emission, the same shape the Yoshi egg needs. Today the
admission arms are already `#if NDS_P2_<FIGHTER>`-guarded in places, but the
generated **data** never is, and `--gc-sections` cannot drop it because the
shared packet tables reference every row.

Unlike the egg — whose rows all sat at array tails, making the guard trivially
index-safe — these ranges are in the **middle** of the tables (Fox is roots
2..9, Captain 13..22). Guarding them changes every later ordinal, so this needs
the emitter to renumber per configuration rather than to wrap tail rows. That is
the real cost of this lever and it is the reason it is sized here rather than
attempted.

## What this would unblock

- The Yoshi egg (~1 KB), currently reverted on exactly this budget.
- Kirby Vulcan Jab and Pikachu Thunder, the two remaining `P2-3f53` items, which
  would hit the same wall.
- Any future resident growth in the four-CPU arm, which is otherwise blocked.

## What it does NOT claim

**Returning image is not a tick lever on its own.** It buys arena headroom. The
related finding `…_p2-2p8-roster-variance/` measured owner-image growth of
28,848 B costing **+70,016 WORK-H with zero extra triangles**, which makes
"shrinking the image recovers ticks" a reasonable hypothesis — but it is a
hypothesis, and this artifact does not test it. Nothing here should be counted
against the 321,866-tick residual until it is measured.
