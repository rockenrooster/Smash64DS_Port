/* P2 Saffron City Venusaur (kind nITKindFushigibana). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itground/itfushigibana.c:22-289.
 *
 * Art/state live in the stage's own map file: the descriptor's file base is
 * gGRCommonStruct.yamabuki.item_head (port include/gr/ground.h:431-434) and
 * the reloc tokens below are decomp/BattleShip-main/include/reloc_data.us.h
 * :4007 (:0x278 attributes), :4008 (:0x2C0 hit parties) and :4009 (:0x308
 * razor weapon attributes); the port's generated reloc header does not
 * publish Yamabuki tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file). The WPDesc carries them with
 * the same &token shape as battleship_item_dogas.c:135-157.
 *
 * The monster hitbox script (ITMonsterEvent / itGetMonsterEvent) has no port
 * equivalent -- <it/item.h> only carries the attack-event helper -- so the
 * decomp definitions (ittypes.h:120-132, item.h:51) are transcribed verbatim
 * below, cited per line, the same way this TU carries the source header's
 * per-kind proc prototypes. They are TU-local (battleship_item_porygon.c
 * carries its own identical copy; separate TUs never share a symbol); nothing
 * under include/ is touched. Symbols the port headers do not publish yet
 * (nSYAudioVoiceYamabukiFushigibana, decomp gmsound.h:649;
 * dGRYamabukiMonsterAttackKind, defined by the Yamabuki stage TU's decomp
 * include :14) are referenced verbatim through local externs and listed in
 * the task report -- no values invented here.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal
 * (ITFUSHIGIBANA_* rides on <it/item.h>; the 0x03 razor render flags and the
 * event-id clamp at 2 back to 1 are the source's own).
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <wp/weapon.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <ef/effect.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:4007. */
uintptr_t llGRYamabukiMapFushigibanaItemAttributes = 0x278u;
/* decomp/BattleShip-main/include/reloc_data.us.h:4008. */
uintptr_t llGRYamabukiMapFushigibanaHitParties = 0x2C0u;
/* decomp/BattleShip-main/include/reloc_data.us.h:4009. */
uintptr_t llGRYamabukiMapFushigibanaRazorWeaponAttributes = 0x308u;

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
 * battleship_item_hitokage.c read and write it through extern, exactly as
 * the two decomp sources do (itfushigibana.c:14, ithitokage.c:7). */
extern s32 dGRYamabukiMonsterAttackKind;

/* decomp sys/utils.h:19-:20. Same seam as
 * battleship_item_box.c:85-86. */
extern s32 syUtilsRandIntRange(s32 range);

/* No port header publishes them; battleship_item_lgun.c:79 and
 * battleship_item_taru.c:85 carry the same local externs. */
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
extern f32 syUtilsArcTan2(f32 y, f32 x);

extern GObj *efManagerDamageSlashMakeEffect(Vec3f *pos, s32 size, f32 rotate);
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                f32 f_index);
/* efManagerDustCollideMakeEffect rides on <ef/effect.h> via decomp
 * ef/efmanager.h:35 (LBParticle *); no local extern needed. */

/* decomp/BattleShip-main/decomp/src/it/ittypes.h:120-132 verbatim. Full-scale
 * hitbox subaction event; used by Venusaur and Porygon. Carried here because
 * no port header publishes it (see file header). */
typedef struct ITMonsterEvent
{
    u8 timer;
    s32 angle : 10;
    u32 damage : 8;
    u16 size;
    u32 knockback_scale;
    u32 knockback_weight;
    u32 knockback_base;
    s32 element;
    ub32 can_setoff : 1;
    s32 shield_damage;
    u16 fgm_id;
} ITMonsterEvent;

/* decomp/BattleShip-main/decomp/src/it/item.h:51 verbatim. Guarded so a later
 * port declaration wins without a redefinition. */
#ifndef itGetMonsterEvent
#define itGetMonsterEvent(it_desc, off) ((ITMonsterEvent*)((uintptr_t) * (it_desc).p_file + (intptr_t) (off)))
#endif

/* decomp itfushigibana.h:8-15 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly as
 * the Tomato and Star files carry theirs. */
extern void itFushigibanaCommonUpdateMonsterEvent(GObj *item_gobj);
extern sb32 itFushigibanaCommonProcUpdate(GObj *item_gobj);
extern GObj* itFushigibanaMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
extern sb32 itFushigibanaWeaponRazorProcUpdate(GObj *weapon_gobj);
extern sb32 itFushigibanaWeaponRazorProcHit(GObj *weapon_gobj);
extern sb32 itFushigibanaWeaponRazorProcHop(GObj *weapon_gobj);
extern sb32 itFushigibanaWeaponRazorProcReflector(GObj *weapon_gobj);
extern GObj *itFushigibanaWeaponRazorMakeWeapon(GObj *item_gobj, Vec3f *pos);

// decomp itfushigibana.c:22-44 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITFushigibanaItemDesc =
{
    nITKindFushigibana,                         // Item Kind
    &gGRCommonStruct.yamabuki.item_head,        // Pointer to item file data?
    &llGRYamabukiMapFushigibanaItemAttributes,  // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,                // Main matrix transformations
        nGCMatrixKindNull,                      // Secondary matrix transformations?
        0                                       // ???
    },

    nGMAttackStateNew,                          // Hitbox Update State
    itFushigibanaCommonProcUpdate,              // Proc Update
    NULL,                                       // Proc Map
    NULL,                                       // Proc Hit
    NULL,                                       // Proc Shield
    NULL,                                       // Proc Hop
    NULL,                                       // Proc Set-Off
    NULL,                                       // Proc Reflector
    NULL                                        // Proc Damage
};

// decomp itfushigibana.c:46-68 verbatim, adapted only for the port's WPDesc
// shape (o_attributes is intptr_t here; the &token shape is the same one
// battleship_item_dogas.c:135-157 uses).
WPDesc dITFushigibanaWeaponRazorWeaponDesc =
{
    0x03,                                       // Render flags?
    nWPKindFushigibanaRazor,                    // Weapon Kind
    &gGRCommonStruct.yamabuki.item_head,        // Pointer to item's loaded files?
    &llGRYamabukiMapFushigibanaRazorWeaponAttributes, // Offset of weapon attributes in loaded files

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,             // Main matrix transformations
        nGCMatrixKindNull,                      // Secondary matrix transformations?
        0                                       // ???
    },

    itFushigibanaWeaponRazorProcUpdate,         // Proc Update
    NULL,                                       // Proc Map
    itFushigibanaWeaponRazorProcHit,            // Proc Hit
    itFushigibanaWeaponRazorProcHit,            // Proc Shield
    itFushigibanaWeaponRazorProcHop,            // Proc Hop
    itFushigibanaWeaponRazorProcHit,            // Proc Set-Off
    itFushigibanaWeaponRazorProcReflector,      // Proc Reflector
    itFushigibanaWeaponRazorProcHit             // Proc Absorb
};

// 0x80184440
// decomp itfushigibana.c:77-112 verbatim.
void itFushigibanaCommonUpdateMonsterEvent(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITMonsterEvent *ev = itGetMonsterEvent(dITFushigibanaItemDesc, &llGRYamabukiMapFushigibanaHitParties); // (ITMonsterEvent*) ((uintptr_t)*dITFushigibanaItemDesc.p_file + (intptr_t)&Fushigibana_Event);

    if (ip->multi == ev[ip->event_id].timer)
    {
        ip->attack_coll.angle            = ev[ip->event_id].angle;
        ip->attack_coll.damage           = ev[ip->event_id].damage;
        ip->attack_coll.size             = ev[ip->event_id].size;
        ip->attack_coll.knockback_scale  = ev[ip->event_id].knockback_scale;
        ip->attack_coll.knockback_weight = ev[ip->event_id].knockback_weight;
        ip->attack_coll.knockback_base   = ev[ip->event_id].knockback_base;
        ip->attack_coll.element          = ev[ip->event_id].element;
        ip->attack_coll.can_setoff       = ev[ip->event_id].can_setoff;
        ip->attack_coll.shield_damage    = ev[ip->event_id].shield_damage;
        ip->attack_coll.fgm_id          = ev[ip->event_id].fgm_id;

        ip->event_id++;

        if (ip->event_id == 2)
        {
            ip->event_id = 1;
        }
    }
    ip->multi++;

    if (ip->multi == ITFUSHIGIBANA_RETURN_WAIT)
    {
        Vec3f pos = DObjGetStruct(item_gobj)->translate.vec.f;

        pos.y = 0.0F;

        efManagerDustLightMakeEffect(&pos, -1, 1.0F);
    }
}

// 0x801845B4
// decomp itfushigibana.c:115-163 verbatim.
sb32 itFushigibanaCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f pos;

    dobj->translate.vec.f.x += ip->item_vars.fushigibana.offset.x;
    dobj->translate.vec.f.y += ip->item_vars.fushigibana.offset.y;

    itFushigibanaCommonUpdateMonsterEvent(item_gobj);

    pos = dobj->translate.vec.f;

    pos.x += ITFUSHIGIBANA_RAZOR_SPAWN_OFF_X;

    if
    (
        (ip->item_vars.fushigibana.flags == GRYAMABUKI_MONSTER_WEAPON_INSTANT)                                                     ||
        ((ip->item_vars.fushigibana.flags & GRYAMABUKI_MONSTER_WEAPON_WAIT) && (dobj->anim_frame >= ITFUSHIGIBANA_RAZOR_SPAWN_BEGIN)) &&
        (dobj->anim_frame <= ITFUSHIGIBANA_RAZOR_SPAWN_END)
    )
    {
        dobj->mobj->texture_id_curr = 1;

        if (!ip->item_vars.fushigibana.razor_spawn_wait)
        {
            itFushigibanaWeaponRazorMakeWeapon(item_gobj, &pos);

            ip->item_vars.fushigibana.razor_spawn_wait = ITFUSHIGIBANA_RAZOR_SPAWN_WAIT;

            func_800269C0_275C0(nSYAudioFGMMonsterShoot);

            efManagerDustCollideMakeEffect(&pos);
        }
        if (ip->item_vars.fushigibana.razor_spawn_wait > 0)
        {
            ip->item_vars.fushigibana.razor_spawn_wait--;
        }
    }
    else dobj->mobj->texture_id_curr = 0;

    if (dobj->anim_wait == AOBJ_ANIM_NULL)
    {
        grYamabukiGateSetClosedWait();

        return TRUE;
    }
    return FALSE;
}

// 0x8018470C
// decomp itfushigibana.c:166-204 verbatim.
GObj* itFushigibanaMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITFushigibanaItemDesc, pos, vel, flags);
    s32 unused;
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        ip = itGetStruct(item_gobj);
        dobj = DObjGetStruct(item_gobj);

        ip->event_id = 0;

        ip->multi = 0;

        ip->item_vars.fushigibana.razor_spawn_wait = 0;
        ip->item_vars.fushigibana.offset = *pos;

        ip->is_allow_knockback = TRUE;

        ip->item_vars.fushigibana.flags = syUtilsRandIntRange(GRYAMABUKI_MONSTER_WEAPON_MAX);

        if ((dGRYamabukiMonsterAttackKind == ip->item_vars.fushigibana.flags) || (ip->item_vars.fushigibana.flags & dGRYamabukiMonsterAttackKind))
        {
            ip->item_vars.fushigibana.flags++;

            ip->item_vars.fushigibana.flags %= GRYAMABUKI_MONSTER_WEAPON_MAX;
        }
        if (ip->item_vars.fushigibana.flags == GRYAMABUKI_MONSTER_WEAPON_INSTANT)
        {
            dobj->mobj->texture_id_curr = 1;
        }
        dGRYamabukiMonsterAttackKind = ip->item_vars.fushigibana.flags;

        func_800269C0_275C0(nSYAudioVoiceYamabukiFushigibana);
    }
    return item_gobj;
}

// 0x80184820
// decomp itfushigibana.c:207-218 verbatim.
sb32 itFushigibanaWeaponRazorProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air.x += ITFUSHIGIBANA_RAZOR_ADD_VEL_X * wp->lr;

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x80184874
// decomp itfushigibana.c:221-228 verbatim.
sb32 itFushigibanaWeaponRazorProcHit(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    efManagerDamageSlashMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, wp->attack_coll.damage, wp->lr);

    return TRUE;
}

// 0x801848BC
// decomp itfushigibana.c:232-247 verbatim.
sb32 itFushigibanaWeaponRazorProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir, wp->shield_collide_angle * 2);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x) + F_CLC_DTOR32(180.0F);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    if (wp->physics.vel_air.x > 0.0F)
    {
        wp->lr = +1;
    }
    else wp->lr = -1;

    return FALSE;
}

// 0x80184970
// decomp itfushigibana.c:251-263 verbatim.
sb32 itFushigibanaWeaponRazorProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);

    wpMainReflectorSetLR(wp, fp);

    DObjGetStruct(weapon_gobj)->rotate.vec.f.z = syUtilsArcTan2(wp->physics.vel_air.y, wp->physics.vel_air.x) + F_CLC_DTOR32(180.0F);
    DObjGetStruct(weapon_gobj)->scale.vec.f.x = 1.0F;

    wp->lr = -wp->lr;

    return FALSE;
}

// 0x801849EC
// decomp itfushigibana.c:266-289 verbatim.
GObj* itFushigibanaWeaponRazorMakeWeapon(GObj *item_gobj, Vec3f *pos)
{
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, &dITFushigibanaWeaponRazorWeaponDesc, pos, WEAPON_FLAG_PARENT_ITEM);
    DObj *dobj;
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->lr = -1;

    wp->physics.vel_air.x = ITFUSHIGIBANA_RAZOR_VEL_X;

    dobj = DObjGetStruct(weapon_gobj);

    dobj->translate.vec.f = *pos;

    wp->lifetime = ITFUSHIGIBANA_RAZOR_LIFETIME;

    return weapon_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
