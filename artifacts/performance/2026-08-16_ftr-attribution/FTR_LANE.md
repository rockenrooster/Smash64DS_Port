# FTR — the run's largest lane, attributed per PC in the shipping configuration

**Date:** 2026-08-16
**Basis for every gate figure:** `build-c220-camship`,
`../2026-08-16_camera-ship/ship220-rows.csv`, whole match, 1,600 samples, frames
439–2038, gate arm `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 KEEP_CACHE=1`, mode
163, DLDI ON, `slips=0`. Apparatus 24,947, gate 1,120,380, rank-80 **1,210,624
raw / 1,185,677 net**, band 41–120 **1,218,356**, **REQUIREMENT +65,297**.
**Attribution capture:** `../2026-08-16_sitr-excursion/v3-c221/arm9-profile.csv`
(`build-c221-sitrprof`, `GX_COMPOSE 0`, `FTANIM_TRACK 0`, `CAMERA_FIXED 1`;
`config-diff.txt` is four lines and all four are the profiler or the git string),
54,400,815 PC rows, 1,601 regions.
**UNITS: 2 profile cycles = 1 project tick.**

**Status: census only. 0 lab builds, 0 emulator runs of my own, 0 production
source edits, 0 defaults flipped, nothing published, nothing re-banked.**
**Boundary PASSED, exit 0, 0 `Exception:` over 316,172 lines**
(`boundary.trimmed.log`); its pacing smoke is exact against the previous five
runs — `ticks=294353408` identical to the tick, `ftrTri=132712/p067840/p164872/
own424`, `itcm=30164/32768 free=2604`, `renderer=12896`. Both root ROMs SHA-256
**identical before and after** Boundary's own rebuild, `54C07FAC…` / `6C939434…`
(`root-roms-before.txt` / `root-roms-after.txt`).

---

## 0. The answer in one table

`FTR` is not a symbol set. It is the dynamic extent of **one function**:
`ndsFighterDisplayContractSubmit` opens the span at
`src/port/reloc_backend_renderer_dl.c:16863` (`owner_start = cpuGetTiming()`,
the `bl` at `0x0205eef0`) and closes it at `:16878` / `:16937`. A per-PC profiler
charges every leaf to itself and never to the span it ran inside, which is why
this lane had never been attributed. It is reconstructed here from **call flow**
(`attribute.py`; the model and its single assumption are in that file's
docstring).

**THE MODEL'S TOTAL IS A CHECK THAT COULD HAVE FAILED, AND IT PASSED:**

| population | model | the instrument's own FTR |
|---|---:|---:|
| marginal-80, the gate's own top-80 frames | **291,051** | band 41–120 **290,400** |
| marginal-80, the profile's own top-80 by non-idle | **290,958** | P50 **290,432** |
| whole match, 1,601 regions | 277,192 | — |

0.2% against a number the script never reads, from a **different build**, by a
method that never sees the bucket. `sum of per-symbol self == root inclusive`
holds to one tick. Direct call sites reconcile against entry-PC counts at
**100%** for every top symbol and **92.3%** across all 128 symbols the flow
reaches — the shortfall is register-indirect dispatch, and none of it is in the
rows below.

The two marginal-80 masks **share only 25 of 80 frames** (the tail permutes
across builds — `../2026-08-16_match-io-audit/IO_AUDIT.md` A2 recorded exactly
this) and still agree to **0.03%**. That is the flatness of this lane proven from
the profile itself, independent of the tick HUD.

---

## 1. What owns the 291,051, by symbol (marginal-80, `m80gate`)

Full tables: `ftr-m80gate.txt`, `ftr-m80prof.txt`, `ftr-whole.txt`.
`FTRc/f` = invocations inside the span per frame. `share` = the fraction of that
symbol's total calls the span owns.

| self tk/fr | incl tk/fr | FTRc/f | bytes | cyc/in | share | symbol |
|---:|---:|---:|---:|---:|---:|---|
| 30,616 | 241,951 | 2.00 | 7,544 | 4.40 | 100% | `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` |
| 26,228 | 26,228 | 54.00 | 548 | 3.06 | 100% | `ndsRendererNativeEmitProductionPrimitiveGroups` |
| 22,201 | 135,075 | 2.00 | 3,472 | 3.04 | 100% | `ndsRendererExecuteNativeFighterOwnerProduction` |
| 18,549 | 18,549 | 52.94 | 616 | 1.83 | 97% | `ndsRendererMtxMulAffine20p12` |
| 18,454 | 32,069 | 67.00 | 3,720 | 2.28 | 100% | `ndsRendererNativePrepareProductionRun` |
| 13,295 | 36,922 | 56.14 | 2,440 | 2.01 | 97% | `ndsRendererAdapterBuildDObjXObjMatrix` |
| 11,159 | 11,159 | 29.46 | 2,056 | 2.55 | 97% | `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` |
| 10,800 | 10,800 | 72.20 | 170 | 3.07 | 42% | `memcpy` |
| 10,614 | 12,025 | 32.00 | 392 | 3.89 | 100% | `ndsRendererLoadHardwareSplitMatrices` |
| 9,988 | 19,300 | 52.19 | 388 | 4.74 | 100% | `ftDisplayMainDrawDefault` |
| 6,327 | 18,658 | 196.00 | 492 | 3.15 | 100% | `ndsRendererNativeApplyStateDelta` |
| 5,881 | 9,516 | 49.00 | 580 | 2.72 | 100% | `ndsRendererNativeShadeProductionActions` |
| 5,202 | 5,214 | 30.00 | 1,656 | 3.95 | 100% | `ndsRendererNativeApplyMaterial.part.0` |
| 5,096 | 42,017 | 56.16 | 206 | 3.70 | 97% | `ndsRendererAdapterBuildDObjLocalMatrix` |
| 4,723 | 4,723 | 13.00 | 576 | 2.53 | 100% | `ndsRendererNativeEmitProductionCrossRun` |
| 4,154 | 4,154 | 72.15 | 248 | 3.76 | 99% | `ndsRendererSyncTextureTile` |
| 4,117 | 4,117 | 54.19 | 56 | 8.95 | 100% | `ndsFighterDisplayContractCountFlags` |
| 3,498 | 3,498 | 32.00 | 304 | 2.16 | 100% | `ndsFighterDisplayContractSelectDL` |
| 3,473 | 3,473 | 30.04 | 132 | 1.73 | 100% | `ndsRendererAdapterMaterialAnimHash` |
| 3,348 | 30,190 | 2.00 | 2,276 | 12.41 | 100% | `ndsBaseFTDisplayMainProcDisplay` |
| 3,346 | 291,051 | 2.00 | 676 | 11.46 | 50% | `ndsFighterDisplayContractSubmit` (root) |

### The tree, with the builders rolled up

```
ndsFighterDisplayContractSubmit                            291,051   2.00/fr   [the span]
├─ ndsBaseFTDisplayMainProcDisplay          CAPTURE         30,190   2.00/fr
│   └─ ftDisplayMainDrawAll → ftDisplayMainDrawDefault      19,300  52.19/fr   (self-recursive, r=0.96)
├─ gmCameraLookAtFuncMatrix                 CAPTURE         10,285   2.00/fr
├─ ndsFighterDisplayContractCountFlags      CAPTURE          4,117  54.19/fr   (self-recursive)
└─ ndsFighterMarioFoxDLAllDrawForSlot       PLAYBACK       241,951   2.00/fr
    ├─ ndsRendererAdapterBuildDObjLocalMatrix               42,017  56.16/fr   27.94 per fighter
    │   └─ ndsRendererAdapterBuildDObjXObjMatrix            36,922  56.14/fr
    │       └─ ...BuildFighterTraRotRpyDirect20p12          11,159  29.46/fr
    ├─ ndsRendererMtxMulAffine20p12                         18,549  52.94/fr   26.10 per fighter
    ├─ ndsRendererAdapterMaterialAnimHash                    3,473  30.04/fr
    └─ ndsRendererExecuteNativeFighterOwnerProduction      135,075   2.00/fr
        ├─ ndsRendererNativePrepareProductionRun            32,069  67.00/fr
        ├─ ndsRendererNativeEmitProductionPrimitiveGroups   26,228  54.00/fr
        ├─ ndsRendererNativeApplyStateDelta                 18,658 196.00/fr
        ├─ ndsRendererLoadHardwareSplitMatrices             12,025  32.00/fr
        ├─ ndsRendererNativeShadeProductionActions           9,516  49.00/fr
        ├─ ndsRendererNativeApplyMaterial.part.0             5,214  30.00/fr
        └─ ndsRendererNativeEmitProductionCrossRun           4,723  13.00/fr
```

**The whole lane is per-DObj work at ~28 DObjs per fighter.** Eleven of the
twelve largest rows run 52–72 times a frame, i.e. 26–36 per fighter, and the
call rates agree across unrelated subsystems (`BuildDObjLocalMatrix` 56.16,
`BuildDObjXObjMatrix` 56.14, `EmitProductionPrimitiveGroups` 54.00,
`CountFlags` 54.19, `ftDisplayMainDrawDefault` 52.19, `MtxMulAffine20p12`
52.94). **`ndsFighterDisplayContractSubmit` is entered 4.14 times a frame but
only 2.00 reach the timer** — the other 2.14 early-return above it, so exactly
two fighter draws are in the bucket.

### By phase

| phase | tk/fr | share |
|---|---:|---:|
| NATIVE PRODUCTION emit (build and push the GX stream) | 71,448 | 24.5% |
| PER-JOINT MATRIX build (local → XObj → affine compose) | 61,848 | 21.3% |
| the two walker bodies' own instructions | 52,817 | 18.1% |
| MATERIAL and TEXTURE | 29,539 | 10.1% |
| CAPTURE pass (walk the DObj tree, record the draw contract) | 24,919 | 8.6% |
| MEMORY movers | 13,368 | 4.6% |
| CAMERA | 10,758 | 3.7% |
| SOFT FLOAT leaves | 4,038 | **1.4%** |

**The float→fixed class is 1.4% of the largest lane in the run.** It stays
closed, and this is the first number that sizes it inside FTR specifically.

---

## 2. THE FINDING: the lane is 59.3% cache fill and only 21.6% issue

The profiler's seven classes are a **complete partition** — verified per symbol
on all 333 symbols over 1M cycles, `sum(classes) == total_cycles` at ratio
1.0000 (the 0.71% run-level residue is `dma_hold`, which this scan did not
carry). So these shares are of the lane, not of some sub-total. `issue` is the
**residual** class and reads slightly negative on a symbol whose fetch stall
overlaps issue (−2.4% on `MtxMulAffine20p12`); `icache_fill` is a directly
measured counter.

The first two columns are the marginal-80 (`m80gate`). The third is computed
**entirely on the whole mask** — FTR's whole-match figure over the run's own
whole-match total from `arm9-profile.meta.txt` — because a lane share against a
run total must not mix populations.

| class | FTR tk/fr (m80gate) | share of FTR | FTR whole / run whole |
|---|---:|---:|---:|
| **icache_fill** | **88,486** | **30.4%** | **85,457 / 355,292 = 24.1%** |
| dcache_fill | 84,164 | 28.9% | 80,747 / 271,192 = 29.8% |
| issue (residual) | 62,929 | 21.6% | — |
| bus_contention | 20,896 | 7.2% | **19,992 / 37,978 = 52.6%** |
| write_buffer | 19,445 | 6.7% | — |
| interlock | 15,131 | 5.2% | — |

**The largest lane in the run spends more on instruction fetch than on issuing
instructions.** This is the campaign's own recurring shape — *the compare was
never the cost* — arriving in the draw: the fighter draw's hot bodies alone are
`MarioFoxDLAllDrawForSlot` 7,544 + `NativePrepareProductionRun` 3,720 +
`ExecuteNativeFighterOwnerProduction` 3,472 + `BuildDObjXObjMatrix` 2,440 +
`ndsBaseFTDisplayMainProcDisplay` 2,276 + `BuildFighterTraRotRpyDirect` 2,056 =
**21,508 bytes against an 8 KB I-cache**, re-walked twice a frame.

`ndsRendererMtxMulAffine20p12` is the extreme case: **616 bytes, 52.94 calls a
frame, 72.9% instruction fetch**, which is 512 cycles of fill per call over 20
cache lines = **25.6 cycles a line — the whole function re-fetched from main RAM
on essentially every call.**

**FTR holds 52.6% of the entire run's GX-FIFO stall** (`bus_contention`), which
is where the "submit less geometry" lever lives. That is a fidelity question and
no owner decision is asked for here.

---

## 3. REFUSED, SIZED: cold-path out-of-lining does not convert in this lane

`../2026-08-14_hot-footprint/HOT_FOOTPRINT.md` sized footprint reduction
whole-match (ceiling ~81,800 tk/fr, realistic 25,000–40,000) and left it as
*"hand work per function … must be sized against the ≥16,000 tk/fr floor before
it is started"*, after `-freorder-blocks-and-partition` was refuted on this
toolchain. **It has now been sized per function inside the one lane where a
uniform cut converts 1:1** (`density.py`, HOT_FOOTPRINT's method with its
literal-pool correction, pools resolved from objdump's own `@ (addr <sym+off>)`
annotation):

```
FTR pays  42,080 B in 1,315 lines
   live   34,306 B  81.5%   <- this is the work
   pool      644 B   1.5%   <- Thumb-1 cannot inline a 32-bit constant; NOT removable
   cold    7,130 B  16.9%   <- fetched, never executed
PERFECT-COMPACTION CEILING: 9,714 tk/fr = 11.0% of the lane's fetch
```

**FTR's fetched code is 81.5% live and its cold bytes are scattered**, so
compaction removes only 11.0% of the fill even at a ceiling that basic-block
granularity cannot reach. The two densest large bodies are already at the wall:
`BuildFighterTraRotRpyDirect20p12` needs 63 of its 65 paid lines (3.1%
recoverable) and `MtxMulAffine20p12` does not appear in the top 26 at all.

**9,714 is 14.9% of the requirement at an unachievable ceiling; a realistic
third-to-half is 3,200–4,900, which is 23–35% of the ≥14,080 cross-build floor.
Do not start hand out-of-lining for the FTR lane.** (Nothing here speaks to the
other 267,000 tk/fr of whole-match instruction fetch outside FTR.)

## 3b. REFUSED, SIZED: ITCM cannot buy enough of this lane either

A body in ITCM pays no fetch. The bound below is **not** the `icache_fill`
figure — an instruction still retires — it is
`cycles − (instructions + dcache + write_buffer + bus + interlock)`, the cost the
symbol sheds if fetch is free and nothing else changes.

| fetch-free tk/fr | bytes | tk/fr per byte | symbol | fits? |
|---:|---:|---:|---|---|
| 7,380 | 7,544 | 0.98 | `ndsFighterMarioFoxDLAllDrawForSlot` | no |
| 3,799 | 2,056 | 1.85 | `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` | no |
| **2,989** | **616** | **4.85** | **`ndsRendererMtxMulAffine20p12`** | **yes** |
| 2,277 | 2,440 | 0.93 | `ndsRendererAdapterBuildDObjXObjMatrix` | no |
| 1,896 | 388 | 4.89 | `ftDisplayMainDrawDefault` | yes |
| 1,335 | 392 | 3.41 | `ndsRendererLoadHardwareSplitMatrices` | yes |

`../2026-08-16_itcm-census/ITCM_CENSUS.md` puts the recoverable ITCM pool at
**688 B by eviction (+54)**, on top of 220 B free on the instrument — call it
908 B. The best tenants that fit buy **~3,000–4,000 tk/fr**, i.e. **4.6–6.1% of
the requirement**, all under the cross-build floor.

Cross-check, independent of my bound: `../2026-08-16_anim-itcm/ANIM_ITCM.md`
actually moved `ndsR2AnimValueQ` (1,028 B, 21.13 tk/fr per byte) and banked a
paired median of **−3,840**, i.e. 17.7% of that function's total cost.
`MtxMulAffine20p12` costs 18,549 tk/fr at **30.1 tk/fr per byte** — higher rent —
and 17.7% of it is **3,283**. Two unrelated estimates land at ~3,000–3,300.

**Not built.** It is real, it is fidelity-neutral, and a same-binary `.data`
route would price it with **zero placement floor** — but at ~3,000 it is a
rider, not a cycle.

---

## 4. The conversion curve, and why it is 1.000 everywhere

`convert-ftr.py` re-sorts the c220 basis's own 1,600 rows with a uniform D on
every frame, capped at that frame's own FTR. **All 1,600 frames are touched; 80
sit at or above rank-80 by definition, and no sub-population can sink below the
rank and stop paying** — which is exactly what made the `SITR` lane's conversion
collapse from 0.860 to 0.244.

```
D=1,000    moved   1,000  ratio 1.000  level +64,297
D=2,989    moved   2,989  ratio 1.000  level +62,308
D=5,143    moved   5,143  ratio 1.000  level +60,154
D=9,714    moved   9,714  ratio 1.000  level +55,583
D=14,080   moved  14,080  ratio 1.000  level +51,217
D=44,600   moved  44,600  ratio 1.000  level +20,697
D=65,297   moved  65,297  ratio 1.000  level      0
D=120,000  moved 120,000  ratio 1.000  level −54,703
```

The ratio never leaves 1.000 below ~120,000 because the lane is flat. Flatness
holds **per symbol, not just per lane**: the marginal-80 / whole-match ratio is
**1.00–1.09 on all twenty largest rows** (`ftr-concentration.txt`; only `memcpy`
1.28 and `BuildDObjXObjMatrix` 1.23 exceed it). *A flat lane is the
best-converting lane* — and there is correspondingly **no excursion inside FTR
to attack**. `lanes.txt`'s "FTR 22% closes the gate" is a 1% rounding: 22% =
63,895 leaves **+1,402**.

---

## 5. The only item in this lane that clears the floor — SPECIFIED, NOT BUILT

**The capture pass re-derives the fighter's draw contract from BattleShip's own
display code every frame, for both fighters, and it costs 34,307 tk/fr.**

`ndsFighterDisplayContractCapture` (`reloc_backend_renderer_dl.c:13809-13869`,
inlined into the root) redirects `gSYTaskmanDLHeads` to a scratch arena, sets
`active = TRUE`, and runs the source display chain so the DL-emitting shims
**record** events instead of drawing. The playback pass then walks the same
~28-DObj tree a second time in `ndsFighterMarioFoxDLAllDrawForSlot`. The
contract is the oracle for which DL, material, render mode and flags each DObj
wants.

| item | tk/fr | level if free | mechanism |
|---|---:|---:|---|
| `ndsBaseFTDisplayMainProcDisplay` (the DObj walk) | 30,190 | +35,107 | memoise the event list |
| `ndsFighterDisplayContractCountFlags` | 4,117 | | same key |
| **subtotal, contract memo ceiling** | **34,307** | **+30,990** | **52.5% of the requirement** |
| `gmCameraLookAtFuncMatrix`, second call per frame | 5,143 | +60,154 | see below |

**The camera half is small, and its mechanism is read from the source rather
than assumed.** `gmCameraLookAtFuncMatrix` is called **twice a frame, once per
fighter**, with `NULL` as the out-matrix; the call site (`:13856-13867`) wants
only its side effects on the camera globals, which are **camera state and do not
depend on which fighter is being captured**. On the shipping arm
(`NDS_R2_CAMERA_FIXED = 1`, owner-accepted 2026-08-16) the port wrapper
`src/import/battleship_gmcamera.c:823-832` **early-returns through
`ndsCameraLookAtFuncMatrixFixed(mtx, cobj, level)`, which is not passed `dls` at
all** — so the scratch DL-head redirection two lines above it is irrelevant to
this call, and its only inputs are the camera `CObj` and a build constant.

**What is NOT proven, and it is the whole question:** fighter 0's *playback*
runs between the two capture calls, so "the two calls have identical inputs"
needs the camera `CObj` and the published globals to be untouched by a draw.
The discriminator is one counter — hash the camera globals after each call and
count mismatches over the window. **5,143 tk/fr, level +60,154** — under the
floor, correct only as a rider on something larger.

**The memo half is a ceiling, not a price, and I did not measure the thing that
decides it: how often the contract actually changes.** There is no counter for
it. *A memo is a memory stream* — the scratch arena is 6,240 bytes and a memo
must key, validate and replay it, so its net is strictly below 34,307. The
contract is small and fixed-size in practice: this cycle's own Boundary reads
**`ftrContract=6784/6784` over 212 frames = exactly 32.0 events per frame, 16
per fighter**, which agrees with `ndsRendererLoadHardwareSplitMatrices` 32.00
and `ndsRendererNativeApplyProductionPreamble` 32.00 measured independently in
the profile. A 16-entry key is cheap; that is what makes the memo worth pricing
rather than dismissing.

The cheapest discriminating measurement, and the one the next cycle should take
first, is a **hash of `sNdsFighterDisplayContract.events[0..event_count)` per
fighter per frame, counted as changed / unchanged over the 1,600-frame window**
— one counter, one lab build, no gameplay change, and it either sizes the memo
or kills it outright. **Predict the count before the run.**

---

## 6. Two model errors, both caught in-cycle before anything was quoted

1. **Recursion inflation.** `ftDisplayMainDrawDefault` and
   `ndsFighterDisplayContractCountFlags` are directly recursive at **0.96 calls
   per invocation** (tree walks). The first inclusive pass therefore multiplied
   them by `1/(1−r) ≈ 25` and printed `ftDisplayMainDrawDefault` at **476,903
   tk/fr — larger than the whole lane.** Fixed by converting `f` from all
   invocations to top-level ones (`× (1 − r)`), which is what `I(S)` prices.
   `diag.py` asserts the graph has exactly these two back edges and no others.
2. **Root divisor.** Restricting the root's call sites to the post-gate ones
   without also dividing by the invocations that *reach* the gate halved the
   whole flow: model total **133,909** with a uniform **48%** share on every
   symbol — 3,200/6,624. The uniform share is what gave it away.

Neither reached a conclusion. Recorded because the tells are reusable: **an
inclusive figure larger than its own root is a cycle, and a constant share
across unrelated symbols is a divisor.**

---

## 7. What was NOT done

- **No build, no emulator run of my own.** Every number is an exact reduction of
  captures already on disk.
- **No per-PC account of the 267,000 tk/fr of instruction fetch outside FTR.**
  §3's refusal is scoped to this lane only.
- **The contract-change rate was not measured**, so §5's 34,307 is a ceiling.
- `STG` (175,424 P50, `f` to close 38%) was not touched; it is the second-largest
  flat lane and the same instrument now exists for it — the root would be the
  stage draw span rather than `ndsFighterDisplayContractSubmit`.
- No closed lane was reopened: `EXCHANGE.md`'s 2.68 and the float→fixed class
  stay closed (and are now sized at 1.4% of this lane), link-order placement
  stays closed, the D-cache layout lane stays closed.

## Reproduce

```powershell
python artifacts/performance/2026-08-16_ftr-attribution/masks.py
arm-none-eabi-objdump -d --no-show-raw-insn builds/build-c221-sitrprof/smash64ds-battle-playable-tickhud-hwtri.elf > c221.dis
arm-none-eabi-objdump -d                    builds/build-c221-sitrprof/smash64ds-battle-playable-tickhud-hwtri.elf > c221-raw.dis
python artifacts/performance/2026-08-16_ftr-attribution/callsites.py c221.dis
python artifacts/performance/2026-08-16_ftr-attribution/scan.py          # ~3 min, one pass over 3.87 GB
python artifacts/performance/2026-08-16_ftr-attribution/attribute.py m80gate
python artifacts/performance/2026-08-16_ftr-attribution/groups.py    m80gate
python artifacts/performance/2026-08-16_ftr-attribution/density.py   c221-raw.dis m80gate
python artifacts/performance/2026-08-16_ftr-attribution/convert-ftr.py
python artifacts/performance/2026-08-16_ftr-attribution/diag.py
```
