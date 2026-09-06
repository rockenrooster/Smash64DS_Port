/* P2-6 step 8. 1P character select (difficulty + stock).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pgame.c whole,
 * following src/import/battleship_mnoption.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter clears the source overlay's stale entry handle and binds the
 * source preview to the DS renderer through its creation/material/draw seams.
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
 *   scSubsys, gc/lb/sy/if/audio/reloc/ovl refs: left unresolved, no shims,
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

#define mnPlayers1PGameStartScene ndsBaseMNPlayers1PGameStartScene
void ndsBaseMNPlayers1PGameStartScene(void);

/* Exact source header decomp lb/lbcommon.h:11 (matrix list at :3521), same
 * extern form as battleship_mnplayersvs.c:38. */
extern sb32 (*dLBCommonFuncMatrixList[])(void);

/* Exact source header decomp mn/mnplayers/mnplayers1pgame.h (each used
 * before its definition; campaign-build-1 error lines :422-:3448). */
extern void mnPlayers1PGameUpdateCursor(GObj *gobj, s32 player, s32 cursor_status);
extern void mnPlayers1PGameUpdateCursorPlacementPriorities(s32 player);
extern void mnPlayers1PGameAnnounceFighter(s32 player, s32 slot);
extern void mnPlayers1PGameMakePortraitFlash(s32 player);
extern void mnPlayers1PGameMakeStock(s32 stock, s32 fkind);
extern void mnPlayers1PGameUpdateNameAndEmblem(s32 player);
extern s32 mnPlayers1PGameGetForcePuckFighterKind(void);
extern void mnPlayers1PGameSetSceneData(void);
extern s32 mnPlayers1PGameGetNextTimeValue(s32 value);
extern s32 mnPlayers1PGameGetPrevTimeValue(s32 value);
extern sb32 mnPlayers1PGameCheckReady(void);

/* Landed precedent extern (battleship_mntraining.c:90); called at :3448. */
extern void efManagerInitEffects(void);

extern void ndsFighterManagerRegisterDisplayFighter(GObj *gobj, u32 slot);
extern void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot);
static GObj *ndsMNPlayers1PGameMakeFighter(FTDesc *desc);
static void ndsMNPlayers1PGameDestroyFighter(GObj *gobj);
static void ndsMNPlayers1PGameDraw(void);

/* Keep the source's selection, rotation and costume logic. These three
 * backend boundaries give its single preview the same instance lifetime and
 * material invalidation as the VS character-select bridge. */
#define ftManagerMakeFighter ndsMNPlayers1PGameMakeFighter
#define ftManagerDestroyFighter ndsMNPlayers1PGameDestroyFighter
#define gcDrawAll ndsMNPlayers1PGameDraw
#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pgame.c"
#undef gcDrawAll
#undef ftManagerDestroyFighter
#undef ftManagerMakeFighter

#undef mnPlayers1PGameStartScene

static GObj *ndsMNPlayers1PGameMakeFighter(FTDesc *desc)
{
    GObj *gobj;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsFighterRendererInvalidateMaterialCachesForSlot((u32)desc->player);
#endif
    gobj = ftManagerMakeFighter(desc);
    ndsFighterManagerRegisterDisplayFighter(gobj, (u32)desc->player);
    return gobj;
}

static void ndsMNPlayers1PGameDestroyFighter(GObj *gobj)
{
    ndsFighterManagerRegisterDisplayFighter(NULL, (u32)ftGetStruct(gobj)->nds_slot);
    ftManagerDestroyFighter(gobj);
}

static void ndsMNPlayers1PGameDraw(void)
{
    GObj *fighter = sMNPlayers1PGameSlot.player;
    ndsPlatformSet3DLayerEnabled((fighter != NULL) &&
        ((fighter->flags & GOBJ_FLAG_HIDDEN) == 0u));
    ndsPlatformSet3DViewportSource(10, 10, 310, 230);
    gcDrawAll();
    ndsPlatformReset3DViewport();
}

void mnPlayers1PGameStartScene(void)
{
    /* N64 overlay 27 is DMA-loaded on every entry (scmanager.c:1167-1171),
     * zeroing BSS; the DS TU stays resident, and source InitVars
     * (mnplayers1pgame.c:3387-3409) nulls every entry GObj handle except
     * sMNPlayers1PGameTimeGObj, which MakeTimeSelect (:1019-1026) ejects
     * when non-NULL. Reentry would eject the torn-down GObj, so null it. */
    sMNPlayers1PGameTimeGObj = NULL;
    ndsBaseMNPlayers1PGameStartScene();
    ndsFighterManagerRegisterDisplayFighter(NULL, (u32)sMNPlayers1PGameManPlayer);
}

#endif /* NDS_P2_1P_GAME */
