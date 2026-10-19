/* P7 U9: the VS Record screen's numbers over every selectable fighter.
 *
 * The source's views (mn/mndata/mnvsrecord.c) read the twelve-by-twelve
 * LBBackupData records; the wide records (nds_p7_records.c, U2) hold one row
 * per selection id with each count widened to every opponent. This module
 * loads the rows of the fighters the screen shows (the twelve under the
 * source's own lock, Metal Mario and every compiled content under the P7
 * block's) into the screen's buffer and answers the source's formulas over
 * them -- mnVSRecordGetKOs, GetTKO, GetTotalTKO, GetWinPercent, GetSDPercent,
 * GetUsePercent, GetAvg, GetWinPercentAgainst, GetAvgAgainst -- with "have
 * kind" meaning "shown". Pure arithmetic over the buffer; the screen draws.
 */
#include <string.h>
#include <ssb_types.h>
#include <ft/fighter.h>
#include <sc/scene.h>
#include <nds/nds_p4.h>
#include <nds/nds_p7.h>

/* The source's mnVSRecordCheckHaveFighterKind: Ness, Purin, Captain and Luigi
 * behind the save's fighter mask, everything else admitted. */
static u32 ndsP7VsRecordOriginalShown(u32 kind)
{
    switch (kind)
    {
    case nFTKindNess:
    case nFTKindPurin:
    case nFTKindCaptain:
    case nFTKindLuigi:
        return ((gSCManagerBackupData.fighter_mask &
                 LBBACKUP_MASK_FIGHTER(kind)) != 0u) ? TRUE : FALSE;
    default:
        return TRUE;
    }
}

void ndsP7VsRecordLoad(NDSP7VsRecordData *d)
{
    u32 sel;

    if (d == NULL)
    {
        return;
    }
    memset(d, 0, sizeof(*d));
    for (sel = 0u; sel < NDS_P7_SEL_COUNT; sel++)
    {
        u32 shown;

        if (sel < NDS_P7_SEL_METAL)
        {
            shown = ndsP7VsRecordOriginalShown(sel);
        }
        else if (sel == NDS_P7_SEL_METAL)
        {
#if NDS_P2_MMARIO
            shown = ndsP7FighterUnlocked(sel);
#else
            shown = FALSE;
#endif
        }
        else
        {
#if NDS_P4
            const u32 content = sel - NDS_P7_SEL_CONTENT(0u);

            shown = ((content < NDS_P4_CONTENT_LIMIT) &&
                     (ndsP4Fighter(content) != NULL)) ?
                ndsP7FighterUnlocked(sel) : FALSE;
#else
            shown = FALSE;
#endif
        }
        if (shown != FALSE)
        {
            d->sel[d->count++] = (u8)sel;
        }
    }
    ndsP7RecordsReadRange(0u, NDS_P7_SEL_COUNT, d->rows);
}

/* Shown index -> its row. */
static const NDSP7VSRecord *ndsP7VsRecordRow(const NDSP7VsRecordData *d, u32 i)
{
    return &d->rows[d->sel[i]];
}

u32 ndsP7VsRecordKOsAgainst(const NDSP7VsRecordData *d, u32 i, u32 j)
{
    return ndsP7VsRecordRow(d, i)->ko_count[d->sel[j]];
}

/* mnVSRecordGetKOs: the row's KOs of every shown fighter. */
u32 ndsP7VsRecordKOs(const NDSP7VsRecordData *d, u32 i)
{
    u32 j;
    u32 total = 0u;

    for (j = 0u; j < d->count; j++)
    {
        total += ndsP7VsRecordKOsAgainst(d, i, j);
    }
    return total;
}

/* mnVSRecordGetTKO: every shown fighter's KOs of this one plus its
 * self-destructs, capped at 9,999 as the source caps it. */
u32 ndsP7VsRecordTKO(const NDSP7VsRecordData *d, u32 i)
{
    u32 j;
    u32 total = ndsP7VsRecordRow(d, i)->selfdestructs;

    for (j = 0u; j < d->count; j++)
    {
        total += ndsP7VsRecordKOsAgainst(d, j, i);
    }
    return (total > 9999u) ? 9999u : total;
}

/* mnVSRecordGetTotalTKO. */
u32 ndsP7VsRecordTotalTKO(const NDSP7VsRecordData *d)
{
    u32 i;
    u32 total = 0u;

    for (i = 0u; i < d->count; i++)
    {
        total += ndsP7VsRecordTKO(d, i);
    }
    return total;
}

/* mnVSRecordGetWinPercent: KOs over the shown fighters' total TKO, x100. */
f32 ndsP7VsRecordWinPercent(const NDSP7VsRecordData *d, u32 i)
{
    const f32 tko = (f32)ndsP7VsRecordTotalTKO(d);

    return ((tko != 0.0F) ? ((f32)ndsP7VsRecordKOs(d, i) / tko) : 0.0F) *
           100.0F;
}

/* mnVSRecordGetSDPercent. */
f32 ndsP7VsRecordSDPercent(const NDSP7VsRecordData *d, u32 i)
{
    const f32 tko = (f32)ndsP7VsRecordTKO(d, i);

    return ((tko != 0.0F) ?
            ((f32)ndsP7VsRecordRow(d, i)->selfdestructs / tko) : 0.0F) *
           100.0F;
}

/* The Ranking view's TIME column: time_used as the source keeps it. */
u32 ndsP7VsRecordTimeUsed(const NDSP7VsRecordData *d, u32 i)
{
    return ndsP7VsRecordRow(d, i)->time_used;
}

/* mnVSRecordGetUsePercent, over the shown fighters' games. */
f32 ndsP7VsRecordUsePercent(const NDSP7VsRecordData *d, u32 i)
{
    u32 j;
    u32 games = 0u;

    for (j = 0u; j < d->count; j++)
    {
        games += ndsP7VsRecordRow(d, j)->games_played;
    }
    return ((games != 0u) ?
            ((f32)ndsP7VsRecordRow(d, i)->games_played / (f32)games) : 0.0F) *
           100.0F;
}

/* mnVSRecordGetAvg: the mean number of players in this fighter's games. */
f32 ndsP7VsRecordAvg(const NDSP7VsRecordData *d, u32 i)
{
    const NDSP7VSRecord *row = ndsP7VsRecordRow(d, i);

    return (row->games_played != 0u) ?
        ((f32)row->player_count_tally / (f32)row->games_played) : 0.0F;
}

/* mnVSRecordGetWinPercentAgainst. */
f32 ndsP7VsRecordWinPercentAgainst(const NDSP7VsRecordData *d, u32 i, u32 j)
{
    const f32 kos_for = (f32)ndsP7VsRecordKOsAgainst(d, i, j);
    const f32 total = kos_for + (f32)ndsP7VsRecordKOsAgainst(d, j, i);

    return ((total != 0.0F) ? (kos_for / total) : 0.0F) * 100.0F;
}

/* mnVSRecordGetAvgAgainst. */
f32 ndsP7VsRecordAvgAgainst(const NDSP7VsRecordData *d, u32 i, u32 j)
{
    const NDSP7VSRecord *row = ndsP7VsRecordRow(d, i);
    const u32 against = row->played_against[d->sel[j]];

    return (against != 0u) ?
        ((f32)row->player_count_tallies[d->sel[j]] / (f32)against) : 0.0F;
}
