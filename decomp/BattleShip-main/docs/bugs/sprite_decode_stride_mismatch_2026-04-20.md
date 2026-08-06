# Sprite Decode Stride vs. Clamped Width Mismatch (2026-04-20) — FIXED

**Symptom:** Heavy diagonal shear across a specific class of sprites — every
row progressively shifted horizontally relative to the one above, so the
image read as "sheared" even though the colors were right. Affected at
least:

| Texture                               | File | Format   | Scene |
|---------------------------------------|------|----------|-------|
| Character-select portraits            | 19   | RGBA32   | `mnplayers*` — 1P, VS, training |
| Fighter-select "puck" (slot glow)     | 17   | RGBA16   | `mnplayers*` |
| 1P map wallpaper (top)                | varies | RGBA16 | `grWallpaperMakeCommon` stage wallpapers |
| Tutorial "How to Play" banner         | 198 (0xC6) | CI8  | Explain scene (`scexplain.c`) |
| Tutorial textbox                      | 198  | CI8      | Explain scene |
| Tutorial "knocked off the stage" text | 198  | CI8      | Explain scene |
| Tutorial "HereText" red arrow         | 198  | RGBA16   | Explain scene |

Visually identical across the set — a clean diagonal ribbon pattern tearing
each sprite. Separate from the SMASH / fighter-portrait 32bpp swizzle class
(see [sprite_32bpp_tmem_swizzle](sprite_32bpp_tmem_swizzle_2026-04-20.md));
that one causes a qword-granularity scramble rather than a progressive
shear, and has its own fix.

## Root cause

Fast3D's `ImportTexture*` decoders in `libultraship/src/fast/interpreter.cpp`
are structured as:

1. Derive `widthBytes` from the current tile's `line_size_bytes` (i.e. the
   row stride set by `gDPSetTile`, computed from `tex_width`).
2. Compute `width = widthBytes / bpp_factor`.
3. Clamp `width` / `height` by the tile's SetTileSize `lrs/ult` extent and
   by the `masks` / `maskt` wrap bounds.
4. When the LoadBlock was flat (`fullImageLineSizeBytes == sizeBytes`, the
   default from `gDPSetTextureImage(..., width=1, ...)` that all sprites
   use), reassign `fullImageLineSizeBytes = width * bpp_factor`.
5. Decode iterates `y ∈ [0, height), x ∈ [0, width)`, source-indexed by
   `y * (fullImageLineSizeBytes / bpp_factor) + x`.

Step 4 is the bug. When the clamps in step 3 tighten `width` below the
TMEM-line-derived natural width (which happens for any sprite with
`bitmap->width < bitmap->width_img`), the reassigned
`fullImageLineSizeBytes` becomes smaller than the real DRAM row stride.
The decode then reads row `y+1`'s pixels from `y*width` bytes into the
source instead of `y*naturalWidth` bytes — a progressive horizontal shift
that compounds across rows → classic diagonal shear.

The character-select portrait case is the canonical example: the Mario
portrait is `bitmap->width = 46`, `bitmap->width_img = 48` (because
`width_img` rounds up to a qword boundary in 32bpp storage), so `tile_w`
clamps `width` from 48 to 46 and the reassignment pulls rows from stride
184 bytes instead of the actual 192.

## Why IA8 / I4 weren't affected

The [2026-04-14 title-border fix](title_border_right_edge_slice_2026-04-14.md)
had already landed `ClampUploadWidthToTile` in `ImportTextureIA8` and
`ImportTextureI4`, which encodes the exact distinction needed here:
`naturalWidth` (pre-clamp) is the source stride for decode indexing;
`width` (post-clamp) is the extent of the decode loop. That fix never
made it to the other five decoders.

## Why `ImportTextureCi4` wasn't affected either

CI4 accidentally does the right thing — it names its natural width
`resultLineSizeBytes`, clamps a separate `width` variable, and uses the
unclamped `resultLineSizeBytes` in the `fullImageLineSizeBytes`
reassignment. The variable naming just happened to preserve the invariant.

## Why `ImportTextureCi8` was *also* broken, and needed a bigger rewrite

CI8 had a second, more severe bug stacked on top: its decode loop ran
*before* width/height were computed, iterating `for k in [0, lineSizeBytes)
... mTexUploadBuffer[i++]`. The result was a packed buffer that had
`lineSizeBytes` pixels per row — but then `UploadTexture(width, height)`
was called with post-clamp `width < lineSizeBytes`, so each upload row
read from the wrong offset inside the buffer.

Fixed by moving the whole width/height/clamp computation *above* the
decode and replacing the `i,j,k` loop with the standard
`for y in [0, height) for x in [0, width)` pattern using
`fullImageLineSizeBytes` as the source stride (same pattern as every
other `ImportTexture*`).

## Fix

`libultraship/src/fast/interpreter.cpp`:

- `ImportTextureRgba16` — introduce `naturalWidth = widthBytes / 2` before
  the clamps; reassign `fullImageLineSizeBytes = naturalWidth * 2`.
- `ImportTextureRgba32` — same with `widthBytes / 4` and `naturalWidth * 4`.
- `ImportTextureCi8` — moved width/height + tile_w/tile_h/mask clamps
  above the decode; replaced the `i,j,k` decode with `for y in [0,height)
  for x in [0,width)` using `fullImageLineSizeBytes` as stride; added the
  `fullImageLineSizeBytes == sizeBytes → naturalWidth` reassignment that
  was previously absent from CI8.

`port/bridge/lbreloc_byteswap.cpp`:

- `portFixupSpriteBitmapData` now reads `bitmap->width` (offset 0) in
  addition to `width_img` (offset 2) and includes both in the
  `SSB64_TEX_FIXUP_LOG=1` output as `w=<bitmap.width> wi=<bitmap.width_img>`,
  so future "is the clamp firing?" questions can be answered from a
  single session log.

## When the fix is a no-op

For sprites with `bitmap->width == bitmap->width_img` (the common case),
the clamp never fires, `width == naturalWidth`, and the decode produces
byte-identical output. The fix is strictly a correctness improvement —
no behavioural change for previously-working sprites.

## Files

- `libultraship/src/fast/interpreter.cpp` — decoder fixes.
- `port/bridge/lbreloc_byteswap.cpp` — log extension.
- `docs/sprite_broken_textures_handoff_2026-04-19.md` — fully resolved
  (paired with the 32bpp swizzle fix, every texture listed in that handoff
  is now accounted for).

## Class-of-bug lesson

When several texture formats share a near-identical decode structure but
only *some* have been patched, treat the unpatched ones as latent bugs
waiting for a sprite whose `tex_width` rounds up to a different
`width_img`. The ClampUploadWidthToTile distinction (natural source stride
vs. clamped decode extent) applies to every `ImportTexture*` — the ones
that happen to work right now are either using unclamped natural widths
as source strides by coincidence, or have no clamp logic at all (IA4,
IA16, I8).
