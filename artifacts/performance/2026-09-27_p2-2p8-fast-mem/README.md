# P2-2p8: memcpy/memset in ARM, FGM id map, stage matrix leaves (2026-09-27)

**Outcome: four exact changes BANKED; two levers priced and refuted.**
All arms: four-CPU stress (Donkey/Samus/Link/Kirby, Dream Land, items on),
route 1, frames 2..1973, `build-p2-fourcpu-tickhud`, HEAD `7e47e1f21e6` plus
the change. Every candidate's replay digest is IDENTICAL to the control
(`compare-replay-digest.py --sequence --resync 4`).

## Control

`rebase` `F712AF50`: the committed tree rebuilt (the committed `56AF4CCB` ROM
measures within noise: `animrd` 1,257,472/1,725,376 vs 1,258,688/1,727,296,
digest identical). WORK-H P50/P95/P99 1,258,688 / 1,727,296 / 2,127,488;
VBlanks 2/3/4/5+ 422/1,420/115/16; 20.97 FPS.

## Where it came from

A fresh per-PC profile of `7e47e1f21e6` (`builds/p2p8-profile-7e47`, 385
frames, local) ranked by symbol, by P90-99 vs P40-60 frame excess, and by
dynamic call-site counts (the BL instruction's own execution count):

- `memcpy` 33.8K + `memset` 15.7K ticks/frame: newlib's generic C members,
  compiled -Os Thumb, which Task 37 moved into ITCM unchanged. One LDR/STR per
  word, 16 bytes a loop (1,036 loop trips a frame). 252 memcpy calls a frame,
  173 of them the stage GX patcher's 64-byte matrix copies.
- `ndsRendererMtxMul20p12` 16.4K: 27 stage bindings a frame x ~1,100 cycles; the
  rolled k loop reloaded both operands per product.
- `ndsRendererBuildShiftedRawHardwareMatrix` 11.6K: Thumb, s64 shift and range
  compare per cell.
- `ndsAudioFgmFindEntry` ~2.4K mean, far more on sound bursts: a linear scan of
  573 32-byte entries (one cache line each) per cue start and per live handle.

## Changes (each measured on its own, cumulative)

| arm | ROM | change | WORK-H P50 / P95 / P99 | 2-VBlank |
|---|---|---|---|---:|
| rebase | F712AF50 | control | 1,258,688 / 1,727,296 / 2,127,488 | 422 |
| fgmmap | DE4EC439 | FGM id -> entry map (`nds_audio_fgm.c`) | 1,253,312 / 1,721,856 / 2,136,384 | 435 |
| mtxmul | 4F22CCFC | unrolled 4x4 multiply, lhs row in registers (`nds_renderer_dl_core.c`) | 1,248,896 / 1,717,312 / 2,134,848 | 460 |
| shift32 | 3D8F364D | 32-bit range check for the shifted raw matrix (`nds_renderer_textures_effects.c`) | 1,242,112 / 1,710,912 / 2,129,216 | 484 |
| fastmem | F6CE3DAB | ARM LDM/STM memcpy + memset in ITCM (`nds_fast_mem.c`, `NDS_FAST_MEM`) | **1,215,616 / 1,657,152 / 2,056,192** | **556** |

Net: P50 -43,072, P95 -70,144, P99 -71,296; VBlanks 2/3/4/5+ 556/1,315/90/12,
21.61 FPS. STG P50 241,024 -> 208,128; AUD P95 18,432 -> 12,224.

- **FGM id map.** `sNdsAudioFgmIdMap[688]` (u16 index+1, 1,376 B BSS) built when
  the pack loads, dropped on reset; ids are unique, so it names the scan's own
  first match. Ids past the map, or a pack that never loaded, still scan.
- **MtxMul20p12.** Same products, same s64 sums (two's-complement addition
  associates), same round/clamp per cell; only operand reuse changed.
- **Shifted raw matrix.** For shift < 32, `v << s` fits an s32 exactly when
  `(v << s) >> s == v`; shifts >= 32 keep the s64 form.
- **memcpy/memset.** Same contracts (return dst). 32-byte LDM/STM when both
  pointers are word aligned, word loop, then bytes; a misaligned pair copies
  bytewise as newlib's did. Linked instead of the Task 37 libc members
  (`NDS_TASK37_LIBC_MEMBERS` keeps memcmp only when `NDS_FAST_MEM=1`). ITCM
  30,776 -> 30,728 B. Tick-HUD builds run `ndsFastMemSelfTest` at boot: sizes
  0..100 x source/destination offsets 0..3, guard bytes both sides, return
  value: `gNdsFastMemSelfTestRuns` 3,232, `gNdsFastMemSelfTestFailures` 0.

## Refuted (reverted, not banked)

- **Hand-written cpuGetTiming** (`fasttime2` `0461A607`): ITCM literals instead
  of main-RAM pointers, no push/pop, 22 instructions. The profile priced the
  wrapper at 20.6K/frame (499 calls x ~95 cycles); WORK-H moved -1.6K/-0.5K,
  inside noise. Not banked (D9). Lesson: the profile's per-instruction cycles for
  I/O-bound leaf code did not transfer to the gate.
- **Bigger animation cache** (`keep48` `4F1B4948`, lab): keep-free 128 -> 48 KiB
  gave the ring arena 200,496 B (from 118,576). Motion reads 362 -> 275
  (each ~29.3K ticks: 10.6M -> 7.9M ticks a match) but WORK-H P95 moved only
  -2.7K. Motion reads are not a tail lever on this roster; A2's "0 motion reads
  after GO" remains a DONE item, not a performance one. `animctr`/`animrd` are
  the counter reads on the committed ROM (hits 339 / misses 367 / fills 363 /
  6 ring recycles; 362 stream reads, 717,016 B).

## Files

`<arm>{.json,-rows.csv,-run.log}` for every arm above.

## Section 2: memcmp and the flat-walk cache (same day, same control)

| arm | ROM | change | WORK-H P50 / P95 / P99 | 2-VBlank |
|---|---|---|---|---:|
| fastmem | F6CE3DAB | (section 1 result) | 1,215,616 / 1,657,152 / 2,056,192 | 556 |
| memcmp | 537A52A1 | ARM memcmp in ITCM; Task 37 libc extraction off while `NDS_FAST_MEM=1` | 1,205,632 / 1,658,880 / 2,047,488 | 585 |
| flatassoc | 6A6CF3CE | searched 4-entry flat-walk cache (`reloc_backend_compat_shims.c`) | **1,198,912 / 1,659,712 / 2,038,144** | **624** |

VBlanks 2/3/4/5+ 624/1,249/88/12, 21.89 FPS. P95 is flat across both (inside
the ~±7K single-run noise); P50 -16.7K, P99 -18K, SRC P50 -9K.

- **memcmp.** 129 calls a frame, all but two from the stage world source-key
  compare. Words while both pointers are aligned, then the first differing word
  (or the tail) bytewise, so the value is newlib's: the difference of the first
  differing bytes as unsigned chars. The boot self-test adds memcmp at seven
  positions both ways (29,088 cases, 0 failures). With all three members
  replaced, `NDS_TASK37_ITCM_LIBC` is 0 whenever `NDS_FAST_MEM` is 1.
- **Flat-walk cache.** `ndsFTParamsFlatWalkFor` was direct-mapped on
  `(ptr >> 4) & 3`; its steady keys are the four fighters' TopN joints
  (`ftmain.c:942`, per fighter per tick), and equally strided DObjs shared a
  slot, so the flatten walk re-ran on most calls (~12K/frame in the profile).
  Now four compares find a resident key; a miss takes a stale or empty slot,
  else the next in turn. Same table, same invalidation, same results.

## Section 3: Cycle 98's pre-walk census leaves the gate ROM (A9)

`prewalk` `5BF3DC64`: `NDS_FTR_PRE_WALK_CENSUS` (default 0) now gates the
Cycle 98 collection-identity census (a memset of a draw collection plus an FNV
hash per fighter draw, counters read only by a debugger) that every tick-HUD
frame still ran. Instrument only; the shipping ROM never had it. WORK-H
P50/P95/P99 1,198,912/1,659,712/2,038,144 -> 1,197,376/1,641,216/2,055,360
(P95 partly noise; FTR P50 -4.9K, P95 -5.6K); VBlanks 625/1,252/84/12, 21.91 FPS;
replay identical.

## Section 4: Phase 2 B3's per-GObj MISC split leaves the gate ROM (A9)

`miscsplit` `7F2123F3`: `NDS_P2_MISC_SPLIT` (default 0) gates the per-GObj
capture/proc-display spans in `gcCaptureCameraGObj` (four clock reads, two
nine-counter nested sums and a kind classification per displayed GObj). The
MCAP and MPRO columns read 0 and fold into MCAM; MWPN/MITM/MEFX/MACT/MPRT and
the top-level MISC bucket are measured as before. WORK-H P50/P95/P99
1,197,376/1,641,216/2,055,360 -> **1,182,784/1,622,336/2,024,704**; MISC P50
-13.6K; VBlanks 687/1,200/76/10, 22.22 FPS; replay identical.

Refuted (reverted): unrolling the replay digest's byte-wise FNV mix
(`digestunroll` `262BE55A`, identical digest words): P50 -1.7K, P95 +6.1K.

## Section 5: Cycle 86's per-fighter SRC split leaves the gate ROM (A9)

`srcsplit` `75F3EEF5`: `NDS_TICK_HUD_SRC_SPLIT` (default 0) gates the two
clock reads around each fighter proc every tick (SCAT/SHDT/SPRM/SINT/SPHD/SPHC
and SCPU). Those columns read 0 in gate runs; GCRA and SRC are measured as
before; analysis runs set the flag to 1. WORK-H P50/P95/P99
1,182,784/1,622,336/2,024,704 -> **1,174,720/1,614,528/2,017,664**; SRC P50
-8.7K; VBlanks/FPS 712 1175 75 11 22.32; replay identical.

## Section 6: pose clock through libgcc's adder; HUD divide-by-15

| arm | ROM | change | WORK-H P50 / P95 / P99 | 2-VBlank |
|---|---|---|---|---:|
| srcsplit | 75F3EEF5 | (section 5 result) | 1,174,720 / 1,614,528 / 2,017,664 | 712 |
| f32libgcc | 418FC19C | `ndsF32AddBits` = libgcc binary32 add on ARM9 (`nds_f32_exact.h`) | 1,165,248 / 1,613,568 / 2,008,832 | 740 |
| div15 | FC908A40 | HUD palette ramps divide by 15 via multiply-shift | **1,162,816 / 1,612,416 / 2,007,552** | **764** |

VBlanks/FPS after div15: 764 1129 70 10 22.57; every step replay identical.

- **Pose clock.** The integer binary32 adder (proved equal to the IEEE adder
  over 1.14 billion host operations, re-run today: 0 mismatches) was outlined
  by GCC at ~50 instructions in main RAM, ~300 calls a frame. On the ARM9 the
  header now adds the reinterpreted floats, i.e. libgcc's ITCM `__aeabi_fadd`
  -- the same IEEE round-to-nearest-even add; soft-float passes floats in core
  registers, so the reinterpretation is free. The host keeps the integer form
  and its proof. SRC P50 -7.9K, P95 -11.6K.
- **Divide by 15.** `(x * 34953) >> 19` equals `x / 15` for every x < 2^16
  (checked exhaustively; inputs are u8 channel x 15 + 7 <= 3,832). Thumb has no
  UMULL, so each `/ 15u` was a `__udivsi3` call (~55 a frame). HUD P50 -5K.

Refuted (reverted): dropping the DMA wait at the top of each stage GX run
(`nowait` `94BA349A`): patches never touch an in-flight span, but WORK-H
moved +1.5K/+1.8K -- the CPU stalls on the bus during the DMA anyway.

## Section 7: map-collision float inputs from the vertex cache

`mpf32` `A5A7644F`: the floor query's accepted segment uses the f32 vertex
cache (four O2R reads and six `__aeabi_i2f` calls gone; float-input
`ndsMPLineDistanceFCf`/`ndsMPGetFCAnglef`), and the wall sweep's ceil/floor
range bounds come from the float's bits (`ndsMPF32TruncFrac`, checked on the
host against the truncate-and-compare form over 2.4 million values, 0
mismatches) instead of an f2iz + i2f + compare per bound. Every (f32) of an s16
vertex is exact, so every operand and rounding is the integer form's. WORK-H
P50/P95/P99 1,162,816/1,612,416/2,007,552 -> 1,162,112/1,602,240/2,004,544 (P95
inside ~1.5x single-build noise; SRC P50 -2.6K, P95 -3.6K); VBlanks/FPS 757 1138 69 9 22.55;
replay identical.

## Section 8: stage billboard orientation memo; kind-46 trig; digest seam reads

Task 103 lab (`task103now`, `D95C74EA`): the stage prepare spends ~87K/frame
on matrices, 31.7K of it on 11 billboard MVP recalcs (~2.9K each; counters in
`k48ctr`: 12,432 kind-46 and ~9,900 other applies per match).

- **Orientation memo.** Kind 48's rows depend only on the camera's Mod1 and the
  two recalc scales; kind 46's on the camera perspective, rotate.z, both scales
  and the incoming gGCScaleX accumulator (the outgoing accumulator is stored
  too). Within one stage prepare, bindings with the same inputs now reuse the
  converted rows (float rows + syMatrixF2L + N64 conversion skipped). Static
  storage (the prepare's camera is on the DTCM stack), reset per prepare, only
  with the stage camera; the 0x4A roll-witness DObj always takes the full path.
  `gNdsMvpMemoEnable` (default 1) is the same-binary A/B word.
- **Kind 46 trig.** `syMatrixRotRpyRF` (six sin/cos evaluations) ran for kind
  46 too and its cos/sin were discarded; it now runs only in the custom-0x46
  arm that reads them. Pure function, same results.

Same-ROM A/B (`473CCE25`, `k46off` = BootSet `gNdsMvpMemoEnable=0`):
WORK-H P50/P95/P99 1,165,120/1,618,368/2,008,384 -> **1,157,440/1,609,728/
2,000,128**; STG P50 206,656 -> 199,104; VBlanks/FPS 777 1118 67 11 22.63; digest IDENTICAL.
Against `mpf32` (different layout): P50 -4.7K, P95 +7.5K.

**Digest tool.** `k46` against the control first read DIVERGED at sample 44:
the pre-GO load wait ended a frame sooner, and `--resync` could not realign
because three samples (every 384, the sampler's ring stops) carried a DGSB read
across the stop -- DGSA equal, DGSB different, the next sample equal in full.
`compare-replay-digest.py --resync` now accepts exactly that shape (a tick-B
state feeds the next tick A, so a real difference cannot heal one tick later)
and reports it as seam tick-B words: `k46` reads IDENTICAL AFTER ONE RESYNC
(shift +1, 3 seam tick-B words).

## 9. Texture-pool witness sampled on key generation (A9)

`ndsRendererRecordTextureKeyPoolUse` (tick-HUD only) scanned all ~79 dynamic
texture-cache entries every frame to keep `gNdsRendererTextureKeyPoolEntriesHighWater`.
Every insert and release stamps `sNdsRendererHardwareTextureKeyGeneration`, so
the entry count can only change on a frame where the generation moved. The scan
now runs on those frames, plus every 16th frame as a backstop. The high-water
stays exact (123 over the match); nothing in the gate build reads it.

`B9AF6D9D` against `473CCE25` (`k46on`), replay identical over 1,972 samples:
WORK-H P50/P95/P99 1,157,440/1,609,728/2,000,128 -> **1,152,192/1,606,592/
2,002,496**; OTHR P50 359,296 -> 353,536; VBlanks/FPS 777 1118 67 11 22.63 ->
792 1106 64 11 22.71. Evidence `texpool.*`.

## 10. Replay digest mixes one word per multiply (A9)

The tick-HUD replay digest (instrument only, two ticks per presented frame)
folded each word byte by byte (FNV-1a: four xor/multiply steps). It now folds a
word in one step: `h = (h ^ v) * 16777619; h ^= h >> 15`. Each step is a
bijection in both the hash and the word, so a single differing word still
changes the digest. The xorshift moves high-bit differences, such as a float's
sign, into lower bits, so two of them cannot cancel in the top bit.
`ndsReplayDigestMixVec3` is now inline. Digest words change, so later A/Bs
use `wmixf` as the control.

Same-ROM A/B (`80BB8AF4`, lab word `gNdsReplayDigestWordMix`):
- `wmix0` (byte-wise) replays IDENTICAL to `texpool`, so the lab ROM's gameplay
  is unchanged.
- `wmix1` (word-wise): paired WORK-H median -3,392 (mean -3,473); SRC/STG/FTR/
  MISC deltas under 100.

The lab word was then deleted. Final `7C2EF9D0` (`wmixf`) replays IDENTICAL to
`wmix1`, so it is the measured candidate. WORK-H P50/P95/P99 against `texpool`:
1,152,192/1,606,592/2,002,496 -> **1,148,416/1,599,104/2,000,384**. VBlanks
804/1,095/64/10. Native failures 0; heap low-water unchanged.

## 11. Lean per-draw phase clocks leave the gate ROM (A9)

The lean fighter path timed each draw's phases for gNdsFtrLean: head, guard
and its parts, kernel, patch and its parts, book, and submit and its parts.
That was about 30 clock reads per fighter per frame, plus the counter updates.
The `NDS_FTR_LEAN_PHASE_TICKS` Makefile flag (default 0) now controls these
clocks. With the flag at 0, `NDS_FTR_LEAN_CLOCK()` returns 0 and the phase
counters are not written. They are compiled in only when the flag is 1 or in
an `NDS_FTR_LEAN_KTIME` attribution build. Event and materialize ticks stay,
because those events are rare. The ship image also loses its unused
`cpuGetTiming()` calls for `t0`, `t1` and the head.

Same-ROM A/B (`4E02C85C`, a lab word gating only the clock reads; counters
written in both arms):
- `lclk0` (clocks off) vs `lclk1`: paired WORK-H median -4,864; FTR -4,608.
- Both arms replay IDENTICAL to `wmixf`.

The lab word was then deleted. Final `887951B9` (`lclkf`, clocks and counter
writes compiled out) replays IDENTICAL to `wmixf`:
- WORK-H P50/P95/P99 1,148,416/1,599,104/2,000,384 -> **1,134,016/1,583,744/
  1,961,536**; paired median -14,080 (FTR -9,088; cross-build).
- VBlanks 858/1,044/64/7.
- Lean draws 6,862, materializations 62, native failures 0.

## 12. STG span on committed segments only (A9)

`ndsStageGCDrawAllLoopRecordCapturedDisplay` wrapped every display GObj of the
stage camera (~35 a frame) in two clock reads. Only the stage's own segments
commit. The tick-HUD STG span is now taken inside
`ndsRendererAdapterCommitNativeStageDisplay`, on the matched segment only.
Task 103's E3 fork and the level-1 profile keep the old wrapper. The segment
lookup for a display is no longer in STG. It falls in MISC's window, which
subtracts STG: STG P50 -5.3K, MISC +3.1K.

Same-ROM A/B (`C8563B9C`, lab word): paired WORK-H median -1,792. Both arms
replay IDENTICAL to `lclkf`. Final `FE3AF61F` (`stgf`) also replays IDENTICAL
to `lclkf`. Against `lclkf`, paired WORK-H median is -2,112. WORK-H P50/P95/P99
is **1,131,008/1,584,000/1,984,128**; P95 is flat (+256) and P99 is within
run-to-run spread. VBlanks 862/1,045/59/7.

## 13. Fresh profile (`4922D3C5`), WallSweep into ITCM, two refuted stage cuts

A new whole-match profile at `4922d3c5d94` (`profile-4922-census.txt`;
384 frames from frame 200) re-ranked the frame. Per frame, in ticks:
- Soft-float: ~92K (`__aeabi_fadd` 50K, 1,806 calls). The callers are source
  gameplay floats (pose clock, collision, AI), a closed lane.
- Materialization: ~90K excess in tail frames.
- `ndsStageGxDraw`: 52.6K. Dream Land's program is 54 runs, 7,009 words and
  297 patches a frame; 146 of the patches are painter matrices.

The census places `.itcm` at 30,160 of 32,736 B: 2,576 B are free since the
fast libc left ITCM. Its admission table ranks
`ndsStageMPAdjustFloorLoopWallSweep` (1,644 B, ~8.9K ticks/frame of non-memory
stall) first among the main-RAM functions that fit.

**WallSweep into ITCM (banked).** `itcmsw` `63784792` against `headchk`
(`29C70289`, HEAD rebuilt), replay IDENTICAL:
- WORK-H P50/P95/P99 1,131,712/1,582,912/1,961,792 -> **1,125,376/1,568,128/
  1,964,096**.
- Paired median -6,720; SRC -8,512. The layout moved STG +1.2K and FTR +0.6K.
- Two-VBlank frames 863 -> 895. ITCM now 31,800 B (936 B free).

**Refuted: stage DMA wait moved to the first GX write** (`sgw0`/`sgw1`, same
ROM). `ndsStageGxDraw` patches words only in main RAM before `ndsStageGxAppend`
writes GX, so the fighter-packet wait at its top was moved down. Paired WORK-H
-128, which is no effect: the wait moves into Append. Reverted.

**Refuted: ARM-state painter Z column** (`noz0`/`noz1`, same ROM). SMULL
replaced Thumb-1's 16-bit-partial s64 multiply in the 146 painter matrices a
frame. The rounding was host-checked (2e8 cases, 0 mismatches). Paired WORK-H
went +1,984 (STG +1,920): the cost is the stores into the DMA buffer, not the
arithmetic. Reverted.

## 14. ITCM residency swap: old fighter matrix builder out, MP floor + pose update in

`ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` (2,056 B of ITCM) serves
only the old fighter path: route 0 and declined lean draws. The four-CPU census
never executed it. It moves to main RAM. Three census admissions take its room:

| Function | Bytes | Non-memory stall in main RAM (ticks/frame) |
|---|---|---|
| `mpCollisionGetFCCommonFloor` | 1,484 | ~7.1K |
| `ndsFtPoseUpdate` | 1,136 | ~4.2K |
| `ndsMPLineExtentRejects` | 252 | ~1.3K |

`ndsMPLineExtentRejects` left ITCM on 09-09 to fix a 56-byte link overflow.
ITCM is now 32,616 of 32,736 B.

`itcm2` `FA8BE821` against `itcmsw`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,125,376/1,568,128/1,964,096 -> **1,112,768/1,551,168/
  1,964,864**. The median is now under 1,120,000.
- Paired median -12,608 (SRC -12,544, FTR +832).
- VBlanks 936/973/58/6. Native failures 0.

Placement-only: no instruction changes. The MP live-link checker cannot run on
this ELF: it reports `__deregister_frame_info`, which comes from the build
config, not from this move. It names `mpCollisionGetFCCommonFloor` only as an
API symbol.

## 15. ITCM round 3: two idle old-path residents out, six small admissions in

Out: `ndsRendererLoadHardwareGxComposedMatrices` (668 B) and
`ndsRendererNativeStageEmitNoZVertex` (176 B). Both are old-path residents that
the four-CPU census never executed.

In, 822 B, together ~6.9K ticks/frame of census non-memory stall in main RAM:

| Function | Bytes | Stall (ticks/frame) |
|---|---|---|
| `ndsRendererBuildShiftedRawHardwareMatrix` | 284 | 1.9K |
| `ndsR2CamDiv64` | 68 | 1.3K |
| `ndsFighterStructIsTrackedPointer` | 88 | 1.1K |
| `ndsRendererAdapterStageWorldSourceKeyMatches` | 222 | 1.0K |
| `ftParamsUpdateFighterPartsTransform` | 88 | 0.6K |
| `ndsRendererAdapterDirectMvpRecalcKind` | 72 | 0.45K |

`ndsRendererAdapterStageWorldSourceKeyMatches` left ITCM on 09-07 to fix a
16-byte link overflow. ITCM is now 32,600 of 32,736 B.

`itcm3` `AD8E0F00` against `itcm2`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,112,768/1,551,168/1,964,864 -> **1,111,040/1,547,776/
  1,951,552**.
- Paired median -3,648: SRC -3.9K and STG -1.7K, offset by FTR +1.1K of
  layout drift.
- VBlanks 952/963/52/6.

## 16. ITCM round 4: pre-pose-engine anim residents out, `ndsFtPosePlay` in

Out, 1,578 B that the four-CPU census never executed, or ran ~2 ticks a frame:
- `ndsR2AnimValueQ` (1,076 B): `NDS_R2_ANIM_Q_ITCM_ON` now defaults to 0,
  because the P2 pose engine evaluates every fighter joint.
- `ndsR2AnimTargetValue` (240 B) and `ndsR2AnimBuildTrackTable` (72 B).
- `ndsFTParamsInvalidateFighterParts` (54 B), the flat walk's recursive fallback.
- The out-of-line `ndsFtrLeanActive` (136 B); hot callers inline it.

In: `ndsFtPosePlay` (1,592 B, ARM). The census charged it ~5.7K ticks/frame of
non-memory stall in main RAM. `gcPlayDObjAnimJoint` (612 B, 12 ticks/frame
here) stays: stages with animated parts use it.

`itcm4` `C2259F24` against `itcm3`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,111,040/1,547,776/1,951,552 -> **1,102,720/1,538,624/
  1,955,136**.
- Paired median -8,512: SRC -4.2K, STG -2.5K, MISC -0.9K, FTR -0.7K.
- VBlanks 976/944/47/6. ITCM is 32,608 of 32,736 B.

## 17. Refuted: the lean kernel's half sine table in ITCM

The 2 KB half copy of `gSYSinTable` (`NDS_FTR_LEAN_SIN_ITCM=1`) needed room, so
these left ITCM:
- `ndsRendererNativeApplyStateDelta`, `ndsRendererAdapterBuildDObjLocalMatrix`,
  `ndsRendererR2ClampDiffuseToMaterial` and `ndsRendererRecordTextureState`.
  The census gave all four together ~0.55K ticks/frame of rent.
- `_arm_addsubsf3.o`, whose fadd/fsub are dead Task 16 goldens but whose l2f and
  ui2f run.

`sinitcm` `F22816AE` against `itcm4`: FTR paired -5.1K, as slice 5 measured, but
SRC +1.2K, STG +1.1K and MISC +1.4K. WORK-H paired median -1,088 (mean +201);
P95 +7K. That is not a win, so it was reverted. Mismatch counter 0.

**Digest tool.** The run differed from `itcm4` at 3 of the 4 ring stops,
without any shift. Each was DGSA equal, DGSB different and the next sample
equal: the sampler reads DGSB across a stop. With `--resync`,
`compare-replay-digest.py` now reports that shape as IDENTICAL WITH N SEAM
TICK-B WORDS. Strict mode still reports DIVERGED, and injected DGSA or DGSB
mutations still read DIVERGED.

## 18. libc heap witness reads the top chunk directly

A fresh profile (`profile-4f08-census.txt`) put newlib's
`__malloc_update_mallinfo` at ~1.8K ticks/frame. With `_mallinfo_r`, `mallinfo`
and the malloc locks, the total is ~2.3K. The caller is
`ndsTaskmanSampleLibcHeapNow`, once a frame. The only field it needs is
`keepcost`, which is `chunksize(top)`. The disassembly of
`__malloc_update_mallinfo` shows the store: it loads `__malloc_av_[2]`, masks
`top->size & ~3`, and writes that to `mallinfo+36`. The rest of the call walks
all 128 bins.

The witness now reads the top chunk directly:
- The direct read is armed only when it matched `mallinfo().keepcost` at the
  post-shrink reset.
- The tick-HUD build re-checks every 128th sample
  (`gNdsTaskmanLibcTopDirectMismatch`).

Same-ROM A/B (`71EB6367`):
- Paired WORK-H median -2,944.
- `gNdsTaskmanLibcTopChunkMin` (13,528) and the runtime high-water (30,976) are
  identical on both arms, with 0 mismatches.
- Both arms replay IDENTICAL to `itcm4`.

Final `3FDE5B3B` (`libcf`), replay IDENTICAL to `itcm4`:
- WORK-H P50/P95/P99 1,102,720/1,538,624/1,955,136 -> **1,099,200/1,536,512/
  1,948,416**.
- Paired median -2,304; mismatches 0.

The same census shows ITCM at 128 B free. The remaining admissions are worth
about 0.5K each, so the ITCM lever is spent.

## 19. Battle-camera framing witness out of every ROM (A9)

`ndsCameraRecordFrame` computes four fighters' off-screen margins in soft
float, once a frame, in every build. Only `probe-native-render-scene.ps1`
reads it. It now compiles only with `NDS_CAMERA_FRAME_WITNESS=1` (Makefile,
default 0). Its globals go with it, so the probe fails on a missing symbol
instead of reading zeros.

Same-ROM A/B (`BA72BD7E`, lab word): witness-off minus witness-on, paired WORK-H
median -2,368 (SRC -2,368). Both arms replay IDENTICAL to `libcf`.

Final `947E6F19` (`camf`) replays IDENTICAL to `libcf`:
- WORK-H P50/P95/P99 1,099,136/1,536,576/1,961,152.
- Paired median against `libcf` is +320. The work removed is the same-ROM
  -2.4K; this cross-build compare also moves every later function's address.
  It is reported, not claimed as a further cut.
- VBlanks 986/932/49/6.

Build note: the first `-j8` build after the Makefile edit failed in the
particle-texture generator (Python `io.open`) and the run measured the stale
lab ROM. A serial rebuild succeeded, and `nm` confirmed the witness symbols
are absent.

## 20. The VS Results emblem hook is no longer called in battle

`gcCaptureCameraGObj` called `ndsResultsEmblemRecordCapturedDisplay` for every
display GObj that the stage did not take. Its first test bails on any scene but
VS Results, but a battle still paid the call and its large-frame prologue:
~2.0K ticks/frame in the `4f08` census. The scene test now sits at the call
site. The behaviour is identical because the hook has no side effects before
that test.

`emblem` `E544771E` against `camf`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,099,136/1,536,576/1,961,152 -> **1,098,432/1,536,384/
  1,953,920**.
- Paired median -2,048 (MISC -1,728, STG -704).
- VBlanks 1,001/920/46/6: over half the presented frames now fit two VBlanks.

## 21. Stage world-key compare reads words, not four memcmp calls

`ndsRendererAdapterStageWorldSourceKeyMatches` validates each stage node's
cached world matrix. For each node it called `memcmp` four times: translate
(12 B), rotate.a (4 B), rotate.vec (12 B) and scale (12 B). That was 129 calls
a frame and ~3.4K ticks of `memcmp` in the `4f08` census. The compares now read
the float fields as `may_alias` words, the same bitwise equality.

`keycmp` `962DCD90` against `emblem`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,098,432/1,536,384/1,953,920 -> **1,094,528/1,533,888/
  1,956,352**.
- Paired median -2,240, all of it STG.
- VBlanks 1,009/909/50/5.

## 22. 64-bit divides and the reject's square root on the DS units

The `4f08` census charged `__udivmoddi4` ~2.2K ticks/frame for about eight
divides a frame, ~275 ticks each: libgcc's bit-by-bit loop. The callers:
- `ndsP2HbRejectPoints`. The hurtbox reject used the kernel header's portable
  hooks, a C 64-bit divide plus a 32-step digit-by-digit square root, on every
  cache-slot miss. It now binds `NDS_R2_CFX_DIV64` and `NDS_R2_CFX_ISQRT64` to
  `ndsR2HwMathCfxDiv64` and `ndsR2HwMathCfxIsqrt64`, as
  `nds_r2_collision_fixed.c` does under `NDS_R2_CFX_HWMATH`. The arithmetic is
  proven identical (`scripts/check-r2-hwmath.ps1`).
- `ndsNativeWallpaperDraw`: two `s64 / 5` a frame, now `ndsR2HwMathDivideLead`.
  The unit truncates toward zero, which is C's rule.
- `ndsBattlePlayablePacingUpdate`: two `u64 / ticks` FPS statistics a frame,
  the same change. The operands are below 2^40.

`hwdiv` `FB41F7D6` against `keycmp`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,094,528/1,533,888/1,956,352 -> **1,087,808/1,523,648/
  1,932,416**.
- Paired median -3,456, mean -6,197. The mean is larger because the reject's
  slot misses cluster in hit-active frames (SRC mean -3.8K).
- VBlanks 1,034/891/43/5. Native failures 0.

Build note: the first attempt also bound `NDS_R2_COLLISION_DIV64`. That broke
`nds_r2_collision_mtx.h` and the hook is not needed here, so it was dropped.

## 23. Where the P95 set's tail lives now, and two refuted hurtbox caches

**Attribution** (`attrsplit`, `NDS_TICK_HUD_SRC_SPLIT=1 NDS_P2_MISC_SPLIT=1`,
replay IDENTICAL to `hwdiv`). P95-set mean over the median:

| Bucket | Excess over median |
|---|---|
| FTR | +283K |
| SINT | +122K |
| SHDT | +106K |
| SPRM | +72K |
| MISC | +60K (items +30K, weapons +25K, effects +22K, capture +19K) |

In the `257b` profile (`profile-257b-census.txt`), the hit-heavy frames carry
the hurtbox reject's own composition: `ndsP2HbRejectPoints` +42.5K,
`ndsR2CfxBuildLocal` +14.6K and `ndsP2HbVec` +8.4K. The source's float hit
tests for the pairs it passes add a further ~+32K.

**Lean entry thrash, measured** (`lru`, a lab-only shadow LRU at every lean key
event; reverted). It counts the misses an LRU of 2, 3 or 4 distinct keys
would take, against the actual materializations:

| Slot | Actual | 2 entries | 3 entries | 4 entries |
|---|---|---|---|---|
| DK | 26 | 25 | 15 | 12 |
| Samus | 10 | 9 | 4 | 4 |
| Link (variants cover it) | 15 | 27 | 13 | 10 |
| Kirby (variants cover it) | 11 | 39 | 26 | 18 |

The 2-entry model reproduces DK and Samus. A third entry would save about 10 +
5 materializations of 62, roughly -25K P95 by the spike-removal estimate, for
~17.7 KB of heap per slot.

**Refuted: per-damage box cache** (`hbbox`). The reject caches its hurtbox half
(world centre, extent, column sums) per damage box and latch epoch. Hits were
5,609 against 8,542 fills. Paired WORK-H -2.1K, but P95 +1.7K and SRC +1.3K.

**Refuted: per-fighter latch epochs** (`hbep`, with the box cache). Box hits
were unchanged at 5,614: each box is tested about 1.7 times per its own
fighter's epoch. Paired -1.2K, SRC +1.9K. Shadow mode (`hbepshadow`) showed 0
flips and replay IDENTICAL. Both changes were reverted.

## 24. Lean spare buffer: evicted lists are parked, not re-materialized

Each fighter slot keeps two lean lists in its half-region pair. A slot whose
display states outnumber two re-materializes a list it held moments earlier.
The section 23 shadow LRU showed this for Donkey, and each materialization is a
~480K-tick spike.

**Design.** In route 1, once the recorder has given up the lower half, a slot
gets one more half-sized buffer from the battle's general heap.
- The buffer is allocated lazily, at the first eviction of a valid list, and
  only while the heap keeps the stage body's floor: 25,600 + 36,420 B after the
  allocation.
- An entry-to-buffer permutation (`phys`) lets an eviction swap the evicted list
  into the spare. A miss whose key the spare holds swaps it back in and is
  treated as an entry switch.
- Nothing is copied. A packet's `words` pointer points into its own buffer, so
  a list never moves.
- A wide list (entry 0 over both halves) restores the identity map first.
  `ndsFtrLeanPacketDrop` resets the map, and a heap-generation change forgets
  the buffer.
- The slot-state struct grows by 168 B per slot. Padding brings the array's
  growth to exactly 1,024 B (one D-cache way). Unpadded (`sparef`), the same
  code read P50 +8.2K and SRC +4.2K from cache phase alone.

**Same-ROM A/B** (`85D19B8E`, lab word; `spare0` off, `spare1` on; both
replay IDENTICAL to `hwdiv`):
- Materializations 62 -> 44 (DK 26 -> 16, Link 15 -> 7). Three spare buffers
  were allocated; the fourth was refused by the floor.
- WORK-H P95/P99 1,528,128/1,932,352 -> **1,504,704/1,792,704**; paired mean
  -5.9K. The median is unchanged, as a tail-only cut should be.
- Heap low-water 122,412 -> 69,340 B (floor 25,600).

The lab verify arm (`sparever`, Slow 16) re-materialized 179 re-selected lists,
spare hits included, with 0 mismatches.

**Final** `52272220` (`sparepad`) against `hwdiv`:
- WORK-H P50/P90/P95/P99 1,087,808/1,368,448/1,523,648/1,932,416 ->
  **1,091,072/1,367,424/1,493,952/1,784,064**.
- Paired median +2.2K, mean -3.7K. The extra cost is FTR's entry-map lookup,
  +1.2K median.
- VBlanks 1,026/912/33/2. Native failures 0.

## 25. Stage matrix memo; data sections pinned to 1 KB; motion-acquisition map

**Stage memo.** `sNdsNativeStageMatrixGen` is bumped wherever the stage owner
takes a frame's camera and binding matrices. For one generation,
`nds_stage_gx.exec.inc` memoizes the VIEW patch's hardware affine (22
rebuilds a frame before) and each (binding, shift) composed matrix (~52 rebuilds
a frame, 23 distinct). Same-ROM A/B (`F83D7C29`, lab word; `smemo0` off,
`smemo1` on, both replay IDENTICAL): paired WORK-H median -4,736, STG -4,928.

**Layout noise.** The lab ROM read P50 +13K and P95 +28K against `sparepad` in
*both* arms, and the final unpinned memo build (`smemof`) read P50 +6.5K and
P95 +22K. Most of that sat in SRC code the change never touches.
`.main.rw` and `.main.bss` start right after the code, so any code growth
re-phases every global's D-cache set. They now start on 1 KB boundaries: `.main`
pads its own end so the load image stays contiguous, and `.main.bss` is
`ALIGN(1024)`. That removes the data half of the drift for future builds. Code
(I-cache) layout still moves.

The pinned final `0E7B9ABF` (`pin`, memo plus pin) replays IDENTICAL to
`sparepad`:
- WORK-H P50/P95/P99 1,096,576/1,501,312/1,796,736, against `sparepad`'s
  1,091,072/1,493,952/1,784,064.
- Paired median +5.5K, SRC +3.3K. That is the one-time layout move, not the
  memo, whose own effect is the same-ROM -4.7K.

From here, changes under ~10K are priced by same-ROM toggle words.

**Motion acquisition (read-only map, for the next step).** P2 fighters' clips
(DK/Samus/Link/Kirby) are READY_STREAM bytes: slot offsets plus u16 command
runs, immutable after load, with no absolute pointers. About 340-370 hits a
match are still copied into the fighter heap, registered and resolved on every
status change; the Mario/Fox battlepack skips all of that. Serving the cache
payload directly needs:
- a per-heap pin table;
- ring eviction that respects pins (6 ring wraps a match);
- an `ndsBattlePackContains`-style resolver hook in
  `ndsRelocResolvePointerFromFileBase` and `ndsRelocPointerIsFighterAObj16`.

## 26. Event32 ledger: ForgetRange skips on a block interval, not a page count

Every fighter motion load into a figatree heap calls
`ndsAObjEvent32ForgetRange` over the old occupant. It skips the ledger scan
when no 4 KiB page of the range holds an entry. Lab counters (`forgetprobe`,
`fgt-cnt`, `fgt-why`) showed:
- 702 calls a match; 457 skipped.
- 163 scanned; only 4 of those removed anything (1,446 entries, file
  retirement). The other 159 scans removed nothing.
- All 159 empty scans were single-page ranges (a ~1 KiB clip). The page's 10
  entries (`fgt-why2`) all sit in blocks 0-6, below the range at block 8.
  Each empty scan still read the 1,508-entry ledger: 6 KiB through a 4 KiB
  D-cache.

The fix keeps a [lo, hi] 16-byte block interval per page (2 x 1,024 B BSS,
a 1 KiB-multiple growth):
- The first add after the count reaches zero sets the interval; later adds
  widen it. It always bounds every live entry, so a skip is still exact.
- Removals leave the interval wide. The first build (`fgt-on`) therefore
  skipped only 82 more calls: the page's `hi` was 255, stale from an earlier
  file's entries.
- The scan that does run now re-derives the interval of the range's two edge
  pages from the entries it keeps.

Same-ROM A/B (`FBD06447`, lab word `gNdsAObjEvent32ForgetBlocksOn`):
- `fgt2-off` vs `fgt2-on`: skips 457 -> 698 (4 scans left).
- WORK-H P50/P95 1,095,552/1,509,568 -> 1,092,864/1,499,584 (-2.7K/-10.0K),
  paired mean -2.6K; SRC P95 -15.8K.
- Both arms replay IDENTICAL.

Final `48F428A1` (`fgtfin`, toggle removed) vs `pin`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,093,568/1,498,304/1,793,856 (-3.0K/-3.0K/-2.9K).
- 1,086 of 1,972 frames at or under 1,120,000 (`pin` 1,081).
- Heap low-water 69,340 B; native failures 0.

## 27. CSS reserve re-measured after this batch's static growth

The owed all-content check (`3aef3ef6b33`, `smash64ds-p2-shell-freeplay-hwtri`
in `builds/build-css-margin`, owner CSS route
`2026-09-23_css-preview-heap/tools/run-owner-css.ps1`) reads
`CSSRESERVE free=262096 fail=0` against the required 183,072 B (margin 79,024).
Link, Yoshi and Pikachu previews draw (cumulative triangles 11,824 / 23,140 /
33,550; screenshots `artifacts/visibility/2026-09-06_css-*-margin0927.png`).
Log: `css-owner-margin0927.log`. Pass `-Rom`/`-Elf` as absolute paths: the
emulator starts in its own directory, a relative ROM never opens the GDB
listener, and the failed run prints the previous run's capture.

## 28. The replay digest is instrument time: charged to HUD

`ndsReplayDigestTick` exists only in the tick-HUD ROM (`#if NDS_TICK_HUD`),
and it ran inside WORK, in OTHR (after the SRC span closes). The median frame
spent ~4.4K ticks in it (profile `p2p8-profile-257b`). The shipping ROM has
no digest. The guardrail says the instrument stays out of the gate. So the
digest's own ticks are now timed and added to the HUD bucket, and WORK-H
(WORK - HUD) takes them back out. WORK, ALL and the named total are unchanged.

This is re-accounting, not a speedup.

`8B541A87` (`dgt`) vs `fgtfin`, replay IDENTICAL:
- HUD paired median +4,992.
- WORK paired median +768, mean +1.8K. That is layout, plus the two timer
  reads per tick.
- WORK-H P50/P95/P99 1,089,600/1,499,392/1,789,248 (-4.0K/+1.1K/-4.6K).
- 1,107 of 1,972 frames at or under 1,120,000 (`fgtfin` 1,086).

## 29. Two lab clocks out of the gate ROM

Both were attribution that ran in every tick-HUD build.
- The lean materializer's per-part clocks (`NDS_FTR_LEAN_MAT_PART`, about 300
  clock reads per materialization) now compile only under
  `NDS_FTR_LEAN_TIMED` (`NDS_FTR_LEAN_PHASE_TICKS=1`). Materializations are
  tail frames. `materialize_ticks` (two reads) stays.
- Dream Land's Whispy AOT tick counters (8 reads a frame) now need
  `NDS_WHISPY_AOT_TICKS=1`. Without it the symbols are absent, so
  `probe-whispy-native-aot.ps1 -TraceFrames` fails loudly rather than reading 0.

`8C78E908` (`instr`) vs `dgt`, replay IDENTICAL:
- The 29 FTR-spike frames: FTR -31.5K each.
- WORK-H P50/P95/P99 1,090,752/1,494,272/1,788,608 (+1.2K/-5.1K/-0.6K).
- Paired median +640 (layout), mean -0.9K.
- Materializations 44, native failures 0.

## 30. Where the D-cache fills go, and the DTCM hot stack

**Instrument.** A scratch copy of the melonDS fork (the owner's tree was not
touched) records every D-cache line fill by (PC, 32-byte line) when
`MELONDS_ARM9_DMISS=1` is set beside the profiler. It writes
`<base>.dmiss.csv`. The patch is `dmiss/melonds-dmiss.patch`; the reports are
`dmiss/dmiss_report.py` (by class, function and FTStruct line) and
`dmiss/dmiss_static.py` (by static object). The census window is frames
200-584, 385 frames (`dmiss/census.txt`).

**What it showed.**
- 12,907 line fills a frame. ITCM code took 4,635 of them against 290K cycles
  of ITCM-tier memory stall: about 62 cycles, 31 ticks, per fill. A 32-byte
  fill over the 16-bit main-RAM bus is that slow, so fills are about a third
  of the frame.
- The largest single object is the gameplay coroutine's own main-RAM stack:
  1,470 fills a frame, 11%. The ARM946 D-cache is read-allocate, so a push is
  a write-through and the pop of a frame whose line was evicted is a fill.
  The kernel compose's world stack alone is 216 a frame.
- Next is the FTStruct pool (`sFTManagerStructsAllocBuf`, 4 x 3,012 B):
  1,073 fills a frame, spread over 88 of its 95 lines.
- The top static object is `gSYSinTable` (288 fills a frame).
- Heap pages holding per-fighter state make up most of the rest.

**Stack depth.** A lab paint probe measured the battle loop's reach below
`ndsR2BattleRun`'s SP over the stress match:
- update phase 1,200 B;
- present phase 9,460 B, on rare effect paths whose frames hold a 3 KB
  `NDSRendererTraversalState` (`stkdepth2`).

The whole loop cannot move to DTCM, and storage reads into stack locals
would break there anyway: DMA and the ARM7 cannot see DTCM.

**Change.** Two audited draw subtrees run on an 8 KB DTCM stack
(`gNdsDtcmHotStack`, `ndsDtcmHotStackRun` in `src/port/coroutine.c`, an ARM
trampoline that runs in place when already on it):
- `ndsFtrLeanRun` (the lean fighter list: kernel compose, patch, submit);
- `ndsRendererCommitNativeStageSegment` (the stage GX commit).

Neither does I/O, DMAs from a local or switches coroutines. Texture uploads
stage through static buffers.

The DTCM came from R2-03 E29's Mario dense tables (8,582 B,
`.dtcm.fighter`). Every VS kind draws through the lean list, which reads them
only when it materializes a Mario list, so they moved to `.main.rw`. The
renderer frame summary, which rode the same section, stays in DTCM as
`.dtcm`. `check-task20-dtcm-layout.ps1` was already failing at HEAD (lab
control words it did not model). It now models:
- no fighter tables in DTCM;
- the named `.dtcm` owners, with pinned sizes;
- the hot stack leading `.dtcm.bss`, pinned.

It passes on the gate and all-content ROMs.

**Measured.** Same-ROM (`FF6AC2FA`, lab word; `hotc0` off, `hotc1` on), both
replay IDENTICAL:
- WORK-H P50/P95/P99 1,089,472/1,491,840/1,752,320 ->
  1,068,800/1,465,152/1,721,472.
- Paired median -18,432, mean -19.4K; FTR -11.5K, STG -7.0K.
- Frames at or under 1,120,000: 1,114 -> 1,214.
- Deepest reach 3,740 of 8,192 B.

Final `7D51B305` (`hotfin`, toggle removed) vs `instr`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,069,440/1,468,224/1,720,256.
- 1,216 of 1,972 frames at or under 1,120,000.
- Heap low-water 69,340; native failures 0.

All-content CSS reserve free 249,808 (margin 66,736; the tables cost
12,288 of arena). Previews draw.

**Trap hit on the way.** PowerShell `Copy-Item` keeps the source file's
modification time. Restoring `nds_r2_battle.c` from a backup that way left
make's lab object in place, so three arms (`hot0`, `hot1`, `hotdiag`) and one
"HEAD" rerun carried an 11 KB stack paint twice a frame (+110K WORK-H). The
ROM hash gave it away: `base2` equalled `stkdepth2`.

## 31. Hot stack round 2: map collision and particles; pose refuted

A fill census on `5506c91a436` (`dmiss2/`) read 12,268 fills a frame, down
from 12,907. The gameplay stack's share fell from 1,486 to 985. Of what
remains, map collision is the largest block (~124 a frame: wall sweep, floor
query, line lookups under `mpProcessUpdateMain`), then particles (~34), camera
capture, matrix leaves and the pose (~50).

**Pose, refuted.** `ndsFtPoseUpdate` on the hot stack (`hpose0`/`hpose1`,
same ROM, IDENTICAL) read paired +384. Its frames were too few fills to pay
the trampoline. Reverted.

**Kept.**
- `mpProcessUpdateMain` runs on the hot stack from the live bridge. Its
  geometry and collision callbacks only set flags and positions. The one
  side effect is an item FGM: storage IPC requests are static, and their
  destinations are range-checked to main RAM. `proc_map`, which changes
  status and loads motions, runs after it returns.
- `lbParticleDrawTextures` runs on it too: GX writes, no I/O, no DMA from
  locals.

Same-ROM (`7348A4BD`, two lab words; `hb0` off, `hb1` on), both replay
IDENTICAL:
- WORK-H P50/P95 1,069,760/1,469,696 -> 1,061,184/1,457,984.
- Paired median -7,872, mean -9.7K; SRC -4.0K, MISC -3.6K.
- Deepest reach still 3,740 B.

Final `54785514` (`hot2fin`) vs `hotfin`, replay IDENTICAL:
- WORK-H P50/P95/P99 1,062,016/1,461,056/1,716,800.
- 1,241 of 1,972 frames at or under 1,120,000.
- Layout gate passes.

## 32. The lean kernel's half sine table, in DTCM

Section 17 refuted the half table in ITCM because of the code it evicted. The
fill census puts the 4 KB `gSYSinTable` at 288 line fills a frame, most of
them the kernel's six lookups a joint. `NDS_FTR_LEAN_SIN_DTCM` (default 1)
places the same symmetric-exact 2 KB half in `.dtcm`. The kernel fills it
from `gSYSinTable` on first use; gameplay still reads the full table.

Room was needed in two places:
- DTCM: the hot stack went from 8 KB to 7 KB. Its deepest reach is 3,740 B.
- ITCM: the table's index mirror, inlined six times a joint, overflowed ITCM
  by 144 B. `gcPlayDObjAnimJoint` (612 B) left ITCM for plain `.text`. The
  pose engine owns fighter joints now, and the census ran it at ~9
  instructions a frame.

The layout gate names the table.

`D0627BDB` (`sindtcm`) vs `hot2fin` (cross-build), replay IDENTICAL, lab
mismatch counter 0:
- WORK-H P50/P95 1,057,600/1,455,360 (-4.4K/-5.7K).
- Paired median -4,224: FTR -3,584, STG -2,112, SRC +1,536.
- 1,253 of 1,972 frames at or under 1,120,000.
- P99 1,751,616 (+35K). That is six frames whose event cost (motion/FGM
  timing) moved by 100-500K with the layout. The 60 costliest control frames
  pair at median -3,840.

## 33. Refuted: a bigger motion-clip arena

The ~367 in-match NitroFS clip reads cost ~30K ticks each
(`gNdsRelocAssetFighterStreamReadTicks64` 169,749 x 64 over 367 reads). The
arena takes whatever is spare above a 128 KiB keep-free at reservation, and
the match's heap low-water is 69,340 B. So `ak96` lowered the keep-free to
96 KiB:
- The arena reserved 118,576 B, against ~65 KB used before.
- Hits 334 -> 339 and misses 372 -> 367: the misses are first uses.
- The tighter heap refused lean spares: materializations 44 -> 54.
- WORK-H P95 1,462,912, worse than `sindtcm`.

Reverted. Zero motion reads after GO needs clips resident or prefetched
before first use, not a larger LRU.

## 34. DTCM hot scalars, second batch, ranked by fills per byte

With the half sine table in, 284 B of DTCM were left under the 0x02ff3000
boot-stack ceiling. The hot stack dropped to 6 KB (reach 3,740 B), which freed
1 KB. `scratchpad dtcm_pick.py`, kept here as `dmiss2/`'s method, ranked the
census's static objects by fills a frame per byte. It kept only single-TU
`.bss.<name>` / `.data.<name>` input sections and filled a 1,500 B budget:
- 49 objects, about 638 fills a frame;
- plus libnds `glGlobalData` (`.bss.glGlobalData`, 76 B, 68 fills a frame).

They include the taskman DL/heap descriptors, `gGCCommonLinks`,
`sGCProcessQueue`, `sNdsFtPose`, `gGRCommonStruct`, `gGMCameraMatrix`/
`gGMCameraStruct`'s neighbours, the MP line/yakumono state, the stage world
index and a set of per-frame counters. All are ARM9-only: no DMA endpoint,
nothing the ARM7 or IPC sees. They join the existing hot-scalar blocks by
name. The three names this configuration compiles out
(`sNdsRendererTask36CaptureActive`, `sNdsEffectPacketArmed`,
`gNdsCameraFrameCount`) left the list, so `check-dtcm-residency.py` passes
again: 159/159 resident. The layout gate passes.

`4501F881` (`hs2`) vs `sindtcm` (cross-build), replay IDENTICAL:
- WORK-H P50/P95/P99 1,046,592/1,444,736/1,704,384 (-11.0K/-10.6K/-47K).
- Paired median -11,008, mean -11.8K: SRC -5.1K, STG -2.4K, MISC -2.4K,
  FTR -0.8K.
- 1,288 of 1,972 frames at or under 1,120,000.
- Heap low-water 69,340; materializations 44.

All-content config: DTCM end 0x02ff2ef4, layout gate passes, CSS reserve free
253,904, previews draw.

## 35. Trampoline into ITCM; FTStruct field heat (a proposal, not a change)

**ITCM.** A fresh census (`dmiss3/`, on `006bc827673`) charged the hot-stack
trampoline pair in main RAM ~1.2M cycles of instruction fetch. So
`ndsDtcmHotStackCall` (80 B, now `.itcm.*`) and `ndsDtcmHotStackRun` (132 B)
moved into the 472 B that `gcPlayDObjAnimJoint` left, together with decomp
`syVectorAdd3D` (40 B, by input-section rule).

`878584CE` (`itcm6`) vs `hs2`, replay IDENTICAL after one resync (a sampler
label shift at sample 44):
- WORK-H P50/P95/P99 1,045,696/1,443,008/1,671,872.
- Paired on the aligned samples: median -1,536, mean -1.6K.
- 1,298 of 1,972 frames at or under 1,120,000.

**FTStruct field heat.** The scratch melonDS now also counts every cached
data read by word (`MELONDS_ARM9_DWATCH=lo-hi`, `dmiss3/arm9-profile.dwatch.csv`;
patch in `dmiss/melonds-dmiss.patch` plus `dmiss3/melonds-dwatch.patch`).
Joined with the fill census and the pool base (0x02311868 in that ROM),
`dmiss3/ftfield_heat.py` gives, per FTStruct field, reads a frame and fills
apportioned by reads. The totals are 4,021 reads and 1,074 fills a frame, a
27% miss rate.

The hot aggregates are:
- `coll_data`: 903 reads, 68 fills;
- `joints`: 461 reads, 46 fills;
- `attack_colls`: 244 reads, 61 fills;
- `physics`: 225 reads, 17 fills;
- `computer`: 202 reads, 33 fills;
- `colanim`: 124 reads, 46 fills.

About forty hot scalars are spread over some 25 lines; together they draw
~400 fills a frame. The worst are `status_id`, `ga`, `figatree_heap`,
`nds_magic`/`nds_slot`, `motion_vars`, `input`, the capture/catch/item/throw
GObjs, `fkind`/`team` and the status wait counters.

A hot-first reorder could remove an estimated 150-250 fills a frame (5-8K
ticks). But `include/ft/fighter.h` pins FTStruct to the BattleShip source
layout with static asserts: port-only fields go after it, and lab tools key
on the offsets. Changing that is a layout-contract decision for the owner, so
it is recorded here and not done.

## 36. ndsStageGxDraw into ITCM, eleven low-rent residents out

The census shows main-RAM code running about 1.1 extra cycles an
instruction over ITCM. `ndsStageGxDraw` (3,724 B) carried ~8.5M cycles of
that non-memory stall; it had been kept out only for space. Eleven residents
at under ~1,300 cycles a byte of census rent (3,452 B) moved to main RAM,
each marked noinline so no ITCM caller pulls it back in:
- `ndsRendererNativeApplyStateDelta`, `ndsRendererRecordTextureState`,
  `ndsRendererRecordSetTile` and `ndsRendererSyncTextureTile`;
- `ndsRendererR2ClampDiffuseToMaterial` and `ndsRendererR2MaterialColor15`;
- `ndsRendererHardwareApplyTextureParams`, `ndsRendererHardwareBindTextureName`
  and `ndsRendererHardwareEndBatch`;
- `ndsRendererAdapterBuildDObjLocalMatrix` and `ndsFtrLeanPacketGuard`.

`memcmp` also left: 120 B, now its own `.text` section. Without `noinline` the
first link overflowed ITCM by 808 B.

`0EA2F97D` (`itcm7`) vs `itcm6`, replay IDENTICAL after one resync:
- Aligned paired WORK-H median -4,928, mean -4.1K: STG -6.7K, FTR +1.0K,
  MISC +0.4K.
- WORK-H P50/P95/P99 1,039,616/1,441,728/1,729,856 (-6.1K/-1.3K/+58K).
- 1,311 of 1,972 frames at or under 1,120,000 (from 1,298).

The tail cost is FTR in materialization and hit frames (top 5% FTR mean
+10.6K). The evicted tint/shade and texture-bind helpers run on those paths.
The follow-up is `_arm_addsubsf3.o` (684 B, mostly dead Task 16 goldens). It
can leave ITCM once port `ui2f`/`l2f` and a `__floatsisf` alias replace its
live conversions, and that frees about 530 B to re-admit the tail helpers.

## 37. libgcc's int-to-float member out of ITCM; texture-bind helpers back in

`_arm_addsubsf3.o` held 684 B of ITCM. Most of it was Task 16 goldens that
never run; it stayed only because its `__aeabi_ui2f`, `__aeabi_l2f` and
`__floatsisf` are live. `src/nds/nds_float_conv.c` now supplies them in ITCM
(ARM, CLZ, round-to-nearest-even):
- `ui2f` and `l2f` are new, host-checked against the host's own conversion
  (`conv_impl.h`/`conv_test.c` here): all 2^32 unsigned inputs and 4e8 random
  plus every power-of-two edge and halfway 64-bit input, 0 mismatches.
- `__floatsisf` forwards to the Task 16 `__aeabi_i2f`.

`NDS_P2_FLOAT_CONV` (default: the Task 16 i2f flag) renames the member's
copies to `__nds_p2_libgcc_*_golden` and places the member in main RAM.
`check-task9-float-itcm.ps1` fails on this tree and on the pre-change build
alike ("Task 16 off-mode did not retain the stock fadd wrapper"), so that
check is stale, not broken by this change.

`EBF9D29D` (`fconv`) vs `itcm7`: replay IDENTICAL; WORK-H P50/P95
1,038,656/1,439,744.

The 472 B left free took back `ndsRendererHardwareBindTextureName`,
`ndsRendererHardwareApplyTextureParams` and `ndsRendererHardwareEndBatch`
(432 B, noinline; texture binds on the materialization and effect paths).

`BB4B8539` (`readmit`) vs `fconv`, replay IDENTICAL:
- Paired WORK-H median -2,752 (MISC -1.7K); top 5% median -5.8K.
- WORK-H P50/P95/P97/P99 1,037,440/1,436,224/1,510,080/1,725,312.
- 1,322 of 1,972 frames at or under 1,120,000.
- P99 moves ±50K between builds with event timing (`itcm6` 1.67M, `hs2`
  1.70M, `sindtcm` 1.75M).

## 38. Calico's thread and copy/fill code out of ITCM; twenty census admissions in

The fresh census (`dmiss4/census.txt`, on `4fb5cd1dfd9`) rented calico's
`thread_hot.32.o` (thread switch/block/unblock, 1,100 B) and
`arm-copy-fill.32.o` (164 B) under 800 cycles a byte. `threadUnblockAllByValue`,
`armCopyMem32` and `armFillMem32` never ran on the ARM9. The linker's
`*.32.o` ITCM rule now has an `EXCLUDE_FILE` for those two members, so the
code runs from main RAM. Their `.bss` placement is unchanged.

`C036871C` (`calev`, eviction alone) vs `readmit`: replay IDENTICAL; paired
WORK-H median +128 (942 frames better, 1,011 worse), so the eviction is
neutral.

The freed 1,264 B took the census section D pack: twenty main-RAM functions
at 2,400+ non-mem stall cycles a byte, 1,258 B, admitted by input-section
name in the `.itcm` rule:
- collision: `mpCollisionCheckExistLineID`, the three `*LineCollisionSame`,
  `ndsMPGetTopologyEdgeLineID`;
- objects: `gcRunGObj`, `ndsR2AObjLiveCount`, `ndsGcGetGObjLifetimeSerial`;
- vectors and matrices: `syVectorMag3D`, `syVectorDiff3D`,
  `syMatrixTraRotRpyRScaF`;
- camera: `ndsR2CamMulQ`, `ndsR2CamDivQ`, `ndsR2CamSqrt64`;
- fighter display and look-at: `ftDisplayLightsDrawReflect`,
  `ndsFtrLookAtInputs`, `ndsFtrLookAtOutputs`, `ndsFtrLookAtMixWords`,
  `ndsFighterGetNativeOwnerSlot`;
- `func_ovl2_800F8FFC`.

The census put 4.48M non-mem stall cycles in reach (~5.8K ticks a frame).

`7278DFE0` (`caladmit`) vs `readmit`, replay IDENTICAL:
- Paired WORK-H median -8,128, mean -7,970; 1,953 of 1,972 frames better.
- Top 5% median -8,000. Mean by bucket: SRC -4.9K, FTR -1.9K, STG -0.7K.
- WORK-H P50/P95/P97/P99 1,029,696/1,430,336/1,495,808/1,715,392.
- 1,354 of 1,972 frames at or under 1,120,000.

The measured gain exceeds the census reach. The admitted code no longer
competes for the I-cache, which is the likely source of the difference; it
is not attributed.

Two static checks had gone stale on earlier commits from today:
- `check-gbi-decode-fixtures.ps1` now accepts the lab STG span end
  (`4922d3c5d94`) between a native-stage deactivation and its return.
- `check-renderer-itcm-placement.ps1` now lists this session's ITCM
  evictions (`17fcab47d7e`, section 35) as evicted rather than pinned.

## 39. The stage segment commit out of ITCM; an 82-function census pack in

Section 38's census, rerun with `--top 300`
(`dmiss4/census-top300.txt`), set two ITCM residents against a long section D
tail:
- `ndsRendererCommitNativeStageSegment`: 4,252 B renting 1,309 cycles a byte.
- `__aeabi_l2f` (`nds_float_conv.c`): 244 B that never ran in the match.

Both moved to main RAM: the commit dropped its `NDS_R2_ITCM_PACK2_CODE`, and
the conversion now sits in `.text.ndsFloatConvL2f`.

The 4,560 B went to the section D tail. The tail was ranked by non-mem stall
less the ITCM tier's own 0.31 cycles an instruction, per byte. The pack skips
`.text.hot` members, libgcc members and names already admitted.

The first link overflowed by 40 B, so the seven lowest-ranked small entries
were dropped. That left 83 functions admitted by input-section name. Most are
Thumb functions of 2-20 bytes: interrupt checks, map-collision tests and
decomp wrappers. At 7-40 cycles an instruction, they read as I-cache misses
on nearly every call from main RAM. The larger entries are:
- `ndsBaseMPProcessUpdateMain` (424 B);
- `ftDisplayMainDrawDefault` (388 B);
- `ftComputerProcDefault` (364 B);
- `ndsFighterDisplayContractProjectTarget` (368 B);
- `ftMainUpdateMotionEventsAll` (288 B);
- `ftComputerCheckEvadeDistance` (236 B);
- `ftPhysicsApplyGroundVelFriction` (192 B);
- `sinf`/`cosf` (228 B combined).

The estimated reach was ~11.7K ticks a frame gross, before the commit's
eviction cost.

`F07B0D40` (`repack5`) vs `caladmit`, replay IDENTICAL: paired WORK-H
median -12,544.

The P1 ROM (`smash64ds-battle-playable-hwtri`) then overflowed ITCM by
1,376 B. It shares this linker script, keeps `.itcm.native_fighter` (7.0 KB)
where the four-CPU build holds the lean kernel, and inlines the whole lean run
into `ndsFtrLeanRunOnHotStack`: 2,144 B there, 12 B in the four-CPU build.
Dropping that entry leaves 82 functions, and P1 links with 768 B spare.

`85B19565` (`repack5b`, the committed tree) vs `caladmit`, replay IDENTICAL:
- Paired WORK-H median -11,200, mean -11,151; 1,949 of 1,972 frames better.
- Top 5% median -12,608.
- Mean by bucket: SRC -6.8K, FTR -3.4K, MISC -2.4K, STG +1.6K. STG is the
  commit's eviction cost.
- WORK-H P50/P95/P97/P99 1,017,472/1,415,744/1,486,528/1,703,424.
- 1,394 of 1,972 frames at or under 1,120,000.
- The 1.3K below `repack5` is layout phase: the 12 B thunk carried 40K census
  stall cycles, about 50 ticks a frame.

`check-renderer-itcm-placement.ps1` lists the commit as evicted. The GBI,
DTCM layout and DTCM residency checks pass.

## 40. Every source tick on the DTCM hot stack

**Where the stack fills were.** The dmiss4 fill census was split by
instruction class (`stack_fills.py`): pop and `[sp, #]` loads against every
other load. That leaves 637 stack fills a frame after the earlier hot-stack
rounds. About 525 of them land in the top 1.5 KB of the gameplay coroutine's
main-RAM stack (0x2280400-0x22809ff), spread over more than 100 functions.
These are the update's own base frames: every return into a frame whose line
the update's data had evicted pays a fill.

**Change.** `ndsR2BattleRun` runs each `ndsR2HostBattleUpdateOnce` through
`ndsDtcmHotStackRun`.

The trampoline gains a busy word, `gNdsDtcmHotStackBusy`, a DTCM hot scalar:
- An entry from outside sets it and clears it on return.
- A hot-stack call from another stack while it is set runs in place on that
  stack. A coroutine the tick switches to can therefore never reuse the top
  over suspended frames.
- The lab high-water scan skips while it is set.

The tick's I/O is safe on a DTCM stack:
- Storage reads bounce any destination outside main RAM
  (`ndsAudioStorageReadCard`).
- PXI requests and DMA sources are static or heap buffers.
- The pointer-range checks in the tree accept DTCM addresses.

The trampoline's 28 B took `ndsPlatformReadInput` (120 B, rent 627 cycles a
byte) back out of ITCM.

**Same-ROM A/B** (lab toggle, since removed; `uh0` off, `uh1` on), replay
IDENTICAL:
- Paired WORK-H median -15,424; 1,969 of 1,972 frames better.
- Top 5% median -26,816. SRC -16.0K mean; the other buckets are flat.
- Hot-stack high-water 3,740 B, unchanged: the tick's depth stays under the
  draw subtrees' depth.

**Final** `5258C6EA` (`updhot`) vs `repack5b`, replay IDENTICAL:
- Paired median -16,768; 1,969 of 1,972 frames better.
- WORK-H P50/P95/P97/P99 1,000,768/1,390,784/1,468,032/1,667,392.
- 1,459 of 1,972 frames at or under 1,120,000.
- VBlanks 1,381/570/21/1: 70.0% of presented frames in two.
- Native failures 0; heap low-water 69,340 B.

The P1 ROM links with 848 B ITCM spare.

**Owed.** The high-water comes from the stress roster alone. A deeper tick path
on another roster, stage or item set would overflow into `.dtcm` below the
stack, and nothing checks for that at run time. Re-read
`gNdsDtcmHotStackHighWater` on the next all-stage or item campaign run.

## 41. Refuted: an ftGetStruct memo; DC_FlushRange without per-range drains

**ftGetStruct memo** (`ftmemo`, `6842824F`). The leaf attribution
(`dmiss5/census5-leaves.txt`) put `ftGetStruct` at 468 calls a frame, 49.6 cycles a
call (~11.6K ticks). In the shipping configuration the pool-pointer test
always fails, so each call reads two GObj lines: `id` and `user_data`.

A 4-entry DTCM memo (gobj to fp) was built with three invalidation points:
- a free hook in `gcSetGObjPrevAlloc`, through the objman overlay patch;
- objman setup;
- the heap generation.

Result vs `updhot`, replay IDENTICAL: paired median +576, top 5% -0.9K. The
callers read the same GObj and FTStruct lines right after the call, so the
misses moved rather than vanished (see "a memo is a memory stream"). Reverted.

**No-drain flush** (`nodrain`, `AE0CD6D1`). calico's `armDCacheFlush` drains
the write buffer on every call: ~143 cycles, 66 lean dirty-run flushes a frame
plus 24 debugger-group members. An ITCM clean+invalidate loop with one drain
per batch measured flat:
- paired median +256;
- FTR mean -1.5K;
- STG mean +1.3K, from the ITCM layout shift.

Reverted.

Also measured:
- **MISC split** (lab `NDS_P2_MISC_SPLIT=1`, `miscsplit`). Medians: capture
  interception and NDL-native draws 27.5K; un-owned `proc_display` 32.9K;
  camera loop remainder 25.7K.
- **Stage GX flush.** Its per-PC cost is bus contention: the first main-RAM
  loads after each DMA start wait ~430 cycles behind the burst.

## 42. Third ITCM pack (dmiss6 census)

The census on `a2381568e27` (`dmiss6/census.txt`) found a few more swaps.
Evicted: four second-pack residents renting 810-900 cycles a byte
(`ftComputerProcDefault`, `ndsBaseMPProcessCheckTestFloorCollision`,
`osContStartReadData`, `fabsf`; 532 B).

Admitted, 604 B:
- `ftDisplayMainProcDisplay`;
- `ndsMPFindLineYakumonoID`;
- `syVectorScale3D`;
- `ndsFighterDisplayContractSetRenderMode`;
- `ftMainPlayAnim`;
- `ndsFighterDisplayContractSetCycleType`;
- `DynamicArrayGet`.

The census put ~1.7M non-mem stall cycles in reach, about 2.2K ticks a frame.

`851469AA` (`pack3`) vs `updhot`, replay IDENTICAL:
- Paired WORK-H median -2,176; 1,614 of 1,972 frames better.
- WORK-H P50/P95/P97/P99 997,888/1,385,920/1,465,408/1,666,048.
- 1,465 frames at or under 1,120,000.

Link check: the P1 ROM links with 776 B of ITCM spare. The frozen root P1 ROM
(576F51ED) was copied aside before that link check and restored after it.

The census also shows what the section 40 move did to stack fills. They fell
from 637 a frame to 320. What remains is present-phase base frames:
- the camera capture loop;
- the stage world-matrix prep;
- the fighter display procs.

## 43. The stage owner prep and the fighter display proc on the DTCM hot stack

After section 40, 320 stack fills a frame remained, all in the present phase
(dmiss6). The whole present cannot move: the rare native item and effect
emitters each hold a 3 KB `NDSRendererTraversalState` on the stack (60 sites),
and those paths reached 9.5 KB. Two shallow subtrees carry much of the rest:
- `ndsRendererAdapterPrepareNativeStageOwner`, once a frame: stage world
  matrices, MVP recalc, material snapshot. Its body is now
  `...PrepareNativeStageOwnerBody`, and the public name runs the body through
  `ndsDtcmHotStackRun`.
- `ftDisplayMainProcDisplay`, the fighter display seam, split the same way.
  Its lean run already used the hot stack and now runs in place under it.

Neither subtree reads storage into a stack buffer or hands a stack address to
DMA. The native-stage consumed-field certificate tracks the renamed body:
`generate_nds_native_stage.py` and `check_nds_native_stage.py` name the new
closure. The regenerated manifest differs in that one line, and the generated
include is unchanged.

**Same-ROM A/B** (lab toggle, since removed; `ph0` off, `ph1` on), replay
IDENTICAL:
- Paired WORK-H median -11,776; 1,971 of 1,972 frames better.
- Mean by bucket: STG -8.7K, FTR -3.7K.
- Hot-stack high-water 3,876 B (was 3,740).

**Final** `8B62F8D4` (`presenthot`) vs `pack3`, replay IDENTICAL:
- Paired median -11,072; 1,970 of 1,972 frames better.
- WORK-H P50/P95/P97/P99 986,752/1,374,400/1,452,864/1,652,544.
- 1,493 of 1,972 frames at or under 1,120,000.
- VBlanks 1,418/535/19/1: 71.9% of presented frames in two.
- Native failures 0; heap low-water 69,340 B.

The P1 ROM links with 864 B ITCM spare; the frozen root was restored after the
link check.

## 44. Refuted: two lookups from the tail split

The dmiss7 tail split (`dmiss7/census7-top20.txt`, 20 costliest frames) put
the loaded-file scans (`ndsRelocFindLoadedFileByData`/`Containing`, ~16K
cycles a tail frame) and the pack span map (`ndsPreviewFileOffset`, ~12K on 9
of 20) among the motion-start premium. Both A/Bs are same-ROM with lab
toggles, and replay is IDENTICAL in each.

**Find rows** (`fr0`/`fr1`). A 1 KB mirror of each row's (data, size) was
rebuilt whenever the table epoch moved. Result: paired median +192, top 5%
+1.6K. On load-heavy frames the epoch moves as often as the scans ran, so the
rebuild costs what it saves. Reverted.

**Span bisection** (`sb0`/`sb1`). A section's spans were marked sorted at
REGISTER, and a sorted section was bisected instead of scanned. Result: paired
median 0, top 5% -0.3K. Whether the mask was ever set is unverified. Reverted.

A premium row with a few thousand cycles on half the tail frames has not, on
this evidence, been a lever.

## 45. DTCM hot scalars, third batch

The dmiss7 fill census ranked small static objects by D-cache fills per byte
(`dtcm3_pick.py`, input sections resolved from the objects). Forty-four
objects of 4-16 bytes (212 B) join the DTCM hot-scalar blocks. They are the
densest fillers: frame counters, the stage display cursor, pose bind flags,
the taskman graphics heap descriptor, the VBlank count, and similar.

The residency check cannot name a function-local static, so the loaded-file
lookup's 4-way memo became the file-scope `sNdsRelocFindContainingMemo`.

`check-dtcm-residency.py` and `check-task20-dtcm-layout.ps1` pass: forbidden
DMA references 0, and `.dtcm.bss` ends at 0x02ff2fc4 against the 0x02ff3000
ceiling. The P1 ROM links, its DTCM ending at 0x02ff21e8; the frozen root was
restored.

`FAD6C57E` (`dtcm3`) vs `presenthot`, replay IDENTICAL:
- Paired WORK-H median -128, which is flat.
- Top 5% median -1,344, mean -3,634; SRC -2.3K.
- WORK-H P50/P95/P99 986,112/1,371,008/1,649,728 (P95 -3.4K).
- 1,500 frames at or under 1,120,000.

Banked for the tail. The census put ~330 fills a frame on these objects, but
the median did not move, so most of those fills fall on the heavy frames.

## 46. Fighter FTStruct pool in ARM9 shared WRAM

ARM9 ran with no shared WRAM. Calico's ARM7 image started at 0x037F8000 and
held both 16 KB blocks, but it only needs about 50 KB. That is 21.4 KB of
`.wram` plus 28.8 KB of `.wram.bss`, including 16 KB of BGM `sBuffers`.

Changes, under `NDS_P2_ARM9_WRAM` (default 1; 0 is the lab control):

- ARM7 links from 0x037FC000 (`linker/nds_arm7_ds7_wram16.ld`, a copy of
  calico `ds7.ld` with only the origin moved). Its image now ends at
  0x03808440, below the DLDI shelter at 0x0380B000.
- `main()` first writes WRAMCNT = 2, which gives ARM9 the first block and
  ARM7 the second. ARM7's 0x037FC000 already addressed the second block
  under WRAMCNT = 3, so no ARM7 byte moves.
- MPU region 3 (unused by calico's crt0) maps 0x03000000 as 16 KB of
  cacheable, write-buffered data. A line fill there crosses the 32-bit WRAM
  bus instead of 16-bit main RAM.
- `ftManagerAllocFighter`'s first allocation is the FTStruct pool (about
  12 KB for four fighters). When it fits, it takes the WRAM block. The block
  is granted once per taskman heap generation, and any second request falls
  back to the heap.

BGM health (`bgmctl`/`bgmwram`, 600 samples): refills 29 in both arms,
playing 1, result equal, overruns 0, read failures 0.

Checks: DTCM residency 204/204; Task 20 layout; renderer ITCM placement
32,688/32,768; GBI fixtures. The P1 ROM links (the frozen root was
restored, `576F51ED`).

`8A133553` (`wramfin`, flag default) vs `dtcm3`, replay IDENTICAL:
- Paired WORK-H median -21,632, mean -21,966; 1,946 better, 26 worse.
- Top 5% median -26.6K, mean -29.4K; SRC -24K of that.
- WORK-H P50/P95/P99 965,184/1,344,320/1,610,752 (P95 -26.7K).
- 1,551 frames at or under 1,120,000; vbi2 1,502 (was 1,421).
- Native failures 0; general heap free min 69,340.

The same-ROM lab arm `wram1` (flag set on the command line) read
-21,312/-21,503, so the default build reproduces it.

Banked. The FTStruct is the sim's hottest record: every status, physics and
hit proc reads it. Moving 12 KB off 16-bit RAM is worth more than any of the
day's single-function cuts.

Next: about 4 KB of the block is still free. If ARM7's BGM buffers move,
ARM9 could hold all 32 KB.

## 47. Fighter GObj and top joint in ARM9 shared WRAM

The dmiss8 census (`builds/p2p8-dmiss8`, WRAM build) put about 98 D-cache
fills a frame on each fighter's first heap objects. The GObj is filled by
`ftGetStruct` about 36 times a frame, and the top joint DObj by
physics, AI and camera reads. The source grows its GObj and DObj pools
from the arena one 136 B object at a time, so a fighter's GObj and top
joint are simply the first two objects made in `ftManagerMakeFighter`.

The change: `ndsGcDonateFighterObjs` pushes one WRAM GObj slot and one WRAM
DObj slot onto the source free lists right before `ftManagerMakeFighter`,
and the source's own allocations take them.

- There are 4 slots, at the top of the WRAM block
  (`include/nds/nds_arm9_wram.h`).
- The FTStruct pool must end below the slots.
- `gcSetupObjman` resets the slot count when it drops the lists, so a slot
  is never on a list twice.
- The arena also keeps 1,088 B.

Same-ROM A/B (`wo0`/`wo1`, lab toggle since removed), replay IDENTICAL:
- Paired median -5,248, mean -5,645; 1,942 frames better, 30 worse.
- The run read 4 slots used.

Final `19D81E71` (`wobj`) vs `wramfin`, replay IDENTICAL:
- Paired median -4,832, mean -5,307; top 5% mean -4.8K.
- WORK-H P50/P95/P99 959,872/1,338,048/1,616,896.
- 1,571 frames at or under 1,120,000; vbi2 1,514; native failures 0.

Checks pass: DTCM residency, Task 20 layout, ITCM placement. The P1 ROM
links; the root was restored to `576F51ED`.

On DSi mode: the header maps NWRAM for ARM9 at 0x03700000-0x037BFFFF and for
ARM7 at 0x037C0000-0x037FFFFF. Neither overlaps the ARM9 block at
0x03000000.

Banked. About 11 cycles are saved per moved fill, against about 20 for the
FTStruct pool. The remaining 3,248 B of the block would buy roughly 2-4K
more (census statics or more joints).

## 48. Refuted: the present on the hot stack, and data-uncached code

The dmiss8 census still put ~234 fills a frame on the gameplay coroutine's
main-RAM stack. Most of them were on the present's outer frames:
`gcCaptureCameraGObj` (31), the stage display commit, effect dispatch, and
the trampoline's own pushes. The same census put 1,879 fills a frame
(18%) on main-RAM code lines. 1,578 of them were a function reading its own
literal pool.

**Present on the DTCM hot stack** (`ph1b`, lab word). The HUD's
stack-buffer DMA was made static for this. The run faulted at presented
frame 45, in `ndsRendererAdapterBuildDObjXObjMatrix` (a data abort) during
the entry animations. The present's generic draw recursion does not fit in
the 6 KB stack on top of its nested subtrees. Reverted. The hot stack keeps
only the measured subtrees.

**Data-uncached code** (`td0`/`td1`, lab word). MPU region 2 made the first
1 MB of `.main` data-uncached; instructions stayed I-cached. Result: paired
WORK-H median +42,432, and all buckets were worse; replay IDENTICAL. Each
literal-pool line fill serves several loads, so uncached loads cost more
than the fills they replace. Reverted.

## 49. All 32 KB of shared WRAM to ARM9; pose state moves in

ARM7 now links in its private WRAM (`linker/nds_arm7_ds7_arm9wram.ld`,
origin 0x03800000), and ARM9 takes both shared blocks (WRAMCNT = 0, MPU
region 3 at 32 KB). To fit the image below the DLDI shelter, some ARM7
content moves to ARM7's main-RAM reservation (0x02ff0000):
- the BGM worker's stack;
- calico's microphone, power-management and RTC services (code and bss),
  which the battle never runs.

ARM7 now ends at 0x0380a690.

**The storage thread's stack must stay in WRAM.** A first layout (`w32`)
moved it and the sector buffer to main RAM:
- It read paired median -8,960, but the top 5% mean was +6.3K.
- The same-ROM pose A/B in that layout isolated about +20K on tail frames
  from the storage thread's main-RAM traffic. Those are the motion-start
  frames, where reads happen.

Each pose slot's joints and tracks (4,404 B) now use a fixed 4,408 B WRAM
slot (`include/nds/nds_arm9_wram.h`), not two arena allocations. The WRAM
layout is:
- 0x03000000: the FTStruct pool (at most 12,288 B);
- 0x03003000: four pose slots;
- the GObj/DObj slots at the top.

The layout is checked at compile time.

Same-ROM pose A/B (`pw0`/`pw1`, lab word since removed; replay IDENTICAL):
- Paired median -8,960, mean -11,034.
- Top 5% mean -15.5K.

Final `429D478C` (`w32b`) vs `wobj`, replay IDENTICAL:
- Paired median -8,960, mean -8,372; 1,875 frames better, 93 worse.
- Top 5% median -18.2K, mean -14.3K (SRC -16K).
- WORK-H P50/P95/P99 948,928/1,322,752/1,606,912.
- 1,601 frames at or under 1,120,000.

Other results and checks:
- BGM: refills 99, overruns 0, read failures 0.
- Native failures 0; heap low-water unchanged at 69,340.
- Checks pass: DTCM residency, Task 20 layout, ITCM placement.
- The P1 ROM links; the root was restored to `576F51ED`.
- CSS smoke on the P2 ROM (`smash64ds.nds`, moved out of the root after the
  probe): the Link, Yoshi and Pikachu previews draw 12,162, 23,140 and
  33,550 triangles, the same as before this change.

## 50. Not banked: stage GX projection stamp

The dmiss9 census attributed 174 `memcpy` calls a frame (~25.7K cycles) to
`ndsStageGxDraw`: the 64 B matrix patches of Dream Land's program. The
patch kinds are 22 VIEW, 76 NOZ, 40 COMPOSED_NOZ, 30 CORNER_NOZ and
6 COMPOSED.

NOZ and PROJECTION patches depend only on the projection, and the
projection moved on just 54 of 321 stage frames. So a stamp (kept in each
patch's unused `aux`) skipped the rewrite whenever the projection was
unchanged.

The added bytes overflowed ITCM by 16 B, so `ndsFtrLookAtOutputs` (64 B,
~1,060 cycles a byte) was evicted to make room.

- Same-ROM (`ps0`/`ps1`): paired median -1,280, mean -1,226 (STG -1.0K);
  replay IDENTICAL.
- Final build (`psf`) vs `w32b`: paired median +3,968, and 1,774 of 1,972
  frames were worse.

The eviction and the 64 B shift of the code that follows it cost more than
the stamp saves. Reverted.

A rebuild of the reverted tree (`w32c`) reproduces `w32b` exactly: every
bucket equal, replay IDENTICAL. The ROM hash differs by build stamp only.

## 51. First any-stage sweep: Dream Land is the easiest stage

The owner's gate (09-27) is any 4 fighters on any stage, items on. The
four-CPU stress target hard-wires Dream Land and admits no other stage.
`NDS_LAB_FOURCPU_SWEEP=1` adds boot-pokable stage and roster words
(`gNdsLabFourCpuGkind`, `gNdsLabFourCpuKinds`).

Building the sweep ROM (`build-p2p8-sweep`) took three things:
- The eight `NDS_P2_STAGE_*` admission flags.
- A lab-only ITCM eviction of `ndsFighterDisplayContractProjectTarget`.
  Tick-HUD plus all stages overflowed ITCM by 112 B.
- The shipping build's missing NitroFS files (620). Without them, other
  stages crash at load.

This ROM runs ~12K P50 above the gate ROM on Dream Land (959,936 against
948,928).

Runs were parallel, one runner slot each; GDB ports must be spaced (ARM7 =
ARM9 + 1). Roster: DK/Samus/Link/Kirby, items on, 1,972 samples.

| Stage | P50 | P95 | P99 | frames <= 1.12M | native failures |
|---|---:|---:|---:|---:|---:|
| Dream Land | 959,936 | 1,340,864 | 1,643,520 | 79.9% | 0 |
| Jungle | 981,760 | 1,367,680 | 1,795,008 | 77.4% | 0 |
| Hyrule | 1,002,496 | 1,402,048 | 1,830,848 | 73.7% | 12 |
| Zebes | 1,068,352 | 1,505,024 | 1,879,744 | 60.1% | 0 |
| Castle | 1,073,344 | 1,441,280 | 1,789,184 | 58.8% | 0 |
| Saffron | 1,121,728 | 1,536,384 | 1,964,544 | 49.6% | 3 |
| Mushroom | 1,185,536 | 1,520,128 | 1,859,904 | 35.2% | 0 |
| Yoshi's Island | 1,267,968 | 1,688,128 | 2,089,984 | 20.2% | 0 |
| Sector Z | crash at frame 79 | | | | |

Where the extra cost goes (median buckets against Dream Land):
- **Yoshi's Island:** MISC +242K (MCAM 196K against 81K) and SRC +130K.
- **Mushroom Kingdom:** MISC +142K (items 130K: its stage hazards are items)
  and SRC +110K.
- **Castle / Saffron:** items 111K / 84K (bumper, Pokemon).
- **Dream Land:** items median 128. FTR is flat at ~143K on every stage.

**Sector Z** data-aborts at presented frame 79 in `gcParseDObjAnimJoint`
(objanim.c:348) with this roster. Still to find out: whether the shipping
flow (SSS) does the same.

**Native failures** (stage domain):
- Hyrule: 12, NO_PROGRAM.
- Saffron: 3.
- Roster Captain/Luigi/Mario/Fox on Dream Land: 48 (scene 22, identity
  0x03F300A1, 24 REJECTED + 24 NO_PROGRAM from frame 196). These appear
  with fast WRAM on and off alike.

## 52. Yoshi's Island clouds share the billboard camera memo

Yoshi's Island was the worst stage in the section 51 sweep (P50 1.27M). Its
D-miss census (`builds/p2p8-yoshi-census`, 384 frames), compared with Dream
Land's, put the extra float work in `ndsRendererAdapterMvpMod1F`. It runs
9 times a frame, and each run is `syMatrixLookAtF` plus `guMtxCatF`.

The cause: `ndsRendererAdapterSubmitNativeYosterCloud` recalculated each of
the frame's nine cloud drawables with no camera memo (`camera == NULL`). So
every drawable rebuilt the look-at Mod1, the float perspective and the
billboard rows. The stage prepare's own bindings already use a camera memo.

The change: the clouds now get a static camera memo. It is reused while the
camera's eye, at and perspective inputs are bit-identical and no one else
has reset the billboard-row memo since (`sNdsMvpMemoEpoch`), so every value
it serves is the one the recalculation would produce. When it is rebuilt,
it resets the shared row memo, as the stage prepare does.

Results (sweep ROM, Yoshi's Island, stress roster):
- Same-ROM (`cc0`/`cc1`): paired median -51,200 (MISC -51.2K); 1,965 of
  1,972 frames better; replay IDENTICAL.
- Final (`ccf`) vs `cc0`: paired median -49,568. P50/P95 1,269,056/1,685,824
  -> 1,220,864/1,634,368. Frames at or under 1,120,000: 400 -> 546.
- Gate ROM (Dream Land, no clouds), `ccg` vs `w32b`: paired median +1,024
  (layout), P95 1,322,752 -> 1,321,984. Replay IDENTICAL; native failures 0.

Checks pass: DTCM residency, Task 20 layout, ITCM placement. The P1 ROM
links; the root was restored to `576F51ED`.

## 53. Hurtbox reject: memoized edge conversions

The Saffron census (`builds/p2p8-yamabuki-census`) puts `ndsP2HbRejectPoints`
at ~48 calls a frame on over-gate frames, ~2,270 cycles each. Of that, about
190 float-to-Q12 conversions a frame (`ndsP2HbVec`, ~100 cycles each) go to
inputs that repeat:
- One attack's two points and radius are tried against every damage box of
  a victim in turn.
- A damage box's offset and size never change.

Both are now memoized on the float bits they convert (one attack slot, eight
damage slots), so a hit returns exactly what the conversion would.

Results:
- Same-ROM on Saffron (`hm0_g7`/`hm1_g7`): top 5% mean -6.9K, P95 -4.5K,
  median flat; replay IDENTICAL. Dream Land: top 5% mean -3.5K.
- Final on Saffron (`hmf_g7` vs `hm0_g7`): P95 1,550,656 -> 1,541,952; top 5%
  mean -10.3K (SRC -10.7K); median +0.4K.
- Gate ROM (`hmg` vs `ccg`): median +0.6K, P95 +0.5K, top 5% mean -3.5K;
  replay IDENTICAL; native failures 0.

Banked as a tail lever.
