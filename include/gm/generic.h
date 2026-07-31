#ifndef SSB64_NDS_GM_GENERIC_H
#define SSB64_NDS_GM_GENERIC_H

#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include <ssb_types.h>
#include <sys/objtypes.h>

extern void *gGMCommonFiles[8];
extern u32 dGMCommonFileIDs[8];

#ifndef _GMDEF_H_
#define CAMERA_FLAG_BOUND_LEFT (1u << nGMCameraBoundLeft)
#define CAMERA_FLAG_BOUND_RIGHT (1u << nGMCameraBoundRight)
#define CAMERA_FLAG_BOUND_BOTTOM (1u << nGMCameraBoundBottom)
#define CAMERA_FLAG_BOUND_TOP (1u << nGMCameraBoundTop)
#endif /* _GMDEF_H_ */

/* These two are gm/gmdef.h's, restated here because most port translation units
 * never reach the decomp tree. A unit that does reach it -- every one that
 * compiles a decomp source in place -- gets gmdef.h's definitions first, and
 * these would redeclare them. Testing gmdef.h's own include guard needs no
 * bookkeeping in the including unit and cannot drift out of step with it.
 * The values are identical either way; scripts/check-decomp-header-mirror.py
 * fails if that ever stops being true. */
#ifndef _GMDEF_H_
typedef enum GMCameraBounds {
    nGMCameraBoundLeft,
    nGMCameraBoundRight,
    nGMCameraBoundBottom,
    nGMCameraBoundTop
} GMCameraBounds;

typedef enum GMCameraStatus {
    nGMCameraStatusDefault,
    nGMCameraStatusPlayerZoom,
    nGMCameraStatusAnim,
    nGMCameraStatusInishie,
    nGMCameraStatusMapZoom,
    nGMCameraStatusPlayerFollow,
    nGMCameraStatusZebes,
    nGMCameraEnumCount
} GMCameraStatus;
#endif /* _GMDEF_H_ */

/* gm/gmtypes.h's struct, restated for the port translation units that never
 * reach the decomp tree. Field-for-field identical -- the layouts must match,
 * because a unit holding a different one would corrupt gGMCameraStruct. */
#ifndef _GMTYPES_H_
typedef struct GMCamera {
    s32 status_default;
    s32 status_curr;
    s32 status_prev;
    void (*func_camera)(GObj *);
    f32 target_dist;
    Vec3f vel_at;
    s32 viewport_ulx;
    s32 viewport_uly;
    s32 viewport_lrx;
    s32 viewport_lry;
    s32 viewport_center_x;
    s32 viewport_center_y;
    s32 viewport_width;
    s32 viewport_height;
    f32 fovy;
    GObj *pzoom_fighter_gobj;
    f32 pzoom_eye_x;
    f32 pzoom_eye_y;
    f32 pzoom_dist;
    f32 pzoom_pan_scale;
    f32 pzoom_fov;
    Vec3f zoom_origin_pos;
    Vec3f zoom_target_pos;
    GObj *pfollow_fighter_gobj;
    f32 pfollow_eye_x;
    f32 pfollow_eye_y;
    f32 pfollow_dist;
    f32 pfollow_pan_scale;
    f32 pfollow_fov;
    Vec3f vel_all;
    LookAt look_at;
} GMCamera;
#endif /* _GMTYPES_H_ */

extern GObj *gGMCameraGObj;
extern GMCamera gGMCameraStruct;

#endif
