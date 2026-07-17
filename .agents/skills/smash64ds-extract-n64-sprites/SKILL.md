---
name: smash64ds-extract-n64-sprites
description: Extract source-faithful BattleShip O2R Sprite art, diagnose SP_TEXSHUF combing or interleaving, and bake clean Nintendo DS RGB555+A1, A3I5, or A5I3 assets. Use for clean source PNG requests, countdown/GO/traffic-light/flare fidelity, shuffled N64 texture rows, or DS-ready sprite conversion.
---

# Smash64DS N64 Sprite Extraction

Extract first, then convert. Never blur, redraw, or use AI to hide a decode error.

## Decode the source

1. Treat `decomp/` as read-only and inspect the BattleShip draw path, Sprite
   attributes, format, size, Bitmap strips, `width_img`, and `bmheight`.
2. Verify the authoritative O2R file hash before reading relocated data.
3. Undo `SP_TEXSHUF` on odd rows with the format's TMEM word swap:

   - 4-bit: `x ^ 8`
   - 8-bit: `x ^ 4`
   - RGBA16: `x ^ 2`
   - RGBA32: `x ^ 2`

4. Export an exact transparent PNG before resampling. A combed, dithered, or
   interlaced-looking result means the deinterleave is wrong; do not smooth it.

## Bake for Nintendo DS

- Big GO: resample once in premultiplied space, threshold alpha at 112, pack
  little-endian direct RGB555+A1 (`0x8000 | r5 | g5<<5 | b5<<10`), and use
  integer-positioned bitmap OAM. Do not use A3I5 color quantization or draw-time
  antialiasing for the letters.
- Traffic housing and dim lamps: preserve source intensity as RGB shading, use
  a hard opaque cutout mask, and keep the A3I5 hardware quad behind the flare.
- Flare/core/contour: retain source intensity as graded A5I3 alpha and queue it
  after the housing. The traffic box is not translucent; only the flare is.

## Verify

Run:

```powershell
python scripts/check_ifcommon_hybrid_oam.py --export-source
```

Require the source O2R hash, prepared-byte parity, pinned RGB555 payload hash,
zero gameplay conversion/upload, and one exact-ROM capture. Compare the source
PNG, DS atlas preview, and device capture before accepting a visual change.
