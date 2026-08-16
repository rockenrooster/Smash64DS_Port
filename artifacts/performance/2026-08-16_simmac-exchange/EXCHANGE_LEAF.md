# The warm-MAC exchange rate is 0.83x and 1.00x — a leaf fixed-point conversion pays one soft-float multiply per boundary float, and that is the whole answer

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **HEAD `cc9a40e19bf`**
2 lab builds (`build-c208-simmac`, `build-c209-simmac2`), 7 whole-match gate runs,
0 gameplay values changed, no default flipped, no ROM published.
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.

```text
REQUIREMENT  +94,481 net ticks per presented frame at rank-80.  Basis:
             build-c206-shipgx0, rank-80 1,239,808 raw / 1,214,861 net against
             the 1,120,380 gate; SHIPPING renderer (GX_COMPOSE 0), bore 0, mode
             163 one-minute match, 1,600 samples, frames 439-2038, slips=0
             (../2026-08-16_gap-position/POSITION.md section 1).

ANSWER       MEASURED, IN SITU, SAME BINARY, ON THE TWO BODIES SIMSIDE.md NAMED
             AS THE CLEANEST WARM-MAC CANDIDATES:

               gmCollisionGetWorldPosition   gross 289.6  fixed 348.2  R = 0.83x
               func_ovl2_800ED490            gross 987.3  fixed 987.8  R = 1.00x

             The transform's fixed form is 20% DEARER than the soft float it
             replaces. The compose is a dead heat. Together the two are a
             31,850 tk/fr lane that converts to -2,660 tk/fr -- a LOSS.
             Counting the float bodies' own self time in the numerator (which
             the gross does not) lifts them only to 1.00x and 1.17x.

MECHANISM    AN f32 <-> Q EDGE CONVERSION COSTS 31-42 CYCLES, i.e. between one
             __aeabi_fmul (26.5 cyc) and one __aeabi_fadd (38 cyc). So a leaf
             conversion's rate is decided by CONVERSIONS PER DELETED FLOAT
             OPERATION, and that number is a property of the FUNCTION SIGNATURE,
             knowable with no build:
               transform  15 floats in + 3 out = 18 conv / 18 ops = 1.00
               compose    24 floats in + 12 out = 36 conv / 63 ops = 0.57
             1.00 conv/op cannot pay. 0.57 breaks even. The 5.14x prior
             (guMtxCatF -> ndsRendererMtxMul20p12) has conv/op = 0 because both
             sides already hold their native representation. Section 5.

CONSEQUENCE  SIMSIDE.md's warm MAC subset does NOT convert at one rate and
             SIZING IT BY FUNCTION WAS THE ERROR. Its two largest members --
             44.6% of the 71,491 -- are LEAVES and are worth zero. POSITION.md's
             fidelity-neutral inventory stays 0.563x; the projected 0.875x-1.173x
             is REFUTED for the leaf route. What survives is the CHAIN route,
             where conv/op falls as the chain lengthens -- and the only chain
             ever measured here is the ring, at 2.68x with a crippled divide.
             Section 7.

EQUIVALENCE  GRADED LIVE, WHOLE MATCH, EVERY CAPTURED CALL, AT ZERO FIDELITY
             RISK. 8,052 components against the decomp float body's own result
             on the same inputs: 6,241 exactly equal, 1,811 off by ONE Q12
             quantum, none worse. MAX DEVIATION 1 quantum = 0.000244 world
             units against the cluster's 0.0200 bound -- 82x inside it.
             Section 6.

CONTROL      All 19 whole-match invariants are BIT-IDENTICAL on all six arms,
             including gNdsCfxFighterDamagePhaseCalls 2,684 and Hits 26. The
             arms cannot play different fights by construction and they did not.
```

---

## 1. Basis, stated once

| | |
|---|---|
| target | `smash64ds-battle-playable-tickhud-hwtri` (the measurement instrument) |
| build | `builds/build-c209-simmac2`, `NDS_R2_SIM_MAC_SHADOW=1` |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1`, GX_COMPOSE 0 (shipping), bore 0, DLDI on |
| match | Boundary `battle_playable_realtime`, mode 163, one minute |
| window | 1,600 samples, frames 440–2039, `-RingDump` stride 96, `slips=0` on every arm |
| route | `gNdsR2SimMacShadowArm`, a `.data` word, `-SetGlobals`; `readback == requested` and `stuck: true` on all six |
| provenance | `romSha256` **identical on all six arms** — a same-binary route has no placement term |
| level check | this instrument's arm-0 rank-80 is **1,238,912 raw / 1,213,965 net** against `build-c206-shipgx0`'s 1,239,808 / 1,214,861 — **896 apart**, far inside the ≥14,080 cross-build floor, so the arm sits at the banked shipping level and the shadow is inert at arm 0 |

The `.main` cost of the whole instrument is **+3,224 B** against `build-c206-shipgx0`
(990,956 → 994,180). That is the instrument, not a conversion: a conversion would
delete the float bodies rather than run beside them.

---

## 2. Why a shadow and not a route

`func_ovl2_800ED490` and `gmCollisionGetWorldPosition` feed the hit decision and the
level-3 AI. A replacement route would make the two arms play **different fights**, and
the ticks would then price the fight — one route A/B on this exact collision code ended
with damage 130/51 against 33/65 on the same ELF, one poked bit apart.

So the candidate arm **runs the decomp float body exactly as before, keeps its result,
and additionally evaluates the fixed form and discards it.** Consequences:

1. Every arm plays the bit-identical match **by construction**, so all nineteen
   whole-match invariants MUST be equal — and an unequal one would mean the
   *instrument* is broken, not that the candidate is interesting. They are equal.
2. The measured delta is the **replacement cost**, directly. It does not have to be
   backed out of a net, which is where `a-residual-divided-by-a-count` lives.
3. The fixed result can be graded against the float body's own answer, on live match
   data, every call, at zero risk (§6).

---

## 3. THE FIRST RESULT COST NOTHING AND CHANGED THE PLAN: both bodies are unreachable from port code

The first build wrapped both functions with the `#define`-before-`#include` rename and
measured its own coverage over a whole match:

```text
build-c208-simmac, arm 0, 2,039 presented frames
  gNdsR2SimMacXfrmCalls  =  168      0.082 entries/frame   against 45.2
  gNdsR2SimMacCmpsCalls  =    0      EXACTLY ZERO          against 19.0
```

The rename moves the **definition and `gmcollision.c`'s own call sites together**, so a
wrapper sees cross-TU calls only. For `func_ovl2_800ED490` there is exactly one cross-TU
site in the tree (`lb/lbcommon.c:1602`) and it never executes in this match. For
`gmCollisionGetWorldPosition` the cross-TU sites are the AI's target check, two
`scsubsysfighter` seams, `grsector`, the Fox blaster and about twenty-five sites in
`reloc_backend_diagnostic_recorders.c` — **0.18% of its entries between them.**

**So these two bodies cannot be routed, wrapped, shadowed or A/B'd from port code at
all**, and `Makefile:2302` forbids a decomp overlay patch for a new adaptation. Any
conversion of them is a **caller rewrite**, not an interception — which is exactly what
`include/nds/nds_r2_collision_fixed.h` was built for and what the ring already does.

The second build therefore drives the kernels from the one seam in this file that has
**zero in-TU callers** — the fighter damage-collide gateway, the property the ring
wrapper already relies on. It captured **2,684 of 2,684** calls
(`gNdsR2SimMacDriveCalls == gNdsCfxFighterDamagePhaseCalls`, exactly), on the live joint
matrix `parts->mtx_translate` and the live hurtbox offset — the same operands
`gm/gmcollision.c:504/527/1953` hands the real call.

That seam is still only **1.32 calls/frame**, so the arm word carries a repeat count and
the price is read from the slope. §4.

---

## 4. The measurement, and the statistic it needs

**The naive statistics are all wrong here and it is worth saying why.** The driving seam
fires in bursts — a hitbox has to be live — so most frames carry no evaluation at all:

```text
r64tc vs r0, per-frame paired WORK-H delta, 1,600 frames
  median      128        (structurally ~0: most frames have no drive)
  mean    123,628
  trim-40  19,755        (trimming deletes exactly the frames that carry the work)
  min  -4,198,848   max  +9,041,856   (cartridge-read frames, not reproducible)
```

The median is zero by construction, the mean is carried by two cartridge frames, and the
trimmed mean deletes the signal. **The statistic that works is the per-ring-stop window
sum**: `-PerStopGlobals` records the evaluation counter at every 96-frame stop, so each
window carries its own exact evaluation count and the ratio is a direct price. It is
also stable, which is the check that it is measuring the right thing:

```text
r64tc, 96-frame windows with >= 1,000 evaluations
  632.8  658.8  664.5  666.5  666.8  668.0  673.8  676.5  676.9  705.3  714.3
  MEDIAN 668.0 tk per evaluation, spread 632.8-714.3 over 11 windows
```

Full table in `window-regression.txt`. The two independent repeat arms agree:

| arm | route word | evaluations/frame | added tk/frame | tk/evaluation |
|---|---:|---:|---:|---:|
| `r1tc` | 3 | 2.81 | +540…+5,591 | **not resolvable** |
| `r16tc` | 4,099 | 45.02 | +27,858 | 618.8 aggregate / **679.7** window-median |
| `r64tc` | 16,387 | 180.09 | +117,742 | 653.8 aggregate / **668.0** window-median |
| `r64t` | 16,385 | 90.04 | +31,598 | **348.2** window-median (transform only) |

`r16tc` and `r64tc` agree within 5% across a 4× change in evaluation density, which
bounds the once-per-driving-call term (the fetch of 2,784 B of kernel that nothing else
touches) at roughly **≤500 tk per call** and says it is amortised away at 16 repeats.
`r1tc` at 2.81 evaluations a frame is inside the session noise and **resolves nothing** —
stated rather than fitted, because a 1,987 tk/eval figure from one noisy arm is exactly
the shape of `a-residual-divided-by-a-count`.

**Split, from the transform-only arm:**

```text
transform only  (r64t)                 348.2 tk/evaluation
transform + compose (r64tc)            668.0 tk/evaluation  (blended, equal counts)
compose  = 2 x 668.0 - 348.2      =    987.8 tk/evaluation
```

---

## 5. THE EXCHANGE RATE, and the law under it

Gross is the soft-float **library** bill the conversion deletes, on the same convention
`CAMERA_Q20_12.md` §4 used, and it is confirmed two ways: from `SIMSIDE.md` §3's measured
marginal-80 per-call rates, and from this build's own disassembly, which shows the float
bodies making exactly **9 `__aeabi_fmul` + 9 `__aeabi_fadd`** and **36 + 27** `blx`
calls — the source's operation counts to the unit.

| body | gross tk/entry | fixed tk/entry | **R** | entries/fr | lane tk/fr | net tk/fr |
|---|---:|---:|---:|---:|---:|---:|
| `gmCollisionGetWorldPosition` | 289.6 | 348.2 | **0.83x** | 45.2 | 13,091 | **−2,650** |
| `func_ovl2_800ED490` | 987.3 | 987.8 | **1.00x** | 19.0 | 18,759 | **−10** |
| **both** | | | | | **31,850** | **−2,660** |

**A negative net means the conversion costs more than it deletes.**

`CAMERA_Q20_12.md` §4 notes that a library-only gross is generous to the fixed side,
because the float body's own self time belongs in the numerator too. Adding it (80
executed instructions per transform entry, 227 per compose, at the ~1.5 cyc/instruction
this build measures) gives **R = 1.00x and 1.17x**. Either convention, there is no lever
here.

### 5.1 Why — and it generalises

Solve the two measured costs against their executed instruction mixes and the edge
conversion falls out at **31–42 cycles**, against `__aeabi_fmul` at 26.5 and
`__aeabi_fadd` at 38. `ndsR2CollisionF32ToFixed` is the ~15-instruction exponent form
(the naive `(int)(v*4096+0.5f)` would be far worse) and `…FixedToF32` carries a 64-bit
`clz` and a round-to-nearest-even decision. **The cheap conversion is still one
soft-float operation.**

So for a leaf whose inputs and outputs are all `f32`:

```text
R  ~=  (26.5 n_mul + 38 n_add)  /  (31..42 n_conv + ~10 n_mac)
```

and the term that decides it is **conversions per deleted operation**, which is a
property of the signature:

| body | floats in | floats out | conv | ops | **conv/op** | R |
|---|---:|---:|---:|---:|---:|---:|
| `gmCollisionGetWorldPosition` | 15 | 3 | 18 | 18 | **1.00** | 0.83 measured |
| `func_ovl2_800ED490` | 24 | 12 | 36 | 63 | **0.57** | 1.00 measured |
| `gmCollisionSetInvertMatrix` | 12 | 12 | 24 | 61 | **0.39** | ~1.16–1.38 predicted |
| `guMtxCatF` → `ndsRendererMtxMul20p12` | — | — | **0** | 128 | **0** | 5.14 measured (prior) |

**That is why the 5.14x prior does not transfer and the reason is not warmth, not
transcendentals and not Thumb**: the prior compares two kernels that each already hold
their operand's native representation, so it pays no boundary at all. `SIMSIDE.md` §5
named the absence of transcendentals and the high entry rates as reasons to expect a
better rate than 1.70x; both are true of these two bodies and **neither mattered**,
because the term that dominates was not in the model.

### 5.2 The corollary, which is where the remaining value is

A CHAIN conversion holds `n_conv` fixed at the chain's endpoints while `n_op` grows with
its length. That is the whole design of `include/nds/nds_r2_collision_fixed.h`
("the representation crosses the float boundary exactly twice per joint per frame …
instead of once per call") and it is the only route these measurements leave open. The
one chain ever measured in this tree is the ring, at **2.68x**, and `EXCHANGE.md` §0.4
attributes its shortfall to four `__udivmoddi4` per entry from
`NDS_R2_CFX_DIV64` — **still the portable C divide; the DS hardware divider hook at
`nds_r2_collision_fixed.h:205` has never been defined.** That is an unpriced,
bit-exact-by-construction item, not a claim.

---

## 6. Equivalence, graded live at zero risk

The `rgrade` arm (route word 7) computed the decomp float body's own answer and the fixed
answer from the **same inputs** on every captured call, and binned the difference in Q12
world quanta. 2,684 calls × 3 components = 8,052 graded values, whole match:

| bucket | meaning | count |
|---|---|---:|
| `Dev0` | exactly equal | **6,241** (77.5%) |
| `Dev1` | 1–4 quanta (≤ 0.00098 world units) | **1,811** (22.5%) |
| `Dev2` | 5–16 quanta | 0 |
| `Dev3` | 17–81 quanta (still inside the 0.0200 bound) | 0 |
| `Dev4`/`Dev5` | over the bound | **0** |
| `MaxDevQ12` | worst single component, whole match | **1** |

**Maximum deviation over the whole match is one Q12 quantum = 0.000244 world units,
against the cluster's standing 0.0200 bound — 82x inside it.** `XfrmDecline` and
`CmpsDecline` are **0** on every arm, so no live joint matrix left the kernel's domain.

Two things this does and does not settle, stated plainly:

- **It settles the arithmetic.** The Q26/Q12 kernels reproduce the decomp float bodies on
  the live domain far inside the bound they were designed to, and it is measured on the
  gate match rather than on captured matrices replayed on the host.
- **It does not settle a conversion**, because the graded population is the
  damage-gateway's 2,684 calls, not `gmCollisionGetWorldPosition`'s ~92,000. It is a
  live-domain result over one seam, not coverage of the call set. And it is moot for the
  lever question either way: at R ≤ 1.00 there is nothing to trade fidelity *for*.

---

## 7. What this does to `POSITION.md`'s 0.563x

```text
POSITION.md fidelity-neutral inventory            53,215   0.563x

SIMSIDE.md's projection for the warm MAC subset (71,491 tk/fr):
    at 1.70x                                      29,437   0.312x  ->  0.875x
    at 2.68x                                      44,806   0.474x  ->  1.037x
    at 5.14x                                      57,584   0.609x  ->  1.173x

MEASURED on its two largest members, 31,850 of 71,491 = 44.6% of the subset:
    0.83x and 1.00x                               -2,660  -0.028x  ->  0.535x
```

**The projection is refuted for the leaf route.** The inventory stays **0.563x**, and the
44.6% of the warm-MAC subset that has been measured is worth **less than nothing**.

What is NOT refuted, and must not be read as refuted:

1. **The chain route.** Nothing here measures a chain. The ring's 2.68x stands, and its
   named cause — a bit-by-bit 64-bit divide where the DS has a free hardware unit — is
   still unaddressed.
2. **The other ten members of the subset**, whose signatures were not enumerated. The
   model in §5.1 predicts each of them from its signature with no build; the one worked
   example, `gmCollisionSetInvertMatrix` at conv/op 0.39, predicts ~1.2–1.4x. None of
   the fourteen is a chain interior, so none of them is predicted above ~1.5x.
3. **Column N** (bit-exact helper acceleration, `SIMSIDE.md` §6). This measurement is
   evidence *for* it, not against: the thing that turned out to be expensive is the
   representation boundary, and Column N never crosses one.

---

## 8. What this cycle did NOT do

- **No conversion was implemented and nothing was wired.** The shadow computes and
  discards; both arms run the decomp float body.
- **No fidelity decision was taken or asked for.** At R ≤ 1.00 there is nothing to ask.
- **The intercept is not measured.** `r1tc` at 2.81 evaluations/frame is inside session
  noise; the once-per-call term is bounded at ≤ ~500 tk from the `r16tc`/`r64tc`
  agreement and is not fitted. Because that term would be paid **per entry** at the real
  spread rate, 348.2 and 987.8 are **lower bounds** on the real per-entry cost and
  0.83x/1.00x are **upper bounds** on R.
- **Two disclosed biases, both upward on the fixed cost.** The compose shadow refreshes a
  twelve-float file-static per evaluation (~12 tk) that a real compose would not pay, and
  its second operand is a warmer object than the real one. Neither is quantified.
- **The graded population is the damage gateway, not the whole call set** (§6).
- **`gmCollisionGetFighterPartsWorldPosition` and the ten remaining warm-MAC members were
  not measured**, only modelled.
