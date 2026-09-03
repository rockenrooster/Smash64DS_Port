/* P2-5 Star (kind nITKindStar). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itstar.c:11-130.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x148 (reloc_data.us.h:3736; == reloc_data.jp.h:3687); the port's
 * generated reloc header does not publish a Star token, so this TU owns its
 * uintptr_t token the same way battleship_item_gbumper.c owns GBumper's
 * (local token, no generator involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITSTAR_* tuning, SFX/BGM IDs,
 * itmap helpers) are referenced verbatim and listed in the task report --
 * no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <if/interface.h>
#include <gm/gmsound.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <gm/generic.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

/* reloc_data.us.h:3736 (== reloc_data.jp.h:3687). */
uintptr_t llITCommonDataStarItemAttributes = 0x148u;

extern void *gITManagerCommonData;

/* decomp itstar.h:8-12. The port publishes no per-kind item procs, so the
 * source header's declarations travel with this TU, exactly as the Tomato and
 * Heart files carry theirs. */
sb32 itStarCommonProcUpdate(GObj *item_gobj);
sb32 itStarCommonProcMap(GObj *item_gobj);
sb32 itStarCommonProcHit(GObj *item_gobj);
GObj *itStarMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);


/* decomp itstar.c:11-33 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITStarItemDesc =
{
    nITKindStar,                            // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataStarItemAttributes,      // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itStarCommonProcUpdate,                 // Proc Update
    itStarCommonProcMap,                    // Proc Map
    itStarCommonProcHit,                    // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x80174930
// decomp itstar.c:42-57 verbatim.
sb32 itStarCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITSTAR_GRAVITY, ITSTAR_TVEL);

    ip->multi--;

    if (ip->multi == 0)
    {
        itMainRefreshAttackColl(item_gobj);
    }
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

// 0x80174990
// decomp itstar.c:60-77 verbatim.
sb32 itStarCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    s32 unused;
    sb32 is_collide_floor = itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR);

    if (itMapCheckCollideAllRebound(item_gobj, (MAP_FLAG_CEIL | MAP_FLAG_RWALL | MAP_FLAG_LWALL), ITSTAR_MAP_REBOUND_COMMON, NULL) != FALSE)
    {
        itMainSetSpinVelLR(item_gobj);
    }
    if (is_collide_floor != FALSE)
    {
        ip->physics.vel_air.y = ITSTAR_BOUNCE_Y;

        func_800269C0_275C0(nSYAudioFGMStarMapCollide);
    }
    return FALSE;
}

// 0x80174A0C
// decomp itstar.c:80-83 verbatim.
sb32 itStarCommonProcHit(GObj *item_gobj)
{
    return TRUE;
}

// 0x80174A18
// decomp itstar.c:86-130 verbatim.
GObj* itStarMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    CObj *cobj = CObjGetStruct(gGMCameraGObj);
    GObj *item_gobj;
    DObj *dobj;
    ITStruct *ip;
    Vec3f vel_real;
#if defined(REGION_US)
    Vec3f translate;
#endif

    vel_real.x = (pos->x < cobj->vec.at.x) ? ITSTAR_VEL_X : -ITSTAR_VEL_X;
    vel_real.y = ITSTAR_BOUNCE_Y;
    vel_real.z = 0.0F;

    item_gobj = itManagerMakeItem(parent_gobj, &dITStarItemDesc, pos, &vel_real, flags);

    if (item_gobj != NULL)
    {
        dobj = DObjGetStruct(item_gobj);

#if defined(REGION_US)
        translate = dobj->translate.vec.f;
#endif

        ip = itGetStruct(item_gobj);

        ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER; // Star Man can only interact with fighters

        ip->multi = ITSTAR_INTERACT_DELAY;

        ip->is_unused_item_bool = TRUE;

        gcAddXObjForDObjFixed(dobj, 0x2E, 0);

        dobj->rotate.vec.f.z = 0.0F;

#if defined(REGION_US)
        dobj->translate.vec.f = translate;
#else
        dobj->translate.vec.f = *pos;
#endif
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
