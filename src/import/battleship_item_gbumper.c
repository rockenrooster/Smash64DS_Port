/* P2-5i1 GBumper (stage bumper, kind 23). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itground/itgbumper.c:12-116.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work (docs/p2/P2-5-items.md:160-164). The reloc
 * token for the attribute row is 0xCF0
 * (reloc_data.us.h:3784); the port's generated reloc header does
 * not publish a GBumper token, so this TU owns its uintptr_t token the same
 * way battleship_link_bomb.c:50-52 owns Link's (local token, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3784. */
uintptr_t llITCommonDataGBumperItemAttributes = 0xCF0u;

extern void *gITManagerCommonData;
extern volatile u32 gNdsGBumperMakeCount;

/* decomp itgbumper.c:12-34 verbatim, adapted only for the port's ITDesc
 * shape (o_attributes is const void * here, lbRelocGetFileData takes the
 * token the same way). */
ITDesc dITGBumperItemDesc =
{
    nITKindGBumper,                         /* Item Kind */
    &gITManagerCommonData,                  /* Pointer to item file data? */
    &llITCommonDataGBumperItemAttributes,   /* Offset of item attributes in file? */

    /* DObj transformation struct */
    {
        nGCMatrixKindTraRotRpyRSca,         /* Main matrix transformations */
        nGCMatrixKindNull,                  /* Secondary matrix transformations? */
        0                                   /* ??? */
    },

    nGMAttackStateNew,                      /* Hitbox Update State */
    itGBumperCommonProcUpdate,              /* Proc Update */
    NULL,                                   /* Proc Map */
    itGBumperCommonProcHit,                 /* Proc Hit */
    NULL,                                   /* Proc Shield */
    NULL,                                   /* Proc Hop */
    NULL,                                   /* Proc Set-Off */
    NULL,                                   /* Proc Reflector */
    NULL                                    /* Proc Damage */
};

/* decomp itgbumper.c:43-63 verbatim. */
sb32 itGBumperCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if ((ip->item_vars.bumper.hit_anim_length == 0) && (dobj->mobj->palette_id == 1.0F))
    {
        dobj->mobj->palette_id = 0;
    }
    else ip->item_vars.bumper.hit_anim_length--;

    if (ip->multi != 0)
    {
        dobj->scale.vec.f.x = dobj->scale.vec.f.y = ( 2.0F - ( (10 - ip->multi) * 0.1F ) );

        ip->multi--;
    }
    else dobj->scale.vec.f.x = dobj->scale.vec.f.y = 1;

    return FALSE;
}

/* decomp itgbumper.c:66-81 verbatim. */
sb32 itGBumperCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->scale.vec.f.x = 2.0F;
    dobj->scale.vec.f.y = 2.0F;

    ip->item_vars.bumper.hit_anim_length = ITBUMPER_HIT_ANIM_LENGTH;

    dobj->mobj->palette_id = 1.0F;

    ip->multi = ITBUMPER_HIT_SCALE;

    return FALSE;
}

/* decomp itgbumper.c:84-116 verbatim. */
GObj* itGBumperMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITGBumperItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip;
        DObj *dobj;

        itMainClearOwnerStats(item_gobj);

        ip = itGetStruct(item_gobj);
        dobj = DObjGetStruct(item_gobj);

        ip->multi = 0;

        ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER;
        ip->attack_coll.can_rehit_shield = TRUE;

        ip->physics.vel_air.x = 0.0F;
        ip->physics.vel_air.y = 0.0F;
        ip->physics.vel_air.z = 0.0F;

        dobj->mobj->palette_id = 0;

        if (gSCManagerBattleState->gkind == nGRKindCastle)
        {
            ip->attack_coll.knockback_weight = ITBUMPER_CASTLE_KNOCKBACK;
            ip->attack_coll.angle = ITBUMPER_CASTLE_ANGLE;
        }
        gNdsGBumperMakeCount++;
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
