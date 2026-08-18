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

/* Text fields, and the 32x8 bitmap-OBJ cells each is composed of.  Six fields
 * of four cells is 128 px of text a field, which is wider than any label the
 * SSB64 menus draw in this font, and 24 cells is 12 KiB -- the budget bank I
 * can hold, so both engines run the same layout. */
#define NDS_UI_KIT_TEXT_SLOTS 6u
#define NDS_UI_KIT_TEXT_CHUNKS 4u
#define NDS_UI_KIT_TEXT_CHUNK_W 32u
#define NDS_UI_KIT_TEXT_CHUNK_H 8u
#define NDS_UI_KIT_TEXT_MAX_PX \
    (NDS_UI_KIT_TEXT_CHUNKS * NDS_UI_KIT_TEXT_CHUNK_W)

/* Whole-image slots: cursor and portraits. */
#define NDS_UI_KIT_SPRITE_SLOTS 8u

#define NDS_UI_KIT_OAM_IDS \
    ((NDS_UI_KIT_TEXT_SLOTS * NDS_UI_KIT_TEXT_CHUNKS) + \
     NDS_UI_KIT_SPRITE_SLOTS)

/* The menu cues, by the source's own FGM ids (gm/gmsound.h, REGION_US
 * numbering).  mnmodeselect.c:805 and mnvsmode.c:1361 scroll with
 * MenuScroll2, mnmodeselect.c:734 confirms with MenuSelect, and MenuDenied is
 * the refusal. */
#define NDS_UI_KIT_SFX_MOVE 0u
#define NDS_UI_KIT_SFX_CONFIRM 1u
#define NDS_UI_KIT_SFX_BACK 2u
#define NDS_UI_KIT_SFX_COUNT 3u

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

/* --- Audio. One of NDS_UI_KIT_SFX_*. --- */
void ndsUiKitSfx(u32 cue);

/* --- Published state. Read by scripts/menus/probe-p2-1c-ui-kit.ps1; none of
 * it is read by gameplay. --- */
extern volatile u32 gNdsUiKitEnterCount;
extern volatile u32 gNdsUiKitEnterRejectCount;
extern volatile u32 gNdsUiKitExitCount;
extern volatile u32 gNdsUiKitEngine;
/* Pack load: bytes read, the FNV-1a the load computed, and the mismatch
 * count. A zero-byte load with a clean hash is impossible by construction --
 * the hash is seeded and folded over every byte that reaches the engine. */
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

#endif /* NDS_UI_KIT_H */
