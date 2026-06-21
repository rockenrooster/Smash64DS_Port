#ifndef SSB64_NDS_GR_GROUND_H
#define SSB64_NDS_GR_GROUND_H

#include <PR/ultratypes.h>
#include <ssb_types.h>
#include <sys/objtypes.h>

#ifndef GMCOMMON_PLAYERS_MAX
#define GMCOMMON_PLAYERS_MAX 4
#endif

/*
 * Narrow ground shadow for imported movie scenes. The full BattleShip ground
 * headers bring in stage/map/fighter dependencies that are outside the current
 * opening-movie boundary.
 */

enum {
    nMPMapObjKindMoviePlayer1 = 0
};

typedef struct GRFileInfo
{
    uintptr_t *file_id;
    uintptr_t *offset;
} GRFileInfo;

typedef struct MPGeometryData MPGeometryData;
typedef struct MPItemWeights MPItemWeights;

typedef struct MPGroundDesc
{
    DObjDesc *dobjdesc;
    AObjEvent32 **anim_joints;
    MObjSub ***p_mobjsubs;
    AObjEvent32 ***p_matanim_joints;
} MPGroundDesc;

typedef struct MPGroundData
{
    MPGroundDesc gr_desc[4];
    MPGeometryData *map_geometry;
    u8 layer_mask;
    Sprite *wallpaper;
    SYColorRGB fog_color;
    u8 fog_alpha;
    SYColorRGB emblem_colors[GMCOMMON_PLAYERS_MAX];
    s32 unused;
    Vec3f light_angle;
    s16 camera_bound_top;
    s16 camera_bound_bottom;
    s16 camera_bound_right;
    s16 camera_bound_left;
    s16 map_bound_top;
    s16 map_bound_bottom;
    s16 map_bound_right;
    s16 map_bound_left;
    u32 bgm_id;
    void *map_nodes;
    MPItemWeights *item_weights;
    s16 alt_warning;
    s16 camera_bound_team_top;
    s16 camera_bound_team_bottom;
    s16 camera_bound_team_right;
    s16 camera_bound_team_left;
    s16 map_bound_team_top;
    s16 map_bound_team_bottom;
    s16 map_bound_team_right;
    s16 map_bound_team_left;
    Vec3h zoom_start;
    Vec3h zoom_end;
} MPGroundData;

extern GObj *gGRCommonLayerGObjs[4];
extern f32 gMPCollisionLightAngleX;
extern f32 gMPCollisionLightAngleY;

void grWallpaperMakeDecideKind(void);
void grCommonSetupInitAll(void);
s32 mpCollisionGetMapObjCountKind(s32 kind);
void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids);
void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos);
void mpCollisionGetPlayerMapObjPosition(s32 player, Vec3f *pos);
void mpCollisionInitGroundData(void);
void mpCollisionSetPlayBGM(void);

#endif
