/* P2 Mew (kind nITKindMew). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itmew.c:1-176.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x838 (reloc_data.us.h:3765) and the monster anim node base is 0xBCC0
 * (reloc_data.us.h:3814); the port's generated reloc header does not publish
 * Mew tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITMEW_ and ITMONSTER_ tuning,
 * itGetMonsterAnimNode, SFX/voice IDs, effect managers) are referenced
 * verbatim and listed in the task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <nds/nds_obj_anim.h>
#include <sys/audio.h>
#include <ef/effect.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3765. */
uintptr_t llITCommonDataMewItemAttributes = 0x838u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3814. */
uintptr_t llITCommonDataMewDataStart = 0xBCC0u;

extern void *gITManagerCommonData;

/* decomp itmew.h:8-13 verbatim. The port publishes no per-kind item procs, so
 * the source header's declarations travel with this TU, exactly as the Tomato
 * and Star files carry theirs. */
extern sb32 itMewFlyProcUpdate(GObj *item_gobj);
extern void itMewFlyInitVars(GObj *item_gobj);
extern void itMewFlySetStatus(GObj *item_gobj);
extern sb32 itMewCommonProcUpdate(GObj *item_gobj);
extern sb32 itMewCommonProcMap(GObj *item_gobj);
extern GObj* itMewMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp itmew.c:11-33 verbatim, adapted only for the port's ITDesc shape
 * (o_attributes is const void * here, lbRelocGetFileData takes the token the
 * same way). */
ITDesc dITMewItemDesc =
{
    nITKindMew,                             // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataMewItemAttributes,       // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,         // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itMewCommonProcUpdate,                  // Proc Update
    itMewCommonProcMap,                     // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018AC74
// decomp itmew.c:36-49 verbatim.
ITStatusDesc dITMewStatusDescs[/* */] =
{
    // Status 0 (Neutral FLy)
    {
        itMewFlyProcUpdate,                 // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// decomp itmew.c:57-61 verbatim.
enum itMewStatus
{
    nITMewStatusFly,
    nITMewStatusEnumCount
};

// 0x8017EBE0
// decomp itmew.c:70-92 verbatim.
sb32 itMewFlyProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    Vec3f pos = DObjGetStruct(item_gobj)->translate.vec.f;

    if (ip->multi == 0)
    {
        return TRUE;
    }
    if (ip->item_vars.mew.esper_gfx_int == 0)
    {
        ip->item_vars.mew.esper_gfx_int = ITMEW_EFFECT_SPAWN_INT;

        efManagerHealSparklesMakeEffect(&pos);
    }
    ip->item_vars.mew.esper_gfx_int--;

    ip->multi--;

    ip->physics.vel_air.y += ITMEW_FLY_ADD_VEL_Y;

    return FALSE;
}

// 0x8017EC84
// decomp itmew.c:95-118 verbatim.
void itMewFlyInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->multi = ITMEW_LIFETIME;

    if (syUtilsRandIntRange(2) != 0)
    {
        ip->physics.vel_air.x = ITMEW_STARTVEL_X;
    }
    else ip->physics.vel_air.x = -ITMEW_STARTVEL_X;

    ip->physics.vel_air.y = ITMEW_STARTVEL_Y;

    func_800269C0_275C0(nSYAudioFGMMewFly);

    if (ip->kind == nITKindMew)
    {
        func_800269C0_275C0(nSYAudioVoiceMBallMewAppear);
    }
    efManagerRippleMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

    ip->item_vars.mew.esper_gfx_int = 0;
}

// 0x8017ED20
// decomp itmew.c:121-125 verbatim.
void itMewFlySetStatus(GObj *item_gobj)
{
    itMewFlyInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITMewStatusDescs, nITMewStatusFly);
}

// 0x8017ED54
// decomp itmew.c:128-141 verbatim.
sb32 itMewCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->physics.vel_air.y = 0.0F;

        itMewFlySetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x8017EDA4
// decomp itmew.c:144-153 verbatim.
sb32 itMewCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x8017EDE4
// decomp itmew.c:156-176 verbatim.
GObj* itMewMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITMewItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y; // Starting to think this is a macro

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        // This ptr stuff is likely also a macro
        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataMewDataStart), 0.0F);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
