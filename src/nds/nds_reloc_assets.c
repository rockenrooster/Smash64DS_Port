/* The relocation asset table: an asset id to its NitroFS path (the fighters'
 * rows from nds/generated/nds_fighter_production.generated.h), file loads
 * with their extern ids, the animation directory and the fighters' motion
 * stream spans; and ndsFsLock/ndsFsUnlock, one lock for the whole
 * filesystem, because libfat and NitroFS are not reentrant and more than one
 * thread reads. */

#include <filesystem.h>
#include <nds.h>
#include <calico.h>

/* ONE LOCK FOR THE WHOLE FILESYSTEM. libfat/nitrofs are not reentrant and
 * the BGM refill runs on a calico worker thread (nds_audio_bgm.c), so a
 * main-thread open or read that overlapped a worker fread walked a corrupt
 * directory: fopen returned ENOENT for files that exist (Mario Appear and
 * Fox Arwing entry animations on Jungle/Hyrule/Zebes, 2026-09-07), and the
 * force loader fell back to the raw heap. Recursive, because the reloc
 * loaders nest (extern tree -> zeroed heap load). */
static RMutex sNdsFsMutex;

void ndsFsLock(void)
{
    rmutexLock(&sNdsFsMutex);
}

void ndsFsUnlock(void)
{
    rmutexUnlock(&sNdsFsMutex);
}
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <nds/generated/nds_fighter_production.generated.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_frontend_overlay.h>

/* The battle's general heap and its admission test (include/nds/nds_startup.h,
 * sys/taskman.h), declared here so this loader stays off the source's
 * headers. */
struct SYMallocRegion;
extern struct SYMallocRegion gSYTaskmanGeneralHeap;
extern volatile u32 gNdsTaskmanHeapGeneration;
s32 ndsSyMallocWouldFit(const struct SYMallocRegion *bp, size_t size,
                        u32 alignment);
void *syTaskmanMalloc(size_t size, u32 align);

/* src/port/nds_p4.c; declared here so this loader stays off ft/fighter.h. */
const char *ndsP4RelocAssetPath(u32 file_id);

/* Keep this loader TU independent of the very broad startup header; the
 * published prototype lives there for normal consumers. */
extern void ndsTaskmanSampleLibcHeapNow(void);

/* sniprintf, NOT snprintf, in this file and in main.c -- the only two places the
 * port formats a string. newlib's snprintf drags in the whole floating-point
 * formatter: _svfprintf_r (10,047) pulls _dtoa_r (4,608), __mprec (2,480) and
 * __global_locale, and that last one pulls the 14,420-byte locale category
 * table. Six call sites, every one of them "%s" and "%lu", were paying 31,555
 * bytes of image for a conversion none of them asks for.
 *
 * That is not a size nicety on this target. Linked bytes come out of the
 * boot-time taskman arena one-for-one, so 31,555 is SEVEN arena steps of
 * 4,096 -- and the arena is tight enough (2026-07-31) that eight steps is the
 * difference between the battle running and dying under ifCommonSetMaxNumGObj's
 * 25 KiB latch. check-nds-printf-integer-only.ps1 keeps it from coming back;
 * any new format that genuinely needs %f has to justify 31 KB first. */

#define NDS_O2R_RESOURCE_HEADER_SIZE 0x40
#define NDS_O2R_MAGIC_OFFSET 0x04
#define NDS_O2R_MAGIC "OLER"

typedef struct NDSRelocAssetEntry {
    u32 asset_id;
    u32 file_id;
    const char *path;
} NDSRelocAssetEntry;

volatile u32 gNdsRelocAssetInitResult;
volatile u32 gNdsRelocAssetHeaderReadCount;
volatile u32 gNdsRelocAssetPayloadReadCount;
volatile u32 gNdsRelocAssetOpenFailCount;
volatile u32 gNdsRelocAssetOpenFailErrno;
volatile u32 gNdsRelocAssetOpenFailAsset;
volatile u32 gNdsRelocAssetDirectFailStep;
volatile u32 gNdsRelocAssetOpenRetryCount;
volatile u32 gNdsRelocAssetOpenRetrySuccessCount;
volatile char gNdsRelocAssetOpenFailPath[64];
volatile u32 gNdsRelocAssetFormatFailCount;
volatile u32 gNdsRelocAssetShortReadCount;
volatile u32 gNdsRelocAssetDirectReadCount;
volatile u32 gNdsRelocAssetDirectFallbackCount;
/* Same-binary performance control. Zero restores the established stdio/FAT
 * loader without changing code placement. */
__attribute__((used)) volatile u32 gNdsRelocAssetDirectDispatch = 1u;
__attribute__((used)) volatile u32 gNdsRelocAssetFighterStreamDispatch = 1u;
volatile u32 gNdsRelocAssetFighterStreamReads;
/* ARM9 time (/64 system ticks) and bytes of the clip body reads. */
volatile u32 gNdsRelocAssetFighterStreamReadTicks64;
volatile u32 gNdsRelocAssetFighterStreamReadBytes;
volatile u32 gNdsRelocAssetFighterStreamMisses;
volatile u32 gNdsRelocAssetFighterStreamFailures;

static const NDSRelocAssetEntry sNdsRelocAssets[] = {
    { 0, 0, NDS_NITRO_RELOC_MENUS "MNCommon" },
    { 194, 194, NDS_NITRO_RELOC_MISC_NAMED "N64Logo" },
    { 52, 52, NDS_NITRO_RELOC_MOVIES "MVCommon" },
    { 63, 63, NDS_NITRO_RELOC_TRANSITIONS "MVOpeningRoomTransition" },
    { 56, 56, NDS_NITRO_RELOC_MOVIES "MVOpeningRoomScene1" },
    { 57, 57, NDS_NITRO_RELOC_MOVIES "MVOpeningRoomScene2" },
    { 58, 58, NDS_NITRO_RELOC_MOVIES "MVOpeningRoomScene3" },
    { 59, 59, NDS_NITRO_RELOC_MOVIES "MVOpeningRoomScene4" },
    { 75, 75, NDS_NITRO_RELOC_MOVIES "MVOpeningRunCrash" },
    { 90, 90, NDS_NITRO_RELOC_MOVIES "MVOpeningRoomWallpaper" },
    { 53, 53, NDS_NITRO_RELOC_MOVIES "MVOpeningPortraitsSet1" },
    { 54, 54, NDS_NITRO_RELOC_MOVIES "MVOpeningPortraitsSet2" },
    { 37, 37, NDS_NITRO_RELOC_INTERFACE "IFCommonAnnounceCommon" },
    { 0x57, 0x57, NDS_NITRO_RELOC_INTERFACE "IFCommonItem" },
    { 65, 65, NDS_NITRO_RELOC_MOVIES "MVOpeningCommon" },
    { 55, 55, NDS_NITRO_RELOC_MOVIES "MVOpeningRun" },
    { 71, 71, NDS_NITRO_RELOC_MOVIES "MVOpeningYamabuki" },
    { 73, 73, NDS_NITRO_RELOC_MOVIES "MVOpeningSector" },
    { 167, 167, NDS_NITRO_RELOC_MENUS "MNTitle" },
    { 168, 168, NDS_NITRO_RELOC_MENUS "MNTitleFireAnim" },
    { 0xa6, 0xa6, NDS_NITRO_RELOC_INTERFACE "IFCommonPlayer" },
    { 0x52, 0x52, NDS_NITRO_RELOC_INTERFACE "IFCommonGameStatus" },
    { 0xa4, 0xa4, NDS_NITRO_RELOC_INTERFACE "IFCommonPlayerDamage" },
    { 0xa5, 0xa5, NDS_NITRO_RELOC_INTERFACE "IFCommonTimer" },
    { 0x24, 0x24, NDS_NITRO_RELOC_INTERFACE "IFCommonDigits" },
    { 0xc5, 0xc5, NDS_NITRO_RELOC_INTERFACE "IFCommonBattlePause" },
    { 0x26, 0x26, NDS_NITRO_RELOC_INTERFACE "IFCommonPlayerTags" },
    { 0x19, 0x19, NDS_NITRO_RELOC_FIGHTERS_COMMON "FTStocksZako" },
    { 0x22, 0x22, NDS_NITRO_RELOC_MENUS "MNVSResults" },
    { 0x1d, 0x1d, NDS_NITRO_RELOC_EXTERN_DATA "MiscData029" },
    { 0x138, 0x138, NDS_NITRO_RELOC_FIGHTERS_MAIN "NNessModel" },
    { 0xf1, 0xf1, NDS_NITRO_RELOC_FIGHTERS_MAIN "NNessMain" },
    { 0x132, 0x132, NDS_NITRO_RELOC_FIGHTERS_MAIN "NPurinModel" },
    { 0xea, 0xea, NDS_NITRO_RELOC_FIGHTERS_MAIN "NPurinMain" },
    { 0x133, 0x133, NDS_NITRO_RELOC_FIGHTERS_MAIN "NPikachuModel" },
    { 0xf5, 0xf5, NDS_NITRO_RELOC_FIGHTERS_MAIN "NPikachuMain" },
    { 0x131, 0x131, NDS_NITRO_RELOC_FIGHTERS_MAIN "NKirbyModel" },
    { 0xe7, 0xe7, NDS_NITRO_RELOC_FIGHTERS_MAIN "NKirbyMain" },
    { 0x137, 0x137, NDS_NITRO_RELOC_FIGHTERS_MAIN "NCaptainModel" },
    { 0xed, 0xed, NDS_NITRO_RELOC_FIGHTERS_MAIN "NCaptainMain" },
    { 0x130, 0x130, NDS_NITRO_RELOC_FIGHTERS_MAIN "NYoshiModel" },
    { 0xf8, 0xf8, NDS_NITRO_RELOC_FIGHTERS_MAIN "NYoshiMain" },
    { 0x136, 0x136, NDS_NITRO_RELOC_FIGHTERS_MAIN "NLinkModel" },
    { 0xe3, 0xe3, NDS_NITRO_RELOC_FIGHTERS_MAIN "NLinkMain" },
    { 0xdf, 0xdf, NDS_NITRO_RELOC_FIGHTERS_MAIN "NLuigiMain" },
    { 0x135, 0x135, NDS_NITRO_RELOC_FIGHTERS_MAIN "NSamusModel" },
    { 0xdb, 0xdb, NDS_NITRO_RELOC_FIGHTERS_MAIN "NSamusMain" },
    { 0x134, 0x134, NDS_NITRO_RELOC_FIGHTERS_MAIN "NDonkeyModel" },
    { 0xd6, 0xd6, NDS_NITRO_RELOC_FIGHTERS_MAIN "NDonkeyMain" },
    { 0x12f, 0x12f, NDS_NITRO_RELOC_FIGHTERS_MAIN "NFoxModel" },
    { 0xd3, 0xd3, NDS_NITRO_RELOC_FIGHTERS_MAIN "NFoxMain" },
    { 0x12d, 0x12d, NDS_NITRO_RELOC_FIGHTERS_MAIN "NMarioModel" },
    { 0xcf, 0xcf, NDS_NITRO_RELOC_FIGHTERS_MAIN "NMarioMain" },
    { 0x158, 0x158, NDS_NITRO_RELOC_FIGHTERS_MAIN "BossModel" },
    { 0xf9, 0xf9, NDS_NITRO_RELOC_FIGHTERS_MAIN "BossMainMotion" },
    { 0xfa, 0xfa, NDS_NITRO_RELOC_FIGHTERS_MAIN "BossMain" },
    { 0x12e, 0x12e, NDS_NITRO_RELOC_EXTERN_DATA "MiscData302" },
    { 0x159, 0x159, NDS_NITRO_RELOC_FIGHTERS_MAIN "MasterHandIcon" },
    /* The campaign's variants: Giant DK (his main; the rest is Donkey's) and
     * Metal Mario (main, motion, model; specials are Mario's). */
    { 0xd7, 0xd7, NDS_NITRO_RELOC_FIGHTERS_MAIN "GDonkeyMain" },
    { 0xce, 0xce, NDS_NITRO_RELOC_FIGHTERS_MAIN "MMarioMain" },
    { 0xcd, 0xcd, NDS_NITRO_RELOC_FIGHTERS_MAIN "MMarioMainMotion" },
    { 0x12c, 0x12c, NDS_NITRO_RELOC_FIGHTERS_MAIN "MMarioModel" },
    { 0xf, 0xf, NDS_NITRO_RELOC_MENUS "MNScreenAdjust" },
    { 0x88, 0x88, NDS_NITRO_RELOC_BONUS "Bonus2Common" },
    { 0xe, 0xe, NDS_NITRO_RELOC_BONUS "BonusPicturePlatform" },
    { 0xd, 0xd, NDS_NITRO_RELOC_BONUS "BonusPicture" },
    { 0xc, 0xc, NDS_NITRO_RELOC_MISC_NAMED "CharacterNames" },
    { 0x18, 0x18, NDS_NITRO_RELOC_MENUS "MNPlayersDifficulty" },
    { 0xfc, 0xfc, NDS_NITRO_RELOC_SCENE "SCExplainMain" },
    { 0xc6, 0xc6, NDS_NITRO_RELOC_SCENE "SCExplainGraphics" },
    { 0x1b, 0x1b, NDS_NITRO_RELOC_STAGES "GRWallpaperTrainingYellow" },
    { 0x1c, 0x1c, NDS_NITRO_RELOC_STAGES "GRWallpaperTrainingBlue" },
    { 0xfe, 0xfe, NDS_NITRO_RELOC_SCENE "SC1PTrainingMode" },
    { 0xc4, 0xc4, NDS_NITRO_RELOC_MENUS "MNSoundTest" },
    { 0x4e, 0x4e, NDS_NITRO_RELOC_MENUS "MNBackupClearHeaderOption" },
    { 0x4d, 0x4d, NDS_NITRO_RELOC_MENUS "MNBackupClear" },
    { 0x4, 0x4, NDS_NITRO_RELOC_MENUS "MNOption" },
    { 0x10, 0x10, NDS_NITRO_RELOC_MENUS "MNCharacters" },
    { 0x1f, 0x1f, NDS_NITRO_RELOC_MENUS "MNVSRecordMain" },
    { 0x20, 0x20, NDS_NITRO_RELOC_MENUS "MNDataCommon" },
    { 0x5, 0x5, NDS_NITRO_RELOC_MENUS "MNData" },
    { 0xad, 0xad, NDS_NITRO_RELOC_MENUS "MNCongraYoshiTop" },
    { 0xac, 0xac, NDS_NITRO_RELOC_MENUS "MNCongraYoshiBottom" },
    { 0xb1, 0xb1, NDS_NITRO_RELOC_MENUS "MNCongraSamusTop" },
    { 0xb0, 0xb0, NDS_NITRO_RELOC_MENUS "MNCongraSamusBottom" },
    { 0xb5, 0xb5, NDS_NITRO_RELOC_MENUS "MNCongraPurinTop" },
    { 0xb4, 0xb4, NDS_NITRO_RELOC_MENUS "MNCongraPurinBottom" },
    { 0xaf, 0xaf, NDS_NITRO_RELOC_MENUS "MNCongraPikachuTop" },
    { 0xae, 0xae, NDS_NITRO_RELOC_MENUS "MNCongraPikachuBottom" },
    { 0xc1, 0xc1, NDS_NITRO_RELOC_MENUS "MNCongraNessTop" },
    { 0xc0, 0xc0, NDS_NITRO_RELOC_MENUS "MNCongraNessBottom" },
    { 0xbb, 0xbb, NDS_NITRO_RELOC_MENUS "MNCongraMarioTop" },
    { 0xba, 0xba, NDS_NITRO_RELOC_MENUS "MNCongraMarioBottom" },
    { 0xbd, 0xbd, NDS_NITRO_RELOC_MENUS "MNCongraLuigiTop" },
    { 0xbc, 0xbc, NDS_NITRO_RELOC_MENUS "MNCongraLuigiBottom" },
    { 0xb3, 0xb3, NDS_NITRO_RELOC_MENUS "MNCongraLinkTop" },
    { 0xb2, 0xb2, NDS_NITRO_RELOC_MENUS "MNCongraLinkBottom" },
    { 0xab, 0xab, NDS_NITRO_RELOC_MENUS "MNCongraKirbyTop" },
    { 0xaa, 0xaa, NDS_NITRO_RELOC_MENUS "MNCongraKirbyBottom" },
    { 0xbf, 0xbf, NDS_NITRO_RELOC_MENUS "MNCongraFoxTop" },
    { 0xbe, 0xbe, NDS_NITRO_RELOC_MENUS "MNCongraFoxBottom" },
    { 0xb9, 0xb9, NDS_NITRO_RELOC_MENUS "MNCongraDonkeyTop" },
    { 0xb8, 0xb8, NDS_NITRO_RELOC_MENUS "MNCongraDonkeyBottom" },
    { 0xb7, 0xb7, NDS_NITRO_RELOC_MENUS "MNCongraCaptainTop" },
    { 0xb6, 0xb6, NDS_NITRO_RELOC_MENUS "MNCongraCaptainBottom" },
    { 0x4c, 0x4c, NDS_NITRO_RELOC_MOVIES "MVEnding" },
    { 0xc3, 0xc3, NDS_NITRO_RELOC_SCENE "SCStaffroll" },
    { 0x9, 0x9, NDS_NITRO_RELOC_MENUS "MNMessage" },
    { 0x17, 0x17, NDS_NITRO_RELOC_MENUS "MNPlayers1PMode" },
    { 0x4f, 0x4f, NDS_NITRO_RELOC_MENUS "MN1PContinue" },
    { 0x2, 0x2, NDS_NITRO_RELOC_MENUS "MN1P" },
    { 0xb, 0xb, NDS_NITRO_RELOC_SCENE "SC1PIntro" },
    { 0xa, 0xa, NDS_NITRO_RELOC_SCENE "SC1PChallenger" },
    { 0x97, 0x97, NDS_NITRO_RELOC_SCENE "SC1PStageClear3" },
    { 0x51, 0x51, NDS_NITRO_RELOC_SCENE "SC1PStageClear2" },
    { 0x50, 0x50, NDS_NITRO_RELOC_SCENE "SC1PStageClear1" },
    { 0x23, 0x23, NDS_NITRO_RELOC_FIGHTERS_COMMON "FTEmblemModels" },
    { 0x28, 0x28, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionAeroplane" },
    { 0x29, 0x29, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionCheck" },
    { 0x2a, 0x2a, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionGakubuthi" },
    { 0x2b, 0x2b, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionKannon" },
    { 0x2c, 0x2c, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionStar" },
    { 0x2d, 0x2d, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionSudare1" },
    { 0x2e, 0x2e, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionSudare2" },
    { 0x30, 0x30, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionBlock" },
    { 0x31, 0x31, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionRotScale" },
    { 0x32, 0x32, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionCurtain" },
    { 0x33, 0x33, NDS_NITRO_RELOC_TRANSITIONS "LBTransitionCamera" },
    { 0xc7, 0xc7, NDS_NITRO_RELOC_MISC_NAMED "SYKseg1Validate" },
    { 6, 6, NDS_NITRO_RELOC_MENUS "MNVSMode" },
    { 0x11, 0x11, NDS_NITRO_RELOC_MENUS "MNPlayersCommon" },
    { 0x12, 0x12, NDS_NITRO_RELOC_MENUS "MNPlayersGameModes" },
    { 0x13, 0x13, NDS_NITRO_RELOC_MENUS "MNPlayersPortraits" },
    { 0x14, 0x14, NDS_NITRO_RELOC_FIGHTERS_COMMON "FTEmblemSprites" },
    { 0x15, 0x15, NDS_NITRO_RELOC_MENUS "MNSelectCommon" },
    { 0x16, 0x16, NDS_NITRO_RELOC_MENUS "MNPlayersSpotlight" },
    { 0x1a, 0x1a, NDS_NITRO_RELOC_STAGES "GRWallpaperTrainingBlack" },
    { 0x1e, 0x1e, NDS_NITRO_RELOC_MENUS "MNMaps" },
    { 0x21, 0x21, NDS_NITRO_RELOC_MENUS "MNCommonFonts" },
    { 0xff, 0xff, NDS_NITRO_RELOC_STAGES "GRPupupuMap" },
    { 0x104, 0x104, NDS_NITRO_RELOC_STAGES "GRInishieMap" },
    { 0x109, 0x109, NDS_NITRO_RELOC_STAGES "GRHyruleMap" },
    { 0x1005f, 0x5f, NDS_NITRO_RELOC_STAGES "StageCastle" },
    { 0x10058, 0x58, NDS_NITRO_RELOC_STAGES "StageDreamLand" },
    { 0x71, 0x71, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank113" },
    { 0x67, 0x67, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank103" },
    { 0x68, 0x68, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank104" },
    { 0x98, 0x98, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank152" },
    /* P2-4s1 Yoster closure (Makefile:4186-4190; IDs reloc_data.h:527,535). */
#if NDS_P2_STAGE_YOSTER
    { 0x107, 0x107, NDS_NITRO_RELOC_STAGES "GRYosterMap" },
    { 0x1005d, 0x5d, NDS_NITRO_RELOC_STAGES "StageYoshi" },
    { 0x6e, 0x6e, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank110" },
    { 0x6f, 0x6f, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank111" },
    { 0x9a, 0x9a, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank154" },
#endif
    /* P2-4s2 Castle. The wallpaper its map header references,
     * MVOpeningRoomWallpaper, is asset 90 and already rowed unconditionally
     * above, so only the map and its two banks are new. */
#if NDS_P2_STAGE_CASTLE
    { 0x103, 0x103, NDS_NITRO_RELOC_STAGES "GRCastleMap" },
    { 0x6a, 0x6a, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank106" },
    { 0x9c, 0x9c, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank156" },
#endif
    /* P2-4s3 Congo Jungle. Its map declares exactly these three, checked
     * against the O2R header rather than assumed. */
#if NDS_P2_STAGE_JUNGLE
    { 0x105, 0x105, NDS_NITRO_RELOC_STAGES "GRJungleMap" },
    { 0x1005c, 0x5c, NDS_NITRO_RELOC_STAGES "StageJungle" },
    { 0x6c, 0x6c, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank108" },
    { 0x9e, 0x9e, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank158" },
#endif
    /* P2-4s4 Planet Zebes, closure checked against the map header. */
#if NDS_P2_STAGE_ZEBES
    { 0x101, 0x101, NDS_NITRO_RELOC_STAGES "GRZebesMap" },
    { 0x10059, 0x59, NDS_NITRO_RELOC_STAGES "StageZebes" },
    { 0x69, 0x69, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank105" },
    { 0x9d, 0x9d, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank157" },
#endif
    /* P2-4s6 Saffron City: the only stage map so far with two Misc banks. */
    /* P2-4s7 Mushroom Kingdom and P2-4s8 Sector Z. */
#if NDS_P2_STAGE_INISHIE
    { 0x1005b, 0x5b, NDS_NITRO_RELOC_STAGES "StageHyruleWallpaper" },
    { 0x6b, 0x6b, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank107" },
    { 0x9b, 0x9b, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank155" },
#endif
#if NDS_P2_STAGE_SECTOR
    { 0x106, 0x106, NDS_NITRO_RELOC_STAGES "GRSectorMap" },
    { 0x10063, 0x63, NDS_NITRO_RELOC_STAGES "StageSector" },
    { 0x6d, 0x6d, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank109" },
    { 0x99, 0x99, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank153" },
#endif
    /* P2-6 step 7 (boss). Final Destination's runtime map (Last), its
     * geometry bank and the wallpaper container its header references; the
     * 1P arenas ride NDS_P2_1P_GAME. The native packet draws the wallpaper;
     * file 96 is staged because the extern-tree loader refuses the map when
     * any dependency in its extern table is unrowed. */
#if NDS_P2_1P_GAME
    { 0x10a, 0x10a, NDS_NITRO_RELOC_STAGES "GRLastMap" },
    { 0x72, 0x72, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank114" },
    /* THE O2R WALLPAPER CONTAINERS ARE MISLABELLED, CONSISTENTLY. The o2r
     * names its supplemental wallpaper files from a guess table
     * (decomp tools/generate_yamls.py SUPPLEMENTAL_NAMES): ROM file 91 is
     * called StageHyruleWallpaper, 96 StageYamabukiWallpaper, 97
     * StageInishieWallpaper, 98 StageLastWallpaper and 119
     * StageMetalWallpaper, while the map files' own relocation tables
     * (relocData/*.reloc) say 91 is Mushroom Kingdom's background, 96
     * Final Destination's, 97 Duel Zone's, 98 Meta Crystal's and 119 the
     * bonus boards'. The token is the truth and the name is only where the
     * bytes live, so each row pairs the ROM file id with the container that
     * HOLDS that id; do not "correct" a name to match the stage. */
    { 0x10060, 0x60, NDS_NITRO_RELOC_STAGES "StageYamabukiWallpaper" },
    /* The other 1P arenas (reloc_backend_assets.c names each dependency). */
    { 0x10d, 0x10d, NDS_NITRO_RELOC_STAGES "GRMetalMap" },
    { 0x75, 0x75, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank117" },
    { 0x10062, 0x62, NDS_NITRO_RELOC_STAGES "StageLastWallpaper" },
    { 0x10e, 0x10e, NDS_NITRO_RELOC_STAGES "GRYosterSmallMap" },
    { 0x76, 0x76, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank118" },
    { 0x100, 0x100, NDS_NITRO_RELOC_STAGES "GRPupupuSmallMap" },
    { 0x65, 0x65, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank101" },
    { 0x64, 0x64, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank100" },
    { 0x10c, 0x10c, NDS_NITRO_RELOC_STAGES "GRZakoMap" },
    { 0x74, 0x74, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank116" },
    { 0x10061, 0x61, NDS_NITRO_RELOC_STAGES "StageInishieWallpaper" },
    /* The bonus boards (reloc_backend_assets.c names each dependency); 119
     * lives in the o2r container named StageMetalWallpaper, see above. */
    { 0x10077, 0x77, NDS_NITRO_RELOC_STAGES "StageMetalWallpaper" },
    { 0x78, 0x78, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank120" },
    { 0x79, 0x79, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank121" },
    { 0x7a, 0x7a, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank122" },
    { 0x7b, 0x7b, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank123" },
    { 0x10f, 0x10f, NDS_NITRO_RELOC_STAGES "GRBonus1MarioMap" },
    { 0x7c, 0x7c, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank124" },
    { 0x110, 0x110, NDS_NITRO_RELOC_STAGES "GRBonus1FoxMap" },
    { 0x7d, 0x7d, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank125" },
    { 0x111, 0x111, NDS_NITRO_RELOC_STAGES "GRBonus1DonkeyMap" },
    { 0x7e, 0x7e, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank126" },
    { 0x112, 0x112, NDS_NITRO_RELOC_STAGES "GRBonus1SamusMap" },
    { 0x7f, 0x7f, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank127" },
    { 0x113, 0x113, NDS_NITRO_RELOC_STAGES "GRBonus1LuigiMap" },
    { 0x80, 0x80, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank128" },
    { 0x114, 0x114, NDS_NITRO_RELOC_STAGES "GRBonus1LinkMap" },
    { 0x81, 0x81, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank129" },
    { 0x115, 0x115, NDS_NITRO_RELOC_STAGES "GRBonus1YoshiMap" },
    { 0x82, 0x82, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank130" },
    { 0x116, 0x116, NDS_NITRO_RELOC_STAGES "GRBonus1CaptainMap" },
    { 0x83, 0x83, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank131" },
    { 0x117, 0x117, NDS_NITRO_RELOC_STAGES "GRBonus1KirbyMap" },
    { 0x84, 0x84, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank132" },
    { 0x118, 0x118, NDS_NITRO_RELOC_STAGES "GRBonus1PikachuMap" },
    { 0x85, 0x85, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank133" },
    { 0x119, 0x119, NDS_NITRO_RELOC_STAGES "GRBonus1PurinMap" },
    { 0x86, 0x86, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank134" },
    { 0x11a, 0x11a, NDS_NITRO_RELOC_STAGES "GRBonus1NessMap" },
    { 0x87, 0x87, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank135" },
    { 0x11b, 0x11b, NDS_NITRO_RELOC_STAGES "GRBonus2MarioMap" },
    { 0x89, 0x89, NDS_NITRO_RELOC_BONUS "BonusDataBank137" },
    { 0x11c, 0x11c, NDS_NITRO_RELOC_STAGES "GRBonus2FoxMap" },
    { 0x8a, 0x8a, NDS_NITRO_RELOC_BONUS "BonusDataBank138" },
    { 0x11d, 0x11d, NDS_NITRO_RELOC_STAGES "GRBonus2DonkeyMap" },
    { 0x8b, 0x8b, NDS_NITRO_RELOC_BONUS "BonusDataBank139" },
    { 0x11e, 0x11e, NDS_NITRO_RELOC_STAGES "GRBonus2SamusMap" },
    { 0x8c, 0x8c, NDS_NITRO_RELOC_BONUS "BonusDataBank140" },
    { 0x11f, 0x11f, NDS_NITRO_RELOC_STAGES "GRBonus2LuigiMap" },
    { 0x8d, 0x8d, NDS_NITRO_RELOC_BONUS "BonusDataBank141" },
    { 0x120, 0x120, NDS_NITRO_RELOC_STAGES "GRBonus2LinkMap" },
    { 0x8e, 0x8e, NDS_NITRO_RELOC_BONUS "BonusDataBank142" },
    { 0x121, 0x121, NDS_NITRO_RELOC_STAGES "GRBonus2YoshiMap" },
    { 0x8f, 0x8f, NDS_NITRO_RELOC_BONUS "BonusDataBank143" },
    { 0x122, 0x122, NDS_NITRO_RELOC_STAGES "GRBonus2CaptainMap" },
    { 0x90, 0x90, NDS_NITRO_RELOC_BONUS "BonusDataBank144" },
    { 0x123, 0x123, NDS_NITRO_RELOC_STAGES "GRBonus2KirbyMap" },
    { 0x91, 0x91, NDS_NITRO_RELOC_BONUS "BonusDataBank145" },
    { 0x124, 0x124, NDS_NITRO_RELOC_STAGES "GRBonus2PikachuMap" },
    { 0x92, 0x92, NDS_NITRO_RELOC_BONUS "BonusDataBank146" },
    { 0x125, 0x125, NDS_NITRO_RELOC_STAGES "GRBonus2PurinMap" },
    { 0x93, 0x93, NDS_NITRO_RELOC_BONUS "BonusDataBank147" },
    { 0x126, 0x126, NDS_NITRO_RELOC_STAGES "GRBonus2NessMap" },
    { 0x94, 0x94, NDS_NITRO_RELOC_BONUS "BonusDataBank148" },
    { 0x127, 0x127, NDS_NITRO_RELOC_STAGES "GRBonus3Map" },
    { 0x95, 0x95, NDS_NITRO_RELOC_BONUS "BonusDataBank149" },
    { 0xa2, 0xa2, NDS_NITRO_RELOC_EXTERN_DATA "MiscData162" },
    { 0x96, 0x96, NDS_NITRO_RELOC_BONUS "BonusDataBank150" },
    { 0xfd, 0xfd, NDS_NITRO_RELOC_EXTERN_DATA "MiscData253" },
    { 0xe6, 0xe6, NDS_NITRO_RELOC_EXTERN_DATA "MiscData230" },
#endif
#if NDS_P2_STAGE_YAMABUKI
    { 0x108, 0x108, NDS_NITRO_RELOC_STAGES "GRYamabukiMap" },
    { 0x1005e, 0x5e, NDS_NITRO_RELOC_STAGES "StagePokemon" },
    { 0x70, 0x70, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank112" },
    { 0x9f, 0x9f, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank159" },
    { 0xa0, 0xa0, NDS_NITRO_RELOC_EXTERN_DATA "MiscDataBank160" },
#endif
    { 0xa3, 0xa3, NDS_NITRO_RELOC_FIGHTERS_COMMON "FTManagerCommon" },
    { 0xcb, 0xcb, NDS_NITRO_RELOC_FIGHTERS_MAIN "MarioMain" },
    { 0xca, 0xca, NDS_NITRO_RELOC_FIGHTERS_MAIN "MarioMainMotion" },
    { 0x128, 0x128, NDS_NITRO_RELOC_FIGHTERS_MAIN "MarioModel" },
    { 0x12a, 0x12a, NDS_NITRO_RELOC_FIGHTERS_MAIN "MarioShieldPose" },
    { 0xcc, 0xcc, NDS_NITRO_RELOC_FIGHTERS_MAIN "MarioSpecial1" },
    { 0x164, 0x164, NDS_NITRO_RELOC_FIGHTERS_MAIN "MarioSpecial2" },
    { 0x129, 0x129, NDS_NITRO_RELOC_FIGHTERS_MAIN "MarioSpecial3" },
    { 0xd1, 0xd1, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxMain" },
    { 0xd0, 0xd0, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxMainMotion" },
    { 0x139, 0x139, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxModel" },
    { 0x13a, 0x13a, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxShieldPose" },
    { 0xd2, 0xd2, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxSpecial1" },
    { 0x15a, 0x15a, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxSpecial2" },
    { 0xa1, 0xa1, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxSpecial3" },
    { 0x13c, 0x13c, NDS_NITRO_RELOC_FIGHTERS_MAIN "FoxSpecial4" },
    { 0x53, 0x53, NDS_NITRO_RELOC_EFFECTS "EFCommonEffects1" },
    { 0x54, 0x54, NDS_NITRO_RELOC_EFFECTS "EFCommonEffects2" },
    { 0x55, 0x55, NDS_NITRO_RELOC_EFFECTS "EFCommonEffects3" },
    { 0xc9, 0xc9, NDS_NITRO_RELOC_EXTERN_DATA "MiscData201" },
    { 0x12b, 0x12b, NDS_NITRO_RELOC_EXTERN_DATA "MiscData299" },
    { 0x13b, 0x13b, NDS_NITRO_RELOC_EXTERN_DATA "MiscData315" },
    { 0x6d, 0x6d, NDS_NITRO_RELOC_EXTERN_DATA "ExternDataBank109" },
    /* P2-3f48. The item subsystem's shared data and its only dependency.
     * MiscData086 also appears in Yoshi's generated dependency rows below;
     * ndsRelocAssetFindEntry returns the first match and both rows carry the
     * same path, so the duplicate is inert. It is listed here because the item
     * file needs it whether or not Yoshi is in the build. */
    { 0xfb, 0xfb, NDS_NITRO_RELOC_ITEMS "ITCommonData" },
    { 0x56, 0x56, NDS_NITRO_RELOC_EXTERN_DATA "MiscData086" },
    /* P2-3: these rows are generated from BattleShip FTData / relocData and
     * O2R headers.  Keeping the path table generated is the first production
     * pipeline invariant: admitting another fighter must not grow a second
     * hand-maintained asset manifest here. */
#define NDS_P2_FIGHTER_ASSET_ENTRY(symbol_, id_, path_) { id_, id_, path_ },
#define NDS_P2_FIGHTER_DEPENDENCY_ENTRY(id_, path_) { id_, id_, path_ },
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_LUIGI_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_DONKEY_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_CAPTAIN_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
    NDS_P2_SAMUS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_SAMUS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#if NDS_P2_LINK
    NDS_P2_LINK_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_LINK_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
    NDS_P2_PIKACHU_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_PIKACHU_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
    NDS_P2_YOSHI_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_YOSHI_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
    NDS_P2_NESS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_NESS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
    NDS_P2_PURIN_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_PURIN_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
    NDS_P2_KIRBY_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_KIRBY_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#undef NDS_P2_FIGHTER_DEPENDENCY_ENTRY
#undef NDS_P2_FIGHTER_ASSET_ENTRY
    /* R02/R03/K06 -- THE PATH HALF OF THE RESULTS DEMO POSES, AND IT WAS THE
     * MISSING HALF. `NDS_*_DEMO_ANIM_ASSET_ROWS` carries three fields, and
     * until now only two of them were ever consumed: `sNdsRelocDemoAnimTokens`
     * (reloc_backend_assets.c) expanded `symbol_`/`id_` so the token resolved,
     * and NOTHING expanded `path_`. So every Results demo pose resolved its
     * asset id, answered ndsRelocIsFighterAnimID, entered
     * ndsRelocForceLoadFighterAObj16File -- and died at its
     * `ndsRelocAssetGetPath(asset_id) == NULL` guard, because these ids live
     * under `reloc_submotions/` with source-derived names that
     * ndsRelocAssetP2FighterAnimEntry's `reloc_animations/<stem><NNN>`
     * formatter cannot produce and no hand row here listed.
     *
     * The loader then returns NULL and ftMainSetStatus assigns fp->figatree
     * unconditionally, so the fighter binds the previous motion's figatree with
     * no decline. That is the whole of "all fighters hold a battle pose on the
     * Results screen" and "No Contest does not clap": the 35 payloads were
     * staged, all 47 cells routed statically, and the runtime could not name
     * a single file.
     *
     * These rows mirror sNdsRelocDemoAnimTokens arm for arm; the two tables
     * must stay in step, which check_results_demo_motion_closure.py asserts. */
#define NDS_RELOC_DEMO_ANIM_PATH_ENTRY(symbol_, id_, path_) { id_, id_, path_ },
    NDS_MARIOFOX_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#endif
    NDS_P2_SAMUS_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#if NDS_P2_LINK
    NDS_P2_LINK_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#endif
    NDS_P2_PIKACHU_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
    NDS_P2_YOSHI_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
    NDS_P2_NESS_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
    NDS_P2_PURIN_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
    NDS_P2_KIRBY_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#if NDS_P2_1P_GAME
    /* The 1P scenes' poses: the same pair of tables, the same lockstep. */
    NDS_1P_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_PATH_ENTRY)
#endif
#undef NDS_RELOC_DEMO_ANIM_PATH_ENTRY
};

/* P8 R2: the directories a table path's first byte names (nds_reloc_assets.h
 * NDS_NITRO_*), by code. */
static const char *const sNdsNitroDirs[] = {
    NULL,
    "nitro:/reloc/reloc_menus/",
    "nitro:/reloc/reloc_misc_named/",
    "nitro:/reloc/reloc_movies/",
    "nitro:/reloc/reloc_transitions/",
    "nitro:/reloc/reloc_interface/",
    "nitro:/reloc/reloc_fighters_common/",
    "nitro:/reloc/reloc_extern_data/",
    "nitro:/reloc/reloc_fighters_main/",
    "nitro:/reloc/reloc_stages/",
    "nitro:/reloc/reloc_bonus/",
    "nitro:/reloc/reloc_scene/",
    "nitro:/reloc/reloc_animations/",
    "nitro:/reloc/reloc_effects/",
    "nitro:/reloc/reloc_items/",
    "nitro:/reloc/reloc_submotions/",
    "nitro:/reloc/p4/",
    "nitro:/stages/",
    "nitro:/audio/",
    "nitro:/fighters/",
    "nitro:/fighters/p4/",
    "nitro:/p4/audio/",
};
/* The opens below expand a table path here; they run under the filesystem
 * lock or on the one loading thread, never two at once. */
static char sNdsNitroPathScratch[NDS_NITRO_PATH_MAX];

const char *ndsNitroPath(const char *path, char *buf)
{
    const u32 code = (path != NULL) ? (u8)path[0] : 0u;
    const char *in;
    u32 n = 0u;

    if ((code == 0u) ||
        (code >= (sizeof(sNdsNitroDirs) / sizeof(sNdsNitroDirs[0]))))
    {
        return path;
    }
    for (in = sNdsNitroDirs[code]; *in != '\0'; in++)
    {
        buf[n++] = *in;
    }
    for (in = path + 1; (*in != '\0') && (n + 1u < NDS_NITRO_PATH_MAX); in++)
    {
        buf[n++] = *in;
    }
    buf[n] = '\0';
    return buf;
}

static u16 ndsReadLe16(const u8 *bytes)
{
    return (u16)bytes[0] | ((u16)bytes[1] << 8);
}

static u32 ndsReadLe32(const u8 *bytes)
{
    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

#define NDS_RELOC_MARIO_ANIM_FIRST 0x1f3u
#define NDS_RELOC_MARIO_ANIM_LAST 0x281u
#define NDS_RELOC_FOX_ANIM_FIRST 0x282u
#define NDS_RELOC_FOX_ANIM_LAST 0x31fu
#define NDS_RELOC_MARIO_ANIM_PATH_CAPACITY 64u

/* Three copies of these four ids existed independently before slice 1 phase 7
 * -- this file's routing spans, reloc_backend_assets.c's per-animation defines,
 * and now the header's. Pin them so a future renumber cannot silently split the
 * fighter attribution from the path routing. */
_Static_assert(NDS_RELOC_MARIO_ANIM_FIRST == NDS_K0_MARIO_ANIM_FIRST,
               "K0 fighter span drifted from the Mario animation routing span");
_Static_assert(NDS_RELOC_MARIO_ANIM_LAST == NDS_K0_MARIO_ANIM_LAST,
               "K0 fighter span drifted from the Mario animation routing span");
_Static_assert(NDS_RELOC_FOX_ANIM_FIRST == NDS_K0_FOX_ANIM_FIRST,
               "K0 fighter span drifted from the Fox animation routing span");
_Static_assert(NDS_RELOC_FOX_ANIM_LAST == NDS_K0_FOX_ANIM_LAST,
               "K0 fighter span drifted from the Fox animation routing span");

/* Slice 1 phase 7. Published by the taskman update loops; see the header. */
__attribute__((used)) volatile u32 gNdsK0BattleInGo;
__attribute__((used)) volatile u32 gNdsK0AfterGoAcquisitions[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoPackHits[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoFatReads[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoSeeks[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoByteSwaps[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoRelocs[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoNormalizes[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoCacheCopies[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoTokenResolves[2];
__attribute__((used)) volatile u32 gNdsK0AfterGoPathLookups[2];

u32 ndsK0AfterGoFighter(u32 asset_id)
{
    if (gNdsK0BattleInGo == 0u)
    {
        return NDS_K0_FIGHTER_NONE;
    }
    if ((asset_id >= NDS_RELOC_MARIO_ANIM_FIRST) &&
        (asset_id <= NDS_RELOC_MARIO_ANIM_LAST))
    {
        return NDS_K0_FIGHTER_MARIO;
    }
    if ((asset_id >= NDS_RELOC_FOX_ANIM_FIRST) &&
        (asset_id <= NDS_RELOC_FOX_ANIM_LAST))
    {
        return NDS_K0_FIGHTER_FOX;
    }
    return NDS_K0_FIGHTER_NONE;
}

/* "nitro:/reloc/reloc_animations/<stem>%03lu" without the formatter. Every
 * asset->path resolution of a motion comes through the three entry functions
 * below (GetPath, ReadHeader, ReadExternFileIDs and both payload loaders), and
 * a sniprintf there cost a few thousand ticks per call on status-change
 * frames. -> the length written, or `capacity` when it does not fit, which the
 * callers refuse exactly as they refused a truncated sniprintf. */
static size_t ndsRelocFormatAnimPath(char *path, size_t capacity,
                                     const char *stem, u32 number)
{
    static const char prefix[] = "nitro:/reloc/reloc_animations/";
    char digits[10];
    u32 count = 0u;
    size_t length = 0u;
    const char *p;

    do
    {
        digits[count++] = (char)('0' + (number % 10u));
        number /= 10u;
    } while (number != 0u);
    while (count < 3u)
    {
        digits[count++] = '0';
    }
    for (p = prefix; *p != '\0'; p++)
    {
        if (length + 1u >= capacity)
        {
            return capacity;
        }
        path[length++] = *p;
    }
    for (p = stem; *p != '\0'; p++)
    {
        if (length + 1u >= capacity)
        {
            return capacity;
        }
        path[length++] = *p;
    }
    while (count != 0u)
    {
        if (length + 1u >= capacity)
        {
            return capacity;
        }
        path[length++] = digits[--count];
    }
    path[length] = '\0';
    return length;
}

static const NDSRelocAssetEntry *ndsRelocAssetMarioAnimEntry(u32 asset_id)
{
    static NDSRelocAssetEntry entry;
    static char path[NDS_RELOC_MARIO_ANIM_PATH_CAPACITY];
    u32 index;
    int written;

    if ((asset_id < NDS_RELOC_MARIO_ANIM_FIRST) ||
        (asset_id > NDS_RELOC_MARIO_ANIM_LAST))
    {
        return NULL;
    }

    /* The path is a function of the id: the last answer is still exact. */
    if ((entry.path != NULL) && (entry.asset_id == asset_id))
    {
        return &entry;
    }
    index = asset_id - NDS_RELOC_MARIO_ANIM_FIRST;
    if (index == 0u)
    {
        written = sniprintf(path, sizeof(path),
                           "nitro:/reloc/reloc_animations/FTMarioAnimWait");
    }
    else if (index == 44u)
    {
        written = sniprintf(path, sizeof(path),
                           "nitro:/reloc/reloc_animations/FTMarioAnimDownBounceD");
    }
    else if (index == 46u)
    {
        written = sniprintf(path, sizeof(path),
                           "nitro:/reloc/reloc_animations/FTMarioAnimDownStandD");
    }
    else
    {
        written = (int)ndsRelocFormatAnimPath(path, sizeof(path),
                                              "FTMarioAnim", index);
    }
    if ((written < 0) || ((size_t)written >= sizeof(path)))
    {
        return NULL;
    }

    entry.asset_id = asset_id;
    entry.file_id = asset_id;
    entry.path = path;
    return &entry;
}

static const NDSRelocAssetEntry *ndsRelocAssetFoxAnimEntry(u32 asset_id)
{
    static NDSRelocAssetEntry entry;
    static char path[NDS_RELOC_MARIO_ANIM_PATH_CAPACITY];
    int written;

    if ((asset_id < NDS_RELOC_FOX_ANIM_FIRST) ||
        (asset_id > NDS_RELOC_FOX_ANIM_LAST))
    {
        return NULL;
    }

    if ((entry.path != NULL) && (entry.asset_id == asset_id))
    {
        return &entry;
    }
    written = (int)ndsRelocFormatAnimPath(path, sizeof(path), "FTFoxAnim",
                                          asset_id - NDS_RELOC_FOX_ANIM_FIRST);
    if ((written < 0) || ((size_t)written >= sizeof(path)))
    {
        return NULL;
    }

    entry.asset_id = asset_id;
    entry.file_id = asset_id;
    entry.path = path;
    return &entry;
}

/* P2-3 fighter-local animation O2Rs are generated as contiguous numbered files.
 * Do not retain one pointer + one path string per motion in ARM9 RAM merely to
 * rediscover that numbering at runtime.  The production manifest validates the
 * complete id/path sequence before emitting *_ANIM_PATH_STEM, so this is an AOT
 * representation of the same source catalog rather than a naming guess. */
/* One row per contiguous run of a corpus stem, generated from each fighter's
 * MOTION TABLE (generate_fighter_production_manifest.py). A fighter's files are
 * not all under his own stem -- Purin takes 77 from FTKirbyAnim and Kirby 2
 * from FTKirbyCopyAnim -- so resolving by "this fighter's range, one or two
 * stems" made a fighter's animations depend on which OTHER fighter flags were
 * in the build. Rows may overlap between fighters; they agree, because the
 * path is a property of the asset id, not of who asks for it. */
typedef struct NDSP2FighterAnimSegment {
    u32 first;
    u32 last;
    const char *stem;
    u32 zero_id;
} NDSP2FighterAnimSegment;

#define NDS_P2_ANIM_SEGMENT_ROW(first, last, stem, zero_id) \
    { (first), (last), (stem), (zero_id) },

static const NDSP2FighterAnimSegment sNdsP2FighterAnimSegments[] = {
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
    NDS_P2_SAMUS_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#if NDS_P2_LINK
    NDS_P2_LINK_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
    NDS_P2_PIKACHU_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
    NDS_P2_YOSHI_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
    NDS_P2_NESS_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
    NDS_P2_PURIN_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
    NDS_P2_KIRBY_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
};

static const NDSRelocAssetEntry *ndsRelocAssetP2FighterAnimEntry(u32 asset_id)
{
    static NDSRelocAssetEntry entry;
    static char path[NDS_RELOC_MARIO_ANIM_PATH_CAPACITY];
    const NDSP2FighterAnimSegment *segment = NULL;
    size_t i;
    int written;

    if ((entry.path != NULL) && (entry.asset_id == asset_id))
    {
        return &entry;
    }
    for (i = 0u; i < (sizeof(sNdsP2FighterAnimSegments) /
                 sizeof(sNdsP2FighterAnimSegments[0])); i++)
    {
        if ((asset_id >= sNdsP2FighterAnimSegments[i].first) &&
            (asset_id <= sNdsP2FighterAnimSegments[i].last))
        {
            segment = &sNdsP2FighterAnimSegments[i];
            break;
        }
    }
    if (segment == NULL)
    {
        return NULL;
    }

    written = (int)ndsRelocFormatAnimPath(path, sizeof(path), segment->stem,
                                          asset_id - segment->zero_id);
    if ((written < 0) || ((size_t)written >= sizeof(path)))
    {
        return NULL;
    }
    entry.asset_id = asset_id;
    entry.file_id = asset_id;
    entry.path = path;
    return &entry;
}

static const NDSRelocAssetEntry *ndsRelocAssetFindEntry(u32 asset_id)
{
    size_t i;

    /* K0 line 7, "token -> file discovery". Every asset->path resolution in the
     * port funnels through here -- GetPath, ReadHeader, ReadExternFileIDs and
     * both payload loaders -- so one mark covers the family. */
    NDS_K0_MARK(gNdsK0AfterGoPathLookups, asset_id);
    if ((asset_id >= NDS_RELOC_MARIO_ANIM_FIRST) &&
        (asset_id <= NDS_RELOC_MARIO_ANIM_LAST))
    {
        return ndsRelocAssetMarioAnimEntry(asset_id);
    }
    if ((asset_id >= NDS_RELOC_FOX_ANIM_FIRST) &&
        (asset_id <= NDS_RELOC_FOX_ANIM_LAST))
    {
        return ndsRelocAssetFoxAnimEntry(asset_id);
    }
    {
        const NDSRelocAssetEntry *p2_anim =
            ndsRelocAssetP2FighterAnimEntry(asset_id);

        if (p2_anim != NULL)
        {
            return p2_anim;
        }
    }

    /* P4 donor files and synthesized motion files (include/nds/nds_p4.h):
     * donor IDs start above the vanilla table (0x854), DS-synthesized ones at
     * 0x1600, both far below the port's 0x10000 synthetic assets. */
    if ((asset_id >= 0x854u) && (asset_id < 0x10000u))
    {
        static NDSRelocAssetEntry p4_entry;
        const char *p4_path = ndsP4RelocAssetPath(asset_id);

        if (p4_path != NULL)
        {
            p4_entry.asset_id = asset_id;
            p4_entry.file_id = asset_id;
            p4_entry.path = p4_path;
            return &p4_entry;
        }
    }
    for (i = 0; i < (sizeof(sNdsRelocAssets) / sizeof(sNdsRelocAssets[0])); i++)
    {
        if (sNdsRelocAssets[i].asset_id == asset_id)
        {
            return &sNdsRelocAssets[i];
        }
    }
    return NULL;
}

const char *ndsRelocAssetGetPath(u32 asset_id)
{
    const NDSRelocAssetEntry *entry = ndsRelocAssetFindEntry(asset_id);

    return (entry != NULL) ? entry->path : NULL;
}

/* reloc/reloc_animations file ids, read once from the FNT (P2-2p8,
 * 2026-09-28). nitroromResolvePath walks a directory's FNT subtable one entry
 * at a time through the ROM read interface, and this directory holds 460
 * files: resolving one fighter clip measured ~2.6M ticks (about two frames of
 * WORK) inside the frame that changed action. Every Ness/Yoshi/Pikachu/Purin
 * motion (none is in the BPS1 stream pack) and every other fighter's AObj32
 * entry and spline clip takes that path. A Ness/Yoshi/Pikachu/Purin match
 * paid 394 of them: P95 6.03M, 1.54M with this table, replay digest identical
 * (artifacts/performance/2026-09-27_p2-2p8-fast-mem, section 61).
 *
 * File ids within one directory are consecutive in FNT order, so one buffered
 * pass over the subtable at boot yields (asset, file id) runs. Each member is
 * checked against the path ndsRelocAssetFindEntry formats for that asset, and
 * the loader still validates the O2R header's file id, so a foreign row can
 * only fall back to the walk, never load a wrong file. */
#define NDS_RELOC_ANIM_DIR_PATH "reloc/reloc_animations"
#define NDS_RELOC_ANIM_DIR_PREFIX "nitro:/" NDS_RELOC_ANIM_DIR_PATH "/"
#define NDS_RELOC_ANIM_DIR_RUNS_MAX 32u
#define NDS_RELOC_ANIM_DIR_CHUNK 256u

typedef struct NDSRelocAnimDirRun
{
    u16 asset_first;
    u16 count;
    u16 nitro_first;
    u16 pad;
} NDSRelocAnimDirRun;

static NDSRelocAnimDirRun sNdsRelocAnimDirRuns[NDS_RELOC_ANIM_DIR_RUNS_MAX];
static u16 sNdsRelocAnimDirRunCount;
static u16 sNdsRelocAnimDirAssetMin;
static u16 sNdsRelocAnimDirAssetMax;
__attribute__((used)) volatile u32 gNdsRelocAnimDirFiles;
__attribute__((used)) volatile u32 gNdsRelocAnimDirRows;
__attribute__((used)) volatile u32 gNdsRelocAnimDirFailStep;
__attribute__((used)) volatile u32 gNdsRelocAnimDirRuns;
__attribute__((used)) volatile u32 gNdsRelocAnimDirHits;

typedef struct NDSRelocAnimDirReader
{
    NitroRom *rom;
    u32 offset;
    u32 pos;
    u32 fill;
    u8 buf[NDS_RELOC_ANIM_DIR_CHUNK];
} NDSRelocAnimDirReader;

static s32 ndsRelocAnimDirReadByte(NDSRelocAnimDirReader *r)
{
    if (r->pos >= r->fill)
    {
        if (nitroromRead(r->rom, r->offset, r->buf, sizeof(r->buf)) == false)
        {
            return -1;
        }
        r->offset += sizeof(r->buf);
        r->pos = 0u;
        r->fill = sizeof(r->buf);
    }
    return r->buf[r->pos++];
}

static void ndsRelocAnimDirNote(u32 asset_id, u32 nitro_id)
{
    NDSRelocAnimDirRun *run;

    if (sNdsRelocAnimDirRunCount != 0u)
    {
        run = &sNdsRelocAnimDirRuns[sNdsRelocAnimDirRunCount - 1u];
        if ((asset_id == (u32)run->asset_first + run->count) &&
            (nitro_id == (u32)run->nitro_first + run->count))
        {
            run->count++;
            goto noted;
        }
    }
    if (sNdsRelocAnimDirRunCount >= NDS_RELOC_ANIM_DIR_RUNS_MAX)
    {
        /* Unlisted clips keep the directory walk. */
        return;
    }
    run = &sNdsRelocAnimDirRuns[sNdsRelocAnimDirRunCount++];
    run->asset_first = (u16)asset_id;
    run->count = 1u;
    run->nitro_first = (u16)nitro_id;
    run->pad = 0u;
noted:
    if ((gNdsRelocAnimDirRows == 0u) || (asset_id < sNdsRelocAnimDirAssetMin))
    {
        sNdsRelocAnimDirAssetMin = (u16)asset_id;
    }
    if (asset_id > sNdsRelocAnimDirAssetMax)
    {
        sNdsRelocAnimDirAssetMax = (u16)asset_id;
    }
    gNdsRelocAnimDirRows++;
}

/* The asset whose ndsRelocAssetFindEntry path is exactly this file, or 0. */
static u32 ndsRelocAnimDirAssetForName(const char *name, u32 len)
{
    static const char prefix[] = NDS_RELOC_ANIM_DIR_PREFIX;
    char full[sizeof(prefix) + NITROROM_NAME_MAX + 1u];
    const NDSRelocAssetEntry *entry;
    u32 digits = len;
    u32 number = 0u;
    u32 asset_id = 0u;
    u32 i;

    while ((digits != 0u) && (name[digits - 1u] >= '0') &&
           (name[digits - 1u] <= '9'))
    {
        digits--;
    }
    if ((digits == len) || ((len - digits) > 4u))
    {
        return 0u;
    }
    for (i = digits; i < len; i++)
    {
        number = (number * 10u) + (u32)(name[i] - '0');
    }
    if ((digits == 11u) && (memcmp(name, "FTMarioAnim", 11u) == 0))
    {
        asset_id = NDS_RELOC_MARIO_ANIM_FIRST + number;
    }
    else if ((digits == 9u) && (memcmp(name, "FTFoxAnim", 9u) == 0))
    {
        asset_id = NDS_RELOC_FOX_ANIM_FIRST + number;
    }
    else
    {
        for (i = 0u; i < (sizeof(sNdsP2FighterAnimSegments) /
                          sizeof(sNdsP2FighterAnimSegments[0])); i++)
        {
            const NDSP2FighterAnimSegment *segment =
                &sNdsP2FighterAnimSegments[i];
            u32 candidate = segment->zero_id + number;

            if ((strlen(segment->stem) == digits) &&
                (memcmp(name, segment->stem, digits) == 0) &&
                (candidate >= segment->first) && (candidate <= segment->last))
            {
                asset_id = candidate;
                break;
            }
        }
    }
    if ((asset_id == 0u) || (asset_id > 0xffffu))
    {
        return 0u;
    }
    entry = ndsRelocAssetFindEntry(asset_id);
    memcpy(full, prefix, sizeof(prefix) - 1u);
    memcpy(&full[sizeof(prefix) - 1u], name, len);
    full[sizeof(prefix) - 1u + len] = '\0';
    if ((entry == NULL) || (entry->path == NULL) ||
        (strcmp(ndsNitroPath(entry->path, sNdsNitroPathScratch), full) != 0))
    {
        return 0u;
    }
    return asset_id;
}

static s32 ndsRelocAssetOpenFighterStream(void);

static void ndsRelocAnimDirBuild(void)
{
    NDSRelocAnimDirReader reader;
    const NitroRomDir *dir;
    char name[NITROROM_NAME_MAX + 1u];
    NitroRom *rom = nitroromGetSelf();
    int dir_id;
    u32 file_id;

    sNdsRelocAnimDirRunCount = 0u;
    gNdsRelocAnimDirRows = 0u;
    gNdsRelocAnimDirFiles = 0u;
    if (rom == NULL)
    {
        gNdsRelocAnimDirFailStep = 1u;
        return;
    }
    dir_id = nitroromResolvePath(rom, NITROROM_ROOT_DIR,
                                 NDS_RELOC_ANIM_DIR_PATH);
    if ((dir_id < (s32)NITROROM_ROOT_DIR) ||
        ((u32)(dir_id - NITROROM_ROOT_DIR) >= rom->num_dirs))
    {
        gNdsRelocAnimDirFailStep = 2u;
        return;
    }
    dir = &rom->dir_table[dir_id - NITROROM_ROOT_DIR];
    reader.rom = rom;
    reader.offset = rom->fnt_offset + dir->subtable_offset;
    reader.pos = 0u;
    reader.fill = 0u;
    file_id = dir->file_id_base;
    for (;;)
    {
        s32 kind = ndsRelocAnimDirReadByte(&reader);
        u32 len;
        u32 i;

        if (kind <= 0)
        {
            if (kind < 0)
            {
                gNdsRelocAnimDirFailStep = 3u;
                sNdsRelocAnimDirRunCount = 0u;
            }
            break;
        }
        len = (u32)kind & 0x7fu;
        for (i = 0u; i < len; i++)
        {
            s32 c = ndsRelocAnimDirReadByte(&reader);

            if (c < 0)
            {
                gNdsRelocAnimDirFailStep = 3u;
                sNdsRelocAnimDirRunCount = 0u;
                return;
            }
            name[i] = (char)c;
        }
        if (((u32)kind & 0x80u) != 0u)
        {
            /* Subdirectory: its 16-bit id follows; it uses no file id. */
            if ((ndsRelocAnimDirReadByte(&reader) < 0) ||
                (ndsRelocAnimDirReadByte(&reader) < 0))
            {
                gNdsRelocAnimDirFailStep = 3u;
                sNdsRelocAnimDirRunCount = 0u;
                return;
            }
            continue;
        }
        if (file_id < rom->num_files)
        {
            u32 asset_id = ndsRelocAnimDirAssetForName(name, len);

            if (asset_id != 0u)
            {
                ndsRelocAnimDirNote(asset_id, file_id);
            }
        }
        NDS_DIAG(gNdsRelocAnimDirFiles++);
        file_id++;
    }
    gNdsRelocAnimDirRuns = sNdsRelocAnimDirRunCount;
}

static int ndsRelocAnimDirLookup(u32 asset_id)
{
    u32 i;

    if ((sNdsRelocAnimDirRunCount == 0u) ||
        (asset_id < sNdsRelocAnimDirAssetMin) ||
        (asset_id > sNdsRelocAnimDirAssetMax))
    {
        return -1;
    }
    for (i = 0u; i < sNdsRelocAnimDirRunCount; i++)
    {
        const NDSRelocAnimDirRun *run = &sNdsRelocAnimDirRuns[i];
        u32 delta = asset_id - run->asset_first;

        if (delta < run->count)
        {
            NDS_DIAG(gNdsRelocAnimDirHits++);
            return (int)(run->nitro_first + delta);
        }
    }
    return -1;
}

/* nitro:/reloc/p4 file ids, read once from the FNT (2026-10-12). The
 * directory holds every content's loose file (2,161 with eighteen contents),
 * and a resolve walks its subtable to the name, two ROM reads an entry
 * through the ARM7 storage path: the VS select's commit of Mad Piano's motion
 * file (11 KB) took 14 VBlanks in its five opens (header, extern ids, size,
 * data), and a content's select clips load the same way. One buffered pass at
 * boot keeps (asset, file id) runs -- the names are four hex digits, sorted,
 * so the runs ascend -- and the linker wraps nitroromResolvePath
 * (scripts/p4/p4_rules.mk): libfat's open, the direct loaders and every other
 * resolver answer "reloc/p4/<id>" from the runs. A name the runs lack walks
 * as before. */
#define NDS_RELOC_P4_DIR_PATH "reloc/p4"
#define NDS_RELOC_P4_DIR_RUNS_MAX 192u

typedef struct NDSRelocP4DirRun
{
    u16 asset_first;
    u16 count;
    u16 nitro_first;
} NDSRelocP4DirRun;

static NDSRelocP4DirRun sNdsRelocP4DirRuns[NDS_RELOC_P4_DIR_RUNS_MAX];
static u32 sNdsRelocP4DirRunCount;
static NitroRom *sNdsRelocP4DirRom;
__attribute__((used)) volatile u32 gNdsRelocP4DirFiles;
__attribute__((used)) volatile u32 gNdsRelocP4DirRuns;
__attribute__((used)) volatile u32 gNdsRelocP4DirFailStep;
__attribute__((used)) volatile u32 gNdsRelocP4DirHits;

/* A content file's name, four lowercase hex digits, as its asset id; or -1. */
static s32 ndsRelocP4DirParse(const char *name, u32 len)
{
    u32 value = 0u;
    u32 i;

    if (len != 4u)
    {
        return -1;
    }
    for (i = 0u; i < 4u; i++)
    {
        const char c = name[i];

        if ((c >= '0') && (c <= '9'))
        {
            value = (value << 4) | (u32)(c - '0');
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            value = (value << 4) | ((u32)(c - 'a') + 10u);
        }
        else
        {
            return -1;
        }
    }
    return (s32)value;
}

static void ndsRelocP4DirBuild(NitroRom *rom)
{
    NDSRelocAnimDirReader reader;
    const NitroRomDir *dir;
    char name[NITROROM_NAME_MAX + 1u];
    s32 last_asset = -1;
    int dir_id;
    u32 file_id;

    sNdsRelocP4DirRunCount = 0u;
    sNdsRelocP4DirRom = rom;
    if (rom == NULL)
    {
        gNdsRelocP4DirFailStep = 1u;
        return;
    }
    dir_id = nitroromResolvePath(rom, NITROROM_ROOT_DIR,
                                 NDS_RELOC_P4_DIR_PATH);
    if ((dir_id < (s32)NITROROM_ROOT_DIR) ||
        ((u32)(dir_id - NITROROM_ROOT_DIR) >= rom->num_dirs))
    {
        gNdsRelocP4DirFailStep = 2u;
        return;
    }
    dir = &rom->dir_table[dir_id - NITROROM_ROOT_DIR];
    reader.rom = rom;
    reader.offset = rom->fnt_offset + dir->subtable_offset;
    reader.pos = 0u;
    reader.fill = 0u;
    file_id = dir->file_id_base;
    for (;;)
    {
        NDSRelocP4DirRun *run;
        s32 kind = ndsRelocAnimDirReadByte(&reader);
        s32 asset;
        u32 len;
        u32 i;

        if (kind <= 0)
        {
            if (kind < 0)
            {
                gNdsRelocP4DirFailStep = 3u;
                sNdsRelocP4DirRunCount = 0u;
            }
            break;
        }
        len = (u32)kind & 0x7fu;
        for (i = 0u; i < len; i++)
        {
            s32 c = ndsRelocAnimDirReadByte(&reader);

            if (c < 0)
            {
                gNdsRelocP4DirFailStep = 3u;
                sNdsRelocP4DirRunCount = 0u;
                return;
            }
            name[i] = (char)c;
        }
        if (((u32)kind & 0x80u) != 0u)
        {
            /* Subdirectory: its 16-bit id follows; it uses no file id. */
            if ((ndsRelocAnimDirReadByte(&reader) < 0) ||
                (ndsRelocAnimDirReadByte(&reader) < 0))
            {
                gNdsRelocP4DirFailStep = 3u;
                sNdsRelocP4DirRunCount = 0u;
                return;
            }
            continue;
        }
        asset = ndsRelocP4DirParse(name, len);
        if (asset < 0)
        {
            /* A foreign name keeps its file id out of every run. */
            file_id++;
            continue;
        }
        if ((asset <= last_asset) || (file_id >= rom->num_files))
        {
            /* Out of order: the runs must ascend for the search below;
             * the names from here on walk. */
            gNdsRelocP4DirFailStep = 4u;
            break;
        }
        last_asset = asset;
        run = (sNdsRelocP4DirRunCount != 0u) ?
            &sNdsRelocP4DirRuns[sNdsRelocP4DirRunCount - 1u] : NULL;
        if ((run != NULL) &&
            ((u32)asset == (u32)run->asset_first + run->count) &&
            (file_id == (u32)run->nitro_first + run->count))
        {
            run->count++;
        }
        else if (sNdsRelocP4DirRunCount < NDS_RELOC_P4_DIR_RUNS_MAX)
        {
            run = &sNdsRelocP4DirRuns[sNdsRelocP4DirRunCount++];
            run->asset_first = (u16)asset;
            run->count = 1u;
            run->nitro_first = (u16)file_id;
        }
        else
        {
            gNdsRelocP4DirFailStep = 5u;
            break;
        }
        gNdsRelocP4DirFiles++;
        file_id++;
    }
    gNdsRelocP4DirRuns = sNdsRelocP4DirRunCount;
}

static int ndsRelocP4DirLookup(u32 asset_id)
{
    u32 lo = 0u;
    u32 hi = sNdsRelocP4DirRunCount;

    while (lo < hi)
    {
        const u32 mid = (lo + hi) >> 1;
        const NDSRelocP4DirRun *run = &sNdsRelocP4DirRuns[mid];

        if (asset_id < run->asset_first)
        {
            hi = mid;
        }
        else if ((asset_id - run->asset_first) >= run->count)
        {
            lo = mid + 1u;
        }
        else
        {
            NDS_DIAG(gNdsRelocP4DirHits++);
            return (int)(run->nitro_first + (asset_id - run->asset_first));
        }
    }
    return -1;
}

int __real_nitroromResolvePath(NitroRom *nr, u16 base_dir, const char *path);
int __wrap_nitroromResolvePath(NitroRom *nr, u16 base_dir, const char *path);

int __wrap_nitroromResolvePath(NitroRom *nr, u16 base_dir, const char *path)
{
    static const char prefix[] = NDS_RELOC_P4_DIR_PATH "/";
    const char *p = path;

    if ((sNdsRelocP4DirRunCount != 0u) && (nr == sNdsRelocP4DirRom) &&
        (p != NULL))
    {
        while (*p == '/')
        {
            p++;
        }
        /* An absolute path, or one from the root (what libfat's open of
         * "nitro:/reloc/p4/<id>" and the direct loaders pass). */
        if (((p != path) || (base_dir == NITROROM_ROOT_DIR)) &&
            (strncmp(p, prefix, sizeof(prefix) - 1u) == 0))
        {
            const char *leaf = p + (sizeof(prefix) - 1u);
            s32 asset = ndsRelocP4DirParse(leaf, (u32)strlen(leaf));

            if (asset >= 0)
            {
                int id = ndsRelocP4DirLookup((u32)asset);

                if (id >= 0)
                {
                    return id;
                }
            }
        }
    }
    return __real_nitroromResolvePath(nr, base_dir, path);
}

/* libfat's disc cache, which nitroFSInit creates for a ROM read through DLDI
 * or the DSi SD: libnds's weak default (dvm_calico.o) is 16 pages of 8
 * sectors, 65,832 B of libc heap below the taskman arena for the whole run.
 * NitroFS reads are whole files at a time; the cache's lasting use is the FAT
 * sectors of the ROM file's cluster chain (about 22 sectors for a 90 MB ROM
 * on 32 KB clusters), which eight pages still hold. The other 32 KB go to
 * every scene's arena (2026-10-11). */
unsigned g_dvmDefaultCachePages = 8u;

void ndsRelocAssetsInit(void)
{
    if (nitroFSInit(NULL))
    {
        gNdsRelocAssetInitResult = NDS_RELOC_ASSET_INIT_PASS;
        ndsRelocAnimDirBuild();
        ndsRelocP4DirBuild(nitroromGetSelf());
        /* Its resident directory is read in 256 B chunks: do that on the
         * boot stack, not inside the first frame that streams a clip. */
        (void)ndsRelocAssetOpenFighterStream();
#if NDS_BOOT_DIAG_TEXT
        /* P2-1L (11). gNdsRelocAssetInitResult is what every verifier reads;
         * the console line is the human copy, and the owner's free-play ROM
         * builds with NDS_BOOT_DIAG_TEXT := 0 so its bottom screen is clean. */
        ndsConsolePrintf("NitroFS: PASS\n");
#endif
    }
    else
    {
        gNdsRelocAssetInitResult = 0;
#if NDS_BOOT_DIAG_TEXT
        ndsConsolePrintf("NitroFS: FAIL\n");
#endif
    }
}

s32 ndsRelocAssetReadHeaderFromFile(FILE *file, u32 expected_file_id,
                                            NDSRelocAssetHeader *out_header,
                                            long *out_data_offset)
{
    u8 header[NDS_O2R_RESOURCE_HEADER_SIZE + 16];
    u32 extern_count;
    long data_size_offset;
    long data_offset;

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    if (memcmp(&header[NDS_O2R_MAGIC_OFFSET], NDS_O2R_MAGIC, 4) != 0)
    {
        gNdsRelocAssetFormatFailCount++;
        return FALSE;
    }

    out_header->file_id = ndsReadLe32(&header[NDS_O2R_RESOURCE_HEADER_SIZE]);
    out_header->reloc_intern_offset =
        ndsReadLe16(&header[NDS_O2R_RESOURCE_HEADER_SIZE + 4]);
    out_header->reloc_extern_offset =
        ndsReadLe16(&header[NDS_O2R_RESOURCE_HEADER_SIZE + 6]);
    extern_count = ndsReadLe32(&header[NDS_O2R_RESOURCE_HEADER_SIZE + 8]);
    out_header->extern_file_ids_num = extern_count;

    if (out_header->file_id != expected_file_id)
    {
        gNdsRelocAssetFormatFailCount++;
        return FALSE;
    }

    data_size_offset = NDS_O2R_RESOURCE_HEADER_SIZE + 12 + ((long)extern_count * 2);
    if (fseek(file, data_size_offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    if (fread(header, 1, 4, file) != 4)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    out_header->data_size = ndsReadLe32(header);

    data_offset = data_size_offset + 4;
    if (out_data_offset != NULL)
    {
        *out_data_offset = data_offset;
    }
    return TRUE;
}

static s32 ndsRelocAssetReadHeaderUnlocked(u32 asset_id, NDSRelocAssetHeader *out_header)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    long data_offset;
    s32 ok;

    if (out_header == NULL)
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(ndsNitroPath(entry->path, sNdsNitroPathScratch), "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, entry->file_id, out_header, &data_offset);
    ndsTaskmanSampleLibcHeapNow();
    fclose(file);

    if (ok != FALSE)
    {
        gNdsRelocAssetHeaderReadCount++;
    }
    return ok;
}

s32 ndsRelocAssetReadHeader(u32 asset_id, NDSRelocAssetHeader *out_header)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetReadHeaderUnlocked(asset_id, out_header);
    ndsFsUnlock();
    return result;
}

static s32 ndsRelocAssetReadExternFileIDsUnlocked(u32 asset_id, u16 *out_file_ids,
                                   u32 capacity, u32 *out_count)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;
    u32 i;

    if (out_count != NULL)
    {
        *out_count = 0;
    }
    if ((out_file_ids == NULL) && (capacity != 0))
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(ndsNitroPath(entry->path, sNdsNitroPathScratch), "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if (header.extern_file_ids_num > capacity)
    {
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        gNdsRelocAssetFormatFailCount++;
        return FALSE;
    }
    if (fseek(file, NDS_O2R_RESOURCE_HEADER_SIZE + 12, SEEK_SET) != 0)
    {
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }

    for (i = 0; i < header.extern_file_ids_num; i++)
    {
        u8 id_bytes[2];

        if (fread(id_bytes, 1, sizeof(id_bytes), file) != sizeof(id_bytes))
        {
            ndsTaskmanSampleLibcHeapNow();
            fclose(file);
            gNdsRelocAssetShortReadCount++;
            return FALSE;
        }
        out_file_ids[i] = ndsReadLe16(id_bytes);
    }
    ndsTaskmanSampleLibcHeapNow();
    fclose(file);

    if (out_count != NULL)
    {
        *out_count = header.extern_file_ids_num;
    }
    return TRUE;
}

s32 ndsRelocAssetReadExternFileIDs(u32 asset_id, u16 *out_file_ids, u32 capacity, u32 *out_count)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetReadExternFileIDsUnlocked(asset_id, out_file_ids, capacity, out_count);
    ndsFsUnlock();
    return result;
}

static s32 ndsRelocAssetLoadDataUnlocked(u32 asset_id, void *dst, size_t dst_capacity,
                           NDSRelocAssetHeader *out_header)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;
    s32 ok;

    if (dst == NULL)
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(ndsNitroPath(entry->path, sNdsNitroPathScratch), "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header, &data_offset);
    if (ok == FALSE)
    {
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if ((size_t)header.data_size > dst_capacity)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    /* K0 lines 1 and 2, marked in every payload loader rather than only in the
     * one the acquisition path happens to use today -- a "zero" must not be
     * able to hide behind an uninstrumented door. */
    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);
    if (fseek(file, data_offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    ndsTaskmanSampleLibcHeapNow();
    fclose(file);

    if (out_header != NULL)
    {
        *out_header = header;
    }
    gNdsRelocAssetPayloadReadCount++;
    NDS_K0_MARK(gNdsK0AfterGoFatReads, asset_id);
    return TRUE;
}

s32 ndsRelocAssetLoadData(u32 asset_id, void *dst, size_t dst_capacity, NDSRelocAssetHeader *out_header)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetLoadDataUnlocked(asset_id, dst, dst_capacity, out_header);
    ndsFsUnlock();
    return result;
}

static s32 ndsRelocAssetLoadDataAndExternIDsUnlocked(u32 asset_id, void *dst,
                                      size_t dst_capacity,
                                      NDSRelocAssetHeader *out_header,
                                      u16 *out_file_ids, u32 file_id_capacity,
                                      u32 *out_file_id_count)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;
    u32 i;

    if (out_file_id_count != NULL)
    {
        *out_file_id_count = 0u;
    }
    if ((dst == NULL) || ((out_file_ids == NULL) && (file_id_capacity != 0u)))
    {
        return FALSE;
    }
    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }
    file = fopen(ndsNitroPath(entry->path, sNdsNitroPathScratch), "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }
    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);
    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    gNdsRelocAssetHeaderReadCount++;
    if ((header.extern_file_ids_num > file_id_capacity) ||
        ((size_t)header.data_size > dst_capacity))
    {
        if (header.extern_file_ids_num > file_id_capacity)
        {
            gNdsRelocAssetFormatFailCount++;
        }
        else
        {
            gNdsRelocAssetShortReadCount++;
        }
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if (header.extern_file_ids_num != 0u)
    {
        if (fseek(file, NDS_O2R_RESOURCE_HEADER_SIZE + 12, SEEK_SET) != 0)
        {
            gNdsRelocAssetShortReadCount++;
            ndsTaskmanSampleLibcHeapNow();
            fclose(file);
            return FALSE;
        }
        for (i = 0u; i < header.extern_file_ids_num; i++)
        {
            u8 id_bytes[2];

            if (fread(id_bytes, 1u, sizeof(id_bytes), file) !=
                sizeof(id_bytes))
            {
                gNdsRelocAssetShortReadCount++;
                ndsTaskmanSampleLibcHeapNow();
                fclose(file);
                return FALSE;
            }
            out_file_ids[i] = ndsReadLe16(id_bytes);
        }
    }
    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);
    if (fseek(file, data_offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1u, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    ndsTaskmanSampleLibcHeapNow();
    fclose(file);

    if (out_header != NULL)
    {
        *out_header = header;
    }
    if (out_file_id_count != NULL)
    {
        *out_file_id_count = header.extern_file_ids_num;
    }
    gNdsRelocAssetPayloadReadCount++;
    NDS_K0_MARK(gNdsK0AfterGoFatReads, asset_id);
    return TRUE;
}

s32 ndsRelocAssetLoadDataAndExternIDs(u32 asset_id, void *dst, size_t dst_capacity, NDSRelocAssetHeader *out_header, u16 *out_file_ids, u32 file_id_capacity, u32 *out_file_id_count)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetLoadDataAndExternIDsUnlocked(asset_id, dst, dst_capacity, out_header, out_file_ids, file_id_capacity, out_file_id_count);
    ndsFsUnlock();
    return result;
}

/* Task 76. The same redundancy one level up, and the larger half of it. A
 * caller that must zero its destination before the read needs the payload size
 * first, and the only way it had to learn that was ndsRelocAssetAllocSize --
 * which opens the file and parses the header this load then re-reads. On the
 * fighter-animation path that is a whole second NitroFS directory walk per
 * load, inside the frame that needs the move (Task 71).
 *
 * One open supplies both. `align` is the caller's allocation granularity; the
 * zeroed region is data_size rounded up to it, which is exactly the region the
 * caller used to memset from ndsRelocAssetAllocSize's return, and it is
 * reported back so the caller can size its own failure handling.
 *
 * The zero happens after the header is known and before the payload read, so
 * the bytes left in dst are identical to the old memset-then-load order. On
 * every failure path reachable once the header is known, dst is left fully
 * zeroed -- which is what the animation caller's `fail:` memset did. */

#define NDS_FTANIM_STREAM_MAGIC 0x31535042u /* "BPS1", little endian */
#define NDS_FTANIM_STREAM_VERSION 1u
#define NDS_FTANIM_STREAM_PATH "animation/ftanim_stream_pack.bin"

typedef struct NDSFtAnimStreamHeader
{
    u32 magic;
    u32 version;
    u32 blob_bytes;
    u32 clip_count;
    u32 first_id;
    u32 last_id;
    u32 dir_off;
    u32 data_off;
} NDSFtAnimStreamHeader;

typedef struct NDSFtAnimStreamEntry
{
    u32 offset;
    u32 size;
} NDSFtAnimStreamEntry;

#define NDS_FTANIM_STREAM_DIR_MAX_ENTRIES 1600u
/* A resident row packs (offset / 16) << 14 | size: clips are 16-byte aligned
 * (emit_stream_pack) and the largest is 11,568 B, so 18 + 14 bits cover a
 * 4 MiB pack exactly. A row that does not fit leaves the resident copy off. */
#define NDS_FTANIM_STREAM_DIR_SIZE_BITS 14u
#define NDS_FTANIM_STREAM_DIR_CHUNK 32u

typedef struct NDSFtAnimStreamDirRouteCell
{
    u32 route;
    u32 pad[7];
} NDSFtAnimStreamDirRouteCell;

_Static_assert(sizeof(NDSFtAnimStreamDirRouteCell) == 32u,
               "fighter-stream directory route must own one D-cache line");

static NitroRom *sNdsFtAnimStreamRom;
static NDSFtAnimStreamHeader sNdsFtAnimStreamHeader;
/* BPS1's dense directory is 1,599 rows (Mario through Pikachu, 2026-09-28).
 * It is immutable metadata, and every live clip request used to issue a
 * separate NitroROM range read just to recover these two words before reading
 * the payload.  Warm/cache acquisition asks once for size and once for bytes,
 * so that path paid the row read twice.  Keep a measured, bounded resident
 * copy, packed to 4 B a row (6,400 B; the 8 B rows it replaced were 9,600 B
 * for 1,200); a future wider pack simply leaves this disabled and uses the
 * established per-row read below.
 *
 * The route cell owns a complete D-cache line because the tick sampler pokes it
 * for same-ROM A/B.  A bare aligned u32 can share a line with unrelated writes
 * and get restored by a later writeback (the fighter draw-memo route already
 * hit that exact failure mode). */
static u32 sNdsFtAnimStreamDir[NDS_FTANIM_STREAM_DIR_MAX_ENTRIES]
    __attribute__((aligned(32)));
static u16 sNdsFtAnimStreamFileId;
static u8 sNdsFtAnimStreamState;
static u16 sNdsFtAnimStreamDirCount;
static u8 sNdsFtAnimStreamDirReady;
__attribute__((used, section(".data"), aligned(32)))
volatile NDSFtAnimStreamDirRouteCell gNdsRelocAssetFighterStreamDirRoute = {
    1u, { 0u, 0u, 0u, 0u, 0u, 0u, 0u }
};
__attribute__((used)) volatile u32 gNdsRelocAssetFighterStreamDirBytes;
__attribute__((used)) volatile u32 gNdsRelocAssetFighterStreamDirHits;
__attribute__((used)) volatile u32 gNdsRelocAssetFighterStreamDirFallbackReads;
__attribute__((used)) volatile u32 gNdsRelocAssetFighterStreamDirLoadFailures;
static s32 ndsRelocAssetOpenFighterStream(void)
{
    NDSFtAnimStreamHeader *header = &sNdsFtAnimStreamHeader;
    NitroRom *rom;
    u32 file_size;
    u32 dense_count;
    int file_id;

    if (sNdsFtAnimStreamState != 0u)
    {
        return (sNdsFtAnimStreamState == 1u) ? TRUE : FALSE;
    }
    rom = nitroromGetSelf();
    if (rom == NULL)
    {
        goto fail;
    }
    file_id = nitroromResolvePath(rom, NITROROM_ROOT_DIR,
                                  NDS_FTANIM_STREAM_PATH);
    if ((file_id < 0) || (file_id >= (s32)NITROROM_ROOT_DIR))
    {
        goto fail;
    }
    file_size = nitroromGetFileSize(rom, (u16)file_id);
    if ((file_size < sizeof(*header)) ||
        (nitroromReadFile(rom, (u16)file_id, 0u, header,
                          sizeof(*header)) == false))
    {
        goto fail;
    }
    if ((header->magic != NDS_FTANIM_STREAM_MAGIC) ||
        (header->version != NDS_FTANIM_STREAM_VERSION) ||
        (header->blob_bytes != file_size) ||
        (header->first_id > header->last_id))
    {
        goto fail;
    }
    dense_count = header->last_id - header->first_id + 1u;
    if ((header->clip_count > dense_count) ||
        (header->dir_off < sizeof(*header)) ||
        (dense_count > ((0xffffffffu - header->dir_off) /
                        sizeof(NDSFtAnimStreamEntry))) ||
        ((header->dir_off + dense_count * sizeof(NDSFtAnimStreamEntry)) >
         header->data_off) ||
        (header->data_off > file_size))
    {
        goto fail;
    }
    sNdsFtAnimStreamRom = rom;
    sNdsFtAnimStreamFileId = (u16)file_id;
    sNdsFtAnimStreamDirCount = 0u;
    sNdsFtAnimStreamDirReady = FALSE;
    gNdsRelocAssetFighterStreamDirBytes = 0u;
    if (dense_count <= NDS_FTANIM_STREAM_DIR_MAX_ENTRIES)
    {
        NDSFtAnimStreamEntry chunk[NDS_FTANIM_STREAM_DIR_CHUNK];
        u32 row = 0u;

        while (row < dense_count)
        {
            u32 n = dense_count - row;
            u32 i;

            if (n > NDS_FTANIM_STREAM_DIR_CHUNK)
            {
                n = NDS_FTANIM_STREAM_DIR_CHUNK;
            }
            if (nitroromReadFile(rom, (u16)file_id,
                                 header->dir_off +
                                     (row * sizeof(NDSFtAnimStreamEntry)),
                                 chunk, n * sizeof(NDSFtAnimStreamEntry)) ==
                false)
            {
                break;
            }
            for (i = 0u; i < n; i++)
            {
                u32 offset = chunk[i].offset;
                u32 size = chunk[i].size;

                if (((offset & 15u) != 0u) ||
                    ((offset >> 4) >=
                     (1u << (32u - NDS_FTANIM_STREAM_DIR_SIZE_BITS))) ||
                    (size >= (1u << NDS_FTANIM_STREAM_DIR_SIZE_BITS)))
                {
                    break;
                }
                sNdsFtAnimStreamDir[row + i] =
                    ((offset >> 4) << NDS_FTANIM_STREAM_DIR_SIZE_BITS) | size;
            }
            if (i != n)
            {
                break;
            }
            row += n;
        }
        if (row == dense_count)
        {
            sNdsFtAnimStreamDirCount = (u16)dense_count;
            sNdsFtAnimStreamDirReady = TRUE;
            gNdsRelocAssetFighterStreamDirBytes = dense_count * sizeof(u32);
        }
        else
        {
            /* Residency is only a performance route.  The validated stream is
             * still usable through the exact per-row reader below. */
            gNdsRelocAssetFighterStreamDirLoadFailures++;
        }
    }
    sNdsFtAnimStreamState = 1u;
    return TRUE;

fail:
    sNdsFtAnimStreamState = 2u;
    gNdsRelocAssetFighterStreamFailures++;
    return FALSE;
}

/* THE P4 CLIP STREAM (2026-10-12). The contents' and Kirby's powers'
 * FIGATREE clips in the cast pack's BPS1 format, ids past the cast's
 * (scripts/p4/p4_anim_stream.py). A content's clip was a loose O2R file in
 * nitro:/reloc/p4, and an animation-cache miss paid the FAT path walk, the
 * O2R header and relocation reads, the byte swap and the AObj16 normalizers:
 * four Banjos' over-gate frames spent ~330K cycles in storage and ~190K in
 * the normalizers (task37 census, build-prof-banjo). Its directory (3,717
 * dense rows) stays in the ROM: a miss reads the clip's 8-byte row once (the
 * size query and the read that follows it share the memo), then the clip.
 * An event32 or spline clip has an empty row and still loads loose.
 *
 * 2026-10-13: the rows read stay in a direct-mapped memo by id. Most misses
 * re-fetch a clip the animation cache dropped (four Meta Knights: 240
 * misses over 95 clip ids in 1,200 frames), and each paid a second storage
 * round trip for its 8-byte row. A row is the ROM's, so it never goes stale;
 * an id is past the cast's pack, so 0 marks an empty way. */
#define NDS_FTANIM_P4_STREAM_PATH "animation/ftanim_p4_stream.bin"
#define NDS_FTANIM_P4_ROW_MEMO 128u
static NitroRom *sNdsFtAnimP4StreamRom;
static NDSFtAnimStreamHeader sNdsFtAnimP4StreamHeader;
static u16 sNdsFtAnimP4StreamFileId;
static u8 sNdsFtAnimP4StreamState;
static u16 sNdsFtAnimP4RowMemoId[NDS_FTANIM_P4_ROW_MEMO];
static NDSFtAnimStreamEntry sNdsFtAnimP4RowMemo[NDS_FTANIM_P4_ROW_MEMO];
__attribute__((used)) volatile u32 gNdsRelocAssetP4StreamRowReads;

/* 2026-10-24 (P8 P7): in a VS battle, the directory a sector at a time. A
 * heavy match's four contents (Crash, Sonic, Mr. Game & Watch, Palutena)
 * fetch 578 distinct clips, and the memo above re-read 322 rows on Hyrule
 * Temple -- a storage round trip each, a third of the match's 917. A 512-byte
 * sector of the directory costs the round trip an 8-byte row does and holds
 * 64 rows, and a content's clip ids lie close together, so a match's rows are
 * a few sectors. A page is taken when first needed: from the loaned range's
 * idle tail, else from the general heap while it keeps the floor the
 * optional caches keep (25,600 B and the match's later growth, as
 * NDS_FTR_LEAN_SPARE_KEEP_FREE); it lives as long as the heap generation.
 * Without one -- or outside a VS battle, whose menus budget their heap
 * themselves -- the memo above serves. A full set replaces its pages in
 * turn. */
#define NDS_FTANIM_P4_PAGE_BYTES 512u
#define NDS_FTANIM_P4_PAGES 24u
#define NDS_FTANIM_P4_PAGE_KEEP_FREE (25600u + 24576u)
static u8 *sNdsFtAnimP4Page[NDS_FTANIM_P4_PAGES];
static u32 sNdsFtAnimP4PageSector[NDS_FTANIM_P4_PAGES];  /* sector + 1; 0 empty */
static u32 sNdsFtAnimP4PageCount;
static u32 sNdsFtAnimP4PageNext;
static u32 sNdsFtAnimP4PageGen;   /* gNdsTaskmanHeapGeneration + 1 */
__attribute__((used)) volatile u32 gNdsRelocAssetP4StreamPageReads;

/* The row at directory byte `at` of the P4 pack, from its sector's page, or
 * NULL when no page holds it (the caller reads the row alone). */
static const NDSFtAnimStreamEntry *ndsRelocAssetP4StreamPageRow(u32 at)
{
    const u32 sector = at / NDS_FTANIM_P4_PAGE_BYTES;
    const u32 within = at % NDS_FTANIM_P4_PAGE_BYTES;
    u32 bytes;
    u32 slot;
    u32 i;

    if (((within + (u32)sizeof(NDSFtAnimStreamEntry)) >
         NDS_FTANIM_P4_PAGE_BYTES) ||
        (ndsSceneIsVSBattle() == FALSE))
    {
        return NULL;
    }
    if (sNdsFtAnimP4PageGen != gNdsTaskmanHeapGeneration + 1u)
    {
        sNdsFtAnimP4PageGen = gNdsTaskmanHeapGeneration + 1u;
        sNdsFtAnimP4PageCount = 0u;
        sNdsFtAnimP4PageNext = 0u;
    }
    for (i = 0u; i < sNdsFtAnimP4PageCount; i++)
    {
        if (sNdsFtAnimP4PageSector[i] == sector + 1u)
        {
            return (const NDSFtAnimStreamEntry *)(const void *)
                (sNdsFtAnimP4Page[i] + within);
        }
    }
    if (sNdsFtAnimP4PageCount < NDS_FTANIM_P4_PAGES)
    {
        u8 *page = ndsVSBattleOverlayTryAlloc(NDS_FTANIM_P4_PAGE_BYTES, 32u);

        if ((page == NULL) &&
            (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                                 NDS_FTANIM_P4_PAGE_BYTES +
                                     NDS_FTANIM_P4_PAGE_KEEP_FREE,
                                 32u) != FALSE))
        {
            page = syTaskmanMalloc(NDS_FTANIM_P4_PAGE_BYTES, 32u);
        }
        if (page == NULL)
        {
            return NULL;
        }
        slot = sNdsFtAnimP4PageCount++;
        sNdsFtAnimP4Page[slot] = page;
    }
    else
    {
        slot = sNdsFtAnimP4PageNext;
        sNdsFtAnimP4PageNext = (slot + 1u) % NDS_FTANIM_P4_PAGES;
    }
    sNdsFtAnimP4PageSector[slot] = 0u;
    bytes = sNdsFtAnimP4StreamHeader.blob_bytes -
        (sector * NDS_FTANIM_P4_PAGE_BYTES);
    if (bytes > NDS_FTANIM_P4_PAGE_BYTES)
    {
        bytes = NDS_FTANIM_P4_PAGE_BYTES;
    }
    if (nitroromReadFile(sNdsFtAnimP4StreamRom, sNdsFtAnimP4StreamFileId,
                         sector * NDS_FTANIM_P4_PAGE_BYTES,
                         sNdsFtAnimP4Page[slot], bytes) == false)
    {
        return NULL;
    }
    sNdsFtAnimP4PageSector[slot] = sector + 1u;
    gNdsRelocAssetP4StreamPageReads++;
    return (const NDSFtAnimStreamEntry *)(const void *)
        (sNdsFtAnimP4Page[slot] + within);
}

static s32 ndsRelocAssetOpenP4Stream(void)
{
    NDSFtAnimStreamHeader *header = &sNdsFtAnimP4StreamHeader;
    NitroRom *rom;
    u32 file_size;
    u32 dense_count;
    int file_id;

    if (sNdsFtAnimP4StreamState != 0u)
    {
        return (sNdsFtAnimP4StreamState == 1u) ? TRUE : FALSE;
    }
    sNdsFtAnimP4StreamState = 2u;
    rom = nitroromGetSelf();
    if (rom == NULL)
    {
        return FALSE;
    }
    file_id = nitroromResolvePath(rom, NITROROM_ROOT_DIR,
                                  NDS_FTANIM_P4_STREAM_PATH);
    if ((file_id < 0) || (file_id >= (s32)NITROROM_ROOT_DIR))
    {
        return FALSE;
    }
    file_size = nitroromGetFileSize(rom, (u16)file_id);
    if ((file_size < sizeof(*header)) ||
        (nitroromReadFile(rom, (u16)file_id, 0u, header,
                          sizeof(*header)) == false) ||
        (header->magic != NDS_FTANIM_STREAM_MAGIC) ||
        (header->version != NDS_FTANIM_STREAM_VERSION) ||
        (header->blob_bytes != file_size) ||
        (header->first_id > header->last_id))
    {
        return FALSE;
    }
    dense_count = header->last_id - header->first_id + 1u;
    if ((header->clip_count > dense_count) ||
        (header->dir_off < sizeof(*header)) ||
        (dense_count > ((0xffffffffu - header->dir_off) /
                        sizeof(NDSFtAnimStreamEntry))) ||
        ((header->dir_off + dense_count * sizeof(NDSFtAnimStreamEntry)) >
         header->data_off) ||
        (header->data_off > file_size))
    {
        return FALSE;
    }
    sNdsFtAnimP4StreamRom = rom;
    sNdsFtAnimP4StreamFileId = (u16)file_id;
    sNdsFtAnimP4StreamState = 1u;
    return TRUE;
}

/* The P4 pack's row for `asset_id`: FALSE outside its range, or for an empty
 * or malformed row. */
static s32 ndsRelocAssetP4StreamEntry(u32 asset_id,
                                      NDSFtAnimStreamEntry *entry)
{
    const NDSFtAnimStreamHeader *header = &sNdsFtAnimP4StreamHeader;

    if ((ndsRelocAssetOpenP4Stream() == FALSE) ||
        (asset_id < header->first_id) || (asset_id > header->last_id))
    {
        return FALSE;
    }
    {
        const u32 at = header->dir_off +
            ((asset_id - header->first_id) * (u32)sizeof(NDSFtAnimStreamEntry));
        const NDSFtAnimStreamEntry *row = ndsRelocAssetP4StreamPageRow(at);
        const u32 way = asset_id & (NDS_FTANIM_P4_ROW_MEMO - 1u);

        if (row != NULL)
        {
            *entry = *row;
        }
        else
        {
            if ((u32)sNdsFtAnimP4RowMemoId[way] != asset_id)
            {
                sNdsFtAnimP4RowMemoId[way] = 0u;
                if ((asset_id > 0xffffu) ||
                    (nitroromReadFile(sNdsFtAnimP4StreamRom,
                                      sNdsFtAnimP4StreamFileId, at,
                                      &sNdsFtAnimP4RowMemo[way],
                                      sizeof(NDSFtAnimStreamEntry)) == false))
                {
                    return FALSE;
                }
                sNdsFtAnimP4RowMemoId[way] = (u16)asset_id;
                gNdsRelocAssetP4StreamRowReads++;
            }
            *entry = sNdsFtAnimP4RowMemo[way];
        }
    }
    return ((entry->offset >= header->data_off) && (entry->size != 0u) &&
            (entry->offset <= header->blob_bytes) &&
            (entry->size <= (header->blob_bytes - entry->offset))) ?
        TRUE : FALSE;
}

/* ndsRelocAssetLoadFighterStreamClip for an id past the cast's pack. */
static s32 ndsRelocAssetLoadP4StreamClip(u32 asset_id, void *dst,
                                         u32 *out_size)
{
    NDSFtAnimStreamEntry entry;

    if (ndsRelocAssetP4StreamEntry(asset_id, &entry) == FALSE)
    {
        gNdsRelocAssetFighterStreamMisses++;
        return FALSE;
    }
    if (out_size != NULL)
    {
        *out_size = entry.size;
    }
    if (dst == NULL)
    {
        return TRUE;
    }
    if (nitroromReadFile(sNdsFtAnimP4StreamRom, sNdsFtAnimP4StreamFileId,
                         entry.offset, dst, entry.size) == false)
    {
        gNdsRelocAssetFighterStreamFailures++;
        return FALSE;
    }
    gNdsRelocAssetFighterStreamReads++;
    return TRUE;
}

s32 ndsRelocAssetLoadFighterStreamClip(u32 asset_id, void *dst,
                                       u32 *out_size)
{
    if (out_size != NULL)
    {
        *out_size = 0u;
    }
    {
        NDSFtAnimStreamEntry entry;
        const NDSFtAnimStreamHeader *header = &sNdsFtAnimStreamHeader;
        u32 row;

        if (gNdsRelocAssetFighterStreamDispatch == 0u)
        {
            gNdsRelocAssetFighterStreamMisses++;
            return FALSE;
        }
        /* Past the cast's pack: the contents' (their ids follow the cast's). */
        if ((ndsRelocAssetOpenFighterStream() != FALSE) &&
            (asset_id > header->last_id))
        {
            return ndsRelocAssetLoadP4StreamClip(asset_id, dst, out_size);
        }
        if ((ndsRelocAssetOpenFighterStream() == FALSE) ||
            (asset_id < header->first_id) || (asset_id > header->last_id))
        {
            gNdsRelocAssetFighterStreamMisses++;
            return FALSE;
        }
        row = asset_id - header->first_id;
        if ((gNdsRelocAssetFighterStreamDirRoute.route != 0u) &&
            (sNdsFtAnimStreamDirReady != FALSE) &&
            (row < sNdsFtAnimStreamDirCount))
        {
            u32 packed = sNdsFtAnimStreamDir[row];

            entry.offset =
                (packed >> NDS_FTANIM_STREAM_DIR_SIZE_BITS) << 4;
            entry.size =
                packed & ((1u << NDS_FTANIM_STREAM_DIR_SIZE_BITS) - 1u);
            gNdsRelocAssetFighterStreamDirHits++;
        }
        else
        {
            u32 row_offset = header->dir_off +
                (row * sizeof(NDSFtAnimStreamEntry));

            if (nitroromReadFile(sNdsFtAnimStreamRom,
                                 sNdsFtAnimStreamFileId, row_offset, &entry,
                                 sizeof(entry)) == false)
            {
                gNdsRelocAssetFighterStreamFailures++;
                return FALSE;
            }
            gNdsRelocAssetFighterStreamDirFallbackReads++;
        }
        if ((entry.offset < header->data_off) || (entry.size == 0u) ||
            (entry.offset > header->blob_bytes) ||
            (entry.size > (header->blob_bytes - entry.offset)))
        {
            gNdsRelocAssetFighterStreamMisses++;
            return FALSE;
        }
        if (out_size != NULL)
        {
            *out_size = entry.size;
        }
        if (dst == NULL)
        {
            return TRUE;
        }
        {
            u32 read_start = (u32)tickGetCount();

            if (nitroromReadFile(sNdsFtAnimStreamRom,
                                 sNdsFtAnimStreamFileId, entry.offset, dst,
                                 entry.size) == false)
            {
                gNdsRelocAssetFighterStreamFailures++;
                return FALSE;
            }
            gNdsRelocAssetFighterStreamReadTicks64 +=
                (u32)tickGetCount() - read_start;
        }
        gNdsRelocAssetFighterStreamReads++;
        NDS_DIAG(gNdsRelocAssetFighterStreamReadBytes += entry.size);
#if (defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP) && NDS_TICK_HUD
        /* LAB: motion reads per frame, in the SPHD column (unused without
         * the SRC split). */
        {
            extern volatile u32 gNdsTickHudSrcPhysicsDefaultTicks;

            gNdsTickHudSrcPhysicsDefaultTicks++;
        }
#endif
        return TRUE;
    }
}

/* ONE STORAGE COMMAND PER CLIP (P2-2p8, 2026-09-30).
 *
 * ndsRelocAssetLoadFighterStreamClip reads a clip's exact byte range. Clips
 * sit at 16-byte offsets, so the range starts and ends inside a sector, and
 * the storage path splits it: the ARM9 reader sends the 32-byte-multiple body
 * and then a second request for the last few bytes, and the ARM7 reads the
 * body as a partial head sector, a multi-sector run and a partial tail sector
 * -- four sector-read calls and two PXI round trips for a ~2.3 KB clip, with
 * the tail sector read twice. The ARM9 is blocked for all of it (~28-37K
 * ticks a clip on the four-CPU lab).
 *
 * These two read the WHOLE sectors a clip touches in one request instead: the
 * span starts at the clip's sector and is a multiple of 512, so both sides
 * take their direct path. The caller supplies a 32-byte aligned buffer of
 * `span` bytes and finds the clip `head` bytes into it. The bytes are the
 * pack's own; only the number of commands changes. The pack is 512-aligned in
 * the ROM image (every NitroFS file is), so a sector here is a sector of the
 * card image. */
static s32 ndsRelocAssetFighterStreamClipStart(u32 asset_id, u32 *out_start,
                                               u32 *out_size)
{
    const NDSFtAnimStreamHeader *header = &sNdsFtAnimStreamHeader;
    u32 row;
    u32 packed;
    u32 offset;
    u32 size;

    /* Past the cast's pack: the contents' clip, from its ROM row. */
    if ((gNdsRelocAssetFighterStreamDispatch != 0u) &&
        (ndsRelocAssetOpenFighterStream() != FALSE) &&
        (asset_id > header->last_id))
    {
        NDSFtAnimStreamEntry entry;

        if (ndsRelocAssetP4StreamEntry(asset_id, &entry) == FALSE)
        {
            return FALSE;
        }
        *out_start = nitroromGetFileOffset(sNdsFtAnimP4StreamRom,
                                           sNdsFtAnimP4StreamFileId) +
            entry.offset;
        *out_size = entry.size;
        return TRUE;
    }
    if ((gNdsRelocAssetFighterStreamDispatch == 0u) ||
        (ndsRelocAssetOpenFighterStream() == FALSE) ||
        (sNdsFtAnimStreamDirReady == FALSE) ||
        (gNdsRelocAssetFighterStreamDirRoute.route == 0u) ||
        (asset_id < header->first_id) || (asset_id > header->last_id))
    {
        return FALSE;
    }
    row = asset_id - header->first_id;
    if (row >= sNdsFtAnimStreamDirCount)
    {
        return FALSE;
    }
    packed = sNdsFtAnimStreamDir[row];
    offset = (packed >> NDS_FTANIM_STREAM_DIR_SIZE_BITS) << 4;
    size = packed & ((1u << NDS_FTANIM_STREAM_DIR_SIZE_BITS) - 1u);
    if ((offset < header->data_off) || (size == 0u) ||
        (offset > header->blob_bytes) ||
        (size > (header->blob_bytes - offset)))
    {
        return FALSE;
    }
    *out_start = nitroromGetFileOffset(sNdsFtAnimStreamRom,
                                       sNdsFtAnimStreamFileId) + offset;
    *out_size = size;
    return TRUE;
}

s32 ndsRelocAssetFighterStreamClipSpan(u32 asset_id, u32 *out_head,
                                       u32 *out_size, u32 *out_span,
                                       u32 *out_sector)
{
    u32 start;
    u32 size;

    if (ndsRelocAssetFighterStreamClipStart(asset_id, &start, &size) == FALSE)
    {
        return FALSE;
    }
    *out_head = start & 511u;
    *out_size = size;
    *out_span = ((start + size + 511u) & ~511u) - (start & ~511u);
    if (out_sector != NULL)
    {
        *out_sector = start & ~511u;
    }
    return TRUE;
}

s32 ndsRelocAssetReadFighterStreamClipSpan(u32 asset_id, void *dst, u32 span)
{
    u32 start;
    u32 size;
    u32 read_start;

    if ((dst == NULL) ||
        (ndsRelocAssetFighterStreamClipStart(asset_id, &start, &size) ==
         FALSE) ||
        (span != (((start + size + 511u) & ~511u) - (start & ~511u))))
    {
        return FALSE;
    }
    read_start = (u32)tickGetCount();
    if (nitroromRead(sNdsFtAnimStreamRom, start & ~511u, dst, span) == false)
    {
        gNdsRelocAssetFighterStreamFailures++;
        return FALSE;
    }
    NDS_DIAG(gNdsRelocAssetFighterStreamReadTicks64 += (u32)tickGetCount() - read_start);
    gNdsRelocAssetFighterStreamReads++;
    NDS_DIAG(gNdsRelocAssetFighterStreamReadBytes += size);
#if (defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP) && NDS_TICK_HUD
    /* LAB: motion reads per frame (SPHD), as the exact-range reader counts. */
    {
        extern volatile u32 gNdsTickHudSrcPhysicsDefaultTicks;

        gNdsTickHudSrcPhysicsDefaultTicks++;
    }
#endif
    return TRUE;
}

/* Calico's NitroRom API reads the application's FNT/FAT by file id and then
 * dispatches one exact ROM range read. It bypasses the stdio/FAT path which
 * otherwise reopens, walks and seeks a fighter clip during the frame that
 * changes action. The bytes and O2R validation below are identical to
 * ndsRelocAssetReadHeaderFromFile; any unavailable/direct-read failure falls
 * through to that established loader. */
static s32 ndsRelocAssetLoadIntoZeroedHeapDirect(
    const NDSRelocAssetEntry *entry, void *dst, u32 align,
    size_t *out_alloc_size, NDSRelocAssetHeader *out_header)
{
    NitroRom *rom;
    NDSRelocAssetHeader header;
    u8 head[NDS_O2R_RESOURCE_HEADER_SIZE + 16];
    const char *path;
    int nitro_id;
    u32 extern_count;
    u32 data_size_offset;
    u32 data_offset;
    u32 file_size;
    size_t alloc_size;

    if ((gNdsRelocAssetDirectDispatch == 0u) ||
        (entry == NULL) || (entry->path == NULL))
    {
        return FALSE;
    }
    path = ndsNitroPath(entry->path, sNdsNitroPathScratch);
    if (strncmp(path, "nitro:/", 7u) != 0)
    {
        return FALSE;
    }
    rom = nitroromGetSelf();
    if (rom == NULL)
    {
        gNdsRelocAssetDirectFailStep = 1u;
        return FALSE;
    }
    path += 7u;
    nitro_id = ndsRelocAnimDirLookup(entry->asset_id);
    if (nitro_id < 0)
    {
        nitro_id = nitroromResolvePath(rom, NITROROM_ROOT_DIR, path);
    }
    if ((nitro_id < 0) || (nitro_id >= (s32)NITROROM_ROOT_DIR))
    {
        gNdsRelocAssetDirectFailStep = 2u;
        return FALSE;
    }
    file_size = nitroromGetFileSize(rom, (u16)nitro_id);
    if ((file_size < sizeof(head)) ||
        (nitroromReadFile(rom, (u16)nitro_id, 0u, head,
                          sizeof(head)) == false) ||
        (memcmp(&head[NDS_O2R_MAGIC_OFFSET], NDS_O2R_MAGIC, 4u) != 0))
    {
        gNdsRelocAssetDirectFailStep = 3u;
        return FALSE;
    }
    header.file_id = ndsReadLe32(&head[NDS_O2R_RESOURCE_HEADER_SIZE]);
    header.reloc_intern_offset =
        ndsReadLe16(&head[NDS_O2R_RESOURCE_HEADER_SIZE + 4]);
    header.reloc_extern_offset =
        ndsReadLe16(&head[NDS_O2R_RESOURCE_HEADER_SIZE + 6]);
    extern_count = ndsReadLe32(&head[NDS_O2R_RESOURCE_HEADER_SIZE + 8]);
    header.extern_file_ids_num = extern_count;
    if ((header.file_id != entry->file_id) ||
        (extern_count > ((0xffffffffu -
                          (NDS_O2R_RESOURCE_HEADER_SIZE + 16u)) / 2u)))
    {
        gNdsRelocAssetDirectFailStep = 4u;
        return FALSE;
    }
    data_size_offset = NDS_O2R_RESOURCE_HEADER_SIZE + 12u +
                       (extern_count * 2u);
    data_offset = data_size_offset + 4u;
    if ((data_offset > file_size) ||
        (nitroromReadFile(rom, (u16)nitro_id, data_size_offset, head,
                          sizeof(u32)) == false))
    {
        gNdsRelocAssetDirectFailStep = 5u;
        return FALSE;
    }
    header.data_size = ndsReadLe32(head);
    if (header.data_size > (file_size - data_offset))
    {
        gNdsRelocAssetDirectFailStep = 6u;
        return FALSE;
    }
    alloc_size = ((size_t)header.data_size + (size_t)align - 1u) &
                 ~((size_t)align - 1u);
    memset(dst, 0, alloc_size);
    if ((header.data_size != 0u) &&
        (nitroromReadFile(rom, (u16)nitro_id, data_offset, dst,
                          header.data_size) == false))
    {
        gNdsRelocAssetDirectFailStep = 7u;
        memset(dst, 0, alloc_size);
        return FALSE;
    }
    if (out_alloc_size != NULL)
    {
        *out_alloc_size = alloc_size;
    }
    if (out_header != NULL)
    {
        *out_header = header;
    }
    return TRUE;
}

static s32 ndsRelocAssetLoadIntoZeroedHeapUnlocked(u32 asset_id, void *dst, u32 align,
                                    size_t *out_alloc_size,
                                    NDSRelocAssetHeader *out_header)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;
    size_t alloc_size;

    if (out_alloc_size != NULL)
    {
        *out_alloc_size = 0u;
    }
    if ((dst == NULL) || (align == 0u))
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    if (ndsRelocAssetLoadIntoZeroedHeapDirect(entry, dst, align,
                                               out_alloc_size,
                                               out_header) != FALSE)
    {
        gNdsRelocAssetHeaderReadCount++;
        gNdsRelocAssetPayloadReadCount++;
        gNdsRelocAssetDirectReadCount++;
        return TRUE;
    }
    gNdsRelocAssetDirectFallbackCount++;

    file = fopen(ndsNitroPath(entry->path, sNdsNitroPathScratch), "rb");
    if (file == NULL)
    {
        u32 retry;

        /* Witness: Jungle/Hyrule entry animations failed here (2026-09-07)
         * while the same asset opened on Saffron and Sector, ENOENT on a
         * file that exists. Retry to learn whether the miss is transient. */
        gNdsRelocAssetOpenFailErrno = (u32)errno;
        gNdsRelocAssetOpenFailAsset = asset_id;
        gNdsRelocAssetOpenFailCount++;
        {
            /* The path the lookup walked, so a corrupt string is one read
             * away from a poisoned directory cache. */
            const char *failed = ndsNitroPath(entry->path,
                                              sNdsNitroPathScratch);
            u32 n;

            for (n = 0u; n < 63u && failed[n] != '\0'; n++)
            {
                gNdsRelocAssetOpenFailPath[n] = failed[n];
            }
            gNdsRelocAssetOpenFailPath[n] = '\0';
        }
        for (retry = 0u; (retry < 4u) && (file == NULL); retry++)
        {
            NDS_DIAG(gNdsRelocAssetOpenRetryCount++);
            file = fopen(ndsNitroPath(entry->path, sNdsNitroPathScratch), "rb");
        }
        if (file == NULL)
        {
            return FALSE;
        }
        NDS_DIAG(gNdsRelocAssetOpenRetrySuccessCount++);
    }
    /* K0 line 2, "get_fat / f_lseek". The open walks the directory and seats
     * the cluster chain; the fseek below is the f_lseek proper. Both are marked
     * because both are FAT work the resident pack is supposed to delete. */
    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);

    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    gNdsRelocAssetHeaderReadCount++;

    alloc_size = ((size_t)header.data_size + (size_t)align - 1u) &
                 ~((size_t)align - 1u);
    memset(dst, 0, alloc_size);
    if (out_alloc_size != NULL)
    {
        *out_alloc_size = alloc_size;
    }

    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);
    if (fseek(file, data_offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        memset(dst, 0, alloc_size);
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    ndsTaskmanSampleLibcHeapNow();
    fclose(file);

    if (out_header != NULL)
    {
        *out_header = header;
    }
    gNdsRelocAssetPayloadReadCount++;
    /* K0 line 1, "fighter-animation FAT reads". */
    NDS_K0_MARK(gNdsK0AfterGoFatReads, asset_id);
    return TRUE;
}

s32 ndsRelocAssetLoadIntoZeroedHeap(u32 asset_id, void *dst, u32 align, size_t *out_alloc_size, NDSRelocAssetHeader *out_header)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetLoadIntoZeroedHeapUnlocked(asset_id, dst, align, out_alloc_size, out_header);
    ndsFsUnlock();
    return result;
}

/* Header and payload from one open.
 *
 * Callers that need both used to call ndsRelocAssetReadHeader and then
 * ndsRelocAssetLoadData, which opens the same file twice and parses the same
 * header twice -- and the second parse overwrites the first through
 * out_header, so the first was only ever a validity probe. On NitroFS an open
 * by path is a directory walk: Task 71 measured strncasecmp at 30,484
 * ticks/frame on a frame that loads a fighter animation, second only to memcpy
 * among that frame's risers, and half of it is this duplicate.
 *
 * The counters are deliberately bumped exactly as the two-call sequence bumped
 * them -- one header, one payload -- because gNdsRelocAssetHeaderReadCount is
 * read as a per-event delta, and a load that stopped counting its header
 * would look like a load that never happened. */
/* Raw NitroFS range read. No asset-table entry, no o2r header, no file id --
 * slice 1's resident figatree pack is the only such payload the animation path
 * reads, and it is streamed in chunks because the seam's budget is one BGM
 * packet (~186 ms), not the whole load.
 *
 * It counts through the SAME open/short-read/payload counters as the asset
 * loads here, deliberately: a run whose after-GO animation I/O is supposed to
 * read zero must not have a second, uncounted reader hiding in it. */
static s32 ndsRelocAssetReadRawRangeUnlocked(const char *path, u32 offset, void *dst,
                              u32 bytes)
{
    FILE *file;

    if ((path == NULL) || (dst == NULL) || (bytes == 0u))
    {
        return FALSE;
    }
    file = fopen(path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }
    if (fseek(file, (long)offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1u, (size_t)bytes, file) != (size_t)bytes)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    ndsTaskmanSampleLibcHeapNow();
    fclose(file);
    gNdsRelocAssetPayloadReadCount++;
    return TRUE;
}

s32 ndsRelocAssetReadRawRange(const char *path, u32 offset, void *dst, u32 bytes)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetReadRawRangeUnlocked(path, offset, dst, bytes);
    ndsFsUnlock();
    return result;
}

/* The chunked form of the same read, one open for the whole payload. Contract
 * and the measurement that motivated it: include/nds/nds_reloc_assets.h. */
static s32 ndsRelocAssetStreamOpenUnlocked(NdsRelocAssetStream *stream, const char *path)
{
    char full[NDS_NITRO_PATH_MAX];

    if (stream == NULL)
    {
        return FALSE;
    }
    path = ndsNitroPath(path, full);
    stream->file = NULL;
    stream->rom = NULL;
    stream->nitro_id = 0u;
    stream->size = 0u;
    stream->pos = 0u;
    if (path == NULL)
    {
        return FALSE;
    }
    if (strncmp(path, "nitro:/", 7u) == 0)
    {
        /* By file id (nds_reloc_assets.h): the same resolve libfat's open
         * makes, then every read is one ROM range read. */
        NitroRom *rom = nitroromGetSelf();
        int id = (rom != NULL) ?
            nitroromResolvePath(rom, NITROROM_ROOT_DIR, path + 7u) : -1;

        if ((id >= 0) && (id < (int)NITROROM_ROOT_DIR) &&
            ((u32)id < rom->num_files))
        {
            stream->rom = rom;
            stream->nitro_id = (u32)id;
            stream->size = nitroromGetFileSize(rom, (u16)id);
            return TRUE;
        }
    }
    stream->file = (void *)fopen(path, "rb");
    if (stream->file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }
    return TRUE;
}

s32 ndsRelocAssetStreamOpen(NdsRelocAssetStream *stream, const char *path)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetStreamOpenUnlocked(stream, path);
    ndsFsUnlock();
    return result;
}

static s32 ndsRelocAssetStreamReadUnlocked(NdsRelocAssetStream *stream, u32 offset, void *dst,
                            u32 bytes)
{
    FILE *file;

    if ((stream == NULL) || (dst == NULL) || (bytes == 0u))
    {
        return FALSE;
    }
    if (stream->rom != NULL)
    {
        if ((offset > stream->size) || (bytes > (stream->size - offset)) ||
            (nitroromReadFile((NitroRom *)stream->rom, (u16)stream->nitro_id,
                              offset, dst, bytes) == false))
        {
            gNdsRelocAssetShortReadCount++;
            return FALSE;
        }
        stream->pos = offset + bytes;
        gNdsRelocAssetPayloadReadCount++;
        return TRUE;
    }
    if (stream->file == NULL)
    {
        return FALSE;
    }
    file = (FILE *)stream->file;
    if (fseek(file, (long)offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    if (fread(dst, 1u, (size_t)bytes, file) != (size_t)bytes)
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    stream->pos = offset + bytes;
    gNdsRelocAssetPayloadReadCount++;
    return TRUE;
}

static s32 ndsRelocAssetStreamReadNextUnlocked(NdsRelocAssetStream *stream,
                                               void *dst, u32 bytes)
{
    if ((stream == NULL) || (dst == NULL) || (bytes == 0u))
    {
        return FALSE;
    }
    if (stream->rom != NULL)
    {
        if ((stream->pos > stream->size) ||
            (bytes > (stream->size - stream->pos)) ||
            (nitroromReadFile((NitroRom *)stream->rom, (u16)stream->nitro_id,
                              stream->pos, dst, bytes) == false))
        {
            gNdsRelocAssetShortReadCount++;
            return FALSE;
        }
        stream->pos += bytes;
        return TRUE;
    }
    if ((stream->file == NULL) ||
        (fread(dst, 1u, (size_t)bytes, (FILE *)stream->file) !=
         (size_t)bytes))
    {
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }
    stream->pos += bytes;
    return TRUE;
}

s32 ndsRelocAssetStreamReadNext(NdsRelocAssetStream *stream, void *dst,
                                u32 bytes)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetStreamReadNextUnlocked(stream, dst, bytes);
    ndsFsUnlock();
    return result;
}

u32 ndsRelocAssetStreamSize(const NdsRelocAssetStream *stream)
{
    return ((stream != NULL) && (stream->rom != NULL)) ? stream->size : 0u;
}

s32 ndsRelocAssetStreamRead(NdsRelocAssetStream *stream, u32 offset, void *dst, u32 bytes)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetStreamReadUnlocked(stream, offset, dst, bytes);
    ndsFsUnlock();
    return result;
}

static void ndsRelocAssetStreamCloseUnlocked(NdsRelocAssetStream *stream)
{
    if (stream == NULL)
    {
        return;
    }
    stream->rom = NULL;
    if (stream->file == NULL)
    {
        return;
    }
    ndsTaskmanSampleLibcHeapNow();
    fclose((FILE *)stream->file);
    stream->file = NULL;
}

void ndsRelocAssetStreamClose(NdsRelocAssetStream *stream)
{
    ndsFsLock();
    ndsRelocAssetStreamCloseUnlocked(stream);
    ndsFsUnlock();
}

static s32 ndsRelocAssetLoadHeaderAndDataUnlocked(u32 asset_id, void *dst,
                                   size_t dst_capacity,
                                   NDSRelocAssetHeader *out_header)
{
    const NDSRelocAssetEntry *entry;
    FILE *file;
    NDSRelocAssetHeader header;
    long data_offset;

    if (dst == NULL)
    {
        return FALSE;
    }

    entry = ndsRelocAssetFindEntry(asset_id);
    if (entry == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    file = fopen(ndsNitroPath(entry->path, sNdsNitroPathScratch), "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    gNdsRelocAssetHeaderReadCount++;

    if ((size_t)header.data_size > dst_capacity)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    /* K0 lines 1 and 2, marked in every payload loader rather than only in the
     * one the acquisition path happens to use today -- a "zero" must not be
     * able to hide behind an uninstrumented door. */
    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);
    if (fseek(file, data_offset, SEEK_SET) != 0)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        ndsTaskmanSampleLibcHeapNow();
        fclose(file);
        return FALSE;
    }
    ndsTaskmanSampleLibcHeapNow();
    fclose(file);

    if (out_header != NULL)
    {
        *out_header = header;
    }
    gNdsRelocAssetPayloadReadCount++;
    NDS_K0_MARK(gNdsK0AfterGoFatReads, asset_id);
    return TRUE;
}

s32 ndsRelocAssetLoadHeaderAndData(u32 asset_id, void *dst, size_t dst_capacity, NDSRelocAssetHeader *out_header)
{
    s32 result;

    ndsFsLock();
    result = ndsRelocAssetLoadHeaderAndDataUnlocked(asset_id, dst, dst_capacity, out_header);
    ndsFsUnlock();
    return result;
}
