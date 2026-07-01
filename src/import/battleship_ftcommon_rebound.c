/*
 * Bounded BattleShip ftcommonrebound.c import for NDS rebound proofs.
 * Public symbols are remapped so the port backend can gate the original path.
 */
#include <ft/fighter.h>

#define ftCommonReboundProcUpdate ndsBaseFTCommonReboundProcUpdate
#define ftCommonReboundSetStatus ndsBaseFTCommonReboundSetStatus
#define ftCommonReboundWaitProcUpdate ndsBaseFTCommonReboundWaitProcUpdate
#define ftCommonReboundWaitSetStatus ndsBaseFTCommonReboundWaitSetStatus

void ndsBaseFTCommonReboundProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonReboundSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonReboundWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonReboundWaitSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonrebound.c"

#undef ftCommonReboundProcUpdate
#undef ftCommonReboundSetStatus
#undef ftCommonReboundWaitProcUpdate
#undef ftCommonReboundWaitSetStatus
