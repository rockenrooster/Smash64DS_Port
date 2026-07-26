# Task 91 E1 — Task 79's path-to-remove is 13,888 ticks/frame, not ≥100,000

**Date:** 2026-07-26
**Status:** **STOP on plan Task 79 as specified.** Measured on the tick-HUD ROM,
the configuration the P95 gate uses. No runtime change.
**Inputs:** `artifacts/task91-draw-phases.json`,
`scripts/census-fighter-draw-phases.ps1`, `NDS_TASK91_DRAW_PHASE_CENSUS=1`.
**Executes:** `docs/optimization/COMPILER_FIRST_ARCHITECTURE.md` Task 79.

## 1. Result

```
counter            total    per frame
WalkTicks          6,464      3,232.0
ValidateTicks     21,312     10,656.0
DrawCalls              4          2.0
NativeEligible         4          2.0
```

Two draw calls per frame, one per fighter, and the native owner is eligible on
**2 of 2** — the fast path is always taken.

| phase | ticks/frame |
|---|---|
| `ndsFighterCollectAllDObjsWithDL` (generic DObj tree walk) | 3,232 |
| native-owner revalidation loop + `ValidateNativeOwnerCached` | 10,656 |
| **total Task 79 deletes** | **13,888** |

**Task 79's target is ≥100,000. The path it names costs 13,888.** That is a
factor of 7.2 short, and it is below the plan's own "~20K–30K simple exact
change" band. Against a measured ±8,000 placement noise floor it is 1.7×, which
is the band Tasks 87, 88 and 89 all regressed in.

## 2. My Task 91 E0 estimate was wrong by ~8×, and why

E0 classified census symbols into removable and retained and produced
"~100,000–125,000". The error was mechanical: **`ndsFighterMarioFoxDLAllDrawForSlot`
is 36,680 ticks/frame for the whole function**, and I attributed it to the walk
and validation because those are what the function opens with. In fact
13,888 of that is walk plus validation and the rest is native production
dispatch, matrix preparation and GX work — the part the plan's own target path
says the runtime keeps. The same mistake inflated `ftGetStruct`,
`ndsRelocFindLoadedFileContaining` and the `DisplayContract*` family, none of
which run wholly inside the deleted phases.

This is the third time this campaign has produced a wrong size from symbol
totals without counting what actually executes in the phase:

- Task 83 sized `mem*` from instruction counts — wrong by 3–15×.
- Task 84 E0 priced cold-buffer bytes at the frame average — wrong by 1.8×.
- Task 91 E0 attributed a whole function to its first two phases — wrong by 8×.

E0 flagged its own number as "a classification over call sites, not a
measurement" and said implementation must not start on it. That caveat was the
only thing that made the estimate safe to publish, and it is why this task cost
one build and one run rather than a reverted subsystem change.

## 3. What this does and does not close

**Closed: Task 79 as written.** "Stop translating native fighter state back into
generic BattleShip-shaped render structures" is worth 13,888 ticks/frame on the
Boundary configuration. Do not build the generated render-program dispatch to
recover it.

**Not closed:** the rest of the fighter draw. `FTR` P50 is 543,552 after Task 90
and the walk plus validation is 2.6% of it. The other 97% is native production,
shading, matrix preparation and GX submission — work the generated program still
performs, and which a compiler-first rewrite therefore does not remove. That is
the substantive finding: **the fighter path is already native where it matters,
and the generic scaffolding around it is thin.**

**Also measured, worth recording:** the native owner is eligible on every draw
call. There is no fallback cost hiding here, consistent with Task 70's finding
that fallback is 0.44% of the P95.

## 4. Consequence for the campaign

`COMPILER_FIRST_ARCHITECTURE.md`'s premise is that generic runtime abstraction
dominates the frame and that removing it is worth ≥100K per task. For the
fighter draw that premise is now measured and false — Task 77 E0 found the
generated tables already exist, Task 78 E0 found animation at 82,807 against a
≥100K target, and Task 91 E1 finds the render-program lever at 13,888 against
the same target. Three consecutive plan tasks have come in under their gate on
measurement.

The plan is amended in-file rather than replaced (§5). What survives is its
engineering rule — ask whether an abstraction needs to exist — and what does not
survive is its sizing: the abstractions it names have already been removed by
Tasks 36, 53, 54 and the native-owner work, and what is left is real rendering.

`WORK-H` P95 is 1,726,912 against a 1,120,000 gate. The remaining **606,912** is
not in scaffolding. Any task that proposes to close it should now be required to
name the measured phase it deletes, on the tick-HUD ROM, before it is scheduled.

## 5. Plan amendment

`COMPILER_FIRST_ARCHITECTURE.md` gains an "Amended by Task 91 E1" section
recording the 13,888 measurement and marking Task 79 STOP, in the same form as
its existing Task 77 E0 and Task 78 E0 amendments.

## 6. Instrument kept

`scripts/census-fighter-draw-phases.ps1` and `NDS_TASK91_DRAW_PHASE_CENSUS`
(default 0) stay. They are the first instrument in this campaign that reports a
fighter-draw phase split on the tick-HUD ROM — the M2 ledger cannot, being
restricted to profile level 1 by `nds_renderer.h:39` with its only target
overriding `FAST_RUN_DEFAULT`. Any future task proposing to delete a fighter
draw phase should extend these counters to that phase and measure it first.
