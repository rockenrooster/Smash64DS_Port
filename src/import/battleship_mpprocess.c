/*
 * Private whole-TU import of BattleShip mp/mpprocess.c.
 *
 * Every source-global and exported function is renamed so this object can be
 * compiled beside the current DS collision seam without changing live
 * gameplay. The Makefile compiles but does not link this object; separate
 * function/data sections make its completeness and provenance auditable until
 * mpcommon and its map callbacks graduate as one coherent slice.
 */
#include <nds/nds_mpprocess_source.h>

/* mpprocess.c compares one floor material but does not include ft/fighter.h. */
#define MAP_VERTEX_COLL_CLIFF 0x00008000u
#define MAP_FLAG_LCLIFF 0x00001000u
#define MAP_FLAG_RCLIFF 0x00002000u
#define MAP_FLAG_CEILHEAVY 0x00004000u
#define MAP_FLAG_FLOOREDGE 0x00008000u
#define MAP_FLAG_CLIFF_MASK (MAP_FLAG_LCLIFF | MAP_FLAG_RCLIFF)
#define MAP_VERTEX_MAT_MASK (~0x0000ff00u)
#define nMPMaterial4 4

_Static_assert(MAP_VERTEX_COLL_CLIFF == 0x00008000u,
               "BattleShip cliff vertex flag changed");
_Static_assert(MAP_FLAG_LCLIFF == 0x00001000u,
               "BattleShip left-cliff flag changed");
_Static_assert(MAP_FLAG_RCLIFF == 0x00002000u,
               "BattleShip right-cliff flag changed");
_Static_assert(MAP_FLAG_CEILHEAVY == 0x00004000u,
               "BattleShip heavy-ceiling flag changed");
_Static_assert(MAP_FLAG_FLOOREDGE == 0x00008000u,
               "BattleShip floor-edge flag changed");
_Static_assert(MAP_FLAG_CLIFF_MASK == 0x00003000u,
               "BattleShip cliff mask changed");
_Static_assert(MAP_VERTEX_MAT_MASK == ~0x0000ff00u,
               "BattleShip material mask changed");
_Static_assert(nMPMaterial4 == 4,
               "BattleShip material 4 value changed");

#define sMPProcessMultiWallCollidesNum \
    sNdsBaseMPProcessMultiWallCollidesNum
#define sMPProcessPad0x80130DE4 sNdsBaseMPProcessPad0x80130DE4
#define sMPProcessMultiWallCollideLineIDs \
    sNdsBaseMPProcessMultiWallCollideLineIDs
#define sMPProcessLastWallCollidePosition \
    sNdsBaseMPProcessLastWallCollidePosition
#define sMPProcessLastWallLineID sNdsBaseMPProcessLastWallLineID
#define sMPProcessLastWallFlags sNdsBaseMPProcessLastWallFlags
#define sMPProcessLastWallAngle sNdsBaseMPProcessLastWallAngle

#define mpProcessResetMultiWallCount ndsBaseMPProcessResetMultiWallCount
#define mpProcessSetMultiWallLineID ndsBaseMPProcessSetMultiWallLineID
#define mpProcessSetLastWallCollideLeft \
    ndsBaseMPProcessSetLastWallCollideLeft
#define mpProcessSetLastWallCollideRight \
    ndsBaseMPProcessSetLastWallCollideRight
#define mpProcessSetLastWallCollideStats \
    ndsBaseMPProcessSetLastWallCollideStats
#define mpProcessGetLastWallCollideStats \
    ndsBaseMPProcessGetLastWallCollideStats
#define mpProcessCheckCeilEdgeCollisionL \
    ndsBaseMPProcessCheckCeilEdgeCollisionL
#define mpProcessCeilEdgeAdjustLeft ndsBaseMPProcessCeilEdgeAdjustLeft
#define mpProcessCheckCeilEdgeCollisionR \
    ndsBaseMPProcessCheckCeilEdgeCollisionR
#define mpProcessCeilEdgeAdjustRight ndsBaseMPProcessCeilEdgeAdjustRight
#define mpProcessRunCeilEdgeAdjust ndsBaseMPProcessRunCeilEdgeAdjust
#define mpProcessCheckFloorEdgeCollisionL \
    ndsBaseMPProcessCheckFloorEdgeCollisionL
#define mpProcessFloorEdgeLAdjust ndsBaseMPProcessFloorEdgeLAdjust
#define mpProcessCheckFloorEdgeCollisionR \
    ndsBaseMPProcessCheckFloorEdgeCollisionR
#define mpProcessFloorEdgeRAdjust ndsBaseMPProcessFloorEdgeRAdjust
#define mpProcessRunFloorEdgeAdjust ndsBaseMPProcessRunFloorEdgeAdjust
#define mpProcessSetCollProjectFloorID \
    ndsBaseMPProcessSetCollProjectFloorID
#define mpProcessUpdateMain ndsBaseMPProcessUpdateMain
#define mpProcessCheckTestLWallCollision \
    ndsBaseMPProcessCheckTestLWallCollision
#define mpProcessRunLWallCollision ndsBaseMPProcessRunLWallCollision
#define mpProcessCheckTestRWallCollision \
    ndsBaseMPProcessCheckTestRWallCollision
#define mpProcessRunRWallCollision ndsBaseMPProcessRunRWallCollision
#define mpProcessCheckTestFloorCollisionNew \
    ndsBaseMPProcessCheckTestFloorCollisionNew
#define mpProcessCheckTestFloorCollision \
    ndsBaseMPProcessCheckTestFloorCollision
#define mpProcessCheckTestLCliffCollision \
    ndsBaseMPProcessCheckTestLCliffCollision
#define mpProcessCheckTestRCliffCollision \
    ndsBaseMPProcessCheckTestRCliffCollision
#define mpProcessCheckTestLWallCollisionAdjNew \
    ndsBaseMPProcessCheckTestLWallCollisionAdjNew
#define mpProcessRunLWallCollisionAdjNew \
    ndsBaseMPProcessRunLWallCollisionAdjNew
#define mpProcessCheckTestRWallCollisionAdjNew \
    ndsBaseMPProcessCheckTestRWallCollisionAdjNew
#define mpProcessRunRWallCollisionAdjNew \
    ndsBaseMPProcessRunRWallCollisionAdjNew
#define mpProcessCheckTestCeilCollisionAdjNew \
    ndsBaseMPProcessCheckTestCeilCollisionAdjNew
#define mpProcessRunCeilCollisionAdjNew \
    ndsBaseMPProcessRunCeilCollisionAdjNew
#define mpProcessCheckTestFloorCollisionAdjNew \
    ndsBaseMPProcessCheckTestFloorCollisionAdjNew
#define mpProcessRunFloorCollisionAdjNewNULL \
    ndsBaseMPProcessRunFloorCollisionAdjNewNULL
#define mpProcessSetLandingFloor ndsBaseMPProcessSetLandingFloor
#define mpProcessSetCollideFloor ndsBaseMPProcessSetCollideFloor

#include "../../decomp/BattleShip-main/decomp/src/mp/mpprocess.c"

#undef mpProcessSetCollideFloor
#undef mpProcessSetLandingFloor
#undef mpProcessRunFloorCollisionAdjNewNULL
#undef mpProcessCheckTestFloorCollisionAdjNew
#undef mpProcessRunCeilCollisionAdjNew
#undef mpProcessCheckTestCeilCollisionAdjNew
#undef mpProcessRunRWallCollisionAdjNew
#undef mpProcessCheckTestRWallCollisionAdjNew
#undef mpProcessRunLWallCollisionAdjNew
#undef mpProcessCheckTestLWallCollisionAdjNew
#undef mpProcessCheckTestRCliffCollision
#undef mpProcessCheckTestLCliffCollision
#undef mpProcessCheckTestFloorCollision
#undef mpProcessCheckTestFloorCollisionNew
#undef mpProcessRunRWallCollision
#undef mpProcessCheckTestRWallCollision
#undef mpProcessRunLWallCollision
#undef mpProcessCheckTestLWallCollision
#undef mpProcessUpdateMain
#undef mpProcessSetCollProjectFloorID
#undef mpProcessRunFloorEdgeAdjust
#undef mpProcessFloorEdgeRAdjust
#undef mpProcessCheckFloorEdgeCollisionR
#undef mpProcessFloorEdgeLAdjust
#undef mpProcessCheckFloorEdgeCollisionL
#undef mpProcessRunCeilEdgeAdjust
#undef mpProcessCeilEdgeAdjustRight
#undef mpProcessCheckCeilEdgeCollisionR
#undef mpProcessCeilEdgeAdjustLeft
#undef mpProcessCheckCeilEdgeCollisionL
#undef mpProcessGetLastWallCollideStats
#undef mpProcessSetLastWallCollideStats
#undef mpProcessSetLastWallCollideRight
#undef mpProcessSetLastWallCollideLeft
#undef mpProcessSetMultiWallLineID
#undef mpProcessResetMultiWallCount

#undef sMPProcessLastWallAngle
#undef sMPProcessLastWallFlags
#undef sMPProcessLastWallLineID
#undef sMPProcessLastWallCollidePosition
#undef sMPProcessMultiWallCollideLineIDs
#undef sMPProcessPad0x80130DE4
#undef sMPProcessMultiWallCollidesNum
#undef nMPMaterial4
#undef MAP_VERTEX_MAT_MASK
#undef MAP_FLAG_CLIFF_MASK
#undef MAP_FLAG_FLOOREDGE
#undef MAP_FLAG_CEILHEAVY
#undef MAP_FLAG_RCLIFF
#undef MAP_FLAG_LCLIFF
#undef MAP_VERTEX_COLL_CLIFF
