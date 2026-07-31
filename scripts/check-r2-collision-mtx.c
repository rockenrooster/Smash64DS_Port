/* Falsifier for the R2-07 L7 collision matrix kernel.
 *
 * Compiles include/nds/nds_r2_collision_mtx.h -- the code that will ship -- and
 * runs it against a transcription of the decomp's float originals
 * (func_ovl2_800ED490 and gmCollisionSetInvertMatrix, gm/gmcollision.c) over
 * joint matrices built the way gmCollisionTransformMatrixAll builds them.
 *
 * The bar is NOT bit-exactness. 20.12 cannot reproduce float, and
 * PROJECT_GOAL.md does not ask it to -- it asks for mechanical equivalence.
 * The bar is the E64b/E65 precedent: a stated bound in the units the gameplay
 * reads, over the domain the gameplay actually visits. Collision decides hits,
 * so the reported quantity is world-unit error on a transformed point, not
 * matrix-cell ULPs -- a cell error only matters through what it does to a
 * position, and a bound stated in cells would be unreadable against a hitbox
 * size.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/nds/nds_r2_collision_mtx.h"

typedef float Mtx44f[4][4];

/* ---- the decomp originals, transcribed ------------------------------- */

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
}

/* gm/gmcollision.c:228. Returns 0 where the source would spin on a zero
 * determinant, so the harness can skip the case instead of hanging. */
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

    dst[1][0] = -dst[1][0];
    dst[3][0] = -dst[3][0];
    dst[0][1] = -dst[0][1];
    dst[2][1] = -dst[2][1];
    dst[1][2] = -dst[1][2];
    dst[3][2] = -dst[3][2];

    if (scale == 0.0F) { return 0; }
    scale = 1.0F / scale;

    dst[0][0] *= scale; dst[1][0] *= scale; dst[2][0] *= scale; dst[3][0] *= scale;
    dst[0][1] *= scale; dst[1][1] *= scale; dst[2][1] *= scale; dst[3][1] *= scale;
    dst[0][2] *= scale; dst[1][2] *= scale; dst[2][2] *= scale; dst[3][2] *= scale;
    return 1;
}

/* gm/gmcollision.c:29, the row convention the joint matrices actually carry. */
static void BuildJoint(Mtx44f m, float rx, float ry, float rz,
                       float sx, float sy, float sz,
                       float tx, float ty, float tz)
{
    float sinx = sinf(rx), cosx = cosf(rx);
    float siny = sinf(ry), cosy = cosf(ry);
    float sinz = sinf(rz), cosz = cosf(rz);

    m[0][0] = cosy * cosz;
    m[0][1] = cosy * sinz;
    m[0][2] = -siny;
    m[1][0] = (sinx * siny * cosz) - (cosx * sinz);
    m[1][1] = (sinx * siny * sinz) + (cosx * cosz);
    m[1][2] = sinx * cosy;
    m[2][0] = (cosx * siny * cosz) + (sinx * sinz);
    m[2][1] = (cosx * siny * sinz) - (sinx * cosz);
    m[2][2] = cosx * cosy;

    m[0][0] *= sx; m[0][1] *= sx; m[0][2] *= sx;
    m[1][0] *= sy; m[1][1] *= sy; m[1][2] *= sy;
    m[2][0] *= sz; m[2][1] *= sz; m[2][2] *= sz;

    m[3][0] = tx; m[3][1] = ty; m[3][2] = tz;
    m[0][3] = m[1][3] = m[2][3] = 0.0F;
    m[3][3] = 1.0F;
}

/* ---- conversion ------------------------------------------------------- */

static void ToFixed(NDSR2CollisionMtx *out, Mtx44f in)
{
    for (int r = 0; r < 4; r++)
    {
        for (int c = 0; c < 3; c++)
        {
            double v = (double)in[r][c] * NDS_R2_COLLISION_MTX_ONE;
            out->m[r][c] = (int32_t)(v < 0.0 ? v - 0.5 : v + 0.5);
        }
    }
}

static double Cell(const NDSR2CollisionMtx *m, int r, int c)
{
    return (double)m->m[r][c] / NDS_R2_COLLISION_MTX_ONE;
}

/* Error in world units: transform the same point by both matrices. This is the
 * quantity collision reads, so it is the quantity the bound is stated in. */
static double PointError(Mtx44f f, const NDSR2CollisionMtx *x,
                         double px, double py, double pz)
{
    double worst = 0.0;
    double fv[3], xv[3];

    for (int c = 0; c < 3; c++)
    {
        fv[c] = (double)f[0][c] * px + (double)f[1][c] * py +
                (double)f[2][c] * pz + (double)f[3][c];
        xv[c] = Cell(x, 0, c) * px + Cell(x, 1, c) * py +
                Cell(x, 2, c) * pz + Cell(x, 3, c);
        double d = fabs(fv[c] - xv[c]);
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

/* One sweep over a stated scale domain. E65's lesson applies directly here: the
 * bound is meaningless without the domain, because there is an amplifier and it
 * is steep. For the animation cubic that amplifier was L*|rate|; here it is
 * 1/det, and det is the product of the three joint scales -- so a chain that
 * reaches 0.25 on every axis amplifies the Q12 quantum by 64, while a
 * unit-scaled joint amplifies it by 1. Which of those SSB64 actually visits is
 * a measurement (scripts/census-fighter-gameplay-joints.ps1), not something to
 * assume, so this reports each domain separately instead of averaging them into
 * one reassuring number. */
static int Sweep(const char *name, double scale_lo, double scale_hi, int gated,
                 double bound)
{
    const int cases = 400000;
    double compose_worst = 0.0, invert_worst = 0.0;
    double compose_sum = 0.0, invert_sum = 0.0;
    long compose_n = 0, invert_n = 0, singular = 0;
    double worst_scale = 0.0;

    for (int i = 0; i < cases; i++)
    {
        Mtx44f a, b, fdst;
        NDSR2CollisionMtx xa, xb, xdst;
        double sx = Rand(scale_lo, scale_hi);
        double sy = Rand(scale_lo, scale_hi);
        double sz = Rand(scale_lo, scale_hi);

        BuildJoint(a, (float)Rand(-3.15, 3.15), (float)Rand(-3.15, 3.15),
                   (float)Rand(-3.15, 3.15), (float)sx, (float)sy, (float)sz,
                   (float)Rand(-400.0, 400.0), (float)Rand(-400.0, 400.0),
                   (float)Rand(-400.0, 400.0));
        BuildJoint(b, (float)Rand(-3.15, 3.15), (float)Rand(-3.15, 3.15),
                   (float)Rand(-3.15, 3.15), (float)Rand(scale_lo, scale_hi),
                   (float)Rand(scale_lo, scale_hi),
                   (float)Rand(scale_lo, scale_hi),
                   (float)Rand(-30.0, 30.0), (float)Rand(-30.0, 30.0),
                   (float)Rand(-30.0, 30.0));

        ToFixed(&xa, a);
        ToFixed(&xb, b);

        FloatCompose(fdst, a, b);
        ndsR2CollisionCompose(&xdst, &xa, &xb);
        {
            double e = PointError(fdst, &xdst, Rand(-20, 20), Rand(-20, 20),
                                  Rand(-20, 20));
            compose_sum += e; compose_n++;
            if (e > compose_worst) { compose_worst = e; }
        }

        {
            Mtx44f finv;
            NDSR2CollisionMtx xinv;
            int fok = FloatInvert(finv, a);
            int xok = ndsR2CollisionInvert(&xinv, &xa);

            if (!fok || !xok) { singular++; }
            else
            {
                /* The inverse carries a WORLD point into joint-local space, so
                 * the sample point is a world position near the joint, not a
                 * local offset. */
                double e = PointError(finv, &xinv,
                                      (double)a[3][0] + Rand(-20, 20),
                                      (double)a[3][1] + Rand(-20, 20),
                                      (double)a[3][2] + Rand(-20, 20));
                invert_sum += e; invert_n++;
                if (e > invert_worst)
                {
                    invert_worst = e;
                    worst_scale = sx < sy ? (sx < sz ? sx : sz)
                                          : (sy < sz ? sy : sz);
                }
            }
        }
    }

    printf("  %-14s %5.2f-%-5.2f %10.6f %10.6f %10.6f %10.6f  %s\n",
           name, scale_lo, scale_hi, compose_worst,
           compose_sum / (double)(compose_n ? compose_n : 1),
           invert_worst, invert_sum / (double)(invert_n ? invert_n : 1),
           gated ? (((compose_worst > bound) || (invert_worst > bound))
                        ? "RED" : "green")
                 : "(reported)");
    (void)singular;
    (void)worst_scale;
    if (!gated) { return 0; }
    return ((compose_worst > bound) || (invert_worst > bound)) ? 1 : 0;
}

int main(void)
{
    /* 0.02 world units is the bound E64b/E65 already carry for the animation
     * cubic. It is the same kind of quantity, so reuse it rather than invent a
     * second number for the reader to reconcile. */
    const double bound = 0.0200;
    int fail = 0;

    printf("R2-07 L7 collision matrix kernel -- 20.12 vs the decomp float\n");
    printf("  Error is WORLD UNITS on a transformed point, which is what\n");
    printf("  collision reads -- a matrix-cell bound cannot be compared against\n");
    printf("  a hurtbox. Bound %.4f, from E64b/E65.\n\n", bound);
    printf("  %-14s %11s %10s %10s %10s %10s  %s\n", "scale domain", "range",
           "compose max", "mean", "invert max", "mean", "gate");

    /* Only the near-unit domain is gated. SSB64 fighter joints are unit-scaled
     * unless an animation scales a part, and gmCollisionSetMatrixNcs multiplies
     * parent scale down the chain, so a real chain COULD compound below one --
     * but by how much is unmeasured. Gating the wider domains would be asserting
     * a fact nobody has established; reporting them shows how fast the bound
     * degrades if the census comes back with small scales. */
    fail |= Sweep("near-unit", 0.90, 1.10, 1, bound);
    fail |= Sweep("moderate", 0.50, 1.50, 0, bound);
    fail |= Sweep("conservative", 0.25, 2.00, 0, bound);

    printf("\n  Amplifier is 1/det, and det is the product of the three joint\n"
           "  scales -- 0.25 on every axis amplifies the Q12 quantum by 64,\n"
           "  unit scale by 1. WHICH DOMAIN SSB64 VISITS IS UNMEASURED: run\n"
           "  scripts/census-fighter-gameplay-joints.ps1 before gating a wider\n"
           "  one, and before wiring this kernel into anything.\n");
    if (fail)
    {
        printf("\nRED: the gated domain exceeds %.4f. Not fit to wire in.\n",
               bound);
    }
    else
    {
        printf("\nGated domain GREEN at %.4f. The wider domains are reported,\n"
               "not gated -- this is not yet clearance to wire the kernel in.\n",
               bound);
    }
    return fail;
}
