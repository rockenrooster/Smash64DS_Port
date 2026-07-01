#include <ft/fighter.h>
#include <sys/obj.h>

#define ftCommonFallProcInterrupt ndsBaseFTCommonFallProcInterrupt
#define ftCommonFallSetStatus ndsBaseFTCommonFallSetStatus

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonfall.c"

#undef ftCommonFallProcInterrupt
#undef ftCommonFallSetStatus
