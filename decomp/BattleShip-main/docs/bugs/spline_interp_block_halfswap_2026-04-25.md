# Spline Interp Data Halfswap — 2026-04-25

## Symptom

Samus's roll/dodge (`nFTCommonStatusEscapeF` / `nFTCommonStatusEscapeB`) plays the morph-ball animation but doesn't translate her position — fixed by the initial commit. After that landed, two follow-on bugs surfaced:

1. **Second roll didn't move** — first roll worked, subsequent rolls slid in place.
2. **Roll was slow and short** — first roll moved, but at half speed and half distance vs the original game.

Both reproduce in single-player gameplay and in the intro fight scene (`SSB64_START_SCENE=45`, OpeningJungle DK vs Samus).

## Root cause

Fighter figatree files go through two load-time passes:

1. `pass1_swap_u32` — blanket BSWAP32 of every u32.
2. `portRelocFixupFighterFigatree` — for figatree files only, halfswaps every non-token u32 slot. Halfswap = swap the two u16 halves of each u32. Required for AObjEvent16 streams (16-bit events packed two-per-u32).

`SetTranslateInterp` events point at a `SYInterpDesc` plus three full-width data tables (`Vec3f points[]`, `f32 keyframes[]`, `f32 quartics[]`) all living in the figatree file, so the halfswap pass touches them too. For full-width float data the halfswap is wrong — each f32's u16 halves get swapped, producing values like `9.6e9`, `1.27e17`, `-4.3e36` instead of the actual control-point coordinates.

`syInterpCubic(out, desc, t)` then walks `t` from 0→1 across the dodge frames but every interpolation result lands on garbage point data (typically the last point's bytes), so `out` (= `dobj->translate.vec.f`) stays constant. `ftPhysicsApplyGroundVelTransN` computes `vel_ground = (translate.now - translate.prev) * scale = 0` and the character doesn't move.

### SYInterpDesc header — also halfswapped

The header looks like this on N64 BE:

```
+0x00: u8 kind          /* nSYInterpKindLinear/Bezier/etc */
+0x01: 1-byte pad
+0x02: s16 points_num
+0x04: f32 unk04 (zero in practice)
+0x08: u32 points       /* PORT relocation token */
+0x0C: f32 length       /* spline arc length, used by cubic kinds */
+0x10: u32 keyframes    /* token */
+0x14: u32 quartics     /* token */
```

After pass1 BSWAP32 + halfswap on LE, the bytes at offsets 0..3 land in order `[pad, kind, points_num_lo, points_num_hi]`. The PORT struct as originally declared (`u8 kind; s16 points_num; ...`) reads:

- byte 0 as `kind` → gets the BE *pad* byte (almost always 0) → `kind` decodes to 0/Linear regardless of the real value
- bytes 2-3 as LE s16 `points_num` → gets the BE points_num low byte then high byte → decodes correctly **by coincidence of the BE u16 byte order matching the post-halfswap LE byte placement**

`length` at +0x0C is a pure halfswapped f32 — `0.846F` reads instead of the real `38.56F`.

Because `kind` defaulted to Linear, `syInterpCubicSplineTimeFrac` ran the linear-interp code path (straight lines between adjacent points) instead of the original `nSYInterpKindBezier` cubic path. Total endpoint displacement is the same, but the rate of motion-progress over time is very different — Linear distributes motion uniformly across keyframe segments while Bezier uses cubic-integral arc-length parameterization with `length` as the scale factor. Result: roll velocity ~half of what the original game produces.

### Cubic kinds need phantom control points

The Bezier / Catrom paths read `points[target_frame .. target_frame+3]` (sliding window of 4 control points). With `target_frame` walking `[0, points_num-2]`, the highest accessed index is `points_num+1`. The original IDO data layout allocates `points_num + 2` control points (verified empirically: a Samus EscapeF spline with `points_num=5` has 7 Vec3f between `desc->points` and `desc->keyframes` = 84 bytes). Un-halfswapping only `points_num` Vec3f leaves the trailing phantom control points still halfswapped, which produces exponentially-growing translate values when `target_frame` walks past the un-fixed boundary.

### Visited-set vs heap reuse

`ftMainSetStatus` calls `lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, fp->figatree_heap)` on every status change — every motion's figatree DMAs into the same per-fighter heap address. After the first dodge, my visited set has the spline-data addresses. After the next motion overwrites the heap with fresh halfswapped bytes at the same addresses, the second dodge's syInterpGet* calls find the addresses already-visited and skip un-halfswap → garbage.

## Fix

`src/sys/interp.c` + `src/sys/interp.h` + `port/port_aobj_fixup.{h,cpp}`:

1. **Data blocks** — un-halfswap on first access in `syInterpGetPoints` / `syInterpGetKeyframes` / `syInterpGetQuartics`, gated on `port_aobj_is_in_halfswapped_range(p)` so non-figatree-derived interp blocks pass through untouched.

2. **Bezier phantom control points** — `port_interp_points_u32_count(desc)` returns `(points_num + 2) * 3` u32 for non-Linear kinds, `points_num * 3` for Linear. Covers the trailing phantoms so cubic sliding-window reads land on un-halfswapped data.

3. **SYInterpDesc layout** — added `#if IS_BIG_ENDIAN` branch in `interp.h` that swaps `kind` and the implicit pad on LE: `{u8 _pad0; u8 kind; s16 points_num;}` instead of `{u8 kind; s16 points_num;}`. This makes the LE struct read `kind` from the byte that *post-halfswap* holds the original BE kind value. A later Sector Z fix normalized non-figatree descriptors into the same byte order on first access; see `sector_arwing_interp_desc_byteswap_2026-04-28.md`.

4. **length f32** — `port_unhalfswap_interp_desc` un-halfswaps the u32 word at offset 0x0C on first access. Same idempotency-via-visited-set as the data blocks.

5. **Heap-reuse idempotency** — moved the visited set into `port_aobj_fixup` (which already owns `port_aobj_register_halfswapped_range`). Whenever a new range gets registered (= figatree heap reload), evict any visited-set entries that fall inside it. Subsequent un-halfswap calls then re-fix the fresh halfswapped bytes.

Verification (intro scene, scene 45, 5000 frames, Samus EscapeF):

| metric                       | before fix      | after data-block-only fix | after full fix (this commit) |
|------------------------------|-----------------|---------------------------|------------------------------|
| `desc.kind`                  | 0 (Linear)      | 0 (Linear, wrong)         | **2 (Bezier, correct)**      |
| `desc.length`                | 0.846F          | 0.846F (wrong)            | **38.56F (correct)**         |
| TransN.translate constant    | yes (9.6e9)     | no                        | no                           |
| `vel_ground.x` steady-state  | 0               | ~18                       | **~36.5 (doubled)**          |
| total ground distance        | 0               | ~470 units                | **~1067 units**              |
| second roll moves            | n/a             | no                        | **yes**                      |

## Class

Same family as `aobjevent32_halfswap_2026-04-18` — figatree halfswap is correct for u16-packed events, wrong for full-width data living in the same file. Same family as `fixup_idempotency_heap_reuse` — bump-reset heaps re-DMA fresh halfswapped bytes at addresses our visited set already remembered.

Look for this pattern any time figatree-loaded data is read as float/Vec3f/u32 array and produces values with implausible exponents, constant-across-frames behaviour, or works once but breaks after a status change.

## Files touched

- `src/sys/interp.h` — `#if IS_BIG_ENDIAN` branch on `SYInterpDesc` swapping `kind`/pad order on LE.
- `src/sys/interp.c` — un-halfswap helpers (`port_unhalfswap_interp_desc`, `port_unhalfswap_block`), `port_interp_points_u32_count` covering Bezier phantom control points, accessor wrappers.
- `port/port_aobj_fixup.h`, `port/port_aobj_fixup.cpp` — `port_aobj_unhalfswap_visit` shared visited set with eviction-on-range-add.
