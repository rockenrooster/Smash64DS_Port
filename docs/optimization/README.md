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

**The open direction is the raster axis** — pixels, the one quantity 99 tasks
have never varied. See `RASTER_AXIS_CAMPAIGN.md` for the thesis, its kill
criterion, and the task list. Its first task is also the control that decides
whether the emulator can see fill cost at all; every number this campaign owns
came from a fork that is cache-accurate for the ARM9 and is **not** a
cycle-accurate model of the DS rasterizer.
