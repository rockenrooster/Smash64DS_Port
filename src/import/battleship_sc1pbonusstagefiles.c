/* P2-6 step 5 companion. Bonus-stage file setup.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstagefiles.c whole,
 * same wrapper pattern as battleship_sc1pbonusstage.c (import as ndsBase*,
 * re-export under the source name). Symbols are unique to this TU
 * (sSC1PBonusStageStatusBuffer, sSC1PBonusStageForceStatusBuffer,
 * sc1PBonusStageSetupFiles), so no port TU collides; the rename exists to
 * keep the seam uniform and reviewable.
 *
 * NOTE: this TU defines NO item-file symbol. The task brief's parenthetical
 * gSC1PBonusStageItemFiles (plural) exists nowhere in decomp or src (grep
 * hits: only ittarget.c:8 and sc1pbonusstage.c:318,430, both singular
 * gSC1PBonusStageItemFile). The Target item's provider is the sibling TU:
 * gSC1PBonusStageItemFile is decomp sc1pbonusstage.c:318 and
 * sc1PBonusStageUpdateTargetCount is :483, both defined by
 * battleship_sc1pbonusstage.c behind the same NDS_P2_1P_GAME gate, which is
 * what lets battleship_item_target.c:31,50,80 link. This file's job is the
 * common-file setup both bonus scenes share (sc1PBonusStageFuncStart calls
 * sc1PBonusStageSetupFiles at sc1pbonusstage.c:1008).
 *
 * No shims: every symbol this TU touches (lLBRelocTableAddr,
 * llRelocFileCount, dGMCommonFileIDs, gGMCommonFiles, lbRelocInitSetup,
 * lbRelocLoadFilesListed) is port-provided (reloc_data.h + gm/generic.h).
 * Gated on NDS_P2_1P_GAME with its sibling.
 */

#if NDS_P2_1P_GAME

#include <ssb_types.h>
#include <gm/generic.h>
#include <reloc_data.h>
#include <sc/scene.h>

#define sc1PBonusStageSetupFiles ndsBaseSC1PBonusStageSetupFiles
void ndsBaseSC1PBonusStageSetupFiles(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstagefiles.c"

#undef sc1PBonusStageSetupFiles

/* P7 C2: a content plays its own bonus boards (stages.json `bonus`, the
 * pair its donor's character row names: scripts/p5/stage_export.py
 * contents_bonus.json). The source chose the board by the player's kind,
 * which for a content is its parent's (sc1PBonusStageInitVars); here,
 * before anything loads, the kind becomes the content's board of the same
 * class. A Remix board's btx words fill row 0 of the descriptor table
 * the bonus scene reads it by (battleship_sc1pbonusstage.c) until the
 * scene ends. A content whose donor names a source board takes that
 * board's kind and its own row. */
#if NDS_P5
#include <gr/ground.h>
#include <nds/nds_p4.h>
#include <nds/nds_p5.h>
#include "nds_p5_native_stages.generated.h"

extern GRBonusTarget dSC1PBonusStageTargetDescs[];
extern intptr_t dSC1PBonusStageBumperDescs[][2];

static GRBonusTarget sNdsP7BonusSavedTarget;
static intptr_t sNdsP7BonusSavedBumper[2];
/* 0 none, 1 the target row borrowed, 2 the bumper row. */
static u8 sNdsP7BonusBorrowed;
__attribute__((used)) volatile u32 gNdsP7BonusBoardKind;

/* The bonus stage's player content: the campaign's player (its parent is
 * the kind the source chose by). */
static u32 ndsP7BonusContent(void)
{
    const NDSP4Fighter *f;

    if (gSCManagerSceneData.scene_prev != nSCKind1PGame)
    {
        return 0u;
    }
    f = ndsP4Fighter(gNdsP4OnePlayerContent);
    return ((f != NULL) && (f->parent_kind == (s32)gSCManagerSceneData.fkind)) ?
        (u32)gNdsP4OnePlayerContent : 0u;
}

static void ndsP7BonusBoardBegin(void)
{
    s32 gkind = gSCManagerBattleState->gkind;
    u32 cls = ndsGrBonusClass(gkind);
    u32 content = ndsP7BonusContent();
    s32 board = -1;

    sNdsP7BonusBorrowed = 0u;
    gNdsP7BonusBoardKind = (u32)gkind;
    if ((cls == 0u) || (content == 0u))
    {
        return;
    }
#define NDS_P7_CONTENT_BOARD(c, btt, btp) \
    if (content == (u32)(c)) { board = (cls == 1u) ? (btt) : (btp); }
    NDS_P5_CONTENT_BONUS_ROWS(NDS_P7_CONTENT_BOARD)
#undef NDS_P7_CONTENT_BOARD
    if ((board < 0) || (board == gkind) || (ndsGrBonusClass(board) != cls))
    {
        return;
    }
#define NDS_P7_BOARD_WORDS(g, k, w0, w1, w2) \
    if (board == (s32)(g)) \
    { \
        if ((k) == 1) \
        { \
            sNdsP7BonusSavedTarget = dSC1PBonusStageTargetDescs[0]; \
            dSC1PBonusStageTargetDescs[0].start = (intptr_t)(w0); \
            dSC1PBonusStageTargetDescs[0].dobjdesc = (intptr_t)(w1); \
            dSC1PBonusStageTargetDescs[0].anim_joint = (intptr_t)(w2); \
            sNdsP7BonusBorrowed = 1u; \
        } \
        else \
        { \
            sNdsP7BonusSavedBumper[0] = dSC1PBonusStageBumperDescs[0][0]; \
            sNdsP7BonusSavedBumper[1] = dSC1PBonusStageBumperDescs[0][1]; \
            dSC1PBonusStageBumperDescs[0][0] = (intptr_t)(w0); \
            dSC1PBonusStageBumperDescs[0][1] = (intptr_t)(w1); \
            sNdsP7BonusBorrowed = 2u; \
        } \
    }
    NDS_P5_BONUS_STAGE_ROWS(NDS_P7_BOARD_WORDS)
#undef NDS_P7_BOARD_WORDS
    if ((ndsP5BonusClass((u32)board) != 0u) && (sNdsP7BonusBorrowed == 0u))
    {
        return;     /* a content board this build did not land */
    }
    gSCManagerBattleState->gkind = (u8)board;
    gNdsP7BonusBoardKind = (u32)board;
}

void ndsP7BonusBoardEnd(void)
{
    if (sNdsP7BonusBorrowed == 1u)
    {
        dSC1PBonusStageTargetDescs[0] = sNdsP7BonusSavedTarget;
    }
    else if (sNdsP7BonusBorrowed == 2u)
    {
        dSC1PBonusStageBumperDescs[0][0] = sNdsP7BonusSavedBumper[0];
        dSC1PBonusStageBumperDescs[0][1] = sNdsP7BonusSavedBumper[1];
    }
    sNdsP7BonusBorrowed = 0u;
}
#else
void ndsP7BonusBoardEnd(void)
{
}
#endif

void sc1PBonusStageSetupFiles(void)
{
#if NDS_P5
    ndsP7BonusBoardBegin();
#endif
    ndsBaseSC1PBonusStageSetupFiles();
}

#endif /* NDS_P2_1P_GAME */
