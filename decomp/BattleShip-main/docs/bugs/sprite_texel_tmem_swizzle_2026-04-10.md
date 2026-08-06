# Sprite Texel Byteswap + TMEM Line Swizzle (2026-04-10) — FIXED

**Symptoms:** All sprite-rendered content (N64 logo, HUD, menus) had garbled textures on the port — colors were roughly right but the image had a sheared/zigzag/sawtooth pattern. Letters were unreadable. The 3D N64 logo's diagonals stairstepped wrong. Looked like a Fast3D rendering bug or texture filter issue.

**Diagnostic path:**
1. Captured GBI traces from both port and Mupen64Plus (via the `mupen64plus-rsp-trace` plugin) and diffed with `debug_tools/gbi_diff/gbi_diff.py`. Across the entire N64 logo scene (54 frames) the port emitted **byte-perfect identical** GBI commands (0 opcode/w0/w1 mismatches with `--ignore-addresses`). That ruled out game code's display list construction.
2. Logged the actual texel bytes for `bitmap[0].buf` at runtime in `lbCommonMakeSObjForGObj` and compared against the raw .o2r file. After the first BSWAP32 fix the bytes matched the file but the rendered image was still wrong.
3. Forced point sampling and 4x internal resolution — neither helped, ruling out filter/upscale.
4. Found a real but unrelated DX11 filter-default bug (`gfx_direct3d_common.h` defaulted `mCurrentFilterMode = FILTER_NONE` so `linear_filter && mCurrentFilterMode == FILTER_LINEAR` was always false → DX11 always point-sampled regardless of game request). Fixed independently by changing the default to `FILTER_LINEAR`.
5. Compared mupen64plus screenshot (with glide64mk2) — clean and crisp — with the port. The port matched the .o2r contents exactly, but the .o2r contents themselves were wrong relative to mupen.
6. Verified Torch's vpk0 decompression by comparing ROM-decompressed bytes vs .o2r contents — perfect match. So Torch wasn't corrupting data; the issue was data interpretation.
7. Extracted the texture as a Python PNG and tested several swap patterns. Found that **XOR4 on odd rows (strip-local)** produces a clean, mupen-matching image: `debug_traces/logo_xor4_strip_local.png`.

**Root causes (two stacked issues):**

1. **Pass2 doesn't see sprite textures.** `portRelocByteSwapBlob` pass2 finds in-file textures by walking stored display lists for `G_SETTIMG` → `G_LOADBLOCK` pairs. Sprite files (e.g. `N64Logo`) build their LOAD blocks at **runtime** in `lbCommonDrawSObjBitmap` from `bitmap.buf` — those addresses never appear in any stored DL. Pass2 misses the texel data and it's left in pass1's u32-byteswapped state. Fast3D's `ImportTextureRgba16` reads texels as N64 BE u16 (`(addr[0] << 8) | addr[1]`), so it needs the bytes in original BE order — another BSWAP32 restores them.

2. **N64 RDP TMEM line swizzle.** Even with bytes in correct BE order, sprite textures still rendered with a shear because the N64 stores RGBA16 textures in DRAM **pre-swizzled** to avoid TMEM bank conflicts when sampled. The hardware XORs the byte address based on row parity: for 16bpp/IA/CI, odd rows have the byte address XOR'd with 0x4 (i.e., the two 4-byte halves of each 8-byte qword are swapped). `LOAD_BLOCK` with `dxt=0` loads the swizzled data into TMEM as-is; the sampler unscrambles it during reads. Fast3D doesn't emulate TMEM addressing, so it would render the swizzled data linearly and produce a sheared image.

**Fix:** Added `portFixupSpriteBitmapData(sprite, bitmaps)` in `port/bridge/lbreloc_byteswap.cpp`. Walks the bitmap array, resolves each `bm->buf` via `portRelocResolvePointer`, then per bitmap:
1. Apply BSWAP32 over `width_img × actualHeight × bpp / 8` bytes to restore N64 BE byte order.
2. For 16bpp textures, apply the inverse TMEM line swizzle: on every odd row of the bitmap, swap the two 4-byte halves within each 8-byte qword. Strip-local row indexing (each LOAD_BLOCK is independent in TMEM).

Idempotent via the existing `sStructU16Fixups` tracking set. Called from `lbCommonMakeSObjForGObj` right after `portFixupBitmapArray`. Sprite struct field offsets read directly with hard-coded byte offsets — `bmsiz` is at `0x31` (not `0x32`): the C struct packs `u8 bmfmt; u8 bmsiz;` followed by 2 bytes of padding before the next u32, and the field offset must match the C compiler's natural placement.

**Files:**
- `port/bridge/lbreloc_byteswap.cpp` — `portFixupSpriteBitmapData`
- `port/bridge/lbreloc_byteswap.h` — declaration + doc
- `src/lb/lbcommon.c` — call site in `lbCommonMakeSObjForGObj`
- `libultraship/include/fast/backends/gfx_direct3d_common.h` — separate DX11 filter-default fix (`FILTER_NONE` → `FILTER_LINEAR`); was hiding the bug because every sample was point-sampled regardless of game request.

**Latent issues not yet addressed:**
- `apply_fixup_tex_u16` in pass2 uses `rotate16` for in-file 16bpp textures and palettes, which is wrong for the same reason — Fast3D reads textures as BE u16. Most game textures are CI4/CI8 (which use `apply_fixup_tex_bytes` = bswap32 = correct) or 32bpp, so it hasn't surfaced. If any non-sprite RGBA16 textures or LUT palettes show shear, that's the same bug class.
- The TMEM line swizzle applies to **all** 16bpp textures the RDP samples, not just sprites. If other rendering paths load 16bpp via `LOAD_BLOCK` with `dxt=0` and aren't sprite-routed, they'll need the same unswizzle.
