# Lane 2: what the three common-file owners keep in a VS battle, what the DS still reads, and what can be handed back

Read-only investigation, 2026-09-28. Nothing was built and no emulator was run. Every number below comes from a script in `artifacts/performance/2026-09-28_p2-2p8-ram-supply/tools/` (section 8) run against the saved build `builds/build-fp-argmax/` and the source as committed at `039192a0ab6`; every `file:line` is resolved by `tools/lane2_cites.py` with `git show 039192a0ab6:<path>` (ledger in appendix C).

- Config: `builds/build-fp-argmax/nds_build_config.h` (`NDS_IF_GAMESTATUS_COMPACT 1`, `NDS_R2_EFFECT_POOL 38`, `NDS_P2_ITEM_CORE 1`, `NDS_P2_MENU_SHELL 1`, `NDS_P2_COMPACT_BATTLE_FIGHTERS 1`, `NDS_P2_FOUR_CPU_STRESS 0`). ELF: `builds/build-fp-argmax/smash64ds-p2-shell-freeplay-hwtri.elf` (26,970,620 B, 2026-09-28 15:02:29).
- Pinning. Line numbers are for commit `039192a0ab6` (HEAD at the last build of this report: `039192a0ab6`). The working tree also carries other work's uncommitted edits in: `CLAUDE.md`, `Makefile`, `docs/DIAGNOSTIC_REFERENCE.md`, `docs/README.md`, `docs/p2/BUGS_REMAINING_DIAGNOSIS_2026-09-19.md`, `scripts/menus/probe-p2-campaign.ps1`, `src/port/diagnostics_renderer_census.c`, `src/port/reloc_backend_assets.c`, `src/port/reloc_backend_compat_shims.c`, `src/port/reloc_backend_ftmain_runtime.c`, `src/port/renderer_adapter_matrix.c`, `src/port/sprite_preview_backend.c`. Of those, the cited ones are: `Makefile`, `src/port/reloc_backend_assets.c`, `src/port/reloc_backend_compat_shims.c`, `src/port/sprite_preview_backend.c`; working-copy line numbers in them are shifted, the pinned numbers are not. Files the scripts read from the working tree that differ from the pinned commit: none.
- Confidence tags: H = holds for the linked code as read, no unproven premise; M = holds unless a listed UNPROVEN item is false, or needs a mechanism not yet built or measured; L = weak.

## 0. The answer

| | bytes | share of the 570,000-700,000 B resident-bank target |
|---|---:|---:|
| held today by the 19 rows below (compact GameStatus on, HEAD default) | 325,232 | |
| still needed after load (bytes a reader touches, plus pools and tables in use) | 147,776 | |
| **A: reclaimable with no new reader of file bytes** (readers that assume the old layout are converted, section 5.4), net of span rows, root cells and identity stubs | **129,300** | 18.5% - 22.7% |
| **A+B**: also points the readers that only run while the interface, effect or item is being built at the NitroFS copy | **214,660** | 30.7% - 37.7% |
| lever outside A/B: particle pools 112/24/80 -> 64/20/48 (M) | 11,120 | 1.6% - 2.0% |
| lever outside A/B: skip the item files when no item can spawn (L) | 97,920 | |

What the numbers say:

1. **These three owners cannot fund the bank by themselves.** Firm supply is 129,300 B (18.5-22.7% of the target); with every setup-time reader retargeted it is 214,660 B. 93,988 B of A is H-confidence; 35,312 B is M-confidence and depends on there being no plan to draw the pause overlay or the Sudden Death text natively (section 4.2).
2. **The census figures are uncompacted.** The task's 208,672 / 82,976 / 94,704 B are the flag-off sizes (406,040 B with the pools and the arrow sprite). At HEAD's default `NDS_IF_GAMESTATUS_COMPACT ?= 1` owner A already holds 99,560 B: the other seven files as loaded (56,400 B block, GameStatus counted as a 16 B placeholder), a 21,056 B GameStatus image and 22,104 B of baked end streams. Compaction dropped 131,232 B of letter pixels and spent 22,104 B on streams: net 109,128 B.
3. **The DS reads almost none of the pixels, display lists or vertices in these files after load.** The shipping ELF links no display-list interpreter (28 of 28 forbidden symbols absent; a list no native owner admits falls through to a NO_PROGRAM record, `src/port/renderer_adapter_stage.c:12735`). The lower-screen HUD draws from `nitro:/menus/battle_hud.bin`, not from the IFCommon files (`src/nds/nds_battle_hud.c:251`). Effect and item owners bake geometry into ROM tables, admit a list by root identity or by a few compared words, and read texel bytes from the file in only two places (DamageSlash / DamageFlyMDust frames at scene prepare, item textures at first draw).
4. **The bytes are concentrated.** By file, largest first: IFCommonAnnounceCommon 30,176 (M), EFCommonEffects1 30,176, EFCommonEffects2 23,164, MiscData086 19,712, EFCommonEffects3 11,096, IFCommonBattlePause 5,136 (M), the three lower-HUD files 9,536 in all, GameStatus pad 304. By class see section 5.2.
5. **The obstacle is not the bytes, it is layout assumptions in the native owners.** Effect and item owners compare source offsets and source sizes against the loaded file directly (26 raw pointer-word compares and 27 raw size compares for items alone, 10 raw root addresses for EF2). Any compaction breaks them until they are converted to the pack-aware helpers the fighter path already uses (section 5.4). The five IFCommon sprite files need no owner conversion: every reader of their surviving bytes goes through `lbRelocGetFileData`, whose resolver already maps compacted files (`src/port/reloc_backend_assets.c:16332`).
6. **Recommended order** (section 6): EF2 + EF3 + the five IFCommon sprite files first (79,108 B, 11.3%-13.9% of the target), then EF1 (109,284 B cumulative, 85% of A), then the item file last (highest edit surface per byte). The B-tier readers are a separate decision because each adds a NitroFS read at a time the game did not pay for before.
7. **The 2026-09-05 item closure prototype no longer bounds the item file.** `scripts/items/item_memory_closure.py` kept every typed display list and vertex array because at the time `SubmitStageDL` walked display lists, and concluded a 1,940 B saving. That premise ended with the native-only adoption (`docs/p2/BUG_NOTES.md`, 2026-09-07); section 4.4.

## 1. Scope, order of allocation, and what "held" means

The three owners run in this order inside `scVSBattleStartBattle`: `scVSBattleSetupFiles` (`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:137`) loads the eight IFCommon files with one `lbRelocLoadFilesListed` (`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattlefiles.c:23-37`, ids in `decomp/BattleShip-main/decomp/src/gm/gmcommon.c:11-20`); `efParticleInitAll` (`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:153`) allocates the particle pools; `itManagerInitItems` (`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:160`) allocates the ITStruct pool, the 82,976 B item tree and the arrow sprite; `efManagerInitEffects` (`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:164`) allocates the EFStruct pool and three separate EF files (`decomp/BattleShip-main/decomp/src/ef/efmanager.c:1754-1756`, sized by `lbRelocGetFileSize`, `src/port/reloc_backend_assets.c:12753`). Only after all of that does the per-player fighter loop run (`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:177`).

Two consequences drive the whole estimate.

- Every allocation is a `syTaskmanMalloc` bump allocation from `gSYTaskmanGeneralHeap`, 16 B aligned (`src/port/reloc_backend_assets.c:802`). A bump heap gives bytes back only if an allocation is made smaller when it is made (load-time compaction, or a file stripped at build time); freeing in place returns nothing. Because all three owners allocate before any fighter file, shrinking any of them moves every later allocation down by the same amount, so the reclaim is credited 1:1 to the heap's free top, which the elastic motion cache now takes on demand (`e830e06b180`; the compaction window itself must first hand that cache back, `src/port/reloc_backend_assets.c:16141`).
- When the three owners load, the heap is at its emptiest of the whole battle setup, so a temporary top-of-heap window for a load-time compaction is cheap here (the GameStatus loader already uses one, `src/port/reloc_backend_assets.c:15916`). A build-time pack needs no window at all.

Measured heap floor across the 30 four-CPU stress runs found in `artifacts/performance/2026-09-*`: `gNdsTaskmanGeneralHeapFreeMin` 24,668 / 112,192 / 122,412 B (min / median / max). The cliff below which the GObj cap latches and the countdown dereferences NULL is 25,600 B free (`src/import/battleship_lbparticle.c:190-200`).

## 2. Inventory (question 1)

**T1 inventory**

| owner | fid | file / allocation | runtime slot | O2R container B | payload B | aligned alloc B | NitroFS == O2R |
|---|---:|---|---|---:|---:|---:|---|
| A `scVSBattleSetupFiles` | 166 | IFCommonPlayer | gGMCommonFiles[0] | 1,056 | 976 | 976 | yes |
| A `scVSBattleSetupFiles` | 82 | IFCommonGameStatus | gGMCommonFiles[1] | 152,368 | 152,288 | 152,288 | yes |
| A `scVSBattleSetupFiles` | 164 | IFCommonPlayerDamage | gGMCommonFiles[2] | 5,744 | 5,664 | 5,664 | yes |
| A `scVSBattleSetupFiles` | 165 | IFCommonTimer | gGMCommonFiles[3] | 4,816 | 4,736 | 4,736 | yes |
| A `scVSBattleSetupFiles` | 36 | IFCommonDigits | gGMCommonFiles[4] | 2,416 | 2,336 | 2,336 | yes |
| A `scVSBattleSetupFiles` | 197 | IFCommonBattlePause | gGMCommonFiles[5] | 6,496 | 6,416 | 6,416 | yes |
| A `scVSBattleSetupFiles` | 38 | IFCommonPlayerTags | gGMCommonFiles[6] | 3,920 | 3,840 | 3,840 | yes |
| A `scVSBattleSetupFiles` | 37 | IFCommonAnnounceCommon | gGMCommonFiles[7] | 32,496 | 32,416 | 32,416 | yes |
| A subtotal |  | 8 files, one block (flag OFF) |  |  |  | 208,672 |  |
| A subtotal |  | same block, compact flag ON (HEAD default) |  |  |  | 56,400 + GameStatus image 21,056 + baked streams 22,104 (measured) |  |
| B `itManagerInitItems` | 251 | ITCommonData | gITManagerCommonData | 3,608 | 3,392 | 3,392 | yes |
| B | 86 | MiscData086 (extern of all 68 ITCommonData slots) | (in the same tree) | 79,664 | 79,584 | 79,584 | yes |
| B |  | extern tree total = gNdsITCommonDataBytes |  |  |  | 82,976 |  |
| B | 87 | IFCommonItem (`ifCommonItemArrowSetAttr`) | sIFCommonItemArrowSprite | 240 | 160 | 160 | yes |
| B |  | ITStruct pool: 924 B x ITEM_ALLOC_MAX 16 | sNdsItemStructsFree |  |  | 14,784 |  |
| C `ndsBaseEFManagerInitEffects` | 83 | EFCommonEffects1 | gEFManagerFiles[0] | 52,816 | 52,736 | 52,736 | yes |
| C `ndsBaseEFManagerInitEffects` | 84 | EFCommonEffects2 | gEFManagerFiles[1] | 28,432 | 28,352 | 28,352 | yes |
| C `ndsBaseEFManagerInitEffects` | 85 | EFCommonEffects3 | gEFManagerFiles[2] | 13,696 | 13,616 | 13,616 | yes |
| C subtotal |  | three files = census 94,704 |  |  |  | 94,704 |  |
| C |  | EFStruct pool: 60 B x NDS_R2_EFFECT_POOL 38 | sEFManagerStructsAllocFree |  |  | 2,280 |  |
| C |  | visual templates: 352 B x 7 (port wrapper) | sNdsVisualTemplates |  |  | 2,464 |  |
| (adjacent) `efParticleInitAll` |  | LBParticle 112 x 96, LBGenerator 24 x 92, LBTransform 80 x 192 | gEFParticle*GObj |  |  | 28,320 |  |

Notes on the table.

- IFCommon compaction status: **only IFCommonGameStatus (fid 82) is compacted** (`src/port/reloc_backend_assets.c:15916` loader, called from `src/port/reloc_backend_assets.c:16142`): 131,232 B of the twelve letters' pixel payloads are dropped, the 21,056 B image keeps the Sprite/Bitmap headers and the lamp, rod and frame pixels, and 22,104 B of run-length TIME UP / GAME SET banks are allocated separately (measured 22,104 B in `artifacts/performance/2026-09-23_p2-2p8-if-gamestatus-compact/README.md`; the same figure is in `docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`). The other seven IFCommon files (56,384 B of payload) are loaded whole. `lbRelocGetAllocSize` sizes the block with GameStatus as a 16 B placeholder (`src/port/reloc_backend_assets.c:16072`), hence 56,400 B.
- IFCommonItem (fid 87) is an owner-B file: `ifCommonItemArrowSetAttr` runs at the end of `itManagerInitItems` (`src/import/battleship_item_link_core.c:807-902`).
- The item tree is one allocation: `lbRelocGetExternHeapFile` writes ITCommonData (3,392 B) and MiscData086 (79,584 B) into a 82,976 B block (`src/import/battleship_item_link_core.c:856`).
- Struct sizes are from the DWARF of the ELF (`lane2_struct_sizes.py`): EFStruct 60 B, ITStruct 924 B, LBParticle 96 B, LBGenerator 92 B, LBTransform 192 B, NDSVisualTemplate 352 B. Pool capacities are the source's own (EFFECT_ALLOC_NUM 38, `src/import/battleship_efmanager.c:208`; particle pools 112/24/80 for the menu shell, `src/import/battleship_lbparticle.c:266`; ITEM_ALLOC_MAX 16, `include/it/item.h:439`).
- NitroFS holds byte-identical copies of all of these O2R files (`builds/build-fp-argmax/nitrofs/reloc/`, compared byte for byte by `lane2_inventory.py`), so a setup-time reader can be retargeted at a NitroFS range without a new asset.

## 3. Byte classes (question 2)

Classification walks from the source roots, not from labels alone.

- **IFCommon sprite files** (`decomp/BattleShip-main/decomp/src/sys/objman.c:1591` explains why headers matter): the files are runs of `[pixels][Bitmap[n]][Sprite]` units. Sprite headers are found from the internal-fixup slot at `Sprite+52` and validated against their own fields (`lane2_sprites.py`); counts equal the `.spritelist` counts (84 sprites in the five dead-pixel files, 6 in PlayerTags, 24 in GameStatus, 1 in IFCommonItem). No sprite in any of these files uses `G_IM_SIZ_4c` (`lane2_4c_check.py`: 0 of 115), so the in-place 4c expansion at SObj creation (`src/port/sprite_preview_backend.c:137`) never touches their pixels.
- **Effect files**: typed label partition of the O2R (`lane2_partition.py`, 63 decomp `llEFCommonEffects*` names, 59 with offsets in `src/import/battleship_efmanager_symbols.h`, plus the shadow texture root), then pointer-closure reachability over the O2R internal fixups from those roots (`lane2_reach.py`), then a structural F3DEX2 decode of every reachable list (`lane2_dl.py`). Reachability is interval-level, so it over-counts live bytes and A is a floor.
- **ITCommonData (file 251)**: attribute tables, not display data. The closure script's row census (`tools/item-closure-run/report.txt`): 34 ITAttributes x72 + 12 WPAttributes x52 + 24 attack events x8 + 30 f32 x4 = 3384 bytes; file is 3392 bytes, delta 8 bytes (unclassified table tail; kept live). Every row is named by a per-kind descriptor and decoded by `ndsItDecodeAttributes` (`src/import/battleship_item_link_core.c:368-448`), so all of it stays.
- **Item file**: the closure script's label categories (reproduced exactly by `lane2_items.py`), overlaid with the structural decode of the 56 display-list roots and reachability from the 68 ITCommonData slots, the 40 numeric `itGetPData` roots and the DObjDesc roots.

**T2 IFCommon sprite files: bytes by class and reader**

| file | fid | payload B | sprites | Sprite headers (kept) | Bitmap arrays | pixels: no reader | pixels read only while building | pad |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| IFCommonPlayerDamage | 164 | 5,664 | 12 | 816 | 192 | 4,472 | 0 | 184 |
| IFCommonTimer | 165 | 4,736 | 15 | 1,020 | 240 | 3,280 | 0 | 196 |
| IFCommonDigits | 36 | 2,336 | 13 | 884 | 208 | 1,080 | 0 | 164 |
| IFCommonBattlePause | 197 | 6,416 | 16 | 1,088 | 256 | 4,872 | 0 | 200 |
| IFCommonPlayerTags | 38 | 3,840 | 6 | 408 | 96 (kept) | 0 | 3,264 | 72 |
| IFCommonAnnounceCommon | 37 | 32,416 | 28 | 1,904 | 448 | 29,632 | 0 | 432 |
| IFCommonItem | 87 | 160 | 1 | 68 | 16 (kept) | 0 | 56 | 20 |
| IFCommonGameStatus (resident compact image) | 82 | 21,056 of 152,288 | 24 | 1,632 | 816 (kept) | 0 | 18,016 | 592 |
| IFCommonGameStatus already dropped (12 letter payloads) | 82 | 131,232 | 12 | - | - | 131,232 | - | - |

**T3c effect files: byte classes (structural decode over label partition; label totals differ only where a Gfx/Vtx label lumps trailing bytes)**

| file | texture | palette | Gfx | DObjDLLink | Vtx | animation | MObjSub | DObjDesc | pad | other/unplaced |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| EFCommonEffects1 | 46,580 | 600 | 1,640 | 240 | 672 | 1,232 | 824 | 0 | 940 | 8 |
| EFCommonEffects2 | 18,392 | 384 | 2,080 | 192 | 944 | 2,936 | 1,120 | 796 | 1,500 | 8 |
| EFCommonEffects3 | 512 | 120 | 6,184 | 688 | 2,896 | 688 | 1,000 | 0 | 1,520 | 8 |

**T4b MiscData086 byte classes after structural overlay**

| class | bytes |
|---|---:|
| texture | 40,392 |
| display_list | 12,064 |
| vertices | 6,640 |
| dobjdesc | 6,240 |
| animation | 5,476 |
| pad | 4,660 |
| material | 2,296 |
| palette | 1,680 |
| other | 112 |
| dllink | 16 |
| unclassified | 8 |

## 4. Does the DS still read these bytes after load? (question 3)

### 4.1 No display list from these files is executed

- `scripts/check_native_only_rom.py` defines the set of display-list interpreter symbols the ROM must not link (`scripts/check_native_only_rom.py:13`) and the Makefile enforces it at ROM packaging (`Makefile:7515`). `lane2_nm_check.py` reads the symbol table of the shipping-like ELF: **0 of 28 forbidden symbols present**, and no symbol matching a display-list walker pattern (22,667 symbols).
- `ndsRendererAdapterSubmitStageDL` is present in the ELF, but it is the admission dispatcher: it tries each native owner in turn and, if none admits the list, records a `NDS_NATIVE_FAILURE_NO_PROGRAM` and stops (`src/port/renderer_adapter_stage.c:12735`). Nothing is executed.
- What admission reads: the DObj's list *address* (root identity via `ndsRelocNativeRootOffset`, `src/port/reloc_preview_pack.c:150-163`, or raw address equality for EF2's entry roots, `src/port/renderer_adapter_stage.c:6218-6227`) and a handful of list words at fixed indices (item owners from `src/port/renderer_adapter_stage.c:8793` on; DamageSlash `src/port/renderer_adapter_stage.c:8471`). The display-list scanners in `renderer_adapter_stage.c` that walk many words (`src/port/renderer_adapter_stage.c:1279-1306`, `src/port/renderer_adapter_stage.c:1324-1371`, `src/port/renderer_adapter_stage.c:1378-1440`) read the frame's DL heap (`gSYTaskmanDLHeads`), not file lists, and the owner-hash walker is not linked (no `OwnerHash` symbols in the ELF).
- What draws: geometry from ROM tables (ImpactWave `src/nds/nds_renderer_textures_effects.c:3754`, RebirthHalo `src/nds/nds_renderer_textures_effects.c:3784`, entry effects `src/nds/nds_renderer_native_common.c:6319`, every item owner: "its TLUT, image, state, and both geometry phases are immutable source data and bake into this owner", `src/nds/nds_native_item_hammer.exec.inc:2`).

So display-list bytes count as dead except for the compared words, and vertex bytes count as dead outright.

### 4.2 Owner A: the IFCommon files

| file | who reads what after load | cited reader |
|---|---|---|
| IFCommonPlayerDamage, IFCommonTimer, IFCommonDigits | Sprite headers only. Timer, stock and damage GObjs are redirected to the lower HUD (`src/import/battleship_ifcommon.c:876-918`; mode set by `src/nds/nds_platform.c:577`); the HUD state comes from game state, not sprite data (`src/import/battleship_ifcommon.c:757-874`); the glyphs come from `battle_hud.bin` (`src/nds/nds_battle_hud.c:240-320`, `src/nds/nds_battle_hud.c:251`). | headers: `decomp/BattleShip-main/decomp/src/sys/objman.c:1591` via `src/port/sprite_preview_backend.c:133-155` |
| IFCommonBattlePause, IFCommonAnnounceCommon | Sprite headers only. The native OAM path recognises an SObj by its `Bitmap` pointer and size against the GameStatus assets (`src/nds/nds_ifcommon_oam.c:2759-2797`); anything else is not drawn and only records a failure (`src/port/sprite_preview_backend.c:791`, `src/port/sprite_preview_backend.c:833`). Pause decals were drawn by the old layered path on 2026-09-06 (`src/port/battle_playable_compat_stubs.c:201`; `docs/p2/BUG_NOTES.md:967`), which no longer exists. | none for pixels |
| IFCommonPlayerTags | Sprite headers and Bitmap[] (the baked-cell table is keyed by the `Sprite.bitmap` pointer). Pixels are read once, when the interface is created (`src/nds/nds_ifcommon_oam.c:3445-3564`, call `src/import/battleship_ifcommon.c:469`); the OAM emit uses the baked cell (`src/nds/nds_ifcommon_oam.c:3901`). | setup only |
| IFCommonItem | arrow sprite baked once at `itManagerInitItems` (`src/nds/nds_ifcommon_oam.c:3290-3376`, call `src/import/battleship_ifcommon.c:489`) | setup only |
| IFCommonGameStatus (image) | Sprite/Bitmap headers matched by pointer (`src/nds/nds_ifcommon_oam.c:2759-2797`) and moved by `src/nds/nds_ifcommon_oam.c:2658`; lamp, rod and frame pixels read once by the cloud/traffic atlas prepare (`src/nds/nds_ifcommon_oam.c:2380`); letter payloads listed by `src/nds/nds_ifcommon_oam.c:2458`, baked and dropped by `src/port/reloc_backend_assets.c:15916`; end banks built by `src/nds/nds_ifcommon_oam.c:2597` | headers resident, pixels at scene entry |
| IFCommonPlayer | not classified; 976 B, kept | |

### 4.3 Owner C: the effect files

**T3 effect files: read model (bytes)**

| file | fid | payload | still read (DObjDesc/MObjSub/anim/DLLink + 48 B DL words) | read only at scene prepare | identity key only | unreferenced by any source root | reachable Gfx, never read | Vtx, never read | textures/palettes, no reader | pad |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| EFCommonEffects1 | 83 | 52,736 | 2,208 | 20,064 | 0 | 23,476 | 1,480 | 608 | 3,960 | 940 |
| EFCommonEffects2 | 84 | 28,352 | 4,836 | 0 | 5,280 | 1,360 | 1,896 | 816 | 12,664 | 1,500 |
| EFCommonEffects3 | 85 | 13,616 | 2,376 | 0 | 0 | 136 | 6,056 | 2,896 | 632 | 1,520 |
| **all three** |  | 94,704 | 9,420 | 20,064 | 5,280 | 24,972 | 9,432 | 4,320 | 17,256 | 3,960 |

Per-effect closures (bytes reachable from that effect's roots; shared bytes are counted in each):

**T3b per-effect closure bytes (pointer closure from that effect's ll roots; shared bytes counted in each)**

| file | effect | closure B | tex+pal | Gfx | Vtx | anim | MObjSub+DObjDesc |
|---|---|---:|---:|---:|---:|---:|---:|
| EFCommonEffects1 | CommonSpark | 4,320 | 3,616 | 176 | 64 | 224 | 120 |
| EFCommonEffects1 | DamageFlyMDust | 14,968 | 14,336 | 128 | 64 | 224 | 152 |
| EFCommonEffects1 | DamageSlash | 7,208 | 5,728 | 608 | 128 | 328 | 304 |
| EFCommonEffects1 | FlyOrbs | 400 | 80 | 192 | 64 | 40 | 0 |
| EFCommonEffects1 | ImpactWave | 1,168 | 288 | 280 | 288 | 168 | 128 |
| EFCommonEffects1 | QuakeMag0 | 96 | 0 | 0 | 0 | 96 | 0 |
| EFCommonEffects1 | QuakeMag1 | 208 | 0 | 0 | 0 | 208 | 0 |
| EFCommonEffects1 | QuakeMag2 | 208 | 0 | 0 | 0 | 208 | 0 |
| EFCommonEffects1 | QuakeMag3 | 64 | 0 | 0 | 0 | 64 | 0 |
| EFCommonEffects2 | CatchSwirl | 2,496 | 256 | 608 | 48 | 672 | 504 |
| EFCommonEffects2 | DeadExplode1 | 112 | 0 | 0 | 0 | 112 | 0 |
| EFCommonEffects2 | DeadExplode2 | 112 | 0 | 0 | 0 | 112 | 0 |
| EFCommonEffects2 | DeadExplode3 | 120 | 0 | 0 | 0 | 100 | 0 |
| EFCommonEffects2 | DeadExplode4 | 112 | 0 | 0 | 0 | 112 | 0 |
| EFCommonEffects2 | DeadExplodeDefault | 7,688 | 5,280 | 464 | 384 | 912 | 376 |
| EFCommonEffects2 | FireSpark | 2,904 | 2,104 | 200 | 64 | 204 | 288 |
| EFCommonEffects2 | NessPKFlash | 4,972 | 4,128 | 216 | 64 | 296 | 44 |
| EFCommonEffects2 | ReflectBreak | 2,784 | 1,024 | 456 | 192 | 420 | 384 |
| EFCommonEffects2 | ShadowTexture | 5,280 | 5,280 | 0 | 0 | 0 | 0 |
| EFCommonEffects2 | ShockSmall | 5,600 | 5,152 | 112 | 64 | 108 | 144 |
| EFCommonEffects3 | ItemGetSwirl | 2,440 | 256 | 960 | 48 | 640 | 504 |
| EFCommonEffects3 | MBallRays | 2,320 | 256 | 720 | 304 | 496 | 496 |
| EFCommonEffects3 | RebirthHalo | 8,720 | 128 | 4,536 | 2,544 | 48 | 0 |

Readers, by file.

- **All three**: `ndsEFManagerResolveAllDescOffsets` (`src/import/battleship_efmanager.c:2023`) resolves every EFDesc at init; `ndsEFManagerMapFileOffset` (`src/import/battleship_efmanager.c:1277-1343`) already routes each descriptor offset through `ndsRelocNativeAssetAddress` (`src/port/reloc_preview_pack.c:198-218`) and `ndsRelocNativeRootAddress` (`src/port/reloc_preview_pack.c:165-196`), and the span check uses the loaded size (`src/import/battleship_efmanager.c:1107`). So DObjDesc, MObjSub, AnimJoint, MatAnimJoint and DObjDLLink bytes are the "keep_runtime" class.
- **EFCommonEffects1 (83)**: DamageSlash reads 13 CI4 frames and DamageFlyMDust 7 frames once, "before the first battle presentation" (`src/nds/nds_native_damage_slash.exec.inc:124-188`, `src/nds/nds_native_damage_fly_mdust.exec.inc:44-99`), from raw `asset_base + offset`; the fly-mdust preflight also compares source offsets against the loaded size (`src/nds/nds_native_damage_fly_mdust.exec.inc:202`). DamageSlash compares 48 B of list words per root (`src/port/renderer_adapter_stage.c:8438`, `src/port/renderer_adapter_stage.c:8471`). ImpactWave is admitted by effect-struct identity and draws ROM texels (`src/nds/nds_renderer_textures_effects.c:3754`). FlyOrbs (0x7E80) and CommonSpark (0x8FA0) have no owner: no reference in `src/`, and hit sparks and fly orbs are routed to procedural templates (`src/port/reloc_backend_compat_shims.c:8662`, source-effect shim `src/port/reloc_backend_compat_shims.c:8252`). 23,476 B are unreachable from any decomp root.
- **EFCommonEffects2 (84)**: CatchSwirl, DeadExplodeDefault and ReflectBreak are entry effects admitted by raw address equality on ten roots (`src/port/renderer_adapter_stage.c:6218-6227`); their texels are ROM tables (`src/nds/nds_renderer_native_common.c:6319`). FireSpark (0x1f78), NessPKFlash (0x6c28) and ShockSmall (0x1500) have no owner. ShadowTexture is used only by decomp `ftshadow.c`, whose maker returns NULL in this port (`src/port/reloc_backend_compat_shims.c:12811-12815`); but the static texture corpus still holds three keys into this file (0x3af8, 0x3f00, 0x4708) and every applicable record must build a key or stage admission declines (`src/nds/nds_renderer_textures_effects.c:5984-6052`, `src/nds/nds_renderer_textures_effects.c:6054`), so those three offsets must keep mapping (28 B stubs each).
- **EFCommonEffects3 (85)**: MBallRays and ItemGetSwirl roots by raw `dl - base` compares (`src/port/renderer_adapter_stage.c:6631`), RebirthHalo roots likewise (`src/port/renderer_adapter_stage.c:7438`); all geometry and texels are ROM tables (`src/nds/nds_renderer_textures_effects.c:3784`, `src/nds/nds_renderer_native_common.c:6319`).

### 4.4 Owner B: the item files

**T4 MiscData086: read model (bytes; payload 79,584)**

| class / reader | bytes | verdict |
|---|---:|---|
| Gfx never read (no interpreter; owner checks compare addresses) | 11,032 | dead |
| Gfx words the owner candidate checks compare (upper bound, 8 B per (root, index)) | 1,032 | keep |
| Vtx (geometry is generated ROM data) | 6,640 | dead |
| pad / gap filler | 4,660 | dead |
| textures+palettes reachable from kinds WITH a native owner (bound at first draw) | 21,912 | read at bind |
| textures+palettes reachable only from kinds WITHOUT an owner (12 Pokemon + weapons) | 20,160 | no reader today |
| DObjDesc, MObjSub, animation, DLLink, unclassified (reached) | 13,468 | keep |
| same classes, not reached from any known root (UNPROVEN dead, kept) | 680 | keep |

**T8 item native owners on MiscData086: Gfx words compared at draw, texture/palette offsets bound**

| owner | DL roots | dl[] indices compared | bytes (upper bound) | TLUT/IMAGE offsets bound |
|---|---|---|---:|---|
| CASTLE_BUMPER | 0x7558 |  | 0 | 0x7238, 0x7260, 0x7288 |
| ITEM_BAT | 0x1bf0, 0x1c48, 0x1d48 | 9, 14 | 48 | 0x19d8, 0x1a20, 0x1a68, 0x1ab0 |
| ITEM_BOMBHEI | 0x3310 | 11, 17, 21 | 24 | 0x27e8 |
| ITEM_BOX | 0x6630 | 8, 14, 18, 24, 28, 32, 33 | 56 | 0x6098, 0x60c0, 0x60e8, 0x62f0 |
| ITEM_CAPSULE | 0x3e0, 0x440, 0x540, 0x5e0 | 9, 10, 17 | 96 | 0x8, 0x30, 0x58, 0x80, 0x108, 0x190 |
| ITEM_EGG | 0x103b0 | 10, 16, 21 | 24 | 0x10158, 0x10180 |
| ITEM_FFLOWER | 0x4520, 0x4578, 0x4608 | 3, 6, 8, 9, 12 | 120 | 0x4168, 0x4200, 0x4308 |
| ITEM_GSHELL | 0x5ec0 | 12, 17 | 16 |  |
| ITEM_HAMMER | 0x25f0 | 11, 17, 21, 36 | 32 | 0x22c8, 0x22f0 |
| ITEM_HARISEN | 0x20a0 | 11, 17, 22 | 24 | 0x1eb8, 0x1ee0 |
| ITEM_HEART | 0xff8 | 11, 17, 21, 32, 36 | 40 | 0xb48, 0xb70, 0xd78 |
| ITEM_IWARK | 0xa050 | 10, 16, 21 | 24 | 0x98e8, 0x9910 |
| ITEM_KIRBYSTAR | 0x5458 | 11, 15 | 16 | 0x4c18 |
| ITEM_LGUN | 0x3db0 | 11, 17, 22, 28, 32, 38, 43 | 56 | 0x3a88, 0x3af0, 0x3b38, 0x3b80 |
| ITEM_MBALL | 0x9250, 0x9340 | 10, 11, 16, 17, 21 | 80 | 0x7d90, 0x7db8, 0x8e20 |
| ITEM_MSBOMB | 0x37a0, 0x38b0 | 10, 14, 16, 20, 21, 25 | 96 | 0x3608, 0x3630, 0x3658, 0x36e0 |
| ITEM_NBUMPER | 0x7558 | 10, 16, 21 | 24 | 0x7288 |
| ITEM_RSHELL | 0x5ec0 | 12, 17 | 16 |  |
| ITEM_STAR | 0x1440 | 8, 14, 18, 23, 29 | 40 | 0x1238 |
| ITEM_STARROD | 0x49b0, 0x4a18, 0x4aa0 | 5, 10, 11, 15 | 96 | 0x4798, 0x47c0, 0x47e8, 0x4870 |
| ITEM_SWORD | 0x17d8, 0x1850 | 7, 10, 15 | 48 | 0x1668 |
| ITEM_TARU | 0x7000 | 11, 17, 21, 29, 34, 38 | 48 | 0x69e8, 0x6a10, 0x6a38, 0x6c40 |
| ITEM_TOMATO | 0x9c0 | 11, 17, 21 | 24 | 0x758, 0x780 |

- Load and structure: `src/import/battleship_item_link_core.c:807-902`, `src/import/battleship_item_link_core.c:856`; `ndsItDecodeAttributes` reads each kind's ITAttributes row once and keeps pointers to its DObjDesc / MObjSub / animation rows (`src/import/battleship_item_link_core.c:368-448`); DObj trees are built from those descriptors (`src/import/battleship_item_link_core.c:1024`). All of that stays.
- 23 native owners (22 items and the castle bumper) live on MiscData086. Each admits by `ndsRelocNativeRootOffset` (18 sites, first at `src/port/renderer_adapter_stage.c:8793`) plus compared list words (26 raw pointer-word compare lines such as `src/port/renderer_adapter_stage.c:9300`, 27 raw size compares such as `src/port/renderer_adapter_stage.c:9297`). The compared words are 1,032 B (8 B per root and index, upper bound). Reading the same words unmodified needs the display list contiguous up to the highest compared index: 4,640 B more (`lane2_prefix.py`), which would take the item A to 15,072 B.
- Texel and palette bytes of the owned kinds (21,912 B) are read from the file at first draw of each kind: `tlut = base + OFFSET` (`src/nds/nds_native_item_hammer.exec.inc:32`, plus 21 raw size compares such as `src/nds/nds_native_item_hammer.exec.inc:26` across 23 exec files).
- The 12 Pokemon kinds and the weapons have no owner on this file; their textures (20,160 B) are kept in both scenarios because an owner added later would read them the same way, while their display lists and vertices count as dead. 680 B of typed bytes no known root reaches are kept as UNPROVEN.
- Cross-file readers: 68 ITCommonData slots point into MiscData086 and one Yoshi main-file slot (0x40 -> 0x5458) does too (`lane2_xrefs.py`: 69 slots from 2 of 2,132 containers; no other common file has any external referrer). Yoshi's closure shares the file (`src/import/battleship_item_link_core.c:832`). The thrown Master Ball effect recovers file 86's base by subtracting a source offset from a fixed-up pointer (`src/import/battleship_efmanager.c:2905`, `src/port/renderer_adapter_stage.c:9338`).
- Prior art: `scripts/items/item_memory_closure.py` (2026-09-05) reports the item file as not viable, saving 1,940 B, because its live-reader list included a display-list walker. That walker is gone from the ROM (23 owners replaced it); the script's label graph, pointer check (402/402) and numeric roots were reused here, its keep-all-typed-geometry rule was not.

### 4.5 Pools: configured capacity against measured high-water

**T5 pools: configured capacity vs measured high-water**

| pool | elem B | capacity | bytes | measured over 30 four-CPU runs (2026-09-06..09-26) | note |
|---|---:|---:|---:|---|---|
| EFStruct (effects) | 60 | 38 | 2,280 | free-min 0/24/26 -> live max 38/14/12 (min/median/max over runs) | SATURATED in 3 runs (free 0): keep |
| LBParticle structs | 96 | 112 | 10,752 | 33/37/53 | 47% of cap at worst |
| LBGenerator | 92 | 24 | 2,208 | 10/10/15 | 62% of cap at worst |
| LBTransform | 192 | 80 | 15,360 | 19/20/36 | 45% of cap at worst |
| ITStruct (items) | 924 | 16 | 14,784 | no concurrent-item counter exists | unmeasured |
| WPStruct (adjacent, not this lane) | 704 | 10 | 7,040 | live max 1/2/2 | out of scope |

- The effect-pool counter saturates in 3 of 30 runs (all the 2026-09-17 Captain / Luigi / Donkey Kong / Kirby roster runs): keep 38.
- `gNdsParticleRejectCount` is **not** a capacity witness in this code: it counts scripts refused because they are not packed (`lbParticleMakeScriptID` and siblings) and bank-load failures, and reads 41 in the three runs where the effect pool saturates. The capacity evidence is the three Max counters against 112 / 24 / 80.
- The particle capacities are the source's own sizes, chosen for the menu shell and the four-CPU target (`src/import/battleship_lbparticle.c:266`, `src/import/battleship_lbparticle.c:403`); direct-boot targets use 48 / 24 / 24. History matters here: a soak that reached KOs found generators and transforms saturated at their old caps and two of six KO bursts dropped for lack of a transform (`src/import/battleship_lbparticle.c:217`). The 64 / 20 / 48 lever keeps 21% / 33% / 33% headroom over the measured maxima (53 / 15 / 36) but the runs behind those maxima did not hold items, several KOs and Results at once (UNPROVEN, section 7).
- ITStruct: 16 slots of 924 B; no counter exists for concurrent items (`gNdsItemSpawnLawSpawnCount` counts spawns, 2 in most runs and 71 in one). Not proposed for reduction.

## 5. What can be handed back (question 4)

### 5.1 Per-file table

`must keep` = bytes some post-load reader still needs, or a pool or table in use. It is the same under A and B except that B moves the setup-time readers to NitroFS, so B's own keep would be lower by the B increment. Confidence is per column.

| fid | file / allocation | payload B | bytes by class | must keep B | still read after load: what + reader file:line | reclaim A B | reclaim A+B B | conf A/B | note |
|---|---|---|---|---|---|---|---|---|---|
| 166 | IFCommonPlayer | 976 | DObjDesc / DL / Vtx / anim / one IA8 image (not split) | 976 | everything kept: 976 B is not worth a span table | 0 | 0 | H/H |  |
| 82 | IFCommonGameStatus (resident compact image) | 21,056 | Sprite hdr 1,632 / Bitmap[] 816 / lamp+rod+frame pixels 18,016 / pad 592; already dropped: 12 letter payloads 131,232 of the 152,288 B source | 2,448 | headers+Bitmap[]: `nds_ifcommon_oam.c:2759-2797` (pointer+size match), `nds_ifcommon_oam.c:2658` (rebase); lamp/rod/frame pixels: read once by `nds_ifcommon_oam.c:2380` at scene entry; letters baked by `nds_ifcommon_oam.c:2458`, dropped by `reloc_backend_assets.c:15916` | 304 | 18,320 | H/M | A = pad only; B = the 18,016 B of pixels if the atlas prepare reads NitroFS |
| 82 | IFCommonGameStatus baked end streams (separate allocation) | 22,104 | run-length TIME UP / GAME SET banks | 0 | built by `nds_ifcommon_oam.c:2597`; decoded into the OBJ end bank once, at the announcement | 0 | 22,104 | -/M | B = keep them in a NitroFS stream read at the announcement (UNPROVEN cost) |
| 164 | IFCommonPlayerDamage | 5,664 | Sprite hdr 816 / Bitmap[] 192 / pixels 4,472 / pad 184 | 816 | Sprite headers: copied into SObjs by `objman.c:1591` via `sprite_preview_backend.c:133-155`; pixels: lower HUD draws damage digits from `battle_hud.bin` (`nds_battle_hud.c:251`); route `battleship_ifcommon.c:876-918`, state `battleship_ifcommon.c:757-874` | 4,704 | 4,704 | H/H |  |
| 165 | IFCommonTimer | 4,736 | Sprite hdr 1,020 / Bitmap[] 240 / pixels 3,280 / pad 196 | 1,020 | Sprite headers: copied into SObjs by `objman.c:1591` via `sprite_preview_backend.c:133-155`; pixels: same: lower HUD, `nds_battle_hud.c:251`/`battleship_ifcommon.c:876-918` | 3,536 | 3,536 | H/H |  |
| 36 | IFCommonDigits | 2,336 | Sprite hdr 884 / Bitmap[] 208 / pixels 1,080 / pad 164 | 884 | Sprite headers: copied into SObjs by `objman.c:1591` via `sprite_preview_backend.c:133-155`; pixels: same: lower HUD, `nds_battle_hud.c:251`/`battleship_ifcommon.c:876-918` | 1,296 | 1,296 | H/H |  |
| 197 | IFCommonBattlePause | 6,416 | Sprite hdr 1,088 / Bitmap[] 256 / pixels 4,872 / pad 200 | 1,088 | Sprite headers: copied into SObjs by `objman.c:1591` via `sprite_preview_backend.c:133-155`; pixels: no native owner: unrecognised SObjs are not drawn, only a failure is recorded (`sprite_preview_backend.c:791`, `sprite_preview_backend.c:833`, `nds_ifcommon_oam.c:2759-2797`) | 5,136 | 5,136 | M/M |  |
| 37 | IFCommonAnnounceCommon | 32,416 | Sprite hdr 1,904 / Bitmap[] 448 / pixels 29,632 / pad 432 | 1,904 | Sprite headers: copied into SObjs by `objman.c:1591` via `sprite_preview_backend.c:133-155`; pixels: no native owner (`nds_ifcommon_oam.c:2759-2797`, `sprite_preview_backend.c:833`); letters/period never reach an OAM emitter | 30,176 | 30,176 | M/M |  |
| 38 | IFCommonPlayerTags | 3,840 | Sprite hdr 408 / Bitmap[] 96 / pixels 3,264 / pad 72 | 504 | Sprite headers: copied into SObjs by `objman.c:1591` via `sprite_preview_backend.c:133-155`; pixels: tag bake reads I8 pixels at interface creation (`nds_ifcommon_oam.c:3445-3564`, call `battleship_ifcommon.c:469`); OAM emit uses the baked cell (`nds_ifcommon_oam.c:3901`) | 0 | 3,264 | H/H |  |
| 87 | IFCommonItem (arrow sprite) | 160 | Sprite hdr 68 / Bitmap[] 16 / pixels 56 / pad 20 | 160 | arrow bake at itManagerInitItems: `nds_ifcommon_oam.c:3290-3376`, call `battleship_ifcommon.c:489` | 0 | 0 | H/H | 160 B: not worth touching |
| 251 | ITCommonData (68 attribute rows + externs) | 3,392 | ITAttributes rows, extern chains | 3,392 | decoded once per kind by `battleship_item_link_core.c:368-448`; rows point into file 86 | 0 | 0 | H/H |  |
| 86 | MiscData086 (ITCommonObject) | 79,584 | texture 40,392 / display_list 12,064 / vertices 6,640 / dobjdesc 6,240 / animation 5,476 / pad 4,660 / material 2,296 / palette 1,680 / other 112 / dllink 16 | 57,252 | DObjDesc/MObjSub/anim/DLLink 13,468: `battleship_item_link_core.c:368-448`, `battleship_item_link_core.c:1024`; owner DL-word compares 1,032: `renderer_adapter_stage.c:8793`..; owned-kind textures 21,912: raw base+offset at first draw `nds_native_item_hammer.exec.inc:32`; owner-less kinds' textures 20,160: no reader; Yoshi shares the file (`battleship_item_link_core.c:832`) | 19,712 | 41,624 | H/M | A: DL 11,032 + Vtx 6,640 + pad 4,660 less 181 span rows and 56 root cells; B adds the bind-read textures |
| - | ITStruct pool (16 x 924) | 14,784 | pool | 14,784 | capacity `item.h:439`; no concurrent-item counter exists | 0 | 0 | H/H |  |
| 83 | EFCommonEffects1 | 52,736 | texture 46,580 / palette 600 / display_list 1,640 / dllink 240 / vertices 672 / animation 1,232 / material 824 / pad 940 / unplaced 8 | 22,272 | DObjDesc/MObjSub/anim/DLLink via EFDesc resolver `battleship_efmanager.c:1277-1343`; slash 13 + mdust 7 frames read once at scene prepare (`nds_native_damage_slash.exec.inc:124-188`, `nds_native_damage_fly_mdust.exec.inc:44-99`); DamageSlash 48 B of DL words compared (`renderer_adapter_stage.c:8438`, `renderer_adapter_stage.c:8471`); the other 20,064 B of reachable islands are unread | 30,176 | 50,240 | H/M | 22 kept runs, 3 DL root cells, 0 identity stubs |
| 84 | EFCommonEffects2 | 28,352 | texture 18,392 / palette 384 / display_list 2,080 / dllink 192 / vertices 944 / animation 2,936 / material 1,120 / dobjdesc 796 / pad 1,500 / unplaced 8 | 4,836 | EFDesc resolver `battleship_efmanager.c:1277-1343`; 10 entry roots admitted by raw address equality (`renderer_adapter_stage.c:6218-6227`); shadow keys need three offsets to map (`nds_renderer_textures_effects.c:5984-6052`, `nds_renderer_textures_effects.c:6054`); shadow maker is NULL (`reloc_backend_compat_shims.c:12811-12815`) | 23,164 | 23,164 | H/H | 15 kept runs, 11 DL root cells, 3 identity stubs |
| 85 | EFCommonEffects3 | 13,616 | texture 512 / palette 120 / display_list 6,184 / dllink 688 / vertices 2,896 / animation 688 / material 1,000 / pad 1,520 / unplaced 8 | 2,376 | EFDesc resolver `battleship_efmanager.c:1277-1343`; 9 roots admitted by raw dl - base compares (`renderer_adapter_stage.c:6631`, `renderer_adapter_stage.c:7438`); geometry/texels are ROM tables (`nds_renderer_textures_effects.c:3784`, `nds_renderer_native_common.c:6319`) | 11,096 | 11,096 | H/H | 6 kept runs, 9 DL root cells, 0 identity stubs |
| - | EFStruct pool (38 x 60) | 2,280 | pool / table | 2,280 | capacity `battleship_efmanager.c:208`; measured saturated in 3 of 30 runs | 0 | 0 | H/H |  |
| - | visual templates (7 x 352) | 2,464 | pool / table | 2,464 | procedural stand-ins built by `battleship_efmanager.c:593-621`; readers not audited (L) | 0 | 0 | L/L |  |
| - | particle pools (efParticleInitAll) | 28,320 | pool / table | 28,320 | allocated by `battleship_lbparticle.c:403`; capacities `battleship_lbparticle.c:266` | 0 | 0 | H/H |  |
| | **TOTAL** | 434,360 | | 147,776 | | **129,300** | **214,660** | | |

### 5.2 Where the reclaimed bytes come from, and what the overhead is

| class of dead byte | interface files | effect files | item file | total |
|---|---:|---:|---:|---:|
| pixels/texels with no reader after load | 43,336 | 22,536 | 0 | 65,872 |
| islands no source root reaches (EF only) | 0 | 24,972 | 0 | 24,972 |
| display lists (no interpreter linked) | 0 | 9,432 | 11,032 | 20,464 |
| vertices (geometry is ROM tables) | 0 | 4,320 | 6,640 | 10,960 |
| Bitmap[] arrays | 1,344 | 0 | 0 | 1,344 |
| pad / gaps | 1,768 | 3,960 | 4,660 | 10,388 |
| **gross dead** | | | | **134,000** |
| less span rows, root cells, identity stubs | | | | -4,700 |
| **A (reclaimable, no new reader)** | | | | **129,300** |

| family | what is charged | count | bytes |
|---|---|---:|---:|
| IF sprite files (5) | span row per kept Sprite header run, 12 B | 84 | 1,008 |
| IFCommonGameStatus | span row per kept sprite, 12 B | 24 | 288 |
| EF1-3 | span row per kept run (upper bound), 12 B | 43 | 516 |
| EF1-3 | 8 B root cell per reachable DObj display-list root | 23 | 184 |
| EF2 | 28 B stub (12 B span + 16 B bytes) per static-corpus identity key | 3 | 84 |
| MiscData086 | span row per kept run (upper bound), 12 B | 181 | 2,172 |
| MiscData086 | 8 B root cell per decoded display-list root | 56 | 448 |
| **total overhead** | | | **4,700** |
| gross dead bytes before overhead (A + overhead) | | | 134,000 |

- The 12 B per kept run is `NDSRelocIfSpan` / `NDSPreviewPackSpan` (`src/port/reloc_backend_assets.c:15750-15755`, `include/nds/nds_preview_pack.h` span row). It is an upper bound: a span row is only needed for a run some reader locates by source offset.
- The 8 B root cell is `{G_ENDDL, source root offset}` in the pack's dense root table. It is needed for **every reachable DObj display-list root, not only the admitted ones**: the draw pass calls the owners only for a DObj with a non-NULL list (`src/port/renderer_adapter_stage.c:12735` shows what happens next), and an owner-less effect must keep producing the NO_PROGRAM record it produces today rather than silently vanish. `ndsStageRejectNativeRender` already reads the root through `ndsRelocNativeRootOffset`.
- The fixup table (8 B per pointer slot) lives in the NitroFS pack, not in RAM; it needs a temporary buffer while loading.
- The three EF2 identity stubs exist because `ndsPreviewFileOffset` maps source offsets through spans only (`src/port/reloc_preview_pack.c:198-218`), so a key whose texel span was dropped would fail to build.

### 5.3 Mechanisms, and how offsets and fixups stay valid

Two mechanisms already exist in the tree.

- **M1, load-time compaction (IFCommonGameStatus precedent).** Load and finalize the whole file in free space at the top of the heap, copy the kept runs into an ordinary allocation, re-seat every internal pointer from the slots the fixup walk recorded, turn pointers into dropped runs into NULL, keep a span table for source-offset lookups (`src/port/reloc_backend_assets.c:15916`, `src/port/reloc_backend_assets.c:15768`, `src/port/reloc_backend_assets.c:15750-15755`). Costs a full-file window at load (152,288 B for GameStatus) and one copy; needs `MAX_SPANS` above its current 65 for anything but a coarse keep list.
- **M2, build-time pack (fighter FPC1 / battle-core precedent).** The generator emits only the kept runs, a fixup table, a span table and a dense table of identity root cells; the loader reads it straight into the final allocation (`scripts/fighters/generate_preview_core_packs.py:119`, `scripts/fighters/generate_preview_core_packs.py:211`, `scripts/fighters/generate_preview_core_packs.py:423`, `scripts/fighters/generate_battle_core_packs.py:2-16`, `src/port/reloc_preview_pack.c:851`). No window, no runtime copy, an independent decoder to verify it, and cross-file slots handled by a patch manifest (`src/port/reloc_preview_pack.c:925`).

| reader family | how it finds bytes | after a shrink | state |
|---|---|---|---|
| `lbRelocGetFileData(type, file, &llSym)` (IF sprite headers, item attribute rows) | `ndsRelocGetFileData` maps the source offset through the IF span table or the pack's spans, then checks the loaded size (`src/port/reloc_backend_assets.c:16332`) | unchanged | exists for both mechanisms |
| EFDesc offsets | `ndsEFManagerMapFileOffset` -> `ndsRelocNativeAssetAddress` / `ndsRelocNativeRootAddress` (`src/import/battleship_efmanager.c:1277-1343`, `src/port/reloc_preview_pack.c:198-218`, `src/port/reloc_preview_pack.c:165-196`) | unchanged | exists |
| internal pointers (DObjDesc.dl, MObjSub, anim, DLLink, Sprite.bitmap) | patched at load | re-seated to packed addresses; pointers into dropped runs are NULL | M1: recorded slots; M2: fixup rows |
| root identity for admission | `ndsRelocNativeRootOffset` decodes the ENDDL cell (`src/port/reloc_preview_pack.c:150-163`) | works for cell-aware owners (item owners' root check, DamageSlash, DamageFlyMDust, the reject record) | exists |
| raw address / raw `dl - base` compares (EF2 x10, EF3 x9) | plain arithmetic | **breaks**: convert to `ndsRelocNativeRootOffset` | conversion |
| raw pointer-word compares (`dl[i].w1 == base + OFFSET`) | plain arithmetic on a re-seated word | **breaks**: convert to root identity, or map the constant through `ndsRelocNativeAssetAddress` | conversion |
| raw size compares (`data_size >= FILE_END`, `file_bytes < FILE_END`) | the loaded size, which shrinks | **breaks**: use `ndsRelocNativeSourceSize` (`src/port/reloc_preview_pack.c:138-148`) | conversion |
| raw texel reads (`asset_base + offset`, `base + OFFSET`) | plain arithmetic | **breaks** for dropped runs; kept runs need the mapper | conversion |
| cross-file externs (ITCommonData -> 86, Yoshi main -> 86) | extern chains patched at load | retarget through spans / root cells | new (patch manifest, `src/port/reloc_preview_pack.c:925`) |
| static-corpus identity keys (EF2 x3) | `ndsRendererHardwareBuildBattleStaticTextureKey` (`src/nds/nds_renderer_textures_effects.c:5984-6052`) | need stub spans | new |
| thrown Master Ball base recovery by subtraction (`src/import/battleship_efmanager.c:2905`) | pointer minus source offset | breaks if file 86 is packed | new |

Recommendation: **M1 for the five IFCommon sprite files** (their readers are all token readers, the loader exists, no owner code changes) and **M2 for EF1-3 and MiscData086** (three admission families need root cells; a pack has them, a verifier, no window, and the extern manifest the item tree needs).

### 5.4 The layout-assuming code that must change first (counts by `lane2_sites.py`)

| where | what | lines matched |
|---|---|---:|
| `src/port/renderer_adapter_stage.c` item owners | root identity by `ndsRelocNativeRootOffset` (already cell-aware) | 18 |
| same | raw `base + NDS_NATIVE_ITEM_*_OFFSET` pointer-word compare lines | 26 |
| same | raw `data_size >= *_FILE_END` compare lines (items) | 27 |
| same | EF2 entry roots by raw address equality (`src/port/renderer_adapter_stage.c:6218-6227`) | 10 |
| same | EF3 roots by raw `dl - base` (`src/port/renderer_adapter_stage.c:6631`, `src/port/renderer_adapter_stage.c:7438`) | 11 |
| same | DamageSlash raw pointer-word fingerprints (`src/port/renderer_adapter_stage.c:8471`) | 2 |
| same | `loaded->data_size` uses in the file (all assets; only those tied to files 83-86 matter) / uses of `ndsRelocNativeSourceSize` | 88 / 1 |
| `src/nds/nds_native_item_*.exec.inc` (asset-86 owners) | raw `file_bytes < *_FILE_END` lines | 21 |
| same | raw `tlut/image = base + *_OFFSET` lines | 7 |
| `src/nds/nds_native_damage_slash.exec.inc`, `nds_native_damage_fly_mdust.exec.inc` | raw `asset_base + offset` texel reads (`src/nds/nds_native_damage_slash.exec.inc:124-188`, `src/nds/nds_native_damage_fly_mdust.exec.inc:44-99`) and the source-size preflight (`src/nds/nds_native_damage_fly_mdust.exec.inc:202`) | 3 |
| `src/nds/nds_renderer_native_common.c` | entry-effect owner branches keyed on asset 84 / 85 | 9 |

### 5.5 What would move the numbers

- **Unmodified item admission** (fingerprint words compared as today): +4,640 B kept, item A 15,072 B.
- **A native owner for an effect that has none today** (FlyOrbs, CommonSpark, FireSpark, NessPKFlash, ShockSmall): their textures are counted dead; the per-effect closures above bound the loss at 15,080 B of texture and palette (T3b, closure upper bound).
- **A native pause overlay or Sudden Death text**: costs the M-tier pixels (35,312 B) back, or a NitroFS reader for them.
- **B-tier**: each retargeted reader adds a NitroFS read at a time the game does not pay for today: EF1 frames (20,064 B) at scene prepare, GameStatus pixels (18,016 B) at scene entry, PlayerTags pixels (3,264 B) at interface creation, 22,104 B of end streams at the announcement, and the item textures (21,912 B) at the first draw of each kind, in the middle of a battle. None of those costs is measured (section 7).
- **Skipping the item files** (97,920 B, L): unavailable whenever Yoshi is in the match (his closure needs file 86), a bumper stage is loaded, or Pikachu / Jigglypuff can throw the Master Ball; needs a consumer census I did not do.

**T7 levers outside A and B**

| lever | bytes | conf | note |
|---|---:|---|---|
| particle pools right-sized 112/24/80 -> 64/20/48 | 11,120 | M | measured max 53/15/36 in 30 runs; KO burst needs a transform (silent missing effect if short) |
| skip item files + pool when no item can spawn | 97,920 | L | MiscData086 is also an extern of Yoshi's main file (slot 0x40 -> 0x5458, lane2_xrefs.py); ITCommonData is read by the thrown Master Ball effect (Pikachu/Jigglypuff), stage bumpers and the Poke Ball; UNPROVEN consumer census |

## 6. Recommended implementation outline

Order by bytes per unit of risk. Each step is independently shippable and each ends with the same three checks: replay digest identical, native-failure records identical by (domain, root, reason), and heap low-water improved by the step's bytes (net of overhead, to the 16 B alignment).

**Step 0 - instruments (no bytes).** `scripts/common/check_common_packs.py` (new): decode every generated pack independently and assert byte identity of every kept run against the O2R, every fixup slot and target, every admitted root has a cell, every offset a reader names lies in a kept span, and (for the item pack) the 68 + 1 cross-file slots resolve. Model it on `decode_pack` (`scripts/fighters/generate_preview_core_packs.py:423`) and `check_native_owner_image_spans.py`.

**Step 1 - EF2 + EF3 (34,260 B, H/H).**
- Generator: `scripts/common/generate_common_packs.py` (new), modeled on `scripts/fighters/generate_preview_core_packs.py:119`/`scripts/fighters/generate_preview_core_packs.py:211` and `scripts/fighters/generate_battle_core_packs.py:2-16`; keep-spec derived from `tools/lane2_effects.py` (keep_runtime, keep_fingerprint; drop the rest); output `nitro:/common/ef84.cpk`, `ef85.cpk` and `src/nds/generated/nds_common_pack.generated.h`.
- Loader: extend `ndsPreviewSection` (`src/port/reloc_preview_pack.c:87`) to consult a common-pack registry; add `ndsRelocLoadCommonPack` beside `src/port/reloc_backend_assets.c:15916`/`src/port/reloc_preview_pack.c:851` in `src/port/reloc_backend_assets.c` and `src/port/reloc_preview_pack.c`; return the packed size from `lbRelocGetFileSize` (`src/port/reloc_backend_assets.c:12753`) so `efManagerInitEffects`' allocation shrinks (`decomp/BattleShip-main/decomp/src/ef/efmanager.c:1754-1756`); read with `ndsRelocAssetReadRawRange` (`include/nds/nds_reloc_assets.h:78`).
- Conversions: in `src/port/renderer_adapter_stage.c` the EF2 root compares (`src/port/renderer_adapter_stage.c:6218-6227`) and the EF3 root compares (`src/port/renderer_adapter_stage.c:6631`, `src/port/renderer_adapter_stage.c:7438`); the entry-effect branches of `src/nds/nds_renderer_native_common.c`; identity stubs for the EF2 static keys (`src/nds/nds_renderer_textures_effects.c:5984-6052`).

**Step 2 - the five IFCommon sprite files (44,848 B; 9,536 H, 35,312 M).**
- Generalize the GameStatus compaction (`src/port/reloc_backend_assets.c:15916`, `src/port/reloc_backend_assets.c:15768`, `src/port/reloc_backend_assets.c:15750-15755`) from one file with one hard-coded keep policy to a table of (asset, keep runs); generate the tables with `scripts/if/generate_if_keep_runs.py` (new; the parser is `tools/lane2_sprites.py`); raise `NDS_RELOC_IF_COMPACT_MAX_SPANS`; size the block through `lbRelocGetAllocSize` (`src/port/reloc_backend_assets.c:16072`).
- No owner conversion. Keep Sprite headers, and keep Bitmap[] for IFCommonPlayerTags.
- Decide first whether a native pause overlay or Sudden Death text is planned (the M tier).

**Step 3 - EFCommonEffects1 (30,176 B, H/M; +20,064 B in B).**
- Same pack path as step 1. Convert DamageSlash's fingerprint compares to root identity (`src/port/renderer_adapter_stage.c:8438`, `src/port/renderer_adapter_stage.c:8471`); route the 13 + 7 frame reads through `ndsRelocNativeAssetAddress` (`src/nds/nds_native_damage_slash.exec.inc:124-188`, `src/nds/nds_native_damage_fly_mdust.exec.inc:44-99`) and the preflight through the source size (`src/nds/nds_native_damage_fly_mdust.exec.inc:202`).
- B: read those 20 frames from `nitro:` at scene prepare with `ndsRelocAssetReadRawRange` (`include/nds/nds_reloc_assets.h:78`) instead of keeping 20,064 B resident.

**Step 4 - MiscData086 (19,712 B, H/M; +21,912 B in B).** Highest edit surface per byte: 23 owners (section 5.4), the 68 + 1 cross-file slots (extern patch manifest, `src/port/reloc_preview_pack.c:925`), the Master Ball base recovery (`src/import/battleship_efmanager.c:2905`). Do it last, one owner family at a time, converting each owner's admission to root identity + `ndsRelocNativeAssetAddress` for texels before the pack is switched on, and verify with each owner's `CandidateStep` / `SubmitStep` counters (for example `gNdsItemHammerCandidateStep`, `gNdsItemHammerSubmitStep`).

**Step 5 - B-tier and levers.** GameStatus pixels (18,016 B) and the baked end streams (22,104 B) to NitroFS reads; PlayerTags (3,264 B); particle pool right-size (11,120 B) after a soak that forces KO bursts with items live; item-file skip only after a consumer census.

## 7. UNPROVEN, and assumptions

UNPROVEN:

1. **No other reader of file display-list bytes.** Proved: 0 of 28 forbidden symbols; the stage submit's fall-through is a NO_PROGRAM record; the variable-index list walkers in `renderer_adapter_stage.c` are the diagnostics hash (not linked) and DL-heap scanners; `ndsRendererNativeVisitSourceCommand` serves the fighter path. Not proved: that no other linked code indexes a `Gfx` of an item or effect file by a computed index. The claim about vertex bytes is the same argument.
2. **Owner-less effects** (FlyOrbs 0x7E80, CommonSpark 0x8FA0, FireSpark 0x1f78, NessPKFlash 0x6c28, ShockSmall 0x1500) rest on "no reference in `src/`" by offset regex; a computed reference would be missed.
3. **EF1's 23,476 B of unreferenced islands** rest on reachability from the decomp roots plus the port-side consumers found by asset id (83-86) in generated headers and `owner_asset_id` branches. Reachability is interval-level (an interval reached by one pointer counts whole), so A is a floor.
4. **BattlePause and AnnounceCommon** (35,312 B, M): no native owner today. If the owner intends to show the pause overlay or Sudden Death text natively, these pixels return to the keep list or need a NitroFS reader.
5. **Every B-tier cost** (NitroFS read latency at scene prepare, scene entry, interface creation, announcement and, worst, first item draw in battle) is unmeasured.
6. **ITStruct pool** (14,784 B): no concurrent-item counter exists, so no reduction is proposed.
7. **Particle-pool lever**: maxima are from 30 four-CPU stress runs (2026-09-06..26); worst case with items, several KOs and Results together is unmeasured; the rejected-script counter is not a capacity witness.
8. **Item file**: the 680 B of typed bytes no known root reaches are kept; the 12 Pokemon kinds' and weapons' textures (20,160 B) are kept for a future owner; the 68 + 1 cross-file slots and the Master Ball base recovery need the extern-patch treatment (not built).
9. **M1 window** for the item tree (82,976 B) and EF1 (52,736 B) is not evaluated against `NDS_RELOC_IF_COMPACT_MARGIN` (64 KiB); M2 avoids it.
10. **EF3 structural decode**: two label-rooted Gfx intervals (0x3130, 0x3150) do not decode to a terminated list and are classified by label. Effect textures whose structural extent exceeds their label extent are treated by label (conservative for A).
11. **Baseline**: the heap census that found the ~130 KB four-kind deficit was taken with compaction off (`artifacts/performance/2026-09-23_p2-2p8-if-gamestatus-compact/README.md`); the flag-on baseline used here (net 109,128 B better) was not re-measured on a four-fighter battle.

Assumptions:

- The shipping-like configuration is the saved `build-fp-argmax` plus HEAD source. Pool high-water comes from the four-CPU stress ROM (`smash64ds-p2-fourcpu-tickhud-hwtri`), which runs the same allocation code.
- Reclaim is credited 1:1 to the free top (all three owners allocate before the fighter files; 16 B alignment; nothing else sizes off these files).
- Overheads: 12 B per kept run (upper bound), 8 B per reachable display-list root, 28 B per identity key.
- Line numbers for `src/port/reloc_backend_assets.c` and `src/port/sprite_preview_backend.c` are for the pinned commit; the working copies are being edited by other work (compact ground maps) and would drift.
- NitroFS copies are byte-identical to the O2R files (verified).
- Scope is the VS battle (`scVSBattleStartBattle`). The 1P, bonus, training and Results scenes call the same loaders but their readers were not audited (1P, for one, also seeds an SObj from an IFCommonDigits Sprite header, `src/import/battleship_sc1pgame_runtime.c:85`); a change to these files needs those scenes re-checked.

## 8. Method and reproduction

Run from the repo root: `bash artifacts/performance/2026-09-28_p2-2p8-ram-supply/tools/lane2_run_all.sh`, then `python artifacts/performance/2026-09-28_p2-2p8-ram-supply/tools/lane2_build_report.py` (about 10 s, read-only over the repo, writes only into this folder). The run log is `tools/lane2_all_output.txt`.

| script | what it produces |
|---|---|
| `lane2_o2r.py` | O2R container parser and relocation-chain walker |
| `lane2_struct_sizes.py` | `sizeof` of the pool structs from the ELF's DWARF (arm-none-eabi-gdb, static) |
| `lane2_inventory.py` | file sizes, allocations, pools, NitroFS byte identity |
| `lane2_sprites.py`, `lane2_ifcommon.py`, `lane2_4c_check.py` | IFCommon byte classes, read model, no-4c proof |
| `lane2_partition.py`, `lane2_reach.py`, `lane2_dl.py`, `lane2_structural.py`, `lane2_effects.py`, `lane2_roots.py` | effect-file typed partition, reachability, F3DEX2 decode, read model, display-list roots |
| `lane2_items_offsets.py`, `lane2_items.py`, `lane2_prefix.py` | item-file placement (a slice of the closure script), read model, the unmodified-admission prefix |
| `lane2_static_keys.py`, `lane2_xrefs.py` | static-corpus keys into these files; whole-tree external-referrer census (2,132 containers) |
| `lane2_nm_check.py` | 0 of 28 forbidden interpreter symbols in the ELF |
| `lane2_pools.py` | pool counters from the tick-HUD JSON of 30 runs |
| `lane2_sites.py` | counts of layout-assuming owner code |
| `lane2_estimate.py`, `lane2_tables.py`, `lane2_perfile.py` | the master table, the levers, the tables in this report |
| `lane2_cites.py` | every `file:line` in this report, by regex against the blobs of the pinned commit (`git show`), not the working tree |
| `lane2_build_report.py` | this report, from `lane2_report_template.md` |

Method notes.

- Read tags per byte: `keep_runtime` (DObjDesc, MObjSub, animation, DObjDLLink), `keep_fingerprint` (list words an owner compares), `prepare_only` (read only while the effect is being prepared), `identity_only` (a static-corpus key needs an address, not the bytes), `dead_*` (no reader), `pad`.
- Determinism: every script is deterministic. An earlier draft of `lane2_partition.py` iterated Python sets (hash-seed dependent) and let a `<DL>_DLLink` label collide with its display list at one offset, which moved 28 B between classes in EFCommonEffects1 from run to run; it now sorts labels and places a `_DLLink` list after its display list (0x59B8, not 0x5948). The full pipeline was checked identical over six hash seeds.
- A counts only bytes with no reader; B adds `prepare_only` / `setup_read` bytes. The two never overlap and their difference is the sum of the B increments in the table above (85,360 B).
- Sprite-file overhead is one span row per Sprite header run; effect-file overhead is one span row per kept run, one root cell per reachable root, one stub per identity key; the item file is one span row per kept run plus one cell per decoded root.

## Appendix A. Owner-by-owner totals

| owner | flag off (census) | HEAD default |
|---|---:|---:|
| A `scVSBattleSetupFiles` | 208,672 | 99,560 |
| B `itManagerInitItems` (tree; pool + arrow are extra) | 82,976 | 82,976 |
| C `ndsBaseEFManagerInitEffects` (three files; pool and templates are extra) | 94,704 | 94,704 |

## Appendix B. Master table (all 19 rows)

**T6 master table**

| owner | fid | file / allocation | payload B | resident now B | must keep B | reclaimable A B | reclaimable A+B B | conf A/B |
|---|---:|---|---:|---:|---:|---:|---:|---|
| A | 166 | IFCommonPlayer | 976 | 976 | 976 | 0 | 0 | H/H |
| A | 82 | IFCommonGameStatus (compact image) | 152,288 | 21,056 | 2,448 | 304 | 18,320 | H/M |
| A | 82 | IFCommonGameStatus baked end streams | 0 | 22,104 | 0 | 0 | 22,104 | -/M |
| A | 164 | IFCommonPlayerDamage | 5,664 | 5,664 | 816 | 4,704 | 4,704 | H/H |
| A | 165 | IFCommonTimer | 4,736 | 4,736 | 1,020 | 3,536 | 3,536 | H/H |
| A | 36 | IFCommonDigits | 2,336 | 2,336 | 884 | 1,296 | 1,296 | H/H |
| A | 197 | IFCommonBattlePause | 6,416 | 6,416 | 1,088 | 5,136 | 5,136 | M/M |
| A | 38 | IFCommonPlayerTags | 3,840 | 3,840 | 504 | 0 | 3,264 | H/H |
| A | 37 | IFCommonAnnounceCommon | 32,416 | 32,416 | 1,904 | 30,176 | 30,176 | M/M |
| B | 87 | IFCommonItem | 160 | 160 | 160 | 0 | 0 | H/H |
| B | 251 | ITCommonData | 3,392 | 3,392 | 3,392 | 0 | 0 | H/H |
| B | 86 | MiscData086 (ITCommonObject) | 79,584 | 79,584 | 57,252 | 19,712 | 41,624 | H/M |
| B |  | ITStruct pool (16 x 924) | 14,784 | 14,784 | 14,784 | 0 | 0 | H/H |
| C | 83 | EFCommonEffects1 | 52,736 | 52,736 | 22,272 | 30,176 | 50,240 | H/M |
| C | 84 | EFCommonEffects2 | 28,352 | 28,352 | 4,836 | 23,164 | 23,164 | H/H |
| C | 85 | EFCommonEffects3 | 13,616 | 13,616 | 2,376 | 11,096 | 11,096 | H/H |
| C |  | EFStruct pool (38 x 60) | 2,280 | 2,280 | 2,280 | 0 | 0 | H/H |
| C |  | visual templates (7 x 352) | 2,464 | 2,464 | 2,464 | 0 | 0 | L/L |
| C |  | particle pools (efParticleInitAll) | 28,320 | 28,320 | 28,320 | 0 | 0 | H/H |
|  |  | **TOTAL** |  | 325,232 | 147,776 | 129,300 | 214,660 |  |

## Appendix C. Citation ledger

| id | location | why it is cited |
|---|---|---|
| A01 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattlefiles.c:23-37` | owner A: one lbRelocLoadFilesListed over dGMCommonFileIDs |
| A02 | `decomp/BattleShip-main/decomp/src/gm/gmcommon.c:11-20` | the eight IF file ids and slot order |
| A03 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:137` | VS setup order: IF files first |
| A04 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:153` | then particle pools |
| A05 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:160` | then item tree + pool |
| A06 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:164` | then effect files |
| A07 | `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:177` | then per-player fighter files (after all three owners) |
| A08 | `src/port/reloc_backend_assets.c:15916` | IFCommonGameStatus compact loader (top-of-heap window, copy kept runs, re-seat) |
| A09 | `src/port/reloc_backend_assets.c:16142` | loader call site; ndsTaskmanElasticReleaseAll() hands the window back first |
| A10 | `src/nds/nds_battle_hud.c:240-320` | lower HUD prepare |
| A11 | `src/nds/nds_battle_hud.c:251` | lower HUD glyphs come from a NitroFS blob, not the IF files |
| A12 | `src/nds/nds_platform.c:577` | shipping config routes timer/stock/damage to the lower HUD |
| A13 | `src/import/battleship_ifcommon.c:876-918` | timer/stock/damage GObjs are redirected; state only, no pixels |
| A14 | `src/import/battleship_ifcommon.c:757-874` | HUD state comes from game state, not sprite data |
| A15 | `src/port/sprite_preview_backend.c:791` | foreground SObjs: native OAM first |
| A16 | `src/port/sprite_preview_backend.c:833` | anything unrecognised records a native failure, is not drawn |
| A17 | `src/port/sprite_preview_backend.c:1004` | battle: lower-HUD route before the layered path |
| A18 | `src/port/sprite_preview_backend.c:137` | SObj creation expands 4c bitmaps in place (no IF sprite is 4c: lane2_4c_check.py) |
| A19 | `src/nds/nds_ifcommon_oam.c:2759-2797` | native OAM recognises SObjs by Bitmap pointer + size (GameStatus assets only) |
| A20 | `src/nds/nds_ifcommon_oam.c:3901` | native OAM draw: tag / arrow / recognised GameStatus assets |
| A21 | `src/nds/nds_ifcommon_oam.c:3445-3564` | tag bake reads the sprite's I8 pixels (setup) |
| A22 | `src/nds/nds_ifcommon_oam.c:3290-3376` | arrow bake reads the sprite pixels (setup) |
| A23 | `src/import/battleship_ifcommon.c:469` | tag bake call at interface creation |
| A24 | `src/import/battleship_ifcommon.c:489` | arrow bake call at itManagerInitItems' ifCommonItemArrowSetAttr |
| A25 | `src/nds/nds_ifcommon_oam.c:2101` | GameStatus prepare (bakes GO bank, binds metadata) |
| A26 | `src/nds/nds_ifcommon_oam.c:2380` | cloud/traffic atlas prepare: reads lamp/rod/frame pixels once |
| A27 | `src/nds/nds_ifcommon_oam.c:2458` | the twelve letter payloads dropped by the compact load |
| A28 | `src/nds/nds_ifcommon_oam.c:2597` | TIME UP / GAME SET run-length streams (22,104 B resident) |
| A29 | `src/nds/nds_ifcommon_oam.c:2658` | retained Sprite/Bitmap pointers moved into the image |
| A30 | `src/port/battle_playable_compat_stubs.c:201` | pause decals were once drawn (top-screen layered path, 2026-09-06); now only native owners draw |
| A31 | `src/port/sprite_preview_backend.c:133-155` | SObj creation copies the Sprite header (why headers stay); expands 4c only |
| A32 | `decomp/BattleShip-main/decomp/src/sys/objman.c:1591` | gcAddSObjForGObj copies the 68 B Sprite header into the SObj (headers are read whenever an interface SObj is made or re-pointed) |
| B01 | `src/import/battleship_item_link_core.c:807-902` | owner B: pool + extern tree load + arrow |
| B02 | `src/import/battleship_item_link_core.c:856` | 82,976 B tree (ITCommonData + MiscData086) in one allocation |
| B03 | `src/import/battleship_item_link_core.c:368-448` | ITAttributes decode: pointers to DObjDesc/MObjSub/anim rows (kept) |
| B04 | `src/import/battleship_item_link_core.c:1024` | DObj trees built from DObjDesc arrays |
| B05 | `include/it/item.h:439` | ITStruct pool capacity |
| B06 | `src/port/renderer_adapter_stage.c:8793` | first item owner admission (root identity + DL word compares follow) |
| B07 | `src/port/renderer_adapter_stage.c:9300` | example raw base+OFFSET pointer-word compare (dl[11].w1) that a re-seat would break |
| B08 | `src/port/renderer_adapter_stage.c:9297` | example raw data_size >= source FILE_END compare |
| B09 | `src/import/battleship_item_link_core.c:832` | MiscData086 is shared with Yoshi's fighter closure |
| B10 | `src/import/battleship_efmanager.c:2905` | thrown Master Ball effect reads ITCommonData through gITManagerCommonData (file 86 base recovered by subtraction) |
| B11 | `src/port/renderer_adapter_stage.c:9338` | decomp efManagerMBallThrownMakeEffect subtracts a source offset from a fixed-up pointer |
| B12 | `src/nds/nds_native_item_hammer.exec.inc:32` | item owner reads texel/palette bytes at raw base + offset at first draw (bind) |
| B13 | `src/nds/nds_native_item_hammer.exec.inc:26` | raw source-size compare against the loaded size |
| B14 | `src/nds/nds_native_item_hammer.exec.inc:2` | item owners bake geometry/state into ROM; only TLUT/image bytes are read from the file |
| C01 | `src/import/battleship_efmanager.c:154` | owner C: decomp efManagerInitEffects renamed; loads the three files and the EFStruct pool |
| C02 | `src/import/battleship_efmanager.c:208` | EFStruct pool = NDS_R2_EFFECT_POOL (38) x 60 B |
| C03 | `src/import/battleship_efmanager.c:2359-2417` | init sequence incl. desc resolve and visual templates |
| C04 | `src/import/battleship_efmanager.c:1107` | EFDesc span check uses the LOADED file size |
| C05 | `src/import/battleship_efmanager.c:1277-1343` | EFDesc offsets already resolve through ndsRelocNativeAssetAddress / ndsRelocNativeRootAddress |
| C06 | `src/import/battleship_efmanager.c:593-621` | 7 x 352 B procedural templates (heap) |
| C07 | `src/port/renderer_adapter_stage.c:6218-6227` | EF2 entry-effect roots: RAW address equality (must become cell-aware) |
| C08 | `src/port/renderer_adapter_stage.c:6631` | EF3 MBallRays/ItemGetSwirl roots: raw dl - base compare |
| C09 | `src/port/renderer_adapter_stage.c:7438` | EF3 RebirthHalo roots: raw dl - base compare |
| C10 | `src/port/renderer_adapter_stage.c:8438` | EF1 DamageSlash: cell-aware root, then raw fingerprint compares |
| C11 | `src/port/renderer_adapter_stage.c:8471` | DamageSlash raw pointer-word fingerprint |
| C12 | `src/nds/nds_native_damage_slash.exec.inc:124-188` | 13 CI4 frames read at scene prepare, raw asset_base + offset |
| C13 | `src/nds/nds_native_damage_fly_mdust.exec.inc:44-99` | 7 frames read at scene prepare, raw offsets |
| C14 | `src/nds/nds_native_damage_fly_mdust.exec.inc:202` | preflight compares source offsets against the loaded size |
| C15 | `src/nds/nds_renderer_textures_effects.c:3754` | ImpactWave texels are ROM tables |
| C16 | `src/nds/nds_renderer_textures_effects.c:3784` | RebirthHalo texels are ROM tables |
| C17 | `src/nds/nds_renderer_native_common.c:6319` | entry-effect texels are ROM tables (sNdsEntryEffectTextures) |
| C18 | `src/port/reloc_backend_compat_shims.c:12811-12815` | shadow maker returns NULL: decomp ftshadow.c never runs |
| C19 | `src/nds/nds_renderer_textures_effects.c:5984-6052` | static-corpus key build: identity only, fails if the offset does not map |
| C20 | `src/nds/nds_renderer_textures_effects.c:6054` | every applicable record must build a key or stage admission declines |
| C21 | `src/import/battleship_lbparticle.c:266` | particle pool capacities 112/24/80 (menu shell / four-CPU) |
| C22 | `src/import/battleship_lbparticle.c:403` | pool allocation |
| C23 | `src/import/battleship_lbparticle.c:217` | history: a saturated transform pool silently drops KO bursts |
| C24 | `src/import/battleship_efmanager.c:2023` | resolves every EFDesc against the loaded EF/IT/FT files at init |
| C25 | `src/port/reloc_backend_compat_shims.c:8252` | source (file/particle) effect makers reached for a subset of effect kinds |
| C26 | `src/port/reloc_backend_compat_shims.c:8662` | hit sparks / fly orbs / star-rod spark go to procedural visual templates, not EF file lists |
| C27 | `decomp/BattleShip-main/decomp/src/ef/efmanager.c:1754-1756` | decomp: the three EF files, three separate 16 B-aligned allocations sized by lbRelocGetFileSize |
| C28 | `src/port/renderer_adapter_stage.c:12735` | fall-through of the stage/effect/item submit: no owner admitted the list -> NO_PROGRAM record, nothing executed |
| C29 | `src/port/renderer_adapter_stage.c:1279-1306` | DL-heap scanner: walks the frame's emitted list (gSYTaskmanDLHeads), not a file list |
| C30 | `src/port/renderer_adapter_stage.c:1378-1440` | DL-heap scanner for item procs: prim/env colour and other-mode words of the frame's list |
| C31 | `src/port/renderer_adapter_stage.c:1324-1371` | DL-heap scanner for effect procs |
| M01 | `src/port/reloc_preview_pack.c:150-163` | root identity from an ENDDL cell (pack) or dl - data (plain) |
| M02 | `src/port/reloc_preview_pack.c:165-196` | source root offset -> cell address |
| M03 | `src/port/reloc_preview_pack.c:198-218` | source offset -> packed address through spans |
| M04 | `include/nds/nds_preview_pack.h:67-71` | 12 B span row |
| M05 | `include/nds/nds_preview_pack.h:60-63` | 8 B fixup row (file only, not resident) |
| M06 | `scripts/fighters/generate_preview_core_packs.py:119` | fighter pack generator (spans, fixups, root cells) |
| M07 | `scripts/fighters/generate_preview_core_packs.py:211` | 8 B root cells {ENDDL, source root offset} |
| M08 | `scripts/fighters/generate_preview_core_packs.py:423` | independent decoder used as the pack's verifier |
| M09 | `scripts/check_native_only_rom.py:13` | display-list interpreter symbols the ROM must not link |
| M10 | `Makefile:7515` | the guard runs at ROM packaging (ARM9 image) |
| M11 | `src/port/reloc_backend_assets.c:11516-11526` | allocation size = 16 B-aligned payload size |
| M12 | `src/port/reloc_backend_assets.c:802` | 16 B allocation alignment |
| P01 | `scripts/fighters/generate_battle_core_packs.py:2-16` | battle-time pack precedent: root identity cells + extern patch manifest (BEX1) for cross-file slots |
| P02 | `src/port/reloc_preview_pack.c:925` | runtime patch of a packed file's cross-file externs once the other files are resident |
| P03 | `src/port/reloc_preview_pack.c:851` | pack load + publish: the loader a common-file pack path would mirror |
| P04 | `src/port/reloc_preview_pack.c:138-148` | source extent of a packed file (what raw size compares must use) |
| R01 | `src/port/reloc_backend_assets.c:16332` | ll* offset-token resolver: IF compact map or FPC span map, then size check (already handles both) |
| R02 | `src/port/reloc_backend_assets.c:15768` | IF compact source->image offset map (static table, 65 rows max) |
| R03 | `src/port/reloc_backend_assets.c:15750-15755` | IF span row (12 B) |
| R04 | `src/port/reloc_backend_assets.c:16072` | sizes the IFCommon block that lbRelocLoadFilesListed allocates (compact GameStatus counted as a placeholder); EF and item allocations are sized by lbRelocGetFileSize instead |
| R05 | `include/nds/nds_reloc_assets.h:78` | NitroFS range reader for retargeted (scenario B) readers |
| R06 | `src/port/reloc_backend_assets.c:5909` | what owners call to get (base, size): size is the LOADED (compact) size |
| R07 | `src/port/reloc_preview_pack.c:87` | pack section lookup: keyed to the fighter resident table today |
| R08 | `src/port/reloc_backend_assets.c:16141` | elastic motion cache hands the heap top back before a compaction window is used |
| R09 | `src/port/reloc_backend_assets.c:12753` | the size hook the EF/IT allocations are made from (return the packed size here) |
| S01 | `src/import/battleship_sc1pgame_runtime.c:85` | 1P also reads an IFCommonDigits Sprite header (not audited here: scope is the VS battle) |
