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
#include <sys/taskman.h>
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

static sb32 ndsMarioFireballProcReflectorProbe(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    sb32 result;

    if (wp != NULL)
    {
        gNdsFighterReflectorProofFireballVelXBefore =
            (s32)(wp->physics.vel_air.x * 1000.0F);
    }
    result = wpMarioFireballProcReflector(weapon_gobj);
    if (wp != NULL)
    {
        FTStruct *owner = ftGetStruct(wp->owner_gobj);

        gNdsFighterReflectorProofFireballProcCount++;
        gNdsFighterReflectorProofFireballVelXAfter =
            (s32)(wp->physics.vel_air.x * 1000.0F);
        if (owner != NULL)
        {
            gNdsFighterReflectorProofFireballOwnerKind = (u32)owner->fkind;
        }
    }
    return result;
}

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
    gNdsFighterReflectorProofFireballCanReflect =
        (wp->attack_coll.can_reflect != FALSE) ? 1u : 0u;
    gNdsFighterReflectorProofFireballCanAbsorb =
        (wp->attack_coll.can_absorb != FALSE) ? 1u : 0u;
    gNdsFighterReflectorProofFireballCanShield =
        (wp->attack_coll.can_shield != FALSE) ? 1u : 0u;
    gNdsFighterReflectorProofFireballAttackCount =
        (u32)wp->attack_coll.attack_count;
    gNdsFighterReflectorProofFireballDamage =
        (u32)wp->attack_coll.damage;
    gNdsFighterReflectorProofFireballSizeMilli =
        (u32)(wp->attack_coll.size * 1000.0F);
    if ((u32)wp->lifetime > gNdsFighterProjectileProofLifetimeMax)
    {
        gNdsFighterProjectileProofLifetimeMax = (u32)wp->lifetime;
    }
}

/* wpmanager.c's free list, and it is an ordinary global rather than a static,
 * so the exact reason a make failed is readable from here without touching the
 * decomp TU. wpManagerMakeWeapon has exactly two NULL returns: the struct pool
 * is empty (head stays NULL), or gcMakeGObjSPAfter refused and the struct was
 * pushed back (head is non-NULL). Sampled AFTER the call, that is a decision,
 * not a heuristic. */
extern WPStruct *sWPManagerStructsAllocFree;

/* objman.c's GObj pool state. gcGetGObjSetNextAlloc (objman.c:398) refuses only
 * when sGCCommonsMaxNum is not -1 and the pool is at that cap, which is the
 * ifCommonSetMaxNumGObj latch. Reading it at END of run reads the Results
 * scene, where it is -1 again; reading it AT the refusal is the evidence. */
extern s16 sGCCommonsMaxNum;
extern s32 sGCCommonsActiveNum;

/* Weapons alive, exactly: WEAPON_ALLOC_MAX minus the free-list depth. The
 * counter this replaces was a fixed 1 -- it latched on the first weapon and
 * never moved -- so a 2026-08-01 soak reading `WeaponCountMax 1` alongside
 * `SpawnCall 11 / SpawnSuccess 7` looked like a one-at-a-time pool limit and
 * was not evidence of anything. Walking <=32 nodes eleven times a match is
 * free. */
static void ndsMarioFireballSampleWeaponPool(void)
{
    const WPStruct *node = sWPManagerStructsAllocFree;
    u32 free_depth = 0u;
    u32 live;

    while ((node != NULL) && (free_depth <= WEAPON_ALLOC_MAX))
    {
        free_depth++;
        node = node->next;
    }
    live = (free_depth <= WEAPON_ALLOC_MAX) ?
        ((u32)WEAPON_ALLOC_MAX - free_depth) : 0u;
    if (live > gNdsFighterProjectileProofWeaponCountMax)
    {
        gNdsFighterProjectileProofWeaponCountMax = live;
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
    dWPMarioFireballWeaponDesc.proc_reflector =
        ndsMarioFireballProcReflectorProbe;
    weapon_gobj = battleship_wpMarioFireballMakeWeapon(fighter_gobj, pos,
                                                       index);
    if (weapon_gobj != NULL)
    {
        gNdsFighterProjectileProofSpawnSuccessCount++;
        ndsMarioFireballRecordCreatedWeapon(weapon_gobj);
    }
    else if (sWPManagerStructsAllocFree != NULL)
    {
        gNdsFighterProjectileProofSpawnFailGObjCount++;
        gNdsFighterProjectileProofSpawnFailGObjMax =
            (u32)(u16)sGCCommonsMaxNum;
        gNdsFighterProjectileProofSpawnFailGObjActive =
            (u32)sGCCommonsActiveNum;
        /* The number the cap is actually about. ifCommonSetMaxNumGObj latches
         * when this drops under 25,600, so this says how far the battle is
         * from never latching -- i.e. how many bytes a pool trim has to
         * return before the fireball stops being refused. */
        gNdsFighterProjectileProofSpawnFailHeapFree =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                  (uintptr_t)gSYTaskmanGeneralHeap.ptr);
        gNdsWeaponStructBytes = (u32)sizeof(WPStruct);
    }
    else
    {
        gNdsFighterProjectileProofSpawnFailPoolCount++;
    }
    ndsMarioFireballSampleWeaponPool();
    return weapon_gobj;
}

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialn.c"

#endif
