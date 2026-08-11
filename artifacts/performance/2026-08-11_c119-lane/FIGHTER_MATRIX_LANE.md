# The 20.12 matrix lane belongs to the FIGHTERS, not the stage

Cycle 119, second attribution pass. `task37_census.py --split-top-frames 80
--exclude-regions 1558 --attribute-leaves` over the same
`build-c119-profile-gate` CSV, this time with the matrix builders themselves as
leaves. No extra run. Raw output: `stageworld-attribution.txt`,
`xobjmatrix-pcdetail.txt`.

## The correction

`LEAF_ATTRIBUTION.md` grouped 62,891 tk/frame as "20.12 matrix kernels —
renderer" and the board carried it as a stage lane. It is not. Attributing the
builders to their callers puts **35,752 of it in one function**:

| cyc/frame | tk/frame | tk prem | calls/hot | calls/ctl | frames | leaf | caller |
|---:|---:|---:|---:|---:|---|---|---|
| 37,173 | **18,560** | 957 | 52.5 | 49.8 | 80/80 | `ndsRendererMtxMulAffine20p12` | `ndsFighterMarioFoxDLAllDrawForSlot` |
| 24,500 | **12,233** | 685 | 57.5 | 54.3 | 80/80 | `…BuildDObjXObjMatrix` | `…BuildDObjLocalMatrix` |
| 9,931 | **4,959** | 226 | 55.8 | 53.3 | 80/80 | `…BuildDObjLocalMatrix` | `ndsFighterMarioFoxDLAllDrawForSlot` |
| 18,727 | 9,350 | **0** | 16.0 | 16.0 | 80/80 | `…BuildPersistentStageWorldMatrix` | `…PrepareNativeStageOwner` |
| 8,052 | 4,020 | 9 | 28.1 | 28.0 | 80/80 | `…StageWorldSourceKeyMatches` | `…BuildPersistentStageWorldMatrix` |
| 7,463 | 3,726 | −0 | 26.0 | 26.0 | 80/80 | `…StageWorldSourceKeyMatches` | `…PrepareNativeStageOwner` |
| 6,192 | 3,092 | 26 | 68.9 | 68.3 | 68.9 | `…FindStageWorldEntry` | `…BuildPersistentStageWorldMatrix` |

Every leaf reconciles at **100.0%**. Only **1.5** of the 54.2 affine multiplies
come from the stage. Mario has 25 joints and Fox 27; 52 joints is the whole of
the 52.5 composes and 55.8 local builds. **The lane is two fighters' joint
hierarchies, evaluated from scratch every frame.**

`tk prem` is ~1,900 across the three rows, so this is FLAT work: a deletion moves
P50 and P95 together, one for one. That is the good case — no clustering
discount ([[cluster-where-the-percentile-lives]]).

## Every non-hardware lever on it is already dead

- **The arithmetic.** Slice 42 row-blocked both kernels, bit-exact, and measured
  −2,368 / −5,184 / +6,912 across four route values. Sub-floor and
  self-contradictory. `artifacts/performance/2026-08-11_mtx-route/FINDING.md`.
- **The instructions.** `--pc-detail` on `BuildDObjXObjMatrix`: 36,155,480 cycles
  over **324 distinct PCs**, top PC 6.8%. The four costliest rows are loads at
  **28.7, 29.2, 16.8 and 13.3 cyc/insn** — 21% of the function is D-cache misses
  on scattered `DObj`/`FTParts` fields, not arithmetic. There is no instruction
  to delete; the only lever is the call count
  ([[flat-function-only-lever-is-not-entering-it]]).
- **The local-matrix memo.** R2-03 E8 refuted it at **+16,301**: 97.5% of builds
  are the fighter-parts kind, so the only complete key folds 16 floats of
  `FTParts`, and 71% of calls paid the key and built anyway. **It is now dead
  twice over**, and the second reason is new: E8 priced the payload at E6's
  1,061 tk/build, but `NDS_R2_FIGHTER_MTX_DIRECT` graduated after that and today
  the payload is **302 tk/call** (`BuildDObjLocalMatrix` 89 + `BuildDObjXObjMatrix`
  213). At E8's measured 13.6 hits/frame the gross saving is now ~4,100 tk —
  under the floor before the key costs anything. **Do not revive it, even with a
  cheap write-site generation key.**
- **`NDS_R2_FIGHTER_MTX_DIRECT`.** Already `override … := 1` in both published
  targets (Makefile 1397, 1732). Level 2 is only the rebuild-and-compare
  verifier, not a further optimization. Nothing dormant here.

## What is left, and it is the only thing left

**The 52 affine composes and the 32 matrix loads exist solely to hand a matrix
to the GX.** `ndsRendererLoadHardwareSplitMatrices` is
`SetMatrixMode(GL_MODELVIEW); LoadMatrix4x4(modelview)` and nothing else, and the
fighter vertices are submitted in MODEL space ("vertices arrive at world x16",
nds_renderer.c:13593) — the geometry engine already does the vertex transform.
No CPU consumer reads a composed fighter world.

| per frame | now | on the GX matrix stack |
|---|---:|---:|
| CPU affine composes | 52.5 × 708 cyc = 37,170 | 0 |
| `LoadHardwareSplitMatrices` | 31.8 × ~700 cyc = 22,340 | projection loaded once |
| GX commands | 32 × 32 words | ~52 `MTX_MULT_4x3` + ~32 PUSH/POP ≈ 740 words |
| **total** | **~59,510 cyc = 29,732 tk** | **~4,000 cyc ≈ 2,000 tk** |

`binding_parents[i] < i` is already a preorder table
(`ndsRendererAdapterComposeOwnerWorldsFlat`), so a DFS over it drives PUSH/POP
directly. The DS `MTX_MULT` computes `current = M × current`, which is exactly
the `MtxMulAffine20p12(&local, out, out)` convention already in the loop — no
reassociation.

**The translate shift transfers.** `LoadHardwareSplitMatrices` scales `m[3][*]`
by `NDS_RENDERER_HW_WORLD_UNIT_SHIFT` at FIFO-write time. Scaling the
translation row of *every* matrix in a chain scales the composed translation by
the same factor and leaves the linear part untouched, so pre-scaling each local
and the camera seed reproduces it.

**Not bit-exact, and it does not need to be.** The GX rounds its own way. These
matrices reach GX and nothing else, which is the same doctrine
`ComposeOwnerWorldsFlat` already relies on (reloc_backend_renderer_dl.c:3526)
and which `PROJECT_GOAL.md` grants render-side. The Boundary visual gate is the
check.

Predicted **~27,000 tk/frame**, flat, against a ±8,544 cross-build floor.
