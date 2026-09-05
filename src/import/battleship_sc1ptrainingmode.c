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
 *   scdef.h:455-464); view Normal/CloseUp, magnify_wait 180 (:433-457,
 *   scdef.h:92,466-473); damage 3-digit + combo 2-digit (:790,:874,
 *   scdef.h:77-82); Reset/Exit via A-button reload (:461-489).
 *
 * Gated on NDS_P2_1P_GAME: the Makefile defines no NDS_P2_TRAINING flag
 * (verified 2026-09-05: only NDS_P2_1P_GAME at Makefile:700), so this rides
 * the campaign flag like the P2-6 step 5 bonus-stage TU and the P2-7 item 5
 * menu imports until P2-7 mints its own gate.
 *
 * Shims vs unresolved, see handoff report:
 * - Training menu enums (Main/CP/Item/Speed/View/MenuOptionSprites, decomp
 *   sc/scdef.h:399-524): shimmed below, verbatim, because port
 *   include/sc/scene.h carries none of them. Enum members cannot be
 *   #ifndef-guarded; when the port header gains them, delete this block.
 * - SC1PTrainingModeSprites / SC1PTrainingModeFiles / SC1PTrainingModeMenu
 *   (decomp sc/sctypes.h:117-183): shimmed below, verbatim, guarded by
 *   NDS_SC1PTRAININGMODE_TYPES_DEFINED (port include/sc/scene.h lacks all
 *   three; struct layout can only be completed here, and the header is
 *   owned by another slice).
 * - nSYAudioBGMTrainingMode (decomp gm/gmsound.h:74, ordinal 42 by count
 *   from nSYAudioBGMPupupu = 0; port BGM ordinals match decomp on every
 *   carried name, e.g. BattleSelect = 10 both sides) and
 *   nSYAudioFGMTrainingSel2 (decomp gm/gmsound.h:257, ordinal 162; port
 *   brackets it exactly with StageSelect = 159 and MenuScroll1 = 163):
 *   shimmed below as value macros. Port include/gm/gmsound.h carries
 *   neither; every other audio ID this TU touches (GamePause, MenuScroll2,
 *   MenuSelect, MenuDenied, PublicExcited) is already carried.
 * - itManagerMakeItemSetupCommon (:403, port include/it/item.h:1009):
 *   port-PROVIDED but gated -- defined by battleship_item_link_core.c
 *   behind NDS_P2_ITEM_CORE (Makefile:4106-4108), so a 1P-game-only build
 *   links this TU against nothing. Same cross-gate shape as
 *   battleship_item_target.c riding this TU's symbols; recorded, not
 *   shimmed (a local copy would fork spawn behaviour).
 * - ll* rows: NONE unresolved. dSC1PTrainingModeWallpaperDescs (:79-84)
 *   needs the 3 training wallpaper FileID/Sprite pairs and
 *   sc1PTrainingModeLoadSprites (:641-649) needs llSC1PTrainingModeFileID
 *   plus its 6 sprite-array rows; all are staged in include/reloc_data.h
 *   (:499-537), unlike the bonus-stage TU's 60 missing rows.
 * - sc1PTrainingModeLoadWallpaper (:652, called by grwallpaper.c:271):
 *   defined here (strong); battleship_grwallpaper.c:12 only declares it
 *   and src/port/battle_playable_compat_stubs.c:137 carries a WEAK stub,
 *   so the strong def wins with no edit needed there.
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): sc1PTrainingModeStartScene (adapter below) vs
 *   src/port/title_backend.c:458 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
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

/* decomp sc/scdef.h:399-417 verbatim. Port include/sc/scene.h carries the
 * scene/game-type enums but none of the training menu enums. */
typedef enum SC1PTrainingModeMain
{
    nSC1PTrainingModeMenuMainEnumStart,
    nSC1PTrainingModeMenuMainCP = nSC1PTrainingModeMenuMainEnumStart,
    nSC1PTrainingModeMenuMainScrollStart = nSC1PTrainingModeMenuMainCP,
    nSC1PTrainingModeMenuMainItem,
    nSC1PTrainingModeMenuMainSpeed,
    nSC1PTrainingModeMenuMainView,
    nSC1PTrainingModeMenuMainScrollEnd = nSC1PTrainingModeMenuMainView,
    nSC1PTrainingModeMenuMainReset,
    nSC1PTrainingModeMenuMainExit,
    nSC1PTrainingModeMenuEnumEnd = nSC1PTrainingModeMenuMainExit,
    nSC1PTrainingModeMenuMainEnumCount
} SC1PTrainingModeMain;

/* decomp sc/scdef.h:419-429 verbatim. */
typedef enum SC1PTrainingModeCP
{
    nSC1PTrainingModeMenuCPEnumStart,
    nSC1PTrainingModeMenuCPStand = nSC1PTrainingModeMenuCPEnumStart,
    nSC1PTrainingModeMenuCPWalk,
    nSC1PTrainingModeMenuCPEvade,
    nSC1PTrainingModeMenuCPJump,
    nSC1PTrainingModeMenuCPAttack,
    nSC1PTrainingModeMenuCPEnumCount
} SC1PTrainingModeCP;

/* decomp sc/scdef.h:431-453 verbatim. */
typedef enum SC1PTrainingModeItem
{
    nSC1PTrainingModeMenuItemEnumStart,
    nSC1PTrainingModeMenuItemNone = nSC1PTrainingModeMenuItemEnumStart,
    nSC1PTrainingModeMenuItemMaximTomato,
    nSC1PTrainingModeMenuItemHeart,
    nSC1PTrainingModeMenuItemStar,
    nSC1PTrainingModeMenuItemBeamSword,
    nSC1PTrainingModeMenuItemHomeRunBat,
    nSC1PTrainingModeMenuItemFan,
    nSC1PTrainingModeMenuItemStarRod,
    nSC1PTrainingModeMenuItemRayGun,
    nSC1PTrainingModeMenuItemFireFlower,
    nSC1PTrainingModeMenuItemHammer,
    nSC1PTrainingModeMenuItemMotionSensorBomb,
    nSC1PTrainingModeMenuItemBobomb,
    nSC1PTrainingModeMenuItemBumper,
    nSC1PTrainingModeMenuItemGreenShell,
    nSC1PTrainingModeMenuItemRedShell,
    nSC1PTrainingModeMenuItemPokeBall,
    nSC1PTrainingModeMenuItemEnumCount
} SC1PTrainingModeItem;

/* decomp sc/scdef.h:455-464 verbatim. */
typedef enum SC1PTrainingModeSpeed
{
    nSC1PTrainingModeMenuSpeedEnumStart,
    nSC1PTrainingModeMenuSpeedFull = nSC1PTrainingModeMenuSpeedEnumStart,
    nSC1PTrainingModeMenuSpeed2Thirds,
    nSC1PTrainingModeMenuSpeedHalf,
    nSC1PTrainingModeMenuSpeedQuarter,
    nSC1PTrainingModeMenuSpeedEnumCount
} SC1PTrainingModeSpeed;

/* decomp sc/scdef.h:466-473 verbatim. NOTE the source order: CloseUp is the
 * enum start and Normal second (pin sheet lists them "Normal/CloseUp";
 * the code compares against ViewNormal at :437). */
typedef enum SC1PTrainingModeView
{
    nSC1PTrainingModeMenuViewEnumStart,
    nSC1PTrainingModeMenuViewCloseUp = nSC1PTrainingModeMenuViewEnumStart,
    nSC1PTrainingModeMenuViewNormal,
    nSC1PTrainingModeMenuViewEnumCount
} SC1PTrainingModeView;

/* decomp sc/scdef.h:475-524 verbatim. */
typedef enum SC1PTrainingModeMenuOptionSprites
{
    nSC1PTrainingModeMenuOptionSpriteItemStart,
    nSC1PTrainingModeMenuOptionSpriteItemNone = nSC1PTrainingModeMenuOptionSpriteItemStart,
    nSC1PTrainingModeMenuOptionSpriteItemMaximTomato,
    nSC1PTrainingModeMenuOptionSpriteItemHeart,
    nSC1PTrainingModeMenuOptionSpriteItemStar,
    nSC1PTrainingModeMenuOptionSpriteItemBeamSword,
    nSC1PTrainingModeMenuOptionSpriteItemHomeRunBat,
    nSC1PTrainingModeMenuOptionSpriteItemFan,
    nSC1PTrainingModeMenuOptionSpriteItemStarRod,
    nSC1PTrainingModeMenuOptionSpriteItemRayGun,
    nSC1PTrainingModeMenuOptionSpriteItemFireFlower,
    nSC1PTrainingModeMenuOptionSpriteItemHammer,
    nSC1PTrainingModeMenuOptionSpriteItemMotionSensorBomb,
    nSC1PTrainingModeMenuOptionSpriteItemBobomb,
    nSC1PTrainingModeMenuOptionSpriteItemBumper,
    nSC1PTrainingModeMenuOptionSpriteItemGreenShell,
    nSC1PTrainingModeMenuOptionSpriteItemRedShell,
    nSC1PTrainingModeMenuOptionSpriteItemPokeBall,
    nSC1PTrainingModeMenuOptionSpriteItemEnd = nSC1PTrainingModeMenuOptionSpriteItemPokeBall,

    nSC1PTrainingModeMenuOptionSpriteSpeedStart,
    nSC1PTrainingModeMenuOptionSpriteSpeedFull = nSC1PTrainingModeMenuOptionSpriteSpeedStart,
    nSC1PTrainingModeMenuOptionSpriteSpeed2Thirds,
    nSC1PTrainingModeMenuOptionSpriteSpeedHalf,
    nSC1PTrainingModeMenuOptionSpriteSpeedQuarter,
    nSC1PTrainingModeMenuOptionSpriteSpeedEnd = nSC1PTrainingModeMenuOptionSpriteSpeedQuarter,

    nSC1PTrainingModeMenuOptionSpriteCPStart,
    nSC1PTrainingModeMenuOptionSpriteCPStand = nSC1PTrainingModeMenuOptionSpriteCPStart,
    nSC1PTrainingModeMenuOptionSpriteCPWalk,
    nSC1PTrainingModeMenuOptionSpriteCPEvade,
    nSC1PTrainingModeMenuOptionSpriteCPJump,
    nSC1PTrainingModeMenuOptionSpriteCPAttack,
    nSC1PTrainingModeMenuOptionSpriteCPEnd = nSC1PTrainingModeMenuOptionSpriteCPAttack,

    nSC1PTrainingModeMenuOptionSpriteViewStart,
    nSC1PTrainingModeMenuOptionSpriteViewNormal = nSC1PTrainingModeMenuOptionSpriteViewStart,
    nSC1PTrainingModeMenuOptionSpriteViewCloseUp,
    nSC1PTrainingModeMenuOptionSpriteViewEnd = nSC1PTrainingModeMenuOptionSpriteViewCloseUp,

    nSC1PTrainingModeMenuOptionSpriteIndicatorStart,
    nSC1PTrainingModeMenuOptionSpriteLeftArrow = nSC1PTrainingModeMenuOptionSpriteIndicatorStart,
    nSC1PTrainingModeMenuOptionSpriteRightArrow,
    nSC1PTrainingModeMenuOptionSpriteCursor,

    nSC1PTrainingModeMenuOptionSpriteEnumCount
} SC1PTrainingModeMenuOptionSprites;

/* decomp sc/sctypes.h:117-183 verbatim. Port include/sc/scene.h carries
 * SCBattleState/SCCommonData but none of the training structs. */
#ifndef NDS_SC1PTRAININGMODE_TYPES_DEFINED
#define NDS_SC1PTRAININGMODE_TYPES_DEFINED 1
typedef struct SC1PTrainingModeSprites
{
    Vec2h pos;
    Sprite *sprite;
} SC1PTrainingModeSprites;

typedef struct SC1PTrainingModeFiles
{
    s32 file_id;
    intptr_t offset;
    SYColorRGB fog_color;
} SC1PTrainingModeFiles;

typedef struct SC1PTrainingModeMenu
{
    s32 main_menu_option;
    s32 damage;
    s32 combo;
    s32 item_hold;
    s32 item_menu_option;
    s32 cp_menu_option;
    s32 speed_menu_option;
    s32 view_menu_option;
    s32 dummy;
    SC1PTrainingModeSprites *display_label_sprites;
    Sprite **display_option_sprites;
    SC1PTrainingModeSprites *menu_label_sprites;
    Sprite **menu_option_sprites;
    SC1PTrainingModeSprites *unk_trainmenu_0x34;
    SC1PTrainingModeSprites *unk_trainmenu_0x38;
    GObj *damage_display_gobj;
    GObj *combo_display_gobj;
    GObj *cp_display_gobj;
    GObj *speed_display_gobj;
    GObj *item_display_gobj;
    GObj *menu_label_gobj;
    GObj *cursor_gobj;
    GObj *cp_option_gobj;
    GObj *item_option_gobj;
    GObj *speed_option_gobj;
    GObj *view_option_gobj;
    GObj *arrow_option_gobj;
    SObj *hscroll_option_sobj[4];
    GObj *unk_trainmenu_0x7C;
    GObj *combo0;
    SObj *vscroll_option_sobj[6][2];
    u32 cursor_ulx, cursor_uly;
    u32 cursor_lrx, cursor_lry;
    u16 button_hold;
    u16 button_tap;
    u16 button_queue;
    s32 rapid_scroll_wait;
    u8 damage_reset_wait;
    u8 combo_reset_wait;
    ub8 exit_or_reset;
    u8 lagtic_wait;
    u8 frameadvance_wait;
    u8 item_spawn_wait;
    u16 magnify_wait;
    ub8 is_read_menu_inputs;
    s32 unknown[2];
} SC1PTrainingModeMenu;
#endif

/* Audio ordinals port include/gm/gmsound.h does not carry (values by count
 * in decomp gm/gmsound.h under REGION_US; see file header). Macros, not
 * gameplay stubs: they only select which BGM/SFX ID is requested. */

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

#define sc1PTrainingModeStartScene ndsBaseSC1PTrainingModeStartScene
void ndsBaseSC1PTrainingModeStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1ptrainingmode.c"

#undef sc1PTrainingModeStartScene

void sc1PTrainingModeStartScene(void)
{
    ndsBaseSC1PTrainingModeStartScene();
}

#endif /* NDS_P2_1P_GAME */
