/* P2-7 item 2. Newcomer-unlock message scene, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mncommon/mnmessage.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md item 2):
 * - Bits are decomp lb/lbdef.h:134-140, applied at mnmessage.c:284-301:
 *   unlock_mask |= (1 << id), fighter_mask |= (1 << fkind) plus
 *   characters_fkind = fkind for the four newcomers (Luigi/Ness/Captain/
 *   Purin via fkinds[] :287), then lbBackupWrite() :301.
 * - Challenger-approaches vs unlocked screens are the same TU: wallpaper
 *   (collage :68-80), tint (:83-101), exclaim (:104-119), per-unlock message
 *   sprite (:122-171, 7 offsets :128-137 + US positions :142-149), four
 *   cameras (:174-271), queue drain in mnMessageStartScene (:409-462, one
 *   task per queued unlock_message, VSResults exit routes to PlayersVS
 *   :448-452 else Startup (US) :456-458).
 * - mnMessageFuncRun :305-325: 120-tic hold, stick-centred reset of the
 *   dead-up-star wait, A/B/START applies the unlock and loads the scene.
 *
 * There is no battleship_mnmessagefiles.c: decomp mn/mncommon/ holds only
 * mnmessage.c + mnmessage.h (no mnmessagefiles.c exists there), and the
 * single TU already carries its file list (dMNMessageFileIDs :15), status
 * buffer (:50) and setup (FuncStart :328-358). Unlike the bonus stage,
 * nothing splits off.
 *
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05, same gate as battleship_mnoption.c /
 * battleship_mnbackupclear.c / battleship_mnsoundtest.c), so this rides the
 * campaign flag until P2-7 mints its own gate.
 *
 * Shell status: same as the Options/SoundTest imports -- the native shell
 * cannot reach nSCKindMessage today and item 9 (Menu completion) wires it,
 * not this slice. Stops at the import by design.
 *
 * Shims vs unresolved, see handoff report:
 * - sc1PManagerCheckUnlockSoundTest (called :429, REGION_JP arm only):
 *   shimmed below as a local extern; PROVIDED by the sibling TU
 *   battleship_sc1pmanager.c behind the same gate (whole-TU include of
 *   sc1pmanager.c, which defines it at :189). On REGION_US the call
 *   compiles out and the extern is harmless.
 * - nSYAudioBGMMessage (played :356): in include/gm/gmsound.h since the
 *   2026-09-05 widening. nSYAudioFGMDeadUpStar (:357) IS carried
 *   (port gmsound.h:60) and func_800269C0_275C0 rides include/sys/audio.h:92.
 * - ll* rows: NONE unresolved. dMNMessageFileIDs needs llMNCommonFileID +
 *   llMNMessageFileID (both declared: reloc_data.h:156 + :1207); the
 *   wallpaper needs llMNCommonSmashBrosCollageSprite (:178); the exclaim +
 *   7 unlock sprites are the staged NDS_MN_MESSAGE_RELOC_SYMBOLS block
 *   (:1209-1221, file 0x9 reloc_menus). No manifest staging outstanding.
 * - Resolved port-side, no action: gcMakeGObjSPAfter / gcAddGObjDisplay /
 *   gcRunAll / gcDrawAll (sys/objman.h), gcMakeCameraGObj /
 *   gcMakeDefaultCameraGObj (sys/objhelper.h), lbCommonDrawSObjAttr /
 *   lbCommonMakeSObjForGObj / lbCommonClearExternSpriteParams /
 *   lbCommonDrawSprite (mn/menu.h, owned by battleship_lb_common.c),
 *   lbRelocGetFileData / lbRelocInitSetup / lbRelocLoadFilesListed +
 *   lLBRelocTableAddr / llRelocFileCount (reloc_data.h),
 *   syTaskmanStartTask / syTaskmanSetLoadScene (sys/taskman.h), syVideoInit
 *   + SYVIDEO_SETUP_DEFAULT (sys/video.h), syRdpSetViewport (sys/rdp.h),
 *   syControllerFuncRead (sys/controller.h), syAudioPlayBGM (sys/audio.h),
 *   scSubsysControllerGetPlayerTapButtons /
 *   scSubsysControllerGetPlayerStickInRangeLR/UD (sc/scene.h:561-567),
 *   lbBackupWrite (mn/menu.h + battleship_lbbackup.c), gSCManagerSceneData
 *   / gSCManagerBackupData + nLBBackupUnlock* + SCKind + ovl1_VRAM /
 *   ovl22_BSS_END (sc/scene.h), nFTKindLuigi/Ness/Captain/Purin (ft/
 *   fighter.h).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnMessageStartScene (adapter below) vs
 *   src/port/title_backend.c:419 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <gm/gmsound.h>
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

/* decomp sc/sc1pmode/sc1pmanager.c:189. Defined by the sibling TU
 * battleship_sc1pmanager.c behind this same gate; only called here on the
 * REGION_JP arm (mnmessage.c:429), so on REGION_US this extern binds
 * nothing and costs nothing. */
sb32 sc1PManagerCheckUnlockSoundTest(void);

#define mnMessageStartScene ndsBaseMNMessageStartScene
void ndsBaseMNMessageStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mncommon/mnmessage.c"

#undef mnMessageStartScene

void mnMessageStartScene(void)
{
    ndsBaseMNMessageStartScene();
}

#endif /* NDS_P2_1P_GAME */
