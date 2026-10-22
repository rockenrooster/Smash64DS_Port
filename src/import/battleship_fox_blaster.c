
#include <ef/effect.h>
#include <ft/fighter.h>
#include <nds/nds_effects.h>
#include <nds/nds_startup.h>
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
extern void gcDrawDObjDLHead1(GObj *gobj);
extern void wpDisplayMain(GObj *weapon_gobj,
                          void (*proc_display)(GObj *));
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
sb32 wpFoxBlasterProcUpdate(GObj *weapon_gobj);
sb32 wpFoxBlasterProcMap(GObj *weapon_gobj);
sb32 wpFoxBlasterProcHit(GObj *weapon_gobj);
sb32 wpFoxBlasterProcHop(GObj *weapon_gobj);
sb32 wpFoxBlasterProcReflector(GObj *weapon_gobj);
void ftFoxSpecialNSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirNSetStatus(GObj *fighter_gobj);

/* OWNER 2026-09-21: no muzzle flash on Kirby's copied blaster.
 *
 * The source calls this maker in three places -- once at the muzzle inside
 * wpFoxBlasterMakeWeapon, and again from ProcHit/ProcMap/ProcHop when the bolt
 * lands. On Fox the muzzle call lands out at the gun barrel and reads as a
 * muzzle flash. On Kirby the same position comes off HIS joint 17, which sits
 * on his body, so the flash reads as Kirby himself flashing every time he
 * fires. The owner asked for that gone and pointed at Fox as the reference.
 *
 * Only the muzzle call is suppressed, and only for Kirby. The impact glows
 * happen at the projectile, nowhere near the shooter, and are the same on both
 * fighters. If the placement is ever corrected rather than removed, delete the
 * flag and this comment together -- the maker itself is unchanged. */
sb32 gNdsFoxBlasterSuppressMuzzleGlow;

/* Keep the source impact event while using the bounded untextured DS shape. */
__attribute__((weak)) LBParticle *efManagerFoxBlasterGlowMakeEffect(Vec3f *pos)
{
    if (gNdsFoxBlasterSuppressMuzzleGlow != FALSE)
    {
        return NULL;
    }
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectHitElectric, pos,
                                       0.55F, 1, NULL);
    return NULL;
}

#define wpFoxBlasterMakeWeapon battleship_wpFoxBlasterMakeWeapon
#include "../../decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c"
#undef wpFoxBlasterMakeWeapon

/* FoxSpecial1's two source attack_offsets are both zero and the relocation
 * validator in reloc_backend_assets.c pins that invariant. Keep the DS bore
 * correction as a replacement for those zero Y offsets, not an accumulated
 * float add. This is exact, constant-time and emits no software-float helper. */
static void ndsFoxBlasterSetBoreAttackOffsets(WPStruct *wp)
{
    f32 local_y;

    if (wp == NULL)
    {
        return;
    }
    /* wpProcessUpdateHitOffsets rotates this weapon-local vector by the beam's
     * Rz. Right-facing is 0; left-facing is pi. Sign the local Y with lr so the
     * transformed correction is world +Y in both cases. */
    local_y = (wp->lr < 0) ?
        -(f32)NDS_FOX_BLASTER_BORE_OFFSET_Y :
         (f32)NDS_FOX_BLASTER_BORE_OFFSET_Y;
    wp->attack_coll.offsets[0].y = local_y;
    wp->attack_coll.offsets[1].y = local_y;
}

/* Reflection can flip lr / horizontal velocity and then recomputes the beam's
 * Rz in the BattleShip callback. Re-sign the local collision offset AFTER that
 * source callback so a reflected shot keeps the same world +Y bore line rather
 * than mirroring the hitbox below the weapon. */
static sb32 ndsFoxBlasterProcReflectorBore(GObj *weapon_gobj)
{
    sb32 result = wpFoxBlasterProcReflector(weapon_gobj);

    ndsFoxBlasterSetBoreAttackOffsets(wpGetStruct(weapon_gobj));
    return result;
}

/* Kirby-owned blasters run every glow-raising callback with the suppression
 * flag up. The source procs are otherwise untouched -- physics, damage,
 * reflection and lifetime are all still BattleShip's; only the DS stand-in for
 * EFCommon script 0x62 declines. Fox's weapons never take these wrappers. */
static sb32 (*sNdsFoxBlasterKirbyProcMap)(GObj *);
static sb32 (*sNdsFoxBlasterKirbyProcHop)(GObj *);
/* Both are defined further down this file, after the source include. */
static sb32 ndsFoxBlasterProcHitCounted(GObj *weapon_gobj);
static sb32 ndsFoxBlasterProcReflectorBore(GObj *weapon_gobj);

static sb32 ndsFoxBlasterProcHitKirbyNoGlow(GObj *weapon_gobj)
{
    sb32 result;

    gNdsFoxBlasterSuppressMuzzleGlow = TRUE;
    result = ndsFoxBlasterProcHitCounted(weapon_gobj);
    gNdsFoxBlasterSuppressMuzzleGlow = FALSE;
    return result;
}

static sb32 ndsFoxBlasterProcMapKirbyNoGlow(GObj *weapon_gobj)
{
    sb32 result;

    gNdsFoxBlasterSuppressMuzzleGlow = TRUE;
    result = (sNdsFoxBlasterKirbyProcMap != NULL) ?
        sNdsFoxBlasterKirbyProcMap(weapon_gobj) :
        wpFoxBlasterProcMap(weapon_gobj);
    gNdsFoxBlasterSuppressMuzzleGlow = FALSE;
    return result;
}

static sb32 ndsFoxBlasterProcHopKirbyNoGlow(GObj *weapon_gobj)
{
    sb32 result;

    gNdsFoxBlasterSuppressMuzzleGlow = TRUE;
    result = (sNdsFoxBlasterKirbyProcHop != NULL) ?
        sNdsFoxBlasterKirbyProcHop(weapon_gobj) :
        wpFoxBlasterProcHop(weapon_gobj);
    gNdsFoxBlasterSuppressMuzzleGlow = FALSE;
    return result;
}

static sb32 ndsFoxBlasterProcReflectorKirbyNoGlow(GObj *weapon_gobj)
{
    sb32 result;

    gNdsFoxBlasterSuppressMuzzleGlow = TRUE;
    result = ndsFoxBlasterProcReflectorBore(weapon_gobj);
    gNdsFoxBlasterSuppressMuzzleGlow = FALSE;
    return result;
}

/* wpManager selects func_ovl3_80167618 for this descriptor, whose source
 * callback eventually reaches lbCommonDObjScaleXProcDisplay. The port's
 * shared compatibility definition of that function is deliberately a no-op:
 * effect descriptors also reference it, so making it globally draw would
 * revive unrelated source models and can double-submit their DS replacements.
 *
 * Own the seam here instead. wpDisplayMain retains the source weapon's
 * translucent/no-Z render state, while gcDrawDObjDLHead1 hands this one DObj
 * to the DS renderer. This is also the valid interpreted control for the
 * native-quad lab; the old zero-draw callback is not a performance baseline. */
static void ndsFoxBlasterProcDisplay(GObj *weapon_gobj)
{
    wpDisplayMain(weapon_gobj, gcDrawDObjDLHead1);
}

sb32 ndsFoxBlasterIsNativeDisplayGObj(void *weapon_gobj_ptr)
{
    GObj *weapon_gobj = weapon_gobj_ptr;
    WPStruct *wp;

    if ((weapon_gobj == NULL) ||
        (weapon_gobj->proc_display != ndsFoxBlasterProcDisplay))
    {
        return FALSE;
    }
    wp = wpGetStruct(weapon_gobj);
    return ((wp != NULL) && (wp->kind == nWPKindBlaster)) ? TRUE : FALSE;
}


/* P2-3r3: the natural projectile proof samples the weapon list once per
 * VSBattle update, but a point-blank laser is born and destroyed inside ONE
 * gcRunAll (probe: spawn at x=-220, one 160-unit step, proc_hit for 6 damage,
 * destroyed at lifetime 0) -- no sampler cadence can see it. The Mario
 * fireball credits its own destroy paths for exactly this reason
 * (battleship_mario_fireball.c:652/689/700); this is the blaster's twin: at
 * the hit seam the attack coll is live, so record the same kind/state/damage
 * facts the list walk would have, then run the source hit proc unchanged. */
static sb32 ndsFoxBlasterProcHitCounted(GObj *weapon_gobj)
{
    WPStruct *wp = (WPStruct *)weapon_gobj->user_data.p;

    gNdsFighterProjectileProofHitDestroyCount++;
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
    if ((u32)wp->attack_coll.damage > gNdsFighterProjectileProofDamageMax)
    {
        gNdsFighterProjectileProofDamageMax = (u32)wp->attack_coll.damage;
    }
    return wpFoxBlasterProcHit(weapon_gobj);
}

GObj *wpFoxBlasterMakeWeapon(GObj *fighter_gobj, Vec3f *pos)
{
    GObj *weapon_gobj;
    sb32 shooter_is_kirby;

    gNdsFighterProjectileProofSpawnCallCount++;
    {
        FTStruct *shooter =
            (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

        shooter_is_kirby =
            ((shooter != NULL) && (shooter->fkind == nFTKindKirby)) ?
                TRUE : FALSE;
    }
    gNdsFoxBlasterSuppressMuzzleGlow = shooter_is_kirby;
    weapon_gobj = battleship_wpFoxBlasterMakeWeapon(fighter_gobj, pos);
    gNdsFoxBlasterSuppressMuzzleGlow = FALSE;
    if (weapon_gobj != NULL)
    {
        WPStruct *wp = (WPStruct *)weapon_gobj->user_data.p;

        /* OWNER 2026-08-14: collision follows the same corrected bore line as
         * the visible beam. Previously only rendering moved, leaving the source
         * attack sphere at Y=223.398 while the visible beam was +72 world units
         * higher. Natural SquatWait evidence still hit Mario's slot-1 hurtbox:
         * radius 20 reached 243.398 while that crouch box ended at 242.218.
         *
         * BattleShip's wpProcessUpdateHitPositions owns attack_pos initialization
         * and regeneration from these offsets. WPStruct is free-list storage, so
         * attack_pos may still contain the previous weapon's values here; do NOT
         * read or patch it. Changing the two source-owned offsets is sufficient
         * for the first nGMAttackStateNew update and every later update. The
         * weapon DObj/root and WPAttributes map-collision box stay source-exact.
         * Radius remains the source-exact 20 -- no hitbox-thinning guess. */
        ndsFoxBlasterSetBoreAttackOffsets(wp);
        wp->proc_reflector = ndsFoxBlasterProcReflectorBore;
        wp->proc_hit = ndsFoxBlasterProcHitCounted;
        weapon_gobj->proc_display = ndsFoxBlasterProcDisplay;
        /* OWNER 2026-09-21, second pass: Kirby's copied blaster must show no
         * glow AT ALL, not just no muzzle flash.
         *
         * Suppressing only the muzzle left the three IMPACT calls live, and on
         * Kirby those land on him too: the bore offset is Fox's, Kirby is much
         * shorter, and the bolt reaches a surface almost immediately, so the
         * hit/map/hop glow fires at point-blank range and reads as a white and
         * gold flash over his body. Mark the WEAPON rather than the shooter,
         * because these callbacks run long after the constructor returned and
         * have no fighter argument of their own. */
        if ((shooter_is_kirby != FALSE) && (wp->proc_map != NULL))
        {
            sNdsFoxBlasterKirbyProcMap = wp->proc_map;
            wp->proc_map = ndsFoxBlasterProcMapKirbyNoGlow;
        }
        if (shooter_is_kirby != FALSE)
        {
            sNdsFoxBlasterKirbyProcHop = wp->proc_hop;
            wp->proc_hit = ndsFoxBlasterProcHitKirbyNoGlow;
            wp->proc_shield = ndsFoxBlasterProcHitKirbyNoGlow;
            wp->proc_setoff = ndsFoxBlasterProcHitKirbyNoGlow;
            wp->proc_absorb = ndsFoxBlasterProcHitKirbyNoGlow;
            wp->proc_hop = ndsFoxBlasterProcHopKirbyNoGlow;
            wp->proc_reflector = ndsFoxBlasterProcReflectorKirbyNoGlow;
        }
        gNdsFighterProjectileProofSpawnSuccessCount++;
        /* One weapon is provably live right now; the once-per-update list
         * sampler can miss a same-update lifetime entirely (see the hit
         * seam above). */
        if (gNdsFighterProjectileProofWeaponCountMax < 1u)
        {
            gNdsFighterProjectileProofWeaponCountMax = 1u;
        }
    }
    return weapon_gobj;
}

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspecialn.c"

