# Task 103 — 61% of the stage bucket is outside the path five tasks optimised, and GX words are not free

**Date:** 2026-07-27
**Status:** Census complete, three findings, one **correction to the record**.
No runtime change; the instrument is kept default-off with its script, on the
Task 91 precedent.
**Authorized by:** the owner — "execute the plan RASTER_AXIS_CAMPAIGN.md".
**Inputs:** `artifacts/task103-stage-run-phases.json` (E0),
`artifacts/task103-e1-stage-run-phases.json`,
`artifacts/task103-e2-stage-run-phases.json`,
`artifacts/task103-e2-buckets.json`. Builds `builds/build-task103-{census,e1,e2}`,
60-frame two-stop delta over frames 439–499, plus a 128-sample ring dump of the
E2 build for its own `STG`.

## 1. Why this ran

Task 99 left the stage bucket "89% fixed" and Task 100 refuted the last proposed
currency for that remainder. `RASTER_AXIS_CAMPAIGN.md` fork B asked what the
~331,300 ticks actually are, with the hypothesis that they are per-run
scaffolding — ~6,135 over each of 54 runs.

The hypothesis was wrong about the arithmetic and, more usefully, wrong about
where to look.

## 2. The partition

`NDS_TASK103_STAGE_RUN_PHASE` times the stage replay run's three spans, the
generic branch of the same loop, the whole loop body, the segment commit, and
the material prep ahead of it. It never removes a run: Task 99 arm C varied run
count by culling and measured **+109,888**, because that disarms the Task 36
capture-once replay.

Against this build's own `STG` P50 of **388,480**:

| block | ticks/frame | share of `STG` |
|---|---|---|
| **outside the segment commit entirely** | **238,254** | **61.3%** |
| generic emit — 21 runs, 103 triangles | 63,903 | 16.4% |
| replay word push — 3,916 words | 37,244 | 9.6% |
| replay begin-run — 33 runs | 17,401 | 4.5% |
| per-segment scaffolding — 8 commits | 13,231 | 3.4% |
| run-loop overhead | 13,097 | 3.4% |
| replay tail + end-batch | 4,592 | 1.2% |
| material prep | 759 | 0.2% |

The instrument costs ~18,100 ticks/frame (`STG` 370,368 → 388,480, ~250 timer
reads), nearly all of it inside the commit path, so the "outside" figure is
lightly *under*-stated rather than inflated.

## 3. Finding 1 — the stage's largest block was never in the loop

**61% of the stage bucket is outside `ndsRendererCommitNativeStageSegment`
altogether.** It is in the owner prepare path
(`ndsRendererAdapterPrepareNativeStageOwner`,
`src/port/reloc_backend_movement.c:13704`) and the other two `STG` accumulation
sites at `:13251` and `:13336`.

Tasks 51, 52, 53, 54, 55, 99 and 100 all worked on the run loop. The run loop is
**136,236 ticks/frame — 35% of the bucket.** Task 53's −33% and Task 54's
"GX-throughput-bound" were both statements about a third of the stage.

Nothing has profiled the other two thirds. That is the largest unattributed block
in the frame and it is where the next task goes.

## 4. Finding 2 — 21 runs carrying 103 triangles cost 63,903

The run loop runs 54 iterations: **33 replayed, 21 generic.** The replayed 33
carry 3,916 GX words for 59,236 ticks — 1,795 per run. The generic 21 carry
**103 triangles between them** and cost **63,903 — 3,043 per run, 620 per
triangle.**

Twenty-one runs the Task 36 replay does not serve cost more than the
thirty-three it does. That is a concrete, sized, unclaimed lever: bringing them
under the replay is worth up to ~64,000 ticks/frame, and realistically less
because the replay has its own per-run cost of 1,795.

It also re-reads Task 99 cleanly. Its probe halved triangles at the generic emit
seam; only ~103 triangles/frame reach that seam, so removing ~51 for −19,584 is
**~384 ticks per triangle**, in the same range as the 620 measured directly here,
not the ~194 the task computed against a static table count of 202.

## 5. Finding 3 — GX words cost 9.51 ticks each, and Task 55 E2 was a null below noise

The replay's word-push loop is **37,244 ticks/frame for 3,916 words = 9.51 ticks
per word.** A store to `GFX_FIFO` costing nine ticks is FIFO backpressure, and it
is the one place the campaign's backpressure reasoning was right.

**This overturns the reading of Task 55 E2.** That experiment elided 355 GX words
per frame and measured `ALL` P50 at **+64**, which Tasks 55, 98 and this
campaign's own opening all read as "words do not cost ticks."

At 9.51 ticks/word, 355 words is **~3,376 ticks** — comfortably below the
**5,000–7,000** build-placement noise floor Task 100 arm C measured directly. Task
55 E2 did not show that words are free. It showed that removing 355 of them is
below what this instrument can resolve, which is a different statement and does
not generalise.

Task 98 §2's table has "GX words −355/frame → +64 → expected ~0" as its anchor row
and built the "cost is per-datum vs per-operation" thesis on it. That row is
retired. The correct statement is that the whole word stream is worth 37,244
ticks/frame, which is real but bounded, and that no lever removing a few hundred
words can ever be measured on a ±7,000 floor.

## 6. What this changes

- **`RASTER_AXIS_CAMPAIGN.md` fork B is answered and superseded.** The ~331,300
  is not per-run scaffolding: begin-run plus tail plus per-segment scaffolding
  plus loop overhead is 48,321 across everything, ~12% of the bucket.
- **The per-operation rule from Task 99 §4 survives but was aimed at the wrong
  operations.** The expensive operation is not a run or a bind — it is whatever
  the owner prepare path does 61% of the bucket's worth of.
- **Two sized levers now exist**, which is two more than the campaign had this
  morning: the 21 generic runs (≤64,000) and the unprofiled 238,254.

## 7. Next

Profile the stage owner prepare path — the three `gNdsTickHudStageTicks`
accumulation sites in `src/port/reloc_backend_movement.c` — with the same
in-place span method. It is 61% of the stage bucket and has never been looked at.

## 8. State

`WORK-H` P95 1,732,608 on the clean baseline (`build-task100-A`) against the
1,120,000 gate. The Task 103 instrument is default-off; `scripts/census-stage-run-phases.ps1`
drives it. No runtime change.
