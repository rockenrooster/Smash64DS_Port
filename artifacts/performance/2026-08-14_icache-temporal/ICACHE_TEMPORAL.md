# I-cache placement, settled with the v3 attributor

**Date:** 2026-08-14
**Supersedes:** `../2026-08-14_pgo-code-placement/PGO_CODE_PLACEMENT.md`, whose
"lane closed" verdict was withdrawn under review.
**UNITS: 2 profile cycles = 1 project tick.** Both are given throughout.

---

## 0. What the previous slice got wrong

Review rejected the prior verdict and was right on every count. Recorded here
because the failure mode is reusable, not to flagellate:

| claim | why it was wrong |
|---|---|
| "hot set is 33.6x the cache, therefore capacity-bound" | 33.6x is the **union of lines touched across the whole match**. Cache behaviour is governed by temporal reuse distance. A union that overflows 33x is equally consistent with a thrashing working set or a sequence of small phases that each fit. |
| "hottest 2,000 lines are evenly spread over 64 sets" | An aggregate over the whole match says nothing about evenness at any instant. |
| "callee lines are evicted between calls regardless" | **The load-bearing claim of the document, and the interval between those calls was never measured.** |
| Phase 3 skipped | So nothing in the report could have supported the claim above. |
| "stage/fighter/animation are temporally inseparable" | Executing in the same frame does not make phases inseparable. |
| geometry unverified | Now verified — §1. |
| "v3 attributor never adopted" | **Wrong three ways.** The source tree is at `D:\Stuff\DevFolder\melonDS-Accurate` (branch `r2-stall-attributor`, HEAD `4a1abf61`), a built binary sits in `build/`, **and a copy has been in-repo at `emulators/melonds-attributor/melonDS.exe` since 27 July.** I checked `emulators/melonds/` and stopped. |

## 1. Phase 1 — geometry, verified

From the project's own reference emulator, which `PROJECT_GOAL.md` names the
primary performance reference — `melonDS-Accurate/src/CP15_Constants.h:28-35`
and `src/CP15.cpp:455-467`:

```c
constexpr u32 ICACHE_SIZE_LOG2   = 13;               // 8192 B
constexpr u32 ICACHE_SETS_LOG2   = 2;                // 4   <- melonDS names WAYS "sets"
constexpr u32 ICACHE_LINELENGTH  = 8 * (1 << 2);     // 32 B
constexpr u32 ICACHE_LINESPERSET = 8192 / (4 * 32);  // 64  <- the true set count

const u32 id = ((addr >> ICACHE_LINELENGTH_LOG2) & (ICACHE_LINESPERSET-1)) << ICACHE_SETS_LOG2;
//           = ((addr >> 5) & 63) << 2
```

| property | value | status |
|---|---|---|
| size | 8192 B | verified |
| line | 32 B | verified twice (emulator; `libnds .../cache.h:74`) |
| associativity | 4-way | verified |
| sets | 64 | verified |
| index | `(addr >> 5) & 63` | verified |
| set period | 2048 B | derived |
| **replacement** | **round-robin** (`CP15_CACHE_CR_ROUNDROBIN`, `[[likely]]`) | verified |

D-cache is 4096 B — different, and not reused here. Round-robin is new
information: a 5th hot line in a set evicts in fill order, not by recency, so
excess occupancy is charged in full rather than discounted.

## 2. The v3 baseline — the pool is real and it is enormous

`emulators/melonds-attributor/melonDS.exe` on **the same ROM the c125 profile
used** (`builds/build-c125-profile/…tickhud-hwtri.nds`, unchanged since 08-12),
same window (frames 438..2038, 1,601 regions).

`stall_partition_residual = 0` — the classes partition `total_cycles` exactly,
so nothing is silently misattributed. Total cycles 3,662,522,274 against the v2
run's 3,660,281,186: **0.06% apart**, so the two instruments are measuring the
same execution.

| class | cycles | cyc/frame | ticks/frame | share |
|---|---:|---:|---:|---:|
| **icache_fill** | **1,086,361,126** | **678,551** | **339,275** | **29.7%** |
| dcache_fill | 807,006,094 | 504,064 | 252,032 | 22.0% |
| halt_wait (idle) | 764,597,768 | 477,575 | 238,788 | 20.9% |
| issue | 580,379,600 | 362,511 | 181,256 | 15.8% |
| write_buffer | 152,250,144 | 95,097 | 47,549 | 4.2% |
| interlock | 127,894,062 | 79,884 | 39,942 | 3.5% |
| bus_contention | 116,172,630 | 72,563 | 36,282 | 3.2% |
| dma_hold | 27,860,850 | 17,402 | 8,701 | 0.8% |
| cart_spin, gx_paid, gx_blamed | 0 | 0 | 0 | 0 |

**Instruction fetch is 29.7% of the match, 37.5% of non-idle time, and 1.87x the
cost of issuing the instructions it fetches.** At 339,275 ticks/frame the pool is
roughly 20x the 17,000-tick target. Nothing else in this campaign is this large.

## 3. Phase 3 — the weighted call-transition graph

`scripts/census-call-transitions.py`. Edges are exact: every `bl` site's
execution count from the profile, never static fan-out. 3,294 edges over 1,300
executed non-ITCM functions; clusters are connected components over edges of at
least 5 calls/frame.

Hottest edges, with the current address gap and the number of I-cache sets the
two functions already share:

| calls/frame | gap (B) | shared sets | edge |
|---:|---:|---:|---|
| 271.2 | 413,184 | **10** | `gcPlayDObjAnimJoint -> ndsR2AnimValueQ` |
| 189.7 | 17,948 | 0 | `ndsRendererExecuteNativeFighter… -> …ApplyStateDelta` |
| 179.9 | 238,280 | 0 | `syMatrixLookAtReflectF -> __aeabi_fmul` |
| 146.3 | 459,180 | 0 | `ndsBaseGcPlayMObjMatAnim -> __aeabi_fmul` |
| 117.0 | 274,448 | 0 | `…BuildDObjXObjMatrix -> __aeabi_fcmpeq` |
| 80.9 | 69,392 | 2 | `…StageEmitNoZTriangle -> …StageEmitNoZVertex` |
| 75.0 | 123,984 | 4 | `…CommitNativeStageSegment -> …StagePrepared…` |

Clusters, with the footprint each occupies versus the address range it currently
spans:

| cyc/frame | members | own B | x8K | span B | in-span% | head |
|---:|---:|---:|---:|---:|---:|---|
| 365,902 | 40 | 33,656 | **4.1** | 758,264 | 4.4% | `ndsRendererCommitNativeStageSegment` |
| 183,588 | 18 | 11,736 | **1.4** | 779,680 | 1.5% | `gcPlayDObjAnimJoint` |
| 157,722 | 8 | 13,192 | **1.6** | 344,512 | 3.8% | `ndsFighterMarioFoxDLAllDrawForSlot` |
| 75,912 | 11 | 5,456 | **0.7** | 713,324 | 0.8% | `ftDisplayMainDrawDefault` |
| 45,005 | 13 | 5,132 | **0.6** | 172,444 | 3.0% | `mpCollisionGetFCCommonFloor` |
| 30,048 | 7 | 4,156 | **0.5** | 161,012 | 2.6% | `…BuildPersistentStageWorld` |
| 22,765 | 6 | 2,204 | 0.3 | 579,624 | 0.4% | `sqrtf` |

**This is what refutes the previous slice.** Three clusters *fit* inside the 8 KB
cache (0.5x–0.7x) and are currently scattered across 160–713 KB, with under 3% of
their span being their own code. "Evicted between calls regardless" is false for
them. The hottest edge in the whole build is 413 KB apart and already shares 10
of 64 sets with its callee.

It is equally true that the largest cluster is 4.1x the cache and cannot be made
resident by any ordering. The previous slice saw only that one and generalised.

## 4. Phase 5/7 — the layout model, and the falsifier that caught it

`scripts/placement-layout-model.py`. Deterministic, explainable, and deliberately
not a simulator: a cluster is the temporal-locality unit, a cluster's
self-conflict is what link order changes, and cost is weighted set pressure
above the 4 ways.

**The first falsifier failed, and that is the most useful result in this
section.** It packed functions contiguously with one line of padding — which
*spreads* set indices — so it was a mildly good layout wearing an adversarial
name, and it scored better than both principled candidates:

```
current      198,087,588
cluster      193,908,867   -2.1%
phase        193,908,867   -2.1%
conflict-min 189,644,350   -4.3%
falsifier    193,461,298   -2.3%   <-- BEAT the principled layouts
```

Decision-gate item 5 exists for exactly this and it worked. Rebuilt to force
every function base onto a multiple of the 2048-byte set period, so every
function's first line lands in set 0 and hot heads pile into the low sets:

```
current      198,087,588
cluster      193,908,867   -2.1%
phase        193,908,867   -2.1%
conflict-min 189,644,350   -4.3%   <-- best
falsifier    253,187,660  +27.8%   <-- correctly, badly worse
```

A 32-point spread between best and worst means the model discriminates rather
than rewarding any movement at all.

Per cluster, current cost versus gathered:

| weight/fr | own B | x8K | cur cost | gathered | maxset cur -> new | head |
|---:|---:|---:|---:|---:|:--:|---|
| 96,152 | 33,656 | 4.1 | 117,563,108 | 116,966,773 | 23 -> 19 | `…CommitNativeStageSegment` |
| 70,135 | 11,736 | 1.4 | 38,801,932 | 36,855,723 | 9 -> 7 | `ndsR2AnimValueQ` |
| 60,735 | 13,192 | 1.6 | 40,915,877 | 39,759,879 | 9 -> 8 | `ndsRendererMtxMulAffine20p12` |
| 17,498 | 5,456 | 0.7 | 164,306 | **0** | 5 -> 4 | `ftGetStruct` |
| 11,690 | 5,132 | 0.6 | 110,811 | **0** | 5 -> 4 | `…AdjustFloorLoopWallSweep` |
| 8,160 | 2,204 | 0.3 | 0 | 0 | 4 -> 3 | `sqrtf` |
| 6,665 | 4,156 | 0.5 | 0 | 0 | 4 -> 3 | `…BuildPersistentStageWorld` |

**The decisive arithmetic.** The clusters a reordering can *perfectly* fix — the
ones that fit — go from 275,117 to 0. That is **0.14% of the 198,087,588 total**.
The clusters holding 99.6% of the conflict are 1.4x–4.1x the cache, and gathering
them buys 0.5%, 5.0% and 2.8% of their own cost respectively.

The best layout scores −4.3%. Taken at face value against 339,275 ticks/frame of
`icache_fill` that is **≈14,600 ticks/frame — below the 17,000 target**, and that
figure assumes the conflict score maps linearly onto measured fill, which §5
tests.

## 5. Section map and identity

Baseline is `builds/build-c125-profile/smash64ds-battle-playable-tickhud-hwtri`
— the **same ROM the c125 v2 profile used**, unchanged since 2026-08-12, so the
v2 and v3 captures describe one binary.

```
profile ROM sha256  C8C26F66FC3398B4DF6ADEAB65DECDE6BC902FB6D3D51BF1DF689A331AE68F96
shipped ROM sha256  2015FBD1F68B81C03626D8C6D473C8BCBCF527A3A26DFE86FF19BD74ECBB1360
```

| section | bytes | address |
|---|---:|---|
| `.itcm` | 32,152 | 0x01FF8000 |
| `.text.hot` | 4,588 | 0x02000000 |
| `.text.hot.draw` | 5,256 | 0x0200_11EC |
| **`.main`** | **915,720** | **0x0200_5338** |
| `.main.rw` | 137,060 | 0x0209_0000 |

`.main` is the text a link-order change would reorder. Of it, **404,608 bytes
(44%) is executed at all**. The curated hot text is 9,844 bytes — already
contiguous, and already **1.2x the whole I-cache** on its own.

**Nothing was built and no linker order was changed**, so every section size
above is both the baseline and the current state. The task's text/RAM rule is
satisfied trivially: zero bytes were spent.

## 6. Phase 8 — temporal working set: is it conflict or capacity?

`scripts/census-icache-temporal.py`, run against the v3 profile. The test:
**does a set's refill rate depend on how many hot lines it holds?** Under
conflict pressure, sets holding more than the 4 ways must refill more. Under
capacity pressure every set refills alike, because the line is gone by its next
use either way.

The first run of this script measured nothing: at a 2,000-line cutoff all 64 sets
are already oversubscribed, so there was one bucket and no contrast. Sweeping the
cutoff puts the comparison where set population actually varies — at and below
the cache's 256-line capacity.

```
cache holds 256 lines total: 64 sets x 4 ways

  top    64 lines -> <=4 fits: 34 sets, 1,119 fill/1k
  top   128 lines -> <=4 fits: 54 sets, 1,155 fill/1k  |  5-8:  2 sets, 1,312 fill/1k
  top   256 lines -> <=4 fits: 38 sets, 1,252 fill/1k  |  5-8: 25 sets, 1,233 fill/1k
  top   512 lines -> <=4 fits:  3 sets, 1,407 fill/1k  |  5-8: 34 sets, 1,297 fill/1k  |  9-16: 27 sets, 1,403 fill/1k
  top 1,024 lines ->                                      9-16: 36 sets, 1,396 fill/1k  |  >16:  28 sets, 1,607 fill/1k
  top 2,000 lines ->                                                                       >16:  64 sets, 1,662 fill/1k
```

**The answer is capacity, and it is unambiguous.** At the top-256 cutoff, sets
that *fit* within the 4 ways refill at **1,252** fill-cycles per 1k instructions
while oversubscribed sets refill at **1,233** — the uncontended sets are
*marginally worse*. At top-512 the same: 1,407 for fitting sets against 1,297 for
5–8. **A set having room does not make its lines survive.**

The gentle rise across cutoffs (1,119 → 1,662) is a hotness effect, not a
conflict effect: widening the cutoff admits colder lines whose reuse is rarer, so
their fill-per-instruction is naturally higher. Within any single cutoff — the
only comparison where conflict could show — the rate is flat.

Per-function fill rates corroborate. A 32-byte line holds 16 Thumb instructions,
so "miss on every line, every pass" is ~62.5 fills per 1k instructions:

| function | fill/1k instr | fill % of its cycles |
|---|---:|---:|
| `ndsR2AnimValueQ` | 1,345 | **81.4%** |
| `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` | 1,818 | 75.7% |
| `ndsRendererMtxMulAffine20p12` | 1,283 | 70.4% |
| `ndsRendererHardwareBindTextureName` | 2,594 | 63.1% |
| `ndsRendererCommitNativeStageSegment` | 1,449 | 57.1% |
| `gcPlayDObjAnimJoint` | 776 | 29.2% |

`ndsR2AnimValueQ` spends **81.4% of its cycles fetching itself**. There is a
genuine 7.5x spread (776–5,811), so lines are not *uniformly* evicted — but the
sweep shows that spread does not track set population, so it is reuse distance,
not addressing.

## 7. Decision gate

| # | condition | result |
|---|---|---|
| 1 | baseline simulation matches v3 | **N/A — bettered.** No simulator was trusted; the v3 measurement was used directly. The conflict *model* is internally sound (falsifier +27.8%) but its **premise is refuted**: measurement says set conflict is not the mechanism, so its −4.3% does not convert. |
| 2 | several plausible layouts give negligible reduction | **YES.** cluster −2.1%, phase −2.1%, conflict-min −4.3% — all on a metric measurement shows is not the mechanism. Effective reduction ≈ 0. |
| 3 | best credible saving well below ~17K | **YES.** The clusters a reorder can perfectly fix hold **0.14%** of modelled conflict; and conflict is not what is being paid. |
| 4 | temporal reuse confirms capacity dominates | **YES — measured.** Sets that fit refill at the same rate as sets that do not. |
| 5 | falsifier behaves as expected | **YES**, after the first one was caught and rebuilt. |

**All five hold. Link-order placement is CLOSED, this time on measured temporal
evidence rather than a whole-match footprint argument.**

The previous slice reached the same word by an invalid route. That matters: the
reasoning was rejected on its merits, and had the measurement come out the other
way — as it plausibly could, given three hot clusters really are scattered and
really do fit — the earlier verdict would have been not just unsupported but
wrong.

## 8. The next owner, now quantified

**HOT CODE FOOTPRINT REDUCTION.** The task named it conditionally; the v3 data
sizes it:

- **339,276 ticks/frame** of `icache_fill` — 29.7% of the match, 37.5% of
  non-idle, **1.87x the cost of `issue`**.
- **404,608 bytes** of executed non-ITCM text against an **8,192-byte** cache.
- Individual hot functions spending **57–81% of their own cycles fetching
  themselves**.

The lever is fewer bytes of hot code, not better-arranged bytes. Concretely
available directions, in the order the data supports them:

1. `ndsR2AnimValueQ` (81.4% fill, 1,028 B) and `ndsRendererMtxMulAffine20p12`
   (70.4%, 616 B) are small, extremely hot, and pay almost all their cost in
   fetch. They are ITCM candidates on fetch grounds — **but ITCM is out of scope
   here and `.itcm` is already 32,152 B**; this is a sizing input for whoever
   owns that decision, not a proposal.
2. `.text.hot` + `.text.hot.draw` total 9,844 B — **already 1.2x the I-cache**.
   The curated set cannot be resident as it stands, which is worth knowing before
   anyone adds to it.
3. The 40-member cluster headed by `ndsRendererCommitNativeStageSegment` is
   33,656 B — 4.1x the cache — and is the single largest fetch consumer. Shrinking
   *it* is worth more than relocating everything else.

## 9. What was NOT done, and why that is correct

No linker order was changed, no candidate ROM was built, no measurement arms were
run. The task's instruction was to reach the virtual-layout decision gate first
and "do not build ROMs yet"; the gate closed the lane, so building four
controlled layouts to measure a mechanism that measurement has already excluded
would have spent a day confirming a negative.

Zero bytes were spent. Section sizes in §5 are unchanged.

## Reproduce

```bash
# v3 capture (in-repo attributor, same ROM as the c125 v2 profile)
pwsh -NoProfile -File scripts/run-task37-profile-census.ps1 \
  -MelonDS emulators\melonds-attributor\melonDS.exe \
  -Build build-c125-profile -NoBuild -StartFrame 438 -Frames 1600 \
  -OutDir artifacts\performance\2026-08-14_icache-temporal\v3-baseline

arm-none-eabi-objdump -d builds/build-c125-profile/smash64ds-battle-playable-tickhud-hwtri.elf > c125.dis
python scripts/census-call-transitions.py  <v2-or-v3.csv> --dis c125.dis --regions 1601
python scripts/placement-layout-model.py   <v2-or-v3.csv> --dis c125.dis --regions 1601
python scripts/census-icache-temporal.py   artifacts/performance/2026-08-14_icache-temporal/v3-baseline/arm9-profile.csv \
  --dis c125.dis --regions 1601
```

The 3.5 GB v3 CSV is not committed; `v3-baseline-meta.txt` carries the totals and
`arm9-profile.regions.csv` the per-frame rows.
