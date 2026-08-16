# The DS divide and root units are safely usable, they are 2.8x-8.3x cheaper than the software forms, and they cannot serve `__aeabi_fdiv` at any price. Column N is 0.050x, not 0.315x.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `dd80585d6eb`**
4 lab builds (`build-c210-hwmath` … `build-c213-hwmath4`), 5 emulator runs, 0 gameplay
values changed, no default flipped, no ROM published, both root ROMs byte-unchanged.
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states
its window.

```text
REQUIREMENT  +94,481 net ticks per presented frame at rank-80.  Basis:
             build-c206-shipgx0, rank-80 1,239,808 raw / 1,214,861 net against
             the 1,120,380 gate; SHIPPING renderer (GX_COMPOSE 0), bore 0, mode
             163 one-minute match, 1,600 samples, frames 439-2038, slips=0
             (../2026-08-16_gap-position/POSITION.md section 1).

SAFE?        YES, AND THE SURVEY IS FROM THE LINKED ELF, NOT FROM GREP.  In
             build-c206-shipgx0 exactly EIGHT functions reach 0x04000280-
             0x040002BF and every one is mainline draw- or simulation-phase
             code.  The port registers ONE interrupt handler and its whole body
             is `sVBlankCount++`.  No libnds or calico code in the binary
             touches either unit.  There is no interrupt-context user, so a
             mainline write/poll/read cannot be interleaved.  Section 1.

PRICE        MEASURED IN SITU ON THE ARM9, 4,096 iterations per arm, loop and
             operand generator subtracted:

               64-bit divide  software 292.5 tk -> hardware 106.0 / 65.0   2.8x / 4.5x
               64-bit isqrt   software 294.9 tk -> hardware  55.5 / 35.5   5.3x / 8.3x
               f32 divide     __aeabi_fdiv 71.5 tk -> hardware 137.6 / 97.6
                                                            0.52x / 0.73x  REFUTED
               sqrtf          shipped Thumb 153.0 tk -> ARM state 90.5     1.69x

             The two hardware numbers are the leading-poll form (libnds,
             battleship_gmcamera.c) and SM64DS's form without it.  THE LEADING
             POLL COSTS 41.0 tk ON THE DIVIDE AND 20.0 ON THE ROOT and protects
             nothing.  Section 3.

ITEM A       NDS_R2_CFX_DIV64, NDS_R2_CFX_ISQRT64 -- and a THIRD hook nobody had
             counted, NDS_R2_COLLISION_DIV64 -- are now defined against the
             units, +44 B of text, 0 libgcc 64-bit divides left in the object
             against 6 in the control.  Graded 0 mismatches over 65,536 live-
             shaped operands per class across four builds.  It takes the
             collision ring's exchange rate from 2.68 to 2.19-2.08.  IT IS
             STILL A LOSS AND THE LANE IS STILL 0.161x OF THE REQUIREMENT EVEN
             AT AN EXCHANGE RATE OF ZERO.  Sections 4 and 5.

ITEM B       COLUMN N IS 0.050x, NOT 0.315x, AND BOTH CORRECTIONS ARE LARGE.
             Its sqrtf half is ALREADY SHIPPED -- NDS_R2_FIXED_SQRT defaults to
             1 and reads 1 in SIMSIDE.md's own basis build.  Its fdiv half is
             REFUTED on price by 1.36x-1.92x.  What survives is one line of
             Makefile: nds_r2_sqrtf.o built -marm instead of -mthumb, worth
             4,300-4,750 tk/fr for +24 B.  Section 6.

RETRACTED    A 110-in-65,536 wrong-answer rate in build-c210's f32 divide was
             REAL, DETERMINISTIC, AND MINE: `(int32_t *)` cast onto an `int *`,
             and int32_t is `long` on devkitARM.  GCC took the strict-aliasing
             licence and deleted the sticky term.  Fixed at the seam, confirmed
             by rebuilding the identical arm.  Section 7.
```

---

## 1. The hazard survey, from the linked ELF

`AGENTS.md` requires reading `decomp/sm64-nds` and `decomp/sm64ds-decomp` before a
substantial DS hardware decision. This is one, and the reference answers it directly.

**What SM64DS does.** `decomp/sm64ds-decomp/src/ARMMathSaveState.c` and
`ARMMathLoadState.c` save and restore `DIVCNT & 3`, `DIV_NUMER`, `DIV_DENOM`,
`SQRTCNT & 1` and `SQRT_PARAM`, and they are called from `ARMSaveContext` /
`ARMRestoreContext`. **A shipping DS title treats both units as thread context.** It
also splits the divide: `cstd::fdiv_async` starts it, `cstd::fdiv_result` collects it
later — the unit's latency is long enough to be worth hiding, which is a fact about the
hardware and is confirmed by section 3.

**What this binary does.** Disassembling `builds/build-c206-shipgx0` (the requirement's
own basis) and resolving every base register gives the complete list of users of
`0x04000280`–`0x040002BF`:

| function | registers | unit |
|---|---|---|
| `ndsRendererR2WriteLightVector` | `0x298` `0x2a0` `0x2b4` | divide + root |
| `ndsRendererHardwareClipVertex` | `0x298` `0x2a0` | divide |
| `ndsRendererHardwareSubmitVertex` | `0x298` `0x2a0` | divide |
| `ndsRendererHardwareClipVertexNdcDepth` | `0x298` `0x2a0` | divide |
| `ndsRendererSubmitNativeImpactWave` | `0x2a0` | divide |
| `div64` (libnds, outlined) | `0x298` `0x2a0` | divide |
| `ndsR2CamDiv64` / `ndsR2CamSqrt64` | `0x298` `0x2a0` / `0x2b4` | divide / root |
| `sqrtf` (`src/nds/r2/nds_r2_sqrtf.c`) | `0x2b0` `0x2b4` `0x2b8` | root |

and **nothing else, libnds and calico included**. The port's only registered interrupt
handler is `nds_platform.c:397` → `ndsPlatformVBlankInterrupt`, whose whole body is
`sVBlankCount++`. So:

- **No interrupt-context user exists**, therefore no mainline sequence can be
  interleaved, therefore no protocol is required and the units are safely usable.
- **`sqrtf`'s IME mask** (`nds_r2_sqrtf.c:39-45`) is protecting a reachability that this
  survey says does not exist. It is left alone: removing a safety property is not this
  cycle's business, and it costs two I/O writes.
- **The property is a fact about the link, not about the design.** It is recorded in
  `include/nds/nds_r2_hwmath_unit.h` together with SM64DS's context-save shape, which is
  what the seam has to become if this port ever gains preemption or puts a divide inside
  an ISR.

> **A first pass of this survey said the divide unit was unused.** It scanned literal
> pools only, and seven of the eight users form their address as `mov rX, #0x4000000`
> plus an immediate offset, or from a `0x04000200` base with offsets `0x80`/`0x90`/
> `0x98`/`0xa0`. `[[confident-absence]]`: one modality is not a survey.

---

## 2. Basis for every number below

| | |
|---|---|
| arms | `builds/build-c210-hwmath`, `-c211-hwmath2`, `-c212-hwmath3`, `-c213-hwmath4` |
| target | `smash64ds-battle-playable-tickhud-hwtri` |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_HWMATH_BENCH=1`, GX_COMPOSE 0, bore 0, DLDI on |
| instrument | `src/port/nds_r2_hwmath_bench.c`, run once at boot from `ndsPlatformInit`, built `-marm` |
| timed window | 4,096 iterations per arm after a 64-iteration warm pass; **a baseline arm times the loop, the LCG and the sink with no kernel, and is subtracted from every other arm** |
| graded window | 65,536 operands per class, untimed, against the software form on the identical operand |
| readout | `sample-tick-hud-buckets.ps1 -ExtraGlobals`, `-Samples 8 -StartFrame 60`, `slips=0` on every run |
| raw | `c210-hwmath.json`, `c210-repeat.json`, `c211-hwmath2.json`, `c212-hwmath3.json`, `c213-hwmath4.json` |

**What this price is and is not.** Every arm runs its kernel back to back, so the
instruction cache is warm and the branch history ideal. That is the right basis for
*which of two implementations of the same operation is cheaper* — both arms get the same
favour — and the wrong basis for *what this will cost in situ*, where a kernel entered a
few times a frame pays compulsory fetch on top. **Read every figure as a lower bound on
the in-situ cost and an upper bound on the saving.**

**The instrument checks out against an independent measurement.** `__aeabi_fdiv` reads
**71.5 tk/call** here against `SIMSIDE.md` §3's **59.19 tk/call** from a per-PC profile of
the live match — 1.21x apart on different operand mixes, with this one called from ARM
state. `sqrtf` reads **153.0** against `SIMSIDE.md`'s **144.62**, 1.06x apart.

**Reproducibility across builds is the check that the timed arms mean anything.**
`DivSoftTicks` reads 1,212,736 / 1,212,672 / 1,212,736 / 1,212,672 on the four builds
(0.005% spread); `SqrtLeadTicks` 241,984 / 241,920 / 241,792 / 241,792 (0.08%). The one
arm that moves is `SqrtfArmTicks` — 393,344 / 409,792 / 385,216 / 385,216, a ±3% band —
so it is quoted as a range and its best case is not used alone.

---

## 3. The prices

`build-c213-hwmath4`, baseline arm 14,528 ticks over 4,096 iterations = 3.55 tk/iteration,
subtracted from every row.

| kernel | raw ticks | **tk per operation** | against software |
|---|---:|---:|---:|
| `(int64)a / (int64)b` → `__aeabi_ldivmod` | 1,212,672 | **292.5** | — |
| DS divide, 64/64, leading poll | 448,704 | **106.0** | **2.76x** |
| DS divide, 64/64, SM64DS sequence | 280,768 | **65.0** | **4.50x** |
| `ndsR2CfxIsqrt64Portable` (32-step restoring root) | 1,222,848 | **294.9** | — |
| DS root, 64-bit, leading poll | 241,792 | **55.5** | **5.32x** |
| DS root, 64-bit, SM64DS sequence | 159,808 | **35.5** | **8.31x** |
| `__aeabi_fdiv` | 307,328 | **71.5** | — |
| f32 divide on the unit, leading poll | 577,920 | **137.6** | **0.52x** |
| f32 divide on the unit, SM64DS sequence *(c211/c212)* | 414,080 | **97.6** | **0.73x** |
| `sqrtf` as shipped (Thumb, `bl __aeabi_lmul`) | 641,408 | **153.0** | — |
| the same header in ARM state | 385,216–409,792 | **90.5–96.5** | **1.59x–1.69x** |

### 3.1 The leading poll costs 41.0 ticks and protects nothing

GBATEK: writing `DIVCNT`, `DIV_NUMER` or `DIV_DENOM` restarts the division and raises the
busy flag; the same holds for the root. **Only the last write matters**, so the poll that
*precedes* the parameter writes waits out a result nobody reads. libnds's `div64` and
`sqrt64`, and therefore `nds_renderer.c`'s five call sites, and
`battleship_gmcamera.c`'s `ndsR2CamDiv64`/`ndsR2CamSqrt64`, all carry it. SM64DS's
`cstd::div` and `cstd::sqrt` do not.

```text
divide  106.0 - 65.0 = 41.0 tk per call    (82 ARM9 cycles)
root     55.5 - 35.5 = 20.0 tk per call
```

**Both forms grade bit-identical**: `DivMismatch`, `DivFastMismatch`, `SqrtMismatch`,
`SqrtFastMismatch`, `QuotMismatch`, `QuotLeadMismatch`, `RemMismatch`, `RemLeadMismatch`
are all **0 over 65,536 operands each**, on four builds, with **32,914 negative
denominators** and **223 rounding half-cases** as live controls. The shipped hooks use
the leading-poll form anyway, because it is the in-tree-proven sequence and this cycle did
not set out to change an idiom that five renderer sites depend on. **41.0 tk per divide
across those sites is an unpriced item and it is on the board.**

### 3.2 Why the f32 divide cannot pay, stated as a floor

The hardware unit's own latency is **65.0 tk minimum** for a 64-bit divide before any
IEEE unpacking, normalisation or rounding — and `__aeabi_fdiv`, a hand-written
`ieee754-sf.S` routine that does the whole job, costs **71.5**. There is **6.5 ticks** of
room for a 24-bit unpack of two operands, an exponent computation, a guard/sticky
decision, a round and a repack. The measured kernel needs 32.6.

**This is not a "the algorithm could be tighter" result. It is a floor argument**: no
correctly-rounded f32 divide built on this unit can cost less than the unit, and the unit
alone is 91% of the routine it would replace.

---

## 4. Item A — the hooks, finally defined

`include/nds/nds_r2_collision_fixed.h:205,215` has carried `NDS_R2_CFX_DIV64` and
`NDS_R2_CFX_ISQRT64` undefined since it was written, and `EXCHANGE.md` §0.4 names the
portable divide behind the first as the measured cause of the collision ring's 2.68x.
Both are now bound in `src/port/nds_r2_collision_fixed.c`.

**There is a third hook and it was not in the brief.** `include/nds/nds_r2_collision_mtx.h:361`
carries `NDS_R2_COLLISION_DIV64` with the same never-taken "the port overrides this with
the DS hardware divider" comment. It is the divide inside `ndsR2CollisionInvertMatrix44`,
i.e. inside `ndsR2CollisionFixedInvertF32` — **one of the four f32-boundary entry points
the wired ring actually calls** (`EXCHANGE.md` §3.1 lists `InvertF32` among the ring
rows). Binding only the two named hooks left a live `bl __aeabi_ldivmod` in the object.
It was found by disassembling the result, not by reading the brief.

**Compiled proof, and its control:**

```text
src/port/nds_r2_collision_fixed.o, -marm, -O2, NDS_R2_COLLISION_FIXED=1

  NDS_R2_CFX_HWMATH=0   6 references to __aeabi_ldivmod / __udivmoddi4   7,916 B text
  NDS_R2_CFX_HWMATH=1   0                                                7,960 B text
```

**+44 bytes**, and the hardware form also deletes the 32-iteration restoring-root loop.
The campaign's standing failure mode — added bytes inverting a win — barely applies here.

**Bit-exactness is proved in two halves, not bounded as one** (`[[prove-the-parser-half-exactly]]`):

- **The algorithm half, exhaustively, on the host.** `scripts/check-r2-hwmath.ps1` +
  `check-r2-hwmath.c` compile the shipped kernel with the unit modelled by an exact C
  divide and grade it against the host's IEEE divide over **six complete 2^23 significand
  axes** plus an exponent cross-product plus 8M random pairs: **58,339,748 cases,
  56,337,530 bit-identical, 0 mismatches**, with 19,781,825 round-ups and 16,779,783
  exact quotients as live controls and the decline path asserted on every special class.
- **The unit half, in the ROM.** The bench grades each hardware primitive against its
  software counterpart on the identical operand: **0 mismatches, 65,536 operands per
  class, four builds.** The root's exactness additionally already carries an in-tree
  proof through `sqrtf` (`scripts/check-r2-fixed-sqrt.ps1`).
- **One claim the falsifier corrected.** The header originally asserted that a
  round-to-nearest tie is reachable. It is not: with a 2^32-scaled numerator the quotient
  of two 24-bit significands either does not terminate (so sticky is set) or terminates
  within 24 significant bits (so the guard bit is zero), and a tie needs exactly 25. The
  checker counted zero over 58M and now **asserts** zero, with round-ups and exact
  quotients as its two neighbouring live states.

**Every call site's denominator is proven non-zero before the divide** — the `s^2` guard
at `:530`, `|det| >= 2^21` at `:724`, and the `dist[fixed_axis] == 0` decline at `:961` —
so the hook adds no obligation its callers did not already carry. The 64/64 mode is used
rather than 64/32 because GBATEK gives them the same 34-cycle latency and the wider one
has no denominator range restriction, which is the difference between "the same
arithmetic" and "the same arithmetic on the domain I happened to check".

---

## 5. What that does to the ring's 2.68x: it does not save it

`EXCHANGE.md` §3.1's whole-match rows, repriced at the measured ratio. **This is
arithmetic on that document's own measured `__udivmoddi4` row, not a new A/B, and it is
labelled as such.**

```text
fixed added            +3,392 tk/fr      of which __udivmoddi4  +988
float deleted          -1,264 tk/fr
rate = 3,392 / 1,264 = 2.68              (cost / deleted; 1.00 is break-even)

hardware divide serves the same calls at 106.0/292.5 = 36.2%  (leading poll)
                                      or  65.0/292.5 = 22.2%  (SM64DS form)

  __udivmoddi4 row   988 -> 358   added 2,762   rate 2,762/1,264 = 2.19
  __udivmoddi4 row   988 -> 220   added 2,624   rate 2,624/1,264 = 2.08
```

The root savings are **not** counted: they sit inside `ndsR2CollisionFixedMakeFrame`'s
`+1,256` row, which `EXCHANGE.md` did not decompose, and dividing that residual by a call
count to get a price is the shape recorded as
`[[a-residual-divided-by-a-count-is-not-a-price]]`. They move the rate further toward
1.00 and are stated as unquantified rather than fitted.

**It does not matter, and that is the actual answer to item A.** `EXCHANGE.md` §0.3:
the identifiable float in the whole fighter narrow phase is **15,217 tk/fr at rank-80**.
Against **+94,481** that is **0.161x even at an exchange rate of zero**. The lane was
never large enough. **No end-to-end A/B of the ring was run and none should be**: two
builds and two whole-match runs to confirm that a closed lane is still closed is not a
use of the budget, and the hooks are correct and free whether or not the ring ever ships.

`NDS_R2_CFX_HWMATH` therefore defaults to `$(NDS_R2_COLLISION_FIXED)`, which is `0`
everywhere: at 0 the translation unit that binds the hooks is not linked at all, so no
shipped byte moves. There is no arm to choose between — the arithmetic is proven
identical and the hardware form is not slower — so a default-off "candidate" here would
only be a flag nobody flips.

---

## 6. Item B — Column N, honestly sized

`SIMSIDE.md` §6 offers Column N as **21,886 tk/fr sim + 7,900 draw = 29,786 = 0.315x**,
"unsized, conversion unmeasured", with `REG_DIVCNT` and `REG_SQRTCNT` named as the DS
units that could serve `__aeabi_fdiv` (59.19 tk/call) and `sqrtf` (144.62 tk/call).
**Its conversions-per-deleted-operation is 0 by construction** — a bit-exact helper
replacement crosses no representation boundary — which is exactly why it was the right
column to test after `EXCHANGE_LEAF.md` closed the leaf route. Two of its three
components do not survive contact.

### 6.1 The `sqrtf` half is already shipped

`Makefile:668` reads `NDS_R2_FIXED_SQRT ?= 1`, and the generated config of **every basis
this campaign quotes** carries `#define NDS_R2_FIXED_SQRT 1`:

```text
builds/build-c200-trackprof-off/nds_build_config.h:93   (SIMSIDE.md's own basis)
builds/build-c206-shipgx0/nds_build_config.h:93         (the +94,481 basis)
builds/build-c209-simmac2/nds_build_config.h:93         (EXCHANGE_LEAF.md's basis)
```

So the 144.62 tk/call `SIMSIDE.md` measured **is already the hardware-unit
implementation** (`src/nds/r2/nds_r2_sqrtf.c`, R2-03 E1, bit-exact against newlib). The
8,068 + 3,540 tk/fr is not headroom waiting for a hardware unit; it is what the hardware
unit already costs. **11,608 tk/fr comes off Column N's advertised surface.**

### 6.2 The `fdiv` half is refuted

**18,178 tk/fr (13,818 sim + 4,360 draw) converts at 0.52x–0.73x, i.e. it costs more.**
§3.2 gives the floor argument. **Column N's largest component is worth zero, and
attempting it would add between 6,600 and 16,800 tk/fr.**

### 6.3 What survives is one Makefile line

The shipped `sqrtf` is **Thumb**, and ARMv5TE Thumb has no `UMULL`, so
`nds_r2_sqrtf.h:73`'s 48-bit `root * root` is a library call — visible at `0x0208b10c` in
`build-c206-shipgx0`'s own disassembly as `bl __aeabi_lmul`. `src/nds/r2/nds_r2_sqrtf.c`
is the one R2 kernel object the Makefile does **not** give `-marm`
(`nds_r2_collision_fixed.o` and `nds_r2_sim_mac_fixed.o` both have it).

```text
the identical header, same flags, one mode apart
  -mthumb   192 B   1 bl __aeabi_lmul
  -marm     216 B   0
                    +24 B

measured price   153.0 tk/call -> 90.5-96.5 tk/call     saving 37%-41%
graded           gNdsR2HwMathBenchSqrtfMismatch = 0 over 65,536 inputs
                 (the ARM build of the header against the shipped Thumb sqrtf)
```

Applied to the lane `SIMSIDE.md` and `DRAW_FIXEDPOINT.md` measured — 8,068 sim + 3,540
draw = **11,608 tk/fr** at marginal-80:

```text
COLUMN N, honest        11,608 x 0.37..0.41  =  4,300 - 4,750 tk/fr   0.046x - 0.050x
COLUMN N, as advertised                        29,786 tk/fr           0.315x
```

**It is fidelity-neutral by construction** (bit-identical result, graded), **needs no
owner decision**, spans sim and draw, and costs 24 bytes. It is also **below the
≥14,080 rank-80 cross-build floor**, so proving it at the gate needs a same-binary
`.data` route — one word selecting between a Thumb body and an ARM body of the same
header — not a two-build A/B. That build is the next cycle's, and it is small.

### 6.4 The ladder

```text
POSITION.md fidelity-neutral inventory              53,215   0.563x   -> +41,266 left
+ sqrtf in ARM state (column N, all that survives)   ~4,500   0.048x
                                                    ------
                                                    57,715   0.611x   -> +36,766 left
```

`SIMSIDE.md`'s "+ column N, unsized surface 29,786" line should be read as **4,300–4,750
measured**, and its `REG_DIVCNT`/`REG_SQRTCNT` row as **priced and refused**.

---

## 7. Retraction: a 110-in-65,536 wrong-answer rate that was mine

`build-c210-hwmath` graded its hardware f32 divide against `__aeabi_fdiv` on 65,536 live
operands and reported **110 disagreements**. It reproduced **exactly** on a second run of
the same ROM, so it was deterministic, and none of the other five graded classes
disagreed at all. It is written up rather than quietly fixed because the diagnosis went
through two wrong hypotheses first.

- **Hypothesis 1: the DS 64/32 mode is inexact.** Refuted. `build-c211` graded the 64/32
  quotient and remainder separately against software: **0 and 0** over the same 65,536
  operands, with **223 rounding half-cases** present as a control.
- **Hypothesis 2: the leading poll allows a stale read.** This was the frightening one,
  because the leading-poll idiom is what libnds's `div64`/`sqrt64`, five
  `nds_renderer.c` sites and `battleship_gmcamera.c` all use. Refuted. `build-c212`
  graded **both** sequences on the identical operands in one binary: **0, 0, 0, 0**.
- **Actual cause, found by disassembly.** `int32_t` on devkitARM is `long int`, not `int`
  — `_Static_assert(__builtin_types_compatible_p(int32_t, int))` fails and the `long`
  one passes. The bench's wrapper declared its remainder parameter `int *` and cast it to
  `(int32_t *)`, a cast between incompatible pointer types. GCC took the strict-aliasing
  licence: in c210's schedule it treated the store through the `long *` as unable to
  touch the `int` object, folded `remainder` back to its `0` initialiser, and **deleted
  the `| remainder` term from the sticky expression**. At `0x0208bec8` the register
  holding `DIVREM_RESULT` is overwritten by the significand shift and never read; the
  emitted sticky is `(quotient & 0xff) != 0` alone. Wrong sticky at a half-case flips the
  rounding about half the time: **223 half-cases, 110 wrong.**
- **Confirmed at the seam.** `build-c213-hwmath4` restores the *exact* c210 arm — the
  leading-poll 64/32 primitive driving the f32 kernel — with only the pointer types made
  consistent end to end. **`FdivMismatch = 0`**, 223 half-cases still live.

**The structural fix, so the wrong form is inexpressible:** there is no pointer cast left
anywhere in the chain. `nds_r2_hwmath.h` requires
`int64_t (*)(int64_t, int32_t, int32_t *)`, the ROM wrapper and the host model both
supply exactly that, and any future drift is a hard `-Werror` incompatible-pointer error
rather than silent UB. The reason is written at the declaration.

**Scope of the defect: none shipped.** The f32 kernel is refused on price (§6.2), is
wired to nothing, and lives only in a lab-only translation unit that is not linked at the
default flag. The two hooks that *are* defined never touch the remainder.

---

## 8. Item C — the chain map, assessment only

`EXCHANGE_LEAF.md` closed the leaf float→fixed route and left the **chain** route, where
`n_conv` stays fixed at the endpoints while `n_op` grows with the chain's length. Three
chains exist on the sim side. Scored from `SIMSIDE.md` §4.1's measured entry rates and
op counts; **no build was made and none is proposed for this cycle.**

| chain | members | tk/fr | front endpoint | back endpoint | conv/op | entries/fr |
|---|---|---:|---|---|---:|---|
| **C1 collision cluster** | `gmCollisionTransformMatrixAll` → `func_ovl2_800ED490` → `gmCollisionSetInvertMatrix` → `gmCollisionGetWorldPosition` → `gmCollisionTestRectangle` | 50,044 | joint matrix, 16 f32 | `vec_scale` + a boolean | ~0.02 | 11–45 |
| **C2 animation → joint matrix** | `ndsR2FtAnimParseDObjFigatree` → `ndsBaseGcPlayDObjAnimJoint` → `ndsBaseGcPlayMObjMatAnim` → `gmCollisionTransformMatrixAll` | 20,357 | **s16 track data — 0 conversions** | Q26 joint matrix — **0 if C1 also converts** | **~0** | 22–93 |
| **C3 trig leaves** | `lbCommonSin` / `lbCommonCos` | 4,519 | **u16 Q15 `gSYSinTable` — 0** | 0 inside C2 | **~0** | 84 |

**C1 is built, wired and measured, and it is the precedent that matters here: its conv/op
is already ~0.02 and it still returned 2.68.** So **conv/op near zero is necessary and not
sufficient.** What killed C1 was not the boundary — it was **3,228 B of new executing text
entered 0.97 times a frame**, i.e. compulsory instruction fetch, the same mechanism that
inverted the camera chain's −4,736 into +1,600 when it was inlined. That is the term the
`EXCHANGE_LEAF.md` signature model does not contain.

**C2 is the only chain worth a build, and the one structural reason is entry rate.** Its
members are entered **22–93 times a frame against C1's 0.97**, so the same added bytes
amortise 23x–96x better. Its front endpoint is genuinely free: figatree track values are
`s16` on disk — the repo's own `ftAnimGetTargetValue` replacement is verified exhaustively
over "all 65,536 s16 × 8 track ids" — so a fixed chain does not convert at the front, it
simply **stops** converting. C3 is already implemented inside
`include/nds/nds_r2_collision_fixed.h` (`ndsR2CfxBuildLocal` reads `gSYSinTable` as Q15
integers) and is unwired outside the ring; it is C2's interior, not a separate item.

**The first question a C2 cycle must answer is a footprint question, not an arithmetic
one**: how many bytes of new executing text, against 20,357 tk/fr at 22–93 entries a
frame. Measure the bytes before the arithmetic. `[[route-to-attribute-rebank-to-bank]]`.

---

## 9. What this cycle did NOT do

- **No end-to-end A/B of the collision ring**, and §5 says why: the lane's ceiling is
  0.161x of the requirement at an exchange rate of zero, so two builds and two whole-match
  runs would only re-confirm a closed lane. The 2.19/2.08 reprice is arithmetic on
  `EXCHANGE.md`'s own measured row and is labelled as arithmetic.
- **The root savings inside the ring were not quantified**, only stated to be positive.
- **`sqrtf` was NOT moved to `-marm`.** The measurement is of an ARM build of the same
  header inside the bench object; changing `nds_r2_sqrtf.o`'s flags is a codegen change
  whose gate proof needs a same-binary route, and starting that chain was outside what
  this cycle could finish. Nothing in `src/nds/r2/` was touched.
- **The 41.0 tk leading poll was not removed from anything.** It is measured, it is
  proven equivalent both ways, and five renderer sites plus libnds's own inline depend on
  the current form; changing a shared idiom is its own row.
- **`sqrtf`'s IME mask was not removed**, though §1 shows the reachability it guards
  against does not exist in this binary.
- **No fidelity decision was taken or asked for.** Everything here is bit-exact by
  construction and graded.
- **No pixel capture, no ROM published, both root ROMs byte-unchanged**
  (`smash64ds.nds` `54c07fac…`, `smash64ds-battle-playable-hwtri.nds` `6c939434…`, before
  and after; the second is the bore-84 link and must not be published).

## 10. Reproduction

```powershell
pwsh -File scripts\check-r2-hwmath.ps1            # host, ~30 s, no ROM

make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c213-hwmath4 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_HWMATH_BENCH=1
pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c213-hwmath4 -NoBuild `
    -Samples 8 -StartFrame 60 -TimeoutSeconds 600 -ExtraGlobals <the gNdsR2HwMathBench* set>
```

The hazard survey in §1 needs no build: disassemble any linked ELF, resolve the base
register of every load/store, and list the effective addresses in
`0x04000280`–`0x040002BF`.
