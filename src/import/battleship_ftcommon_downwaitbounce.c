#include <ef/effect.h>
#include <ft/fighter.h>
#include <sys/audio.h>

#define ftCommonDownWaitProcUpdate ndsBaseFTCommonDownWaitProcUpdate
#define ftCommonDownWaitProcInterrupt ndsBaseFTCommonDownWaitProcInterrupt
#define ftCommonDownWaitSetStatus ndsBaseFTCommonDownWaitSetStatus
#define ftCommonDownBounceProcUpdate ndsBaseFTCommonDownBounceProcUpdate
#define ftCommonDownBounceCheckUpOrDown \
    ndsBaseFTCommonDownBounceCheckUpOrDown
#define ftCommonDownBounceUpdateEffects \
    ndsBaseFTCommonDownBounceUpdateEffects
#define ftCommonDownBounceSetStatus ndsBaseFTCommonDownBounceSetStatus

void ndsBaseFTCommonDownWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceProcUpdate(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownBounceCheckUpOrDown(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceUpdateEffects(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondownwaitbounce.c"

#undef ftCommonDownWaitProcUpdate
#undef ftCommonDownWaitProcInterrupt
#undef ftCommonDownWaitSetStatus
#undef ftCommonDownBounceProcUpdate
#undef ftCommonDownBounceCheckUpOrDown
#undef ftCommonDownBounceUpdateEffects
#undef ftCommonDownBounceSetStatus
