# Task 81 — The frame as a partition, and the one class that is not stall

**Date:** 2026-07-26
**Status:** Complete. Executes the last live item on
`docs/optimization/COMPILER_FIRST_ARCHITECTURE.md`'s roadmap. No runtime change.
**Inputs:** `artifacts/task81-recensus/` (fresh, current master),
`artifacts/task81-partition.json`, `scripts/task81_partition_census.py`.
**Compared against:** `artifacts/task83-recensus/` (pre-Tasks 85/86/90).

## 1. What the plan asked for, and why it mattered

> "re-derive the category totals at the top of this document **as a partition
> rather than as overlapping families**, with stall separated from instruction
> cost."

The document's opening table carries its own warning: soft-float, `mem*` and
matrix work all occur *inside* renderer work, "these are overlapping categories,
not a partition", and "a task may not claim two of them as independent budgets."
That warning made the table unusable for scheduling — every lever looked larger
than it was, because its cost was also counted inside another row. Three tasks
have now been mis-sized against it.

`scripts/task81_partition_census.py` replaces it. Every symbol lands in exactly
one class, first match wins, leaf kernels claimed before caller-shaped classes
so a float helper is never charged to both itself and its caller. The classes sum
to the frame.

## 2. The partition

Fresh census, 128 presented frames from 439, tick-HUD ROM, current master.
Total 1,759,350 ticks/frame; idle 306,358; **work 1,452,992**.

| class | ticks/f | %work | stall | stall% | instr | vs prior |
|---|---|---|---|---|---|---|
| **fighter: native production** | **255,061** | 17.6% | 170,448 | 66.8% | 84,614 | −2,473 |
| **soft-float + libgcc** | **191,810** | 13.2% | 36,659 | **19.1%** | **155,151** | −1,525 |
| fighter: generic scaffolding | 173,892 | 12.0% | 129,455 | 74.4% | 44,437 | +5,999 |
| `mem*` (libc) | 116,671 | 8.0% | 76,336 | 65.4% | 40,335 | **−20,508** |
| renderer: texture + material | 109,208 | 7.5% | 88,919 | 81.4% | 20,290 | −3,334 |
| renderer: matrix | 107,810 | 7.4% | 56,788 | 52.7% | 51,022 | −2,275 |
| animation (`gc*` + figatree) | 106,700 | 7.3% | 73,020 | 68.4% | 33,680 | −45 |
| unclassified | 78,204 | 5.4% | 57,011 | 72.9% | 21,193 | −900 |
| stage: native replay | 77,069 | 5.3% | 56,665 | 73.5% | 20,404 | −975 |
| fighter simulation (`ft*`) | 53,251 | 3.7% | 44,962 | 84.4% | 8,289 | −485 |
| renderer: GX submit + vertex | 47,555 | 3.3% | 26,598 | 55.9% | 20,957 | −523 |
| stage: geometry + collision | 34,036 | 2.3% | 23,687 | 69.6% | 10,349 | +323 |
| resource relocation + cart | 33,776 | 2.3% | 17,703 | 52.4% | 16,073 | −935 |
| renderer: other | 25,833 | 1.8% | 18,494 | 71.6% | 7,339 | **−16,358** |
| game/scene/system | 21,553 | 1.5% | 17,512 | 81.3% | 4,041 | −260 |
| platform + HUD | 18,791 | 1.3% | 10,303 | 54.8% | 8,488 | −413 |
| audio | 1,737 | 0.1% | 1,553 | 89.4% | 183 | −37 |
| **sum of classes** | **1,452,958** | **100.0%** | | | | |

Sum matches work to 34 ticks of rounding. It is a partition.

The two large negative deltas are this session's shipped work landing where it
should: `mem*` −20,508 is Tasks 85 and 86, and `renderer: other` −16,358 is
Task 90's light-shade LUT cache.

## 3. Three findings that change the campaign's direction

**(a) The generated path is the single largest class.** `fighter: native
production` — the precompiled runs, epochs, state deltas and prepared dense
vertices that `COMPILER_FIRST_ARCHITECTURE.md` was written to produce more of —
is **255,061 ticks/frame, 17.6% of work**, the biggest entry in the table. The
generic scaffolding around it is 173,892, and Task 91 E1 measured only 13,888 of
that as removable. Generating more native code does not shrink this; the native
code *is* the cost.

**(b) Everything is stall except one class.** Sixteen of seventeen classes run at
52–89% stall. That is the pointer-chasing data-layout problem
`COMPILER_FIRST_ARCHITECTURE.md` names, and it is real — but ITCM is packed to
32,012 of 32,768 bytes and Task 83 closed further repacking ("do not run a third
repack"), Tasks 87–89 established the layout is at a local optimum, and three
attempts to move it regressed.

**(c) `soft-float + libgcc` is the exception: 191,810 ticks, 155,151
instructions, 19.1% stall.** It is the only class in the frame that is
**instruction-bound rather than stall-bound**. It is second largest. It cannot be
fixed by placement, layout, caching or generation — only by executing fewer
floating-point operations.

## 4. The next task, and the one thing blocking it

Per the plan: *"Rank the new top kernels. The next task comes from the new
profile."* The profile names soft-float, on (c).

Two directions are already closed and must not be reopened:

- **Hardware divider / sqrt: closed by Task 50 at E0.** Eligible ceiling ~0.55%
  of budget; the dense hot divide sites live in `gmcollision`, `mp*`,
  `ftMainProcPhysicsMap` and `ftComputer` — gameplay the state hash forbids
  changing. The `__aeabi_ddiv` win the spec anticipated is absent in battle.
- **Blanket float→fixed conversion of gameplay.** Same barrier, and
  `PROJECT_GOAL.md` gates gameplay on mechanical equivalence.

What is *not* closed: soft-float called from **renderer-side** code, which is
gated on the fidelity budget rather than the state hash, and where a fixed-point
equivalent is a presentation decision the owner can approve. The largest
contributors are `__aeabi_fadd` and `__aeabi_fmul` — add and multiply, not
divide, so Task 50's finding does not cover them.

**The blocker is caller attribution.** The per-PC profiler cannot attribute a
leaf helper to its callers; Task 83 flagged this and it is still true. But the
method now exists and is proven: Task 84's `$lr` sampling failed until Task 85
fixed it by breaking at the **exact entry address** from `nm` rather than at the
symbol (which lands past the prologue). Applied to `__aeabi_fadd` and
`__aeabi_fmul`, that yields a caller histogram in one run.

**Task 92 E0: attribute soft-float to its callers, and split the total into
state-hash-frozen gameplay versus fidelity-gated renderer.** If the renderer
share is large, it is the last unexplored class of its size in the frame. If it
is small, soft-float joins divide/sqrt as closed, and the campaign has no
remaining class above 120,000 that is not stall — which is itself the answer, and
should be reported to the owner as such rather than chased further.

## 5. Two numbers that must not be confused

This census measures the **mean** over 128 frames: work 1,452,992, so the mean is
332,992 over the 1,120,000 figure. `PROJECT_GOAL.md` gates on **P95**, which after
Task 90 is `WORK-H` 1,726,912 — **606,912** over.

They are different quantities and this document quotes both deliberately. A task
that closes 100,000 of mean work does not close 100,000 of P95 unless the work it
removes is present on the P95 frames. Task 70 established the P95 frames are not
the fallback frames, so this cannot be assumed either way.

## 6. Instrument kept

`scripts/task81_partition_census.py` stays. It is cheap (no emulator run — it
reads an existing `census.json`) and it is now the only correct way to size a
class in this codebase. Sizing from the overlapping table, or from per-symbol
totals, has been wrong by 3–15× (Task 83), 1.8× (Task 84 E0) and 8× (Task 91 E0).
