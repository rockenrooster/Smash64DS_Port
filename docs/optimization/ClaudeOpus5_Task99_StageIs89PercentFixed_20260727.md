# Task 99 — Half the stage's triangles are worth 19,584 ticks, and the last visual lever closes

**Date:** 2026-07-27
**Status:** **STOP on geometry decimation.** Probe reverted, no runtime change.
This closes the one visual lever Task 98 left standing.
**Authorized by:** the owner — "depends of the result, let's try it".
**Inputs:** `builds/build-task99-A` (baseline),
`builds/build-task99-B` (`NDS_TASK99_STAGE_TRI_KEEP=2`), 128 ring samples each,
frames 439–566, identical ROM/window/runner.

## 1. The one surviving hypothesis

Task 98 refuted encoding and texture resolution and named the reason: this
frame's cost is per-operation overhead, not per-datum throughput. It left exactly
one visual lever standing, on the argument that **fewer triangles is an
operation reduction, not a payload reduction** — fewer triangles means fewer
runs and fewer draws, not merely less data per draw.

The owner's question was whether a visibly simpler Dream Land would be worth
real ticks. Rather than estimate, a measurement probe answered it directly:
`NDS_TASK99_STAGE_TRI_KEEP` early-returns from
`ndsRendererNativeStageEmitNoZTriangle` for every triangle whose offset is not a
multiple of N, using that function's existing `return 0u` not-emitted path. At
N=2 the stage draws 101 of its 202 triangles. The result is visually broken by
construction; it exists to produce a slope.

## 2. Result

| bucket | A (202 tris) | B (101 tris) | Δ |
|---|---|---|---|
| **`STG` P50** | 370,496 | 350,912 | **−19,584 (−5.3%)** |
| `STG` P95 | 376,576 | 358,400 | −18,176 |
| `WORK-H` P50 | 1,320,128 | 1,304,000 | −16,128 |
| `WORK-H` P95 | 1,761,664 | 1,712,000 | −49,664 |
| `ALL` P50 | 1,680,064 | 1,680,000 | −64 |
| VBlank 3-interval | 512 | 525 | +13 |

```
101 triangles removed  ->  19,584 ticks
                       ->  ~194 ticks per stage triangle
all 202 triangles      ->  ~39,200 ticks
STG with no geometry   ->  ~331,300 of 370,496  =  89.4% fixed
```

**The stage bucket is ~89% fixed overhead.** Nine tenths of the 370,496 ticks
the stage spends every frame do not care whether any geometry exists.

This also retires a prior I formed an hour earlier: dividing the `STG` bucket by
its triangle count gave ~1,832 ticks/triangle and suggested halving the stage was
worth ~185,000. The measured marginal cost is **194** — the estimate was 9.4x
too high, and it was wrong in the familiar direction, attributing fixed overhead
to the per-item quantity.

## 3. Verdict

The trade on offer was: destroy Dream Land's silhouette to recover 16,128
(P50) to 49,664 (P95) against a **613,888** gap. Even reading the P95 optimistically
that is 8% of the gap for half the stage gone, and a decimation mild enough to
remain recognizable would land nearer 12,000–25,000 — inside the ±8,000
build-to-build placement noise floor doubled.

**Not worth making, and the owner does not need to weigh it.** A visual trade is
only worth putting in front of them when the tick side is real; here it is not.

## 4. What this settles

Three payload reductions, three refutations, one mechanism:

| lever | reduction | ticks | source |
|---|---|---|---|
| GX words | −355/frame | **+64** | Task 55 E2 |
| texels | any | ~0 (cost is ~1,621/bind) | Task 98 §3 |
| **stage triangles** | **−101/frame (−50%)** | **−19,584** | **this task** |

Task 98 predicted that only an *operation* reduction would pay, and treated
triangle count as one. That was too generous: a triangle is not an operation
here. The operations are binds, runs, draws and dispatches, and the stage issues
essentially the same number of them whether it carries 202 triangles or 101 —
the run structure is unchanged, only the corners inside each run were skipped.

**The refined rule: a change pays only if it removes a run, bind, draw or
dispatch — not if it removes items from inside one.** That is a much narrower
target than "fewer triangles", and it is the correct reading of every result
from Task 55 onward.

## 5. State

`WORK-H` P95 1,761,664 against the 1,120,000 gate on this build.

Visual approximation is now closed by measurement in all three of its forms
(encoding, resolution, geometry count), alongside the exactness-preserving
directions closed in Tasks 78–96. No lever of the required size is currently
identified, and the campaign should not propose another payload reduction of any
kind without first showing it removes operations rather than items.
