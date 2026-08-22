#include <ef/effect.h>
#include <ft/fighter.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* BattleShip ftcommon.h:20. The broad decomp header is intentionally not part
 * of the port ABI mirror, so keep the one source constant this bounded import
 * needs beside the code that consumes it. */
#define NDS_FTCOMMON_ENTRY_WAIT 120

GObj *efManagerMarioEntryDokanMakeEffect(Vec3f *pos, s32 fkind);
GObj *efManagerFoxEntryArwingMakeEffect(Vec3f *pos, s32 lr);
GObj *efManagerDonkeyEntryTaruMakeEffect(Vec3f *pos);
void ndsEFManagerRetryDeferredDescs(void);

/* P2-2 normal-match entry parity.
 *
 * The full decomp ftcommonentry.c contains the entry status IDs and helpers for
 * every fighter in SSB64. P2-2 deliberately ships only Mario and Fox fighter
 * data, so pulling the whole TU into the DS ABI mirror would make unsupported
 * Donkey/Samus/Link/... status enums and character entry routines link
 * requirements of a Mario/Fox-only build. The functions below are the source
 * bodies for the common path, narrowed only where the source switches on fkind:
 * Mario and Fox keep their exact status IDs/effects; an unexpected kind falls
 * back to EntryNull instead of fabricating an unsupported character sequence.
 * When P2-3 brings another fighter live, its source branch belongs here at the
 * same time as its status table/assets.
 *
 * Source authority: decomp/src/ft/ftcommon/ftcommonentry.c:49-268. */

void ftCommonEntrySetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusEntry, 0.0F, 1.0F,
                    FTSTATUS_PRESERVE_NONE);

    fp->is_invisible = TRUE;
    fp->is_shadow_hide = TRUE;
    fp->is_ghost = TRUE;
    fp->is_playertag_hide = TRUE;
}

void ftCommonEntryNullProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->status_vars.common.entry.entry_wait != 0)
    {
        fp->status_vars.common.entry.entry_wait--;

        if (fp->status_vars.common.entry.entry_wait == 0)
        {
            /* P2-2 has no Boss fighter; this is the source's non-Boss arm. */
            fp->lr = fp->status_vars.common.entry.lr;
            DObjGetStruct(fighter_gobj)->translate.vec.f = fp->entry_pos;
            fp->coll_data.floor_line_id =
                fp->status_vars.common.entry.floor_line_id;
            ftCommonWaitSetStatus(fighter_gobj);
        }
    }
}

static void ndsFTCommonAppearUpdateEffectsMarioFox(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        /* In the source only Pikachu/Purin and their polygon variants spawn
         * Master-Ball rays for flag1. Mario/Fox therefore only consume it. */
        fp->motion_vars.flags.flag1 = 0;
    }
    if (fp->motion_vars.flags.flag2 != 0)
    {
        fp->motion_vars.flags.flag2 = 0;
        fp->is_shadow_hide = FALSE;
    }
}

void ftCommonAppearProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsFTCommonAppearUpdateEffectsMarioFox(fighter_gobj);

    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->lr = fp->status_vars.common.entry.lr;
        DObjGetStruct(fighter_gobj)->translate.vec.f = fp->entry_pos;
        fp->coll_data.floor_line_id = fp->status_vars.common.entry.floor_line_id;
        ftCommonWaitSetStatus(fighter_gobj);
    }
}

void ftCommonAppearProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *topn_joint = fp->joints[nFTPartsJointTopN];
    DObj *transn_joint = fp->joints[nFTPartsJointTransN];

    topn_joint->translate.vec.f.y =
        fp->entry_pos.y + transn_joint->translate.vec.f.y;

    if (fp->status_vars.common.entry.is_rotate != FALSE)
    {
        topn_joint->translate.vec.f.x =
            fp->entry_pos.x - transn_joint->translate.vec.f.x;
        topn_joint->translate.vec.f.z =
            fp->entry_pos.z - transn_joint->translate.vec.f.z;
        topn_joint->rotate.vec.f.y = F_CST_DTOR32(180.0F);
    }
    else
    {
        topn_joint->translate.vec.f.x =
            fp->entry_pos.x + transn_joint->translate.vec.f.x;
        topn_joint->translate.vec.f.z =
            fp->entry_pos.z + transn_joint->translate.vec.f.z;
    }
}

static void ndsFTCommonAppearInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_ghost = TRUE;
    fp->camera_mode = nFTCameraModeEntry;
    fp->is_shadow_hide = TRUE;
    fp->is_playertag_hide = TRUE;
}

void ftCommonAppearSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id;
    s32 entry_id;

    /* Effects are initialized before fighter-special files in VSBattle. The DS
     * effect resolver defers those descriptors, so retry at the same seam
     * immediately before the source entry effect is requested. */
    ndsEFManagerRetryDeferredDescs();

    entry_id = (fp->lr == +1) ? 0 : 1;
    fp->entry_pos = DObjGetStruct(fighter_gobj)->translate.vec.f;
    fp->status_vars.common.entry.is_rotate = FALSE;
    fp->status_vars.common.entry.lr = fp->lr;
    fp->lr = 0;
    fp->status_vars.common.entry.floor_line_id = fp->coll_data.floor_line_id;

    if (fp->fkind == nFTKindMario)
    {
        status_id = (entry_id == 0) ? nFTMarioStatusAppearR :
                                      nFTMarioStatusAppearL;
        efManagerMarioEntryDokanMakeEffect(&fp->entry_pos, fp->fkind);
    }
    else if (fp->fkind == nFTKindFox)
    {
        status_id = (entry_id == 0) ? nFTFoxStatusAppearR :
                                      nFTFoxStatusAppearL;
        efManagerFoxEntryArwingMakeEffect(
            &fp->entry_pos, fp->status_vars.common.entry.lr);
    }
#if NDS_P2_DONKEY
    else if (fp->fkind == nFTKindDonkey)
    {
        /* BattleShip ftcommonentry.c:15,204-207. DK owns distinct Appear
         * statuses and the Special2 barrel entry; using EntryNull here leaves
         * the source's initial invisibility latched for the whole match. */
        status_id = (entry_id == 0) ? nFTDonkeyStatusAppearR :
                                      nFTDonkeyStatusAppearL;
        efManagerDonkeyEntryTaruMakeEffect(&fp->entry_pos);
    }
#endif
    else
    {
        /* Unsupported in P2-2. Source uses EntryNull for polygon fighters;
         * using it here fails safely without inventing another fighter's entry. */
        status_id = nFTCommonStatusEntryNull;
    }

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    ndsFTCommonAppearInitStatusVars(fighter_gobj);

    fp->status_vars.common.entry.entry_wait = NDS_FTCOMMON_ENTRY_WAIT;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
    fp->motion_vars.flags.flag0 = 0;
}
