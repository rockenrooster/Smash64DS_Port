#if NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI

#include <ft/fighter.h>

#if NDS_P4_FALCO
/* P4 Falco runs this code with fkind == Fox. Falco.asm patches three Fox
 * constants for him: up_special_delay_ (launch delay 0x16, not 35) and
 * up_special_velocity_1/2/3_ (98.0, not 115.0). Two of the velocity sites read
 * FTFOX_FIREFOX_VEL; the grounded one in ftFoxSpecialHiDecideSetStatus is a
 * literal 115.0F, rewritten by the angle call that immediately follows it. */
#include <nds/nds_p4.h>
#include <sys/utils.h> /* declare the real syUtilsArcTan2 before the hook */
s32 ndsP4FoxFirefoxLaunchDelay(const FTStruct *fp, s32 fox_delay);
f32 ndsP4FoxFirefoxVel(const FTStruct *fp, f32 fox_vel);
f32 ndsP4FoxSpecialHiArcTan2(FTStruct *fp, f32 y, f32 x, sb32 is_ground_launch);
#define FTFOX_FIREFOX_LAUNCH_DELAY ndsP4FoxFirefoxLaunchDelay(fp, 35)
#define FTFOX_FIREFOX_VEL ndsP4FoxFirefoxVel(fp, 115.0F)
#define syUtilsArcTan2(y, x)                                                  \
    ndsP4FoxSpecialHiArcTan2(fp, (y), (x),                                    \
        sizeof(__func__) == sizeof("ftFoxSpecialHiDecideSetStatus"))
#endif

#ifndef FTFOX_FIREFOX_LAUNCH_DELAY
#define FTFOX_FIREFOX_LAUNCH_DELAY 35
#endif
#ifndef FTFOX_FIREFOX_GRAVITY_DELAY
#define FTFOX_FIREFOX_GRAVITY_DELAY 15
#endif
#ifndef FTFOX_FIREFOX_DECELERATE_DELAY
#define FTFOX_FIREFOX_DECELERATE_DELAY 2
#endif
#ifndef FTFOX_FIREFOX_DECELERATE_VEL
#define FTFOX_FIREFOX_DECELERATE_VEL 3.03571438789F
#endif
#ifndef FTFOX_FIREFOX_DECELERATE_END
#define FTFOX_FIREFOX_DECELERATE_END 1.5F
#endif
#ifndef FTFOX_FIREFOX_BOUND_ANGLE
#define FTFOX_FIREFOX_BOUND_ANGLE F_CLC_DTOR32(20.0F)
#endif
#ifndef FTFOX_FIREFOX_TRAVEL_TIME
#define FTFOX_FIREFOX_TRAVEL_TIME 30
#endif
#ifndef FTFOX_FIREFOX_VEL
#define FTFOX_FIREFOX_VEL 115.0F
#endif
#ifndef FTFOX_FIREFOX_ANGLE_STICK_THRESHOLD
#define FTFOX_FIREFOX_ANGLE_STICK_THRESHOLD 45
#endif
#ifndef FTFOX_FIREFOX_MODEL_STICK_THRESHOLD
#define FTFOX_FIREFOX_MODEL_STICK_THRESHOLD 11
#endif
#ifndef FTFOX_FIREFOX_AIR_DRIFT
#define FTFOX_FIREFOX_AIR_DRIFT 1.0F
#endif
#ifndef FTFOX_FIREFOX_LANDING_LAG
#define FTFOX_FIREFOX_LANDING_LAG 0.34F
#endif

void ftFoxSpecialHiHoldSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirHiHoldSetStatus(GObj *fighter_gobj);
void ftFoxSpecialHiStartSwitchStatusAir(GObj *fighter_gobj);
void ftFoxSpecialAirHiStartSwitchStatusGround(GObj *fighter_gobj);
void ftFoxSpecialAirHiSetStatusFromGround(GObj *fighter_gobj);
void ftFoxSpecialHiDecideSetStatus(GObj *fighter_gobj);
void ftFoxSpecialHiHoldSwitchStatusAir(GObj *fighter_gobj);
void ftFoxSpecialAirHiHoldSwitchStatusGround(GObj *fighter_gobj);
void ftFoxSpecialAirHiEndSetStatus(GObj *fighter_gobj);
void ftFoxSpecialHiEndSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirHiSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirHiBoundSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirHiEndSwitchStatusGround(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspecialhi.c"
#if NDS_P4_FALCO
#undef syUtilsArcTan2
#endif

#endif
