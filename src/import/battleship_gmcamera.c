#include <gm/generic.h>
#include <if/interface.h>
#include <sys/debug.h>
#include <sys/matrix.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#define syMatrixAdvance(mtx, mtx_heap, type) \
    (mtx = (mtx_heap).ptr, (mtx_heap).ptr = (void *)((type *)(mtx_heap).ptr + 1))
#define syMatrixAdvanceW(mtx, mtx_heap) syMatrixAdvance(mtx, mtx_heap, Mtx)

Mtx *sGCMatrixProjectL;
Mtx44f gGCMatrixPerspF;

f32 syVectorMag3D(Vec3f *vec);
f32 lbCommonTan(f32 angle);
f32 lbCommonSin(f32 angle);
f32 lbCommonCos(f32 angle);
void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]);
void lbCommonDrawSprite(GObj *camera_gobj);
void lbCommonInitCameraVec(CObj *cobj, u8 tk, u8 arg2);
void lbCommonInitCameraOrtho(CObj *cobj, u8 tk, u8 arg2);
void syRdpSetViewport(Vp *viewport, f32 ulx, f32 uly, f32 lrx, f32 lry);
void syRdpSetFuncLights(void (*func_lights)(Gfx **));
void syRdpResetSettings(Gfx **dl);
void func_80017CC8(CObj *cobj);
void func_8001663C(Gfx **dls, CObj *cobj, s32 buffer_id);
void gcPrepCameraMatrix(Gfx **dls, CObj *cobj);
void gcSetCameraMatrixMode(s32 val);
void gcRunFuncCamera(CObj *cobj, s32 arg);
void gcCaptureCameraGObj(GObj *camera_gobj, sb32 is_tag_mask_or_id);
void gcDrawDObjTreeForGObj(GObj *gobj);
void gmCameraDefaultFuncCamera(GObj *camera_gobj);
void gmCameraPlayerZoomFuncCamera(GObj *camera_gobj);
void gmCameraAnimFuncCamera(GObj *camera_gobj);
void gmCameraInishieFuncCamera(GObj *camera_gobj);
void gmCameraMapZoomFuncCamera(GObj *camera_gobj);
void gmCameraPlayerFollowFuncCamera(GObj *camera_gobj);
void gmCameraZebesFuncCamera(GObj *camera_gobj);
void gmCameraSetStatusDefault(void);
void grZebesAcidGetLevelInfo(f32 *current, f32 *step);
void grWallpaperMakeDecideKind(void);
void gmCameraMakePlayerArrowsCamera(void);
void ifScreenFlashMakeInterface(u8 alpha);
void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                        f32 *dist_x, f32 *dist_y);
void mpCollisionGetPlayerMapObjPosition(s32 player, Vec3f *pos);

#include "../../decomp/BattleShip-main/decomp/src/gm/gmcamera.c"
