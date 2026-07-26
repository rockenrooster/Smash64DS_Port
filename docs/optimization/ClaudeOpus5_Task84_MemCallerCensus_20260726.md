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
