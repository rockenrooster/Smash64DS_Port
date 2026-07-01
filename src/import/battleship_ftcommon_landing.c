#include <ft/fighter.h>
#include <sys/obj.h>

#define ftCommonLandingProcInterrupt ndsBaseFTCommonLandingProcInterrupt
#define ftCommonLandingSetStatusParam ndsBaseFTCommonLandingSetStatusParam
#define ftCommonLandingSetStatus ndsBaseFTCommonLandingSetStatus
#define ftCommonLandingAirNullSetStatus ndsBaseFTCommonLandingAirNullSetStatus
#define ftCommonLandingAirSetStatus ndsBaseFTCommonLandingAirSetStatus
#define ftCommonLandingFallSpecialSetStatus \
    ndsBaseFTCommonLandingFallSpecialSetStatus

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonlanding.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonlandingair.c"

#undef ftCommonLandingProcInterrupt
#undef ftCommonLandingSetStatusParam
#undef ftCommonLandingSetStatus
#undef ftCommonLandingAirNullSetStatus
#undef ftCommonLandingAirSetStatus
#undef ftCommonLandingFallSpecialSetStatus
