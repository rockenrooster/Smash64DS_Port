#include <ft/fighter.h>

/* These constants live in BattleShip's decomp-internal ftcommon.h rather than
 * the port ABI mirror.  Preserve the source values verbatim. */
#ifndef FTCOMMON_FURAFURA_BREAKOUT_WAIT_DEFAULT
#define FTCOMMON_FURAFURA_BREAKOUT_WAIT_DEFAULT 400
#endif
#ifndef FTCOMMON_FURAFURA_BREAKOUT_WAIT_MIN
#define FTCOMMON_FURAFURA_BREAKOUT_WAIT_MIN 90
#endif

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonfurafura.c"
