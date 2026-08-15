# Phase 8: the resident pack costs 2.9x the gate, and the owner is the cache it deleted

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **HEAD `79a9447fd6d`**
**Native Battle Kernel slice 1: Task A (flag-1 Boundary), Task B (pacing attribution),
Task C (phase 8 gate), Task D (verdict).**
Predecessor: `…/2026-08-15_battlepack-resident/BATTLEPACK_RESIDENT.md`.
**Builds spent: 4** (2 Boundary arms, 2 tick-HUD gate arms). Root ROMs unchanged (§8).

---

## 0. Outcome first

```text
BOUNDARY      GREEN at flag 1 AND GREEN at flag 0.  The R2-04 E2 FPS-HUD assert
              is fixed at its owning seam; it was never a torn write.
GATE          flag 0  P50 940,416  P90 1,097,920  P95 1,186,112 raw / 1,161,165 net
              flag 1  P50 939,648  P90 1,540,032  P95 3,447,488 raw / 3,422,541 net
              rank-80 moves +2,261,376.  The -73,659 projection did not merely
              fail to appear; the arm is 2.9x the control at the percentile.
OWNER         the 111 extra uncached acquisitions: 767.6 of the 770 extra
              whole-match VBlanks fall inside the gameplay window, and the
              P95-set bracket puts 86.3% of the excess in SITR at 36.19x.
NOT THE OWNER the 18 streamed chunks (<=0.3%, and they run ~430 presented frames
              before the window opens) and cross-build placement (P50 moved -768,
              inside its own ~5,700 floor, against a 160x-the-floor P95 move).
ROOT CAUSE    the carve did not shrink the raw file cache, it DELETED it:
              262,144 B -> 4,096 B.  Two entries fit.  126 stores were refused.
VERDICT       slice 1 as built is REFUTED at the current pool size.  The pack
              path is not shown to be the problem -- the pool is -- but this pair
              cannot price the deletion's benefit either, because the starvation
              swamps it.  NDS_R2_BATTLEPACK stays default 0.
```

---

## 1. Task A — the FPS-HUD assert was a debugger-coherency defect

Landed `79a9447fd6d`. Full derivation in that commit message; the short form:

**melonDS's GDB stub does not see the ARM9 D-cache.** `ARMv5::ReadMem`
(`melonDS-Accurate/src/ARM.cpp:1545`) special-cases ITCM and DTCM and otherwise
falls through to `ARM::ReadMem` → `BusRead32` (`:1491`). There is no DCache
lookup on that path.

**Measured, no build spent.** `scripts/probe-fpshud-publication.ps1` samples the
four globals at every `ndsBattlePlayableFrameCompleteMarker` stop. On the flag-1
arm `build-c163-battlepack`, 420 consecutive presented frames, 29 publications:

```text
n= 269  x10=248 sc=18 fw=15 tw=17857920  recompute=282  BAD  changed=X10
n= 270  x10=248 sc=19 fw=13 tw=17587904  recompute=248  OK   changed=SC,FW,TW
```

three times (n=269/270, 352/353, 380/381). **`X10` LEADS the other three by one
presented frame — it does not lag them**, which retracts the direction recorded
in `BATTLEPACK_RESIDENT.md` §2 and on the board.

**The compiled sequence says why.** In `ndsPlatformRenderDebugHud` (the HUD
function is inlined), one store precedes the `SampleCount` read-modify-write:

```text
2008028: str r4,[r2]   X10          write MISS (ARM946E-S does not write-
                                    allocate) -> reaches main RAM
200802a: ldr r2,[r1]   SampleCount  LINEFILL of that same 32-byte line
200802e: str r2,[r1]   SampleCount  write HIT -> marks dirty, ABORTS the bus
2008034: str r1,[r2]   FrameWindow  write, so RAM keeps the PREVIOUS sample's
200803a: str r1,[r2]   TickWindow   three words until the line is evicted
```

All four are in one line (`0x02104f40..0x4c`), so the straddle hypothesis was
right about the layout and wrong about the consequence, and the single-writer
hypothesis was right and irrelevant. The write is coherent; the **read** is not.

**Corroboration to the digit.** The Boundary failure was
`FPS_HUD=289,14,15,16856768`. The probe on that same build reads sc=14 → x10=**298**
and sc=15 → x10=**289** — exactly the pair the assert reported as one group.

**Fix:** `DC_FlushRange` at the publication seam
(`ndsPlatformPublishBattleFpsHudGroup`), per global so it cannot depend on the
linker keeping four objects adjacent.

| arm | samples | publications | X10-only transitions | inconsistent samples |
|---|---:|---:|---:|---:|
| before (`build-c163-battlepack`) | 420 | 29 | **3** | **3** |
| after (`build-battle-playable-proof-hwtri-harness`) | 420 | 29 | **0** | **0** |

**Boundary GREEN at flag 1 and GREEN at flag 0**, 0 `Exception:` lines in either
log. `NDS_R204_FPSHUD_SHADOW` was **not** spent: it exists to prove the group is
self-consistent at publication, and the per-frame probe proves that for free.
The brief asked for the shadow; the repo rule "cheapest discriminating
measurement first" outranks it.

---

## 2. Task C — the gate, measured

Both arms: `smash64ds-battle-playable-tickhud-hwtri`, `NDS_R2_BOTH_CPU=1`,
`NDS_TICK_HUD_DRAW=1`, DLDI **on**, mode 163 one-minute match, `-Samples 1600
-RingDump`, window = presented frames **439–2038**, HEAD `79a9447fd6d`.
`P95` is rank-80 of 1,600, the campaign's convention. Apparatus 24,947.

| | flag 0 `build-c164-gate-bp0` | flag 1 `build-c164-gate-bp1` | delta |
|---|---:|---:|---:|
| P50 | 940,416 | 939,648 | **−768** |
| P90 (rank 160) | 1,097,920 | 1,540,032 | +442,112 |
| **P95 (rank 80) raw** | **1,186,112** | **3,447,488** | **+2,261,376** |
| P95 net of apparatus | 1,161,165 | 3,422,541 | +2,261,376 |
| top-1% (rank 16) | 1,570,944 | 6,118,208 | +4,547,264 |
| max | 2,300,928 | 7,252,800 | +4,951,872 |
| mean `WORK-H` | 960,540 | 1,219,250 | +258,710 |
| frames over 1,120,380 | 135 | 271 | +136 |

**The flag-0 arm is a control that reproduces the bank.** Against
`build-c158-gate` at HEAD `a159069af0d` (P50 939,136 · P90 1,096,448 · P95
1,184,064) the deltas are +1,280 / +1,472 / **+2,048** — well inside the
cross-build floors, on a tree that has since taken the Phase-2 framebuffer
collapse, the `ftmain` patch and the whole battlepack. The requirement on this
HEAD is **65,732 at the 80th-largest frame**, not 64,452.

### Cadence — `NDS_TICK_HUD_DRAW=1` arm, stated as such

Whole match, the ROM's own counters (2,038 presented frames flag 0, 2,037 flag 1):

```text
flag 0   VBI 2:1724  3:287  4:14  5+:13   max 26   pacing VBlanks 4,501
flag 1   VBI 1591    3:258  4:39  5+:149  max 19   pacing VBlanks 5,271  (+770, +17.1%)
```

In-window (`ALL`/560,190, 1,600 samples): two-VBlank **84.2% → 77.1%**, max 6 → 13.
Cadence acceptance still reads from a `DRAW=0` arm (owner, `plan.md` §6); no
`DRAW=0` arm was built this cycle because the verdict does not turn on it.

### The end-of-match invariant pair — the arms fought the same match

`gNdsBattleTextHudP0Damage` **0** and `gNdsBattleTextHudP1Damage` **76** on both
arms; 2,038 vs 2,037 presented frames; total acquisitions **355 on both**. So
`route-ab-cannot-price-gameplay-change` does not apply: this is a cost delta, not
a different fight.

### Engagement, read from the same runs that produced the buckets

| counter | flag 0 | flag 1 |
|---|---:|---:|
| `gNdsBattlePackHits` | **0** | **197** |
| `gNdsBattlePackMisses` | 355 | 158 |
| total acquisitions | **355** | **355** |
| `gNdsBattlePackLoadSteps` / `LoadFails` | (absent) | 18 / 0 |
| `gNdsR2AnimCacheHits` | **338** | **30** |
| `gNdsR2AnimCacheFills` | 17 | **2** |
| `gNdsR2AnimCacheRejects` | **0** | **126** |
| `gNdsR2AnimCacheArenaReservedBytes` | 262,144 | 292,032 |
| `gNdsTaskmanArenaChosenSize` / `AllocFailCount` | 1,376,256 / 0 | 1,376,256 / 0 |

Proven boot headroom 320,576 (flag 0) and 319,840 (flag 1); arena healthy on both.
**Nothing is broken on the flag-1 arm** — it is doing exactly what it was built
to do, and that is the problem.

---

## 3. Task B — the attribution, and it is not close

The three candidates were separable, and two of them are refuted by arithmetic.

### 3.1 The 18 streamed chunks own **≤0.3%**

`ndsBattlePackResidencyStep()` runs once per `ndsR2AnimCachePreloadStep()`
(`reloc_backend_assets.c:7404`), which is called once per `scVSBattleFuncUpdate`
— one **source update**, 60 Hz. 18 steps is therefore the first 18 source updates
of the battle scene, i.e. presented frames ~1–9, and the measurement window opens
at presented frame **439**.

Independently of that reasoning, the arithmetic bounds them:

```text
whole-match extra VBlanks (ROM's own pacing counter)   4,501 -> 5,271   = +770
extra ALL ticks inside the 1,600-frame window          430,010,624      = +767.6 VBlanks
                                                       -----------------------------
everything outside the window, chunks included                          <= 2.4 VBlanks
```

**≤0.3% of the cost, and it is bounded whether or not the chunk frames are where
the source says they are.**

### 3.2 Cross-build placement owns ~0

P50 moved **−768** across the pair. The measured cross-build P50 floor is ~5,700
and the P95 floor is ≥14,080 with unreliable sign (`docs/VERIFYING.md`). A
+2,261,376 move at rank-80 is **160× the P95 floor**, and a placement effect large
enough to do that would have moved P50 with it. The flag-0 arm additionally
reproduces a bank taken on a different HEAD to +2,048, which is what a small
placement term looks like.

The brief asked for a flag-falsifier third arm. It was **not built**, and the
reason is that it would have been spent on the wrong question: with the candidate
2.9× the control at the percentile, the floor is no longer the discriminator.

### 3.3 The 111 extra uncached acquisitions own the rest

```text
full ROM loads   flag 0:  17 fills                              = 17
                 flag 1:  126 rejects + 2 fills                 = 128
                                                                  ----
extra                                                             111
430,010,624 ticks / 111  =  3,873,969 ticks per extra uncached acquisition (~6.9 VBlanks)
```

The P95-set bracket names the same owner from the other side. Against each arm's
own two-VBlank population:

| bracket | flag 0 excess | share | flag 1 excess | share | ratio |
|---|---:|---:|---:|---:|---:|
| `WORK-H` | +463,086 | 100% | **+3,880,918** | 100% | 5.24x |
| `SRC` | +413,671 | 89.3% | +3,843,396 | 99.0% | 13.74x |
| `SITR` | +192,781 | 41.6% | **+3,348,118** | **86.3%** | **36.19x** |
| `SPRM` | +44,676 | 9.6% | +241,219 | 6.2% | 134.61x |
| `SCAT` | +10,616 | 2.3% | +46,100 | 1.2% | 37.03x |
| `FTR` (draw) | +9,344 | 2.0% | +2,990 | 0.1% | 1.01x |
| `STG` (draw) | +757 | 0.2% | −71 | −0.0% | 1.00x |

`SITR` is the fighter interrupt/status proc — the bracket that performs the
acquisition. It goes from 41.6% of the excess to **86.3%**, at 36× the two-VBlank
frame. The draw side does not move at all. **This is the acquisition path, on the
frames that set P95.**

> **A correction the next cycle should carry.** The banked dose-response modelled
> a cache **miss** at **+645,225** (`…/2026-08-14_native-battle-kernel/`). Measured
> here on the gate arm with DLDI on, an extra uncached acquisition costs
> **3,873,969** — **6.0×** that. The old figure is a whole-frame regression
> coefficient taken on the profile arm with a *warm* cache; it does not price a
> load taken with the cache deleted, and it must not be reused for one.

### 3.4 The root cause, stated exactly

The carve did not shrink the raw file cache. **It deleted it.**

```text
NDS_BATTLEPACK_RESERVE_BYTES                287,936
gNdsR2AnimCacheArenaReservedBytes           292,032
                                            -------
left for the raw animation file cache         4,096   (was 262,144)
entries that fit (gNdsR2AnimCacheFills)           2   (was 17)
stores refused  (gNdsR2AnimCacheRejects)        126   (was 0)
cache hits                                       30   (was 338)
```

Every one of the 308 acquisitions that used to be a cache hit and is not served
by the pack now streams from ROM through FAT/DLDI, and 111 of them are net-new
full loads. §5 of `BATTLEPACK_RESIDENT.md` called this "unpriced". It is priced.

---

## 4. Task D — the verdict

**Slice 1 as built is refuted at the current pool size.** Not the architecture —
the *pool*. Three things are true at once and the packet must not collapse them:

1. **The deletion is real and firing.** 197 of 355 acquisitions (55.5%) are served
   from the pack, against a control that reads 0 with the same 355 total.
2. **The cost is attributable, in full, to a pool too small to hold both the pack
   and the cache it displaced.** Not to the pack path, not to streaming, not to
   placement.
3. **This measurement cannot price the deletion's benefit.** P50 is flat because
   the median frame has no acquisition, and P95 is swamped by the starvation. The
   **−73,659 at rank-80 remains a projection** and is now *less* supported than
   before, because the one arm that could have isolated it — pack resident, cache
   intact — does not exist. Do not report the benefit as measured, in either
   direction.

### 4.1 Price the RAM — and it does not close

Making the un-packed fighter whole again means restoring its cache alongside the
blob:

```text
reserve needed = blob 287,936 + raw cache 262,144   =  550,080
reserve today                                       =  292,032
                                                       -------
additional taskman-arena bytes required, ONE fighter   258,048
both fighters resident (K-POOL): ~559,632 needed vs ~301,564 available
                                                       shortfall ~258,068
```

The two shortfalls are the same number for the same reason. Against it:

| source | bytes | state |
|---|---:|---|
| `RAM_RECOVERY_PLAN` Phase 2 (`gSYFramebufferSets`) | 146,560 | **SPENT 2026-08-15**, measured |
| Phase 1 (`sOriginalDLPreview` + `…DisplayPreview`) | 21,600 | unspent, "needs final proof" |
| Phase 4 scratch-lifetime overlay, **candidates only** | ≤80,096 | unsized, unproven |
| Phase 3 (sprite-preview pair, ~150 KB) | **0** | **premise REFUTED** — both live under HW triangles |
| Phase 5 (arena consumers / finite pools) | unsized | unstarted |

**Optimistic ceiling of everything named and not yet refuted: 146,560 + 21,600 +
80,096 = 248,256 — short of the 258,048 the ONE-fighter non-destructive case
needs, and 26% of what two resident fighters need.** So the honest answer to "name
which phases cover the shortfall" is: **none do, and the named remainder does not
reach it even at its optimistic face value.** The only `.bss` objects large enough
to change that are `sNdsAudioFgmCache` (204,800 — already proven *under*-sized: 59
cues / 575,760 B working set) and `sNdsRelocSceneFileBuffer` (185,696, "already
optimized once").

And Phase 2's bytes are not yet arena bytes: converting them still requires the
three-file coupling `§K-RAM` names — cut `NDS_TASKMAN_ARENA_SIZE`'s companion
constants, lower the `0x130000` search floor, and reteach the Task 36
replay-admission guard (`nds_renderer.c:5734-5739`).

### 4.2 What this does NOT say

- It does not say the BattlePack is wrong. It says a resident pack that pays for
  itself by evicting the raw cache is a **net loss of 2.9× the gate**, which is
  what §K0's own rule predicted from the other direction: the raw cache is not
  "halfway there" to be casually spent, it is carrying 338 of 355 acquisitions.
- It does not say the pool must grow before anything else. A pack that is
  *smaller than the cache it displaces* would also close it, and the items-off
  re-pack (553,696 B for both fighters, mismatch 0) is already proven — the
  single-fighter Fox blob is 287,904 B against 262,144 B of cache, i.e. it is
  **1.098× the thing it evicts.** That ratio, not the RAM plan, is the cheapest
  lever nobody has priced.

**`NDS_R2_BATTLEPACK` stays default 0.** No default flip is proposed; flipping a
shipping default is the owner's call and the measurement does not ask for it.

---

## 5. Blast radius of the Task A mechanism — it is narrow, and here is why

The mechanism needs a specific shape: a store that **misses** a line (reaching
RAM), then a **load** of that same line (filling it), then stores that **hit** it
(dirtying it and aborting the bus write) — read by an observer that sees RAM.

**Exposed:**

- **A multi-word group that must be self-consistent, published at a seam.** This
  is the only shape that produces a *torn* read rather than a merely stale one,
  and the FPS-HUD group was the instance. Fixed.
- **Any value read within about one line-lifetime of its last write** — per-frame
  sampling, or a breakpoint inside/near the writing function. A `counter++` is a
  load-modify-store, so RAM holds the value from that line's last **linefill**,
  not from the last increment.
- **The converse, already documented (cycle 100):** a `-SetGlobals` poke into a
  line the guest writes is stomped by the writeback while the readback still
  reports success. Same root cause, opposite direction.

**Not exposed:**

- **Whole-run totals read at the end-of-run stop** — which is nearly every
  counter this campaign has banked. The D-cache is 8 KB / 4-way / 64 line
  indices with round-robin replacement and the match churns every index
  continuously, so a line is evicted (and written back) within microseconds of
  its last touch, long before a detach-time read.
- **Anything in ITCM or DTCM.** `ARMv5::ReadMem` special-cases both and reads the
  TCM array directly. That is also the cheap structural remedy if a future
  diagnostic group must be coherent without a flush.

**Two independent facts say the exposure is narrow in practice, and they are from
this cycle's own runs:** the flag-0 gate arm reproduced a bank taken on a
different HEAD to **+2,048 of 1,186,112**, and the acquisition totals came out
**identical at 355** across two different binaries. Neither survives systematic
staleness.

**No specific banked number in `docs/HANDOFF.md` is implicated**, and none was
re-measured. One instrument has the exposed shape by construction and is named
here so a later cycle can decide, not because there is evidence against it:
`sBattleTickHudRing` is written per presented frame and read by `-RingDump` in one
stop per 96 frames, so its most recent slots are the ones most likely to be dirty.
Against that, `sample-tick-hud-buckets.ps1` already hard-fails on an identical
payload and accounts ring-stop skew (`skew` 0/±1 on every stop this cycle), and
that guard has never fired. **Cheap general remedy where it is needed:**
`DC_FlushRange` at the publishing seam, exactly as
`ndsPlatformPublishBattleFpsHudGroup` now does — never a per-frame `DC_FlushAll`,
which would dwarf what it measures.

---

## 6. What this cycle did NOT do

- **No `DRAW=0` cadence arm.** Cadence figures above are `DRAW=1` and labelled.
- **No flag-falsifier third arm**, and no A/B/A. §3.2 gives the reason: the
  placement floor stopped being the discriminator at 160× its magnitude.
- **No arm that isolates the deletion's benefit** (pack resident + cache intact).
  That is the single most valuable missing measurement and it is next cycle's.
- **No phase-6 oracle**, per the brief.
- **No per-fighter or after-GO K0 counters** — phase 7's assertion is still
  unproven as `§K1` words it.
- **`scripts/probe-battlepack-pacing.ps1` produced NO usable rows, and the reason
  was mine.** It ran ~25 minutes on the flag-1 gate arm, then I force-killed gdb
  to harvest a partial capture — which **discarded gdb's buffered stdout**, since
  the helper redirects it at the process level. The capture is 587 bytes and holds
  only the two setup lines. This is the documented trap ("a probe that may be
  killed still needs `set logging enabled on`") re-earned; the script now sets
  incremental logging to its own artifact path and prefers that file, so the wrong
  form is no longer expressible. **§3's conclusion does not depend on it** — the
  window arithmetic and the P95-set bracket are independent instruments — but the
  per-frame dose-response it would have added is not in this packet.

---

## 7. Reproduction

```powershell
# Boundary, both arms.  verify-all.ps1 writes child output with
# [Console]::Out.Write, so only an OS-level redirect captures it.
$env:NDS_R2_BATTLEPACK='1'   # then '0'
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary > %TEMP%\b.log 2>&1"

# the FPS-HUD mechanism, no build
.\scripts\probe-fpshud-publication.ps1 -Build build-c163-battlepack

# the gate, both arms (the flag-1 line adds NDS_R2_BATTLEPACK=1)
.\scripts\sample-tick-hud-buckets.ps1 -RunnerSlot 2 -Build build-c164-gate-bp0 `
    -MakeFlags NDS_R2_BOTH_CPU=1 -Samples 1600 -RingDump -TimeoutSeconds 2400 `
    -ExtraGlobals gNdsBattlePackHits,gNdsBattlePackMisses,gNdsR2AnimCacheHits,`
gNdsR2AnimCacheFills,gNdsR2AnimCacheRejects,gNdsR2AnimCacheArenaReservedBytes,`
gNdsTaskmanArenaChosenSize,gNdsTaskmanArenaAllocFailCount,`
gNdsBattlePlayablePacingVBlanks,gNdsBattlePlayablePacingPresentedFrames,`
gNdsBattleTextHudP0Damage,gNdsBattleTextHudP1Damage `
    -RowsCsv artifacts/performance/2026-08-15_battlepack-gate/c164-gate-bp0-rows.csv `
    -JsonOut artifacts/performance/2026-08-15_battlepack-gate/c164-gate-bp0.json
python scripts/census-tick-hud-p95-set.py --rows artifacts/performance/2026-08-15_battlepack-gate/c164-gate-bp0-rows.csv
```

`gNdsBattlePackLoadSteps` must be dropped from the flag-0 `-ExtraGlobals` list:
its only writer is inside `#if NDS_R2_BATTLEPACK`, so `--gc-sections` removes it
and the harness throws (correctly) on a name that does not exist.

## 8. Root ROMs

Unchanged across the cycle — no published target was built.

```text
smash64ds.nds                          54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds    2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360
```
