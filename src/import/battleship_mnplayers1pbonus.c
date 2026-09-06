/* P2-6 step 8. Bonus-stage character select.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pbonus.c whole,
 * following src/import/battleship_mnoption.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter only nulls the one entry GObj handle source InitVars never
 * clears (see the StartScene wrapper); no other behaviour invented here.
 *
 * The include OWNS every symbol it defines under its source name
 * (unified-owner rule, see src/import/battleship_sc1pgame_runtime.c file doc).
 * No shims, no stubs in this TU.
 *
 * Reloc: dMNPlayers1PBonusFileIDs (:17-30) loads the same 11 ids as the 1P
 * game select: llMNPlayersCommonFileID, llFTEmblemSpritesFileID,
 * llMNSelectCommonFileID, llMNPlayersGameModesFileID,
 * llMNPlayersPortraitsFileID, llMNPlayers1PModeFileID,
 * llMNPlayersDifficultyFileID, llFTStocksZakoFileID, llMNCommonFontsFileID,
 * llIFCommonDigitsFileID, llMNPlayersSpotlightFileID.
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
 * - Menu-owned sMNPlayers1PBonus* statics + slot type MNPlayersSlotBonus
 *   (decomp mn/menu.h): owned here by the include, not shimmed.
 * - nMNPlayersCursorStatus* and every other nMNPlayers* member the source
 *   names: carried by include/mn/mndef.h (2026-09-05 widening). Reloc
 *   census the same day: two rows unstaged, the Bonus1/Bonus2 game-mode
 *   text sprites (llMNPlayersGameModesBonus1BreakTheTargetsTextSprite,
 *   ...Bonus2BoardThePlatformsTextSprite), queued for the reloc-staging
 *   agent as an --extend of MNPlayersGameModes.
 * - Best-time/task-count backup accessors, ftParam*/ftGetStruct,
 *   scSubsys*/gc/lb/sy/if/audio/reloc/ovl refs: left unresolved, no shims,
 *   no stubs.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnPlayers1PBonusStartScene (adapter below) vs
 *   src/port/title_backend.c:434 NDS_SCENE_STUB.
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

#define mnPlayers1PBonusStartScene ndsBaseMNPlayers1PBonusStartScene
void ndsBaseMNPlayers1PBonusStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pbonus.c"

#undef mnPlayers1PBonusStartScene

void mnPlayers1PBonusStartScene(void)
{
    /* Same overlay-lifetime gap for overlay 29 (scmanager.c:1181-1197):
     * source InitVars (mnplayers1pbonus.c:2755-2776) nulls only
     * TotalTimeGObj, while MakeBestTime / MakeBestTaskCount (:1043, :1116)
     * eject HiScoreGObj when non-NULL on the first cursor move, so a
     * revisit would eject the torn-down GObj. Null it on entry. */
    sMNPlayers1PBonusHiScoreGObj = NULL;
    ndsBaseMNPlayers1PBonusStartScene();
}

#endif /* NDS_P2_1P_GAME */
