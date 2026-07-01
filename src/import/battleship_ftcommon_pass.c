/*
 * Bounded BattleShip ftcommonpass.c / ftcommonsquat.c import for the NDS
 * pass-through input proof. Public symbols are remapped so the port backend
 * can guard the original path and keep unrelated crouch/attack branches
 * deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <nds/nds_startup.h>
#include <sys/obj.h>

#define ftCommonPassProcInterrupt ndsBaseFTCommonPassProcInterrupt
#define ftCommonPassSetStatusParam ndsBaseFTCommonPassSetStatusParam
#define ftCommonPassSetStatusSquat ndsBaseFTCommonPassSetStatusSquat
#define ftCommonGuardPassSetStatus ndsBaseFTCommonGuardPassSetStatus
#define ftCommonPassCheckInputSuccess ndsBaseFTCommonPassCheckInputSuccess
#define ftCommonPassCheckInterruptCommon ndsBaseFTCommonPassCheckInterruptCommon
#define ftCommonPassCheckInterruptSquat ndsBaseFTCommonPassCheckInterruptSquat
#define ftCommonGuardPassCheckInterruptGuard ndsBaseFTCommonGuardPassCheckInterruptGuard
#define ftCommonSquatCheckGotoPass ndsBaseFTCommonSquatCheckGotoPass
#define ftCommonSquatProcUpdate ndsBaseFTCommonSquatProcUpdate
#define ftCommonSquatProcInterrupt ndsBaseFTCommonSquatProcInterrupt
#define ftCommonSquatSetStatusNoPass ndsBaseFTCommonSquatSetStatusNoPass
#define ftCommonSquatSetStatusPass ndsBaseFTCommonSquatSetStatusPass
#define ftCommonSquatCheckInterruptCommon ndsBaseFTCommonSquatCheckInterruptCommon
#define ftCommonSquatWaitProcUpdate ndsBaseFTCommonSquatWaitProcUpdate
#define ftCommonSquatWaitProcInterrupt ndsBaseFTCommonSquatWaitProcInterrupt
#define ftCommonSquatWaitSetStatus ndsBaseFTCommonSquatWaitSetStatus
#define ftCommonSquatWaitSetStatusNoPass ndsBaseFTCommonSquatWaitSetStatusNoPass
#define ftCommonSquatWaitCheckInterruptLanding ndsBaseFTCommonSquatWaitCheckInterruptLanding
#define ftCommonSquatRvProcInterrupt ndsBaseFTCommonSquatRvProcInterrupt
#define ftCommonSquatRvSetStatus ndsBaseFTCommonSquatRvSetStatus
#define ftCommonSquatRvCheckInterruptSquatWait ndsBaseFTCommonSquatRvCheckInterruptSquatWait

void ndsBaseFTCommonPassProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonPassSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                       f32 frame_begin, u32 flags);
void ndsBaseFTCommonPassSetStatusSquat(GObj *fighter_gobj);
void ndsBaseFTCommonGuardPassSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassCheckInputSuccess(FTStruct *fp);
sb32 ndsBaseFTCommonPassCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassCheckInterruptSquat(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardPassCheckInterruptGuard(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatCheckGotoPass(GObj *fighter_gobj);
void ndsBaseFTCommonSquatProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonSquatProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonSquatSetStatusNoPass(GObj *fighter_gobj);
void ndsBaseFTCommonSquatSetStatusPass(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitSetStatusNoPass(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatWaitCheckInterruptLanding(GObj *fighter_gobj);
void ndsBaseFTCommonSquatRvProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonSquatRvSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatRvCheckInterruptSquatWait(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonpass.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonsquat.c"

#undef ftCommonPassProcInterrupt
#undef ftCommonPassSetStatusParam
#undef ftCommonPassSetStatusSquat
#undef ftCommonGuardPassSetStatus
#undef ftCommonPassCheckInputSuccess
#undef ftCommonPassCheckInterruptCommon
#undef ftCommonPassCheckInterruptSquat
#undef ftCommonGuardPassCheckInterruptGuard
#undef ftCommonSquatCheckGotoPass
#undef ftCommonSquatProcUpdate
#undef ftCommonSquatProcInterrupt
#undef ftCommonSquatSetStatusNoPass
#undef ftCommonSquatSetStatusPass
#undef ftCommonSquatCheckInterruptCommon
#undef ftCommonSquatWaitProcUpdate
#undef ftCommonSquatWaitProcInterrupt
#undef ftCommonSquatWaitSetStatus
#undef ftCommonSquatWaitSetStatusNoPass
#undef ftCommonSquatWaitCheckInterruptLanding
#undef ftCommonSquatRvProcInterrupt
#undef ftCommonSquatRvSetStatus
#undef ftCommonSquatRvCheckInterruptSquatWait
