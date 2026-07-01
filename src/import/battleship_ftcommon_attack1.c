/*
 * Bounded BattleShip ftcommonattack1.c import for the NDS Mario/Fox
 * Wait -> Attack11 proof. Public symbols are remapped so the port backend can
 * guard the original path and keep follow-up/item branches deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <sys/obj.h>

#define ftCommonAttack11ProcUpdate ndsBaseFTCommonAttack11ProcUpdate
#define ftCommonAttack12ProcUpdate ndsBaseFTCommonAttack12ProcUpdate
#define ftCommonAttack13ProcUpdate ndsBaseFTCommonAttack13ProcUpdate
#define ftCommonAttack11ProcInterrupt ndsBaseFTCommonAttack11ProcInterrupt
#define ftCommonAttack12ProcInterrupt ndsBaseFTCommonAttack12ProcInterrupt
#define ftCommonAttack13ProcInterrupt ndsBaseFTCommonAttack13ProcInterrupt
#define ftCommonAttack11ProcStatus ndsBaseFTCommonAttack11ProcStatus
#define ftCommonAttack11SetStatus ndsBaseFTCommonAttack11SetStatus
#define ftCommonAttack12SetStatus ndsBaseFTCommonAttack12SetStatus
#define ftCommonAttack13SetStatus ndsBaseFTCommonAttack13SetStatus
#define ftCommonAttack1CheckInterruptCommon \
    ndsBaseFTCommonAttack1CheckInterruptCommon
#define ftCommonAttack11CheckGoto ndsBaseFTCommonAttack11CheckGoto
#define ftCommonAttack12CheckGoto ndsBaseFTCommonAttack12CheckGoto
#define ftCommonAttack13CheckGoto ndsBaseFTCommonAttack13CheckGoto

void ndsBaseFTCommonAttack11ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack13ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11ProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12ProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack13ProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11ProcStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11SetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12SetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack13SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack1CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack11CheckGoto(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack12CheckGoto(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack13CheckGoto(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattack1.c"

#undef ftCommonAttack11ProcUpdate
#undef ftCommonAttack12ProcUpdate
#undef ftCommonAttack13ProcUpdate
#undef ftCommonAttack11ProcInterrupt
#undef ftCommonAttack12ProcInterrupt
#undef ftCommonAttack13ProcInterrupt
#undef ftCommonAttack11ProcStatus
#undef ftCommonAttack11SetStatus
#undef ftCommonAttack12SetStatus
#undef ftCommonAttack13SetStatus
#undef ftCommonAttack1CheckInterruptCommon
#undef ftCommonAttack11CheckGoto
#undef ftCommonAttack12CheckGoto
#undef ftCommonAttack13CheckGoto
