#include <ft/fighter.h>
#include <sys/obj.h>

#define ftCommonFallProcInterrupt ndsBaseFTCommonFallProcInterrupt
#define ftCommonFallSetStatus ndsBaseFTCommonFallSetStatus

#if NDS_P4
#include <nds/nds_p4.h>
/* P4: Remix's check_float_ (PeachSpecial.asm) runs in place of this
 * interrupt's jump check, after the aerial check: it is tried where the
 * aerial check ends (ndsP4AirJumpCheck). Its fall_override_ takes
 * ftCommonFallSetStatus' status change, the file's only one. */
#define ftCommonAttackAirCheckInterruptCommon(g_) \
    (ftCommonAttackAirCheckInterruptCommon(g_) || ndsP4AirJumpCheck(g_))
#define ftMainSetStatus(g_, s_, f_, a_, p_) ndsP4FallSetStatus(g_, s_, f_, a_, p_)
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonfall.c"
#if NDS_P4
#undef ftCommonAttackAirCheckInterruptCommon
#undef ftMainSetStatus
#endif

#undef ftCommonFallProcInterrupt
#undef ftCommonFallSetStatus
