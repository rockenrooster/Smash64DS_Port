#ifndef _FTFOX_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxstatus.h"
#else
#define _FTFOX_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTFoxSpecialStatusDescs[] = {
    NDS_FT_STATUS_ENTRY(nFTFoxMotionAttack100Start,
                        nFTMotionAttackIDAttack100, nMPKineticsGround,
                        FALSE, nFTStatusAttackIDAttack100,
                        ftCommonAttack100StartProcUpdate, NULL,
                        ftPhysicsApplyGroundVelFriction,
                        mpCommonSetFighterFallOnEdgeBreak),
    NDS_FT_STATUS_ENTRY(nFTFoxMotionAttack100Loop,
                        nFTMotionAttackIDAttack100, nMPKineticsGround,
                        FALSE, nFTStatusAttackIDAttack100,
                        ftCommonAttack100LoopProcUpdate,
                        ftCommonAttack100LoopProcInterrupt,
                        ftPhysicsApplyGroundVelFriction,
                        mpCommonSetFighterFallOnEdgeBreak),
    NDS_FT_STATUS_ENTRY(nFTFoxMotionAttack100End,
                        nFTMotionAttackIDAttack100, nMPKineticsGround,
                        FALSE, nFTStatusAttackIDAttack100, ftAnimEndSetWait,
                        NULL, ftPhysicsApplyGroundVelFriction,
                        mpCommonSetFighterFallOnEdgeBreak),
    NDS_FT_STATUS_STUB16
};

#endif

#endif
