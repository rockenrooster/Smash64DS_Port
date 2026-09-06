# 03 — RDP materials, textures, and target draw intent

## Classify equations and effects, not macro names

The N64 combiner and blender are separate stages. Combiner inputs/cycle state and
render/depth/coverage behavior both matter; two lists with the same texture can
need different target materials. Analyze the actual equations and state used by
the supported content. Do not map the name of a source preset to whichever DS
mode sounds similar. [Combiner][combine] [Render modes][render]

Build a small material recipe set, not a per-pixel RDP emulator on ARM9:

| Source intent | Candidate native treatment | Must verify |
|---|---|---|
| Untextured shade/solid color | Prepared colors or appropriate native lighting | Load-time color/normal history, color precision |
| Texture times shade/constant tint | Native textured modulation | Alpha equation and vertex tint/lighting equivalence |
| Binary cutout | Suitable texture alpha and target polygon state | Transparent texels, depth write, edge behavior |
| Translucent surface | Explicit ordered translucent recipe | Alpha precision, IDs/depth semantics, overlapping surfaces |
| Static multi-input color combination | Offline bake only invariant terms | Dynamic colors/palettes/UVs must remain live |
| Two textures or framebuffer-dependent effect | Dedicated proven replacement/path | DS capability, geometry cost, ordering, visual contract |
| Texture rectangle or UI sprite | BG/OAM or a native 3D quad as appropriate | Scaling, clipping, alpha, coordinate convention, ordering |

These are **candidate mappings**, not exact identities. Record whether each recipe
is exact at a stated boundary, project-approved visual adaptation, or unsupported.
A second pass is not automatically equivalent to a source second combiner cycle;
depth writes and destination blending can change its meaning.

## Texture identity includes how the image is interpreted

N64 TMEM is a small tile-addressed working store with separately configured tile
descriptors, not a set of DS texture handles. Convert the logical sampled image
and palette plus the effective sampling state. Do not treat the most recent
texture-image pointer as sufficient identity. [TMEM and tiles][tmem]

A useful material/texture key can need source generation, image byte region,
format/size, row layout, palette/TLUT generation, tile origin/extent, shift, mask,
mirror/clamp, texture scale, and combiner-dependent interpretation. Omit a field
only when a converter has proved it irrelevant or compiled it into another field.
Dynamic palette colors make a pointer-only cache stale without any file change.

Normalize source texture loads into logical images before target repacking. Raw
TMEM contents can contain source-specific row/layout effects; blindly copying
that buffer as a linear target texture is not a decoder. Repeated partial writes
need versioned regions or reconstructed final images at the relevant draw.

## Sampling is more than “divide UV by two”

Source vertex ST precision, texture scale, tile origin, shift, wrap mask, mirror,
and clamp compose into the sampled location. Fold constant operations offline;
keep animated operations explicit. The source load macros document mask and shift
behavior independently of image size. [Texture load sampling][sampling]

Target UV conversion must account for the final native dimensions and any atlas
placement. Padding a non-power-of-two image does not by itself preserve source
repeat boundaries. A UV origin offset that fixes one image can break a shifted
tile or flipped rectangle. Test negative coordinates and exact boundaries.

A DS atlas trades fewer binds for shared residency, edge padding, palette coupling,
and altered repeat behavior. Prefer it for content whose sampling contract fits.
Do not force unrelated, independently animated palettes into one shared palette.
Pre-expanding a tile or baking a border can be useful, but budget the retained
bytes and verify source edge samples.

The N64 filtering mode called bilinear uses a documented three-texel
approximation; matching an ordinary bilinear preview is not proof of matching
that source filter. Preserve point-sampled behavior where required, and label any
target filtering adaptation explicitly. [Source filter note][filter]

## Native formats: convert meaning as well as byte order

The original [color helper](../examples/n64_data.h) rearranges a decoded N64
RGBA5551 value into a DS direct-color word. It is not merely a byte swap, and it
is not the correct operation for an arbitrary palette or intensity record.
[N64 texture formats][formats] [DS RGB component packing][dsvideo]

Choose target palette/alpha formats from the **effective** material use, not just
the source format label. An intensity texture can supply color, alpha, or both
through different combiner recipes. A palette remap must preserve transparent
index policy and any index-based animation. Keep native palette metadata separate
from pixel data; validate both residency and palette-slot compatibility.

Texture images, palette entries, runtime IDs, and handle-table slots are separate
resources. Deduplicating pixel bytes can still exhaust palette slots or the
runtime table. Check their joint requirement before entering a scene/frame.
Do not recover table overflow by resetting a global allocator while old draws
still reference it.

## Dynamic effects and local failure

For scrolling UVs, color cycles, texture swaps, and palette animation, prefer a
small per-frame binding/update over regenerating static geometry. UV-generated
reflection effects also depend on normals and often camera/object transforms;
a static texture ID and a cached pose are not the whole dependency set.

A missing optional effect may use a local project-approved replacement. Required
geometry/material failures must be visible errors or pre-submission admission
failures. An opaque white quad, missing fighter limb, stale texture, or globally
reset palette cache is not a performance policy.

Use the companion skill for exact DS texture formats, VRAM mapping, GX state,
palette constraints, blend/depth rules, and upload lifetime. This chapter is the
source-to-target material decision layer.

[combine]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/gdp/gDPSetCombineMode.html
[render]: https://ultra64.ca/files/documentation/online-manuals/man-v5-1/n64man/gdp/gDPSetRenderMode.htm
[tmem]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-04.html
[sampling]: https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gdp/gDPLoadTexture.html
[filter]: https://ultra64.ca/files/documentation/online-manuals/man-v5-2/allman52/n64man/gdp/gDPSetTextureFilter.htm
[formats]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro13/13-01.html
[dsvideo]: https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/video.h
