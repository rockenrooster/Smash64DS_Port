# The isolation arm exists: slice 1 is REFUTED as a P95 lever, and phase 8's root cause was wrong

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `30b38f3e9d3`**
**Native Battle Kernel slice 1: the missing arm — pack resident AND raw cache intact.**
Predecessor: `…/2026-08-15_battlepack-gate/BATTLEPACK_GATE.md`.
**Builds spent: 2** (one arm, rebuilt once after the first sizing was refused by the heap).
Root ROMs unchanged (§8).

---

## 0. Outcome first

```text
THE ARM     built, valid, and it had never existed: pack resident, 197 hits,
            AND gNdsR2AnimCacheRejects = 0 with only NINE full ROM loads all
            match -- fewer than the control's seventeen.
VERDICT     IT DOES NOT PAY.  rank-80 3,447,872 raw / 3,422,925 net against the
            control's 1,186,112 / 1,161,165: +2,261,760.  P50 -288 (flat).
RETRACTED   the -73,659 projection.  Measured, the sign is OPPOSITE and the
            magnitude 30x larger.  It is no longer a projection.
REFUTED     phase 8's root cause.  It attributed +2,261,376 to 111 net-new
            uncached acquisitions.  This arm removes them -- 128 ROM loads -> 9
            -- and keeps the ENTIRE residual: rank-80 moves +384, 0.011%.
            The cache deletion was a passenger, not the driver.
THE OWNER   is the pack path itself.  It is the only thing the two expensive
            arms share, and they agree to +384 across different arenas and a
            40x difference in cache size.
LEAD        gNdsRelocResolveOffsetCount 0 -> 3,629 is the ONLY counter that
            differs between arms.  A LEAD, not an attribution -- see section 5
            for why this packet refuses to divide the residual by it.
NOT SHIPPABLE  this arm grows NDS_TASKMAN_ARENA_SIZE and Boundary's verifier
            pins that constant, so it cannot be mistaken for a candidate.
```

---

## 1. The arm, and the first attempt the heap refused

The configuration is `NDS_R2_BATTLEPACK=1` plus a new lab flag
`NDS_R2_BATTLEPACK_KEEP_CACHE=1`: the animation arena reserves the blob **and** a
raw file cache, and `NDS_TASKMAN_ARENA_SIZE` grows by the same amount so the
general heap is not robbed to pay for it.

### The first sizing was wrong, and the way it failed is the finding

Asked for the full 262,144 B cache: reserve `287,936 + 262,144 = 550,080`, arena
`0x150000 → 0x18f000` (+258,048), against `check-boot-headroom.ps1`'s **319,840 B
of proven headroom**. Measured:

```text
gNdsTaskmanArenaChosenSize            1,564,672   requested 1,634,304 -- SHORT by 69,632
gNdsTaskmanArenaAllocFailCount               17   the step-down loop gave up 17 x 0x1000
gNdsR2AnimCacheArenaReservedBytes       550,080   ReserveFailCount 0  <- the reservation SUCCEEDED
general heap free bytes                   6,076   against the mandated 32,768 floor
gNdsBattlePackHits / LoadSteps / State    0/0/0   residency never ran
soak verdict                      NEVER-STARTED   zero battle frames presented
```

The ROM booted, passed every allocator guard, and never started the battle. A
2,400 s gate run against it never reached ring stop 0 — which is the same
signature the boot ladder already records for `build-c82-src-nobracket`
(*"240 s, never reached ring stop 0"*).

**Two lessons, both new faces of one already paid for.**

1. **THE BOOT LADDER IS NECESSARY AND NOT SUFFICIENT — third recurrence, and the
   first for ARENA growth rather than static growth.** `check-boot-headroom`
   meters the static image against an empirical boot threshold; it does not
   meter the heap a runtime `calloc` can actually be *given*. **Measured
   grantable ceiling on this arm: 1,564,672**, i.e. **+188,416 B**, which is
   58.9% of what the ladder's 319,840 implied. `gNdsTaskmanArenaAllocFailCount`
   is the tell and **nothing gates on it**.
2. **`NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE` cannot protect against this.**
   `ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap, 550,080 + 32,768, …)` returned
   TRUE, correctly: at reservation time the heap *did* hold 582,848 free. It is a
   point-in-time check and cannot see the scene's later demand, which is exactly
   what starved. A guard that passes and a ROM that never starts a match are not
   in contradiction.

### The sizing that works, and it is measured rather than derived

```text
arena     0x17a000 = 1,548,288   (+172,032; 16,384 UNDER the proven grantable ceiling)
reserve   287,936 blob + 163,840 cache = 451,776
```

Every gate check passes on the shipped arm:

```text
gNdsTaskmanArenaChosenSize        1,548,288   exactly as requested   AllocFailCount 0
gNdsR2AnimCacheArenaReservedBytes   451,776   ReserveFailCount 0
gNdsR2AnimCacheArenaUsedBytes       406,416   = 287,936 carve + 118,480 of actual cache use
gNdsTaskmanGeneralHeapFreeMin        52,864   floor 32,768 -- 20,096 clear
gNdsR2AnimCacheRejects                    0
```

**The acceptance test is `Rejects == 0`, not the byte count.** A 163,840 B cache
is equivalent to the control's 262,144 exactly when it still refuses nothing, and
it does: it used **118,480** of its 163,840, leaving 45,360 spare (27.7%). Even
131,072 would have sufficed.

### Why this is diagnostic and cannot drift into a bank

`verify-battle-mariofox-gcrunall-loop-harness.ps1` asserts
`gNdsTaskmanArenaChosenSize == 1376256` in two places, so **this arm is
structurally incapable of passing Boundary.** That is the guard that stops a lab
configuration being mistaken for a shippable one, and it is worth more than the
comment in the Makefile. `NDS_R2_BATTLEPACK` and `NDS_R2_BATTLEPACK_KEEP_CACHE`
both stay default 0.

**The shipping default is unchanged, and that is MEASURED rather than argued.**
`build-c165-default-check` (this tree, defaults, `BOTH_CPU=1`) against the banked
control `build-c164-gate-bp0` (HEAD `79a9447fd6d`):

```text
                           text     data        bss     total   fake_heap_start
build-c164-gate-bp0     985,468  148,288  1,307,016  2,440,772      0x022463c4
build-c165-default-check 985,468  148,288  1,307,016  2,440,772      0x022463c4
```

Byte-identical in every section. **Zero shipped bytes changed**, so Boundary's
state — GREEN at flag 0 and flag 1 on `79a9447fd6d` — is undisturbed and was not
re-run. Compare ELF sections rather than the `.nds`: NitroFS packs directory
entries nondeterministically.

---

## 2. The gate, measured

All three arms: `smash64ds-battle-playable-tickhud-hwtri`, `NDS_R2_BOTH_CPU=1`,
`NDS_TICK_HUD_DRAW=1`, DLDI **on**, mode 163 one-minute match, `-Samples 1600
-RingDump`, window = presented frames **439–2038**. `P95` is rank-80 of 1,600,
the campaign's convention. Apparatus 24,947. A and B are the banked phase-8 arms
at HEAD `79a9447fd6d`; C is this cycle.

| | **A** control<br>no pack · cache 262,144 · arena 0x150000 | **B** destructive<br>pack · cache 4,096 · arena 0x150000 | **C** ISOLATION<br>pack · cache 163,840 · arena 0x17A000 |
|---|---:|---:|---:|
| P50 | 940,416 | 939,648 | **940,128** |
| P90 (rank 160) | 1,097,920 | 1,540,032 | 1,216,832 |
| **P95 (rank 80) raw** | **1,186,112** | **3,447,488** | **3,447,872** |
| P95 net of apparatus | 1,161,165 | 3,422,541 | 3,422,925 |
| top-1% (rank 16) | 1,570,944 | 6,118,208 | 6,175,104 |
| max | 2,300,928 | 7,252,800 | 7,245,056 |
| mean `WORK-H` | 960,540 | 1,219,250 | 1,196,937 |
| frames over 1,120,380 | 135 | 271 | 218 |
| **frames over 2,000,000** | **2** | **130** | **128** |

```text
C - A   P50 -288 (flat)   rank-80 +2,261,760   mean +236,397   over-gate +83
C - B   P50 +480          rank-80    +384      mean  -22,313   over-gate -53
```

`VERIFYING.md` requires an A/B to be ranked on **P50, mean and over-gate count**,
not on rank-80 alone (cross-build P95 floor ≥14,080, sign unreliable). All three
agree: **P50 flat, mean +236,397, over-gate +83.** The cost is real, it is not
placement, and it does not touch the median frame.

### Cadence — `NDS_TICK_HUD_DRAW=1` arm, stated as such

In-window (`ALL`/560,190, 1,600 samples):

```text
A  2:1348  3:237  4:13  5:1  6:1                                    max  6   two-VBlank 84.2%
C  2:1290  3:177  4:5  5:7  6:28  7:27  8:23  9:15  10:2  11:7
                 12:12  13:6  14:1                                  max 14   two-VBlank 80.6%
```

Whole match, the ROM's own counters: C `gNdsBattlePlayablePacingVBlanks` **5,218**
over 2,038 presented frames, present-interval **max 19** (A 4,501; B 5,271).
Cadence acceptance still reads from a `DRAW=0` arm (owner, `plan.md` §6); none was
built because the verdict does not turn on it.

### Same fight — the arms are comparable

`gNdsBattleTextHudP0Damage` **0** and `gNdsBattleTextHudP1Damage` **76** on all
three arms; **355 total acquisitions on all three**; 2,038 presented frames on A
and C. `route-ab-cannot-price-gameplay-change` does not apply: this is a cost
delta, not a different fight.

### Engagement — and this is what makes C the isolation

| counter | A control | B destructive | **C isolation** |
|---|---:|---:|---:|
| `gNdsBattlePackHits` | 0 | 197 | **197** |
| `gNdsBattlePackMisses` | 355 | 158 | **158** |
| total acquisitions | **355** | **355** | **355** |
| `gNdsR2AnimCacheHits` | 338 | 30 | **149** |
| `gNdsR2AnimCacheFills` | 17 | 2 | **9** |
| `gNdsR2AnimCacheRejects` | **0** | **126** | **0** |
| `gNdsR2AnimCacheArenaReservedBytes` | 262,144 | 292,032 | **451,776** |
| **full ROM loads** (`Fills + Rejects`) | **17** | **128** | **9** |

Every one of C's 158 non-pack acquisitions is accounted for: **149 cache hits + 9
fills = 158.** The isolation arm does **fewer** ROM loads than the control.

---

## 3. The verdict, and it retracts two things

### 3.1 Slice 1 does not pay — the projection is now a measurement

The modelled full deletion of the acquisition path was **−73,659 at rank-80**
(`…/2026-08-14_native-battle-kernel/`), carried as a projection through three
cycles. Measured on the arm built to isolate it:

> **+2,261,760 at rank-80.** Opposite sign, ~30× the magnitude.

**`−73,659` is retracted. Do not carry it forward in any form.** The upper-bound
dose-response model that produced it priced the acquisition path's *removal*
without pricing what replaces it, and what replaces it is more expensive than
what it deleted.

### 3.2 Phase 8's root cause is refuted

Phase 8 attributed its +2,261,376 to **111 net-new uncached acquisitions at
3,873,969 ticks each**, and named the carve deleting the raw cache
(262,144 → 4,096 B) as the root cause. That attribution divided the whole residual
by a count without an arm that could separate the two things the destructive arm
changed at once.

This arm separates them. It restores the cache to full health — `Rejects` 126 →
**0**, ROM loads 128 → **9**, i.e. it removes *more* than the 111 the model
charged — and the residual **does not move**:

```text
rank-80   B 3,447,488  ->  C 3,447,872     +384   (0.011%)
```

against a cross-build P95 floor of **≥14,080**. Two arms differing in arena size
(0x150000 vs 0x17A000), in cache size by **40×**, and in ROM loads by **14×**,
landing 384 ticks apart, do not have independent causes. **They share one, and
the only thing they share is the pack path.**

> **THE CORRECTION, STATED SO IT CANNOT BE MISREAD.** The cache deletion was
> real, and it was a passenger. `+645,225 a miss` was already retracted as a
> warm-cache coefficient; **`3,873,969 per uncached acquisition` is now retracted
> too, and for a stronger reason — it priced a mechanism that owns ~0 of the
> residual.** Both figures came from dividing a residual by whichever count was
> to hand. Neither may be reused.

### 3.3 What this does NOT say

- It does not say the BattlePack format is wrong. Slot equivalence is mismatch 0
  and the resolver refuses nothing (`gNdsObjAnimRunawayCount 0`,
  `gNdsRelocResolveMisalignCount 0`).
- It does not say the *pool* is fine. The pool question is now simply moot for
  this decision: buying the RAM does not buy the win, because the win was never
  there. `§K-RAM`'s shortfall arithmetic no longer gates anything.
- It does not identify *which part* of the pack path costs. Section 5.

---

## 4. Where the cost lives — the bracket, on the frames that set P95

P95 set (top 80 by `WORK-H`) against each arm's own two-VBlank population:

| bracket | A excess | share | **C excess** | **share** | **ratio** |
|---|---:|---:|---:|---:|---:|
| `WORK-H` | +463,086 | 100% | **+3,882,369** | 100% | 5.21x |
| `SRC` | +413,671 | 89.3% | +3,757,271 | 96.8% | 13.23x |
| `SINT` | +185,726 | 40.1% | +3,333,000 | 85.8% | 24.56x |
| **`SITR`** | **+192,781** | **41.6%** | **+3,326,913** | **85.7%** | **34.48x** |
| `SPRM` | +44,676 | 9.6% | +235,768 | 6.1% | 131.25x |
| `SCAT` | +10,616 | 2.3% | +47,019 | 1.2% | 37.67x |
| `SPHD` | +75,445 | 16.3% | +115,886 | 3.0% | 2.88x |
| `FTR` (draw) | +9,344 | 2.0% | +4,119 | 0.1% | 1.01x |
| `STG` (draw) | +757 | 0.2% | +52,189 | 1.3% | 1.30x |

`SITR` — the fighter interrupt/status proc, the bracket that performs the
acquisition — goes from 41.6% of the excess to **85.7% at 34.48×**, matching the
destructive arm's 86.3% at 36.19× almost exactly. The draw side does not move.

**The population is 128 frames**, not a distributed cost: mean `WORK-H` **4,118,565**
on those frames with `SITR` **68.4%** of it, against 2 such frames on the control.
That is why P50 is flat and mean is +236,397.

---

## 5. The lead — and why this packet will not turn it into a number

Two 2.5-minute soaks, same match, control (`build-c164-gate-bp0`) against
isolation (`build-c165-keepcache-bp1`):

```text
sNdsAObjEvent32NormalizedCount        245  ==   245
gNdsAObjEvent32NormalizeScriptCount   225  ==   225
gNdsAObjEvent32NormalizeReuseCount  1,609  == 1,609
gNdsTaskmanMallocCount              1,069  == 1,069
gNdsObjAnimRunawayCount                 0  ==     0
gNdsRelocResolveOffsetCount             0  -> 3,629      <- the only difference
```

**This excludes the leading hypothesis.** The AObj16 normalizer is *not* being
re-run on packed scripts — every normalizer counter is identical, so
`ndsRelocPointerIsFighterAObj16`'s admission gate is doing its job. Allocation
counts are identical too, so the cost is not a new allocation.

The one differing counter is `gNdsRelocResolveOffsetCount` **0 → 3,629**: the
blob-relative-offset branch in `ndsRelocFindKnownFileContaining`, taken 3,629
times in a one-minute match on the pack arm and never on the control. The prior
cycle recorded it as *proof of engagement*; **nobody has ever priced it.**

> **DO NOT DIVIDE THE RESIDUAL BY 3,629.** That is precisely the operation that
> produced `3,873,969 per uncached acquisition` and `+645,225 a miss`, both now
> retracted. For the record the division would demand ~104,000 ticks per resolve,
> which is not a plausible price for a table walk — so either the resolver is not
> the whole story or it is doing something far worse than a scan, and **only
> per-PC attribution can tell which.** `scripts/census-profile-pc-per-region.py`
> reads per-frame call counts for any symbol out of a v3 capture without a build.

**The build that separates the last two candidates** — the pack's *dispatch*
versus the pack's mere *presence* in the arena — is the slice-51 falsifier:
pack resident, streamed, carved, and `ndsBattlePackFindFigatree` returning NULL.
Candidate layout, dispatch reverted. That is one build and it is next cycle's
first.

---

## 6. What this cycle did NOT do

- **No falsifier arm** (pack resident, dispatch reverted). It is the single most
  valuable next build and §5 states it precisely.
- **No matched control at the grown arena** (pack off, arena 0x17A000). The
  natural control turned out stronger: B and C already differ in arena size and
  agree to +384, which bounds the arena term far below the effect.
- **No `DRAW=0` cadence arm**; every cadence figure above is `DRAW=1` and labelled.
- **No phase-6 oracle**, per the brief.
- **No per-fighter or after-GO K0 counters** — phase 7's assertion is still
  unproven as `§K1` words it.
- **Task B was not attempted, because its premise is spent.** Section 7.
- **`NDS_R2_BATTLEPACK` stays default 0.** No flip proposed; none would be
  defensible.

---

## 7. Task B — the premise was already spent, with the arithmetic to close it

The Makefile recipe **already** generates the blob `--items-off`
(`Makefile:3488`), so the shipping 287,904 B `battlepack_fox.bin`
(sha256 `f6a49219a32583f4…`) *is* the items-off pack. The full pack is 332,992 B.

```text
Fox pack, items off, as shipped   287,904        clips 137   scripts 2,713
raw cache it displaces            262,144
                                  -------
over by                            25,760        = 1.098x its own displacement

  of which:  stream               272,292  (94.6% of the blob)  <- ALREADY 10,148 OVER
             per-slot tables       14,444
             directory              1,096
             header etc.              72
```

**The stream alone exceeds the displacement by 10,148 B.** Deleting *all* 15,612 B
of metadata still leaves the pack too large, so **"shrink the metadata" is closed
by arithmetic**, and `§K-RAM` already closed "compact the stream losslessly" (u16
command headers, s16 target words — no f32 to narrow, no dictionary beats a
16-bit alphabet).

Two levers remain, and they are not equivalent in kind:

1. **Drop clips beyond items-off** — needs another 25,760 B, ~11 clips at the
   items-off average. **No proven-unreachable set of that size exists**: the
   14-clip matchup lead reaches the byte target but its handlers are merely
   *absent*, which `§K-RAM` already ruled is not proof.
2. **A lossy stream** — quantisation or a reduced key rate. That is a
   **visual-fidelity trade** under `PROJECT_GOAL.md`'s sacrifice order.

> **`BLOCKED(decision: the pack cannot fit its own displacement losslessly).`**
> Option 1 costs the correctness work to *prove* ~11 clips unreachable for this
> matchup, and a wrong proof is a garbage script handed to the parser — whose
> failure mode is a freeze, not a diagnostic. Option 2 costs animation fidelity
> and needs the owner's eye. **Neither is chosen here.**
>
> **And this decision is not currently worth taking.** §3 says a resident pack
> that fits would still not pay: the isolation arm already has a healthy cache
> and costs +2,261,760. Fitting the pack removes the *RAM* objection, not the
> *cost*. This fork should stay parked until §5's falsifier says whether the pack
> path can be made cheap at all.

---

## 8. Root ROMs

Unchanged across the cycle — no published target was built.

```text
smash64ds.nds                          54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds    2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360
```

## 9. Reproduction

```powershell
# the arm
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c165-keepcache-bp1 `
     NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1

# GATE THE ARM ON THE SOAK FIRST -- five minutes, and it would have refused the
# first sizing before a 2,400 s gate run was spent on it.  Check Rejects == 0,
# ArenaChosenSize == requested, AllocFailCount == 0, ReserveFailCount == 0, and
# that a match actually completes.
.\scripts\soak-freeze-watch.ps1 -RunnerSlot 2 -Build build-c165-keepcache-bp1 `
    -Target smash64ds-battle-playable-tickhud-hwtri -NoBuild -BothCpu $true `
    -MatchMinutes 0 -MinutesToRun 2.5

# the gate.  Detached with an OS-level redirect: a buffered child's stdout is
# lost to ANY abrupt parent termination, and this run lost one capture to a
# 10-minute tool cap exactly as the previous cycle lost one to a force-kill.
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\sample-tick-hud-buckets.ps1 ^
    -RunnerSlot 2 -Build build-c165-keepcache-bp1 -NoBuild -Samples 1600 -RingDump ^
    -TimeoutSeconds 2400 -ExtraGlobals <list> ^
    -RowsCsv artifacts\performance\2026-08-15_battlepack-isolation\c165-iso-rows.csv ^
    -JsonOut artifacts\performance\2026-08-15_battlepack-isolation\c165-iso.json > run.log 2>&1"

python scripts/census-tick-hud-p95-set.py `
    --rows artifacts/performance/2026-08-15_battlepack-isolation/c165-iso-rows.csv `
    --apparatus 24947
```

`-BothCpu` is a `[bool]`, so it cannot be passed through `pwsh -File` (which does
no PowerShell parsing of argument values and hands it the string `"1"`). Call
`soak-freeze-watch.ps1` directly, or pass `$true` from a real PowerShell parse.
