# Task 108 E0 — The load-free `SRC` excursion executes no extra code

**Date:** 2026-07-27
**Status:** E0 complete. **The load-free excursion is not executed work**, and
two instruments disagree about it by 46×. No runtime change.
**Inputs:** `artifacts/task108-loadfree-census/`, `artifacts/task108-median-census/`,
`artifacts/task108-loadframe-census/`, `artifacts/task75-load-census.json`.
**Follows:** Task 75 E0, which found 2 of 7 `SRC` excursions carry no load.

## 1. The question

Task 75 E0 found frames 453 and 454 running `SRC` at 2.0× and 1.9× its median
with zero cartridge loads. No task had profiled that population — Task 71's
window (469–470) contained a load. If the residual shares a cause with the loads
(relocation, figatree parse) one fix serves both and the preload's 103,488
ceiling rises; if not, the gate needs two fixes.

## 2. Three windows, one build, `run-task37-profile-census.ps1`

| window | loads | cycles | instructions | cyc/insn |
|---|---|---|---|---|
| 450–451 | **2** | 6,722,304 | **2,691,321** | 2.50 |
| 453–454 | 0 | 6,722,750 | 1,916,515 | 3.51 |
| 455–456 | 0 | 6,722,372 | 1,909,048 | 3.52 |

Against the tick HUD for the same frames:

| frame | `WORK-H` | `SRC` | `WAIT` | loads |
|---|---|---|---|---|
| 453 | 1,593,664 | 636,096 | 85,056 | 0 |
| 454 | 1,553,408 | 598,656 | 125,696 | 0 |
| 455 | 1,276,352 | 315,520 | 402,752 | 0 |
| 456 | 1,276,864 | 316,096 | 401,984 | 0 |

**The excursion frames execute 7,467 more instructions than the median frames —
0.4% — while the tick HUD reports them costing 297,000 ticks/frame more work.**
Total cycles are identical to 0.006%. `armWaitForIrq` differs by 11,262 cycles
where the tick HUD's `WAIT` differs by 296,992 ticks/frame.

A per-symbol diff of the two windows finds nothing: largest delta `__aeabi_fadd`
+5,290 cycles, sum of all positive deltas 25,527 cycles ≈ 6,381 ticks/frame.

## 3. The window is not the problem — validated, not assumed

The obvious suspicion is that the profiler windowed the wrong frames. It did not.
Windowing 450–451, which Task 75's ring reports as carrying two loads, produces
`_ntrcardRomReadSector` in the table and **+775,000 instructions** over both other
windows. Loads have an unmistakable signature and it appears exactly where the
independent counter says it should.

Two checks of mine failed and are recorded rather than quietly dropped:

- **The `ALL` cross-check proves nothing here.** I claimed 0.06% agreement
  between census cycles and the tick HUD's `ALL` for 453+454 established the
  window. It does not: every frame in this region is 3 VBlanks, so 455+456 match
  the same total to 0.05%. A quantized quantity cannot discriminate frames.
- **The off-by-one theory is refuted.** Comparing ring dumps, every excursion in
  the Task 75 window sits +1 from the same excursion in the Task 106 window
  (449→450, 464→465, 452/453→453/454) because the runs started one frame apart,
  which looked like a labelling offset. The load-frame test rules it out: frame
  450 is a load frame in both the ring and the profiler.

## 4. What this means

The excursion is real — two independent tick-HUD runs and a VBlank histogram
agree the frames are expensive — but it is **not code being executed**. Same
cycles, same instructions, same cyc/insn, no symbol carrying the difference.

The tick HUD computes `WORK = ALL − WAIT`, where `WAIT` is time parked in
`swiWaitForVBlank`. On these frames the CPU spends far less time there without
executing more. So the difference is time inside the update phase during which
the CPU is **blocked on something that is neither `swiWaitForVBlank` nor
`armWaitForIrq`** — a GX FIFO stall, a DMA wait, or a hardware register spin.
It is charged to `SRC` because it happens inside `scVSBattleFuncUpdate`, and it
is charged to `WORK` because it is not `WAIT`.

**That is why no per-PC census could ever have found it, and why every symbol-
ranked search has come back empty on these frames.**

## 5. Consequence for the campaign

`AGENTS.md` already records the general form of this: *"GX backpressure is
distributed into the named buckets as memory stall on the write that could not
retire."* This is the same effect landing in `SRC` rather than a renderer bucket.

Two consequences follow, and neither was visible before this task:

1. **The preload's ceiling does not rise.** The load-free excursion does not
   share a cause with the loads — it shares no executed code with anything. Task
   75's ~103,488 stands as the whole of what removing on-demand loading buys.
2. **A symbol-level lever cannot address the residual.** The remaining tail is
   a blocking relationship between the CPU and a hardware unit during the update
   phase, and it needs an instrument that can attribute *stall*, not cycles.

## 6. What E1 needs

Not another per-PC census. The question is which hardware unit blocks and on
what write. The candidates are the GX FIFO (`GFX_FIFO` stores that cannot retire
while the geometry engine is full), a DMA channel, and a cartridge register spin
that is not the sector read itself.

The cheapest discriminator is a bracketed counter inside `scVSBattleFuncUpdate`
around the suspected blocking sites, sampled per frame through the census ring
the way Task 75's load counter was — the ring already exists and takes one
selected source.

## 7. Cost

Four builds, four census runs. It closed the question Task 75 opened, corrected
two of my own checks, and established that the residual tail is not reachable by
the method every task from 78 to 105 used.
