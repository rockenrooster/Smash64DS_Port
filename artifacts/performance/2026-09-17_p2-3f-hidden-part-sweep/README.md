# Every fighter swept: three drawing hidden parts exist, one is still uncovered

Kirby's copy and Yoshi's throw were both found by a human watching a fighter
disappear. This sweeps the whole roster mechanically for the same class, so the
next one is not found that way.

## The class

An `ftdata.c` motion row's third field carries `FTANIM_FLAG_*` in its low half
and an anim-desc mask in its high half. A set bit at position `31 - i` installs
`<Fighter>Main.hiddenparts[i]`. If that hidden part's root joint carries a
display list, it **adds a root** to the live vector. `nds_renderer_assets.c`
rejects a native owner on `owner->root_count != root_count` before it compares
offsets, so nothing but a complete root program can match — and at
`NDS_RENDERER_PROFILE_LEVEL 0` a declined owner draws **nothing**. The whole
fighter vanishes for the length of the move.

Indices 0, 1 and 2 are the `TRANSN`/`XROTN`/`YROTN` joints, `kind 3`, which
never carry a display list. Only indices 3 and up can do this.

## Method

For each of the 26 owners the generator knows, with a `<n>_<Title>Main.c` and a
`reloc_fighters_main/<Title>Main` O2R:

1. Collect every `ftdata.c` row naming `llFT<Title>Anim…FileID`, OR the hex
   literals in its flags field, and mask off the low half.
2. Read that Main's `hiddenparts` table — offset and row count taken from the
   file's own `/* @ 0xNNNN … hiddenparts target */` comment, the same
   convention `OWNER_ROOT_PROGRAM_SOURCES` already pins for Samus (`0x0050`),
   Link (`0x00d0`) and Yoshi (`0x0084`).
3. For each mask bit, resolve the hidden part's root joint to a JointTree
   descriptor (`joint - 4`), skip anything `setup_parts` already selects, and
   report it only if its descriptor has a display list.

## Result

Three `(owner, mask)` combinations in the entire roster:

| owner | mask | hidden parts | drawing roots | motions | covered? |
|---|---|---|---|---|---|
| samus | `0x00180000` | 11, 12 | j24 `0x2c20`, j25 `0x2ce8` | `FSmash`, `FSmashHigh`, `FSmashLow`, `FSmashMidHigh`, `FSmashMidLow` | **NO** |
| samus | `0x1ff80000` | 3–12 | the same two | `Catch`, `CatchPull`, `ThrowF`, `ThrowB` | yes — the Catch program's own model-part commands set joint 24 to part 1 and hide joint 25, so neither offset survives into its vector |
| yoshi | `0x18000000` | 3, 4 | j9 `0x2800` | `Catch`, `CatchPull`, `EggLay*` 202-206, `ThrowF`, `ThrowB` | yes, as of `2026-09-17_p2-3f52-yoshi-root-programs/` |

Link's Catch mask `0x1C000000` does not appear, and that is correct rather than
a gap: its hidden joints have `dl == NULL`: they become drawable only through
the motion's own modelpart-0 writes, which the existing Link Catch program
already carries.

## The one that is open: Samus's forward smash

Derived through the real runtime context, which reproduces the shipped Catch
assertion (21 roots) as its control:

```
=== high: canonical 14 roots, 20 resident
  Catch  0x1FF80000: 21 roots, new vs canonical=[0x8d90, 0x9140 x5, 0x8a70], NOT RESIDENT=[]
  FSmash 0x00180000: 16 roots, new vs canonical=[0x2c20, 0x2ce8],           NOT RESIDENT=['0x2c20', '0x2ce8']
  Heavy  0x10000000: 14 roots, new vs canonical=[],                          NOT RESIDENT=[]
```

Identical in Low. `0x2c20` and `0x2ce8` are **not resident in any detail**, so
every one of Samus's five forward-smash motions presents a 16-root vector that
no owner can match.

This is a derivation, not an observation: nobody has reported Samus vanishing on
F-smash, and the question added to `docs/BUGS.md` has not been answered. The
static chain says it must, by the same mechanism that made Kirby's copy and
Yoshi's throw vanish.

### Implemented

Two appendix rows (`0x2c20`, `0x2ce8`, binding 7 — joint 16 is both hidden
parts' parent and holds canonical binding 7 in both details) and one program
with no model-part events, since `dSamusMainMotion_FSmash*` issues none
(`216_SamusMainMotion.c:1205-1284` contain no model-part command).

The Samus branch of `build_owner_root_programs` asserted `len(programs) != 1 or
programs[0][0] != "Catch"` and read its hidden-part IDs from one module
constant, so it became a per-program loop keyed by each program's own source
mask. Its appendix check also had to change shape: exact equality between a
program's new roots and the appendix cannot hold once two programs need
different subsets of it. It is now a per-program subset plus a union equality at
the end, which keeps the property that mattered — no appendix row is baked that
no program uses.

Verified before the edit, by monkey-patching the appendix in memory: both roots
bake (25 commands, 1 epoch, both details), the 16-root vector passes the
vertex-cache closure as self-contained RAW, `_verify_program_roots_lit` passes,
and every cross slot is `CURRENT`. Regenerating afterwards moved 683 lines, all
of them Samus symbols; Link, Yoshi, Kirby and the rest are byte-identical.

## The sweep became a standing check

`scripts/fighters/check_hidden_part_root_coverage.py`, the sibling of
`check_model_part_mutation_coverage.py`. It does not stop at "a drawing hidden
part exists"; it computes the **live root vector** each motion produces and
requires some emitted root program to carry exactly it.

Two things had to be got right, and both were caught by running it against
cases whose answer is already known rather than against a synthetic one:

- **A motion command on a setup-omitted joint is not always a no-op.**
  `_owner_root_program_overrides` treats it as one, which is correct in general
  and wrong here, because `ftMainSetStatus` installed that joint from this same
  mask before the motion ran. Samus Catch is the proof: all six of its
  modelpart-0 writes land on hidden joints, and dropping them reads 16 roots
  for a 21-root program.
- **Samus keeps those writes in a subroutine.** `dSamusMainMotion_Catch` calls
  `ftMotionCommandSubroutine(dSamusMainMotion_0x0D10)`, and the eight
  `SetModelPartID` commands are in there — the same eight
  `OWNER_ROOT_PROGRAMS["samus"]` currently hard-codes. Reading only the named
  motion body finds no events at all. The checker expands subroutines in source
  order, so a later write to a joint still wins.

With both fixed it reported F-smash alone, in both details, and passed Samus
Catch, Yoshi Catch and Yoshi Throw — so it was validated against three
known-good cases and the one known-bad case, in the state that actually shipped,
not against a synthetic fixture. With the F-smash program landed it reads

```
  hidden-part root coverage: 11 fighters swept, 6 owner/detail cases install a drawing hidden part
HIDDEN_PART_ROOT_COVERAGE_OK every drawing hidden part a motion installs is carried by a native root program
```

Known limitation, stated rather than hidden: it follows `Subroutine` but not
`Goto`, so a model-part write reachable only through a jump would be missed.

## A constant that does not match its source

`SAMUS_CATCH_HIDDENPART_IDS` is `range(3, 12)` while the Catch mask
`0x1FF80000` is indices 3..**12**. That is not a live defect — index 12 is joint
25, which the Catch motion hides with `(25, -1)`, so the vector is 21 roots
either way, and this sweep's mask-derived run reproduces exactly that. It is
still a constant that does not match its source, and moving to mask-derived IDs
removes it.
