#if NDS_IMPORT_BATTLESHIP_FOX_BLASTER

#include <ef/effect.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef FTFOX_BLASTER_HOLD_JOINT
#define FTFOX_BLASTER_HOLD_JOINT 17
#endif

#ifndef FTFOX_BLASTER_SPAWN_OFF_X
#define FTFOX_BLASTER_SPAWN_OFF_X 60.0F
#endif

#ifndef WPBLASTER_VEL_X
#define WPBLASTER_VEL_X 160.0F
#endif

#ifndef WPBLASTER_ADD_SCALE_X
#define WPBLASTER_ADD_SCALE_X (16.0F / 3.0F)
#endif

#ifndef WPBLASTER_CLAMP_SCALE_X
#define WPBLASTER_CLAMP_SCALE_X (160.0F / 3.0F)
#endif

uintptr_t llFoxSpecial1BlasterWeaponAttributes;

extern f32 syUtilsArcTan2(f32 y, f32 x);
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
sb32 wpFoxBlasterProcUpdate(GObj *weapon_gobj);
sb32 wpFoxBlasterProcMap(GObj *weapon_gobj);
sb32 wpFoxBlasterProcHit(GObj *weapon_gobj);
sb32 wpFoxBlasterProcHop(GObj *weapon_gobj);
sb32 wpFoxBlasterProcReflector(GObj *weapon_gobj);
void ftFoxSpecialNSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirNSetStatus(GObj *fighter_gobj);

// ponytail: particle visuals stay stubbed until the effect-manager memory gate.
__attribute__((weak)) LBParticle *efManagerFoxBlasterGlowMakeEffect(Vec3f *pos)
{
    (void)pos;
    return NULL;
}

#include "../../decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspecialn.c"

#endif
