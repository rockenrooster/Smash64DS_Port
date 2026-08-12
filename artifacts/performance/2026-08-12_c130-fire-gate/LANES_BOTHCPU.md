# Lane ceilings and tail attribution, RE-COMPUTED ON `BOTH_CPU 1`

This replaces `../2026-08-12_c125-slice48/EXHAUSTION.md`'s ceiling table, every
row of which was computed on the Boundary arm after `build-c124-slice48` turned
out to be `NDS_R2_BOTH_CPU 0` (`../2026-08-12_c126-armcheck/ARM_MISLABEL.md`).
`HANDOFF.md` has carried "must be recomputed before any lane is trusted as dead"
since that discovery. This is that recomputation.

**No build and no new run were spent.** The distribution is
`c130-gate-rows.csv`, the 1600-sample `BOTH_CPU 1` gate run taken to price the
R2-07 bug fixes: frames 438+, `-RingDump`, DLDI ON, `slips=0`, `WORK-H` P50
944,256 / P95 1,217,472. The ROM carries the two bug fixes on top of HEAD, worth
18 particle quads over ~12 frames of the window (`GATE.md`), which is immaterial
to lane sizing.

Both tables now come out of `analyze-tick-hud-excursion.ps1` (`-Ceilings`), so
the next re-bank is one command rather than a hand computation in a document —
which is how the c125 numbers survived on the wrong arm with nothing able to
re-derive them.

## Where the gate stands

```
WORK-H P50   944,256      P95 1,217,472      over gate 168/1600 (10.5%)
clean-frame P95 1,075,904   gap to gate 97,092
```

## Who owns the tail (hot mean − clean mean, 168 hot / 1432 clean)

`SRC` **334,234 = 84.4%** of the 395,863-tick excursion; `STG` 25,677, `MISC`
15,936, `AUD` 12,748, `FTR` 7,132, `OTHR-WAIT` 132, `BG` 3. Identity closes with
per-frame max error 0.

| SRC sub-owner | excursion | %SRC | clean mean | hot mean | hot/clean |
|---|---:|---:|---:|---:|---:|
| `SITR` | **156,723** | 46.9% | 106,951 | 263,674 | 2.47x |
| `SHDT` | **76,023** | 22.7% | 6,245 | 82,268 | **13.17x** |
| `SPHD` | 40,319 | 12.1% | 62,183 | 102,502 | 1.65x |
| `SPRM` | 24,681 | 7.4% | 2,119 | 26,800 | **12.65x** |
| `SOBJ` | 23,895 | 7.1% | 88,819 | 112,714 | 1.27x |
| `SCPU` | 12,561 | 3.8% | 45,948 | 58,510 | 1.27x |

## Lane ceilings — the most each lane could ever pay

Baseline (rank 80 of 1600) **1,220,480**; gate 1,120,380, so **100,100 to find**.
Read the deltas, not the absolutes: the rank statistic sits slightly above the
harness percentile by construction.

| lane | ceiling, `BOTH_CPU 1` | c125's Boundary-arm figure | change |
|---|---:|---:|---|
| `SRC` | **207,104** | 133,056 | **+56%** |
| `GCRA` | **207,104** | 133,056 | +56% |
| `SBAS` | 140,992 | — | — |
| `SGCO` | 135,040 | — | — |
| `SINT` | 104,768 | 57,280 | +83% |
| `SITR` | **83,712** | — | — |
| `SHDT` | **51,584** | 38,912 | +33% |
| `SPHD` | **44,288** | 17,152 | **+158%** |
| `MISC` | 23,552 | 31,680 | −26% |
| `SOBJ` | 17,472 | — | — |
| `SCPU` | **15,104** | 4,288 | **+252%** |
| `SPRM` | **11,008** | 2,944 | **+274%** |
| `AUD` | 11,008 | 13,312 | −17% |
| `FTR` | **8,512** | **0** | **not zero** |
| `STG` | 3,008 | 2,048 | +47% |
| `OTHR-WAIT` / `SOUT` / `SPHC` | 576 / 192 / 128 | — | — |
| `BG` / `SCAT` / `SWRM` | 0 | 0 | — |

**Lanes nest** — `SRC` contains `GCRA` contains `SGCO` contains
`SITR`/`SPHD`/`SPHC`/`SOBJ`. **These ceilings do not add.**

## What the correct arm changes

1. **The gate is reachable inside `SRC` alone.** Its ceiling 207,104 is 2.07x the
   100,100 gap. On the Boundary-arm table it was 133,056 against a gap that made
   the lane look barely sufficient.
2. **Three lanes were sized on the wrong arm and are 2.5–3.7x larger here.**
   `SCPU` +252%, `SPRM` +274%, `SPHD` +158%. `SCPU` is `ftComputerProcessAll` and
   the reason is structural rather than incidental: this arm runs **two** level-3
   CPUs, so the AI runs twice per frame. Any conclusion of the form "the CPU lane
   is too small to matter" was drawn from a match with one CPU in it.
3. **`FTR`'s ceiling is not zero.** `HANDOFF.md` records "`FTR` — 0/80 on the
   tail: NOT a P95 lever". That is a Boundary-arm verdict; here it is 8,512.
   Small, still not a lane to open first, but the *reason* it was closed no
   longer holds and should not be quoted as settled.
4. **`SHDT` and `SPRM` are the presence lanes.** 13.17x and 12.65x hot/clean, on
   medians of 4,416 and 2,048 — nearly absent at P50 and worth 51,584 and 11,008
   at P95. This is the shape the campaign has twice mistaken for a dead lane by
   ranking on mean self time.
5. **`MISC` and `AUD` shrank.** Both are smaller here than on the Boundary arm,
   so the two lanes most recently ground are the two the correct arm deprioritizes.

## The lane this names next

`SITR` — the fighter interrupt proc with `SCPU` nested inside it — carries the
largest single excursion (156,723, 46.9% of `SRC`) and a ceiling of 83,712, which
is 84% of the gap on its own. `SHDT` is second at 51,584 with the sharpest
presence signal in the table.

`SHDT`'s reach-bound lever is refuted geometrically (slice 47: `ReachTests 2,373
WouldSkip 0`) and must not be retried — but **one refuted lever is not a refuted
lane**, and on this arm the lane is a third larger than the figure that lever was
judged against.
