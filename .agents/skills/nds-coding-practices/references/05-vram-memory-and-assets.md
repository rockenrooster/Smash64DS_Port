# 05 — VRAM, textures, and alpha representation

Declare one bank/allocation owner and a scene admission map before uploading. Account texture bytes, palette bytes, required alignments/bank roles, handle slots, staging, and peak overlap separately. A texture allocation can fail while aggregate VRAM appears free. Do not remap a bank or free/reuse a palette while displayed content still references it.

## Native GX alpha contract

| libnds format | Color/alpha representation | Required treatment |
|---|---|---|
| `GL_RGBA` | Direct RGB15: R bits 0–4, G 5–9, B 10–14; alpha bit 15 | Binary texel alpha. Preserve bit 15; do not use for unapproved graded-alpha reduction. |
| `GL_RGB` | Same uploaded direct format, but libnds forces bit 15 on | **Opaque-only upload. Never use for cutouts.** |
| `GL_RGB4`, `GL_RGB16`, `GL_RGB256` | 2/4/8-bit indices; palettes are RGB15, not RGBA5551 | Index 0 is transparent only with `GL_TEXTURE_COLOR0_TRANSPARENT`. Reserve/remap it deliberately; other palette alpha bits cannot create holes. |
| `GL_RGB32_A3` | Low 5 bits color index, high 3 bits alpha | 32 RGB colors and 8 alpha codes per texel. Index 0 can be opaque; color-zero flag is not this alpha mechanism. |
| `GL_RGB8_A5` | Low 3 bits color index, high 5 bits alpha | 8 RGB colors and 32 alpha codes per texel. Keep alpha in texels, not palette entries. |
| `GL_COMPRESSED` | 4×4 blocks, mode-dependent palette/interpolation and secondary index data | Some modes support binary transparency. Verify block modes and decoded mask; not a general graded-alpha format. |

For ordinary indexed cutouts, map **all source transparent texels** to reserved index 0 and every opaque texel—including opaque black or source index 0—to a nonzero entry. Capacity is then 3/15/255 distinct opaque colors after the chosen RGB quantization. Fail or select another representation when it does not fit; do not silently erase colors. Palette animation needs stable remaps or regenerated/versioned texels whenever alpha membership changes. OAM rules differ; see [06](06-2d-video-bg-oam.md).

Do not infer alpha from RGB color or index number in source data. N64 TLUT alpha may be at any entry; intensity may feed alpha through the combiner. The companion's material chapter owns source decoding. Ordinary DS palette bit 15 is **not** a portable alpha channel. Native 4-bit packing places the first pixel in the low nibble; do not copy a normalized N64 high-nibble-first stream unchanged.

## Conversion and upload checks

Preserve source/material provenance, dimensions, layout, alpha class (`opaque`, `cutout`, `graded`), transparent count/mask hash, and generation. Compare an independent decode of native output with the intended alpha. Quantization or alpha-test threshold must be explicit and tested, not an implicit `alpha != 0`. Choose a format that fits the actual color/alpha requirements before quantizing.

Use appropriate zero-alpha padding around cutout atlas regions; retain the coordinate origin, edge sampling, wrap/mirror/clamp and subtexture boundaries. Color dilation may reduce filtered fringes while leaving alpha zero, but must not change coverage. Do not assume desktop filtering or an N64 tile maps to a whole DS texture.

Check converter options, upload type, allocated size, palette upload, and return/status values. The pinned `glTexImage2D()` takes `GL_TEXTURE_SIZE_ENUM` dimensions such as `TEXTURE_SIZE_8`, not pixel counts; verify installed signatures. `glColorTableEXT()` is void in this baseline, so verify palette allocation through the supported query/ownership layer rather than inventing a bool return.

Cache identity includes image/palette generations, format, dimensions, tile view and alpha policy. Binding state includes texture parameters **including color-zero transparency**, palette binding, polygon mode and alpha. Changing repeat flags with `glTexParameter()` must preserve all required flags. Raw MMIO mixed with libnds cached state can make a later same-ID bind ineffective.

Correct bytes do not prove correct rendering: validate the material and global raster state in [07](07-3d-gx.md). Sources: [SOURCES](SOURCES.md); executable semantic checks live in the companion's `tests/test_texture_alpha.py`.
