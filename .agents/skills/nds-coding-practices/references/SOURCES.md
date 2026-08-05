# Sources and Research Baseline

This pack is an implementation guide, not a replacement for current project
headers. Source priority is defined in `00-source-priority-and-versioning.md`.

## Primary current sources checked on 2026-08-03

### devkitPro/libnds

- Repository: <https://github.com/devkitPro/libnds>
- Research baseline commit:
  `84e6082ce27c87ed218fb369a9944644aa2243a6`
- Relevant current headers/source include:
  - `include/nds/dma.h`
  - `include/nds/arm9/cache.h`
  - `include/nds/arm9/background.h`
  - `include/nds/arm9/sprite.h`
  - `include/nds/arm9/videoGL.h`
  - `include/nds/interrupts.h`
  - umbrella headers under `include/nds/`

The DMA header explicitly warns that DS DMA cannot see dirty ARM9 data-cache
contents and shows `DC_FlushRange` before DMA. The cache header documents the
32-byte alignment contract for `DC_InvalidateRange`.

### devkitPro/nds-examples

- Repository: <https://github.com/devkitPro/nds-examples>
- Research baseline commit:
  `f1ba715a451c6407f8b0f805999d0153062ff552`
- Current official examples were used to confirm composition patterns including
  `pmMainLoop`, `scanKeys`, VBlank synchronization, OAM allocation/update,
  backgrounds, and GX initialization.

Examples are demonstrations, not complete architecture or performance rules.
Project headers still win.

### devkitPro/Calico

- Repository: <https://github.com/devkitPro/calico>
- Research baseline commit:
  `81b75e314d57ed1784545e28554e567f26f572f1`
- Calico is the current low-level foundation used by libnds and provides modern
  runtime facilities for threading, synchronization, inter-processor messaging,
  and platform services.

This is why the pack warns against dropping legacy ARM7/FIFO templates into a
current project.

## Hardware reference

### GBATEK

- Canonical site: <https://problemkaputt.de/gbatek.htm>

Used for CPU, memory map and timings, VRAM bank, video, OAM, GX, DMA, timer,
IPC, sound, and hardware-register behavior — including the access-width rule
(8-bit writes to VRAM/palette/OAM are ignored), polygon/vertex RAM capacities
(2048/6144), per-scanline OBJ and raster budgets, and display capture
(`DISPCAPCNT`). For conflicting facts, verify against the exact DS mode and
current library implementation.

## Additional authoritative inputs

- Installed devkitPro/devkitARM package headers and linker scripts.
- Installed libfat, Maxmod, and project-selected library documentation/source.
- ARM946E-S and ARM7TDMI architecture documentation for codegen/cache/ISA
  questions.
- Project source/original implementation as the behavioral oracle for ports.

## Citation/version policy for maintainers

When changing an API-specific rule:

1. record the installed/upstream version or commit;
2. link the exact header/example;
3. distinguish API contract from hardware fact;
4. update code examples and the compatibility warning together;
5. do not copy third-party tutorial code without checking current headers.

## Revision history

- 2026-08-03: initial pack.
- 2026-08-03 (rev 2), audited against installed libnds headers: corrected the
  `glColor3b` component range in `examples/gx_frame.c` (components are 0-255;
  the low 3 bits are ignored) and the `oamSet` flip-comment order in
  `examples/sprite_oam.c` (hflip precedes vflip). Added the video-memory
  byte-write rule, polygon/vertex RAM caps and per-scanline budgets, display
  capture, an approximate cost model, unaligned-load rotation behavior, and
  the DS-resident-image `const` clarification.

## Known scope limits

- Primary target is retail DS mode; DSi mode needs additional TWL/SCFG/memory
  review.
- Direct-register examples are intentionally limited because library shadow
  state and ownership must be understood first.
- Exact performance varies by ROM, storage, scene, emulator, and hardware.
- The static examples in this pack were designed for current libnds conventions
  but must be compiled against the consuming project's installed toolchain.
