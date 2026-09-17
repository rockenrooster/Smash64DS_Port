# Yoshi's grab family needed a root program, and the source says exactly which

Follow-up to `2026-09-17_p2-3f52-yoshi-throw-variant/`, whose own correction
section called this out: the model-part variant it baked (joint 7 = `0x7D10`) is
real and needed, but `ThrowF`/`ThrowB` also carry `0x18000000`, which installs a
**drawing** hidden part. A drawing hidden part *adds* a root, and
`nds_renderer_assets.c` rejects a variant on `owner->root_count != root_count`
before it ever compares offsets. So the variant alone could never make the throw
draw. This artifact implements the two complete root programs that can.

Owner reports covered: *"Yoshi: Grab attacks turn yoshi invisible"* and *"B
attack turns yoshi invisible and egg is also invisible"* — one bug, two symptoms.

## Every step re-derived from the payload, none assumed

The earlier artifact reasoned from the decomp C. This one re-read the same facts
out of the O2R payloads the generator actually consumes, and they agree.

**1. The Main payload is the right one.** `OWNER_ROOT_PROGRAM_SOURCES` stores
`(path, file_id, container_offset)`. Samus is `0x00d9` = 217 and its decomp file
is `217_SamusMain.c`; Link is `0x00e1` = 225 and `225_LinkMain.c`. Yoshi is
`247_YoshiMain.c`, so `0x00f7`, and the loader's own file-ID assertion accepts
it. `container_offset` is the C comment's own offset — `0x0124` for
`FTAttributes.modelparts_container`, matching Samus's `0x0288` convention.

`_verify_owner_modelpart_resolver` now carries a Yoshi falsifier too. The
container has exactly one non-NULL row, descriptor 3, and its model part 0 must
resolve to the display list the JointTree names: `0x2398` High, `0x5CF8` Low. It
does. A wrong Main/model pairing cannot pass that.

**2. `setup_parts` and the hidden-part table are complements.**
`dYoshiMain_setup_parts` is `0xFBFFFFE0`. Under the index ↔ bit `31-index` rule
its zero bits are exactly descriptors **5 and 27**, which is why the canonical
owner has 26 selected descriptors and 18 drawable roots.

The grab family's `0x18000000` sets bits 28 and 27, i.e. hidden-part IDs **3 and
4**, and `dYoshiMain_hiddenparts` rows 3 and 4 are

```
hiddenpart 3: root=31 parent=4  partindex=1 kind=0   -> descriptor 27
hiddenpart 4: root=9  parent=7  partindex=1 kind=0   -> descriptor 5
```

The two joints the mask installs are precisely the two joints `setup_parts`
omits. That is not a coincidence to be argued about; it is the mechanism.

**3. Only one of them draws.** Descriptor 27 is `{ 1, NULL }` in both JointTrees,
so it adds no root. Descriptor 5 is `&dYoshiModel_Joint_0x3148_post_post_post[8]`
in High and NULL in Low — and BattleShip's Low-detail fallback
(`_owner_raw_joint_descriptors`, mirroring `lbCommonSetupFighterPartsDObjs`)
selects the High list when Low is NULL. Both details therefore resolve to
**`0x2800`**, and the live vector goes **18 → 19**.

**4. The two live vectors, printed from the generator's own resolver:**

| program | roots | vector difference |
|---|---|---|
| Catch | 19 | canonical + `0x2800` inserted at ordinal 4 |
| Throw | 19 | the same, with ordinal 2 replaced by `0x7D10` |

Catch covers `Catch`, `CatchPull` and `EggLay` 202-206 — every motion carrying
`0x18000000` without a model-part command. Throw covers the `ThrowF`/`ThrowB`
window between `SetModelPartID(7, 1)` and the restore to `(7, 0)`, which are
`246_YoshiMainMotion.c`'s only four model-part commands.

The generator asserts that difference rather than assuming it: Catch must carry
no variant root and Throw must carry exactly the variant set, checked against
the emitted vector. A resolver change that collapsed the two into one program
would raise instead of silently shipping a duplicate.

## The check that was wrong for Yoshi, and why relaxing it cost nothing

`_assert_owner_root_program_vertex_cache` required every program root to read
only its own or the immediately previous root's vertex cache. Yoshi's first
program build failed it:

```
yoshi high program root 11 run 33: cache bindings [0, 11] escape current/previous [10, 11]
```

Running the same rule against Yoshi's **canonical 18-root vector** produced the
identical failure — two runs, both details:

```
high canonical 18: run 32/33 root[10]=0xae68 reads [0, 10] = ['0x2050', '0xae68']
low  canonical 18: run 22/23 root[10]=0xb0d0 reads [0, 10] = ['0x5a88', '0xb0d0']
```

So this is a property of Yoshi's shipped geometry, not of the new programs. The
hip root restores cache from root 0 ten bindings back, and the canonical owner
already carries that through its display-keyed cross-slot map. Adjacency was a
sufficient condition that Samus and Link happened to satisfy, not a necessary
one.

The rule is now **derived from the canonical vector** instead: a read between
two source roots that the canonical vector already performs is allowed; the
cross-slot packer still has to place it. The strict clause that matters is
untouched — a root in `new_offsets` must still be entirely self-contained, and
offsets absent from the canonical vector (every variant and appendix bake) get
no allowance at all.

**Proof the relaxation changed nothing else:** regenerating
`nds_native_fighter_owner.generated.inc` after the change altered **654 lines,
all of them Yoshi symbols**. Samus, Link, Kirby and every other owner are
byte-identical.

## What is in the ROM

- `P2_ROOT_PROGRAM_APPENDIX["yoshi"] = {(2, 0x2800)}` in both details — binding
  2 is joint 7's canonical binding, the parent the hidden joint hangs from.
  `0x7D10` needed no new bake; it is the variant the previous artifact added.
- `sNdsNativeYoshiCatchRoots[19]` / `…Low`, `sNdsNativeYoshiThrowRoots[19]` /
  `…Low`, their cross-palette slots and binding parents, and
  `NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT 1`.
- Runtime plumbing Yoshi had never had: owner runtimes, the
  `ndsRendererNativeFighterOwnerForProgramDetail` branch, the
  `SetRootProgram` bound (`program <= 2u`), `program_count = 3u` in the
  selector, and the binding-parent / cross-slot lookups in
  `nds_renderer_native_common.c`.

Program selection is automatic: `renderer_adapter_fighter.c:3582` hands the live
root offsets to `ndsRendererNativeFighterSelectRootProgram`, which matches them
against each program. No per-motion wiring exists or is needed.

Binding parents are `INVALID_U8` for all 19 roots, as for Samus Catch and Link
Catch: hidden joint 9 is inserted by `ftMainSetStatus`, not by the canonical
`setup_parts` walk, so there is no source parent schedule to publish, and
production receives each live DObj matrix directly.

## Checks

- `check_native_owner_geometry_closure.py` **GREEN**; Yoshi reads
  `18 canonical + 2 variant roots, 421 source triangles, 421 in tables` (High)
  and `302/302` (Low).
- `check_model_part_mutation_coverage.py` **GREEN**: 184 resolutions, 7
  fighters, 2 declared foreign.
- The generator's own per-program asserts all pass, including the vertex-cache
  closure and `_verify_program_roots_lit`.
- A full default build (`NDS_P2_SHELL_ROSTER ?= 7`, so `NDS_P2_YOSHI := 1`)
  compiles and links clean: `NATIVE_ONLY_PASS: smash64ds.elf, 262 actual link
  inputs`, zero errors.
- The linked ELF is the oracle, and all fourteen symbols are in it:

```
0212eca8 t sNdsNativeYoshiThrowLowOwner     0212ecc4 t sNdsNativeYoshiThrowHighOwner
0212ece0 t sNdsNativeYoshiCatchLowOwner     0212ecfc t sNdsNativeYoshiCatchHighOwner
0213be44 t sNdsNativeYoshiThrowRootsLow     0213c228 t sNdsNativeYoshiThrowRoots
0213bf88 t sNdsNativeYoshiCatchRootsLow     0213c380 t sNdsNativeYoshiCatchRoots
```

## Not proven

**That Yoshi draws during a grab, a throw or an egg lay.** Every link in the
static chain is now derived and checked, but nothing here demonstrates pixels,
and a correct root vector does not by itself prove the model is oriented right.

**Owed: captures of Yoshi's forward throw, back throw and Neutral-B egg lay.**

The egg-hatching intro remains **not this class** — `Appear1`/`Appear2` carry
only `0x40000000`, hidden index 1, TransN, no display list, so the root vector
does not change. Up-B egg shells are an effect, not a root. Both stay open under
their own causes.
