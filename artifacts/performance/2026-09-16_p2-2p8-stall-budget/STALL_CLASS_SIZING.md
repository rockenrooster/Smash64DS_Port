# The stall class, sized: right in kind, too small in size

Follow-up to `README.md` in this directory, which established that the non-idle
frame is **1,616,422 tk/fr = 576,491 issue + 1,039,931 stall (64.3%)** and that
the gate therefore asks for a 47.7% cut in stall rather than a 30.7% cut in
work. This document sizes the candidates that class implies.

**Verdict: NO-GO at the 10% bar. The best candidate is 1.6-3.0% of the gap.**

## The stall is data, about 3.6:1 over instruction fetch

melonDS books an icache line fill against the instruction executing when the
fetch unit crosses a 32-byte boundary, which shows up as a stall spike at a
fixed offset: **Thumb at `pc & 31 == 28`, ARM at `pc & 31 == 24`** — two
instruction widths, two offsets, one mechanism, so the discriminator is not a
fitted artefact.

| component | tk/fr | share | method |
|---|---:|---:|---|
| **data stall** | **560,739** | **53.9%** | memory-op PC stall 644,328 − LDM/STM register width 41,127 − fetch landing on memory PCs 42,462 |
| instruction fetch (icache) | 155,651 | 15.0% | sequential 107,886 + branch-target 47,765 |
| pipeline intrinsic (refill, interlock, multiply) | 323,548 | 31.1% | residual |

Cross-checks: an ALU-as-probe method (ALU and BL-prefix instructions have no
data access; MAIN−ITCM deltas 0.442 and 0.421 agree) gives icache 151,833,
within 2.5%. Branch-target fills were measured by decoding targets and splitting
same-line from other-line with **ITCM as a zero-icache control** (MAIN +1.126
cyc/branch against ITCM −0.181). Uncertainty: data ±8%, icache ±30%.

Model-free anchor: **ITCM-resident code carries 232,704 tk/fr of stall with
instruction fetch impossible by construction, and 69.4% of it sits on memory-op
PCs.**

**Consequence: code layout is worth at most 155,651 and DTCM/data layout is the
right class.** The claim published here that `.text.hot`/`.text.hot.draw` have
6,420 free bytes and are "not resource-blocked" is **WRONG**:
`linker/nds_hot_text.ld:180-227` records that section as closed in BOTH
directions, with two independent estimators having got the sign wrong (Task 94
+6,144 P50; R2-03 E66 +24,448 P95).

## Two premises of my own, refuted

**1. `FTParts` does not bury its hot fields behind matrices.** DWARF from the
linked ELF, matching `include/ft/fighter.h:461-482`:

| offset | field | line |
|---:|---|---:|
| 0 | `transform_update_mode` | 0 |
| **4** | `unk_dobjtrans_word` (the union) | 0 |
| 8 | `next` | 0 |
| 12-15 | `flags`, `joint_id`, `is_have_anim`, `unk_dobjtrans_0xF` | 0 |
| 16 | `unk_dobjtrans_0x10` (Mtx44f) | 0-2 |
| 80 | `mtx_translate` | 2-4 |
| 156 | `unk_dobjtrans_0x9C` | 4-6 |

`sizeof` is **224 = 7 x 32**, so instances are line-aligned and **every hot field
is in line 0**. The matrices are after, not before. The invalidate replay
therefore touches **exactly one line per part** already, and no reordering can
improve a one-line touch.

**2. The invalidate walk does not walk `FTParts`.** It walks **`DObj`**
(`decomp/BattleShip-main/decomp/src/sys/objtypes.h:414`, `sib_next` :418,
`child` :420) and reaches parts through `user_data.p`. The three expensive
loads are the DObj chase, not the part clear:

| path | instruction | exec/fr | cyc/exec | stall tk/fr |
|---|---|---:|---:|---:|
| DFS rebuild, `DObj->child` | `ldr r3,[r6,#16]` | 184.4 | **42.0** | 3,784 |
| DFS rebuild, `DObj->sib_next` | `ldr r6,[r6,#8]` | 184.4 | 30.7 | 2,739 |
| DFS rebuild, `DObj->user_data.p` | `ldr r2,[r6,#132]` | 150.0 | 25.9 | 1,870 |
| flat replay, reset variant reads `FTParts+0` | `ldr r0,[r3,#0]` | 102.6 | **48.6** | 2,441 |
| flat replay **store** to `FTParts+4` | `str r5,[r1,#4]` | 290.1 | **4.4** | 487 |

The "one word per struct" intuition is confirmed for the store and it is already
nearly free — 4.4 cyc/exec, because the store retires into the write buffer
instead of allocating a line.

## Line efficiency, and why the suspects are too small

| structure | sizeof | lines/visit | visits/fr | bytes fetched/fr | useful | efficiency |
|---|---:|---:|---:|---:|---:|---:|
| `DObj` (flatten DFS) | 136 | 2 | 184.4 | 11,802 | 2,213 | **18.8%** |
| `GObj` (`ndsBaseGcRunAll`) | 136 | 2 | 122.4 | 7,834 | 1,591 | **20.3%** |
| `FTParts` (flat replay) | 224 | **1** | 392.7 | 12,566 | 3,142 | 25% |

Those three walks total **23,063 bytes fetched per frame**. Whole-frame data
traffic, from data stall divided by the measured fill penalty (~44 cycles per
32-byte line), is **~9,644 line fills = ~300 KB per frame** (published here
first as ~25,500 fills / ~816 KB, which was wrong by 2.2-2.7x -- a `/129`
against `/258` slip, corrected in `RENDERER_STREAMING_SIZING.md`). The suspected
structures are **2.8% of it**.

Whole-frame data stall by owner:

| bucket | tk/fr | % |
|---|---:|---:|
| renderer streaming (vertex / matrix / packet) | 254,344 | **39.5** |
| everything else, diffuse | 158,777 | 24.6 |
| object-graph walk (GObj/DObj) | 84,304 | 13.1 |
| pose / animation data | 53,931 | 8.4 |
| bulk copy and clear | 39,887 | 6.2 |
| I/O register, FIFO, DMA wait | 27,311 | 4.2 |
| collision geometry | 25,774 | 4.0 |

## DTCM: the break-even nobody can clear

DTCM is 16,000 bytes (`C:/devkitPro/calico/lib/ds9.ld:16`, already net of stacks
and reserved BIOS memory). Current occupants:

| symbol | bytes |
|---|---:|
| `sNdsNativeFighterPreparedDense` | 6,130 |
| `sNdsNativeFighterDenseNormals` | 2,452 |
| `sNdsShieldPoseDObjScratch` | 1,408 |
| irq table, frame summary, scheduler, controller playback | 318 |
| **used / free** | **10,308 / 1,992** |

**Correction to the row above:** 5,704 was taken from `dtcm LENGTH = 0x3e80` in
calico's `ds9.ld`, which is the region and not the budget.
`linker/nds_hot_text.ld:171` asserts `__dtcm_bss_end <= 0x02ff3000` against the
boot stack's measured low-water mark, so the ceiling is 12,288 bytes and **1,992
are free**. Every DTCM figure below that assumes 5,704 is correspondingly
optimistic, and the recommended 8-slot candidate at 3,168 bytes never fit. See
`…_p2-2p8-dtcm-falsifier/`.

83% of it is already two dense fighter geometry tables.

**5,704 bytes is 178 cache lines. DTCM saves one fill per resident line per
refetch avoided, so reaching 10% of the gap (49,638 tk/fr = 2,256 fills at 44
cycles) needs every one of those 178 lines refetched 12.7 times per frame.** A
structure walked once per frame yields K≈1 and returns about **3,400 tk/fr for
the entire 5,704 bytes — 0.7% of the gap.**

| candidate | DTCM bytes | movable | tk/fr | per byte |
|---|---:|---|---:|---:|
| `sNdsFtPartsFlat` at 8 slots | 3,168 | **yes**, file-scope static | 8,000-15,040 | 2.5-4.7 |
| `sNdsFtPartsFlat` as shipped (4 slots) | 1,584 | yes | ~2,000 | 1.3 |
| `NdsFtPose[4]` header | 320 | yes | ~1,500 | 4.7 |
| `glGlobalData` | 76 | yes | ~1,000 | **13** |
| `GObj`/`DObj` node set (~122 live) | 16,592 | **no** — taskman heap, 2.9x free DTCM | — | — |
| `FTStruct` x4 | 12,048 | **no** — 2.1x free DTCM | — | — |
| `FTParts` x~120 | 26,880 | **no** — parts pool, 4.7x free DTCM | — | — |

Anything from `syTaskmanMalloc` cannot be relocated by an attribute: it needs a
separate DTCM arena and every producer and consumer repointed, and the
heap-generation invalidation logic depends on taskman identity.

## Field packing: no-go on `FTParts`, blocked on `DObj`

`FTParts` is already one line for the hot walk, so a reorder buys zero. It would
also be far more invasive than the board recorded: the real count is **125 field
accesses over 108 lines** in the four decomp files (`gm/gmcollision.c` 64 lines
/ 81 accesses, `ft/ftparam.c` 25, `ft/ftmanager.c` 17, `ft/ftcomputer.c` 2) plus
about 200 port-side sites, and `include/ft/fighter.h:461-482` is field-for-field
identical to `decomp/.../ft/fttypes.h:652-681`. Nothing is mechanically pinned —
no `offsetof`, no static assert, no assembly, no asset blob — but the field
*names* encode the N64 offsets, so a reorder makes every name a lie.

The structure that would pay is **`DObj`**: moving `user_data` (offset 132, line
4) beside `sib_next`/`child` collapses 2 lines to 1 on 184.4 visits a frame, and
the same move on `GObj` (`flags` at 124) on 122.4 visits. Ceiling at perfect
efficiency is about **40,000 tk/fr, 8.1%**. It is blocked: `DObj`/`GObj` are
declared in `decomp/.../sys/objtypes.h:414` with **no `include/sys/` shadow**, so
the port consumes the decomp header directly, and `decomp/` is byte-pristine
under `check-decomp-pristine.ps1` on every `verify-all.ps1` profile.

## Bottom line

| candidate | floor | ceiling | % of 496,382 |
|---|---:|---:|---:|
| `sNdsFtPartsFlat` to DTCM at 8 slots | 8,000 | 15,040 | **1.6-3.0%** |
| all of DTCM spent optimally at K≈1 | — | 3,400 | 0.7% |
| `DObj`+`GObj` repack (blocked by pristine decomp) | — | 40,000 | 8.1% |
| entire object-graph bucket deleted (unreachable) | — | 84,304 | 17.0% |
| entire icache stall deleted (unreachable) | — | 155,651 | 31.4% |

**Nothing in this class reaches 10% of the gap.**

The one candidate worth its build is `sNdsFtPartsFlat` in DTCM, because it is
the only lane whose failure mode is **cured** rather than repeated: N05.03 died
when a 16-slot table at 3,264 bytes took 80% of the 4 KB data cache and evicted
the stage (SRC −15,040, STG +51,520, net +33,984). DTCM is not a cache, so that
mechanism is structurally impossible there.

**Falsifier, run before any widening: move the table into DTCM at its SHIPPED
four slots and read STG alone.** Nothing else changes, so if STG does not
improve, this table was never the evictor, widening cannot pay, and the lane
dies for one build instead of four.

## The reframe that matters more than the candidate

The frame moves **~300 KB of data through a 4 KB data cache every frame** (not
the ~816 KB first published here), and
**39.5% of data stall is renderer streaming** — vertex, matrix and packet
traffic that no 5,704-byte buffer touches. This is a working-set **volume**
problem, not a working-set **placement** problem, and placement is all DTCM can
change.

## Instrument cost, for whoever quotes these figures next

The measurement apparatus is **41,894 tk/fr, 2.6% of the non-idle frame**:
`ndsPlatformRenderDebugHud` 20,077, `tickGetCount` 16,583, `cpuGetTiming` 5,234.
`tickGetCount`'s 13,793 tk/fr of stall is hardware-register latency and no cache
work removes it.
