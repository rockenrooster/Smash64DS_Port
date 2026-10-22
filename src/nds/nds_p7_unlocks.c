/* P7 U3 (docs/P7/P7_Implementation_Plan.md): the unlock registry's
 * evaluator. The rows (scripts/p7/unlocks.json, generated into
 * nds_p7_unlocks.generated.inc) are data; this reads one run's statistics at
 * a trigger site and names the rows it meets that are not unlocked yet, their
 * prerequisites held and their content compiled, in table order -- then the
 * companion rows those open (a stage that comes with a fighter). It decides
 * nothing else: delivery (a challenger fight or a message) and the bits are
 * the caller's (U4, U6). Host-tested: scripts/p7/test_p7_unlocks.py. */
#include <ssb_types.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <sc/scene.h>
#include <nds/nds_p4.h>
#include <nds/nds_p5.h>
#include <nds/nds_p7.h>

#include "nds_p7_unlocks.generated.inc"

u32 ndsP7UnlockRowCount(void)
{
    return NDS_P7_UNLOCK_ROWS;
}

const NDSP7UnlockRow *ndsP7UnlockRow(u32 index)
{
    return (index < NDS_P7_UNLOCK_ROWS) ? &sNdsP7UnlockRows[index] : NULL;
}

static u32 ndsP7UnlockOpen(const NDSP7UnlockRow *row)
{
    return (row->kind == NDS_P7_UNLOCK_FIGHTER) ? ndsP7FighterUnlocked(row->id) :
        ndsP7StageUnlocked(row->id);
}

/* The row's own content, and its challenger's stage, in this build. */
static u32 ndsP7UnlockCompiled(const NDSP7UnlockRow *row)
{
    if ((row->kind == NDS_P7_UNLOCK_FIGHTER) && (row->content != 0u) &&
        (ndsP7UnlockContentCompiled(row->content) == FALSE))
    {
        return FALSE;
    }
    if ((row->kind == NDS_P7_UNLOCK_STAGE) && (ndsP7UnlockStageCompiled(row->stage_gkind) == FALSE))
    {
        return FALSE;
    }
    if ((row->delivery == NDS_P7_DELIVERY_CHALLENGER) &&
        (ndsP7UnlockStageCompiled(row->challenger_gkind) == FALSE))
    {
        return FALSE;
    }
    return TRUE;
}

/* Every selection id in `mask` unlocked, counting this run's new ones. */
static u32 ndsP7UnlockHave(u64 mask, u64 fresh)
{
    u32 sel;

    for (sel = 0u; sel < NDS_P7_SEL_COUNT; sel++)
    {
        if ((((mask >> sel) & 1u) != 0u) && (((fresh >> sel) & 1u) == 0u) &&
            (ndsP7FighterUnlocked(sel) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* The save's 1P records over the original fighters (the source's, updated
 * by the run before this is asked). */
static u32 ndsP7UnlockRecords(const NDSP7UnlockRow *row)
{
    u32 fkind;

    for (fkind = 0u; fkind < GMCOMMON_FIGHTERS_PLAYABLE_NUM; fkind++)
    {
        const LBBackup1PRecord *rec = &gSCManagerBackupData.spgame_records[fkind];

        if ((row->all_cleared != 0u) && (rec->is_spgame_complete == FALSE))
        {
            return FALSE;
        }
        if ((row->all_targets != 0u) && (rec->bonus1_task_count < NDS_P7_BONUS_TASKS))
        {
            return FALSE;
        }
        if ((((row->fighters >> fkind) & 1u) != 0u) &&
            ((rec->bonus1_task_count < row->bonus_targets) ||
             (rec->bonus2_task_count < row->bonus_platforms)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsP7UnlockMeets(const NDSP7UnlockRow *row, const NDSP7Run *run)
{
    if (row->site == NDS_P7_SITE_BONUS_END)
    {
        if ((run->site != NDS_P7_SITE_BONUS1_END) && (run->site != NDS_P7_SITE_BONUS2_END))
        {
            return FALSE;
        }
    }
    else if (row->site != run->site)
    {
        return FALSE;
    }
    if ((row->fighters != 0u) && (((row->fighters >> run->sel) & 1u) == 0u))
    {
        return FALSE;
    }
    if ((row->site == NDS_P7_SITE_STAGE_CLEAR) &&
        ((run->stage != row->stage) ||
         ((row->max_stage_falls != NDS_P7_ANY) && (run->stage_falls > row->max_stage_falls))))
    {
        return FALSE;
    }
    if ((run->difficulty < row->min_difficulty) ||
        ((row->max_continues != NDS_P7_ANY) && (run->continues > row->max_continues)) ||
        ((row->max_falls != NDS_P7_ANY) && (run->falls > row->max_falls)) ||
        ((row->stock_setting != NDS_P7_ANY) && (run->stock_setting != row->stock_setting)) ||
        ((row->max_time_s != 0xFFFFu) && (run->time_s > row->max_time_s)))
    {
        return FALSE;
    }
    if ((row->min_race_left_s != 0u) &&
        ((run->race_left_s == NDS_P7_NO_RACE) || (run->race_left_s < row->min_race_left_s)))
    {
        return FALSE;
    }
    return ndsP7UnlockRecords(row);
}

u32 ndsP7UnlockMatch(const NDSP7Run *run, u8 *out, u32 max)
{
    u64 fresh = 0u;
    u32 n = 0u;
    u32 i;

    if ((run == NULL) || (out == NULL))
    {
        return 0u;
    }
    /* The run's own rows, in table order; then the companions they open. */
    for (i = 0u; i < NDS_P7_UNLOCK_ROWS; i++)
    {
        const NDSP7UnlockRow *row = &sNdsP7UnlockRows[i];

        if ((row->site == NDS_P7_SITE_UNLOCK) || (ndsP7UnlockOpen(row) != FALSE) ||
            (ndsP7UnlockCompiled(row) == FALSE) || (ndsP7UnlockHave(row->requires, fresh) == FALSE) ||
            (ndsP7UnlockMeets(row, run) == FALSE))
        {
            continue;
        }
        if (row->kind == NDS_P7_UNLOCK_FIGHTER)
        {
            fresh |= (u64)1u << row->id;
        }
        if (n < max)
        {
            out[n++] = (u8)i;
        }
    }
    for (i = 0u; i < NDS_P7_UNLOCK_ROWS; i++)
    {
        const NDSP7UnlockRow *row = &sNdsP7UnlockRows[i];

        if ((row->site != NDS_P7_SITE_UNLOCK) || (fresh == 0u) || ((row->requires & fresh) == 0u) ||
            (ndsP7UnlockOpen(row) != FALSE) || (ndsP7UnlockCompiled(row) == FALSE) ||
            (ndsP7UnlockHave(row->requires, fresh) == FALSE))
        {
            continue;
        }
        if (n < max)
        {
            out[n++] = (u8)i;
        }
    }
    return n;
}

u32 ndsP7UnlockCompanions(u8 *out, u32 max)
{
    u32 n = 0u;
    u32 i;

    if (out == NULL)
    {
        return 0u;
    }
    for (i = 0u; i < NDS_P7_UNLOCK_ROWS; i++)
    {
        const NDSP7UnlockRow *row = &sNdsP7UnlockRows[i];

        if ((row->site != NDS_P7_SITE_UNLOCK) || (ndsP7UnlockOpen(row) != FALSE) ||
            (ndsP7UnlockCompiled(row) == FALSE) || (ndsP7UnlockHave(row->requires, 0u) == FALSE))
        {
            continue;
        }
        if (n < max)
        {
            out[n++] = (u8)i;
        }
    }
    return n;
}

u32 ndsP7UnlockPending(const NDSP7UnlockRow *row)
{
    return ((row != NULL) && (ndsP7UnlockOpen(row) == FALSE) &&
            (ndsP7UnlockCompiled(row) != FALSE) &&
            (ndsP7UnlockHave(row->requires, 0u) != FALSE)) ? TRUE : FALSE;
}

void ndsP7UnlockDeliver(const NDSP7UnlockRow *row)
{
    if (row == NULL)
    {
        return;
    }
    if (row->kind == NDS_P7_UNLOCK_FIGHTER)
    {
        ndsP7SetFighterUnlocked(row->id);
    }
    else
    {
        ndsP7SetStageUnlocked(row->id);
    }
}

/* This build's contents and stages: a row whose fighter or stage (or whose
 * challenger's stage) is not compiled stays dormant. The host test supplies
 * its own (NDS_P7_UNLOCK_HOOKS 0). */
#ifndef NDS_P7_UNLOCK_HOOKS
#define NDS_P7_UNLOCK_HOOKS 1
#endif
#if NDS_P7_UNLOCK_HOOKS
/* U5: resident, since the content queries run in resident code (the 1P
 * manager that sets it lives in the front-end overlay's head). */
NDSP7Challenger gNdsP7Challenger;

u32 ndsP7ChallengerContent(s32 player)
{
    const SCBattleState *state = gSCManagerBattleState;

    if ((gNdsP7Challenger.active == 0u) || (gNdsP7Challenger.content == 0u) ||
        (player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return 0u;
    }
    if (gSCManagerSceneData.scene_curr == nSCKind1PChallenger)
    {
        /* The screen's one fighter (sc1pchallenger.c:213). */
        return (player == 0) ? gNdsP7Challenger.content : 0u;
    }
    /* The fight's enemy: the one CPU the challenger rows make. */
    if ((state != NULL) && (state->players[player].pkind == nFTPlayerKindCom) &&
        (state->players[player].is_spgame_enemy != FALSE) &&
        (state->players[player].fkind == gNdsP7Challenger.fkind))
    {
        return gNdsP7Challenger.content;
    }
    return 0u;
}

u32 ndsP7UnlockContentCompiled(u32 content)
{
    return (ndsP4Fighter(content) != NULL) ? TRUE : FALSE;
}

u32 ndsP7UnlockStageCompiled(u32 gkind)
{
    if (gkind <= (u32)nGRKindInishie)
    {
        return TRUE;
    }
#if NDS_P5
    return ndsP5StageAdmitted(gkind);
#else
    return FALSE;
#endif
}
#endif
