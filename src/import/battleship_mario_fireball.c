#if NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL

#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mp/map.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef FTMARIO_FIREBALL_SPAWN_JOINT
#define FTMARIO_FIREBALL_SPAWN_JOINT 16
#endif

uintptr_t llMarioMainFireballWeaponAttributes;
uintptr_t llMarioSpecial1FireballWeaponAttributes;
uintptr_t llLuigiSpecial1FireballWeaponAttributes;

extern f32 lbCommonMag2D(Vec3f *vec);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
void ftMarioSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftMarioSpecialNSwitchStatusAir(GObj *fighter_gobj);

#define wpMarioFireballMakeWeapon battleship_wpMarioFireballMakeWeapon
#include "../../decomp/BattleShip-main/decomp/src/wp/wpmario/wpmariofireball.c"
#undef wpMarioFireballMakeWeapon

static sb32 ndsMarioFireballProcUpdateProbe(GObj *weapon_gobj)
{
    sb32 result = wpMarioFireballProcUpdate(weapon_gobj);

    if (result != FALSE)
    {
        gNdsFighterProjectileProofUpdateDestroyCount++;
    }
    return result;
}

static sb32 ndsMarioFireballProcMapProbe(GObj *weapon_gobj)
{
    sb32 result = wpMarioFireballProcMap(weapon_gobj);

    if (result != FALSE)
    {
        gNdsFighterProjectileProofMapDestroyCount++;
    }
    return result;
}

static sb32 ndsMarioFireballProcHitProbe(GObj *weapon_gobj)
{
    sb32 result = wpMarioFireballProcHit(weapon_gobj);

    if (result != FALSE)
    {
        gNdsFighterProjectileProofHitDestroyCount++;
    }
    return result;
}

static void ndsMarioFireballRecordCreatedWeapon(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wp == NULL)
    {
        return;
    }
    if ((wp->kind >= 0) && (wp->kind < 32))
    {
        gNdsFighterProjectileProofKindMask |= 1u << wp->kind;
    }
    if ((wp->attack_coll.attack_state >= 0) &&
        (wp->attack_coll.attack_state < 32))
    {
        gNdsFighterProjectileProofAttackStateMask |=
            1u << wp->attack_coll.attack_state;
    }
    if ((u32)wp->attack_coll.damage >
        gNdsFighterProjectileProofDamageMax)
    {
        gNdsFighterProjectileProofDamageMax =
            (u32)wp->attack_coll.damage;
    }
    if ((u32)wp->lifetime > gNdsFighterProjectileProofLifetimeMax)
    {
        gNdsFighterProjectileProofLifetimeMax = (u32)wp->lifetime;
    }
    if (gNdsFighterProjectileProofWeaponCountMax == 0u)
    {
        gNdsFighterProjectileProofWeaponCountMax = 1u;
    }
}

GObj *wpMarioFireballMakeWeapon(GObj *fighter_gobj, Vec3f *pos, s32 index)
{
    GObj *weapon_gobj;

    gNdsFighterProjectileProofSpawnCallCount++;
    dWPMarioFireballWeaponDesc.proc_update = ndsMarioFireballProcUpdateProbe;
    dWPMarioFireballWeaponDesc.proc_map = ndsMarioFireballProcMapProbe;
    dWPMarioFireballWeaponDesc.proc_hit = ndsMarioFireballProcHitProbe;
    dWPMarioFireballWeaponDesc.proc_shield = ndsMarioFireballProcHitProbe;
    dWPMarioFireballWeaponDesc.proc_setoff = ndsMarioFireballProcHitProbe;
    dWPMarioFireballWeaponDesc.proc_absorb = ndsMarioFireballProcHitProbe;
    weapon_gobj = battleship_wpMarioFireballMakeWeapon(fighter_gobj, pos,
                                                       index);
    if (weapon_gobj != NULL)
    {
        gNdsFighterProjectileProofSpawnSuccessCount++;
        ndsMarioFireballRecordCreatedWeapon(weapon_gobj);
    }
    return weapon_gobj;
}

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialn.c"

#endif
