# Task 83 — Re-census after the repack: two instruments agree, and the "in reach" metric does not

**Date:** 2026-07-26
**Status:** Complete. Closes the follow-up Task 82 E0 §8.5 required. No runtime
change.
**Inputs:** `artifacts/task83-recensus/` against `artifacts/task78-anim-census/`
— same window (frames 439–567), same configuration, differing only by
`NDS_TASK82_ITCM_REPACK`.

## 1. Why this ran

Task 82 E0 required it in writing: a repack invalidates the ranking that chose
it. Leaving it undone is exactly how the previous pack went stale, so this is the
follow-up rather than a discovery.

## 2. The repack is confirmed by a second instrument

| | before | after | Δ |
|---|---|---|---|
| frame total | 1,833,713 | 1,820,582 | −13,131 |
| **work** (excl. idle) | 1,515,768 | **1,497,682** | **−18,086** |
| stall (excl. idle) | 944,626 | 927,683 | −16,942 |
| idle | 317,945 | 322,900 | +4,955 |

The tick HUD measured `WORK-H` P50 −19,264 for the same change. The per-PC
profiler says work −18,086. **Two independent instruments agree to within 6%**,
and the idle rise confirms the shape: the frame finishes earlier and waits
longer.

Stall fell 16,942 of the 18,086, which is the mechanism the repack targets.

## 3. The admitted symbols moved, and barely improved

| symbol | before | after | Δ | tier |
|---|---|---|---|---|
| `ndsRendererTask36ReplayRun` | 41,200 | 40,316 | −885 | `.text.hot.draw` → `.itcm` |
| `ndsRendererHardwareGetLightShadeLut` | 20,132 | 17,556 | −2,575 | `.main` → `.itcm` |
| `ndsRendererMtxMul20p12` | 29,646 | 28,800 | −846 | `.text.hot.draw` → `.itcm` |
| `ndsRendererMtxLoadN64ToDS20p12` | 14,857 | 13,506 | −1,351 | `.text.hot.draw` → `.itcm` |

Total directly attributable to the moved symbols: **−5,657**. The frame improved
by **18,086**. So **69% of the win is not in the symbols that moved** — it is in
the `.text.hot`, `.text.hot.draw` and `.main` space they vacated, which re-laid
out everything else.

## 4. The "non-mem stall in reach" metric is badly optimistic per symbol

Census section D predicted `ndsRendererTask36ReplayRun` had **4,062,873 cycles
(15,870 ticks/frame) of non-mem stall in reach**. Moving it to ITCM bought
**885**. That is an overestimate of roughly **18×**.

Task 82 E0 §6 flagged this as an upper bound and gave the reason — ITCM removes
instruction-fetch stall but not pipeline hazards or branch mispredicts, and the
metric counts both. The measurement now puts a number on how loose it is.

**What section D is good for is ranking, not sizing.** The order it produced was
right: the repack won. The magnitudes it produced were wrong by more than an
order of magnitude per symbol, and the aggregate only came out well because the
layout effect made up the difference. No future task should quote an "in reach"
figure as an expected result.

## 5. The remaining ITCM lever is small

New headroom, from the fresh census:

```
zero-eviction pack:   724 B, 3,139,642 non-mem stall cycles in reach
with eviction:      3,158 B, 8,754,979 non-mem stall cycles in reach
```

Down from 5,178 B / 17,163,581 before. Applying §4's correction, the realistic
remaining value is **a few thousand ticks at most**, not the 34,199 the raw
figure implies. ITCM is now essentially packed: 32,012 of 32,768 bytes, with the
top of the ranking already resident.

**Do not run a third repack** without a new reason — the cheap half is taken.

## 6. The ranking the next task should use

Work is now 1,497,682 ticks/frame.

| family | ticks/frame | % of work |
|---|---|---|
| soft-float | 175,920 | 11.7% |
| matrix | 157,560 | 10.5% |
| tex/material | 140,440 | 9.4% |
| `mem*` | 136,676 | 9.1% |
| animation | 83,202 | 5.6% |

Top of the frame by stall:

```
 41,349  3.68  .itcm           memset
 35,900  6.56  .main           ndsRendererHardwareResolveOrBindTexture
 32,556  2.22  .itcm           memcpy
 30,966  4.31  .itcm           ndsRendererTask36ReplayRun
 30,612  2.54  .itcm           ndsRendererNativeShadeProductionActions
 29,971  5.47  .main           ndsFighterMarioFoxDLAllDrawForSlot
 26,630  4.54  .itcm           ndsRendererExecuteNativeFighterOwnerProduction
```

The ordering is essentially unchanged — the repack moved where code lives, not
what it does. Tasks 79–81 closed texture/material memoisation; the live
candidates are:

- **`mem*` (136,676, and `memset`+`memcpy` are the two largest stall rows).**
  Unsized by call count, which after Tasks 79–81 is the first thing it should
  get. `memset` moves 121–242 KiB/frame and `memcpy` 208–416 KiB/frame by
  instruction count; nothing has established what is doing that.
- **soft-float (175,920).** Largest family, but it runs at ~1.15 cycles per
  instruction — genuine compute, not stall. It only shrinks by executing fewer
  float operations, which means finding the callers, and the per-PC profiler
  cannot attribute leaf helpers to callers.

`mem*` is the better next target: it is stall-dominated, it has an obvious
denominator question, and the answer is likely a small number of large clears.
