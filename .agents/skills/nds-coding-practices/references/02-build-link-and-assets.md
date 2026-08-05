# Build, Link, Sections, and Assets

## Begin from the supported template

Prefer the current devkitPro project templates and package manager rather than
copying an old Makefile. Keep toolchain, libraries, examples, and ARM7 runtime
from compatible generations.

A healthy project makes these outputs easy to inspect:

- ARM9 and ARM7 ELF files;
- map files;
- section sizes and addresses;
- disassembly for selected symbols;
- generated asset manifests;
- final NDS image and build flags.

## Compilation practices

- Use optimization suitable for the project, commonly `-O2` as a baseline.
- Treat `-O3`, `-Os`, LTO, and `-Ofast` as measured choices, not upgrades.
- Avoid `-ffast-math` when exact NaN, infinity, rounding, or source behavior
  matters; the larger issue is often that floating point exists at all.
- Enable useful warnings and fix signedness, truncation, format, and alignment
  warnings instead of suppressing them globally.
- Keep debug instrumentation compile-time removable.
- Compile generated data as `const` when immutable so it lands in read-only
  sections and cannot be corrupted. On DS this is a correctness choice, not a
  RAM saving: the whole linked ARM9 image is resident in main RAM, so `const`
  versus `.data` changes nothing about RAM use. Data too large to stay
  permanently resident belongs in the filesystem and streamed, not linked into
  the binary.

## Linker and section practices

Inspect the map before manually placing code or data. TCM is scarce and can
lose by displacing a better resident or increasing call/branch overhead.

When the active libnds/toolchain provides section macros, use those rather than
inventing incompatible section names. Confirm:

- section capacity;
- load versus run address;
- startup copy/zero behavior;
- alignment;
- whether function pointers or overlays remain valid;
- impact on both hot path and whole frame.

Do not put large lookup tables in DTCM just because they are frequently read.
Working-set locality and contention matter more than symbol popularity.

## Binary layouts

Use fixed-width fields and compile-time checks for data exchanged with hardware,
ROM assets, ARM7, or serialized files.

```c
#include <stdint.h>

struct __attribute__((packed)) AssetHeaderDisk {
    uint32_t magic_le;
    uint16_t width_le;
    uint16_t height_le;
};

_Static_assert(sizeof(struct AssetHeaderDisk) == 8, "disk layout changed");
```

Do not directly cast arbitrary byte buffers to packed structures and issue
unaligned word loads. Decode fields explicitly or use `memcpy` into aligned
locals when the format permits.

## Build-time asset conversion

Prefer a deterministic converter that emits:

- final DS pixel/tile/texture bytes;
- final palette bytes;
- dimensions, format, and bank requirements;
- transparent-index and alpha policy;
- byte counts and hashes;
- generated C headers or binary files;
- preview/oracle images when visual fidelity matters.

Fail the build on:

- palette overflow;
- unsupported dimensions;
- texture alignment violations;
- map/tile-base overlap;
- out-of-range indices;
- payload larger than its declared VRAM allocation;
- non-deterministic output.

## Asset ownership

Choose one runtime representation whenever possible:

- ROM-native compressed form for cold assets;
- main-RAM prepared form for streamed assets;
- VRAM-native resident form for active assets.

Avoid keeping source PNG-equivalent bytes, decoded RGBA, quantized indexed data,
and VRAM-native data simultaneously without a documented lifetime reason.

## C++ runtime choices

C++ is viable, but be deliberate about:

- exceptions and unwind tables;
- RTTI and virtual dispatch;
- static constructors and initialization order;
- iostream footprint;
- heap use by containers;
- destructor work during scene transitions;
- hidden copies and temporaries.

Prefer explicit arenas, fixed-capacity containers, spans/views, and trivially
copyable hardware payloads in hot or memory-constrained code.

## Build verification

A code change is not complete until the final linked image confirms:

- intended symbol and section placement;
- no accidental software floating-point/division helpers in hot paths;
- no unexpected duplicate asset/runtime path;
- RAM, TCM, and ROM growth understood;
- generated assets rebuilt from their source rather than hand-edited.
