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
