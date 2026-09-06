# Nintendo DS Hardware Model

## CPUs and responsibilities

### ARM9

- ARM946E-S, approximately 67 MHz in DS mode.
- ARMv5TE instruction set; ARM and Thumb code.
- Instruction and data caches.
- 32 KiB ITCM and 16 KiB DTCM in the normal DS configuration.
- Primary owner of application logic and both video engines.
- Sole application-facing owner of the main 3D engine.
- No hardware floating-point unit.

### ARM7

- ARM7TDMI, approximately 33 MHz in DS mode.
- ARMv4T instruction set; ARM and Thumb code.
- No ARM9-style data cache.
- Owns low-level sound hardware and several platform services.
- Has limited local memory and bus bandwidth; it is not a free general-purpose
  coprocessor.

## Display system

- Two 256 x 192 pixel LCDs.
- Separate main and sub 2D engines, each with four BG layers and 128 OAM
  entries.
- The 3D engine feeds BG0 of the main engine; the sub engine has no independent
  3D engine.
- The main engine has a display-capture unit (`DISPCAPCNT`) that can write its
  composited or 3D-only output into a VRAM bank; the sub engine has none.
- VBlank occurs at approximately 59.8 Hz.
- OAM, palettes, VRAM banks, blending, windows, and display routing are
  persistent hardware state.
- Both 2D engines and the 3D render engine have per-scanline capacity limits;
  exceeding them drops content on that row rather than slowing down.

## Memory domains

| Domain | Typical role | Important property |
|---|---|---|
| Main RAM, 4 MiB in retail DS mode | code, heaps, game state, assets | cached by ARM9 behind a 16-bit bus; misses cost tens of cycles; shared visibility requires coherency |
| ARM9 ITCM | selected hot code | tiny, deterministic, displacement matters |
| ARM9 DTCM | selected hot data/stack depending linker | tiny, ARM9-local behavior |
| Shared WRAM | partitioned ARM9/ARM7 data | mapping and ownership must be explicit |
| ARM7 WRAM | ARM7 code/data | not a general ARM9 heap |
| VRAM A-I, 656 KiB total | BG, OBJ, textures, palettes, LCD capture | bank mapping defines meaning and address |
| Palette RAM | BG/OBJ palettes | engine-specific and format-limited |
| OAM | sprite attributes | engine-specific, normally updated from shadow OAM |

**Physical capacity is not allocatable application memory.** The reviewed
Calico `ds9.ld` requires ordinary DS-mode ARM9 sections to end by `0x02380000`
(the nominal 3.5 MiB split). Loaded sections, startup reservations, runtime data,
and stacks reduce usable heap further. Static excess can fail the link;
dynamic excess can fail at runtime. See `17-libnds2-calico-facts.md` and the
actual map/allocator, not a blanket 4 MiB application budget.

VRAM bank capacities are:

- A-D: 128 KiB each;
- E: 64 KiB;
- F-G: 16 KiB each;
- H: 32 KiB;
- I: 16 KiB.

A bank cannot serve incompatible roles simultaneously. Changing a bank's mode
changes what its addresses mean; old pointers are no longer trustworthy.

On ARM9, VRAM, palette RAM, and OAM ignore byte writes, so byte-path copies
silently lose data. The separate ARM7 plain CPU-access VRAM mapping permits
byte stores; that exception is not a safe ARM9 graphics upload path.

## Hardware engines

### DMA

Each CPU has four DMA channels. DMA does not participate in ARM9 data-cache
coherency. Channels are global resources on their CPU and require ownership.
DMA cannot access ARM9 ITCM or DTCM. Flushing does not fix an inaccessible
address, and a stack buffer may be in DTCM depending on the linker/runtime.

### Math hardware

The ARM9 side exposes hardware divide and square-root units. They are shared,
stateful units; overlapping users require serialization or a central wrapper.
They do not make arbitrary floating-point code cheap.

### GX 3D

The geometry engine consumes a persistent command/state stream. Matrix mode,
matrix contents, polygon format, texture state, color, normal, and primitive
state carry forward until changed. FIFO capacity and geometry completion can
stall the CPU.

Polygon RAM and vertex RAM bound one frame at 2048 polygons and 6144
vertices; geometry beyond either cap is dropped for the rest of the frame,
not queued. The render engine rasterizes in lockstep with scanout using a
small scanline lead — there is no full frame buffer.

### Sound

The sound hardware is controlled from the ARM7 side. Application code normally
uses the current runtime's ARM9-facing audio API or Maxmod rather than touching
sound registers from ARM9.

## Ownership table

| Resource | Recommended single owner |
|---|---|
| VRAM bank map | video/platform initialization layer |
| Main OAM | main-screen sprite renderer |
| Sub OAM | sub-screen UI renderer |
| Each affine OAM slot | one allocator/renderer |
| GX matrix and draw state | 3D renderer or explicit state scopes |
| DMA channel | transfer service or documented subsystem reservation |
| Hardware divide/sqrt | math wrapper if multiple contexts can overlap |
| Audio channels/handles | audio service |
| Shared ARM9/ARM7 queue | one producer and one consumer protocol |
| VBlank commit order | frame/platform layer |

## Design consequences

- A dual-CPU system requires protocols, not shared global variables.
- Hardware acceleration often shifts cost to setup, synchronization, or bus
  contention.
- A pointer is not a resource handle when VRAM can be remapped or an asset can
  be evicted.
- Main and sub engines are similar but not interchangeable.
- Per-frame and per-scanline capacity limits (polygon/vertex RAM, OBJ raster
  budget) are hard hardware behavior, not tunables; exceeding them drops
  content silently.
- Emulator success cannot prove cache, bus-contention, storage, or exact
  presentation timing behavior on hardware.
