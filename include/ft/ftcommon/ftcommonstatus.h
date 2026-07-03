#ifndef _FTCOMMON_STATUS_H_
#define _FTCOMMON_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

#define NDS_FT_COMMON_ACTION_STATUS_COUNT \
    (nFTCommonStatusSpecialStart - nFTCommonStatusActionStart)
#define NDS_FT_ACTION_INDEX(status_) ((status_) - nFTCommonStatusActionStart)

#define NDS_FT_STATUS_GROUND_DAMAGE(motion_) \
    NDS_FT_STATUS_ENTRY((motion_), nFTMotionAttackIDNone, nMPKineticsGround, \
                        FALSE, nFTStatusAttackIDNone, \
                        ftCommonDamageCommonProcUpdate, \
                        ftCommonDamageCommonProcInterrupt, \
                        ftCommonDamageCommonProcPhysics, \
                        mpCommonProcFighterOnCliffEdge)

#define NDS_FT_STATUS_AIR_DAMAGE(motion_) \
    NDS_FT_STATUS_ENTRY((motion_), nFTMotionAttackIDNone, nMPKineticsGround, \
                        FALSE, nFTStatusAttackIDNone, \
                        ftCommonDamageAirCommonProcUpdate, \
                        ftCommonDamageAirCommonProcInterrupt, \
                        ftCommonDamageCommonProcPhysics, \
                        ftCommonDamageAirCommonProcMap)

FTStatusDesc dFTCommonNullStatusDescs[nFTCommonStatusActionStart] = {
    [0 ... nFTCommonStatusActionStart - 1] = NDS_FT_STATUS_STUB
};

FTStatusDesc dFTCommonActionStatusDescs[NDS_FT_COMMON_ACTION_STATUS_COUNT] = {
    [0 ... NDS_FT_COMMON_ACTION_STATUS_COUNT - 1] = NDS_FT_STATUS_STUB,

    [NDS_FT_ACTION_INDEX(nFTCommonStatusWait)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionWait, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonWaitProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonProcFighterOnCliffEdge),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusWalkSlow)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionWalkSlow, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonWalkProcInterrupt,
                            ftCommonWalkProcPhysics,
                            mpCommonProcFighterOnCliffEdge),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusWalkMiddle)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionWalkMiddle, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonWalkProcInterrupt,
                            ftCommonWalkProcPhysics,
                            mpCommonProcFighterOnCliffEdge),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusWalkFast)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionWalkFast, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonWalkProcInterrupt,
                            ftCommonWalkProcPhysics,
                            mpCommonProcFighterOnCliffEdge),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusWalkEnd)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionWalkEnd, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL, NULL, NULL, NULL),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusDash)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionDash, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftCommonDashProcUpdate,
                            ftCommonDashProcInterrupt,
                            ftCommonDashProcPhysics, ftCommonDashProcMap),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusRun)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionRun, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonRunProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusRunBrake)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionRunBrake, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetWait,
                            ftCommonRunBrakeProcInterrupt,
                            ftCommonRunBrakeProcPhysics,
                            mpCommonProcFighterOnCliffEdge),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusTurn)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionTurn, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftCommonTurnProcUpdate,
                            ftCommonTurnProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusTurnRun)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionTurnRun, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonTurnRunProcUpdate,
                            ndsBaseFTCommonTurnRunProcInterrupt,
                            ftPhysicsApplyGroundVelTransN,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusKneeBend)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionKneeBend, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ftCommonKneeBendProcUpdate,
                            ftCommonKneeBendProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusGuardKneeBend)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionGuardKneeBend,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ftCommonKneeBendProcUpdate,
                            ftCommonKneeBendProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusJumpF)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionJumpF, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetFall,
                            ftCommonJumpProcInterrupt,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusJumpB)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionJumpB, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetFall,
                            ftCommonJumpProcInterrupt,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusJumpAerialF)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionJumpAerialF,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL, NULL,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusJumpAerialB)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionJumpAerialB,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL, NULL,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusFall)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionFall, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonFallProcInterrupt,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusFallAerial)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionFallAerial,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonFallProcInterrupt,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusSquat)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionSquat, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonSquatProcUpdate,
                            ndsBaseFTCommonSquatProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusSquatWait)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionSquatWait,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonSquatWaitProcUpdate,
                            ndsBaseFTCommonSquatWaitProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusSquatRv)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionSquatRv, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetWait,
                            ndsBaseFTCommonSquatRvProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusLandingLight)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionLandingLight,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetWait,
                            ftCommonLandingProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonProcFighterOnCliffEdge),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusLandingHeavy)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionLandingHeavy,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetWait,
                            ftCommonLandingProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonProcFighterOnCliffEdge),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusPass)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionPass, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetFall,
                            ndsBaseFTCommonPassProcInterrupt,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusGuardPass)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionGuardPass,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftAnimEndSetFall,
                            ndsBaseFTCommonPassProcInterrupt,
                            ftPhysicsApplyAirVelDriftFastFall,
                            mpCommonProcFighterCliffFloorCeil),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusOttottoWait)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionOttottoWait,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL,
                            ftCommonOttottoProcInterrupt, NULL,
                            ftCommonOttottoProcMap),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusOttotto)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionOttotto,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, ftCommonOttottoProcUpdate,
                            ftCommonOttottoProcInterrupt, NULL,
                            ftCommonOttottoProcMap),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageHi1)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageHi1),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageHi2)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageHi2),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageHi3)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageHi3),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageN1)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageN1),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageN2)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageN2),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageN3)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageN3),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageLw1)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageLw1),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageLw2)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageLw2),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageLw3)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageLw3),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageAir1)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageAir1),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageAir2)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageAir2),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageAir3)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageAir3),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageE1)] =
        NDS_FT_STATUS_GROUND_DAMAGE(nFTCommonMotionDamageE),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageE2)] =
        NDS_FT_STATUS_AIR_DAMAGE(nFTCommonMotionDamageE),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageFlyHi)] =
        NDS_FT_STATUS_AIR_DAMAGE(nFTCommonMotionDamageFlyHi),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageFlyN)] =
        NDS_FT_STATUS_AIR_DAMAGE(nFTCommonMotionDamageFlyN),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageFlyLw)] =
        NDS_FT_STATUS_AIR_DAMAGE(nFTCommonMotionDamageFlyLw),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageFlyTop)] =
        NDS_FT_STATUS_AIR_DAMAGE(nFTCommonMotionDamageFlyTop),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageFlyRoll)] =
        NDS_FT_STATUS_AIR_DAMAGE(nFTCommonMotionDamageFlyRoll),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusWallDamage)] =
        NDS_FT_STATUS_AIR_DAMAGE(nFTCommonMotionWallDamage),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusDamageFall)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionDamageFall,
                            nFTMotionAttackIDNone, nMPKineticsGround,
                            FALSE, nFTStatusAttackIDNone, NULL,
                            ftCommonDamageFallProcInterrupt,
                            ftPhysicsApplyAirVelDriftFastFall,
                            ftCommonDamageFallProcMap),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusTwister)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionTwister,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonTwisterProcUpdate, NULL,
                            ndsBaseFTCommonTwisterProcPhysics, NULL),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusTaruCann)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionNull,
                            nFTMotionAttackIDNone, nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone, NULL, NULL,
                            ftCommonTaruCannProcPhysics, NULL),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusReboundWait)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionNull, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ftCommonReboundWaitProcUpdate, NULL,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),
    [NDS_FT_ACTION_INDEX(nFTCommonStatusRebound)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionRebound,
                            nFTMotionAttackIDNone, nMPKineticsGround,
                            FALSE, nFTStatusAttackIDNone,
                            ftCommonReboundProcUpdate, NULL,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusGuardOn)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionGuardOn, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonGuardOnProcUpdate,
                            ndsBaseFTCommonGuardCommonProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusGuard)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionNull, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonGuardProcUpdate,
                            ndsBaseFTCommonGuardCommonProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusGuardOff)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionGuardOff, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonGuardOffProcUpdate, NULL,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusGuardSetOff)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionNull, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonGuardSetOffProcUpdate, NULL,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnGroundBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusEscapeF)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionEscapeF, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonEscapeProcUpdate,
                            ndsBaseFTCommonEscapeProcInterrupt,
                            ftPhysicsApplyGroundVelTransN,
                            mpCommonSetFighterFallOnEdgeBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusEscapeB)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionEscapeB, nFTMotionAttackIDNone,
                            nMPKineticsGround, FALSE,
                            nFTStatusAttackIDNone,
                            ndsBaseFTCommonEscapeProcUpdate,
                            ndsBaseFTCommonEscapeProcInterrupt,
                            ftPhysicsApplyGroundVelTransN,
                            mpCommonSetFighterFallOnEdgeBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusAttack11)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionAttack11,
                            nFTMotionAttackIDAttack11, nMPKineticsGround,
                            FALSE, nFTStatusAttackIDAttack11,
                            ftCommonAttack11ProcUpdate,
                            ftCommonAttack11ProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnEdgeBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusAttack12)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionAttack12,
                            nFTMotionAttackIDAttack12, nMPKineticsGround,
                            FALSE, nFTStatusAttackIDAttack12,
                            ftCommonAttack12ProcUpdate,
                            ftCommonAttack12ProcInterrupt,
                            ftPhysicsApplyGroundVelFriction,
                            mpCommonSetFighterFallOnEdgeBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusAttackDash)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionAttackDash,
                            nFTMotionAttackIDAttackDash, nMPKineticsGround,
                            FALSE, nFTStatusAttackIDAttackDash,
                            ftAnimEndSetWait, NULL,
                            ftPhysicsApplyGroundVelTransN,
                            mpCommonSetFighterFallOnEdgeBreak),

    [NDS_FT_ACTION_INDEX(nFTCommonStatusAttackAirLw)] =
        NDS_FT_STATUS_ENTRY(nFTCommonMotionAttackAirLw,
                            nFTMotionAttackIDAttackAirLw, nMPKineticsAir,
                            FALSE, nFTStatusAttackIDAttackAirLw,
                            ndsBaseFTCommonAttackAirLwProcUpdate, NULL,
                            ftPhysicsApplyAirVelDrift,
                            ftCommonAttackAirProcMap)
};

#endif
