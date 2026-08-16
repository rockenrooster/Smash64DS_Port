# The draw-only soft-float lane is 34,178 tk/fr, it is FLAT, and the collision lane's 1.00 exchange rate does NOT transfer — measured, on one instrument, at 5.14x

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **HEAD `b16dc16997a`**
**Zero builds, zero emulator runs.** Every figure is derived from a v3 capture already on
disk (`../2026-08-15_ftanim-dispatch-attribution/`) plus the linked ELF of the arm that
produced it. **UNITS: 2 profile cycles = 1 project tick.** Every table states its window.

```text
GATE      THE ARITHMETIC GATE IS PASSED, NOT A STOP.  The conservative removable
          ceiling after replacement cost is ~24,800 tk/fr against the brief's
          16,000 floor.  Lowest of three independent routes; section 5.

LANE      DRAW-ONLY + DRAW+DISPATCH soft float = 30,638 tk/fr at rank-80
          (27,316 whole match, concentration 1.12x -- FLAT), 2,271.9 helper
          calls/frame, 872 static call sites, 35 functions, 43,214 B of caller
          code.  Plus draw-side `sqrtf` 3,540.  TOTAL 34,178 = 0.420x of +81,297.
          Section 2.  The stale "31.2% x 94,602" shortcut is NOT used anywhere.

PRICE     THE DECIDING MEASUREMENT, and it is the collision lane's falsifier:
          the SAME OPERATION exists in this binary in both forms.  A float 4x4
          concat (`guMtxCatF`) costs 2,921 tk/call; the port's 20.12 concat
          (`ndsRendererMtxMul20p12`) costs 568.40 tk/call.  Same mask, same
          build, both live.  RATIO 5.14x.  Section 4.

WHY IT    The float library is ITCM-RESIDENT (all fourteen helpers, 0x01ff8000
DIFFERS   region, `Task 9 float ITCM passed`).  That is why it is cheap to reach
          and why the collision ring lost: the ring put its replacement in
          `.main` at 0.97 entries/frame and paid compulsory fetch.  The draw
          replacement does NOT have to: ITCM has 2,976 B free, a Q12 leaf set
          fits, and every non-multiply replacement (add/sub/compare/shift) is
          INLINE AND SMALLER than the `bl` it deletes.  Section 4.3.

REFUSED   No design here introduces `__udivmoddi4` or a generic 64-bit divide or
          sqrt loop.  The DS hardware divide and sqrt units are UNUSED in this
          tree -- `NDS_R2_CFX_DIV64`/`ISQRT64` are still undefined, confirmed by
          search over `src/` and `include/` -- so 73.7 fdiv/frame and 24.3
          draw-side sqrtf/frame have an uncontended hardware seam.  Section 6.

NOT DONE  NO IMPLEMENTATION.  Stated plainly rather than discovered: the package
          that clears 16K is the whole draw-side conversion -- 35 functions, five
          of them decomp-derived `syMatrix*`/`guMtx*` bodies -- and it needs a
          Q12 leaf set, a hardware-divider seam that has never existed here,
          engagement counters, a 1,600-frame A/B, a frame-locked pixel pair and
          OWNER VISUAL ACCEPTANCE.  That does not fit one cycle and it is not
          started half-way.  Section 7 hands it forward, sized, with its first
          build named.
```

---

## 0. Basis, stated once

| | |
|---|---|
| arm | `builds/build-c200-trackprof-off` |
| target | `smash64ds-battle-playable-tickhud-hwtri` |
| config | `BOTH_CPU=1` `BATTLEPACK=1` `KEEP_CACHE=1` `GX_COMPOSE=1` `TASK37_PROFILE=1` `TICK_HUD_DRAW=0`, `FTANIM_TRACK=1`/`DISPATCH=0` (dense path inert), DLDI on |
| capture | `../2026-08-15_ftanim-dispatch-attribution/v3-off/arm9-profile.csv`, 1,601 regions |
| per-PC reduction | `../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv` |
| mask | **marginal-80**, threshold **1,224,970 ticks**, axis `total_cycles − halt_wait` |
| whole-match divisor | 3,202 (1,601 regions × 2 cycles/tick) |
| requirement | **+81,297** net (`build-c200-bank84`, rank-80 1,226,624 raw / 1,201,677 net) |

**The mask matches the bank.** This arm's own rank-80 is 1,224,970 against the bank's
1,226,624 — 1,654 apart, inside the ≥14,080 cross-build floor. So "marginal-80 here" and
"the frames that set the gate" are the same population.

**`TICK_HUD_DRAW=0` does not remove the game's draw.** It gates the tick-HUD's own
on-screen output only; every renderer symbol below is live and the stage/fighter draw
counters are unchanged.

**Three tool defects were hit and fixed before any number below was believed** —
section 8. One of them (`--no-show-raw-insn`) silently produced a *three-caller* table
instead of failing, and `analyze-leaf-helper-attribution.py` now refuses that input.

---

## 1. The whole soft-float bill, split by frame phase

Attribution is exact, not sampled: every `bl`/`blx`/`b` into a helper is an instruction
with its own PC, and the profile reports that PC's execution count. Helper cycles come
from the **same marginal mask** as the counts (`census-marginal-frame-owners.py
--census-out`), so no two populations are mixed.

Phase comes from the **linked ELF**, not from names. Each caller's static ancestors are
walked to the set of functions with no caller of their own; those roots are labelled
DRAW / SIM / DISPATCH from an entry-point table, and the graph is first **pruned to code
that executed in this match** (1,363 live of 3,803 linked) so dead scenes cannot inflate
a class.

```text
soft-float total, marginal-80          168,060 tk/fr
  shared                     57,521    34.2%   15 fns
  sim-only                   39,537    23.5%  148 fns
  sim+dispatch               37,662    22.4%   47 fns
  DRAW+DISPATCH              17,407    10.4%   20 fns
  DRAW-ONLY                  13,231     7.9%   15 fns
  unresolved                  2,702     1.6%    6 fns
```

**Cross-check, independent of the attribution.** Summing the helpers' own PC ranges
directly from `nm` gives **165,187 tk/fr marginal-80 / 78,460 whole match**, against the
attribution's 168,060 — 1.7% apart, the difference being the `fsub` thunk fold charging
its callers at fadd's rate. **The whole soft-float class concentrates 2.11x. The draw-side
subset concentrates 1.12x** (§2), so the concentration lives in the collision half, not
here — which is exactly why the draw side converts against a *level* gate and the collision
side did not.

- **`draw-only`** = every live static root is a draw root.
- **`draw+dispatch`** = every live *hard* root is a draw root; the remainder is
  `syTaskmanRunTask` / `syMainThread`, which dispatch both halves through a function
  pointer and therefore say nothing. **Both classes are convertible**; neither has a
  simulation root.
- **`shared`** is real and is 15 functions, 57,521 tk/fr, dominated by
  `func_ovl2_800ED490` (23,829) and `gmCollisionGetWorldPosition` (17,113) — collision
  matrix code reached from both phases. **Out of scope by the brief and by
  `PROJECT_GOAL.md`**: converting it touches simulation.
- **`unresolved` 2,702** is library-internal (`__floatsisf`, `__aeabi_ui2f`) plus
  `.vectors`. Not a lever.

**Two rows moved class when the labeller was corrected, and it mattered by 26,000 tk/fr.**
A first pass sorted roots by module prefix, which put `efManagerShieldProcDisplay` and
`efManagerImpactWaveProcDisplay` in SIM — they are **display** callbacks — and that alone
dragged the entire renderer/matrix/particle cluster into `shared`. A GObj callback's role
is in its suffix, not its module. The role rules now run before any prefix rule and the
tool documents why.

---

## 2. The draw-side lane, by category

Marginal-80 unless stated. `entr/fr` is the caller's own **exact** entry-PC execution
rate; `calls/fr` is its soft-float calls.

| category | gross tk/fr | whole | entr/fr | calls/fr | sites | bytes | fns |
|---|---:|---:|---:|---:|---:|---:|---:|
| **camera + projection** | **13,209** | 13,035 | 19.2 | 814.2 | 181 | 2,764 | 7 |
| **particle / quad math** | **9,015** | 6,190 | 13.6 | 719.2 | 242 | 11,244 | 7 |
| fighter matrix prep | 3,445 | 2,994 | 18.9 | 255.8 | 125 | 13,158 | 9 |
| renderer adapter math | 2,431 | 2,511 | 64.1 | 289.2 | 94 | 2,848 | 2 |
| material / colour | 1,301 | 1,273 | 4.2 | 87.9 | 125 | 2,792 | 2 |
| HUD / interface display | 1,084 | 1,171 | 5.0 | 89.8 | 71 | 1,336 | 3 |
| stage / wallpaper matrix | 153 | 142 | 3.1 | 16.0 | 34 | 9,072 | 5 |
| **TOTAL** | **30,638** | **27,316** | 128.1 | **2,271.9** | 872 | 43,214 | 35 |

**Concentration 1.12x — this lane is FLAT.** The gate is a *level*, so a flat cut moves
rank-80 by its full value (`a-flat-lane-is-the-best-converting-lane`). Nothing here has to
be aimed at the tail.

**Every one of the 35 callers is in `main/hot`. None is in ITCM.** The helpers they call
all are. That asymmetry is section 4.3.

Top rows, with the shape that decides convertibility — **ops per invocation**:

| caller | phase | tk/fr | entr/fr | calls/fr | ops/entry | bytes |
|---|---|---:|---:|---:|---:|---:|
| `ndsRendererSubmitParticleQuad` | draw-only | 5,358 | 3.7 | 441.3 | 119 | 4,232 |
| `syMatrixLookAtReflectF` | draw+dispatch | 4,999 | 4.0 | 321.4 | 80 | 988 |
| `syMatrixPerspFastF` | draw+dispatch | 2,710 | 6.1 | 112.6 | 18 | 304 |
| `syMatrixLookAtF` | draw-only | 2,184 | 2.0 | 126.0 | 63 | 712 |
| `guMtxCatF` | draw+dispatch | 2,061 | 1.0 | 128.0 | **128** | 140 |
| `lbParticleDrawTextures` | draw-only | 1,967 | 4.0 | 134.6 | 34 | 3,948 |
| `syMatrixF2L` | draw+dispatch | 1,933 | 6.1 | 196.4 | 32 | 408 |
| `ndsParticleSetCurrentCamera` | draw-only | 1,527 | 4.2 | 131.5 | 31 | 1,696 |
| `…BuildNativeMaterialSnapshot` | draw+dispatch | 1,287 | 4.1 | 87.2 | 21 | 2,388 |
| `ifCommonPlayerDamageProcDisplay` | draw-only | 1,064 | 2.0 | 87.3 | 44 | 716 |

`guMtxCatF` is exactly 64 `fmul` + 64 `fadd` per entry — a 4×4 concat, reproduced to the
unit. That it reproduces is the check that the call-site counts are real.

### 2.1 Op mix over the draw-side set

Per-call costs are **measured** (the helper's own marginal cycles ÷ its own marginal
calls), not read off a disassembly. `__aeabi_fsub` is a 4-byte thunk that falls into
`__aeabi_fadd` (`nm`: `01fff30c` size `4`, `01fff310` next), so its calls fold into
fadd's divisor — without that fold fadd reads 28.87 tk/call instead of 18.98 and every
downstream figure is 1.5x wrong.

| helper | calls/fr | tk/fr | **tk/call** |
|---|---:|---:|---:|
| `__aeabi_fmul` | 959.4 | 12,691 | **13.23** |
| `__aeabi_fadd` | 356.9 | 6,773 | **18.98** |
| `__aeabi_f2iz` | 273.3 | 1,765 | 6.46 |
| `__aeabi_fcmplt` | 194.3 | 1,418 | 7.30 |
| `__aeabi_fcmpeq` | 169.6 | 897 | 5.29 |
| `__aeabi_fsub` | 90.5 | 1,718 | 18.98 |
| `__aeabi_fcmpgt` | 79.9 | 590 | 7.38 |
| `__aeabi_fdiv` | 73.7 | 4,360 | **59.19** |
| `__aeabi_i2f` | 40.8 | 333 | 8.16 |
| `__aeabi_ui2f` | 28.9 | 58 | 2.00 |
| `__aeabi_fcmple` / `fcmpge` / `l2f` | 4.7 | 36 | 7.2 / 7.1 / 14.2 |

---

## 3. `sqrtf` — a separate 3,540 tk/fr of draw-side transcendental

`sqrtf` is not an AEABI soft-float helper and is absent from every prior soft-float
ranking. Measured here on the same mask: **80.26 entries/frame, 144.62 tk/call, 11,608
tk/fr, 220 B, ITCM-resident** (`libm_a-ef_sqrt.o` is a Task 37 ITCM member).

| phase | tk/fr | share |
|---|---:|---:|
| sim-only (`func_ovl2_800EDE5C` 4,924, `ftComputer*` 887, …) | 6,004 | 51.7% |
| shared (`syVectorMag3D`, `syVectorNorm3D`) | 2,061 | 17.8% |
| **draw-only** (`lbParticleDrawTextures` 911, `syMatrixLookAtF` 868) | **1,779** | 15.3% |
| **draw+dispatch** (`syMatrixLookAtReflectF` 1,735, …) | **1,761** | 15.2% |
| sim+dispatch | 4 | 0.0% |

**Draw-side `sqrtf` = 3,540 tk/fr over 24.3 calls/frame.** The DS hardware square-root
unit answers a 32-bit root in ~13 cycles; with register setup call it ~10–13 tk against
144.62, i.e. **~3,200 tk/fr removable**, and the unit is unused in this tree (section 6).

**Draw-side float + sqrt total: 30,638 + 3,540 = 34,178 tk/fr = 0.420x of +81,297.**

---

## 4. The deciding measurement: the same operation, both forms, one instrument

The collision lane closed on a measured exchange rate of ~1.00 three separate times
(`SPLIT.md` 0.987, `FOOTPRINT.md` 1.001/1.014, `EXCHANGE.md` 2.68). **Before proposing
anything, that rate has to be either inherited or falsified — and it can be falsified for
free, because this binary already contains both forms of one operation.**

| | float | 20.12 fixed |
|---|---|---|
| symbol | `guMtxCatF` | `ndsRendererMtxMul20p12` |
| operation | 4×4 matrix concat | 4×4 matrix concat |
| bytes | 140 | 312 |
| entries/frame (marginal-80) | 1.00 | 18.55 |
| own self cost | 860 tk/fr | 10,544 tk/fr |
| soft-float called | 2,061 tk/fr (64 fmul + 64 fadd) | none |
| **total per call** | **2,921 tk** | **568.40 tk** |

### **RATIO 5.14x. The collision lane's 1.00 does not transfer.**

Same mask, same build, both live, neither estimated. The float form costs **45.6 tk per
multiply-accumulate**; the fixed form costs **8.88**.

### 4.1 And the multiply itself is 1.00 tick

Every long-multiply instruction in the image was located by disassembly and priced from
its own PC's cycles:

```text
ALL long multiplies, marginal-80    10,073.4 exec/frame   2.09 cycles each = 1.05 TICKS
  ndsRendererMtxMul20p12   1,187.2 smlal/fr   2.00 cyc      (the 20.12 concat)
  __aeabi_fmul             2,364.4 umull/fr   2.00 cyc      (inside the float helper)
  __aeabi_lmul               539.2 muls/fr    4.49 cyc      (the Thumb 64-bit trap)
```

**`__aeabi_fmul`'s own `umull` costs exactly what `ndsRendererMtxMul20p12`'s `smlal`
costs.** All 26.5 cycles of difference are unpack, align, normalise and round. That is
the arithmetic reason the ratio is 5.14 and not 1.

**Corollary, and it is `mean-self-time-is-not-a-budget` in reverse: only 11.3% of
`ndsRendererMtxMul20p12`'s cost is its multiplies** (1,187 of 10,544 tk/fr). A fixed
replacement is dominated by loads, stores and control flow — so a design that counts
SMULLs and calls the rest free will over-claim. The 568.40 tk/call above is the whole
function and is what section 5 prices against.

### 4.2 The Thumb trap is live and measured

`__aeabi_lmul` runs **539.2 `muls`/frame at 4.49 cycles** — 2.2x the cost of a hardware
`umull`, because `-mthumb` has no 64-bit multiply. Any Q-format multiply written without
`__attribute__((target("arm")))` becomes this call. `ndsR2AnimValueQ` already carries that
attribute for exactly this reason and getting it wrong once cost +25,472 P50
(`thumb-hides-64bit-cost`). **Every multiply in the proposed package must be ARM-state or
ITCM-leaf; none may be plain Thumb `(s64)a*b`.**

### 4.3 Why the fetch story is not the collision ring's

**The float library is ITCM-resident.** `nm` puts all fourteen helpers in `0x01ff8000`–
`0x01fff4b8`, and Boundary prints the manifest: `Task 9 float ITCM passed … itcm=29792/32768
free=2976`, members `_arm_addsubsf3.o _arm_muldivsf3.o _arm_cmpsf2.o _arm_unordsf2.o
_arm_fixsfsi.o _arm_fixunssfsi.o`, plus `libm_a-ef_sqrt.o` from Task 37. Zero-wait fetch,
never evicted. **That is why a float call is only ~0.8 fetch-tick and why the collision
ring's `.main` replacement at 0.97 entries/frame lost `issue −1,717` to `icache_fill
+1,854`.**

The draw package does not have to repeat that, for two reasons that are properties of the
code rather than hopes:

1. **Most replacements are inline and strictly smaller than what they delete.** A Q12 add,
   subtract, compare, truncate and int-convert are one ARM/Thumb instruction each; each
   replaces a 4-byte `bl`. That is **1,242.4 calls/frame — 55% of the lane — converting to
   code that shrinks the caller.**
2. **The multiply can stay a call.** A Q12 multiply leaf in ARM state is ~32 B and **ITCM
   has 2,976 B free**, so `bl __aeabi_fmul` becomes `bl ndsR2Q12Mul` with an identical
   fetch profile — the same instruction, the same 4 bytes, a resident callee.

**What is NOT claimed:** that the added bytes are free. `SPLIT.md` measured 0.754 tk/fr
per byte for one body and explicitly forbids carrying that constant, so it is not carried.
The bound used instead is the machine's own line-fill cost — `icache_fill` at 23–51 cycles
per 32 B line, i.e. **≤25.5 tk per added line per frame in the worst compulsory case** —
and the design above is chosen so the added-byte count is near zero rather than argued
down.

---

## 5. The conservative removable ceiling — three routes, and the lowest one is used

**Route A — measured same-operation ratio.** Charge the replacement at 1/5.14 of the float
bill it deletes, which is the measured `guMtxCatF` → `ndsRendererMtxMul20p12` exchange and
is **fetch-inclusive** (both arms are real functions in `.main`):

```text
30,638 − 30,638/5.14 = 24,677 tk/fr
```

**Route B — per-op accounting, replacement priced as inline integer work.** fmul→SMULL
path 3.0 tk, fadd/fsub→ADD 0.5, f2iz→ASR 0.5, compares→CMP 0.5, i2f/ui2f→LSL 0.5,
fdiv→hardware divider 18.0:

```text
gross 30,638 − replacement 4,823 = 25,814 tk/fr
```

**Route C — the pessimistic ITCM-leaf variant**, where every multiply keeps its call
overhead (`bl` + SMULL + shifts + return ≈ 6.75 tk instead of 3.0) and the divider costs
25 tk:

```text
fmul   959.4 x (13.23 − 6.75)  =  6,216
fadd   356.9 x (18.98 − 0.50)  =  6,595
fsub    90.5 x (18.98 − 0.50)  =  1,672
f2iz   273.3 x ( 6.46 − 0.50)  =  1,629
cmps   448.4 x (~5.8  − 0.50)  =  2,376
fdiv    73.7 x (59.19 − 25.0)  =  2,520
i2f     40.8 x ( 8.16 − 0.50)  =    313
ui2f    28.9 x ( 2.00 − 0.50)  =     43
                                 -------
                                  21,364 tk/fr
```

| route | removable tk/fr | x +81,297 |
|---|---:|---:|
| A — measured 5.14x exchange | 24,677 | 0.304x |
| B — per-op inline accounting | 25,814 | 0.318x |
| **C — pessimistic ITCM-leaf** | **21,364** | **0.263x** |
| + draw-side `sqrtf` via the hardware unit | +3,200 | +0.039x |

# **CONSERVATIVE CEILING = 21,364 + 3,200 = 24,564 tk/fr = 0.302x of +81,297.**

# **That is 1.54x the brief's 16,000 floor. THE GATE IS PASSED; this is not a STOP.**

The three routes agree within 21%, and the lowest is the one quoted. **It is a ceiling,
not a prediction** — it assumes every draw-side float op is convertible with no boundary
conversion left behind, which section 7.2 lists as the first thing an implementation must
disprove for itself.

---

## 6. What is refused, before anyone proposes it

- **`__udivmoddi4` is already executing 11.70 times/frame at 248.65 tk/call = 2,909 tk/fr**
  on this mask (whole match 1,999). It is *already* in the build. **No design here may add
  to it**, and none does: the 73.7 fdiv/frame go to the hardware divider or to a
  reciprocal constant, never to a library 64-bit divide. The collision ring's `+17,377 at
  rank-80` from exactly this call is the standing reason.
- **No generic 64-bit divide or sqrt loop, and no large cold kernel.** Route C's whole
  replacement surface is one ITCM leaf plus inline single instructions.
- **The DS hardware divide and square-root units are unused in this tree.** A search over
  `src/` and `include/` for `REG_DIVCNT` / `DIV_NUMER` / `DIV_DENOM` / `REG_SQRTCNT` /
  `divf32` / `sqrtf32` returns **nothing but the two undefined hooks**
  `NDS_R2_CFX_DIV64` and `NDS_R2_CFX_ISQRT64` (`include/nds/nds_r2_collision_fixed.h:205`
  and `:215`), which fall through to portable software. So the units are available and
  uncontended — but they are **single shared registers**, so the first user owns defining
  the seam (no interrupt-time use, no re-entrancy).
- **Nothing in `shared`, `sim-only` or `sim+dispatch` is touched** — 134,720 tk/fr, 80.2%
  of the soft-float bill, including the whole `gmCollision*` cluster. Simulation state,
  collision, RNG, animation and status timing stay float.
- **`decomp/` is not edited.** Five of the 35 callers are decomp bodies (`guMtxCatF`,
  `syMatrixLookAtF`, `syMatrixLookAtReflectF`, `syMatrixPerspFastF`, `syMatrixF2L`); they
  are reached port-side through the existing `#define`-before-`#include` overlay
  mechanism, which moves definition and callers together.

---

## 7. What this cycle did NOT do, and what the next one inherits

### 7.1 Not done, plainly

- **No implementation, no build, no A/B, no pixel pair.** The package that clears 16K is
  the *whole* draw-side conversion; no sub-package clears it alone (camera+projection is
  13,209 gross, the largest single category, and 15,640 with the renderer adapter math —
  both under the floor). A partial conversion is also the shape that has already failed
  twice here: the dense-animation lane converted 23.25% and read −74 tk/fr because the
  generic path's bytes stayed hot.
- **No engagement counters written.** The brief requires proof that every targeted float
  call is removed and that no simulation caller routes through the fixed path. The counter
  set is designed in 7.3 and not landed.
- **Owner visual acceptance is not sought and cannot be given by an agent.** Precision
  changes; the frame-locked pixel pair over fighter movement, camera motion, shield,
  particles and Dream Land is a prerequisite, not a formality.

### 7.2 The three things an implementation must disprove for itself

1. **Boundary conversions.** The ceiling assumes the camera chain runs in Q20.12 from the
   first read of the camera's f32 state, so ~20 conversions/frame replace 814 float ops.
   If instead each function converts at its own entry and exit, the boundary cost is the
   `StoreF32`/`LoadF32` failure the collision ring paid (+588 tk/fr for 964 B) multiplied
   by 35 functions. **Design the chain, not the function.**
2. **Range.** 20.12 gives ±524,288 with 1/4096 resolution. Dream Land world coordinates and
   the perspective `w` divide are the two places to check against measured extremes before
   writing code; `ndsRendererAdapterF2LFixedWExact` already encodes the renderer's answer.
3. **That the 5.14x survives at the caller.** The measurement is one function pair. It is
   strong (same op, same mask, same build) and it is *one* pair.

### 7.3 The cheapest first step, named

**One build, one same-binary route, one gate run — the camera+projection chain only.**

- Convert `syMatrixLookAtF` / `syMatrixLookAtReflectF` / `syMatrixPerspFastF` / `guMtxCatF`
  to a Q20.12 chain feeding `syMatrixF2L`'s consumer directly (`syMatrixF2L` **disappears**
  when its input is already fixed — that is 1,933 tk/fr of the 13,209 deleted outright, not
  converted).
- Gross 13,209 + the 2,603 of draw-side `sqrtf` that lives inside `syMatrixLookAtReflectF`
  (1,735) and `syMatrixLookAtF` (868) = **15,812 tk/fr gross**, 2,764 B of caller code,
  **181 call sites, 19.2 entries/frame** — the smallest coherent unit and the one with the
  best ops-per-entry in the lane.
- Below the 16K floor **as a standalone lever**, and that is the point: it is the
  **falsifier**, not the package. It costs one build and one run, and it converts the
  5.14x from one function pair into a lane rate. If it lands ≥0.8 of its route-C share the
  remaining categories follow; if it reads ≈0 the whole lane closes for the same reason the
  collision lane did, for the price of one build instead of thirty-five conversions.
- **The route word must carry an explicit `.data` section attribute.** A zero-initialised
  route word lands in `.bss` and shifts every later `.data` object, which is how the last
  same-binary pair acquired a ~10,000 tk/fr placement floor.
- Counters: `gNdsR2DrawFixedMulCalls` / `…AddCalls` / `…DivCalls` / `…SqrtCalls` and
  `gNdsR2DrawFixedSimCallerHits` (must read **hard zero** — that is the negative control
  proving no simulation caller routes through the fixed path). Publish from
  `diagnostics.c` with `__attribute__((used))` and `nm`-verify against `--gc-sections`;
  a dropped diagnostic global has turned Boundary red here before.

---

## 8. Tooling — three defects found, two fixed structurally

1. **`analyze-leaf-helper-attribution.py` silently returns a 3-caller table if the
   disassembly was made with `--no-show-raw-insn`**, because its instruction regex requires
   the raw-byte column. It produced `__aeabi_fadd` at **86,183 cycles/call** and nobody
   would have read that as a parse failure. `classify-softfloat-caller-phase.py` now
   **refuses** a disassembly yielding under 1,000 function labels, with the cause named in
   the message.
2. **Mixing a marginal count with a whole-match rate is one flag away**, and the tool only
   *warns*. The correct form is `census-marginal-frame-owners.py --report --census-out`,
   which emits a marginal-mask census; used here, it moved `__aeabi_fadd` from 514.4 to
   38.0 cycles/call — **13.5x**. Recorded here rather than re-derived next time.
3. **`--thunk fsub=fadd` is not the same as the default `__aeabi_fsub=__aeabi_fadd`.** The
   short form matches no symbol and silently disables the fold, inflating fadd by 1.52x.
   Pass no `--thunk` for soft float; the default is correct.

**New:** `scripts/classify-softfloat-caller-phase.py` — reverse call graph from the linked
ELF, pruned to executed code, roots labelled by callback **role** before module prefix.

---

## 9. Verification state

- **No build, no emulator run, no flag flipped, no production source edited** in this task.
- Boundary for the cycle is green at the bore-0 shipping default with 0 `Exception:`
  (`boundary-bore0.trimmed.log`, run for Task A).
- Root ROMs unchanged and not rebuilt by this task: `smash64ds.nds` `54c07fac…`,
  `smash64ds-battle-playable-hwtri.nds` `6c939434…`.
- `decomp/` untouched; `DECOMP_PRISTINE=PASS`.
