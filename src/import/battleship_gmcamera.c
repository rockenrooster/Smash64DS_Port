#include <gm/generic.h>
#include <nds/nds_p5.h>
#include <if/interface.h>
#include <sys/debug.h>
#include <sys/matrix.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

#include <nds/nds_r2_camera_fixed.h>

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
#define gmCameraMakeBattleCamera battleship_gmCameraMakeBattleCamera

/* 2026-10-05 (owner: fixed point only): the interest box's strong definition
 * is in battleship_gmcamera_fixed.c; the three cameras here that call it bind
 * to that one at link time. */
#pragma weak gmCameraUpdateInterests
/* 2026-10-06: the default battle camera's strong definition is the Q12 one in
 * battleship_gmcamera_fixed.c (dGMCameraFuncList binds to it at link time). */
#pragma weak gmCameraDefaultFuncCamera
/* 2026-10-06: Zebes' and Mushroom Kingdom's cameras too (the same Q12 body). */
#pragma weak gmCameraZebesFuncCamera
#pragma weak gmCameraInishieFuncCamera
/* 2026-10-06: the exact integer-compare form is in battleship_gmcamera_fixed.c. */
#pragma weak gmCameraCheckTargetInBounds

#include "../../decomp/BattleShip-main/decomp/src/gm/gmcamera.c"

#undef gmCameraMakeBattleCamera
#undef gmCameraLookAtFuncMatrix

const LookAt *ndsR2CameraCurrentLookAt(void)
{
    return &gGMCameraStruct.look_at;
}

/* ==========================================================================
 * Q20.12 CAMERA + PROJECTION KERNELS.  Rationale, basis and the ratio they
 * exist to falsify are in include/nds/nds_r2_camera_fixed.h.
 *
 * They live in this translation unit, not in a header, because BOTH producers
 * need them -- the game camera below and the renderer adapter's camera sites in
 * reloc_backend_renderer_dl.c -- and a `static inline` header form would put
 * ~600 bytes of `.main` in two objects instead of one.  Bytes are the thing
 * this campaign keeps paying for: the collision ring's arithmetic win was
 * entirely eaten by compulsory fetch of its own replacement bytes.
 * ========================================================================== */

volatile u32 gNdsR2CameraFixedLookAtCalls __attribute__((used));
volatile u32 gNdsR2CameraFixedPerspCalls __attribute__((used));
volatile u32 gNdsR2CameraFixedFloatLookAtCalls __attribute__((used));
volatile u32 gNdsR2CameraFixedFloatPerspCalls __attribute__((used));
volatile u32 gNdsR2CameraFixedGameCalls __attribute__((used));
volatile u32 gNdsR2CameraFixedGameFloatCalls __attribute__((used));
volatile u32 gNdsR2CameraFixedSaturateCount __attribute__((used));
volatile u32 gNdsR2CameraFixedDegenerateCount __attribute__((used));
volatile u32 gNdsR2CameraFixedRescaleCount __attribute__((used));

/* A9 (2026-09-27): the framing witness is a diagnostic for
 * scripts/diagnostics/probe-native-render-scene.ps1, built only with
 * NDS_CAMERA_FRAME_WITNESS=1. It cost ~1.7K ticks a frame (four fighters'
 * soft-float margins) in every build, the gate ROM included. */
#define ndsCameraResetFrameWitness() ((void)0)
#define ndsCameraRecordFrame(center, half_w, half_h) \
    ((void)(center), (void)(half_w), (void)(half_h))

/* These three functions are the source battle-follow callers with one bounded
 * diagnostic call immediately after gmCameraUpdateInterests.  The original
 * interest calculation runs once; no camera math is repeated. */
static void ndsCameraDefaultFuncCameraWitness(GObj *camera_gobj)
{
    /* The Q12 camera (battleship_gmcamera_fixed.c), the default status's
     * strong definition; the float chain above stays for the lab witness. */
    gmCameraDefaultFuncCamera(camera_gobj);
}

static void ndsCameraZebesFuncCameraWitness(GObj *camera_gobj)
{
    /* The Q12 camera (battleship_gmcamera_fixed.c), as the default's. */
    gmCameraZebesFuncCamera(camera_gobj);
}

static void ndsCameraInishieFuncCameraWitness(GObj *camera_gobj)
{
    /* The Q12 camera (battleship_gmcamera_fixed.c), as the default's. */
    gmCameraInishieFuncCamera(camera_gobj);
}

void gmCameraMakeBattleCamera(void)
{
    ndsCameraResetFrameWitness();
    dGMCameraFuncList[nGMCameraStatusDefault] = ndsCameraDefaultFuncCameraWitness;
    dGMCameraFuncList[nGMCameraStatusInishie] = ndsCameraInishieFuncCameraWitness;
    dGMCameraFuncList[nGMCameraStatusZebes] = ndsCameraZebesFuncCameraWitness;
    battleship_gmCameraMakeBattleCamera();
#if NDS_P5
    /* P5 (Remix Camera.asm :67, :83): Hyrule Temple and Dinosaur Land frame
     * the match with Mushroom Kingdom's camera, set the way
     * gmCameraMakeDefaultCamera sets a stage's status (gmcamera.c:1175). */
    if ((gSCManagerBattleState != NULL) &&
        (ndsP5StageCameraInishie((u32)gSCManagerBattleState->gkind) != FALSE))
    {
        gGMCameraStruct.status_curr = nGMCameraStatusInishie;
        gGMCameraStruct.status_default = gGMCameraStruct.status_prev =
            gGMCameraStruct.status_curr;
        gGMCameraStruct.func_camera =
            dGMCameraFuncList[gGMCameraStruct.status_curr];
    }
#endif
}

#define NDS_R2_CAM_Q 12
#define NDS_R2_CAM_ONE (1 << NDS_R2_CAM_Q)

/* The DS divide and square-root units, written out rather than pulled from
 * libnds so this TU keeps its decomp include set.  Same register sequence as
 * nds/arm9/math.h's div64/sqrt64 -- which are `static inline` there and
 * therefore add no call either way -- MINUS the leading busy poll, i.e. the
 * form include/nds/nds_r2_hwmath_unit.h ships and SM64DS's cstd::div and
 * cstd::sqrt use (decomp/sm64ds-decomp/src/_ZN4cstd3divEii.c).
 *
 * WHY THE LEADING POLL IS GONE (2026-08-16, with NDS_R2_CAMERA_FIXED's default
 * flip).  GBATEK: writing DIVCNT, DIV_NUMER or DIV_DENOM restarts the division
 * and raises DIVCNT bit 15, and the same holds for SQRTCNT / SQRT_PARAM.  Only
 * the LAST write matters, so a poll that PRECEDES the parameter writes waits out
 * a result nobody is going to read.  These two were the last leading polls in
 * the binary; they were left in place while this chain was default-off because
 * they executed zero times and editing them would have staled the -4,736 tk/fr
 * the owner's decision was priced on.  The decision is taken, so they execute --
 * measured elsewhere at 43.5 tk/call for a divide and 23.0 tk/call for a root
 * (../../../artifacts/performance/2026-08-16_hwmath-route/HWROUTE.md section 2,
 * per-PC iteration counts at two live sites) -- and they come out.
 *
 * ONE SHARED SET OF REGISTERS.  Every caller of these kernels runs inside the
 * frame loop's display phase; none is reachable from an interrupt handler, so
 * the write/poll/read sequence cannot be interleaved.  nds_r2_sqrtf.c masks IME
 * around its own use because sqrtf is reachable from imported audio; that does
 * not apply here and the mask is two I/O writes that would be pure cost. */
#define NDS_R2_CAM_DIVCNT      (*(volatile u16 *)0x04000280)
#define NDS_R2_CAM_DIV_NUMER   (*(volatile s64 *)0x04000290)
#define NDS_R2_CAM_DIV_DENOM   (*(volatile s32 *)0x04000298)
#define NDS_R2_CAM_DIV_RESULT  (*(volatile s32 *)0x040002A0)
#define NDS_R2_CAM_SQRTCNT     (*(volatile u16 *)0x040002B0)
#define NDS_R2_CAM_SQRT_RESULT (*(volatile u32 *)0x040002B4)
#define NDS_R2_CAM_SQRT_PARAM  (*(volatile s64 *)0x040002B8)
#define NDS_R2_CAM_BUSY        0x8000u
#define NDS_R2_CAM_DIV_64_32   1u
#define NDS_R2_CAM_SQRT_64     1u

/* ARM state, not Thumb.  -mthumb has no SMULL, so every (s64)a * b below would
 * become a call into __aeabi_lmul -- already live at 539.2 muls/frame and
 * 4.49 cycles each.  One missing target("arm") on a pure-precision change cost
 * +36,032 P95 once and the attribute won -71,616 back. */
#define NDS_R2_CAM_ARM __attribute__((noinline, target("arm")))
/* Same reason, without forcing an out-of-line copy of a function that is
 * already the compilation unit's boundary. MEASURED, not assumed: the first
 * build of these kernels left both bodies in Thumb and objdump found
 * EIGHTEEN `bl __aeabi_lmul` in the look-at alone -- the nine sums of squares
 * and the nine translation-row products -- at 4.49 cycles a multiply plus the
 * call. That is the whole trap, reproduced. */
#define NDS_R2_CAM_ARM_FN __attribute__((target("arm")))
/* THE LEAVES ARE DELIBERATELY NOT INLINED, and this is the measured result of
 * the cycle rather than an oversight.
 *
 * One look-at entry executes FORTY-TWO calls: nine `bl ndsR2CamF32ToQ`, twelve
 * `bl ndsR2CamMulQ`, nine `bl ndsR2CamDivQ64` (each calling ndsR2CamDiv64
 * again) and three `bl ndsR2CamSqrt64`, for bodies of one to seven
 * instructions. That looks obviously wrong, and the obvious fix was built and
 * measured: `always_inline` plus a matching `target("arm")` (GCC will not
 * inline across differing target attributes, so both are needed) cut it to
 * twelve calls and grew `.main` by 3,032 B --
 *
 *     build-c201 (this shape)   paired median WORK-H  -4,736 tk/fr
 *     build-c202 (all inlined)  paired median WORK-H  +1,600 tk/fr
 *
 * -- i.e. inlining INVERTED the win. The kernels are entered 8.138 times a
 * frame; at that rate their own bytes are compulsory instruction fetch, and
 * ~95 added cache lines at the machine's measured 23-51 cycle icache_fill cost
 * swamp forty-two `bl`s. Same mechanism as the collision ring's K-ICACHE null,
 * at 8.138 entries/frame instead of 0.97.
 *
 * So the classification's "55% of the lane converts to code SMALLER than the
 * `bl` it deletes" does not hold for a normalize-heavy kernel: the fixed forms
 * of a divide, a root and a float->Q12 conversion are not one instruction each.
 * Do not "optimize" this by inlining without re-measuring the pair. */
#define NDS_R2_CAM_LEAF NDS_R2_CAM_ARM static

/* ITCM (2026-09-27 census: ~1.3K ticks/frame non-memory stall at 68 B). */
NDS_R2_CAM_ARM static s32 __attribute__((section(".itcm")))
ndsR2CamDiv64(s64 numerator, s32 denominator)
{
    NDS_R2_CAM_DIVCNT = NDS_R2_CAM_DIV_64_32;
    NDS_R2_CAM_DIV_NUMER = numerator;
    NDS_R2_CAM_DIV_DENOM = denominator;
    while ((NDS_R2_CAM_DIVCNT & NDS_R2_CAM_BUSY) != 0u)
    {
    }
    return NDS_R2_CAM_DIV_RESULT;
}

NDS_R2_CAM_ARM static s32 ndsR2CamSqrt64(s64 value)
{
    NDS_R2_CAM_SQRTCNT = NDS_R2_CAM_SQRT_64;
    NDS_R2_CAM_SQRT_PARAM = value;
    while ((NDS_R2_CAM_SQRTCNT & NDS_R2_CAM_BUSY) != 0u)
    {
    }
    return (s32)NDS_R2_CAM_SQRT_RESULT;
}

/* Q12 quotient of two Q12 (or two equally-scaled) values, rounded half away
 * from zero.  The float chain it replaces ends in ndsRendererMtxLoadN64ToDS20p12
 * rounding to nearest, so truncating here would bias every matrix element one
 * way; one extra fractional bit and a halve is free next to the divide. */
NDS_R2_CAM_LEAF s32 ndsR2CamDivQ64(s64 numerator, s32 denominator)
{
    s32 q = ndsR2CamDiv64(numerator << (NDS_R2_CAM_Q + 1), denominator);

    return (q >= 0) ? ((q + 1) >> 1) : -(((-q) + 1) >> 1);
}

NDS_R2_CAM_LEAF s32 ndsR2CamDivQ(s32 numerator, s32 denominator)
{
    return ndsR2CamDivQ64((s64)numerator, denominator);
}

NDS_R2_CAM_LEAF s32 ndsR2CamMulQ(s32 a, s32 b)
{
    return (s32)((((s64)a * b) + (NDS_R2_CAM_ONE >> 1)) >> NDS_R2_CAM_Q);
}

/* The concat is the only place a product can leave Q20.12's +/-524,288, because
 * it multiplies a translation row by a projection term.  Range analysis says it
 * cannot at this camera's distances; the counter is what turns "says" into
 * "measured", and it must read hard zero. */
NDS_R2_CAM_LEAF s32 ndsR2CamMulQSat(s32 a, s32 b)
{
    s64 wide = (((s64)a * b) + (NDS_R2_CAM_ONE >> 1)) >> NDS_R2_CAM_Q;

    if ((wide > (s64)0x7fffffff) || (wide < -(s64)0x80000000))
    {
        NDS_DIAG(gNdsR2CameraFixedSaturateCount++);
        return (wide > 0) ? (s32)0x7fffffff : (s32)0x80000000;
    }
    return (s32)wide;
}

/* f32 -> Q12, integer only.  Nine values per look-at and six per perspective:
 * at 13.23 + 6.46 ticks the `(s32)(v * 4096.0F)` form would hand back about a
 * ninth of what these kernels delete, which is exactly the boundary-conversion
 * failure the classification told the implementation to disprove for itself. */
NDS_R2_CAM_LEAF s32 ndsR2CamF32ToQ(f32 value)
{
    union
    {
        f32 f;
        u32 u;
    } bits;
    u32 exponent;
    u32 magnitude;
    s32 shift;

    bits.f = value;
    exponent = (bits.u >> 23) & 0xffu;
    if (exponent == 0u)
    {
        return 0;
    }
    if (exponent == 0xffu)
    {
        NDS_DIAG(gNdsR2CameraFixedDegenerateCount++);
        return 0;
    }
    magnitude = (bits.u & 0x7fffffu) | 0x800000u;
    shift = (s32)exponent - 127 - 23 + NDS_R2_CAM_Q;
    if (shift < 0)
    {
        magnitude = (shift <= -24) ? 0u : (magnitude >> (u32)(-shift));
    }
    else if (shift > 7)
    {
        /* |value| >= 2^19, which Q20.12 cannot hold. */
        NDS_DIAG(gNdsR2CameraFixedSaturateCount++);
        magnitude = 0x7fffffffu;
    }
    else
    {
        magnitude <<= (u32)shift;
    }
    return ((bits.u & 0x80000000u) != 0u) ? -(s32)magnitude : (s32)magnitude;
}

/* Q12 -> f32 without a multiply: __aeabi_i2f, then subtract 12 from the
 * exponent field.  Exact for every non-zero q (|q| >= 1 leaves the exponent
 * far above denormal), and it drops 13.23 ticks of __aeabi_fmul from each of
 * the thirty-two write-backs a frame. */
NDS_R2_CAM_LEAF f32 ndsR2CamQToF32(s32 value)
{
    union
    {
        f32 f;
        u32 u;
    } bits;

    if (value == 0)
    {
        return 0.0F;
    }
    bits.f = (f32)value;
    bits.u -= ((u32)NDS_R2_CAM_Q << 23);
    return bits.f;
}

/* FTOFRAC8(x) == (int)MIN(x * 128.0F, 127.0F) & 0xff.  x * 128 in Q12 is
 * x >> 5, and the cast truncates toward zero rather than flooring, so the
 * negative half needs the explicit form. */
NDS_R2_CAM_LEAF s32 ndsR2CamFrac8(s32 value)
{
    s32 scaled = (value >= 0) ? (value >> 5) : -((-value) >> 5);

    if (scaled > 127)
    {
        scaled = 127;
    }
    return scaled & 0xff;
}

NDS_R2_CAM_ARM_FN void ndsR2CameraLookAtReflect20p12(
    NDSRendererMatrix20p12 *out, LookAt *l,
    f32 eye_x, f32 eye_y, f32 eye_z,
    f32 at_x, f32 at_y, f32 at_z,
    f32 up_x, f32 up_y, f32 up_z)
{
    s32 ex = ndsR2CamF32ToQ(eye_x);
    s32 ey = ndsR2CamF32ToQ(eye_y);
    s32 ez = ndsR2CamF32ToQ(eye_z);
    s32 lx = ndsR2CamF32ToQ(at_x) - ex;
    s32 ly = ndsR2CamF32ToQ(at_y) - ey;
    s32 lz = ndsR2CamF32ToQ(at_z) - ez;
    s32 ux = ndsR2CamF32ToQ(up_x);
    s32 uy = ndsR2CamF32ToQ(up_y);
    s32 uz = ndsR2CamF32ToQ(up_z);
    s32 rx;
    s32 ry;
    s32 rz;
    s32 mag;

    NDS_DIAG(gNdsR2CameraFixedLookAtCalls++);

    /* Negated, because positive Z is behind us -- matrix.c:305. */
    mag = ndsR2CamSqrt64(((s64)lx * lx) + ((s64)ly * ly) + ((s64)lz * lz));
    if (mag == 0)
    {
        NDS_DIAG(gNdsR2CameraFixedDegenerateCount++);
        mag = 1;
    }
    lx = -ndsR2CamDivQ(lx, mag);
    ly = -ndsR2CamDivQ(ly, mag);
    lz = -ndsR2CamDivQ(lz, mag);

    rx = ndsR2CamMulQ(uy, lz) - ndsR2CamMulQ(uz, ly);
    ry = ndsR2CamMulQ(uz, lx) - ndsR2CamMulQ(ux, lz);
    rz = ndsR2CamMulQ(ux, ly) - ndsR2CamMulQ(uy, lx);
    mag = ndsR2CamSqrt64(((s64)rx * rx) + ((s64)ry * ry) + ((s64)rz * rz));
    if (mag == 0)
    {
        NDS_DIAG(gNdsR2CameraFixedDegenerateCount++);
        mag = 1;
    }
    rx = ndsR2CamDivQ(rx, mag);
    ry = ndsR2CamDivQ(ry, mag);
    rz = ndsR2CamDivQ(rz, mag);

    ux = ndsR2CamMulQ(ly, rz) - ndsR2CamMulQ(lz, ry);
    uy = ndsR2CamMulQ(lz, rx) - ndsR2CamMulQ(lx, rz);
    uz = ndsR2CamMulQ(lx, ry) - ndsR2CamMulQ(ly, rx);
    mag = ndsR2CamSqrt64(((s64)ux * ux) + ((s64)uy * uy) + ((s64)uz * uz));
    if (mag == 0)
    {
        NDS_DIAG(gNdsR2CameraFixedDegenerateCount++);
        mag = 1;
    }
    ux = ndsR2CamDivQ(ux, mag);
    uy = ndsR2CamDivQ(uy, mag);
    uz = ndsR2CamDivQ(uz, mag);

    /* NULL is the renderer adapter's three sites, whose LookAt is a stack local
     * that nothing reads -- so the six conversions and sixteen constant bytes
     * are skipped there rather than computed and dropped. */
    if (l != NULL)
    {
        l->l[0].l.dir[0] = (s8)ndsR2CamFrac8(rx);
        l->l[0].l.dir[1] = (s8)ndsR2CamFrac8(ry);
        l->l[0].l.dir[2] = (s8)ndsR2CamFrac8(rz);
        l->l[1].l.dir[0] = (s8)ndsR2CamFrac8(ux);
        l->l[1].l.dir[1] = (s8)ndsR2CamFrac8(uy);
        l->l[1].l.dir[2] = (s8)ndsR2CamFrac8(uz);
        l->l[0].l.col[0] = 0x00;
        l->l[0].l.col[1] = 0x00;
        l->l[0].l.col[2] = 0x00;
        l->l[0].l.pad1 = 0x00;
        l->l[0].l.colc[0] = 0x00;
        l->l[0].l.colc[1] = 0x00;
        l->l[0].l.colc[2] = 0x00;
        l->l[0].l.pad2 = 0x00;
        l->l[1].l.col[0] = 0x00;
        l->l[1].l.col[1] = 0x80;
        l->l[1].l.col[2] = 0x00;
        l->l[1].l.pad1 = 0x00;
        l->l[1].l.colc[0] = 0x00;
        l->l[1].l.colc[1] = 0x80;
        l->l[1].l.colc[2] = 0x00;
        l->l[1].l.pad2 = 0x00;
    }

    out->m[0][0] = rx;
    out->m[1][0] = ry;
    out->m[2][0] = rz;
    out->m[3][0] = -(s32)(((((s64)ex * rx) + ((s64)ey * ry) + ((s64)ez * rz)) +
                           (NDS_R2_CAM_ONE >> 1)) >> NDS_R2_CAM_Q);

    out->m[0][1] = ux;
    out->m[1][1] = uy;
    out->m[2][1] = uz;
    out->m[3][1] = -(s32)(((((s64)ex * ux) + ((s64)ey * uy) + ((s64)ez * uz)) +
                           (NDS_R2_CAM_ONE >> 1)) >> NDS_R2_CAM_Q);

    out->m[0][2] = lx;
    out->m[1][2] = ly;
    out->m[2][2] = lz;
    out->m[3][2] = -(s32)(((((s64)ex * lx) + ((s64)ey * ly) + ((s64)ez * lz)) +
                           (NDS_R2_CAM_ONE >> 1)) >> NDS_R2_CAM_Q);

    out->m[0][3] = 0;
    out->m[1][3] = 0;
    out->m[2][3] = 0;
    out->m[3][3] = NDS_R2_CAM_ONE;
}

NDS_R2_CAM_ARM_FN void ndsR2CameraPerspFast20p12(
    NDSRendererMatrix20p12 *out, u16 *persp_norm,
    f32 fovy, f32 aspect, f32 near, f32 far,
    f32 scale)
{
    /* The table index stays in float on purpose.  One step is pi/2048 of half
     * the field of view; reproducing decomp's two f32 roundings in fixed point
     * to guarantee the SAME step is more risk than three float calls are worth,
     * and getting it wrong is a 0.15% zoom that nothing else would explain. */
    f32 half = fovy * 0.008726646F;
    s32 id = ((s32)(half * ((f32)0x800 / PI32))) & 0xFFF;
    s32 idc = id + 0x400;
    s32 sn = (s32)gSYSinTable[id & 0x7FF];
    s32 cs = (s32)gSYSinTable[idc & 0x7FF];
    s32 scale_q = ndsR2CamF32ToQ(scale);
    s32 near_q = ndsR2CamF32ToQ(near);
    s32 far_q = ndsR2CamF32ToQ(far);
    s32 denom = near_q - far_q;
    s32 cot;
    s64 nf;
    u32 row;
    u32 col;

    NDS_DIAG(gNdsR2CameraFixedPerspCalls++);

    if ((id & 0x800) != 0)
    {
        sn = -sn;
    }
    if ((idc & 0x800) != 0)
    {
        cs = -cs;
    }
    if (sn == 0)
    {
        NDS_DIAG(gNdsR2CameraFixedDegenerateCount++);
        sn = 1;
    }
    if (denom == 0)
    {
        NDS_DIAG(gNdsR2CameraFixedDegenerateCount++);
        denom = 1;
    }
    /* Both table entries carry the same scale, so the quotient is dimensionless
     * and ndsR2CamDivQ returns it in Q12 directly. */
    cot = ndsR2CamDivQ(cs, sn);

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            out->m[row][col] = 0;
        }
    }
    out->m[0][0] = ndsR2CamMulQ(ndsR2CamDivQ(cot, ndsR2CamF32ToQ(aspect)),
                                scale_q);
    out->m[1][1] = ndsR2CamMulQ(cot, scale_q);
    out->m[2][2] = ndsR2CamDivQ(ndsR2CamMulQ(near_q + far_q, scale_q), denom);
    out->m[2][3] = -scale_q;
    /* 2 * near * far * scale leaves Q20.12 outright at this stage's numbers
     * (256 x 39936 alone is 8.4e10 in Q12), so it never lands in an s32: the
     * product stays 64-bit until the hardware divider consumes it. */
    nf = (((s64)near_q * far_q) >> (NDS_R2_CAM_Q - 1));
    nf = ((nf * scale_q) >> NDS_R2_CAM_Q);
    out->m[3][2] = ndsR2CamDivQ64(nf, denom);
    out->m[3][3] = 0;

    if (persp_norm != NULL)
    {
        if ((near_q + far_q) <= (2 << NDS_R2_CAM_Q))
        {
            *persp_norm = 0xFFFF;
        }
        else
        {
            /* (2.0F * 65536.0F) / (near + far), truncated by the f32 -> u16
             * assignment decomp performs -- including its wrap at 65536, which
             * the following zero test is what catches. */
            u16 norm = (u16)ndsR2CamDiv64(((s64)131072) << NDS_R2_CAM_Q,
                                          near_q + far_q);

            if (norm == 0u)
            {
                norm = 1u;
            }
            *persp_norm = norm;
        }
    }
}

/* THE BATTLE CAMERA'S MATRICES, Q20.12 ONLY (2026-10-06, owner: "Delete old
 * machinery"). gmcamera.c:985 builds the perspective, the look-at and their
 * concat; the Q20.12 chain has shipped since 2026-08-16 and the lean level 2
 * since cycle 103, so the float arms, the level word
 * gNdsCameraMatrixLeanEnabled and the route word gNdsR2CameraFixedEnabled are
 * gone (artifacts/performance/2026-08-16_camera-fixedpoint holds their A/Bs).
 * What remains differs from the decomp function by three bit-exact
 * omissions: W1, a rescale reuses the look-at (its arguments are
 * byte-identical); W2, a NULL `mtx` skips the closing conversion (the
 * renderer's caller passes NULL and reads gGMCameraMatrix and the projection
 * itself); W3s, the look-at x perspective concat takes the perspective's five
 * non-zero terms. The projection Mtx is still taken from the graphics heap and
 * published (W2b, measured and not shipped: dropping it moved every later
 * allocation of the frame and failed the Boundary realtime verifier's phase
 * accounting). */
volatile u32 gNdsCameraMatrixLeanRescaleCount;
volatile u32 gNdsCameraMatrixLeanSkippedF2LCount;
volatile u32 gNdsCameraMatrixLeanSkippedProjectCount;

static void ndsCameraStoreQAsMtx(const NDSRendererMatrix20p12 *src, Mtx *dst)
{
    s32 *integral = &dst->m[0][0];
    s32 *fractional = &dst->m[2][0];
    u32 row;

    /* q12 << 4 IS the s15.16 syMatrixF2L would have written, minus the four
     * low bits ndsRendererMtxLoadN64ToDS20p12 rounds away again downstream. */
    for (row = 0u; row < 4u; row++)
    {
        u32 e0 = (u32)(src->m[row][0] << 4);
        u32 e1 = (u32)(src->m[row][1] << 4);
        u32 e2 = (u32)(src->m[row][2] << 4);
        u32 e3 = (u32)(src->m[row][3] << 4);

        integral[(row * 2u) + 0u] = (s32)COMBINE_INTEGRAL(e0, e1);
        fractional[(row * 2u) + 0u] = (s32)COMBINE_FRACTIONAL(e0, e1);
        integral[(row * 2u) + 1u] = (s32)COMBINE_INTEGRAL(e2, e3);
        fractional[(row * 2u) + 1u] = (s32)COMBINE_FRACTIONAL(e2, e3);
    }
}

static void ndsCameraFixedPublishPersp(CObj *cobj, f32 scale, Mtx *project_mtx,
                                       NDSRendererMatrix20p12 *persp_q)
{
    ndsR2CameraPerspFast20p12(persp_q, &cobj->projection.persp.norm,
                              cobj->projection.persp.fovy,
                              cobj->projection.persp.aspect,
                              cobj->projection.persp.near,
                              cobj->projection.persp.far,
                              scale);
    if (project_mtx == NULL)
    {
        NDS_DIAG(gNdsCameraMatrixLeanSkippedProjectCount++);
    }
    else
    {
        ndsCameraStoreQAsMtx(persp_q, project_mtx);
        sGCMatrixProjectL = project_mtx;
    }
}

/* W3s in Q20.12: the same five non-zero perspective terms, the same twenty
 * products and four sums, the same left-to-right association. */
static void ndsCameraCatQ(const NDSRendererMatrix20p12 *look_at,
                          const NDSRendererMatrix20p12 *persp,
                          NDSRendererMatrix20p12 *out)
{
    const s32 p00 = persp->m[0][0];
    const s32 p11 = persp->m[1][1];
    const s32 p22 = persp->m[2][2];
    const s32 p23 = persp->m[2][3];
    const s32 p32 = persp->m[3][2];
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        const s32 l0 = look_at->m[i][0];
        const s32 l1 = look_at->m[i][1];
        const s32 l2 = look_at->m[i][2];
        const s32 l3 = look_at->m[i][3];

        out->m[i][0] = ndsR2CamMulQSat(l0, p00);
        out->m[i][1] = ndsR2CamMulQSat(l1, p11);
        out->m[i][2] = ndsR2CamMulQSat(l2, p22) + ndsR2CamMulQSat(l3, p32);
        out->m[i][3] = ndsR2CamMulQSat(l2, p23);
    }
}

sb32 gmCameraLookAtFuncMatrix(Mtx *mtx, CObj *cobj, Gfx **dls)
{
    NDSRendererMatrix20p12 look_at_q;
    NDSRendererMatrix20p12 persp_q;
    NDSRendererMatrix20p12 camera_q;
    Mtx *temp_mtx = NULL;
    s32 max_q;
    u32 i;
    u32 j;

    NDS_DIAG(gNdsR2CameraFixedGameCalls++);

    (void)dls;
    syMatrixAdvanceW(temp_mtx, gSYTaskmanGraphicsHeap);
    ndsCameraFixedPublishPersp(cobj, cobj->projection.persp.scale, temp_mtx,
                               &persp_q);
    ndsR2CameraLookAtReflect20p12(&look_at_q, &gGMCameraStruct.look_at,
                                  cobj->vec.eye.x, cobj->vec.eye.y,
                                  cobj->vec.eye.z, cobj->vec.at.x,
                                  cobj->vec.at.y, cobj->vec.at.z,
                                  cobj->vec.up.x, cobj->vec.up.y,
                                  cobj->vec.up.z);
    ndsCameraCatQ(&look_at_q, &persp_q, &camera_q);

    max_q = 0;
    for (i = 0u; i < 4u; i++)
    {
        for (j = 0u; j < 4u; j++)
        {
            s32 magnitude = camera_q.m[i][j];

            if (magnitude < 0)
            {
                magnitude = -magnitude;
            }
            if (magnitude > max_q)
            {
                max_q = magnitude;
            }
        }
    }
    if (max_q > (32000 << NDS_R2_CAM_Q))
    {
        NDS_DIAG(gNdsR2CameraFixedRescaleCount++);
        NDS_DIAG(gNdsCameraMatrixLeanRescaleCount++);
        ndsCameraFixedPublishPersp(
            cobj,
            ndsR2CamQToF32(ndsR2CamDivQ(32000 << NDS_R2_CAM_Q, max_q)),
            temp_mtx, &persp_q);
        /* W1: the look-at arguments have not changed, so it is not recomputed. */
        ndsCameraCatQ(&look_at_q, &persp_q, &camera_q);
    }
    /* gGMCameraMatrix stays f32 because its four readers are f32 and all four
     * are DRAW side -- ifCommonPlayerTagProcDisplay, ndsBaseFTDisplayMainProcDisplay,
     * ndsIFCommonNativeOamBeginFrame and
     * ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame, enumerated from
     * the linked image's literal pools rather than from grep.  There is no
     * simulation reader, which is why converting this producer cannot move
     * gameplay state. */
    for (i = 0u; i < 4u; i++)
    {
        for (j = 0u; j < 4u; j++)
        {
            gGMCameraMatrix[i][j] = ndsR2CamQToF32(camera_q.m[i][j]);
        }
    }
    /* W2 */
    if (mtx != NULL)
    {
        ndsCameraStoreQAsMtx(&camera_q, mtx);
    }
    else
    {
        NDS_DIAG(gNdsCameraMatrixLeanSkippedF2LCount++);
    }
    return 0;
}
