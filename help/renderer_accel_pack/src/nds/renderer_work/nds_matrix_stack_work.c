#include <math.h>
#include <string.h>

#include <nds/renderer_work/nds_matrix_stack_work.h>

static NdsMtxFixed ndsMtxMulFixed(NdsMtxFixed a, NdsMtxFixed b)
{
    return (NdsMtxFixed)(((s64)a * (s64)b) >> NDS_MTX_FRAC_BITS);
}

NdsMtxFixed ndsMtxFromFloat(f32 value)
{
    return (NdsMtxFixed)(value * (f32)NDS_MTX_ONE);
}

f32 ndsMtxToFloat(NdsMtxFixed value)
{
    return (f32)value / (f32)NDS_MTX_ONE;
}

void ndsMtxIdentity(NdsMtx44 *out)
{
    u32 i;
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    for (i = 0; i < 4; i++) out->m[i][i] = NDS_MTX_ONE;
}

void ndsMtxMul(NdsMtx44 *out, const NdsMtx44 *a, const NdsMtx44 *b)
{
    NdsMtx44 tmp;
    u32 r, c, k;
    if ((out == NULL) || (a == NULL) || (b == NULL)) return;
    memset(&tmp, 0, sizeof(tmp));
    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 4; c++)
        {
            s64 acc = 0;
            for (k = 0; k < 4; k++)
            {
                acc += (s64)a->m[r][k] * (s64)b->m[k][c];
            }
            tmp.m[r][c] = (NdsMtxFixed)(acc >> NDS_MTX_FRAC_BITS);
        }
    }
    *out = tmp;
}

void ndsMtxTranslate(NdsMtx44 *out, f32 x, f32 y, f32 z)
{
    ndsMtxIdentity(out);
    if (out == NULL) return;
    out->m[3][0] = ndsMtxFromFloat(x);
    out->m[3][1] = ndsMtxFromFloat(y);
    out->m[3][2] = ndsMtxFromFloat(z);
}

void ndsMtxScale(NdsMtx44 *out, f32 x, f32 y, f32 z)
{
    ndsMtxIdentity(out);
    if (out == NULL) return;
    out->m[0][0] = ndsMtxFromFloat(x);
    out->m[1][1] = ndsMtxFromFloat(y);
    out->m[2][2] = ndsMtxFromFloat(z);
}

void ndsMtxRotateZ(NdsMtx44 *out, f32 radians)
{
    f32 s = sinf(radians);
    f32 c = cosf(radians);
    ndsMtxIdentity(out);
    if (out == NULL) return;
    out->m[0][0] = ndsMtxFromFloat(c);
    out->m[0][1] = ndsMtxFromFloat(s);
    out->m[1][0] = ndsMtxFromFloat(-s);
    out->m[1][1] = ndsMtxFromFloat(c);
}

void ndsMtxRotateY(NdsMtx44 *out, f32 radians)
{
    f32 s = sinf(radians);
    f32 c = cosf(radians);
    ndsMtxIdentity(out);
    if (out == NULL) return;
    out->m[0][0] = ndsMtxFromFloat(c);
    out->m[0][2] = ndsMtxFromFloat(-s);
    out->m[2][0] = ndsMtxFromFloat(s);
    out->m[2][2] = ndsMtxFromFloat(c);
}

void ndsMtxRotateX(NdsMtx44 *out, f32 radians)
{
    f32 s = sinf(radians);
    f32 c = cosf(radians);
    ndsMtxIdentity(out);
    if (out == NULL) return;
    out->m[1][1] = ndsMtxFromFloat(c);
    out->m[1][2] = ndsMtxFromFloat(s);
    out->m[2][1] = ndsMtxFromFloat(-s);
    out->m[2][2] = ndsMtxFromFloat(c);
}

void ndsMtxLoadN64Mtx(NdsMtx44 *out, const Mtx *src)
{
    u32 r, c;
    if ((out == NULL) || (src == NULL)) return;
    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 4; c++)
        {
            /* N64 Mtx entries are s15.16. Convert to this helper's Q20.12. */
            out->m[r][c] = (NdsMtxFixed)((s32)src->m[r][c] >> 4);
        }
    }
}

NdsVec3 ndsMtxTransformPoint(const NdsMtx44 *m, NdsVec3 p)
{
    NdsVec3 out = {0, 0, 0};
    if (m == NULL) return out;
    out.x = ndsMtxMulFixed(p.x, m->m[0][0]) + ndsMtxMulFixed(p.y, m->m[1][0]) + ndsMtxMulFixed(p.z, m->m[2][0]) + m->m[3][0];
    out.y = ndsMtxMulFixed(p.x, m->m[0][1]) + ndsMtxMulFixed(p.y, m->m[1][1]) + ndsMtxMulFixed(p.z, m->m[2][1]) + m->m[3][1];
    out.z = ndsMtxMulFixed(p.x, m->m[0][2]) + ndsMtxMulFixed(p.y, m->m[1][2]) + ndsMtxMulFixed(p.z, m->m[2][2]) + m->m[3][2];
    return out;
}

void ndsMatrixStackInit(NdsMatrixStack *st)
{
    if (st == NULL) return;
    memset(st, 0, sizeof(*st));
    ndsMtxIdentity(&st->stack[0]);
}

const NdsMtx44 *ndsMatrixStackTop(const NdsMatrixStack *st)
{
    if (st == NULL) return NULL;
    return &st->stack[st->depth];
}

NdsMtx44 *ndsMatrixStackMutableTop(NdsMatrixStack *st)
{
    if (st == NULL) return NULL;
    return &st->stack[st->depth];
}

void ndsMatrixStackLoad(NdsMatrixStack *st, const NdsMtx44 *m)
{
    if ((st == NULL) || (m == NULL)) return;
    st->stack[st->depth] = *m;
}

void ndsMatrixStackMul(NdsMatrixStack *st, const NdsMtx44 *m)
{
    NdsMtx44 result;
    if ((st == NULL) || (m == NULL)) return;
    ndsMtxMul(&result, &st->stack[st->depth], m);
    st->stack[st->depth] = result;
}

void ndsMatrixStackPush(NdsMatrixStack *st)
{
    if (st == NULL) return;
    if ((st->depth + 1u) >= NDS_MTX_STACK_MAX)
    {
        st->overflow = 1u;
        return;
    }
    st->stack[st->depth + 1u] = st->stack[st->depth];
    st->depth++;
}

void ndsMatrixStackPop(NdsMatrixStack *st)
{
    if (st == NULL) return;
    if (st->depth == 0u)
    {
        st->underflow = 1u;
        return;
    }
    st->depth--;
}
