/* P2-7 item 6. Attract demo battle scene, source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sccommon/scautodemo.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with its taskman
 * /video setup and start/update/draw functions, scene entry imported as
 * ndsBase* and re-exported under its source name, so a later measured DS
 * arena rebudget has a seam and diff stays reviewable). Adapter is verbatim
 * pass-through; no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md ATTRACT rows):
 * - demo setup: game_type Demo, stage cycle dSCAutoDemoGroundOrder, all COM
 *   lv9, dmg 0-30 P1/P2 else 40-100 (:546-579, :535-543).
 * - fighters: first 2 from title pick gSCManagerSceneData.demo_fkind, rest
 *   shuffled unlocked (:511-533); title picks 2 via demo_mask_prev
 *   (mntitle.c:302-335); trigger idle 650 tics (1190 if extend_wait)
 *   (mntitle.c:712-723).
 * - NO input recording; scripted CPUvCPU (:566-571).
 * - exit on A/B/START tap back to nSCKindTitle (:224-241); focus timeline
 *   ends via scAutoDemoExit to nSCKindStartup (US) (:384-394).
 *
 * Gated on NDS_P2_1P_GAME: Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05: only NDS_P2_1P_GAME at Makefile:700, gate block at
 * :4042), so rides campaign flag like P2-6 step 5 bonus-stage TU until P2-7
 * mints own gate.
 *
 * Battle path: source does NOT go through gSCManagerTransferBattleState.
 * scAutoDemoInitDemo (:546-579) copies dSCManagerDefaultBattleState into own
 * sSCAutoDemoBattleState and points gSCManagerBattleState at it directly.
 * Port ndsMatchConfigApply (src/port/nds_match_config.c:410) writes
 * gSCManagerTransferBattleState instead. Bridge needed where demo wires to
 * shell/battle: either seed transfer state from demo tables then apply, or
 * teach battle entry to accept gSCManagerBattleState set by InitDemo.
 * Reported, not built here.
 *
 * Shell status: no native attract module; title shell
 * (src/nds/nds_menu_shell_router.c ndsMenuShellRunTitle :445) has NO idle
 * timer. Wiring is later slice, not this import.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - SCAutoDemoProc { focus_end_wait; func_change; func_focus }: shimmed
 *   below, verbatim from decomp sc/sctypes.h:298-303. Port
 *   include/sc/scene.h carries SCBattleState/SCCommonData but none of the
 *   autodemo/explain structs. Guarded; delete when header gains it.
 * - nMPMapObjKindAutoDemoPlayer1..8 (0x18-0x1F): shimmed below as value
 *   macros, verbatim from decomp mp/mpdef.h:107-114. Port
 *   include/gr/ground.h MPMapObjKind ends at MoviePlayer3 then Rebirth=0x20,
 *   no autodemo rows. Enum members cannot be #ifndef-guarded; macros keep
 *   source expressions compiling with source values. Owning home is
 *   include/gr/ground.h (reported follow-up).
 * - Port headers declare no gmCamera makers (decomp gm/gmcamera.h); same
 *   extern pattern as battleship_sc1ptrainingmode.c:300-314. Definitions
 *   live in battleship_gmcamera.c whole-TU import; dLBCommonFuncMatrixList
 *   defined in src/port/reloc_backend_fighter_display_seam.c:90.
 * - scAutoDemoSetupFiles (:629, decl scautodemo.h:36, def
 *   scautodemofiles.c:23): left UNRESOLVED. Companion TU not in scope
 *   (only these two files allowed); stubbing would invent file setup.
 * - ll* rows: NONE unresolved. dSCAutoDemoFighterNameSpriteOffsets (:106)
 *   needs llCharacterNamesFileID + 12 llCharacterNames*Sprite rows; all
 *   staged in include/reloc_data.h:460-474.
 * - Resolved port-side, no action: nGRKindPupupu/Zebes/Castle/Jungle/Sector/
 *   Yoster/Yamabuki/Hyrule (include/sc/scene.h:413-421),
 *   nSCBattleGameTypeDemo + nSCKindTitle/nSCKindStartup/nSCKindAutoDemo
 *   (same header), LBBACKUP_CHARACTER_MASK_STARTER + LBBACKUP_MASK_FIGHTER
 *   (include/ft/fighter.h:143-162), gSCManagerSceneData demo_* +
 *   is_extend_demo_wait (include/sc/scene.h:566-579),
 *   nFTPlayerKindCom (include/ft/fighter.h:113),
 *   nFTPartsDetailHigh/Low + detail_base + closeup_camera_zoom
 *   (include/ft/fighter.h:188-191,3654,4089), is_magnify_display
 *   (include/if/interface.h:105), mpCollisionGetMapObjCountKind/
 *   GetMapObjIDsKind/GetMapObjPositionID (src/port/
 *   reloc_backend_compat_shims.c:17539-17650),
 *   nSYAudioVoicePublicExcited (include/gm/gmsound.h:704),
 *   func_800266A0_272A0/func_800269C0_275C0 (include/sys/audio.h:99-101),
 *   ovl64_BSS_END (DECLARE_OVL in include/sc/scene.h:588-604).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   scAutoDemoStartScene (adapter below) vs
 *   src/port/title_backend.c:489 NDS_SCENE_STUB.
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

/* decomp sc/sctypes.h:298-303 verbatim. Port include/sc/scene.h carries
 * SCBattleState/SCCommonData but not this struct. */
#ifndef NDS_SCAUTODEMO_PROC_DEFINED
#define NDS_SCAUTODEMO_PROC_DEFINED 1
typedef struct SCAutoDemoProc
{
    u16 focus_end_wait;
    void (*func_change)(void);
    void (*func_focus)(void);
} SCAutoDemoProc;
#endif

/* decomp mp/mpdef.h:107-114 verbatim values. Port include/gr/ground.h has no
 * autodemo rows; macros (not enum members) so source expressions keep source
 * values until header promotes them. */
#ifndef nMPMapObjKindAutoDemoPlayer1
#define nMPMapObjKindAutoDemoPlayer1 0x18
#endif
#ifndef nMPMapObjKindAutoDemoPlayer2
#define nMPMapObjKindAutoDemoPlayer2 0x19
#endif
#ifndef nMPMapObjKindAutoDemoPlayer3
#define nMPMapObjKindAutoDemoPlayer3 0x1A
#endif
#ifndef nMPMapObjKindAutoDemoPlayer4
#define nMPMapObjKindAutoDemoPlayer4 0x1B
#endif
#ifndef nMPMapObjKindAutoDemoPlayer5
#define nMPMapObjKindAutoDemoPlayer5 0x1C
#endif
#ifndef nMPMapObjKindAutoDemoPlayer6
#define nMPMapObjKindAutoDemoPlayer6 0x1D
#endif
#ifndef nMPMapObjKindAutoDemoPlayer7
#define nMPMapObjKindAutoDemoPlayer7 0x1E
#endif
#ifndef nMPMapObjKindAutoDemoPlayer8
#define nMPMapObjKindAutoDemoPlayer8 0x1F
#endif

/* Port headers declare no gmCamera makers (decomp gm/gmcamera.h:53-83);
 * same extern pattern as battleship_sc1ptrainingmode.c:300-314. */
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
void grWallpaperMakeDecideKind(void);
void gmRumbleMakeActor(void);
void gmRumbleInitPlayers(void);
/* Companion TU scautodemofiles.c owns this; declared here so the call site
 * (:629) reads honestly as unresolved at link, not invented. */
void scAutoDemoSetupFiles(void);

#define scAutoDemoStartScene ndsBaseSCAutoDemoStartScene
void ndsBaseSCAutoDemoStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scautodemo.c"

#undef scAutoDemoStartScene

void scAutoDemoStartScene(void)
{
    ndsBaseSCAutoDemoStartScene();
}

#endif /* NDS_P2_1P_GAME */
