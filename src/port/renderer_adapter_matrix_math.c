/* Renderer adapter, matrix math: Q20.12 identity and products, the N64
 * Mtx and float conversions, the camera producers, joint attachments and
 * the Sector Z Arwing's basis. A slice of reloc_backend_renderer_dl.c, after
 * renderer_adapter_matrix.c. */

static void ndsRendererAdapterMtxIdentity20p12(
    NDSRendererMatrix20p12 *out)
{
    if (out == NULL)
    {
        return;
    }

    /* R2-03 E69. Was `memset(out, 0, 64)` plus a four-iteration diagonal loop --
     * a library call and a loop to write twelve zeros. */
    ndsRendererMatrixIdentity20p12(
        out, 1 << NDS_RENDERER_ADAPTER_MTX_FRAC_BITS);
}

static void ndsRendererAdapterMulBefore(NDSRendererMatrix20p12 *target,
                                        const NDSRendererMatrix20p12 *incoming,
                                        u32 *valid)
{
    if ((target == NULL) || (incoming == NULL) || (valid == NULL))
    {
        return;
    }

    if (*valid != 0u)
    {
        ndsRendererMtxMul20p12(incoming, target, target);
    }
    else
    {
        ndsRendererMatrixCopy20p12(target, incoming);
        *valid = TRUE;
    }
}


static void ndsRendererAdapterMtxFromN64(
    const Mtx *src, NDSRendererMatrix20p12 *dst)
{
    if ((src == NULL) || (dst == NULL))
    {
        return;
    }
    ndsRendererMtxLoadN64ToDS20p12(src, dst);
}

/* The renderer's camera producers in Q20.12: the same representable value the
 * float chain (syMatrixLookAtReflect / syMatrixPerspFast, syMatrixF2L,
 * ndsRendererAdapterMtxFromN64) reached through s15.16, with no float and no
 * intermediate Mtx. The float arm and its route word were deleted 2026-10-06.
 * `look_at` is a stack local nothing reads at every caller, so the six
 * reflectance conversions are skipped (NULL). */
static void ndsRendererAdapterCameraLookAtReflect(
    NDSRendererMatrix20p12 *out, LookAt *look_at,
    f32 eye_x, f32 eye_y, f32 eye_z,
    f32 at_x, f32 at_y, f32 at_z,
    f32 up_x, f32 up_y, f32 up_z)
{
    (void)look_at;
    ndsCameraLookAtReflect20p12(out, NULL, eye_x, eye_y, eye_z,
                                  at_x, at_y, at_z, up_x, up_y, up_z);
}

static void ndsRendererAdapterCameraPerspFast(
    NDSRendererMatrix20p12 *out, u16 *persp_norm,
    f32 fovy, f32 aspect, f32 near, f32 far, f32 scale)
{
    ndsCameraPerspFast20p12(out, persp_norm, fovy, aspect, near, far,
                              scale);
}

static inline sb32 ndsRendererAdapterFloatPow2ToS32(
    f32 value, u32 scale_bits, s32 *out)
{
    union
    {
        f32 f;
        u32 u;
    } bits = { value };
    u32 exponent = (bits.u >> 23) & 0xffu;
    u32 magnitude;
    s32 shift;

    if ((out == NULL) || (exponent == 0xffu))
    {
        return FALSE;
    }
    if (exponent == 0u)
    {
        *out = 0;
        return TRUE;
    }

    magnitude = (bits.u & 0x7fffffu) | 0x800000u;
    shift = (s32)exponent - 127 - 23 + (s32)scale_bits;
    if (shift < 0)
    {
        magnitude = (shift <= -24) ? 0u : magnitude >> (u32)-shift;
    }
    else
    {
        if (shift > 8)
        {
            return FALSE;
        }
        magnitude <<= (u32)shift;
        if (((bits.u & 0x80000000u) == 0u) &&
            (magnitude > 0x7fffffffu))
        {
            return FALSE;
        }
    }

    *out = ((bits.u & 0x80000000u) != 0u) ?
        (s32)(0u - magnitude) : (s32)magnitude;
    return TRUE;
}

/* R2-03 E9. The fighter-parts path converts float -> N64 16.16 -> DS 20.12, and
 * E8 established that 97.5% of local-matrix builds take it. The intermediate is
 * a pure round trip: F2LFixedWExact packs each value into COMBINE_INTEGRAL /
 * COMBINE_FRACTIONAL halves, and MtxCellS16p16 recombines exactly those halves,
 * so recombine(split(x)) == x. The whole two-step therefore equals, per cell,
 * RoundShiftS32(FloatPow2ToS32(v, 16), 4) -- and the cell mapping works out to
 * the identity, with the w column zero except (3,3) = 1.0.
 *
 * Computing that directly is bit-exact by construction rather than by tolerance,
 * so unlike E6's fixed-point angle lever this needs no fidelity budget. Level 2
 * runs both and compares anyway, because "by construction" is exactly the kind
 * of claim E8 proved I get wrong by reading. */

static s32 ndsRendererAdapterRoundShift20p12(s32 value)
{
    /* Mirrors nds_renderer.c's ndsRendererRoundShiftS32(value, 4), which is
     * file-static there: round half away from zero, not an arithmetic shift. */
    s64 wide = value;

    if (wide < 0)
    {
        return (s32)(-(((-wide) + 8) >> 4));
    }
    return (s32)((wide + 8) >> 4);
}

static sb32 ndsRendererAdapterF2LDirect20p12(
    const Mtx44f *src, NDSRendererMatrix20p12 *dst)
{
    u32 row;
    u32 col;
    s32 e;

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            if (ndsRendererAdapterFloatPow2ToS32(
                    (*src)[row][col], 16u, &e) == FALSE)
            {
                return FALSE;
            }
            dst->m[row][col] = ndsRendererAdapterRoundShift20p12(e);
        }
        dst->m[row][3] = 0;
    }
    /* F2LFixedWExact pairs src[3][2] with a literal 0x00010000, i.e. 1.0 in
     * 16.16, which lands in cell (3,3). */
    dst->m[3][3] = ndsRendererAdapterRoundShift20p12(0x00010000);
    return TRUE;
}

static sb32 ndsRendererAdapterF2LFixedWExact(
    const Mtx44f *src, Mtx *dst)
{
    s32 e1;
    s32 e2;

#define NDS_F2L_FIXED(value, output) \
    do \
    { \
        if (ndsRendererAdapterFloatPow2ToS32((value), 16u, &(output)) == FALSE) \
        { \
            return FALSE; \
        } \
    } while (0)

    NDS_F2L_FIXED((*src)[0][0], e1);
    NDS_F2L_FIXED((*src)[0][1], e2);
    dst->m[0][0] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[2][0] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[0][2], e1);
    dst->m[0][1] = COMBINE_INTEGRAL((u32)e1, 0u);
    dst->m[2][1] = COMBINE_FRACTIONAL((u32)e1, 0u);
    NDS_F2L_FIXED((*src)[1][0], e1);
    NDS_F2L_FIXED((*src)[1][1], e2);
    dst->m[0][2] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[2][2] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[1][2], e1);
    dst->m[0][3] = COMBINE_INTEGRAL((u32)e1, 0u);
    dst->m[2][3] = COMBINE_FRACTIONAL((u32)e1, 0u);
    NDS_F2L_FIXED((*src)[2][0], e1);
    NDS_F2L_FIXED((*src)[2][1], e2);
    dst->m[1][0] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[3][0] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[2][2], e1);
    dst->m[1][1] = COMBINE_INTEGRAL((u32)e1, 0u);
    dst->m[3][1] = COMBINE_FRACTIONAL((u32)e1, 0u);
    NDS_F2L_FIXED((*src)[3][0], e1);
    NDS_F2L_FIXED((*src)[3][1], e2);
    dst->m[1][2] = COMBINE_INTEGRAL((u32)e1, (u32)e2);
    dst->m[3][2] = COMBINE_FRACTIONAL((u32)e1, (u32)e2);
    NDS_F2L_FIXED((*src)[3][2], e1);
    dst->m[1][3] = COMBINE_INTEGRAL((u32)e1, 0x00010000u);
    dst->m[3][3] = COMBINE_FRACTIONAL((u32)e1, 0x00010000u);

#undef NDS_F2L_FIXED
    return TRUE;
}

static inline s32 ndsRendererAdapterFighterSinFromIndex(s32 index)
{
    u32 id = (u32)index & 0xfffu;
    s32 value = (s32)gSYSinTable[id & 0x7ffu];

    return ((id & 0x800u) != 0u) ? -value : value;
}

static inline s32 ndsRendererAdapterFighterCosFromIndex(s32 index)
{
    return ndsRendererAdapterFighterSinFromIndex(index + 0x400);
}

static NDS_RENDERER_ADAPTER_FIGHTER_MATRIX_CODE sb32
ndsRendererAdapterBuildFighterTraRotRpyExact(
    Mtx *mtx,
    f32 tx,
    f32 ty,
    f32 tz,
    f32 r,
    f32 p,
    f32 y)
{
    s32 indexr;
    s32 indexp;
    s32 indexy;
    s32 sinr;
    s32 sinp;
    s32 siny;
    s32 cosr;
    s32 cosp;
    s32 cosy;
    s32 fixed_x;
    s32 fixed_y;
    s32 fixed_z;
    u32 e1;
    u32 e2;

    if ((mtx == NULL) ||
        (ndsFighterMatrixAngleToIndexExact(r, &indexr) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(p, &indexp) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(y, &indexy) == 0) ||
        (ndsRendererAdapterFloatPow2ToS32(tx, 16u, &fixed_x) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(ty, 16u, &fixed_y) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(tz, 16u, &fixed_z) == FALSE))
    {
        return FALSE;
    }

    sinr = ndsRendererAdapterFighterSinFromIndex(indexr);
    cosr = ndsRendererAdapterFighterCosFromIndex(indexr);
    sinp = ndsRendererAdapterFighterSinFromIndex(indexp);
    cosp = ndsRendererAdapterFighterCosFromIndex(indexp);
    siny = ndsRendererAdapterFighterSinFromIndex(indexy);
    cosy = ndsRendererAdapterFighterCosFromIndex(indexy);

    e1 = (u32)((cosp * cosy) >> 14);
    e2 = (u32)((cosp * siny) >> 14);
    mtx->m[0][0] = COMBINE_INTEGRAL(e1, e2);
    mtx->m[2][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)(-sinp * 2);
    mtx->m[0][1] = COMBINE_INTEGRAL(e1, 0u);
    mtx->m[2][1] = COMBINE_FRACTIONAL(e1, 0u);

    e1 = (u32)((((sinr * sinp) >> 15) * cosy) >> 14) -
         (u32)((cosr * siny) >> 14);
    e2 = (u32)((((sinr * sinp) >> 15) * siny) >> 14) +
         (u32)((cosr * cosy) >> 14);
    mtx->m[0][2] = COMBINE_INTEGRAL(e1, e2);
    mtx->m[2][2] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)((sinr * cosp) >> 14);
    mtx->m[0][3] = COMBINE_INTEGRAL(e1, 0u);
    mtx->m[2][3] = COMBINE_FRACTIONAL(e1, 0u);

    e1 = (u32)((((cosr * sinp) >> 15) * cosy) >> 14) +
         (u32)((sinr * siny) >> 14);
    e2 = (u32)((((cosr * sinp) >> 15) * siny) >> 14) -
         (u32)((sinr * cosy) >> 14);
    mtx->m[1][0] = COMBINE_INTEGRAL(e1, e2);
    mtx->m[3][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)((cosr * cosp) >> 14);
    mtx->m[1][1] = COMBINE_INTEGRAL(e1, 0u);
    mtx->m[3][1] = COMBINE_FRACTIONAL(e1, 0u);

    mtx->m[1][2] = COMBINE_INTEGRAL((u32)fixed_x, (u32)fixed_y);
    mtx->m[3][2] = COMBINE_FRACTIONAL((u32)fixed_x, (u32)fixed_y);
    mtx->m[1][3] = COMBINE_INTEGRAL((u32)fixed_z, 0x00010000u);
    mtx->m[3][3] = COMBINE_FRACTIONAL((u32)fixed_z, 0x00010000u);
    return TRUE;
}

/* R2-03 E10. Same observation as E9, applied to the branch E9 did not touch.
 * This builder already works in fixed point -- the sin/cos table lookup E6
 * proposed exists here already -- and then packs its s16.16 results into the N64
 * split format purely so MtxFromN64 can unpack them again. The pairing is the
 * same as F2LFixedWExact's, so the cell mapping is again the identity, and
 * writing 20.12 straight out is bit-exact by construction.
 *
 * Kept as a separate function rather than parameterising the original: the two
 * differ only in their store, and a shared version would need a branch per cell
 * in the hottest loop of the phase. */
/* 2026-09-27: out of ITCM. Only the old fighter path (route 0 and declined
 * lean draws) reaches it; the four-CPU census never executed it, and its
 * 2,056 B now hold the MP floor query and the pose update. */
static NDS_RENDERER_ADAPTER_FIGHTER_MATRIX_CODE sb32
ndsRendererAdapterBuildFighterTraRotRpyDirect20p12(
    NDSRendererMatrix20p12 *dst,
    f32 tx,
    f32 ty,
    f32 tz,
    f32 r,
    f32 p,
    f32 y)
{
    s32 indexr;
    s32 indexp;
    s32 indexy;
    s32 sinr;
    s32 sinp;
    s32 siny;
    s32 cosr;
    s32 cosp;
    s32 cosy;
    s32 fixed_x;
    s32 fixed_y;
    s32 fixed_z;

    if ((dst == NULL) ||
        (ndsFighterMatrixAngleToIndexExact(r, &indexr) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(p, &indexp) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(y, &indexy) == 0) ||
        (ndsRendererAdapterFloatPow2ToS32(tx, 16u, &fixed_x) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(ty, 16u, &fixed_y) == FALSE) ||
        (ndsRendererAdapterFloatPow2ToS32(tz, 16u, &fixed_z) == FALSE))
    {
        return FALSE;
    }

    sinr = ndsRendererAdapterFighterSinFromIndex(indexr);
    cosr = ndsRendererAdapterFighterCosFromIndex(indexr);
    sinp = ndsRendererAdapterFighterSinFromIndex(indexp);
    cosp = ndsRendererAdapterFighterCosFromIndex(indexp);
    siny = ndsRendererAdapterFighterSinFromIndex(indexy);
    cosy = ndsRendererAdapterFighterCosFromIndex(indexy);

#define NDS_RS20P12(value) ndsRendererAdapterRoundShift20p12((s32)(value))

    dst->m[0][0] = NDS_RS20P12((cosp * cosy) >> 14);
    dst->m[0][1] = NDS_RS20P12((cosp * siny) >> 14);
    dst->m[0][2] = NDS_RS20P12(-sinp * 2);
    dst->m[0][3] = 0;

    dst->m[1][0] = NDS_RS20P12(
        ((((sinr * sinp) >> 15) * cosy) >> 14) - ((cosr * siny) >> 14));
    dst->m[1][1] = NDS_RS20P12(
        ((((sinr * sinp) >> 15) * siny) >> 14) + ((cosr * cosy) >> 14));
    dst->m[1][2] = NDS_RS20P12((sinr * cosp) >> 14);
    dst->m[1][3] = 0;

    dst->m[2][0] = NDS_RS20P12(
        ((((cosr * sinp) >> 15) * cosy) >> 14) + ((sinr * siny) >> 14));
    dst->m[2][1] = NDS_RS20P12(
        ((((cosr * sinp) >> 15) * siny) >> 14) - ((sinr * cosy) >> 14));
    dst->m[2][2] = NDS_RS20P12((cosr * cosp) >> 14);
    dst->m[2][3] = 0;

    dst->m[3][0] = NDS_RS20P12(fixed_x);
    dst->m[3][1] = NDS_RS20P12(fixed_y);
    dst->m[3][2] = NDS_RS20P12(fixed_z);
    dst->m[3][3] = NDS_RS20P12(0x00010000);

#undef NDS_RS20P12
    return TRUE;
}

static void ndsRendererAdapterBuildDObjFallbackMtx(DObj *dobj, Mtx *mtx)
{
    if ((dobj == NULL) || (mtx == NULL))
    {
        return;
    }

    syMatrixTraRotRpyRSca(mtx,
                          dobj->translate.vec.f.x,
                          dobj->translate.vec.f.y,
                          dobj->translate.vec.f.z,
                          dobj->rotate.vec.f.x,
                          dobj->rotate.vec.f.y,
                          dobj->rotate.vec.f.z,
                          dobj->scale.vec.f.x,
                          dobj->scale.vec.f.y,
                          dobj->scale.vec.f.z);
}

static f32 ndsRendererAdapterSquareF(f32 value)
{
    return value * value;
}

static void ndsRendererAdapterBuildBillboardMtxF(Mtx44f *mtx_f,
                                                 DObj *dobj,
                                                 u32 kind)
{
    CObj *cobj;
    f32 distx;
    f32 disty;
    f32 distz;
    f32 res;
    u32 is_translate;

    if ((mtx_f == NULL) || (dobj == NULL))
    {
        return;
    }

    memset(mtx_f, 0, sizeof(*mtx_f));
    (*mtx_f)[0][0] = 1.0F;
    (*mtx_f)[1][1] = 1.0F;
    (*mtx_f)[2][2] = 1.0F;
    (*mtx_f)[3][3] = 1.0F;

    cobj = (gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL;
    if (cobj == NULL)
    {
        return;
    }

    is_translate = (((kind & 1u) == 0u) ? TRUE : FALSE);
    distx = dobj->translate.vec.f.x - cobj->vec.eye.x;
    disty = dobj->translate.vec.f.y - cobj->vec.eye.y;
    distz = dobj->translate.vec.f.z - cobj->vec.eye.z;

    switch (kind)
    {
    case 33:
    case 34:
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(disty));
        (*mtx_f)[2][2] = 1.0F;
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            distx *= inv;
            disty *= inv;
            (*mtx_f)[0][0] = -distx;
            (*mtx_f)[0][1] = -disty;
            (*mtx_f)[1][0] = disty;
            (*mtx_f)[1][1] = -distx;
        }
        break;
    case 35:
    case 36:
        res = 1.0F / sqrtf(ndsRendererAdapterSquareF(distx) +
                           ndsRendererAdapterSquareF(disty) +
                           ndsRendererAdapterSquareF(distz));
        distx *= res;
        disty *= res;
        distz *= res;
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(disty));
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            (*mtx_f)[2][2] = res;
            (*mtx_f)[0][0] = -distx;
            (*mtx_f)[1][0] = disty * inv;
            (*mtx_f)[2][0] = -distx * distz * inv;
            (*mtx_f)[0][1] = -disty;
            (*mtx_f)[1][1] = -distx * inv;
            (*mtx_f)[2][1] = -disty * distz * inv;
            (*mtx_f)[0][2] = -distz;
        }
        break;
    case 37:
    case 38:
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(distz));
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            distx *= inv;
            distz *= inv;
            (*mtx_f)[0][2] = distx;
            (*mtx_f)[2][0] = -distx;
            (*mtx_f)[0][0] = -distz;
            (*mtx_f)[2][2] = -distz;
        }
        break;
    case 39:
    case 40:
        res = 1.0F / sqrtf(ndsRendererAdapterSquareF(distx) +
                           ndsRendererAdapterSquareF(disty) +
                           ndsRendererAdapterSquareF(distz));
        distx *= res;
        disty *= res;
        distz *= res;
        res = sqrtf(ndsRendererAdapterSquareF(distx) +
                    ndsRendererAdapterSquareF(distz));
        if (res != 0.0F)
        {
            f32 inv = 1.0F / res;

            (*mtx_f)[0][0] = -distz * inv;
            (*mtx_f)[1][0] = -disty * distx * inv;
            (*mtx_f)[2][0] = -distx;
            (*mtx_f)[1][1] = res;
            (*mtx_f)[2][1] = -disty;
            (*mtx_f)[0][2] = distx * inv;
            (*mtx_f)[1][2] = -disty * distz * inv;
            (*mtx_f)[2][2] = -distz;
        }
        break;
    default:
        break;
    }

    if (is_translate != FALSE)
    {
        (*mtx_f)[3][0] = dobj->translate.vec.f.x;
        (*mtx_f)[3][1] = dobj->translate.vec.f.y;
        (*mtx_f)[3][2] = dobj->translate.vec.f.z;
    }
}

static sb32 ndsRendererAdapterBuildBillboardMtx(DObj *dobj, u32 kind,
                                                Mtx *mtx)
{
    Mtx44f mtx_f;

    if ((dobj == NULL) || (mtx == NULL))
    {
        return FALSE;
    }

    ndsRendererAdapterBuildBillboardMtxF(&mtx_f, dobj, kind);
    syMatrixF2LFixedW(&mtx_f, mtx);
    return TRUE;
}

static sb32 ndsRendererAdapterBuildRecalcLocalMtx(DObj *dobj, u32 kind,
                                                  Mtx *mtx)
{
    if ((dobj == NULL) || (mtx == NULL))
    {
        return FALSE;
    }

    switch (kind)
    {
    case nGCMatrixKindRecalcRotPyrR:
        syMatrixRotPyrR(mtx,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRecalcRotRpyR:
        syMatrixRotRpyR(mtx,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRecalcRotPyrRSca:
        syMatrixTraRotPyrRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindRecalcRotRpyRSca:
        syMatrixTraRotRpyRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKind45:
        syMatrixTraRotPyrRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              dobj->rotate.vec.f.x, 0.0F, 0.0F,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKind46:
        syMatrixTraRotRpyRSca(mtx,
                              0.0F, 0.0F, 0.0F,
                              0.0F, 0.0F, dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKind47:
    case nGCMatrixKind48:
    case nGCMatrixKind49:
    case nGCMatrixKind50:
        syMatrixSca(mtx,
                    dobj->scale.vec.f.x,
                    dobj->scale.vec.f.y,
                    dobj->scale.vec.f.z);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static sb32 ndsRendererAdapterBuildFighterPartsMtx(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    FTParts *parts;
    Mtx mtx;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    parts = ftGetParts(dobj);
    if (parts == NULL)
    {
        return FALSE;
    }

    if (parts->transform_update_mode != 0)
    {
        {
            NDSRendererMatrix20p12 direct;

            if (ndsRendererAdapterF2LDirect20p12(
                    &parts->unk_dobjtrans_0x10, &direct) != FALSE)
            {
                *out = direct;
                return TRUE;
            }
        }
        /* BattleShip lbCommonFighterPartsFuncMatrix quantizes the complete
         * source float matrix through syMatrixF2LFixedW before the RSP sees
         * it. Preserve that 16.16 boundary, then convert to DS 20.12. */
        if (ndsRendererAdapterF2LFixedWExact(
                &parts->unk_dobjtrans_0x10, &mtx) == FALSE)
        {
            syMatrixF2LFixedW(&parts->unk_dobjtrans_0x10, &mtx);
        }
    }
    else if (ndsFcmpNeC(dobj->scale.vec.f.x, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.y, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.z, 1.0F))
    {
        /* Bit-exact pattern compares (nds_fcmp.h): three __aeabi_fcmpeq
         * calls per unscaled joint otherwise, 76,616 executions a profile. */
        syMatrixTraRotRpyRSca(&mtx,
                              dobj->translate.vec.f.x,
                              dobj->translate.vec.f.y,
                              dobj->translate.vec.f.z,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
    }
    else
    {
        {
            NDSRendererMatrix20p12 direct;

            if (ndsRendererAdapterBuildFighterTraRotRpyDirect20p12(
                    &direct,
                    dobj->translate.vec.f.x,
                    dobj->translate.vec.f.y,
                    dobj->translate.vec.f.z,
                    dobj->rotate.vec.f.x,
                    dobj->rotate.vec.f.y,
                    dobj->rotate.vec.f.z) != FALSE)
            {
                *out = direct;
                return TRUE;
            }
        }
        if (ndsRendererAdapterBuildFighterTraRotRpyExact(
                &mtx,
                dobj->translate.vec.f.x,
                dobj->translate.vec.f.y,
                dobj->translate.vec.f.z,
                dobj->rotate.vec.f.x,
                dobj->rotate.vec.f.y,
                dobj->rotate.vec.f.z) == FALSE)
        {
            syMatrixTraRotRpyR(&mtx,
                               dobj->translate.vec.f.x,
                               dobj->translate.vec.f.y,
                               dobj->translate.vec.f.z,
                               dobj->rotate.vec.f.x,
                               dobj->rotate.vec.f.y,
                               dobj->rotate.vec.f.z);
        }
    }
    ndsRendererAdapterMtxFromN64(&mtx, out);
    return TRUE;
}

/* Matrix kind 0x4F, the joint-attach transform. It is not an XObjTransformKind
 * enumerator: source routes every kind >= 66 through
 * sGCMatrixFuncList[kind - 66] (objdisplay.c:1161), and the battle task installs
 * dLBCommonFuncMatrixList, whose pair 13 is func_ovl0_800C994C
 * (lbcommon.c:1445). That callback ignores the DObj's own translate/rotate/scale
 * entirely and loads the matrix from the world matrix of the joint stored in
 * dobj->user_data.p:
 *
 *     attach = dobj->user_data.p;  parts = attach->user_data.p;
 *     func_ovl2_800EDBA4(attach);
 *     gmCollisionCopyMatrix(f, parts->mtx_translate);
 *     syMatrixF2LFixedW(&f, mtx);
 *
 * dEFManagerShieldEffectDesc's root DObj uses it (efmanager.c:462) and
 * efManagerShieldMakeEffect stores fp->joints[nFTPartsJointYRotN] there
 * (efmanager.c:4139). That is the whole attachment mechanism -- the shield has no
 * update proc that moves it, so without this case the root fell to
 * ndsRendererAdapterBuildDObjFallbackMtx, which for its identity vectors builds
 * the identity and parks the bubble at the world origin. Measured 2026-08-04 on
 * builds/build-c50-flag1 inside the tic 1994-1982 guard: k0=0x4f, user_data.p
 * non-NULL, root translate/rotate 0 and scale 1.
 *
 * func_ovl2_800EDBA4 FIRST, exactly as source does, and not as a formality:
 * mtx_translate is a per-frame cache that ndsFTParamsInvalidateFighterParts
 * clears, and the fighter's own draw (kind 0x4B) reads unk_dobjtrans_0x10 rather
 * than filling it, so the same probe read mtx_translate as all zeros. Copying it
 * without the rebuild would collapse the bubble to a point rather than move it.
 *
 * gmCollisionCopyMatrix's copy into a local is skipped: it writes columns 0-2
 * only, which is exactly what F2LFixedW reads. */
/* SOURCE'S gGCScaleX, AND IT IS NOT BOOKKEEPING. func_ovl0_800C994C's middle
 * line is `gGCScaleX = sqrtf(SQUARE(f[0][0]) + SQUARE(f[0][1]) +
 * SQUARE(f[0][2]))` -- the length of row 0 of the joint's world matrix -- and
 * the kind-44 billboard below multiplies the perspective by it. For the shield
 * that length is the SHIELD SIZE: ftCommonGuardUpdateShieldCollision writes
 * `((0.65 * shield_health/55) + 0.35) * attr->shield_size / 30` into
 * fp->joints[nFTPartsJointYRotN]->scale (ftcommonguard1.c:125), which for Fox
 * at full health is 280/30 = 9.3333 -- exactly the row-0 length this probe
 * measures. Dropping it makes the bubble a five-pixel dot, because the billboard
 * then scales the model by 1.0 instead of by the guard's own size.
 *
 * gcDrawDObjTreeDLLinksForGObj resets it to 1.0 per tree, which
 * ndsRendererAdapterPrepareInitialMatrices mirrors: the world build that calls
 * this runs first, the recalc that consumes it runs after, both inside one
 * call. */
static f32 sNdsRendererAdapterMvpRecalcScaleX = 1.0F;

static sb32 ndsRendererAdapterBuildJointAttachMtx(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *attach;
    FTParts *parts;
    Mtx mtx;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    attach = (DObj *)dobj->user_data.p;
    if ((attach == NULL) || (attach == DOBJ_PARENT_NULL) ||
        (attach->parent_gobj == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(attach);
    if (parts == NULL)
    {
        return FALSE;
    }
    /* func_ovl2_800EDBA4 walks to the joint root and dereferences ftGetStruct and
     * ftGetParts without a guard, so both are checked above before the call. */
    func_ovl2_800EDBA4(attach);
    sNdsRendererAdapterMvpRecalcScaleX =
        sqrtf((parts->mtx_translate[0][0] * parts->mtx_translate[0][0]) +
              (parts->mtx_translate[0][1] * parts->mtx_translate[0][1]) +
              (parts->mtx_translate[0][2] * parts->mtx_translate[0][2]));
    if (ndsRendererAdapterF2LFixedWExact(&parts->mtx_translate, &mtx) == FALSE)
    {
        syMatrixF2LFixedW(&parts->mtx_translate, &mtx);
    }
    ndsRendererAdapterMtxFromN64(&mtx, out);
    return TRUE;
}

/* Matrix kind 0x50, func_ovl0_800C99CC. Same attachment DATA as 0x4F -- the
 * joint DObj in user_data.p -- and deliberately different maths: source takes
 * only the joint's world POSITION and builds a pure translation, so the halo
 * neither inherits the fighter's yaw nor scales with him. Its size comes from
 * the child's own scale, which efManagerRebirthHaloMakeEffect writes from the
 * maker's argument (efmanager.c:6017).
 *
 * gmCollisionGetFighterPartsWorldPosition dereferences
 * ftGetStruct(main_dobj->parent_gobj) before testing anything, so the same two
 * guards the 0x4F builder uses are required here. It also refreshes the
 * per-frame transform cache itself (gmCollisionTransformMatrixAll when
 * transform_update_mode is 0), which is why this path needs no separate
 * func_ovl2_800EDBA4 call.
 *
 * Fills the caller's Mtx rather than the 20.12 output, so it shares the common
 * ndsRendererAdapterMtxFromN64 tail with every other syMatrix* case. */
static sb32 ndsRendererAdapterBuildJointAttachTraMtx(DObj *dobj, Mtx *out)
{
    DObj *attach;
    Vec3f translate_base = { 0.0F, 0.0F, 0.0F };

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    attach = (DObj *)dobj->user_data.p;
    if ((attach == NULL) || (attach == DOBJ_PARENT_NULL) ||
        (attach->parent_gobj == NULL) || (ftGetParts(attach) == NULL))
    {
        return FALSE;
    }
    gmCollisionGetFighterPartsWorldPosition(attach, &translate_base);
    if (ndsEffectTowardEye(dobj) != FALSE)
    {
        /* nds_effects.h: Ness's PSI Magnet, Falcon's punch and kick and
         * Ganondorf's, drawn toward the camera eye by the shield's floor so
         * the fighter's own body stops hiding them. Translation only; the
         * source pose and position are untouched. */
        CObj *cobj = (gGCCurrentCamera != NULL) ?
            CObjGetStruct(gGCCurrentCamera) : NULL;

        if (cobj != NULL)
        {
            f32 dx = cobj->vec.eye.x - translate_base.x;
            f32 dy = cobj->vec.eye.y - translate_base.y;
            f32 dz = cobj->vec.eye.z - translate_base.z;
            f32 len = sqrtf((dx * dx) + (dy * dy) + (dz * dz));

            if (len > 1.0F)
            {
                f32 step = 150.0F / len;

                translate_base.x += dx * step;
                translate_base.y += dy * step;
                translate_base.z += dz * step;
            }
        }
    }
    syMatrixTra(out, translate_base.x, translate_base.y, translate_base.z);
    return TRUE;
}

/* Matrix kind 0x4A, func_ovl0_800CB2F0: no matrix, one write. The source
 * contract is at NDS_RENDERER_ADAPTER_JOINT_PITCH_TO_ROLL_KIND. The guards are
 * the 0x4F builder's for the same reason: func_ovl2_800EDBA4 dereferences
 * ftGetStruct(attach->parent_gobj) and the FTParts of every joint up to the
 * root without testing them. A declined guard writes nothing, where source
 * would fault. noinline: one effect's path stays out of the shared XObj switch,
 * and out of ITCM should that switch ever inline into its ITCM caller. */
static void __attribute__((noinline))
ndsRendererAdapterCopyJointPitchToRoll(DObj *dobj)
{
    DObj *attach = (DObj *)dobj->user_data.p;
    FTParts *parts;

    if ((attach == NULL) || (attach == DOBJ_PARENT_NULL) ||
        (attach->parent_gobj == NULL))
    {
        return;
    }
    parts = ftGetParts(attach);
    if (parts == NULL)
    {
        return;
    }
    func_ovl2_800EDBA4(attach);
    dobj->rotate.vec.f.z = (parts->mtx_translate[0][2] > 0.0F) ?
        attach->rotate.vec.f.x : -attach->rotate.vec.f.x;
    sNdsRendererAdapterCustom4ARollDObj = dobj;
    gNdsRendererAdapterCustom4AWitness[0]++;
    DC_FlushRange((const void *)(uintptr_t)gNdsRendererAdapterCustom4AWitness,
                  sizeof(gNdsRendererAdapterCustom4AWitness));
}

/* BattleShip lbcommon.c:1477-1604 (func_ovl0_800C9A38) + :1619-1634
 * (func_ovl0_800C9F70), specialized only at the final output boundary for the
 * DS renderer.  This is the 0x52 matrix installed on a held item's root by
 * itMainSetFighterHold.  The source deliberately removes joint/parent scale,
 * preserves world orientation + translation, then folds hitlag shuffle before
 * converting to fixed matrix form.  LinkBomb is the first live item client. */
/* S2 (2026-09-28): held items whose attach joint was freed. */
volatile u32 gNdsRendererAdapterItemAttachStale;
static sb32 ndsRendererAdapterBuildItemAttachMtx(DObj *dobj, Mtx *out)
{
    DObj *attach_dobj;
    DObj *parent_dobj;
    FTParts *parts;
    FTStruct *fp;
    Mtx44f *p;
    Mtx44f f;
    Mtx44f item_world;
    f32 scale;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    attach_dobj = (DObj *)dobj->user_data.p;
    if ((attach_dobj == NULL) || (attach_dobj == DOBJ_PARENT_NULL) ||
        (attach_dobj->parent_gobj == NULL))
    {
        return FALSE;
    }
    /* S2: the source keeps a held item's attach pointer when the fighter
     * joint it names is ejected, and the freed DObj is recycled. With
     * Ness/Yoshi/Pikachu/Purin on Zebes the item's joint belonged to an
     * Effect by frame 1920; its user_data (read as FTParts) was a WRAM DObj
     * whose matrix ran past the 32 KB WRAM region and data-aborted. N64 reads
     * the same garbage without a fault. A joint that no longer belongs to a
     * fighter takes the fallback matrix (render only). */
    if (attach_dobj->parent_gobj->id != nGCCommonKindFighter)
    {
        NDS_DIAG(gNdsRendererAdapterItemAttachStale++);
        return FALSE;
    }
    parts = ftGetParts(attach_dobj);
    fp = ftGetStruct(attach_dobj->parent_gobj);
    if ((parts == NULL) || (fp == NULL))
    {
        return FALSE;
    }

#define NDS_ITEM_ATTACH_NORMALIZE_ROW(dst, src, row) do { \
    scale = sqrtf(SQUARE((src)[row][0]) + SQUARE((src)[row][1]) + \
                  SQUARE((src)[row][2])); \
    if (scale != 0.0F) { scale = 1.0F / scale; } \
    (dst)[row][0] = (src)[row][0] * scale; \
    (dst)[row][1] = (src)[row][1] * scale; \
    (dst)[row][2] = (src)[row][2] * scale; \
} while (0)

    if ((fp->is_use_animlocks != FALSE) ||
        (attach_dobj->parent == DOBJ_PARENT_NULL))
    {
        func_ovl2_800EDBA4(attach_dobj);
        p = &parts->mtx_translate;
        NDS_ITEM_ATTACH_NORMALIZE_ROW(item_world, (*p), 0);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(item_world, (*p), 1);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(item_world, (*p), 2);
        item_world[3][0] = (*p)[3][0];
        item_world[3][1] = (*p)[3][1];
        item_world[3][2] = (*p)[3][2];
    }
    else
    {
        parent_dobj = attach_dobj->parent;
        if ((parent_dobj == NULL) || (parent_dobj == DOBJ_PARENT_NULL) ||
            (ftGetParts(parent_dobj) == NULL))
        {
            return FALSE;
        }
        gmCollisionTransformMatrixAll(attach_dobj, parts,
                                      parts->unk_dobjtrans_0x10);
        p = &parts->unk_dobjtrans_0x10;
        NDS_ITEM_ATTACH_NORMALIZE_ROW(f, (*p), 0);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(f, (*p), 1);
        NDS_ITEM_ATTACH_NORMALIZE_ROW(f, (*p), 2);
        f[3][0] = (*p)[3][0];
        f[3][1] = (*p)[3][1];
        f[3][2] = (*p)[3][2];

        func_ovl2_800EDBA4(parent_dobj);
        p = &ftGetParts(parent_dobj)->mtx_translate;
        scale = sqrtf(SQUARE((*p)[0][0]) + SQUARE((*p)[0][1]) +
                      SQUARE((*p)[0][2]));
        if (scale != 0.0F) { scale = 1.0F / scale; }
        f[0][0] *= scale; f[1][0] *= scale; f[2][0] *= scale;
        scale = sqrtf(SQUARE((*p)[1][0]) + SQUARE((*p)[1][1]) +
                      SQUARE((*p)[1][2]));
        if (scale != 0.0F) { scale = 1.0F / scale; }
        f[0][1] *= scale; f[1][1] *= scale; f[2][1] *= scale;
        scale = sqrtf(SQUARE((*p)[2][0]) + SQUARE((*p)[2][1]) +
                      SQUARE((*p)[2][2]));
        if (scale != 0.0F) { scale = 1.0F / scale; }
        f[0][2] *= scale; f[1][2] *= scale; f[2][2] *= scale;
        func_ovl2_800ED490(item_world, *p, f);
    }

#undef NDS_ITEM_ATTACH_NORMALIZE_ROW

    if (fp->shuffle_tics != 0u)
    {
        const Vec2f *offset =
            &dFTDisplayMainShufflePositions[fp->is_shuffle_electric]
                                           [fp->shuffle_frame_index];
        item_world[3][0] += offset->x;
        item_world[3][1] += offset->y;
    }
    syMatrixF2LFixedW(&item_world, out);
    NDS_DIAG(gNdsItemRendererAttach52BuildCount++);
    return TRUE;
}

#if NDS_JOINT_RESIDENT
/* The 0x52 matrix from the resident fixed-point fighter world (owner ruling
 * D13): the float builder's guards, then its non-root, non-animlock branch in
 * fixed point straight to the Q20.12 the GX takes. No FTParts latch is written
 * (the float branch latches the attach local and the parent chain). FALSE =
 * the float builder above decides, as before. */
static sb32 ndsRendererAdapterBuildItemAttachResident(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *attach_dobj;
    FTStruct *fp;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    attach_dobj = (DObj *)dobj->user_data.p;
    if ((attach_dobj == NULL) || (attach_dobj == DOBJ_PARENT_NULL) ||
        (attach_dobj->parent_gobj == NULL) ||
        (attach_dobj->parent_gobj->id != nGCCommonKindFighter) ||
        (ftGetParts(attach_dobj) == NULL))
    {
        return FALSE;
    }
    fp = ftGetStruct(attach_dobj->parent_gobj);
    if ((fp == NULL) ||
        (ndsJointItemAttach(
             attach_dobj,
             (fp->shuffle_tics != 0u) ?
                 &dFTDisplayMainShufflePositions[fp->is_shuffle_electric]
                                                [fp->shuffle_frame_index] :
                 NULL,
             out->m) == 0))
    {
        return FALSE;
    }
    NDS_DIAG(gNdsItemRendererAttach52BuildCount++);
    return TRUE;
}
#endif

static void ndsRendererAdapterGetDObjVectorTracks(
    DObj *dobj,
    GCTranslate **translate,
    GCRotate **rotate,
    GCScale **scale)
{
    uintptr_t cursor;
    u32 i;

    if ((dobj == NULL) || (translate == NULL) || (rotate == NULL) ||
        (scale == NULL))
    {
        return;
    }

    *translate = &dobj->translate;
    *rotate = &dobj->rotate;
    *scale = &dobj->scale;

    if (dobj->vec == NULL)
    {
        return;
    }

    cursor = (uintptr_t)dobj->vec->data;
    for (i = 0u; i < 3u; i++)
    {
        switch (dobj->vec->kinds[i])
        {
        case nGCDrawVectorKindTranslate:
            *translate = (GCTranslate *)cursor;
            cursor += sizeof(GCTranslate);
            break;
        case nGCDrawVectorKindRotate:
            *rotate = (GCRotate *)cursor;
            cursor += sizeof(GCRotate);
            break;
        case nGCDrawVectorKindScale:
            *scale = (GCScale *)cursor;
            cursor += sizeof(GCScale);
            break;
        default:
            break;
        }
    }
}

#if NDS_STAGE_SECTOR
/* BattleShip grsector.c:230-274 (func_ovl2_80106730). The argument order is
 * intentionally the source order: vec1 becomes matrix row 2, vec2 row 0 and
 * vec3 row 1. Both grSectorArwingLaser3DFuncMatrix (:304, :306-314) and the
 * independent 3D-laser spawn matrix (:817, :819-829) use this permutation. */
static sb32 ndsRendererAdapterSectorArwingBasis(
    DObj *dobj, Vec3f *vec1, Vec3f *vec2, Vec3f *vec3)
{
    DObj *path_dobj;
    AObj *aobj;
    f32 vlen = 0.0F;
    sb32 noanim;

    /* BattleShip reaches func_ovl2_80106730 only from a live Sector Arwing
     * callback, where both of these owners exist. The DS matrix adapter can
     * encounter the 0x53 XObj after an allocation/animation admission failure,
     * so preserve that source precondition explicitly before dereferencing it.
     * The caller keeps drawing a source-derived fixed basis and this witness
     * makes the broken lifetime loud instead of converting it into a missing
     * actor or a CPU walk through bad RAM. */
    if (dobj->parent_gobj == NULL)
    {
        gNdsSectorArwingBasisDecline = 1u;
        return FALSE;
    }
    path_dobj = ndsGRSectorArwingMapDObj11();
    if (path_dobj == NULL)
    {
        gNdsSectorArwingBasisDecline = 2u;
        return FALSE;
    }
    noanim = ((dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) != 0u) ? TRUE : FALSE;
    aobj = dobj->aobj;

    while (aobj != NULL)
    {
        if ((aobj->kind != nGCAnimKindNone) &&
            (noanim == FALSE) &&
            (aobj->track == nGCAnimTrackTraI))
        {
            if (ndsTraIDescUsable(dobj, aobj, 3u) == FALSE)
            {
                gNdsSectorArwingBasisDecline = 3u;
                return FALSE;
            }
            vlen = gcGetAObjValue(aobj);
            if (vlen < 0.0F)
            {
                vlen = 0.0F;
            }
            else if (vlen > 1.0F)
            {
                vlen = 1.0F;
            }
            syInterpQuad(vec1, aobj->interpolate, vlen);
        }
        aobj = aobj->next;
    }

    if ((ndsGRSectorArwingLaserCount() == 0) &&
        (path_dobj->anim_wait != AOBJ_ANIM_NULL))
    {
        aobj = path_dobj->aobj;
        while (aobj != NULL)
        {
            if ((aobj->kind != nGCAnimKindNone) &&
                (noanim == FALSE) &&
                (aobj->track == nGCAnimTrackTraI))
            {
                if (ndsTraIDescUsable(dobj, aobj, 3u) == FALSE)
                {
                    gNdsSectorArwingBasisDecline = 3u;
                    return FALSE;
                }
                syInterpCubic(vec3, aobj->interpolate, vlen);
            }
            aobj = aobj->next;
        }
        syVectorNorm3D(vec3);
    }
    lbCommonCross3D(vec3, vec1, vec2);
    lbCommonCross3D(vec1, vec2, vec3);
    syVectorNorm3D(vec1);
    syVectorNorm3D(vec2);
    syVectorNorm3D(vec3);
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterBuildDObjXObjMatrix(
    DObj *dobj, XObj *xobj, NDSRendererMatrix20p12 *out)
{
    Mtx mtx;
    GCTranslate *translate = NULL;
    GCRotate *rotate = NULL;
    GCScale *scale = NULL;

    if ((dobj == NULL) || (xobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    /* Only the nGCMatrixKindVec* family reads these tracks; every other kind
     * takes its values straight off the DObj. The gather was unconditional and
     * walked the DObj's GCDrawVector list on every builder call -- 996
     * ticks/frame over 340,237 executions in the c106 profile. The family is
     * contiguous (nGCMatrixKindVecTra = 56 .. nGCMatrixKindVecTraRotRpyRSca =
     * 63), so the gate is a subtract and a compare. */
    if (((u32)xobj->kind - (u32)nGCMatrixKindVecTra) <=
        ((u32)nGCMatrixKindVecTraRotRpyRSca - (u32)nGCMatrixKindVecTra))
    {
        ndsRendererAdapterGetDObjVectorTracks(
            dobj, &translate, &rotate, &scale);
    }

    switch (xobj->kind)
    {
    case 1:
        /* Convert in place: the 64-byte struct copy into `mtx` was the
         * builder's only memcpy (13,810 calls a profile, 2026-08-23 census)
         * and the converter reads its source exactly once. */
        ndsRendererAdapterMtxFromN64(&xobj->mtx, out);
        return TRUE;
    case 2:
        return FALSE;
    case NDS_RENDERER_ADAPTER_MVP_RECALC_RPY_0X47_KIND:
        /* This source callback rewrites the current MVP rotation rows after
         * the ordinary translation matrix. It is applied after world*camera
         * composition, not multiplied as another affine local transform. */
        return FALSE;
    case nGCMatrixKindTra:
        syMatrixTra(&mtx, dobj->translate.vec.f.x,
                    dobj->translate.vec.f.y,
                    dobj->translate.vec.f.z);
        break;
    case nGCMatrixKindRotD:
        syMatrixRotD(&mtx, dobj->rotate.a, dobj->rotate.vec.f.x,
                     dobj->rotate.vec.f.y, dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotD:
        syMatrixTraRotD(&mtx, dobj->translate.vec.f.x,
                        dobj->translate.vec.f.y,
                        dobj->translate.vec.f.z,
                        dobj->rotate.a,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRotRpyD:
        syMatrixRotRpyD(&mtx, dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRpyD:
        syMatrixTraRotRpyD(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindRotR:
        syMatrixRotR(&mtx, dobj->rotate.a, dobj->rotate.vec.f.x,
                     dobj->rotate.vec.f.y, dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotR:
        syMatrixTraRotR(&mtx, dobj->translate.vec.f.x,
                        dobj->translate.vec.f.y,
                        dobj->translate.vec.f.z,
                        dobj->rotate.a,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRSca:
        syMatrixTraRotRSca(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.a,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z,
                           dobj->scale.vec.f.x,
                           dobj->scale.vec.f.y,
                           dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindRotRpyR:
        syMatrixRotRpyR(&mtx, dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRpyR:
        syMatrixTraRotRpyR(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotRpyRSca:
        syMatrixTraRotRpyRSca(&mtx, dobj->translate.vec.f.x,
                              dobj->translate.vec.f.y,
                              dobj->translate.vec.f.z,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindRotPyrR:
        syMatrixRotPyrR(&mtx, dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotPyrR:
        syMatrixTraRotPyrR(&mtx, dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
        break;
    case nGCMatrixKindTraRotPyrRSca:
        syMatrixTraRotPyrRSca(&mtx, dobj->translate.vec.f.x,
                              dobj->translate.vec.f.y,
                              dobj->translate.vec.f.z,
                              dobj->rotate.vec.f.x,
                              dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z,
                              dobj->scale.vec.f.x,
                              dobj->scale.vec.f.y,
                              dobj->scale.vec.f.z);
        break;
    case nGCMatrixKindSca:
        syMatrixSca(&mtx, dobj->scale.vec.f.x,
                    dobj->scale.vec.f.y,
                    dobj->scale.vec.f.z);
        break;
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
        ndsRendererAdapterBuildBillboardMtx(dobj, xobj->kind, &mtx);
        break;
    case NDS_RENDERER_ADAPTER_ROT_SCA_MTX_KIND:
        /* lbCommonRotScaFuncMatrix: rotate + row scale, ZERO translation.
         *
         * SCOPED TO THE CASE THE EVIDENCE COVERS. L01 is DamageSlash, whose
         * ROOT carries {0x28, 0x45}: 0x28 already puts the world contact in
         * row 3, so taking the translation here as well applies it twice --
         * an error of R_bb*t that grows with distance from the origin and
         * swings with the camera, which is exactly the reported symptom.
         *
         * But 0x45 is ALSO the main transform of a CHILD DObj with no
         * translate-bearing sibling in three other descriptors --
         * StarRodSpark, DamageFlySparks and HaveStruct. Those children took
         * dobj->translate from the old fallback. Dropping it there is
         * defensible from the source, and it is also a behaviour change to
         * three effects this row never examined, on a build where the owner
         * reports effects going missing. So do not make it here: when 0x45 is
         * the only transform on the DObj, keep exactly what shipped before.
         * The narrow fix needs no such gamble.
         *
         * Not added to the DObj-world cache-key ineligibility lists: unlike
         * kinds 33-40 and the attach kinds, this one is a pure function of the
         * DObj's own rotate/scale, which the key already covers. */
        if (dobj->xobjs_num > 1)
        {
            syMatrixTraRotRpyRSca(&mtx, 0.0F, 0.0F, 0.0F,
                                  dobj->rotate.vec.f.x,
                                  dobj->rotate.vec.f.y,
                                  dobj->rotate.vec.f.z,
                                  dobj->scale.vec.f.x,
                                  dobj->scale.vec.f.y,
                                  dobj->scale.vec.f.z);
            NDS_DIAG(gNdsRendererAdapterCustom45AppliedCount++);
        }
        else
        {
            ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
            NDS_DIAG(gNdsRendererAdapterCustom45SoloCount++);
        }
        break;
    case nGCMatrixKindRecalcRotPyrR:
    case nGCMatrixKindRecalcRotRpyR:
    case nGCMatrixKindRecalcRotPyrRSca:
    case nGCMatrixKindRecalcRotRpyRSca:
    case nGCMatrixKind45:
    case nGCMatrixKind46:
    case nGCMatrixKind47:
    case nGCMatrixKind48:
    case nGCMatrixKind49:
    case nGCMatrixKind50:
        ndsRendererAdapterBuildRecalcLocalMtx(dobj, xobj->kind, &mtx);
        break;
    case NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND:
        if (ndsRendererAdapterBuildFighterPartsMtx(dobj, out) != FALSE)
        {
            return TRUE;
        }
        ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        break;
    case NDS_RENDERER_ADAPTER_JOINT_ATTACH_MTX_KIND:
        if (ndsRendererAdapterBuildJointAttachMtx(dobj, out) != FALSE)
        {
            return TRUE;
        }
        ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        break;
    case NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND:
        if (ndsRendererAdapterBuildJointAttachTraMtx(dobj, &mtx) == FALSE)
        {
            ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        }
        break;
    case NDS_RENDERER_ADAPTER_JOINT_PITCH_TO_ROLL_KIND:
        /* Source returns 1: no gSPMatrix, so no local transform either. The
         * roll it writes is consumed by the kind-46 recalc after the world
         * build, in the same ndsRendererAdapterPrepareInitialMatrices. */
        ndsRendererAdapterCopyJointPitchToRoll(dobj);
        return FALSE;
    case NDS_RENDERER_ADAPTER_ITEM_ATTACH_MTX_KIND:
    {
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        u32 lab_attach = cpuGetTiming();
#endif
#if NDS_JOINT_RESIDENT
        const sb32 resident =
            ndsRendererAdapterBuildItemAttachResident(dobj, out);
#else
        const sb32 resident = FALSE;
#endif

        if ((resident == FALSE) &&
            (ndsRendererAdapterBuildItemAttachMtx(dobj, &mtx) == FALSE))
        {
            ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        NDS_DIAG(gNdsLabItemAcc[9] += cpuGetTiming() - lab_attach);
        NDS_DIAG(gNdsLabItemAcc[10]++);
#endif
        if (resident != FALSE)
        {
            return TRUE;
        }
        break;
    }
#if NDS_STAGE_SECTOR
    case NDS_RENDERER_ADAPTER_SECTOR_ARWING_MTX_KIND:
    {
        Vec3f vec1 = { -1.0F, 0.0F, 0.0F };
        /* BattleShip grsector.c:299-302 supplies this same fixed +Z row when
         * arwing_laser_count == 2. It is also the deterministic fallback when
         * the DS adapter catches a broken source-lifetime precondition below;
         * vec2 cannot remain uninitialized on an early basis decline. */
        Vec3f vec2 = { 0.0F, 0.0F, 1.0F };
        Vec3f vec3 = { 0.0F, 1.0F, 0.0F };
        Mtx44f arwing;

        if (ndsGRSectorArwingLaserCount() == 2)
        {
            vec2.x = 0.0F;
            vec2.y = 0.0F;
            vec2.z = 1.0F;
        }
        else
        {
            if (ndsRendererAdapterSectorArwingBasis(
                    dobj, &vec1, &vec2, &vec3) == FALSE)
            {
                /* The helper can have updated vec1 before a later bad TraI
                 * descriptor. Restore all three source-derived fixed rows so
                 * a decline stays deterministic and the Arwing still draws. */
                vec1.x = -1.0F;
                vec1.y = 0.0F;
                vec1.z = 0.0F;
                vec2.x = 0.0F;
                vec2.y = 0.0F;
                vec2.z = 1.0F;
                vec3.x = 0.0F;
                vec3.y = 1.0F;
                vec3.z = 0.0F;
            }
        }

        arwing[0][0] = vec2.x;
        arwing[0][1] = vec2.y;
        arwing[0][2] = vec2.z;
        arwing[1][0] = vec3.x;
        arwing[1][1] = vec3.y;
        arwing[1][2] = vec3.z;
        arwing[2][0] = vec1.x;
        arwing[2][1] = vec1.y;
        arwing[2][2] = vec1.z;
        arwing[0][3] = arwing[1][3] = arwing[2][3] = 0.0F;
        arwing[3][0] = dobj->translate.vec.f.x + ndsGRSectorArwingTargetX();
        arwing[3][1] = dobj->translate.vec.f.y;
        arwing[3][2] = dobj->translate.vec.f.z;
        arwing[3][3] = 1.0F;

        guMtxF2L(arwing, &mtx);
        NDS_DIAG(gNdsRendererAdapterSectorArwingMtxCount++);
        break;
    }
#endif
    case nGCMatrixKindVecTra:
        syMatrixTra(&mtx, translate->vec.f.x,
                    translate->vec.f.y,
                    translate->vec.f.z);
        break;
    case nGCMatrixKindVecRotR:
        syMatrixRotR(&mtx, rotate->a,
                     rotate->vec.f.x,
                     rotate->vec.f.y,
                     rotate->vec.f.z);
        break;
    case nGCMatrixKindVecRotRpyR:
        syMatrixRotRpyR(&mtx,
                        rotate->vec.f.x,
                        rotate->vec.f.y,
                        rotate->vec.f.z);
        break;
    case nGCMatrixKindVecSca:
        syMatrixSca(&mtx, scale->vec.f.x,
                    scale->vec.f.y,
                    scale->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotR:
        syMatrixTraRotR(&mtx,
                        translate->vec.f.x,
                        translate->vec.f.y,
                        translate->vec.f.z,
                        rotate->a,
                        rotate->vec.f.x,
                        rotate->vec.f.y,
                        rotate->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotRSca:
        syMatrixTraRotRSca(&mtx,
                           translate->vec.f.x,
                           translate->vec.f.y,
                           translate->vec.f.z,
                           rotate->a,
                           rotate->vec.f.x,
                           rotate->vec.f.y,
                           rotate->vec.f.z,
                           scale->vec.f.x,
                           scale->vec.f.y,
                           scale->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotRpyR:
        syMatrixTraRotRpyR(&mtx,
                           translate->vec.f.x,
                           translate->vec.f.y,
                           translate->vec.f.z,
                           rotate->vec.f.x,
                           rotate->vec.f.y,
                           rotate->vec.f.z);
        break;
    case nGCMatrixKindVecTraRotRpyRSca:
        syMatrixTraRotRpyRSca(&mtx,
                              translate->vec.f.x,
                              translate->vec.f.y,
                              translate->vec.f.z,
                              rotate->vec.f.x,
                              rotate->vec.f.y,
                              rotate->vec.f.z,
                              scale->vec.f.x,
                              scale->vec.f.y,
                              scale->vec.f.z);
        break;
    default:
        ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        break;
    }

    ndsRendererAdapterMtxFromN64(&mtx, out);
    return TRUE;
}


/* EF-GROUND: kind72 (0x48, func_ovl0_800CAB48, lbcommon.c:1852-1932) row
 * core of the ApplyMvpRecalc kind72 arm, which every efground actor's
 * billboard drawable takes (lr +/-1). Orientation rows from the camera persp x rotX/rotY x scale;
 * scale_accum threads the source's gGCScaleX mutation in walk order. */
/* BEGIN-EF-GROUND-BILLBOARD-ROWS (host-extracted verbatim by
 * scripts/stages/test_ef_ground_billboard_rows.py; keep this body portable:
 * float ops + sinf/cosf only, no renderer types). */
static void ndsRendererAdapterEfGroundBillboardRows(const float persp[4][4],
    float rotx, float roty, float scale_x, float scale_y,
    float *scale_accum, float out_rows[3][4])
{
    float sinx = sinf(rotx);
    float cosx = cosf(rotx);
    float siny = sinf(roty);
    float cosy = cosf(roty);
    float scaley = scale_y * (*scale_accum);
    float scalex = (*scale_accum *= scale_x);
    u32 c;

    for (c = 0u; c < 4u; c++)
    {
        out_rows[0][c] = (persp[0][c] * cosy + persp[2][c] * -siny) * scalex;
        out_rows[1][c] = (persp[0][c] * sinx * siny + persp[1][c] * cosx +
            persp[2][c] * sinx * cosy) * scaley;
        out_rows[2][c] = (persp[0][c] * cosx * siny + persp[1][c] * -sinx +
            persp[2][c] * cosx * cosy) * scalex;
    }
}
/* END-EF-GROUND-BILLBOARD-ROWS */

/* EF-GROUND: kind46 (nGCMatrixKind46, objdisplay.c:960-1004) row core of
 * the ApplyMvpRecalc kind46 arm, the efground actors' lr +/-3 variants
 * (efground.c:1332-1336 arms 0x2E where lr_bool is set). Orientation rows from the camera persp x rotZ x scale; scale_accum
 * threads the source's gGCScaleX mutation in walk order (f12 = scale.y *
 * prior, then the global *= scale.x). Bit-identical to the inline arm it
 * replaces: __sinf/__cosf are sinf/cosf here (n64_stubs.c) and
 * RotRpyR(0,0,z)[0][0..1] is 1.0f * cos/sin, and float * commutes. */
/* BEGIN-EF-GROUND-KIND46-ROWS (host-extracted verbatim by
 * scripts/stages/test_ef_ground_billboard_rows.py; keep this body portable:
 * float ops + sinf/cosf only, no renderer types). */
static void ndsRendererAdapterEfGroundKind46Rows(const float persp[4][4],
    float rotz, float scale_x, float scale_y,
    float *scale_accum, float out_rows[3][4])
{
    float sinz = sinf(rotz);
    float cosz = cosf(rotz);
    float scaley = scale_y * (*scale_accum);
    float scalex = (*scale_accum *= scale_x);

    out_rows[0][0] = persp[0][0] * scalex * cosz;
    out_rows[0][1] = persp[1][1] * scaley * sinz;
    out_rows[0][2] = 0.0F;
    out_rows[0][3] = 0.0F;
    out_rows[1][0] = persp[0][0] * scalex * -sinz;
    out_rows[1][1] = persp[1][1] * scaley * cosz;
    out_rows[1][2] = 0.0F;
    out_rows[1][3] = 0.0F;
    out_rows[2][0] = 0.0F;
    out_rows[2][1] = 0.0F;
    out_rows[2][2] = persp[2][2] * scalex;
    out_rows[2][3] = persp[2][3] * scalex;
}
/* END-EF-GROUND-KIND46-ROWS */

/* THE SOURCE MVP-RECALC CALLBACKS THE PORT IMPLEMENTS. They `continue` out
 * of gcPrepDObjMatrix without emitting a gSPMatrix, so neither contributes a
 * local matrix to the parent chain: they rewrite the COMPOSED MVP instead, and
 * ndsRendererAdapterApplyMvpRecalc is where that happens. A DObj carrying one
 * must therefore skip it in the local build or the transform is applied twice
 * and in the wrong space.
 *
 * Samus made two previously-unmeasured cases observable in P2: built-in kind 46
 * (Charge Shot) and custom kind 0x46 / func_ovl0_800CA194 (Bomb).  Both preserve
 * the already-composed translation row in source; treating either as a normal
 * local rotation rotates the weapon's world translation around the origin.
 *
 * The remaining recalc kinds (41-43, 45, 47, 49-50) still fall through to
 * ndsRendererAdapterBuildRecalcLocalMtx. Each has a different source formula
 * and still has no measured consumer, so leave those untouched rather than
 * converting them on speculation. */
static sb32 ndsRendererAdapterIsMvpRecalcKind(u32 kind)
{
    return ((kind == NDS_RENDERER_ADAPTER_MVP_RECALC_RPY_0X47_KIND) ||
            (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND) ||
            (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND) ||
            (kind == nGCMatrixKind46) || (kind == nGCMatrixKind48) ||
            (kind == NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND)) ?
        TRUE : FALSE;
}

/* A source MVP-recalc XObj (kinds 41-50: gcPrepDObjMatrix emits gSPMvpRecalc
 * and rewrites rows 0-2 of the MVP, objdisplay.c:800) followed in the same
 * DObj by an ordinary transform XObj has no effect in the source: the later
 * XObj's gSPMatrix(MUL | MODELVIEW) makes the RSP recompute the MVP from the
 * modelview stack, which discards the rewrite. The Poke Ball's root is the
 * measured case: its DObjDesc carries flags with Null transform kinds, so
 * gcDecideDObj3TransformsKind gives it RecalcRotPyrR (objanim.c:2318) before
 * itMBallMakeItem adds TraRotRpyR (itmball.c:494) -- { 41, 27 }. Built as a
 * local rotation ahead of the TraRotRpyR, it rotated the ball's world
 * translation about the stage origin by its spin, so a thrown or dropped ball
 * flew off along an arc while the game held it on its real path (owner,
 * BUGS.md Items; 2026-10-07 lab probe: at spin 0.754 a ball at (1186, 200)
 * drew at (727, 958)). */
static sb32 ndsRendererAdapterRecalcXObjSuperseded(const DObj *dobj, u32 index)
{
    const u32 kind = dobj->xobjs[index]->kind;
    u32 j;

    if ((kind < nGCMatrixKindRecalcRotPyrR) || (kind > nGCMatrixKind50))
    {
        return FALSE;
    }
    for (j = index + 1u; j < dobj->xobjs_num; j++)
    {
        if ((dobj->xobjs[j] != NULL) &&
            (dobj->xobjs[j]->kind >= nGCMatrixKindTra) &&
            (dobj->xobjs[j]->kind <= nGCMatrixKindSca))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* P2-2p8 2026-09-27 ITCM swap: out (noinline, so no ITCM caller pulls it
 * back in) for ndsStageGxDraw; census rent under ~1,300 cycles a byte. */
static sb32 __attribute__((noinline))
ndsRendererAdapterBuildDObjLocalMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 incoming;
    u32 valid = FALSE;
    u32 has_mvp_recalc_rpy_0x47 = FALSE;
    u32 i;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        if ((dobj->xobjs[i] != NULL) &&
            (ndsRendererAdapterIsMvpRecalcKind(dobj->xobjs[i]->kind) !=
                FALSE))
        {
            has_mvp_recalc_rpy_0x47 = TRUE;
            continue;
        }
        if ((dobj->xobjs[i] == NULL) ||
            (ndsRendererAdapterRecalcXObjSuperseded(dobj, i) != FALSE))
        {
            continue;
        }
        if (valid == FALSE)
        {
            /* The first contributing xobj is built straight into `out`. It used
             * to land in `incoming` and then be copied, and with a mean of one
             * xobj per DObj that copy was the whole of MulInto's work: 2,266
             * ticks/frame of ndsRendererMatrixCopy20p12 over 1,060,479
             * executions in the c106 profile, plus 64 bytes of stack traffic
             * per call. A FALSE return may leave `out` partly written, which is
             * harmless -- `valid` stays FALSE and the block below overwrites it
             * with the identity or the fallback matrix. */
            if (ndsRendererAdapterBuildDObjXObjMatrix(
                    dobj, dobj->xobjs[i], out) != FALSE)
            {
                valid = TRUE;
            }
        }
        else if (ndsRendererAdapterBuildDObjXObjMatrix(
                     dobj, dobj->xobjs[i], &incoming) != FALSE)
        {
            /* EVERY LATER XOBJ APPLIES BEFORE THE ONES ALREADY COMPOSED.
             * gcPrepDObjMatrix (sys/objdisplay.c:1187-1189) emits one
             * gSPMatrix(G_MTX_MUL | G_MTX_MODELVIEW) per XObj in table order,
             * and F3DEX MUL forms new * top, so the last XObj is the first
             * transform a vertex meets. Custom kind 0x50 (a fighter joint's
             * WORLD translation) already composed this way; the Congo barrel,
             * the only source DObj with two XObjs ({ 0x28, RotRpyR },
             * grjungle.c:14), rotated around the stage origin because its
             * RotRpyR was multiplied AFTER the kind-40 placement (owner,
             * 2026-09-07). One-XObj DObjs never reach this arm. */
            ndsRendererAdapterMulBefore(out, &incoming, &valid);
        }
    }


    if (valid == FALSE)
    {
        Mtx mtx;

        if (has_mvp_recalc_rpy_0x47 != FALSE)
        {
            /* The source special callback can legally follow the current
             * parent/camera matrix without another local affine transform. */
            ndsRendererAdapterMtxIdentity20p12(out);
        }
        else
        {
            /* Measured cold: 0 of 101,569 builder calls in a 60-second both-CPU
             * match take this arm (101,528 xobj, 41 recalc identity). The board
             * has the run; do not re-instrument it. */
            ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
            ndsRendererAdapterMtxFromN64(&mtx, out);
        }
    }
    return TRUE;
}

/* ITCM (2026-09-27 census: ~0.45K ticks/frame non-memory stall at 72 B). */
static u32 NDS_ITCM_PACK2_CODE ndsRendererAdapterDirectMvpRecalcKind(DObj *dobj)
{
    u32 i;

    if (dobj == NULL)
    {
        return 0u;
    }
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        if ((dobj->xobjs[i] != NULL) &&
            (ndsRendererAdapterIsMvpRecalcKind(dobj->xobjs[i]->kind) !=
                FALSE))
        {
            return (u32)dobj->xobjs[i]->kind;
        }
    }
    return 0u;
}

/* gcPrepDObjMatrix accumulates scale.x down the parent chain, restoring it
 * between siblings. Recompute it from the live chain rather than relying on
 * side effects from a world-matrix cache hit. Stage kind-48 nodes are leaves. */
static sb32 ndsRendererAdapterMvpParentScaleX(DObj *dobj, f32 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj->parent;
    u32 count = 0u;
    f32 scale_x = 1.0F;

    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        if (count == NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX)
        {
            return FALSE;
        }
        chain[count++] = cursor;
        cursor = cursor->parent;
    }
    while (count != 0u)
    {
        u32 i;
        cursor = chain[--count];
        /* The layer packets currently use recalc only at drawable leaves.
         * A recalc ancestor also changes the descendant's translation path;
         * it cannot be reduced to an affine world plus this scale factor. */
        if (ndsRendererAdapterDirectMvpRecalcKind(cursor) != 0u)
        {
            return FALSE;
        }
        for (i = 0u; i < cursor->xobjs_num; i++)
        {
            u32 kind;
            if (cursor->xobjs[i] == NULL) { continue; }
            kind = cursor->xobjs[i]->kind;
            if ((kind >= 41u) && (kind <= 50u)) { return FALSE; }
            switch (kind)
            {
            case nGCMatrixKindTraRotRSca:
            case nGCMatrixKindTraRotRpyRSca:
            case nGCMatrixKindTraRotPyrRSca:
            case nGCMatrixKindSca:
                scale_x *= cursor->scale.vec.f.x;
                break;
            case nGCMatrixKindVecSca:
            case nGCMatrixKindVecTraRotRSca:
            case nGCMatrixKindVecTraRotRpyRSca:
            {
                GCTranslate *translate = NULL;
                GCRotate *rotate = NULL;
                GCScale *scale = NULL;
                ndsRendererAdapterGetDObjVectorTracks(cursor, &translate, &rotate, &scale);
                if (scale == NULL) { return FALSE; }
                scale_x *= scale->vec.f.x;
                break;
            }
            default:
                if (kind >= NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND)
                {
                    return FALSE;
                }
                break;
            }
        }
    }
    *out = scale_x;
    return TRUE;
}
