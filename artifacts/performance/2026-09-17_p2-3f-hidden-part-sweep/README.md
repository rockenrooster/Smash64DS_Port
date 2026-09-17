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

## A fourth sub-case, swept and found clean — with two lessons

`SetModelPartID(joint, -1)` hides a joint. If that joint canonically draws, the
root count goes *down*, which no variant can cover either. There are 42 such
commands in the game: Samus joint 25 (×1), Kirby joint 12 (×2), and Link joints
11, 19, 20 and 21 (×39). Only two of those joints actually drop a root — Link's
11 (`0x2630` High, `0x6370` Low) and 19 (`0x2c88`); Samus 25 and Kirby 12 are not
in `setup_parts`, and Link's 20 and 21 have `dl == NULL` canonically.

**Link is fully covered**, but the first sweep said otherwise — it reported
seven uncovered vectors, and both reasons it was wrong are worth keeping:

1. **A vector between two adjacent commands is never presented.** Motion
   commands run to completion until a `Wait`, so emitting one state per command
   invents states no frame ever sees. `dLinkMainMotion_AttackAirD` does
   `SetModelPartID(21, 0)` then `SetModelPartID(19, -1)` with nothing between
   them; the 20-root state in the middle does not exist to the renderer. Emit
   one state per `Wait` boundary. That alone took seven findings to one.
2. **A decomp parser must respect `#if defined(REGION_US)` / `REGION_JP`.** The
   last finding was `dLinkMainMotion_Catch`, whose five model-part writes are
   all adjacent between `WaitAsync(4)` and `WaitAsync(17)` — so its first
   presented state is the full 22-root vector, which the `Catch` program
   matches exactly. The phantom came from the `#if defined(REGION_JP)` arm
   below `End()`, which carries an extra `SetModelPartID(21, 0)` that a
   line-based parser absorbs into the US motion.

That second point is a **live defect in `check_hidden_part_root_coverage.py` as
committed**: it does no region filtering at all. It is GREEN today because none
of the three flagged masks has a JP arm that changes its vector, which is luck
rather than design — the JP arms hold ten model-part commands across Link and
Kirby. The build is `-DREGION_US`, so the checker must parse that arm. Fixing it
is owed.

## Registering a checker has a second wiring site, and it caught me

Adding the two `Invoke-VerifyScript` calls to `verify-all.ps1`'s host block made
the next Boundary **fail**:

```
Exception: verify-all.ps1:456
Verifier accounting mismatch: {0} passed, {1} expected. Refusing to
report 'Boundary verification profile passed.'
```

All three arms had run and passed — four-CPU WORK-H 1,600,960 / 2,320,576,
native failures 0, slips 0, identical to the run before. The failure was
entirely the accounting gate: `Invoke-VerifyScript` increments
`$script:verifiersPassed` on *every* success, host checks included, and
`$expectedVerifiers` is a literal that must move with them. Its own comment says
so — *"Keep this count synchronized with the unconditional Invoke-VerifyScript
calls before the runtime plan"* — and I added two calls without moving `15` to
`17`.

This is the same shape as the `SetRootProgram` bound that silently reset Kirby's
Stone and CopyLink to canonical, fixed earlier the same day: **a number that
validates a set of call sites is itself a call site.** The difference is the
failure direction, and it is the right one — the count being too high throws and
refuses to print the pass line, where the Kirby bound being too low silently
degraded output. A gate that fails loudly when you extend it is a gate doing its
job.

## The third mechanism, checked for completeness

> **CORRECTION, 2026-09-17.** The "seven times in the whole game" below is
> wrong, and the error is one of scope: it is seven *motion-command* sites. The
> underlying function `ftParamHideModelPartAll` has **three more callers in C**,
> which no sweep of `*MainMotion.c` can see — `ftcommonguard2.c:23` and
> `ftcommonguard1.c:391` (Yoshi's shield) and `efmanager.c:5441`
> (`efManagerYoshiEggEscapeMakeEffect`, the egg-hatching intro). All three hide
> Yoshi's whole body and put an `EFDesc` egg in its place, and **neither egg has
> a native owner**, so Yoshi goes invisible in both states. That is the owner's
> "character intro is invisible (egg hatching)" report, whose cause I had
> previously recorded as "elsewhere" after correctly ruling out the anim-desc
> mechanism and then stopping. The lesson is the general one: a mechanism reached
> through a motion command is usually also reachable from C, and sweeping the
> command tables alone understates it.

A root-count change has one more source: `ftMotionCommandHideModelPartAll`,
which collapses the live vector to whatever a following `SetModelPartID`
re-enables. It has seven **motion-command** sites and every one of those is
carried:

| file | motions | program |
|---|---|---|
| `228_KirbyMainMotion.c` | `0x1B3C`, `0x1B6C`, `StoneStartAir` ×2, `0x1B9C`, `StoneGround_0x1BB4` | Kirby Stone, one root `0x18A60` |
| `216_SamusMainMotion.c` | `dSamusMainMotion_0x0000` | Samus MorphUnfold, one root |

There is no gap, but there is a caveat worth recording before anyone writes a
checker for this one: **`dSamusMainMotion_0x0044` does not hide anything.** It
is `SetModelPartID(6, 2)` alone, and its one-root MorphBall vector is only
correct because `0x0000` already hid everything in an earlier motion. The hide
persists across the transition. A per-motion checker cannot see that and would
report MorphBall as a 14-root vector with no program — a false positive. Any
check of this mechanism has to model the hide's lifetime, which is why the
coverage check above deliberately stops at the anim-desc mask.

## A constant that does not match its source

`SAMUS_CATCH_HIDDENPART_IDS` is `range(3, 12)` while the Catch mask
`0x1FF80000` is indices 3..**12**. That is not a live defect — index 12 is joint
25, which the Catch motion hides with `(25, -1)`, so the vector is 21 roots
either way, and this sweep's mask-derived run reproduces exactly that. It is
still a constant that does not match its source, and moving to mask-derived IDs
removes it.
