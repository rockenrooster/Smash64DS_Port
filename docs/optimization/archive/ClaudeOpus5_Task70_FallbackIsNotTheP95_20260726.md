# Task 70 — The fallback is real, and it is not what sets the P95

**Date:** 2026-07-26
**Status:** Negative result on the lever. Redirect, with the replacement named.
**Inputs:** `artifacts/task70-perframe.json` (128 frames, per-frame buckets and
per-frame native-owner fallback count on the same ring index),
`build-task70-census`.

Task 69 named a single cause for the native-owner fallback — the animation-lock
rejection, 10 of 256 draws — but could not show those draws land on the frames
that cost the P95. This task rings the per-frame fallback delta alongside the
buckets so the two share an index, and answers it.

## 1. The frames match exactly

The 10 fallbacks fall on 10 frames, in two runs of five:

**478, 479, 480, 481, 482** and **544, 545, 546, 547, 548.**

Those are precisely the runs Task 67 named from the per-frame bucket series, and
precisely the frames its two per-PC censuses were windowed on (544–545 and
478–479). Identity is settled: three independent instruments — profiler
attribution, bucket series, and now a direct per-frame counter — pick out the
same ten frames. The mechanism Task 67 described is real.

## 2. And removing it entirely would move the gate by 0.44%

| statistic | value |
|---|---|
| `WORK-H` P95, all 128 frames | 1,977,792 |
| `WORK-H` P95, the 118 clean frames only | **1,969,024** |
| difference | **8,768 (0.44%)** |

The separation test fails on its own terms too: the cheapest fallback frame is
1,700,928, and **14 of 118 clean frames reach it**. The two populations overlap
heavily.

Only two fallback frames are genuinely extreme — 478 at 2,664,064 and 544 at
2,743,360, the two highest values in the window, both the *entry* frame of a run.
The other eight sit at 1.70M–1.86M against a 1.36M median: elevated, ordinary,
and comfortably below the clean P95.

Two frames out of 128 cannot set a P95. **The fallback lever is worth ~9,000
ticks of an 857,792-tick gap — about 1%.** This falsifies the closing claim of
the Task 69 write-up, that narrowing the animation-lock rejection would cost the
P95 its driver. It would not. The rejection is worth fixing only if it is nearly
free.

## 3. What does set the P95

The ten most expensive frames with **no fallback at all**:

| frame | `WORK-H` | `SRC` | `FTR` | `STG` |
|---|---|---|---|---|
| 449 | 2,377,216 | **1,293,568** | 568,768 | 378,368 |
| 495 | 2,120,704 | **976,640** | 571,200 | 383,936 |
| 469 | 2,068,608 | **1,036,544** | 576,704 | 387,200 |
| 536 | 2,060,608 | **1,000,768** | 573,056 | 383,360 |
| 505 | 2,008,320 | **981,824** | 570,752 | 386,560 |
| 444 | 1,977,792 | **949,632** | 575,104 | 383,232 |
| 464 | 1,969,024 | **943,872** | 573,696 | 383,168 |
| 439 | 1,957,120 | **935,168** | 573,504 | 379,456 |
| *median frame* | 1,362,112 | 318,592 | 572,352 | 381,184 |

`FTR` is **flat** — 568,768 to 576,704 across every one of them, against a median
of 572,352. `STG` is flat, 377K–388K. The entire excursion is `SRC`, which swings
from a 318,592 median to 935K–1,294K.

Holding each bucket at its own median and recomputing:

| held at P50 | resulting `WORK-H` P95 | P95 saved |
|---|---|---|
| `SRC` | 1,718,208 | **259,584** |
| `FTR` | 1,975,040 | 2,752 |

`SRC` alone is **30% of the 857,792 gap**. `FTR` is 0.3%. And it is not rare: 26
of 128 frames run `SRC` above 1.5x its median.

## 4. Where this leaves the chain

Task 67 corrected Task 66 by showing that `SRC` charges renderer work rather than
simulation work — `SRC` spans `ndsTask39EffectsUpdate()` and
`scVSBattleFuncUpdate()`, which call outward into the renderer adapter. That
correction stands. What did not follow, and what this task retires, is the
inference that the `SRC` excursion and the native-owner fallback are the same
event. They are not: `SRC` spikes on 26 frames, the fallback occurs on 10, and
the eight biggest `SRC` frames have no fallback.

Both of Task 67's per-PC censuses were windowed on fallback frames — 544 and
478. Those are the 2 frames in 128 that are unrepresentative of the statistic
being gated. **No clean expensive frame has ever been profiled.**

## 5. Next

Window the per-PC profiler on a clean `SRC` frame — 469, 495 or 536, all
~2.06–2.12M with `SRC` near 1.0M, no fallback, and no BGM refill (449 is the
worst frame but confounds `AUD` 68,096 with `SRC` 1,293,568, so it is the wrong
one to start on). That names the functions behind the 630K–975K `SRC` excursion,
which is the largest single identified contributor to the gate that remains.

Standing rule this earns: **profile the frames that set the statistic, not the
frames that are easiest to spot.** A spike visible in a bucket series is not
automatically the spike the gate is made of.
