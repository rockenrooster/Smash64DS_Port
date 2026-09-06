/* P2-6 step 8. Continue screen (score halves at decomp :1075).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mn1pmode/mn1pcontinue.c whole,
 * following src/import/battleship_mnoption.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Score pin: mnPlayers1PGameContinueFuncRun Yes arm does
 * gSCManagerSceneData.spgame_score *= 0.5F (decomp :1075), then rebuilds
 * the score display and plays nSYAudioFGM1PGameContinue (:1078,:1084).
 * No-path (:1087+) makes the room fade + Game Over text, BGM
 * nSYAudioBGM1PGameOver + AnnounceGameOver voice (P2-6 pin sheet rows).
 *
 * The include OWNS every symbol it defines under its source name
 * (unified-owner rule, see src/import/battleship_sc1pgame_runtime.c file doc).
 * No shims, no stubs in this TU.
 *
 * Reloc: dMN1PContinueFileIDs (:37-44) loads llMN1PContinueFileID,
 * llSC1PStageClear2FileID, llIFCommonAnnounceCommonFileID,
 * llIFCommonPlayerDamageFileID, llSC1PStageClear1FileID. All five globals
 * exist under src/ + reloc_data.h externs (MN1PContinue staged 2026-09-04;
 * SC1PStageClear1/2 staged with the tally tables; IFCommon* carried since
 * P2-1). No unstaged file.
 *
 * Shims vs unresolved, see handoff report:
 * - nMN1PContinueOptionYes/No: include/sc/scene.h:385 (decomp sc/scdef.h:385,
 *   not mndef.h as first reported); nFTDemoStatusFigureDropped/FigureStand
 *   (:348,:1079): include/ft/fighter.h:176. Both carried, checked 2026-09-05.
 * - nSYAudioFGM1PGameContinue / nSYAudioBGM1PGameOver / AnnounceGameOver
 *   ordinals: in include/gm/gmsound.h since the 2026-09-05 widening.
 * - func_800269C0_275C0 voice helper: NOT shimmed or stubbed; resolves via
 *   include/sys/audio.h like the mnmessage TU (link reveals).
 * - ftManagerSetupFilesAllKind (:1210), scSubsysFighterSetStatus,
 *   ovl setup/BSS refs: left unresolved (link reveals).
 * - All other externs (gc/lb/sy/sc/ft/if/reloc): left unresolved, no shims,
 *   no stubs.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnPlayers1PGameContinueStartScene (adapter below) vs
 *   src/port/title_backend.c:435 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
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

#define mnPlayers1PGameContinueStartScene ndsBaseMNPlayers1PGameContinueStartScene
void ndsBaseMNPlayers1PGameContinueStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mn1pmode/mn1pcontinue.c"

#undef mnPlayers1PGameContinueStartScene

void mnPlayers1PGameContinueStartScene(void)
{
    /* The source manager calls Continue directly after a lost battle without
     * changing scene_curr. Select its menu task before the DS dispatcher runs;
     * the source exit and manager retain ownership of the following scene. */
    gSCManagerSceneData.scene_curr = nSCKind1PContinue;

    /* N64 reloads this overlay's BSS for each loss; the DS keeps the TU live.
     * InitVars resets the other decision state, but leaves these fields cold.
     * A retained Yes deadline would otherwise select Yes on the next visit. */
    sMN1PContinueOptionYesRetryTic = 0;
    sMN1PContinueOptionChangeWait = 0;
    sMN1PContinueIsSelectContinue = FALSE;
    ndsBaseMNPlayers1PGameContinueStartScene();
}

#endif /* NDS_P2_1P_GAME */
