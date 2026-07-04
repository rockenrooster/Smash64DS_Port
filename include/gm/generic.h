#ifndef SSB64_NDS_GM_GENERIC_H
#define SSB64_NDS_GM_GENERIC_H

#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include <ssb_types.h>
#include <sys/objtypes.h>

extern void *gGMCommonFiles[8];
extern u32 dGMCommonFileIDs[8];

#define CAMERA_FLAG_BOUND_LEFT (1u << nGMCameraBoundLeft)
#define CAMERA_FLAG_BOUND_RIGHT (1u << nGMCameraBoundRight)
#define CAMERA_FLAG_BOUND_BOTTOM (1u << nGMCameraBoundBottom)
#define CAMERA_FLAG_BOUND_TOP (1u << nGMCameraBoundTop)

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

extern GObj *gGMCameraGObj;
extern GMCamera gGMCameraStruct;

#endif
