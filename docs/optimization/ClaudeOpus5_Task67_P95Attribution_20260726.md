# Task 67 — What the bad frames are: the renderer falls out of its fast path

**Date:** 2026-07-26
**Status:** Census. No code changed.
**Inputs:** `artifacts/task67-src-series.json` (256-frame per-frame series),
`artifacts/task67-spike-census/` (per-PC census windowed on frames 544–545),
compared against `artifacts/task65-census/` (mean over frames 438–566).

Task 66 found the milestone gate is a **P95**, that `WORK-H` P95 is 1,985,024
against a 1,120,000 budget, and that the P50→P95 gap is ~600K concentrated in
`SRC` and `FTR`. This task asks what actually happens on those frames.

## 1. The bad frames are not random

From the per-frame series over the settled window (frames 438–565):

| bucket | median | frames >1.5× median | pattern |
|---|---|---|---|
| `AUD` | 2,496 | 9 of 128 | **strictly periodic**, gaps 11–15 frames |
| `FTR` | 576,768 | 12 of 128 | **runs of ~5**: 438–439, 478–482, 544–548 |
| `SRC` | 317,216 | 25 of 128 | irregular, partly in runs |
| `MISC` | 47,104 | 55 of 128 | tracks the `FTR` runs |

`AUD` is the ADPCM BGM refill — `src/nds/nds_audio_bgm.c` streams from NitroFS
with `fread`, so every ~13 presented frames one `swiWaitForVBlank`-adjacent
frame pays a blocking cart read. Cost ~65,000 when it fires against a 2,496
median. Known, scheduled, bounded, and **not** the P95 driver.

The `FTR`/`MISC`/`SRC` runs are. The worst frames:

| frame | `WORK-H` | `FTR` | `SRC` | `MISC` |
|---|---|---|---|---|
| 544 | 2,761,088 | 1,027,520 | 1,129,088 | 195,264 |
| 478 | 2,672,064 | 1,013,248 | 1,048,192 | 194,176 |
| 449 | 2,385,728 | 576,576 | 1,288,128 | 46,464 |
| *median frame* | ~1,364,672 | 576,768 | 317,216 | 47,104 |

## 2. Aiming the per-PC profiler at frame 544

Window `NDS_TASK37_PROFILE_START=544`, `FRAMES=2`:

| | spike (544–545) | mean (438–566) | delta |
|---|---|---|---|
| wall | 2,240,694 | 1,851,253 | +389,441 |
| idle wait | 318,794 | 323,976 | −5,182 |
| **real work** | **1,921,900** | **1,527,277** | **+394,623** |

Wall 2,240,694 is exactly four VBlank periods, matching the `ALL` P95 of
2,240,512 the tick HUD reported independently.

**Subtract the instrument first.** Frame 544 or 545 coincided with a tick-HUD
console redraw: `ndsPlatformTickHudSort` +38,849, `_svfprintf_r` +11,349,
`__ssvfiscanf_r` +9,516, `_vfiprintf_r` +8,741, `div64` +14,627,
`__udivmoddi4` +8,188, `consolePrintChar` +7,260, `ndsTaskmanArenaBytes`
+7,408 — about **105,000 ticks the published profile-0 ROM does not pay.**
The real burst is therefore **~290,000 ticks.**

## 3. It is a path switch, not extra content

The functions that rise, and by how much:

| delta | spike | mean | ratio | function |
|---|---|---|---|---|
| +53,758 | 59,406 | 5,648 | **10.5×** | `ndsRendererHardwareSubmitVertex` |
| +41,209 | 45,533 | 4,324 | **10.5×** | `ndsRendererSubmitHardwareTriangle` |
| +32,072 | 35,339 | 3,267 | **10.8×** | `ndsRendererScanList` |
| +30,353 | 36,780 | 6,427 | 5.7× | `ndsRelocPointerRangeInLoadedFile` |
| +16,805 | 18,643 | 1,838 | 10.1× | `ndsRendererHardwareBeginTriangleBatch` |
| +13,623 | 15,153 | 1,530 | 9.9× | `ndsRendererDecodeInputVertex` |
| +12,510 | 16,387 | 3,877 | 4.2× | `ndsRelocFindLoadedFileContaining` |
| +9,282 | 10,268 | 986 | 10.4× | `ndsRendererApplyVertexCommand` |

And the functions that **fall**:

| delta | spike | mean | function |
|---|---|---|---|
| −44,623 | 25,224 | 69,847 | `__aeabi_fadd` |
| −32,383 | **1,493** | 33,876 | `gcPlayDObjAnimJoint` |
| −29,266 | 20,698 | 49,964 | `__aeabi_fmul` |
| −15,548 | 20,474 | 36,022 | `ndsRendererNativeShadeProductionActions` |
| −14,406 | **0** | 14,406 | `battleship_ftAnimParseDObjFigatree` |
| −13,116 | **0** | 13,116 | `_ntrcardRecvByCpu` |

`ScanList` / `DecodeInputVertex` / `ApplyVertexCommand` / `SubmitVertex` /
`SubmitHardwareTriangle` are the **generic display-list interpreter**. They run
about **ten times** their normal rate. Meanwhile animation joint playback drops
to essentially zero and figatree parsing to exactly zero.

So the expensive frame is not doing more of the same work. **The renderer has
left its native/compiled path and is interpreting display lists instead** — and
paying `ndsRelocPointerRangeInLoadedFile` 5.7× to validate pointers while it
does, which is what that path costs.

This is the best kind of P95 problem: the content is unchanged, so removing it
costs nothing visually. It is a fast-path residency question, not a geometry
budget question.

## 4. A correction to Task 66

Task 66 concluded from the tick-HUD buckets that `SRC` is the largest single
contributor to the P95. The per-PC data does not support that reading. On the
spike frame, **every** `SIM/*` group measured *lower* than on the mean frame
(`SIM/fighter` 34,842 vs 60,269), while `REND/renderer` rose 617,426 → 921,344
and `PORT/reloc` 242,340 → 325,483.

`SRC` spans `ndsTask39EffectsUpdate()` + `scVSBattleFuncUpdate()`
(`src/port/taskman_seam.c:4361`). Those call outward into the port's renderer
adapter and reloc backend, so a tick-HUD bucket named for the simulation is
charging renderer work. **`SRC` spiking does not mean the simulation spiked.**

Task 65's ranking is unaffected and in fact reinforced: the renderer is the
campaign, at 52% of mean work and now essentially all of the P95 burst too.

## 5. What to do next

1. **Find why the fast path is abandoned.** ~1 frame in 20, in runs of about
   five. Candidates: a texture or material not yet resident on first use, a
   run-cache generation miss, an animation change forcing re-preparation, or
   Task 44 steady-state admission failing for a frame. The counters to read are
   already present — `sNdsRendererRuntimeFrameSummary` carries
   `texture_lookup_miss_count`, `texture_lookup_active_hit_count` and probe
   counts, and Task 44 has its own admission counters. Sampling those per frame
   alongside the buckets is one run and no new instrumentation.
2. **Then Task 68 (texture/material resolution) on the steady-state P50.**
   Task 65 priced it at 114,031 ticks/frame at 5.74 cycles/instruction. The
   mechanism is now known precisely: `ndsRendererHardwareResolveOrBindTexture`
   (`src/nds/nds_renderer.c:11793`) builds a **236-byte**
   `NDSRendererHardwareTextureKey` from scratch on every textured run —
   `memset` of 236 bytes, **59 field assignments**, a hash, an open-address
   probe, and a confirming 236-byte `memcmp` on the hit. For a fixed
   Mario/Fox/Dream Land match that key sequence barely changes frame to frame.
   Note the existing comment at `nds_renderer.c:8173` records that hashing all
   59 words was already tried and cost more than it saved — so the lever is
   memoizing the resolve at the call site, not micro-optimising inside it.

## 6. Caveat

The spike census is **one window of two frames**. The path-switch signature is
unambiguous within it and the tick-HUD series shows the same `FTR`/`MISC`
signature at 478–482 and 438–439, but a second census windowed on 478 should
confirm before any fix is designed against this. That is one ~20-minute run.
