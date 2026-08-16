# The camera chain converts in situ at 1.7x, not 5.14x — the same-operation prior does NOT survive contact with a normalize-heavy lane

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **HEAD `4957d636f16`**
**3 lab builds + 2 capture ROMs, 4 gate runs, 2 capture runs, 0 published bytes moved.**
Every figure states its window and its basis.
**UNITS: 2 profile cycles = 1 project tick.**

```text
RATE      THE HEADLINE, AND IT REFUTES THE PRIOR.  The measured in-situ exchange
          rate for the camera + projection chain is at most 1.70x whole match
          (1.91x on the gate population), against the in-binary same-operation
          prior of 5.14x.  It is an UPPER bound: the numerator counts only the
          soft-float library bill, and the float form's own self time -- which
          the census does not carry -- can only make it smaller.  Section 4.

NET       Same-binary route, floor zero: WORK-H paired median -4,736 tk/fr whole
          match, -5,568 tk/fr on the control's own top-80 frames.  P50 -4,160,
          P90 -6,016, mean -4,728, trimmed mean -4,573.  That is 5.5-6.5% of the
          +85,393 gap.  Section 3.

RANK-80   The rank-80 POINT ESTIMATE is -1,408 and it is NOISE, not the result.
          Rank-by-rank the delta is -7,808 / -6,016 / -4,160 / -3,904 / -4,160 /
          -5,248 / -4,992 at ranks 40/160/320/640/800/1200/1600; rank-80 is the
          one rank that disagrees, because ranks 10 and 20 read +30,336 and
          +20,224 from two cartridge-read frames that moved between runs.  The
          cut is a LEVEL and the level estimators agree.  Section 3.2.

ENGAGE    PROVEN FIRING AND PROVEN INERT.  Candidate: 8,148 fixed look-ats and
          8,224 fixed perspectives against 2 and 2 float (one pre-poke frame);
          control: 0 and 0 fixed, 4,076 and 4,152 float.  Saturate 0,
          degenerate 0, rescale 0 on both arms.  ALL SEVENTEEN match invariants
          are bit-identical across both arms and equal to the bank's.  Section 5.

CONTROL   The negative control is static and exact: all eight new or routed
          symbols classify `draw+dispatch` from the linked ELF -- no `sim-only`,
          no `sim+dispatch`, no `shared`.  gGMCameraMatrix's four readers were
          enumerated from the image's literal pools and every one is a display
          or present callback.  Section 5.2.

TRAP      TWO SELF-INFLICTED WOUNDS, AND FIXING THE SECOND ONE INVERTED THE
          RESULT.  Build 1 left the kernels in Thumb: EIGHTEEN `bl __aeabi_lmul`
          in one look-at.  Build 2 fixed the state and still executed FORTY-TWO
          calls per entry, because `noinline` and mismatched target attributes
          stop GCC inlining a seven-instruction leaf -- and inlining them all
          (+3,032 B) turned -4,736 into +1,600.  Section 6.

PIXELS    BLOCKED(decision: draw-side precision).  Frame-locked on the SIMULATION
          clock across two ROMs differing in one config line: 6.5350% and
          3.6325% of the top screen differ, max channel delta 251, against a
          same-build adjacent-present floor of 35.2217% / 37.7983% on the same
          crop.  Structurally identical picture -- same stage, fighters, HUD,
          no corruption -- with speckle over textured surfaces.  For scale, the
          GX_COMPOSE change REFUSED on pixels measured 0.0692%.  Section 8.

NOT DONE  NOT BANKED, NOT DEFAULTED ON.  The route word ships at 0.  Section 7.
```

---

## 0. Basis, stated once

| | |
|---|---|
| target | `smash64ds-battle-playable-tickhud-hwtri` (the measurement instrument) |
| builds | `build-c201-camfix` (v1 kernels), `build-c202-camfix2` (v2, leaves inlined) |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_FIGHTER_GX_COMPOSE_LAB=1`, DLDI on |
| config parity | `builds/build-c201-camfix/nds_build_config.h` differs from `build-c199-bank0`'s in **one line**, `NDS_TASK10_GIT_SHORT` |
| window | 1,600 samples, frames 439..2038, `-RingDump`, `slips=0` on all four runs |
| series | **`WORK-H`**, rank-80 = 80th-largest of the run's own 1,600 rows, net = raw − 24,947 apparatus |
| requirement | **+85,393** (`build-c199-bank0`, rank-80 1,230,720 raw / 1,205,773 net) |
| route | `gNdsR2CameraFixedEnabled`, a `.data` word, poked with `-SetGlobals`; the harness records `readback` == `requested` and `stuck: true` on every run |

**The pairs are same-binary and the harness proves it**: `c201-route0` and `c201-route1`
report `romSha256 AC3206B294B4E92F`; `c202-route0` and `c202-route1` report
`81A3F03395828553`. A same-binary route has **no placement term**. That is not a
formality here — see §3.3, where two builds whose route-0 behaviour is *identical* read
**+5,440 paired median and +12,864 at rank-80** apart from placement alone.

---

## 1. What was converted, and how the chain was chosen

`DRAW_FIXEDPOINT.md` §7.3 named "the camera + projection chain only" as the falsifier.
The chain was then resolved **exactly**, from entry-PC execution counts on
`build-c200-trackprof-off`'s marginal-80 mask (counts, not samples):

```text
gmCameraLookAtFuncMatrix   2.000/fr   the game camera (via ndsFighterDisplayContractSubmit)
syMatrixLookAtReflect      2.000/fr   the renderer adapter's camera modelview
syMatrixPerspFast          2.138/fr   the renderer adapter's projection
syMatrixLookAtReflectF     4.000/fr   = 2.000 + 2.000, both producers
syMatrixPerspFastF         6.138/fr   = 2.000 + 2.138 + 2.000 particle camera
syMatrixF2L                6.138/fr   = 2.000 + 2.138 + 2.000, EVERY entry in the match
guMtxCatF                  1.000/fr   the particle camera only -- NOT converted
syMatrixLookAtF            2.000/fr   the particle camera only -- NOT converted
```

So **two of the three camera producers were converted** — the game camera and the
renderer adapter's — and the particle camera (`ndsParticleSetCurrentCamera`) was left
alone. That split is not arbitrary: the two converted producers share one look-at and one
perspective kernel; the particle camera uses `syMatrixLookAtF` (no reflectance),
`syMatrixOrthoF` and a full 4x4 `guMtxCatF`, which is a different kernel set.

**`syMatrixF2L` is deleted, not converted, for all 6.138 of its entries a frame**, because
after conversion every one of its three callers already holds a Q20.12 matrix.

### 1.1 The renderer arm is precision-neutral by construction

Every renderer site ran

```c
syMatrixLookAtReflect(&mtx, &look_at, ...);      /* float chain + syMatrixF2L */
ndsRendererAdapterMtxFromN64(&mtx, out);         /* s15.16 -> Q20.12, round to nearest */
```

i.e. float -> 16.16 -> **12 fractional bits**. The value the GX hardware sees already
carries exactly twelve fractional bits, so a Q20.12 kernel lands in the *same
representable set*; only the intermediate rounding differs. The three renderer sites also
pass a `LookAt` stack local that nothing reads, so the fixed arm passes `NULL` and skips
six `FTOFRAC8` conversions and sixteen constant byte stores that the float arm still pays.

### 1.2 The game-camera arm keeps a float boundary, deliberately

`gGMCameraMatrix` stays `f32` because its readers are `f32`. Its reader set was taken
from the linked image's **literal pools**, not from grep: `ifCommonPlayerTagProcDisplay`,
`ndsBaseFTDisplayMainProcDisplay`, `ndsIFCommonNativeOamBeginFrame` and
`ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame` — four readers, all display or
present callbacks, **no simulation reader**. The write-back is 32 `__aeabi_i2f` a frame
(the `* (1/4096.0f)` is folded into an exponent subtraction, so no `__aeabi_fmul`).

`gGCMatrixPerspF` and `sGCMatrixProjectL` have exactly three referrers between them and
all three are inside this translation unit, so the fixed arm does not have to maintain
`gGCMatrixPerspF` at all. The dead `sGCMatrixProjectL` publish and its graphics-heap
`Mtx` **are** maintained — bit-for-bit the same allocation and store — because dropping
them is `NDS_R2_CAMERA_MATRIX_LEAN` level 3, which has a recorded, unexplained
pacing-accounting failure and is not shipped.

### 1.3 Refusals honoured

- **Nothing was added to `__udivmoddi4`.** Every divide and every root goes to the DS
  hardware units at `0x04000280` / `0x040002B0`. No generic 64-bit divide, no sqrt loop,
  no large cold kernel.
- **Every multiply is ARM-state.** The kernels carry `target("arm")`; §6 is the measured
  proof of what happens without it.
- The two units are one shared register set. Every call site here is in the frame loop's
  display phase and none is interrupt-reachable, which is the same argument
  `nds_renderer.c` already relies on for `sqrt64`.

---

## 2. Three implementations were built; only the first one is a win

| | v1 `build-c201-camfix` | v2 `build-c202-camfix2` | v3 (ITCM) |
|---|---|---|---|
| kernels | ARM state, leaves out-of-line | ARM state, leaves `always_inline` | v1 shape, `section(".itcm")` |
| look-at body | 1,148 B, 42 calls/entry | ~3,068 B, 12 calls/entry | — |
| `.main` vs the bank | +2,944 B | +5,976 B | — |
| **paired median WORK-H** | **−4,736** | **+1,600** | **did not link** |

**v3 does not fit and that is a finding, not an accident.** `.itcm` on the *measurement
instrument* (`smash64ds-battle-playable-tickhud-hwtri`) is **32,188 / 32,768 — 580 bytes
free**, not the 2,976 the classification quoted. That 2,976 is Boundary's manifest for the
**proof** ROM (`Renderer ITCM placement passed: elf=smash64ds-battle-playable-proof-hwtri.elf`).
Two link attempts: the two kernels alone overflowed `itcm` by **916 bytes**; with their
leaves, by **1,452**.

> **And the deeper reason is structural, not budgetary.** A `.data` route word requires
> *both* arms to be resident at once, so a replacement for an ITCM-resident library can
> never be tested for ITCM residency by a same-binary route — the library it replaces is
> occupying the space. Answering the ITCM question needs a compile-time-gated pair, whose
> cross-build floor is >=14,080 at rank-80, i.e. **three times the effect being measured**.

---

## 3. The measurement

### 3.1 The level

All four runs: 1,600 samples, frames 439..2038, `slips=0`, DLDI on, `WORK-H`.

| | v1 route 0 (float) | v1 route 1 (fixed) | delta |
|---|---:|---:|---:|
| **paired median, per frame** | — | — | **-4,736** |
| paired mean | — | — | -4,728 |
| frames improved | — | — | **1,439 / 1,600** |
| P50 | 943,104 | 938,944 | -4,160 |
| P90 | 1,125,120 | 1,119,104 | -6,016 |
| mean | 968,422 | 963,694 | -4,728 |
| trimmed mean (drop top 8) | 964,340 | 959,766 | -4,573 |
| rank-80 raw / net / gap | 1,220,800 / 1,195,853 / **+75,473** | 1,219,392 / 1,194,445 / **+74,065** | -1,408 |
| over-gate frames | 130 | 124 | -6 |

**Bucket attribution passes**: the paired median moves **FTR -3,264** and **STG -1,664**
(sum -4,928 against WORK-H's -4,736), which is exactly where the two converted producers
run — the game camera under the fighter display contract, the adapter camera under stage
and fighter matrix preparation. No other bucket moves at the median.

### 3.2 Why the rank-80 point estimate is not the result

```text
rank      1      5     10     20     40     80    160    320    640    800   1200   1600
delta -34.3k -42.3k +30.3k +20.2k  -7.8k  -1.4k  -6.0k  -4.2k  -3.9k  -4.2k  -5.2k  -5.0k
```

Every rank from 160 down reads -3,900 to -6,000, i.e. the cut is a **level**, as a
per-frame-constant workload must be. Ranks 10 and 20 read **+30,336 and +20,224**: those
are cartridge-read frames whose cost is not reproducible between two emulator sessions
(the paired delta's min/max are -157,248 / +152,448, and the two runs' `WORK-H` maxima
differ by 34,304 on a match whose seventeen invariants are bit-identical). Rank-80 sits
inside that contaminated band, so its **-1,408 is a sample of the noise, not a smaller
effect**. The paired per-frame median is immune to it by construction and is what this
document quotes. `a-threshold-on-the-quantum-sorts-noise`, in a new place.

### 3.3 The cross-build floor, measured on this pair rather than assumed

`c201-route0` and `c202-route0` execute **the same code path** — route 0 never enters a
fixed kernel, and both arms' engagement counters prove it (`FixedLookAtCalls 0`,
`FloatLookAtCalls 4,076`, identical on both). They are two separately-linked builds:

```text
paired median  +5,440      P50  +5,952      rank-80  +12,864
```

So **placement alone moves the paired median by 5,440 — more than this cycle's entire
effect** — and rank-80 by 12,864, consistent with the campaign's >=14,080 floor. Only
within-pair deltas are quotable here, which is exactly why both A/Bs are same-binary.

---

## 4. THE EXCHANGE RATE, and it refutes the prior

The gross the conversion deletes, from `DRAW_FIXEDPOINT.md` section 2/3 rates and the
**exact** entry-PC counts (the call-count half is independently confirmed by this cycle's
own engagement counters — 4,076 = 2.000/frame, 4,152 = 2.037/frame):

| symbol | tk/fr | entries/fr | converted | deleted |
|---|---:|---:|---:|---:|
| `syMatrixLookAtReflectF` | 4,999.3 | 4.000 | 4.000 | 4,999.3 |
| its `sqrtf` | 1,735.0 | 4.000 | 4.000 | 1,735.0 |
| `syMatrixPerspFastF` | 2,709.7 | 6.138 | 4.138 | 1,826.8 |
| `syMatrixF2L` | 1,933.4 | 6.138 | 6.138 | 1,933.4 |
| `ndsCameraCatCamera` | 681.0 | 2.000 | 2.000 | 681.0 |
| `gmCameraLookAtFuncMatrix` | 481.9 | 2.000 | 2.000 | 481.9 |
| **GROSS, marginal-80** | | | | **11,657** |
| **GROSS, whole match** (category ratio 13,209/13,035) | | | | **11,504** |

Charging the replacement the way `DRAW_FIXEDPOINT.md` section 5 route A does — replacement
cost = gross / R, so R = gross / (gross - net):

```text
whole match      R = 11,504 / (11,504 - 4,736) = 1.700x
gate population  R = 11,657 / (11,657 - 5,568) = 1.915x
prior            R =  2,921 /    568.40        = 5.14x
```

# **MEASURED 1.70x. THE 5.14x PRIOR IS REFUTED FOR THIS LANE.**

**And 1.70x is an UPPER bound.** The numerator counts only the soft-float *library* bill
those callers cause; the float form's own self time — the loads, stores and shuffling
around each `bl`, which the census does not carry — belongs in it too, and adding it drives
R toward 1. The prior's 2,921 tk *did* include `guMtxCatF`'s self time, so the two are not
even measured the same way, and the difference runs against the prior.

### 4.1 Why the prior does not transfer, named rather than guessed

1. **The prior is a pure multiply-accumulate pair.** `guMtxCatF` is 64 `fmul` + 64 `fadd`
   and nothing else; its fixed twin is 64 `smlal`. The camera chain is not that shape: per
   look-at entry it needs **three square roots and nine divisions**, plus five more
   divisions per perspective. In fixed point those go to the DS hardware units, and a
   hardware divide is a *register* sequence — mode write, poll, 64-bit numerator write,
   denominator write, poll, result read — not free ALU work. In float they go to `sqrtf`
   (already hardware-backed on this tree, 144.62 tk/call) and `__aeabi_fdiv`
   (59.19 tk/call), which are far from the 13.23 tk of an `fmul`. **The lane's
   transcendental half converts at a much worse rate than its arithmetic half, and the
   camera chain is the most transcendental-heavy member of the lane.**
2. **The float library is ITCM-resident and the replacement cannot be** (section 2). The
   prior's fixed arm, `ndsRendererMtxMul20p12`, is `.main` code that is *already hot* — it
   runs 18.55 times a frame. A new kernel entered 8.138 times a frame pays compulsory fetch
   on bytes nothing else touches.
3. **The boundary is real**: nine `f32 -> Q12` conversions per look-at (done as integer bit
   manipulation, not `(s32)(v * 4096.0F)`, or it would have been worse) and 32
   `__aeabi_i2f` a frame writing `gGMCameraMatrix` back for its four float readers.

---

## 5. Engagement and the negative control

### 5.1 The route fired, and it was inert on the control

| counter | v1 route 0 | v1 route 1 | v2 route 0 | v2 route 1 |
|---|---:|---:|---:|---:|
| `...FixedLookAtCalls` | 0 | **8,148** | 0 | 8,148 |
| `...FixedPerspCalls` | 0 | **8,224** | 0 | 8,224 |
| `...FloatLookAtCalls` | 4,076 | 2 | 4,076 | 2 |
| `...FloatPerspCalls` | 4,152 | 2 | 4,152 | 2 |
| `...GameCalls` | 0 | 4,074 | 0 | 4,074 |
| `...GameFloatCalls` | 4,076 | 2 | 4,076 | 2 |
| `...SaturateCount` | 0 | **0** | 0 | **0** |
| `...DegenerateCount` | 0 | **0** | 0 | **0** |
| `...RescaleCount` | 0 | **0** | 0 | **0** |

The residual `2`s are one frame: `-SetGlobals` pokes at the first frame-complete marker, so
exactly one frame of each producer runs float before the route lands. **8,148 of 8,150
look-ats and 8,224 of 8,226 perspectives ran in Q20.12.**

`SaturateCount 0` and `DegenerateCount 0` are the range proof: no product left Q20.12's
+/-524,288 and no vector was degenerate across 8,148 look-ats. `RescaleCount 0` confirms
the >32,000 rescale pass never fires in this match on either arm, which is what the c200
entry-PC counts predicted (`ndsCameraCatCamera` exactly 2.000/frame against
`gmCameraLookAtFuncMatrix`'s 2.000).

### 5.2 No simulation caller routes through the fixed path

Two independent proofs, both from the linked ELF rather than from names:

- `classify-softfloat-caller-phase.py` on `build-c201-camfix`'s disassembly classifies all
  eight new or routed symbols — `ndsR2CameraLookAtReflect20p12`,
  `ndsR2CameraPerspFast20p12`, `ndsRendererAdapterCameraLookAtReflect`,
  `ndsRendererAdapterCameraPerspFast`, `gmCameraLookAtFuncMatrix`, `ndsCameraCatQ`,
  `ndsR2CamDiv64`, `ndsR2CamSqrt64` — as **`draw+dispatch`, 100.0%**. Zero `sim-only`,
  zero `sim+dispatch`, zero `shared`.
- `gGMCameraMatrix`'s referrers, taken from the image's **literal pools**: four readers,
  every one a display or present callback (section 1.2). `gGCMatrixPerspF` and
  `sGCMatrixProjectL` have three referrers between them and all three are inside the
  camera's own translation unit.

**And the runtime control agrees**: all **seventeen** match invariants are bit-identical
across all four runs *and* equal to `build-c199-bank0`'s — `P1Damage 76 / spark 16 /
shield 480 / AObj 774 / packHits 257 / runaway 0 / Task36 2/161 / parse
144,383-108,128-36,255-70,796 / heap-min 53,136 / arena 1,548,288 / allocFail 0 /
resident 287,904 / loadFails 0`. A simulation caller taking the fixed path could not leave
those untouched.

---

## 6. Two self-inflicted traps, both measured, and the second one inverted the result

**Trap 1 — the kernels compiled to Thumb.** The first working build put
`__attribute__((target("arm")))` on the *helpers* and not on the two kernel bodies.
`objdump` found **eighteen `bl __aeabi_lmul` inside one look-at** — the nine sums of
squares and the nine translation-row products — at 4.49 cycles a multiply plus the call.
Adding the attribute to the kernels turned those into **6 `smull` + 12 `smlal`** inline.
`thumb-hides-64bit-cost` reproduced exactly, and caught by disassembling before measuring
rather than by an inexplicable number afterwards.

**Trap 2 — `noinline` and mismatched target attributes stop inlining.** Even in ARM state,
one look-at entry still executed **42 calls**: 9 `bl ndsR2CamF32ToQ`, 12 `bl ndsR2CamMulQ`,
9 `bl ndsR2CamDivQ64` (each calling `ndsR2CamDiv64` again), 3 `bl ndsR2CamSqrt64`, for
bodies of one to seven instructions. GCC will not inline across differing `target`
attributes, so a plain-Thumb leaf called from an ARM kernel stays a `bl` however small it
is. **The classification's "55% of the lane converts to code SMALLER than the `bl` it
deletes" only happens if the leaves are `always_inline` *and* carry the same target.**

**But fixing trap 2 made the result WORSE, and that is the cycle's most useful surprise.**
v2 inlined every leaf and cut 42 calls per entry to 12 — and the paired median went from
**-4,736 to +1,600**. Working it across the two same-binary pairs and the measured
cross-build control term (section 3.3):

```text
fixed_v2 - fixed_v1 = (float_v2 - float_v1) + (delta_v2 - delta_v1)
                    = +5,440 + 6,336 = +11,776 tk/fr
for +3,032 B of .main = ~95 cache lines, over 8.138 kernel entries a frame
```

which is ~30 cycles per line per entry — **inside the 23-51 cycle `icache_fill` band this
campaign has already measured**. That is a corroboration, not a banked price: the first
term is a cross-build difference, and this document does not quote per-line costs as
prices (`a-residual-divided-by-a-count-is-not-a-price`).

> **The rule this earns: on a kernel entered a handful of times a frame, inlining is a
> COST, not an optimization.** The collision ring lost its whole arithmetic win to
> compulsory fetch of its own bytes at 0.97 entries/frame (`K-ICACHE`); the same mechanism
> is here at 8.138 entries/frame, and it is strong enough to **invert the sign** of a
> 4,736-tick win.

---

## 7. What this cycle did NOT do

- **Nothing is banked and no default moved.** `gNdsR2CameraFixedEnabled` initialises to
  `NDS_R2_CAMERA_FIXED`, whose Makefile default is **0**. The shipped path is the float
  chain, byte-for-byte the behaviour it had, and the control arm's seventeen invariants
  prove it.
- **The particle camera is untouched** — `ndsParticleSetCurrentCamera`'s `syMatrixLookAtF`
  (2.000/fr, 2,184 tk/fr + 868 sqrt), `syMatrixOrthoF` and `guMtxCatF` (1.000/fr,
  2,061 tk/fr) are a different kernel set and were out of scope.
- **`gmCameraCheckTargetInBounds`** (92.2 tk/fr) was left in float.
- **The ITCM arm could not be built** (section 2), so the placement question is open and is
  the single largest uncertainty in the rate above.

---

## 8. The pixels — BLOCKED(decision: draw-side precision)

**This is an owner decision and nothing here chooses it.** The route ships at 0.

Two proof-target ROMs whose generated configs differ in **exactly one line**
(`#define NDS_R2_CAMERA_FIXED 0` vs `1`) — `build-c204-cap0`
`99824E52C9D95B2A`, `build-c204-cap1` `07D9331C32739BD2`. Captured with
`capture-melonds.ps1 -ExactTimeRemain 1694 -SoftwareRenderer -NoJit`, which locks both
arms on the **simulation clock** (`EXACT_LOCK=gSCManagerBattleState->time_remain,1694,1692`)
rather than the presented-frame counter — the presented counter drifts in proportion to how
much faster the candidate is, so it cannot be used across arms. Comparison is
`compare-capture-pair.ps1` cropped to the **top screen, 400x300 = 120,000 pixels**; the
bottom screen is excluded because it carries the tick HUD's own FPS readout, which differs
between arms *by construction*.

| pair | differing | max channel delta |
|---|---:|---:|
| **cross-ROM, tic 1694** | **7,842 / 120,000 = 6.5350%** | 251 |
| **cross-ROM, tic 1692** | **4,359 / 120,000 = 3.6325%** | 251 |
| same-build adjacent-tic floor, control | 42,266 / 120,000 = 35.2217% | 255 |
| same-build adjacent-tic floor, candidate | 45,358 / 120,000 = 37.7983% | 255 |

**What the picture looks like.** The candidate frame is structurally identical: same stage,
same two fighters in the same places, same HUD (`DMG 44% / 0%`, `TIME 00:29`,
`STOCK x1`), no corruption, no missing or displaced object, no changed telegraph. The diff
mask (`diff-lock1694-top.png`) is **speckle distributed over textured surfaces** — canopy,
ground band, platform edges — not a displaced object and not a contiguous shift. That is
the signature of a one-LSB change in a Q12 matrix cell moving texture sampling and triangle
edges by a fraction of a pixel.

**Two honest caveats the owner should have with the number:**

1. **6.5% is not "invisible".** For scale, the `GX_COMPOSE` change that was *refused* on
   pixel evidence measured **0.0692%** and **0.1233%** on the same instrument and the same
   crop. This is 50-90x that. A precision change to the view-projection touches every
   textured pixel in the scene, so a large *count* of differing pixels at a small *per-pixel*
   magnitude is the expected shape — but the count is what it is.
2. **This method cannot fully separate precision from present-phase skew.** The lock
   synchronises the simulation; the pixels are still the last *completed* present, and the
   candidate arm is ~4,736 tk/fr faster, so some of the 6.5% is the present landing at a
   different point of the scroll. The bound: one whole present of quantization is worth
   **35-38%** on this crop, so the measured 3.6-6.5% is far below a present — but it is not
   zero and the split is not measured here.

**What the owner must look at**, both in `artifacts/visibility/2026-08-16_camera-fixedpoint/`:
`lock1694-route0-a.png` (float, shipping) against `lock1694-route1-a.png` (Q20.12), and
`diff-lock1694-top.png` for where the difference lives.

**What is being asked, and what it buys.** Accepting this precision change on the camera
chain buys **-4,736 tk/fr paired median, 5.5% of the +85,393 gap**. It is not a lever that
closes anything on its own, and section 4 says the rest of the draw-side lane will not
convert at the prior's rate either. The decision is therefore not "accept this cut" but
**"is a draw-side precision budget of this shape open at all"** — because if it is, the
particle/quad-math category (9,015 tk/fr, and far more multiply-accumulate than this one)
is the next and better candidate, and if it is not, the whole 34,178 tk/fr lane closes.

---

## 9. What the next cycle inherits

1. **The rate to size the remaining lane with is 1.70x, not 5.14x — but it is a
   LOWER-BOUND-of-value rate for the lane as a whole**, because the camera chain is the
   lane's most transcendental-heavy member (3 roots + 9 divides per look-at). The two
   categories that are mostly multiply-accumulate — **particle/quad math 9,015 tk/fr** and
   **fighter matrix prep 3,445** — should sit between 1.70x and 5.14x, and the honest way to
   size the residual ~18,000 tk/fr of draw-side conversion today is:

```text
draw-side lane            34,178 tk/fr   (30,638 soft float + 3,540 draw-side sqrtf)
converted this cycle     -11,657         (gross, marginal-80; see section 4)
RESIDUAL                  22,521 tk/fr

at the MEASURED 1.70x     22,521 x (1 - 1/1.70) =  9,273 tk/fr   0.109x of +85,393
at the PRIOR   5.14x      22,521 x (1 - 1/5.14) = 18,140 tk/fr   0.212x
```

   (The brief framed the residual as ~18,000 because it assumed the camera chain would
   take 15,812; it took 11,657, because `syMatrixPerspFastF`'s particle-camera third and
   `gmCameraCheckTargetInBounds` were left in float. At 1.70x, 18,000 would be 7,412.)

   i.e. **the whole remaining draw-side soft-float lane is now worth 0.11x-0.21x of the
   gap, against the 0.302x the conservative ceiling claimed for the WHOLE lane.** `DRAW_FIXEDPOINT.md`
   section 5's 24,564 tk/fr ceiling should be read as **~11,000-15,000** on this evidence,
   and `LADDER.md` section 4's amendment (which lifted the engineering share from 0.357x to
   0.659x on that ceiling) needs re-reading against it.

2. **The ITCM question is the largest single uncertainty and it needs a compile-time pair.**
   580 B free on the instrument, 2,976 on the proof ROM, and a `.data` route structurally
   cannot test it. Whoever takes it must either free ITCM first or accept a >=14,080
   cross-build floor against a ~5,000-tick effect — i.e. it is not measurable as things
   stand, and that is a tooling problem before it is a performance one.

3. **The particle camera is unconverted and is the cheapest remaining piece of this lane**:
   `ndsParticleSetCurrentCamera` drives `syMatrixLookAtF` (2.000/fr, 2,184 tk/fr + 868 of
   `sqrtf`), `syMatrixOrthoF` and the last live `guMtxCatF` (1.000/fr, 2,061 tk/fr) —
   ~5,100 tk/fr gross, and its concat is the *exact* shape the 5.14x prior was measured on.
   **If any part of this lane converts at the prior's rate, it is that one.** It also needs
   no float write-back if its consumer is already 20.12.

4. **`gGMCameraStruct` is in the Task 9 state hash** (`nds_task9_state_hash.c:575`, and the
   Makefile's `NDS_R2_CAMERA_MATRIX_LEAN` comment says so explicitly). The game-camera arm
   writes its six `FTOFRAC8` reflectance bytes, so **the fixed arm can move the Task 9 hash
   even though all seventeen match invariants are bit-identical**. This build does not
   compile the hash, so it is not measured here; a default flip must measure it first. The
   renderer arm is free of this — it passes `NULL`.
