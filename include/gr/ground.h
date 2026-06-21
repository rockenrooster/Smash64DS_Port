#ifndef SSB64_NDS_GR_GROUND_H
#define SSB64_NDS_GR_GROUND_H

#include <PR/ultratypes.h>
#include <ssb_types.h>

/*
 * Narrow ground shadow for imported movie scenes. The full BattleShip ground
 * headers bring in stage/map/fighter dependencies that are outside the current
 * opening-movie boundary.
 */

enum {
    nMPMapObjKindMoviePlayer1 = 0
};

extern GObj *gGRCommonLayerGObjs[4];

void grWallpaperMakeDecideKind(void);
void grCommonSetupInitAll(void);
s32 mpCollisionGetMapObjCountKind(s32 kind);
void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids);
void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos);
void mpCollisionInitGroundData(void);

#endif
