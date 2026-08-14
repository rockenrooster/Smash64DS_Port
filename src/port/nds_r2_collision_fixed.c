/* Out-of-line ARM entry points for the whole-cluster fixed-point fighter
 * hurtbox narrow phase. The arithmetic is in include/nds/nds_r2_collision_fixed.h
 * so the host falsifier grades the code that ships; this file exists to give it
 * addresses and, above all, to give it ARM STATE.
 *
 * Why a separate translation unit rather than inlining into
 * src/import/battleship_gmcollision.c: that TU builds -mthumb, ARMv5TE Thumb
 * has no SMULL, and every (int64)a * b in this kernel would become a call to
 * __aeabi_lmul. Memory "Thumb hides 64-bit cost" is the record of what that
 * costs -- a pure-precision change measured +36,032 P95 until one target("arm")
 * attribute turned it into -71,616 -- and Makefile:3466 records the same thing
 * for the renderer, where a single missing -marm was worth 511,174 ticks per
 * Results tic. So the Makefile builds THIS object -marm and the check script
 * fails the build if any __aeabi_lmul or soft-float call survives in it.
 *
 * Do not read Makefile:3480's "R2-07 L7a REFUTED" as forbidding that. L7a
 * measured recompiling gmcollision.c -marm while its arithmetic was still
 * FLOAT, and the comment states exactly why it could not pay: f32 helpers are
 * libgcc code whose own mode the caller's flag does not change, so -marm could
 * only buy the call sites. That reasoning applies to float code and to nothing
 * else. Here the arithmetic is 64-bit integer, which is the one case where the
 * flag changes the instruction selected rather than the calling convention
 * around it.
 *
 * NOTHING CALLS THESE YET, by design: this is a compiled, unit-proven,
 * unreferenced subset. --gc-sections drops the lot, so the cycle that lands it
 * costs zero bytes of .text and zero of the taskman arena. Wiring them --
 * deleting the float bodies, not wrapping them -- is a separate cycle with its
 * own map check and gate A/B.
 *
 * State: none. Every entry point below is a pure function of its arguments, so
 * charter 3.11 (no gameplay-time allocation) and 3.12 (nothing keyed on a
 * pointer that survives a scene boundary) are satisfied by construction rather
 * than by a guard.
 */

#include <nds/nds_r2_collision_fixed.h>

int ndsR2CollisionFixedBuildLocal(NDSR2CfxMtx *dst, const uint16_t *sin_table,
                                  const float rotate[3], const float scale[3],
                                  const float translate[3])
{
    return ndsR2CfxBuildLocal(dst, sin_table, rotate, scale, translate);
}

int ndsR2CollisionFixedCompose(NDSR2CfxMtx *dst, const NDSR2CfxMtx *lhs,
                               const NDSR2CfxMtx *rhs)
{
    return ndsR2CfxCompose(dst, lhs, rhs);
}

/* The cofactor form is the one that ships, and it is NOT the one Phase 4
 * proposed. The row-scaled near-orthogonal inverse was measured at 0.1035 world
 * units against a 0.0200 bound and is deliberately NOT exported here, so no
 * later cycle can wire it by reaching for the obvious name; it stays in the
 * header as a static inline the falsifier keeps grading, beside the row-skew
 * measurement that explains why it lost. */
int ndsR2CollisionFixedMakeFrame(NDSR2CfxFrame *dst, const NDSR2CfxMtx *src)
{
    return ndsR2CfxMakeFrameCofactor(dst, src);
}

void ndsR2CollisionFixedWorldToLocal(int32_t out[3],
                                     const NDSR2CfxFrame *frame,
                                     const int32_t point[3])
{
    ndsR2CfxWorldToLocal(out, frame, point);
}

void ndsR2CollisionFixedTransformPoint(int32_t out[3], const NDSR2CfxMtx *mtx,
                                       const int32_t point[3])
{
    ndsR2CfxTransformPoint(out, mtx, point);
}

int ndsR2CollisionFixedTestRectangle(const int32_t pos_curr[3],
                                     const int32_t pos_prev[3], int32_t radius,
                                     int is_transfer,
                                     const NDSR2CfxFrame *frame,
                                     const int32_t offset[3],
                                     const int32_t size[3],
                                     const int32_t inv_scale[3])
{
    return ndsR2CfxTestRectangle(pos_curr, pos_prev, radius, is_transfer, frame,
                                 offset, size, inv_scale);
}
