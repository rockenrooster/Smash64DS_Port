# R2-03 E16 — the fighter's largest cost is the DS's own hardware lighting, done on the CPU

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** Premise proven, with no exceptions. **Every fighter epoch is lit,
534 vertices a frame are shaded at ~169 ticks each, and the equation being
evaluated is the Nintendo DS geometry engine's own — on an engine E14 measured
idle.** Designed, not yet built.

## 1. The equation

`ndsRendererHardwareLitShadeColorPrepared`, per vertex:

```
colour = ambient + (diffuse * dot(normal, light_dir)) / 127
```

`light_color_1` is the diffuse, `light_color_2` the ambient, and the `r/g/b` of
`sNdsNativeFighterDenseVertices[].rgba` is the **normal** — F3DEX packs the
normal into the colour field for lit vertices. Then a material modulate, an
`RGB15` pack, and `color_modulate`.

That is, term for term, what the DS geometry engine computes in hardware when a
polygon is submitted with lighting enabled and `GFX_NORMAL` per vertex.

## 2. The premise, measured

479 frames, both fighters:

| counter | total | per frame |
|---|---:|---:|
| **lit epochs** | **23,255** | 48.5 |
| **unlit epochs** | **0** | 0 |
| epochs taking the LUT path | 23,255 | 48.5 |
| epochs applying a material | 13,280 (57%) | 27.7 |
| **vertices lit** | **245,757** | **513.1** |
| vertices copied from a shared source | 10,322 | 21.5 |

**Not one fighter epoch in a match is unlit.** The path is not a rare branch that
happens to be expensive; it is the only path the fighter takes.

Shade cost in the same build is 90,295 ticks/frame, so **~169 ticks per shaded
vertex**, covering a dot product, a LUT lookup, a material scale, an `RGB15`
pack and a modulate.

## 3. Why precomputation cannot have this, and hardware can

R2-03 E1 refuted memoising the shade across frames: 1,796 of 1,835 frames changed.
That result is correct and it is *explained* here rather than worked around.

The light direction is transformed into each root's local space by that root's
modelview. The fighter animates, so every joint's modelview changes every frame,
so the local-space light direction changes, so every dot product changes. **The
output is unmemoisable for a structural reason, and no amount of key refinement
fixes it.**

The DS geometry engine has exactly this problem solved in silicon: the light
vector is set once in view space and the hardware applies the current matrix per
vertex. The thing that makes the software memo impossible is the thing the
hardware does for free.

And the hardware is available. E14: command FIFO empty entering and leaving all
946 fighter submissions, geometry engine busy on 0 of them.

**`GFX_LIGHT_VECTOR`, `GFX_LIGHT_COLOR`, `glLight` and `POLY_FORMAT_LIGHT` appear
nowhere in `src/nds` or `src/port`. The renderer has never used DS hardware
lighting at all.**

## 4. The design

- **Load time.** Pack each dense vertex's normal into the DS `GFX_NORMAL` word
  (10-bit signed per axis). The source bytes are already normals; this is a
  format change, done once, which is exactly the "compute once, not every frame"
  trade `PROJECT_GOAL.md` asks for.
- **Per root.** Set light 0's vector and colour from `light_color_1`, material
  ambient from `light_color_2`, material diffuse from the epoch's material
  colour, and fold `color_modulate` — the damage flash — into the material
  rather than into every vertex. This is ~28 roots a frame, not 534 vertices.
- **Per vertex, in the emit.** Write the precomputed `GFX_NORMAL` word instead of
  the computed `GFX_COLOR` word. **One FIFO word either way — the traffic is
  unchanged.**

The per-vertex ARM9 cost falls to a single load and store of a constant word,
which the emit already pays. It also drops the write traffic into
`sNdsNativeFighterPreparedDense[].shaded_rgba` / `.packed_color`.

**Expected: most of 90,295 ticks/frame, less ~28 roots of light and material
setup.** Against R2-03's 250,833 gap that is the largest single cut identified
in the phase.

## 4a. The prerequisite the design above omits, and it is not optional

Written as §4 alone, this plan produces wrong lighting and would look like a bug
in the normal packing. The matrix path has to change first.

`ndsRendererLoadHardwareRawComposedMatrix` loads an **identity projection** and
the **CPU-composed modelview x projection** as the modelview, via
`ndsRendererHardwareSetMatrixMode(GL_MODELVIEW)` — matrix mode 1, position
only. Mode 1 **does not update the directional/vector matrix**, which is the
matrix the geometry engine transforms normals by. So today there is no valid
vector matrix at all, and a naive `GFX_NORMAL` would be lit against whatever was
left there.

Loading the composed MVP into the vector matrix instead would be equally wrong:
normals must be rotated by the modelview only, never by the projection.

The fix is to stop composing on the CPU and let the hardware do it:

- load the projection into `GL_PROJECTION` and the modelview into
  `GL_MODELVIEW_VECTOR` (mode 2), which writes the position *and* vector
  matrices.

The plumbing for this already exists: `NDSRendererNativeFighterRoot` carries
**both** `composed_matrix` and `modelview_matrix`, and
`ndsRendererNativeBindProductionRoot` already loads both into the traversal
state. Only the projection needs adding to the struct — the adapter has it as
`sNdsRendererAdapterNativeOwnerProjection`.

The scaling is exact rather than approximate. `ndsRendererBuildRawHardwareMatrix`
divides row 3 of the composed matrix by the world-unit shift; under the
row-vector convention `C[3] = M[3] x P`, so scaling `M`'s row 3 *before* the
multiply gives the identical result. Split loading therefore reproduces the
current transform, modulo the hardware's internal precision versus the CPU's
20.12 rounding — which is a sub-pixel difference and a screenshot question, not a
correctness one.

**Open, and the first thing to settle when building this:** whether
`ndsRendererAdapterComposeNativeRootMatrix` can then be *deleted*, taking a 4x4
multiply per root out of the 120,407 MatrixPrep bracket. That depends on whether
the composed matrix has any live consumer on the production path besides the
hardware load. It has many consumers in the file — including
`ndsRendererTransformVertex20p12`, a CPU vertex transform — and **it is not
established which of them the production path actually reaches.** If the hardware
load is its only production consumer, the compose is free to delete and the
matrix change pays for itself; if the near-plane or snapshot paths need it, the
compose stays and the matrix change is a small net cost that only pays off once
the lighting lands on top of it.

An earlier revision of this document asserted the deletion as a side benefit.
That was stated ahead of the evidence and is downgraded here to the open question
it actually is.

**Light vector placement follows from the same reasoning.** `GFX_LIGHT_VECTOR`
stores the supplied vector transformed by the vector matrix *at the moment it is
written*. The software path transforms the light into each root's local space and
dots it with untransformed local normals; the hardware equivalent is the mirror
image — write the light once per frame in **view** space while the vector matrix
is identity, then let each root's vector matrix carry its local normals into view
space. Same relationship, computed by the engine instead of the CPU.

This makes E16 a larger change than §4 implies, and a strictly better one: it
moves the per-root matrix compose onto the idle hardware along with the
per-vertex lighting.

## 4b. Checked and cleared: the engine is used, just starved

The obvious worry about §4a is that the transform unit is bypassed altogether —
that `NDSNativePreparedDenseVertex.gx_xy` holds CPU-projected screen
coordinates, which would explain the idle engine and would make hardware
lighting impossible.

It does not. `ndsRendererBuildRawHardwareMatrix` leaves composed rows 0..2 alone
and rescales only row 3, for the 1/256 source-unit convention — *"preserving
X/W, Y/W, and Z/W"*. Vertices reach the engine in model space and the hardware
performs the transform; `gx_xy` is just the two coordinates packed into one
`VERTEX_16` word.

So the idle engine in E14 needs no exotic explanation: **the CPU is slow enough
feeding it that the FIFO drains before the next command arrives.** The transform
unit works, it is simply never the constraint, and E16 remains viable.

## 5. Risks, stated before building

- **The DS light model is not bit-identical to the N64's.** Colours will shift
  slightly — RGB15 material and light channels versus the software path's RGB8
  intermediate. This is a rendering-side change, so it gates on the fidelity
  budget and **the owner's visual approval**, not on exactness. `PROJECT_GOAL.md`
  lists "simplified lighting" among the explicitly allowed compromises.
- **Two light colours, one hardware light.** The source's `light_color_2` is an
  ambient term, which maps to material ambient rather than a second light, so one
  of the DS's four lights suffices. Worth confirming against BattleShip before
  building.
- **Per-vertex alpha.** The software path unpacks `vtx->a` but the packed output
  is `RGB15` only, so alpha is not per-vertex today and nothing is lost.
- **57% of epochs apply a material.** Those need the material register reloaded
  per epoch; the other 43% can keep the root's.

## 6. Status and scope

**Not implemented.** This is a real renderer change — load-time table format, the
emit's per-vertex word, and per-root light/material state — and it needs a
synchronized screenshot pair and the owner's eye before it graduates. Starting it
half-way would be worse than not starting it.

What is done is the part that makes it safe to start: the premise is measured and
holds without exception, the mechanism that defeated E1 is explained, and the
hardware is proven idle and proven unused.

## 7. Note on the instrument

The census counters accumulated across E15 and E16 now perturb the build
noticeably — `WORK` P50 1,176,576 against the control's 1,129,856, and the VBlank
histogram has collapsed to `2:6 3:461`. They are all under
`NDS_TASK91_DRAW_PHASE_CENSUS`, default 0, so nothing ships with them, but the
next measurement on this path should read the control build for absolutes and use
the census only for shares.
