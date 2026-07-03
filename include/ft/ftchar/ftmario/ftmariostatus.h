#ifndef _FTMARIO_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariostatus.h"
#else
#define _FTMARIO_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTMarioSpecialStatusDescs[] = {
    NDS_FT_STATUS_ENTRY(nFTMarioMotionAttack13, nFTMotionAttackIDAttack13,
                        nMPKineticsGround, FALSE,
                        nFTStatusAttackIDAttack13, ftAnimEndSetWait, NULL,
                        ftPhysicsApplyGroundVelFriction,
                        mpCommonSetFighterFallOnEdgeBreak),
    NDS_FT_STATUS_STUB16
};

#endif

#endif
