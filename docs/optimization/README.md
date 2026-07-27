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

## Where the campaign stands (2026-07-26, Task 96)

`WORK-H` P95 **1,733,888** against `PROJECT_GOAL.md`'s 1,120,000 gate —
**613,888 over**. Every direction has been closed with a measurement: texture
memo (93), soft-float conversion (92), dense-vertex re-shade (90), animation as
originally scoped (77 E1), `mem*` (87/88), placement (87/88/89/94), incremental
animation reorganization (95), and the wholesale animation channel rewrite (96).

**No remaining lever above ~20,000 ticks/frame is permitted by the current
contracts.** Closing the gate now needs the owner to relax one — the sacrifice
order in `PROJECT_GOAL.md` is audio fidelity, then visual fidelity, then 60 Hz
simulation, then gameplay fidelity — or to accept the current frame rate. That
is a decision, not a task, and it is not mine to make.
