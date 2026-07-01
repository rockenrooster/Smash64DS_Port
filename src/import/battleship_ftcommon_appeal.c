/*
 * Bounded BattleShip ftcommonappeal.c import for NDS common-status proofs.
 * Public symbols are remapped so the port backend can gate original Appeal.
 */
#include <ft/fighter.h>
#include <sys/obj.h>

void ftKirbySpecialNLoseCopy(GObj *fighter_gobj);
sb32 ftCommonCatchCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonGuardOnCheckInterruptCommon(GObj *fighter_gobj);

#define ftCommonAppealProcInterrupt ndsBaseFTCommonAppealProcInterrupt
#define ftCommonAppealSetStatus ndsBaseFTCommonAppealSetStatus
#define ftCommonAppealCheckInterruptCommon \
    ndsBaseFTCommonAppealCheckInterruptCommon

void ndsBaseFTCommonAppealProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAppealSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAppealCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonappeal.c"

#undef ftCommonAppealProcInterrupt
#undef ftCommonAppealSetStatus
#undef ftCommonAppealCheckInterruptCommon
