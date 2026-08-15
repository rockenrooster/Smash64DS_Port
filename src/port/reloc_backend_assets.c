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

#include <nds/nds_battlepack_anim.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_reloc_assets.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>

#if NDS_R2_BATTLEPACK && !NDS_R2_ANIM_CACHE
/* Structural, not documentation. The resident pack IS the animation arena's
 * tenant: its allocation, heap-generation ownership, drop, re-arm and streamed
 * load all ride the anim cache's seams. Turning the pack on with the cache off
 * would compile the loader out and leave `gNdsBattlePackHits` reading a
 * plausible, meaningless 0 -- exactly the "flag that silently never fired"
 * failure this campaign has already shipped once. */
#error "NDS_R2_BATTLEPACK requires NDS_R2_ANIM_CACHE (the pack lives in its arena)"
#endif

#define NDS_RELOC_OPENING_ROOM_FILE_COUNT 8u
#define NDS_RELOC_OPENING_ROOM_FILE_MASK 0xffu
#define NDS_RELOC_LOADED_FILE_CAPACITY 96u
#define NDS_RELOC_NORMALIZED_MOBJ_SUB_CAPACITY 128u
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
#define NDS_RELOC_ASSET_FT_STOCKS_ZAKO 0x19u
#define NDS_RELOC_ASSET_MN_VS_RESULTS 0x22u
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
#define NDS_RELOC_ASSET_MARIO_ANIM_FIRE_FLOWER_AIR 0x281u
#define NDS_RELOC_ASSET_MARIO_ANIM_APPEAR1 0x279u
#define NDS_RELOC_ASSET_MARIO_ANIM_APPEAR2 0x27au
#define NDS_RELOC_ASSET_FOX_ANIM_FIRST 0x282u
#define NDS_RELOC_ASSET_FOX_ANIM_LAST 0x31fu
#define NDS_RELOC_ASSET_FOX_ANIM_APPEAR 0x309u
#define NDS_RELOC_ASSET_FOX_ANIM_ARWING 0x30au

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
#define NDS_RELOC_SYMBOL_GR_PUPUPU_MAP_HEADER 0x14u
#define NDS_RELOC_SYMBOL_GR_HYRULE_MAP_HEADER 0x14u
#define NDS_RELOC_SYMBOL_GR_INISHIE_MAP_HEADER 0x14u
#define NDS_RELOC_SYMBOL_MARIO_MAIN_ATTRIBUTES 0x428u
#define NDS_RELOC_SYMBOL_FOX_MAIN_ATTRIBUTES 0x46cu
/* Both weapon attribute structs sit at file offset 0: the fireball's in file
 * 204 (llMarioSpecial1FireballWeaponAttributes = 0x0) and the blaster's in
 * file 210 (llFoxSpecial1BlasterWeaponAttributes = 0x0), per
 * decomp/BattleShip-main/tools/reloc_data_symbols.us.txt:3838/:3842. */
#define NDS_RELOC_SYMBOL_MARIO_SPECIAL1_FIREBALL_WEAPON_ATTRIBUTES 0x0u
#define NDS_RELOC_SYMBOL_FOX_SPECIAL1_BLASTER_WEAPON_ATTRIBUTES 0x0u

#define NDS_OPENING_PORTRAITS_CARD_WIDTH 300u
#define NDS_OPENING_PORTRAITS_CARD_HEIGHT 55u
#define NDS_OPENING_PORTRAITS_COVER_WIDTH 656u
#define NDS_OPENING_PORTRAITS_COVER_HEIGHT 55u
#define NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH 96u
#define NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT 64u
#define NDS_TITLE_MAX_WIDTH 320u
#define NDS_TITLE_MAX_HEIGHT 240u
#define NDS_TITLE_FILE_BUFFER_SIZE 176000u
/* One granularity, named once, so a caller that has to pass it as a value
 * cannot drift from the macro that rounds with it. */
#define NDS_RELOC_ALIGN_BYTES 0x10u
#define NDS_RELOC_ALIGN(value) \
    (((value) + (NDS_RELOC_ALIGN_BYTES - 1u)) & ~(NDS_RELOC_ALIGN_BYTES - 1u))
#define NDS_RELOC_STAGE_CASTLE_STATIC_SIZE NDS_RELOC_ALIGN(0x26cd0u)
#define NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE NDS_RELOC_ALIGN(0x6890u)
#define NDS_OPENING_ACTION_PREVIEW_MAX_WIDTH 320u
#define NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT 264u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH 320u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT 240u
#if (NDS_DEV_SCENE_HARNESS != 0) || NDS_TASK34_STAGE_STREAM_CENSUS || \
    (NDS_TASK36_HW_COMPOSE == 2)
#define NDS_OPENING_ACTION_PREVIEW_CACHE_PIXELS 1u
#else
#define NDS_OPENING_ACTION_PREVIEW_CACHE_PIXELS \
    (NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH * \
     NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT)
#endif
#if NDS_DEV_SCENE_HARNESS != 0
#define NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE \
    (NDS_RELOC_STAGE_CASTLE_STATIC_SIZE + \
     NDS_RELOC_EXTERN_DATA_BANK_113_STATIC_SIZE)
#else
#define NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE 270000u
#endif
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

static NDSFighterDLDrawState
    sNdsFighterDLAllDrawStates[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
static NDSRendererStats
    sNdsFighterDLAllDrawStats[2][NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
#endif
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
static NDSRelocNormalizedMObjSub
    sNdsRelocNormalizedMObjSubs[NDS_RELOC_NORMALIZED_MOBJ_SUB_CAPACITY];
static u32 sNdsRelocNormalizedMObjSubCount;
static u32 sNdsRelocOwnerScene = NDS_RELOC_ASSET_INVALID;
static u32 sNdsRelocSceneGeneration;
/* The taskman-heap generation the resident reloc set was established under.
 * Ownership authority for the scene cache; see ndsRelocPrepareSceneCache. */
static u32 sNdsRelocResidentHeapGeneration;
/* Split so the two eviction causes are never conflated. The generation count
 * should be non-zero on any run that re-enters a scene (Sudden Death,
 * rematch); a zero there means this contract never engaged. */
volatile u32 gNdsRelocSceneReentryGenerationEvictCount;
volatile u32 gNdsRelocSceneReentryRangeEvictCount;
static LBFileNode *sNdsRelocStatusBuffer;
static s32 sNdsRelocStatusBufferCount;
static s32 sNdsRelocStatusBufferMax;
static LBFileNode *sNdsRelocForceStatusBuffer;
static s32 sNdsRelocForceStatusBufferCount;
static s32 sNdsRelocForceStatusBufferMax;

typedef union NDSRelocSceneFileBuffer
{
    u8 title[NDS_TITLE_FILE_BUFFER_SIZE];
    u8 opening_action[NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE];
} NDSRelocSceneFileBuffer;

/* BattleShip's scene manager serializes the opening-movie and title scenes.
 * Harness builds cannot enter either scene; their only users of this store are
 * the two exactly bounded static assets in ndsRelocStaticBufferForAsset.
 * Normal startup keeps the full opening extent. Reuse the larger store instead
 * of permanently reserving both buffers in ARM9 main memory. */
static NDSRelocSceneFileBuffer sNdsRelocSceneFileBuffer
    __attribute__((aligned(16)));
#define sNdsTitleFileBuffer sNdsRelocSceneFileBuffer.title
#define sNdsOpeningActionPreviewFileBuffer \
    sNdsRelocSceneFileBuffer.opening_action
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

static const NDSVSResultsSpriteDesc sNdsVSResultsSpriteDescs[] = {
    { 0x0358u, 62u, 13u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0x0990u, 83u, 17u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0x0d38u, 62u, 13u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0x10d8u, 62u, 13u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0x49e8u, 15u, 12u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0x4b08u, 15u, 12u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0x4c28u, 15u, 12u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0x4d48u, 15u, 12u, 1u, G_IM_FMT_IA, G_IM_SIZ_8b },
    { 0xd5c8u, 300u, 220u, 9u, G_IM_FMT_I, G_IM_SIZ_4b },
    { 0xe2a0u, 42u, 35u, 2u, G_IM_FMT_RGBA, G_IM_SIZ_16b }
};

/* Exact BattleShip HUD and fighter-interface Sprite manifests.  These are
 * the mixed-width Sprite records needed by the live Mario/Fox battle HUD and
 * its countdown/GO sequence. Keeping the offsets, dimensions and formats
 * explicit avoids guessing at arbitrary relocated data after the blanket u32
 * endian pass. */
static const NDSRelocSpriteNormalizeDesc
    sNdsBattleInterfaceSpriteDescs[] = {
    /* IFCommonGameStatus countdown and GO (reloc asset 0x52). */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x4d78u, 62u, 73u, 5u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0xa730u, 70u, 74u, 6u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0xc370u, 24u, 73u, 2u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },
    /* The blue mixed-width announcement letters, which is why neither GAME SET
     * nor TIME UP has ever appeared: the normalization manifest stopped after
     * countdown/GO, so their headers kept the blanket endian pass's swapped
     * width/height and bmfmt/bmsiz and the compositor could not draw them.
     * `dIFCommonAnnounceGameSetSpriteData` (decomp ifcommon.c:154) spells GAME
     * SET from G/A/M/E/S and `...TimeUp...` (:2244) spells TIME UP from
     * T/I/M/E/U/P -- M and E are shared, so nine descriptors serve both
     * announcements, which is why they land together.
     *
     * Every field here was READ OUT OF THE ASSET, not inferred: the extracted
     * relocData file `assets/us/relocData/82.vpk0.bin` is the same bytes the ROM
     * loads, so `struct sprite` (include/PR/sp.h, big-endian, natural alignment)
     * can be parsed on the host. Two earlier attempts did this through gdb on a
     * running ROM and lost both runs -- one to a Results-only boot where
     * `gGMCommonFiles[1]` is a different asset, one to a single bad expression
     * aborting the whole printf -- and neither was necessary. The parser was
     * validated by reproducing all five manifest entries that already work
     * (62x73/5, 70x74/6, 24x73/2 RGBA 32b; 8x53/1 IA 8b; 15x15/1 I 4b) exactly,
     * and T/I/M/E/U/P match the widths BUGS.md had already recorded by hand.
     * All nine are RGBA/32b with attr 0x240 (SP_TEXSHUF | SP_TRANSPARENT), the
     * same shape as the countdown entries above. */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0xe4a8u, 36u, 56u, 3u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* T */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0xf740u, 17u, 57u, 2u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* I */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x127e0u, 50u, 56u, 4u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* M, shared by both announcements */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x144e0u, 32u, 56u, 2u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* E, shared by both announcements */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x16eb8u, 41u, 58u, 3u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* U */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x18fe8u, 36u, 56u, 3u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* P */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x1b5f8u, 39u, 58u, 3u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* S */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x1de68u, 43u, 56u, 3u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* A */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x20788u, 41u, 57u, 3u,
      G_IM_FMT_RGBA, G_IM_SIZ_32b },  /* G */
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x20990u, 8u, 53u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x21760u, 97u, 33u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x21878u, 15u, 11u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x21950u, 15u, 15u, 1u,
      G_IM_FMT_I, G_IM_SIZ_4b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x21a10u, 11u, 11u, 1u,
      G_IM_FMT_I, G_IM_SIZ_4b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x21ba8u, 19u, 19u, 1u,
      G_IM_FMT_I, G_IM_SIZ_4b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x22128u, 32u, 41u, 1u,
      G_IM_FMT_I, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x22588u, 30u, 32u, 1u,
      G_IM_FMT_I, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x22f18u, 46u, 49u, 1u,
      G_IM_FMT_I, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x23a28u, 48u, 57u, 1u,
      G_IM_FMT_I, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x24620u, 53u, 53u, 1u,
      G_IM_FMT_I, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, 0x25290u, 55u, 55u, 1u,
      G_IM_FMT_I, G_IM_SIZ_8b },

    /* IFCommonDigits (reloc asset 0x24). */
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0068u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0118u, 5u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x01c8u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0278u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0328u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x03d8u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0488u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0538u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x05e8u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0698u, 8u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0710u, 6u, 3u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x0828u, 11u, 11u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_DIGITS, 0x08d8u, 6u, 10u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },

    /* IFCommonPlayerDamage (reloc asset 0xa4). */
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x0148u, 16u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x02d8u, 11u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x0500u, 17u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x0698u, 16u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x08c0u, 17u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x0a58u, 15u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x0c80u, 17u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x0e18u, 16u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x1040u, 17u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x1270u, 17u, 19u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x1458u, 19u, 16u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, 0x15d8u, 20u, 12u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },

    /* IFCommonTimer (reloc asset 0xa5). */
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0138u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0228u, 8u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x03a8u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0528u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x06a8u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0828u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x09a8u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0b28u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0ca8u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0e28u, 12u, 18u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x0f08u, 6u, 16u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x1018u, 11u, 11u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x1090u, 11u, 3u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_4b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x1140u, 5u, 9u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },
    { NDS_RELOC_ASSET_IF_COMMON_TIMER, 0x1238u, 9u, 9u, 1u,
      G_IM_FMT_IA, G_IM_SIZ_8b },

    /* Mario/Fox model-owned stock icons and fighter emblems. */
    { NDS_RELOC_ASSET_MARIO_MODEL, 0x72d0u, 8u, 10u, 1u,
      G_IM_FMT_CI, G_IM_SIZ_4b },
    { NDS_RELOC_ASSET_MARIO_MODEL, 0x74c8u, 27u, 25u, 1u,
      G_IM_FMT_I, G_IM_SIZ_4b },
    { NDS_RELOC_ASSET_FOX_MODEL, 0x7c28u, 8u, 10u, 1u,
      G_IM_FMT_CI, G_IM_SIZ_4b },
    { NDS_RELOC_ASSET_FOX_MODEL, 0x7e08u, 30u, 24u, 1u,
      G_IM_FMT_I, G_IM_SIZ_4b }
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

#define NDS_KNOWN_ASSET_SYMBOL(asset, name, value) { asset, &name, value },
static const NDSRelocKnownAssetSymbol sNdsKnownAssetSymbols[] = {
    NDS_IFCOMMON_RELOC_SYMBOLS(NDS_KNOWN_ASSET_SYMBOL)
    NDS_VS_RESULTS_RELOC_SYMBOLS(NDS_KNOWN_ASSET_SYMBOL)
};
#undef NDS_KNOWN_ASSET_SYMBOL

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

/* Task 85. memcpy(&value, addr, 4) is the portable way to read a possibly
 * unaligned word, and on most targets the compiler turns it into one load. It
 * cannot here: the ARM946E-S has no unaligned-access support, so GCC must emit
 * a real call that handles the misaligned case byte by byte.
 *
 * Task 85 measured the result -- 51% of every memcpy call in the frame moves 2
 * or 4 bytes, and at ~70 ticks of call overhead each that is around 30,000
 * ticks/frame spent almost entirely on function-call machinery.
 *
 * The aligned test below is two instructions and the fallback keeps the exact
 * original behaviour, so this needs no proof about where these pointers come
 * from. That matters more than it looks: an unaligned LDR on this core does not
 * fault, it rotates, so a wrong alignment assumption would corrupt silently
 * rather than crash. */
static u32 ndsRelocReadNative32(const void *addr)
{
    u32 value;

#if NDS_TASK85_ALIGNED_NATIVE_ACCESS
    if ((((uintptr_t)addr) & 3u) == 0u)
    {
        return *(const u32 *)addr;
    }
#endif
    memcpy(&value, addr, sizeof(value));
    return value;
}

static void ndsRelocWriteNative32(void *addr, u32 value)
{
#if NDS_TASK85_ALIGNED_NATIVE_ACCESS
    if ((((uintptr_t)addr) & 3u) == 0u)
    {
        *(u32 *)addr = value;
        return;
    }
#endif
    memcpy(addr, &value, sizeof(value));
}

static void ndsRelocWriteNativePointer(void *addr, void *target)
{
    ndsRelocWriteNative32(addr, (u32)(uintptr_t)target);
}

static s32 ndsRelocIsMarioFoxAnimID(u32 asset_id)
{
    return (((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_WAIT) &&
             (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_FIRE_FLOWER_AIR)) ||
            ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FIRST) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_LAST))) ?
        TRUE :
        FALSE;
}

/* BattleShip's Mario battle animation files are one contiguous reloc bank.
 * The imported motion descriptors retain the addresses of the original
 * llFTMarioAnim*FileID symbols, so bridge each symbol to its exact source file
 * number instead of leaving unproven motions with a null figatree. */
static const uintptr_t *const sNdsRelocMarioBattleAnimFileIDs[] =
{
    &llFTMarioAnimWaitFileID, /* 499 */
    &llFTMarioAnimWalk1FileID, /* 500 */
    &llFTMarioAnimWalk2FileID, /* 501 */
    &llFTMarioAnimWalk3FileID, /* 502 */
    &llFTMarioAnimWalkEndFileID, /* 503 */
    &llFTMarioAnimDashFileID, /* 504 */
    &llFTMarioAnimRunFileID, /* 505 */
    &llFTMarioAnimRunBrakeFileID, /* 506 */
    &llFTMarioAnimTurnFileID, /* 507 */
    &llFTMarioAnimTurnRunFileID, /* 508 */
    &llFTMarioAnimJumpFFileID, /* 509 */
    &llFTMarioAnimJumpBFileID, /* 510 */
    &llFTMarioAnimJumpAerialFFileID, /* 511 */
    &llFTMarioAnimJumpAerialBFileID, /* 512 */
    &llFTMarioAnimFallFileID, /* 513 */
    &llFTMarioAnimFallAerialFileID, /* 514 */
    &llFTMarioAnimCrouchFileID, /* 515 */
    &llFTMarioAnimCrouchIdleFileID, /* 516 */
    &llFTMarioAnimCrouchEndFileID, /* 517 */
    &llFTMarioAnimLandingAirXFileID, /* 518 */
    &llFTMarioAnimShieldDropFileID, /* 519 */
    &llFTMarioAnimTeeterFileID, /* 520 */
    &llFTMarioAnimTeeterstartFileID, /* 521 */
    &llFTMarioAnimSleepFileID, /* 522 */
    &llFTMarioAnimDamaged1FileID, /* 523 */
    &llFTMarioAnimDamaged2FileID, /* 524 */
    &llFTMarioAnimFalconDivePulledFileID, /* 525 */
    &llFTMarioAnimDamageX1FileID, /* 526 */
    &llFTMarioAnimDamageX2FileID, /* 527 */
    &llFTMarioAnimDamageX3FileID, /* 528 */
    &llFTMarioAnimDamaged3FileID, /* 529 */
    &llFTMarioAnimDamaged4FileID, /* 530 */
    &llFTMarioAnimDamaged5FileID, /* 531 */
    &llFTMarioAnimDamaged6FileID, /* 532 */
    &llFTMarioAnimDamageAirFileID, /* 533 */
    &llFTMarioAnimDamaged7FileID, /* 534 */
    &llFTMarioAnimDamageFlyX1FileID, /* 535 */
    &llFTMarioAnimDamageFlyX2FileID, /* 536 */
    &llFTMarioAnimDamage2FileID, /* 537 */
    &llFTMarioAnimShieldBreakFileID, /* 538 */
    &llFTMarioAnimDamageFlyTopFileID, /* 539 */
    &llFTMarioAnimDamagedFileID, /* 540 */
    &llFTMarioAnimFallSpecialFileID, /* 541 */
    &llFTMarioAnimCeilingBonkFileID, /* 542 */
    &llFTMarioAnimDownBounceDFileID, /* 543 */
    &llFTMarioAnimStunLandUFileID, /* 544 */
    &llFTMarioAnimDownStandDFileID, /* 545 */
    &llFTMarioAnimStunStartUFileID, /* 546 */
    &llFTMarioAnimTechFFileID, /* 547 */
    &llFTMarioAnimTechBFileID, /* 548 */
    &llFTMarioAnimDownForwardDFileID, /* 549 */
    &llFTMarioAnimDownForwardUFileID, /* 550 */
    &llFTMarioAnimDownBackDFileID, /* 551 */
    &llFTMarioAnimDownBackUFileID, /* 552 */
    &llFTMarioAnimDownAttackDFileID, /* 553 */
    &llFTMarioAnimDownAttackUFileID, /* 554 */
    &llFTMarioAnimTechFileID, /* 555 */
    &llFTMarioAnimClangRecoilFileID, /* 556 */
    &llFTMarioAnimShieldOnFileID, /* 557 */
    &llFTMarioAnimShieldOffFileID, /* 558 */
    &llFTMarioAnimRollFFileID, /* 559 */
    &llFTMarioAnimRollBFileID, /* 560 */
    &llFTMarioAnimCatchFileID, /* 561 */
    &llFTMarioAnimCatchPullFileID, /* 562 */
    &llFTMarioAnimThrowFFileID, /* 563 */
    &llFTMarioAnimThrowBFileID, /* 564 */
    &llFTMarioAnimEggLayPulledFileID, /* 565 */
    &llFTMarioAnimThrownDKPulledFileID, /* 566 */
    &llFTMarioAnimThrownMarioBrosFileID, /* 567 */
    &llFTMarioAnimThrownDKFileID, /* 568 */
    &llFTMarioAnimThrown2FileID, /* 569 */
    &llFTMarioAnimThrown1FileID, /* 570 */
    &llFTMarioAnimTauntFileID, /* 571 */
    &llFTMarioAnimCliffCatchFileID, /* 572 */
    &llFTMarioAnimCliffWaitFileID, /* 573 */
    &llFTMarioAnimCliffQuickFileID, /* 574 */
    &llFTMarioAnimCliffClimbQuick1FileID, /* 575 */
    &llFTMarioAnimCliffClimbQuick2FileID, /* 576 */
    &llFTMarioAnimCliffSlowFileID, /* 577 */
    &llFTMarioAnimCliffClimbSlow1FileID, /* 578 */
    &llFTMarioAnimCliffClimbSlow2FileID, /* 579 */
    &llFTMarioAnimCliffAttackQuick1FileID, /* 580 */
    &llFTMarioAnimCliffAttackQuick2FileID, /* 581 */
    &llFTMarioAnimCliffAttackSlow1FileID, /* 582 */
    &llFTMarioAnimCliffAttackSlow2FileID, /* 583 */
    &llFTMarioAnimCliffEscapeQuick1FileID, /* 584 */
    &llFTMarioAnimCliffEscapeQuick2FileID, /* 585 */
    &llFTMarioAnimCliffEscapeSlow1FileID, /* 586 */
    &llFTMarioAnimCliffEscapeSlow2FileID, /* 587 */
    &llFTMarioAnimLightItemPickupFileID, /* 588 */
    &llFTMarioAnimItemThrowSmashBFileID, /* 589 */
    &llFTMarioAnimItemThrowSmashUFileID, /* 590 */
    &llFTMarioAnimItemThrowSmashDFileID, /* 591 */
    &llFTMarioAnimItemThrowDashFileID, /* 592 */
    &llFTMarioAnimItemDropFileID, /* 593 */
    &llFTMarioAnimItemThrowAirSmashBFileID, /* 594 */
    &llFTMarioAnimItemThrowAirSmashUFileID, /* 595 */
    &llFTMarioAnimItemThrowAirSmashFFileID, /* 596 */
    &llFTMarioAnimHeavyItemPickupFileID, /* 597 */
    &llFTMarioAnimHeavyItemThrowSmashBFileID, /* 598 */
    &llFTMarioAnimStarRodNeutralFileID, /* 599 */
    &llFTMarioAnimStarRodTiltFileID, /* 600 */
    &llFTMarioAnimStarRodSmashFileID, /* 601 */
    &llFTMarioAnimStarRodDashFileID, /* 602 */
    &llFTMarioAnimHammerIdleFileID, /* 603 */
    &llFTMarioAnimHammerLandingFileID, /* 604 */
    &llFTMarioAnimFireFlowerShootFileID, /* 605 */
    &llFTMarioAnimJab1FileID, /* 606 */
    &llFTMarioAnimJab2FileID, /* 607 */
    &llFTMarioAnimJab3FileID, /* 608 */
    &llFTMarioAnimDashAttackFileID, /* 609 */
    &llFTMarioAnimFTiltHighFileID, /* 610 */
    &llFTMarioAnimFTiltFileID, /* 611 */
    &llFTMarioAnimFTiltLowFileID, /* 612 */
    &llFTMarioAnimUTiltFileID, /* 613 */
    &llFTMarioAnimDTiltFileID, /* 614 */
    &llFTMarioAnimFSmashHighFileID, /* 615 */
    &llFTMarioAnimFSmashMidHighFileID, /* 616 */
    &llFTMarioAnimFSmashFileID, /* 617 */
    &llFTMarioAnimFSmashMidLowFileID, /* 618 */
    &llFTMarioAnimFSmashLowFileID, /* 619 */
    &llFTMarioAnimUSmashFileID, /* 620 */
    &llFTMarioAnimDSmashFileID, /* 621 */
    &llFTMarioAnimAttackAirNFileID, /* 622 */
    &llFTMarioAnimAttackAirFFileID, /* 623 */
    &llFTMarioAnimAttackAirBFileID, /* 624 */
    &llFTMarioAnimAttackAirUFileID, /* 625 */
    &llFTMarioAnimAttackAirDFileID, /* 626 */
    &llFTMarioAnimLandingAirFFileID, /* 627 */
    &llFTMarioAnimLandingAirBFileID, /* 628 */
    &llFTMarioAnimLandingAirUFileID, /* 629 */
    &llFTMarioAnimEnterPipeFileID, /* 630 */
    &llFTMarioAnimExitPipeFileID, /* 631 */
    &llFTMarioAnimExitPipeWalkFileID, /* 632 */
    &llFTMarioAnimAppear1FileID, /* 633 */
    &llFTMarioAnimAppear2FileID, /* 634 */
    &llFTMarioAnimFireballGroundFileID, /* 635 */
    &llFTMarioAnimFireballAirFileID, /* 636 */
    &llFTMarioAnimSuperJumpPunchAirFileID, /* 637 */
    &llFTMarioAnimMarioTornadoGroundFileID, /* 638 */
    &llFTMarioAnimMarioTornadoAirFileID, /* 639 */
    &llFTMarioAnimDamageFileID, /* 640 */
    &llFTMarioAnimFireFlowerShootAirFileID, /* 641 */
};

static u32 ndsRelocMarioBattleAnimAssetIDForToken(u32 token)
{
    u32 i;

    for (i = 0u;
         i < (sizeof(sNdsRelocMarioBattleAnimFileIDs) /
              sizeof(sNdsRelocMarioBattleAnimFileIDs[0]));
         i++)
    {
        if (token == ndsRelocFileID(sNdsRelocMarioBattleAnimFileIDs[i]))
        {
            return NDS_RELOC_ASSET_MARIO_ANIM_WAIT + i;
        }
    }
    return NDS_RELOC_ASSET_INVALID;
}

/* BattleShip files 642..799 are Fox's complete contiguous animation bank. */
static const uintptr_t *const sNdsRelocFoxAnimFileIDs[] =
{
    &llFTFoxAnimEggLayFileID, /* 642 */
    &llFTFoxAnimWalk1FileID, /* 643 */
    &llFTFoxAnimWalk2FileID, /* 644 */
    &llFTFoxAnimWalk3FileID, /* 645 */
    &llFTFoxAnimWalkEndFileID, /* 646 */
    &llFTFoxAnimDashFileID, /* 647 */
    &llFTFoxAnimRunFileID, /* 648 */
    &llFTFoxAnimRunBrakeFileID, /* 649 */
    &llFTFoxAnimTurnFileID, /* 650 */
    &llFTFoxAnimTurnRunFileID, /* 651 */
    &llFTFoxAnimJumpFFileID, /* 652 */
    &llFTFoxAnimJumpBFileID, /* 653 */
    &llFTFoxAnimJumpAerialFFileID, /* 654 */
    &llFTFoxAnimJumpAerialBFileID, /* 655 */
    &llFTFoxAnimFallFileID, /* 656 */
    &llFTFoxAnimFallAerialFileID, /* 657 */
    &llFTFoxAnimCrouchFileID, /* 658 */
    &llFTFoxAnimCrouchIdleFileID, /* 659 */
    &llFTFoxAnimCrouchEndFileID, /* 660 */
    &llFTFoxAnimLandingAirXFileID, /* 661 */
    &llFTFoxAnimShieldDropFileID, /* 662 */
    &llFTFoxAnimTeeterFileID, /* 663 */
    &llFTFoxAnimTeeterstartFileID, /* 664 */
    &llFTFoxAnimSleepFileID, /* 665 */
    &llFTFoxAnimDamaged1FileID, /* 666 */
    &llFTFoxAnimDamaged2FileID, /* 667 */
    &llFTFoxAnimFalconDivePulledFileID, /* 668 */
    &llFTFoxAnimDamageX1FileID, /* 669 */
    &llFTFoxAnimDamageX2FileID, /* 670 */
    &llFTFoxAnimDamageX3FileID, /* 671 */
    &llFTFoxAnimDamaged3FileID, /* 672 */
    &llFTFoxAnimDamaged4FileID, /* 673 */
    &llFTFoxAnimDamaged5FileID, /* 674 */
    &llFTFoxAnimDamaged6FileID, /* 675 */
    &llFTFoxAnimDamageAirFileID, /* 676 */
    &llFTFoxAnimDamaged7FileID, /* 677 */
    &llFTFoxAnimDamageFlyX1FileID, /* 678 */
    &llFTFoxAnimDamageFlyX2FileID, /* 679 */
    &llFTFoxAnimDamage2FileID, /* 680 */
    &llFTFoxAnimShieldBreakFileID, /* 681 */
    &llFTFoxAnimDamageFlyTopFileID, /* 682 */
    &llFTFoxAnimDamagedFileID, /* 683 */
    &llFTFoxAnimFallSpecialFileID, /* 684 */
    &llFTFoxAnimCeilingBonkFileID, /* 685 */
    &llFTFoxAnimStunLandDFileID, /* 686 */
    &llFTFoxAnimStunLandUFileID, /* 687 */
    &llFTFoxAnimStunStartDFileID, /* 688 */
    &llFTFoxAnimStunStartUFileID, /* 689 */
    &llFTFoxAnimTechFFileID, /* 690 */
    &llFTFoxAnimTechBFileID, /* 691 */
    &llFTFoxAnimDownForwardDFileID, /* 692 */
    &llFTFoxAnimDownForwardUFileID, /* 693 */
    &llFTFoxAnimDownBackDFileID, /* 694 */
    &llFTFoxAnimDownBackUFileID, /* 695 */
    &llFTFoxAnimDownAttackDFileID, /* 696 */
    &llFTFoxAnimDownAttackUFileID, /* 697 */
    &llFTFoxAnimTechFileID, /* 698 */
    &llFTFoxAnimClangRecoilFileID, /* 699 */
    &llFTFoxAnimShieldOnFileID, /* 700 */
    &llFTFoxAnimShieldOffFileID, /* 701 */
    &llFTFoxAnimRollFFileID, /* 702 */
    &llFTFoxAnimRollBFileID, /* 703 */
    &llFTFoxAnimCatchFileID, /* 704 */
    &llFTFoxAnimCatchPullFileID, /* 705 */
    &llFTFoxAnimThrowFFileID, /* 706 */
    &llFTFoxAnimThrowBFileID, /* 707 */
    &llFTFoxAnimEggLayPulledFileID, /* 708 */
    &llFTFoxAnimThrownDKPulledFileID, /* 709 */
    &llFTFoxAnimThrownMarioBrosFileID, /* 710 */
    &llFTFoxAnimThrownDKFileID, /* 711 */
    &llFTFoxAnimThrown2FileID, /* 712 */
    &llFTFoxAnimThrown1FileID, /* 713 */
    &llFTFoxAnimThrown3FileID, /* 714 */
    &llFTFoxAnimThrownFoxBFileID, /* 715 */
    &llFTFoxAnimTauntFileID, /* 716 */
    &llFTFoxAnimCliffCatchFileID, /* 717 */
    &llFTFoxAnimCliffWaitFileID, /* 718 */
    &llFTFoxAnimCliffQuickFileID, /* 719 */
    &llFTFoxAnimCliffClimbQuick1FileID, /* 720 */
    &llFTFoxAnimCliffClimbQuick2FileID, /* 721 */
    &llFTFoxAnimCliffSlowFileID, /* 722 */
    &llFTFoxAnimCliffClimbSlow1FileID, /* 723 */
    &llFTFoxAnimCliffClimbSlow2FileID, /* 724 */
    &llFTFoxAnimCliffAttackQuick1FileID, /* 725 */
    &llFTFoxAnimCliffAttackQuick2FileID, /* 726 */
    &llFTFoxAnimCliffAttackSlow1FileID, /* 727 */
    &llFTFoxAnimCliffAttackSlow2FileID, /* 728 */
    &llFTFoxAnimCliffEscapeQuick1FileID, /* 729 */
    &llFTFoxAnimCliffEscapeQuick2FileID, /* 730 */
    &llFTFoxAnimCliffEscapeSlow1FileID, /* 731 */
    &llFTFoxAnimCliffEscapeSlow2FileID, /* 732 */
    &llFTFoxAnimLightItemPickupFileID, /* 733 */
    &llFTFoxAnimItemThrowSmashBFileID, /* 734 */
    &llFTFoxAnimItemThrowSmashUFileID, /* 735 */
    &llFTFoxAnimItemThrowSmashDFileID, /* 736 */
    &llFTFoxAnimItemThrowDashFileID, /* 737 */
    &llFTFoxAnimItemDropFileID, /* 738 */
    &llFTFoxAnimItemThrowAirSmashBFileID, /* 739 */
    &llFTFoxAnimItemThrowAirSmashUFileID, /* 740 */
    &llFTFoxAnimItemThrowAirSmashFFileID, /* 741 */
    &llFTFoxAnimHeavyItemPickupFileID, /* 742 */
    &llFTFoxAnimHeavyItemThrowSmashBFileID, /* 743 */
    &llFTFoxAnimStarRodNeutralFileID, /* 744 */
    &llFTFoxAnimStarRodTiltFileID, /* 745 */
    &llFTFoxAnimStarRodSmashFileID, /* 746 */
    &llFTFoxAnimStarRodDashFileID, /* 747 */
    &llFTFoxAnimHammerIdleFileID, /* 748 */
    &llFTFoxAnimHammerLandingFileID, /* 749 */
    &llFTFoxAnimFireFlowerShootFileID, /* 750 */
    &llFTFoxAnimJab1FileID, /* 751 */
    &llFTFoxAnimJab2FileID, /* 752 */
    &llFTFoxAnimJabLoopStartFileID, /* 753 */
    &llFTFoxAnimJabLoopFileID, /* 754 */
    &llFTFoxAnimJabLoopEndFileID, /* 755 */
    &llFTFoxAnimDashAttackFileID, /* 756 */
    &llFTFoxAnimFTiltHighFileID, /* 757 */
    &llFTFoxAnimFTiltMidHighFileID, /* 758 */
    &llFTFoxAnimFTiltFileID, /* 759 */
    &llFTFoxAnimFTiltMidLowFileID, /* 760 */
    &llFTFoxAnimFTiltLowFileID, /* 761 */
    &llFTFoxAnimUTiltFileID, /* 762 */
    &llFTFoxAnimDTiltFileID, /* 763 */
    &llFTFoxAnimFSmashFileID, /* 764 */
    &llFTFoxAnimUSmashFileID, /* 765 */
    &llFTFoxAnimDSmashFileID, /* 766 */
    &llFTFoxAnimAttackAirNFileID, /* 767 */
    &llFTFoxAnimAttackAirFFileID, /* 768 */
    &llFTFoxAnimAttackAirBFileID, /* 769 */
    &llFTFoxAnimAttackAirUFileID, /* 770 */
    &llFTFoxAnimAttackAirDFileID, /* 771 */
    &llFTFoxAnimLandingAirFFileID, /* 772 */
    &llFTFoxAnimLandingAirBFileID, /* 773 */
    &llFTFoxAnimEnterPipeFileID, /* 774 */
    &llFTFoxAnimExitPipeFileID, /* 775 */
    &llFTFoxAnimExitPipeWalkFileID, /* 776 */
    &llFTFoxAnimAppearFileID, /* 777 */
    &llFTFoxAnimArwingFileID, /* 778 */
    &llFTFoxAnimLaserFileID, /* 779 */
    &llFTFoxAnimLaserAerialFileID, /* 780 */
    &llFTFoxAnimFireFoxStartGroundFileID, /* 781 */
    &llFTFoxAnimReadyingFireFoxGroundFileID, /* 782 */
    &llFTFoxAnimFireFoxGroundFileID, /* 783 */
    &llFTFoxAnimFireFoxEndGroundFileID, /* 784 */
    &llFTFoxAnimFireFoxStartAerialFileID, /* 785 */
    &llFTFoxAnimReadyingFireFoxAirFileID, /* 786 */
    &llFTFoxAnimFireFoxAirFileID, /* 787 */
    &llFTFoxAnimFireFoxEndAirFileID, /* 788 */
    &llFTFoxAnimLandingWhileFireFoxAirFileID, /* 789 */
    &llFTFoxAnimShineStartFileID, /* 790 */
    &llFTFoxAnimSwitchDirectionShineFileID, /* 791 */
    &llFTFoxAnimReflectingFileID, /* 792 */
    &llFTFoxAnimShineFileID, /* 793 */
    &llFTFoxAnimShireStartAirFileID, /* 794 */
    &llFTFoxAnimSwitchDirectionShineAirFileID, /* 795 */
    &llFTFoxAnimUnknownFileID, /* 796 */
    &llFTFoxAnimShineAirEndFileID, /* 797 */
    &llFTFoxAnimDamageFileID, /* 798 */
    &llFTFoxAnimFireFlowerShootAirFileID, /* 799 */
};

static u32 ndsRelocFoxAnimAssetIDForToken(u32 token)
{
    u32 i;

    for (i = 0u;
         i < (sizeof(sNdsRelocFoxAnimFileIDs) /
              sizeof(sNdsRelocFoxAnimFileIDs[0]));
         i++)
    {
        if (token == ndsRelocFileID(sNdsRelocFoxAnimFileIDs[i]))
        {
            return NDS_RELOC_ASSET_FOX_ANIM_FIRST + i;
        }
    }
    return NDS_RELOC_ASSET_INVALID;
}

/* Task 74 tried memoizing this. ndsRelocFileID returns (u32)(uintptr_t)file_id,
 * the address of the file-id global rather than a value read from it, so every
 * token below is a link-time constant and the chain is a pure function of its
 * argument -- a direct-mapped cache in front of it was provably safe, and Task
 * 71 had priced the chain at 9,306 ticks/frame on a load frame.
 *
 * It measured worse. SRC, the bucket it targets, rose 1,920 at P95 and WORK-H
 * rose 11,584 at P50, while STG -- which a token lookup cannot touch -- moved
 * 8,128. The 512 bytes of cache arrays shift layout, and this ROM's placement
 * sensitivity puts the noise floor above the effect being chased. A hundred
 * compares against link-time immediates are branch-predictable and already
 * resident; three lookup arrays in .main.bss are not.
 *
 * Do not re-attempt without an instrument that resolves below ~8,000 ticks.
 *
 * R2-06 E10 built that instrument -- per-frame profiler regions split by a
 * marker symbol, ~1 tick resolution -- and R2-06 E11 then used it to REFUTE the
 * whole line, including a fix that added no data at all. What it measured here:
 *
 *   - 630 calls on the 16 load frames of window 796..923, ZERO on all 112 clean
 *     frames, 39,475 ticks per load frame, 1,003 cycles / 550 instructions each.
 *   - Task 71's 9,306 was 4.2x low, because ndsRelocMarioBattleAnimAssetIDForToken,
 *     ndsRelocFoxAnimAssetIDForToken and ndsRelocIsMarioFoxAnimID all inline into
 *     this function; it priced the compare chain and not the scans behind it.
 *   - The chain is NOT the cost. 74.3% of calls resolve inside it. The two
 *     inlined pointer scans are 51.6% of the function on 14.3% of calls, because
 *     a full miss walks all 143 + 158 entries. So hoisting the scans, the
 *     obvious-looking fix, would have made 74.3% of calls pay 301 iterations.
 *   - The Mario/Fox pointer arrays cannot become index arithmetic: their targets
 *     span 1.7 MB non-monotonically (density 0.0%).
 *
 * E11 hoisted the ndsRelocIsMarioFoxAnimID range check above both scans and
 * deleted the five dead WAIT/WALK compares it subsumed -- provably identical,
 * NEGATIVE bytes added, and it worked: the function fell to 31,808 (-7,667,
 * -5,103 instructions) with the load-frame set bit-identical. It still lost.
 * Against a matched control (r206-e11-{control,tokenfirst}-128) the body improved
 * at every percentile below P90 and the tail did not: WORK-H P95 +15,744, P99
 * +59,200, over-gate 9 -> 11, and the two added frames were load frames 828 and
 * 847. Control-to-control noise on this ROM is P95 +5,376 and +/-1 over-gate.
 *
 * The lesson is about size, not about this function: a load-frame-only saving of
 * ~8,000 ticks cannot be banked through P95 here, because relinking moves the
 * tail by more than the saving. Do not bring another small load-frame cut. Either
 * remove this work in one change large enough to clear ~16,000 of tail movement,
 * or move it off the gameplay frame entirely, which changes WHEN the work happens
 * instead of shuffling where the code sits. */
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
    if (token == ndsRelocFileID(&llMNVSResultsFileID)) return NDS_RELOC_ASSET_MN_VS_RESULTS;
    if (token == ndsRelocFileID(&llFTEmblemModelsFileID)) return NDS_RELOC_ASSET_FT_EMBLEM_MODELS;
    if (token == ndsRelocFileID(&llFTStocksZakoFileID)) return NDS_RELOC_ASSET_FT_STOCKS_ZAKO;
    if (token == ndsRelocFileID(&llLBTransitionAeroplaneFileID)) return NDS_RELOC_ASSET_TRANSITION_AEROPLANE;
    if (token == ndsRelocFileID(&llLBTransitionCheckFileID)) return NDS_RELOC_ASSET_TRANSITION_CHECK;
    if (token == ndsRelocFileID(&llLBTransitionGakubuthiFileID)) return NDS_RELOC_ASSET_TRANSITION_GAKUBUTHI;
    if (token == ndsRelocFileID(&llLBTransitionKannonFileID)) return NDS_RELOC_ASSET_TRANSITION_KANNON;
    if (token == ndsRelocFileID(&llLBTransitionStarFileID)) return NDS_RELOC_ASSET_TRANSITION_STAR;
    if (token == ndsRelocFileID(&llLBTransitionSudare1FileID)) return NDS_RELOC_ASSET_TRANSITION_SUDARE1;
    if (token == ndsRelocFileID(&llLBTransitionSudare2FileID)) return NDS_RELOC_ASSET_TRANSITION_SUDARE2;
    if (token == ndsRelocFileID(&llLBTransitionBlockFileID)) return NDS_RELOC_ASSET_TRANSITION_BLOCK;
    if (token == ndsRelocFileID(&llLBTransitionRotScaleFileID)) return NDS_RELOC_ASSET_TRANSITION_ROTSCALE;
    if (token == ndsRelocFileID(&llLBTransitionCurtainFileID)) return NDS_RELOC_ASSET_TRANSITION_CURTAIN;
    if (token == ndsRelocFileID(&llLBTransitionCameraFileID)) return NDS_RELOC_ASSET_TRANSITION_CAMERA;
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
    {
        u32 mario_anim_asset_id =
            ndsRelocMarioBattleAnimAssetIDForToken(token);

        if (mario_anim_asset_id != NDS_RELOC_ASSET_INVALID)
        {
            return mario_anim_asset_id;
        }
    }
    {
        u32 fox_anim_asset_id = ndsRelocFoxAnimAssetIDForToken(token);

        if (fox_anim_asset_id != NDS_RELOC_ASSET_INVALID)
        {
            return fox_anim_asset_id;
        }
    }
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WAIT) return NDS_RELOC_ASSET_MARIO_ANIM_WAIT;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK1) return NDS_RELOC_ASSET_MARIO_ANIM_WALK1;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK2) return NDS_RELOC_ASSET_MARIO_ANIM_WALK2;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK3) return NDS_RELOC_ASSET_MARIO_ANIM_WALK3;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK_END) return NDS_RELOC_ASSET_MARIO_ANIM_WALK_END;
    if (ndsRelocIsMarioFoxAnimID(token) != FALSE) return token;
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

extern void ndsAObjEvent32ResetNormalizedScripts(void);

#if NDS_TASK44_STAGE_STEADY
/* Task 44 item 3: the Dream Land stage-asset mutation generation.
 *
 * The complete-stage owner used to re-look-up its four assets and rebuild the
 * 57-DObj topology stamp on every preparation just to learn nothing had moved.
 * Steady-state admission now compares this counter instead, so EVERY path that
 * can replace, unload, or invalidate one of those four files must bump it.
 * The complete enumeration is exactly three seams, all of them below:
 *
 *   1. ndsRelocResetLoadedFiles  - every unload/reset path funnels here
 *      (scene-cache eviction, title backend teardown).
 *   2. ndsRelocRegisterLoadedFile - the only writer of NDSRelocLoadedFile::data;
 *      covers both a first load and a same-asset-id replacement in place.
 *      Bumped only for the four Dream Land ids, so fighter/animation loads do
 *      not force stage revalidation.
 *   3. ndsRelocPrepareSceneCache - bumps sNdsRelocSceneGeneration; covered
 *      explicitly because that path skips the reset when nothing was resident.
 *
 * It starts at 1 so a zero in the owner workspace always means "never
 * admitted" and takes the full validation path.
 */
static u32 sNdsRelocStageAssetMutation = 1u;

static s32 ndsRelocIsDreamLandStageAsset(u32 asset_id)
{
    return ((asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_103) ||
            (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_104) ||
            (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_152) ||
            (asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP)) ? TRUE : FALSE;
}

static void ndsRelocBumpStageAssetMutation(void)
{
    sNdsRelocStageAssetMutation++;
    if (sNdsRelocStageAssetMutation == 0u)
    {
        sNdsRelocStageAssetMutation = 1u;
    }
}
#endif

static void ndsRelocResetLoadedFiles(void)
{
    ndsAObjEvent32ResetNormalizedScripts();
#if NDS_TASK44_STAGE_STEADY
    ndsRelocBumpStageAssetMutation();
#endif
    memset(sNdsRelocLoadedFiles, 0, sizeof(sNdsRelocLoadedFiles));
    sNdsRelocLoadedFileCount = 0;
    memset(sNdsRelocNormalizedMObjSubs, 0,
           sizeof(sNdsRelocNormalizedMObjSubs));
    sNdsRelocNormalizedMObjSubCount = 0u;
    ndsFighterMarioFoxResetFileSlots();
}

static void ndsRelocPrepareSceneCache(void)
{
    u32 scene = (u32)gSCManagerSceneData.scene_curr;
    u32 evicted_files = 0u;
    u32 evicted_bytes = 0u;
    u32 i;

    /* Scene KIND is not scene INSTANCE, and this guard used to conflate them.
     *
     * Everything below is the port's only invalidation for a scene change: it
     * evicts the resident reloc files, discards the battle texture keys that
     * hold native pointers INTO those files, and bumps sNdsRelocSceneGeneration
     * -- which becomes each asset's owner_generation, which is where
     * topology_generation comes from, which is the only thing that resets the
     * Task 36 replay. Skipping it leaves every generation-keyed cache believing
     * data is live after the taskman heap has been rewound under it.
     *
     * Skipping on an equal kind is right for a repeated call WITHIN a scene and
     * wrong for a RE-ENTRY into the same kind. Sudden Death is exactly that --
     * nSCKindVSBattle to nSCKindVSBattle -- so it evicted nothing, discarded no
     * texture keys, and never advanced the generation. The owner's screenshot of
     * that state is duplicated fighter sprites at wrong positions over corrupt
     * stage geometry, drawn at a healthy 28.9 FPS with FTR 385,728: not a cost
     * defect, stale bindings replayed against reused memory.
     *
     * So also treat the resident set as stale when it no longer lives inside the
     * live taskman heap. A scene load rewinds that heap, so any file whose data
     * now sits at or above the bump cursor belongs to the previous instance
     * whatever the kind says. This is the same ownership test
     * ndsR2AnimCacheArenaStillOwned already uses for the animation arena, which
     * is why that cache survived this bug and these did not. */
    if (sNdsRelocOwnerScene == scene)
    {
        const u8 *heap_start = (const u8 *)gSYTaskmanGeneralHeap.start;
        const u8 *heap_cursor = (const u8 *)gSYTaskmanGeneralHeap.ptr;
        sb32 resident_is_stale = FALSE;

        /* THE HEAP GENERATION IS THE AUTHORITY, not the cursor.
         *
         * The cursor test below was this guard's original re-entry fix, and it
         * is unsound for the same reason it was unsound in the animation cache:
         * a rewind does not move our data, it moves the CURSOR, and the new
         * scene's own allocations push that cursor straight back past the stale
         * files. Every one of them is then inside [start, cursor) again, the
         * scan finds nothing, and this function early-returns having evicted
         * nothing, discarded no texture keys or OAM names, and never advanced
         * sNdsRelocSceneGeneration. Whether it fires is a race between when
         * this runs and how much the new scene has allocated -- which is why
         * the symptom is intermittent and why it presents as wrong textures on
         * a second entry rather than as a crash.
         *
         * gNdsTaskmanHeapGeneration cannot be raced: it is bumped at the two
         * primitives that rewind the heap, before any new-scene allocation. */
        if (sNdsRelocResidentHeapGeneration != gNdsTaskmanHeapGeneration)
        {
            resident_is_stale = TRUE;
            gNdsRelocSceneReentryGenerationEvictCount++;
        }
        else
        {
            /* Kept as a SECONDARY corruption check: same generation but a file
             * outside the live region means something other than a scene
             * rewind moved it, and that is worth evicting on too. */
            for (i = 0; i < sNdsRelocLoadedFileCount; i++)
            {
                const u8 *data = (const u8 *)sNdsRelocLoadedFiles[i].data;

                if ((data != NULL) &&
                    ((data < heap_start) || (data >= heap_cursor)))
                {
                    resident_is_stale = TRUE;
                    gNdsRelocSceneReentryRangeEvictCount++;
                    break;
                }
            }
        }
        if (resident_is_stale == FALSE)
        {
            return;
        }
        gNdsRelocSceneReentryEvictCount++;
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        evicted_files++;
        evicted_bytes += sNdsRelocLoadedFiles[i].data_size;
    }

    if (evicted_files != 0u)
    {
#if NDS_RENDERER_HW_TRIANGLES
        /* Texture keys hold native pointers into the scene-owned reloc files.
         * Drop every battle entry before those files become stale. */
        ndsRendererHardwareDiscardBattleStaticTextures();
        /* And the native OAM path's retained texture NAMES, which point at the
         * VRAM the line above just released. They were cleared only by
         * ndsIFCommonNativeOamInit, which runs once at boot, so after a scene
         * reload ndsIFCommonNativeOamPrepareClouds saw them non-zero and
         * early-returned TRUE without re-uploading -- leaving the sprite path
         * emitting handles into reused VRAM. Same shape as the
         * sNdsRendererBattleStaticTexturePrepared latch, which already had its
         * invalidation wired; this one did not. */
        ndsIFCommonNativeOamDiscardTextures();
#endif
        ndsRelocResetLoadedFiles();
    }
    sNdsRelocStatusBufferCount = 0;
    sNdsRelocForceStatusBufferCount = 0;

    sNdsRelocOwnerScene = scene;
    /* Stamp the heap generation this resident set belongs to. Read here, after
     * the eviction above, because this is the point the set becomes current. */
    sNdsRelocResidentHeapGeneration = gNdsTaskmanHeapGeneration;
    sNdsRelocSceneGeneration++;
#if NDS_TASK44_STAGE_STEADY
    /* Seam 3: a scene change with nothing resident skips the reset above, so
     * the stage owner would otherwise never learn its generation moved. */
    ndsRelocBumpStageAssetMutation();
#endif
#if NDS_RENDERER_HW_TRIANGLES
    /* The adapter's frame-local matrix storage lives in this scene's original
     * taskman heap. Drop its pointer before that heap can be reused. */
    ndsRendererAdapterResetSceneCaches();
#endif
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
    return ndsRelocIsMarioFoxAnimID(asset_id);
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

s32 ndsRelocGetLoadedAssetView(u32 asset_id, const void **out_data,
                               u32 *out_size)
{
    NDSRelocLoadedFile *loaded;

    if ((out_data == NULL) || (out_size == NULL))
    {
        return FALSE;
    }
    *out_data = NULL;
    *out_size = 0u;
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if ((loaded == NULL) || (loaded->data == NULL) ||
        (loaded->data_size == 0u) ||
        (loaded->owner_scene != (u32)gSCManagerSceneData.scene_curr) ||
        (loaded->owner_generation != sNdsRelocSceneGeneration))
    {
        return FALSE;
    }
    *out_data = loaded->data;
    *out_size = loaded->data_size;
    return TRUE;
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

/* R2-07 cycle 109. The memo is four-way move-to-front, not one-deep.
 *
 * This is 1 of the 30 callers' shared lookup and it sits under the fighter draw
 * path: `ndsRendererAdapterValidateNativeOwnerMaterials` asks it for
 * `palette_image`, `block_image` and `current_image` -- THREE DIFFERENT pointers
 * per material, per fighter, per frame -- and `ndsFighterDrawPlanResolve`,
 * `ndsFighterDLDraw*`, `ndsFighterDLScan*` and `ndsFighterDLExec*` interleave
 * their own streams on top, so a one-entry memo is being asked to serve several
 * interleaved streams.
 *
 * MEASURED, and it corrects the premise this was written on. Over a 60-second
 * both-CPU match: 30,385 calls, way 0 alone **25,434 (83.7%)**, ways 1-3
 * **4,385 more (14.4%)**, only 1,014 reaching the linear scan. So the one-entry
 * memo was already catching most of it, and the total volume is ~15 calls per
 * frame rather than the ~178,000 per match the material seam's call count
 * suggested -- because `ValidateNativeOwnerMaterials` sits behind the owner
 * validate cache, which cycle 98 measured at 3,961/2, so its three searches per
 * material are already elided ~999 times in 1,000.
 *
 * The 4,385 deleted scans are worth on the order of 500 ticks/frame: real,
 * repeatable, free in footprint (binary byte-identical, 160 bytes of headroom),
 * and far below anything this instrument can resolve. Banked under "keep every
 * repeatable correctness-preserving gain", not headlined. Do not brief this as a
 * lever; the counters are kept so the seam never has to be guessed at again.
 *
 * Four ways, checked most-recently-used first, so K=1 behaviour is the w==0 case
 * and nothing about the search ORDER changes for a single stream.
 *
 * Return-value equivalence rests on at most ONE entry being able to match a
 * given pointer, and the non-obvious half of that is the boundary:
 * `ndsRelocPointerRangeInLoadedFile` accepts `addr == base + data_size` (it
 * tests `>`, not `>=`), so two adjacent allocations would both appear to contain
 * a pointer sitting exactly on the seam. They do not, because the inner
 * `ndsRelocRangeInLoadedFile` then rejects `size > data_size - offset`, and at
 * that boundary the remainder is 0 while every caller passes size >= 1. So
 * "first match in scan order" and "the matching way" name the same file. Where
 * the previous code already preferred its one memo over scan order, this prefers
 * one of four the same way -- the hazard class is unchanged, not widened.
 *
 * `ndsRelocPointerRangeInLoadedFile` is two compares and a range check, so a way
 * that misses costs almost nothing; the win is deleting whole scans. */
#define NDS_RELOC_FIND_MEMO_WAYS 4u

volatile u32 gNdsRelocFindMemoHits;
volatile u32 gNdsRelocFindMemoWay0;
volatile u32 gNdsRelocFindMemoScans;
volatile u32 gNdsRelocFindMemoAbsent;

static NDSRelocLoadedFile *ndsRelocFindLoadedFileContaining(const void *ptr,
                                                             size_t size)
{
    static u32 memo[NDS_RELOC_FIND_MEMO_WAYS] = {
        0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
    };
    u32 w;
    u32 i;

    for (w = 0u; w < NDS_RELOC_FIND_MEMO_WAYS; w++)
    {
        u32 idx = memo[w];

        if ((idx < sNdsRelocLoadedFileCount) &&
            (ndsRelocPointerRangeInLoadedFile(&sNdsRelocLoadedFiles[idx],
                                              ptr,
                                              size) != FALSE))
        {
            gNdsRelocFindMemoHits++;
            if (w == 0u)
            {
                gNdsRelocFindMemoWay0++;
            }
            else
            {
                /* Move to front, so a stream that keeps hitting way 3 does not
                 * keep paying for three probes. */
                while (w > 0u)
                {
                    memo[w] = memo[w - 1u];
                    w--;
                }
                memo[0] = idx;
            }
            return &sNdsRelocLoadedFiles[idx];
        }
    }

    gNdsRelocFindMemoScans++;
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                             ptr,
                                             size) != FALSE)
        {
            for (w = NDS_RELOC_FIND_MEMO_WAYS - 1u; w > 0u; w--)
            {
                memo[w] = memo[w - 1u];
            }
            memo[0] = i;
            return &sNdsRelocLoadedFiles[i];
        }
    }
    gNdsRelocFindMemoAbsent++;
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
    NDSRelocLoadedFile *loaded;

    /* The pack first, and it is O(1): a resident clip is never registered as a
     * loaded file, so without this the resolver would fall through to the
     * status buffers, find the clip's asset id mapped to the pack pointer, and
     * rebase slot words against a data_size taken from the FILE's header rather
     * than the blob's extent. Answering here is both faster and the only
     * correct bound. */
    if (ndsBattlePackContains(ptr, size, out_base, out_size) != FALSE)
    {
        return TRUE;
    }

    loaded = ndsRelocFindLoadedFileContaining(ptr, size);
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

/* Offset counts the file-relative fallback below; Misalign counts the words that
 * fallback would have turned into an address no aligned type can live at, and
 * Value is the last one. Misalign > 0 is the freeze class being refused at its
 * source. Offset > 0 with Misalign 0 says the resolver ran and had nothing to
 * refuse -- which is a different statement from "this never ran", and the two
 * were indistinguishable before. Defined here for the same reason
 * gNdsRelocHeapDeclineCount is: this file's counter block is further down. */
volatile u32 gNdsRelocResolveOffsetCount;
volatile u32 gNdsRelocResolveMisalignCount;
volatile u32 gNdsRelocResolveMisalignValue;

/* THE SHIELD FREEZE, at the seam that manufactures the pointer.
 *
 * This resolver's contract is "give me a pointer to a `size`-byte object of this
 * file, or NULL". It had no alignment requirement, so its offset fallback --
 * `base + raw` for ANY stored word no larger than the file -- happily returned
 * addresses that no aligned type can live at. A figatree slot holding a plain
 * data word rather than a relocated pointer resolves that way: the caller
 * (lbCommonAddFighterPartsFigatree) hands it to gcAddDObjAnimJoint, and
 * gcParseDObjAnimJoint then walks it and never terminates, because a misaligned
 * LDR on ARM9 rotates the word it reads and the opcodes come out as noise. The
 * three captures on 2026-08-02/03 all froze there on 0x23842ea -- an in-file
 * address that is 2 mod 4, which is exactly the shape this function produced and
 * nothing else in the port produces.
 *
 * A misaligned result is therefore not a pointer to refuse to trust; it is not a
 * pointer at all. Return NULL, which every caller already handles -- the joint
 * gets AOBJ_ANIM_NULL and no animation, the same answer a NULL slot already
 * gets -- and count it, because "no misaligned words were seen" and "this code
 * never ran" have to be distinguishable. */
void *ndsRelocResolvePointerFromFileBase(const void *file_base,
                                         const void *ptr,
                                         size_t size)
{
    const void *base = NULL;
    size_t data_size = 0u;
    uintptr_t raw;
    uintptr_t align_mask;
    uintptr_t resolved;

    if (ptr == NULL)
    {
        return NULL;
    }
    align_mask = (size >= sizeof(u32)) ? (sizeof(u32) - 1u)
                                       : ((size >= sizeof(u16)) ? 1u : 0u);

    /* THE PACK'S SLOT WORDS ARE OFFSETS BY CONSTRUCTION, SO ASK THAT QUESTION
     * FIRST -- measured, this ordering was 92.4% of the resident pack's whole
     * P95 excess (`artifacts/performance/2026-08-15_battlepack-mechanism/`).
     *
     * The probe below asks "is `ptr` ALREADY an absolute pointer into a known
     * file?". For a clip served from the pack it can never be: the generator
     * emits blob-relative byte offsets, so `ptr` is a small integer. Its MISS
     * path is the expensive one -- `ndsRelocFindKnownFileContaining` falls
     * through the loaded-file scan into `ndsRelocFindStatusNodeContaining` over
     * BOTH status buffers, and every node visited there runs
     * `ndsRelocStatusNodeDataSize` -> `ndsRelocAssetIDForToken`, a ~300-compare
     * chain whose full miss also walks the 143 + 158 Mario/Fox pointer arrays.
     * At ~136 node visits and ~1,003 cycles each that is ~136,000 cycles PER
     * SLOT, and a fighter action change resolves ~18 of them: v3-arm measured
     * `ndsRelocAssetIDForToken` 207.9M cycles and
     * `ndsRelocFindStatusNodeContaining` 113.6M over 641 frames, 99.8%/100.0%
     * of it on the 69 frames that carry an acquisition.
     *
     * Answering from the pack costs one O(1) range test. It cannot change a
     * result: `data_size` is the blob extent (287,904 B) and every DS address is
     * >= 0x02000000, so a word that is genuinely an absolute pointer fails the
     * bound below, falls through, and takes the original path -- where
     * `ndsBattlePackContains` answers it O(1) anyway. Off a pack build this is a
     * compiled-out stub returning FALSE and the function is byte-identical.
     *
     * `#if`'d rather than left to that stub because a cross-TU call is NOT free
     * at flag 0 -- there is no LTO here, so the shipped ROM would grow a call
     * and a branch for a question that can never be answered yes. At flag 0 this
     * file must still compile to the same bytes it does today. */
#if NDS_R2_BATTLEPACK
    if (ndsBattlePackContains(file_base, 1u, &base, &data_size) != FALSE)
    {
        raw = (uintptr_t)ptr;
        if ((raw <= data_size) && (size <= (size_t)(data_size - raw)))
        {
            gNdsRelocResolveOffsetCount++;
            resolved = (uintptr_t)base + raw;
            if ((resolved & align_mask) != 0u)
            {
                gNdsRelocResolveMisalignCount++;
                gNdsRelocResolveMisalignValue = (u32)resolved;
                return NULL;
            }
            return (u8 *)base + raw;
        }
        base = NULL;
        data_size = 0u;
    }
#endif /* NDS_R2_BATTLEPACK */

    if (ndsRelocFindKnownFileContaining(ptr, size, NULL, NULL) != FALSE)
    {
        if (((uintptr_t)ptr & align_mask) != 0u)
        {
            gNdsRelocResolveMisalignCount++;
            gNdsRelocResolveMisalignValue = (u32)(uintptr_t)ptr;
            return NULL;
        }
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
    gNdsRelocResolveOffsetCount++;
    resolved = (uintptr_t)base + raw;
    if ((resolved & align_mask) != 0u)
    {
        gNdsRelocResolveMisalignCount++;
        gNdsRelocResolveMisalignValue = (u32)resolved;
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

#if NDS_TASK44_STAGE_STEADY
    /* Seam 2: this is the only writer of ->data, so a new load and a same-id
     * replacement are both caught here. */
    if (ndsRelocIsDreamLandStageAsset(asset_id) != FALSE)
    {
        ndsRelocBumpStageAssetMutation();
    }
#endif
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
    return ((ndsRelocIsMarioFoxAnimID(asset_id) != FALSE) &&
            (asset_id != NDS_RELOC_ASSET_MARIO_ANIM_APPEAR1) &&
            (asset_id != NDS_RELOC_ASSET_MARIO_ANIM_APPEAR2) &&
            (asset_id != NDS_RELOC_ASSET_FOX_ANIM_APPEAR) &&
            (asset_id != NDS_RELOC_ASSET_FOX_ANIM_ARWING)) ? TRUE : FALSE;
}

static s32 ndsRelocIsFighterAObj32Asset(u32 asset_id)
{
    return ((asset_id == NDS_RELOC_ASSET_MARIO_ANIM_APPEAR1) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_APPEAR2) ||
            (asset_id == NDS_RELOC_ASSET_FOX_ANIM_APPEAR) ||
            (asset_id == NDS_RELOC_ASSET_FOX_ANIM_ARWING)) ? TRUE : FALSE;
}

s32 ndsRelocPointerIsFighterAObj16(const void *ptr)
{
    NDSRelocLoadedFile *loaded;

    /* THE ADMISSION GATE, and the reason a resident clip has to be named here.
     *
     * `gcAddDObjAnimJoint` (`battleship_sys_objanim.c:1710`) admits a script
     * only if it is NULL, or this says AObj16, or `ndsAObjEvent32NormalizeScript`
     * succeeds on it. A pack clip is deliberately NOT registered as a loaded
     * file -- that registration is one of the things slice 1 phase 5 deletes --
     * so without this branch every packed script would fall through to the
     * AObjEvent32 normalizer, which would either refuse it (a joint silently
     * loses its animation) or write through a `const` pointer.
     *
     * The pack contains AObj16 fighter scripts and nothing else: the generator
     * routes the four AObjEvent32 ids (0x279 0x27a 0x309 0x30a) out by name,
     * never silently. */
    if (ndsBattlePackContains(ptr, sizeof(u16), NULL, NULL) != FALSE)
    {
        return TRUE;
    }

    loaded = ndsRelocFindLoadedFileContaining(ptr, sizeof(u16));
    return ((loaded != NULL) &&
            (ndsRelocIsFighterAObj16Asset(loaded->asset_id) != FALSE)) ?
               TRUE : FALSE;
}

s32 ndsRelocPointerIsFighterAObj32(const void *ptr)
{
    NDSRelocLoadedFile *loaded =
        ndsRelocFindLoadedFileContaining(ptr, sizeof(u32));

    return ((loaded != NULL) &&
            (ndsRelocIsFighterAObj32Asset(loaded->asset_id) != FALSE)) ?
               TRUE : FALSE;
}

/* Slice 45. Same-binary A/B route for the alias-scan reorder below, and .data
 * aligned(32) for the same reason gNdsR2MPRoute is: the poke has to own its
 * cache line so `-SetGlobals` picks an arm at IDENTICAL placement. A relink
 * moves WORK-H P95 by more than this cut is worth (R2-06 E11 measured +15,744
 * from a change that added negative bytes), so a cross-build pair could not
 * read it.
 *
 *   bit 1  order the alias scan's conjunction cheap-test-first
 *
 * Default is the candidate, so an unpoked ROM is the fast arm and
 * `-SetGlobals gNdsR2RelocAliasRoute=0` restores the original order.
 *
 * BANKED, and the flag now defaults to 0 so the reorder ships with the test
 * folded out. One binary, `builds/build-c122-alias`: Resolves 16,002 -> 1,143
 * of the same 16,067 visits, WORK-H P95 1,227,456 -> 1,215,296 (-12,160), P50
 * +256. The control arm reproduced the c122 bank to 2,176, which is what makes
 * the -12,160 readable at all. */
#if NDS_R2_RELOC_ALIAS_ROUTE
volatile u32 gNdsR2RelocAliasRoute
    __attribute__((section(".data"), aligned(32))) = 1u;
#define NDS_R2_RELOC_ALIAS_ROUTE_ON(bit) ((gNdsR2RelocAliasRoute & (bit)) != 0u)
#else
#define NDS_R2_RELOC_ALIAS_ROUTE_ON(bit) (1)
#endif

/* Engagement proof, per the standing rule that an optimization which silently
 * never fires is indistinguishable from one that fired and saved nothing.
 * `Resolves` counts the calls to ndsRelocAssetIDForToken this scan actually
 * makes; `Visits` counts the nodes it walks. The ratio IS the cut. */
volatile u32 gNdsR2RelocAliasVisits;
volatile u32 gNdsR2RelocAliasResolves;

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
        /* Slice 45. `ndsRelocAssetIDForToken` was resolved for EVERY node here
         * even though `addr == data` rejects almost all of them, and that
         * resolver is the single largest non-idle symbol in the whole-match
         * tail: +41,731 cyc/frame on 71 of the 80 costliest frames, 11,721,422
         * cycles a match, 28% of it concentrated in that tail. It is a ~550
         * instruction if-chain plus, on a miss, two pointer scans over all
         * 143 + 158 Mario/Fox animation ids (R2-06 E11) -- and a node whose
         * `addr` does not match is always a miss for the purposes of this test,
         * so every one of those walks was discarded.
         *
         * Reordering is exact, not an approximation. `&&` short-circuits, the
         * two operands commute here because `ndsRelocAssetIDForToken` is a pure
         * function of its argument -- `ndsRelocFileID` returns
         * `(u32)(uintptr_t)file_id`, the ADDRESS of a link-time global rather
         * than anything read from it, so the chain has no memory input and no
         * side effect (its body contains no assignment or counter; the same
         * property is what let Task 74 call a cache in front of it "provably
         * safe"). The surviving predicate is unchanged, so the set of removed
         * nodes is identical.
         *
         * This is deliberately NOT Task 74's memo, which is a dead lane: that
         * put three lookup arrays in .main.bss and lost to a chain of
         * branch-predictable link-time immediates that was already resident.
         * Nothing is added here -- the call count goes down. */
        if (NDS_R2_RELOC_ALIAS_ROUTE_ON(1u))
        {
            gNdsR2RelocAliasVisits++;
            if (nodes[i].addr == data)
            {
                u32 node_asset_id = ndsRelocAssetIDForToken((u32)nodes[i].id);

                gNdsR2RelocAliasResolves++;
                if ((node_asset_id != asset_id) &&
                    (ndsRelocIsFighterAObj16Asset(node_asset_id) != FALSE))
                {
                    ndsRelocRemoveStatusNodeAt(nodes, count, i);
                    continue;
                }
            }
        }
        else
        {
            u32 node_asset_id = ndsRelocAssetIDForToken((u32)nodes[i].id);

            gNdsR2RelocAliasVisits++;
            gNdsR2RelocAliasResolves++;
            if ((nodes[i].addr == data) && (node_asset_id != asset_id) &&
                (ndsRelocIsFighterAObj16Asset(node_asset_id) != FALSE))
            {
                ndsRelocRemoveStatusNodeAt(nodes, count, i);
                continue;
            }
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

/* Task 85, same reasoning as ndsRelocReadNative32 above. */
static u16 ndsRelocReadNative16(const void *addr)
{
    u16 value;

#if NDS_TASK85_ALIGNED_NATIVE_ACCESS
    if ((((uintptr_t)addr) & 1u) == 0u)
    {
        return *(const u16 *)addr;
    }
#endif
    memcpy(&value, addr, sizeof(value));
    return value;
}

static void ndsRelocWriteNative16(void *addr, u16 value)
{
#if NDS_TASK85_ALIGNED_NATIVE_ACCESS
    if ((((uintptr_t)addr) & 1u) == 0u)
    {
        *(u16 *)addr = value;
        return;
    }
#endif
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

#if NDS_R2_RELOC_FIXUP_TIMING
/* R2-06 E9 sizing. Ticks split three ways inside this function, plus the shapes
 * that predict them: table_bytes drives the O(n^2) scan, data_size drives the
 * lane swap, and ScriptWords drives the per-script normalize. */
volatile u32 gNdsR2FixupAObj16SwapTicks;
volatile u32 gNdsR2FixupAObj16SuccessorTicks;
volatile u32 gNdsR2FixupAObj16NormalizeTicks;
volatile u32 gNdsR2FixupAObj16TableBytes;
volatile u32 gNdsR2FixupAObj16DataBytes;
volatile u32 gNdsR2FixupAObj16Scripts;
volatile u32 gNdsR2FixupAObj16ScriptWords;
#endif

static s32 ndsRelocNormalizeFighterAObj16File(NDSRelocLoadedFile *loaded)
{
    uintptr_t base;
    uintptr_t table_bytes;
    u32 i;
#if NDS_R2_RELOC_FIXUP_TIMING
    u32 fixup_sub;
#endif

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

#if NDS_R2_RELOC_FIXUP_TIMING
    /* R2-06 E8/E9. This function is 88.4% of the in-frame relocation, and it has
     * THREE candidate costs that had never been separated: the lane swap over the
     * whole payload, the O(n^2) successor scan, and the per-script normalize
     * (which walks the payload a second time). Splitting them is what decides
     * which loop E9 should attack -- naming the O(n^2) from reading the code was
     * an inference, and 23,491 ticks/call is far more than a table of a few tens
     * of entries can spend on compares. */
    gNdsR2FixupAObj16TableBytes += (u32)table_bytes;
    gNdsR2FixupAObj16DataBytes += (u32)loaded->data_size;
    fixup_sub = cpuGetTiming();
#endif
    for (i = (u32)table_bytes; (i + sizeof(u32)) <= loaded->data_size;
         i += sizeof(u32))
    {
        u16 first = ndsRelocReadNative16((u8 *)loaded->data + i);
        u16 second = ndsRelocReadNative16((u8 *)loaded->data + i + sizeof(u16));

        ndsRelocWriteNative16((u8 *)loaded->data + i, second);
        ndsRelocWriteNative16((u8 *)loaded->data + i + sizeof(u16), first);
    }
#if NDS_R2_RELOC_FIXUP_TIMING
    gNdsR2FixupAObj16SwapTicks += cpuGetTiming() - fixup_sub;
#endif

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

#if NDS_R2_RELOC_FIXUP_TIMING
            gNdsR2FixupAObj16Scripts++;
            fixup_sub = cpuGetTiming();
#endif
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
#if NDS_R2_RELOC_FIXUP_TIMING
            gNdsR2FixupAObj16SuccessorTicks += cpuGetTiming() - fixup_sub;
            gNdsR2FixupAObj16ScriptWords += word_count;
            fixup_sub = cpuGetTiming();
#endif

            ndsRelocNormalizeAObj16Script((u16 *)((u8 *)loaded->data +
                                                  script_offset),
                                          word_count);
#if NDS_R2_RELOC_FIXUP_TIMING
            gNdsR2FixupAObj16NormalizeTicks += cpuGetTiming() - fixup_sub;
#endif
        }
    }

    loaded->format_fixups_applied = TRUE;
    return TRUE;
}

/* The O2R fighter files preserve the N64's big-endian mixed-width layout.
 * The common relocation pass first byte-swaps each u32, which is correct for
 * floats, pointers, and integers but reverses the two u16 lanes within these
 * exact FTAttributes words.  BattleShip reads all six words directly. */
_Static_assert(offsetof(FTAttributes, dead_fgm_ids) == 0xb4u,
               "FTAttributes dead FGM layout changed");
_Static_assert(offsetof(FTAttributes, deadup_sfx) == 0xb8u,
               "FTAttributes dead-up FGM layout changed");
_Static_assert(offsetof(FTAttributes, damage_sfx) == 0xbau,
               "FTAttributes damage FGM layout changed");
_Static_assert(offsetof(FTAttributes, smash_sfx) == 0xbcu,
               "FTAttributes smash FGM layout changed");
_Static_assert(offsetof(FTAttributes, item_pickup) == 0xc4u,
               "FTAttributes smash padding layout changed");
_Static_assert(offsetof(FTAttributes, itemthrow_vel_scale) == 0xe4u,
               "FTAttributes item-throw layout changed");
_Static_assert(offsetof(FTAttributes, itemthrow_damage_scale) == 0xe6u,
               "FTAttributes item-throw damage layout changed");
_Static_assert(offsetof(FTAttributes, heavyget_sfx) == 0xe8u,
               "FTAttributes heavy-get FGM layout changed");
_Static_assert(offsetof(FTAttributes, halo_size) == 0xecu,
               "FTAttributes heavy-get padding layout changed");

static void ndsRelocSwapNativeU16WordLanes(void *word)
{
    u8 *bytes = word;
    u16 first = ndsRelocReadNative16(bytes);
    u16 second = ndsRelocReadNative16(bytes + sizeof(u16));

    ndsRelocWriteNative16(bytes, second);
    ndsRelocWriteNative16(bytes + sizeof(u16), first);
}

static s32 ndsRelocFighterAttributesMatchSource(
    u32 asset_id, const FTAttributes *attr)
{

    if (attr == NULL)
    {
        return FALSE;
    }
    if (asset_id == NDS_RELOC_ASSET_MARIO_MAIN)
    {
        return
            (attr->dead_fgm_ids[0] == nSYAudioVoiceMarioDead) &&
            (attr->dead_fgm_ids[1] == nSYAudioFGMMarioDeadSlam) &&
            (attr->deadup_sfx == nSYAudioVoiceMarioDeadUp) &&
            (attr->damage_sfx == nSYAudioVoiceMarioDamage) &&
            (attr->smash_sfx[0] == nSYAudioVoiceMarioSmash1) &&
            (attr->smash_sfx[1] == nSYAudioVoiceMarioSmash2) &&
            (attr->smash_sfx[2] == nSYAudioVoiceMarioSmash3) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == nSYAudioVoiceMarioHeavyGet);
    }
    if (asset_id == NDS_RELOC_ASSET_FOX_MAIN)
    {
        return
            (attr->dead_fgm_ids[0] == nSYAudioVoiceFoxDead) &&
            (attr->dead_fgm_ids[1] == nSYAudioFGMFoxDeadSlam) &&
            (attr->deadup_sfx == nSYAudioVoiceFoxDeadUp) &&
            (attr->damage_sfx == nSYAudioVoiceFoxDamage) &&
            (attr->smash_sfx[0] == nSYAudioVoiceFoxSmash3) &&
            (attr->smash_sfx[1] == nSYAudioVoiceFoxSmash1) &&
            (attr->smash_sfx[2] == nSYAudioVoiceFoxSmash2) &&
            (attr->itemthrow_vel_scale == 0x64u) &&
            (attr->itemthrow_damage_scale == 0x64u) &&
            (attr->heavyget_sfx == nSYAudioFGMVoiceEnd);
    }
    return FALSE;
}

static s32 ndsRelocNormalizeFighterAttributesFile(
    NDSRelocLoadedFile *loaded)
{
    u32 attr_offset;
    u8 *attr_bytes;
    FTAttributes *attr;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MARIO_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_MARIO_MAIN_ATTRIBUTES;
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_FOX_MAIN)
    {
        attr_offset = NDS_RELOC_SYMBOL_FOX_MAIN_ATTRIBUTES;
    }
    else
    {
        return TRUE;
    }
    if (ndsRelocRangeInLoadedFile(
            loaded, attr_offset, sizeof(FTAttributes)) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    attr_bytes = (u8 *)loaded->data + attr_offset;
    attr = (FTAttributes *)attr_bytes;
    if (loaded->format_fixups_applied == FALSE)
    {
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, dead_fgm_ids));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, deadup_sfx));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, smash_sfx));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, smash_sfx) +
            (2u * sizeof(u16)));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, itemthrow_vel_scale));
        ndsRelocSwapNativeU16WordLanes(
            attr_bytes + offsetof(FTAttributes, heavyget_sfx));
        loaded->format_fixups_applied = TRUE;
    }
    if (ndsRelocFighterAttributesMatchSource(loaded->asset_id, attr) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }
    return TRUE;
}

static size_t ndsRelocAssetAllocSize(u32 asset_id);
static s32 ndsRelocFinalizeLoadedFile(NDSRelocLoadedFile *loaded);
static s32 ndsRelocNormalizeWeaponAttributesFile(NDSRelocLoadedFile *loaded);
static s32 ndsRelocNormalizeBattleInterfaceSprites(
    NDSRelocLoadedFile *loaded);
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

/* Counts heap allocations DECLINED rather than allowed to spin. Any non-zero
 * value means the general heap ran out during play and the ROM survived it --
 * before the guards at the two loaders below, that was a hard freeze with no
 * error and no recovery. A rising count in a soak is the heap budget getting
 * tight, not a bug in itself. Defined here because both guarded sites are
 * above the rest of this file's counter block. */
volatile u32 gNdsRelocHeapDeclineCount;

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
        /* Ask BEFORE allocating. syTaskmanMalloc cannot return NULL -- on
         * exhaustion syMallocSet spins in `while (TRUE);` (decomp
         * src/sys/malloc.c:30) -- so the NULL test below it has always been
         * dead code and this call is a freeze, not a failed load. Same defect
         * the anim cache was moved off the heap for on 2026-07-29; that fix
         * covered ndsR2AnimCacheStore and left the two loaders that reach the
         * heap on their own. See the arena's own use of this guard at :6078. */
        if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap, asset_size,
                                0x10) == FALSE)
        {
            gNdsRelocHeapDeclineCount++;
            ndsRelocRecordExternalFixupFail(asset_id);
            return NULL;
        }
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

#if NDS_R2_RELOC_FIXUP_TIMING
/* R2-06 E8. This function is the whole reason the gate is missed, and the four
 * passes below have never been priced separately.
 *
 * E8 attributed 8 of the 9 over-gate frames to frames on which this runs:
 * `NDS_TASK75_MARK_ASSET_LOAD` fires here, the census ring flagged 16 of 128
 * frames, and those frames carry `WORK-H` median 1,113,152 against 974,080 clean
 * (+139,072), all of it in `SRC` (+139,328). Clean-frame P95 is 1,056,640 --
 * INSIDE the 1,120,000 gate by 63,360 -- so the milestone turns on getting this
 * out of the frame.
 *
 * The anim cache does not help: its own comment at the hit path says a hit
 * "replaces the NitroFS walk, the cartridge read and the word byte-swap with one
 * copy" and that "the fixups below still run against this heap, because they
 * write absolute pointers into it". So all 16 of those frames are cache HITS that
 * still relocate.
 *
 * Instrument only, default off, no behaviour change. Which of the four passes
 * dominates decides the repair, and the two candidate repairs are very different
 * amounts of work:
 *   - cache the POST-fixup image, which needs the destination address to be
 *     stable per asset (`AnimForceResident` reads 0, so today it is not: the
 *     destination is a shared scratch heap that different animations rotate
 *     through, which is also why 52 of 81 force-loads are repeats);
 *   - or give each resident animation its own destination buffer, trading RAM
 *     for the fixup pass, which PROJECT_GOAL explicitly ranks as a good trade.
 * Do not pick one without this split. */
volatile u32 gNdsR2FixupFinalizeCalls;
volatile u32 gNdsR2FixupFinalizeTicks;
volatile u32 gNdsR2FixupFinalizeMaxTicks;
volatile u32 gNdsR2FixupInternalTicks;
volatile u32 gNdsR2FixupAObj16Ticks;
volatile u32 gNdsR2FixupAttributesTicks;
volatile u32 gNdsR2FixupWeaponAttributesTicks;
volatile u32 gNdsR2FixupExternalTicks;
volatile u32 gNdsR2FixupSpritesTicks;
#endif

static s32 ndsRelocFinalizeLoadedFile(NDSRelocLoadedFile *loaded)
{
#if NDS_R2_RELOC_FIXUP_TIMING
    u32 fixup_enter;
    u32 fixup_phase;
#endif

    if (loaded == NULL)
    {
        return FALSE;
    }
    if (loaded->fixups_applying != FALSE)
    {
        return TRUE;
    }

    /* Task 75 E0: counted here rather than at the open, because this is the one
     * point every load path funnels through exactly once per file -- the early
     * return above is the re-entrant case and must not be counted twice. */
    NDS_TASK75_MARK_ASSET_LOAD();
    loaded->fixups_applying = TRUE;
#if NDS_R2_RELOC_FIXUP_TIMING
    fixup_enter = cpuGetTiming();
    fixup_phase = fixup_enter;
    gNdsR2FixupFinalizeCalls++;
    if (ndsRelocApplyInternalPointerFixups(loaded) == FALSE)
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
    gNdsR2FixupInternalTicks += cpuGetTiming() - fixup_phase;
    fixup_phase = cpuGetTiming();
    if (ndsRelocNormalizeFighterAObj16File(loaded) == FALSE)
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
    gNdsR2FixupAObj16Ticks += cpuGetTiming() - fixup_phase;
    fixup_phase = cpuGetTiming();
    if (ndsRelocNormalizeFighterAttributesFile(loaded) == FALSE)
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
    gNdsR2FixupAttributesTicks += cpuGetTiming() - fixup_phase;
    fixup_phase = cpuGetTiming();
    if (ndsRelocNormalizeWeaponAttributesFile(loaded) == FALSE)
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
    gNdsR2FixupWeaponAttributesTicks += cpuGetTiming() - fixup_phase;
    fixup_phase = cpuGetTiming();
    if (ndsRelocApplyExternalPointerFixups(loaded) == FALSE)
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
    gNdsR2FixupExternalTicks += cpuGetTiming() - fixup_phase;
#else
    if ((ndsRelocApplyInternalPointerFixups(loaded) == FALSE) ||
        (ndsRelocNormalizeFighterAObj16File(loaded) == FALSE) ||
        (ndsRelocNormalizeFighterAttributesFile(loaded) == FALSE) ||
        (ndsRelocNormalizeWeaponAttributesFile(loaded) == FALSE) ||
        (ndsRelocApplyExternalPointerFixups(loaded) == FALSE))
    {
        loaded->fixups_applying = FALSE;
        return FALSE;
    }
#endif
    loaded->fixups_applying = FALSE;

#if NDS_R2_RELOC_FIXUP_TIMING
    fixup_phase = cpuGetTiming();
#endif
    if (ndsRelocNormalizeBattleInterfaceSprites(loaded) == FALSE)
    {
        return FALSE;
    }
#if NDS_R2_RELOC_FIXUP_TIMING
    {
        u32 total;

        gNdsR2FixupSpritesTicks += cpuGetTiming() - fixup_phase;
        total = cpuGetTiming() - fixup_enter;
        gNdsR2FixupFinalizeTicks += total;
        if (total > gNdsR2FixupFinalizeMaxTicks)
        {
            gNdsR2FixupFinalizeMaxTicks = total;
        }
    }
#endif

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

/* Undo the loader's word swap over a run of s16 that carries no other type.
 *
 * ndsRelocApplyWordByteSwap byte-swaps every 32-bit word of a reloc file. That
 * is right for u32, f32 and pointers, and it EXCHANGES THE POSITIONS of any two
 * s16 sharing a word. Every other struct in this file undoes that with explicit
 * ndsRelocSwapS16Pair calls (Sprite, Bitmap, MObjSub); this is the same
 * operation expressed over a byte range, because MPGroundData's second s16 run
 * is long and its pairing is not obvious from the field names. */
static void ndsRelocSwapWordS16Halves(void *base, u32 begin, u32 end)
{
    u32 offset;

    for (offset = begin; offset < end; offset += (u32)sizeof(u32))
    {
        s16 *pair = (s16 *)((u8 *)base + offset);

        ndsRelocSwapS16Pair(&pair[0], &pair[1]);
    }
}

/* WPAttributes s16 runs are scrambled by the loader's blanket u32 word swap,
 * exactly the way the fighter attributes' u16 pairs were: each u32 word in the
 * O2R payload holds TWO s16 lanes, and the byte swap reverses their order, so
 * a plain C struct read (which wpManagerMakeWeapon does at wpmanager.c:285-288
 * and itmanager.c does for items) sees every s16 pair swapped. The fireball is
 * the visible casualty -- map_coll {top 50, center 0, bottom -50, width 50}
 * (204_MarioSpecial1.c:33) reads as {0, 50, +50, -50}, so the floor clamp
 * rests the weapon translate at floor_y - 50 instead of floor_y + 50 and the
 * quad sinks a full collision box into the ground before bouncing.
 *
 * The runs that need un-swapping are the ONLY s16-carrying fields in the
 * struct, both 4-aligned, both pure-s16: attack_offsets[2] (6 s16, 12 bytes at
 * offset 0x10) and map_coll top/center/bottom/width (4 s16 at 0x1C). The
 * bitfield words from 0x24 on are single-u32 and survive the word swap
 * correctly -- swapping THEIR halves would corrupt size/angle/damage, which
 * is why the range stops at map_coll_width. */
static void ndsRelocNormalizeWeaponAttributes(WPAttributes *attr)
{
    _Static_assert((offsetof(WPAttributes, attack_offsets) %
                    sizeof(u32)) == 0u,
                   "WPAttributes attack_offsets must start on a word");
    _Static_assert((offsetof(WPAttributes, map_coll_width) -
                    offsetof(WPAttributes, attack_offsets)) == 18u,
                   "WPAttributes s16 run must be attack_offsets[2] plus "
                   "map_coll top/center/bottom/width");

    if (attr == NULL)
    {
        return;
    }
    ndsRelocSwapWordS16Halves(
        attr,
        (u32)offsetof(WPAttributes, attack_offsets),
        (u32)offsetof(WPAttributes, map_coll_width) + (u32)sizeof(s16));
}

/* Engagement + integrity check after the swap. The map_coll box is the
 * gameplay-visible value -- a wrong box makes projectiles sink into floors or
 * float over them -- so it is pinned to the source's literal values
 * (204_MarioSpecial1.c:33 for the fireball, 210_FoxSpecial1.c:27 for the
 * blaster) rather than guessed. attack_offsets are all zero for the P1 weapons
 * (fireball, blaster), so they are pinned too: a weapon whose offsets are
 * non-zero will fail here until this table grows, which is the intended
 * fail-closed behaviour for an asset this loader has never examined. */
static s32 ndsRelocWeaponAttributesMatchSource(u32 asset_id,
                                               const WPAttributes *attr)
{
    s32 i;

    if (attr == NULL)
    {
        return FALSE;
    }
    for (i = 0; i < 2; i++)
    {
        if ((attr->attack_offsets[i].x != 0) ||
            (attr->attack_offsets[i].y != 0) ||
            (attr->attack_offsets[i].z != 0))
        {
            return FALSE;
        }
    }
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL1)
    {
        return (attr->map_coll_top == 50) &&
               (attr->map_coll_center == 0) &&
               (attr->map_coll_bottom == -50) &&
               (attr->map_coll_width == 50);
    }
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL1)
    {
        return (attr->map_coll_top == 10) &&
               (attr->map_coll_center == 0) &&
               (attr->map_coll_bottom == -10) &&
               (attr->map_coll_width == 10);
    }
    return TRUE;
}

static s32 ndsRelocNormalizeWeaponAttributesFile(
    NDSRelocLoadedFile *loaded)
{
    u32 attr_offset;
    u8 *attr_bytes;
    WPAttributes *attr;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL1)
    {
        attr_offset = NDS_RELOC_SYMBOL_MARIO_SPECIAL1_FIREBALL_WEAPON_ATTRIBUTES;
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_FOX_SPECIAL1)
    {
        attr_offset = NDS_RELOC_SYMBOL_FOX_SPECIAL1_BLASTER_WEAPON_ATTRIBUTES;
    }
    else
    {
        return TRUE;
    }
    if (ndsRelocRangeInLoadedFile(
            loaded, attr_offset, sizeof(WPAttributes)) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }

    attr_bytes = (u8 *)loaded->data + attr_offset;
    attr = (WPAttributes *)attr_bytes;
    if (loaded->format_fixups_applied == FALSE)
    {
        ndsRelocNormalizeWeaponAttributes(attr);
        loaded->format_fixups_applied = TRUE;
    }
    if (ndsRelocWeaponAttributesMatchSource(loaded->asset_id, attr) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(loaded->asset_id);
        return FALSE;
    }
    return TRUE;
}

/* MPGroundData has TWO s16-only runs, both 4-aligned: the camera/map bounds
 * ahead of bgm_id, and everything from alt_warning to the end of the struct.
 *
 * This used to be a `if (min > max) swap` heuristic over eight named pairs, and
 * it was wrong twice over.
 *
 * It never touched `alt_warning` at all -- alt_warning is the FIRST s16 of the
 * second run, so the word swap left it holding camera_bound_team_top. Dream
 * Land therefore read +3500 where 255_GRPupupuMap.c:59 says -2900. FGM 153
 * AltitudeWarn fires on `pos_prev.y >= alt_warning && topn.y < alt_warning`
 * (decomp ftmain.c:1817) -- the bottom blast-zone whistle -- so at +3500 it
 * played every time a fighter fell back down through y = 3500, which is exactly
 * what a big upward knockback does. That is the owner's "FGM 153 AltitudeWarn
 * plays at wrong trigger", and the trigger was never the defect: the threshold
 * was.
 *
 * And because alt_warning shifts the rest of the run by one s16, the team
 * fields do not pair top-with-bottom inside a word -- alt_warning pairs with
 * team_top, team_bottom with team_right, team_left with map_team_top, and so
 * on. The heuristic was comparing fields that never shared a word, so every
 * team bound was left scrambled too.
 *
 * Swapping the halves of each word is the exact inverse and needs no heuristic.
 * The static asserts pin the two runs; if a field is ever added between them
 * the build fails rather than silently reverting to the old symptom. */
static void ndsRelocNormalizeGroundDataBounds(MPGroundData *ground_data)
{
    _Static_assert((offsetof(MPGroundData, camera_bound_top) %
                    sizeof(u32)) == 0u,
                   "MPGroundData bounds run must start on a word");
    _Static_assert((offsetof(MPGroundData, bgm_id) -
                    offsetof(MPGroundData, camera_bound_top)) == 16u,
                   "MPGroundData bounds run must be eight s16");
    _Static_assert((offsetof(MPGroundData, alt_warning) %
                    sizeof(u32)) == 0u,
                   "MPGroundData alt_warning run must start on a word");
    _Static_assert((sizeof(MPGroundData) -
                    offsetof(MPGroundData, alt_warning)) == 32u,
                   "MPGroundData alt_warning run must reach the struct end");

    if (ground_data == NULL)
    {
        return;
    }
    ndsRelocSwapWordS16Halves(ground_data,
                              (u32)offsetof(MPGroundData, camera_bound_top),
                              (u32)offsetof(MPGroundData, bgm_id));
    ndsRelocSwapWordS16Halves(ground_data,
                              (u32)offsetof(MPGroundData, alt_warning),
                              (u32)sizeof(MPGroundData));
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

static s32 ndsRelocNormalizeBattleInterfaceSprites(
    NDSRelocLoadedFile *loaded)
{
    u32 i;

    if (loaded == NULL)
    {
        return FALSE;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsBattleInterfaceSpriteDescs); i++)
    {
        const NDSRelocSpriteNormalizeDesc *desc =
            &sNdsBattleInterfaceSpriteDescs[i];
        Sprite *sprite;
        u32 display_list_words;

        if (desc->asset_id != loaded->asset_id)
        {
            continue;
        }
        if (ndsRelocRangeInLoadedFile(loaded, desc->offset,
                                      sizeof(Sprite)) == FALSE)
        {
            return FALSE;
        }

        sprite = (Sprite *)((u8 *)loaded->data + desc->offset);
        /* libultra generates one sprite's display list as a fixed 24-word frame
         * plus 12 words per bitmap, so `ndisplist` is DERIVABLE and does not need
         * a per-offset table. This used to be `36` with three hardcoded
         * exceptions (84 for 0x4d78, 96 for 0xa730, 48 for 0xc370); read out of
         * the extracted asset, all six known sprites fit `12n + 24` exactly --
         * 1 bitmap 36, 2 -> 48, 3 -> 60, 4 -> 72, 5 -> 84, 6 -> 96. Keeping the
         * table would have meant a fourth, fifth and sixth exception for the
         * GAME SET / TIME UP letters below, each of which is a 2-, 3- or
         * 4-bitmap sprite. The check this feeds is still fail-closed: a sprite
         * whose raw `ndisplist` disagrees with the formula is refused, not
         * normalized. */
        display_list_words = (12u * desc->bitmap_count) + 24u;
        if (((u32)(u16)sprite->width == desc->width) &&
            ((u32)(u16)sprite->height == desc->height) &&
            ((u32)(u16)sprite->nbitmaps == desc->bitmap_count) &&
            (sprite->bmfmt == desc->bmfmt) &&
            (sprite->bmsiz == desc->bmsiz))
        {
            continue;
        }

        /* The blanket u32 endian pass exchanges each adjacent halfword.
         * Validate that exact raw signature, including libultra's 36-word
         * generated display list, before touching a known manifest entry. */
        if (((u32)(u16)sprite->width != desc->height) ||
            ((u32)(u16)sprite->height != desc->width) ||
            ((u32)(u16)sprite->nbitmaps != display_list_words) ||
            ((u32)(u16)sprite->ndisplist != desc->bitmap_count) ||
            (ndsRelocPointerRangeInLoadedFile(
                loaded, sprite->bitmap,
                sizeof(Bitmap) * desc->bitmap_count) == FALSE))
        {
            return FALSE;
        }

        ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt,
                                            desc->bmsiz);
        if (((u32)(u16)sprite->width != desc->width) ||
            ((u32)(u16)sprite->height != desc->height) ||
            ((u32)(u16)sprite->nbitmaps != desc->bitmap_count) ||
            (ndsRelocNormalizeSpriteBitmapTable(
                loaded, sprite, desc->bitmap_count) == FALSE))
        {
            return FALSE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS)
    {
        if (ndsIFCommonNativeOamPrepareGameStatus(
                loaded->data, loaded->data_size) == FALSE)
        {
            /* Preparation is an optimization seam.  The relocated source
             * asset remains valid and the exact BG3 SObj compositor is the
             * required fallback if OBJ capacity or conversion rejects it. */
        }
    }
    return TRUE;
}

static void ndsRelocNormalizeStageDreamLandSprite(
    NDSRelocLoadedFile *loaded)
{
    Sprite *sprite;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_STAGE_DREAM_LAND) ||
        (ndsRelocRangeInLoadedFile(
            loaded, NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE,
            sizeof(Sprite)) == FALSE))
    {
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data +
                        NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE);
    if ((sprite->width == 300) && (sprite->height == 220) &&
        (sprite->nbitmaps == 44) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_16b))
    {
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA,
                                        G_IM_SIZ_16b);
    (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite, 44u);
}

static void ndsRelocNormalizeVSResultsSprites(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_VS_RESULTS))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsVSResultsSpriteDescs); i++)
    {
        const NDSVSResultsSpriteDesc *desc =
            &sNdsVSResultsSpriteDescs[i];
        Sprite *sprite;

        if (ndsRelocRangeInLoadedFile(loaded, desc->offset,
                                      sizeof(Sprite)) == FALSE)
        {
            continue;
        }
        sprite = (Sprite *)((u8 *)loaded->data + desc->offset);
        if (((u32)(u16)sprite->width == desc->width) &&
            ((u32)(u16)sprite->height == desc->height) &&
            ((u32)(u16)sprite->nbitmaps == desc->bitmap_count) &&
            (sprite->bmfmt == desc->bmfmt) &&
            (sprite->bmsiz == desc->bmsiz))
        {
            continue;
        }
        ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt,
                                            desc->bmsiz);
        (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                                 desc->bitmap_count);
    }
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
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_J);
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
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Q);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_T);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_V);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_W);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Z);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_PERIOD);
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

static void ndsRelocNormalizeMObjSubWordSwapped(MObjSub *mobjsub)
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

static s32 ndsRelocMObjSubAttachmentFieldsLookNative(
    const MObjSub *mobjsub)
{
    if (mobjsub == NULL)
    {
        return FALSE;
    }

    return (mobjsub->pad00 == 0u) &&
           (ndsRelocMObjSubFlagsKnown(mobjsub->flags) != FALSE) &&
           (mobjsub->fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->siz <= G_IM_SIZ_32b) &&
           (mobjsub->block_fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->block_siz <= G_IM_SIZ_32b);
}

static void ndsRelocRegisterNormalizedMObjSub(
    const NDSRelocLoadedFile *loaded, const MObjSub *mobjsub)
{
    u32 i;

    if ((loaded == NULL) || (mobjsub == NULL))
    {
        return;
    }
    for (i = 0u; i < sNdsRelocNormalizedMObjSubCount; i++)
    {
        NDSRelocNormalizedMObjSub *entry =
            &sNdsRelocNormalizedMObjSubs[i];

        if ((entry->record == mobjsub) &&
            (entry->asset_id == loaded->asset_id) &&
            (entry->owner_generation == loaded->owner_generation))
        {
            return;
        }
    }
    if (sNdsRelocNormalizedMObjSubCount >=
        NDS_RELOC_NORMALIZED_MOBJ_SUB_CAPACITY)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }
    sNdsRelocNormalizedMObjSubs[sNdsRelocNormalizedMObjSubCount].record =
        mobjsub;
    sNdsRelocNormalizedMObjSubs[sNdsRelocNormalizedMObjSubCount].asset_id =
        loaded->asset_id;
    sNdsRelocNormalizedMObjSubs[sNdsRelocNormalizedMObjSubCount]
        .owner_generation = loaded->owner_generation;
    sNdsRelocNormalizedMObjSubCount++;
}

static s32 ndsRelocMObjSubAlreadyNormalized(
    const NDSRelocLoadedFile *loaded, const MObjSub *mobjsub)
{
    u32 i;

    if ((loaded == NULL) || (mobjsub == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < sNdsRelocNormalizedMObjSubCount; i++)
    {
        const NDSRelocNormalizedMObjSub *entry =
            &sNdsRelocNormalizedMObjSubs[i];

        if ((entry->record == mobjsub) &&
            (entry->asset_id == loaded->asset_id) &&
            (entry->owner_generation == loaded->owner_generation))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* BattleShip O2R payloads are blanket-swapped as u32 words. MObjSub mixes
 * u16/u8 fields inside those words, so public object attachment must restore
 * those lanes before objman.c copies the material into its live MObj. Keep
 * pointers outside loaded files native. Within a loaded file, the
 * asset/generation registry identifies records already normalized in place;
 * every other record still has the O2R word-swapped representation. */
s32 ndsRelocCopyMObjSubForAttachment(MObjSub *dst, const MObjSub *src)
{
    NDSRelocLoadedFile *loaded;

    if ((dst == NULL) || (src == NULL))
    {
        return -1;
    }

    *dst = *src;
    loaded = ndsRelocFindLoadedFileContaining(src, sizeof(*src));
    if (loaded == NULL)
    {
        return 0;
    }
    if (ndsRelocMObjSubAlreadyNormalized(loaded, src) != FALSE)
    {
        return (ndsRelocMObjSubAttachmentFieldsLookNative(dst) != FALSE) ?
            0 : -1;
    }

    ndsRelocNormalizeMObjSubWordSwapped(dst);
    return (ndsRelocMObjSubAttachmentFieldsLookNative(dst) != FALSE) ? 1 : -1;
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
    if (mobjsub == NULL)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }

    if (ndsRelocMObjSubMixedFieldsLookNative(mobjsub) != FALSE)
    {
        ndsRelocRegisterNormalizedMObjSub(loaded, mobjsub);
        ndsRelocRecordMObjSubNormalize(loaded, mobjsub);
        return;
    }

    ndsRelocNormalizeMObjSubWordSwapped(mobjsub);
    ndsRelocRegisterNormalizedMObjSub(loaded, mobjsub);
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
    for (i = 0; i < ARRAY_COUNT(sNdsKnownAssetSymbols); i++)
    {
        if ((sNdsKnownAssetSymbols[i].asset_id != NDS_RELOC_ASSET_INVALID) &&
            (loaded->asset_id == sNdsKnownAssetSymbols[i].asset_id) &&
            (symbol == sNdsKnownAssetSymbols[i].symbol))
        {
            *out_offset = sNdsKnownAssetSymbols[i].offset;
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
        ndsRelocNormalizeStageDreamLandSprite(loaded);
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
    ndsRelocNormalizeStageDreamLandSprite(loaded);
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

#if NDS_TICK_HUD
/* R2-04 E0. Sizes the animation-cache opportunity: see the block inside
 * ndsRelocForceLoadFighterAObj16File for why AnimForceResident answering 0 does
 * not settle it. */
#define NDS_R204_ANIM_ID_SPAN \
    ((NDS_RELOC_ASSET_FOX_ANIM_LAST - NDS_RELOC_ASSET_MARIO_ANIM_WAIT) + 1u)
volatile u32 gNdsR204AnimForceLoadTotal;
volatile u32 gNdsR204AnimForceLoadDistinct;
volatile u32 gNdsR204AnimForceLoadRepeat;
/* Non-static so the sampler can read the set itself, not just its size: the
 * warm list R2-04 needs is the ID set, and a count cannot be turned back into
 * one. 301 bits, 10 words. */
volatile u32 gNdsR204AnimSeen[(NDS_R204_ANIM_ID_SPAN + 31u) / 32u];
#endif

#if NDS_R2_ANIM_CACHE
/* R2-04 E1. Task 75's absorption: keep each fighter animation's payload resident
 * so the frame that needs a move does not re-walk NitroFS and re-read the
 * cartridge for it.
 *
 * Keyed by asset_id, not by destination heap. E0 measured the destination-keyed
 * check (AnimForceResident) at 0 across two windows because the destination is
 * caller-owned and reused, while 53 of 82 force-loads (64.6%) repeat an asset
 * already loaded once. The match's working set is 29 distinct animations, so
 * NDS_R2_ANIM_CACHE_ENTRIES only has to cover that, not the 301-ID space --
 * which could not fit anyway against NDS_RELOC_LOADED_FILE_CAPACITY of 96.
 *
 * What is stored is the BYTE-SWAPPED, PRE-FIXUP image.
 * ndsRelocApplyInternalPointerFixups writes absolute pointers derived from
 * loaded->data, so a fixed-up image is position-dependent and must never be
 * replayed into a different heap. A hit copies the swapped bytes and re-runs the
 * fixups against the real destination, which is correct by construction and
 * still removes the directory walk and the cartridge read -- the part Task 71
 * profiled inside the frame.
 *
 * Every failure path degrades to the uncached load, so a full cache, a failed
 * allocation or an unexpected size is a performance outcome and never a
 * correctness one. */
/* 128, not 64. Cycle 105 read gNdsR204AnimForceLoadDistinct on the BOTH-CPU gate
 * arm over a whole 60-second match: the working set is 85 distinct animations,
 * not the 29 the E1 comment above was written against. 64 is below that, and the
 * entry count is a HARD refusal in both store paths -- ndsR2AnimWarmLoadOne
 * counts it as a warm failure and the miss path never fills -- so at 64 the last
 * 21 animations of the match could not be cached whatever the arena had left. */
#define NDS_R2_ANIM_CACHE_ENTRIES 128u

/* THIS CACHE MUST NEVER CALL syTaskmanMalloc, AND THAT IS NOT A STYLE RULE.
 *
 * 2026-07-29: the owner's "lots of freeze bugs that seem random" was this. A soak
 * caught it 3.5 minutes into a two-CPU match with the backtrace
 *
 *   syMallocSet (gSYTaskmanGeneralHeap, 3472, 16)  decomp/src/sys/malloc.c:30
 *     <- syTaskmanMalloc <- ndsR2AnimCacheStore <- ndsRelocForceLoadFighterAObj16File
 *     <- lbRelocGetForceExternHeapFile (llFTMarioAnimAttackAirDFileID)
 *     <- ftMainSetStatus (status_id=213) <- ftCommonAttackAirCheckInterruptCommon
 *
 * and malloc.c:30 is `while (TRUE);`. The source allocator does NOT return NULL
 * when its region overflows -- it spins forever. So the `payload == NULL` reject
 * paths below, which were written to make a full cache a performance outcome
 * rather than a correctness one, were DEAD CODE: the allocation could not fail,
 * it could only hang the console.
 *
 * Two properties of gSYTaskmanGeneralHeap make that inevitable rather than
 * unlucky. It is a bump region (SYMallocRegion: start/ptr/end) with no free, and
 * unlike the graphics heap nothing ever calls syMallocReset on it -- so every
 * byte this cache took was gone for the rest of the run, and the cache has no
 * eviction and bounds its ENTRY COUNT while leaving its BYTES unbounded. It also
 * meant the cached payload pointers would have dangled through any future reset.
 *
 * The fix is to stop borrowing the game's heap. A speculative cache gets its own
 * declared arena, and overflowing it returns NULL through the existing reject
 * path, which is what that path was always for.
 *
 * SIZE IS BOUNDED BY THE TASKMAN ARENA SEARCH, NOT BY THE WORKING SET. This was
 * first written at 128 KiB, sized from the 41 warm assets = 91,104 bytes above,
 * with the note that "a link failure on BSS is a loud, safe failure". **That was
 * wrong, and it shipped a ROM that could not start a battle.** BSS here does not
 * overflow a link; it silently competes with the runtime `calloc` that sizes
 * gSYTaskmanGeneralHeap. src/port/diagnostics.c:7403 searches downward from
 * 0x150000 in 0x1000 steps but FLOORS AT 0x130000, then drops to a coarse list
 * beginning at 0x100000 -- so crossing that floor costs 196,608 bytes in one
 * step, not the 0x1000 the loop implies.
 *
 * Measured, shipped hwtri ROM: without this arena the fine search secured
 * 1,286,144 bytes. With 128 KiB of it, all 33 fine steps failed
 * (TASKARENA=1048576,33) and battle start died in the exact way this comment was
 * written to prevent -- ftManagerSetupFilesAllKind(fkind=1) asking 116,752
 * against 58,024 free. The arena meant to stop a heap-exhaustion hang caused a
 * bigger one, two levels down, at a seam it never mentions.
 *
 * The static-BSS attempt was tried twice and failed twice, and BOTH failures were
 * the same misunderstanding: this buffer is not free, and its budget belongs to
 * the tightest configuration rather than the shipped one.
 *
 *   - 128 KiB: shipped arena 1,286,144 -> 1,048,576 (all 33 fine steps failed),
 *     battle start dead, 116,752 asked against 58,024 free. 131,072 was
 *     affordable in ISOLATION -- 1,286,144 - 131,072 still clears the 1,107,392
 *     battle start needs. The cliff, not the size, was fatal.
 *   - 32 KiB: failed by THIRTY-TWO BYTES. Sized against the shipped build's
 *     40,960 of headroom; the tick-HUD target starts from 1,277,952 and has only
 *     32,768, and arena plus counters is 32,800. Measured BSS also cleared the
 *     flag that had been blamed -- 1,742,216 against 1,709,416 is this arena and
 *     nothing else, so NDS_R2_BOTH_CPU contributes none.
 *   - 16 KiB fit, and then MEASURED THE COST OF FITTING: WORK-H P95 1,096,768 ->
 *     1,204,352, +107,584, gate missed by 84,352, with Fills=2 against
 *     Rejects=44 and 76 overflows in 128 frames. A cache that small is not a
 *     cache, and a miss is not free: ndsRelocAssetLoadHeaderAndData does a real
 *     fopen/fread/fclose through NitroFS, which on the owner's DLDI setup is SD
 *     I/O inside a gameplay frame. P50 moved only +14,080 while P95 moved
 *     +107,584 -- a tail-shaped regression, which is exactly what a handful of
 *     frames doing file reads looks like.
 *
 * So the arena comes from gSYTaskmanGeneralHeap instead, which is where a buffer
 * sized by measured game data belongs. That costs the boot-time search NOTHING,
 * so it cannot cross the 0x130000 cliff or trip the Task 36 replay admission
 * guard (include/nds/nds_renderer.h:124-134) that shares that constant -- and
 * 92,160 is affordable there in a way it never was in BSS. Reserved lazily on
 * first store, so ftManagerSetupFilesAllKind has already taken the fighters'
 * 116,752 bytes and this can never be the allocation that starves battle start.
 *
 * Sized to the R2-04 E5 working set below: 41 warm assets = 91,104 bytes, rounded
 * to 92,160. Overflow still degrades to the uncached load, and the SAFETY of this
 * fix does not depend on the size at all -- even a zero-byte arena would be safe,
 * because the reject path is live. Only the tick cost depends on it.
 *
 * Two general lessons, in TASK_STANDING_RULES: never call an allocator that
 * cannot fail from a code path that is allowed to fail, and never fix a heap
 * problem with a static buffer without pricing it against the heap.
 *
 * CYCLE 105 RE-SIZED IT, AND THE OLD VALUE WAS THE GATE ARM'S TAIL. 92,160 was
 * sized against a 41-asset list measured on the Boundary arm. Read on the
 * both-CPU gate arm over a whole match, that arena is 100% FULL and has been
 * refusing loads all match:
 *
 *   ArenaReservedBytes 92,160   ArenaUsedBytes 92,160   Overflows 142
 *   Rejects 142   OverflowLastSize 912   OverflowLastUsed 92,160
 *   ForceLoadTotal 353 = Hits 206 + Misses 147   ForceLoadDistinct 85
 *
 * A refused asset is not merely uncached: every later use of it re-runs
 * ndsRelocAssetLoadHeaderAndData, which is a real fopen/fread/fclose through
 * NitroFS -- on DLDI, SD I/O inside a gameplay frame. Frames 478 -> 2007 did
 * gNdsRelocAssetPayloadReadCount +111 against 113 frames whose SINT bucket
 * exceeded 400,000 ticks, and the median SINT is 169,248. That is the whole-match
 * P95 tail: capping SINT at its own median takes WORK-H P95 1,638,582 ->
 * 1,333,093. The reads are also why AUD and HUD -- buckets whose WORK cannot vary
 * with the match -- read 4.9x and 2.5x on those frames: the cartridge stalls
 * everything, not just the caller.
 *
 * Sized to the MEASURED 85-asset working set below: aligned data totals 197,184
 * bytes, so 200,704 covers it with 3,520 spare. The budget it spends from is
 * gSYTaskmanGeneralHeap, whose low-water free read 154,776 with the old 92,160
 * already taken -- i.e. a pool of ~246,936 -- so the reserve's request plus
 * NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE (233,472) still fits, and the run's own
 * gNdsTaskmanGeneralHeapFreeMin is the check on that arithmetic rather than this
 * comment. Overflow still degrades to the uncached load. */
/* RESIZED 2026-08-11 from 200,704 to 262,144 (docs/RAM_RECOVERY_PLAN.md Phase
 * 6). The 200,704 above was correct for the 85-asset set it was measured
 * against, but the match outgrew it: the arena ran 200,384/200,704 = 99.85% full
 * and refused live assets, and because this is a bump allocator with no
 * eviction every refusal re-read that asset from the ROM for the rest of the
 * match. That streaming was the largest single owner of the P95 tail.
 *
 * The new size is measured, not estimated. gNdsR2AnimCacheRejectedUniqueBytes
 * (added for this) counts the DISTINCT assets refused, because the reject COUNT
 * cannot size anything -- a refused asset is re-requested every time it is
 * needed, so Boundary's 15 rejects were only 9 distinct assets:
 *
 *   used high-water            200,384   (bump allocator: used IS the high-water)
 *   + unique refused, aligned   18,336   (9 assets, track overflow 0)
 *   = ANIM_REQUIRED_BYTES      218,720
 *   chosen                     262,144   -> 43,424 spare (19.9%)
 *
 * The old campaign estimate was ~82 KiB; the real shortfall is 18,016, so that
 * figure was 4.5x too large and sizing from it would have wasted ~64 KiB.
 *
 * Affordable because Phases 1-2 recovered 169,152 bytes of static main RAM,
 * which the taskman probe converted into arena: gNdsTaskmanGeneralHeapFreeMin
 * went 15,120 -> 133,628, i.e. from 17,648 BELOW the 32,768 safety floor to
 * 100,860 above it. Spending 61,440 of that leaves the floor with ~39 KiB of
 * margin, and the run's own gNdsTaskmanGeneralHeapFreeMin is the check on this
 * arithmetic rather than this comment. Overflow still degrades to the uncached
 * load. */
/* SLICE 1 PHASE 5, 2026-08-15. When the resident figatree pack is on, the arena
 * stops being a raw-file cache and becomes the pack's home. The two cannot both
 * exist: the arena-internal budget is
 *
 *     262,144   this reservation, reclaimed outright by residency
 *   +  39,420   gSYTaskmanGeneralHeap free-min 72,188 less the 32,768 floor
 *   ---------
 *     301,564   (BATTLEPACK_POOL.md section 1)
 *
 * and one fighter's blob is 287,904 (Fox) or 271,728 (Mario). 290,816 leaves the
 * Fox blob 2,912 bytes of bump slack and projects a general-heap low-water of
 * 43,516 against the mandated 32,768 floor -- 10,748 of margin. Both fighters
 * would need ~559,632 and do not fit, which is why this slice packs one and the
 * other keeps the generic path.
 *
 * CONSEQUENCE, STATED BECAUSE IT IS NOT FREE: the un-packed fighter loses this
 * cache. Its acquisitions were 95% hits and become ROM loads. That is a
 * PERFORMANCE outcome and never a correctness one -- every failure path here
 * degrades to the on-demand load -- but it is real, it is unmeasured on the gate
 * arm, and the remedy is to pack both fighters, which needs the taskman arena
 * itself grown out of the 146,560 B that RAM_RECOVERY_PLAN Phase 2 recovered. */
#if NDS_R2_BATTLEPACK
/* The ARM9 data cache line is 32 bytes and the FAT/DLDI read path may maintain
 * the destination by line, so the blob is placed on a line boundary and the
 * reserve rounded up to one. */
#define NDS_BATTLEPACK_LINE_BYTES 32u
#define NDS_BATTLEPACK_RESERVE_BYTES \
    ((NDS_R2_BATTLEPACK_BLOB_BYTES + (2u * NDS_BATTLEPACK_LINE_BYTES) - 1u) & \
     ~(NDS_BATTLEPACK_LINE_BYTES - 1u))
/* 4,096 of raw cache behind the pack. It is nearly nothing, and that is the
 * honest state of the one-fighter fit: the budget above is 301,564 and the Fox
 * blob is 287,904 of it. Projected general-heap low-water 42,236 against the
 * 32,768 floor.
 *
 * MEASURED 2026-08-15 (BATTLEPACK_GATE.md): that 4,096 is not "nearly nothing",
 * it is the whole verdict. The carve did not shrink the raw cache, it DELETED it
 * -- Fills 17 -> 2, Rejects 0 -> 126, hits 338 -> 30 -- and the 111 net-new
 * uncached acquisitions cost 3,873,969 ticks each, which is the entire
 * +2,261,376 at rank-80. So this pair cannot price the deletion's BENEFIT: the
 * starvation swamps it. NDS_R2_BATTLEPACK_KEEP_CACHE builds the arm that can. */
#if NDS_R2_BATTLEPACK_KEEP_CACHE
/* 163,840 of raw cache behind the pack, NOT the control's 262,144, and the
 * reason is the heap rather than the design. Blob + full cache = 550,080 needs
 * an arena of 0x18f000, and the measured grantable ceiling is 1,564,672 (see
 * NDS_TASKMAN_ARENA_SIZE): that arm starved the general heap to 6,076 B and the
 * battle never started. 287,936 + 163,840 = 451,776 against a 0x17a000 arena
 * projects a general-heap low-water of ~54,588, i.e. 21,820 clear of the floor.
 *
 * THE ACCEPTANCE TEST IS gNdsR2AnimCacheRejects == 0, not the byte count. A
 * smaller cache is equivalent to the control's exactly when it still refuses
 * nothing, and with Fox served from the pack this holds only Mario's working set
 * -- against a 192,240 B both-fighter arena high-water (slice 46). If Rejects is
 * non-zero the arm did not isolate the deletion and its ticks price starvation
 * again, which is the whole thing this configuration exists to avoid. */
#define NDS_R2_ANIM_CACHE_ARENA_BYTES (NDS_BATTLEPACK_RESERVE_BYTES + 163840u)
#else
#define NDS_R2_ANIM_CACHE_ARENA_BYTES (NDS_BATTLEPACK_RESERVE_BYTES + 4096u)
#endif
#else
#define NDS_R2_ANIM_CACHE_ARENA_BYTES 262144u
#endif

/* R2-04 E4. The match's animation working set, measured rather than guessed:
 * gNdsR204AnimSeen dumped at frame 1928 (roughly two thirds through the 3,600
 * tick match) after 230 force-loads, of which 189 (82.2%) were repeats of these
 * 41 assets. 14 Mario, 27 Fox, 91,104 bytes -- 12.5% of the 728,064 the full
 * 301-ID space would need, which E3 showed does not fit the arena.
 *
 * An asset missing from this list is a performance outcome, never a correctness
 * one: it simply takes the on-demand path it takes today. The list is derived
 * from observed play, so gNdsR204AnimForceLoadRepeat/Total is its regression
 * check -- if that ratio falls, the list has drifted from what the match uses.
 *
 * CYCLE 105 REPLACED THE LIST, BECAUSE IT WAS MEASURED ON THE WRONG ARM. The 41
 * above came off a Boundary match (Mario human vs one level-3 Fox). The gate arm
 * is NDS_R2_BOTH_CPU, where Mario is also a level-3 CPU, and two CPUs playing
 * their whole movesets touch a substantially wider set: gNdsR204AnimSeen dumped
 * at presented frame 2038 of the 1,600-sample window holds 85 IDs, and the old
 * list covered only 27 of them. The 14 it warmed that this match never asks for
 * were spending arena on assets nobody used.
 *
 * That is why the list moved rather than merely growing. The union of the two
 * lists is 99 assets = 229,024 bytes, which does NOT fit the heap budget (see
 * NDS_R2_ANIM_CACHE_ARENA_BYTES); the 85 measured here do, at 197,184. A Boundary
 * asset dropped from this list is a performance outcome and never a correctness
 * one -- it takes the on-demand path, and with the arena no longer full it is
 * then cached on that first use, which the old arena could not do.
 *
 * gNdsR204AnimForceLoadRepeat/Total remains the regression check on the list.
 *
 * SLICE 46 REPLACED IT AGAIN, FOR THE SAME REASON ONE LAYER DOWN. Cycle 105
 * fixed the ARM the list was measured on; it did not fix the fact that a list
 * measured once drifts. Dumped from the c123 gate ROM, `gNdsR204AnimSeen` holds
 * **87** ids and the 85-entry list above overlapped only **57** of them:
 *
 *   used AND warmed   57      warmed but NEVER used   28   (arena spent on nothing)
 *   used but NOT warm 30      <- these streamed from ROM inside the match
 *
 * The 30 are the misses. `gNdsR2AnimCacheMisses` reads 32 on the same run, and
 * they are not spread: the miss-only symbols `strncasecmp` and
 * `ndsRelocApplyWordByteSwap` sit on 22 of the 80 costliest frames, and capping
 * `SINT` on the 32 highest-`SINT` frames is worth **-32,512 WORK-H P95**.
 *
 * This SHRINKS the arena rather than growing it, which is why it needs no RAM
 * budget at all: the old list cached 83 warm entries and then filled 32 more on
 * demand -- 115 residents, 257,200 bytes, 98.1% of the arena -- where the
 * measured set is 87 at roughly 194,500. The 28 never-used warms were paying
 * for themselves twice, in arena bytes and in the countdown steps they consumed
 * while the assets the match actually wanted went unwarmed.
 *
 * An asset missing from this list is still a performance outcome and never a
 * correctness one, so tailoring it to the measured match is safe in the way a
 * gameplay change would not be: a miss simply takes the on-demand path.
 * `gNdsR204AnimForceLoadRepeat/Total` remains the drift check -- re-dump
 * `gNdsR204AnimSeen` and re-diff rather than assuming this list is still true.
 */
static const u16 sNdsR204AnimWarmList[] = {
    0x1f3u, 0x1f4u, 0x1f5u, 0x1fbu, 0x1fdu, 0x1feu, 0x1ffu, 0x200u,
    0x201u, 0x202u, 0x203u, 0x204u, 0x205u, 0x206u, 0x207u, 0x209u,
    0x20eu, 0x212u, 0x216u, 0x218u, 0x21au, 0x21cu, 0x21du, 0x21fu,
    0x220u, 0x223u, 0x228u, 0x229u, 0x22au, 0x22du, 0x22eu, 0x22fu,
    0x230u, 0x26cu, 0x26du, 0x270u, 0x271u, 0x272u, 0x27bu, 0x27cu,
    0x27du, 0x27eu, 0x27fu, 0x280u, 0x282u, 0x284u, 0x285u, 0x28au,
    0x28cu, 0x28du, 0x28eu, 0x28fu, 0x290u, 0x291u, 0x292u, 0x293u,
    0x294u, 0x295u, 0x296u, 0x298u, 0x2a0u, 0x2a7u, 0x2a8u, 0x2a9u,
    0x2abu, 0x2acu, 0x2afu, 0x2b5u, 0x2b9u, 0x2bcu, 0x2bdu, 0x2bfu,
    0x2f7u, 0x2fau, 0x2fbu, 0x2fcu, 0x302u, 0x30bu, 0x30cu, 0x311u,
    0x312u, 0x313u, 0x314u, 0x316u, 0x317u, 0x318u, 0x319u
};

typedef struct NDSR2AnimCacheEntry {
    u32 asset_id;
    u32 size;
    void *payload;
    NDSRelocAssetHeader header;
    /* R2-06 E13. TRUE when this payload's script region already carries the
     * ndsRelocNormalizeFighterAObj16File transform, so the load path can set
     * format_fixups_applied and skip it. Zero for anything the prebake declined,
     * which is why the load path reads the flag rather than assuming. */
    u32 aobj16_ready;
} NDSR2AnimCacheEntry;

#if NDS_R2_AOBJ16_PREBAKE
static sb32 ndsR2AnimPrebakeAObj16(u32 asset_id, void *payload, u32 size,
                                   const NDSRelocAssetHeader *header);
#endif

static NDSR2AnimCacheEntry sNdsR2AnimCache[NDS_R2_ANIM_CACHE_ENTRIES];
static u32 sNdsR2AnimCacheCount;
/* Reserved from gSYTaskmanGeneralHeap, NOT static BSS -- see the header comment on
 * NDS_R2_ANIM_CACHE_ARENA_BYTES for why BSS cannot afford this. */
static u8 *sNdsR2AnimCacheArena;
static u32 sNdsR2AnimCacheArenaBytes;
static u32 sNdsR2AnimCacheArenaUsed;
/* The taskman-heap generation this block was reserved under. Ownership is this
 * value matching gNdsTaskmanHeapGeneration; see ndsR2AnimCacheArenaStillOwned. */
static u32 sNdsR2AnimCacheArenaGeneration;
volatile u32 gNdsR2AnimCacheHits;
volatile u32 gNdsR2AnimCacheMisses;
volatile u32 gNdsR2AnimCacheFills;
volatile u32 gNdsR2AnimCacheBytes;
volatile u32 gNdsR2AnimCacheRejects;
/* The two ways ownership can fail, kept apart on purpose. A generation mismatch
 * is the ORDINARY second-entry case and should read non-zero on any run that
 * reaches Sudden Death or a rematch -- a zero there means the contract never
 * engaged and the run proves nothing. A range fault is not ordinary: it means
 * the block left the live region without a scene rewind. */
volatile u32 gNdsR2AnimCacheArenaGenerationMismatches;
volatile u32 gNdsR2AnimCacheArenaRangeFaults;
/* Engagement proof. An arena that never fills and an arena that overflows every
 * frame are indistinguishable from the reject count alone, and this campaign has
 * shipped a flag that silently never fired. */
volatile u32 gNdsR2AnimCacheArenaUsedBytes;
volatile u32 gNdsR2AnimCacheArenaOverflows;
volatile u32 gNdsR2AnimCacheArenaReservedBytes;
volatile u32 gNdsR2AnimCacheArenaReserveCount;
volatile u32 gNdsR2AnimCacheArenaReserveFailCount;
volatile u32 gNdsR2AnimCacheArenaInvalidations;
/* ANIM_REQUIRED_BYTES, measured. The plan (docs/RAM_RECOVERY_PLAN.md 0.3/6.1)
 * forbids sizing the arena from the old ~82 KiB estimate, and the reject COUNT
 * cannot size it either: a refused asset is re-requested every time it is
 * needed, so 15 rejects is 15 attempts over an unknown number of distinct
 * assets. Summing every refusal would therefore overcount badly.
 *
 * These track the DISTINCT assets that were refused and their bytes. The arena
 * is a bump allocator that never frees, so gNdsR2AnimCacheArenaUsedBytes is
 * already its high-water; the requirement is
 *     UsedBytes + RejectedUniqueBytes + per-entry 16-byte alignment slack.
 * 64 ids is far above the 15 refusals seen on Boundary and the 38 on the
 * both-CPU arm; Overflow is the tell if that assumption ever breaks. */
#define NDS_R2_ANIM_REJECT_TRACK_MAX 64u
static u32 sNdsR2AnimRejectedIds[NDS_R2_ANIM_REJECT_TRACK_MAX];
static u32 sNdsR2AnimRejectedCount;
volatile u32 gNdsR2AnimCacheRejectedUniqueBytes;
volatile u32 gNdsR2AnimCacheRejectedUniqueCount;
volatile u32 gNdsR2AnimCacheRejectedTrackOverflow;

/* Record a refusal once per distinct asset. Linear scan: the set is tiny and
 * this only runs on the failure path, which is by definition rare. */
static void ndsR2AnimNoteRejected(u32 asset_id, u32 size)
{
    u32 i;

    for (i = 0u; i < sNdsR2AnimRejectedCount; i++)
    {
        if (sNdsR2AnimRejectedIds[i] == asset_id)
        {
            return;
        }
    }
    if (sNdsR2AnimRejectedCount >= NDS_R2_ANIM_REJECT_TRACK_MAX)
    {
        gNdsR2AnimCacheRejectedTrackOverflow++;
        return;
    }
    sNdsR2AnimRejectedIds[sNdsR2AnimRejectedCount++] = asset_id;
    gNdsR2AnimCacheRejectedUniqueCount = sNdsR2AnimRejectedCount;
    gNdsR2AnimCacheRejectedUniqueBytes += (size + 15u) & ~15u;
}
/* WHY the last overflow overflowed. Overflows 126 beside UsedBytes 3,728 and
 * ReservedBytes 92,160 is not self-consistent -- a 92 KiB arena holding 3.7 KiB
 * cannot refuse an animation of a few KiB -- and on 2026-08-02 that ambiguity
 * cost a reading. These two make the arithmetic checkable from the soak: the
 * request that was refused, and the arena's own cursor at that instant. */
volatile u32 gNdsR2AnimCacheArenaOverflowLastSize;
volatile u32 gNdsR2AnimCacheArenaOverflowLastUsed;

/* Take the arena from the heap it is protecting, once, and only out of what is
 * genuinely spare.
 *
 * The reservation happens LAZILY, on the first store rather than at battle start,
 * and that ordering is the safety property: by the time any animation wants to be
 * cached, ftManagerSetupFilesAllKind has already taken its 116,752 bytes for the
 * fighters. So this can never be the allocation that starves battle start -- the
 * failure mode that a 128 KiB static array caused and that cost the owner a ROM
 * that could not start a match.
 *
 * NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE is then the second half of that guarantee:
 * syTaskmanMalloc is only called when the request AND that reserve both fit, so
 * the cache declines rather than consuming the last of the heap. Declining is
 * free -- it degrades to the on-demand load, which is what the port did before
 * this cache existed. */
#define NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE 32768u

/* True while the reserved block is still ours, decided by the heap GENERATION
 * and not by where the cursor happens to sit.
 *
 * gSYTaskmanGeneralHeap is a bump region that every scene entry rewinds. This
 * used to infer ownership from the cursor -- "our block ends at or before
 * `ptr`, so it has not been reclaimed" -- which is a heuristic, and it
 * false-positives exactly on the path that matters. A second entry into the
 * battle scene (Sudden Death, or START-rematch) rewinds the heap and then
 * allocates; as soon as the new scene's own allocations push the cursor back
 * past our stale block, the test starts passing again and every cached payload
 * pointer is handed out pointing into memory the new scene already owns. That
 * is silent corruption, and strictly worse than the hang the cache was moved to
 * the heap to avoid.
 *
 * `gNdsTaskmanHeapGeneration` cannot be fooled that way: it is bumped at the
 * two primitives that move the cursor backwards, so a mismatch is proof the
 * block is dead no matter where the cursor is now. The range test is retained
 * BELOW it as a secondary corruption check -- if the block ever falls outside
 * the live region while the generation still matches, something other than a
 * scene rewind has moved it, and that is worth failing on too. */
static sb32 ndsR2AnimCacheArenaStillOwned(void)
{
    const u8 *cursor = (const u8 *)gSYTaskmanGeneralHeap.ptr;

    if ((sNdsR2AnimCacheArena == NULL) || (sNdsR2AnimCacheArenaBytes == 0u))
    {
        return FALSE;
    }
    if (sNdsR2AnimCacheArenaGeneration != gNdsTaskmanHeapGeneration)
    {
        gNdsR2AnimCacheArenaGenerationMismatches++;
        return FALSE;
    }
    if ((sNdsR2AnimCacheArena < (const u8 *)gSYTaskmanGeneralHeap.start) ||
        ((sNdsR2AnimCacheArena + sNdsR2AnimCacheArenaBytes) > cursor))
    {
        /* Same generation but out of range: not a scene rewind. Fail closed and
         * count it separately so the two causes are never conflated. */
        gNdsR2AnimCacheArenaRangeFaults++;
        return FALSE;
    }
    return TRUE;
}

/* Defined below with the residency loader; declared here because THIS is the
 * one seam where the arena stops being ours, and the pack lives in it. Binding
 * the invalidation to the writer rather than to each reader is the shape the
 * gMPCollisionGeometry fix settled on. */
static void ndsBattlePackResidencyDrop(void);

static void ndsR2AnimCacheArenaDropForReset(void)
{
    /* BEFORE the pointers are cleared: the pack's storage is inside this block,
     * so a lookup that survived the rewind would otherwise hand the parser a
     * figatree pointing into memory the next scene already owns. */
    ndsBattlePackResidencyDrop();
    sNdsR2AnimCacheArena = NULL;
    sNdsR2AnimCacheArenaBytes = 0u;
    sNdsR2AnimCacheArenaUsed = 0u;
    sNdsR2AnimCacheArenaGeneration = 0u;
    /* Drops every entry, which is what makes the payload pointers unreachable.
     * The entries are the only holders of those pointers. */
    sNdsR2AnimCacheCount = 0u;
    gNdsR2AnimCacheArenaUsedBytes = 0u;
    gNdsR2AnimCacheArenaReservedBytes = 0u;
    gNdsR2AnimCacheBytes = 0u;
    gNdsR2AnimCacheArenaInvalidations++;
}

/* Invalidate before ANY use, not only on the lookup path. Called at the top of
 * every cache entry point: a stale generation must never survive into a find, a
 * store or a preload step, and the preload path in particular used to touch the
 * cursor and the warm list without consulting ownership at all. Cheap -- one
 * compare when the cache is empty, which is the state this leaves it in. */
static void ndsR2AnimCacheValidateGeneration(void)
{
    if ((sNdsR2AnimCacheArena != NULL) &&
        (ndsR2AnimCacheArenaStillOwned() == FALSE))
    {
        ndsR2AnimCacheArenaDropForReset();
    }
}

static sb32 ndsR2AnimCacheArenaEnsure(void)
{
    void *block;

    if (ndsR2AnimCacheArenaStillOwned() != FALSE)
    {
        return TRUE;
    }
    if (sNdsR2AnimCacheArena != NULL)
    {
        /* Had a block, no longer owns it: the heap was rewound under us. Every
         * cached pointer is stale, so drop the whole cache before re-reserving. */
        ndsR2AnimCacheArenaDropForReset();
    }
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                            (size_t)NDS_R2_ANIM_CACHE_ARENA_BYTES +
                                NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE,
                            NDS_RELOC_ALIGN_BYTES) == FALSE)
    {
        gNdsR2AnimCacheArenaReserveFailCount++;
        return FALSE;
    }
    block = syTaskmanMalloc((size_t)NDS_R2_ANIM_CACHE_ARENA_BYTES,
                            NDS_RELOC_ALIGN_BYTES);
    if (block == NULL)
    {
        gNdsR2AnimCacheArenaReserveFailCount++;
        return FALSE;
    }
    sNdsR2AnimCacheArena = block;
    sNdsR2AnimCacheArenaBytes = NDS_R2_ANIM_CACHE_ARENA_BYTES;
#if NDS_R2_BATTLEPACK
    /* CARVE THE PACK'S REGION FIRST, at reservation, not by being the first
     * caller of the bump allocator. Ordering was the first design and it lost:
     * fighter setup stores 3,728 bytes of animation into this arena before the
     * first scene update runs a pack step, so the 287,936-byte request was
     * refused by 848 bytes and `gNdsBattlePackHits` read 0 with
     * `gNdsR2AnimCacheArenaOverflowLastSize 287936` naming the victim
     * (soak 2026-08-15_015724). Now the blob owns [0, RESERVE) of every arena
     * generation and no store can get in front of it. */
    sNdsR2AnimCacheArenaUsed = NDS_BATTLEPACK_RESERVE_BYTES;
#else
    sNdsR2AnimCacheArenaUsed = 0u;
#endif
    /* Stamp the generation the block was taken under. Must be read AFTER the
     * allocation: syTaskmanMalloc cannot rewind the heap, so this is the same
     * value either way, but taking it here keeps the store adjacent to the
     * pointer it qualifies. */
    sNdsR2AnimCacheArenaGeneration = gNdsTaskmanHeapGeneration;
    gNdsR2AnimCacheArenaReservedBytes = NDS_R2_ANIM_CACHE_ARENA_BYTES;
    gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    gNdsR2AnimCacheArenaReserveCount++;
    return TRUE;
}

/* Bump allocation from the cache's own arena. Returns NULL on overflow, which is
 * the whole point: see the comment on NDS_R2_ANIM_CACHE_ARENA_BYTES. */
static void *ndsR2AnimCacheArenaAlloc(u32 size)
{
    u32 aligned;

    if (ndsR2AnimCacheArenaEnsure() == FALSE)
    {
        gNdsR2AnimCacheArenaOverflows++;
        gNdsR2AnimCacheArenaOverflowLastSize = size;
        gNdsR2AnimCacheArenaOverflowLastUsed = sNdsR2AnimCacheArenaUsed;
        return NULL;
    }
    aligned = (sNdsR2AnimCacheArenaUsed + 15u) & ~15u;
    if ((size == 0u) || (aligned > sNdsR2AnimCacheArenaBytes) ||
        (size > (sNdsR2AnimCacheArenaBytes - aligned)))
    {
        gNdsR2AnimCacheArenaOverflows++;
        gNdsR2AnimCacheArenaOverflowLastSize = size;
        gNdsR2AnimCacheArenaOverflowLastUsed = sNdsR2AnimCacheArenaUsed;
        return NULL;
    }
    sNdsR2AnimCacheArenaUsed = aligned + size;
    gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    return &sNdsR2AnimCacheArena[aligned];
}

/* TRUE when no allocation of any size can succeed. Exact, not a threshold: this
 * is ndsR2AnimCacheArenaAlloc's own refusal test with `size` at its minimum.
 *
 * It exists because ndsR2AnimWarmLoadOne calls ndsRelocAssetAllocSize -- a real
 * fopen plus header read -- BEFORE it asks the arena for bytes, so a full arena
 * paid one NitroFS open per remaining warm entry to learn something it already
 * knew. That was tolerable when the arena had 19.9% spare; with the resident
 * pack occupying it, it would be the whole rest of the list. */
static sb32 ndsR2AnimCacheArenaExhausted(void)
{
    u32 aligned;

    if (ndsR2AnimCacheArenaEnsure() == FALSE)
    {
        return TRUE;
    }
    aligned = (sNdsR2AnimCacheArenaUsed + 15u) & ~15u;
    return (aligned >= sNdsR2AnimCacheArenaBytes) ? TRUE : FALSE;
}

/* Give back the most recent allocation. Only valid for the immediately preceding
 * alloc with nothing in between, which is exactly the shape of the warm loader's
 * failure path -- it used to leak the buffer it had just taken. */
static void ndsR2AnimCacheArenaRelease(void *payload, u32 size)
{
    u8 *bytes = payload;

    if ((sNdsR2AnimCacheArena != NULL) && (bytes != NULL) &&
        ((bytes + size) == &sNdsR2AnimCacheArena[sNdsR2AnimCacheArenaUsed]))
    {
        sNdsR2AnimCacheArenaUsed = (u32)(bytes - sNdsR2AnimCacheArena);
        gNdsR2AnimCacheArenaUsedBytes = sNdsR2AnimCacheArenaUsed;
    }
}

/* ---- Slice 1 phase 5: the resident figatree pack's setup-time load --------
 *
 * The blob is a NitroFS payload streamed into the arena above, NOT `.incbin`
 * into `.rodata`: +288,992 B of ARM9 image was MEASURED to push
 * gNdsTaskmanArenaChosenSize 0x150000 -> 0x140000 with 16 alloc failures,
 * because the arena is one calloc from the same libnds heap the image bounds.
 * See the NDS_R2_ANIM_CACHE_ARENA_BYTES comment and BATTLEPACK_POOL.md.
 *
 * IT IS STEPPED, AND THE BOUND IS NOT LOADING-TIME GENEROSITY. It rides
 * ndsR2AnimCachePreloadStep for two reasons the warm walk already paid for:
 *
 *   1. The BGM packet seam. E4 loaded 41 assets in ONE call at this seam and
 *      Boundary refused the build on the ADPCM smoke -- the stream is
 *      double-buffered at 8,196 bytes against 44,100 a second, so the main
 *      thread owns ~186 ms between seams. 287,904 bytes in one read does not
 *      fit it.
 *   2. Lazy reservation is a SAFETY property, not an optimisation. Taking the
 *      arena on the first step rather than at battle start means
 *      ftManagerSetupFilesAllKind has already taken its 116,752 bytes, so this
 *      can never be the allocation that starves battle start -- the failure
 *      mode a 128 KiB static array once caused.
 *
 * Every failure degrades to the generic acquisition path: the pack is simply
 * not published, ndsBattlePackFindFigatree returns NULL, and the load runs as
 * it does today. A partial blob is never published. */
#if NDS_R2_BATTLEPACK

#define NDS_BATTLEPACK_PATH "nitro:/animation/battlepack_fox.bin"
/* 40 B of header padded to the 48 B directory offset (generate_battlepack_anim
 * BLOB_DIR_OFF). Read as words so the u32 fields are naturally aligned -- an
 * unaligned ARM9 LDR rotates the word instead of faulting. */
#define NDS_BATTLEPACK_HEADER_WORDS 12u
/* One chunk per scene update. 287,904 / 16,384 = 18 steps, and the countdown is
 * far longer than that; the warm walk's own step is 4 assets, i.e. 4 NitroFS
 * opens plus ~8 KB, so this is one open plus 16 KB and stays in the same class.
 */
#define NDS_BATTLEPACK_STEP_BYTES 16384u

#define NDS_BATTLEPACK_LOAD_IDLE 0u
#define NDS_BATTLEPACK_LOAD_STREAMING 1u
#define NDS_BATTLEPACK_LOAD_DONE 2u
#define NDS_BATTLEPACK_LOAD_FAILED 3u

static u8 *sNdsBattlePackDst;
static u32 sNdsBattlePackTotal;
static u32 sNdsBattlePackCursor;
static u32 sNdsBattlePackLoadState;

static void ndsBattlePackResidencyDrop(void)
{
    ndsBattlePackDrop();
    sNdsBattlePackDst = NULL;
    sNdsBattlePackTotal = 0u;
    sNdsBattlePackCursor = 0u;
    sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_IDLE;
    gNdsBattlePackResidentBytes = 0u;
}

/* Retry a load that failed, without re-streaming one that succeeded. */
static void ndsBattlePackResidencyRearm(void)
{
    if (sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_FAILED)
    {
        sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_IDLE;
    }
}

static void ndsBattlePackResidencyFail(void)
{
    /* Hand the reserve to the raw cache rather than stranding 288 KiB of arena
     * on a degraded run -- but ONLY while the cache has taken nothing yet, or
     * rewinding the cursor would alias entries that already live above it. */
    if ((sNdsR2AnimCacheArena != NULL) &&
        (sNdsR2AnimCacheArenaUsed == NDS_BATTLEPACK_RESERVE_BYTES))
    {
        sNdsR2AnimCacheArenaUsed = 0u;
        gNdsR2AnimCacheArenaUsedBytes = 0u;
    }
    ndsBattlePackResidencyDrop();
    sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_FAILED;
    gNdsBattlePackLoadFails++;
}

/* TRUE while the pack still needs steps, so the caller holds the warm walk off
 * the arena until the blob owns its bytes. */
static sb32 ndsBattlePackResidencyStep(void)
{
    u32 want;

    if ((sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_DONE) ||
        (sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_FAILED))
    {
        return FALSE;
    }
    if (sNdsBattlePackLoadState == NDS_BATTLEPACK_LOAD_IDLE)
    {
        u32 head[NDS_BATTLEPACK_HEADER_WORDS];
        u32 total;
        uintptr_t base;

        /* The reserve exists only inside a live arena, so take ownership first.
         * This is also where the lazy reservation happens -- see the header
         * comment: at the first scene update, not at battle start. */
        if (ndsR2AnimCacheArenaEnsure() == FALSE)
        {
            ndsBattlePackResidencyFail();
            return FALSE;
        }
        if (ndsRelocAssetReadRawRange(NDS_BATTLEPACK_PATH, 0u, head,
                                      (u32)sizeof(head)) == FALSE)
        {
            ndsBattlePackResidencyFail();
            return FALSE;
        }
        /* head[2] is blob_bytes. Magic and version are re-checked by
         * ndsBattlePackAdopt against the bytes that actually landed; all this
         * needs is a length that fits the carved reserve. A blob larger than
         * the build-time NDS_R2_BATTLEPACK_BLOB_BYTES the reserve was sized
         * from means the NitroFS payload and the ARM9 image disagree, which is
         * a build defect and is refused rather than truncated. */
        total = head[2];
        if ((total < (u32)sizeof(head)) ||
            (total > (NDS_BATTLEPACK_RESERVE_BYTES -
                      NDS_BATTLEPACK_LINE_BYTES)))
        {
            ndsBattlePackResidencyFail();
            return FALSE;
        }
        base = ((uintptr_t)sNdsR2AnimCacheArena +
                (NDS_BATTLEPACK_LINE_BYTES - 1u)) &
               ~(uintptr_t)(NDS_BATTLEPACK_LINE_BYTES - 1u);
        sNdsBattlePackDst = (u8 *)base;
        sNdsBattlePackTotal = total;
        /* From zero, not from sizeof(head): re-reading the 48 header bytes
         * through the same path costs nothing and keeps the CPU from having
         * written any byte the file read may later invalidate a line under. */
        sNdsBattlePackCursor = 0u;
        sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_STREAMING;
    }
    want = sNdsBattlePackTotal - sNdsBattlePackCursor;
    if (want > NDS_BATTLEPACK_STEP_BYTES)
    {
        want = NDS_BATTLEPACK_STEP_BYTES;
    }
    if (ndsRelocAssetReadRawRange(NDS_BATTLEPACK_PATH, sNdsBattlePackCursor,
                                  &sNdsBattlePackDst[sNdsBattlePackCursor],
                                  want) == FALSE)
    {
        ndsBattlePackResidencyFail();
        return FALSE;
    }
    sNdsBattlePackCursor += want;
    gNdsBattlePackLoadSteps++;
    if (sNdsBattlePackCursor < sNdsBattlePackTotal)
    {
        return TRUE;
    }
    if (ndsBattlePackAdopt(sNdsBattlePackDst, sNdsBattlePackTotal) == FALSE)
    {
        ndsBattlePackResidencyFail();
        return FALSE;
    }
    sNdsBattlePackLoadState = NDS_BATTLEPACK_LOAD_DONE;
    gNdsBattlePackResidentBytes = sNdsBattlePackTotal;
    return FALSE;
}

#else /* !NDS_R2_BATTLEPACK */

static void ndsBattlePackResidencyDrop(void)
{
}

static void ndsBattlePackResidencyRearm(void)
{
}

static sb32 ndsBattlePackResidencyStep(void)
{
    return FALSE;
}

#endif /* NDS_R2_BATTLEPACK */

static NDSR2AnimCacheEntry *ndsR2AnimCacheFind(u32 asset_id)
{
    u32 i;

    /* The ownership test belongs on the READ path too, not only where the arena is
     * reserved. Every entry's payload points into the reserved block, so once the
     * heap has been rewound a hit would hand back a pointer into memory the next
     * scene is already reusing -- silent corruption, and strictly worse than the
     * hang this whole change set exists to remove. A miss is free.
     *
     * Cheap by construction: two pointer compares against a bump cursor, and only
     * when the cache is non-empty. */
    ndsR2AnimCacheValidateGeneration();
    if (sNdsR2AnimCacheCount == 0u)
    {
        return NULL;
    }
    for (i = 0u; i < sNdsR2AnimCacheCount; i++)
    {
        if (sNdsR2AnimCache[i].asset_id == asset_id)
        {
            return &sNdsR2AnimCache[i];
        }
    }
    return NULL;
}

/* Called with the payload already byte-swapped and not yet fixed up. */
static void ndsR2AnimCacheStore(u32 asset_id, const void *data, u32 size,
                                const NDSRelocAssetHeader *header)
{
    NDSR2AnimCacheEntry *entry;
    void *payload;

    if ((data == NULL) || (size == 0u) || (header == NULL) ||
        (sNdsR2AnimCacheCount >= NDS_R2_ANIM_CACHE_ENTRIES))
    {
        gNdsR2AnimCacheRejects++;
        return;
    }
    payload = ndsR2AnimCacheArenaAlloc(size);
    if (payload == NULL)
    {
        ndsR2AnimNoteRejected(asset_id, size);
        gNdsR2AnimCacheRejects++;
        return;
    }
    memcpy(payload, data, size);
    entry = &sNdsR2AnimCache[sNdsR2AnimCacheCount++];
    entry->asset_id = asset_id;
    entry->size = size;
    entry->payload = payload;
    entry->header = *header;
    /* Written unconditionally, and that is not defensive tidiness: entries are
     * reused across a scene rewind, so leaving this field alone would let a slot
     * keep a stale TRUE from a previous match's asset and skip a transform that
     * had never been applied to these bytes. */
#if NDS_R2_AOBJ16_PREBAKE
    entry->aobj16_ready =
        (ndsR2AnimPrebakeAObj16(asset_id, payload, size, header) != FALSE)
            ? 1u : 0u;
#else
    entry->aobj16_ready = 0u;
#endif
    gNdsR2AnimCacheFills++;
    gNdsR2AnimCacheBytes += size;
}

volatile u32 gNdsR2AnimWarmLoaded;
volatile u32 gNdsR2AnimWarmBytes;
volatile u32 gNdsR2AnimWarmFailed;
static u32 sNdsR204AnimWarmCursor;

#if NDS_R2_AOBJ16_PREBAKE
/* R2-06 E13. Move ndsRelocNormalizeFighterAObj16File off the gameplay frame.
 *
 * WHY THIS ONE. Cycle 107 attributed the load frame exactly
 * (task37_census.py --split-by-symbol ndsRelocFinalizeLoadedFile, 74 load frames
 * against 326 control, premium 650,610 cycles/frame): the reloc + copy family is
 * 23.6% of it and ndsRelocFinalizeLoadedFile alone is 10.0%, of which the AObj16
 * pass is the bulk -- 10,236,800 ticks a match against a 63,115,584 SINT
 * excursion, i.e. 16.2%, or ~29,000 per force-load frame. R2-06 E11's standing
 * rule is that a load-frame saving cannot be banked by making the work faster,
 * only by moving it off the frame, which is what cycle 105's arena fix did for
 * the cartridge read. This does it for the transform.
 *
 * WHY IT IS SOUND, and the two facts that make it so:
 *
 *   1. The transform is position-INDEPENDENT. The swap loop runs from
 *      `table_bytes` to the end, and the normalize walks each script; neither
 *      touches the pointer table, and every quantity either derives from is an
 *      OFFSET (`value - base`). Running it at warm time against the arena base
 *      therefore produces byte-identical script bytes to running it at load time
 *      against the caller's heap.
 *   2. Nothing runs between. ndsRelocFinalizeLoadedFile's order is internal
 *      fixups, then AObj16, then attributes/weapon/sprites/external -- so the
 *      only pass that can change the input is the internal fixup, and that is
 *      what this reproduces.
 *
 * The internal fixups have to run first because the AObj16 pass reads the table
 * as ABSOLUTE pointers, which only exist once they are written. They are an
 * intrusive linked list threaded through the slots themselves -- each raw word
 * is (next_slot_index << 16 | target_word_index) -- so applying them CONSUMES
 * the list. This therefore records every (offset, raw word) pair while walking
 * the list, applies, transforms, and writes the raw words back. The restore is
 * exact by construction and independent of where the slots sit, which is
 * deliberately stronger than restoring a table region and assuming no slot lives
 * past it.
 *
 * Every failure declines the asset with `aobj16_ready` 0 and leaves the payload
 * as it was, so a decline is a performance outcome and never a correctness one --
 * the load path then runs the pass exactly as it does today.
 *
 * Sized by measurement, not by fear: the first arm ran a 512-slot scratch and
 * gNdsR2AObj16PrebakeSlotsMax read 21 over the whole match, so 64 keeps a 3x
 * margin and hands 3,584 bytes back to a heap whose free-min sits 9,368 above
 * the anim cache's own KEEP_FREE reserve. A longer list declines the asset. */
#define NDS_R2_AOBJ16_PREBAKE_SLOTS_MAX 64u

typedef struct NDSR2AObj16PrebakeSlot {
    u32 offset;
    u32 word;
} NDSR2AObj16PrebakeSlot;

static NDSR2AObj16PrebakeSlot
    sNdsR2AObj16PrebakeSlots[NDS_R2_AOBJ16_PREBAKE_SLOTS_MAX];

/* Same-binary A/B route. Two builds of this change differing only by 3,584
 * bytes of scratch read WORK-H P50 1,093,152 and 1,119,136 -- 25,760 apart,
 * 4.5x the cross-build P50 floor -- while the second one did strictly LESS
 * work (351 skips against 259). Relinking moved the body further than the
 * change did, exactly as R2-06 E11 says it will, so the two arms have to be
 * the SAME bytes. Poked to 0 by the harness at the first frame-complete
 * marker; warm stepping runs one asset per scene update, so at most one entry
 * can be prebaked before the poke lands and gNdsR2AObj16PrebakeReady reports
 * it. Route 0 is a decline, which is a performance outcome only. */
volatile u32 gNdsR2AObj16PrebakeRoute = 1u;

volatile u32 gNdsR2AObj16PrebakeReady;
volatile u32 gNdsR2AObj16PrebakeSlotsMax;
volatile u32 gNdsR2AObj16PrebakeDeclineKind;
volatile u32 gNdsR2AObj16PrebakeDeclineList;
volatile u32 gNdsR2AObj16PrebakeDeclineFixup;
volatile u32 gNdsR2AObj16PrebakeDeclineFormat;
volatile u32 gNdsR2AObj16PrebakeSkips;

static void ndsR2AObj16PrebakeRestore(void *payload, u32 count)
{
    u32 i;

    for (i = 0u; i < count; i++)
    {
        ndsRelocWriteNative32((u8 *)payload + sNdsR2AObj16PrebakeSlots[i].offset,
                              sNdsR2AObj16PrebakeSlots[i].word);
    }
}

static sb32 ndsR2AnimPrebakeAObj16(u32 asset_id, void *payload, u32 size,
                                   const NDSRelocAssetHeader *header)
{
    NDSRelocLoadedFile view;
    u32 count = 0u;
    u32 guard;
    u16 slot_index;

    if (gNdsR2AObj16PrebakeRoute == 0u)
    {
        return FALSE;
    }

    if ((payload == NULL) || (size == 0u) || (header == NULL) ||
        (ndsRelocIsFighterAObj16Asset(asset_id) == FALSE))
    {
        gNdsR2AObj16PrebakeDeclineKind++;
        return FALSE;
    }

    slot_index = header->reloc_intern_offset;
    guard = (size / sizeof(u32)) + 1u;
    while (slot_index != 0xffffu)
    {
        u32 offset = (u32)slot_index * sizeof(u32);

        if ((guard == 0u) || ((offset + sizeof(u32)) > size) ||
            (count >= NDS_R2_AOBJ16_PREBAKE_SLOTS_MAX))
        {
            gNdsR2AObj16PrebakeDeclineList++;
            return FALSE;
        }
        guard--;
        sNdsR2AObj16PrebakeSlots[count].offset = offset;
        sNdsR2AObj16PrebakeSlots[count].word =
            (u32)ndsRelocReadNative32((u8 *)payload + offset);
        slot_index = (u16)(sNdsR2AObj16PrebakeSlots[count].word >> 16);
        count++;
    }
    if (count > gNdsR2AObj16PrebakeSlotsMax)
    {
        gNdsR2AObj16PrebakeSlotsMax = count;
    }

    memset(&view, 0, sizeof(view));
    view.asset_id = asset_id;
    view.data = payload;
    view.data_size = size;
    view.reloc_intern_offset = header->reloc_intern_offset;
    view.reloc_extern_offset = header->reloc_extern_offset;

    if (ndsRelocApplyInternalPointerFixups(&view) == FALSE)
    {
        ndsR2AObj16PrebakeRestore(payload, count);
        gNdsR2AObj16PrebakeDeclineFixup++;
        return FALSE;
    }
    if (ndsRelocNormalizeFighterAObj16File(&view) == FALSE)
    {
        ndsR2AObj16PrebakeRestore(payload, count);
        gNdsR2AObj16PrebakeDeclineFormat++;
        return FALSE;
    }

    ndsR2AObj16PrebakeRestore(payload, count);
    gNdsR2AObj16PrebakeReady++;
    return TRUE;
}
#endif

/* R2-04 E4. Task 75's absorption proper: make the match's animation streams
 * resident so no gameplay frame pays a NitroFS walk and a cartridge read for a
 * move.
 *
 * Deliberately does NOT go through ndsRelocRegisterLoadedFile. Registering 41
 * assets that all live at one scratch address would consume half of
 * NDS_RELOC_LOADED_FILE_CAPACITY (96) and leave every entry pointing at storage
 * this function owns. Instead each asset gets its own buffer and the word swap is
 * done here: ndsRelocApplyWordByteSwap is a plain big-endian-to-native pass over
 * data_size, and its only side effects are counters for the N64 logo and the
 * opening-room assets, neither of which a fighter animation can be. The cached
 * image is therefore identical to the one the miss path snapshots.
 *
 * Every failure degrades to the on-demand load. */
static void ndsR2AnimWarmLoadOne(u32 asset_id)
{
    NDSRelocAssetHeader header;
    size_t alloc_size;
    size_t loaded_size = 0u;
    void *payload;
    u32 words;
    u32 w;

    if (ndsR2AnimCacheFind(asset_id) != NULL)
    {
        return;
    }
    /* Already resident in the pack. Warming it into the raw cache would spend a
     * NitroFS open and a second copy of bytes the acquisition path stopped
     * reading the moment the pack answered for this id. */
    if (ndsBattlePackFindFigatree(asset_id) != NULL)
    {
        return;
    }
    if (ndsR2AnimCacheArenaExhausted() != FALSE)
    {
        gNdsR2AnimWarmFailed++;
        return;
    }
    alloc_size = ndsRelocAssetAllocSize(asset_id);
    if ((alloc_size == 0u) ||
        (sNdsR2AnimCacheCount >= NDS_R2_ANIM_CACHE_ENTRIES))
    {
        gNdsR2AnimWarmFailed++;
        return;
    }
    payload = ndsR2AnimCacheArenaAlloc((u32)alloc_size);
    if (payload == NULL)
    {
        ndsR2AnimNoteRejected(asset_id, (u32)alloc_size);
        gNdsR2AnimWarmFailed++;
        return;
    }
    if (ndsRelocAssetLoadIntoZeroedHeap(asset_id, payload,
                                        NDS_RELOC_ALIGN_BYTES,
                                        &loaded_size, &header) == FALSE)
    {
        /* Used to return here having already taken the buffer, so a failed warm
         * load permanently consumed arena for nothing. */
        ndsR2AnimCacheArenaRelease(payload, (u32)alloc_size);
        gNdsR2AnimWarmFailed++;
        return;
    }
    words = (u32)(loaded_size / sizeof(u32));
    for (w = 0u; w < words; w++)
    {
        void *word = (u8 *)payload + (w * sizeof(u32));

        ndsRelocWriteNative32(word, ndsRelocReadBe32(word));
    }
    {
        NDSR2AnimCacheEntry *entry = &sNdsR2AnimCache[sNdsR2AnimCacheCount++];

        entry->asset_id = asset_id;
        entry->size = (u32)loaded_size;
        entry->payload = payload;
        entry->header = header;
#if NDS_R2_AOBJ16_PREBAKE
        /* AFTER the word swap above and BEFORE anyone can copy this out: the
         * transform's input is the swapped, pre-fixup image, which is exactly
         * what the miss path snapshots too. */
        entry->aobj16_ready =
            (ndsR2AnimPrebakeAObj16(asset_id, payload, (u32)loaded_size,
                                    &header) != FALSE) ? 1u : 0u;
#else
        entry->aobj16_ready = 0u;
#endif
    }
    gNdsR2AnimWarmLoaded++;
    gNdsR2AnimWarmBytes += (u32)loaded_size;
    gNdsR2AnimCacheBytes += (u32)loaded_size;
}

/* R2-04 E5. Arms the warm walk; it does not load anything itself.
 *
 * E4 loaded all 41 in one call at this seam and Boundary refused the build on
 * the BGM ADPCM smoke: SeamMissCount 0 -> 1, ErrorStopCount 0 -> 1,
 * OverrunCount 0 -> 1, gNdsAudioBgmPlaying 1 -> 0 with StopCalls still 0. The
 * stream is double-buffered at 8,196 bytes per packet against 44,100 bytes per
 * second, so the main thread owns a hard ~186 ms budget between buffer seams,
 * and 41 back-to-back NitroFS walks plus 84 KB of cartridge reads do not fit in
 * it. Missing one seam kills BGM for the rest of the match.
 *
 * Standing consequence, recorded in TASK_STANDING_RULES: prepare-at-load work
 * on this seam is bounded by the BGM packet duration, not by loading-time
 * generosity. Anything longer has to be stepped. */
void ndsR2AnimCachePreloadMatch(void)
{
    /* Arm the walk AND settle ownership first. This is the second-entry seam:
     * Sudden Death and the rematch both call it after the heap has been rewound
     * under a cache still holding last match's entries, and it previously only
     * touched the cursor -- so the stale entries survived into the new match and
     * the first find could hand back a pointer into reused memory. */
    ndsR2AnimCacheValidateGeneration();
    sNdsR204AnimWarmCursor = 0u;
    /* And re-arm the pack. The generation test above already dropped it if the
     * heap moved; this covers the case where it did NOT -- a re-entry that
     * reuses the same generation must still not be left with a FAILED state
     * from the previous match's attempt. Re-arming a DONE pack would re-stream
     * 287,904 bytes for nothing, so only a failed load is retried. */
    ndsBattlePackResidencyRearm();
}

/* R2-04 E5. One asset per scene update. The countdown alone is far longer than
 * the 41 frames this needs, so the working set is resident before the first
 * scored frame, and a stepped frame costs exactly what the on-demand path
 * already costs when a fighter changes action -- which demonstrably does not
 * miss a seam.
 *
 * SLICE 46: that stopped being true once the list grew to 85. Measured on the
 * c123 gate arm, `gNdsR2AnimWarmLoaded` is **83 of 85** at the end of a whole
 * match -- the walk never finishes -- and the match takes 32 anim-cache MISSES
 * inside the sample window. They are not cheap and they are not spread: the
 * miss-only symbols `strncasecmp` and `ndsRelocApplyWordByteSwap` sit on 22 of
 * the 80 costliest frames, and capping `SINT` on just the 32 highest-`SINT`
 * frames is worth **-32,512 WORK-H P95**.
 *
 * So step several per update. The bound is NOT loading-time generosity, it is
 * the BGM packet seam: E4 loaded all 41 in ONE call here and Boundary refused
 * the build on the ADPCM smoke (SeamMiss 0->1, gNdsAudioBgmPlaying 1->0),
 * because the stream is double-buffered at 8,196 bytes against 44,100 a second
 * and the main thread owns ~186 ms between seams. E4's failure puts a real
 * number on one load: 41 did not fit 186 ms, so a load is >4.5 ms and a safe
 * step is single digits, not tens. Four finishes the 85-entry list in 22
 * updates and spends ~18 ms of a 186 ms budget.
 *
 * `.data` aligned(32) so `-SetGlobals gNdsR2AnimWarmStep=1` restores the old
 * cadence at IDENTICAL placement -- and a route A/B is legitimate here in a way
 * it is not for a gameplay change, because a hit and a miss load the same bytes
 * and differ only in where they came from. The cache's own contract says so:
 * every failure path degrades to the uncached load, a performance outcome and
 * never a correctness one. */
volatile u32 gNdsR2AnimWarmStep
    __attribute__((section(".data"), aligned(32))) = 4u;

void ndsR2AnimCachePreloadStep(void)
{
    /* Cycle 85 SWRM. The one asset load inside SRC, and therefore the honest
     * load signal the owner's loading-state exclusion actually wants -- the
     * banked rule thresholds on SRC itself, which is circular for SRC (board,
     * cycle 81). The guard is inverted rather than early-returning so the
     * bracket has a single exit and the exhausted-cursor frames, which are most
     * of the match, are still charged their validate-and-compare cost. */
#if NDS_TICK_HUD
    u32 warm_start = cpuGetTiming();
#endif

    {
        /* Read the route ONCE. Reading it per iteration would put a volatile
         * load inside the loop being measured, the same reason
         * gcPlayDObjAnimJoint hoists its own. Clamped so a poke of 0 cannot
         * stall the walk forever and leave the match streaming silently. */
        u32 step = gNdsR2AnimWarmStep;
        u32 i;

        if (step == 0u)
        {
            step = 1u;
        }
        ndsR2AnimCacheValidateGeneration();
        /* The pack first, and ALONE while it is streaming. Two reasons: the
         * arena is a bump allocator, so the blob must own the low bytes before
         * any warm payload takes them; and this update's BGM-seam budget is
         * already spent by a 16 KB chunk. `step = 0` for that update. */
        if (ndsBattlePackResidencyStep() != FALSE)
        {
            step = 0u;
        }
        for (i = 0u; i < step; i++)
        {
            if (sNdsR204AnimWarmCursor >= (sizeof(sNdsR204AnimWarmList) /
                                           sizeof(sNdsR204AnimWarmList[0])))
            {
                break;
            }
            ndsR2AnimWarmLoadOne(
                sNdsR204AnimWarmList[sNdsR204AnimWarmCursor++]);
        }
    }
#if NDS_TICK_HUD
    gNdsTickHudSrcAnimWarmTicks += cpuGetTiming() - warm_start;
#endif
}
#endif

static void *ndsRelocForceLoadFighterAObj16File(u32 token, u32 asset_id,
                                                void *heap)
{
    NDSRelocAssetHeader header;
    NDSRelocLoadedFile *loaded;
    size_t asset_size;
    void *packed;

    if ((heap == NULL) ||
        (ndsRelocIsMarioFoxAnimID(asset_id) == FALSE))
    {
        return NULL;
    }

#if NDS_R2_ANIM_CACHE
    /* Settle arena ownership BEFORE the pack lookup, not only before the raw
     * cache find further down. The pack's bytes live in that arena, and the
     * pack is consulted ahead of every other guard in this function -- so a
     * scene rewind that had not yet been noticed would otherwise return a
     * figatree pointing into memory the new scene already owns. Invalidating at
     * the seam where the invariant breaks, rather than at each reader, is the
     * shape the gMPCollisionGeometry fix settled on. */
    ndsR2AnimCacheValidateGeneration();
#endif
    packed = ndsBattlePackFindFigatree(asset_id);

    /* SLICE 1 PHASE 5 -- the acquisition path, deleted.
     *
     * Everything below this block exists to reconstruct an N64 loaded-file
     * image at `heap` on every action change. For a resident clip none of it
     * is needed: the bytes are already in the parser's own format, at a fixed
     * address, and `ftmain.c:4623` now takes this function's return value
     * (decomp patch, 2026-08-15) instead of hardcoding `figatree_heap`.
     *
     * The three status writes stay. They are a table store each, they are what
     * `lbRelocGetStatusBufferFile` answers from, and pointing them at the pack
     * keeps every existing reader correct. What goes is the memcpy, the alias
     * strip, the loaded-file registration and the whole finalize chain --
     * internal fixups, AObj16 normalization, attribute/weapon normalization,
     * external fixups and the sprite pass. */
    if (packed != NULL)
    {
        ndsRelocSetStatusBufferFile(token, packed);
        ndsRelocSetStatusBufferFile(asset_id, packed);
        ndsRelocSetForceStatusBufferFile(token, packed);
        gNdsBattlePackHits++;
        return packed;
    }
    gNdsBattlePackMisses++;

    /* Existence check without I/O. This used to be ndsRelocAssetAllocSize,
     * which answers it by opening the file and parsing the header -- a full
     * NitroFS directory walk for a question the asset table already answers,
     * and for a size the load below reports anyway (Task 76).
     * ndsRelocAssetGetPath is a table lookup. */
    if (ndsRelocAssetGetPath(asset_id) == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    asset_size = 0u;
#if NDS_TICK_HUD
    /* Observation only -- the load below is unchanged. "Force" may well mean
     * the caller wants pristine data restored, and the renderer does mutate
     * loaded fighter data, so the reload is not assumed redundant. This just
     * measures how often it reloads an asset the destination already holds. */
    NDS_TICK_HUD_NATIVE_OWNER_MARK(
        nNDSTickHudNativeOwnerFallbackAnimForceLoad);
    {
        NDSRelocLoadedFile *resident = ndsRelocFindLoadedFileByData(heap);

        if ((resident != NULL) && (resident->asset_id == asset_id))
        {
            NDS_TICK_HUD_NATIVE_OWNER_MARK(
                nNDSTickHudNativeOwnerFallbackAnimForceResident);
        }
    }
    /* R2-04 E0. AnimForceResident reads 0 because it asks whether the
     * DESTINATION heap already holds the asset, and the destination is
     * caller-owned and reused, so it almost never does. That refutes a
     * destination-side residency check, not the whole opportunity: what decides
     * whether an asset-keyed cache is worth building is how often the SAME
     * animation is force-loaded more than once over a match. A 128-frame window
     * is ~4 seconds and cannot show it; Mario returns to Wait/Walk/Jump for the
     * whole minute.
     *
     * Bitmap over the 301 Mario+Fox animation IDs: total loads, distinct assets,
     * repeats. repeats/total is exactly the fraction a cache would remove, and
     * distinct sizes the cache. Lab counters, tick-HUD builds only. */
    {
        u32 anim_index = asset_id - NDS_RELOC_ASSET_MARIO_ANIM_WAIT;

        gNdsR204AnimForceLoadTotal++;
        if (anim_index < NDS_R204_ANIM_ID_SPAN)
        {
            u32 word = anim_index >> 5;
            u32 mask = 1u << (anim_index & 31u);

            if ((gNdsR204AnimSeen[word] & mask) != 0u)
            {
                gNdsR204AnimForceLoadRepeat++;
            }
            else
            {
                gNdsR204AnimSeen[word] |= mask;
                gNdsR204AnimForceLoadDistinct++;
            }
        }
    }
#endif
#if NDS_R2_ANIM_CACHE
    /* R2-04 E1. A hit replaces the NitroFS walk, the cartridge read and the word
     * byte-swap with one copy. The fixups below still run against this heap,
     * because they write absolute pointers into it. */
    {
        const NDSR2AnimCacheEntry *cached = ndsR2AnimCacheFind(asset_id);

        if (cached != NULL)
        {
            memcpy(heap, cached->payload, cached->size);
            asset_size = cached->size;
            header = cached->header;
            ndsRelocRemoveFighterAObj16LoadedAliases(asset_id, heap);
            loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
#if NDS_R2_AOBJ16_PREBAKE
            /* R2-06 E13. The script region of this copy already carries the
             * transform, applied once at warm time against the arena base --
             * position-independent, so byte-identical to what the pass would
             * write here. Claiming it is what removes the pass from the frame.
             * The flag is per entry: anything the prebake declined still runs
             * the pass below, unchanged. */
            if ((loaded != NULL) && (cached->aobj16_ready != 0u))
            {
                loaded->format_fixups_applied = TRUE;
                gNdsR2AObj16PrebakeSkips++;
            }
#endif
            if ((loaded == NULL) ||
                (ndsRelocFinalizeLoadedFile(loaded) == FALSE))
            {
                goto fail;
            }
            gNdsR2AnimCacheHits++;
            ndsFighterManagerRecordExternToken(token, heap);
            ndsRelocSetStatusBufferFile(token, heap);
            ndsRelocSetStatusBufferFile(asset_id, heap);
            ndsRelocSetForceStatusBufferFile(token, heap);
            return heap;
        }
        gNdsR2AnimCacheMisses++;
    }
#endif
    /* One open for the size, the zero and the payload. Task 72 collapsed the
     * probe-then-load pair here; the sizing call above was the third walk of
     * the same directory for the same header, and this is the on-demand
     * fighter-animation load Task 71 caught running inside the frame that needs
     * the move. The callee zeroes heap over the aligned payload size before
     * reading, which is the memset that used to stand here, and reports that
     * size back for the failure path below. */
    if (ndsRelocAssetLoadIntoZeroedHeap(asset_id, heap, NDS_RELOC_ALIGN_BYTES,
                                        &asset_size, &header) == FALSE)
    {
        goto fail;
    }

    ndsRelocRemoveFighterAObj16LoadedAliases(asset_id, heap);
    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
    if (loaded == NULL)
    {
        goto fail;
    }
    if (ndsRelocApplyWordByteSwap(loaded) == FALSE)
    {
        goto fail;
    }
#if NDS_R2_ANIM_CACHE
    /* Snapshot here: swapped, not yet fixed up. One instruction later the
     * fixups make the image position-dependent and unusable as a template. */
    ndsR2AnimCacheStore(asset_id, heap, (u32)asset_size, &header);
#endif
    if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
    {
        goto fail;
    }

    ndsFighterManagerRecordExternToken(token, heap);
    ndsRelocSetStatusBufferFile(token, heap);
    ndsRelocSetStatusBufferFile(asset_id, heap);
    ndsRelocSetForceStatusBufferFile(token, heap);
    return heap;

fail:
    memset(heap, 0, asset_size);
    ndsRelocRecordExternalFixupFail(asset_id);
    return NULL;
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
        (ndsRelocIsMarioFoxAnimID(asset_id) != FALSE))
    {
        file = ndsRelocForceLoadFighterAObj16File(token, asset_id, heap);
#if NDS_FIGHTER_ANIM_AUDIT
        gNdsFighterAnimAuditLoadSerial++;
        gNdsFighterAnimAuditLoadAssetID = asset_id;
        gNdsFighterAnimAuditLoadResolved = (file != NULL) ? 1u : 0u;
        gNdsFighterAnimAuditLoadFallback = (file == NULL) ? 1u : 0u;
#endif
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

    /* THE SHIELD FREEZE. This is the on-demand status-animation load, reached
     * from a gameplay frame whenever a fighter enters a status whose asset is
     * not resident -- and a shield hit drives rebound into damage-fall, which
     * is one of the commonest routes there. syTaskmanMalloc does not fail, it
     * HANGS (syMallocSet's `while (TRUE);`), so once the general heap is full
     * this call never returns and the match freezes with no error. The NULL
     * test under it could never fire.
     *
     * The 2026-07-29 freeze root-cause fixed ndsR2AnimCacheStore by giving the
     * cache its own arena, but the loader underneath it kept allocating from
     * the shared heap, which is why the class came back as "hitting Fox's
     * shield freezes sometimes". Declining costs a missing animation for one
     * status, which the caller already handles; hanging costs the match. */
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap, asset_size, 0x10) == FALSE)
    {
        gNdsRelocHeapDeclineCount++;
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
                            if (asset_id == NDS_RELOC_ASSET_MN_VS_RESULTS)
                            {
                                ndsRelocNormalizeVSResultsSprites(loaded);
                            }
                            if (asset_id ==
                                NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM)
                            {
                                ndsRelocNormalizeTitleFireSprites(loaded);
                            }
                            ndsRelocNormalizeGroundMapAsset(loaded);
                            ndsRelocNormalizeStageDreamLandSprite(loaded);
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

/* Byte span of an ALREADY-LOADED reloc file, or 0 if it is not resident.
 *
 * lbRelocGetFileSize cannot answer this and must not be used for it. It sizes
 * an allocation that has yet to be made, so ndsRelocExternTreeAllocSize
 * deliberately returns 0 once the asset has a status node -- a resident file
 * needs no further heap -- and lbRelocGetFileSize then falls back to
 * sizeof(Sprite). Asking it about a loaded file therefore reports 68 bytes for
 * a file that is resident and perfectly intact, which on 2026-08-01 read as
 * "EFCommonEffects1/2/3 are not in the ROM" when all three were loaded and
 * EFCommonEffects2 was 28,432 bytes on disk. Anything bounds-checking an
 * offset against a live file wants this function. */
size_t ndsRelocGetLoadedFileSize(const void *file_id)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
    NDSRelocLoadedFile *loaded = ndsRelocFindLoadedFileByAsset(asset_id);

    return (loaded != NULL) ? (size_t)loaded->data_size : 0u;
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
