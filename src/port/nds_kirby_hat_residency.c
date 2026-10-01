#include "nds_build_config.h"
#include <ft/fighter.h>
#include <sc/scene.h>
#include <reloc_data.h>
#include <nds/arm9/cache.h>
#include <nds/nds_kirby_hat_residency.h>
#include <nds/nds_renderer.h>

volatile u32 gNdsKirbyHatRequiredHighMask;
volatile u32 gNdsKirbyHatRequiredLowMask;
volatile u32 gNdsKirbyHatAdmissionFailure;

void __attribute__((noinline, used)) ndsKirbyHatResidencyHalt(u32 reason)
{
    gNdsKirbyHatAdmissionFailure = reason;
    DC_FlushAll();
    for (;;) { __asm__ volatile("" ::: "memory"); }
}

#if NDS_P2_KIRBY
/* The hat bit a Kirby shows once it holds `kind`'s power: catch selects the
 * donor's copy_id, CopyInitCopyVars then uses that row's modelpart (this also
 * handles source aliases). 0 = no hat. */
static u32 ndsKirbyHatBitForKind(const FTKirbyCopy *copy, s32 kind)
{
    s32 part;

    if ((kind < 0) || (kind >= nFTKindEnumCount))
        ndsKirbyHatResidencyHalt(2u);
    kind = copy[kind].copy_id;
    if ((kind < 0) || (kind >= nFTKindEnumCount))
        ndsKirbyHatResidencyHalt(2u);
    part = copy[kind].copy_modelpart_id;
    if (part == 0) { return 0u; }
    if ((part < 3) || (part > 13)) { ndsKirbyHatResidencyHalt(3u); }
    return 1u << part;
}

static const FTKirbyCopy *ndsKirbyHatCopyTable(void)
{
    if (gFTDataKirbyMainMotion == NULL) { ndsKirbyHatResidencyHalt(1u); }
    return lbRelocGetFileData(FTKirbyCopy *, gFTDataKirbyMainMotion,
                              &llKirbyMainMotionSpecialNFTKirbyCopy);
}

/* Every power a Kirby can take from the closed set of this match's fighters
 * (a Kirby can transfer a power it acquired from the same set). */
static u32 ndsKirbyHatPlayerMask(const FTKirbyCopy *copy)
{
    u32 mask = 0u;
    s32 player;

    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if (gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot)
            continue;
        mask |= ndsKirbyHatBitForKind(copy,
            gSCManagerBattleState->players[player].fkind);
    }
    return mask;
}

static u32 ndsKirbyHatCountKirbys(void)
{
    u32 kirbys = 0u;
    s32 player;

    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if ((gSCManagerBattleState->players[player].pkind != nFTPlayerKindNot) &&
            (gSCManagerBattleState->players[player].fkind == nFTKindKirby))
            kirbys++;
    }
    return kirbys;
}
#endif

static void ndsKirbyHatAdmit(u32 high, u32 low)
{
    gNdsKirbyHatRequiredHighMask = high;
    gNdsKirbyHatRequiredLowMask = low;
    if (ndsRendererNativePrepareKirbyHatMatch(high, low) == FALSE)
        ndsKirbyHatResidencyHalt(4u);
}

void ndsKirbyHatPrepareMatch(void)
{
    u32 high = 0u;
    u32 low = 0u;
#if NDS_P2_KIRBY
    if (ndsKirbyHatCountKirbys() != 0u)
    {
        high = ndsKirbyHatPlayerMask(ndsKirbyHatCopyTable());
        /* The source selects low detail for three or four fighters, and can
         * temporarily raise it for KO/pause. High-only matches never lower it. */
        if ((gSCManagerBattleState->pl_count + gSCManagerBattleState->cp_count)
                >= 3) { low = high; }
    }
#endif
    ndsKirbyHatAdmit(high, low);
}

#if NDS_P2_1P_GAME
void ndsKirbyHatPrepare1PMatch(const u8 *team_copy_kinds, u32 team_count,
                               s32 team_final_copy)
{
    u32 high = 0u;
    u32 low = 0u;
#if NDS_P2_KIRBY
    if (ndsKirbyHatCountKirbys() != 0u)
    {
        const FTKirbyCopy *copy = ndsKirbyHatCopyTable();
        u32 mask = ndsKirbyHatPlayerMask(copy);
        u32 i;

        /* The Kirby Team (sc1pgame.c:1203-1219, 1356-1360): each wave's
         * member spawns holding the next row of the team's copy table, the
         * last one the manager's final copy, so those powers are on screen
         * whatever the fighters in the opening wave hold. */
        for (i = 0u; (team_copy_kinds != NULL) && (i < team_count); i++)
        {
            mask |= ndsKirbyHatBitForKind(copy, (s32)team_copy_kinds[i]);
        }
        if (team_copy_kinds != NULL)
        {
            mask |= ndsKirbyHatBitForKind(copy, team_final_copy);
        }
        /* A ladder fight's detail follows its fighter count at each spawn
         * (sc1pgame.c:1379, 2139) and the closing zoom raises the player
         * (sc1PGameSetCameraZoom), so both details stay admitted. */
        high = mask;
        if ((gSCManagerBattleState->pl_count + gSCManagerBattleState->cp_count)
                >= 3) { low = mask; }
    }
#else
    (void)team_copy_kinds;
    (void)team_count;
    (void)team_final_copy;
#endif
    ndsKirbyHatAdmit(high, low);
}
#endif
