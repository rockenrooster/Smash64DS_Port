/* P2 Saffron City Chansey (kind nITKindGLucky). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itground/itglucky.c:12-232.
 *
 * Art/state live in the stage's own map file: the descriptor's file base is
 * gGRCommonStruct.yamabuki.item_head (port include/gr/ground.h:431-434) and
 * the reloc token below is decomp/BattleShip-main/include/reloc_data.us.h
 * :4000 (:0xBC attributes); the port's generated reloc header does not
 * publish Yamabuki tokens, so this TU owns its uintptr_t token the same way
 * battleship_item_gbumper.c owns GBumper's (local token, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal
 * (ITGLUCKY_* rides on <it/item.h>). ITEM_TOGGLE_MASK_KIND is decomp
 * it/itdef.h:73, transcribed verbatim under guard because no port header
 * publishes it. Symbols the port headers do not publish yet
 * (nSYAudioVoiceYamabukiLucky, decomp gmsound.h:651;
 * grYamabukiGateClearMonsterGObj, defined by the Yamabuki stage TU's decomp
 * include) are referenced verbatim and listed in the task report -- no values
 * invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <common.h>
#include <gr/ground.h>
#include <ef/effect.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:4000. */
uintptr_t llGRYamabukiMapGLuckyItemAttributes = 0xBCu;

/* decomp/BattleShip-main/decomp/src/it/itdef.h:73. No port header publishes
 * it; guarded so a later port declaration wins without a redefinition. */
#ifndef ITEM_TOGGLE_MASK_KIND
#define ITEM_TOGGLE_MASK_KIND(kind) (1 << (kind))
#endif

extern void grYamabukiGateClearMonsterGObj(void);
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                f32 f_index);

/* decomp sys/utils.h:19-:20. Same seam as
 * battleship_item_box.c:85-86. */
extern f32 syUtilsRandFloat(void);

/* decomp itglucky.h:8-15 verbatim. The port publishes no per-kind item procs,
 * so the source header's declarations travel with this TU, exactly as the
 * Tomato and Star files carry theirs. */
extern void itGLuckyDamagedSetStatus(GObj *item_gobj);
extern void itGLuckyCommonUpdateEggSpawn(GObj *lucky_gobj);
extern sb32 itGLuckyCommonProcUpdate(GObj *item_gobj);
extern sb32 itGLuckyCommonProcHit(GObj *item_gobj);
extern sb32 itGLuckyDamagedProcUpdate(GObj *item_gobj);
extern sb32 itGLuckyDamagedProcDead(GObj *item_gobj);
extern sb32 itGLuckyCommonProcDamage(GObj *item_gobj);
extern GObj* itGLuckyMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// decomp itglucky.c:12-34 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token the
// same way).
ITDesc dITGLuckyItemDesc =
{
    nITKindGLucky,                          // Item Kind
    &gGRCommonStruct.yamabuki.item_head,    // Pointer to item file data?
    &llGRYamabukiMapGLuckyItemAttributes,   // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itGLuckyCommonProcUpdate,               // Proc Update
    NULL,                                   // Proc Map
    itGLuckyCommonProcHit,                  // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    itGLuckyCommonProcDamage                // Proc Damage
};

// decomp itglucky.c:36-49 verbatim.
ITStatusDesc dITGLuckyStatusDescs[/* */] =
{
    // Status 0 (Neutral Damage)
    {
        itGLuckyDamagedProcUpdate,          // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// decomp itglucky.c:57-61 verbatim.
enum itGLuckyStatus
{
    nITGLuckyStatusDamaged,
    nITGLuckyStatusEnumCount
};

// 0x8017C240
// decomp itglucky.c:70-75 verbatim.
void itGLuckyDamagedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITGLuckyStatusDescs, nITGLuckyStatusDamaged);

    itGetStruct(item_gobj)->proc_dead = itGLuckyDamagedProcDead;
}

// 0x8017C280
// decomp itglucky.c:78-131 verbatim.
void itGLuckyCommonUpdateEggSpawn(GObj *lucky_gobj)
{
    ITStruct *lucky_ip = itGetStruct(lucky_gobj);
    ITStruct *egg_ip;
    s32 unused;
    DObj *dobj = DObjGetStruct(lucky_gobj);
    GObj *egg_gobj;
    Vec3f pos;
    Vec3f vel;

    if (lucky_ip->multi == 0)
    {
        if (lucky_ip->item_vars.glucky.egg_spawn_count != 0)
        {
            if ((gSCManagerBattleState->item_toggles & ITEM_TOGGLE_MASK_KIND(nITKindEgg)) && (gSCManagerBattleState->item_appearance_rate != nSCBattleItemSwitchNone)) // Return to this when 0x8 is mapped
            {
                pos = dobj->translate.vec.f;

                pos.x -= ITGLUCKY_EGG_SPAWN_OFF_X;
                pos.y += ITGLUCKY_EGG_SPAWN_OFF_Y;

                vel.x = -((syUtilsRandFloat() * ITGLUCKY_EGG_SPAWN_MUL) + ITGLUCKY_EGG_SPAWN_ADD_X);
                vel.y = (syUtilsRandFloat() * ITGLUCKY_EGG_SPAWN_MUL) + ITGLUCKY_EGG_SPAWN_ADD_Y;
                vel.z = 0.0F;

                egg_gobj = itManagerMakeItemSetupCommon(lucky_gobj, nITKindEgg, &pos, &vel, (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM));

                if (egg_gobj != NULL)
                {
                    egg_ip = itGetStruct(egg_gobj);

                    func_800269C0_275C0(nSYAudioFGMKirbySpecialLwStart); // Bruh lol

                    lucky_ip->multi = 10;
                    lucky_ip->item_vars.glucky.egg_spawn_count--;

                    efManagerDustLightMakeEffect(&pos, egg_ip->lr, 1.0F);
                }
            }
            else
            {
                lucky_ip->multi = 10;
                lucky_ip->item_vars.glucky.egg_spawn_count--;
            }
        }
    }
    if (lucky_ip->item_vars.glucky.egg_spawn_count != 0)
    {
        if (lucky_ip->multi > 0)
        {
            lucky_ip->multi--;
        }
    }
}

// 0x8017C400
// decomp itglucky.c:134-153 verbatim.
sb32 itGLuckyCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->translate.vec.f.x += ip->item_vars.glucky.pos.x;
    dobj->translate.vec.f.y += ip->item_vars.glucky.pos.y;

    if ((dobj->anim_frame >= ITGLUCKY_EGG_SPAWN_BEGIN) && (dobj->anim_frame <= ITGLUCKY_EGG_SPAWN_END))
    {
        itGLuckyCommonUpdateEggSpawn(item_gobj);
    }
    if (dobj->anim_wait == AOBJ_ANIM_NULL)
    {
        grYamabukiGateSetClosedWait();

        return TRUE;
    }
    else return FALSE;
}

// 0x8017C4AC
// decomp itglucky.c:156-163 verbatim.
sb32 itGLuckyCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    return FALSE;
}

// 0x8017C4BC
// decomp itglucky.c:166-178 verbatim.
sb32 itGLuckyDamagedProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj;

    itMainApplyGravityClampTVel(ip, ITGLUCKY_GRAVITY, ITGLUCKY_TVEL);

    dobj = DObjGetStruct(item_gobj);

    dobj->rotate.vec.f.z -= ITGLUCKY_HIT_ROTATE_Z * ip->lr;

    return FALSE;
}

// 0x8017C524
// decomp itglucky.c:181-184 verbatim.
sb32 itGLuckyDamagedProcDead(GObj *item_gobj)
{
    return TRUE;
}

// 0x8017C530
// decomp itglucky.c:187-208 verbatim.
sb32 itGLuckyCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->damage_knockback >= ITGLUCKY_NDAMAGE_KNOCKBACK_MIN)
    {
        f32 angle = ftCommonDamageGetKnockbackAngle(ip->damage_angle, ip->ga, ip->damage_knockback);

        ip->physics.vel_air.x = (__cosf(angle) * ip->damage_knockback * -ip->damage_lr);
        ip->physics.vel_air.y = (__sinf(angle) * ip->damage_knockback);

        ip->attack_coll.attack_state = nGMAttackStateOff;
        ip->damage_coll.hitstatus = nGMHitStatusNone;

        dobj->anim_wait = AOBJ_ANIM_NULL;

        grYamabukiGateClearMonsterGObj();
        itGLuckyDamagedSetStatus(item_gobj);
    }
    return FALSE;
}

// 0x8017C5F4
// decomp itglucky.c:211-232 verbatim.
GObj* itGLuckyMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITGLuckyItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER;

        ip->item_vars.glucky.pos = *pos;

        ip->is_allow_knockback = TRUE;

        ip->multi = 0;

        ip->item_vars.glucky.egg_spawn_count = ITGLUCKY_EGG_SPAWN_COUNT;

        func_800269C0_275C0(nSYAudioVoiceYamabukiLucky);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
