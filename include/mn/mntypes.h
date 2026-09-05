#ifndef SSB64_NDS_MNTYPES_H
#define SSB64_NDS_MNTYPES_H

#include <stdint.h>

#include <ssb_types.h>
#include <mn/mndef.h>
#include <sys/audio.h>
#include <sys/objtypes.h>

typedef struct MNTitleSpriteDesc
{
    Vec2i pos;
    intptr_t offset;
} MNTitleSpriteDesc;

typedef struct MNVSResultsScore
{
    s32 score;
    s32 place;
    s32 player;
} MNVSResultsScore;

typedef struct MNCongraPicture
{
    u32 bottom_file_id;
    intptr_t bottom_offset;
    u32 top_file_id;
    intptr_t top_offset;
} MNCongraPicture;

typedef struct MNCharactersMotion
{
    s32 status_id;
    s32 anim_length;
    u32 flags;
} MNCharactersMotion;

typedef struct MNCharactersSpecialMotion
{
    MNCharactersMotion motions[3][8];
} MNCharactersSpecialMotion;

typedef struct MNPlayersSlot1PGame
{
    GObj *cursor;
    GObj *puck;
    GObj *player;
    GObj *name_emblem_gobj;
    GObj *panel_doors;
    GObj *panel;
    GObj *flash;
    void *figatree_heap;
    s32 fkind;
    u32 costume;
    s32 cursor_status;
    sb32 is_selected;
    sb32 is_recalling;
    s32 recall_end_tic;
    f32 recall_start_x;
    f32 recall_end_x;
    f32 recall_start_y;
    f32 recall_mid_y;
    f32 recall_end_y;
    s32 recall_tics;
    s32 holder_player;
    s32 held_player;
    sb32 is_fighter_selected;
    sb32 is_status_selected;
    f32 puck_vel_x;
    f32 puck_vel_y;
    f32 cursor_pickup_x;
    f32 cursor_pickup_y;
    sb32 is_cursor_adjusting;
    alSoundEffect *p_sfx;
    u16 sfx_id;
    sb32 is_hold_b;
    u32 unk_0xB4;
    s32 hold_b_tics;
} MNPlayersSlot1PGame;

typedef struct MNPlayersSlotTraining
{
    GObj *cursor;
    GObj *puck;
    GObj *player;
    GObj *type_button;
    GObj *name_emblem_gobj;
    GObj *panel_doors;
    GObj *panel;
    GObj *team_color_button;
    GObj *handicap_cpu_level;
    GObj *arrows;
    GObj *handicap_cpu_level_value;
    GObj *flash;
    GObj *type;
    void *figatree_heap;
    u32 cpu_level;
    u32 handicap;
    s32 team;
    u32 unk_0x44;
    s32 fkind;
    u32 costume;
    s32 cursor_status;
    sb32 is_selected;
    sb32 is_recalling;
    s32 recall_end_tic;
    f32 recall_start_x;
    f32 recall_end_x;
    f32 recall_start_y;
    f32 recall_mid_y;
    f32 recall_end_y;
    s32 recall_tics;
    s32 holder_player;
    s32 held_player;
    s32 pkind;
    sb32 is_fighter_selected;
    sb32 is_status_selected;
    f32 puck_vel_x;
    f32 puck_vel_y;
    f32 cursor_pickup_x;
    f32 cursor_pickup_y;
    sb32 is_cursor_adjusting;
    s32 door_offset;
    alSoundEffect *p_sfx;
    u16 sfx_id;
    u16 unk_0xAE;
    sb32 is_hold_b;
    u32 unk_0xB4;
    s32 hold_b_tics;
} MNPlayersSlotTraining;

typedef struct MNPlayersSlotVS
{
    GObj *cursor;
    GObj *puck;
    GObj *player;
    GObj *type_button;
    GObj *name_emblem_gobj;
    GObj *panel_doors;
    GObj *panel;
    GObj *team_color_button;
    GObj *handicap_cpu_level;
    GObj *arrows;
    GObj *handicap_cpu_level_value;
    GObj *flash;
    GObj *type;
    void *figatree_heap;
    u32 cpu_level;
    u32 handicap;
    s32 team;
    u32 unk_0x44;
    s32 fkind;
    u32 costume;
    u32 shade;
    s32 cursor_status;
    sb32 is_selected;
    sb32 is_recalling;
    s32 recall_end_tic;
    f32 recall_start_x;
    f32 recall_end_x;
    f32 recall_start_y;
    f32 recall_mid_y;
    f32 recall_end_y;
    s32 recall_tics;
    s32 holder_player;
    s32 held_player;
    s32 pkind;
    sb32 is_fighter_selected;
    sb32 is_status_selected;
    f32 puck_vel_x;
    f32 puck_vel_y;
    f32 cursor_pickup_x;
    f32 cursor_pickup_y;
    sb32 is_cursor_adjusting;
    s32 door_offset;
    alSoundEffect *p_sfx;
    u16 sfx_id;
    sb32 is_hold_b;
    u32 unk_0xB4;
    s32 hold_b_tics;
} MNPlayersSlotVS;

typedef struct MNPlayersSlotBonus
{
    GObj *cursor;
    GObj *puck;
    GObj *player;
    GObj *name_emblem_gobj;
    GObj *panel_doors;
    GObj *panel;
    GObj *flash;
    void *figatree_heap;
    s32 fkind;
    u32 costume;
    s32 cursor_status;
    sb32 is_selected;
    sb32 is_recalling;
    s32 recall_end_tic;
    f32 recall_start_x;
    f32 recall_end_x;
    f32 recall_start_y;
    f32 recall_mid_y;
    f32 recall_end_y;
    s32 recall_tics;
    s32 holder_player;
    s32 held_player;
    sb32 is_fighter_selected;
    sb32 is_status_selected;
    f32 puck_vel_x;
    f32 puck_vel_y;
    f32 cursor_pickup_x;
    f32 cursor_pickup_y;
    sb32 is_cursor_adjusting;
    alSoundEffect *p_sfx;
    u16 sfx_id;
    sb32 is_hold_b;
    u32 unk_0x80;
    s32 hold_b_tics;
} MNPlayersSlotBonus;

#define IS_BETWEEN(val, min, max) \
(( ( (val) >= (min) ) && ( (val) <= (max) ) ) ? TRUE : FALSE)

#endif
