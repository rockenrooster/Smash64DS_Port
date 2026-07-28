# R2-03 E0 — the frame after R2-02, and what the fighter is actually spending

**Date:** 2026-07-28
**Phase:** R2-03 (`Smash64DS_Runtime2_SwitchPlan.md` §7) sizing. No runtime
change.
**Configuration:** `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1
NDS_R2_STAGE_ACTORS=1` — the post-R2-02 program, which is what R2-03 builds on.
128 frames from 438, melonDS-Accurate, corrected symbol attribution
(`task65_subsystem_census.py --nm`).

---

## 1. The frame, re-baselined

The R2-00b baseline is stale: three cuts have landed since.

```text
wall per frame         1,523,013
idle VBlank wait         258,169   16.95% of wall
REAL WORK              1,264,844
30 FPS budget          1,120,000   (PROJECT_GOAL.md)
GAP                      144,844
```

**The gap to the product gate is now 144,844 ticks/frame.** It was 407,000 at
Task 65 and ~327,000 at R2-00b. Of the remaining work, 39.0% is retired
instructions, 39.1% memory stall, 22.0% non-memory stall — so more than half of
what is left is still the machine waiting, not the machine computing.

**By kernel** (tick-HUD units):

| kernel | ticks/frame | %work | cyc/insn | vs R2-00b |
|---|---:|---:|---:|---|
| other | 672,247 | 53.15 | 3.53 | — |
| soft-float | 177,503 | 14.03 | 1.19 | 177,857 (unchanged) |
| matrix | 141,130 | 11.16 | 2.29 | 156,627 |
| gx-submit | 110,800 | 8.76 | 2.61 | 144,852 |
| mem\* | 78,428 | 6.20 | 2.60 | 98,207 |
| texture-resolve | 74,600 | 5.90 | 5.05 | 108,681 |
| rom-read | 10,136 | 0.80 | 2.51 | — |

R2-02 took gx-submit, texture-resolve, matrix and `mem*` down by 34,052, 34,081,
15,497 and 19,779. **Soft-float did not move**, which is right — nothing in
R2-02 touched it — and it is now the largest named kernel in the frame.

**By subsystem:** REND/renderer 431,360 (34.1% of work), REND/adapter 147,492,
LIB/devkitpro 138,490, NDS/other 93,232, PORT/reloc 92,236, SIM/system 84,757,
LIB/mem\* 78,428, SIM/fighter 59,675.

## 2. The fighter, partitioned

`FTR`'s tick-HUD bucket reads P50 ~547K / P95 ~999K against the switch plan's
provisional **250K** combined-fighter budget. The census names where it goes.

**Work the generated program already knows and re-derives anyway** — the
category §7 says R2-03 deletes ("no `PrepareProductionRun` policy re-checks, no
traversal-state/stats dependency, no per-frame texture identity proof"):

| ticks/frame | cyc/insn | function |
|---:|---:|---|
| 37,206 | 5.55 | `ndsFighterMarioFoxDLAllDrawForSlot` — generic DObj tree walk + per-frame display-list revalidation, per fighter |
| 22,467 | 2.90 | `ndsRendererNativePrepareProductionRun` — the policy re-check |
| 18,998 | 3.20 | `ndsRendererAdapterBuildDObjLocalMatrix` |
| 16,380 | 6.92 | `ndsFTParamsInvalidateFighterParts` |
| 13,065 | 3.40 | `ndsRendererAdapterBuildDObjWorldMatrix` |
| 12,480 | 3.33 | `ndsRendererAdapterPrepareInitialMatrices` |
| 12,434 | 3.89 | `ndsRendererAdapterBuildNativeMaterialSnapshot` — the texture identity proof |
| 12,336 | 2.86 | `ndsRendererAdapterBuildFighterTraRotRpyExact` |
| **145,366** | | **rediscovery subtotal** |

That subtotal is **145,366 against a 144,844 gap.** The arithmetic is a
coincidence of two independent numbers, and no cut recovers a whole symbol —
but it is the right order of magnitude, and it is the category the plan already
decided to delete.

**Work that is real drawing:**

| ticks/frame | cyc/insn | function |
|---:|---:|---|
| 48,422 | 2.44 | `ndsRendererNativeShadeProductionActions` |
| 33,516 | 2.58 | `ndsRendererNativeEmitProductionRawUntexturedRun` |
| 31,536 | 4.21 | `ndsRendererExecuteNativeFighterOwnerProduction` |
| 12,431 | 2.65 | `ndsRendererNativeEmitProductionRawTexturedRun` |
| 10,335 | 3.90 | `ndsRendererNativeApplyStateDelta` |

`ndsRendererNativeShadeProductionActions` is the **largest single non-idle,
non-soft-float function in the whole frame**. It re-lights every dense vertex of
both fighters every frame: LUT or per-channel lit shade, material scale,
modulate, pack to RGB15. Whether any of that changes frame to frame is an open
question and the first thing to measure — its inputs are the prepared light
direction (which follows the fighter's root modelview, so probably moves),
`stats->prim_color`, and `state->color_modulate` (which moves on the Task 39
hurt flash).

**Not the fighter, still large:** `__aeabi_fadd` 70,910, `__mulsf3` 50,139,
`memset` 38,393, `gcPlayDObjAnimJoint` 33,955, `ndsRendererMtxMul20p12` 29,649,
`memcpy` 27,100.

## 3. What E3 learned that applies here

**Price the work the fast path does not admit.** For the fighter this does *not*
mean the fallback — Task 70 already showed native-owner fallback is 0.44% of the
P95, and `gNdsTickHudNativeOwnerFallbackByReason[]` carries `Calls` and
`Eligible` denominators to prove the path is taken. The fighter's unadmitted
work is different in shape: the native owner *is* used, but
`ndsFighterMarioFoxDLAllDrawForSlot` walks the generic tree and revalidates
every display list **before** handing over, every frame, per fighter. 37,206
ticks at 5.55 cycles per instruction — the highest stall ratio of any large
function in the frame, which is what a pointer-chasing tree walk looks like.

**An eligibility constant is a claim about the data.** The fighter path is full
of per-frame proofs of things the generator baked: material snapshots, texture
identity, `TraRotRpyExact` rebuilds, local and world matrix rebuilds. Each is a
candidate for the E3 treatment — hash the inputs, count the frames they change
on, and delete the recomputation if the count is zero.
`NDS_R2_STAGE_ACTORS_PROOF` is the pattern to copy.

## 4. Ranked candidates for R2-03 E1

1. **`ndsRendererNativeShadeProductionActions`, 48,422.** Biggest single lever.
   E0 it with an input-hash falsifier before writing anything: if the shade
   inputs are constant across frames the whole loop is a memo; if they move
   every frame, the lever is the per-vertex math, not the recomputation.
2. **`ndsFighterMarioFoxDLAllDrawForSlot`, 37,206 at 5.55 cyc/insn.** The walk
   and revalidation the plan names first. `NDS_TASK91_DRAW_PHASE_CENSUS` already
   splits it into `gNdsTask91WalkTicks` / `gNdsTask91ValidateTicks`; size with
   that before writing.
3. **The adapter matrix rebuild, 56,879** across `BuildDObjLocalMatrix`,
   `BuildDObjWorldMatrix`, `PrepareInitialMatrices` and
   `BuildFighterTraRotRpyExact`. Also where most of the 141,130-tick matrix
   kernel lives.
4. **`ndsRendererNativePrepareProductionRun`, 22,467**, and
   `ndsRendererAdapterBuildNativeMaterialSnapshot`, 12,434 — the two the plan
   names explicitly as deletions.
5. **`gcPlayDObjAnimJoint`, 33,955 self plus ~40,000 of soft-float.** Gameplay
   path, verifier-gated on the Task 37 state hash. Carried over from the
   soft-float E0; unchanged by R2-02.

## 5. Evidence

| file |
|---|
| `artifacts/task37-census-r203e0/arm9-profile.csv` |
| `artifacts/task37-census-r203e0/census.json` |
| `artifacts/task37-census-r203e0/census.txt` |

ROM `builds/build-r2-03-e0-prof`, `NDS_TASK37_PROFILE=1
NDS_TASK37_PROFILE_START=438 NDS_TASK37_PROFILE_FRAMES=128` plus the three
R2-02 flags. `addr2line` renamed 18,443 of 58,679 PCs from the symbol table;
per-function figures here are the corrected ones.
