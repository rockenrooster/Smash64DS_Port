/*
 * Fenced BattleShip normal-moveset imports for the Mario/Fox tilt, smash,
 * and aerial-interrupt slice. Public symbols are remapped so existing port
 * seams can route to the original exactly once while the fence is off by
 * default.
 */
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <ft/ftdata_file_slots.h>
#include <it/item.h>
#include <reloc_data.h>
#include <sys/obj.h>

sb32 itMainCheckShootNoAmmo(GObj *item_gobj);
void func_ovl2_800EE018(DObj *main_dobj, Vec3f *vec);
void ndsBaseFTCommonSquatWaitSetStatus(GObj *fighter_gobj);

__attribute__((weak)) uintptr_t llNessMainMotionAttackS4ReflectorFTSpecialColl;

__attribute__((weak)) sb32 itMainCheckShootNoAmmo(GObj *item_gobj)
{
    (void)item_gobj;
    return FALSE;
}

__attribute__((weak)) GObj *
efManagerPikachuThunderShockMakeEffect(GObj *fighter_gobj, Vec3f *pos,
                                       s32 frame)
{
    (void)fighter_gobj;
    (void)pos;
    (void)frame;
    return NULL;
}

__attribute__((weak)) void ftParamProcPauseEffect(GObj *effect_gobj)
{
    (void)effect_gobj;
}

__attribute__((weak)) void ftParamProcResumeEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

#define ftCommonAttackS3SetStatus ndsBaseFTCommonAttackS3SetStatus
#define ftCommonAttackS3CheckInterruptCommon \
    ndsBaseFTCommonAttackS3CheckInterruptCommon

void ndsBaseFTCommonAttackS3SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS3CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacks3.c"

#undef ftCommonAttackS3SetStatus
#undef ftCommonAttackS3CheckInterruptCommon

#define ftCommonAttackHi3SetStatus ndsBaseFTCommonAttackHi3SetStatus
#define ftCommonAttackHi3CheckInterruptCommon \
    ndsBaseFTCommonAttackHi3CheckInterruptCommon

void ndsBaseFTCommonAttackHi3SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi3CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattackhi3.c"

#undef ftCommonAttackHi3SetStatus
#undef ftCommonAttackHi3CheckInterruptCommon

#define ftCommonAttackLw3ProcUpdate ndsBaseFTCommonAttackLw3ProcUpdate
#define ftCommonAttackLw3ProcInterrupt \
    ndsBaseFTCommonAttackLw3ProcInterrupt
#define ftCommonAttackLw3CheckInterruptSelf \
    ndsBaseFTCommonAttackLw3CheckInterruptSelf
#define ftCommonAttackLw3InitStatusVars \
    ndsBaseFTCommonAttackLw3InitStatusVars
#define ftCommonAttackLw3SetStatus ndsBaseFTCommonAttackLw3SetStatus
#define ftCommonAttackLw3CheckInterruptCommon \
    ndsBaseFTCommonAttackLw3CheckInterruptCommon
#define ftCommonSquatWaitSetStatus ndsBaseFTCommonSquatWaitSetStatus

void ndsBaseFTCommonAttackLw3ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttackLw3ProcInterrupt(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw3CheckInterruptSelf(GObj *fighter_gobj);
void ndsBaseFTCommonAttackLw3InitStatusVars(GObj *fighter_gobj);
void ndsBaseFTCommonAttackLw3SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw3CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacklw3.c"

#undef ftCommonAttackLw3ProcUpdate
#undef ftCommonAttackLw3ProcInterrupt
#undef ftCommonAttackLw3CheckInterruptSelf
#undef ftCommonAttackLw3InitStatusVars
#undef ftCommonAttackLw3SetStatus
#undef ftCommonAttackLw3CheckInterruptCommon
#undef ftCommonSquatWaitSetStatus

#define ftCommonAttackS4ProcUpdate ndsBaseFTCommonAttackS4ProcUpdate
#define ftCommonAttackS4SetStatus ndsBaseFTCommonAttackS4SetStatus
#define ftCommonAttackS4CheckInterruptDash \
    ndsBaseFTCommonAttackS4CheckInterruptDash
#define ftCommonAttackS4CheckInterruptTurn \
    ndsBaseFTCommonAttackS4CheckInterruptTurn
#define ftCommonAttackS4CheckInterruptCommon \
    ndsBaseFTCommonAttackS4CheckInterruptCommon

void ndsBaseFTCommonAttackS4ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttackS4SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptDash(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptTurn(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacks4.c"

#undef ftCommonAttackS4ProcUpdate
#undef ftCommonAttackS4SetStatus
#undef ftCommonAttackS4CheckInterruptDash
#undef ftCommonAttackS4CheckInterruptTurn
#undef ftCommonAttackS4CheckInterruptCommon

#define ftCommonAttackHi4SetStatus ndsBaseFTCommonAttackHi4SetStatus
#define ftCommonAttackHi4CheckInputSuccess \
    ndsBaseFTCommonAttackHi4CheckInputSuccess
#define ftCommonAttackHi4CheckInterruptMain \
    ndsBaseFTCommonAttackHi4CheckInterruptMain
#define ftCommonAttackHi4CheckInterruptKneeBend \
    ndsBaseFTCommonAttackHi4CheckInterruptKneeBend
#define ftCommonAttackHi4CheckInterruptCommon \
    ndsBaseFTCommonAttackHi4CheckInterruptCommon

void ndsBaseFTCommonAttackHi4SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi4CheckInputSuccess(FTStruct *fp);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptMain(FTStruct *fp);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptKneeBend(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattackhi4.c"

#undef ftCommonAttackHi4SetStatus
#undef ftCommonAttackHi4CheckInputSuccess
#undef ftCommonAttackHi4CheckInterruptMain
#undef ftCommonAttackHi4CheckInterruptKneeBend
#undef ftCommonAttackHi4CheckInterruptCommon

#define ftCommonAttackLw4SetStatus ndsBaseFTCommonAttackLw4SetStatus
#define ftCommonAttackLw4CheckInputSuccess \
    ndsBaseFTCommonAttackLw4CheckInputSuccess
#define ftCommonAttackLw4CheckInterruptMain \
    ndsBaseFTCommonAttackLw4CheckInterruptMain
#define ftCommonAttackLw4CheckInterruptSquat \
    ndsBaseFTCommonAttackLw4CheckInterruptSquat
#define ftCommonAttackLw4CheckInterruptCommon \
    ndsBaseFTCommonAttackLw4CheckInterruptCommon

void ndsBaseFTCommonAttackLw4SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw4CheckInputSuccess(FTStruct *fp);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptMain(FTStruct *fp);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptSquat(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattacklw4.c"

#undef ftCommonAttackLw4SetStatus
#undef ftCommonAttackLw4CheckInputSuccess
#undef ftCommonAttackLw4CheckInterruptMain
#undef ftCommonAttackLw4CheckInterruptSquat
#undef ftCommonAttackLw4CheckInterruptCommon

#define ftCommonJumpAerialUpdateModelYaw \
    ndsBaseFTCommonJumpAerialUpdateModelYaw
#define ftCommonJumpAerialProcUpdate ndsBaseFTCommonJumpAerialProcUpdate
#define ftCommonJumpAerialProcInterrupt \
    ndsBaseFTCommonJumpAerialProcInterrupt
#define ftYoshiJumpAerialProcPhysics ndsBaseFTYoshiJumpAerialProcPhysics
#define ftNessJumpAerialProcPhysics ndsBaseFTNessJumpAerialProcPhysics
#define ftCommonJumpAerialProcPhysics \
    ndsBaseFTCommonJumpAerialProcPhysics
#define ftCommonJumpAerialSetStatus ndsBaseFTCommonJumpAerialSetStatus
#define ftCommonJumpAerialMultiSetStatus \
    ndsBaseFTCommonJumpAerialMultiSetStatus
#define ftCommonJumpAerialMultiCheckJumpButtonHold \
    ndsBaseFTCommonJumpAerialMultiCheckJumpButtonHold
#define ftCommonJumpAerialMultiGetJumpInputType \
    ndsBaseFTCommonJumpAerialMultiGetJumpInputType
#define ftCommonJumpAerialCheckInterruptCommon \
    ndsBaseFTCommonJumpAerialCheckInterruptCommon

void ndsBaseFTCommonJumpAerialUpdateModelYaw(FTStruct *fp);
void ndsBaseFTCommonJumpAerialProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonJumpAerialProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTYoshiJumpAerialProcPhysics(GObj *fighter_gobj);
void ndsBaseFTNessJumpAerialProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonJumpAerialProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonJumpAerialSetStatus(GObj *fighter_gobj,
                                        s32 input_source);
void ndsBaseFTCommonJumpAerialMultiSetStatus(GObj *fighter_gobj,
                                             s32 input_source);
sb32 ndsBaseFTCommonJumpAerialMultiCheckJumpButtonHold(FTStruct *fp);
s32 ndsBaseFTCommonJumpAerialMultiGetJumpInputType(FTStruct *fp);
sb32 ndsBaseFTCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonjumpaerial.c"

#undef ftCommonJumpAerialUpdateModelYaw
#undef ftCommonJumpAerialProcUpdate
#undef ftCommonJumpAerialProcInterrupt
#undef ftYoshiJumpAerialProcPhysics
#undef ftNessJumpAerialProcPhysics
#undef ftCommonJumpAerialProcPhysics
#undef ftCommonJumpAerialSetStatus
#undef ftCommonJumpAerialMultiSetStatus
#undef ftCommonJumpAerialMultiCheckJumpButtonHold
#undef ftCommonJumpAerialMultiGetJumpInputType
#undef ftCommonJumpAerialCheckInterruptCommon
