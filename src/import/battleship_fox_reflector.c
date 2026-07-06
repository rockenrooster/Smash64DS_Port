#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR

#include <common.h>
#include <ef/effect.h>
#include <ft/ftdata_file_slots.h>
#include <ft/fighter.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>

#define FTFOX_REFLECTOR_RELEASE_LAG 18
#define FTFOX_REFLECTOR_TURN_FRAMES 4
#define FTFOX_REFLECTOR_GRAVITY_DELAY 4
#define FTFOX_REFLECTOR_GRAVITY 0.8F

#define NDS_REFLECTOR_WEAK_STATUS(name)                \
    __attribute__((weak)) void name(GObj *fighter_gobj) \
    {                                                   \
        (void)fighter_gobj;                             \
    }

NDS_REFLECTOR_WEAK_STATUS(ftMarioSpecialLwSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftDonkeySpecialLwStartSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftSamusSpecialLwSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftLinkSpecialLwSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftYoshiSpecialLwStartSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftCaptainSpecialLwSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftKirbySpecialLwStartSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftPikachuSpecialLwStartSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftPurinSpecialLwSetStatus)
NDS_REFLECTOR_WEAK_STATUS(ftNessSpecialLwStartSetStatus)

#undef NDS_REFLECTOR_WEAK_STATUS

void ftFoxSpecialLwStartSetStatus(GObj *fighter_gobj);
void ftFoxSpecialLwLoopSetStatus(GObj *fighter_gobj);
void ftFoxSpecialLwEndSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirLwStartSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirLwLoopSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirLwEndSetStatus(GObj *fighter_gobj);
void ftFox_SpecialAirLwTurn_SetStatus(GObj *fighter_gobj);
sb32 ftFoxSpecialLwTurnCheckInterruptLoop(GObj *fighter_gobj);

uintptr_t llFoxMainMotionLwReflectorFTSpecialColl = 0x19B0u;

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspeciallw.c"
#define ftFoxSpecialLwStartSetStatus battleship_ftFoxSpecialLwStartSetStatus
#define ftFoxSpecialAirLwStartSetStatus battleship_ftFoxSpecialAirLwStartSetStatus
#define ftFoxSpecialLwHitSetStatus battleship_ftFoxSpecialLwHitSetStatus
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspeciallw.c"
#undef ftFoxSpecialLwHitSetStatus
#undef ftFoxSpecialAirLwStartSetStatus
#undef ftFoxSpecialLwStartSetStatus

static void ndsFoxReflectorPatchSpecialColl(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->special_coll = lbRelocGetFileData(
            FTSpecialColl *, gFTDataFoxMainMotion,
            &llFoxMainMotionLwReflectorFTSpecialColl);
    }
}

void ftFoxSpecialLwStartSetStatus(GObj *fighter_gobj)
{
    battleship_ftFoxSpecialLwStartSetStatus(fighter_gobj);
    ndsFoxReflectorPatchSpecialColl(fighter_gobj);
}

void ftFoxSpecialAirLwStartSetStatus(GObj *fighter_gobj)
{
    battleship_ftFoxSpecialAirLwStartSetStatus(fighter_gobj);
    ndsFoxReflectorPatchSpecialColl(fighter_gobj);
}

void ftFoxSpecialLwHitSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    gNdsFighterReflectorProofHitSetCallCount++;
    if (fp != NULL)
    {
        gNdsFighterReflectorProofReflectLRBeforeHit = fp->reflect_lr;
    }
    battleship_ftFoxSpecialLwHitSetStatus(fighter_gobj);
}

#endif
