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
