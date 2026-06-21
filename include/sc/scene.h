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
#define LBBACKUP_ERROR_RANDOMKNOCKBACK 1

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

enum { nSCBattleGameStatusWait };
enum { nSCBattleHandicapOff };
enum { nSCBattleItemSwitchNone, nSCBattleItemSwitchVeryLow,
       nSCBattleItemSwitchLow, nSCBattleItemSwitchMiddle };
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
enum { nSC1PGameDifficultyVeryEasy, nSC1PGameDifficultyEasy };
enum {
    nGRKindCastle,
    nGRKindSector,
    nGRKindJungle,
    nGRKindZebes,
    nGRKindHyrule,
    nGRKindYoster,
    nGRKindPupupu,
    nGRKindYamabuki
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

void lbBackupIsSramValid(void);
void lbBackupApplyOptions(void);
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
