/* Relocation backend.
 *
 * BattleShip call sites pass file IDs as the address of a generated
 * ll...FileID symbol. The generated relocation header is not present in this
 * checkout, so the DS port owns a narrow manifest for the imported slices.
 * The current Opening Room step mirrors the sm64-nds NitroFS pattern: the
 * eight real BattleShip_o2r resources are staged into the ROM, copied into the
 * original task heap, blanket u32 byte-swapped, internally pointer-relocated,
 * and resolved through selected symbol-offset probes. The startup logo Sprite
 * and the current MVCommon logo/spotlight MObjSub slice have narrow
 * mixed-width normalizers; general mixed-width struct fixups, external
 * dependencies, and renderer-safe texture/display-list fixups remain deferred
 * and are reported separately. */
#include "nds_scene_harness_config.h"

#include <stdio.h>

#include <nds/arm9/cache.h>
#include <nds/generated/nds_fighter_production.generated.h>
#include <nds/nds_p4.h>
/* P7 C1: the contents' 1P art sprites (scripts/p4/p4_onep_art.py). */
#include "nds_p4_onep_art.generated.h"
#include <nds/nds_battlepack_anim.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_frontend_overlay.h>
#include <nds/nds_shield_pose.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
/* P2-3f9: gSCManagerBattleState -- the animation cache's BattlePack carve is a
 * per-match decision and the roster is where that decision comes from. */
#include <sc/scene.h>
#include <nds/nds_p5.h>


#define NDS_RELOC_OPENING_ROOM_FILE_COUNT 8u
#define NDS_RELOC_OPENING_ROOM_FILE_MASK 0xffu
#define NDS_RELOC_LOADED_FILE_CAPACITY 96u
#define NDS_RELOC_NORMALIZED_MOBJ_SUB_CAPACITY 128u
#define NDS_RELOC_NORMALIZED_WEAPON_ATTR_CAPACITY 64u
#define NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES (128u * 1024u)

#define NDS_RELOC_ASSET_INVALID 0xffffffffu
#define NDS_RELOC_ASSET_MN_COMMON 0u
#define NDS_RELOC_ASSET_N64_LOGO 194u
#define NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE 37u
#define NDS_RELOC_ASSET_IF_COMMON_PLAYER 0xa6u
#define NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS 0x52u
/* relocData file 87 (87_IFCommonItem.spritelist): the one sprite every
 * common item's pickup arrow draws. */
#define NDS_RELOC_ASSET_IF_COMMON_ITEM 0x57u
#define NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE 0xa4u
#define NDS_RELOC_ASSET_IF_COMMON_TIMER 0xa5u
#define NDS_RELOC_ASSET_IF_COMMON_DIGITS 0x24u
#define NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE 0xc5u
#define NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS 0x26u
#define NDS_RELOC_ASSET_SY_KSEG1_VALIDATE 0xc7u
#define NDS_RELOC_ASSET_MV_COMMON 52u
#define NDS_RELOC_ASSET_OPENING_COMMON 65u
#define NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION 63u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE1 56u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE2 57u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE3 58u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE4 59u
#define NDS_RELOC_ASSET_OPENING_RUN 55u
#define NDS_RELOC_ASSET_OPENING_YAMABUKI 71u
#define NDS_RELOC_ASSET_OPENING_SECTOR 73u
#define NDS_RELOC_ASSET_OPENING_RUN_CRASH 75u
#define NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER 90u
#define NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1 53u
#define NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2 54u
#define NDS_RELOC_ASSET_MN_TITLE 167u
#define NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM 168u
#define NDS_RELOC_ASSET_MN_VS_MODE 6u
#define NDS_RELOC_ASSET_MN_PLAYERS_COMMON 0x11u
#define NDS_RELOC_ASSET_MN_PLAYERS_GAME_MODES 0x12u
#define NDS_RELOC_ASSET_MN_PLAYERS_PORTRAITS 0x13u
#define NDS_RELOC_ASSET_FT_EMBLEM_SPRITES 0x14u
#define NDS_RELOC_ASSET_MN_SELECT_COMMON 0x15u
#define NDS_RELOC_ASSET_MN_PLAYERS_SPOTLIGHT 0x16u
#define NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLACK 0x1au
#define NDS_RELOC_ASSET_MN_MAPS 0x1eu
#define NDS_RELOC_ASSET_MN_COMMON_FONTS 0x21u
#define NDS_RELOC_ASSET_FT_STOCKS_ZAKO 0x19u
#define NDS_RELOC_ASSET_MN_VS_RESULTS 0x22u
#define NDS_RELOC_ASSET_SC1P_TRAINING_SPRITES 0x1du /* reloc_extern_data/MiscData029, stage_reloc_file.py */
#define NDS_RELOC_ASSET_NNESS_MODEL 0x138u /* reloc_fighters_main/NNessModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NPURIN_MODEL 0x132u /* reloc_fighters_main/NPurinModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NPIKACHU_MODEL 0x133u /* reloc_fighters_main/NPikachuModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NKIRBY_MODEL 0x131u /* reloc_fighters_main/NKirbyModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NCAPTAIN_MODEL 0x137u /* reloc_fighters_main/NCaptainModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NYOSHI_MODEL 0x130u /* reloc_fighters_main/NYoshiModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NLINK_MODEL 0x136u /* reloc_fighters_main/NLinkModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NSAMUS_MODEL 0x135u /* reloc_fighters_main/NSamusModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NDONKEY_MODEL 0x134u /* reloc_fighters_main/NDonkeyModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NFOX_MODEL 0x12fu /* reloc_fighters_main/NFoxModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_NMARIO_MODEL 0x12du /* reloc_fighters_main/NMarioModel, admit_fighter.py --fighter polygons */
#define NDS_RELOC_ASSET_BOSS_MODEL 0x158u /* reloc_fighters_main/BossModel, admit_fighter.py --fighter boss */
#define NDS_RELOC_ASSET_BOSS_MAIN_MOTION 0xf9u /* reloc_fighters_main/BossMainMotion, admit_fighter.py --fighter boss */
#define NDS_RELOC_ASSET_BOSS_MAIN 0xfau /* reloc_fighters_main/BossMain, admit_fighter.py --fighter boss */
/* 250_BossMain.c dBossMain_attr: right after the 12-byte sprites at 0xdc. */
#define NDS_RELOC_SYMBOL_BOSS_MAIN_ATTRIBUTES 0xe8u
/* The polygons' shared texture bank (302, o2r reloc_extern_data/MiscData302)
 * every polygon model names, and MasterHandIcon (345), which every polygon
 * main and Master Hand's main name: extern dependencies only. */
#define NDS_RELOC_ASSET_NCOMMON_TEXTURE 0x12eu
#define NDS_RELOC_ASSET_MASTER_HAND_ICON 0x159u
#define NDS_RELOC_ASSET_MN_SCREEN_ADJUST 0xfu /* reloc_menus/MNScreenAdjust, stage_reloc_file.py */
#define NDS_RELOC_ASSET_BONUS2_COMMON 0x88u /* reloc_bonus/Bonus2Common, stage_reloc_file.py */
#define NDS_RELOC_ASSET_BONUS_PICTURE_PLATFORM 0xeu /* reloc_bonus/BonusPicturePlatform, stage_reloc_file.py */
#define NDS_RELOC_ASSET_BONUS_PICTURE 0xdu /* reloc_bonus/BonusPicture, stage_reloc_file.py */
#define NDS_RELOC_ASSET_CHARACTER_NAMES 0xcu /* reloc_misc_named/CharacterNames, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY 0x18u /* reloc_menus/MNPlayersDifficulty, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC_EXPLAIN_MAIN 0xfcu /* reloc_scene/SCExplainMain, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS 0xc6u /* reloc_scene/SCExplainGraphics, stage_reloc_file.py */
#define NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_YELLOW 0x1bu /* reloc_stages/GRWallpaperTrainingYellow, stage_reloc_file.py */
#define NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLUE 0x1cu /* reloc_stages/GRWallpaperTrainingBlue, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC1P_TRAINING_MODE 0xfeu /* reloc_scene/SC1PTrainingMode, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_SOUND_TEST 0xc4u /* reloc_menus/MNSoundTest, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_BACKUP_CLEAR_HEADER_OPTION 0x4eu /* reloc_menus/MNBackupClearHeaderOption, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_BACKUP_CLEAR 0x4du /* reloc_menus/MNBackupClear, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_OPTION 0x4u /* reloc_menus/MNOption, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CHARACTERS 0x10u /* reloc_menus/MNCharacters, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MNVS_RECORD_MAIN 0x1fu /* reloc_menus/MNVSRecordMain, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_DATA_COMMON 0x20u /* reloc_menus/MNDataCommon, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_DATA 0x5u /* reloc_menus/MNData, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_YOSHI_TOP 0xadu /* reloc_menus/MNCongraYoshiTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_YOSHI_BOTTOM 0xacu /* reloc_menus/MNCongraYoshiBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_SAMUS_TOP 0xb1u /* reloc_menus/MNCongraSamusTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_SAMUS_BOTTOM 0xb0u /* reloc_menus/MNCongraSamusBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_PURIN_TOP 0xb5u /* reloc_menus/MNCongraPurinTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_PURIN_BOTTOM 0xb4u /* reloc_menus/MNCongraPurinBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_TOP 0xafu /* reloc_menus/MNCongraPikachuTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_BOTTOM 0xaeu /* reloc_menus/MNCongraPikachuBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_NESS_TOP 0xc1u /* reloc_menus/MNCongraNessTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_NESS_BOTTOM 0xc0u /* reloc_menus/MNCongraNessBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_MARIO_TOP 0xbbu /* reloc_menus/MNCongraMarioTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_MARIO_BOTTOM 0xbau /* reloc_menus/MNCongraMarioBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_LUIGI_TOP 0xbdu /* reloc_menus/MNCongraLuigiTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_LUIGI_BOTTOM 0xbcu /* reloc_menus/MNCongraLuigiBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_LINK_TOP 0xb3u /* reloc_menus/MNCongraLinkTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_LINK_BOTTOM 0xb2u /* reloc_menus/MNCongraLinkBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_KIRBY_TOP 0xabu /* reloc_menus/MNCongraKirbyTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_KIRBY_BOTTOM 0xaau /* reloc_menus/MNCongraKirbyBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_FOX_TOP 0xbfu /* reloc_menus/MNCongraFoxTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_FOX_BOTTOM 0xbeu /* reloc_menus/MNCongraFoxBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_DONKEY_TOP 0xb9u /* reloc_menus/MNCongraDonkeyTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_DONKEY_BOTTOM 0xb8u /* reloc_menus/MNCongraDonkeyBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_TOP 0xb7u /* reloc_menus/MNCongraCaptainTop, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_BOTTOM 0xb6u /* reloc_menus/MNCongraCaptainBottom, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MV_ENDING 0x4cu /* reloc_movies/MVEnding, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC_STAFFROLL 0xc3u /* reloc_scene/SCStaffroll, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_MESSAGE 0x9u /* reloc_menus/MNMessage, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN_PLAYERS1P_MODE 0x17u /* reloc_menus/MNPlayers1PMode, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN1P_CONTINUE 0x4fu /* reloc_menus/MN1PContinue, stage_reloc_file.py */
#define NDS_RELOC_ASSET_MN1P 0x2u /* reloc_menus/MN1P, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC1P_INTRO 0xbu /* reloc_scene/SC1PIntro, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC1P_CHALLENGER 0xau /* reloc_scene/SC1PChallenger, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC1P_STAGE_CLEAR3 0x97u /* reloc_scene/SC1PStageClear3, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC1P_STAGE_CLEAR2 0x51u /* reloc_scene/SC1PStageClear2, stage_reloc_file.py */
#define NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1 0x50u /* reloc_scene/SC1PStageClear1, stage_reloc_file.py */
#define NDS_RELOC_ASSET_FT_EMBLEM_MODELS 0x23u
#define NDS_RELOC_ASSET_TRANSITION_AEROPLANE 0x28u
#define NDS_RELOC_ASSET_TRANSITION_CHECK 0x29u
#define NDS_RELOC_ASSET_TRANSITION_GAKUBUTHI 0x2au
#define NDS_RELOC_ASSET_TRANSITION_KANNON 0x2bu
#define NDS_RELOC_ASSET_TRANSITION_STAR 0x2cu
#define NDS_RELOC_ASSET_TRANSITION_SUDARE1 0x2du
#define NDS_RELOC_ASSET_TRANSITION_SUDARE2 0x2eu
#define NDS_RELOC_ASSET_TRANSITION_BLOCK 0x30u
#define NDS_RELOC_ASSET_TRANSITION_ROTSCALE 0x31u
#define NDS_RELOC_ASSET_TRANSITION_CURTAIN 0x32u
#define NDS_RELOC_ASSET_TRANSITION_CAMERA 0x33u
#define NDS_RELOC_ASSET_STAGE_CASTLE 0x1005fu
#define NDS_RELOC_ASSET_STAGE_DREAM_LAND 0x10058u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_113 0x71u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_103 0x67u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_104 0x68u
#define NDS_RELOC_ASSET_MISC_DATA_BANK_152 0x98u
#define NDS_RELOC_ASSET_GR_PUPUPU_MAP 0xffu
#define NDS_RELOC_ASSET_GR_INISHIE_MAP 0x104u
#define NDS_RELOC_ASSET_GR_HYRULE_MAP 0x109u
#if NDS_P2_1P_GAME
/* P2-6 step 7 (boss). Final Destination's runtime map (Last): map file
 * 266 = 0x10a (GRLastMap) plus its geometry/display bank 114 = 0x72
 * (ExternDataBank114 = StageLastFile2). A bank name carries the relocData
 * file number in decimal and the asset id is that number in hex, the same
 * derivation as every id here. The 1P arenas ride the 1P Game flag, so
 * there is no NDS_P2_STAGE_LAST. */
#define NDS_RELOC_ASSET_GR_LAST_MAP 0x10au
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_114 0x72u
/* The wallpaper sprite container the map header references (96 = 0x60,
 * StageLastBackground, held by the o2r container named StageYamabukiWallpaper;
 * nds_reloc_assets.c explains the mislabelling). The native
 * packet draws the wallpaper, but the extern-tree loader refuses a parent
 * whose dependency is unrowed, so the file is rowed to satisfy the map's
 * fixup -- the Yoshi shape (sprite-container namespace 0x10000 | id). */
#define NDS_RELOC_ASSET_STAGE_LAST_WALLPAPER 0x10060u
/* The other 1P arenas, same derivation (relocData number in decimal, id in
 * hex) and the same reason each dependency is rowed: the extern-tree loader
 * refuses a map whose extern table names an unrowed file.
 *   Meta Crystal: GRMetalMap 269 = 0x10d, StageMetalFile2 117 = 0x75, and
 *     the wallpaper container StageMetalBackground 98 = 0x62 (held by the
 *     o2r container named StageLastWallpaper; nds_reloc_assets.c explains
 *     the mislabelling).
 *   Duel Zone: GRZakoMap 268 = 0x10c, StageBattlefieldFile2 116 = 0x74, and
 *     the wallpaper StageBattlefieldBackground 97 = 0x61 (o2r container
 *     StageInishieWallpaper).
 *   Small Yoshi's Island: GRYosterSmallMap 270 = 0x10e and
 *     StageYosterSmallFile2 118 = 0x76, which depends on StageYosterImages
 *     (bank 110, rowed with Yoshi's Island) and the StageYoshi wallpaper
 *     (rowed with it too).
 *   Beta Dream Land (the 1P Kirby Team arena): GRPupupuSmallMap 256 = 0x100,
 *     StagePupupuBeta1 101 = 0x65, which depends on StagePupupuBetaImages
 *     100 = 0x64, and the StageDreamLand wallpaper (rowed since P1). */
#define NDS_RELOC_ASSET_GR_METAL_MAP 0x10du
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_117 0x75u
#define NDS_RELOC_ASSET_STAGE_METAL_WALLPAPER 0x10062u
#define NDS_RELOC_ASSET_GR_YOSTER_SMALL_MAP 0x10eu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_118 0x76u
#define NDS_RELOC_ASSET_GR_PUPUPU_SMALL_MAP 0x100u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_101 0x65u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_100 0x64u
#define NDS_RELOC_ASSET_GR_ZAKO_MAP 0x10cu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_116 0x74u
#define NDS_RELOC_ASSET_STAGE_ZAKO_WALLPAPER 0x10061u
/* The bonus boards (P2-6 step 5/6), same derivation as the arenas above:
 * twelve Break the Targets maps 271-282 = 0x10f-0x11a with their geometry
 * banks 124-135, twelve Board the Platforms maps 283-294 = 0x11b-0x126 with
 * banks 137-148 (o2r reloc_bonus/BonusDataBank*), Race 295 = 0x127 with banks
 * 149 and 162, the four shared image banks 120-123 the geometry banks depend
 * on (Mario/Donkey/Luigi 120, Fox/Samus/Captain 121, Yoshi/Pikachu/Purin
 * 122, Link/Kirby/Ness 123), the shared wallpaper 119 every board map names
 * (held by the o2r container named StageMetalWallpaper -- nds_reloc_assets.c
 * explains the mislabelling), the target attribute file 253 the boards load
 * (llITBonus1ObjectHeaderFileID) with the target model file 150 it names,
 * and Kirby's copy-ability table 230 = 0xe6
 * the Kirby Team stage loads (o2r MiscData230). Each is rowed because the
 * extern-tree loader refuses a parent with an unrowed dependency. */
#define NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER 0x10077u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_120 0x78u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_121 0x79u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_122 0x7au
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_123 0x7bu
#define NDS_RELOC_ASSET_GR_BONUS1_MARIO_MAP 0x10fu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_124 0x7cu
#define NDS_RELOC_ASSET_GR_BONUS1_FOX_MAP 0x110u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_125 0x7du
#define NDS_RELOC_ASSET_GR_BONUS1_DONKEY_MAP 0x111u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_126 0x7eu
#define NDS_RELOC_ASSET_GR_BONUS1_SAMUS_MAP 0x112u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_127 0x7fu
#define NDS_RELOC_ASSET_GR_BONUS1_LUIGI_MAP 0x113u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_128 0x80u
#define NDS_RELOC_ASSET_GR_BONUS1_LINK_MAP 0x114u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_129 0x81u
#define NDS_RELOC_ASSET_GR_BONUS1_YOSHI_MAP 0x115u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_130 0x82u
#define NDS_RELOC_ASSET_GR_BONUS1_CAPTAIN_MAP 0x116u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_131 0x83u
#define NDS_RELOC_ASSET_GR_BONUS1_KIRBY_MAP 0x117u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_132 0x84u
#define NDS_RELOC_ASSET_GR_BONUS1_PIKACHU_MAP 0x118u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_133 0x85u
#define NDS_RELOC_ASSET_GR_BONUS1_PURIN_MAP 0x119u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_134 0x86u
#define NDS_RELOC_ASSET_GR_BONUS1_NESS_MAP 0x11au
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_135 0x87u
#define NDS_RELOC_ASSET_GR_BONUS2_MARIO_MAP 0x11bu
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_137 0x89u
#define NDS_RELOC_ASSET_GR_BONUS2_FOX_MAP 0x11cu
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_138 0x8au
#define NDS_RELOC_ASSET_GR_BONUS2_DONKEY_MAP 0x11du
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_139 0x8bu
#define NDS_RELOC_ASSET_GR_BONUS2_SAMUS_MAP 0x11eu
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_140 0x8cu
#define NDS_RELOC_ASSET_GR_BONUS2_LUIGI_MAP 0x11fu
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_141 0x8du
#define NDS_RELOC_ASSET_GR_BONUS2_LINK_MAP 0x120u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_142 0x8eu
#define NDS_RELOC_ASSET_GR_BONUS2_YOSHI_MAP 0x121u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_143 0x8fu
#define NDS_RELOC_ASSET_GR_BONUS2_CAPTAIN_MAP 0x122u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_144 0x90u
#define NDS_RELOC_ASSET_GR_BONUS2_KIRBY_MAP 0x123u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_145 0x91u
#define NDS_RELOC_ASSET_GR_BONUS2_PIKACHU_MAP 0x124u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_146 0x92u
#define NDS_RELOC_ASSET_GR_BONUS2_PURIN_MAP 0x125u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_147 0x93u
#define NDS_RELOC_ASSET_GR_BONUS2_NESS_MAP 0x126u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_148 0x94u
#define NDS_RELOC_ASSET_GR_BONUS3_MAP 0x127u
#define NDS_RELOC_ASSET_BONUS_DATA_BANK_149 0x95u
#define NDS_RELOC_ASSET_MISC_DATA_162 0xa2u
#define NDS_RELOC_ASSET_IT_BONUS1_OBJECT 0x96u
/* US file 253 (relocFileDescriptions.us.txt), the target's ITAttributes;
 * its one extern is the target model in file 150 above. */
#define NDS_RELOC_ASSET_IT_BONUS1_OBJECT_HEADER 0xfdu
#define NDS_RELOC_ASSET_KIRBY_SPECIAL1 0xe6u
#endif
/* P2-4 opt-in stages. An asset id is its relocData file number in hex, which
 * is how every id here was derived: Yoster 263/93/111/154 and Castle
 * 259/106/156. Defined unconditionally -- they are constants, and the arms
 * that use them carry the flag. */
#define NDS_RELOC_ASSET_GR_YOSTER_MAP 0x107u
#define NDS_RELOC_ASSET_STAGE_YOSHI 0x1005du
/* relocData 110 (110_StageYosterImages), which extern bank 111 depends on.
 * Rowing 111 without it left the dependency unresolved, so its pointer stayed
 * raw, gMPCollisionGroundData never got set, and mpCollisionInitGroundData
 * aborted on the NULL -- the same two-table trap as Peach's Castle's borrowed
 * wallpaper. The abort ledger named it in one run:
 * LOOPABORTDEP unresolved=1 token=6e parent=6f. */
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_110 0x6eu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_111 0x6fu
#define NDS_RELOC_ASSET_MISC_DATA_BANK_154 0x9au
#define NDS_RELOC_ASSET_GR_CASTLE_MAP 0x103u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_106 0x6au
#define NDS_RELOC_ASSET_MISC_DATA_BANK_156 0x9cu
#define NDS_RELOC_ASSET_MV_OPENING_ROOM_WALLPAPER 0x5au
#define NDS_RELOC_ASSET_GR_JUNGLE_MAP 0x105u
#define NDS_RELOC_ASSET_STAGE_JUNGLE 0x1005cu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_108 0x6cu
#define NDS_RELOC_ASSET_MISC_DATA_BANK_158 0x9eu
#define NDS_RELOC_ASSET_GR_ZEBES_MAP 0x101u
#define NDS_RELOC_ASSET_STAGE_ZEBES 0x10059u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_105 0x69u
#define NDS_RELOC_ASSET_MISC_DATA_BANK_157 0x9du
#define NDS_RELOC_ASSET_GR_YAMABUKI_MAP 0x108u
#define NDS_RELOC_ASSET_STAGE_POKEMON 0x1005eu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_112 0x70u
#define NDS_RELOC_ASSET_MISC_DATA_BANK_159 0x9fu
#define NDS_RELOC_ASSET_MISC_DATA_BANK_160 0xa0u
#define NDS_RELOC_ASSET_STAGE_HYRULE_WALLPAPER 0x1005bu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_107 0x6bu
#define NDS_RELOC_ASSET_MISC_DATA_BANK_155 0x9bu
#define NDS_RELOC_ASSET_GR_SECTOR_MAP 0x106u
#define NDS_RELOC_ASSET_STAGE_SECTOR 0x10063u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_109_STAGE 0x6du
#define NDS_RELOC_ASSET_MISC_DATA_BANK_153 0x99u
#define NDS_RELOC_ASSET_FT_MANAGER_COMMON 0xa3u
#define NDS_RELOC_ASSET_MARIO_MAIN 0xcbu
#define NDS_RELOC_ASSET_MARIO_MAIN_MOTION 0xcau
#define NDS_RELOC_ASSET_MARIO_MODEL 0x128u
#define NDS_RELOC_ASSET_MARIO_SHIELD_POSE 0x12au
#define NDS_RELOC_ASSET_MARIO_SPECIAL1 0xccu
#define NDS_RELOC_ASSET_MARIO_SPECIAL2 0x164u
#define NDS_RELOC_ASSET_MARIO_SPECIAL3 0x129u
#define NDS_RELOC_ASSET_FOX_MAIN 0xd1u
#define NDS_RELOC_ASSET_FOX_MAIN_MOTION 0xd0u
#define NDS_RELOC_ASSET_FOX_MODEL 0x139u
#define NDS_RELOC_ASSET_FOX_SHIELD_POSE 0x13au
#define NDS_RELOC_ASSET_FOX_SPECIAL1 0xd2u
#define NDS_RELOC_ASSET_FOX_SPECIAL2 0x15au
#define NDS_RELOC_ASSET_FOX_SPECIAL3 0xa1u
#define NDS_RELOC_ASSET_FOX_SPECIAL4 0x13cu
#define NDS_RELOC_ASSET_EF_COMMON_EFFECTS1 0x53u
#define NDS_RELOC_ASSET_EF_COMMON_EFFECTS2 0x54u
#define NDS_RELOC_ASSET_EF_COMMON_EFFECTS3 0x55u
#define NDS_RELOC_ASSET_MISC_DATA_201 0xc9u
#define NDS_RELOC_ASSET_MISC_DATA_299 0x12bu
#define NDS_RELOC_ASSET_MISC_DATA_315 0x13bu
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_109 0x6du
/* P2-3f48. The item subsystem's shared data file and its one dependency.
 * ITCommonData carries the display-object descriptors, models, textures and
 * animation for EVERY common, monster and stage item, which is why the whole
 * of P2-5 waits on it: each descriptor reaches it through
 * &gITManagerCommonData (decomp it/itmanager.c:106,148). Its payload is 3,392
 * bytes and it declares 68 external pointers, all of them into MiscData086 --
 * which Yoshi's own reloc closure already stages, so on a ROM carrying him
 * this pair costs 3,392 bytes rather than 82,976. */
#define NDS_RELOC_ASSET_IT_COMMON_DATA 0xfbu
#define NDS_RELOC_ASSET_MISC_DATA_086 0x56u
#define NDS_RELOC_ASSET_MARIO_ANIM_WAIT 0x1f3u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK1 0x1f4u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK2 0x1f5u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK3 0x1f6u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK_END 0x1f7u
#define NDS_RELOC_ASSET_MARIO_ANIM_FIRE_FLOWER_AIR 0x281u
#define NDS_RELOC_ASSET_MARIO_ANIM_APPEAR1 0x279u
#define NDS_RELOC_ASSET_MARIO_ANIM_APPEAR2 0x27au
#define NDS_RELOC_ASSET_FOX_ANIM_FIRST 0x282u
#define NDS_RELOC_ASSET_FOX_ANIM_LAST 0x31fu
#define NDS_RELOC_ASSET_FOX_ANIM_APPEAR 0x309u
#define NDS_RELOC_ASSET_FOX_ANIM_ARWING 0x30au

/* Slice 1 phase 7. `ndsK0AfterGoFighter` attributes every K0 counter from the
 * asset id alone, so its spans and this file's must agree exactly or the
 * assertion would be measuring a different fighter than the one the pack
 * serves. Pinned rather than shared because these defines are text-matched by
 * check-ft-hitstatus-fixtures.ps1 and must stay where they are. */
_Static_assert(NDS_RELOC_ASSET_MARIO_ANIM_WAIT == NDS_K0_MARIO_ANIM_FIRST,
               "K0 fighter span drifted from the Mario animation asset ids");
_Static_assert(NDS_RELOC_ASSET_MARIO_ANIM_FIRE_FLOWER_AIR ==
                   NDS_K0_MARIO_ANIM_LAST,
               "K0 fighter span drifted from the Mario animation asset ids");
_Static_assert(NDS_RELOC_ASSET_FOX_ANIM_FIRST == NDS_K0_FOX_ANIM_FIRST,
               "K0 fighter span drifted from the Fox animation asset ids");
_Static_assert(NDS_RELOC_ASSET_FOX_ANIM_LAST == NDS_K0_FOX_ANIM_LAST,
               "K0 fighter span drifted from the Fox animation asset ids");

#define NDS_FIGHTER_MARIOFOX_FILE_MASK 0x7fffu
#define NDS_FIGHTER_MARIOFOX_SETUP_FILES (1u << 0)

#define NDS_FIGHTER_MANAGER_EXTERN_COMMON (1u << 0)
#define NDS_FIGHTER_MANAGER_EXTERN_COMMON_MOVESET (1u << 1)
#define NDS_FIGHTER_MANAGER_EXTERN_MARIO_MAIN (1u << 2)
#define NDS_FIGHTER_MANAGER_EXTERN_FOX_MAIN (1u << 3)
#define NDS_FIGHTER_MANAGER_EXTERN_REQUIRED_MASK 0x0fu

#define NDS_FIGHTER_MANAGER_STATUS_MARIO_MAINMOTION (1u << 0)
#define NDS_FIGHTER_MANAGER_STATUS_MARIO_MODEL (1u << 1)
#define NDS_FIGHTER_MANAGER_STATUS_MARIO_SHIELD (1u << 2)
#define NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL1 (1u << 3)
#define NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL2 (1u << 4)
#define NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL3 (1u << 5)
#define NDS_FIGHTER_MANAGER_STATUS_FOX_MAINMOTION (1u << 6)
#define NDS_FIGHTER_MANAGER_STATUS_FOX_MODEL (1u << 7)
#define NDS_FIGHTER_MANAGER_STATUS_FOX_SHIELD (1u << 8)
#define NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL1 (1u << 9)
#define NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL2 (1u << 10)
#define NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL3 (1u << 11)
#define NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL4 (1u << 12)
#define NDS_FIGHTER_MANAGER_STATUS_REQUIRED_MASK 0x1fffu

/* O2R stores each external file id as u16.  Keep that native width here too:
 * it lets the loader cover the full source roster without turning every loaded
 * file record into a u32-padded dependency table.  An offline census of the US
 * fighter mains puts the maximum at KirbyMain's 144 ids (DonkeyMain is 81;
 * Mario/Fox/Luigi are 48/51/62). P4 builds: EXTRA's MRGAW main has 284, the
 * roster's maximum (Banjo's 208 the main tree's; generate_p4_fighter.py
 * MAX_EXTERN_IDS checks every file). */
#define NDS_RELOC_EXTERN_FILE_ID_CAPACITY 288u

#define NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_MOBJ 0x042f8u
#define NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_DOBJ 0x07e98u
#define NDS_RELOC_SYMBOL_MVCOMMON_DESK_DOBJ 0x08df8u
#define NDS_RELOC_SYMBOL_MVCOMMON_OUTSIDE_DL 0x24200u
#define NDS_RELOC_SYMBOL_MVCOMMON_HAZE_DL 0x098f8u
#define NDS_RELOC_SYMBOL_MVCOMMON_SUNLIGHT_DL 0x24708u
#define NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ 0x0aeb8u
#define NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM 0x0af70u
#define NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MOBJ 0x1bc60u
#define NDS_RELOC_SYMBOL_MVCOMMON_LOGO_DOBJ 0x1c4a8u
#define NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MATANIM 0x1c52cu
#define NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_AIR_MOBJ 0x1dca0u
#define NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_GROUND_MOBJ 0x1f0f8u
#define NDS_RELOC_SYMBOL_MVCOMMON_DESK_GROUND_MOBJ 0x20480u
#define NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_DL 0x1f790u
#define NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_ANIM 0x1f924u
#define NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MOBJ 0x22c90u
#define NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_DL 0x22e18u
#define NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MATANIM 0x22f10u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_TRANSITION_OVERLAY_DL 0x05a0u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE1_CAM_ANIM 0x0000u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE2_CAM_ANIM 0x0000u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_WALLPAPER_SPRITE 0x26c88u
#define NDS_RELOC_SYMBOL_N64_LOGO_SPRITE 0x73c0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_A 0x05e0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_B 0x09a8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_C 0x0d80u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_D 0x1268u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_E 0x1628u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_F 0x1a00u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_G 0x1f08u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_H 0x2408u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_I 0x26b8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_J 0x2a90u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K 0x2f98u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L 0x3358u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M 0x3980u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N 0x3e88u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O 0x44b0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P 0x4890u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Q 0x4f10u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R 0x5418u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S 0x57f0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_T 0x5bd0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U 0x60d8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_V 0x65d8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_W 0x6c00u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X 0x7108u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y 0x7608u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Z 0x7ae8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_PERIOD 0x7e50u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_MARIO_CAM_ANIM 0x0000u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_DONKEY_CAM_ANIM 0x0030u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_SAMUS_CAM_ANIM 0x0060u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_FOX_CAM_ANIM 0x0090u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_LINK_CAM_ANIM 0x00c0u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_YOSHI_CAM_ANIM 0x00f0u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_PIKACHU_CAM_ANIM 0x0120u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_KIRBY_CAM_ANIM 0x0150u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_SAMUS 0x09960u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_MARIO 0x13310u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_FOX 0x1ccc0u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_PIKACHU 0x26670u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_COVER 0x2b2d0u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_LINK 0x09960u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_KIRBY 0x13310u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_DONKEY 0x1ccc0u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_YOSHI 0x26670u
#define NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER 0x058a0u
#define NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER 0x3ee58u
#define NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT 0x3cc90u
#define NDS_RELOC_SYMBOL_TITLE_LOGO_FULL 0x0bbb0u
#define NDS_RELOC_SYMBOL_TITLE_BORDER_UPPER 0x0c208u
#define NDS_RELOC_SYMBOL_TITLE_TM 0x0f398u
#define NDS_RELOC_SYMBOL_TITLE_CUTOUT 0x11988u
#define NDS_RELOC_SYMBOL_TITLE_TM_UNK 0x11aa8u
#define NDS_RELOC_SYMBOL_TITLE_COPYRIGHT 0x15320u
#define NDS_RELOC_SYMBOL_TITLE_PRESS_START 0x15a48u
#define NDS_RELOC_SYMBOL_TITLE_SUPER 0x16728u
#define NDS_RELOC_SYMBOL_TITLE_SMASH 0x245c8u
#define NDS_RELOC_SYMBOL_TITLE_BROS 0x25188u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME1 0x01018u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME2 0x02078u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME3 0x030d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME4 0x04138u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME5 0x05198u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME6 0x061f8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME7 0x07258u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME8 0x082b8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME9 0x09318u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME10 0x0a378u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME11 0x0b3d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME12 0x0c438u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME13 0x0d498u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME14 0x0e4f8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME15 0x0f558u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME16 0x105b8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME17 0x11618u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME18 0x12678u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME19 0x136d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME20 0x14738u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME21 0x15798u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME22 0x167f8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME23 0x17858u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME24 0x188b8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME25 0x19918u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME26 0x1a978u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME27 0x1b9d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME28 0x1ca38u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME29 0x1da98u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME30 0x1eaf8u
#define NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_LEFT 0x01e8u
#define NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_MIDDLE 0x0330u
#define NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_RIGHT 0x0568u
#define NDS_RELOC_SYMBOL_MNCOMMON_FRAME 0x1420u
#define NDS_RELOC_SYMBOL_MNCOMMON_DECAL_PAPER 0x2a30u
#define NDS_RELOC_SYMBOL_MNCOMMON_SMASH_LOGO 0x31f8u
#define NDS_RELOC_SYMBOL_MNCOMMON_GAME_MODE_TEXT 0xd240u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT0 0xd310u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT1 0xd3e0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT2 0xd4b0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT3 0xd580u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT4 0xd650u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT5 0xd720u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT6 0xd7f0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT7 0xd8c0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT8 0xd990u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT9 0xda60u
#define NDS_RELOC_SYMBOL_MNCOMMON_INFINITY 0xdc48u
#define NDS_RELOC_SYMBOL_MNCOMMON_ARROW_R 0xdd90u
#define NDS_RELOC_SYMBOL_MNCOMMON_ARROW_L 0xde30u
#define NDS_RELOC_SYMBOL_MNCOMMON_SMASH_BROS_COLLAGE 0x18000u
#define NDS_RELOC_SYMBOL_MNVSMODE_VS_START_TEXT 0x24c8u
#define NDS_RELOC_SYMBOL_MNVSMODE_RULE_PERIOD_TEXT 0x2748u
#define NDS_RELOC_SYMBOL_MNVSMODE_TIME_TEXT 0x28e0u
#define NDS_RELOC_SYMBOL_MNVSMODE_STOCK_TEXT 0x2a80u
#define NDS_RELOC_SYMBOL_MNVSMODE_TEAM_TEXT 0x2c20u
#define NDS_RELOC_SYMBOL_MNVSMODE_TIME_PERIOD_TEXT 0x2ec8u
#define NDS_RELOC_SYMBOL_MNVSMODE_MIN_TEXT 0x2fc8u
#define NDS_RELOC_SYMBOL_MNVSMODE_STOCK_PERIOD_TEXT 0x3248u
#define NDS_RELOC_SYMBOL_MNVSMODE_VS_OPTIONS_TEXT 0x3828u
#define NDS_RELOC_SYMBOL_MNVSMODE_CONSOLE_ICON_DARK 0x5eb0u
#define NDS_RELOC_SYMBOL_MNVSMODE_VS_TEXT 0x6118u
#define NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE 0x26c88u
/* All nine stage map files put their MPGroundData here; see
 * ndsRelocNormalizeGroundMapAsset below for the citations. */
#define NDS_RELOC_SYMBOL_GR_PUPUPU_MAP_HEADER 0x14u
#define NDS_RELOC_SYMBOL_GR_HYRULE_MAP_HEADER 0x14u
#define NDS_RELOC_SYMBOL_GR_INISHIE_MAP_HEADER 0x14u
#if NDS_P2_1P_GAME
/* llGRLastMapMapHeader = 0x0 (symbols file :4200): Last carries no VS-style
 * header at 0x14, so it stays out of sNdsRelocGroundMapAssets below, whose
 * normalizer is hardcoded to that offset. */
#define NDS_RELOC_SYMBOL_GR_LAST_MAP_HEADER 0x0u
#endif
#define NDS_RELOC_SYMBOL_MARIO_MAIN_ATTRIBUTES 0x428u
#define NDS_RELOC_SYMBOL_FOX_MAIN_ATTRIBUTES 0x46cu
#if NDS_P2_LUIGI
/* ftdata.c `FTData dFTLuigiData` field 24 is 0x00000580 -- the same field that
 * gives Mario 0x428 and Fox 0x46c -- and 221_LuigiMain.c agrees twice over
 * ("Pre-attributes data (352 words, 0x0580 bytes)", and its last pre-attribute
 * block is `@ 0x0574, 12 bytes` = 0x580). */
#define NDS_RELOC_SYMBOL_LUIGI_MAIN_ATTRIBUTES 0x580u
#define NDS_RELOC_ASSET_LUIGI_MAIN 0xddu
#endif
#if NDS_P2_DONKEY
#define NDS_RELOC_SYMBOL_DONKEY_MAIN_ATTRIBUTES 0x4a4u
#define NDS_RELOC_ASSET_DONKEY_MAIN 0xd5u
#endif
#if NDS_P2_CAPTAIN
/* 236_CaptainMain.c:87 -- "Pre-attributes data (290 words, 0x0488 bytes)",
 * the same shape that gives Donkey 0x4a4 at 213_DonkeyMain.c:88. */
#define NDS_RELOC_SYMBOL_CAPTAIN_MAIN_ATTRIBUTES 0x488u
#define NDS_RELOC_ASSET_CAPTAIN_MAIN 0xecu
#endif
/* dFTSamusData field 24 is 0x610; 217_SamusMain.c places the final 12-byte
 * skeleton pointer block at 0x604, so dSamusMain_attr begins at 0x610. */
#define NDS_RELOC_SYMBOL_SAMUS_MAIN_ATTRIBUTES 0x610u
#define NDS_RELOC_ASSET_SAMUS_MAIN 0xd9u
/* reloc_data_symbols.us.txt:3844/:3846. Bomb's WPAttributes starts at byte
 * 0x0c of SamusMain; Charge Shot's starts at byte 0 of SamusSpecial1. */
#define NDS_RELOC_SYMBOL_SAMUS_MAIN_BOMB_WEAPON_ATTRIBUTES 0x0cu
#define NDS_RELOC_ASSET_SAMUS_SPECIAL1 0xdau
#if NDS_P2_LINK
/* dFTLinkData field 24 is 0x708; 225_LinkMain.c's source FTAttributes starts
 * there. llLinkMainFileID is 0xe1 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_LINK_MAIN_ATTRIBUTES 0x708u
#define NDS_RELOC_ASSET_LINK_MAIN 0xe1u
/* reloc_data_symbols.us.txt:3850/:3852. Grounded Spin Attack's WPAttributes
 * starts at 0x0c of LinkMain; Boomerang's starts at 0 of LinkSpecial1. */
#define NDS_RELOC_SYMBOL_LINK_MAIN_SPIN_ATTACK_WEAPON_ATTRIBUTES 0x0cu
#define NDS_RELOC_ASSET_LINK_SPECIAL1 0xe2u
#endif
/* 247_YoshiMain.c: the pre-attributes data ends with the 12-byte skeleton
 * table at 0x470, so its source FTAttributes (dYoshiMain_attr) starts at
 * 0x47c. llYoshiMainFileID is 0xf7 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_YOSHI_MAIN_ATTRIBUTES 0x47cu
#define NDS_RELOC_ASSET_YOSHI_MAIN 0xf7u
/* reloc_data_symbols.us.txt:3879-3880. Egg Throw's and the stars' WPAttributes
 * overlap YoshiMain's file-handle words at 0x0c/0x40 exactly as Pikachu's
 * Thunder pair does. */
#define NDS_RELOC_SYMBOL_YOSHI_MAIN_EGG_THROW_WEAPON_ATTRIBUTES 0x0cu
#define NDS_RELOC_SYMBOL_YOSHI_MAIN_STAR_WEAPON_ATTRIBUTES 0x40u
/* fighter_production_manifest.json: dFTNessData field 24 puts his FTAttributes at
 * 0x5bc; llNessMainFileID is 0xef in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NESS_MAIN_ATTRIBUTES 0x5bcu
#define NDS_RELOC_ASSET_NESS_MAIN 0xefu
#define NDS_RELOC_ASSET_NESS_SPECIAL1 0xf0u
#define NDS_RELOC_SYMBOL_NESS_MAIN_PK_THUNDER_WEAPON_ATTRIBUTES 0xcu
#define NDS_RELOC_SYMBOL_NESS_MAIN_PK_THUNDER_TRAIL_WEAPON_ATTRIBUTES 0x40u
#define NDS_RELOC_SYMBOL_NESS_SPECIAL1_PK_FIRE_WEAPON_ATTRIBUTES 0x0u
/* fighter_production_manifest.json: dFTPurinData field 24 puts his FTAttributes at
 * 0x474; llPurinMainFileID is 0xe9 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_PURIN_MAIN_ATTRIBUTES 0x474u
#define NDS_RELOC_ASSET_PURIN_MAIN 0xe9u
/* fighter_production_manifest.json: dFTKirbyData field 24 puts his FTAttributes at
 * 0x808; llKirbyMainFileID is 0xe5 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_KIRBY_MAIN_ATTRIBUTES 0x808u
#define NDS_RELOC_ASSET_KIRBY_MAIN 0xe5u
#define NDS_RELOC_ASSET_KIRBY_MAIN_MOTION 0xe4u
#define NDS_RELOC_SYMBOL_KIRBY_MAIN_CUTTER_WEAPON_ATTRIBUTES 0x8u
#if NDS_P2_GDONKEY
/* fighter_production_manifest.json: dFTGDonkeyData field 24 puts his FTAttributes at
 * 0x3c8; llGDonkeyMainFileID is 0xd7 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_GDONKEY_MAIN_ATTRIBUTES 0x3c8u
#define NDS_RELOC_ASSET_GDONKEY_MAIN 0xd7u
#endif
#if NDS_P2_MMARIO
/* fighter_production_manifest.json: dFTMMarioData field 24 puts his FTAttributes at
 * 0x2a8; llMMarioMainFileID is 0xce in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_MMARIO_MAIN_ATTRIBUTES 0x2a8u
#define NDS_RELOC_ASSET_MMARIO_MAIN 0xceu
#define NDS_RELOC_ASSET_MMARIO_MAIN_MOTION 0xcdu
#define NDS_RELOC_ASSET_MMARIO_MODEL 0x12cu
#endif
#if NDS_P2_NMARIO
/* fighter_production_manifest.json: dFTNMarioData field 24 puts his FTAttributes at
 * 0x298; llNMarioMainFileID is 0xcf in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NMARIO_MAIN_ATTRIBUTES 0x298u
#endif
#define NDS_RELOC_ASSET_NMARIO_MAIN 0xcfu
#if NDS_P2_NFOX
/* fighter_production_manifest.json: dFTNFoxData field 24 puts his FTAttributes at
 * 0x2a4; llNFoxMainFileID is 0xd3 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NFOX_MAIN_ATTRIBUTES 0x2a4u
#endif
#define NDS_RELOC_ASSET_NFOX_MAIN 0xd3u
#if NDS_P2_NDONKEY
/* fighter_production_manifest.json: dFTNDonkeyData field 24 puts his FTAttributes at
 * 0x298; llNDonkeyMainFileID is 0xd6 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NDONKEY_MAIN_ATTRIBUTES 0x298u
#endif
#define NDS_RELOC_ASSET_NDONKEY_MAIN 0xd6u
#if NDS_P2_NSAMUS
/* fighter_production_manifest.json: dFTNSamusData field 24 puts his FTAttributes at
 * 0x3bc; llNSamusMainFileID is 0xdb in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NSAMUS_MAIN_ATTRIBUTES 0x3bcu
#endif
#define NDS_RELOC_ASSET_NSAMUS_MAIN 0xdbu
#if NDS_P2_NLUIGI
/* fighter_production_manifest.json: dFTNLuigiData field 24 puts his FTAttributes at
 * 0x298; llNLuigiMainFileID is 0xdf in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NLUIGI_MAIN_ATTRIBUTES 0x298u
#endif
#define NDS_RELOC_ASSET_NLUIGI_MAIN 0xdfu
#if NDS_P2_NLINK
/* fighter_production_manifest.json: dFTNLinkData field 24 puts his FTAttributes at
 * 0x2d8; llNLinkMainFileID is 0xe3 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NLINK_MAIN_ATTRIBUTES 0x2d8u
#endif
#define NDS_RELOC_ASSET_NLINK_MAIN 0xe3u
#if NDS_P2_NYOSHI
/* fighter_production_manifest.json: dFTNYoshiData field 24 puts his FTAttributes at
 * 0x2b8; llNYoshiMainFileID is 0xf8 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NYOSHI_MAIN_ATTRIBUTES 0x2b8u
#endif
#define NDS_RELOC_ASSET_NYOSHI_MAIN 0xf8u
#if NDS_P2_NCAPTAIN
/* fighter_production_manifest.json: dFTNCaptainData field 24 puts his FTAttributes at
 * 0x29c; llNCaptainMainFileID is 0xed in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NCAPTAIN_MAIN_ATTRIBUTES 0x29cu
#endif
#define NDS_RELOC_ASSET_NCAPTAIN_MAIN 0xedu
#if NDS_P2_NKIRBY
/* fighter_production_manifest.json: dFTNKirbyData field 24 puts his FTAttributes at
 * 0x2c0; llNKirbyMainFileID is 0xe7 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NKIRBY_MAIN_ATTRIBUTES 0x2c0u
#endif
#define NDS_RELOC_ASSET_NKIRBY_MAIN 0xe7u
#if NDS_P2_NPIKACHU
/* fighter_production_manifest.json: dFTNPikachuData field 24 puts his FTAttributes at
 * 0x2a8; llNPikachuMainFileID is 0xf5 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NPIKACHU_MAIN_ATTRIBUTES 0x2a8u
#endif
#define NDS_RELOC_ASSET_NPIKACHU_MAIN 0xf5u
#if NDS_P2_NPURIN
/* fighter_production_manifest.json: dFTNPurinData field 24 puts his FTAttributes at
 * 0x2a0; llNPurinMainFileID is 0xea in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NPURIN_MAIN_ATTRIBUTES 0x2a0u
#endif
#define NDS_RELOC_ASSET_NPURIN_MAIN 0xeau
#if NDS_P2_NNESS
/* fighter_production_manifest.json: dFTNNessData field 24 puts his FTAttributes at
 * 0x2f0; llNNessMainFileID is 0xf1 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_NNESS_MAIN_ATTRIBUTES 0x2f0u
#endif
#define NDS_RELOC_ASSET_NNESS_MAIN 0xf1u
/* dFTPikachuData field 24 is 0x41c; 243_PikachuMain.c's pre-attributes data is
 * 263 words, so its source FTAttributes starts there. llPikachuMainFileID is
 * 0xf3 in the US relocation symbol table. */
#define NDS_RELOC_SYMBOL_PIKACHU_MAIN_ATTRIBUTES 0x41cu
#define NDS_RELOC_ASSET_PIKACHU_MAIN 0xf3u
/* reloc_data_symbols.us.txt:3873-3877. Thunder's head/trail WPAttributes
 * overlap PikachuMain's file-handle words at 0x0c/0x40 exactly as Samus's Bomb
 * does; Thunder Jolt's air/ground pair sit back to back at the head of
 * PikachuSpecial1 (244_PikachuSpecial1.c). */
#define NDS_RELOC_SYMBOL_PIKACHU_MAIN_THUNDER_HEAD_WEAPON_ATTRIBUTES 0x0cu
#define NDS_RELOC_SYMBOL_PIKACHU_MAIN_THUNDER_TRAIL_WEAPON_ATTRIBUTES 0x40u
/* Both weapon attribute structs sit at file offset 0: the fireball's in file
 * 204 (llMarioSpecial1FireballWeaponAttributes = 0x0) and the blaster's in
 * file 210 (llFoxSpecial1BlasterWeaponAttributes = 0x0), per
 * decomp/BattleShip-main/tools/reloc_data_symbols.us.txt:3838/:3842. */

#define NDS_OPENING_PORTRAITS_CARD_WIDTH 300u
#define NDS_OPENING_PORTRAITS_CARD_HEIGHT 55u
#define NDS_OPENING_PORTRAITS_COVER_WIDTH 656u
#define NDS_OPENING_PORTRAITS_COVER_HEIGHT 55u
#define NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH 96u
#define NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT 64u
#define NDS_TITLE_FILE_BUFFER_SIZE 176000u
/* One granularity, named once, so a caller that has to pass it as a value
 * cannot drift from the macro that rounds with it. */
#define NDS_RELOC_ALIGN_BYTES 0x10u
#define NDS_RELOC_ALIGN(value) \
    (((value) + (NDS_RELOC_ALIGN_BYTES - 1u)) & ~(NDS_RELOC_ALIGN_BYTES - 1u))
#define NDS_RELOC_STAGE_CASTLE_STATIC_SIZE NDS_RELOC_ALIGN(0x26cd0u)
#define NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE NDS_RELOC_ALIGN(0x6890u)
#define NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT 264u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH 320u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT 240u
#define NDS_OPENING_ACTION_PREVIEW_CACHE_PIXELS 1u
#define NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE \
    (NDS_RELOC_STAGE_CASTLE_STATIC_SIZE + \
     NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE)
#define NDS_OPENING_ACTION_PREVIEW_FRAME_HOLD 36u

#define NDS_RELOC_G_IM_FMT_MAX 4u

#define NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ_READY (1u << 0)
#define NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM_READY (1u << 1)
#define NDS_OPENING_ROOM_FIRST_EVENT_READY_MASK \
    (NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM_READY)

#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_IDS_READY (1u << 0)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_DLS_READY (1u << 1)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_TABLE_READY (1u << 2)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_OPCODE_READY (1u << 3)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_READY_MASK \
    (NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_IDS_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_DLS_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_TABLE_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_OPCODE_READY)

#define NDS_FIGHTER_DL_OP_NOOP 0x00u
#define NDS_FIGHTER_DL_OP_VTX 0x01u
#define NDS_FIGHTER_DL_OP_MODIFYVTX 0x02u
#define NDS_FIGHTER_DL_OP_CULLDL 0x03u
#define NDS_FIGHTER_DL_OP_TRI1 0x05u
#define NDS_FIGHTER_DL_OP_TRI2 0x06u
#define NDS_FIGHTER_DL_OP_TEXTURE 0xd7u
#define NDS_FIGHTER_DL_OP_POPMTX 0xd8u
#define NDS_FIGHTER_DL_OP_SPECIAL_1 0xd5u
#define NDS_FIGHTER_DL_OP_MTX 0xdau
#define NDS_FIGHTER_DL_OP_GEOMETRYMODE 0xd9u
#define NDS_FIGHTER_DL_OP_MOVEWORD 0xdbu
#define NDS_FIGHTER_DL_OP_DL 0xdeu
#define NDS_FIGHTER_DL_OP_ENDDL 0xdfu
#define NDS_FIGHTER_DL_OP_SETOTHERMODE_H 0xe3u
#define NDS_FIGHTER_DL_OP_SETOTHERMODE_L 0xe2u
#define NDS_FIGHTER_DL_OP_SETSCISSOR 0xedu
#define NDS_FIGHTER_DL_OP_SETCOMBINE 0xfcu
#define NDS_FIGHTER_DL_OP_SETCIMG 0xffu
#define NDS_FIGHTER_DL_OP_SETFOGCOLOR 0xf8u
#define NDS_FIGHTER_DL_OP_SETBLENDCOLOR 0xf9u
#define NDS_FIGHTER_DL_OP_SETENVCOLOR 0xfbu
#define NDS_FIGHTER_DL_OP_SETPRIMCOLOR 0xfau
#define NDS_FIGHTER_DL_OP_SETTIMG 0xfdu
#define NDS_FIGHTER_DL_OP_SETTILE 0xf5u
#define NDS_FIGHTER_DL_OP_LOADBLOCK 0xf3u
#define NDS_FIGHTER_DL_OP_LOADTLUT 0xf0u
#define NDS_FIGHTER_DL_OP_SETTILESIZE 0xf2u
#define NDS_FIGHTER_DL_OP_RDPSETOTHERMODE 0xefu
#define NDS_FIGHTER_DL_OP_RDPPIPESYNC 0xe7u
#define NDS_FIGHTER_DL_OP_RDPLOADSYNC 0xe6u
#define NDS_FIGHTER_DL_OP_RDPTILESYNC 0xe8u
#define NDS_FIGHTER_DL_OP_RDPFULLSYNC 0xe9u
#define NDS_FIGHTER_DL_DRAW_WIDTH 96u
#define NDS_FIGHTER_DL_DRAW_HEIGHT 72u
#define NDS_FIGHTER_DL_DRAW_MAX_VTX 32u
#define NDS_FIGHTER_DL_DRAW_MAX_TRIS 96u
#define NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED 4u
#define NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED 32u

typedef struct NDSRelocLoadedFile {
    u32 asset_id;
    u32 bit;
    void *data;
    u32 data_size;
    u32 owner_scene;
    u32 owner_generation;
    u16 reloc_intern_offset;
    u16 reloc_extern_offset;
    u32 extern_count;
    /* Owned by this scene; zero-dependency animation reloads allocate nothing. */
    u16 *extern_file_ids;
    u32 external_fixup_count;
    u32 external_fixup_fail_count;
    u32 internal_fixup_count;
    u8 internal_fixups_applied;
    u8 external_fixups_applied;
    u8 format_fixups_applied;
    u8 fixups_applying;
    u8 offsets_are_relative;
    /* [0]/[1] preview pack kind and section; [2] nonzero for a compact
     * polygon model (ndsRelocCompactModelProbe). */
    u8 reserved[3];
    /* A compact polygon model's backed range, as source offsets from `data`:
     * only [compact_lo, compact_hi) is memory of this file. */
    u16 compact_lo;
    u16 compact_hi;
} NDSRelocLoadedFile;

/* THIRD SITE OF THE SAME CONDITION, and it must move with the other two: the
 * declarations in nds_preview_pack.h and the `#include "reloc_preview_pack.c"`
 * below. The `#else` arm here defines ndsRelocNativeSourceSize as a MACRO, so a
 * configuration that compiles the real function while leaving this narrow
 * turns that function's own definition into a macro expansion and fails with
 * "expected identifier or '(' before 'const'" inside reloc_preview_pack.c --
 * which is how this third site was found. */
static s32 ndsPreviewFileOffset(const NDSRelocLoadedFile *loaded,
                               u32 source_offset, u32 size, u32 *out_offset);
static u32 ndsRelocNativeSourceSize(const NDSRelocLoadedFile *loaded);
static u32 ndsRelocNativeRootOffset(const NDSRelocLoadedFile *loaded, const Gfx *dl);

typedef struct NDSRelocNormalizedMObjSub
{
    const MObjSub *record;
    u32 asset_id;
    u32 owner_generation;
} NDSRelocNormalizedMObjSub;

typedef struct NDSFighterDLScanContext {
    const NDSRelocLoadedFile *primary_file;
    u32 slot;
} NDSFighterDLScanContext;

typedef struct NDSFighterDLExecVtx {
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    u8 valid;
} NDSFighterDLExecVtx;

typedef struct NDSFighterDLExecState {
    const NDSRelocLoadedFile *primary_file;
    u32 slot;
    NDSFighterDLExecVtx vertices[32];
    u32 vertex_valid_mask;
    u32 vertex_decoded_count;
    u32 vertex_command_count;
    u32 triangle_command_count;
    u32 triangle_count;
    u32 triangle_valid_count;
    u32 color_checksum;
    s32 min_x;
    s32 max_x;
    s32 min_y;
    s32 max_y;
    s32 min_z;
    s32 max_z;
    u32 bounds_valid;
    u32 unsupported_opcode;
    u32 unsupported_command_count;
    u32 vertex_range_reject_count;
} NDSFighterDLExecState;

typedef struct NDSFighterDLDrawVtx {
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    u8 valid;
} NDSFighterDLDrawVtx;

typedef struct NDSFighterDLDrawTri {
    u8 v0;
    u8 v1;
    u8 v2;
} NDSFighterDLDrawTri;

typedef struct NDSFighterDLDrawState {
    const NDSRelocLoadedFile *primary_file;
    u32 slot;
    const Gfx *segment_e_base;
    const Gfx *segment_e_end;
    NDSFighterDLDrawVtx vertices[NDS_FIGHTER_DL_DRAW_MAX_VTX];
    NDSFighterDLDrawTri tris[NDS_FIGHTER_DL_DRAW_MAX_TRIS];
    u32 vertex_valid_mask;
    u32 vertex_decoded_count;
    u32 triangle_count;
    u32 triangle_valid_count;
    u32 unsupported_opcode;
    u32 unsupported_command_count;
    u32 vertex_range_reject_count;
    u32 color_checksum;
} NDSFighterDLDrawState;

static u8
    sNdsFighterDLAllDrawClean[GMCOMMON_PLAYERS_MAX]
                            [NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];

typedef struct NDSRelocSymbolProbe {
    const void *marker;
    u32 asset_id;
    u32 offset;
    u32 bit;
} NDSRelocSymbolProbe;

typedef struct NDSTitleSpriteDesc {
    const void *symbol;
    u32 offset;
    s16 center_x;
    s16 center_y;
    u16 width;
    u16 height;
    u8 bmfmt;
    u8 bmsiz;
} NDSTitleSpriteDesc;

typedef struct NDSRelocKnownSymbol {
    const void *symbol;
    u32 offset;
} NDSRelocKnownSymbol;

typedef struct NDSRelocKnownAssetSymbol {
    u32 asset_id;
    const void *symbol;
    u32 offset;
} NDSRelocKnownAssetSymbol;

typedef struct NDSOpeningActionPreviewDesc {
    u32 scene_kind;
    u32 asset_id;
    const void *symbol;
    u32 offset;
    u16 width;
    u16 height;
    u16 bitmap_count;
    u8 bmfmt;
    u8 bmsiz;
    s16 x;
    s16 y;
} NDSOpeningActionPreviewDesc;

typedef struct NDSVSResultsSpriteDesc {
    u32 offset;
    u16 width;
    u16 height;
    u16 bitmap_count;
    u8 bmfmt;
    u8 bmsiz;
} NDSVSResultsSpriteDesc;

typedef struct NDSRelocSpriteNormalizeDesc {
    u32 asset_id;
    u32 offset;
    u16 width;
    u16 height;
    u16 bitmap_count;
    u8 bmfmt;
    u8 bmsiz;
} NDSRelocSpriteNormalizeDesc;

typedef struct NDSOpeningActionPreviewCache {
    u32 asset_id;
    u32 offset;
    u32 ready;
    u16 width;
    u16 height;
    u8 bmfmt;
    u8 bmsiz;
    u32 pixel_count;
    u16 pixels[NDS_OPENING_ACTION_PREVIEW_CACHE_PIXELS];
} NDSOpeningActionPreviewCache;

static NDSRelocLoadedFile sNdsRelocLoadedFiles[NDS_RELOC_LOADED_FILE_CAPACITY];
static u32 sNdsRelocLoadedFileCount;
/* Bumped by every change to the table's order, count or `data` pointers; the
 * by-data lookup's one-entry memo is valid only within one epoch. */
static u32 sNdsRelocLoadedFilesEpoch;

/* ZERO-COPY MOTION CACHE HITS. A stream-format clip (BPS1: relative offsets,
 * no fixups) that the raw motion cache holds is handed to the fighter where it
 * lies, the way the resident battlepack hands out its clips, instead of being
 * copied into the fighter's figatree heap and registered there on every status
 * change (~11K ticks a change: the copy, the heap-overwrite scan, the
 * registration and three status-buffer writes; receipt
 * 2026-09-28_p2-2p8-status-change).
 *
 * The cache can give those bytes back (the elastic yield, the ring overwrite,
 * a drop), so each fighter's clip is PINNED: pending from the fetch until the
 * install binds it, then bound until the fighter's next fetch. Before any of
 * those three reuses a pinned range, the pin is RESCUED -- the clip copied into
 * the fighter's own heap, registered there exactly as the copy path registers
 * it, and every pointer into it moved by the same delta: the authoritative
 * force-file record, fp->figatree (Kirby's capture re-binds it) and each
 * joint's script cursor (and TraI descriptor) the pose engine or the generic
 * player holds. After a rescue the fighter is exactly where the copy path
 * would have put it. A fetch drops the fighter's pins first: the install that
 * always follows replaces the binding, and nothing reads the old clip between
 * the two. The bytes the pose engine reads are the cached bytes either way. */
#define NDS_R2_ANIM_PIN_SLOTS 8u
#define NDS_R2_ANIM_PIN_FREE 0u
#define NDS_R2_ANIM_PIN_PENDING 1u
#define NDS_R2_ANIM_PIN_BOUND 2u
typedef struct NDSR2AnimPin
{
    u8 *heap;
    FTStruct *fp;
    GObj *gobj;
    const u8 *payload;
    u32 size;
    u32 asset_id;
    u32 generation;
    u32 state;
    NDSRelocAssetHeader header;
} NDSR2AnimPin;
static NDSR2AnimPin sNdsR2AnimPins[NDS_R2_ANIM_PIN_SLOTS];
__attribute__((used)) volatile u32 gNdsR2AnimZeroCopyHits;
__attribute__((used)) volatile u32 gNdsR2AnimZeroCopyRescues;
__attribute__((used)) volatile u32 gNdsR2AnimZeroCopyDrops;

static const NDSR2AnimPin *ndsR2AnimPinForPayload(const void *payload)
{
    u32 i;

    for (i = 0u; i < NDS_R2_ANIM_PIN_SLOTS; i++)
    {
        const NDSR2AnimPin *pin = &sNdsR2AnimPins[i];

        if ((pin->state != NDS_R2_ANIM_PIN_FREE) &&
            (pin->payload == (const u8 *)payload) &&
            (pin->generation == gNdsTaskmanHeapGeneration))
        {
            return pin;
        }
    }
    return NULL;
}
/* What ndsRelocPrepareFighterAnimHeapOverwrite's full scan saw about the asset
 * about to be registered at that heap: the index of its entry, or -1 for none.
 * The registration that follows at once reads it instead of scanning the table
 * a second time; any change to the table in between bumps the epoch (or the
 * count) and sends it back to the scan. One-shot. */
static struct
{
    u32 asset_id;
    const void *data;
    u32 epoch;
    u32 count;
    s32 index;
    u32 valid;
} sNdsRelocPreparedAnim;
static const void *sNdsRelocRelativeOffsetsMemoBase;
static NDSRelocLoadedFile *sNdsRelocRelativeOffsetsMemo;
static NDSRelocNormalizedMObjSub
    sNdsRelocNormalizedMObjSubs[NDS_RELOC_NORMALIZED_MOBJ_SUB_CAPACITY];
static u32 sNdsRelocNormalizedMObjSubCount;
/* Idempotency record for the wpManagerMakeWeapon seam
 * (ndsRelocEnsureWeaponAttributesNormalized): the s16-run swap is its own
 * inverse, so a second make of the same resident struct must not swap it
 * back. Keyed by (asset id, file offset) plus the load generation, because a
 * same-asset replacement loads fresh swapped bytes under the same id/offset.
 * 64 entries cover the landed population (fighter mains/specials, stage and
 * boss weapons, item weapons) several times over; overflow fails closed
 * through the fixup counter. Reset with the loaded-file table. */
typedef struct NDSRelocNormalizedWeaponAttr
{
    u32 asset_id;
    u32 offset;
    u32 owner_generation;
} NDSRelocNormalizedWeaponAttr;

static NDSRelocNormalizedWeaponAttr
    sNdsRelocNormalizedWeaponAttrs[NDS_RELOC_NORMALIZED_WEAPON_ATTR_CAPACITY];
static u32 sNdsRelocNormalizedWeaponAttrCount;
static u32 sNdsRelocOwnerScene = NDS_RELOC_ASSET_INVALID;
static u32 sNdsRelocSceneGeneration;
/* The taskman-heap generation the resident reloc set was established under.
 * Ownership authority for the scene cache; see ndsRelocPrepareSceneCache. */
static u32 sNdsRelocResidentHeapGeneration;
/* The generation count should be non-zero on any run that re-enters a scene
 * (Sudden Death, rematch); a zero there means this contract never engaged.
 * RangeEvictCount is retained for verifier/backward-compatibility only.  The
 * old cursor/range heuristic was retired after it proved capable of evicting
 * live caller-owned reloc files during recursive dependency loading. */
volatile u32 gNdsRelocSceneReentryGenerationEvictCount;
volatile u32 gNdsRelocSceneReentryRangeEvictCount;
static LBFileNode *sNdsRelocStatusBuffer;
static s32 sNdsRelocStatusBufferCount;
static s32 sNdsRelocStatusBufferMax;
static LBFileNode *sNdsRelocForceStatusBuffer;
static s32 sNdsRelocForceStatusBufferCount;
static s32 sNdsRelocForceStatusBufferMax;

/* BattleShip's scene manager serializes the opening-movie and title scenes.
 * Harness builds cannot enter either scene; their only users of this store are
 * the two exactly bounded static assets in ndsRelocStaticBufferForAsset.
 * Normal startup keeps the full opening extent. One store serves both, sized by
 * the larger. */
#define NDS_RELOC_SCENE_FILE_BUFFER_SIZE \
    ((NDS_TITLE_FILE_BUFFER_SIZE > NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE) \
        ? NDS_TITLE_FILE_BUFFER_SIZE \
        : NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE)

/* P2-3r13. THIS STORE LEFT ARM9 .bss FOR THE SCENE ARENA, AND THAT IS WHAT
 * MAKES FOUR DISTINCT FIGHTER KINDS FIT THE SHIPPING CONFIGURATION.
 *
 * It was a 185,696 B static union -- the title file, the opening-action preview
 * file, and (in harness builds) the Peach's Castle + bank-113 static
 * destinations -- permanently resident in main memory. A VSBattle touches none
 * of them, yet every byte of ARM9 .bss costs the taskman arena one for one:
 * ndsTaskmanArenaBytes steps `calloc` down 4,096 at a time until the libnds
 * heap grants it, so static growth is subtracted from the scene arena silently
 * and only gNdsTaskmanArenaChosenSize ever sees it. The four-distinct-kind
 * roster needed ~186 KB of arena it did not have (board row P2-3r11).
 *
 * The scenes that DO use this store are exactly the scenes with an idle arena.
 * BattleShip's own title/opening scenes load their files with
 * `lbRelocGetExternHeapFile(id, syTaskmanMalloc(...))`, so an arena allocation
 * is what the source does here; the static buffer was the port's deviation.
 * Taking it from the arena is therefore neutral for those scenes, neutral for
 * Castle (ndsRelocEnsureLoadedAsset's fallback would charge the same arena for
 * the same bytes) and returns the whole extent to every battle.
 *
 * Keyed on gNdsTaskmanHeapGeneration for the reason nds_renderer.c's owner-image
 * slot is: the general heap is a bump region that every scene entry rewinds, and
 * a cursor comparison false-positives exactly when a later scene has already
 * re-allocated over the block. Callers must handle NULL -- an arena that cannot
 * afford the store declines instead of spinning in syMallocSet. */
static u8 *sNdsRelocSceneFileBufferPtr;
static u32 sNdsRelocSceneFileBufferGeneration;
__attribute__((used)) volatile u32 gNdsRelocSceneFileBufferAllocCount;
__attribute__((used)) volatile u32 gNdsRelocSceneFileBufferDeclineCount;
__attribute__((used)) volatile u32 gNdsRelocSceneFileBufferBytes =
    NDS_RELOC_SCENE_FILE_BUFFER_SIZE;

NDS_COLD static u8 *ndsRelocSceneFileBuffer(void)
{
    if ((sNdsRelocSceneFileBufferPtr != NULL) &&
        (sNdsRelocSceneFileBufferGeneration == gNdsTaskmanHeapGeneration))
    {
        return sNdsRelocSceneFileBufferPtr;
    }
    sNdsRelocSceneFileBufferPtr = NULL;
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                            NDS_RELOC_SCENE_FILE_BUFFER_SIZE,
                            0x10) == FALSE)
    {
        gNdsRelocSceneFileBufferDeclineCount++;
        return NULL;
    }
    sNdsRelocSceneFileBufferPtr =
        syTaskmanMalloc(NDS_RELOC_SCENE_FILE_BUFFER_SIZE, 0x10);
    sNdsRelocSceneFileBufferGeneration = gNdsTaskmanHeapGeneration;
    if (sNdsRelocSceneFileBufferPtr != NULL)
    {
        gNdsRelocSceneFileBufferAllocCount++;
    }
    return sNdsRelocSceneFileBufferPtr;
}
