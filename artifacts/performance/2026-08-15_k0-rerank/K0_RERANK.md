# Phase 7 is measured, the 5-minute match is green, and the re-rank says slice 2 is not the lever

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `7309ce785ad`**
**Native Battle Kernel slice 1 — close acceptance items 3 and 4, prove phase 7, re-rank the gate.**
Predecessor: `…/2026-08-15_pacing-publication/PACING_PUBLICATION.md`.

One build (`build-c171-k0-5min-bp1`), one 5-minute measurement run, one Boundary run.
Everything else is re-analysis of committed rows.

---

## 0. Outcome first

```text
PHASE 7   MEASURED, AND SIX OF THE SEVEN K0 LINES READ EXACTLY ZERO.
          999 after-GO acquisitions for the packed fighter (Fox) over a
          FIVE-minute match, against a control -- the un-packed fighter
          (Mario), same run, same frames -- that reads non-zero on every one
          of them.  The seventh line SPLITS: the asset->path half is 0
          (deleted), the token->asset-id half is 999 (NOT deleted, because it
          runs UPSTREAM of the pack dispatch).  That is the honest result and
          it names a real residue in the architecture.

ITEM 4    THE FIVE-MINUTE MATCH RAN AND THE GUEST SAYS SO.
          gSCManagerTransferBattleState.time_limit = 5 read out of the guest,
          gNdsBattlePlayablePacingLogicFrames 17,772 of 18,000 = 98.7%
          coverage, 8,886 presented frames, slips 0, every allocator gate 0,
          heap low-water 51,876 against the 32,768 floor.  WORK-H P95
          1,198,720 against the 1-minute arm's 1,177,920: +20,800 across a
          5x longer match.  LENGTH STILL DOES NOT ACCUMULATE COST.

ITEM 3    THE PRODUCT'S 4.1 CADENCE POINTS ARE NAMED, AND THEY ARE TWO THINGS.
          On the DRAW=0 arm the deficit is 70 frames.  55 of them are WORK-H
          bound (mean overrun 36,982 over the cadence boundary) and their
          excess is 81.2% inside gcRunAll -- the same lane as the P95 gap.
          The other 15 are NOT: their WORK-H is under the boundary and 13 of
          them carry a burst in the HUD bucket.  On a DRAW=0 arm that bucket
          is THE GAME'S OWN BATTLE-INTERFACE OAM DRAW, it fires 1.41 times a
          second at a mean of 100,853 ticks, those frames drop at 47% against
          an 8% base rate -- AND `WORK-H = WORK - HUD` BY CONSTRUCTION, so the
          gate metric cannot see it.

RE-RANK   SLICE 2 IS NOT THE LARGEST OWNER AND CANNOT CLOSE +32,593.
          Its whole addressable lane -- the gcRunAll scheduler machinery -- is
          17,786 tk/fr on the frames that set P95, so a 100% deletion is 1.8x
          short of the requirement, and a flat vector cannot delete 100%.
          The bracket that isolates it (GCRA-REM) is 3.6-4.4% of the P95-set
          excess at 1.17-1.21x.  The largest owners are the fighter process
          BODIES slice 2 deliberately leaves untouched: SITR 31.6-36.0%,
          SHDT 20.5-27.0% at 16-17x presence, SPHD 11.3-15.2%, SPRM 8.3-10.2%
          at 20-23x -- and this now reproduces on TWO matches and two window
          sizes rather than one.

OUTLIER   THE UNEXPLAINED MULTI-MEGATICK FRAMES ARE CHARACTERISED.  Every one
          of them carries an excess of 2^22 ticks (4,194,304 = 0.1252 s) in
          whichever single bucket was open, +-0.5% in the tight cases, on six
          different arms and six different buckets, at about one per 2,100
          presented frames.  Not this change (c158, 2026-08-14, has one).  Not
          real work: named, priced, and handed forward with its discriminator.
```

---

## 1. Phase 7 — the after-GO zero-I/O assertion, measured

### 1.1 What was built, and why it is not inferred from `BattlePackHits`

`plan.md` §K0 lists seven quantities that must read zero once the battle is in GO.
Every one of them already had a whole-run counter and every one of them reads zero
on the pack-hit path **by construction** — the early return in
`ndsRelocForceLoadFighterAObj16File` (`src/port/reloc_backend_assets.c:7593`)
precedes all of them. `HANDOFF.md`'s own standing rule says what that is worth:

> *a zero one level DOWNSTREAM of a rejected request reads exactly like a dead
> lane.*

So each K0 line is now counted **at its own site**, keyed by the asset's fighter,
and gated on the battle actually being in GO. Ten `volatile u32[2]` arrays, index
`[0]` Mario / `[1]` Fox (`include/nds/nds_reloc_assets.h`):

| K0 line | counter | site |
|---|---|---|
| fighter-animation FAT reads | `gNdsK0AfterGoFatReads` | the payload `fread` success in **both** payload loaders, `nds_reloc_assets.c` |
| `get_fat` / `f_lseek` | `gNdsK0AfterGoSeeks` | the `fopen` (directory walk + chain seat) and the `fseek`, same file |
| animation payload byte-swaps | `gNdsK0AfterGoByteSwaps` | `ndsRelocApplyWordByteSwap` entry, and the warm loader's word swap |
| animation-file relocation / fixups | `gNdsK0AfterGoRelocs` | `ndsRelocFinalizeLoadedFile` entry (the whole fixup chain) |
| AObj16 file normalization | `gNdsK0AfterGoNormalizes` | `ndsRelocNormalizeFighterAObj16File`, **past** its two early returns |
| raw animation-file cache copies | `gNdsK0AfterGoCacheCopies` | the `memcpy(heap, cached->payload, …)` |
| token → file discovery | `gNdsK0AfterGoTokenResolves` | `ndsRelocAssetIDForToken` at the acquisition entry (token → asset id) |
| " | `gNdsK0AfterGoPathLookups` | `ndsRelocAssetFindEntry` (asset id → NitroFS path), which the whole family funnels through |
| denominator | `gNdsK0AfterGoAcquisitions` | `ndsRelocForceLoadFighterAObj16File` entry |
| denominator | `gNdsK0AfterGoPackHits` | the pack-hit return |

The GO gate is `gNdsK0BattleInGo`, published once per logic update from a read
`ndsR2HostBattleUpdateOnce` and the main taskman loop **already perform**
(`battle_status_before`). It is not a latch: a rematch and a Sudden Death entry
each run their own countdown, and the asset work their scene setup does is
legitimately pre-GO for that entry.

**The control is in the same run.** Only one fighter's clips fit the arena, so
`NDS_R2_BATTLEPACK=1` leaves the other fighter on the generic path. This is a
stronger control than a second binary: identical ELF, identical frames,
identical match.

### 1.2 The result

`build-c171-k0-5min-bp1`, `NDS_R2_BOTH_CPU=1` · `NDS_R2_BATTLEPACK=1` ·
`NDS_R2_BATTLEPACK_KEEP_CACHE=1` · `NDS_R2_SOAK_MATCH_MINUTES=5` ·
`NDS_TICK_HUD_DRAW=1`, DLDI **on**, arena 1,548,288, window frames 439–8886.

| K0 line | Mario `[0]` — control, generic path | **Fox `[1]` — packed** |
|---|---:|---:|
| acquisitions after GO | 812 | **999** |
| served by the pack | 0 | **999** |
| **FAT reads** | 21 | **0** |
| **seeks (`get_fat` / `f_lseek`)** | 42 | **0** |
| **payload byte-swaps** | 21 | **0** |
| **relocation / fixups** | 812 | **0** |
| **AObj16 normalizations** | 42 | **0** |
| **raw cache copies** | 791 | **0** |
| **token → asset-id resolves** | 812 | **999** ← *not deleted* |
| **asset → path lookups** | 833 | **0** |

**Six of the seven K0 lines are zero for the packed fighter across 999 after-GO
acquisitions, against a control that is non-zero on every one of them.**

Five independent arithmetic cross-checks, none of which was arranged:

```text
gNdsBattlePackHits            999 == PackHits[1] 999 == Acquisitions[1] 999
                                     -> EVERY Fox acquisition after GO took the
                                        pack.  Zero fall-through.
gNdsR2AnimCacheHits           791 == CacheCopies[0] 791
Seeks[0] 42                       == 2 x FatReads[0] 21   (one fopen + one fseek
                                        per generic load, exactly)
gNdsR2AnimCacheMisses/Fills    23 vs FatReads[0] 21        (2 fills before GO)
gNdsRelocAssetPayloadReadCount 121 whole-run vs 21 attributed to after-GO Mario
                                        -- the other 100 are pre-GO scene loads
                                        and non-animation assets, so nothing
                                        escaped the attribution downward.
gNdsBattlePackMisses          814 vs Acquisitions[0] 812   (2 Mario acquisitions
                                        before GO)
```

`Relocs[0]` 812 against `Normalizes[0]` 42 is not a discrepancy: every generic
acquisition runs the finalize chain, but only 42 of them actually walked the
AObj16 payload — the other 770 came from a prebaked cache entry that had already
claimed the transform (`entry->aobj16_ready`, `NDS_R2_AOBJ16_PREBAKE`).

**Staleness cannot manufacture this result.** These are whole-run totals read at
the single end-of-run stop, which is the shape the 2026-08-15 D-cache finding
explicitly excluded from exposure; and a stale read is *always behind, never
ahead*, so staleness could only shrink the control, never turn Fox's rows to zero.

### 1.3 The seventh line, and it is a real residue

`lbRelocGetForceExternHeapFile` (`reloc_backend_assets.c:7754`) computes
`asset_id = ndsRelocAssetIDForToken(token)` **before** it calls the acquisition
function, so the pack is never consulted until after the token has been resolved.
`ndsRelocAssetIDForToken` is the ~300-compare chain that §K-MECHANISM already
named as the miss-path cost driver; on the pack path it still runs **999 times in
five minutes**.

That is not a defect and it does not weaken the other six rows — but it is the
part of K0 line 7 that slice 1 did **not** delete, and it should be recorded as
such rather than rounded to "token → file discovery = 0". The fix is an ordering
change (consult the pack by token, or memoise the token → asset map), not an
architecture change, and it is not priced here.

---

## 2. Item 4 — the five-minute match

`sample-tick-hud-buckets.ps1 -Samples 8448 -StartFrame 438 -RingDump`, the same
instrument the 2026-08-13 acceptance battery used for this item
(`…/2026-08-13_c-stress/STRESS_GATE.md` §"Item 4"), on the shipping-candidate
configuration rebuilt with the soak match timer.

**The match length is read out of the guest, not assumed:**

```text
gSCManagerTransferBattleState.time_limit        5   (game minutes)
gNdsBattlePlayablePacingLogicFrames        17,772   of 18,000 = 98.7% coverage
presented frames                            8,886   window 439..8886
slips                                           0
```

| | 5-minute (this run) | 1-minute banked (`c170-seam-bp1`) | 5-minute 2026-08-13 (`c132-stress5`) |
|---|---:|---:|---:|
| `WORK-H` P50 | 946,944 | 940,320 | 929,344 |
| **`WORK-H` P95** | **1,198,720** | **1,177,920** | 1,205,760 |
| VBI 2 / 3 / 4 / 5+ | 7,418 / 1,402 / 55 / 11 | 1,745 / 272 / 13 / 8 | 7,415 / 1,394 / 58 / 19 |
| two-VBlank share | 83.5% | 85.6% | 83.4% |
| max interval | 19 | 19 | 26 |

**Length does not accumulate cost**, for the third independent time and now on the
pack arm: +20,800 P95 against a 1-minute arm across a different match and a
different binary, inside the ±14,080 cross-build floor plus the match difference.
The one length-dependent signal is the same one 2026-08-13 saw — the 3-VBlank
share rises with duration (15.8% here against 13.3% on the 1-minute arm; 16.6%
against 13.1% there).

**Allocator and heap, on the binary that produced the ticks:**

| counter | value | requirement |
|---|---:|---|
| `gNdsTaskmanArenaChosenSize` | 1,548,288 | == requested |
| `gNdsTaskmanArenaAllocFailCount` | **0** | == 0 |
| `gNdsR2AnimCacheArenaReserveFailCount` | **0** | == 0 |
| `gNdsR2AnimCacheRejects` | **0** | == 0 |
| `gNdsSyMallocOverflowCount` | **0** | == 0 |
| `gNdsBattlePackLoadFails` | **0** | == 0 |
| `gNdsBattlePackState` / `LoadSteps` | 1 (READY) / 18 | blob adopted |
| `gNdsTaskmanGeneralHeapFreeMin` | **51,876** | > 32,768 floor, > 25,600 GObj latch |
| `sGCCommonsMaxNum` | −1 | cap never fired |

Heap low-water 51,876 against the 1-minute chain battery's 52,472 and 52,864:
**five times the match, 596 bytes of movement.** `syTaskmanStartTask` rewinds the
heap per scene entry, so nothing accumulates.

**No freeze is the sampler's own evidence here**, not a separate soak: 8,886
presented frames were sampled to completion with `slips 0`, zero repeated-frame
warnings and zero ring-stop skew. A frozen ROM cannot present the next 8,447
frames. The 660 s chain arm of the battery (10 entries / 8 matches / 8 restarts /
2 Sudden Deaths, `NO-FREEZE`) already ran on this configuration last cycle and was
not re-run.

> **Deviation from the brief, stated.** The brief said "run the 5-minute-match
> soak". `soak-freeze-watch.ps1`'s `-MinutesToRun` ceiling is 12.0 and its own
> header measures one game minute at ~136 s of wall clock, so a 5-game-minute
> match needs ~11.3 minutes plus scene load and would have run out of ceiling
> mid-match. The 2026-08-13 acceptance battery used the tick-HUD sampler for
> exactly this item for exactly this reason, and that instrument additionally
> reads the guest's own match timer, the VBlank histogram, every allocator gate
> and — this cycle — the phase-7 counters, from one run.

---

## 3. Item 3 — what the product's 4.1 cadence points are

Read from the `NDS_TICK_HUD_DRAW=0` arm, which is what `plan.md` §1 item 3 gates
on (owner, 2026-08-14). Source rows are last cycle's `c170-seam-bp1-draw0`; the
three 2^22 instrument frames of §5 are excluded from the attribution.

```text
1,597 frames        two-VBlank 1,448 = 90.7%      need 1,518 for 95%
DEFICIT             70 frames
cadence boundary    1,113,152  (the highest WORK-H that still presented in 2 VBI)
```

**The 70 cheapest dropped frames split cleanly in two:**

| | frames | what makes them late |
|---|---:|---|
| `WORK-H` **over** the boundary | **55** | logic/draw cost. Mean overrun **36,982**. |
| `WORK-H` **at or under** it | **15** | not cost. **13 of the 15 carry a HUD-bucket burst > 50,000.** |

### 3.1 The 55: the same lane as the P95 gap

Excess of the 70-frame cadence set over a 2-VBlank frame, by bracket:

| owner | excess | ratio | share |
|---|---:|---:|---:|
| `SRC` / `GCRA` | **+163,330** | 1.53x | **81.2%** |
| `SITR` | +67,839 | 1.65x | 33.7% |
| `SPHD` | +36,345 | 1.58x | 18.1% |
| `SHDT` | +27,763 | **5.83x** | 13.8% |
| `GCRA-REM` | +15,115 | 1.17x | 7.5% |
| `SCPU` | +11,458 | 1.27x | 5.7% |
| `SPRM` | +4,980 | 3.51x | 2.5% |
| `AUD` | +21,401 | **4.05x** | 10.6% |
| draw side (`FTR`+`STG`+`MISC`+`BG`) | +16,201 | — | 8.1% |

**Cadence and P95 are the same lane on this arm.** That was not true before: the
2026-08-14 reading of the cadence set was 64% instrument (102 of 160 below the
boundary, 98 of those carrying the tick-HUD draw burst). With the instrument's
draw out, only 15 of 70 are non-cost.

### 3.2 The 15: the game's own interface draw, which the gate cannot see

On a `NDS_TICK_HUD_DRAW=0` arm `ndsPlatformRenderDebugHud` is compiled out, so the
`HUD` bucket is **`gNdsTickHudForegroundTicks` alone** — the bracket around
`ndsIFCommonNativeOamDrawGObj`, the battle interface's native OAM draw
(`src/port/sprite_preview_backend.c:3032-3048`, `:3130-3143`). That is product,
not instrument. Measured on this arm:

```text
HUD bucket        p50 1,024   p90 21,632   p99 73,024   max 2,130,944
                  73 of 1,597 frames above 50,000, mean 100,853 when it fires
                  inter-burst gap median 25 presented frames -> 1.41 bursts/s
drop rate         HUD>50k frames drop 34/73 = 47%
                  every other frame          115/1,524 = 8%      -> 5.9x
```

Cross-arm split, same build, same match: HUD sums 59,493,952 (`DRAW=1`) vs
11,406,016 (`DRAW=0`) over 1,600 frames — the instrument's draw is ~30,055 tk/fr
and **the game's interface draw is ~7,128 tk/fr**.

> **AND `WORK-H` EXCLUDES IT BY CONSTRUCTION.** The tick HUD's own identity is
> `ALL = named + HUD + OTHR` and `WORK-H = (ALL − WAIT) − HUD`
> (`taskman_seam.c:5176-5206`; the census re-derives both to max error 0). So
> subtracting `HUD` to remove the *instrument's* draw removes the *product's*
> interface draw with it. Roughly 7,128 tk/frame of real, shipped product cost —
> and up to ~100,853 on the frames that decide cadence — sits **outside the number
> the gate is scored on**. This is recorded, not re-scored: `plan.md` §0 fixes the
> scoring and the 24,947 apparatus figure is owner-approved. It is named here
> because the next cycle that tries to close the last few thousand ticks needs to
> know the gate metric is missing a product lane.

**Answer to item 3, in one line:** of the 4.1 points, **≈79% (55 of 70 frames) is
the `gcRunAll` simulation lane** — the same `SITR`/`SPHD`/`SHDT` owners the P95 gap
has — and **≈21% (15 of 70) is the game's battle-interface OAM redraw**, firing
1.41 times a second at ~100,853 ticks, invisible to `WORK-H`.

---

## 4. The re-rank, and slice 2's size

### 4.1 Bracket granularity, no profile needed, on the shipping candidate

`scripts/census-tick-hud-p95-set.py`. Two independent populations: last cycle's
banked 1-minute gate arm (80 frames set P95) and this cycle's 5-minute arm
(423 frames set P95, a 5.3x larger sample and a different fight).

| owner | 1-minute, 80 frames | 5-minute, 423 frames |
|---|---|---|
| `SRC` = `GCRA` | +358,851 · **84.8%** | +357,544 · **85.7%** |
| **`SITR`** | +152,483 · 2.46x · **36.0%** | +131,621 · 2.19x · **31.6%** |
| **`SHDT`** | +86,833 · **16.34x** · 20.5% | +112,531 · **17.02x** · **27.0%** |
| `SPHD` | +64,459 · 2.03x · 15.2% | +46,980 · 1.72x · 11.3% |
| **`SPRM`** | +35,110 · **20.40x** · 8.3% | +42,621 · **23.15x** · 10.2% |
| `SCAT` | +8,170 · 7.53x · 1.9% | +3,379 · 3.69x · 0.8% |
| `SCPU` | −3,810 · 0.91x | +1,796 · 1.04x |
| **`GCRA-REM`** | **+15,246 · 1.17x · 3.6%** | **+18,517 · 1.21x · 4.4%** |
| `MISC` | +28,566 · 6.7% | +31,358 · 7.5% |
| `FTR` | +9,937 · 1.03x · 2.3% | +12,252 · 1.04x · 2.9% |
| `STG` | +840 · 1.00x · 0.2% | +1,344 · 1.01x · 0.3% |
| `AUD` | +24,898 · 4.70x · 5.9% | +14,310 · 3.02x · 3.4% |
| draw side total | 9.3% | 10.7% |

Nesting identities hold exactly on both (`GCRA` excess == sum of children,
delta 0; `SRC − GCRA` = −75 and +37).

> **This retracts `GATE_ARM_OWNERS.md`'s "the sub-`SRC` ranking is match-specific,
> only `SITR` survives".** On these two populations `SITR`, `SHDT`, `SPHD` and
> `SPRM` all survive and keep their order within one position. What did not
> survive was `SPHC` and `SCPU`, which are small on both.

> **And it kills a false draw-side owner.** Without the 2^22 exclusion, `STG`
> reads +53,383 / 10.2% on the 1-minute set. **That is one frame** (1937,
> `STG` 4,373,952 against a 174,405 mean); excluded, `STG` is **+840, 1.00x,
> 0.2%**. A single instrument frame in 1,600 was about to be reported as a stage
> lane.

### 4.2 What slice 2 can actually address

§K2's justification is *"~90% of the tail excess is inside `gcRunAll`"*. True —
and it does not transfer, because the excess is inside `gcRunAll` **precisely
because the fighter process bodies are inside `gcRunAll`**, and slice 2's design
explicitly leaves those untouched. The bracket that isolates what a flattened
scheduler could touch is `GCRA-REM` (= `GCRA` minus `SINT`/`SHDT`/`SPHD`/`SPHC`/
`SCAT`/`SPRM`): **3.6–4.4% of the P95-set excess, at 1.17–1.21x — a flat lane**,
and it also contains the camera, effects, items, weapons and interface GObjs,
which flattening does not remove either.

Sized directly, from the only v3 stall capture on the gate arm
(`build-c159-profile-bothcpu`, `…/2026-08-14_runtime2-p95-closure/gate-p95-pc.csv`,
symbol census over the 80 frames that set P95, basis cycles/(2 × 80)):

| symbol | tk/fr on the P95 set | what it is |
|---|---:|---|
| `ndsBaseGcRunAll` | **9,550** | the process-graph walk itself |
| `gcRunGObjProcess` | **5,988** | per-GObj process dispatch |
| `gcRunGObj` | 1,255 | per-GObj entry |
| `gcRunAll` | 491 | the port wrapper (carries the tick-HUD bracket) |
| `gcLinkGObjProcess` | 226 | graph maintenance |
| `gcGetGObjProcess` / `gcAddGObjProcess` / `gcEndGObjProcess` / `gcDefaultFuncRun` | 87 / 71 / 41 / 77 | |
| **total** | **≈ 17,786** | |

Everything else in the `gc*` family on those frames is animation, camera or draw
work that a flat vector does not touch: `gcPlayDObjAnimJoint` 28,303,
`gcCaptureCameraGObj` 9,489, `gcPlayAnimAll` 8,879, `ndsBaseGcPlayMObjMatAnim`
6,514, `gcParseMObjMatAnimJoint` 5,627, `gcAddDObjAnimJoint` 4,670,
`gcParseDObjAnimJoint` 4,422.

**The verdict.** The requirement is **+32,593 net at the 80th-largest frame**.
Slice 2's entire addressable lane is **17,786 tk/fr**, so *deleting the scheduler
outright is 1.8x short* — and a flat vector cannot delete it outright, because it
still has to call each process. A realistic flattening removes the linked-list
rediscovery and the priority re-walk, i.e. most of `ndsBaseGcRunAll`'s 9,550 and
part of `gcRunGObjProcess`'s 5,988: **~4,000–10,000 tk/fr, under the ≥14,080
cross-build floor.** Slice 2 is a correct architectural direction and a poor next
mechanism.

> **Caveat, stated because it bounds the claim.** That per-symbol capture is
> `build-c159-profile-bothcpu` (2026-08-14, `DRAW=0`, pre-pack, pre-mechanism-fix,
> pre-seam) — the only v3 capture on the gate arm. Slice 1 did not touch the
> scheduler, so the sizing carries; it is a *sizing*, not a bank, and it should be
> re-taken on the shipping candidate if a package is ever written against it.

### 4.3 What the ranking points at instead

- **`SITR`** — the fighter `UpdateInterrupt` proc less its AI. Largest owner on
  both populations (31.6–36.0%), and `plan.md` §9 still records *"no mechanism has
  ever been priced against `SITR` itself"*. It is where the action-change
  machinery lives, which is where slice 1's remaining residue (§1.3) also lives.
- **`SHDT`** at **16.3–17.0x presence** and 20.5–27.0% of the excess. `HANDOFF`
  records it CLOSED — but closed *for the mechanisms tried*, on a whole-match bar
  of 47,424, never sized on the P95 frames. §K-RESULT's own rule applies: a lane
  closed as a micro-optimization says nothing about whether the work should exist.
- **The soft-float trio.** `__aeabi_fadd` 48,731 (3.6%) + `__aeabi_fmul` 37,492
  (2.8%) + `__aeabi_fdiv` 13,539 (1.0%) = **99,762 tk/fr on the P95 set**, 97%
  `issue`, no cache component — the largest single *nameable* lane in the capture
  and **3.0x the requirement**, with 38.0% of it caller-attributed inside the
  fighter procs. It is the only lane on the board whose ceiling exceeds the gap by
  a wide margin.
- Still true and unchanged: **no single function exceeds 3.6%**.

---

## 5. The multi-megatick frames — characterised, and they are 2^22

`PACING_PUBLICATION.md` §8 handed forward two unexplained frames (1843, 1937,
"~+4.1M each in one bucket"). Across every healthy arm on disk the class has one
signature and it is exact:

| arm | frame | bucket | excess over that bucket's median | Δ from 2^22 |
|---|---:|---|---:|---:|
| `c158-gate` | 1994 | `SPHC` | 4,194,368 | **+64** |
| `c166-nodispatch` | 533 | `SINT` | 4,191,488 | −2,816 |
| `c166-nodispatch` | 1772 | `MISC` | 4,271,040 | +76,736 |
| `c169-noarena` | 779 | `STG` | 4,203,648 | +9,344 |
| `c169-noarena` | 1819 | `SINT` | 4,210,368 | +16,064 |
| `c169-noarena` | 1826 | `FTR` | 4,193,792 | **−512** |
| `c170-seam` (gate) | 1843 | `SINT` | 4,195,072 | **+768** |
| `c170-seam` (gate) | 1937 | `STG` | 4,201,024 | +6,720 |
| `c170-seam` (draw0) | 771 | `STG` | 4,198,976 | +4,672 |
| `c170-seam` (draw0) | 1315 | `SPHD` | 4,216,704 | +22,400 |
| `c170-seam` (draw0) | 1648 | `FTR` | 4,194,496 | **+192** |
| `c171` 5-minute | 2704 | `SINT` | 4,195,200 | **+896** |
| `c171` 5-minute | 7590 | `SINT` | 4,206,656 | +12,352 |
| `c171` 5-minute | 7974 / 7275 | `SINT` | 4,144,384 / 4,466,944 | −49,920 / +272,640 |

**2^22 = 4,194,304 ticks = 0.12515 s at 33.514 MHz.** The tight cases land within
0.02% of it. `ALL` rises by the same amount and then rounds up to the next VBlank
boundary (5,312,xxx = 2^22 + 2 VBlanks, 5,872,xxx = +3, 6,993,472 = +5), so the
guest's own free-running timer really did advance — this is not bucket arithmetic.

What is established:

- **It predates every 2026-08-15 change**: `c158-gate` (2026-08-14, no pack) has one.
- **It is not a function of game state**: the `DRAW=1` and `DRAW=0` arms of the
  *same build* play the same match and put the events on completely different
  presented frames (1843/1937 vs 771/1315/1648). Two arms (`c164-bp0`, `c168`)
  have none at all.
- **Rate ≈ 1 per 2,100 presented frames** (4 in 8,448 on the 5-minute arm; 0–3 in
  1,600 elsewhere), i.e. roughly one per 70 s.
- **It lands in whichever bracket is open, with `SINT` over-represented** — 6 of
  14 against `SINT`'s 13.9% share of `ALL` — which is the one clue that points at
  the acquisition path rather than a uniform host hitch.
- **It never touched a banked percentile**: on the 1-minute gate arm rank-16
  (top-1%) is 1,518,528 with them and 1,512,448 without; rank-80 moves 1,177,920
  → 1,176,128.

Leading hypothesis, **not measured**: a fixed-duration I/O stall — a payload read
whose slow path costs ~0.125 s — observed by the guest's hardware timer inside
whatever bracket was open. The cheap discriminator has never been run and costs
nothing extra: `sample-tick-hud-buckets.ps1 -PerFrameGlobals
gNdsRelocAssetPayloadReadCount,gNdsR2AnimCacheFills` on the next gate run answers
"did that frame take a FAT read" directly. Handed forward.

**The attribution consequence is real and is already banked above:** these frames
must be excluded from *attribution* (never from a percentile). `--drop-frames` was
added to `census-tick-hud-p95-set.py` for exactly this, with the prohibition in
its help text.

---

## 6. What this cycle did NOT do

- **No default flip.** `NDS_R2_BATTLEPACK` / `…_KEEP_CACHE` remain 0 / 0.
  `BLOCKED(decision: shipping default)` is now *ready* to go to the owner —
  acceptance items 3 and 4 are answered and phase 7 is measured — but the flip is
  the owner's and this cycle did not take it.
- **Phase 6's evaluator oracle was not built, and should not be.** See the kernel
  doc §13: there is one evaluator, so an evaluator-vs-evaluator oracle has one
  arm. The obligation is re-stated there as a *representation* obligation with its
  residual gap named.
- **Item 3 is still NOT MET and is not claimed.** 90.7–90.9% two-VBlank against
  ≥95%. This cycle attributed the deficit; it did not close it.
- **No `Latest` verifier run.** The edits touch `nds_reloc_assets.c`, which is
  shared startup code, but every increment is gated on `gNdsK0BattleInGo`, which
  no non-battle scene ever sets, and Boundary boots the ROM through the same
  startup and asset loader. Stated so the next cycle can decide differently.
- **The 660 s chain soak was not re-run.** It passed on this configuration last
  cycle (`PACING_PUBLICATION.md` §7); only the long single match was missing.
- **`gNdsRelocAssetIDForToken`'s survival (§1.3) was not priced or fixed.**
- **No root ROM was rebuilt.** Hashed before the first build and after the last:
  `smash64ds.nds` `54C07FAC80C50418…`, `smash64ds-battle-playable-hwtri.nds`
  `2015FBD1F68B81C0…` — unchanged.

---

## 7. Reproduction

```powershell
# the arm: shipping candidate + the soak match timer
make -C . TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c171-k0-5min-bp1 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_SOAK_MATCH_MINUTES=5

# one run: five-minute match, phase-7 counters, allocator gates, coverage
.\scripts\sample-tick-hud-buckets.ps1 -RunnerSlot 2 -Build build-c171-k0-5min-bp1 -NoBuild `
    -Samples 8448 -StartFrame 438 -RingDump -TimeoutSeconds 4200 `
    -ExtraGlobals (Get-Content -Raw artifacts\performance\2026-08-15_k0-rerank\extraglobals.txt).Split(',') `
    -RowsCsv …\c171-k0-5min-rows.csv -JsonOut …\c171-k0-5min.json

# the re-rank, zero builds, on any rows CSV
python scripts/census-tick-hud-p95-set.py --rows <rows.csv> --drop-frames <2^22 frames>
python scripts/census-tick-hud-p95-set.py --rows <draw0 rows.csv> --cadence 70 --drop-frames 771,1315,1648

# slice 2's sizing, zero builds, off the committed reduced CSV
python scripts/census-marginal-frame-owners.py --report --owner-roots `
    --pc-csv artifacts/performance/2026-08-14_runtime2-p95-closure/gate-p95-pc.csv `
    --build builds/build-c159-profile-bothcpu --top 900
```
