/* Falsifier for the whole-cluster fixed-point fighter hurtbox narrow phase.
 *
 * Compiles include/nds/nds_r2_collision_fixed.h -- the code that will ship --
 * and grades every kernel in it against a transcription of the decomp float
 * originals in gm/gmcollision.c, over the domain the game actually visits.
 *
 * The bar is not bit-exactness. 20.12/6.26 cannot reproduce f32 and
 * PROJECT_GOAL.md does not ask it to; it asks for mechanical equivalence. The
 * bar is the E64b/E65 bound R2-07 L7 already carries: 0.0200 world units of
 * error on a transformed point, because a position is what collision reads and
 * a matrix-cell bound cannot be compared against a hurtbox.
 *
 * Structure, following the campaign's precedent of splitting a change into an
 * enumerable half proven exhaustively and a bounded half proven with density:
 *
 *   ENUMERATED   the sine table (all 4,096 indices)
 *                the integer square root over the live s^2 band, every raw
 *                value, plus 0..2^20 exhaustively
 *   BOUNDED      the matrix stages, over the measured live scale domain and
 *                two wider ones, with the adversarial corners called out
 *   DIFFERENTIAL the rectangle test's DECISION, float against fixed, with the
 *                perturbation margin of every case measured so the flip rate
 *                can be read against the error bound rather than asserted
 *
 * The live scale domain is not assumed: NDS_R2_COLLISION_L7_ORACLE read it off
 * a natural mode-163 match on 2026-07-31 -- 460 samples, every one a joint
 * collision actually inverted that frame -- and got 1.1138 to 1.1199, a single
 * scale spanning 0.006. include/nds/nds_r2_collision_mtx.h carries the record.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/nds/nds_r2_collision_fixed.h"

/* gSYSinTable, the game's own sine table, read from the decomp rather than
 * transcribed. A falsifier that grades a copy of the table proves the copy.
 * sintable.c pulls PR/ultratypes.h, which -Iinclude resolves to the PORT's
 * stdint-backed copy -- the same one the ROM compiles it against. */
#include "../decomp/BattleShip-main/decomp/src/sys/sintable.c"

typedef float Mtx44f[4][4];

#define BOUND 0.0200
#define SINTABLE_COUNT 0x800

/* Live domain, from the L7 oracle. */
#define LIVE_SCALE_LO 1.1138
#define LIVE_SCALE_HI 1.1199

static int g_fail = 0;

static void Verdict(const char *label, double worst, double bound, long cases,
                    int gated)
{
    const char *state;

    if (!gated)
    {
        state = "(reported)";
    }
    else if (worst > bound)
    {
        state = "RED";
        g_fail = 1;
    }
    else
    {
        state = "green";
    }
    printf("  %-46s %12ld %12.7f %9.4f  %s\n", label, cases, worst, bound,
           state);
}

/* ---- the decomp originals, transcribed ------------------------------- */

/* lb/lbcommon.c:321 as the port ships it
 * (src/port/reloc_backend_compat_shims.c:13519). */
static float FloatSin(float angle)
{
    int32_t index = ((int32_t)(angle * 651.8986206f)) & 0xFFF;
    float sin = (float)gSYSinTable[index & (SINTABLE_COUNT - 1)] *
                (1.0f / 32768.0f);

    return (index & 0x800) ? -sin : sin;
}

static float FloatCos(float angle)
{
    int32_t index =
        ((int32_t)((angle + 1.5707963268f) * 651.8986206f)) & 0xFFF;
    float cos = (float)gSYSinTable[index & (SINTABLE_COUNT - 1)] *
                (1.0f / 32768.0f);

    return (index & 0x800) ? -cos : cos;
}

/* gm/gmcollision.c:29 */
static void FloatBuildLocal(Mtx44f mtx, const float rotate[3],
                            const float scale[3], const float translate[3])
{
    float sinx = FloatSin(rotate[0]), cosx = FloatCos(rotate[0]);
    float siny = FloatSin(rotate[1]), cosy = FloatCos(rotate[1]);
    float sinz = FloatSin(rotate[2]), cosz = FloatCos(rotate[2]);

    mtx[0][0] = cosy * cosz;
    mtx[0][1] = cosy * sinz;
    mtx[0][2] = -siny;

    mtx[1][0] = (sinx * siny * cosz) - (cosx * sinz);
    mtx[1][1] = (sinx * siny * sinz) + (cosx * cosz);
    mtx[1][2] = sinx * cosy;

    mtx[2][0] = (cosx * siny * cosz) + (sinx * sinz);
    mtx[2][1] = (cosx * siny * sinz) - (sinx * cosz);
    mtx[2][2] = cosx * cosy;

    if (scale[0] != 1.0f)
    {
        mtx[0][0] *= scale[0]; mtx[0][1] *= scale[0]; mtx[0][2] *= scale[0];
    }
    if (scale[1] != 1.0f)
    {
        mtx[1][0] *= scale[1]; mtx[1][1] *= scale[1]; mtx[1][2] *= scale[1];
    }
    if (scale[2] != 1.0f)
    {
        mtx[2][0] *= scale[2]; mtx[2][1] *= scale[2]; mtx[2][2] *= scale[2];
    }
    mtx[3][0] = translate[0];
    mtx[3][1] = translate[1];
    mtx[3][2] = translate[2];
    mtx[0][3] = mtx[1][3] = mtx[2][3] = 0.0f;
    mtx[3][3] = 1.0f;
}

/* gm/gmcollision.c:208 */
static void FloatCompose(Mtx44f dst, Mtx44f lhs, Mtx44f rhs)
{
    dst[0][0] = (lhs[0][0] * rhs[0][0]) + (lhs[1][0] * rhs[0][1]) + (lhs[2][0] * rhs[0][2]);
    dst[0][1] = (lhs[0][1] * rhs[0][0]) + (lhs[1][1] * rhs[0][1]) + (lhs[2][1] * rhs[0][2]);
    dst[0][2] = (lhs[0][2] * rhs[0][0]) + (lhs[1][2] * rhs[0][1]) + (lhs[2][2] * rhs[0][2]);

    dst[1][0] = (lhs[0][0] * rhs[1][0]) + (lhs[1][0] * rhs[1][1]) + (lhs[2][0] * rhs[1][2]);
    dst[1][1] = (lhs[0][1] * rhs[1][0]) + (lhs[1][1] * rhs[1][1]) + (lhs[2][1] * rhs[1][2]);
    dst[1][2] = (lhs[0][2] * rhs[1][0]) + (lhs[1][2] * rhs[1][1]) + (lhs[2][2] * rhs[1][2]);

    dst[2][0] = (lhs[0][0] * rhs[2][0]) + (lhs[1][0] * rhs[2][1]) + (lhs[2][0] * rhs[2][2]);
    dst[2][1] = (lhs[0][1] * rhs[2][0]) + (lhs[1][1] * rhs[2][1]) + (lhs[2][1] * rhs[2][2]);
    dst[2][2] = (lhs[0][2] * rhs[2][0]) + (lhs[1][2] * rhs[2][1]) + (lhs[2][2] * rhs[2][2]);

    dst[3][0] = ((lhs[0][0] * rhs[3][0]) + (lhs[1][0] * rhs[3][1]) + (lhs[2][0] * rhs[3][2])) + lhs[3][0];
    dst[3][1] = ((lhs[0][1] * rhs[3][0]) + (lhs[1][1] * rhs[3][1]) + (lhs[2][1] * rhs[3][2])) + lhs[3][1];
    dst[3][2] = ((lhs[0][2] * rhs[3][0]) + (lhs[1][2] * rhs[3][1]) + (lhs[2][2] * rhs[3][2])) + lhs[3][2];
    dst[0][3] = dst[1][3] = dst[2][3] = 0.0f;
    dst[3][3] = 1.0f;
}

/* gm/gmcollision.c:228. Returns 0 where the source would spin on a zero
 * determinant, so the harness skips instead of hanging. */
static int FloatInvert(Mtx44f dst, Mtx44f src)
{
    float scale;

    dst[0][0] = (src[1][1] * src[2][2]) - (src[1][2] * src[2][1]);
    dst[1][0] = (src[1][0] * src[2][2]) - (src[1][2] * src[2][0]);
    dst[2][0] = (src[1][0] * src[2][1]) - (src[1][1] * src[2][0]);
    dst[3][0] = (src[3][0] * dst[0][0]) - (src[3][1] * dst[1][0]) + (src[3][2] * dst[2][0]);

    dst[0][1] = (src[0][1] * src[2][2]) - (src[0][2] * src[2][1]);
    dst[1][1] = (src[0][0] * src[2][2]) - (src[0][2] * src[2][0]);
    dst[2][1] = (src[0][0] * src[2][1]) - (src[0][1] * src[2][0]);
    dst[3][1] = (src[3][0] * dst[0][1]) - (src[3][1] * dst[1][1]) + (src[3][2] * dst[2][1]);

    dst[0][2] = (src[0][1] * src[1][2]) - (src[0][2] * src[1][1]);
    dst[1][2] = (src[0][0] * src[1][2]) - (src[0][2] * src[1][0]);
    dst[2][2] = (src[0][0] * src[1][1]) - (src[0][1] * src[1][0]);
    dst[3][2] = (src[3][0] * dst[0][2]) - (src[3][1] * dst[1][2]) + (src[3][2] * dst[2][2]);

    scale = (src[0][0] * dst[0][0]) - (src[0][1] * dst[1][0]) + (src[0][2] * dst[2][0]);

    dst[1][0] = -dst[1][0]; dst[3][0] = -dst[3][0];
    dst[0][1] = -dst[0][1]; dst[2][1] = -dst[2][1];
    dst[1][2] = -dst[1][2]; dst[3][2] = -dst[3][2];

    if (scale == 0.0f) { return 0; }
    scale = 1.0f / scale;

    dst[0][0] *= scale; dst[1][0] *= scale; dst[2][0] *= scale; dst[3][0] *= scale;
    dst[0][1] *= scale; dst[1][1] *= scale; dst[2][1] *= scale; dst[3][1] *= scale;
    dst[0][2] *= scale; dst[1][2] *= scale; dst[2][2] *= scale; dst[3][2] *= scale;
    dst[0][3] = dst[1][3] = dst[2][3] = 0.0f;
    dst[3][3] = 1.0f;
    return 1;
}

/* gm/gmcollision.c:196 */
static void FloatWorldPosition(Mtx44f mtx, float vec[3])
{
    float x = ((mtx[0][0] * vec[0]) + (mtx[1][0] * vec[1]) + (mtx[2][0] * vec[2])) + mtx[3][0];
    float y = ((mtx[0][1] * vec[0]) + (mtx[1][1] * vec[1]) + (mtx[2][1] * vec[2])) + mtx[3][1];
    float z = ((mtx[0][2] * vec[0]) + (mtx[1][2] * vec[1]) + (mtx[2][2] * vec[2])) + mtx[3][2];

    vec[0] = x; vec[1] = y; vec[2] = z;
}

/* gm/gmcollision.c:472 */
static void FloatAxisScales(Mtx44f mtx, float out[3])
{
    int r;

    for (r = 0; r < 3; r++)
    {
        out[r] = sqrtf(mtx[r][0] * mtx[r][0] + mtx[r][1] * mtx[r][1] +
                       mtx[r][2] * mtx[r][2]);
    }
}

/* ---- the same chain in double, as an ATTRIBUTION reference -------------
 *
 * Not a third implementation to be maintained: it exists so a row that is over
 * bound can be split into "the kernel is wrong" and "the f32 reference is",
 * which at |t| in the thousands is not a hypothetical -- ulp(32767) is 0.00195
 * and each compose spends several of them. Table values are the game's own,
 * read as doubles, so the only difference from the float path is the width of
 * the arithmetic. */
typedef double Mtx43d[4][3];

static double ExactSin(float angle)
{
    int32_t index = ((int32_t)(angle * 651.8986206f)) & 0xFFF;
    double s = (double)gSYSinTable[index & (SINTABLE_COUNT - 1)] / 32768.0;

    return (index & 0x800) ? -s : s;
}

static double ExactCos(float angle)
{
    int32_t index =
        ((int32_t)((angle + 1.5707963268f) * 651.8986206f)) & 0xFFF;
    double c = (double)gSYSinTable[index & (SINTABLE_COUNT - 1)] / 32768.0;

    return (index & 0x800) ? -c : c;
}

static void ExactBuildLocal(Mtx43d m, const float rotate[3],
                            const float scale[3], const float translate[3])
{
    double sinx = ExactSin(rotate[0]), cosx = ExactCos(rotate[0]);
    double siny = ExactSin(rotate[1]), cosy = ExactCos(rotate[1]);
    double sinz = ExactSin(rotate[2]), cosz = ExactCos(rotate[2]);
    int c;

    m[0][0] = cosy * cosz;
    m[0][1] = cosy * sinz;
    m[0][2] = -siny;
    m[1][0] = (sinx * siny * cosz) - (cosx * sinz);
    m[1][1] = (sinx * siny * sinz) + (cosx * cosz);
    m[1][2] = sinx * cosy;
    m[2][0] = (cosx * siny * cosz) + (sinx * sinz);
    m[2][1] = (cosx * siny * sinz) - (sinx * cosz);
    m[2][2] = cosx * cosy;

    for (c = 0; c < 3; c++)
    {
        m[0][c] *= (double)scale[0];
        m[1][c] *= (double)scale[1];
        m[2][c] *= (double)scale[2];
        m[3][c] = (double)translate[c];
    }
}

static void ExactCompose(Mtx43d dst, Mtx43d lhs, Mtx43d rhs)
{
    Mtx43d out;
    int r, c;

    for (r = 0; r < 3; r++)
    {
        for (c = 0; c < 3; c++)
        {
            out[r][c] = lhs[0][c] * rhs[r][0] + lhs[1][c] * rhs[r][1] +
                        lhs[2][c] * rhs[r][2];
        }
    }
    for (c = 0; c < 3; c++)
    {
        out[3][c] = lhs[0][c] * rhs[3][0] + lhs[1][c] * rhs[3][1] +
                    lhs[2][c] * rhs[3][2] + lhs[3][c];
    }
    memcpy(dst, out, sizeof(Mtx43d));
}

static int ExactInvert(Mtx43d dst, Mtx43d s)
{
    double c00 = s[1][1] * s[2][2] - s[1][2] * s[2][1];
    double c10 = s[1][0] * s[2][2] - s[1][2] * s[2][0];
    double c20 = s[1][0] * s[2][1] - s[1][1] * s[2][0];
    double det = s[0][0] * c00 - s[0][1] * c10 + s[0][2] * c20;
    double inv;
    int c;

    if (det == 0.0) { return 0; }
    inv = 1.0 / det;

    dst[0][0] = c00 * inv;
    dst[1][0] = -c10 * inv;
    dst[2][0] = c20 * inv;
    dst[0][1] = -(s[0][1] * s[2][2] - s[0][2] * s[2][1]) * inv;
    dst[1][1] = (s[0][0] * s[2][2] - s[0][2] * s[2][0]) * inv;
    dst[2][1] = -(s[0][0] * s[2][1] - s[0][1] * s[2][0]) * inv;
    dst[0][2] = (s[0][1] * s[1][2] - s[0][2] * s[1][1]) * inv;
    dst[1][2] = -(s[0][0] * s[1][2] - s[0][2] * s[1][0]) * inv;
    dst[2][2] = (s[0][0] * s[1][1] - s[0][1] * s[1][0]) * inv;

    for (c = 0; c < 3; c++)
    {
        dst[3][c] = -(s[3][0] * dst[0][c] + s[3][1] * dst[1][c] +
                      s[3][2] * dst[2][c]);
    }
    return 1;
}

static void ApplyD(Mtx43d m, const double p[3], double out[3])
{
    int c;

    for (c = 0; c < 3; c++)
    {
        out[c] = m[0][c] * p[0] + m[1][c] * p[1] + m[2][c] * p[2] + m[3][c];
    }
}

static void FloatToD(Mtx43d out, Mtx44f in)
{
    int r, c;

    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 3; c++) { out[r][c] = (double)in[r][c]; }
    }
}

static double MaxDiff3(const double a[3], const double b[3])
{
    double worst = 0.0;
    int c;

    for (c = 0; c < 3; c++)
    {
        double d = fabs(a[c] - b[c]);

        if (d > worst) { worst = d; }
    }
    return worst;
}

/* gm/gmcollision.c:621 / :645 */
static unsigned FloatOutcodeXY(const float p[3], const float c[3])
{
    unsigned flags = 0u;

    if (p[0] < -c[0]) { flags |= 1u; }
    if (p[0] > c[0]) { flags |= 2u; }
    if (p[1] < -c[1]) { flags |= 4u; }
    if (p[1] > c[1]) { flags |= 8u; }
    return flags;
}

static unsigned FloatOutcodeZ(const float p[3], const float c[3])
{
    unsigned flags = 0u;

    if (p[2] < -c[2]) { flags |= 1u; }
    if (p[2] > c[2]) { flags |= 2u; }
    return flags;
}

/* gm/gmcollision.c:661. `center_bias` is not in the source: it is the
 * perturbation the margin bisection applies to the half extents, and it is 0
 * for every non-margin call. */
static int FloatTestRectangle(const float pos_curr[3], const float pos_prev[3],
                              float radius, int is_transfer, Mtx44f mtx,
                              const float offset[3], const float size[3],
                              const float scale[3], float center_bias)
{
    float center[3];
    float a[3];
    float b[3];
    float clipped[3];
    float distx, disty, distz;
    unsigned flags_a, flags_b;
    int i;
    int guard;

    for (i = 0; i < 3; i++)
    {
        center[i] = size[i] + (radius / scale[i]) + center_bias;
    }
    if (is_transfer)
    {
        a[0] = pos_curr[0]; a[1] = pos_curr[1]; a[2] = pos_curr[2];
        if (mtx != NULL) { FloatWorldPosition(mtx, a); }
        for (i = 0; i < 3; i++) { a[i] -= offset[i]; }
        for (i = 0; i < 3; i++)
        {
            if ((-center[i] > a[i]) || (a[i] > center[i])) { return 0; }
        }
        return 1;
    }
    for (i = 0; i < 3; i++) { a[i] = pos_curr[i]; b[i] = pos_prev[i]; }
    if (mtx != NULL) { FloatWorldPosition(mtx, a); FloatWorldPosition(mtx, b); }
    for (i = 0; i < 3; i++) { a[i] -= offset[i]; b[i] -= offset[i]; }

    distx = b[0] - a[0];
    disty = b[1] - a[1];
    distz = b[2] - a[2];

    flags_a = FloatOutcodeXY(a, center);
    flags_b = FloatOutcodeXY(b, center);

    for (guard = 0; guard <= 8; guard++)
    {
        unsigned flags_main;

        if ((flags_a == 0u) && (flags_b == 0u)) { break; }
        if ((flags_a & flags_b) != 0u) { return 0; }
        if (guard == 8) { return NDS_R2_CFX_DECLINE; }
        flags_main = (flags_a != 0u) ? flags_a : flags_b;

        if (flags_main & 1u)
        {
            clipped[0] = -center[0];
            clipped[1] = (((clipped[0] - a[0]) / distx) * disty) + a[1];
            clipped[2] = (((clipped[0] - a[0]) / distx) * distz) + a[2];
        }
        else if (flags_main & 2u)
        {
            clipped[0] = center[0];
            clipped[1] = (((clipped[0] - a[0]) / distx) * disty) + a[1];
            clipped[2] = (((clipped[0] - a[0]) / distx) * distz) + a[2];
        }
        else if (flags_main & 4u)
        {
            clipped[1] = -center[1];
            clipped[0] = (((clipped[1] - a[1]) / disty) * distx) + a[0];
            clipped[2] = (((clipped[1] - a[1]) / disty) * distz) + a[2];
        }
        else
        {
            clipped[1] = center[1];
            clipped[0] = (((clipped[1] - a[1]) / disty) * distx) + a[0];
            clipped[2] = (((clipped[1] - a[1]) / disty) * distz) + a[2];
        }
        if (flags_main == flags_a)
        {
            a[0] = clipped[0]; a[1] = clipped[1]; a[2] = clipped[2];
            flags_a = FloatOutcodeXY(a, center);
        }
        else
        {
            b[0] = clipped[0]; b[1] = clipped[1]; b[2] = clipped[2];
            flags_b = FloatOutcodeXY(b, center);
        }
    }
    return ((FloatOutcodeZ(a, center) & FloatOutcodeZ(b, center)) != 0u) ? 0 : 1;
}

/* ---- conversion and error helpers ------------------------------------ */

static int32_t Q12(double v)
{
    double s = v * (double)NDS_R2_CFX_POS_ONE;

    return (int32_t)(s < 0.0 ? s - 0.5 : s + 0.5);
}

/* World-unit error of a fixed FRAME against the float inverse, on a point. */
static double FrameError(Mtx44f finv, const NDSR2CfxFrame *frame,
                         const double p[3])
{
    double worst = 0.0;
    int32_t px[3];
    int32_t out[3];
    int c;

    for (c = 0; c < 3; c++) { px[c] = Q12(p[c]); }
    ndsR2CfxWorldToLocal(out, frame, px);
    for (c = 0; c < 3; c++)
    {
        double fv = (double)finv[0][c] * p[0] + (double)finv[1][c] * p[1] +
                    (double)finv[2][c] * p[2] + (double)finv[3][c];
        double xv = (double)out[c] / (double)NDS_R2_CFX_POS_ONE;
        double d = fabs(fv - xv);

        if (d > worst) { worst = d; }
    }
    return worst;
}

static uint32_t rng_state = 0x13572468u;
static double Rand(double lo, double hi)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return lo + (hi - lo) * ((double)(rng_state >> 8) / 16777216.0);
}

static void ResetRng(void) { rng_state = 0x13572468u; }

/* ======================================================================
 * T1 -- the sine table, ENUMERATED
 */
static void TestSinTable(void)
{
    long cases = 0;
    double worst = 0.0;
    int32_t index;

    /* Every reachable index. The fixed form returns the raw table entry; the
     * float form returns that entry times 1/32768, which is exact in f32
     * because the entry is at most 2^15 and the factor is a power of two. So
     * the two must agree EXACTLY, not merely within a bound. */
    for (index = 0; index < 0x1000; index++)
    {
        int32_t q15 = ndsR2CfxTableQ15(gSYSinTable, index);
        double as_float = (double)q15 / 32768.0;
        int32_t raw = (int32_t)gSYSinTable[index & (SINTABLE_COUNT - 1)];
        double reference = (double)((index & 0x800) ? -raw : raw) / 32768.0;
        double d = fabs(as_float - reference);

        if (d > worst) { worst = d; }
        cases++;
    }
    Verdict("T1a sine table, all 4096 indices (exact)", worst, 0.0, cases, 1);

    /* The index arithmetic, over a dense angle sweep well past +/-2pi: the
     * fixed and float paths must land on the same table entry, including
     * cosine's add-90-to-the-ANGLE (which is not add-0x400-to-the-index). */
    {
        long mismatches = 0;
        int i;

        cases = 0;
        for (i = -400000; i <= 400000; i++)
        {
            float angle = (float)i * 0.0001f;
            double fs = (double)FloatSin(angle);
            double fc = (double)FloatCos(angle);
            double xs = (double)ndsR2CfxSinQ15(gSYSinTable, angle) / 32768.0;
            double xc = (double)ndsR2CfxCosQ15(gSYSinTable, angle) / 32768.0;

            if ((fs != xs) || (fc != xc)) { mismatches++; }
            cases++;
        }
        Verdict("T1b sin/cos index arithmetic (mismatches)",
                (double)mismatches, 0.0, cases, 1);
    }
}

/* ======================================================================
 * T2 -- the integer square root, ENUMERATED
 */
static void TestIsqrt(void)
{
    long cases = 0;
    long bad = 0;
    uint64_t v;
    int32_t lo;
    int32_t hi;
    int32_t raw;

    /* Exhaustive over 0 .. 2^20. */
    for (v = 0; v <= (1u << 20); v++)
    {
        uint64_t r = ndsR2CfxIsqrt64Portable(v);

        if ((r * r > v) || ((r + 1) * (r + 1) <= v)) { bad++; }
        cases++;
    }
    Verdict("T2a isqrt64 exhaustive 0..2^20 (failures)", (double)bad, 0.0,
            cases, 1);

    /* Exhaustive over every raw s^2 the LIVE scale domain can produce. This is
     * the domain the kernel actually indexes, and it is small enough to
     * enumerate rather than sample. */
    lo = (int32_t)(LIVE_SCALE_LO * LIVE_SCALE_LO * (double)NDS_R2_CFX_ROT_ONE);
    hi = (int32_t)(LIVE_SCALE_HI * LIVE_SCALE_HI * (double)NDS_R2_CFX_ROT_ONE) +
         1;
    bad = 0;
    cases = 0;
    for (raw = lo; raw <= hi; raw++)
    {
        uint64_t arg = (uint64_t)raw << 22;
        uint64_t r = ndsR2CfxIsqrt64Portable(arg);
        double exact = sqrt((double)raw / (double)NDS_R2_CFX_ROT_ONE);
        double got = (double)r / 16777216.0;

        if ((r * r > arg) || ((r + 1) * (r + 1) <= arg)) { bad++; }
        if (fabs(exact - got) > 1.0e-7) { bad++; }
        cases++;
    }
    Verdict("T2b isqrt64 over every live s^2 raw value (failures)",
            (double)bad, 0.0, cases, 1);

    /* Corners: the guard edges and the largest argument the kernel can form. */
    bad = 0;
    cases = 0;
    {
        static const uint64_t corners[] = {
            0u, 1u, 2u, 3u, 4u,
            (uint64_t)NDS_R2_CFX_S2_MIN << 22,
            (uint64_t)NDS_R2_CFX_S2_MAX << 22,
            ((uint64_t)1 << 52) - 1u, (uint64_t)1 << 52,
            ((uint64_t)1 << 61) - 1u,
        };
        size_t i;

        for (i = 0; i < sizeof(corners) / sizeof(corners[0]); i++)
        {
            uint64_t r = ndsR2CfxIsqrt64Portable(corners[i]);

            if ((r * r > corners[i]) ||
                ((r + 1) * (r + 1) <= corners[i])) { bad++; }
            cases++;
        }
    }
    Verdict("T2c isqrt64 guard corners (failures)", (double)bad, 0.0, cases, 1);
}

/* ======================================================================
 * The joint-chain model, shared by T3-T6.
 *
 * A fighter joint's world matrix is the root's local matrix composed with each
 * descendant's, and the L7 oracle measured the world rows at a single scale of
 * 1.114-1.120 -- so the chain is modelled as a scaled root and unit-scaled
 * children, which is what produces that. `depth` is the number of COMPOSES.
 */
typedef struct ChainCase
{
    Mtx44f fworld;
    Mtx43d eworld;
    NDSR2CfxMtx xworld;
    int declined;
} ChainCase;

static void BuildChain(ChainCase *out, double scale_lo, double scale_hi,
                       int depth, double world_extent)
{
    Mtx44f facc;
    Mtx43d eacc;
    NDSR2CfxMtx xacc;
    float rot[3];
    float scale[3];
    float trans[3];
    int level;

    out->declined = 0;

    rot[0] = (float)Rand(-3.15, 3.15);
    rot[1] = (float)Rand(-3.15, 3.15);
    rot[2] = (float)Rand(-3.15, 3.15);
    scale[0] = (float)Rand(scale_lo, scale_hi);
    scale[1] = (float)Rand(scale_lo, scale_hi);
    scale[2] = (float)Rand(scale_lo, scale_hi);
    trans[0] = (float)Rand(-world_extent, world_extent);
    trans[1] = (float)Rand(-world_extent, world_extent);
    trans[2] = (float)Rand(-world_extent, world_extent);

    FloatBuildLocal(facc, rot, scale, trans);
    ExactBuildLocal(eacc, rot, scale, trans);
    if (ndsR2CfxBuildLocal(&xacc, gSYSinTable, rot, scale, trans) == 0)
    {
        out->declined = 1;
        return;
    }

    for (level = 0; level < depth; level++)
    {
        Mtx44f flocal;
        Mtx44f fnext;
        Mtx43d elocal;
        NDSR2CfxMtx xlocal;
        NDSR2CfxMtx xnext;

        rot[0] = (float)Rand(-3.15, 3.15);
        rot[1] = (float)Rand(-3.15, 3.15);
        rot[2] = (float)Rand(-3.15, 3.15);
        scale[0] = scale[1] = scale[2] = 1.0f;
        trans[0] = (float)Rand(-30.0, 30.0);
        trans[1] = (float)Rand(-30.0, 30.0);
        trans[2] = (float)Rand(-30.0, 30.0);

        FloatBuildLocal(flocal, rot, scale, trans);
        ExactBuildLocal(elocal, rot, scale, trans);
        if (ndsR2CfxBuildLocal(&xlocal, gSYSinTable, rot, scale, trans) == 0)
        {
            out->declined = 1;
            return;
        }
        FloatCompose(fnext, facc, flocal);
        ExactCompose(eacc, eacc, elocal);
        if (ndsR2CfxCompose(&xnext, &xacc, &xlocal) == 0)
        {
            out->declined = 1;
            return;
        }
        memcpy(facc, fnext, sizeof(Mtx44f));
        xacc = xnext;
    }
    memcpy(out->fworld, facc, sizeof(Mtx44f));
    memcpy(out->eworld, eacc, sizeof(Mtx43d));
    out->xworld = xacc;
}

/* Apply the fixed forward matrix / frame and return the result in world units,
 * quantising the query point on the way in exactly as the runtime would. */
static void FixedApplyMtx(const NDSR2CfxMtx *m, const double p[3],
                          double out[3])
{
    int32_t px[3];
    int32_t xo[3];
    int c;

    for (c = 0; c < 3; c++) { px[c] = Q12(p[c]); }
    ndsR2CfxTransformPoint(xo, m, px);
    for (c = 0; c < 3; c++)
    {
        out[c] = (double)xo[c] / (double)NDS_R2_CFX_POS_ONE;
    }
}

static void FixedApplyFrame(const NDSR2CfxFrame *f, const double p[3],
                            double out[3])
{
    int32_t px[3];
    int32_t xo[3];
    int c;

    for (c = 0; c < 3; c++) { px[c] = Q12(p[c]); }
    ndsR2CfxWorldToLocal(xo, f, px);
    for (c = 0; c < 3; c++)
    {
        out[c] = (double)xo[c] / (double)NDS_R2_CFX_POS_ONE;
    }
}

/* ======================================================================
 * T3 -- the local matrix build and T4 -- the compose, as a chain
 */
static void TestChainForward(const char *label, double scale_lo,
                             double scale_hi, int depth, double world_extent,
                             double probe, int gated, int cases)
{
    double worst_vs_float = 0.0;
    double worst_float_vs_exact = 0.0;
    double worst_vs_exact = 0.0;
    long counted = 0;
    long declined = 0;
    int i;

    for (i = 0; i < cases; i++)
    {
        ChainCase cc;
        Mtx43d fd;
        double p[3];
        double vf[3];
        double ve[3];
        double vx[3];
        double e;

        BuildChain(&cc, scale_lo, scale_hi, depth, world_extent);
        if (cc.declined) { declined++; continue; }

        p[0] = Rand(-probe, probe);
        p[1] = Rand(-probe, probe);
        p[2] = Rand(-probe, probe);

        FloatToD(fd, cc.fworld);
        ApplyD(fd, p, vf);
        ApplyD(cc.eworld, p, ve);
        FixedApplyMtx(&cc.xworld, p, vx);

        e = MaxDiff3(vx, vf);
        if (e > worst_vs_float) { worst_vs_float = e; }
        e = MaxDiff3(vf, ve);
        if (e > worst_float_vs_exact) { worst_float_vs_exact = e; }
        e = MaxDiff3(vx, ve);
        if (e > worst_vs_exact) { worst_vs_exact = e; }
        counted++;
    }
    printf("  %-42s %10ld %11.7f %11.7f %11.7f  %s\n", label, counted,
           worst_vs_float, worst_float_vs_exact, worst_vs_exact,
           !gated ? "(reported)"
                  : ((worst_vs_float > BOUND) ? "RED" : "green"));
    if (gated && (worst_vs_float > BOUND)) { g_fail = 1; }
    if (declined != 0)
    {
        printf("  %-42s %10ld declined\n", "", declined);
    }
}

/* ======================================================================
 * T4 -- how orthogonal the game's joint rows actually are
 *
 * This is the measurement the row-scaled inverse stands or falls on, and it is
 * about the SOURCE, not about fixed point: M^-1[c][r] = M[r][c]/s_r^2 holds
 * exactly for a rotation with per-row scaling and only approximately for
 * whatever the game's quantised sine table produces. Reported as the relative
 * skew |row_i . row_j| / (s_i s_j), computed in double off the exact chain so
 * no representation of ours is in the answer.
 */
static void TestRowSkew(const char *label, double scale_lo, double scale_hi,
                        int depth, int cases)
{
    double worst = 0.0;
    double sum = 0.0;
    long counted = 0;
    int i;

    for (i = 0; i < cases; i++)
    {
        ChainCase cc;
        double s[3];
        int a, b;

        BuildChain(&cc, scale_lo, scale_hi, depth, 400.0);
        if (cc.declined) { continue; }
        for (a = 0; a < 3; a++)
        {
            s[a] = sqrt(cc.eworld[a][0] * cc.eworld[a][0] +
                        cc.eworld[a][1] * cc.eworld[a][1] +
                        cc.eworld[a][2] * cc.eworld[a][2]);
        }
        for (a = 0; a < 3; a++)
        {
            for (b = a + 1; b < 3; b++)
            {
                double dot = cc.eworld[a][0] * cc.eworld[b][0] +
                             cc.eworld[a][1] * cc.eworld[b][1] +
                             cc.eworld[a][2] * cc.eworld[b][2];
                double rel = fabs(dot) / (s[a] * s[b]);

                if (rel > worst) { worst = rel; }
                sum += rel;
                counted++;
            }
        }
    }
    printf("  %-42s %10ld  max rel skew %.8f  mean %.8f\n", label, counted,
           worst, counted ? sum / (double)counted : 0.0);
}

/* ======================================================================
 * T5 -- the two frame forms, graded side by side
 */
static void TestFrames(const char *label, double scale_lo, double scale_hi,
                       int depth, double world_extent, double reach, int gated,
                       int cases)
{
    double worst_row = 0.0;
    double worst_cof = 0.0;
    double worst_cof_exact = 0.0;
    double worst_float_exact = 0.0;
    double worst_scale = 0.0;
    long counted = 0;
    long row_declined = 0;
    long cof_declined = 0;
    long chain_declined = 0;
    int i;

    for (i = 0; i < cases; i++)
    {
        ChainCase cc;
        Mtx44f finv;
        NDSR2CfxFrame frow;
        NDSR2CfxFrame fcof;
        float fscale[3];
        double p[3];
        int rok;
        int cok;
        int c;

        BuildChain(&cc, scale_lo, scale_hi, depth, world_extent);
        if (cc.declined) { chain_declined++; continue; }
        if (FloatInvert(finv, cc.fworld) == 0) { continue; }
        FloatAxisScales(cc.fworld, fscale);

        rok = ndsR2CfxMakeFrameRowScaled(&frow, &cc.xworld);
        cok = ndsR2CfxMakeFrameCofactor(&fcof, &cc.xworld);
        if (!rok) { row_declined++; }
        if (!cok) { cof_declined++; }
        if (!cok) { continue; }

        p[0] = (double)cc.fworld[3][0] + Rand(-reach, reach);
        p[1] = (double)cc.fworld[3][1] + Rand(-reach, reach);
        p[2] = (double)cc.fworld[3][2] + Rand(-reach, reach);

        if (rok)
        {
            double e = FrameError(finv, &frow, p);

            if (e > worst_row) { worst_row = e; }
        }
        {
            double e = FrameError(finv, &fcof, p);
            Mtx43d einv;
            double vx[3];
            double ve[3];

            if (e > worst_cof) { worst_cof = e; }
            if (ExactInvert(einv, cc.eworld) != 0)
            {
                ApplyD(einv, p, ve);
                FixedApplyFrame(&fcof, p, vx);
                e = MaxDiff3(vx, ve);
                if (e > worst_cof_exact) { worst_cof_exact = e; }
                {
                    Mtx43d fd;
                    double vf[3];

                    FloatToD(fd, finv);
                    ApplyD(fd, p, vf);
                    e = MaxDiff3(vf, ve);
                    if (e > worst_float_exact) { worst_float_exact = e; }
                }
            }
        }
        for (c = 0; c < 3; c++)
        {
            double e = fabs((double)fscale[c] -
                            (double)fcof.scale[c] /
                                (double)NDS_R2_CFX_POS_ONE);

            if (e > worst_scale) { worst_scale = e; }
        }
        counted++;
    }
    printf("  %-42s %10ld  row-scaled %10.7f  cofactor %10.7f\n", label,
           counted, worst_row, worst_cof);
    printf("  %-42s %10s  cofactor-vs-exact %.7f  float-vs-exact %.7f\n", "",
           "", worst_cof_exact, worst_float_exact);
    printf("  %-42s %10s  declined: chain %ld  row %ld  cofactor %ld;"
           "  vec_scale max err %.7f\n",
           "", "", chain_declined, row_declined, cof_declined, worst_scale);
    if (gated)
    {
        if (worst_cof > BOUND) { g_fail = 1; printf("    RED cofactor\n"); }
        if (worst_scale > BOUND) { g_fail = 1; printf("    RED vec_scale\n"); }
    }
}

/* ======================================================================
 * T6 -- gmCollisionTestRectangle, DIFFERENTIAL on the DECISION
 *
 * The kernels' position error reaches the hit test as a change in where the
 * segment sits relative to the box faces, so the only figure that matters is
 * how often the BOOLEAN differs -- and, for the cases where it does, by how
 * little the float case was clearing the face.
 *
 * `margin` is the perturbation margin: the smallest |eps| such that adding eps
 * to all three half extents flips the float decision. It is the amount of box
 * the case has to spare, in world units, and a case is at risk exactly when its
 * margin is under the position error bound.
 */
static double PerturbationMargin(const float pc[3], const float pp[3],
                                 float radius, int transfer, Mtx44f mtx,
                                 const float off[3], const float size[3],
                                 const float scale[3], int decision)
{
    double lo = 0.0;
    double hi = 1.0;
    int i;

    /* Which direction flips it: a hit needs the box shrinking, a miss needs it
     * growing. Expand until the flip is bracketed or the search gives up. */
    for (i = 0; i < 24; i++)
    {
        double eps = decision ? -hi : hi;
        int flipped = FloatTestRectangle(pc, pp, radius, transfer, mtx, off,
                                         size, scale, (float)eps) != decision;

        if (flipped) { break; }
        hi *= 2.0;
        if (hi > 4096.0) { return 4096.0; }
    }
    for (i = 0; i < 40; i++)
    {
        double mid = 0.5 * (lo + hi);
        double eps = decision ? -mid : mid;
        int flipped = FloatTestRectangle(pc, pp, radius, transfer, mtx, off,
                                         size, scale, (float)eps) != decision;

        if (flipped) { hi = mid; } else { lo = mid; }
    }
    return 0.5 * (lo + hi);
}

static void TestRectangleDifferential(const char *label, double scale_lo,
                                      double scale_hi, int depth,
                                      double world_extent, int cases,
                                      double position_bound, int gated)
{
    long counted = 0;
    long mismatches = 0;
    long declines = 0;
    long at_risk = 0;
    double worst_mismatch_margin = 0.0;
    double smallest_margin = 1.0e30;
    long margin_hist[6];
    int i;

    memset(margin_hist, 0, sizeof(margin_hist));

    for (i = 0; i < cases; i++)
    {
        ChainCase cc;
        Mtx44f finv;
        NDSR2CfxFrame frame;
        float fscale[3];
        float pc[3];
        float pp[3];
        float off[3];
        float size[3];
        float radius;
        int32_t xpc[3];
        int32_t xpp[3];
        int32_t xoff[3];
        int32_t xsize[3];
        int32_t xradius;
        int fdec;
        int xdec;
        double margin;
        int c;

        BuildChain(&cc, scale_lo, scale_hi, depth, world_extent);
        if (cc.declined) { declines++; continue; }
        if (FloatInvert(finv, cc.fworld) == 0) { continue; }
        FloatAxisScales(cc.fworld, fscale);
        if (ndsR2CfxMakeFrameCofactor(&frame, &cc.xworld) == 0)
        {
            declines++;
            continue;
        }

        /* Hitbox radius and hurtbox half extents in the range the fighter data
         * uses: Fox's blaster hitbox is radius 20, the shield sphere is 30, and
         * ftparam.c:713 halves every damage_coll size at setup. The attacker's
         * swept segment is a frame of travel -- fast knockback moves a fighter
         * tens of units per tick, so the sweep is sampled to 80. */
        radius = (float)Rand(2.0, 40.0);
        for (c = 0; c < 3; c++)
        {
            off[c] = (float)Rand(-12.0, 12.0);
            size[c] = (float)Rand(1.0, 18.0);
        }
        /* Aim the segment near the joint so a meaningful fraction of cases are
         * genuine near-misses rather than trivial rejects. */
        for (c = 0; c < 3; c++)
        {
            pc[c] = (float)((double)cc.fworld[3][c] + Rand(-45.0, 45.0));
            pp[c] = (float)((double)pc[c] + Rand(-80.0, 80.0));
        }

        for (c = 0; c < 3; c++)
        {
            xpc[c] = Q12((double)pc[c]);
            xpp[c] = Q12((double)pp[c]);
            xoff[c] = Q12((double)off[c]);
            xsize[c] = Q12((double)size[c]);
        }
        xradius = Q12((double)radius);

        fdec = FloatTestRectangle(pc, pp, radius, 0, finv, off, size, fscale,
                                  0.0f);
        xdec = ndsR2CfxTestRectangle(xpc, xpp, xradius, 0, &frame, xoff, xsize,
                                     frame.inv_scale);
        if ((fdec == NDS_R2_CFX_DECLINE) || (xdec == NDS_R2_CFX_DECLINE))
        {
            declines++;
            continue;
        }
        counted++;

        margin = PerturbationMargin(pc, pp, radius, 0, finv, off, size, fscale,
                                    fdec);
        if (margin < smallest_margin) { smallest_margin = margin; }
        if (margin < position_bound) { at_risk++; }
        {
            int bucket = 0;
            double edges[5] = { 0.001, 0.01, 0.1, 1.0, 10.0 };

            while ((bucket < 5) && (margin >= edges[bucket])) { bucket++; }
            margin_hist[bucket]++;
        }

        if (fdec != xdec)
        {
            mismatches++;
            if (margin > worst_mismatch_margin)
            {
                worst_mismatch_margin = margin;
            }
        }
    }

    printf("  %-46s %12ld cases, %ld mismatches (%.6f%%), %ld declined\n",
           label, counted, mismatches,
           counted ? 100.0 * (double)mismatches / (double)counted : 0.0,
           declines);
    printf("  %-46s margin < %.4f: %ld (%.6f%%);  smallest margin %.7f;"
           "  largest margin at a mismatch %.7f\n",
           "", position_bound, at_risk,
           counted ? 100.0 * (double)at_risk / (double)counted : 0.0,
           smallest_margin, worst_mismatch_margin);
    printf("  %-46s margin histogram  <0.001 %ld | <0.01 %ld | <0.1 %ld |"
           " <1 %ld | <10 %ld | >=10 %ld\n",
           "", margin_hist[0], margin_hist[1], margin_hist[2], margin_hist[3],
           margin_hist[4], margin_hist[5]);

    /* The gate is NOT "zero mismatches" -- a quantised representation must
     * disagree on a case whose margin is under its own quantum, and asserting
     * otherwise would be asserting bit-exactness. The gate is that every
     * mismatch is inside the position bound: a disagreement on a case with more
     * than 0.0200 world units of box to spare would mean the arithmetic is
     * wrong, not merely quantised. */
    if (gated && (worst_mismatch_margin > position_bound))
    {
        g_fail = 1;
        printf("    RED: a decision flipped with %.7f world units of margin,"
               " past the %.4f bound\n",
               worst_mismatch_margin, position_bound);
    }
}

/* ======================================================================
 * T7 -- adversarial corners
 */
static void TestCorners(void)
{
    long bad = 0;
    long cases = 0;
    int i;

    /* Blast-zone extremes. MPGroundData's map_bound_* are s16 fields, so
     * +/-32,767 world units is the widest coordinate the stage format can
     * express; the position guard is 131,072, four times that, so nothing
     * reachable may decline on range. */
    for (i = 0; i < 20000; i++)
    {
        ChainCase cc;
        NDSR2CfxFrame frame;

        BuildChain(&cc, LIVE_SCALE_LO, LIVE_SCALE_HI, 6, 32767.0);
        if (cc.declined) { bad++; cases++; continue; }
        if (ndsR2CfxMakeFrameCofactor(&frame, &cc.xworld) == 0) { bad++; }
        cases++;
    }
    Verdict("T7a blast-zone extremes +/-32767 (declines)", (double)bad, 0.0,
            cases, 1);

    /* Near-zero and large scales: these MUST decline rather than wrap, and the
     * count below is the number that wrongly ACCEPTED. */
    bad = 0;
    cases = 0;
    for (i = 0; i < 20000; i++)
    {
        ChainCase cc;
        NDSR2CfxFrame frame;
        double s = (i & 1) ? Rand(0.0005, 0.02) : Rand(8.0, 400.0);

        BuildChain(&cc, s, s, 2, 400.0);
        if (cc.declined) { cases++; continue; }
        if (ndsR2CfxMakeFrameCofactor(&frame, &cc.xworld) != 0)
        {
            /* Accepting is only a fault if the result is then out of bound.
             * Grade it that way rather than by the guard's own opinion. */
            Mtx44f finv;
            double p[3];

            if (FloatInvert(finv, cc.fworld) != 0)
            {
                p[0] = (double)cc.fworld[3][0] + Rand(-64.0, 64.0);
                p[1] = (double)cc.fworld[3][1] + Rand(-64.0, 64.0);
                p[2] = (double)cc.fworld[3][2] + Rand(-64.0, 64.0);
                if (FrameError(finv, &frame, p) > BOUND) { bad++; }
            }
        }
        cases++;
    }
    Verdict("T7b degenerate scales accepted out of bound", (double)bad, 0.0,
            cases, 1);

    /* Zero and identity matrices, and a matrix whose rows are parallel. */
    bad = 0;
    cases = 0;
    {
        NDSR2CfxMtx m;
        NDSR2CfxFrame frame;

        memset(&m, 0, sizeof(m));
        if (ndsR2CfxMakeFrameCofactor(&frame, &m) != 0) { bad++; }
        if (ndsR2CfxMakeFrameRowScaled(&frame, &m) != 0) { bad++; }
        cases += 2;

        memset(&m, 0, sizeof(m));
        m.r[0][0] = m.r[1][1] = m.r[2][2] = NDS_R2_CFX_ROT_ONE;
        if (ndsR2CfxMakeFrameCofactor(&frame, &m) == 0) { bad++; }
        if (ndsR2CfxMakeFrameRowScaled(&frame, &m) == 0) { bad++; }
        cases += 2;

        memset(&m, 0, sizeof(m));
        m.r[0][0] = m.r[1][0] = m.r[2][2] = NDS_R2_CFX_ROT_ONE;
        if (ndsR2CfxMakeFrameCofactor(&frame, &m) != 0) { bad++; }
        cases++;
    }
    Verdict("T7c zero / identity / singular corners (failures)", (double)bad,
            0.0, cases, 1);
}

/* ====================================================================== */

int main(void)
{
    printf("R2-07 whole-cluster fixed-point collision kernels\n");
    printf("  Error is WORLD UNITS on a transformed point. Bound %.4f, the\n"
           "  E64b/E65 figure R2-07 L7 already carries.\n", BOUND);
    printf("  Live joint scale %.4f-%.4f, measured on a natural mode-163\n"
           "  match (NDS_R2_COLLISION_L7_ORACLE, 460 samples, 2026-07-31).\n\n",
           LIVE_SCALE_LO, LIVE_SCALE_HI);

    printf("ENUMERATED\n");
    printf("  %-46s %12s %12s %9s\n", "kernel / domain", "cases", "max error",
           "bound");
    TestSinTable();
    TestIsqrt();

    printf("\nBOUNDED -- forward chain (build + compose), probe +/-20\n");
    printf("  The gated column is fixed-vs-float, because the game's f32 result\n"
           "  is the behaviour to be equivalent to. The other two attribute it:\n"
           "  where fixed-vs-float is large and float-vs-exact is large with it,\n"
           "  the REFERENCE is what moved.\n");
    printf("  %-42s %10s %11s %11s %11s\n", "domain", "cases", "vs float",
           "float vs ex", "vs exact");
    ResetRng();
    TestChainForward("T3 live 1.1138-1.1199, depth 6, |t|<=400",
                     LIVE_SCALE_LO, LIVE_SCALE_HI, 6, 400.0, 20.0, 1, 200000);
    TestChainForward("T3 live, depth 12, |t|<=400", LIVE_SCALE_LO,
                     LIVE_SCALE_HI, 12, 400.0, 20.0, 1, 100000);
    TestChainForward("T3 live, depth 6, |t|<=32767 (blast zone)",
                     LIVE_SCALE_LO, LIVE_SCALE_HI, 6, 32767.0, 20.0, 1, 100000);
    TestChainForward("T3 wide 0.90-1.10, depth 6", 0.90, 1.10, 6, 400.0, 20.0,
                     0, 100000);
    TestChainForward("T3 wide 0.50-1.50, depth 6", 0.50, 1.50, 6, 400.0, 20.0,
                     0, 100000);
    TestChainForward("T3 wide 0.25-2.00, depth 6", 0.25, 2.00, 6, 400.0, 20.0,
                     0, 100000);

    printf("\nROW ORTHOGONALITY of the game's own joint matrices\n");
    ResetRng();
    TestRowSkew("T4 live, depth 1 (one local matrix)", LIVE_SCALE_LO,
                LIVE_SCALE_HI, 0, 50000);
    TestRowSkew("T4 live, depth 6", LIVE_SCALE_LO, LIVE_SCALE_HI, 6, 50000);
    TestRowSkew("T4 live, depth 12", LIVE_SCALE_LO, LIVE_SCALE_HI, 12, 50000);

    printf("\nBOUNDED -- the world->local frame, both forms\n");
    ResetRng();
    TestFrames("T5 live, depth 6, reach +/-64", LIVE_SCALE_LO, LIVE_SCALE_HI, 6,
               400.0, 64.0, 1, 200000);
    TestFrames("T5 live, depth 6, reach +/-4096", LIVE_SCALE_LO, LIVE_SCALE_HI,
               6, 400.0, 4096.0, 1, 100000);
    TestFrames("T5 live, depth 12, reach +/-64", LIVE_SCALE_LO, LIVE_SCALE_HI,
               12, 400.0, 64.0, 1, 100000);
    TestFrames("T5 wide 0.90-1.10, depth 6, reach +/-64", 0.90, 1.10, 6, 400.0,
               64.0, 0, 100000);
    TestFrames("T5 wide 0.50-1.50, depth 6, reach +/-64", 0.50, 1.50, 6, 400.0,
               64.0, 0, 100000);
    TestFrames("T5 wide 0.25-2.00, depth 6, reach +/-64", 0.25, 2.00, 6, 400.0,
               64.0, 0, 100000);

    printf("\nDIFFERENTIAL -- gmCollisionTestRectangle decisions\n");
    ResetRng();
    TestRectangleDifferential("T6 live, depth 6", LIVE_SCALE_LO, LIVE_SCALE_HI,
                              6, 400.0, 200000, BOUND, 1);
    TestRectangleDifferential("T6 live, depth 12", LIVE_SCALE_LO,
                              LIVE_SCALE_HI, 12, 400.0, 100000, BOUND, 1);

    printf("\nADVERSARIAL CORNERS\n");
    printf("  %-46s %12s %12s %9s\n", "case", "cases", "failures", "bound");
    ResetRng();
    TestCorners();

    if (g_fail)
    {
        printf("\nRED. Not fit to wire in.\n");
    }
    else
    {
        printf("\nGREEN on every gated row.\n");
    }
    return g_fail;
}
