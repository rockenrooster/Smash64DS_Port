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
 * - Best-time/task-count backup accessors, ftParam and ftGetStruct,
 *   scSubsys, gc/lb/sy/if/audio/reloc/ovl refs: left unresolved, no shims,
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
#include <nds/nds_platform.h>
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

/* Exact source header decomp lb/lbcommon.h:11 (matrix list at :2909), same
 * extern form as battleship_mnplayersvs.c:38. */
extern sb32 (*dLBCommonFuncMatrixList[])(void);

/* Exact source header decomp mn/mnplayers/mnplayers1pbonus.h (each used
 * before its definition; campaign-build-1 error lines :279-:2835). */
extern void mnPlayers1PBonusUpdateCursor(GObj *gobj, s32 player, s32 cursor_status);
extern void mnPlayers1PBonusUpdateCursorPlacementPriorities(s32 player);
extern void mnPlayers1PBonusAnnounceFighter(s32 player, s32 slot);
extern void mnPlayers1PBonusMakePortraitFlash(s32 player);
extern s32 mnPlayers1PBonusGetForcePuckFighterKind(void);
extern void mnPlayers1PBonusUpdateCursorGrabPriorities(s32 player, s32 puck);
extern void mnPlayers1PBonusSetSceneData(void);
extern sb32 mnPlayers1PBonusCheckBonusCompleteAll(void);

/* Landed precedent extern (battleship_mntraining.c:90); called at :2835. */
extern void efManagerInitEffects(void);

/* THE PREVIEW, NOT THE ROSTER (owner r67: Bonus 1/2 Practice "do not
 * work"). mnPlayers1PBonusFuncStart sets up every playable fighter's files
 * (:2837-2840) so any puck pick can show its model at once; the 12 kinds do
 * not fit the DS heap, and the fourth (Samus, 73,120 B) halted in
 * ndsSyMallocOverflowHalt on entry. The 1P game select's preview boundary
 * (battleship_mnplayers1pgame.c) replaces that preload: the one fighter the
 * select shows loads into a resettable arena when it is made and retires
 * when it is destroyed. Selection, costume and rotation stay the source's. */
extern void ndsFighterManagerRegisterDisplayFighter(GObj *gobj, u32 slot);
GObj *ndsMNPlayers1PPreviewMakeFighter(FTDesc *desc);
void ndsMNPlayers1PPreviewDestroyFighter(GObj *gobj);
void ndsMNPlayers1PPreviewSkipPreload(s32 fkind);
void ndsMNPlayers1PPreviewRetire(void);
static void ndsMNPlayers1PBonusDraw(void);

#define ftManagerMakeFighter ndsMNPlayers1PPreviewMakeFighter
#define ftManagerDestroyFighter ndsMNPlayers1PPreviewDestroyFighter
#define ftManagerSetupFilesAllKind ndsMNPlayers1PPreviewSkipPreload
#define gcDrawAll ndsMNPlayers1PBonusDraw
#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pbonus.c"
#undef gcDrawAll
#undef ftManagerSetupFilesAllKind
#undef ftManagerDestroyFighter
#undef ftManagerMakeFighter

#undef mnPlayers1PBonusStartScene

/* The select's preview is 3D, which BG0 shows only while a scene asks, in
 * the cameras' (10,10)-(310,230) window -- the 1P game select's draw. */
static void ndsMNPlayers1PBonusDraw(void)
{
    GObj *fighter = sMNPlayers1PBonusSlot.player;

    ndsPlatformSet3DLayerEnabled((fighter != NULL) &&
        ((fighter->flags & GOBJ_FLAG_HIDDEN) == 0u));
    ndsPlatformSet3DViewportSource(10, 10, 310, 230);
    gcDrawAll();
    ndsPlatformReset3DViewport();
}

void mnPlayers1PBonusStartScene(void)
{
    /* Same overlay-lifetime gap for overlay 29 (scmanager.c:1181-1197):
     * source InitVars (mnplayers1pbonus.c:2755-2776) nulls only
     * TotalTimeGObj, while MakeBestTime / MakeBestTaskCount (:1043, :1116)
     * eject HiScoreGObj when non-NULL on the first cursor move, so a
     * revisit would eject the torn-down GObj. Null it on entry. */
    sMNPlayers1PBonusHiScoreGObj = NULL;
    ndsBaseMNPlayers1PBonusStartScene();
    ndsFighterManagerRegisterDisplayFighter(NULL,
                                            (u32)sMNPlayers1PBonusManPlayer);
    ndsMNPlayers1PPreviewRetire();
}

#endif /* NDS_P2_1P_GAME */
