/*
 * Public ABI bridge for the renamed BattleShip mp/mpprocess.c TU.
 *
 * Keep this file to exact one-to-one forwarding wrappers (mpProcessUpdateMain
 * also picks the stack it runs on; the two wall tests first ask the port's
 * whole-test reject, ndsMPProcessWallTestMiss, which answers only when the
 * source test would have found nothing). The source import owns collision
 * behavior; this port seam owns only live symbol selection.
 */
#include <nds/nds_mpprocess_source.h>
#include <port/coroutine.h>

void mpProcessSetCollProjectFloorID(MPCollData *coll_data)
{
    ndsBaseMPProcessSetCollProjectFloorID(coll_data);
}

void mpProcessResetMultiWallCount(void)
{
    ndsBaseMPProcessResetMultiWallCount();
}

void mpProcessSetMultiWallLineID(s32 line_id)
{
    ndsBaseMPProcessSetMultiWallLineID(line_id);
}

sb32 mpProcessCheckTestLWallCollision(MPCollData *coll_data)
{
    if (ndsMPProcessWallTestMiss(coll_data, nMPLineKindLWall) != FALSE)
    {
        ndsBaseMPProcessResetMultiWallCount();
        return FALSE;
    }
    return ndsBaseMPProcessCheckTestLWallCollision(coll_data);
}

void mpProcessRunLWallCollision(MPCollData *coll_data)
{
    ndsBaseMPProcessRunLWallCollision(coll_data);
}

sb32 mpProcessCheckTestRWallCollision(MPCollData *coll_data)
{
    if (ndsMPProcessWallTestMiss(coll_data, nMPLineKindRWall) != FALSE)
    {
        ndsBaseMPProcessResetMultiWallCount();
        return FALSE;
    }
    return ndsBaseMPProcessCheckTestRWallCollision(coll_data);
}

void mpProcessRunRWallCollision(MPCollData *coll_data)
{
    ndsBaseMPProcessRunRWallCollision(coll_data);
}

sb32 mpProcessCheckTestFloorCollisionNew(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckTestFloorCollisionNew(coll_data);
}

sb32 mpProcessCheckTestFloorCollision(MPCollData *coll_data, s32 line_id)
{
    return ndsBaseMPProcessCheckTestFloorCollision(coll_data, line_id);
}

sb32 mpProcessCheckTestLCliffCollision(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckTestLCliffCollision(coll_data);
}

sb32 mpProcessCheckTestRCliffCollision(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckTestRCliffCollision(coll_data);
}

sb32 mpProcessCheckTestLWallCollisionAdjNew(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckTestLWallCollisionAdjNew(coll_data);
}

void mpProcessRunLWallCollisionAdjNew(MPCollData *coll_data)
{
    ndsBaseMPProcessRunLWallCollisionAdjNew(coll_data);
}

sb32 mpProcessCheckTestRWallCollisionAdjNew(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckTestRWallCollisionAdjNew(coll_data);
}

void mpProcessRunRWallCollisionAdjNew(MPCollData *coll_data)
{
    ndsBaseMPProcessRunRWallCollisionAdjNew(coll_data);
}

void mpProcessSetLandingFloor(MPCollData *coll_data)
{
    ndsBaseMPProcessSetLandingFloor(coll_data);
}

void mpProcessSetCollideFloor(MPCollData *coll_data)
{
    ndsBaseMPProcessSetCollideFloor(coll_data);
}

sb32 mpProcessCheckTestCeilCollisionAdjNew(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckTestCeilCollisionAdjNew(coll_data);
}

void mpProcessRunCeilCollisionAdjNew(MPCollData *coll_data)
{
    ndsBaseMPProcessRunCeilCollisionAdjNew(coll_data);
}

void mpProcessRunCeilEdgeAdjust(MPCollData *coll_data)
{
    ndsBaseMPProcessRunCeilEdgeAdjust(coll_data);
}

sb32 mpProcessCheckTestFloorCollisionAdjNew(
    MPCollData *coll_data, sb32 (*proc_map)(GObj *), GObj *gobj)
{
    return ndsBaseMPProcessCheckTestFloorCollisionAdjNew(
        coll_data, proc_map, gobj);
}

sb32 mpProcessRunFloorCollisionAdjNewNULL(MPCollData *coll_data)
{
    return ndsBaseMPProcessRunFloorCollisionAdjNewNULL(coll_data);
}

sb32 mpProcessCheckFloorEdgeCollisionL(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckFloorEdgeCollisionL(coll_data);
}

void mpProcessFloorEdgeLAdjust(MPCollData *coll_data)
{
    ndsBaseMPProcessFloorEdgeLAdjust(coll_data);
}

sb32 mpProcessCheckFloorEdgeCollisionR(MPCollData *coll_data)
{
    return ndsBaseMPProcessCheckFloorEdgeCollisionR(coll_data);
}

void mpProcessFloorEdgeRAdjust(MPCollData *coll_data)
{
    ndsBaseMPProcessFloorEdgeRAdjust(coll_data);
}

void mpProcessRunFloorEdgeAdjust(MPCollData *coll_data)
{
    ndsBaseMPProcessRunFloorEdgeAdjust(coll_data);
}

/* The one exception to exact forwarding: the map-collision step runs on the
 * DTCM hot stack (port/coroutine.h). Its geometry and collision callbacks set
 * flags and positions only; status changes, motion loads and every other I/O
 * happen after it returns, in the caller's proc_map. */
typedef struct NDSMPProcessUpdateCall
{
    MPCollData *coll_data;
    sb32 (*proc_coll)(MPCollData *, GObj *, u32);
    GObj *gobj;
    u32 flags;
} NDSMPProcessUpdateCall;

static unsigned int ndsMPProcessUpdateMainOnHotStack(void *arg)
{
    const NDSMPProcessUpdateCall *call = arg;

    return (unsigned int)ndsBaseMPProcessUpdateMain(
        call->coll_data, call->proc_coll, call->gobj, call->flags);
}

sb32 mpProcessUpdateMain(
    MPCollData *coll_data,
    sb32 (*proc_coll)(MPCollData *, GObj *, u32),
    GObj *gobj,
    u32 flags)
{
    NDSMPProcessUpdateCall call = { coll_data, proc_coll, gobj, flags };

    return (sb32)ndsDtcmHotStackRun(ndsMPProcessUpdateMainOnHotStack, &call);
}
