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
 *
 * -- with one qualification since NDS_R2_CFX_HWMATH landed. The two hooks the
 * header has carried undefined since it was written, NDS_R2_CFX_DIV64 and
 * NDS_R2_CFX_ISQRT64, are now bound to the ARM9 divide and square-root
 * coprocessors here, so the kernels touch shared MMIO state and are no longer
 * pure in the strict sense. What that costs is set out in
 * include/nds/nds_r2_hwmath_unit.h: this binary has no interrupt-context user
 * of either unit -- the port's one registered handler is a counter increment --
 * so a mainline sequence cannot be interleaved, and the property the purity was
 * buying (no protocol needed) still holds for the same reason it did before.
 * It is now a measured property of the link rather than a structural one, which
 * is a real downgrade and is why it is written down here and not only there.
 *
 * The hooks are bound in THIS translation unit and nowhere else, deliberately:
 * scripts/check-r2-collision-fixed.c compiles the same header on the host and
 * must keep the portable default, and every executing instance of a
 * divide-using kernel is instantiated out-of-line below.
 */

/* NDS_R2_CFX_HWMATH arrives through the Makefile's global
 * `-include $(BUILD)/nds_build_config.h` (Makefile:2382), the same way
 * NDS_R2_COLLISION_FIXED reaches src/import/battleship_gmcollision.c. */
#if NDS_R2_CFX_HWMATH
#include <nds/nds_r2_hwmath_unit.h>
#define NDS_R2_CFX_DIV64(numerator, denominator) \
    ndsR2HwMathCfxDiv64((int64_t)(numerator), (int64_t)(denominator))
#define NDS_R2_CFX_ISQRT64(value) ndsR2HwMathCfxIsqrt64(value)
/* THE THIRD HOOK, and it was not in the brief. include/nds/nds_r2_collision_mtx.h:361
 * carries its own never-defined NDS_R2_COLLISION_DIV64 with the same "the port
 * overrides this with the DS hardware divider" comment, and it is the divide
 * inside ndsR2CollisionInvertMatrix44 -- i.e. inside ndsR2CollisionFixedInvertF32,
 * which is one of the four f32-boundary entry points the wired ring actually
 * calls (EXCHANGE.md section 3.1 lists InvertF32 among the ring rows). Binding
 * only the two CFX hooks left a `bl __aeabi_ldivmod` in this object, found by
 * disassembling it rather than by reading the brief. That header is included by
 * nds_r2_collision_fixed.h, so defining the macro here reaches it. */
#define NDS_R2_COLLISION_DIV64(numerator, denominator) \
    ndsR2HwMathCfxDiv64((int64_t)(numerator), (int64_t)(denominator))
#endif

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

/* The f32 boundary. These four are the ONLY entry points the wired ring calls,
 * and they are here rather than inlined into the ring so that every 64-bit
 * product in the cluster stays inside the one object the Makefile builds -marm
 * and check-r2-collision-fixed.ps1 disassembles. A copy inlined into a Thumb
 * translation unit would be __aeabi_lmul per multiply and no gate would notice.
 *
 * ndsR2CollisionFixedInvertF32 is nds_r2_collision_mtx.h's already-graded
 * ndsR2CollisionInvertMatrix44 -- gmCollisionSetInvertMatrix's own cofactor
 * arithmetic at Q26, float in and float out, so unk_dobjtrans_0x9C keeps its
 * type, its layout and all nine of its readers. It is not restated here for the
 * same reason the sine table is not: a transcription proves the transcription.
 */
int ndsR2CollisionFixedLoadF32(NDSR2CfxMtx *dst, float src[4][4])
{
    return ndsR2CfxLoadF32(dst, src);
}

void ndsR2CollisionFixedStoreF32(float dst[4][4], const NDSR2CfxMtx *src)
{
    ndsR2CfxStoreF32(dst, src);
}

int ndsR2CollisionFixedAxisScalesF32(float out[3], float src[4][4])
{
    return ndsR2CfxAxisScalesF32(out, src);
}

int ndsR2CollisionFixedInvertF32(float dst[4][4], float src[4][4])
{
    /* The cast is the C array-qualifier wart, not a const violation: `float
     * (*)[4]` does not implicitly convert to `const float (*)[4]`, and every
     * caller here hands over an FTParts field it does not own. Taking the
     * parameter non-const keeps that cast to exactly one place. */
    return ndsR2CollisionInvertMatrix44(dst, (const float (*)[4])src);
}
