# Native Results OAM owner (VS Results, scene 24)

Design for `src/nds/nds_results_oam.c` + `include/nds/nds_results_oam.h`.
Every measured claim carries its source; anything marked PROPOSAL is a design
choice, not a source fact. Written 2026-09-08 from a full read of
`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c`,
`src/nds/nds_ifcommon_oam.c` and `src/port/sprite_preview_backend.c`.

## Why this exists

Every visible non-wallpaper SObj in Results reaches
`ndsDrawLayeredSObjFrame` (`sprite_preview_backend.c:826`) and is recorded as
`NDS_NATIVE_FAILURE_SPRITE`, because no native sprite program exists for the
screen. The first native failure of a whole VS run is a Results player tag:
identity `0x1b` = GObj id 0 / DL link 27, status `0x30001` = IA/8b, which
matches `gcMakeGObjSPAfter(0, NULL, 18, ...)` at `mnvsresults.c:1056`.

## 1. Source draw order (load-bearing)

Cameras live on GObj link 16 / DL link 63. `gcDrawAll` walks
`gGCCommonDLLinks[63]` head to tail (`sys/objman.c:2100`) and the list is
sorted descending by priority (`objman.c:459`), so **the highest camera
priority draws first, furthest back**.

| order | prio | DL link | contents | created |
|---|---|---|---|---|
| 1 | 80 | 26 | wallpaper SObj (`mnvsresults.c:694`) | DrawWallpaperTic |
| 2 | 70 | 34 | WallpaperTint2, full-viewport black fill, no SObj (`:1775`) | DrawWallpaperTic, skipped for NoContest (`:3237`) |
| 3 | 60 | 33 | emblem DObj tree (3D) (`:615`) | FuncStart |
| 4 | 55 | 35 | WallpaperTint, full-viewport black fill, no SObj (`:1721`) | FuncStart (`:3379`) |
| 5 | 50 | 18,15,10,9 | fighters (3D) | InitFightersAllTic |
| 6 | 30 | 27 | player tags (`:1048`) | InitFightersAllTic |
| 7 | 20 | 29 | winner name + WINS!/NO CONTEST (`:1142`) | MakeResultsTic |
| 8 | 17 | 30 | Tint, full-viewport black fill, no SObj (`:1667`) | tic 180 (30 NoContest) |
| 9 | 15 | 31 | labels, rule, header arrows/stocks, KO/TKO/PTS/PLACE rows, bar | tics 120, 210-290 |
| 10 | 10 | 32 | `lbTransition` DObj tree (3D) | FuncStart |

Consequence: the Tint darkens wallpaper, emblem, fighters, tags and results
text but **not** the header table. The two wallpaper tints darken less again.
Three independent black rectangles at three depths.

## 2. Sprite inventory

Source positions are 320x240 screen coordinates; the wallpaper sits at
`pos = (10,10)` for a 300x220 image inside the `(10,10)-(310,230)` viewport
(`mnvsresults.c:763-764`, `:735`).

**Player tags** — link 27, GObj link 18, one SObj per present player
(`:1048-1096`). `llIFCommonPlayerTags{1..4}P/CPSprite`, IA/8b, 19x24 / 20x24 /
20x24 / 21x24 (`reloc_backend_assets.c:2743-2749`); CP is asset
`IF_COMMON_PLAYER_TAGS` offset `0xcd8` (`include/reloc_data.h:329`). Colour is
`envcolor = dIFCommonPlayerTagEnvColors[]` (all zero, `if/ifcommon.c:293`) and
`sprite.rgb = dIFCommonPlayerTagPrimColors[]` (R = `ED,4E,FF,4E,AC`,
`ifcommon.c:284`). `attr &= ~SP_FASTCOPY; attr |= SP_TRANSPARENT` (`:1073`).
Position from a static table by (present count, distance, spot) (`:995-1046`),
set once, never animated.

**Results text** — link 29, GObj link 20, one SObj per drawn glyph
(`mnVSResultsMakeString`, `:1142-1225`). 28 announce letters plus Exclaim and
Period, IA/8b (`reloc_backend_assets.c:2687-2742`); widths match the source's
own `widths[]` (`:1158-1162`), heights 36-39 except Period 11. `char_id =
c - 'A'`, `!` = 0x1A, `.` = 0x1B, space = 0x1C (`:1123-1140`); digits in the
string are kerning escapes that draw nothing (`:1186-1189`). Per glyph only
`scalex` is set (`:1204`) — **anisotropic on purpose**. Colour is swapped
relative to the field names: `envcolor = colors[].prim`, `sprite.rgb =
colors[].env` (`:1215-1220`), so the ramp runs outline colour to white.

**Header** — link 31, GObj link 22. Per player an arrow
`llMNVSResults{1..4}PArrowSprite` 15x12 IA/8b at `(GetColumnX(i)+17, 49)`
(`:1874-1876`, dims `reloc_backend_assets.c:1388-1391`), prim `FFFFFF` / env
`000000` (`:1840-1851`); a stock icon 8x10 CI/4b with the costume LUT at
`(arrow.x - 10, 49)` (`:1881-1886`). Row labels KOs/TKO/PTS 62x13 and PLACE
83x17, IA/8b (`reloc_backend_assets.c:1384-1387`). Mode label
`llMNPlayersGameModes{FreeForAll,TeamBattle}TextSprite` 112x11 / 110x9,
**I/4b** at `(32,29)` (`:2392-2398`). Numbers via `mnVSResultsMakeNumber`
(`:1593-1641`): optional dash 6x3 then digits 8x10 (digit `1` is 5x10),
IA/8b. Place row uses damage digits 11-17 x 19 or, for place 1,
`llMNVSResultsWinnerSprite` **42x35 RGBA/16b, 2 bitmaps**
(`reloc_backend_assets.c:1393`). Number colour is swapped the same way
(`:1487-1505`). Row schedule at `:2207-2327`. Nothing animates after creation
except the bar width.

**Fills with no SObj** — these are silently dropped today, which is a
native-only violation independent of the sprites. `ndsMenuFillSink` is gated
on `NDS_SCENE_FLAG_MENU` (`sprite_preview_backend.c:1204-1209`) and the
Results scene row carries `NDS_SCENE_FLAG_ARENA_RESET` instead
(`nds_scene_manager.c:43-44`).

| fill | proc | rect | value | per frame |
|---|---|---|---|---|
| Tint | `:1643` | `(10,10,310,230)` | black, `sMNVSResultsTintAlpha` | `+= 0x09` clamp `0x80` |
| WallpaperTint | `:1699` | same | black | `-= 0x05` from `0xFF` |
| WallpaperTint2 | `:1753` | same | black | same |
| bar | `:2024` | `(87, y, 87+width, y)` | opaque white | `width += 10` clamp 190 |
| label rule | `:2335` | `(32,42,282,44)` | opaque white | static, then the SObj |

## 3. What the battle OAM path can and cannot lend

`src/nds/nds_ifcommon_oam.c`. Asset identity is a 25-entry static table
matched by `(bitmap, width, height)` (`:2310-2348`) bound from
`IF_COMMON_GAME_STATUS`, a file Results never loads (`mnvsresults.c:69-79`),
so `sNdsIFCommonPrepared` is FALSE for the whole scene and every guarded entry
point fails closed. Cells come from a fixed bank plan in OBJ VRAM bank E
(64 KiB, `nds_platform.c:481`): GO 0..17408, END ..38144, SPARK ..60672, TAG
..63744 (`:48-56`), alignment 128 matching
`SpriteMapping_Bmp_1D_128` (`:2020`). Emission is two-pass with rollback
(`:3200-3401`) and one `oamUpdate` per frame (`:3403-3431`) — **copy that
shape**. Capacity: 128 OAM ids allocated downward from 127, 32 affine slots
(`:2350-2370`), 16 OBJ palette banks.

Blocked for Results: the call site is gated on
`gNdsSceneManagerCurrIsBattle` (`sprite_preview_backend.c:788-790`); the tag
path requires `gSCManagerBattleState`, `battle_player->fighter_gobj`,
`gGMCameraGObj` and camera projection (`:3072-3140`) where Results writes
`SObj->pos` directly (`mnvsresults.c:1024-1044`); dispatch keys on
`ifCommonPlayerTagProcDisplay` (`:3221`); the bake early-outs on the prepared
latch (`:2885`); the tag palette hard-codes `palette[1] = 1 << 15`
(`:2971-2995`), correct only because battle tag env is black; and the emitter
refuses `scale_x_q16 != scale_y_q16` (`:2567-2579`), which Results legitimately
needs. Reusable verbatim: `ndsIFCommonSpriteSize` (`:1413-1428`),
`ndsIFCommonBilerpPremultipliedRgba` (`:650-700`), `ndsIFCommonBitmapAlpha`
(`:2499-2506`), the rounding helpers (`:2508-2518`) and the TEXSHUF row
walkers (`:763`, `:823`, `:574`, `:2786`).

During scene 24 the ifcommon tenant is inert and the UI kit is not entered, so
**all OBJ VRAM, all 128 OAM ids, all 32 affine slots and all 16 palette banks
are free**.

## 4. Design

### 4.1 Coordinate map

The Results wallpaper is already full-bleed: `ndsNativeWallpaperAffine`'s
format-1 branch stretches the converted 240x176 asset across 256x192
(`nds_native_wallpaper.c:105-111`, `:149-150`). The foreground must use the
same map or it will sit inset inside a background that is not.

```
NDS_RESULTS_SRC_ORIGIN_X 10, NDS_RESULTS_SRC_ORIGIN_Y 10
NDS_RESULTS_SCALE_X_Q16  55924  /* round(65536 * 256/300) */
NDS_RESULTS_SCALE_Y_Q16  57195  /* round(65536 * 192/220) */
```

PROPOSAL alternative, one constant pair away and more source-faithful: use
`NDS_IFCOMMON_SCREEN_SCALE_Q16` (52429, x0.8) for both and place the wallpaper
at (8,8) instead of stretching it. Owner's choice; the code differs only in
those constants.

### 4.2 Identity and baking

Identity comes from reloc provenance, not a manifest:
`ndsRelocGetLoadedPointerProvenance(sobj->sprite.bitmap, &asset, &offset)`
(`reloc_backend_assets.c:5834-5853`), already used for the wallpaper. Format
and dimensions come from the live `Sprite`. Nothing to drift.

| class | sprites | DS cell | palette |
|---|---|---|---|
| IA8 | tags, arrows, labels, digits, glyphs | 4bpp; index = intensity nibble; alpha nibble < 8 becomes index 0 | 16-entry env-to-prim ramp |
| I4 | mode label | 4bpp; index = the nibble (it is alpha) | black-to-prim ramp |
| CI4 | stock icons | direct colour `Bmp`, prefiltered through the LUT | none |
| RGBA16 | WINNER plate | direct colour `Bmp`, prefiltered, 1-bit alpha | none |

Every cell is **prefiltered once at bake time at its final DS size** with the
premultiplied bilinear sampler, then drawn at OAM scale 1 — OAM affine
sampling is nearest-neighbour and would destroy 8x10 digits at 0.85. Only the
tint plane uses an affine slot.

Tiler: cover the rect with legal shapes from `ndsIFCommonSpriteSize`, fewest
tiles first (OBJ ids are scarcer than the 64 KiB bank), maximum 4 tiles or it
is a loud failure. Worked examples: 62x13 to 53x11 to one 64x16; 83x17 to
71x15 to 64x16 + 8x16; 112x11 to 96x10 to 64x16 + 32x16; a 39x37 glyph to
33x32 to one 64x32; the 42x35 WINNER to 36x31 to one 64x32 `Bmp`.

Palette banks keyed on `(prim, env)`, entries 1..15 lerped env to prim with
bit 15 set, entry 0 transparent. **Entry 1 must be the env colour**, not
black — the battle tag path's black is env-specific. Live-at-once census:
white ramp 1, tag prims <= 4, team number colours <= 3, announce colours <= 2,
mode label 1, so **<= 11 of 16**.

### 4.3 Depth

Emit every Results OBJ at priority 0, allocating **downward from 127** in the
order `gcDrawAll` calls the display procs — which is exactly the table in
section 1. Lower OAM index draws on top, so the source's back-to-front order
falls out with no sorting code.

### 4.4 Fills

**Tint planes:** one 64x64 opaque-black `Bmp` cell (8192 B, baked at Enter),
emitted as 4 OBJs with `affineIndex = 0` (`oamRotateScale(&oamMain, 0, 0, 128,
128)`, x2) and `sizeDouble`, covering 256x192.
`paletteAlpha = ndsIFCommonBitmapAlpha(source_alpha)`. Needs one scoped
register change at Enter: `REG_BLDCNT |= BLEND_DST_BG0 | BLEND_DST_BACKDROP`,
preserving `BLEND_SRC_BG0 | BLEND_DST_BG2` (3D over wallpaper,
`nds_platform.c:508`) and restoring the saved value at Exit.

**Opaque rects:** map the source rect (RDP `gDPFillRectangle` is inclusive)
and tile it from pre-baked solid-white 4bpp cells at 8 px granularity
(1024 B total). The bar grows 10 source px per frame, one granule, so it is
exact at every step; the 201 px label rule rounds to 200.

### 4.5 API and call sites

```c
void ndsResultsOamEnter(void);
void ndsResultsOamExit(void);
s32  ndsResultsOamIsActive(void);
void ndsResultsOamBeginFrame(void);
s32  ndsResultsOamDrawGObj(struct GObj *gobj);
void ndsResultsOamCommit(void);
u32  ndsResultsOamBakeGObj(struct GObj *gobj);      /* optional pre-bake */
s32  ndsResultsOamEmitTintPlane(u32 source_alpha);
s32  ndsResultsOamEmitFillRect(s32 sx0, s32 sy0, s32 sx1, s32 sy1);
```

| function | call site |
|---|---|
| Enter | `battleship_mnvsresults.c:648`, before `ndsBaseMNVSResultsStartScene()` |
| BeginFrame | `sprite_preview_backend.c:862`, after `ndsIFCommonNativeOamBeginFrame()` |
| DrawGObj | `sprite_preview_backend.c:788`, a new arm beside the battle arm |
| Commit | `nds_platform.c:3924`, after `ndsIFCommonNativeOamCommit()` |
| Exit | `battleship_mnvsresults.c:212` `ndsMNVSResultsSetLoadScene`, first statement |

The five fills are intercepted in `battleship_mnvsresults.c` with the
`#define` before `#include` idiom already used there (`:146-158`): four port
procs call the source proc first (so the alpha/width state advance is never
duplicated; its dead `Gfx` words land in the arena that
`syTaskmanResetGraphicsHeap()` rewinds each frame) and then emit. The label
proc is re-expressed rather than called, because the source emits its rule
before its SObj and calling it would put the SObj at the wrong depth.

### 4.6 Draw, two passes, never a subset

Pass 1 resolves, bakes and counts every visible SObj; any failure returns
FALSE before a single OBJ is written. Pass 2 emits with rollback
(`oamClear` back to the entry id) so a partial emit is impossible. Returning
FALSE hands the GObj back to `ndsDrawLayeredSObjFrame`, which records one
`NDS_NATIVE_FAILURE_SPRITE` per visible SObj — **no new failure plumbing on
the SObj path**. The fill entry points have no such caller and record their own
failure through `ndsRendererRecordNativeFailure`
(`include/nds/nds_renderer.h:1414`).

Budget failures each get their own counter and none is silent:
`Inactive, UnsupportedFormat, BadProvenance, TileOverflow, CellSlotsFull,
VramFull, PaletteFull, OamFull`.

### 4.7 Lifecycle

Enter: reset state, bake the solid-white and tint cells, arm affine slot 0,
save and extend `REG_BLDCNT`, clear OAM. BeginFrame: clear the previous
frame's id range, reset the cursor to 127. Bake: lazily in pass 1, memoised,
so each glyph bakes once per scene. Commit: one `oamUpdate` when anything was
emitted. Exit: clear OAM, restore `REG_BLDCNT`, drop every cell so the battle's
bank plan gets its VRAM back. Re-entry after a rematch is covered because
Enter resets everything and Exit runs on the single load-scene path.

### 4.8 Budget (worst case: 4P team Time with three-digit scores)

About **36.1 KiB of 64 KiB** and **94 of 128 OBJs**; realistic FFA is ~66
OBJs. Affine slots 1 of 32, palette banks <= 11 of 16, cell slots 45 of 48.
The only configuration approaching 128 is No Contest, where two tint planes
are live at once, and that screen draws no PTS or PLACE rows.

## 5. Accepted DS deltas (each with its source value)

1. Three black rectangles at three depths become three OBJ planes at matching
   OAM indices. The Tint is exact. The two wallpaper tints also darken the 3D
   fighters, because an OBJ cannot sit below BG0 — bounded and invisible in
   Time/Stock (WallpaperTint reaches 0 at tic 52, fighters appear at tic 120;
   Tint2 overlaps tics 120-131 at <= 55/255 while the fighters are themselves
   ramping in). **No Contest is the exception** (all tics are 1), where it
   darkens the fighters for tics 1-52; record it, and if the owner rejects it,
   skip the WallpaperTint plane for that kind.
2. Fill alpha 0-255 becomes 4-bit OBJ alpha: Tint `0x80` (50.2%) renders as
   8/15 (53.3%); the tints' 51 steps become 15 visible ones.
3. Per-texel alpha does not exist on a DS OBJ. IA8 keeps the intensity as a
   15-step ramp and thresholds alpha at >= 8, the compromise the shipped tag
   bake already takes (`nds_ifcommon_oam.c:2928-2940`). The I4 mode label
   carries its anti-aliasing entirely in alpha, so its fringe darkens toward
   black instead of toward the wallpaper.
4. Palette index 0 is reserved transparent, leaving 15 levels; index 1 is the
   env colour.
5. Anisotropic glyph scale is reproduced by baking at
   `(scalex * SCALE_X, 1.0 * SCALE_Y)`.
6. Down-scaling is prefiltered at bake time, not sampled by OAM affine.
7. The 300x220 viewport maps to 256x192, a 6.7%/9.1% stretch versus source
   aspect, matching the wallpaper already on screen.
8. Fill rectangles quantise to 8 px.
9. `REG_BLDCNT` is extended and restored, never repurposed.
10. Not owned here: wallpaper (already native), emblem, fighters, the
    `lbTransition` wipe, and the confetti (already on the particle atlas).

## 6. Open questions to settle by measurement, not assumption

- Whether a bitmap OBJ's per-object alpha consumes `REG_BLDALPHA`'s EVA/EVB
  or supplies its own coefficients. If it does, the tint plane must write
  EVA/EVB per frame and its interaction with `BLEND_SRC_BG0` must be measured.
  One probe settles it.
- Whether `oamGfxPtrToOffset`'s step under `SpriteMapping_Bmp_1D_128` is 128
  bytes for 4bpp tiled cells as well as bitmap cells. The shipped bank offsets
  are all multiples of 128 but not of 256, which is consistent. Assert on the
  alignment constant regardless.
- The CP player-tag sprite's dimensions: the normalize manifest lists only
  1P-4P, so the CP header may be un-normalized. The design does not need the
  constant (dimensions come from the live `Sprite`) but a CPU player in
  Results would expose it.
- The per-frame cost of baking ~14 announce glyphs on one tic. The pre-bake
  entry point exists for that; the first A/B should report the tic-120
  present interval.
