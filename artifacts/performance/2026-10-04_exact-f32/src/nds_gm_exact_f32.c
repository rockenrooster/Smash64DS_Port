/* P2-2p8 (2026-10-04): gm/gmcollision.c's float matrix chains, exact.
 *
 * func_ovl2_800ED490 (the latch walk's compose), gmCollisionGetWorldPosition
 * (a point through a joint matrix) and gmCollisionTransformMatrixAll (a
 * joint's local from its TRS) are the gameplay float work of every latch walk
 * -- the held item's attach runs one each frame, ~9 composes and locals --
 * and of the attack-position and hurtbox float paths. Each source operation
 * is a soft-float call that unpacks, rounds and packs; here the same chains
 * run on unpacked operands (include/nds/nds_exact_f32.h), each product and
 * sum rounded once as IEEE requires, so every result has the source's bits.
 *
 * battleship_gmcollision.c declares the three source definitions weak; the
 * strong ones below take every call, the source TU's own included. An operand
 * outside the exact domain (ndsXf32Admit), aliasing operands, or the A/B word
 * gNdsExactF32 == 0 take the source expression itself (the *Float copies
 * below, the same operations in the same order). Lab builds can run both and
 * count disagreements (gNdsExactF32Verify).
 *
 * ARM state (Makefile -marm): UMULL and CLZ. */

#include <ft/fighter.h>
#include <nds/nds_exact_f32.h>

#if !NDS_R2_SIM_MAC_SHADOW /* the shadow instrument renames two of these */

extern f32 lbCommonSin(f32 x);
extern f32 lbCommonCos(f32 x);

/* Same-ROM A/B word: 0 = the source float expressions. */
volatile u32 gNdsExactF32 __attribute__((used, section(".data"))) = 1u;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* 1 = compute both, keep the float result, count bit differences. */
volatile u32 gNdsExactF32Verify __attribute__((used, section(".data"))) = 0u;
__attribute__((used)) volatile u32 gNdsExactF32VerifyRuns;
__attribute__((used)) volatile u32 gNdsExactF32VerifyFail;
#endif
__attribute__((used)) volatile u32 gNdsExactF32Declines;

static inline u32 ndsXf32Bits(const f32 *p)
{
    u32 bits;

    __builtin_memcpy(&bits, p, sizeof(bits));
    return bits;
}

static inline void ndsXf32Store(f32 *p, u32 bits)
{
    __builtin_memcpy(p, &bits, sizeof(bits));
}

/* ---- func_ovl2_800ED490 ------------------------------------------------ */

static void __attribute__((noinline))
ndsGmComposeFloat(Mtx44f dst, Mtx44f lhs, Mtx44f rhs)
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

/* The exact chain into out[4][3]; FALSE when an operand is outside the
 * domain (nothing written). */
static sb32 ndsGmComposeExact(u32 out[4][3], Mtx44f lhs, Mtx44f rhs)
{
    NDSXf32 l[4][3];
    NDSXf32 r[4][3];
    u32 ok = 1u;
    u32 i;
    u32 j;

    for (i = 0u; i < 4u; i++)
    {
        for (j = 0u; j < 3u; j++)
        {
            const u32 lb = ndsXf32Bits(&lhs[i][j]);
            const u32 rb = ndsXf32Bits(&rhs[i][j]);

            ok &= ndsXf32Admit(lb) & ndsXf32Admit(rb);
            l[i][j] = ndsXf32Unpack(lb);
            r[i][j] = ndsXf32Unpack(rb);
        }
    }
    if (ok == 0u)
    {
        return FALSE;
    }
    for (i = 0u; i < 4u; i++)
    {
        for (j = 0u; j < 3u; j++)
        {
            NDSXf32 acc = ndsXf32Add(ndsXf32Mul(l[0][j], r[i][0]),
                                     ndsXf32Mul(l[1][j], r[i][1]));

            acc = ndsXf32Add(acc, ndsXf32Mul(l[2][j], r[i][2]));
            if (i == 3u)
            {
                acc = ndsXf32Add(acc, l[3][j]);
            }
            out[i][j] = ndsXf32Pack(acc);
        }
    }
    return TRUE;
}

void func_ovl2_800ED490(Mtx44f dst, Mtx44f lhs, Mtx44f rhs)
{
    u32 out[4][3];
    u32 i;
    u32 j;

    if ((gNdsExactF32 == 0u) || (dst == lhs) || (dst == rhs) ||
        (ndsGmComposeExact(out, lhs, rhs) == FALSE))
    {
        gNdsExactF32Declines++;
        ndsGmComposeFloat(dst, lhs, rhs);
        return;
    }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (gNdsExactF32Verify != 0u)
    {
        ndsGmComposeFloat(dst, lhs, rhs);
        gNdsExactF32VerifyRuns++;
        for (i = 0u; i < 4u; i++)
        {
            for (j = 0u; j < 3u; j++)
            {
                if (ndsXf32Bits(&dst[i][j]) != out[i][j])
                {
                    gNdsExactF32VerifyFail++;
                }
            }
        }
        return;
    }
#endif
    for (i = 0u; i < 4u; i++)
    {
        for (j = 0u; j < 3u; j++)
        {
            ndsXf32Store(&dst[i][j], out[i][j]);
        }
    }
}

/* ---- gmCollisionGetWorldPosition --------------------------------------- */

static void __attribute__((noinline))
ndsGmWorldPositionFloat(Mtx44f mtx, Vec3f *vec)
{
    Vec3f product;

    product.x = ((mtx[0][0] * vec->x) + (mtx[1][0] * vec->y) + (mtx[2][0] * vec->z)) + mtx[3][0];
    product.y = ((mtx[0][1] * vec->x) + (mtx[1][1] * vec->y) + (mtx[2][1] * vec->z)) + mtx[3][1];
    product.z = ((mtx[0][2] * vec->x) + (mtx[1][2] * vec->y) + (mtx[2][2] * vec->z)) + mtx[3][2];

    *vec = product;
}

void gmCollisionGetWorldPosition(Mtx44f mtx, Vec3f *vec)
{
    NDSXf32 m[4][3];
    NDSXf32 v[3];
    u32 out[3];
    u32 ok;
    u32 i;
    u32 j;

    if (gNdsExactF32 == 0u)
    {
        ndsGmWorldPositionFloat(mtx, vec);
        return;
    }
    {
        const u32 xb = ndsXf32Bits(&vec->x);
        const u32 yb = ndsXf32Bits(&vec->y);
        const u32 zb = ndsXf32Bits(&vec->z);

        ok = ndsXf32Admit(xb) & ndsXf32Admit(yb) & ndsXf32Admit(zb);
        v[0] = ndsXf32Unpack(xb);
        v[1] = ndsXf32Unpack(yb);
        v[2] = ndsXf32Unpack(zb);
    }
    for (i = 0u; i < 4u; i++)
    {
        for (j = 0u; j < 3u; j++)
        {
            const u32 b = ndsXf32Bits(&mtx[i][j]);

            ok &= ndsXf32Admit(b);
            m[i][j] = ndsXf32Unpack(b);
        }
    }
    if (ok == 0u)
    {
        gNdsExactF32Declines++;
        ndsGmWorldPositionFloat(mtx, vec);
        return;
    }
    for (j = 0u; j < 3u; j++)
    {
        NDSXf32 acc = ndsXf32Add(ndsXf32Mul(m[0][j], v[0]),
                                 ndsXf32Mul(m[1][j], v[1]));

        acc = ndsXf32Add(acc, ndsXf32Mul(m[2][j], v[2]));
        out[j] = ndsXf32Pack(ndsXf32Add(acc, m[3][j]));
    }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (gNdsExactF32Verify != 0u)
    {
        ndsGmWorldPositionFloat(mtx, vec);
        gNdsExactF32VerifyRuns++;
        if ((ndsXf32Bits(&vec->x) != out[0]) ||
            (ndsXf32Bits(&vec->y) != out[1]) ||
            (ndsXf32Bits(&vec->z) != out[2]))
        {
            gNdsExactF32VerifyFail++;
        }
        return;
    }
#endif
    ndsXf32Store(&vec->x, out[0]);
    ndsXf32Store(&vec->y, out[1]);
    ndsXf32Store(&vec->z, out[2]);
}

/* ---- gmCollisionTransformMatrixAll ------------------------------------- */

typedef struct NDSGmTrig
{
    f32 sinx, cosx, siny, cosy, sinz, cosz;
} NDSGmTrig;

static void __attribute__((noinline))
ndsGmLocalFloat(const NDSGmTrig *t, const Vec3f *scale, Mtx44f mtx)
{
    const f32 sinx = t->sinx, cosx = t->cosx;
    const f32 siny = t->siny, cosy = t->cosy;
    const f32 sinz = t->sinz, cosz = t->cosz;

    mtx[0][0] = cosy * cosz;
    mtx[0][1] = cosy * sinz;
    mtx[0][2] = -siny;

    mtx[1][0] = (sinx * siny * cosz) - (cosx * sinz);
    mtx[1][1] = (sinx * siny * sinz) + (cosx * cosz);
    mtx[1][2] = sinx * cosy;

    mtx[2][0] = (cosx * siny * cosz) + (sinx * sinz);
    mtx[2][1] = (cosx * siny * sinz) - (sinx * cosz);
    mtx[2][2] = cosx * cosy;

    if (scale->x != 1.0F)
    {
        mtx[0][0] *= scale->x;
        mtx[0][1] *= scale->x;
        mtx[0][2] *= scale->x;
    }
    if (scale->y != 1.0F)
    {
        mtx[1][0] *= scale->y;
        mtx[1][1] *= scale->y;
        mtx[1][2] *= scale->y;
    }
    if (scale->z != 1.0F)
    {
        mtx[2][0] *= scale->z;
        mtx[2][1] *= scale->z;
        mtx[2][2] *= scale->z;
    }
}

static inline NDSXf32 ndsXf32Neg(NDSXf32 v)
{
    v.s ^= 0x80000000u;
    return v;
}

/* The rotation and scale rows into out[3][3]; FALSE (nothing written) when
 * an operand is outside the domain. A scale of exactly 1.0 is skipped, as the
 * source's `!= 1.0F` skips it (only +1.0 compares equal). */
static sb32 ndsGmLocalExact(u32 out[3][3], const NDSGmTrig *t,
                            const Vec3f *scale)
{
    const u32 sxb = ndsXf32Bits(&t->sinx), cxb = ndsXf32Bits(&t->cosx);
    const u32 syb = ndsXf32Bits(&t->siny), cyb = ndsXf32Bits(&t->cosy);
    const u32 szb = ndsXf32Bits(&t->sinz), czb = ndsXf32Bits(&t->cosz);
    u32 scb[3];
    NDSXf32 sx, cx, sy, cy, sz, cz;
    NDSXf32 row[3][3];
    NDSXf32 ss;
    NDSXf32 cs;
    u32 ok;
    u32 r;
    u32 c;

    scb[0] = ndsXf32Bits(&scale->x);
    scb[1] = ndsXf32Bits(&scale->y);
    scb[2] = ndsXf32Bits(&scale->z);
    ok = ndsXf32Admit(sxb) & ndsXf32Admit(cxb) & ndsXf32Admit(syb) &
         ndsXf32Admit(cyb) & ndsXf32Admit(szb) & ndsXf32Admit(czb);
    for (r = 0u; r < 3u; r++)
    {
        if (scb[r] != 0x3f800000u)
        {
            ok &= ndsXf32Admit(scb[r]);
        }
    }
    if (ok == 0u)
    {
        return FALSE;
    }
    sx = ndsXf32Unpack(sxb); cx = ndsXf32Unpack(cxb);
    sy = ndsXf32Unpack(syb); cy = ndsXf32Unpack(cyb);
    sz = ndsXf32Unpack(szb); cz = ndsXf32Unpack(czb);
    ss = ndsXf32Mul(sx, sy); /* sinx * siny */
    cs = ndsXf32Mul(cx, sy); /* cosx * siny */

    row[0][0] = ndsXf32Mul(cy, cz);
    row[0][1] = ndsXf32Mul(cy, sz);
    row[0][2] = ndsXf32Neg(sy);

    row[1][0] = ndsXf32Add(ndsXf32Mul(ss, cz), ndsXf32Neg(ndsXf32Mul(cx, sz)));
    row[1][1] = ndsXf32Add(ndsXf32Mul(ss, sz), ndsXf32Mul(cx, cz));
    row[1][2] = ndsXf32Mul(sx, cy);

    row[2][0] = ndsXf32Add(ndsXf32Mul(cs, cz), ndsXf32Mul(sx, sz));
    row[2][1] = ndsXf32Add(ndsXf32Mul(cs, sz), ndsXf32Neg(ndsXf32Mul(sx, cz)));
    row[2][2] = ndsXf32Mul(cx, cy);

    for (r = 0u; r < 3u; r++)
    {
        if (scb[r] != 0x3f800000u)
        {
            const NDSXf32 k = ndsXf32Unpack(scb[r]);

            for (c = 0u; c < 3u; c++)
            {
                row[r][c] = ndsXf32Mul(row[r][c], k);
            }
        }
        for (c = 0u; c < 3u; c++)
        {
            out[r][c] = ndsXf32Pack(row[r][c]);
        }
    }
    return TRUE;
}

void gmCollisionTransformMatrixAll(DObj *dobj, FTParts *parts, Mtx44f mtx)
{
    Vec3f *translate = &dobj->translate.vec.f;
    Vec3f *rotate = &dobj->rotate.vec.f;
    Vec3f *scale = &dobj->scale.vec.f;
    NDSGmTrig t;
    u32 out[3][3];
    u32 r;
    u32 c;

    (void)parts;
    t.sinx = lbCommonSin(rotate->x);
    t.cosx = lbCommonCos(rotate->x);

    t.siny = lbCommonSin(rotate->y);
    t.cosy = lbCommonCos(rotate->y);

    t.sinz = lbCommonSin(rotate->z);
    t.cosz = lbCommonCos(rotate->z);

    if ((gNdsExactF32 == 0u) || (ndsGmLocalExact(out, &t, scale) == FALSE))
    {
        gNdsExactF32Declines++;
        ndsGmLocalFloat(&t, scale, mtx);
    }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    else if (gNdsExactF32Verify != 0u)
    {
        ndsGmLocalFloat(&t, scale, mtx);
        gNdsExactF32VerifyRuns++;
        for (r = 0u; r < 3u; r++)
        {
            for (c = 0u; c < 3u; c++)
            {
                if (ndsXf32Bits(&mtx[r][c]) != out[r][c])
                {
                    gNdsExactF32VerifyFail++;
                }
            }
        }
    }
#endif
    else
    {
        for (r = 0u; r < 3u; r++)
        {
            for (c = 0u; c < 3u; c++)
            {
                ndsXf32Store(&mtx[r][c], out[r][c]);
            }
        }
    }
    mtx[3][0] = translate->x;
    mtx[3][1] = translate->y;
    mtx[3][2] = translate->z;
}

#endif /* !NDS_R2_SIM_MAC_SHADOW */
