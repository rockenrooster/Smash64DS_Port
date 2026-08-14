/* Grade the SHIPPING fixed-point collision kernels against the joint world
 * matrices a live gate-arm match actually produces.
 *
 * Input is the CFXM rows written by scripts/probe-collision-fixed-domain.ps1,
 * one per joint whose inverse the hit path built that frame:
 *
 *   CFXM <frame> <round> <player> <joint> m00 m01 m02 m10 m11 m12 m20 m21 m22
 *        t0 t1 t2 vs_x vs_y vs_z <scale_latch>
 *
 * Why this exists rather than another synthetic sweep. Cycle A proved the
 * kernels over a chain BUILT IN FIXED POINT -- animation angles in, Q26 local
 * matrices, Q26 composes, then the frame. The seam that can actually ship keeps
 * the forward chain float (artifacts/performance/2026-08-13_c-collision-fixed/
 * SEAM_CORRECTION.md), so the frame's real input is FTParts::mtx_translate
 * quantised on the way in. That input domain has never been graded, and the
 * guards' live decline rate -- checklist item 6, the one thing cycle A left
 * open -- can only be read off real matrices.
 *
 * It compiles include/nds/nds_r2_collision_fixed.h, the code that ships, and
 * grades it against a transcription of the decomp float originals
 * (gm/gmcollision.c:228 gmCollisionSetInvertMatrix and :196
 * gmCollisionGetWorldPosition) plus a double-precision exact reference. Bound is
 * the 0.0200 world units R2-07 L7 already carries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <nds/nds_r2_collision_fixed.h>

#define MAX_ROWS 65536
#define BOUND_WORLD 0.0200

typedef struct Sample
{
    unsigned frame;
    int round;
    int player;
    int joint;
    float m[4][3];
    float vec_scale[3];
    int scale_latch;
} Sample;

static Sample g_samples[MAX_ROWS];
static int g_count;

/* --- the decomp float originals, transcribed ------------------------------ */

/* gm/gmcollision.c:228. Term order and the six sign flips are the source's. */
static void float_invert(float dst[4][3], const float src[4][3])
{
    float scale;

    dst[0][0] = (src[1][1] * src[2][2]) - (src[1][2] * src[2][1]);
    dst[1][0] = (src[1][0] * src[2][2]) - (src[1][2] * src[2][0]);
    dst[2][0] = (src[1][0] * src[2][1]) - (src[1][1] * src[2][0]);
    dst[3][0] = (src[3][0] * dst[0][0]) - (src[3][1] * dst[1][0]) +
                (src[3][2] * dst[2][0]);

    dst[0][1] = (src[0][1] * src[2][2]) - (src[0][2] * src[2][1]);
    dst[1][1] = (src[0][0] * src[2][2]) - (src[0][2] * src[2][0]);
    dst[2][1] = (src[0][0] * src[2][1]) - (src[0][1] * src[2][0]);
    dst[3][1] = (src[3][0] * dst[0][1]) - (src[3][1] * dst[1][1]) +
                (src[3][2] * dst[2][1]);

    dst[0][2] = (src[0][1] * src[1][2]) - (src[0][2] * src[1][1]);
    dst[1][2] = (src[0][0] * src[1][2]) - (src[0][2] * src[1][0]);
    dst[2][2] = (src[0][0] * src[1][1]) - (src[0][1] * src[1][0]);
    dst[3][2] = (src[3][0] * dst[0][2]) - (src[3][1] * dst[1][2]) +
                (src[3][2] * dst[2][2]);

    scale = (src[0][0] * dst[0][0]) - (src[0][1] * dst[1][0]) +
            (src[0][2] * dst[2][0]);

    dst[1][0] = -dst[1][0];
    dst[3][0] = -dst[3][0];
    dst[0][1] = -dst[0][1];
    dst[2][1] = -dst[2][1];
    dst[1][2] = -dst[1][2];
    dst[3][2] = -dst[3][2];

    scale = 1.0f / scale;

    dst[0][0] *= scale; dst[1][0] *= scale;
    dst[2][0] *= scale; dst[3][0] *= scale;
    dst[0][1] *= scale; dst[1][1] *= scale;
    dst[2][1] *= scale; dst[3][1] *= scale;
    dst[0][2] *= scale; dst[1][2] *= scale;
    dst[2][2] *= scale; dst[3][2] *= scale;
}

/* gm/gmcollision.c:196 */
static void float_world_position(const float mtx[4][3], const float in[3],
                                 float out[3])
{
    out[0] = ((mtx[0][0] * in[0]) + (mtx[1][0] * in[1]) +
              (mtx[2][0] * in[2])) + mtx[3][0];
    out[1] = ((mtx[0][1] * in[0]) + (mtx[1][1] * in[1]) +
              (mtx[2][1] * in[2])) + mtx[3][1];
    out[2] = ((mtx[0][2] * in[0]) + (mtx[1][2] * in[1]) +
              (mtx[2][2] * in[2])) + mtx[3][2];
}

/* The exact answer, in double, off the same matrix: local = (p - t) . R^-1. */
static int exact_world_to_local(const float src[4][3], const double p[3],
                                double out[3])
{
    double r[3][3];
    double inv[3][3];
    double det;
    double d[3];
    int i;
    int j;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            r[i][j] = (double)src[i][j];
        }
    }
    det = r[0][0] * (r[1][1] * r[2][2] - r[1][2] * r[2][1]) -
          r[0][1] * (r[1][0] * r[2][2] - r[1][2] * r[2][0]) +
          r[0][2] * (r[1][0] * r[2][1] - r[1][1] * r[2][0]);
    if (det == 0.0)
    {
        return 0;
    }
    inv[0][0] = (r[1][1] * r[2][2] - r[1][2] * r[2][1]) / det;
    inv[1][0] = -(r[1][0] * r[2][2] - r[1][2] * r[2][0]) / det;
    inv[2][0] = (r[1][0] * r[2][1] - r[1][1] * r[2][0]) / det;
    inv[0][1] = -(r[0][1] * r[2][2] - r[0][2] * r[2][1]) / det;
    inv[1][1] = (r[0][0] * r[2][2] - r[0][2] * r[2][0]) / det;
    inv[2][1] = -(r[0][0] * r[2][1] - r[0][1] * r[2][0]) / det;
    inv[0][2] = (r[0][1] * r[1][2] - r[0][2] * r[1][1]) / det;
    inv[1][2] = -(r[0][0] * r[1][2] - r[0][2] * r[1][0]) / det;
    inv[2][2] = (r[0][0] * r[1][1] - r[0][1] * r[1][0]) / det;

    for (i = 0; i < 3; i++)
    {
        d[i] = p[i] - (double)src[3][i];
    }
    for (j = 0; j < 3; j++)
    {
        out[j] = d[0] * inv[0][j] + d[1] * inv[1][j] + d[2] * inv[2][j];
    }
    return 1;
}

/* --- decline attribution --------------------------------------------------
 * The kernel returns 0 for five different reasons and a bare count cannot tell
 * them apart. Each branch below is the same predicate the header applies, in the
 * same order, so an attributed decline names the guard that fired.
 */
enum
{
    DECLINE_NONE = 0,
    DECLINE_CONVERT,
    DECLINE_RANGE,
    DECLINE_S2,
    DECLINE_DET,
    DECLINE_CELL,
    DECLINE_COUNT
};

static const char *const g_decline_name[DECLINE_COUNT] = {
    "none", "f32->fixed overflow", "rot/pos range", "row scale s^2",
    "determinant", "inverse cell"
};

static int attribute_decline(const NDSR2CfxMtx *fx)
{
    int32_t s2[3];
    int32_t inv_s2[3];
    int32_t s[3];
    int32_t inv_s[3];
    unsigned c;

    for (c = 0u; c < 3u; c++)
    {
        if (ndsR2CfxAbs32(fx->r[c][0]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(fx->r[c][1]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(fx->r[c][2]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(fx->t[c]) >= NDS_R2_CFX_POS_MAX)
        {
            return DECLINE_RANGE;
        }
    }
    if (ndsR2CfxRowScales(fx, s2, inv_s2, s, inv_s) == 0)
    {
        return DECLINE_S2;
    }
    /* Anything left is the determinant or an inverse cell; separate them by
     * recomputing the determinant the way the kernel does. */
    {
        int64_t c00 = ndsR2CfxShr(((int64_t)fx->r[1][1] * fx->r[2][2]) -
                                      ((int64_t)fx->r[1][2] * fx->r[2][1]),
                                  NDS_R2_CFX_ROT_BITS);
        int64_t c10 = ndsR2CfxShr(((int64_t)fx->r[1][0] * fx->r[2][2]) -
                                      ((int64_t)fx->r[1][2] * fx->r[2][0]),
                                  NDS_R2_CFX_ROT_BITS);
        int64_t c20 = ndsR2CfxShr(((int64_t)fx->r[1][0] * fx->r[2][1]) -
                                      ((int64_t)fx->r[1][1] * fx->r[2][0]),
                                  NDS_R2_CFX_ROT_BITS);
        int64_t det = ndsR2CfxShr(((int64_t)fx->r[0][0] * c00) -
                                      ((int64_t)fx->r[0][1] * c10) +
                                      ((int64_t)fx->r[0][2] * c20),
                                  NDS_R2_CFX_ROT_BITS);

        if ((det < (int64_t)(INT32_C(1) << 21)) &&
            (det > -(int64_t)(INT32_C(1) << 21)))
        {
            return DECLINE_DET;
        }
    }
    return DECLINE_CELL;
}

/* --- io ------------------------------------------------------------------ */

static int parse_line(const char *line, Sample *out)
{
    int n;

    n = sscanf(line,
               "CFXM %u %d %d %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %d",
               &out->frame, &out->round, &out->player, &out->joint,
               &out->m[0][0], &out->m[0][1], &out->m[0][2],
               &out->m[1][0], &out->m[1][1], &out->m[1][2],
               &out->m[2][0], &out->m[2][1], &out->m[2][2],
               &out->m[3][0], &out->m[3][1], &out->m[3][2],
               &out->vec_scale[0], &out->vec_scale[1], &out->vec_scale[2],
               &out->scale_latch);
    return (n == 20);
}

int main(int argc, char **argv)
{
    static const int probes[] = { 1, 4, 16, 64 };
    FILE *fp;
    char line[1024];
    int i;
    int declines[DECLINE_COUNT];
    int admitted = 0;
    int convert_fail = 0;
    double worst_vs_float = 0.0;
    double worst_vs_exact = 0.0;
    double worst_float_vs_exact = 0.0;
    double sum_vs_float = 0.0;
    long compares = 0;
    double scale_min = 1e30;
    double scale_max = -1e30;
    double worst_scale_err = 0.0;
    int scale_checked = 0;
    unsigned frame_lo = 0xFFFFFFFFu;
    unsigned frame_hi = 0;
    int worst_joint = -1;
    unsigned worst_frame = 0;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <probe-capture.txt>\n", argv[0]);
        return 2;
    }
    fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    memset(declines, 0, sizeof(declines));
    while ((fgets(line, sizeof(line), fp) != NULL) && (g_count < MAX_ROWS))
    {
        const char *p = strstr(line, "CFXM ");

        if ((p != NULL) && parse_line(p, &g_samples[g_count]))
        {
            g_count++;
        }
    }
    fclose(fp);
    if (g_count == 0)
    {
        fprintf(stderr, "no CFXM rows in %s\n", argv[1]);
        return 2;
    }

    for (i = 0; i < g_count; i++)
    {
        const Sample *s = &g_samples[i];
        NDSR2CfxMtx fx;
        NDSR2CfxFrame frame;
        float finv[4][3];
        int row;
        int col;
        int overflow = 0;
        int axis;
        int pi;

        if (s->frame < frame_lo) { frame_lo = s->frame; }
        if (s->frame > frame_hi) { frame_hi = s->frame; }
        for (axis = 0; axis < 3; axis++)
        {
            if ((double)s->vec_scale[axis] < scale_min)
            {
                scale_min = s->vec_scale[axis];
            }
            if ((double)s->vec_scale[axis] > scale_max)
            {
                scale_max = s->vec_scale[axis];
            }
        }

        for (row = 0; row < 3; row++)
        {
            for (col = 0; col < 3; col++)
            {
                fx.r[row][col] = ndsR2CollisionF32ToFixed(s->m[row][col],
                                                          NDS_R2_CFX_ROT_BITS);
                if (fx.r[row][col] == NDS_R2_COLLISION_F32_OVERFLOW)
                {
                    overflow = 1;
                }
            }
        }
        for (col = 0; col < 3; col++)
        {
            fx.t[col] = ndsR2CollisionF32ToFixed(s->m[3][col],
                                                 NDS_R2_CFX_POS_BITS);
            if (fx.t[col] == NDS_R2_COLLISION_F32_OVERFLOW)
            {
                overflow = 1;
            }
        }
        if (overflow)
        {
            convert_fail++;
            declines[DECLINE_CONVERT]++;
            continue;
        }

        if (ndsR2CfxMakeFrameCofactor(&frame, &fx) == 0)
        {
            declines[attribute_decline(&fx)]++;
            continue;
        }
        admitted++;

        /* vec_scale agreement, only where the source's own scale latch says the
         * float value is live this frame. */
        if (s->scale_latch != 0)
        {
            for (axis = 0; axis < 3; axis++)
            {
                double got = (double)frame.scale[axis] /
                             (double)NDS_R2_CFX_POS_ONE;
                double err = fabs(got - (double)s->vec_scale[axis]);

                if (err > worst_scale_err) { worst_scale_err = err; }
            }
            scale_checked++;
        }

        float_invert(finv, s->m);

        for (pi = 0; pi < (int)(sizeof(probes) / sizeof(probes[0])); pi++)
        {
            for (axis = 0; axis < 3; axis++)
            {
                float probe_f[3];
                float ref[3];
                double probe_d[3];
                double exact[3];
                int32_t probe_q[3];
                int32_t local[3];
                int c;

                for (c = 0; c < 3; c++)
                {
                    probe_f[c] = s->m[3][c];
                }
                probe_f[axis] += (float)probes[pi];
                for (c = 0; c < 3; c++)
                {
                    probe_d[c] = (double)probe_f[c];
                    probe_q[c] = ndsR2CollisionF32ToFixed(probe_f[c],
                                                          NDS_R2_CFX_POS_BITS);
                }

                float_world_position(finv, probe_f, ref);
                ndsR2CfxWorldToLocal(local, &frame, probe_q);

                for (c = 0; c < 3; c++)
                {
                    double got = (double)local[c] / (double)NDS_R2_CFX_POS_ONE;
                    double d = fabs(got - (double)ref[c]);

                    if (d > worst_vs_float)
                    {
                        worst_vs_float = d;
                        worst_joint = s->joint;
                        worst_frame = s->frame;
                    }
                    sum_vs_float += d;
                    compares++;
                }
                if (exact_world_to_local(s->m, probe_d, exact))
                {
                    for (c = 0; c < 3; c++)
                    {
                        double got = (double)local[c] /
                                     (double)NDS_R2_CFX_POS_ONE;
                        double de = fabs(got - exact[c]);
                        double df = fabs((double)ref[c] - exact[c]);

                        if (de > worst_vs_exact) { worst_vs_exact = de; }
                        if (df > worst_float_vs_exact)
                        {
                            worst_float_vs_exact = df;
                        }
                    }
                }
            }
        }
    }

    printf("R2-07 cycle B -- fixed-point collision frame on the LIVE domain\n");
    printf("  source: %s\n", argv[1]);
    printf("  joint matrices: %d   frames %u..%u\n", g_count, frame_lo,
           frame_hi);
    printf("  live vec_scale domain: %.6f .. %.6f\n", scale_min, scale_max);
    printf("\nGUARDS (the cofactor frame, built from the FLOAT mtx_translate)\n");
    printf("  admitted            %d (%.4f%%)\n", admitted,
           100.0 * (double)admitted / (double)g_count);
    for (i = 1; i < DECLINE_COUNT; i++)
    {
        printf("  declined %-22s %d\n", g_decline_name[i], declines[i]);
    }
    (void)convert_fail;
    printf("\nACCURACY, world units, probes at 1/4/16/64 units on each axis\n");
    printf("  comparisons              %ld\n", compares);
    printf("  max fixed vs float       %.7f   (bound %.4f)  joint %d frame %u\n",
           worst_vs_float, BOUND_WORLD, worst_joint, worst_frame);
    printf("  mean fixed vs float      %.7f\n",
           (compares != 0) ? (sum_vs_float / (double)compares) : 0.0);
    printf("  max fixed vs exact       %.7f\n", worst_vs_exact);
    printf("  max FLOAT vs exact       %.7f\n", worst_float_vs_exact);
    printf("  max vec_scale delta      %.7f  (%d matrices with the scale latch "
           "live)\n", worst_scale_err, scale_checked);

    if (worst_vs_float > BOUND_WORLD)
    {
        printf("\nRED: the live domain exceeds the 0.0200 bound.\n");
        return 1;
    }
    printf("\nGREEN: every live joint is inside the 0.0200 bound.\n");
    return 0;
}
