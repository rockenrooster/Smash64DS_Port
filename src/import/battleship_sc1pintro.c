/* P2-6 step 8. 1P Game VS-style stage intro (portraits + camera animations).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pintro.c whole
 * (2044 lines: dSC1PIntroFileIDs :18, sky/banners/VS decal/labels/figures/
 * stage info, player/ally/VS fighter makers, fighter + stage camera makers,
 * announce, fighter-file setup, FuncStart, taskman setup, sc1PIntroStartScene
 * :2037), following battleship_sc1pstageclear.c (scene TU with its
 * taskman/video setup and start/update/draw functions), NOT a data-only
 * transcription.
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
 * Reloc files (dSC1PIntroFileIDs :18):
 * - SC1PIntro 0xb: STAGED (llSC1PIntroFileID in
 *   src/port/diagnostics_mp_taskman_state.c:474, manifest rows in
 *   include/reloc_data.h:1316-1388, asset in src/port/reloc_backend_assets.c,
 *   NitroFS in NDS_1P_RELOC_FILES). Covers the 47 sprites plus all 23
 *   per-fighter/stage camera AnimJoints.
 * - CharacterNames 0xc, BonusPicture 0xd, BonusPicturePlatform 0xe: UNSTAGED.
 *   No llCharacterNamesFileID / llBonusPictureFileID /
 *   llBonusPicturePlatformFileID global exists under src/, and no
 *   CharacterNames/BonusPicture rows exist under include/. Orchestrator:
 *     python scripts/menus/stage_reloc_file.py --file CharacterNames --list NDS_1P_RELOC_FILES
 *     python scripts/menus/stage_reloc_file.py --file BonusPicture --list NDS_1P_RELOC_FILES
 *     python scripts/menus/stage_reloc_file.py --file BonusPicturePlatform --list NDS_1P_RELOC_FILES
 *   Until then the name-sprite table (:527-538), the VS-label table
 *   (:459-472, Link/Pikachu rows), and sc1PIntroMakeBonusPicture (:1045,
 *   :1055, :1065) stay honestly open at link; offsets invented here would be
 *   fabricated data.
 *
 * Fighter-side symbols the intro resolves (all port-provided, no action):
 * ftManagerMakeFighter (battleship_ftmanager.c:94), ftManagerSetupFilesAllKind
 * (battleship_ftmanager.c:80), ftManagerAllocFighter + gFTManagerFigatreeHeapSize
 * (reloc_backend_compat_shims.c:1639), dFTManagerDefaultFighterDesc
 * (include/ft/fighter.h:4158), ftParamGetCostumeCommonID
 * (reloc_backend_compat_shims.c:16271), ftParamInitAllParts
 * (reloc_backend_compat_shims.c:1807), ftParamSetModelPartID
 * (reloc_backend_compat_shims.c:8323), scSubsysFighterSetStatus
 * (reloc_backend_compat_shims.c:16316), ftMainSetStatus
 * (reloc_backend_ftmain_status_compat.c:4488), ftDisplayMainProcDisplay
 * (reloc_backend_fighter_display_seam.c:1), dSCSubsysFighterScales +
 * scSubsysFighterSetLightParams (reloc_backend_fighter_display_seam.c:93,110),
 * gSC1PManagerKirbyTeamModelPartID (owned by the sc1pmanager.c whole-TU include
 * in battleship_sc1pmanager.c under the same flag). Fighter production data
 * already carries every kind the intro instantiates: Mario/Fox/Donkey/Samus/
 * Luigi/Link/Yoshi/Captain/Kirby/Pikachu/Purin/Ness plus the Yoshi/Kirby teams,
 * the Mario Bros pair, Giant DK, Metal Mario, Boss, and the nFTKindNStart..NEnd
 * polygon range (ftchar_data_slots.c slots + reloc_backend_ftdata_symbols.c
 * manifests per the P2-6 inventory); SetupFilesAllKind/AllocFighter size from
 * that manifest, so no new fighter data is staged here.
 *
 * Camera AnimJoints are not sprites: the source pulls each joint with
 * lbRelocGetFileData(AObjEvent32*, sSC1PIntroFiles[0], &llSC1PIntro*CamAnimJoint)
 * (:1178, :1558) and runs the AObj camera path gcAddCObjCamAnimJoint(cobj, joint,
 * 0.0F) + gcPlayCamAnim(gobj). The port already runs source camera AnimJoints:
 * gcAddCObjCamAnimJoint is the whole-TU import in
 * src/import/battleship_sys_objanim.c:20,52,1972, gcPlayCamAnim is externed in
 * src/port/diagnostics_mp_taskman_state.c:546 and observed by
 * src/port/taskman_seam_scene_capture.c:824,993,1233, and the opening movie
 * already drives the same seam (llMVOpeningRoomScene1/2CamAnimJoint,
 * llMVOpeningCommon*CamAnimJoint probed in reloc_backend_assets.c:8466-8501,
 * 8850-8857). No seam is missing; the intro's 23 joints ride it once staged
 * (they are, inside SC1PIntro).
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - FTDemoDesc { fkind; costume; shade }: shimmed below, verbatim from decomp
 *   ft/fttypes.h:717-722 (port include/ft/fighter.h carries FTDesc but not
 *   FTDemoDesc; the intro keeps three FTDemoDesc globals :68-80 and passes them
 *   by value :757). Guarded so a later header promotion collides loudly.
 * - BGM/voice enumerators the TU names but the port headers lack (separate task
 *   widens them -- listed, NOT defined here): nSYAudioBGM1PIntro (:1983),
 *   nSYAudioBGMBossStage (:1981), nSYAudioVoiceAnnounce{Versus,Mario,Fox,Donkey,
 *   Samus,Luigi,Link,Yoshi,Captain,Kirby,Pikachu,Purin,Ness,Link,YoshiTeam,Fox,
 *   MarioBros,Pikachu,GDonkey,KirbyTeam,Samus,MMario,Zako,BreakTheTargets,
 *   BoardThePlatforms,RaceToTheFinish} (:1709-1741, :1787-1795). Their numeric
 *   values can only come from the gm/gmsound.h promotion.
 * - Everything else the TU calls is port-provided, no action:
 *   lbRelocGetFileData / lbRelocInitSetup / lbRelocLoadFilesListed,
 *   gcMakeGObjSPAfter / gcAddGObjDisplay / gcAddGObjProcess / gcMoveGObjDL /
 *   gcMakeCameraGObj / gcMakeDefaultCameraGObj / gcAddXObjForCamera /
 *   gcRunAll / gcPlayAnimAll / gcEndProcessAll, lbCommonMakeSObjForGObj /
 *   lbCommonDrawSObjAttr / lbCommonDrawSprite / lbCommonClearExternSpriteParams,
 *   scSubsysControllerGetPlayerTapButtons / GetPlayerStickInRangeLR/UD
 *   (include/sc/scene.h), syTaskmanMalloc / syTaskmanSetLoadScene /
 *   scManagerFuncUpdate / scManagerFuncDraw, syVideoInit, syRdpSetViewport,
 *   syControllerFuncRead, sySchedulerGetTicCount / sySchedulerSetTicCount,
 *   syAudioPlayBGM (include/sys/audio.h), func_800269C0_275C0
 *   (include/sys/audio.h:92), func_800266A0_272A0 (same basis as the
 *   battleship_mvopening*.c imports), func_80017EC0
 *   (opening_movie_backend.c:4479), efParticleInitAll / efManagerInitEffects,
 *   gSCManagerSceneData / gSCManager1PGameBattleState / gSYTaskmanDLHeads,
 *   lLBRelocTableAddr / llRelocFileCount, dLBCommonFuncMatrixList,
 *   ovl24_BSS_END + ovl1_VRAM (DECLARE_OVL in include/sc/scene.h covers both).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   sc1PIntroStartScene (adapter below) vs
 *   src/port/title_backend.c:471 NDS_SCENE_STUB.
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

/* decomp ft/fttypes.h:717-722 verbatim. Port include/ft/fighter.h has FTDesc
 * but not FTDemoDesc; the included TU stores three of them (:68-80) and takes
 * one by value (:757). Guarded so a later header promotion collides loudly
 * instead of silently. */
#ifndef NDS_SC1PINTRO_FTDEMODESC_DEFINED
#define NDS_SC1PINTRO_FTDEMODESC_DEFINED 1
typedef struct FTDemoDesc
{
    s32 fkind;
    s32 costume;
    s32 shade;
} FTDemoDesc;
#endif

#define sc1PIntroStartScene ndsBaseSC1PIntroStartScene
void ndsBaseSC1PIntroStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pintro.c"

#undef sc1PIntroStartScene

void sc1PIntroStartScene(void)
{
    ndsBaseSC1PIntroStartScene();
}

#endif /* NDS_P2_1P_GAME */
