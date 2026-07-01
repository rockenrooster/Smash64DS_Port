/*
 * Bounded BattleShip ftcommonfurasleep.c import for NDS sleep-damage proofs.
 * Public symbols are remapped so the port backend can gate the original path.
 */
#include <ft/fighter.h>

#define ftCommonFuraSleepProcUpdate ndsBaseFTCommonFuraSleepProcUpdate
#define ftCommonFuraSleepSetStatus ndsBaseFTCommonFuraSleepSetStatus

void ndsBaseFTCommonFuraSleepProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonFuraSleepSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonfurasleep.c"

#undef ftCommonFuraSleepProcUpdate
#undef ftCommonFuraSleepSetStatus
