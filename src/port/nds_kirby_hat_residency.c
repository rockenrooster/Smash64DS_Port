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

void ndsKirbyHatPrepareMatch(void)
{
    u32 high = 0u;
    u32 low = 0u;
#if NDS_P2_KIRBY
    u32 kirbys = 0u;
    s32 player;
    const FTKirbyCopy *copy;

    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        if (gSCManagerBattleState->players[player].pkind != nFTPlayerKindNot)
        {
            if (gSCManagerBattleState->players[player].fkind == nFTKindKirby)
                kirbys++;
        }
    }
    if (kirbys != 0u)
    {
        if (gFTDataKirbyMainMotion == NULL) { ndsKirbyHatResidencyHalt(1u); }
        copy = lbRelocGetFileData(FTKirbyCopy *, gFTDataKirbyMainMotion,
                                 &llKirbyMainMotionSpecialNFTKirbyCopy);
        for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
        {
            s32 kind = gSCManagerBattleState->players[player].fkind;
            s32 part;
            if (gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot)
                continue;
            if ((kind < 0) || (kind >= nFTKindEnumCount))
                ndsKirbyHatResidencyHalt(2u);
            /* Catch selects the donor's copy_id; CopyInitCopyVars then uses
             * that row's modelpart. This also handles source aliases. A Kirby
             * can transfer a power it acquired from this same closed VS set. */
            kind = copy[kind].copy_id;
            if ((kind < 0) || (kind >= nFTKindEnumCount))
                ndsKirbyHatResidencyHalt(2u);
            part = copy[kind].copy_modelpart_id;
            if (part == 0) { continue; }
            if ((part < 3) || (part > 13)) { ndsKirbyHatResidencyHalt(3u); }
            high |= 1u << part;
        }
        /* The source selects low detail for three or four fighters, and can
         * temporarily raise it for KO/pause. High-only matches never lower it. */
        if ((gSCManagerBattleState->pl_count + gSCManagerBattleState->cp_count)
                >= 3) { low = high; }
    }
#endif
    gNdsKirbyHatRequiredHighMask = high;
    gNdsKirbyHatRequiredLowMask = low;
    if (ndsRendererNativePrepareKirbyHatMatch(high, low) == FALSE)
        ndsKirbyHatResidencyHalt(4u);
}
