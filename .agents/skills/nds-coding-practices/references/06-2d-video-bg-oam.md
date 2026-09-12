# 06 — BG and OAM rendering

Choose the native engine that represents the content without changing its compositing behavior: tiled BG for static/repeating UI; OAM for suitable movable sprites; affine/bitmap BG for compatible surfaces; GX quads when depth or a GX material is genuinely required. “Sprite” describes content, not its hardware path. GL2D-style sprites use GX and must follow [07](07-3d-gx.md), not OAM rules.

## Layout and ownership

Set the main/sub display mode and VRAM roles before initialization. Each BG needs nonoverlapping map/tile/bitmap bases, dimensions, pixel format, palette and priority. Base units are hardware/API-specific, not arbitrary byte addresses. Tiled images, linear bitmaps and OAM 1D/2D mappings are not interchangeable. Select legal sprite dimensions and allocation/mapping alignment.

Maintain one shadow-OAM owner: use `oamSet`/hide/affine operations and commit with `oamUpdate()` during a bounded safe window. Do not mix raw hardware OAM writes with library shadow state. Affine double-size enlarges the clipping region, not the source bitmap. Check signed coordinates, wrap behavior, priority, shared affine matrices, and per-scanline OBJ capacity.

## Transparency differs by path

| Path | Alpha behavior to verify |
|---|---|
| Tiled OBJ | Color index 0 is transparent; there is no GX color-zero flag. Remap source masks, including opaque source index 0. |
| Indexed BG modes | Index-zero and backdrop behavior depend on the selected BG mode/compositing; do not apply an OBJ recipe to an arbitrary framebuffer. |
| Direct bitmap OBJ | Bit 15 marks per-pixel visibility; per-object alpha/mode and destination blend layers also matter. In libnds bitmap-OBJ use, the `oamSet()` palette argument carries object alpha (0 hides; 1–15 visible levels). |
| 3D output on main BG0 | GX texture/material alpha plus 2D layer priority and blend targets. Clear alpha must be zero where lower 2D layers should show. |
| Direct display framebuffer | Not automatically an OBJ or GX alpha surface; consult that display mode. |

Transparent RGB need not be black, and opaque black must remain visible. Never key an imported image by RGB alone when it already has an alpha mask. Semi-transparent OBJ blending also depends on the layers selected as blend destinations; correct texel data alone cannot guarantee the desired composition.

## Transfers and lifetime

Write VRAM/palette/OAM with supported aligned halfword/word operations. Generic byte-path `memset` or `memcpy` is not a safe promise. CPU or DMA writes need destination mapping, width, count, capacity, and safe timing checked. Keep decompression and filesystem I/O outside VBlank/IRQ; prepare in ordinary memory, then commit bounded changes.

Before freeing sprite graphics, hide every referring shadow entry, commit the hide, and reach the required display-safe release point. Merely freeing the OAM allocator's block does not remove a hardware reference. The [sprite example](../examples/sprite_oam.c) now includes a transparent border and hide-before-free teardown. Test edges over two contrasting layers, not just a matching solid background.
