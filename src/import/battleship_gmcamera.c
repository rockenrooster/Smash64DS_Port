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

/* FOUR PIECES OF THIS FUNCTION ARE DEAD OR REDUNDANT, all bit-exact to remove.
 * None is a fixed-point change; they are here because they shrink what a
 * fixed-point conversion would have to convert and they cost nothing to prove.
 * The route word is a LEVEL so one binary can be poked through every
 * combination with byte-identical placement:
 *
 *   0  decomp original
 *   1  W1 + W2      shipped 2026-08-09 (cycle 103), WORK-H P50 -1,728
 *   2  + W3s        sparse concat -- SHIPPED DEFAULT
 *   3  + W2b        drop the dead projection F2L, its heap Mtx and its publish
 *                   -- MEASURED AND NOT SHIPPED, see ndsCameraPublishPersp
 *
 * The levels stay because level 3 is an open question, not a spent scaffold:
 * deleting them would delete the reproduction. Collapse them when W2b is
 * resolved either way.
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
 * W2b -- THE PROJECTION MATRIX IS WRITE-ONLY ON THIS PORT. See
 * ndsCameraBuildPersp below.
 *
 * W3s -- THE CONCAT IS 69% ZEROS. See ndsCameraCatLookAtPersp below.
 *
 * Gated at RUNTIME on a .data word, not a #if, for the reason cycle 101 paid
 * for: a compile-time gate moved 672 bytes of `.main` and inverted the sign of
 * a real win through FTR placement alone. Both A/B arms must link identical. */
volatile u32 gNdsCameraMatrixLeanEnabled
    __attribute__((section(".data"))) = NDS_R2_CAMERA_MATRIX_LEAN;

volatile u32 gNdsCameraMatrixLeanRescaleCount;
volatile u32 gNdsCameraMatrixLeanSkippedF2LCount;
volatile u32 gNdsCameraMatrixLeanSkippedProjectCount;

/* W2b -- THE PROJECTION MATRIX IS WRITE-ONLY ON THIS PORT. decomp converts
 * gGCMatrixPerspF into a graphics-heap Mtx and publishes it as
 * sGCMatrixProjectL for objdisplay.c:804-833 and :2887-2917, which emit it as
 * G_MW_MATRIX move-words. This port does not compile objdisplay.c -- the
 * renderer is reloc_backend_renderer_dl.c, and for this very camera it builds
 * its OWN 20.12 projection from the same CObj (:2891-2905, kind 0x4C). A scan
 * of the linked image finds exactly TWO references to sGCMatrixProjectL and
 * both are the stores here. So the syMatrixF2L, the 64-byte graphics-heap
 * allocation and the store are all dead: a second sixteen-element
 * float-to-fixed conversion, twice a frame, feeding nothing.
 *
 * The pointer would be NULLed rather than left stale, so that if objdisplay.c
 * is ever imported it faults on the first read instead of quietly rendering
 * through a matrix left in an earlier frame's graphics heap.
 *
 * IT IS NOT SHIPPED, AND THE REASON IS NOT THIS FUNCTION. Dropping the
 * syMatrixAdvanceW also stops consuming 64 bytes of gSYTaskmanGraphicsHeap per
 * call, which MOVES EVERY LATER ALLOCATION IN THE FRAME. With it on, the
 * Boundary realtime verifier fails its locked-30 phase accounting
 * (phaseLag=-1: gNdsBattlePlayablePacingPhasePresentCount sums one ahead of
 * gNdsBattlePlayablePacingPresentedFrames), deterministically, on a
 * byte-identical binary -- route 3 red, route 0 green, same ROM. Neither
 * counter has a second write site and the present path is presented-then-phase
 * with no early return between them, so that skew is unexplained, and an
 * unexplained state difference is a failure. Left at level 3, off by default,
 * with the reproduction recorded, rather than shipped or deleted.
 *
 * project_mtx == NULL is the "skip it" request; the Mtx is allocated ONCE by
 * the caller, exactly as decomp does, so the rescale pass reuses it rather than
 * advancing the heap a second time. */
static void ndsCameraPublishPersp(CObj *cobj, f32 scale, Mtx *project_mtx)
{
    syMatrixPerspFastF(gGCMatrixPerspF, &cobj->projection.persp.norm,
                       cobj->projection.persp.fovy,
                       cobj->projection.persp.aspect,
                       cobj->projection.persp.near,
                       cobj->projection.persp.far,
                       scale);
    if (project_mtx == NULL)
    {
        gNdsCameraMatrixLeanSkippedProjectCount++;
    }
    else
    {
        syMatrixF2L(&gGCMatrixPerspF, project_mtx);
        sGCMatrixProjectL = project_mtx;
    }
}

/* W3s -- THE CONCAT IS 69% ZEROS. syMatrixPerspFastF (matrix.c:575) writes only
 * FIVE non-zero elements -- [0][0], [1][1], [2][2], [2][3] and [3][2] -- and
 * explicitly stores 0 into the other eleven, [3][3] included. guMtxCatF
 * (libultra/gu/mtxcatf.c) is a general 4x4 product: 64 multiplies, 48 adds and
 * a sixteen-element temp copy, of which 44 multiplies and 44 adds are against a
 * literal zero. What is left is 20 multiplies and 4 adds.
 *
 * BIT-EXACT, and the association is preserved rather than merely equivalent.
 * guMtxCatF accumulates left to right, so for column 2 it forms
 * (((l0*0 + l1*0) + l2*P22) + l3*P32); a zero of either sign is the additive
 * identity for every finite value, so that reduces to exactly
 * (l2*P22) + l3*P32 -- the same two products and the same single add, in the
 * same order. The only representable difference is the SIGN of a zero result,
 * which no consumer of gGMCameraMatrix can observe: func_ovl2_800EB924
 * (ftparam.c:2421) reciprocates a value already clamped away from zero by
 * |scale| < 0.1, and gmCameraGetMatrixMax takes ABSF.
 *
 * Each row is read whole before it is written, so `out` may alias either input
 * -- which is the only thing guMtxCatF's temp[4][4] was buying. */
static void ndsCameraCatLookAtPersp(Mtx44f look_at, Mtx44f persp, Mtx44f out)
{
    const f32 p00 = persp[0][0];
    const f32 p11 = persp[1][1];
    const f32 p22 = persp[2][2];
    const f32 p23 = persp[2][3];
    const f32 p32 = persp[3][2];
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        const f32 l0 = look_at[i][0];
        const f32 l1 = look_at[i][1];
        const f32 l2 = look_at[i][2];
        const f32 l3 = look_at[i][3];

        out[i][0] = l0 * p00;
        out[i][1] = l1 * p11;
        out[i][2] = (l2 * p22) + (l3 * p32);
        out[i][3] = l2 * p23;
    }
}

static void ndsCameraCatCamera(u32 level, Mtx44f look_at)
{
    if (level >= 2u)
    {
        ndsCameraCatLookAtPersp(look_at, gGCMatrixPerspF, gGMCameraMatrix);
    }
    else
    {
        guMtxCatF(look_at, gGCMatrixPerspF, gGMCameraMatrix);
    }
}

sb32 gmCameraLookAtFuncMatrix(Mtx *mtx, CObj *cobj, Gfx **dls)
{
    const u32 level = gNdsCameraMatrixLeanEnabled;
    Mtx *temp_mtx = NULL;
    Mtx44f look_at_f;
    f32 max;

    if (level == 0u)
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
    if (level < 3u)
    {
        syMatrixAdvanceW(temp_mtx, gSYTaskmanGraphicsHeap);
    }
    else
    {
        sGCMatrixProjectL = NULL;
    }
    ndsCameraPublishPersp(cobj, cobj->projection.persp.scale, temp_mtx);

    syMatrixLookAtReflectF(&look_at_f, &gGMCameraStruct.look_at,
                           cobj->vec.eye.x, cobj->vec.eye.y, cobj->vec.eye.z,
                           cobj->vec.at.x, cobj->vec.at.y, cobj->vec.at.z,
                           cobj->vec.up.x, cobj->vec.up.y, cobj->vec.up.z);
    ndsCameraCatCamera(level, look_at_f);

    max = gmCameraGetMatrixMax();

    if (max > 32000.0F)
    {
        gNdsCameraMatrixLeanRescaleCount++;
        ndsCameraPublishPersp(cobj, 32000.0F / max, temp_mtx);
        /* W1: look_at_f still holds the first call's result, and the arguments
         * have not changed. The source recomputes it here; we reuse it. */
        ndsCameraCatCamera(level, look_at_f);
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
