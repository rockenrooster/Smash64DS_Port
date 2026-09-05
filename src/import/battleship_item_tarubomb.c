/* P2-6 Race to the Finish barrel bomb (kind nITKindTaruBomb). Verbatim-adapted
 * from decomp/BattleShip-main/decomp/src/it/itground/ittarubomb.c:13-412,
 * the way battleship_item_taru.c carries the barrel.
 *
 * grbonus3.c:53-55 spawns one every 180 ticks from the Bonus3 map's
 * TaruBomb map object; the port's Bonus3 ground (battleship_grbonus3.c)
 * publishes gGRCommonStruct.bonus3.item_head, which is this descriptor's
 * file base. The four reloc tokens are file-relative offsets into
 * GRBonus3Map (decomp/BattleShip-main/tools/reloc_data_symbols.us.txt:
 * 4329-4333); no port header publishes them, so this TU owns them the way
 * battleship_item_taru.c owns the barrel's pair.
 *
 * Until 2026-09-05 no provider existed and nITKindTaruBomb had no maker-table
 * row (battleship_item_link_core.c), so the race spawned nothing.
 *
 * Gated on NDS_P2_1P_GAME (the Makefile lists it beside the bonus Target):
 * the descriptor reads a ground struct only the Bonus3 map populates.
 * Every numeric constant below is the decomp source's own macro or literal
 * (ITTARUBOMB_* in include/it/item.h).
 */
#if NDS_P2_1P_GAME

#include <it/item.h>
#include <gr/ground.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <if/interface.h>
#include <gm/gmsound.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>
#include <math.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* reloc_data_symbols.us.txt:4329-4333, GRBonus3Map file-relative. */
uintptr_t llGRBonus3MapTaruBombItemAttributes = 0xA8u;
uintptr_t llGRBonus3MapTaruBombAttackEvents = 0xF0u;
uintptr_t llGRBonus3MapTaruBombDataStart = 0x788u;
uintptr_t llGRBonus3MapTaruBombEffectDisplayList = 0x8A0u;

/* decomp ittarubomb.h:8-24: the port publishes no per-kind item procs, so
 * the source header's declarations travel with this TU. */
void itTaruBombContainerSmashUpdateEffect(GObj *effect_gobj);
void itTaruBombContainerSmashMakeEffect(Vec3f *pos);
sb32 itTaruBombFallProcUpdate(GObj *item_gobj);
sb32 itTaruBombCommonProcHit(GObj *item_gobj);
sb32 itTaruBombCommonProcDamage(GObj *item_gobj);
void itTaruBombRollSetStatus(GObj *item_gobj);
sb32 itTaruBombFallCheckCollideGround(GObj *item_gobj, f32 common_rebound);
sb32 itTaruBombFallProcMap(GObj *item_gobj);
void itTaruBombCommonSetMapCollisionBox(GObj *item_gobj);
sb32 itTaruBombExplodeProcUpdate(GObj *item_gobj);
sb32 itTaruBombRollProcUpdate(GObj *item_gobj);
sb32 itTaruBombRollProcMap(GObj *item_gobj);
GObj *itTaruBombMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
void itTaruBombExplodeInitVars(GObj *item_gobj);
void itTaruBombExplodeSetStatus(GObj *item_gobj);
void itTaruBombExplodeMakeEffectGotoSetStatus(GObj *item_gobj);

/* Same local-extern shape as battleship_item_box.c:83-84 and
 * battleship_item_taru.c:81-87 for the helpers no port header publishes. */
extern EFStruct *efManagerGetEffectNoForce(void);
extern void efManagerSetPrevStructAlloc(EFStruct *ep);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 id);
extern Vec3f *lbCommonReflect2D(Vec3f *dst, Vec3f *p);
extern f32 syUtilsRandFloat(void);
extern f32 syUtilsArcTan2(f32 y, f32 x);
extern alSoundEffect *func_800269C0_275C0(u16);

/* decomp ittarubomb.c:22-44 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here). */
ITDesc dITTaruBombItemDesc =
{
    nITKindTaruBomb,                            // Item Kind
    &gGRCommonStruct.bonus3.item_head,          // Pointer to item file data?
    &llGRBonus3MapTaruBombItemAttributes,       // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,                 // Main matrix transformations
        nGCMatrixKindNull,                       // Secondary matrix transformations?
        0                                        // ???
    },

    nGMAttackStateNew,                          // Hitbox Update State
    itTaruBombFallProcUpdate,                   // Proc Update
    itTaruBombFallProcMap,                      // Proc Map
    itTaruBombCommonProcHit,                    // Proc Hit
    itTaruBombCommonProcHit,                    // Proc Shield
    NULL,                                       // Proc Hop
    itTaruBombCommonProcHit,                    // Proc Set-Off
    itTaruBombCommonProcHit,                    // Proc Reflector
    itTaruBombCommonProcDamage                  // Proc Damage
};

// decomp ittarubomb.c:46-84 verbatim.
ITStatusDesc dITTaruBombStatusDescs[/* */] =
{
    // Status 0 (Air Wait Fall)
    {
        itTaruBombFallProcUpdate,               // Proc Update
        itTaruBombFallProcMap,                  // Proc Map
        itTaruBombCommonProcHit,                // Proc Hit
        itTaruBombCommonProcHit,                // Proc Shield
        NULL,                                   // Proc Hop
        itTaruBombCommonProcHit,                // Proc Set-Off
        itTaruBombCommonProcHit,                // Proc Reflector
        itTaruBombCommonProcDamage              // Proc Damage
    },

    // Status 1 (Neutral Explosion)
    {
        itTaruBombExplodeProcUpdate,            // Proc Update
        NULL,                                   // Proc Map
        NULL,                                   // Proc Hit
        NULL,                                   // Proc Shield
        NULL,                                   // Proc Hop
        NULL,                                   // Proc Set-Off
        NULL,                                   // Proc Reflector
        NULL                                    // Proc Damage
    },

    // Status 2 (Ground Roll)
    {
        itTaruBombRollProcUpdate,               // Proc Update
        itTaruBombRollProcMap,                  // Proc Map
        itTaruBombCommonProcHit,                // Proc Hit
        itTaruBombCommonProcHit,                // Proc Shield
        NULL,                                   // Proc Hop
        itTaruBombCommonProcHit,                // Proc Set-Off
        itTaruBombCommonProcHit,                // Proc Reflector
        itTaruBombCommonProcDamage              // Proc Damage
    }
};

enum itTaruBombStatus
{
    nITTaruBombStatusFall,
    nITTaruBombStatusExplode,
    nITTaruBombStatusRoll,
    nITTaruBombStatusEnumCount
};

// 0x80184A70
// decomp ittarubomb.c:106-133 verbatim.
void itTaruBombContainerSmashUpdateEffect(GObj *effect_gobj) // RTTF bomb explode GFX process
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);

    ep->effect_vars.container.lifetime--;

    if (ep->effect_vars.container.lifetime == 0)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else while (dobj != NULL)
    {
        dobj->scale.vec.f.y -= 1.3F;

        dobj->translate.vec.f.x += dobj->scale.vec.f.x; // This makes no sense, seems this custom effect is very... custom
        dobj->translate.vec.f.y += dobj->scale.vec.f.y;
        dobj->translate.vec.f.z += dobj->scale.vec.f.z;

        dobj->rotate.vec.f.x += dobj->anim_wait; // ??? Seems to be rotation step, but only in this case? Otherwise -FLOAT32_MAX?
        dobj->rotate.vec.f.y += dobj->anim_speed;
        dobj->rotate.vec.f.z += dobj->anim_frame;

        dobj = dobj->sib_next;
    }
}

// 0x80184B44
// decomp ittarubomb.c:135-177 verbatim. The display-list address math is the
// battleship_item_box.c:262 shape: o_attributes is the address of the token,
// so one dereference recovers the source's file offset; the DataStart /
// EffectDisplayList terms keep the source's (intptr_t)&token shape exactly.
void itTaruBombContainerSmashMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    EFStruct *ep = efManagerGetEffectNoForce();
    DObj *dobj;
    s32 i;
    Gfx *dl;

    if (ep != NULL)
    {
        effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

        if (effect_gobj != NULL)
        {
            gcAddGObjDisplay(effect_gobj, gcDrawDObjTreeForGObj, 11, GOBJ_PRIORITY_DEFAULT, ~0);

            dl = (Gfx*) ((*(uintptr_t*) ((uintptr_t)*dITTaruBombItemDesc.p_file + *(uintptr_t*)dITTaruBombItemDesc.o_attributes) - (intptr_t)&llGRBonus3MapTaruBombDataStart) + (intptr_t)&llGRBonus3MapTaruBombEffectDisplayList);

            for (i = 0; i < ITTARUBOMB_EFFECT_COUNT; i++)
            {
                dobj = gcAddDObjForGObj(effect_gobj, dl);

                gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);

                dobj->translate.vec.f = *pos;

                dobj->scale.vec.f.x = (syUtilsRandFloat() * 48.0F) + -24.0F;
                dobj->scale.vec.f.y = (syUtilsRandFloat() * 50.0F) + 10.0F;
                dobj->scale.vec.f.z = (syUtilsRandFloat() * 32.0F) + -16.0F;

                dobj->anim_wait = F_CLC_DTOR32((syUtilsRandFloat() * 100.0F) + -50.0F);
                dobj->anim_speed = F_CLC_DTOR32((syUtilsRandFloat() * 100.0F) + -50.0F);
                dobj->anim_frame = F_CLC_DTOR32((syUtilsRandFloat() * 100.0F) + -50.0F);
            }
            ep->effect_vars.container.lifetime = ITTARUBOMB_GFX_LIFETIME;

            effect_gobj->user_data.p = ep;

            gcAddGObjProcess(effect_gobj, itTaruBombContainerSmashUpdateEffect, 1, 3);
        }
    }
}

// 0x80184D74
// decomp ittarubomb.c:179-190 verbatim.
sb32 itTaruBombFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj;

    itMainApplyGravityClampTVel(ip, ITTARUBOMB_GRAVITY, ITTARUBOMB_TVEL);

    dobj = DObjGetStruct(item_gobj);
    dobj->rotate.vec.f.z += ip->item_vars.tarubomb.roll_rotate_step;

    return FALSE;
}

// 0x80184DC4
// decomp ittarubomb.c:193-200 verbatim.
sb32 itTaruBombCommonProcHit(GObj *item_gobj)
{
    func_800269C0_275C0(nSYAudioFGMTaruBombHit);
    itTaruBombContainerSmashMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);
    itTaruBombExplodeMakeEffectGotoSetStatus(item_gobj);

    return FALSE;
}

// 0x80184E04
// decomp ittarubomb.c:203-212 verbatim.
sb32 itTaruBombCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->percent_damage >= ITTARUBOMB_HEALTH_MAX)
    {
        return itTaruBombCommonProcHit(item_gobj);
    }
    return FALSE;
}

// 0x80184E44
// decomp ittarubomb.c:215-222 verbatim.
void itTaruBombRollSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.y = 0.0F;

    itMainSetStatus(item_gobj, dITTaruBombStatusDescs, nITTaruBombStatusRoll);
}

// 0x80184E78
// decomp ittarubomb.c:225-240 verbatim.
sb32 itTaruBombFallCheckCollideGround(GObj *item_gobj, f32 common_rebound)
{
    sb32 is_collide_floor = itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR);

    if (itMapCheckCollideAllRebound(item_gobj, (MAP_FLAG_CEIL | MAP_FLAG_RWALL | MAP_FLAG_LWALL), common_rebound, NULL) != FALSE)
    {
        itMainSetSpinVelLR(item_gobj);
    }
    if (is_collide_floor != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x80184EDC
// decomp ittarubomb.c:243-272 verbatim.
sb32 itTaruBombFallProcMap(GObj *item_gobj)
{
    if (itTaruBombFallCheckCollideGround(item_gobj, ITTARUBOMB_MAP_REBOUND_COMMON) != FALSE)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        if (ip->physics.vel_air.y >= 90.0F) // Is it even possible to meet this condition? Didn't they mean <= inverse of this value?
        {
            itTaruBombCommonProcHit(item_gobj); // This causes the bomb to smash on impact when landing from too high; doesn't seem possible to trigger

            return TRUE;
        }
        else if (ip->physics.vel_air.y < 30.0F)
        {
            itTaruBombRollSetStatus(item_gobj);
        }
        else
        {
            lbCommonReflect2D(&ip->physics.vel_air, &ip->coll_data.floor_angle);

            ip->physics.vel_air.y *= 0.2F;

            itMainSetSpinVelLR(item_gobj);
        }
        func_800269C0_275C0(nSYAudioFGMTaruBombMap);
        itMainClearOwnerStats(item_gobj);
    }
    return FALSE;
}

// 0x80184FAC
// decomp ittarubomb.c:275-283 verbatim.
void itTaruBombCommonSetMapCollisionBox(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    DObjGetStruct(item_gobj)->rotate.vec.f.x = F_CLC_DTOR32(90.0F);

    ip->coll_data.map_coll.top = ip->coll_data.map_coll.width;
    ip->coll_data.map_coll.bottom = -ip->coll_data.map_coll.width;
}

// 0x80184FD4
// decomp ittarubomb.c:286-299 verbatim.
sb32 itTaruBombExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi++;

    if (ip->multi == ITTARUBOMB_EXPLODE_LIFETIME)
    {
        return TRUE;
    }
    else itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITTaruBombItemDesc, &llGRBonus3MapTaruBombAttackEvents));

    return FALSE;
}

// 0x80185030
// decomp ittarubomb.c:302-321 verbatim.
sb32 itTaruBombRollProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 roll_rotate_step;
    f32 sqrt_vel;

    ip->physics.vel_air.x += (-(syUtilsArcTan2(ip->coll_data.floor_angle.y, ip->coll_data.floor_angle.x) - F_CLC_DTOR32(90.0F) /*HALF_PI32*/) * ITTARUBOMB_MUL_VEL_X);

    ip->lr = (ip->physics.vel_air.x >= 0.0F) ? +1 : -1;

    sqrt_vel = sqrtf(SQUARE(ip->physics.vel_air.x) + SQUARE(ip->physics.vel_air.y));

    roll_rotate_step = ((ip->lr == -1) ? ITTARUBOMB_ROLL_ROTATE_MUL : -ITTARUBOMB_ROLL_ROTATE_MUL) * sqrt_vel;

    ip->item_vars.tarubomb.roll_rotate_step = roll_rotate_step;

    DObjGetStruct(item_gobj)->rotate.vec.f.z += roll_rotate_step;

    return FALSE;
}

// 0x8018511C
// decomp ittarubomb.c:324-337 verbatim.
sb32 itTaruBombRollProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestLRWallCheckFloor(item_gobj) == FALSE)
    {
        itMainSetStatus(item_gobj, dITTaruBombStatusDescs, nITTaruBombStatusFall);
    }
    else if (ip->coll_data.mask_curr & (MAP_FLAG_RWALL | MAP_FLAG_LWALL))
    {
        return itTaruBombCommonProcHit(item_gobj);
    }
    return FALSE;
}

// 0x8018518C
// decomp ittarubomb.c:340-353 verbatim.
GObj* itTaruBombMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITTaruBombItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->item_vars.tarubomb.roll_rotate_step = 0.0F;

        itTaruBombCommonSetMapCollisionBox(item_gobj);
    }
    return item_gobj;
}

// 0x801851F4
// decomp ittarubomb.c:356-377 verbatim.
void itTaruBombExplodeInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = 0;
    ip->event_id = 0;

    ip->attack_coll.fgm_id = nSYAudioFGMExplodeL;

    ip->attack_coll.can_rehit_item = TRUE;
    ip->attack_coll.can_reflect = FALSE;

    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;
    ip->attack_coll.element = nGMHitElementFire;

    ip->attack_coll.can_setoff = FALSE;

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    itMainRefreshAttackColl(item_gobj);
    itMainUpdateAttackEvent(item_gobj, itGetAttackEvent(dITTaruBombItemDesc, &llGRBonus3MapTaruBombAttackEvents));
}

// 0x80185284
// decomp ittarubomb.c:380-384 verbatim.
void itTaruBombExplodeSetStatus(GObj *item_gobj)
{
    itTaruBombExplodeInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITTaruBombStatusDescs, nITTaruBombStatusExplode);
}

// 0x801852B8
// decomp ittarubomb.c:387-412 verbatim.
void itTaruBombExplodeMakeEffectGotoSetStatus(GObj *item_gobj)
{
    LBParticle *pc;
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;

    pc = efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);

    if (pc != NULL)
    {
        pc->xf->scale.x =
        pc->xf->scale.y =
        pc->xf->scale.z = ITTARUBOMB_EXPLODE_EFFECT_SCALE;
    }
    efManagerQuakeMakeEffect(1);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;

    itTaruBombExplodeSetStatus(item_gobj);
}

#endif /* NDS_P2_1P_GAME */
