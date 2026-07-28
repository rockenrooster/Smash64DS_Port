# Task 71 — The frames that set the P95 are loading assets off the cartridge

**Date:** 2026-07-26
**Status:** Census. Cause identified. No code changed.
**Inputs:** `artifacts/task71-clean-src-census/` (per-PC census windowed on
frames 469–470), against `artifacts/task65-census/` (128-frame mean).

Task 70 established that the native-owner fallback sets 0.44% of the `WORK-H`
P95, and that the gate is instead set by `SRC` excursions on frames with no
fallback at all — worth 259,584 ticks of P95, 30% of the gap. Every per-PC census
before this one was windowed on a fallback frame. This is the first census of a
frame that actually sets the statistic.

## 1. The window

Frame 469 was chosen because it isolates the variable: `FTR` 576,704, `STG`
387,200, `MISC` 47,424, `AUD` 1,216 — all at median — while `SRC` runs 1,036,544
against a 318,592 median. It is also a single-frame spike (468 ramps at 666,240,
470 is back to 324,352), so a 2-frame window brackets it.

Cross-check that the window landed where intended: census wall is 7,842,340
cycles over 2 frames = 1,960,585 ticks/frame, less `armWaitForIrq` 242,768 =
**1,717,817** of work. The tick HUD's independent `WORK-H` for 469 and 470 means
to **1,712,928**. Agreement 0.3%.

## 2. It is a file load

Per-symbol delta against the 128-frame mean, in ticks per frame (the window
averages 469 with a median frame, so the single-frame excursion is about twice
each figure):

| delta | frame 469–470 | mean | x | symbol |
|---|---|---|---|---|
| +71,644 | 131,369 | 59,725 | 2.2x | `memcpy` |
| +30,484 | 30,484 | — | new | `strncasecmp` |
| +28,241 | 28,241 | — | new | `ndsRelocFinalizeLoadedFile` |
| +25,960 | 39,359 | 13,399 | 2.9x | `_ntrcardRomReadSector` |
| +19,256 | 19,256 | — | new | `ntrcardRomRead` |
| +17,950 | 34,568 | 16,618 | 2.1x | `battleship_ftAnimParseDObjFigatree` |
| +11,710 | 11,710 | — | new | `ndsRelocApplyWordByteSwap` |
| +10,888 | 44,764 | 33,876 | 1.3x | `gcPlayDObjAnimJoint` |
| +10,470 | 10,470 | — | new | `mutexUnlock` |
| +9,306 | 9,306 | — | new | `ndsRelocAssetIDForToken` |

("new" means absent from the baseline's top-45 table, not that it never runs.)

That is a NitroFS open, a cartridge read, and a relocation, in order:
`strncasecmp` walks the filename table — 84 bytes of code at 1,451 cycles per
byte, so a tight comparison loop over directory entries; `ntrcardRomRead` and
`_ntrcardRomReadSector` pull the sectors; `memcpy` moves the payload;
`ndsRelocApplyWordByteSwap`, `ndsRelocAssetIDForToken` and
`ndsRelocFinalizeLoadedFile` relocate it; `mutexUnlock` is the filesystem lock.

The load-path symbols sum to **+207,071 ticks/frame** across the window, about
**414,000 on frame 469 alone**, against that frame's `SRC` excursion of 717,952.
Adding the animation work that follows the load — `battleship_ftAnimParseDObjFigatree`
and `gcPlayDObjAnimJoint`, +28,838 — accounts for roughly **two thirds** of the
excursion directly.

Note also what is *absent*: `ndsRendererHardwareSubmitVertex`,
`ndsRendererScanList` and `ndsRendererSubmitHardwareTriangle` do not appear in
this frame's top-45 at all. The generic display-list interpreter is not running.
This frame is clean in exactly the sense Task 70 measured, which is the control
this census needed.

## 3. What is being loaded

`src/port/reloc_backend_assets.c` carries a per-animation asset table:
`sNdsRelocFoxAnimFileIDs` indexes to 799, with matching
`NDS_RELOC_ASSET_MARIO_ANIM_*` and `NDS_RELOC_ASSET_FOX_ANIM_*` identifiers.
**Each fighter animation is a separate file in the ROM**, and
`battleship_ftAnimParseDObjFigatree` rising 2.1x on the same frame says the thing
loaded is animation data being parsed immediately after arrival.

So the mechanism is: a fighter enters a move whose animation is not resident, and
the game opens, reads, relocates and parses that animation inside the frame that
needs it. That is why the excursion is bursty, why it lands on `SRC` (the bucket
spanning `scVSBattleFuncUpdate()`), why `FTR` and `STG` stay flat, and why it
recurs — 26 of 128 frames run `SRC` above 1.5x median in a one-minute
Mario-versus-Fox match, which is about the rate at which two fighters use moves
they have not used recently.

## 4. Why this is the right target

`PROJECT_GOAL.md` states the trade directly: loading time is cheap, gameplay CPU
is precious, and ROM/RAM may be traded aggressively for speed. This is that case
in its purest form — the work is not rendering, not simulation, and not
mechanically required to happen during the frame. It is a synchronous cartridge
read on the critical path.

Three separable costs, in increasing order of effort:

1. **The path lookup.** `strncasecmp` at 30,484 ticks/frame is spent comparing
   filenames. The asset identity is already known numerically before the open.
2. **`ndsRelocAssetIDForToken`.** A chain of roughly a hundred comparisons
   followed by two linear scans of the Mario and Fox animation tables, on every
   relocation. +9,306 ticks/frame.
3. **The load itself.** Preloading a match's fighter animations at match start
   removes the read, the copy and the relocation from gameplay entirely. This is
   the large one, and the one the goal document most directly endorses.

## 5. Bounds, honestly

`SRC` held at its median is worth 259,584 of P95 (Task 70). This census attributes
about two thirds of one spike frame's excursion to the load path, so the
realistic ceiling for removing on-demand loading is roughly **170,000 ticks of
P95**, against a gap of 857,792 — about **20%**. Useful, the largest single
identified item, and not on its own sufficient.

It also has not been shown that every one of the 26 high-`SRC` frames is a load;
one frame has been profiled. The cheap confirmation is a counter on the load
entry point sampled per frame through the ring, the same shape as Task 70.
