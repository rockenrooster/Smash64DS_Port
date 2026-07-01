#include <ft/fighter.h>
#include <gr/ground.h>
#include <sys/obj.h>

#define ftCommonOttottoProcUpdate ndsBaseFTCommonOttottoProcUpdate
#define ftCommonOttottoProcInterrupt ndsBaseFTCommonOttottoProcInterrupt
#define ftCommonOttottoProcMap ndsBaseFTCommonOttottoProcMap
#define ftCommonOttottoWaitSetStatus ndsBaseFTCommonOttottoWaitSetStatus
#define ftCommonOttottoSetStatus ndsBaseFTCommonOttottoSetStatus

void ndsBaseFTCommonOttottoProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonottotto.c"

#undef ftCommonOttottoProcUpdate
#undef ftCommonOttottoProcInterrupt
#undef ftCommonOttottoProcMap
#undef ftCommonOttottoWaitSetStatus
#undef ftCommonOttottoSetStatus
