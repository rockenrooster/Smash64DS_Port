# Yes, the card is read during the match — seven times, and all seven land in the twelve most expensive frames. The instrument also inflates five frames by exactly 2^22.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **HEAD `c4862724704`**
**0 builds, 0 source changes, 3 whole-match runs on an existing ROM, no default flipped,
no ROM published, both root ROMs byte-unchanged.**
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states
its window.

```text
BASIS       builds/build-c219-animitcm-ship, the tick-HUD instrument, gate arm
            NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1,
            mode 163 one-minute match, 1,600 ring samples, frames 439-2038,
            DLDI ON, slips=0.  Published rank-80 1,216,896 raw / 1,191,949 net
            against the 1,120,380 gate -> REQUIREMENT +71,569.

ANSWER A    THE CARD IS READ SEVEN TIMES INSIDE THE MATCH WINDOW.
            gNdsRelocAssetPayloadReadCount 101 -> 108, HeaderReadCount
            1,593 -> 1,600, ShortReadCount 0.  Each read is one animation-cache
            MISS (Misses 3 -> 10, Fills 3 -> 10).  Cycle 105's mechanism is GONE:
            arena Overflows 0, Rejects 0, UsedBytes 415,984 of ReservedBytes
            451,776, WarmFailed 0.  The reads are on frames
            456, 830, 1015, 1186, 1625, 1655, 1886 -- ranks 4, 1, 7, 10, 12, 8, 9
            of 1,600.  Every single one is a top-twelve frame.
            Deleting all seven is worth 12,736 at rank-80.  Section 1.

ANSWER A2   THE CAMERA CYCLE'S RANK-10/20 ATTRIBUTION IS WRONG AS STATED, and
            not because there are no card reads.  It is wrong twice: the
            instrument is bit-deterministic (three separate emulator sessions on
            one ROM produced byte-identical 1,600-row CSVs this cycle), and
            ranks 10 and 20 are NOT THE SAME FRAME in the two arms -- rank-10 is
            control frame 1655 against candidate frame 1975.  The +30,336 and
            +20,224 are rank-permutation artifacts of a rank-by-rank estimator
            applied to a tail that permutes 11 of its top 20.  Section 3.

FOUND       AN INSTRUMENT DEFECT WORTH UP TO 6,592 AT RANK-80, IN 9 OF 13 RECENT
            WHOLE-MATCH RUNS.  cpuGetTiming() intermittently reports a span
            exactly 4,194,304 = 2^22 ticks too large.  Proven two ways: the
            residual lands within +-260 of the run's own ALL median on four of
            five frames, and the inflated ALL is not a whole number of VBlanks
            while every clean ALL is.  Corrected, this basis reads rank-80
            1,210,304 raw / 1,185,357 net -- REQUIREMENT +64,977, not +71,569.
            Section 2.

ANSWER B    THE FORCE-LOAD PATH IS LIVE AND IT IS THE LARGEST IDENTIFIED ITEM ON
            THE BOARD, but the ~228,600 per-hit price is REFUTED as a price.
            134 force-loads on 116 frames.  WORK-H mean lift +193,677 (1 load),
            +179,545 (2), +274,542 (3) -- the implied per-load figure falls
            193,677 -> 89,773 -> 91,514, so it is not a fixed per-event cost.
            The owner is SITR, which vindicates cycle 105's SINT attribution.
            Deleting the premium on the 109 non-I/O force-load frames is worth
            52,736 at rank-80; 77,056 if those frames become quiet frames.
            Section 4.

ANSWER C    THE OVER-GATE SET IS FIVE POPULATIONS, NOT ONE, and two independent
            methods agree on the split.  SHDT 33 / SITR 27 / SPHD 8 / SPRM 7 /
            MISC 5.  They are disjoint: SHDT frames carry 14,112 of SITR excess
            and SITR frames carry 2,176 of SHDT excess.  SHDT -- live hitbox hit
            detection -- has a run P50 of 4,608 and a mean of 14,544, so every
            lane sizing done at the median or the mean has missed it; it owns 41%
            of the over-gate frames and 88% of the remaining gap.  Section 5.

NOT DONE    No fix built, no counter added, no default flipped, no gate rebanked
            by me -- section 6 says exactly what a rebank would require.
```

---

## 1. Item A — the card is read seven times, and every read is a top-twelve frame

Three whole-match runs on `builds/build-c219-animitcm-ship`, the ROM the basis
number was measured on, with **no rebuild**: the basis run (already on disk), a
`-RingStopStride 96` run and a `-RingStopStride 8` run, both carrying
`-PerStopGlobals`.

**The determinism control is free and it passes.** All three runs emit
**byte-identical 1,600-row CSVs** — `io-rows.csv` and `io8-rows.csv` compare equal
to `../2026-08-16_anim-itcm/ship-rows.csv` field for field. Adding 183 extra GDB
stops changed nothing, so the counters are joinable to the basis analysis and the
"cost is not reproducible between two emulator sessions" premise is dead on
arrival.

| counter | frames 438 → 2038 | reading |
|---|---|---|
| `gNdsRelocAssetPayloadReadCount` | 101 → **108** | **seven card payload reads** |
| `gNdsRelocAssetHeaderReadCount` | 1,593 → **1,600** | one header per payload |
| `gNdsRelocAssetShortReadCount` | 0 → 0 | no truncated read |
| `gNdsR2AnimCacheMisses` / `Fills` | 3 → 10 / 3 → 10 | **each read is one cold miss** |
| `gNdsR2AnimCacheHits` | 18 → 145 | 127 served from RAM |
| `gNdsR2AnimCacheArenaOverflows` / `Rejects` | 0 → 0 / 0 → 0 | **cycle 105's mechanism is gone** |
| `gNdsR2AnimCacheArenaUsedBytes` | 399,936 → 415,984 (of 451,776 reserved) | arena is not full |
| `gNdsR2AnimWarmLoaded` / `WarmFailed` | 43 → 43 / 0 → 0 | warm list quiet in-match |
| `gNdsR204AnimForceLoadTotal` | 21 → 155 | **134 force-loads** |
| `gNdsBattlePackHits` / `Misses` | 42 → 257 / 21 → 155 | **215 acquisitions never reach the load path** |

`ForceLoadTotal` and `BattlePackMisses` move by the same +134 — the counter is
incremented immediately after `gNdsBattlePackMisses++`
(`reloc_backend_assets.c:7620,7664`), so their equality is a structural check that
both were read correctly. Of the **349** in-match animation acquisitions, **215
(61.6%) are served by the battle pack**, 127 by the RAM animation cache, and **7 by
the card**.

**Cycle 105's finding is closed, not merely improved.** It measured
`Overflows 142 / Rejects 142` with the arena at its ceiling and a refused asset
re-reading off the card on every later use; `+111` payload reads across the window.
This tree reads `Overflows 0 / Rejects 0` and **+7**.

### 1.1 Which frames, and what they cost

The `-RingStopStride 8` run puts each read in an 8-frame window; the whole-match
per-frame run (`pf-rows.csv`) names the frame exactly. Both agree.

| ring frame | `WORK-H` | rank of 1,600 | `SITR` on that frame |
|---:|---:|---:|---:|
| 456 | 1,748,224 | **4** | 69,568 |
| 830 | 1,813,184 | **1** | 895,104 |
| 1015 | 1,706,240 | **7** | 831,488 |
| 1186 | 1,571,584 | **10** | 822,912 |
| 1625 | 1,703,552 | **8** | 860,480 |
| 1655 | 1,549,440 | **12** | 710,720 |
| 1886 | 1,622,400 | **9** | 707,712 |

*(ranks on the artifact-corrected series of section 2; `SITR` run median 102,944)*

**Seven reads, seven top-twelve frames.** The probability of that under
independence is negligible, but it is still an association, so it is priced
against a **matched** control rather than against the run median: a frame that did
a force-load and **no** I/O.

| population | n | `WORK-H` P50 | `WORK-H` mean |
|---|---:|---:|---:|
| no force-load | 1,484 | 927,328 | 942,183 |
| force-load, no card read | 109 | 1,109,824 | 1,136,530 |
| **force-load + card read** | **7** | **1,703,552** | **1,673,518** |

**Card-read premium over a plain force-load: +593,728 median / +536,988 mean**, of
which **+490,476 is `SITR`**. That is the honest price of one in-match DS card read
on this tree, measured against a population that does everything else the same.

**What the seven are worth at the gate: 12,736.** Exact re-rank of the basis's own
1,600 rows — set the seven frames to a plain force-load frame's cost and re-sort:
rank-80 1,210,304 → **1,197,568**, gap **+64,977 → +52,241**. (Setting them to a
quiet frame's cost gives the identical answer, because they sit far above rank-80
either way; what moves rank-80 is that seven frames leave the top.) `POSITION.md`
carries this lane as *"In-match asset I/O — delete all seven load frames | 9,863"*;
the count is exactly right and the price on the corrected basis is **12,736**.

---

## 2. Found in passing: `cpuGetTiming()` reports a span exactly 2^22 too large

Five frames of the basis run read `WORK-H` above 5,000,000 against a run median of
933,696. They are **not** work.

**Proof 1 — the residual is the median.** Subtract 4,194,304 from every bucket of
those frames that exceeds it:

| frame | `ALL` | `ALL` − 2^22 | run P50 `ALL` | error |
|---:|---:|---:|---:|---:|
| 1056 | 5,311,744 | 1,117,440 | 1,117,632 | **−192** |
| 1357 | 5,312,192 | 1,117,888 | 1,117,632 | **+256** |
| 1750 | 5,312,128 | 1,117,824 | 1,117,632 | **+192** |
| 1961 | 5,312,064 | 1,117,760 | 1,117,632 | **+128** |
| 1629 | 5,872,576 | 1,678,272 | (HUD-refresh frame, `HUD` 414,400) | — |

A real 0.125 s stall cannot leave the remainder within 0.02% of the median four
times.

**Proof 2 — the inflated value is not a whole number of VBlanks.** `ALL` is
VBlank-quantised: every clean value in this run is a near-exact multiple of
≈558,816 (the observed set is 1,117,632 / 1,678,272 / 2,238,x / 3,358,080 — 2, 3, 4
and 6 VBlanks). 5,311,744 is **9.505** VBlanks. The corrected value is exactly 2.

**Proof 3 — it lands wherever the program counter is, in whichever span is open.**
Frames 1056/1629/1961 carry it in `FTR`, frame 1357 in `SCPU` (and therefore in its
parents `SINT`→`GCRA`→`SRC`), frame 1750 in `SPHD`. Nothing else on those frames
moves.

**The mechanism, from the linked image rather than from a header.**
`cpuGetTiming` (`0x020be710`) is
`((u32)(tickGetCount() - start)) << 6` — which is also where the campaign's 64-tick
sampling granularity comes from. `tickGetCount` (`0x020bfc2c`) assembles a 64-bit
count as `(software_overflow_counter << 16) | TIMER_DATA`, and corrects for an
un-serviced overflow with the heuristic
`((timer ^ 0x8000) >> 15) & (IF >> 5)` — i.e. it credits **one** pending overflow,
and only while the timer's top bit is clear. When that correction fails on a span's
**start** read the span reads exactly one overflow too long: 2^16 tick units ×
`<< 6` = **2^22 = 4,194,304**. The exact trigger (how the tick IRQ comes to be
deferred that long) is **not** proven here.

**It is in 9 of the last 13 whole-match runs and it moves rank-80.**

| run | contaminated frames | rank-80 raw | corrected | delta |
|---|---|---:|---:|---:|
| `c206-shipgx0` | 1054, 1119 | 1,239,808 | 1,237,056 | −2,752 |
| `c207-gx1` | 812, 1587, 1703 | 1,232,768 | 1,228,352 | −4,416 |
| `c201-route0` / `route1` | none / none | 1,220,800 / 1,219,392 | same | 0 |
| `c202-route0` / `route1` | 2 / 2 | 1,233,664 / 1,233,792 | 1,230,976 / 1,227,200 | −2,688 / −6,592 |
| `c216-t0` / `t1` | none / none | 1,216,064 / 1,222,976 | same | 0 |
| `c217-tilesync` | 1710 | 1,218,752 | 1,217,216 | −1,536 |
| `c218-a0` / `a1` | 3 / 2 | 1,231,616 / 1,217,408 | 1,226,816 / 1,213,184 | −4,800 / −4,224 |
| `c215-hwmath-ship` | none | 1,227,392 | same | 0 |
| **`c219-animitcm-ship` (basis)** | **1056, 1357, 1629, 1750, 1961** | **1,216,896** | **1,210,304** | **−6,592** |

Two consequences, and the second is the serious one:

1. **The current requirement is +64,977, not +71,569** (rank-80 1,210,304 raw /
   1,185,357 net). Every figure in sections 1, 4 and 5 is quoted on the corrected
   series.
2. **It injects 0–6,592 of build-dependent noise into every cross-build rank-80
   comparison**, on top of the ≥14,080 placement floor, and it is *not* symmetric —
   it can only inflate. A cross-build pair where one arm caught three of these and
   the other caught none is 4,800 apart before any code difference is considered.

The filter is one line — `ALL >= (1 << 22)` detects it completely, because `ALL`
contains every span — and it belongs in the sampler, not in each analysis. Not
done this cycle (no source change was in scope); recorded as an item in section 6.

---

## 3. Item A, second half — what actually owns the camera pair's ranks 10 and 20

`../2026-08-16_camera-fixedpoint/CAMERA_Q20_12.md` §3.2 states that ranks 10 and 20
read **+30,336** and **+20,224** because they *"are cartridge-read frames whose cost
is not reproducible between two emulator sessions"*. Both halves fail.

**The instrument is bit-deterministic.** Three separate emulator sessions on one
ROM this cycle produced byte-identical 1,600-row CSVs, extreme frames included.
Within a fixed binary and invocation, a card read's cost reproduces exactly. (The
camera pair's two arms are also not "two sessions of the same thing" — they are one
binary with a poked route word, which changes the work.)

**Ranks 10 and 20 are different frames in the two arms.** Rank-by-rank
differencing subtracts *the 10th-largest of one run* from *the 10th-largest of the
other*; when the tail permutes, the result is not a frame's cost.

| | control | candidate | "delta at rank" |
|---|---|---|---:|
| rank 10 | frame **1655**, 1,611,968 | frame **1975**, 1,642,304 | +30,336 |
| rank 20 | frame **1937**, 1,457,216 | frame **1447**, 1,477,440 | +20,224 |

The two arms' top-20 **sets** overlap 19 of 20, but only **9 of 20** sit at the same
rank. Paired on the same frame, the four frames involved read:

| frame | control | candidate | paired |
|---:|---:|---:|---:|
| 1655 | 1,611,968 | 1,606,464 | **−5,504** (the level cut) |
| 1937 | 1,457,216 | 1,449,344 | **−7,872** (the level cut) |
| 1975 | 1,489,856 | 1,642,304 | **+152,448** |
| 1447 | 1,348,160 | 1,477,440 | **+129,280** |

**So the answer to "what owns ranks 10 and 20" is: nothing does.** The two numbers
are estimator artifacts. What *is* real, and what the camera document reached for
and mis-named, is that the load-frame population moves by ±100,000–150,000 between
the two arms while the median frame moves −4,768: paired deltas of **−124,160
(830), −142,016 (1500), −103,360 (747), +152,448 (1975), +129,280 (1447)** against
a whole-run paired median of −4,768. That is a real, reproducible, arm-dependent
effect on exactly the frames section 1 shows are card-read and force-load frames,
and **this cycle does not explain it** — it is stated as an open observation, not a
mechanism. Neither arm carried the 2^22 artifact, so that is not the cause.

**The camera document's conclusion survives; only its reason does not.** Rank-80's
−1,408 really is not the result, and the paired per-frame median really is the
right estimator — because rank-by-rank fails on a permuting tail, not because of
session noise. **The banked −4,736 is untouched.**

---

## 4. Item B — the force-load path, priced against the falsifier

**The per-frame instrument had to be validated first, and it failed.**
`-PerFrameGlobals` emits a **torn** row: `ALL` comes from iteration *f* and every
other bucket from iteration *f+1*, so **1,526 of its 1,600 rows violate
`ALL == WORK + WAIT`**, an identity the ring path satisfies on all 1,600. Measured,
not assumed: joining its counters to the ring buckets gives a `WORK-H` association
of **+212,620** at offset +1 against **−6,751** at offset 0 and **−882** at offset
−1. Every number below therefore uses **ring buckets** with **per-frame counters at
offset +1**. *(An earlier pass of this analysis using the torn rows directly named
`GCRARES` as the owner. That is retracted — it was the misalignment.)*

**134 force-loads on 116 of 1,600 frames.** The falsifier the brief asked for is
"more of the cause":

| force-loads on the frame | n | `WORK-H` mean | lift | **implied per-load** | `SITR` | `SCPU` | `SPHD` | `SHDT` | `SPRM` |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 1,484 | 942,183 | — | — | 106,599 | 48,751 | 69,595 | 13,059 | 2,774 |
| 1 | 94 | 1,135,860 | +193,677 | **+193,677** | 194,404 | 40,688 | 114,458 | 39,748 | 34,018 |
| 2 | 12 | 1,121,728 | +179,545 | **+89,773** | 299,685 | 37,179 | 63,285 | 5,509 | 2,203 |
| 3 | 3 | 1,216,725 | +274,542 | **+91,514** | 312,171 | 31,659 | 149,397 | 4,459 | 2,048 |
| (+ card read) | 7 | 1,673,518 | +731,335 | — | 699,712 | 31,771 | 67,886 | 10,688 | 83,255 |

**Read the third-from-last column.** A fixed per-event price would be flat; it falls
**2.1×** from one load to two. **The "a cache hit costs 117,000–570,000, ~228,600
mean" reading is refuted as a price** — cycle 105 saw 5 frames of 30 and could not
see the shape. What is real is that a frame carrying a force-load costs about
**+193,677**, and most of that is the frame's own event, not the load: on the
single-load frames `SPHD` is +44,863, `SHDT` +26,689 and `SPRM` +31,244 above
baseline, and none of those scale with the load count either. `SCPU` moves the
other way (−8,063 / −11,572 / −17,092) — the AI does less on a frame where the
state already changed.

**The count-linear part is `SITR`**, and it is the only column that behaves like a
per-event cost: +87,805 / +193,086 / +205,572 for one, two and three loads, i.e.
**~68,500–96,500 per force-load**. `SITR` is `SINT` minus the AI, so **cycle 105's
`SINT` attribution stands** — corrected only in magnitude, and now with a falsifier
behind it.

**Is it live? Yes, and it is the largest identified item on the board.** Exact
re-rank of the basis's own 1,600 rows:

| intervention | rank-80 | moved | gap |
|---|---:|---:|---:|
| (control) | 1,210,304 | 0 | +64,977 |
| the seven card reads removed | 1,197,568 | 12,736 | +52,241 |
| the force-load premium removed from all 109 non-I/O force-load frames | 1,157,568 | **52,736** | +12,241 |
| every force-load frame reduced to a quiet frame | 1,133,248 | **77,056** | **−12,079** |
| all `SITR` above its own median deleted, every frame | 1,124,448 | 85,856 | −20,879 |

The last two rows are **ceilings from an exact re-rank, not implementations**. But
the middle row is the honest size of the lane: **52,736 = 81% of the remaining
+64,977**, from 109 frames.

---

## 5. Item C — the over-gate set is five populations, and two methods agree

Population: the 80 largest `WORK-H` frames of the corrected basis series. Each
frame is decomposed into a **disjoint leaf set** whose closure is asserted exact —
`leafsum == WORK-H` on all 1,600 rows, no clamping — using the tree read from the
accumulation sites, not assumed:

```
WORK-H = FTR + STG + BG + AUD + MISC + (OTHR-WAIT) + SRC
SRC    = GCRA + SWRM + SRCRES
GCRA   = SINT + SPHD + SPHC + SCAT + SHDT + SPRM + GCRARES
SINT   = SCPU + SITR                (reloc_backend_diagnostic_recorders.c:6001)
```

**Method 1**, label each frame by which leaf carries the most excess over the run's
own median for that leaf. **Method 2**, Ward clustering on the normalised
excess-share vectors, k chosen by silhouette. They agree:

| cluster | method 1 | method 2 | median own excess | what it is |
|---|---:|---:|---:|---|
| **`SHDT`** | 33 | 34 | 259,776 | live hitbox hit detection (`ftMainProcSearchHitAll`) |
| **`SITR`** | 27 | 27 | 231,264 | fighter interrupt/state proc, AI excluded |
| **`SPHD`** | 8 | 7 | 215,136 | `ftMainProcPhysicsMapDefault` |
| **`SPRM`** | 7 | 7 | 298,496 | params/animation-event interpreter — the "all at once" frames |
| **`MISC`** | 5 | 5 | 215,680 | frames 450–453, 455 (window start) |

Ward's best k is **5** (silhouette 0.458; k=3 is 0.456 and k=4 is 0.427).

**They are genuinely disjoint, not one heavy-frame population wearing five hats.**
The conditional profile — the median excess of *every* leaf on each cluster's own
frames — shows the other mechanisms sitting at baseline:

| owner | n | `SHDT` | `SITR` | `SPHD` | `SPRM` | `SCPU` | `GCRARES` | `MISC` | `FTR` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `SHDT` | 33 | **259,776** | 14,112 | 26,816 | 128 | 38,560 | 16,288 | 2,112 | 0 |
| `SITR` | 27 | 2,176 | **231,264** | 24,000 | 64 | 7,904 | 16,352 | 23,744 | 0 |
| `SPHD` | 8 | 3,008 | 22,368 | **215,136** | 32 | 8,064 | 1,264 | 15,136 | 0 |
| `SPRM` | 7 | 202,368 | 95,584 | 9,600 | **298,496** | 0 | 38,944 | 32,576 | 1,088 |
| `MISC` | 5 | 160,448 | 992 | 0 | 64 | 0 | 25,440 | **215,680** | 0 |
| *run P50* | 1,600 | *4,608* | *102,944* | *72,832* | *2,112* | *53,600* | *81,888* | *108,800* | *294,208* |

`SHDT` frames carry **14,112** of `SITR` excess against a `SITR` median of 102,944;
`SITR` frames carry **2,176** of `SHDT` excess against a `SHDT` median of 4,608.
Only the 7-frame `SPRM` cluster is genuinely combined, and it is the 1.7M
population.

**94.8% of the median over-gate frame's excess is inside `SRC`** (percentiles
0/5/25/50/75/95/100 = 43.5 / 53.8 / 87.4 / 94.8 / 97.4 / 99.3 / 100.0). Only three
frames — 450, 451, 452 — are majority outside the simulation.

**What each is worth, by exact re-rank:**

| delete all excess on… | rank-80 | moved | gap |
|---|---:|---:|---:|
| the `SHDT` cluster (33 frames) | 1,153,152 | **57,152** | +7,825 |
| the `SITR` cluster (27 frames) | 1,159,104 | **51,200** | +13,777 |
| the `SPHD` cluster (8 frames) | 1,195,840 | 14,464 | +50,513 |
| the `SPRM` cluster (7 frames) | 1,197,568 | 12,736 | +52,241 |
| the `MISC` cluster (5 frames) | 1,204,288 | 6,016 | +58,961 |
| all 80 | 1,112,320 | 97,984 | −33,007 |

### 5.1 The consequence the brief predicted, with the number attached

**`SHDT` — live hitbox hit detection — has a run P50 of 4,608 and a mean of
14,544.** On its own 33 over-gate frames it is **+259,776**, a **56×** concentration.
Any lane sizing done from a mean or a P50 self-time reads `SHDT` as noise; measured
where the percentile lives it owns **41% of the over-gate frames** and **88% of the
remaining +64,977**. This is `mean-self-time-predicts-p50-not-p95` and
`cluster-where-the-percentile-lives` in a new place, and it is the concrete answer
to "does averaging over the marginal-80 blur the lane estimates": **yes, and the
worst-blurred lane is the one with the smallest median.**

### 5.2 Where Items B and C meet

Cross-tabulating the clusters against the force-load counter reconciles the two:

| cluster | n | frames carrying a force-load | frames carrying a card read |
|---|---:|---:|---:|
| `SHDT` | 33 | 7 | 0 |
| `SITR` | 27 | **16** | **6** |
| `SPHD` | 8 | 5 | 0 |
| `SPRM` | 7 | **7** | **1** |
| `MISC` | 5 | 0 | 0 |
| **total** | 80 | **35** | 7 |

Base rate is 116/1,600 = **7.2%**, so the over-gate set is enriched **6.0×** in
force-loads — but the enrichment is entirely in the `SITR` and `SPRM` clusters
(23 of 34 frames), and the largest cluster, `SHDT`, is **not** a force-load
population. **They are two different levers and neither subsumes the other.**

---

## 6. What this changes, and what the next cycle inherits

1. **The requirement is +64,977 on this basis, not +71,569.** The 6,592 difference
   is instrument, not work. Nothing was rebanked by me; the board should carry both
   numbers until the sampler filters the artifact, because every prior figure in the
   campaign was computed without it.
2. **The sampler should reject or flag `ALL >= (1 << 22)`.** One condition,
   complete by construction, in `scripts/sample-tick-hud-buckets.ps1` where the ring
   is stitched. It removes 0–6,592 of asymmetric build-dependent noise from every
   cross-build comparison. Not built here.
3. **`-PerFrameGlobals` emits a torn row and must not be used for bucket values.**
   1,526 of 1,600 rows fail `ALL == WORK + WAIT`. Its counters are sound at ring
   offset +1. Cycle 105's spike probe used this path, which is a second reason its
   per-frame excess figures should not be quoted as prices.
4. **The two largest remaining levers are named and sized**: the force-load premium
   (52,736 at rank-80, 109 frames, owner `SITR`) and the `SHDT` cluster (57,152,
   33 frames). They overlap in 7 frames only.
5. **Open and unexplained**: why the load-frame population moves ±100,000–150,000
   between two arms of the same binary (section 3). It is not the 2^22 artifact and
   it is not session noise.

## 7. What was NOT done

- **No build, no source change, no default flipped, no ROM published.** Root ROMs
  byte-unchanged before and after: `smash64ds.nds` `af968925…`,
  `smash64ds-battle-playable-hwtri.nds` `d5f37f22…`.
- **No fix for the 2^22 artifact and no filter added to the sampler** — it is a
  harness edit and this was a diagnostic cycle.
- **The 2^22 trigger condition is not proven**, only the magnitude, the site and the
  arithmetic that produces exactly 2^22.
- **Nothing was rebanked.** The corrected +64,977 is stated, not adopted; adopting it
  means re-quoting the ratio in every ranking document, which is a documentation
  change this cycle did not make.
- **No per-PC profile of the force-load path** — section 4 prices it and names its
  bucket, but does not name instructions. Cycle 106's 25.1% attribution is still the
  only instruction-level account.
- `build-c205-camtoggle` was not rebuilt.

## 8. Reproduction

```powershell
# no build: the ROM is already at builds/build-c219-animitcm-ship
pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c219-animitcm-ship -NoBuild `
    -RingDump -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 `
    -PerStopGlobals gNdsRelocAssetPayloadReadCount,gNdsRelocAssetHeaderReadCount,`
gNdsRelocAssetShortReadCount,gNdsR204AnimForceLoadTotal,gNdsR204AnimForceLoadDistinct,`
gNdsR204AnimForceLoadRepeat,gNdsR2AnimCacheHits,gNdsR2AnimCacheMisses,gNdsR2AnimCacheRejects,`
gNdsR2AnimCacheFills,gNdsR2AnimCacheArenaOverflows,gNdsR2AnimCacheArenaUsedBytes,`
gNdsR2AnimCacheArenaReservedBytes,gNdsR2AnimWarmLoaded,gNdsR2AnimWarmFailed `
    -RowsCsv ...\io-rows.csv -JsonOut ...\io.json
# the same with -RingStopStride 8 -> io8.json (8-frame localisation)
# and the per-frame counter run (buckets torn, counters sound at offset +1):
pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c219-animitcm-ship -NoBuild `
    -Samples 1600 -StartFrame 438 -TimeoutSeconds 7200 -AllowRepeatedFrames `
    -PerFrameGlobals gNdsR204AnimForceLoadTotal,gNdsRelocAssetPayloadReadCount,`
gNdsBattlePackHits,gNdsBattlePackMisses,gNdsR2AnimCacheHits `
    -RowsCsv ...\pf-rows.csv -JsonOut ...\pf.json
```

Then, from the repo root, `python artifacts/performance/2026-08-16_match-io-audit/`
`{counters,forceload,overgate,cluster}.py` — all four are pure arithmetic over CSVs
already on disk and need no emulator. `evidence.txt` is their combined output.

**Run these from a normal PowerShell session, not from a Git-Bash-spawned shell.**
`verify-all.ps1`'s toolchain pre-flight throws *"Recursive make is unusable … 
`NDS_RECURSIVE_MAKE=FAIL:127`"* under Git Bash's environment and passes with
`NDS_RECURSIVE_MAKE=OK` under PowerShell; the message blames the toolchain, and the
toolchain is fine.
