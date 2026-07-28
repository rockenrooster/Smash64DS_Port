# R2-03 E4 — fighter matrix preparation is 91,338 ticks/frame

**Date:** 2026-07-28
**Phase:** R2-03 sizing, final split. No runtime change.
**Instrument:** `NDS_TASK91_DRAW_PHASE_CENSUS`, two more brackets mirroring the
profile-level-1 matrix/material split the tick-HUD target cannot compile.
**Result: the 113,855-tick owner-preparation span is 80% matrix.
`MatrixPrep` = 91,338 ticks/frame — 63% of the frame's 144,844-tick gap and the
largest single mechanism in the frame after the idle wait.**

---

## 1. The split

Whole one-minute Boundary match, cumulative to frame 1,828, 3,553 draw calls,
`NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1 NDS_R2_STAGE_ACTORS=1
NDS_TASK91_DRAW_PHASE_CENSUS=1`.

| phase | per call | per frame | share of OwnerPrep |
|---|---:|---:|---:|
| **MatrixPrep** | **46,993** | **91,338** | **80.2%** |
| MaterialPrep | 11,063 | 21,504 | 18.9% |
| unsplit remainder | 521 | 1,014 | 0.9% |
| OwnerPrep | 58,578 | 113,855 | 100% |
| Total (inclusive) | 255,823 | 497,231 | — |

`OwnerPrep` and `Total` reproduce E3's 113,199 and 494,863 to within 0.6% and
0.5% on an independent build — two runs, two binaries, agreeing.

The split already existed as `gNdsRendererProfileMatrixTicks` /
`...MaterialTicks` at profile level 1, and the tick-HUD target overrides profile
level to 0. That is the same restriction Task 91 was created to work around, and
this is the same fix: mirror the brackets under the lab flag that the Boundary
configuration can actually compile.

## 2. Where the frame stands

```text
REAL WORK              1,264,844
30 FPS budget          1,120,000
GAP                      144,844

fighter MatrixPrep        91,338   63% of the gap
fighter MaterialPrep      21,504   15%
stage layer1              22,738   16%   (R2-02 leftover, generator work)
```

Three named mechanisms cover 93% of the remaining gap, and one of them is most
of it.

`MatrixPrep` is `ndsRendererAdapterPrepareNativeOwnerHierarchy` (or its
production equivalent) — the per-frame construction of the fighter's joint
matrices. The E0 census's constituent symbols add up the same way:
`ndsRendererAdapterBuildDObjLocalMatrix` 18,998,
`ndsRendererAdapterBuildDObjWorldMatrix` 13,065,
`ndsRendererAdapterPrepareInitialMatrices` 12,480,
`ndsRendererAdapterBuildFighterTraRotRpyExact` 12,336,
`ndsRendererAdapterFindDObjWorldMatrix` 1,442, plus the matrix kernel
(`MtxMul20p12` 29,649, `MtxMulAffine20p12` 16,861, `MtxLoadN64ToDS20p12`
13,747) and the soft-float those pull in.

**That last part explains the frame's largest kernel.** Soft-float is 177,503
ticks/frame and did not move through all of R2-02, and `__aeabi_fadd` alone is
70,910. The source's joint transforms are float TRA/ROT/RPY triples;
`BuildFighterTraRotRpyExact` and `BuildDObjLocalMatrix` are what turn them into
DS 20.12 matrices, and they are render-side, not gameplay. Soft-float is not an
independent problem from `MatrixPrep`; it is largely the same ticks counted a
different way.

## 3. What the cut is, and what it is not

**It is not a memo.** The pose changes every frame, so unlike R2-02 E3's actor
segments there is no constant here to stop recomputing. The falsifier is not
needed for the matrix half and would return the same answer R2-03 E1 got.

**It is what §7 already specifies:** "per-epoch generated submit consuming only
baked facts (`poly_fmt`, texture slot, palette, **matrix binding**, corner
stream)". The generated program knows the joint tree, the binding assignment,
and which joints move; the runtime rediscovers the tree, rebuilds each local
matrix from floats, composes the world matrices, and re-derives the bindings,
every frame, for both fighters. The cut is to generate the schedule and evaluate
it in fixed point — which removes the float→fixed conversion that is feeding
`__aeabi_fadd`, and removes the traversal that is feeding
`FindDObjWorldMatrix`.

**MaterialPrep, 21,504, is a different shape and probably is a memo.** It is the
per-frame material snapshot and texture identity proof — `§7`'s "no per-frame
texture identity proof" — and its inputs are the loaded asset and the material
state, not the pose. That one deserves the E3 falsifier
(`NDS_R2_STAGE_ACTORS_PROOF` is the pattern) before anything is written, and if
it comes back constant it is a much cheaper 21,504 than the matrix work.

**Recommended order for R2-03 proper:**

1. **MaterialPrep falsifier** — cheapest possible information, and a clean
   ~21,500 if the hash does not move. One build, one run.
2. **MatrixPrep, the generated fixed-point joint schedule** — 91,338, the phase
   the plan describes, and the only remaining lever big enough to close the
   gate. Gate it as §7 says: pixel parity on the same pose (Task 49 GX differ +
   screenshot), Boundary green on the state hash, and the provisional 250K
   combined-fighter budget.
3. layer1 (stage segment 4), 22,738 — R2-02's leftover, generator work.

## 4. Evidence

| SHA-256 (first 16) | file |
|---|---|
| `53FA04655F0D1414` | `artifacts/performance/r2-03-e4-ownerprep-split-1700.json` |

ROM `builds/build-r2-03-e4-phases`.
