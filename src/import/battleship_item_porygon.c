/* P2 Saffron City Porygon (kind nITKindPorygon). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itground/itporygon.c:11-119.
 *
 * Art/state live in the stage's own map file: the descriptor's file base is
 * gGRCommonStruct.yamabuki.item_head (port include/gr/ground.h:431-434) and
 * the reloc tokens below are decomp/BattleShip-main/include/reloc_data.us.h
 * :4003 (:0x16C attributes) and :4004 (:0x1B4 hit parties); the port's
 * generated reloc header does not publish Yamabuki tokens, so this TU owns
 * its uintptr_t tokens the same way battleship_item_gbumper.c owns GBumper's
 * (local tokens, no generator involvement, no hand-edited generated file).
 *
 * The monster hitbox script (ITMonsterEvent / itGetMonsterEvent) has no port
 * equivalent -- <it/item.h> only carries the attack-event helper -- so the
 * decomp definitions (ittypes.h:120-132, item.h:51) are transcribed verbatim
 * below, cited per line, the same way this TU carries the source header's
 * per-kind proc prototypes. They are TU-local; nothing under include/ is
 * touched. Symbols the port headers do not publish yet
 * (nSYAudioVoiceYamabukiPorygon, decomp gmsound.h:653) are referenced
 * verbatim and listed in the task report -- no values invented here.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal
 * (ITPORYGON_* rides on <it/item.h>; the event-id clamp at 2 back to 1 is the
 * source's own).
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

/* decomp/BattleShip-main/include/reloc_data.us.h:4003. */
uintptr_t llGRYamabukiMapPorygonItemAttributes = 0x16Cu;
/* decomp/BattleShip-main/include/reloc_data.us.h:4004. */
uintptr_t llGRYamabukiMapPorygonHitParties = 0x1B4u;

extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                f32 f_index);

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

/* decomp itporygon.h:8-10 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly as
 * the Tomato and Star files carry theirs. */
extern void itPorygonCommonUpdateMonsterEvent(GObj *item_gobj);
extern sb32 itPorygonCommonProcUpdate(GObj *item_gobj);
extern GObj* itPorygonMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// decomp itporygon.c:11-33 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token the
// same way).
ITDesc dITPorygonItemDesc =
{
    nITKindPorygon,                         // Item Kind
    &gGRCommonStruct.yamabuki.item_head,    // Pointer to item file data?
    &llGRYamabukiMapPorygonItemAttributes,   // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itPorygonCommonProcUpdate,              // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x80183B10
// decomp itporygon.c:42-77 verbatim.
void itPorygonCommonUpdateMonsterEvent(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITMonsterEvent *ev = itGetMonsterEvent(dITPorygonItemDesc, &llGRYamabukiMapPorygonHitParties); // (ITMonsterEvent*) ((uintptr_t)*dITPorygonItemDesc.p_file + (intptr_t)&Porygon_Event);

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

    if (ip->multi == ITPORYGON_SHAKE_STOP_WAIT)
    {
        Vec3f pos = DObjGetStruct(item_gobj)->translate.vec.f;

        pos.y = 0.0F;

        efManagerDustLightMakeEffect(&pos, -1, 1.0F);
    }
}

// 0x80183C84
// decomp itporygon.c:80-97 verbatim.
sb32 itPorygonCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->translate.vec.f.x += ip->item_vars.porygon.offset.x;
    dobj->translate.vec.f.y += ip->item_vars.porygon.offset.y;

    itPorygonCommonUpdateMonsterEvent(item_gobj);

    if (dobj->anim_wait == AOBJ_ANIM_NULL)
    {
        grYamabukiGateSetClosedWait();

        return TRUE;
    }
    else return FALSE;
}

// 0x80183D00
// decomp itporygon.c:100-119 verbatim.
GObj* itPorygonMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITPorygonItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->item_vars.porygon.offset = *pos;

        ip->is_allow_knockback = TRUE;

        ip->multi = 0;

        ip->event_id = 0;

        func_800269C0_275C0(nSYAudioVoiceYamabukiPorygon);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
