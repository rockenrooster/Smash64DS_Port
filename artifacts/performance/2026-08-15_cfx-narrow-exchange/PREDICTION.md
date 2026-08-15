# Slice 53 — written BEFORE the first run, so the arithmetic can be wrong in public

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `1e2833cccba`.
Premise: `../2026-08-15_cfx-ring-draw0/FOOTPRINT.md` §5 — the resident exchange
rate is straddled between 0.052 and 0.467 tk/byte/call and one build settles it.
**UNITS: 2 profile cycles = 1 project tick.**

## 0. The brief's premise is wrong on one factual point, and it is stated here

The brief (and `FOOTPRINT.md` §5's own closing paragraph) asks for
`gmCollisionGetWorldPosition` on the grounds that **it early-exits**. Read
against the source it does not: `gm/gmcollision.c:196-205` is nine `f32`
multiplies and nine adds with no branch at all. The body that early-exits is
`gmCollisionTestRectangle` (`:661`), which is where the 0.052 tk/byte/call was
measured in the first place.

Two further facts settle the target:

1. `gmCollisionGetWorldPosition` has **fifteen in-TU call sites** in
   `gmcollision.c` (`:504 :514 :527 :537 :686 :703 :704 :793 :845 :846 :1029
   :1047 :1048 :1953 :1978 :2075 :2106`), and essentially all of its 19.29
   marginal calls per frame come from them. The `#define`-before-`#include`
   rename that wires the ring **cannot** intercept it — that technique renames
   the definition *and* the call sites together, which is exactly the trap
   `battleship_gmcollision.c:167-169` documents for `gmCollisionTestRectangle`.
2. The one place the fighter narrow phase can be intercepted is already hooked:
   `gmCollisionCheckFighterAttackDamageCollide`, whose entire body is
   `func_ovl2_800EDE00`, `func_ovl2_800EDE5C` and **one** `gmCollisionTestRectangle`
   call (`:1379-1400`). Prepare already replaces the first two.

So slice 53 converts **the consumer tail of the damage phase** —
`gmCollisionTestRectangle` *and* the two `gmCollisionGetWorldPosition` calls it
makes — which is the body the brief actually wanted (`TestRectangle` is the
early-exiting one) plus the body it named. Everything else about the brief holds:
one build pair, one differing byte, exchange rate read off the same `--diff`.

## 1. The instrument

| arm | build | `…_NARROW_DISPATCH` | everything else |
|---|---|---|---|
| **B** | `build-c181-cfxnarrow-b-d0` | **1** | ring dispatch 1, `DRAW=0`, `BOTH_CPU=1`, DLDI on |
| **A** | `build-c182-cfxnarrow-a-d0` | **0** | identical |

`NDS_R2_COLLISION_FIXED_NARROW=1` in **both**, so both link every byte of the new
kernel and differ in one initialised `.data` word (`gNdsCfxNarrowEnable`, read
through a `volatile`). `scripts/compare-elf-sections.py --max-diff 1` asserts it.

## 2. Flip budget — stated before the run, and it is ZERO

The decision moves into port code for the first time, so this is the number that
decides whether any tick figure may be read at all.

```text
gNdsCfxFighterDamagePhaseCalls   1,938  both arms   (== c170/c174/c175/c176 bank)
gNdsCfxFighterDamagePhaseHits       20  both arms
gNdsCfxNarrowCalls               1,938  arm B, 0 arm A
gNdsCfxNarrowAnswered           ~1,938  arm B (declines expected 0)
gNdsCfxNarrowHits                   20  arm B  (== DamagePhaseHits)
gNdsCfxNarrowDeclined                0  arm B  (0-50 tolerated and explained)
```

Any hit flip changes damage, changes knockback, and changes the whole match, so
the per-symbol **call counts** in the `--diff` are the corroborating check: on a
same fight `gmCollisionCheckFighterAttackDamageCollide` must read the same count
on both arms, as it did at 1,552 across the c179/c180 pair.

## 3. The tick prediction, with the arithmetic that produced it

Read off `../2026-08-15_cfx-ring-draw0/conc-float-c180.txt` (float control,
`DRAW=0`, marginal-80 mask) and the compiled sizes in `FOOTPRINT.md` §5.

**Deleted** (per marginal frame): `gmCollisionTestRectangle` self 2,118 ·
`gmCollisionGetWorldPosition` self ≈ 1,529 (88% of 1,738; two of its calls per
non-transfer TestRectangle) · their soft float, priced at the measured per-call
averages `__mulsf3` 13.0 tk and `__aeabi_fadd` 18.3 tk and `__divsf3` 59 tk:
GetWorldPosition is 9+9 = 282 tk/call → ≈4,794, TestRectangle's own 3 fdiv +
~12 fadd/fsub + compares ≈ 557 tk/call → ≈6,600.
**Total ≈ 15,000 tk/fr, range 11,000–18,000.**

**Added** (per marginal frame): new fixed text executing at the ring's own
measured 0.467 tk/byte/call — `MakeFrame` 1,212 B, `TestRectangle` 1,504 B,
`LoadF32` 448 B of extra calls, ring glue ≈250 B; 11.85 calls/frame.
Between 60% and 100% of `TestRectangle`'s bytes touched:
**15,560–18,890 tk/fr.**

```text
PREDICTED marginal (rank-80 population) B - A :  +560 ... +7,890   (rate 1.04-1.26)
PREDICTED whole match                B - A :     +50 ... +670
PREDICTED VERDICT                          :     the lane CLOSES
```

**The known bias, stated rather than discovered afterwards:** this arm rebuilds
the joint frame **per pair** (11.85/frame) where the decomp inverts **per joint**
(6.95/frame, latched). A resident implementation would pay `MakeFrame` at the
lower rate. `MakeFrame` is its own census symbol, so its row can be rescaled by
6.95/11.85 after the fact rather than argued about now.

**The prediction to beat:** the previous cycle wrote down +6,000…+9,000 marginal
and measured +441 — wrong by 15x, from sizing off an attribution model instead of
waiting for the difference. This one is sized off per-call averages of shared
library symbols, which is the same class of estimator. Treat it accordingly.
