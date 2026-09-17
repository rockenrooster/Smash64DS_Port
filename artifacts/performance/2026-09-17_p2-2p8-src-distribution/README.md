# CORRECTED: this is the WHOLE non-idle frame, not SRC

> **Owner review, `docs/optimization/SRC.md`, 2026-09-17:** *"The later 'SRC
> distribution' is actually a whole-frame non-idle profile. Its approximately
> 1.614M ticks cannot be treated as an exclusive SRC decomposition. Some policy
> percentages in that report also reuse the sampled caller census that another
> report explicitly corrected."*
>
> **Both points are right and they are mine to own.** The method below says so
> in its own words — a per-PC profile with only `armWaitForIrq` excluded. Take
> out the idle spin and what remains is every lane at once: FTR, STG, MISC and
> OTHR are all inside that 1,614,414, so calling it SRC overstates SRC by
> whatever the other lanes carry. The soft-float gate split also reuses the
> 29,846-sample caller census that a separate report had already corrected.
>
> **What survives.** "No big rocks" holds — it is a true statement about the
> frame, which is the harder claim anyway, and the 1,108-symbol tail really
> does exceed the gap. **What does not survive** is the inference drawn from
> it: *"SRC cannot be closed by picking functions"* was never measured, because
> SRC was never isolated here. Nor should the per-gate soft-float percentages
> be quoted until they are recomputed against the corrected census.
>
> The original title was "SRC has no big rocks: 1,108 functions under 5,000
> ticks carry more than the gap." Everything below is unchanged so the error is
> legible rather than tidied away.

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
the fidelity doctrine. Only the **28.4% unresolved slice (~25,600 tk/fr)** was not
*labelled* with a policy gate. **Resolving it (below) shows 91.2% of it is the
gameplay collision narrow phase**, so the genuinely ungated remainder is
approximately zero.

## What this means for the SRC authorization

Stated plainly, because it is the answer to the question that was asked:

1. **SRC cannot be closed by picking functions.** Its residual is a 1,100-function
   tail, its top candidate is measured NO-GO, and its named "unreached" block is
   a label rather than a mechanism.
2. **The only class-sized lever left in the whole frame is soft-float**, and
   after resolving the unresolved slice, **~99.7% of it is behind
   gameplay-behaviour (76.8%) or render-fidelity (22.9%) gates** that are owner
   policy, not engineering difficulty.
3. Therefore **closing 480,960 ticks is a policy decision before it is an
   optimization problem.** The 30 Hz simulation lever the owner has just
   declined was one such policy lever; declining it is legitimate and it removes
   the largest remaining one.

## RESOLVED: the 28.4% "unresolved" slice, and it makes this worse

Done, and it corrects the split above **against** us. The unresolved bucket is
not diffuse and not ungated — it is 29 callers, and **91.2% of it is gameplay
and physics**:

| tk/fr | share of slice | caller |
|---:|---:|---|
| **16,940** | **66.1%** | `func_ovl2_800ED490` |
| 2,492 | 9.7% | `func_ovl2_800EDE5C` |
| 1,045 | 4.1% | `ndsMPLineExtentSweepRejects` |
| 798 ×3 | 9.4% | `ndsBaseMPProcessUpdateMain`, L/R wall-collision adj |
| rest | 10.7% | floor-edge collision, camera, HUD, wallpaper persp |

They were "unresolved" only because `func_ovl2_*` carries no source name the
classifier recognised. **They are the fighter collision narrow phase.**
`func_ovl2_800ED490(Mtx44f dst, Mtx44f lhs, Mtx44f rhs)` is a float 4×4 compose
reached through `gmCollisionCheckFighterAttackDamageCollide`, and the 7.68x
recorded beside it in `battleship_gmcollision.c:156` is **not a speedup** — it
is its spike ratio on the 80 frames that *set P95*.

**Converting it is a gameplay change, and that has already been tried.**
`NDS_R2_SIM_MAC_SHADOW` exists as a lab-only shadow instrument precisely because
the replacement route could not be priced safely: *"a route A/B cannot price a
gameplay change; one on this exact code ended with damage 130/51 against
33/65"* (`battleship_gmcollision.c:199-203`).

### Revised gate split — the correction

| gate | was reported | **actually** |
|---|---:|---:|
| GAMEPLAY (state-hash frozen) | 50.9% / ~45,900 | **76.8% / 69,226 tk/fr** |
| RENDERER + HUD + camera | 20.4% | ~22.9% |
| genuinely ungated | ~28.4% | **~0%** |

**The "only part available to ordinary engineering" was an artefact of an
unlabelled symbol.** Three quarters of the largest class in the frame is behind
gameplay behaviour, and the remainder is behind render fidelity. This does not
weaken the conclusion below; it removes the exception to it.

## What is worth doing next, in order

**CORRECTION — the validity/hierarchy walk is NOT available, and I cited it
three times as if it were.** The 09-16 SRC candidate sizing recommended it at
**−10,000 to −15,000**, and I repeated that in this artifact, in two commit
messages and to the owner. A *second* 09-16 artifact,
`…/2026-09-16_p2-2p8-n0503-flat-cache/`, had already **measured** it across four
arms on one instrument with every divergence witness identical:

| arm | SRC P50 | STG P50 | **WORK-H P50** |
|---|---:|---:|---:|
| 4 slots (shipped) | 558,656 | 344,960 | 1,595,328 |
| 16 slots | **−15,040** | **+51,520** | **+33,984** |
| 32 slots | −15,104 | +55,360 | +40,896 |

The −15,040 is real and lands exactly inside the predicted band. It is then
**more than cancelled**: the table must reach ≥16 slots to hold its working set,
16 slots is 3,264 bytes, and that is 80% of the 4 KB ARM9 dcache, so STG — which
has nothing to do with this cache — pays +51,520. Verdict there: *"the fix
works, and it cannot be paid for."* The sizing artifact's recommendation was
superseded by the measurement on the same day, and I propagated the stale half.

**What is actually left, then:**

- The FTR-side copy/convert (**≤ −30,000**), which n0503 does not touch.
- Shrinking this table rather than growing it. N05.03 reported `Overflows = 0`
  at `MAX = 48` in two arms, so the shipped `MAX = 96` is headroom nothing
  reaches: 1,584 → 816 bytes, 39% → 20% of the dcache, no behaviour change and
  no slot-count change. It moves the dial n0503 proved this lane is sensitive
  to, in the direction n0503 could not. Being measured now.

## What this artifact does not claim

No tick saving is asserted for any change. The per-gate soft-float shares are
sample shares from a 29,846-sample window at frame 439 applied to a measured
class total; they locate the work, they do not price a fix. The instrument's own
cost is correctly outside the series being explained — `analyze-tick-hud-excursion.ps1:46`
records that HUD is excluded from WORK-H by construction, and the measured HUD
lane (20,160 tk/fr) matches `ndsPlatformRenderDebugHud` (20,077) to 0.4%.
