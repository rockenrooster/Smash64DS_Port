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
#if NDS_P2_SAMUS
GObj *efManagerSamusEntryPointMakeEffect(Vec3f *pos);
#endif
#if NDS_P2_LINK
GObj *efManagerLinkEntryWaveMakeEffect(Vec3f *pos);
GObj *efManagerLinkEntryBeamMakeEffect(Vec3f *pos);
#endif
#if NDS_P2_CAPTAIN
/* Already compiled in battleship_efmanager.o and dropped by --gc-sections for
 * want of a caller; it links the moment Falcon's branch below calls it. */
GObj *efManagerCaptainEntryCarMakeEffect(Vec3f *pos, s32 lr);
void ftCaptainAppearEndSetStatus(GObj *fighter_gobj);
#endif
void ndsEFManagerRetryDeferredDescs(void);

/* NDS_PARTIAL_IMPORT: decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonentry.c
 *
 * Registered in scripts/check-architecture.ps1. Importing the TU whole would
 * make every unlanded fighter's Appear status enums (nFTSamus*, nFTLink*,
 * nFTYoshi*, nFTKirby*, nFTPikachu*, nFTPurin*, nFTNess*, nFTBoss*) and nine
 * entry-effect makers link requirements of a build that has none of them, and
 * would index status tables that are 16-entry stubs. Landed kinds carry their
 * exact source branch; every new production fighter adds its branch HERE as it
 * lands, at the same time as its status table and assets.
 *
 * P2-2 normal-match entry parity.
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

static void ndsFTCommonAppearUpdateEffectsNoMBall(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        /* Source ftCommonAppearUpdateEffects (ftcommonentry.c:91) with its one
         * fkind test resolved: only Pikachu/Purin and their polygon variants
         * spawn Master-Ball rays for flag1, so every landed kind -- Mario, Fox,
         * Luigi, Donkey, Captain -- takes this arm and only consumes it. The
         * name said MarioFox until P2-3f5; it was never Mario/Fox-specific.
         * Samus also takes this exact consume-only arm. */
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

    ndsFTCommonAppearUpdateEffectsNoMBall(fighter_gobj);

    /* Source-faithful end check. The figatree bind/play seam now publishes the
     * first live Appear frame before this callback runs (measured Mario: 1.0 on
     * its first pre-GO update), so no DS-only bind-tick guard belongs here.
     * `status_total_tics` cannot be such a guard anyway: BattleShip deliberately
     * leaves it at zero while player control is locked, which is the entire
     * pre-GO window. Gating on it therefore held Appear until GO and made the
     * fighters look frozen instead of entering their idle Wait animations. */
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

    if ((fp->fkind == nFTKindMario)
#if NDS_P2_LUIGI
        /* BattleShip ftcommonentry.c:20 and :192-196. Luigi's row of
         * `dFTCommonEntryAppearStatusIDs` IS `{ nFTMarioStatusAppearR,
         * nFTMarioStatusAppearL }` -- those are status INDICES into the
         * fighter's own special table, and Luigi's real nine-entry table
         * (`dFTLuigiSpecialStatusDescs`, live under NDS_P2_LUIGI) carries
         * AppearR/AppearL at exactly those two indices with his own
         * `nFTLuigiMotionAppearR/L` scripts. The source's Dokan switch arm is
         * `case nFTKindMario: case nFTKindLuigi: case nFTKindMMario:` and the
         * maker itself selects the file: `efManagerMarioEntryDokanMakeEffect`
         * (efmanager.c:5669-5682) points the effect desc at
         * `gFTDataLuigiSpecial2` for Luigi and `gFTMarioFileSpecial2` for
         * Mario, which is why it takes fkind at all.
         *
         * Metal Mario shares this arm in the source and is deliberately NOT
         * added: he is P2-6 content with no status table here, so he keeps the
         * EntryNull fallback below until his row lands. */
        || (fp->fkind == nFTKindLuigi)
#endif
        )
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
#if NDS_P2_SAMUS
    else if (fp->fkind == nFTKindSamus)
    {
        /* BattleShip ftcommonentry.c:16,209-211. Samus owns her own
         * AppearR/AppearL status pair and the Special2 entry-point effect.
         * EntryNull is not source-equivalent here: it suppresses both the
         * status motion and the entry-point effect. */
        status_id = (entry_id == 0) ? nFTSamusStatusAppearR :
                                      nFTSamusStatusAppearL;
        efManagerSamusEntryPointMakeEffect(&fp->entry_pos);
    }
#endif
#if NDS_P2_LINK
    else if (fp->fkind == nFTKindLink)
    {
        /* BattleShip ftcommonentry.c:213-215. Link owns his Appear pair and
         * spawns both the entry wave and entry beam from LinkSpecial2. */
        status_id = (entry_id == 0) ? nFTLinkStatusAppearR :
                                      nFTLinkStatusAppearL;
#if !NDS_P2_LINK_BOMB_TOUR
        efManagerLinkEntryWaveMakeEffect(&fp->entry_pos);
        efManagerLinkEntryBeamMakeEffect(&fp->entry_pos);
#else
        /* The item-lifecycle proof starts Link from Wait and owns no entry
         * claim. Keep the two independent generic entry animations out of
         * that proof; ordinary Link builds still spawn both source effects. */
#endif
    }
#endif
#if NDS_P2_CAPTAIN
    else if (fp->fkind == nFTKindCaptain)
    {
        /* BattleShip ftcommonentry.c:20,231-238,264-267. FALCON IS THE ONLY
         * FIGHTER WHOSE ENTRY IS A TWO-STATUS LADDER: everyone else gets one
         * Appear status, `dFTCommonEntryAppearStatusIDs` gives Falcon
         * AppearRStart/AppearLStart, and ftCaptainAppearStartProcUpdate hands
         * off to AppearREnd/AppearLEnd below.
         *
         * Two more Falcon-only things live in this function, and both are
         * about the Falcon Flyer arriving from the far side of the stage:
         * `lr == -1` sets is_rotate, which ftCommonAppearProcPhysics turns into
         * the 180-degree TopN flip, and the ftParamMoveDLLink(gobj, 1) tail
         * below moves him to a different display list until the car has passed
         * z > -1000 (ftCaptainAppearStartProcUpdate moves him back). */
        status_id = (entry_id == 0) ? nFTCaptainStatusAppearRStart :
                                      nFTCaptainStatusAppearLStart;
        if (fp->status_vars.common.entry.lr == -1)
        {
            fp->status_vars.common.entry.is_rotate = TRUE;
        }
        efManagerCaptainEntryCarMakeEffect(
            &fp->entry_pos, fp->status_vars.common.entry.lr);
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
#if NDS_P2_CAPTAIN
    if ((fp->fkind == nFTKindCaptain) &&
        (fp->status_vars.common.entry.lr == -1))
    {
        ftParamMoveDLLink(fighter_gobj, 1);
    }
#endif
}

#if NDS_P2_CAPTAIN
/* BattleShip ftcommonentry.c:321-343. The Appear-Start half of Falcon's ladder
 * and the hand-off to his Appear-End statuses. The source puts both here rather
 * than in ft/ftchar/ftcaptain, so they land with the entry ladder they belong
 * to and not in battleship_captain.c. */
void ftCaptainAppearStartProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsFTCommonAppearUpdateEffectsNoMBall(fighter_gobj);

    if ((fp->status_vars.common.entry.lr == -1) &&
        (fp->dl_link != FTDISPLAY_DLLINK_DEFAULT) &&
        (DObjGetStruct(fighter_gobj)->translate.vec.f.z > -1000.0F))
    {
        ftParamMoveDLLink(fighter_gobj, FTDISPLAY_DLLINK_DEFAULT);
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ftCaptainAppearEndSetStatus);
}

void ftCaptainAppearEndSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj,
                    ((fp->status_vars.common.entry.lr == +1) ?
                         nFTCaptainStatusAppearREnd :
                         nFTCaptainStatusAppearLEnd),
                    0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ndsFTCommonAppearInitStatusVars(fighter_gobj);

    fp->is_shadow_hide = FALSE;
}
#endif
