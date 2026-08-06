# Broken Sprite Textures — Investigation Handoff

**Date:** 2026-04-19
**Status:** **RESOLVED (2026-04-20)** — every texture in the table is
fixed by one of two landed commits; there are no remaining items from
this investigation.

Two root causes, not one:

- **32bpp TMEM-line-swizzle granularity** — items 1 (SMASH logo) and the
  32bpp portion of 5 (character portraits). Swap granularity had to move
  from 8-byte-qword 4-byte halves (4b/8b/16b) to 16-byte-group 8-byte halves
  for 32bpp, because `G_IM_SIZ_32b_LOAD_BLOCK = G_IM_SIZ_32b` splits each
  RGBA32 texel across two TMEM words via the low/high bank layout. Writeup:
  [docs/bugs/sprite_32bpp_tmem_swizzle_2026-04-20.md](bugs/sprite_32bpp_tmem_swizzle_2026-04-20.md).

- **Fast3D decode stride vs. clamped width** — items 2–4 (tutorial "How to
  Play" banner + textbox + HereText arrow), 5 (character portraits — the
  diagonal shear that remained after the 32bpp fix was this class of bug),
  and 6 (the 2P/3P/4P "player card" horizontal stripes turned out to be the
  same bug, not a distinct issue). `ImportTextureRgba16` /
  `ImportTextureRgba32` / `ImportTextureCi8` were reassigning
  `fullImageLineSizeBytes` from the *clamped* width when SetTileSize made
  `tex_width < width_img`, and CI8 additionally ran its decode loop before
  the clamp. Writeup:
  [docs/bugs/sprite_decode_stride_mismatch_2026-04-20.md](bugs/sprite_decode_stride_mismatch_2026-04-20.md).

Worth flagging for future investigations: the original handoff table listed
the portrait file as 168, which is actually `llMNTitleFireAnimFileID`
(literal fire animation). The real portraits are
`llMNPlayersPortraitsFileID = 0x13 = 19` — 12 × (48×21 + 48×21 + 48×3)
RGBA32 strips. The earlier "TGA dumps look like fire" session had found the
right data, just at the wrong file ID.

---

## (original, partially superseded) Status: OPEN — root cause not found; two fixup approaches were ineffective

## Affected Textures

All broken textures show the same symptom: fine horizontal combing or diagonal stripe artifacts. The colors and shapes are correct but the scanline-level pixel positions are scrambled.

| # | Texture | Scene | Format | File ID | Notes |
|---|---------|-------|--------|---------|-------|
| 1 | SMASH title logo | Title screen | RGBA32 (bmsiz=3) | 167 | 21 strips, w=172 h=4 each |
| 2 | "How to Play" banner | Tutorial (Explain scene) | CI8 (bmsiz=2) | 198 | Black/white horizontal banding |
| 3 | HereText indicator | Tutorial | RGBA16 | 198 | Small red indicator, scrambled |
| 4 | Tutorial textbox | Tutorial | CI8 (bmsiz=2) | 198 | Green horizontal combed stripes |
| 5 | Character portraits | Fighter-select screen | RGBA32 (bmsiz=3) | 168 | 32×32 portraits, diagonal stripes |
| 6 | Player card (2P/3P/4P) | Fighter-select screen | Unknown | Unknown | Horizontal stripes |

The artifact pattern on SMASH and portraits is visually identical to what other sprites showed before the XOR-4 TMEM deswizzle fix was applied.

## What Is Already Working

The XOR-4 deswizzle fix in `portFixupSpriteBitmapData` is applied for `bpp < 32` (4b, 8b, 16b sprites, excluding 4c compressed pre-decode). This fixed the N64 logo, background wallpaper, and all other title-screen sprites. Those are not regressed.

## Confirmed Facts

### SMASH (file 167, RGBA32)

- `portFixupSpriteBitmapData` **IS** called — confirmed by `SSB64_TEX_FIXUP_LOG=1` output (all 21 bitmaps appear as `[sprite.bitmap]` entries).
- BSWAP32 is applied correctly. Bytes like `FF001100` = R=255,G=0,B=17,A=0 are in correct N64 BE order.
- XOR-4 deswizzle is **not** applied to 32bpp. This is correct per hardware: `gbi.h` defines `G_IM_SIZ_32b_LOAD_BLOCK = G_IM_SIZ_32b` (not `G_IM_SIZ_16b`), so 32bpp textures are loaded differently and are not DRAM pre-swizzled the same way.
- `line_size *= 2` fix for G_IM_SIZ_32b is present in `interpreter.cpp:2149-2152` and computes correct tile dimensions (172×4).
- Visual pattern still matches "XOR-4 not applied" artifact. Contradiction with the hardware path analysis is unresolved.

### Tutorial textures (file 198, CI8 + RGBA16)

- No `SSB64_TEX_FIXUP_LOG=1` data captured for these — the game was not navigated to the tutorial screen during log collection.
- Cannot confirm whether `portFixupSpriteBitmapData` is being called for these textures.

### Character portraits (file 168, RGBA32)

- `portFixupSpriteBitmapData` **IS** called — file=168 appears in the `SSB64_TEX_FIXUP_LOG=1` output as `[sprite.bitmap]` entries.
- Created via `lbCommonMakeSObjForGObj` in `mnPlayers*MakePortrait` functions (`src/mn/mnplayers/`).
- Same broken diagonal stripe pattern as SMASH.

## What Was Tried (Both Ineffective)

### Fix A: Separate tracking sets for struct vs. buf addresses

**Theory:** `portFixupSprite` and `portFixupBitmap` use `sStructU16Fixups` for idempotency keyed on struct base address. Since `sStructU16Fixups` also keys buf addresses for BSWAP tracking, a coincidence between a struct pointer and a buf address could suppress a BSWAP.

**Action:** Added `sSpriteTouched` and `sBitmapStructTouched` sets, removed sprite/bitmap struct addresses from `sStructU16Fixups`.

**Result:** No visible change. **Reverted to HEAD.**

### Fix B: Apply XOR-4 deswizzle to RGBA32

**Theory:** Despite `G_IM_SIZ_32b_LOAD_BLOCK = G_IM_SIZ_32b`, the data may still be DRAM pre-swizzled.

**Action:** Removed `bpp < 32` guard from deswizzle condition in `portFixupSpriteBitmapData`.

**Result:** No visible change. Confirmed analytically incorrect — applying XOR-4 swaps adjacent RGBA32 pixel pairs (2 pixels per 8-byte qword), which an earlier empirical test showed reverses the gradient on odd rows. **Reverted to HEAD.**

## Current Code State

All code is at HEAD. No pending changes in `port/bridge/lbreloc_byteswap.cpp` or `libultraship/`.

## Debug Data Available

- `debug_traces/tex_fixup.log` — 88,938 lines from a title-screen session. Contains file=167 (SMASH) and file=168 (portraits). Does NOT contain file=198 (tutorial not visited).
- `debug_traces/sprite_deswizzle/` — XOR variant comparison images for Mario/CharName sprites (these are already-fixed textures from a prior session, not the broken ones).

## Recommended Next Steps

1. **Capture fresh `SSB64_TEX_FIXUP_LOG=1` log covering tutorial and fighter-select screens.** Navigate: title → character select → tutorial. Check file=198 entries — are `[sprite.bitmap]` entries present? What are the bpp, w, h, bswap_skipped, and first16 values?

2. **Capture `SSB64_GBI_TRACE=1` for a SMASH render frame.** Compare the SetTile/LoadBlock/SetTileSize sequence against `src/lb/lbcommon.c:2629-2675` (the expected GBI sequence). Verify the tile's siz, line, lrs, dxt values match expectations at render time.

3. **Instrument `ImportTextureRgba32` in interpreter.cpp.** Log the effective line size, computed width/height, and first few texel bytes on every call for file=167 bitmaps. Confirm width=172, height=4 for all 21 strips.

4. **Check UV/sampler path for RGBA32.** The combing could be a UV normalization issue rather than a texel data issue — if tex_width or tex_height is computed incorrectly in the rendering path (not just in the SetTile dimension path), adjacent rows could appear interleaved.

5. **Check whether tutorial CI8 textures are created via a different path** than `lbCommonMakeSObjForGObj`. If `portFixupSpriteBitmapData` is not being called for file=198, that is the root cause for textures 2-4 above.

## Key Files

- `port/bridge/lbreloc_byteswap.cpp` — `portFixupSpriteBitmapData` (BSWAP + deswizzle)
- `src/lb/lbcommon.c:2629-2675` — `lbCommonDrawSObjBitmap` 32bpp GBI sequence
- `src/lb/lbcommon.c:3033-3050` — `lbCommonMakeSObjForGObj` (calls fixup chain)
- `src/mn/mnplayers/` — portrait creation (`mnPlayers*MakePortrait`)
- `src/mn/mncommon/mntitle.c` — SMASH sprite creation
- `libultraship/src/fast/interpreter.cpp:863-912` — `ImportTextureRgba32`
- `libultraship/src/fast/interpreter.cpp:2143-2173` — tile dimension computation with `line_size *= 2`
- `include/PR/gbi.h:455,460` — `G_IM_SIZ_32b_LINE_BYTES=2`, `G_IM_SIZ_32b_LOAD_BLOCK=G_IM_SIZ_32b`
