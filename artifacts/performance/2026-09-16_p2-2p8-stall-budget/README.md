# The gap is not work. It is 64% memory stall, and the gate is reachable only by attacking that.

Every lane this campaign has killed died the same way. That stopped being a
coincidence and became a measurement.

## The whole-frame split

From `artifacts/performance/2026-09-16_p2-2p8-n0409-profile/arm9-profile.meta.txt`
(`instructions=148735753`, `cycles=487368912`, `regions=129`; ticks = cycles/258):

| | tk/fr |
|---|---:|
| instruction issue, at 1 cycle each | **576,495** |
| memory stall | 1,312,531 |
| total | 1,889,026 |
| whole-frame CPI | **3.28** |

`armWaitForIrq` is 272,600 of that stall at CPI 55,908 — it is the idle VBlank
wait, not a stall, and must come out. Doing so reproduces the board's
independently derived non-idle figure exactly:

| non-idle frame | tk/fr | share |
|---|---:|---:|
| **instruction issue** | **576,491** | **35.7%** |
| **memory stall** | **1,039,931** | **64.3%** |
| total | 1,616,422 | (board: 1,616,382) |

## What that means for the gate

The gate is **1,120,000**.

**The instruction issue floor alone is 576,491 — 543,509 UNDER the gate.**

The ARM9 is not executing too many instructions. It is waiting for memory. So
the requirement is not "delete 30.7% of the work":

> **the gate requires cutting memory stall by 47.7%**, from 1,039,931 to 543,509.

## Why every lane failed

This explains the campaign's whole record, and it is why the failures kept
looking like bad luck:

| lane | what it deleted | what it paid | net |
|---|---|---|---|
| collision ring (`NDS_R2_COLLISION_FIXED`) | issue -1,717 | icache fill +1,854 | **+64** |
| GX compose bank at four fighters | CPU multiply -18,165 | FIFO traffic | **+22,848 P50** |
| invalidation cache, 16 slots (N05.03) | SRC -15,040 | dcache eviction, STG +51,520 | **+33,984** |
| SRC bound-domain candidate (sized, not built) | <= 57,034 issue | +3,900..+7,800 icache on 8-16 KB of tables | **no-go** |

Each one traded *issue* for *fetch*. Issue was never the constraint. Four
independent attempts, one mechanism.

## Where the stall is

Ranked by stall, idle excluded, the top 21 symbols hold **299,927 tk/fr = 28.8%
of all real stall**. It is diffuse, not concentrated — no single function is the
answer:

| symbol | stall tk/fr | issue tk/fr | CPI |
|---|---:|---:|---:|
| `ndsFighterMarioFoxDLAllDrawForSlot` | 37,799 | 7,892 | 5.79 |
| `ndsFighterPacketTryReplay` | 21,860 | 2,509 | **9.71** |
| `memset` | 20,299 | 5,643 | 4.60 |
| `ndsFTParamsInvalidateSubtree` | 16,712 | 3,636 | 5.60 |
| `ndsRendererCommitNativeStageSegment` | 15,211 | 12,623 | 2.20 |
| `ndsFtPoseParse` | 15,083 | 6,369 | 3.37 |
| `memcpy` | 15,017 | 9,637 | 2.56 |
| `tickGetCount` | 13,793 | 2,790 | **5.94** |
| `ndsFtPoseUpdate` | 13,190 | 3,998 | 4.30 |
| `battleship_ftMainProcUpdateInterrupt` | 11,203 | 1,259 | **9.89** |
| `ndsFighterDisplayContractSubmit` | 10,827 | 859 | **13.59** |
| `ndsBaseGcRunAll` | 9,871 | 1,231 | **9.02** |

The highest-CPI entries are the dispatchers and submitters — functions with
little arithmetic that touch a lot of scattered state. That is the signature of
a **working-set** problem, not a hot-loop problem.

## The resource that is actually scarce

| TCM | size | used | free |
|---|---:|---:|---:|
| **ITCM** (code) | 32,736 | 32,632 | **104** |
| **DTCM** (data, zero wait state) | 16,000 | 10,296 | **5,704** |

Sizes from `calico/lib/ds9.ld` (`itcm LENGTH = 0x7fe0`, `dtcm LENGTH = 0x3e80`,
already net of stacks and reserved BIOS memory); usage from the linked ELF
(`.itcm` 0x7f78; `.dtcm` 0x2220 + `.dtcm.bss` 0x618).

**The campaign has spent years of effort competing for ITCM, which is 99.7%
full, while the frame is 64% DATA stall and DTCM has 5,704 bytes free.**

The ARM9 data cache is only **4 KB**, which is why the N05.03 table could not be
made big enough to hold its own working set without evicting the stage: a
3,264-byte table is 80% of the entire data cache. DTCM is not a cache — it is
addressable zero-wait-state memory, and nothing placed there can be evicted by
anything else.

## The direction this implies

Not "find another function to delete". The remedies for stall are different in
kind from the remedies for work:

1. **Put the hottest small, frequently-touched structures in DTCM.** 5,704 bytes
   is not much, but the 4 KB dcache is the thing being thrashed, so removing a
   few hot kilobytes of scattered state from cache contention helps everything
   else that remains cached.
2. **Shrink working sets rather than instruction counts.** `FTParts` is large
   (three `Mtx44f` before its useful fields), and the invalidate walk touches
   **one word per multi-line struct** — the classic case where packing the hot
   fields together converts many line fills into one.
3. **Order traversals by address**, so a walk that must touch N structures
   touches them in a line-friendly sequence.

None of these is sized yet. This document establishes only that they are the
right *class* of candidate, and that the previous class cannot work: **the
issue-side budget is already 543,509 ticks under the gate, so no amount of
arithmetic deletion can close a gap that is entirely stall.**

## Caveat, stated plainly

"Issue at 1 cycle per instruction" is a floor, not an achievable target. Thumb
on ARM946E-S retires most ALU instructions in one cycle, but branches,
multiplies and unavoidable first-touch misses are not compressible to it. The
576,491 figure is therefore a bound that proves the gap is *not* issue-bound; it
is not a claim that a 576,491-tick frame is reachable. What is load-bearing here
is the ratio — **64.3% of non-idle frame time is the CPU waiting for memory** —
and the direct consequence that a 47.7% stall reduction, not a 30.7% work
reduction, is what the gate actually asks for.
