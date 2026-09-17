# SRC has no big rocks: 1,108 functions under 5,000 ticks carry more than the gap

Owner opened SRC on 2026-09-17 ("you can work on SRC, but 30 Hz simulation is
not desired"), reversing the 09-16 NO-GO. This is the first thing that needed
doing, and it is measurement, not implementation.

## Two things were already settled and are reused, not redone

1. **SRC.md's own top candidate is already measured NO-GO.**
   `…/2026-09-16_p2-2p8-src-candidate-sizing/` sized the "bound fixed
   pose/transform domain" at floor −4,000 / mid −25,000 / **ceiling −70,000**,
   refuted three of its premises against the per-PC profile (fixed-to-float
   publication is 2,814 tk/fr and the pose engine issues **zero** float ops;
   render-side conversions are already Q20.12; "separate transform preparation"
   is the collision family, already built and measured at **+64**), and SRC.md
   concedes at its line 112 that *"SRC alone is not established as sufficient to
   close the entire deficit."* Nothing here reopens that.

2. **"Unreached residual (GObj indirect dispatch) 234,325 tk/fr" is a label on a
   residual, not a measured mechanism.** `gcRunGObjProcess` measures
   **1,828,473 cycles = ~7,087 tk/fr**, 0.38% of the profile. The 234,325 is
   what is left after subtracting named candidates, and it is a *tail*.

## The whole distribution

Per-PC profile `…/2026-09-16_p2-2p8-n0409-profile/arm9-profile.csv` (6,048,201
PCs, 129 regions, ticks = cycles/258) joined to the profiled ELF's symbol
ranges. `armWaitForIrq` (idle spin) excluded.

**Work = 1,614,414 tk/fr across 1,192 symbols.**

| coverage | reached at symbol # |
|---|---|
| 25% | 15 |
| 50% | 48 |
| 75% | 127 |
| 90% | 274 |
| 95% | 397 |

| band | symbols | carries |
|---|---:|---:|
| > 20,000 tk/fr | **13** | — |
| > 10,000 tk/fr | 37 | — |
| > 5,000 tk/fr | 84 | — |
| **≤ 5,000 tk/fr** | **1,108** | **571,666 tk/fr (35% of work)** |

The largest single symbol in the frame is `__aeabi_fadd` at **50,375 tk/fr =
3.1%**. The top twenty carry **31.2%**.

**The sub-5,000 tail alone (571,666) exceeds the entire 480,960 gap.** That is
the shape of this problem stated numerically: there is nothing left to pick off.
It is also why every leaf lever this campaign has tried came back at noise — the
distribution has no rocks, only gravel.

## The one coherent class that does span the tail

Soft-float, summed across every helper:

| symbol | tk/fr |
|---|---:|
| `__aeabi_fadd` | 50,375 |
| `__mulsf3` (= `__aeabi_fmul`) | 39,794 |
| `__divsf3` | 17,699 |
| comparisons, `__fixsfsi`, `__floatdisf`, rest | 17,501 |
| **total** | **125,369 = 7.8% of work = 26.1% of the gap** |

This is the largest coherent class anywhere in the frame — bigger than any
single candidate in FTR.md, STG.md or MISC.md.

## And it is gated by policy, not by engineering

`…/2026-09-16_p2-2p8-n0409-profile/softfloat-callers.txt` attributes the
`fadd + fmul` class (90,169 tk/fr, and that figure reproduces exactly against
this profile) by caller:

| gate | share | ≈ tk/fr |
|---|---:|---:|
| **GAMEPLAY (state-hash frozen)** | **50.9%** | ~45,900 |
| UNRESOLVED | 28.4% | ~25,600 |
| RENDERER (fidelity-gated) | 20.4% | ~18,400 |
| second-order (float calling float) | 0.3% | ~270 |

Half of it is `lbCommonSin` / `lbCommonCos` and friends under a **frozen state
hash** — converting them changes game behaviour. A fifth is renderer work under
the fidelity doctrine. Only the **28.4% unresolved slice (~25,600 tk/fr, 5.3% of
the gap)** is not already behind a stated policy gate, and that is the only part
of the largest class in the frame that is available to ordinary engineering.

## What this means for the SRC authorization

Stated plainly, because it is the answer to the question that was asked:

1. **SRC cannot be closed by picking functions.** Its residual is a 1,100-function
   tail, its top candidate is measured NO-GO, and its named "unreached" block is
   a label rather than a mechanism.
2. **The only class-sized lever left in the whole frame is soft-float**, and
   **71.3% of it is behind gameplay-behaviour or render-fidelity gates** that are
   owner policy, not engineering difficulty.
3. Therefore **closing 480,960 ticks is a policy decision before it is an
   optimization problem.** The 30 Hz simulation lever the owner has just
   declined was one such policy lever; declining it is legitimate and it removes
   the largest remaining one.

## What is worth doing next, in order

- **Resolve the 28.4% unresolved soft-float slice** (~25,600 tk/fr). It is the
  only unclaimed part of the biggest class, and attribution is cheap — the
  instrument already exists and `ndsMPLineExtentSweepRejects` is already visible
  in its top callers.
- Keep the two halves the 09-16 sizing salvaged, both reachable without a
  pose/transform rewrite: the validity/hierarchy walk (**−10,000 to −15,000**,
  and its recorded falsifier is wrong in the favourable direction — the walk is
  59% and the clear 14%, so it deletes pointer-chase dcache stalls rather than
  issue slots) and the FTR-side copy/convert (**≤ −30,000**).

## What this artifact does not claim

No tick saving is asserted for any change. The per-gate soft-float shares are
sample shares from a 29,846-sample window at frame 439 applied to a measured
class total; they locate the work, they do not price a fix. The instrument's own
cost is correctly outside the series being explained — `analyze-tick-hud-excursion.ps1:46`
records that HUD is excluded from WORK-H by construction, and the measured HUD
lane (20,160 tk/fr) matches `ndsPlatformRenderDebugHud` (20,077) to 0.4%.
