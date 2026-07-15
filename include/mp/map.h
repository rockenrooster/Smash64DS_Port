#ifndef SSB64_NDS_MP_MAP_H
#define SSB64_NDS_MP_MAP_H

/*
 * Narrow map compatibility header for imported ground setup slices.
 *
 * The original BattleShip mp/map.h pulls in the full map, item, fighter, and
 * collision ABI. That is broader than the current bounded VSBattle ground
 * setup proof, and conflicts with the project-owned compatibility headers.
 */
#include <gr/ground.h>

typedef struct MPLineGroup {
    u16 line_count;
    u16 *line_id;
} MPLineGroup;

extern MPLineGroup gMPCollisionLineGroups[nMPLineKindEnumCount];
void ndsMPCollisionInvalidateTopology(void);

#ifndef SSB64_NDS_MPOBJECTCOLL_DECLARED
#define SSB64_NDS_MPOBJECTCOLL_DECLARED
typedef struct MPObjectColl {
    f32 top;
    f32 center;
    f32 bottom;
    f32 width;
} MPObjectColl;
#endif

#ifndef SSB64_NDS_MPCOLLDATA_DECLARED
#define SSB64_NDS_MPCOLLDATA_DECLARED
typedef struct MPCollData {
    Vec3f *p_translate;
    s32 *p_lr;
    Vec3f pos_prev;
    Vec3f pos_diff;
    Vec3f vel_speed;
    Vec3f vel_push;
    MPObjectColl map_coll;
    MPObjectColl *p_map_coll;
    Vec2f cliffcatch_coll;
    u16 mask_prev;
    u16 mask_curr;
    u16 mask_unk;
    u16 mask_stat;
    u16 update_tic;
    s32 ewall_line_id;
    sb32 is_coll_end;
    Vec3f line_coll_dist;
    s32 floor_line_id;
    f32 floor_dist;
    u32 floor_flags;
    Vec3f floor_angle;
    s32 ceil_line_id;
    u32 ceil_flags;
    Vec3f ceil_angle;
    s32 lwall_line_id;
    u32 lwall_flags;
    Vec3f lwall_angle;
    s32 rwall_line_id;
    u32 rwall_flags;
    Vec3f rwall_angle;
    s32 cliff_id;
    s32 ignore_line_id;
} MPCollData;
#endif

#define MAP_PROC_TYPE_DEFAULT 0
#define MAP_PROC_TYPE_CLIFF (1 << 0)
#define MAP_PROC_TYPE_CLIFFEDGE (1 << 0)
#define MAP_PROC_TYPE_PROJECT (1 << 1)
#define MAP_PROC_TYPE_STOPEDGE (1 << 1)
#define MAP_PROC_TYPE_PASS (1 << 2)
#define MAP_PROC_TYPE_CEILHEAVY (1 << 3)

#ifndef MAP_VERTEX_COLL_PASS
#define MAP_VERTEX_COLL_PASS 0x00004000u
#endif

#ifndef MAP_FLAG_FLOOR
#define MAP_FLAG_FLOOR 0x00000800u
#endif
#ifndef MAP_FLAG_CEIL
#define MAP_FLAG_CEIL 0x00000400u
#endif
#ifndef MAP_FLAG_RWALL
#define MAP_FLAG_RWALL 0x00000020u
#endif
#ifndef MAP_FLAG_LWALL
#define MAP_FLAG_LWALL 0x00000001u
#endif
#ifndef MAP_FLAG_MAIN_MASK
#define MAP_FLAG_MAIN_MASK \
    (MAP_FLAG_FLOOR | MAP_FLAG_CEIL | MAP_FLAG_RWALL | MAP_FLAG_LWALL)
#endif

void mpProcessSetCollProjectFloorID(MPCollData *coll_data);
void mpProcessResetMultiWallCount(void);
void mpProcessSetMultiWallLineID(s32 line_id);
sb32 mpProcessCheckTestLWallCollision(MPCollData *coll_data);
void mpProcessRunLWallCollision(MPCollData *coll_data);
sb32 mpProcessCheckTestRWallCollision(MPCollData *coll_data);
void mpProcessRunRWallCollision(MPCollData *coll_data);
sb32 mpProcessCheckTestFloorCollisionNew(MPCollData *coll_data);
sb32 mpProcessCheckTestFloorCollision(MPCollData *coll_data, s32 line_id);
sb32 mpProcessCheckTestLCliffCollision(MPCollData *coll_data);
sb32 mpProcessCheckTestRCliffCollision(MPCollData *coll_data);
sb32 mpProcessCheckTestLWallCollisionAdjNew(MPCollData *coll_data);
void mpProcessRunLWallCollisionAdjNew(MPCollData *coll_data);
sb32 mpProcessCheckTestRWallCollisionAdjNew(MPCollData *coll_data);
void mpProcessRunRWallCollisionAdjNew(MPCollData *coll_data);
void mpProcessSetLandingFloor(MPCollData *coll_data);
void mpProcessSetCollideFloor(MPCollData *coll_data);
sb32 mpProcessCheckTestCeilCollisionAdjNew(MPCollData *coll_data);
void mpProcessRunCeilCollisionAdjNew(MPCollData *coll_data);
void mpProcessRunCeilEdgeAdjust(MPCollData *coll_data);
sb32 mpProcessCheckTestFloorCollisionAdjNew(MPCollData *coll_data,
                                             sb32 (*proc_map)(GObj *),
                                             GObj *gobj);
sb32 mpProcessRunFloorCollisionAdjNewNULL(MPCollData *coll_data);
sb32 mpProcessCheckFloorEdgeCollisionL(MPCollData *coll_data);
void mpProcessFloorEdgeLAdjust(MPCollData *coll_data);
sb32 mpProcessCheckFloorEdgeCollisionR(MPCollData *coll_data);
void mpProcessFloorEdgeRAdjust(MPCollData *coll_data);
void mpProcessRunFloorEdgeAdjust(MPCollData *coll_data);
sb32 mpCollisionCheckFloorLineCollisionSame(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle);
sb32 mpCollisionCheckFloorLineCollisionDiff(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle);
sb32 mpCollisionCheckCeilLineCollisionSame(Vec3f *position,
                                           Vec3f *translate,
                                           Vec3f *ga_last,
                                           s32 *stand_line_id,
                                           u32 *stand_coll_flags,
                                           Vec3f *angle);
sb32 mpCollisionCheckCeilLineCollisionDiff(Vec3f *position,
                                           Vec3f *translate,
                                           Vec3f *ga_last,
                                           s32 *stand_line_id,
                                           u32 *stand_coll_flags,
                                           Vec3f *angle);
sb32 mpCollisionCheckLWallLineCollisionSame(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle);
sb32 mpCollisionCheckRWallLineCollisionSame(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle);
sb32 mpCollisionGetLRCommonLWall(s32 line_id, Vec3f *object_pos,
                                 f32 *dist, u32 *flags, Vec3f *angle);
sb32 mpCollisionGetLRCommonRWall(s32 line_id, Vec3f *object_pos,
                                 f32 *dist, u32 *flags, Vec3f *angle);
sb32 mpCollisionGetFCCommonCeil(s32 line_id, Vec3f *object_pos,
                                f32 *ceil_dist, u32 *ceil_flags,
                                Vec3f *angle);
void mpCollisionGetCeilEdgeL(s32 line_id, Vec3f *object_pos);
void mpCollisionGetCeilEdgeR(s32 line_id, Vec3f *object_pos);
s32 mpCollisionGetEdgeUpperLLineID(s32 line_id);
s32 mpCollisionGetEdgeUpperRLineID(s32 line_id);
s32 mpCollisionGetEdgeLeftULineID(s32 line_id);
s32 mpCollisionGetEdgeLeftDLineID(s32 line_id);
s32 mpCollisionGetEdgeRightULineID(s32 line_id);
s32 mpCollisionGetEdgeRightDLineID(s32 line_id);
void mpCollisionGetLWallEdgeU(s32 line_id, Vec3f *object_pos);
void mpCollisionGetRWallEdgeU(s32 line_id, Vec3f *object_pos);
sb32 mpProcessUpdateMain(MPCollData *coll_data,
                         sb32 (*proc_coll)(MPCollData*, GObj*, u32),
                         GObj *gobj,
                         u32 flags);
sb32 mpCommonRunFighterAllCollisions(MPCollData *coll_data,
                                      GObj *fighter_gobj,
                                      u32 flags);
void mpCommonRunDefaultCollision(MPCollData *coll_data, GObj *gobj,
                                 u32 flags);
void mpCommonCopyCollDataStats(MPCollData *this_coll_data, Vec3f *pos,
                               MPCollData *other_coll_data);
void mpCommonResetCollDataStats(MPCollData *coll_data);
void mpCommonRunWeaponCollisionDefault(GObj *weapon_gobj, Vec3f *pos,
                                       MPCollData *coll_data);
u16 mpCollisionGetVertexFlagsLineID(s32 line_id);
sb32 mpCollisionCheckExistPlatformLineID(s32 line_id);
sb32 mpCommonCheckFighterOnFloor(GObj *fighter_gobj);
sb32 mpCommonCheckFighterOnCliffEdge(GObj *fighter_gobj);
sb32 mpCommonCheckFighterCeilHeavyCliff(GObj *fighter_gobj);

#endif
