# Task 86 - Inline the 64-byte matrix copies: WORK-H P95 -18,432

**Date:** 2026-07-26
**Status:** **KEEP, default on.** Boundary green
(`artifacts/task86-verify-boundary.log`, 0 failures).
**Inputs:** `artifacts/task86-A.json` / `-B.json`, one tree, 128 frames from 439.

## 1. Where the remaining memcpy calls are

Task 85 removed the 2- and 4-byte calls. The next group is 64-byte whole-matrix
copies, but Task 85's `$lr` sampler attributes at function granularity and
inlining smeared them across neighbouring symbols - it named
`ndsRendererAdapterDObjWorldIndexHash`, which on inspection contains no `memcpy`
at all, only a four-line pointer hash.

`objdump -d` settles it exactly, with no sampling and no inference:

```
10 ndsRelocFinalizeLoadedFile
 9 ndsRendererAdapterPrepareInitialMatrices
 8 ndsFighterMarioFoxDLAllDrawForSlot
 5 ndsRendererHardwareResolveOrBindTexture
 4 ndsRendererAdapterGetFrameCameraMatrices
 3 ndsRendererAdapterBuildDObjWorldMatrix
```

Those are `bl memcpy` sites the compiler emitted, not calls anyone wrote. The
source says:

```c
*projection = camera_projection;
*modelview  = entry->modelview;
*out        = *cached;
```

`NDSRendererMatrix20p12` is `s32 m[4][4]` - 64 bytes. On ARMv5 GCC cannot assume
the pointers are aligned and will not open-code 16 words blind, so a plain struct
assignment becomes a library call.

## 2. The change

`ndsRendererMatrixCopy20p12` in `include/nds/nds_renderer.h`: sixteen explicit
element assignments. Nine call sites replaced across
`reloc_backend_renderer_dl.c`.

Straight-line rather than a loop **deliberately** -
`-ftree-loop-distribute-patterns` rewrites a word-copy loop straight back into
the `memcpy` this exists to avoid. Indexing `m` rather than casting to `u32 *`
keeps it free of aliasing games, and the compiler pairs the accesses into
LDM/STM itself.

Verified before spending any emulator time: `bl memcpy` sites in the three
targeted functions fall **16 -> 9**, total 212 -> 205. A change that had not
actually removed the calls would have measured as noise and been indistinguishable
from one that had.

## 3. Result

| bucket | A | B | delta |
|---|---|---|---|
| **`WORK-H` P95** | 1,760,512 | **1,742,080** | **-18,432** |
| **`WORK-H` P50** | 1,354,240 | **1,340,032** | **-14,208** |
| `WORK` P95 | 1,829,568 | 1,765,248 | -64,320 |
| `STG` P50 | 378,240 | 368,896 | -9,344 |
| `STG` P95 | 385,088 | 376,512 | -8,576 |
| `FTR` P50 | 569,536 | 562,880 | -6,656 |

`WORK-H` improves on **118 of 128 frames**, mean -16,206. VBlank histogram:
3-interval **490 -> 499**, 4-interval **72 -> 63**. `WAIT` rises 17,280, which is
the frame finishing earlier and idling longer. Gap to the 1,120,000 target:
**640,512 -> 622,080**.

Unlike Task 85 this moves the **P50** as well, and for a clear reason: matrices
are built every frame for every DObj, so this is steady-state work rather than a
burst path. Both fixes were the same shape - a compiler-emitted `memcpy` where
an inline sequence belonged - but they land in different halves of the
distribution.

## 4. What is left

Total `memcpy` call sites are 205. The largest remaining group is
`ndsRelocFinalizeLoadedFile` (10 sites), which runs during relocation - the same
burst path Task 85 improved - and `ndsFighterMarioFoxDLAllDrawForSlot` (8).
Neither has been looked at, and the objdump method above makes finding them
exact rather than statistical.

The `memcpy`-emitted-by-the-compiler pattern has now paid twice. It is worth
checking as a matter of course on this target: a struct assignment that looks
free in C is a library call at 64 bytes, and nothing in the source hints at it.
