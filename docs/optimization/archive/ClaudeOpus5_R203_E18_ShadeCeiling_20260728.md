# R2-03 E18 — E16's ceiling is 53,760, not 90,295

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** Measurement. **E16's estimate was ~40% too high and is corrected
here before anyone builds it.** `NDS_R2_FIGHTER_SHADE_SKIP`, lab only, default 0.

## 1. Why this ran before the implementation

E16 proposed replacing the fighter's per-vertex software lighting with the DS
geometry engine's hardware lighting, and priced it at "most of 90,295
ticks/frame" from E15's shade bracket.

That figure came from a build carrying the whole E15/E16 census, whose counters
inflate everything they touch, and the bracket enclosed the per-epoch preamble as
well as the per-vertex loop. Hardware lighting replaces the loop; it does not
delete the preamble. So the number that matters was never measured.

The implementation is a four-part change whose main risk is the light-space
mapping. Pricing it first is cheap and decides whether that risk is worth taking.

## 2. Method

`return TRUE` immediately before the per-vertex action loop, leaving the light
direction preparation and LUT lookup in place — so the arm isolates exactly the
work hardware lighting would take over, and nothing else.

Both arms carry `NDS_R2_FIGHTER_HW_MTX=1`, so E17 is held constant.

**Engagement is unambiguous:** with the loop skipped,
`sNdsNativeFighterPreparedDense[].packed_color` is never written and both
fighters render as solid black silhouettes against an untouched stage. A tick
delta could be argued with; this cannot.

## 3. Result

128 presented frames each.

| bucket | shade on | shade skipped | delta |
|---|---:|---:|---:|
| **WORK P50** | 1,099,584 | 1,044,800 | **−54,784** |
| WORK P95 | 1,528,064 | 1,533,056 | +4,992 |
| **FTR P50** | 489,856 | 436,096 | **−53,760** |
| FTR P95 | 962,624 | 927,040 | −35,584 |
| STG P50 | 175,296 | 173,824 | −1,472 |
| **VBlank 2 / 3** | 381 / 167 | **431 / 123** | **+50 frames at 30 FPS** |

`STG` moves 1,472 — untouched, and the placement floor that makes `FTR`'s
53,760 real.

## 4. The correction

**E16 said "most of 90,295". The measured ceiling is 53,760** — about 40% less.

The gap is instrument inflation plus bracket scope, in the direction this
campaign has been caught by before but not previously in an *estimate handed
forward as a target*. E15 already carried the warning that its absolutes were
inflated 10-20% and only its ranking was safe; E16 then quoted an absolute from
it anyway. The ranking was right — the shade is still the largest single item in
the fighter — and the size was not.

**Ceiling, not expected value.** Hardware lighting must still write GX light and
material state per epoch or per root, so the realised saving will be below
53,760. The honest range for E16 is **35,000–50,000**.

## 5. Does E16 still justify itself

Yes, and the case is now quantitative rather than rhetorical:

- 53,760 is 21% of R2-03's 250,833 gap; with E17's 17,600 already measured, the
  two together are ~28% of it.
- It moves 50 of 566 frames from three VBlank intervals to two on its own.
- Nothing else identified in R2-03 is larger. The remaining ranked items are
  epoch state spans (~52,000) and per-root matrix work (~40,000), both of which
  are *also* worth doing and neither of which is bigger.

But it is a four-part change against a 35,000–50,000 return, not the ~90,000 that
was on the board. That is still the best cut available in the phase; it is no
longer an obvious one, and the sequencing should reflect it — **E17 graduates on
its own merits first, and the epoch state spans deserve pricing the same way
before E16 is built**, because they may be cheaper per tick won.

## 6. For the standing rules

E13 already established that an all-zero reading needs a positive control. This
is the mirror: **an estimate carried forward from an instrumented build needs
re-measuring on a clean one before it becomes a target.** The rule that E15's
absolutes were unsafe was written in E15's own document and then not applied one
experiment later.

Cheapest general form: when a proposed cut is priced off a bracket, spend one
build disabling the thing being replaced and measure the difference. It costs a
build and it caught a 40% error here.
