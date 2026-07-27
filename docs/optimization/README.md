# Optimization documents

## Read these first

- `TASK_STANDING_RULES.md` — how a performance task is run, measured and judged,
  and every rule a previous task learned the hard way. Read it before proposing
  or gating an optimization. It is the only file here that accumulates.
- `COMPILER_FIRST_ARCHITECTURE.md` — the standing plan the campaign executes
  against, amended in place as tasks refute or resize its premises.
- `../P1_EXECUTION_BOARD.md` — current priority and status. The only dynamic
  queue; do not maintain a second current-task roster here.
- `NATIVE_RENDERER_PLAN.md` — the retained renderer contract.

## Task documents

One task per file, named `<Model>_Task<NN>_<Slug>_<YYYYMMDD>.md`. They are dated
evidence, not plans: each records what was measured, the verdict (SHIPPED, KEEP,
REVERTED, STOP), and the numbers behind it. A task that measured nothing does not
get a file.

They are **append-only history**. Do not edit an old task's numbers to agree with
a newer measurement — write the correction in the new task and cite the old one,
the way Task 92 §4 corrected Task 78's sizing and Task 96 §4 corrected Task 92's
§5.

`archive/` holds retired queues and closed tasks. Move a task there once nothing
current cites it; leave the ones the standing rules still point at in place.

## Where the campaign stands (2026-07-27, Task 99)

`WORK-H` P95 **1,761,664** on the current tick-HUD build against
`PROJECT_GOAL.md`'s 1,120,000 gate — **641,664 over**.

**Two whole searches are closed by measurement, not by argument:**

- **Exactness-preserving** — nineteen tasks (78–96): texture memo (93),
  soft-float conversion (92), dense-vertex re-shade (90), animation as originally
  scoped (77 E1), `mem*` (87/88), placement (87/88/89/94), incremental animation
  reorganization (95), the wholesale animation channel rewrite (96). Nothing left
  above the ~20,000 ticks/frame bar.
- **Visual approximation, payload form** — three tasks (98, 99): cheaper vertex
  encoding (−27.2% of words → ~0 ticks), lower texture resolution (cost is
  ~1,621 per *bind*, flat in texel count), fewer triangles (−50% of the stage →
  −19,584, leaving the stage bucket ~89% fixed).

Both shared an unstated assumption — **that the frame's cost is something the
CPU does**. The record now contains a contradiction that only resolves if that is
wrong: Task 54 called the stage GX-throughput-bound, but the two quantities GX
throughput scales with, command words and triangles, were later measured at +64
and −19,584 respectively. That bucket has never actually been attributed.

The raster axis was opened on that reasoning and **closed by Task 100 at its
first test**: a quarter of the frame's pixels stopped being drawn and `STG` moved
−320 against a ≥40,000 kill criterion. It has an architectural reason — the DS
rasterizer consumes already-swapped polygon RAM during scanout and structurally
cannot stall the CPU — so pixels join words and triangles as a refuted currency.
Do not propose another fill, coverage, AA or overdraw lever.

**Task 103 then partitioned the stage bucket and found the campaign had been
looking in the wrong third.** Against a 388,480 `STG`:

| block | ticks/frame | share |
|---|---|---|
| **outside the segment commit entirely — never profiled** | **238,254** | **61.3%** |
| generic emit — 21 runs, 103 triangles | 63,903 | 16.4% |
| replay word push — 3,916 words @ 9.51 each | 37,244 | 9.6% |
| everything else in the run loop | 48,321 | 12.4% |

Tasks 51–55, 99 and 100 all optimised the run loop, which is 35% of the bucket.
Task 103 also retires Task 55 E2's "words are free" — words cost **9.51 ticks
each**, so that experiment was a below-noise null, not a refutation, and Task
98 §2's anchor row falls with it.

**Task 103 E3/E4 then closed the attribution.** All four writers of
`gNdsTickHudStageTicks` were tapped using the timestamps they already compute —
zero added instrument — and the partition closes to **192 ticks (0.05%)** against
the build's own `STG`, so it is exhaustive and does not double-count. The answer:

| block | ticks/frame | share of `STG` |
|---|---|---|
| **`ndsRendererPrepareNativeStageOwner`** — 1 call/frame | **160,588** | 40.8% |
| generic emit — 21 runs, 103 triangles | 63,607 | 16.2% |
| **`ndsRendererAdapterPrepareNativeStageMatrices`** — 1 call/frame | **55,077** | 14.0% |
| replay word push — 3,916 words | 37,233 | 9.5% |
| everything else | 76,967 | 19.5% |

**Two calls per frame are 215,665 ticks — 55% of the stage bucket and 16% of all
frame work**, and no task in this campaign has touched either. Task 44's
steady-state admission is confirmed working: the asset-lookup and topology path
it replaced costs 1,725 ticks.

Next: split those two internally with the same method before proposing a lever.
Both are per-frame preparation over a topology Task 44 has already proven
unchanged — the shape a memo or incremental update attacks, and with one call
per frame there is no per-run transfer problem of the kind that killed Task 79 E1.
