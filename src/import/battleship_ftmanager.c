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
#define ftManagerAllocFigatreeHeapKind ndsBaseFTManagerAllocFigatreeHeapKind
#define ftManagerAllocFighter ndsBaseFTManagerAllocFighter

void ndsBaseFTManagerSetupFileSize(void);
void ndsBaseFTManagerSetupFilesAllKind(s32 fkind);
GObj *ndsBaseFTManagerMakeFighter(FTDesc *desc);
void ndsBaseFTManagerDestroyFighter(GObj *fighter_gobj);
void ndsBaseFTManagerAllocFighter(u32 data_flags, s32 allocs_num);

#if NDS_P2_1P_GAME
#include <nds/nds_frontend_overlay.h>

/* Campaign battles (1P game, bonus, training) borrow the front-end overlay's
 * tail as asset storage (ndsFrontendOverlayBeginScene), and so far only the
 * native owner images used it: the Polygon Team ended with ~100 KB free there
 * and 5,248 B in the taskman arena. ftmanager's own allocations -- each
 * fighter's Main extern tree (ftmanager.c:285), the common files, the pools
 * and the figatree heaps -- are scene-lifetime bump allocations exactly like
 * the arena's, so they take the tail first and spill to the arena when it is
 * full. Every other scene has no cursor and allocates from the arena. */
static void *ndsFTManagerHeapMalloc(size_t size, u32 align)
{
    if ((gSCManagerSceneData.scene_curr == nSCKind1PGame) ||
        (gSCManagerSceneData.scene_curr == nSCKind1PBonusStage) ||
        (gSCManagerSceneData.scene_curr == nSCKind1PTrainingMode))
    {
        return ndsSceneAssetAlloc(size, align);
    }
    return syTaskmanMalloc(size, align);
}
#else
#define ndsFTManagerHeapMalloc syTaskmanMalloc
#endif

#if NDS_P2_ARM9_WRAM
#include <nds/nds_arm9_wram.h>

/* The first allocation in ftManagerAllocFighter is the FTStruct pool; it
 * goes to ARM9's shared-WRAM block when it fits (src/nds/main.c). The block
 * is granted once per taskman heap generation, so a second pool in one scene
 * falls back to the heap instead of aliasing the first. */
extern volatile u32 gNdsTaskmanHeapGeneration;
static u32 sNdsFTManagerPoolToWram;
static u32 sNdsFTManagerPoolWramGranted;
static u32 sNdsFTManagerPoolWramGeneration;
static void *ndsFTManagerPoolMalloc(size_t size, u32 align)
{
    u32 generation = gNdsTaskmanHeapGeneration;

    if ((sNdsFTManagerPoolToWram != 0u) &&
        (size <= NDS_ARM9_WRAM_FTSTRUCT_POOL_MAX) &&
        ((sNdsFTManagerPoolWramGranted == 0u) ||
         (sNdsFTManagerPoolWramGeneration != generation)))
    {
        sNdsFTManagerPoolToWram = 0u;
        sNdsFTManagerPoolWramGranted = 1u;
        sNdsFTManagerPoolWramGeneration = generation;
        return (void *)NDS_ARM9_WRAM_BASE;
    }
    sNdsFTManagerPoolToWram = 0u;
    return ndsFTManagerHeapMalloc(size, align);
}
#define syTaskmanMalloc ndsFTManagerPoolMalloc
#elif NDS_P2_1P_GAME
#define syTaskmanMalloc ndsFTManagerHeapMalloc
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftmanager.c"
#if NDS_P2_ARM9_WRAM || NDS_P2_1P_GAME
#undef syTaskmanMalloc
#endif

#undef ftManagerSetupFileSize
#undef ftManagerSetupFilesAllKind
#undef ftManagerMakeFighter
#undef ftManagerDestroyFighter
#undef ftManagerAllocFigatreeHeapKind
#undef ftManagerAllocFighter

/* SIZE THE FIGHTER POOLS BY THE FIGHTERS THIS BATTLE CAN HOLD.
 *
 * scVSBattleStartBattle and scVSBattleStartSuddenDeath both call
 * ftManagerAllocFighter(..., GMCOMMON_PLAYERS_MAX): one FTStruct (3,012 B)
 * and one FTPARTS_JOINT_NUM_MAX run of FTParts (8,288 B) for each of FOUR
 * fighters, whatever the CSS chose. In a two-player match half of that --
 * 22,600 bytes of the taskman arena -- is a free list nothing ever pops:
 * VS battle creates fighters only for players whose pkind is not
 * nFTPlayerKindNot, and a Sudden Death field is a subset of those players.
 * Both pools are free lists popped in order (ftmanager.c), and the shipping
 * imported-manager build never indexes them by player slot -- the live-GObj
 * registry in reloc_backend_fighter_model.c answers "which struct is player
 * N" -- so a shorter list changes capacity only, never which struct a fighter
 * gets or what it contains.
 *
 * Measured 2026-09-22, Fox vs Kirby, walk ROM with a heap ledger: Kongo
 * Jungle had 25,904 bytes left when Kirby's 30,160-byte owner image asked,
 * and syMallocSet's overflow halt froze the match at load. Zebes, Yoshi's
 * Island and Saffron froze the same way, and Sector Z loaded into 7,748 bytes
 * -- under the 25,600-byte ifCommonSetMaxNumGObj latch from the first frame,
 * and too little for the 8,552-byte copy hat Kirby loads when he swallows.
 *
 * Every other scene, and a battle whose state is not yet readable, keeps the
 * source count. A three-player match gets 11,300 bytes back; four, none. */
volatile u32 gNdsFTManagerPoolSlotsTrimmed;

#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_GDONKEY || NDS_P2_MMARIO || NDS_P2_NMARIO || NDS_P2_NFOX || NDS_P2_NDONKEY || NDS_P2_NSAMUS || NDS_P2_NLUIGI || NDS_P2_NLINK || NDS_P2_NYOSHI || NDS_P2_NCAPTAIN || NDS_P2_NKIRBY || NDS_P2_NPIKACHU || NDS_P2_NPURIN || NDS_P2_NNESS || NDS_P2_1P_GAME
static void ndsFTManagerPreloadVSOwnerImagesLargestFirst(void);
#endif

void ftManagerAllocFighter(u32 data_flags, s32 allocs_num)
{
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerBattleState != NULL) &&
        (allocs_num == GMCOMMON_PLAYERS_MAX))
    {
        s32 players = 0;
        s32 player;

        for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
        {
            if (gSCManagerBattleState->players[player].pkind !=
                nFTPlayerKindNot)
            {
                players++;
            }
        }
        if ((players > 0) && (players < allocs_num))
        {
            gNdsFTManagerPoolSlotsTrimmed = (u32)(allocs_num - players);
            allocs_num = players;
        }
    }
#if NDS_P2_ARM9_WRAM
    sNdsFTManagerPoolToWram = 1u;
#endif
    ndsBaseFTManagerAllocFighter(data_flags, allocs_num);
#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_GDONKEY || NDS_P2_MMARIO || NDS_P2_NMARIO || NDS_P2_NFOX || NDS_P2_NDONKEY || NDS_P2_NSAMUS || NDS_P2_NLUIGI || NDS_P2_NLINK || NDS_P2_NYOSHI || NDS_P2_NCAPTAIN || NDS_P2_NKIRBY || NDS_P2_NPIKACHU || NDS_P2_NPURIN || NDS_P2_NNESS || NDS_P2_1P_GAME
    /* Both VS entries (battle and Sudden Death) come through here before
     * their player loop allocates a figatree heap or makes a fighter. */
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerBattleState != NULL))
    {
        ndsFTManagerPreloadVSOwnerImagesLargestFirst();
    }
#endif
}

/* A fighter's figatree heap is sized by its largest animation file (Pikachu
 * 10,752 B, Yoshi 9,360, Link 7,328) and lives exactly as long as the battle.
 * In a two-player VS match the objman seam has idle fighter-packet storage left
 * over; take the heap from there and leave the arena its bytes. Anything that
 * does not fit, and every other scene, is the source allocation unchanged. The
 * animation loaders take the heap as an address and register their own
 * loaded-file range over it, so nothing asks whether it is inside the arena. */
void *ndsBattleIdleScratchAlloc(size_t size, u32 alignment);
void *ndsBaseFTManagerAllocFigatreeHeapKind(s32 fkind);

void *ftManagerAllocFigatreeHeapKind(s32 fkind)
{
    FTData *data = dFTManagerDataFiles[fkind];
    void *heap = ndsBattleIdleScratchAlloc(data->file_anim_size, 0x10u);

    return (heap != NULL) ? heap : ndsBaseFTManagerAllocFigatreeHeapKind(fkind);
}

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
/* Campaign variant images. Zako waves replace the fighter mid-battle
 * (sc1PGameSpawnEnemyTeamNext picks the next N-kind variation and calls
 * ftManagerMakeFighter, with either detail by fighter count). Preloading all
 * twelve polygon images at scene start kept every replacement a residency hit
 * but held ~170 KB the stage could not spare (5,248 B low-water); the stage
 * now arms the polygon image pool instead, and a replacement streams its image
 * at construction like a Kirby copy hat. NLuigi reuses the NMario packet and
 * needs no slot of its own. Metal Mario's stage preloads its single owner,
 * both details. Other stages never create these kinds, so nothing else pays.
 * Results are intentionally ignored: fighter creation re-ensures anyway. */
static void ndsFTManagerPreloadVariantOwnerImages(void)
{
    /* The campaign's stage field persists through menus and VS matches. */
    if (gSCManagerSceneData.scene_curr != nSCKind1PGame)
    {
        return;
    }
    if (gSCManagerSceneData.spgame_stage == (u8)nSC1PGameStageZako)
    {
        /* Not all twelve: the polygon images share a buffer pool for this
         * arena (nds_renderer_assets.c), filled by fighter construction --
         * the first wave at load, replacements as they are made. */
        ndsRendererNativeArmPolygonImagePool();
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
            /* Kirby is not listed: he can only copy electricity from a
             * Pikachu or Ness who is already in this match. */
            ((fkind == nFTKindPikachu) || (fkind == nFTKindNPikachu) ||
             (fkind == nFTKindNess) || (fkind == nFTKindNNess)))
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

#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_GDONKEY || NDS_P2_MMARIO || NDS_P2_NMARIO || NDS_P2_NFOX || NDS_P2_NDONKEY || NDS_P2_NSAMUS || NDS_P2_NLUIGI || NDS_P2_NLINK || NDS_P2_NYOSHI || NDS_P2_NCAPTAIN || NDS_P2_NKIRBY || NDS_P2_NPIKACHU || NDS_P2_NPURIN || NDS_P2_NNESS || NDS_P2_1P_GAME
/* THE ONE MAPPING FROM FIGHTER KIND TO NATIVE OWNER-IMAGE SLOT. Shared by the
 * construction-time ensure below and the VS preload in ftManagerAllocFighter,
 * which orders the loads by size; two copies of this chain would drift. */
static u32 ndsFTManagerImageSlotForKind(s32 fkind)
{
    u32 image_slot = NDS_NATIVE_IMAGE_OWNER_SLOTS;

#if NDS_P2_LUIGI
    if (fkind == nFTKindLuigi)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_LUIGI;
    }
#endif
#if NDS_P2_DONKEY
    if (fkind == nFTKindDonkey)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_DONKEY;
    }
#endif
#if NDS_P2_CAPTAIN
    if (fkind == nFTKindCaptain)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_CAPTAIN;
    }
#endif
#if NDS_P2_SAMUS
    if (fkind == nFTKindSamus)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_SAMUS;
    }
#endif
#if NDS_P2_LINK
    if (fkind == nFTKindLink)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_LINK;
    }
#endif
#if NDS_P2_PIKACHU
    if (fkind == nFTKindPikachu)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_PIKACHU;
    }
#endif
#if NDS_P2_YOSHI
    if (fkind == nFTKindYoshi)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_YOSHI;
    }
#endif
#if NDS_P2_NESS
    if (fkind == nFTKindNess)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NESS;
    }
#endif
#if NDS_P2_PURIN
    if (fkind == nFTKindPurin)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_PURIN;
    }
#endif
#if NDS_P2_KIRBY
    if (fkind == nFTKindKirby)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_KIRBY;
    }
#endif
#if NDS_P2_MMARIO
    if (fkind == nFTKindMMario)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_MMARIO;
    }
#endif
#if NDS_P2_NMARIO
    if (fkind == nFTKindNMario)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NMARIO;
    }
#endif
#if NDS_P2_NFOX
    if (fkind == nFTKindNFox)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NFOX;
    }
#endif
#if NDS_P2_NDONKEY
    if (fkind == nFTKindNDonkey)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NDONKEY;
    }
#endif
#if NDS_P2_NSAMUS
    if (fkind == nFTKindNSamus)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NSAMUS;
    }
#endif
#if NDS_P2_NLINK
    if (fkind == nFTKindNLink)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NLINK;
    }
#endif
#if NDS_P2_NYOSHI
    if (fkind == nFTKindNYoshi)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NYOSHI;
    }
#endif
#if NDS_P2_NCAPTAIN
    if (fkind == nFTKindNCaptain)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NCAPTAIN;
    }
#endif
#if NDS_P2_NKIRBY
    if (fkind == nFTKindNKirby)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NKIRBY;
    }
#endif
#if NDS_P2_NPIKACHU
    if (fkind == nFTKindNPikachu)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NPIKACHU;
    }
#endif
#if NDS_P2_NPURIN
    if (fkind == nFTKindNPurin)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NPURIN;
    }
#endif
#if NDS_P2_NNESS
    if (fkind == nFTKindNNess)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_NNESS;
    }
#endif
#if NDS_P2_1P_GAME
    if (fkind == nFTKindBoss)
    {
        image_slot = NDS_NATIVE_IMAGE_SLOT_BOSS;
    }
#endif
#if NDS_P2_NLUIGI
    if (fkind == nFTKindNLuigi)
    {
        /* Reuses the NMario image packet. */
        image_slot = NDS_NATIVE_IMAGE_SLOT_NMARIO;
    }
#endif
#if NDS_P2_GDONKEY
    if (fkind == nFTKindGDonkey)
    {
        /* Reuses the Donkey image packet. */
        image_slot = NDS_NATIVE_IMAGE_SLOT_DONKEY;
    }
#endif
    return image_slot;
}

#if NDS_P2_1P_GAME
/* Polygon image pool eviction (nds_renderer_assets.c): does a linked fighter
 * still name this image slot? sc1PGameSpawnEnemyTeamNext makes the next
 * polygon in the replaced one's FTStruct before its GObj unlinks, so that GObj
 * already names the new kind and no longer pins the old image. */
u32 ndsFTManagerOwnerImageSlotLive(u32 image_slot)
{
    GObj *fighter_gobj;

    for (fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
         fighter_gobj != NULL;
         fighter_gobj = fighter_gobj->link_next)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (ndsFTManagerImageSlotForKind(fp->fkind) == image_slot))
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif
#endif

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
        u32 image_slot = ndsFTManagerImageSlotForKind(desc->fkind);

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
                 (gSCManagerSceneData.scene_curr == nSCKind1PBonus1Players) ||
                 (gSCManagerSceneData.scene_curr == nSCKind1PBonus2Players) ||
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

#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_GDONKEY || NDS_P2_MMARIO || NDS_P2_NMARIO || NDS_P2_NFOX || NDS_P2_NDONKEY || NDS_P2_NSAMUS || NDS_P2_NLUIGI || NDS_P2_NLINK || NDS_P2_NYOSHI || NDS_P2_NCAPTAIN || NDS_P2_NKIRBY || NDS_P2_NPIKACHU || NDS_P2_NPURIN || NDS_P2_NNESS || NDS_P2_1P_GAME
/* LOAD THE BIGGEST OWNER IMAGE FIRST, SO IT IS THE ONE THE SCRATCH HOLDS.
 *
 * The battle scratch in idle fighter-packet ports is a bump allocator, and the
 * source makes fighters in port order: in Fox (P1) vs Kirby (P2), Fox's 15,336
 * byte image took the scratch and Kirby's 30,160 went to the taskman arena --
 * the allocation that froze Kongo Jungle at load. This runs from
 * ftManagerAllocFighter, before any figatree heap or fighter exists, and
 * ensures every player's images through the SAME ndsFTManagerEnsureOwnerImages
 * construction will call (same kinds, details and electric skeletons), only in
 * descending size. Construction then finds them resident. Nothing is loaded
 * that construction would not have loaded; only the order changes. */
static void ndsFTManagerBattleLowDetail(FTDesc *desc);

/* The detail construction will give this player: the source's fighter-count
 * rule, then the same low-detail override ftManagerMakeFighter applies. */
static u32 ndsFTManagerPreloadDetail(s32 player, sb32 high)
{
    FTDesc desc = dFTManagerDefaultFighterDesc;

    desc.fkind = gSCManagerBattleState->players[player].fkind;
    desc.pkind = gSCManagerBattleState->players[player].pkind;
    desc.detail = (high != FALSE) ? nFTPartsDetailHigh : nFTPartsDetailLow;
    ndsFTManagerBattleLowDetail(&desc);
    return (u32)desc.detail;
}

static void ndsFTManagerPreloadVSOwnerImagesLargestFirst(void)
{
    s32 order[GMCOMMON_PLAYERS_MAX];
    u32 bytes[GMCOMMON_PLAYERS_MAX];
    s32 count = 0;
    s32 player;
    s32 i;
    sb32 high = ((gSCManagerBattleState->pl_count +
                  gSCManagerBattleState->cp_count) < 3) ? TRUE : FALSE;

    for (player = 0; player < GMCOMMON_PLAYERS_MAX; player++)
    {
        u32 slot;

        if (gSCManagerBattleState->players[player].pkind == nFTPlayerKindNot)
        {
            continue;
        }
        slot = ndsFTManagerImageSlotForKind(
            gSCManagerBattleState->players[player].fkind);
        order[count] = player;
        bytes[count] = (slot < NDS_NATIVE_IMAGE_OWNER_SLOTS) ?
            (ndsRendererNativeOwnerImageSize(slot, 0u) +
             ((ndsFTManagerPreloadDetail(player, high) ==
               (u32)nFTPartsDetailHigh) ?
                  0u : ndsRendererNativeOwnerImageSize(slot, 1u))) :
            0u;
        count++;
    }
    /* Insertion sort, descending; equal sizes keep port order. */
    for (i = 1; i < count; i++)
    {
        s32 moving_player = order[i];
        u32 moving_bytes = bytes[i];
        s32 j = i;

        while ((j > 0) && (bytes[j - 1] < moving_bytes))
        {
            order[j] = order[j - 1];
            bytes[j] = bytes[j - 1];
            j--;
        }
        order[j] = moving_player;
        bytes[j] = moving_bytes;
    }
    for (i = 0; i < count; i++)
    {
        FTDesc desc = dFTManagerDefaultFighterDesc;

        desc.fkind = gSCManagerBattleState->players[order[i]].fkind;
        desc.pkind = gSCManagerBattleState->players[order[i]].pkind;
        desc.detail = (u8)ndsFTManagerPreloadDetail(order[i], high);
        ndsFTManagerEnsureOwnerImages(&desc);
    }
}
#endif

/* include/nds/renderer_fighter_lean.h (not included here: it drags the
 * renderer's lean-path types into a decomp TU). */
void ndsFtrLeanAdmitNoteFighter(u32 player, u32 fkind, u32 costume,
                                u32 detail);

#if NDS_P2_1P_GAME && NDS_P2_MENU_WALK
extern volatile u32 gNdsMenuShellWalkRoute;
extern volatile u32 gNdsCampaignWalkMeasure;

/* Campaign walk only (owner 2026-09-30: "make the human a lvl 9 CPU and the
 * opponents lvl 1 CPUs for campaign walking"; playtest ROMs keep the source
 * defaults). NDS_P2_MENU_WALK is 0 in every published configuration, and the
 * walk's campaign route is the only caller that sets route 1. The battle
 * state keeps the source's own pkind/level rows, so the 1P setup, its counts
 * and the fighters' detail level are exactly a human run's; only the made
 * fighter is driven by the computer. Every enemy, wave replacements included,
 * is made through here, so each one takes level 1. */
static void ndsCampaignWalkCpuRoute(FTDesc *desc)
{
    if ((desc == NULL) || (gNdsMenuShellWalkRoute != 1u) ||
        (gSCManagerSceneData.scene_curr != nSCKind1PGame) ||
        (gSCManagerBattleState == NULL) ||
        ((u32)desc->player >= GMCOMMON_PLAYERS_MAX))
    {
        return;
    }
    if ((s32)desc->player == (s32)gSCManagerSceneData.player)
    {
        desc->pkind = nFTPlayerKindCom;
        desc->level = 9;
    }
    else if ((gSCManagerBattleState->players[desc->player].is_spgame_enemy !=
              FALSE) && (gNdsCampaignWalkMeasure == 0u))
    {
        desc->level = 1;
        /* Walk speed: an 18-Yoshi team outlasted a level-9 player's five
         * minutes. Enemies start at 120%, so a hit or two ends each; Master
         * Hand keeps its own HP rules. */
        if (desc->fkind != nFTKindBoss)
        {
            desc->damage = 120;
        }
    }
}
#endif

/* Owner 2026-10-01: low-detail models are allowed in one- and two-fighter
 * matches ("the DS screen is small anyways"). The source picks
 * nFTPartsDetailHigh below three fighters (scvsbattle.c:188/460,
 * sc1pgame.c:1379/2139); battles take the four-fighter Lo-poly parts instead.
 * The source's own close-ups (KO, pause, stage-clear zoom) still switch to
 * high detail through ftParamSetModelPartDetailAll, as in every four-fighter
 * match, and a low-detail battle already prepares both owner images. Playable
 * kinds and Metal Mario (whose owner image carries both details); Master
 * Hand keeps the source's choice. Display scenes (CSS, intro, Results) are
 * untouched. */
static void ndsFTManagerBattleLowDetail(FTDesc *desc)
{
    u32 scene = (u32)gSCManagerSceneData.scene_curr;

    if ((desc == NULL) || (desc->detail != nFTPartsDetailHigh) ||
        (desc->pkind == nFTPlayerKindDemo) ||
        !(((desc->fkind >= 0) && (desc->fkind <= nFTKindPlayableEnd)) ||
          (desc->fkind == nFTKindMMario)))
    {
        return;
    }
    if ((scene == (u32)nSCKindVSBattle) || (scene == (u32)nSCKind1PGame) ||
        (scene == (u32)nSCKind1PBonusStage) ||
        (scene == (u32)nSCKind1PTrainingMode))
    {
        desc->detail = nFTPartsDetailLow;
    }
}

GObj *ftManagerMakeFighter(FTDesc *desc)
{
#if NDS_P2_1P_GAME && NDS_P2_MENU_WALK
    ndsCampaignWalkCpuRoute(desc);
#endif
    ndsFTManagerBattleLowDetail(desc);
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
    /* Installed whenever the table is not, not only over a NULL pointer:
     * after a battle or Results that loaded Kirby, gFTDataKirbyMainMotion
     * still points into that scene's rewound arena (no select reloads it --
     * the source's ftManagerSetupFilesKind did), so the constructor read
     * joint 6's model part from freed memory and the preview drew 6 of its
     * 7 roots into ndsPreviewPackLoadHalt(20). Owner r70: the CSS froze
     * after a 4P match. */
    if ((desc != NULL) && (desc->fkind == nFTKindKirby) &&
        ((gSCManagerSceneData.scene_curr == nSCKindPlayersVS) ||
         (gSCManagerSceneData.scene_curr == nSCKind1PGamePlayers) ||
         (gSCManagerSceneData.scene_curr == nSCKind1PBonus1Players) ||
         (gSCManagerSceneData.scene_curr == nSCKind1PBonus2Players)) &&
        (gFTDataKirbyMainMotion != sNdsFTManagerKirbyPreviewCopyTable))
    {
        if (sNdsFTManagerKirbyPreviewMainMotionActive == FALSE)
        {
            sNdsFTManagerKirbyPreviewMainMotionSaved = gFTDataKirbyMainMotion;
        }
        sNdsFTManagerKirbyPreviewMainMotionActive = TRUE;
        gFTDataKirbyMainMotion = sNdsFTManagerKirbyPreviewCopyTable;
    }
#endif
    {
        GObj *fighter_gobj;

#if NDS_P2_ARM9_WRAM
        /* The fighter GObj and top joint DObj are the source's first two
         * object allocations below: they take WRAM slots. */
        ndsGcDonateFighterObjs();
#endif
        fighter_gobj = ndsBaseFTManagerMakeFighter(desc);

        /* P2-2p8 Phase 1 slice 2b, the admission's creation seam (spec 2.8):
         * note every battle fighter; the last one of the battle runs the
         * fighter texture admission when gNdsFtrLeanAdmit is already set
         * (src/port/renderer_fighter_lean.c). */
        if ((fighter_gobj != NULL) && (desc != NULL))
        {
            ndsFtrLeanAdmitNoteFighter((u32)desc->player, (u32)desc->fkind,
                                       (u32)desc->costume, (u32)desc->detail);
        }
        return fighter_gobj;
    }
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
