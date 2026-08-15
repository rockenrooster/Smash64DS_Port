# The ring's null is I-CACHE, measured — and the resident version does not clear the bar either

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `ca4291cbb7f`.
Premise: `…/2026-08-15_cfx-ring-wiring/RING.md` §4, which named two candidates
and measured neither.

## 0. Outcome first

**Candidate 2 (the I-cache footprint of 5,596 bytes of new ARM text that now
executes) owns the null, and candidate 1 (the f32 boundary) does not — because
the boundary's ARITHMETIC is free and only its BYTES cost anything.**

Whole window, arm B (dispatch 1) minus arm A′ (dispatch 0), 1,601 regions,
ROMs whose `.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/`.dtcm` are **0
differing bytes** and whose `.main.rw` differs in **exactly 1 byte** (§4):

```text
                    A' c178 (off)    B c177 (ON)       B - A'   ticks/frame
instructions        1,061,301,485  1,060,649,389     -652,096          -204
stall_issue           691,990,538    686,493,942   -5,496,596        -1,717
stall_icache_fill   1,176,682,184  1,182,617,802   +5,935,618        +1,854
stall_dcache_fill     860,594,923    861,109,464     +514,541          +161
stall_write_buffer    163,271,642    163,375,074     +103,432           +32
stall_interlock       138,761,322    138,814,034      +52,712           +16
stall_bus_contention  118,189,962    117,990,450     -199,512           -62
                                                                  --------
non-idle (total_cycles - halt_wait)              +910,301 cyc         +284
```

The six stall classes sum to `+284` exactly, which is the non-idle delta:
`stall_partition_residual` is **0** on A′ and **−42 of 3.87e9** on B.

**The arithmetic win is real and it is 1.08x cancelled by fetch.** `issue`
**−1,717 tk/fr**; `icache_fill` **+1,854 tk/fr**. And the ring executes **204
fewer instructions per frame** than the float path it displaces — the fixed
kernels are cheaper to run and more expensive to fetch.

**And the resident version (§5) is sized on that same measurement: −6,261 tk/fr
whole match, which at this cycle's measured 3.11x percentile concentration is
+19,470 at rank-80 = 0.60x of +32,593. It clears at a concentration of 5.21x and
not below**, and 5.2–11.7x is exactly the presence range
`…/2026-08-15_k1-owner-pricing/` §5 reports for these bodies on a clean `DRAW=0`
mask. So the bar is straddled, the deciding quantity is a concentration factor
nobody has measured cleanly, and **one `DRAW=0` re-run of this same byte-identical
pair settles it with no new code.**

## 1. Where each class comes from — the group decomposition

Per-PC, joined ON THE PROGRAM COUNTER (legitimate only because the two arms have
identical layout, §4), attributed by `nm` ranges. Ticks/frame over 1,601 regions.

```text
    total     issue    icache    dcache      instr   group (B - A')
    2,555       +12    +2,110      +345     +1,508   ring kernels: 8 new symbols, 5,596 B ARM
      588       -12      +516       +57       +383     of which the two f32 boundary kernels (964 B)
    1,967       +23    +1,594      +288     +1,125     of which the six fixed-arithmetic kernels (4,632 B)
   -1,011      -237      -392      -246       -426   decomp float producers displaced
   -1,697    -1,615       -83         0     -1,488   soft-float library
      437      +123      +219       +62       +202   everything else (eviction + HUD/printf noise)
    2,515         0         0         0          0   armWaitForIrq (idle)
    2,799    -1,717    +1,854      +161       -204   == ALL SYMBOLS ==
```

Read it in three lines:

1. **The win is the soft-float library disappearing: `issue` −1,615 tk/fr**, plus
   −237 from the float bodies' own code. Total `issue` won: **−1,852**.
2. **The ring's own eight symbols add `issue` +12 tk/fr.** Twelve. The fixed
   arithmetic — six int64 kernels, ~1.5 calls/frame each — computes for free.
3. **The ring's own symbols add `icache_fill` +2,110 tk/fr.** The displaced float
   bodies and their library give back only −475. The rest of the binary nets
   **+219**, so this is **not** eviction damage to neighbours: it is
   **compulsory fetch of the ring's own instructions**.

**Candidate 1 is refuted as an arithmetic explanation and re-cast as a footprint
one.** `StoreF32` + `LoadF32` are 964 B and cost **+588 tk/fr**, of which
**+516 (88%) is `icache_fill`** and **−12 is `issue`**. RING.md §4's reasoning
("~90,000 conversions at ~11 instructions each") was two errors that cancel into
a wrong conclusion: the static size 516 B ÷ 12 values is **not** the per-value
instruction count (the body is two loops; the common path is **38 ARM
instructions per fixed→f32 value** and **22 per f32→fixed**, read off the
disassembly at `0208b694`/`0208b4d4`), and the cost that matters is not the
instructions at all.

## 2. The rate, and R2-07 L7's constant is 2.45x too high here

The ring's own `icache_fill` is **+2,110 tk/fr = 4,220 cycles/frame** for
**5,596 B** of added executing ARM text:

```text
0.754 cycles/frame per byte      measured here, whole match
1.85  cycles/frame per byte      R2-07 L7, 2,332 B (nds_r2_collision_mtx.h)
```

RING.md predicted ~5,175 tk/fr from L7's rate and the truth is **1,854 net /
2,110 gross**. **The byte-rate is not a constant** — it depends on how often the
bytes are entered and what else is resident, the same way
`fifo-word-price-depends-on-queue` records for FIFO writes. Do not carry 1.85
into another sizing; carry the method.

At the percentile the same quantities scale by ~3.1–3.2x (§3), i.e. about
**1.2 ticks/frame per byte at rank-80**.

## 3. What the ring actually intercepted — exact call counts, free

Entry-PC executions from the reduced per-PC CSVs (`entry-pc-gives-exact-call-counts`),
1,600-frame window, both arms:

```text
symbol                                     A' (float)   B (ring)   ring took
func_ovl2_800ED490          compose             2,471      1,177     1,294  52%
gmCollisionTransformMatrixAll local            4,157      3,115     1,042  25%
gmCollisionSetInvertMatrix  inverse               937        141       796  85%
func_ovl2_800EDBA4          chain              1,133        337       796  70%
func_ovl2_800EDE5C          axis scales        1,693      1,693         0  early-returns
gmCollisionTestRectangle    decision           1,693      1,693         0  by design
gmCollisionGetWorldPosition point x mtx        2,620      2,620         0  by design
lbCommonSin / lbCommonCos                     37,615     34,489     3,126   8.3%
sqrtf                                         68,289     65,900     2,389   3.5%
ndsR2CfxPrepareFighterJoint                        0      1,552
```

Three things this kills.

- **`lbCommonSin`/`Cos` were never a collision lane.** 37,615 calls; the ring
  removed 8.3%. `PREDICTION.md` §2 booked ~4,975 tk/fr for them.
- **`gmCollisionTransformMatrixAll` keeps three quarters of its calls** — the
  ring reaches it only through the damage-phase seam; `ftParamSetAnimLocks` and
  `gmCollisionGetFighterPartsWorldPosition` hold the rest.
- **`func_ovl2_800EDE5C` still runs 1,693 times in arm B** and costs 22 tk/fr:
  the ring set `unk_dobjtrans_0x6`, so it early-returns. The mechanism works
  exactly as designed; there is simply nothing left in it.

Percentile concentration, measured rather than assumed: the marginal-mask
(80 most expensive frames, each arm its own) deltas are `issue` **−5,767** and
`icache_fill` **+5,963** against whole-match −1,717 / +1,854, i.e. **3.11x and
3.22x**. Both classes concentrate on the same frames.

**Caveat on the marginal mask, stated rather than buried.** These arms are
`NDS_TICK_HUD_DRAW=1` (matching the c175/c176 pair that produced the null), so
their 80 most expensive frames include tick-HUD print bursts —
`ndsPlatformRenderDebugHud` +1,869, `_svfiprintf_r` +587, `consolePrintChar`
+541 appear in the marginal diff. **Every headline number in §0–§2 is
whole-match and unaffected.** Percentile figures below carry the 3.1x factor
measured here and are labelled where they do.

## 4. The instrument — placement floor ZERO, verified, and one trap re-caught

`arm-none-eabi-objcopy -O binary --only-section=<s>` on both profile ELFs,
byte-compared with `cmp -l`:

```text
.itcm            32,152 B    differing bytes 0
.text.hot         4,588 B    differing bytes 0
.text.hot.draw    5,268 B    differing bytes 0
.main           932,960 B    differing bytes 0
.main.rw        137,428 B    differing bytes 1   at 0x3F24, 1 -> 0
.dtcm             8,800 B    differing bytes 0
```

`0x3F24` is the same offset RING.md §5 found on the c175/c176 pair, and `nm`
puts `gNdsCfxRingEnable` there. **The two profile ROMs differ in exactly the
dispatch switch.**

**The trap, re-caught one cycle after RING.md recorded it.** The first run of
this comparison pointed at a build directory that did not exist (a
`DEVKITARM is not set` failure had produced no ELF), and `objcopy` on a missing
file left a stale/absent output whose `cmp` reported **0 differing bytes for
every section**. That is RING.md §3's `--only-section=.text`/empty-string result
in a second costume: **a byte comparison that cannot fail is not a control.**
The loop above now fails closed on an `objcopy` error and on a zero-length
section, and `census-marginal-frame-owners.py --diff` refuses outright when the
two builds' `nm` address→name maps differ (§7).

Arms:

| arm | build | dispatch | out |
|---|---|---|---|
| B | `build-c177-cfxring-b-prof` | 1 | `v3-b-c177/`, `b-c177-pc.csv` |
| A′ | `build-c178-cfxring-a2-prof` | 0 | `v3-a2-c178/`, `a2-c178-pc.csv` |

Both: `NDS_TASK37_PROFILE=1 START=438 FRAMES=1600 PER_FRAME_REGION=1`,
`NDS_R2_BOTH_CPU=1`, `NDS_R2_BATTLEPACK=1`, `NDS_R2_BATTLEPACK_KEEP_CACHE=1`,
`NDS_R2_COLLISION_FIXED=1`, `NDS_TICK_HUD_DRAW` default (1), DLDI on,
`emulators/melonds-attributor/melonDS.exe`, no `-RunnerSlot`.
Format `melonDS-arm9-retail-profile-v3` verified in both meta files, with
`icache_fill` and `halt_wait` present — the columns a v2 capture lacks.

**The brief said this run needed no rebuild. It did.** The census window is a
compile-time constant (`run-task37-profile-census.ps1:130`) and `c175`/`c176`
carry `NDS_TASK37_PROFILE 0`, so `Assert-Task37CensusWindow` throws on them.
Two builds were spent; both are lab targets, neither root ROM was rebuilt.

**Instrument levels are not comparable to the tick-HUD arms.** On this
instrument A′/B read P50 955,553/955,738, rank-80 1,363,110/1,359,694 on
`total_cycles − halt_wait`, against c176/c175's WORK-H 942,336/942,400 and
1,173,696/1,177,344. Different metric, plus the profiler's own per-frame CP15
write. Only the **B − A′ delta** is carried out of here, and the whole-match
mean delta **+284 tk/fr** is the robust form of it (a sum over 1,600 frames, not
an order statistic). `timestamp_discontinuities` is 0 on A′ and 1 on B.

## 5. Task B — the resident-representation version, sized. It does not clear +32,593

Design only. Not implemented, not built.

### 5.1 Every f32 crossing in the current chain, and what residency removes

Per prepared joint, from `src/port/nds_r2_collision_ring.c`:

```text
  chain boundary          LoadF32(mtx_translate)            12 f32 -> fixed
  per level, cache miss   BuildLocal reads DObj TRS           6 f32 -> fixed   <- a TRUE boundary
                          StoreF32(unk_dobjtrans_0x10)      12 fixed -> f32
  per level, cache hit    LoadF32(unk_dobjtrans_0x10)       12 f32 -> fixed
  per level               StoreF32(mtx_translate)           12 fixed -> f32
  invert                  InvertF32 reads mtx_translate     12 f32 -> fixed
                          InvertF32 writes 0x9C             16 fixed -> f32
  axis scales             AxisScalesF32 reads mtx_translate 12 f32 -> fixed
                          AxisScalesF32 writes vec_scale     3 fixed -> f32   <- a TRUE boundary
```

Measured call counts (§3) give **75,856 conversions in the 1,600-frame window**,
47.4/frame. A resident representation — fixed `mtx_translate`, fixed
`unk_dobjtrans_0x10`, fixed `unk_dobjtrans_0x9C`, `vec_scale` left f32 — removes
every line above except the two marked, which is the header's own
"exactly twice per joint per frame".

**But §1 already priced that saving and it is small.** The two boundary kernels
are +588 tk/fr whole match, and deleting them recovers 964 B of the 5,596. The
residency argument cannot be made on conversion count; it has to be made on what
residency *unlocks* — `TestRectangle`, `GetWorldPosition`,
`GetFighterPartsWorldPosition` and the un-intercepted 75% of `TransformMatrixAll`.

### 5.2 The exchange rate, measured — it is 0.987

The single number that explains the null, whole match, this pair:

```text
  +2,726 tk/fr   the ring's own execution (8 symbols + __udivmoddi4, 5,596 B)
  -2,762 tk/fr   the float it deleted (bodies + every libgcc float/sqrt symbol)
    +321 tk/fr   everything else (eviction + HUD/printf instrument noise)
  --------
    +284 tk/fr   net non-idle
```

**The fixed replacement costs 0.987x what the float it deletes cost.**
Break-even to 1.3%. And the two rates that generalise:

```text
0.4871 tk/fr per byte    cost of executing fixed ARM text (2,726 / 5,596 B)
0.4937 tk/fr per byte    float deleted per byte of fixed text (2,762 / 5,596 B)
```

A resident version wins only by moving the SECOND number — deleting more float
per byte of kernel — because the first is a property of the machine.

### 5.3 The prize, measured on the float control

Self time of the float narrow phase on arm A′, whole match, /3,202:

```text
1,091 lbCommonCos     655 gmCollisionTransformMatrixAll   251 func_ovl2_800EDBA4
1,051 lbCommonSin     529 func_ovl2_800ED490              180 gmCollisionTestRectangle
  519 gmCollisionGetFighterPartsWorldPosition             149 gmCollisionGetWorldPosition
  219 gmCollisionSetInvertMatrix    63 func_ovl2_800EDE5C   9 func_ovl2_800EDA0C   1 CopyMatrix
-----
4,717  self
```

plus its soft-float leaves (`analyze-leaf-helper-attribution.py --mask all`):

```text
1,449 func_ovl2_800ED490   1,211 lbCommonCos    796 gmCollisionGetFighterPartsWorldPosition
  789 lbCommonSin            731 gmCollisionTransformMatrixAll   526 gmCollisionSetInvertMatrix
  450 gmCollisionGetWorldPosition   388 gmCollisionTestRectangle  130 func_ovl2_800EDE5C
   74 ranges / EDA0C / EF5D4 / damage position
-----
6,544  soft-float leaves
```

`lbCommonSin`/`Cos` are only **66.3%** collision-driven (4,157
`TransformMatrixAll` calls x 6 against 37,615 calls), so 33.7% of their
4,142 tk/fr stays whatever happens:

```text
11,261  self + leaves
-1,396  sin/cos not driven by the collision cluster
=======
 9,865  tk/fr whole match -- the ENTIRE prize of a complete narrow-phase conversion
-2,762  already collected by the wired ring
=======
 7,103  tk/fr still on the table
```

### 5.4 The text, priced against §5.2's rate rather than guessed

```text
  +  fixed TestRectangle (float body 1,680 B Thumb)                ~1,400 B ARM
  +  fixed TransformPoint / GetWorldPosition                         ~300 B
  +  fixed GetFighterPartsWorldPosition                              ~400 B
  +  TestSphere fixed->f32 expansion prologue (never executes, P1)   ~600 B
  +  f32 views for func_ovl2_800EDA0C and the renderer adapter       ~300 B
  -  StoreF32 + LoadF32 retire (residency's whole point)             -964 B
  =  total executing ARM text                                      ~7,400 B
```

At 0.4871 tk/fr per byte: **~3,604 tk/fr of cost** against **9,865 tk/fr
removed**.

### 5.5 The prediction, labelled as one

> **PREDICTION.** A resident fixed representation across the whole fighter
> narrow phase is worth **−6,261 tk/fr whole match**, which at this cycle's
> **measured** percentile concentration of **3.11x** is **+19,470 at rank-80 —
> 0.60x of the +32,593 requirement. It does not clear.**
>
> **It clears at a concentration of 5.21x and not below.**
> `…/2026-08-15_k1-owner-pricing/` §5 reports 5.2–11.7x *presence* for exactly
> these bodies on a `DRAW=0` mask, so the bar is genuinely straddled and the
> decisive quantity is a number nobody has measured cleanly: **the narrow
> phase's cost concentration on an uncontaminated P95 mask.** This cycle's 3.11x
> is measured on `DRAW=1` frames whose top-80 includes tick-HUD print bursts
> (§3), which dilutes it in an unknown direction.
>
> Basis: §5.3's 9,865 tk/fr removed, §5.4's 7,400 B at §5.2's measured
> 0.4871 tk/fr per byte, and the `issue`/`icache_fill` marginal-to-whole ratios
> 3.11x/3.22x measured in §3. Both the saving and the cost concentrate on the
> same frames, so the ratio is what decides, not the level.
>
> **The cheapest way to settle it costs one capture and no new code:** re-run
> this exact A′/B pair with `NDS_TICK_HUD_DRAW=0` and read the concentration off
> the same `--diff`. If it is ≥5.21x the resident version is worth building; if
> it is 3.1x it is not, and the lane closes on arithmetic that was never there.

**And one lever inside this is NOT priced and could change the answer on its
own: placement.** Every measured byte of this cycle's fetch cost is in `.main`.
`.text.hot` has **3,604 B free** of its 8,192 cap and `.text.hot.draw` 2,924 B.
Moving the six fixed-arithmetic kernels (4,632 B) into a hot section is a
*placement* experiment against a *measured* +2,110 tk/fr of compulsory fetch,
with no new arithmetic and no new correctness obligation — one build, one A/B.
It is not sized here because nothing in this campaign has measured the hot
sections' own eviction behaviour under a 4.6 KB addition.

**The one lever inside this that is NOT priced and could change the answer:
placement.** Every measured byte of this cycle's icache cost is `.main`. The
`.text.hot` cap has **3,604 B free** and `.text.hot.draw` **2,924 B**. Moving
the six fixed-arithmetic kernels (4,632 B) into a hot section is a *placement*
experiment against a *measured* +2,110 tk/fr of compulsory fetch, and it is the
cheapest remaining test of this whole lane — one build, one A/B, no new
arithmetic and no new correctness obligation. It is not sized here because
nothing in this campaign has measured the hot sections' own eviction behaviour
under a 4.6 KB addition.

### 5.6 "Fifteen referrers" — the number is real but it is attached to the wrong symbol

`plan.md` §K-PACKAGE/§K-RING and the board both read "a fixed `mtx_translate`
(fifteen referrers)". The source is
`…/2026-08-13_c-collision-seam/elf-referrers.txt`, and the fifteen belong to
**`func_ovl2_800EDBA4`**, not to the field:

```text
INSIDE the cluster or its entries (10) -- these call the FUNCTION and are
  unaffected by the field's representation:
    func_ovl2_800EDE5C, func_ovl2_800EE018,
    the eight gmCollisionCheck{Fighter,Weapon,Item}Attack* entries
OUTSIDE (5) -- these call it and then READ mtx_translate:
    battleship_ftMainProcParams            6,232 calls/window
    func_ovl0_800C9A38 (lbcommon 0x4F)        29
    gmCollisionGetFighterPartsWorldPosition 1,018
    ndsBaseFTComputerSetFighterDamageDetectSize   0 in this matchup
    ndsRendererAdapterBuildDObjXObjMatrix  88,275 (EDBA4 itself runs 1,133)
```

**Fifteen callers of a function do not block a representation change; they need
the function to keep its contract.** What matters is readers of the *field*, and
those are 19 decomp sites plus ~30 port sites — of which **~30 funnel through
one helper, `gmCollisionGetWorldPosition`** (nine referrers in the ELF census),
and `gmCollisionCopyMatrix` and `func_ovl2_800EDA0C` take most of the rest.
Classified:

| referrer class | sites | verdict |
|---|---|---|
| `gmCollisionGetWorldPosition(parts->mtx_translate, v)` | ~30 | **convertible in one helper** — 18 soft-float ops per call become 9 int64 muls + 6 conversions |
| `gmCollisionCopyMatrix` / `func_ovl0_800C994C` (kind 0x4F) | 2 | **convertible and a NET WIN** — the renderer converts `mtx_translate` to 20.12 fixed anyway (`ndsRendererAdapterF2LFixedWExact`, `syMatrixF2LFixedW`); the ring's translation row is already Q12 |
| `func_ovl2_800EDA0C` (Euler extraction, 29 calls/window) | 2 | **needs an f32 view** — normalise + `atan2` chain, new arithmetic for 0.02 calls/frame; expand and run the float body |
| `ftmain.c:4031` afterimage, `lbcommon.c:1955/2018` row-0 magnitude and sign | 3 | **convertible** — 6 conversions, or a sign test on a Q26 int |
| `reloc_backend_fighter_model.c:1691-1717` (`ndsFighterPartsSetIdentity`) | 1 | **inert** — absent from the linked ELF, re-verified this cycle on `build-c175-cfxring-b` |
| `battleship_gmcollision.c:310/365` (L7 oracle) | 2 | **inert** — inside `NDS_R2_COLLISION_L7_ORACLE`, off |

**Nothing in that table blocks.** The referrer count was never the obstacle; the
fetch cost is.

### 5.7 The correctness obligation, and how it would be graded

Reinterpreting `unk_dobjtrans_0x9C` re-opens `STACK.md` §5.1: `gmCollisionTestSphere`
writes `*p_angle` (a shield hit's knockback angle) and a normal — continuous
gameplay values that **no flip count can express**. The grading that works:

1. **Do not convert its arithmetic.** Give it a prologue that expands the fixed
   frame back into a local `Mtx44f` (28 conversions) and run the decomp body
   byte-for-byte from there. The obligation becomes **grade the expansion**, not
   the body — exactly the shape the T8 wiring falsifier already grades
   (`world 0.0022583 / local 0.0014343 at depth 12 against 0.0200`).
2. **State its reachability, from the ELF and the census, not from grep.**
   `gmCollisionTestSphere` is present (`0x0207fe58`, 0x4ac bytes) and executes
   **zero** times in this matchup, on both arms of this cycle. Under P1 the
   expansion is unreachable code and costs nothing but bytes.
3. **`gmCollisionTestRectangle` keeps a flip count**, because its output IS a
   boolean: the existing falsifier's 300,000 randomised cases at the live domain
   0.9937–2.0479, 0 mismatches, stands and would be re-run at the widened
   consumer domain.
4. **The end-of-match invariant pair stays the stop rule.** This cycle's arms
   reproduce it: `gmCollisionCheckFighterAttackDamageCollide` executes **1,552**
   times on BOTH arms (entry-PC, §3), and `gmCollisionTestRectangle` **1,693** on
   both — same fight, same decisions, on the byte-identical pair.

## 6. Task C — the 2^22 outliers: bounded negative evidence, free

The board records "every one is 2^22 ticks (4,194,304) in whichever single
bucket was open … ~1 per 2,100 presented frames … `ALL` rises with it, so the
guest timer really advanced".

**Two v3 captures over the gate arm's own 1,600-frame window, DLDI on, contain
zero such frames.** Per-region `total_cycles`:

```text
arm      regions   max total_cycles      regions >= 2^22 ticks   stall_cart_spin
A' c178    1,600      3,361,733 ticks                        0                 0
B  c177    1,600      3,361,633 ticks                        0                 0
```

Both maxima are the same region (1558) and 20% below the threshold. The v3
attributor accounts for **every emulated cycle** (`stall_partition_residual` 0 /
−42), so a guest-side stall of 0.1252 s would have to appear here as cycles in
some class, and `cart_spin` — the class a cartridge/DLDI wait lands in — is
**0 for the entire window on both arms**.

Expectation at the board's rate over 3,200 frames is ~1.5 events; observing 0 is
**not** proof of absence. What it does say: the phenomenon is **not visible to
the instrument that counts emulated cycles**, on the same configuration and
match, which moves the search toward the tick-HUD's own reader
(`cpuGetTiming()`, `nds_platform.c:353` — libnds's cascaded TIMER0/TIMER1 pair,
read non-atomically) and away from guest execution. That is a narrowing, not an
answer, and it cost nothing. The handed-forward `-PerStopGlobals` probe was
**not** run.

**The +23,040 the counter arm reads over the `c170` bank was not investigated.**

## 7. Tooling — one trap made structural, one that cannot be

**Made structural.** `census-marginal-frame-owners.py --diff` is new. A stall
partition is only readable as a *difference*: one capture says a function costs
N cycles of `icache_fill`, and only a pair can say whether a switch MOVED work
from `issue` into `icache_fill`. The mode joins two reduced per-PC CSVs on the
program counter, which is valid **only** across identical layouts, so it
verifies that itself from the two ELFs' `nm` address→name maps and refuses
otherwise (`assert_same_layout`). A relinked pair can no longer be diffed by
accident; the wrong form is inexpressible rather than documented.

**Cannot be made structural by content inspection**, and this is the answer to
the `docs/VERIFYING.md:31` question. The 2026-08-15 Makefile damage was an
**eaten `\` continuation**, which leaves a *syntactically valid* file: there is
no residue in the bytes for a checker to find, so no grep, no line-ending rule
and no escape scan can catch the class. The only gate that can is one that
**parses the result**: `make --dry-run --no-print-directory <target>` fails on a
broken recipe continuation in seconds without building anything.
`scripts/check-architecture.ps1` is the owning checker (it already sweeps the
tracked tree and is registry-wired). **It was NOT added this cycle** — a new
failure mode in a checker that gates Boundary is not something to land in a
cycle whose deliverable is a measurement. One actionable line is recorded in
`docs/VERIFYING.md` naming the command and the owner.

## 8. What this cycle did NOT do

- **No production change.** No flag flipped, no default moved, no arithmetic
  edited. `NDS_R2_COLLISION_FIXED` is still `?= 0`.
- **No resident implementation.** §5 is a sizing, and its verdict is negative.
- **No placement experiment** (§5.5 names it as the cheapest remaining test),
  and **no clean `DRAW=0` concentration capture** (§5.5 names that as the one
  measurement that decides the resident version).
- **No Boundary run.** The only tracked source edit is a new `--diff` mode in a
  standalone analysis script that no build, ROM or verifier imports; the two
  builds are lab targets.
- **No stress battery** — gated on a KEEP, and nothing was kept.
- **Task C's `-PerStopGlobals` probe and the +23,040 residual** were not run.

## 9. Root ROMs

Hashed before the first build and after the last, unchanged, and neither was
rebuilt (mtimes 2026-08-09 and 2026-08-14):

```text
smash64ds.nds                       54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A
smash64ds-battle-playable-hwtri.nds 2015FBD1F68B81C03626D8C6D473C8BCBCF527A3A26DFE86FF19BD74ECBB1360
```

## 10. Reproduction

```powershell
# arm B (dispatch 1). ~9 min build+capture, writes 3.6 GiB, not committed
.\scripts\run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe `
    -Build build-c177-cfxring-b-prof -StartFrame 438 -Frames 1600 -TimeoutSeconds 7200 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1,NDS_R2_COLLISION_FIXED=1,NDS_R2_COLLISION_FIXED_DISPATCH=1 `
    -OutDir artifacts/performance/2026-08-15_cfx-ring-split/v3-b-c177

# arm A' -- same flags, DISPATCH=0. Build first, then capture with -NoBuild.
# (-MelonDS and -RunnerSlot together throw; never pass both.)

python scripts/census-marginal-frame-owners.py --reduce --marginal 80 `
    --profile artifacts/performance/2026-08-15_cfx-ring-split/v3-b-c177 `
    --out artifacts/performance/2026-08-15_cfx-ring-split/b-c177-pc.csv
# ... same for A'

python scripts/census-marginal-frame-owners.py --diff --top 30 `
    --pc-csv     artifacts/performance/2026-08-15_cfx-ring-split/b-c177-pc.csv `
    --pc-csv-base artifacts/performance/2026-08-15_cfx-ring-split/a2-c178-pc.csv `
    --build      builds/build-c177-cfxring-b-prof `
    --build-base builds/build-c178-cfxring-a2-prof
```

Committed beside this file: `diff-b-minus-a2.txt` (the table in §1),
`a2-c178-softfloat-callers-all.txt` and `…-callers.txt` (§5.2),
`b-c177-pc.csv` / `a2-c178-pc.csv` and their meta (the reduced per-PC censuses,
so nobody re-scans 7.3 GiB), and both `arm9-profile.meta.txt` /
`arm9-profile.regions.csv`. The two 3.6 GiB profiler CSVs are **not** committed.
