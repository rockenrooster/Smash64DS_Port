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
void wpDisplayMain(GObj *weapon_gobj, void (*proc_display)(GObj *));
void gcDrawDObjTreeForGObj(GObj *gobj);

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
#if NDS_P2_LINK_SPECIAL_TOUR
volatile u32 gNdsLinkBoomerangProjectCount;
volatile f32 gNdsLinkBoomerangProjectWorldX;
volatile f32 gNdsLinkBoomerangProjectWorldY;
volatile f32 gNdsLinkBoomerangProjectScreenX;
volatile f32 gNdsLinkBoomerangProjectScreenY;
volatile u32 gNdsLinkBoomerangDisplayCallCount;
volatile u32 gNdsLinkBoomerangDisplayTreeCount;
volatile u32 gNdsLinkBoomerangDisplayMode;
volatile u32 gNdsLinkBoomerangDisplayAttackState;

static void ndsLinkBoomerangProject(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                                    f32 *screen_x, f32 *screen_y)
{
    func_ovl2_800EB924(cobj, matrix, pos, screen_x, screen_y);
    gNdsLinkBoomerangProjectCount++;
    gNdsLinkBoomerangProjectWorldX = pos->x;
    gNdsLinkBoomerangProjectWorldY = pos->y;
    gNdsLinkBoomerangProjectScreenX = *screen_x;
    gNdsLinkBoomerangProjectScreenY = *screen_y;
}
#define func_ovl2_800EB924 ndsLinkBoomerangProject
#endif
#define wpLinkBoomerangMakeWeapon battleship_wpLinkBoomerangMakeWeapon
#include "../../decomp/BattleShip-main/decomp/src/wp/wplink/wplinkboomerang.c"
#undef wpLinkBoomerangMakeWeapon
#if NDS_P2_LINK_SPECIAL_TOUR
#undef func_ovl2_800EB924
#endif

static void ndsLinkBoomerangDrawTree(GObj *weapon_gobj)
{
#if NDS_P2_LINK_SPECIAL_TOUR
    gNdsLinkBoomerangDisplayTreeCount++;
#endif
    gcDrawDObjTreeForGObj(weapon_gobj);
}

/* BattleShip wpmanager.c selects func_ovl3_80167618 for Boomerang's 0x01
 * descriptor.  That source callback is wpDisplayMain(...,
 * lbCommonDObjScaleXProcDisplay): it walks the complete DObj tree on DL head 1
 * after resetting gGCScaleX to 1.0.  The shared DS compatibility definition of
 * lbCommonDObjScaleXProcDisplay is intentionally a no-op because effects also
 * use it and some already have specialized DS owners.
 *
 * Own only Boomerang's seam, as the Fox blaster port already does for the same
 * wpmanager situation.  wpDisplayMain preserves BattleShip's weapon visibility
 * and normal/no-Z state; the DS tree submit consumes the same live DObj
 * translation/rotation/scale hierarchy while replacing only the immutable Gfx
 * root.  No gameplay, lifetime, collision, or return/catch callback changes. */
static void ndsLinkBoomerangProcDisplay(GObj *weapon_gobj)
{
#if NDS_P2_LINK_SPECIAL_TOUR
    WPStruct *wp = wpGetStruct(weapon_gobj);

    gNdsLinkBoomerangDisplayCallCount++;
    gNdsLinkBoomerangDisplayMode = (u32)wp->display_mode;
    gNdsLinkBoomerangDisplayAttackState = (u32)wp->attack_coll.attack_state;
#endif
    wpDisplayMain(weapon_gobj, ndsLinkBoomerangDrawTree);
}

GObj *wpLinkBoomerangMakeWeapon(GObj *fighter_gobj, Vec3f *pos)
{
    GObj *weapon_gobj = battleship_wpLinkBoomerangMakeWeapon(fighter_gobj, pos);

    if (weapon_gobj != NULL)
    {
        weapon_gobj->proc_display = ndsLinkBoomerangProcDisplay;
    }
    return weapon_gobj;
}
