# LoadTile Fixup Short-Changes Last-Row Right Edge (2026-04-22) — FIXED

**Symptom:** On Yoster (and any stage whose 2D decorative sprites use
the chained-`G_LOADTILE` pattern), ~5 thin horizontal semi-opaque
pink/pale stripes render across the viewport at regular vertical
positions. Each stripe appears at the bottom of one stacked
decoration quad, does not react to scene content behind it, and
persists across camera moves. The stripes look like "stuck scanline"
artifacts.

Bisection with `SSB64_THIN_ALPHA_KILL=2 SSB64_THIN_ALPHA_KILL_FILE=ExternDataBank111`
localized them to 5 quads at Bank111 off `0x300-0x400` whose textures
are stored as **vertically-stacked strips** of a single composite
sprite in `ExternDataBank110`.

## Root cause

The game draws the composite sprite as 5 edge-to-edge quads, each
quad textured by a 14-row sub-rectangle of the full 65×65 image.
The GBI sequence per strip is:

```
G_SETTIMG  fmt=RGBA siz=32b width=66  addr=source
G_SETTILE  tile=7 (load tile)   tmem=0 line=17
G_LOADTILE tile=7 uls=0 ult=Y*4 lrs=256 lrt=(Y+14)*4   ← no LoadBlock
G_SETTILE  tile=0 (render)      tmem=0 line=17 cms=2 cmt=2
G_SETTILESIZE tile=0 uls=0 ult=Y*4 lrs=256 lrt=(Y+14)*4
```

The port's `GfxDpLoadTile` (`libultraship/src/fast/interpreter.cpp`)
populates `loaded_texture[idx].addr = source + start_offset_bytes`
and records `full_image_line_size_bytes = full_w * pixel_size`
(= 66 × 4 = 264) alongside `line_size_bytes = tile_w * pixel_size`
(= 65 × 4 = 260). The later `ImportTextureRgba32` decode then reads
pixels at `(y * full_image_line_size_bytes + x * pixel_size)` for
`y ∈ [0, tile_h)`, `x ∈ [0, tile_w)`.

Under the chained-LoadTile pattern, the port must byte-swap each
loaded strip from N64 BE to host LE on first read. `GfxDpLoadTile`
did this via:

```cpp
portRelocFixupTextureAtRuntime(texture_to_load.addr,
                               start_offset_bytes + orig_size_bytes);
```

where `orig_size_bytes = tile_line_size_bytes * tile_height = 260 * 14
= 3640`. But the decoder's read extent is `(tile_h-1) *
full_image_line_size_bytes + tile_line_size_bytes = 13 * 264 + 260 =
3692`. So the **last 52 bytes (13 RGBA32 pixels) of row 13 stay in
pass1-BSWAP32 (LE) state** — the fixup never touches them because
it uses the packed tile stride (260) instead of the full-image
stride (264) when computing the upper byte bound.

Pixel-level fingerprint. In the captured ASCII alpha mask of the
first strip (with `F7EBF000` = transparent-pink background):

```
row  0..12  right edge: F7EBF000 F7EBF000 F7EBF000 F7EBF000 F7EBF000 FFFFFF00
row 13      right edge: 00F0EBF7 00F0EBF7 00F0EBF7 00F0EBF7 00F0EBF7 00FFFFFF
```

Row 13's RGBA bytes are exactly byte-reversed relative to rows 0-12.
The byte-reversed `F7` (originally the R channel of the pink
background) lands in the **alpha slot**, flipping the right 13 pixels
of the bottom row from alpha=0 (transparent) to alpha=F7 (247, near
opaque). Each strip therefore draws one opaque horizontal band across
its bottom row, and five stacked strips produce five thin stripes.

Also: strip N's row 13 lives at the same source bytes as strip N+1's
row 0 (they share a boundary row). Strip N+1's fixup *does* cover
those bytes, but it runs **after** strip N's upload, so strip N's
texture cache entry is already frozen with bad data and never
re-decoded on subsequent frames because `TextureCacheLookup` hits on
`(addr, fmt, siz, palette, origSize)`.

## Fix

Extend the fixup range to cover the actual decode read extent:

```cpp
portRelocFixupTextureAtRuntime(texture_to_load.addr,
                               start_offset_bytes + tile_height * full_image_line_size_bytes);
```

`tile_height * full_image_line_size_bytes` over-covers by at most one
stride-row worth of trailing bytes (`full_image_line_size_bytes -
tile_line_size_bytes`), which is safe because
`portRelocFixupTextureAtRuntime` is idempotent (marks fixed ranges
and no-ops on repeats) and clips to file-blob extents, so stepping
one row past the last used pixel cannot corrupt unrelated data.

The `GfxDpLoadBlock` path already uses the correct `orig_size_bytes`
semantics because LoadBlock's `full_image_line_size_bytes ==
tile_line_size_bytes` (no sub-rectangle). Only LoadTile suffers from
this class of bug.

## Class of bug

Classic "port reads more bytes than it fixes up" off-by-stride-row.
Applies to any reloc-resident texture loaded with `tile_width <
full_image_width` via LoadTile — common for decoration sprites that
draw in vertical strips to stay within TMEM. The fingerprint is
always "rightmost `(full_w - tile_w)` pixels of the last row come
out byte-reversed", so if a future decoration bug has a thin opaque
band at the bottom-right of a stacked sprite, check LoadTile fixup
coverage first.
