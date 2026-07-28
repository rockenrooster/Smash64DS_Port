# R2-02 E8 — the preflight for five segments had no reader

**Date:** 2026-07-28
**Phase:** R2-02 (`Smash64DS_Runtime2_SwitchPlan.md` §7)
**Verdict:** **KEEP, graduated to default-on. R2-02's stage budget is met.**
`STG` P50 **212,480 → 177,088** (−35,392), against the 180,000 phase budget —
**2,912 under**. P95 219,072 → 182,976. 2-VBlank frames **13 → 198 of 565**.

---

## 1. What this was for

After E7 the stage stood at 212,480 against a 180,000 budget, 32,480 over, and
the board's ranked list had been built from a partition measured on the
*defaults* build. Re-censusing on the graduated program moved the target:

```text
STG 242,574 (census window, frames 439-499)
  prepare owner       111,849  46.1%
    prepare matrices    42,557        (54,901 before E7)
    renderer prepare    49,840
      apply state span    20,370   21 calls @ 970
      init stats+trav     13,565    5 calls @ 2,713
      unattributed        13,721
      prepare run            995   21 calls @ 47   (E1a took this from 98,828)
    validate task36 world 8,588
    prepare materials     5,623
  display commit      130,219  53.7%
    generic emit         67,126   21 runs @ 3,196, 103 tris @ 652
    replay               29,124   33 runs @ 883
    loop overhead        13,120
    per-segment scaffold 13,852
```

Two numbers name the cut. `init stats + traversal` is **5** calls — exactly the
five segments the Task 36 replay does not serve (1, 2, 3, 4, 6; the census also
prints `task36 reuse: 3.0 hits, 5.0 misses`). `apply state span` is **21** calls
— exactly the 21 runs those five segments own. Both are per-frame reconstruction
of a state over a topology Task 44 has already proven unchanged.

## 2. The finding

Following what those five segments' preflight actually produces:

| output | who reads it, in the steady state |
|---|---|
| `runs[]` | E1a reuses the whole table; `PrepareRun` is already skipped on exactly this condition |
| `epoch_mask` | restored from E1a's memo before the loop |
| `preflight_stats` | `ndsRendererTask36ReplayCapturePreparedSegment` **early-returns for an ineligible segment**; the next segment reinitialises it |
| `traversal` | `sNdsNativeStageOwnerExecution.traversal` is referenced **nowhere outside this function** — the commit path consumes `runs[]`, not the traversal |
| `sync_command_count` | the one member that escapes the loop, assigned into the caller's stats |

So once E1a's table is valid, the entire per-segment preflight body for those
five segments — a 1,292-byte `ndsRendererInitStats`, an
`ndsRendererInitTraversalState`, 21 run-level and 16 binding-level state spans —
computes a `preflight_stats` and a traversal state that **nothing reads**.

Task 104 had already written the second half of this down, one level lower, in
`ndsRendererTask36ReplayUsePreparedSegment`: *"once the segment loop ends the
only member anything reads is `sync_command_count`"*. Task 104 acted on it for
the three replay-hit segments. The same sentence was true of the other five, and
of the whole loop body rather than just the clear-and-copy.

This is §7 read literally — *"no generic preflight, no stats temporaries"* — and
it is the first R2-02 arm that deletes preflight rather than optimising it.

## 3. The change

In the segment loop, before `ndsRendererInitStats`:

```c
if ((r2_reuse != 0u) &&
    (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE))
{
    gNdsR2StagePreflightElideCount++;
    continue;
}
```

Two deliberate details:

- **Eligible segments are excluded by name, not by "the replay hit".** On a
  capture frame `frame_replay` is FALSE, so segments 0, 5 and 7 must run the
  full body to produce the stats being captured. Keying on eligibility rather
  than on the hit keeps the capture frame correct.
- **`sync_command_count` is memoised** into `r2_prepared_sync_count` beside
  `r2_prepared_epoch_mask`. With today's mask the last segment (7) is
  replay-served and overwrites the field anyway, so the memo changes nothing —
  but that is a coupling to which segment happens to run last, and R2-02 has
  already been bitten once by an invariant that lived only in a mask
  (`NDS_TASK36_REPLAY_SEGMENT_MASK`, E3/E4). Four bytes removes it.

## 4. Engagement

A flag that compiles but never fires is indistinguishable from a null result,
and this campaign has shipped that mistake (Task 52). Read from the running ROM:

```text
AT500  frames=500  elide=2495  reuse=499  build=2
AT600  frames=600  elide=2995  reuse=599  build=2
TASK36 outcome=2 (READY)  segmask=0xa1  words=3916
```

**5 elisions per frame** — the five ineligible segments — 1 reuse per frame, and
only 2 build frames in the whole run. The Task 36 replay is still READY at its
full 3,916 words, so the cut did not disarm the mechanism it depends on.

## 5. Result

Control `build-r2-02-graduated` (E1a+E2+E7, ROM `DFBE1ED0E2BB97DB`), candidate
`build-r2-02-e8-on` (ROM `6147F78C30FDF7BF`), 128-frame ring dump.

| bucket | control P50 | E8 P50 | Δ P50 | control P95 | E8 P95 | Δ P95 |
|---|---:|---:|---:|---:|---:|---:|
| `STG` | 212,480 | **177,088** | **−35,392** | 219,072 | **182,976** | **−36,096** |
| `WORK` | 1,201,728 | 1,163,328 | −38,400 | 1,669,824 | 1,592,320 | −77,504 |
| `WORK-H` | 1,196,096 | 1,159,232 | −36,864 | 1,593,728 | 1,579,584 | −14,144 |
| `WAIT` | 475,200 | 272,832 | −202,368 | 539,072 | 555,200 | +16,128 |
| `FTR` | 554,304 | 554,880 | +576 | 998,400 | 1,004,288 | +5,888 |
| `ALL` | 1,680,000 | 1,679,936 | −64 | 1,680,512 | 1,680,576 | +64 |

VBlank intervals: control 2:13 3:538 4:10 5+:5 max 18; **E8 2:198 3:349 4:14
5+:4 max 18**, 565 frames. 35% of frames now present in two VBlank intervals
where 2% did before. `WAIT` falling 202,368 at P50 is the same fact from the
other side — the frame finishes earlier and spends less time held at the VBlank.

`ALL` is flat because it is VBlank-quantized and the median frame is still three
intervals; the histogram is the pacing signal, not `ALL`.

## 6. Exactness

The cut removes work, so the obligation is to show the render did not change.

**The presented-frame counter is the wrong lock and it nearly produced a false
regression.** Both arms stopped at `gNdsBattlePlayablePacingPresentedFrames ==
1100` differed on 57% of the top screen — and they were at
`gSCManagerBattleState->time_remain` **1790 and 1792**, two simulation ticks
apart. A faster build presents more frames per unit of simulation, so the
presented-frame index drifts against the match clock, and it drifts *more* the
better the optimization works.

Locked on the match clock instead, which decrements once per simulation tick and
is identical across builds by construction:

| lock | presented frame | top-screen pixels differing (x 8–407, y 48–351) |
|---|---:|---:|
| control run A vs control run B, frame 1100 | 1100 | **0** of 121,600 |
| control vs E8, presented frame 500 | 500 | **0** of 121,600 |
| control vs E8, `time_remain` 1800 | 1095 both | **0** of 121,600 |

Zero — not "within a budget". The first row is the instrument's own control: two
runs of the same ROM are bit-identical, which is what establishes that a
difference elsewhere would have been real.

Screenshots: `artifacts/visibility/r2-02-e8/`.

## 7. What is left

`STG` 177,088 against 180,000. **The phase's stage budget is met.** The ladder:

```text
STG P50   351,488  baseline
          256,704  after E1a  (-94,784)  prepare-run elision
          224,320  after E2   (-30,912)  GXFIFO DMA rigid replay
          212,480  after E7   (-11,840)  view-projection hoist
          177,088  after E8   (-35,392)  preflight elision
          180,000  gate                          -- MET, 2,912 under
```

The remaining stage cost is now majority `display commit`, and the single
largest item in the whole bucket is **`generic emit` at 67,126 ticks/frame for
21 runs and 103 triangles** — 3,196 per run against the replay's 883, and 652
per triangle against ~294. That is where the next stage work is, and E4 already
established that it cannot be reached by widening the replay masks. Note also
that `apply state span` and `init stats+traversal` were only *elided*, not
deleted: the build frame still pays them, and the generic emit path still
reconstructs the same state at commit time.

## 8. Cost of the lesson

Two harness defects cost more than the change did, and both are now rules.

**The capture photographed the wrong window.** `Graphics.CopyFromScreen` at a
window rectangle captures whatever is on top at those coordinates, and
`SetForegroundWindow` does not reliably raise a window for a background process.
Two captures of an unrelated application came back byte-identical, which reads
as "no visual delta" and was one step from being written up as evidence. It was
caught by looking at one of the images. `capture-melonds.ps1` already does this
correctly with `ShowWindow` + `HWND_TOPMOST`; the scratchpad capture did not.

**The frame lock compared two different moments.** See §6. The general form is
worse than it looks: the drift is proportional to how much the candidate
improved, so this failure mode gets stronger as the optimization gets better.

Both are in `TASK_STANDING_RULES.md`. The third, smaller item: the Cut-G exact
capture harness asserts the GO! sprite census and refuses every frame outside
that window, so it cannot be used to compare mid-match stage geometry at all —
which is why a scratchpad capture existed to get wrong in the first place.
