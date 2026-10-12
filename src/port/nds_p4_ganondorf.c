/*
 * P4 Ganondorf: his Warlock Punch, Warlock Kick and entry craft on his own
 * files. Source: JSsixtyfour/smashremix 5e04fe7, src/captainshared.asm
 * (punch_anim_struct, kick_anim_struct, entry_anim_struct and the
 * get_*_anim_struct_ / get_entry_file_ptr_ hooks on his character id). Remix
 * gives Captain's three makers his files at Captain's offsets: the punch on
 * GND file 8, the kick and the craft on GND file 7, re-skinned. Those are
 * his special files 3 and 2, his own storage (the generator's
 * OWN_SPECIAL_FILES), so a match with Captain keeps both fighters' colours;
 * the makers below are Captain's (decomp efmanager.c) on that storage.
 * Ganondorf runs Captain's code (fp->fkind == nFTKindCaptain).
 */
#include <nds/nds_p4.h>

#if NDS_P4_GANONDORF

#include <ef/effect.h>
#include <ft/fighter.h>

void gcDrawDObjDLHead1(GObj *gobj);
void gcDrawDObjTreeForGObj(GObj *gobj);
void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints, f32 anim_frame);
void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint, f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);
void efManagerCaptainEntryCarProcUpdate(GObj *effect_gobj);

#ifndef lbRelocGetFileData
#define lbRelocGetFileData(type, file, offset) \
    ((type)((uintptr_t)(file) + (intptr_t)(offset)))
#endif
#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* His kick and craft file (GND file 7) and his punch file (GND file 8):
 * generated, OWN_SPECIAL_FILES. */
extern void *gNdsP4GanondorfSpecial2;
extern void *gNdsP4GanondorfSpecial3;

/* Makers that found a file or a joint missing: counted, never a fault. */
__attribute__((used)) volatile u32 gNdsP4GanondorfArticleMisses;

/* ---- Warlock Punch (dEFManagerCaptainFalconPunchEffectDesc) ----
 *
 * Its display is lbCommonDObjScaleXProcDisplay, empty in the port: the single
 * DObj goes through display-list head 1, as the Captain bridge in
 * battleship_efmanager.c sends Falcon Punch. */
static EFDesc sNdsP4GanondorfPunchEffectDesc = {
    EFFECT_FLAG_USERDATA,
    15,
    &gNdsP4GanondorfSpecial3,
    { 0x50, nGCMatrixKindRotRpyR, 0x00 },
    { nGCMatrixKindNull, nGCMatrixKindNull, 0x00 },
    efManagerNoEjectProcUpdate,
    gcDrawDObjDLHead1,
    0x0760, 0x0690, 0x0000, 0x0830
};

/* efManagerCaptainFalconPunchMakeEffect: Captain's hand (joint 16), or
 * joint 30 for a Kirby holding his power (get_punch_bone_). */
static GObj *ndsP4GanondorfPunchMakeEffect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;
    DObj *joint = ((fp->fkind == nFTKindCaptain) || (fp->fkind == nFTKindNCaptain)) ?
        fp->joints[16] : fp->joints[30];

    if ((gNdsP4GanondorfSpecial3 == NULL) || (joint == NULL))
    {
        gNdsP4GanondorfArticleMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectForce(&sNdsP4GanondorfPunchEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = joint;
    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(-90.0F);

    return effect_gobj;
}

/* ---- Warlock Kick (dEFManagerCaptainFalconKickEffectDesc) ---- */
static EFDesc sNdsP4GanondorfKickEffectDesc = {
    0x4 | EFFECT_FLAG_USERDATA,
    15,
    &gNdsP4GanondorfSpecial2,
    { 0x50, nGCMatrixKindRotRpyR, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    efManagerNoEjectProcUpdate,
    gcDrawDObjTreeForGObj,
    0x0B08, 0x0960, 0x0B90, 0x0C00
};

/* efManagerCaptainFalconKickMakeEffect: on joint 23, tilted in the aerial
 * kick. */
static GObj *ndsP4GanondorfKickMakeEffect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;

    if ((gNdsP4GanondorfSpecial2 == NULL) || (fp->joints[23] == NULL))
    {
        gNdsP4GanondorfArticleMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectForce(&sNdsP4GanondorfKickEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = fp->joints[23];
    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(90.0F);

    if (fp->status_id == nFTCaptainStatusSpecialAirLw)
    {
        dobj->rotate.vec.f.z = -fp->lr * F_CLC_DTOR32(60.0F);
    }
    return effect_gobj;
}

/* ---- Entry (dEFManagerCaptainEntryCarEffectDesc) ----
 *
 * The craft's tree at 0x5FC0, its flight at 0x6200 and the two child tracks
 * at 0x6518 and 0x6598, as efManagerCaptainEntryCarMakeEffect reads them. */
static EFDesc sNdsP4GanondorfEntryCarEffectDesc = {
    0x4 | EFFECT_FLAG_USERDATA,
    10,
    &gNdsP4GanondorfSpecial2,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    efManagerCaptainEntryCarProcUpdate,
    gcDrawDObjTreeDLLinksForGObj,
    0x5FC0, 0x0000, 0x0000, 0x0000
};

static GObj *ndsP4GanondorfEntryCarMakeEffect(Vec3f *pos, s32 lr)
{
    DObj *node_dobj;
    GObj *effect_gobj;
    DObj *dobj;
    s32 i;

    if (gNdsP4GanondorfSpecial2 == NULL)
    {
        gNdsP4GanondorfArticleMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectNoForce(&sNdsP4GanondorfEntryCarEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    gcAddAnimJointAll(effect_gobj,
        lbRelocGetFileData(AObjEvent32**, gNdsP4GanondorfSpecial2, 0x6200), 0.0F);

    node_dobj = ((dobj->child != NULL) && (dobj->child->child != NULL)) ?
        dobj->child->child->child : NULL;

    for (i = nFTPartsJointCommonStart; (i > 0) && (node_dobj != NULL); i--)
    {
        gcAddXObjForDObjFixed(node_dobj, nGCMatrixKindRecalcRotRpyRSca, 0);
        gcAddDObjAnimJoint(node_dobj,
            lbRelocGetFileData(AObjEvent32*, gNdsP4GanondorfSpecial2, 0x6518), 0.0F);

        node_dobj = node_dobj->sib_next;
        if (node_dobj == NULL)
        {
            break;
        }
        gcAddDObjAnimJoint(node_dobj,
            lbRelocGetFileData(AObjEvent32*, gNdsP4GanondorfSpecial2, 0x6598), 0.0F);

        node_dobj = node_dobj->sib_next;
    }
    if (i > 0)
    {
        gNdsP4GanondorfArticleMisses++;
    }
    gcPlayAnimAll(effect_gobj);

    dobj->translate.vec.f = *pos;

    if (lr == -1)
    {
        dobj->rotate.vec.f.y = F_CLC_DTOR32(180.0F);
    }
    if (DObjGetStruct(effect_gobj)->rotate.vec.f.y == F_CLC_DTOR32(0.0F))
    {
        efManagerSortZNeg(dobj->child);
    }
    else efManagerSortZPos(dobj->child);

    return effect_gobj;
}

/* The Falcon Flyer's case of ftCommonAppearSetStatus (his entry_script is
 * Captain's): arriving from the far side flips him, and the craft is his. */
static void ndsP4GanondorfEntryCase(FTStruct *fp)
{
    if (fp->status_vars.common.entry.lr == -1)
    {
        fp->status_vars.common.entry.is_rotate = TRUE;
    }
    ndsP4GanondorfEntryCarMakeEffect(&fp->entry_pos, fp->status_vars.common.entry.lr);
}

const NDSP4Overrides gNdsP4GanondorfOverrides = {
    .captain_falcon_punch = ndsP4GanondorfPunchMakeEffect,
    .captain_falcon_kick = ndsP4GanondorfKickMakeEffect,
    .entry_case = ndsP4GanondorfEntryCase,
};

#endif /* NDS_P4_GANONDORF */
