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

**The exactness-preserving direction is exhausted.** No remaining lever of that
kind is above ~20,000 ticks/frame.

That is not the same as the campaign being blocked, and Task 96 first said it
was. Tasks 78–96 were all exactness-preserving *by choice*. `PROJECT_GOAL.md`
§Sacrifice Order does not ask for that: it ranks audio fidelity first, **visual
fidelity second**, the original 60 Hz simulation third and gameplay fidelity
fourth, and states that **stable 30 FPS is the most protected requirement**.
`AGENTS.md` already carries the procedure — rendering-side changes gate on a
reported fidelity budget (synchronized screenshot diffs plus the owner's visual
approval), not pixel exactness.

So the open direction is **visual approximation**: fewer vertices, coarser
textures, simpler lighting, fewer draw calls. The renderer classes are 290,406
ticks/frame between them and `fighter: native production` (255,061, which
includes prepared dense vertices) sits on top of that. It has not been tried
once in this campaign.

Its gate is the owner's eyes on a screenshot, which is why it is a decision and
a task rather than a task alone.
