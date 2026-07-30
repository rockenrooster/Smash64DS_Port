# R2-03 E19 — the state spans cannot be priced by skipping them

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** **Method refuted, no number produced.** The arm removes far more than
it prices. `NDS_R2_FIGHTER_STATESPAN_SKIP` is kept at default 0 and should not be
used to cost this phase.

## 1. What was attempted

E18's one-build pricing method worked cleanly on the shade, so it was pointed at
the next ranked item: the per-epoch state spans, ~52,000/frame on E15's bracket.
Skip `ndsRendererNativeApplyStateSpan` either side of the material, measure the
difference.

## 2. Why the number is meaningless

| counter | control | span-skip arm |
|---|---:|---:|
| P0 hardware triangles / frame | 320 | **8.0** |
| P1 hardware triangles / frame | 306 | **0** |
| FTR P50 | 489,856 | 238,336 |

The arm reads **−251,520 FTR P50**, which is not the cost of the state spans. The
spans establish the texture, polygon-format and geometry-mode state the emit
requires; without them the runs are rejected before they submit, so the delta is
overwhelmingly the disappearance of ~618 triangles a frame.

The screenshot showed it first — both fighters gone except one small fragment —
and the triangle counters quantified it.

## 3. E18 re-checked against the same failure, and it holds

This failure mode applies to any skip arm, so E18's arm was re-measured with the
same counters before its number was allowed to stand:

| counter | E18 shade-skip arm | control rate |
|---|---:|---:|
| P0 hardware triangles / frame | **320** | 320 |
| P1 hardware triangles / frame | **306** | 306 |

Identical. The shade skip removed the colour computation and nothing else, which
is what the black silhouettes implied and what the counters now prove.
**E18's 53,760 stands.**

## 4. What the state spans still need

The skip method is unavailable here, so the honest position is:

- The only figure for the spans remains **E15's bracket, ~52,000/frame**, which
  carries that build's 10-20% instrument inflation and is safe for ranking only —
  exactly the caveat E18 was written to enforce. It must not be treated as a
  target.
- The right next method is **R2-02 F's**, not E18's: rather than deleting the
  spans, measure how much of the replay is *redundant* — how many adjacent epochs
  re-apply state that is already current. That is the same question R2-02 F asked
  of the stage's spans, it needs a counter rather than a deletion, and it prices
  the achievable cut instead of the whole phase.

## 5. For the standing rules

E18 gave the rule that an instrument's number is safe for ranking and unsafe as a
target. E19 gives its precondition:

**A skip arm prices a phase only if what remains still does its work. Verify that
with a structural counter, not with the tick delta.** Here the emit's triangle
count had to stay at its control rate; it went to zero, and the resulting
−251,520 would have looked like an enormous, entirely fictitious opportunity.

The generalisation is that a skip arm is a *dependency test* as much as a
measurement, and it silently converts into one when the skipped work feeds the
work being measured.
