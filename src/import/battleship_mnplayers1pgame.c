/* P2-6 step 8. 1P character select (difficulty + stock).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pgame.c whole,
 * following src/import/battleship_mnoption.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Difficulty/stock pin (decomp :3262-3279): mnPlayers1PGameSetSceneData
 * writes spgame_time_limit/player from the menu statics, difficulty
 * (sMNPlayers1PGameLevelValue) + stock (sMNPlayers1PGameStockValue) into the
 * backup, fkind/costume from the slot, then lbBackupWrite. The step-1
 * bridge in battleship_sc1pgame_runtime.c:105 mirrors this MINUS the
 * menu-owned sMNPlayers1PGame* statics. Unified-owner rule: this include
 * OWNS every symbol it defines under its source name, so the real
 * mnPlayers1PGameSetSceneData here owns the name (the bridge copy that once
 * lived in battleship_sc1pgame_runtime.c is gone; checked 2026-09-05, no
 * second definition in src/). Only the scene entry mnPlayers1PGameStartScene is renamed
 * to ndsBase*; SetSceneData is intentionally NOT renamed so the duplicate
 * fails loudly until the bridge is removed.
 *
 * No shims, no stubs in this TU.
 *
 * Reloc: dMNPlayers1PGameFileIDs (:30-43) loads 11 ids:
 * llMNPlayersCommonFileID, llFTEmblemSpritesFileID, llMNSelectCommonFileID,
 * llMNPlayersGameModesFileID, llMNPlayersPortraitsFileID,
 * llMNPlayers1PModeFileID, llMNPlayersDifficultyFileID,
 * llFTStocksZakoFileID, llMNCommonFontsFileID, llIFCommonDigitsFileID,
 * llMNPlayersSpotlightFileID.
 * Staged/confirmed: MNPlayersCommon + MNPlayersPortraits (VS select already
 * stages them), MNSelectCommon / MNPlayersGameModes / MNPlayersSpotlight /
 * FTEmblemSprites / MNCommonFonts / IFCommonDigits / FTStocksZako (all have
 * NDS_MENU_RELOC_SYMBOLS X rows generating reloc_data.h externs),
 * MNPlayers1PMode (extern reloc_data.h:1224, staged 2026-09-04).
 * UNSTAGED (no extern under include/, no global under src/): one file --
 *   MNPlayersDifficulty (llMNPlayersDifficultyFileID).
 * Orchestrator: python scripts/menus/stage_reloc_file.py --file MNPlayersDifficulty --list NDS_1P_RELOC_FILES
 * Portraits/names ride MNPlayersPortraits + MNPlayersCommon as noted.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu-owned sMNPlayers1PGame* statics + slot type MNPlayersSlot1PGame
 *   (decomp mn/menu.h): owned here by the include, not shimmed.
 * - nSC1PGameDifficultyVeryEasy/VeryHard (:1151,:1169): include/sc/scene.h:
 *   189; the difficulty effect table dSC1PGameComputerDesc lives in the
 *   runtime TU.
 * - nMNPlayersCursorStatus* and every other nMNPlayers* member the source
 *   names: carried by include/mn/mndef.h (2026-09-05 widening). Reloc census
 *   the same day: one row unstaged, llFTStocksZakoSprite (queued for the
 *   reloc-staging agent); everything else this TU names is rowed.
 * - ftParamGetCostumeCommonID / ftParamInitAllParts / ftGetStruct,
 *   scSubsys*/gc/lb/sy/if/audio/reloc/ovl refs: left unresolved, no shims,
 *   no stubs.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnPlayers1PGameStartScene (adapter below) vs
 *   src/port/title_backend.c:436 NDS_SCENE_STUB; plus the SetSceneData
 *   bridge deletion above vs battleship_sc1pgame_runtime.c:105.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
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

#define mnPlayers1PGameStartScene ndsBaseMNPlayers1PGameStartScene
void ndsBaseMNPlayers1PGameStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pgame.c"

#undef mnPlayers1PGameStartScene

void mnPlayers1PGameStartScene(void)
{
    ndsBaseMNPlayers1PGameStartScene();
}

#endif /* NDS_P2_1P_GAME */
