/* P2-5 Hammer (kind nITKindHammer). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/ithammer.c:10-248.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x374 (reloc_data.us.h:3745; == reloc_data.jp.h:3696); the port's
 * generated reloc header does not publish a Hammer token, so this TU owns
 * its uintptr_t token the same way battleship_item_gbumper.c owns GBumper's
 * (local token, no generator involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITHAMMER_* tuning, ColAnim
 * ID, itmap helpers) are referenced verbatim and listed in the task report
 * -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <if/interface.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* reloc_data.us.h:3745 (== reloc_data.jp.h:3696). */
uintptr_t llITCommonDataHammerItemAttributes = 0x374u;

extern void *gITManagerCommonData;

/* decomp ithammer.c:10-32 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITHammerItemDesc =
{
    nITKindHammer,                          // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataHammerItemAttributes,    // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itHammerFallProcUpdate,                 // Proc Update
    itHammerFallProcMap,                    // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// decomp ithammer.c:34-95 verbatim.
ITStatusDesc dITHammerStatusDescs[/* */] =
{
    // Status 0 (Ground Wait)
    {
        NULL,                               // Proc Update
        itHammerWaitProcMap,                // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Air Wait Fall)
    {
        itHammerFallProcUpdate,             // Proc Update
        itHammerFallProcMap,                // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 2 (Fighter Hold)
    {
        NULL,                               // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 3 (Fighter Throw)
    {
        itHammerThrownProcUpdate,           // Proc Update
        itHammerThrownProcMap,              // Proc Map
        itHammerCommonProcHit,              // Proc Hit
        itHammerCommonProcHit,              // Proc Shield
        NULL,                               // Proc Hop
        itHammerCommonProcHit,              // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 4 (Fighter Drop)
    {
        itHammerFallProcUpdate,             // Proc Update
        itHammerDroppedProcMap,             // Proc Map
        itHammerCommonProcHit,              // Proc Hit
        itHammerCommonProcHit,              // Proc Shield
        NULL,                               // Proc Hop
        itHammerCommonProcHit,              // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// // // // // // // // // // // //
//                               //
//          ENUMERATORS          //
//                               //
// // // // // // // // // // // //

// decomp ithammer.c:103-111 verbatim.
enum itHammerStatus
{
    nITHammerStatusWait,
    nITHammerStatusFall,
    nITHammerStatusHold,
    nITHammerStatusThrown,
    nITHammerStatusDropped,
    nITHammerStatusEnumCount
};

// 0x80176110
// decomp ithammer.c:120-123 verbatim.
void itHammerCommonSetColAnim(GObj *item_gobj)
{
    itMainCheckSetColAnimID(item_gobj, nGMColAnimItemHammerEnd, 0);
}

// 0x80176134
// decomp ithammer.c:126-134 verbatim.
sb32 itHammerFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITHAMMER_GRAVITY, ITHAMMER_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

// 0x8017616C
// decomp ithammer.c:137-142 verbatim.
sb32 itHammerWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itHammerFallSetStatus);

    return FALSE;
}

// 0x80176194
// decomp ithammer.c:145-148 verbatim.
sb32 itHammerFallProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITHAMMER_MAP_REBOUND_COMMON, ITHAMMER_MAP_REBOUND_GROUND, itHammerWaitSetStatus);
}

// 0x801761C4
// decomp ithammer.c:152-155 verbatim.
void itHammerWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITHammerStatusDescs, nITHammerStatusWait);
}

// 0x801761F8
// decomp ithammer.c:158-166 verbatim.
void itHammerFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITHammerStatusDescs, nITHammerStatusFall);
}

// 0x8017623C
// decomp ithammer.c:169-174 verbatim.
void itHammerHoldSetStatus(GObj *item_gobj)
{
    DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(0.0F);

    itMainSetStatus(item_gobj, dITHammerStatusDescs, nITHammerStatusHold);
}

// 0x80176270
// decomp ithammer.c:177-185 verbatim.
sb32 itHammerThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITHAMMER_GRAVITY, ITHAMMER_TVEL);
    itVisualsUpdateSpin(item_gobj);

    return FALSE;
}

// 0x801762A8
// decomp ithammer.c:188-191 verbatim.
sb32 itHammerThrownProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITHAMMER_MAP_REBOUND_COMMON, ITHAMMER_MAP_REBOUND_GROUND, itHammerWaitSetStatus);
}

// 0x801762D8
// decomp ithammer.c:195-203 verbatim.
sb32 itHammerCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

// 0x80176300
// decomp ithammer.c:206-213 verbatim.
void itHammerThrownSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITHammerStatusDescs, nITHammerStatusThrown);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);

    ftParamTryUpdateItemMusic();
}

// 0x80176348
// decomp ithammer.c:216-219 verbatim.
sb32 itHammerDroppedProcMap(GObj *item_gobj)
{
    return itMapCheckDestroyDropped(item_gobj, ITHAMMER_MAP_REBOUND_COMMON, ITHAMMER_MAP_REBOUND_GROUND, itHammerWaitSetStatus);
}

// 0x80176378
// decomp ithammer.c:222-230 verbatim.
void itHammerDroppedSetStatus(GObj *item_gobj)
{
    itMainClearColAnim(item_gobj);
    itMainSetStatus(item_gobj, dITHammerStatusDescs, nITHammerStatusDropped);

    DObjGetStruct(item_gobj)->child->rotate.vec.f.y = F_CST_DTOR32(90.0F);

    ftParamTryUpdateItemMusic();
}

// 0x8017633C8
// decomp ithammer.c:233-248 verbatim.
GObj* itHammerMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITHammerItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        DObjGetStruct(item_gobj)->rotate.vec.f.y = F_CST_DTOR32(90.0F);

        ip->is_unused_item_bool = TRUE;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
