#if NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI

#include <ft/fighter.h>

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

#endif
