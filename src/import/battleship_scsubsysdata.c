/* Compile the original BattleShip shared fighter scene data. */
#include <ft/fighter.h>

extern void scSubsysFighterApplyVelTransN(GObj *fighter_gobj);

#define mvOpeningFighterProcUpdate ndsSCSubsysOpeningFighterProcUpdateStub
#include "../../decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdata.c"
#undef mvOpeningFighterProcUpdate

void ndsSCSubsysOpeningFighterProcUpdateStub(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
