#include <ft/fighter.h>
#include <it/item.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)(gobj)->obj)
#endif

#ifndef WEAPON_ATKCOLL_NUM_MAX
#define WEAPON_ATKCOLL_NUM_MAX 2
#endif

#ifndef ITEM_ATKCOLL_NUM_MAX
#define ITEM_ATKCOLL_NUM_MAX 2
#endif

#ifndef SSB64_NDS_WP_ATTACK_COLL_DECLARED
typedef struct WPAttackPos {
    Vec3f pos_curr;
    Vec3f pos_prev;
    sb32 unk_wphitpos_0x18;
    Mtx44f mtx;
    f32 unk_wphitpos_0x5C;
} WPAttackPos;

typedef struct WPAttackColl {
    s32 attack_state;
    s32 damage;
    f32 stale;
    s32 element;
    Vec3f offsets[WEAPON_ATKCOLL_NUM_MAX];
    f32 size;
    s32 angle;
    u32 knockback_scale;
    u32 knockback_weight;
    u32 knockback_base;
    s32 shield_damage;
    s32 priority;
    u8 interact_mask;
    u16 fgm_id;
    ub32 can_setoff : 1;
    ub32 can_rehit_item : 1;
    ub32 can_rehit_fighter : 1;
    ub32 can_rehit_shield : 1;
    ub32 can_hop : 1;
    ub32 can_reflect : 1;
    ub32 can_absorb : 1;
    ub32 can_not_heal : 1;
    ub32 can_shield : 1;
    u32 motion_attack_id : 6;
    u16 motion_count;
    GMStatFlags stat_flags;
    u16 stat_count;
    s32 attack_count;
    WPAttackPos attack_pos[WEAPON_ATKCOLL_NUM_MAX];
    GMAttackRecord attack_records[GMATTACKREC_NUM_MAX];
} WPAttackColl;
#endif

#ifndef SSB64_NDS_IT_ATTACK_COLL_DECLARED
typedef struct ITAttackPos {
    Vec3f pos_curr;
    Vec3f pos_prev;
    sb32 unk_ithitpos_0x18;
    Mtx44f mtx;
    f32 unk_ithitpos_0x5C;
} ITAttackPos;

typedef struct ITAttackColl {
    s32 attack_state;
    s32 damage;
    f32 throw_mul;
    f32 stale;
    s32 element;
    Vec3f offsets[ITEM_ATKCOLL_NUM_MAX];
    f32 size;
    s32 angle;
    u32 knockback_scale;
    u32 knockback_weight;
    u32 knockback_base;
    s32 shield_damage;
    s32 priority;
    u8 interact_mask;
    u16 fgm_id;
    ub32 can_setoff : 1;
    ub32 can_rehit_item : 1;
    ub32 can_rehit_fighter : 1;
    ub32 can_rehit_shield : 1;
    ub32 can_hop : 1;
    ub32 can_reflect : 1;
    ub32 can_shield : 1;
    u32 motion_attack_id : 6;
    u16 motion_count;
    GMStatFlags stat_flags;
    u16 stat_count;
    s32 attack_count;
    ITAttackPos attack_pos[ITEM_ATKCOLL_NUM_MAX];
    GMAttackRecord attack_records[GMATTACKREC_NUM_MAX];
} ITAttackColl;

typedef struct ITDamageColl {
    u8 interact_mask;
    s32 hitstatus;
    Vec3f offset;
    Vec3f size;
} ITDamageColl;
#endif

f32 lbCommonSin(f32 angle);
f32 lbCommonCos(f32 angle);
void syDebugPrintf(const char *fmt, ...);
void scManagerRunPrintGObjStatus(void);

/* BUGS.md row 4 was settled here and the probe is removed, per the standing rule
 * that temporary instrumentation does not survive a handoff.
 *
 * What it established, so nobody re-runs it: gmCollisionGetFighterAttackDamagePosition
 * is CORRECT. Interposing on it and recording both halves plus the real dst
 * into globals gave atk (2341.83, 33.05), dmg (2380.33, -155.55) and dst
 * (2361.08, -61.25) -- the exact midpoint. Hit sparks get a true contact point,
 * not the origin, and the "sparks at the world origin" reading was a debugger
 * artifact: the function inlines at -Os, so the caller's `pos` lived in
 * registers and the stack slot gdb read had never been written. The
 * non-inlinable wrapper forced the spill and gdb then agreed with the globals
 * to the last digit.
 *
 * Re-add the same way if the row is ever reopened -- gmcollision.c names that
 * function exactly once, its definition, so a #define moves the definition
 * without capturing a call site (the trap battleship_lbparticle.c documents for
 * lbParticleMakeGenerator), and decomp/ stays untouched. */

/* The ring wrapper exists for two independent reasons and either one turns it
 * on: the law-1 engagement counter (NDS_TICK_HUD) and the wired fixed-point
 * cluster (NDS_R2_COLLISION_FIXED). They share one pair of wrappers because
 * they hook the same two entry points, and hooking them twice would mean two
 * renames of one definition. */
#define NDS_R2_CFX_RING_WRAP 1


#if NDS_R2_CFX_RING_WRAP
/* R2-07 slice 52 law-1 counter: the ENGAGEMENT of the fighter narrow phase.
 *
 * `plan.md` §9 law 1 requires a per-frame counter on the SPIKING quantity, on
 * the gate arm, before any code changes. The 2026-08-15 v3 capture on the
 * shipping candidate (`…/2026-08-15_k1-owner-pricing/`) says what spikes: on the
 * 80 frames that SET P95, `gmCollisionCheckFighterAttackDamageCollide` runs at
 * **10.81x** its whole-match rate and every body it reaches runs at 5.2-11.7x --
 * `gmCollisionTestRectangle` 11.70x, `gmCollisionSetInvertMatrix` 11.54x,
 * `func_ovl2_800EDE5C` 11.27x, `func_ovl2_800ED490` 7.68x. The proc that
 * contains them is FLAT (`battleship_ftMainProcUpdateInterrupt` 1.06x,
 * `ndsBaseGcRunAll` 1.03x), so the tick is not spent evenly: it is spent on the
 * frames where a hitbox is live. A whole-match bar cannot see that, which is
 * why the slice-52 sizing that used one under-read the lane.
 *
 * These two entry points are the WHOLE gateway to that geometry. Verified from
 * the linked ELF, not from grep: `…AttackDamageCollide` has exactly two
 * referrers and `…AttackShieldCollide` exactly one, all three in
 * `battleship_ftmain.c` -- ZERO in-TU callers, which is what makes the
 * `#define`-before-include rename above capture every call rather than half of
 * them. (`gmCollisionTestRectangle` and `gmCollisionSetInvertMatrix` do NOT have
 * that property: their callers are inside gmcollision.c, so a rename would move
 * the definition AND the call sites and the counter would read zero forever.)
 *
 * `hits` is not decoration. Slice 52 replaces this arithmetic with fixed point,
 * so the A/B has to show the pair COUNT unchanged (same fight) and the HIT count
 * unchanged (same decisions) before any tick delta means anything -- a fixed
 * kernel that quietly stops returning TRUE would read as a large, clean win.
 *
 * `NDS_TICK_HUD` gates it, so the published ROM carries zero bytes; every
 * measuring arm carries it. `used` because a `volatile u32` whose only consumer
 * is a debugger is what `--gc-sections` collects. */
#if NDS_TICK_HUD
volatile u32 gNdsCfxFighterDamagePhaseCalls __attribute__((used));
volatile u32 gNdsCfxFighterDamagePhaseHits __attribute__((used));
volatile u32 gNdsCfxFighterShieldPhaseCalls __attribute__((used));
volatile u32 gNdsCfxFighterShieldPhaseHits __attribute__((used));
#endif

sb32 ndsBaseGmCollisionCheckFighterAttackDamageCollide(
    FTAttackColl *attack_coll, FTDamageColl *damage_coll);
sb32 ndsBaseGmCollisionCheckFighterAttackShieldCollide(
    FTAttackColl *attack_coll, GObj *fighter_gobj, DObj *dobj, f32 *p_angle);

#define gmCollisionCheckFighterAttackDamageCollide \
    ndsBaseGmCollisionCheckFighterAttackDamageCollide
#define gmCollisionCheckFighterAttackShieldCollide \
    ndsBaseGmCollisionCheckFighterAttackShieldCollide
#endif /* NDS_R2_CFX_RING_WRAP */


/* The weapon/item attack paths against fighter hurtboxes: same joint frame,
 * same TestRectangle. Their only callers are in ftmain.c, so this rename moves
 * the definitions and captures every call. */
sb32 ndsBaseGmCollisionCheckWeaponAttackFighterDamageCollide(
    WPAttackColl *attack_coll, s32 attack_id, FTDamageColl *damage_coll);
sb32 ndsBaseGmCollisionCheckItemAttackFighterDamageCollide(
    ITAttackColl *attack_coll, s32 attack_id, FTDamageColl *damage_coll);
#define gmCollisionCheckWeaponAttackFighterDamageCollide     ndsBaseGmCollisionCheckWeaponAttackFighterDamageCollide
#define gmCollisionCheckItemAttackFighterDamageCollide     ndsBaseGmCollisionCheckItemAttackFighterDamageCollide

#if NDS_P2_JOINT_RESIDENT
/* Owner ruling D13 (2026-10-04): fighter joint worlds stay in fixed point
 * (src/port/nds_p2_hurtbox_reject.c). The decomp body keeps its float walk
 * under the ndsBase name for the joints the resident world declines
 * (animation locks, a guard). gmcollision.c never calls it, so the rename
 * moves the definition and captures every call. */
void ndsBaseGmCollisionGetFighterPartsWorldPosition(DObj *main_dobj,
                                                    Vec3f *vec);
#define gmCollisionGetFighterPartsWorldPosition \
    ndsBaseGmCollisionGetFighterPartsWorldPosition
#endif

#if NDS_P2_JOINT_RESIDENT
/* P2-2p8 (2026-10-05): every entry point into this file that can set a
 * fighter FTParts latch (unk_dobjtrans_0x5/6/7, transform_update_mode 1) bumps
 * gNdsFtPartsLatchWrites first, so the per-tick latch clears
 * (src/port/reloc_backend_compat_shims.c) may skip a subtree no latch was set
 * in since its last clear. These six have no callers inside gmcollision.c, so
 * the rename moves each definition and captures every call;
 * func_ovl2_800EDBA4's three in-file callers (func_ovl2_800EDE00/EDE5C and the
 * world-position body) are reached only through entry points that bump. */
#define NDS_FTPARTS_LATCH_GATE 1
extern volatile u32 gNdsFtPartsLatchWrites;
void ndsLatchBaseFuncOvl2800EDBA4(DObj *main_dobj);
void ndsLatchBaseFuncOvl2800EE018(DObj *main_dobj, Vec3f *vec);
sb32 ndsLatchBaseWeaponAttackShieldCollide(WPAttackColl *attack_coll,
                                            s32 attack_id, GObj *fighter_gobj,
                                            DObj *dobj, f32 *p_angle,
                                            Vec3f *vec);
sb32 ndsLatchBaseWeaponAttackSpecialCollide(WPAttackColl *attack_coll,
                                             s32 attack_id, FTStruct *fp,
                                             FTSpecialColl *special_coll);
sb32 ndsLatchBaseItemAttackShieldCollide(ITAttackColl *attack_coll,
                                          s32 attack_id, GObj *fighter_gobj,
                                          DObj *dobj, f32 *p_angle,
                                          Vec3f *vec);
sb32 ndsLatchBaseItemAttackSpecialCollide(ITAttackColl *attack_coll,
                                           s32 attack_id, FTStruct *fp,
                                           FTSpecialColl *special_coll);
#define func_ovl2_800EDBA4 ndsLatchBaseFuncOvl2800EDBA4
#define func_ovl2_800EE018 ndsLatchBaseFuncOvl2800EE018
#define gmCollisionCheckWeaponAttackShieldCollide \
    ndsLatchBaseWeaponAttackShieldCollide
#define gmCollisionCheckWeaponAttackSpecialCollide \
    ndsLatchBaseWeaponAttackSpecialCollide
#define gmCollisionCheckItemAttackShieldCollide \
    ndsLatchBaseItemAttackShieldCollide
#define gmCollisionCheckItemAttackSpecialCollide \
    ndsLatchBaseItemAttackSpecialCollide
#define NDS_FTPARTS_LATCH_MARK() (gNdsFtPartsLatchWrites++)
#else
#define NDS_FTPARTS_LATCH_GATE 0
#define NDS_FTPARTS_LATCH_MARK() ((void)0)
#endif

/* 2026-10-06: the strong definitions of the three attack-near-fighter tests
 * (bounds once, order-key compares, same answers) are in
 * nds_p2_hurtbox_reject.c. */
#pragma weak gmCollisionCheckFighterInFighterRange
#pragma weak gmCollisionCheckWeaponInFighterRange
#pragma weak gmCollisionCheckItemInFighterRange

#if NDS_P2_JOINT_RESIDENT
/* 2026-10-06 (owner: "hit VFX play at wrong locations"). The hurtbox test
 * decides in fixed point and writes no FTParts latch
 * (src/port/nds_p2_hurtbox_reject.c), but these four read the hurt or shield
 * joint's parts->mtx_translate straight after a hit, relying on the source
 * test's float walk to have filled it this tick: they placed every hit spark,
 * shock and sword slash halfway between the attack and a stale or empty
 * matrix. The strong definitions in src/port/nds_p2_hurtbox_reject.c take
 * the joint's world through gmCollisionGetFighterPartsWorldPosition -- the
 * world the test itself used, with the float walk as its fallback. */
#pragma weak gmCollisionGetShieldPosition
#pragma weak gmCollisionGetFighterAttackDamagePosition
#pragma weak gmCollisionGetWeaponAttackFighterDamagePosition
#pragma weak gmCollisionGetItemAttackFighterDamagePosition
#endif

#include "../../decomp/BattleShip-main/decomp/src/gm/gmcollision.c"

#if NDS_FTPARTS_LATCH_GATE
#undef func_ovl2_800EDBA4
#undef func_ovl2_800EE018
#undef gmCollisionCheckWeaponAttackShieldCollide
#undef gmCollisionCheckWeaponAttackSpecialCollide
#undef gmCollisionCheckItemAttackShieldCollide
#undef gmCollisionCheckItemAttackSpecialCollide

void func_ovl2_800EDBA4(DObj *main_dobj)
{
    NDS_FTPARTS_LATCH_MARK();
    ndsLatchBaseFuncOvl2800EDBA4(main_dobj);
}

void func_ovl2_800EE018(DObj *main_dobj, Vec3f *vec)
{
    NDS_FTPARTS_LATCH_MARK();
    ndsLatchBaseFuncOvl2800EE018(main_dobj, vec);
}

sb32 gmCollisionCheckWeaponAttackShieldCollide(WPAttackColl *attack_coll,
                                               s32 attack_id,
                                               GObj *fighter_gobj, DObj *dobj,
                                               f32 *p_angle, Vec3f *vec)
{
    NDS_FTPARTS_LATCH_MARK();
    return ndsLatchBaseWeaponAttackShieldCollide(attack_coll, attack_id,
                                                 fighter_gobj, dobj, p_angle,
                                                 vec);
}

sb32 gmCollisionCheckWeaponAttackSpecialCollide(WPAttackColl *attack_coll,
                                                s32 attack_id, FTStruct *fp,
                                                FTSpecialColl *special_coll)
{
    NDS_FTPARTS_LATCH_MARK();
    return ndsLatchBaseWeaponAttackSpecialCollide(attack_coll, attack_id, fp,
                                                  special_coll);
}

sb32 gmCollisionCheckItemAttackShieldCollide(ITAttackColl *attack_coll,
                                             s32 attack_id,
                                             GObj *fighter_gobj, DObj *dobj,
                                             f32 *p_angle, Vec3f *vec)
{
    NDS_FTPARTS_LATCH_MARK();
    return ndsLatchBaseItemAttackShieldCollide(attack_coll, attack_id,
                                               fighter_gobj, dobj, p_angle,
                                               vec);
}

sb32 gmCollisionCheckItemAttackSpecialCollide(ITAttackColl *attack_coll,
                                              s32 attack_id, FTStruct *fp,
                                              FTSpecialColl *special_coll)
{
    NDS_FTPARTS_LATCH_MARK();
    return ndsLatchBaseItemAttackSpecialCollide(attack_coll, attack_id, fp,
                                                special_coll);
}
#endif

#if NDS_P2_JOINT_RESIDENT
#undef gmCollisionGetFighterPartsWorldPosition

int ndsP2JointWorldPosition(DObj *joint, Vec3f *vec);

/* Calls that name a joint the fighter never made: counted, the position
 * left as the caller's offset. The source faults on NULL there; a Remix part
 * index read on another fighter can name such a joint (Kirby has no joint 7). */
__attribute__((used)) volatile u32 gNdsCollisionNullJointCount;

void gmCollisionGetFighterPartsWorldPosition(DObj *main_dobj, Vec3f *vec)
{
    if (main_dobj == NULL)
    {
        gNdsCollisionNullJointCount++;
        return;
    }
    if (ndsP2JointWorldPosition(main_dobj, vec) != 0)
    {
        return;
    }
    NDS_FTPARTS_LATCH_MARK();
    ndsBaseGmCollisionGetFighterPartsWorldPosition(main_dobj, vec);
}
#endif

#undef gmCollisionCheckWeaponAttackFighterDamageCollide
#undef gmCollisionCheckItemAttackFighterDamageCollide

int ndsP2HurtboxDecidePoints(const Vec3f *pos_curr, const Vec3f *pos_prev,
                             f32 attack_size, s32 attack_state,
                             const FTDamageColl *damage);
extern volatile u32 gNdsP2HurtboxRejects;
extern volatile u32 gNdsP2HurtboxPasses;

/* The kernel's decision -- a proven miss or, since 2026-10-05, the pair
 * decided in fixed point -- returns TRUE with *hit set; FALSE leaves the pair
 * to the float test. */
static sb32 ndsP2HurtboxDecideGate(const Vec3f *pos_curr, const Vec3f *pos_prev,
                                   f32 attack_size, s32 attack_state,
                                   const FTDamageColl *damage, sb32 *hit)
{
    const int decided = ndsP2HurtboxDecidePoints(pos_curr, pos_prev,
                                                 attack_size, attack_state,
                                                 damage);

    if (decided >= 0)
    {
        NDS_DIAG(gNdsP2HurtboxRejects++);
        *hit = (decided != 0) ? TRUE : FALSE;
        return TRUE;
    }
    NDS_DIAG(gNdsP2HurtboxPasses++);
    return FALSE;
}

sb32 gmCollisionCheckWeaponAttackFighterDamageCollide(
    WPAttackColl *attack_coll, s32 attack_id, FTDamageColl *damage_coll)
{
    sb32 hit = FALSE;

    if (ndsP2HurtboxDecideGate(&attack_coll->attack_pos[attack_id].pos_curr,
                               &attack_coll->attack_pos[attack_id].pos_prev,
                               attack_coll->size, attack_coll->attack_state,
                               damage_coll, &hit) != FALSE)
    {
        return hit;
    }
    NDS_FTPARTS_LATCH_MARK();
    return ndsBaseGmCollisionCheckWeaponAttackFighterDamageCollide(
        attack_coll, attack_id, damage_coll);
}

sb32 gmCollisionCheckItemAttackFighterDamageCollide(
    ITAttackColl *attack_coll, s32 attack_id, FTDamageColl *damage_coll)
{
    sb32 hit = FALSE;

    if (ndsP2HurtboxDecideGate(&attack_coll->attack_pos[attack_id].pos_curr,
                               &attack_coll->attack_pos[attack_id].pos_prev,
                               attack_coll->size, attack_coll->attack_state,
                               damage_coll, &hit) != FALSE)
    {
        return hit;
    }
    NDS_FTPARTS_LATCH_MARK();
    return ndsBaseGmCollisionCheckItemAttackFighterDamageCollide(
        attack_coll, attack_id, damage_coll);
}


#if NDS_R2_CFX_RING_WRAP
#undef gmCollisionCheckFighterAttackDamageCollide
#undef gmCollisionCheckFighterAttackShieldCollide

/* R2-07 slice 52, the wired ring. `ndsR2CfxPrepareFighterJoint` runs
 * func_ovl2_800EDBA4, func_ovl2_800EDE00 and func_ovl2_800EDE5C in fixed point
 * and sets the source's own unk_dobjtrans_0x5/0x6/0x7 latches, so the decomp
 * body below finds its float producers already satisfied and early-returns out
 * of them -- the same joints, in the same order, with no speculative work. Its
 * outputs are the same f32 fields with the same layout; the decision itself is
 * still gmCollisionTestRectangle's, unchanged, in decomp code.
 *
 * Before the base call, not after: the base is where the latches are read. */
/* P2-2p8 A5 (src/port/nds_p2_hurtbox_reject.c): the pair decided in fixed
 * point, or a proof that the float test below would miss; -1 leaves it to
 * the float tail. */
int ndsP2HurtboxDecideTest(const FTAttackColl *attack,
                           const FTDamageColl *damage);
extern volatile u32 gNdsP2HurtboxRejects;
extern volatile u32 gNdsP2HurtboxPasses;

sb32 gmCollisionCheckFighterAttackDamageCollide(FTAttackColl *attack_coll,
                                                FTDamageColl *damage_coll)
{
    sb32 hit;
    {
        /* The kernel proves a miss or, since 2026-10-05, decides the pair in
         * fixed point (src/port/nds_p2_hurtbox_reject.c); -1 leaves it to the
         * float tail below. */
        const int decided = ndsP2HurtboxDecideTest(attack_coll, damage_coll);

        if (decided >= 0)
        {
            NDS_DIAG(gNdsP2HurtboxRejects++);
            return (decided != 0) ? TRUE : FALSE;
        }
        NDS_DIAG(gNdsP2HurtboxPasses++);
    }

    NDS_FTPARTS_LATCH_MARK();
    hit = ndsBaseGmCollisionCheckFighterAttackDamageCollide(attack_coll,
                                                            damage_coll);
#if NDS_TICK_HUD
    NDS_DIAG(gNdsCfxFighterDamagePhaseCalls++);
    if (hit != FALSE)
    {
        NDS_DIAG(gNdsCfxFighterDamagePhaseHits++);
    }
#endif
    return hit;
}

sb32 gmCollisionCheckFighterAttackShieldCollide(FTAttackColl *attack_coll,
                                                GObj *fighter_gobj, DObj *dobj,
                                                f32 *p_angle)
{
    sb32 hit;

    NDS_FTPARTS_LATCH_MARK();
    hit = ndsBaseGmCollisionCheckFighterAttackShieldCollide(
        attack_coll, fighter_gobj, dobj, p_angle);
#if NDS_TICK_HUD
    NDS_DIAG(gNdsCfxFighterShieldPhaseCalls++);
    if (hit != FALSE)
    {
        NDS_DIAG(gNdsCfxFighterShieldPhaseHits++);
    }
#endif
    return hit;
}
#endif /* NDS_R2_CFX_RING_WRAP */

