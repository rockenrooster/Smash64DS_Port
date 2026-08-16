# `SITR`'s direct children, re-derived on the c185 tree — animation is 34.5% of the bracket, and its parser is a 41,376 tk/fr AOT deletion

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `40dd9c89e80` / `c39ad139bf7`**
**Attribution only.** 2 lab builds, 2 v3 captures, 3 reduce passes, 1 Boundary. No production
source edited, no flag flipped, no re-bank, no published ROM rebuilt.
Predecessor: `…/2026-08-15_sitr-callback-decomp/SITR_CALLBACK_DECOMPOSITION.md` (binding, preserved).

---

## 0. Outcome first

```text
OWNER     ANIMATION.  The direct child `ftMainPlayAnim` -- the first half of the
          INLINED ftMainPlayAnimEventsAll -- is 89,099 tk/fr on the frames that
          set P95, 34.5% of the 258,196 tk/fr SITR bracket and the largest single
          child by 1.43x.  Whole match 57,356, concentration 1.55x.  Every row of
          it is priced by a MEASURED call rate against a program-wide caller set,
          not by static reachability.

MECHANISM YES, AND IT IS THE ONE ALREADY ON THE QUEUE.  Inside that child the
          PARSE half -- ndsR2FtAnimParseDObjFigatree + BuildTrackTable +
          TargetValue + AObjToQConvert -- is 41,376 tk/fr at rank-80 (whole
          18,564, concentration 2.23x; 40,003 on the c185 gate arm's own rank-80
          population, 3.3% apart), running 96.94 times per marginal frame to
          re-derive AObj node fields from STATIC FIGATREE ROM DATA.  That is
          1.44x the +28,689 net requirement and 2.59x the 16K floor, and it is
          deletable by representation, not by approximation: an AOT typed-track
          pack replaces a parse with an index.  Hand it to task 3 unchanged.

RANK      ANIM 89,099 > STATUS_CALLBACKS 62,155 > root body 7,986 > MOTION_EVENTS
          2,998 > COLANIM 1,905, with `ftComputerProcessAll` 12,435 SUBTRACTED by
          construction (it IS SCPU).  ftKeyProcessKeyEvents, ftHammerUpdateStats
          and ftParamTryUpdateItemMusic are measured at 0 / 0 / 1 -- three of the
          eleven source-level children never engage under P1.

CYCLE 109 HALF CONFIRMED, HALF REFUTED.  "~96% in ftMainPlayAnim and
          ftComputerProcessAll" is right that ftMainPlayAnim is the owner and
          WRONG that the pair is ~96%: ftComputerProcessAll is 12,435 and is
          subtracted out of SITR entirely, and the two together are 39.3% of the
          bracket, not 96%.

RETRACTION `SITR = 310,662.4 marginal-80` IS AN INSTRUMENT ARTEFACT AND THE
          NUMBER IS 260,354.  One frame -- 756 -- carries a 2^22 event in SINT
          (4,163,136 over the median, 0.74% off 4,194,304) and it is inside the
          c185 raw top-80 mask.  It alone contributes +50,309 of that mean.  The
          DRAW=0 arm of the same bank has no such frame and reads 258,196
          independently.  This CHANGES NO VERDICT: the callback ceiling of
          6,373.5 / 14,395.5 is further below a 16% smaller bracket.
```

---

## 1. The capture, and it is the c185 configuration

`build-c192-sitr-profile-gxc`, target `smash64ds-battle-playable-tickhud-hwtri`.
Its `nds_build_config.h` differs from `build-c185-gxcompose-bank`'s in **four
defines and nothing else**, machine-diffed:

| define | c185 bank | c192 profile | why |
|---|---|---|---|
| `NDS_TASK37_PROFILE` / `_FRAMES` / `_PER_FRAME_REGION` | 0 / 128 / 0 | **1 / 1600 / 1** | the instrument |
| `NDS_TICK_HUD_DRAW` | 1 | **0** | draw-noise reduction, permitted by the brief, proven below |
| `NDS_TASK107_…` / `NDS_TASK108_SITR_CALLBACK_CENSUS` | (absent) | **0 / 0** | both default OFF; absent code |
| `NDS_TASK10_GIT_SHORT` | `dd80585` | `c39ad13` | a string literal |

**`NDS_R2_FIGHTER_GX_COMPOSE` is 1**, matching the bank. This needed the
Makefile's one documented escape, `NDS_R2_FIGHTER_GX_COMPOSE_LAB=1`
(`Makefile:1754`): the tick-HUD block `override`s the flag to 0 unconditionally
otherwise. **The first arm of this cycle (`build-c191-sitr-profile-c185`) was
built without it and came out at flag 0** — caught by reading the generated
header, not by trusting the invocation. It was kept and re-used as the falsifier
arm rather than discarded.

`NDS_R2_BOTH_CPU=1`, `NDS_R2_BATTLEPACK=1`, `NDS_R2_BATTLEPACK_KEEP_CACHE=1`,
DLDI **on**, `emulators/melonds-attributor/melonDS.exe`, **no `-RunnerSlot`**.

```text
format=melonDS-arm9-retail-profile-v3   regions=1601   window frames 438..2038
instructions 984,318,545   cycles 3,754,393,168   program_counters 54,881,164
stall_partition_residual -4   timestamp_discontinuities 1
```

> **Two instrument facts stated rather than buried.** `stall_partition_residual`
> is **−4 cycles of 3.75 billion** (1.1e-9). `timestamp_discontinuities` is **1**,
> where `c191` and `c172` both read 0. Neither is dismissed by argument: they are
> bounded by the agreement in §2, where this capture correlates **r=0.9821** with
> `c191` and **r=0.9839** with `c172` and shares **72 of 80** marginal frames
> with `c191`.

**Mask.** `total_cycles − halt_wait`, never `total_cycles`. The 80 marginal
frames are those at or above **1,172,523 ticks**; median frame 950,566, max
3,101,208. **Basis for every marginal figure below: `ticks/frame = cycles /
(2 × 80) = cycles / 160`; for every whole-match figure, `cycles / 3,202`.** The
two divisors are different and are never mixed.

---

## 2. Frame-sequence identity — the gate the brief put on a `DRAW=0` arm

### 2.1 The two c185 gate arms are the same fight, frame for frame

From the banked rows (`…/2026-08-15_gxcompose-bank/whole-c185-rows.csv` and
`…-d0-rows.csv`), 1,600 common frames, Pearson `r` per bracket:

| bracket | `r` DRAW=1 vs DRAW=0 |
|---|---:|
| `SCPU` | **0.9998** |
| `SPRM` | **1.0000** |
| `SHDT` | 0.9987 |
| `SPHD` | 0.9980 |
| `SINT` | **0.5790** ← |
| `SINT`, dropping frame 756 | **0.9991** |

**One frame explains the entire `SINT` anomaly.** Every other simulation bracket
already agrees to four decimal places, which is what "the same match at the same
frame numbering" looks like.

### 2.2 That frame is the 2^22 class, and it is inside the banked mask

| | c185 DRAW=1 | c185 DRAW=0 |
|---|---:|---:|
| `SINT` median | 149,248 | 149,408 |
| frames with `SINT` − median > 3.0M | **1** (frame **756**, `SINT` 4,312,384) | **0** |
| excess over median | 4,163,136 (**Δ from 2^22 = −31,168, 0.74%**) | — |
| in the top-80 `WORK-H` mask | **yes** | — |
| **`SITR` marginal-80 with it** | **310,662.4** | 258,196.0 |
| **`SITR` marginal-80 without it** | **260,353.6** (79 frames) | 258,196.0 |

`K0_RERANK.md` §5 characterised this class and `census-tick-hud-p95-set.py`
gained `--drop-frames` for it, with the prohibition in its help text: **exclude
from attribution, never from a percentile.** The bank's 1,174,016 rank-80 is
untouched and stays exactly as banked. What changes is the *attribution* input:

> **`SITR_CALLBACK_DECOMPOSITION.md`'s "`SITR` = 310,662.4 marginal-80" is
> retracted to 260,354 (DRAW=1, 79 frames) / 258,196 (DRAW=0, 80 frames) — two
> arms, 0.83% apart.** Its verdict is unaffected and in fact strengthened: a
> callback ceiling of 6,373.5 unique closure, or 14,395.5 including the
> deliberately impossible `ftMainSetStatus` + figatree-attach grant, is
> **2.5% / 5.6%** of a 258,196 bracket rather than 2.1% / 4.6% of a 310,662 one.
> `NDS_TASK108_SITR_CALLBACK_CENSUS` stays default OFF, as instructed.

### 2.3 The profile instrument and the tick-HUD do NOT share a frame bracket

This is the reason the analysis below is masked on the profile's own top-80
rather than on the gate arm's frame numbers, and it is measured, not assumed:

| pair | best `r` | at lag | top-80 overlap |
|---|---:|---:|---:|
| `c192` profile vs `c191` profile | **0.9821** | 0 | **72 / 80** |
| `c192` profile vs `c172` profile | **0.9839** | 0 | — |
| `c192` profile vs `c185` DRAW=0 gate rows (`WORK`) | 0.6745 | **+1** | 12 / 80 |
| `c185` DRAW=1 vs `c185` DRAW=0 gate rows (`WORK-H`, 756 dropped) | 0.5083 | 0 | 70 / 80 |

Three profile arms at three HEADs agree at `r ≥ 0.982`; a profile arm against a
tick-HUD arm of the *same build family* peaks at 0.67 and needs a one-frame
shift. The tick-HUD's `WORK` is a sum of brackets **inside** one logic+draw
iteration; a profile region is the wall-clock span between two CP15 markers.
They measure the same match and **not the same interval**, so their rank-80
memberships are different sets by construction — note that even the two tick-HUD
arms of the same bank share only 70 of 80.

**What IS equivalent is the population's cost**, which is what a percentile
attribution needs:

```text
c192 profile rank-80 non-idle   1,172,523 ticks
c185 DRAW=0 rank-80 WORK-H      1,182,848 ticks     0.87% apart
c185 DRAW=1 rank-80 WORK-H      1,174,016 ticks     0.13% apart
```

The owner's literal population was also run, as a falsifier: §7.

---

## 3. What the direct children ARE, on the linked image

`SITR = SINT − SCPU` where `SINT` brackets the whole of
`battleship_ftMainProcUpdateInterrupt` and `SCPU` brackets the whole of
`ftComputerProcessAll` nested inside it
(`src/port/reloc_backend_diagnostic_recorders.c:5998-6014`,
`src/import/battleship_ftcomputer.c:167-186`). So **`ftComputerProcessAll` is
subtracted exactly, not approximately** — it is reported below and then removed.

Every `bl`/`blx` leaving the root, from `objdump -d` of the c192 ELF (identical
to c172's, so the shape did not move this cycle):

| source-level child (`ftmain.c:1214-1572`) | linked form | marginal calls/frame |
|---|---|---:|
| `ftMainPlayAnimEventsAll` | **INLINED** → `ftMainPlayAnim` + `ftMainUpdateMotionEventsAll` | 5.49 / 5.46 |
| `ftMainRunUpdateColAnim` | **INLINED** → `ftMainUpdateColAnim` | 6.99 |
| `ftParamResetStatUpdateColAnim` | 5 direct sites | 0.03 |
| `ftComputerProcessAll` | direct | **4.00** |
| `ftKeyProcessKeyEvents` | direct | **0.00** |
| `ftHammerUpdateStats` | direct | **0.00** |
| `ftParamTryUpdateItemMusic` | direct | 0.03 |
| `func_800269C0_275C0` (heal FGM) | direct | 0.74 |
| `ftGetStruct` | 3 sites | 264.24 |
| `proc_passive` / `proc_update` / `proc_interrupt` / `proc_lagend` | **INDIRECT — invisible to any static graph** | — |
| stick/jostle arithmetic | `__aeabi_fadd`×7, `fsub`×6, `fcmpeq`×6, `fcmplt`×5, `i2f`×4, `fmul`×4, `f2iz`×2 | — |

**The root runs 4.00 times per marginal frame** (3.89 whole) — two fighters ×
two 60 Hz logic ticks per presented frame. That rate is the denominator for
everything below.

`scripts/analyze-subtree-attribution.py` gained `--child-group` this cycle so a
dispatch slot is **one** child rather than sixty, and so an inlined source-level
child gets its name back. The 60 enumerated callback targets come from Task 108's
`callback-targets-all.csv` verbatim.

---

## 4. The direct-child table

c192, marginal-80 mask (80 frames ≥ 1,172,523 ticks), against the measured
bracket `SITR` = **258,196 tk/fr** (c185 DRAW=0) / **260,354** (c185 DRAW=1, 79
frames).

| # | direct child | **marginal-80 tk/fr** | whole tk/fr | excess | conc | marginal calls/fr | excl. syms | basis |
|--:|---|---:|---:|---:|---:|---:|---:|---|
| 1 | **`ftMainPlayAnim`** (ANIM) | **89,099** | 57,356 | +31,743 | **1.55x** | 5.49 (3.82 from the root) | 14 priced | per-row call-rate share, §5 |
| 2 | **status callbacks** (60 targets) | **62,155** | 25,143 | +37,012 | **2.47x** | 6.89 total¹ | 513 | exclusive-among-children closure |
| 3 | root body itself | 7,986 | 7,567 | +419 | 1.06x | 4.00 | 1 | its own PCs |
| 4 | `ftMainUpdateMotionEventsAll` | 2,998² | 1,166 | +1,832 | 2.57x | 5.46 | self only | self + `ftMainParseMotionEvent` |
| 5 | `ftMainUpdateColAnim` (+Reset) | 1,905² | 1,527 | +378 | 1.25x | 6.99 | self only | self only |
| 6 | `ftParamTryUpdateItemMusic` | 1 | 0 | — | 20.01x | 0.03 | 1 | items off |
| 7 | `ftKeyProcessKeyEvents` | **0** | 0 | — | — | **0.00** | 2 | no key player |
| 8 | `ftHammerUpdateStats` | **0** | 0 | — | — | **0.00** | 1 | items off |
| — | *(`ftComputerProcessAll` = `SCPU`, **subtracted**)* | *12,435* | *12,272* | *+163* | *1.01x* | *4.00* | *50* | *not part of SITR* |
| — | **named non-SCPU subtotal** | **164,144** | 92,759 | | | | | **63.6% of 258,196** |
| — | **unattributed residual** | **94,052** | | | | | | §6 |

¹ 60 targets; Task 108 counted 11,028 callback calls over 1,600 frames whole match.
² self time of the lane's own bodies only; their descendants are shared with the
callbacks and with `ftMainSetStatus` and are left in the residual rather than
double-charged. **No shared descendant is charged to two children anywhere in
this table.**

**Concentration matters as much as size, and it is quoted for every row** — the
campaign has now made this error in both directions (`MENU`'s asset I/O priced at
3.1x that converts 0.30x; §7's draw-side demotion on excess share that is the
largest lane). The gate is a **level**, so a uniform deletion converts 1:1 at
rank-80 and a lane above 1.00x converts *better* than 1:1. Both leading children
are above 1.00x: ANIM at 1.55x, callbacks at 2.47x.

---

## 5. The animation child, priced row by row

### 5.1 Why the lane and the child are two numbers

`ftMainPlayAnim` has exactly three program-wide callers, and the split is
measured from entry-PC call counts, not assumed:

```text
ftMainPlayAnim                       5.49 calls / marginal frame
  - battleship_ftMainSetStatus       1.01   (cross-checked: lbCommonAddFighterPartsFigatree,
                                             sole caller ftMainSetStatus, reads 0.99)
  - ftMainPlayAnimEventsAll (out-of-line copy, 58 *SetStatus/*ProcInterrupt callers)
                                     0.66
  = battleship_ftMainProcUpdateInterrupt's own inlined call site
                                     3.82  of the root's 4.00 invocations
                                           -> 95.5% of them have hitlag_tics == 0,
                                              which is exactly the source's guard
```

So the **lane** (everything under `ftMainPlayAnim`) is one quantity and the
**direct child** is 3.82/5.49 = **69.6%** of it. The other 30.4% arrives through
`ftMainSetStatus`, whose 143 program-wide callers include 53 reachable from
SITR's own callbacks — still SITR, charged to row 2 — and the rest under
`SPHD`/`SPRM`/`SCAT`/`SPHC`. `SHDT` reaches neither `ftMainSetStatus` nor
`ftMainPlayAnim` at all.

### 5.2 The lane, per symbol (`anim-lane-attribution.csv`)

| symbol | marg tk/fr | whole | share to the lane | charged marg | share evidence |
|---|---:|---:|---:|---:|---|
| `ndsR2FtAnimParseDObjFigatree` | 26,368 | 12,936 | 1.000 | 26,368 | sole caller `ftParamUpdateAnimKeys` |
| `ndsR2AnimValueQ` | 27,457 | 19,130 | 0.978 | 26,843 | sole caller `gcPlayDObjAnimJoint` |
| `gcPlayDObjAnimJoint` | 27,038 | 20,022 | 0.978 | 26,433 | 96.94 of 99.16 calls from `ftParamUpdateAnimKeys` |
| `ftParamUpdateAnimKeys` | 13,796 | 10,208 | 1.000 | 13,796 | sole caller `ftMainPlayAnim` |
| `ndsR2AnimBuildTrackTable.constprop.0.isra.0` | 7,308 | 3,061 | 1.000 | 7,308 | sole caller the parser |
| `ndsBaseGcPlayMObjMatAnim` | 6,461 | 5,579 | 0.905 | 5,848 | 81.35 of 89.88 via `gcPlayMObjMatAnim` |
| `ndsFTParamsInvalidateSubtree` | 9,291 | 7,176 | 0.572 | 5,316 | 5.55 of 9.70; 4.15 via `…TransformAll` |
| `ndsR2AnimTargetValue.constprop.0` | 5,014 | 1,834 | 1.000 | 5,014 | sole caller the parser |
| `gcParseMObjMatAnimJoint` | 5,298 | 4,063 | 0.905 | 4,795 | same 89.88 rate, same split |
| `ndsR2AnimAObjToQConvert` | 2,687 | 734 | 1.000 | 2,687 | both callers in the parse lane |
| `gcPlayMObjMatAnim` | 1,622 | 1,203 | 1.000 | 1,622 | 81.35 calls; other caller is the 0.99/fr attach |
| `ftMainPlayAnim` | 909 | 655 | 1.000 | 909 | lane root |
| `ftParamsUpdateFighterPartsTransform` | 576 | 426 | 0.989 | 570 | 5.49 of 5.55 |
| `ndsR2CubicValueFixed` | 556 | 277 | 0.978 | 543 | sole caller `gcPlayDObjAnimJoint` |
| **LANE TOTAL** | | | | **128,050** | whole **82,431**, conc **1.55x** |
| *(not charged, ambiguous)* | *`gcParseDObjAnimJoint` 4,428* | | | | *3 callers; 63.34 calls against the effects path's 62.09* |

**Soft float is deliberately excluded from this lane total**, which makes 128,050
a floor: `ndsR2AnimValueQ` is already a fixed-point kernel and the parser is
mostly integer, but `__aeabi_*` cost reached from here is left in the shared
residual rather than apportioned.

### 5.3 The split that matters for a mechanism

| half | marginal-80 tk/fr | whole | conc | marginal calls/fr | what it is |
|---|---:|---:|---:|---:|---|
| **PARSE** (`…ParseDObjFigatree` + `BuildTrackTable` + `TargetValue` + `AObjToQConvert`) | **41,376** | 18,564 | **2.23x** | 96.94 / 38.71 / 242.53 / 181.66 | turns static FIGATREE ROM bytes into AObj node fields |
| **EVALUATE** (`gcPlayDObjAnimJoint` + `ndsR2AnimValueQ` + `ndsR2CubicValueFixed`) | 53,819 | 38,547 | 1.40x | 99.16 / 387.40 / 2.41 | walks the AObj list and evaluates each curve |
| keys driver, joint transform, MObj material anim | 32,856 | 25,320 | — | 5.49 / 5.55 / 89.88 | |

Per-call prices, for anyone sizing a replacement:
`ndsR2FtAnimParseDObjFigatree` **272 ticks/call** at 17.66 joints per
`ftParamUpdateAnimKeys` call; `gcPlayDObjAnimJoint` **273 ticks/call**;
`ndsR2AnimValueQ` **70.9 ticks/call** at 3.91 AObj nodes per DObj.

---

## 6. The mechanism, its exact ceiling, and what it is not

**The parse half is a `compute once, not every frame` violation, measured.**
`ndsR2FtAnimParseDObjFigatree` runs **96.94 times per marginal frame** —
once per joint per fighter per logic tick — and on 38.71 of those it rebuilds the
per-DObj track table and re-quantises AObj values (`ndsR2AnimAObjToQConvert`,
181.66 calls/frame). Its inputs are the clip's FIGATREE bytes, which are
**constant ROM data**: `ndsR2AnimBuildTrackTable` (`battleship_ftanim.c:442-466`)
walks `root_dobj->aobj` and re-derives the same `track_aobjs[]` mapping every
time the same clip and joint are entered, and `ndsR2AnimTargetValue`
(`:189-262`) re-derives the same `arg × 2^-k` quantisation from the same `s16`.

**Ceiling: 41,376 tk/fr at rank-80** — `1.44x` the +28,689 net requirement and
`2.59x` the 16K implementation floor, at concentration **2.23x**, i.e. it
converts *better* than 1:1. Whole match 18,564. On the c185 gate arm's own
rank-80 population (§7.1) the same four rows read **40,003**, 3.3% apart, so the
ceiling does not depend on which definition of "the frames that set P95" is
used.

The ceiling is quoted on the **whole lane**, not on the direct child's 69.6%,
because the mechanism lives in the callee: an AOT pack replaces
`ndsR2FtAnimParseDObjFigatree` wherever it is called from, so all 96.94 calls
are addressed whether they arrived via the interrupt proc or via
`ftMainSetStatus`.

**What it is: a representation change.** Emit, host-side, one typed track row per
(clip, joint, segment) already in the Q form `ndsR2AnimValueQ` consumes —
`kind`, `track`, `value_base`, `value_target`, `rate_base`, `rate_target`,
`length`, `length_invert` — and have the runtime index it instead of parsing.
That is the task already queued as **animation representation (task 3)**, and it
consumes these numbers directly. It preserves behavior exactly if the emitted
rows are bit-identical to what the parser writes, which is checkable host-side
over all clips by the same shape the 2026-08-15 pack oracle used.

**What it is NOT.** It is **not** the evaluate half: `ndsR2AnimValueQ` is already
fixed-point, already `target("arm")` for SMULL/CLZ, and its 70.9 ticks/call over
387.40 calls is real per-frame curve evaluation that a representation change
makes *cheaper at best*, never zero. It is **not** an AObj layout change — that
family is closed and must not be reopened. It is **not** a cadence change.

**Two DS-side costs the implementation must carry, priced before it starts.**
(1) ROM/RAM: the packed rows must be resident or the parse is replaced by a load,
which is the `K-RAM` problem that already cost a gate run and a soak — the arena
low-water is 51,876 against a 32,768 floor and a 25,600 GObj latch, so the pack
must be sized against that, not against ROM. (2) The rows are read once per joint
per tick and are pure streaming reads: they should be laid out in clip-major,
joint-major order so the D-cache sees sequential lines, because a memo is a memory
stream and this campaign has priced cache-line traffic above arithmetic twice.

---

## 7. The other children, and the owner's literal population

**Status callbacks — 62,155 tk/fr at 2.47x, and this does not reopen Task 108.**
That figure is the *exclusive-among-children static closure* over 513 symbols and
is an upper bound: it contains the fighter's map/ground collision queries
(`mpCollisionGetFCCommonFloor` 10,527, `ndsStageMPSweepFloorLoopSweep` 5,997,
`ndsStageMPAdjustFloorLoopWallSweep` 5,853, `ndsMPFindLineEndpoints` 4,782)
which the interrupt predicates run and which `SPHD` also reaches. Task 108's
result stands verbatim: the *callback-specific* unique closure is 6,373.5 and the
impossible callback + `ftMainSetStatus` + figatree bound is 14,395.5. **What is
inside row 2 and is NOT callback-specific** is the map-collision query lane and
the status-transition tail: `battleship_ftMainSetStatus` 5,481 at **5.29x**,
`gcAddDObjAnimJoint` 4,161 at 4.64x, `ndsRelocAssetIDForToken` 4,228 at **6.22x**
(the K0 line-7 residue, still not fixed and still not priced),
`ndsRelocNormalizeFighterAObj16File` 3,445 at **19.05x**. None of these is ≥16K
alone; they are recorded so the next cycle does not re-derive them.

**Three of eleven source children never engage.** `ftKeyProcessKeyEvents` and
`ftHammerUpdateStats` read **0.00 calls per marginal frame** from their entry
PCs — a measured zero against a table where the same column reads 4.00 for the
root, not an inferred one.

### 7.1 The owner's literal population — run, and it agrees

The c185 DRAW=1 rank-80 frames were carried onto this profile as an explicit
region list (79 regions; frame 756 dropped per §2.2) using
`census-marginal-frame-owners.py --region-list`, added this cycle for exactly
this.

> **The frame→region alignment had to be MEASURED, and the harness banner is off
> by one.** `run-task37-profile-census.ps1` prints *"region r = presented frame
> 438 + r"*. Mapped that way, the c185 rank-80 frames land at **median profile
> rank 454 of 1600** and only 13 of 79 are in the profile's own top-80 — and the
> resulting attribution reads **every** `SITR` row at or *below* its whole-match
> rate (`STATUS_CALLBACKS` 1.03x, `gcPlayDObjAnimJoint` 0.92x), which is the
> signature of a mask anti-correlated with the work it is meant to select. At
> `region = frame − 439` they land at **median rank 43**, mean cost 1.347x the
> match median, and **63 of 79 are in the profile's own top-80**. Lags −1 and +2
> are both as bad as 0, so the alignment is unique, not fitted.

At the measured alignment every row reproduces the primary table within 5% and
the ranking is unchanged:

| row | profile's own top-80 | c185 rank-80 population | Δ |
|---|---:|---:|---:|
| root body | 7,986 | 7,982 | −0.05% |
| `STATUS_CALLBACKS` | 62,155 | 59,820 | −3.8% |
| `ftComputerProcessAll` (SCPU) | 12,435 | 11,814 | −5.0% |
| `ftMainPlayAnim` | 909 | 901 | −0.9% |
| `ftParamUpdateAnimKeys` | 13,796 | 13,674 | −0.9% |
| `ndsR2FtAnimParseDObjFigatree` | 26,368 | 25,530 | −3.2% |
| `gcPlayDObjAnimJoint` | 27,038 | 26,745 | −1.1% |
| `ndsR2AnimValueQ` | 27,457 | 26,992 | −1.7% |
| **PARSE half (the mechanism)** | **41,376** | **40,003** | **−3.3%** |
| EVALUATE half | 53,818 | 53,090 | −1.4% |

**So the mechanism's ceiling is 40,003–41,376 tk/fr on two different definitions
of "the frames that set P95", and the verdict does not depend on which.** The
primary basis stays the profile's own top-80 — same ROM, same run, same
instrument, and it needs no cross-instrument alignment at all.

**The falsifier arm.** `build-c191-sitr-profile-c185` is the same tree one
compile-time flag apart (`NDS_R2_FIGHTER_GX_COMPOSE` 0 vs 1) plus the git string.
Every headline row reproduces within 2%: root body 8,031 vs 7,986, status
callbacks 62,786 vs 62,155, `ftComputerProcessAll` 12,061 vs 12,435, parse root
26,576 vs 26,368, `ndsR2AnimValueQ` 28,303 vs 27,457. **The SITR attribution does
not depend on the GX-compose flag** — which is the expected result for a
simulation bracket and is now measured rather than assumed. As a free byproduct
the pair reprices GX compose on the profile instrument: mean −9,486 tk/fr,
rank-80 −7,939, against the DRAW=1 gate pair's mean −10,737 / rank-80 −17,152.

---

## 8. Reconciliation, and the residual named

```text
measured bracket   SITR = SINT - SCPU      258,196 tk/fr  (c185 DRAW=0, own top-80)
                                           260,354        (c185 DRAW=1, 79 frames)
named children (§4, excluding SCPU)        164,144        63.6%
residual                                    94,052        36.4%
```

The residual is **not** unattributed work; it is work this table refuses to
charge twice. It is, in order of size, the shared leaf pool reached from every
child — `__aeabi_fadd` (2,819 calls/marginal frame across the whole subtree),
`__aeabi_fmul` (3,086), `__aeabi_fcmpeq` (795), `__aeabi_fcmplt` (409),
`ftGetStruct` 9,806 at 264.24 calls/frame, `memcpy`/`memset` — plus the
descendants of rows 2/4/5 that are shared with each other. Each of those symbols
is also called from outside `SITR` entirely, so its self time cannot be assigned
to a child without a per-call-site instrument this task was told not to build.

Two independent consistency checks on the residual's size:
`analyze-subtree-attribution.py`'s SHARED row over the same subtree is **508,694
tk/fr across 310 symbols** — 1.97x the whole bracket — which is the arithmetic
proof that a static exclusivity partition cannot answer this question and that
the call-rate route in §5 was necessary. And the subtree census total, 592,821
tk/fr, is 2.30x the bracket for the same reason: it counts every call of every
reachable symbol, including the ones the root never made.

---

## 9. What this cycle did NOT do

- **No production source touched.** The only tracked source edits are two
  standalone analysis scripts (`analyze-subtree-attribution.py`,
  `census-marginal-frame-owners.py`) that no build imports.
- **No re-bank.** `c185` remains the bank at 1,174,016 raw / 1,149,069 net,
  +28,689 net. No candidate was built, so none was measured.
- **`NDS_TASK108_SITR_CALLBACK_CENSUS` untouched at default 0**, as instructed.
  `NDS_R2_FIGHTER_GX_COMPOSE` untouched at `?= 0` with the published block still
  pinning it; the lab arm used the documented `_LAB` escape.
- **`ftMainProcUpdateInterrupt`, the callback chains, the scheduler, the AObj
  layout and the collision code were not modified**, per the task's own bar.
- **No mechanism implemented.** §6 is a sized, implementation-ready hand-off, not
  a change. No A/B was run against it because no candidate exists yet.
- **The AOT pack's own price was not measured** — only the work it deletes. A
  packed-row read is not free and §6 names both costs it must carry.
- **`gcParseDObjAnimJoint` (4,428 marginal) was left unattributed** rather than
  assigned on a plausible-looking rate match.
- **`decomp/` untouched**; `DECOMP_PRISTINE=PASS` on the Boundary log.
- Root ROMs hashed before the first build and after the last — §11.

---

## 10. Inherited by the next cycle

1. **The animation-representation task (queue item 3) now has its numbers**:
   lane 128,050 marginal-80 / 82,431 whole / 1.55x; deletable parse half **41,376
   / 18,564 / 2.23x** at 96.94 calls per marginal frame; evaluate half 53,819 at
   1.40x, reducible but not deletable.
2. **`SITR` does NOT close as a bracket.** It has a child with a ≥16K
   correctness-preserving mechanism at 2.59x the floor.
3. The `SITR` bracket figure to quote from now on is **258,196 / 260,354**, not
   310,662.
4. `ndsRelocAssetIDForToken` at 4,228 marginal / **6.22x** is `K0_RERANK` §1.3's
   named residue, now sized on the marginal mask for the first time. Still an
   ordering fix, still unpriced, and 0.15x the floor on its own.
5. `census-marginal-frame-owners.py --region-list` and
   `analyze-subtree-attribution.py --child-group / --census-whole / --callers`
   exist now; the last one prints the exact call rate and program-wide caller set
   for every subtree symbol, which is what turned an unusable SHARED row into
   §5's table.

---

## 11. Root ROMs and Boundary

```text
before the first build and after the last, both unchanged:
  smash64ds.nds                        54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
  smash64ds-battle-playable-hwtri.nds  6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99
```

Boundary at the shipping default: see `boundary.log` (§12 of this directory's
files list) — result and `Exception:` scan recorded in the handoff row.

---

## 12. Reproduction

```powershell
# the arm: c185's configuration + the profiler, GX compose via the documented lab escape
.\scripts\run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe `
    -Build build-c192-sitr-profile-gxc -StartFrame 438 -Frames 1600 -PerFrameRegion $true `
    -OutDir artifacts\performance\2026-08-15_sitr-direct-children\v3-c192 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1,`
NDS_R2_FIGHTER_GX_COMPOSE_LAB=1,NDS_TICK_HUD_DRAW=0

python scripts/census-marginal-frame-owners.py --reduce --profile <v3-c192> --out c192-pc.csv --marginal 80
python scripts/census-marginal-frame-owners.py --report --pc-csv c192-pc.csv `
    --build builds/build-c192-sitr-profile-gxc --census-out c192-marginal-census.json
python scripts/analyze-subtree-attribution.py --dis c192.dis `
    --census c192-marginal-census.json --census-whole <v3-c192>/census.json `
    --root battleship_ftMainProcUpdateInterrupt `
    --child-group "STATUS_CALLBACKS=$(cat callback-targets-60.txt)" `
    --child-group "ANIM=ftMainPlayAnim" `
    --child-group "MOTION_EVENTS=ftMainUpdateMotionEventsAll" `
    --child-group "COLANIM=ftMainUpdateColAnim,ftParamResetStatUpdateColAnim" `
    --frames 80 --whole-frames 1601 --callers 900 --pc-csv c192-pc.csv
python scripts/census-marginal-frame-owners.py --concentration --pc-csv c192-pc.csv `
    --build builds/build-c192-sitr-profile-gxc --symbols @sitr-chain-symbols.txt

# the owner's literal population, as a cross-check
python scripts/census-marginal-frame-owners.py --reduce --profile <v3-c192> `
    --out c192-c185mask-pc.csv --region-list @c185-rank80-regions.txt
```
