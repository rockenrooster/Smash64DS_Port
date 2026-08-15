# The GX stack leak is fixed at its seam; the asset-I/O lane is Mario's animation and it converts 0.30x, not 3.1x

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `ba2c5e57026`.
Premise: `../2026-08-15_cfx-narrow-exchange/MENU.md` + `EXCHANGE.md`.
**UNITS: 2 profile cycles = 1 project tick.** Requirement **+32,593 net at rank-80**
(bank `c170` 1,177,920 raw / 1,152,973 net).

## 0. Outcome first

1. **The GX matrix-stack leak PREDATES slice 43 and is now FIXED at its seam.**
   Measured on a `NDS_R2_FIGHTER_GX_COMPOSE=0` ROM the position/vector stack
   level advances **+3.000 per presented frame wrapping mod 32** with GXSTAT's
   overflow/underflow error bit set on **128 of 128** sampled frames. After the
   one-line fix the level reads **0 on all 128 frames and at all 17 whole-match
   stops (frames 534..2038)** and the error bit is **clear everywhere**.
2. **The site is the Task 36 replay capture, and it is a delta recorded where a
   state was required.** `nds_renderer.c:6218` set `run->local_pushed` from
   `capture_push_balance`, the run's own *net* push delta. Task 36's
   `EnsureWorld` pops the previous binding's world before pushing its own
   (`:30290-30294`), so **every run after the first in a segment records balance
   0 while the stack is still one push deep**. Replay assigns that flag verbatim
   (`:30783`, last run wins) and `EndSegment` pops on it (`:30424`), so each
   replayed segment left its push on the stack forever.
   **Rate check: 3.000 replayed segments a frame, +3.000 levels a frame.**
3. **The in-match asset I/O lane is unpacked-Mario animation, NOT BGM — and its
   convertible value at rank-80 is 9,863 tk/fr (0.30x the requirement), not the
   100,689 / 3.1x the menu carries.** It happens on **7 frames of 1,600**, and
   those 7 frames are ranks 3, 5, 10, 13, 14, 16 and 23 — deep *inside* the top
   80. Deleting them outright moves the 80th-largest frame 1,194,342 -> 1,184,479.
4. **BGM is exonerated by measurement, not by argument.** `ndsAudioBgmReadPacket`
   runs **76 times in the window at presence 1.056x** (0.0475/region whole match
   vs 0.0500 on the P95 set) and **0 times on the 7 load frames**. It is the
   flattest row in the lane.
5. **`armCopyMem32` is the same work counted at a lower layer, not a second
   producer.** Its calls equal `_dvmCacheCopy`'s **exactly** (1,986 = 1,986) and
   `_ntrcardRomReadSector` executes **0 times** in the window, so every byte it
   moves comes through the FatFs disc cache that `get_fat`/`f_lseek`/`_FAT_read_r`
   already bill.
6. **The draw side is 348,268 tk/fr at rank-80 across 36 symbols at ~1.02x** —
   2.0x the menu's 170,953 seven-symbol figure and **10.7x the requirement** —
   and it converts 1:1 because it is flat. Its per-frame work is now enumerated
   exactly (§4). Its hardware floor is **~27,467 tk/fr of GX FIFO bus
   contention**, which no software change removes.

---

## 1. Task A — the leak, measured, sited and repaired

### 1.1 It is not slice 43's, and it is not the particles

Instrument: `sample-tick-hud-buckets.ps1 -NoBuild -PerFrameGlobals` reading the
ROM's own `gNdsHardwareRendererStatus` (GFX_STATUS, `nds_platform.c:3292`) once
per presented frame. Arm `build-c173-cfxcount-bp1`
(`NDS_R2_FIGHTER_GX_COMPOSE 0`, `NDS_TICK_HUD 1`, `NDS_TICK_HUD_DRAW 1`,
`BOTH_CPU 1`, `BATTLEPACK 1`), frames **438..565 = 128 presented frames**,
`gxstat-c173-rows.csv`.

```text
total level advance 378 over 127 intervals = 2.9764/frame  (= +3.000, one stale read)
GXSTAT error bit (15)  set on 128 of 128 frames
projection stack level (13) 0 on 128 of 128 frames
```

**Negative control inside the same run.** `gNdsParticleBatchOpens` stops
advancing at frame 471 and stays flat for the rest of the window, as do
`gNdsParticleScaleEscalations` and `gNdsParticleCameraLoads`. On the **37 of 127
intervals that open no particle batch at all the level still advances 3.000 on
average** (against 2.967 on the 90 intervals that do). The particle and Whispy
paths cannot be the leak.

### 1.2 The linked ELF says the ARM9's own push/pop is exactly balanced

Every `MATRIX_PUSH` (`0x04000444`) and `MATRIX_POP` (`0x04000448`) store in
`build-c181-cfxnarrow-b-d0` was located by disassembly (17 sites in 10
functions, every POP writing the immediate `1`) and its **exact execution
count** read out of the reduced per-PC census `b-c181-pc.csv` (1,600 regions,
gate arm, `DRAW=0`).

```text
register-write PUSH total   5,143 (3.2144/fr)
register-write POP  total   4,831 (3.0194/fr)                    net +312
raw FIFO MATRIX_POP word, ndsRendererFinishWhispyNativePacket
  (store at 0x020116cc)       312 (0.1950/fr)                    net -312
                                                        ------------------
  every push/pop the ARM9 writes itself                          net  0
```

The residual is **exactly** the Whispy packet's raw pop — 312 whole match, 27 on
the marginal 80, matching the register imbalance to the unit on both
populations. So the CPU-visible ledger closes at zero and the leak's producer is
the one the census cannot see: **the Task 36 replay word streams DMA'd into
`GFX_FIFO` (`nds_renderer.c:30765-30768`).**

Two more exact facts from the same census pin it:

- `ndsRendererNativeStageTask36EndSegment.part.0` is entered **3.000 times per
  frame** (4,800 calls / 1,600 regions) and its `MATRIX_POP` store at
  `0x02013dec` executes **0 times in the whole match**. The segment pop never
  fired, once, all match.
- `ndsRendererNativeStageTask36EnsureWorld` executes **0 times** and
  `ndsRendererExecuteNativeFighterOwnerHierarchy` **0 times**, so no live path
  was pushing. `gNdsRendererTask36CaptureSegmentMask` reads **161 = 0xA1 =
  segments 0, 5 and 7** — three replayed segments, three leaked levels a frame.

### 1.3 The fix

`src/nds/nds_renderer.c:6218` (`ndsRendererTask36ReplayCaptureEndRun`):

```c
-        run->local_pushed = (owner->capture_push_balance != 0) ? TRUE : FALSE;
+        run->local_pushed =
+            (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE) ?
+                TRUE : FALSE;
```

`EnsureWorld` runs *inside* the capture bracket, so the live flag is already the
state the stream leaves. The `capture_push_balance` 0..1 sanity guard at `:6207`
is untouched. It cannot underflow: the flag is TRUE only where a push is
genuinely outstanding.

### 1.4 Proof

`build-c183-gxstackfix`, same flags as `c173`.

| arm | window | stack level | error bit | advance/frame |
|---|---|---|---|---|
| `c173` control | 128 frames, 438..565 | 28,31,2,5,... mod 32 | **1 / 128** | **+3.000** |
| `c183` fixed | 128 frames, 438..565 | **0 on 128 of 128** | **0 / 128** | **0.0000** |
| `c183` fixed | 17 ring stops, frames 534..2038 | **0 at every stop** | **0 at every stop** | — |

Whole-match end-of-run invariants on `c183`, against the `c170`/`c174`/`c175`/
`c176`/`c181` bank: `gNdsBattleTextHudP1Damage` **76**,
`gNdsDamageSparkScaleCount` **15**, `gNdsShieldAnimJointAttachCount` **1,352**,
`gNdsAObjEvent32NormalizedHighWater` **1,266**, `gNdsBattlePackHits` **197**,
`gNdsObjAnimRunawayCount` **0** — **all equal**. Polygon RAM min/median/max
432/465/510 over the 128-frame window; no low-polygon frames.

### 1.5 And the whole repo history corroborates it, for free

`battle_playable_realtime`'s pacing smoke prints GXSTAT. **Every Boundary log in
`artifacts/` from 2026-08-03 to 2026-08-15 reads `gxstat=0x6009600`** — position
/vector stack level **22**, error bit **set** — across 15 runs, six different
arms, and both `NDS_R2_PATH` states:

```text
2026-08-03 realtime_attrib_1sheet      0x6009300   (level 19, error set)
2026-08-03 realtime_atlas4sheet        0x6009600   (level 22, error set)
2026-08-13 c-threeleg / c-r2path-recheck / c-anim-anomalies   0x6009600
2026-08-14 boundary-after-dldi-reset   0x6009600
2026-08-15 cfx-ring-wiring / k0-rerank / k1-owner-pricing /
           battlepack-arena-price / pacing-publication (6 arms)  0x6009600
2026-08-15 THIS CYCLE, after the fix   0x6000000   (level 0, error CLEAR)
```

The leak was on the **shipping-default Boundary arm** (`BOTH_CPU 0`,
published-block flags) the whole time, not only on the gate arm, and the repair
clears it there too. `artifacts/task72-verify-boundary.log` reads `0x6000000`,
so the defect post-dates that run.

**Not quotable as an A/B.** `c183` whole match reads `WORK-H` P50 947,520 / P95
1,187,648, and `c173` was never run whole-match, so there is no within-pair
delta here. The cross-build floor is >=14,080 with unreliable sign
(`docs/VERIFYING.md`), and the fix adds 3 `MATRIX_POP` FIFO words a frame.
**`NDS_R2_FIGHTER_GX_COMPOSE`'s -13,632 is still un-re-measured and must not be
re-banked** (its baseline was 1,258,112 against today's 1,177,344); the flag was
not flipped and stays `?= 0`.

---

## 2. Task B — the asset-I/O lane is Mario's animation, and it converts 0.30x

Basis: the c181 v3 capture already on disk
(`../2026-08-15_cfx-narrow-exchange/v3-b-c181/`, 1,600 regions, gate arm,
`BOTH_CPU=1` / `DRAW=0` / `BATTLEPACK=1`), read with
`scripts/census-profile-pc-per-region.py`. **Zero emulator runs were needed.**

### 2.1 The producer, by exact call counts

| symbol | calls/match | per marginal frame | per body frame | presence |
|---|---:|---:|---:|---:|
| `_FAT_read_r` | 3,790 | **43.11** | 0.2243 | **192.2x** |
| `move_window` | 106,276 | 622.74 | 37.14 | 16.8x |
| `get_fat.isra.0` | 102,305 | 579.53 | 36.80 | 15.7x |
| `armCopyMem32` = `_dvmCacheCopy` | 1,986 | 10.28 | 0.766 | 13.4x |
| `_dvmDiscCacheReadWrite` | 1,904 | 9.64 | 0.745 | 12.9x |
| `f_lseek` = `_FAT_seek_r` | 217 | 1.375 | 0.0704 | 19.5x |
| `ndsRelocAssetLoadIntoZeroedHeap` | **7** | 0.0875 | **0.0** | inf |
| `ndsRelocApplyWordByteSwap` | **7** | 0.0875 | **0.0** | inf |
| `ndsRelocAssetReadHeaderFromFile` | **7** | 0.0875 | **0.0** | inf |
| `ndsAudioBgmReadPacket` | **76** | 0.0500 | 0.0474 | **1.056x** |
| `ndsAudioBgmReadExact` | 152 | 0.100 | 0.0947 | 1.056x |
| `ndsR2AnimCachePreloadStep` | 3,200 | 2.000 | 2.000 | **1.000x** |
| `_ntrcardRomReadSector` | **0** | — | — | — |

### 2.2 It is seven frames

Seven regions — **971, 1029, 1103, 1193, 1202, 1264, 1441** — each carry
337..614 `_FAT_read_r` calls and exactly one `ndsRelocAssetReadHeaderFromFile`.
Together they hold **3,238 of 3,790 (85.4%) of the match's FAT reads**, and
**3,238 of the 3,449 (93.9%) that land on the marginal 80**. The next 24 regions
carry 5..15 each; 31 regions hold 90% of the whole match's reads.

Share of each row that lands on those 7 frames, against the **8.75% baseline**
(7 of 80 marginal frames):

```text
_FAT_read_r                     93.9%   <- the load itself
ndsRelocNormalizeFighterAObj16File 34.1% = ndsRelocAssetFindEntry = sniprintf
f_lseek                         30.0%
get_fat                         20.7%   armCopyMem32 19.7%   _dvmDiscCacheReadWrite 20.6%
ndsRelocAssetIDForToken         19.0%
memcpy                          14.9%
memset                           9.2%   <- baseline: NOT asset I/O at all
ndsAudioBgm*                     0.0%
every draw-side symbol       8.8-9.0%   <- baseline, i.e. these are ordinary frames + a load
```

### 2.3 What it is worth, and the retraction

The 7 load frames sit at ranks **3, 5, 10, 13, 14, 16, 23** of 1,600 with
1,419,702..1,697,805 ticks against a rank-80 of **1,194,342**.

```text
delete all 7 frames outright  ->  rank-80 1,194,342 -> 1,184,479   gain 9,863 = 0.30x
```

That is the **ceiling**, and it is reachable-ish (removing the load work drops
each of those frames by ~230..500K, i.e. below the new rank-80). Their excess
over the new rank-80 is 2,420,612 ticks, which is **30,258 tk/fr of the 80-frame
mask mean** — so the mask mean overstates this lane's percentile value by
**3.07x**.

> **RETRACTION.** `MENU.md` §6.1 and `docs/HANDOFF.md` head both present this
> lane as **"100,689 tk/fr, +67,454 excess, 3.1x the requirement"**. The mask
> mean is right; **the conversion is not**. A lane at 15..19x concentration
> living on 7 frames above rank-80 converts at **0.30x**, not 3.1x — the exact
> mirror of the correction §7/`MENU` §6.2 made for the flat draw side. This also
> independently reproduces the 2026-08-14 result recorded in `HANDOFF.md`
> ("deleting those 13 frames ENTIRELY moves the 80th-largest frame **9,874**"):
> different capture, different tree, **9,863**.

### 2.4 The RAM position on packing Mario, stated on this tree

`plan.md` §K-POOL: both fighters need **~559,632 B** against **~301,564 B**
available; §K-RAM: the second pack needs **+258,048 B** against an optimistic
ceiling of 248,256, and the 2026-08-15 attempt was granted only **188,416** of
it — the arena reservation still succeeded, `general heap free` hit **6,076**
against the 32,768 floor and **the battle never started**. Nothing in this cycle
changed that. **Packing Mario is a ~9,863 tk/fr lever behind a RAM problem that
has already burned one gate run and one soak.** It should be ranked accordingly,
not as the board's largest item.

---

## 3. Task B rider — `armCopyMem32`

Same work, one producer, billed at three layers:
`_FAT_read_r` -> `_dvmDiscCacheReadWrite` (1,904) -> `_dvmCacheCopy` (1,986) ->
`armCopyMem32` (1,986). `_ntrcardRomReadSector` executes **0** times in the
window, so the `ntrcardRomRead` referrer the ELF shows is not live during
gameplay: every cartridge byte arrives through the disc cache. **Not a separate
producer; do not add its 15,791 tk/fr to the FatFs rows as if it were.**

---

## 4. Task C — the draw lane, enumerated

Basis: rank-80 symbol census `../2026-08-15_cfx-ring-draw0/b-c179-pc.csv`
(the MENU basis), `--marginal 80`, mask `>= 1,171,083`; call counts from the
c181 v3 capture (identical draw code). `marginal80-c179-top90.txt`.

| part | tk/fr | conc | measured rate |
|---|---:|---:|---|
| **Fighter draw kernel** (`MarioFoxDLAllDrawForSlot` 30,241 · `ExecuteNativeFighterOwnerProduction` 24,514 · `NativeEmitProductionPrimitiveGroups` 26,485 · `NativePrepareProductionRun` 20,316 · `NativeShadeProductionActions` 7,079 · `NativeEmitProductionCrossRun` 4,651) | **113,286** | 1.02-1.03x | 1.975 fighter draws, 66.25 production runs, 53.5 primitive groups per marginal frame |
| **Matrix compose + load** (`MtxMulAffine20p12` 18,909 · `AdapterBuildDObjXObjMatrix` 12,811 · `AdapterBuildFighterTraRotRpyDirect20p12` 12,554 · `MtxMul20p12` 10,611 · `LoadHardwareSplitMatrices` 10,405 · `AdapterBuildPersistentStageWorldMatrix` 9,332 · `LoadHardwareMatrixPair` 5,319 · `AdapterBuildDObjLocalMatrix` 5,137) | **85,078** | ~1.05x | 53.86 affine muls, 31.69 split-matrix loads per marginal frame |
| **Stage segment/run scaffolding** (`CommitNativeStageSegment` 27,880 · `NativeStageBeginRun.part.0` 16,448 · `Task36ReplayRun` 5,208 · `HardwareEndBatch.part.0` 3,582) | **53,118** | 1.00x | **8.00 segments, 54.00 runs, 68.71 batch ends** per frame, every frame |
| **Material / texture state** (`SyncTextureTile` 8,867 · `HardwareResolveOrBindTexture` 7,500 · `HardwareBindTextureName` 7,464 · `NativeApplyStateDelta` 6,960 · `glBindTexture` 6,404 · `NativeApplyMaterial.part.0` 5,664) | **42,859** | ~1.0x | |
| **Stage no-Z band** (`LoadNoZMatrix` 12,282 · `EmitNoZTriangle` 5,200 · `EmitNoZVertex` 5,126) | **22,608** | **1.00x** | **47.00 / 27.00 / 81.00** per frame, every frame |
| **Draw-list walk / adapters** (`ndsStageGCDrawAllLoopRecordCapturedDisplay` 9,616 · `AdapterPrepareNativeStageOwner` 5,855 · `AdapterMarkDisplayProcHeads` 4,008) | **19,479** | ~1.0x | |
| **Particle draw** (`lbParticleDrawTextures`) | **11,840** | 1.19x | |
| **total** | **348,268** | **~1.02x** | **10.7x the requirement** |

**The FIFO itself is ~27,467 tk/fr of `bus_contention`** and is a hardware
floor: `NativeEmitProductionPrimitiveGroups` 10,287 (with **0 icache** — pure GX
writes) · `LoadHardwareSplitMatrices` 3,771 · `CommitNativeStageSegment` 3,192 ·
`LoadNoZMatrix` 2,632 · `LoadHardwareMatrixPair` 2,063 ·
`NativeStageBeginRun` 1,988 · `EmitProductionCrossRun` 1,730 ·
`EmitNoZVertex` 1,082 · `HardwareEndBatch` 722. Deleting it means deleting
geometry, which is category 2 or 3, not category 1.

### 4.1 The stage's per-frame draw is now known exactly, from the generated tables

`src/nds/nds_native_stage_owner.generated.inc` is static, so the whole live no-Z
sequence was reconstructed offline and **reproduces the measured counters to the
unit**: segments **1, 2, 3 and 6** emit **27 triangles / 47 matrix loads / 81
vertices**, which is the only subset of the 8 segments that does (segments 0, 5
and 7 are the replayed ones, `CaptureSegmentMask 0xA1`; segment 4 emits no no-Z
triangles).

```text
27 triangles = 17 single-binding (1 LoadNoZMatrix each) + 10 multi-binding (3 each)
47 loads use 15 distinct binding_index values; coordinate_shift is 0 for ALL of them
```

### 4.2 Category 1 (pure waste), sized

- **5 of the 47 no-Z matrix loads per frame are bit-identical repeats** — same
  binding, same shift, and the same `projected_z` because they sit inside one
  triangle. At the measured 261 tk/call (12,282 / 47) that is **1,305 tk/fr =
  0.04x the requirement.** Exact, and small. The one-line guard is a compare
  against the previous `(binding_index, coordinate_shift)` inside
  `ndsRendererNativeStageEmitNoZTriangle`'s per-vertex loop (`:31219-31223`).
- **32 of the 47 (68.1%) rebuild a base matrix already composed this frame**,
  but `projected_z` advances per triangle
  (`ndsRendererHardwareNextProjectedDepth()`, `:32473`), so only
  `ndsRendererBuildRawHardwareMatrix` (a 64 B copy plus four rounded shifts) is
  removable — `SetNoZColumn`, the second 64 B copy and the 16-word GX load must
  still run. **Do not size this at 32/47 of 12,282.**

**Honest state: no category-1 item in the draw lane has been sized above
~1,300 tk/fr, and I am not going to invent one.** What the enumeration *does*
establish is where to point the next measurement. Every draw-side row now has an
exact call rate (`perregion3-c181.csv`, entry-PC counts, marginal-80 basis),
which is what a candidate has to be argued against:

| symbol | rank-80 tk/fr | calls / marginal frame | tk/call | presence |
|---|---:|---:|---:|---:|
| `ndsRendererNativeApplyStateDelta` | 6,960 | **194.45** | 36 | 1.03x |
| `ndsRendererHardwareBindTextureName` | 7,464 | **103.45** | 72 | 1.01x |
| `ndsRendererSyncTextureTile` | 8,867 | **72.68** | 122 | 1.02x |
| `ndsRendererAdapterBuildDObjXObjMatrix` | 12,811 | 57.05 | 225 | 1.04x |
| `ndsRendererAdapterBuildDObjLocalMatrix` | 5,137 | 57.14 | 90 | 1.04x |
| `glBindTexture` | 6,404 | 55.73 | 115 | 1.01x |
| `ndsRendererNativeShadeProductionActions` | 7,079 | 48.55 | 146 | 1.03x |
| `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` | 12,554 | 31.78 | 395 | 0.83x |
| `ndsRendererNativeApplyMaterial.part.0` | 5,664 | 29.71 | 191 | 1.03x |
| `ndsStageGCDrawAllLoopRecordCapturedDisplay` | 9,616 | 27.44 | 350 | 1.00x |
| `ndsRendererAdapterMarkDisplayProcHeads` | 4,008 | 27.44 | 146 | 1.00x |
| **`ndsRendererTask36ReplayRun`** | 5,208 | **33.00** | 158 | **1.000x** |
| `ndsRendererLoadHardwareMatrixPair.constprop.0` | 5,319 | 19.26 | 276 | 1.01x |
| `ndsRendererMtxMul20p12` | 10,611 | 18.48 | 574 | 1.00x |
| `ndsRendererAdapterBuildPersistentStageWorldMatrix` | 9,332 | 16.26 | 574 | **1.00x** |
| `ndsRendererNativeEmitProductionCrossRun` | 4,651 | 12.78 | 364 | 1.04x |
| `lbParticleDrawTextures` | 11,840 | 4.00 | 2,960 | 1.00x |
| `ndsRendererAdapterPrepareNativeStageOwner` | 5,855 | 1.00 | 5,855 | 1.00x |
| `ndsRendererHardwareResolveOrBindTexture` | 7,500 | **0.175** | **42,857** | 1.64x |

`ndsRendererTask36ReplayRun` at **exactly 33.00/frame** is the third independent
confirmation of the segment split: segments 0, 5 and 7 hold 26 + 3 + 4 = **33
runs**, and 54 − 33 = 21 live runs across segments 1, 2, 3, 4 and 6.

Two candidates were checked and are **not** clean category-1 items:

- `ndsRendererAdapterBuildPersistentStageWorldMatrix` **already carries a
  per-frame cache** (`validated_frame == frame`, plus slice 44's
  `NDS_R2_STAGE_VALIDATE_STRIDE 8` stale-reuse path with its own
  `gNdsR2Slice44StaleReuse` counter, `reloc_backend_renderer_dl.c:2877-2897`).
  16.26 calls/frame is what remains *after* memoisation; the open question is the
  hit rate, not whether a memo exists. Read `gNdsR2Slice44StaleReuse` and the
  `m2_world_matrix_cache_hit_count` group before proposing anything.
- `ndsRendererCommitNativeStageSegment` (27,880, 8.00 calls/frame, 2,572 B,
  **15,910 tk/fr icache = 1,989 tk/call of pure instruction fetch**) is
  **footprint, not waste** — the body is cold on every one of the 8 consecutive
  calls. `K-EXCHANGE` has already refuted layout as a lever (ceiling +219 tk/fr).
  **Do not re-open it as placement.**

The two rows that still look like unmeasured "recomputed unchanged" and are
worth one counter each:

| candidate | tk/fr | shape | engagement counter to land first |
|---|---:|---|---|
| `ndsRendererSyncTextureTile` | 8,867 | **72.68 VRAM tile syncs per frame, every frame**, 4,943 tk/fr of `write_buffer`; Dream Land's stage textures do not change during a match | syncs/frame **and** syncs whose source bytes are unchanged since that tile's last sync |
| `ndsRendererHardwareBindTextureName` -> `glBindTexture` | 7,464 + 6,404 | **103.45 bind requests a frame collapse to 55.73 GX binds** — the tracker already elides 46%; the residual 55.73 is the question | binds/frame split by "same name as the currently bound one" vs a genuine change, per frame |

**The counter discipline that applies (plan.md §9 law 1):** publish it from code
(header + `diagnostics.c`, `__attribute__((used))`, `nm`-verified against
`--gc-sections` — devkitARM ignores `retain`, so a diagnostic global with no
compiled writer is dropped and reads as a deleted symbol, which
`sample-tick-hud-buckets.ps1:359` now refuses on), on the **gate arm**, before
any code change.

### 4.3 One thing this enumeration retracts

`MENU.md` §6.2's draw side is **170,953 tk/fr over 7 symbols**. The lane as
actually enumerated is **348,268 over 36**, still at ~1.02x. The menu's figure
is a floor, not the lane; the correction in `plan.md` §7 ("a 1.00x lane converts
1:1, so this is the largest lane on the board") is therefore **understated by
2.0x**, not overstated.

---

## 5. Method notes worth keeping

1. **A 3.7 GB v3 capture answers "which frames" for free.**
   `census-profile-pc-per-region.py` turned Task B from "one gate run" into a
   60-second host scan, and it is what made the 7-frame structure visible at
   all. Two scans covered Tasks B and C.
2. **The linked ELF plus the per-PC census is an exact push/pop ledger.**
   Disassembling for stores to `0x04000444`/`0x04000448` and reading each store
   PC's `all_instructions` out of the reduced census gives the exact number of
   MATRIX_PUSH and MATRIX_POP the ARM9 executed in a match. It closed to zero
   and that is what promoted the DMA'd replay stream from a suspect to the
   answer.
3. **A per-frame counter that stalls is a free negative control.**
   `gNdsParticleBatchOpens` flattening mid-window exonerated the whole
   particle/Whispy path without a second run.
4. **`-PerFrameGlobals` on `gNdsHardwareRendererStatus` reads live.** The global
   shares a 32-byte line with `gNdsHardwareRendererFlushCount` (a `++`, so the
   line is loaded and dirty), and the reads still tracked +3/frame with only one
   stale sample in 128. Whole-run `-PerStopGlobals` was clean at all 17 stops.
