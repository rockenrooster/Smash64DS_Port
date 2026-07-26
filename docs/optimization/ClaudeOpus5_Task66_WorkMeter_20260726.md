# Task 66 — A per-frame work meter, and the P95 the campaign was not measuring

**Date:** 2026-07-26
**Status:** KEEP. Instrument only; published ROM proven byte-identical.
**Published ROM:** `4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090`
before and after — every line added is inside `#if NDS_TICK_HUD`, verified by
building `smash64ds-battle-playable-hwtri` from HEAD and from the change and
comparing SHA-256.

Task 64 found that `ALL` is VBlank-quantized wall time and therefore cannot
show a saving smaller than one 560,190-tick period. Task 65 measured the split
with the host profiler but could only produce a **mean**, because that profiler
aggregates its whole window. `PROJECT_GOAL.md` gates the milestone on a **P95**.
This task closes that hole: the tick HUD now measures per-frame work directly,
so every future A/B reads a real percentile instead of an inferred average.

## 1. What was added

Two buckets, appended after `OTHR` so every historical index and every number
already in the ledger stays comparable:

- **`WAIT`** — the span the loop spends parked in `swiWaitForVBlank`,
  accumulated in `ndsPlatformEndFrame` around `ndsPlatformWaitForScheduledVBlank`
  on both the HW-triangle and framebuffer paths.
- **`WORK`** — `ALL − WAIT`, sampled as its own series. Not derived from the two
  printed percentiles, because the P95 of a difference is not the difference of
  two P95s.

`gNdsRendererProfileVBlankWaitTicks` already existed but only accumulates under
`NDS_RENDERER_PROFILE_LEVEL >= 1`, and both the tick-HUD and proof targets pin
the profile level to 0 — the wait was measurable everywhere except in the
configuration every measurement is actually taken in. `gNdsTickHudVBlankWaitTicks`
is tick-HUD-owned and has no such gap.

The on-screen table keeps rows 11–19 for the original nine buckets; `WORK` takes
row 20. `WAIT` is `ALL − WORK` and is read over GDB.

`scripts/sample-tick-hud-buckets.ps1` now derives its GDB field list, its printf
arity, its parse regex and its statistics loop from `$bucketNames`, so adding a
bucket is one line. It previously had the count written out in four places; the
first run of this task silently dropped both new buckets from the table because
one of them was missed and nothing else failed.

## 2. The meter agrees with the host profiler

Two unrelated instruments — a host per-PC cycle profiler and guest
`cpuGetTiming()` spans — on the same configuration:

| | Task 65 (host profiler) | Task 66 (tick HUD, mean) | agreement |
|---|---|---|---|
| idle VBlank wait | 323,976 | 323,502 | **0.15%** |
| real work | 1,527,277 | 1,498,086 (`WORK-H`) | 1.9% |

That is as close as two independent instruments get, and it settles the question
Task 64 left open: **`OTHR` really is the VBlank wait.** `OTHR` P50 337,088
against `WAIT` P50 320,832 — the 16K difference is the residual that `OTHR` also
carries. GX backpressure is not pooled in `OTHR`; Task 65 already showed it
distributed through the named buckets as memory stall.

## 3. The correction: the gate is nearly twice as far as the mean implied

128 samples, frames 438–565, tick-HUD ROM, `git=25b171f96`:

| bucket | P50 | P95 | spread | mean |
|---|---|---|---|---|
| ALL | 1,680,064 | 2,240,512 | 1.33 | 1,863,828 |
| FTR | 576,768 | 1,016,256 | **1.76** | 617,822 |
| STG | 386,560 | 392,576 | 1.02 | 387,222 |
| SRC | 317,120 | 948,800 | **2.99** | 392,292 |
| MISC | 47,104 | 157,248 | 3.34 | 73,772 |
| OTHR | 337,088 | 547,520 | 1.62 | 339,712 |
| **WAIT** | **320,832** | **531,584** | 1.66 | 323,502 |
| **WORK** | **1,389,696** | **2,066,560** | 1.49 | 1,540,326 |
| **WORK-H** | **1,371,776** | **1,985,024** | 1.45 | 1,498,086 |

`WORK-H` is `WORK` with the tick HUD's own console redraw subtracted per sample.
That redraw runs about twice a second and costs a few hundred thousand ticks
when it does — `HUD` spread is **318x**. It barely moves a mean, which is why
Task 65 did not see it, but it lands squarely on a P95. The published profile-0
ROM carries no tick HUD, so `WORK-H` is the honest published figure.

Against the `PROJECT_GOAL.md` budget of **P95 ≤ 1,120,000**:

| statistic | value | gap to budget |
|---|---|---|
| `WORK-H` P50 | 1,371,776 | 251,776 |
| **`WORK-H` P95** | **1,985,024** | **865,024** |

**Task 65's stated gap of 407,277 was a mean-based figure and understates the
actual gate by more than 2x.** That number should be read as "the typical frame
is ~250K over" — the milestone gate is the P95, and it is **865,024 ticks away.**

## 4. What that changes

**There are two problems, not one.**

1. **Steady-state cost.** `WORK-H` P50 is 251,776 over budget. This is what the
   Task 65 attribution describes and what specialization attacks.
2. **Burstiness.** `WORK-H` P95 is 1,985,024 — the same frame costs 45% more at
   the 95th percentile than at the median, and it is concentrated in exactly two
   buckets: **`SRC` spread 2.99** (317,120 → 948,800) and **`FTR` spread 1.76**
   (576,768 → 1,016,256). `STG` by contrast is flat at 1.02, which is a direct
   consequence of Task 44/53 steady-state work — the stage no longer has bad
   frames.

Burstiness was invisible while `ALL` was the gate, because `ALL` quantizes: a
frame that costs 45% more still reports 3 VBlanks until it crosses into 4.
`PROJECT_GOAL.md` does tolerate rare outliers ("P99: 1.70M may still constitute
a successful 30 FPS implementation"), but a 1.45 spread at P95 is not a rare
outlier — it is one frame in twenty.

**This does not change the Task 65 ranking, it adds a second axis to it.** The
renderer is still 52% of the work and still the campaign. But `SRC` — which
Task 65 correctly showed is only ~10% of *mean* work and therefore not worth a
specialization rewrite for its size — turns out to be the **largest single
contributor to the P95 problem**, tripling on bad frames. Those are different
findings about the same bucket and both are true: `SRC` is not where the frame
usually goes, and `SRC` is where the frame goes when it goes badly.

Finding what makes `SRC` triple is now a prerequisite, and it is a cheap
question: the tick HUD can already be sampled per frame, so correlating `SRC`
spikes against match events costs one run, not a rewrite.

## 5. Verification

- Published ROM `smash64ds-battle-playable-hwtri` SHA-256 identical before and
  after (built both ways from the same tree state).
- `scripts/check-gbi-decode-fixtures.ps1` passes, with the Task 41 HUD assertion
  extended to pin the new bucket names and the `WAIT` accumulation site.
- `scripts/sample-tick-hud-buckets.ps1` reports 11 buckets plus `WORK-H`;
  `WAIT` agrees with the Task 65 host profiler to 0.15%.
- Baseline recorded at `artifacts/task66-tickhud-baseline.json`.

## 6. Standing rule this replaces

Task 64 said "search against work, gate on `ALL`" and left the work figure to be
inferred. It no longer needs inferring:

> **Search on `WORK-H` P50. Gate on `WORK-H` P95 ≤ 1.12M. Use `ALL` only to
> confirm the VBlank step was actually crossed.**
