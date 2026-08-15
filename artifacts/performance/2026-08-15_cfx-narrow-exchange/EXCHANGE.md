# The fixed-point collision lane closes on arithmetic. The exchange rate is 2.68, and even at ZERO the lane was 0.47x too small

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `1e2833cccba`.
Premise: `../2026-08-15_cfx-ring-draw0/FOOTPRINT.md` §5 — the resident rate was
straddled between 0.052 and 0.467 tk/byte/call and one build settles it.
Prediction written before the first run: `PREDICTION.md` (and it is wrong, §5).
**UNITS: 2 profile cycles = 1 project tick.**

## 0. Outcome first

1. **The exchange rate is 2.68 whole match.** The fixed consumer path costs
   **+3,392 tk/fr** against **−1,264 tk/fr** of float deleted. It is not
   break-even like the producers were (1.001); it is nearly three times dearer.
2. **At rank-80 the conversion costs +29,290 tk/fr net non-idle**, against
   +32,593 *required in the other direction*. 13.1x its own whole-match cost.
3. **And the closure does not even need the rate.** The identifiable float the
   whole fighter narrow phase contains is **15,217 tk/fr at rank-80**. *Even at
   an exchange rate of 0.00 — fixed point free — the lane's ceiling is 0.47x the
   requirement.* Whole match, the narrow phase's soft float is **840 tk/fr of a
   59,694 tk/fr soft-float bill: 1.4%.** **The lane was never large enough to
   close the gap at any price.**
4. **The mechanism has a name and it is not cache alone.** The fixed formulation
   calls libgcc's **64-bit divide 4.0 times per narrow-phase entry**
   (`__udivmoddi4` 7.76 → 11.65 calls/fr, +3.89 against 0.97 entries/fr), and
   `__aeabi_ldivmod`/`__udivmoddi4` is a **bit-by-bit loop**. It costs +988 tk/fr
   whole match and **+17,377 at rank-80** — more than the entire float bill the
   conversion deletes. `nds_r2_collision_fixed.h:210-217` says the DS has a
   hardware unit at ~26–36 cycles and that `NDS_R2_CFX_DIV64`/`ISQRT64` are
   overridable for exactly this reason. **Nothing ever overrode them.**
5. **The most optimistic surviving version still loses.** Resident
   `unk_dobjtrans_0x9C` (no `LoadF32`, `MakeFrame` per joint at 0.65x instead of
   per pair) **plus** the DS hardware divider takes the added side from 3,392 to
   ≈1,633 against 1,264 deleted: **rate ≈1.29.** Residency does not rescue it.
6. **`5.21x`, then the concentration, then the byte rate — all three were the
   wrong quantity.** What decides is (a) how much float is in the lane at all,
   and (b) what the fixed form *calls*. Both are now measured.

**Instrument: a second one-byte pair.** `.itcm`/`.text.hot`/`.text.hot.draw`/
`.main`/`.dtcm` **0 differing bytes**; `.main.rw` **exactly 1** at `0x3F24`,
which `readelf` + `nm` put at `gNdsCfxNarrowEnable` (`.main.rw` VMA `0x020e8120`,
symbol `0x020ec044`). Asserted by `scripts/compare-elf-sections.py --max-diff 1`
(`section-compare.txt`). **The placement floor is zero on this pair.**

---

## 1. What was built, and why it is not `gmCollisionGetWorldPosition` alone

**The brief's premise had one factual error and it is stated rather than worked
around.** `FOOTPRINT.md` §5 and the brief both ask for
`gmCollisionGetWorldPosition` *"because it early-exits"*. Read against the
source, `gm/gmcollision.c:196-205` is **nine `f32` multiplies and nine adds with
no branch at all**. The body that early-exits is `gmCollisionTestRectangle`
(`:661`), which is where the 0.052 tk/byte/call was measured in the first place.

And `gmCollisionGetWorldPosition` is **not interceptable**: it has fifteen in-TU
call sites in `gmcollision.c` (`:504 :514 :527 :537 :686 :703 :704 :793 :845
:846 :1029 :1047 :1048 :1953 :1978 :2075 :2106`) and essentially all of its 1.64
whole-match calls per frame come from them. The `#define`-before-`#include`
rename that wires the ring renames the definition **and** the call sites
together — the trap `battleship_gmcollision.c:167-169` already documents for
`gmCollisionTestRectangle`.

So slice 53 converts the one interceptable unit that contains **both** bodies:
the tail of `gmCollisionCheckFighterAttackDamageCollide` (`:1379-1400`), which
is `func_ovl2_800EDE00`, `func_ovl2_800EDE5C` and **one**
`gmCollisionTestRectangle` — and `ndsR2CfxPrepareFighterJoint` already does the
first two in fixed point. `gmCollisionTestRectangle` in turn makes **two** of the
`gmCollisionGetWorldPosition` calls. The wrapper already exists, so nothing new
was needed to hook it.

```text
ndsR2CfxTestFighterDamage (Thumb glue)   240 B   <- new
  ndsR2CfxPosQ12                         114 B   <- new
  ndsR2CfxPosQ12Vec                      158 B   <- new
  ndsR2CollisionFixedLoadF32             448 B   <- already executing (extra calls)
  ndsR2CollisionFixedMakeFrame         1,212 B   <- new, ARM
  ndsR2CollisionFixedTestRectangle     1,504 B   <- new, ARM
                                       -------
  new executing text                   3,228 B
```

Fail-closed on the source's own latches: unless prepare left
`unk_dobjtrans_0x5/0x6/0x7` **all** set, the fixed path declines and the decomp
body runs — which is also what guarantees the two side effects
(`unk_dobjtrans_0x9C`, `vec_scale`) the bypassed base would otherwise have
produced. The Thumb glue disassembles to exactly five `bl` targets and **no
`__aeabi_lmul` and no soft float**, so every 64-bit product stays in the one
`-marm` object `check-r2-collision-fixed.ps1` grades.

## 2. The instrument

| arm | build | `…_NARROW_DISPATCH` |
|---|---|---|
| **B** | `build-c181-cfxnarrow-b-d0` | **1** |
| **A** | `build-c182-cfxnarrow-a-d0` | **0** |

Both: `NDS_R2_COLLISION_FIXED=1`, `…_DISPATCH=1` (the ring stays on in both),
`NDS_R2_COLLISION_FIXED_NARROW=1` in **both** so both link every byte,
`NDS_TICK_HUD_DRAW=0`, `NDS_R2_BOTH_CPU=1`, `NDS_R2_BATTLEPACK=1`,
`KEEP_CACHE=1`, DLDI on, `NDS_TASK37_PROFILE=1`, window 438..2038 = **1,601
regions**, `emulators/melonds-attributor/melonDS.exe`, no `-RunnerSlot`. Flags
read back from each build's own `nds_build_config.h`, not from the invocation.
`format=melonDS-arm9-retail-profile-v3`, `stall_partition_residual` **0** and
**0**, `timestamp_discontinuities` **0** and **0** — cleaner than the c179/c180
pair, which carried −10 and 1.

> **Do NOT compare these numbers to the c179/c180 pair.** `.main` grew
> 932,368 → 935,624 B (+3,256), and the *control* arm c182 reads **+2,266 tk/fr**
> whole match against c180. That is cross-build re-addressing, about as large as
> the effect being measured, and it is precisely why only within-pair deltas are
> quoted here.

## 3. The result

**Whole match, 1,601 regions, B − A, ticks/frame** (`diff-b-minus-a.txt`):

```text
issue -364 | icache_fill +2,459 | dcache_fill +74 | write_buffer +8
interlock +51 | bus_contention -0 | instructions +3,056 (a COUNT)

ALL SYMBOLS +1,750 ; armWaitForIrq -478
net non-idle = +2,228 ticks/frame
```

Cross-checked against the two metas independently:
`(3,779,041,542 − 663,171,712) − (3,773,438,994 − 664,702,241) = 7,133,077`
cycles `/ 3,202` = **+2,227.7 tk/fr**.

**Marginal, 80 frames each arm its own mask, ticks/frame:**

```text
issue -5,268 | icache_fill +40,439 | dcache_fill -4,415 | write_buffer -1,182
interlock -225 | bus_contention -83 | instructions +54,861

ALL SYMBOLS +14,031 ; armWaitForIrq -15,259
net non-idle = +29,290 ticks/frame
```

### 3.1 The exchange rate, off the same `--diff`, on the mask-free half

| whole match, B − A | tk/fr |
|---|---:|
| `ndsR2CollisionFixedMakeFrame` | **+1,256** |
| `__udivmoddi4` | **+988** |
| `ndsR2CollisionFixedLoadF32` | +265 |
| `ndsR2CollisionFixedTestRectangle` | +191 |
| `ndsR2CfxPosQ12` | +164 |
| `ndsR2CfxPosQ12Vec` | +139 |
| `ndsR2CfxTestFighterDamage` | +107 |
| `__clzsi2` / `__clzdi2` | +71 / +65 |
| ring rows (`InvertF32`, `BuildLocal`, `Prepare`, `StoreF32`) | +146 |
| **fixed added** | **+3,392** |
| `__aeabi_fadd` | −464 |
| `gmCollisionTestRectangle` | −192 |
| `__mulsf3` | −190 |
| `__divsf3` | −185 |
| `gmCollisionGetWorldPosition` | −154 |
| `gmCollisionCheckFighterAttackDamageCollide` (the wrapper's own branch) | −36 |
| `__aeabi_fcmplt` | −28 |
| `func_ovl2_800EDE5C` | −15 |
| **float deleted** | **−1,264** |
| **EXCHANGE RATE** | **2.68** |

Per narrow-phase entry (0.97 entries/frame whole match): **added 3,497 tk/call
against deleted 1,303 tk/call.**

### 3.2 The ceiling, which is the closure that does not depend on the rate

At rank-80 the identifiable float the conversion removes is:

```text
__aeabi_fadd -5,941 · gmCollisionTestRectangle -2,555 · __mulsf3 -2,368
__divsf3 -2,350 · gmCollisionGetWorldPosition -2,003
                                              --------
                                              -15,217 tk/fr
```

**15,217 is 0.47x of +32,593.** A conversion that cost *nothing at all* could not
close the gap. Whole match the same set is **840 tk/fr of soft float** against a
total `fadd`+`fmul`+`fdiv` bill of **59,694 tk/fr — 1.4%**.

> **This retires the premise, not just the implementation.** `K-EXCHANGE`'s
> *"the fighter narrow phase IS the P95 owner at 11x presence"* is true about
> **presence** and false about **size**. It concentrates 11–13x and it is small:
> ~0.6% of the marginal frame. Concentration made it look like a lane.

### 3.3 Engagement and the negative control, exact from entry PCs

| symbol | B calls/fr | A calls/fr |
|---|---:|---:|
| `gmCollisionCheckFighterAttackDamageCollide` (wrapper) | **0.97** | **0.97** |
| `ndsR2CfxTestFighterDamage` | 0.97 (134 tk/fr) | 0.97 (**27** tk/fr, early return) |
| `ndsR2CollisionFixedMakeFrame` | **0.97** | **0.00** |
| `ndsR2CollisionFixedTestRectangle` | **0.97** | **0.00** |
| `ndsR2CollisionFixedLoadF32` | 1.62 | 0.65 |
| `gmCollisionTestRectangle` (float) | **0.09** | **1.06** |
| `gmCollisionGetWorldPosition` (float) | **0.20** | **1.64** |
| `func_ovl2_800EDE5C` (float) | 0.09 | 1.06 |
| `__udivmoddi4` | **11.65** | **7.76** |
| `battleship_ftMainProcUpdateInterrupt` | **3.89** | **3.89** |

The float `TestRectangle` loses **91.5%** of its calls and
`gmCollisionGetWorldPosition` **87.8%** — the conversion is complete, not
partial. Both fixed kernels read **exactly 0.00** on the control: the falsifier
arm is inert by construction, not by luck. And the wrapper and
`ftMainProcUpdateInterrupt` are identical to two decimal places on both arms,
which is the **same-fight** check the tick delta needs before it means anything.

### 3.4 A SECOND instrument, a same-binary `.data` route, agrees to 2.8%

`sample-tick-hud-buckets.ps1 -Build build-c181-cfxnarrow-b-d0 -NoBuild -RingDump
-Samples 1600 -StartFrame 438 -SetGlobals gNdsCfxNarrowEnable=1|0` — **one
binary, one poked word at the first frame-complete marker, before first read.**
Arm identity is the poke, not the build directory, so there is not even a
one-byte difference between these two arms.

| `WORK-H` | ON | OFF | Δ |
|---|---:|---:|---:|
| P50 | 950,272 | 950,656 | **−384** |
| P95 | 1,221,440 | 1,191,488 | +29,952 |
| **rank-80** | **1,222,848** | **1,194,368** | **+28,480** |
| 2-VBlank | 1,835/2,038 (90.0%) | 1,843/2,038 (90.4%) | −8 frames |

**+28,480 against the profile pair's +29,290 — 2.8% apart, on two different
instruments and two different ROM configurations.** And P50 is **flat (−384)**:
the whole cost lands on the expensive frames, exactly as 0.97 entries/frame at
12–17x presence predicts.

### 3.5 Flip budget: stated as ZERO before the run, measured ZERO

`PREDICTION.md` §2 wrote the budget down first. Both arms, whole match:

```text
gNdsCfxFighterDamagePhaseCalls 1,938   gNdsCfxFighterDamagePhaseHits  20
gNdsBattleTextHudP1Damage         76   gNdsDamageSparkScaleCount      15
gNdsShieldAnimJointAttachCount 1,352   gNdsAObjEvent32NormalizedHighWater 1,266
gNdsBattlePackHits               197   gNdsObjAnimRunawayCount         0
gNdsCfxRingPrepareCalls        1,938   ChainFixed/InvertFixed/ScaleFixed 1,006
                                       every *Declined                  0
```

**Every one of these equals the `c170`/`c174`/`c175`/`c176` bank.** Engagement on
the candidate: `gNdsCfxNarrowCalls` **1,938 == DamagePhaseCalls exactly**,
`Answered` **1,938** (100% — not one fall-through), `Hits` **20 ==
DamagePhaseHits exactly**, `Declined` **0**. Control: all four **0**.

> **KEEP THIS EVEN THOUGH THE TICKS ARE NOT KEEPABLE.** The collision DECISION
> itself ran in port fixed-point code for **1,938 of 1,938 pairs** of a whole
> both-CPU match and reproduced the decomp float outcome **exactly** — same hit
> count, same damage, same sparks, same shield attaches, same AObj high-water.
> The kernel is not wrong. It is slower.

## 4. Why it costs what it costs — and it is NOT only compulsory fetch

`icache_fill` is +2,459 of the +2,228 whole-match net, so fetch is still the
largest single class. But the surprise is the **`issue`** side, and it is a
library call the previous cycles' byte-rate model could not see:

```text
__udivmoddi4   7.76 -> 11.65 calls/fr   = +3.89 = 4.0 sixty-four-bit divides
                                          per narrow-phase entry
               whole match +988 tk/fr  (issue +395, icache +559)
               rank-80    +17,377 tk/fr
```

Four divides per entry: `ndsR2CfxMakeFrameCofactor` divides the adjugate by the
determinant and takes three `1/|row|` reciprocals for `inv_scale`
(`nds_r2_collision_fixed.h:663-763`), and `ndsR2CfxTestRectangle`'s clip
interpolation divides again when it clips. **libgcc's 64-bit divide is a
bit-by-bit loop.** Its own header already says so and offers the escape hatch —
`NDS_R2_CFX_DIV64` and `NDS_R2_CFX_ISQRT64` are `#ifndef`-overridable *"because
the DS has a hardware unit at ~26–36 cycles"* — and no port ever defined them.

**The asymmetry that made the float side cheap, measured on the same arms:**
`__mulsf3` is 408 B and pays **0.77 tk of instruction fetch per call**; a body
that size costs 147–320 tk when cold. At **1,545 calls per frame** the
soft-float library is permanently I-cache resident. A fixed-point kernel entered
**0.97 times per frame** is cold on every single call. *That* is why fixed point
loses here, and it is a property of the call rate, not of the arithmetic.

## 5. RETRACTION — my own written-down prediction, wrong on both halves

`PREDICTION.md`, written before the first run:

```text
predicted marginal   +560 ... +7,890      measured  +29,290    3.7x low
predicted whole        +50 ... +670       measured   +2,228    3.3x low
predicted rate        1.04 ... 1.26       measured     2.68
```

**The prize half was right and the price half was 2.2x low, and the reason is
nameable.** Predicted deleted ≈15,000 tk/fr at rank-80; measured **15,217**.
Predicted added from `3,228 B × 0.467 tk/byte/call × 11.85` = 17,864; measured
**≈44,500**. The model priced *the kernel's own bytes* and did not count **the
library the kernel calls**. That is the third distinct way this campaign has
mispriced a candidate from a static quantity: a residual ÷ a count, a static size
÷ a count, and now **a byte rate applied to a body that calls out of itself.**

`FOOTPRINT.md` §5's 9x straddle (0.052 vs 0.467 tk/byte/call) was answered — and
the answer is that **neither bound was the binding constraint.** The fixed
`TestRectangle` measures **0.13 tk/byte/call** whole match (191 tk/fr ÷ 1,504 B ÷
0.97 calls/fr), close to the *early-exiting* end of the straddle exactly as
predicted; and it did not matter, because `MakeFrame` and `__udivmoddi4`
together are **11.7x** `TestRectangle`'s cost.

## 6. Verdict — the lane closes

**Fixed-point arithmetic on ARM946E-S does not beat soft float for the fighter
collision cluster once instruction fetch and libgcc's 64-bit divide are paid
for.** Three independent readings, none of which depends on the others:

1. **Rate.** Producers 1.001 (straight-line, `FOOTPRINT.md`), consumers **2.68**
   (early-exiting, here). Both shapes measured; neither is below 1.00.
2. **Ceiling.** The whole lane contains **15,217 tk/fr at rank-80 = 0.47x** the
   requirement. Free would not be enough.
3. **Best case.** Resident frame **plus** the DS hardware divider prices at
   **≈1.29**, still a cost.

`NDS_R2_COLLISION_FIXED` and `NDS_R2_COLLISION_FIXED_NARROW` both remain `?= 0`;
the published targets carry zero bytes of either. **The kernels, the falsifier
and the wiring are kept** — they are correct, decision-exact, and they are the
evidence. What is retired is the *direction*: "convert collision to fixed point
to buy P95".

## 7. What this cycle did NOT do

- **No placement work, no concentration capture, no slice-2 implementation, no
  flag flip** (all out of scope by the brief).
- **No `-Os` build** — still ~23x under the ≥16,000 floor, still rides a package.
- **No hardware-divider experiment.** It is named, sized (≈1,054 tk/fr whole
  match of the 3,392 added) and left: it cannot rescue this lane, and whether it
  is worth doing for other 64-bit divides in the tree is a different question
  with a different owner.
- **No resident implementation.** §0 item 5 prices it from measured rows rather
  than building it, and the price is above 1.00.
- **No Boundary run.** Both new flags default to `0`, so every published and
  verifier-covered target is byte-identical to HEAD's; the shipping default was
  not moved. (Stated as a deviation, not hidden: the change *is* live in the two
  lab builds and nowhere else.)
- **Root ROMs untouched** (§9).

## 8. Reproduction

```powershell
# ONE BUILD AT A TIME, no -j, ~4 min each
$f='NDS_TASK37_PROFILE=1 NDS_TASK37_PROFILE_START=438 NDS_TASK37_PROFILE_FRAMES=1600 ' +
   'NDS_TASK37_PROFILE_PER_FRAME_REGION=1 NDS_TASK37_PROFILE_RESULTS=0 NDS_R2_BOTH_CPU=1 ' +
   'NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_COLLISION_FIXED=1 ' +
   'NDS_R2_COLLISION_FIXED_DISPATCH=1 NDS_R2_COLLISION_FIXED_NARROW=1 NDS_TICK_HUD_DRAW=0'
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c181-cfxnarrow-b-d0 `
     NDS_R2_COLLISION_FIXED_NARROW_DISPATCH=1 ($f -split ' ')
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c182-cfxnarrow-a-d0 `
     NDS_R2_COLLISION_FIXED_NARROW_DISPATCH=0 ($f -split ' ')

python scripts/compare-elf-sections.py --a builds/build-c181-cfxnarrow-b-d0 `
    --b builds/build-c182-cfxnarrow-a-d0 --max-diff 1

.\scripts\run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe `
    -Build build-c181-cfxnarrow-b-d0 -NoBuild -StartFrame 438 -Frames 1600 `
    -TimeoutSeconds 7200 -OutDir artifacts/performance/2026-08-15_cfx-narrow-exchange/v3-b-c181
# ... same for build-c182-cfxnarrow-a-d0 into v3-a-c182

python scripts/census-marginal-frame-owners.py --reduce --marginal 80 `
    --profile .../v3-b-c181 --out .../b-c181-pc.csv          # and the A arm
python scripts/census-marginal-frame-owners.py --diff --top 30 `
    --pc-csv .../b-c181-pc.csv --pc-csv-base .../a-c182-pc.csv `
    --build builds/build-c181-cfxnarrow-b-d0 --build-base builds/build-c182-cfxnarrow-a-d0

# invariants + a second, same-binary tick reading (the poke IS the arm)
.\scripts\sample-tick-hud-buckets.ps1 -Build build-c181-cfxnarrow-b-d0 -NoBuild -RingDump `
    -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 -SetGlobals gNdsCfxNarrowEnable=1 `
    -ExtraGlobals <21 names, see gate-on.log> -RowsCsv .../gate-on-rows.csv
# ... and again with gNdsCfxNarrowEnable=0
```

## 9. Root ROMs

Hashed before the first build and after the last. **Unchanged, and neither was
rebuilt** (both new flags default to `0`):

```text
smash64ds.nds                       54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A
smash64ds-battle-playable-hwtri.nds 2015FBD1F68B81C03626D8C6D473C8BCBCF527A3A26DFE86FF19BD74ECBB1360
```

Committed beside this file: `PREDICTION.md` (written before the first run),
`MENU.md`, `diff-b-minus-a.txt`, `section-compare.txt`, both reduced per-PC CSVs
and their meta, both `arm9-profile.meta.txt` / `arm9-profile.regions.csv`, both
gate logs and rows CSVs. The two 3.62 GiB profiler CSVs and the two 19 MB build
logs are **not** committed.
