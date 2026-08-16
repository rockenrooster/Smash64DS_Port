# `SITR` is not a cost, it is a call-count event: a status transition re-attaches 22 animation joints, the per-joint pipeline runs 1.58x, and its quantisation runs 12.68x — on 288 frames worth 72,768 at rank-80

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `31c4bca3922`**
1 lab build (`build-c221-sitrprof`), 1 v3 profile capture, 2 whole-match counter runs on the
existing basis ROM, **0 production source edits, 0 defaults flipped, 0 ROMs published, both
root ROMs byte-unchanged.**
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states its
window.

```text
REQUIREMENT  +65,297 net ticks per presented frame at rank-80.  BASIS
             build-c220-camship, rank-80 1,210,624 raw / 1,185,677 net against the
             1,120,380 gate (apparatus 24,947), band 41-120 1,218,356; shipping
             renderer (GX_COMPOSE 0), bore 0, mode 163 one-minute match,
             NDS_R2_BOTH_CPU=1, 1,600 samples, frames 439-2038, slips=0.
             REPRODUCED from the basis rows here: overgate.py re-derives rank-80
             1,210,624 and +65,297 with the leaf closure exact on all 1,600 rows.

RESTATED     THE SITR CLUSTER IS 25 FRAMES ON THIS BASIS, NOT 27, AND IT IS WORTH
             45,056, NOT 51,200.  Both inherited figures are IO_AUDIT.md's, taken
             on build-c219-animitcm-ship.  Re-derived on the current basis by the
             same two methods: 25 frames, median own excess 227,968, median SITR
             332,288 against a run median of 104,320 = 3.19x.  Exact re-rank of
             clearing their SITR excess: rank-80 1,210,624 -> 1,165,568, 45,056
             moved, level +20,241.  Section 1.

MECHANISM    A STATUS TRANSITION, AND THE COST IS A CALL COUNT, NOT A PRICE.  The
             interrupt proc itself is entered 4.00 times per event frame against
             3.87 otherwise -- 1.03x, i.e. NOT more often.  Inside one invocation:
               battleship_ftMainSetStatus        1.22 calls/fr vs 0.00
               lbCommonAddFighterPartsFigatree   1.22          vs 0.00
               ndsRelocSetStatusBufferFile       2.43          vs 0.00
               ndsRelocAssetIDForToken           2.61          vs 0.00
               ndsRelocNormalizeFighterAObj16File 0.49         vs 0.00
               gcAddDObjAnimJoint               22.06          vs 0.35   62.4x
             and the attach then makes the ENTIRE per-joint animation pipeline run
             again inside the same presented frame:
               ftMainPlayAnim / ftParamUpdateAnimKeys  5.65 vs 3.71   1.52x
               ndsR2FtAnimParseDObjFigatree          100.83 vs 63.69  1.58x
               gcPlayDObjAnimJoint                   100.91 vs 64.04  1.58x
               ndsR2AnimValueQ                       386.15 vs 242.05 1.60x
               ndsR2AnimBuildTrackTable               37.05 vs 13.09  2.83x
               ndsR2AnimTargetValue                  253.20 vs 39.80  6.36x
               ndsR2AnimAObjToQConvert               208.99 vs 16.48 12.68x
             Sections 3 and 4.

LEVER        288 OF 1,600 FRAMES CARRY AN ATTACH OR A FORCE-LOAD, AND THEY CARRY
             81.2% OF THE RUN'S WHOLE SITR EXCESS.  On them SITR reads 1.77x its
             run median and EVERY OTHER LEAF READS 0.87x-1.08x -- SCPU 0.87, FTR
             1.00, STG 1.00, SHDT 1.01, GCRARES 1.03, SPHD 1.08.  Exact re-rank of
             clearing their SITR excess: rank-80 1,210,624 -> 1,137,856,
             72,768 moved, level -7,471.  That is 1.11x the entire remaining
             requirement, from 18% of the frames.  Ceiling, not an implementation.
             Sections 2 and 5.

REFUTED      "THE FORCE-LOAD IS THE OWNER."  Per-frame counters over all 1,600
             frames rank the attach (r=+0.623) and the STEPPED parse (r=+0.650)
             ABOVE the force-load (r=+0.487) and the card read (r=+0.549), and 10
             of the 25 cluster frames carry no force-load at all.  The implied
             per-load SITR cost still falls with count (+122,931 / +96,777 /
             +68,794 for 1 / 2 / 3), reproducing IO_AUDIT.md section 4's
             refutation on a new basis.  Section 3.1.

NOT THE LANE cpuGetTiming's 2^22 artifact (2 samples, frames 1464 and 1849,
             corrected live by the sampler), the HUD refresh (r=+0.058; the 114
             HUD frames have SITR median 104,384 against a run median of 104,320),
             the draw side (FTR 1.00x on the 288), and extra logic ticks (the
             interrupt proc's own entry rate is 1.03x) are each measured and each
             ruled out.  Sections 3.2 and 4.3.
```

---

## 1. The cluster, re-derived on the current basis

`IO_AUDIT.md` §5 clustered the over-gate set on `build-c219-animitcm-ship`. The basis moved
on 2026-08-16 (`CAMERA_SHIP.md`), so `overgate.py` re-runs both of its methods on
`../2026-08-16_camera-ship/ship220-rows.csv`, which the sampler already wrap-corrected live
(2 samples, frames 1464 and 1849 — its warning block is in `ship220-run.log`).

**Control first.** The leaf decomposition
`WORK-H = FTR+STG+BG+AUD+MISC+(OTHR-WAIT)+SRC`, `SRC = GCRA+SWRM+SRCRES`,
`GCRA = SINT+SPHD+SPHC+SCAT+SHDT+SPRM+GCRARES`, `SINT = SCPU+SITR`
closes **exactly on all 1,600 rows — 0 violations, 0 negative leaves** — and reproduces the
published basis: rank-80 **1,210,624**, net **1,185,677**, level **+65,297**, band 41–120
**1,218,356**.

| cluster | c219 (`IO_AUDIT.md`) | **c220 (this cycle)** | median own excess | median own | run median | ratio |
|---|---:|---:|---:|---:|---:|---:|
| `SHDT` | 33 | **32** | 261,216 | 265,824 | 4,608 | 57.69× |
| **`SITR`** | 27 | **25** | **227,968** | **332,288** | **104,320** | **3.19×** |
| `SPRM` | 7 | 8 | 294,144 | 296,256 | 2,112 | 140.27× |
| `SPHD` | 8 | 8 | 195,488 | 267,776 | 72,288 | 3.70× |
| `MISC` | 5 | 4 | 216,640 | 324,512 | 107,872 | 3.01× |
| `AUD` | — | 3 | 128,192 | 130,880 | 2,688 | 48.69× |

**Quote 3.19×, not 57×.** `SHDT`'s 57.69× is real *for `SHDT`* — its run median is 4,608, so
almost anything it does is a large multiple. `SITR`'s run median is 104,320 and its cluster
median is 332,288: the concentration is **3.19×**, and a lever on it converts near 1:1 rather
than at 57:1.

**Exact re-rank** (set the named frames' `SITR` to the run median, re-sort `WORK-H`, read the
80th value — a ceiling, not an implementation):

| delete own excess on… | rank-80 | moved | level |
|---|---:|---:|---:|
| *(control)* | 1,210,624 | 0 | **+65,297** |
| `SHDT` cluster (32 frames) | 1,160,384 | 50,240 | +15,057 |
| **`SITR` cluster (25 frames)** | **1,165,568** | **45,056** | **+20,241** |
| `SPHD` (8) | 1,200,832 | 9,792 | +55,505 |
| `SPRM` (8) | 1,204,160 | 6,464 | +58,833 |
| `MISC` (4) | 1,205,184 | 5,440 | +59,857 |
| `AUD` (3) | 1,206,400 | 4,224 | +61,073 |

The 25 frames: 447, 521, 530, 553, 702, 830, 877, 954, 989, 991, 1013, 1015, 1032, 1186,
1229, 1302, 1323, 1372, 1447, 1471, 1491, 1625, 1655, 1886, 1900.

---

## 2. Counters first, on the ROM that produced the basis

Two whole-match runs on `builds/build-c220-camship` with `-PerFrameGlobals`, **no rebuild**,
sixteen counters between them. The buckets those runs write are torn (`IO_AUDIT.md` §4), so
**only their counter columns are used**; the bucket series is the basis run's `-RingDump`
rows.

**The offset is measured, not assumed.** Frames whose counter delta shows a force-load read a
mean `SITR` of **240,297** at offset **+1** against a run mean of 117,500 — a **+122,797**
lift — while offsets −1, 0 and +2 give +18,879, +10,484 and +4,997. Unique, not fitted, and
it reproduces `IO_AUDIT.md` §4's offset on a different basis.

**Determinism control, free and it passes.** The two counter runs are separate emulator
sessions on one ROM and their `WORK-H` columns are **identical on all 1,600 frames**.

| counter | total, frames 438→2037 | median/frame | on the 25 | ratio |
|---|---:|---:|---:|---:|
| `gNdsR204AnimForceLoadTotal` | 134 | 0 | 1 | — |
| `gNdsRelocAssetPayloadReadCount` | 7 | 0 | 0 | — |
| `gNdsFighterNaturalMotionFigatreeAttachCount` | 6,329 | 0 | **18** | — |
| `gNdsR2FtAnimParseCalls` | 112,661 | 72 | 95 | 1.32× |
| `gNdsR2FtAnimParseStepped` | 29,424 | 15 | **34** | **2.27×** |
| `gNdsR2FtAnimParseEarlyOut` | 83,237 | 56 | 57 | 1.02× |
| `gNdsR2CubicEvals` | 225,864 | 142 | 202 | 1.42× |
| `gNdsR2RelocAliasVisits` | 13,992 | 0 | 107 | — |
| `gNdsR2AnimCacheBytes` | 17,744 | 0 | 0 | — |
| `gNdsShieldAnimJointInstallCalls` | 29 | 0 | 0 | — |
| `gNdsR2FtAnimRecipMisses` | **0** | 0 | 0 | — |
| `gNdsFTComputerStatusChangeCount` | **0** | 0 | 0 | — |

The last two are **dead in this configuration** — reported as zero rather than quietly
dropped, because a zero counter and a missing mechanism look identical.
`gNdsFighterStructStatusSetCount` was checked and rejected before the run: it is reset in
`taskman_seam.c:1077` and read in `reloc_backend_fighter_model.c:2193` but **never
incremented anywhere in the tree**, so it could not have discriminated anything.

The 7 card-read frames are **456, 830, 1015, 1186, 1625, 1655, 1886** — the identical list
`IO_AUDIT.md` §1.1 measured on `c219`. The load events are a property of the match, not of the
binary.

### 2.1 The 288-frame population, and why it is the right one

288 of 1,600 frames carry a figatree attach or a force-load.

| leaf | event-288 median | run median | ratio |
|---|---:|---:|---:|
| **`SITR`** | **184,224** | **104,320** | **1.77×** |
| `SCPU` | 46,624 | 53,696 | 0.87× |
| `FTR` | 290,368 | 290,432 | 1.00× |
| `STG` | 175,616 | 175,424 | 1.00× |
| `SHDT` | 4,672 | 4,608 | 1.01× |
| `GCRARES` | 84,000 | 81,632 | 1.03× |
| `MISC` | 106,784 | 107,872 | 0.99× |
| `SPHD` | 78,048 | 72,288 | 1.08× |

**One leaf moves and nothing else does.** They hold **50 of the 80 over-gate frames (62%)**
and **81.2% of the run's total `SITR` excess** (30,221,632 of 37,210,240 ticks). Their
`WORK-H` median is 1,054,016 against 916,224 for the other 1,312.

`SCPU` at **0.87×** is the tell that this is a state transition rather than a busy frame: the
level-3 AI does *less* on a frame where the status already changed.

| delete `SITR` excess on… | rank-80 | moved | level |
|---|---:|---:|---:|
| *(control)* | 1,210,624 | 0 | +65,297 |
| the 25-frame cluster | 1,165,568 | 45,056 | +20,241 |
| **every attach / force-load frame (288)** | **1,137,856** | **72,768** | **−7,471** |
| `SITR` → its median on all 1,600 | 1,124,096 | 86,528 | −21,231 |

---

## 3. What the counters refute

### 3.1 The force-load is not the owner

Pearson `r` against per-frame `SITR`, all 1,600 frames:

```text
gNdsR2FtAnimParseStepped                   +0.6498      gNdsR2AnimCacheBytes    +0.5674
gNdsR2FtAnimParseCalls                     +0.6619      gNdsRelocAssetPayloadReadCount +0.5489
gNdsR2CubicEvals                           +0.6525      gNdsR2RelocAliasVisits  +0.4876
gNdsFighterNaturalMotionFigatreeAttachCount +0.6230     gNdsR204AnimForceLoadTotal +0.4870
gNdsR2FtAnimParseEarlyOut                  +0.3695      gNdsRelocFindMemoScans  +0.0689
```

The animation columns and the attach rank **above** the force-load and the card read. Ten of
the 25 cluster frames carry no force-load at all.

**And the per-load figure still is not a price.** Grouped by force-loads on the frame:

| loads | n | `SITR` mean | lift | implied per load | parse calls | cubic evals |
|---:|---:|---:|---:|---:|---:|---:|
| 0 | 1,484 | 107,902 | — | — | 68.1 | 136.7 |
| 1 | 101 | 230,833 | +122,931 | **+122,931** | 95.4 | 188.5 |
| 2 | 12 | 301,456 | +193,554 | **+96,777** | 128.2 | 255.9 |
| 3 | 3 | 314,283 | +206,381 | **+68,794** | 143.3 | 300.7 |

It falls 1.79× from one load to three. `[[a-residual-divided-by-a-count-is-not-a-price]]`,
reproduced on a new basis. What *does* scale is the animation call count in the last two
columns, which is the mechanism §4 names.

### 3.2 Four candidate causes, each measured and each ruled out

| candidate | measurement | verdict |
|---|---|---|
| the `2^22` `cpuGetTiming` artifact | 2 corrected samples, frames 1464 and 1849, on both the basis run and both counter runs — none of them a cluster frame | not the lane |
| the HUD refresh | `r(SITR, HUD) = +0.058`; the 114 frames with `HUD > 100,000` have `SITR` median **104,384** against a run median of **104,320**; 2 of the 25 are HUD frames | not the lane |
| a whole-frame slowdown | on the 4 cluster frames with *no* counter movement the draw side is flat (`FTR` 1.09×, `STG` 1.00×, `BG` 0.99×, `SPRM` 0.97×) while the sim rises (`SITR` 2.91×, `SCPU` 2.25×, `SHDT` 1.87×, `SPHD` 1.54×) | confined to the simulation |
| extra 60 Hz logic ticks | `ftMainProcUpdateInterrupt`'s own entry-PC rate is **4.00 on event frames against 3.87 on the rest, 1.03×**; on those 4 counter-flat frames the parse-call count is at the run median | the root is not entered more often |

---

## 4. The per-PC census, in the shipping configuration for the first time

`build-c221-sitrprof`. Its `nds_build_config.h` differs from `build-c220-camship`'s in **four
lines and nothing else**, machine-diffed into `config-diff.txt`:

```text
NDS_TASK37_PROFILE                  0 -> 1
NDS_TASK37_PROFILE_FRAMES         128 -> 1600
NDS_TASK37_PROFILE_PER_FRAME_REGION 0 -> 1
NDS_TASK10_GIT_SHORT        "f5e13aa" -> "31c4bca"      (a string literal)
```

`NDS_R2_FIGHTER_GX_COMPOSE 0`, `NDS_R2_FTANIM_TRACK 0`, `NDS_R2_STRIP_ROUTE 0`,
`NDS_R2_CAMERA_FIXED 1`, `NDS_TICK_HUD_DRAW 1`, `NDS_R2_BOTH_CPU 1`, battle pack on. **Every
prior per-PC census in the campaign was `GX_COMPOSE=1` and/or `FTANIM_TRACK=1`** — c200, c191,
c192 — which is the caveat `ANIM_ITCM.md` §2 and `ITCM_CENSUS.md` §1 both had to flag. This
one carries no such caveat.

```text
format=melonDS-arm9-retail-profile-v3   regions=1601   window frames 438..2038
instructions 1,048,562,499   cycles 3,886,597,991   program_counters 54,400,815
stall_partition_residual -19 of 3.89e9 (4.9e-9)   timestamp_discontinuities 2
```

### 4.1 The alignment is measured, twice

`run-task37-profile-census.ps1`'s banner prints *"region r = presented frame 438 + r"*.
`align.py` scores every offset in 434…444 against two independent anchors of the basis run:

| offset | 7 card-read frames, median profile rank | top-80 median profile rank | `r`(profile non-idle, tick-HUD `WORK-H`) |
|---:|---:|---:|---:|
| 438 | 545.0 | 257.5 | +0.342 |
| **439** | **12.0** | **118.5** | **+0.694** |
| 440 | 679.0 | 371.5 | +0.338 |

Every other offset in the range gives a card-anchor median of 315–699. **`region = frame −
439`**, confirming `SITR_DIRECT_CHILDREN.md` §7.1 and `census-marginal-frame-owners.py`'s own
docstring independently, on a new capture. `r = +0.694` is the documented ceiling for a
profile arm against a tick-HUD arm (~0.67); the two do not bracket the same interval.

Carried onto the profile, the masks are expensive where they should be: the 25 cluster frames
read a median **1,253,579** non-idle ticks against **891,737** for the quiet mask and
**940,239** for the whole run.

### 4.2 The call rates — the load-bearing table

Entry-PC instruction counts. The profiler is instruction-accurate, so the count at a
function's entry PC **is** its call count.

| symbol | event-288 | rest (1,313) | 25-frame cluster | whole | ratio |
|---|---:|---:|---:|---:|---:|
| `ftMainProcUpdateInterrupt` (= `SINT` root) | 4.00 | 3.87 | 4.00 | 3.89 | **1.03×** |
| `ftComputerProcessAll` (= `SCPU`) | 4.00 | 3.87 | 4.00 | 3.89 | 1.03× |
| `battleship_ftMainSetStatus` | **1.22** | **0.00** | 1.08 | 0.22 | **798×** |
| `lbCommonAddFighterPartsFigatree` | **1.22** | **0.00** | 1.08 | 0.22 | ∞ |
| `ndsRelocSetStatusBufferFile` | 2.43 | 0.00 | 2.16 | 0.44 | ∞ |
| `ndsRelocAssetIDForToken` | 2.61 | 0.00 | 3.16 | 0.47 | ∞ |
| `ndsRelocNormalizeFighterAObj16File` | 0.49 | 0.00 | 0.92 | 0.09 | ∞ |
| **`gcAddDObjAnimJoint`** | **22.06** | **0.35** | 19.44 | 4.26 | **62.4×** |
| `ftMainPlayAnim` / `ftParamUpdateAnimKeys` | 5.65 | 3.71 | 5.60 | 4.06 | 1.52× |
| `ndsR2FtAnimParseDObjFigatree` | 100.83 | 63.69 | 100.48 | 70.37 | **1.58×** |
| `gcPlayDObjAnimJoint` | 100.91 | 64.04 | 100.64 | 70.67 | 1.58× |
| `ndsR2AnimValueQ` | 386.15 | 242.05 | 382.32 | 267.97 | 1.60× |
| `ndsR2AnimBuildTrackTable.constprop.0.isra.0` | 37.05 | 13.09 | 37.72 | 17.40 | **2.83×** |
| `ndsR2AnimTargetValue.constprop.0` | 253.20 | 39.80 | 231.68 | 78.19 | **6.36×** |
| `ndsR2AnimAObjToQConvert` | 208.99 | 16.48 | 191.80 | 51.11 | **12.68×** |
| `get_fat.isra.0` | 176.09 | 36.10 | 612.40 | 61.28 | 4.88× |

**Read the first row against the rest.** The bracket's own root is entered 1.03× as often, so
nothing about the excursion is "the frame ran more simulation". Everything below it is extra
work inside the same four invocations.

### 4.3 The mechanism, stated once

> A fighter changes status. `battleship_ftMainSetStatus` runs (1.22/frame, **zero** on an
> ordinary frame) and calls `lbCommonAddFighterPartsFigatree`, which resolves the new clip's
> asset through `ndsRelocAssetIDForToken` — a **~110-branch linear `if`-chain plus, on a miss,
> two pointer scans over all 143 + 158 Mario/Fox animation ids**
> (`reloc_backend_assets.c:1922-2054`, whose own header comment already says *"remove this
> work in one change large enough to clear ~16,000 of tail movement, or move it off the
> gameplay frame entirely"*) — then attaches the clip joint by joint through **22.06
> `gcAddDObjAnimJoint` calls**.
>
> The attach re-enters the whole per-joint animation pipeline **inside the same presented
> frame**: parse 1.58×, evaluate 1.60×, and the three functions that turn constant FIGATREE
> ROM bytes into AObj node fields run **2.83×, 6.36× and 12.68×**.

That is a `compute once, not every frame` violation of exactly the shape `PROJECT_GOAL.md`
names, and the *count*, not the per-call price, is what moves.

### 4.4 Grouped, tk/fr

Against the derived control `rest` = the 1,313 frames not in `event288`, computed as
`(whole × 1601 − event288 × 288) / 1313`, so the two populations are exhaustive and disjoint
by construction and no second scan was needed.

| group | event-288 | rest | **Δ** | 25-frame cluster − rest |
|---|---:|---:|---:|---:|
| **ANIM parse** (`ParseDObjFigatree` + `BuildTrackTable` + `TargetValue` + `AObjToQConvert`) | 43,349 | 15,255 | **+28,094** | +27,047 |
| **ANIM evaluate** (`gcPlayDObjAnimJoint` + `ndsR2AnimValueQ` + `ftParamUpdateAnimKeys` + …) | 87,043 | 60,230 | **+26,813** | +27,617 |
| **ATTACH chain** (`ftMainSetStatus` → `…AddFighterPartsFigatree` → `AssetIDForToken` → `gcAddDObjAnimJoint`) | 30,169 | 6,368 | **+23,801** | +39,797 |
| CARD I/O stack (FAT + NitroFS + newlib + calico locks) | 19,487 | 3,910 | +15,576 | **+93,578** |
| MEMORY movers (`memcpy`/`memset`/`armCopyMem32`) | 43,666 | 29,347 | +14,319 | +32,498 |
| SOFT FLOAT leaves | 70,507 | 64,479 | +6,028 | +28,000 |
| everything else (non-idle) | 819,930 | 773,561 | +46,369 | +110,469 |
| **TOTAL non-idle** | **1,114,150** | **953,150** | **+161,001** | **+359,006** |
| `armWaitForIrq` (idle) | 247,451 | 228,235 | +19,217 | +207,339 |

Single largest symbols by Δ on the 288: `ndsR2FtAnimParseDObjFigatree` **+17,591**,
`ndsR2AnimValueQ` +9,041, `gcPlayDObjAnimJoint` +7,970, `memcpy` +7,183,
`battleship_ftMainSetStatus` **+6,658**, `ftParamUpdateAnimKeys` +4,804,
`gcAddDObjAnimJoint` +4,540, `armCopyMem32` +4,288, `ndsR2AnimBuildTrackTable` +4,257,
`ndsRelocAssetIDForToken` **+4,118**, `ndsR2AnimTargetValue` +3,615.
Largest **negative**: `ndsPlatformRenderDebugHud` −2,260 and `mpCollisionGetFCCommonFloor`
−751 — the instrument's own HUD and a collision query that does less on a transition frame.

> **The two instruments disagree on the size of the delta and both are right.** The tick-HUD
> says only `SITR` moves (§2.1, medians); the profile says +161,001 spread across the frame
> (§4.4, means). A mean over a population containing the 7 card frames is not a median, and
> the profile also carries its own apparatus (`ndsDrawSObjIntoPreview` +1,970,
> `_svfiprintf_r` +1,361, `ndsPlatformRenderDebugHud` −2,260). **The number to quote at the
> gate is the exact re-rank of the tick-HUD `SITR` series: 72,768.** The profile's job here is
> attribution, not pricing.

---

## 5. The sized lever, and what it is not

```text
LEVER      the attach-driven re-parse on 288 of 1,600 frames
CEILING    72,768 at rank-80  (1,210,624 -> 1,137,856, level +65,297 -> -7,471)
           = 1.11x the entire remaining requirement, from 18% of the frames
SHAPE      +28,094 parse  +26,813 evaluate  +23,801 attach chain, per event frame
CONVERTS   at 1.00x or better -- SITR's concentration on this population is 1.77x
           and the gate is a level, so a uniform deletion converts 1:1 and a lane
           above 1.00x converts better
```

**Three candidate routes, sized, none built, none recommended over another:**

1. **The token resolver.** `ndsRelocAssetIDForToken` is **+4,118 tk/fr on the 288 at 2.61
   calls/frame — 1,578 ticks per call** for a linear `if`-chain the source itself flags. Its
   own file already carries a *reordering* fix for a different caller (Slice 45,
   `reloc_backend_assets.c:3269-3292`), which is evidence the seam is workable. This is the
   smallest and most self-contained of the three and it is **pure lookup**, so it cannot
   change behaviour.
2. **The per-joint quantisation.** `ndsR2AnimTargetValue` (6.36×) and `ndsR2AnimAObjToQConvert`
   (12.68×) re-derive `arg × 2^-k` from the same constant `s16` every time a clip is attached.
   Together **+6,246 tk/fr on the 288** and the highest call-count multipliers in the tree.
3. **The re-attach itself.** 22.06 `gcAddDObjAnimJoint` calls make the pipeline run a second
   time in one presented frame. Halving that would take roughly half of +54,907, but
   **whether the newly attached clip must be evaluated in the same logic tick is a gameplay
   question that BattleShip source must answer before anything is written** — it is not a
   placement or a representation change, and it is not proposed here.

**What this does NOT do is reopen the AOT track pack.**
`../2026-08-15_ftanim-dispatch-attribution/RESULT.md` built that representation and measured
it at **−74 tk/fr whole match** at 23.25% parse-call coverage, −319 extrapolated to 100%, and
closed the lane on the finding that *"the parse path is FETCH-bound and the replacement is
another fetch-bound path of similar footprint"*. Nothing here contradicts that. One fact is
recorded for whoever prices it next and it is **not** an argument to rebuild:
`ndsR2FtAnimParseDObjFigatree` costs **28,853 tk/fr on the 288 against 14,427 whole match**,
so a whole-match null is not the same measurement as a rank-80 one on this lane. Re-pricing it
would need a rank-80 number, not a re-derivation of the old one.

---

## 6. What this cycle did NOT do

- **No production source was edited, no default flipped, no ROM published.** The only build is
  `build-c221-sitrprof`, a lab profile instrument.
- **No fix was implemented and no candidate was A/B'd.** §5 is a sized hand-off; there is no
  arm to measure yet.
- **The AOT track pack was not rebuilt or reopened.** §5 states its standing verdict verbatim.
- **`ndsRelocAssetIDForToken` was not rewritten.** It is sized at +4,118 on the 288 and left.
- **The `SHDT` cluster (32 frames, 50,240) was not touched.** It overlaps the `SITR` cluster in
  0 frames on this basis and is a separate lever.
- **Nothing was re-banked.** The requirement is +65,297 on `build-c220-camship` before and
  after.
- **No `analyze-subtree-attribution.py` closure was run**, so the numbers in §4.4 are per-symbol
  self time grouped by hand, not an exclusive subtree partition. Shared leaves
  (`memcpy`, `__aeabi_fadd`, `ftGetStruct`) are listed in their own groups and are **not**
  charged to the animation or attach groups.
- **The 4 counter-flat cluster frames (530, 989, 991, 1302) are diagnosed only to the level of
  "the simulation, not the draw".** Their `SCPU` at 2.25× is unexplained and is left as an open
  observation, not a mechanism.
- **`decomp/` untouched**; both root ROMs byte-unchanged (§8).
- **`build-c205-camtoggle` was not rebuilt.**

---

## 7. Free by-product: the four zero-execution ITCM residents, confirmed in the shipping build

`../2026-08-16_itcm-frsub/ITCM_FRSUB.md` §4 proposed an ITCM free-space route and named one
open premise: its zeros came from `build-c200-trackprof-off` at `GX_COMPOSE=1`. This capture
is the shipping configuration, and `v3-c221/census.txt` reads, in cycles over the whole
window:

```text
   0    444  __addsf3 (+1 alias)            0    192  ndsRendererNativeApplyStateSpan
   0    456  __aeabi_frsub                  0    256  ndsRendererNativeEmitDenseRawRun
   0    448  __nds_task16_..._fsub_golden   0    128  ...EmitProductionRawTexturedRun
   0     54  ndsFTParamsInvalidateFighterParts  0 112  ...EmitProductionRawUntexturedRun
```

**688 B of port-side ITCM, plus 54, execute zero instructions in the configuration that
ships.** The premise is closed. And the same census confirms the refutation in the other
direction: `__aeabi_l2f` **3,016,910 cycles**, `__floatsisf` 876,227, `__aeabi_ui2f` 456,083 —
the live tail of the member the frsub eviction would have moved.

`census.txt` §D ("ITCM admissions by non-mem stall per byte, 220 B free now, **+1,858 B
recoverable by eviction**") independently ranks `ndsR2CamDiv64` (68 B), 
`ndsStageCollisionLoopGeometryReady` (68 B), `DynamicArrayGet` (22 B) and `ftGetStruct` (92 B)
above the unbuilt bind-texture item, agreeing with `ITCM_FRSUB.md` §3 from the campaign's own
tooling. **Its 1,858 is a symbol-level count and 456 of it is the frsub blob, which
`ITCM_FRSUB.md` §1 shows no available mechanism can take** — that is a defect in the census's
recoverable figure, not in the eviction list.

---

## 8. Verification and hashes

```text
root ROMs, before the first build and after the last, unchanged and not rebuilt:
  smash64ds.nds                        54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
  smash64ds-battle-playable-hwtri.nds  6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99
```

**Boundary green, 0 `Exception:`, nine checks** — `boundary.trimmed.log`: `DECOMP_PRISTINE=PASS`
(`pinned_historical_files=10 ds_markers=0 decomp_patch_pipeline=absent`), GBI decode fixtures,
particle bank pack, harness registry, attack visual effects, Task 9 float ITCM
(`itcm=30164/32768 free=2604`), renderer ITCM placement (`itcm=30164 renderer=12896`), Task 20
DTCM layout, `battle_playable_realtime`, published ROM contract.

**Its realtime pacing smoke is the control that this cycle changed no production code**, and
it is exact. Against the last four Boundary runs on this tree (`c206` / `c209b` / `c217` /
`c219` / `c220`):

| | previous | this cycle | Δ |
|---|---|---|---|
| `binds` / `vtx` / `tri` | `54 / 2484 / 828` | `54 / 2484 / 828` | **identical** |
| `ftrTri` | `132712/p067840/p164872/own424` | `132712/p067840/p164872/own424` | **identical** |
| `frames` / `fps` / `rprof` | `212 / 241/480 / 0` | `212 / 241/480 / 0` | identical |
| `ticks` | 294,353,408 | **294,353,408** | **0** |
| proof-ROM `.itcm` | 30,164 / `renderer=12896` | 30,164 / `renderer=12896` | identical |

Boundary links its own `smash64ds-battle-playable-proof-hwtri`, so a tick count identical **to
the tick** across two separately-invoked builds is the strongest available statement that the
production tree is byte-unchanged.

---

## 9. Reproduction

```powershell
# counters, no rebuild, on the basis ROM
pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c220-camship -NoBuild -RunnerSlot 6 `
    -Samples 1600 -StartFrame 438 -TimeoutSeconds 7200 -AllowRepeatedFrames `
    -PerFrameGlobals gNdsR204AnimForceLoadTotal,gNdsRelocAssetPayloadReadCount,`
gNdsR2FtAnimParseCalls,gNdsR2FtAnimNullSkips,gNdsR2AnimCacheHits,gNdsBattlePackHits,`
gNdsR2CubicEvals,gNdsRelocFindMemoScans -RowsCsv ...\pf220-rows.csv -JsonOut ...\pf220.json
#   ... and the same with the section 2 second set -> pf220b-rows.csv

# the per-PC capture.  -PerFrameRegion defaults to $true and MUST be omitted under
# `pwsh -File`, which passes every argument as a string and cannot bind "1" to [bool].
pwsh -File scripts\run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe `
    -Build build-c221-sitrprof -StartFrame 438 -Frames 1600 -TimeoutSeconds 5400 `
    -OutDir ...\v3-c221 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1
```

Then, from the repo root and needing **no emulator**:
`overgate.py` (§1) → `mklists.py` (frame lists) → `join.py`, `join2.py`, `dpop.py` (§§2–3) →
`align.py` (§4.1, and it rewrites the region lists at the offset it measures) →
`multimask.py` (one scan of the 3.87 GB capture, four masks) → `attribute2.py` (§4.2–4.4) →
`rerank.py` (§§1, 2.1). `attribution-event288.txt` is `attribute2.py`'s output.

> **`nohup … &` inside a tool-run shell detaches the process from the task wrapper.** Doing it
> once here started a second copy of `multimask.py` that raced the tracked one for the same
> output file; the orphan finished first and the duplicate was stopped before it could
> overwrite a complete artifact. Use the harness's own background mode, never `nohup &`.
> `[[wait-on-the-writer-not-the-file]]`, in a new place.
