# Residency / storage / audio architecture — read-only investigation (2026-09-22)

Labels: **M** = MEASURED (source named), **E** = ESTIMATE (arithmetic shown), **U** = UNKNOWN. "canon run" =
`artifacts/performance/2026-09-17_p2-2p8-roster-variance/canon-regression-{run.log,memory.json}` (DK/Samus/Link/Kirby,
Dream Land, 60 s, 1,972 presented frames, dldi=ON). "rows" = `.../2026-09-17_p2-2p8-dtcm-hot-scalars/fourcpu-rows.csv`.

**Verdict in five lines**
1. Today every motion change in a 4-distinct-kind match is a storage read: 681 acquisitions, 0 cache hits, anim arena
   **0 B** reserved (M, canon memory.json). The cache is refused by a 128 KiB keep-free rule the match never needs.
2. Full residency of all gameplay motions is **RED**: 1.22–1.58 MB raw BPS1 (M/E) vs ~0.55–0.65 MB of identifiable
   battle-time RAM even with scene overlays (E). SFX full residency is impossible (>2 MB per match, E). BGM is 0.6–3.5 MB/track (M).
3. `gSYFramebufferSets` is **not** battle-dead: 141,440 of its 147,840 B already hold the fighter GX packets (M, code).
4. Every in-match read pays a FatFs cluster walk that grows with ROM size (no fast-seek in the linked libdvm FatFs) and
   blocks ARM9 while calico's ARM7 runs DLDI. A pre-resolved extent map + direct `blkDevReadSectors` is feasible in repo code.
5. Recommended end state: resident per-kind motion bank (hot set + compact) in an overlay-reused region, ARM7-owned
   BGM stream and FGM voices with async fill; motion cold reads bounded and counted. First slice below.

## Q1. Motion path: status change → figatree
- Source: `decomp/BattleShip-main/decomp/src/ft/ftmain.c:4600-4624` picks `motion_desc` from `mainmotion` (gameplay) or
  `submotion` (demo); ShieldPose motions resolve into the resident ShieldPose file (:4617-4619); otherwise
  `lbRelocGetForceExternHeapFile(anim_file_id, fp->figatree_heap)` and `fp->figatree = figatree_heap` (:4621-4624);
  `lbCommonAddFighterPartsFigatree` binds it (:4704) → port parser `src/nds/nds_ft_pose.c:675` (`ndsFtPoseParse`).
  Port wrapper `src/import/battleship_ftmain.c:191-236` (substitutes the resident pack pointer when BattlePack is on).
- `src/port/reloc_backend_assets.c:15219-15276` `lbRelocGetForceExternHeapFile`: token→asset via `ndsRelocAssetIDForToken`
  (:4541) → `ndsRelocP2FighterAnimAssetIDForToken` (:4397-4538). BattleShip passes `&llFT*FileID` pointer tokens, which fall
  to a linear scan of ~690 rows calling `ndsRelocFileID` per row (:4526-4536) — tail +9.5K tk (M, tail_symbols.txt).
- `ndsRelocForceLoadFighterAObj16File` (:14889-15162): (a) resident BattlePack, **Mario/Fox IDs only** (:14919-14924) →
  pointer return, no copy (:14941-14949); (b) raw-cache hit → memcpy into `figatree_heap` + register (:15026-15078);
  (c) miss → `ndsRelocPrepareFighterAnimHeapOverwrite` (:6908-6957: loaded-file memmove + `ndsAObjEvent32ForgetRange`, an
  O(n) scan, n high-water 1,623 (M, canon log), tail +9K) → `ndsRelocAssetLoadFighterStreamClip`
  (`src/nds/nds_reloc_assets.c:1372-1445`): row from the resident 9,320 B BPS1 directory (:1265-1286), then
  `nitroromReadFile` of `animation/ftanim_stream_pack.bin` (:1429) → cache store (:15097) if an arena exists;
  (d) IDs outside the pack → generic O2R loader with swap/fixups/AObj16 normalize (:15121-15156).
- Storage stack: `nitroromGetSelf` uses stdio on `argv[0]` when present (flashcart/DLDI), else ntrcard
  (`C:/devkitPro/calico/include/calico/nds/nitrorom.h:94-99`) → libdvm → FatFs `f_lseek/get_fat/f_read` → DLDI, which
  calico runs on ARM7 (`linker/nds_hot_text.ld:21` "DLDI (on ARM7 WRAM)"); ARM9 waits.
- Warm list: `sNdsR204AnimWarmList` (:13123-13138) = 106 IDs, **Mario/Fox only** (0x1F3-0x319). It is drained before BGM
  (`ndsR2AnimCachePreloadFinish` :14841-14859 via `ndsRelocFinishSceneSetup` :14875-14887). Roster filter (:14319-14379)
  warms nothing when >2 distinct kinds; "P2-3 fighters are still admitted on demand" (:14316-14318). Fox BattlePack carves
  only if Fox is present and ≤2 kinds (:13386-13431).
- DK/Samus/Link/Kirby (M, canon memory.json): carve declined; `animCacheArenaReservedBytes 0`, `ReserveFailCount 1`,
  `FailSkips 1,353`; `animCacheMisses 681, hits 0`, `animStreamReads 676` + `animDirectReads 5`, `ftPoseBinds 677`.
  Cause: `ndsR2AnimCacheArenaEnsure` needs free > pending fighter bytes + `NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE` = 128 KiB
  (:13572-13583, :45, :13266), but the whole-match low-water is 111,680 (M). → 0.345 in-frame reads per presented frame (E: 681/1,972).

## Q2. Motion bytes (BPS1 stream pack, parsed from `assets/animation/ftanim_stream_pack.bin`, M)
Pack: 1,139 clips, dense IDs 0x1F3-0x67F, 2,630,496 B. Sub-motions (Win/Selected/Claps/Pose1P) live in `reloc_submotions`,
not in the pack (`include/nds/generated/nds_fighter_production.generated.h:3408-3410`); Appear1/2 entry clips are AObj32
and excluded (`scripts/generate_battlepack_anim.py:71`). Classes from each `dFT*MotionDescs` table (`decomp/.../ft/ftdata.c`,
e.g. :1427) mapped through the generated ID rows; item/taunt/pipe/victim classes by name regex (±1 clip).

| kind | clips | all main-table bytes | core | items | taunt | pipes (MK only) | victim-only* | Kirby copy |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Donkey | 151 | 332,512 | 274,384 | 39,408 | 2,896 | 7,536 | 8,288 | — |
| Samus | 146 | 294,576 | 237,700 | 39,776 | 2,480 | 4,908 | 9,712 | — |
| Link | 142 | 310,912 | 247,472 | 45,840 | 4,288 | 4,608 | 8,704 | — |
| Kirby | 186 | 392,768 | 237,236 | 46,416 | 3,008 | 5,344 | 9,836 | 90,928 (31 clips) |
| Captain | 148 | 414,976 | 346,960 | 48,368 | 4,144 | 4,528 | 10,976 | — |
| Mario / Fox (pack range) | 141/156 | 339,560 / 346,244 | | | | | | |
*victim-only = ThrownDK/FalconDivePulled/EggLayPulled/ThrownMarioBros/ThrownFox (only if that opponent is present).
Kirby copy by victim (M): DK 12,128; Captain 13,872; Yoshi 13,280; Link 9,520; Samus 8,160; Pikachu 8,176; Purin 7,680
(FTKirbyCopyAnim057/058 = Purin Pound, generated.h:3162-3163); Ness 6,432; Fox 6,096; Mario/Luigi 5,584.
- Stress roster, all main-table clips: **1,330,768 B** (M). Per-match tight set (Dream Land, items on, taunt kept,
  victim/copy only for present kinds): DK 316,688 + Samus 283,748 + Link 300,008 + Kirby 320,468 = **1,220,912 B** (M sum).
  Excluding appeal and items: **1,037,800 B** (M sum).
- Pikachu/Yoshi/Ness are **not in the pack** (Makefile:3986-3996) and still use the generic O2R loader. O2R bytes (M):
  Ness 418,080, Pikachu 407,072, Yoshi 395,056; BPS1/O2R ratio 0.91-0.96 on packed kinds (M) → ~390/381/369 KB (E, ×0.935).
- Worst 4 distinct kinds (shipping admits 12, `builds/build/nds_build_config.h:105-116`): Captain + Kirby + Ness + Pikachu
  ≈ **1.58 MB** raw (E: 414,976+392,768+~390,900+~380,600). Repeated kinds share one bank.
- Compressibility of the real clip bytes (host, per clip): LZ10-style **0.78**, deflate 0.64, whole-kind LZMA 0.45 (M ratios;
  DS decode cost U). Deflate at ~30-60 cyc/B would cost 33-66K tk per 2.2 KB bind (E) — too slow; LZ-class ~5-11K (E).
- Match working set: Mario/Fox both-CPU = 85 distinct of 353 acquisitions, 197,184 aligned B (M, reloc_backend_assets.c:12902-12916);
  scaled to 4 kinds / 681 acquisitions ≈ 170 clips ≈ **360-394 KB** (E). P2 distinct set not instrumented (U).

## Q3. FGM (sound effects)
- Pack `nitro:/audio/fgm_phase_pack_ima.bin`: 573 entries, 6,968,728 B (`include/nds/nds_audio_fgm.h:8,107`); 513 unique data
  ranges = 6,949,928 B; cue p50 8,744 B, p99 53,916, max 59,344; 24 entries carry envelopes (≤18 points) (M, pack parse).
- Cache: 237,568 B BSS (`src/nds/nds_audio_fgm.c:198`) as 8 slots 60K/40K/40K/28K/4×16K (:34-38, :1099-1121); 12 handles
  (nds_audio_fgm.h:110). Victim = smallest free slot that fits, no recency (:1183-1203) → effectively one slot per size class
  (`artifacts/performance/2026-08-13_c-collision-stack/STACK.md:173-186`).
- A play reads storage (a) on every slot miss (:1204-1211 → `ndsAudioFgmReadRange` :1156-1181, NitroROM then stdio fallback)
  and (b) on **every** play of an enveloped cue, hit or miss (:2133-2142). Both are synchronous inside `ndsAudioFgmPlayAtPan`
  (:2067), i.e. on the gameplay frame; then `soundPlaySample`, which "synchronizes with ARM7" (:2183-2186).
- Frequency: canon run `fgmDirectReads 326` / 1,972 frames (M). 2-fighter match: 188 plays, 150 misses (79.8%), 59 distinct
  cues, working set 575,760 B vs 204,800 B cache then (M, STACK.md:129-148). Every non-BGM I/O frame carried an FGM play (M,
  `2026-08-13_c-band-io/BAND_IO_OWNER.md:67-73`).
- Per-kind own sets (FGM+Voice, M pack parse): DK 167,300; Samus 406,024; Link 154,356; Kirby 393,944 (stress sum 1,121,624);
  Captain 256,592; Pikachu 410,140; Ness 297,248; Yoshi 238,012; Mario 179,268; Fox 165,584. Shared: announcer 1,106,876,
  crowd 692,824, hit/impact families ~0.4 MB. A match's reachable set ≈ 2.2-3.3 MB (E: 1.12 own + 0.69 crowd + 0.4 hits +
  in-battle announcer share) → residency impossible; streaming required.
- ARM9 per-frame cost (M, n0409 profile avg tk/fr): `ndsAudioFgmPlayAtPan` 2,349 (tail +7.8K), `ndsAudioFgmUpdate` 1,573;
  update does a u64 divide per live handle (:1805-1807; `__udivmoddi4` 3,850 tk/fr total, shared) and `soundSetVolume`
  PXI sends per envelope/release step (:1859, :1936).

## Q4. BGM
- Format: 22,050 Hz IMA-ADPCM, packets of 16,384 samples = 8,196 B, 2 ping-pong buffers = 16,392 B
  (`include/nds/nds_audio_bgm.h:63,664-669`; buffers `src/nds/nds_audio_bgm.c:919-920`, aligned(4) only). One packet = 743 ms (E).
  Tracks 0.6-3.5 MB (M: Dream Land 711,920; Jungle 3,470,676) → never resident.
- Seam: ARM9 timer0 IRQ (:1323-1355) → mailbox → worker thread does only `soundStart` of the next channel (:1357-1424).
  Refill: `ndsAudioBgmServiceRefills` (:1559-1602) runs **on the main thread** in `ndsAudioBgmUpdate` (:2050), called from
  `ndsAudioBackendUpdate` (`src/port/taskman_seam_battle_host.c:231-240`); each packet = 2 `nitroromReadFile` calls
  (8 B record + payload, :1257/:1274, read at :1127) + `DC_FlushRange` + `soundPreparePcm` (calico PXI to ARM7).
- Cost (M, rows): 152 AUD spike frames in 1,971, spike median **126,752** tk (p90 149,312) vs 3,648 baseline → **~123K tk per
  refill** (3.7 ms, ≈2.2 MB/s effective incl. seek), every ~13 presented frames (gap median 13). 319 BGM reads (M) = 2 × ~156 packets
  over ~116 s wall (E: 1,972 × mean ALL 1,962,295 tk). Older split: CPU part ~34K tk/packet on a 12 MB ROM (M, BAND_IO_OWNER.md:61-63);
  the rest is DLDI wait.
- FatFs fast-seek: the linked FatFs (libdvm-2.1.0 `ff.o` in `C:/devkitPro/libnds/lib/libfat.a`) has no `clmt_clust` and FIL is
  private to libdvm's devoptab → CLMT is not usable without rebuilding the toolchain (inference from symbols). Each backward seek
  walks the chain from the ROM file's head: 447 steps, 13,159 cyc per seek on a 12 MB ROM (M, BAND_IO_OWNER.md:113-119); four-CPU ROM
  30.2 MB, shipping ROM 62.7 MB (M file sizes) → ~2.5× / ~5.2× that walk (E, linear in offset).
- Pre-resolved sector list **is possible in repo code**: calico `blkDevReadSectors(BlkDevice_Dldi, buf, lba, n)` exists on ARM9
  and ARM7 (`calico/dev/blk.h:57-58`: ARM9 buffer must be 32-B aligned); `dvmReadPartitionTable` gives the partition. Boot-time
  read-only FAT32/exFAT walk of `argv[0]` → extent list (<1 KB), validated against `nitroromReadFile`; Slot-1/emulator boot is identity.
- Savings (E): extent map removes walk (~16.5K tk @30 MB, ~34K @62.7 MB) + dvm copy if buffers are 32-B aligned (~15K;
  `armCopyMem32` tail +10.3K M) → ~90K/refill remains as DLDI wait on ARM9. Idle slices (2-4 sectors per VBlank wait, deadline
  743 ms; WAIT p50 253,632 tk M) take it off the critical path but shrink toward 0 idle at the 1.12M gate. ARM7 streaming:
  **0 ARM9 tk**, removes a ~123K spike per ~13 frames (≈9.5K tk/fr avg, E: 152×123K/1,971); ARM7 busy ~3.7 ms per 743 ms (E).

## Q5. RAM during a four-fighter VS battle
- Sections (M, `arm-none-eabi-size -A`): four-CPU lab ELF .main 1,378,436, .main.rw 202,004, .main.bss 898,544, .itcm 32,632
  (full), .dtcm 8,800 + 2,028. Shipping `smash64ds.elf`: .main 1,726,508, .main.rw 252,252, .main.bss 931,152 → **+430,928 B**
  static vs the lab ELF (12 kinds, menu shell, 1P; `builds/build/nds_build_config.h:133,216`).
- Scene split (nm -S -l, path/symbol classification, ±10%, E): shipping non-battle = **491,211 B**: CSS 143,105, 1P 134,269
  (incl. 40,452 of N-fighter/Boss/MMario/GDonkey tables in ftdata.c), MENU 101,152 (incl. ui kit 21,590), OPENING 48,178,
  DIAG 37,819, RESULTS 26,688. Lab ELF non-battle = 220,491 (CSS 74,464, OPENING 45,936, MENU 33,650, RESULTS 27,956, DIAG 37,553).
- Largest BSS (M, lab nm): FGM cache 237,568; `gSYFramebufferSets` 147,840 — **already reused**: 4 × 35,360-word fighter packet
  regions = 141,440 B (`src/nds/nds_renderer_preamble.c:3280-3326`), released for Results (`src/port/taskman_seam_harness.c:334`);
  renderer BSS ≈ 304 KB across preamble 111,264 / assets 61,588 / adapter_matrix 59,120 / native_common 24,425 / adapter_fighter
  21,668 / adapter_stage 15,916 / textures_effects 10,466; BGM 16,392; FGM entries 18,336; BPS1 dir 9,600.
  Texture refresh Large+Small (20,480 B) had high-water 0 in the canon run (M) — other stages U.
- Arena (M canon): taskman arena chosen 1,355,520 (83 page steps below 0x1A7000, `src/port/diagnostics_mp_taskman_state.c:767`,
  search `src/port/diagnostics_taskman_heap.c:117-215`) → static bytes convert ~1:1 into arena (M precedent, reloc_backend_assets.c:12943-12948).
  General-heap low-water **111,680** vs floor 25,600 (margin 86,080); known tenants: compact fighter cores 125,108, ShieldPose
  11,799, ITCommonData 82,976, graphics heap 1,536, anim cache 0; DObj max 203; effect pool 38 (14 active max).
  ~1.0 MB of the arena has no per-owner census (stage/effect/IF files, GObj/DObj pools, FTStructs, figatree heaps) — **U**.
  `figatree_heap` per fighter = largest O2R anim of that kind (`decomp/.../ft/ftmanager.c:199-203,364-368`).

## Q6. Overlays
- None today: no `.ovl.*` segments (readelf program headers), no `ovlInit/ovlLoad*` in src/linker/Makefile. Calico supports ROM
  overlays (`calico/include/calico/nds/arm9/ovl.h:38-58`) with the example `C:/devkitPro/examples/nds/filesystem/nitrofs/overlays/overlays.ld`
  (main-RAM area `INSERT AFTER .main.bss` :81, ITCM area :113, `.ovltable`, PHDR flags 0x200007); overlays load through
  `nitroromGetSelf` (same storage path).
- What a battle/front-end split takes: (1) add overlay PHDRS/SECTIONS to `linker/nds_hot_text.ld` (repo copy of calico ds9.ld);
  (2) partition at object granularity — front-end TUs (mn*, battleship_mn*, nds_menu_shell*, mv*/opening, sc1p*, *AnimSelected,
  results) into `ovl_frontend`; split TUs that mix battle and menu code; (3) scene manager loads/activates before calling any
  front-end entry; add an ELF-relocation checker for battle→overlay references (runtime crashes otherwise);
  (4) **the overlay area sits after BSS and shrinks the heap by its size**, so battle must reuse that address range as a
  "scene region" (motion bank / SFX heads) — otherwise nothing is gained; (5) reload cost ~450 KB ≈ 0.2 s per transition (E, 2.2 MB/s).
- ARM9 boot image limit: `lma9` 0x27C000 (`linker/nds_hot_text.ld:13`); overlays are outside it.

## Q7. RAM budget: all gameplay motions + needed SFX resident, worst four kinds
| row | bytes | label |
|---|---:|---|
| Demand: motions, stress tight / worst 4 kinds (raw BPS1) | 1,220,912 / ~1,580,000 | M / E |
| Demand: same with LZ10 0.78 | ~952,000 / ~1,232,000 | E |
| Demand: SFX reachable per match | ~2,200,000-3,300,000 | E (not resident; stream) |
| Supply: front-end overlay reused as scene region (shipping, excl. DIAG) | ~453,000 | E |
| Supply: general-heap margin above 25,600 floor (lab; shipping U, likely lower) | 86,080 | M |
| Supply: renderer caches retired by precompiled GX | 0-100,000 | E, conditional |
| Supply: framebuffer reuse (only unused tail) | 6,400 | M |
| Supply: FGM cache 237,568 → keep as resident SFX hot set | 0 new | M |
| **Total supply** | **~546,000-646,000** | E |
| **Deficit** stress tight LZ10 / worst LZ10 | **-306K..-406K / -586K..-686K** | E |
Full residency does not fit (RED) unless a motion format reaches ≤~0.41× (worst) / ≤~0.53× (stress) of BPS1 (E: 646K ÷ 1.58M /
1.22M) — LZ gives 0.78; the slice-32 dense bank went the other way (4.46×, `scripts/generate_battlepack_anim.py:6-9`). Fallback:
- **Tier 0** resident per-kind hot set, served zero-copy by pointer (generalize the Fox BattlePack path :14941-14949; also
  deletes heap-overwrite/forget-range/token-scan work: tail +9K/+9K/+9.5K M). Size to ≥98.5% of acquisitions: E ~360-600 KB
  raw (360-394 KB = CPU-match scaling above; 600 KB = assumed human-play union ≤50% of the 1.22 MB tight set, U until measured),
  LZ-compressed cold part. Fits the shipping supply (E).
- **Tier 2** cold clip = extent-map read (no FAT walk, 32-B aligned, whole sectors), per-motion LBA table built at admission.
  E ~22-26K tk/miss on melonDS DLDI (123K per 8.2 KB refill minus walk/copy, scaled to 2.2 KB); real SD latency U (1-3 ms = 34-100K).
- **P99 risk**: 681 acquisitions/match; 1.5% cold → ~10 miss frames/1,972 (0.5%). Misses coincide with status/hit event frames,
  which already own the tail (SPRM p99 225,216, SHDT p99 443,008 M rows), so each miss adds 2-9% of the 1.12M budget exactly where
  P99 lives → **medium-high** unless cold rate ≤~0.5% and counted per match. This contradicts N02.04 "no post-GO demand reads"
  (`docs/p2/native-optimization/04_RESIDENCY_AND_ASSETS.md`) → owner decision needed.

## Q8. ARM7
- Today: calico default `ds7_maine.elf` (`C:/devkitPro/devkitARM/ds_rules:12,39`), no repo ARM7 code. It already runs DLDI
  (inferred: DLDI linked into ARM7 WRAM, ARM9 `_blkDevReadWriteSectors` is a 68 B PXI stub + wait) and the 16-channel sound
  engine (ARM9 `soundPreparePcm/soundStart/soundChSetVolume` are PXI senders with `_soundPxiCheckCredits`, inferred from symbols). maine occupies .wram 39,708 + .wram.bss 23,100 B of
  ~96 KB ARM7-visible WRAM plus 16 KB DLDI (M size -A) → ~17 KB free (E); a custom ARM7 without wireless frees more (U).
- Move to ARM7 (ARM9 cost removed, E from M): (1) **BGM stream** from an LBA extent list: ~123K tk spike per ~13 frames
  (~9.5K tk/fr avg) + timer0 IRQ + worker thread; (2) **FGM voices, envelopes, release ramps, cue fill**: ARM9 posts one PXI word
  per play (User channel 23-30, 26-bit immediate: cue 10 b + pan 7 b + handle 8 b; `calico/nds/pxi.h:42,91`); removes ~4-8K tk/fr
  avg (PlayAtPan 2,349 + Update 1,573 + share of u64 divides) and **all 326 synchronous FGM reads** (misses become ≤1 frame audio
  latency: resident cue heads, e.g. first 512 B ≈ 32-64 ms, cover an ARM7 SD read); (3) no sequencing exists to move (BGM and
  note schedules are pre-rendered). Motion reads cannot move: the parser needs the clip in the same tick.
- Constraints: FIFO 16 words each way (M, pxi.h:42) → events as single words, bulk data via 32-B-line-owned descriptors in main
  RAM; ARM9 D-cache: flush before ARM7/sound DMA reads (already at nds_audio_fgm.c:1214, nds_audio_bgm.c:1279), invalidate
  before ARM9 reads ARM7-written data; never share a cache line between writers; no TCM payloads. One ARM7 DLDI user at a time:
  an ARM7 BGM read (~3.7 ms) delays any ARM9 blkDev request and sound PXI replies queued behind it.

## TOP RISKS
1. Full motion residency is RED (−0.3 to −0.7 MB, E); the N02 "zero post-GO demand reads" contract needs a new format or an owner exception.
2. The shipping ELF carries +430,928 B static vs the lab ELF (M); its four-fighter heap low-water is unmeasured (U) — every figure
   above is from the lab build; overlays are mandatory, not optional, for the shell ROM.
3. Read cost grows with ROM size (FAT walk ∝ offset, M/E); content growth silently worsens every remaining read.
4. Cold reads and FGM misses land on event frames that already own P99 (M correlation evidence BAND_IO_OWNER.md §1).
5. All read costs are melonDS-DLDI (M); real flashcart SD latency/throughput U (could be 2-5× worse).
6. Custom ARM7 is a new build artifact; WRAM ~17 KB free under maine (E); DLDI arbitration between ARM7 stream and ARM9 requests.
7. Overlay split: cross-overlay calls/function tables fail only at runtime; mixed TUs; region reuse must be designed in.
8. Keep-free policy (128 KiB) disables the anim cache on every 4-kind roster (M); admission must use measured low-water + 25,600 floor.
9. Unverified in-match loaders: items/monsters (only 2 item spawns in canon run, M) and Pikachu/Yoshi/Ness generic anim path (U).

## RECOMMENDED FIRST SLICE — "stress-roster match pack" (removes the 681 motion reads; audio next)
1. Instrument (0 RAM): per-dense-ID seen bitmap + counts over BPS1 (1,165 bits) for P2 kinds, dumped by the stress verifier →
   the real distinct set/bytes (today U; E ~170 clips ≈ 360-394 KB).
2. Extent reader (<1 KB RAM): boot-time LBA map of the ROM file; `blkDevReadSectors` into 32-B-aligned buffers for the three
   in-match clients (`nds_reloc_assets.c:1429`, `nds_audio_fgm.c:1165`, `nds_audio_bgm.c:1127`), nitrorom fallback kept.
   Removes FAT walk + dvm copy on all 1,326 reads/match (tail get_fat +17.8K, f_lseek +11K, armCopyMem32 +10.3K M).
3. Match pack: pre-GO (existing barrier :14841) bulk-load the four kinds' clip set into one arena block and answer by pointer
   (BattlePack path generalized per kind; drop the Mario/Fox gate :14919-14924). Admission: measured low-water rule instead of
   128 KiB keep-free. **RAM ≈ 360-394 KB (E)** for the measured set, **1,220,912 B (M)** for the full tight set.
   Funding in the lab ELF: 86,080 margin (M) + front-end region CSS/opening/results/menu 182,938 (E) = ~269 KB → ~90-125 KB short
   for the measured set (E): cover with LZ on cold clips or DIAG BSS (37,553, lab only). Shipping: ~453 KB overlay + margin fits (E).
   Pass criteria: `animCacheMisses`/stream reads 0 after GO, heap low-water ≥ 25,600, P2 kinds' `gNdsK0AfterGo*` counters 0.
Follow-ups: ARM7 BGM stream + ARM7 FGM voices with head-resident async fill (removes the other 645 reads/match), then the
full-set compact-format experiment that decides whether RED can turn GREEN.
