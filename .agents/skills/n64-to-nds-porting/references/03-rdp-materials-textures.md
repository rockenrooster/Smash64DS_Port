# 03 — Preserve texture and sprite alpha end to end

An opaque quad can originate in decoding, material lowering, native packing, upload or draw state. **Do not assume it is the image converter.** Trace:

`source bytes + effective tile/TLUT -> decoded RGBA -> source color/alpha equations -> native texels/palette -> upload -> GX/BG/OAM state -> visible coverage`

Record the material/asset ID, source state provenance, alpha class, dimensions, generation, transparent count/mask and intended blend/depth behavior. Probe each boundary; once native/uploaded alpha is correct, inspect material state rather than repeatedly rewriting the decoder. [Source contracts](SOURCES.md); DS format/state details are in the companion's references 05–07.

## Decode the actual source format

| N64 format | Alpha information to preserve |
|---|---|
| RGBA16 | Big-endian RGBA5551: R bits 11–15, G 6–10, B 1–5, A bit 0. |
| RGBA32 | Separate 8-bit R/G/B/A channels; reconstruct the actual TMEM/load layout before treating it as linear bytes. |
| CI4/CI8 | Index selects the effective TLUT entry. TLUT is RGBA16 **or IA16** according to state; any index can carry alpha, including 15/255. |
| IA4 | 3-bit intensity + 1-bit alpha. |
| IA8 | 4-bit intensity + 4-bit alpha. |
| IA16 | 8-bit intensity + 8-bit alpha. |
| I4/I8 | Intensity is available as RGB **and alpha**; the combiner determines whether that alpha is used. Do not force it opaque. |

Source index 0 is not inherently transparent. Multiple transparent entries and an opaque index 0 are valid. Follow TLUT/tile state through caller, root and child lists; material extraction must not discard root-level palette loads. Test the resolved state at the draw, not merely the presence of a material branch. A CI4 bank and the TLUT mode must be resolved before lookup. Distinguish logical linear texels from N64 load/TMEM packing; normalized high-nibble-first CI4 is not native DS low-nibble-first packing.

The [host alpha helper](../tools/texture_alpha.py) decodes tightly packed **normalized** rows/streams, not raw ROM/TMEM. Its RGBA5551 bit conversion agrees with [n64_data.h](../examples/n64_data.h). Neither function decides combiner behavior or spatial sampling.

## Derive color and alpha separately

Resolve inherited cycle type, both combiner equations/inputs, primitive/environment/shade values, textures, alpha compare, blender/coverage and depth state. N64 RGB and alpha use separate `(A-B)*C+D` equations. A texture can contain alpha that the source intentionally ignores; conversely an intensity texture can drive a mask. Preserve the **effective result**, not every raw source alpha bit unconditionally.

Names are not equations: N64 `DECALRGBA` outputs texture alpha, but DS `POLY_DECAL` uses texture alpha to mix RGB and takes output alpha from the polygon. Mapping them by name can create a fully filled quad. For plain texture RGB/alpha, white vertex color plus a validated DS modulation recipe is the appropriate candidate—not a universal lowering for all combiners.

Bake only invariant terms. Live primitive/shade alpha, palette animation, color tracks, UVs, texture selection and per-vertex values need typed bindings or another faithful native representation. GX polygon alpha is per-polygon; a varying source vertex-alpha field is not automatically representable by one value. Reject or explicitly implement the unsupported case. Multipass and quantization are not automatically source-equivalent.

## Choose a native representation

| Required result | Candidate and acceptance rule |
|---|---|
| Binary direct-color mask | DS `GL_RGBA`; rearrange source RGB bits and move source A bit 0 to DS bit 15. A byte swap alone is wrong. |
| Binary indexed mask | `GL_RGB4/16/256` plus reserved index 0 and `GL_TEXTURE_COLOR0_TRANSPARENT`; **remap texels**, not just palette alpha. |
| Graded per-texel alpha | `GL_RGB32_A3` (A3I5) or `GL_RGB8_A5` (A5I3), with explicit color/alpha quantization limits and matching blending. |
| Block-compressed cutout | DS 4×4 only with a validated transparency-capable block mode and matching palette/index data. |
| Native tiled OBJ | Transparent index 0 intrinsically; no GX color-zero flag. Match OAM layout and blend mode. |
| Bitmap OBJ / mixed 2D+3D | Use that hardware path's visibility/alpha/layer rules, not a GX palette recipe. |

DS ordinary palettes are **RGB15**, not per-entry RGBA. Reserve native index 0 for all transparent texels; move opaque source index 0 and opaque black to nonzero entries. With zero reserved, capacity is 3/15/255 distinct opaque RGB colors. Merge only when the chosen quantization and animation semantics permit; otherwise choose a larger/different format or fail. An animated palette can require texel remapping when transparent membership changes. Cache/version the mapping and palette together.

A3I5/A5I3 alpha lives in each texel's high bits, independently of palette color. Color index 0 can be opaque. Do not apply index-zero remapping rules mechanically to these formats. Graded-to-binary thresholding loses information and requires explicit acceptance; “nonzero means opaque” is not a neutral conversion.

**Never upload a cutout with `GL_RGB`: pinned libnds sets every direct-color alpha bit to one.** Check converter force-opaque options and any blanket `| 0x8000` too. Correct alpha before upload is insufficient evidence. Do not fix a missing mask by making the entire polygon half-transparent or keying black/magenta over an existing source alpha channel.

## Sampling and material state

Reconstruct TMEM/tile view, line stride, dimensions, tile origins, shifts, masks, wrap/mirror/clamp, texture scale/generation and palette bank. Keep signed UVs and source fixed units until composing the sampling transform; quantize to DS 12.4 at the final boundary. Atlas padding needs zero-alpha coverage and correct neighbor/edge behavior. N64 filtering is not desktop bilinear; resolution/filter adaptations require an approved visual boundary. Source texture rectangles also have endpoint/cycle conventions—not just two corners to copy.

Distinguish per-polygon attributes from **global raster controls**. DS blending, alpha-test enable/threshold and global texturing are not separate stored settings for each queued draw. Use a compatible global policy and native masks/materials; do not toggle `glAlphaFunc()` per queued mesh and claim independent thresholds. `POLY_ALPHA(0)` means wireframe. Omit emission only when intended output is truly zero, while retaining required source state/update work.

Keep texture/palette, format/color-zero flag, polygon equation/alpha/ID, depth and culling coherent. Runtime caches and offline pre-baked corpora must share the complete semantic key, including effective alpha use, sampler state and palette/material/source generations. Test the same image under alpha-using and alpha-ignoring materials; both must coexist without aliasing. Separate cutout and translucent recipes; IDs, ordering, depth writes and 2D destination layers affect visible overlap. No global cache reset, stale handle substitution or omitted draw on allocation failure.

## Regression fixture selection

Select fixtures relevant to the changed material/sampling boundary; this is not an all-cases gate for every graphic edit: transparent TLUT entry 15 with opaque entry 0; multiple transparent entries; opaque black; equal RGB with different alpha; IA/I alpha ramps; palette alpha animation; correct RGBA uploaded under the wrong type; lost color-zero flag; correct alpha under decal mode; transparent padding/wrap edges; two overlapping sprites and an opaque occluder; camera movement and reload.

Compare decoded native masks/quantized alpha and real submitted state, then native captures over contrasting backgrounds. After resizing, test thin-stroke/edge coverage; averaging can erase sparse effects, but coverage-preserving reduction still needs the project's adaptation approval. Host checks cannot prove raster, palette allocation, GPU ordering or the game's material compiler. `tests/test_texture_alpha.py` covers the documented normalized conversion subset; its fixture is original synthetic data, not an extracted game texture.
