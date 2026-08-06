# Fighter Render Miscoloring — Fresh Investigation

**Date:** 2026-04-11
**Branch:** `agent/fighter-render`
**Worktree:** `C:\Users\Jack Rickey\dev\new_attempt\ssb64-fighter-render`

Investigation into fighter rendering bugs in all intro scenes (Mario, Kirby, Samus, etc.): non-textured regions miscoloured, textured regions corrupted. The user explicitly asked for a fresh look that disregards the previous round of memory files, so the analysis below was re-derived from code and git history.

---

## 1. Pipeline summary

### 1.1 Torch extraction (`torch/src/factories/ssb64/RelocFactory.cpp`)

Fighters live in the reloc-file table; Torch treats every reloc entry as an opaque blob. `RelocFactory::parse` (lines 119-208):

1. Reads the 12-byte `LBTableEntry` from `RELOC_TABLE_ROM_ADDR + file_id * 12` (BE fields: data offset + compressed-size-words + extern-reloc-word-offset + decompressed-size-words).
2. Decompresses with `vpk0_decode` if the high bit of `dataOffset` is set, otherwise memcpys.
3. Reads the big-endian `u16[]` extern-file-id list sitting between this file's compressed data and the next file's `dataOffset`.
4. Serializes as `{u32 file_id, u16 reloc_intern, u16 reloc_extern, u32 num_extern_ids, u16[] extern_ids, u32 data_size, u8[data_size] raw_bytes}` into `.o2r`.

**No fighter-specific handling.** Torch has zero knowledge of FTCommonPart, MObjSub, FTMotionDesc, figatree format — it's all a byte blob. Any interpretation is the port's job.

There is **no** `yamls/us/*fighter*.yml` — the fighter-common and fighter-main files are covered by the blanket reloc-table extraction driven by `config.yml → RELOC_FILE_COUNT` + `RelocFactory`. Torch's `.hash.yml` for fighters doesn't exist either.

Relevant reloc-table entries from `port/resource/RelocFileTable.cpp`:
- `reloc_fighters_main/MarioMain` (file_id 203), `MarioMainMotion` (202), `MarioSpecial1` (204), …
- `reloc_fighters_common/FTEmblemSprites` (20), `FTStocksZako` (25), `FTManagerCommon` (163)
- `reloc_animations/FTMarioAnim*` (500-ish…)
- `reloc_submotions/FT*`

All share the same factory + binary layout.

### 1.2 Archive → runtime load (`port/bridge/lbreloc_bridge.cpp::lbRelocLoadAndRelocFile`)

Lines 301-497, per-file pipeline:
1. Resolve the `RelocFile` via LUS `ResourceManager::LoadResource(gRelocFileTable[file_id])`.
2. `memcpy(ram_dst, relocFile->Data.data(), copySize)` — raw N64 BE bytes into the game heap.
3. Optional `SSB64_DUMP_FILE_ID` hook dumps the pre-swap blob for diffing against ROM.
4. `portRelocByteSwapBlob(ram_dst, copySize, file_id)` — two-pass BE→LE conversion (see §1.3).
5. Walk the internal reloc chain: for each 4-byte slot in the chain, compute an intra-file target pointer, call `portRelocRegisterPointer(target)` → u32 token, overwrite the slot with the token. Each slot's preceding command is also inspected by `portRelocFixupTextureFromChain` for chain-walk texture / vertex byte-order patches.
6. Walk the external reloc chain the same way but resolve against the cached `status_buffer` for the extern dependency file (load-on-demand if missing).
7. **Figatree-only** postpass: `portRelocIsFighterFigatreeFile(file_id)` pattern-matches `reloc_animations/FT*` and `reloc_submotions/FT*` paths. If this is a figatree file, a bitmap of "which words got relocated" is recorded during chain walks, and then `portRelocFixupFighterFigatree` runs rotate16 on every non-reloc word in the whole file. Rationale: figatrees are streams of `AObjEvent16` (two 16-bit commands per u32 word), and pass1's blanket `BSWAP32` leaves the two u16s in swapped positions; rotate16 restores them. Reloc-chain slots are skipped because they already hold native-endian token u32s after the chain walk.

### 1.3 Byte-swap passes (`port/bridge/lbreloc_byteswap.cpp`)

Central function `portRelocByteSwapBlob` (lines 699-718):

- **Pass 1** (`pass1_swap_u32`, lines 432-441): Blanket `BSWAP32` of every u32 word in the blob. Correct for u32 and f32 fields and for GBI display-list commands (which are encoded as pairs of native BE u32 words by definition). Incorrect for any sub-u32 field (u16, u8, u8u8 pairs) and for any multi-byte texture / vertex data that must stay in BE byte order for Fast3D's per-byte channel readers.
- **Pass 2** (`scan_display_lists` + `apply_fixups`, lines 447-693): Walks the now-LE display-list commands looking for `G_VTX` / `G_SETTIMG` + `G_LOADBLOCK` / `G_LOADTLUT` pairs whose `w1` already has segment=0x0E (intra-file). For each such pair it records a `FixupRegion` and applies one of `FIXUP_VERTEX` (per-Vtx rotate16 of the three s16 pair words + bswap32 of the RGBA word), `FIXUP_TEX_BYTES` (bswap32 of 4b/8b texture bytes), or `FIXUP_TEX_U16` (bswap32 of 16b texture / palette bytes). Pass 2 **only catches slots where the cmd's w1 already encodes seg=0x0E at file-load time**; it misses every reference that is stored as a raw "to-be-rewritten" pointer slot in the reloc chain and is filled in later by the chain walker. This is the same coverage hole that motivated the chain-walk fixup.
- **Chain-walk fixup** (`portRelocFixupTextureFromChain`, lines 771-989): called from `lbreloc_bridge.cpp:405` for every internal-reloc slot. If the slot's preceding cmd is `G_SETTIMG`, walks forward up to 8 commands looking for `G_LOADBLOCK`/`G_LOADTLUT`, derives the texture byte size, and BSWAP32s those bytes in place. If the preceding cmd is `G_VTX`, per-Vtx rotate16/bswap32 with strict validation. Idempotent via `sStructU16Fixups`.
- **Runtime texture fixup** (`portRelocFixupTextureAtRuntime`, lines 1025-1087): called from LUS `GfxDpLoadBlock`/`LoadTile`/`LoadTlut` with a resolved texture address + byte count. Applies BSWAP32 to any unfixed bytes that live inside a reloc file. Designed to catch runtime-built DLs (sprites, fighter material LoadBlocks) that pass2 and chain walk never see.
- **Runtime vertex fixup** (`portRelocFixupVertexAtRuntime`, lines 1089-1130): called from LUS `gfx_vtx_handler_f3dex2`. Per-Vtx idempotent rotate16/bswap32 for any Vtx the interpreter actually dispatches from a reloc file.
- **Struct fixups** (lines 1175-1497): `portFixupSprite`, `portFixupBitmap` / `portFixupBitmapArray`, `portFixupSpriteBitmapData`, `portFixupMObjSub`, `portFixupFTAttributes`. All take a pointer to a post-pass1 struct, apply a hand-coded sequence of rotate16 + bswap32 + `fixup_u16_u8u8` (for the `[u16][u8][u8]` MObjSub word) based on the struct's field layout. All idempotent via `sStructU16Fixups`.

### 1.4 Fighter resource layout

The key structs (`src/ft/fttypes.h` / `src/sys/objtypes.h`):

- `FTCommonPart` — 16 bytes on both N64 and port. Layout: `{u32 dobjdesc, u32 p_mobjsubs, u32 p_costume_matanim_joints, u8 flags, 3 bytes padding}`. On the port, pointer fields are u32 reloc tokens and the `u8 pad[3]; u8 flags;` trailer is carefully placed so that after pass1 BSWAP32 of the last u32 word the flags byte ends up at struct offset 0x0F — matching where a LE struct reads it. `_Static_assert(sizeof(FTCommonPart) == 16)`.
- `FTCommonPartContainer::commonparts[2]` — 32 bytes total, stride = 16 = matches N64.
- `FTModelPart` — similar, 20 bytes, same pad-before-flags trick.
- `FTAccessPart` — 16 bytes, no flags byte.
- `FTModelPartContainer::modelparts_desc[]` — u32 token array on PORT.
- `FTMotionDesc` — `{u32 anim_file_id, intptr_t offset, u32 anim_desc.word}`. **On PC this struct is 24 bytes** (u32 + 4 padding + intptr_t + u32 + 4 padding for 8-byte alignment), versus 12 on N64. This is safe **only** because every `FTMotionDesc` array is a compile-time C initializer inside `src/sc/scsubsys/scsubsys*.c` or `src/ft/ftdata.c` (not a reloc-file blob), so stride, layout, and indexing are all self-consistent per ABI. No code does byte-offset arithmetic into these arrays.
- `MObjSub` (objtypes.h:320, 0x78 bytes): contains `{u16 pad00, u8 fmt, u8 siz, u32 sprites-token, u16×4, s32, f32×6, u32 palettes-token, u16 flags, u8 block_fmt, u8 block_siz, u16 block_dxt, u16×3, f32×4, u32, SYColorPack primcolor, u8 prim_l, u8 prim_m, u8[2] pad, SYColorPack envcolor, blendcolor, light1color, light2color, s32×4}`. This struct is **heavily** u16/u8-packed and absolutely requires `portFixupMObjSub` to be readable post-pass1.
- `SYColorPack` (ssb_types.h:97): union `{ struct { u8 r, g, b, a; }; u32 pack; }`. Post-pass1, `pack` read as u32 is numerically correct (BSWAP32 restores the same numeric value on LE); but `.s.r/g/b/a` reads bytes `[A, B, G, R]` from the wrong in-memory positions.

### 1.5 Fighter rendering path

Two distinct code paths create MObjs for a fighter:

**Path A — `gcSetupCustomDObjsWithMObj` / `gcAddMObjAll`** (`src/sys/objanim.c` lines 2683-2770, 2772-2825):
- Used by scene geometry (OpeningRoom background, boss pieces, items, weapons).
- On PORT, loops `p_mobjsubs` as a u32 token array, resolves each token, calls **`portFixupMObjSub(mobjsub)`** then `gcAddMObjForDObj(dobj, mobjsub)`. The copy `mobj->sub = *mobjsub` at `src/sys/objman.c:1327` therefore captures a properly-fixed MObjSub.

**Path B — `lbCommonAddMObjForFighterPartsDObj`** (`src/lb/lbcommon.c` lines 1012-1095) — the **fighter** path:
- Called from `lbCommonSetupFighterPartsDObjs` (same file, lines 1098-1207) and from `ftParamInitAllParts` / `ftParamSetModelPartID` / `ftParamResetModelPartAll` (`src/ft/ftparam.c` lines 787, 821, 893, 927, 1042, 1072, 1092).
- Calls `gcAddMObjForDObj(dobj, mobjsub)` **directly, without any `portFixupMObjSub` call**.

Drawing: `ftDisplayMainDrawDefault` (src/ft/ftdisplaymain.c:756) → `gcDrawMObjForDObj(dobj, gSYTaskmanDLHeads)` (src/sys/objdisplay.c:1322) → reads `mobj->sub.flags`, `.fmt`, `.siz`, `.block_fmt`, `.block_siz`, `.block_dxt`, `.primcolor.s.r/g/b/a`, `.envcolor.s.r/g/b/a`, `.blendcolor.s.r/g/b/a`, `.light1color.pack`, `.light2color.pack`, `.prim_l`, `.prim_m` and emits `gDPSet*Color`, `gSPLightColor`, `gDPSetTextureImage`, `gDPLoadBlock`, etc. based on `flags & MOBJ_FLAG_*` bits.

### 1.6 Color-combiner / matanim color updates

`gcPlayMObjMatAnim` (src/sys/objanim.c:1382): runs per-frame for each mobj that has an attached matanim track. For color tracks (PrimColor, EnvColor, BlendColor, Light1Color, Light2Color) it overwrites the matching `mobj->sub.<field>` with a `SYColorPack color` local that it builds with BSWAP-independent per-channel bit shifts (commit `25edeb0`, with a fall-through initializer from `a7024aa`). So any material with a costume matanim gets its color field correctly rewritten every frame.

**Note:** matanim only touches the color union fields (primcolor, envcolor, blendcolor, light1color, light2color) and the texture-animation fields (trau/scau/scrollu/etc.). It does **not** touch `flags`, `fmt`, `siz`, `block_fmt`, `block_siz`, `block_dxt`, `unk0C/0E/36/38/3A`, `prim_l`, or `prim_m`.

`ftDisplayLightsDrawReflect` (src/ft/ftdisplaylights.c) emits a pre-fighter `gSPNumLights(1)` + `gSPLight(LIGHT_1, directional white)` + (port-only hack) `gSPLight(LIGHT_2, ambient 0x40)`. Per-material `gSPLightColor(LIGHT_1, mobj->sub.light1color.pack)` in `objdisplay.c:1444` is supposed to override this **only if** `flags & MOBJ_FLAG_LIGHT1` is set.

---

## 2. Hypotheses, ranked

### 2.1 H1 (dominant): fighter MObjSubs are never run through `portFixupMObjSub`, so every byte-granular field in every fighter material is read from the wrong in-memory position

**Evidence:**
1. `portFixupMObjSub` is defined in `port/bridge/lbreloc_byteswap.cpp:1404` and declared in `port/bridge/lbreloc_byteswap.h:109`.
2. Searching the entire codebase (`Grep portFixupMObjSub`) turns up exactly two call sites, both inside `src/sys/objanim.c` at lines 2734 and 2798 — the `gcSetupCustomDObjsWithMObj` and `gcAddMObjAll` PORT branches. These are the **scene-geometry** loaders (OpeningRoom background, items, stages).
3. `src/lb/lbcommon.c` contains no reference to `portFixupMObjSub` at all. `lbCommonAddMObjForFighterPartsDObj` (line 1030), `lbCommonSetupCustomTreeDObjsWithMObj` (line 1263), and `lbCommonAddMObjForTreeDObjs` (line 1313, 1327) all call `gcAddMObjForDObj` on a resolved MObjSub pointer without fixing it first.
4. `gcAddMObjForDObj` (src/sys/objman.c:1307) executes `mobj->sub = *mobjsub` at line 1327 — a plain struct copy that captures whatever byte ordering is in the source MObjSub at that moment.
5. Therefore every fighter MObj ends up with `mobj->sub.*` populated from pass1-only data. Every subsequent byte-granular read of `mobj->sub.*` in `gcDrawMObjForDObj` (objdisplay.c:1322-1720) is against the wrong in-memory byte positions.

**Concrete consequences of reading a pass1-only MObjSub on LE PC:**

For a `[u16 a][u8 b][u8 c]` word like `{flags, block_fmt, block_siz}` at offset 0x30, the N64 BE bytes are `[a_hi, a_lo, b, c]`. After pass1 BSWAP32 they become `[c, b, a_lo, a_hi]`. LE reads then produce:
- `sub.flags` (u16 at 0x30) → `(b << 8) | c` = `(block_fmt << 8) | block_siz`. For a valid `block_fmt ∈ {0..4}` (RGBA/YUV/CI/IA/I) and `block_siz ∈ {0..3}` (4b/8b/16b/32b), this "flags" value is 0x0000..0x0403. The `MOBJ_FLAG_*` bits are 0x01, 0x02, 0x04, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000. So the actual flags observed by `objdisplay.c` are:
  - `MOBJ_FLAG_TEXTURE (1<<7 = 0x80)` — **never** set (`block_siz` never reaches 0x80).
  - `MOBJ_FLAG_PRIMCOLOR (1<<9 = 0x200)` — set iff `block_fmt == 2 (CI)`, 3 (IA) with that bit, etc.
  - `MOBJ_FLAG_ENVCOLOR (1<<10 = 0x400)` — set iff `block_fmt == 4 (I)`.
  - `MOBJ_FLAG_LIGHT1 (1<<12 = 0x1000)` and `MOBJ_FLAG_LIGHT2 (1<<13 = 0x2000)` — **never** set (requires `block_fmt ≥ 0x10`).
  - `MOBJ_FLAG_ALPHA (1<<0 = 0x01)`, `FRAC (1<<4 = 0x10)`, `SPLIT (1<<1 = 0x02)` — only set if `block_siz` has the corresponding low bit.
- `sub.fmt` (u8 at 0x02 in the `[u16 pad00][u8 fmt][u8 siz]` word at 0x00) reads from a BSWAP32'd word whose bytes are `[siz, fmt, pad_lo, pad_hi]`; offset 2 gets `pad_lo` = 0 for a valid file. So `mobj->sub.fmt = 0 (RGBA)` for every fighter material.
- `sub.siz` at 0x03 gets `pad_hi` = 0 similarly → `G_IM_SIZ_4b` for every fighter material.
- `sub.block_fmt` at 0x32 gets `flags_lo` (low byte of the u16 flags). `sub.block_siz` at 0x33 gets `flags_hi`.
- `sub.block_dxt` (u16 at 0x34) — two u16s `[block_dxt, unk36]` position-swapped; `sub.block_dxt` reads the raw unk36 value.
- `sub.primcolor.s.r/g/b/a` reads bytes `[A, B, G, R]` from the pass1-bswapped word — **every RGBA channel is inverted-and-swapped**. This drives `gDPSetPrimColor`, so shaded (untextured or lit) fighter parts get a literal `(alpha, blue, green, red)` instead of `(red, green, blue, alpha)`.
- `sub.envcolor.s.r/g/b/a`, `sub.blendcolor.s.r/g/b/a` — same channel corruption.
- `sub.light1color.pack` / `sub.light2color.pack` read as **u32** are numerically correct — BSWAP32 on LE restores the same numeric value a BE u32 would have. So `gSPLightColor(LIGHT_1, mobj->sub.light1color.pack)` emits the right command — **except** it's gated by the mangled `flags` field, which never sets bits 12/13, so the command is never actually emitted for fighter materials.
- `sub.prim_m` (u8 at 0x55) and `sub.prim_l` (u8 at 0x54) at the `[u8 prim_l, u8 prim_m, u8 pad, u8 pad]` word at 0x54 — after BSWAP32 the bytes are `[pad, pad, prim_m, prim_l]`; offset 0x55 returns `pad` (0), offset 0x54 returns `pad` (0). So `gDPSetPrimColor(..., mobj->sub.prim_m, mobj->lfrac*255, ...)` gets LOD level 0 regardless of what the file specified.

**Impact matching the reported symptoms:**
- **Non-textured areas miscoloured**: primcolor.s.r/g/b/a are byte-reversed → solid-shaded parts of fighters (skin, costume flats) get literal ABGR output. Mario's red hat would look like whatever `(A,B,G,R)` maps to, skin tones come out as teal-ish, etc.
- **Textures where they shouldn't be / missing**: `mobj->sub.fmt = 0, .siz = 0` always → `gDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_4b, ...)` for every material regardless of the real format. A 4bpp CI texture loaded as RGBA 4b reads the wrong bytes per pixel. The `mobj->sub.block_siz` switch at `objdisplay.c:1571` falls into whichever case `flags_hi` encodes, which picks a wrong `gDPLoadBlock` arithmetic.
- **`MOBJ_FLAG_LIGHT1/LIGHT2` never fire** → fighter materials never emit per-material light colors → fighters use the stale directional+ambient left by `ftDisplayLightsDrawReflect`. This is why `fbbcfa6` ("Initialize fighter Light_t color and emit ambient light") seemed to help — it was treating the consequence (stale light DMEM), not the cause (missing flags bit).
- **`MOBJ_FLAG_PRIMCOLOR` fires for CI/IA materials only** → primary colour gets set to the byte-reversed ABGR garbage. Fighters with CI-palette skin (Mario, Kirby, Samus) are especially affected — any CI body part gets a `gDPSetPrimColor` with wrong channels every draw.

This is a single-cause explanation for **both** symptoms ("miscolouring in non-textured areas" = primcolor corruption; "corrupted textures" = wrong fmt/siz/block_*/flags). It also explains why the earlier matanim byte-order fix (`25edeb0`) and fighter-Light_t hack (`fbbcfa6`) only partially helped — those fix *different* symptoms of the same root class (runtime matanim writes, directional light DMEM), but the initial copy and all the static field derivations still come from the wrong bytes.

### 2.2 H2 (secondary, local): `ftparam.c` reads `detail_p_mobjsubs[i]` / `detail_p_costume_matanim_joints[i]` with C array stride (8 bytes on PC) on a u32 token array (4 bytes)

**Evidence:**
- `FTCommonPart.p_mobjsubs` is `u32` on PORT (fttypes.h:185). After `FTPARTS_GET_MOBJSUBS()` resolves it, the resulting pointer is typed as `MObjSub***` and points at an in-file array of u32 tokens.
- `ftparam.c` lines 811, 917, 1062: `mobjsubs = detail_p_mobjsubs[joint_id - nFTPartsJointCommonStart];` — plain C indexing, which on PC uses `sizeof(MObjSub**) = 8`. Reads 8 bytes per index, pulling two u32 tokens into a single `void*`.
- Contrast with `lbcommon.c:1184`: `PORT_RESOLVE_ARRAY(detail_p_mobjsubs, i)` — the correct, 4-byte-stride accessor.
- Same bug for `costume_matanim_joints[i]` on lines 817, 923, 1068.
- Scope: only hit when `FTMODELPARTCONTAINER_GET_DESC(modelparts_container, i) == NULL`, i.e. fighter joints without an override model part. Fires during `ftParamInitAllParts`, `ftParamSetModelPartID`, and `ftParamResetModelPartAll` (which is called from `ftMainSetStatus` on line 4536 — every fighter status change).

**Impact:** When this path runs, `mobjsubs` points into the wrong place entirely (index `i*2` in the token array). The resulting `lbCommonAddMObjForFighterPartsDObj` call walks a completely unrelated MObjSub list — either a different material chain belonging to a different joint, or random garbage. The fighter's default-detail joints get miscoloured *in addition to* the H1 issue, and every pose change can reshuffle which joints get which materials.

This is a local fix, but it can produce weird per-status-change colour changes on top of H1.

### 2.3 H3 (contributing, possibly fixed): figatree fixup walks non-reloc words and applies rotate16 blindly

**Evidence:**
- `portRelocFixupFighterFigatree` (lbreloc_bridge.cpp:133-157) rotates every non-reloc word in any file whose resource path starts with `reloc_animations/FT` or `reloc_submotions/FT`.
- This assumes figatree files only contain `AObjEvent16` streams and intra-chain pointer slots. If any word inside a figatree is a u32/f32/s32 (float joint rotations, s32 payload), rotate16 corrupts it.
- `src/ft/ftanim.c` reads `event16->.u` (u16), `event16->s/2` (s16), and for `nGCAnimEvent16SetTranslateInterp` it reads `*(u32*)event16` as a reloc token (line 437). The reloc-token slot would be correctly skipped by the fixup since `reloc_words[i] == 1`. For the u16/s16 event fields the rotate16 is correct.
- I did **not** find a figatree-interpretation path that reads f32 values out of the same stream, so this is probably benign — but it's speculative because I didn't audit every event16 opcode and some may carry f32 payloads (e.g. joint-angle values). Speculative.

**Impact if wrong:** mangled animation values but not colours or textures. Would explain fighter pose weirdness only, not the colour symptoms. Listing for completeness.

### 2.4 H4 (low): LP64 `FTMotionDesc.intptr_t` widening

`FTMotionDesc` is `{u32, intptr_t, u32}` = 12 bytes on N64, **24 bytes on PC** (4-byte alignment padding + 8-byte `intptr_t` + 4-byte field + 4-byte tail alignment padding).

Every `FTMotionDesc[]` array I found is a compile-time C initializer in `src/ft/ftdata.c` or `src/sc/scsubsys/scsubsys*.c` (e.g. line 138 of ftdata.c: `FTMotionDesc dFTMarioMotionDescs[] = { llFTMarioAnimWaitFileID, 0x80000000, 0x00000000, ... };`). C's brace-elision fills this consistently at `sizeof(FTMotionDesc)`-sized strides, and every reader uses the same C struct type — so the stride is self-consistent per ABI. `motion_desc->offset` compares against `0x80000000` (ftmain.c:4855) which zero-extends cleanly from u32 to 64-bit intptr_t on PC.

**No fighter file contains a raw FTMotionDesc array** that I could find — they're all compile-time. So this isn't an active bug in the current port, but it's a known LP64 landmine that would bite if any motion-desc array got moved into a reloc blob. The CLAUDE.md "LP64 long audit" already flags this class of issue. Very low priority unless proved otherwise.

### 2.5 H5 (already mitigated): the existing partial fixes for fighter rendering are symptomatic, not root-cause

The port has already accumulated several commits aimed at fighter rendering (all on this branch):
- `fbbcfa6` — initialise fighter Light_t color + emit ambient at LIGHT_2. This compensates for the `MOBJ_FLAG_LIGHT1/LIGHT2` branches never firing (H1 consequence). Without it, fighter lights default to whatever the RSP last had.
- `25edeb0` — fix matanim color interpolation byte order. This correctly writes LE channels into `mobj->sub.primcolor` etc., but the rest of the struct (flags/fmt/siz/block_*) is still mangled.
- `a7024aa` — initialise the `color` local to white to avoid uninitialized fallthrough in matanim.
- `ec9f5a1` — init shadow_alt_left/right. Unrelated.
- `e808126` — Kirby/Samus helmet copy; FTKirbyCopy u16 fixup. Isolated, not the general fighter issue.
- `96926fc` — defensive NULL guards for fighter joint derefs. Symptomatic workaround for downstream problems caused by upstream corruption.

None of these change the pass1→MObjSub wire. They all operate on data that has already been read through the wrong byte positions.

---

## 3. Recommended next experiment

**Add instrumentation in `gcAddMObjForDObj` (src/sys/objman.c:1307) that emits one `port_log` line per call** recording:
1. The **source** pointer (`mobjsub`), and whether it lives inside a reloc file (via `portRelocFindContainingFile`), plus the file_id and byte offset.
2. First 4 u32 words raw from `*((u32*)mobjsub + 0..3)`.
3. Derived values as the port currently reads them: `mobjsub->fmt`, `.siz`, `.flags` (u16), `.block_fmt`, `.block_siz`, `.primcolor.pack`, `.primcolor.s.r/g/b/a`, `.light1color.pack`.
4. The containing DObj's GObj kind so we can distinguish fighter vs scene-geometry vs item call sites.

Expected outcome:
- For call sites reached via `gcSetupCustomDObjsWithMObj`/`gcAddMObjAll` (scene geometry, which goes through `portFixupMObjSub`), the decoded fields will match the original file layout — primcolor channels sensible, flags containing recognised `MOBJ_FLAG_*` bits, fmt/siz ∈ valid ranges.
- For call sites reached via `lbCommonAddMObjForFighterPartsDObj` (fighters), the same bytes will show `fmt=0, siz=0` and `flags` looking like `0x0NNN` where the low byte tracks block_siz and the next byte tracks block_fmt. primcolor.s.r/g/b/a will show the byte-reversed pattern (e.g. Mario's body at `(R=0xFF, G=0x00, B=0x00, A=0xFF)` will log as `r=0xFF, g=0x00, b=0x00, a=0xFF` in the pack u32 but `(r=0xFF, g=0x00, b=0x00, a=0xFF)` from `.s.*` fields should **appear swapped** — if the hypothesis holds, `s.r=0xFF, s.g=0x00, s.b=0x00, s.a=0xFF` would match pack, but the values after BSWAP32 will be reversed).

One 30-second capture of the first few frames of `mvopeningroom` → `ftOpeningMario` scenes would be decisive: if fighter call sites show `fmt=0/siz=0/flags=0x0N..` and scene-geometry call sites show valid fmt/siz/flags, H1 is confirmed and a one-line insertion of `portFixupMObjSub(mobjsub)` before `gcAddMObjForDObj` in every `lbcommon.c` call site should land the fix.

**Secondary experiment** (while we're in there): capture `fp->fkind` + joint index + the values of `detail_p_mobjsubs` raw bytes around `ftparam.c:811`. If the 8-byte-stride bug in H2 is live, adjacent joints' MObjSub chains will appear misplaced.

---

## 4. What I ruled out

- **Torch extraction producing garbage bytes.** The RelocFactory is a byte-preserving blob exporter; no struct interpretation, no format-aware transforms. The `SSB64_DUMP_FILE_ID` hook in `lbreloc_bridge.cpp:331` exists specifically to diff the post-memcpy bytes against `debug_tools/reloc_extract/reloc_extract.py`'s ROM extraction, and nothing in the git history indicates a mismatch on fighter files. If Torch were corrupting fighter data, OpeningRoom fighter parts would be identically wrong in *every* field, but the matanim-fix commit (`25edeb0`) only found the color channels wrong — which is consistent with pass1-only MObjSub, not with upstream corruption.
- **Vertex data being wrong.** The lazy runtime vertex fixup (`portRelocFixupVertexAtRuntime`) handles per-Vtx rotate16/bswap32 for every G_VTX the interpreter actually dispatches, and `fbbcfa6` + OpeningRoom success indicate geometry arrives at Fast3D with correct positions/normals/uvs/colors. Fighter meshes are made of vertices from the same reloc-file pool that the lazy fixup covers.
- **Display list format / seg 0x0E resolution.** Already fixed in `648d822` (G_DL seg 0x0E resolution) and `e375c8f` (widened-stride fix at runtime). Fighter DL execution works; the problem is the *data the DL refers to via side channels* (MObjSub reads in objdisplay.c build the combiner/lighting state before the fighter's actual mesh DL runs).
- **Fighter-common tokenisation.** The `u32` token field on FTCommonPart/FTModelPart/FTAccessPart, the `_Static_assert(sizeof(FTCommonPart) == 16)` guard, and the `PORT_RESOLVE_ARRAY` / `PORT_RESOLVE` macro sites I walked in `lbcommon.c`, `ftparam.c`, `ftdisplaymain.c`, and `ftmain.c` all resolve consistently for the fighter-setup paths. The initial `dobjdesc` / `p_mobjsubs` / `p_costume_matanim_joints` token resolution is fine. The breakage is strictly one layer deeper — inside the MObjSub struct those resolved pointers point at.
- **Lighting hack in `ftDisplayLightsDrawReflect` being wrong.** The ambient-at-LIGHT_2 + directional-at-LIGHT_1 pattern is consistent with `gSPNumLights(NUMLIGHTS_1)` + F3DEX2 semantics ("n + 1 lights = n directional + 1 ambient at slot n+1"). It's arguably a bit of a hack, but it produces sensible baseline lighting — the fighter darkness is explained by `MOBJ_FLAG_LIGHT1/LIGHT2` never firing, not by broken lighting setup.
- **`portFixupFTAttributes`.** That struct carries physics/collision/damage/SFX fields; none of them are in the fighter display path. Confirmed at `lbreloc_byteswap.cpp:1456`.

---

## 5. Open questions the main session needs to answer before implementing

1. **Is there a reason the original author skipped fixup on the fighter path?** The commit that added `portFixupMObjSub` (`6ae0077`, 2026-04-07) wired it into `objanim.c` but not `lbcommon.c`. Was that an oversight or is there a known ordering issue (e.g. the fighter code touches the MObjSub before `lbCommonSetupFighterPartsDObjs` runs, so a fixup-at-load-time would be more appropriate)? Git blame / commit message doesn't say.
2. **Are there any fighter-code sites that *expect* the u8/u16 fields to be in pass1-only state** and compensate locally? Fast global grep (`grep -rn 'mobj->sub\.\(flags\|fmt\|siz\|primcolor\)' src/ft/`) returns nothing, so probably not, but worth a closer look before landing a change.
3. **Where is the *right place* to apply the fixup — at load time (once per file, across every embedded MObjSub the file references) or at use time (in `gcAddMObjForDObj` itself, idempotent)?** The existing scene-geometry path puts it at use time. The simplest fix for fighters would mirror that: a `portFixupMObjSub(mobjsub)` call immediately before each `gcAddMObjForDObj` in `lbcommon.c`. The fixup function is already idempotent via `sStructU16Fixups`, so a use-time call is safe even if the same MObjSub shows up on two code paths. But load-time fixup (walking the MObjSub array inside the fighter-main reloc file) would make the data correct for any future reader without per-site manual plumbing.
4. **Does `MObj *new_mobj = gcAddMObjForDObj(...)` expose the post-copy `new_mobj->sub` to any code path that writes back to the *original* MObjSub struct** (as opposed to only reading)? If anything writes via the copy's pointer back to the pool, a duplicate fixup could double-rotate it. I did not find such a write path but didn't audit exhaustively.
5. **What, if anything, does Fast3D do with per-vertex RGBA on fighters?** Lighting mode uses vertex normals → RDP computes a per-vertex colour from light1+ambient+normal. Non-lighting materials use the vertex colour directly. If any fighter material uses non-lighting (e.g. eye textures, skin textures with baked shading), the vertex-colour fixup is the relevant path, and the lazy runtime vertex fixup would handle it. This is a secondary path and probably fine, but worth confirming while instrumenting.
6. **Is there a way to detect this automatically?** The existing `gcLogMObjResolveWarning` (src/sys/objdisplay.c:77) already logs `fmt/siz/block_fmt/block_siz/flags` per warning — we could add a PORT-only sanity check in `gcAddMObjForDObj` that flags "impossible" field combinations (e.g. `fmt > 4`, `siz > 3`, `flags & ~KNOWN_FLAGS_MASK`) so this class of bug would surface as a startup warning in future struct changes.

---

## Appendix A — Key file & line references

- Fighter MObjSub path (missing fixup):
  - `src/lb/lbcommon.c:1030` — `lbCommonAddMObjForFighterPartsDObj`, PORT branch.
  - `src/lb/lbcommon.c:1263` — `lbCommonSetupCustomTreeDObjsWithMObj`, PORT branch.
  - `src/lb/lbcommon.c:1313` — `lbCommonAddMObjForTreeDObjs`, PORT branch.
- Scene-geometry MObjSub path (has fixup):
  - `src/sys/objanim.c:2734` — `gcSetupCustomDObjsWithMObj`, PORT branch.
  - `src/sys/objanim.c:2798` — `gcAddMObjAll`, PORT branch.
- Fixup function: `port/bridge/lbreloc_byteswap.cpp:1404 portFixupMObjSub`.
- MObjSub struct definition: `src/sys/objtypes.h:320-370`.
- SYColorPack definition: `include/ssb_types.h:97-102`.
- Draw-time reads of MObjSub fields: `src/sys/objdisplay.c:1322-1720 gcDrawMObjForDObj`.
- Fighter render entry: `src/ft/ftdisplaymain.c:756 ftDisplayMainDrawDefault`.
- Fighter init flow:
  - `src/ft/ftmanager.c:787` → `lbCommonSetupFighterPartsDObjs` → `lbCommonAddMObjForFighterPartsDObj`.
  - `src/ft/ftparam.c:1042` (`ftParamInitAllParts`) → same.
  - `src/ft/ftparam.c:893, 787, 1042, 1072, 1092` → ft model-part variants.
- ftparam raw-indexing bug sites: `src/ft/ftparam.c:811, 817, 917, 923, 1062, 1068`.
- Matanim color-path LE fix: `src/sys/objanim.c:1382-1628 gcPlayMObjMatAnim` (commits `25edeb0`, `a7024aa`).
- Fighter lights port-hack: `src/ft/ftdisplaylights.c:10-70 ftDisplayLightsDrawReflect`.
- Byte-swap pipeline: `port/bridge/lbreloc_byteswap.cpp:699-1497`.
- Reloc file loader: `port/bridge/lbreloc_bridge.cpp:301-497 lbRelocLoadAndRelocFile`.
- Figatree fixup: `port/bridge/lbreloc_bridge.cpp:133-157 portRelocFixupFighterFigatree`.
- Torch reloc factory (opaque blob): `torch/src/factories/ssb64/RelocFactory.cpp:119-208`.
