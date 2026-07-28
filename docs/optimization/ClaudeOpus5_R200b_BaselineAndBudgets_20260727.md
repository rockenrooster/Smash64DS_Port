# R2-00b — The re-baseline, and the budget table it corrects

**Date:** 2026-07-27
**Phase:** R2-00b (`docs/Smash64DS_Runtime2_SwitchPlan.md` §7).
**Status:** Census complete. Budgets frozen below. No runtime code changed.
**Standing rules apply** (`TASK_STANDING_RULES.md`).

**Instrument:** per-PC ARM9 profiler in the repo melonDS build
(`emulators/melonds/melonDS.exe`, SHA-256 `de80e46b…`), windowed by the Task 37
CP15 markers. Drivers `scripts/run-task37-profile-census.ps1` and
`scripts/task65_subsystem_census.py`.
**ROM:** `smash64ds-battle-playable-tickhud-hwtri`, `NDS_TASK37_PROFILE=1`,
`build-task37-profile`, master `0204c54329` + the docs repair `da4a654722`.
**Window:** presented frames 438–566 (128 frames), 449,272,412 cycles over
59,366 distinct program counters, 0 timestamp discontinuities.
**Evidence:** `artifacts/r2-00b-baseline-census/`.

---

## 0. Units, and the cross-check that validates them

The profiler counts ARM9 core cycles at 67.028 MHz; the ROM's tick HUD counts
`cpuGetTiming()` at 33.514 MHz. The factor is exactly 2 and every number below
is in tick-HUD units, directly comparable to the `P95 ≤ 1.12M` budget.

The conversion is not asserted, it is checked. Task 65 published 1,851,253
wall ticks/frame; its raw total was 473,920,686 cycles over the same 128-frame
window, and 473,920,686 / 128 / 2 = 1,851,253 exactly. The arithmetic below
reproduces Task 65's own published figure from its own raw input.

Two internal identities also hold exactly, which is what makes the split
trustworthy rather than merely plausible:

```text
cycles − non-mem stall − mem stall = 140,263,568 = retired instructions   OK
total cycles − tier cycles         =  79,015,679 = idle + unattributed    OK
```

## 1. The frame, today

| | ticks/frame | share |
|---|---|---|
| wall (what `ALL` measures) | 1,754,970 | 100% |
| idle VBlank wait (`armWaitForIrq`) | 308,622 | 17.59% |
| **REAL WORK** | **1,446,348** | 82.41% |
| 30 FPS budget (`PROJECT_GOAL.md`) | 1,120,000 | |
| **GAP** | **326,348** | |

Composition of the work:

| | ticks/frame | share of work |
|---|---|---|
| retired instructions | 547,912 | 37.9% |
| memory stall | 555,943 | 38.4% |
| non-memory stall | 342,494 | 23.7% |
| **total stall** | **898,437** | **62.1%** |

### Against Task 65

| | Task 65 (07-25) | now (07-27) | delta |
|---|---|---|---|
| wall | 1,851,253 | 1,754,970 | −96,283 |
| idle | 323,976 | 308,622 | −15,354 |
| REAL WORK | 1,527,277 | 1,446,348 | **−80,929** |
| gap to 1.12M | 407,277 | **326,348** | −80,929 |
| retired | 576,751 | 547,912 | −28,839 |
| memory stall | 587,532 | 555,943 | −31,589 |
| non-memory stall | 362,994 | 342,494 | −20,500 |

**The switch plan's §1 premise is confirmed and its headline number is stale.**
Tasks 100–108 plus the bug-#10 work bought 80,929 ticks/frame of real work,
5.3%. The gap is 326,348, not 407,277. The *shape* is unchanged: stall was 62%
and is 62.1%, and memory stall alone (555,943) is still larger than the whole
gap to 30 FPS (326,348). Every design rule in §3 stands on that ratio, and the
ratio did not move.

## 2. An attribution defect in the instrument, fixed here

`task65_subsystem_census.py` classified by DWARF source path with the rule
`("PORT/reloc", "src/port/reloc")`. That prefix also matches
`src/port/reloc_backend_renderer_dl.c`, which is **renderer adapter code that
happens to live under a `reloc_` filename** — `ndsFighterMarioFoxDLAllDrawForSlot`,
`ndsRendererAdapterBuildNativeProductionInputs`,
`ndsRendererAdapterBuildNativeMaterialSnapshot`.

The census therefore filed **147,777 ticks/frame — 10.2% of the frame's work —
into a bucket named after loading.** Task 65's §2 table carried the same defect,
so any plan built from it under-counted the renderer by that amount.

A more specific `REND/adapter` rule now precedes the generic one. Corrected:

| group | ticks/frame | % of work |
|---|---|---|
| REND/renderer | 575,777 | 39.81% |
| **REND/adapter** | **147,777** | **10.22%** |
| LIB/devkitpro | 147,456 | 10.20% |
| LIB/mem* | 98,166 | 6.79% |
| PORT/reloc *(actual relocation)* | 93,922 | 6.49% |
| NDS/other | 93,173 | 6.44% |
| SIM/system (decomp sy) | 84,986 | 5.88% |
| SIM/fighter (decomp ft) | 60,191 | 4.16% |
| *(remaining 11 groups)* | 144,901 | 10.02% |

**The renderer is 723,554 ticks/frame — exactly 50.0% of the frame's work.**
All gameplay (`SIM/*`, every decomp subsystem plus `src/import`) totals
**190,649**, or 13.2%.

By cross-cutting kernel: soft-float 178,617 (12.35%), matrix 141,815 (9.81%),
texture-resolve 108,188 (7.48%), `mem*` 98,166 (6.79%), gx-submit 89,915
(6.22%), rom-read 10,541 (0.73%).

## 3. Budgets, frozen

§4 of the plan called its table provisional and said R2-00 would freeze it
against a fresh census. Freezing it unchanged would be dishonest in two places,
so the table is amended and the amendments are argued.

| subsystem | §4 provisional | measured now | frozen budget | required cut |
|---|---|---|---|---|
| 60 Hz gameplay core, two logical ticks | 150K | 190,649 | **150K** | −40,649 |
| fighter visual pose / animation | 100K | *(inside the above and REND)* | **100K** | — |
| fighter rendering (both fighters) | 250K | — | **250K** | — |
| Dream Land | 180K | — | **180K** | — |
| background | 40K | — | **40K** | — |
| effects + audio | 80K | — | **80K** | — |
| camera + miscellaneous | 80K | — | **80K** | — |
| platform / presentation | 80K | — | **80K** | — |
| headroom | 160K | — | **160K** | — |
| **total** | **~1.12M** | **1,446,348** | **1,120,000** | **−326,348** |

Two corrections to how that table must be read:

1. **The renderer rows sum to 470K (fighters 250 + Dream Land 180 + background
   40) against 723,554 measured — a 35% cut, not a trim.** This is the third
   time this range has been asked for. `KNOWN_ISSUES.md` still lists M2 at
   385,088/388,224 against a 170–250K target and M3 at 489,184/489,536 against
   150–250K, both as open P1 blockers. R2's claim is that the architecture, not
   the effort, is what failed those targets. That claim is falsifiable at the
   R2-02 and R2-03 gates and should be treated as on trial there.

2. **§4 has no row for the renderer adapter.** It is 147,777 ticks/frame and it
   is not loading, not gameplay, and not the renderer core — it is the generic
   scaffolding §2 of the plan says R2 deletes. It is not given its own budget
   row because R2-02/R2-03 are supposed to *remove* it rather than shrink it:
   a direct owned path consumes baked facts and has no adapter. Its budget is
   therefore **0**, and any adapter cost surviving R2-05 is a phase failure, not
   a line item.

**Read the two attributions correctly — they are not interchangeable.** This
census attributes by where code *lives* (DWARF source path), so a matrix helper
called from the stage is charged to the renderer and counted once. The tick-HUD
`STG`/`FTR` buckets attribute by *bracket*, so the same helper is charged to the
stage. Task 65 chose the first deliberately, because the second double-counts
shared kernels when you sum buckets. Neither is wrong; mixing them is.

Concretely: symbol-name matching puts 197,928 ticks/frame in stage-named code
and 244,987 in fighter-named code, while Task 103 measured the `STG` *bucket* at
388,480 (≈366K after Task 104). The difference is the shared kernels — matrix,
texture-resolve, `mem*`, gx-submit, soft-float — which total 616,701 ticks/frame
across all callers and are exactly what §3.4 of the plan means by generic
scaffolding. **Phase gates must state which view they are quoting.** R2-02's
gate quotes `STG`, so its target is the bucket, and its 180K budget is a ~51%
bucket cut, not the 9% the symbol view would suggest.

The remaining nine rows are frozen as written. They were never measured
bottom-up and this census cannot measure most of them separately (the tick-HUD
buckets do not partition this way), so they stand as design targets to be
tested phase by phase, exactly as §7 intends.

## 4. What this does not answer

The instrument used here attributes **cycles**, not stall cause. Task 108
established that the P95 excursion at frames 453/454 executes the same
instructions in the same cycles as a median frame while the tick HUD charges it
~297,000 ticks/frame more — so the tail is a blocking relationship between the
CPU and some hardware unit, and no per-PC census can name it. Nothing in this
document addresses the tail. That is R2-00a's job.

Consequence for sequencing, and it is a real one: **R2-00a produces a new
emulator binary, and `PERF_LEDGER.md`'s own 2026-07-22 banner records what that
costs** — the last emulator change moved tick counts ~40% and made pre/post rows
non-comparable in absolute terms. When the attributor build is adopted, this
baseline must be re-run on it before any R2 phase gate reads absolute ticks. The
within-arm A/B deltas survive; the absolute numbers above do not.

## 5. Cost

One build, one 128-frame census run, one classifier fix. It replaced a two-day-old
baseline that was 5.3% stale, corrected a 147,777-tick attribution error that
Task 65 shipped and every later plan inherited, and produced the first honest
statement of what the renderer actually costs: half the frame.
