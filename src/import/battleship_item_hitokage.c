/* P2 Saffron City Charmander (kind nITKindHitokage). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itground/ithitokage.c:15-317.
 *
 * Art/state live in the stage's own map file: the descriptor's file base is
 * gGRCommonStruct.yamabuki.item_head (port include/gr/ground.h:431-434) and
 * the reloc tokens below are decomp/BattleShip-main/include/reloc_data.us.h
 * :4005 (:0x1FC attributes) and :4006 (:0x244 flame weapon attributes); the
 * port's generated reloc header does not publish Yamabuki tokens, so this TU
 * owns its uintptr_t tokens the same way battleship_item_gbumper.c owns
 * GBumper's (local tokens, no generator involvement, no hand-edited
 * generated file). The WPDesc carries them with the same &token shape as
 * battleship_item_dogas.c:135-157.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal
 * (ITHITOKAGE_* rides on <it/item.h>). The monster-weapon select macros are
 * decomp gr/grvars.h:28-31, transcribed verbatim under guard because the port
 * only publishes GRYAMABUKI_MONSTER_WEAPON_MAX. Symbols the port headers do
 * not publish yet (nSYAudioVoiceYamabukiHitokage, decomp gmsound.h:650;
 * dGRYamabukiMonsterAttackKind, defined by the Yamabuki stage TU's decomp
 * include :14; wpMapTestAllCheckCollEnd; gITManagerParticleBankID) are
 * referenced verbatim through local externs and listed in the task report --
 * no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <wp/weapon.h>
#include <ft/fighter.h>
#include <common.h>
#include <gr/ground.h>
#include <ef/effect.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:4005. */
uintptr_t llGRYamabukiMapHitokageItemAttributes = 0x1FCu;
/* decomp/BattleShip-main/include/reloc_data.us.h:4006. */
uintptr_t llGRYamabukiMapHitokageFlameWeaponAttributes = 0x244u;

/* decomp/BattleShip-main/decomp/src/gr/grvars.h:28-31. The port publishes
 * only GRYAMABUKI_MONSTER_WEAPON_MAX (ground.h:85); guarded so a later port
 * declaration wins without a redefinition. */
#ifndef GRYAMABUKI_MONSTER_WEAPON_NONE
#define GRYAMABUKI_MONSTER_WEAPON_NONE      (0)
#endif
#ifndef GRYAMABUKI_MONSTER_WEAPON_WAIT
#define GRYAMABUKI_MONSTER_WEAPON_WAIT      (1)
#endif
#ifndef GRYAMABUKI_MONSTER_WEAPON_INSTANT
#define GRYAMABUKI_MONSTER_WEAPON_INSTANT   (2)
#endif
#ifndef GRYAMABUKI_MONSTER_WEAPON_ALL
#define GRYAMABUKI_MONSTER_WEAPON_ALL       (3)
#endif

/* decomp gryamabuki.c:14. Owned by the Yamabuki stage TU; both this file and
 * battleship_item_fushigibana.c read and write it through extern, exactly as
 * the two decomp sources do (ithitokage.c:7, itfushigibana.c:14). */
extern s32 dGRYamabukiMonsterAttackKind;

/* decomp gryamabuki.c:202. Owned by the Yamabuki stage TU; same seam as
 * battleship_item_glucky.c. */
extern void grYamabukiGateClearMonsterGObj(void);

/* decomp sys/utils.h:19-:20. Same seam as
 * battleship_item_box.c:85-86. */
extern s32 syUtilsRandIntRange(s32 range);

/* No port header publishes it; battleship_item_fflower.c:82 carries the same
 * local extern. */
extern sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);

extern LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
extern LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos);

/* Same shape as battleship_item_fflower.c:85. */
extern s32 gITManagerParticleBankID;

/* decomp ithitokage.h:8-19 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly as
 * the Tomato and Star files carry theirs. */
extern sb32 itHitokageCommonProcUpdate(GObj *item_gobj);
extern sb32 itHitokageDamagedProcUpdate(GObj *item_gobj);
extern sb32 itHitokageDamagedProcDead(GObj *item_gobj);
extern sb32 itHitokageCommonProcDamage(GObj *item_gobj);
extern GObj* itHitokageMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itHitokageWeaponFlameProcUpdate(GObj *weapon_gobj);
extern sb32 itHitokageWeaponFlameProcMap(GObj *weapon_gobj);
extern sb32 itHitokageWeaponFlameProcHit(GObj *weapon_gobj);
extern sb32 itHitokageWeaponFlameProcReflector(GObj *weapon_gobj);
extern GObj* itHitokageWeaponFlameMakeWeapon(GObj *item_gobj, Vec3f *pos, Vec3f *vel);
extern void itHitokageCommonMakeFlame(GObj *item_gobj, Vec3f *pos);

// decomp ithitokage.c:15-37 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITHitokageItemDesc =
{
    nITKindHitokage,                        // Item Kind
    &gGRCommonStruct.yamabuki.item_head,    // Pointer to item file data?
    &llGRYamabukiMapHitokageItemAttributes, // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itHitokageCommonProcUpdate,             // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    itHitokageCommonProcDamage              // Proc Damage
};

// decomp ithitokage.c:39-52 verbatim.
ITStatusDesc dITHitokageStatusDescs[/* */] =
{
    // Status 0 (Neutral Damage)
    {
        itHitokageDamagedProcUpdate,        // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// decomp ithitokage.c:54-76 verbatim, adapted only for the port's WPDesc
// shape (o_attributes is intptr_t here; the &token shape is the same one
// battleship_item_dogas.c:135-157 uses).
WPDesc dITHitokageWeaponFlameWeaponDesc =
{
    0x00,                                   // Render flags?
    nWPKindHitokageFlame,                   // Weapon Kind
    &gGRCommonStruct.yamabuki.item_head,    // Pointer to character's loaded files?
    &llGRYamabukiMapHitokageFlameWeaponAttributes,// Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,         // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    itHitokageWeaponFlameProcUpdate,        // Proc Update
    itHitokageWeaponFlameProcMap,           // Proc Map
    itHitokageWeaponFlameProcHit,           // Proc Hit
    itHitokageWeaponFlameProcHit,           // Proc Shield
    NULL,                                   // Proc Hop
    itHitokageWeaponFlameProcHit,           // Proc Set-Off
    itHitokageWeaponFlameProcReflector,     // Proc Reflector
    NULL                                    // Proc Absorb
};

// decomp ithitokage.c:84-88 verbatim.
enum itHitokageStatus
{
    itHitokageStatusDamaged,
    itHitokageStatusEnumCount
};

// 0x80183DA0
// decomp ithitokage.c:97-102 verbatim.
void itHitokageDamagedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITHitokageStatusDescs, itHitokageStatusDamaged);

    itGetStruct(item_gobj)->proc_dead = itHitokageDamagedProcDead;
}

// 0x80183DE0
// decomp ithitokage.c:105-144 verbatim.
sb32 itHitokageCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f pos;

    dobj->translate.vec.f.x += ip->item_vars.hitokage.offset.x;
    dobj->translate.vec.f.y += ip->item_vars.hitokage.offset.y;

    pos = dobj->translate.vec.f;

    pos.x += ITHITOKAGE_FLAME_SPAWN_OFF_X;

    if
    (
        (ip->item_vars.hitokage.flags == GRYAMABUKI_MONSTER_WEAPON_INSTANT)                                                  ||
        ((ip->item_vars.hitokage.flags & GRYAMABUKI_MONSTER_WEAPON_WAIT) && (dobj->anim_frame >= ITHITOKAGE_FLAME_SPAWN_BEGIN)) &&
        (dobj->anim_frame <= ITHITOKAGE_FLAME_SPAWN_END)
    )
    {
        dobj->mobj->texture_id_curr = 1;

        if (ip->item_vars.hitokage.flame_spawn_wait <= 0)
        {
            itHitokageCommonMakeFlame(item_gobj, &pos);

            ip->item_vars.hitokage.flame_spawn_wait = ITHITOKAGE_FLAME_SPAWN_WAIT;
        }
        else ip->item_vars.hitokage.flame_spawn_wait--;
    }
    else dobj->mobj->texture_id_curr = 0;

    if (dobj->anim_wait == AOBJ_ANIM_NULL)
    {
        grYamabukiGateSetClosedWait();

        return TRUE;
    }
    return FALSE;
}

// 0x80183F20
// decomp ithitokage.c:147-159 verbatim.
sb32 itHitokageDamagedProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj;

    itMainApplyGravityClampTVel(ip, ITHITOKAGE_GRAVITY, ITHITOKAGE_TVEL);

    dobj = DObjGetStruct(item_gobj);

    dobj->rotate.vec.f.z -= (ITHITOKAGE_HIT_ROTATE_Z * ip->lr);

    return FALSE;
}

// 0x80183F88
// decomp ithitokage.c:162-165 verbatim.
sb32 itHitokageDamagedProcDead(GObj *item_gobj)
{
    return TRUE;
}

// 0x80183F94
// decomp ithitokage.c:168-189 verbatim.
sb32 itHitokageCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->damage_knockback >= ITHITOKAGE_NDAMAGE_KNOCKBACK_MIN)
    {
        f32 angle = ftCommonDamageGetKnockbackAngle(ip->damage_angle, ip->ga, ip->damage_knockback);

        ip->physics.vel_air.x = __cosf(angle) * ip->damage_knockback * -ip->damage_lr;
        ip->physics.vel_air.y = __sinf(angle) * ip->damage_knockback;

        ip->attack_coll.attack_state = nGMAttackStateOff;
        ip->damage_coll.hitstatus = nGMHitStatusNone;

        dobj->anim_wait = AOBJ_ANIM_NULL;

        grYamabukiGateClearMonsterGObj();
        itHitokageDamagedSetStatus(item_gobj);
    }
    return FALSE;
}

// 0x80184058
// decomp ithitokage.c:192-226 verbatim.
GObj* itHitokageMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITHitokageItemDesc, pos, vel, flags);
    s32 unused;
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        ip = itGetStruct(item_gobj);
        dobj = DObjGetStruct(item_gobj);

        ip->item_vars.hitokage.flame_spawn_wait = 0;
        ip->item_vars.hitokage.offset = *pos;

        ip->is_allow_knockback = TRUE;

        ip->item_vars.hitokage.flags = syUtilsRandIntRange(GRYAMABUKI_MONSTER_WEAPON_MAX);

        if ((dGRYamabukiMonsterAttackKind == ip->item_vars.hitokage.flags) || (ip->item_vars.hitokage.flags & dGRYamabukiMonsterAttackKind))
        {
            ip->item_vars.hitokage.flags++;

            ip->item_vars.hitokage.flags %= GRYAMABUKI_MONSTER_WEAPON_MAX;
        }
        if (ip->item_vars.hitokage.flags == GRYAMABUKI_MONSTER_WEAPON_INSTANT)
        {
            dobj->mobj->texture_id_curr = 1;
        }
        dGRYamabukiMonsterAttackKind = ip->item_vars.hitokage.flags;

        func_800269C0_275C0(nSYAudioVoiceYamabukiHitokage);
    }
    return item_gobj;
}

// 0x8018415C
// decomp ithitokage.c:229-238 verbatim.
sb32 itHitokageWeaponFlameProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x80184188
// decomp ithitokage.c:241-250 verbatim.
sb32 itHitokageWeaponFlameProcMap(GObj *weapon_gobj)
{
    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    else return FALSE;
}

// 0x801841CC
// decomp ithitokage.c:253-259 verbatim.
sb32 itHitokageWeaponFlameProcHit(GObj *weapon_gobj)
{
    func_800269C0_275C0(nSYAudioFGMExplodeS);
    efManagerSparkleWhiteMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f);

    return FALSE;
}

// 0x80184204
// decomp ithitokage.c:262-278 verbatim.
sb32 itHitokageWeaponFlameProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);
    Vec3f *translate;

    wp->lifetime = ITHITOKAGE_FLAME_LIFETIME;

    wpMainReflectorSetLR(wp, fp);

    translate = &DObjGetStruct(weapon_gobj)->translate.vec.f;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);

    return FALSE;
}

// 0x801842C8
// decomp ithitokage.c:281-302 verbatim.
GObj* itHitokageWeaponFlameMakeWeapon(GObj *item_gobj, Vec3f *pos, Vec3f *vel)
{
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, &dITHitokageWeaponFlameWeaponDesc, pos, WEAPON_FLAG_PARENT_ITEM);
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air = *vel;

    wp->lifetime = ITHITOKAGE_FLAME_LIFETIME;

    wp->lr = -1;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, pos->x, pos->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, pos->x, pos->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);

    return weapon_gobj;
}

// 0x801843C4
// decomp ithitokage.c:305-317 verbatim.
void itHitokageCommonMakeFlame(GObj *item_gobj, Vec3f *pos)
{
    ITStruct *ip;
    Vec3f vel;

    vel.x = __cosf(ITHITOKAGE_FLAME_SPAWN_ANGLE) * -ITHITOKAGE_FLAME_VEL_BASE;
    vel.y = __sinf(ITHITOKAGE_FLAME_SPAWN_ANGLE) * ITHITOKAGE_FLAME_VEL_BASE;
    vel.z = 0.0F;

    itHitokageWeaponFlameMakeWeapon(item_gobj, pos, &vel);

    func_800269C0_275C0(nSYAudioFGMLizardonFlame);
}

#endif /* NDS_P2_ITEM_CORE */
