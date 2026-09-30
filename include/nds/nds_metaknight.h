#ifndef NDS_METAKNIGHT_H
#define NDS_METAKNIGHT_H

#include <ft/fighter.h>
#include <nds/nds_metaknight_types.h>

/* Pinned JIGGLYPUFF base action array plus the sixteen add_new_action rows
 * in EXTRA MetaKnight/main.asm. The resolved donor export must verify this
 * identity mapping before these callbacks are bound to production tables. */
enum NDSMetaKnightStatus {
    nNDSMetaKnightStatusUSP = 0xDC,
    nNDSMetaKnightStatusUSPAirStart = 0xDD,
    nNDSMetaKnightStatusUSPAir = 0xDE,
    nNDSMetaKnightStatusJump2 = 0xDF,
    nNDSMetaKnightStatusJump3 = 0xE0,
    nNDSMetaKnightStatusJump4 = 0xE1,
    nNDSMetaKnightStatusJump5 = 0xE2,
    nNDSMetaKnightStatusJump6 = 0xE3,
    nNDSMetaKnightStatusTornadoStart = 0xE6,
    nNDSMetaKnightStatusTornadoLoop = 0xE7,
    nNDSMetaKnightStatusTornadoEnd = 0xE8,
    nNDSMetaKnightStatusTornadoEndAir = 0xEA,
    nNDSMetaKnightStatusJabStart = 0xEC,
    nNDSMetaKnightStatusJabLoop = 0xED,
    nNDSMetaKnightStatusJabEnd = 0xEE,
    nNDSMetaKnightStatusTiltF = 0xEF,
    nNDSMetaKnightStatusTiltF2 = 0xF0,
    nNDSMetaKnightStatusTiltF3 = 0xF1,
    nNDSMetaKnightStatusCapeStart = 0xF2,
    nNDSMetaKnightStatusCapeEnd = 0xF3,
    nNDSMetaKnightStatusCapeF = 0xF4,
    nNDSMetaKnightStatusCapeB = 0xF5,
    nNDSMetaKnightStatusCapeN = 0xF6,
    nNDSMetaKnightStatusCapeAirStart = 0xF7,
    nNDSMetaKnightStatusCapeAirEnd = 0xF8,
    nNDSMetaKnightStatusCapeAirF = 0xF9,
    nNDSMetaKnightStatusCapeAirB = 0xFA,
    nNDSMetaKnightStatusCapeAirN = 0xFB
};

/* Required native material seam. TRUE projects source CharEnvColor's
 * moveset-table value 0xFFFFFF00; FALSE clears only that moveset override.
 * The integrator must clear it at the normal status-reset boundary and at
 * scene teardown. It must also cover the player indicator and wing materials.
 * There is deliberately no weak/no-op substitute for this required output. */
void ndsMetaKnightSetCapeEnvironment(GObj *fighter_gobj, sb32 hidden);
sb32 ndsMetaKnightCapeHidden(const FTStruct *fp);
sb32 ndsMetaKnightCheckSpecialN(GObj *fighter_gobj);
sb32 ndsMetaKnightCheckSpecialHi(GObj *fighter_gobj);
sb32 ndsMetaKnightCheckSpecialLw(GObj *fighter_gobj);
sb32 ndsMetaKnightCheckSpecialAir(GObj *fighter_gobj);

void ndsMetaKnightInitPassiveVars(GObj *fighter_gobj);
void ndsMetaKnightOnActionChanged(FTStruct *fp, s32 next_status);
sb32 ndsMetaKnightCaptureDKInterrupt(GObj *fighter_gobj);
sb32 ndsMetaKnightWingIsVisible(const FTStruct *fp);
void ndsMetaKnightMultiJumpSetStatus(GObj *fighter_gobj, s32 input_source);
sb32 ndsMetaKnightCheckJumpInterrupt(GObj *fighter_gobj);
void ndsMetaKnightAerialAttackPhysics(GObj *fighter_gobj);
void ndsMetaKnightJabStartUpdate(GObj *fighter_gobj);
void ndsMetaKnightJabLoopUpdate(GObj *fighter_gobj);
void ndsMetaKnightJabLoopRecoil(GObj *fighter_gobj);
void ndsMetaKnightTiltFUpdate(GObj *fighter_gobj);
void ndsMetaKnightUSPAirInitial(GObj *fighter_gobj);
void ndsMetaKnightUSPGroundInitial(GObj *fighter_gobj);
void ndsMetaKnightUSPAirStartUpdate(GObj *fighter_gobj);
void ndsMetaKnightUSPUpdate(GObj *fighter_gobj);
void ndsMetaKnightUSPPhysics(GObj *fighter_gobj);
void ndsMetaKnightUSPInterrupt(GObj *fighter_gobj);
void ndsMetaKnightUSPMap(GObj *fighter_gobj);
void ndsMetaKnightNSPAirInitial(GObj *fighter_gobj);
void ndsMetaKnightNSPGroundInitial(GObj *fighter_gobj);
void ndsMetaKnightNSPStartUpdate(GObj *fighter_gobj);
void ndsMetaKnightNSPLoopUpdate(GObj *fighter_gobj);
void ndsMetaKnightNSPLoopPhysics(GObj *fighter_gobj);
void ndsMetaKnightNSPLoopMap(GObj *fighter_gobj);
void ndsMetaKnightNSPEndAirMap(GObj *fighter_gobj);
void ndsMetaKnightDSPGroundInitial(GObj *fighter_gobj);
void ndsMetaKnightDSPAirInitial(GObj *fighter_gobj);
void ndsMetaKnightDSPStartUpdate(GObj *fighter_gobj);
void ndsMetaKnightDSPStartPhysics(GObj *fighter_gobj);
void ndsMetaKnightDSPStartGroundMap(GObj *fighter_gobj);
void ndsMetaKnightDSPStartAirMap(GObj *fighter_gobj);
void ndsMetaKnightDSPAttackAirUpdate(GObj *fighter_gobj);
void ndsMetaKnightDSPAttackAirPhysics(GObj *fighter_gobj);

#endif
