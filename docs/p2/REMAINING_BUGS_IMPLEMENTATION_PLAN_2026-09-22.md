# r36 owner playtest — diagnosis and implementation plan (2026-09-22)

This supersedes `REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-21.md` as the live
plan. That file stays as the record of the r36 batch; several of its
conclusions are refuted below and each refutation says which.

**Input.** The owner played `builds/remaining-bugs-playtest-r36/smash64ds.nds`
(linked 2026-09-22 06:35 from `260b754be00`) and re-opened eight rows in
`docs/BUGS.md`, two of which a previous session had marked **FIXED**, plus one
new regression. The verbatim queue is the owner's; nothing here rewords it.

**Standing rule this batch is built on.** Six of the rows below failed because
a *recorded claim in this repository* was wrong and pointed the previous
session away from the seam. Every row here states which premise it had to
re-derive. Do not close a row on a claim you did not re-measure.

---

## Status

**Playtest build: `builds/remaining-bugs-playtest-r37/smash64ds.nds`,
sha256 `9336b6debe5761b3...`, linked 2026-09-22 09:47.**

| Row | Seam | State |
|---|---|---|
| G1 VFX play or not by fighter combination | graded-quad texture table | **Implemented**, `2dc42e97624` |
| Y1 Yoshi's guard egg invisible | EFDesc offset mapper | **Implemented**, `889c75ff60d` |
| P2 Pikachu down-B blue burst missing | particle quad sheet admission | **Implemented**, `889c75ff60d` |
| C1 CSS hover-to-preview delay | preview service byte budget | **Implemented**, `079ef17d06b` |
| C2/C5 one eye closed | texture-part status mirror | **Implemented**, `079ef17d06b` |
| S1 Castle roof texture missing | runtime alpha-mux test | **Implemented**, `d97f7f3b2e1` |
| S2 Zebes floor lights too hard | per-triangle alpha averaging | **Implemented**, `9bc15c3ee0b` |
| P1/K1/J1/C3/C4/C5 face colour != body colour | fighter shade fold | **Implemented**, `3061d6ae332` |
| C6 Link turns gray | retired preview's texture entries | **Implemented**, `5646bedf79a` |
| S2 Zebes acid edge | same, deferred second level | **NOT repaired**, needs a cadence measurement |
| S3 Saffron door always open | gate animation, not the blob | **NOT repaired**, witness added `b1f4be2fc9e` |

Nothing here is owner-accepted. Every "Implemented" row builds clean and is
`NATIVE_ONLY_PASS` at 316 link inputs; none has been playtested. Static
checkers re-run green after the batch: `check-r2-shade-twin` (5 claims, CLAIM 5
mutation-verified RED/GREEN), `check_fighter_face_body_material`,
`check-nds-particle-banks`, `check_nds_native_stage`,
`test_css_preview_transaction` 8/8, `test_yamabuki_gate_animation` 11/11,
`check-native-owner-wiring`, `check_native_owner_image_spans`,
`check-decomp-pristine`, `check-melonds-policy`.

**The unrepaired rows are named rather than quietly narrowed.** Each has a
diagnosis and a single named next observation; neither is blocked on a decision.

### S3 Saffron, the one row with no repair

Frame 0 of the CLOSE script is itself the open pose:
`160_StageYamabukiFile4.c:158` sets `TraZ = 330` with a negative rate and only
then ramps to 0, and `grYamabukiMakeGate` installs exactly that script at setup
with `gcAddAnimJointAll(..., 0.0F)`. **A gate whose joints are installed and
never advanced sits wide open for the whole match** while its state machine,
collision and sound keep cycling.

This corrects both earlier readings. The descriptor says the authored pose is
CLOSED and the DObjDesc at `0x08A0` agrees (TraZ 0, TraY 390, TraZ 0), so this
morning's inference of an authored OPEN pose was wrong — but the descriptor's
conclusion, that a frozen gate reads closed, is wrong too, because the gate is
never at its authored pose once the close script's first block runs.

`b1f4be2fc9e` samples the first animated child's TraZ where the gate is already
identified, with min and max, because one sample cannot tell a frozen value
from one caught mid-cycle:

```
pinned at 330  -> joints are not being advanced
330 <-> 0      -> they are; the fault is upstream in the state machine
pinned at 0    -> the door is closed; the row is about other geometry
Samples == 0   -> the gate never reaches the draw loop at all
```

Ruled out and not to be reopened: the port's `gcPlayAnimAll` advances every
DObj's joint unconditionally (`ndsGcPlayAnimAllStableSkip`'s skip is
MObj-material only); the `ll*` offset arithmetic resolves correctly; and the
DObjDesc list and both anim tables are the same length, so the lockstep walk in
`ndsAObjEvent32NormalizeDObjTable` cannot run off the end. Three behavioural
patches have been written for this row on hypotheses and the owner has rejected
all three, so this one gets an observation first.

---

## G1 — VFX that play or not depending on the fighter combination

**Landed: `2dc42e97624`.**

The five graded call sites — Pikachu's Thunder, Thunder Jolt and the Jolt
effect, Ness's PK Thunder and PK tail — share one dedicated A3I5 texture table
in `ndsRendererNativeBindGraded`
(`src/nds/nds_native_textured_quad.exec.inc`). It had eight rows.

A row cannot be recycled inside the frame that bound it, because the geometry
is already in the list and the rasteriser reads its texels at scanline time.
That rule is correct and stays. Its consequence is that the table has to
**hold the whole set of distinct graded images drawn in one frame**, and that
set is roster-sized: each live instance brings its own image, because these
owners pass a live `material->current_image` that changes per animation frame.
Four fighters can want twelve at once.

The ninth request returns FALSE and the caller falls through to the generic
cache — which carries one alpha bit, the entire reason this helper exists. An
IA8 glow reduced to a threshold draws as a sub-pixel hairline. On screen that
reads as *the effect did not play*, and which effects lose the race depends on
who else is on the stage. That is the owner's sentence.

Sixteen rows cost `16 * 28` bytes of table and **no** extra VRAM until a frame
actually needs the ninth texture, because entries are created on demand.

Separately, the cache-hit predicate was the image pointer alone, while width
and height come from the live tile window, the TLUT selects CI4 over IA8, and
the lane decides whether the fill reads the file word-swapped. The key now
carries all four.

**Refuted while doing this.** A review suggested the CI4 palette could be
clobbered by the IA8 `ndsGradedQuadRamp` on a colour change. It cannot: the
caller pins `prim` and `env` to zero for every CI4 request, so that branch is
IA8-only by construction. Recorded so nobody re-raises it.

**Owed.** `gNdsGradedQuadLiveHighWater` now reports the real peak simultaneous
live set. Read it after a four-fighter Pikachu/Ness match and set
`NDS_GRADED_QUAD_TEXTURES` from that measurement rather than from the
arithmetic in its comment. `gNdsGradedQuadIdentityMisses` non-zero proves the
old pointer-only key was binding wrong texels in the shipped ROM.

---

## Y1 — Yoshi's guard egg is invisible

**Landed: `889c75ff60d`.**

`ndsEFManagerMapFileOffset` maps EFDesc source offsets through
`ndsRelocNativeAssetAddress`, which resolves **data spans only**.
`dEFManagerYoshiShieldEffectDesc` carries `EFFECT_FLAG_USERDATA` and neither
`0x1` nor `0x4`, so `efManagerMakeEffect` hands `o_dobjsetup` straight to
`gcAddDObjForGObj` as `DObj::dv` — it is a **display list**, YoshiModel
`0xA860`. The compact battle pack prunes geometry and leaves one
`ENDDL` + source-offset identity cell per retained root, outside every retained
data span. Decoded from `builds/build/battle-core/06.fpc`: `0xA860` is in none
of the 49 retained Model spans and **is** root cell 54 at
`roots_offset 0x477C`.

So the mapper answered NULL, `ndsEFManagerBeginMappedDesc` failed, the wrapper
returned NULL before the source maker ran, and `ftParamHideModelPartAll` still
hid Yoshi. **No effect GObj and no DObj were ever created**, which is why every
renderer-admission arm anyone added for this root was upstream of nothing.

The fix falls back to `ndsRelocNativeRootAddress`, the cell-aware resolver
`wpManagerMakeWeapon` already uses for this exact root — the identity on an
unpacked file, NULL for an offset that is not a root, so no descriptor that
maps today changes.

**Three premises that had to be re-derived.**

1. `05fc0add5de` ("reinstate the owner") **is** in r36 and is not the blocker.
2. Its admission arm tests raw `gFTDataYoshiModel + 0xa860`, an address that
   does not exist in a compact pack. Every neighbouring owner in that file uses
   the cell-aware `ndsRelocNativeRootOffset()`.
3. "The egg had no native owner" was false. `renderer_adapter_stage.c:7010-7027`
   already owns asset 338 / root `0xA860` and already admits
   `nGCCommonKindEffect` with `mobj == NULL`. The reinstated bank row is a
   duplicate.

**Owed, as a separate commit.** Removing the duplicate arm, its generated bank
row and `scripts/check-p2-yoshi-egg-efdesc-native.ps1` (with `$expectedVerifiers`
19 → 18 in `verify-all.ps1`) collapses two owners back to one and returns
2,048 B of persistent VRAM. It has checker blast radius, so it is not folded
into the repair. Until it is done, a lab (unpacked) build draws this egg from a
different bank than the shipped build — "measure the config you ship" applies.

---

## P2 — Pikachu's down-B blue self-hit burst

**Landed: `889c75ff60d`.**

The trigger chain was already right: the motion event emits
`nEFKindThunderAmp`, `ftParam` dispatches it, the maker runs, script `0x74` is
packed and texture 46's texels are in the NitroFS pack. But texture 46 was not
**on the quad sheet**, so `gNdsParticleQuadFirstRow[46]` read `0xFF`,
`ndsParticleQuadFrameFor` returned NULL, and the draw loop took the `continue`
that emits zero pixels. Every other script in the down-B chain has a quad row,
which is precisely the owner's "doesn't render *all* related VFX".

`9d0de28d3b2` recorded that "being outside that sheet is its intended state".
That is false for a common-bank particle, whose only draw path **is** the sheet:
the only readers of `gNdsParticleTextures` are the Fox-blaster, Whispy, shield
and fireball special cases, and `nds_particle_runtime.h` says a particle whose
texture is not in the atlas draws nothing. It is the identical failure the
producer already documents for texture 12.

Fixed at the producer. Texture 46 joins `QUAD_RESTORED_SOURCE_LIVE` with a
per-texture cap in the texture-7 idiom; admitted at 16x16 for 256 B and one
packed frame, 39,680 of 40,960 with Yoster on, **excluded set unchanged**.

**A trap worth not repeating.** A bare `python
scripts/generate_nds_particle_banks.py` bakes with `YOSTER_BAKE_ENABLED` false,
because the Makefile is what exports `NDS_P2_STAGE_YOSTER`. Diffing a bare run
against the committed flag-ON JSON shows the whole `yoster` bank vanishing and
texture 192 looking evicted. Nothing is evicted. This cost a cycle tonight and
is now written into the producer beside the cap.

---

## P1 / K1 / J1 / C3 / C4 / C5 — face colour differs from body colour

**Diagnosed. Not yet implemented — the design is below and it is the largest
remaining piece.**

### Why r36 did not fix it

`8c7dab82134` added `ndsRendererR2ClampDiffuseToMaterial` to the replay
re-derive at 04:07; r36 linked at 06:35. The clamp is compiled in — its guard
nest is `NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2) &&
NDS_R2_FIGHTER_HW_LIGHT`, all three live in r36's config. It shipped, and it is
the wrong curve.

### What the two runs are

`ctx["direct_epoch_policies"]` against
`nds_native_fighter_owner.generated.inc:500`:

- family 0 — `USE_VERTEX | USE_TEXTURE`, textured: the **face** runs.
- family 1 — `USE_MATERIAL | USE_VERTEX`, untextured: the **body** runs.

Every body epoch on the three models carries one tinted prim: Pikachu
`0xFFD933`, Purin `0xFFCDD8`, Kirby `0x00FF5A` and `0x00B300`.

*The checker docstring's claim that Purin's face inherits a prim the model
never sets is true of the source command stream and irrelevant to the port:*
those epochs are family 0, `use_material == 0`, so no prim is ever folded into
them. It is not the defect.

### The arithmetic

Face (textured, no material):

```
DS  : clamp31( RGB5(l2) + RGB5(l1)*d )  then TEXTURE MODULATE by the texel
N64 : clamp8 ( l2 + l1*d ) / 8          then the RDP multiplies by the texel
```

These agree, because the texture modulate is a **post-shade multiply** and
gives the face the source's inner clamp for free. With `l1 = 0xFFFFFF` and
`l2 = 0x808080` the face saturates at `d = 0.484` and is at full texel
brightness for the whole rest of the range.

Body (untextured, prim folded into the material):

```
N64            : prim * min(1, (l2 + l1*d)/255)        saturates at d = 0.498
port, no clamp : clamp31( prim*l2/255 + prim*l1/255*d )
port, clamped  : prim*l2/255 + (prim - prim*l2/255)*d  reaches prim at d = 1
```

The unclamped form is **exact for d <= 0.498** and overshoots past it, per
channel, because each channel saturates at 31 rather than at its own prim.
Measured by the checker: `purin high root 0 epoch 3 dot=1.0 prim=0xffcdd8ff
source RGB5 (31,25,27) vs port RGB5 (31,31,31)` — **a pink body drawn pure
white**. That was r35.

The clamped form preserves hue exactly (it is the same ramp with the light
reduced from `l1` to `255 - l2` so it can never saturate) but arrives late: at
`d = 0.5` the body draws at **75.1%** of prim while the face beside it is at
100% of its texel. **That is r36 — a body uniformly dimmer than the face over
the entire lit half.** The seam did not go away, it changed sign.

### Why no choice of diffuse/ambient can fix it

We need `out(d) = prim * min(1, s(d))`. The geometry engine computes
`clamp31(A + D*d)` and clamps at 31 **per channel**, not at prim per channel.
The channels of a tinted prim reach 31 at different `d`, so they cannot
saturate together; holding them below 31 is exactly the ramp that arrives late.
Both failures are one fact: **the DS has no inner clamp, and the source's clamp
happens before the prim multiply.**

Checked and rejected:

- **Toon shading.** `toon[i] = prim * i / 31` with the index carried in red is
  arithmetically exact. `TOON_TABLE` is a *rendering* engine register latched
  once per frame, so a frame holding Pikachu and Kirby cannot have two.
- **A second light.** The engine sums lights and saturates once at the end.
- **Emission, or splitting prim between material and light colour.** Each is
  another factor in the same product; none introduces a clamp between prim and
  the sum.

### The fix

Give the body the post-shade multiply the face already has:

```
diffuse = RGB5(light1), ambient = RGB5(light2)   (no prim fold)
texture = one texel whose colour is prim, POLY mode = modulation
```

which evaluates to `clamp31(RGB5(l2) + RGB5(l1)*d) * prim` — the source's own
order of operations, and the identical arithmetic the face runs already prove
correct on hardware.

**Cost is one bind plus one `GFX_TEX_COORD` per run, not per vertex.** The DS
texcoord register is sticky and every texel of the tile is the same colour, so
the untextured vertex loop stays untextured — about two extra FIFO words
against ~53 runs a frame per fighter. Compare the alternative, per-vertex
software shading, which is exact but costs Kirby alone 832 vertices a frame.

**Implementation order.**

1. `ndsRendererHardwarePrepareFighterTintTexture(material_color, &name)`: a
   16-entry `{prim, name}` cache over 8x8 `GL_RGB16` textures built with
   `ndsRendererHardwarePrepareIFCommonPal16Atlas`, palette entry 0 = `RGB15(prim)`,
   every texel index 0. ~4 creations per match total.
2. `ndsRendererNativeShadeProductionActions`: when `use_material != 0`, the prim
   is not white and `color_modulate` is identity, write the **unfolded** light
   colours and record the tint texture for the epoch. Otherwise fall through to
   today's clamped fold, with a counter on each fallback reason.
3. Run prepare: when an epoch carries a tint texture and the run is untextured,
   pass `use_texture = TRUE` with that name to
   `ndsRendererHardwareBeginTriangleBatch` and emit one `GFX_TEX_COORD`. The
   `(use_texture != FALSE) != (policy->textured != 0u)` reject must learn about
   this third state.
4. Mirror both decisions in the packet recorder and in `ndsFighterPacketApplyTint`.
   *Replay must mirror every derive step* — this row's previous repair existed
   only because that had been missed once already.
5. Re-derive `check_fighter_face_body_material.py`'s fold census **from the
   producer**. It currently models the port without the clamp and still reports
   r35's numbers, so it is a stale falsifier; do not patch it to agree.

**The change is strictly additive**: any epoch the new path cannot serve —
non-identity `color_modulate`, no palette slot — behaves exactly as today. The
blast radius is the three tinted fighters' body runs.

---

## S3 — Saffron's Pokemon gate is always open

**The recorded premise is overturned. The previous FIXED was not the fix.**

Three measurements:

1. **The shipped blob is not stale.** Extracted `native_stage_yamabuki.bin`
   from the r36 ROM (file offset 65,964,032) and compared byte-for-byte with
   `builds/build/nitrofs/stages/native_stage_yamabuki.bin`. **Identical**, and
   both carry `camera_binding_mask = 0xE4894` at header offset 78. The widened
   mask from `a6c548f40de` did ship.
2. **The mask consumer is correctly gated and is the only one.**
   `ndsRendererNativeStageTask51EnsureWorld` has exactly one call site
   (`nds_renderer_native_owners.c:3668`), guarded at `:3643`. Bindings 17/18/19
   skip the baked constant world matrix and fall through to the live compose.
3. **The descriptor's pose claim is wrong, and it inverts the row.**
   `yamabuki.py` asserts the authored pose is CLOSED and the previous session
   closed the row as "frozen closed; now cycling". The source contradicts it:
   `grYamabukiMakeGate` **ends with `grYamabukiGateAddAnimClose()`**. A gate
   that must be told to close at setup is authored **open**. That matches what
   the owner saw in r35 and again in r36. "Frozen closed" is not a state the
   owner ever reported.

**Where the remaining fault is.** `grYamabukiGateAddAnimOffset` calls
`gcAddAnimJointAll(gate_gobj, map_head + offset, 0.0F)` then
`gcPlayAnimAll(gate_gobj)`, and **frame 0.0 of the close animation is the open
pose**. `grYamabukiMakeGate` also installs a per-frame process,
`gcAddGObjProcess(gate_gobj, gcPlayAnimAll, nGCProcessKindFunc, 5)`, which is
what advances it. A gate whose animation is installed but never advanced sits
at frame 0 forever — permanently open. Determine whether the port runs
processes on this Ground-link GObj.

The draw path is not the suspect: `ndsStageGCDrawAllLoopIsYamabukiGate`
(`reloc_backend_movement.c:14538`) recognises it and already writes
`gNdsStageGCDrawAllLoopGateSeenCount` and `gNdsStageGCDrawAllLoopGateFailStep`.

Also verify the `ll*` offset arithmetic:
`(intptr_t)&llGRYamabukiMapGateOpenAnimJoint` uses the address `0x9b0` as an
offset. That address-as-offset family has killed three effects here and
recurred four times.

**Stale note to correct while there.** `battleship_gryamabuki_ground.c`'s header
says the ground-monster item kinds have no maker so the spawner returns NULL.
They exist now — `battleship_item_fushigibana.c`, `_glucky.c`, `_hitokage.c`,
`_marumine.c` and `_porygon.c` all call `grYamabukiGateSetClosedWait()`.

---

## S1, S2, C1, C2, C5, C6

Under investigation in the stage and CSS lanes; findings land in
`docs/p2/agent-lane-stages.md` and `docs/p2/agent-lane-css.md` and are folded
into this plan as they arrive. Carried premises worth stating now:

- **C1.** The "replace the blocking loader, cut the 13-tick dwell" diagnosis is
  obsolete; that work landed. The current service has an 8 KiB aggregate budget
  but issues one 2 KiB load step per invocation, and
  `ndsMNPlayersVSPreviewRecordLatency` is credited *before*
  `ndsMNPlayersVSPreviewRebuildChangedKind` and before the first eligible draw,
  which the CSS takes on alternating source updates. **The existing latency
  number does not measure the owner's symptom**, so "5 of 12 within 24 tics" is
  not an acceptance threshold.
- **S1.** The owner says all roof geometry now renders. Preserve that and
  diagnose only the missing texture.
- **S2.** Two sub-symptoms under one row — the acid edge and the floor-light
  taper. A correct base colour closes neither.

---

## Verification owed before any of this is called fixed

Per `docs/BUG_FIXING_PROCESS.md`, none of the implemented rows is closed.
Each owes: engagement on the natural shipping path, the required pixels, source
contract, resource safety, native-only output and cadence, then owner
acceptance. What exists today is a clean build and `NATIVE_ONLY_PASS` at 316
link inputs.

Counters to read on the next playtest ROM, all of which already exist:

```
gNdsGradedQuadLiveHighWater      peak simultaneous graded textures in a frame
gNdsGradedQuadIdentityMisses     same pointer, different content (should be 0)
gNdsGradedQuadTextureFails       graded refusals, i.e. effects that degraded
gNdsEFDescDisabledCount/Last     EFDesc mapping refusals; Yoshi's shield desc
gNdsParticleQuadMissMask[1]      bit 14 is texture 46
gNdsStageGCDrawAllLoopGateSeenCount / GateFailStep   Saffron gate recognition
```
