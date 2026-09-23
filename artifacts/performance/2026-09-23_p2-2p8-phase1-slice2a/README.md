# P2-2p8 Phase 1 slice 2a: VRAM census and a lossless battle VRAM plan

Scope: measurement and a proposal only, per `phase1-slice2a-brief.md`. Implementation belongs to 2b.

Setup:
- ROM `CEC2D792...` (`final-census-a.json`), tree `de8bbadec2e` plus the lab-only census below.
- Four-CPU match: DK slot 0, Samus 1, Link 2, Kirby 3.
- Sampler on runner slot 9 / GDB 3423, 1,972 samples from frame 2, `-RingDump`.

**Route 0 digest:** identical to slice 1's `final-route0-rows.csv` on all 1,972 frames in three runs:
- census off (`final-route0-off`)
- census on (`final-census-a`)
- the key-recording run (`census-b4`)

Files: `digest-slice1-final-route0-vs-*.json`. Native failures are 573 / 254, the same as before.

## Headline

1. **Texture VRAM is full for the whole match.** Banks A+B hold 262,144 B.
   - Use is 246,944-261,600 B from before GO to the end, a peak of 99.8%.
   - The largest free contiguous run ranges from 8,800 B at GO down to 384 B.
   - **Fighters own only 40,576 B at GO (16%).** The rest belongs to scene-lifetime residents:
     - battle static 66,432 B
     - IFCommon GO/GAME SET/countdown atlases 57,344 B
     - particle atlas 42,368 B
     - entry-effect textures kept after GO 29,632 B
     - halo+shield 8,192 B
2. **Link's AppearL failure is fragmentation on top of fullness.** At frame 156 Link asks for a Pal16 64x128 (4,096 B). There are 5,216 B free, but they are split into 4 runs, the largest 2,176 B. Slice 1 said "VRAM full"; the exact cause is "no contiguous 4 KB".
3. **The P95 episode (frames 798-1043) is eviction churn in a fragmented cache.**
   - At frame 798 there are 15,200 B free in **15 runs**, the largest 2,560 B, so no texture larger than 2.5 KB can be placed without evicting another.
   - Link's resident textures fall from 15.5 KB to 5.6 KB and Kirby's to 512 B across ring stops 7-9.
   - The fighters re-upload every frame (408 uploads after GO, only 60 distinct keys), and every upload moves the fence.
4. **BG3 (bank D, 128 KB) carries nothing in a VS battle.**
   - 0 bytes are written by clear, copy or final-write after the battle window resets.
   - 0 opaque pixels at GO, frame 156, 900 and 1500.
   - It is still enabled: DISPCNT has BG3 on and the layer mask is 3.
   - Mapping D as texture slot 3 in VS battles adds **131,072 B of texture VRAM, losslessly**.
5. **Correction to the brief's KNOWN list: the battle wallpaper is not a 16-colour source.**
   - Every battle wallpaper is a 300x220 **RGBA16** sprite (`scripts/stages/generate_native_wallpapers.py` asserts RGBA16).
   - The baked 240x176 images have 210 to 2,735 distinct colours.
   - An 8bpp or tiled BG2 would be lossy on 8 of 9 stages. Only inishie (210 colours) fits 8bpp.
   - Only the Results wallpaper is 16-level (I4 source, 13 levels used).
   - **BG2 keeps its 16-bit bank C.**
6. **The four-kind reachable set, enumerated without drawing, is 55,584 B for this roster and 57,056 B for the heaviest roster.** Both figures are texels plus palettes. The heaviest roster is DK+Samus+Link+Ness.
   - Every runtime-recorded key is in the enumerated set (0 misses).
   - DK matches exactly (14 keys, 11,776 B). Samus and Link are strict supersets.
   - Kirby matches except for a key-representation quirk (below).
7. **The plan fits with margin, losslessly.** With D as texture slot 3 there are 393,216 B:
   - stress roster, peak before GO: 287,392 B (26.9% margin)
   - heaviest roster plus the worst weapon reserve: 334,624 B (14.9% margin)
   - Palettes (F+G, 32 KB) peak at 6,144 B today and about 9 KB planned. They are not binding.

## 1. VRAM census

### Instrument (lab-only, `NDS_TICK_HUD`; no GX/VRAM writes)

**Tag per libnds texture name.** Each name carries the site that created or uploaded it (the caller PC of the fenced wrapper), the fighter draw slot active at the upload, and the profile owner.
- Hooks: `src/nds/nds_renderer_preamble.c:4574-4678` in the Fenced GenTextures/TexImage2D/DeleteTextures wrappers, plus the tag reset after `glResetTextures` at `src/nds/nds_renderer_textures_effects.c:3144`.
- The IFCommon atlas wrappers record their caller at `nds_renderer_textures_effects.c:3480/3493/3513`, so an atlas texture is attributed to the function that asked for it.

**Census walk.** `src/nds/nds_renderer_textures_effects.c:4128-4712` reads libnds's own tables:
- `glGlobalData.texturePtrs` / `palettePtrs` give TEXIMAGE_PARAM sizes and palette sizes.
- The two `s_vramBlock` allocators give the memory-order block list, clipped to the banks VRAMCNT maps as texture / texture palette.

It buckets every live name as:
- cache entry: static / stage_warm / fighter slot 0-3 / stage / other
- entry effect: startup-only / kept
- tint tile
- halo+shield
- otherwise by creating site (host-mapped with nm)

**Capture points.**
- `gNdsVramCensusEnable=1` walks at every frame end (`src/port/renderer_fighter_lean.c:555`).
- Snapshots are latched at GO, at the first fighter texture reject (`src/nds/nds_renderer_native_common.c:11539`), and at the minimum-largest-free-run frame of 798-1043 and of 1044 onward.
- Match peaks are kept per bucket.

**BG pixel counts.** `ndsPlatformVramCensusOverlayOpaque` at `src/nds/nds_platform.c:1101-1135` counts opaque pixels. The draw slot is set around the fighter draw at `src/port/renderer_adapter_fighter.c:5294-5314`.

**Checks.**
- `tex_size_field == tex_bytes == tex_alloc_bytes` in every snapshot, so libnds `texSize` is in bytes, every allocated block is a live name, and there is no hidden allocation.
- `pal_alloc_bytes == pal_bytes` likewise.
- 0 name-table overflow, 0 walk-guard hits.

**Allocator facts measured here.**
- libnds's texture allocator spans the LCD range of **A-D** (`0x06800000-0x06880000`) and its palette allocator spans **E-G**.
- It allocates only inside banks whose VRAMCNT mode is texture (A=0x83 slot 0, B=0x8B slot 1; F/G texture palettes). The raw free minus the usable free is exactly C+D's 262,144 B at every snapshot.
- Mapping D as a texture slot needs no allocator change.

### Texture VRAM (A+B = 262,144 B usable)

| | GO (f196) | entry burst (f156, first reject) | episode worst (f798) | after worst (f1165) |
|---|---:|---:|---:|---:|
| used B (names) | 248,704 (163) | 256,928 (186) | 246,944 (160) | 260,960 (181) |
| free B | 13,440 | 5,216 | 15,200 | 1,184 |
| free runs | 7 | 4 | **15** | 5 |
| largest free run B | 8,800 | **2,176** | **2,560** | **384** |
| pending request | - | **Pal16 64x128 = 4,096 B** | - | - |

Match peak used is 261,600 B. The minimum largest free run is 384 B (f1165).

### By owner class (texel bytes / names; palette bytes in `final-census-a-report.txt`)

| class | GO | burst f156 | episode f798 | after f1165 | match peak |
|---|---:|---:|---:|---:|---:|
| battle static (cache static entries) | 66,432 / 44 | 66,432 / 44 | 66,432 / 44 | 66,432 / 44 | 66,432 |
| IFCommon clouds + traffic (READY/GO/TIME UP/GAME SET letters, countdown lights) | 57,344 / 3 | 57,344 / 3 | 57,344 / 3 | 57,344 / 3 | 57,344 |
| particle atlas | 42,368 / 8 | 42,368 / 8 | 42,368 / 8 | 42,368 / 8 | 42,368 |
| entry-effect textures kept after GO | 29,632 / 24 | 29,632 / 24 | 29,632 / 24 | 29,632 / 24 | 29,632 |
| entry-effect textures startup-only (released at GO) | 0 | **18,528 / 41** | 0 | 0 | 18,528 |
| fighter slot 0 DK | 18,176 / 14 | 18,176 / 14 | 18,176 / 14 | 21,120 / 19 | 21,120 |
| fighter slot 1 Samus | 8,896 / 15 | 8,896 / 15 | 8,896 / 15 | 8,896 / 15 | 9,920 |
| fighter slot 2 Link | 12,992 / 24 | 5,568 / 14 | 5,568 / 14 | 15,616 / 28 | 15,616 |
| fighter slot 3 Kirby | 512 / 1 | 0 | 512 / 1 | 1,536 / 3 | 2,560 |
| rebirth halo + entry shield | 8,192 / 10 | 8,192 / 10 | 8,192 / 10 | 8,192 / 10 | 8,192 |
| shield quads (`ndsEFManagerShieldQuadProcDisplay`) | 0 | 0 | 3,072 / 3 | 3,072 / 3 | 4,096 |
| damage slash | 0 | 0 | 1,536 / 2 | 1,536 / 2 | 1,536 |
| scene textures + host update + stage DL + cache stage | 3,584 / 10 | 1,280 / 5 | 4,608 / 11 | 4,608 / 11 | ~6,100 |
| fighter tint tiles | 256 / 8 | 192 / 6 | 288 / 9 | 288 / 9 | 288 |
| Fox gun + no-texture object | 320 / 2 | 320 / 2 | 320 / 2 | 320 / 2 | 320 |

Fighter bytes here are VRAM allocations. They can exceed the key's need, because an entry refreshed in place keeps its larger allocation.

**Timeline** (ring stops, `final-census-a.json` `ringStopReads`):
- Used stays between 251,008 and 261,536 B.
- The free run is at most 6,496 B, at f224.
- During frames 800-992: 15-16 free runs, and Link is squeezed to 5,568 B and Kirby to 512 B.

### Palette VRAM (F+G = 32,768 B usable)

| | GO | burst | episode | after | peak |
|---|---:|---:|---:|---:|---:|
| used B (palettes) | 4,718 (171) | 5,540 (193) | 5,300 (175) | 5,860 (204) | 6,144 |
| free / runs / largest | 28,050 / 78 / 26,384 | 27,228 / 66 / 26,476 | 27,468 / 72 / 26,192 | 26,908 / 87 / 26,128 | |

Palettes are fragmented into small holes, but one 26 KB run always remains. They are not binding.

### BG2 / BG3 in battle

| layer | bank | what it carries in this VS battle | bytes written | opaque px (of 49,152) at GO / f156 / f900 / f1500 |
|---|---|---|---|---|
| BG2 (affine Bmp16 256x256) | C 128 KB | battle wallpaper, 240x176 RGB555, camera-driven affine | 98,304 once at setup (the commit); **0 per frame** after | 49,152 / 49,152 / 49,152 / 49,152 |
| BG3 (Bmp16 256x256, priority 0) | D 128 KB | **nothing** | **0** (clear 0, copy 0, final 0) | **0 / 0 / 0 / 0** |

Which scenes need 16-bit there:
- **BG2/BG3 elsewhere.** Both are the UI kit's 16-bit layers (`src/nds/nds_ui_kit.c`, used by the menu shells, router and harness screens: N64 sprites drawn at 16 bits). Menus, CSS and 1P keep today's layout.
- **Battle BG2** needs 16 bits because of the wallpapers' colour counts (`wallpaper-colours.txt`):

  | wallpaper | colours |
  |---|---:|
  | pupupu | 2,735 |
  | castle | 1,947 |
  | zebes | 1,603 |
  | yoster | 1,087 |
  | yamabuki | 819 |
  | hyrule | 812 |
  | sector | 658 |
  | jungle | 339 |
  | inishie | 210 |

  Unique 8x8 tiles are 309-660 of 660, so a tiled BG would not deduplicate either.
- **Battle BG3** needs nothing.
- **1P battles were not measured.** The wallpaper preload itself is VS-only (`gkind <= 8`, `src/port/taskman_seam_battle_host.c:393`), so the remap below is scoped to the same condition.

## 2. Reachable fighter texture set per kind (enumerated without drawing)

**Method** (`tools/reach5.py`, with helpers `reach.py`, `reach2.py`, `reach3.py`; host only).

**Input.** The relocData closure each fighter's creation loads, from the pack estimator's index `estimate_fighter_pack.index_closure`: payload bytes, pointer map and typed objects.

**Simulation.** For each detail it walks the fighter's own part tables as `ftparam.c` does:
- `FTCommonPartContainer.commonparts[detail].dobjdesc[j].dl`. LOW falls back to HIGH per joint when the LOW dl is NULL, which is the rule at `ftparam.c:788`.
- The matching `p_mobjsubs[j]`.
- `FTModelPartDesc.modelparts[mp][detail]` for every model part, including Entry/Appear states and Kirby's hats.

It interprets each display list:
- SETTIMG / SETTILE / LOADBLOCK / LOADTILE / LOADTLUT / SETTILESIZE / TEXTURE / DL / ENDDL.
- At every material segment call (`G_DL 0x0E000000+8i`) it applies MObj i's branch as `objdisplay.c` builds it:
  - PALETTE: SETTIMG(palette), plus LOADTLUT when SPLIT or ALPHA is set
  - FRAC|SPLIT: next sprite
  - FRAC|ALPHA: current sprite
- It does this for **every sprite** (texture animation) and **every palette** (costume / palette animation).

**DS conversion.** Each textured triangle gives (image, tlut, DS size) using the port's rules, ported from `nds_renderer_textures_effects.c` 11925-12070 and 5085-5130:
- load extent
- masked-clamp materialisation
- clamped-window period
- wrap period
- next pow2, 128 max

The DS format is Pal16 when the texture has at most 16 colours (CI4 always), otherwise direct 16-bit.

**Other files.** Display lists in the kind's other files that the part tables never reach are reported as "other files": weapons, items, the entry Arwing, and Yoshi's item file.

**Cross-check.** Runtime keys come from `census-b4`: every fighter-slot upload is recorded with its image and TLUT, and the provenance is mapped back to relocData offsets through the battle-core pack spans (`tools/probe_pack.py`).

| kind | LOW per instance (texel B / palette B / textures) | LOW, all palettes | HIGH per instance | other-file textures (B / n) | runtime keys (B) | runtime keys outside LOW enumeration |
|---|---:|---:|---:|---:|---:|---:|
| Mario | 5,120 / 192 / 6 | 9,216 / 448 / 14 | 5,120 / 192 / 6 | 1,472 / 5 | - | - |
| Fox | 3,872 / 256 / 8 | 6,944 / 448 / 14 | 3,872 / 256 / 8 | 44,896 / 50 | - | - |
| Donkey | 11,776 / 448 / 14 | 56,832 / 2,112 / 66 | 9,216 / 480 / 15 | 1,792 / 4 | 14 (11,776) | 0 |
| Samus | 11,008 / 768 / 24 | 50,688 / 2,944 / 92 | 11,008 / 800 / 25 | 2,304 / 3 | 19 (9,728) | 0 |
| Luigi | 6,144 / 224 / 7 | 14,336 / 576 / 18 | 4,096 / 160 / 5 | 1,216 / 5 | - | - |
| Link | 22,208 / 1,216 / 38 | 23,744 / 1,312 / 41 | 21,984 / 1,216 / 38 | 14,144 / 9 | 27 (15,488) | 0 |
| Yoshi | 1,184 / 320 / 10 | 16,320 / 4,224 / 132 | 160 / 64 / 2 | 48,128 / 64 | - | - |
| Captain | 5,984 / 544 / 17 | 35,648 / 3,008 / 94 | 6,208 / 640 / 20 | 10,880 / 15 | - | - |
| Kirby | 2,272 / 256 / 8 | 11,904 / 1,088 / 34 | 10,528 / 928 / 29 | 7,232 / 16 | 6 (3,072) | 4 (see below) |
| Pikachu | 5,376 / 416 / 13 | 26,880 / 2,080 / 65 | 5,376 / 416 / 13 | 8,832 / 8 | - | - |
| Purin | 2,816 / 224 / 7 | 14,080 / 1,120 / 35 | 1,792 / 160 / 5 | 11,776 / 6 | - | - |
| Ness | 9,120 / 512 / 16 | 12,480 / 896 / 28 | 9,056 / 448 / 14 | 7,168 / 5 | - | - |

**Kirby's copy hats** (hat-only textures, LOW) are added only for opponents present:

| copied kind | texel B | palette B | textures |
|---|---:|---:|---:|
| Samus | 1,920 | 128 | 4 |
| DK | 1,152 | 96 | 3 |
| Pikachu | 864 | 128 | 4 |
| Mario | 512 | 32 | 1 |
| Luigi | 512 | 32 | 1 |
| Captain | 416 | 96 | 3 |
| Link | 128 | 32 | 1 |
| Ness | 32 | 32 | 1 |
| Fox, Yoshi, Purin | 0 | 0 | 0 |

The joint-6 model parts come from `229_KirbyMain.c` `modelparts_desc_0x0CC`, and the copy table from `228_KirbyMainMotion.c` `FTKirbyCopy`.

**Completeness.**
- **Source-complete.** Every image and palette any runtime key used lies inside the enumerated closure. The walk covers both details, every model part (Entry/Appear states included), every MObj sprite (texture animation) and every palette (costume and palette animation).
- **DK is exact.** The enumerated LOW set equals the recorded set byte for byte (14 keys, 11,776 B).
- **Samus and Link are strict supersets** (24 vs 19 and 38 vs 27 textures). The extras are states this match never entered.
- **Kirby:** 4 of its 6 runtime keys record the *palette* pointer as the image (a palette-animated body material: image = palette k, TLUT = palette 0). The enumerator produces the same DS texture as (sprite `0x1CF60`, palette k). This is a key-representation difference, not a missing texture. The runtime used 5 palettes of this material in one match, so Kirby's budget below counts all of them (+4 x 544 B).
- **Where it is not exact:**
  - The per-instance column counts one palette per material (the instance's costume). Palette-animated materials (Kirby's body) need the all-palettes figure; the only one observed is Kirby's.
  - The 8 kinds outside this roster are host-derived and unvalidated at runtime. This slice may build only the default roster, and the roster is a make variable (`NDS_P2_FOUR_CPU_KIND0..3`).
  - "Other files" are counted, not attributed to a detail.
  - Entry-effect props are their own class (the entry-effect generator), not part of this set.

**Budgets, per instance.** The Kirby rows include its 5-palette body and the hats of the opponents present:
- **Stress roster** DK+Samus+Link+Kirby: **55,584 B** (texels 52,512 + palettes 3,072); other-file textures 25,472 B.
- **Heaviest four-kind body roster:** DK+Samus+Link+Ness, **57,056 B** (texels 54,112 + palettes 2,944).

The heaviest other-file reserve among weapon-bearing kinds, excluding Fox's Arwing (an entry prop) and Yoshi's shared item file (items class), is Link+Purin+Captain+Pikachu: 45,632 B.

## 3. Battle VRAM plan (lossless only)

**Change.** In VS battles only (`gkind <= 8`, the same gate as the wallpaper preload):
- Hide BG3 and map **bank D to texture slot 3**. This is `vramSetBankD(VRAM_D_TEXTURE_SLOT3)`: slot 3, because libnds computes a texture's offset from its LCD address, which is A/B/C/D = slots 0-3.
- BG2 stays in bank C.
- Texture VRAM becomes A+B+D = **393,216 B**.

**Where and cost.**
- At battle setup, next to the existing overlay enable and wallpaper preload (`src/port/taskman_seam_battle_host.c:393`): one VRAMCNT write and one `bgHide`.
- At battle exit: remap D to main BG `0x06020000`, then DMA-clear 128 KB before the next scene's BG3 use. That is about 1 ms at a scene transition.
- Menus, CSS and 1P never see it.
- Nothing is lossy: BG3 is empty in VS battles.

**Budget per class (texture VRAM).**

| class | bytes | lifetime |
|---|---:|---|
| battle static (this stage) | 66,432 | scene |
| IFCommon clouds + traffic | 57,344 | scene |
| particle atlas | 42,368 | scene |
| entry-effect textures kept | 29,632 | scene |
| halo + shield | 8,192 | scene |
| scene / host / Fox gun / no-texture | 3,904 | scene |
| **scene subtotal** | **207,872** | |
| fighters, stress roster (admitted, pinned) | 52,512 | match |
| fighters, heaviest roster | 54,112 | match |
| entry burst (startup-only entry props) | 18,528 | setup until GO |
| dynamic (shield quads, damage slash, stage dynamic, tint tiles; measured peaks) | 8,480 | per frame |
| weapons / other-file reserve, stress roster / heaviest | 25,472 / 45,632 | on use |

**Fit.**

| case | used | of 393,216 | margin |
|---|---:|---:|---:|
| stress roster, peak before GO (scene + fighters + burst + dynamic) | 287,392 | 73.1% | 105,824 B (26.9%) |
| stress roster after GO (+ weapon reserve, burst freed) | 294,336 | 74.9% | 98,880 B (25.1%) |
| heaviest body roster + worst weapon reserve + burst + dynamic | 334,624 | 85.1% | 58,592 B (14.9%) |
| today, A+B only: scene + stress fighters alone | 260,384 | 99.3% of 262,144 | no room for burst or dynamic |

**Fixed regions instead of first-fit** (fragmentation is the measured failure mode):
- **Resident region, A+B.** Allocate scene-lifetime textures first at setup, then admit the fighters, all pinned and never freed in the match. With D locked while they allocate, they pack contiguously:
  - scene without the IFCommon atlases: 150,528 B
  - fighters: 52,512-54,112 B
  - total 203,040-204,640 B, which leaves 57,504-59,104 B
- **Transient region, D.** Unlock D and lock A+B for the rest of the match. D holds:
  - the IFCommon atlases (57,344 B)
  - the entry burst (18,528 B, freed at GO)
  - dynamic textures and weapons (8,480 + 25,472 B)

  That totals 109,824 of 131,072 B (83.8%). Only this region allocates and frees during the match, and the cache's LRU only ever finds evictable entries here.
- **Mechanism.** libnds's own `glLockVRAMBank` / `glUnlockVRAMBank` ("prevent consideration of the bank when allocating", `videoGL.h:506`). No allocator of our own is needed.
- **Heaviest roster.** Move the particle atlas (42,368 B) to D instead, and put the IFCommon atlases in A+B, if D would overflow.

**Palette budget (F+G, 32,768 B).** Scene palettes are 3,700 B at GO. Add the entry-burst palettes (1,216 B), fighter admission (3,072 B for the stress roster, 2,944 B for the heaviest) and dynamic (~1,000 B): about 9,000 B, with 72% free. No change is needed. Palettes stay first-fit; their largest free run never dropped below 26 KB.

**Other lossless levers (not needed with D; for the record).**
- Release the IFCommon clouds after GO and re-upload them before GAME SET/TIME UP: 57,344 B, one upload at match end.
- Release entry-kept textures no gameplay root of the present kinds references.

Nothing lossy is proposed, and none is needed.

## 4. 2b recommendation

### Admission

1. **Host-generated admission lists.**
   - Promote `tools/reach5.py` into `scripts/fighters/`, next to the owner generator, to emit a generated table: per kind x detail, the DS texture list, each entry being image and TLUT source offsets, tile descriptor, DS size/format and palette index.
   - Add palette-animated materials with all their palettes (Kirby's body), and a Kirby hat list per copied kind.
   - Why host-side rather than a creation-time C walk: the DS size depends on display-list tile state, which the host already has to interpret. A C walk would duplicate the texture half of the renderer. This matches the spec's host-generated direction (D2).
2. **Creation-time upload before GO**, at the `battleship_ftmanager.c` owner-image seam (spec 2.8, `EnsureOwnerImages`):
   - For each listed texture of the present kinds (their costume palette, the Kirby hats of opponents present), build the same `NDSRendererHardwareTextureKey` the draw builds.
   - Run the cache's existing convert+upload half into an entry marked pinned-admitted.
   - Order: scene set, then fighters, then (D unlocked) the transient classes.
   - Any failure latches (kind, key) in `gNdsFtrLeanAdmitFail`.
3. **A/B in one ROM with runtime words.** `gNdsFtrLeanAdmit`:
   - 0 = today
   - 1 = admit+pin in A+B
   - 2 = admit+pin + D remap + region locks

   Arms:
   - (1) prices admission alone; it will overflow A+B the way the slice 1 stand-in did.
   - (2) is the plan.
   - (0) vs (2) is the gate.

   The sampler pokes at the first battle frame, after setup. So word 2 performs the remap and the admission at the first frame end after the poke, which is still roughly 150 frames before the entry burst. D holds nothing then (BG3 is empty), so remapping mid-scene is invisible.
4. **Gates, whole four-CPU match, slice 2a instruments kept.**
   - Native failures 0, including the entry frames 150-200.
   - `fighter_uploads_after_go` = 0.
   - Admit failures 0.
   - Key-recorder keys not in the admitted list = 0 (new counter).
   - MTEX over 798-1043 near 0 (the episode gone).
   - Route 0 digest identical.
   - Report WORK-H P50/P95.
   - **Before trusting the other 8 kinds' lists,** run the same key-recorder oracle on rosters that cover them. That needs the coordinator's leave to build `NDS_P2_FOUR_CPU_KIND0..3` variants.

### Lean BSS diet

Sizes are from this ELF (`arm-none-eabi-nm -S`). Sizing goes to real root counts: the selected-roots high-water is 21 on this roster (`gNdsRendererAdapterOwnerSelectedRootsHighWater`), and the max per kind should come from the generator.

| symbol | today | proposed | saves |
|---|---:|---:|---:|
| `sNdsFtrLeanWorlds` (lean.c, 32 x Q20.12) | 2,048 | 24 x 64 = 1,536 (or generator max) | 512 |
| `sNdsFtrLeanInputs` (32 x 52) | 1,664 | 24 x 52 = 1,248 | 416 |
| `sNdsFtrLeanInstance` (root arrays 32) | 1,764 | root arrays 24 | ~260 |
| `sNdsFtrLeanStats` (NDSRendererStats) | 1,300 | share the adapter's existing stats scratch | 1,300 |
| `gNdsFtrLean` counters (oracle / census / witness / Phase 0) | 788 | lab-only under `NDS_TICK_HUD`; shipping keeps the route/admit words only | 788 |
| packet `tint_binds[16]` x 4 packets | 1,024 | 4 binds per packet (observed 2) | 768 |
| kernel `sNdsFtrLeanWorlds` (40 x 64) | 2,560 | keep 40 (native joint max) unless the generator gives less | 0 |
| slice 2a census: tags 1,024 + `gNdsVramCensus` 6,044 + maps 576 | lab only | already compiled out without `NDS_TICK_HUD` | - |

That comes to about 4.0 KB saved in the shipping image.

**Heap in this lab build.** `gNdsTaskmanGeneralHeapFreeMin` is 74,772 B, against 87,060 B in slice 1. About 7.2 KB of that is the lab census BSS. It is still 49 KB above the 25,600 B GObj-cap latch.

## Runs and files

**Runs.**
- `final-census-a`: census on, snapshots, peaks, per-stop timeline, BG. Report: `final-census-a-report.txt`.
- `census-b4`: fighter keys, always-on recorder. Decoded: `census-b4-keys.txt` / `.json`.
- `final-route0-off`: census off, the digest control.
- Earlier bring-up arms, kept for their logs:
  - `census-a`, `census-a2`: before usable-bank clipping and atlas-user attribution
  - `census-b2`: DS size read too early
  - `census-b3`

**Host outputs.**
- `reachable-final.json`: per-kind LOW/HIGH rows.
- `reachable-tables.md`
- `enumerated-banks.json`: source texel/palette banks by estimator disposition.
- `wallpaper-colours.txt`

**Tools** (`tools/`):
- `build-s2a.ps1`, `run-s2a.ps1` (holds the build lock; runner slot 9 / GDB 3423)
- `census_report.py`, `keys_report.py`
- `reach*.py`, `final_tables.py`, `probe_pack.py`, `enumerate_textures.py`

**Rules.**
- No subagents, no git commands.
- Code is lab-only (`NDS_TICK_HUD`), confined to the slice 1 files plus `nds_platform.c`.
- Nothing under `decomp/` or generated outputs.
- Builds were only the permitted target, under the lock; runs only on slot 9.
