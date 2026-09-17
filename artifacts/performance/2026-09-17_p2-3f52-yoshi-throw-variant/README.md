# Yoshi's grab throw had no native root, so the whole fighter stopped drawing

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

## Still open in this row

The owner reports three Yoshi invisibility bugs and this addresses one:

| symptom | status |
|---|---|
| grab attacks turn Yoshi invisible | **addressed here** — `SetModelPartID(7, 1)` |
| B attack turns Yoshi invisible, egg invisible | **still open** — no model-part command for it, so it is hidden-part creation from the motion's anim-desc mask, the mechanism Samus Catch and Link Entry use |
| character intro invisible (egg hatching) | **still open** — same class |
| Up-B egg shells not rendering | **still open** — an effect, not a root |

Those three are genuinely `OWNER_ROOT_PROGRAMS` work and the board's original
framing is right for them.
