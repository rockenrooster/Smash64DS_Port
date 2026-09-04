/* P2 Saffron City Voltorb (kind nITKindMarumine). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itground/itmarumine.c:11-202.
 *
 * Art/state live in the stage's own map file: the descriptor's file base is
 * gGRCommonStruct.yamabuki.item_head (port include/gr/ground.h:431-434) and
 * the reloc tokens below are decomp/BattleShip-main/include/reloc_data.us.h
 * :4001 (:0x104 attributes) and :4002 (:0x14C attack events); the port's
 * generated reloc header does not publish Yamabuki tokens, so this TU owns
 * its uintptr_t tokens the same way battleship_item_gbumper.c owns GBumper's
 * (local tokens, no generator involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal
 * (ITMARUMINE_* rides on <it/item.h>; the 0x46 XObj kind and the event-id
 * clamp at 4 back to 3 are the source's own). The attack-event script reads
 * through the port's itGetAttackEvent exactly as battleship_item_box.c:513
 * does. Symbols the port headers do not publish yet
 * (nSYAudioVoiceYamabukiMarumine, decomp gmsound.h:652) are referenced
 * verbatim and listed in the task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
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

/* decomp/BattleShip-main/include/reloc_data.us.h:4001. */
uintptr_t llGRYamabukiMapMarumineItemAttributes = 0x104u;
/* decomp/BattleShip-main/include/reloc_data.us.h:4002. */
uintptr_t llGRYamabukiMapMarumineAttackEvents = 0x14Cu;

extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern GObj *efManagerQuakeMakeEffect(s32 id);

/* gcAddXObjForDObjFixed rides on <sys/objman.h>; no local extern needed. */

/* decomp itmarumine.h:8-13 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly as
 * the Tomato and Star files carry theirs. */
extern void itMarumineExplodeMakeEffectGotoSetStatus(GObj *item_gobj);
extern void itMarumineExplodeUpdateAttackEvent(GObj *item_gobj);
extern sb32 itMarumineCommonProcUpdate(GObj *item_gobj);
extern sb32 itMarumineExplodeProcUpdate(GObj *item_gobj);
extern void itMarumineExplodeSetStatus(GObj *item_gobj);
extern GObj* itMarumineMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// decomp itmarumine.c:11-33 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITMarumineItemDesc =
{
    nITKindMarumine,                        // Item Kind
    &gGRCommonStruct.yamabuki.item_head,    // Pointer to item file data?
    &llGRYamabukiMapMarumineItemAttributes,  // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTra,                   // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itMarumineCommonProcUpdate,             // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// decomp itmarumine.c:35-48 verbatim.
ITStatusDesc dITMarumineStatusDescs[/* */] =
{
    // Status 0 (Neutral Explosion)
    {
        itMarumineExplodeProcUpdate,        // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// decomp itmarumine.c:56-60 verbatim.
enum itMarumineStatus
{
    nITMarumineStatusExplode,
    nITMarumineStatusEnumCount
};

// 0x801837A0
// decomp itmarumine.c:69-94 verbatim.
void itMarumineExplodeMakeEffectGotoSetStatus(GObj *item_gobj)
{
    s32 unused;
    LBParticle *pc;
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNone;

    pc = efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);

    if (pc != NULL)
    {
        pc->xf->scale.x = ITMARUMINE_EXPLODE_EFFECT_SCALE;
        pc->xf->scale.y = ITMARUMINE_EXPLODE_EFFECT_SCALE;
        pc->xf->scale.z = ITMARUMINE_EXPLODE_EFFECT_SCALE;
    }
    efManagerQuakeMakeEffect(1);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;

    ip->attack_coll.fgm_id = nSYAudioFGMExplodeL;

    itMainRefreshAttackColl(item_gobj);
    itMarumineExplodeSetStatus(item_gobj);
}

// 0x80183830
// decomp itmarumine.c:97-122 verbatim.
void itMarumineExplodeUpdateAttackEvent(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttackEvent *ev = itGetAttackEvent(dITMarumineItemDesc, &llGRYamabukiMapMarumineAttackEvents); // (ITAttackEvent*) ((uintptr_t)*dITMarumineItemDesc.p_file + (intptr_t)&llGRYamabukiMapMarumineAttackEvents);

    if (ip->multi == ev[ip->event_id].timer)
    {
        ip->attack_coll.angle  = ev[ip->event_id].angle;
        ip->attack_coll.damage = ev[ip->event_id].damage;
        ip->attack_coll.size   = ev[ip->event_id].size;

        ip->attack_coll.can_reflect = FALSE;
        ip->attack_coll.can_shield = FALSE;

        ip->attack_coll.element = nGMHitElementFire;

        ip->attack_coll.can_setoff = FALSE;

        ip->event_id++;

        if (ip->event_id == 4)
        {
            ip->event_id = 3;
        }
    }
}

// 0x80183914
// decomp itmarumine.c:125-145 verbatim.
sb32 itMarumineCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->translate.vec.f.x += ip->item_vars.marumine.offset.x;
    dobj->translate.vec.f.y += ip->item_vars.marumine.offset.y;

    if (dobj->anim_wait == AOBJ_ANIM_NULL)
    {
        itMainRefreshAttackColl(item_gobj);
        itMainClearOwnerStats(item_gobj);

        ip->item_vars.marumine.offset.x = 0.0F;
        ip->item_vars.marumine.offset.y = 0.0F;

        itMarumineExplodeMakeEffectGotoSetStatus(item_gobj);
        func_800269C0_275C0(nSYAudioFGMExplodeL);
    }
    return FALSE;
}

// 0x801839A8
// decomp itmarumine.c:148-167 verbatim.
sb32 itMarumineExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->translate.vec.f.x += ip->item_vars.marumine.offset.x;
    dobj->translate.vec.f.y += ip->item_vars.marumine.offset.y;

    itMarumineExplodeUpdateAttackEvent(item_gobj);

    ip->multi++;

    if (ip->multi == ITMARUMINE_EXPLODE_LIFETIME)
    {
        grYamabukiGateSetClosedWait();

        return TRUE;
    }
    else return FALSE;
}

// 0x80183A20
// decomp itmarumine.c:170-182 verbatim.
void itMarumineExplodeSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = 0;

    ip->attack_coll.throw_mul = 1.0F;

    ip->event_id = 0;

    itMarumineExplodeUpdateAttackEvent(item_gobj);
    itMainSetStatus(item_gobj, dITMarumineStatusDescs, nITMarumineStatusExplode);
}

// 0x80183A74
// decomp itmarumine.c:185-202 verbatim.
GObj* itMarumineMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITMarumineItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);
        DObj *dobj = DObjGetStruct(item_gobj);

        ip->item_vars.marumine.offset = *pos;

        ip->is_allow_knockback = TRUE;

        gcAddXObjForDObjFixed(dobj, 0x46, 0);
        func_800269C0_275C0(nSYAudioVoiceYamabukiMarumine);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
