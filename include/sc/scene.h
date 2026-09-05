#ifndef SSB64_NDS_SCENE_H
#define SSB64_NDS_SCENE_H

#include <macros.h>
#include <PR/mbi.h>
#include <ssb_types.h>
#include <ft/fighter.h>
#include <sys/dma.h>

#define GMCOMMON_PLAYERS_MAX 4
#define GMCOMMON_FIGHTERS_PLAYABLE_NUM 12
#define SCBATTLE_TIMELIMIT_INFINITE 100
#define SCBATTLE_GAMERULE_TIME 1
#define SCBATTLE_GAMERULE_STOCK 2
#define SCBATTLE_GAMERULE_BONUS 4
#define SCBATTLE_GAMERULE_1PGAME 8
#define LBBACKUP_ERROR_RANDOMKNOCKBACK 1
#define LBBACKUP_ERROR_HALFSTICKRANGE 0x2
#define LBBACKUP_ERROR_1PGAMEMARIO 0x4
#define LBBACKUP_ERROR_VSBATTLECASTLE 0x8

/* decomp include/macros.h:87 verbatim. Port include/macros.h lacks it;
 * sc1pstageclear needs it and reaches this header. Guarded so a later
 * macros.h promotion wins instead of colliding. */
#ifndef NBITS
#define NBITS(t) ((int) (sizeof(t) * 8) )
#endif

/* decomp ft/fttypes.h:717-722 verbatim, port typedef style. sc1pintro
 * needs it and reaches this header via sc/scene.h. */
typedef struct FTDemoDesc
{
    s32 fkind;
    s32 costume;
    s32 shade;
} FTDemoDesc;

typedef struct GObj GObj;
typedef struct SYTaskmanSetup SYTaskmanSetup;

typedef enum SCKind {
    nSCKindNoController,
    nSCKindTitle,
    nSCKindDebugMaps,
    nSCKindDebugCube,
    nSCKindDebugBattle,
    nSCKindDebugFalls,
    nSCKindDebugUnknown,
    nSCKindModeSelect,
    nSCKind1PMode,
    nSCKindVSMode,
    nSCKindVSOptions,
    nSCKindVSItemSwitch,
    nSCKindMessage,
    nSCKind1PChallenger,
    nSCKind1PIntro,
    nSCKindScreenAdjust,
    nSCKindPlayersVS,
    nSCKind1PGamePlayers,
    nSCKindPlayers1PTraining,
    nSCKind1PBonus1Players,
    nSCKind1PBonus2Players,
    nSCKindMaps,
    nSCKindVSBattle,
    nSCKindUnknownMario,
    nSCKindVSResults,
    nSCKindVSRecord,
    nSCKindCharacters,
#if defined(REGION_US)
    nSCKindStartup,
#endif
    nSCKindOpeningRoom,
    nSCKindOpeningPortraits,
    nSCKindOpeningMario,
    nSCKindOpeningDonkey,
    nSCKindOpeningSamus,
    nSCKindOpeningFox,
    nSCKindOpeningLink,
    nSCKindOpeningYoshi,
    nSCKindOpeningPikachu,
    nSCKindOpeningKirby,
    nSCKindOpeningRun,
    nSCKindOpeningYoster,
    nSCKindOpeningCliff,
    nSCKindOpeningStandoff,
    nSCKindOpeningYamabuki,
    nSCKindOpeningClash,
    nSCKindOpeningSector,
    nSCKindOpeningJungle,
    nSCKindOpeningNewcomers,
    nSCKindBackupClear,
    nSCKindEnding,
    nSCKind1PContinue,
    nSCKind1PScoreUnk,
    nSCKind1PStageClear,
    nSCKind1PGame,
    nSCKind1PBonusStage,
    nSCKind1PTrainingMode,
#if defined(REGION_US)
    nSCKindCongra,
#endif
    nSCKindStaffroll,
    nSCKindOption,
    nSCKindData,
    nSCKindSoundTest,
    nSCKindExplain,
    nSCKindAutoDemo
} SCKind;

enum {
    nLBBackupUnlockLuigi,
    nLBBackupUnlockNess,
    nLBBackupUnlockCaptain,
    nLBBackupUnlockPurin,
    nLBBackupUnlockInishie,
    nLBBackupUnlockSoundTest,
    nLBBackupUnlockItemSwitch,
    nLBBackupUnlockEnumCount
};

#define LBBACKUP_UNLOCK_MASK_INISHIE (1u << nLBBackupUnlockInishie)
/* decomp lb/lbdef.h:134-137, :153. The four unlockable fighters; the Poke
 * Ball will not produce Mew until at least one of them has been earned
 * (it/itmain.c:650-656). */
#define LBBACKUP_UNLOCK_MASK_LUIGI (1u << nLBBackupUnlockLuigi)
#define LBBACKUP_UNLOCK_MASK_NESS (1u << nLBBackupUnlockNess)
#define LBBACKUP_UNLOCK_MASK_CAPTAIN (1u << nLBBackupUnlockCaptain)
#define LBBACKUP_UNLOCK_MASK_PURIN (1u << nLBBackupUnlockPurin)
#define LBBACKUP_UNLOCK_MASK_NEWCOMERS (LBBACKUP_UNLOCK_MASK_LUIGI | LBBACKUP_UNLOCK_MASK_PURIN | LBBACKUP_UNLOCK_MASK_CAPTAIN | LBBACKUP_UNLOCK_MASK_NESS)
#define LBBACKUP_UNLOCK_MASK_ITEMSWITCH (1u << nLBBackupUnlockItemSwitch)
#define LBBACKUP_UNLOCK_MASK_SOUNDTEST (1u << nLBBackupUnlockSoundTest)
/* decomp lb/lbdef.h:142-154: every unlock, and the prizes that are not fighters. */
#define LBBACKUP_UNLOCK_MASK_ALL (LBBACKUP_UNLOCK_MASK_ITEMSWITCH | LBBACKUP_UNLOCK_MASK_SOUNDTEST | LBBACKUP_UNLOCK_MASK_INISHIE | LBBACKUP_UNLOCK_MASK_NEWCOMERS)
#define LBBACKUP_UNLOCK_MASK_PRIZE (LBBACKUP_UNLOCK_MASK_ALL & ~LBBACKUP_UNLOCK_MASK_NEWCOMERS)
#define LBBACKUP_MASK_STAGE(kind) (1u << (kind))
#define LBBACKUP_GROUND_MASK_ALL \
    (LBBACKUP_MASK_STAGE(nGRKindCastle) | \
     LBBACKUP_MASK_STAGE(nGRKindSector) | \
     LBBACKUP_MASK_STAGE(nGRKindJungle) | \
     LBBACKUP_MASK_STAGE(nGRKindZebes) | \
     LBBACKUP_MASK_STAGE(nGRKindHyrule) | \
     LBBACKUP_MASK_STAGE(nGRKindYoster) | \
     LBBACKUP_MASK_STAGE(nGRKindPupupu) | \
     LBBACKUP_MASK_STAGE(nGRKindYamabuki))

typedef enum SCBattleGameRules {
    nSCBattleGameRuleTime,
    nSCBattleGameRuleStock,
    nSCBattleGameRuleBonus,
    nSCBattleGameRule1PGame
} SCBattleGameRules;

enum {
    nSCBattleGameStatusWait,
    nSCBattleGameStatusGo,
    nSCBattleGameStatusPause,
    nSCBattleGameStatusUnpause,
    nSCBattleGameStatusEnd = 5,
    nSCBattleGameStatusBossDefeat,
    nSCBattleGameStatusSet
};
enum { nSCBattleHandicapOff, nSCBattleHandicapOn, nSCBattleHandicapAuto };
/* decomp sc/scdef.h:245-254. The port stopped at Middle while nothing could
 * select a rate, but dITManagerAppearanceRatesMin/Max already carry all six
 * entries (it/itmanager.c:19-38) -- so High and VeryHigh were reachable at
 * runtime and merely unnamed, and a menu walking the rate would have stopped
 * two short of the source's range. */
enum { nSCBattleItemSwitchNone, nSCBattleItemSwitchVeryLow,
       nSCBattleItemSwitchLow, nSCBattleItemSwitchMiddle,
       nSCBattleItemSwitchHigh, nSCBattleItemSwitchVeryHigh,
       nSCBattleItemSwitchEnumCount };
enum {
    nSCBattleGameTypeDemo,
    nSCBattleGameTypeRoyal,
    nSCBattleGameTypeBonus,
    nSCBattleGameTypeExplain,
    nSCBattleGameTypeMovie,
    nSCBattleGameType1PGame,
    nSCBattleGameTypeUnk6,
    nSCBattleGameTypeTraining
};
/* P2-6. WIDENED to the source's five, from two. `SC1PGameComputer` below
 * sizes four arrays by `nSC1PGameDifficultyEnumCount`, so a truncated copy
 * would not merely be incomplete -- it would lay those arrays out at 2
 * entries where the transcribed ladder writes 5, and the overflow would be
 * silent. Source: decomp sc/scdef.h:280-289. Nothing read the two-value
 * form, so widening it changes no existing value or caller. */
enum {
    nSC1PGameDifficultyVeryEasy,
    nSC1PGameDifficultyEasy,
    nSC1PGameDifficultyNormal,
    nSC1PGameDifficultyHard,
    nSC1PGameDifficultyVeryHard,
    nSC1PGameDifficultyEnumCount
};

/* P2-6 ladder position. `gSCManagerSceneData.spgame_stage` indexes both
 * dSC1PGameStageDesc[18] and dSC1PGameComputerDesc[18] directly
 * (sc1pgame.c:979-980), and sc1PManagerUpdateScene's loop bound is
 * `spgame_stage <= nSC1PGameStageCommonEnd` (sc1pmanager.c:318), so the
 * ENUMERATOR VALUES ARE TABLE INDICES and their order is load-bearing.
 * Verbatim from decomp sc/scdef.h:291-316, aliases included: the four
 * challenger rows continue the same numbering, which is why the two tables
 * carry eighteen rows rather than fourteen. */
enum {
    nSC1PGameStageCommonStart,
    nSC1PGameStageLink = nSC1PGameStageCommonStart, /* VS Link */
    nSC1PGameStageYoshi,                            /* VS Yoshi Team */
    nSC1PGameStageFox,                              /* VS Fox */
    nSC1PGameStageBonus1,                           /* Break the Targets */
    nSC1PGameStageMario,                            /* VS Mario Bros. */
    nSC1PGameStagePikachu,                          /* VS Pikachu */
    nSC1PGameStageDonkey,                           /* VS Giant Donkey Kong */
    nSC1PGameStageBonus2,                           /* Board the Platforms */
    nSC1PGameStageKirby,                            /* VS Kirby Team */
    nSC1PGameStageSamus,                            /* VS Samus */
    nSC1PGameStageMMario,                           /* VS Metal Mario */
    nSC1PGameStageBonus3,                           /* Race to the Finish */
    nSC1PGameStageZako,                             /* VS Fighting Polygon Team */
    nSC1PGameStageBoss,                             /* VS Master Hand */
    nSC1PGameStageCommonEnd = nSC1PGameStageBoss,

    nSC1PGameStageChallengerStart,
    nSC1PGameStageLuigi = nSC1PGameStageChallengerStart,
    nSC1PGameStageNess,
    nSC1PGameStagePurin,
    nSC1PGameStageCaptain,
    nSC1PGameStageChallengerEnd = nSC1PGameStageCaptain
};

/* P2-6 bonus tally. The 58 bonuses plus their count, verbatim in the
 * source's order (decomp sc/scdef.h:319-379); the order IS the table index,
 * so it must not be sorted or deduplicated. */
enum {
    nSC1PGameBonusCheapShot,
    nSC1PGameBonusStarFinish,
    nSC1PGameBonusNoItem,
    nSC1PGameBonusShieldBreaker,
    nSC1PGameBonusJudoWarrior,
    nSC1PGameBonusHawk,
    nSC1PGameBonusShooter,
    nSC1PGameBonusHeavyDamage,
    nSC1PGameBonusAllVariations,
    nSC1PGameBonusItemStrike,
    nSC1PGameBonusDoubleKO,
    nSC1PGameBonusTrickster,
    nSC1PGameBonusGiantImpact,
    nSC1PGameBonusSpeedster,
    nSC1PGameBonusItemThrow,
    nSC1PGameBonusTripleKO,
    nSC1PGameBonusLastChance,
    nSC1PGameBonusPacifist,
    nSC1PGameBonusPerfect,
    nSC1PGameBonusNoMiss,
    nSC1PGameBonusNoDamage,
    nSC1PGameBonusFullPower,
    nSC1PGameBonusStageClear,
    nSC1PGameBonusNoMissClear,
    nSC1PGameBonusNoDamageClear,
    nSC1PGameBonusSpeedKing,
    nSC1PGameBonusSpeedDemon,
    nSC1PGameBonusMewCatcher,
    nSC1PGameBonusStarClear,
    nSC1PGameBonusVegetarian,
    nSC1PGameBonusHeartThrob,
    nSC1PGameBonusThrowDown,
    nSC1PGameBonusSmashMania,
    nSC1PGameBonusSmashless,
    nSC1PGameBonusSpecialMove,
    nSC1PGameBonusSingleMove,
    nSC1PGameBonusPokemonFinish,
    nSC1PGameBonusBoobyTrap,
    nSC1PGameBonusFighterStance,
    nSC1PGameBonusMystic,
    nSC1PGameBonusCometMystic,
    nSC1PGameBonusAcidClear,
    nSC1PGameBonusBumperClear,
    nSC1PGameBonusTornadoClear,
    nSC1PGameBonusArwingClear,
    nSC1PGameBonusCounterAttack,
    nSC1PGameBonusMeteorSmash,
    nSC1PGameBonusAerial,
    nSC1PGameBonusLastSecond,
    nSC1PGameBonusLucky3,
    nSC1PGameBonusJackpot,
    nSC1PGameBonusYoshiRainbow,
    nSC1PGameBonusKirbyRanks,
    nSC1PGameBonusBrosCalamity,
    nSC1PGameBonusDKDefender,
    nSC1PGameBonusDKPerfect,
    nSC1PGameBonusGoodFriend,
    nSC1PGameBonusTrueFriend,
    nSC1PGameBonusEnumCount
};

/* P2-6 campaign descriptors, field for field from decomp sc/sctypes.h:11-31
 * and :111-115. Declared here beside the enums they index rather than in a
 * new header, because include/ precedes the decomp root and a narrow port
 * header named after a decomp one starves the decomp TUs that need the
 * full one. */
/* P2-6 team sizes, verbatim from decomp sc/sc1pmode/sc1pgame.h:9-20. The
 * ladder's Yoshi and Kirby team rows and the Fighting Polygon Team row index
 * by these, so they belong beside the descriptors rather than in the table. */
#define SC1PGAME_STAGE_MAX_TEAM_COUNT 30
#define SC1PGAME_STAGE_MAX_VARIATIONS_COUNT 12
#define SC1PGAME_STAGE_YOSHI_VARIATIONS_COUNT 6
#define SC1PGAME_STAGE_YOSHI_TEAM_COUNT 18
#define SC1PGAME_STAGE_KIRBY_VARIATIONS_COUNT 7
#define SC1PGAME_STAGE_KIRBY_TEAM_COUNT 8
#define SC1PGAME_STAGE_KIRBY_SIM_COUNT 2
#define SC1PGAME_STAGE_MAX_OPPONENT_COUNT 3

typedef struct SC1PGameComputer {
    ub8 is_team_attack;
    u8 item_appearance_rate;
    u8 enemy_level[nSC1PGameDifficultyEnumCount];
    u8 enemy_handicap[nSC1PGameDifficultyEnumCount];
    u8 ally_level[nSC1PGameDifficultyEnumCount];
    u8 ally_handicap[nSC1PGameDifficultyEnumCount];
} SC1PGameComputer;

typedef struct SC1PGameStage {
    u8 screenflash_alpha;
    u8 gkind;
    u32 item_toggles;
    u8 opponent_count;
    u8 fkind[2];
    u8 opponent_behavior;
    u8 ally_count;
    u8 ally_behavior;
} SC1PGameStage;

/* decomp sc/sctypes.h:56-103 verbatim, port typedef style. */
typedef struct SC1PGameBossPlan
{
    s32 unk_sc1pbossplan_0x0;
    u8 dl_link;
    u32 camera_tag;
    Vec3f pos;
} SC1PGameBossPlan;

typedef struct SC1PGameBossAnim
{
    intptr_t o_anim_joint;
    intptr_t o_matanim_joint;
    f32 anim_speed;
} SC1PGameBossAnim;

typedef struct SC1PGameBossEffect
{
    void (*proc_update)(GObj *);
    void (*proc_display)(GObj *);
    intptr_t o_dobjdesc;
    intptr_t o_mobjsub;
} SC1PGameBossEffect;

typedef struct SC1PGameBossWallpaper
{
    s32 loop_count;
    s32 effect_count;
    s32 anim_count;
    s32 plan_count;
    s32 dobj_color_id;
    s32 color_id;
    s32 change_wait_base;
    s32 change_damage_min;
    sb32 is_random_wallpaper;
    SC1PGameBossEffect *bosseffect;
    SC1PGameBossAnim *bossanim;
    SC1PGameBossPlan *bossplan;
} SC1PGameBossWallpaper;

typedef struct SC1PGameBossMain
{
    sb32 is_skip_wallpaper_change;
    s32 wallpaper_id;
    s32 change_wait;
    void *file_head;
    SC1PGameBossWallpaper *bosswallpaper;
    s32 bossplayer;
} SC1PGameBossMain;

/* decomp sc/scdef.h:391-397 verbatim. */
typedef enum SC1PStageClearKind
{
    nSC1PStageClearKindStage,
    nSC1PStageClearKindGame,
    nSC1PStageClearKindResult
} SC1PStageClearKind;

/* decomp sc/sctypes.h:105-109 verbatim, port typedef style. */
typedef struct SC1PStageClearStats
{
    s32 bonus_array_id;
    s32 bonus_id;
} SC1PStageClearStats;

typedef struct SC1PStageClearScore {
    intptr_t offset;
    s32 points;
} SC1PStageClearScore;

/* decomp sc/scdef.h:399-417 verbatim. */
typedef enum SC1PTrainingModeMain
{
    nSC1PTrainingModeMenuMainEnumStart,
    nSC1PTrainingModeMenuMainCP = nSC1PTrainingModeMenuMainEnumStart,
    nSC1PTrainingModeMenuMainScrollStart = nSC1PTrainingModeMenuMainCP,
    nSC1PTrainingModeMenuMainItem,
    nSC1PTrainingModeMenuMainSpeed,
    nSC1PTrainingModeMenuMainView,
    nSC1PTrainingModeMenuMainScrollEnd = nSC1PTrainingModeMenuMainView,
    nSC1PTrainingModeMenuMainReset,
    nSC1PTrainingModeMenuMainExit,
    nSC1PTrainingModeMenuEnumEnd = nSC1PTrainingModeMenuMainExit,
    nSC1PTrainingModeMenuMainEnumCount
} SC1PTrainingModeMain;

/* decomp sc/scdef.h:419-429 verbatim. */
typedef enum SC1PTrainingModeCP
{
    nSC1PTrainingModeMenuCPEnumStart,
    nSC1PTrainingModeMenuCPStand = nSC1PTrainingModeMenuCPEnumStart,
    nSC1PTrainingModeMenuCPWalk,
    nSC1PTrainingModeMenuCPEvade,
    nSC1PTrainingModeMenuCPJump,
    nSC1PTrainingModeMenuCPAttack,
    nSC1PTrainingModeMenuCPEnumCount
} SC1PTrainingModeCP;

/* decomp sc/scdef.h:431-453 verbatim. */
typedef enum SC1PTrainingModeItem
{
    nSC1PTrainingModeMenuItemEnumStart,
    nSC1PTrainingModeMenuItemNone = nSC1PTrainingModeMenuItemEnumStart,
    nSC1PTrainingModeMenuItemMaximTomato,
    nSC1PTrainingModeMenuItemHeart,
    nSC1PTrainingModeMenuItemStar,
    nSC1PTrainingModeMenuItemBeamSword,
    nSC1PTrainingModeMenuItemHomeRunBat,
    nSC1PTrainingModeMenuItemFan,
    nSC1PTrainingModeMenuItemStarRod,
    nSC1PTrainingModeMenuItemRayGun,
    nSC1PTrainingModeMenuItemFireFlower,
    nSC1PTrainingModeMenuItemHammer,
    nSC1PTrainingModeMenuItemMotionSensorBomb,
    nSC1PTrainingModeMenuItemBobomb,
    nSC1PTrainingModeMenuItemBumper,
    nSC1PTrainingModeMenuItemGreenShell,
    nSC1PTrainingModeMenuItemRedShell,
    nSC1PTrainingModeMenuItemPokeBall,
    nSC1PTrainingModeMenuItemEnumCount
} SC1PTrainingModeItem;

/* decomp sc/scdef.h:455-464 verbatim. */
typedef enum SC1PTrainingModeSpeed
{
    nSC1PTrainingModeMenuSpeedEnumStart,
    nSC1PTrainingModeMenuSpeedFull = nSC1PTrainingModeMenuSpeedEnumStart,
    nSC1PTrainingModeMenuSpeed2Thirds,
    nSC1PTrainingModeMenuSpeedHalf,
    nSC1PTrainingModeMenuSpeedQuarter,
    nSC1PTrainingModeMenuSpeedEnumCount
} SC1PTrainingModeSpeed;

/* decomp sc/scdef.h:466-473 verbatim. */
typedef enum SC1PTrainingModeView
{
    nSC1PTrainingModeMenuViewEnumStart,
    nSC1PTrainingModeMenuViewCloseUp = nSC1PTrainingModeMenuViewEnumStart,
    nSC1PTrainingModeMenuViewNormal,
    nSC1PTrainingModeMenuViewEnumCount
} SC1PTrainingModeView;

/* decomp sc/scdef.h:475-524 verbatim. */
typedef enum SC1PTrainingModeMenuOptionSprites
{
    nSC1PTrainingModeMenuOptionSpriteItemStart,
    nSC1PTrainingModeMenuOptionSpriteItemNone = nSC1PTrainingModeMenuOptionSpriteItemStart,
    nSC1PTrainingModeMenuOptionSpriteItemMaximTomato,
    nSC1PTrainingModeMenuOptionSpriteItemHeart,
    nSC1PTrainingModeMenuOptionSpriteItemStar,
    nSC1PTrainingModeMenuOptionSpriteItemBeamSword,
    nSC1PTrainingModeMenuOptionSpriteItemHomeRunBat,
    nSC1PTrainingModeMenuOptionSpriteItemFan,
    nSC1PTrainingModeMenuOptionSpriteItemStarRod,
    nSC1PTrainingModeMenuOptionSpriteItemRayGun,
    nSC1PTrainingModeMenuOptionSpriteItemFireFlower,
    nSC1PTrainingModeMenuOptionSpriteItemHammer,
    nSC1PTrainingModeMenuOptionSpriteItemMotionSensorBomb,
    nSC1PTrainingModeMenuOptionSpriteItemBobomb,
    nSC1PTrainingModeMenuOptionSpriteItemBumper,
    nSC1PTrainingModeMenuOptionSpriteItemGreenShell,
    nSC1PTrainingModeMenuOptionSpriteItemRedShell,
    nSC1PTrainingModeMenuOptionSpriteItemPokeBall,
    nSC1PTrainingModeMenuOptionSpriteItemEnd = nSC1PTrainingModeMenuOptionSpriteItemPokeBall,

    nSC1PTrainingModeMenuOptionSpriteSpeedStart,
    nSC1PTrainingModeMenuOptionSpriteSpeedFull = nSC1PTrainingModeMenuOptionSpriteSpeedStart,
    nSC1PTrainingModeMenuOptionSpriteSpeed2Thirds,
    nSC1PTrainingModeMenuOptionSpriteSpeedHalf,
    nSC1PTrainingModeMenuOptionSpriteSpeedQuarter,
    nSC1PTrainingModeMenuOptionSpriteSpeedEnd = nSC1PTrainingModeMenuOptionSpriteSpeedQuarter,

    nSC1PTrainingModeMenuOptionSpriteCPStart,
    nSC1PTrainingModeMenuOptionSpriteCPStand = nSC1PTrainingModeMenuOptionSpriteCPStart,
    nSC1PTrainingModeMenuOptionSpriteCPWalk,
    nSC1PTrainingModeMenuOptionSpriteCPEvade,
    nSC1PTrainingModeMenuOptionSpriteCPJump,
    nSC1PTrainingModeMenuOptionSpriteCPAttack,
    nSC1PTrainingModeMenuOptionSpriteCPEnd = nSC1PTrainingModeMenuOptionSpriteCPAttack,

    nSC1PTrainingModeMenuOptionSpriteViewStart,
    nSC1PTrainingModeMenuOptionSpriteViewNormal = nSC1PTrainingModeMenuOptionSpriteViewStart,
    nSC1PTrainingModeMenuOptionSpriteViewCloseUp,
    nSC1PTrainingModeMenuOptionSpriteViewEnd = nSC1PTrainingModeMenuOptionSpriteViewCloseUp,

    nSC1PTrainingModeMenuOptionSpriteIndicatorStart,
    nSC1PTrainingModeMenuOptionSpriteLeftArrow = nSC1PTrainingModeMenuOptionSpriteIndicatorStart,
    nSC1PTrainingModeMenuOptionSpriteRightArrow,
    nSC1PTrainingModeMenuOptionSpriteCursor,

    nSC1PTrainingModeMenuOptionSpriteEnumCount
} SC1PTrainingModeMenuOptionSprites;

/* decomp sc/sctypes.h:117-183 verbatim, port typedef style. */
typedef struct SC1PTrainingModeSprites
{
    Vec2h pos;
    Sprite *sprite;
} SC1PTrainingModeSprites;

typedef struct SC1PTrainingModeFiles
{
    s32 file_id;
    intptr_t offset;
    SYColorRGB fog_color;
} SC1PTrainingModeFiles;

typedef struct SC1PTrainingModeMenu
{
    s32 main_menu_option;
    s32 damage;
    s32 combo;
    s32 item_hold;
    s32 item_menu_option;
    s32 cp_menu_option;
    s32 speed_menu_option;
    s32 view_menu_option;
    s32 dummy;
    SC1PTrainingModeSprites *display_label_sprites;
    Sprite **display_option_sprites;
    SC1PTrainingModeSprites *menu_label_sprites;
    Sprite **menu_option_sprites;
    SC1PTrainingModeSprites *unk_trainmenu_0x34;
    SC1PTrainingModeSprites *unk_trainmenu_0x38;
    GObj *damage_display_gobj;
    GObj *combo_display_gobj;
    GObj *cp_display_gobj;
    GObj *speed_display_gobj;
    GObj *item_display_gobj;
    GObj *menu_label_gobj;
    GObj *cursor_gobj;
    GObj *cp_option_gobj;
    GObj *item_option_gobj;
    GObj *speed_option_gobj;
    GObj *view_option_gobj;
    GObj *arrow_option_gobj;
    SObj *hscroll_option_sobj[4];
    GObj *unk_trainmenu_0x7C;
    GObj *combo0;
    SObj *vscroll_option_sobj[6][2];
    u32 cursor_ulx, cursor_uly;
    u32 cursor_lrx, cursor_lry;
    u16 button_hold;
    u16 button_tap;
    u16 button_queue;
    s32 rapid_scroll_wait;
    u8 damage_reset_wait;
    u8 combo_reset_wait;
    ub8 exit_or_reset;
    u8 lagtic_wait;
    u8 frameadvance_wait;
    u8 item_spawn_wait;
    u16 magnify_wait;
    ub8 is_read_menu_inputs;
    s32 unknown[2];
} SC1PTrainingModeMenu;

/* decomp sc/scdef.h:526-541 and sc/sctypes.h (the SCStaffroll* structs),
 * verbatim, for the imported credits scene (battleship_scstaffroll.c,
 * 2026-09-05). */
typedef enum SCStaffrollCompany
{
    nSCStaffrollCompanyNull = -1,
    nSCStaffrollCompanyHAL,
    nSCStaffrollCompanyNINTENDO,
    nSCStaffrollCompanyCreatures,
    nSCStaffrollCompanyGAMEFREAK,
    nSCStaffrollCompanyRare,
    nSCStaffrollCompanyMickeys,
    nSCStaffrollCompanyKENProd,
    nSCStaffrollCompanyAONIProd,
    nSCStaffrollCompanyARTSVISION,
    nSCStaffrollCompanyEZAKIProd,
    nSCStaffrollCompanyNOA
} SCStaffrollCompany;

typedef struct SCStaffrollMatrix
{
    u8 filler_0x0[0xC];
    f32 unk_gmcreditsmtx_0xC;
    f32 unk_gmcreditsmtx_0x10;
    f32 unk_gmcreditsmtx_0x14;
} SCStaffrollMatrix;

typedef struct SCStaffrollText
{
    s32 character_start;        // Where to begin reading text from in main character array
    s32 character_count;        // Number of characters in credits role card to display
} SCStaffrollText;

typedef struct SCStaffrollSprite
{
    u8 width;
    u8 height;
    intptr_t offset;
} SCStaffrollSprite;

typedef struct SCStaffrollStaff
{
    u8 filler_0x0[0x4];
    s32 staff_id;
} SCStaffrollStaff;

typedef struct SCStaffrollName SCStaffrollName;
struct SCStaffrollName
{
    SCStaffrollName *next;
    s32 name_id;
    sb32 job_or_name;   // 0 = job (e.g. Director), 1 = name (e.g. Masahiro Sakurai)
    f32 offset_x;
    f32 unkgmcreditsstruct0x10;
    f32 interpolation;
    s32 status;
    s32 unkgmcreditsstruct0x1C;
};

typedef struct SCStaffrollJob
{
    s32 prefix_id;    // e.g. "Chief" -> Chief Programmers
    s32 job_id;       // Job text to use
    s32 staff_count;  // Number of staff members to roll until new job is shown
} SCStaffrollJob;

typedef struct SCStaffrollSetup
{
    f32 unk_gmcreditsunk_0x0;
    DObj *dobj;
    f32 spacing;
    f32 unk_gmcreditsunk_0xC;
    f32 unk_gmcreditsunk_0x10;
} SCStaffrollSetup;

typedef struct SCStaffrollProjection
{
    Vec3f pv0;
    Vec3f pv1;
    Vec3f pv2;
    Vec3f pv3;
    f32 px0;
    f32 py0;
    f32 px1;
    f32 py1;
    f32 px2;
    f32 py2;
    f32 px3;
    f32 py3;
} SCStaffrollProjection;

/* decomp sc/sctypes.h:256-296 verbatim, port typedef style. */
typedef struct SCExplainMain
{
    SObj *textbox_sobj;
    GObj *stick_gobj;
    GObj *spark_gobj;
    GObj *rgb_gobj;
    SObj *phase_sobj0;
    SObj *phase_sobj1;
    SObj *phase_sobj2;
    SObj *phase_sobj3;
    SObj *phase_sobj4;
    SObj *phase_sobj5;
    s32 phase_advance_wait;
    s32 phase;
    u8 unk_scexplainif_0x30;
    u8 stick_status;
} SCExplainMain;

typedef struct SCExplainArgs
{
    u16 sprite_pos_x;
    u8 sprite_pos_y;
    u8 sprite_status;
} SCExplainArgs;

typedef struct SCExplainPhase
{
    u16 phase_time;
    u16 unused;
    u8 textbox_pos_x;
    u8 textbox_pos_y;
    Sprite *sprite;
    SCExplainArgs control_stick_args;
    SCExplainArgs phase_args0;
    SCExplainArgs phase_args1;
    SCExplainArgs phase_args2;
    SCExplainArgs phase_args3;
    SCExplainArgs phase_args4;
    SCExplainArgs rgb_overlay_args;
    SCExplainArgs phase_args5;
} SCExplainPhase;

/* decomp sc/sctypes.h:298-303 verbatim, port typedef style. */
typedef struct SCAutoDemoProc
{
    u16 focus_end_wait;
    void (*func_change)(void);
    void (*func_focus)(void);
} SCAutoDemoProc;
enum {
    nGRKindCastle,
    nGRKindSector,
    nGRKindJungle,
    nGRKindZebes,
    nGRKindHyrule,
    nGRKindYoster,
    nGRKindPupupu,
    nGRKindYamabuki,
    nGRKindStarterEnd = nGRKindYamabuki,
    nGRKindInishie,
    nGRKindUnlockEnd = nGRKindInishie,
    nGRKindBattleEnd = nGRKindInishie,
    nGRKindPupupuSmall,
    nGRKindPupupuNew,
    nGRKindExplain,
    nGRKindYosterSmall,
    nGRKindMetal,
    nGRKindZako,
    nGRKindBonus3,
    nGRKindLast,
    nGRKindCommonEnd = nGRKindLast,
    nGRKindBonusStageStart,
    nGRKindBonus1Start = nGRKindBonusStageStart,
    nGRKindBonus1Mario = nGRKindBonus1Start,
    nGRKindBonus1Fox,
    nGRKindBonus1Donkey,
    nGRKindBonus1Samus,
    nGRKindBonus1Luigi,
    nGRKindBonus1Link,
    nGRKindBonus1Yoshi,
    nGRKindBonus1Captain,
    nGRKindBonus1Kirby,
    nGRKindBonus1Pikachu,
    nGRKindBonus1Purin,
    nGRKindBonus1Ness,
    nGRKindBonus1End = nGRKindBonus1Ness,
    nGRKindBonus2Start,
    nGRKindBonus2Mario = nGRKindBonus2Start,
    nGRKindBonus2Fox,
    nGRKindBonus2Donkey,
    nGRKindBonus2Samus,
    nGRKindBonus2Luigi,
    nGRKindBonus2Link,
    nGRKindBonus2Yoshi,
    nGRKindBonus2Captain,
    nGRKindBonus2Kirby,
    nGRKindBonus2Pikachu,
    nGRKindBonus2Purin,
    nGRKindBonus2Ness,
    nGRKindBonus2End = nGRKindBonus2Ness,
    nGRKindBonusStageEnd = nGRKindBonus2End
};

enum {
    nSCBattleTeamIDBattleStart,
    nSCBattleTeamIDRed = nSCBattleTeamIDBattleStart,
    nSCBattleTeamIDBlue,
    nSCBattleTeamIDGreen,
    nSCBattleTeamIDBattleEnd = nSCBattleTeamIDGreen
};

typedef struct LBBackupVSRecord {
    u16 ko_count[GMCOMMON_FIGHTERS_PLAYABLE_NUM];
    u32 time_used;
    u32 damage_given;
    u32 damage_taken;
    u16 unk;
    u16 selfdestructs;
    u16 games_played;
    u16 player_count_tally;
    u16 player_count_tallies[GMCOMMON_FIGHTERS_PLAYABLE_NUM];
    u16 played_against[GMCOMMON_FIGHTERS_PLAYABLE_NUM];
} LBBackupVSRecord;

typedef struct LBBackup1PRecord {
    u32 spgame_hiscore;
    u32 spgame_continues;
    u32 spgame_total_bonuses;
    u8 spgame_best_difficulty;
    u32 bonus1_time;
    u8 bonus1_task_count;
    u32 bonus2_time;
    u8 bonus2_task_count;
    ub8 is_spgame_complete;
} LBBackup1PRecord;

typedef struct LBBackupData {
    LBBackupVSRecord vs_records[GMCOMMON_FIGHTERS_PLAYABLE_NUM];
    ub8 is_allow_screenflash;
    ub8 sound_mono_or_stereo;
    s16 screen_adjust_h;
    s16 screen_adjust_v;
    u8 characters_fkind;
    u8 unlock_mask;
    u16 fighter_mask;
    u8 spgame_difficulty;
    u8 spgame_stock_count;
    LBBackup1PRecord spgame_records[GMCOMMON_FIGHTERS_PLAYABLE_NUM];
    u16 ground_mask;
    u8 vs_itemswitch_battles;
    u16 vs_total_battles;
    u8 error_flags;
    u8 boot;
    u16 signature;
    s32 checksum;
} LBBackupData;

typedef struct SCPlayerData {
    u8 level, handicap, pkind, fkind, team, player, costume, shade, color;
    ub8 is_single_stockicon;
    u8 tag;
    s8 stock_count;
    ub8 is_spgame_enemy;
    u8 place;
    s32 falls, score;
    s32 total_kos_players[GMCOMMON_PLAYERS_MAX];
    s32 unk_pblock_0x28, unk_pblock_0x2C;
    s32 total_selfdestructs, total_damage_given, total_damage_all;
    s32 total_damage_players[GMCOMMON_PLAYERS_MAX];
    s32 stock_damage_all, combo_damage_foe, combo_count_foe;
    GObj *fighter_gobj;
    u32 stale_id;
    struct { u16 attack_id, motion_count; } stale_info[5];
} SCPlayerData;

typedef struct SCBattleResults {
    s32 tko;
    s32 kos;
    u8 player_or_team;
    u8 unk_battleres_0x9;
    ub8 is_human;
} SCBattleResults;

typedef struct SCBattleState {
    u8 game_type, gkind;
    ub8 is_team_battle;
    u8 game_rules, pl_count, cp_count, time_limit, stocks, handicap;
    ub8 is_team_attack, is_stage_select;
    u8 damage_ratio;
    u32 item_toggles;
    ub8 is_reset_players;
    u8 game_status;
    u32 time_remain, time_passed;
    u8 item_appearance_rate;
    ub32 is_show_score : 1;
    ub32 is_not_teamshadows : 1;
    SCPlayerData players[GMCOMMON_PLAYERS_MAX];
} SCBattleState;

typedef struct SCCommonData {
    u8 scene_curr, scene_prev;
    u8 unlock_messages[nLBBackupUnlockEnumCount];
    u8 challenger_fkind;
    u16 demo_mask_prev;
    u8 demo_first_fkind, demo_fkind[2], gkind;
    ub8 is_suddendeath, is_continue, is_reset;
    u8 player, fkind, costume, spgame_time_limit, spgame_stage;
    u8 ally_players[2];
    u32 spgame_time_remain, spgame_score, continues_used, bonus_count;
    u32 bonus_get_mask[3];
    u8 bonus_tasks_complete, bonus_fkind, bonus_costume;
    u8 training_man_fkind, training_man_costume;
    u8 training_com_fkind, training_com_costume;
    ub8 is_extend_demo_wait;
    u8 demo_gkind_order, maps_vsmode_gkind, maps_training_gkind;
    u8 challenger_level_drop;
    ub8 is_title_anim_viewed;
} SCCommonData;

#define DECLARE_OVL(n) \
    extern uintptr_t ovl##n##_ROM_START, ovl##n##_ROM_END, ovl##n##_VRAM; \
    extern uintptr_t ovl##n##_TEXT_START, ovl##n##_TEXT_END; \
    extern uintptr_t ovl##n##_DATA_START, ovl##n##_RODATA_END; \
    extern uintptr_t ovl##n##_BSS_START, ovl##n##_BSS_END

DECLARE_OVL(0); DECLARE_OVL(1); DECLARE_OVL(2); DECLARE_OVL(3);
DECLARE_OVL(4); DECLARE_OVL(5); DECLARE_OVL(6); DECLARE_OVL(7);
DECLARE_OVL(8); DECLARE_OVL(9); DECLARE_OVL(10); DECLARE_OVL(11);
DECLARE_OVL(12); DECLARE_OVL(13); DECLARE_OVL(14); DECLARE_OVL(15);
DECLARE_OVL(16); DECLARE_OVL(17); DECLARE_OVL(18); DECLARE_OVL(19);
DECLARE_OVL(20); DECLARE_OVL(21); DECLARE_OVL(22); DECLARE_OVL(23);
DECLARE_OVL(24); DECLARE_OVL(25); DECLARE_OVL(26); DECLARE_OVL(27);
DECLARE_OVL(28); DECLARE_OVL(29); DECLARE_OVL(30); DECLARE_OVL(31);
DECLARE_OVL(32); DECLARE_OVL(33); DECLARE_OVL(34); DECLARE_OVL(35);
DECLARE_OVL(36); DECLARE_OVL(37); DECLARE_OVL(38); DECLARE_OVL(39);
DECLARE_OVL(40); DECLARE_OVL(41); DECLARE_OVL(42); DECLARE_OVL(43);
DECLARE_OVL(44); DECLARE_OVL(45); DECLARE_OVL(46); DECLARE_OVL(47);
DECLARE_OVL(48); DECLARE_OVL(49); DECLARE_OVL(50); DECLARE_OVL(51);
DECLARE_OVL(52); DECLARE_OVL(53); DECLARE_OVL(54); DECLARE_OVL(55);
DECLARE_OVL(56); DECLARE_OVL(57); DECLARE_OVL(58); DECLARE_OVL(59);
DECLARE_OVL(60); DECLARE_OVL(61); DECLARE_OVL(62); DECLARE_OVL(63);
DECLARE_OVL(64);

#define SCMANAGER_OVERLAY_DEFINE(n) { \
    (uintptr_t)&ovl##n##_ROM_START, (uintptr_t)&ovl##n##_ROM_END, \
    (uintptr_t)&ovl##n##_VRAM, (uintptr_t)&ovl##n##_TEXT_START, \
    (uintptr_t)&ovl##n##_TEXT_END, (uintptr_t)&ovl##n##_DATA_START, \
    (uintptr_t)&ovl##n##_RODATA_END, (uintptr_t)&ovl##n##_BSS_START, \
    (uintptr_t)&ovl##n##_BSS_END }

extern SCCommonData gSCManagerSceneData, dSCManagerDefaultSceneData;
extern SCBattleState dSCManagerDefaultBattleState;
extern SCBattleState gSCManager1PGameBattleState;
extern SCBattleState gSCManagerTransferBattleState;
extern SCBattleState gSCManagerVSBattleState;
extern SCBattleState *gSCManagerBattleState;
extern LBBackupData gSCManagerBackupData, dSCManagerDefaultBackupData;
/* decomp sc/sc1pmode/sc1pgame.c:726. Set when the player catches Mew out of
 * a Poke Ball; the 1P bonus screen reads it. */
extern ub8 gSC1PGameBonusMewCatcher;

void scManagerRunLoop(sb32 arg);
void scManagerFuncPrint(void);
void scManagerFuncUpdate(SYTaskmanSetup *arg);
void scManagerFuncDraw(void);
void scManagerRunPrintGObjStatus(void);
void sc1PManagerUpdateScene(void);
void sc1PBonusStageStartScene(void);
void sc1PChallengerStartScene(void);
void sc1PIntroStartScene(void);
void sc1PStageClearStartScene(void);
void sc1PTrainingModeStartScene(void);
void scAutoDemoStartScene(void);
void scExplainStartScene(void);
void scStaffrollStartScene(void);
void scVSBattleStartScene(void);

/* P2-7 save data, transcribed in src/import/battleship_lbbackup.c. */
s32 lbBackupCreateChecksum(LBBackupData *backup);
sb32 lbBackupIsChecksumValid(void);
sb32 lbBackupIsSramValid(void);
void lbBackupApplyOptions(void);
void lbBackupCorrectErrors(void);
void lbBackupClearNewcomers(void);
void lbBackupClear1PHighScore(void);
void lbBackupClearVSRecord(void);
void lbBackupClearBonusStageTime(void);
void lbBackupClearPrize(void);
void lbBackupClearAllData(void);
sb32 scSubsysControllerGetPlayerTapButtons(u32 mask);
s32 scSubsysControllerGetPlayerHoldButtons(u32 mask);
sb32 scSubsysControllerCheckNoInputAll(void);
s32 scSubsysControllerGetPlayerStickLR(s8 range, sb32 right_or_left);
s32 scSubsysControllerGetPlayerStickUD(s8 range, sb32 up_or_down);
sb32 scSubsysControllerGetPlayerStickInRangeLR(s32 range_l_min, s32 range_r_min);
sb32 scSubsysControllerGetPlayerStickInRangeUD(s32 range_d_min, s32 range_u_min);
void scSubsysFighterSetLightParams(f32 light_angle_x, f32 light_angle_y,
                                   u8 r, u8 g, u8 b, u8 a);

#endif
