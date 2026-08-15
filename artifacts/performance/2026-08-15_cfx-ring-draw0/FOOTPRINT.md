# The ring's fetch is charged PER ENTRY, the entries concentrate 11.6x, and placement cannot reach either

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `bccd70d6bda`.
Premise: `../2026-08-15_cfx-ring-split/SPLIT.md` §5.5, which named two unpriced
levers — placement, and a clean `DRAW=0` concentration capture.
**UNITS: 2 profile cycles = 1 project tick.**

## 0. Outcome first

1. **Placement is REFUTED, without a build.** The ring's cost tracks its *entry
   count* to two decimal places across an **11.6x** range of entry density, and
   its per-entry price moves **+11%** over that range — *upward*. Nothing
   survives between entries at any address, and there is no eviction surplus for
   a layout to recover: the ring's damage to the rest of the binary is
   **+219 tk/fr whole match**, 8.0% of what it costs itself.
2. **The clean `DRAW=0` concentration is measured and it is 11.68–11.83x**, not
   the 3.11x the `DRAW=1` mask reported. **2.42–2.48x of that gap is tick-HUD
   dilution**, measured two independent ways on the float control (cost and
   calls). Answering the brief exactly as asked: **11.7x, and 5.21x is cleared
   by 2.2x.**
3. **And clearing it changes nothing, because 5.21x tested the wrong quantity.**
   That bar applied ONE concentration factor to the NET, which is valid only if
   the price stays put while the prize concentrates. Measured, **the float the
   ring deletes concentrates 11.68x and the fixed text that replaces it
   concentrates 11.83x** — the same frames, the same factor. So the factor
   cancels and the **EXCHANGE RATE** decides: measured **1.001 whole match and
   1.014 at rank-80**. The wired ring is exactly break-even at every percentile
   (**+3 tk/fr whole match, +441 at rank-80**), and residency extends that same
   1.00 rate to more bodies. **`5.21x` is RETRACTED as a decision rule.**
4. **The rate moves on BYTES, and that move is measured free.** Recompiling
   `src/port/nds_r2_collision_fixed.c` at `-Os` instead of `-O2` (same `-marm`,
   same TU) takes its ten entry points from **7,916 B to 5,228 B, −34.0%**, with
   SMULL/SMLAL preserved and no `__aeabi_lmul` introduced. That is the only
   measured way found to move a 1.00 exchange rate — and standalone it is
   **~23x under `plan.md` §2's ≥16,000 build floor**, so it must ride a package.
5. **`+284` is RETRACTED**: the wired ring's whole-match cost on the arm the
   cadence gate is read from is **+3 tk/fr**, not +284. The difference was
   HUD/printf noise the `DRAW=1` pair could not separate.

**Section-compare on the new pair: `.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/
`.dtcm` 0 differing bytes, `.main.rw` exactly 1 at `0x3F24` = `gNdsCfxRingEnable`
(`nm`: `0x020eb384`, `.main.rw` VMA `0x020e7460`). The placement floor is zero on
this pair too**, now asserted by a script that refuses every way the comparison
can be vacuous rather than by a hand loop (§6).

---

## 1. Task A — placement, refuted before a build was spent

The brief asked for one build moving 5,596 B of ring text out of `.main` into
`.text.hot` (3,604 B free) and `.text.hot.draw` (2,924 B free), against the
measured +2,110 tk/fr of compulsory fetch. It also asked for the prior closure
to be read first. Read, and the reading is that this differs from `plan.md` §3's
caveat in the direction that kills it: **§3's caveat rescues a lane that DELETES
A LAYER. Moving bytes to a different address is the micro-optimization the
caveat explicitly still forbids.** Five independent reasons, in cost order:

### 1.1 The repository already measured this experiment, twice, and both signs were wrong

`linker/nds_hot_text.ld:180-200` is not a comment, it is a bank:

| experiment | prediction | measured |
|---|---|---|
| Task 94 — move a **500 B** member OUT of `.text.hot` (top-ranked Task 81 candidate, 720 B free, zero eviction) | a win | **WORK-H P50 +6,144** on 122 of 128 frames; `STG` rose 3,712 *without ever calling it* |
| R2-03 E65/E66 — admit a **2,032 B** callee immediately after its only caller (#1 unplaced candidate on the Task 37 census, 1,815,752 recoverable stall cycles ≈ 7,093 tk/fr) | −7,894 | **WORK-H P95 +24,448**, P90 +8,000, over-gate 7→8, worse on 103 of 128 frames, median +2,624 |

Two *different* estimators — tier cycles/instruction, and per-symbol recoverable
stall — got the **sign** wrong on this list. The in-tree verdict is *"treat
`.text.hot` as closed in both directions"*, and `docs/HANDOFF.md:141` repeats it.
Task A proposes the same experiment at **2.8x E65's byte count**, sized from a
third stall census.

### 1.2 The mechanism was measured a year-fraction ago and it is capacity, not conflict

`../2026-08-14_icache-temporal/ICACHE_TEMPORAL.md` §6 swept the hot-line cutoff
so set population actually varies: at top-256, sets that FIT inside the 4 ways
refill at **1,252** fill-cycles per 1k instructions while oversubscribed sets
refill at **1,233**. Uncontended sets are marginally *worse*. Placement changes
which lines conflict; measurement says conflict is not what is being paid.

### 1.3 A contiguous 5,596 B block has the same set profile at every address

The I-cache index is `(addr >> 5) & 63` (verified from the reference emulator,
`ICACHE_TEMPORAL` §1), so the set period is **2,048 B**. 5,596 B is **2.73 set
periods**: the block occupies every one of the 64 sets at least twice under
*any* base address. Translation permutes which line lands in which set; it
cannot change the block's per-set occupancy. There is no quieter set to move to.

### 1.4 The cache turns over 55–121 times per presented frame

`c179` whole match, `stall_icache_fill` 1,141,316,610 cycles / 1,601 regions =
**712,877 cycles/frame** of instruction fetch. A 32-byte main-RAM line fill costs
between **23 and 51 ARM9 cycles** — `melonDS-Accurate/src/CP15.cpp:530-545`
computes `ns = MemTimings[1] + stall` and `seq = MemTimings[2] + 1`, and
`NDS.cpp:258` sets main RAM to a 16-bit bus at N=8/S=1, which with
`ARM9ClockShift` 1 gives `ns` 23 and `seq` 4, so a whole line streams in
`23 + 7x4 = 51`. That is **13,978–30,995 line fills per frame against a 256-line
cache: 55 to 121 complete turnovers per presented frame.** At 10.66 narrow-phase
entries per marginal frame there are **5 to 11 complete cache turnovers between
consecutive entries.** No address survives that.

### 1.5 The decisive one: the ring's price is per-ENTRY and it does not fall when entries get denser

This is the measurement the previous cycles did not have. `--concentration`
(§6) on the ring's eight symbols, both arms of both pairs:

```text
                             entries/frame   ring tk/fr   ticks per ENTRY
c177 DRAW=1, whole match             0.97        2,587             2,668
c177 DRAW=1, marginal 80             4.14       10,740             2,596
c179 DRAW=0, whole match             0.97        2,671             2,755
c179 DRAW=0, marginal 80            10.66       31,574             2,961
```

**An 11.0x range of entry density moves the per-entry price by +11%, and it
moves UP.** If any of the ring's 175 lines survived between two entries, packing
the entries 11x closer would make the per-entry price FALL. It does not.
Every entry pays a full re-fetch already, which is the floor; a layout change
cannot reduce how much other code runs between entries.

And the cost/call correspondence is not approximate. On the `DRAW=0` mask the
ring's **cost concentration is 11.82x and its call concentration 11.57x**; on
the `DRAW=1` mask **4.15x and 4.16x**. The ring's cost *is* its call count times
its byte count, twice over, on two different masks.

### 1.6 The ceiling, so the refusal is a number and not a preference

The only quantity a better layout could recover is the ring's eviction damage to
everything else, which `SPLIT.md` §1 measured at **+219 tk/fr whole match** —
and that bucket also contains HUD/printf noise, so it is an over-estimate. At
this cycle's measured 11.8x that is **≲2,600 tk/fr at rank-80: 0.08x of
+32,593.** A build spent to chase 0.08x, against two recorded builds that
measured the opposite sign, is not a measurement — it is a coin toss with a
known bias.

### 1.7 It is also not mechanically available as briefed

5,596 B does not fit in either section, so it would have to be **split across
both** — and `.text.hot`'s membership list is (linker comment, twice measured)
*"a curated 8 KiB working set [where] removing a member re-addresses the other
ten"*. Adding 3,604 B re-addresses all eleven, which is exactly the Task 94
result. `.text.hot.draw` is machine-generated (`nds_task32_draw_hot.inc`), so
hand-adding to it is a generator change, not a linker-script edit.


---

## 2. The instrument — a second one-byte pair, this time on the mask that matters

| arm | build | dispatch | `NDS_TICK_HUD_DRAW` | out |
|---|---|---|---|---|
| **B** | `build-c179-cfxring-b-d0` | 1 | **0** | `v3-b-c179/`, `b-c179-pc.csv` |
| **A'** | `build-c180-cfxring-a2-d0` | 0 | **0** | `v3-a2-c180/`, `a2-c180-pc.csv` |

Both: `NDS_TASK37_PROFILE=1`, window 438..2038 (1,601 regions), `PER_FRAME_REGION=1`,
`NDS_R2_BOTH_CPU=1`, `NDS_R2_BATTLEPACK=1`, `NDS_R2_BATTLEPACK_KEEP_CACHE=1`,
`NDS_R2_COLLISION_FIXED=1`, DLDI on, `emulators/melonds-attributor/melonDS.exe`,
no `-RunnerSlot`. Flags read back from each build's own `nds_build_config.h`,
not from the invocation. `format=melonDS-arm9-retail-profile-v3` in both metas;
`stall_partition_residual` **-10** (B) and **0** (A');
`timestamp_discontinuities` 1 and 0.

```text
section          bytes      differing
.itcm           32,152              0
.text.hot        4,588              0
.text.hot.draw   5,268              0
.main          932,368              0
.main.rw       137,428              1   at 0x3F24
.dtcm            8,800              0
```

`nm` puts `gNdsCfxRingEnable` at `0x020eb384` and `.main.rw`'s VMA is
`0x020e7460`: `0x3F24` exactly. **The two ROMs differ in the dispatch switch and
nothing else, so this comparison has no placement floor.** `.main` is 932,368 B
here against the `DRAW=1` pair's 932,960 -- the 592 B of HUD draw compiled out.

Marginal masks: candidate >= **1,171,083** ticks, control >= **1,178,511**
ticks, 80 frames each, each arm its own (`total_cycles - halt_wait`).

**Caveat, stated rather than buried: `DRAW=0` is cleaner, not clean.**
`NDS_TICK_HUD` is still 1, so the tick HUD's *print* path survives and still
concentrates on the marginal frames: `_svfiprintf_r` **5.58x**, `consolePrintChar`
**5.26x**, `_vfiprintf_r` **4.99x**, about 8,800 tk/fr between them. That is
apparatus. It matters only that the collision cluster concentrates at **11.6x**,
more than twice as hard, so the mask is selected by collision work rather than by
the instrument -- which is exactly what `DRAW=1` could not say.

## 3. The result: the ring is EXACTLY break-even, and it is break-even at every percentile

**Whole window, 1,601 regions, B - A', ticks/frame** (`diff-b-minus-a2.txt`):

```text
issue -1,771 | icache_fill +1,801 | dcache_fill +54 | write_buffer -27
interlock +9 | bus_contention -63 | instructions -621 (a COUNT, section 6)

non-idle (total_cycles - halt_wait)   3,101,489,200 - 3,101,479,844 = +9,356 cyc
                                      = +2.9 ticks/frame
```

**Marginal, 80 frames each arm, ticks/frame:**

```text
issue -19,119 | icache_fill +20,112 | dcache_fill +538 | write_buffer -268
interlock -36 | bus_contention -792 | instructions -6,380

non-idle = -31 - (-472 armWaitForIrq) = +441 ticks/frame
```

### 3.1 RETRACTION: the `+284` whole-match cost was instrument noise

`SPLIT.md` section 0 published **+284 tk/fr** whole match for the wired ring, and
section 5.2 attributed **+321** of it to "everything else (eviction + HUD/printf
instrument noise)". On the arm the cadence gate is actually read from, that term
is gone and **the whole-match figure is +3 tk/fr.** The wired ring is
tick-neutral to one part in 300,000 of a frame. `+284` is withdrawn as a cost.

### 3.2 A PREDICTION I WROTE DOWN AND GOT WRONG, BOTH HALVES

Before reading `c180` I predicted the marginal delta at **+6,000...+9,000** and
the whole-match delta at **+200...+400**. Measured: **+441** and **+3**. I
over-predicted the cost roughly 15x at the margin, because I sized the float the
ring intercepts from `OWNER_PRICING.md` section 5's per-call soft-float
*attribution* -- a model -- instead of waiting for the difference, which is the
measurement. The lesson is the one `--diff` was built for and I did not apply to
my own arithmetic: **a stall partition is only readable as a difference.**

### 3.3 The exchange rate, and it is 1.00

Summing the ring's own rows against every row it deleted:

```text
                        whole match      marginal 80
  fixed added               +2,705           +32,010
  float deleted             -2,702           -31,569
  ------------------------------------------------
  ratio                      1.001             1.014
```

The largest deletions at the margin are the soft-float library, measured rather
than attributed: `__mulsf3` **-10,239**, `__aeabi_fadd` **-7,190**,
`sqrtf` -2,069, `__muldi3` -638, `__floatdisf` -475; and the decomp bodies
`func_ovl2_800ED490` -2,928, `gmCollisionSetInvertMatrix` -2,127,
`gmCollisionTransformMatrixAll` -1,329, `func_ovl2_800EDBA4` -1,155,
`lbCommonSin` -715, `lbCommonCos` -704. The largest additions are the ring's own
eight symbols, and **97% of every one of them is `icache_fill`.**

### 3.4 Both sides concentrate identically, which is the whole finding

```text
                          whole match   marginal    concentration
  float the ring deleted        2,702     31,569           11.68x
  fixed the ring added          2,705     32,010           11.83x
```

**The prize and the price live on the same frames and scale together.** That is
why the ring reads +3 whole match and +441 at rank-80: a 1.00 exchange rate
multiplied by any concentration is still a 1.00 exchange rate.

## 4. Task B -- the clean concentration is 11.7x, and 5.21x was the wrong bar

**Answering the question exactly as asked: the concentration is 11.68-11.83x,
2.2x ABOVE the 5.21x bar.** The `DRAW=1` mask's 3.11x was diluted, and the
dilution factor is now measured two independent ways on the *float control*:

Same eight bodies (`ED490`, `SetInvertMatrix`, `TransformMatrixAll`,
`GetWorldPosition`, `TestRectangle`, `EDE5C`, `GetFighterPartsWorldPosition`,
`EDBA4`), same window, three captures:

| quantity, float cluster | `DRAW=1` (c178) | `DRAW=0` (c172) | `DRAW=0` (c180) | correction |
|---|---:|---:|---:|---:|
| self cost concentration | 2.91x | 7.04x | **7.04x** | **2.42x** |
| call concentration | 3.49x | 8.66x | **8.84x** | **2.48–2.53x** |

The two `DRAW=0` captures are a different build at a different HEAD (`c172` is
the pre-ring shipping candidate at `48741fcaf05`) and they agree on the cost
figure to **three significant figures** — so the correction is a property of the
mask, not of the arm.

**But the bar does not decide the question it was built to decide, and this is
the correction that matters.** `SPLIT.md` section 5.5 derived "it clears at
>=5.21x" by applying ONE concentration factor to the NET (`-6,261 x k >= 32,593`).
That is only valid if the cost term stays put while the prize concentrates.
Section 3.4 measures both at ~11.7x. **The factor cancels out of the ratio, so
what decides residency is the EXCHANGE RATE, not the concentration** -- and the
exchange rate for the converted half is 1.00 at both populations.

`5.21x` is therefore **retracted as a decision rule**, not merely satisfied.

### 4.1 What the marginal frames actually are

Free, and it re-frames the lane. On the clean mask the P95 frames are not frames
where more fighter procs run -- `battleship_ftMainProcUpdateInterrupt` costs
**1.05x** there and is called **1.03x** as often. They are frames where the
*narrow phase* runs 11x more:

```text
                                             whole match   marginal   x
  gmCollisionCheckFighterAttackDamageCollide   0.97/frame  10.47/fr  10.81
  gmCollisionTestRectangle                     1.06        11.85     11.21
  gmCollisionGetWorldPosition                  1.64        19.29     11.79
  gmCollisionSetInvertMatrix                   0.59         6.95     11.88
  battleship_ftMainProcUpdateInterrupt         3.89         4.00      1.03
```

**The fighter narrow phase IS the P95 owner on this arm, at 11x presence.** The
lane's premise was right; only its arithmetic was wrong.

## 5. Residency, re-sized on measured bytes — and the one quantity these captures do NOT determine

`SPLIT.md` §5.4 estimated the resident version's text. It no longer has to be
estimated: compiling `src/port/nds_r2_collision_fixed.c` alone with the build's
own flags emits every entry point, including the four `--gc-sections` drops.

```text
symbol                                 -O2 bytes   -Os bytes
ndsR2CollisionFixedBuildLocal              1,180         796
ndsR2CollisionFixedCompose                   344         324
ndsR2CollisionFixedInvertF32               1,592         896
ndsR2CollisionFixedMakeFrame               1,212         864
ndsR2CollisionFixedAxisScalesF32             912         120
ndsR2CollisionFixedLoadF32                   448           4
ndsR2CollisionFixedStoreF32                  516         120
ndsR2CollisionFixedTestRectangle           1,504         772
ndsR2CollisionFixedTransformPoint            100         100
ndsR2CollisionFixedWorldToLocal              108           4
outlined statics (-Os only)                    -       1,228
-------------------------------------------------------------
TOTAL                                      7,916       5,228   -34.0%
```

Undefined-symbol sets are identical at both levels (no `__aeabi_lmul`);
SMULL 62 -> 58 and SMLAL 26 -> 18, so the 64-bit products survive.

Two of §5.4's estimates were low and one is now free: `TestRectangle` is
**1,504 B**, not ~1,400; `TransformPoint` + `WorldToLocal` together are
**208 B**, not ~300; and the `TestSphere` expansion prologue costs **zero
fetch** rather than ~600 B, because `gmCollisionTestSphere` executes zero times
on both arms and code that is never entered is never fetched.

**The quantity the captures do not determine, and it is the one that decides.**
§3.3's exchange rate was measured over bodies that are *straight-line matrix
arithmetic* — compose, inverse, local build, axis scales — which touch
essentially all of their bytes on every call. The bodies residency adds are not
that shape, and the difference is large and visible in the control:

```text
                             bytes  marginal calls/fr  marginal icache tk/fr  tk per byte per call
gmCollisionTestRectangle     1,680             11.85                  1,029                 0.052
ndsR2CollisionFixedStoreF32    516             17.76                  4,281                 0.467
```

**A 9x spread**, and the reason is structural rather than positional:
`gmCollisionTestRectangle` early-exits on its outcode tests and touches a small
fraction of its 1,680 B per call, while the fixed kernels run to the end. A
fixed `TestRectangle` keeps those early-exits (`ndsR2CfxOutcodeXY`/`OutcodeZ`,
`include/nds/nds_r2_collision_fixed.h:834-929`), so charging its 1,504 B at the
kernels' 0.467 tk/byte/call **over-charges it by up to an order of magnitude**,
while charging it at the float body's 0.052 under-charges it by the ARM/Thumb
density ratio. The two bounds straddle the requirement, so **this cycle does not
publish a resident number.** §5.4's −6,261 whole match and §5.5's 0.60x are left
standing as the previous cycle's, neither confirmed nor replaced.

**What settles it is one build, and it is not another concentration capture.**
Convert ONE early-exiting body — `gmCollisionGetWorldPosition` is the cheapest
(196 B float against a 100 B fixed `TransformPoint`, 19.29 calls per marginal
frame, ~30 call sites funnelling through one helper) — and read its exchange
rate off the same one-byte `--diff`. If an early-exiting body's rate is
materially below 1.00, residency is worth building; if it is 1.00 like the
straight-line half, the lane closes on arithmetic rather than on cache.

## 6. Tooling — two helpers added, one units defect fixed, one published number corrected

1. **`scripts/compare-elf-sections.py` (new).** The one-byte property is the
   entire instrument, and it has been verified by hand loops that COULD NOT FAIL
   twice: `--only-section=.text` hashing the empty string (this linker script has
   no `.text`), and an `objcopy` against a build directory that did not exist
   reporting 0 differing bytes for every section. The script refuses on a
   missing/empty/non-ELF input, a section absent from either ELF's headers, a
   zero-length section, a non-zero `objcopy`, a zero-length extraction, and a
   size mismatch; `--max-diff N` asserts the property instead of eyeballing it.
   **Falsified before use**: it reproduces the c177/c178 pair at exactly 1
   differing byte at `0x3F24`, and refuses on both historical traps.
2. **`census-marginal-frame-owners.py --concentration` (new).** Per-symbol COST
   concentration and CALL concentration side by side, off the reduced CSV that
   already carries `all_*` and `marg_*`. Both columns, because they are different
   numbers and this campaign has substituted one for the other; call counts come
   from the entry PC, so they are exact.
3. **A units defect in `--diff`, and a number it corrupted.** The `instructions`
   column is a COUNT, and the mode divided it by `2 x frames` like a cycle total.
   Every "instructions per frame" it has printed is therefore **exactly half** the
   true value. **`SPLIT.md`'s "the ring executes 204 FEWER instructions per
   frame" is corrected to −407**; this cycle's `DRAW=0` figures are **−621**
   whole match and **−6,380** at the margin. Fixed at the source, so the halved
   form is no longer printable. `plan.md` §2's units audit exists for this class.

## 7. What this cycle did NOT do

- **No placement build** (§1 is the reason, and it is arithmetic).
- **No `-Os` build.** Predicted whole-match effect is ~−700 tk/fr, ~23x under
  `plan.md` §2's ≥16,000 build floor for a ROM candidate. It rides a package or
  it does not get built. **One consequence found and not fixed**: at `-Os` GCC
  outlines `ndsR2CfxCosQ15`, which moves the declared soft-float edge out of
  `ndsR2CollisionFixedBuildLocal` and would trip
  `scripts/check-r2-collision-fixed.ps1`'s float-edge rule. The fix is
  `always_inline` on the two table helpers, not a widened allowlist.
- **No resident implementation and no resident number** (§5).
- **No production change.** `NDS_R2_COLLISION_FIXED` is still `?= 0`, so the ring
  is compile-time absent from every published target; no flag flipped, no default
  moved, no `Makefile` edited.
- **No Boundary run.** The only tracked source edits are two standalone analysis
  scripts no build, ROM or verifier imports.
- **No stress battery** — gated on a KEEP, and nothing was kept.
- **Task C was not written as a design.** §5 replaced it: the design cannot be
  chosen until the early-exit exchange rate is measured, and choosing it first is
  how §5.4's estimates became a verdict.

## 8. Root ROMs

Hashed before the first build and after the last. Unchanged, and neither was
rebuilt (mtimes 2026-08-09 and 2026-08-14):

```text
smash64ds.nds                       54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A
smash64ds-battle-playable-hwtri.nds 2015FBD1F68B81C03626D8C6D473C8BCBCF527A3A26DFE86FF19BD74ECBB1360
```

## 9. Reproduction

```powershell
# arm B (dispatch 1) then arm A' (dispatch 0), ONE BUILD AT A TIME, ~5 min each
.\scripts\run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe `
    -Build build-c179-cfxring-b-d0 -StartFrame 438 -Frames 1600 -TimeoutSeconds 7200 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1,NDS_R2_COLLISION_FIXED=1,NDS_R2_COLLISION_FIXED_DISPATCH=1,NDS_TICK_HUD_DRAW=0 `
    -OutDir artifacts/performance/2026-08-15_cfx-ring-draw0/v3-b-c179
# ... same with DISPATCH=0 into build-c180-cfxring-a2-d0 / v3-a2-c180

python scripts/compare-elf-sections.py --a builds/build-c179-cfxring-b-d0 `
    --b builds/build-c180-cfxring-a2-d0 --max-diff 1

python scripts/census-marginal-frame-owners.py --reduce --marginal 80 `
    --profile artifacts/performance/2026-08-15_cfx-ring-draw0/v3-b-c179 `
    --out artifacts/performance/2026-08-15_cfx-ring-draw0/b-c179-pc.csv   # and A'

python scripts/census-marginal-frame-owners.py --diff --top 22 `
    --pc-csv      artifacts/performance/2026-08-15_cfx-ring-draw0/b-c179-pc.csv `
    --pc-csv-base artifacts/performance/2026-08-15_cfx-ring-draw0/a2-c180-pc.csv `
    --build       builds/build-c179-cfxring-b-d0 `
    --build-base  builds/build-c180-cfxring-a2-d0

python scripts/census-marginal-frame-owners.py --concentration `
    --pc-csv artifacts/performance/2026-08-15_cfx-ring-draw0/b-c179-pc.csv `
    --build  builds/build-c179-cfxring-b-d0 --symbols <names>
```

Committed beside this file: `diff-b-minus-a2.txt`, `conc-ring-c179.txt`,
`conc-float-c180.txt`, `section-compare.txt`, `cfx-object-sizes.txt`, both
reduced per-PC CSVs and their meta, and both `arm9-profile.meta.txt` /
`arm9-profile.regions.csv`. The two 3.9 GiB profiler CSVs are **not** committed.
