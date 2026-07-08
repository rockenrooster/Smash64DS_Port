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

#define NDS_RELOC_OPENING_ROOM_FILE_COUNT 8u
#define NDS_RELOC_OPENING_ROOM_FILE_MASK 0xffu
#define NDS_RELOC_LOADED_FILE_CAPACITY 96u
#define NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES (128u * 1024u)

#define NDS_RELOC_ASSET_INVALID 0xffffffffu
#define NDS_RELOC_ASSET_MN_COMMON 0u
#define NDS_RELOC_ASSET_N64_LOGO 194u
#define NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE 37u
#define NDS_RELOC_ASSET_IF_COMMON_PLAYER 0xa6u
#define NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS 0x52u
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
#define NDS_RELOC_ASSET_STAGE_CASTLE 0x1005fu
#define NDS_RELOC_ASSET_STAGE_DREAM_LAND 0x10058u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_113 0x71u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_103 0x67u
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_104 0x68u
#define NDS_RELOC_ASSET_MISC_DATA_BANK_152 0x98u
#define NDS_RELOC_ASSET_GR_PUPUPU_MAP 0xffu
#define NDS_RELOC_ASSET_GR_INISHIE_MAP 0x104u
#define NDS_RELOC_ASSET_GR_HYRULE_MAP 0x109u
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
#define NDS_RELOC_ASSET_MARIO_ANIM_WAIT 0x1f3u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK1 0x1f4u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK2 0x1f5u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK3 0x1f6u
#define NDS_RELOC_ASSET_MARIO_ANIM_WALK_END 0x1f7u
#define NDS_RELOC_ASSET_MARIO_ANIM_DASH 0x1f8u
#define NDS_RELOC_ASSET_MARIO_ANIM_TURN_RUN 0x1fcu
#define NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_DROP 0x207u
#define NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST 0x20bu
#define NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_LAST 0x21cu
#define NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_ON 0x22du
#define NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_OFF 0x22eu
#define NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_GROUND 0x27bu
#define NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_AIR 0x27cu
#define NDS_RELOC_ASSET_MARIO_ANIM_SUPER_JUMP_PUNCH 0x27du
#define NDS_RELOC_ASSET_MARIO_ANIM_TORNADO_GROUND 0x27eu
#define NDS_RELOC_ASSET_MARIO_ANIM_TORNADO_AIR 0x27fu
#define NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE 0x280u
#define NDS_RELOC_ASSET_FOX_ANIM_EGGLAY 0x282u
#define NDS_RELOC_ASSET_FOX_ANIM_WALK1 0x283u
#define NDS_RELOC_ASSET_FOX_ANIM_WALK2 0x284u
#define NDS_RELOC_ASSET_FOX_ANIM_WALK3 0x285u
#define NDS_RELOC_ASSET_FOX_ANIM_WALK_END 0x286u
#define NDS_RELOC_ASSET_FOX_ANIM_DASH 0x287u
#define NDS_RELOC_ASSET_FOX_ANIM_TURN_RUN 0x28bu
#define NDS_RELOC_ASSET_FOX_ANIM_JUMP_F 0x292u
#define NDS_RELOC_ASSET_FOX_ANIM_JUMP_AERIAL_B 0x295u
#define NDS_RELOC_ASSET_FOX_ANIM_CATCH 0x2dcu
#define NDS_RELOC_ASSET_FOX_ANIM_THROW_B 0x2dfu
#define NDS_RELOC_ASSET_FOX_ANIM_JAB1 0x2efu
#define NDS_RELOC_ASSET_FOX_ANIM_FTILT_HIGH 0x2f2u
#define NDS_RELOC_ASSET_FOX_ANIM_FTILT_LOW 0x2f6u
#define NDS_RELOC_ASSET_FOX_ANIM_UTILT 0x2f7u
#define NDS_RELOC_ASSET_FOX_ANIM_DTILT 0x2f8u
#define NDS_RELOC_ASSET_FOX_ANIM_FSMASH 0x2f9u
#define NDS_RELOC_ASSET_FOX_ANIM_ATTACK_AIR_N 0x2fcu
#define NDS_RELOC_ASSET_FOX_ANIM_LASER 0x30bu
#define NDS_RELOC_ASSET_FOX_ANIM_LASER_AERIAL 0x30cu
#define NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND 0x30du
#define NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_LANDING_AIR 0x315u

#define NDS_FIGHTER_MARIOFOX_FILE_MASK 0x7fffu
#define NDS_FIGHTER_MARIOFOX_SETUP_FILES (1u << 0)
#define NDS_FIGHTER_MARIOFOX_SETUP_MANAGER_ALLOC (1u << 1)
#define NDS_FIGHTER_MARIOFOX_SETUP_MARIO_FILES (1u << 2)
#define NDS_FIGHTER_MARIOFOX_SETUP_FOX_FILES (1u << 3)
#define NDS_FIGHTER_MARIOFOX_SETUP_MARIO_ATTR (1u << 4)
#define NDS_FIGHTER_MARIOFOX_SETUP_FOX_ATTR (1u << 5)
#define NDS_FIGHTER_MARIOFOX_SETUP_MARIO_COMMONPART (1u << 6)
#define NDS_FIGHTER_MARIOFOX_SETUP_FOX_COMMONPART (1u << 7)
#define NDS_FIGHTER_MARIOFOX_SETUP_MARIO_GOBJ (1u << 8)
#define NDS_FIGHTER_MARIOFOX_SETUP_FOX_GOBJ (1u << 9)
#define NDS_FIGHTER_MARIOFOX_SETUP_DISPLAY (1u << 10)
#define NDS_FIGHTER_MARIOFOX_SETUP_PROCESS_DEFER (1u << 11)

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

#define NDS_RELOC_EXTERN_FILE_ID_CAPACITY 64u

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
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K 0x2f98u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L 0x3358u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M 0x3980u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N 0x3e88u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O 0x44b0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P 0x4890u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R 0x5418u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S 0x57f0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U 0x60d8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X 0x7108u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y 0x7608u
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
#define NDS_RELOC_SYMBOL_GR_PUPUPU_MAP_HEADER 0x14u
#define NDS_RELOC_SYMBOL_GR_HYRULE_MAP_HEADER 0x14u
#define NDS_RELOC_SYMBOL_GR_INISHIE_MAP_HEADER 0x14u

#define NDS_OPENING_PORTRAITS_CARD_WIDTH 300u
#define NDS_OPENING_PORTRAITS_CARD_HEIGHT 55u
#define NDS_OPENING_PORTRAITS_COVER_WIDTH 656u
#define NDS_OPENING_PORTRAITS_COVER_HEIGHT 55u
#define NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH 96u
#define NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT 64u
#define NDS_TITLE_MAX_WIDTH 320u
#define NDS_TITLE_MAX_HEIGHT 240u
#define NDS_TITLE_FILE_BUFFER_SIZE 176000u
#define NDS_OPENING_ACTION_PREVIEW_MAX_WIDTH 320u
#define NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT 264u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH 320u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT 240u
#define NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE 270000u
#if (NDS_DEV_SCENE_HARNESS >= 11)
#define NDS_OPENING_ACTION_PREVIEW_CACHE_COUNT 1u
#else
#define NDS_OPENING_ACTION_PREVIEW_CACHE_COUNT 3u
#endif
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

#define NDS_RELOC_ALIGN(value) (((value) + 0xFu) & ~0xFu)
#define NDS_FIGHTER_DL_SCAN_ASSET_ARENA 0xfffffffeu
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
#define NDS_FIGHTER_DL_MULTI_DRAW_WIDTH 96u
#define NDS_FIGHTER_DL_MULTI_DRAW_HEIGHT 72u
#define NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED 4u
#define NDS_FIGHTER_DL_MULTI_DRAW_MAX_CANDIDATES 32u
#define NDS_FIGHTER_DL_ALL_DRAW_WIDTH 96u
#define NDS_FIGHTER_DL_ALL_DRAW_HEIGHT 72u
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
    u32 extern_file_ids[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 external_fixup_count;
    u32 external_fixup_fail_count;
    u32 internal_fixup_count;
    u8 internal_fixups_applied;
    u8 external_fixups_applied;
    u8 format_fixups_applied;
    u8 fixups_applying;
} NDSRelocLoadedFile;

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

static NDSFighterDLDrawState
    sNdsFighterDLAllDrawStates[2][NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
static NDSRendererStats
    sNdsFighterDLAllDrawStats[2][NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
static u8
    sNdsFighterDLAllDrawClean[2][NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];

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

typedef struct NDSOpeningActionPreviewCache {
    u32 asset_id;
    u32 offset;
    u32 ready;
    u16 width;
    u16 height;
    u8 bmfmt;
    u8 bmsiz;
    u32 pixel_count;
    u16 pixels[NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH *
               NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT];
} NDSOpeningActionPreviewCache;

#define NDS_RELOC_STAGE_CASTLE_STATIC_SIZE NDS_RELOC_ALIGN(0x26cd0u)
#define NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE NDS_RELOC_ALIGN(0x6890u)

static NDSRelocLoadedFile sNdsRelocLoadedFiles[NDS_RELOC_LOADED_FILE_CAPACITY];
static u32 sNdsRelocLoadedFileCount;
static u32 sNdsRelocOwnerScene = NDS_RELOC_ASSET_INVALID;
static u32 sNdsRelocSceneGeneration;
static LBFileNode *sNdsRelocStatusBuffer;
static s32 sNdsRelocStatusBufferCount;
static s32 sNdsRelocStatusBufferMax;
static LBFileNode *sNdsRelocForceStatusBuffer;
static s32 sNdsRelocForceStatusBufferCount;
static s32 sNdsRelocForceStatusBufferMax;

static u8 sNdsTitleFileBuffer[NDS_TITLE_FILE_BUFFER_SIZE];
static u8 sNdsOpeningActionPreviewFileBuffer[
    NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE] __attribute__((aligned(16)));
static NDSOpeningActionPreviewCache sNdsOpeningActionPreviewCaches[
    NDS_OPENING_ACTION_PREVIEW_CACHE_COUNT];

static const NDSOpeningActionPreviewDesc sNdsOpeningActionPreviewDescs[] = {
    {
        nSCKindOpeningRun, NDS_RELOC_ASSET_OPENING_RUN,
        &llMVOpeningRunWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER,
        160, 120, 11, G_IM_FMT_CI, G_IM_SIZ_8b, 48, 40
    },
    {
        nSCKindOpeningCliff, NDS_RELOC_ASSET_OPENING_SECTOR,
        &llMVOpeningSectorCockpitSprite,
        NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT,
        320, 240, 48, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningYamabuki, NDS_RELOC_ASSET_OPENING_YAMABUKI,
        &llMVOpeningYamabukiWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER,
        320, 264, 53, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningJungle, NDS_RELOC_ASSET_OPENING_RUN,
        &llMVOpeningRunWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER,
        160, 120, 11, G_IM_FMT_CI, G_IM_SIZ_8b, 48, 40
    },
    {
        nSCKindOpeningYoster, NDS_RELOC_ASSET_OPENING_YAMABUKI,
        &llMVOpeningYamabukiWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER,
        320, 264, 53, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningSector, NDS_RELOC_ASSET_OPENING_SECTOR,
        &llMVOpeningSectorCockpitSprite,
        NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT,
        320, 240, 48, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningStandoff, NDS_RELOC_ASSET_OPENING_RUN,
        &llMVOpeningRunWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER,
        160, 120, 11, G_IM_FMT_CI, G_IM_SIZ_8b, 48, 40
    },
    {
        nSCKindOpeningClash, NDS_RELOC_ASSET_OPENING_SECTOR,
        &llMVOpeningSectorCockpitSprite,
        NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT,
        320, 240, 48, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningNewcomers, NDS_RELOC_ASSET_OPENING_YAMABUKI,
        &llMVOpeningYamabukiWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER,
        320, 264, 53, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
};

static const NDSRelocSymbolProbe sNdsRelocSymbolProbes[] = {
    {
        &llMVCommonRoomBackgroundDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_DOBJ,
        (1u << 0)
    },
    {
        &llMVCommonRoomDeskDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_DESK_DOBJ,
        (1u << 18)
    },
    {
        &llMVCommonRoomOutsideDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_OUTSIDE_DL,
        (1u << 16)
    },
    {
        &llMVCommonRoomHazeDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_HAZE_DL,
        (1u << 17)
    },
    {
        &llMVCommonRoomSunlightDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SUNLIGHT_DL,
        (1u << 1)
    },
    {
        &llMVOpeningRoomTransitionOverlayDisplayList,
        NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION,
        NDS_RELOC_SYMBOL_OPENING_ROOM_TRANSITION_OVERLAY_DL,
        (1u << 2)
    },
    {
        &llMVOpeningRoomScene1CamAnimJoint,
        NDS_RELOC_ASSET_OPENING_ROOM_SCENE1,
        NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE1_CAM_ANIM,
        (1u << 3)
    },
    {
        &llMVOpeningRoomScene2CamAnimJoint,
        NDS_RELOC_ASSET_OPENING_ROOM_SCENE2,
        NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE2_CAM_ANIM,
        (1u << 15)
    },
    {
        &llMVCommonRoomPencilsDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ,
        (1u << 5)
    },
    {
        &llMVCommonRoomPencilsAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM,
        (1u << 6)
    },
    {
        &llMVCommonRoomLogoDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_LOGO_DOBJ,
        (1u << 9)
    },
    {
        &llMVCommonRoomLogoMObjSub,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MOBJ,
        (1u << 10)
    },
    {
        &llMVCommonRoomLogoMatAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MATANIM,
        (1u << 11)
    },
    {
        &llMVCommonRoomBossShadowDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_DL,
        (1u << 7)
    },
    {
        &llMVCommonRoomBossShadowAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_ANIM,
        (1u << 8)
    },
    {
        &llMVCommonRoomSpotlightDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_DL,
        (1u << 12)
    },
    {
        &llMVCommonRoomSpotlightMObjSub,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MOBJ,
        (1u << 13)
    },
    {
        &llMVCommonRoomSpotlightMatAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MATANIM,
        (1u << 14)
    },
    {
        &llMVOpeningRoomWallpaperSprite,
        NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER,
        NDS_RELOC_SYMBOL_OPENING_ROOM_WALLPAPER_SPRITE,
        (1u << 4)
    },
};

static const NDSTitleSpriteDesc sNdsTitleSpriteDescs[] = {
    {
        &llMNTitleCutoutSprite,
        NDS_RELOC_SYMBOL_TITLE_CUTOUT,
        157, 94,
        208, 90,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitleSmashSprite,
        NDS_RELOC_SYMBOL_TITLE_SMASH,
        161, 88,
        172, 62,
        G_IM_FMT_RGBA, G_IM_SIZ_32b
    },
    {
        &llMNTitleSuperSprite,
        NDS_RELOC_SYMBOL_TITLE_SUPER,
        55, 96,
        64, 50,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleBrosSprite,
        NDS_RELOC_SYMBOL_TITLE_BROS,
        268, 96,
        56, 52,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleTMUnkSprite,
        NDS_RELOC_SYMBOL_TITLE_TM_UNK,
        270, 132,
        32, 12,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitleCopyrightSprite,
        NDS_RELOC_SYMBOL_TITLE_COPYRIGHT,
        160, 208,
        300, 44,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleBorderUpperSprite,
        NDS_RELOC_SYMBOL_TITLE_BORDER_UPPER,
        160, 15,
        300, 10,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitlePressStartSprite,
        NDS_RELOC_SYMBOL_TITLE_PRESS_START,
        162, 177,
        96, 18,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleLogoAnimFullSprite,
        NDS_RELOC_SYMBOL_TITLE_LOGO_FULL,
        260, 60,
        128, 124,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitleTMSprite,
        NDS_RELOC_SYMBOL_TITLE_TM,
        277, 157,
        32, 12,
        G_IM_FMT_I, G_IM_SIZ_4b
    }
};

static const NDSRelocKnownSymbol sNdsTitleFireAnimFrameSymbols[] = {
    { &llMNTitleFireAnimFrame1Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME1 },
    { &llMNTitleFireAnimFrame2Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME2 },
    { &llMNTitleFireAnimFrame3Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME3 },
    { &llMNTitleFireAnimFrame4Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME4 },
    { &llMNTitleFireAnimFrame5Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME5 },
    { &llMNTitleFireAnimFrame6Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME6 },
    { &llMNTitleFireAnimFrame7Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME7 },
    { &llMNTitleFireAnimFrame8Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME8 },
    { &llMNTitleFireAnimFrame9Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME9 },
    { &llMNTitleFireAnimFrame10Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME10 },
    { &llMNTitleFireAnimFrame11Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME11 },
    { &llMNTitleFireAnimFrame12Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME12 },
    { &llMNTitleFireAnimFrame13Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME13 },
    { &llMNTitleFireAnimFrame14Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME14 },
    { &llMNTitleFireAnimFrame15Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME15 },
    { &llMNTitleFireAnimFrame16Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME16 },
    { &llMNTitleFireAnimFrame17Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME17 },
    { &llMNTitleFireAnimFrame18Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME18 },
    { &llMNTitleFireAnimFrame19Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME19 },
    { &llMNTitleFireAnimFrame20Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME20 },
    { &llMNTitleFireAnimFrame21Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME21 },
    { &llMNTitleFireAnimFrame22Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME22 },
    { &llMNTitleFireAnimFrame23Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME23 },
    { &llMNTitleFireAnimFrame24Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME24 },
    { &llMNTitleFireAnimFrame25Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME25 },
    { &llMNTitleFireAnimFrame26Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME26 },
    { &llMNTitleFireAnimFrame27Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME27 },
    { &llMNTitleFireAnimFrame28Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME28 },
    { &llMNTitleFireAnimFrame29Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME29 },
    { &llMNTitleFireAnimFrame30Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME30 },
};

static const NDSRelocKnownSymbol sNdsMNCommonSymbols[] = {
    { &llMNCommonOptionTabLeftSprite, NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_LEFT },
    { &llMNCommonOptionTabMiddleSprite, NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_MIDDLE },
    { &llMNCommonOptionTabRightSprite, NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_RIGHT },
    { &llMNCommonFrameSprite, NDS_RELOC_SYMBOL_MNCOMMON_FRAME },
    { &llMNCommonGameModeTextSprite, NDS_RELOC_SYMBOL_MNCOMMON_GAME_MODE_TEXT },
    { &llMNCommonDigit0Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT0 },
    { &llMNCommonDigit1Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT1 },
    { &llMNCommonDigit2Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT2 },
    { &llMNCommonDigit3Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT3 },
    { &llMNCommonDigit4Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT4 },
    { &llMNCommonDigit5Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT5 },
    { &llMNCommonDigit6Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT6 },
    { &llMNCommonDigit7Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT7 },
    { &llMNCommonDigit8Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT8 },
    { &llMNCommonDigit9Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT9 },
    { &llMNCommonInfinitySprite, NDS_RELOC_SYMBOL_MNCOMMON_INFINITY },
    { &llMNCommonArrowRSprite, NDS_RELOC_SYMBOL_MNCOMMON_ARROW_R },
    { &llMNCommonArrowLSprite, NDS_RELOC_SYMBOL_MNCOMMON_ARROW_L },
    { &llMNCommonDecalPaperSprite, NDS_RELOC_SYMBOL_MNCOMMON_DECAL_PAPER },
    { &llMNCommonSmashLogoSprite, NDS_RELOC_SYMBOL_MNCOMMON_SMASH_LOGO },
    { &llMNCommonSmashBrosCollageSprite, NDS_RELOC_SYMBOL_MNCOMMON_SMASH_BROS_COLLAGE },
};

static const NDSRelocKnownSymbol sNdsMNVSModeSymbols[] = {
    { &llMNVSModeVSStartTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_VS_START_TEXT },
    { &llMNVSModeRulePeriodTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_RULE_PERIOD_TEXT },
    { &llMNVSModeTimeTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_TIME_TEXT },
    { &llMNVSModeStockTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_STOCK_TEXT },
    { &llMNVSModeTeamTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_TEAM_TEXT },
    { &llMNVSModeTimePeriodTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_TIME_PERIOD_TEXT },
    { &llMNVSModeMinTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_MIN_TEXT },
    { &llMNVSModeStockPeriodTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_STOCK_PERIOD_TEXT },
    { &llMNVSModeVSOptionsTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_VS_OPTIONS_TEXT },
    { &llMNVSModeConsoleIconDarkSprite, NDS_RELOC_SYMBOL_MNVSMODE_CONSOLE_ICON_DARK },
    { &llMNVSModeVSTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_VS_TEXT },
};

#define NDS_IFCOMMON_ASSET_SYMBOL(asset, name, value) { asset, &name, value },
static const NDSRelocKnownAssetSymbol sNdsIFCommonSymbols[] = {
    NDS_IFCOMMON_RELOC_SYMBOLS(NDS_IFCOMMON_ASSET_SYMBOL)
};
#undef NDS_IFCOMMON_ASSET_SYMBOL

static u32 ndsRelocFileID(const void *file_id)
{
    return (u32)(uintptr_t)file_id;
}

static u32 ndsFighterManagerExternBit(u32 token)
{
    if (token == ndsRelocFileID(&llFTManagerCommonFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_COMMON;
    }
    if (token == ndsRelocFileID(&llFTCommonMovesetFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_COMMON_MOVESET;
    }
    if (token == ndsRelocFileID(&llMarioMainFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_MARIO_MAIN;
    }
    if (token == ndsRelocFileID(&llFoxMainFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_FOX_MAIN;
    }
    return 0u;
}

static u32 ndsFighterManagerStatusBit(u32 token)
{
    if (token == ndsRelocFileID(&llMarioMainMotionFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_MAINMOTION;
    }
    if (token == ndsRelocFileID(&llMarioModelFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_MODEL;
    }
    if (token == ndsRelocFileID(&llMarioShieldPoseFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SHIELD;
    }
    if (token == ndsRelocFileID(&llMarioSpecial1FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL1;
    }
    if (token == ndsRelocFileID(&llMarioSpecial2FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL2;
    }
    if (token == ndsRelocFileID(&llMarioSpecial3FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL3;
    }
    if (token == ndsRelocFileID(&llFoxMainMotionFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_MAINMOTION;
    }
    if (token == ndsRelocFileID(&llFoxModelFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_MODEL;
    }
    if (token == ndsRelocFileID(&llFoxShieldPoseFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SHIELD;
    }
    if (token == ndsRelocFileID(&llFoxSpecial1FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL1;
    }
    if (token == ndsRelocFileID(&llFoxSpecial2FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL2;
    }
    if (token == ndsRelocFileID(&llFoxSpecial3FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL3;
    }
    if (token == ndsRelocFileID(&llFoxSpecial4FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL4;
    }
    return 0u;
}

static void ndsFighterManagerRefreshProof(void)
{
    u32 mask = 0u;

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    mask |= 1u << 0;
#endif
    if ((gNdsFighterManagerExternMask &
         NDS_FIGHTER_MANAGER_EXTERN_REQUIRED_MASK) ==
        NDS_FIGHTER_MANAGER_EXTERN_REQUIRED_MASK)
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterManagerStatusBufferMask &
         NDS_FIGHTER_MANAGER_STATUS_REQUIRED_MASK) ==
        NDS_FIGHTER_MANAGER_STATUS_REQUIRED_MASK)
    {
        mask |= 1u << 2;
    }
    gNdsFighterManagerFigatreeHeapSize = (u32)gFTManagerFigatreeHeapSize;
    if (gNdsFighterManagerFigatreeHeapSize != 0u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterManagerFighterMask & 0x3u) == 0x3u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterManagerDataMask & 0x3u) == 0x3u)
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterManagerWaitMask | gNdsFighterManagerEntryMask) & 0x3u) ==
        0x3u)
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterManagerStatusBufferHitCount >= 13u)
    {
        mask |= 1u << 7;
    }

    gNdsFighterManagerMask = mask;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterManagerResult = NDS_FIGHTER_MANAGER_PASS;
    }
}

static void ndsFighterManagerRecordExternToken(u32 token, const void *file)
{
    u32 bit;

    if (file == NULL)
    {
        return;
    }
    bit = ndsFighterManagerExternBit(token);
    if (bit != 0u)
    {
        gNdsFighterManagerExternMask |= bit;
        ndsFighterManagerRefreshProof();
    }
}

static void ndsFighterManagerRecordStatusToken(u32 token, const void *file)
{
    u32 bit;

    if (file == NULL)
    {
        return;
    }
    bit = ndsFighterManagerStatusBit(token);
    if (bit != 0u)
    {
        gNdsFighterManagerStatusBufferMask |= bit;
        gNdsFighterManagerStatusBufferHitCount++;
        ndsFighterManagerRefreshProof();
    }
}

static u32 ndsFloatBits(f32 value)
{
    union {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return bits.u;
}

static f32 ndsFloatFromBits(u32 value)
{
    union {
        f32 f;
        u32 u;
    } bits;

    bits.u = value;
    return bits.f;
}

static u32 ndsPupupuStageAssetBit(u32 asset_id)
{
    if (asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP) return 1u << 0;
    if (asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND) return 1u << 1;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_103) return 1u << 2;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_104) return 1u << 3;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_152) return 1u << 4;
    return 0;
}

static u32 ndsFighterMarioFoxAssetBit(u32 asset_id)
{
    if (asset_id == NDS_RELOC_ASSET_FT_MANAGER_COMMON) return 1u << 0;
    if (asset_id == NDS_RELOC_ASSET_MARIO_MAIN_MOTION) return 1u << 1;
    if (asset_id == NDS_RELOC_ASSET_MARIO_MAIN) return 1u << 2;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL1) return 1u << 3;
    if (asset_id == NDS_RELOC_ASSET_MARIO_MODEL) return 1u << 4;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL3) return 1u << 5;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SHIELD_POSE) return 1u << 6;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL2) return 1u << 7;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL3) return 1u << 8;
    if (asset_id == NDS_RELOC_ASSET_FOX_MAIN_MOTION) return 1u << 9;
    if (asset_id == NDS_RELOC_ASSET_FOX_MAIN) return 1u << 10;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL1) return 1u << 11;
    if (asset_id == NDS_RELOC_ASSET_FOX_MODEL) return 1u << 12;
    if (asset_id == NDS_RELOC_ASSET_FOX_SHIELD_POSE) return 1u << 13;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL4) return 1u << 14;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL2) return 1u << 15;
    return 0;
}

static u32 ndsFighterMarioFoxDependencyBit(u32 asset_id)
{
    if (ndsFighterMarioFoxAssetBit(asset_id) != 0u)
    {
        return ndsFighterMarioFoxAssetBit(asset_id);
    }
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_201) return 1u << 16;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_299) return 1u << 17;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_315) return 1u << 18;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_109) return 1u << 19;
    return 0;
}

static void ndsRelocRecordExternalFixupSuccess(u32 source_asset_id,
                                               u32 dep_asset_id)
{
    if (ndsPupupuStageAssetBit(source_asset_id) != 0u)
    {
        gNdsStagePupupuExternalFixupCount++;
        gNdsStagePupupuRelocDependencyMask |=
            ndsPupupuStageAssetBit(dep_asset_id);
    }
    if (ndsFighterMarioFoxAssetBit(source_asset_id) != 0u)
    {
        gNdsFighterMarioFoxExternalFixupCount++;
        gNdsFighterMarioFoxRelocDependencyMask |=
            ndsFighterMarioFoxDependencyBit(dep_asset_id);
    }
}

static void ndsRelocRecordExternalFixupFail(u32 source_asset_id)
{
    if (ndsPupupuStageAssetBit(source_asset_id) != 0u)
    {
        gNdsStagePupupuExternalFixupFailCount++;
    }
    if (ndsFighterMarioFoxAssetBit(source_asset_id) != 0u)
    {
        gNdsFighterMarioFoxExternalFixupFailCount++;
    }
}

static u32 ndsRelocReadBe32(const void *addr)
{
    const u8 *bytes = addr;

    return ((u32)bytes[0] << 24) |
           ((u32)bytes[1] << 16) |
           ((u32)bytes[2] << 8) |
           (u32)bytes[3];
}

static u32 ndsRelocReadNative32(const void *addr)
{
    u32 value;

    memcpy(&value, addr, sizeof(value));
    return value;
}

static void ndsRelocWriteNative32(void *addr, u32 value)
{
    memcpy(addr, &value, sizeof(value));
}

static void ndsRelocWriteNativePointer(void *addr, void *target)
{
    ndsRelocWriteNative32(addr, (u32)(uintptr_t)target);
}

static s32 ndsRelocIsMarioFoxNaturalCombatAnimID(u32 asset_id)
{
    return (((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_WAIT) &&
             (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_TURN_RUN)) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_DROP) ||
            ((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST) &&
             (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_LAST)) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_ON) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_OFF) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_GROUND) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_AIR) ||
            ((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_SUPER_JUMP_PUNCH) &&
             (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_TORNADO_AIR)) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE) ||
            ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_EGGLAY) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_TURN_RUN)) ||
            ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_JUMP_F) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_JUMP_AERIAL_B)) ||
            ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_CATCH) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_THROW_B)) ||
            (asset_id == NDS_RELOC_ASSET_FOX_ANIM_JAB1) ||
            ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FTILT_HIGH) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_ATTACK_AIR_N)) ||
            (asset_id == NDS_RELOC_ASSET_FOX_ANIM_LASER) ||
            (asset_id == NDS_RELOC_ASSET_FOX_ANIM_LASER_AERIAL) ||
            ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_LANDING_AIR))) ?
        TRUE :
        FALSE;
}

static u32 ndsRelocAssetIDForToken(u32 token)
{
    if (token == ndsRelocFileID(&llN64LogoFileID)) return NDS_RELOC_ASSET_N64_LOGO;
    if (token == ndsRelocFileID(&llIFCommonPlayerFileID)) return NDS_RELOC_ASSET_IF_COMMON_PLAYER;
    if (token == ndsRelocFileID(&llIFCommonGameStatusFileID)) return NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS;
    if (token == ndsRelocFileID(&llIFCommonPlayerDamageFileID)) return NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE;
    if (token == ndsRelocFileID(&llIFCommonTimerFileID)) return NDS_RELOC_ASSET_IF_COMMON_TIMER;
    if (token == ndsRelocFileID(&llIFCommonDigitsFileID)) return NDS_RELOC_ASSET_IF_COMMON_DIGITS;
    if (token == ndsRelocFileID(&llIFCommonBattlePauseFileID)) return NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE;
    if (token == ndsRelocFileID(&llIFCommonPlayerTagsFileID)) return NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS;
    if (token == ndsRelocFileID(&llIFCommonAnnounceCommonFileID)) return NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE;
    if (token == ndsRelocFileID(&llSYKseg1ValidateFileID)) return NDS_RELOC_ASSET_SY_KSEG1_VALIDATE;
    if (token == ndsRelocFileID(&llMVCommonFileID)) return NDS_RELOC_ASSET_MV_COMMON;
    if (token == ndsRelocFileID(&llMVOpeningCommonFileID)) return NDS_RELOC_ASSET_OPENING_COMMON;
    if (token == ndsRelocFileID(&llMVOpeningRoomTransitionFileID)) return NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene1FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE1;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene2FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE2;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene3FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE3;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene4FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE4;
    if (token == ndsRelocFileID(&llMVOpeningRunFileID)) return NDS_RELOC_ASSET_OPENING_RUN;
    if (token == ndsRelocFileID(&llMVOpeningYamabukiFileID)) return NDS_RELOC_ASSET_OPENING_YAMABUKI;
    if (token == ndsRelocFileID(&llMVOpeningSectorFileID)) return NDS_RELOC_ASSET_OPENING_SECTOR;
    if (token == ndsRelocFileID(&llMVOpeningRunCrashFileID)) return NDS_RELOC_ASSET_OPENING_RUN_CRASH;
    if (token == ndsRelocFileID(&llMVOpeningRoomWallpaperFileID)) return NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER;
    if (token == ndsRelocFileID(&llMVOpeningPortraitsSet1FileID)) return NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1;
    if (token == ndsRelocFileID(&llMVOpeningPortraitsSet2FileID)) return NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2;
    if (token == ndsRelocFileID(&llMNTitleFileID)) return NDS_RELOC_ASSET_MN_TITLE;
    if (token == ndsRelocFileID(&llMNTitleFireAnimFileID)) return NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM;
    if (token == ndsRelocFileID(&llMNCommonFileID)) return NDS_RELOC_ASSET_MN_COMMON;
    if (token == ndsRelocFileID(&llMNVSModeFileID)) return NDS_RELOC_ASSET_MN_VS_MODE;
    if (token == ndsRelocFileID(&llMNPlayersCommonFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_COMMON;
    if (token == ndsRelocFileID(&llMNPlayersGameModesFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_GAME_MODES;
    if (token == ndsRelocFileID(&llMNPlayersPortraitsFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_PORTRAITS;
    if (token == ndsRelocFileID(&llFTEmblemSpritesFileID)) return NDS_RELOC_ASSET_FT_EMBLEM_SPRITES;
    if (token == ndsRelocFileID(&llMNSelectCommonFileID)) return NDS_RELOC_ASSET_MN_SELECT_COMMON;
    if (token == ndsRelocFileID(&llMNPlayersSpotlightFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_SPOTLIGHT;
    if (token == ndsRelocFileID(&llGRWallpaperTrainingBlackFileID)) return NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLACK;
    if (token == ndsRelocFileID(&llMNMapsFileID)) return NDS_RELOC_ASSET_MN_MAPS;
    if (token == ndsRelocFileID(&llMNCommonFontsFileID)) return NDS_RELOC_ASSET_MN_COMMON_FONTS;
    if (token == ndsRelocFileID(&llStageDreamLandFileID)) return NDS_RELOC_ASSET_STAGE_DREAM_LAND;
    if (token == ndsRelocFileID(&llStageCastleFileID)) return NDS_RELOC_ASSET_STAGE_CASTLE;
    if (token == ndsRelocFileID(&ll_113_FileID)) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_113;
    if (token == ndsRelocFileID(&llGRPupupuMapFileID)) return NDS_RELOC_ASSET_GR_PUPUPU_MAP;
    if (token == ndsRelocFileID(&llGRInishieMapFileID)) return NDS_RELOC_ASSET_GR_INISHIE_MAP;
    if (token == ndsRelocFileID(&llGRHyruleMapFileID)) return NDS_RELOC_ASSET_GR_HYRULE_MAP;
    if (token == 0x58u) return NDS_RELOC_ASSET_STAGE_DREAM_LAND;
    if (token == 0x5fu) return NDS_RELOC_ASSET_STAGE_CASTLE;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_113) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_113;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_103) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_103;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_104) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_104;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_152) return NDS_RELOC_ASSET_MISC_DATA_BANK_152;
    if (token == ndsRelocFileID(&llFTManagerCommonFileID)) return NDS_RELOC_ASSET_FT_MANAGER_COMMON;
    if (token == ndsRelocFileID(&llMarioMainMotionFileID)) return NDS_RELOC_ASSET_MARIO_MAIN_MOTION;
    if (token == ndsRelocFileID(&llMarioMainFileID)) return NDS_RELOC_ASSET_MARIO_MAIN;
    if (token == ndsRelocFileID(&llMarioSpecial1FileID)) return NDS_RELOC_ASSET_MARIO_SPECIAL1;
    if (token == ndsRelocFileID(&llMarioModelFileID)) return NDS_RELOC_ASSET_MARIO_MODEL;
    if (token == ndsRelocFileID(&llMarioSpecial3FileID)) return NDS_RELOC_ASSET_MARIO_SPECIAL3;
    if (token == ndsRelocFileID(&llMarioShieldPoseFileID)) return NDS_RELOC_ASSET_MARIO_SHIELD_POSE;
    if (token == ndsRelocFileID(&llMarioSpecial2FileID)) return NDS_RELOC_ASSET_MARIO_SPECIAL2;
    if (token == ndsRelocFileID(&llFoxSpecial3FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL3;
    if (token == ndsRelocFileID(&llFoxMainMotionFileID)) return NDS_RELOC_ASSET_FOX_MAIN_MOTION;
    if (token == ndsRelocFileID(&llFoxMainFileID)) return NDS_RELOC_ASSET_FOX_MAIN;
    if (token == ndsRelocFileID(&llFoxSpecial1FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL1;
    if (token == ndsRelocFileID(&llFoxModelFileID)) return NDS_RELOC_ASSET_FOX_MODEL;
    if (token == ndsRelocFileID(&llFoxShieldPoseFileID)) return NDS_RELOC_ASSET_FOX_SHIELD_POSE;
    if (token == ndsRelocFileID(&llFoxSpecial4FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL4;
    if (token == ndsRelocFileID(&llFoxSpecial2FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL2;
    if (token == ndsRelocFileID(&llEFCommonEffects1FileID)) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS1;
    if (token == ndsRelocFileID(&llEFCommonEffects2FileID)) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS2;
    if (token == ndsRelocFileID(&llEFCommonEffects3FileID)) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS3;
    if (token == ndsRelocFileID(&llFTCommonMovesetFileID)) return NDS_RELOC_ASSET_MISC_DATA_201;
    if (token == NDS_RELOC_ASSET_FT_MANAGER_COMMON) return NDS_RELOC_ASSET_FT_MANAGER_COMMON;
    if (token == NDS_RELOC_ASSET_MARIO_MAIN_MOTION) return NDS_RELOC_ASSET_MARIO_MAIN_MOTION;
    if (token == NDS_RELOC_ASSET_MARIO_MAIN) return NDS_RELOC_ASSET_MARIO_MAIN;
    if (token == NDS_RELOC_ASSET_MARIO_SPECIAL1) return NDS_RELOC_ASSET_MARIO_SPECIAL1;
    if (token == NDS_RELOC_ASSET_MARIO_MODEL) return NDS_RELOC_ASSET_MARIO_MODEL;
    if (token == NDS_RELOC_ASSET_MARIO_SPECIAL3) return NDS_RELOC_ASSET_MARIO_SPECIAL3;
    if (token == NDS_RELOC_ASSET_MARIO_SHIELD_POSE) return NDS_RELOC_ASSET_MARIO_SHIELD_POSE;
    if (token == NDS_RELOC_ASSET_MARIO_SPECIAL2) return NDS_RELOC_ASSET_MARIO_SPECIAL2;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL3) return NDS_RELOC_ASSET_FOX_SPECIAL3;
    if (token == NDS_RELOC_ASSET_FOX_MAIN_MOTION) return NDS_RELOC_ASSET_FOX_MAIN_MOTION;
    if (token == NDS_RELOC_ASSET_FOX_MAIN) return NDS_RELOC_ASSET_FOX_MAIN;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL1) return NDS_RELOC_ASSET_FOX_SPECIAL1;
    if (token == NDS_RELOC_ASSET_FOX_MODEL) return NDS_RELOC_ASSET_FOX_MODEL;
    if (token == NDS_RELOC_ASSET_FOX_SHIELD_POSE) return NDS_RELOC_ASSET_FOX_SHIELD_POSE;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL4) return NDS_RELOC_ASSET_FOX_SPECIAL4;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL2) return NDS_RELOC_ASSET_FOX_SPECIAL2;
    if (token == NDS_RELOC_ASSET_EF_COMMON_EFFECTS1) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS1;
    if (token == NDS_RELOC_ASSET_EF_COMMON_EFFECTS2) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS2;
    if (token == NDS_RELOC_ASSET_EF_COMMON_EFFECTS3) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS3;
    if (token == NDS_RELOC_ASSET_MISC_DATA_201) return NDS_RELOC_ASSET_MISC_DATA_201;
    if (token == NDS_RELOC_ASSET_MISC_DATA_299) return NDS_RELOC_ASSET_MISC_DATA_299;
    if (token == NDS_RELOC_ASSET_MISC_DATA_315) return NDS_RELOC_ASSET_MISC_DATA_315;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_109) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_109;
    if (token == ndsRelocFileID(&llFTMarioAnimWaitFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_WAIT;
    if (token == ndsRelocFileID(&llFTMarioAnimWalk1FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_WALK1;
    if (token == ndsRelocFileID(&llFTMarioAnimWalk2FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_WALK2;
    if (token == ndsRelocFileID(&llFTMarioAnimWalk3FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_WALK3;
    if (token == ndsRelocFileID(&llFTMarioAnimWalkEndFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_WALK_END;
    if (token == ndsRelocFileID(&llFTMarioAnimDashFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DASH;
    if (token == ndsRelocFileID(&llFTMarioAnimRunFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DASH + 1u;
    if (token == ndsRelocFileID(&llFTMarioAnimRunBrakeFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DASH + 2u;
    if (token == ndsRelocFileID(&llFTMarioAnimTurnFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DASH + 3u;
    if (token == ndsRelocFileID(&llFTMarioAnimTurnRunFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_TURN_RUN;
    if (token == ndsRelocFileID(&llFTMarioAnimShieldDropFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_DROP;
    if (token == ndsRelocFileID(&llFTMarioAnimDamaged1FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST;
    if (token == ndsRelocFileID(&llFTMarioAnimDamaged2FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 1u;
    if (token == ndsRelocFileID(&llFTMarioAnimFalconDivePulledFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 2u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageX1FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 3u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageX2FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 4u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageX3FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 5u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamaged3FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 6u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamaged4FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 7u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamaged5FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 8u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamaged6FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 9u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageAirFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 10u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamaged7FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 11u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageFlyX1FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 12u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageFlyX2FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 13u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamage2FileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 14u;
    if (token == ndsRelocFileID(&llFTMarioAnimShieldBreakFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 15u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageFlyTopFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_FIRST + 16u;
    if (token == ndsRelocFileID(&llFTMarioAnimDamagedFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE_LOW_LAST;
    if (token == ndsRelocFileID(&llFTMarioAnimShieldOnFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_ON;
    if (token == ndsRelocFileID(&llFTMarioAnimShieldOffFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_SHIELD_OFF;
    if (token == ndsRelocFileID(&llFTMarioAnimFireballGroundFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_GROUND;
    if (token == ndsRelocFileID(&llFTMarioAnimFireballAirFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_AIR;
    if (token == ndsRelocFileID(&llFTMarioAnimSuperJumpPunchAirFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_SUPER_JUMP_PUNCH;
    if (token == ndsRelocFileID(&llFTMarioAnimMarioTornadoGroundFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_TORNADO_GROUND;
    if (token == ndsRelocFileID(&llFTMarioAnimMarioTornadoAirFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_TORNADO_AIR;
    if (token == ndsRelocFileID(&llFTMarioAnimDamageFileID)) return NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE;
    if (token == ndsRelocFileID(&llFTFoxAnimEggLayFileID)) return NDS_RELOC_ASSET_FOX_ANIM_EGGLAY;
    if (token == ndsRelocFileID(&llFTFoxAnimWalk1FileID)) return NDS_RELOC_ASSET_FOX_ANIM_WALK1;
    if (token == ndsRelocFileID(&llFTFoxAnimWalk2FileID)) return NDS_RELOC_ASSET_FOX_ANIM_WALK2;
    if (token == ndsRelocFileID(&llFTFoxAnimWalk3FileID)) return NDS_RELOC_ASSET_FOX_ANIM_WALK3;
    if (token == ndsRelocFileID(&llFTFoxAnimWalkEndFileID)) return NDS_RELOC_ASSET_FOX_ANIM_WALK_END;
    if (token == ndsRelocFileID(&llFTFoxAnimDashFileID)) return NDS_RELOC_ASSET_FOX_ANIM_DASH;
    if (token == ndsRelocFileID(&llFTFoxAnimRunFileID)) return NDS_RELOC_ASSET_FOX_ANIM_DASH + 1u;
    if (token == ndsRelocFileID(&llFTFoxAnimRunBrakeFileID)) return NDS_RELOC_ASSET_FOX_ANIM_DASH + 2u;
    if (token == ndsRelocFileID(&llFTFoxAnimTurnFileID)) return NDS_RELOC_ASSET_FOX_ANIM_DASH + 3u;
    if (token == ndsRelocFileID(&llFTFoxAnimTurnRunFileID)) return NDS_RELOC_ASSET_FOX_ANIM_TURN_RUN;
    if (token == ndsRelocFileID(&llFTFoxAnimJumpFFileID)) return NDS_RELOC_ASSET_FOX_ANIM_JUMP_F;
    if (token == ndsRelocFileID(&llFTFoxAnimJumpBFileID)) return NDS_RELOC_ASSET_FOX_ANIM_JUMP_F + 1u;
    if (token == ndsRelocFileID(&llFTFoxAnimJumpAerialFFileID)) return NDS_RELOC_ASSET_FOX_ANIM_JUMP_F + 2u;
    if (token == ndsRelocFileID(&llFTFoxAnimJumpAerialBFileID)) return NDS_RELOC_ASSET_FOX_ANIM_JUMP_AERIAL_B;
    if (token == ndsRelocFileID(&llFTFoxAnimCatchFileID)) return NDS_RELOC_ASSET_FOX_ANIM_CATCH;
    if (token == ndsRelocFileID(&llFTFoxAnimCatchPullFileID)) return NDS_RELOC_ASSET_FOX_ANIM_CATCH + 1u;
    if (token == ndsRelocFileID(&llFTFoxAnimThrowFFileID)) return NDS_RELOC_ASSET_FOX_ANIM_CATCH + 2u;
    if (token == ndsRelocFileID(&llFTFoxAnimThrowBFileID)) return NDS_RELOC_ASSET_FOX_ANIM_THROW_B;
    if (token == ndsRelocFileID(&llFTFoxAnimJab1FileID)) return NDS_RELOC_ASSET_FOX_ANIM_JAB1;
    if (token == ndsRelocFileID(&llFTFoxAnimFTiltHighFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FTILT_HIGH;
    if (token == ndsRelocFileID(&llFTFoxAnimFTiltMidHighFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FTILT_HIGH + 1u;
    if (token == ndsRelocFileID(&llFTFoxAnimFTiltFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FTILT_HIGH + 2u;
    if (token == ndsRelocFileID(&llFTFoxAnimFTiltMidLowFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FTILT_HIGH + 3u;
    if (token == ndsRelocFileID(&llFTFoxAnimFTiltLowFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FTILT_LOW;
    if (token == ndsRelocFileID(&llFTFoxAnimUTiltFileID)) return NDS_RELOC_ASSET_FOX_ANIM_UTILT;
    if (token == ndsRelocFileID(&llFTFoxAnimDTiltFileID)) return NDS_RELOC_ASSET_FOX_ANIM_DTILT;
    if (token == ndsRelocFileID(&llFTFoxAnimFSmashFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FSMASH;
    if (token == ndsRelocFileID(&llFTFoxAnimUSmashFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FSMASH + 1u;
    if (token == ndsRelocFileID(&llFTFoxAnimDSmashFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FSMASH + 2u;
    if (token == ndsRelocFileID(&llFTFoxAnimAttackAirNFileID)) return NDS_RELOC_ASSET_FOX_ANIM_ATTACK_AIR_N;
    if (token == ndsRelocFileID(&llFTFoxAnimLaserFileID)) return NDS_RELOC_ASSET_FOX_ANIM_LASER;
    if (token == ndsRelocFileID(&llFTFoxAnimLaserAerialFileID)) return NDS_RELOC_ASSET_FOX_ANIM_LASER_AERIAL;
    if (token == ndsRelocFileID(&llFTFoxAnimFireFoxStartGroundFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND;
    if (token == ndsRelocFileID(&llFTFoxAnimFireFoxStartAerialFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND + 1u;
    if (token == ndsRelocFileID(&llFTFoxAnimReadyingFireFoxGroundFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND + 2u;
    if (token == ndsRelocFileID(&llFTFoxAnimReadyingFireFoxAirFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND + 3u;
    if (token == ndsRelocFileID(&llFTFoxAnimFireFoxGroundFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND + 4u;
    if (token == ndsRelocFileID(&llFTFoxAnimFireFoxAirFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND + 5u;
    if (token == ndsRelocFileID(&llFTFoxAnimFireFoxEndGroundFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND + 6u;
    if (token == ndsRelocFileID(&llFTFoxAnimFireFoxEndAirFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND + 7u;
    if (token == ndsRelocFileID(&llFTFoxAnimLandingWhileFireFoxAirFileID)) return NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_LANDING_AIR;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WAIT) return NDS_RELOC_ASSET_MARIO_ANIM_WAIT;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK1) return NDS_RELOC_ASSET_MARIO_ANIM_WALK1;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK2) return NDS_RELOC_ASSET_MARIO_ANIM_WALK2;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK3) return NDS_RELOC_ASSET_MARIO_ANIM_WALK3;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK_END) return NDS_RELOC_ASSET_MARIO_ANIM_WALK_END;
    if (token == NDS_RELOC_ASSET_FOX_ANIM_EGGLAY) return NDS_RELOC_ASSET_FOX_ANIM_EGGLAY;
    if (token == NDS_RELOC_ASSET_FOX_ANIM_WALK1) return NDS_RELOC_ASSET_FOX_ANIM_WALK1;
    if (token == NDS_RELOC_ASSET_FOX_ANIM_WALK2) return NDS_RELOC_ASSET_FOX_ANIM_WALK2;
    if (token == NDS_RELOC_ASSET_FOX_ANIM_WALK3) return NDS_RELOC_ASSET_FOX_ANIM_WALK3;
    if (token == NDS_RELOC_ASSET_FOX_ANIM_WALK_END) return NDS_RELOC_ASSET_FOX_ANIM_WALK_END;
    if (ndsRelocIsMarioFoxNaturalCombatAnimID(token) != FALSE) return token;
    return NDS_RELOC_ASSET_INVALID;
}

static u32 ndsRelocOpeningRoomBitForAsset(u32 file_id)
{
    if (file_id == NDS_RELOC_ASSET_MV_COMMON) return (1u << 0);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION) return (1u << 1);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE1) return (1u << 2);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE2) return (1u << 3);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE3) return (1u << 4);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE4) return (1u << 5);
    if (file_id == NDS_RELOC_ASSET_OPENING_RUN_CRASH) return (1u << 6);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER) return (1u << 7);
    return 0;
}

static s32 ndsRelocIsOpeningRoomAsset(u32 asset_id)
{
    return (ndsRelocOpeningRoomBitForAsset(asset_id) != 0u) ? TRUE : FALSE;
}

static void ndsRelocResetLoadedFiles(void)
{
    memset(sNdsRelocLoadedFiles, 0, sizeof(sNdsRelocLoadedFiles));
    sNdsRelocLoadedFileCount = 0;
    ndsFighterMarioFoxResetFileSlots();
}

static void ndsRelocPrepareSceneCache(void)
{
    u32 scene = (u32)gSCManagerSceneData.scene_curr;
    u32 evicted_files = 0u;
    u32 evicted_bytes = 0u;
    u32 i;

    if (sNdsRelocOwnerScene == scene)
    {
        return;
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        evicted_files++;
        evicted_bytes += sNdsRelocLoadedFiles[i].data_size;
    }

    if (evicted_files != 0u)
    {
        ndsRelocResetLoadedFiles();
    }
    sNdsRelocStatusBufferCount = 0;
    sNdsRelocForceStatusBufferCount = 0;

    sNdsRelocOwnerScene = scene;
    sNdsRelocSceneGeneration++;
    gNdsMemoryLedgerEvictedFiles = evicted_files;
    gNdsMemoryLedgerEvictedBytes = evicted_bytes;
}

static s32 ndsRelocAssetIsInterface(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE:
    case NDS_RELOC_ASSET_IF_COMMON_PLAYER:
    case NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS:
    case NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE:
    case NDS_RELOC_ASSET_IF_COMMON_TIMER:
    case NDS_RELOC_ASSET_IF_COMMON_DIGITS:
    case NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE:
    case NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS:
    case NDS_RELOC_ASSET_SY_KSEG1_VALIDATE:
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsRelocAssetIsStage(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_STAGE_CASTLE:
    case NDS_RELOC_ASSET_STAGE_DREAM_LAND:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_113:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_103:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_104:
    case NDS_RELOC_ASSET_MISC_DATA_BANK_152:
    case NDS_RELOC_ASSET_GR_PUPUPU_MAP:
    case NDS_RELOC_ASSET_GR_INISHIE_MAP:
    case NDS_RELOC_ASSET_GR_HYRULE_MAP:
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsRelocAssetIsFighter(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_FT_MANAGER_COMMON:
    case NDS_RELOC_ASSET_MARIO_MAIN:
    case NDS_RELOC_ASSET_MARIO_MAIN_MOTION:
    case NDS_RELOC_ASSET_MARIO_MODEL:
    case NDS_RELOC_ASSET_MARIO_SHIELD_POSE:
    case NDS_RELOC_ASSET_MARIO_SPECIAL1:
    case NDS_RELOC_ASSET_MARIO_SPECIAL2:
    case NDS_RELOC_ASSET_MARIO_SPECIAL3:
    case NDS_RELOC_ASSET_FOX_MAIN:
    case NDS_RELOC_ASSET_FOX_MAIN_MOTION:
    case NDS_RELOC_ASSET_FOX_MODEL:
    case NDS_RELOC_ASSET_FOX_SHIELD_POSE:
    case NDS_RELOC_ASSET_FOX_SPECIAL1:
    case NDS_RELOC_ASSET_FOX_SPECIAL2:
    case NDS_RELOC_ASSET_FOX_SPECIAL3:
    case NDS_RELOC_ASSET_FOX_SPECIAL4:
    case NDS_RELOC_ASSET_MISC_DATA_201:
    case NDS_RELOC_ASSET_MISC_DATA_299:
    case NDS_RELOC_ASSET_MISC_DATA_315:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_109:
        return TRUE;
    default:
        break;
    }
    if (((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_WAIT) &&
         (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_DAMAGE)) ||
        ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_EGGLAY) &&
         (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_JAB1)) ||
        ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_JUMP_F) &&
         (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_JUMP_AERIAL_B)) ||
        ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FTILT_HIGH) &&
         (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_ATTACK_AIR_N)) ||
        (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_GROUND) ||
        (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_FIREBALL_AIR) ||
        ((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_SUPER_JUMP_PUNCH) &&
         (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_TORNADO_AIR)) ||
        (asset_id == NDS_RELOC_ASSET_FOX_ANIM_LASER) ||
        (asset_id == NDS_RELOC_ASSET_FOX_ANIM_LASER_AERIAL) ||
        ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_START_GROUND) &&
         (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_FIREFOX_LANDING_AIR)))
    {
        return TRUE;
    }
    return FALSE;
}

static s32 ndsRelocAssetIsMenu(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_MN_COMMON:
    case NDS_RELOC_ASSET_MN_TITLE:
    case NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM:
    case NDS_RELOC_ASSET_MN_VS_MODE:
    case NDS_RELOC_ASSET_MN_PLAYERS_COMMON:
    case NDS_RELOC_ASSET_MN_PLAYERS_GAME_MODES:
    case NDS_RELOC_ASSET_MN_PLAYERS_PORTRAITS:
    case NDS_RELOC_ASSET_FT_EMBLEM_SPRITES:
    case NDS_RELOC_ASSET_MN_SELECT_COMMON:
    case NDS_RELOC_ASSET_MN_PLAYERS_SPOTLIGHT:
    case NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLACK:
    case NDS_RELOC_ASSET_MN_MAPS:
    case NDS_RELOC_ASSET_MN_COMMON_FONTS:
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsRelocAssetIsOpening(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_N64_LOGO:
    case NDS_RELOC_ASSET_MV_COMMON:
    case NDS_RELOC_ASSET_OPENING_COMMON:
    case NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE1:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE2:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE3:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE4:
    case NDS_RELOC_ASSET_OPENING_RUN:
    case NDS_RELOC_ASSET_OPENING_YAMABUKI:
    case NDS_RELOC_ASSET_OPENING_SECTOR:
    case NDS_RELOC_ASSET_OPENING_RUN_CRASH:
    case NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER:
    case NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1:
    case NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2:
        return TRUE;
    default:
        return FALSE;
    }
}

static void ndsRelocUpdateMemoryLedger(void)
{
    u32 scene = (u32)gSCManagerSceneData.scene_curr;
    u32 capacity = (u32)ndsTaskmanArenaSize();
    u32 used = gNdsTaskmanGeneralHeapUsed;
    u32 i;

    if ((gSYTaskmanGeneralHeap.start != NULL) &&
        (gSYTaskmanGeneralHeap.ptr != NULL) &&
        ((uintptr_t)gSYTaskmanGeneralHeap.ptr >=
         (uintptr_t)gSYTaskmanGeneralHeap.start))
    {
        used = (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                     (uintptr_t)gSYTaskmanGeneralHeap.start);
    }

    gNdsMemoryLedgerResult = 0;
    gNdsMemoryLedgerScene = scene;
    gNdsMemoryLedgerGeneration = sNdsRelocSceneGeneration;
    gNdsMemoryLedgerArenaCapacity = capacity;
    gNdsMemoryLedgerArenaUsed = used;
    if (used > gNdsMemoryLedgerArenaHighWater)
    {
        gNdsMemoryLedgerArenaHighWater = used;
    }
    gNdsMemoryLedgerArenaHeadroom = (capacity > used) ? (capacity - used) : 0u;
    gNdsMemoryLedgerDLBytes =
        gNdsTaskmanContexts * (u32)((sizeof(Gfx) * 7680u) +
                                    (sizeof(Gfx) * 2560u));
    gNdsMemoryLedgerGraphicsBytes =
        gNdsTaskmanContexts * gNdsTaskmanGraphicsHeapSize;
    gNdsMemoryLedgerRdpBytes = gNdsTaskmanRdpBufferSize;
    gNdsMemoryLedgerFigatreeHeapSize = (u32)gFTManagerFigatreeHeapSize;
    gNdsMemoryLedgerRelocFiles = sNdsRelocLoadedFileCount;
    gNdsMemoryLedgerRelocBytes = 0u;
    gNdsMemoryLedgerRelocStageBytes = 0u;
    gNdsMemoryLedgerRelocFighterBytes = 0u;
    gNdsMemoryLedgerRelocInterfaceBytes = 0u;
    gNdsMemoryLedgerRelocMenuBytes = 0u;
    gNdsMemoryLedgerRelocOpeningBytes = 0u;
    gNdsMemoryLedgerRelocOtherBytes = 0u;
    gNdsMemoryLedgerRelocStaleFiles = 0u;
    gNdsMemoryLedgerRelocStaleBytes = 0u;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        NDSRelocLoadedFile *loaded = &sNdsRelocLoadedFiles[i];
        u32 bytes = loaded->data_size;

        gNdsMemoryLedgerRelocBytes += bytes;
        if (loaded->owner_scene != scene)
        {
            gNdsMemoryLedgerRelocStaleFiles++;
            gNdsMemoryLedgerRelocStaleBytes += bytes;
        }
        if (ndsRelocAssetIsInterface(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocInterfaceBytes += bytes;
        }
        else if (ndsRelocAssetIsStage(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocStageBytes += bytes;
        }
        else if (ndsRelocAssetIsFighter(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocFighterBytes += bytes;
        }
        else if (ndsRelocAssetIsMenu(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocMenuBytes += bytes;
        }
        else if (ndsRelocAssetIsOpening(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocOpeningBytes += bytes;
        }
        else
        {
            gNdsMemoryLedgerRelocOtherBytes += bytes;
        }
    }

    if ((gNdsMemoryLedgerArenaHeadroom >=
         NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES) &&
        (gNdsMemoryLedgerRelocStaleFiles == 0u) &&
        (gNdsMemoryLedgerRelocStaleBytes == 0u))
    {
        gNdsMemoryLedgerResult = NDS_MEMORY_LEDGER_PASS;
    }
}

static NDSRelocLoadedFile *ndsRelocFindLoadedFileByAsset(u32 asset_id)
{
    u32 i;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (sNdsRelocLoadedFiles[i].asset_id == asset_id)
        {
            return &sNdsRelocLoadedFiles[i];
        }
    }
    return NULL;
}

static NDSRelocLoadedFile *ndsRelocFindLoadedFileByData(void *file)
{
    u32 i;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (sNdsRelocLoadedFiles[i].data == file)
        {
            return &sNdsRelocLoadedFiles[i];
        }
    }
    return NULL;
}

static s32 ndsRelocRangeInLoadedFile(const NDSRelocLoadedFile *loaded,
                                      uintptr_t offset, size_t size)
{
    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if ((offset > loaded->data_size) ||
        (size > (size_t)(loaded->data_size - offset)))
    {
        return FALSE;
    }
    return TRUE;
}

static void *ndsRelocFindStatusNode(LBFileNode *nodes, s32 count, u32 token)
{
    s32 i;

    for (i = 0; i < count; i++)
    {
        if (nodes[i].id == token)
        {
            return nodes[i].addr;
        }
    }
    return NULL;
}

static void ndsRelocAddStatusNode(LBFileNode *nodes, s32 *count,
                                  s32 max_count, u32 token, void *addr)
{
    if ((nodes == NULL) || (count == NULL) || (addr == NULL) ||
        (max_count <= 0))
    {
        return;
    }
    if (ndsRelocFindStatusNode(nodes, *count, token) != NULL)
    {
        return;
    }
    if (*count >= max_count)
    {
        gNdsRelocAssetOpenFailCount++;
        return;
    }
    nodes[*count].id = token;
    nodes[*count].addr = addr;
    (*count)++;
}

static void ndsRelocSetStatusNode(LBFileNode *nodes, s32 *count,
                                  s32 max_count, u32 token, void *addr)
{
    s32 i;

    if ((nodes == NULL) || (count == NULL) || (addr == NULL))
    {
        return;
    }
    for (i = 0; i < *count; i++)
    {
        if (nodes[i].id == token)
        {
            nodes[i].addr = addr;
            return;
        }
    }
    ndsRelocAddStatusNode(nodes, count, max_count, token, addr);
}

static void ndsRelocAddStatusBufferFile(u32 token, void *addr)
{
    ndsRelocAddStatusNode(sNdsRelocStatusBuffer, &sNdsRelocStatusBufferCount,
                          sNdsRelocStatusBufferMax, token, addr);
}

static void ndsRelocSetStatusBufferFile(u32 token, void *addr)
{
    ndsRelocSetStatusNode(sNdsRelocStatusBuffer, &sNdsRelocStatusBufferCount,
                          sNdsRelocStatusBufferMax, token, addr);
}

static void ndsRelocSetForceStatusBufferFile(u32 token, void *addr)
{
    ndsRelocSetStatusNode(sNdsRelocForceStatusBuffer,
                          &sNdsRelocForceStatusBufferCount,
                          sNdsRelocForceStatusBufferMax, token, addr);
}

static s32 ndsRelocPointerRangeInLoadedFile(const NDSRelocLoadedFile *loaded,
                                             const void *ptr, size_t size)
{
    uintptr_t base;
    uintptr_t addr;

    if ((loaded == NULL) || (loaded->data == NULL) || (ptr == NULL))
    {
        return FALSE;
    }

    base = (uintptr_t)loaded->data;
    addr = (uintptr_t)ptr;
    if ((addr < base) || (addr > (base + loaded->data_size)))
    {
        return FALSE;
    }
    return ndsRelocRangeInLoadedFile(loaded, addr - base, size);
}

static NDSRelocLoadedFile *ndsRelocFindLoadedFileContaining(const void *ptr,
                                                             size_t size)
{
    static u32 last_loaded_file_index = 0xffffffffu;
    u32 i;

    if ((last_loaded_file_index < sNdsRelocLoadedFileCount) &&
        (ndsRelocPointerRangeInLoadedFile(
             &sNdsRelocLoadedFiles[last_loaded_file_index],
             ptr,
             size) != FALSE))
    {
        return &sNdsRelocLoadedFiles[last_loaded_file_index];
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                             ptr,
                                             size) != FALSE)
        {
            last_loaded_file_index = i;
            return &sNdsRelocLoadedFiles[i];
        }
    }
    return NULL;
}

s32 ndsRelocPointerRangeInLoadedFiles(const void *ptr, size_t size)
{
    return (ndsRelocFindLoadedFileContaining(ptr, size) != NULL) ? TRUE :
                                                                  FALSE;
}

static size_t ndsRelocStatusNodeDataSize(const LBFileNode *node)
{
    NDSRelocLoadedFile *loaded;
    u32 asset_id;

    if ((node == NULL) || (node->addr == NULL))
    {
        return 0u;
    }

    loaded = ndsRelocFindLoadedFileByData(node->addr);
    if (loaded != NULL)
    {
        return loaded->data_size;
    }

    asset_id = ndsRelocAssetIDForToken((u32)node->id);
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if (loaded != NULL)
    {
        return loaded->data_size;
    }

    loaded = ndsRelocFindLoadedFileByAsset((u32)node->id);
    return (loaded != NULL) ? loaded->data_size : 0u;
}

static s32 ndsRelocPointerRangeInBuffer(const void *base, size_t data_size,
                                        const void *ptr, size_t size)
{
    uintptr_t start;
    uintptr_t addr;

    if ((base == NULL) || (ptr == NULL))
    {
        return FALSE;
    }

    start = (uintptr_t)base;
    addr = (uintptr_t)ptr;
    if ((addr < start) || (addr > (start + data_size)))
    {
        return FALSE;
    }
    return (((addr - start) <= data_size) &&
            (size <= (size_t)(data_size - (addr - start)))) ? TRUE : FALSE;
}

static s32 ndsRelocFindStatusNodeContaining(LBFileNode *nodes, s32 count,
                                            const void *ptr, size_t size,
                                            const void **out_base,
                                            size_t *out_size)
{
    s32 i;

    for (i = 0; i < count; i++)
    {
        size_t data_size = ndsRelocStatusNodeDataSize(&nodes[i]);

        if ((data_size != 0u) &&
            (ndsRelocPointerRangeInBuffer(nodes[i].addr, data_size, ptr,
                                          size) != FALSE))
        {
            if (out_base != NULL)
            {
                *out_base = nodes[i].addr;
            }
            if (out_size != NULL)
            {
                *out_size = data_size;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsRelocFindKnownFileContaining(const void *ptr, size_t size,
                                           const void **out_base,
                                           size_t *out_size)
{
    NDSRelocLoadedFile *loaded = ndsRelocFindLoadedFileContaining(ptr, size);

    if (loaded != NULL)
    {
        if (out_base != NULL)
        {
            *out_base = loaded->data;
        }
        if (out_size != NULL)
        {
            *out_size = loaded->data_size;
        }
        return TRUE;
    }
    if (ndsRelocFindStatusNodeContaining(sNdsRelocForceStatusBuffer,
                                         sNdsRelocForceStatusBufferCount,
                                         ptr, size, out_base, out_size) != FALSE)
    {
        return TRUE;
    }
    return ndsRelocFindStatusNodeContaining(sNdsRelocStatusBuffer,
                                            sNdsRelocStatusBufferCount,
                                            ptr, size, out_base, out_size);
}

void *ndsRelocResolvePointerFromFileBase(const void *file_base,
                                         const void *ptr,
                                         size_t size)
{
    const void *base = NULL;
    size_t data_size = 0u;
    uintptr_t raw;

    if (ptr == NULL)
    {
        return NULL;
    }
    if (ndsRelocFindKnownFileContaining(ptr, size, NULL, NULL) != FALSE)
    {
        return (void *)ptr;
    }
    if (ndsRelocFindKnownFileContaining(file_base, 1u, &base, &data_size) ==
        FALSE)
    {
        return NULL;
    }

    raw = (uintptr_t)ptr;
    if ((raw > data_size) || (size > (size_t)(data_size - raw)))
    {
        return NULL;
    }
    return (u8 *)base + raw;
}

static NDSRelocLoadedFile *ndsRelocRegisterLoadedFile(u32 asset_id, u32 bit,
                                                       void *data,
                                                       const NDSRelocAssetHeader *header)
{
    NDSRelocLoadedFile *loaded;

    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if (loaded == NULL)
    {
        if (sNdsRelocLoadedFileCount >= NDS_RELOC_LOADED_FILE_CAPACITY)
        {
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return NULL;
        }
        loaded = &sNdsRelocLoadedFiles[sNdsRelocLoadedFileCount++];
    }

    loaded->asset_id = asset_id;
    loaded->bit = bit;
    loaded->data = data;
    loaded->data_size = header->data_size;
    loaded->owner_scene = (u32)gSCManagerSceneData.scene_curr;
    loaded->owner_generation = sNdsRelocSceneGeneration;
    loaded->reloc_intern_offset = header->reloc_intern_offset;
    loaded->reloc_extern_offset = header->reloc_extern_offset;
    loaded->extern_count = 0;
    loaded->external_fixup_count = 0;
    loaded->external_fixup_fail_count = 0;
    loaded->internal_fixup_count = 0;
    loaded->internal_fixups_applied = FALSE;
    loaded->external_fixups_applied = FALSE;
    loaded->format_fixups_applied = FALSE;
    loaded->fixups_applying = FALSE;

    if (header->extern_file_ids_num > 0u)
    {
        if ((header->extern_file_ids_num > NDS_RELOC_EXTERN_FILE_ID_CAPACITY) ||
            (ndsRelocAssetReadExternFileIDs(asset_id,
                                            loaded->extern_file_ids,
                                            NDS_RELOC_EXTERN_FILE_ID_CAPACITY,
                                            &loaded->extern_count) == FALSE))
        {
            loaded->extern_count = 0;
            ndsRelocRecordExternalFixupFail(asset_id);
        }
    }

    return loaded;
}

static s32 ndsRelocApplyWordByteSwap(NDSRelocLoadedFile *loaded)
{
    u32 words;
    u32 i;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        gNdsOpeningRoomRelocWordSwapFailCount++;
        return FALSE;
    }

    words = loaded->data_size / sizeof(u32);
    for (i = 0; i < words; i++)
    {
        void *word = (u8 *)loaded->data + (i * sizeof(u32));

        ndsRelocWriteNative32(word, ndsRelocReadBe32(word));
    }

    if (loaded->asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        gNdsStartupLogoRelocWordSwapCount += words;
    }
    else if (ndsRelocIsOpeningRoomAsset(loaded->asset_id) != FALSE)
    {
        gNdsOpeningRoomRelocWordSwapCount += words;
    }
    return TRUE;
}

static s32 ndsRelocApplyInternalPointerFixups(NDSRelocLoadedFile *loaded)
{
    u16 reloc_intern;
    u32 guard;
    u32 fixed_count = 0;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        gNdsOpeningRoomRelocPointerFixupFailCount++;
        return FALSE;
    }
    if (loaded->internal_fixups_applied != FALSE)
    {
        return TRUE;
    }

    reloc_intern = loaded->reloc_intern_offset;
    guard = (loaded->data_size / sizeof(u32)) + 1u;

    while (reloc_intern != 0xffffu)
    {
        uintptr_t slot_offset = (uintptr_t)reloc_intern * sizeof(u32);
        u32 reloc_word;
        u16 next_reloc;
        u16 target_words;
        uintptr_t target_offset;
        void *slot;
        void *target;

        if ((guard == 0) || ((slot_offset + sizeof(u32)) > loaded->data_size))
        {
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return FALSE;
        }
        guard--;

        slot = (u8 *)loaded->data + slot_offset;
        reloc_word = ndsRelocReadNative32(slot);
        next_reloc = (u16)(reloc_word >> 16);
        target_words = (u16)(reloc_word & 0xffffu);
        target_offset = (uintptr_t)target_words * sizeof(u32);

        if (target_offset >= loaded->data_size)
        {
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return FALSE;
        }

        target = (u8 *)loaded->data + target_offset;
        ndsRelocWriteNativePointer(slot, target);

        fixed_count++;
        reloc_intern = next_reloc;
    }

    loaded->internal_fixup_count = fixed_count;
    loaded->internal_fixups_applied = TRUE;

    if (loaded->asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        gNdsStartupLogoRelocPointerFixupCount += fixed_count;
    }
    else if (ndsRelocIsOpeningRoomAsset(loaded->asset_id) != FALSE)
    {
        gNdsOpeningRoomRelocPointerFixupCount += fixed_count;
    }
    if (ndsPupupuStageAssetBit(loaded->asset_id) != 0u)
    {
        gNdsStagePupupuInternalFixupCount += fixed_count;
    }
    return TRUE;
}

static s32 ndsRelocIsFighterAObj16Asset(u32 asset_id)
{
    return ndsRelocIsMarioFoxNaturalCombatAnimID(asset_id);
}

static void ndsRelocRemoveStatusNodeAt(LBFileNode *nodes, s32 *count, s32 index)
{
    s32 remaining;

    if ((nodes == NULL) || (count == NULL) || (index < 0) || (index >= *count))
    {
        return;
    }

    remaining = (*count - index) - 1;
    if (remaining > 0)
    {
        memmove(&nodes[index], &nodes[index + 1],
                (size_t)remaining * sizeof(nodes[0]));
    }
    (*count)--;
}

static void ndsRelocRemoveFighterAObj16StatusAliases(LBFileNode *nodes,
                                                     s32 *count,
                                                     u32 asset_id,
                                                     void *data)
{
    s32 i = 0;

    if ((nodes == NULL) || (count == NULL) || (data == NULL))
    {
        return;
    }

    while (i < *count)
    {
        u32 node_asset_id = ndsRelocAssetIDForToken((u32)nodes[i].id);

        if ((nodes[i].addr == data) && (node_asset_id != asset_id) &&
            (ndsRelocIsFighterAObj16Asset(node_asset_id) != FALSE))
        {
            ndsRelocRemoveStatusNodeAt(nodes, count, i);
            continue;
        }
        i++;
    }
}

static void ndsRelocRemoveFighterAObj16LoadedAliases(u32 asset_id, void *data)
{
    u32 i = 0;

    if (data == NULL)
    {
        return;
    }

    while (i < sNdsRelocLoadedFileCount)
    {
        NDSRelocLoadedFile *loaded = &sNdsRelocLoadedFiles[i];

        if ((loaded->data == data) && (loaded->asset_id != asset_id) &&
            (ndsRelocIsFighterAObj16Asset(loaded->asset_id) != FALSE))
        {
            u32 remaining = (sNdsRelocLoadedFileCount - i) - 1u;

            if (remaining > 0u)
            {
                memmove(&sNdsRelocLoadedFiles[i],
                        &sNdsRelocLoadedFiles[i + 1u],
                        (size_t)remaining * sizeof(sNdsRelocLoadedFiles[0]));
            }
            sNdsRelocLoadedFileCount--;
            continue;
        }
        i++;
    }

    ndsRelocRemoveFighterAObj16StatusAliases(sNdsRelocStatusBuffer,
                                             &sNdsRelocStatusBufferCount,
                                             asset_id, data);
    ndsRelocRemoveFighterAObj16StatusAliases(sNdsRelocForceStatusBuffer,
                                             &sNdsRelocForceStatusBufferCount,
                                             asset_id, data);
}

static u16 ndsRelocReadNative16(const void *addr)
{
    u16 value;

    memcpy(&value, addr, sizeof(value));
    return value;
}

static void ndsRelocWriteNative16(void *addr, u16 value)
{
    memcpy(addr, &value, sizeof(value));
}

static u16 ndsRelocAObj16EncodeForNativeBitfields(u16 source)
{
    u16 opcode = (u16)((source >> 11) & 0x1fu);
    u16 flags = (u16)((source >> 1) & 0x3ffu);
    u16 toggle = (u16)(source & 1u);

    return (u16)(opcode | (flags << 5) | (toggle << 15));
}

static u32 ndsRelocAObj16FlagCount(u16 flags)
{
    u32 count = 0;

    while (flags != 0u)
    {
        if ((flags & 1u) != 0u)
        {
            count++;
        }
        flags = (u16)(flags >> 1);
    }
    return count;
}

static u32 ndsRelocAObj16CommandWords(u16 opcode, u16 flags, u16 toggle)
{
    u32 words = 1u;
    u32 flagged = ndsRelocAObj16FlagCount(flags);

    if (toggle != 0u)
    {
        words++;
    }

    switch (opcode)
    {
    case nGCAnimEvent16SetValBlock:
    case nGCAnimEvent16SetVal:
    case nGCAnimEvent16SetVal0RateBlock:
    case nGCAnimEvent16SetVal0Rate:
    case nGCAnimEvent16SetValAfterBlock:
    case nGCAnimEvent16SetValAfter:
        words += flagged;
        break;

    case nGCAnimEvent16SetValRateBlock:
    case nGCAnimEvent16SetValRate:
        words += flagged * 2u;
        break;

    case nGCAnimEvent16SetTargetRate:
        words += flagged;
        break;

    case nGCAnimEvent16Loop:
    case nGCAnimEvent16SetTranslateInterp:
        words = 2u;
        break;

    default:
        break;
    }
    return words;
}

static void ndsRelocNormalizeAObj16Script(u16 *script, u32 word_count)
{
    u32 index = 0;
    u32 guard = word_count;

    while ((index < word_count) && (guard != 0u))
    {
        u16 source = script[index];
        u16 opcode = (u16)((source >> 11) & 0x1fu);
        u16 flags = (u16)((source >> 1) & 0x3ffu);
        u16 toggle = (u16)(source & 1u);
        u32 step = ndsRelocAObj16CommandWords(opcode, flags, toggle);

        script[index] = ndsRelocAObj16EncodeForNativeBitfields(source);
        if (opcode == nGCAnimEvent16End)
        {
            return;
        }
        if ((step == 0u) || ((index + step) > word_count))
        {
            return;
        }
        index += step;
        guard--;
    }
}

static s32 ndsRelocNormalizeFighterAObj16File(NDSRelocLoadedFile *loaded)
{
    uintptr_t base;
    uintptr_t table_bytes;
    u32 i;

    if ((loaded == NULL) || (loaded->data == NULL) ||
        (ndsRelocIsFighterAObj16Asset(loaded->asset_id) == FALSE))
    {
        return TRUE;
    }
    if (loaded->format_fixups_applied != FALSE)
    {
        return TRUE;
    }

    base = (uintptr_t)loaded->data;
    table_bytes = loaded->data_size;
    for (i = 0; ((i * sizeof(u32)) < table_bytes) &&
                (((i + 1u) * sizeof(u32)) <= loaded->data_size); i++)
    {
        uintptr_t value =
            (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                            (i * sizeof(u32)));

        if ((value >= base) && ((value - base) < table_bytes))
        {
            table_bytes = value - base;
        }
    }
    if ((table_bytes == 0u) || (table_bytes >= loaded->data_size) ||
        ((table_bytes % sizeof(u32)) != 0u))
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    for (i = (u32)table_bytes; (i + sizeof(u32)) <= loaded->data_size;
         i += sizeof(u32))
    {
        u16 first = ndsRelocReadNative16((u8 *)loaded->data + i);
        u16 second = ndsRelocReadNative16((u8 *)loaded->data + i + sizeof(u16));

        ndsRelocWriteNative16((u8 *)loaded->data + i, second);
        ndsRelocWriteNative16((u8 *)loaded->data + i + sizeof(u16), first);
    }

    for (i = 0; (i * sizeof(u32)) < table_bytes; i++)
    {
        uintptr_t value =
            (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                            (i * sizeof(u32)));

        if ((value >= (base + table_bytes)) &&
            ((value - base) < (uintptr_t)loaded->data_size))
        {
            u32 script_offset = (u32)(value - base);
            uintptr_t script_end = base + loaded->data_size;
            u32 j;
            u32 word_count;

            for (j = 0; (j * sizeof(u32)) < table_bytes; j++)
            {
                uintptr_t next =
                    (uintptr_t)ndsRelocReadNative32((u8 *)loaded->data +
                                                    (j * sizeof(u32)));

                if ((next > value) && (next < script_end))
                {
                    script_end = next;
                }
            }
            word_count = (u32)((script_end - value) / sizeof(u16));

            ndsRelocNormalizeAObj16Script((u16 *)((u8 *)loaded->data +
                                                  script_offset),
                                          word_count);
        }
    }

    loaded->format_fixups_applied = TRUE;
    return TRUE;
}

static size_t ndsRelocAssetAllocSize(u32 asset_id);
static s32 ndsRelocFinalizeLoadedFile(NDSRelocLoadedFile *loaded);
static size_t ndsRelocExternTreeAllocSize(u32 asset_id, u32 *seen,
                                          u32 *seen_count);
static NDSRelocLoadedFile *ndsRelocLoadExternTreeAsset(u32 asset_id,
                                                       uintptr_t *heap_ptr);

static void *ndsRelocStaticBufferForAsset(u32 asset_id, size_t asset_size)
{
#if NDS_DEV_SCENE_HARNESS != 0
    if ((asset_id == NDS_RELOC_ASSET_STAGE_CASTLE) &&
        (asset_size <= NDS_RELOC_STAGE_CASTLE_STATIC_SIZE))
    {
        return sNdsOpeningActionPreviewFileBuffer;
    }
    if ((asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_113) &&
        (asset_size <= NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE) &&
        ((NDS_RELOC_STAGE_CASTLE_STATIC_SIZE +
          NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE) <=
         sizeof(sNdsOpeningActionPreviewFileBuffer)))
    {
        return &sNdsOpeningActionPreviewFileBuffer[
            NDS_RELOC_STAGE_CASTLE_STATIC_SIZE];
    }
#else
    (void)asset_id;
    (void)asset_size;
#endif
    return NULL;
}

static NDSRelocLoadedFile *ndsRelocEnsureLoadedAsset(u32 asset_id)
{
    NDSRelocLoadedFile *loaded;
    size_t asset_size;
    NDSRelocAssetHeader header;
    void *heap;

    ndsRelocPrepareSceneCache();
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);

    if (loaded != NULL)
    {
        if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
        {
            return NULL;
        }
        return loaded;
    }

    asset_size = ndsRelocAssetAllocSize(asset_id);
    if (asset_size == 0)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    heap = ndsRelocStaticBufferForAsset(asset_id, asset_size);
    if (heap == NULL)
    {
        heap = syTaskmanMalloc(asset_size, 0x10);
    }
    if (heap == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if (ndsRelocAssetLoadData(asset_id, heap, asset_size, &header) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
    if (loaded == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if (ndsRelocApplyWordByteSwap(loaded) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
    {
        return NULL;
    }
    return loaded;
}

static s32 ndsRelocApplyExternalPointerFixups(NDSRelocLoadedFile *loaded)
{
    u16 reloc_extern;
    u32 guard;
    u32 extern_index = 0;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if (loaded->external_fixups_applied != FALSE)
    {
        return TRUE;
    }
    if (loaded->reloc_extern_offset == 0xffffu)
    {
        loaded->external_fixups_applied = TRUE;
        return TRUE;
    }
    if (loaded->extern_count == 0u)
    {
        loaded->external_fixup_fail_count++;
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    reloc_extern = loaded->reloc_extern_offset;
    guard = (loaded->data_size / sizeof(u32)) + 1u;

    while (reloc_extern != 0xffffu)
    {
        uintptr_t slot_offset = (uintptr_t)reloc_extern * sizeof(u32);
        u32 reloc_word;
        u16 next_reloc;
        u16 target_words;
        uintptr_t target_offset;
        u32 dep_asset_id;
        NDSRelocLoadedFile *dep;

        if ((guard == 0) ||
            ((slot_offset + sizeof(u32)) > loaded->data_size) ||
            (extern_index >= loaded->extern_count))
        {
            loaded->external_fixup_fail_count++;
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }
        guard--;

        reloc_word = ndsRelocReadNative32((u8 *)loaded->data + slot_offset);
        next_reloc = (u16)(reloc_word >> 16);
        target_words = (u16)(reloc_word & 0xffffu);
        target_offset = (uintptr_t)target_words * sizeof(u32);
        dep_asset_id = ndsRelocAssetIDForToken(loaded->extern_file_ids[extern_index++]);

        dep = ndsRelocEnsureLoadedAsset(dep_asset_id);
        if ((dep == NULL) || (target_offset >= dep->data_size))
        {
            loaded->external_fixup_fail_count++;
            ndsRelocRecordExternalFixupFail(loaded->asset_id);
            return FALSE;
        }

        ndsRelocWriteNativePointer((u8 *)loaded->data + slot_offset,
                                   (u8 *)dep->data + target_offset);

        loaded->external_fixup_count++;
        ndsRelocRecordExternalFixupSuccess(loaded->asset_id, dep->asset_id);

        reloc_extern = next_reloc;
    }

    loaded->external_fixups_applied = TRUE;
    return TRUE;
}

static s32 ndsRelocFinalizeLoadedFile(NDSRelocLoadedFile *loaded)
{
    if (loaded == NULL)
    {
        return FALSE;
    }
    if (loaded->fixups_applying != FALSE)
    {
        return TRUE;
    }

    loaded->fixups_applying = TRUE;
    if ((ndsRelocApplyInternalPointerFixups(loaded) == FALSE) ||
        (ndsRelocNormalizeFighterAObj16File(loaded) == FALSE) ||
        (ndsRelocApplyExternalPointerFixups(loaded) == FALSE))
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
    loaded->fixups_applying = FALSE;

    if (ndsPupupuStageAssetBit(loaded->asset_id) != 0u)
    {
        gNdsStagePupupuRelocAssetMask |=
            ndsPupupuStageAssetBit(loaded->asset_id);
        gNdsStagePupupuRelocDependencyMask |=
            ndsPupupuStageAssetBit(loaded->asset_id);
        if ((gNdsStagePupupuRelocAssetMask & 0x1fu) == 0x1fu)
        {
            gNdsStagePupupuRelocResult = NDS_STAGE_PUPUPU_RELOC_PASS;
        }
    }
    if (ndsFighterMarioFoxAssetBit(loaded->asset_id) != 0u)
    {
        gNdsFighterMarioFoxRelocAssetMask |=
            ndsFighterMarioFoxAssetBit(loaded->asset_id);
        gNdsFighterMarioFoxRelocDependencyMask |=
            ndsFighterMarioFoxAssetBit(loaded->asset_id);
        if ((gNdsFighterMarioFoxRelocAssetMask &
             NDS_FIGHTER_MARIOFOX_FILE_MASK) ==
            NDS_FIGHTER_MARIOFOX_FILE_MASK)
        {
            gNdsFighterMarioFoxRelocResult =
                NDS_FIGHTER_MARIOFOX_RELOC_PASS;
            gNdsFighterMarioFoxSetupMask |=
                NDS_FIGHTER_MARIOFOX_SETUP_FILES;
        }
    }
    return TRUE;
}

static void ndsRelocSwapS16Pair(s16 *a, s16 *b)
{
    s16 tmp = *a;

    *a = *b;
    *b = tmp;
}

static void ndsRelocNormalizeS16Range(s16 *min_value, s16 *max_value)
{
    if ((min_value != NULL) &&
        (max_value != NULL) &&
        (*min_value > *max_value))
    {
        ndsRelocSwapS16Pair(min_value, max_value);
    }
}

static void ndsRelocNormalizeGroundDataBounds(MPGroundData *ground_data)
{
    if (ground_data == NULL)
    {
        return;
    }
    ndsRelocNormalizeS16Range(&ground_data->camera_bound_bottom,
                              &ground_data->camera_bound_top);
    ndsRelocNormalizeS16Range(&ground_data->camera_bound_left,
                              &ground_data->camera_bound_right);
    ndsRelocNormalizeS16Range(&ground_data->map_bound_bottom,
                              &ground_data->map_bound_top);
    ndsRelocNormalizeS16Range(&ground_data->map_bound_left,
                              &ground_data->map_bound_right);
    ndsRelocNormalizeS16Range(&ground_data->camera_bound_team_bottom,
                              &ground_data->camera_bound_team_top);
    ndsRelocNormalizeS16Range(&ground_data->camera_bound_team_left,
                              &ground_data->camera_bound_team_right);
    ndsRelocNormalizeS16Range(&ground_data->map_bound_team_bottom,
                              &ground_data->map_bound_team_top);
    ndsRelocNormalizeS16Range(&ground_data->map_bound_team_left,
                              &ground_data->map_bound_team_right);
}

static void ndsRelocNormalizeGroundMapHeader(NDSRelocLoadedFile *loaded,
                                             u32 offset)
{
    if ((loaded == NULL) ||
        (loaded->data == NULL) ||
        ((offset + sizeof(MPGroundData)) > loaded->data_size))
    {
        return;
    }
    ndsRelocNormalizeGroundDataBounds(
        (MPGroundData *)((u8 *)loaded->data + offset));
}

static void ndsRelocNormalizeGroundMapAsset(NDSRelocLoadedFile *loaded)
{
    if (loaded == NULL)
    {
        return;
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP)
    {
        ndsRelocNormalizeGroundMapHeader(
            loaded,
            NDS_RELOC_SYMBOL_GR_PUPUPU_MAP_HEADER);
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_GR_HYRULE_MAP)
    {
        ndsRelocNormalizeGroundMapHeader(
            loaded,
            NDS_RELOC_SYMBOL_GR_HYRULE_MAP_HEADER);
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_GR_INISHIE_MAP)
    {
        ndsRelocNormalizeGroundMapHeader(
            loaded,
            NDS_RELOC_SYMBOL_GR_INISHIE_MAP_HEADER);
    }
}

static void ndsRelocSwapSpriteAttrZDepth(Sprite *sprite)
{
    u16 attr = sprite->attr;

    sprite->attr = (u16)sprite->zdepth;
    sprite->zdepth = (s16)attr;
}

static void ndsRelocReverseSpriteColorBytes(Sprite *sprite)
{
    u8 red = sprite->red;
    u8 green = sprite->green;
    u8 blue = sprite->blue;
    u8 alpha = sprite->alpha;

    sprite->red = alpha;
    sprite->green = blue;
    sprite->blue = green;
    sprite->alpha = red;
}

static void ndsRelocNormalizeSpriteHeaderFields(Sprite *sprite, u8 bmfmt,
                                                u8 bmsiz)
{
    ndsRelocSwapS16Pair(&sprite->x, &sprite->y);
    ndsRelocSwapS16Pair(&sprite->width, &sprite->height);
    ndsRelocSwapS16Pair(&sprite->expx, &sprite->expy);
    ndsRelocSwapSpriteAttrZDepth(sprite);
    ndsRelocReverseSpriteColorBytes(sprite);
    ndsRelocSwapS16Pair(&sprite->startTLUT, &sprite->nTLUT);
    ndsRelocSwapS16Pair(&sprite->istart, &sprite->istep);
    ndsRelocSwapS16Pair(&sprite->nbitmaps, &sprite->ndisplist);
    ndsRelocSwapS16Pair(&sprite->bmheight, &sprite->bmHreal);

    /* The blanket u32 endian pass shifts these 8-bit format fields into the
     * padding before the bitmap pointer. Keep correction tied to known Sprite
     * manifests rather than guessing across every relocated resource. */
    sprite->bmfmt = bmfmt;
    sprite->bmsiz = bmsiz;
}

static s32 ndsRelocNormalizeSpriteBitmapTable(NDSRelocLoadedFile *loaded,
                                               Sprite *sprite,
                                               u32 bitmap_count)
{
    Bitmap *bitmap;
    u32 i;

    bitmap = sprite->bitmap;
    if (ndsRelocPointerRangeInLoadedFile(
            loaded, bitmap, sizeof(Bitmap) * bitmap_count) == FALSE)
    {
        return FALSE;
    }

    for (i = 0; i < bitmap_count; i++)
    {
        ndsRelocSwapS16Pair(&bitmap[i].width, &bitmap[i].width_img);
        ndsRelocSwapS16Pair(&bitmap[i].s, &bitmap[i].t);
        ndsRelocSwapS16Pair(&bitmap[i].actualHeight, &bitmap[i].LUToffset);
    }
    return TRUE;
}

static void ndsRelocNormalizeN64LogoSprite(NDSRelocLoadedFile *loaded)
{
    Sprite *sprite;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO) ||
        (ndsRelocRangeInLoadedFile(loaded, NDS_RELOC_SYMBOL_N64_LOGO_SPRITE,
                                   sizeof(Sprite)) == FALSE))
    {
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data +
                        NDS_RELOC_SYMBOL_N64_LOGO_SPRITE);
    if ((sprite->width == 128) &&
        (sprite->height == 108) &&
        (sprite->nbitmaps == 8) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_16b))
    {
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA, G_IM_SIZ_16b);
    (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite, 8u);
}

static void ndsRelocNormalizeOpeningPortraitsSprite(
    NDSRelocLoadedFile *loaded,
    u32 offset,
    u32 expected_width,
    u32 expected_height,
    u8 bmfmt,
    u8 bmsiz)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsOpeningPortraitsSpriteNormalizeFailCount++;
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if (((u32)(u16)sprite->width == expected_width) &&
        ((u32)(u16)sprite->height == expected_height) &&
        (sprite->bmfmt == bmfmt) &&
        (sprite->bmsiz == bmsiz))
    {
        gNdsOpeningPortraitsSpriteNormalizeCount++;
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, bmfmt, bmsiz);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != expected_width) ||
        ((u32)(u16)sprite->height != expected_height) ||
        (bitmap_count == 0) ||
        (bitmap_count > 128u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsOpeningPortraitsSpriteNormalizeFailCount++;
        return;
    }

    gNdsOpeningPortraitsSpriteNormalizeCount++;
}

static void ndsRelocNormalizeOpeningPortraitsSprites(NDSRelocLoadedFile *loaded)
{
    if (loaded == NULL)
    {
        return;
    }

    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1)
    {
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_SAMUS,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_MARIO,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_FOX,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_PIKACHU,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_COVER,
            NDS_OPENING_PORTRAITS_COVER_WIDTH,
            NDS_OPENING_PORTRAITS_COVER_HEIGHT, G_IM_FMT_I, G_IM_SIZ_4b);
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2)
    {
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_LINK,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_KIRBY,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_DONKEY,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_YOSHI,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
    }

    if (gNdsOpeningPortraitsSpriteNormalizeFailCount == 0)
    {
        gNdsOpeningPortraitsRelocResult = NDS_OPENING_PORTRAITS_RELOC_PASS;
    }
}

static void ndsRelocNormalizeIFAnnounceSprite(NDSRelocLoadedFile *loaded,
                                              u32 offset)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsOpeningMarioSpriteNormalizeFailCount++;
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if ((sprite->bmfmt == G_IM_FMT_IA) &&
        (sprite->bmsiz == G_IM_SIZ_8b) &&
        ((u32)(u16)sprite->width <= NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH) &&
        ((u32)(u16)sprite->height <= NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT) &&
        ((u32)(u16)sprite->nbitmaps != 0) &&
        ((u32)(u16)sprite->nbitmaps <= 16u))
    {
        gNdsOpeningMarioSpriteNormalizeCount++;
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_IA, G_IM_SIZ_8b);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width == 0) ||
        ((u32)(u16)sprite->height == 0) ||
        ((u32)(u16)sprite->width > NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH) ||
        ((u32)(u16)sprite->height > NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT) ||
        (bitmap_count == 0) ||
        (bitmap_count > 16u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsOpeningMarioSpriteNormalizeFailCount++;
        return;
    }

    gNdsOpeningMarioSpriteNormalizeCount++;
}

static void ndsRelocNormalizeIFAnnounceMarioSprites(NDSRelocLoadedFile *loaded)
{
    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE))
    {
        return;
    }

    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_A);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_B);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_C);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_D);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_E);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_F);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_G);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_H);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_I);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y);
}

static s32 ndsRelocNormalizeTitleSprite(NDSRelocLoadedFile *loaded,
                                        const NDSTitleSpriteDesc *desc)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE) ||
        (desc == NULL) ||
        (ndsRelocRangeInLoadedFile(loaded, desc->offset,
                                   sizeof(Sprite)) == FALSE))
    {
        gNdsTitleSpriteNormalizeFailCount++;
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + desc->offset);
    if (((u32)(u16)sprite->width == desc->width) &&
        ((u32)(u16)sprite->height == desc->height) &&
        (sprite->bmfmt == desc->bmfmt) &&
        (sprite->bmsiz == desc->bmsiz))
    {
        gNdsTitleSpriteNormalizeCount++;
        return TRUE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt, desc->bmsiz);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != desc->width) ||
        ((u32)(u16)sprite->height != desc->height) ||
        ((u32)(u16)sprite->width == 0) ||
        ((u32)(u16)sprite->height == 0) ||
        ((u32)(u16)sprite->width > NDS_TITLE_MAX_WIDTH) ||
        ((u32)(u16)sprite->height > NDS_TITLE_MAX_HEIGHT) ||
        (bitmap_count == 0) ||
        (bitmap_count > 128u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsTitleSpriteNormalizeFailCount++;
        return FALSE;
    }

    gNdsTitleSpriteNormalizeCount++;
    return TRUE;
}

static void ndsRelocNormalizeTitleSprites(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsTitleSpriteDescs); i++)
    {
        (void)ndsRelocNormalizeTitleSprite(loaded,
                                           &sNdsTitleSpriteDescs[i]);
    }

    if ((gNdsTitleSpriteNormalizeFailCount == 0) &&
        (gNdsTitleSpriteNormalizeCount >= ARRAY_COUNT(sNdsTitleSpriteDescs)))
    {
        gNdsTitleRelocResult = NDS_TITLE_RELOC_PASS;
    }
}

static s32 ndsRelocNormalizeTitleFireSprite(NDSRelocLoadedFile *loaded,
                                            u32 offset)
{
    Sprite *sprite;
    u32 bitmap_count;
    Bitmap *bitmap;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if (((u32)(u16)sprite->width == 32u) &&
        ((u32)(u16)sprite->height == 32u) &&
        ((u32)(u16)sprite->nbitmaps == 1u) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_32b))
    {
        gNdsTitleFireSpriteNormalizeCount++;
        return TRUE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA, G_IM_SIZ_32b);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != 32u) ||
        ((u32)(u16)sprite->height != 32u) ||
        (bitmap_count != 1u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    bitmap = sprite->bitmap;
    if ((bitmap == NULL) ||
        ((u32)(u16)bitmap->width != 32u) ||
        ((u32)(u16)bitmap->width_img != 32u) ||
        ((u32)(u16)bitmap->actualHeight != 32u))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    gNdsTitleFireSpriteNormalizeCount++;
    return TRUE;
}

static void ndsRelocNormalizeTitleFireSprites(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsTitleFireAnimFrameSymbols); i++)
    {
        (void)ndsRelocNormalizeTitleFireSprite(
            loaded,
            sNdsTitleFireAnimFrameSymbols[i].offset);
    }
}

static s32 ndsRelocNormalizeOpeningActionPreviewSprite(
    NDSRelocLoadedFile *loaded,
    const NDSOpeningActionPreviewDesc *desc)
{
    Sprite *sprite;

    if ((loaded == NULL) || (desc == NULL) ||
        (loaded->asset_id != desc->asset_id) ||
        (ndsRelocRangeInLoadedFile(loaded, desc->offset,
                                   sizeof(Sprite)) == FALSE))
    {
        gNdsOpeningMovieActionPreviewSpriteNormalizeFailCount++;
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + desc->offset);
    if (((u32)(u16)sprite->width == desc->width) &&
        ((u32)(u16)sprite->height == desc->height) &&
        ((u32)(u16)sprite->nbitmaps == desc->bitmap_count) &&
        (sprite->bmfmt == desc->bmfmt) &&
        (sprite->bmsiz == desc->bmsiz))
    {
        gNdsOpeningMovieActionPreviewSpriteNormalizeCount++;
        return TRUE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt, desc->bmsiz);
    if (((u32)(u16)sprite->width != desc->width) ||
        ((u32)(u16)sprite->height != desc->height) ||
        ((u32)(u16)sprite->nbitmaps != desc->bitmap_count) ||
        (desc->width == 0) || (desc->height == 0) ||
        (desc->width > NDS_OPENING_ACTION_PREVIEW_MAX_WIDTH) ||
        (desc->height > NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT) ||
        (desc->bitmap_count == 0) ||
        (desc->bitmap_count > 128u) ||
        (ndsRelocNormalizeSpriteBitmapTable(
            loaded, sprite, desc->bitmap_count) == FALSE))
    {
        gNdsOpeningMovieActionPreviewSpriteNormalizeFailCount++;
        return FALSE;
    }

    gNdsOpeningMovieActionPreviewSpriteNormalizeCount++;
    return TRUE;
}

static void ndsRelocReverseColorPackBytes(SYColorPack *color)
{
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;

    if (color == NULL)
    {
        return;
    }

    red = color->s.r;
    green = color->s.g;
    blue = color->s.b;
    alpha = color->s.a;
    color->s.r = alpha;
    color->s.g = blue;
    color->s.b = green;
    color->s.a = red;
}

static u32 ndsRelocEffectiveMObjSubFlags(const MObjSub *mobjsub)
{
    u32 flags;

    if (mobjsub == NULL)
    {
        return 0;
    }

    flags = mobjsub->flags;
    if (flags == MOBJ_FLAG_NONE)
    {
        return MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA;
    }
    return flags;
}

static s32 ndsRelocMObjSubFlagsKnown(u32 flags)
{
    return (flags &
            ~(MOBJ_FLAG_ALPHA | MOBJ_FLAG_SPLIT | MOBJ_FLAG_PALETTE |
              MOBJ_FLAG_FRAC | MOBJ_FLAG_TEXTURE | MOBJ_FLAG_PRIMCOLOR |
              MOBJ_FLAG_ENVCOLOR | MOBJ_FLAG_BLENDCOLOR |
              MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2 | 0x8u | 0x20u |
              0x40u)) == 0;
}

static s32 ndsRelocMVCommonMObjSubFlagsLookNative(u32 flags)
{
    return (flags == MOBJ_FLAG_NONE) ||
           (flags == MOBJ_FLAG_PRIMCOLOR) ||
           (flags == (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_LIGHT1));
}

static void ndsRelocRecordMObjSubNormalize(
    const NDSRelocLoadedFile *loaded, const MObjSub *mobjsub)
{
    u32 flags;
    u32 effective_flags;

    if (mobjsub == NULL)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }

    flags = mobjsub->flags;
    effective_flags = ndsRelocEffectiveMObjSubFlags(mobjsub);
    if (gNdsOpeningRoomRelocMObjSubNormalizeCount == 0)
    {
        gNdsOpeningRoomRelocMObjSubFirstFlags = flags;
    }
    gNdsOpeningRoomRelocMObjSubNormalizeCount++;
    gNdsOpeningRoomRelocMObjSubSourceResult =
        NDS_OPENING_ROOM_RELOC_MOBJ_SOURCE_PASS;

    if (flags == MOBJ_FLAG_NONE)
    {
        gNdsOpeningRoomRelocMObjSubZeroFlagCount++;
    }
    if ((flags & MOBJ_FLAG_PRIMCOLOR) != 0)
    {
        gNdsOpeningRoomRelocMObjSubPrimColorCount++;
    }
    if ((flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        gNdsOpeningRoomRelocMObjSubLightCount++;
    }
    if ((effective_flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        gNdsOpeningRoomRelocMObjSubTextureFlagCount++;
        if (gNdsOpeningRoomRelocMObjSubFirstTextureOffset == 0xffffffffu)
        {
            if ((loaded != NULL) &&
                (ndsRelocPointerRangeInLoadedFile(
                     loaded, mobjsub, sizeof(*mobjsub)) != FALSE))
            {
                gNdsOpeningRoomRelocMObjSubFirstTextureOffset =
                    (u32)((const u8 *)mobjsub - (const u8 *)loaded->data);
            }
            gNdsOpeningRoomRelocMObjSubFirstTextureFlags = flags;
        }
    }

    if (ndsRelocMObjSubFlagsKnown(flags) == FALSE)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
    }
}

static s32 ndsRelocMObjSubMixedFieldsLookNative(const MObjSub *mobjsub)
{
    if (mobjsub == NULL)
    {
        return FALSE;
    }

    return (ndsRelocMVCommonMObjSubFlagsLookNative(mobjsub->flags) != FALSE) &&
           (mobjsub->fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->siz <= G_IM_SIZ_32b) &&
           (mobjsub->block_fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->block_siz <= G_IM_SIZ_32b);
}

static void ndsRelocNormalizeMObjSubMixedFields(NDSRelocLoadedFile *loaded,
                                                MObjSub *mobjsub)
{
    u16 old_pad00;
    u8 old_fmt;
    u8 old_siz;
    u16 old_flags;
    u8 old_block_fmt;
    u8 old_block_siz;
    u8 old_prim_l;
    u8 old_prim_m;
    u8 old_prim_pad0;
    u8 old_prim_pad1;

    if (mobjsub == NULL)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }

    if (ndsRelocMObjSubMixedFieldsLookNative(mobjsub) != FALSE)
    {
        ndsRelocRecordMObjSubNormalize(loaded, mobjsub);
        return;
    }

    old_pad00 = mobjsub->pad00;
    old_fmt = mobjsub->fmt;
    old_siz = mobjsub->siz;
    old_flags = mobjsub->flags;
    old_block_fmt = mobjsub->block_fmt;
    old_block_siz = mobjsub->block_siz;
    old_prim_l = mobjsub->prim_l;
    old_prim_m = mobjsub->prim_m;
    old_prim_pad0 = mobjsub->prim_pad[0];
    old_prim_pad1 = mobjsub->prim_pad[1];

    mobjsub->pad00 = ((u16)old_siz << 8) | old_fmt;
    mobjsub->fmt = (u8)(old_pad00 >> 8);
    mobjsub->siz = (u8)(old_pad00 & 0xffu);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk08, (s16 *)&mobjsub->unk0A);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk0C, (s16 *)&mobjsub->unk0E);

    mobjsub->flags = ((u16)old_block_siz << 8) | old_block_fmt;
    mobjsub->block_fmt = (u8)(old_flags >> 8);
    mobjsub->block_siz = (u8)(old_flags & 0xffu);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->block_dxt,
                        (s16 *)&mobjsub->unk36);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk38, (s16 *)&mobjsub->unk3A);

    mobjsub->prim_l = old_prim_pad1;
    mobjsub->prim_m = old_prim_pad0;
    mobjsub->prim_pad[0] = old_prim_m;
    mobjsub->prim_pad[1] = old_prim_l;
    ndsRelocReverseColorPackBytes(&mobjsub->primcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->envcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->blendcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->light1color);
    ndsRelocReverseColorPackBytes(&mobjsub->light2color);

    ndsRelocRecordMObjSubNormalize(loaded, mobjsub);
}

static void ndsRelocNormalizeMObjSubTable(NDSRelocLoadedFile *loaded,
                                          u32 offset,
                                          u32 head_count)
{
    MObjSub ***p_mobjsubs;
    u32 head_index;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(*p_mobjsubs)) ==
         FALSE))
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }

    p_mobjsubs = (MObjSub ***)((u8 *)loaded->data + offset);
    for (head_index = 0; head_index < head_count; head_index++)
    {
        MObjSub **mobjsubs;
        u32 list_index;

        if (ndsRelocPointerRangeInLoadedFile(
                loaded, &p_mobjsubs[head_index], sizeof(*p_mobjsubs)) ==
            FALSE)
        {
            gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
            return;
        }

        mobjsubs = p_mobjsubs[head_index];
        if (mobjsubs == NULL)
        {
            continue;
        }
        if (ndsRelocPointerRangeInLoadedFile(
                loaded, mobjsubs, sizeof(*mobjsubs)) == FALSE)
        {
            gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
            return;
        }

        for (list_index = 0; list_index < 8u; list_index++)
        {
            MObjSub *mobjsub;

            if (ndsRelocPointerRangeInLoadedFile(
                    loaded, &mobjsubs[list_index], sizeof(*mobjsubs)) ==
                FALSE)
            {
                gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
                return;
            }

            mobjsub = mobjsubs[list_index];
            if (mobjsub == NULL)
            {
                break;
            }
            if (ndsRelocPointerRangeInLoadedFile(
                    loaded, mobjsub, sizeof(*mobjsub)) == FALSE)
            {
                gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
                return;
            }
            ndsRelocNormalizeMObjSubMixedFields(loaded, mobjsub);
        }
    }
}

static void ndsRelocNormalizeMVCommonMObjSubs(NDSRelocLoadedFile *loaded)
{
    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON))
    {
        return;
    }

    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_MOBJ, 52u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MOBJ, 2u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_AIR_MOBJ, 4u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_GROUND_MOBJ, 2u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_DESK_GROUND_MOBJ, 8u);
    ndsRelocNormalizeMObjSubTable(loaded,
                                  NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MOBJ,
                                  2u);
}

static s32 ndsRelocResolveSymbolOffset(NDSRelocLoadedFile *loaded,
                                        const void *symbol, u32 *out_offset)
{
    uintptr_t raw_symbol = (uintptr_t)symbol;
    u32 i;

    if ((loaded == NULL) || (out_offset == NULL))
    {
        return FALSE;
    }

    if (symbol == &llN64LogoSprite)
    {
        if (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO)
        {
            return FALSE;
        }
        *out_offset = NDS_RELOC_SYMBOL_N64_LOGO_SPRITE;
        return TRUE;
    }
    for (i = 0; i < ARRAY_COUNT(sNdsIFCommonSymbols); i++)
    {
        if ((sNdsIFCommonSymbols[i].asset_id != NDS_RELOC_ASSET_INVALID) &&
            (loaded->asset_id == sNdsIFCommonSymbols[i].asset_id) &&
            (symbol == sNdsIFCommonSymbols[i].symbol))
        {
            *out_offset = sNdsIFCommonSymbols[i].offset;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE)
    {
        if (symbol == &llIFCommonAnnounceCommonLetterASprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_A;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterBSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_B;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterCSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_C;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterDSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_D;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterESprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_E;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterFSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_F;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterGSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_G;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterHSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_H;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterISprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_I;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterKSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterLSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterMSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterNSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterOSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterPSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterRSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterSSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterUSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterXSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterYSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_COMMON)
    {
        if (symbol == &llMVOpeningCommonMarioCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_MARIO_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonDonkeyCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_DONKEY_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonSamusCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_SAMUS_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonFoxCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_FOX_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonLinkCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_LINK_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonYoshiCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_YOSHI_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonPikachuCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_PIKACHU_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonKirbyCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_KIRBY_CAM_ANIM;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1)
    {
        if (symbol == &llMVOpeningPortraitsSet1SamusSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_SAMUS;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1MarioSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_MARIO;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1FoxSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_FOX;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1PikachuSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_PIKACHU;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1CoverSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_COVER;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2)
    {
        if (symbol == &llMVOpeningPortraitsSet2LinkSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_LINK;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet2KirbySprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_KIRBY;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet2DonkeySprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_DONKEY;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet2YoshiSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_YOSHI;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_RUN)
    {
        if (symbol == &llMVOpeningRunWallpaperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_YAMABUKI)
    {
        if (symbol == &llMVOpeningYamabukiWallpaperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_SECTOR)
    {
        if (symbol == &llMVOpeningSectorCockpitSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_TITLE)
    {
        if (symbol == &llMNTitleLogoAnimFullSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_LOGO_FULL;
            return TRUE;
        }
        if (symbol == &llMNTitleBorderUpperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_BORDER_UPPER;
            return TRUE;
        }
        if (symbol == &llMNTitleTMSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_TM;
            return TRUE;
        }
        if (symbol == &llMNTitleCutoutSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_CUTOUT;
            return TRUE;
        }
        if (symbol == &llMNTitleTMUnkSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_TM_UNK;
            return TRUE;
        }
        if (symbol == &llMNTitleCopyrightSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_COPYRIGHT;
            return TRUE;
        }
        if (symbol == &llMNTitlePressStartSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_PRESS_START;
            return TRUE;
        }
        if (symbol == &llMNTitleSuperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_SUPER;
            return TRUE;
        }
        if (symbol == &llMNTitleSmashSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_SMASH;
            return TRUE;
        }
        if (symbol == &llMNTitleBrosSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_BROS;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM)
    {
        for (i = 0; i < ARRAY_COUNT(sNdsTitleFireAnimFrameSymbols); i++)
        {
            if (symbol == sNdsTitleFireAnimFrameSymbols[i].symbol)
            {
                *out_offset = sNdsTitleFireAnimFrameSymbols[i].offset;
                return TRUE;
            }
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_COMMON)
    {
        for (i = 0; i < ARRAY_COUNT(sNdsMNCommonSymbols); i++)
        {
            if (symbol == sNdsMNCommonSymbols[i].symbol)
            {
                *out_offset = sNdsMNCommonSymbols[i].offset;
                return TRUE;
            }
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_VS_MODE)
    {
        for (i = 0; i < ARRAY_COUNT(sNdsMNVSModeSymbols); i++)
        {
            if (symbol == sNdsMNVSModeSymbols[i].symbol)
            {
                *out_offset = sNdsMNVSModeSymbols[i].offset;
                return TRUE;
            }
        }
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP) &&
        (symbol == &llGRPupupuMapMapHeader))
    {
        *out_offset = NDS_RELOC_SYMBOL_GR_PUPUPU_MAP_HEADER;
        return TRUE;
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_INISHIE_MAP) &&
        (symbol == &llGRInishieMapMapHeader))
    {
        *out_offset = NDS_RELOC_SYMBOL_GR_INISHIE_MAP_HEADER;
        return TRUE;
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_HYRULE_MAP) &&
        (symbol == &llGRHyruleMapMapHeader))
    {
        *out_offset = NDS_RELOC_SYMBOL_GR_HYRULE_MAP_HEADER;
        return TRUE;
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND) &&
        (symbol == &llStageDreamLandSprite))
    {
        *out_offset = NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE;
        return TRUE;
    }

    for (i = 0; i < (sizeof(sNdsRelocSymbolProbes) / sizeof(sNdsRelocSymbolProbes[0])); i++)
    {
        if (symbol == sNdsRelocSymbolProbes[i].marker)
        {
            if (loaded->asset_id != sNdsRelocSymbolProbes[i].asset_id)
            {
                return FALSE;
            }
            *out_offset = sNdsRelocSymbolProbes[i].offset;
            return TRUE;
        }
    }

    if (raw_symbol < loaded->data_size)
    {
        *out_offset = (u32)raw_symbol;
        return TRUE;
    }
    if ((raw_symbol >= 0x02000000u) && (raw_symbol < 0x04000000u))
    {
        uintptr_t initialized_offset = *(const uintptr_t *)symbol;

        if (initialized_offset < loaded->data_size)
        {
            *out_offset = (u32)initialized_offset;
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsRelocProbeOpeningRoomFirstEventData(NDSRelocLoadedFile *loaded)
{
    DObjDesc *dobjdesc;
    AObjEvent32 **anim_joints;
    u32 mask = 0;
    u32 dl_count = 0;
    u32 anim_count = 0;
    u32 i;

    if ((loaded == NULL) || (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON))
    {
        return;
    }

    if (ndsRelocRangeInLoadedFile(loaded, NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ,
                                  sizeof(DObjDesc) *
                                  NDS_OPENING_ROOM_PENCILS_DOBJ_ENTRIES) == FALSE)
    {
        return;
    }
    if (ndsRelocRangeInLoadedFile(loaded, NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM,
                                  sizeof(AObjEvent32 *) *
                                  NDS_OPENING_ROOM_PENCILS_ANIM_JOINTS) == FALSE)
    {
        return;
    }

    dobjdesc = (DObjDesc *)((u8 *)loaded->data +
                            NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ);
    anim_joints = (AObjEvent32 **)((u8 *)loaded->data +
                                   NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM);

    if ((dobjdesc[0].id == 0) &&
        (dobjdesc[1].id == 1) &&
        (dobjdesc[2].id == 1) &&
        (dobjdesc[3].id == DOBJ_ARRAY_MAX))
    {
        gNdsOpeningRoomFirstEventPencilsDObjEntries =
            NDS_OPENING_ROOM_PENCILS_DOBJ_ENTRIES;
        mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_IDS_READY;
    }

    for (i = 0; i < NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(loaded, dobjdesc[i].dl,
                                             sizeof(Gfx)) != FALSE)
        {
            dl_count++;
        }
    }
    gNdsOpeningRoomFirstEventPencilsDLPtrs = dl_count;
    if (dl_count == NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS)
    {
        mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_DLS_READY;
    }

    for (i = 0; i < NDS_OPENING_ROOM_PENCILS_ANIM_JOINTS; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(loaded, anim_joints[i],
                                             sizeof(AObjEvent32)) != FALSE)
        {
            anim_count++;
        }
    }
    gNdsOpeningRoomFirstEventPencilsAnimJoints = anim_count;
    if (anim_count == NDS_OPENING_ROOM_PENCILS_ANIM_JOINTS)
    {
        mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_TABLE_READY;
    }

    if (ndsRelocPointerRangeInLoadedFile(loaded, anim_joints[0],
                                         sizeof(AObjEvent32)) != FALSE)
    {
        gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode =
            NDS_AOBJ_EVENT32_OPCODE(anim_joints[0]->u);
        if (gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode ==
            nGCAnimEvent32SetValBlock)
        {
            mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_OPCODE_READY;
        }
    }

    gNdsOpeningRoomFirstEventDataMask |= mask;
    if ((gNdsOpeningRoomFirstEventDataMask &
         NDS_OPENING_ROOM_FIRST_EVENT_DATA_READY_MASK) ==
        NDS_OPENING_ROOM_FIRST_EVENT_DATA_READY_MASK)
    {
        gNdsOpeningRoomFirstEventDataResult =
            NDS_OPENING_ROOM_FIRST_EVENT_DATA_PASS;
    }
}

static void ndsRelocProbeOpeningRoomSymbols(void **files)
{
    u32 probe_mask = 0;
    u32 first_event_mask = 0;
    u32 i;

    if (files == NULL)
    {
        return;
    }

    for (i = 0; i < (sizeof(sNdsRelocSymbolProbes) / sizeof(sNdsRelocSymbolProbes[0])); i++)
    {
        NDSRelocLoadedFile *loaded =
            ndsRelocFindLoadedFileByAsset(sNdsRelocSymbolProbes[i].asset_id);
        void *file = (loaded != NULL) ? loaded->data : NULL;
        void *resolved = ndsRelocGetFileData(file, sNdsRelocSymbolProbes[i].marker);

        if (resolved == ((u8 *)file + sNdsRelocSymbolProbes[i].offset))
        {
            probe_mask |= sNdsRelocSymbolProbes[i].bit;

            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomPencilsDObjDesc)
            {
                first_event_mask |= NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ_READY;
                gNdsOpeningRoomFirstEventPencilsDObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomPencilsAnimJoint)
            {
                first_event_mask |= NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM_READY;
                gNdsOpeningRoomFirstEventPencilsAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVOpeningRoomScene1CamAnimJoint)
            {
                gNdsOpeningRoomLogoCameraAssetMask |=
                    NDS_OPENING_ROOM_LOGO_CAMERA_ASSET_CAMANIM_READY;
                gNdsOpeningRoomLogoCameraAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVOpeningRoomScene2CamAnimJoint)
            {
                gNdsOpeningRoomScene2CameraAssetMask |=
                    NDS_OPENING_ROOM_SCENE2_CAMERA_ASSET_CAMANIM_READY;
                gNdsOpeningRoomScene2CameraAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomLogoDObjDesc)
            {
                gNdsOpeningRoomLogoAssetMask |=
                    NDS_OPENING_ROOM_LOGO_ASSET_DOBJ_READY;
                gNdsOpeningRoomLogoDObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomDeskDObjDesc)
            {
                gNdsOpeningRoomDeskAssetMask |=
                    NDS_OPENING_ROOM_DESK_ASSET_DOBJ_READY;
                gNdsOpeningRoomDeskDObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomLogoMObjSub)
            {
                gNdsOpeningRoomLogoAssetMask |=
                    NDS_OPENING_ROOM_LOGO_ASSET_MOBJ_READY;
                gNdsOpeningRoomLogoMObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomLogoMatAnimJoint)
            {
                gNdsOpeningRoomLogoAssetMask |=
                    NDS_OPENING_ROOM_LOGO_ASSET_MATANIM_READY;
                gNdsOpeningRoomLogoMatAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomBossShadowDisplayList)
            {
                gNdsOpeningRoomBossShadowAssetMask |=
                    NDS_OPENING_ROOM_BOSS_SHADOW_ASSET_DISPLAY_READY;
                gNdsOpeningRoomBossShadowDisplayListOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomBossShadowAnimJoint)
            {
                gNdsOpeningRoomBossShadowAssetMask |=
                    NDS_OPENING_ROOM_BOSS_SHADOW_ASSET_ANIM_READY;
                gNdsOpeningRoomBossShadowAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomSpotlightDisplayList)
            {
                gNdsOpeningRoomSpotlightAssetMask |=
                    NDS_OPENING_ROOM_SPOTLIGHT_ASSET_DISPLAY_READY;
                gNdsOpeningRoomSpotlightDisplayListOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomSpotlightMObjSub)
            {
                gNdsOpeningRoomSpotlightAssetMask |=
                    NDS_OPENING_ROOM_SPOTLIGHT_ASSET_MOBJ_READY;
                gNdsOpeningRoomSpotlightMObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomSpotlightMatAnimJoint)
            {
                gNdsOpeningRoomSpotlightAssetMask |=
                    NDS_OPENING_ROOM_SPOTLIGHT_ASSET_MATANIM_READY;
                gNdsOpeningRoomSpotlightMatAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
        }
    }

    gNdsOpeningRoomRelocSymbolProbeMask |= probe_mask;
    gNdsOpeningRoomFirstEventProbeMask |= first_event_mask;
    if ((gNdsOpeningRoomFirstEventProbeMask &
         NDS_OPENING_ROOM_FIRST_EVENT_READY_MASK) ==
        NDS_OPENING_ROOM_FIRST_EVENT_READY_MASK)
    {
        gNdsOpeningRoomFirstEventTick = 280;
        gNdsOpeningRoomFirstEventResult = NDS_OPENING_ROOM_FIRST_EVENT_PASS;
        ndsRelocProbeOpeningRoomFirstEventData(
            ndsRelocFindLoadedFileByAsset(NDS_RELOC_ASSET_MV_COMMON));
    }
}

static size_t ndsRelocAssetAllocSize(u32 asset_id)
{
    NDSRelocAssetHeader header;

    if ((asset_id != NDS_RELOC_ASSET_INVALID) &&
        (ndsRelocAssetReadHeader(asset_id, &header) != FALSE))
    {
        return (size_t)NDS_RELOC_ALIGN(header.data_size);
    }
    return 0;
}

static s32 ndsRelocSeenAsset(u32 *seen, u32 seen_count, u32 asset_id)
{
    u32 i;

    for (i = 0; i < seen_count; i++)
    {
        if (seen[i] == asset_id)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsRelocAddSeenAsset(u32 *seen, u32 *seen_count, u32 asset_id)
{
    if ((seen == NULL) || (seen_count == NULL))
    {
        return FALSE;
    }
    if (ndsRelocSeenAsset(seen, *seen_count, asset_id) != FALSE)
    {
        return FALSE;
    }
    if (*seen_count >= NDS_RELOC_EXTERN_FILE_ID_CAPACITY)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return FALSE;
    }
    seen[*seen_count] = asset_id;
    (*seen_count)++;
    return TRUE;
}

static size_t ndsRelocExternTreeAllocSize(u32 asset_id, u32 *seen,
                                          u32 *seen_count)
{
    NDSRelocAssetHeader header;
    u32 extern_ids[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 extern_count = 0;
    size_t total;
    u32 i;

    if ((asset_id == NDS_RELOC_ASSET_INVALID) ||
        (ndsRelocAddSeenAsset(seen, seen_count, asset_id) == FALSE))
    {
        return 0;
    }
    if (ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                               sNdsRelocStatusBufferCount,
                               asset_id) != NULL)
    {
        return 0;
    }
    if (ndsRelocAssetReadHeader(asset_id, &header) == FALSE)
    {
        return 0;
    }

    total = (size_t)NDS_RELOC_ALIGN(header.data_size);
    if ((header.extern_file_ids_num == 0u) ||
        (header.extern_file_ids_num > NDS_RELOC_EXTERN_FILE_ID_CAPACITY) ||
        (ndsRelocAssetReadExternFileIDs(asset_id,
                                        extern_ids,
                                        NDS_RELOC_EXTERN_FILE_ID_CAPACITY,
                                        &extern_count) == FALSE))
    {
        return total;
    }

    for (i = 0; i < extern_count; i++)
    {
        u32 dep_asset_id = ndsRelocAssetIDForToken(extern_ids[i]);

        total = (size_t)NDS_RELOC_ALIGN(total);
        total += ndsRelocExternTreeAllocSize(dep_asset_id, seen, seen_count);
    }
    return total;
}

static NDSRelocLoadedFile *ndsRelocLoadExternTreeAsset(u32 asset_id,
                                                       uintptr_t *heap_ptr)
{
    NDSRelocLoadedFile *loaded;
    NDSRelocAssetHeader header;
    u32 i;
    void *status_file;
    void *file_alloc;
    size_t asset_size;

    if ((asset_id == NDS_RELOC_ASSET_INVALID) || (heap_ptr == NULL))
    {
        return NULL;
    }

    status_file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                         sNdsRelocStatusBufferCount,
                                         asset_id);
    if (status_file != NULL)
    {
        return ndsRelocFindLoadedFileByData(status_file);
    }

    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if (loaded != NULL)
    {
        if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
        {
            return NULL;
        }
        ndsRelocNormalizeGroundMapAsset(loaded);
        ndsRelocAddStatusBufferFile(asset_id, loaded->data);
        return loaded;
    }

    asset_size = ndsRelocAssetAllocSize(asset_id);
    if ((asset_size == 0u) ||
        (ndsRelocAssetReadHeader(asset_id, &header) == FALSE))
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    *heap_ptr = NDS_RELOC_ALIGN(*heap_ptr);
    file_alloc = (void *)*heap_ptr;
    *heap_ptr += asset_size;

    if (ndsRelocAssetLoadData(asset_id, file_alloc, asset_size, &header) ==
        FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, file_alloc, &header);
    if (loaded == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    ndsRelocAddStatusBufferFile(asset_id, file_alloc);

    if (ndsRelocApplyWordByteSwap(loaded) == FALSE)
    {
        return NULL;
    }

    for (i = 0; i < loaded->extern_count; i++)
    {
        u32 dep_asset_id = ndsRelocAssetIDForToken(loaded->extern_file_ids[i]);

        if (ndsRelocLoadExternTreeAsset(dep_asset_id, heap_ptr) == NULL)
        {
            loaded->external_fixup_fail_count++;
            ndsRelocRecordExternalFixupFail(asset_id);
            return NULL;
        }
    }

    if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
    {
        return NULL;
    }
    ndsRelocNormalizeGroundMapAsset(loaded);
    return loaded;
}

void lbRelocInitSetup(LBRelocSetup *setup)
{
    sNdsRelocInitCount++;
    ndsRelocPrepareSceneCache();

    if (setup != NULL)
    {
        sNdsRelocStatusBuffer = setup->status_buffer;
        sNdsRelocStatusBufferMax = setup->status_buffer_size;
        sNdsRelocStatusBufferCount = 0;
        sNdsRelocForceStatusBuffer = setup->force_status_buffer;
        sNdsRelocForceStatusBufferMax = setup->force_status_buffer_size;
        sNdsRelocForceStatusBufferCount = 0;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomRelocInitCount++;
    }
}

size_t lbRelocGetFileSize(const void *file_id)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    u32 seen[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 seen_count = 0;
    size_t asset_size = ndsRelocExternTreeAllocSize(asset_id, seen,
                                                    &seen_count);
#else
    size_t asset_size = ndsRelocAssetAllocSize(asset_id);
#endif

    if (asset_size != 0)
    {
        return asset_size;
    }

    return sizeof(Sprite);
}

void *lbRelocGetExternHeapFile(const void *file_id, void *heap)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
    NDSRelocLoadedFile *loaded;
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    uintptr_t heap_ptr = (uintptr_t)heap;
#else
    size_t asset_size;
    NDSRelocAssetHeader header;
#endif

    if ((asset_id == NDS_RELOC_ASSET_INVALID) || (heap == NULL))
    {
        return heap;
    }
    ndsRelocPrepareSceneCache();

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    loaded = ndsRelocLoadExternTreeAsset(asset_id, &heap_ptr);
    if (loaded == NULL)
    {
        return heap;
    }
    if (asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        ndsRelocNormalizeN64LogoSprite(loaded);
        gNdsStartupLogoRelocResult = NDS_STARTUP_LOGO_RELOC_PASS;
        gNdsStartupLogoRelocSize = loaded->data_size;
    }
    ndsFighterManagerRecordExternToken(token, loaded->data);
    return loaded->data;
#else
    asset_size = ndsRelocAssetAllocSize(asset_id);
    if (asset_size == 0)
    {
        return heap;
    }
    if (ndsRelocAssetLoadData(asset_id, heap, asset_size, &header) == FALSE)
    {
        return heap;
    }

    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
    if (loaded == NULL)
    {
        return heap;
    }
    if ((ndsRelocApplyWordByteSwap(loaded) != FALSE) &&
        (ndsRelocFinalizeLoadedFile(loaded) != FALSE) &&
        (asset_id == NDS_RELOC_ASSET_N64_LOGO))
    {
        ndsRelocNormalizeN64LogoSprite(loaded);
        gNdsStartupLogoRelocResult = NDS_STARTUP_LOGO_RELOC_PASS;
        gNdsStartupLogoRelocSize = header.data_size;
    }
    ndsFighterManagerRecordExternToken(token, heap);
    ndsRelocAddStatusBufferFile(token, heap);
    return heap;
#endif
}

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
static void ndsRelocCopyLoadedFileToHeap(const NDSRelocLoadedFile *loaded,
                                         void *heap)
{
    uintptr_t src_base;
    uintptr_t dst_base;
    u32 words;
    u32 i;

    if ((loaded == NULL) || (loaded->data == NULL) || (heap == NULL))
    {
        return;
    }

    memcpy(heap, loaded->data, loaded->data_size);

    src_base = (uintptr_t)loaded->data;
    dst_base = (uintptr_t)heap;
    words = loaded->data_size / sizeof(u32);
    for (i = 0; i < words; i++)
    {
        void *slot = (u8 *)heap + (i * sizeof(u32));
        uintptr_t value = (uintptr_t)ndsRelocReadNative32(slot);

        if ((value >= src_base) &&
            ((value - src_base) < (uintptr_t)loaded->data_size))
        {
            ndsRelocWriteNativePointer(slot,
                                       (void *)(dst_base +
                                                (value - src_base)));
        }
    }
}

static void *ndsRelocForceLoadFighterAObj16File(u32 token, u32 asset_id,
                                                void *heap)
{
    NDSRelocAssetHeader header;
    NDSRelocLoadedFile *loaded;
    size_t asset_size;

    if ((heap == NULL) ||
        (ndsRelocIsFighterAObj16Asset(asset_id) == FALSE))
    {
        return NULL;
    }

    asset_size = ndsRelocAssetAllocSize(asset_id);
    if ((asset_size == 0u) ||
        (ndsRelocAssetReadHeader(asset_id, &header) == FALSE))
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if (ndsRelocAssetLoadData(asset_id, heap, asset_size, &header) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    ndsRelocRemoveFighterAObj16LoadedAliases(asset_id, heap);
    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
    if (loaded == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    if ((ndsRelocApplyWordByteSwap(loaded) == FALSE) ||
        (ndsRelocFinalizeLoadedFile(loaded) == FALSE))
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    ndsFighterManagerRecordExternToken(token, heap);
    ndsRelocSetStatusBufferFile(token, heap);
    ndsRelocSetStatusBufferFile(asset_id, heap);
    ndsRelocSetForceStatusBufferFile(token, heap);
    return heap;
}
#endif

void *lbRelocGetForceExternHeapFile(const void *file_id, void *heap)
{
    u32 token = ndsRelocFileID(file_id);
    void *file;
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    u32 asset_id = ndsRelocAssetIDForToken(token);
    NDSRelocLoadedFile *loaded;

    if ((heap != NULL) &&
        (ndsRelocIsFighterAObj16Asset(asset_id) != FALSE))
    {
        file = ndsRelocForceLoadFighterAObj16File(token, asset_id, heap);
        return (file != NULL) ? file : heap;
    }
#endif

    file = lbRelocGetExternHeapFile(file_id, heap);
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    if ((file != NULL) && (heap != NULL) && (file != heap))
    {
        loaded = ndsRelocFindLoadedFileByData(file);
        if (loaded != NULL)
        {
            ndsRelocCopyLoadedFileToHeap(loaded, heap);
            file = heap;
        }
    }
#endif

    ndsRelocSetForceStatusBufferFile(token, file);
    return file;
}

void *lbRelocGetStatusBufferFile(const void *file_id)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
    void *file;
    size_t asset_size;
    void *heap;

    ndsRelocPrepareSceneCache();

    file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                  sNdsRelocStatusBufferCount,
                                  token);
    if ((file == NULL) && (asset_id != NDS_RELOC_ASSET_INVALID))
    {
        file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                      sNdsRelocStatusBufferCount,
                                      asset_id);
    }
    if (file != NULL)
    {
        ndsFighterManagerRecordStatusToken(token, file);
        return file;
    }

    asset_size = ndsRelocAssetAllocSize(asset_id);
    if ((asset_id == NDS_RELOC_ASSET_INVALID) || (asset_size == 0u))
    {
        return NULL;
    }

    heap = syTaskmanMalloc(asset_size, 0x10);
    if (heap == NULL)
    {
        return NULL;
    }
    file = lbRelocGetExternHeapFile(file_id, heap);
    ndsFighterManagerRecordStatusToken(token, file);
    return file;
}

size_t lbRelocGetAllocSize(u32 *ids, u32 len)
{
    size_t total = 0;
    u32 i;

    for (i = 0; i < len; i++)
    {
        u32 asset_id = ndsRelocAssetIDForToken(ids[i]);
        size_t asset_size = ndsRelocAssetAllocSize(asset_id);

        total = NDS_RELOC_ALIGN(total);
        if (asset_size != 0)
        {
            total += asset_size;
        }
        else
        {
            total += sizeof(uintptr_t);
        }
    }
    return total;
}

size_t lbRelocLoadFilesExtern(u32 *ids, u32 len, void **files, void *heap)
{
    u32 i;
    u32 mask = 0;
    u32 header_mask = 0;
    u32 payload_mask = 0;
    u32 word_swap_mask = 0;
    u32 pointer_fixup_mask = 0;
    uintptr_t heap_ptr = (uintptr_t)heap;
    uintptr_t heap_start = (uintptr_t)heap;

    ndsRelocPrepareSceneCache();

    for (i = 0; i < len; i++)
    {
        u32 token = ids[i];
        u32 asset_id = ndsRelocAssetIDForToken(token);
        u32 bit = ndsRelocOpeningRoomBitForAsset(asset_id);
        NDSRelocAssetHeader header;
        size_t asset_size = 0;
        void *file_alloc = (void *)(uintptr_t)token;

        mask |= bit;

        if ((asset_id != NDS_RELOC_ASSET_INVALID) &&
            (ndsRelocAssetReadHeader(asset_id, &header) != FALSE))
        {
            header_mask |= bit;
            asset_size = (size_t)NDS_RELOC_ALIGN(header.data_size);

            if ((heap != NULL) && (asset_size != 0))
            {
                heap_ptr = NDS_RELOC_ALIGN(heap_ptr);
                file_alloc = (void *)heap_ptr;

                if (ndsRelocAssetLoadData(asset_id, file_alloc, asset_size, &header) != FALSE)
                {
                    NDSRelocLoadedFile *loaded;

                    payload_mask |= bit;
                    if (bit != 0u)
                    {
                        gNdsOpeningRoomRelocBytesLoaded += header.data_size;
                        gNdsOpeningRoomRelocLastFileID = asset_id;
                        gNdsOpeningRoomRelocLastSize = header.data_size;
                    }

                    loaded = ndsRelocRegisterLoadedFile(asset_id, bit, file_alloc, &header);
                    ndsRelocAddStatusBufferFile(token, file_alloc);
                    if (ndsRelocApplyWordByteSwap(loaded) != FALSE)
                    {
                        word_swap_mask |= bit;
                        if (ndsRelocFinalizeLoadedFile(loaded) != FALSE)
                        {
                            pointer_fixup_mask |= bit;
                            if (asset_id == NDS_RELOC_ASSET_MV_COMMON)
                            {
                                ndsRelocNormalizeMVCommonMObjSubs(loaded);
                            }
                            if ((asset_id ==
                                 NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1) ||
                                (asset_id ==
                                 NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2))
                            {
                                ndsRelocNormalizeOpeningPortraitsSprites(
                                    loaded);
                            }
                            if (asset_id ==
                                NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE)
                            {
                                ndsRelocNormalizeIFAnnounceMarioSprites(
                                    loaded);
                            }
                            if (asset_id ==
                                NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM)
                            {
                                ndsRelocNormalizeTitleFireSprites(loaded);
                            }
                            ndsRelocNormalizeGroundMapAsset(loaded);
                        }
                    }
                    heap_ptr += asset_size;
                }
            }
        }

        if (files != NULL)
        {
            files[i] = file_alloc;
        }
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomRelocLoadCount += len;
        gNdsOpeningRoomRelocFileMask |= mask;
        gNdsOpeningRoomRelocHeaderMask |= header_mask;
        gNdsOpeningRoomRelocPayloadMask |= payload_mask;
        gNdsOpeningRoomRelocWordSwapMask |= word_swap_mask;
        gNdsOpeningRoomRelocPointerFixupMask |= pointer_fixup_mask;
        gNdsOpeningRoomRelocContentReady =
            ((gNdsOpeningRoomRelocPayloadMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK) ? 1u : 0u;
        if (((gNdsOpeningRoomRelocWordSwapMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK) &&
            ((gNdsOpeningRoomRelocPointerFixupMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK))
        {
            ndsRelocProbeOpeningRoomSymbols(files);
        }
        /* Full data fixup still requires mixed-width struct fixups and
         * renderer-specific texture/display-list fixups. Keep the existing
         * gate false until those contracts are implemented and verified. */
        gNdsOpeningRoomRelocFixupReady = 0;

        if ((len == NDS_RELOC_OPENING_ROOM_FILE_COUNT) &&
            ((gNdsOpeningRoomRelocFileMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK))
        {
            gNdsOpeningRoomRelocResult = NDS_OPENING_ROOM_RELOC_PASS;
        }
    }
    if (gSCManagerSceneData.scene_curr == nSCKindOpeningMario)
    {
        if ((ndsRelocFindLoadedFileByAsset(
                 NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE) != NULL) &&
            (ndsRelocFindLoadedFileByAsset(
                 NDS_RELOC_ASSET_OPENING_COMMON) != NULL) &&
            (gNdsOpeningMarioSpriteNormalizeCount >= 5u) &&
            (gNdsOpeningMarioSpriteNormalizeFailCount == 0))
        {
            gNdsOpeningMarioRelocResult = NDS_OPENING_MARIO_RELOC_PASS;
        }
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindPlayersVS) &&
        (len == 7u))
    {
        gNdsPlayersVSOriginalLoadedFileCount = len;
        gNdsPlayersVSOriginalRelocResult = NDS_PLAYERS_VS_ORIGINAL_RELOC_PASS;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindMaps) &&
        (len == 5u))
    {
        gNdsMapsOriginalLoadedFileCount = len;
        gNdsMapsOriginalRelocResult = NDS_MAPS_ORIGINAL_RELOC_PASS;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (len == 8u))
    {
        gNdsSCVSBattleOriginalLoadedFileCount = len;
        gNdsSCVSBattleOriginalRelocResult =
            NDS_SCVSBATTLE_ORIGINAL_RELOC_PASS;
    }

    return (heap != NULL) ? (size_t)(heap_ptr - heap_start) : 0;
}

void *ndsRelocGetFileData(void *file, const void *symbol)
{
    NDSRelocLoadedFile *loaded = ndsRelocFindLoadedFileByData(file);
    u32 offset;

    if (file == NULL)
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }
    if (loaded == NULL)
    {
        return file;
    }
    if ((ndsRelocResolveSymbolOffset(loaded, symbol, &offset) == FALSE) ||
        (offset >= loaded->data_size))
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }

    if (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO)
    {
        gNdsOpeningRoomRelocSymbolResolveCount++;
        gNdsOpeningRoomRelocLastSymbolOffset = offset;
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP) &&
        (symbol == &llGRPupupuMapMapHeader))
    {
        MPGroundData *ground_data = (MPGroundData *)((u8 *)file + offset);

        gNdsStagePupupuRelocResult = NDS_STAGE_PUPUPU_RELOC_PASS;
        gNdsStagePupupuMapHeaderOffset = offset;
        gNdsStagePupupuGroundDataPtrReady = (ground_data != NULL) ? 1u : 0u;
        if ((ground_data != NULL) && (ground_data->map_geometry != NULL))
        {
            gNdsStagePupupuGeometryPtrReady = 1;
        }
        if ((ground_data != NULL) && (ground_data->wallpaper != NULL))
        {
            gNdsStagePupupuWallpaperPtrReady = 1;
        }
        if ((ground_data != NULL) && (ground_data->map_nodes != NULL))
        {
            gNdsStagePupupuMapNodesPtrReady = 1;
        }
        if (ground_data != NULL)
        {
            gNdsStagePupupuLightAngleXBits =
                ndsFloatBits(ground_data->light_angle.x);
            gNdsStagePupupuLightAngleYBits =
                ndsFloatBits(ground_data->light_angle.y);
            gNdsStagePupupuBGM = ground_data->bgm_id;
        }
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_INISHIE_MAP) &&
        (symbol == &llGRInishieMapMapHeader))
    {
        MPGroundData *ground_data = (MPGroundData *)((u8 *)file + offset);

        gNdsStageInishieMapHeaderOffset = offset;
        gNdsStageInishieGroundDataPtrReady = (ground_data != NULL) ? 1u : 0u;
        if ((ground_data != NULL) && (ground_data->map_geometry != NULL))
        {
            gNdsStageInishieGeometryPtrReady = 1u;
        }
        if ((ground_data != NULL) && (ground_data->map_nodes != NULL))
        {
            gNdsStageInishieMapNodesPtrReady = 1u;
        }
        if ((gNdsStageInishieGroundDataPtrReady != 0u) &&
            (gNdsStageInishieGeometryPtrReady != 0u))
        {
            gNdsStageInishieRelocResult = NDS_STAGE_INISHIE_RELOC_PASS;
        }
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND) &&
        (symbol == &llStageDreamLandSprite))
    {
        gNdsStagePupupuWallpaperPtrReady = 1;
    }
    return (u8 *)file + offset;
}
