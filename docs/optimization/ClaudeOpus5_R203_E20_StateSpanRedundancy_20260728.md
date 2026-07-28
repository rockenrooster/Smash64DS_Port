# R2-03 E20 — the state replay re-applies the same delta 1.8 times a frame

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** Measurement. **64.2% of state-delta applications in a frame are
repeats**, bounding the elidable work at ~35,000 ticks/frame. The cut is a
value-compare guard, and the premise it still needs is named in §4.

## 1. Method, chosen because E19 refuted the other one

E19 established that the spans cannot be priced by deleting them: they are
load-bearing, and removing them takes the emit with them. So this asks R2-02 F's
question instead — not *what does the phase cost* but *how much of it is
redundant*, which is what a guard could actually elide.

There are 70 fighter state deltas. A per-delta frame stamp counts how many
applications in a frame re-apply a delta that frame has already applied.

**E19's structural check applied:** `gNdsFighterDLAllDrawP0HardwareTriangleCount`
reads 320/frame, its control rate, so the arm measures what it claims and has not
quietly deleted the payload.

## 2. Result

479 frames, both fighters, per presented frame:

| counter | per frame |
|---|---:|
| state-span calls | 80.2 |
| **delta applications** | **194.4** |
| **of which repeats within the frame** | **124.8 (64.2%)** |
| distinct applications | 69.6 |
| span cost (`gNdsR2ExecStateTicks`) | 54,510 |

There are 70 deltas in the table and 69.6 distinct applications a frame:
**essentially every delta is applied once for real and then about 1.8 more times
redundantly.**

At 54,510 ticks for 194.4 applications — 280 ticks each — the 124.8 repeats are
worth **~35,000 ticks/frame**.

## 3. What this is worth

~35,000 is an **upper bound**, and the guard is not free: a value compare costs
something per application. If the compare is a fifth of an apply, the realised
cut is **25,000–30,000 ticks/frame**.

That places it below E16's 53,760 ceiling and above E17's shipped 17,600. Against
R2-03's 250,833 gap it is ~12%, and it is a far smaller change than E16 — a guard
inside one function, no light-space reasoning, no load-time table, no emit
change.

## 4. The premise this still needs, stated rather than assumed

**"Applied twice in a frame" is not the same as "the second one was a no-op."** A
delta may legitimately need re-applying because something between the two
applications changed that state — a material application, or another delta
writing the same field. This counter cannot see that.

So the correct guard is **value-based, not frame-based**: write only when the
value differs from what the renderer currently holds. That is correct under any
interleaving, and it is also the only version whose saving equals the measurement.

**Before building it, the falsifier is: of the 124.8 repeats, how many write a
value equal to the current one?** That is the E5 pattern — one counter, one build,
and it converts this upper bound into the actual number. A frame-stamp repeat that
turns out to write a *different* value is not redundancy at all, and if most of
them do, this cut collapses the way E19's method did.

## 5. Position in the phase

| cut | size | status |
|---|---:|---|
| E17 split matrix load | −17,600 | built, Boundary green both arms, awaiting visual approval |
| E16 hardware lighting | 35,000–50,000 | measured ceiling 53,760; four-part change, light-space risk |
| **E20 state-delta guard** | **25,000–30,000** | premise measured, falsifier named, not built |
| per-root matrix work | ~40,000 (inflated bracket) | unpriced |

E20 is the best ratio of return to risk currently on the board, and its falsifier
costs one build.
