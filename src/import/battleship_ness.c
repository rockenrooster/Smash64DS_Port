/* P2-3 Ness runtime state machine: BattleShip specials verbatim.
 *
 * PK Fire (a spark weapon that blooms into the PK Fire pillar ITEM on
 * contact), PK Thunder (the player-steered bolt, its trail, and the PKT2
 * self-launch "Jibaku" with its wall bounce) and PSI Magnet (the absorb
 * status) keep their source status bodies as the behavioral authority. The
 * port adapts only the surrounding ABI: articles live in
 * battleship_ness_weapons.c / battleship_ness_items.c. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <wp/weapon.h>
#include <ft/ftstatus_callbacks.h>
#include "battleship_ness_common.h"

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Exact US constants from BattleShip ft/ftchar/ftness/ftness.h. */
#define FTNESS_PKFIRE_SPAWN_JOINT nFTPartsJointTopN
#define FTNESS_PKFIRE_SPAWN_OFF_X 100.0F
#define FTNESS_PKFIRE_SPAWN_OFF_Y 180.0F
#define FTNESS_PKFIRE_SPARK_ANGLE_AIR F_CLC_DTOR32(-38.0F)
#define FTNESS_PKFIRE_SPARK_ANGLE_GROUND F_CLC_DTOR32(-3.6F)
#define FTNESS_PKFIRE_SPARK_VEL_AIR 95.0F
#define FTNESS_PKFIRE_SPARK_VEL_GROUND 73.0F
#define WPPKTHUNDER_PARTS_COUNT 5
#define FTNESS_PKTHUNDER_TRAIL_POS_COUNT ((WPPKTHUNDER_PARTS_COUNT * 2) + ((WPPKTHUNDER_PARTS_COUNT * 2) / WPPKTHUNDER_PARTS_COUNT))
#define FTNESS_PKTHUNDER_SPAWN_JOINT 12
#define FTNESS_PKTHUNDER_SPAWN_VEL_Y 60.0F
#define FTNESS_PKTHUNDER_COLLIDE_X 250.0F
#define FTNESS_PKTHUNDER_COLLIDE_Y 370.0F
#define FTNESS_PKTHUNDER_END_DELAY 30
#define FTNESS_PKTHUNDER_LANDING_LAG F_PCT_TO_DEC(17.0F)
#define FTNESS_PKTHUNDER_GRAVITY_DELAY 25
#define FTNESS_PKTHUNDER_FALLSPECIAL_DRIFT 0.6F
#define FTNESS_PKJIBAKU_DELAY 30
#define FTNESS_PKJIBAKU_ANIM_LENGTH 28
#define FTNESS_PKJIBAKU_HALT_ANGLE F_CLC_DTOR32(155.0F)
#define FTNESS_PKJIBAKU_VEL 200.0F
#define FTNESS_PKJIBAKU_DECELERATE ((float)(43.0F / 7.0F))
#define FTNESS_PKJIBAKU_PASS_FRAME_END ((float)(FTNESS_PKJIBAKU_ANIM_LENGTH - 3))
#define FTNESS_PKJIBAKU_REBOUND_VEL_MAG 0.5F
#define FTNESS_PSYCHICMAGNET_RELEASE_LAG 30
#define FTNESS_PSYCHICMAGNET_GRAVITY_DELAY 4
#define FTNESS_PSYCHICMAGNET_COLANIM_ID 0x3F
#define FTNESS_PSYCHICMAGNET_COLANIM_LENGTH 0

/* Source sibling declarations (BattleShip ftnessfunctions.h). The port keeps
 * one compatible fighter ABI view, so the three included bodies see one
 * another here; the status-table callbacks are in ftstatus_callbacks.h. */
void ftNessAppearWaitSetStatus(GObj *fighter_gobj);
void ftNessAppearEndSetStatus(GObj *fighter_gobj);
void ftNessSpecialNProcAccessory(GObj *fighter_gobj);
void ftNessSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialNSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialNInitStatusVars(GObj *fighter_gobj);
void ftNessSpecialNSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirNSetStatus(GObj *fighter_gobj);
void ftNessSpecialHiDecThunderTimers(FTStruct *fp);
void ftNessSpecialHiMakePKThunder(GObj *fighter_gobj);
sb32 ftNessSpecialHiCheckCollidePKThunder(GObj *fighter_gobj);
void ftNessSpecialAirHiStartSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialHiStartSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialHiInitStatusVars(GObj *fighter_gobj);
void ftNessSpecialHiStartSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirHiStartSetStatus(GObj *fighter_gobj);
void ftNessSpecialHiUpdatePKThunder(GObj *fighter_gobj);
void ftNessSpecialHiSetPKThunderDestroy(GObj *fighter_gobj);
void ftNessSpecialAirHiHoldSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialHiHoldSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialHiHoldInitStatusVars(GObj *fighter_gobj);
void ftNessSpecialHiHoldSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirHiHoldSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirHiEndSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialHiEndSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialHiClearProcDamage(GObj *fighter_gobj);
void ftNessSpecialHiEndSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirHiEndSetStatus(GObj *fighter_gobj);
void ftNessSpecialHiCollideWallPhysics(GObj *fighter_gobj, MPCollData *coll_data);
void ftNessSpecialHiUpdateModelPitch(GObj *fighter_gobj);
sb32 ftNessSpecialHiProcPass(GObj *fighter_gobj);
void ftNessSpecialAirHiJibakuSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialHiJibakuSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialHiJibakuInitStatusVars(GObj *fighter_gobj);
void ftNessSpecialHiJibakuSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirHiJibakuSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirHiJibakuBoundSetStatus(GObj *fighter_gobj, Vec3f *angle, Vec3f *pos);
void ftNessSpecialLwCheckRelease(FTStruct *fp);
void ftNessSpecialLwDecReleaseLag(FTStruct *fp);
void ftNessSpecialLwProcAbsorb(GObj *fighter_gobj);
void ftNessSpecialAirLwStartSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialLwStartSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialLwInitStatusVars(GObj *fighter_gobj);
void ftNessSpecialLwStartSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirLwStartSetStatus(GObj *fighter_gobj);
void ftNessSpecialLwUpdateReleaseLag(GObj *fighter_gobj);
void ftNessSpecialAirLwHoldSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialLwHoldSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialLwHoldSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirLwHoldSetStatus(GObj *fighter_gobj);
void ftNessSpecialLwHitSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirLwHitSetStatus(GObj *fighter_gobj);
void ftNessSpecialLwEndSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirLwEndSetStatus(GObj *fighter_gobj);
void ftNessSpecialAirLwHitSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialLwHitSwitchStatusAir(GObj *fighter_gobj);
void ftNessSpecialAirLwEndSwitchStatusGround(GObj *fighter_gobj);
void ftNessSpecialLwEndSwitchStatusAir(GObj *fighter_gobj);

/* Article and effect declarations normally supplied by BattleShip's broad
 * include graph; the implementations live in the shared owners and the
 * companion Ness TUs. */
GObj *efManagerNessPsychicMagnetMakeEffect(GObj *fighter_gobj);
GObj *efManagerNessPKThunderWaveMakeEffect(GObj *fighter_gobj);
GObj *efManagerImpactWaveMakeEffect(Vec3f *pos, s32 index, f32 rotate);
GObj *efManagerQuakeMakeEffect(s32 magnitude);
f32 lbCommonMag2D(Vec3f *vec);
Vec3f *lbCommonScale2D(Vec3f *vec, f32 factor);
Vec3f *lbCommonReflect2D(Vec3f *a, Vec3f *b);
f32 syUtilsArcTan2(f32 y, f32 x);

/* NessMainMotion special-collision offsets (reloc_data.us.h:3717-3718). The
 * AttackS4 reflector symbol is weak-zero in the shared moveset TU until his
 * TU supplies the real value; the PSI Magnet absorb box is his alone. */
uintptr_t llNessMainMotionAttackS4ReflectorFTSpecialColl = 0x1114u;
uintptr_t llNessMainMotionLwAbsorbFTSpecialColl = 0x16D4u;
/* ftcommonentry.c:91 and :158, verbatim with the one fkind test resolved:
 * only Pikachu/Purin spawn Master-Ball rays for flag1, so Ness's arm only
 * consumes the flag. The port's entry TU keeps its own static copies. */
static void ftCommonAppearUpdateEffects(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        fp->motion_vars.flags.flag1 = 0;
    }
    if (fp->motion_vars.flags.flag2 != 0)
    {
        fp->motion_vars.flags.flag2 = 0;

        fp->is_shadow_hide = FALSE;
    }
}

static void ftCommonAppearInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_ghost = TRUE;

    fp->camera_mode = nFTCameraModeEntry;

    fp->is_shadow_hide = TRUE;
    fp->is_playertag_hide = TRUE;
}

/* BattleShip ftcommonentry.c:285-320 (0x8013DE60..0x8013DF14), verbatim.
 * The source keeps Ness's three-status entry ladder beside the generic entry
 * seam; the port's entry TU carries only the per-kind dispatch, so the four
 * ladder bodies ride with the rest of his runtime here. */
void ftNessAppearStartProcUpdate(GObj *fighter_gobj)
{
    ftCommonAppearUpdateEffects(fighter_gobj);
    ftAnimEndCheckSetStatus(fighter_gobj, ftNessAppearWaitSetStatus);
}

void ftNessAppearWaitProcUpdate(GObj *fighter_gobj)
{
    ftCommonAppearUpdateEffects(fighter_gobj);
    ftAnimEndCheckSetStatus(fighter_gobj, ftNessAppearEndSetStatus);
}

void ftNessAppearWaitSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTNessStatusAppearWait, 0.0F, 1.0F,
                    (FTSTATUS_PRESERVE_MODELPART | FTSTATUS_PRESERVE_COLANIM));
    ftCommonAppearInitStatusVars(fighter_gobj);

    fp->is_shadow_hide = FALSE;
}

void ftNessAppearEndSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj,
                    ((fp->status_vars.common.entry.lr == +1) ?
                         nFTNessStatusAppearREnd : nFTNessStatusAppearLEnd),
                    0.0F, 1.0F,
                    (FTSTATUS_PRESERVE_MODELPART | FTSTATUS_PRESERVE_COLANIM));
    ftCommonAppearInitStatusVars(fighter_gobj);

    fp->is_shadow_hide = FALSE;
}

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftness/ftnessspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftness/ftnessspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftness/ftnessspeciallw.c"
