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
 * MNPlayers1PMode (extern reloc_data.h:1224, staged 2026-09-04), and since
 * then MNPlayersDifficulty (reloc_data.h's MNPlayersDifficulty block).
 * Portraits/names ride MNPlayersPortraits + MNPlayersCommon as noted.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu-owned sMNPlayers1PBonus* statics + slot type MNPlayersSlotBonus
 *   (decomp mn/menu.h): owned here by the include, not shimmed.
 * - nMNPlayersCursorStatus* and every other nMNPlayers* member the source
 *   names: carried by include/mn/mndef.h (2026-09-05 widening). The two
 *   game-mode titles that census found unstaged
 *   (llMNPlayersGameModesBonus1BreakTheTargetsTextSprite,
 *   ...Bonus2BoardThePlatformsTextSprite) are X rows of MNPlayersGameModes
 *   in reloc_data.h now, with geometry rows in reloc_backend_assets.c.
 * - Best-time/task-count backup accessors, ftParam and ftGetStruct,
 *   scSubsys, gc/lb/sy/if/audio/reloc/ovl refs: left unresolved, no shims,
 *   no stubs.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnPlayers1PBonusStartScene (adapter below) vs
 *   src/port/title_backend.c:434 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <string.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <mn/menu.h>
#include <nds/nds_platform.h>
#if NDS_P2_MENU_SHELL
#include <nds/nds_menu_shell.h>
#endif
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

#if NDS_P2_MENU_SHELL
/* The total row is rebuilt only where the source rebuilds it -- at entry
 * (:2860-2863) and on the title toggle (:2093-2101) -- so its three values
 * are read once per made GObj and kind instead of summing twelve records a
 * frame. Reset by StartScene: a revisit's GObj may reuse the address. */
static GObj *sNdsMNPlayers1PBonusTotalGObj;
static s32 sNdsMNPlayers1PBonusTotalKind;
static u32 sNdsMNPlayers1PBonusTotal[3];

/* The select's 2D screen is native (nds_menu_shell_onep.c, the 1P select's
 * owner). Everything here is READ from what this frame's processes left:
 * the SObj positions and GObj flags the source set, and the records through
 * the source's own accessors in the arithmetic of the Make function whose
 * SObjs the native screen replaces. No source state is written. */
static void ndsMNPlayers1PBonusPresentNative(void)
{
    NdsMenuShellBonusCssState state;
    GObj *cursor = sMNPlayers1PBonusSlot.cursor;
    GObj *puck = sMNPlayers1PBonusSlot.puck;
    GObj *name_emblem = sMNPlayers1PBonusSlot.name_emblem_gobj;
    SObj *cursor_sobj = (cursor != NULL) ? SObjGetStruct(cursor) : NULL;
    SObj *puck_sobj = (puck != NULL) ? SObjGetStruct(puck) : NULL;

    if ((cursor_sobj == NULL) || (puck_sobj == NULL))
    {
        return;
    }
    state.cursor_x = (s32)cursor_sobj->pos.x;
    state.cursor_y = (s32)cursor_sobj->pos.y;
    state.cursor_status = (u32)sMNPlayers1PBonusSlot.cursor_status;
    state.puck_x = (s32)puck_sobj->pos.x;
    state.puck_y = (s32)puck_sobj->pos.y;
    /* mnPlayers1PBonusPuckProcUpdate (:2214-2223) hides the token itself. */
    state.puck_visible = ((puck->flags & GOBJ_FLAG_HIDDEN) == 0u) ?
        TRUE : FALSE;
    /* mnPlayers1PBonusUpdateNameAndEmblem (:1573-1584) hides the pair while
     * no fighter is under the puck and re-makes it for the slot's fighter. */
    state.gate_fkind = ((name_emblem != NULL) &&
                        ((name_emblem->flags & GOBJ_FLAG_HIDDEN) == 0u)) ?
        (s32)sMNPlayers1PBonusSlot.fkind : -1;
    state.bonus_kind = (u32)sMNPlayers1PBonusBonusKind;
    state.fighter_mask = (u32)sMNPlayers1PBonusFighterMask;
    /* mnPlayers1PBonusReadyProcUpdate (:2554-2571). */
    state.ready_visible = ((sMNPlayers1PBonusIsSelected != FALSE) &&
                           (sMNPlayers1PBonusReadyBlinkWait < 30)) ?
        TRUE : FALSE;

    /* mnPlayers1PBonusMakeHiScore (:1171-1178), which the puck process runs
     * every frame (:2248): the record GObj exists only while the puck is
     * over a portrait cell (:1048, :1121), locked cells included. */
    state.record_kind = NDS_MENU_SHELL_BONUS_RECORD_NONE;
    state.record_value[0] = 0u;
    state.record_value[1] = 0u;
    state.record_value[2] = 0u;
    if (sMNPlayers1PBonusHiScoreGObj != NULL)
    {
        s32 fkind = mnPlayers1PBonusGetForcePuckFighterKind();

        if (fkind != nFTKindNull)
        {
            if (mnPlayers1PBonusCheckBonusComplete(fkind) != FALSE)
            {
                /* mnPlayers1PBonusMakeBestTime (:1050-1092). */
                u32 best_time = mnPlayers1PBonusGetBestTime(fkind);

                state.record_kind = NDS_MENU_SHELL_BONUS_RECORD_TIME;
                state.record_value[0] =
                    (u32)mnPlayers1PBonusGetMins(best_time);
                state.record_value[1] =
                    (u32)mnPlayers1PBonusGetSec(best_time);
                state.record_value[2] =
                    (u32)mnPlayers1PBonusGetCSec(best_time);
            }
            else
            {
                /* mnPlayers1PBonusMakeBestTaskCount (:1145). */
                state.record_kind = NDS_MENU_SHELL_BONUS_RECORD_COUNT;
                state.record_value[0] =
                    (u32)mnPlayers1PBonusGetBestTaskCount(fkind);
            }
        }
    }

    /* mnPlayers1PBonusMakeTotalTime (:1187-1244) with its own carries
     * (:1210-1243): hundredths, then seconds plus their carry, then minutes
     * plus theirs. */
    state.total_visible = (sMNPlayers1PBonusTotalTimeGObj != NULL) ?
        TRUE : FALSE;
    if ((state.total_visible != FALSE) &&
        ((sMNPlayers1PBonusTotalTimeGObj != sNdsMNPlayers1PBonusTotalGObj) ||
         (sMNPlayers1PBonusBonusKind != sNdsMNPlayers1PBonusTotalKind)))
    {
        s32 centiseconds = mnPlayers1PBonusGetTotalCSec();
        s32 remainder = centiseconds / 100;
        s32 seconds = mnPlayers1PBonusGetTotalSec() + remainder;

        sNdsMNPlayers1PBonusTotal[2] = (u32)(centiseconds % 100);
        remainder = seconds / TIME_SEC;
        seconds %= TIME_SEC;
        sNdsMNPlayers1PBonusTotal[1] = (u32)seconds;
        sNdsMNPlayers1PBonusTotal[0] =
            (u32)(mnPlayers1PBonusGetTotalMins() + remainder);
        sNdsMNPlayers1PBonusTotalGObj = sMNPlayers1PBonusTotalTimeGObj;
        sNdsMNPlayers1PBonusTotalKind = sMNPlayers1PBonusBonusKind;
    }
    state.total_value[0] = sNdsMNPlayers1PBonusTotal[0];
    state.total_value[1] = sNdsMNPlayers1PBonusTotal[1];
    state.total_value[2] = sNdsMNPlayers1PBonusTotal[2];

    ndsMenuShellBonusCssPresent(&state);
}
#endif

/* The select's preview is 3D, which BG0 shows only while a scene asks, in
 * the cameras' (10,10)-(310,230) window -- the 1P game select's draw. Its
 * 2D screen is presented natively first, from the state this frame's
 * processes left, as the 1P select's draw does. */
static void ndsMNPlayers1PBonusDraw(void)
{
    GObj *fighter = sMNPlayers1PBonusSlot.player;

#if NDS_P2_MENU_SHELL
    ndsMNPlayers1PBonusPresentNative();
#endif
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
    /* The slot too, which the overlay's BSS zeroing also cleared and no init
     * resets: its `player` is the last visit's fighter GObj, and the first
     * fighter change of a revisit hands it to mnPlayers1PBonusMakeFighter,
     * which reads its root DObj and destroys it (:1343-1346) -- a data abort
     * on the freed GObj (owner r71: "Bonus CSS freezes often"; every visit
     * after the first). The 1P game select's InitSlot nulls its own
     * (mnplayers1pgame.c:3356); this one never did. */
    memset(&sMNPlayers1PBonusSlot, 0, sizeof(sMNPlayers1PBonusSlot));
#if NDS_P2_MENU_SHELL
    sNdsMNPlayers1PBonusTotalGObj = NULL;
#endif
    ndsBaseMNPlayers1PBonusStartScene();
#if NDS_P2_MENU_SHELL
    ndsMenuShellOnePlayerCssExit();
#endif
    ndsFighterManagerRegisterDisplayFighter(NULL,
                                            (u32)sMNPlayers1PBonusManPlayer);
    ndsMNPlayers1PPreviewRetire();
}

#endif /* NDS_P2_1P_GAME */
