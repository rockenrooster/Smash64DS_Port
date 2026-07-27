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

## 7. E3 — the whole bucket, partitioned exactly

`gNdsTickHudStageTicks` is written at exactly four sites, all in
`src/port/reloc_backend_movement.c`. Each already computes the timestamp it
needs for the tick HUD, so tapping all four **adds no timer reads at all** —
which matters, because E0–E2's ~18,100 ticks/frame of instrument sat inside the
span being measured.

| site | ticks/frame | calls/frame | share |
|---|---|---|---|
| **prepare owner** | **236,039** | **1.0** | **60.0%** |
| display commit | 156,823 | 27.6 | 39.9% |
| finish owner | 417 | 1.0 | 0.1% |
| DObj traversal | 0 | 0.0 | 0.0% |
| **SUM** | **393,280** | | |

The E3 build's own `STG` P50 is **393,472**. The partition closes to **192
ticks, 0.05%** — so the four sites are exhaustive, they do not nest, and the
bucket does not double-count.

**`ndsRendererAdapterPrepareNativeStageOwner` costs 236,039 ticks/frame at one
call per frame.** Nothing in 103 tasks had ever profiled it.

## 8. E4 — inside that one call

| step | ticks/frame | share |
|---|---|---|
| **`ndsRendererPrepareNativeStageOwner`** | **160,588** | **68.6%** |
| **`ndsRendererAdapterPrepareNativeStageMatrices`** | **55,077** | **23.5%** |
| validate Task 36 world | 8,540 | 3.6% |
| prepare materials | 5,522 | 2.4% |
| config / frame setup | 2,535 | 1.1% |
| admit / revalidate (Task 44 steady path) | 1,725 | 0.7% |
| SUM | 233,987 | (vs 234,926 at the site — 939 in entry/exit) |

Task 44's steady-state admission is working exactly as designed: the whole
asset-lookup and topology-rebuild path costs **1,725 ticks**. The cost is
entirely in what runs *after* admission.

## 9. The whole stage bucket, in one table

Everything above composed, against `STG` 393,472:

| block | ticks/frame | share |
|---|---|---|
| **renderer prepare owner** — 1 call | **160,588** | 40.8% |
| generic emit — 21 runs, 103 triangles | 63,607 | 16.2% |
| **prepare matrices** — 1 call | **55,077** | 14.0% |
| replay word push — 3,916 words @ 9.51 | 37,233 | 9.5% |
| replay begin-run — 33 runs | 17,223 | 4.4% |
| run-loop overhead | 13,466 | 3.4% |
| per-segment scaffolding — 8 commits | 13,280 | 3.4% |
| validate Task 36 world | 8,540 | 2.2% |
| prepare materials | 5,522 | 1.4% |
| replay tail + end-batch | 4,449 | 1.1% |
| config / frame setup | 2,535 | 0.6% |
| admit / revalidate | 1,725 | 0.4% |
| finish owner + dispatch remainder | ~2,400 | 0.6% |

**Two function calls per frame are 215,665 ticks — 55% of the stage bucket and
16% of all frame work.** Neither has been touched by any task in this campaign.

For scale: the gate needs **641,664**, and `fighter: native production` — the
largest class in Task 81's partition — is 255,061.

## 10. E5/E6 — inside the 160,588, and the head is the target

| step | ticks/frame | calls/frame | per call |
|---|---|---|---|
| **`ndsRendererNativeStagePrepareRun`** | **97,044** | 21.0 | 4,621 |
| `ndsRendererNativeStageApplyStateSpan` | 30,895 | 21.0 | 1,471 |
| init stats + traversal | 26,218 | 8.0 | 3,277 |
| Task 36 prepared-segment reuse | 11,858 | 8.0 (3 hit, 5 miss) | 1,482 |
| validate topology | 554 | 1.0 | 554 |

**Only 3 of the 8 segments hit the Task 36 prepared-segment reuse.** The five
misses are segments 1,2,3,4,6, and they own exactly the 21 runs — the same 21
that take the generic emit path in §4. `NDS_TASK36_REPLAY_SEGMENT_MASK` is
`(1<<0)|(1<<5)|(1<<7)`, and the exclusion is deliberate and correct: the Task 36
Phase B conservation census records that mode 2 "captures and replays only
complete rigid segments 0, 5 and 7 … Dynamic segments remain on the live path."
Widening the mask would freeze moving stage geometry. **Not a free lever.**

Splitting `PrepareRun` itself:

| | ticks/frame |
|---|---|
| **head — policy match, two memsets, texture resolve, colour/alpha queries** | **67,119** |
| dense-vertex loop | 23,351 |

143 dense vertices/frame at 163 ticks each; 57 of them (39.9%) take the
camera-dependent near-plane transform, 86 do not. **The memo candidate is those
86**, worth at most ~14,000 — below the bar.

**The head is the target: 67,119 ticks/frame, 3,196 per run.** Its inputs are
`stats`, which the state spans build from compile-time constant tables, so for a
given run it plausibly recomputes an identical answer every frame. Task 98 puts
~1,621 of each run's head in `resolve-or-bind` alone.

## 10a. E7 — the one lever tried, and REVERTED

On a reuse hit, `ndsRendererTask36ReplayUsePreparedSegment` assigns the whole
stats struct, so everything `ndsRendererInitStats` wrote a line earlier is
overwritten and the traversal state is never read before the `continue`. Both
resets are provably dead on the 3 segments the replay serves. Hoisting the reuse
check above them is an exact reordering — the check never reads the incoming
stats.

E6's 3,277-per-call figure predicted ~9,800 ticks/frame. Measured, on a matched
pair from one source file differing only in this block's position:

| bucket | control | candidate | Δ |
|---|---|---|---|
| `STG` P50 | 370,048 | 367,296 | −2,752 |
| `STG` P95 | 376,064 | 374,528 | −1,536 |
| `FTR` P50 | 543,552 | 548,352 | +4,800 |
| **`WORK` P50** | 1,332,864 | 1,342,272 | **+9,408** |
| `WORK` P95 | 1,826,816 | 1,762,752 | −64,064 |

**REVERTED.** `STG` moved the right way but by **less than the 5,000–7,000
floor**, so the improvement is not even resolvable; and `FTR` rose 4,800, which
this change cannot cause and which is the re-addressing collateral that closed
Tasks 87–89, 94 and 95. Same shape as Task 95: mechanism correct, frame worse.

**The estimate was 3.5× too high, and the reason is methodological.** Bracketing
a *short* span with two `cpuGetTiming()` reads over-attributes, because the reads
are a large fraction of what is being timed. The init pair really costs ~900 per
call, not 3,277. Long spans (the 67,119 head, at 3,196 per call) are not affected
— there the reads are ~4%. **Trust E-series span numbers in proportion to span
length**, and re-derive any lever sized from a sub-1,000-tick span before
building it.

## 11. Next

**The `PrepareRun` head — 67,119 ticks/frame over 21 runs.** It is now the
largest precisely-sized block in the stage that nothing has attacked, it sits on
a long span so its measurement is trustworthy (§10a), and its inputs are
constant-table-driven render state rather than anything the camera moves.

Before building a memo there, check Task 81's "stage memo inert" result — that
task closed a texture-direction memo and this must not repeat it. If the earlier
memo was keyed differently or applied at the emit rather than the prepare seam,
this is a different lever; if not, it is closed and the answer is to say so.

`ndsRendererAdapterPrepareNativeStageMatrices` (55,077, one call) is second, but
Task 44 already reduced it to exactly the 16 dynamic bindings, so what remains is
real matrix composition rather than redundancy.

## 12. State

`WORK-H` P95 1,732,608 on the clean baseline against the 1,120,000 gate — no
change, because **no lever shipped in this task.** E7 was tried and reverted
(§10a); the tree carries only the default-off census instrument and
`scripts/census-stage-run-phases.ps1`. Both the default and census builds were
rebuilt clean after the revert.

What this task did deliver is attribution: the stage bucket is now accounted for
to within 0.05%, three of its blocks are sized to the function, and the next
target is named with a number instead of a hypothesis.
