/* P2-6 step 2. 1P Game stage-clear tally screen.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pstageclear.c whole
 * (2276 lines: dSC1PStageClearFileIDs :24-33, dSC1PStageClearBonusData
 * :36-415, lights, statics, every sc1PStageClear* / func_ovl56_* function,
 * dGM1PStageClearVideoSetup / dGM1PStageClearTaskmanSetup,
 * sc1PStageClearStartScene :2269), following battleship_sc1pbonusstage.c
 * (scene TU with its taskman/video setup and start/update/draw functions),
 * NOT a data-only transcription.
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
 * Ownership transfer: the 58-row dSC1PStageClearBonusData was a verbatim
 * transcription in src/import/battleship_sc1pstageclear_tables.c; once this
 * whole TU is included that table is owned by the include, so the tables TU
 * is deleted (its CFILES row is replaced by this file, reported alongside).
 *
 * Reloc files (dSC1PStageClearFileIDs :24-33, all staged -- ll*FileID
 * definitions in src/port/diagnostics_mp_taskman_state.c, manifests in
 * include/reloc_data.h, assets in src/port/reloc_backend_assets.c):
 * SC1PStageClear1 0x50, SC1PStageClear2 0x51, IFCommonPlayerDamage,
 * IFCommonTimer, IFCommonDigits, SC1PStageClear3 0x97,
 * GRWallpaperTrainingBlack 0x1a. No file the scene loads is unstaged.
 * Two sprite rows inside staged files have no manifest entry yet (see
 * unresolved below); file-level staging is complete.
 *
 * Counters: sc1pstageclear.c itself reads NO gSC1PGameBonus* symbol (grep
 * over the TU: zero hits) -- it consumes the baked
 * gSCManagerSceneData.bonus_get_mask[3] / spgame_score /
 * bonus_tasks_complete plus battle state. The gSC1PGameBonus* writers the
 * inventory names all defer to the strong owner under this flag:
 * Tomato/Heart (battleship_ftcommon_get.c:41-44 #if !NDS_P2_1P_GAME),
 * MewCatcher (battleship_item_map_core.c:168 same gate), Star/GiantImpact
 * (reloc_backend_compat_shims.c:396 same gate), ShieldBreaker (weak in
 * battleship_ftcommon_shieldbreakfly.c:28, strong ub8 in the included
 * sc1pgame.c:720), and the Attack/Defend arrays plus BrosCalamity
 * (sc1pgame.c:706-750, defined once by battleship_sc1pgame_runtime.c under
 * the same flag). Nothing the tally consumes lacks a flag-on definition.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - NBITS: shimmed below, verbatim from decomp include/macros.h:87 (port
 *   include/macros.h lacks it; used at :1273,1440,1473).
 * - SC1PStageClearKind (Stage/Game/Result): shimmed below, verbatim from
 *   decomp sc/scdef.h:391-397 (port include/sc/scene.h lacks it).
 * - SC1PStageClearStats { bonus_array_id; bonus_id }: shimmed below,
 *   verbatim from decomp sc/sctypes.h:105-109 (port lacks it).
 * - BGM IDs (decomp gm/gmsound.h:58-62, REGION_US, counted positionally
 *   from nSYAudioBGMPupupu = 0; port include/gm/gmsound.h stops at
 *   Results = 22): 1PBonusStage 26, 1PStageClear 27, 1PBonusStageClear 28,
 *   1PGameClear 29, 1PBonusStageFailure 30. FGM IDs (decomp :263-265,
 *   counted from nSYAudioFGMExplodeS = 0; cross-checked against the port
 *   values MenuSelect = 158 .. PlayerSlotWhoosh = 167, which match):
 *   ScoreDisplayBonus 168, StageClearScoreRegister 169,
 *   StageClearScoreDisplay 170. All guarded #ifndef so a later header
 *   promotion collides loudly instead of silently.
 * - gSYSchedulerCurrentFramebuffer (:2129): local extern, same as
 *   battleship_lbtransition.c:12 (no port header publishes it).
 * - llIFCommonDigitsColonSprite (:1383) + llIFCommonTimerSymbolCrossSprite
 *   (:949,:1089): local externs. Their files (IFCommonDigits,
 *   IFCommonTimer) are staged but these two rows have no manifest
 *   entry/definition under include/ or src/ -- definitions must be staged
 *   by the orchestrator (header edit is out of scope here). Left
 *   unresolved at link, never stubbed; invented offsets would be
 *   fabricated data.
 * - Everything else the TU calls is port-provided, no action:
 *   lbRelocGetFileData / lbRelocInitSetup / lbRelocLoadFilesListed,
 *   gcMakeGObjSPAfter / gcAddGObjDisplay / gcAddGObjProcess / gcEjectGObj /
 *   gcMakeCameraGObj / gcMakeDefaultCameraGObj / gcRunAll / gcDrawAll,
 *   lbCommonMakeSObjForGObj / lbCommonDrawSObjAttr / lbCommonDrawSObjNoAttr /
 *   lbCommonDrawSprite, dLBCommonFuncMatrixList
 *   (reloc_backend_fighter_display_seam.c:90),
 *   ftDisplayLightsDrawReflect (include/ft/fighter.h:4300, same basis as
 *   the battleship_mvopening*.c imports), scSubsysFighterGetLightAngleX/Y
 *   (reloc_backend_fighter_display_seam.c:100,105),
 *   scSubsysControllerGetPlayerTapButtons (include/sc/scene.h:561),
 *   syTaskmanSetLoadScene / syTaskmanStartTask (battleship_sys_taskman.c),
 *   syVideoInit, syRdpSetViewport, syControllerFuncRead,
 *   syAudioPlayBGM (include/sys/audio.h:86), func_800269C0_275C0
 *   (include/sys/audio.h:92), gSCManagerSceneData / gSCManagerBackupData /
 *   gSCManager1PGameBattleState, gSYTaskmanDLHeads, gSYFramebufferSets,
 *   lLBRelocTableAddr / llRelocFileCount, ovl56_BSS_END + ovl1_VRAM
 *   (DECLARE_OVL in include/sc/scene.h covers both).
 * - Collisions needing reported gating (not renamed away):
 *   dSC1PStageClearBonusData here vs the deleted tables TU (resolved by
 *   the deletion); sc1PStageClearStartScene (adapter below) vs
 *   src/port/title_backend.c:457 NDS_SCENE_STUB.
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

/* decomp include/macros.h:87 verbatim. Port include/macros.h lacks it;
 * the included TU uses it at :1273,:1440,:1473. */
#ifndef NBITS
#define NBITS(t) ((int) (sizeof(t) * 8) )
#endif

/* decomp sc/scdef.h:391-397 verbatim. Port include/sc/scene.h carries the
 * bonus enum but not this kind enum. Guarded so a later header promotion
 * collides loudly instead of silently. */
#ifndef nSC1PStageClearKindStage
typedef enum SC1PStageClearKind
{
    nSC1PStageClearKindStage,
    nSC1PStageClearKindGame,
    nSC1PStageClearKindResult
} SC1PStageClearKind;
#endif

/* decomp sc/sctypes.h:105-109 verbatim. Port lacks it; the bonus-stat
 * helpers (:1234,:1262,:1438,:1460) need the type. Same guard policy. */
#ifndef NDS_SC1PSTAGECLEAR_STATS_DEFINED
#define NDS_SC1PSTAGECLEAR_STATS_DEFINED 1
typedef struct SC1PStageClearStats
{
    s32 bonus_array_id;
    s32 bonus_id;
} SC1PStageClearStats;
#endif

/* decomp gm/gmsound.h:58-62, REGION_US, counted positionally from
 * nSYAudioBGMPupupu = 0. Port include/gm/gmsound.h ends at Results = 22
 * (values agree to there); these continue the same count. */
#ifndef nSYAudioBGM1PBonusStage
#define nSYAudioBGM1PBonusStage 26
#endif
#ifndef nSYAudioBGM1PStageClear
#define nSYAudioBGM1PStageClear 27
#endif
#ifndef nSYAudioBGM1PBonusStageClear
#define nSYAudioBGM1PBonusStageClear 28
#endif
#ifndef nSYAudioBGM1PGameClear
#define nSYAudioBGM1PGameClear 29
#endif
#ifndef nSYAudioBGM1PBonusStageFailure
#define nSYAudioBGM1PBonusStageFailure 30
#endif

/* decomp gm/gmsound.h:263-265, counted from nSYAudioFGMExplodeS = 0;
 * cross-checked: the port's MenuSelect = 158 .. PlayerSlotWhoosh = 167
 * match the same count, so 168/169/170 follow. */
#ifndef nSYAudioFGMScoreDisplayBonus
#define nSYAudioFGMScoreDisplayBonus 168
#endif
#ifndef nSYAudioFGMStageClearScoreRegister
#define nSYAudioFGMStageClearScoreRegister 169
#endif
#ifndef nSYAudioFGMStageClearScoreDisplay
#define nSYAudioFGMStageClearScoreDisplay 170
#endif

/* Same local extern as battleship_lbtransition.c:12; no port header
 * publishes it. Used by sc1PStageClearCopyFramebufToWallpaper (:2129). */
extern void *gSYSchedulerCurrentFramebuffer;

/* Staged files, unstaged rows: IFCommonDigits / IFCommonTimer are staged
 * as files, but these two sprite rows have no manifest entry or definition
 * under include/ or src/ (header edit out of scope). Declared so the TU
 * compiles; the link stays honestly open until the orchestrator stages the
 * definitions. Offsets invented here would be fabricated data. */

#define sc1PStageClearStartScene ndsBaseSC1PStageClearStartScene
void ndsBaseSC1PStageClearStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pstageclear.c"

#undef sc1PStageClearStartScene

void sc1PStageClearStartScene(void)
{
    ndsBaseSC1PStageClearStartScene();
}

#endif /* NDS_P2_1P_GAME */
