# Nintendo 64 vs. Nintendo DS: Hardware and Performance Reference

**Scope:** retail Nintendo 64 versus the original Nintendo DS and DS Lite operating in native DS/NTR mode.  
**Research date:** September 16, 2026.  
**Purpose:** a side-by-side programmer's hardware reference, especially for native game-engine ports. This is not a comparison of emulators, commercial-game screenshots, or nominal “console generations.”

## How to read this reference

**KiB/MiB are binary capacities; MB/s means decimal millions of bytes per second.** Clock rates are nominal, not measurements of the oscillator in an individual console. “CPU cycle,” “bus cycle,” “pixel,” “vertex,” “polygon,” and “frame” are deliberately kept separate.

A **documented limit** describes a hardware resource or specified behavior. A **measured timing** is a published hardware observation under stated conditions. A **calculated peak** is arithmetic from clocks, widths, or capacities—not a measured application result. A **porting implication** is an engineering deduction from those facts. Where a useful universal number does not exist, the table says so rather than inventing one.

The baseline excludes the N64 Expansion Pak unless explicitly stated, the 64DD, DS Slot-2 RAM accessories, flashcart-specific behavior, DSi enhanced mode, overclocking, and replacement display/output hardware. Links such as [N01] and [D03] lead to the sources listed at the end. Original Nintendo/NEC/MIPS/ARM documentation is preferred; GBATEK supplies first-hand reverse-engineered DS details, and BlocksDS supplies implementation-oriented explanations.

## Contents

1. [Architectural overview](#1-architectural-overview)
2. [CPU architecture and instruction sets](#2-cpu-architecture-and-instruction-sets)
3. [Caches, TCM, address translation, and alignment](#3-caches-tcm-address-translation-and-alignment)
4. [Arithmetic and instruction timing](#4-arithmetic-and-instruction-timing)
5. [Physical memory inventory](#5-physical-memory-inventory)
6. [DS VRAM banks and allocation limits](#6-ds-vram-banks-and-allocation-limits)
7. [Address-space landmarks](#7-address-space-landmarks)
8. [Bandwidth: comparable and non-comparable numbers](#8-bandwidth-comparable-and-non-comparable-numbers)
9. [Memory latency and access patterns](#9-memory-latency-and-access-patterns)
10. [DMA, coherency, IPC, and synchronization](#10-dma-coherency-ipc-and-synchronization)
11. [RSP versus DS geometry hardware](#11-rsp-versus-ds-geometry-hardware)
12. [Geometry capacities and command costs](#12-geometry-capacities-and-command-costs)
13. [Rasterization, fill rate, and overdraw](#13-rasterization-fill-rate-and-overdraw)
14. [Texture formats, sizes, and residency](#14-texture-formats-sizes-and-residency)
15. [Filtering, color combining, and numerical precision](#15-filtering-color-combining-and-numerical-precision)
16. [Depth, transparency, fog, antialiasing, and shadows](#16-depth-transparency-fog-antialiasing-and-shadows)
17. [2D engines, backgrounds, and sprites](#17-2d-engines-backgrounds-and-sprites)
18. [Displays, capture, scan timing, and frame budgets](#18-displays-capture-scan-timing-and-frame-budgets)
19. [Audio hardware and audio latency](#19-audio-hardware-and-audio-latency)
20. [Cartridges, storage, saves, and boot](#20-cartridges-storage-saves-and-boot)
21. [Input, wireless, clocks, and peripherals](#21-input-wireless-clocks-and-peripherals)
22. [Interrupts, timers, and scheduling](#22-interrupts-timers-and-scheduling)
23. [Physical systems, revisions, and expansion](#23-physical-systems-revisions-and-expansion)
24. [Worked memory and throughput budgets](#24-worked-memory-and-throughput-budgets)
25. [Native-porting consequences](#25-native-porting-consequences)
26. [Misleading claims corrected](#26-misleading-claims-corrected)
27. [What must be measured rather than assumed](#27-what-must-be-measured-rather-than-assumed)
28. [Source index](#28-source-index)

---

## 1. Architectural overview

| Resource | Nintendo 64 | Nintendo DS / DS Lite |
|---|---|---|
| Main general-purpose processor | NEC VR4300-family, MIPS III-derived, 64-bit integer architecture | ARM946E-S, ARMv5TE, 32-bit |
| Main CPU clock | 93.75 MHz | Approximately 67.027964 MHz |
| Other programmable computation | RSP: scalar processor plus 128-bit vector register operations | ARM7TDMI, ARMv4T, approximately 33.513982 MHz |
| Graphics architecture | RCP containing programmable RSP and fixed-function RDP | Fixed-function geometry engine and scanline 3D renderer, plus two 2D engines |
| Principal graphics clock | 62.5 MHz RCP | Approximately 33.513982 MHz geometry/bus timing domain |
| Main CPU floating point | Hardware single- and double-precision FPU | No FPU on either CPU |
| Main RAM | 4 MiB CPU-addressable RDRAM; 8 MiB with Expansion Pak | 4 MiB main RAM |
| Additional CPU-local RAM | RSP's 4 KiB instruction and 4 KiB data memories | ARM9: 32 KiB ITCM and 16 KiB DTCM; ARM7/private and shared WRAM also present |
| CPU instruction/data caches | 16 KiB / 8 KiB | ARM9: 8 KiB / 4 KiB; ARM7: none |
| Graphics memory model | Framebuffers, depth, assets, and work share RDRAM; 4 KiB texture working memory in RDP | 656 KiB banked VRAM plus internal geometry/scanline storage; banks have restricted roles |
| 3D scene-storage ceiling | No corresponding fixed 2,048-polygon scene-list ceiling | 2,048 polygons and 6,144 vertices per active geometry list |
| Screens | External television/output chain | Two 256 × 192 LCDs; only one 3D engine |
| Native tiled/sprite 2D hardware | No DS-style independent BG/OBJ engines | Two engines, each with backgrounds and objects |
| Audio model | CPU/RSP synthesize and mix; AI streams the result | 16 hardware sound channels controlled from ARM7 |
| CPU-to-device cache coherence | Software responsibility | Software responsibility, including ARM9 versus ARM7 |

Sources: [N01]–[N06], [D01], [D03], [D12], [D14], [D29], [A01].

**Interpretation:** the DS is not an N64 with a lower CPU clock. It replaces a programmable graphics/audio coprocessor and unified high-bandwidth graphics memory with specialized engines, a second scalar CPU, small fast local memories, and restrictive banked graphics storage. A native port must redistribute work, not merely recompile the original implementation.

## 2. CPU architecture and instruction sets

| Property | N64 main CPU | DS ARM9 | DS ARM7 |
|---|---|---|---|
| Core family | VR4300 / R4300i family | ARM946E-S, containing ARM9E-S core | ARM7TDMI |
| ISA | MIPS III-derived | ARMv5TE | ARMv4T |
| Integer register width | 64 bits | 32 bits | 32 bits |
| Main programmer-visible register set | 32 GPRs, including constant-zero register | R0–R15; R13/SP, R14/LR, R15/PC have special uses | Same basic arrangement |
| Extra register state | HI/LO, CP0 system registers, FPU registers | CPSR and banked exception-mode registers/SPSRs | CPSR and banked exception-mode registers/SPSRs |
| Instruction encoding | Fixed 32-bit MIPS instructions | 32-bit ARM; 16-bit Thumb | 32-bit ARM; 16-bit Thumb |
| Thumb-2 | Not applicable | No | No |
| Normal execution organization | In-order, single-issue, five-stage pipeline | In-order, single-issue, five-stage pipeline | In-order, single-issue, three-stage pipeline |
| General out-of-order execution | No | No | No |
| Architectural branch delay slot | Yes | No | No |
| Instruction/data organization | Separate instruction and data caches | Harvard-style instruction/data paths and caches | Unified instruction/data path |
| Console byte order | Big-endian | Little-endian | Little-endian |
| Typical game pointers / C ABI | Usually 32-bit pointers despite 64-bit integer registers | 32-bit pointers | 32-bit pointers |
| Native 64-bit add/shift/integer operations | Yes | Multi-instruction sequences | Multi-instruction sequences |
| Multiply producing a wide result | Integer multiply to HI/LO | Long multiply instructions produce 64-bit result | Long multiply instructions produce 64-bit result |
| CPU divide instruction | Yes | No ARM SDIV/UDIV; separate MMIO divider exists | No CPU divide instruction |
| Hardware floating point | CP1 FPU | None | None |
| Packed DSP-style arithmetic | RSP is the principal vector resource, separate from CPU | ARMv5TE signed halfword multiply/MAC and saturation instructions | No ARMv5TE DSP extensions |
| NEON / SSE / modern SIMD | No | No | No |
| Barrel-shifter operand support | MIPS shift instructions | Shifted operand integrated into many ARM operations | Same general ARM feature |
| Conditional execution | Branches and ISA-specific conditional operations | Most ARM-state instructions can be conditional | Most ARM-state instructions can be conditional |
| General-purpose threading model | Software scheduling, not hardware threads | Independent core; no hardware threads | Independent core; no hardware threads |

Sources: [N05], [M01], [M02], [A01], [D42], [D44]–[D46].

The ARM register count is often overstated in comparisons. Banked exception registers do not give an ordinary function dozens of simultaneously accessible working registers. Conversely, an N64 game's 32-bit ABI does not remove the processor's 64-bit integer instructions.

**Porting implications:** audit byte order in assets and packed structures; replace MIPS assembly and delay-slot assumptions; inspect generated code for software floating point and 64-bit helper calls. ARM and Thumb are two code-density/performance choices, not two different CPU clocks. Thumb can reduce instruction-cache pressure while increasing instruction count or restricting useful DSP operations. Choose per function after measurement.

## 3. Caches, TCM, address translation, and alignment

### 3.1 Cache organization

| Property | Nintendo 64 CPU | DS ARM9 | DS ARM7 |
|---|---|---|---|
| Instruction cache | 16 KiB | 8 KiB | None |
| Data cache | 8 KiB | 4 KiB | None |
| Instruction-cache line | 32 bytes | 32 bytes | Not applicable |
| Data-cache line | 16 bytes | 32 bytes | Not applicable |
| Associativity | Direct-mapped, both caches | Four-way, both caches | Not applicable |
| Instruction-cache entries | 512 lines | 256 lines in 64 sets | None |
| Data-cache entries | 512 lines | 128 lines in 32 sets | None |
| Ordinary data-cache policy | Write-back, write-allocate | Configurable write-back/write-through; no write allocation | No cache |
| Read miss | Fetches a cache line | Fetches a cache line | Direct memory access |
| Store miss | Can allocate a line | Does not automatically allocate a line | Direct memory access |
| Cache replacement | Fixed direct-map placement | Configurable replacement/lockdown mechanisms | Not applicable |
| Automatic cache snooping by DMA | No | No | No cache to snoop |
| Automatic coherence with other processor | No CPU/RSP coherence | No ARM9/ARM7 coherence | Must cooperate with ARM9 protocol |
| L2/L3 cache | None | None | None |

Sources: [M01], [D02], [B02].

An N64 direct-mapped cache can suffer conflict misses despite its larger capacity. A DS four-way cache reduces some conflicts, but its much smaller working set and memory refill cost remain crucial. **“Four-way” does not make a 4 KiB cache equivalent to an 8 KiB cache.**

### 3.2 Explicit fast RAM and protection

| Property | Nintendo 64 | Nintendo DS |
|---|---|---|
| Main-CPU software-managed TCM | None | 32 KiB ARM9 ITCM; 16 KiB ARM9 DTCM |
| Meaning of TCM | RSP local memories are separate coprocessor storage, not CPU TCM | Real explicitly addressed RAM, not a cache copy of main RAM |
| Typical use | CPU relies on cache; RSP DMA manages its scratch memories | ITCM hot code; DTCM hot data/stack; placement chosen by software |
| ITCM placement | Not applicable | Base at zero; mappings/mirrors must be interpreted correctly |
| DTCM placement | Not applicable | Base is software-configurable |
| DMA access to TCM | Not applicable | General DS DMA cannot access ARM9 TCM |
| Other CPU access to TCM | Not applicable | ARM7 cannot directly access ARM9 TCM |
| Address translation | MMU/TLB; common KSEG0/KSEG1 accesses bypass translation | ARM9 MPU protection/attributes, not an MMU |
| TLB | 32 paired-page entries | None |
| Protection regions | MIPS privilege/address mechanisms | Eight ARM9 MPU regions; no corresponding ARM7 MPU |
| Virtual-memory paging | Hardware address translation exists; not normal game demand paging | No MMU-based paging |
| Cached versus uncached views | KSEG0/KSEG1 physical-memory aliases | Memory aliases plus MPU/cacheability configuration; SDK-specific layout |
| Nominal versus usable local RAM | RSP microcode/data consume their own memories | Startup, exception, stack, and SDK reservations reduce free TCM/WRAM |

Sources: [N05], [N06], [D02], [B01], [B02].

### 3.3 Alignment is a correctness issue, not just an optimization

| Operation | N64 concern | DS concern |
|---|---|---|
| Unaligned word access | Ordinary aligned instructions can fault; explicit merge instructions exist | Legacy ARM unaligned behavior is not modern ARM/x86 behavior; rotation, ignored address bits, or faults may apply |
| Struct casting into packed data | Endianness and alignment must be handled | Same, plus different CPU byte order from N64 |
| DMA shared buffer | Device alignment and 16-byte data-cache-line boundaries matter | Device alignment and 32-byte cache-line boundaries matter |
| Executable code generation/loading | Instruction-cache invalidation and data visibility | Clean written code, invalidate instruction cache, and handle write-buffer ordering |
| VRAM/OAM/palette byte writes | RDRAM and RDP interfaces have their own rules | ARM9 byte stores to DS VRAM/OAM/palette are not ordinary byte-addressable RAM writes |

Sources: [N06], [D04], [D45], [B02].

**Implementation consequence:** isolate independently owned DMA buffers on whole cache lines. Invalidating a line shared with unrelated dirty data can destroy that data. A compiler `volatile` qualifier does not perform cache maintenance or make DMA coherent.

## 4. Arithmetic and instruction timing

These are operation timings, not whole-loop benchmarks. Memory accesses, dependencies, transfers into/out of coprocessors, and pipeline effects add costs.

### 4.1 N64 CPU arithmetic

| Operation | Published VR4300 timing | Clock domain / qualification |
|---|---:|---|
| 32-bit integer multiply, signed/unsigned | 5 | CPU cycles; manual's multiply/divide stall table |
| 64-bit integer multiply, signed/unsigned | 8 | CPU cycles; same qualification |
| 32-bit integer divide, signed/unsigned | 37 | CPU cycles |
| 64-bit integer divide, signed/unsigned | 69 | CPU cycles |
| Floating add/subtract, single or double | 3 | FPU execution cycles |
| Floating multiply, single | 5 | FPU execution cycles |
| Floating multiply, double | 8 | FPU execution cycles |
| Floating divide/square root, single | 29 | FPU execution cycles |
| Floating divide/square root, double | 58 | FPU execution cycles |
| Floating absolute/move/negate | 1 | FPU execution cycle |
| Floating-to-integer rounding/truncation family | 5 | FPU execution cycles |

The NEC manual's integer table explicitly describes pipeline stalls. Its floating-point table describes execution times; an immediately consuming instruction can require an additional cycle. Exceptional operands can follow different paths. These are **not** a promise of overlapping a new floating multiply with an add every cycle. At 93.75 MHz, one CPU cycle is **10.667 ns**, calculated. Source: [M02], printed pp. 76 and 233.

### 4.2 DS arithmetic

| Operation | ARM9 / hardware support | ARM7 |
|---|---|---|
| Integer ALU | Native 32-bit operations | Native 32-bit operations |
| Multiply | Native multiply and long multiply; timing depends on instruction/dependency | Native multiply; operand-dependent timing |
| Signed 16-bit DSP multiply/MAC | ARMv5TE instructions; useful for fixed-point kernels | Not present |
| Saturating arithmetic | QADD/QSUB and related ARMv5TE operations | Software sequences |
| Floating add/multiply/divide | Software routines | Software routines |
| Trigonometry | Software, approximation, or lookup tables | Same |
| 32/32 signed integer division | MMIO divider: 18 bus cycles | Not directly the ARM7's hardware unit |
| 64/32 or 64/64 signed division | MMIO divider: 34 bus cycles | Software or request assistance |
| Integer square root, 32/64-bit input | MMIO unit: 13 bus cycles, 32-bit result | Software or request assistance |

Sources: [A01], [D08], [D42], [D46], [B03].

The divider times convert to approximately **537 ns**, **1,015 ns**, and **388 ns** respectively, before register-access overhead. They are **bus cycles**, not 67 MHz ARM9 cycles. The units can run while the CPU performs independent work, but immediate polling defeats that opportunity. They do not make floating division a single hardware instruction. Protect shared math-unit use against interrupt code or other software that changes its registers.

For ARM7 timing formulas, `S` and `N` mean sequential/nonsequential memory cycles and `I` means internal cycles. A formula such as an ALU's `1S` is not automatically one wall-clock CPU cycle when code resides in slow memory. ARM9 interlocks and its separate fetch/data paths likewise make a single universal “instructions per MHz” ratio inappropriate. [D42]

## 5. Physical memory inventory

| Memory resource | Nintendo 64 | Nintendo DS |
|---|---|---|
| Main game RAM, CPU-visible payload | 4 MiB | 4 MiB |
| Optional baseline-compatible main-RAM expansion | Additional 4 MiB through Expansion Pak | None built into baseline; Slot-2 accessories are a different bus/resource |
| Physical extra graphics bits | RDRAM has ninth bits: 36 Mibit physical for 32 Mibit payload | No equivalent ninth-bit extension to main RAM |
| RSP instruction storage | 4 KiB | No RSP |
| RSP data storage | 4 KiB | No RSP |
| CPU instruction cache | 16 KiB | 8 KiB ARM9 |
| CPU data cache | 8 KiB | 4 KiB ARM9 |
| CPU-local instruction TCM | None | 32 KiB |
| CPU-local data TCM | None | 16 KiB |
| ARM7 private work RAM | Not applicable | 64 KiB |
| Configurable shared work RAM | Not applicable | 32 KiB, divided/assigned between CPUs |
| Banked graphics VRAM | No separate DS-like external VRAM pool | 656 KiB |
| Texture working memory | 4 KiB RDP TMEM | Up to 512 KiB of the VRAM banks can be active texture storage |
| Standard 2D palette RAM | No DS-style palette RAM | 2 KiB total, separate from VRAM |
| Sprite attribute RAM | No DS-style OAM | 2 KiB total, separate from VRAM |
| Internal geometry RAM | RSP local resources and command buffers, not the DS scheme | Published 248 KiB geometry-memory figure: 104 KiB polygon plus 144 KiB vertex storage |
| Scanline rendering storage | RDP has internal span/pipeline buffers | Internal 48-line rendering cache, not ordinary game heap |
| Wireless packet RAM | None | 8 KiB |
| Boot ROMs | PIF boot ROM plus cartridge boot code | Actual BIOS ROMs: ARM9 4 KiB; ARM7 16 KiB; GBA mode 16 KiB |
| Firmware flash | No DS-style internal menu firmware flash | Normally 256 KiB; some regional variants differ |

Sources: [N01], [N04], [N10], [M01], [D01], [D02], [D04], [D12], [B01].

**Do not sum all these numbers into “usable RAM.”** Cache contains copies; BIOS is read-only; some pools are device-private; VRAM roles compete; stacks and runtime state consume RAM. N64 ninth bits are not an additional 512 KiB malloc arena. The DS's internal geometry memory is not a place to store animation data. The published 248 KiB accounting should not be casually doubled again because the geometry engine swaps lists; the directly useful programming limits are 2,048 polygons and 6,144 vertices per active list.

The ARM9 BIOS has a **32 KiB address window but only 4 KiB of actual ROM content**. Confusing the mapping window with physical ROM produces contradictory spec sheets. [D01], [B01]

## 6. DS VRAM banks and allocation limits

N64 graphics allocations are primarily carved out of unified RDRAM. DS graphics allocation is also a **bank-assignment problem**.

### 6.1 Physical banks

| DS bank | Capacity | LCDC CPU-mapped base |
|---|---:|---|
| A | 128 KiB | `0x06800000` |
| B | 128 KiB | `0x06820000` |
| C | 128 KiB | `0x06840000` |
| D | 128 KiB | `0x06860000` |
| E | 64 KiB | `0x06880000` |
| F | 16 KiB | `0x06890000` |
| G | 16 KiB | `0x06894000` |
| H | 32 KiB | `0x06898000` |
| I | 16 KiB | `0x068A0000` |
| **Total** | **656 KiB** | These are real banks, not additional copies of other mappings |

### 6.2 Addressable roles

| Role | Maximum addressable allocation | Constraint |
|---|---:|---|
| 3D texture images | 512 KiB | A–D only |
| 3D texture palettes | 96 KiB | E/F/G roles |
| Main-engine background VRAM | 512 KiB | Competes for appropriate banks |
| Main-engine object VRAM | 256 KiB | Competes for appropriate banks |
| Sub-engine background VRAM | 128 KiB | Restricted bank choices |
| Sub-engine object VRAM | 128 KiB | Restricted bank choices |
| ARM7-accessible VRAM work memory | Up to 256 KiB | C and/or D, at the expense of graphics uses |
| Display-capture target | One of A–D | Target must have the appropriate LCDC assignment |
| Extended 2D palettes | Bank-dependent | Occupy VRAM rather than the separate standard palette RAM |

Source: [D04]; mapping/usable-memory interpretation: [B01].

The role maxima are **not simultaneously additive**. Assigning A–D to textures leaves none of those banks free for a capture target or other role. Texture and extended-palette mappings are not normal CPU-visible writable arrays while active; uploading typically requires the proper remap. Treat remapping as a synchronized ownership change, not free extra bandwidth.

## 7. Address-space landmarks

This is a navigation map, not a replacement for the full register reference. Mirrored ranges do not create new physical storage.

| Function | N64 physical / common virtual address | DS address / qualification |
|---|---|---|
| Main RAM | Physical `0x00000000`; cached `0x80000000`; uncached `0xA0000000` | `0x02000000`; 4 MiB physical with mirrors |
| Expanded main RAM | Physical `0x00400000` onward when Expansion Pak installed | Not present in baseline DS |
| Coprocessor local memory | RSP DMEM `0x04000000`; IMEM `0x04001000` | ARM9 ITCM at zero; DTCM base configurable |
| Shared work RAM | No directly equivalent pool | `0x03000000` mapping depends on ownership |
| ARM7 private work RAM | Not applicable | `0x03800000` |
| Graphics/system I/O | SP `0x04040000`; DP `0x04100000`; MI `0x04300000` | Main I/O window `0x04000000`; register meaning can differ by CPU |
| Video/audio interfaces | VI `0x04400000`; AI `0x04500000` | Display/geometry and ARM7 sound registers in I/O space |
| Cartridge/system DMA interfaces | PI `0x04600000`; RI `0x04700000`; SI `0x04800000` | DMA/card controls in I/O space |
| Standard palette RAM | Not applicable | `0x05000000` |
| VRAM | Primarily RDRAM plus private TMEM | `0x06000000`-region role mappings; LCDC bases above |
| OAM | Not applicable | `0x07000000` |
| Cartridge ROM | Physical `0x10000000`, behind PI | Slot-1 is command/data-port accessed, not an execute-in-place ROM array |
| GBA/Slot-2 ROM window | Not applicable | `0x08000000` region |
| Boot hardware | PIF ROM `0x1FC00000`; PIF RAM `0x1FC007C0` | ARM9 BIOS window at `0xFFFF0000`; ARM7 BIOS at zero in its address space |

Sources: [N20], [N06], [B01], [D34].

A DS address is meaningful only with the **CPU, bank state, MPU attributes, and mode** specified. An ARM7 pointer into ARM9-local TCM is not valid shared memory. A source game's 32-bit “segmented address” is not automatically a CPU pointer on either machine.

## 8. Bandwidth: comparable and non-comparable numbers

### 8.1 Raw interfaces

| Path | Width / timing | Calculated ceiling | What the number does **not** mean |
|---|---|---:|---|
| N64 RDRAM physical transfer | 9 physical bits at 500 million transfers/s | **562.5 MB/s physical** | Not 562.5 MB/s of ordinary CPU bytes |
| N64 RDRAM payload equivalent | Eight ordinary bits of each nine-bit transfer | **500 MB/s payload** | Shared ceiling, not sustained CPU/RDP application throughput |
| N64 CPU–RCP SysAD path | 32-bit data path at 62.5 MHz | **250 MB/s** | Does not bypass RDRAM setup, arbitration, protocol, or cache behavior |
| DS main RAM sequential path | 16-bit bus, about 33.514 MHz, one sequential halfword per bus cycle | **67.028 MB/s** | Not random reads, cache misses, or RAM-to-RAM copy speed |
| DS local ARM9 32-bit access arithmetic | Four bytes per 67.028 MHz local access opportunity | **268.112 MB/s equivalent** | Not a DMA pipe; not a promise of that rate for a complete copy loop |
| DS Slot-1 fast transfer setting | Eight-bit transfers at bus clock / 5 | **6.703 MB/s** | Command gaps and actual card behavior reduce usable throughput |
| DS Slot-1 slower setting | Eight-bit transfers at bus clock / 8 | **4.189 MB/s** | Not an SD card/flashcart filesystem benchmark |
| DS Wi-Fi physical rates | 1 or 2 Mbit/s | **0.125 or 0.25 MB/s** before overhead | Not 11 Mbit/s application throughput |

Sources for inputs: [N01], [M01], [D03], [D34], [D38]. All MB/s figures above are calculated ceilings, not new hardware measurements.

For a like-for-like raw payload comparison, N64's 500 MB/s RDRAM path is approximately **7.46×** the DS main-RAM sequential ceiling. Comparing the N64 CPU-facing 250 MB/s interface gives approximately **3.73×**. Neither ratio is a prediction that every N64 loop runs that much faster.

### 8.2 Why a single bandwidth number is inadequate

| Contention / transfer issue | N64 | DS |
|---|---|---|
| Main-memory clients | CPU, RSP, RDP, VI, AI, PI, SI through RCP | Both CPUs and multiple DMA/peripheral clients |
| Graphics framebuffer traffic | Color/depth reads and writes consume RDRAM bandwidth | Internal 3D line/depth storage avoids the same full external framebuffer traffic |
| Texture access | Uploads to small TMEM; RDP samples locally | Renderer samples dedicated banked VRAM |
| CPU cache writebacks | Compete for memory bandwidth | Compete for memory bandwidth |
| Display activity | VI continuously reads output buffers | 2D/3D/capture accesses can contend for applicable VRAM banks |
| Small transfers | Setup and arbitration can dominate | Setup, nonsequential waits, and register access can dominate |
| RAM-to-RAM copy | Requires both reads and writes on shared resources | Same, with especially costly DMA direction alternation |

Sources: [N02], [N03], [N14], [D06], [D11], [B01].

A copy of `N` bytes is at least `N` bytes read plus `N` bytes written unless the architecture provides a special mechanism. Even an ideal shared 67 MB/s interface does not imply a 67 MB/s full read-and-write copy. Conversely, adding every device's advertised bandwidth together does not describe one shared bottleneck.

## 9. Memory latency and access patterns

### 9.1 DS measured access timing

GBATEK publishes the following observations with caches disabled where applicable. **All cycles in this table are approximately 33.514 MHz bus cycles.** The nanosecond column is calculated from that clock. These are memory-access timings, not complete instruction costs. [D03]

| ARM9 access | Bus cycles | Approximate time |
|---|---:|---:|
| Main RAM, nonsequential 32-bit read | 10 | 298 ns |
| Main RAM, sequential 32-bit read | 2 | 59.7 ns |
| Main RAM, nonsequential 16-bit read | 9 | 269 ns |
| Main RAM, sequential 16-bit read | 1 | 29.8 ns |
| WRAM/BIOS/I/O/OAM, nonsequential data access | 4 | 119 ns |
| Same group, sequential data access | 1 | 29.8 ns |
| VRAM/palette, nonsequential 32-bit read | 5 | 149 ns |
| VRAM/palette, sequential 32-bit read | 2 | 59.7 ns |
| ARM9 TCM/cache-hit local access | 0.5 | 14.9 ns |
| Main-RAM 32-byte cache-line refill | 23 | 686 ns |

The table's I/O row describes bus access, **not a guarantee that every peripheral command completes then**. FIFO backpressure, active rendering/capture, other bus masters, dirty evictions, and dependency interlocks can add time.

### 9.2 Fetching instructions is not identical to reading data

On ARM9, uncached external instruction fetches do not obtain the same fast sequential behavior as data streams. GBATEK reports nine bus cycles for an uncached main-RAM ARM opcode; Thumb fetches two 16-bit opcodes together, averaging 4.5 bus cycles when both are used. Branching away can lose that amortization. ARM7's local WRAM path is much friendlier than its access to shared main RAM. [D03]

**Porting implication:** an instruction-cache miss problem can resemble an arithmetic problem in a coarse profiler. Smaller dispatch loops, fewer cold branches, compact tables, and ITCM placement can beat replacing a few multiplies with assembly.

### 9.3 What can honestly be said about N64 latency

| Latency category | N64 | DS |
|---|---|---|
| Main CPU clock period | 10.667 ns, calculated | ARM9 14.919 ns; ARM7/bus 29.838 ns, calculated |
| Cached access | Datasheet describes single-cycle cache access paths; load-use effects remain | Local cache/TCM opportunity of one ARM9 clock |
| External RAM first-word latency | Depends on RCP arbitration, RDRAM state, access type, cache refill, and competing clients | Published measured examples above; still condition-dependent |
| Sequential transfer spacing | Much better than a fresh random transaction | One bus cycle per sequential 16-bit main-RAM beat |
| Dirty cache miss | May require eviction before/use alongside refill mechanisms | Same fundamental concern |
| Graphics buffer placement | Nintendo recommends considering RDRAM bank placement to improve rendering access latency | Bank role and contention matter in VRAM; main-RAM locality remains separate |
| Universal guaranteed random-read nanoseconds | Not established by the sources used here | No single number covers every CPU, mapping, client, and contention state |

Sources: [M01], [N14], [D03].

Do not substitute the often-repeated unspecific “N64 RAM latency is X ns” for a matching test. A comparison needs the same question: uncached load completion, cache-line fill, first beat, full burst, CPU pointer chase, or RDP service latency. Those are different measurements.

## 10. DMA, coherency, IPC, and synchronization

| Resource / behavior | Nintendo 64 | Nintendo DS |
|---|---|---|
| DMA organization | Specialized SP, PI, SI, AI, VI and graphics interfaces | Four general-purpose DMA channels per CPU, eight total |
| Audio DMA | AI reads mixed output buffers | Sixteen sound-source channels plus two capture paths, separate from general DMA |
| Transfer width | Interface-specific | General DMA supports 16- or 32-bit transfers |
| Transfer triggering | Interface/task dependent | Immediate and device/display events; available triggers differ by CPU |
| ARM9 DMA count | Not applicable | Up to 2^21 transfer units |
| Geometry streaming | RSP/RDP command paths | Geometry-FIFO-triggered DMA available |
| CPU access during DMA | Arbitration and target-dependent stalls | Bus access can stall; ARM9 can exploit cache/TCM where the required accesses remain local |
| Fast local memory DMA | RSP DMA directly services its local memories | General DMA cannot service ARM9 TCM |
| Shared-memory coherence | CPU cache maintenance required around device transfers | ARM9 cache maintenance required around DMA and ARM7 sharing |
| Dedicated inter-CPU mailbox | CPU/RSP task/status mechanisms | IPCSYNC plus FIFO in each direction |
| DS IPC FIFO capacity | Not applicable | 16 × 32-bit entries in each direction, 64 bytes each |
| Message notification | Device/task completion interrupts | FIFO and synchronization interrupts |

Sources: [N02], [N04], [N06], [D06], [D07], [D49].

**Critical DS DMA caveat:** for main-RAM-to-main-RAM DMA, reads and writes alternate between source and destination, so the memory accesses lose sequential timing. GBATEK explicitly warns about this case. This is why “DMA is always faster than the CPU” is false. A staged transfer through a suitable different memory can change the pattern, but it has extra passes and must be benchmarked. [D06]

### Ownership protocol for either system

A useful engineering pattern is: **finish producer writes → make them visible → hand ownership to the device/other processor → wait for completion → invalidate stale CPU copies as appropriate → consume**. The exact cache operation differs for a device reading versus writing. Handle dirty lines before a device writes; invalidate after completion before CPU consumption when necessary. Do not let the CPU modify a device-owned buffer midway through transfer.

An IPC FIFO message containing a pointer does not make the pointed-to payload coherent. TCM is not a valid shared payload location. DMA completion, FIFO empty, graphics idle, and “the frame is on the LCD” are also distinct events. [N06], [D07], [D14], [B02]

## 11. RSP versus DS geometry hardware

| Capability | N64 RSP / RDP | DS geometry / ARM7 |
|---|---|---|
| Programmable graphics-side instruction stream | RSP executes uploaded microcode | Geometry engine accepts fixed commands, not arbitrary programs |
| Scalar coprocessor execution | RSP has scalar MIPS-derived instructions | ARM7 is a separate general-purpose ARM CPU |
| Vector datapath | RSP operates on eight 16-bit vector lanes | No comparable eight-lane programmable vector coprocessor |
| Local program/data storage | 4 KiB IMEM plus 4 KiB DMEM | Geometry state is specialized; ARM7 has its own work RAM |
| Matrix transforms | Implemented by graphics microcode | Fixed matrix load/multiply/transform commands |
| Custom skinning or deformation | Can be implemented in RSP microcode within its constraints | CPU/precomputation, with fixed transforms used where applicable |
| Lighting model | Microcode-defined processing feeding RDP | Fixed vertex-lighting model with four directional lights |
| Audio processing | RSP can run audio microcode | Dedicated sound channels plus ARM7 software |
| Custom decompression/vector kernels | Possible on programmable RSP | CPU implementation; cannot upload code into geometry unit |
| Multiple tasks | CPU and RSP overlap; RSP tasks share one processor | ARM9/ARM7 overlap, but memory and peripheral ownership matter |
| Graphics command interpretation | RSP interprets microcode-specific display lists and generates RDP commands | Native geometry command stream consumed by fixed hardware |
| Modern programmable shaders | Neither RSP/RDP is a modern shader pipeline | No vertex/fragment shader programs |

Sources: [N02]–[N04], [N17], [D12], [D13], [D21].

Multiplying eight vector lanes by 62.5 MHz gives **500 million lane-operation opportunities per second** for an eligible idealized RSP vector instruction stream. That arithmetic is **not 500 MFLOPS**, not eight independently scheduled cores, and not an application benchmark. Loads, dependencies, microcode, DMA, and instruction mix matter.

For a native port, separate **work that can become DS geometry commands** from **work that actually needs programmable arithmetic**. The former can often be offloaded efficiently; the latter must be redesigned for ARM, moved offline, cached, or avoided. Sending matrix work back to ARM9 merely because the N64 implementation used RSP code can unnecessarily discard useful DS hardware.

## 12. Geometry capacities and command costs

### 12.1 Capacity and representation

| Property | Nintendo 64 | Nintendo DS |
|---|---|---|
| Basic rasterized primitives | Triangles and rectangles through RDP | Triangles, quads, triangle strips, quad strips |
| Primitive input organization | Microcode-specific vertex caches/display lists | Vertex commands plus polygon state |
| Fixed scene polygon capacity | No DS-style fixed whole-scene list limit | **2,048 polygons per active list** |
| Fixed scene vertex capacity | Microcode/local-memory limits are not a whole-frame equivalent | **6,144 stored vertices per active list** |
| Independent triangles at both DS limits | Not applicable | 2,048 × 3 = 6,144 vertices |
| Independent quads before vertex exhaustion | Not applicable | 1,536 × 4 = 6,144 vertices |
| Triangle strip for N triangles | Microcode dependent | N + 2 input vertices before clipping/other effects |
| Quad strip for N quads | Microcode dependent | 2N + 2 input vertices before clipping/other effects |
| Clipping impact | Adds setup/work; microcode-dependent | Can expand stored geometry and remove strip-sharing advantages |
| Backface culling | Supported through geometry state | Front/back rendering flags supported |
| Geometry buffering | Software/task managed | Two internal geometry sets exchanged on swap |
| Geometry FIFO | RSP/RDP-specific paths | 256 FIFO entries plus four pipeline entries |
| Physical FIFO entry | Not directly comparable | 40 bits: command tag plus 32-bit parameter |
| Packed command encoding | Typically 64-bit GBI commands, interpreted by selected microcode | Up to four command bytes packed into one 32-bit word, followed by parameters |
| FIFO-full CPU behavior | Interface-specific | Writes can stall waiting for space |
| Completion detection | RSP/DP status and interrupts | FIFO state, engine busy state, RAM usage, and swap state are separate |

Sources: [N15], [D13], [D14], [D17], [B04].

**2,048 is a polygon-storage limit, not simply a “triangle throughput” rating.** It does not imply 2,048 independent quads fit; those require too many vertices. Conversely, shared strips can permit more quads than independent-quad arithmetic suggests. Leave margin for clipped geometry rather than designing exactly at both limits.

### 12.2 Selected DS geometry-command execution times

These are published **33.514 MHz bus-cycle** command costs, not ARM9 instruction costs. Parameter delivery, FIFO waits, and some state-dependent additions are separate. [D15]

| Command / operation | Published cycles |
|---|---:|
| Matrix mode | 1 |
| Matrix push / store | 17 |
| Matrix pop / restore | 36 |
| Matrix identity | 19 |
| Load 4×4 / 4×3 matrix | 34 / 30 |
| Multiply 4×4 / 4×3 / 3×3 | 35 / 31 / 28 |
| Scale / translate | 22 / 22 |
| Normal / lighting command | Approximately 9–12, light-dependent |
| Full 16-bit-coordinate vertex command | 9 |
| Packed/partial/delta vertex commands | 8 |
| Color, UV, polygon attributes, texture parameters | 1 each |
| Light vector / light color | 6 / 1 |
| Shininess-table command | 32 |
| Begin / end | 1 / 1 |
| Box / position / vector test | 103 / 9 / 5 |
| Swap buffers | Wait until the relevant VBlank, then documented additional processing; not just a fixed “392-cycle draw” |

Position-plus-vector matrix mode adds work to applicable multiply/translate operations. Loading fewer words reduces command traffic but does not necessarily reduce fixed engine cost in the same proportion. Query/readback operations also introduce synchronization into otherwise streaming work.

### 12.3 Matrices, lights, and vertex formats

| Feature | N64 | DS |
|---|---|---|
| Matrix representation | Standard graphics microcode uses fixed-point matrices; CPU math may use FPU | 32-bit fixed-point matrix elements with 12 fractional bits |
| Matrix modes | Microcode-dependent stacks/operations | Projection, position, position-plus-vector, texture |
| Position/direction stack | Microcode and local-memory dependent | 31 documented usable stored slots, indexed 0–30; slot 31 produces overflow behavior |
| Projection stack | Microcode dependent | One stored level |
| Position input | Standard vertex formats and microcode-dependent transforms | 16-bit coordinates with 12 fractional bits, or compact alternatives |
| Packed position alternative | Microcode dependent | Three signed 10-bit coordinates with six fractional bits |
| Normal vector | Standard microcode vertex normal conventions | Signed 10-bit components with nine fractional bits |
| Material lighting | Microcode-defined | Diffuse, ambient, specular, emission |
| Light count | Depends on microcode and work budget | Four directional lights |
| Specular response | Microcode-defined | 128-entry shininess table / fixed lighting behavior |
| Arbitrary point-light attenuation | Can be programmed or computed on CPU | Not a programmable point-light shader; CPU approximation may be required |
| Weighted multi-bone skinning | Implementation-defined | No modern hardware skinning shader; CPU/AOT geometry or rigid matrix groups |

Sources: [N04], [N09], [D18]–[D21], [D51].

A fixed-point vertex coordinate range is not the world-size limit. World-to-view scaling and matrices choose how model coordinates map into representable ranges. Precision, overflow, and clipping still need explicit tests.

## 13. Rasterization, fill rate, and overdraw

### 13.1 N64 RDP theoretical peaks

| RDP cycle mode | Documented maximum pixel rate per RCP clock | Calculated at 62.5 MHz |
|---|---:|---:|
| Fill, 16-bit target | 4 pixels | 250 million pixels/s |
| Fill, 32-bit target | 2 pixels | 125 million pixels/s |
| Copy | 4 pixels | 250 million pixels/s |
| One-cycle rendering | 1 pixel | 62.5 million pixels/s |
| Two-cycle rendering | 0.5 pixel | 31.25 million pixels/s |

Source: [N07]. These are different operating modes, not five simultaneously available rates. They are ideal large-span/rectangle limits before realistic memory stalls and setup. The 250-million figure is not the rate of arbitrary fully textured, shaded, blended, depth-tested triangles.

### 13.2 Architectural differences that matter more than a marketing fill rate

| Property | N64 RDP | DS renderer |
|---|---|---|
| Render destination | Framebuffer/depth image in RDRAM | Internal scanline storage feeding main 2D engine |
| Persistent ordinary full 3D framebuffer | Yes, software-allocated | No full 192-line internal framebuffer |
| Rendering schedule | Work submitted to rasterizer; VI later scans buffer | Scanline-oriented, starts ahead of display |
| Temporary rendering lead | Buffered spans and external framebuffer behavior | Up to 48 scanlines of internal lead/cache |
| Overdraw bottleneck | Pixel work plus color/depth RDRAM traffic | Scanline service time and finite render lead |
| Expensive horizontal band | Costs frame rendering time | Can drain scanline lead even when whole-frame totals look reasonable |
| “Too much work” symptom | Missed frame deadline / lower update cadence | Geometry loss or rendering underflow can occur, depending on violated limit |
| Monitoring | RSP/RDP/VI timing and status | Polygon/vertex counts, geometry busy/FIFO status, RDLINES and underflow flags |
| Drawing at 30 rather than 60 updates/s | More time to complete a new framebuffer is often useful | More CPU/geometry preparation time, but old geometry is still rasterized every LCD refresh |
| One universal sustainable fill rate | No, rendering state and memory behavior matter | No, polygon distribution, overdraw, blending, and scanline deadlines matter |

Sources: [N07], [N08], [D12], [D14], [D16], [B04].

At the nominal DS refresh, one full visible 3D viewport represents about **2.941 million final displayed pixels per second**, calculated as `256 × 192 × 59.8261`. That is an **output demand**, not the renderer's shaded-pixel capacity. Rendering a pixel repeatedly, rejecting depth, handling transparency, and marking edges change the work.

**Three independent DS graphics gates:** the scene must fit geometry RAM; commands must finish in time; scanline rendering must keep ahead of the LCD. Passing any two does not prove the third. A frame can be under 2,048 polygons yet fail because many polygons overlap one region.

## 14. Texture formats, sizes, and residency

### 14.1 Storage and addressing

| Property | Nintendo 64 | Nintendo DS |
|---|---|---|
| Active texture resource | 4 KiB RDP TMEM | Up to 512 KiB banked VRAM |
| Larger source texture assets | Can reside in RDRAM/cartridge and be streamed/tiled into TMEM | Can reside in main RAM/card, but renderer samples active texture VRAM |
| Texture descriptors | Eight tile descriptors | Texture parameters and palette address associated with polygons |
| Texture dimensions | Tile/image addressing and TMEM packing; not a universal 32×32 maximum | Powers of two, 8 through 1,024 in each dimension |
| Non-power-of-two | Tiles/subimages possible; wrapping constraints still matter | Pad/atlas/subrect approaches; base texture size fields are powers of two |
| Base address granularity | Texture-load and TMEM alignment rules | Texture base in eight-byte units |
| Palette storage | TLUT consumes upper half of TMEM in color-indexed mode | Separate texture-palette VRAM roles, up to 96 KiB |
| Palette colors | RGBA16 or IA16 TLUT | RGB555 entries with format-specific transparency/alpha handling |
| CPU access while sampling | CPU prepares RDRAM source; RDP loads TMEM | Active texture mapping is not ordinary CPU upload mapping |
| Texture wrap/mirror/clamp | Supported through tile state | Repeat/flip/clamp controls |
| Texture upload synchronization | Explicit load/tile/pipeline synchronization | Bank ownership, rendering, and remapping synchronization |
| Texture memory double-buffered by geometry swap | No automatic replacement of all texture storage | **No**; swapping geometry does not swap textures |

Sources: [N10], [N16], [D04], [D12], [D23].

The DS has **128× as much maximum active texture storage as N64 TMEM**, calculated from 512 KiB / 4 KiB. That does not mean 128× the total texture assets or 128× the rendering capability. N64 streams a tiny working set from a much faster shared memory path; DS favors making a useful texture set resident.

### 14.2 Native texture formats

| Texture format / capability | N64 | DS |
|---|---|---|
| 16-bit direct color | RGBA5551 | RGB555 plus one-bit alpha |
| 32-bit direct color | RGBA8888 | No native equivalent |
| 8-bit color indices | CI8 | 256-color indexed |
| 4-bit color indices | CI4 | 16-color indexed |
| 2-bit color indices | No corresponding standard RDP CI2 format | Four-color indexed |
| Intensity-only | I4, I8 | No matching direct I-format; palettes can encode grayscale |
| Intensity plus alpha | IA4, IA8, IA16 | No same direct IA encoding; paletted alpha formats can approximate use cases |
| YUV | YUV16 with conversion support | No corresponding native 3D YUV texture format |
| Indexed color with per-texel alpha | Through available IA/TLUT/combiner choices, not DS A3I5/A5I3 | A3I5: three alpha bits + five index bits; A5I3: five alpha bits + three index bits |
| Block compression | No matching DS 4×4 block format | Native 4×4 compressed format |
| Transparent palette entry | Format/render-state dependent | Index-zero transparency selectable for relevant indexed modes |

Sources: [N15], [N10], [D24].

DS 4×4 compression uses **two bits per texel plus a 16-bit descriptor for each 4×4 block**: 48 bits for 16 texels, or **three bits per texel before palette storage**, calculated. Describing it as “2 bpp total” understates memory use. Image data and descriptors have particular VRAM-slot relationships; the largest compressed dimensions are 1,024×512 or 512×1,024, not a free 1,024×1,024 texture. It is not simply DXT/S3TC under another name. [D24]

### 14.3 Small worked examples

| Image / packing example | N64 TMEM implication | DS implication |
|---|---|---|
| 32×32 RGBA16 | 2 KiB, leaving room subject to layout/state | 2 KiB |
| 64×32 RGBA16 | 4 KiB, fills unpaletted TMEM | 4 KiB |
| 64×64 RGBA16 | 8 KiB: cannot all fit in 4 KiB TMEM simultaneously | 8 KiB |
| 32×32 RGBA32 | 4 KiB logical texture size | No native RGBA8888 texture |
| 64×64 CI4 / indexed 16-color | 2 KiB indices, plus TLUT constraints | 2 KiB indices plus palette |
| 512×512 direct 16-bit | 512 KiB source; must tile/stream | Fills all 512 KiB active texture capacity |
| 1,024×1,024 indexed 2-bit | No native CI2 equivalent | 256 KiB indices plus palette; size fields alone do not guarantee every other format fits |

All byte totals are calculations from dimensions and format depth. Real allocations also include alignment, palettes, compression descriptors, and any duplicates needed for safe streaming.

## 15. Filtering, color combining, and numerical precision

| Feature | Nintendo 64 | Nintendo DS |
|---|---|---|
| Point/nearest sampling | Supported | Native sampling behavior |
| Filtered texture sampling | RDP hardware filtering | No general hardware bilinear texture filter |
| Exact N64 filter behavior | “Bilinear” mode actually interpolates the nearest three texels, not ideal four-tap bilinear | No equivalent filter; texture art or additional work must compensate |
| Mipmapping | Hardware LOD and mipmapped tile selection/combination | No automatic hardware mipmap/LOD chain |
| Trilinear-like mip blending | Two-cycle combiner can combine adjacent levels | No native equivalent |
| Anisotropic texture filtering | No modern anisotropic filter | No |
| Perspective-correct texture coordinates | Supported | Supported |
| Texture-coordinate precision | RSP/RDP fixed-point pipeline conventions | Submitted UVs use signed 16-bit values with four fractional bits |
| General color combiner | Configurable `(A − B) × C + D`, independently for color/alpha; two cycles possible | Fixed modulation, decal, toon/highlight, shadow-related modes |
| Two texture inputs in a material | RDP two-cycle/tile combinations | No equivalent general dual-texture combiner |
| Primitive/environment colors | Separate combiner inputs | Not an equivalent set of arbitrary combiner inputs |
| Chroma key / noise inputs | RDP combiner facilities | No direct general equivalent |
| Vertex color storage | Standard RSP/RDP RGBA conventions; 8-bit RGB pipeline | RGB555 input expanded to internal RGB666 representation |
| Texture direct-color precision | Up to RGBA8888 | RGB555A1 |
| Final display precision | Framebuffer format, VI filtering/dither, and analog output chain are separate stages | Up to 18-bit RGB LCD output; 3D input/texture/capture precision can be lower |
| Gamma / dithering | VI/RDP features, mode-dependent | Not a drop-in replacement for N64 VI gamma/filter pipeline |

Sources: [N09]–[N15], [D20], [D23], [D25], [D01], [B04], [B05].

Nintendo explicitly documents that N64's familiar “bilinear” filter uses three nearest texels to save hardware, producing a triangulation bias on suitable patterns. Reproducing N64 screenshots with an ideal PC bilinear filter is therefore not automatically exact. [N11]

**Material-porting implication:** classify each source combiner into a supported DS material, an offline texture/palette bake, or an explicitly costed multipass case. A generic runtime interpreter for every source combiner can add CPU cost without providing hardware features that do not exist. Baked textures preserve particular lighting/color assumptions, so validate dynamic colors and alpha separately.

## 16. Depth, transparency, fog, antialiasing, and shadows

| Feature | N64 | DS |
|---|---|---|
| Depth storage model | Compressed depth plus delta information in RDRAM; extra graphics bits participate | Internal depth representation; Z- or W-buffer selection |
| Meaning of “16-bit vs 24-bit Z” | N64's physical/storage encoding is not plain uniform 16-bit linear Z | DS's 24-bit depth representation does not imply uniform 24-bit input precision |
| CPU-readable ordinary depth image | Stored in RDRAM, though encoded | Not exposed as an ordinary full-frame depth buffer in main RAM |
| Depth compare/write control | Render-mode dependent | Polygon flags control comparison and translucent depth behavior |
| Coplanar/decal handling | RDP surface modes and delta-depth behavior | Equal-depth tolerance/flags and ordering; not bit-identical |
| Polygon opacity | Combiner/blender alpha pathways | Five-bit polygon alpha; 31 opaque, 1–30 translucent, zero selects wireframe behavior |
| Translucent order | Correct compositing still requires suitable order | Automatic mode is Y-oriented ordering, **not a general back-to-front depth sort** |
| Polygon identity | RDP surface/coverage rules | Six-bit polygon ID affects translucent overlaps, shadows, edges |
| Alpha testing | Threshold and dither options | Global threshold with fixed renderer semantics |
| Vertex alpha | RSP/RDP shade alpha can participate, with fog interactions | Not an equivalent arbitrary per-vertex RGBA alpha pipeline |
| Fog | RSP/RDP cooperation; blender and cycle-mode implications | Hardware fog table/color with fixed behavior |
| Fog table | Different pipeline model | 32 density entries with interpolation |
| Polygon edge antialiasing | Coverage-based RDP plus VI processing | Hardware edge antialiasing with polygon/depth constraints |
| Edge outlining | Must construct desired effect | Dedicated edge marking; eight edge colors selected using polygon IDs |
| Toon shading | Can construct through source pipeline techniques | 32-entry toon table |
| Shadowing | Software/microcode/material techniques | Special shadow-polygon mode, not a general programmable stencil API |
| General stencil buffer/API | No modern generalized stencil interface | No modern generalized stencil interface |
| Multisample settings like a PC GPU | Not a modern selectable-MSAA model | Not a modern selectable-MSAA model |

Sources: [N13], [N14], [D16], [D20], [D22], [D25], [B05].

DS antialiasing concerns polygon edges; it does **not** add bilinear texture filtering. Polygon IDs are a functional rendering resource, not merely debugging labels. Reusing an ID indiscriminately across translucent objects can change results. N64 fog can consume shade-alpha information that a naïve material translation expects to use for transparency. [N13], [D20], [D22]

Do not assume a depth-buffer fix alone will repair alpha artifacts. Sorting, depth writes, face culling, polygon IDs, texture alpha, capture precision, and the source material equation are separate variables.

## 17. 2D engines, backgrounds, and sprites

| Resource / feature | N64 | DS |
|---|---|---|
| Independent tile-background engines | None in DS/GBA sense | Main engine A and sub engine B |
| Background layers | Built with RDP drawing/microcode | Up to four BG layers per 2D engine, mode-dependent |
| Relationship to 3D | RDP 2D and 3D share rasterization resources | Main BG0 can be the 3D layer; it replaces that 2D BG0 |
| Native tile maps | Software/microcode representation | Text, affine, and extended modes |
| Tile size | Source format/renderer choice | Standard 8×8 tile basis |
| Text BG dimensions | Software choice | 256×256, 512×256, 256×512, or 512×512 |
| Affine BG dimensions | Software choice | Up to 1,024×1,024 map dimensions |
| Extended bitmap BG sizes | Framebuffer/texture constraints | Up to 512×512, with engine/bank restrictions |
| Large bitmap mode | Software choice | Main engine only: 512×1,024 or 1,024×512 indexed bitmap |
| Native sprite/object entries | No fixed DS-style OAM table | 128 OAM object entries per engine |
| Affine object matrices | Software/microcode | 32 matrices per engine |
| Standard object dimensions | Renderer-defined | Hardware size/shape combinations from 8×8 through 64×64 |
| Object color formats | RDP texture formats | Indexed 4/8-bit and bitmap direct-color modes |
| Bitmap alpha | RDP combiner/blender | RGB555A1 pixels and object alpha rules |
| Sprite rotation/scaling | Geometry/microcode work | Affine OBJ hardware |
| Scrolling/windowing/mosaic | RDP/software effects | Dedicated 2D facilities |
| Layer blending / brightness | RDP/VI and software choices | Dedicated 2D blending and brightness controls |
| Per-scanline object work limit | No equivalent OAM evaluator | Finite object-rendering budget; 128 entries does not guarantee every object pixel fits |
| Second display engine supports 3D directly | No second display | No; only main-engine BG0 accepts live 3D |

Sources: [N07], [N15], [D26], [D27], [B06], [B07], [D11].

A DS HUD, menu, background, or label implemented with BG/OBJ hardware can avoid geometry-list pressure. That is still native hardware rendering. The tradeoffs are palette constraints, affine/object limits, bank allocation, and composition rules. Rendering all interface elements as 3D quads voluntarily spends the smallest geometry resource.

## 18. Displays, capture, scan timing, and frame budgets

### 18.1 Output architecture

| Property | N64 | DS / DS Lite |
|---|---|---|
| Physical display | External TV/monitor | Two built-in LCDs |
| Native LCD grid | Not applicable | 256×192 each; 98,304 total pixels |
| Screen diagonal | TV-dependent | Approximately 3 inches each |
| Common rendering modes | Around 320×240 and 640×480 interlaced; software/region dependent | Fixed visible grid |
| Official broad resolution description | Nintendo lists 256×224 through 640×480 with interlace support | Two fixed 256×192 grids |
| Scan frequency | NTSC/PAL and VI-mode dependent; not one universal exact 60 Hz | Approximately 59.8261 Hz |
| Analog video | Stock AV output; available signals depend on region/revision/cable | No standard stock TV-output connector |
| Pixel aspect/display scaling | VI and TV determine presentation | Fixed LCD grid; no TV overscan |
| Displayed color versus storage | Official specification lists 21-bit video output; RGBA32 storage is a different fact | 18-bit LCD output; RGB555 assets/capture are lower precision |
| Input-to-visible delay | Game scheduling + VI scan + TV processing | Game scheduling + geometry swap + scanline position + LCD response |

Sources: [N01], [N20], [D01], [D11]. Exact television processing delay and LCD response time require measuring the actual display chain.

### 18.2 DS display timing, calculated from documented clocks/counts

| Quantity | Value |
|---|---:|
| Nominal bus / ARM7 clock | 33,513,982 Hz |
| ARM9 clock | 67,027,964 Hz |
| Pixel clock | Bus / 6 ≈ 5.585664 MHz |
| Total dots per scanline | 355 |
| Visible dots per line | 256 |
| Total scanlines per refresh | 263 |
| Visible scanlines | 192 |
| Bus cycles per scanline | 2,130 |
| Bus cycles per refresh | **560,190** |
| ARM9 CPU cycles per refresh | **1,120,380** |
| Refresh period | **16.7151 ms** |
| Refresh frequency | **59.8261 Hz** |
| Two-refresh update period | **33.4302 ms** |
| Bus/timer cycles per two-refresh update | **1,120,380** |
| ARM9 CPU cycles per two-refresh update | **2,240,760** |
| Exactly every other refresh | **29.9130 new frames/s**, conventionally called “30 FPS” |

Inputs: [D03], [D11], [D09]. Counts and conversions are calculations.

**Important unit collision:** **1,120,380** is both the ARM9-cycle count for one display refresh and the bus/timer-tick count for two refreshes. A profiler using 33.514 MHz timers must not label that two-refresh budget “1.12 million ARM9 cycles.”

At one new list per refresh, capacity arithmetic gives approximately 122,524 polygon placements/s and 367,572 vertex slots/s. At every other refresh, the rate of **newly submitted** lists halves; the **per-list** capacities do not double. These products are storage-derived ceilings, not benchmarked render throughput.

### 18.3 Capture and dual-screen 3D

| Capability | N64 | DS |
|---|---|---|
| Render-to-image basis | Render into selected RDRAM color/depth buffers | Main-engine display capture into appropriately assigned VRAM |
| Capture sizes | Software framebuffer choice | 128×128, 256×64, 256×128, 256×192 |
| Capture pixel precision | Chosen framebuffer format | 15-bit RGB plus alpha flag behavior |
| Capturable source | Existing buffers/output arrangements | Main combined graphics or 3D-only; VRAM/main-memory FIFO and blending options |
| Capture source is sub engine | No equivalent split | Not directly; capture is main-engine capability |
| Captured depth texture | Depth exists in encoded RDRAM storage | Display capture is not general full-frame depth readback |
| Two simultaneous independent 3D engines | No | No |
| Common two-screen 3D approach | Not applicable | Alternate rendered views and capture/display the previous view |
| Cost of full DS capture | Not applicable | 96 KiB image payload; uses a suitable 128 KiB bank allocation |

Sources: [N14], [D28], [B04].

Alternating independently animated views typically yields roughly **29.913 updates/s per screen**, with scheduling/capture latency and bank costs. It is not two full-capability 60 Hz 3D pipelines. Also, capturing 18-bit 3D output into RGB555 reduces color precision.

## 19. Audio hardware and audio latency

| Property | Nintendo 64 | Nintendo DS / DS Lite |
|---|---|---|
| Mixing architecture | CPU prepares work; RSP audio microcode synthesizes/mixes; AI streams the resulting samples | ARM7 controls a dedicated 16-channel sound engine |
| Fixed hardware voice count | No useful universal count: RSP microcode, effects, sample rate, and graphics scheduling determine the voice budget | 16 hardware channels; software can mix additional voices into a channel at CPU cost |
| PCM sources | Software/RSP produces the AI output buffer | Each channel supports signed PCM8 or PCM16 |
| Compressed source decoding | Microcode/software; Nintendo audio compression is not the DS codec | Hardware IMA-ADPCM mode |
| Square-wave generation | Software/RSP synthesis | Six eligible channels, 8–13 |
| Noise generation | Software/RSP synthesis | Two eligible channels, 14–15 |
| PSG/noise channels additional to PCM channels | Not applicable | No; they occupy the same total of 16 channels |
| Per-voice pitch, pan, envelope | Defined by audio engine/microcode | Timer-controlled source rate, hardware volume and pan; envelopes/sweeps managed in software |
| Stereo output | Interleaved stereo sample stream through AI and the external output chain | Stereo speakers and headphone output |
| Source bit depth versus output precision | Conventional AI sample buffers use 16-bit samples per channel | PCM16 source does **not** imply a 16-bit final DAC; native DS output uses approximately 32.7 kHz, 10-bit PWM |
| Global sample rate | Programmable AI divider; actual rate is returned by the SDK | Individual source-channel rates feed the common output mixer |
| Output-buffer queue | AI provides two queued DMA address/length entries | Hardware channels fetch their own sample data; software streaming design is separate |
| Audio DMA granularity | AI buffer address and length require 8-byte alignment; retail length field is 18 bits, so maximum valid aligned length is just below 256 KiB | Channel source/loop data have word/alignment and format-specific requirements |
| Built-in microphone | None on the baseline console/controller | Yes; sampled through the ARM7-controlled SPI ADC path |
| Hardware sound capture | No equivalent DS capture pair | Two capture units, supporting PCM8/PCM16 destinations and selected channel/mixer sources |
| Native MP3/AAC decoder | No | No |

Sources: [N17]–[N19], [N21], [D29]–[D33], [D40].

**Do not equate all ADPCM formats.** An N64 game's compressed samples generally need decoding/re-encoding or a matching software decoder before use by DS IMA-ADPCM hardware. Keeping “ADPCM” in the filename does not establish compatibility.

The original `osAiSetFrequency` documentation accepts requested frequencies of **3,000–368,000 Hz for NTSC** and **3,050–376,000 Hz for PAL**, subject to its divider behavior. Therefore “48 kHz is the AI register's absolute maximum” is not an accurate hardware/API statement. Equally, the API range is **not** a guarantee that every setting gives ordinary full-precision, high-quality analog reproduction. Audio format, bit-clock configuration, output hardware, and useful fidelity must be distinguished. [N18]

For DS channels, the source-rate relationship is approximately:

```text
channel source rate = (33,513,982 / 2) / (65,536 - timer_reload)
```

GBATEK reports format-dependent channel-start delays of approximately **1 sample for PSG, 3 for PCM, and 11 for ADPCM**. These are sample-count delays, not a fixed millisecond latency for every sample rate. Capture has additional routing and documented hardware quirks; it is not a general-purpose, lossless replacement for a software mixer. [D30], [D32], [D33]

**Application latency:** a software buffer containing 1,024 stereo sample frames at 32,000 sample frames/s represents **32 ms of audio**, calculated. Queueing two such buffers, issuing sounds only on game updates, and delaying ARM7 commands can add latency independently of the DAC. Neither platform has one universally correct “audio latency in milliseconds.”

## 20. Cartridges, storage, saves, and boot

| Property | Nintendo 64 | Nintendo DS / DS Lite |
|---|---|---|
| Main game medium | Cartridge ROM accessed through the peripheral interface, PI | Slot-1 game card with a command/data protocol |
| Normal ROM access model | Cartridge address space is memory-mapped; DMA is important for bulk transfers | Slot-1 contents are **not** an ordinary CPU-executable memory-mapped array |
| Normal execution location | Time-critical code normally runs from RDRAM/cache | ARM9/ARM7 executables are loaded into accessible RAM; overlays can be loaded later |
| Transfer setup | PI timing, address, DMA length, arbitration, and cartridge behavior matter | Command issuance, transfer size, programmable gaps, card readiness, and ownership matter |
| Slot-1 data width | Not applicable | 8-bit card data bus |
| Slot-1 clock options | Not applicable | Approximately 6.703 MHz or 4.189 MHz, derived from bus clock /5 or /8 |
| Slot-1 gross byte-rate ceiling | Not applicable | Approximately **6.703 MB/s** or **4.189 MB/s**, before overhead |
| Slot-1 commands | Not applicable | 8-byte command packets; data-block size is configurable |
| Card transfer blocks | Device/interface dependent | Common block selections run from 512 B through 16 KiB; special short transfers also exist |
| Separate save interface | Save technology and interface depend on cartridge | Serial save interface; game-specific EEPROM, FRAM, flash, or NAND arrangements |
| Cartridge EEPROM examples | 4 Kbit = 512 B; 16 Kbit = 2 KiB | Capacity varies by save chip; do not assume one universal size |
| Controller-based save | Controller Pak, with its own filesystem and SI transport | No N64-style controller memory-card slot |
| Controller Pak allocation model | 256-byte allocation pages; up to 16 file entries in Nintendo's filesystem | Not applicable |
| Built-in user mass storage | None | None in the original DS/DS Lite baseline |
| Built-in SD/microSD slot | None | None; a flashcart's microSD controller is accessory hardware |
| Secondary cartridge slot | No GBA slot | Slot-2 GBA cartridge / accessory bus |
| Optional memory accessory | Expansion Pak expands the actual RDRAM pool | Official browser accessory provides 8 MiB through Slot-2; not fast main RAM |
| Boot system | PIF/lockout initialization and cartridge boot code | BIOS plus firmware boot/menu logic, then loading of ARM9/ARM7 images |
| Security/boot protocol as a performance feature | Not a compute accelerator | Not a compute accelerator; command encryption is not general graphics/crypto acceleration |
| Filesystem | Game-defined data layout and libraries | Game-defined/card layout; homebrew filesystem performance additionally depends on flashcart and driver |

Sources: [N03], [N20], [N22], [N24], [D05], [D34]–[D36], [D54], [B01].

The DS header stores separate ARM9/ARM7 ROM offsets, RAM destinations, entry points, sizes, and overlay tables. Its capacity field encodes **128 KiB shifted left by the field value**. That encoding is not evidence that every mathematically representable capacity is implemented by a real card. Likewise, a cartridge-address aperture must not be confused with the largest commercially manufactured cartridge. [D54]

A single reliable sustained “N64 cartridge MB/s” figure is not assigned here. PI timings, ROM chips, alignment, transfer length, and other bus traffic change the result. DS Slot-1's clock-derived limits are similarly **not** promises of filesystem read speed. Tiny random reads can be dominated by transaction setup even when the sequential rate is acceptable. [N03], [D34]

**Porting consequence:** ROM pointers, overlays, asset paging, and save calls need explicit adaptation. A memory-mapped N64 asset lookup is not automatically a cheap DS card read. Stage content, fighter closures, and animation blocks should be scheduled and batched around the actual card/flashcart path, with latency measured on that path.

## 21. Input, wireless, clocks, and peripherals

| Feature | Nintendo 64 | Nintendo DS / DS Lite |
|---|---|---|
| Direct local player interfaces | Four controller ports | One built-in set of controls; additional players normally use other systems over wireless |
| Directional input | D-pad and analog control stick | D-pad; no built-in analog stick |
| Face buttons | A, B, four C buttons | A, B, X, Y |
| Other standard game controls | L, R, Z, Start | L, R, Start, Select |
| Analog triggers | No | No |
| Controller acquisition path | PIF/SI serial transactions | Key registers plus ARM7-owned extended inputs and IPC where needed |
| Analog-stick report | Signed 8-bit X/Y fields; real mechanical range is smaller and varies | Not applicable |
| Touch surface | None | Lower-screen single-touch resistive panel |
| Touch conversion | Not applicable | ADC can use 12-bit or 8-bit conversion modes; calibration converts raw readings to screen coordinates |
| Touch pressure information | Not applicable | Z1/Z2 measurements can support pressure/contact estimation; not a guaranteed calibrated force sensor |
| Multitouch | None | No independent multitouch tracking |
| Microphone | Accessory-dependent, not baseline | Built in |
| Hinge/lid sensor | Not applicable | Available through ARM7 extended input state |
| Real-time calendar clock | No universal battery-backed calendar clock in the baseline console; accessory/cartridge possibilities are separate | Battery-powered calendar RTC with alarms; distinct from the CPU/timer clocks |
| Built-in wireless radio | None | 2.4 GHz radio with 802.11b-related and Nintendo local protocols |
| Documented DS Wi-Fi transmission rates | Not applicable | **1 or 2 Mbit/s** PHY selections; not a blanket 11 Mbit/s device |
| Application wireless throughput | Not applicable without accessories | Lower than PHY rate, depending on headers, ACKs, contention, retries, and protocol |
| Wireless packet RAM | None | 8 KiB dedicated Wi-Fi memory |
| Classic DS access-point security | Not applicable | WEP-era hardware/software capability; modern WPA2/WPA3 support must not be assumed |
| Bluetooth | None | None |
| Built-in infrared | None | None; special cartridge IR hardware is separate |
| GPS | None | None |
| Built-in camera | None | None; DSi is a different model |
| Accelerometer / gyroscope | None in standard controls | None built in; accessory sensors are separate |
| Rumble | Controller Rumble Pak accessory | Slot-2 Rumble Pak/accessory, not built-in console vibration |
| USB host/device port | None | None |
| Ethernet | None | None |
| Native external video output | Console AV output | No standard retail video-out port |

Sources: [N01], [N20], [N25], [D01], [D37]–[D40], [D48], [D50], [D54]. Accessory exclusions describe the unmodified baseline, not every product ever attached to either system.

Nintendo's controller documentation gives approximately **2 ms** for its read transaction to return controller data. This is a library/interface observation, **not** complete button-to-photon latency. It also explicitly warns that stick ranges differ among controllers and with wear; a signed 8-bit report does not provide 256 mechanically usable positions per axis. [N25]

On DS, the touchscreen ADC is also involved in microphone and related analog conversions, and SPI is a shared peripheral path. Touchscreen sample frequency, filtering, calibration, microphone scheduling, and ARM7-to-ARM9 delivery are software decisions. A 12-bit ADC does not turn a 256×192 panel into a 4096×4096 display. [D39], [D40]

**Networking implication:** radio bitrate does not directly establish a game-player limit or a deterministic packet deadline. Choose the communication protocol, synchronization model, packet size/rate, loss handling, and CPU ownership explicitly; measure worst-case delivery instead of budgeting from 2 Mbit/s alone. [D37], [D38]

## 22. Interrupts, timers, and scheduling

| Property | Nintendo 64 | Nintendo DS / DS Lite |
|---|---|---|
| CPU timer basis | CP0 COUNT advances at half the CPU clock | General-purpose hardware timers advance from approximately 33.514 MHz, before prescaling |
| Baseline timer tick | Approximately 21.333 ns, calculated | Approximately 29.838 ns, calculated |
| CPU count-register width | 32 bits | General-purpose timer counters are 16 bits |
| General-purpose timer count | CP0 count/compare mechanism plus software scheduling and device events | Four timers on ARM9 and four on ARM7 |
| Timer prescalers | Software/compare scheduling is not the DS timer model | 1, 64, 256, 1024 |
| Timer cascading | Software extension/overflow handling | Timers after timer 0 can count the previous timer's overflow |
| Timer interrupt | COUNT/COMPARE interrupt mechanism | Timer-overflow IRQ options |
| Interrupt delivery | CPU exception system plus RCP/device interrupt sources | Separate interrupt enable, pending, and master-enable state for each CPU |
| Graphics-related interrupts | VI, RSP/RDP completion and related device events | VBlank/HBlank/VCount and ARM9 geometry-FIFO-related events |
| Other asynchronous events | AI, PI, SI and system events | DMA, IPC, cartridge, keys; ARM7-owned SPI/Wi-Fi/RTC-related events |
| Core-to-core notification | CPU/RSP mailbox/semaphore/task mechanisms | IPC synchronization and FIFO interrupts |
| Interrupt latency | Depends on masking, current instruction, memory state, exception code, and scheduler | Same, with CPU ownership, cache/TCM placement, and DMA/bus effects |
| Operating-system scheduler | Game/runtime policy | Game/runtime policy; hardware does not supply a desktop-style scheduler |
| Fixed logic frequency | None dictated by the CPU hardware | None dictated by the CPU hardware |

Sources: [M02], [N20], [D07], [D09], [D10], [D49].

**Counter arithmetic:** at the nominal clocks, an N64 32-bit COUNT wraps after roughly **91.63 seconds**. An unprescaled DS 16-bit timer wraps after roughly **1.96 ms**. These are calculations, not recommendations to use an unextended counter for long profiling windows. Read cascaded timers safely, account for rollover, and subtract instrumentation overhead.

A software engine can run simulation, pose evaluation, particles, sound commands, and display-list production at different rates. That does not change the LCD refresh schedule or enlarge hardware geometry RAM. Choosing 30 Hz logic also changes simulation semantics unless the engine's time integration, input handling, collisions, and event scheduling are adapted; the clock hardware alone cannot guarantee equivalence.

## 23. Physical systems, revisions, and expansion

This section deliberately separates physical-product specifications from programmer-visible performance. A motherboard revision, brighter display, or larger battery is not automatically a faster CPU.

| Property | Nintendo 64 | Original DS | DS Lite |
|---|---|---|---|
| Product format | Stationary console | Folding handheld | Smaller folding handheld |
| Nominal enclosure dimensions | 260 × 190 × 73 mm | 148.7 × 84.7 × 28.9 mm, folded | 133.0 × 73.9 × 21.5 mm, folded |
| Weight directly established by the retrieved manufacturer specification | Approximately 1.1 kg | Not independently re-established from an accessible manufacturer document here | Approximately 218 g including battery/stylus, in Nintendo's indexed historical specification |
| CPU/geometry performance baseline | VR4300/RCP clocks above | Native NTR clocks and resources above | Same native NTR programming/performance class, not DSi enhanced hardware |
| Screens | External display | Two approximately 3-inch, 256×192 screens | Two approximately 3-inch, 256×192 screens |
| Power source | External mains power adapter | Rechargeable battery / adapter | Rechargeable battery / adapter |
| Guaranteed universal power draw | No; board revision, scene, accessories, and measurement point matter | No; brightness, sound, wireless, scene, and battery condition matter | Same qualifications |
| Guaranteed modern battery runtime | Not applicable | No; original advertising ranges do not predict an aged battery | Same qualification |
| Cooling / operating environment | Console enclosure; board and ambient conditions matter | Handheld thermal/power design | Handheld thermal/power design |
| Native main-memory expansion | Expansion Pak changes 4 MiB to 8 MiB | No equivalent drop-in main-RAM upgrade | No equivalent drop-in main-RAM upgrade |
| Expansion changes CPU clock | No | Slot-2 RAM does not change CPU clock | Same |
| Other model families | 64DD, iQue, development systems must be treated separately | DSi and development/debug systems are separate | DSi/DSi XL are not simply Lite display revisions |

Sources: [N01], [D01], [D11], [D36], [P01]. Nintendo's historical DS comparison survives in search-indexed specifications, but its original Japanese page currently redirects; the physical dimensions/DS Lite weight above are consequently lower-confidence archival references than the accessible technical manuals. They do not affect any performance calculation in this document.

**Not asserted as universal specifications:** semiconductor fabrication node, die area, transistor count, exact package revision, per-rail current, battery capacity across replacements, analog video quality by board revision, and LCD response time. Those require a particular motherboard/IC/panel/battery part number and corresponding evidence. An adapter's rated maximum output is not a measurement of a console's actual consumption.

Similarly, a DS Lite's different panel/backlight implementation does not provide extra polygon slots, larger caches, or faster ARM9 execution in the baseline considered here. A particular LCD's ghosting or response-time behavior must be measured separately from the approximately 16.715 ms display-refresh period.

## 24. Worked memory and throughput budgets

All numerical results in this section are calculations from the capacities/formats already sourced above. They are **not measured allocation totals or benchmark scores**.

### 24.1 Framebuffer and depth storage

| Allocation | Calculated payload | Important qualification |
|---|---:|---|
| N64 320×240 RGB16 color buffer | 150 KiB | Does not include alignment or other engine allocations |
| N64 320×240, two RGB16 color buffers + 16-bit depth storage | 450 KiB | Depth uses N64-specific encoding/extra graphics bits |
| N64 320×240, three RGB16 color buffers + depth | 600 KiB | Triple buffering is a software choice |
| N64 640×480 RGB16 color buffer | 600 KiB | Four times the 320×240 pixel count |
| N64 640×480, two RGB16 color buffers + depth | 1,800 KiB | A large share of a 4 MiB baseline |
| N64 640×480, two RGB32 color buffers + 16-bit depth storage | 3,000 KiB | Leaves little baseline RAM before code/assets/runtime |
| DS 256×192 RGB555 capture | 96 KiB | One suitable 128 KiB capture bank is required |
| DS two such capture images | 192 KiB of image data | Ordinarily consumes two 128 KiB banks, 256 KiB allocated |
| DS 256×256 RGB555 bitmap | 128 KiB | Allocation includes rows below the 192 visible lines |

DS's ordinary 3D rendering path does **not** require reproducing the N64's full RDRAM color/depth-buffer arrangement in main RAM. Conversely, double-screen capture can consume half of the four banks otherwise eligible for textures. [N14], [D04], [D12], [D28]

### 24.2 Texture-size examples

| Image / format | Payload or working-set calculation | Platform consequence |
|---|---:|---|
| 64×32 RGBA16 | 4,096 B | Fits N64 4 KiB TMEM before other layout constraints |
| 32×32 RGBA32 | 4,096 B | Same raw TMEM capacity |
| 64×64 CI4 indices | 2,048 B | N64 TLUT mode reserves the other TMEM half for palette organization |
| 256×256 DS direct-color16 | 128 KiB | Occupies one 128 KiB texture bank |
| 512×512 DS direct-color16 | 512 KiB | Occupies all available image-texture space |
| 1024×1024 DS direct-color16 | 2 MiB | Cannot be resident as one texture in the 512 KiB image pool |
| 1024×1024 2-bit indices | 256 KiB | Index storage fits; palette/format/bank restrictions still apply |
| DS 4×4 compressed block | 32 index bits + 16 metadata bits = 48 bits per 16 texels | **3 bits/texel before palette storage**, not simply 2 bits/texel |

Sources for format constraints: [N10], [N15], [D23], [D24].

### 24.3 Example DS bank pressure

One possible allocation can reserve A/B for **256 KiB of textures**, C for a **128 KiB sub-background mapping**, and D for a **128 KiB display-capture target**. E can provide a texture-palette allocation; F/G/H/I remain subject to their individual role restrictions. This example demonstrates a tradeoff, not a universal recommended layout. The important result is that a renderer using capture plus sub-screen content may have only half the headline texture-image capacity remaining. [D04]

Main RAM also needs an explicit budget. Account separately for ARM9 code/read-only data, initialized/static data, stacks, heap, ARM7-shared state, asset working sets, decompression buffers, display-list staging, filesystem buffers, audio samples, and safety headroom. Do not subtract an invented fixed “OS reserve” and call the rest universally available. Linker layout and runtime choices determine the real result. [N26], [B01]

### 24.4 Bandwidth-demand arithmetic

```text
Example: upload 128 KiB each new game frame

At 29.913 new frames/s:
    131,072 × 29.913 ≈ 3.92 MB/s of destination payload

At 59.826 new frames/s:
    131,072 × 59.826 ≈ 7.84 MB/s of destination payload
```

That is only payload. Reading the source, cache maintenance, remapping, DMA setup, and destination/bus contention are additional work. A main-RAM-to-main-RAM copy must transport both a read and a write; it does not copy 67 MB of useful payload each second merely because one bus direction has a 67 MB/s gross ceiling. [D03], [D06]

For uncompressed PCM16 stereo, sample traffic is `4 × sample_frames_per_second` bytes/s: **128 kB/s at 32 kHz** or **192 kB/s at 48 kHz**, calculated. Multiple source voices, resampling, effects, and mixing intermediates can make the memory traffic considerably greater than the final stereo stream.

## 25. Native-porting consequences

These are engineering conclusions from the hardware comparison, **not measured speedups or a promise that a particular redesign reaches a target frame rate**.

### 25.1 Preserve the computation's result, not its original machine implementation

A VR4300-oriented loop can rely on hardware floating point, 64-bit registers, and a larger instruction/data cache. The same C code on ARM9 can become software-FPU calls, pairs of 32-bit operations, and repeated external-memory misses. Inspect the generated ARM assembly rather than assuming source-level simplicity implies machine-level simplicity. Fixed point, bounded-width integers, precomputed transforms, lookup tables, and offline conversion are candidates where their precision/range preserve the required behavior. [M01], [M02], [A01], [B03]

### 25.2 Treat RAM layout as part of the algorithm

A linked object graph that is tolerable on N64 may be dominated by DS main-memory latency. Dense indices, compact hot records, linear command streams, cached traversal results, and separate cold metadata are ways to reduce required transactions. They are not universally interchangeable: alignment padding, cache conflicts, conversion overhead, and update frequency can reverse an expected benefit. Measure actual access patterns and cache misses. [D02], [D03]

### 25.3 Spend ITCM/DTCM on the measured hot working set

Place the smallest valuable code/data working sets in local memory, not whichever functions happen to be easiest to annotate. TCM has an opportunity cost and can introduce call/placement constraints. Keep DMA-visible exchange buffers out of CPU-private TCM, and give shared cache lines explicit ownership. [D02], [D06], [B01], [B02]

### 25.4 Use DS geometry hardware for operations it actually implements

The geometry engine can transform, light, clip, and project supported vertex streams. It does not run an RSP skinning, audio, decompression, collision, or display-list-decoding microprogram. Decide which work is offline, ARM9 fixed-point, ARM7-suitable, or directly expressible through geometry commands. Measure matrix/state overhead as well as vertex count. [N04], [D13], [D15], [D18], [D21]

### 25.5 Budget geometry, command throughput, and scanline rendering independently

A scene can be below 2,048 polygons and still fail through command cost or expensive scanline overdraw. It can also be simple to rasterize but overflow vertex storage. Keep counters for submitted/stored geometry, matrix and material changes, clipping growth, FIFO stalls, and rendering underflow/line-buffer state. Running gameplay at every other refresh does not merge two geometry banks into one larger scene. [D12]–[D16], [B04]

### 25.6 Convert assets to a native texture/material representation

N64 tile/TMEM/TLUT state is not a direct DS material format. Pre-resolve tile addressing, palettes, combine modes, animation tracks, geometry topology, and unsupported filtering/blending into the simplest DS representation that meets the visual requirement. Minimize per-frame parsing and conversion. Keep transparency order and depth semantics explicit; identical RGB values alone do not establish a visually equivalent material. [N10]–[N13], [D20], [D23]–[D25]

### 25.7 Let the 2D engines remove work from the 3D pipeline

A suitable HUD, menu, label, background, or sprite can use tiled/bitmap/OBJ hardware instead of consuming polygon slots and ARM9-submitted 3D commands. This is a resource trade: 2D work still needs VRAM/OAM/palette bandwidth and obeys priority, affine, blending, and per-scanline restrictions. It is not free, but it is different hardware with its own budget. [D26], [D27], [B06], [B07]

### 25.8 Use ARM7 only where ownership and communication costs make sense

ARM7 already has natural responsibility for sound, touchscreen/SPI, Wi-Fi, and system services. Moving a pointer-heavy workload into uncached shared main RAM can be worse than leaving it on ARM9. Suitable additional work has a compact local working set, coarse-grained messages, clear buffer ownership, and enough independent computation to amortize IPC. The correct comparison is end-to-end completion and ARM9 time saved, not the sum of the two clock frequencies. [D03], [D07], [D29], [D37], [D39]

### 25.9 Benchmark representative gameplay and tail latency

Profile cold scene entry, warm steady state, high-overdraw scenes, large geometry lists, audio/wireless activity, and storage streaming separately. Report median and tail frame cost, missed deadlines, and the actual measurement clock. A faster median with worse long stalls may not improve a “stable 30 FPS” requirement. Model-specific emulator timing is evidence only to the extent that its timing model has been validated for these access patterns. [D03], [B03], [B04]

## 26. Misleading claims corrected

The references in earlier sections support this correction table; the right column distinguishes real limits from marketing shorthand.

| Claim | Correction |
|---|---|
| “The DS has a 100 MHz CPU: 67 + 33.” | It has two different CPUs with different resources and responsibilities. A single ARM9 task does not execute at their summed clock. |
| “The N64 is 64-bit, so it is twice as fast.” | Register width is not an overall performance ratio. Workload, instructions, caches, FPU, memory, and graphics architecture matter. |
| “Both have 4 MB RAM, so a port will fit unchanged.” | Memory roles, working sets, framebuffers, code size, asset conversion, and optional N64 Expansion Pak usage differ. |
| “N64 has 4.5 MB of normal usable game RAM.” | The additional ninth-bit storage is graphics-related, not a general CPU byte-addressable heap. |
| “N64 CPU can stream ordinary data at 562.5 MB/s.” | That physical RDRAM bit-rate accounting includes ninth bits and is not CPU sustained bandwidth; the CPU's own interface is another constraint. |
| “DS main RAM is 67 MB/s, so every access is fast.” | That is a gross sequential ceiling. First-access latency, read/write alternation, CPU instruction access, and contention matter. |
| “DS DMA is always faster than CPU copies.” | Setup, alignment, buffer size, memory endpoints, cache maintenance, and nonsequential main-to-main DMA behavior can overturn that assumption. |
| “DTCM is the best DMA buffer.” | DS DMA cannot directly access ARM9 TCM. |
| “ARM9 cache coherency is handled by volatile.” | Volatile is not cache clean/invalidate, a write-buffer drain, or a cross-CPU ownership protocol. |
| “ARM7 is the DS's RSP.” | ARM7 is another scalar general-purpose processor, not an eight-lane programmable vector unit. |
| “DS has no geometry acceleration.” | Its fixed engine provides substantial transform, lighting, clipping, projection, and rasterization support. |
| “DS's 656 KiB VRAM can all hold 3D textures.” | Active texture-image roles are limited to A–D, totaling 512 KiB; bank use also competes with other features. |
| “N64 can only have 4 KiB of textures in an entire level.” | TMEM is the active working store. Other textures can remain in RDRAM and be loaded as needed. |
| “DS 4×4 compressed textures cost exactly 2 bits/texel.” | Index data is 2 bits/texel, but block metadata raises it to 3 bits/texel before palettes. |
| “DS's polygon limit is 2,048 triangles.” | It is 2,048 stored polygons; native quads and clipping/storage behavior must be counted correctly. |
| “30 FPS allows 4,096 DS polygons in one scene.” | Per-list storage remains 2,048 polygons / 6,144 vertices. Two-refresh updates do not combine the two list banks. |
| “122k polygons/s guarantees that throughput for any scene.” | That product of list capacity and refresh rate is not a measured geometry/raster throughput guarantee. |
| “An empty FIFO means the scene is on screen.” | Command buffering, geometry execution, swap timing, and scanline rendering are distinct stages. |
| “Two DS screens mean two 3D engines.” | There is one 3D engine. Capture/alternation and 2D presentation are separate techniques. |
| “N64 texture filtering is ordinary PC bilinear.” | Its standard triangular three-point interpolation differs from four-sample bilinear filtering. |
| “DS has no antialiasing.” | It has specific polygon-edge antialiasing; this is not a general MSAA specification. |
| “DS has a fully programmable GPU.” | Its graphics pipeline is fixed function; it does not expose vertex/pixel shaders or arbitrary RSP microcode. |
| “PCM16 means DS output has 16-bit DAC precision.” | Source format and final approximately 10-bit PWM output are different specifications. |
| “802.11b means DS transmits at 11 Mbit/s.” | The documented DS hardware rate selections are 1 and 2 Mbit/s. |
| “A frame always gives 16.667 ms on DS.” | Native refresh is approximately 16.715 ms; exactly every other refresh is approximately 29.913 FPS. |
| “1,120,380 ticks uniquely identifies a CPU-cycle budget.” | It is ARM9 cycles per refresh **or** bus/timer ticks per two refreshes. The unit must be specified. |
| “LCD refresh period equals input latency.” | Polling, simulation, queueing, scanout position, and pixel response all contribute separately. |

## 27. What must be measured rather than assumed

A useful exhaustive reference includes boundaries of the evidence. The following properties cannot be reduced to one trustworthy universal number without defining a test.

### 27.1 CPU/memory measurements

Specify the processor, code placement, data placement, cacheability, cache warmth, alignment, stride, transfer width, access dependency, read/write pattern, DMA activity, competing core, display/capture state, timer source, and instrumentation overhead. A dependent pointer chase estimates a different property from an unrolled sequential load loop; a copy benchmark is different again. [M01], [D02], [D03], [D06]

For N64, distinguish CPU-to-RDRAM access through the RCP from RDP/RSP traffic, cache-line refill versus an uncached load, and physical ninth-bit transfer accounting. RDRAM row/bank behavior, arbitration, and graphics traffic prevent a single generic “RAM latency” from predicting all accesses. For DS, the published GBATEK table gives useful measured access-pattern cases, not a promise that a whole C load expression always completes in the tabulated bus cycles. [N03], [N14], [D03]

### 27.2 Graphics measurements

For N64, record RSP microcode, transformation/lighting/clipping work, RDP cycle type, framebuffer format, depth operations, texturing/filtering, blending/AA, and TMEM loads. “Triangles/s” without those conditions is under-specified. [N04], [N07]–[N14]

For DS, record polygon and vertex occupancy, matrix/normal/light command counts, clipping, primitive sizes, texture format/bank placement, translucency, overdraw, and scanline distribution. Measure command production and hardware rendering separately. A scene concentrated on a few scanlines may be worse than the same aggregate polygon count spread across the screen. A frame-average pixel-rate number can hide scanline deadline failure. [D12]–[D16], [B04]

### 27.3 Latency measurements

| Latency being asked about | Define these endpoints and conditions |
|---|---|
| CPU instruction | Operand readiness to result availability; include or exclude fetch/memory stalls explicitly |
| Cache miss | Load issue, critical-word arrival, and complete line fill are different endpoints |
| DMA | Setup-to-completion versus active-transfer-only; include or exclude cache preparation |
| GPU submission | CPU command production, FIFO acceptance, geometry completion, swap, and visible scanout differ |
| Input | Physical switch/touch event → sample → game consumption → rendered response → emitted light |
| Audio | Event generation → mixer/IPC → queued sample → DAC → acoustic output |
| Wireless | Payload queued → actual transmission → acknowledged reception → gameplay application |
| Storage | Read request → driver/card transaction → decompression → cache-visible usable asset |
| Display | VBlank timing, top/bottom scanout, LCD response or external TV/scaler delay |

No new hardware constants are implied by this table; it defines measurement boundaries.

### 27.4 Revision-specific electrical and physical data

A complete electrical characterization requires the actual board revision and component markings. RAM refresh implementation, oscillator tolerance, rail power, signal integrity, DAC behavior, LCD panel response, battery health, and accessory electrical loading are not reliably inferred from “N64” or “DS” alone. Where no accessible primary measurement or part-specific specification was established, this reference deliberately does not manufacture an exact number.

### 27.5 Acceptance criteria for a trustworthy comparison

Use a repeatable workload and state exactly what was measured. Separate **hardware/documented capacities**, **calculated theoretical ceilings**, **published measured timings**, and **your application's measured performance**. Preserve the same visual and gameplay requirements when comparing implementations. Record both typical performance and deadline failures, not only a best-case synthetic number.

**Overall conclusion:** N64 offers a stronger general-purpose numeric feature set, programmable vector processing, and a much higher-bandwidth shared graphics-memory path. DS offers useful CPU-local memory, a capable specialized geometry engine, much larger directly resident texture-image capacity, and dedicated 2D/audio/peripheral engines. The practical winner depends on which of those resources the implementation uses—and on whether its memory traffic and synchronization fit the actual hardware. [N01], [N04], [M01], [D01]–[D04], [D12], [D26], [D29]

## 28. Source index

**Source hierarchy:** original manufacturer manuals/datasheets establish specified hardware behavior; GBATEK reports first-hand reverse engineering and timing observations; BlocksDS documents practical native development. Mirrors host the original Nintendo/MIPS/NEC documents. A mirror is not the document's author. The supplementary N64 emulator-development notes are incomplete and are not used as a universal performance authority.

Dates in archived manuals describe those hardware revisions, not a new 2026 hardware revision. Modern documentation can be corrected over time. The calculations in this reference use explicitly stated nominal clocks and are not claimed to be new measurements. In particular, this work did not run new console benchmarks.

Some archived pages contain tables or figures as images. The arithmetic-timing tables in the NEC manual and the MIPS cache description were checked against the rendered PDF pages rather than inferred only from extracted text.

### Nintendo, CPU manufacturers, and supplementary N64 documentation

| Reference | Document |
|---|---|
| [A01] | ARM9E-S Technical Reference Manual: ARMv5TE core and execution architecture |
| [M01] | MIPS R4300i datasheet: caches, interfaces, CPU organization |
| [M02] | NEC VR4300 user manual U10504EJ7V0UMJ1: ISA, integer/FPU timing, system architecture |
| [N01] | Nintendo: Nintendo 64 technical details |
| [N02] | Nintendo Programming Manual §3.1: Hardware architecture / CPU and RCP |
| [N03] | Nintendo Programming Manual §3.2: Memory and system interfaces |
| [N04] | Nintendo Programming Manual §3.3: Reality Signal Processor |
| [N05] | Nintendo Programming Manual §3.5: CPU |
| [N06] | Nintendo Programming Manual §3.6: Memory management and cache coherency |
| [N07] | Nintendo Programming Manual §12.1: RDP pipeline and cycle types |
| [N08] | Nintendo Programming Manual §12.2: Synchronization |
| [N09] | Nintendo Programming Manual §12.3: Rasterizer |
| [N10] | Nintendo Programming Manual §12.4: Texture memory and lookup |
| [N11] | Nintendo Programming Manual §12.5: Texture filtering |
| [N12] | Nintendo Programming Manual §12.6: Color combiner |
| [N13] | Nintendo Programming Manual §12.7: Blender |
| [N14] | Nintendo Programming Manual §12.8: Framebuffers and memory organization |
| [N15] | Nintendo Programming Manual §13.1: Texture formats |
| [N16] | Nintendo Programming Manual §13.3: Texture coordinates and tiling |
| [N17] | Nintendo Programming Manual §17.1: Audio processing architecture |
| [N18] | Nintendo API: osAiSetFrequency — AI divider/frequency range |
| [N19] | Nintendo API: osAiSetNextBuffer — AI buffer alignment and limits |
| [N20] | N64 first-hand emulator-development hardware notes; supplementary address/register reference |
| [N21] | Nintendo API: osAiGetStatus — AI DMA queue status |
| [N22] | Nintendo API: osEepromProbe — 4 Kbit and 16 Kbit cartridge EEPROM |
| [N23] | Nintendo API: osPfsInitPak — Controller Pak initialization |
| [N24] | Nintendo API: osPfsAllocateFile — Controller Pak allocation units and file count |
| [N25] | Nintendo Programming Manual §26.2: Standard controller, acquisition timing, stick range |
| [N26] | Nintendo Programming Manual §4.1: Runtime and memory management |

### GBATEK first-hand hardware reference

| Reference | Topic |
|---|---|
| [D01] | GBATEK: DS technical data |
| [D02] | GBATEK: DS cache and TCM |
| [D03] | GBATEK: DS measured memory timings |
| [D04] | GBATEK: DS VRAM banks and mappings |
| [D05] | GBATEK: DS cartridge/main-RAM control |
| [D06] | GBATEK: DS DMA transfers |
| [D07] | GBATEK: DS IPC FIFO and synchronization |
| [D08] | GBATEK: DS hardware divider and square root |
| [D09] | GBATEK: DS timers and clock |
| [D10] | GBATEK: Timer registers, prescaling and cascade behavior shared by DS |
| [D11] | GBATEK: DS video architecture and scan timing |
| [D12] | GBATEK: DS 3D overview and buffering |
| [D13] | GBATEK: DS geometry commands and FIFO |
| [D14] | GBATEK: DS geometry/RAM/scanline status |
| [D15] | GBATEK: DS 3D command map and execution times |
| [D16] | GBATEK: DS 3D display/swap control |
| [D17] | GBATEK: DS vertex formats and polygon definitions |
| [D18] | GBATEK: DS matrix load/multiply |
| [D19] | GBATEK: DS matrix stacks |
| [D20] | GBATEK: DS polygon attributes, depth and alpha |
| [D21] | GBATEK: DS lighting/material parameters |
| [D22] | GBATEK: DS toon, edge, fog, blending and antialiasing |
| [D23] | GBATEK: DS texture addressing and attributes |
| [D24] | GBATEK: DS texture formats and compression |
| [D25] | GBATEK: DS fixed texture blending |
| [D26] | GBATEK: DS background modes |
| [D27] | GBATEK: DS objects/sprites |
| [D28] | GBATEK: DS capture and main-memory display FIFO |
| [D29] | GBATEK: DS sound overview |
| [D30] | GBATEK: DS sound-channel formats and timers |
| [D31] | GBATEK: DS mixer and PWM output |
| [D32] | GBATEK: DS sound timing and behavior notes |
| [D33] | GBATEK: DS sound capture |
| [D34] | GBATEK: DS card protocol, clocks and I/O |
| [D35] | GBATEK: DS save technologies |
| [D36] | GBATEK: DS Slot-2 expansion RAM |
| [D37] | GBATEK: DS wireless communications |
| [D38] | GBATEK: DS Wi-Fi packet headers and physical rates |
| [D39] | GBATEK: DS SPI peripheral bus |
| [D40] | GBATEK: DS touchscreen/microphone ADC |
| [D42] | GBATEK: ARM instruction cycle formulas |
| [D43] | GBATEK: DS BIOS access control |
| [D44] | GBATEK: ARM register set and banking |
| [D45] | GBATEK: ARM alignment behavior |
| [D46] | GBATEK: ARM architecture-version differences |
| [D48] | GBATEK: DS real-time clock |
| [D49] | GBATEK: DS interrupts |
| [D50] | GBATEK: DS buttons and extended input |
| [D51] | GBATEK: DS matrix types and representations |
| [D54] | GBATEK: DS card header, executable/overlay layout and capacity field |

### Native development and archival physical specifications

| Reference | Document |
|---|---|
| [B01] | BlocksDS: memory map and reserved/usable regions |
| [B02] | BlocksDS: TCM, caches and DMA coherency |
| [B03] | BlocksDS: optimization guide |
| [B04] | BlocksDS: 3D rendering, capacities and scanline monitoring |
| [B05] | BlocksDS: advanced 3D, depth and shadows |
| [B06] | BlocksDS: sprites and affine objects |
| [B07] | BlocksDS: special 2D effects |
| [P01] | Nintendo Japanese historical DS/DS Lite specifications — search-indexed archival dimensions; original page redirects |

### Reference links

[A01]: https://documentation-service.arm.com/static/5e8e2f18fd977155116a77fb "ARM9E-S Technical Reference Manual: ARMv5TE core and execution architecture"
[B01]: https://blocksds.skylyrac.net/docs/internal/memory_map/ "BlocksDS: memory map and reserved/usable regions"
[B02]: https://blocksds.skylyrac.net/tutorial/intermediate/tcm_and_cache/ "BlocksDS: TCM, caches and DMA coherency"
[B03]: https://blocksds.skylyrac.net/docs/guides/optimization_guide/ "BlocksDS: optimization guide"
[B04]: https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/ "BlocksDS: 3D rendering, capacities and scanline monitoring"
[B05]: https://blocksds.skylyrac.net/tutorial/advanced/advanced_3d/ "BlocksDS: advanced 3D, depth and shadows"
[B06]: https://blocksds.skylyrac.net/tutorial/basic/sprites/ "BlocksDS: sprites and affine objects"
[B07]: https://blocksds.skylyrac.net/tutorial/intermediate/special_2d_effects/ "BlocksDS: special 2D effects"
[D01]: https://problemkaputt.de/gbatek-ds-technical-data.htm "GBATEK: DS technical data"
[D02]: https://problemkaputt.de/gbatek-ds-memory-control-cache-and-tcm.htm "GBATEK: DS cache and TCM"
[D03]: https://problemkaputt.de/gbatek-ds-memory-timings.htm "GBATEK: DS measured memory timings"
[D04]: https://problemkaputt.de/gbatek-ds-memory-control-vram.htm "GBATEK: DS VRAM banks and mappings"
[D05]: https://problemkaputt.de/gbatek-ds-memory-control-cartridges-and-main-ram.htm "GBATEK: DS cartridge/main-RAM control"
[D06]: https://problemkaputt.de/gbatek-ds-dma-transfers.htm "GBATEK: DS DMA transfers"
[D07]: https://problemkaputt.de/gbatek-ds-inter-process-communication-ipc.htm "GBATEK: DS IPC FIFO and synchronization"
[D08]: https://problemkaputt.de/gbatek-ds-maths.htm "GBATEK: DS hardware divider and square root"
[D09]: https://problemkaputt.de/gbatek-ds-timers.htm "GBATEK: DS timers and clock"
[D10]: https://problemkaputt.de/gbatek-gba-timers.htm "GBATEK: Timer registers, prescaling and cascade behavior shared by DS"
[D11]: https://problemkaputt.de/gbatek-ds-video-stuff.htm "GBATEK: DS video architecture and scan timing"
[D12]: https://problemkaputt.de/gbatek-ds-3d-overview.htm "GBATEK: DS 3D overview and buffering"
[D13]: https://problemkaputt.de/gbatek-ds-3d-geometry-commands.htm "GBATEK: DS geometry commands and FIFO"
[D14]: https://problemkaputt.de/gbatek-ds-3d-status.htm "GBATEK: DS geometry/RAM/scanline status"
[D15]: https://problemkaputt.de/gbatek-ds-3d-i-o-map.htm "GBATEK: DS 3D command map and execution times"
[D16]: https://problemkaputt.de/gbatek-ds-3d-display-control.htm "GBATEK: DS 3D display/swap control"
[D17]: https://problemkaputt.de/gbatek-ds-3d-polygon-definitions-by-vertices.htm "GBATEK: DS vertex formats and polygon definitions"
[D18]: https://problemkaputt.de/gbatek-ds-3d-matrix-load-multiply.htm "GBATEK: DS matrix load/multiply"
[D19]: https://problemkaputt.de/gbatek-ds-3d-matrix-stack.htm "GBATEK: DS matrix stacks"
[D20]: https://problemkaputt.de/gbatek-ds-3d-polygon-attributes.htm "GBATEK: DS polygon attributes, depth and alpha"
[D21]: https://problemkaputt.de/gbatek-ds-3d-polygon-light-parameters.htm "GBATEK: DS lighting/material parameters"
[D22]: https://problemkaputt.de/gbatek-ds-3d-toon-edge-fog-alpha-blending-anti-aliasing.htm "GBATEK: DS toon, edge, fog, blending and antialiasing"
[D23]: https://problemkaputt.de/gbatek-ds-3d-texture-attributes.htm "GBATEK: DS texture addressing and attributes"
[D24]: https://problemkaputt.de/gbatek-ds-3d-texture-formats.htm "GBATEK: DS texture formats and compression"
[D25]: https://problemkaputt.de/gbatek-ds-3d-texture-blending.htm "GBATEK: DS fixed texture blending"
[D26]: https://problemkaputt.de/gbatek-ds-video-bg-modes-control.htm "GBATEK: DS background modes"
[D27]: https://problemkaputt.de/gbatek-ds-video-objs.htm "GBATEK: DS objects/sprites"
[D28]: https://problemkaputt.de/gbatek-ds-video-capture-and-main-memory-display-mode.htm "GBATEK: DS capture and main-memory display FIFO"
[D29]: https://problemkaputt.de/gbatek-ds-sound.htm "GBATEK: DS sound overview"
[D30]: https://problemkaputt.de/gbatek-ds-sound-channels-0-15.htm "GBATEK: DS sound-channel formats and timers"
[D31]: https://problemkaputt.de/gbatek-ds-sound-control-registers.htm "GBATEK: DS mixer and PWM output"
[D32]: https://problemkaputt.de/gbatek-ds-sound-notes.htm "GBATEK: DS sound timing and behavior notes"
[D33]: https://problemkaputt.de/gbatek-ds-sound-capture.htm "GBATEK: DS sound capture"
[D34]: https://problemkaputt.de/gbatek-ds-cartridge-i-o-ports.htm "GBATEK: DS card protocol, clocks and I/O"
[D35]: https://problemkaputt.de/gbatek-ds-cartridge-backup.htm "GBATEK: DS save technologies"
[D36]: https://problemkaputt.de/gbatek-ds-cart-expansion-ram.htm "GBATEK: DS Slot-2 expansion RAM"
[D37]: https://problemkaputt.de/gbatek-ds-wireless-communications.htm "GBATEK: DS wireless communications"
[D38]: https://problemkaputt.de/gbatek-ds-wifi-hardware-headers.htm "GBATEK: DS Wi-Fi packet headers and physical rates"
[D39]: https://problemkaputt.de/gbatek-ds-serial-peripheral-interface-bus-spi.htm "GBATEK: DS SPI peripheral bus"
[D40]: https://problemkaputt.de/gbatek-ds-touch-screen-controller-tsc.htm "GBATEK: DS touchscreen/microphone ADC"
[D42]: https://problemkaputt.de/gbatek-arm-cpu-instruction-cycle-times.htm "GBATEK: ARM instruction cycle formulas"
[D43]: https://problemkaputt.de/gbatek-ds-memory-control-bios.htm "GBATEK: DS BIOS access control"
[D44]: https://problemkaputt.de/gbatek-arm-cpu-register-set.htm "GBATEK: ARM register set and banking"
[D45]: https://problemkaputt.de/gbatek-arm-cpu-memory-alignments.htm "GBATEK: ARM alignment behavior"
[D46]: https://problemkaputt.de/gbatek-arm-cpu-versions.htm "GBATEK: ARM architecture-version differences"
[D48]: https://problemkaputt.de/gbatek-ds-real-time-clock-rtc.htm "GBATEK: DS real-time clock"
[D49]: https://problemkaputt.de/gbatek-ds-interrupts.htm "GBATEK: DS interrupts"
[D50]: https://problemkaputt.de/gbatek-ds-keypad.htm "GBATEK: DS buttons and extended input"
[D51]: https://problemkaputt.de/gbatek-ds-3d-matrix-types.htm "GBATEK: DS matrix types and representations"
[D54]: https://problemkaputt.de/gbatek-ds-cartridge-header.htm "GBATEK: DS card header, executable/overlay layout and capacity field"
[M01]: https://datasheets.chipdb.org/MIPS/R4300i_datasheet.pdf "MIPS R4300i datasheet: caches, interfaces, CPU organization"
[M02]: https://datasheets.chipdb.org/NEC/Vr-Series/Vr43xx/U10504EJ7V0UMJ1.pdf "NEC VR4300 user manual U10504EJ7V0UMJ1: ISA, integer/FPU timing, system architecture"
[N01]: https://www.nintendo.com/en-gb/Hardware/Nintendo-History/Nintendo-64/Technical-Details/Technical-Details-627050.html "Nintendo: Nintendo 64 technical details"
[N02]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro03/03-01.html "Nintendo Programming Manual §3.1: Hardware architecture / CPU and RCP"
[N03]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro03/03-02.html "Nintendo Programming Manual §3.2: Memory and system interfaces"
[N04]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro03/03-03.html "Nintendo Programming Manual §3.3: Reality Signal Processor"
[N05]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro03/03-05.html "Nintendo Programming Manual §3.5: CPU"
[N06]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro03/03-06.html "Nintendo Programming Manual §3.6: Memory management and cache coherency"
[N07]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-01.html "Nintendo Programming Manual §12.1: RDP pipeline and cycle types"
[N08]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-02.html "Nintendo Programming Manual §12.2: Synchronization"
[N09]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-03.html "Nintendo Programming Manual §12.3: Rasterizer"
[N10]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-04.html "Nintendo Programming Manual §12.4: Texture memory and lookup"
[N11]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-05.html "Nintendo Programming Manual §12.5: Texture filtering"
[N12]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-06.html "Nintendo Programming Manual §12.6: Color combiner"
[N13]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-07.html "Nintendo Programming Manual §12.7: Blender"
[N14]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-08.html "Nintendo Programming Manual §12.8: Framebuffers and memory organization"
[N15]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro13/13-01.html "Nintendo Programming Manual §13.1: Texture formats"
[N16]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro13/13-03.html "Nintendo Programming Manual §13.3: Texture coordinates and tiling"
[N17]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro17/17-01.html "Nintendo Programming Manual §17.1: Audio processing architecture"
[N18]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osAiSetFrequency.html "Nintendo API: osAiSetFrequency — AI divider/frequency range"
[N19]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osAiSetNextBuffer.html "Nintendo API: osAiSetNextBuffer — AI buffer alignment and limits"
[N20]: https://n64.readthedocs.io/ "N64 first-hand emulator-development hardware notes; supplementary address/register reference"
[N21]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osAiGetStatus.html "Nintendo API: osAiGetStatus — AI DMA queue status"
[N22]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osEepromProbe.html "Nintendo API: osEepromProbe — 4 Kbit and 16 Kbit cartridge EEPROM"
[N23]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osPfsInitPak.html "Nintendo API: osPfsInitPak — Controller Pak initialization"
[N24]: https://ultra64.ca/files/documentation/online-manuals/man/n64man/os/osPfsAllocateFile.html "Nintendo API: osPfsAllocateFile — Controller Pak allocation units and file count"
[N25]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro26/26-02.html "Nintendo Programming Manual §26.2: Standard controller, acquisition timing, stick range"
[N26]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro04/04-01.html "Nintendo Programming Manual §4.1: Runtime and memory management"
[P01]: https://www.nintendo.co.jp/ "Nintendo Japanese historical DS/DS Lite specifications — search-indexed archival dimensions; original page redirects"
