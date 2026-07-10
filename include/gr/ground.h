#ifndef SSB64_NDS_GR_GROUND_H
#define SSB64_NDS_GR_GROUND_H

#include <PR/ultratypes.h>
#include <ssb_types.h>
#include <sys/objtypes.h>

#ifndef GMCOMMON_PLAYERS_MAX
#define GMCOMMON_PLAYERS_MAX 4
#endif

#define GRPUPUPU_WHISPY_BLINK_WAIT_BASE 30
#define GRPUPUPU_WHISPY_BLINK_WAIT_RANDOM 270
#define GRPUPUPU_WHISPY_WAIT_DURATION_BASE 960
#define GRPUPUPU_WHISPY_WAIT_DURATION_RANDOM 1140
#define GRPUPUPU_WHISPY_WIND_DURATION_BASE 240
#define GRPUPUPU_WHISPY_WIND_DURATION_RANDOM 80
#define GRPUPUPU_WHISPY_WIND_RUMBLE_WAIT 18
#define GRPUPUPU_WHISPY_POS_X (-525.0F)
#define GRPUPUPU_WHISPY_WIND_VEL_BASE 6.0F
#define GRPUPUPU_WHISPY_WIND_DIST_DECAY 0.0006F
#define GRPUPUPU_WHISPY_WINDBOX_TOP 1000.0F
#define GRPUPUPU_WHISPY_WINDBOX_BOTTOM (-10.0F)
#define GRPUPUPU_WHISPY_WINDBOX_EDGELEFT (-2325.0F)
#define GRPUPUPU_WHISPY_WINDBOX_EDGERIGHT 2275.0F

/*
 * Narrow ground shadow for imported movie scenes. The full BattleShip ground
 * headers bring in stage/map/fighter dependencies that are outside the current
 * opening-movie boundary.
 */

typedef enum MPMapObjKind
{
    nMPMapObjKindBattlePlayerStart = 0,
    nMPMapObjKindBattlePlayer1 = nMPMapObjKindBattlePlayerStart,
    nMPMapObjKindBattlePlayer2,
    nMPMapObjKindBattlePlayer3,
    nMPMapObjKindBattlePlayer4,
    nMPMapObjKindBattlePlayerEnd = nMPMapObjKindBattlePlayer4,
    nMPMapObjKindItem,
    nMPMapObjKindScaleL,
    nMPMapObjKindScaleR,
    nMPMapObjKindPakkunL,
    nMPMapObjKindPakkunR,
    nMPMapObjKindPowerBlock,
    nMPMapObjKindDokanL,
    nMPMapObjKindDokanR,
    nMPMapObjKindAcid,
    nMPMapObjKindTwister,
    nMPMapObjKindMonster,
    nMPMapObjKindMonsterUnused1,
    nMPMapObjKindMonsterUnused2,
    nMPMapObjKindMonsterUnused3,
    nMPMapObjKindMonsterUnused4,
    nMPMapObjKindBumper,
    nMPMapObjKindDokanWall,
    nMPMapObjKindMoviePlayer1,
    nMPMapObjKindMoviePlayer2,
    nMPMapObjKindMoviePlayer3,
    nMPMapObjKindRebirth = 0x20
} MPMapObjKind;

typedef enum MPLineType
{
    nMPLineKindFloor,
    nMPLineKindCeil,
    nMPLineKindRWall,
    nMPLineKindLWall,
    nMPLineKindEnumCount
} MPLineType;

typedef enum MPYakumonoStatus
{
    nMPYakumonoStatusNone,
    nMPYakumonoStatusOn,
    nMPYakumonoStatusShow,
    nMPYakumonoStatusOff,
    nMPYakumonoStatusHidden
} MPYakumonoStatus;

#define NDS_MP_YAKUMONO_DOBJ_SLOTS 64

typedef struct GRFileInfo
{
    uintptr_t *file_id;
    uintptr_t *offset;
} GRFileInfo;

typedef struct MPVertexLinks
{
    u16 vertex1;
    u16 vertex2;
} MPVertexLinks;

typedef struct MPVertexArray
{
    u16 vertex_id[1];
} MPVertexArray;

typedef struct MPVertexData
{
    Vec2h pos;
    u16 vertex_flags;
} MPVertexData;

typedef struct MPVertexPosContainer
{
    MPVertexData vpos[1];
} MPVertexPosContainer;

typedef struct MPLineData
{
    u16 group_id;
    u16 line_count;
} MPLineData;

typedef struct MPLineInfo
{
    u16 yakumono_id;
    MPLineData line_data[4];
} MPLineInfo;

typedef struct MPGeometryData
{
    u16 yakumono_count;
    void *vertex_data;
    void *vertex_id;
    void *vertex_links;
    MPLineInfo *line_info;
    u16 mapobj_count;
    void *mapobjs;
} MPGeometryData;
typedef struct MPItemWeights MPItemWeights;
typedef struct MPMapObjData
{
    u16 mapobj_kind;
    Vec2h pos;
} MPMapObjData;

typedef struct MPMapObjContainer
{
    MPMapObjData mapobjs[1];
} MPMapObjContainer;

typedef struct MPYakumonoDObj
{
    DObj *dobjs[NDS_MP_YAKUMONO_DOBJ_SLOTS];
} MPYakumonoDObj;

typedef struct MPBounds
{
    f32 top;
    f32 bottom;
    f32 right;
    f32 left;
} MPBounds;

typedef struct MPAllBounds
{
    MPBounds start;
    MPBounds stop;
    MPBounds current;
    MPBounds diff;
} MPAllBounds;

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

typedef struct GRDisplayDesc
{
    void (*pri_proc_display)(GObj *);
    void (*sec_proc_display)(GObj *);
    u8 dl_link;
    void (*proc_update)(GObj *);
} GRDisplayDesc;

typedef struct GRAttackColl
{
    s32 kind;
    s32 damage;
    s32 angle;
    s32 knockback_scale;
    s32 knockback_weight;
    s32 knockback_base;
    s32 element;
} GRAttackColl;

typedef struct GRObstacle
{
    GObj *gobj;
    sb32 (*proc_update)(GObj *, GObj *, s32 *);
} GRObstacle;

typedef struct GRHazard
{
    GObj *gobj;
    sb32 (*proc_update)(GObj *, GObj *, GRAttackColl **, s32 *);
} GRHazard;

extern GRObstacle sFTMainGroundObstacles[2];
extern GRHazard sFTMainGroundHazards[1];
extern s32 sFTMainGroundObstaclesNum;
extern s32 sFTMainGroundHazardsNum;

typedef struct GRPupupuEffect
{
    Vec3f pos;
    f32 rotate;
} GRPupupuEffect;

#ifndef SSB64_NDS_LBTRANSFORM_DECLARED
#define SSB64_NDS_LBTRANSFORM_DECLARED
typedef struct LBTransform
{
    struct LBTransform *next;
    Vec3f translate;
    Vec3f rotate;
    Vec3f scale;
    u8 transform_status;
    u8 transform_id;
    u16 users_num;
    Mtx44f affine;
    Mtx44f projection;
    f32 pc0_magnitude;
    f32 pc1_magnitude;
    void (*proc_dead)(struct LBTransform *);
    u16 generator_id;
    GObj *effect_gobj;
} LBTransform;
#endif

typedef struct GRCommonGroundVarsPupupu
{
    void *map_head;
    GObj *map_gobj[4];
    s32 particle_bank_id;
    LBTransform *leaves_xf;
    LBTransform *dust_xf;
    u16 whispy_wind_wait;
    u16 whispy_wind_duration;
    s16 whispy_blink_wait;
    u8 whispy_status;
    u8 flowers_back_wait;
    u8 flowers_front_wait;
    u8 rumble_wait;
    s8 lr_players;
    u8 flowers_back_status;
    u8 flowers_front_status;
    s8 whispy_eyes_status;
    s8 whispy_mouth_status;
    s8 whispy_mouth_texture;
    s8 whispy_eyes_texture;
} GRCommonGroundVarsPupupu;

typedef struct GRInishieScale
{
    DObj *platform_dobj;
    DObj *string_dobj;
    f32 string_length;
    f32 platform_base_y;
} GRInishieScale;

typedef struct GRCommonGroundVarsInishie
{
    void *map_head;
    void *item_head;
    GRInishieScale scale[2];
    f32 splat_alt;
    f32 splat_accelerate;
    u16 splat_wait;
    u8 splat_status;
    u8 players_tt[4];
    ub8 players_ga[4];
} GRCommonGroundVarsInishie;

typedef union GRStruct
{
    GRCommonGroundVarsPupupu pupupu;
    GRCommonGroundVarsInishie inishie;
} GRStruct;

extern GObj *gGRCommonLayerGObjs[4];
extern GRStruct gGRCommonStruct;
extern MPGroundData *gMPCollisionGroundData;
extern MPGeometryData *gMPCollisionGeometry;
extern MPMapObjContainer *gMPCollisionMapObjs;
extern MPYakumonoDObj *gMPCollisionYakumonoDObjs;
extern Vec3f *gMPCollisionSpeeds;
extern s32 gMPCollisionYakumonosNum;
extern MPAllBounds gMPCollisionBounds;
extern f32 gMPCollisionLightAngleX;
extern f32 gMPCollisionLightAngleY;
extern u32 gMPCollisionBGMCurrent;
extern u32 gMPCollisionBGMDefault;

void grWallpaperMakeDecideKind(void);
void grWallpaperPausePerspUpdate(void);
void grWallpaperResumePerspUpdate(void);
void grWallpaperRunProcessAll(void);
void grWallpaperResumeProcessAll(void);
void grCommonSetupInitAll(void);
void ndsGRCompatibilityNonPupupuSetup(void);
void grDisplayLayer0PriProcDisplay(GObj *ground_gobj);
void grDisplayLayer0SecProcDisplay(GObj *ground_gobj);
void grDisplayLayer1PriProcDisplay(GObj *ground_gobj);
void grDisplayLayer1SecProcDisplay(GObj *ground_gobj);
void grDisplayLayer2PriProcDisplay(GObj *ground_gobj);
void grDisplayLayer2SecProcDisplay(GObj *ground_gobj);
void grDisplayLayer3PriProcDisplay(GObj *ground_gobj);
void grDisplayLayer3SecProcDisplay(GObj *ground_gobj);
void grDisplayDObjSetNoAnimXObj(GObj *ground_gobj, DObjDesc *dobjdesc);
GObj *grDisplayMakeGeometryLayer(MPGroundDesc *gr_desc, s32 gr_desc_id,
                                 DObj **dobjs);
void grModelSetupGroundDObjs(GObj *gobj, DObjDesc *dobjdesc, DObj **dobjs,
                             DObjTransformTypes *transform_types);
void grMainSetupMakeGround(void);
GObj *grPupupuMakeGround(void);
void grPupupuInitAll(void);
void grPupupuProcUpdate(GObj *ground_gobj);
void ndsGRPupupuRunSafeUpdateProbe(void);
void grInishieMakeScale(void);
void grInishieScaleProcUpdate(GObj *ground_gobj);
void *ndsGRInishieScaleGetSourceSetupMapHead(void);
void ndsGRInishieScaleMakeProofShell(void);
void mpCollisionClearYakumonoAll(void);
void mpCollisionInitYakumonoAll(void);
void mpCollisionUpdateBoundsCurrent(void);
void mpCollisionUpdateBoundsDiff(void);
void mpCollisionAdvanceUpdateTic(GObj *ground_gobj);
void mpCollisionPlayYakumonoAnim(GObj *ground_gobj);
void mpCollisionSetYakumonoPosID(s32 line_id, Vec3f *yakumono_pos);
void mpCollisionSetYakumonoOnID(s32 line_id);
void mpCollisionSetYakumonoOffID(s32 line_id);
s32 mpCollisionGetMapObjCountKind(s32 kind);
void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids);
void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos);
s32 mpCollisionSetDObjNoID(s32 line_id);
void mpCollisionGetPlayerMapObjPosition(s32 player, Vec3f *pos);
void mpCollisionInitGroundData(void);
sb32 mpCollisionCheckProjectFloor(Vec3f *pos, s32 *floor_line_id,
                                  f32 *floor_dist, u32 *floor_flags,
                                  Vec3f *floor_angle);
void mpCollisionGetFloorEdgeL(s32 line_id, Vec3f *object_pos);
void mpCollisionGetFloorEdgeR(s32 line_id, Vec3f *object_pos);
s32 mpCollisionGetLineTypeID(s32 line_id);
void mpCollisionGetVertexPositionID(s32 line_id, s32 vertex_id,
                                    Vec3f *object_pos);
sb32 mpCollisionGetFCCommonFloor(s32 line_id, Vec3f *object_pos,
                                 f32 *floor_dist, u32 *floor_flags,
                                 Vec3f *angle);
void mpCommonSetFighterProjectFloor(GObj *fighter_gobj);
s32 mpCollisionGetEdgeUnderLLineID(s32 line_id);
s32 mpCollisionGetEdgeUnderRLineID(s32 line_id);
sb32 mpCollisionCheckExistLineID(s32 line_id);
s32 mpCollisionGetVertexCountLineID(s32 line_id);
void mpCollisionGetSpeedLineID(s32 line_id, Vec3f *vel);
void mpCollisionSetPlayBGM(void);

#endif
