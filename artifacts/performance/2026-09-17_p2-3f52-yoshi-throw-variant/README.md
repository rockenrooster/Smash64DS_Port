# Yoshi's grab throw had no native root -- and that was only half of it

> **CORRECTION, read the section at the bottom.** The model-part root this
> document bakes is real and needed, but `ThrowF`/`ThrowB` ALSO install a
> drawing hidden part, which adds a root the vector cannot carry. A per-binding
> variant can never cover a root-count change, so the throw still does not draw.
> A Yoshi root program is required. The derivation below stands; the claim that
> it resolves the symptom does not.
>
> **RESOLVED by `2026-09-17_p2-3f52-yoshi-root-programs/`**, which implements the
> two programs this correction specifies. The variant baked here is still needed
> — the Throw program references it rather than baking `0x7D10` twice.

Owner, `docs/BUGS.md`: *"Yoshi: Grab attacks turn yoshi invisible."*

Same failure class as Kirby's copy, and found by looking for it after that one:
a move mutates the live root vector, one root in the new vector has no native
bake, the owner declines, and at `NDS_RENDERER_PROFILE_LEVEL 0` a declined owner
draws **nothing** — so the whole fighter vanishes for the length of the move.

## The derivation, end to end from source

`P2_MODEL_PART_ROOT_VARIANTS` had **no yoshi entry at all**, so every Yoshi
model-part mutation was unbaked.

1. **Which mutations exist.** `246_YoshiMainMotion.c` contains exactly four
   model-part commands, and they are two pairs:
   `dYoshiMainMotion_ThrowF` at :965 `SetModelPartID(7, 1)` and :975
   `SetModelPartID(7, 0)`; `dYoshiMainMotion_ThrowB` at :988 and :998, the same
   pair. **Those are Yoshi's only model-part mutations anywhere in the file** —
   grep for `SetModelPartID|HideModelPart|ShowModelPart` returns those four
   lines and nothing else. So Yoshi needs exactly one variant, not a family.

2. **Which joint.** `dYoshiMain_modelparts_container[28]`
   (`247_YoshiMain.c:137`) has a **single** non-NULL entry, at index **3**.
   Container index + 4 is the source joint — the convention already recorded for
   Kirby — so index 3 is **joint 7**, exactly the joint the motion names.

3. **Which display list.** That entry is
   `dYoshiMain_modelparts_desc_0x0D4[4]`, and `FTModelPartDesc` is
   `modelparts[part][detail]`, so four rows are two parts × two details:

   | row | part | detail | display list | offset |
   |---|---|---|---|---|
   | 0 | 0 | high | `dYoshiModel_Joint_0x2398_DisplayList` | `0x2398` |
   | 1 | 0 | low | `dYoshiModel_Joint_0x5CF8_DisplayList` | `0x5CF8` |
   | 2 | **1** | high | `dYoshiModel_gap_0x6E70_sub_0xEA0` | **`0x7D10`** |
   | 3 | **1** | low | `dYoshiModel_gap_0x6E70_sub_0xEA0` | **`0x7D10`** |

   Both detail rows for model part 1 name the **same** display list, so the
   variant is one offset for both.

4. **Which canonical binding it replaces.** Not guessed from the topology
   table — measured. Building the Yoshi owner context gives 18 canonical roots
   in each detail, and `0x2398` (high) and `0x5CF8` (low) are both at
   **binding 2**. That is the row the throw replaces, and it confirms the
   joint-7 mapping independently of the container arithmetic.

## The fix

One row in `P2_MODEL_PART_ROOT_VARIANTS`, `(2, 0x7D10)` in both details, plus
the matching owner branch in the runtime's variant resolver — Yoshi had none,
because it had never had a variant to resolve.

Only one root changes and the rest of the tree stays canonical, so this is a
**per-binding variant, not a root program**. That is why the board's note
("derive root programs from `247_YoshiMain.c` like Link Catch") overstated the
work: Link's Catch rewrites five model parts and adds hidden parts, Yoshi's
throw swaps one.

## What is proven, and what is not

Baked and reachable:

- The generator emits
  `sNdsNativeYoshiRootVariants[1] = { 2u, { 0x00007d10u, 34u, 65535u, 128u, 7u, 0u, 0u, 1u } }`
  — 128 source commands, matching the 128-command display list, and 7 epochs.
- Both tables link into the canonical ROM at 20 bytes each
  (`0213be30` and `0213bce8`), so the resolver can reach them.
- `check_native_owner_geometry_closure.py` is GREEN.
- The four-CPU stress gate is unaffected and cannot be: that build has
  `NDS_P2_YOSHI 0`, since its roster is Donkey/Samus/Link/Kirby.

**Not proven: that the throw now draws.** This is a visual claim about a
gameplay state that needs Yoshi to actually grab and throw someone, and nothing
here demonstrates pixels. The static chain says the root will resolve where it
previously could not; it does not say the model is the right one or that it is
oriented correctly. **Owed: a capture of Yoshi's forward and back throws.**

## CORRECTION: this fix is necessary but NOT sufficient

The throw does not draw yet, and the reason is a second mechanism this document
did not check.

`ThrowF` and `ThrowB` are not only model-part swaps. In `ftdata.c`'s Yoshi
motion table both rows read

```c
{ &llFTYoshiAnimThrowFFileID, dYoshiMainMotion_ThrowF,
  FTANIM_FLAG_ANIMLOCKS | 0x18000000 },
```

— the same `0x18000000` that `Catch` and `CatchPull` carry. Those bits install
**hidden parts 3 and 4**, and Yoshi's hidden part 4 is
`{ root 9, parent 7, partindex 1, kind 0 }` (`247_YoshiMain.c:125`), whose joint
**draws**: descriptor 5 of the high `dYoshiModel_JointTree` is
`&dYoshiModel_Joint_0x3148_post_post_post[8]`, resolving to
`dYoshiModel_Joint_0x2800_DisplayList` at **`0x2800`**. The low tree's entry at
that index is NULL, so `ftMainUpdateHiddenPartID` falls back to the high
commonpart and the joint draws at **both** details.

**A drawing hidden part ADDS a root**, so the live vector goes from 18 to 19.
`P2_MODEL_PART_ROOT_VARIANTS` replaces a root at an existing binding and can
never cover that: `nds_renderer_assets.c` rejects on
`owner->root_count != root_count` before it ever compares offsets. So
`(2, 0x7D10)` is real and needed — the throw genuinely does swap joint 7 to part
1 — but by itself it cannot make the throw draw.

**What is actually required** is a complete ordered root program for Yoshi, and
two of them:

1. joint 7 canonical + hidden part 4 — `Catch`, `CatchPull`, and the five
   `EggLay*` motions (202-206), which carry the same `0x18000000`.
2. joint 7 = `0x7D10` + hidden part 4 — the `ThrowF`/`ThrowB` window between
   `SetModelPartID(7, 1)` and the restore to `(7, 0)`.

`0x7D10` is already resident, because the owner's roots are canonical plus
variants plus appendix, so a program can reference the variant this change
added rather than baking it twice. `0x2800` needs one new appendix row, the
same offset in both details. The rest is plumbing Yoshi has never had: an owner
branch in `build_owner_root_programs`, a `program_count` for
`NDS_RENDERER_NATIVE_FIGHTER_OWNER_YOSHI`, and an
`NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT` gate.

**This also explains the owner's other two Yoshi reports.** "B attack turns
yoshi invisible and egg is also invisible" is the same hidden part reached
through the `EggLay*` motions — the same program fixes both. The
egg-hatching intro is **not** this class, and the guess in the earlier revision
of this document that it was is withdrawn: `Appear1`/`Appear2` carry only
`0x40000000`, which is hidden index 1, TransN, no display list, so the root
vector is unchanged.

The artifact's original "Not proven: that the throw draws" was right, and this
is why.

## Still open in this row

The owner reports three Yoshi invisibility bugs and this addresses one:

| symptom | status |
|---|---|
| grab attacks turn Yoshi invisible | **half done** — the model-part root is baked here; the hidden part it also installs still needs a root program |
| B attack turns Yoshi invisible, egg invisible | **open, cause confirmed** — the same drawing hidden part 4, via `EggLay*` 202-206. One Yoshi program fixes this and the grab together |
| character intro invisible (egg hatching) | **open, NOT this class** — `Appear1`/`Appear2` carry only `0x40000000`, hidden index 1, no display list. Cause is elsewhere |
| Up-B egg shells not rendering | **open** — an effect, not a root |

Those three are genuinely `OWNER_ROOT_PROGRAMS` work and the board's original
framing is right for them.
