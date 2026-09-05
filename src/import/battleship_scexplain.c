/* P2-7 item 6. How to Play screen, source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sccommon/scexplain.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with its taskman
 * /video setup and start/update/draw functions, scene entry imported as
 * ndsBase* and re-exported under its source name, so a later measured DS
 * arena rebudget has a seam and diff stays reviewable). Adapter is verbatim
 * pass-through; no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md ATTRACT rows):
 * - scexplain scene + voice announce nSYAudioVoiceAnnounceHowToPlay (:794).
 * - battle state: game_type Explain, gkind nGRKindExplain, pl_count 2,
 *   Mario + Luigi both pkind GameKey (:151-169); fighters get scripted
 *   FTKeyEvent streams dSCExplainKeyKeyEventss (:760-769).
 * - phase machine 0..22 then scene_curr = nSCKindCharacters (:621-657);
 *   Fire Flower spawn at phase 16 (:602-618); exit on A/B/START tap back to
 *   nSCKindTitle (:581-599).
 * - reached from title idle via mnTitleProceedDemoNext default arm
 *   (mntitle.c:481-483: scene_curr = nSCKindExplain).
 *
 * Gated on NDS_P2_1P_GAME: Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05: only NDS_P2_1P_GAME at Makefile:700, gate block at
 * :4042), so rides campaign flag like P2-6 step 5 bonus-stage TU until P2-7
 * mints own gate.
 *
 * Shell status: no native HowToPlay module; title shell has no idle timer
 * (see sibling battleship_scautodemo.c header). Wiring is later slice, not
 * this import.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - SCExplainMain / SCExplainArgs / SCExplainPhase: shimmed below, verbatim
 *   from decomp sc/sctypes.h:256-296. Port include/sc/scene.h carries
 *   SCBattleState/SCCommonData but none of the explain structs. Guarded;
 *   delete when header gains them.
 * - Port headers declare no gmCamera makers (decomp gm/gmcamera.h:53-83);
 *   same extern pattern as battleship_sc1ptrainingmode.c:300-314.
 *   Definitions live in battleship_gmcamera.c whole-TU import;
 *   dLBCommonFuncMatrixList defined in src/port/
 *   reloc_backend_fighter_display_seam.c:90.
 * - scExplainSetupFiles (:696, decl scexplain.h:44, def scexplainfiles.c:23):
 *   left UNRESOLVED. Companion TU not in scope (only these two files
 *   allowed); stubbing would invent file setup.
 * - ll* rows: NONE unresolved. llSCExplainMainFileID + 5 rows
 *   (include/reloc_data.h:511-519) and llSCExplainGraphicsFileID + ~36 rows
 *   (:526-565: stick/dobj/mobj/mat-anims, tap-spark, special-move RGB,
 *   A/B/Z buttons, HereText, PlusSymbol, TapTheStick, all textbox sprites)
 *   cover every llSCExplain* the TU names.
 * - Resolved port-side, no action: nGRKindExplain + nSCBattleGameTypeExplain
 *   + nSCKindTitle/nSCKindCharacters (include/sc/scene.h:90-91,160,427),
 *   nFTKindMario/nFTKindLuigi + nFTPlayerKindGameKey/Not
 *   (include/ft/fighter.h:113-117), nFTCameraModeExplain (same:2169),
 *   nITKindFFlower (include/it/item.h),
 *   nSYAudioVoiceAnnounceHowToPlay (include/gm/gmsound.h:734),
 *   func_800266A0_272A0/func_800269C0_275C0 (include/sys/audio.h:99-101),
 *   func_80017EC0 (include/sys/objhelper.h:92),
 *   mpCollisionGetPlayerMapObjPosition (src/port/
 *   reloc_backend_compat_shims.c:17652; decl battleship_gmcamera.c:59),
 *   itManagerMakeItemSetupCommon (include/it/item.h:1009, defined by
 *   battleship_item_link_core.c behind NDS_P2_ITEM_CORE -- same cross-gate
 *   shape as training TU riding item core; recorded, not shimmed),
 *   ovl63_BSS_END (DECLARE_OVL in include/sc/scene.h:588-604).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   scExplainStartScene (adapter below) vs
 *   src/port/title_backend.c:490 NDS_SCENE_STUB.
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

/* decomp sc/sctypes.h:256-296 verbatim. Port include/sc/scene.h carries
 * SCBattleState/SCCommonData but none of the explain structs. */
#ifndef NDS_SCEXPLAIN_TYPES_DEFINED
#define NDS_SCEXPLAIN_TYPES_DEFINED 1
typedef struct SCExplainMain
{
    SObj *textbox_sobj;
    GObj *stick_gobj;
    GObj *spark_gobj;
    GObj *rgb_gobj;
    SObj *phase_sobj0;
    SObj *phase_sobj1;
    SObj *phase_sobj2;
    SObj *phase_sobj3;
    SObj *phase_sobj4;
    SObj *phase_sobj5;
    s32 phase_advance_wait;
    s32 phase;
    u8 unk_scexplainif_0x30;
    u8 stick_status;
} SCExplainMain;

typedef struct SCExplainArgs
{
    u16 sprite_pos_x;
    u8 sprite_pos_y;
    u8 sprite_status;
} SCExplainArgs;

typedef struct SCExplainPhase
{
    u16 phase_time;
    u16 unused;
    u8 textbox_pos_x;
    u8 textbox_pos_y;
    Sprite *sprite;
    SCExplainArgs control_stick_args;
    SCExplainArgs phase_args0;
    SCExplainArgs phase_args1;
    SCExplainArgs phase_args2;
    SCExplainArgs phase_args3;
    SCExplainArgs phase_args4;
    SCExplainArgs rgb_overlay_args;
    SCExplainArgs phase_args5;
} SCExplainPhase;
#endif

/* Port headers declare no gmCamera makers (decomp gm/gmcamera.h:53-83);
 * same extern pattern as battleship_sc1ptrainingmode.c:300-314. */
extern sb32 (*dLBCommonFuncMatrixList[])(void);
void gmCameraSetViewportDimensions(s32 ulx, s32 uly, s32 lrx, s32 lry);
GObj *gmCameraMakeWallpaperCamera(void);
void gmCameraMakeBattleCamera(void);
void gmCameraMakePlayerArrowsCamera(void);
void gmCameraMakePlayerMagnifyCamera(void);
void gmCameraScreenFlashMakeCamera(void);
GObj *gmCameraMakeInterfaceCamera(void);
GObj *gmCameraMakeEffectCamera(void);
void grWallpaperMakeDecideKind(void);
void gmRumbleMakeActor(void);
void gmRumbleInitPlayers(void);
/* Companion TU scexplainfiles.c owns this; declared here so the call site
 * (:696) reads honestly as unresolved at link, not invented. */
void scExplainSetupFiles(void);

#define scExplainStartScene ndsBaseSCExplainStartScene
void ndsBaseSCExplainStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scexplain.c"

#undef scExplainStartScene

void scExplainStartScene(void)
{
    ndsBaseSCExplainStartScene();
}

#endif /* NDS_P2_1P_GAME */
