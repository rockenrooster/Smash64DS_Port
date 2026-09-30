/* P2-3 Kirby runtime state machine: BattleShip specials verbatim.
 *
 * Inhale (the vacuum, the two-body swallow, spit-as-star and Copy), Final
 * Cutter, Stone and his forward throw keep their source status bodies as the
 * behavioral authority. The copied neutral specials are in
 * battleship_kirby_copy.c, the Final Cutter beam in battleship_kirby_weapons.c
 * and the victim half of Inhale in battleship_ftcommon_capturekirby.c. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/develop.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#include "battleship_kirby_common.h"

#if NDS_P4_METAKNIGHT
#include <nds/nds_p4_roster.h>
#include "../../builds/p4/meta-knight-lifecycle/nds_meta_lifecycle.generated.h"

static s16 ndsMetaKirbyVictimCopyID(FTStruct *victim, const FTKirbyCopy *copy)
{
    if (victim->fkind == (s32)NDS_P4_RUNTIME_METAKNIGHT)
    {
        /* Source bool_inhale_copy FALSE resolves to Kirby/NONE. Keep the
         * victim's own kind/model/status; only the copied ability is none. */
        return NDS_META_KIRBY_COPY_ID;
    }
    return ((u32)victim->fkind < (u32)nFTKindEnumCount) ?
        copy[victim->fkind].copy_id : nFTKindKirby;
}

#include "../../builds/p4/meta-knight-lifecycle/ftkirbyspecialn.c"
#else
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspecialn.c"
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspeciallw.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbythrowf.c"
