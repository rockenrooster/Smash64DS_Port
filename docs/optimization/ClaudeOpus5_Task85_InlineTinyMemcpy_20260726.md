# Task 85 - Inline the 2- and 4-byte memcpy calls: SRC P95 -76,544

**Date:** 2026-07-26
**Status:** **KEEP, default on.** Boundary green
(`artifacts/task85-verify-boundary.log`, 0 failures).
**Inputs:** `artifacts/task85-A.json` / `-B.json`, one tree, 128 frames from 439.

## 1. Fixing the instrument Task 84 could not

Task 84 could attribute `memset` but not `memcpy`: 82% of `memcpy` bytes resolved
to addresses inside BSS data objects, which cannot be return addresses. The cause
was breaking on the *symbol* `memcpy`, where GDB lands past the prologue and
`lr` has already been reused as scratch by devkitARM's block copy.

Breaking at the **exact entry address** (`break *0x01fff4ac`, taken from `nm`)
fixes it completely. 170,560 samples, every one resolving to real code.

## 2. What the frame is actually doing

| calls | share | bytes | B/call | caller |
|---|---|---|---|---|
| 15,684 | 9.2% | 62,736 | 4 | `ndsRelocApplyWordByteSwap` |
| 15,018 | 8.8% | 30,036 | 2 | `ndsRelocReadNative16` |
| 15,018 | 8.8% | 30,036 | 2 | `ndsRelocWriteNative16` |
| 30,036 | 17.6% | 60,072 | 2 | `ndsRelocNormalizeFighterAObj16File` (two sites) |
| 12,006 | 7.0% | 48,024 | 4 | `ndsRelocReadNative32` |

**51% of every `memcpy` call in the frame moves 2 or 4 bytes** - 437 calls per
frame carrying 3% of the bytes and paying full call overhead for it.

The cause is a correct and normally free idiom:

```c
static u32 ndsRelocReadNative32(const void *addr)
{
    u32 value;
    memcpy(&value, addr, sizeof(value));   /* portable unaligned read */
    return value;
}
```

On most targets the compiler folds that into one load. It cannot here: the
ARM946E-S has no unaligned-access support, so GCC must emit a real call that
handles the misaligned case byte by byte. The idiom is right; the target makes it
expensive.

## 3. The change

Test alignment inline, fall back to `memcpy` when it fails:

```c
if ((((uintptr_t)addr) & 3u) == 0u)
{
    return *(const u32 *)addr;
}
memcpy(&value, addr, sizeof(value));
```

Two instructions on the fast path, and the fallback keeps the original behaviour
exactly - so this needs **no proof about where these pointers come from**. That
matters more than it looks: an unaligned `LDR` on this core does not fault, it
*rotates*, so a wrong alignment assumption would corrupt silently rather than
crash. Four accessors changed, all in `reloc_backend_assets.c`.

## 4. Result

| bucket | A | B | delta |
|---|---|---|---|
| **`SRC` P95** | 739,392 | **662,848** | **-76,544** |
| **`WORK-H` P95** | 1,800,896 | **1,760,512** | **-40,384** |
| `WORK` P95 | 1,855,808 | 1,829,568 | -26,240 |
| `WORK-H` P50 | 1,345,984 | 1,354,240 | +8,256 |

VBlank histogram improves on all four measures: 3-interval **487 -> 490**,
4-interval **74 -> 72**, 5+ **6 -> 5**, max **19 -> 18**. Gap to the 1,120,000
target: **680,896 -> 640,512**.

## 5. Reading the P50 rise honestly

`WORK-H` P50 rose 8,256 and only 22 of 128 frames improved. That is not a
contradiction, it is the expected shape: these accessors run during
**relocation**, which happens on the ~26 animation-load frames, not on median
ones. On the other ~102 frames the change does nothing except move code, so what
shows there is placement - and +8,256 sits exactly at the ±8,000 placement floor
Task 79 established.

The corroboration is that the buckets which rose (`STG` +5,696, `FTR` +6,272) are
ones relocation cannot touch, while `SRC` - where reloc work actually lands -
fell 76,544, which is 9.6x the floor. And the VBlank histogram, which is
independent of bucket attribution entirely, improved on every axis.

This is a burst fix. `PROJECT_GOAL.md` gates on a burst statistic.

## 6. What remains

`memcpy` is still 849 calls/frame. Removing 437 of them leaves the 64-byte matrix
copies - `ndsRendererAdapterBuildDObjLocalMatrix` (10,098 calls),
`DObjWorldIndexHash` (9,838), `BuildPersistentStageWorldMatrix` (6,389 + 5,352),
`PrepareNativeOwnerMatrices` (6,227), `BuildDObjWorldMatrix` (5,975) - roughly
44,000 sampled calls copying whole 4x4 s20.12 matrices. Those are real work
rather than idiom overhead, but at 64 bytes each they are candidates for
by-reference passing rather than copying, and nothing has looked at whether the
copies are needed.
