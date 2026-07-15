#ifndef SSB64_NDS_MPPROCESS_SOURCE_H
#define SSB64_NDS_MPPROCESS_SOURCE_H

/*
 * Renamed source API for the complete BattleShip mp/mpprocess.c import.
 *
 * The private compile checkpoint and the reversible live bridge share this
 * exact source-facing ABI. Public live names remain declared by mp/map.h.
 */
#include <stddef.h>
#include <stdint.h>

#include <mp/map.h>

/* Dependencies used by the renamed whole-TU source import. */
extern u16 gMPCollisionUpdateTic;

sb32 mpCollisionCheckLWallLineCollisionDiff(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle);
sb32 mpCollisionCheckRWallLineCollisionDiff(Vec3f *position,
                                            Vec3f *translate,
                                            Vec3f *ga_last,
                                            s32 *stand_line_id,
                                            u32 *stand_coll_flags,
                                            Vec3f *angle);
sb32 mpCollisionCheckExistLineID(s32 line_id);
sb32 mpCollisionCheckProjectFloor(Vec3f *position, s32 *project_line_id,
                                  f32 *ga_dist, u32 *stand_coll_flags,
                                  Vec3f *angle);
sb32 mpCollisionGetFCCommonFloor(s32 line_id, Vec3f *object_pos,
                                 f32 *floor_dist, u32 *floor_flags,
                                 Vec3f *angle);
void mpCollisionGetFloorEdgeL(s32 line_id, Vec3f *object_pos);
void mpCollisionGetFloorEdgeR(s32 line_id, Vec3f *object_pos);
s32 mpCollisionGetLineTypeID(s32 line_id);
s32 mpCollisionGetVertexCountLineID(s32 line_id);
void mpCollisionGetVertexPositionID(s32 line_id, s32 vertex_id,
                                    Vec3f *object_pos);
s32 mpCollisionGetEdgeUnderLLineID(s32 line_id);
s32 mpCollisionGetEdgeUnderRLineID(s32 line_id);
void mpCollisionGetLWallEdgeD(s32 line_id, Vec3f *object_pos);
void mpCollisionGetRWallEdgeD(s32 line_id, Vec3f *object_pos);

/* Source state read by the retained multiwall diagnostic in LIVE builds. */
extern s32 sNdsBaseMPProcessMultiWallCollidesNum;

#if UINTPTR_MAX == 0xffffffffu
#define NDS_MPPROCESS_ABI_OFFSET(field, expected)                         \
    _Static_assert(offsetof(MPCollData, field) == (expected),             \
                   "MPCollData." #field " ABI offset changed")

_Static_assert(sizeof(MPObjectColl) == 0x10u,
               "MPObjectColl target ABI size changed");
_Static_assert(offsetof(MPObjectColl, top) == 0x00u,
               "MPObjectColl.top ABI offset changed");
_Static_assert(offsetof(MPObjectColl, center) == 0x04u,
               "MPObjectColl.center ABI offset changed");
_Static_assert(offsetof(MPObjectColl, bottom) == 0x08u,
               "MPObjectColl.bottom ABI offset changed");
_Static_assert(offsetof(MPObjectColl, width) == 0x0cu,
               "MPObjectColl.width ABI offset changed");

_Static_assert(sizeof(MPCollData) == 0xd0u,
               "MPCollData target ABI size changed");
NDS_MPPROCESS_ABI_OFFSET(p_translate, 0x00u);
NDS_MPPROCESS_ABI_OFFSET(p_lr, 0x04u);
NDS_MPPROCESS_ABI_OFFSET(pos_prev, 0x08u);
NDS_MPPROCESS_ABI_OFFSET(pos_diff, 0x14u);
NDS_MPPROCESS_ABI_OFFSET(vel_speed, 0x20u);
NDS_MPPROCESS_ABI_OFFSET(vel_push, 0x2cu);
NDS_MPPROCESS_ABI_OFFSET(map_coll, 0x38u);
NDS_MPPROCESS_ABI_OFFSET(p_map_coll, 0x48u);
NDS_MPPROCESS_ABI_OFFSET(cliffcatch_coll, 0x4cu);
NDS_MPPROCESS_ABI_OFFSET(mask_prev, 0x54u);
NDS_MPPROCESS_ABI_OFFSET(mask_curr, 0x56u);
NDS_MPPROCESS_ABI_OFFSET(mask_unk, 0x58u);
NDS_MPPROCESS_ABI_OFFSET(mask_stat, 0x5au);
NDS_MPPROCESS_ABI_OFFSET(update_tic, 0x5cu);
NDS_MPPROCESS_ABI_OFFSET(ewall_line_id, 0x60u);
NDS_MPPROCESS_ABI_OFFSET(is_coll_end, 0x64u);
NDS_MPPROCESS_ABI_OFFSET(line_coll_dist, 0x68u);
NDS_MPPROCESS_ABI_OFFSET(floor_line_id, 0x74u);
NDS_MPPROCESS_ABI_OFFSET(floor_dist, 0x78u);
NDS_MPPROCESS_ABI_OFFSET(floor_flags, 0x7cu);
NDS_MPPROCESS_ABI_OFFSET(floor_angle, 0x80u);
NDS_MPPROCESS_ABI_OFFSET(ceil_line_id, 0x8cu);
NDS_MPPROCESS_ABI_OFFSET(ceil_flags, 0x90u);
NDS_MPPROCESS_ABI_OFFSET(ceil_angle, 0x94u);
NDS_MPPROCESS_ABI_OFFSET(lwall_line_id, 0xa0u);
NDS_MPPROCESS_ABI_OFFSET(lwall_flags, 0xa4u);
NDS_MPPROCESS_ABI_OFFSET(lwall_angle, 0xa8u);
NDS_MPPROCESS_ABI_OFFSET(rwall_line_id, 0xb4u);
NDS_MPPROCESS_ABI_OFFSET(rwall_flags, 0xb8u);
NDS_MPPROCESS_ABI_OFFSET(rwall_angle, 0xbcu);
NDS_MPPROCESS_ABI_OFFSET(cliff_id, 0xc8u);
NDS_MPPROCESS_ABI_OFFSET(ignore_line_id, 0xccu);

#undef NDS_MPPROCESS_ABI_OFFSET
#endif

void ndsBaseMPProcessResetMultiWallCount(void);
void ndsBaseMPProcessSetMultiWallLineID(s32 line_id);
void ndsBaseMPProcessSetLastWallCollideLeft(void);
void ndsBaseMPProcessSetLastWallCollideRight(void);
void ndsBaseMPProcessSetLastWallCollideStats(f32 pos, s32 line_id,
                                             u32 flags, Vec3f *angle);
void ndsBaseMPProcessGetLastWallCollideStats(f32 *pos, s32 *line_id,
                                             u32 *flags, Vec3f *angle);
sb32 ndsBaseMPProcessCheckCeilEdgeCollisionL(MPCollData *coll_data);
void ndsBaseMPProcessCeilEdgeAdjustLeft(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckCeilEdgeCollisionR(MPCollData *coll_data);
void ndsBaseMPProcessCeilEdgeAdjustRight(MPCollData *coll_data);
void ndsBaseMPProcessRunCeilEdgeAdjust(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckFloorEdgeCollisionL(MPCollData *coll_data);
void ndsBaseMPProcessFloorEdgeLAdjust(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckFloorEdgeCollisionR(MPCollData *coll_data);
void ndsBaseMPProcessFloorEdgeRAdjust(MPCollData *coll_data);
void ndsBaseMPProcessRunFloorEdgeAdjust(MPCollData *coll_data);
void ndsBaseMPProcessSetCollProjectFloorID(MPCollData *coll_data);
sb32 ndsBaseMPProcessUpdateMain(
    MPCollData *coll_data,
    sb32 (*proc_coll)(MPCollData *, GObj *, u32),
    GObj *gobj,
    u32 flags);
sb32 ndsBaseMPProcessCheckTestLWallCollision(MPCollData *coll_data);
void ndsBaseMPProcessRunLWallCollision(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestRWallCollision(MPCollData *coll_data);
void ndsBaseMPProcessRunRWallCollision(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestFloorCollisionNew(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestFloorCollision(MPCollData *coll_data,
                                              s32 line_id);
sb32 ndsBaseMPProcessCheckTestLCliffCollision(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestRCliffCollision(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestLWallCollisionAdjNew(MPCollData *coll_data);
void ndsBaseMPProcessRunLWallCollisionAdjNew(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestRWallCollisionAdjNew(MPCollData *coll_data);
void ndsBaseMPProcessRunRWallCollisionAdjNew(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestCeilCollisionAdjNew(MPCollData *coll_data);
void ndsBaseMPProcessRunCeilCollisionAdjNew(MPCollData *coll_data);
sb32 ndsBaseMPProcessCheckTestFloorCollisionAdjNew(
    MPCollData *coll_data, sb32 (*proc_map)(GObj *), GObj *gobj);
sb32 ndsBaseMPProcessRunFloorCollisionAdjNewNULL(MPCollData *coll_data);
void ndsBaseMPProcessSetLandingFloor(MPCollData *coll_data);
void ndsBaseMPProcessSetCollideFloor(MPCollData *coll_data);

#endif
