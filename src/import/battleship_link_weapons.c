/* P2-3 Link weapon runtime: BattleShip Boomerang and Spin Attack verbatim. */
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <reloc_data.h>
#include <string.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif
#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

/* Relocation tokens from BattleShip's US relocation symbol table. */
uintptr_t llLinkMainSpinAttackWeaponAttributes = 0x0cu;
uintptr_t llLinkSpecial1BoomerangWeaponAttributes = 0x00u;

/* Exact source constants from wp/wpvars.h. */
#define WPSPINATTACK_LIFETIME 100
#define WPSPINATTACK_VEL 28.0F
#define WPSPINATTACK_VEL_CLAMP 0.4F
#define WPSPINATTACK_OFF_X 40.0F
#define WPSPINATTACK_OFF_Y 80.0F
#define WPSPINATTACK_ANGLE F_CLC_DTOR32(10.0F)
#define WPBOOMERANG_OFF_X 150.0F
#define WPBOOMERANG_OFF_Y 290.0F
#define WPBOOMERANG_HOMING_ANGLE_MAX F_CST_DTOR32(1.5F)
#define WPBOOMERANG_HOMING_ANGLE_MIN F_CST_DTOR32(0.75F)
#define WPBOOMERANG_VEL_SMASH 114.0F
#define WPBOOMERANG_VEL_TILT 85.0F
#define WPBOOMERANG_RETURN_DAMAGE 8
#define WPBOOMERANG_ANGLE_STICK_THRESHOLD 10
#define WPBOOMERANG_LIFETIME_SMASH 190
#define WPBOOMERANG_LIFETIME_TILT 160
#define WPBOOMERANG_LIFETIME_REFLECT 100

extern f32 syUtilsArcTan2(f32 y, f32 x);
extern Mtx44f gGMCameraMatrix;
void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                        f32 *screen_x, f32 *screen_y);
void wpMainPlayFGM(WPStruct *wp, u16 sfx_id);
f32 lbCommonSim2D(Vec3f *a, Vec3f *b);
Vec3f *lbCommonReflect2D(Vec3f *a, Vec3f *b);
GObj *efManagerDustCollideMakeEffect(Vec3f *pos);
void ftLinkSpecialNGetSetStatus(GObj *fighter_gobj);
void ftKirbyCopyLinkSpecialNGetSetStatus(GObj *fighter_gobj);

sb32 wpLinkSpinAttackProcUpdate(GObj *weapon_gobj);
sb32 wpLinkSpinAttackProcMap(GObj *weapon_gobj);
sb32 wpLinkSpinAttackProcHit(GObj *weapon_gobj);
sb32 wpLinkBoomerangProcUpdate(GObj *weapon_gobj);
sb32 wpLinkBoomerangProcMap(GObj *weapon_gobj);
sb32 wpLinkBoomerangProcHit(GObj *weapon_gobj);
sb32 wpLinkBoomerangProcShield(GObj *weapon_gobj);
sb32 wpLinkBoomerangProcHop(GObj *weapon_gobj);
sb32 wpLinkBoomerangProcReflector(GObj *weapon_gobj);

#include "../../decomp/BattleShip-main/decomp/src/wp/wplink/wplinkspinattack.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wplink/wplinkboomerang.c"
