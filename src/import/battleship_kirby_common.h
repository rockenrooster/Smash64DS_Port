/* P2-3 Kirby: the constants and sibling declarations his four TUs share.
 *
 * Constants are BattleShip ft/ftchar/ftkirby/ftkirby.h (REGION_US arm) and
 * wp/wpvars.h; the declarations are the source's own ftkirbyfunctions.h,
 * which only names ftKirby* symbols over the port's mirrored fighter ABI, so
 * it is included directly with the decomp's broad ftdef.h graph masked off. */
#ifndef BATTLESHIP_KIRBY_COMMON_H
#define BATTLESHIP_KIRBY_COMMON_H

#define FTKIRBY_JUMPAERIAL_VEL_MUL 0.8F
#define FTKIRBY_COPY_MODELPARTS_JOINT 6
#define FTKIRBY_COPYDAMAGE_LOSECOPY_RANDOM (1.0F / 12.0F)
#define FTKIRBY_CHARGE_EFFECT_JOINT 16
#define FTKIRBY_VACUUM_RELEASE_LAG 40
#define FTKIRBY_VACUUM_COPY_STICK_RANGE_MIN -40
#define FTKIRBY_VACUUM_TURN_STICK_RANGE_MIN 28
#define FTKIRBY_VACUUM_THROW_DAMAGE 10
#define FTKIRBY_VACUUM_COPY_DAMAGE 6
#define FTKIRBY_VACUUM_COPY_ANGLE F_CLC_DTOR32(75.0F)
#define FTKIRBY_VACUUM_COPY_VEL_BASE 100.0F
#define FTKIRBY_VACUUM_THROW_VEL_BASE 120.0F
#define FTKIRBY_VACUUM_STOPGFX_DIST_MIN 9216.0F
#define FTKIRBY_VACUUM_SPECIALNWAIT_DIST_MIN 1024.0F
#define FTKIRBY_VACUUM_GRAVITY_MUL 2.0F
#define FTKIRBY_VACUUM_FALL_MAX_MUL 2.0F
#define FTKIRBY_FINALCUTTER_BEAM_SPAWN_JOINT 0
#define FTKIRBY_FINALCUTTER_OFF_X 200.0F
#define FTKIRBY_FINALCUTTER_AIR_ACCEL_MUL 0.5F
#define FTKIRBY_STONE_DURATION_MAX 160
#define FTKIRBY_STONE_DURATION_MIN 18
#define FTKIRBY_STONE_FALL_VEL -140.0F
#define FTKIRBY_STONE_SLIDE_ANGLE F_CLC_DTOR32(25.0F)
#define FTKIRBY_STONE_SLIDE_TRACTION_MUL 1.15F
#define FTKIRBY_STONE_SLIDE_VEL_MUL 36.0F
#define FTKIRBY_STONE_SLIDE_CLAMP_VEL_X 30.0F
#define FTKIRBY_STONE_HEALTH_MAX 38
#define FTKIRBY_STONE_HEALTH_MID 22
#define FTKIRBY_STONE_HEALTH_LOW 10
#define FTKIRBY_STONE_COLANIM_ID_HIGH 0x33
#define FTKIRBY_STONE_COLANIM_LENGTH_HIGH 0
#define FTKIRBY_STONE_COLANIM_ID_MID 0x34
#define FTKIRBY_STONE_COLANIM_LENGTH_MID 0
#define FTKIRBY_STONE_COLANIM_ID_LOW 0x35
#define FTKIRBY_STONE_COLANIM_LENGTH_LOW 0
#define FTKIRBY_COPYMARIO_FIREBALL_SPAWN_JOINT 17
#define FTKIRBY_COPYFOX_BLASTER_SPAWN_JOINT 17
#define FTKIRBY_COPYFOX_BLASTER_SPAWN_OFF_X 70.0F
#define FTKIRBY_COPYDONKEY_GIANTPUNCH_CHARGE_MAX 10
#define FTKIRBY_COPYDONKEY_GIANTPUNCH_CHARGE_DAMAGE_MUL 2
#define FTKIRBY_COPYDONKEY_GIANTPUNCH_CHARGE_COLANIM_ID 6
#define FTKIRBY_COPYDONKEY_GIANTPUNCH_CHARGE_COLANIM_LENGTH 0
#define FTKIRBY_COPYDONKEY_GIANTPUNCH_CHRAGE_ANIM_SPEED 2.0F
#define FTKIRBY_COPYDONKEY_GIANTPUNCH_VEL_MUL 8.0F
#define FTKIRBY_COPYSAMUS_CHARGE_JOINT 0
#define FTKIRBY_COPYSAMUS_CHARGE_OFF_Y 200.0F
#define FTKIRBY_COPYSAMUS_CHARGE_OFF_Z 210.0F
#define FTKIRBY_COPYSAMUS_CHARGE_MAX 7
#define FTKIRBY_COPYSAMUS_CHARGE_INT 20
#define FTKIRBY_COPYSAMUS_CHARGE_COLANIM_ID 6
#define FTKIRBY_COPYSAMUS_CHARGE_COLANIM_LENGTH 0
#define FTKIRBY_COPYSAMUS_CHARGE_RECOIL_BASE 10.0F
#define FTKIRBY_COPYSAMUS_CHARGE_RECOIL_MUL 2.0F
#define FTKIRBY_COPYSAMUS_CHARGE_RECOIL_ADD 20.0F
#define FTKIRBY_COPYLINK_BOOMERANG_SPAWN_JOINT 0
#define FTKIRBY_COPYLINK_BOOMERANG_SMASH_STICK_MIN 56
#define FTKIRBY_COPYLINK_BOOMERANG_SMASH_BUFFER 8
#define FTKIRBY_COPYCAPTAIN_FALCONPUNCH_VEL_BASE 65.0F
#define FTKIRBY_COPYCAPTAIN_FALCONPUNCH_VEL_MUL 0.92F
#define FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_JOINT 0
#define FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_ANGLE F_CLC_DTOR32(-45.0F)
#define FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_OFF_X 200.0F
#define FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_OFF_Y 200.0F
#define FTKIRBY_COPYPIKACHU_THUNDERJOLTVEL 40.0F
#define FTKIRBY_COPYPIKACHU_THUNDERJOLT_COLANIM_ID 0x3B
#define FTKIRBY_COPYPIKACHU_THUNDERJOLT_COLANIM_LENGTH 0
#define FTKIRBY_COPYPURIN_POUND_VEL_BASE 65.0F
#define FTKIRBY_COPYPURIN_POUND_VEL_MUL 0.92F
#define FTKIRBY_COPYNESS_PKFIRE_SPAWN_JOINT 0
#define FTKIRBY_COPYNESS_PKFIRE_SPAWN_OFF_X 240.0F
#define FTKIRBY_COPYNESS_PKFIRE_SPAWN_OFF_Y 190.0F
#define FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_AIR F_CLC_DTOR32(-38.0F)
#define FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_GROUND F_CLC_DTOR32(-3.6F)
#define FTKIRBY_COPYNESS_PKFIRE_SPARK_VEL_AIR 95.0F
#define FTKIRBY_COPYNESS_PKFIRE_SPARK_VEL_GROUND 73.0F
#define WPFINALCUTTER_LIFETIME 20
#define WPFINALCUTTER_VEL 100.0F

/* The source's own ftKirby* declaration set over the port ABI. Its ftdef.h
 * include is the decomp's broad fighter header, which the port's fighter.h
 * mirrors, so that guard is pre-defined to keep the two apart. */
#ifndef _FTDEF_H_
#define _FTDEF_H_
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyfunctions.h"
#undef _FTDEF_H_
#else
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyfunctions.h"
#endif

/* Shared owners and other fighters' articles Kirby's TUs reach. */
GObj *wpKirbyCutterMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
GObj *wpMarioFireballMakeWeapon(GObj *fighter_gobj, Vec3f *pos, s32 index);
GObj *wpFoxBlasterMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
GObj *wpSamusChargeShotMakeWeapon(GObj *fighter_gobj, Vec3f *pos, s32 charge_level, sb32 is_release);
GObj *wpLinkBoomerangMakeWeapon(GObj *fighter_gobj, Vec3f *pos);
GObj *wpPikachuThunderJoltAirMakeWeapon(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel);
GObj *wpNessPKFireMakeWeapon(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel, f32 angle);
GObj *efManagerCaptainFalconPunchMakeEffect(GObj *fighter_gobj);
GObj *efManagerKirbyVulcanJabMakeEffect(Vec3f *pos, s32 lr, f32 rotate, f32 vel, f32 add);
GObj *efManagerKirbyCutterUpMakeEffect(GObj *fighter_gobj);
GObj *efManagerKirbyCutterDownMakeEffect(GObj *fighter_gobj);
GObj *efManagerKirbyCutterDrawMakeEffect(GObj *fighter_gobj);
GObj *efManagerKirbyCutterTrailMakeEffect(GObj *fighter_gobj);
LBParticle *efManagerKirbyInhaleWindMakeEffect(GObj *fighter_gobj);
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);
GObj *efManagerQuakeMakeEffect(s32 magnitude);
/* efmanager.c:5879/5957 build the spat-out and lost-copy stars from
 * ITCommonData (llITCommonDataFileID 0xfb), a file the port does not pack or
 * load yet -- the same residency the Pikachu/Purin Master Ball article and
 * P2-5's items need. Until row P2-3f48 lands it, both calls resolve to NULL,
 * which ftcommoncapturekirby.c:270 and ftkirbyspecialn.c:949 tolerate.
 * ACCEPTED DELTA (visual, temporary): no star sprite on spit / lose-copy. */
#define efManagerCaptureKirbyStarMakeEffect(fighter_gobj) ((GObj *)NULL)
#define efManagerLoseKirbyStarMakeEffect(fighter_gobj) ((void)0)
void ftParamProcPauseEffect(GObj *effect_gobj);
void ftParamProcResumeEffect(GObj *fighter_gobj);
void scManagerRunPrintGObjStatus(void);
void ftCommonCaptureKirbyProcCapture(GObj *fighter_gobj, GObj *capture_gobj);
f32 syUtilsArcTan2(f32 y, f32 x);
void ftCommonThrownKirbyStarMakeEffect(GObj *fighter_gobj, f32 arg1, f32 arg2);
void ftCommonThrownKirbyStarInitStatusVars(GObj *fighter_gobj);
void ftCommonThrownKirbyStarSetStatus(GObj *fighter_gobj);
void ftCommonThrownCopyStarSetStatus(GObj *fighter_gobj);
void ftCommonThrownKirbyEscape(GObj *fighter_gobj);
void ftCommonThrownCommonStarUpdatePhysics(GObj *fighter_gobj, f32 decelerate);
void ftCommonCaptureKirbyUpdatePositionsMag(GObj *fighter_gobj, Vec3f *dist);
void ftCommonCaptureKirbyUpdatePositionsAll(GObj *fighter_gobj);
void ftCommonCaptureWaitKirbySetStatus(GObj *fighter_gobj);
void ftCommonCaptureWaitKirbyUpdateBreakoutVars(FTStruct *this_fp,
                                                FTStruct *capture_fp);

/* BattleShip ftcommon.h:292. */
#define FTCOMMON_THROWNCOPYSTAR_DECELERATE 5.2F

/* Copy specials reach into their owners' seams: the Samus/DK escape helpers
 * the port carries under base names (battleship_samus.c takes the same
 * macros), Yoshi's capture callback, the shared 2D reflect and figatree
 * helpers, and the Final Cutter weapon's own prototypes (wpkirbycutter.h). */
s32 ndsBaseFTCommonEscapeGetStatus(FTStruct *fp);
void ndsBaseFTCommonEscapeSetStatus(GObj *fighter_gobj, s32 status_id,
                                    s32 itemthrow_buffer_tics);
#define ftCommonEscapeGetStatus ndsBaseFTCommonEscapeGetStatus
#define ftCommonEscapeSetStatus(fighter_gobj, status_id) \
    ndsBaseFTCommonEscapeSetStatus((fighter_gobj), (status_id), 0)
sb32 ftCommonEscapeCheckInterruptSpecialNDonkey(GObj *fighter_gobj);
void ftCommonCaptureYoshiProcCapture(GObj *fighter_gobj, GObj *capture_gobj);
Vec3f *lbCommonReflect2D(Vec3f *a, Vec3f *b);
void lbCommonAddFighterPartsFigatree(DObj *root_dobj, void *figatree,
                                     f32 anim_frame);
sb32 wpKirbyCutterProcUpdate(GObj *weapon_gobj);
sb32 wpKirbyCutterProcMap(GObj *weapon_gobj);
sb32 wpKirbyCutterProcHit(GObj *weapon_gobj);
sb32 wpKirbyCutterProcShield(GObj *weapon_gobj);
sb32 wpKirbyCutterProcSetOff(GObj *weapon_gobj);
sb32 wpKirbyCutterProcReflector(GObj *weapon_gobj);

#endif /* BATTLESHIP_KIRBY_COMMON_H */
