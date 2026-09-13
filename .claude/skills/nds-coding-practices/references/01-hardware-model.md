# 01 — Hardware model

Ordinary DS has ARM946E-S ARM9 (about 67 MHz), ARM7TDMI ARM7 (about 33 MHz), 4 MiB main RAM, 32 KiB ARM9 ITCM, 16 KiB ARM9 DTCM, 32 KiB configurable shared WRAM, 64 KiB ARM7-private WRAM, and 656 KiB banked VRAM. Neither CPU has a hardware floating-point unit. DSi capabilities must be explicitly selected, not assumed from the host emulator. Sources and baseline: [SOURCES](SOURCES.md).

| Resource | Implementation consequence |
|---|---|
| ARM9 instruction/data caches; 32-byte cache lines | External writers/readers need explicit ownership and cache maintenance; `volatile` is not coherence. |
| ITCM/DTCM | Fast CPU-local storage; not DMA/GX/ARM7-accessible. Check linker allocation, stack use, and hot working set. |
| Main/shared memory buses | CPU, DMA, graphics, and service traffic can contend. Fast host or emulator memory is not target evidence. |
| Banked VRAM A–I | Roles and legal addresses differ by bank. 656 KiB is not a fungible texture heap. |
| Main/sub 2D engines | BG/OAM are often cheaper for UI; main BG0 can carry the single 3D engine. Screen routing is separate. |
| GX geometry/raster units | Geometry storage, command/FIFO service, texture banks, and per-scanline raster work are separate limits. |
| DMA and hardware divider/sqrt | Shared engines need owners, visibility, alignment, and completion rules. Hardware math is not automatically reentrant. |
| ARM7 services | Preserve input, audio, power, storage and communication services provided by the runtime. ARM7 is not an RSP replacement. |

CPU mappings and usable heap are runtime facts, not physical-capacity facts. Calico's linker reserves regions for its services; do not reclaim them because the physical RAM exists. See [17](17-libnds2-calico-facts.md) for concrete pitfalls.

Hardware boundaries also affect correctness: narrow VRAM writes may be ignored; TCM transfers fail despite flushing; shared buffers can return stale cache lines; expired textures can still be sampled after the CPU submits another frame. Model each producer, consumer, transfer, and release explicitly.
