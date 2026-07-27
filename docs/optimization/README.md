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
**Two sized levers now exist:** the 21 generic runs the Task 36 replay does not
serve (≤64,000), and the 238,254 in the owner prepare path that nothing has
looked at. Task 103 also retires Task 55 E2's "words are free" — words cost
**9.51 ticks each**, so that experiment was a below-noise null, not a refutation,
and Task 98 §2's anchor row falls with it.

Next: profile the three `gNdsTickHudStageTicks` accumulation sites in
`src/port/reloc_backend_movement.c` with `scripts/census-stage-run-phases.ps1`'s
in-place span method.
