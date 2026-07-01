/*
 * Bounded BattleShip ftcommondash.c import for the NDS Mario/Fox
 * Dash -> Run proof. Public symbols are remapped so the port backend can
 * guard the original path and keep unrelated movement branches deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>

#define ftCommonDashProcUpdate ndsBaseFTCommonDashProcUpdate
#define ftCommonDashProcInterrupt ndsBaseFTCommonDashProcInterrupt
#define ftCommonDashProcPhysics ndsBaseFTCommonDashProcPhysics
#define ftCommonDashProcMap ndsBaseFTCommonDashProcMap
#define ftCommonDashSetStatus ndsBaseFTCommonDashSetStatus
#define ftCommonDashCheckInterruptCommon ndsBaseFTCommonDashCheckInterruptCommon
#define ftCommonDashCheckTurn ndsBaseFTCommonDashCheckTurn

void ndsBaseFTCommonDashProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDashSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ndsBaseFTCommonDashCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDashCheckTurn(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondash.c"

#undef ftCommonDashProcUpdate
#undef ftCommonDashProcInterrupt
#undef ftCommonDashProcPhysics
#undef ftCommonDashProcMap
#undef ftCommonDashSetStatus
#undef ftCommonDashCheckInterruptCommon
#undef ftCommonDashCheckTurn
