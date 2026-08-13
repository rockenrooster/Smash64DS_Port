# The gate residue — every open lane priced at the consumer, and why none reaches 16,000

**Outcome B.** No lever on the corrected gate arm predicts **≥16,000 `WORK-H`
P95 ticks/frame** as a single change. This document is the ranked table of
everything priced, the arithmetic behind each rejection, and the contingency
ladder with its measured sizes.

**No build, no emulator run, no source change was spent on the sizing.** All
figures come off two artifacts already on disk:

- `../2026-08-12_c130-fire-gate/c130-gate-rows.csv` — the 1,600-sample
  `BOTH_CPU 1` whole-match gate run (lane-level; **current** code).
- `../2026-08-12_c123-rebank/profile/` — `build-c123-profile`, `BOTH_CPU 1`,
  `NDS_TICK_HUD_DRAW 0`, `regions=1601` (symbol-level).

---

## 0. FIRST — the profile's cycles are NOT ticks. Divide by two.

Every per-frame figure taken off `arm9-profile.csv` before today was wrong by a
constant factor, in two different directions, and both errors are in documents
this cycle was told to build on.

**The ARM9 runs at 2x the tick timer.** Proof, from the artifacts and not from
the datasheet:

| reading | value |
|---|---|
| `arm9-profile.regions.csv` `total_cycles` P50 | **2,240,838** |
| 2 VBlanks of ticks (`2 x 560,190`) | **1,120,380** |
| ratio | **2.0001** |
| profile region histogram, `cycles/2` in VBlank units | 2:1414 3:170 4:13 5+:3 |
| c130 gate arm `ALL` histogram, in VBlank units | 2:1330 3:251 4:15 5+:3 |
| profile non-idle mean, `cycles/(2 x 1601)` | **966,428 tk/frame** |
| c130 gate arm `WORK-H` mean | **974,821 tk/frame** (0.9% apart) |

If cycles were ticks the profile ROM — which has the HUD *draw* compiled out —
would be presenting at 4 VBlanks where the gate ROM presents at 2, i.e. the
cheaper binary would be running at half the frame rate. It is not.

**So: `ticks/frame = cycles / (2 x regions) = cycles / 3202` for this profile.**

Consequences, all of which STRENGTHEN the two standing refutations:

| document | figure as published | corrected |
|---|---:|---:|
| `SITR_NEXT_CUT.md` CORRECTION, "best case if the header scan became free" | 13,000 | **6,500** |
| `2026-08-13_sitr-aobj-layout/REFUTED_AOBJ_SIDE_ARRAY.md`, side-array best case | −6,758 | **−3,379** |
| same, "any layout" ceiling at 26 B / at the 20 B dense-bank record | 9,155 / 10,491 | **4,578 / 5,246** |
| `ndsR2AnimValueQ` Cubic-arm prologue split | 2,200–2,700 | **1,100–1,350** |
| `__aeabi_lmul` ITCM library-member admission | 1,005 | **~503** |

The two pairing crumbs the brief carried forward are therefore **1/6 of the
±8,544 placement floor each** and are not pairing material. They are removed
from the menu.

`scripts/analyze-symbol-line-profile.py` had a third convention: it normalised
to `--frame-budget 1128000` over measured non-idle, which inflates every row by
`1,128,000 / 966,428 = 1.167x`. Fixed in this commit — it now reads `regions`
out of `arm9-profile.meta.txt` and reports `cycles / (2 x regions)`, printing
the basis line so the next reader can check it.

---

## 1. Where P95 actually lives, and which lanes pay 1:1

P95 is the **80th largest of 1,600**. A change only moves it if it moves the
frames ranked around 80. Composition of ranks 56–115 (`c130-gate-rows.csv`):

| lane | band mean | band median | **band min** | shape |
|---|---:|---:|---:|---|
| `FTR` | 304,989 | 302,432 | **296,320** | **flat** |
| `STG` | 175,554 | 175,200 | **171,520** | **flat** |
| `MISC` | 131,218 | 121,888 | 83,776 | mostly flat |
| `SOBJ` | 106,443 | 96,320 | 74,880 | mostly flat |
| `SPHD` | 96,359 | 80,672 | 38,272 | spread |
| `SITR` | 222,868 | 224,192 | 7,104 | **bimodal** |
| `SHDT` | 71,641 | **6,656** | 3,008 | **bimodal** — absent on half the band |
| `SCPU` | 52,426 | 46,912 | **896** | anti-correlated (896 on the rank-80 frame) |
| `SPRM` | 9,981 | 2,048 | 1,792 | bimodal, tiny |

**Only `FTR` and `STG` are flat where the percentile lives.** A flat deletion of
D ticks in either pays exactly D on P95, and deletions in different flat lanes
add exactly: 8,000 from `FTR` **and** 8,000 from `STG` measures **16,000**;
12,000+12,000 measures 24,000; 16,000+16,000 measures 32,000 (recomputed rank,
not a model).

Everything else is bimodal, so a proportional cut buys less than its mean cost
suggests — the frames it empties fall below rank 80 and a different population
sets the percentile.

---

## 2. Lane-level upper bounds — the most each lane could ever pay

Method: multiply the lane by `(1-f)` on **every** frame and re-take the 80th
largest of `WORK-H`. Baseline rank-80 **1,220,480**; gate 1,120,380; gap
**100,100**. This is not the `-Ceilings` statistic (which flattens a lane to its
own median and therefore prices only the excursion); it is the harder question
"what if this lane got f% cheaper everywhere".

| lane | median | mean | ΔP95 if deleted **100%** | cut needed for ΔP95 = 16,000 | mean work at that cut |
|---|---:|---:|---:|---:|---:|
| `SRC` | 323,648 | 354,590 | 512,256 | −2.8% | 9,820 |
| `FTR` | 303,232 | 297,214 | 311,744 | **−4.8%** | **14,232** |
| `STG` | 174,656 | 177,629 | 177,344 | −9.1% | 16,119 |
| `SITR` | 105,408 | 123,407 | 180,544 | −6.9% | 8,536 |
| `MISC` | 105,024 | 112,794 | 120,320 | −15.2% | 17,152 |
| `SOBJ` | 83,200 | 91,328 | 100,352 | −15.5% | 14,120 |
| `SPHD` | 57,920 | 66,416 | 100,160 | −11.9% | 7,912 |
| `SCPU` | 44,608 | 47,267 | 48,384 | −32.1% | 15,167 |
| `SHDT` | 4,416 | 14,227 | 55,936 | −26.6% | **3,779** |
| `OTHR-WAIT` | 19,520 | 19,506 | 20,096 | −80% | 15,600 |
| `SPRM` | 2,048 | 4,710 | **13,056** | **UNREACHABLE at 100%** | — |
| `AUD` | 2,816 | 8,987 | **13,824** | **UNREACHABLE at 100%** | — |
| `BG` | 4,096 | 4,100 | 3,968 | **UNREACHABLE at 100%** | — |

**Three of the brief's five menu items close on this table alone:**

- **`SPRM` is dead.** `ftMainProcParams` deleted *in its entirety* pays 13,056,
  below the 16,000 bar and inside the 9,664 repeat spread. The "+274% vs the
  Boundary table" that put it on the menu is a ratio on a 2,944 base.
- **`SCPU` is effectively dead.** `ftComputerProcessAll` needs a **32.1%**
  reduction — 15,167 ticks/frame out of a level-3 CPU AI that must stay
  mechanically equivalent — and its tail multiplier is **0.29x**: it reads 896
  on the rank-80 frame, i.e. the AI is nearly absent on the frames that decide
  P95. A 16,000-tick flat cut of `SCPU` pays **12,224**, not 16,000, because
  most tail frames have no `SCPU` left to take it from.
- **`SPHD` is the one survivor of item 3** at −11.9% (7,912 ticks/frame), which
  is the cheapest lane after `SHDT` in absolute work. No design exists (§4).

**`SHDT` has by far the best leverage** — 3,779 ticks/frame of real work buys
16,000 P95, a **4.2x** tail multiplier, the best in the table. It is also the
only lane where the required cut is a quarter of the lane rather than a
twentieth of a very large one.

---

## 3. Named consumers, priced — nothing deletable reaches 16,000

Symbol-level, `build-c123-profile`, ticks/frame = `cycles / 3202`. **Caveat:
this profile predates `NDS_R2_FOX_GUN_OVERLAY` (Makefile:1019, `?= 1`, ON) and
the flame fix; per `../2026-08-12_c130-fire-gate/GATE.md` the pair cost +9,856
`WORK-H` P95, inside that arm's own 9,664 repeat spread.** Lane figures in §1–2
are from the current code; symbol figures here are from c123.

| tk/frame | symbol | lane | status |
|---:|---|---|---|
| 31,029 | `__aeabi_fadd` (all callers) | shared | **not a target**: 295 caller functions, largest single owner `ndsBaseGcPlayMObjMatAnim` at **5,160**; float in `gm*`/`mp*`/`ftMain*`/`ftComputer` is FROZEN |
| 28,734 | `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` (+inlines) | `FTR` | largest inline `ndsRendererAdapterComposeOwnerWorldsFlat` **4,994**, `BuildNativeProductionInputs` 4,977, `PrepareNativeMaterials` 4,443, dispatcher self 6,603 — no 16,000 sub-block |
| 27,700 | `ndsRendererCommitNativeStageSegment` (+inlines) | `STG` | self 10,428, `NativeStageEmitVertex` 5,956, `Task29GXRecord` 2,845, `AccountRun` 2,391 — honest vertex emission |
| 25,071 | `ndsRendererNativeEmitProductionPrimitiveGroups` | `FTR` | `.itcm`, 548 B, 26,276,646 insns — the fighter primitive emit itself |
| 21,462 | `__aeabi_fmul` | shared | as fadd |
| 21,141 | `gcPlayDObjAnimJoint` | `SITR`/`SOBJ` | **layout closed 2026-08-13**; call count is the only lever left |
| 21,027 | `ndsRendererExecuteNativeFighterOwnerProduction` | `FTR` | `.itcm` |
| 19,322 | `ndsR2AnimValueQ` | `SITR`/`SOBJ` | CPI 1.65, issue-bound; prologue split now sizes **1,100–1,350** |
| 18,182 | `ndsRendererAdapterBuildDObjXObjMatrix` (self 11,030 + mem 6,538 + float 614) | `FTR` | already ground c106 (the unconditional track gather, the `MulInto` copy); largest single owner of `memcpy`/`memset` in the ROM |
| 17,590 | `ndsRendererNativePrepareProductionRun` | `FTR` | `.itcm` |
| 17,342 | `ndsRendererMtxMulAffine20p12` | `FTR`/`STG` | the GX-palette replacement **exists and was withdrawn**: slice 43 measured **−13,632 P95** and is forced off on a matrix-stack leak (`nds_platform.c:3197`); owner-gated, and under the bar anyway |
| **14,691** | `cpuGetTiming` + `tickGetCount` | **apparatus** | **not product work** — see §5 |
| 14,242 | `memset` | shared | 148,369 calls; largest owner is the row above |
| 13,752 | `ndsRendererTask36ReplayRun` | `STG` | GXFIFO DMA is already ON (`Makefile:1471/1615/1830`) |
| 13,649 | `ndsRendererNativeStageBeginRun.part.0` | `STG` | |
| 13,480 | `ndsR2FtAnimParseDObjFigatree` | `SITR` | animation arithmetic spent (slices 34, 41) |
| 13,424 | `memcpy` | shared | 166,750 calls |
| 13,310 | `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` | `FTR` | |
| 11,632 | `ndsRendererNativeStageLoadNoZMatrix` | `STG` | with `EmitNoZTriangle` 5,501 + `EmitNoZVertex` 5,377 = **22,510** on the no-Z band |
| 11,040 | `ftParamUpdateAnimKeys` | `SITR` | |
| 10,744 | `ndsRendererLoadHardwareSplitMatrices` | `FTR` | |
| 10,607 | `mpCollisionGetFCCommonFloor` (self; +2,853 float) | `SPHD` | float FROZEN |
| 10,257 | `ndsRendererMtxMul20p12` | `FTR`/`STG` | |
| 8,991 | `syMatrixLookAtReflectF` (self 4,163 + float 4,828) | `SOBJ` | 512,546 soft-float calls a match |
| 6,558 | `ndsMPCollisionEnsureLineGroups` (self 2,234 + mem 4,323) | `SPHD` | 45,040 calls a match = **28.1/frame, flat** — so it pays ~6,558, not `SPHD`'s 2.02x proportional multiplier |
| 5,712 | `ndsR2AnimBuildTrackTable` (self 3,085 + mem 2,627) | `SITR` | lazy track table closed (slice 31) |

**The whole distribution is flat.** Non-idle work is **966,428 tk/frame** spread
over 1,379 executed symbols; the largest is 31,029 and is a shared leaf with no
owner over 5,160. Below the top ten, deleting a symbol *entirely* does not reach
16,000. That is the residue's central fact, and it is why the answer is B.

Soft-float totals for the record (`analyze-leaf-helper-attribution.py`,
244,314,171 cyc = **76,300 tk/frame**, 7.89% of non-idle): matrices/transform
15,947 · other 15,564 · gameplay 13,897 · collision 12,370 · animation
evaluation 11,357 · CPU AI 2,839 · renderer 2,739 · particles 1,588. No
subsystem is a 16,000 lever, and the four largest are inside the frozen-float
fence or already spent.

---

## 4. What is still genuinely open, ranked by cost-to-close

| rank | shape | cut needed | mean work | why it is not this cycle's change |
|---|---|---|---:|---|
| 1 | **`SHDT` — touch fewer parts in hit detection** | −26.6% | **3,779** | The lane's only designed lever is refuted geometrically (slice 47: `ReachTests 2,373 WouldSkip 0`), the transform chain is honest work with latches that clear once per fighter per frame (c123), and `gmCollisionTestRectangle` is shared with item/weapon/ground so no leaf can be charged to it. **A broad-phase that rejects a whole fighter pair before per-part tests is the only untried shape** — it must be exact (hit detection is gameplay), and its rejection rate has never been measured. That measurement, not a build, is the next step. |
| 2 | **`SPHD` — `ftMainProcPhysicsMapDefault`** | −11.9% | 7,912 | Never designed against. Largest identified component `mpCollisionGetFCCommonFloor` 10,607 is float-frozen; `ndsMPCollisionEnsureLineGroups` 6,558 is flat and pays 6,558, not 16,000. |
| 3 | **`FTR` flat deletion** | −4.8% | 14,232 | Pays 1:1 with no rank discount — the safest lever in the table — but 14,232 ticks/frame is 4.8% of a lane whose largest deletable sub-block is 4,994. Needs three independent deletions or one structural change. |
| 4 | **`STG` flat deletion** | −9.1% | 16,119 | Same shape, worse ratio. `STG` is 174,656 against SwitchPlan §4's 180K line — **already under its budget**, so §7 rung 1's "`STG` at ~195K against 180K" is stale by 20,344. |
| 5 | **`SITR`** | −6.9% | 8,536 | Its content is the fighter animation lane: arithmetic spent (34, 41), layout closed (2026-08-13), call count is all that is left. |
| 6 | **`SOBJ` / `MISC`** | −15.5% / −15.2% | 14,120 / 17,152 | `MISC` is particles and particles are flat; sub-rating is rung 2, i.e. a fidelity decision. |
| — | `SCPU`, `SPRM`, `AUD`, `BG` | — | — | **Closed by arithmetic in §2.** |

**Pairing that works:** flat cuts in `FTR` and `STG` add exactly. Two
independent 8,000-tick deletions, one in each, measure 16,000. That is the only
pairing route in the table that does not require a lane nobody has designed
against.

---

## 5. The gate figure carries 14,691 tk/frame of measuring apparatus

`cpuGetTiming` + `tickGetCount` cost **47,041,843 cycles = 14,691 ticks/frame**
over **280,841 calls a match (175.4/frame at 83.7 ticks per bracket read)**.
Every caller is a tick-HUD bracket: the six `ftMainProc*` wrappers at 12,464
calls each, `ndsStageGCDrawAllLoopRecordCapturedDisplay` at 88,324 (its read is
inside `#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL == 1)`,
`reloc_backend_movement.c:13768`), `syTaskmanRunTask` 28,802,
`lbParticleDrawTextures` 12,800, `gcRunAll` 6,400, `ndsPlatformEndFrame` 6,400.

The published ROM builds `NDS_TICK_HUD=0` and executes none of it, but those
reads land inside the `SRC`/`FTR`/`STG` brackets and therefore inside `WORK-H`.

**So the product-side gap is ≈72,500, not 87,236** — 1,207,616 − 1,120,380 −
14,691, subject to placement, since a no-HUD binary cannot report `WORK-H` at
all and would not lay out identically. This does not move the gate (the gate is
defined on this ROM and every banked figure includes the same apparatus), and it
is **not a lever** — optimising the instrument improves the number without
improving the game. It is recorded so the ladder below is chosen against the
right distance.

---

## 6. Contingency plan — SwitchPlan §7's ladder with today's numbers

### Rung 1 — buy headroom flat (agent-executable, deletion preferred)

§7 says *"`FTR` at ~389K against its 250K line and `STG` at ~195K against 180K
are the flat levers"*. **Both figures are stale on the corrected arm:**

| lane | §4 line | §7's figure | **measured, c130 gate arm** | over/under |
|---|---:|---:|---:|---:|
| `FTR` | 250K | ~389K | **303,232** | **+53,232** |
| `STG` | 180K | ~195K | **174,656** | **−5,344 (under)** |

`FTR` remains the flat lever and it is worth **53,232** against its own line —
3.3x the 16,000 bar and 61% of the product-side gap. `STG` is no longer a lever
against its line at all. Targets inside `FTR`, largest first: the fighter draw
dispatcher's own 28,734 (`ComposeOwnerWorldsFlat` 4,994 + `BuildNativeProductionInputs`
4,977 + `PrepareNativeMaterials` 4,443 + self 6,603), the native production
trio 63,688 (`EmitProductionPrimitiveGroups` 25,071 +
`ExecuteNativeFighterOwnerProduction` 21,027 + `NativePrepareProductionRun`
17,590), the matrix builders 42,236 (`BuildDObjXObjMatrix` 18,182 +
`BuildFighterTraRotRpyDirect20p12` 13,310 + `LoadHardwareSplitMatrices`
10,744), of which 6,538 is `memcpy`/`memset`. **Required: 14,232 ticks/frame in one change, or two
independent ~8,000s split across `FTR` and `STG`.**

### Rung 2 — cosmetic systems below simulation rate (round-robin, never batched)

`MISC` is particles and is 105,024 median / 112,794 mean. A −15.2% proportional
reduction (17,152 ticks/frame) is worth 16,000 P95. **Round-robin a quarter of
the generators per frame; do not batch quarter-rate work onto one frame** —
R2-03 E30 and the band table in §1 agree that batching lowers the median and
raises P95, and P95 is the gate. The *draw* half (`lbParticleDrawTextures`
9,208, `ndsRendererSubmitParticleQuad` 4,992) cannot be sub-rated without
visible strobing; only the update half is eligible.

### Rung 3 — reduce visual fidelity — **BLOCKED(decision: owner)**

Priced candidates, in size order, each with its visible delta:

| candidate | measured size | visible delta |
|---|---:|---|
| Stage no-Z band (`LoadNoZMatrix` 11,632 + `EmitNoZTriangle` 5,501 + `EmitNoZVertex` 5,377) | **22,510** | the depth-disabled background/foreground bands; a rate or geometry reduction here is the largest single cosmetic item in `STG` |
| `NDS_R2_FIGHTER_GX_COMPOSE` re-enable | **−13,632** (slice 43, measured) | none claimed — frame-locked captures were pixel-identical — but it is withdrawn on a **matrix-stack leak** (~3 pushes/frame, wrapping mod 32, `nds_platform.c:3197`). This is a **correctness** gate, not a fidelity one, and HANDOFF requires owner proof. |
| particle round-robin (rung 2) | up to 16,000 | quarter-rate generator motion |

### Rung 4 — a COMPENSATED 30 Hz simulation — **BLOCKED(decision: owner, in writing)**

`NDS_TASK106_UPDATES_PER_PRESENT=1` measured the **uncompensated** ceiling at
**−119,744 P95** (playing at half speed). That is 1.37x the banked 87,236 gap
and 1.65x the product-side 72,500 — **the only remaining move with the measured
size to close the gate alone**. Compensating it (advancing timers, physics
integration and animation two ticks per presented frame) is the design; the
judgement is `PROJECT_GOAL.md` sacrifice-order item 4 and belongs to the owner.

**Do not widen the gate.** 1.12M is the product contract.

---

## 7. What this cycle did NOT do

- No source change to any runtime path, no build, no emulator run, no ROM. Both
  root ROMs are untouched.
- Did not measure the `SHDT` broad-phase rejection rate — that is the single
  cheapest next measurement in the residue and it needs a counter, not a build
  of a candidate.
- Did not re-run `-Ceilings`; §2's statistic is deliberately a different and
  harder one, and the `-Ceilings` table in `LANES_BOTHCPU.md` still stands for
  what it measures (excursion above a lane's own median).
- Did not re-profile on current code. §3's symbol attribution is c123 and
  predates the fox-gun overlay and the flame fix (+9,856 P95 combined, inside
  the repeat spread); §1, §2 and §6 are all measured on the current arm.
