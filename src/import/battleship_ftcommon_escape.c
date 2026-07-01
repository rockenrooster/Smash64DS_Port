/*
 * Bounded BattleShip ftcommonescape.c import for NDS guard-escape proofs.
 * Public symbols are remapped so the port backend can gate original Escape.
 */
#include <ft/fighter.h>
#include <sys/obj.h>

sb32 ftCommonGuardCheckInterruptEscape(GObj *fighter_gobj);
sb32 ftCommonLightThrowCheckInterruptEscape(GObj *fighter_gobj);

#define ftCommonEscapeProcUpdate ndsBaseFTCommonEscapeProcUpdate
#define ftCommonEscapeProcInterrupt ndsBaseFTCommonEscapeProcInterrupt
#define ftCommonEscapeProcStatus ndsBaseFTCommonEscapeProcStatus
#define ftCommonEscapeSetStatus ndsBaseFTCommonEscapeSetStatus
#define ftCommonEscapeGetStatus ndsBaseFTCommonEscapeGetStatus
#define ftCommonEscapeCheckInterruptSpecialNDonkey \
    ndsBaseFTCommonEscapeCheckInterruptSpecialNDonkey
#define ftCommonEscapeCheckInterruptDash \
    ndsBaseFTCommonEscapeCheckInterruptDash
#define ftCommonEscapeCheckInterruptGuard \
    ndsBaseFTCommonEscapeCheckInterruptGuard

void ndsBaseFTCommonEscapeProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeProcStatus(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeSetStatus(GObj *fighter_gobj, s32 status_id,
                                    s32 itemthrow_buffer_tics);
s32 ndsBaseFTCommonEscapeGetStatus(FTStruct *fp);
sb32 ndsBaseFTCommonEscapeCheckInterruptGuard(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonescape.c"

#undef ftCommonEscapeProcUpdate
#undef ftCommonEscapeProcInterrupt
#undef ftCommonEscapeProcStatus
#undef ftCommonEscapeSetStatus
#undef ftCommonEscapeGetStatus
#undef ftCommonEscapeCheckInterruptSpecialNDonkey
#undef ftCommonEscapeCheckInterruptDash
#undef ftCommonEscapeCheckInterruptGuard
