# The simulation side of the soft-float class is 142,786 tk/fr, three quarters of it is multiply-accumulate, and 71,491 of it is warm MAC in fourteen functions

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **HEAD `df035595bdc`**
**Zero builds, zero emulator runs.** Every figure is derived from a profile capture and a
linked ELF already on disk, by the same method that produced `DRAW_FIXEDPOINT.md`.
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.

```text
REQUIREMENT  +94,481 net ticks per presented frame at rank-80.  Basis:
             build-c206-shipgx0, rank-80 raw 1,239,808 / net 1,214,861 against
             the 1,120,380 gate; SHIPPING renderer (GX_COMPOSE 0), bore 0,
             mode 163 one-minute match, 1,600 samples, frames 439-2038,
             slips=0 (../2026-08-16_gap-position/POSITION.md section 1).
             Every ratio below divides by that number.

SIZE         SIM-ONLY + SIM+DISPATCH + SHARED soft float AND sqrtf, marginal-80:
             142,786 tk/fr = 1.511x the requirement.  Split shared 59,582 /
             sim-only 45,539 / sim+dispatch 37,665.  That is 4.19x the whole
             draw-side lane and 2.68x POSITION.md's entire fidelity-neutral
             inventory.  Section 2.

SHAPE        AND IT IS THE OPPOSITE SHAPE TO THE CAMERA CHAIN.  75.9% of the
             cycles are fmul + fadd + fsub; fdiv is 9.7% and sqrtf 5.7%.  The
             camera chain -- the lane that measured 1.70x rather than the 5.14x
             MAC prior -- needs 3 roots and 9 divides per entry.  This one does
             not.  Section 3.

BEST         14 functions are >=80% MAC by cycles AND entered >=8 times a frame:
SUBSET       71,491 tk/fr = 0.757x of the requirement, in 5 collision bodies
             (50,044), 3 animation bodies (13,904) and 5 math leaves (7,541).
             The two largest -- func_ovl2_800ED490 at 18,759 and
             gmCollisionGetWorldPosition at 13,091 -- are PURE 100% MAC, 580 B
             and 196 B, at 19.0 and 45.2 entries a frame.  They are the exact
             shape and warmth of the in-binary 5.14x pair.  Section 4.

RATE         NOT ASSERTED.  Three rates have been measured in this binary and
             they disagree by 3x: 1.70x (camera chain, in situ, transcendental-
             heavy, 8.1 entries/frame), 2.68x (fighter narrow phase, in situ,
             __udivmoddi4 four times per entry, 0.97 entries/frame), 5.14x
             (guMtxCatF vs ndsRendererMtxMul20p12, same operation, same build,
             18.55 entries/frame).  BOTH named causes of the two low rates are
             absent from the warm MAC subset; that is an argument, not a
             measurement.  The subset is worth 29,437 (at 1.70) to 57,584 (at
             5.14) tk/fr -- 0.312x to 0.609x.  Section 5.

COLUMN       THE FIDELITY QUESTION IS NOT SETTLED BY THIS DOCUMENT AND IS NOT
             UNIFORM.  Three columns, sections 6-7:
               N  bit-exact helper acceleration -- fidelity-neutral BY
                  CONSTRUCTION, no proof obligation beyond the helper.
                  Unsized: fdiv 13,818 + sqrtf 8,068 = 21,886 tk/fr of surface,
                  conversion unmeasured.
               P  provable-equivalence fixed point -- the warm MAC subset.
                  Belongs in the fidelity-neutral column IF AND ONLY IF the
                  decision-margin proof lands.  The instrument for it already
                  exists in-tree and has been run once.
               T  rung-3 gameplay-fidelity trades -- everything else.

BLOCKED      Nothing is chosen, nothing is implemented, no source changed, no
             ROM built.  Section 8.
```

---

## 1. Basis, stated once

| | |
|---|---|
| arm | `builds/build-c200-trackprof-off` |
| target | `smash64ds-battle-playable-tickhud-hwtri` |
| config | `BOTH_CPU=1 BATTLEPACK=1 KEEP_CACHE=1 GX_COMPOSE=1 TASK37_PROFILE=1 TICK_HUD_DRAW=0`, DLDI on |
| capture | `../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv` (per-PC, 1,601 regions) |
| census | `../2026-08-15_drawside-softfloat/c200off-census-marg80.json` (marginal mask) |
| phase labels | `../2026-08-15_drawside-softfloat/c200off-softfloat-phase-marg80.json` |
| mask | **marginal-80**, threshold 1,224,970 ticks, axis `total_cycles − halt_wait` |
| tick divisor | 160 (= 2 cycles/tick × 80 marginal frames) |
| whole-match divisor | 3,202 (1,601 regions × 2) |

**The mask matches the bank**: this arm's own rank-80 is 1,224,970 against the bank's
1,226,624, inside the ≥14,080 cross-build floor (`DRAW_FIXEDPOINT.md` §0).

**This is the same capture and the same tool the draw-side classification used**, extended
with a per-`(caller, helper)` matrix that `--json` collapsed away. Reproduce:

```powershell
arm-none-eabi-objdump -d builds/build-c200-trackprof-off/smash64ds-battle-playable-tickhud-hwtri.elf > c200-off.dis
python scripts/analyze-leaf-helper-attribution.py --pc-csv ...\c200-off-pc.csv --mask marginal `
  --census ...\c200off-census-marg80.json --dis c200-off.dis --helpers softfloat `
  --matrix-json c200off-softfloat-matrix-marg80.json
python scripts/analyze-leaf-helper-attribution.py ... --helpers sqrtf `
  --matrix-json c200off-sqrtf-matrix-marg80.json
```

`--matrix-json` was added this cycle for exactly this reason: **the helper axis is the axis
that decides whether a lane converts**, and collapsing it is what made the camera chain and
the collision ring look like the same lever.

**Do NOT pass `--no-show-raw-insn` to objdump** and **do NOT pass `--thunk`** for soft
float — both traps are recorded in `DRAW_FIXEDPOINT.md` §8 and both produce a plausible
wrong table rather than an error. The disassembly used here has 3,807 function labels,
above `classify-softfloat-caller-phase.py`'s 1,000-label refusal floor.

**The method reproduces two known bodies to the unit**, which is the check that the counts
are dynamic call counts and not estimates:

| body | source | float ops in the source | measured ops/entry |
|---|---|---:|---:|
| `gmCollisionGetWorldPosition` | `gm/gmcollision.c:196-205` | 9 fmul + 9 fadd = 18 | **18.0** |
| `func_ovl2_800ED490` | `gm/gmcollision.c:208-224` | 36 fmul + 27 fadd = 63 | **63.0** |

---

## 2. The lane

```text
sim-only + sim+dispatch + shared, marginal-80, soft float AND sqrtf

  shared                     59,582 tk/fr   (57,521 soft float + 2,061 sqrtf)
  sim-only                   45,539         (39,537 + 6,002)
  sim+dispatch               37,665         (37,662 + 4)
                            --------
  TOTAL                     142,786 tk/fr = 1.511x of +94,481
```

The soft-float halves are `DRAW_FIXEDPOINT.md` §1's own numbers, unchanged. The 8,066 of
`sqrtf` is new to this document only in being attributed per caller; its total was already
published there (§3: sim-only 6,004 + shared 2,061).

**The brief's 77,199 is the sim-only + sim+dispatch soft float.** With `sqrtf` it is
83,205, and with the shared half it is 142,786. The shared half is not optional context:
its two largest members are collision matrix bodies reached from both phases, so a
conversion has to decide about them either way.

### 2.1 Concentration — and the warning that goes with it

**Call volume on the marginal-80 frames is 4.96× the whole-match rate**
(9,092.2 calls/frame against 1,834.6; per helper, `fmul` 7.38×, `fadd` 5.79×, `f2iz`
6.25×, `sqrtf` 4.75×, compares 2.3–2.7×). Whole-match call counts come from the same
pc-csv's `all_instructions` column; only the counts are mask-exact, so this is a **call**
concentration and not a cost concentration — pricing whole-match calls needs an all-mask
census, which was not built.

Two consequences, both load-bearing:

1. **Every figure in this document is already a rank-80 figure**, because the mask is the
   marginal-80 population. Do not apply a concentration multiplier on top; it is baked in.
2. **A whole-match or soak measurement of any conversion here will under-read it by about
   5×.** `POSITION.md` §4 makes the same observation about `SRC`: the frames that set
   rank-80 are the simulation-heavy frames. That is favourable for the gate and hostile to
   the instrument — a candidate judged on a whole-match mean would look five times weaker
   than it is at the percentile the gate is defined on.

---

## 3. Op mix, and why it is the opposite of the camera chain's

Per-call costs are measured (the helper's own marginal cycles ÷ its own marginal calls),
with `__aeabi_fsub`'s calls folded into `__aeabi_fadd`'s divisor because `fsub` is a
4-byte thunk that falls into it.

| helper | tk/fr | share | calls/fr | tk/call |
|---|---:|---:|---:|---:|
| `__aeabi_fmul` | 42,764 | 29.9% | 3,232.7 | 13.23 |
| `__aeabi_fadd` | 41,822 | 29.3% | 2,203.8 | 18.98 |
| `__aeabi_fsub` | 23,774 | 16.7% | 1,252.8 | 18.98 |
| `__aeabi_fdiv` | 13,818 | **9.7%** | 233.5 | **59.19** |
| `sqrtf` | 8,068 | **5.7%** | 55.8 | **144.62** |
| `__aeabi_fcmpeq` | 3,409 | 2.4% | 644.6 | 5.29 |
| `__aeabi_fcmplt` | 2,903 | 2.0% | 397.7 | 7.30 |
| `__aeabi_i2f` | 2,044 | 1.4% | 250.5 | 8.16 |
| `__aeabi_f2iz` | 1,332 | 0.9% | 206.2 | 6.46 |
| `__aeabi_fcmpgt` | 1,050 | 0.7% | 142.2 | 7.38 |
| `__floatsisf` / `fcmple` / `ui2f` / `fcmpge` | 1,803 | 1.3% | 472.5 | 2.0–7.4 |
| **TOTAL** | **142,786** | | **9,092.2** | |

**MAC (fmul + fadd + fsub) is 75.9% of the cycles. Transcendental (fdiv + sqrtf) is 15.4%.**

`CAMERA_Q20_12.md` §4.1 names the mechanism that dragged the camera chain from a 5.14×
prior to a measured 1.70×: *"per look-at entry it needs three square roots and nine
divisions, plus five more divisions per perspective … the lane's transcendental half
converts at a much worse rate than its arithmetic half, and the camera chain is the most
transcendental-heavy member of the lane."* This lane is the mirror image of that
sentence. **That is a reason to expect a better rate; it is not a measurement of one.**

---

## 4. Ranked by shape, because shape is what decides these lanes

Three buckets, by the helper the caller's cycles actually go to, cross-tabulated with the
caller's own exact entry-PC execution rate:

| bucket | tk/fr | fns | warm ≥8 entr/fr | mid 2–8 | cold <2 |
|---|---:|---:|---:|---:|---:|
| **MAC ≥80% of cycles** | **82,798** | 64 | **71,491** | 10,567 | 740 |
| mixed | 30,199 | 115 | 17,875 | 5,904 | 6,421 |
| transcendental ≥30% | 29,789 | 31 | 20,069 | 9,299 | 422 |

### 4.1 The warm MAC subset — 14 functions, 71,491 tk/fr

| caller | phase | tk/fr | entr/fr | calls/fr | ops/entry | bytes | op mix per entry |
|---|---|---:|---:|---:|---:|---:|---|
| `func_ovl2_800ED490` | shared | **18,759** | 19.0 | 1,195.4 | 63.0 | 580 | 36 fmul, 27 fadd |
| `gmCollisionGetWorldPosition` | shared | **13,091** | 45.2 | 812.9 | 18.0 | 196 | 9 fmul, 9 fadd |
| `gmCollisionSetInvertMatrix` | sim-only | **10,700** | 11.3 | 692.4 | 61.0 | 716 | 42 fmul, 13 fsub, 4 fadd, 1 fdiv |
| `gmCollisionTransformMatrixAll` | shared | 6,453 | 22.5 | 489.2 | 21.8 | 430 | 14.8 fmul, 2 fsub, 2 fadd, 3 fcmpeq |
| `ndsBaseGcPlayMObjMatAnim` | sim+dispatch | 5,741 | 85.3 | 438.8 | 5.1 | 732 | 1.8 fmul, 1.2 fadd, 1.4 fcmpeq |
| `ndsR2FtAnimParseDObjFigatree` | sim+dispatch | 5,234 | 93.4 | 275.8 | 3.0 | 3,016 | 1.6 fsub, 1.4 fadd |
| `gmCollisionGetFighterPartsWorldPosition` | shared | 5,308 | 3.5 | 329.6 | 94.2 | 304 | 47 fmul, 47 fadd |
| `lbCommonCos` | shared | 4,519 | 83.8 | 419.2 | 5.0 | 64 | 2 fmul, 1 fadd, 1 f2iz, 1 ui2f |
| `ndsBaseGcPlayDObjAnimJoint` | sim+dispatch | 2,929 | 62.1 | 242.5 | 3.9 | 500 | 1.1 fmul, 0.9 fadd, 1.4 fcmpeq |
| `ndsMPLineExtentSweepRejects` | sim+dispatch | 1,040 | 41.0 | 54.8 | 1.3 | 480 | fmul/fadd only |
| `__kernel_cosf` / `__kernel_sinf` | shared | 1,555 | 9.1 | 105.9 | 5.8 | 652 | fmul, fadd |
| `syVectorAdd3D` / `syVectorDiff3D` | shared | 1,467 | 12.8 | 77.3 | 3.0 | 82 | 3 fadd / 3 fsub |

*(`gmCollisionGetFighterPartsWorldPosition` is listed here for family completeness; at
3.5 entries/frame it is on the mid side of the warmth cut and is counted in the 10,567
mid row, not the 71,491.)*

By family, the warm MAC subset is: **collision/MP 50,044 tk/fr (5 fns) · animation 13,904
(3 fns) · math leaves 7,541 (5 fns)**.

### 4.2 What does NOT convert, and why — stated so nobody re-prices it

| caller | tk/fr | entr/fr | why it is the wrong shape |
|---|---:|---:|---|
| `gmCollisionTestRectangle` | 10,603 | 24.4 | **3 fdiv per entry**, 40.9% transcendental. This is the decision body itself |
| `func_ovl2_800EDE5C` | 7,568 | 24.4 | **1.4 sqrtf per entry**, 65.1% transcendental — and it is already converted behind `NDS_R2_COLLISION_FIXED` |
| `ndsStageMPAdjustFloorLoopWallSweep` | 6,946 | 25.5 | 1 fdiv/entry, 20.7% transcendental, mixed |
| `ftComputerCheckDetectTarget` | 4,697 | **1.4** | 200 ops/entry but entered **1.4×/frame** — the collision ring's exact cold-bytes failure mode |
| `syUtilsArcTan.part.0` | 2,366 | 4.6 | **6.1 fdiv per entry**, 69.8% transcendental |
| `syVectorMag3D` / `syVectorNorm3D` | 3,763 | 8.5 / 5.7 | 1 sqrtf per entry each, 62–65% transcendental |
| `gmCameraGetClampDimensionsMax` | 801 | 2.0 | 73.9% transcendental |

---

## 5. The exchange rate, and why this document does not assert one

Three rates have been measured **in this binary**, and they span 3×:

| rate | what it was measured on | entries/frame | transcendental per entry | source |
|---:|---|---:|---|---|
| **1.70x** | camera + projection chain, in situ, whole match | 8.1 | 3 sqrt + 9 div per look-at | `../2026-08-16_camera-fixedpoint/CAMERA_Q20_12.md` §4 |
| **2.68x** | fighter narrow phase, in situ, whole match | 0.97 | `__udivmoddi4` ×4 per entry | `../2026-08-15_cfx-narrow-exchange/EXCHANGE.md` §0 |
| **5.14x** | `guMtxCatF` → `ndsRendererMtxMul20p12`, same operation, same build | 18.55 | none | `DRAW_FIXEDPOINT.md` §4 |

**Both named causes of the two low rates are absent from the warm MAC subset**: it has no
transcendental by construction (≥80% MAC), and its entry rates are 11–93 per frame against
the collision ring's 0.97 and the camera chain's 8.1. `EXCHANGE.md` §0.4 is explicit that
what decides is *"(a) how much float is in the lane at all, and (b) what the fixed form
calls"*, and §0.5 that even the most optimistic residency fix only took the narrow phase to
1.29 — because its fixed form called a bit-by-bit 64-bit divide 4 times per entry.

**That is an argument from mechanism, not a measurement, and it is stated as one.** The
range it supports:

```text
warm MAC subset          71,491 tk/fr
  at 1.70x   71,491 x (1 - 1/1.70) =  29,437 tk/fr   0.312x of +94,481
  at 2.68x   71,491 x (1 - 1/2.68) =  44,806          0.474x
  at 5.14x   71,491 x (1 - 1/5.14) =  57,584          0.609x

whole sim+shared lane   142,786 tk/fr
  at 1.70x                              58,794        0.622x
  at 2.68x                              89,507        0.947x
  at 5.14x                             115,004        1.217x
```

**Nothing here closes the gap on its own at any credible rate**, and the whole lane at the
optimistic rate only just exceeds it. This is a large lever, not a closing one.

---

## 6. Which column each candidate lands in

`PROJECT_GOAL.md` requires mechanical equivalence, not bit-exactness, and equivalence is a
**provable property, not a sacrifice** — but it is provable to different standards for
different candidates, and this is where the sim side splits.

### Column N — bit-exact helper acceleration. Fidelity-neutral BY CONSTRUCTION.

Replacing a soft-float helper with a faster implementation that returns the **identical
IEEE-754 single-precision result** changes nothing anywhere: no caller, no consumer, no
decision, no state hash. The proof obligation is local to the helper, and this repo has
already discharged exactly that shape once — `ftAnimGetTargetValue`'s replacement is
verified *exhaustively*, "checked 524288 (all 65,536 s16 × 8 track ids) … bit-identical on
every input" (`check_ftanim_target_exact.py`, quoted from a Boundary log this cycle).

The surface, on the sim+shared side alone:

| helper | tk/fr | calls/fr | tk/call today | the DS unit that could serve it |
|---|---:|---:|---:|---|
| `__aeabi_fdiv` | 13,818 | 233.5 | 59.19 | `REG_DIVCNT` 64/32, ~26–36 cycles |
| `sqrtf` | 8,068 | 55.8 | 144.62 | `REG_SQRTCNT` 64-bit, ~13 tk |
| **total** | **21,886** | | | |

Plus **7,900 tk/fr on the draw side** (`fdiv` 4,360 + `sqrtf` 3,540, `DRAW_FIXEDPOINT.md`
§2.1/§3) — and the draw side needs no fidelity decision for a bit-exact replacement either,
which means this is the one item on the board that spans both halves without asking the
owner for anything.

**It is UNSIZED and this document does not claim it converts.** The open question is
narrow and answerable with no build: *can a correctly-rounded IEEE single divide and square
root be built on the DS hardware units in fewer than 59.19 and 144.62 ticks including
register setup, polling, normalisation and the rounding decision?* Both units are unused in
this tree — `NDS_R2_CFX_DIV64` and `NDS_R2_CFX_ISQRT64` are still undefined hooks
(`include/nds/nds_r2_collision_fixed.h:205,215`), and `EXCHANGE.md` §0.4 records that
*"nothing ever overrode them"*. They are also a **single shared register set**, so the
first user owns the re-entrancy seam.

### Column P — provable-equivalence fixed point. The warm MAC subset.

The five collision bodies (50,044 tk/fr) feed the hit decision, and the three animation
bodies (13,904) feed joint matrices which feed hurtbox positions. Converting them changes
numbers that gameplay reads. **That does not automatically make them a rung-3 trade** —
what matters is whether the *decision* they produce can be shown identical.

It can, and the design that makes it structural rather than statistical already exists in
this tree:

- **`NDS_R2_CFX_NARROW_DECLINE`** (`src/import/battleship_gmcollision.c:225`): the fixed
  narrow phase **declines** and falls through to the untouched float body whenever its
  input is out of domain. An arm that declines whenever the decision margin is inside the
  kernel's proven error bound is *provably* identical in-domain and *byte-identical*
  out-of-domain. Equivalence stops being a coverage argument.
- **`scripts/grade-r2-collision-live-domain.c`** already grades the shipping fixed kernels
  against transcriptions of the decomp float originals (`gmCollisionSetInvertMatrix`,
  `gmCollisionGetWorldPosition`) **plus a double-precision exact reference**, on matrices
  captured from a live gate-arm match by `scripts/probe-collision-fixed-domain.ps1`, to a
  bound of 0.0200 world units.
- **`NDS_R2_COLLISION_L7_ORACLE`** samples the per-frame hit-decision latches.
- **The seventeen match invariants** (`CAMERA_Q20_12.md` §5.2) are the end-to-end control:
  `P1Damage / spark / shield / AObj / packHits / runaway / Task36 / parse / heap-min /
  arena / allocFail / resident / loadFails`.

**So the proof cost is not "build a new instrument".** It is: an analytic error bound for
each Q-format kernel (desk work), one margin-histogram counter run to establish that the
live minimum decision margin exceeds that bound (one build, one match), and the decline
path for the frames where it does not.

**Two things that must NOT be converted, and neither is expensive to exclude:**

| must stay exact | why | its size in this lane |
|---|---|---:|
| `syUtilsRandFloat` | one LCG shared across 135 draw sites — 65 in the level-3 AI, 44 in `efmanager.c`, 26 in `lbparticle.c` (`POSITION.md` §4.1). Perturbing its *draw sequence* desynchronises the AI | **229 tk/fr** — 0.16% of the lane |
| integer frame counters (hitstun, shieldstun, stale-move queue, invulnerability, ledge cooldown, respawn, match timer) | they are integers in the source and are not in this lane at all | **0** |

### Column T — rung-3 trades

Everything else: the transcendental-heavy 29,789 and the mixed 30,199, minus whatever
column N takes out of them. These are the bodies where the *value*, not the decision, is
consumed — `ftComputerCheckDetectTarget`'s distance arithmetic, `syUtilsArcTan`'s angle,
`gmCameraUpdateInterests`. A numerically different angle is a different fight.

### The coverage caveat, stated correctly

The owner has played SSB64 since childhood and their verdict on **feel** — knockback,
spacing, timing, telegraphs — is expert testimony and the acceptance gate. What a play
session cannot cover is a **coverage** question, never an acuity one: a divergence that
fires rarely, in a situation the session did not reach, or that accumulates over a longer
match than the one played. That is precisely what the decline path and the margin bound are
for, and it is why an equivalence instrument has to be designed **before** the first build:
`[[route-ab-cannot-price-a-gameplay-change]]` — one route A/B on collision ended with damage
130/51 against 33/65 on the same ELF, one poked bit apart.

---

## 7. The honest number, and what it does to 0.563x

`POSITION.md` §6 puts the fidelity-neutral inventory at **53,215 tk/fr = 0.563x**, and its
rows are `gcRunAll` scheduler machinery, the texture-bind collapse, in-match asset I/O,
`ndsRendererSyncTextureTile`, the newlib `_svfiprintf_r` residual and the no-Z repeat
matrix loads. **None of those is a soft-float caller**, and this attribution charges the
*caller of the float helper*, never the dispatcher above it — so the sets are disjoint by
construction. That was not exhaustively cross-checked.

```text
POSITION.md fidelity-neutral inventory            53,215   0.563x

+ column P, warm MAC subset, IF the margin proof lands
    at 1.70x                                      29,437   0.312x  ->  82,652   0.875x
    at 2.68x                                      44,806   0.474x  ->  98,021   1.037x
    at 5.14x                                      57,584   0.609x  -> 110,799   1.173x

+ column N, bit-exact helper acceleration (UNSIZED surface 21,886 sim +
  7,900 draw = 29,786; conversion unmeasured, could be 0)
```

**So the answer to "is the fidelity-neutral inventory incomplete?" is yes, materially.**
The inventory was 0.563×. With the warm MAC subset moved into it — which requires a proof
this cycle did not run, not an owner decision — it becomes **0.875x to 1.173x**, and at the
middle rate it closes.

**What that does NOT mean.** It is not a claim that the gate closes. Three things stand
between the arithmetic and a tick:

1. **The rate is projected, not measured.** The 1.70× and 2.68× measurements both landed
   *below* the shape argument's prediction, twice, on this exact board.
2. **The proof has not been run**, and if the live decision margin turns out to be smaller
   than the kernel error bound on a meaningful fraction of evaluations, the decline path
   fires and the conversion's coverage — and therefore its saving — drops by that fraction.
3. **Bytes are not free and this subset is not small**: 6,798 B of caller code across the
   14 functions, and the campaign has twice measured added `.main` bytes inverting a win
   (`CAMERA_Q20_12.md` §6: +3,032 B turned −4,736 into +1,600).

---

## 8. What this cycle did NOT do

- **No sim-side conversion was implemented, and none was measured.** Sizing and feasibility
  only, as briefed.
- **No build, no emulator run, no flag flipped, no production source edited** for this
  section. The only code changes this cycle are `--matrix-json` on
  `analyze-leaf-helper-attribution.py` and the `verify-all.ps1` toolchain gate.
- **Column N is a named surface, not an item.** Nobody has written a correctly-rounded
  hardware-unit `fdiv`/`sqrtf` for this tree, and until one is measured its 29,786 tk/fr of
  surface converts at an unknown rate that may be zero or negative.
- **The concentration figure is a CALL concentration**, not a cost concentration; pricing
  whole-match calls needs an all-mask census that was not built.
- **The disjointness from `POSITION.md`'s inventory is argued from the attribution method,
  not enumerated.**
- **`func_ovl2_800ED490` and `gmCollisionGetWorldPosition` are not interceptable by the
  `#define`-before-`#include` rename** that the ring uses — they have fifteen in-TU call
  sites in `gmcollision.c` (`EXCHANGE.md` §1). That is a constraint on a **same-binary
  A/B route**, not on an implementation: moving the definition and its call sites together
  is what a conversion wants. It does mean the first measurement of this subset cannot use
  the campaign's preferred zero-placement-floor route, and must budget the ≥14,080 rank-80
  cross-build floor instead.
