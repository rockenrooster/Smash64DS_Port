/*
 * Missing wall-lower-edge ABI used by the BattleShip mpprocess TU.
 *
 * This is a narrow port compatibility provider, not an imported upstream TU.
 * It is compile-only with the private checkpoint and becomes reachable only
 * behind the reversible live source bridge.
 */
#include <nds/nds_mpprocess_source.h>

static void ndsPrivateMPGetWallEdgeD(s32 line_id, Vec3f *object_pos)
{
    s32 vertex_count;
    Vec3f first;
    Vec3f last;

    if (object_pos == NULL)
    {
        return;
    }
    if (line_id < 0)
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
        return;
    }
    vertex_count = mpCollisionGetVertexCountLineID(line_id);
    if (vertex_count <= 0)
    {
        object_pos->x = 0.0F;
        object_pos->y = 0.0F;
        object_pos->z = 0.0F;
        return;
    }
    mpCollisionGetVertexPositionID(line_id, 0, &first);
    mpCollisionGetVertexPositionID(line_id, vertex_count - 1, &last);
    *object_pos = (first.y < last.y) ? first : last;
}

void mpCollisionGetLWallEdgeD(s32 line_id, Vec3f *object_pos)
{
    ndsPrivateMPGetWallEdgeD(line_id, object_pos);
}

void mpCollisionGetRWallEdgeD(s32 line_id, Vec3f *object_pos)
{
    ndsPrivateMPGetWallEdgeD(line_id, object_pos);
}
