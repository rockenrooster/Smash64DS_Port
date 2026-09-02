/* P2-3 Ness: declarations his gameplay and weapon TUs share. The enum is
 * BattleShip wp/wpvars.h:137 (wpNessPKThunderCollide); the prototypes are the
 * source's own wpnesspkfire.h / wpnesspkthunder.h over the port's mirrored
 * weapon ABI, transcribed because those headers pull the decomp's broad
 * sys/wp include graph. wpDisplayPKThunderProcDisplay is wpdisplay.c's,
 * compiled into battleship_wpmanager_core.c. */
#ifndef BATTLESHIP_NESS_COMMON_H
#define BATTLESHIP_NESS_COMMON_H

typedef enum wpNessPKThunderCollide {
    nWPNessPKThunderStatusActive,
    nWPNessPKThunderStatusDestroy,
    nWPNessPKThunderStatusCollide
} wpNessPKThunderCollide;

sb32 wpNessPKFireProcUpdate(GObj *weapon_gobj);
sb32 wpNessPKFireProcMap(GObj *weapon_gobj);
sb32 wpNessPKFireProcHit(GObj *weapon_gobj);
sb32 wpNessPKFireProcHop(GObj *weapon_gobj);
sb32 wpNessPKFireProcReflector(GObj *weapon_gobj);
sb32 wpNessPKFireProcAbsorb(GObj *weapon_gobj);
GObj *wpNessPKFireMakeWeapon(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel, f32 angle);

void wpNessPKThunderHeadSetDestroyTrails(GObj *weapon_gobj, sb32 is_destroy);
void wpNessPKThunderTrailUpdatePositions(GObj *weapon_gobj);
void wpNessPKThunderHeadMakeTrail(GObj *weapon_gobj, s32 trail_id);
sb32 wpNessPKThunderHeadProcUpdate(GObj *weapon_gobj);
sb32 wpNessPKThunderHeadProcMap(GObj *weapon_gobj);
sb32 wpNessPKThunderHeadProcHit(GObj *weapon_gobj);
sb32 wpNessPKThunderHeadProcReflector(GObj *weapon_gobj);
sb32 wpNessPKThunderHeadProcDead(GObj *weapon_gobj);
GObj *wpNessPKThunderHeadMakeWeapon(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel);
sb32 wpNessPKThunderTrailProcUpdate(GObj *weapon_gobj);
sb32 wpNessPKThunderTrailProcHit(GObj *weapon_gobj);
GObj *wpNessPKThunderTrailMakeWeapon(GObj *head_gobj, Vec3f *pos, s32 trail_id);
void wpNessPKReflectHeadMakeTrail(GObj *weapon_gobj, s32 trail_id);
void wpNessPKReflectHeadSetDestroyTrails(GObj *weapon_gobj, s32 unused);
sb32 wpNessPKReflectHeadProcUpdate(GObj *weapon_gobj);
sb32 wpNessPKReflectHeadProcMap(GObj *weapon_gobj);
sb32 wpNessPKReflectHeadProcHit(GObj *weapon_gobj);
sb32 wpNessPKReflectHeadProcReflector(GObj *weapon_gobj);
sb32 wpNessPKReflectHeadProcDead(GObj *weapon_gobj);
GObj *wpNessPKReflectHeadMakeWeapon(GObj *old_gobj, Vec3f *pos, f32 angle);
sb32 wpNessPKReflectTrailProcUpdate(GObj *weapon_gobj);
sb32 wpNessPKReflectTrailProcHit(GObj *weapon_gobj);
GObj *wpNessPKReflectTrailMakeWeapon(GObj *old_gobj, Vec3f *pos, s32 trail_id);
void wpDisplayPKThunderProcDisplay(GObj *weapon_gobj);

#endif
