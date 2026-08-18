#ifndef NDS_UI_KIT_H
#define NDS_UI_KIT_H

#include <PR/ultratypes.h>

/* P2-1c -- the 2D UI kit the VS shell's screens are built out of.
 *
 * WHAT IT IS. Retained slots on one 2D engine's OBJ layer: text fields drawn
 * in the SSB64 menu font, and whole-image sprites (hand cursor, CSS
 * portraits).  A caller sets a slot's content when the content changes, moves
 * it whenever it likes, and the kit publishes shadow OAM once a frame.  The
 * per-frame cost of a screen that is not changing is one `oamSet` per visible
 * slot; composing pixels happens only on a content change, which on a menu
 * means on a cursor move, not on a frame.
 *
 * WHY OBJ AND NOT A BACKGROUND. Both main BG VRAM banks are already spoken
 * for -- C and D are the two 256x256 16-bit bitmaps the battle's source-sprite
 * compositor owns, exactly 128 KiB each, with nothing left over -- and the
 * only bank the boot map leaves unassigned is I, which is sub-engine only.
 * Main OBJ VRAM (bank E, 64 KiB) is the one main-engine surface that is free
 * during a menu scene, because its battle tenant (`nds_ifcommon_oam.c`) is
 * prepared from the battle scene's own asset load and holds nothing until
 * then. The full per-scene bank ownership table is `docs/p2/P2-1c-vram-map.md`.
 *
 * WHY BITMAP OBJ AND NOT PALETTED TILES. `ndsPlatformInit` initialises main
 * OAM as `SpriteMapping_Bmp_1D_128`, so direct-colour cells are the mode this
 * engine is already in.  Sharing it costs 2 bytes a texel and buys: no palette
 * arbitration with the battle's 256-entry OBJ palette, no second DISPCNT
 * boundary to keep consistent, and one code path.  Text is tinted at compose
 * time from the source's own primitive colour instead (mnmaps.c:340).
 *
 * DUAL SCREEN. Every entry point takes an engine.  The main engine gets the
 * whole kit; the sub engine gets text only, because bank I is 16 KiB and the
 * 64x64 portrait cells do not fit beside the text fields.  That is the whole
 * difference, and it is a capacity fact, not a missing feature -- the
 * bottom-screen battle HUD P2-2 owns is text and small sprites.
 *
 * OWNERSHIP. The kit owns OAM ids [0, NDS_UI_KIT_OAM_IDS) and the TOP of its
 * engine's OBJ VRAM.  `nds_ifcommon_oam.c` allocates OAM ids downward from
 * 127 and OBJ VRAM upward from 0, so the two grow away from each other; they
 * are additionally never live at once, and `ndsUiKitEnter` refuses (counting
 * `gNdsUiKitEnterRejectCount`) if the battle's OBJ tenant is still prepared. */

/* Which 2D engine a call addresses. */
#define NDS_UI_KIT_ENGINE_MAIN 0u
#define NDS_UI_KIT_ENGINE_SUB 1u

/* Text fields, and the 32x8 bitmap-OBJ cells each is composed of.  A field of
 * four cells is 128 px of text, which is wider than any label the SSB64 menus
 * draw in this font.
 *
 * EIGHT IS THE BANK-I CEILING, not a round number.  32 cells is exactly 16 KiB
 * and bank I is exactly 16 KiB, so eight fields is the largest layout both
 * engines can run -- which is what keeps one code path for the top-screen menus
 * and P2-2's bottom-screen HUD.  P2-1c needed six; P2-1e's character select
 * needs eight (FREE FOR ALL, BACK, READY TO FIGHT, PRESS START, and one
 * fighter-name field per slot), and the static assert in nds_ui_kit.c is what
 * stops a ninth from silently running bank I off its end. */
#define NDS_UI_KIT_TEXT_SLOTS 8u
#define NDS_UI_KIT_TEXT_CHUNKS 4u
#define NDS_UI_KIT_TEXT_CHUNK_W 32u
#define NDS_UI_KIT_TEXT_CHUNK_H 8u
#define NDS_UI_KIT_TEXT_MAX_PX \
    (NDS_UI_KIT_TEXT_CHUNKS * NDS_UI_KIT_TEXT_CHUNK_W)

/* Whole-image slots: cursor, portraits, tokens, panel labels, digits.
 *
 * LOW ID DRAWS ON TOP (OBJ priority ties break on OAM index), so the order a
 * screen assigns these in is its depth order.  The character select needs the
 * most: one cursor, four tokens, four player-kind labels, four CPU-level
 * labels, four CPU-level digits and twelve portrait cells -- 29, and the
 * digits a rules screen draws are the same slots reused. */
#define NDS_UI_KIT_SPRITE_SLOTS 29u

#define NDS_UI_KIT_OAM_IDS \
    ((NDS_UI_KIT_TEXT_SLOTS * NDS_UI_KIT_TEXT_CHUNKS) + \
     NDS_UI_KIT_SPRITE_SLOTS)

/* The menu cues, by the source's own FGM ids (gm/gmsound.h, REGION_US
 * numbering).  Every one is placed where the source places it, and the two
 * P2-1d added are the two the source distinguishes and P2-1c could not yet
 * name a caller for:
 *
 *   MOVE    MenuScroll2 -- the CURSOR moving between entries
 *                          (mnmodeselect.c:805, mnvsmode.c:1361/:1401).
 *   VALUE   MenuScroll1 -- a VALUE changing inside one entry, left/right
 *                          (mnvsmode.c:1449/:1481/:1516/:1554, mnoption.c).
 *                          This is the caller P2-1c-1 packed 163 for and left
 *                          "for whichever direction P2-1d wires it".
 *   CONFIRM MenuSelect  -- A/START on an entry (mnmodeselect.c:734).
 *   BACK    MenuDenied  -- the REFUSAL. The source never cues plain B (it
 *                          transitions silently); it spends this id when an
 *                          action is refused (mnplayersvs.c:177/:2840/:3175),
 *                          which is what the greyed 1P/OPTION/DATA entries and
 *                          the not-yet-built VS OPTIONS button do here.
 *   START   TitlePressStart -- the title screen's own confirm
 *                          (mntitle.c:501). NOT IN THE FGM PACK: the pack
 *                          carries 158/163/164/165 and this is 157, so the
 *                          seam asks and the FGM miss ring records it. Row
 *                          P2-1d-1 renders it. */
#define NDS_UI_KIT_SFX_MOVE 0u
#define NDS_UI_KIT_SFX_CONFIRM 1u
#define NDS_UI_KIT_SFX_BACK 2u
#define NDS_UI_KIT_SFX_VALUE 3u
#define NDS_UI_KIT_SFX_START 4u
#define NDS_UI_KIT_SFX_COUNT 5u

typedef struct NdsUiKitGlyphMetric {
    u8 width;
    u8 height;
} NdsUiKitGlyphMetric;

typedef struct NdsUiKitImageMetric {
    u32 offset; /* byte offset into the pack */
    u32 bytes;
    u8 cell_w;
    u8 cell_h;
    u8 src_w;
    u8 src_h;
} NdsUiKitImageMetric;

/* P2-1h -- a BACKDROP surface: menu art too large for an OBJ cell, drawn into
 * the main engine's BG2 bitmap.
 *
 * WHY BG2 AND NOT A NEW BANK.  `docs/p2/P2-1c-vram-map.md` prices the three
 * options a menu background could take (3D through BG0, the existing BG2
 * bitmap compositor, or a texture bank) and every one of the other two costs
 * VRAM this build does not have: main C and D are exactly one 256x256 Bmp16
 * each with zero slack, and A/B are the GX texture pool.  BG2 costs NOTHING,
 * because the menu shell ALREADY writes it -- `ndsMenuShellRun` clears both
 * overlay layers on every screen entry so a menu cannot inherit the last
 * battle frame.  Writing art there instead of zeroes adds no bank, no
 * arbitration and no per-frame work: a backdrop is composed once at scene
 * entry and then simply stays on the screen.
 *
 * `x`/`y` are the DS top-left and are SIGNED -- the title emblem's 4/5 origin
 * is y = -2, exactly as the source's own 320-wide frame clips it at 324.
 * `opaque` is a bake-time fact (no transparent texel anywhere), and it is what
 * lets the biggest surface move by whole-row DMA instead of a per-texel
 * transparency test. */
typedef struct NdsUiKitSurfaceMetric {
    u32 offset; /* byte offset into the surface pack */
    u32 bytes;
    u16 width;
    u16 height;
    s16 x;
    s16 y;
    u16 opaque;
    u32 fnv32; /* this surface's own FNV-1a, so one blit is verifiable alone */
} NdsUiKitSurfaceMetric;

/* --- Lifetime. Enter claims the engine's OBJ surface and loads the pack;
 * Exit hides everything the kit owns and releases the claim.  Both are
 * idempotent, and both are safe to call on a scene that never draws UI. --- */
s32 ndsUiKitEnter(u32 engine);
void ndsUiKitExit(void);

/* Publishes shadow OAM.  Called once per presented frame from
 * ndsPlatformEndFrame, after the battle OAM tenant's own commit. */
void ndsUiKitCommit(void);

/* --- Text.  `text` is the source's character set: A-Z, apostrophe, percent,
 * period, space; '0'-'9' are the source's kerning escapes (mnmaps.c:308) and
 * advance by their digit value rather than drawing.  `rgb` is 0x00RRGGBB and
 * is the SObj primitive colour the original set per string. --- */
s32 ndsUiKitSetText(u32 slot, const char *text, u32 rgb);
void ndsUiKitMoveText(u32 slot, s32 x, s32 y);
void ndsUiKitHideText(u32 slot);
/* Laid-out width in pixels, by the source's advance and kerning rules. */
u32 ndsUiKitTextWidth(const char *text);

/* --- Images. `image` indexes the generated manifest
 * (NDS_MN_UI_KIT_IMAGE_*). (x, y) is the top-left of the source sprite. --- */
s32 ndsUiKitSetSprite(u32 slot, u32 image, s32 x, s32 y);
void ndsUiKitMoveSprite(u32 slot, s32 x, s32 y);
void ndsUiKitHideSprite(u32 slot);

/* --- Numbers. The font has no digits (see the SFX block above): the source
 * draws every menu number as one digit SPRITE per place, right-aligned, at an
 * 11 px pitch (mnvsmode.c mnVSModeMakeNumber). This does the same, consuming
 * `digits` sprite slots starting at `slot`, and returns the number of slots it
 * used so a caller can hide the rest. `right_x` is the pixel one past the
 * last digit, as in the source, so a two-digit value grows leftward and the
 * label before it does not move. --- */
#define NDS_UI_KIT_DIGIT_PITCH 11
u32 ndsUiKitSetNumber(u32 slot, u32 slots_available, s32 value, s32 right_x,
                      s32 y);
/* Layout width in pixels a number of this value will occupy. */
u32 ndsUiKitNumberWidth(s32 value);

/* --- Backdrop surfaces (P2-1h). `surfaces` indexes the generated manifest
 * (NDS_MN_UI_KIT_SURFACE_*).
 *
 * `ndsUiKitBlitSurfaces` takes a LIST because a screen's whole backdrop is one
 * NitroFS open: the title reads its own set, the two collage screens read one
 * surface, and the character/stage selects read none. Call it once, at screen
 * entry, AFTER the overlay layers are cleared.
 *
 * A blinking element cannot be re-read at 60 Hz -- one NitroFS open costs
 * more than a whole frame's budget -- so `ndsUiKitCacheSurface` keeps exactly
 * one surface's texels in RAM and `ndsUiKitDrawCachedSurface` /
 * `ndsUiKitEraseCachedSurface` toggle it in place. Erase writes the field the
 * surface was composited over, which is why that field is a bake-time
 * constant rather than something the runtime has to remember. --- */
s32 ndsUiKitBlitSurfaces(const u8 *surfaces, u32 count);

/* P2-1i -- the title's fire ATLAS. Its own entry point because it is the one
 * surface that is not a screen-space backdrop: it is a 255x252 sheet of the
 * thirty `mnTitleMakeFire` states written into BG3's 256x256 bitmap, which the
 * BG3 affine then reads one 51x42 cell of per frame. The backdrop path clips
 * every row to the 192-row SCREEN, which is correct for a backdrop and would
 * throw away 60 of this sheet's rows. Returns FALSE and counts the same
 * surface counters on any failure. */
s32 ndsUiKitBlitFireAtlas(void);
s32 ndsUiKitCacheSurface(u32 surface);
void ndsUiKitDrawCachedSurface(void);
void ndsUiKitEraseCachedSurface(u16 field_texel);

/* --- Audio. One of NDS_UI_KIT_SFX_*. --- */
void ndsUiKitSfx(u32 cue);

/* --- Published state. Read by scripts/menus/probe-p2-1d-menus.ps1; none of
 * it is read by gameplay. --- */
extern volatile u32 gNdsUiKitEnterCount;
extern volatile u32 gNdsUiKitEnterRejectCount;
extern volatile u32 gNdsUiKitExitCount;
extern volatile u32 gNdsUiKitEngine;
/* Pack load: bytes read, the FNV-1a the load computed, and the mismatch
 * count. A zero-byte load with a clean hash is impossible by construction --
 * the hash is seeded and folded over every byte that reaches the engine. */
/* NitroFS opens the pack load spent. It is ONE per kit entry since P2-1d; it
 * was one per staged chunk before (twelve for the P2-1c pack, and the digit
 * block would have made it twenty-three). Published so the residual stays
 * closed by measurement rather than by assertion. */
extern volatile u32 gNdsUiKitPackOpenCount;
extern volatile u32 gNdsUiKitPackBytesLoaded;
extern volatile u32 gNdsUiKitPackHash;
extern volatile u32 gNdsUiKitPackHashMismatchCount;
extern volatile u32 gNdsUiKitPackReadFailCount;
/* Composition and publication. Compose runs on content change only; a screen
 * holding still must show ComposeCount flat and CommitCount climbing. */
extern volatile u32 gNdsUiKitTextComposeCount;
extern volatile u32 gNdsUiKitTextComposeSkipCount;
extern volatile u32 gNdsUiKitTextOverflowCount;
extern volatile u32 gNdsUiKitCommitCount;
extern volatile u32 gNdsUiKitCommitIdleCount;
extern volatile u32 gNdsUiKitVisibleObjectCount;
/* Menu cues requested, by NDS_UI_KIT_SFX_* index. Paired with the FGM miss
 * ring, this separates "the seam never fired" from "the pack has no sample". */
extern volatile u32 gNdsUiKitSfxRequestCount[NDS_UI_KIT_SFX_COUNT];
extern volatile u32 gNdsUiKitSfxLastId;
/* P2-1h backdrop surfaces. `Blit` counts surfaces drawn and `Open` the NitroFS
 * opens they cost, so "one open per screen entry" stays measured rather than
 * asserted. `HashMismatch` is checked per surface against the bake's own
 * constant, so a truncated or stale pack is a counted failure and not a
 * garbled screen; `NoLayer` counts a blit that found no BG2 to draw into,
 * which is the only way this can silently do nothing. */
extern volatile u32 gNdsUiKitSurfaceOpenCount;
extern volatile u32 gNdsUiKitSurfaceBlitCount;
/* P2-1i. The fire atlas is counted SEPARATELY from the backdrop blits above,
 * and that separation is the point: the loop verifier asserts one backdrop
 * blit per backdrop-screen entry, and the atlas is not a backdrop -- it is the
 * title's BG3 animation sheet. Folding it into the backdrop count turned a
 * live invariant into "3 or 4 depending on the screen", so it gets its own
 * counter and its own assertion (exactly one per title entry) instead. */
extern volatile u32 gNdsUiKitFireAtlasBlitCount;
extern volatile u32 gNdsUiKitSurfaceBytes;
extern volatile u32 gNdsUiKitSurfaceHashMismatchCount;
extern volatile u32 gNdsUiKitSurfaceReadFailCount;
extern volatile u32 gNdsUiKitSurfaceNoLayerCount;
extern volatile u32 gNdsUiKitSurfaceLastHash;
extern volatile u32 gNdsUiKitSurfaceCacheCount;
extern volatile u32 gNdsUiKitSurfaceDrawCachedCount;
extern volatile u32 gNdsUiKitSurfaceEraseCachedCount;
extern volatile u32 gNdsUiKitSurfaceTicks;

#endif /* NDS_UI_KIT_H */
