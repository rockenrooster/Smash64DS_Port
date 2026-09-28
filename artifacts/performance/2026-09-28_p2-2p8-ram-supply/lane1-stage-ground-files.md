# Lane 1: which bytes of the stage ground files a VS battle needs (2026-09-28)

Scope: the nine VS stages, shipping-shell configuration. Read-only investigation: no build, no emulator, no repo
edit. Every figure below is produced by a script in `tools/` (section 9); every `file:line` was re-resolved against
the working tree at report time by `tools/lane1_cite_lines.py` (`lane1_citations.json`). Re-resolving matters:
`src/port/reloc_backend_assets.c` was edited concurrently while this lane ran (an elastic motion cache landed after
about line 13,388), and the line numbers of everything past that point moved by ~130. Nothing cited here is inside
that hunk, but treat a line number as "the definition near here" and search by function name if it has drifted.

## 0. The answer in one screen

1. `mpCollisionInitGroundData` allocates the map file's whole extern tree: **176,880 to 239,600 B per stage**
   (Table 1). It is not "the ground file": it is four to five O2R files (map, geometry bank, optional images bank,
   actor bank, wallpaper container; Sector adds the Arwing). Dream Land computes to 202,816 B, exactly the
   2026-09-23 census figure, which is the check that the tree walk here is the runtime's.
2. **158,928 B of every tree is the stage's wallpaper container** (66% to 90% of the tree; Table 4): 158,152 B of
   44 RGBA16 pixel strips plus 776 B of Bitmap-table and Sprite headers. The DS never reads those pixels. The
   background is a separate native image (`native_wallpaper_<stage>.bin`) streamed from NitroFS into BG2, and the
   resident container is consulted only for the Sprite header and for the *address* of its Bitmap table (pointer
   provenance). This one class is worth **158,144 B net per stage, on all nine stages**, and it is the only
   reclaim I rate high confidence. It is a build-time "compact ground map": the map file grows by 784 B and stops
   depending on the container. I built the nine variants and re-parsed them with the stage generator's own parser
   (section 8, `lane1_variant_maps.json`): 9/9 parse, external fixups -1, internal fixups +2, tree shrinks by
   exactly 158,144 B each.
3. Three smaller findings, each with its own confidence (Tables 3 and 5):
   - **T2** the Gfx and Vtx of the display lists the native stage packet draws (packet-only, no other pointer
     reaches them): 7.7 to 14.7 KB net per stage. Their contents are never read; only their addresses are.
     Medium confidence, needs the fighter-pack address seams.
   - **T3** Dream Land only: 14,072 B of texels and palettes that the P1 static texture corpus covers. Low to
     medium; Dream Land is the frozen P1 stage and the corpus engagement is UNPROVEN here.
   - **T4** Kongo Jungle only: its packet lists `ExternDataBank107` (27,792 B, actually Mushroom Kingdom's geometry
     bank) as asset 0, and nothing in the packet indexes it. It is preloaded anyway because admission requires it.
     Medium; it may land in the front-end overlay loan rather than the heap.
4. T1 alone (158,144 B) is more than the ~130 KB the 2026-09-23 census says the heaviest four-kind roster is short at
   battle load, and 23% to 28% of the 570-700 KB the compact motion bank needs.
5. Everything else stays: anim scripts, collision, DObjDesc, MObjSub lists, DLLink tables, headers, dynamic and
   non-packet textures. For the eight blob stages the packet's own textures (3.3 to 29.8 KB) are also read once by
   the warm upload, so they cannot be dropped by stripping; they become reclaimable only if each stage gets a
   baked pin set like Dream Land's (T5, not claimed).

Per-stage reclaim, net of the precedent scheme's bookkeeping (T1 is the recommendation; the rest is upside).
The census's guess that "the ground file's render-only parts" were the lever was right in kind and wrong in
location: on Dream Land the display lists, vertices, texels and palettes total 30,220 B; the wallpaper
container alone is 158,928 B.

**Table 0. Reclaim per stage (bytes).**

| stage | tree alloc | T1 (high) | T1+T2 (T2 medium) | T1..T4 (all tiers) | tree after T1 |
|---|---:|---:|---:|---:|---:|
| Peach's Castle | 176,880 | 158,144 | 165,888 | 165,888 | 18,736 |
| Sector Z | 226,192 | 158,144 | 171,312 | 171,312 | 68,048 |
| Kongo Jungle | 225,392 | 158,144 | 168,276 | 196,068 | 67,248 |
| Planet Zebes | 219,872 | 158,144 | 166,452 | 166,452 | 61,728 |
| Hyrule Castle | 185,920 | 158,144 | 169,644 | 169,644 | 27,776 |
| Yoshi's Island | 229,280 | 158,144 | 166,548 | 166,548 | 71,136 |
| Dream Land | 202,816 | 158,144 | 167,972 | 182,044 | 44,672 |
| Saffron City | 239,600 | 158,144 | 172,848 | 172,848 | 81,456 |
| Mushroom Kingdom | 192,224 | 158,144 | 166,696 | 166,696 | 34,080 |

## 1. Method

### 1.1 The resident set (Q1)

`mpCollisionInitGroundData` (port shim, `src/port/reloc_backend_compat_shims.c:12941`) does
`lbRelocGetExternHeapFile(file_id, syTaskmanMalloc(lbRelocGetFileSize(file_id), 0x10))`
(`src/port/reloc_backend_compat_shims.c:13044`). `lbRelocGetFileSize` (`src/port/reloc_backend_assets.c:12753`) is `ndsRelocExternTreeAllocSize`
(`src/port/reloc_backend_assets.c:11562`): the file's 16-byte-aligned payload plus, for each entry of its O2R extern-id table
in order, the same recursively, each file counted once, and a dependency skipped if `ndsRelocNativeEntryOwnsDependency`
(`src/port/reloc_backend_assets.c:8937`) says the native path owns it. `tools/lane1_o2r.py` reimplements exactly that walk
over the O2R headers (parser reused from `scripts/stages/generate_nds_native_stage.py:load_o2r`,
`scripts/stages/generate_nds_native_stage.py:1425`). The tree is one plain taskman-heap block. The front-end overlay loan (305,408 B in the shell ELF:
`__nds_frontend_end - __nds_frontend_start`; `ndsFrontendOverlayTryAlloc`, `src/nds/nds_frontend_overlay.c:103`) is used instead by
`ndsRelocEnsureLoadedAsset` dependency loads, fighter packs (`ndsSceneAssetAlloc`, `src/nds/nds_frontend_overlay.c:130`) and native
image tables, never by this tree.

Beyond the tree the scene adds (a) `ndsRendererAdapterNativeStagePreloadAssets` (`src/port/renderer_adapter_matrix.c:1702`, called at
`src/port/reloc_backend_compat_shims.c:13075`): every asset the packet lists that the tree did not load (only Jungle has one), placed by
`ndsRelocEnsureLoadedAsset` (`src/port/reloc_backend_assets.c:8863`), which tries the overlay loan first
(`ndsRelocStaticBufferForAsset`, `src/port/reloc_backend_assets.c:8828`); (b) the native stage blob body
(`ndsNativeStageBlobLoad`, `src/port/reloc_backend_compat_shims.c:13071`; `syTaskmanMalloc` at `src/nds/nds_native_stage_blob.c:251`; Dream Land's packet is linked
instead); (c) the Phase 2 GX template body (`ndsStageGxLoad`, `syTaskmanMalloc` at `src/nds/nds_stage_gx.exec.inc:224`), loaded at the
first stage draw and only if `NDS_STAGE_GX_HEAP_KEEP_FREE` bytes remain (`src/nds/nds_stage_gx.exec.inc:8`). (b) and (c) are not
O2R files and not reclaimed here; Table 1 lists them because they are resident stage data. Their sizes are the
`body` fields of the files in `builds/build-p2p8-s1/nitrofs/stages/` (latest lab build, not rebuilt by me).
The gr* ground code loads no further files (`grep` of `gr/grcommon/gr*.c` and `src/import/battleship_gr*_ground.c`:
only small `syTaskmanMalloc` tables; Sector's Arwing looks FoxSpecial3 up with `lbRelocGetForceStatusBufferFile`, it
does not load it). `scVSBattleSetupFiles` loads the common battle files, not stage files.

### 1.2 The class of every byte (Q2)

Two independent methods, cross-checked.

1. **Typed tiling (exact).** The decomp's `relocData/<id>_<Name>.c` files are, per `relocData.md`, 100% typed
   declarations in file order. `tools/lane1_typed.py` parses the US branch of each and sums `sizeof` in order, which
   gives every declaration's offset and length (Gfx 8 B, Vtx 16 B, DObjDesc 44 B, MObjSub 0x78 B, MPGeometryData
   28 B, pointer arrays 4 B, u8/u16/u32 arrays by count, `PAD(n)`, natural alignment). `tools/lane1_tiling.py` maps
   each to a class (textures by `@tex` annotations and `Tex_` names, palettes by `Lut`/`palette`/`lut=` and 16-colour
   `u16[16]` blocks, scripts and tables to `anim`). Validation (`lane1_tiling_validation.json`): **21 of 21** typed
   files tile to exactly their O2R payload size (after the 16-byte tail alignment), **2,717** declarations, and
   **all 2,469 relocation slots** of those files lie inside pointer-bearing declarations (0 outside). The map files
   and wallpaper containers have no typed source: the map is MPGroundData at 0x14 (0xA8 B, the struct size the port
   asserts in `ndsRelocNormalizeGroundDataBounds`) plus item weights plus an attribute tail; the container is
   Bitmap[44] at 0x269C8, Sprite at 0x26C88, and pixel strips before 0x269C8.
2. **Reachability walker.** `tools/lane1_classify.py` starts from the roots the code uses (the map header's
   `gr_desc[4]`, `map_geometry`, `wallpaper`, `map_nodes`, `item_weights` through the relocation table), parses
   DObjDesc arrays (sentinel `id == 18`, `gcSetupCustomDObjs`, `decomp/BattleShip-main/decomp/src/sys/objanim.c:2332`), DObjDLLink tables (sentinel
   `list_id == 4`), decodes F3DEX2 display lists with the RDP state carried into sub-lists (G_VTX, G_SETTIMG plus
   LOADBLOCK/LOADTILE/LOADTLUT for texel and palette extents, G_MOVEMEM, G_MTX, G_DL and branch), and follows
   MObjSub `sprites`/`palettes` arrays. Unreached bytes stay "unreached", never "free". Over the nine stages the
   walker reaches 1,728,800 bytes and 1,725,422 of them (99.80%) carry the same class as the tiling.
   The disagreements are the walker's gap-bounded extents (collision arrays, MObj texture extents that swallow an
   unreferenced neighbouring palette): tex->pal 2,224 B; coll->other16 540 B; mobj->anim 248 B; tex->mobj 152 B; tex->pad 148 B; tex->anim 36 B; coll->pad 18 B; mobj->pad 8 B; coll->mobj 4 B. Reported class totals are the tiling's.

Roots that only code reaches by computed offset (the `ll<Stage>Map*` symbols: Whispy, Bronto, Dedede, Arwing,
Lakitu and so on, resolved against `map_nodes - llXxxMapHead` or, for `efground.c`, against
`gr_desc[1].dobjdesc - o_data`, `decomp/BattleShip-main/decomp/src/ef/efground.c:1594`) are why the walker alone leaves most of the actor banks
unreached; the tiling covers them.

### 1.3 The reader model (Q3)

Three sources, in order of authority:

- **The linked ELF as the oracle for what code exists**: `builds/build-ovl1p/smash64ds-p2-shell-hwtri.elf` (shell
  configuration, 2026-09-28 11:21), `arm-none-eabi-nm`/`objdump` only. It confirms `grWallpaperMake{Common,Static,
  Sector,DecideKind}`, `mpCollisionInitGroundData`, `ndsNativeStageBlobLoad`, `ndsRendererAdapterNativeStagePreloadAssets`,
  `ndsNativeWallpaperDraw`, `ndsNativeBattleWallpaperDraw/Preload`, `ndsStageGxLoad` are linked, and gives the exact
  callers of each (section 4.5) and of the generic stage walker (section 4.3).
- **The native stage packet** (`generate_nds_native_stage.generate`, run per stage): its binding roots, its image
  references (`STATE_EFFECT_IMAGE` deltas, `scripts/stages/generate_nds_native_stage.py:2307`), its owner list. `tools/lane1_readers.py`
  walks the packet's DL roots with the walker and builds the relocation-slot graph over the tiling: a Gfx/Vtx
  declaration is **packet-only** iff the packet's roots reach it and every relocation slot anywhere in the tree that
  points into it comes from a packet-owned DObjDesc/DLLink table or from another packet-only declaration
  (greatest fixpoint). A DL that Bronto's or Lakitu's DObjDesc points at is excluded by construction.
- **Source reading**, cited per class in section 4.

### 1.4 Reclaim accounting

T1 net = tree allocation before minus after, recomputed with `lbRelocGetFileSize`'s alignment rule on the variant
tree. T2 net = packet-only Gfx+Vtx minus 8 B root cell per packet root, minus 12 B span row per removed run (bounded
by removed declarations + files), minus 32 B section row per file. T3 net = static-corpus texel/palette bytes minus
8 B per record identity cell (40 assumed). T4 = the phantom asset's aligned size.

## 2. Q1: resident files per stage

**Table 1. Resident set after `mpCollisionInitGroundData` (O2R files, bytes).**

| stage (gkind) | map | resident O2R files: id name payload B | tree alloc B | extra preload | blob body | .gxp body |
|---|---|---|---:|---|---:|---:|
| Peach's Castle (0) | 259 | 259 GRCastleMap 192; 106 ExternDataBank106 17,696; 90 MVOpeningRoomWallpaper 158,928; 156 MiscDataBank156 64 | 176,880 | - | 9,680 | 17,760 |
| Sector Z (1) | 262 | 262 GRSectorMap 304; 109 ExternDataBank109 47,120; 99 StageSector 158,928; 153 MiscDataBank153 7,680; 161 FoxSpecial3 12,160 | 226,192 | - | 17,599 | 30,068 |
| Kongo Jungle (2) | 261 | 261 GRJungleMap 224; 108 ExternDataBank108 62,944; 92 StageJungle 158,928; 158 MiscDataBank158 3,296 | 225,392 | asset 107: 27,792 | 14,516 | 28,060 |
| Planet Zebes (3) | 257 | 257 GRZebesMap 224; 105 ExternDataBank105 57,184; 89 StageZebes 158,928; 157 MiscDataBank157 3,536 | 219,872 | - | 13,842 | 19,840 |
| Hyrule Castle (4) | 265 | 265 GRHyruleMap 224; 113 ExternDataBank113 26,768; 95 StageCastle 158,928 | 185,920 | - | 14,131 | 30,204 |
| Yoshi's Island (5) | 263 | 263 GRYosterMap 192; 111 ExternDataBank111 47,408; 110 ExternDataBank110 21,040; 93 StageYoshi 158,928; 154 MiscDataBank154 1,712 | 229,280 | - | 12,524 | 29,452 |
| Dream Land (6) | 255 | 255 GRPupupuMap 192; 104 ExternDataBank104 17,392; 103 ExternDataBank103 12,224; 88 StageDreamLand 158,928; 152 MiscDataBank152 14,080 | 202,816 | - | linked (0) | 32,140 |
| Saffron City (7) | 264 | 264 GRYamabukiMap 832; 112 ExternDataBank112 66,160; 94 StagePokemon 158,928; 160 MiscDataBank160 2,704; 159 MiscDataBank159 10,976 | 239,600 | - | 18,209 | 36,296 |
| Mushroom Kingdom (8) | 260 | 260 GRInishieMap 368; 107 ExternDataBank107 27,792; 91 StageHyruleWallpaper 158,928; 155 MiscDataBank155 5,136 | 192,224 | - | 14,088 | 24,188 |

Notes.

- Every wallpaper container is 158,928 B and the same layout (Sprite at 0x26C88, `llStage<X>Sprite`,
  `include/reloc_data.h`). The map header's `wallpaper` slot is an external fixup to it. The file names are the
  decomp's, not the stage's: Castle borrows `MVOpeningRoomWallpaper` (file 90), Hyrule borrows `StageCastle` (95),
  Mushroom Kingdom borrows `StageHyruleWallpaper` (91), Saffron borrows `StagePokemon` (94).
- Jungle: the packet lists bank 107 as asset 0 and `ndsRendererAdapterNativeStagePreloadAssets` loads it
  (27,792 B, "extra preload"), see 4.2.
- Sector's MiscDataBank153 and FoxSpecial3 are the Arwing (both required: `grSectorMakeGround` reads them). Saffron's
  MiscDataBank159 and 160 are the gate and Pokemon actors.
- The blob and `.gxp` columns are body sizes, not in the tree and not reclaimed here.

## 3. Q2: every byte of the tree by class

**Table 2. Every byte of the resident tree by class (typed tiling, exact; each row sums to the payload total).**

| stage | wallpaper pixels | wallpaper Sprite+Bitmap | Gfx | Vtx | texels | TLUT | DObjDesc+DLLink | anim scripts/tables | MObjSub+ptr lists | collision | header+attr | pad+u16 other | total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 158,152 | 776 | 5,144 | 4,192 | 4,840 | 416 | 1,144 | 1,096 | 0 | 530 | 188 | 402 | 176,880 |
| Sector Z | 158,152 | 776 | 9,816 | 12,976 | 19,820 | 1,152 | 2,412 | 19,476 | 128 | 520 | 304 | 660 | 226,192 |
| Kongo Jungle | 158,152 | 776 | 7,272 | 5,360 | 38,884 | 832 | 1,936 | 10,992 | 152 | 408 | 224 | 404 | 225,392 |
| Planet Zebes | 158,152 | 776 | 5,456 | 5,248 | 17,316 | 1,320 | 2,088 | 25,540 | 2,788 | 396 | 224 | 568 | 219,872 |
| Hyrule Castle | 158,152 | 776 | 6,264 | 6,496 | 11,904 | 416 | 980 | 0 | 0 | 406 | 224 | 302 | 185,920 |
| Yoshi's Island | 158,152 | 776 | 6,616 | 5,280 | 34,976 | 896 | 2,420 | 17,676 | 1,280 | 494 | 188 | 526 | 229,280 |
| Dream Land | 158,152 | 776 | 7,360 | 4,880 | 15,116 | 2,864 | 3,124 | 8,616 | 760 | 488 | 188 | 492 | 202,816 |
| Saffron City | 158,152 | 776 | 11,608 | 8,288 | 44,556 | 1,728 | 2,800 | 8,316 | 1,108 | 554 | 832 | 882 | 239,600 |
| Mushroom Kingdom | 158,152 | 776 | 6,360 | 4,928 | 14,992 | 288 | 1,752 | 3,068 | 704 | 530 | 368 | 306 | 192,224 |

Reading it: after the wallpaper, the two classes that dominate are texels (4.8 KB Castle to 44.6 KB Saffron) and anim
scripts/tables (Zebes 25.5 KB, Sector 19.5 KB: material animation streams). Gfx+Vtx together are only
9.3 to 22.8 KB per stage. Per-file rows are in the appendix.

**Table 4. Wallpaper share of the tree, what T1 leaves, and the T5 candidate.**

### Wallpaper share and what T1 leaves

| stage | tree alloc | wallpaper container share of tree | after T1 | packet-referenced texels+TLUT (T5 candidate, not claimed) | packet-only Gfx+Vtx gross | packet DL roots |
|---|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 176,880 | 89.9% | 18,736 | 3,288 | 8,640 | 12 |
| Sector Z | 226,192 | 70.3% | 68,048 | 14,816 | 14,584 | 19 |
| Kongo Jungle | 225,392 | 70.5% | 67,248 | 29,760 | 11,712 | 30 |
| Planet Zebes | 219,872 | 72.3% | 61,728 | 10,088 | 9,568 | 26 |
| Hyrule Castle | 185,920 | 85.5% | 27,776 | 12,320 | 12,760 | 15 |
| Yoshi's Island | 229,280 | 69.3% | 71,136 | 20,344 | 9,520 | 19 |
| Dream Land | 202,816 | 78.4% | 44,672 | 0 | 11,632 | 42 |
| Saffron City | 239,600 | 66.3% | 81,456 | 11,840 | 16,376 | 21 |
| Mushroom Kingdom | 192,224 | 82.7% | 34,080 | 11,392 | 9,800 | 23 |

## 4. Q3: who reads which bytes after load

### 4.1 The wallpaper container (T1)

Creation. The VS battle start runs `mpCollisionInitGroundData` then `grWallpaperMakeDecideKind`
(`decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:155`; `default:` arm `grWallpaperMakeCommon`, Yoster `grWallpaperMakeStatic`, Sector
`grWallpaperMakeSector`; `gMPCollisionGroundData->wallpaper` at `gr/grwallpaper.c` `decomp/BattleShip-main/decomp/src/gr/grwallpaper.c:147`). The port's
`lbCommonMakeSObjForGObj` (`src/port/sprite_preview_backend.c:133`) reads the Sprite header and copies it into the SObj; it dereferences
`Bitmap.buf` only for a 4c-size sprite (`lbCommonDecodeSpriteBitmapsSiz4b`, `src/port/sprite_preview_backend.c:118`), and the wallpaper is
RGBA16.

Draw, route 1: the SObj path. `lbCommonDrawSObjAttr` (`src/port/sprite_preview_backend.c:962`) sends a battle wallpaper GObj to
`ndsDrawLayeredSObjFrame` (`src/port/sprite_preview_backend.c:741`), which snapshots the SObj and later calls
`ndsSObjDrawCachedWallpaperFinal` (`src/port/sprite_preview_backend.c:558`). That function's only use of the resident file is
`ndsRelocGetLoadedPointerProvenance(sprite->bitmap, &asset_id, &bitmap_offset)` (`src/port/sprite_preview_backend.c:603`), which is a
range test of a pointer against the loaded-file table (`src/port/reloc_backend_assets.c:6187`,
`src/port/reloc_backend_assets.c:6126`): no byte of the file is read. It then calls `ndsNativeWallpaperDraw`
(`src/nds/nds_native_wallpaper.c:298`), keyed on `(asset_id, bitmap_offset)` in the generated `kNDSNativeWallpapers` table, whose
`ndsNativeWallpaperUpload` (`src/nds/nds_native_wallpaper.c:191`) streams `nitro:/wallpapers/native_wallpaper_<stage>.bin` row by row
into BG2 VRAM.

Draw, route 2: the direct owner. Every present, `ndsRendererAdapterPrepareNativeStageOwnerBody` calls
`ndsNativeBattleWallpaperDraw(gkind, eye, at)` (`src/port/renderer_adapter_stage.c:4525`, `src/nds/nds_native_wallpaper.c:407`), which uses only
the static `sBattleWallpaperBindings[9]` (`src/nds/nds_native_wallpaper.c:56`) and the camera. It never touches the resident
file. (Dream Land's row is `NONE`: there the SObj route is the only wallpaper owner.) `ndsNativeBattleWallpaperPreload`
(`src/nds/nds_native_wallpaper.c:365`, called from `src/port/taskman_seam_battle_host.c:415`) pays the NitroFS read at scene setup.

Load-time readers of the container: `ndsRelocNormalizeStageDreamLandSprite` (`src/port/reloc_backend_assets.c:9968`,
gated by `ndsRelocIsStageWallpaperAsset`, `src/port/reloc_backend_assets.c:9943`) rewrites the Sprite header lanes and the 44-entry
Bitmap table at `0x26C88`/`sprite->bitmap`: 776 B. The blanket word byte-swap of the whole payload also touches every
byte once, at load.

Conclusion: 158,152 B of pixels are never read after load; 776 B are read at load and by identity. The direct
owner makes route 1 redundant on eight stages, but route 1 is what draws Dream Land, so any change must keep route 1
working with a provenance-capable Sprite/Bitmap (which is why the recommendation keeps the 776 B).

### 4.2 The native stage packet path (Gfx, Vtx, textures)

The path is `ndsRendererAdapterPrepareNativeStageOwner` -> `ndsRendererAdapterCommitNativeStageDisplay`
(`src/port/renderer_adapter_stage.c:4872`). What it reads from the loaded asset files:

- **Assets by identity.** Admission looks each packet asset up by id and requires
  `loaded->data_size == packet asset size` (`src/port/renderer_adapter_stage.c:4585`); `asset_bases[i] = loaded->data`.
- **DL pointers by address only.** `ndsRendererAdapterCollectNativeStageDObjs` reads each DObj's `dl_link` table
  entries (`list_id`, `dl`; `src/port/renderer_adapter_stage.c:3639`), the topology stamp compares `dl_link[k].dl` with the collected
  value (`src/port/renderer_adapter_stage.c:3836`), and validation compares each `dl` with `asset_base + root_offset`
  (`src/nds/nds_renderer_native_owners.c:839`). No Gfx word is dereferenced. The packet carries the state commands
  (`StateDelta`, `scripts/stages/generate_nds_native_stage.py:2307`) and the dense vertices, so nothing re-reads Gfx or Vtx.
- **Texture identity by address, texel bytes only on a cache miss.** `ndsRendererNativeApplyStateDelta` rebuilds
  `stats->texture_image = asset_bases[delta->asset_index] + offset` (`src/nds/nds_renderer_native_owners.c:1271`,
  `NDS_TASK26_IMAGE` `src/nds/nds_renderer_native_owners.c:2046`). Dream Land's textures resolve to pinned entries of the P1 static
  corpus (`ndsRendererHardwareBuildBattleStaticTextureKey`, `src/nds/nds_renderer_textures_effects.c:5984`: "This builds identity only; upload
  reads the offline DS payload"). The blob stages have no corpus: `ndsNativeStageBlobLoad` sets
  `gNdsNativeStageWarmUploads = 1` (`src/nds/nds_native_stage_blob.c:174`); the first prepare uploads through the live path
  (`src/nds/nds_renderer_native_owners.c:1699`), which reads N64 texels from RAM (`ndsRendererResolveTextureDataPointer`,
  `src/nds/nds_renderer_textures_effects.c:13553` for texels, `src/nds/nds_renderer_textures_effects.c:13670` for palettes), marks the entry `stage_warm` (`src/nds/nds_renderer_native_owners.c:1759`), which
  eviction skips for the scene (`src/nds/nds_renderer_textures_effects.c:2881`), and clears the flag after the first accepted prepare
  (`src/nds/nds_renderer_native_owners.c:4657`). So a blob stage's packet textures are read **once** at first use and pinned; a texture the
  packet does not reference, or one uploaded per frame by a material animation, stays a live reader.
- **The decline path is the only Gfx/Vtx reader of packet-covered lists.** If `CommitNativeStageDisplay` returns FALSE
  (owner not admitted, or a segment commit failed) the display proc runs
  (`native_stage_handled == FALSE`, `src/port/opening_movie_backend.c:1799`) and `gcDrawDObjTreeForGObj` reaches
  `ndsStageGCDrawAllLoopRecordDObjDraw` (`src/port/reloc_backend_movement.c:15606`) -> `ndsStageGCDrawAllLoopScanDObjs` (`src/port/reloc_backend_movement.c:15206`) ->
  `ndsRendererAdapterSubmitStageDObjNode` -> `ndsRendererAdapterSubmitStageDL`, the 28,868-byte generic interpreter
  (linked in the shell ELF at `0x02099160`). Pruning packet-only Gfx/Vtx turns that path from "slow but visible"
  (the 2026-09-06 Jungle note at `src/port/renderer_adapter_matrix.c:1697` measured 8-12 FPS) into "cannot draw". Under
  AGENTS.md's no-fallback rule a decline is already a failure; T2 is only safe if that is enforced.
- **Jungle's asset 0.** `pk.assets` for Jungle is `[107, 108, 158, 261]`; bindings, epochs, materials and every
  state delta index asset 1 (bank 108) only (`lane1_readers.py`, `T4`). Bank 107 is loaded because the comment at
  `src/port/renderer_adapter_matrix.c:1697` says Jungle's lists "reach it through segment pointers". The only unrelocated
  pointer words in any stage Gfx are `G_DL 0x0E00xxxx` calls into the runtime-generated material segment E
  (`tools/lane1_segment_scan.py`, all 21 typed files: 50 such words, every one a `G_DL` with segment byte 0x0E, none into
  another bank), and Jungle's typed source and ground code contain no
  `gsSPSegment`. UNPROVEN at runtime; strong static evidence that the requirement is a descriptor leftover.

### 4.3 The generic walker: who still uses it

From the shell ELF's own call graph (`objdump -d`, callers of `ndsRendererAdapterSubmitStageDObjNode`/`SubmitStageDL`/
`SubmitEffectDObjTree`): `ndsStageGCDrawAllLoopRecordDObjDraw`, `ndsStageGCDrawAllLoopScanDObjs`,
`ndsResultsEmblemRecordCapturedDisplay`, and `ndsRendererAdapterSubmitEffectDObjTree`. Stage-file consumers among
them are (a) the decline path above for the layer/map GObjs the packet owns, and (b) effect trees. The stage actor
submitters `ndsStageGCDrawAllLoopSubmit{EfLakitu,EfBronto,TaruCann,YosterCloud}DObjForCamera` call **native
executors** (`ndsRendererAdapterSubmitNative*`), not the walker. `ndsStageGCDrawAllLoopSubmitGroundActorDObj` (the
generic actor commit for Zebes acid / Sector Arwing / Saffron gate / Inishie scales, `src/port/reloc_backend_movement.c:15110`)
is not a separate symbol in the shell ELF (inlined or dead); Zebes acid, Saffron gate and the Inishie scales are packet
owners (`stage_actors` rows in the descriptors), and the Arwing is drawn by a separate native owner (its converted
textures live in a baked packet, see the comment at `src/port/reloc_backend_assets.c:8959`), not by the stage packet. No stage draws its layers through the
walker while its packet is admitted.

### 4.4 The kept classes

| class | read after load | reader |
|---|---|---|
| header + attribute area (map file) | yes, every frame / at spawn | camera bounds `gmCameraSetBoundsPosition` (`decomp/BattleShip-main/decomp/src/gm/gmcamera.c:81-101`), BGM and light angle (`mpCollisionInitGroundData` shim), item weights (`decomp/BattleShip-main/decomp/src/it/itmanager.c:548`), weapon/item attributes on creation |
| collision (`MPGeometryData` and arrays) | yes | gameplay `mpcollision.c`; the port reads the source arrays in place and adds a lazily filled float memo (`ndsMPVertexF32Get`, `src/port/reloc_backend_mp_collision.c:607`; geometry bound at `src/port/reloc_backend_mp_collision.c:593`), so each vertex's bytes are read on first use |
| anim scripts, AnimJoint/MatAnim tables | yes, every frame | `gcPlayAnimAll` (`decomp/BattleShip-main/decomp/src/sys/objanim.c:1428`), `gcParseDObjAnimJoint` (`decomp/BattleShip-main/decomp/src/sys/objanim.c:268`) |
| MObjSub + sprite/palette pointer arrays | struct copied at creation; `sprites[]`/`palettes[]` arrays read per frame | `gcAddMObjAll` (`decomp/BattleShip-main/decomp/src/sys/objanim.c:2429`), material prepare |
| DObjDesc arrays | at DObj setup (packet-owned arrays) and again at every ground-effect spawn (non-owned ones) | `gcSetupCustomDObjs` (`decomp/BattleShip-main/decomp/src/sys/objanim.c:2332`), `efground.c` |
| DObjDLLink tables | yes (topology collect / stamp) | `src/port/renderer_adapter_stage.c:3639`, `src/port/renderer_adapter_stage.c:3836` |
| non-packet Gfx/Vtx (ground effects such as Bronto and Lakitu, Sector rocket/ship, the Arwing, Saffron monsters) | counted as read: native actor executors bake some of it, the source walker is the transactional fallback | `efground.c`; `artifacts/performance/2026-09-26_p2-2p8-phase2-efground/README.md` ("source fallback remains transactional") |
| non-packet / material textures | yes | `ndsRendererResolveTextureDataPointer` |
| padding | no | - |

### 4.5 Non-battle readers of the same files (each loads its own copy)

`ndsBase*` callers of `mpCollisionInitGroundData` in the shell ELF: `SCVSBattleStartBattle`,
`SCVSBattleStartSuddenDeath`, `sc1PGameFuncStart`, `sc1PBonusStageFuncStart`, `sc1PTrainingModeFuncStart`,
`scAutoDemoFuncStart`, `scExplainFuncStart`, and the eight `ndsBaseMVOpening{Donkey,Fox,Kirby,Link,Mario,Pikachu,
Samus,Yoshi}FuncStart`. Each runs the same shim inside its own scene and its own taskman arena, which rewinds at scene
exit, so no scene shares another's copy (`src/nds/nds_native_stage_blob.c` header comment). None reads the wallpaper pixels:
battle-flagged scenes use the two routes of 4.1; the opening movies' `ndsDrawSObjPreview` (`src/port/sprite_preview_backend.c:642`)
records a native failure and reads no pixels. The stage-select screen loads none of these files: the original
`mnMapsFuncStart` is compiled out under `NDS_P2_MENU_SHELL` (`src/import/battleship_mnmaps.c:301`) and `mnMapsStartScene`
(`src/nds/nds_menu_shell_router.c:855`) draws pre-baked UI-kit preview surfaces. The opening room and the Yoshi movie load the
wallpaper containers themselves (`decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningroom.c:619`, `decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningyoster.c:140`), and the VS Results screen uses its
own `MNVSResults` wallpaper (asset 0x22). 1P and Training share the nine ground kinds (Training also
`GRWallpaperTraining*`, separate 133,040 B files). Changing the nine map files therefore changes every one of those
scenes, uniformly and harmlessly for T1 (none reads the pixels), which is the reason to build the compact map for
all callers rather than gate it by scene.

### 4.6 Reader buckets (A/L/F)

A = never read after load (wallpaper pixels, padding, packet-only Gfx/Vtx, Dream Land static-corpus textures).
L = read at load or first use only (Sprite/Bitmap headers, packet-owned DObjDesc, the packet's own textures on the
blob stages). F = read during frames or at spawn (everything in 4.4 marked yes). F is an upper bound: non-packet
Gfx/Vtx are counted F on the strength of the ground-effect fallback, not on a measured read.

**Table 3. Reader buckets and reclaim (bytes).**

| stage | A: never read after load | L: read at load / first use only | F: read during frames | T1 wallpaper | T2 packet-only Gfx+Vtx | T3 static-corpus textures | T4 phantom bank | T1..T4 | tree alloc after T1 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 167,114 | 4,944 | 4,822 | 158,144 | 7,744 | 0 | 0 | 165,888 | 18,736 |
| Sector Z | 173,308 | 16,692 | 36,192 | 158,144 | 13,168 | 0 | 0 | 171,312 | 68,048 |
| Kongo Jungle | 170,228 | 32,208 | 22,956 | 158,144 | 10,132 | 0 | 27,792 | 196,068 | 67,248 |
| Planet Zebes | 168,108 | 12,272 | 39,492 | 158,144 | 8,308 | 0 | 0 | 166,452 | 61,728 |
| Hyrule Castle | 171,146 | 14,020 | 754 | 158,144 | 11,500 | 0 | 0 | 169,644 | 27,776 |
| Yoshi's Island | 168,146 | 22,528 | 38,606 | 158,144 | 8,404 | 0 | 0 | 166,548 | 71,136 |
| Dream Land | 184,668 | 3,636 | 14,512 | 158,144 | 9,828 | 14,072 | 0 | 182,044 | 44,672 |
| Saffron City | 175,326 | 13,848 | 50,426 | 158,144 | 14,704 | 0 | 0 | 172,848 | 81,456 |
| Mushroom Kingdom | 168,182 | 13,488 | 10,554 | 158,144 | 8,552 | 0 | 0 | 166,696 | 34,080 |

## 5. Q4: is a build-time compact ground file feasible?

### 5.1 What the precedents actually do

**Fighter compact packs (FPC1/FPC2)** `scripts/fighters/generate_preview_core_packs.py` (`scripts/fighters/generate_preview_core_packs.py:119`,
`scripts/fighters/generate_preview_core_packs.py:423`), format `include/nds/nds_preview_pack.h` (`include/nds/nds_preview_pack.h:49`), loader
`src/port/reloc_preview_pack.c`. They do **not** keep offsets valid. Main bytes are kept as an identity tiling at
their original offsets, but the Model section is the *concatenation of kept spans* (remapped), described by a span
table `{source_offset, data_offset, bytes}` (12 B each, resident in `NDSPreviewResident.spans`,
`src/port/reloc_preview_pack.c:28`); **every pointer is rebuilt from an explicit fixup table** (`{slot, target}` pairs applied at
load and then discarded; kept targets -> their new section address, a pruned display-list target -> an 8-byte root
cell `{G_ENDDL, original root offset}`, any other pruned target -> NULL); and every consumer that names a source
offset goes through a translator: `ndsPreviewFileOffset` (`src/port/reloc_preview_pack.c:112`), `ndsRelocNativeAssetAddress`
(`src/port/reloc_preview_pack.c:198`), `ndsRelocNativeRootAddress` (`src/port/reloc_preview_pack.c:165`),
`ndsRelocNativeForeignImageAddress` (`src/port/reloc_preview_pack.c:220`). The loaded file is marked
(`loaded->reserved[0]`), and pointer provenance of a compact file is a *compact* offset, so anything keyed on source
offsets (static texture records, wallpaper table rows) must map. "Offsets stay valid" therefore means "the API still
takes source offsets and translates". The address of a pruned list is preserved as identity (the root cell), never
its bytes.

**IFCommonGameStatus compaction** (`ndsRelocLoadIfGameStatusCompact`, `src/port/reloc_backend_assets.c:15916`; receipt
`artifacts/performance/2026-09-23_p2-2p8-if-gamestatus-compact/README.md`; default ON, `Makefile` line
212) is the closer match, because it is the same shape as the wallpaper: a 152,288 B resident
file of which the DS reads only Sprite/Bitmap headers and a few pixel blocks. It runs at load time with no build-time
pack: finalize the file (byte swap, fixups, normalizers) in unowned space at the top of the heap, record every
internal slot the fixup walk patches, drop the payload spans, copy the kept spans to a right-sized allocation,
re-seat every recorded slot (a pointer into a dropped span becomes NULL: 39 of them, all letter `Bitmap.buf`), and map
source offsets through a span table (`ndsRelocIfCompactMap`, `src/port/reloc_backend_assets.c:15768`; `ndsRelocIfBuildSpans`,
`src/port/reloc_backend_assets.c:15791`). Measured: 152,288 -> 21,056 B resident, heap low-water +68,392 B, OBJ VRAM byte-identical,
replay digest IDENTICAL over 1,972 frames. Note the ordering it enforces: **normalizers run before the copy**, because
the blanket 32-bit word swap leaves u8/s16 fields in the wrong lanes and only the normalizers fix them.

**Owned-dependency hook** already in the stage tree loader: `ndsRelocNativeEntryOwnsDependency`
(`src/port/reloc_backend_assets.c:8937`) makes a dependency neither loaded nor counted
(`ndsRelocExternTreeAllocSize` `src/port/reloc_backend_assets.c:11602`), and `ndsRelocResolveNativeEntryExternalFixup`
(`src/port/reloc_backend_assets.c:8950`, used by `ndsRelocApplyExternalPointerFixups`, `src/port/reloc_backend_assets.c:8979`)
resolves the slots that pointed into it (FoxSpecial3 -> ExternDataBank109 resolves to NULL "so accidental raw
consumption fails closed"; the ShieldPose hook `src/port/reloc_backend_assets.c:9056` does the same for fighters). It skips a whole
file; it cannot keep 776 of its bytes.

### 5.2 Applicability to stage files

- **Tail truncation is not available anywhere.** The wallpaper container keeps its tail (Bitmap table 0x269C8, Sprite
  0x26C88, end 0x26CD0) and drops its head; a head-drop would preserve relative offsets but not the offsets the
  native wallpaper table (`src_offset 0x269C8`), the normalizer (`NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE 0x26C88`)
  and the map's external fixup (`target 0x26C88`) all use. So the container needs a remap. In the geometry banks the
  droppable classes are interleaved with kept structures at fine granularity (Dream Land's 104 alone alternates Vtx
  runs, Gfx runs, DObjDesc, collision, scripts), so T2/T3 need the full FPC machinery: many spans, a fixup table,
  root cells, translator seams.
- **T1 needs far less than FPC** because the dropped bytes have no incoming pointers except 44 `Bitmap.buf` slots
  that nothing dereferences: no fixup table, no root cells, one 776 B kept span. The cleanest form is to skip the
  container as a dependency and give the map file its own Sprite/Bitmap stub (section 8). Because the extern list is
  what `lbRelocGetFileSize` walks, the allocation shrinks without touching the loader.
- **Seams that a compact stage bank must pass**, each verified above to compare against source offsets or sizes:
  `renderer_adapter_stage.c` `src/port/renderer_adapter_stage.c:4585` (`data_size ==` packet size; needs `ndsRelocNativeSourceSize`),
  `nds_renderer_native_owners.c` `src/nds/nds_renderer_native_owners.c:839` (address compare needs the root translator),
  `src/nds/nds_renderer_native_owners.c:1271` and `NDS_TASK26_IMAGE` `src/nds/nds_renderer_native_owners.c:2046` (`base + offset` needs
  `ndsRelocNativeAssetAddress`), `ndsRendererHardwareBuildBattleStaticTextureKey` `src/nds/nds_renderer_textures_effects.c:5984` (already
  translated, and rejects omitted spans), the texel/TLUT resolvers `src/nds/nds_renderer_textures_effects.c:13553`/`src/nds/nds_renderer_textures_effects.c:13670`, provenance-keyed
  tables (wallpaper, static corpus), and the normalizers (`src/port/reloc_backend_assets.c:9731`, `src/port/reloc_backend_assets.c:9968`).

## 6. The per-stage table

Resident bytes are the tree allocation plus any extra preload (Jungle); A/L/F and T1..T4 are Table 3; the reader for
each bucket is section 4; "confidence" is per tier; every tier is UNPROVEN at runtime (section 7).

**Table 5. Per-stage result.**

| stage | resident files (ids) | resident B | A never read | L load/first use | F read in frames | T1 | T2 | T3 | T4 | confidence |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| Peach's Castle | 259, 106, 90, 156 | 176,880 | 167,114 | 4,944 | 4,822 | 158,144 | 7,744 | 0 | 0 | T1 high; T2 medium |
| Sector Z | 262, 109, 99, 153, 161 | 226,192 | 173,308 | 16,692 | 36,192 | 158,144 | 13,168 | 0 | 0 | T1 high; T2 medium (FoxSpecial3 12,160 B kept: no packet owner) |
| Kongo Jungle | 261, 108, 92, 158 (+107 preload) | 253,184 | 170,228 | 32,208 | 22,956 | 158,144 | 10,132 | 0 | 27,792 | T1 high; T2 medium; T4 medium (heap vs overlay UNPROVEN) |
| Planet Zebes | 257, 105, 89, 157 | 219,872 | 168,108 | 12,272 | 39,492 | 158,144 | 8,308 | 0 | 0 | T1 high; T2 medium |
| Hyrule Castle | 265, 113, 95 | 185,920 | 171,146 | 14,020 | 754 | 158,144 | 11,500 | 0 | 0 | T1 high; T2 medium (100% of its Gfx/Vtx is packet-only) |
| Yoshi's Island | 263, 111, 110, 93, 154 | 229,280 | 168,146 | 22,528 | 38,606 | 158,144 | 8,404 | 0 | 0 | T1 high; T2 medium |
| Dream Land | 255, 104, 103, 88, 152 | 202,816 | 184,668 | 3,636 | 14,512 | 158,144 | 9,828 | 14,072 | 0 | T1 high (frozen P1 pins); T2 medium; T3 low-medium (corpus engagement UNPROVEN) |
| Saffron City | 264, 112, 94, 160, 159 | 239,600 | 175,326 | 13,848 | 50,426 | 158,144 | 14,704 | 0 | 0 | T1 high; T2 medium |
| Mushroom Kingdom | 260, 107, 91, 155 | 192,224 | 168,182 | 13,488 | 10,554 | 158,144 | 8,552 | 0 | 0 | T1 high; T2 medium |

## 7. Assumptions and what is UNPROVEN

Assumptions.

1. "Shipping" is the `smash64ds-p2-shell-hwtri` line; the ELF oracle is `builds/build-ovl1p/smash64ds-p2-shell-hwtri.elf`
   and blob/`.gxp` sizes come from `builds/build-p2p8-s1/nitrofs/stages/`. Both exist already; I built nothing.
2. All nine `NDS_P2_STAGE_*` flags are on, so every extern token resolves (`ndsRelocAssetIDForToken`). The Dream Land
   equality with the census is the only direct check of the tree arithmetic; the other eight use the same code path.
3. The tree is a taskman-heap block. The Jungle preload goes overlay-first.
4. Typed sources are the US branch; the O2R payloads are US (file ids match `reloc_data.us.h`).
5. The packet is the shipping one: I regenerate it with the checked-in descriptors (`generate()` passes
   `validate_packet`), not the packet inside a particular ROM.

UNPROVEN (nothing here was run on an emulator or device):

- T1 unread: proven by static tracing of every wallpaper consumer and the ELF call graph, not by a run. Confirm with
  the same-ROM A/B (heap low-water +158,144 B, BG2 VRAM identical, `gNdsNativeWallpaperLoadCount`,
  `gNdsSObjWallpaperCacheFastDrawCount` engaged, `gNdsNativeBattleWallpaperFailureCount` 0).
- T1 for Dream Land specifically depends on route 1 (SObj) staying alive; the stub keeps it alive by construction.
- T2: exclusivity is over relocation slots. A read through a computed offset (an `ll` symbol) straight into a Gfx/Vtx
  span would be invisible to it. DObjDesc/DLLink are the only entry points to display lists in these files
  (G_DL/G_VTX targets are relocated), so I expect none, but no reader of effect descriptors was audited line by line.
  The decline path (4.2) is the accepted risk.
- T3: that the P1 static corpus fully engages on every Dream Land texture key (the renderer records a miss as a
  texture fence, not a crash), and that the frozen P1 stage may change at all.
- T4: that bank 107 is unneeded by Jungle, and which region (overlay loan or heap) it lands in.
- Class totals: `anim` also holds `u32` blocks named `gap_...` whose content I did not decode individually (small);
  `other16` (40 to 180 B/stage) are `u16` blocks that are neither 16-colour palettes nor collision; typed tiling
  cannot distinguish a script from other u32 data.
- Blob/`.gxp` sizes are from a lab build; the generators are deterministic but I did not regenerate them.

## 8. Recommended implementation outline

**Step 1 (T1, do this): compact ground map, 158,144 B per stage.** Proven feasible offline:
`tools/lane1_variant_maps.py` writes the nine variants (`variants/GR<Stage>Map`), re-parses each with
`generate_nds_native_stage.load_o2r`, and checks external fixups -1, internal +2, wallpaper id gone from the extern
table, tree allocation down by exactly 158,144 B (`lane1_variant_maps.json`). Transform per map file: append the
container's Bitmap[44] (704 B) and Sprite (72 B) at a 16-byte boundary (+784 B with padding), zero every `Bitmap.buf`,
turn the `wallpaper` slot (header+0x48) and `Sprite.bitmap` (+0x34) into internal fixups, drop the wallpaper id from
the extern table and its slot from the external chain.

Files a change touches:

- new `scripts/stages/generate_compact_ground_maps.py` (reuse `load_o2r` and this lane's `build_variant`);
- `Makefile`: stage the nine `reloc/reloc_stages/GR<Stage>Map` NitroFS files from the generator instead of the
  pattern rule at line 7865 (`$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%`); the wallpaper containers
  stay staged for the opening movies;
- `scripts/stages/generate_native_wallpapers.py` (`SOURCES`, `scripts/stages/generate_native_wallpapers.py:91`) and the generated
  `src/nds/generated/native_wallpapers.generated.inc`: rows keyed by the **map's** asset id and the stub Bitmap offset
  added beside the existing `(0x100xx, 0x269C8)` rows (the opening movies still load the full container), since
  provenance now resolves into the map file;
- `src/nds/nds_native_wallpaper.c` `sBattleWallpaperBindings` (`src/nds/nds_native_wallpaper.c:56`): same re-keying for the
  direct owner;
- `src/port/reloc_backend_assets.c`: `ndsRelocIsStageWallpaperAsset` (`src/port/reloc_backend_assets.c:9943`) and
  `ndsRelocNormalizeStageDreamLandSprite` (`src/port/reloc_backend_assets.c:9968`) must also match the nine map asset ids at
  their stub Sprite offsets (table-driven instead of the single constant `0x26C88`); `ndsRelocNormalizeGroundMapAsset`
  is unchanged (header stays at 0x14);
- bookkeeping that names the wallpaper asset: the Dream Land reloc diagnostic sets `NDS_STAGE_PUPUPU_RELOC_PASS` only
  when the five-bit asset mask is full, and bit 1 is the wallpaper container (`src/port/reloc_backend_assets.c:9240`, documented at
  `docs/DIAGNOSTIC_REFERENCE.md:584`); the memory-ledger stage classification `ndsRelocAssetIsStage` (`src/port/reloc_backend_assets.c:5482`) lists the
  Castle and Dream Land wallpaper containers. Both are diagnostics only; update the expectations rather than keep a
  dead file loaded;
- no change to `mpCollisionInitGroundData` or to the tree loader: the smaller extern list shrinks
  `lbRelocGetFileSize` by itself;
- packet-side, the one edit outside the stage loader: the native stage owner requires `loaded->data_size ==
  adapter asset size` for every packet asset (`src/port/renderer_adapter_stage.c:4585`), and the map is a packet asset, so the nine
  `adapter_asset_sizes` map entries (`scripts/stages/native_stage_descriptors/<stage>.py`, e.g. Sector `0x130`)
  become the variant sizes (+0x310), the C rows in `src/port/renderer_adapter_matrix.c` are re-emitted by
  `scripts/stages/emit_native_stage_runtime_rows.py` (`scripts/stages/emit_native_stage_runtime_rows.py:192`, `--check`), and
  `scripts/stages/check_nds_native_stage.py` (which pins them) is updated. `payload_checksum` is emitted but never
  read at runtime. **Dream Land is the frozen P1 packet** (`dreamland.py`, the golden `DREAMLAND_ADAPTER_ASSET_SIZES`
  pin at `scripts/stages/check_nds_native_stage.py:153`), so its entry needs owner sign-off; a variant that leaves the map size alone (owned-dependency hook plus a
  provenance-capable stub, below) avoids touching that pin.

Checks: replay digest `--sequence` against the same ROM with the flag off, a BG2 VRAM capture per stage (must be
byte-identical), and the counters in section 7. Alternatives: (B') IF-style runtime compaction
(`ndsRelocLoadIfGameStatusCompact` pattern: top-of-heap window, normalizers first, one kept span, slot re-seat; keeps
the 159 KB SD read at every battle start and needs no descriptor change); (B'') owned-dependency hook plus a static
stub (fewest files and no NitroFS change, but a static stub has no provenance, so route 1 fails unless the stub is
registered as a loaded file, and route 1 must be suppressed on the eight stages whose direct owner already draws).

**Step 2 (T4, cheap): drop Jungle's phantom bank.** `scripts/stages/native_stage_descriptors/jungle.py` (the
`stage_images` block at line 59, `asset_order` at 137,
`adapter_asset_ids/sizes`, `include_sha`; rows re-emitted by `emit_native_stage_runtime_rows.py`), regenerate `src/nds/nds_native_stage_jungle.generated.inc`, the blob, the
`.gxp` and the maxima header, update the stale comment at `src/port/renderer_adapter_matrix.c:1697`. Check `gNdsFrontendOverlayUsedBytes`
first: if bank 107 sits in the overlay loan the heap does not move.

**Step 3 (T2): packet-only Gfx/Vtx to root cells, 7.7 to 14.7 KB per stage.** New generator
(`scripts/stages/generate_compact_stage_banks.py`) producing FPC-style sections/spans/fixups/root cells for the packet
asset banks; loader hook in `ndsRelocLoadExternTreeAsset` (`src/port/reloc_backend_assets.c:11802`); the seams of 5.2. Only after
declines are confirmed to be hard failures.

**Step 4 (T3/T5): textures.** Dream Land (T3): identity cells for the 40 static-corpus records; needs owner sign-off
(frozen P1). Other stages (T5): bake a per-stage static pin set the way `generate_battle_playable_static_textures.py`
does for Dream Land; then the packet textures become unread (3.3 to 29.8 KB per stage, Table 4).

**Coupling worth knowing.** The stage GX template only loads if `body + 62,020 B` (25,600 + 36,420) of heap remain
(`src/nds/nds_stage_gx.exec.inc:8`); the 2026-09-28 stage-heap-cliff receipt measured Jungle's body declined (28,064 B, STG 270K ticks
versus 204K when it loaded). Freeing 158 KB of ground tree raises that headroom before it lowers anything else.

## 9. Reproduce

All in `tools/` (Python 3, no build, no emulator, `PYTHONDONTWRITEBYTECODE=1` recommended):

```
python lane1_o2r.py                 # index of the 2,132 O2R files; lane1_o2r.tree()/tree_alloc()
python lane1_validate_tiling.py     # 21/21 typed files, 2,717 decls, 2,469 slots  -> ../lane1_tiling_validation.json
python lane1_run_all.py             # walker vs tiling per stage
python lane1_report_data.py         # every number in this report               -> ../lane1_data.json
python lane1_render_tables.py > ../lane1_tables.md   # Tables 0-5 + appendix
python lane1_variant_maps.py        # nine compact maps, re-parsed               -> ../variants/, ../lane1_variant_maps.json
python lane1_segment_scan.py        # 50 unrelocated pointer words, all G_DL segment E -> ../lane1_segment_scan.json
python lane1_cite_lines.py          # every file:line above                      -> ../lane1_citations.json
python lane1_build_report.py        # assembles this document from the template
```

Modules: `lane1_o2r.py` (index/tree), `lane1_typed.py` + `lane1_tiling.py` (typed tiling), `lane1_classify.py`
(walker), `lane1_static_corpus.py` (P1 corpus records), `lane1_readers.py` (packet + reader attribution).
`lane1_dev_run.py` is the scratch driver used while building the walker.

## Appendix: per-file class bytes

| stage | file | role | payload | Gfx (pkt-only) | Vtx (pkt-only) | texels | TLUT | DObjDesc+DLLink | anim | MObjSub+ptr | coll | hdr+attr | pad+o16 | wallpaper pix / sprite |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Peach's Castle | 259 GRCastleMap | map | 192 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 188 | 4 | 0 / 0 |
| Peach's Castle | 106 ExternDataBank106 | typed | 17,696 | 5,144 (4,640) | 4,192 (4,000) | 4,840 | 416 | 1,144 | 1,044 | 0 | 530 | 0 | 386 | 0 / 0 |
| Peach's Castle | 90 MVOpeningRoomWallpaper | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Peach's Castle | 156 MiscDataBank156 | typed | 64 | 0 (0) | 0 (0) | 0 | 0 | 0 | 52 | 0 | 0 | 0 | 12 | 0 / 0 |
| Sector Z | 262 GRSectorMap | map | 304 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 304 | 0 | 0 / 0 |
| Sector Z | 109 ExternDataBank109 | typed | 47,120 | 6,512 (5,448) | 9,360 (9,136) | 15,980 | 640 | 1,712 | 11,840 | 128 | 520 | 0 | 428 | 0 / 0 |
| Sector Z | 99 StageSector | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Sector Z | 153 MiscDataBank153 | typed | 7,680 | 216 (0) | 96 (0) | 0 | 0 | 0 | 7,368 | 0 | 0 | 0 | 0 | 0 / 0 |
| Sector Z | 161 FoxSpecial3 | typed | 12,160 | 3,088 (0) | 3,520 (0) | 3,840 | 512 | 700 | 268 | 0 | 0 | 0 | 232 | 0 / 0 |
| Kongo Jungle | 261 GRJungleMap | map | 224 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 224 | 0 | 0 / 0 |
| Kongo Jungle | 108 ExternDataBank108 | typed | 62,944 | 6,880 (6,640) | 5,136 (5,072) | 36,836 | 800 | 1,804 | 10,544 | 152 | 408 | 0 | 384 | 0 / 0 |
| Kongo Jungle | 92 StageJungle | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Kongo Jungle | 158 MiscDataBank158 | typed | 3,296 | 392 (0) | 224 (0) | 2,048 | 32 | 132 | 448 | 0 | 0 | 0 | 20 | 0 / 0 |
| Planet Zebes | 257 GRZebesMap | map | 224 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 224 | 0 | 0 / 0 |
| Planet Zebes | 105 ExternDataBank105 | typed | 57,184 | 5,168 (4,256) | 5,120 (4,896) | 15,268 | 1,192 | 1,940 | 24,968 | 2,640 | 396 | 0 | 492 | 0 / 0 |
| Planet Zebes | 89 StageZebes | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Planet Zebes | 157 MiscDataBank157 | typed | 3,536 | 288 (288) | 128 (128) | 2,048 | 128 | 148 | 572 | 148 | 0 | 0 | 76 | 0 / 0 |
| Hyrule Castle | 265 GRHyruleMap | map | 224 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 224 | 0 | 0 / 0 |
| Hyrule Castle | 113 ExternDataBank113 | typed | 26,768 | 6,264 (6,264) | 6,496 (6,496) | 11,904 | 416 | 980 | 0 | 0 | 406 | 0 | 302 | 0 / 0 |
| Hyrule Castle | 95 StageCastle | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Yoshi's Island | 263 GRYosterMap | map | 192 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 188 | 4 | 0 / 0 |
| Yoshi's Island | 111 ExternDataBank111 | typed | 47,408 | 6,128 (4,688) | 5,216 (4,832) | 13,928 | 576 | 2,200 | 17,404 | 1,144 | 494 | 0 | 318 | 0 / 0 |
| Yoshi's Island | 110 ExternDataBank110 | typed | 21,040 | 0 (0) | 0 (0) | 20,536 | 320 | 0 | 0 | 0 | 0 | 0 | 184 | 0 / 0 |
| Yoshi's Island | 93 StageYoshi | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Yoshi's Island | 154 MiscDataBank154 | typed | 1,712 | 488 (0) | 64 (0) | 512 | 0 | 220 | 272 | 136 | 0 | 0 | 20 | 0 / 0 |
| Dream Land | 255 GRPupupuMap | map | 192 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 188 | 4 | 0 / 0 |
| Dream Land | 104 ExternDataBank104 | typed | 17,392 | 5,144 (4,664) | 4,128 (4,000) | 2,132 | 128 | 1,804 | 3,052 | 444 | 488 | 0 | 72 | 0 / 0 |
| Dream Land | 103 ExternDataBank103 | typed | 12,224 | 0 (0) | 0 (0) | 9,344 | 2,576 | 0 | 0 | 0 | 0 | 0 | 304 | 0 / 0 |
| Dream Land | 88 StageDreamLand | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Dream Land | 152 MiscDataBank152 | typed | 14,080 | 2,216 (2,216) | 752 (752) | 3,640 | 160 | 1,320 | 5,564 | 316 | 0 | 0 | 112 | 0 / 0 |
| Saffron City | 264 GRYamabukiMap | map | 832 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 832 | 0 | 0 / 0 |
| Saffron City | 112 ExternDataBank112 | typed | 66,160 | 8,584 (7,624) | 7,120 (6,864) | 38,388 | 1,472 | 1,656 | 6,860 | 820 | 554 | 0 | 706 | 0 / 0 |
| Saffron City | 94 StagePokemon | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Saffron City | 160 MiscDataBank160 | typed | 2,704 | 1,424 (1,168) | 720 (720) | 0 | 0 | 328 | 208 | 0 | 0 | 0 | 24 | 0 / 0 |
| Saffron City | 159 MiscDataBank159 | typed | 10,976 | 1,600 (0) | 448 (0) | 6,168 | 256 | 816 | 1,248 | 288 | 0 | 0 | 152 | 0 / 0 |
| Mushroom Kingdom | 260 GRInishieMap | map | 368 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 368 | 0 | 0 / 0 |
| Mushroom Kingdom | 107 ExternDataBank107 | typed | 27,792 | 5,072 (5,072) | 3,904 (3,904) | 13,728 | 288 | 1,224 | 2,256 | 552 | 530 | 0 | 238 | 0 / 0 |
| Mushroom Kingdom | 91 StageHyruleWallpaper | wallpaper | 158,928 | 0 (0) | 0 (0) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 158,152 / 776 |
| Mushroom Kingdom | 155 MiscDataBank155 | typed | 5,136 | 1,288 (440) | 1,024 (384) | 1,264 | 0 | 528 | 812 | 152 | 0 | 0 | 68 | 0 / 0 |
