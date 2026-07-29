# R2-03 E21 — the state-delta guard is refuted, and E20's 64.2% was the wrong statistic

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** **REFUTED.** The elidable work is ~3,920 ticks/frame, below the
build-placement floor. Do not build the guard. E20's board entry is corrected.

## 1. What E20 claimed and what was missing

E20 measured that **64.2% of state-delta applications in a frame re-apply a
delta index that frame already applied** — 124.8 repeats of 194.4 applications —
and bounded the elidable work at ~35,000 ticks/frame, calling it the best
return-to-risk on the board.

It also stated the premise it had not tested: *"applied twice in a frame" is not
"the second was a no-op"*, because something in between may have changed that
state. The correct guard is value-based, and the falsifier was named as one
build.

## 2. The falsifier

Every case in `ndsRendererNativeApplyStateDelta` writes `stats` purely from
`delta->w0` / `delta->w1`, so identical operands to the previous application of
the same effect means identical writes. Per-effect last-operand tracking counts
those. Validity is cleared on every material application, since materials write
the same `stats` fields — which makes the count conservative, the safe direction.

## 3. Result

479 frames, both fighters:

| counter | per frame | share |
|---|---:|---:|
| delta applications | 194.4 | |
| within-frame index repeats (E20) | 124.8 | 64.2% |
| **identical-operand applications** | **14.0** | **7.2% of applications, 11.2% of repeats** |
| of those, GEOMETRY | 0 | |
| material invalidations | 29.7 | |

`gNdsFighterDLAllDrawP0HardwareTriangleCount` reads 320/frame, its control rate,
so E19's structural check passes and the arm measures what it claims.

**Only 14 of the 194.4 applications a frame re-write what is already there.** The
other 110-odd "repeats" are genuine state changes — the same delta index applied
again with different operands, which is a legitimate state transition and not
redundancy at all.

## 4. Why the cut dies

14.0 applications at ~280 ticks is **~3,920 ticks/frame**, against a
build-placement noise floor of 5,000–7,000. The saving is not measurable.

And it is not free: a value-compare guard pays a comparison on **all 194.4**
applications to skip 14. That is E8's shape exactly — the local-matrix memo that
cost +16,301 ticks/frame and was deleted — and it would very likely be a net
loss.

**Do not build it.**

## 5. The methodological point, which is the durable part

E20's 64.2% was a real measurement of the wrong thing. "The same delta index is
applied again this frame" sounds like redundancy and is not: a state machine that
re-visits the same *knob* with *different values* is doing necessary work, and
counting knob-visits measures the shape of the replay rather than its waste.

**Count identity of the write, not identity of the target.** The two differ here
by a factor of nine, and the first one produced a 35,000-tick opportunity that
does not exist.

This is the third time in this cycle that a plausible headline number survived
until one more counter was added — E13's inert offscreen probe, E19's collapsed
geometry, and now this. In all three the cost of the extra counter was one build,
and in all three the number would otherwise have been acted on.

## 6. R2-03's queue after this

| cut | size | status |
|---|---:|---|
| E17 split matrix load | −17,600 | built, Boundary green both arms, **awaiting visual approval** |
| E16 hardware lighting | 35,000–50,000 | ceiling measured at 53,760; four-part change, light-space risk |
| ~~E20 state-delta guard~~ | ~~25,000–30,000~~ | **REFUTED, ~3,920** |
| per-root matrix work | ~40,000 (inflated bracket) | unpriced |

With E20 gone, **E16 is again the only large cut identified in the phase**, and
the per-root matrix work is the only unpriced item left worth measuring. E16's
sequencing advantage — that E17 already establishes its vector matrix — now
matters more, not less.
