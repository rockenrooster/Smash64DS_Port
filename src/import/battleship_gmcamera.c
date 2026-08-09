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

/* Take decomp's definition under a port name so the port can own the entry
 * point. Both call sites are in OTHER translation units -- decomp's
 * dLBCommonFuncMatrixList kind 0x4C (lbcommon.c:2145) and our own fighter
 * display-contract capture (reloc_backend_renderer_dl.c:12607) -- so neither
 * sees this rename and both bind to the wrapper below. */
#define gmCameraLookAtFuncMatrix battleship_gmCameraLookAtFuncMatrix

#include "../../decomp/BattleShip-main/decomp/src/gm/gmcamera.c"

#undef gmCameraLookAtFuncMatrix

/* TWO PIECES OF THIS FUNCTION ARE DEAD OR REDUNDANT, both bit-exact to remove.
 * Neither is a fixed-point change; they are here because they shrink what the
 * fixed-point conversion has to convert and they cost nothing to prove.
 *
 * W1 -- THE SECOND LOOK-AT IS REDUNDANT. gmcamera.c:985 runs the whole chain
 * twice when gmCameraGetMatrixMax() exceeds 32000: perspective, F2L, look-at,
 * concat. Only `scale` differs between the two syMatrixPerspFastF calls; the
 * two syMatrixLookAtReflectF calls take BYTE-IDENTICAL arguments, and
 * guMtxCatF writes gGMCameraMatrix rather than sp5C, so sp5C is still live
 * when the second call recomputes it. That second call is three sqrtf and
 * about thirty mul/add for a value the function already holds.
 *
 * W2 -- THE CLOSING F2L IS DEAD FOR ONE CALLER. The function ends with
 * syMatrixF2L(&gGMCameraMatrix, mtx), a full 4x4 float-to-fixed conversion
 * through the caller's out-pointer. reloc_backend_renderer_dl.c:12563 declares
 * `Mtx camera_mtx;`, passes it at :12607 and NEVER READS IT -- that call is
 * made purely for the side effects on gGCMatrixPerspF, sGCMatrixProjectL and
 * gGMCameraMatrix that ftdisplaymain.c:1093-1129 consumes. `mtx` is
 * write-only inside the function (the pointer is never stored; temp_mtx comes
 * from the graphics heap), so skipping the conversion when the caller passes
 * NULL is exact. decomp's own kind-0x4C caller keeps it and still gets it.
 *
 * Gated at RUNTIME on a .data word, not a #if, for the reason cycle 101 paid
 * for: a compile-time gate moved 672 bytes of `.main` and inverted the sign of
 * a real win through FTR placement alone. Both A/B arms must link identical. */
volatile u32 gNdsCameraMatrixLeanEnabled
    __attribute__((section(".data"))) = NDS_R2_CAMERA_MATRIX_LEAN;

volatile u32 gNdsCameraMatrixLeanRescaleCount;
volatile u32 gNdsCameraMatrixLeanSkippedF2LCount;

sb32 gmCameraLookAtFuncMatrix(Mtx *mtx, CObj *cobj, Gfx **dls)
{
    Mtx *temp_mtx;
    Mtx44f look_at_f;
    f32 max;

    if (gNdsCameraMatrixLeanEnabled == 0u)
    {
        /* NULL must be safe on BOTH paths, or the control arm would fault the
         * moment the caller below starts passing it. decomp's version writes
         * through `mtx` unconditionally, so give it somewhere to write and
         * throw the result away -- which is exactly the work W2 removes, so
         * the control arm keeps paying it and the comparison stays honest. */
        Mtx discarded;

        return battleship_gmCameraLookAtFuncMatrix(
            (mtx != NULL) ? mtx : &discarded, cobj, dls);
    }
    syMatrixAdvanceW(temp_mtx, gSYTaskmanGraphicsHeap);

    syMatrixPerspFastF(gGCMatrixPerspF, &cobj->projection.persp.norm,
                       cobj->projection.persp.fovy,
                       cobj->projection.persp.aspect,
                       cobj->projection.persp.near,
                       cobj->projection.persp.far,
                       cobj->projection.persp.scale);
    syMatrixF2L(&gGCMatrixPerspF, temp_mtx);
    sGCMatrixProjectL = temp_mtx;

    syMatrixLookAtReflectF(&look_at_f, &gGMCameraStruct.look_at,
                           cobj->vec.eye.x, cobj->vec.eye.y, cobj->vec.eye.z,
                           cobj->vec.at.x, cobj->vec.at.y, cobj->vec.at.z,
                           cobj->vec.up.x, cobj->vec.up.y, cobj->vec.up.z);
    guMtxCatF(look_at_f, gGCMatrixPerspF, gGMCameraMatrix);

    max = gmCameraGetMatrixMax();

    if (max > 32000.0F)
    {
        gNdsCameraMatrixLeanRescaleCount++;
        syMatrixPerspFastF(gGCMatrixPerspF, &cobj->projection.persp.norm,
                           cobj->projection.persp.fovy,
                           cobj->projection.persp.aspect,
                           cobj->projection.persp.near,
                           cobj->projection.persp.far,
                           32000.0F / max);
        syMatrixF2L(&gGCMatrixPerspF, temp_mtx);
        sGCMatrixProjectL = temp_mtx;
        /* W1: look_at_f still holds the first call's result, and the arguments
         * have not changed. The source recomputes it here; we reuse it. */
        guMtxCatF(look_at_f, gGCMatrixPerspF, gGMCameraMatrix);
    }
    /* W2 */
    if (mtx != NULL)
    {
        syMatrixF2L(&gGMCameraMatrix, mtx);
    }
    else
    {
        gNdsCameraMatrixLeanSkippedF2LCount++;
    }
    return 0;
}
