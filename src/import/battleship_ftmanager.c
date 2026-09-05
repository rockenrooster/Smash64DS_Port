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
#include <nds/generated/nds_fighter_production.generated.h>
#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_GDONKEY || NDS_P2_MMARIO
#include <nds/generated/nds_native_fighter_image.generated.h>
#endif

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

void ftManagerSetupFilesAllKind(s32 fkind)
{
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
}

GObj *ftManagerMakeFighter(FTDesc *desc)
{
#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_GDONKEY || NDS_P2_MMARIO
    /* P2-3r4. A P2-3 fighter's generated geometry lives in a NitroFS image, so
     * it has to be resident before anything can draw this fighter. HERE is the
     * right seam: fighter creation is load-time work in every caller (battle
     * setup and the character select's preview rebuild), while a lazy load from
     * the draw path would be a NitroFS read inside a frame -- the exact stall
     * that cost the BGM its seam on the character select.
     *
     * Both detail levels are ensured together because the match decides between
     * them by fighter count, and a 3rd fighter arriving must not be the thing
     * that first touches the disk. */
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
#if NDS_P2_GDONKEY
        if (desc->fkind == nFTKindGDonkey)
        {
            /* Reuses the Donkey image packet. */
            image_slot = NDS_NATIVE_IMAGE_SLOT_DONKEY;
        }
#endif
        if (image_slot < NDS_NATIVE_IMAGE_OWNER_SLOTS)
        {
            (void)ndsRendererNativeEnsureOwnerImage(image_slot, 0u);
            (void)ndsRendererNativeEnsureOwnerImage(image_slot, 1u);
#if NDS_NATIVE_OWNER_IMAGE_VERIFY
            /* Proof build only: the arrays are still compiled in, so the
             * loaded bytes can be compared against them here, once, at the
             * one moment both exist. */
            (void)ndsRendererNativeVerifyOwnerImage(image_slot, 0u);
            (void)ndsRendererNativeVerifyOwnerImage(image_slot, 1u);
#endif
        }
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
