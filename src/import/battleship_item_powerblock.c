/* P2 Mushroom Kingdom POW block (kind nITKindPowerBlock). Verbatim-adapted
 * from decomp/BattleShip-main/decomp/src/it/itground/itpowerblock.c:11-129.
 *
 * Art/state live in the stage's own map file: the descriptor's file base is
 * gGRCommonStruct.inishie.item_head (port include/gr/ground.h:322-325) and
 * the reloc tokens below are decomp/BattleShip-main/include/reloc_data.us.h
 * :3929 (:0xD8 attributes), :3930 (:0x11F8 data start) and :3931 (:0x1288
 * anim joint); the port's generated reloc header does not publish Inishie
 * tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal
 * (the 0x44 main matrix kind is the source's literal, kept verbatim).
 * Symbols the port headers do not publish yet (nSYAudioFGMInishiePowerBlock;
 * grInishiePowerBlockSetWait / grInishiePowerBlockSetDamage, defined by the
 * Inishie stage TU's decomp include) are referenced verbatim and listed in
 * the task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3929. */
uintptr_t llGRInishieMapPowerBlockItemAttributes = 0xD8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3930. */
uintptr_t llGRInishieMapPowerBlockDataStart = 0x11F8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3931. */
uintptr_t llGRInishieMapPowerBlockAnimJoint = 0x1288u;

extern void grInishiePowerBlockSetWait(void);
extern void grInishiePowerBlockSetDamage(void);
extern GObj *efManagerQuakeMakeEffect(s32 id);

/* decomp sys/objanim.h. No port header in this TU's chain publishes them;
 * battleship_item_dogas.c:49-51 carries the same local externs. */
extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                               f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);

/* decomp itpowerblock.h:8-12 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly as
 * the Tomato and Star files carry theirs. */
extern sb32 itPowerBlockCommonProcUpdate(GObj *item_gobj);
extern void itPowerBlockWaitSetStatus(GObj *item_gobj);
extern sb32 itPowerBlockNDamageProcUpdate(GObj *item_gobj);
extern sb32 itPowerBlockWaitProcDamage(GObj *item_gobj);
extern GObj* itPowerBlockMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// decomp itpowerblock.c:11-33 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITPowerBlockItemDesc =
{
    nITKindPowerBlock,                      // Item Kind
    &gGRCommonStruct.inishie.item_head,     // Pointer to item file data?
    &llGRInishieMapPowerBlockItemAttributes,// Offset of item attributes in file?

    // DObj transformation struct
    {
        0x44,                               // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itPowerBlockCommonProcUpdate,           // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// decomp itpowerblock.c:35-48 verbatim.
ITStatusDesc dITPowerBlockStatusDescs[/* */] =
{
    // Status 0 (Neutral Wait)
    {
        NULL,                               // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        itPowerBlockWaitProcDamage          // Proc Damage
    }
};

// decomp itpowerblock.c:56-60 verbatim.
enum itPowerBlockStatus
{
    nITPowerBlockStatusWait,
    nITPowerBlockStatusEnumCount
};

// 0x8017C090
// decomp itpowerblock.c:69-76 verbatim.
sb32 itPowerBlockCommonProcUpdate(GObj *item_gobj)
{
    if (DObjGetStruct(item_gobj)->anim_wait == AOBJ_ANIM_NULL)
    {
        itPowerBlockWaitSetStatus(item_gobj);
    }
    return FALSE;
}

// 0x8017C0D4
// decomp itpowerblock.c:79-86 verbatim.
void itPowerBlockWaitSetStatus(GObj *item_gobj)
{
    ITStruct *ip;

    itMainSetStatus(item_gobj, dITPowerBlockStatusDescs, nITPowerBlockStatusWait);

    ip = itGetStruct(item_gobj), ip->damage_coll.hitstatus = nGMHitStatusNormal;
}

// 0x8017C110
// decomp itpowerblock.c:89-98 verbatim.
sb32 itPowerBlockNDamageProcUpdate(GObj *item_gobj)
{
    if (DObjGetStruct(item_gobj)->anim_wait == AOBJ_ANIM_NULL)
    {
        grInishiePowerBlockSetWait();

        return TRUE;
    }
    else return FALSE;
}

// 0x8017C15C
// decomp itpowerblock.c:101-115 verbatim.
sb32 itPowerBlockWaitProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->proc_update = itPowerBlockNDamageProcUpdate;
    ip->damage_coll.hitstatus = nGMHitStatusNone;

    gcAddDObjAnimJoint(DObjGetStruct(item_gobj), itGetPData(ip, &llGRInishieMapPowerBlockDataStart, &llGRInishieMapPowerBlockAnimJoint), 0.0F);
    gcPlayAnimAll(item_gobj);
    func_800269C0_275C0(nSYAudioFGMInishiePowerBlock);
    efManagerQuakeMakeEffect(3);
    grInishiePowerBlockSetDamage();

    return FALSE;
}

// 0x8017C1E0
// decomp itpowerblock.c:118-129 verbatim.
GObj* itPowerBlockMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITPowerBlockItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->damage_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER;
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
