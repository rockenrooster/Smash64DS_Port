# R2-03 E1 — the hardware square root wins, but only by 38%

**Date:** 2026-07-28
**Phase:** R2-03 E1, first cut from `ClaudeOpus5_R203_E0_SoftFloatCallers_20260728.md`.
**Flag:** `NDS_R2_FIXED_SQRT` (default `0`).
**Verdict: KEEP, on the census.** `sqrtf` falls from **15,760 to 9,720
ticks/frame** — **−6,040**, bit-exact. The 8-frame A/B read flat, and that is
the interesting part of this report.

---

## 1. What was built

`sqrtf` replaced with a correctly-rounded implementation on the DS hardware
square-root unit (`0x040002B0`), which `nds_renderer.c:7988` already uses.
Negative, zero, denormal, infinity and NaN are declined to newlib so their bit
patterns stay newlib's own.

The arithmetic lives in `include/nds/nds_r2_sqrtf.h`, not in the `.c`, so that
the falsifier exercises the shipped code rather than a transcription of it.

**Bit-exact by construction, and that was the design goal rather than a bonus.**
`sqrtf` is on the gameplay path — `syVectorMag3D` feeds collision and the
camera — so an answer differing in the last bit is a pose differing, and
mechanical equivalence would become a judgement call instead of a hash compare.
The rounding argument: with `A = M << (r + 23)` and `R = floor(sqrt(A))`, round
up iff `A > R*R + R`. A tie is impossible because `(R + ½)² ` always carries a
¼ fraction no integer `A` can equal, so there is no tie-break rule to get wrong
and round-to-nearest-even falls out for free.

The unit is global mutable state and the sequence is write-param / poll-busy /
read-result, so `REG_IME` is masked around it. newlib's `sqrtf` was pure
software and reentrant; this one is not, and an ISR taking a square root
mid-sequence would silently return a wrong value — the class `AGENTS.md` calls
a failure rather than a risk.

## 2. Correctness and engagement, before any tick

- `scripts/check-r2-fixed-sqrt.ps1` compiles the shipped kernel on the host with
  the hardware unit stood in for by an exact integer root: **8,775,610 handled
  inputs, 0 mismatches** against IEEE `sqrtf`, across every exponent, perfect
  squares and their neighbours, a dense sweep of both exponent parities, and 8M
  random patterns. It also fails if the kernel declines almost everything, so a
  kernel that quietly did nothing could not pass.
- The same checker compiles the kernel for ARMv5TE and **fails if it calls any
  soft-float routine** — the whole point is to stop calling those, and one stray
  float temporary would hand the saving back.
- Engagement is proven statically from the shipped ELF: `sqrtf` is 104
  instructions, inlines the `0x040002Bx` register block and `REG_IME`, calls
  `__aeabi_lmul` for the 64-bit rounding multiply, and reaches
  `__ieee754_sqrtf` only on the declined branch.

## 3. The A/B read flat

128-frame ring dump, one tree, flag off versus on:

| bucket | off P50 | on P50 | Δ | paired down/up | median Δ |
|---|---|---|---|---|---|
| `WORK` | 1,326,080 | 1,328,064 | +1,984 | 52 / 74 | +608 |
| `STG` | 351,488 | 352,384 | +896 | 29 / 98 | +1,152 |
| `FTR` | 543,104 | 543,360 | +256 | 61 / 67 | +352 |
| `ALL` | 1,680,064 | 1,680,064 | 0 | 50 / 61 | 0 |

No bucket shows a consistent sign. Compare E1a, where `STG` fell on **128 of
128** frames. This is placement and nothing else.

## 4. The census says the saving is real

| | control | candidate |
|---|---|---|
| `__ieee754_sqrtf` | 14,258 | — (not on any hot path) |
| `sqrtf` | 1,502 | **9,720** |
| **total** | **15,760** | **9,720** |

**−6,040 ticks/frame**, and the whole-frame soft-float group falls 172,785 →
164,005. The saving is real, and it lands *inside* the 5,000–7,000
build-placement floor — which is exactly why an 8-frame A/B could not see it.
`TASK_STANDING_RULES.md` already says a flat reading below the floor is "not
measurable this way" rather than a refutation; this run supplies the
constructive half, now recorded there: **when the predicted saving is near the
floor, gate on the symbol census, which measures the function directly and
carries no placement term.**

## 5. Why only 38%, when the hardware should be 17× faster

The DS unit computes a 64-bit integer root in 13 cycles. Getting an answer out
of it does not take 13 cycles. libnds's `sqrt64` is write-`SQRTCNT`,
**poll-busy**, write-`PARAM`, **poll-busy**, read-`RESULT` — and every one of
those is an I/O-bus access with wait states. Counted from the disassembly, the
shipped `sqrtf` is 104 instructions with 4 backward branches; newlib's
`__ieee754_sqrtf` is 108 instructions with 6. Structurally they are the same
size, and the polling is what ate the advantage.

**The reusable form: on this hardware a coprocessor is only worth it if the
result can be collected without spinning on it.** The sqrt unit pays off when a
caller can overlap the latency with other work; for a scalar `sqrtf` that
returns immediately, two busy-wait loops cost about as much as the software
root they replaced.

## 6. Follow-up

The first `poll-busy` in libnds's `sqrt64` guards against a previous operation
still being in flight. With IRQs masked and nothing else using the unit, it
cannot be in flight, so that poll is dead. Removing it should be worth roughly
another 1,500–2,000 ticks/frame. Not done here: it means not calling `sqrt64`
and touching the registers directly, which is a larger claim about ownership of
the unit than this cut needs to make.

## 7. Gate

- Falsifier green (§2), ARM codegen integer-only, engagement proven statically.
- Census −6,040 ticks/frame (§4).
- Boundary green on the candidate configuration.
- Flag defaults to 0. Both R2 gains so far (`NDS_R2_STAGE_DIRECT` −94,784 on
  `STG`, `NDS_R2_FIXED_SQRT` −6,040) are accumulated but not yet shipped; R2-08
  switch acceptance is where they turn on.
