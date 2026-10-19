/* P2-7 item 3. Training mode scene, source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1ptrainingmode.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with its taskman
 * setup and start/update/draw functions, the scene entry imported as ndsBase*
 * and re-exported under its source name, so a later measured DS arena
 * rebudget has a seam and the diff stays reviewable). The adapter is a
 * verbatim pass-through; no behaviour invented here.
 *
 * There is no sc1ptrainingmodefiles.c in decomp (verified: only
 * sc1ptrainingmode.c, sc1pbonusstagefiles.c-style split does not exist for
 * training; sc1PTrainingModeSetupFiles lives in this same TU at :1887), so
 * no battleship_sc1ptrainingmodefiles.c companion is needed.
 *
 * The training menu overlay (pause menu, CP/Item/Speed/View/Reset/Exit rows,
 * combo/damage readouts) lives in this same TU; the training CHARACTER
 * SELECT (writes training_man/com fkind+costume at mnplayers1ptraining.c:
 * 2913-2917) and the training STAGE pick (maps_training_gkind at mnmaps.c:
 * 1397) are separate scene TUs -- the former is battleship_mntraining.c
 * beside this file, the latter rides the existing battleship_mnmaps.c import.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md TRAINING rows):
 * - menu 6 rows + main enum CP/Item/Speed/View/Reset/Exit (:52-60,
 *   decomp sc/scdef.h:399-415); CP opts Stand/Walk/Evade/Jump/Attack
 *   (:63-70, scdef.h:419-429) mapping to nFTComputerBehavior* (ftdef.h).
 * - battle setup: game_type Training, time INFINITE, show_score FALSE,
 *   items 0 (:587-591); slots 1 MAN + 1 COM level 3 (:593-615).
 * - item spawn max 4, vel.y 30, y+200, wait 8, A-button (:393-406,
 *   scdef.h:97-100); speed Full/2Thirds/Half/Quarter (:416-430,
 *   scdef.h:455-464); view CloseUp/Normal in source order, magnify_wait
 *   180 (:433-457, scdef.h:92,466-473); damage 3-digit + combo 2-digit
 *   (:790,:874, scdef.h:77-82); Reset/Exit via A-button reload (:461-489).
 *
 * Gated on NDS_P2_1P_GAME: the Makefile defines no NDS_P2_TRAINING flag
 * (verified 2026-09-05: only NDS_P2_1P_GAME at Makefile:700), so this rides
 * the campaign flag like the P2-6 step 5 bonus-stage TU and the P2-7 item 5
 * menu imports until P2-7 mints its own gate.
 *
 * Shims vs unresolved, checked against port headers (an earlier
 * revision of this comment claimed local shims; there are none -- the port
 * headers below carry everything, and a duplicate here would not compile):
 * - Training menu enums (Main/CP/Item/Speed/View/MenuOptionSprites, decomp
 *   sc/scdef.h:399-524): port-PROVIDED, include/sc/scene.h:410-530
 *   verbatim.
 * - SC1PTrainingModeSprites / SC1PTrainingModeFiles / SC1PTrainingModeMenu
 *   (decomp sc/sctypes.h:117-183): port-PROVIDED,
 *   include/sc/scene.h:533-594 verbatim (field order identical).
 * - nSYAudioBGMTrainingMode (= 42) and nSYAudioFGMTrainingSel2 (= 162):
 *   port-PROVIDED, include/gm/gmsound.h:66,484. No value macros here.
 * - itManagerMakeItemSetupCommon (:403, port include/it/item.h:1009):
 *   provided by battleship_item_link_core.c behind NDS_P2_ITEM_CORE,
 *   which the campaign's required fighter flags enable in the Makefile.
 * - ll* rows: NONE unresolved. dSC1PTrainingModeWallpaperDescs (:79-84)
 *   needs the 3 training wallpaper FileID/Sprite pairs (port
 *   include/reloc_data.h:652-676) and sc1PTrainingModeLoadSprites
 *   (:639-649) needs llSC1PTrainingModeFileID plus its 6 sprite-array
 *   rows (port include/reloc_data.h:682-691).
 * - sc1PTrainingModeLoadWallpaper (:652, called by grwallpaper.c:271):
 *   defined here (strong); battleship_grwallpaper.c:12 only declares it
 *   and src/port/battle_playable_compat_stubs.c:137 carries a WEAK stub,
 *   so the strong def wins with no edit needed there.
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): sc1PTrainingModeStartScene (adapter below) vs
 *   src/port/title_backend.c:489 NDS_SCENE_STUB, already gated
 *   #if !NDS_P2_1P_GAME, so this TU wins exactly when the flag is on.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <ft/ftcomputer.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <mn/menu.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_native_wallpaper.h>
#include <nds/nds_p5.h>
#if NDS_P5
#include "nds_p5_preview.generated.h"
#endif

/* Port headers declare no gmCamera makers (decomp gm/gmcamera.h:53-83);
 * same extern pattern as battleship_scvsbattle.c:42-61. */
extern sb32 (*dLBCommonFuncMatrixList[])(void);
void gmCameraSetStatusDefault(void);
void gmCameraSetStatusPlayerZoom(GObj *fighter_gobj, f32 eye_x, f32 eye_y, f32 dist, f32 pan_scale, f32 fov);
void gmCameraSetViewportDimensions(s32 ulx, s32 uly, s32 lrx, s32 lry);
GObj *gmCameraMakeWallpaperCamera(void);
void gmCameraMakeBattleCamera(void);
void gmCameraMakePlayerArrowsCamera(void);
void gmCameraMakePlayerMagnifyCamera(void);
void gmCameraScreenFlashMakeCamera(void);
GObj *gmCameraMakeInterfaceCamera(void);
GObj *gmCameraMakeEffectCamera(void);
void gmCameraRunFuncCamera(GObj *camera_gobj);
void grWallpaperMakeDecideKind(void);
void gmRumbleMakeActor(void);
void gmRumbleInitPlayers(void);
void wpManagerAllocWeapons(void);
void efManagerInitEffects(void);

/* Source declarations normally supplied by sc/sc1pmode/sc1ptrainingmode.h.
 * The decomp TU uses them before definition (menu update table :52-60,
 * taskman setup :112-148); same narrow-prototype pattern as
 * battleship_sc1pbonusstage.c. */
sb32 sc1PTrainingModeUpdateCPOption(void);
sb32 sc1PTrainingModeUpdateItemOption(void);
sb32 sc1PTrainingModeUpdateSpeedOption(void);
sb32 sc1PTrainingModeUpdateViewOption(void);
sb32 sc1PTrainingModeUpdateResetOption(void);
sb32 sc1PTrainingModeUpdateExitOption(void);
void sc1PTrainingModeFuncUpdate(void);
void sc1PTrainingModeFuncStart(void);
void sc1PTrainingModeFuncLights(Gfx **dls);
void sc1PTrainingModeSetupFiles(void);
void sc1PTrainingModeUpdateOptionArrows(void);
void sc1PTrainingModeUpdateUnderline(void);
void sc1PTrainingModeUpdateDummyBehavior(void);
void sc1PTrainingModeUpdateCPDisplaySprite(void);
void sc1PTrainingModeUpdateCPOptionSprite(void);
void sc1PTrainingModeUpdateItemOptionSprite(void);
void sc1PTrainingModeUpdateSpeedDisplaySprite(void);
void sc1PTrainingModeUpdateSpeedOptionSprite(void);
void sc1PTrainingModeUpdateViewOptionSprite(void);
void sc1PTrainingModeUpdateCursorPosition(void);

#define sc1PTrainingModeStartScene ndsBaseSC1PTrainingModeStartScene
void ndsBaseSC1PTrainingModeStartScene(void);

/* P7 C4 (2026-10-18). Training draws its backdrop natively and reads its
 * per-ground tables by the stage's Training parent.
 *  - sc1PTrainingModeLoadWallpaper (called by grwallpaper.c:271) force-loaded
 *    the backdrop's container over the map's wallpaper, 156 KB of the scene
 *    heap (reloc_backend_assets.c gave a compact map that region), and with
 *    two heavy contents the second fighter's allocation halted (walk-tr3,
 *    ndsSyMallocOverflowHalt). It now names the backdrop: the static
 *    wallpaper keeps the map's Sprite for its placement and draws the
 *    converted image (generate_native_wallpapers.py training_*,
 *    sprite_preview_backend.c).
 *  - dSC1PTrainingModeWallpaperIDs and its heap offsets have the source's
 *    nine grounds; a P5 stage reads them through Remix's background_table
 *    parent (Stages.asm training_id_fix_, patched at the start of
 *    sc1PTrainingModeInitDisplayVars), as the fog colour there does. The
 *    display-vars definition keeps the source body under a base name; its
 *    call (no argument) reaches the port's copy. */
#define sc1PTrainingModeLoadWallpaper ndsBaseSC1PTrainingModeLoadWallpaper
void ndsBaseSC1PTrainingModeLoadWallpaper(void);
#define NDS_TRAINING_DISPLAY_VARS_void ndsBaseSC1PTrainingModeInitDisplayVars(void
#define NDS_TRAINING_DISPLAY_VARS_ ndsSC1PTrainingModeInitDisplayVars(
#define sc1PTrainingModeInitDisplayVars(arg) NDS_TRAINING_DISPLAY_VARS_##arg)
static void ndsSC1PTrainingModeInitDisplayVars(void);
/* The sprite file's four position-and-sprite arrays hold each position as
 * two s16 in one word, and the loader's word byteswap leaves the pair
 * exchanged (a label's y read as its x: the menu's ten labels stacked on one
 * row). The load keeps the source body under a base name; its call reaches
 * the port's copy, which swaps the pairs back. */
#define NDS_TRAINING_LOAD_SPRITES_void ndsBaseSC1PTrainingModeLoadSprites(void
#define NDS_TRAINING_LOAD_SPRITES_ ndsSC1PTrainingModeLoadSprites(
#define sc1PTrainingModeLoadSprites(arg) NDS_TRAINING_LOAD_SPRITES_##arg)
static void ndsSC1PTrainingModeLoadSprites(void);
/* The menu's box and the cursor's underline are display callbacks that write
 * fill rectangles (sc1PTrainingModeMenuProcDisplay :1090,
 * sc1PTrainingModeUnderlineProcDisplay :1642); the port's copies below hand
 * the rectangles to the native battle presenter
 * (nds_ifcommon_oam.c ndsIFCommonTrainingFillRect). Only the definitions are
 * renamed (a function-like macro): the scene's gcAddGObjDisplay references
 * name the port's. */
void sc1PTrainingModeMenuProcDisplay(GObj *interface_gobj);
void sc1PTrainingModeUnderlineProcDisplay(GObj *interface_gobj);
#define sc1PTrainingModeMenuProcDisplay(arg)     ndsBaseSC1PTrainingModeMenuProcDisplay(arg)
#define sc1PTrainingModeUnderlineProcDisplay(arg)     ndsBaseSC1PTrainingModeUnderlineProcDisplay(arg)

/* Training shows none of IFCommonAnnounceCommon's letters
 * (battleship_sc1pgame_runtime.c ndsSC1PLoadCommonFiles). */
void ndsSC1PLoadCommonFiles(sb32 keep_letters);
#pragma push_macro("lbRelocLoadFilesListed")
#undef lbRelocLoadFilesListed
#define lbRelocLoadFilesListed(file_ids, out_ptrs) ndsSC1PLoadCommonFiles(FALSE)
#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1ptrainingmode.c"
#pragma pop_macro("lbRelocLoadFilesListed")

#undef sc1PTrainingModeUnderlineProcDisplay
#undef sc1PTrainingModeMenuProcDisplay

#undef sc1PTrainingModeLoadSprites
#undef sc1PTrainingModeInitDisplayVars
#undef sc1PTrainingModeLoadWallpaper
#undef sc1PTrainingModeStartScene

/* The ground whose Training rows this battle reads. */
static s32 ndsSC1PTrainingModeTableGkind(void)
{
    s32 gkind = gSCManagerBattleState->gkind;

#if NDS_P5
#define NDS_TRAINING_PARENT_ROW(g, parent) \
    if (gkind == (s32)(g)) { gkind = (s32)(parent); }
    NDS_P5_TRAINING_ROWS(NDS_TRAINING_PARENT_ROW)
#undef NDS_TRAINING_PARENT_ROW
#endif
    return ((u32)gkind < ARRAY_COUNT(dSC1PTrainingModeWallpaperIDs)) ?
        gkind : (s32)nGRKindPupupu;
}

u32 gNdsTrainingWallpaperAsset;

void sc1PTrainingModeLoadWallpaper(void)
{
    /* dSC1PTrainingModeWallpaperDescs' order is the files': Black 0x1A,
     * Yellow 0x1B, Blue 0x1C, the converted images' registry ids. */
    gNdsTrainingWallpaperAsset = NDS_TRAINING_WALLPAPER_ASSET_BLACK +
        (u32)dSC1PTrainingModeWallpaperIDs[ndsSC1PTrainingModeTableGkind()];
}

void sc1PTrainingModeMenuProcDisplay(GObj *interface_gobj)
{
    (void)interface_gobj;
    /* :1094-1097: PRIM 00/64/FF at alpha 0x64 under G_RM_CLD_SURF. */
    ndsIFCommonTrainingFillRect(68, 47, 253, 198, 0x0064FFu, 0x64u);
}

void sc1PTrainingModeUnderlineProcDisplay(GObj *interface_gobj)
{
    (void)interface_gobj;
    /* :1646-1648: opaque red, FILL cycle. */
    ndsIFCommonTrainingFillRect(sSC1PTrainingModeMenu.cursor_ulx,
                                sSC1PTrainingModeMenu.cursor_uly,
                                sSC1PTrainingModeMenu.cursor_lrx,
                                sSC1PTrainingModeMenu.cursor_lry,
                                0xFF0000u, 0xFFu);
}

static void ndsSC1PTrainingModeSwapPositions(SC1PTrainingModeSprites *rows,
                                             u32 count)
{
    u32 i;

    for (i = 0u; i < count; i++)
    {
        s16 x = rows[i].pos.y;

        rows[i].pos.y = rows[i].pos.x;
        rows[i].pos.x = x;
    }
}

static void ndsSC1PTrainingModeLoadSprites(void)
{
    ndsBaseSC1PTrainingModeLoadSprites();
    /* The rows the scene reads: the stat labels (4, :692), the menu labels
     * (10, :1060), and the two option tables (6, :1473; 28, :1508). */
    ndsSC1PTrainingModeSwapPositions(sSC1PTrainingModeMenu.display_label_sprites, 4u);
    ndsSC1PTrainingModeSwapPositions(sSC1PTrainingModeMenu.menu_label_sprites, 10u);
    ndsSC1PTrainingModeSwapPositions(sSC1PTrainingModeMenu.unk_trainmenu_0x34, 6u);
    ndsSC1PTrainingModeSwapPositions(sSC1PTrainingModeMenu.unk_trainmenu_0x38, 28u);
}

static void ndsSC1PTrainingModeInitDisplayVars(void)
{
    gMPCollisionGroundData->fog_color = dSC1PTrainingModeWallpaperDescs[
        dSC1PTrainingModeWallpaperIDs[ndsSC1PTrainingModeTableGkind()]].fog_color;
    ifCommonPlayerMagnifyMakeInterface();
    gIFCommonPlayerInterface.is_magnify_display = TRUE;
}

void sc1PTrainingModeStartScene(void)
{
    ndsBaseSC1PTrainingModeStartScene();
    /* The menu box's blend ends with the scene (Exit or Reset from the menu
     * leaves while it shows). */
    ndsIFCommonTrainingEnd();
}

#endif /* NDS_P2_1P_GAME */
