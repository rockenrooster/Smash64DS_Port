/* P2 Kabigon / Snorlax (kind nITKindKabigon). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itkabigon.c:1-306.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0x7A8 (reloc_data.us.h:3763) and the monster anim node base is 0xB158
 * (:3810); the port's generated reloc header does not publish Kabigon
 * tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (ITKABIGON_ and ITMONSTER_
 * tuning is present in include/it/item.h; itGetMonsterAnimNode, the SFX/
 * voice IDs, and the N64 display-list calls are not) are referenced verbatim
 * and listed in the task report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sys/develop.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3763. */
uintptr_t llITCommonDataKabigonItemAttributes = 0x7A8u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3810. */
uintptr_t llITCommonDataKabigonAnimJoint = 0xB158u;

extern void *gITManagerCommonData;

/* decomp sys/objanim.h:16. No port header in this TU's chain publishes it;
 * battleship_item_bombhei.c:53-55 carries the same kind of local externs. */
extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                               f32 anim_frame);

/* decomp sys/utils.h:19-:20. Same seam as
 * battleship_item_bombhei.c:60-61. */
extern f32 syUtilsRandFloat(void);
extern s32 syUtilsRandIntRange(s32 range);

/* decomp ef/efmanager.h:69 and :42. The port defines both
 * (efManagerQuakeMakeEffect and efManagerDustExpandLargeMakeEffect are in
 * the linked ELF); no port header in this TU's chain publishes them, so the
 * prototypes travel here verbatim. */
extern GObj *efManagerQuakeMakeEffect(s32 magnitude);
extern LBParticle *efManagerDustExpandLargeMakeEffect(Vec3f *pos);

/* decomp sys/objdisplay.h:46. The port defines it (in the linked ELF); no
 * port header in this TU's chain publishes it. */
extern void gcDrawDObjTreeForGObj(GObj *gobj);

/* itDisplayCheckItemVisible (decomp it/itdisplay.h:13) is published by
 * it/item.h, included above. Its two neighbours there, itDisplayMapCollisions
 * (:10) and itDisplayHitCollisions (:7), draw debug collision overlays this
 * port does not have -- see the note at the display proc below. */

/* Ground collision bounds the drop (decomp mp/mpcoll.h). The port publishes
 * the variable at include/gr/ground.h:513; the type rides the mp/map.h
 * chain above, so only the declaration travels here. */
extern MPGroundData *gMPCollisionGroundData;

/* Same shape as battleship_link_bomb.c:80. */
extern void func_800269C0_275C0(u16 sfx_id);

/* decomp itkabigon.h:8-17 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly as
 * the Tomato and Star files carry theirs. */
extern sb32 itKabigonFallProcUpdate(GObj *item_gobj);
extern void itKabigonFallProcDisplay(GObj *item_gobj);
extern void itKabigonFallInitVars(GObj *item_gobj);
extern void itKabigonFallSetStatus(GObj *item_gobj);
extern sb32 itKabigonJumpProcUpdate(GObj *item_gobj);
extern void itKabigonCommonProcDisplay(GObj *item_gobj);
extern void itKabigonJumpInitVars(GObj *item_gobj);
extern void itKabigonJumpSetStatus(GObj *item_gobj);
extern sb32 itKabigonCommonProcUpdate(GObj *item_gobj);
extern GObj* itKabigonMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// 0x8018AB40
// decomp itkabigon.c:12-34 verbatim, adapted only for the port's ITDesc
// shape (o_attributes is const void * here, lbRelocGetFileData takes the
// token the same way).
ITDesc dITKabigonItemDesc =
{
    nITKindKabigon,                         // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataKabigonItemAttributes,   // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,         // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itKabigonCommonProcUpdate,              // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

// 0x8018AB74
// decomp itkabigon.c:37-62 verbatim.
ITStatusDesc dITKabigonStatusDescs[/* */] =
{
    // Status 0 (Neutral Jump)
    {
        itKabigonJumpProcUpdate,            // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Neutral Fall)
    {
        itKabigonFallProcUpdate,            // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

// decomp itkabigon.c:70-75 verbatim.
enum itKabigonStatus
{
    nITKabigonStatusJump,
    nITKabigonStatusFall,
    nITKabigonStatusEnumCount
};

// 0x8017E070
// decomp itkabigon.c:84-102 verbatim.
sb32 itKabigonFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->item_vars.kabigon.rumble_wait == 0)
    {
        efManagerQuakeMakeEffect(0);

        ip->item_vars.kabigon.rumble_wait = ITKABIGON_RUMBLE_WAIT;
    }
    ip->item_vars.kabigon.rumble_wait--;

    if (dobj->translate.vec.f.y < (gMPCollisionGroundData->map_bound_bottom + ITKABIGON_MAP_OFF_Y))
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x8017E100
// decomp itkabigon.c:105-135 verbatim.
void itKabigonFallProcDisplay(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    gDPPipeSync(gSYTaskmanDLHeads[0]++);

    /* The source's four arms (itkabigon.c:113-133) are three draws and a
     * debug overlay. The first three differ only in which condition selects
     * them -- all three set the same render mode and call the same draw -- and
     * the fourth, itDisplayHitCollisions, draws the hit-status debug overlay.
     * Debug collision rendering is not a production display mode on DS
     * (battleship_item_link_core.c:861 states this for the shared procs and
     * this follows it), and itDisplayMapCollisions is the same overlay for the
     * map view, so the three production arms collapse into one and the two
     * debug ones drop. The render mode and the draw are the source's. */
    if (itDisplayCheckItemVisible(ip) != FALSE)
    {
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);

        gcDrawDObjTreeForGObj(item_gobj);
    }
    gDPPipeSync(gSYTaskmanDLHeads[0]++);
}

// 0x8017E25C
// decomp itkabigon.c:138-170 verbatim.
void itKabigonFallInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    ip->physics.vel_air.y = ITKABIGON_DROP_VEL_Y;

    dobj->translate.vec.f.x += ((ITKABIGON_DROP_OFF_X_MUL * syUtilsRandFloat()) + ITKABIGON_DROP_OFF_X_ADD);

    itMainRefreshAttackColl(item_gobj);

    ip->item_vars.kabigon.rumble_wait = 0;

    func_800269C0_275C0(nSYAudioFGMKabigonFall);

    if (ip->kind == nITKindKabigon)
    {
        func_800269C0_275C0(nSYAudioVoiceMBallKabigonFall);

        dobj->scale.vec.f.x = dobj->scale.vec.f.y = ITKABIGON_DROP_SIZE_KABIGON;

        ip->attack_coll.size *= ITKABIGON_DROP_SIZE_KABIGON;
    }
    else
    {
        dobj->scale.vec.f.x = dobj->scale.vec.f.y = ITKABIGON_DROP_SIZE_OTHER;

        ip->attack_coll.size *= ITKABIGON_DROP_SIZE_OTHER;
    }
    item_gobj->proc_display = itKabigonFallProcDisplay;

    gcMoveGObjDLHead(item_gobj, 18, item_gobj->dl_link_priority);
}

// 0x8017E350
// decomp itkabigon.c:173-177 verbatim.
void itKabigonFallSetStatus(GObj *item_gobj)
{
    itKabigonFallInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITKabigonStatusDescs, nITKabigonStatusFall);
}

// 0x8017E384
// decomp itkabigon.c:180-210 verbatim.
sb32 itKabigonJumpProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (dobj->translate.vec.f.y >= (gMPCollisionGroundData->map_bound_top - ITKABIGON_MAP_OFF_Y))
    {
        ip->multi--;

        ip->physics.vel_air.y = 0.0F;

        if (ip->multi == 0)
        {
            itKabigonFallSetStatus(item_gobj);
        }
    }
    if (ip->item_vars.kabigon.dust_effect_int == 0)
    {
        Vec3f pos = dobj->translate.vec.f;

        pos.x += (syUtilsRandFloat() * ITKABIGON_JUMP_GFX_MUL_OFF) - ITKABIGON_JUMP_GFX_SUB_OFF;
        pos.y += (syUtilsRandFloat() * ITKABIGON_JUMP_GFX_MUL_OFF) - ITKABIGON_JUMP_GFX_SUB_OFF;

        efManagerDustExpandLargeMakeEffect(&pos);

        ip->item_vars.kabigon.dust_effect_int = ITKABIGON_EFFECT_SPAWN_INT;
    }
    ip->item_vars.kabigon.dust_effect_int--;

    return FALSE;
}

// 0x8017E4A4
// decomp itkabigon.c:213-243 verbatim.
void itKabigonCommonProcDisplay(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    gDPPipeSync(gSYTaskmanDLHeads[0]++);

    /* Same collapse as itKabigonFallProcDisplay above, on the source's
     * itkabigon.c:221-241 -- three production arms that set one render mode
     * and make one draw, plus two debug-overlay arms this port has no display
     * mode for. */
    if (itDisplayCheckItemVisible(ip) != FALSE)
    {
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2);

        gcDrawDObjTreeForGObj(item_gobj);
    }
    gDPPipeSync(gSYTaskmanDLHeads[0]++);
}

// 0x8017E600
// decomp itkabigon.c:246-257 verbatim.
void itKabigonJumpInitVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    func_800269C0_275C0(nSYAudioFGMKabigonJump);

    ip->multi = ITKABIGON_DROP_WAIT;

    ip->item_vars.kabigon.dust_effect_int = ITKABIGON_EFFECT_SPAWN_INT;

    ip->physics.vel_air.y = ITKABIGON_JUMP_VEL_Y;
}

// 0x8017E648
// decomp itkabigon.c:260-264 verbatim.
void itKabigonJumpSetStatus(GObj *item_gobj)
{
    itKabigonJumpInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITKabigonStatusDescs, nITKabigonStatusJump);
}

// 0x8017E67C
// decomp itkabigon.c:267-278 verbatim.
sb32 itKabigonCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        itKabigonJumpSetStatus(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x8017E6C0
// decomp itkabigon.c:281-306 verbatim.
GObj* itKabigonMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITKabigonItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_FIGHTER;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataKabigonAnimJoint), 0.0F);

        if (ip->kind == nITKindKabigon)
        {
            func_800269C0_275C0(nSYAudioVoiceMBallKabigonAppear);
        }
        item_gobj->proc_display = itKabigonCommonProcDisplay;
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
