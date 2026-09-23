# P2-2p8 Phase 1 slice 3: lean draws for all four stress kinds, at a cheaper lean draw

Brief: `phase1-slice3-brief.md` (coordinator). Base: 2c (`b20b8f0f2b2`), HEAD `40560c0cf59`, plus uncommitted changes.

- Target `smash64ds-p2-fourcpu-tickhud-hwtri`, build dir `build-p2-fourcpu-tickhud`.
- Final ROM sha256 `1DAA7067CC3DE49A004061101EC8F433BE0216A710C549FE50123BAD4F0686B2` (the `a15-*` arms). After every other build, the canonical re-make reproduced it (section 6).
- Roster: DK slot 0, Samus 1, Link 2, Kirby 3.
- All four-CPU runs used runner slot 9 / GDB 3423 and took 1,972 samples from frame 2 with `-RingDump`.
- `gNdsFtrLeanAdmit = 2` in every arm. The words are poked at the first frame-complete marker, as whole u32 words.
- Route 1 against route 0 on the same ROM is the A/B. Route 2 is the oracle.

## Verdict

- **Every four-CPU gate is met on the final ROM** (gate table below).
  - The replay digest is identical.
  - The route 2 oracle compared 6,592 draws and 15,583,404 words, with 0 mismatches for every kind.
  - Native failures stay at 39.
  - FTR falls (P50 -36%, P95 -52%, P99 -12%) and WORK-H follows it one for one (P50 -7.9%, P95 -7.7%, P99 -4.5%). Every frame range is faster.
- **Coverage.** The lean path now draws all four stress kinds, in every program the match reaches:
  - DK: 96.4% of draws.
  - Samus: 98.9%.
  - Link: 97.6%.
  - Kirby: 99.0%.
  - Of the 140 draws it declines, the old path re-records 126 and replays 14. Section 1 gives each reason.
- **The lean draw is cheaper on every kind** (guard + kernel + patch + submit, same ROM, slice 1 forms against slice 3):

  | kind | before | after |
  |---|---:|---:|
  | DK | 57.5K | **41.4K** |
  | Samus | 50.0K | **34.7K** (slice 1's own ROM: 55.1K) |
  | Link | 68.4K | **50.1K** |
  | Kirby | 45.9K | **29.6K** |

  - Slow kernel joints: 34 to 51% before, **0.0 to 0.4%** after. The only builder joints left are 150 Samus animation-lock joints, the counted exact fallback.
  - The guard is **3.2 to 4.2K**, down from 6.4 to 11.0K.
  - The submit cleans 2.5 KB per draw, down from 9.5 KB.
  - The DMA wait is priced at **0**: no wait in 6,722 submits.
- **Not reached:**
  - The kernel runs **855 to 954 ticks/joint** (1,178 to 1,298 before) against the spec's ~450. Most of the rest is cache misses (section 2).
  - The guard is not yet only the tuple compare.
- **D3 (1P) is blocked.** The first 1P battle overflows the general heap while it loads, before the admission runs, so bank D is never taken.
  - The same lab ROM built from this slice's start-of-work sources overflows at the same point, so the overflow predates the slice.
  - Slice 3 makes it 8,192 B worse: its code costs the campaign arena two 4 KiB pages.
  - The lab default does reach the non-tick-HUD target (`admit=2` read in the battle). Section 3 has the evidence.

## Gate table (final ROM `1DAA7067`, admit 2 in every arm, route 1 vs route 0)

| gate | route 0 | route 1 | status |
|---|---:|---:|---|
| replay digest | - | **IDENTICAL**, 1,972 frames | met (`digest-a15-route0-vs-route1.json`; route 2, the slow-7 arm and slice 1's ROM are identical too) |
| route 2 oracle, per kind | - | DK 1,660 / Samus 1,663 / Link 1,721 / Kirby 1,548 draws compared, **0 mismatches** each | met (15,583,404 words; 0 in all 8 word classes; 0 keys moved) |
| native failures | 39 | **39** | met: not increased (route 2 and slow 7: 39) |
| FTR P50 / P95 / P99 | 358,080 / 915,638 / 1,170,867 | **229,056 / 435,251 / 1,025,600** | met: -36.0% / -52.5% / -12.4% |
| WORK-H P50 / P95 / P99 | 1,642,592 / 2,474,362 / 3,034,156 | **1,512,928 / 2,284,672 / 2,896,626** | met: -7.9% / -7.7% / -4.5% |
| FTR / WORK-H mean | 385,318 / 1,707,860 | **251,889 / 1,573,727** | met: WORK-H follows FTR (paired per frame: FTR -133,497, WORK-H -134,201) |
| bands FTR P40-60 / P90-95 / P95-99 / P99+ | 356,700 / 696,673 / 906,707 / 911,677 | **233,296 / 423,118 / 607,814 / 868,854** | met: every band lower |
| bands WORK-H P40-60 / P90-95 / P95-99 / P99+ | 1,643,812 / 2,320,179 / 2,709,564 / 4,505,907 | **1,514,178 / 2,113,648 / 2,485,954 / 4,363,933** | met: every band lower |
| per-range means FTR (2-195 / 196-797 / 798-1043 / 1044-1974) | 132,298 / 424,752 / 444,594 / 397,015 | **114,703 / 273,781 / 272,281 / 261,044** | met: no range slower |
| per-range means WORK-H (same ranges) | 1,086,120 / 1,674,481 / 1,885,939 / 1,784,688 | **1,068,794 / 1,522,113 / 1,713,556 / 1,648,092** | met: no range slower |
| VBlank frames (2 / 3 / 4 / 5+) | 5.6% / 46.1% / 38.4% / 9.9% | 6.0% / **62.7%** / 25.6% / **5.6%** | - |
| fighter uploads after GO / admit failures | 0 / 0 | 0 / 0 | the 2c admission is intact in every arm: 119 entries, 0 outside keys, 0 direct rejects, bank D taken once, 0 BG3 refused writes, 0 missed exits |

- **Where the time went.** Paired per-frame means, route 1 minus route 0:
  - FTR -133,497 and WORK -134,320.
  - OTHR +7,000, all of it WAIT (+6,996). Frames finish earlier inside the same VBlank.
  - STG, SRC, MISC, AUD, HUD, GCRA and SINT each moved by less than 600.
  - No other bucket absorbed the saving, and DMA wait is 0.
- **Route 2 oracle, in full.**
  - 6,592 of the 6,722 lean draws were also replayed by the old path, and the two word streams were compared.
  - The other 130 lean draws fell on frames where the old path re-recorded instead of replaying. Each re-record was compared against the lean copy: 0 words differ outside a patch site (`rec_under_hit [130, 0]`).
  - The forced source compose missed 0 times.
  - The plan verifier re-derived all 534 re-tuples, with 0 mismatches.
- Files: `gates-a15-route0-vs-route1.txt`, `kinds-a15.txt` (per-kind counters of all four final arms), `cost-per-draw-a15.md`.

## 1. Coverage (D1)

Final ROM, route 1 (`a15-route1`):

| kind | attempts | lean draws | share | adoptions | programs drawn lean | declines |
|---|---:|---:|---:|---:|---|---|
| DK | 1,725 | 1,663 | 96.4% | 62 | 0 (1,663) | rebind 56, tint_set 5, adopt_pending 1 |
| Samus | 1,685 | 1,666 | 98.9% | 19 | 0 / 1 / 2 / 3 (1,619 / 5 / 3 / 39) | rebind 14, tint_set 4, adopt_pending 1 |
| Link | 1,824 | 1,781 | 97.6% | 16 | 0 / 1 (1,753 / 28) | uncacheable 16, rebind 12, material 11, tint_set 3, adopt_pending 1 |
| Kirby | 1,628 | 1,612 | 99.0% | 6 | 0 / 13 (1,552 / 60) | material 10, rebind 3, tint_set 2, adopt_pending 1 |

**How each kind is drawn.**
- **DK.** His cross slots are patched on the copy, and his worlds are the source seam's Q43.20.
- **Link.** Texgen runs through the ported `PatchTexgen`, with a site-to-slot map built once at adoption (`ndsFtrLeanPatchTexgenMapped`).
- **Kirby.** His programs and his copied-power head are keyed in the tuple (`head_key`).
- **Samus.** She is drawn in all four programs the match reaches.
- **Detail.** The match draws every fighter at low detail (`k_high_draws` 0 on every kind). The detail is part of the tuple, so a high-detail draw adopts its own copy.

**The tuple** is kind, detail, heap generation, root, status generation, plan hit, program and root count.
- A status change first tries a **re-tuple**: if the event-derived collection and each root's DL, matrix DObj and material DObj equal the plan's, the plan is re-keyed and the draw stays lean (534 re-tuples, 4.5K ticks each).
- The skeleton is proved in the kernel. It checks each kept joint's parent link against the live tree, which replaces slice 1's per-draw topology hash.

**Two entries per instance (material variants).**
- A copy holds its base entry plus up to four learned material variants. Each variant is at most 8 words outside the patch sites, stored in the copy's tail.
- The draw switches entries per material identity instead of re-recording: 21 variants learned (76 words), 119 switches, 0 learn failures.
- A new material is drawn once by the old path (`material` decline), and its variant is learned from that recording.

**Texture-site patches replace the identity guard.**
- The material identity is proved through its writers: a per-slot texture-part serial (the `ftParamSetTexturePartID` / `ftParamResetTexturePartAll` shims, counted only when an id moves) and a watch list of animated MObjs.
- The full identity is re-proved only after a draw the old path had to re-record, and in routes 2 and 3.
- Texture ids and tint tiles are patched at their sites on the copy.

**Why each remaining decline exists.** The 140 declined draws split into 126 old-path records and 14 old-path replays.
- **`adopt_pending`** (4 records): an instance's first draw. There is no copy yet, and the copy is adopted from the recording this draw makes.
- **`rebind`** (DK 56, Samus 14, Link 12, Kirby 3; 85 records): `ndsFighterRendererInvalidateMaterialCachesForSlot` replaced the fighter's DObj/MObj graph (a model-part swap), and the same call invalidates the old path's packet. The copy's bindings point into the old graph, so the draw records and the copy is re-adopted.
- **`material`** (Link 11, Kirby 10; 21 records): a material identity no entry of the copy holds yet. It is drawn once by the old path, then learned.
- **`uncacheable`** (Link 16 records): the draw plan refuses the draw (`refusals plan 16`), so neither path has a packet.
- **`tint_set`** (14; these are the 14 old-path replays): the fighter tint set was rebuilt (`gNdsR2FighterTintSetGeneration`), and the copy's tint tiles belong to the old set.
  - The old path replays its packet on these draws and re-derives the tiles.
  - The lean copy could do the same. That is not done: it is 14 draws per match.

## 2. Per-draw cost (D2)

**The arms.**
- **Before** is `a15-slow7`: the same ROM with `gNdsFtrLeanSlow = 7`, which puts every part back into its slice 1 form.
  - Kernel: scale and warm joints take the source builder.
  - Guard: preamble hash, residency proof and link hash every draw, and no re-tuple. With no re-tuple, coverage drops to about 90% in this arm.
  - Submit: light re-derived every draw, whole list cleaned.
- **Slice 1** is `b0-route1`: slice 1's own ROM, which drew Samus only.
- **After** is `a15-route1`.

Mean ticks per lean draw:

| arm | kind | lean draws | guard | kernel | patch | submit | sum | kernel/joint | slow joints |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| slice 1 (`b0`) | Samus | 1,485 | 9,707 | 26,245 | 8,994 | 10,121 | 55,068 | 1,088 | 32.3% |
| before (`a15-slow7`) | DK | 1,575 | 11,046 | 30,539 | 10,722 | 5,225 | 57,533 | 1,220 | 36.4% |
| before | Samus | 1,532 | 10,201 | 26,522 | 8,192 | 5,044 | 49,959 | 1,178 | 33.9% |
| before | Link | 1,613 | 9,412 | 35,218 | 17,776 | 5,949 | 68,355 | 1,298 | 42.2% |
| before | Kirby | 1,467 | 6,442 | 27,196 | 7,886 | 4,403 | 45,927 | 1,269 | 50.6% |
| **after** (`a15-route1`) | DK | 1,663 | **4,202** | **23,290** | **9,287** | **4,610** | **41,389** | **930** | **0.0%** |
| **after** | Samus | 1,666 | **3,659** | **20,014** | **6,487** | **4,517** | **34,677** | **887** | **0.4%** |
| **after** | Link | 1,781 | **3,732** | **25,876** | **15,420** | **5,049** | **50,078** | **954** | **0.0%** |
| **after** | Kirby | 1,612 | **3,246** | **18,379** | **4,190** | **3,815** | **29,630** | **855** | **0.0%** |

- The per-draw bookkeeping outside the four parts is 2,145 ticks (2,155 before). Slice 1 counted it inside submit.
- Adoption costs 56.9K ticks, 103 times per match. Of that, 38.6K is the copy.
- A variant learn costs 48.7K, 21 times per match.

### Kernel

Joint classes on the final ROM. Every class other than lock/convert/xobj/nogobj computes the source builder's cells exactly:

| class | DK | Samus | Link | Kirby | before | after |
|---|---:|---:|---:|---:|---|---|
| fast (unit scale, cold cache) | 26,503 | 24,889 | 28,207 | 17,278 | integer TraRotRpy | unchanged |
| scale (scale != 1) | 1,519 | 6 | 4,865 | 6,333 | source float builder | `syMatrixTraRotRpyRSca`'s own integer cells (same sin table, the same s32 products; exact for \|scale\| < 32, larger takes the builder: 0 seen) |
| warm (warm gameplay cache, `transform_update_mode != 0`) | 13,628 | 12,549 | 15,257 | 11,029 | source builder | `ndsRendererAdapterF2LFixedWExact`'s conversions, cell for cell (a value they cannot convert takes the builder as `convert`: 0 seen) |
| lock (animation locks) | 0 | 150 | 0 | 0 | source builder | source builder with per-depth accumulators: **the counted exact fallback** |
| convert / xobj / nogobj | 0 | 0 | 0 | 0 | source builder | source builder |

**Other kernel changes** (all exact: route 2 compared every output word):
- Worlds sit on a per-depth stack in the kernel frame, not in a per-joint array. The ARM946 D-cache does not allocate on write.
- Output is written straight into the copy's LOAD4x4 modelview parameters. Q20.12 worlds are kept only for Link's texgen group roots.
- Rounding is branchless.
- Joints no drawn binding hangs from are pruned when the table is built.
- A parent-link check replaces the topology hash. A failed check re-collects the joint table once and composes again (76 times, all recovered).

**Why ~450 ticks/joint is not reached.**
- A lab ROM with phase timing compiled in (`a14-ktime`, `748668D1`, `kinds-a14-ktime.txt`) attributes a joint as follows. Each phase figure includes about 50 ticks of timer read, which accounts for that ROM's +200 ticks/joint:
  - Fast local: 469. The joint's DObj (3 to 4 lines) and FTParts line miss, and so do about six sin-table lines. The sin table is 4 KB, the whole D-cache.
  - Compose: 291. 27 64-bit multiply-accumulates for the basis and 9 for the translation, with rounding.
  - Output: 191, averaged over all joints. 16 uncached stores per drawn binding.
- About 400 ticks of each joint is misses, so the kernel is memory-bound. The next levers are outside this slice:
  - a per-joint local memo keyed on the DObj's raw TRS bits;
  - a sin/cos table in fast memory;
  - fewer, wider stores.

**ITCM: not moved.** The brief allows the move only if an eviction pays for itself in route 0 and route 1. The kernel never runs in route 0, so any eviction is a pure route 0 loss. Slice 1 measured +20,000 FTR ticks/frame mean in route 0 for the one candidate.

### Guard

Parts per lean draw:

| part | before | after | what changed |
|---|---:|---:|---|
| tuple | 862 | 1,220 | now includes the re-tuple (534 × 4.5K spread over all draws) |
| topology | 1,572 | 144 | link hash every draw → the kernel's parent-link check (hash kept for routes 2/3) |
| camera | 512 | 449 | camera matrices fetched once per frame per camera |
| identity | 3,335 | 591 | identity hash every draw → texture-part serial plus watch list |
| preamble | 1,210 | 467 | preamble hash every draw → once per memo fill (4,808 skips, 1,928 checks) |
| packet | 1,468 | 520 | residency proof and touch every draw → skipped when every texture of the copy is admitted (all 103 adoptions) |
| shuffle | 273 | 240 | the old path's own shuffle latch |

**The guard is not yet only the tuple compare.** What remains beyond the tuple is either a proof no patch covers or a latch the old path also runs:
- camera and shuffle: the latches;
- identity: the serial and watch compare;
- preamble: the memo serial;
- packet: the texture fence and tint set.

### Patch and submit

- **Patch**, per lean draw, before → after:
  - tint tiles 1,836 → 463: skipped per bind when the tile's colour is unchanged;
  - ApplyTint 2,061 → 1,624: skipped when the preamble memo is unchanged and the modulate is the same;
  - light 843 → 204: patched only when the light moved, 167 of 6,722 draws.
  - projection 1,851 → 1,982: the matrix is compared at root 0 and stored at every root only if it moved. It moved on every draw, since the camera moves.
  - texgen 2,296 → 2,377: Link only, about 9.1K per Link draw.
  - The modelviews cost nothing in patch: the kernel writes them.
- **Submit.** Only the lines the patches wrote are cleaned: 2,531 B per draw, against 9,455 B for the whole list. Flush 1,957 → 1,499.
- **DMA wait, priced.** A lean submit waited on DMA0 0 times in 6,722 submits (0 ticks), and the next packet writer waited 0 times. The GE was busy at frame end in 0 of 1,973 samples.

## 3. 1P check (D3): blocked by a general-heap overflow that predates this slice

**Lab ROM.**
- Built as `smash64ds-p2-shell-freeplay-hwtri`, which is the published `smash64ds` configuration with its output kept under `builds/`. The published root ROM was not touched.
- Own dir `build-p2p8-s3-1p`, with `NDS_P2_MENU_WALK=1 NDS_FTR_LEAN_ADMIT_LAB=1 NDS_FTR_LEAN_ADMIT_DEFAULT=2` (`build-1p.log`).
- Config: `NDS_TICK_HUD 0`, `NDS_FTR_LEAN_ADMIT_DEFAULT 2u`, 1P game 1, compact fighters 1, fighter packets 1, profile level 0.
- **The default reaches the target.** The battle reads `admit=2`.

**Probe.** `probe-p2-campaign.ps1 -TransitionProof` ran on runner slot 8 only.

1. **Tries 1 and 2** (180 s and 480 s ceilings) stalled in the 1P intro (scene 14): `1p-campaign-probe-try1-*`, `-try2-*`.
   - The probe's `break *ndsRendererRecordNativeFailure` stops the target at every native failure.
   - The intro now records a sprite failure (domain 3, `NO_PROGRAM`, status `0x20000`: a CI4 SObj) per SObj per frame, so it crawled.
   - That failure class is in the sprite path, which this slice does not touch.
2. **Try 3** ran with the breakpoint disarmed after its one intro print (`1p-campaign-probe.txt`).
   - The intro finished and the first 1P battle (vs Link) was entered inside 67 s: `CPVRAMD at=first-battle-enter scene=52 admit=2 lent=0 takes=0 returns=0 refused=0 missed=0`.
   - The battle's setup then overflowed the general heap. After both fighters' packs and externs, 51,872 B were free; the run halted at a 1,332 B request with 450 B free (`VERDICT: FAILED general-heap-overflow`).
   - This happens before the scene's texture preparation. The admission never ran, and bank D was never taken.
   - The four words therefore cannot be read across a stage transition on this tree.

**The overflow predates slice 3.**
- The same lab ROM was built from this slice's start-of-work sources (`tools/build-pre-slice3-1p.ps1`, own dir `build-p2p8-s3-1p-pre`).
  - The ten files were swapped while the build lock was held, and restored afterwards. The restore checked all ten byte-identical: 0 mismatches.
  - This ROM ignores the lab flags (its Makefile predates them), so it runs word 0.
- Probed the same way (`1p-pre-slice3-probe.txt`), it overflows at the same point:
  - 60,064 B free after the same packs;
  - a halt at a 144 B request with 136 B free.
- **Slice 3 makes it 8,192 B worse.** The shell image grows +9,768 B loaded (+12,616 B code, -2,848 B bss; `ramdelta-1p-shell-pre-vs-slice3.txt`), and the campaign arena, which the taskman sizes in 4 KiB pages, shrinks from 904,704 to 896,512 B.
- **Arena history of this campaign ROM,** from the probe's own `CPLINE` records:

  | ROM | arena |
  |---|---:|
  | 2026-09-14 | 962,304 |
  | 2026-09-17 | 937,728 (the first battle then used 919,988 B, low-water 17,740 B) |
  | 2026-09-19 | 921,088 |
  | slice 3 start | 904,704 |
  | slice 3 | 896,512 |

  At the Sep 17 battle's use, the start-of-work tree is about 15.3 KB short and slice 3 about 23.5 KB short, before any margin for the 25,600 B GObj-cap floor.

**Instrument changes (all read-only).**
- `scripts/menus/probe-p2-campaign.ps1`:
  - optional `CPVRAMD` lines, printed at first battle entry, stage clear, continue, next intro, next battle, and at every `ndsPlatformVramTakeBankD` entry;
  - the native-failure breakpoint disarmed after each print and re-armed at stage clear;
  - the timeout ceiling raised from 180 to 600 s.
- `src/nds/nds_platform.c`: with `NDS_FTR_LEAN_ADMIT_LAB` and no tick HUD, the bank D words are cleaned every frame so GDB reads memory, not a stale D-cache line.

## 4. RAM

- **Static, tick-HUD image** (`ramdelta-tickhud-2c-vs-final.txt`, 2c's canonical ELF against the final ELF): **+13,328 B** loaded, made of +14,672 B code and -1,344 B bss.
  - **Code added:** adoption and variants (+4.3 KB), the kernel (+2.3 KB), the patch with its texgen map (+2.8 KB), and the draw itself, inlined into `ndsFighterDisplayContractSubmit` (+3.6 KB).
  - **Bss removed:** slice 1's world scratch (4,096 B), single instance (1,532 B), inputs (1,248 B), projection and config.
  - **Bss added:** four instances (2,720 B), slot state (+480 B), 24-bind tint tables in the packet structs (+896 B), and the lab counters (+1,528 B, tick-HUD only).
- **Static, shell image** (the 1P lab ROM, no tick HUD): **+9,768 B** loaded (section 3). The lab counters and oracle compile out there.
- **Heap, final four-CPU ROM.**
  - General-heap low-water: **54,020 B** (2c: 66,308).
  - libc top-chunk minimum: **18,976 B** (2c: 20,032).
  - Margin: 21,252 B above the 32,768 B safety floor, and 28,420 B above the 25,600 B GObj-cap floor.
- **The low-water moves in 4 KiB steps.** Every figure this slice read is 772 mod 4,096: 66,308, 58,116, 54,020, 41,732 and 33,540.
  - The lab ROM with kernel timing compiled in (about 300 B more code) read **33,540**, five steps lower.
  - That drop is why the timing is compiled only with `NDS_FTR_LEAN_KTIME=1`. Without it, the final ROM reads 54,020 again.

## 5. Changes

- **`src/nds/nds_ftr_lean_kernel.c`**
  - The slice 3 kernel: scale and warm classes, per-depth stack, output straight into the list's LOAD4x4 parameters, branchless rounding, parent-link check.
  - Phase timing, compiled only with `NDS_FTR_LEAN_KTIME`.
- **`src/port/renderer_fighter_lean.c`**
  - Instances for four kinds, the tuple and re-tuple, the identity proof (texture-part serial, watch list), the preamble memo proof, and the per-frame camera fetch.
  - The kernel call with rebuild-and-retry, inputs written in place, and material variant learning.
  - `NDS_FTR_LEAN_ADMIT_LAB` lets the admission default through on a non-tick-HUD lab target.
- **`src/nds/nds_renderer_native_common.c`** (the adopted copy)
  - Kind, identity rekey, kept-copy re-adoption, variants (select, switch, learn), and the texgen site map.
  - Dirty-line flush, per-bind tint skip, light-when-moved, and the projection compare.
  - Per-kind oracle counters, and the variant-aware oracle key.
- **`src/nds/nds_renderer_preamble.c`**: 24 tint binds per packet, up from 16; Kirby needs 19. The unused bind `name` is removed, and `tint_bind_seen` is counted.
- **`src/port/renderer_adapter_fighter.c`**: per-kind head timing (lab), the memo-fill serial, and the packet-hit count passed to the lean path.
- **`src/port/renderer_adapter_matrix.c`**: route 2 forces the source compose for all four kinds, animation-lock draws included.
- **`src/port/reloc_backend_compat_shims.c`**: the texture-part serial hook in `ftParamSetTexturePartID` / `ftParamResetTexturePartAll`. It counts only when an id changes.
- **`include/nds/renderer_fighter_lean.h`**: the slice 3 counters and API, the `gNdsFtrLeanSlow` cost-A/B word (bits 1 / 2 / 4, and 8 for kernel timing), and the `NDS_FTR_LEAN_KTIME` default.
- **`src/nds/nds_platform.c`**: the D3 lab flush (section 3).
- **`Makefile`**: `NDS_FTR_LEAN_ADMIT_LAB` (forced 0 for the published targets) and `NDS_FTR_LEAN_KTIME` (forced 0 without the tick HUD). Both are lab-only and default 0.
- **Scripts**
  - `scripts/sample-tick-hud-buckets.ps1`: when the first sampled frame precedes `-StartFrame` by no more than the ring's stop skew, the sampler warns instead of throwing. That is a label skew, not a ROM behaviour.
  - `scripts/menus/probe-p2-campaign.ps1`: section 3 describes the changes.

## 6. Rules and canonical sha

- No subagents, and no git commands.
- Every build held `builds/.p2p8-build.lock`, one target, one build dir, no `-j`:
  - `tools/build-s3.ps1` for the four-CPU and 1P builds;
  - `tools/build-pre-slice3-1p.ps1` held it across its whole source swap.
- No source was edited under a running build.
- Four-CPU runs used runner slot 9 only. The 1P probes used slot 8 only.
- Nothing under `decomp/` changed.
- **Route 0 is today's draw.**
  - Every lean-path change sits behind a route word or a lab flag. The exceptions are the 24-bind tint table and four counters on shared paths: the texture-part serial, the memo-fill serial, the record serial and the tint-bind count.
  - Route 0 reads 6,606 packet hits and 256 records.
  - Its digest is identical to routes 1 and 2 and to slice 1's ROM.
- **Canonical sha** (`canonical-sha-after-all-builds.txt`).
  - The 1P and start-of-work shell builds regenerate the shared particle-bank outputs.
  - The first re-make compiled the five particle-bank dependents before regenerating their header, and gave `6B74EAF2...`: the known hazard 2c documented.
  - Forcing the five dependents reproduced the final ROM exactly: `1DAA7067...`.
- All outputs are in this folder.

## 7. Open

1. **1P battles overflow the general heap** (section 3). This predates the slice, and slice 3 adds 8 KB to the deficit. D3's read needs that heap back first.
   - While route 0 is the shipping draw, the lean draw code of slices 1 to 3 is dead weight in the shipping image. This slice alone accounts for +9.8 KB of it.
   - Compiling the lean draw only into lab builds until it becomes the draw would return that weight, at the cost of lab and ship images differing.
2. **The kernel** runs at 855 to 954 ticks/joint against the ~450 budget. It is memory-bound; section 2 lists the levers.
3. **The guard** is 3.2 to 4.2K, not yet the tuple alone (section 2).
4. **`tint_set` declines** (14 per match) could re-derive the tiles as the old path does.
5. **The 1P intro's per-frame sprite native failure** (domain 3, `NO_PROGRAM`, a CI4 SObj) is outside this slice.

## Files

- **Final arms:** `a15-route0`, `a15-route1`, `a15-route2` and `a15-slow7`, each as `*-rows.csv`, `*.json` and `*-run.log`.
- **Lab arm:** `a14-ktime` (kernel phase timing).
- **Slice 1 baseline:** `b0-route1`, `b0-route2`.
- **Development arms:** `a1` to `a13`.
- **1P:** `1p-campaign-probe*.txt` / `*-run.log`, `1p-pre-slice3-probe*.txt`, `build-1p.log` and `build-1p-pre-slice3.log`.
- **Tools:**
  - `tools/run-s3.ps1`: the sampler wrapper; `-Slow` pokes `gNdsFtrLeanSlow`.
  - `tools/build-s3.ps1`: the build wrapper.
  - `tools/build-pre-slice3-1p.ps1`: the D3 attribution build.
  - `tools/gates3.py`, `tools/kinds3.py`, `tools/cost3.py` and `tools/ramdelta3.py`.
- **Outputs:**
  - `gates-a15-route0-vs-route1.txt`, `kinds-a15.txt`, `kinds-a14-ktime.txt` and `cost-per-draw-a15.md`;
  - `ramdelta-tickhud-2c-vs-final.txt` and `ramdelta-1p-shell-pre-vs-slice3.txt`;
  - `digest-*.json` and `canonical-sha-after-all-builds.txt`.
