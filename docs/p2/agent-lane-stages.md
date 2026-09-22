# Stage lane: three owner rows diagnosed (2026-09-22)

Read-only diagnosis of the three STAGE rows the owner re-reported against build
`r36` (`builds/remaining-bugs-playtest-r36/`). No file under `src/`, `include/`,
`scripts/` or `assets/` was modified; no build and no emulator was run. Every
runtime figure below is either a byte read out of the r36 ROM/ELF artefacts on
disk, or a value produced by running the stage generator in-process (a pure
function that writes nothing).

---

## Corrections to the record, before the rows

Three recorded premises are wrong and each cost cycles. They are corrected here
because the repo's own history says stale premises are the main defect source.

1. **`NDS_TASK51_STAGE_NATIVE` is 0 in the shipped configuration, so the Saffron
   gate fix `a6c548f40de` changed nothing that the ROM executes.** See row S04.
   `docs/p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-21.md:610-618` states the
   Task 51 baked-constant mechanism as the found cause. That mechanism is
   compiled out.
2. **"Castle roof / Inishie platforms untextured: most likely `use_texture` false
   via `NO_TEXEL0`"** (`docs/p2/BUG_NOTES.md:4383-4385`) is **refuted**.
   `ndsRendererHardwareUseTexture` reaches `NO_TEXEL0` only after
   `ndsRendererCombineUsesColor`, which tests all eight colour slots
   (`src/nds/nds_renderer_textures_effects.c:528-538`), and every Castle roof
   policy puts `TEXEL0` in colour slot `a0`. `use_texture` is TRUE on every roof
   run. See row S01.
3. **"The acid subdivision does not fit the vertex format"**
   (`docs/p2/BUG_NOTES.md:4151-4180`) is correct about the arithmetic and wrong
   about the conclusion it invites. The midpoints are half-integers only in the
   *current* coordinate basis; the dense vertex already carries a per-vertex
   coordinate shift, and the acid's extent leaves a full bit of headroom in
   `s16`. See row S03.

---

## Row S04 — Saffron City

```
Bug: "the pokemon garage door hazard is always open. It should close and open
      periodically."  (docs/BUGS.md, Stages section, **NOT FIXED in R36**)
```

### Contract

The gate is one `nGCCommonKindGround` GObj built by `grYamabukiMakeGate`
(`decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c:246-264`) over the
five-entry `DObjDesc` tree at file-160 offset `0x08A0`
(`decomp/BattleShip-main/decomp/src/relocData/160_StageYamabukiFile4.c:94-101`).

Authored poses, read off that table:

| DObj | authored translate | moved channel |
| --- | --- | --- |
| 1 | (1026.3673, 387.0627, **0.0**) | TraZ |
| 2 | (1026.3673, **390.0**, 0.0) | TraY |
| 3 | (1026.3673, 387.0627, **0.0**) | TraZ |
| 4 | (810.0, 429.8401, 0.0) | flag toggle only |

Two AObj joint banks drive them
(`160_StageYamabukiFile4.c:106-112` open table `0x09B0`,
`:148-156` close table `0x0A20`):

* OPEN (`:114-139`) — DObj1 TraZ `0 -> 330`, DObj2 TraY `390 -> -30`,
  DObj3 TraZ `0 -> -330`, each over a 10-tick block; DObj4 sets flags `0x001`
  for 4 ticks then `0x000` for 6.
* CLOSE (`:158-190`) — DObj1 TraZ `330 -> 0`, DObj2 TraY `-30 -> 390`,
  DObj3 TraZ `-330 -> 0`; DObj4 waits 7 then sets flags `0x001` for 3.

So, exactly:

* **Closed pose = the authored `DObjDesc` pose** (z 0 / y 390 / z 0).
* **Open pose = (z +330 / y −30 / z −330).**
* **Block 0 of the CLOSE script is the OPEN extreme.** The close script's first
  command is `aobjEvent32SetValRateBlock(...,0)` carrying 330 / −30 / −330, so
  evaluating the close animation at frame 0.0 and never advancing it leaves the
  gate **open**. Both the previous session ("authored pose is closed") and the
  coordinator ("frame 0 of close is open") are describing true and compatible
  facts about different moments.
* `DOBJ_FLAG_NOTEXTURE` is `1 << 0` (`decomp/.../src/sys/objtypes.h:35`) and
  `objdisplay.c:1714` skips a DObj's whole `dl_link` array when it is set, so
  DObj4 (the graded translucent doorway shell, root `0x0850`, 10 triangles) is
  drawn only while the doorway is open.

State machine (`gryamabuki.c:226-245`): Sleep -> Wait on battle start
(`:46-52`, `monster_wait = rand(1000)+1000`); `gate_wait` starts at 1
(`:274`) so the **first** Wait tick opens the gate (`:145-158`); a monster is
made on a near fighter or on `monster_wait` expiry (`:74-102`); each of the five
ground monsters calls `grYamabukiGateSetClosedWait()` when its own animation
ends (e.g. `decomp/.../src/it/itground/itporygon.c:88-94`), which sets
`gate_wait = 1000` and plays the close animation (`gryamabuki.c:208-217`);
`grYamabukiGateUpdateOpen` (`:175-197`) also closes when `monster_gobj` is NULL.
**Expected observable: closed for ~1000 ticks, then open, cyclically.**

### Divergence

**First measured divergence: the repair that was supposed to fix this row is not
compiled into the ROM the owner played.**

* `src/nds/nds_native_stage_select.inc:1490` and the blob header both carry the
  widened mask. I extracted the Yamabuki stage blob from
  `builds/remaining-bugs-playtest-r36/smash64ds.nds` (header `NSB1` at file
  offset `0x3EE8800`, identified by `asset=3 segment=4 dobj=24 binding=21`) and
  parsed its header with `_BLOB_HEADER_STRUCT`
  (`scripts/stages/generate_nds_native_stage.py:4703`):
  `gkind=7 rigid_binding_mask=0x0 camera_binding_mask=0xE4894`. The mask shipped.
* **It has no reader.** The only consumer of `camera_binding_mask` in C is
  `src/nds/nds_renderer_native_owners.c:3643`, and line **3637** opens
  `#if NDS_TASK51_STAGE_NATIVE`. `ndsRendererNativeStageTask51EnsureWorld` itself
  (`:2583-2639`) is inside the same `#if` (`:2568`).
* `builds/remaining-bugs-playtest-r36/nds_build_config.h:20` reads
  `#define NDS_TASK51_STAGE_NATIVE 0`, and `Makefile:232` makes 0 the default.

So the gate's bindings 17-19 were **never** on the baked-constant path in any
default build. Widening `0x4894 -> 0xE4894` is a no-op there, which is exactly
why r36 is indistinguishable from r35 for this row.

**What the renderer actually does with the gate, in the shipped config:**
Yamabuki's runtime rigid mask is 0, so `ndsRendererAdapterCaptureTask36StageWorld`
puts all 21 bindings in the *dynamic* list
(`src/port/renderer_adapter_stage.c:3331-3348`), and each is recomposed every
frame from the live DObj through
`ndsRendererAdapterBuildPersistentStageWorldMatrix`
(`:3222-3257`). That function's reuse key includes the DObj's own
`translate.vec.f` (`src/port/renderer_adapter_matrix.c:4609-4610`), so an
animated translate invalidates the cache and forces a rebuild. **The gate model
follows the live DObj translate. It always has.**

Therefore the divergence is upstream of the renderer: the gate's DObjs are
sitting at the open pose. Two candidates remain, and they are separable by one
read:

* **(a) the state machine never leaves `nGRYamabukiGateStatusOpen`.** The only
  exits are a monster whose own animation ends (`itporygon.c:88-94` and the four
  siblings) or `monster_gobj == NULL` (`gryamabuki.c:177-179`). All five
  Pokemon TUs now exist in the port and all five call
  `grYamabukiGateSetClosedWait()`, so the stale note in
  `src/import/battleship_gryamabuki_ground.c:14-20` ("the spawner's call returns
  NULL ... so the gate opens, finds nothing to follow, and closes") no longer
  describes the build. If a spawned monster's AObj never reaches its `End`
  event, `monster_gobj` stays live forever and the gate never closes.
* **(b) the gate's own joint animation is installed but not advanced**, pinning
  it at block 0 of the close script — the open pose. `grYamabukiGateAddAnimOffset`
  (`gryamabuki.c:119-123`) installs and plays **one** tick itself; the per-frame
  advance is the separate `gcAddGObjProcess(gate_gobj, gcPlayAnimAll,
  nGCProcessKindFunc, 5)` at `:263`.

Evidence bearing on (b): the port's `gcPlayAnimAll`
(`src/import/battleship_sys_objanim.c:2430-2464`) runs
`ndsGcPlayAnimAllStableSkip` (`:2396-2427`), whose skip is **MObj-material only**
— it calls `gcParseDObjAnimJoint` + `gcPlayDObjAnimJoint` for every DObj
unconditionally, so it cannot freeze a joint track. Processes run off one global
priority list (`decomp/.../src/sys/objman.c:2150-2166`), and
`grYamabukiGateProcUpdate` is a Func process of the same kind on the same link
(`gryamabuki.c:294`); a probe already observed the gate *state* cycling, so that
list is running. That makes (b) less likely than (a) but does not kill it: the
port's `gcAddAnimJointAll` wrapper (`battleship_sys_objanim.c:2590-2606`) is
**fail-silent** — if `ndsAObjEvent32NormalizeDObjTable` returns FALSE it simply
never calls the base installer, and nothing counts the refusal at a per-call
granularity.

The offset arithmetic the coordinator asked me to confirm is **correct**.
`src/import/battleship_gryamabuki_ground.c:53-57` defines the `ll*` symbols as
`(*(uintptr_t *)(uintptr_t)(offset))`, so `&llGRYamabukiMapGateOpenAnimJoint`
evaluates to the literal `0x9b0`, which is what `gryamabuki.c:121` adds to
`map_head`. `map_head` is `gMPCollisionGroundData->map_nodes - 0x8a0`
(`gryamabuki.c:270`), and file-160's tables sit at `0x08A0 / 0x09B0 / 0x0A20`
(`160_StageYamabukiFile4.c:94,106,148`), so `map_head + 0x9b0` lands exactly on
the open table. This is not an instance of the `ll`-offset class.

### Root cause

Recorded cause (Task 51 baked world matrix) is **refuted for every default
build**: the code that would consume the fix is `#if`-ed out. The live cause is
that the gate's DObj translate is pinned at the open pose, and the surviving
mechanisms are (a) a stuck `Open` state, most plausibly a monster GObj that never
retires, or (b) a joint animation that was installed and evaluated once but is
not being advanced.

### Proposed fix

**No renderer edit is proposed, and the existing one should not be built on.**
The minimal, honest change to make now is to stop the no-op from recurring and
to keep the derivation that is actually correct:

`src/nds/nds_native_stage_select.inc:1483-1490` — keep `0xE4894` (it is right,
and `blob_live_world_mask` derives it from the real payload), but the comment
must stop claiming a live consumer:

```c
     * a runtime joint animation moves. Both halves mean "compose this
     * binding's world live"; the only reader is the Task 51 baked-world
-     * opt-out. 0xE0000 is Saffron's gate (S04): grYamabukiGateAddAnimOpen/
+     * opt-out, which is compiled only when NDS_TASK51_STAGE_NATIVE != 0
+     * (Makefile:232 defaults it to 0, so the default ROM has no reader and
+     * every non-rigid binding already composes live).
+     * 0xE0000 is Saffron's gate (S04): grYamabukiGateAddAnimOpen/
```

The behavioural repair cannot be specified until the observation below
separates (a) from (b); proposing one now would be the fourth speculative
Saffron patch in this row's history.

### Confidence

**High** on the refutation (Task 51 compiled out; blob parsed out of the shipped
ROM; single guarded consumer). **Low** on which of (a)/(b) is the live cause.

Falsifier for the refutation: a build config in the owner's delivery path with
`NDS_TASK51_STAGE_NATIVE=1`. `builds/remaining-bugs-playtest-r36/nds_build_config.h:20`
says otherwise for the ROM actually played.

### Open question

One gdb read on a Saffron match decides it. All four are globals or pointer
dereferences, which this stub reads reliably (`gdb stack locals read 0.0` applies
to *stack* locals only):

1. `gGRCommonStruct.yamabuki.gate_status`, `.gate_wait`, `.monster_wait`,
   `.monster_gobj` sampled ~10 times over 1,800 frames.
   * status pinned at 2 (`Open`) with `monster_gobj != NULL` ⇒ cause (a).
   * status cycling 1/2 while the model does not move ⇒ cause (b).
2. In the same session, `((DObj *)ndsGRYamabukiGateGObj()->...)` walked to DObjs
   1-3 and their `translate.vec.f.z` / `.y` printed. Pinned at
   `(330, -30, -330)` confirms the open pose; pinned at `(0, 390, 0)` would mean
   the model is closed and the owner is describing different geometry.
3. `gNdsStageGCDrawAllLoopGateSeenCount` and
   `gNdsStageGCDrawAllLoopGateFailStep` (`src/port/reloc_backend_movement.c:14487-14488`)
   confirm the gate is recognised for draw (`FailStep == 0`). No emitter prints
   them, so they must be read directly.

---

## Row S01 — Peach's Castle foreground roof

```
Bug: "Foreground castle roof renders ALL geometry now but the texture is missing
      on the now visible geometry. A continuous tiled roof surface is almost
      achieved."  (docs/BUGS.md, Stages section, **NOT FIXED in R36**)
```

The owner's previous wording for the same surface, from `docs/BUGS.md` history
(introduced at `6c75e56f677`, 2026-09-12):

> "Foreground castle roof renders as separated red triangular strips/wedges
> instead of a continuous tiled roof surface; roof geometry/texture slices are
> fragmented and misaligned."

Both reports are about the same texels, before and after the 2026-09-09
texel-alpha change (`docs/p2/BUG_NOTES.md:189-204, 391-402`).

### Contract

The steep tower roof is binding 3, source display list `0x1698`
(`src/nds/nds_native_stage_castle.generated.inc:167`, row
`{ 0x00001698u, ..., 9 triangles, 1 run, 1 texture epoch }`). Its source state
(`decomp/BattleShip-main/decomp/build/us/src/relocData/StageCastleFile2/DL_0x1698.dl.inc.c`):

```
gsDPSetAlphaCompare(G_AC_NONE),
gsDPSetRenderMode(G_RM_AA_OPA_SURF, G_RM_AA_OPA_SURF2),
gsDPSetTile(G_IM_FMT_CI, G_IM_SIZ_4b, 1, 0x0000, G_TX_RENDERTILE, 0,
            G_TX_NOMIRROR | G_TX_CLAMP, 3, ..., G_TX_NOMIRROR | G_TX_WRAP, 3, ...),
gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x013C, 0x013C),   /* 80 x 80 window */
gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 31, 2048),             /* 128 CI4 texels */
```

and the combine is inherited — the packet records it as `0xfc121824 / 0xff33ffff`
= `G_CC_MODULATEIA` (`nds_native_stage_castle.generated.inc`, state policy 7).

The measured image content is what matters, and it settles both owner reports.
`Tex_0x0748` is stored at `line = 1` (16 CI4 texels per TMEM row) but its tile
mask is 3/3, i.e. an **8 x 8** period. Decoding the bytes:

```
row  0..7  (16 texels each, mask uses only the first 8):
  15 15 15 15 14 15 15 15 | 0 0 0 0 0 0 0 0
  15 15 15 15 13 15 15 15 | 0 0 0 0 0 0 0 0
  15 15 15 15 13 15 15 15 | 0 0 0 0 0 0 0 0
  13 13 14 14 14 14 14 14 | 0 0 0 0 0 0 0 0
  13 15 15 15 15 15 15 15 | 0 0 0 0 0 0 0 0
  13 15 15 15 15 15 15 15 | 0 0 0 0 0 0 0 0
  14 15 15 15 15 15 15 15 | 0 0 0 0 0 0 0 0
  14 14 14 14 13 13 12 12 | 0 0 0 0 0 0 0 0
```

and `Lut_0x02B0` is a pure grey ramp whose **index 0 is `0xFFFE` — RGB
(248,248,248), alpha 0** — with indices 1-15 running 8..240 grey at alpha 1.

So the real roof texture is a **monochrome 8 x 8 intensity tile** that
`MODULATEIA` multiplies by the vertex/shade colour to produce the red roof, and
**every one of the 80 index-0 texels in the symbol is TMEM row padding beyond the
masked period** — unreachable on N64, because `maskS = 3` wraps S every 8 texels.

### Divergence

**A live, provable defect sits exactly on this surface's classification, and it
is the mechanism that flipped the owner's report from "strips with gaps" to
"filled but untextured".**

`ndsRendererCombineUsesAlpha` (`src/nds/nds_renderer_textures_effects.c:825-829`)
tests only **two** of the four alpha muxes of cycle 0:

```c
static s32 ndsRendererCombineUsesAlpha(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 9) & 0x07u) == source) ||   /* Ac0 */
            (((w1 >> 9) & 0x07u) == source)) ? TRUE : FALSE;   /* Ad0 */
}
```

Slot `Aa0` lives at `w0 >> 12` and slot `Ab0` at `w1 >> 12`, and neither is
tested. Its colour twin `ndsRendererCombineUsesColor` (`:528-538`) tests all
eight colour slots, so the asymmetry is not a deliberate granularity choice —
`ndsRendererCombineSecondOutputUsesAlpha` (`:831-835`) has the same hole for
cycle 1 (`Aa1` at `w1 >> 21`, `Ab1` at `w1 >> 3`).

For `G_CC_MODULATEIA` (`0xfc121824 / 0xff33ffff`) the alpha muxes are
`Aa0 = TEXEL0`, `Ab0 = 0`, `Ac0 = SHADE`, `Ad0 = 0`. The test reads `Ac0 = 4`
(SHADE) and `Ad0 = 7` (0), finds no `TEXEL0`, and returns FALSE — so at
`:11158-11165`:

```c
alpha_ignores_texels =
    ((use_texel1 == FALSE) && (stats != NULL) &&
     ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) == 0u) &&
     (ndsRendererHardwareOutputUsesAlpha(stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
     (ndsRendererHardwareOutputUsesAlpha(stats, NDS_RENDERER_ACMUX_TEXEL1) == FALSE))
        ? TRUE : FALSE;
```

evaluates TRUE for every `MODULATEIA` run with alpha compare off — which is the
Castle roof — and `:11823-11830` then forces every uploaded texel opaque:

```c
if (alpha_ignores_texels != FALSE)
{
    color |= 0x8000u;
}
```

This is precisely the C/D-only reading the 2026-09-09 note identified
(`docs/p2/BUG_NOTES.md:196-201`, *"the port's C/D-only test read 'texel
modulates' as 'texel ignored' for MODULATEIA, where TEXEL0 sits in slot A"*).
That note records the repair as **"host-only generator code"** (`:203`). The
runtime copy was never corrected, and the Castle is a blob stage whose textures
go through this runtime converter, not the Dream-Land-only static corpus
(`src/import/battleship_scvsbattle.c:152-159` gates that on `nGRKindPupupu`;
"a blob stage pins its own textures at owner prepare").

Consequence chain, matching both owner reports:

* **Before 09-09:** padding texels (index 0, alpha 0) uploaded DS-transparent →
  the roof drew as a lattice of its non-zero texels = *"separated red triangular
  strips/wedges"* and the `0909-witness3-castle.png` *"red EDGES with sky
  visible through"* reading.
* **After 09-09:** the same texels upload opaque at RGB (248,248,248) → the gaps
  fill with near-white, the 8-texel greyscale tile is buried under a solid
  bright field, and the surface reads as *"renders ALL geometry now but the
  texture is missing on the now visible geometry. A continuous tiled roof
  surface is almost achieved."*

**Ruled out, with evidence, so they are not re-opened:**

* `use_texture` / `NO_TEXEL0` — `ndsRendererCombineUsesColor` (`:528-538`) tests
  slot `a0`, where `MODULATEIA` and `MODULATEIDECALA` both put TEXEL0, so
  `ndsRendererHardwareUseTexture` (`:1007-1050`) returns TRUE at `:1040`.
* DL-link head inherited render mode — binding 5's head DL `0x2240` sets
  `G_TT_RGBA16`, the render tile and `gsSPTexture(..., G_ON)` before branching
  to `0x2288`, and binding 3's head sets its own alpha-compare/rendermode/tile.
* Clamped sub-pow2 padding — handled. For this tile
  `ndsRendererHardwareTextureMaterializesMaskedClamp` (`:4102-4117`) returns TRUE
  (CLAMP, mask 3, source extent 16, window 80 ≤ 128), so `source_read_width`
  becomes `1 << 3 = 8` at `:11052-11054` and the upload repeats the true 8-texel
  period. The wide 192-texel windows on bindings 5 and 11 take the
  `gNdsRendererClampedWindowPeriodUploadCount` arm at `:10996-11006` instead.
* VRAM/format blow-up — the converted image is repacked to `GL_RGB16` (4 bpp)
  whenever it fits 16 colours (`:11905-11920`), which this grey ramp does.

### Root cause

The runtime alpha-mux test omits combiner slots A and B, so `G_CC_MODULATEIA`
— the combine the Castle roof uses — is classified as "the combiner ignores
texel alpha" and the DS upload discards the texture's alpha. On this surface
the discarded alpha was the only thing masking 80 of 144 stored texels that the
N64 never samples, and forcing them opaque paints the roof with its own TMEM row
padding.

### Proposed fix

Scope the repair to the predicate that destroys texel alpha, so the broader
`ndsRendererHardwareAlpha` / `AlphaUsesVertex` behaviour is untouched in the same
change.

`src/nds/nds_renderer_textures_effects.c`, after `:829` (add the complete test):

```c
static s32 ndsRendererCombineUsesAlpha(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 9) & 0x07u) == source) ||
            (((w1 >> 9) & 0x07u) == source)) ? TRUE : FALSE;
}

+/* SLOTS A AND B ARE PART OF THE ALPHA OUTPUT TOO.
+ *
+ * gbi.h:511-512 defines G_CC_MODULATEIA alpha as (TEXEL0 - 0) * SHADE + 0, and
+ * :3088-3102 packs alpha A/B separately from C/D. The C/D-only test above
+ * therefore reads "texel modulates" as "texel ignored" for the single most
+ * common SSB64 alpha combine. Its colour twin ndsRendererCombineUsesColor
+ * already tests all eight slots; this is that completeness for alpha, and it
+ * is used ONLY by the alpha_ignores_texels predicate, which decides whether to
+ * throw the uploaded texture's alpha away. */
+static s32 ndsRendererCombineUsesAlphaAnySlot(u32 w0, u32 w1, u32 source)
+{
+    return ((((w0 >> 12) & 0x07u) == source) ||   /* Aa0 */
+            (((w1 >> 12) & 0x07u) == source) ||   /* Ab0 */
+            (((w0 >>  9) & 0x07u) == source) ||   /* Ac0 */
+            (((w1 >>  9) & 0x07u) == source) ||   /* Ad0 */
+            (((w1 >> 21) & 0x07u) == source) ||   /* Aa1 */
+            (((w1 >>  3) & 0x07u) == source) ||   /* Ab1 */
+            (((w1 >> 18) & 0x07u) == source) ||   /* Ac1 */
+            (((w1 >>  0) & 0x07u) == source)) ? TRUE : FALSE;   /* Ad1 */
+}
```

`src/nds/nds_renderer_textures_effects.c:11158-11165` (use it):

```c
     alpha_ignores_texels =
         ((use_texel1 == FALSE) &&
          (stats != NULL) &&
          ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) == 0u) &&
-         (ndsRendererHardwareOutputUsesAlpha(
-              stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
-         (ndsRendererHardwareOutputUsesAlpha(
-              stats, NDS_RENDERER_ACMUX_TEXEL1) == FALSE)) ? TRUE : FALSE;
+         (ndsRendererCombineUsesAlphaAnySlot(
+              stats->texture_combine_w0, stats->texture_combine_w1,
+              NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
+         (ndsRendererCombineUsesAlphaAnySlot(
+              stats->texture_combine_w0, stats->texture_combine_w1,
+              NDS_RENDERER_ACMUX_TEXEL1) == FALSE)) ? TRUE : FALSE;
```

Two review notes on the substitution. `stats->texture_combine_count == 0` is
already excluded upstream by `ndsRendererHardwareUseTexture`, so dropping the
`OutputUsesAlpha` wrapper does not lose that guard on this path. And the
substitution is **conservative in one direction only**: the raw all-slot test
does not model the 2-cycle `COMBINED` chaining that `OutputUsesAlpha` does, so
it can only report TEXEL0/TEXEL1 present where the wrapper reported it absent —
i.e. it can only *preserve* texel alpha more often, never destroy it more often.
SSB64 writes the same alpha muxes into both cycles on every combine examined
here, so the two agree on this corpus anyway.

**Expected result, and the reason this does not simply re-hide the roof:** the
masked 8 x 8 period contains only indices 12-15, all alpha 1, so restoring texel
alpha leaves every *reachable* texel opaque. Only the unreachable row padding
goes back to transparent, and the sampler never reads it. The two surfaces the
09-09 change was written for keep working for the same reason: an
`ALPHA_IGNORES_TEXELS` classification is still reached by any combine that
genuinely does not name TEXEL0/TEXEL1 in any alpha slot.

**Blast radius to check before publishing:** `MODULATEIA` is repository-wide.
`ndsRendererHardwareTextureColor(..., force_opaque)` at `:11786` and the
`graded_coverage` gate at `:11711` both read `alpha_ignores_texels`, so every
`MODULATEIA` + `G_AC_NONE` surface in the game changes upload representation
with this patch. The Zebes acid and the Zebes light cones are **not** affected
(both carry `G_AC_THRESHOLD`: acid `othermode_l = 0x0c1849d9`, lights
`0x00000001`).

### Confidence

**High** that the slot omission is a real defect and that it fires on the Castle
roof (both the combine words and the alpha-compare bit are read straight out of
the generated packet). **Medium** that it is the *whole* of the owner's residual
symptom — the texel-content argument is strong and explains both the old and new
wording, but it is a static argument about what the upload contains, not a
capture of the drawn surface.

Falsified by: a Castle capture in which the newly visible roof area is a colour
other than near-white shade, or a per-run texture read showing the roof binding
resolving a different image than `Tex_0x0748` / `Lut_0x02B0`.

### Open question

One capture plus one poke, no new code:

1. `gNdsNativeStageCastleRoofClipArm`
   (`src/nds/nds_renderer_native_owners.c:1420`, `volatile`, default 0) is an
   existing gdb-pokeable arm; setting it to 1 makes
   `ndsRendererNativeStageCaptureCastleRoofClip` (`:3615-3619`) publish per-run
   `PROJECTED_Z / SHIFT / DENSE / SUBMIT_V16 / RESULT / FLAGS` for the roof runs.
   Do **not** poke it with a 1-byte `set var` — `aligned-one-byte-poke-kills-melonds`.
2. A Castle screenshot at the camera used for
   `artifacts/visibility/0909-roofalpha2-castle.png`, cropped to the steep roof,
   compared against the same crop taken with the patch above applied. The
   prediction is a visible 8-texel grey-modulated tile pattern replacing a flat
   bright field.

---

## Row S03 — Planet Zebes

```
Bug: "Acid plane Color is accurate now but texture blending are all visibly too
      HARD. Edges are too defined instead of a gradient/smooth transistion.
      Stage lights on the ground floor have a flat/hard transparency (hard
      upsidown trapezoid shape) instead of looking like a real light source with
      a gradient that tapers to fully transparent towards the top."
      (docs/BUGS.md, Stages section, **NOT FIXED in R36**)
```

**Both sub-symptoms are one defect.** They are not two rows.

### Contract

Both surfaces carry their gradient in **per-vertex shade alpha**, and both
combines consume shade alpha.

**(b) the ground-floor light cone** — binding 10, source display list
`0x5870` in `ExternDataBank105`
(`decomp/BattleShip-main/decomp/src/relocData/105_StageZebesFile2.c:1518-1521`).
Decoded from the real O2R payload (nine commands, one `G_VTX` of 9, three
triangle commands, combine `0xfc121824 / 0xff33ffff` = `G_CC_MODULATEIA`):

| v | xyz | st | rgb | **alpha** |
| --- | --- | --- | --- | --- |
| 0 | (198, 0, 0) | (2048, 0) | 010409 | **42** |
| 1 | (137, 0, 120) | (1734, 0) | 63470f | **112** |
| 2 | (137, 0, 120) | (1734, 0) | 63470f | **111** |
| 3 | (198, 318, 0) | (2048, 2048) | dec52d | **255** |
| 4 | (0, 0, 176) | (1023, 0) | 63470f | **255** |
| 5 | (-137, 0, 120) | (313, 0) | 63470f | **111** |
| 6 | (-137, 0, 120) | (313, 0) | 63470f | **112** |
| 7 | (-198, 0, 0) | (0, 0) | 010409 | **42** |
| 8 | (-198, 318, 0) | (0, 2048) | c7ab0f | **255** |

A cone from a line at y = 318 to a semicircular rim at y = 0, with alpha ramping
**42 -> 255** across it and RGB ramping with it. The N64 interpolates that alpha
per pixel: a beam that fades out along its length. That is the owner's "real
light source with a gradient that tapers to fully transparent".

**(a) the acid** — binding 25, `MiscDataBank157` root `0x09D8`, `G_CC_SHADE`-
consuming combine `0xfc272c04 / 0x1f1093ff`. Its source is an eight-vertex fan,
Y = 0 at every vertex, alpha 220 at the centre and two far vertices and 0 at the
other five (`docs/p2/BUG_NOTES.md:2261-2277`, re-derived here as the same
gradient). The N64 draws a smooth radial fade.

### Divergence

`scripts/stages/generate_nds_native_stage.py:3063-3066` collapses each triangle's
three source alphas to a single number:

```python
tri_alpha = (
    tri_alphas[0] + tri_alphas[1] + tri_alphas[2] + 1
) // 3
```

and `:3076-3086` then clones all three vertices carrying that one value, so the
DS receives one `POLY_ALPHA` per triangle. The comment at `:3009-3019` states
the trade explicitly.

Running the generator in-process over the current descriptor, the emitted
per-polygon alphas are:

* **binding 10 (light cone): 5 triangles, three distinct alphas {136, 207, 255}**
  — `(8,7,6)` = avg(255,42,112) = 136; `(5,4,8)` = avg(111,255,255) = 207;
  `(3,8,4)` = 255; `(3,4,2)` = 207; `(1,0,3)` = 136. After the DS `>> 3` those
  are `POLY_ALPHA` 17, 25, 31. **Three flat bands across a cone whose source
  ramps 42 -> 255.** That is the owner's "flat/hard transparency (hard upsidown
  trapezoid shape)", exactly.
* **binding 25 (acid): 28 triangles, five distinct alphas {37, 73, 147, 183, 220}**
  — the post-subdivision state
  (`scripts/stages/native_stage_descriptors/zebes.py:178`,
  `alpha_subdivide_roots=((157, 0x9D8),)`). Five quantization steps across a
  smooth 0 -> 220 radial fade is the owner's "edges are too defined instead of a
  gradient".

No other Zebes binding emits more than one alpha, so these two are the whole row.

### Root cause

The DS geometry engine has per-vertex colour but **no per-vertex alpha**;
`POLYGON_ATTR` alpha is per polygon. The packet generator resolves that by
averaging, which turns every source alpha ramp into flat facets. The previous
attempt to soften it — one level of midpoint subdivision on the acid only — cut
the acid from 7 facets to 28 but cannot converge, and a second level was refused
because the midpoints are half-integers in the `s16` dense vertex
(`docs/p2/BUG_NOTES.md:4151-4174`).

**That refusal reasoned about the wrong constraint.** The midpoints are
half-integers only in the current coordinate basis. `NDSNativeStageDenseVertex`
already carries a per-vertex coordinate shift
(`src/nds/nds_native_stage_owner.generated.inc:121-130`,
`stage_vertex_coordinate_shift` at `generate_nds_native_stage.py:3857`), and the
acid's extent is 9,876 source units — **storing 2x the coordinates and spending
one more bit of shift makes every first- and second-level midpoint exact**, with
19,752 still inside `s16`. A third level would need 4x (39,504) and is out of
range, so two levels is the provable ceiling.

### Proposed fix

Two changes, in this order, each with its own proof. The second is the one that
actually removes the banding; the first is the bounded step that is provable
today.

**1. Make the subdivision level a per-root property and make its midpoints exact,
then add the light cone.**

`scripts/stages/native_stage_descriptors/zebes.py:178`:

```python
-    alpha_subdivide_roots=((157, 0x9D8),),
+    # (asset_id, root, levels). Level 2 needs exact midpoints, which the
+    # doubled coordinate basis below provides: the acid spans 9,876 source
+    # units, so 2x is 19,752 and stays inside the s16 dense vertex.
+    # (105, 0x5870) is the ground-floor light cone: five source triangles
+    # over a 42 -> 255 vertex-alpha ramp, emitted today as three flat bands.
+    alpha_subdivide_roots=((157, 0x9D8, 2), (105, 0x5870, 2)),
```

`scripts/stages/generate_nds_native_stage.py:2723` and `:3021-3037` take the
level from the row, recurse `level` times, and double the stored coordinates of
every vertex reached from a subdivided root while adding 1 to that root's
coordinate shift. `_append_alpha_midpoint` must **assert** that each new
`x/y/z/s/t` is an exact average of its two parents and raise `falsify` otherwise
— that assertion is what keeps a rounded midpoint from putting a fold in a plane
that must stay planar, which was the correct half of the earlier refusal.

Cost: acid 28 -> 112 triangles, light cone 5 -> 80, stage total 172 -> ~340.
Zebes already runs near the cadence gate, so this must be measured on the
Boundary profile before it is published, and if it does not pay, the level for
the acid drops back to 1 while the light cone keeps 2 (it is five triangles).

**2. The exact repair, if step 1's banding is still visible:** carry the alpha
field in **texture alpha**, which the DS does interpolate per pixel. The light
cone's alpha is a smooth function of its own `(s, t)` — `255` along `t = 2048`
and a bell `42 / 112 / 255 / 112 / 42` along `t = 0` — so a generated `GL_RGB8_A5`
(A5I3, 32 alpha levels) ramp bound to that run reproduces the source exactly with
five triangles and no added geometry. The repository already uploads that format
on the `graded_coverage` path
(`src/nds/nds_renderer_textures_effects.c:11709-11726, 11893-11904`), so this is
an existing mechanism, not a new framework. It is a representation change and
must be split from step 1 and proven on its own.

Do **not** substitute a global alpha, an arbitrary per-stage offset, or a
constant-alpha "good enough" value: the source alpha is live data on both
surfaces (`combine_alpha_reads_shade` is true on every acid run, and the light
cone's `G_CC_MODULATEIA` alpha is `TEXEL0 * SHADE`).

### Confidence

**High.** The source vertex alphas are decoded from the shipped O2R payload, the
emitted per-polygon alphas are produced by running the current generator, and the
two match the owner's two descriptions term for term. The collapse site is a
single arithmetic expression with a comment that already admits the trade.

Falsified by: a Zebes capture in which the light cone shows more than three
distinct opacity levels, or an acid capture showing more than five.

### Open question

None for the diagnosis. Open for the *fix*: whether the frame cost of level-2
subdivision is affordable on Zebes. That is a Boundary-profile measurement of
`WORK-H` P50/P95 on `p2_battle_realtime` with the regenerated packet, against the
current 172-triangle baseline — not a correctness question.

---

## Files read for this diagnosis

Port: `src/nds/nds_renderer_native_owners.c`, `src/nds/nds_renderer_textures_effects.c`,
`src/nds/nds_renderer_preamble.c`, `src/nds/nds_native_stage_blob.c`,
`src/nds/nds_native_stage_select.inc`,
`src/nds/nds_native_stage_{castle,zebes,yamabuki}.generated.inc`,
`src/nds/nds_native_stage_owner.generated.inc`,
`src/port/renderer_adapter_stage.c`, `src/port/renderer_adapter_matrix.c`,
`src/port/reloc_backend_movement.c`, `src/import/battleship_sys_objanim.c`,
`src/import/battleship_gryamabuki_ground.c`, `src/import/battleship_grpupupu_ground.c`.

Producers: `scripts/stages/generate_nds_native_stage.py`,
`scripts/stages/native_stage_descriptors/{castle,zebes,yamabuki}.py`,
`scripts/stages/test_yamabuki_gate_animation.py`, `Makefile`.

Source of truth: `decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c`,
`.../src/it/itground/{itporygon,itglucky,ithitokage}.c`,
`.../src/sys/{objman,objdisplay,objtypes}.h/.c`,
`.../src/relocData/{106_StageCastleFile2,105_StageZebesFile2,160_StageYamabukiFile4}.c`
and the extracted `StageCastleFile2/*.dl.inc.c`, `*.tex.inc.c`, `*.palette.inc.c`.

Artefacts: `builds/remaining-bugs-playtest-r36/{smash64ds.nds,nds_build_config.h}`.
