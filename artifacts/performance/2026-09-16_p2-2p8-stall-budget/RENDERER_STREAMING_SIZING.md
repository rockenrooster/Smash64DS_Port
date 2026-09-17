# Renderer streaming is compulsory traffic — NO-GO, and it corrects this directory's own headline

The last unattacked bucket: **254,344 tk/fr = 39.5% of all data stall**, the
largest single owner of stall in the frame. Sized against the per-PC profile
before any build.

**Verdict: NO-GO. No implementable change reaches 10% of the 496,382 gap.**

## Two corrections to `README.md` in this directory

**1. "~816 KB of data per frame" was wrong by 2.2-2.7x. The real figure is
~300 KB/frame.**

816 KB requires 25,500 fills x 44 cycles = 1,122,000 cycles of fill time inside
a 1,616,422-cycle frame, but total data stall is **560,739**. The published
figure was exactly 2x the truth — a `/129` against `/258` slip. Measured
directly, from load stall 519,006 less LDM register width 20,894, icache landing
on load PCs ~29,300, interlock 18-45k and I/O reads 27,311:

| miss penalty | fills/fr | KB/fr |
|---|---:|---:|
| 40 cyc | 10,608 | 331 |
| **44 cyc (central)** | **9,644** | **301** |
| 48 cyc | 8,840 | 276 |
| 33.5 cyc (memcpy lower bound) | 12,292 | 384 |

**266-384 KB/frame, central 300.** The gate arithmetic and the 64.3% stall split
are unaffected; only the traffic reframe was inflated.

**2. "`.text.hot` still has 6,420 free bytes, so code layout is not
resource-blocked" is wrong.** `linker/nds_hot_text.ld:180-227` records it as
**closed in both directions** — two independent estimators previously got the
sign wrong there (Task 94 +6,144 P50; R2-03 E66 +24,448 P95).

Both of this session's cautions were confirmed rather than repeated: DTCM free
**1,992 B** (`__dtcm_bss_end` 0x02ff2838 against the 0x02ff3000 ceiling), ITCM
free **104 B** (`__itcm_end` 0x01ffff98).

## Decomposition — 227 symbols, largest is 10.2%

| stream | stall tk/fr | % | fills | KB fetched |
|---|---:|---:|---:|---:|
| geometry/vertex read and emit | 80,935 | 31.8 | 1,420 | 44.4 |
| matrix streams | 80,932 | 31.8 | 1,321 | 41.3 |
| other renderer | 32,967 | 13.0 | 641 | 20.0 |
| packet replay / readback | 25,780 | 10.1 | 524 | 16.4 |
| texture / material | 19,520 | 7.7 | 378 | 11.8 |
| packet build / write | 14,209 | 5.6 | 256 | 8.0 |
| **total** | **254,344** | | **4,540** | **141.9** |

The call counts refute the word "streaming": `ndsFighterMarioFoxDLAllDrawForSlot`
runs **2.0 calls/fr at 23,343 cycles each**, `CommitNativeStageSegment` 4.0 at
7,013, `HardwareWriteVertex16Words` **14.9 calls/fr**. Bulk geometry already
travels by DMA. This bucket is scattered **control-plane** data at 500-7,000
cycles per call.

## It is compulsory, not re-read — the number that decides the bucket

| evidence | value |
|---|---:|
| renderer line fetch | 141.9 KB/fr |
| useful bytes its loads consume | 202 KB/fr |
| **aggregate reuse** | **1.42x — fetched is LESS than used** |
| renderer-owned resident static data | 313 KB |
| load stall on already-resident data | **2.0%** |
| load stall on efficient sequential streaming | 13.9% |

A re-read-dominated bucket has fetched **much greater than** used. This one
fetches **less than half its own resident data** per frame. **Ordering, batching
and caching have nothing to recover.**

What is reducible is **over-fetch**, by miss-rate class:

| class | stall | % | fills | KB fetched | words used | line efficiency |
|---|---:|---:|---:|---:|---:|---:|
| resident | 4,044 | 2.0 | 92 | 2.9 | 21,891 | — |
| 2-5 cyc | 20,592 | 10.3 | 468 | 14.6 | 11,804 | — |
| sequential stream | 27,751 | 13.9 | 631 | 19.7 | 6,616 | ~100% |
| partial | 51,554 | 25.8 | 1,172 | 36.6 | 8,181 | 87% |
| scattered | 40,318 | 20.2 | 916 | 28.6 | 2,042 | 28% |
| **near-total miss** | **55,502** | **27.8** | 1,261 | 39.4 | 1,189 | **12%** |

The always-miss half is 95,820 tk/fr fetching **69.7 KB to deliver 12.6 KB —
18.1% line efficiency**. Perfect packing there is **78,000 tk/fr (15.7%)**, and
that is a theoretical bound across dozens of unrelated structures.

Archetype, the largest single such site
(`ndsFighterMarioFoxDLAllDrawForSlot` 0x02067670, 79.3 cyc/x x 27.4/fr):

```asm
lsls r2,r5,#1 ; adds r2,r2,r5 ; lsls r2,r2,#3 ; ldr r2,[r2,r5]
```

a **stride-24 gather reading only the first word** — 4 of 32 bytes used.

## The fighter packet path: three suspected costs, all absent

| suspicion | finding |
|---|---|
| CPU reads back packet memory it just wrote | **No.** Only a header RMW at `nds_renderer_preamble.c:3542`, 60.6 calls/fr, ≤61 words/fr |
| write-allocate pollutes the cache | **No.** ARM946E-S has no write-allocate, and `armDCacheFlush` costs **481.5 cyc/call over ~336 lines = 1.4 cyc/line**, which proves those lines are never resident |
| a staging copy could be removed | **None exists** — the recorder writes straight into the arena (`nds_renderer_native_common.c:9511`) |

The arena is a borrowed framebuffer at 0x02208990 and is **not 32-byte aligned**
(`base & 31 == 16`), inherited by all four slots through the 35,360-byte stride.
Fixing that is worth **<300 tk/fr**.

The real packet cost is writing patch words into cold lines:
`ndsFighterPacketStoreSplitModelview` moves 64 B in and 64 B out per call at
**439 cycles — 6.9 cycles per byte** — and its hot PCs are the *source* matrix
loads (43.7 and 33.0 cyc/x), not the destination.

## Avoidance candidates, priced

| candidate | worth | verdict |
|---|---:|---|
| uncached arena (MPU regions 2 and 3 are free) | ≤1,000 | 0.2% — NO-GO |
| DMA instead of CPU copy | ≤15,000 | 3.0% — NO-GO |
| build directly in the DMA-source buffer | 0 | already implemented |
| eliminate a staging copy | 0 | none exists |
| 32-byte arena alignment | <300 | 0.06% |
| stage CPU-to-GX-FIFO stores by DMA | ~10,500 | 2.1% |

## Bottom line against -496,382

| | tk/fr | % |
|---|---:|---:|
| whole bucket deleted (unreachable) | 254,344 | 51.2 |
| all renderer load stall deleted (unreachable) | 199,761 | 40.2 |
| **perfect layout on the always-miss half** | **78,000** | **15.7** |
| best single buildable change | 300-15,000 | 0.06-3.0 |

**Floor 300, ceiling 78,000 at theoretical perfection.** The 78,000 is spread
over 227 symbols with **no single site above 3,509 tk/fr**, and its mechanism —
packing scattered structures — is the one already blocked for `DObj`/`GObj` by
`decomp/` pristineness.

Renderer streaming can only be reduced by **drawing less**, or by re-laying-out
data so each fetched line carries more than 6 useful bytes. The first is a
fidelity decision; the second is diffuse, invasive and bounded at 15.7%.

## The falsifier this makes worth running

Every layout candidate so far has been bounded by asking what a *better*
arrangement would save. The cheaper and more decisive question is what the 4 KB
data cache is worth **right now**: run the same match with it disabled and
`frame_off - frame_on` is exactly that number.

It cannot be argued with in either direction. If the working set were reusable,
the delta is enormous and layout work has real headroom. If the traffic is
compulsory as measured, the delta tracks word count times main-RAM latency and
no rearrangement can find 496,382. It also validates or kills the 300 KB
correction above independently.

Implemented as `NDS_LAB_NO_DCACHE` (`Makefile:294`, `src/nds/main.c`), clearing
CP15 c1 bit 2 after a full clean. The function is `target("arm")` because CP15
transfers have no Thumb encoding and this build is `-mthumb`.
