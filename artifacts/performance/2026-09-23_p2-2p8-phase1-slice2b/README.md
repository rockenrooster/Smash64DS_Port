# P2-2p8 Phase 1 slice 2b: texture residency (bank D + admission) and the lean BSS diet

Brief: `phase1-slice2b-brief.md` (coordinator). Plan: slice 2a README section 3-4 (approved).
Target `smash64ds-p2-fourcpu-tickhud-hwtri`, build dir `build-p2-fourcpu-tickhud`. The tree is HEAD `982b9fc7b84` plus uncommitted changes.
Final ROM sha256 `4E02C2088438496FDF89DAF5FC40B3B7B4C9BE4542B4F7C12781A21C4A239232` (`f-*.json`). Roster: DK slot 0, Samus 1, Link 2, Kirby 3.
All runs: sampler on runner slot 9 / GDB 3423, 1,972 samples from frame 2, `-RingDump`, route 0. The words are poked at the first frame-complete marker as whole u32 words.

## Verdict

- **Word 2 is a large, lossless win on the stress roster, and bank D alone carries it.**
  - WORK-H P95 falls from 4,207,754 to 2,527,139 (-40%).
  - FTR P95 falls from 2,616,477 to 944,074 (-64%).
  - The P0 episode (frames 798-1043) is gone: MTEX drops from 33,942 to 140 per frame, and FTR from 2,645,564 to 450,850.
  - Link's 254 entry-burst failures are gone (native direct rejects 254 to 0).
  - The replay digests are identical, and every frame range is faster.
- **The admission works, but this tree cannot afford to run it whole.**
  - On a 245-slot lab ROM, the whole stress-roster set was admitted: 114 entries, 66,272 B. DK, Samus and Link then uploaded **zero** textures outside the admitted set.
  - Two memory limits stop it on the shipping-shaped ROM, and both sit outside this slice's files:
    - **Cache slots.** Holding the set needs about 192 cache slots instead of 124, which is +19 KB of static RAM. The four-CPU general-heap low-water then fell to 25,620 B, 20 B above the GObj-cap floor.
    - **The libc reserve.** libnds keeps each texture's records in newlib malloc. Only about 15 admissions fit before the guard floor.
  - So the final ROM keeps 124 slots. It admits 15 of DK's records and leaves the rest on demand.
  - Three gates therefore stay open on the stress roster: admit failures 0, keys outside the list 0, and uploads after GO 0. The details are in the gate table.
- **Native failures are 319, not 0.** All 319 are Kirby's JumpAerialF1 (status 223) native-program rejections. They are identical with word 0 and are not a VRAM cause. Frames 150-200 now have 0 failures.
- **The BSS diet saves 2,780 B in the shipping image.** The admission adds 1,532 B, so the net shipping static RAM delta is **-1,248 B** (nm, non-tick-HUD shell ELF).

## Gate table (final ROM, word 2 vs word 0, same ROM)

| gate | word 0 | word 2 | status |
|---|---:|---:|---|
| native failures, whole match | 573 | **319** | open: all Kirby status 223 (JumpAerialF1), `REJECTED_PROGRAM`, the same 319 at word 0; not VRAM |
| native failures in entry frames 150-200 | 254 (Link AppearL) | **0** | met (per-stop counter reads 0 until frame 320) |
| native direct rejects | 254 | **0** | met |
| `fighter_uploads_after_go` | 408 (224,512 B) | **18** (8,512 B) | open: libc-capped admission (15 of 123 stress records) |
| admit failures | - | **1 latch** | open: the libc-floor stop (`nNDSFtrLeanAdmitFailLibc`, top chunk 16,320 B); 108 records left on demand; 0 resolve / asset / format failures |
| recorded keys outside the admitted list | - | **61** (6 / 21 / 28 / 6) | open: every witnessed key is in the generated table (`cover.py`); they are simply not admitted |
| keys outside, whole admission (245-slot lab ROM `m7`) | - | **0 / 0 / 0 / 4** | Kirby's 4 are not in the table (see Open) |
| MTEX, frames 798-1043 (mean per frame) | 33,942 | **140** | met |
| replay digest | - | **IDENTICAL**, 1,972 frames | met (`digest-f-admit0-vs-f-admit2.json`, also vs admit 1 and the census arm) |
| no frame range slower | - | all four ranges faster | met (frame 3 is the lab admission frame, listed separately) |
| BG3 refused writes while D is lent | - | **0** | met; BG3 had 0 opaque pixels at the take |

### Performance (ticks = cpuGetTiming units)

| metric | word 0 | word 1 | word 2 |
|---|---:|---:|---:|
| WORK-H P50 | 1,690,816 | 1,660,928 | **1,648,576** |
| WORK-H P95 | 4,207,754 | 2,752,470 | **2,527,139** (-40.0%) |
| WORK-H P99 | 4,742,153 | 3,763,850 | **3,162,301** (-33.3%) |
| WORK-H mean | 2,047,182 | 1,766,831 | 1,712,685 |
| FTR P50 | 364,832 | 362,112 | 360,224 |
| FTR P95 | 2,616,477 | 1,173,773 | **944,074** (-63.9%) |
| FTR P99 | 2,833,358 | 2,235,037 | **1,399,902** |
| FTR mean | 723,482 | 451,706 | 398,656 |
| 2-VBlank frames | 113 (5.7%) | 111 (5.6%) | 110 (5.6%) |
| 3-VBlank frames | 803 (40.7%) | 871 (44.1%) | 906 (45.9%) |
| 4-VBlank frames | 591 (30.0%) | 736 (37.3%) | 748 (37.9%) |
| 5+-VBlank frames | 466 (23.6%) | 255 (12.9%) | **209 (10.6%)** |

Band means (bands are WORK-H ranks within each run):

| band | FTR w0 | FTR w2 | WORK-H w0 | WORK-H w2 |
|---|---:|---:|---:|---:|
| P40-60 | 362,182 | 362,734 | 1,693,354 | 1,649,590 |
| P90-95 | 2,609,592 | 748,987 | 4,098,611 | 2,356,347 |
| P95-99 | 2,624,104 | 978,317 | 4,379,084 | 2,771,201 |
| P99+ | 4,447,722 | 1,507,306 | 6,202,970 | 3,903,453 |

Per-range means. Deltas are paired by frame, which is valid because the digests are identical. Frame 3 is excluded from its range.

| frames | FTR w0 | FTR w2 | WORK-H w0 | WORK-H w2 | paired WORK-H | MTEX w0 | MTEX w2 |
|---|---:|---:|---:|---:|---:|---:|---:|
| 3 (lab admission frame) | 18,496 | 18,496 | 986,368 | 6,927,552 | **+5,941,184** | 0 | 237,184 |
| 2-195 (entry) | 388,372 | 176,491 | 1,345,034 | 1,133,835 | -211,199 | 16,632 | 3,853 |
| 196-797 | 462,371 | 447,342 | 1,713,569 | 1,697,750 | -15,819 | 737 | 411 |
| 798-1043 (P0 episode) | 2,645,564 | 450,850 | 4,180,484 | 1,895,267 | **-2,285,217** | 33,942 | 140 |
| 1044-1974 | 454,382 | 399,848 | 1,845,697 | 1,788,577 | -57,120 | 2,325 | 125 |

**Frame 3** carries the lab-flow admission: 5.9M ticks, about 177 ms, for 15 records.
- Where it lands: the sampler pokes the word after setup, so the admission runs at the next frame end. In a shipping build the word is preset, and the admission runs at the creation of the last fighter, during battle load.
- Cost per record: the whole 114-entry admission on the lab ROM took 23.1M ticks (0.69 s), of which 21.6M was convert and upload.

Tables: `gates-final-3arms.txt` and `gates-final.txt` (`tools/gates2b.py`). Counters: `tools/sum2b.py f-admit0 f-admit2`.

### VRAM with word 2 (census arm `f-admit2-census`, the same ROM)

- **VRAMCNT A-D** is `9b818b83`: A texture slot 0, B slot 1, C main BG, and D texture slot 3.
- **Usable texture VRAM** is 393,216 B.
- **At GO:**
  - 250,048 B are used, 26,176 of them in D.
  - The largest free run is 104,896 B.
- **Over the match:**
  - The peak is 266,720 B, 42,848 of it in D.
  - The minimum largest free run is **88,160 B**. The slice 2a minimum was 384 B, so fragmentation is gone.
- **D holds:**
  - fighter textures uploaded after the lock: DK 3,072, Samus 10,368, Link 15,552 and Kirby 3,072 B
  - stage dynamic 2,560 B
  - shield quads 4,096 B
  - damage slash 1,536 B
  - host 2,304 B
  - tint tiles 288 B
- **A+B keep** the scene set: statics, IFCommon clouds, the particle atlas, entry-kept textures and halo+shield.

Source: `vram-classes-f-admit2.txt` (`tools/dclasses.py`).

## What was built

**1. Host generator.** `scripts/fighters/generate_nds_fighter_admission.py` is `reach5.py` promoted.
- **Output:**
  - the NitroFS payload `fighters/admission.bin`: 1,718 records of 28 words, 192,624 B, all 12 kinds x HIGH/LOW
  - the tracked header `include/nds/generated/nds_fighter_admission.generated.h`: format, per-kind counts, payload size, FNV and `ROOT_MAX`
- `--check` fails when the header is stale; it reports OK at FNV `0x09b2f14c`.
- **Each record** carries:
  - image and TLUT as asset id plus SOURCE offset
  - the exact SETTIMG / SETTILE / LOAD / LOADTLUT / SETTILESIZE / TEXTURE words
  - combine, othermode, prim and env
  - a costume mask
  - the Kirby hat kind
- **Palette materials:** every sprite x every palette is enumerated.
- **Makefile:** the NitroFS rule is regenerated from the generator, the estimator, `_paths.py`, the tracked header, `ftparam.c` and relocData. It is a prerequisite of `$(OUTPUT).nds`.
- **Two fixes, both measured against the runtime key recorder:**
  1. The walk now starts from the fighter display's own preamble: `G_CYC_2CYCLE` and `G_RM_FOG_PRIM_A | G_RM_AA_ZB_OPA_SURF2` (`ftdisplaymain.c:1176-1178`). Starting from 1-cycle built every key with a different `ALPHA_IGNORES_TEXELS` bit, and DK, Samus and Link produced 74 outside keys. After the fix they produce 0.
  2. `ROOT_MAX` = 24 is the most DL-bearing joints (common and model parts) of any kind x detail. It sizes the lean arrays.

**2. Creation-time admission.**
- **Where it runs.** `ftManagerMakeFighter` (`src/import/battleship_ftmanager.c`) notes each battle fighter: player, kind, costume and detail. The battle test is `gNdsSceneManagerCurrIsBattle`, not a scene literal. The last fighter runs `ndsFtrLeanAdmitRun` when the word is already set; otherwise it runs at the first frame end that sees the word (the lab poke).
- **Source files.** The adapter resolves each record's source file through the fighter's own FTData file pointers, named by reloc provenance. `ndsRelocGetLoadedAssetView` refuses files loaded under another scene generation, so it is only the fallback.
- **Replay.** Each record is replayed through the renderer's own recorders into a fresh stats block. It then goes through `ResolveOrBindTexture`, the draw path that converts and uploads, followed by the resident lookup that returns the entry. The entry is marked `admitted`.
- **Exemptions.** An `admitted` entry is exempt from eviction, texel1 refresh, stage-source-frame reuse and dynamic-slot recycling. This is a new field; `pinned` keeps its static-record semantics.
- **Records that are skipped, not failed:**
  - spans the compact battle pack omitted: 3 on the stress roster
  - files not loaded in this battle: 1, Kirby's Yoshi-copy prop
- **Stop guards.** Both guards stop the admission and latch a failure; the remaining records stay on demand.
  - the libc top chunk falls under 16 KB
  - the admission reaches the cache slot reserve (the dynamic slots minus the 56 left for the draws' measured same-frame set)
- **Failure latch.** Failures are counted in `gNdsFtrLeanAdmitFail`, and the first is kept in `gNdsFtrLeanAdmitFailFirst` (kind, detail, reason, index, source, reject).

**3. The words** are `gNdsFtrLeanAdmit` in DTCM.
- **0** is today's behaviour.
- **1** admits and pins in A+B.
- **2** is the plan:
  - The `BG3 empty` property is checked: `ndsPlatformVramBg3Empty`, which means no opaque pixel in BG3's visible window. It is not keyed on a scene kind.
  - Bank D becomes texture slot 3; BG3 is hidden and withheld.
  - The admission runs first-fit, so it packs into what A+B has free and spills into D.
  - At the first frame end after it, A+B are locked (`glLockVRAMBank`), and D is the only region that allocates.
- **Withholding BG3** while D is lent:
  - The mask setter strips the foreground and remembers that it was wanted.
  - A BG3 pointer or commit request is refused and counted in `gNdsVramBg3RefusedWrites`. That count was 0 in every run.
  - A BG3 clear is skipped.
- **Battle exit** is in `ndsBattlePlayableRecordLifecycleTaskmanExit`. It releases every cache entry in D, restores the lock word, and locks D. It then asks the platform to remap D.
  - The platform remaps D to BG3, clears it and re-shows it after the next scene's first displayed 3D frame, or at that scene's first BG3 request.
  - The final battle frame stays on screen during the Results hand-off and still reads D, so the remap waits for it.
- **Safety nets:**
  - the VS scene-texture reset closes regions that are still open
  - a non-battle frame while D is lent closes them too (`gNdsVramBankDMissedExits`)

**4. BSS diet.**
- **Lean root arrays:** sized from 32 to `ROOT_MAX` 24.
- **Lab-only under `NDS_TICK_HUD`:**
  - `gNdsFtrLean`, through `NDS_FTR_LEAN_CTR` and `NDS_FTR_LEAN_LAB`
  - the oracle compare and key-moved counters (routes 2 and 3 still patch and disarm)
  - the texture census and union masks
  - the shade self-check
  - the reject and upload witnesses
- **Tint binds:** 16 to 4 per packet. A packet that records more is simply not adopted.

Shipping static RAM, from nm on the non-tick-HUD `smash64ds-p2-shell-hwtri` ELF of the final tree (`ram-delta-shipping.txt`):

| symbol group | pre-2b | final | delta |
|---|---:|---:|---:|
| `gNdsFtrLean` | 788 | 0 | -788 |
| `sNdsFtrLeanInputs` / `Instance` / `Worlds` (lean + kernel) | 1,664 / 1,764 / 4,608 | 1,248 / 1,532 / 4,096 | -1,160 |
| `sNdsFighterPackets` (tint binds 16 to 4) | 15,232 | 14,464 | -768 |
| `sNdsFtrLeanSlots` (union mask lab-only) | 128 | 64 | -64 |
| admission statics (index 208, chunk 896, base table 300, notes and words) | 0 | 1,532 | +1,532 |
| texture cache (124 slots, unchanged) | 24,228 | 24,228 | 0 |
| **total** | | | **-1,248** |

The pre-2b column is read from the pre-2b ELF (`nm-before-tickhud.txt`). These symbols are compiled the same way without the tick-HUD, except `gNdsFtrLean`, which was unconditional before this slice.

## Memory: why the whole admission does not fit this tree

Measured on the 245-slot lab ROM (`m7-admit2`, the same code with a roomy cache):
- **The whole set** is 114 entries (DK 23 / Samus 32 / Link 46 / Kirby 18 records, 119 applied), 66,272 B.
  - 38,048 B filled what A+B had free.
  - 28,224 B went to D.
- **Outside keys** were 0 / 0 / 0 / 4.
- **Failures** were 0 admit failures and 4 uploads after GO (Kirby).

On the shipping-shaped count it cannot be held:

| constraint | what the whole set needs | measured |
|---|---|---|
| **cache slots** (44 B entry + 236 B key each) | 114 admitted + the draws' non-admitted same-frame set (55 at word 0, 41-42 with admission) | 192 slots cost +19 KB static RAM. The four-CPU general-heap low-water fell from 62,484 to **25,620 B**, 20 B above the GObj-cap latch (the arena pays for static RAM in whole pages). So the final ROM keeps 124 slots. |
| **libc reserve** (0xA000 left beside the taskman arena; libnds mallocs a record per texture name plus VRAM and palette block nodes, and its block allocator stores through an unchecked malloc) | about 16 KB above the in-match need for about 114 names | The in-match libc low-water is already 9,568 B at word 0. The guard (16 KB floor) stopped the admission after 15 records (final ROM), 23 records (192 slots) and 33 records (Mario/Fox/Luigi/Yoshi). Without the guard, that roster crashed in `vramBlock__allocateBlock` (malloc NULL) at 194 entries. |

Admitting the whole stress set needs roughly **35 KB of RAM** that this tree does not have. That is about 19 KB of slots plus about 16 KB of libc. It is an owner RAM decision.
- Getting it would mean a larger libc reserve and cache count, paid from the taskman arena (`diagnostics_taskman_heap.c` and the renderer count), or a smaller admitted set.
- **Changing the count also needs the checker updated.** `scripts/check-gbi-decode-fixtures.ps1:1887` pins `NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 124u`, and that file is outside this slice.
- **The tables over-admit** for some kinds, so a smaller admitted set is possible. The generator's costume masks do not restrict Yoshi (210 records per costume against about 10 textures per instance in 2a), Captain (38-63) or Pikachu (43). Their palettes are selected by material animation, which the estimator's costume membership does not resolve.

## Roster variants

- **Build rules.** Each roster was built only in its own build dir (`build-p2p8-roster-mfly`, `build-p2p8-roster-cppn`) with `NDS_LAB_FOURCPU_KINDS` and the admission flags. They used the tree of that moment (the 245-slot cache and the libc guard).
- **Canonical sha check.**
  - The canonical sha256 was `B5D9C696...` before the first variant build, and the same after the last variant plus a canonical rebuild (`canonical-sha-*.txt`).
  - The final tree's `4E02C208...` also reproduced after the shipping-shell build plus a canonical rebuild.
  - Each time, the 193 generated and 139 asset files hash identical to the pre-variant snapshot.
  - The shell build did rewrite the particle-bank outputs, and the canonical rebuild restored them.

| roster | admitted | stopped by | outside keys (per slot) | witnessed outside keys in the table | native failures (w0 = w2) |
|---|---:|---|---|---|---:|
| DK/Samus/Link/Kirby (lab 245-slot) | 114 | - | 0 / 0 / 0 / 4 | Kirby's 4: no | 319 |
| Mario/Fox/Luigi/Yoshi | 33 | libc floor | 0 / 0 / 0 / 15 | 7 of 8 (1 = Yoshi's model DL `Joint_0x5A88`, not in the part tables) | 327 (status 6) |
| Captain/Pikachu/Purin/Ness | 20 | libc floor | 9 / 15 / 7 / 13 | 8 of 8 | 765 (status 208) |

The Captain/Pikachu/Purin/Ness roster is tight on memory at word 0 as well: libc low-water 8,768 B.

## Open (with the exact check)

1. **1P battles have not been checked.** No existing harness reaches a 1P stage cheaply, and the published targets may not be built. The exact check:
   - Build `smash64ds-p2-shell-hwtri` in its own dir with `gNdsFtrLeanAdmit` preset to 2, or poke it before stage setup.
   - Drive into 1P stage 1 (`probe-scene-loop-walk.ps1` or the campaign playback) and read:
     - `gNdsFtrAdmitLab.bg3_not_empty` (0) and `gNdsVramBankDTakes` (1)
     - `gNdsVramBg3RefusedWrites` (must be 0)
     - `gNdsVramBankDReturns` / `gNdsVramBankDMissedExits` at the stage transition
     - the outside counters
   - **Team stages** (Yoshi/Kirby/Polygon teams) create more than 4 fighters, some mid-battle. The admission notes only the first 4, once, so the rest stay on demand.
2. **The battle-exit path has not been exercised.** No gate run reaches the match end within 1,972 frames. The exact check is a run past GAME SET into Results, reading:
   - `gNdsFtrAdmitLab.exit_runs` / `exit_released` / `exit_orphans`
   - `gNdsVramBankDReturns` = 1 and `MissedExits` = 0
   - a Results screenshot compared at word 0 and word 2
3. **The creation-time path has not been exercised.** The lab pokes after setup. The exact check: default the word to 2 in a lab build, or poke it before battle setup, then compare `gNdsFtrAdmitLab.frame` (0 means it ran at creation) and the outside counters.
4. **Kirby's 4 keys outside the table.** They are image = `palettes[k]`, TLUT = `palettes[0]`, from the body MObj at `0x1CE0`.
   - The draw's texture id runs past the one-entry sprite list into the palette list: `sprites[11 + k]` is `palettes[k]`.
   - This happens in the generic fallback during Kirby's JumpAerial statuses. The native program is rejected there too, which is where the 319 native failures come from.
   - A general rule for this (texture ids past the sprite list) inflated the table from 1,718 to 3,314 records and was reverted. The exact fix is to read the texture-id range from the material animations.
5. **Admission cost.** It is about 5.4 ms per record, almost all convert and upload. In a shipping build that lands in battle load, not in a presented frame.

## Files

**Changed:**
- `scripts/fighters/generate_nds_fighter_admission.py` (new)
- `include/nds/generated/nds_fighter_admission.generated.h` (new, generated)
- `Makefile` (the admission payload rule)
- `include/nds/renderer_fighter_lean.h`
- `src/nds/nds_renderer_textures_effects.c`
- `src/nds/nds_renderer_preamble.c`
- `src/nds/nds_renderer_native_common.c`
- `src/nds/nds_platform.c`
- `src/nds/nds_ftr_lean_kernel.c`
- `src/port/renderer_fighter_lean.c`
- `src/port/renderer_adapter_fighter.c`
- `src/import/battleship_ftmanager.c`
- `src/port/taskman_seam_battle_host.c`

Nothing under `decomp/` changed. No tracked generated file is left modified by a variant or shell build.

**Checker note.** `check-gbi-decode-fixtures.ps1 -CollectFailures` reports 1 failure: the NDO6 Ness image pins (23 slots and 399/33/293/31 against the generated 25 and 417/35/311/33). That header is dated 2026-09-22 09:56, before this slice. It is unchanged here (same hash before and after), so the failure is pre-existing.

**Runs:**
- `f-admit0` / `f-admit1` / `f-admit2`: final gates, census off
- `f-admit2-census`: the VRAM classes
- `m1`-`m7`: lab bring-up
  - `m7` is the 245-slot run with the whole admission.
  - `m4`/`m5` are the key diagnostics that found the preamble bit.
- `v1-mfly-*` and `v2-cppn-*`: roster variants
- digests: `digest-*.json`
- builds: `build*.log`

**Tools (`tools/`):**
- `build-s2b.ps1`: holds the lock; one target, one build dir
- `run-s2b.ps1`: slot 9 / GDB 3423
- `sum2b.py`, `gates2b.py`, `dclasses.py`, `diagkey.py`, `ramdelta.py`

The backups of every touched source file from before this slice are in the session scratchpad (`backup-slice2b`).
