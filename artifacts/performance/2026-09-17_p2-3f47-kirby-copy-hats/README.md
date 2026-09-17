# Kirby's ten broken copy victims: the geometry works, the bytes do not

`2026-09-17_p2-2p8-roster-variance/KIRBY_COPY_NATIVE_GAP.md` established the
defect: Kirby's swallow-copy left the native path for ten of the eleven
copyable victims, and at `NDS_RENDERER_PROFILE_LEVEL 0` a rejected root never
reaches the screen, so the whole fighter vanished for the duration of the copy.
Commits `ddf18a57a86` (the fix) and the gating that follows it.

> **Read the status section at the bottom first.** Every hat bakes, the closure
> cross-product goes green and the ROM links — and admitting all ten costs
> **+70,016 WORK-H P50**, drops heap low-water from 111,680 to **73,064** and
> produces **151 native-render failures** whose witness is Kirby's *Stone*. The
> sections are appended to Kirby's resident image when they belong in the
> per-slot hat images. Admission is `False` until they move. The analysis below
> stands; only the shipping decision changed.

## What actually blocked it

Both obstacles are consequences of one fact: **a copy hat is a deferred image,
and the trio seam was written for a resident head.**

### 1. The body's MODIFY_ST colour escapes

The hidden trio body (joint 7) copies the shade of rows the head already
submitted. `_append_kirby_trio_sections` resolved each escape to a
value-identical row in the **resident** dense table. For a face head (modelparts
1 and 14, Kirby's own inhale and boomerang faces) such a row exists. For a
deferred hat it cannot: the hat's rows are in the hat image.

Measured, both details, by building the faithful program for each head and
classifying every escape:

| head | body dense rows | escapes leaving the block | resolved in the resident table | unresolved |
|---|---:|---:|---:|---:|
| 1 (face) | 46 | 14 | 14 | **0** |
| 14 (face) | 46 | 14 | 14 | **0** |
| 4 (Donkey's hat) | 46 | 14 | 0 | **14** |

The interesting part is *how* the failing rows differ from the rows that escape
to them. Comparing all eight fields of the escaping row against its source:

| head | differing fields |
|---|---|
| 1 | `{}` — byte-identical in all eight, 14 of 14 |
| 14 | `{}` — byte-identical in all eight, 14 of 14 |
| 4 | `{s,t}` x10, `{t}` x2, `{s}` x2 |

So a face head's "escape" is a plain duplicate row, and a **copy hat's escape is
a literal MODIFY_ST**: the body re-submits the same vertex at a different
texcoord. Position, matrix binding, cache slot and the colour/normal word are
identical in every case.

Those rows therefore shade from themselves. The resolver keeps the resident-twin
path first (so the two face heads' bytes are unchanged), and falls back to self
only when the source agrees on every shading input and differs only in `s`/`t`.
Anything else still raises.

**This is exact in the shipped configuration, because the alias has no reader
there.** Under `NDS_R2_FIGHTER_HW_LIGHT` the active tables never bind the
pointer at all —

```c
/* src/nds/nds_renderer_assets.c */
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
#define NDS_IMG_BIND_COLOR(tables_, img_)                                      \
    (tables_).dense_color_source = (img_)->dense_color_source;
#else
#define NDS_IMG_BIND_COLOR(tables_, img_)
#endif
```

— and `builds/build/nds_build_config.h` has `NDS_R2_FIGHTER_HW_LIGHT 1` with
`NDS_RENDERER_M2_DETAILED_LEDGER 0`. The software shade loop that would
dereference it (`nds_renderer_native_common.c`, the `#else` arm) is not
compiled, and `prepared_dense` drops `shaded_rgba` entirely. The GX lights every
vertex from its own word — which *is* self-shading, and is already what heads 1
and 14 do on hardware today.

Under a software-lit build the body epoch's own light state would replace the
head epoch's inherited shade for these rows. That is the one bounded difference;
it is counted per head as `self_shaded_escapes` (14 high / 12 low per hat, 0 for
both faces) rather than hidden.

### 2. The program's root vector

Root 0 of a trio program is the head, and `build_owner_root_programs` required a
**resident appendix bake** for every root:

```
ValueError: kirby high head3: root 0x52e8 lacks a resident appendix bake
```

A deferred hat has no resident root. That makes a copy hat's trio program
**mixed-file, exactly like CopyLink's** — which already had this machinery. Each
hat program now carries a `SourceOwners` table, and root 0 resolves into the
per-slot hat image through the same two resolves CopyLink uses
(`...TablesForResolvedRoot` and `...LightPreamblesForResolvedRoot`).

The hat images were never the missing piece: `ftParamSetModelPartDefaultID`
(`reloc_backend_compat_shims.c`) already calls
`ndsRendererNativeEnsureKirbyCopyHat` for **every** modelpart 3..13, both
details. Ten hats were being loaded into the arena every copy and had nowhere to
draw from.

## The latent bug found on the way

`build_owner_root_programs` held a **second** copy of the root-count rule:

```python
expected_count = 9 if head_mp == 1 else 10
```

Correct only while the table held exactly heads 1 and 14. Every copy hat has
nine roots (only Link-copy enables joint 18 and reaches ten), so this would have
rejected the first hat admitted, with a message about root cardinality rather
than about the head. The first copy of this rule became
`kirby_trio_root_count()` in `6da2b34e300`; this one was missed because nothing
connects them textually.

## The runtime is no longer hand-written per head

The seam shipped supporting two heads with the accept set written as three bare
integer literals in `renderer_adapter_fighter.c`, two owner pairs written out
longhand in `nds_renderer_assets.c`, and program numbers 1/2/3/4 hard-coded in
two places. Widening that by hand ten times is how the next head gets
half-wired.

The generator now emits one list:

```c
#define NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(X_) \
    X_(1) X_(14) X_(3) X_(4) X_(5) X_(6) X_(7) X_(8) X_(9) X_(11) X_(12) X_(13)

#define NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT 12u
```

and the owner pairs, the program-to-owner table, the per-program head guard and
the adapter's admission predicate (`ndsRendererNativeKirbyTrioHeadSupported`)
are all expanded from it. Head 10 stays separate because CopyLink is not a trio
context.

The head guard matters more than it used to: twelve programs now share one root
cardinality, so "an identical-looking vector under the wrong live head" is no
longer theoretical. It is keyed per program from the same list.

## Checks

`check_native_owner_geometry_closure.py` now parses the **emitted macro** rather
than the adapter's literals. Reading `KIRBY_TRIO_CONTEXTS` for both sides would
have compared the generator against itself; this reads the artifact the compiler
sees, and fails closed if it is missing.

Proven to fail closed, not merely to print OK:

| perturbation | result |
|---|---|
| drop `X_(4)` from the generated list | RED — "the C accept set [1,3,5,...] has drifted from the baked contexts [1,3,4,5,...]" |
| adapter goes back to its own literal set | RED — "no longer asks ndsRendererNativeKirbyTrioHeadSupported ... update the check rather than deleting it" |
| restored | GREEN — "12 victims, 0 without a baked hat context" |

## Cost

Kirby's owner image grows. Every other owner image is byte-identical.

| image | before | after | delta |
|---|---:|---:|---:|
| `NDSNativeKirbyHighImage` | 40,133 | 68,981 | **+28,848** |
| `NDSNativeKirbyLowImage` | 31,819 | 57,923 | **+26,104** |

These are deferred images, not ARM9 resident bytes. The largest single
contributor is state: `state_deltas` goes 134 -> 587 (high) because the appended
section copies the whole faithful state table per head. That is a deliberate
simplification in the existing seam, not new; deduplicating it across heads is
the obvious follow-up if the arena needs the bytes back.

## Linked-ELF verification

`builds/build-p2-fourcpu-tickhud/smash64ds-p2-fourcpu-tickhud-hwtri.elf`
(`NDS_P2_KIRBY 1`) contains all twelve `sNdsNativeKirbyTrioHead*Roots` and
`*CrossPaletteSlots`, exactly ten `*SourceOwners` (none for faces 1 and 14), the
four dispatch arrays and `ndsRendererNativeKirbyTrioHeadSupported`. The default
`smash64ds.nds` shell build has `NDS_P2_KIRBY 0` and contains none of them,
which is why the ELF check was done on the stress target.

## Status: implemented, GATED OFF, blocked on bytes

`KIRBY_TRIO_ADMIT_COPY_HATS = False`. Everything above is correct and stays in
the tree; what does not fit is the **arena**.

### The measurement that decided it

Same target, same build directory, same roster, 1,972 samples each. The only
difference is the constant.

| | hats OFF | hats ON | delta |
|---|---:|---:|---:|
| gate verdict | **PASS** | **FAIL** | |
| `gNdsRendererNativeFailure.count` | **0** | **151** | +151 |
| failure witness `root` | — | **0x18A60** | Kirby **Stone** |
| `gNdsTaskmanGeneralHeapFreeMin` | **111,680** | **73,064** | **−38,616** |
| `gNdsTaskmanArenaChosenSize` | 1,355,520 | 1,351,424 | −4,096 |
| WORK-H P50 | **1,580,416** | 1,650,432 | **+70,016** |
| WORK-H P95 | 2,320,768 | 2,379,264 | +58,496 |
| STG P50 | 337,472 | 405,568 | +68,096 |
| ALL P50 | 1,678,016 | 2,237,696 | +559,680 |
| P0 / P1 hardware triangles | 349,031 / 333,618 | **identical** | **0** |

Hats OFF reproduces the banked baseline exactly — heap low-water 111,680 and
arena 1,355,520 match the board figure to the byte, native is 0/0, and WORK-H
1,580,416 sits 8,128 from the 1,588,544 baseline, inside the 14,080 cross-build
significance floor.

**The triangle counts are identical in both arms.** Nothing new is drawn. The
+70,016 is not the cost of rendering ten copy hats; it is the cost of carrying
their tables. ALL crossing from 1,678,016 to 2,237,696 is the VBlank quantum
moving from three intervals to four — a cadence collapse, not a gradual cost.

The failure witness is the giveaway: **Kirby's Stone**, `root 0x18A60`, which
this change never touches. A copy hat failing would have implicated a hat root.
Stone failing implicates the resource the whole owner shares.

(`gNdsTaskmanArenaAllocFailCount` is 83 in the passing arm and 84 in the
failing one, so that counter is **not** the mechanism — it was already nonzero.
The heap low-water and the arena size are.)

### Why the fix is to move, not to shrink

The body sections are appended to **Kirby's** resident image, so all twelve ride
in memory whenever Kirby plays, though at most one head is ever live. They
belong in the per-slot **hat image**, which `ftParamSetModelPartDefaultID`
already loads on demand for exactly the copy that needs it — that is what the
deferred design is for.

Sharing the common arrays instead is not enough. Measured across all twelve
sections, `triangles`, `packed_corners`, `run_unique_dense` and
`action_dense_spans` are **byte-identical** after rebasing, and only
`dense_vertices` (14 of 46 rows, high; 12 of 38, low), `state`, `sequence` and
`dense_color_sources` differ. The identical arrays are about **29%** of the
per-head bytes; state alone is another ~20% and cannot be shared, because the
per-head state tables differ in length (31 to 60 rows). Deduplication leaves
roughly +20 KB on the high image, which the arm above shows is still too much.

### What is kept

The self-shade resolver, the mixed-file hat programs, the generated
`NDS_NATIVE_KIRBY_TRIO_HEAD_LIST` and everything expanded from it, the second
root-count fix, and the closure check that now parses the emitted macro and is
proven to fail closed two ways. Flipping the constant to `True` with no other
change reproduces the RED above; the closure check returns to naming the ten
victims, which is its designed state until the sections move.

## The fix is validated, and it removes the compromise rather than adding one

The blocker above says the body sections belong in the per-slot hat image
instead of Kirby's resident one. That was an architectural argument about bytes.
It is now also a **correctness** argument, and it is measured.

For each head, the faithful program's body block was built and every colour
escape traced to the dense range of **root 0 — the head itself**:

| head | body dense | head's own dense | escapes | resolving inside the head |
|---|---|---|---:|---:|
| 4 (Donkey's hat) | [120, 166) | [0, 120) | 14 | **14** |
| 8 (Samus's hat) | [138, 184) | [0, 138) | 14 | **14** |
| 9 (Captain's hat) | [132, 178) | [0, 132) | 14 | **14** |
| 14 (boomerang face) | [118, 164) | [0, 108) | 14 | **14** |
| 1 (inhale face) | [70, 116) | [0, 29) | 14 | 0 — reaches canonical rows |

**Every copy hat's escapes resolve entirely within its own head's rows.** So a
body section carried in the hat image has its colour sources right beside it:
no cross-image reference, no value-identical-twin search against a resident
table, and **no self-shade fallback at all**.

That matters beyond the bytes. The shipped resolver's fallback is exact only
because the hardware-lit configuration never binds `dense_color_source`; under a
software-lit build it substitutes the body epoch's light state for the head's.
Moving the sections into the hat image **removes that compromise instead of
carrying it** — the escape resolves to the row it actually means, in every
configuration.

Head 1 is the exception and does not matter: it is a resident face whose escapes
reach canonical rows, and both faces stay exactly where they are today at zero
growth.

### What the change therefore is

- Faces 1 and 14: unchanged, resident, byte-identical to today.
- Copy hats: body section appended to the **hat** context in
  `build_p2_kirby_hat_runtime_context` rather than to the kirby context, with
  its indices rebased into the hat's local space (`_rebase_dense_word` already
  enforces that a hat word may not reference a resident dense id, so it fails
  closed if the rebasing is wrong).
- The trio program's **body** root joins its head at source owner `kirby_hat`;
  the canonical roots stay `kirby`. The mixed-file machinery that already
  carries root 0 carries root 1 unchanged.
- Kirby's resident image returns to **zero growth**; each hat image grows by one
  body section (~2.9 KB high) and is loaded only for the copy that needs it.
- `KIRBY_TRIO_ADMIT_COPY_HATS` flips to `True` and the self-shade fallback
  becomes dead code for hats — keep it only if a face head ever needs it.

Not started. Recorded because both load-bearing unknowns — whether the escapes
survive the move, and whether they can be named in the hat's index space — are
now answered, and answered in the direction that makes the work worth doing.

### The row mapping resolves too, for all ten hats

One unknown remained after the escape check: the faithful trio program and the
hat image order their rows **differently**. In the faithful program the head is
root 0 and its rows come first; in the hat image the rows are the suffix cut
from a canonical+one-hat build. So "the escape is inside the head" does not by
itself mean the escape can be *named* in the hat image's local index space.

Resolved by value: build the hat context, key every one of its dense rows on the
full eight-field tuple, and look up each escape's source row from the faithful
program.

| detail | hats | escapes per hat | mapped into the hat image |
|---|---|---:|---:|
| high | 3, 4, 5, 6, 7, 8, 9, 11, 12, 13 | 14 | **14 — all ten** |
| low | same ten | 12 | **12 — all ten** |

Hat dense counts range 87–138 (high) and 73–118 (low); every escape found its
row in every one. **The mapping is total**, so the body section can be appended
to the hat context with its colour sources rewritten into hat-local indices, and
nothing needs a cross-image reference or a fallback.

With this and the escape-range result above, the remaining work is mechanical:

1. `build_p2_kirby_hat_runtime_context` gains a step after the suffix cut —
   append the faithful body block, remapping its dense/corner/unique/span
   indices into the hat's local space and its colour escapes through the value
   map above. `_rebase_dense_word` already refuses a resident reference, so a
   mis-rebase fails closed rather than shipping a wrong index.
2. The trio program's **body** root moves to source owner `kirby_hat` beside its
   head; the canonical roots stay `kirby`. The runtime resolves both through the
   `SourceOwners` table that already carries root 0.
3. `KIRBY_TRIO_ADMIT_COPY_HATS` → `True`; Kirby's resident image returns to zero
   growth; the self-shade fallback becomes dead for hats.

Owed after that: the four-CPU gate, and a visual check per hat — the seam still
runs on the `validate_cross_census=False` path, where a wrong cross sequence of
the right length passes silently, so only pixels prove the body resolved against
its own head.

## Not caused by this work

`check_native_owner_weld_consistency.py` (IndexError at line 101) and
`check_nds_native_owner_hierarchy.py` ("mario: retained packet corner trace
mismatch") are RED. Both were re-run with the generator restored from HEAD and
are RED there too. Neither is referenced by the Makefile or by any
`scripts/*.ps1`, which is how they rotted unnoticed.
