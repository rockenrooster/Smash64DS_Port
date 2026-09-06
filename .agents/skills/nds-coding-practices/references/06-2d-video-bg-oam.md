# 2D Video, Backgrounds, Sprites, and OAM

## Understand the two engines

The DS has separate main and sub 2D engines. Each provides four BG layers,
128 sprite entries, windows, mosaic, blending, and engine-specific palette and
OAM state. Similar APIs do not imply shared storage.

The 3D engine appears through BG0 of the **main** engine. The sub engine cannot
host an independent 3D scene.

## Initialize ownership once

A scene or platform layer should own:

- `videoSetMode` and `videoSetModeSub`;
- display routing;
- VRAM bank mappings;
- BG layer allocation and base blocks;
- main/sub OAM initialization and mapping mode;
- blending, windows, brightness, mosaic, and capture state.

Do not let unrelated UI widgets change global display mode or VRAM mappings.

## Background practices

Choose the BG type from the content:

- text/tiled BG for tile-reusable maps and conventional UI;
- affine/extended rotation BG for hardware transforms;
- bitmap BG for unique pixels when memory bandwidth and VRAM allow;
- 3D BG0 only when using the main GX engine.

### Use API or direct registers coherently

libnds maintains background state for scrolling and affine transforms. Its
background header warns that mixing API and direct-register writes for scrolling,
scaling, or rotation can produce unexpected results. Pick one owner for those
registers.

### Base-block math

Choose engine/layer, BG type, dimensions, color depth, and supplying bank.
Calculate tile/bitmap and map intervals from the base-block units; reject
actual overlaps rather than guessing base numbers. Keep palette, wrap, and
priority settings with that initialization. A short code comment is enough;
do not create a separate BG manifest for one static layer.

### Update dirty regions

Static maps and tiles should be uploaded once. For dynamic content:

- update only changed tiles/map entries;
- batch contiguous dirty ranges;
- avoid regenerating an entire map for a small animation;
- keep scroll-only motion in hardware offset registers;
- use affine hardware only when its precision and wrap behavior match the
  desired result.

## Sprite/OAM practices

Use libnds shadow OAM unless the project deliberately owns raw OAM writes.
Typical flow:

1. initialize OAM and OBJ VRAM mapping;
2. allocate/copy sprite graphics and palette;
3. update shadow entries with `oamSet` or related helpers;
4. wait for the chosen frame boundary;
5. commit once with `oamUpdate`.

```c
#include <nds.h>

static void commit_video_state(void)
{
    // Perform after the scene has finished changing shadow OAM.
    oamUpdate(&oamMain);
    oamUpdate(&oamSub);
}
```

### Hide every unused entry

An OAM entry retains old state. At initialization or allocator reset, explicitly
hide unused sprites. Do not assume zeroed memory means hidden hardware sprites.

### OAM ownership

- One allocator owns sprite indices 0-127 per engine.
- One allocator owns affine slots 0-31 per engine.
- A component cannot rewrite an affine slot while another visible sprite uses
  it.
- Sprite graphics allocation must match the OAM mapping mode.
- Shadow OAM writes and direct OAM writes must not race or overwrite one
  another.

### Coordinate behavior

Hardware coordinate fields wrap. A negative C coordinate cast into an OAM
field does not behave like an infinite signed plane. Use deliberate hide or
wrapped encoding for off-screen sprites and understand the sprite dimensions
when culling.

### Per-scanline OBJ budget

Sprite rasterization has a fixed per-scanline cycle budget in each 2D engine.
Wide sprites cost per pixel crossed and affine sprites cost several times
their linear width; once a row's budget is exhausted, the remaining sprites in
OAM evaluation order simply do not render on that row. Exact budgets are in
GBATEK. Design consequences: cap how many wide or affine sprites can share a
row, prefer several narrow sprites over one huge mostly transparent one, and
treat "sprites vanish only on crowded rows" as this budget, not OAM
corruption.

## Sprite formats

Verify together:

- shape and size enum;
- 16-color, 256-color, or bitmap format;
- tile/bitmap mapping mode;
- palette index or bitmap alpha meaning;
- graphics pointer/index;
- priority and OBJ mode;
- flip versus affine mode interaction.

Flips and affine rotation share attribute bits. Do not request both as though
independent.

## Blending and priorities

Priority values and blending targets are global engine state. For complex
composition, a small layer table can make the intended order explicit:

| Owner | Engine/layer | Priority | Blend role |
|---|---|---:|---|
| Example world | main BG2 | 2 | target B |
| Example sprites | main OBJ | 1 | target A |
| Example HUD | main BG0 | 0 | opaque |

Then configure blend control once and update only parameters that animate.
Remember that OBJ semi-transparency, bitmap OBJ alpha, BG blending, master
brightness, and windows interact.

## Windows and mosaic

Window masks and mosaic sizes are engine-global. Centralize them. When a scene
exits, restore or fully initialize them for the next scene; otherwise stale
masks can make layers disappear unexpectedly.

## Display capture

The main engine can capture its composited output — full 2D+3D, 3D only, or a
blend with VRAM or display-FIFO input — into one VRAM bank (A-D) as a 15-bit
bitmap through `DISPCAPCNT`. Main-RAM pixels need a separately owned path into
the display FIFO; capture does not take an arbitrary main-RAM pointer. Capture
enable is per frame (the hardware clears it after one capture). Use capture for
a retained bitmap, dual-screen 3D, motion blur, or screenshots. Reduced-rate GX
submission alone does not require capture; retained geometry still needs its
textures and palettes (see `07-3d-gx.md`).

Rules:

- one owner arms capture and sequences frames;
- the destination bank must be mapped so capture can write it (LCDC
  allocation) and must not be remapped mid-capture;
- displaying last frame's capture while capturing the next requires two banks
  ping-ponged, or a single-bank schedule proven against the capture timing;
- captured output is a bitmap: budget the bank and the BG/display path that
  shows it like any other asset.

VBlank is a good commit boundary for OAM, palette changes, and small register
updates, but it is not unlimited time. Prepare shadow state before VBlank and
perform only bounded commits in the VBlank window.

Large VRAM uploads should be scheduled deliberately, often during loading,
blanking, or across multiple frames. Do not assume `swiWaitForVBlank()` makes an
arbitrarily large copy safe or free.

## Correct tiled-background pattern

See `../examples/tiled_background.c`. The essential ordering is:

1. set display mode;
2. map a VRAM bank to the correct BG engine;
3. call `bgInit`/`bgInitSub` with non-overlapping bases;
4. copy generated tile/map/palette bytes;
5. set scroll/affine state through the same API owner;
6. call `bgUpdate` when using the libnds transform state.

## Correct sprite pattern

See `../examples/sprite_oam.c`. Allocate graphics through the OAM allocator or
through a single project allocator. Populate shadow OAM every frame as needed
and commit once.

## Common failures

### Sprite graphics are garbage

Check OBJ bank mapping, OAM mapping mode, color depth, byte count, graphics
allocation, palette, whether data was copied to the correct engine, and
whether the copy used halfword/word writes (VRAM ignores byte writes).

### Sprites vanish only on crowded rows

The per-scanline OBJ budget is exhausted on those rows; later sprites in OAM
evaluation order are skipped. Reduce wide/affine sprites sharing the row —
this is capacity, not corruption.

### Sprite exists only after a later scene

Initialization order or stale OAM is likely. Fully initialize display mode,
VRAM, OAM, palette, and hidden entries on every cold start and repeated entry.

### Background scroll jumps or resets

Likely mixed direct-register and libnds transform ownership, a missing
`bgUpdate`, or another component writing the same layer.

### Random sprites appear

Unused OAM entries were not hidden, shadow state was not initialized, or a
component wrote outside its allocated index range.

### Main and sub assets are swapped

Check engine-specific VRAM mapping, palette addresses, OAM contexts, and
`bgInit` versus `bgInitSub`.

## Review checklist

- [ ] Display mode and VRAM mapping have one owner.
- [ ] Every BG base interval is calculated and non-overlapping.
- [ ] OAM and affine slots are allocated, bounded, and initialized hidden.
- [ ] Shadow OAM is committed exactly where intended.
- [ ] Main/sub palettes and VRAM destinations are not confused.
- [ ] API and direct-register transform writes are not mixed.
- [ ] Blend/window/mosaic state is reset deterministically.
- [ ] Worst-case per-scanline OBJ load (width and affine cost) fits the budget.
- [ ] Display capture, when used, has one owner, a stable LCDC-mapped bank,
      and explicit frame sequencing.
