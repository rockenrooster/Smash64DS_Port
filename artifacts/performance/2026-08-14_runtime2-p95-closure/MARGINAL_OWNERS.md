# The marginal frames are 92% simulation, and the v3 profile is the wrong arm to prove it on

> **Superseded in part, 2026-08-14 — see `GATE_ARM_OWNERS.md` in this directory.**
> §8's "re-banking is the next cycle's first build" is done (P95 1,210,944 →
> 1,184,064) and the gate-arm v3 capture §8 says is missing now exists. §2.3's
> `SRC`/`GCRA` structure **reproduces** on the new match; §7's ranking *below*
> `SRC` does **not** — only `SITR` survives. §3.2's soft-float question is
> answered there too.

**Date:** 2026-08-14 · **Branch:** `codex/r2-runtime2` · **Builds spent: 0.**
**UNITS: 2 profile cycles = 1 project tick.** Tick-HUD buckets are already ticks.
Every table states the window it was computed over and the divisor it used.

---

## 0. Outcome first

1. **`plan.md` §5's join is refuted.** The v3 stall capture is
   `builds/build-c125-profile`, **`NDS_R2_BOTH_CPU 0` / `NDS_TICK_HUD_DRAW 0`** —
   the Boundary arm with the instrument's draw burst compiled out. The c147 rows
   are `build-c147-ctl`, **`BOTH_CPU 1` / `DRAW 1`**. Different arm, different
   binary, different match. Masking the v3 profile by frame indices taken from
   the c147 rows would have produced a table that means nothing. §1.
2. **The gate-arm ranking does not need the profile at all**, and it is decisive:
   on the **80 frames that literally set `WORK-H` P95**, the excess over a
   two-VBlank frame is **+520,718 ticks, and 92.1% of it is `SRC`** — all of it
   inside `GCRA`. `FTR` + `STG` + `MISC` together are **+24,947, i.e. 4.8%**. §2.
3. **Three fighter procs hold 85.7% of that**: `SINT` +178,455 · `SHDT`
   +119,920 · `SPHD` +112,833. §2.3.
4. **`plan.md` §0's "160 marginal frames" set is 64% instrument.** 102 of the 160
   are *already below* the cadence boundary in `WORK-H`, and 98 of those 102
   carry the tick-HUD draw burst. Only **58** of the 160 are genuinely
   `WORK-H`-bound. §2.2.
5. **The unexamined pool reproduces to the tick** (123,772 vs `plan.md`'s
   123,773) and is **not one owner**: its largest single holder is 9.1%. It is
   also **not new work** — every top holder is already a named owner in another
   lane. §4.
6. **`plan.md` §3 item 9 is refuted as stated.** The newlib formatted-I/O family
   is *linked and reachable* at `NDS_TICK_HUD=0`: `sniprintf` is called from
   `ndsRelocAssetFindEntry`'s NitroFS path builder, which is not instrument
   gated. Its conclusion survives anyway — the residual is **≤769 tk/frame**,
   20x under the floor. §6.

---

## 1. The discriminating read: the two captures are not the same match

Taken first, before any join was attempted.

| | c147 rows | v3 stall capture |
|---|---|---|
| artifact | `../2026-08-13_c-collision-stack/a-c147-ctl-rows.csv` | `../2026-08-14_icache-temporal/v3-baseline/` |
| build | `builds/build-c147-ctl` | `builds/build-c125-profile` |
| `NDS_R2_BOTH_CPU` | **1** (gate arm) | **0** (Boundary arm) |
| `NDS_TICK_HUD` | 1 | 1 |
| `NDS_TICK_HUD_DRAW` | **1** | **0** |
| git | `bf22a37eec3` | `faf01d5` |
| ROM SHA-256 (16) | `466f736cb6718b99` | `c8c26f66fc3398b4` |
| window | frames 439–2038, 1,600 samples | frames 438–2038, 1,601 regions |
| instrument | tick-HUD buckets (ticks) | per-PC per-region stall classes (cycles) |

Sources: `builds/build-c147-ctl/nds_build_config.h:67,96`,
`builds/build-c125-profile/nds_build_config.h:67,93`,
`a-c147-ctl.json`, `ICACHE_TEMPORAL.md` §5.

`BOTH_CPU 0` means **Mario is a human player replaying recorded input**, not a
level-3 CPU. The two captures play different matches, so region *n* of one is
not sample *n* of the other, and `DRAW 0` removes the burst that owns most of
the c147 arm's dropped frames. **There is exactly one v3 capture in the repo**;
every other profile on disk (`2026-08-09` … `2026-08-12`, fourteen captures,
2.5–2.6 GB each) is `profile-v2` and carries no stall classes at all — including
`2026-08-12_c123-rebank/profile`, which *is* the gate arm (`build-c123-profile`,
`BOTH_CPU 1`, `DRAW 0`) but only has cycles and instructions.

**Re-scope taken.** Two independent analyses on two axes each capture actually
supports, never joined:

- **§2 — gate arm, per-frame, bucket granularity, from the c147 rows alone.**
  Right arm, right instrument, no profile involved. This is the axis that
  decides Phase 4.
- **§3 — Boundary arm, per-PC, stall-class granularity, from the v3 capture
  masked by its *own* per-frame non-idle cost.** Names mechanisms and prices
  stall classes; **must not be read as gate ticks.**

---

## 2. Gate arm — the frames that decide the number

Window: `a-c147-ctl-rows.csv`, 1,600 samples, frames 439–2038, gate arm,
DLDI on. Everything below is recomputed from that file; nothing is inherited.

### 2.1 Cadence and the two different frame sets

VBlank interval derived as `round(ALL / 560,190)`; `ALL` is VBlank-quantized by
construction.

```text
VBI 2:1360  3:225  4:12  5:1  7:1  9:1        max 9
two-VBlank share            85.0%   (target >=95%)
cadence boundary            1,116,096   (the most expensive frame that still presented in 2)
WORK-H P50                    924,928
WORK-H P90                  1,100,608
WORK-H P95     rank 80 (0-idx 79)  1,212,224   |  rank 81  1,210,944  (the harness's own rounding)
dropped frames (VBI>=3)           240   of which 225 are exactly 3
160th-cheapest dropped frame  1,210,944
```

Every figure above reproduces `plan.md` §0 exactly, so §0's arithmetic is
confirmed, not replaced.

**But "the 160 frames to convert" and "the frames that set P95" are different
sets, and the plan conflates them.**

| set | definition | n | what it governs |
|---|---|---:|---|
| **cadence set** | the 160 cheapest dropped frames | 160 | the 95% two-VBlank target |
| **P95 set** | the 80 most expensive frames by `WORK-H` | 80 | the `P95 <= 1,120,380` gate |

They overlap in only part of their membership, and the P95 set is the one the
gate is written against: P95 of 1,600 samples **is** the 80th-largest value.
To reach 1,120,380 the 80th-largest must fall **91,844**.

### 2.2 The cadence set is 64% instrument — measured, not inferred

| | count |
|---|---:|
| marginal (cadence) frames | 160 |
| …carrying a tick-HUD draw burst (`HUD` > 100,000; mean there 427,607) | **101** |
| …already **below** the 1,116,096 cadence boundary in `WORK-H` | **102** |
| …of those 102, carrying the burst | **98** |
| …genuinely `WORK-H`-bound (above the boundary) | **58** |
| mean cut those 58 need | **43,916** (max 94,848) |
| two-VBlank frames carrying a burst | **3 of 1,360** |

Whole dropped population (240): 110 carry the burst, 138 exceed the `WORK-H`
boundary. A burst frame almost never presents in two VBlanks (3/1,360 = 0.2%).

**Consequence.** `plan.md` §0's "the cut needed **on those frames** is ~94,848"
is the *worst* frame's requirement, not the set's: the typical `WORK-H`-bound
marginal frame needs **43,916**, and 102 of the 160 need **no `WORK-H` cut at
all** because they were dropped by the instrument. This does not move the P95
gap — §2.3 shows that is real — but it does mean a cadence figure read off the
`DRAW=1` arm is measuring the tick HUD.

> `BLOCKED(decision: instrument cadence arm)` — `plan.md` §6 asks the owner to
> choose (a) read cadence from a `DRAW=0` arm, (b) phase the HUD draw, or
> (c) accept the conservative reading. This document supplies the number that
> choice turns on (98–110 of 240 dropped frames are burst frames) and makes no
> recommendation.

### 2.3 The owner ranking — nested, not double counted

`WORK-H = (FTR + STG + BG + AUD + SRC + MISC) + (OTHR − WAIT)`, which the tick
HUD guarantees (`taskman_seam.c:5201-5206`) and which closes **to 0.0 ticks** on
both populations here. `WORK-H = WORK − HUD` also holds exactly, so the HUD
burst is outside every figure below.

**P95 set — the 80 most expensive frames by `WORK-H`** (1,212,224 … 5,020,288,
mean 1,436,122; 9 of the 80 carry a burst). Excess is against the 1,360
two-VBlank frames' mean (`WORK-H` 915,405).

| bracket | top-80 mean | 2-VBlank mean | **excess** | ratio | share of the excess |
|---|---:|---:|---:|---:|---:|
| **WORK-H** | 1,436,122 | 915,405 | **+520,718** | 1.57x | 100% |
| **SRC** (outermost sim bracket) | 798,559 | 318,743 | **+479,816** | 2.51x | **92.1%** |
| ` ` `GCRA` = `gcRunAll`, inside SRC | 793,388 | 313,503 | +479,885 | 2.53x | 92.2% |
| ` ` SRC outside GCRA (derived) | — | — | **−68** | — | 0.0% |
| ` ` ` ` **`SINT`** fighter interrupt proc | 332,965 | 154,510 | **+178,455** | 2.16x | 34.3% |
| ` ` ` ` ` ` `SCPU` level-3 AI, inside SINT | — | — | +7,222 | — | 1.4% |
| ` ` ` ` ` ` `SITR` = SINT − SCPU (derived) | — | — | **+171,234** | — | 32.9% |
| ` ` ` ` **`SHDT`** live-hitbox hit detect | 126,503 | 6,583 | **+119,920** | **19.2x** | 23.0% |
| ` ` ` ` **`SPHD`** physics-map default | 173,845 | 61,012 | **+112,833** | 2.85x | 21.7% |
| ` ` ` ` `SPRM` params/anim interpreter | 51,368 | 1,991 | +49,377 | 25.8x | 9.5% |
| ` ` ` ` GCRA remainder (SOBJ: camera, effects, items, interface) | — | — | +19,141 | — | 3.7% |
| `MISC` draw residual | 125,511 | 109,098 | +16,414 | 1.15x | 3.2% |
| `AUD` | 23,765 | 7,990 | +15,775 | 2.97x | 3.0% |
| `FTR` fighter draw | — | — | +7,987 | 1.03x | 1.5% |
| `STG` stage draw | — | — | +546 | 1.00x | 0.1% |
| `OTHR − WAIT` | — | — | +198 | — | 0.0% |

**Cadence set — the 160 cheapest dropped frames**, for comparison
(`WORK-H` excess only +104,117, because 102 of them are not `WORK-H`-bound):

```text
SRC +87,294 (83.8%)   GCRA +87,358   SRC-outside-GCRA -64
  SINT +48,173 (SCPU +3,797, SITR +44,376)   SHDT +18,177   SPHD +13,421
  SPRM +2,093   GCRA remainder +5,411
MISC +9,814   FTR +3,393   AUD +2,937   STG +253   OTHR-WAIT +426
```

Both populations agree on the shape: **SRC is essentially all of the excess, and
all of SRC's excess is inside `gcRunAll`.** The draw side is flat where P95
lives — which independently re-confirms the `docs/HANDOFF.md` note that only
`FTR`/`STG` are flat at the percentile, and quantifies it: 1.03x and 1.00x.

---

## 3. Boundary arm — per-PC stall attribution (mechanisms, not gate ticks)

Capture `../2026-08-14_icache-temporal/v3-baseline/arm9-profile.csv`, 3.46 GiB,
52,296,701 rows, 96,218 distinct PCs, `stall_partition_residual = 0`.
Reduced in one streaming pass by `scripts/census-marginal-frame-owners.py`;
the reduction is cached beside this file as `marginal-pc.csv` (top-160 mask) and
`marginal-band-pc.csv` (band mask) so **no future cycle re-scans 3.46 GB**.

**Mask.** A profile region is a *presented frame*, so `total_cycles` is the
VBlank-quantized present interval — 1,537 of 1,601 regions sit within a few
hundred cycles of 2,240,760 = two VBlanks. Sorting on it sorts rounding noise.
The mask is therefore built on **`total_cycles − halt_wait`**, the profile-side
analogue of `WORK-H`:

```text
non-idle ticks/region   P50 898,730   P90 1,007,091   P95 1,081,812   max 3,055,464
mask A  top 160 by non-idle           threshold 1,007,803 ticks
mask B  band [1,007,803 .. 1,210,944] 138 frames  (drops the 22 load-frame outliers
                                       that carried 18.0% of mask A's total)
basis   marginal ticks/frame = cycles / (2 x n frames)
basis   whole-match ticks/frame = cycles / (2 x 1601) = cycles / 3,202
```

**Mask B is the falsifier for mask A, and mask A survives it**: the top-16
owners are the same functions in the same order, all within ~2% except the two
whose cost really was load-frame work (`memcpy` 23,244 → 17,214, `memset`
18,013 → 15,500). The ranking below is therefore not an outlier artefact.

### 3.1 Top owners on the marginal frames — symbol census (mask A, 160 frames)

`issue + icache_fill + dcache_fill + write_buffer + interlock + bus_contention`,
ticks/frame, divisor `2 x 160 = 320`. Mask total **1,123,230 tk/frame** over
1,305 owners, against a whole-match total of **896,334** (+25.3%).

| ticks/fr | share | issue | icache | dcache | wbuf | intlk | bus | owner |
|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 39,750 | 3.5% | 38,125 | 1,624 | 0 | 0 | 0 | 0 | `__aeabi_fadd` |
| 31,024 | 2.8% | 29,211 | 1,813 | 0 | 0 | 0 | 0 | `__aeabi_fmul` |
| 29,431 | 2.6% | 3,383 | 10,474 | 11,109 | 3,303 | 1,163 | 0 | `ndsFighterMarioFoxDLAllDrawForSlot` |
| 28,429 | 2.5% | −1,085 | 16,249 | 8,092 | 448 | 1,533 | 3,192 | `ndsRendererCommitNativeStageSegment` |
| 25,865 | 2.3% | 10,881 | 0 | 3,300 | 149 | 1,279 | **10,256** | `ndsRendererNativeEmitProductionPrimitiveGroups` |
| 25,125 | 2.2% | −158 | **20,427** | 3,980 | 76 | 800 | 0 | `ndsR2AnimValueQ` |
| 24,809 | 2.2% | 4,019 | 8,017 | 9,957 | 550 | 2,267 | 0 | `gcPlayDObjAnimJoint` |
| 23,244 | 2.1% | 9,584 | 143 | 5,821 | 6,613 | 392 | 691 | `memcpy` |
| 21,535 | 1.9% | 10,115 | 697 | 7,390 | 1,702 | 1,580 | 51 | `ndsRendererExecuteNativeFighterOwnerProduction` |
| 20,872 | 1.9% | 3,788 | 10,278 | 5,591 | 298 | 917 | 0 | `ndsR2FtAnimParseDObjFigatree` |
| 18,299 | 1.6% | −372 | 12,900 | 3,996 | 312 | 1,462 | 0 | `ndsRendererMtxMulAffine20p12` |
| 18,013 | 1.6% | 4,420 | 71 | 564 | **12,843** | 116 | 0 | `memset` |
| 17,675 | 1.6% | 8,985 | 273 | 4,074 | 687 | 1,809 | 1,848 | `ndsRendererNativePrepareProductionRun` |
| 11,500 | 1.0% | 11,350 | 151 | 0 | 0 | 0 | 0 | `__aeabi_fdiv` |

**Instrument caveat: `issue` can be negative per PC.** It is the v3 partition's
residual class, so a PC whose named stalls slightly over-attribute reads below
zero. It is sound in aggregate (`stall_partition_residual = 0`) and unusable as
a per-PC quantity on its own.

**Inline attribution (`addr2line -f`) changes the answer in three places** and is
why both are printed: `ndsRendererCommitNativeStageSegment` 28,429 → 10,499 and
`ndsRendererTask29GXRecord` appears at 15,811 (it is inlined into the segment
commit); `ndsFighterMarioFoxDLAllDrawForSlot` dissolves into
`ftDisplayMainDrawDefault` 9,995, `ndsRendererNativeStagePreparedTextureValid`
9,419 and others; `ndsBaseGcRunAll` surfaces at 9,278. 3,202 tk/frame (2.9%)
falls below the 30,000-PC attribution cut and is labelled as such.

### 3.2 Family roll-up (mask A, symbol census)

| family | whole match | marginal 160 | excess |
|---|---:|---:|---:|
| renderer + GX | 391,436 | 408,427 | +16,991 |
| unclassified (`nds*` non-renderer, ITCM helpers, libnds, FAT) | 184,045 | 279,211 | +95,166 |
| source simulation (`ft*`/`gm*`/`mp*`/`sy*`/…) | 148,031 | 175,238 | +27,207 |
| animation | 79,520 | 117,243 | +37,724 |
| **soft-float / libgcc** | **65,555** | **99,981** | **+34,426** |
| libc mem/str | 27,747 | 43,129 | +15,381 |
| **total** | **896,335** | **1,123,230** | **+226,895** |

`__aeabi_fadd` + `__aeabi_fmul` + `__aeabi_fdiv` alone are **82,274 tk/frame**
on the marginal frames and are **97% `issue`** — no cache component to fix, so
the only lever is *executing fewer of them*. Unattributed PCs are 374 tk/frame
(0.0%), so the census is complete.

---

## 4. The unexamined pool: real, reproduced, and already owned

`write_buffer + interlock + bus_contention`, whole match:
**123,772 tk/frame** — `plan.md` §0 says 123,773. Basis `cycles / 3,202`.
On the marginal frames it is **146,893** (mask A) / **138,910** (mask B).

| whole match tk/fr | share | wbuf | intlk | bus | owner | actionable? |
|---:|---:|---:|---:|---:|---|---|
| 11,297 | 9.1% | 145 | 1,239 | **9,914** | `ndsRendererNativeEmitProductionPrimitiveGroups` | **No, not as a pool item.** 88% is bus contention on GX FIFO writes. `memo-is-a-memory-stream` and the FIFO-word price note already own this; the lever is fewer submitted primitives, which is the Phase 6 particle/no-Z ladder. |
| 10,653 | 8.6% | **10,570** | 83 | 0 | `memset` | **Maybe, and it is the cleanest item in the pool.** Pure write-buffer drain — a bulk zeroing whose *size* or *frequency* is the lever, not its code. Needs a call-site census first; none exists. |
| 5,195 | 4.2% | 475 | 1,530 | 3,190 | `ndsRendererCommitNativeStageSegment` | No. Already the largest owner of the closed code-placement lane. |
| 4,302 | 3.5% | 3,174 | 1,128 | 0 | `ndsFighterMarioFoxDLAllDrawForSlot` | No. Fighter draw is 1.03x at P95 (§2.3). |
| 4,216 | 3.4% | 658 | 1,758 | 1,799 | `ndsRendererNativePrepareProductionRun` | No. |
| 4,158 | 3.4% | 184 | 249 | 3,725 | `ndsRendererLoadHardwareSplitMatrices` | No — GX FIFO again. |
| 4,075 | 3.3% | **3,945** | 131 | 0 | `ndsRendererSyncTextureTile` | Possibly: a texture upload's write burst. Same shape as `memset`. |
| 3,285 | 2.7% | 197 | 86 | **3,002** | `tickGetCount` | **No — this is the instrument.** With `cpuGetTiming` it is 13,406 tk/frame on the marginal frames; it is apparatus, already inside the approved 24,947. |
| 3,542 | 2.9% | 3,286 | 187 | 69 | `memcpy` | Partly load-frame: 3,542 whole match, 7,696 on mask A, 6,104 on mask B. |

**Verdict on the pool.** It is **not a lane**. Its largest holder is 9.1%, its
top nine holders sum to 41%, and every one of them is already the property of a
named lane (GX submission, fighter draw, texture upload, or the instrument
itself). The only pool-shaped items are `memset` (10,570 write_buffer) and
`ndsRendererSyncTextureTile` (3,945) — together **14,515 tk/frame whole match**,
under the 16,000 floor before any conversion ratio is applied. **A package built
on "the 123,773-tick pool" would be building on an accounting category, not on a
mechanism.**

---

## 5. Bracket nesting — resolved from the source, not from magnitudes

`plan.md` §5 item 3 asked whether GCRA / SRC / SINT are nested. They are, and
the containment is stated in the code that installs each bracket:

```text
SRC   gNdsTickHudSourceTicks       taskman_seam.c:4481  -- the ONLY writer;
                                   ndsRunMarioFoxProofUpdate brackets
                                   scVSBattleFuncUpdate (taskman_seam.c:4274)
 |
 +- GCRA  gNdsTickHudSrcRunAllTicks   battleship_sys_objman.c:145-176
 |        "the SOLE gateway to the whole simulation INSIDE the SRC bracket ...
 |         SBAS - GCRA is exactly the work SRC does OUTSIDE the simulation"
 |
 |   +- SINT  ftMainProcUpdateInterrupt   reloc_backend_diagnostic_recorders.c:5733
 |   |        fighter proc priority 5, dispatched by gcRunAll
 |   |   +- SCPU  "the level-3 CPU AI is nested INSIDE this proc (ftmain.c:1269)
 |   |             ... the analyzer subtracts: SITR = SINT - SCPU"
 |   +- SHDT  ftMainProcSearchHitAll     …recorders.c:5678   per-fighter GObj proc
 |   +- SPHD / SPHC  ftMainProcPhysicsMap{Default,Capture}  priorities 4/3,
 |   |        mutually exclusive arms of one predicate (ftmain.c:1918-1937)
 |   +- SCAT / SPRM / SCPU  the other three of the six per-fighter procs
 +- SWRM  ndsR2AnimCachePreloadStep
```

`diagnostics.c:3051-3078` confirms every one of these is *published after*
`named` is summed and **never added to it**, precisely so `ALL` and `OTHR` are
not double counted.

**Measured corroboration, both populations.** `SRC − GCRA` — the work SRC does
outside `gcRunAll` — has an excess of **−68** on the P95 set and **−64** on the
cadence set. Two independent populations both put it at zero within one tick of
counter rounding, which is the arithmetic proof that GCRA ⊆ SRC and that the
nesting is complete.

**SHDT's placement is proved twice.** Its bracket comment says only "nested
inside the SRC bracket", but the arithmetic forbids it being a sibling of GCRA:
`SRC` excess 479,816 < `GCRA` 479,885 + `SHDT` 119,920. It takes a fighter
`GObj *` and is dispatched with the other fighter procs, so it runs under
`gcRunAll`.

**So the correct, non-double-counted reading of the P95 excess is:**
take `SRC` (+479,816) **or** its children, never both; and inside `SINT` take
`SITR` (+171,234) plus `SCPU` (+7,222), never `SINT` plus `SCPU`.

---

## 6. newlib formatted I/O — refuted as stated, closed anyway

`plan.md` §3 item 9 records the family as "already attributed — instrument-only
… `NDS_TICK_HUD` only" and asked Phase 0 to "confirm zero reachability at
`NDS_TICK_HUD=0` with one objdump check". The check does not confirm it.

**Symbols, `nm -C --defined-only`, three ELFs:**

| ELF | `NDS_TICK_HUD` | `_vfiprintf_r` `_svfiprintf_r` `__ssvfiscanf_r` `iprintf` `siscanf` `__sprint_r` |
|---|---|---|
| `build-battle-playable-proof-hwtri-harness/…proof-hwtri.elf` | **0** | **all present** |
| `build-c153-whispy-fix/smash64ds-battle-playable-hwtri.elf` | **0** | **all present** |
| `build-c147-ctl/…tickhud-hwtri.elf` | 1 | all present |

`--gc-sections` does not drop them, so absence cannot be the proof. **Call sites,
`objdump -d` over the `NDS_TICK_HUD=0` proof ELF, every `bl`/`blx` target:**

```text
iprintf     <- main, ndsPlatformInit, ndsRelocAssetsInit, consoleClear,
               ndsPlatformPrintDebugLine, __sassert          (boot / one-shot)
sniprintf   <- main                                          (boot)
            <- ndsRelocAssetFindEntry                        <-- NOT instrument gated
siscanf     <- con_write (libnds console escape parse), _tzset_unlocked_r
_vfiprintf_r  <- iprintf, viprintf, __sbprintf
_svfiprintf_r <- sniprintf, _vsniprintf_r
__ssvfiscanf_r<- siscanf
printf / sprintf / siprintf / sscanf / vsiprintf: NO call site in the image
```

`ndsRelocAssetFindEntry` → `ndsRelocAssetMarioAnimEntry` /
`ndsRelocAssetFoxAnimEntry` (`src/nds/nds_reloc_assets.c:144-213`) builds a
NitroFS path with `sniprintf` on **every animation-asset lookup**, at
`NDS_TICK_HUD=0` as much as at 1. The family is reachable in the shipped ROM.

**It is still closed, on cost.** From the v3 per-PC census (whole match,
`build-c125-profile`, divisor 3,202):

```text
consolePrintChar 357.0  con_write 136.8  _vfiprintf_r 511.0  __sprint_r 22.7
iprintf 22.9  siscanf 33.5  __ssvfiscanf_r 409.5   <- console/debug text, TICK_HUD + boot
_svfiprintf_r 737.0  sniprintf 31.7  vsniprintf 21.9  _vsniprintf_r 28.8
FAMILY TOTAL 2,312.8 tk/frame
```

At `NDS_TICK_HUD=0` only the `sniprintf` route survives, bounded above by
`sniprintf` 31.7 + all of `_svfiprintf_r` 737.0 = **≤769 tk/frame**, and the true
value is far lower because `vsniprintf` (73 HUD call sites,
`nds_platform.c:2062-2075`) is the other consumer of the same formatter.
**20x under the 16,000 floor. Closed — with the reason corrected.**

---

## 7. The single largest attributed owner, and what a package against it must do

**Largest attributed owner of the P95 excess: `SRC` / `gcRunAll` — the logical
simulation — at +479,816 ticks on the 80 frames that set P95, 92.1% of the
excess.** It is not a function; it is the whole `gcRunAll` dispatch, and inside
it three fighter procs hold 85.7%:

| owner | P95-set excess | ratio to a 2-VBlank frame | already-recorded state |
|---|---:|---:|---|
| `SINT` (minus AI) = `SITR` | **+171,234** | 2.16x | `SINT` is the fighter INTERRUPT proc with `SCPU` nested — not an animation bucket (HANDOFF). No mechanism has been priced against `SITR` itself. |
| `SHDT` | **+119,920** | **19.2x** | HANDOFF: "CLOSED — bar 47,424 tk/fr … band-only cuts saturate at 78,016, fixed point only". Closed **for the mechanisms tried**, not sized on the P95 frames. |
| `SPHD` | **+112,833** | 2.85x | Never lane-sized. `ftMainProcPhysicsMap` is shared with `SPHC`, which is +0 here. |
| `SPRM` | +49,377 | 25.8x | HANDOFF: "closed by arithmetic — under 16,000 deleted entirely" — that verdict was whole-match mean; **its P95-set excess is 49,377.** |

**What a ≥16,000 (prefer ≥30,000) package must do.**

1. **Target the presence, not the mean.** `SHDT` runs 19.2x and `SPRM` 25.8x
   heavier on the P95 frames than on a two-VBlank frame while their whole-match
   means are small. A cut sized off a census row will under-predict here by an
   order of magnitude, and one sized off the excess will over-predict if the
   mechanism is not the thing that spikes. The package needs a per-frame
   engagement counter on the *spiking* quantity (how many hit-detect pairs, how
   many physics-map segments) before any code changes, on the **gate arm**.
2. **Clear 91,844 at the 80th-largest frame**, not 16,000 on average. The gate is
   P95 ≤ 1,120,380 and the 80th-largest is 1,212,224. A change that removes
   30,000 uniformly moves P95 by 30,000; a change that removes 100,000 from only
   the top 20 frames moves P95 by **zero**. Rank the whole distribution after,
   not the top rows.
3. **Do not build it on the draw side.** `FTR` 1.03x, `STG` 1.00x, `MISC` 1.15x
   at P95. The hot-instruction-footprint lane (`plan.md` §7) is sized off
   whole-match `icache_fill` and is a P50 lever; §2.3 says it cannot be a P95
   lever unless its win lands inside `gcRunAll`. That is a real tension with the
   current plan and it is stated here rather than resolved.
4. **Do not build it on the pool.** §4: the pool has no owner above 9.1%, and its
   only two pool-shaped items sum to 14,515 whole match.
5. **Price soft float separately.** `__aeabi_fadd`/`fmul`/`fdiv` = 82,274
   tk/frame on the Boundary arm's marginal frames, 97% `issue`, so the lever is
   call count. Where those calls live is a gate-arm question this cycle could not
   answer — see §8.

---

## 8. What this document does NOT do

- **It does not attribute the gate arm's P95 excess to functions.** No v3 stall
  capture exists for `BOTH_CPU 1`; the only gate-arm per-PC data on disk
  (`2026-08-12_c123-rebank/profile`, 2.6 GB, v2) has cycles but no stall classes,
  and is `DRAW 0`. §2 gets to bracket granularity and stops there, deliberately,
  rather than borrowing §3's function names onto the wrong arm.
- **It does not re-run the gate.** Zero builds, zero emulator runs.
- **It does not choose the instrument cadence arm** (`plan.md` §6) — owner's.
- **It does not re-open any lane in `plan.md` §3.** Item 9 is corrected in place
  (§6) and stays closed; items 1–8 and 10–11 were not touched.
- **It does not price `memset`'s call sites.** §4 names it as the one clean pool
  item and says explicitly that no call-site census exists.
- **It does not claim the banked 1,210,944 still holds on the settled HEAD.**
  That bank was measured on `build-c147-ctl` (git `bf22a37eec3`), which predates
  the four owner-confirmed 2026-08-14 fixes now committed. Two of them —
  the MASKS/MASKT quad subdivision and Whispy's every-frame dynamic-binding
  validation — add per-frame work. **Re-banking is the next cycle's first build.**

## 9. Reproducing this

```powershell
# one streaming pass over 3.46 GiB; the two reduced CSVs beside this file are its output
python scripts/census-marginal-frame-owners.py --reduce `
    --profile artifacts/performance/2026-08-14_icache-temporal/v3-baseline `
    --out artifacts/performance/2026-08-14_runtime2-p95-closure/marginal-pc.csv --marginal 160
python scripts/census-marginal-frame-owners.py --reduce `
    --profile artifacts/performance/2026-08-14_icache-temporal/v3-baseline `
    --out artifacts/performance/2026-08-14_runtime2-p95-closure/marginal-band-pc.csv `
    --band-min 1007803 --band-max 1210944
# free to re-rank from here on
python scripts/census-marginal-frame-owners.py --report `
    --pc-csv artifacts/performance/2026-08-14_runtime2-p95-closure/marginal-pc.csv `
    --build builds/build-c125-profile --top 25
```
