
#include <ft/fighter.h>

#ifndef FTMARIO_SUPERJUMP_TURN_STICK_THRESHOLD
#define FTMARIO_SUPERJUMP_TURN_STICK_THRESHOLD 50
#endif
#ifndef FTMARIO_SUPERJUMP_TURN_MODEL_THRESHOLD
#define FTMARIO_SUPERJUMP_TURN_MODEL_THRESHOLD 20
#endif
#ifndef FTMARIO_SUPERJUMP_AIR_DRIFT
#define FTMARIO_SUPERJUMP_AIR_DRIFT 0.6F
#endif
#ifndef FTMARIO_SUPERJUMP_LANDING_LAG
#define FTMARIO_SUPERJUMP_LANDING_LAG 0.28F
#endif
#ifndef FTMARIO_SUPERJUMP_STICK_Y_UNK
#define FTMARIO_SUPERJUMP_STICK_Y_UNK -44
#endif

#if NDS_P2_MMARIO
#define ftMarioSpecialHiProcPhysics ndsBaseFTMarioSpecialHiProcPhysics
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialhi.c"
#if NDS_P2_MMARIO
#undef ftMarioSpecialHiProcPhysics

sb32 ndsP7MMarioVsFighter(const FTStruct *fp);
f32 ndsP7MMarioVsUpBRise(void);

/* VS Metal Mario's Super Jump Punch rises slower, so lower (owner
 * 2026-10-23: battleship_ftmanager.c NDS_P7_MMARIO_VS_UPB_RISE). */
void ftMarioSpecialHiProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsBaseFTMarioSpecialHiProcPhysics(fighter_gobj);
    if ((fp->ga == nMPKineticsAir) && (fp->physics.vel_air.y > 0.0F) &&
        (ndsP7MMarioVsFighter(fp) != FALSE))
    {
        fp->physics.vel_air.y *= ndsP7MMarioVsUpBRise();
    }
}
#endif

