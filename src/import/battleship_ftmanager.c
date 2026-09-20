/*
 * Fenced whole BattleShip ft/ftmanager.c import.
 *
 * Default builds keep the current DS manager seam. Set
 * NDS_IMPORT_BATTLESHIP_FTMANAGER=1 to compile and prove the original manager
 * path against the FTData/status-buffer asset slice.
 */
#include <ft/fighter.h>
#include <reloc_data.h>
#include <string.h>
#include <sys/debug.h>
#include <sys/objman.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/*
 * Keep this fenced import on the port's narrow headers. The original
 * lb/library.h pulls broad gm/lb/ft headers that conflict with the active port
 * ABI shadows; reloc_data.h supplies the one macro ftmanager.c needs.
 */
#include <nds/nds_ft_pose.h>
#include <nds/nds_effects.h>
#include <nds/nds_renderer.h>
#include <nds/nds_preview_pack.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_shield_pose.h>
#include <nds/generated/nds_fighter_production.generated.h>
#include <nds/generated/nds_native_fighter_image.generated.h>

#ifndef _LIBRARY_H_
#define _LIBRARY_H_
#endif

#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

#define ftManagerSetupFileSize ndsBaseFTManagerSetupFileSize
#define ftManagerSetupFilesAllKind ndsBaseFTManagerSetupFilesAllKind
#define ftManagerMakeFighter ndsBaseFTManagerMakeFighter
#define ftManagerDestroyFighter ndsBaseFTManagerDestroyFighter

void ndsBaseFTManagerSetupFileSize(void);
void ndsBaseFTManagerSetupFilesAllKind(s32 fkind);
GObj *ndsBaseFTManagerMakeFighter(FTDesc *desc);
void ndsBaseFTManagerDestroyFighter(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftmanager.c"

#undef ftManagerSetupFileSize
#undef ftManagerSetupFilesAllKind
#undef ftManagerMakeFighter
#undef ftManagerDestroyFighter

static const FTFileSize sNdsFTManagerSourceFileSizes[nFTKindEnumCount] =
{
#define NDS_FTMANAGER_FILE_SIZE_ROW(kind_, main_, mainmotion_, submotion_) \
    [kind_] = { (main_), (mainmotion_), (submotion_) },
    NDS_FTMANAGER_FILE_SIZE_CENSUS_ROWS(NDS_FTMANAGER_FILE_SIZE_ROW)
#undef NDS_FTMANAGER_FILE_SIZE_ROW
};

_Static_assert(NDS_FTMANAGER_FILE_SIZE_CENSUS_COUNT == nFTKindEnumCount,
               "generated ftManager file-size census must cover the source roster");

void ftManagerSetupFileSize(void)
{
    /* BattleShip computes these exact immutable answers from its ROM reloc
     * table before any fighter files are resident.  The DS production
     * generator performs the same source table walk against the pinned US O2Rs
     * at build time, including mainmotion_array_count and shield-pose exclusion.
     * Copying the result here preserves the source contract while deleting a
     * DS-only startup pass through thousands of address-token classifications
     * and filesystem metadata lookups. */
    memcpy(gSCManagerFighterFileSizes, sNdsFTManagerSourceFileSizes,
           sizeof(sNdsFTManagerSourceFileSizes));
}

#if NDS_P2_1P_GAME
/* Campaign wave preload: Zako waves replace the fighter mid-battle
 * (sc1PGameSpawnEnemyTeamNext picks the next N-kind variation and calls
 * ftManagerMakeFighter, with either detail by fighter count), and wave
 * replacement is gameplay, not load time -- a NitroFS read there would
 * stall the battle. The source already preloads every N kind's files at
 * scene start (sc1pgame.c Zako branch calls ftManagerSetupFilesAllKind
 * for NStart..NEnd); preload the matching native images here, both
 * details, so every later Ensure is a residency hit. NLuigi reuses the
 * NMario packet and needs no slot of its own. Metal Mario's own stage
 * preloads its single owner the same way. Other stages never create
 * these kinds, so nothing else pays. Each line is additionally gated on
 * its image flag or its verification profile, so both paths load before
 * gameplay. Pure static control builds need no image. Results
 * are intentionally ignored: fighter creation re-ensures anyway. */
static void ndsFTManagerPreloadVariantOwnerImages(void)
{
    /* The campaign's stage field persists through menus and VS matches. */
    if (gSCManagerSceneData.scene_curr != nSCKind1PGame)
    {
        return;
    }
    if (gSCManagerSceneData.spgame_stage == (u8)nSC1PGameStageZako)
    {
#if NDS_NATIVE_OWNER_IMAGE_NMARIO || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NMARIO)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NMARIO, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NMARIO, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NMARIO, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NMARIO, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NFOX || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NFOX)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NFOX, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NFOX, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NFOX, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NFOX, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NDONKEY || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NDONKEY)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NDONKEY, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NDONKEY, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NDONKEY, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NDONKEY, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NSAMUS || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NSAMUS)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NSAMUS, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NSAMUS, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NSAMUS, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NSAMUS, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NLINK || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NLINK)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NLINK, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NLINK, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NLINK, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NLINK, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NYOSHI || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NYOSHI)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NYOSHI, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NYOSHI, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NYOSHI, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NYOSHI, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NCAPTAIN || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NCAPTAIN)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NCAPTAIN, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NCAPTAIN, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NCAPTAIN, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NCAPTAIN, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NKIRBY || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NKIRBY)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NKIRBY, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NKIRBY, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NKIRBY, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NKIRBY, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NPIKACHU || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NPIKACHU)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPIKACHU, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPIKACHU, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPIKACHU, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPIKACHU, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NPURIN || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NPURIN)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPURIN, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPURIN, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPURIN, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NPURIN, 1u);
#endif
#endif
#if NDS_NATIVE_OWNER_IMAGE_NNESS || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_NNESS)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NNESS, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_NNESS, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NNESS, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_NNESS, 1u);
#endif
#endif
    }
    if (gSCManagerSceneData.spgame_stage == (u8)nSC1PGameStageMMario)
    {
#if NDS_NATIVE_OWNER_IMAGE_MMARIO || (NDS_NATIVE_OWNER_IMAGE_VERIFY && NDS_P2_MMARIO)
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_MMARIO, 0u);
        (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_MMARIO, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_MMARIO, 0u);
        (void)ndsRendererNativeVerifyOwnerImage(NDS_NATIVE_IMAGE_SLOT_MMARIO, 1u);
#endif
#endif
    }
}
#endif

#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
static void ndsFTManagerSetupCompactBattleFilesKind(s32 fkind)
{
    FTData *data = dFTManagerDataFiles[fkind];

    /* BattleShip ftManagerSetupFilesKind owns these exact post-Main loads.
     * The compact FPC has already supplied Main and Model, so loading Model a
     * second time would throw away the arena saving this path is proving. */
    if (data->file_mainmotion_id != 0)
    {
        *data->p_file_mainmotion =
            lbRelocGetStatusBufferFile(data->file_mainmotion_id);
    }
    if (data->file_submotion_id != 0)
    {
        *data->p_file_submotion =
            lbRelocGetStatusBufferFile(data->file_submotion_id);
    }
    if (data->file_shieldpose_id != 0)
    {
        /* The native guard package owns the complete ShieldPose source file
         * for its migrated base fighters.  The generator refuses this path if
         * either source motion table ever gains FTANIM_FLAG_SHIELDPOSE, and the
         * FPC loader has already restored Main's nine guard pointers.  Keeping
         * the raw source file here would duplicate the exact residency P2-2 is
         * removing. */
        if (ndsShieldPoseReplacesSourceFile(fkind) != FALSE)
        {
            data->p_file_shieldpose = NULL;
        }
        else
        {
            data->p_file_shieldpose =
                lbRelocGetStatusBufferFile(data->file_shieldpose_id);
        }
    }
    if (data->file_special1_id != 0)
    {
        *data->p_file_special1 =
            lbRelocGetStatusBufferFile(data->file_special1_id);
    }
    if (data->file_special2_id != 0)
    {
        *data->p_file_special2 =
            lbRelocGetStatusBufferFile(data->file_special2_id);
    }
    if (data->file_special3_id != 0)
    {
        *data->p_file_special3 =
            lbRelocGetStatusBufferFile(data->file_special3_id);
    }
    if (data->file_special4_id != 0)
    {
        *data->p_file_special4 =
            lbRelocGetStatusBufferFile(data->file_special4_id);
    }
}
#endif

void ftManagerSetupFilesAllKind(s32 fkind)
{
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
    s32 preview = ndsRelocLoadPreviewFighter(fkind);
    if (preview != FALSE)
    {
        FTData *data = dFTManagerDataFiles[fkind];
#if NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        if ((preview == 2) &&
            (ndsRelocUseBattleCoreFighterData() != FALSE))
        {
            /* FPC1 publishes Main before the source's separately resident
             * MainMotion/special/article dependencies exist.  Recreate the
             * original Main extern closure first; this also loads any direct
             * Main dependency (for example LinkBoomerangModel) that is not one
             * of FTData's named special slots. */
            (void)ndsRelocPatchCompactBattleMainExterns(fkind);
            ndsFTManagerSetupCompactBattleFilesKind(fkind);
        }
#endif
        /* Preserve ftmanager.c's guarded bank creation/publication. Demo's
         * event scripts and per-status figatree loads remain the source path. */
        if ((preview == 2) && (data->particles_script_lo != 0))
        {
            *data->p_particle = efParticleGetLoadBankID(
                data->particles_script_lo, data->particles_script_hi,
                data->particles_texture_lo, data->particles_texture_hi);
            *data->p_particle = efParticleGetBankID(data->particles_script_lo);
        }
        ndsEFManagerRetryDeferredDescs();
        return;
    }
#endif
    /* BattleShip's source contract is still the loader: when the fighter main
     * file is absent it loads main plus the model/motion/special closure in one
     * operation (ftmanager.c:352-360). The DS effect table is initialized
     * earlier, though, so descriptors backed by those fighter-special files are
     * deliberately neutralised until the file becomes resident. Retry exactly
     * after the source load boundary. This is generic residency plumbing, not a
     * Captain exception: Fox reflector, DK/Samus/Link entries and Falcon's
     * EntryCar/Kick/Punch all use the same deferred-desc contract. */
    ndsBaseFTManagerSetupFilesAllKind(fkind);
    ndsEFManagerRetryDeferredDescs();
#if NDS_P2_1P_GAME
    /* Scene-start preload for wave-replaced variant owners; no-op by
     * residency on repeats and on stages that never use them. */
    ndsFTManagerPreloadVariantOwnerImages();
#endif
}

__attribute__((used)) volatile u32 gNdsFTManagerFigatreeSlotKindCount;
__attribute__((used)) volatile u32 gNdsFTManagerFigatreeSlotKindBytes;
__attribute__((used)) volatile u32 gNdsFTManagerFigatreeSlotKindMin;

#if NDS_P2_KIRBY && NDS_P2_MENU_SHELL
/* BattleShip's Kirby constructor needs only the copy table at offset zero of
 * KirbyMainMotion while building a CSS preview (ftmanager.c:632-634). The DS
 * compact preview pack deliberately omits the rest of MainMotion. Publish the
 * exact source table as an unregistered reloc base: ndsRelocGetFileData() then
 * returns it directly, preserving the source lookup without paying for the
 * complete motion file. Source: relocData/228_KirbyMainMotion.c:132-160. */
static FTKirbyCopy sNdsFTManagerKirbyPreviewCopyTable[27] = {
    { nFTKindMario,   12, 1.5F, 17 },
    { nFTKindFox,      7, 1.5F, 17 },
    { nFTKindDonkey,   4, 2.0F, 30 },
    { nFTKindSamus,    8, 1.6F, 17 },
    { nFTKindLuigi,   11, 1.6F, 17 },
    { nFTKindLink,    10, 1.5F, 17 },
    { nFTKindYoshi,    5, 1.7F, 25 },
    { nFTKindCaptain,  9, 1.7F, 17 },
    { nFTKindKirby,    0, 1.6F, 17 },
    { nFTKindPikachu,  6, 1.5F, 17 },
    { nFTKindPurin,    3, 1.6F, 17 },
    { nFTKindNess,    13, 1.6F, 17 },
    { nFTKindKirby,    0, 1.0F, 17 },
    { nFTKindKirby,    0, 1.5F, 17 },
    { nFTKindKirby,    0, 1.5F, 17 },
    { nFTKindKirby,    0, 1.5F, 17 },
    { nFTKindKirby,    0, 2.0F, 30 },
    { nFTKindKirby,    0, 1.6F, 17 },
    { nFTKindKirby,    0, 1.6F, 17 },
    { nFTKindKirby,    0, 1.5F, 17 },
    { nFTKindKirby,    0, 1.7F, 17 },
    { nFTKindKirby,    0, 1.7F, 17 },
    { nFTKindKirby,    0, 1.6F, 17 },
    { nFTKindKirby,    0, 1.5F, 17 },
    { nFTKindKirby,    0, 1.6F, 17 },
    { nFTKindKirby,    0, 1.6F, 17 },
    { nFTKindDonkey,   4, 2.0F, 50 },
};
static void *sNdsFTManagerKirbyPreviewMainMotionSaved;
static sb32 sNdsFTManagerKirbyPreviewMainMotionActive;
#endif

void ndsFTManagerRestoreKirbyPreviewMainMotion(void)
{
#if NDS_P2_KIRBY && NDS_P2_MENU_SHELL
    if (sNdsFTManagerKirbyPreviewMainMotionActive != FALSE)
    {
        if (gFTDataKirbyMainMotion == sNdsFTManagerKirbyPreviewCopyTable)
        {
            gFTDataKirbyMainMotion = sNdsFTManagerKirbyPreviewMainMotionSaved;
        }
        sNdsFTManagerKirbyPreviewMainMotionSaved = NULL;
        sNdsFTManagerKirbyPreviewMainMotionActive = FALSE;
    }
#endif
}

/* The ~7 KB electric body is only worth its arena when someone in the match
 * can land an electric hit. Any other electric source falls back to the
 * source's no-skeleton flash (ndsFTManagerSkeletonReady below), never to a
 * rejected draw. */
static sb32 ndsFTManagerMatchHasElectricAttacker(void)
{
    s32 player;

    if (gSCManagerBattleState == NULL)
    {
        return FALSE;
    }
    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        s32 fkind = gSCManagerBattleState->players[player].fkind;

        if ((gSCManagerBattleState->players[player].pkind != nFTPlayerKindNot) &&
            ((fkind == nFTKindPikachu) || (fkind == nFTKindNPikachu) ||
             (fkind == nFTKindNess) || (fkind == nFTKindNNess) ||
             (fkind == nFTKindKirby) || (fkind == nFTKindNKirby)))
        {
            return TRUE;
        }
    }
    return FALSE;
}

sb32 ndsFTManagerSkeletonReady(s32 fkind)
{
    if (fkind == nFTKindMario)
        return ndsRendererNativeOwnerImageResident(NDS_NATIVE_IMAGE_SLOT_MARIO_SKELETON1, 0u);
    if (fkind == nFTKindFox)
        return ndsRendererNativeOwnerImageResident(NDS_NATIVE_IMAGE_SLOT_FOX_SKELETON1, 0u);
    return FALSE;
}

void ndsFTManagerEnsureOwnerImages(FTDesc *desc)
{
    /* Electric bodies share one image across HIGH/LOW; load at construction,
     * never when a hit first selects the alternate skeleton. Results
     * fighters cannot be hit, and that scene has no arena room to spare. */
    if (desc != NULL && desc->pkind != nFTPlayerKindDemo &&
        gSCManagerSceneData.scene_curr != nSCKindVSResults &&
        ndsFTManagerMatchHasElectricAttacker() != FALSE)
    {
        if (desc->fkind == nFTKindMario)
            (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_MARIO_SKELETON1, 0u);
        if (desc->fkind == nFTKindFox)
            (void)ndsRendererNativeEnsureOwnerImage(NDS_NATIVE_IMAGE_SLOT_FOX_SKELETON1, 0u);
    }
#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_GDONKEY || NDS_P2_MMARIO || NDS_P2_NMARIO || NDS_P2_NFOX || NDS_P2_NDONKEY || NDS_P2_NSAMUS || NDS_P2_NLUIGI || NDS_P2_NLINK || NDS_P2_NYOSHI || NDS_P2_NCAPTAIN || NDS_P2_NKIRBY || NDS_P2_NPIKACHU || NDS_P2_NPURIN || NDS_P2_NNESS || NDS_P2_1P_GAME
    /* P2-3r4. A P2-3 fighter's generated geometry lives in a NitroFS image, so
     * it has to be resident before anything can draw this fighter. HERE is the
     * right seam: fighter creation is load-time work in every caller (battle
     * setup and the character select's preview rebuild), while a lazy load from
     * the draw path would be a NitroFS read inside a frame -- the exact stall
     * that cost the BGM its seam on the character select.
     *
     * Low-detail battles also prepare the high-detail KO/pause view. Other
     * display scenes retain both unless their source fixes a single detail. */
    if (desc != NULL)
    {
        u32 image_slot = NDS_NATIVE_IMAGE_OWNER_SLOTS;

#if NDS_P2_LUIGI
        if (desc->fkind == nFTKindLuigi)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_LUIGI;
        }
#endif
#if NDS_P2_DONKEY
        if (desc->fkind == nFTKindDonkey)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_DONKEY;
        }
#endif
#if NDS_P2_CAPTAIN
        if (desc->fkind == nFTKindCaptain)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_CAPTAIN;
        }
#endif
#if NDS_P2_SAMUS
        if (desc->fkind == nFTKindSamus)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_SAMUS;
        }
#endif
#if NDS_P2_LINK
        if (desc->fkind == nFTKindLink)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_LINK;
        }
#endif
#if NDS_P2_PIKACHU
        if (desc->fkind == nFTKindPikachu)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_PIKACHU;
        }
#endif
#if NDS_P2_YOSHI
        if (desc->fkind == nFTKindYoshi)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_YOSHI;
        }
#endif
#if NDS_P2_NESS
        if (desc->fkind == nFTKindNess)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NESS;
        }
#endif
#if NDS_P2_PURIN
        if (desc->fkind == nFTKindPurin)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_PURIN;
        }
#endif
#if NDS_P2_KIRBY
        if (desc->fkind == nFTKindKirby)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_KIRBY;
        }
#endif
#if NDS_P2_MMARIO
        if (desc->fkind == nFTKindMMario)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_MMARIO;
        }
#endif
#if NDS_P2_NMARIO
        if (desc->fkind == nFTKindNMario)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NMARIO;
        }
#endif
#if NDS_P2_NFOX
        if (desc->fkind == nFTKindNFox)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NFOX;
        }
#endif
#if NDS_P2_NDONKEY
        if (desc->fkind == nFTKindNDonkey)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NDONKEY;
        }
#endif
#if NDS_P2_NSAMUS
        if (desc->fkind == nFTKindNSamus)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NSAMUS;
        }
#endif
#if NDS_P2_NLINK
        if (desc->fkind == nFTKindNLink)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NLINK;
        }
#endif
#if NDS_P2_NYOSHI
        if (desc->fkind == nFTKindNYoshi)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NYOSHI;
        }
#endif
#if NDS_P2_NCAPTAIN
        if (desc->fkind == nFTKindNCaptain)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NCAPTAIN;
        }
#endif
#if NDS_P2_NKIRBY
        if (desc->fkind == nFTKindNKirby)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NKIRBY;
        }
#endif
#if NDS_P2_NPIKACHU
        if (desc->fkind == nFTKindNPikachu)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NPIKACHU;
        }
#endif
#if NDS_P2_NPURIN
        if (desc->fkind == nFTKindNPurin)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NPURIN;
        }
#endif
#if NDS_P2_NNESS
        if (desc->fkind == nFTKindNNess)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_NNESS;
        }
#endif
#if NDS_P2_1P_GAME
        if (desc->fkind == nFTKindBoss)
        {
            image_slot = NDS_NATIVE_IMAGE_SLOT_BOSS;
        }
#endif
#if NDS_P2_NLUIGI
        if (desc->fkind == nFTKindNLuigi)
        {
            /* Reuses the NMario image packet. */
            image_slot = NDS_NATIVE_IMAGE_SLOT_NMARIO;
        }
#endif
#if NDS_P2_GDONKEY
        if (desc->fkind == nFTKindGDonkey)
        {
            /* Reuses the Donkey image packet. */
            image_slot = NDS_NATIVE_IMAGE_SLOT_DONKEY;
        }
#endif
        if (image_slot < NDS_NATIVE_IMAGE_OWNER_SLOTS)
        {
            u32 first_detail = 0u;
            /* High-detail VS and Results fighters never select low detail.
             * AutoDemo switches both ways, so it still prepares both. */
            u32 last_detail =
                (((gSCManagerSceneData.scene_curr == nSCKindVSBattle) ||
                  (gSCManagerSceneData.scene_curr == nSCKindVSResults)) &&
                 (desc->detail == nFTPartsDetailHigh)) ? 0u : 1u;
            u32 detail;

#if NDS_P2_1P_GAME
            /* These source display scenes keep each actor's chosen detail.
             * CSS uses HIGH; the intro explicitly chooses LOW for some team
             * opponents. Neither update changes detail, so load only the
             * requested image. */
            if (((gSCManagerSceneData.scene_curr == nSCKind1PGamePlayers) ||
                 (gSCManagerSceneData.scene_curr == nSCKind1PIntro)) &&
                (desc->pkind == nFTPlayerKindDemo))
            {
                first_detail = last_detail =
                    (desc->detail == nFTPartsDetailLow) ? 1u : 0u;
            }
#endif
            for (detail = first_detail; detail <= last_detail; detail++)
            {
                (void)ndsRendererNativeEnsureOwnerImage(image_slot, detail);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
                (void)ndsRendererNativeVerifyOwnerImage(image_slot, detail);
#endif
            }
        }
    }
#else
    (void)desc;
#endif
}

GObj *ftManagerMakeFighter(FTDesc *desc)
{
    ndsFTManagerEnsureOwnerImages(desc);
    if ((desc != NULL) && (desc->figatree_heap != NULL) &&
        (desc->fkind >= 0) && (desc->fkind < nFTKindEnumCount) &&
        (dFTManagerDataFiles[desc->fkind] != NULL))
    {
        u32 expect = (u32)dFTManagerDataFiles[desc->fkind]->file_anim_size;

        gNdsFTManagerFigatreeSlotKindCount++;
        gNdsFTManagerFigatreeSlotKindBytes += expect;
        if ((gNdsFTManagerFigatreeSlotKindMin == 0u) ||
            (expect < gNdsFTManagerFigatreeSlotKindMin))
        {
            gNdsFTManagerFigatreeSlotKindMin = expect;
        }
    }
#if NDS_P2_KIRBY && NDS_P2_MENU_SHELL
    if ((desc != NULL) && (desc->fkind == nFTKindKirby) &&
        ((gSCManagerSceneData.scene_curr == nSCKindPlayersVS) ||
         (gSCManagerSceneData.scene_curr == nSCKind1PGamePlayers)) &&
        (gFTDataKirbyMainMotion == NULL))
    {
        sNdsFTManagerKirbyPreviewMainMotionSaved = gFTDataKirbyMainMotion;
        sNdsFTManagerKirbyPreviewMainMotionActive = TRUE;
        gFTDataKirbyMainMotion = sNdsFTManagerKirbyPreviewCopyTable;
    }
#endif
    return ndsBaseFTManagerMakeFighter(desc);
}

/* P2-3. THE POSE SLOT IS PART OF THE FIGHTER, so it has to die with it.
 *
 * `ndsFtPoseBindBegin` claims one of a small fixed set of pose slots for a
 * fighter GObj. `ndsFtPoseUnbind` deliberately keeps that ownership when a
 * live fighter is RETARGETED by the event32 attach seam; destruction instead
 * calls `ndsFtPoseRelease`, which returns the slot while retaining its
 * scene-arena backing storage for the next CSS preview rebuild. Without that
 * distinction, dead previews consume the fixed slots until the taskman heap
 * generation changes and later previews fall back to the generic AObj path --
 * whose pool the pose engine's own budget shrank.
 *
 * Releasing here covers every death: the source calls this for CSS preview
 * rebuilds (mnplayersvs.c:2278) and for battle teardown alike. */
void ftManagerDestroyFighter(GObj *fighter_gobj)
{
    if (fighter_gobj != NULL)
    {
        ndsFtPoseRelease(fighter_gobj);
    }
    ndsBaseFTManagerDestroyFighter(fighter_gobj);
}
