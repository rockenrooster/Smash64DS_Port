#include "nds_build_config.h"
#include <ft/fighter.h>
#include <sc/scene.h>
#include <reloc_data.h>
#include <nds/arm9/cache.h>
#include <nds/nds_kirby_hat_residency.h>
#include <nds/nds_renderer.h>
#include <nds/nds_p4.h>

volatile u32 gNdsKirbyHatRequiredHighMask;
volatile u32 gNdsKirbyHatRequiredLowMask;
volatile u32 gNdsKirbyHatAdmissionFailure;
volatile u32 gNdsKirbyHatMissingCount;

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
 * (a Kirby can transfer a power it acquired from the same set): the source
 * cast's powers' hats, and in `*p4_mask` the source hats the contents' powers
 * show. */
static u32 ndsKirbyHatPlayerMask(const FTKirbyCopy *copy, u32 *p4_mask)
{
    u32 mask = 0u;
    s32 player;

    *p4_mask = 0u;
    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if (gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot)
            continue;
#if NDS_P4
        /* P4 S8: a content player's power shows its own hats: the source
         * hats here, its Remix hats from their own images
         * (ndsKirbyHatPrepareMatch); never its parent's copy hat. */
        if (ndsP4KirbyPlayerHasPower(player) != FALSE)
        {
            *p4_mask |= ndsP4KirbyHatMask(player);
            continue;
        }
        /* A content Kirby cannot copy shows no hat (its inhale row). */
        if (ndsP4KirbyPlayerNoCopy(player) != FALSE)
        {
            continue;
        }
#endif
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

sb32 ndsFTManagerLowDetailOnly(s32 player);

/* Owner 2026-10-19: a Kirby kept low in a VS battle of three or four (a CPU,
 * or a wireless guest: ndsFTManagerLowDetailOnly) keeps its low detail in the
 * close-ups, so its hats load low only; the high ones are for a Kirby that can
 * be drawn high. */
static u32 ndsKirbyHatHighReachable(void)
{
    s32 player;

    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if ((gSCManagerBattleState->players[player].pkind != nFTPlayerKindNot) &&
            (gSCManagerBattleState->players[player].fkind == nFTKindKirby) &&
            (ndsFTManagerLowDetailOnly(player) == FALSE))
            return TRUE;
    }
    return FALSE;
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
        /* Every VS battle draws its fighters' low detail (owner 2026-10-01,
         * ndsFTManagerBattleLowDetail), not only one of three or four as the
         * source picks, so the hats load in the low detail in every match:
         * admitted high only, a Remix hat left a Kirby holding its power
         * undrawn in a match of one or two (owner, play-1012y). */
        u32 with_low = 1u;

        u32 p4 = 0u;

        high = ndsKirbyHatPlayerMask(ndsKirbyHatCopyTable(), &p4);
        /* The source selects low detail for three or four fighters, and can
         * temporarily raise it for KO/pause. High-only matches never lower it.
         * A Kirby holding a content's power stays low in those close-ups
         * (ftParamSetModelPartDetailAll, owner 2026-10-08), so the source hats
         * its power shows load in low detail only. */
        if (with_low != 0u) { low = high | p4; }
        else { high |= p4; }
        if (ndsKirbyHatHighReachable() == FALSE) { high = 0u; }
#if NDS_P4
        /* A power's Remix hats load in the detail the match draws: a Kirby
         * holding one stays low in the close-ups of a battle of three or four
         * (ftParamSetModelPartDetailAll), as a content does (owner
         * 2026-10-08: lower content fidelity rather than a budget). */
        {
            u8 hats[8];
            u32 kirby_slots = 0u;
            s32 player;

            for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
            {
                if ((gSCManagerBattleState->players[player].pkind != nFTPlayerKindNot) &&
                    (gSCManagerBattleState->players[player].fkind == nFTKindKirby))
                {
                    kirby_slots |= 1u << player;
                }
            }
            ndsRendererNativeKirbyHatSetKirbySlots(kirby_slots);
            ndsRendererNativeKirbyHatSetP4Hats(
                hats, ndsP4KirbyMatchHats(hats, sizeof(hats)), with_low);
        }
#endif
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
        u32 p4 = 0u;
        u32 mask = ndsKirbyHatPlayerMask(copy, &p4) | p4;
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
