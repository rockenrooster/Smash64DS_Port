/* P2-3 Samus weapon runtime: BattleShip Charge Shot and Bomb verbatim. */
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <reloc_data.h>
#include <wp/weapon.h>

/* Relocation-symbol tokens follow the same DS adapter contract as Mario's
 * fireball: the descriptor stores the token address and ndsRelocGetFileData
 * resolves it against the staged source file at runtime. */
uintptr_t llSamusMainBombWeaponAttributes = 0x0cu;
uintptr_t llSamusSpecial1ChargeShotWeaponAttributes = 0x00u;

/* Narrow declarations normally supplied by BattleShip's broad wp/lb/ef
 * include graph. Their implementations are already imported by the shared
 * weapon/effect owners; Samus keeps the original calls and ordering. */
extern f32 lbCommonMag2D(Vec3f *vec);
extern Vec3f *lbCommonScale2D(Vec3f *vec, f32 factor);
extern Vec3f *lbCommonReflect2D(Vec3f *a, Vec3f *b);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
extern LBParticle *efManagerImpactShockMakeEffect(Vec3f *pos, s32 size);
extern LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
sb32 wpMapTestAllCheckFloor(GObj *weapon_gobj);
sb32 wpMapTestLRWallCheckFloor(GObj *weapon_gobj);
void wpMapSetGround(WPStruct *wp);
void wpMapSetAir(WPStruct *wp);
void wpMainVelSetLR(GObj *weapon_gobj);
void wpMainVelGroundTransferAir(GObj *weapon_gobj);

#define WPCHARGESHOT_GFX_SIZE_DIV 30.0F
#define WPCHARGESHOT_ROTATE_SPEED F_CLC_DTOR32(18.0F)

#define WPSAMUSBOMB_EXPLODE_LIFETIME 6
#define WPSAMUSBOMB_EXPLODE_SIZE 180.0F
#define WPSAMUSBOMB_WAIT_LIFETIME 100
#define WPSAMUSBOMB_WAIT_VEL_Y 10.0F
#define WPSAMUSBOMB_WAIT_ROTATE_SPEED_AIR F_CLC_DTOR32(20.0F)
#define WPSAMUSBOMB_WAIT_ROTATE_SPEED_GROUND F_CLC_DTOR32(10.0F)
#define WPSAMUSBOMB_WAIT_COLLIDE_MOD_VEL 0.9F
#define WPSAMUSBOMB_WAIT_GRAVITY 1.0F
#define WPSAMUSBOMB_WAIT_TVEL 50.0F
#define WPSAMUSBOMB_WAIT_BLINK_SLOW 40
#define WPSAMUSBOMB_WAIT_BLINK_MID 20
#define WPSAMUSBOMB_WAIT_BLINK_TIMER_SLOW 8
#define WPSAMUSBOMB_WAIT_BLINK_TIMER_MID 5
#define WPSAMUSBOMB_WAIT_BLINK_TIMER_FAST 3

/* Source wpvars/wpsamus declarations kept narrow at the port ABI seam. */
void wpSamusChargeShotLaunch(GObj *weapon_gobj);
sb32 wpSamusChargeShotProcDead(GObj *weapon_gobj);
sb32 wpSamusChargeShotProcUpdate(GObj *weapon_gobj);
sb32 wpSamusChargeShotProcMap(GObj *weapon_gobj);
sb32 wpSamusChargeShotProcHit(GObj *weapon_gobj);
sb32 wpSamusChargeShotProcHop(GObj *weapon_gobj);
sb32 wpSamusChargeShotProcReflector(GObj *weapon_gobj);
sb32 wpSamusBombExplodeProcUpdate(GObj *weapon_gobj);
void wpSamusBombExplodeInitVars(GObj *weapon_gobj);
sb32 wpSamusBombProcUpdate(GObj *weapon_gobj);
sb32 wpSamusBombProcMap(GObj *weapon_gobj);
sb32 wpSamusBombProcHit(GObj *weapon_gobj);
sb32 wpSamusBombProcAbsorb(GObj *weapon_gobj);
sb32 wpSamusBombProcHop(GObj *weapon_gobj);
sb32 wpSamusBombProcReflector(GObj *weapon_gobj);

#include "../../decomp/BattleShip-main/decomp/src/wp/wpsamus/wpsamuschargeshot.c"
#define wpSamusBombMakeWeapon ndsBaseWPSamusBombMakeWeapon
#include "../../decomp/BattleShip-main/decomp/src/wp/wpsamus/wpsamusbomb.c"
#undef wpSamusBombMakeWeapon

volatile u32 gNdsSamusBombMakeCount;
volatile u32 gNdsSamusBombMakeSuccessCount;

GObj *wpSamusBombMakeWeapon(GObj *fighter_gobj, Vec3f *pos)
{
    GObj *weapon_gobj;

    gNdsSamusBombMakeCount++;
    weapon_gobj = ndsBaseWPSamusBombMakeWeapon(fighter_gobj, pos);
    if (weapon_gobj != NULL)
    {
        gNdsSamusBombMakeSuccessCount++;
    }
    return weapon_gobj;
}
