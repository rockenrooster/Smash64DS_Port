/* Host falsifier for the hurtbox kernel's local-frame reject.
 *
 * The float side is gm/gmcollision.c's gmCollisionSetInvertMatrix,
 * gmCollisionGetWorldPosition, func_ovl2_800EE24C/2C0 and
 * gmCollisionTestRectangle, transcribed operation for operation (build with
 * -msse2 -mfpmath=sse -ffp-contract=off so every f32 operation rounds once),
 * with vec_scale as func_ovl2_800EDE5C computes it. The fixed side is
 * ndsP2HbRejectLocal from src/port/nds_p2_hurtbox_reject.c, copied verbatim,
 * fed the float world converted to Q26/Q12 and then PERTURBED by up to
 * PERTURB quanta per cell -- far more than the fixed chain's measured error --
 * so a pass here is a pass with slack.
 *
 * Every case: the fixed test rejecting while the float test hits is a
 * violation. Prints the count (must be 0) and how many float misses the fixed
 * test rejected. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef float f32;
typedef float Mtx44f[4][4];
typedef struct { f32 x, y, z; } Vec3f;
typedef uint8_t u8;
typedef uint32_t u32;

/* ---- float source ---- */
static void gmCollisionGetWorldPosition(Mtx44f mtx, Vec3f *vec)
{
    Vec3f product;

    product.x = ((mtx[0][0] * vec->x) + (mtx[1][0] * vec->y) + (mtx[2][0] * vec->z)) + mtx[3][0];
    product.y = ((mtx[0][1] * vec->x) + (mtx[1][1] * vec->y) + (mtx[2][1] * vec->z)) + mtx[3][1];
    product.z = ((mtx[0][2] * vec->x) + (mtx[1][2] * vec->y) + (mtx[2][2] * vec->z)) + mtx[3][2];
    *vec = product;
}

static int gmCollisionSetInvertMatrix(Mtx44f dst, Mtx44f src)
{
    f32 scale;

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

    if (scale == 0.0F)
    {
        return 0;
    }
    scale = 1.0F / scale;

    dst[0][0] *= scale;
    dst[1][0] *= scale;
    dst[2][0] *= scale;
    dst[3][0] *= scale;
    dst[0][1] *= scale;
    dst[1][1] *= scale;
    dst[2][1] *= scale;
    dst[3][1] *= scale;
    dst[0][2] *= scale;
    dst[1][2] *= scale;
    dst[2][2] *= scale;
    dst[3][2] *= scale;
    return 1;
}

static u32 func_ovl2_800EE24C(Vec3f *lhs, Vec3f *rhs)
{
    u32 flags = 0;

    if (lhs->x < -rhs->x) flags |= 1;
    if (lhs->x > rhs->x) flags |= 2;
    if (lhs->y < -rhs->y) flags |= 4;
    if (lhs->y > rhs->y) flags |= 8;
    return flags;
}

static u32 func_ovl2_800EE2C0(Vec3f *lhs, Vec3f *rhs)
{
    u32 flags = 0;

    if (lhs->z < -rhs->z) flags |= 1;
    if (lhs->z > rhs->z) flags |= 2;
    return flags;
}

static int gmCollisionTestRectangle(Vec3f *pos_curr, Vec3f *pos_prev, f32 radius, int opkind, Mtx44f mtx, Vec3f *offset, Vec3f *size, Vec3f *scale)
{
    Vec3f center, sp90, sp84, sp78, sp6C;
    u32 flags_sp78, flags_sp6C, flags_main;
    f32 distx, disty, distz;
    int guard = 0;

    center.x = size->x + (radius / scale->x);
    center.y = size->y + (radius / scale->y);
    center.z = size->z + (radius / scale->z);

    if (opkind == 2)
    {
        sp90 = *pos_curr;
        gmCollisionGetWorldPosition(mtx, &sp90);
        sp90.x -= offset->x; sp90.y -= offset->y; sp90.z -= offset->z;
        return ((-center.x <= sp90.x) && (sp90.x <= center.x) && (-center.y <= sp90.y) && (sp90.y <= center.y) && (-center.z <= sp90.z) && (sp90.z <= center.z));
    }
    sp78 = *pos_curr;
    sp6C = *pos_prev;
    gmCollisionGetWorldPosition(mtx, &sp78);
    gmCollisionGetWorldPosition(mtx, &sp6C);
    sp78.x -= offset->x; sp78.y -= offset->y; sp78.z -= offset->z;
    sp6C.x -= offset->x; sp6C.y -= offset->y; sp6C.z -= offset->z;
    distx = sp6C.x - sp78.x;
    disty = sp6C.y - sp78.y;
    distz = sp6C.z - sp78.z;
    flags_sp78 = func_ovl2_800EE24C(&sp78, &center);
    flags_sp6C = func_ovl2_800EE24C(&sp6C, &center);
loop:
    if (++guard > 64) return -1; /* the source would spin; report */
    if ((flags_sp78 != 0) || (flags_sp6C != 0))
    {
        if (flags_sp78 & flags_sp6C) return 0;
        else if (flags_sp78 != 0) flags_main = flags_sp78;
        else flags_main = flags_sp6C;

        if (flags_main & 1)
        {
            sp84.x = -center.x;
            sp84.y = (((sp84.x - sp78.x) / distx) * disty) + sp78.y;
            sp84.z = (((sp84.x - sp78.x) / distx) * distz) + sp78.z;
        }
        else if (flags_main & 2)
        {
            sp84.x = center.x;
            sp84.y = (((sp84.x - sp78.x) / distx) * disty) + sp78.y;
            sp84.z = (((sp84.x - sp78.x) / distx) * distz) + sp78.z;
        }
        else if (flags_main & 4)
        {
            sp84.y = -center.y;
            sp84.x = (((sp84.y - sp78.y) / disty) * distx) + sp78.x;
            sp84.z = (((sp84.y - sp78.y) / disty) * distz) + sp78.z;
        }
        else if (flags_main & 8)
        {
            sp84.y = center.y;
            sp84.x = (((sp84.y - sp78.y) / disty) * distx) + sp78.x;
            sp84.z = (((sp84.y - sp78.y) / disty) * distz) + sp78.z;
        }
        if (flags_main == flags_sp78)
        {
            sp78 = sp84;
            flags_sp78 = func_ovl2_800EE24C(&sp78, &center);
        }
        else
        {
            sp6C = sp84;
            flags_sp6C = func_ovl2_800EE24C(&sp6C, &center);
        }
        goto loop;
    }
    flags_sp78 = func_ovl2_800EE2C0(&sp78, &center);
    flags_sp6C = func_ovl2_800EE2C0(&sp6C, &center);
    return (flags_sp78 & flags_sp6C) ? 0 : 1;
}

/* ---- fixed side (verbatim from the kernel and its header) ---- */
#define NDS_R2_CFX_ROT_BITS 26u
#define NDS_R2_CFX_POS_BITS 12u
#define NDS_R2_CFX_ROT_ONE (INT32_C(1) << NDS_R2_CFX_ROT_BITS)
#define NDS_R2_CFX_S2_MIN (NDS_R2_CFX_ROT_ONE >> 4)
#define NDS_R2_CFX_S2_MAX (INT32_C(16) * NDS_R2_CFX_ROT_ONE)
#define NDS_P2_HB_MARGIN_Q12 (INT32_C(4) << NDS_R2_CFX_POS_BITS)
typedef struct NDSR2CfxMtx { int32_t r[3][3]; int32_t t[3]; } NDSR2CfxMtx;

static inline int64_t ndsR2CfxShr(int64_t value, unsigned int shift)
{
    return (value + ((int64_t)1 << (shift - 1u))) >> shift;
}

static int ndsR2CfxRowScalesS2(const NDSR2CfxMtx *src, int32_t s2_q26[3])
{
    unsigned int row;

    for (row = 0u; row < 3u; row++)
    {
        int64_t sum = (int64_t)src->r[row][0] * src->r[row][0] +
                      (int64_t)src->r[row][1] * src->r[row][1] +
                      (int64_t)src->r[row][2] * src->r[row][2];
        int64_t s2 = ndsR2CfxShr(sum, NDS_R2_CFX_ROT_BITS);

        if ((s2 < (int64_t)NDS_R2_CFX_S2_MIN) || (s2 > (int64_t)NDS_R2_CFX_S2_MAX))
        {
            return 0;
        }
        s2_q26[row] = (int32_t)s2;
    }
    return 1;
}

#include "invsqrt_tables.h"

static int32_t ndsP2HbInvSqrtQ26(uint32_t s2)
{
    const uint32_t q = 26u - (uint32_t)__builtin_clz(s2);
    const uint32_t m = s2 >> q;
    const uint32_t t = ((q & 1u) != 0u) ? sNdsP2HbInvSqrtOdd[m - 32u]
                                        : sNdsP2HbInvSqrtEven[m - 32u];

    return (int32_t)(t << (15u - ((q + 1u) >> 1)));
}

static int ndsP2HbRejectLocal(const NDSR2CfxMtx *w, const int32_t off[3],
                              const int32_t size[3], int32_t radius,
                              const int32_t p0[3], const int32_t p1[3])
{
    static const u8 next[3] = { 1u, 2u, 0u };
    static const u8 prev[3] = { 2u, 0u, 1u };
    int32_t s2[3];
    int64_t v0[3];
    int64_t v1[3];
    u32 k;
    u32 c;

    if (ndsR2CfxRowScalesS2(w, s2) == 0)
    {
        return 0;
    }
    for (c = 0u; c < 3u; c++)
    {
        v0[c] = (int64_t)p0[c] - w->t[c];
        v1[c] = (int64_t)p1[c] - w->t[c];
    }
    for (k = 0u; k < 3u; k++)
    {
        const u32 i = next[k];
        const u32 j = prev[k];
        int64_t n[3];
        int64_t n1 = 0;
        int64_t det;
        int64_t num0;
        int64_t num1;
        int64_t reach;
        int64_t hi;
        int64_t lo;
        int64_t thr;

        for (c = 0u; c < 3u; c++)
        {
            const u32 a = next[c];
            const u32 b = prev[c];

            n[c] = ndsR2CfxShr((int64_t)w->r[i][a] * w->r[j][b] -
                                   (int64_t)w->r[i][b] * w->r[j][a],
                               NDS_R2_CFX_ROT_BITS);
        }
        det = ndsR2CfxShr((int64_t)w->r[k][0] * n[0] +
                              (int64_t)w->r[k][1] * n[1] +
                              (int64_t)w->r[k][2] * n[2],
                          NDS_R2_CFX_ROT_BITS);
        if (det < 0)
        {
            det = -det;
            n[0] = -n[0];
            n[1] = -n[1];
            n[2] = -n[2];
        }
        if ((det < ((int64_t)1 << 16)) || (det >= ((int64_t)1 << 33)))
        {
            return 0;
        }
        for (c = 0u; c < 3u; c++)
        {
            const int64_t m = (n[c] < 0) ? -n[c] : n[c];

            if (m >= ((int64_t)1 << 30))
            {
                return 0;
            }
            n1 += m;
        }
        reach = (int64_t)size[k] +
                (((int64_t)radius * ndsP2HbInvSqrtQ26((uint32_t)s2[k]) +
                  (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                 NDS_R2_CFX_ROT_BITS);
        hi = (int64_t)off[k] + reach;
        lo = (int64_t)off[k] - reach;
        if ((hi >= ((int64_t)1 << 26)) || (hi <= -((int64_t)1 << 26)) ||
            (lo >= ((int64_t)1 << 26)) || (lo <= -((int64_t)1 << 26)))
        {
            return 0;
        }
        num0 = v0[0] * n[0] + v0[1] * n[1] + v0[2] * n[2];
        num1 = v1[0] * n[0] + v1[1] * n[1] + v1[2] * n[2];
        thr = (int64_t)NDS_P2_HB_MARGIN_Q12 * n1;
        if (((num0 - hi * det) > thr) && ((num1 - hi * det) > thr))
        {
            return 1;
        }
        if (((lo * det - num0) > thr) && ((lo * det - num1) > thr))
        {
            return 1;
        }
    }
    return 0;
}

/* ---- harness ---- */
static uint64_t rng = 0x9e3779b97f4a7c15ull;
static double urand(void)
{
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return (double)(rng >> 11) / 9007199254740992.0;
}
static double uni(double a, double b) { return a + (b - a) * urand(); }
static int32_t q(double v, int bits) { return (int32_t)llround(ldexp(v, bits)); }

#define PERTURB 64

int main(int argc, char **argv)
{
    long cases = (argc > 1) ? atol(argv[1]) : 20000000L;
    long violations = 0, rejects = 0, float_miss = 0, float_hit = 0, spins = 0, near = 0;
    long n;

    for (n = 0; n < cases; n++)
    {
        Mtx44f M, inv;
        Vec3f scale, off, size, pc, pp;
        f32 rx = (f32)uni(-M_PI, M_PI), ry = (f32)uni(-M_PI, M_PI), rz = (f32)uni(-M_PI, M_PI);
        f32 sx, sy, sz, cx, cy, cz;
        f32 s[3];
        f32 radius;
        NDSR2CfxMtx w;
        int32_t off_q[3], size_q[3], p0[3], p1[3];
        int r, c, hit, rej;
        double lc[3], lp[3];

        sx = sinf(rx); cx = cosf(rx); sy = sinf(ry); cy = cosf(ry); sz = sinf(rz); cz = cosf(rz);
        for (r = 0; r < 3; r++)
        {
            double u = urand();
            s[r] = (u < 0.5) ? 1.0f : (f32)uni(0.35, 2.8);
        }
        if (urand() < 0.3) { s[1] = s[0]; s[2] = s[0]; }
        M[0][0] = cy * cz; M[0][1] = cy * sz; M[0][2] = -sy;
        M[1][0] = (sx * sy * cz) - (cx * sz); M[1][1] = (sx * sy * sz) + (cx * cz); M[1][2] = sx * cy;
        M[2][0] = (cx * sy * cz) + (sx * sz); M[2][1] = (cx * sy * sz) - (sx * cz); M[2][2] = cx * cy;
        for (r = 0; r < 3; r++)
            for (c = 0; c < 3; c++)
                M[r][c] *= s[r];
        if (urand() < 0.1) { for (c = 0; c < 3; c++) M[0][c] = -M[0][c]; } /* mirrored */
        M[3][0] = (f32)uni(-3000, 3000); M[3][1] = (f32)uni(-1500, 3000); M[3][2] = (f32)uni(-300, 300);
        M[0][3] = M[1][3] = M[2][3] = 0.0f; M[3][3] = 1.0f;
        if (!gmCollisionSetInvertMatrix(inv, M)) continue;
        scale.x = sqrtf((M[0][0] * M[0][0]) + (M[0][1] * M[0][1]) + (M[0][2] * M[0][2]));
        scale.y = sqrtf((M[1][0] * M[1][0]) + (M[1][1] * M[1][1]) + (M[1][2] * M[1][2]));
        scale.z = sqrtf((M[2][0] * M[2][0]) + (M[2][1] * M[2][1]) + (M[2][2] * M[2][2]));
        off.x = (f32)uni(-60, 60); off.y = (f32)uni(-60, 60); off.z = (f32)uni(-60, 60);
        size.x = (f32)uni(8, 160); size.y = (f32)uni(8, 160); size.z = (f32)uni(8, 160);
        radius = (f32)uni(10, 320);
        /* Points in local space near the box: off + reach * U(-2.5, 2.5). */
        {
            double reach[3] = { size.x + radius / scale.x, size.y + radius / scale.y, size.z + radius / scale.z };
            double o[3] = { off.x, off.y, off.z };
            double span = (urand() < 0.5) ? 1.6 : 3.0;
            for (c = 0; c < 3; c++)
            {
                lc[c] = o[c] + reach[c] * uni(-span, span);
                lp[c] = (urand() < 0.5) ? lc[c] + reach[c] * uni(-0.6, 0.6) : o[c] + reach[c] * uni(-span, span);
            }
        }
        /* local -> world through the float M (the points are data; any map). */
        pc.x = (f32)(lc[0] * M[0][0] + lc[1] * M[1][0] + lc[2] * M[2][0] + M[3][0]);
        pc.y = (f32)(lc[0] * M[0][1] + lc[1] * M[1][1] + lc[2] * M[2][1] + M[3][1]);
        pc.z = (f32)(lc[0] * M[0][2] + lc[1] * M[1][2] + lc[2] * M[2][2] + M[3][2]);
        pp.x = (f32)(lp[0] * M[0][0] + lp[1] * M[1][0] + lp[2] * M[2][0] + M[3][0]);
        pp.y = (f32)(lp[0] * M[0][1] + lp[1] * M[1][1] + lp[2] * M[2][1] + M[3][1]);
        pp.z = (f32)(lp[0] * M[0][2] + lp[1] * M[1][2] + lp[2] * M[2][2] + M[3][2]);

        hit = gmCollisionTestRectangle(&pc, &pp, radius, 0, inv, &off, &size, &scale);
        if (hit < 0) { spins++; continue; }
        if (hit) float_hit++; else float_miss++;

        for (r = 0; r < 3; r++)
        {
            for (c = 0; c < 3; c++)
            {
                int32_t d = (int32_t)(urand() * (2 * PERTURB + 1)) - PERTURB;
                w.r[r][c] = q(M[r][c], 26) + d;
            }
            w.t[r] = q(M[3][r], 12) + (int32_t)(urand() * 9) - 4;
        }
        off_q[0] = q(off.x, 12); off_q[1] = q(off.y, 12); off_q[2] = q(off.z, 12);
        size_q[0] = q(size.x, 12); size_q[1] = q(size.y, 12); size_q[2] = q(size.z, 12);
        p0[0] = q(pc.x, 12); p0[1] = q(pc.y, 12); p0[2] = q(pc.z, 12);
        p1[0] = q(pp.x, 12); p1[1] = q(pp.y, 12); p1[2] = q(pp.z, 12);
        rej = ndsP2HbRejectLocal(&w, off_q, size_q, q(radius, 12), p0, p1);
        if (rej)
        {
            rejects++;
            if (hit)
            {
                if (violations < 10)
                    printf("VIOLATION case %ld\n", n);
                violations++;
            }
        }
    }
    printf("cases %ld float hit %ld miss %ld spins %ld\n", cases, float_hit, float_miss, spins);
    printf("local rejects %ld (%.1f%% of float misses), violations %ld\n",
           rejects, 100.0 * rejects / (double)(float_miss ? float_miss : 1), violations);
    (void)near;
    return violations != 0;
}
