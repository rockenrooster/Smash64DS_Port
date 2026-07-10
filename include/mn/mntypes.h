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

#endif
