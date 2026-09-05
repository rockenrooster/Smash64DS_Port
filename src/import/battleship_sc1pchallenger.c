/* P2-6 step 8. Challenger-approaching screen (the unlocked fighter rotates in).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pchallenger.c whole
 * (381 lines: dSC1PChallengerFileIDs :15, lights, decals, rotating-fighter
 * maker + camera makers, FuncRun/FuncStart, sc1PChallengerStartScene :374),
 * following battleship_sc1pintro.c (scene TU with its taskman/video setup
 * and start/update/draw functions), NOT a data-only transcription.
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c, followed
 * here): the include OWNS every symbol it defines under its source name --
 * no renamed private copies, no ndsExcluded* duplicates. The only rename is
 * the scene entry, imported as ndsBase* and re-exported under its source
 * name (the battleship_sc1pbonusstage.c / battleship_scvsbattle.c seam),
 * so a later measured DS arena rebudget has a seam and the diff stays
 * reviewable. The adapter is a verbatim pass-through; no behaviour invented
 * here. Gated on NDS_P2_1P_GAME like the ladder tables and the runtime.
 *
 * Reloc files (dSC1PChallengerFileIDs :15):
 * - SC1PChallenger 0xa: STAGED (llSC1PChallengerFileID in
 *   src/port/diagnostics_mp_taskman_state.c:479, manifest rows in
 *   include/reloc_data.h:1395-1401, asset in src/port/reloc_backend_assets.c,
 *   NitroFS in NDS_1P_RELOC_FILES). All 4 sprites the TU draws
 *   (DecalExclaim, WarningText, ChallengerText, ApproachingText) are staged.
 *   No file the scene loads is unstaged.
 *
 * Fighter-side symbols the challenger resolves (all port-provided, no action):
 * ftManagerMakeFighter (battleship_ftmanager.c:94),
 * ftManagerSetupFilesAllKind (battleship_ftmanager.c:80),
 * ftManagerAllocFighter + gFTManagerFigatreeHeapSize
 * (reloc_backend_compat_shims.c:1639), dFTManagerDefaultFighterDesc
 * (include/ft/fighter.h:4158), ftParamGetCostumeCommonID
 * (reloc_backend_compat_shims.c:16271), ftParamCheckSetFighterColAnimID
 * (reloc_backend_compat_shims.c:1983), dSCSubsysFighterScales
 * (reloc_backend_fighter_display_seam.c:93). The challenger shows one fighter
 * (sSC1PChallengerFighterKind from gSCManagerSceneData.challenger_fkind); its
 * production data is the same manifest that backs the intro (ftchar_data_slots.c
 * slots + reloc_backend_ftdata_symbols.c), so nothing new is staged here. No
 * camera AnimJoint is consumed: both cameras are fixed (fighter cam :232-267,
 * decals cam :270-292), unlike the intro's AObj camera path.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - No struct shim: the TU needs no FTDemoDesc and no bonus/tally types.
 * - BGM enumerator the TU names but the port header lacks (separate task widens
 *   it -- listed, NOT defined here): nSYAudioBGM1PChallenger (:369). Its numeric
 *   value can only come from the gm/gmsound.h promotion. nSYAudioFGMDeadUpStar
 *   (:370) is already in port include/gm/gmsound.h:60.
 * - Everything else the TU calls is port-provided, no action:
 *   lbRelocGetFileData / lbRelocInitSetup / lbRelocLoadFilesListed,
 *   gcMakeGObjSPAfter / gcAddGObjDisplay / gcAddGObjProcess /
 *   gcMakeCameraGObj / gcMakeDefaultCameraGObj / gcRunAll / gcDrawAll,
 *   lbCommonMakeSObjForGObj / lbCommonDrawSObjAttr / lbCommonDrawSprite /
 *   lbCommonClearExternSpriteParams, scSubsysControllerGetPlayerTapButtons /
 *   GetPlayerStickInRangeLR/UD (include/sc/scene.h), syTaskmanMalloc /
 *   syTaskmanSetLoadScene / syTaskmanStartTask, syVideoInit, syRdpSetViewport,
 *   syControllerFuncRead, syAudioPlayBGM (include/sys/audio.h),
 *   func_800269C0_275C0 (include/sys/audio.h:92), func_80017EC0
 *   (opening_movie_backend.c:4479), efParticleInitAll / efManagerInitEffects,
 *   ftGetStruct-adjacent DObj access via sys/obj.h, gSCManagerSceneData /
 *   gSYTaskmanDLHeads, lLBRelocTableAddr / llRelocFileCount,
 *   dLBCommonFuncMatrixList, ovl23_BSS_END + ovl1_VRAM (DECLARE_OVL in
 *   include/sc/scene.h covers both).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   sc1PChallengerStartScene (adapter below) vs
 *   src/port/title_backend.c:470 NDS_SCENE_STUB.
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

#define sc1PChallengerStartScene ndsBaseSC1PChallengerStartScene
void ndsBaseSC1PChallengerStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pchallenger.c"

#undef sc1PChallengerStartScene

void sc1PChallengerStartScene(void)
{
    ndsBaseSC1PChallengerStartScene();
}

#endif /* NDS_P2_1P_GAME */
