# RAM, VRAM, and Asset Residency

## Budget by bytes, banks, and lifetime

Use byte-accurate sizes, alignment, memory domains, and lifetimes. Existing
allocation code or a short layout table is enough; a separate per-allocation
report is not required. Know where duplicate representations overlap.

A peak-memory budget matters more than the sum of nominal asset sizes.
Transitions, double buffering, decompression scratch, filesystem buffers,
allocator metadata, and ARM7 reservations all overlap.

## Main RAM practices

Retail DS hardware has 4 MiB main RAM, not a 4 MiB free ARM9 heap. In the
reviewed Calico DS layout, ordinary ARM9 sections stop at `0x02380000`; runtime
and ARM7 reservations, loaded sections, and stacks reduce the available budget.
Use the project's map and peak allocator state. See `17-libnds2-calico-facts.md`.
Keep hot runtime data compact and avoid desktop-style object graphs.

Prefer:

- dense indices rather than many pointers;
- fixed-capacity pools for bounded game objects;
- scene arenas reset as a unit;
- structure-of-arrays where only a few fields are hot;
- compact immutable resident tables with their loaded RAM cost budgeted;
- cold tables in filesystem assets, not assumed directly pointer-readable ROM;
- one prepared representation for active assets;
- explicit scratch arenas instead of large stack arrays.

Avoid keeping compressed ROM data, decoded RGBA, indexed pixels, converted
texture data, and VRAM data simultaneously unless a transition genuinely needs
all of them.

## VRAM is banked hardware state

The nine banks total 656 KiB:

| Bank | Capacity |
|---|---:|
| A-D | 128 KiB each |
| E | 64 KiB |
| F-G | 16 KiB each |
| H | 32 KiB |
| I | 16 KiB |

Each bank can be mapped only to roles supported by that bank: main/sub BG,
main/sub OBJ, texture, texture palette, LCD-accessible memory, extended
palettes, or other documented modes.

### Video memory ignores byte writes

On ARM9, VRAM, palette RAM, and OAM accept 16-bit and 32-bit writes only. The DS
ignores 8-bit writes (GBA duplicated some byte writes into both halves; the DS
drops them). Any byte-granularity path — `memset`, a `memcpy` tail for odd
sizes or alignment, a `uint8_t *` pixel loop, a packed-struct copy — silently
loses exactly those bytes, while the identical code works against a main-RAM
staging buffer and a debugger reading main RAM "confirms" the data is correct.
Copy into video memory with halfword/word stores, `dmaCopy`/`swiCopy`, or a
helper whose emitted access width has been verified once. When a converter
emits 8bpp data, pack pixel pairs into halfwords at generation time.

### One global VRAM-map owner

Centralize bank configuration in the video/platform layer. Scene renderers may
request a declared layout, but should not independently remap banks behind one
another.

```c
struct VramLayout {
    uint32_t generation;
    // Project-specific bank role enum for A-I follows.
};
```

Increment the generation whenever mappings change. Runtime handles that can
survive scene changes should record the generation and reject stale use.

### Raw VRAM pointers are temporary capabilities

A pointer returned while a bank is mapped for CPU/LCD access is not a permanent
asset identity. Remapping changes the bank's address and hardware role. After a
remap:

- do not dereference old pointers;
- do not launch DMA using old addresses;
- do not assume texture/BG/OBJ offsets are unchanged;
- rebuild or validate handles against the current layout generation.

Prefer handles containing bank/slot/offset/format/generation over long-lived raw
pointers.

A permanently fixed mapping with scene-local references does not need a new
handle/generation framework. Add generations when references can actually
outlive remapping, eviction, or asynchronous completion.

## Stable mappings beat clever remapping

Frequent bank remapping adds synchronization, stale-pointer risk, and upload
cost. Prefer a stable scene layout when possible. Remap only at explicit
boundaries after:

1. all DMA and display-capture users are complete;
2. no CPU code retains a usable pointer;
3. the affected engine is hidden or transitioned safely;
4. dependent registers and handles will be rebuilt;
5. palette and texture associations are restored.

## Asset format rules

Convert assets at build time to the exact target representation:

- tiled 4bpp/8bpp BG graphics and maps;
- OBJ tile data and palette indices;
- DS texture formats and texture-palette arrangement;
- 15-bit colors with deliberate transparency/alpha policy;
- fixed-point geometry, normals, UVs, and matrices when static;
- audio sample format/rate expected by the selected library.

Every generated asset should declare:

- byte count;
- dimensions;
- element format;
- alignment;
- palette count/format;
- expected destination class;
- any required bank or slot constraints.

Use `_Static_assert` or generated manifest checks for C-integrated payloads.

## Palette practices

- Reserve transparent index behavior deliberately; do not assume index 0 is
  universally transparent in every mode.
- Keep main and sub BG/OBJ palettes distinct.
- Enforce 16-color versus 256-color limits during conversion.
- Track extended-palette bank ownership separately from ordinary palette RAM.
- Do not update a palette while another draw owner assumes its contents are
  stable unless the transition is synchronized.
- Merge palettes offline only when index remapping and visual error are proven.

## Texture residency

A texture handle should encode enough information to validate:

- format and dimensions;
- texel storage slot/offset;
- palette slot/offset when applicable;
- layout generation;
- residency state;
- last-use or pin count if eviction exists.

Do not store a raw pointer as the only texture identity. The 3D engine consumes
VRAM state, not a main-RAM pointer.

Texture uploads should be batched outside the critical draw loop. Avoid
converting, quantizing, swizzling, or allocating during first visible use.

## BG and OBJ allocation

For backgrounds, verify that tile bases, map bases, bitmap bases, and extended
palette reservations do not overlap within the bank layout.

For sprites:

- choose 1D or 2D mapping intentionally;
- match allocation calculations to color depth and sprite dimensions;
- centralize OBJ VRAM allocation;
- free or reset allocations at scene boundaries using the same owner that
  created them;
- never derive a graphics index using assumptions from a different mapping
  mode.

## Compression and streaming

ROM compression saves ROM/storage bandwidth but costs scratch RAM and decode
time. Choose compression by total lifetime:

- cold assets may remain compressed;
- loading-screen assets may decode directly into their final main-RAM/VRAM
  staging representation;
- frame-critical assets should normally already be resident;
- tiny frequently used tables are often better left uncompressed.

Design decompression destinations so source, destination, and scratch overlap
safely—or prove that the codec supports in-place operation.

## Allocator practices

- Reuse suitable persistent, scene, frame, or transfer storage; separate arenas
  only when the lifetimes actually differ.
- Align DMA/cache-shared buffers to 32 bytes and round ownership to full cache
  lines.
- Detect allocation failure in release builds.
- Poison/reset debug arenas when practical.
- Track high-water marks on representative scenes.
- Avoid general heap allocation in IRQs and per-frame rendering.

## Common failures

### VRAM corruption after scene change

Likely causes:

- stale pointer after bank remap;
- stale texture/palette handle;
- DMA still active during remap;
- new map/tile bases overlap old assumptions;
- allocator reset without clearing renderer handles.

### Asset looks shifted or scrambled

Check:

- byte-path copy into video memory (8-bit writes are ignored);
- 1D/2D OBJ mapping mismatch;
- 4bpp versus 8bpp size math;
- BG map dimensions and screen-block layout;
- texture dimensions and format;
- byte/halfword count truncation;
- endian/packing conversion;
- palette index remapping.

### Memory fits individually but crashes in transitions

Measure peak overlap: old scene + new scene + decompression scratch + queued
DMA/audio + filesystem cache + stack.

## Review checklist

- [ ] Every bank A-I has one declared role for the current layout.
- [ ] No byte-granularity CPU write targets VRAM, palette RAM, or OAM.
- [ ] No raw VRAM pointer survives a remap boundary.
- [ ] BG/OBJ/texture/palette ranges are byte-accounted and non-overlapping.
- [ ] Assets are already in DS-native format before active use.
- [ ] Duplicate representations have explicit lifetimes.
- [ ] Cache-shared buffers own complete 32-byte lines.
- [ ] Peak transition memory and allocator high-water marks are known.
