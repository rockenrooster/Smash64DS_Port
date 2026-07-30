# R2-03 E16 — the fighter's lighting moves onto the geometry engine

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** **BUILT, −35,072 FTR P50, geometry bit-identical, awaiting the
owner's visual approval.** `NDS_R2_FIGHTER_HW_LIGHT` defaults to 0 and requires
`NDS_R2_FIGHTER_HW_MTX` (E17), which is what puts the modelview rather than the
composed MVP into the vector matrix.

## 1. What shipped

The per-vertex software shade evaluated, for 484 vertices a frame at ~169 ticks
each:

```
colour = light_color_2 + (light_color_1 * dot(N, L)) / 127
```

That is term for term what the DS geometry engine computes from `GFX_NORMAL`,
on an engine E14 measured idle across all 946 fighter submissions. Four parts:

- **Load time.** `sNdsNativeFighterDenseNormals[]`, one `GFX_NORMAL` word per
  dense vertex, converted once from the source s8 normals (1.0 == 127) to the
  DS's 10-bit signed form (1.0 == 0x1ff).
- **Per owner execute.** `GFX_LIGHT_COLOR` = white, and `GFX_LIGHT_VECTOR` on the
  first epoch that has a direction (§3).
- **Per epoch.** One `GFX_DIFFUSE_AMBIENT`, carrying the source's two light
  colours folded with the epoch material and the damage-flash modulate —
  46.4 register writes a frame in place of 484 vertex shades.
- **Per vertex.** `GFX_NORMAL` instead of `GFX_COLOR`. One FIFO word either way.

Light 0's colour is white and the *source's* light colours become the material's
diffuse and ambient, so the engine evaluates the source equation exactly rather
than an approximation of it.

## 2. The mapping is exact, and E17 is what makes it so

With the DS's row-vector convention a normal submitted under vector matrix `V`
is dotted as `N.V.L_stored`; the software computes `N^T.M.L`. With `V = M` and
the light written while the vector matrix is identity, these are the same
product. E17 already loads `GL_MODELVIEW` in mode 2, which writes the position
*and* vector matrices — before E17 the vector matrix held the composed MVP, and
normals rotated by a projection are meaningless. **E16 was only ever buildable on
top of E17.**

## 3. E16a — measuring where each register write belongs

| counter | per frame | changes |
|---|---:|---:|
| lit epochs | 46.4 | |
| **light direction changes** | | **0 in 22,296 epochs** |
| light colour changes | | 14.8 (32%) |
| material changes | | 33.4 (72%) |

The direction never moves, so the light vector is written once per execute rather
than per epoch — which matters because `GFX_LIGHT_VECTOR` stores the vector
transformed by the vector matrix *at write time* and therefore has to be written
under an identity matrix. The colours and material move often, but both fold into
the one per-epoch `GFX_DIFFUSE_AMBIENT`, so neither needs its own write.

## 4. Result

Both arms `NDS_R2_FIGHTER_HW_MTX=1`, tick-HUD ROM, ring dump, 128 samples,
frames 439..566:

| bucket | A (E17 only) | B (E17+E16) | delta |
|---|---:|---:|---:|
| **FTR P50** | 489,536 | 454,464 | **−35,072** |
| WORK P50 | 1,099,328 | 1,063,360 | −35,968 |
| WORK-H P50 | 1,093,504 | 1,055,296 | −38,208 |
| VBlank histogram | 2:381 3:171 4:11 5+:3 | **2:418 3:139 4:6 5+:3** | |

The histogram is the part that matters: 37 more frames a window land in two
VBlanks instead of three.

**Structural check (E19's rule).** Control and candidate emit
`P0 = 181,440` / `P1 = 173,502` hardware triangles over the same window —
identical to the digit. E16 changes colour and nothing else.

**Engagement (E15's rule).** `gNdsR2LightVectorWrites = 1,114` over 557 frames:
exactly the 2-per-execute the design calls for.

## 5. The bug that cost most of this task, and the rule it produces

The first build drew the fighters as black silhouettes. Three bisects found it:

1. **Full-white diffuse, zero ambient → still black.** The dot product is zero.
2. **Constant normal `(0,0,511)` and constant light `(0,0,-511)` → still black.**
   A guaranteed-positive dot still produced nothing, so the fault was not the
   normal data or the light direction.
3. **Zero diffuse, full-white ambient → bright, textured fighters.** Ambient uses
   neither a normal nor a light vector, so `POLY_FORMAT_LIGHT0`,
   `GFX_DIFFUSE_AMBIENT`, `GFX_LIGHT_COLOR` and the `GFX_NORMAL` stream were all
   proven working, leaving only the light vector.

An engagement counter then showed the write running 928 times with the word
`0xE0100000` — where `NORMAL_PACK(0, 0, -511)` should be `0x20100000`.

**libnds's `NORMAL_PACK` does not mask its z argument.** It is
`(x & 0x3FF) | ((y & 0x3FF) << 10) | (z << 20)`, so a negative z sign-extends
into bits 30 and 31. Those bits are unused in `GFX_NORMAL` but are the **light
number** in `GFX_LIGHT_VECTOR`. Every light vector was being written to light 3,
which `POLY_FORMAT` never enables; light 0 kept its power-on zero vector, so
every dot product was zero and the fighters rendered with ambient only.
`NDS_R2_NORMAL_PACK` masks all three components.

Also fixed along the way: the first build converted only two of the **four**
production emit paths. `ndsRendererNativeEmitProductionPrimitiveGroups` and
`...CrossRun` were still writing `prepared->packed_color`, which the shade no
longer updates — so those runs drew with stale colour. This is E15's failure
shape again (a change applied to a path the production run does not take, or
here only part of the paths), and it is the second time in this campaign that
the four-way emit split has caught someone out.

## 6. Fidelity

Rendering-side. The DS light model works in RGB15 throughout where the software
path kept an RGB8 intermediate, so colours shift slightly; `PROJECT_GOAL.md`
lists "simplified lighting" among the explicitly allowed compromises. Geometry is
bit-identical (§4), so the only question is colour.

`artifacts/visibility/ClaudeOpus5_R203_E16_HwLight_candidate_20260728.png`.

**A frame-locked pair could not be produced.** Live captures drift because the
candidate runs faster and so reaches a later match clock at the same wall delay,
and the camera frames both fighters, so any position difference rescales the
whole shot. `capture-melonds.ps1 -ExactFirstFrame` is gated to the Cut G GO-text
window and refuses any other frame. **Recorded as a harness gap: there is no
general "capture both builds at presented frame N" mode, and every rendering-side
change from here needs one.**

## 7. Position

| cut | size | status |
|---|---:|---|
| E17 split matrix load | −17,600 | **GRADUATED**, in the published ROM |
| **E16 hardware lighting** | **−35,072** | built, geometry identical, **awaiting visual approval** |
| ~~E20 state-delta guard~~ | ~~25,000–30,000~~ | REFUTED (E21) |
| ~~E23 projection hoist~~ | — | REVERTED, sub-floor (E22) |

E17 and E16 together are **−52,672** against R2-03's 250,833 gap, or 21%. The
phase does not close on them. What remains after E16 is not another cut of this
kind: E22 showed the per-root bracket is 30 genuinely distinct matrix loads, E21
showed the state replay is not redundant, and E18 capped the shade at 53,760 of
which E16 now takes 35,072. **The rest of the gap needs a structural change to
what the fighter draw does, not a cheaper way of doing it.**
