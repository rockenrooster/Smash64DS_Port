/* P2 Pippi / Clefairy (kind nITKindPippi). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itmonster/itpippi.c:1-231.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc token for the attribute row is
 * 0xC74 (reloc_data.us.h:3782) and the Pippi data-start base is 0x13598
 * (:3832); the port's generated reloc header does not publish Pippi
 * tokens, so this TU owns its uintptr_t tokens the same way
 * battleship_item_gbumper.c owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * Symbols the port headers do not publish yet (syUtilsRandIntRange and the
 * twelve cross-monster SetStatus entry points this file's metronome table
 * calls) are referenced verbatim and listed in the task report -- no values
 * invented here. itGetMonsterAnimNode, itDisplayCheckItemVisible, the
 * monster voice ID, the anim helpers, and func_800269C0_275C0 ride on
 * it/item.h, gm/gmsound.h, nds/nds_obj_anim.h, and sys/audio.h, so no local
 * externs are written for them.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sys/develop.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/objman.h>
#include <sys/audio.h>
#include <nds/nds_obj_anim.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3782. */
uintptr_t llITCommonDataPippiItemAttributes = 0xC74u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3832. */
uintptr_t llITCommonDataPippiDataStart = 0x13598u;

extern void *gITManagerCommonData;

/* decomp sys/utils.h:20. Same seam as
 * battleship_item_bombhei.c:60-61. */
extern s32 syUtilsRandIntRange(s32 range);

/* The metronome table below calls one SetStatus per monster kind. Five of
 * the twelve (Kabigon, Tosakinto, Nyars, Dogas, Mew) already live in
 * sibling import TUs, three more (MLucky, Starmie, Sawamura) land in this
 * same batch, and the remaining four (Iwark, Lizardon, Spear, Kamex) are
 * owned by a parallel agent; all twelve are referenced verbatim from their
 * decomp headers so the table itself stays verbatim, with no values
 * invented here. Declarations: decomp itiwark.h, itkabigon.h,
 * ittosakinto.h, itnyars.h, itlizardon.h, itspear.h, itkamex.h, itmlucky.h,
 * itstarmie.h, itsawamura.h, itdogas.h, itmew.h. */
extern void itIwarkAttackSetStatus(GObj *item_gobj);
extern void itKabigonJumpSetStatus(GObj *item_gobj);
extern void itTosakintoAppearSetStatus(GObj *item_gobj);
extern void itNyarsAttackSetStatus(GObj *item_gobj);
extern void itLizardonFallSetStatus(GObj *item_gobj);
extern void itSpearFlySetStatus(GObj *item_gobj);
extern void itKamexAppearSetStatus(GObj *item_gobj);
extern void itMLuckyAppearSetStatus(GObj *item_gobj);
extern void itStarmieNFollowSetStatus(GObj *item_gobj);
extern void itSawamuraFallSetStatus(GObj *item_gobj);
extern void itDogasAttackSetStatus(GObj *item_gobj);
extern void itMewFlySetStatus(GObj *item_gobj);

/* decomp itpippi.h:8-13 verbatim. The port publishes no per-kind item
 * procs, so the source header's declarations travel with this TU, exactly
 * as the Nyars and Kabigon files carry theirs. */
extern void itPippiCommonSelectMonster(GObj *item_gobj);
extern void itPippiCommonProcDisplay(GObj *item_gobj);
extern void itPippiCommonMoveDLProcDisplay(GObj *item_gobj);
extern sb32 itPippiCommonProcUpdate(GObj *item_gobj);
extern sb32 itPippiCommonProcMap(GObj *item_gobj);
extern GObj* itPippiMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// 0x8018B370
// decomp itpippi.c:13-27 verbatim.
void (*dITPippiStatusProcList[/* */])(GObj*) =
{
    itIwarkAttackSetStatus,
    itKabigonJumpSetStatus,
    itTosakintoAppearSetStatus,
    itNyarsAttackSetStatus,
    itLizardonFallSetStatus,
    itSpearFlySetStatus,
    itKamexAppearSetStatus,
    itMLuckyAppearSetStatus,
    itStarmieNFollowSetStatus,
    itSawamuraFallSetStatus,
    itDogasAttackSetStatus,
    itMewFlySetStatus
};

// 0x8018B3A0
// decomp itpippi.c:30-52 verbatim, adapted only for the port's ITDesc shape
// (o_attributes is const void * here, lbRelocGetFileData takes the token
// the same way).
ITDesc dITPippiItemDesc =
{
    nITKindPippi,                           // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataPippiItemAttributes,     // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0,                                  // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itPippiCommonProcUpdate,                // Proc Update
    itPippiCommonProcMap,                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

#if !defined(DAIRANTOU_OPT0)

// 0x8018B3D4 - why
// decomp itpippi.c:57 verbatim.
ITStatusDesc dITPippiStatusDesc = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#endif

// 0x80183210
// decomp itpippi.c:68-108 verbatim.
void itPippiCommonSelectMonster(GObj *item_gobj)
{
    s32 kind;
    s32 index;
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    index = syUtilsRandIntRange(ARRAY_COUNT(dITPippiStatusProcList));

    kind = index + nITKindMBallMonsterStart;

    if ((kind == nITKindSpear) || (kind == nITKindKamex))
    {
        if (syUtilsRandIntRange(2) == 0)
        {
            dobj->rotate.vec.f.y = F_CST_DTOR32(180.0F);

            ip->lr = +1;
        }
        else ip->lr = -1;
    }
    if ((kind == nITKindPippi) || (kind == nITKindTosakinto) || (kind == nITKindMLucky))
    {
        ip->attack_coll.attack_state = nGMAttackStateOff;
    }
    if (kind == nITKindSawamura)
    {
        ip->multi = ITSAWAMURA_KICK_WAIT;
    }
    if ((kind == nITKindSawamura) || (kind == nITKindStarmie))
    {
        item_gobj->proc_display = itPippiCommonMoveDLProcDisplay;

        gcMoveGObjDLHead(item_gobj, 18, item_gobj->dl_link_priority);
    }
    if (kind == nITKindLizardon)
    {
        ip->multi = ITLIZARDON_LIFETIME;
    }
    dITPippiStatusProcList[index](item_gobj);
}

// 0x80183344
// decomp itpippi.c:111-141 verbatim.
void itPippiCommonProcDisplay(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    gDPPipeSync(gSYTaskmanDLHeads[0]++);

    /* The source's four arms (itpippi.c:117-139) are three draws and a
     * debug overlay. The first three differ only in which condition selects
     * them -- all three set the same render mode and call the same draw --
     * and the fourth, itDisplayHitCollisions, draws the hit-status debug
     * overlay. Debug collision rendering is not a production display mode
     * on DS (battleship_item_link_core.c:861 states this for the shared
     * procs; battleship_item_kabigon.c:186-201 follows it), and
     * itDisplayMapCollisions is the same overlay for the map view, so the
     * three production arms collapse into one and the two debug ones drop.
     * The render mode and the draw are the source's. */
    if (itDisplayCheckItemVisible(ip) != FALSE)
    {
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2);

        gcDrawDObjTreeForGObj(item_gobj);
    }
    gDPPipeSync(gSYTaskmanDLHeads[0]++);
}

// 0x801834A0 - Render routine of Hitmonlee / Starmie metronome abilities
// decomp itpippi.c:144-174 verbatim.
void itPippiCommonMoveDLProcDisplay(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    gDPPipeSync(gSYTaskmanDLHeads[0]++);

    /* Same collapse as itPippiCommonProcDisplay above, on the source's
     * itpippi.c:150-172 -- three production arms that set one render mode
     * and make one draw, plus two debug-overlay arms this port has no
     * display mode for. */
    if (itDisplayCheckItemVisible(ip) != FALSE)
    {
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);

        gcDrawDObjTreeForGObj(item_gobj);
    }
    gDPPipeSync(gSYTaskmanDLHeads[0]++);
}

// 0x801835FC
// decomp itpippi.c:177-190 verbatim.
sb32 itPippiCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == 0)
    {
        ip->physics.vel_air.x = ip->physics.vel_air.y = 0.0F;

        itPippiCommonSelectMonster(item_gobj);
    }
    ip->multi--;

    return FALSE;
}

// 0x80183650
// decomp itpippi.c:193-202 verbatim.
sb32 itPippiCommonProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itMapTestAllCollisionFlag(item_gobj, MAP_FLAG_FLOOR) != FALSE)
    {
        ip->physics.vel_air.y = 0.0F;
    }
    return FALSE;
}

// 0x80183690
// decomp itpippi.c:205-231 verbatim.
GObj* itPippiMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITPippiItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *ip = itGetStruct(item_gobj);

        ip->multi = ITMONSTER_RISE_STOP_WAIT;

        ip->physics.vel_air.x = ip->physics.vel_air.z = 0.0F;
        ip->physics.vel_air.y = ITMONSTER_RISE_VEL_Y;

        gcAddXObjForDObjFixed(dobj, 0x48, 0);

        dobj->translate.vec.f = *pos;

        dobj->translate.vec.f.y -= ip->attr->map_coll_bottom;

        gcAddDObjAnimJoint(dobj, itGetMonsterAnimNode(ip, &llITCommonDataPippiDataStart), 0.0F);
        func_800269C0_275C0(nSYAudioVoiceMBallPippiAppear);

        item_gobj->proc_display = itPippiCommonProcDisplay;
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
