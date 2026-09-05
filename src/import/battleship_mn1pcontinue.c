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
 * - nMN1PContinueOptionYes/No (+ select/status statics): NOT defined here.
 *   Owning home is port include/mn/mndef.h (separate header-widening task,
 *   blocks compile). nFTDemoStatusFigureDropped/FigureStand (:348,:1079)
 *   likewise belong to their fighter/demo header, not here.
 * - nSYAudioFGM1PGameContinue / nSYAudioBGM1PGameOver / AnnounceGameOver
 *   ordinals: NOT shimmed here; audio owner decides (link reveals).
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
    ndsBaseMNPlayers1PGameContinueStartScene();
}

#endif /* NDS_P2_1P_GAME */
