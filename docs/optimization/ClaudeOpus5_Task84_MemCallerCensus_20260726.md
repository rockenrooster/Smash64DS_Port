# Task 84 — `memset` is 40% one caller clearing 1,292 bytes; the `memcpy` method failed

**Date:** 2026-07-26
**Status:** E0 complete. **GO on `memset`, method failed on `memcpy`.** No
runtime change.
**Input:** 210,234 `$lr`/size samples taken at `memset` and `memcpy` entry from
frame 439 onward, resolved through `addr2line` against the shipped ELF.

## 1. Why sample rather than profile

Task 83 ranked `mem*` as the next target: `memset` (57,206 ticks/frame) and
`memcpy` (59,230) are the two largest stall rows in the frame, and by instruction
count they move 121–242 KiB and 208–416 KiB per frame respectively — with nothing
established about what calls them.

The per-PC profiler cannot answer this. It attributes cycles to the leaf, and
`memset` is a leaf. So this breaks at the call boundary and records the return
address and the size argument. Breakpoint overhead does not matter here: the
question is *what and how much*, not *how long*.

## 2. `memset`, attributed

64,361 calls, 6,519,190 bytes sampled.

| share | bytes | calls | B/call | caller |
|---|---|---|---|---|
| **39.8%** | **2,594,336** | **2,008** | **1,292** | **`ndsRendererInitStats`** |
| 9.3% | 604,544 | 9,446 | 64 | `ndsRendererMtxLoadN64ToDS20p12` |
| 8.7% | 567,200 | 5,672 | 100 | `ndsRendererAdapterBuildNativeMaterialSnapshot` |
| 8.2% | 532,600 | 10,652 | 50 | `ndsRendererAdapterBuildNativeProductionInputs` |
| 7.7% | 499,296 | 8,916 | 56 | `ndsRendererMtxIdentity20p12` |
| 7.1% | 465,480 | 4,310 | 108 | `ndsRendererHardwareResolveOrBindTexture` |

**One caller is 40% of all `memset` traffic**, and its per-call size is 1,292
bytes — *exactly* `sizeof(NDSRendererStats)`. That exact match is what makes the
attribution trustworthy rather than merely plausible: the sampler had no
knowledge of the struct and reproduced its size to the byte.

The rest of the table self-validates the same way. 64 B is a 4×4 s20.12 matrix;
56 B is the identity helper's write; these are the sizes those functions must be
clearing.

`ndsRendererInitStats` does:

```c
memset(stats, 0, sizeof(*stats));   /* 1,292 bytes */
stats->geometry_mode = NDS_RENDERER_GEOM_RESET_MODE;
stats->othermode_h = NDS_RENDERER_TP_PERSP | NDS_RENDERER_TF_BILERP;
stats->texture_source_hash1 = 2166136261u;
stats->texture_source_hash2 = 0x9e3779b9u;
```

Four fields set after a full-struct clear. Whether the other ~1,270 bytes are
read before being written is the E1 question, and it is the whole lever.

## 3. `memcpy` — the method failed, and the numbers must not be used

The same sampler attributed **81.9% of `memcpy` bytes to addresses inside BSS
data objects** — `sNdsRendererAdapterNativeStageWorkspace` and
`sNdsRendererAdapterDObjWorldCache`. Those are not code. A return address cannot
point into a workspace array, so `$lr` was not holding a return address for those
samples.

The likely cause is that devkitARM's `memcpy` is a heavily optimised block copy
that uses `lr` as a scratch register, and GDB's breakpoint does not land on the
first instruction. `memset` is simpler and evidently keeps `lr` intact long
enough.

I checked this rather than reporting the ranking, because a caller table that
names data structures as callers is wrong in a way that looks plausible in a
summary — "82% of memcpy comes from the stage workspace" is a sentence that would
have survived review and been false.

**`memcpy` attribution is unresolved.** Getting it needs a different instrument:
a breakpoint on the exact first instruction address rather than the symbol, or a
census wrapper compiled in. Only one `memcpy` row survives — `ndsRelocAssetLoadHeaderAndData`
at 14.5%, 63 B/call, which is the `fread` path from Task 76 and resolves to real
code.

## 4. What E1 should do

**`ndsRendererInitStats` is the lever.** Two independent questions, in order:

1. **How much of the 1,292 bytes needs zeroing?** If most fields are written
   before being read on every path, the clear can shrink to the fields that
   genuinely need it. This is a correctness question about a 1,292-byte struct
   with many members, so it wants the compiler's help rather than reading: mark
   the struct's fields and let a debug build assert first-read-before-write, or
   clear only a documented prefix and gate on the full verifier.
2. **How often is it called?** 2,008 calls in the sample. The sample spans an
   unknown number of frames — the breakpoints slow the emulator by orders of
   magnitude, so wall-clock does not convert — but the *fraction* is solid and
   the call count per frame is a one-counter question that E1 should answer
   before changing anything, per Tasks 79–81.

If it turns out to be ~50 calls a frame at 1,292 bytes, that is ~65 KiB/frame of
clearing, and roughly 40% of `memset`'s 57,206 ticks — about 22,000 ticks, or
2.7× the placement noise floor. Worth taking, but not the 137,000 the family
headline suggests, and E1 should be scoped against the smaller number.

## 5. Method note

The sampler validates itself two ways and both are worth reusing: the recorded
size must match a known struct size for the attribution to be believed, and the
recorded address must land inside an *executable* section. My first range check
tested only "is this in main RAM", which the BSS addresses passed — main RAM
holds both `.main` code and `.bss`. Checking the section, not the range, is what
caught it.

---

# Task 84 E1 - The clear is priced: up to 41,468 ticks/frame

**Date:** 2026-07-26
**Status:** E1 complete. **The lever is real and it is the largest open one.**
Probe removed, source restored. No runtime change.
**Inputs:** `artifacts/task84-costA.json` / `-costB.json`.

## E1.1 The denominator, counted

`ndsRendererInitStats` runs **1,496 times over 128 frames - 11.7 per frame**,
clearing 15,100 bytes/frame. Not the ~50 calls/frame E0 guessed at; that
estimate was 4x too high.

It also dates the Task 84 sample at 2,008 / 11.7 = **172 frames**, which converts
the whole sample to per-frame rates for the first time:

| | calls/frame | bytes/frame | avg B/call | ticks/frame | ticks/call | ticks/byte |
|---|---|---|---|---|---|---|
| `memset` | 375 | 37,945 | 101 | 57,206 | 152.7 | 1.51 |
| `memcpy` | 849 | 27,434 | 32 | 59,230 | 69.8 | 2.16 |

**This corrects Task 83's sizing badly.** It estimated 121-242 KiB/frame for
`memset` and 208-416 KiB/frame for `memcpy` from instruction counts. The true
figures are **37.9 KiB** and **27.4 KiB** - off by 3-15x. That conversion assumed
4-8 bytes moved per executed instruction; the real averages are 1.2 and 0.5,
because both are dominated by per-call prologue and dispatch on small sizes
rather than by their block loops.

## E1.2 Pricing the clear without touching it

Call count and byte count still do not price a clear: `memset` costs a per-call
part and a per-byte part, and one equation cannot separate them. The aggregate
bound was **1,787 to 22,765 ticks/frame** - a factor of 13, spanning "below the
noise floor" to "worth taking".

So rather than shrink the real clear and risk reading an uninitialised member of
a 151-member struct to find out, the probe **duplicated** it onto a scratch
buffer. The A/B delta is then exactly the cost of one 1,292-byte clear at the
real call frequency, on the real path, with the ROM's behaviour bit-identical
either way.

| | A | B (duplicate clear) | delta |
|---|---|---|---|
| `WORK-H` P50 | 1,345,984 | 1,391,040 | **+45,056** |
| `WORK-H` P95 | 1,800,896 | 1,842,368 | +41,472 |
| `WORK-H` mean | 1,451,707 | 1,493,174 | **+41,468** |
| `STG` P50 | 372,544 | 404,160 | +31,616 |
| VBlank 3-interval | 487 | 459 | -28 |

**One 1,292-byte clear at 11.7 calls/frame costs 41,468 ticks/frame** - 3,544
ticks per call, **2.74 ticks per byte**.

## E1.3 Why that is nearly double the average rate

`memset` averages 1.51 ticks/byte across the frame; this clear runs at 2.74.
Large clears are the *expensive* ones per byte, not the efficient ones. A
1,292-byte buffer spans ~40 cache lines that are cold, so each costs a miss plus
a write-allocate, while the 101-byte average `memset` usually writes lines that
are already resident.

That inverts the assumption behind E0's upper bound, which priced these bytes at
the frame average and so *under*-estimated by 1.8x. **39.8% of `memset` bytes
account for roughly 72% of `memset` time.**

## E1.4 The honest ceiling

41,468 is an **upper bound on what removing the clear would save**, not a
prediction, for one reason: the clear also warms the cache for the writes that
immediately follow it. Delete it and some of those misses move to the first real
write of each field rather than disappearing.

How much moves rather than vanishes cannot be inferred from this measurement.
What it does establish is that the cost is **not** below the noise floor - the
question E0 could not answer - and that this is the largest single open lever,
ahead of anything in Task 83's family ranking.

## E1.5 What E2 must do

The struct is 1,292 bytes: `texture_tiles` (640), `texture_loads` (56), and 151
scalar members (596). Three routes, in increasing order of risk:

1. **Do not clear `texture_tiles` (640 B, half the struct)** if every tile is
   written by a `SETTILE` before it is read. That is a checkable claim rather
   than a judgement call, and it is half the cost.
2. **Reuse one persistent stats buffer**, resetting only the fields whose
   staleness is observable, instead of clearing per traversal.
3. **Reduce the 11.7 calls/frame.** `STG` absorbed 31,616 of the 41,468, so most
   are stage traversals - and Task 81 found the stage does zero texture binds
   during battle, which makes a full 1,292-byte stats clear per stage traversal
   look especially unearned.

Route 1 first: largest, checkable, independently revertible.
