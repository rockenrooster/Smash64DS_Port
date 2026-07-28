# R2-03 E3 — the fighter draw, split, and a correction to E0 and E2

**Date:** 2026-07-28
**Phase:** R2-03 sizing. No runtime change.
**Instrument:** `NDS_TASK91_DRAW_PHASE_CENSUS`, extended with three brackets
(`Total`, `Reset`, `OwnerPrep`) beside the two it already had.
**Result: `ndsFighterMarioFoxDLAllDrawForSlot` costs 494,863 ticks/frame
inclusive, and 113,199 of it is owner preparation.** That single span is 78% of
the frame's remaining 144,844-tick gap.

---

## 1. Correction first

E0 and E2 both quoted `ndsFighterMarioFoxDLAllDrawForSlot` at **37,206
ticks/frame**. That is the symbol census's figure, and the symbol census
measures **self time** — every callee is charged to its own symbol. E2 then
reasoned about "the function's unattributed ~24,000", which is a statement about
a number that was never the function's cost.

Bracketing the whole call says the **inclusive** cost is **494,863 ticks/frame**
— near enough the entire `FTR` bucket (P50 ~547K; the difference is the rest of
the fighter path plus bucket boundaries, which is a good independent check that
the bracket is sound).

E2's measurements of the walk and the revalidation stand: they were bracketed
spans, not census self time, and this run reproduces them to within 1.5% on an
independent build. What was wrong was the denominator they were compared
against, and therefore the conclusion about what mattered.

## 2. The split

Whole one-minute Boundary match, cumulative to frame 1,828, 3,553 draw calls
(1.94/frame — two fighters), `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1
NDS_R2_STAGE_ACTORS=1 NDS_TASK91_DRAW_PHASE_CENSUS=1`.

| phase | per call | per frame | share |
|---|---:|---:|---:|
| Walk — `ndsFighterCollectAllDObjsWithDL` | 1,614 | 3,138 | 0.6% |
| Reset — three `bzero`s, vertex cache, stats | 3,434 | 6,675 | 1.3% |
| Validate — display-list revalidation, MObj chains | 5,102 | 9,916 | 2.0% |
| **OwnerPrep — matrix and material preparation** | **58,240** | **113,199** | **22.9%** |
| Residual — the submit itself, and the tail | 186,214 | 361,936 | 73.1% |
| **Total (inclusive)** | **254,604** | **494,863** | 100% |

`gNdsTask91NativeEligible` is 3,493 of 3,553 (98.3%), so this is the steady-state
shape, not an average over a fallback.

## 3. What OwnerPrep is, and why it is the cut

OwnerPrep is everything between the revalidation passing and the owner inputs
being built: `ndsRendererAdapterPrepareNativeOwnerHierarchy` (or the production
equivalent) and the material preparation. The E0 census's separate symbols that
live inside this span add up the same way —
`ndsRendererAdapterBuildDObjLocalMatrix` 18,998,
`ndsRendererAdapterBuildDObjWorldMatrix` 13,065,
`ndsRendererAdapterPrepareInitialMatrices` 12,480,
`ndsRendererAdapterBuildFighterTraRotRpyExact` 12,336,
`ndsRendererAdapterBuildNativeMaterialSnapshot` 12,434, plus the matrix-kernel
and soft-float time they pull in — which is the cross-check that the bracket is
measuring what its name says.

**This is verbatim what §7 tells R2-03 to delete**: "per-epoch generated submit
consuming only baked facts (`poly_fmt`, texture slot, palette, matrix binding,
corner stream) — no `PrepareProductionRun` policy re-checks, no
traversal-state/stats dependency, no per-frame texture identity proof."

At 113,199 ticks/frame against a 144,844 gap, it is also the first lever in this
campaign large enough to close most of the product gate on its own.

## 4. What this demotes

- **Walk + Reset + Validate = 19,729 ticks/frame**, 4% of the function. E2
  ranked the revalidation stamp first on the strength of a wrong denominator.
  It is still a real ~10,000 and the stamp is still cheap, but it is a tidy-up,
  not the phase. Do it after OwnerPrep, or let the direct path delete it.
- **`Reset` at 6,675** is new information: three `bzero`s per call that the
  symbol census hides inside `memset`'s 38,393 program-wide total. Worth
  knowing, not worth a task.

## 5. Method note

The two brackets E2 used were spans inside the function; the census figure it
compared them to was self time. **Those are different denominators and mixing
them produced a confident wrong ranking.** The standing rules already say
"profile the whole owner before optimising a loop inside it"; this adds the
narrower version — *when you bracket a span, compare it to a bracket, not to a
symbol.* A symbol's self time and a span's inclusive time are not the same
quantity, and the ratio between them here was 13×.

## 6. Evidence

| SHA-256 (first 16) | file |
|---|---|
| `BB495726F4AB05BC` | `artifacts/performance/r2-03-e3-draw-split-1700.json` |

ROM `builds/build-r2-03-e3-phases`. Instrumentation overhead is seven
`cpuGetTiming()` brackets per call, on the order of 700 ticks/frame against
494,863.
