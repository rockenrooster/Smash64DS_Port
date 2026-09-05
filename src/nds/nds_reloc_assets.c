#include <filesystem.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>

#include <nds/generated/nds_fighter_production.generated.h>
#include <nds/nds_reloc_assets.h>

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
volatile u32 gNdsRelocAssetFormatFailCount;
volatile u32 gNdsRelocAssetShortReadCount;
volatile u32 gNdsRelocAssetDirectReadCount;
volatile u32 gNdsRelocAssetDirectFallbackCount;
/* Same-binary performance control. Zero restores the established stdio/FAT
 * loader without changing code placement. */
__attribute__((used)) volatile u32 gNdsRelocAssetDirectDispatch = 1u;
__attribute__((used)) volatile u32 gNdsRelocAssetFighterStreamDispatch = 1u;
volatile u32 gNdsRelocAssetFighterStreamReads;
volatile u32 gNdsRelocAssetFighterStreamMisses;
volatile u32 gNdsRelocAssetFighterStreamFailures;

static const NDSRelocAssetEntry sNdsRelocAssets[] = {
    { 0, 0, "nitro:/reloc/reloc_menus/MNCommon" },
    { 194, 194, "nitro:/reloc/reloc_misc_named/N64Logo" },
    { 52, 52, "nitro:/reloc/reloc_movies/MVCommon" },
    { 63, 63, "nitro:/reloc/reloc_transitions/MVOpeningRoomTransition" },
    { 56, 56, "nitro:/reloc/reloc_movies/MVOpeningRoomScene1" },
    { 57, 57, "nitro:/reloc/reloc_movies/MVOpeningRoomScene2" },
    { 58, 58, "nitro:/reloc/reloc_movies/MVOpeningRoomScene3" },
    { 59, 59, "nitro:/reloc/reloc_movies/MVOpeningRoomScene4" },
    { 75, 75, "nitro:/reloc/reloc_movies/MVOpeningRunCrash" },
    { 90, 90, "nitro:/reloc/reloc_movies/MVOpeningRoomWallpaper" },
    { 53, 53, "nitro:/reloc/reloc_movies/MVOpeningPortraitsSet1" },
    { 54, 54, "nitro:/reloc/reloc_movies/MVOpeningPortraitsSet2" },
    { 37, 37, "nitro:/reloc/reloc_interface/IFCommonAnnounceCommon" },
    { 0x57, 0x57, "nitro:/reloc/reloc_interface/IFCommonItem" },
    { 65, 65, "nitro:/reloc/reloc_movies/MVOpeningCommon" },
    { 55, 55, "nitro:/reloc/reloc_movies/MVOpeningRun" },
    { 71, 71, "nitro:/reloc/reloc_movies/MVOpeningYamabuki" },
    { 73, 73, "nitro:/reloc/reloc_movies/MVOpeningSector" },
    { 167, 167, "nitro:/reloc/reloc_menus/MNTitle" },
    { 168, 168, "nitro:/reloc/reloc_menus/MNTitleFireAnim" },
    { 0xa6, 0xa6, "nitro:/reloc/reloc_interface/IFCommonPlayer" },
    { 0x52, 0x52, "nitro:/reloc/reloc_interface/IFCommonGameStatus" },
    { 0xa4, 0xa4, "nitro:/reloc/reloc_interface/IFCommonPlayerDamage" },
    { 0xa5, 0xa5, "nitro:/reloc/reloc_interface/IFCommonTimer" },
    { 0x24, 0x24, "nitro:/reloc/reloc_interface/IFCommonDigits" },
    { 0xc5, 0xc5, "nitro:/reloc/reloc_interface/IFCommonBattlePause" },
    { 0x26, 0x26, "nitro:/reloc/reloc_interface/IFCommonPlayerTags" },
    { 0x19, 0x19, "nitro:/reloc/reloc_fighters_common/FTStocksZako" },
    { 0x22, 0x22, "nitro:/reloc/reloc_menus/MNVSResults" },
    { 0xe, 0xe, "nitro:/reloc/reloc_bonus/BonusPicturePlatform" },
    { 0xd, 0xd, "nitro:/reloc/reloc_bonus/BonusPicture" },
    { 0xc, 0xc, "nitro:/reloc/reloc_misc_named/CharacterNames" },
    { 0x18, 0x18, "nitro:/reloc/reloc_menus/MNPlayersDifficulty" },
    { 0xfc, 0xfc, "nitro:/reloc/reloc_scene/SCExplainMain" },
    { 0xc6, 0xc6, "nitro:/reloc/reloc_scene/SCExplainGraphics" },
    { 0x1b, 0x1b, "nitro:/reloc/reloc_stages/GRWallpaperTrainingYellow" },
    { 0x1c, 0x1c, "nitro:/reloc/reloc_stages/GRWallpaperTrainingBlue" },
    { 0xfe, 0xfe, "nitro:/reloc/reloc_scene/SC1PTrainingMode" },
    { 0xc4, 0xc4, "nitro:/reloc/reloc_menus/MNSoundTest" },
    { 0x4e, 0x4e, "nitro:/reloc/reloc_menus/MNBackupClearHeaderOption" },
    { 0x4d, 0x4d, "nitro:/reloc/reloc_menus/MNBackupClear" },
    { 0x4, 0x4, "nitro:/reloc/reloc_menus/MNOption" },
    { 0x10, 0x10, "nitro:/reloc/reloc_menus/MNCharacters" },
    { 0x1f, 0x1f, "nitro:/reloc/reloc_menus/MNVSRecordMain" },
    { 0x20, 0x20, "nitro:/reloc/reloc_menus/MNDataCommon" },
    { 0x5, 0x5, "nitro:/reloc/reloc_menus/MNData" },
    { 0xad, 0xad, "nitro:/reloc/reloc_menus/MNCongraYoshiTop" },
    { 0xac, 0xac, "nitro:/reloc/reloc_menus/MNCongraYoshiBottom" },
    { 0xb1, 0xb1, "nitro:/reloc/reloc_menus/MNCongraSamusTop" },
    { 0xb0, 0xb0, "nitro:/reloc/reloc_menus/MNCongraSamusBottom" },
    { 0xb5, 0xb5, "nitro:/reloc/reloc_menus/MNCongraPurinTop" },
    { 0xb4, 0xb4, "nitro:/reloc/reloc_menus/MNCongraPurinBottom" },
    { 0xaf, 0xaf, "nitro:/reloc/reloc_menus/MNCongraPikachuTop" },
    { 0xae, 0xae, "nitro:/reloc/reloc_menus/MNCongraPikachuBottom" },
    { 0xc1, 0xc1, "nitro:/reloc/reloc_menus/MNCongraNessTop" },
    { 0xc0, 0xc0, "nitro:/reloc/reloc_menus/MNCongraNessBottom" },
    { 0xbb, 0xbb, "nitro:/reloc/reloc_menus/MNCongraMarioTop" },
    { 0xba, 0xba, "nitro:/reloc/reloc_menus/MNCongraMarioBottom" },
    { 0xbd, 0xbd, "nitro:/reloc/reloc_menus/MNCongraLuigiTop" },
    { 0xbc, 0xbc, "nitro:/reloc/reloc_menus/MNCongraLuigiBottom" },
    { 0xb3, 0xb3, "nitro:/reloc/reloc_menus/MNCongraLinkTop" },
    { 0xb2, 0xb2, "nitro:/reloc/reloc_menus/MNCongraLinkBottom" },
    { 0xab, 0xab, "nitro:/reloc/reloc_menus/MNCongraKirbyTop" },
    { 0xaa, 0xaa, "nitro:/reloc/reloc_menus/MNCongraKirbyBottom" },
    { 0xbf, 0xbf, "nitro:/reloc/reloc_menus/MNCongraFoxTop" },
    { 0xbe, 0xbe, "nitro:/reloc/reloc_menus/MNCongraFoxBottom" },
    { 0xb9, 0xb9, "nitro:/reloc/reloc_menus/MNCongraDonkeyTop" },
    { 0xb8, 0xb8, "nitro:/reloc/reloc_menus/MNCongraDonkeyBottom" },
    { 0xb7, 0xb7, "nitro:/reloc/reloc_menus/MNCongraCaptainTop" },
    { 0xb6, 0xb6, "nitro:/reloc/reloc_menus/MNCongraCaptainBottom" },
    { 0x4c, 0x4c, "nitro:/reloc/reloc_movies/MVEnding" },
    { 0xc3, 0xc3, "nitro:/reloc/reloc_scene/SCStaffroll" },
    { 0x9, 0x9, "nitro:/reloc/reloc_menus/MNMessage" },
    { 0x17, 0x17, "nitro:/reloc/reloc_menus/MNPlayers1PMode" },
    { 0x4f, 0x4f, "nitro:/reloc/reloc_menus/MN1PContinue" },
    { 0x2, 0x2, "nitro:/reloc/reloc_menus/MN1P" },
    { 0xb, 0xb, "nitro:/reloc/reloc_scene/SC1PIntro" },
    { 0xa, 0xa, "nitro:/reloc/reloc_scene/SC1PChallenger" },
    { 0x97, 0x97, "nitro:/reloc/reloc_scene/SC1PStageClear3" },
    { 0x51, 0x51, "nitro:/reloc/reloc_scene/SC1PStageClear2" },
    { 0x50, 0x50, "nitro:/reloc/reloc_scene/SC1PStageClear1" },
    { 0x23, 0x23, "nitro:/reloc/reloc_fighters_common/FTEmblemModels" },
    { 0x28, 0x28, "nitro:/reloc/reloc_transitions/LBTransitionAeroplane" },
    { 0x29, 0x29, "nitro:/reloc/reloc_transitions/LBTransitionCheck" },
    { 0x2a, 0x2a, "nitro:/reloc/reloc_transitions/LBTransitionGakubuthi" },
    { 0x2b, 0x2b, "nitro:/reloc/reloc_transitions/LBTransitionKannon" },
    { 0x2c, 0x2c, "nitro:/reloc/reloc_transitions/LBTransitionStar" },
    { 0x2d, 0x2d, "nitro:/reloc/reloc_transitions/LBTransitionSudare1" },
    { 0x2e, 0x2e, "nitro:/reloc/reloc_transitions/LBTransitionSudare2" },
    { 0x30, 0x30, "nitro:/reloc/reloc_transitions/LBTransitionBlock" },
    { 0x31, 0x31, "nitro:/reloc/reloc_transitions/LBTransitionRotScale" },
    { 0x32, 0x32, "nitro:/reloc/reloc_transitions/LBTransitionCurtain" },
    { 0x33, 0x33, "nitro:/reloc/reloc_transitions/LBTransitionCamera" },
    { 0xc7, 0xc7, "nitro:/reloc/reloc_misc_named/SYKseg1Validate" },
    { 6, 6, "nitro:/reloc/reloc_menus/MNVSMode" },
    { 0x11, 0x11, "nitro:/reloc/reloc_menus/MNPlayersCommon" },
    { 0x12, 0x12, "nitro:/reloc/reloc_menus/MNPlayersGameModes" },
    { 0x13, 0x13, "nitro:/reloc/reloc_menus/MNPlayersPortraits" },
    { 0x14, 0x14, "nitro:/reloc/reloc_fighters_common/FTEmblemSprites" },
    { 0x15, 0x15, "nitro:/reloc/reloc_menus/MNSelectCommon" },
    { 0x16, 0x16, "nitro:/reloc/reloc_menus/MNPlayersSpotlight" },
    { 0x1a, 0x1a, "nitro:/reloc/reloc_stages/GRWallpaperTrainingBlack" },
    { 0x1e, 0x1e, "nitro:/reloc/reloc_menus/MNMaps" },
    { 0x21, 0x21, "nitro:/reloc/reloc_menus/MNCommonFonts" },
    { 0xff, 0xff, "nitro:/reloc/reloc_stages/GRPupupuMap" },
    { 0x104, 0x104, "nitro:/reloc/reloc_stages/GRInishieMap" },
    { 0x109, 0x109, "nitro:/reloc/reloc_stages/GRHyruleMap" },
    { 0x1005f, 0x5f, "nitro:/reloc/reloc_stages/StageCastle" },
    { 0x10058, 0x58, "nitro:/reloc/reloc_stages/StageDreamLand" },
    { 0x71, 0x71, "nitro:/reloc/reloc_extern_data/ExternDataBank113" },
    { 0x67, 0x67, "nitro:/reloc/reloc_extern_data/ExternDataBank103" },
    { 0x68, 0x68, "nitro:/reloc/reloc_extern_data/ExternDataBank104" },
    { 0x98, 0x98, "nitro:/reloc/reloc_extern_data/MiscDataBank152" },
    /* P2-4s1 Yoster closure (Makefile:4186-4190; IDs reloc_data.h:527,535). */
#if NDS_P2_STAGE_YOSTER
    { 0x107, 0x107, "nitro:/reloc/reloc_stages/GRYosterMap" },
    { 0x1005d, 0x5d, "nitro:/reloc/reloc_stages/StageYoshi" },
    { 0x6e, 0x6e, "nitro:/reloc/reloc_extern_data/ExternDataBank110" },
    { 0x6f, 0x6f, "nitro:/reloc/reloc_extern_data/ExternDataBank111" },
    { 0x9a, 0x9a, "nitro:/reloc/reloc_extern_data/MiscDataBank154" },
#endif
    /* P2-4s2 Castle. The wallpaper its map header references,
     * MVOpeningRoomWallpaper, is asset 90 and already rowed unconditionally
     * above, so only the map and its two banks are new. */
#if NDS_P2_STAGE_CASTLE
    { 0x103, 0x103, "nitro:/reloc/reloc_stages/GRCastleMap" },
    { 0x6a, 0x6a, "nitro:/reloc/reloc_extern_data/ExternDataBank106" },
    { 0x9c, 0x9c, "nitro:/reloc/reloc_extern_data/MiscDataBank156" },
#endif
    /* P2-4s3 Congo Jungle. Its map declares exactly these three, checked
     * against the O2R header rather than assumed. */
#if NDS_P2_STAGE_JUNGLE
    { 0x105, 0x105, "nitro:/reloc/reloc_stages/GRJungleMap" },
    { 0x1005c, 0x5c, "nitro:/reloc/reloc_stages/StageJungle" },
    { 0x6c, 0x6c, "nitro:/reloc/reloc_extern_data/ExternDataBank108" },
    { 0x9e, 0x9e, "nitro:/reloc/reloc_extern_data/MiscDataBank158" },
#endif
    /* P2-4s4 Planet Zebes, closure checked against the map header. */
#if NDS_P2_STAGE_ZEBES
    { 0x101, 0x101, "nitro:/reloc/reloc_stages/GRZebesMap" },
    { 0x10059, 0x59, "nitro:/reloc/reloc_stages/StageZebes" },
    { 0x69, 0x69, "nitro:/reloc/reloc_extern_data/ExternDataBank105" },
    { 0x9d, 0x9d, "nitro:/reloc/reloc_extern_data/MiscDataBank157" },
#endif
    /* P2-4s6 Saffron City: the only stage map so far with two Misc banks. */
    /* P2-4s7 Mushroom Kingdom and P2-4s8 Sector Z. */
#if NDS_P2_STAGE_INISHIE
    { 0x1005b, 0x5b, "nitro:/reloc/reloc_stages/StageHyruleWallpaper" },
    { 0x6b, 0x6b, "nitro:/reloc/reloc_extern_data/ExternDataBank107" },
    { 0x9b, 0x9b, "nitro:/reloc/reloc_extern_data/MiscDataBank155" },
#endif
#if NDS_P2_STAGE_SECTOR
    { 0x106, 0x106, "nitro:/reloc/reloc_stages/GRSectorMap" },
    { 0x10063, 0x63, "nitro:/reloc/reloc_stages/StageSector" },
    { 0x6d, 0x6d, "nitro:/reloc/reloc_extern_data/ExternDataBank109" },
    { 0x99, 0x99, "nitro:/reloc/reloc_extern_data/MiscDataBank153" },
#endif
    /* P2-6 step 7 (boss). Final Destination's runtime map (Last) and its
     * geometry bank; the 1P arenas ride NDS_P2_1P_GAME. File 96
     * (StageLastBackground, the wallpaper sprite) is not staged: the native
     * packet owns the wallpaper and the header field is unused on target. */
#if NDS_P2_1P_GAME
    { 0x10a, 0x10a, "nitro:/reloc/reloc_stages/GRLastMap" },
    { 0x72, 0x72, "nitro:/reloc/reloc_extern_data/ExternDataBank114" },
#endif
#if NDS_P2_STAGE_YAMABUKI
    { 0x108, 0x108, "nitro:/reloc/reloc_stages/GRYamabukiMap" },
    { 0x1005e, 0x5e, "nitro:/reloc/reloc_stages/StagePokemon" },
    { 0x70, 0x70, "nitro:/reloc/reloc_extern_data/ExternDataBank112" },
    { 0x9f, 0x9f, "nitro:/reloc/reloc_extern_data/MiscDataBank159" },
    { 0xa0, 0xa0, "nitro:/reloc/reloc_extern_data/MiscDataBank160" },
#endif
    { 0xa3, 0xa3, "nitro:/reloc/reloc_fighters_common/FTManagerCommon" },
    { 0xcb, 0xcb, "nitro:/reloc/reloc_fighters_main/MarioMain" },
    { 0xca, 0xca, "nitro:/reloc/reloc_fighters_main/MarioMainMotion" },
    { 0x128, 0x128, "nitro:/reloc/reloc_fighters_main/MarioModel" },
    { 0x12a, 0x12a, "nitro:/reloc/reloc_fighters_main/MarioShieldPose" },
    { 0xcc, 0xcc, "nitro:/reloc/reloc_fighters_main/MarioSpecial1" },
    { 0x164, 0x164, "nitro:/reloc/reloc_fighters_main/MarioSpecial2" },
    { 0x129, 0x129, "nitro:/reloc/reloc_fighters_main/MarioSpecial3" },
    { 0xd1, 0xd1, "nitro:/reloc/reloc_fighters_main/FoxMain" },
    { 0xd0, 0xd0, "nitro:/reloc/reloc_fighters_main/FoxMainMotion" },
    { 0x139, 0x139, "nitro:/reloc/reloc_fighters_main/FoxModel" },
    { 0x13a, 0x13a, "nitro:/reloc/reloc_fighters_main/FoxShieldPose" },
    { 0xd2, 0xd2, "nitro:/reloc/reloc_fighters_main/FoxSpecial1" },
    { 0x15a, 0x15a, "nitro:/reloc/reloc_fighters_main/FoxSpecial2" },
    { 0xa1, 0xa1, "nitro:/reloc/reloc_fighters_main/FoxSpecial3" },
    { 0x13c, 0x13c, "nitro:/reloc/reloc_fighters_main/FoxSpecial4" },
    { 0x53, 0x53, "nitro:/reloc/reloc_effects/EFCommonEffects1" },
    { 0x54, 0x54, "nitro:/reloc/reloc_effects/EFCommonEffects2" },
    { 0x55, 0x55, "nitro:/reloc/reloc_effects/EFCommonEffects3" },
    { 0xc9, 0xc9, "nitro:/reloc/reloc_extern_data/MiscData201" },
    { 0x12b, 0x12b, "nitro:/reloc/reloc_extern_data/MiscData299" },
    { 0x13b, 0x13b, "nitro:/reloc/reloc_extern_data/MiscData315" },
    { 0x6d, 0x6d, "nitro:/reloc/reloc_extern_data/ExternDataBank109" },
    /* P2-3f48. The item subsystem's shared data and its only dependency.
     * MiscData086 also appears in Yoshi's generated dependency rows below;
     * ndsRelocAssetFindEntry returns the first match and both rows carry the
     * same path, so the duplicate is inert. It is listed here because the item
     * file needs it whether or not Yoshi is in the build. */
    { 0xfb, 0xfb, "nitro:/reloc/reloc_items/ITCommonData" },
    { 0x56, 0x56, "nitro:/reloc/reloc_extern_data/MiscData086" },
#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY
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
#if NDS_P2_SAMUS
    NDS_P2_SAMUS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_SAMUS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_LINK
    NDS_P2_LINK_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_LINK_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_PIKACHU
    NDS_P2_PIKACHU_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_PIKACHU_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_YOSHI
    NDS_P2_YOSHI_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_YOSHI_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_NESS
    NDS_P2_NESS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_NESS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_PURIN
    NDS_P2_PURIN_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_PURIN_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#if NDS_P2_KIRBY
    NDS_P2_KIRBY_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_ENTRY)
    NDS_P2_KIRBY_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_ENTRY)
#endif
#undef NDS_P2_FIGHTER_DEPENDENCY_ENTRY
#undef NDS_P2_FIGHTER_ASSET_ENTRY
#endif
};

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

static const NDSRelocAssetEntry *ndsRelocAssetMarioAnimEntry(u32 asset_id)
{
    static const char prefix[] = "nitro:/reloc/reloc_animations/";
    static NDSRelocAssetEntry entry;
    static char path[NDS_RELOC_MARIO_ANIM_PATH_CAPACITY];
    u32 index;
    int written;

    if ((asset_id < NDS_RELOC_MARIO_ANIM_FIRST) ||
        (asset_id > NDS_RELOC_MARIO_ANIM_LAST))
    {
        return NULL;
    }

    index = asset_id - NDS_RELOC_MARIO_ANIM_FIRST;
    if (index == 0u)
    {
        written = sniprintf(path, sizeof(path), "%sFTMarioAnimWait", prefix);
    }
    else if (index == 44u)
    {
        written = sniprintf(path, sizeof(path),
                           "%sFTMarioAnimDownBounceD", prefix);
    }
    else if (index == 46u)
    {
        written = sniprintf(path, sizeof(path),
                           "%sFTMarioAnimDownStandD", prefix);
    }
    else
    {
        written = sniprintf(path, sizeof(path), "%sFTMarioAnim%03lu", prefix,
                           (unsigned long)index);
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

    written = sniprintf(path, sizeof(path),
                       "nitro:/reloc/reloc_animations/FTFoxAnim%03lu",
                       (unsigned long)(asset_id - NDS_RELOC_FOX_ANIM_FIRST));
    if ((written < 0) || ((size_t)written >= sizeof(path)))
    {
        return NULL;
    }

    entry.asset_id = asset_id;
    entry.file_id = asset_id;
    entry.path = path;
    return &entry;
}

#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY
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
#if NDS_P2_SAMUS
    NDS_P2_SAMUS_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_LINK
    NDS_P2_LINK_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_PIKACHU
    NDS_P2_PIKACHU_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_YOSHI
    NDS_P2_YOSHI_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_NESS
    NDS_P2_NESS_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_PURIN
    NDS_P2_PURIN_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
#if NDS_P2_KIRBY
    NDS_P2_KIRBY_ANIM_SEGMENTS(NDS_P2_ANIM_SEGMENT_ROW)
#endif
};

static const NDSRelocAssetEntry *ndsRelocAssetP2FighterAnimEntry(u32 asset_id)
{
    static NDSRelocAssetEntry entry;
    static char path[NDS_RELOC_MARIO_ANIM_PATH_CAPACITY];
    const NDSP2FighterAnimSegment *segment = NULL;
    size_t i;
    int written;

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

    written = sniprintf(path, sizeof(path),
                        "nitro:/reloc/reloc_animations/%s%03lu",
                        segment->stem,
                        (unsigned long)(asset_id - segment->zero_id));
    if ((written < 0) || ((size_t)written >= sizeof(path)))
    {
        return NULL;
    }
    entry.asset_id = asset_id;
    entry.file_id = asset_id;
    entry.path = path;
    return &entry;
}
#endif

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
#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY
    {
        const NDSRelocAssetEntry *p2_anim =
            ndsRelocAssetP2FighterAnimEntry(asset_id);

        if (p2_anim != NULL)
        {
            return p2_anim;
        }
    }
#endif

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

void ndsRelocAssetsInit(void)
{
    if (nitroFSInit(NULL))
    {
        gNdsRelocAssetInitResult = NDS_RELOC_ASSET_INIT_PASS;
#if NDS_BOOT_DIAG_TEXT
        /* P2-1L (11). gNdsRelocAssetInitResult is what every verifier reads;
         * the console line is the human copy, and the owner's free-play ROM
         * builds with NDS_BOOT_DIAG_TEXT := 0 so its bottom screen is clean. */
        iprintf("NitroFS: PASS\n");
#endif
    }
    else
    {
        gNdsRelocAssetInitResult = 0;
#if NDS_BOOT_DIAG_TEXT
        iprintf("NitroFS: FAIL\n");
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

s32 ndsRelocAssetReadHeader(u32 asset_id, NDSRelocAssetHeader *out_header)
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

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, entry->file_id, out_header, &data_offset);
    fclose(file);

    if (ok != FALSE)
    {
        gNdsRelocAssetHeaderReadCount++;
    }
    return ok;
}

s32 ndsRelocAssetReadExternFileIDs(u32 asset_id, u16 *out_file_ids,
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

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    if (header.extern_file_ids_num > capacity)
    {
        fclose(file);
        gNdsRelocAssetFormatFailCount++;
        return FALSE;
    }
    if (fseek(file, NDS_O2R_RESOURCE_HEADER_SIZE + 12, SEEK_SET) != 0)
    {
        fclose(file);
        gNdsRelocAssetShortReadCount++;
        return FALSE;
    }

    for (i = 0; i < header.extern_file_ids_num; i++)
    {
        u8 id_bytes[2];

        if (fread(id_bytes, 1, sizeof(id_bytes), file) != sizeof(id_bytes))
        {
            fclose(file);
            gNdsRelocAssetShortReadCount++;
            return FALSE;
        }
        out_file_ids[i] = ndsReadLe16(id_bytes);
    }
    fclose(file);

    if (out_count != NULL)
    {
        *out_count = header.extern_file_ids_num;
    }
    return TRUE;
}

s32 ndsRelocAssetLoadData(u32 asset_id, void *dst, size_t dst_capacity,
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

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    ok = ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header, &data_offset);
    if (ok == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    if ((size_t)header.data_size > dst_capacity)
    {
        gNdsRelocAssetShortReadCount++;
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
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        fclose(file);
        return FALSE;
    }
    fclose(file);

    if (out_header != NULL)
    {
        *out_header = header;
    }
    gNdsRelocAssetPayloadReadCount++;
    NDS_K0_MARK(gNdsK0AfterGoFatReads, asset_id);
    return TRUE;
}

s32 ndsRelocAssetLoadDataAndExternIDs(u32 asset_id, void *dst,
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
    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }
    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);
    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
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
        fclose(file);
        return FALSE;
    }
    if (header.extern_file_ids_num != 0u)
    {
        if (fseek(file, NDS_O2R_RESOURCE_HEADER_SIZE + 12, SEEK_SET) != 0)
        {
            gNdsRelocAssetShortReadCount++;
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
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1u, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        fclose(file);
        return FALSE;
    }
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
#ifndef NDS_R2_FTANIM_STREAM
#define NDS_R2_FTANIM_STREAM 0
#endif

#if NDS_R2_FTANIM_STREAM
#define NDS_FTANIM_STREAM_MAGIC 0x31535042u /* "BPS1", little endian */
#define NDS_FTANIM_STREAM_VERSION 1u
#define NDS_FTANIM_STREAM_PATH "zz_stream/ftanim_stream_pack.bin"

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

static NitroRom *sNdsFtAnimStreamRom;
static NDSFtAnimStreamHeader sNdsFtAnimStreamHeader;
static u16 sNdsFtAnimStreamFileId;
static u8 sNdsFtAnimStreamState;

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
    sNdsFtAnimStreamState = 1u;
    return TRUE;

fail:
    sNdsFtAnimStreamState = 2u;
    gNdsRelocAssetFighterStreamFailures++;
    return FALSE;
}
#endif

s32 ndsRelocAssetLoadFighterStreamClip(u32 asset_id, void *dst,
                                       u32 *out_size)
{
    if (out_size != NULL)
    {
        *out_size = 0u;
    }
#if NDS_R2_FTANIM_STREAM
    {
        NDSFtAnimStreamEntry entry;
        const NDSFtAnimStreamHeader *header = &sNdsFtAnimStreamHeader;
        u32 row;

        if ((gNdsRelocAssetFighterStreamDispatch == 0u) ||
            (ndsRelocAssetOpenFighterStream() == FALSE) ||
            (asset_id < header->first_id) || (asset_id > header->last_id))
        {
            gNdsRelocAssetFighterStreamMisses++;
            return FALSE;
        }
        row = header->dir_off +
              ((asset_id - header->first_id) *
               sizeof(NDSFtAnimStreamEntry));
        if (nitroromReadFile(sNdsFtAnimStreamRom,
                             sNdsFtAnimStreamFileId, row, &entry,
                             sizeof(entry)) == false)
        {
            gNdsRelocAssetFighterStreamFailures++;
            return FALSE;
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
        if (nitroromReadFile(sNdsFtAnimStreamRom,
                             sNdsFtAnimStreamFileId, entry.offset, dst,
                             entry.size) == false)
        {
            gNdsRelocAssetFighterStreamFailures++;
            return FALSE;
        }
        gNdsRelocAssetFighterStreamReads++;
        return TRUE;
    }
#else
    (void)asset_id;
    (void)dst;
    gNdsRelocAssetFighterStreamMisses++;
    return FALSE;
#endif
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
        (entry == NULL) || (entry->path == NULL) ||
        (strncmp(entry->path, "nitro:/", 7u) != 0))
    {
        return FALSE;
    }
    rom = nitroromGetSelf();
    if (rom == NULL)
    {
        return FALSE;
    }
    path = entry->path + 7u;
    nitro_id = nitroromResolvePath(rom, NITROROM_ROOT_DIR, path);
    if ((nitro_id < 0) || (nitro_id >= (s32)NITROROM_ROOT_DIR))
    {
        return FALSE;
    }
    file_size = nitroromGetFileSize(rom, (u16)nitro_id);
    if ((file_size < sizeof(head)) ||
        (nitroromReadFile(rom, (u16)nitro_id, 0u, head,
                          sizeof(head)) == false) ||
        (memcmp(&head[NDS_O2R_MAGIC_OFFSET], NDS_O2R_MAGIC, 4u) != 0))
    {
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
        return FALSE;
    }
    data_size_offset = NDS_O2R_RESOURCE_HEADER_SIZE + 12u +
                       (extern_count * 2u);
    data_offset = data_size_offset + 4u;
    if ((data_offset > file_size) ||
        (nitroromReadFile(rom, (u16)nitro_id, data_size_offset, head,
                          sizeof(u32)) == false))
    {
        return FALSE;
    }
    header.data_size = ndsReadLe32(head);
    if (header.data_size > (file_size - data_offset))
    {
        return FALSE;
    }
    alloc_size = ((size_t)header.data_size + (size_t)align - 1u) &
                 ~((size_t)align - 1u);
    memset(dst, 0, alloc_size);
    if ((header.data_size != 0u) &&
        (nitroromReadFile(rom, (u16)nitro_id, data_offset, dst,
                          header.data_size) == false))
    {
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

s32 ndsRelocAssetLoadIntoZeroedHeap(u32 asset_id, void *dst, u32 align,
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

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }
    /* K0 line 2, "get_fat / f_lseek". The open walks the directory and seats
     * the cluster chain; the fseek below is the f_lseek proper. Both are marked
     * because both are FAT work the resident pack is supposed to delete. */
    NDS_K0_MARK(gNdsK0AfterGoSeeks, asset_id);

    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
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
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        memset(dst, 0, alloc_size);
        fclose(file);
        return FALSE;
    }
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
 * read as a per-event delta by verify-battle-playable-down-air-stall.ps1, and
 * a load that stopped counting its header would look like a load that never
 * happened. */
/* Raw NitroFS range read. No asset-table entry, no o2r header, no file id --
 * slice 1's resident figatree pack is the only such payload the animation path
 * reads, and it is streamed in chunks because the seam's budget is one BGM
 * packet (~186 ms), not the whole load.
 *
 * It counts through the SAME open/short-read/payload counters as the asset
 * loads here, deliberately: a run whose after-GO animation I/O is supposed to
 * read zero must not have a second, uncounted reader hiding in it. */
s32 ndsRelocAssetReadRawRange(const char *path, u32 offset, void *dst,
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
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1u, (size_t)bytes, file) != (size_t)bytes)
    {
        gNdsRelocAssetShortReadCount++;
        fclose(file);
        return FALSE;
    }
    fclose(file);
    gNdsRelocAssetPayloadReadCount++;
    return TRUE;
}

/* The chunked form of the same read, one open for the whole payload. Contract
 * and the measurement that motivated it: include/nds/nds_reloc_assets.h. */
s32 ndsRelocAssetStreamOpen(NdsRelocAssetStream *stream, const char *path)
{
    if (stream == NULL)
    {
        return FALSE;
    }
    stream->file = NULL;
    if (path == NULL)
    {
        return FALSE;
    }
    stream->file = (void *)fopen(path, "rb");
    if (stream->file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }
    return TRUE;
}

s32 ndsRelocAssetStreamRead(NdsRelocAssetStream *stream, u32 offset, void *dst,
                            u32 bytes)
{
    FILE *file;

    if ((stream == NULL) || (stream->file == NULL) || (dst == NULL) ||
        (bytes == 0u))
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
    gNdsRelocAssetPayloadReadCount++;
    return TRUE;
}

void ndsRelocAssetStreamClose(NdsRelocAssetStream *stream)
{
    if ((stream == NULL) || (stream->file == NULL))
    {
        return;
    }
    fclose((FILE *)stream->file);
    stream->file = NULL;
}

s32 ndsRelocAssetLoadHeaderAndData(u32 asset_id, void *dst,
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

    file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        gNdsRelocAssetOpenFailCount++;
        return FALSE;
    }

    if (ndsRelocAssetReadHeaderFromFile(file, entry->file_id, &header,
                                        &data_offset) == FALSE)
    {
        fclose(file);
        return FALSE;
    }
    gNdsRelocAssetHeaderReadCount++;

    if ((size_t)header.data_size > dst_capacity)
    {
        gNdsRelocAssetShortReadCount++;
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
        fclose(file);
        return FALSE;
    }
    if (fread(dst, 1, header.data_size, file) != header.data_size)
    {
        gNdsRelocAssetShortReadCount++;
        fclose(file);
        return FALSE;
    }
    fclose(file);

    if (out_header != NULL)
    {
        *out_header = header;
    }
    gNdsRelocAssetPayloadReadCount++;
    NDS_K0_MARK(gNdsK0AfterGoFatReads, asset_id);
    return TRUE;
}
