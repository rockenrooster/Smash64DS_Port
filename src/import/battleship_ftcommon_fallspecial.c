#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI

#include <ft/fighter.h>

#ifndef FTCOMMON_FALLSPECIAL_PASS_STICK_RANGE_MIN
#define FTCOMMON_FALLSPECIAL_PASS_STICK_RANGE_MIN (-44)
#endif
#ifndef FTCOMMON_FALLSPECIAL_SKIPLANDING_VEL_Y_MAX
#define FTCOMMON_FALLSPECIAL_SKIPLANDING_VEL_Y_MAX (-20.0F)
#endif

__attribute__((weak)) void ftPublicTryPlayFallSpecialReact(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

#if NDS_P4
#include <nds/nds_p4.h>
/* P4: Remix's fall_special_patch_ (PeachSpecial.asm) picks the status of
 * ftCommonFallSpecialSetStatus' change, the file's only one. */
#define ftMainSetStatus(g_, s_, f_, a_, p_) ndsP4FallSpecialSetStatus(g_, s_, f_, a_, p_)
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonfallspecial.c"
#if NDS_P4
#undef ftMainSetStatus
#endif

#endif
