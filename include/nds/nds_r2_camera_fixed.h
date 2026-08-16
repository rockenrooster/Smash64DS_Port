#ifndef NDS_R2_CAMERA_FIXED_H
#define NDS_R2_CAMERA_FIXED_H

/* Q20.12 camera + projection chain.
 *
 * The draw-side soft-float classification
 * (artifacts/performance/2026-08-15_drawside-softfloat/DRAW_FIXEDPOINT.md)
 * priced the whole draw-only + draw+dispatch float bill at 34,178 tk/fr, flat
 * at 1.12x, and falsified the collision lane's ~1.00 exchange rate for it with
 * an in-binary same-operation pair: `guMtxCatF` 2,921 tk/call against
 * `ndsRendererMtxMul20p12` 568.40 tk/call, RATIO 5.14x. This header is the
 * falsifier for that ratio: it converts the camera + projection chain only, so
 * the 5.14x measured on one function pair becomes an in-situ lane rate on real
 * frames.
 *
 * TWO PRODUCERS, ONE KERNEL PAIR. Entry-PC counts on the marginal-80 mask of
 * `builds/build-c200-trackprof-off` (exact, not sampled):
 *
 *   gmCameraLookAtFuncMatrix      2.000/fr  -> persp, look-at-reflect, concat
 *   syMatrixLookAtReflect         2.000/fr  -> renderer adapter camera
 *   syMatrixPerspFast             2.138/fr  -> renderer adapter projection
 *   syMatrixF2L                   6.138/fr  = 2.000 + 2.138 + 2.000, ALL of it
 *
 * so converting both producers DELETES every syMatrixF2L entry in the match
 * rather than converting it -- the renderer's consumer is
 * `NDSRendererMatrix20p12` and the game camera's is a float global written back
 * from the fixed result.
 *
 * OUTPUT PRECISION IS UNCHANGED FOR THE RENDERER ARM. Today the renderer runs
 * float -> syMatrixF2L (s15.16) -> ndsRendererMtxLoadN64ToDS20p12 (>>4, round
 * to nearest) -> Q20.12. The value the hardware sees already carries exactly 12
 * fractional bits. A Q20.12 kernel lands in the same representable set; only
 * the INTERMEDIATE rounding differs.
 *
 * ROUTE, NOT #if. gNdsR2CameraFixedEnabled is a `.data` word so one binary can
 * be poked through both arms with byte-identical placement
 * (`-SetGlobals gNdsR2CameraFixedEnabled=0|1`). It carries an explicit section
 * attribute because a zero-initialised route word lands in `.bss` and shifts
 * every later `.data` object, which is how a previous same-binary pair acquired
 * a ~10,000 tk/fr placement floor.
 *
 * THE DIVIDE AND THE ROOT ARE THE DS HARDWARE UNITS, never a library 64-bit
 * divide: `__udivmoddi4` already runs 11.70 times/frame at 2,909 tk/fr and the
 * collision ring's +17,377 at rank-80 came from adding to exactly that. libnds
 * `div64`/`sqrt64`/`divf32` are static inline wrappers over 0x04000280 /
 * 0x040002B0, so they add no call and no code. Those registers are ONE shared
 * set: every call site here runs in the frame loop, never from an interrupt.
 */

#include <nds/nds_renderer.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 0 = the shipped float chain, 1 = Q20.12. */
extern volatile u32 gNdsR2CameraFixedEnabled;

/* Engagement counters. A route that silently never fires is indistinguishable
 * from a route that fired and saved nothing, and this campaign has shipped that
 * mistake. Every one of these is written by live code, so --gc-sections keeps
 * it; each also carries `used` rather than relying on that. */
extern volatile u32 gNdsR2CameraFixedLookAtCalls;
extern volatile u32 gNdsR2CameraFixedPerspCalls;
extern volatile u32 gNdsR2CameraFixedFloatLookAtCalls;
extern volatile u32 gNdsR2CameraFixedFloatPerspCalls;
extern volatile u32 gNdsR2CameraFixedGameCalls;
extern volatile u32 gNdsR2CameraFixedGameFloatCalls;
/* Must read hard zero. Saturate = a value outside Q20.12's +/-524,288;
 * degenerate = a zero-length vector the float form would have divided by. */
extern volatile u32 gNdsR2CameraFixedSaturateCount;
extern volatile u32 gNdsR2CameraFixedDegenerateCount;
/* The >32000 rescale pass. 0 in this match on the float arm (ndsCameraCatCamera
 * is entered exactly 2.000/frame against gmCameraLookAtFuncMatrix's 2.000), so
 * a non-zero reading on the fixed arm is a divergence, not a cold path. */
extern volatile u32 gNdsR2CameraFixedRescaleCount;

/* Q20.12 look-at with reflectance directions. `l` may be NULL: the renderer's
 * three call sites pass a local LookAt that nothing reads, so the six
 * FTOFRAC8 conversions are skipped there rather than computed and dropped. */
void ndsR2CameraLookAtReflect20p12(NDSRendererMatrix20p12 *out, LookAt *l,
                                   f32 eye_x, f32 eye_y, f32 eye_z,
                                   f32 at_x, f32 at_y, f32 at_z,
                                   f32 up_x, f32 up_y, f32 up_z);

/* Q20.12 fast perspective. `persp_norm` may be NULL, exactly as decomp's. */
void ndsR2CameraPerspFast20p12(NDSRendererMatrix20p12 *out, u16 *persp_norm,
                               f32 fovy, f32 aspect, f32 near, f32 far,
                               f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* NDS_R2_CAMERA_FIXED_H */
