#include <sys/matrix.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#if NDS_RENDERER_HW_TRIANGLES
#include <nds/timers.h>
#endif

#define NDS_RENDERER_ADAPTER_MTX_FRAC_BITS 12
#define NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX 32u
#define NDS_RENDERER_ADAPTER_MATERIAL_MOBJ_MAX 64u
#define NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX 4u
#define NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT 4u
#define NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT 128u
#define NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT 256u
#define NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK \
    (NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT - 1u)
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT 64u
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE \
    (NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT - \
     NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT)
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT 128u
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK \
    (NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT - 1u)
#define NDS_RENDERER_ADAPTER_G_TX_LOADTILE 7u
#define NDS_RENDERER_ADAPTER_G_TX_RENDERTILE 0u
#define NDS_RENDERER_ADAPTER_G_TX_WRAP 0u
#define NDS_RENDERER_ADAPTER_G_TX_NOMASK 0u
#define NDS_RENDERER_ADAPTER_G_TX_NOLOD 0u
#define NDS_RENDERER_ADAPTER_G_ON 1u
#define NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL 0x0au
#define NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_1 0x00u
#define NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_1 0x04u
#define NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_2 0x18u
#define NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_2 0x1cu
#define NDS_RENDERER_ADAPTER_G_MWO_POINT_ST 0x14u
#define NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL 2047u
/* dLBCommonFuncMatrixList is consumed as syMtxProcess pairs; kind 0x4B maps
 * to lbCommonFighterPartsFuncMatrix in BattleShip. */
#define NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND 0x4Bu
/* dLBCommonFuncMatrixList kind 0x4C maps to gmCameraLookAtFuncMatrix. */
#define NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND 0x4Cu

static const Gfx sNdsRendererAdapterEmptySegmentEDL[1] = {
    { { NDS_FIGHTER_DL_OP_ENDDL << 24, 0u } }
};

#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL < 2
static NDSRendererNativeMaterial
    sNdsRendererAdapterNativeOwnerMaterials[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
        [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
static s32 sNdsRendererAdapterNativeOwnerTextureCurr[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
    [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
static s32 sNdsRendererAdapterNativeOwnerTextureNext[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED]
    [NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
static u32 sNdsRendererAdapterNativeOwnerTextureCounts[
    NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
static NDSRendererMatrix20p12
    sNdsRendererAdapterNativeOwnerProjection;
static NDSRendererMatrix20p12
    sNdsRendererAdapterNativeOwnerModelviews[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
#else
static NDSRendererNativeMaterial
    sNdsRendererAdapterNativeMaterials[
        NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
#endif

typedef struct NDSRendererAdapterNativeOwnerWorkspace
{
    NDSRelocLoadedFile *loaded[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
#if NDS_RENDERER_PROFILE_LEVEL < 2
    DObj *matrix_bindings[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    DObj *material_dobjs[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    const NDSRendererMatrix20p12 *modelviews[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererMatrix20p12 composed_matrices[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererConfig production_configs[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    NDSRendererNativeFighterRoot production_roots[
        NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
#endif
    u32 root_offsets[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 material_counts[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
} NDSRendererAdapterNativeOwnerWorkspace;

/* Fighter display callbacks are serialized by taskman. Keep their whole-owner
 * preflight tables off the already-tight nested task stack. */
static NDSRendererAdapterNativeOwnerWorkspace
    sNdsRendererAdapterNativeOwnerWorkspace;

#if NDS_RENDERER_PROFILE_LEVEL < 2
typedef struct NDSRendererAdapterNativeOwnerValidationCache
{
    const void *data;
    u32 asset_id;
    u32 owner_generation;
    u32 data_size;
    u32 root_count;
    u32 root_offsets[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 material_counts[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 valid;
} NDSRendererAdapterNativeOwnerValidationCache;

/* The generated tables and relocated owner payload are immutable for one
 * reloc generation.  Prove every selected root/material identity each draw,
 * but do the full generated-array/span walk only when that identity changes. */
static NDSRendererAdapterNativeOwnerValidationCache
    sNdsRendererAdapterNativeOwnerValidationCache[2];
#endif

typedef struct NDSRendererAdapterCameraCacheEntry
{
    const CObj *cobj;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid;
    u32 modelview_valid;
} NDSRendererAdapterCameraCacheEntry;

typedef struct NDSRendererAdapterDObjWorldCacheEntry
{
    const DObj *dobj;
    NDSRendererMatrix20p12 world;
} NDSRendererAdapterDObjWorldCacheEntry;

typedef struct NDSRendererAdapterStageWorldSourceKey
{
    u32 base_translate[3];
    u32 base_rotate[4];
    u32 base_scale[3];
    u8 xobj_kinds[5];
    u8 xobj_present_mask;
    u8 xobjs_num;
    u8 reserved;
} NDSRendererAdapterStageWorldSourceKey;

typedef struct NDSRendererAdapterStageWorldCacheEntry
{
    const DObj *dobj;
    const DObj *parent;
    NDSRendererAdapterStageWorldSourceKey source_key;
    u32 parent_generation;
    u32 generation;
    u32 validated_frame;
    u8 source_key_valid;
    u8 world_slot;
    u8 reserved[2];
} NDSRendererAdapterStageWorldCacheEntry;

static NDSRendererAdapterCameraCacheEntry
    sNdsRendererAdapterCameraCache[NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT];
static u32 sNdsRendererAdapterCameraCacheFrame;
static u32 sNdsRendererAdapterCameraCacheCount;
static NDSRendererAdapterDObjWorldCacheEntry
    *sNdsRendererAdapterDObjWorldCache;
static u32 sNdsRendererAdapterDObjWorldCacheFrame;
static u32 sNdsRendererAdapterDObjWorldCacheCount;
static u32 sNdsRendererAdapterDObjWorldCacheDynamicLimit =
    NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;
static u32 sNdsRendererAdapterDObjWorldCacheAllocationAttempted;
static u8 sNdsRendererAdapterDObjWorldIndex[
    NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT];
static NDSRendererAdapterStageWorldCacheEntry
    *sNdsRendererAdapterStageWorldCache;
static u32 sNdsRendererAdapterStageWorldCacheCount;
static u32 sNdsRendererAdapterStageWorldCacheAllocationAttempted;
static u32 sNdsRendererAdapterStageWorldNextGeneration;
static u8 sNdsRendererAdapterStageWorldIndex[
    NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT];
#endif

static sb32 ndsRendererAdapterRangeIsEmptySegmentEDL(const Gfx *dl,
                                                     size_t bytes)
{
    uintptr_t base = (uintptr_t)sNdsRendererAdapterEmptySegmentEDL;
    uintptr_t addr = (uintptr_t)dl;
    size_t size = sizeof(sNdsRendererAdapterEmptySegmentEDL);

    if ((dl == NULL) || (addr < base) || (addr > (base + size)))
    {
        return FALSE;
    }
    return (bytes <= (size - (size_t)(addr - base))) ? TRUE : FALSE;
}

static void ndsRendererAdapterMtxIdentity20p12(
    NDSRendererMatrix20p12 *out)
{
    u32 i;

    if (out == NULL)
    {
        return;
    }

    memset(out, 0, sizeof(*out));
    for (i = 0; i < 4u; i++)
    {
        out->m[i][i] = 1 << NDS_RENDERER_ADAPTER_MTX_FRAC_BITS;
    }
}

static void ndsRendererAdapterMulInto(NDSRendererMatrix20p12 *target,
                                      const NDSRendererMatrix20p12 *incoming,
                                      u32 *valid)
{
    if ((target == NULL) || (incoming == NULL) || (valid == NULL))
    {
        return;
    }

    if (*valid != 0u)
    {
        ndsRendererMtxMul20p12(target, incoming, target);
    }
    else
    {
        *target = *incoming;
        *valid = TRUE;
    }
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
        *target = *incoming;
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

static s32 ndsRendererAdapterFloatTo20p12(f32 value)
{
    f32 scaled = value * (f32)(1 << NDS_RENDERER_ADAPTER_MTX_FRAC_BITS);

    if (scaled >= 2147483520.0F)
    {
        return 0x7fffffff;
    }
    if (scaled <= -2147483520.0F)
    {
        return (s32)0x80000000u;
    }
    return (s32)((scaled >= 0.0F) ? (scaled + 0.5F) : (scaled - 0.5F));
}

static void ndsRendererAdapterMtxFromF(
    Mtx44f mtx, NDSRendererMatrix20p12 *dst)
{
    u32 row;
    u32 col;

    if (dst == NULL)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            f32 value = mtx[row][col];

            if (col == 3u)
            {
                value = (row == 3u) ? 1.0F : 0.0F;
            }
            dst->m[row][col] = ndsRendererAdapterFloatTo20p12(value);
        }
    }
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
        ndsRendererAdapterMtxFromF(parts->unk_dobjtrans_0x10, out);
        return TRUE;
    }

    if ((dobj->scale.vec.f.x != 1.0F) ||
        (dobj->scale.vec.f.y != 1.0F) ||
        (dobj->scale.vec.f.z != 1.0F))
    {
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
        syMatrixTraRotRpyR(&mtx,
                           dobj->translate.vec.f.x,
                           dobj->translate.vec.f.y,
                           dobj->translate.vec.f.z,
                           dobj->rotate.vec.f.x,
                           dobj->rotate.vec.f.y,
                           dobj->rotate.vec.f.z);
    }
    ndsRendererAdapterMtxFromN64(&mtx, out);
    return TRUE;
}

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

static sb32 ndsRendererAdapterBuildDObjXObjMatrix(
    DObj *dobj, XObj *xobj, NDSRendererMatrix20p12 *out)
{
    Mtx mtx;
    GCTranslate *translate;
    GCRotate *rotate;
    GCScale *scale;

    if ((dobj == NULL) || (xobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    ndsRendererAdapterGetDObjVectorTracks(dobj, &translate, &rotate, &scale);

    switch (xobj->kind)
    {
    case 1:
        mtx = xobj->mtx;
        break;
    case 2:
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

static sb32 ndsRendererAdapterBuildDObjLocalMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 incoming;
    u32 valid = FALSE;
    u32 i;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }

    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        if ((dobj->xobjs[i] != NULL) &&
            (ndsRendererAdapterBuildDObjXObjMatrix(
                 dobj, dobj->xobjs[i], &incoming) != FALSE))
        {
            ndsRendererAdapterMulInto(out, &incoming, &valid);
        }
    }

    if (valid == FALSE)
    {
        Mtx mtx;

        ndsRendererAdapterBuildDObjFallbackMtx(dobj, &mtx);
        ndsRendererAdapterMtxFromN64(&mtx, out);
    }
    return TRUE;
}

#if NDS_RENDERER_HW_TRIANGLES
static void ndsRendererAdapterResetSceneCaches(void)
{
    ndsRendererHardwareResetSourceCaches();
    sNdsRendererAdapterCameraCacheFrame = 0u;
    sNdsRendererAdapterCameraCacheCount = 0u;
    sNdsRendererAdapterDObjWorldCache = NULL;
    sNdsRendererAdapterDObjWorldCacheFrame = 0u;
    sNdsRendererAdapterDObjWorldCacheCount = 0u;
    sNdsRendererAdapterDObjWorldCacheDynamicLimit =
        NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;
    sNdsRendererAdapterDObjWorldCacheAllocationAttempted = FALSE;
    memset(sNdsRendererAdapterDObjWorldIndex, 0,
           sizeof(sNdsRendererAdapterDObjWorldIndex));
    sNdsRendererAdapterStageWorldCache = NULL;
    sNdsRendererAdapterStageWorldCacheCount = 0u;
    sNdsRendererAdapterStageWorldCacheAllocationAttempted = FALSE;
    sNdsRendererAdapterStageWorldNextGeneration = 1u;
    memset(sNdsRendererAdapterStageWorldIndex, 0,
           sizeof(sNdsRendererAdapterStageWorldIndex));
}

static u32 ndsRendererAdapterDObjWorldIndexHash(const DObj *dobj)
{
    uintptr_t key = (uintptr_t)dobj >> 2;

    key ^= key >> 8;
    key ^= key >> 16;
    return (u32)key & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
}

static sb32 ndsRendererAdapterEnsureDObjWorldCache(void)
{
    uintptr_t aligned;
    size_t bytes = sizeof(NDSRendererAdapterDObjWorldCacheEntry) *
        NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;

    if (sNdsRendererAdapterDObjWorldCache != NULL)
    {
        return TRUE;
    }
    if (sNdsRendererAdapterDObjWorldCacheAllocationAttempted != FALSE)
    {
        return FALSE;
    }
    sNdsRendererAdapterDObjWorldCacheAllocationAttempted = TRUE;
    if ((gSYTaskmanGeneralHeap.ptr == NULL) ||
        (gSYTaskmanGeneralHeap.end == NULL))
    {
        return FALSE;
    }
    aligned = ((uintptr_t)gSYTaskmanGeneralHeap.ptr + 0x0fu) &
        ~(uintptr_t)0x0fu;
    if ((aligned > (uintptr_t)gSYTaskmanGeneralHeap.end) ||
        (bytes > ((uintptr_t)gSYTaskmanGeneralHeap.end - aligned)))
    {
        return FALSE;
    }
    sNdsRendererAdapterDObjWorldCache =
        (NDSRendererAdapterDObjWorldCacheEntry *)syTaskmanMalloc(bytes, 0x10u);
    /* This DS cache now consumes the original scene heap, so keep the P1
     * reserve ledger truthful after its lazy first-frame allocation. */
    ndsRelocUpdateMemoryLedger();
    return (sNdsRendererAdapterDObjWorldCache != NULL) ? TRUE : FALSE;
}

static const NDSRendererMatrix20p12 *
ndsRendererAdapterFindDObjWorldMatrix(const DObj *dobj)
{
    u32 frame = gNdsRendererProfileFrameCount;
    u32 slot;
    u32 probe;

    if (sNdsRendererAdapterDObjWorldCacheFrame != frame)
    {
        sNdsRendererAdapterDObjWorldCacheFrame = frame;
        sNdsRendererAdapterDObjWorldCacheCount = 0u;
        memset(sNdsRendererAdapterDObjWorldIndex, 0,
               sizeof(sNdsRendererAdapterDObjWorldIndex));
    }
    if (ndsRendererAdapterEnsureDObjWorldCache() == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileDObjWorldCacheMissCount++;
#endif
        return NULL;
    }
    slot = ndsRendererAdapterDObjWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT;
         probe++)
    {
        u32 encoded = sNdsRendererAdapterDObjWorldIndex[slot];
        u32 cache_index;

        if (encoded == 0u)
        {
            break;
        }
        cache_index = encoded - 1u;
        if ((cache_index < sNdsRendererAdapterDObjWorldCacheCount) &&
            (sNdsRendererAdapterDObjWorldCache[cache_index].dobj == dobj))
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileDObjWorldCacheHitCount++;
#endif
            return &sNdsRendererAdapterDObjWorldCache[cache_index].world;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileDObjWorldCacheMissCount++;
#endif
    return NULL;
}

static void ndsRendererAdapterStoreDObjWorldMatrix(
    const DObj *dobj, const NDSRendererMatrix20p12 *world)
{
    NDSRendererAdapterDObjWorldCacheEntry *entry;
    u32 cache_index;
    u32 slot;
    u32 probe;

    if ((dobj == NULL) || (world == NULL))
    {
        return;
    }
    if ((sNdsRendererAdapterDObjWorldCache == NULL) ||
        (sNdsRendererAdapterDObjWorldCacheCount >=
         sNdsRendererAdapterDObjWorldCacheDynamicLimit))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileDObjWorldCacheOverflowCount++;
#endif
        return;
    }
    cache_index = sNdsRendererAdapterDObjWorldCacheCount++;
    entry = &sNdsRendererAdapterDObjWorldCache[cache_index];
    entry->dobj = dobj;
    entry->world = *world;
    slot = ndsRendererAdapterDObjWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT;
         probe++)
    {
        if (sNdsRendererAdapterDObjWorldIndex[slot] == 0u)
        {
            sNdsRendererAdapterDObjWorldIndex[slot] = (u8)(cache_index + 1u);
            return;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
    }
}

static u32 ndsRendererAdapterStageWorldIndexHash(const DObj *dobj)
{
    uintptr_t key = (uintptr_t)dobj >> 2;

    key ^= key >> 7;
    key ^= key >> 15;
    return (u32)key & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
}

static sb32 ndsRendererAdapterEnsureStageWorldCache(void)
{
    uintptr_t aligned;
    size_t bytes = sizeof(NDSRendererAdapterStageWorldCacheEntry) *
        NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT;

    if (sNdsRendererAdapterStageWorldCache != NULL)
    {
        return TRUE;
    }
    if (sNdsRendererAdapterStageWorldCacheAllocationAttempted != FALSE)
    {
        return FALSE;
    }
    if ((ndsRendererAdapterEnsureDObjWorldCache() == FALSE) ||
        (sNdsRendererAdapterDObjWorldCacheCount >
         NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE))
    {
        return FALSE;
    }
    sNdsRendererAdapterStageWorldCacheAllocationAttempted = TRUE;
    if ((gSYTaskmanGeneralHeap.ptr == NULL) ||
        (gSYTaskmanGeneralHeap.end == NULL))
    {
        return FALSE;
    }
    aligned = ((uintptr_t)gSYTaskmanGeneralHeap.ptr + 0x0fu) &
        ~(uintptr_t)0x0fu;
    if ((aligned > (uintptr_t)gSYTaskmanGeneralHeap.end) ||
        (bytes > ((uintptr_t)gSYTaskmanGeneralHeap.end - aligned)))
    {
        return FALSE;
    }
    sNdsRendererAdapterStageWorldCache =
        (NDSRendererAdapterStageWorldCacheEntry *)
            syTaskmanMalloc(bytes, 0x10u);
    if (sNdsRendererAdapterStageWorldCache != NULL)
    {
        memset(sNdsRendererAdapterStageWorldCache, 0, bytes);
        sNdsRendererAdapterDObjWorldCacheDynamicLimit =
            NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE;
        ndsRelocUpdateMemoryLedger();
        return TRUE;
    }
    return FALSE;
}

static NDSRendererAdapterStageWorldCacheEntry *
ndsRendererAdapterFindStageWorldEntry(const DObj *dobj)
{
    u32 slot;
    u32 probe;

    if ((dobj == NULL) ||
        (ndsRendererAdapterEnsureStageWorldCache() == FALSE))
    {
        return NULL;
    }
    slot = ndsRendererAdapterStageWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT;
         probe++)
    {
        u32 encoded = sNdsRendererAdapterStageWorldIndex[slot];
        u32 cache_index;

        if (encoded == 0u)
        {
            return NULL;
        }
        cache_index = encoded - 1u;
        if ((cache_index < sNdsRendererAdapterStageWorldCacheCount) &&
            (sNdsRendererAdapterStageWorldCache[cache_index].dobj == dobj))
        {
            return &sNdsRendererAdapterStageWorldCache[cache_index];
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
    }
    return NULL;
}

static NDSRendererAdapterStageWorldCacheEntry *
ndsRendererAdapterFindOrAllocateStageWorldEntry(const DObj *dobj)
{
    NDSRendererAdapterStageWorldCacheEntry *entry;
    u32 cache_index;
    u32 slot;
    u32 probe;

    entry = ndsRendererAdapterFindStageWorldEntry(dobj);
    if (entry != NULL)
    {
        return entry;
    }
    if ((sNdsRendererAdapterStageWorldCache == NULL) ||
        (sNdsRendererAdapterStageWorldCacheCount >=
         NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT))
    {
        return NULL;
    }
    cache_index = sNdsRendererAdapterStageWorldCacheCount++;
    entry = &sNdsRendererAdapterStageWorldCache[cache_index];
    memset(entry, 0, sizeof(*entry));
    entry->dobj = dobj;
    /* A zero frame is valid in accelerated harnesses. Ensure a freshly
     * allocated entry cannot take the same-frame shortcut before its first
     * exact world build; no u32 equals its own bitwise complement. */
    entry->validated_frame = ~gNdsRendererProfileFrameCount;
    entry->world_slot = (u8)(NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE +
                             cache_index);
    sNdsRendererAdapterDObjWorldCache[entry->world_slot].dobj = dobj;
    slot = ndsRendererAdapterStageWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT;
         probe++)
    {
        if (sNdsRendererAdapterStageWorldIndex[slot] == 0u)
        {
            sNdsRendererAdapterStageWorldIndex[slot] =
                (u8)(cache_index + 1u);
            return entry;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
    }
    return NULL;
}

static NDSRendererMatrix20p12 *
ndsRendererAdapterStageWorldEntryMatrix(
    const NDSRendererAdapterStageWorldCacheEntry *entry)
{
    return &sNdsRendererAdapterDObjWorldCache[entry->world_slot].world;
}

static sb32 ndsRendererAdapterCaptureStageWorldSourceKey(
    DObj *dobj, NDSRendererAdapterStageWorldSourceKey *source_key)
{
    u32 i;

    if ((dobj == NULL) || (source_key == NULL) ||
        (dobj->xobjs_num > 5u) || (dobj->vec != NULL))
    {
        return FALSE;
    }
    memset(source_key, 0, sizeof(*source_key));
    memcpy(source_key->base_translate, &dobj->translate.vec.f,
           sizeof(source_key->base_translate));
    memcpy(&source_key->base_rotate[0], &dobj->rotate.a,
           sizeof(source_key->base_rotate[0]));
    memcpy(&source_key->base_rotate[1], &dobj->rotate.vec.f,
           sizeof(source_key->base_rotate) -
               sizeof(source_key->base_rotate[0]));
    memcpy(source_key->base_scale, &dobj->scale.vec.f,
           sizeof(source_key->base_scale));

    source_key->xobjs_num = dobj->xobjs_num;
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        XObj *xobj = dobj->xobjs[i];

        if (xobj == NULL)
        {
            continue;
        }
        source_key->xobj_present_mask |= (u8)(1u << i);
        source_key->xobj_kinds[i] = xobj->kind;
        /* Kind 1 consumes an arbitrary 64-byte matrix. Kinds 33-40 consume
         * live camera state, and 0x4B consumes live FTParts state. Keep those
         * exact paths frame-local instead of enlarging this first stage-only
         * proof cache. */
        if ((xobj->kind == 1u) ||
            ((xobj->kind >= 33u) && (xobj->kind <= 40u)) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static sb32 ndsRendererAdapterStageWorldSourceKeyMatches(
    DObj *dobj, const NDSRendererAdapterStageWorldSourceKey *source_key)
{
    u32 i;

    if ((dobj == NULL) || (source_key == NULL) ||
        (dobj->xobjs_num > 5u) || (dobj->vec != NULL) ||
        (dobj->xobjs_num != source_key->xobjs_num) ||
        (memcmp(source_key->base_translate, &dobj->translate.vec.f,
                sizeof(source_key->base_translate)) != 0) ||
        (memcmp(&source_key->base_rotate[0], &dobj->rotate.a,
                sizeof(source_key->base_rotate[0])) != 0) ||
        (memcmp(&source_key->base_rotate[1], &dobj->rotate.vec.f,
                sizeof(source_key->base_rotate) -
                    sizeof(source_key->base_rotate[0])) != 0) ||
        (memcmp(source_key->base_scale, &dobj->scale.vec.f,
                sizeof(source_key->base_scale)) != 0))
    {
        return FALSE;
    }
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        XObj *xobj = dobj->xobjs[i];
        u32 expected_present =
            (source_key->xobj_present_mask & (u8)(1u << i));

        if (xobj == NULL)
        {
            if (expected_present != 0u)
            {
                return FALSE;
            }
            continue;
        }
        if ((expected_present == 0u) ||
            (source_key->xobj_kinds[i] != xobj->kind) ||
            (xobj->kind == 1u) ||
            ((xobj->kind >= 33u) && (xobj->kind <= 40u)) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsRendererAdapterNextStageWorldGeneration(void)
{
    u32 generation = sNdsRendererAdapterStageWorldNextGeneration++;

    if (generation == 0u)
    {
        generation = sNdsRendererAdapterStageWorldNextGeneration++;
    }
    return generation;
}
#endif

#if NDS_RENDERER_HW_TRIANGLES
static sb32 ndsRendererAdapterBuildDObjWorldMatrixUncached(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 local;
    u32 depth = 0u;
    u32 i;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterMtxIdentity20p12(out);
    for (i = depth; i != 0u; i--)
    {
        if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u], &local) !=
            FALSE)
        {
            ndsRendererMtxMulAffine20p12(&local, out, out);
        }
    }
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterBuildDObjWorldMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 local;
    u32 depth = 0u;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    const NDSRendererMatrix20p12 *cached;
#endif

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
#if NDS_RENDERER_HW_TRIANGLES
    cached = ndsRendererAdapterFindDObjWorldMatrix(dobj);
    if (cached != NULL)
    {
        *out = *cached;
        return TRUE;
    }
#endif

    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }

#if NDS_RENDERER_HW_TRIANGLES
    for (i = 1u; i < depth; i++)
    {
        cached = ndsRendererAdapterFindDObjWorldMatrix(chain[i]);
        if (cached != NULL)
        {
            *out = *cached;
            break;
        }
    }
    if (i == depth)
    {
        ndsRendererAdapterMtxIdentity20p12(out);
    }
#else
    ndsRendererAdapterMtxIdentity20p12(out);
    i = depth;
#endif
    for (; i != 0u; i--)
    {
        if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u], &local) !=
            FALSE)
        {
            /* objdisplay.c:1183-1191 left-multiplies each child local matrix. */
            ndsRendererMtxMulAffine20p12(&local, out, out);
#if NDS_RENDERER_HW_TRIANGLES
            /* DObj transforms are finalized before the camera's gcDrawAll
             * pass. Cache each prefix for sibling/child draws, and reset at
             * the next presented frame so live fighter poses remain live. */
            ndsRendererAdapterStoreDObjWorldMatrix(chain[i - 1u], out);
#endif
        }
    }
    return TRUE;
}

#if NDS_RENDERER_HW_TRIANGLES
static sb32 ndsRendererAdapterBuildPersistentStageWorldMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 parent_world;
    NDSRendererMatrix20p12 local;
    NDSRendererAdapterStageWorldCacheEntry *entry;
    NDSRendererAdapterStageWorldSourceKey source_key;
    u32 parent_generation = 0u;
    u32 depth = 0u;
    u32 frame = gNdsRendererProfileFrameCount;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 reused_persistent = FALSE;
#endif
    u32 i;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    entry = ndsRendererAdapterFindStageWorldEntry(dobj);
    if ((entry != NULL) && (entry->validated_frame == frame))
    {
        *out = *ndsRendererAdapterStageWorldEntryMatrix(entry);
        return TRUE;
    }
    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }
    if (ndsRendererAdapterEnsureStageWorldCache() == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileStageWorldPersistentOverflowCount++;
#endif
        return ndsRendererAdapterBuildDObjWorldMatrixUncached(dobj, out);
    }

    ndsRendererAdapterMtxIdentity20p12(&parent_world);
    for (i = depth; i != 0u; i--)
    {
        DObj *node = chain[i - 1u];
        u32 source_key_valid;
        u32 reuse = FALSE;

        entry = ndsRendererAdapterFindOrAllocateStageWorldEntry(node);
        if (entry == NULL)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileStageWorldPersistentOverflowCount++;
#endif
            return ndsRendererAdapterBuildDObjWorldMatrixUncached(dobj, out);
        }
        if (entry->validated_frame == frame)
        {
            parent_world = *ndsRendererAdapterStageWorldEntryMatrix(entry);
            parent_generation = entry->generation;
            continue;
        }

        source_key_valid = FALSE;
        if ((entry->source_key_valid != FALSE) &&
            (entry->parent == node->parent) &&
            (entry->parent_generation == parent_generation) &&
            (ndsRendererAdapterStageWorldSourceKeyMatches(
                 node, &entry->source_key) != FALSE))
        {
            reuse = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            reused_persistent = TRUE;
            gNdsRendererProfileStageWorldPersistentHitCount++;
#endif
        }
        if (reuse == FALSE)
        {
            source_key_valid = ndsRendererAdapterCaptureStageWorldSourceKey(
                node, &source_key);
            if (ndsRendererAdapterBuildDObjLocalMatrix(node, &local) == FALSE)
            {
                return FALSE;
            }
            ndsRendererMtxMulAffine20p12(
                &local, &parent_world,
                ndsRendererAdapterStageWorldEntryMatrix(entry));
            entry->parent = node->parent;
            entry->parent_generation = parent_generation;
            entry->generation = ndsRendererAdapterNextStageWorldGeneration();
            entry->source_key_valid = source_key_valid;
            if (source_key_valid != FALSE)
            {
                entry->source_key = source_key;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                gNdsRendererProfileStageWorldPersistentMissCount++;
#endif
            }
            else
            {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                gNdsRendererProfileStageWorldPersistentRejectCount++;
#endif
            }
        }
        entry->validated_frame = frame;
        parent_world = *ndsRendererAdapterStageWorldEntryMatrix(entry);
        parent_generation = entry->generation;
    }
    *out = parent_world;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (reused_persistent != FALSE)
    {
        NDSRendererMatrix20p12 oracle_world;

        if (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                dobj, &oracle_world) != FALSE)
        {
            gNdsRendererProfileStageWorldPersistentOracleSampleCount++;
            if (memcmp(out, &oracle_world, sizeof(oracle_world)) != 0)
            {
                NDSRendererAdapterStageWorldCacheEntry *target =
                    ndsRendererAdapterFindStageWorldEntry(dobj);

                gNdsRendererProfileStageWorldPersistentOracleMismatchCount++;
                *out = oracle_world;
                if (target != NULL)
                {
                    target->source_key_valid = FALSE;
                    target->validated_frame = 0u;
                }
            }
        }
    }
#endif
    return TRUE;
}
#endif

static sb32 ndsRendererAdapterBuildCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid)
{
    NDSRendererMatrix20p12 incoming;
    XObj *xobj;
    Mtx mtx;
    u32 i;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return FALSE;
    }

    *projection_valid = FALSE;
    *modelview_valid = FALSE;

    if (cobj == NULL)
    {
        return FALSE;
    }
    if (cobj->xobjs_num <= 0)
    {
        return FALSE;
    }

    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        xobj = cobj->xobjs[i];
        if (xobj == NULL)
        {
            continue;
        }

        switch (xobj->kind)
        {
        case NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND:
        {
            LookAt look_at;
            Mtx mtx;
            NDSRendererMatrix20p12 lookat;
            NDSRendererMatrix20p12 persp;

            syMatrixLookAtReflect(&mtx, &look_at,
                                  cobj->vec.eye.x, cobj->vec.eye.y,
                                  cobj->vec.eye.z, cobj->vec.at.x,
                                  cobj->vec.at.y, cobj->vec.at.z,
                                  cobj->vec.up.x, cobj->vec.up.y,
                                  cobj->vec.up.z);
            ndsRendererAdapterMtxFromN64(&mtx, &lookat);
            syMatrixPerspFast(&mtx, &cobj->projection.persp.norm,
                              cobj->projection.persp.fovy,
                              cobj->projection.persp.aspect,
                              cobj->projection.persp.near,
                              cobj->projection.persp.far,
                              cobj->projection.persp.scale);
            ndsRendererAdapterMtxFromN64(&mtx, &persp);
            ndsRendererMtxMul20p12(&lookat, &persp, projection);
            *projection_valid = TRUE;
            break;
        }
        case nGCMatrixKindPerspFastF:
            syMatrixPerspFast(&mtx, &cobj->projection.persp.norm,
                              cobj->projection.persp.fovy,
                              cobj->projection.persp.aspect,
                              cobj->projection.persp.near,
                              cobj->projection.persp.far,
                              cobj->projection.persp.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case nGCMatrixKindPerspF:
            syMatrixPersp(&mtx, &cobj->projection.persp.norm,
                          cobj->projection.persp.fovy,
                          cobj->projection.persp.aspect,
                          cobj->projection.persp.near,
                          cobj->projection.persp.far,
                          cobj->projection.persp.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case nGCMatrixKindOrtho:
            syMatrixOrtho(&mtx,
                          cobj->projection.ortho.l,
                          cobj->projection.ortho.r,
                          cobj->projection.ortho.b,
                          cobj->projection.ortho.t,
                          cobj->projection.ortho.n,
                          cobj->projection.ortho.f,
                          cobj->projection.ortho.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case 6:
        case 7:
            syMatrixLookAt(&mtx,
                           cobj->vec.eye.x, cobj->vec.eye.y,
                           cobj->vec.eye.z, cobj->vec.at.x,
                           cobj->vec.at.y, cobj->vec.at.z,
                           cobj->vec.up.x, cobj->vec.up.y,
                           cobj->vec.up.z);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 6)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        case 8:
        case 9:
            syMatrixModLookAt(&mtx,
                              cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 1.0F, 0.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 8)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        case 10:
        case 11:
            syMatrixModLookAt(&mtx,
                              cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 0.0F, 1.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 10)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        default:
            break;
        }
    }

    return ((*projection_valid != 0u) || (*modelview_valid != 0u)) ?
        TRUE : FALSE;
}

#if NDS_RENDERER_HW_TRIANGLES
static void ndsRendererAdapterBuildDefaultBattleCameraMatrices(
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid)
{
    u16 norm = 0u;
    Mtx mtx;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return;
    }

    syMatrixPerspFast(&mtx, &norm, 38.0F, 15.0F / 11.0F,
                      256.0F, 39936.0F, 1.0F);
    ndsRendererAdapterMtxFromN64(&mtx, projection);
    *projection_valid = TRUE;

    syMatrixLookAt(&mtx,
                   0.0F, 300.0F, 1600.0F,
                   0.0F, 300.0F, 0.0F,
                   0.0F, 1.0F, 0.0F);
    ndsRendererAdapterMtxFromN64(&mtx, modelview);
    *modelview_valid = TRUE;
}

static void ndsRendererAdapterGetFrameCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid)
{
    u32 frame = gNdsRendererProfileFrameCount;
    u32 i;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return;
    }
    if (sNdsRendererAdapterCameraCacheFrame != frame)
    {
        sNdsRendererAdapterCameraCacheFrame = frame;
        sNdsRendererAdapterCameraCacheCount = 0u;
    }
    for (i = 0u; i < sNdsRendererAdapterCameraCacheCount; i++)
    {
        const NDSRendererAdapterCameraCacheEntry *entry =
            &sNdsRendererAdapterCameraCache[i];

        if (entry->cobj == cobj)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileCameraMatrixCacheHitCount++;
#endif
            *projection = entry->projection;
            *modelview = entry->modelview;
            *projection_valid = entry->projection_valid;
            *modelview_valid = entry->modelview_valid;
            return;
        }
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileCameraMatrixCacheMissCount++;
#endif
    ndsRendererAdapterBuildCameraMatrices(
        cobj, projection, projection_valid, modelview, modelview_valid);
    if ((*projection_valid == FALSE) && (*modelview_valid == FALSE))
    {
        ndsRendererAdapterBuildDefaultBattleCameraMatrices(
            projection, projection_valid, modelview, modelview_valid);
    }
    if (sNdsRendererAdapterCameraCacheCount <
        NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT)
    {
        NDSRendererAdapterCameraCacheEntry *entry =
            &sNdsRendererAdapterCameraCache[
                sNdsRendererAdapterCameraCacheCount++];

        /* BattleShip updates CObj state before gcDrawAll; every DObj in that
         * camera's draw pass consumes the same matrices. The frame token
         * prevents fighter poses or camera motion from crossing presents. */
        entry->cobj = cobj;
        entry->projection = *projection;
        entry->modelview = *modelview;
        entry->projection_valid = *projection_valid;
        entry->modelview_valid = *modelview_valid;
    }
    else
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileCameraMatrixCacheOverflowCount++;
#endif
    }
}
#endif

static void ndsRendererAdapterPrepareInitialMatrices(
    DObj *dobj,
    CObj *cobj,
    u32 persistent_stage_world,
    NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 **projection_ptr,
    NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 **modelview_ptr)
{
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    NDSRendererMatrix20p12 dobj_world;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 dobj_world_valid = FALSE;

    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        return;
    }
    *projection_ptr = NULL;
    *modelview_ptr = NULL;

    if ((projection == NULL) || (modelview == NULL))
    {
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid);
#else
    ndsRendererAdapterBuildCameraMatrices(cobj, &camera_projection,
                                          &camera_projection_valid,
                                          &camera_modelview,
                                          &camera_modelview_valid);
#endif
    if (dobj != NULL)
    {
#if NDS_RENDERER_HW_TRIANGLES
        if (persistent_stage_world != FALSE)
        {
            dobj_world_valid =
                ndsRendererAdapterBuildPersistentStageWorldMatrix(
                    dobj, &dobj_world);
        }
        else
#endif
        {
            dobj_world_valid =
                ndsRendererAdapterBuildDObjWorldMatrix(dobj, &dobj_world);
        }
    }

    if (camera_projection_valid != FALSE)
    {
        *projection = camera_projection;
        *projection_ptr = projection;
    }

    if ((camera_modelview_valid != FALSE) && (dobj_world_valid != FALSE))
    {
        ndsRendererMtxMul20p12(&dobj_world, &camera_modelview, modelview);
        *modelview_ptr = modelview;
    }
    else if (camera_modelview_valid != FALSE)
    {
        *modelview = camera_modelview;
        *modelview_ptr = modelview;
    }
    else if (dobj_world_valid != FALSE)
    {
        *modelview = dobj_world;
        *modelview_ptr = modelview;
    }
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterComposeNativeRootMatrix(
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *out)
{
    if (out == NULL)
    {
        return FALSE;
    }
    if ((modelview != NULL) && (projection != NULL))
    {
        ndsRendererMtxMul20p12(modelview, projection, out);
        return TRUE;
    }
    if (modelview != NULL)
    {
        *out = *modelview;
        return TRUE;
    }
    if (projection != NULL)
    {
        *out = *projection;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsRendererAdapterPrepareNativeOwnerMatrices(
    DObj *root,
    DObj *const *bindings,
    u32 binding_count,
    CObj *cobj,
    const NDSRendererMatrix20p12 **projection_ptr,
    const NDSRendererMatrix20p12 **modelview_ptrs)
{
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    NDSRendererMatrix20p12 world;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 binding_index;

    if ((bindings == NULL) || (projection_ptr == NULL) ||
        (modelview_ptrs == NULL) ||
        (binding_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    *projection_ptr = NULL;
    for (binding_index = 0u;
         binding_index < binding_count;
         binding_index++)
    {
        modelview_ptrs[binding_index] = NULL;
    }

    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid);
    if (camera_projection_valid != FALSE)
    {
        sNdsRendererAdapterNativeOwnerProjection = camera_projection;
        *projection_ptr = &sNdsRendererAdapterNativeOwnerProjection;
    }

    (void)root;
    /* The ordinary per-frame DObj world cache already records every prefix
     * built for a binding.  Walking each selected binding through that cache
     * visits the fighter hierarchy once without the old per-node linear scan
     * over all 14/18 selected roots. */
    for (binding_index = 0u;
         binding_index < binding_count;
         binding_index++)
    {
        if (bindings[binding_index] != NULL)
        {
            if (ndsRendererAdapterBuildDObjWorldMatrix(
                    bindings[binding_index], &world) == FALSE)
            {
                return FALSE;
            }
            if (camera_modelview_valid != FALSE)
            {
                ndsRendererMtxMulAffine20p12(
                    &world, &camera_modelview,
                    &sNdsRendererAdapterNativeOwnerModelviews[
                        binding_index]);
            }
            else
            {
                sNdsRendererAdapterNativeOwnerModelviews[binding_index] =
                    world;
            }
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
        else if (camera_modelview_valid != FALSE)
        {
            sNdsRendererAdapterNativeOwnerModelviews[binding_index] =
                camera_modelview;
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
    }
    return ((*projection_ptr != NULL) ||
            (camera_modelview_valid != FALSE) ||
            (binding_count == 0u)) ? TRUE : FALSE;
}
static u32 ndsRendererAdapterNormalizeNativeGeometryMode(u32 geometry_mode)
{
    const u32 legacy_cull_front = 0x00001000u;
    const u32 legacy_cull_back = G_CULL_BACK;
    u32 legacy = geometry_mode &
        (legacy_cull_front | legacy_cull_back);

    if (legacy != 0u)
    {
        geometry_mode &= ~(legacy_cull_front | legacy_cull_back);
        if ((legacy & legacy_cull_front) != 0u)
        {
            geometry_mode |= NDS_RENDERER_GEOM_CULL_FRONT;
        }
        if ((legacy & legacy_cull_back) != 0u)
        {
            geometry_mode |= NDS_RENDERER_GEOM_CULL_BACK;
        }
    }
    return geometry_mode;
}
#endif

static s32 ndsFighterDLScanRangeInTaskmanArena(const void *ptr, size_t bytes)
{
    const u8 *arena = ndsTaskmanArenaStart();
    uintptr_t base = (uintptr_t)arena;
    uintptr_t addr = (uintptr_t)ptr;
    size_t arena_size = ndsTaskmanArenaSize();

    if ((ptr == NULL) || (arena == NULL) || (addr < base) ||
        (addr > (base + arena_size)))
    {
        return FALSE;
    }
    return (bytes <= (arena_size - (size_t)(addr - base))) ? TRUE : FALSE;
}

static size_t ndsRendererAdapterImmutableCommandSpan(const Gfx *dl,
                                                      void *user)
{
    NDSRelocLoadedFile *loaded;
    uintptr_t base;
    uintptr_t addr;

    (void)user;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if (loaded == NULL)
    {
        return 0u;
    }
    base = (uintptr_t)loaded->data;
    addr = (uintptr_t)dl;
    return loaded->data_size - (size_t)(addr - base);
}

static s32 ndsFighterDLScanValidateRange(const Gfx *dl, size_t bytes,
                                         void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLScanRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static const Gfx *ndsFighterDLScanResolveBranch(const Gfx *dl,
                                                 u32 *resolve_kind,
                                                 void *user)
{
    NDSFighterDLScanContext *context = user;
    uintptr_t raw = (uintptr_t)dl;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if (resolve_kind != NULL)
    {
        *resolve_kind = NDS_RENDERER_RESOLVE_NONE;
    }

    if (ndsRelocFindLoadedFileContaining(dl, sizeof(Gfx)) != NULL)
    {
        return dl;
    }
    if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(Gfx)) != FALSE)
    {
        return dl;
    }

    if ((context != NULL) &&
        (context->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(context->primary_file,
                                   offset,
                                   sizeof(Gfx)) != FALSE))
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        gNdsFighterDLScanBranchResolveCount++;
        return (const Gfx *)((const u8 *)context->primary_file->data + offset);
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      sizeof(Gfx)) != FALSE)
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            gNdsFighterDLScanBranchResolveCount++;
            return (const Gfx *)((const u8 *)sNdsRelocLoadedFiles[i].data +
                                 offset);
        }
    }

    return dl;
}

static const void *ndsFighterDLScanResolveDataPointer(const void *ptr,
                                                      size_t bytes,
                                                      void *user)
{
    NDSFighterDLScanContext *context = user;
    uintptr_t raw = (uintptr_t)ptr;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if ((ndsRelocFindLoadedFileContaining(ptr, bytes) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(ptr, bytes) != FALSE))
    {
        return ptr;
    }

    if ((context != NULL) &&
        (context->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(context->primary_file,
                                   offset,
                                   bytes) != FALSE))
    {
        return (const u8 *)context->primary_file->data + offset;
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      bytes) != FALSE)
        {
            return (const u8 *)sNdsRelocLoadedFiles[i].data + offset;
        }
    }

    return NULL;
}

static void ndsFighterMarioFoxCopyDLScanStats(u32 slot,
                                               const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }

    if (slot == 0u)
    {
        gNdsFighterDLScanP0Blocker = stats->blocker;
        gNdsFighterDLScanP0CommandCount = stats->command_count;
        gNdsFighterDLScanP0FirstOpcode = stats->first_opcode;
        gNdsFighterDLScanP0UnsupportedOpcode = stats->unsupported_opcode;
        gNdsFighterDLScanP0UnsupportedCommandCount =
            stats->unsupported_command_count;
        gNdsFighterDLScanP0VertexCommandCount =
            stats->vertex_command_count;
        gNdsFighterDLScanP0TriangleCommandCount =
            stats->triangle_command_count;
        gNdsFighterDLScanP0VertexCount = stats->vertex_count;
        gNdsFighterDLScanP0TriangleCount = stats->triangle_count;
        gNdsFighterDLScanP0EndCommandCount = stats->end_command_count;
        gNdsFighterDLScanP0BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLScanP0SegmentResolveCount = stats->segment_resolve_count;
        gNdsFighterDLScanP0TextureMask = stats->texture_mask;
        gNdsFighterDLScanP0OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLScanP0CullCommandCount = stats->cull_command_count;
        gNdsFighterDLScanP0StateCommandCount = stats->state_command_count;
        gNdsFighterDLScanP0SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLScanP0RenderCommandCount = stats->render_command_count;
        gNdsFighterDLScanP0MaxDepthSeen = stats->max_depth_seen;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLScanP1Blocker = stats->blocker;
        gNdsFighterDLScanP1CommandCount = stats->command_count;
        gNdsFighterDLScanP1FirstOpcode = stats->first_opcode;
        gNdsFighterDLScanP1UnsupportedOpcode = stats->unsupported_opcode;
        gNdsFighterDLScanP1UnsupportedCommandCount =
            stats->unsupported_command_count;
        gNdsFighterDLScanP1VertexCommandCount =
            stats->vertex_command_count;
        gNdsFighterDLScanP1TriangleCommandCount =
            stats->triangle_command_count;
        gNdsFighterDLScanP1VertexCount = stats->vertex_count;
        gNdsFighterDLScanP1TriangleCount = stats->triangle_count;
        gNdsFighterDLScanP1EndCommandCount = stats->end_command_count;
        gNdsFighterDLScanP1BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLScanP1SegmentResolveCount = stats->segment_resolve_count;
        gNdsFighterDLScanP1TextureMask = stats->texture_mask;
        gNdsFighterDLScanP1OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLScanP1CullCommandCount = stats->cull_command_count;
        gNdsFighterDLScanP1StateCommandCount = stats->state_command_count;
        gNdsFighterDLScanP1SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLScanP1RenderCommandCount = stats->render_command_count;
        gNdsFighterDLScanP1MaxDepthSeen = stats->max_depth_seen;
    }
}

static void ndsFighterMarioFoxScanDLForSlot(u32 slot, FTStruct *fp)
{
    DObj *root;
    DObj *selected;
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSFighterDLScanContext context;
    u32 dobj_index;
    u32 root_x_before;
    u32 root_x_after;

    if ((slot > 1u) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    selected = ndsFighterFindFirstDObjWithDL(root, &dobj_index);

    if (slot == 0u)
    {
        gNdsFighterDLScanP0RootXBeforeBits = root_x_before;
        gNdsFighterDLScanP0DObjIndex = dobj_index;
    }
    else
    {
        gNdsFighterDLScanP1RootXBeforeBits = root_x_before;
        gNdsFighterDLScanP1DObjIndex = dobj_index;
    }

    if (selected == NULL)
    {
        return;
    }

    dl = selected->dl;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));

    if (slot == 0u)
    {
        gNdsFighterDLScanP0FirstDL = (u32)(uintptr_t)dl;
        if (loaded != NULL)
        {
            gNdsFighterDLScanP0AssetID = loaded->asset_id;
            gNdsFighterDLScanP0Offset =
                (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
        }
        else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
        {
            gNdsFighterDLScanP0AssetID = NDS_FIGHTER_DL_SCAN_ASSET_ARENA;
            gNdsFighterDLScanP0Offset =
                (u32)((uintptr_t)dl - (uintptr_t)ndsTaskmanArenaStart());
        }
        else
        {
            gNdsFighterDLScanP0AssetID = 0xffffffffu;
            gNdsFighterDLScanP0Offset = 0xffffffffu;
        }
    }
    else
    {
        gNdsFighterDLScanP1FirstDL = (u32)(uintptr_t)dl;
        if (loaded != NULL)
        {
            gNdsFighterDLScanP1AssetID = loaded->asset_id;
            gNdsFighterDLScanP1Offset =
                (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
        }
        else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
        {
            gNdsFighterDLScanP1AssetID = NDS_FIGHTER_DL_SCAN_ASSET_ARENA;
            gNdsFighterDLScanP1Offset =
                (u32)((uintptr_t)dl - (uintptr_t)ndsTaskmanArenaStart());
        }
        else
        {
            gNdsFighterDLScanP1AssetID = 0xffffffffu;
            gNdsFighterDLScanP1Offset = 0xffffffffu;
        }
    }

    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

    context.primary_file = loaded;
    context.slot = slot;
    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsFighterDLScanValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLScanResolveBranch;
    config.resolve_data = ndsFighterDLScanResolveDataPointer;
    config.user = &context;

    ndsRendererInitStats(&stats);
    ndsRendererScanDisplayList(dl, &config, &stats);
    ndsFighterMarioFoxCopyDLScanStats(slot, &stats);

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;

    if (slot == 0u)
    {
        gNdsFighterDLScanP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLScanP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLScanP0GAAfter = (u32)fp->ga;
        gNdsFighterDLScanP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLScanP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLScanP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLScanP1GAAfter = (u32)fp->ga;
        gNdsFighterDLScanP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLScanCount++;
}

static void ndsFighterMarioFoxRunDLScanProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;

    if ((ndsFighterMarioFoxDLScanProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLScanResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDisplayResult ==
            NDS_FIGHTER_MARIOFOX_DISPLAY_PASS) &&
        (gNdsFighterMarioFoxDisplaySafeResult ==
            NDS_FIGHTER_MARIOFOX_DISPLAY_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDisplayMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDisplayCallbackCount == 2u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLScanMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();

    ndsFighterMarioFoxScanDLForSlot(0u, &sNdsFighterStructPool[0]);
    ndsFighterMarioFoxScanDLForSlot(1u, &sNdsFighterStructPool[1]);

    if ((gNdsFighterDLScanP0FirstDL != 0u) &&
        (gNdsFighterDLScanP1FirstDL != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterDLScanP0AssetID != 0xffffffffu) &&
        (gNdsFighterDLScanP1AssetID != 0xffffffffu))
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterMarioFoxDLScanCount == 2u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLScanP0CommandCount > 0u) &&
        (gNdsFighterDLScanP1CommandCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLScanP0FirstOpcode != 0u) &&
        (gNdsFighterDLScanP1FirstOpcode != 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterDLScanP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLScanP1Blocker == NDS_RENDERER_BLOCKER_NONE))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterDLScanP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLScanP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLScanP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLScanP1UnsupportedCommandCount == 0u))
    {
        mask |= 1u << 7;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLScanGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);

    if ((gNdsFighterDLScanP0StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLScanP1StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLScanP0MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLScanP1MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLScanP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLScanP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLScanP0RootXBeforeBits ==
            gNdsFighterDLScanP0RootXAfterBits) &&
        (gNdsFighterDLScanP1RootXBeforeBits ==
            gNdsFighterDLScanP1RootXAfterBits) &&
        (gNdsFighterDLScanGObjDelta == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterDLScanDrawCallCount == 0u) &&
        (gNdsFighterDLScanMatrixCallCount == 0u) &&
        (gNdsFighterDLScanGameplayUpdateCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLScanMask = mask;
    gNdsFighterMarioFoxDLScanDeferredMask = 0xffu;

    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLScanResult =
            NDS_FIGHTER_MARIOFOX_DL_SCAN_PASS;
        gNdsFighterMarioFoxDLScanSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_SCAN_SAFE_PASS;
    }
}

static u32 ndsFighterDLExecReadU32(const void *ptr)
{
    const u8 *bytes = ptr;

    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

static const void *ndsFighterDLExecResolveDataPointer(uintptr_t raw,
                                                      size_t bytes,
                                                      NDSFighterDLExecState *state)
{
    const void *ptr = (const void *)raw;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if ((ndsRelocFindLoadedFileContaining(ptr, bytes) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(ptr, bytes) != FALSE))
    {
        return ptr;
    }

    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   bytes) != FALSE))
    {
        return (const u8 *)state->primary_file->data + offset;
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      bytes) != FALSE)
        {
            return (const u8 *)sNdsRelocLoadedFiles[i].data + offset;
        }
    }
    return NULL;
}

static const void *ndsFighterDLExecResolveRendererData(const void *ptr,
                                                       size_t bytes,
                                                       void *user)
{
    return ndsFighterDLExecResolveDataPointer((uintptr_t)ptr, bytes, user);
}

static s32 ndsFighterDLExecValidateRange(const Gfx *dl, size_t bytes,
                                         void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLExecRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static void ndsFighterDLExecUpdateBounds(NDSFighterDLExecState *state,
                                         const NDSFighterDLExecVtx *vtx)
{
    if (state->bounds_valid == 0u)
    {
        state->min_x = state->max_x = vtx->x;
        state->min_y = state->max_y = vtx->y;
        state->min_z = state->max_z = vtx->z;
        state->bounds_valid = 1u;
    }
    else
    {
        if (vtx->x < state->min_x) { state->min_x = vtx->x; }
        if (vtx->x > state->max_x) { state->max_x = vtx->x; }
        if (vtx->y < state->min_y) { state->min_y = vtx->y; }
        if (vtx->y > state->max_y) { state->max_y = vtx->y; }
        if (vtx->z < state->min_z) { state->min_z = vtx->z; }
        if (vtx->z > state->max_z) { state->max_z = vtx->z; }
    }
}

static void ndsFighterDLExecDecodeVtx(NDSFighterDLExecState *state,
                                      u32 index, const u8 *src)
{
    NDSFighterDLExecVtx *dst;
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((state == NULL) || (src == NULL) || (index >= 32u))
    {
        return;
    }

    dst = &state->vertices[index];
    xy = ndsFighterDLExecReadU32(src + 0);
    zf = ndsFighterDLExecReadU32(src + 4);
    st = ndsFighterDLExecReadU32(src + 8);
    rgba = ndsFighterDLExecReadU32(src + 12);

    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = rgba >> 24;
    dst->g = rgba >> 16;
    dst->b = rgba >> 8;
    dst->a = rgba;
    dst->valid = TRUE;
    if (dst->a == 0)
    {
        dst->a = 0xffu;
    }

    state->vertex_valid_mask |= 1u << index;
    state->vertex_decoded_count++;
    state->color_checksum =
        (state->color_checksum * 33u) ^
        (u32)((u16)dst->x + ((u16)dst->y << 1) + ((u16)dst->z << 2)) ^
        rgba;
    ndsFighterDLExecUpdateBounds(state, dst);
}

static sb32 ndsFighterDLExecTriangleValid(NDSFighterDLExecState *state,
                                          u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);

    if ((state == NULL) || (i0 >= 32u) || (i1 >= 32u) || (i2 >= 32u))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static s32 ndsFighterMarioFoxVisitDLExecuteCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSFighterDLExecState *state = user;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    op = command->op;
    switch (op)
    {
    case NDS_FIGHTER_DL_OP_NOOP:
        return TRUE;

    case NDS_FIGHTER_DL_OP_MODIFYVTX:
    {
        u32 where = (command->w0 >> 16) & 0xffu;
        u32 packed_index = command->w0 & 0xffffu;
        u32 index = packed_index / 2u;

        if (where != NDS_RENDERER_ADAPTER_G_MWO_POINT_ST)
        {
            return TRUE;
        }
        if (((packed_index & 1u) != 0u) ||
            (index >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            ((state->vertex_valid_mask & (1u << index)) == 0u))
        {
            state->vertex_range_reject_count++;
            return TRUE;
        }
        state->vertices[index].s = (s16)(command->w1 >> 16);
        state->vertices[index].t = (s16)(command->w1 & 0xffffu);
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_VTX:
    {
        u32 v0;
        u32 count;
        size_t bytes;
        const u8 *src;
        u32 i;

        state->vertex_command_count++;
        if (ndsGBIDecodeF3DEX2Vtx(command->w0, 32u, &v0, &count) == FALSE)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        bytes = (size_t)count * 16u;
        src = ndsFighterDLExecResolveDataPointer((uintptr_t)command->w1,
                                                 bytes,
                                                 state);
        if (src == NULL)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        for (i = 0; i < count; i++)
        {
            ndsFighterDLExecDecodeVtx(state, v0 + i, src + (i * 16u));
        }
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_TRI1:
        state->triangle_command_count++;
        state->triangle_count++;
        if (ndsFighterDLExecTriangleValid(state,
                                          ndsGBIDecodeF3DEX2Tri1(command->w0))
            != FALSE)
        {
            state->triangle_valid_count++;
        }
        return TRUE;

    case NDS_FIGHTER_DL_OP_TRI2:
        state->triangle_command_count++;
        state->triangle_count += 2u;
        if (ndsFighterDLExecTriangleValid(state,
                                          ndsGBIDecodeF3DEX2Tri2First(
                                              command->w0)) != FALSE)
        {
            state->triangle_valid_count++;
        }
        if (ndsFighterDLExecTriangleValid(state,
                                          ndsGBIDecodeF3DEX2Tri2Second(
                                              command->w1)) != FALSE)
        {
            state->triangle_valid_count++;
        }
        return TRUE;

    case NDS_FIGHTER_DL_OP_CULLDL:
    case NDS_FIGHTER_DL_OP_TEXTURE:
    case NDS_FIGHTER_DL_OP_POPMTX:
    case NDS_FIGHTER_DL_OP_MTX:
    case NDS_FIGHTER_DL_OP_GEOMETRYMODE:
    case NDS_FIGHTER_DL_OP_MOVEWORD:
    case NDS_FIGHTER_DL_OP_SPECIAL_1:
    case NDS_FIGHTER_DL_OP_DL:
    case NDS_FIGHTER_DL_OP_ENDDL:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_H:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_L:
    case NDS_FIGHTER_DL_OP_SETSCISSOR:
    case NDS_FIGHTER_DL_OP_SETCOMBINE:
    case NDS_FIGHTER_DL_OP_SETCIMG:
    case NDS_FIGHTER_DL_OP_SETFOGCOLOR:
    case NDS_FIGHTER_DL_OP_SETBLENDCOLOR:
    case NDS_FIGHTER_DL_OP_SETENVCOLOR:
    case NDS_FIGHTER_DL_OP_SETPRIMCOLOR:
    case NDS_FIGHTER_DL_OP_SETTIMG:
    case NDS_FIGHTER_DL_OP_SETTILE:
    case NDS_FIGHTER_DL_OP_LOADBLOCK:
    case NDS_FIGHTER_DL_OP_LOADTLUT:
    case NDS_FIGHTER_DL_OP_SETTILESIZE:
    case NDS_FIGHTER_DL_OP_RDPSETOTHERMODE:
    case NDS_FIGHTER_DL_OP_RDPPIPESYNC:
    case NDS_FIGHTER_DL_OP_RDPLOADSYNC:
    case NDS_FIGHTER_DL_OP_RDPTILESYNC:
    case NDS_FIGHTER_DL_OP_RDPFULLSYNC:
        return TRUE;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_command_count++;
        return FALSE;
    }
}

static void ndsFighterMarioFoxCopyDLExecStats(
    u32 slot, const NDSFighterDLExecState *state,
    const NDSRendererStats *stats)
{
    if ((state == NULL) || (stats == NULL))
    {
        return;
    }

    if (slot == 0u)
    {
        gNdsFighterDLExecP0Blocker = stats->blocker;
        gNdsFighterDLExecP0CommandCount = stats->command_count;
        gNdsFighterDLExecP0FirstOpcode = stats->first_opcode;
        gNdsFighterDLExecP0UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLExecP0UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLExecP0VertexCommandCount = state->vertex_command_count;
        gNdsFighterDLExecP0VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLExecP0VertexValidMask = state->vertex_valid_mask;
        gNdsFighterDLExecP0TriangleCommandCount =
            state->triangle_command_count;
        gNdsFighterDLExecP0TriangleCount = state->triangle_count;
        gNdsFighterDLExecP0TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLExecP0MinX = state->min_x;
        gNdsFighterDLExecP0MaxX = state->max_x;
        gNdsFighterDLExecP0MinY = state->min_y;
        gNdsFighterDLExecP0MaxY = state->max_y;
        gNdsFighterDLExecP0MinZ = state->min_z;
        gNdsFighterDLExecP0MaxZ = state->max_z;
        gNdsFighterDLExecP0ColorChecksum = state->color_checksum;
        gNdsFighterDLExecP0OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLExecP0CullCommandCount = stats->cull_command_count;
        gNdsFighterDLExecP0StateCommandCount = stats->state_command_count;
        gNdsFighterDLExecP0SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLExecP0RenderCommandCount = stats->render_command_count;
        gNdsFighterDLExecP0BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLExecP0SegmentResolveCount =
            stats->segment_resolve_count;
        gNdsFighterDLExecP0TextureMask = stats->texture_mask;
        gNdsFighterDLExecVertexRangeRejectCount +=
            state->vertex_range_reject_count;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLExecP1Blocker = stats->blocker;
        gNdsFighterDLExecP1CommandCount = stats->command_count;
        gNdsFighterDLExecP1FirstOpcode = stats->first_opcode;
        gNdsFighterDLExecP1UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLExecP1UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLExecP1VertexCommandCount = state->vertex_command_count;
        gNdsFighterDLExecP1VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLExecP1VertexValidMask = state->vertex_valid_mask;
        gNdsFighterDLExecP1TriangleCommandCount =
            state->triangle_command_count;
        gNdsFighterDLExecP1TriangleCount = state->triangle_count;
        gNdsFighterDLExecP1TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLExecP1MinX = state->min_x;
        gNdsFighterDLExecP1MaxX = state->max_x;
        gNdsFighterDLExecP1MinY = state->min_y;
        gNdsFighterDLExecP1MaxY = state->max_y;
        gNdsFighterDLExecP1MinZ = state->min_z;
        gNdsFighterDLExecP1MaxZ = state->max_z;
        gNdsFighterDLExecP1ColorChecksum = state->color_checksum;
        gNdsFighterDLExecP1OtherModeCommandCount =
            stats->othermode_command_count;
        gNdsFighterDLExecP1CullCommandCount = stats->cull_command_count;
        gNdsFighterDLExecP1StateCommandCount = stats->state_command_count;
        gNdsFighterDLExecP1SkipCommandCount = stats->skip_command_count;
        gNdsFighterDLExecP1RenderCommandCount = stats->render_command_count;
        gNdsFighterDLExecP1BranchCommandCount = stats->branch_command_count;
        gNdsFighterDLExecP1SegmentResolveCount =
            stats->segment_resolve_count;
        gNdsFighterDLExecP1TextureMask = stats->texture_mask;
        gNdsFighterDLExecVertexRangeRejectCount +=
            state->vertex_range_reject_count;
    }
}

static void ndsFighterMarioFoxExecuteDLForSlot(u32 slot, FTStruct *fp)
{
    DObj *root;
    DObj *selected;
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSFighterDLExecState state;
    u32 root_x_before;
    u32 root_x_after;
    u32 unused_index;

    if ((slot > 1u) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    selected = ndsFighterFindFirstDObjWithDL(root, &unused_index);
    if (selected == NULL)
    {
        return;
    }

    dl = selected->dl;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

    bzero(&state, sizeof(state));
    state.primary_file = loaded;
    state.slot = slot;

    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsFighterDLExecValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLScanResolveBranch;
    config.resolve_data = ndsFighterDLExecResolveRendererData;
    config.user = &state;

    ndsRendererInitStats(&stats);
    ndsRendererExecuteDisplayList(dl,
                                  &config,
                                  ndsFighterMarioFoxVisitDLExecuteCommand,
                                  &state,
                                  &stats);
    ndsFighterMarioFoxCopyDLExecStats(slot, &state, &stats);

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLExecP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLExecP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLExecP0GAAfter = (u32)fp->ga;
        gNdsFighterDLExecP0RootXBeforeBits = root_x_before;
        gNdsFighterDLExecP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLExecP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLExecP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLExecP1GAAfter = (u32)fp->ga;
        gNdsFighterDLExecP1RootXBeforeBits = root_x_before;
        gNdsFighterDLExecP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLExecCount++;
}

static void ndsFighterMarioFoxRunDLExecuteProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;

    if ((ndsFighterMarioFoxDLExecuteProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLExecResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDLScanResult ==
            NDS_FIGHTER_MARIOFOX_DL_SCAN_PASS) &&
        (gNdsFighterMarioFoxDLScanSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_SCAN_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLScanMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLScanCount == 2u) &&
        (gNdsFighterDLScanP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLScanP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLScanP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLScanP1UnsupportedCommandCount == 0u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLExecMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();

    ndsFighterMarioFoxExecuteDLForSlot(0u, &sNdsFighterStructPool[0]);
    ndsFighterMarioFoxExecuteDLForSlot(1u, &sNdsFighterStructPool[1]);

    if ((gNdsFighterDLScanP0FirstDL != 0u) &&
        (gNdsFighterDLScanP1FirstDL != 0u))
    {
        mask |= 1u << 1;
    }
    if (gNdsFighterMarioFoxDLExecCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLExecP0CommandCount > 0u) &&
        (gNdsFighterDLExecP1CommandCount > 0u) &&
        (gNdsFighterDLExecP0FirstOpcode != 0u) &&
        (gNdsFighterDLExecP1FirstOpcode != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLExecP0VertexCommandCount > 0u) &&
        (gNdsFighterDLExecP1VertexCommandCount > 0u) &&
        (gNdsFighterDLExecP0VertexDecodedCount > 0u) &&
        (gNdsFighterDLExecP1VertexDecodedCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLExecP0TriangleCommandCount > 0u) &&
        (gNdsFighterDLExecP1TriangleCommandCount > 0u) &&
        (gNdsFighterDLExecP0TriangleCount > 0u) &&
        (gNdsFighterDLExecP1TriangleCount > 0u) &&
        (gNdsFighterDLExecP0TriangleValidCount > 0u) &&
        (gNdsFighterDLExecP1TriangleValidCount > 0u))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterDLExecP0MinX != gNdsFighterDLExecP0MaxX) ||
         (gNdsFighterDLExecP0MinY != gNdsFighterDLExecP0MaxY) ||
         (gNdsFighterDLExecP0MinZ != gNdsFighterDLExecP0MaxZ)) &&
        ((gNdsFighterDLExecP1MinX != gNdsFighterDLExecP1MaxX) ||
         (gNdsFighterDLExecP1MinY != gNdsFighterDLExecP1MaxY) ||
         (gNdsFighterDLExecP1MinZ != gNdsFighterDLExecP1MaxZ)) &&
        (gNdsFighterDLExecP0ColorChecksum != 0u) &&
        (gNdsFighterDLExecP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterDLExecP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLExecP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLExecP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLExecP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLExecRangeRejectCount == 0u) &&
        (gNdsFighterDLExecVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 7;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLExecGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);

    if ((gNdsFighterDLExecP0StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLExecP1StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLExecP0MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLExecP1MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLExecP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLExecP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLExecP0RootXBeforeBits ==
            gNdsFighterDLExecP0RootXAfterBits) &&
        (gNdsFighterDLExecP1RootXBeforeBits ==
            gNdsFighterDLExecP1RootXAfterBits) &&
        (gNdsFighterDLExecGObjDelta == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterDLExecDrawCallCount == 0u) &&
        (gNdsFighterDLExecMatrixCallCount == 0u) &&
        (gNdsFighterDLExecGameplayUpdateCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLExecMask = mask;
    gNdsFighterMarioFoxDLExecDeferredMask = 0xffu;

    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLExecResult =
            NDS_FIGHTER_MARIOFOX_DL_EXEC_PASS;
        gNdsFighterMarioFoxDLExecSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_EXEC_SAFE_PASS;
    }
}

static const Gfx *ndsFighterDLDrawResolveBranch(const Gfx *dl,
                                                 u32 *resolve_kind,
                                                 void *user)
{
    NDSFighterDLDrawState *state = user;
    uintptr_t raw = (uintptr_t)dl;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if (resolve_kind != NULL)
    {
        *resolve_kind = NDS_RENDERER_RESOLVE_NONE;
    }
    if ((ndsRelocFindLoadedFileContaining(dl, sizeof(Gfx)) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(Gfx)) != FALSE))
    {
        return dl;
    }
    if ((state != NULL) && (state->segment_e_base != NULL) &&
        ((raw >> 24) == 0x0eu))
    {
        uintptr_t base = (uintptr_t)state->segment_e_base;
        uintptr_t end = (uintptr_t)state->segment_e_end;

        if ((end > base) && (offset <= (end - base)) &&
            (sizeof(Gfx) <= (size_t)(end - base - offset)))
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            return (const Gfx *)(base + offset);
        }
    }
    if ((raw >> 24) == 0x0eu)
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        return sNdsRendererAdapterEmptySegmentEDL;
    }
    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   sizeof(Gfx)) != FALSE))
    {
        if (resolve_kind != NULL)
        {
            *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
        }
        return (const Gfx *)((const u8 *)state->primary_file->data + offset);
    }
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      sizeof(Gfx)) != FALSE)
        {
            if (resolve_kind != NULL)
            {
                *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
            }
            return (const Gfx *)((const u8 *)sNdsRelocLoadedFiles[i].data +
                                 offset);
        }
    }
    return dl;
}

static const void *ndsFighterDLDrawResolveDataPointer(uintptr_t raw,
                                                      size_t bytes,
                                                      NDSFighterDLDrawState *state)
{
    const void *ptr = (const void *)raw;
    uintptr_t offset = raw & 0x00ffffffu;
    u32 i;

    if ((ndsRelocFindLoadedFileContaining(ptr, bytes) != NULL) ||
        (ndsFighterDLScanRangeInTaskmanArena(ptr, bytes) != FALSE))
    {
        return ptr;
    }
    if ((state != NULL) && (state->segment_e_base != NULL) &&
        ((raw >> 24) == 0x0eu))
    {
        uintptr_t base = (uintptr_t)state->segment_e_base;
        uintptr_t end = (uintptr_t)state->segment_e_end;

        if ((end > base) && (offset <= (end - base)) &&
            (bytes <= (size_t)(end - base - offset)))
        {
            return (const void *)(base + offset);
        }
    }
    if ((raw >> 24) == 0x0eu)
    {
        return NULL;
    }
    if ((state != NULL) &&
        (state->primary_file != NULL) &&
        (ndsRelocRangeInLoadedFile(state->primary_file,
                                   offset,
                                   bytes) != FALSE))
    {
        return (const u8 *)state->primary_file->data + offset;
    }
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                      offset,
                                      bytes) != FALSE)
        {
            return (const u8 *)sNdsRelocLoadedFiles[i].data + offset;
        }
    }
    return NULL;
}

static const void *ndsFighterDLDrawResolveRendererData(const void *ptr,
                                                       size_t bytes,
                                                       void *user)
{
    return ndsFighterDLDrawResolveDataPointer((uintptr_t)ptr, bytes, user);
}

static s32 ndsFighterDLDrawValidateRange(const Gfx *dl, size_t bytes,
                                         void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLDrawRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static void ndsFighterDLDrawDecodeVtx(NDSFighterDLDrawState *state,
                                      u32 index, const u8 *src)
{
    NDSFighterDLDrawVtx *dst;
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((state == NULL) || (src == NULL) ||
        (index >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
    {
        return;
    }

    dst = &state->vertices[index];
    xy = ndsFighterDLExecReadU32(src + 0);
    zf = ndsFighterDLExecReadU32(src + 4);
    st = ndsFighterDLExecReadU32(src + 8);
    rgba = ndsFighterDLExecReadU32(src + 12);

    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = rgba >> 24;
    dst->g = rgba >> 16;
    dst->b = rgba >> 8;
    dst->a = rgba;
    if (dst->a == 0)
    {
        dst->a = 0xffu;
    }
    dst->valid = TRUE;

    state->vertex_valid_mask |= 1u << index;
    state->vertex_decoded_count++;
    state->color_checksum =
        (state->color_checksum * 33u) ^
        (u32)((u16)dst->x + ((u16)dst->y << 1) + ((u16)dst->z << 2)) ^
        rgba;
}

static u32 ndsFighterDLDrawCountValidVertices(u32 mask)
{
    u32 count = 0u;

    while (mask != 0u)
    {
        count += mask & 1u;
        mask >>= 1;
    }
    return count;
}

static void ndsFighterDLDrawSeedPersistentState(
    NDSFighterDLDrawState *state, const NDSFighterDLDrawState *persistent)
{
    if ((state == NULL) || (persistent == NULL))
    {
        return;
    }

    state->segment_e_base = persistent->segment_e_base;
    state->segment_e_end = persistent->segment_e_end;
    memcpy(state->vertices, persistent->vertices, sizeof(state->vertices));
    state->vertex_valid_mask = persistent->vertex_valid_mask;
    state->vertex_decoded_count =
        ndsFighterDLDrawCountValidVertices(state->vertex_valid_mask);
}

static void ndsFighterDLDrawCapturePersistentState(
    NDSFighterDLDrawState *persistent, const NDSFighterDLDrawState *state)
{
    if ((persistent == NULL) || (state == NULL))
    {
        return;
    }

    persistent->segment_e_base = state->segment_e_base;
    persistent->segment_e_end = state->segment_e_end;
    memcpy(persistent->vertices, state->vertices,
           sizeof(persistent->vertices));
    persistent->vertex_valid_mask = state->vertex_valid_mask;
}

static void ndsFighterDLDrawCopyPersistentRendererState(
    NDSRendererStats *dst, const NDSRendererStats *src)
{
#define NDS_RENDERER_COPY_STATE(field) dst->field = src->field

    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    NDS_RENDERER_COPY_STATE(othermode_h);
    NDS_RENDERER_COPY_STATE(othermode_l);
    NDS_RENDERER_COPY_STATE(geometry_mode);
    NDS_RENDERER_COPY_STATE(geometry_clear_mask);
    NDS_RENDERER_COPY_STATE(geometry_set_mask);
    NDS_RENDERER_COPY_STATE(texture_load_kind);
    NDS_RENDERER_COPY_STATE(texture_scale_s);
    NDS_RENDERER_COPY_STATE(texture_scale_t);
    NDS_RENDERER_COPY_STATE(texture_level);
    NDS_RENDERER_COPY_STATE(texture_tile);
    NDS_RENDERER_COPY_STATE(texture_on);
    NDS_RENDERER_COPY_STATE(texture_xparam);
    NDS_RENDERER_COPY_STATE(texture_state_flags);
    NDS_RENDERER_COPY_STATE(texture_image);
    NDS_RENDERER_COPY_STATE(texture_format);
    NDS_RENDERER_COPY_STATE(texture_size);
    NDS_RENDERER_COPY_STATE(texture_image_width);
    NDS_RENDERER_COPY_STATE(texture_tlut_image);
    NDS_RENDERER_COPY_STATE(texture_tlut_count);
    NDS_RENDERER_COPY_STATE(texture_tlut_tile);
    NDS_RENDERER_COPY_STATE(texture_render_tile);
    NDS_RENDERER_COPY_STATE(texture_render_tile_format);
    NDS_RENDERER_COPY_STATE(texture_render_tile_size);
    NDS_RENDERER_COPY_STATE(texture_render_tile_line);
    NDS_RENDERER_COPY_STATE(texture_render_tile_tmem);
    NDS_RENDERER_COPY_STATE(texture_render_tile_palette);
    NDS_RENDERER_COPY_STATE(texture_render_tile_cms);
    NDS_RENDERER_COPY_STATE(texture_render_tile_cmt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_masks);
    NDS_RENDERER_COPY_STATE(texture_render_tile_maskt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_shifts);
    NDS_RENDERER_COPY_STATE(texture_render_tile_shiftt);
    NDS_RENDERER_COPY_STATE(texture_render_tile_flags);
    NDS_RENDERER_COPY_STATE(texture_load_tile);
    NDS_RENDERER_COPY_STATE(texture_load_block_uls);
    NDS_RENDERER_COPY_STATE(texture_load_block_ult);
    NDS_RENDERER_COPY_STATE(texture_load_block_lrs);
    NDS_RENDERER_COPY_STATE(texture_load_block_dxt);
    NDS_RENDERER_COPY_STATE(texture_load_texels);
    NDS_RENDERER_COPY_STATE(texture_tile_size_tile);
    NDS_RENDERER_COPY_STATE(texture_tile_size_uls);
    NDS_RENDERER_COPY_STATE(texture_tile_size_ult);
    NDS_RENDERER_COPY_STATE(texture_tile_size_lrs);
    NDS_RENDERER_COPY_STATE(texture_tile_size_lrt);
    NDS_RENDERER_COPY_STATE(texture_tile_width);
    NDS_RENDERER_COPY_STATE(texture_tile_height);
    memcpy(dst->texture_tiles, src->texture_tiles, sizeof(dst->texture_tiles));
    NDS_RENDERER_COPY_STATE(texture_load_sequence);
    memcpy(dst->texture_loads, src->texture_loads,
           sizeof(dst->texture_loads));
    NDS_RENDERER_COPY_STATE(texture_combine_w0);
    NDS_RENDERER_COPY_STATE(texture_combine_w1);
    NDS_RENDERER_COPY_STATE(texture_combine_count);
    NDS_RENDERER_COPY_STATE(prim_color);
    NDS_RENDERER_COPY_STATE(prim_min_level);
    NDS_RENDERER_COPY_STATE(prim_lod_fraction);
    NDS_RENDERER_COPY_STATE(env_color);
    NDS_RENDERER_COPY_STATE(blend_color);
    NDS_RENDERER_COPY_STATE(light_color_1);
    NDS_RENDERER_COPY_STATE(light_color_2);
    NDS_RENDERER_COPY_STATE(light_color_mask);
    NDS_RENDERER_COPY_STATE(light_dir_x);
    NDS_RENDERER_COPY_STATE(light_dir_y);
    NDS_RENDERER_COPY_STATE(light_dir_z);
    NDS_RENDERER_COPY_STATE(light_dir_mask);
    NDS_RENDERER_COPY_STATE(prim_depth);
    NDS_RENDERER_COPY_STATE(prim_depth_delta);
    NDS_RENDERER_COPY_STATE(fog_color);
    NDS_RENDERER_COPY_STATE(fog_min);
    NDS_RENDERER_COPY_STATE(fog_max);
    NDS_RENDERER_COPY_STATE(fog_status);
    NDS_RENDERER_COPY_STATE(texture_source_hash1);
    NDS_RENDERER_COPY_STATE(texture_source_hash2);

#undef NDS_RENDERER_COPY_STATE
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static void ndsFighterDLDrawResetTransientRendererStats(
    NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }

    /* The prefix is exclusively per-list proof/counter output. The renderer
     * state begins at othermode_h and remains live across BattleShip's stage
     * heads and fighter-part lists. A few diagnostics are interleaved with
     * that state for the profile-2 ABI and are reset explicitly. */
    bzero(stats, offsetof(NDSRendererStats, othermode_h));
    stats->first_cull_w0 = 0u;
    stats->first_cull_w1 = 0u;
    stats->first_branch_dl = NULL;
    stats->first_resolved_branch_dl = NULL;
    stats->geometry_command_count = 0u;
    stats->texture_mask = 0u;
    stats->texture_command_count = 0u;
    stats->texture_set_tile_count = 0u;
    stats->prim_depth_command_count = 0u;
}

static void ndsFighterDLDrawResetRuntimeRendererStats(
    NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }

    /* Profiles 0/1 keep the ordered RDP state below intact. The null-callback
     * path only needs traversal guards and the owner-level hardware totals;
     * historical command/matrix/color ledgers remain profile-2 output. */
    stats->blocker = NDS_RENDERER_BLOCKER_NONE;
    stats->command_count = 0u;
    stats->unsupported_command_count = 0u;
    stats->end_command_count = 0u;
    stats->hardware_triangle_count = 0u;
    stats->hardware_zbuffer_triangle_count = 0u;
    stats->hardware_projected_depth_triangle_count = 0u;
    stats->hardware_decal_depth_triangle_count = 0u;
    stats->hardware_texture_bind_count = 0u;
    stats->hardware_texture_upload_count = 0u;
    stats->hardware_texture_ready_count = 0u;
    stats->hardware_texture_reject_count = 0u;
}
#endif

static sb32 ndsFighterDLDrawAppendTriangle(NDSFighterDLDrawState *state,
                                           u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);

    if ((state == NULL) ||
        (state->triangle_count >= NDS_FIGHTER_DL_DRAW_MAX_TRIS))
    {
        return FALSE;
    }

    state->tris[state->triangle_count].v0 = i0;
    state->tris[state->triangle_count].v1 = i1;
    state->tris[state->triangle_count].v2 = i2;
    state->triangle_count++;

    if ((i0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
        (i1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
        (i2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
    {
        return FALSE;
    }
    mask = (1u << i0) | (1u << i1) | (1u << i2);
    if ((state->vertex_valid_mask & mask) != mask)
    {
        return FALSE;
    }
    state->triangle_valid_count++;
    return TRUE;
}

static s32 ndsFighterMarioFoxVisitDLDrawCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSFighterDLDrawState *state = user;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    op = command->op;
    switch (op)
    {
    case NDS_FIGHTER_DL_OP_NOOP:
        return TRUE;

    case NDS_FIGHTER_DL_OP_MODIFYVTX:
        return TRUE;

    case NDS_FIGHTER_DL_OP_VTX:
    {
        u32 v0;
        u32 count;
        size_t bytes;
        const u8 *src;
        u32 i;

        if (ndsGBIDecodeF3DEX2Vtx(command->w0, NDS_FIGHTER_DL_DRAW_MAX_VTX,
                                  &v0, &count) == FALSE)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        bytes = (size_t)count * 16u;
        src = ndsFighterDLDrawResolveDataPointer((uintptr_t)command->w1,
                                                 bytes,
                                                 state);
        if (src == NULL)
        {
            state->vertex_range_reject_count++;
            return FALSE;
        }
        for (i = 0; i < count; i++)
        {
            ndsFighterDLDrawDecodeVtx(state, v0 + i, src + (i * 16u));
        }
        return TRUE;
    }

    case NDS_FIGHTER_DL_OP_TRI1:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri1(command->w0));
        return TRUE;

    case NDS_FIGHTER_DL_OP_TRI2:
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2First(
                                           command->w0));
        ndsFighterDLDrawAppendTriangle(state,
                                       ndsGBIDecodeF3DEX2Tri2Second(
                                           command->w1));
        return TRUE;

    case NDS_FIGHTER_DL_OP_CULLDL:
    case NDS_FIGHTER_DL_OP_TEXTURE:
    case NDS_FIGHTER_DL_OP_POPMTX:
    case NDS_FIGHTER_DL_OP_MTX:
    case NDS_FIGHTER_DL_OP_GEOMETRYMODE:
    case NDS_FIGHTER_DL_OP_MOVEWORD:
    case NDS_FIGHTER_DL_OP_SPECIAL_1:
    case NDS_FIGHTER_DL_OP_DL:
    case NDS_FIGHTER_DL_OP_ENDDL:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_H:
    case NDS_FIGHTER_DL_OP_SETOTHERMODE_L:
    case NDS_FIGHTER_DL_OP_SETSCISSOR:
    case NDS_FIGHTER_DL_OP_SETCOMBINE:
    case NDS_FIGHTER_DL_OP_SETCIMG:
    case NDS_FIGHTER_DL_OP_SETFOGCOLOR:
    case NDS_FIGHTER_DL_OP_SETBLENDCOLOR:
    case NDS_FIGHTER_DL_OP_SETENVCOLOR:
    case NDS_FIGHTER_DL_OP_SETPRIMCOLOR:
    case NDS_FIGHTER_DL_OP_SETTIMG:
    case NDS_FIGHTER_DL_OP_SETTILE:
    case NDS_FIGHTER_DL_OP_LOADBLOCK:
    case NDS_FIGHTER_DL_OP_LOADTLUT:
    case NDS_FIGHTER_DL_OP_SETTILESIZE:
    case NDS_FIGHTER_DL_OP_RDPSETOTHERMODE:
    case NDS_FIGHTER_DL_OP_RDPPIPESYNC:
    case NDS_FIGHTER_DL_OP_RDPLOADSYNC:
    case NDS_FIGHTER_DL_OP_RDPTILESYNC:
    case NDS_FIGHTER_DL_OP_RDPFULLSYNC:
        return TRUE;

    default:
        if (state->unsupported_opcode == 0u)
        {
            state->unsupported_opcode = op;
        }
        state->unsupported_command_count++;
        return FALSE;
    }
}

#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_STAGE_DL_HEADS 4u

static NDSFighterDLDrawState sNdsRendererAdapterStagePersistentState;
static NDSRendererStats sNdsRendererAdapterStagePersistentStats;
static NDSRendererVertexCache sNdsRendererAdapterStageVertexCache;
static sb32 sNdsRendererAdapterStagePersistentActive;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererAdapterStageOwnerOccurrence;
static u32 sNdsRendererAdapterStageNextOccurrence;
static u32 sNdsRendererAdapterStageListOrdinal;
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
#define NDS_RENDERER_OWNER_HASH_SEED 2166136261u

typedef struct NDSRendererOwnerStatsSnapshot
{
    u32 vertex_command_count;
    u32 source_vertex_count;
    u32 triangle_command_count;
    u32 triangle_count;
    u32 matrix_command_count;
} NDSRendererOwnerStatsSnapshot;

static u32 ndsRendererOwnerHashBytes(u32 hash, const void *data,
                                     size_t bytes)
{
    const u8 *cursor = data;
    size_t i;

    if (hash == 0u)
    {
        hash = NDS_RENDERER_OWNER_HASH_SEED;
    }
    for (i = 0u; i < bytes; i++)
    {
        hash ^= cursor[i];
        hash *= 16777619u;
    }
    /* Zero is the public "not started" sentinel for the compact ledgers.
     * Keep an intermediate hash from ever aliasing that sentinel. */
    if (hash == 0u)
    {
        hash = 1u;
    }
    return hash;
}

static u32 ndsRendererOwnerHashU32(u32 hash, u32 value)
{
    return ndsRendererOwnerHashBytes(hash, &value, sizeof(value));
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererOwnerRootBranchPath(
    const NDSRelocLoadedFile *loaded, const Gfx *dl, u32 selected_event)
{
    u32 hash = 0u;

    hash = ndsRendererOwnerHashU32(hash, 0x524f4f54u);
    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(hash, loaded->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl - (uintptr_t)loaded->data));
    }
    else if ((gSYTaskmanGraphicsHeap.start != NULL) &&
             (gSYTaskmanGraphicsHeap.end != NULL) &&
             ((uintptr_t)dl >=
              (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
             ((uintptr_t)dl <
              (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(hash, 2u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    else
    {
        /* Valid roots are reloc- or taskman-backed. Preserve a stable
         * segmented source token for any future resolver-backed root without
         * hashing its process address. */
        hash = ndsRendererOwnerHashU32(hash, 3u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl & 0x00ffffffu));
    }
    hash = ndsRendererOwnerHashU32(hash, selected_event);
    return hash;
}
#endif

#define NDS_RENDERER_OWNER_POINTER_NULL 0u
#define NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT 1u
#define NDS_RENDERER_OWNER_POINTER_RELOC 2u
#define NDS_RENDERER_OWNER_POINTER_TASKMAN 3u
#define NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP 4u
#define NDS_RENDERER_OWNER_POINTER_SEGMENTED 5u
#define NDS_RENDERER_OWNER_POINTER_RAW 6u

static u32 ndsRendererOwnerHashStablePointer(u32 hash, uintptr_t value)
{
    const NDSRelocLoadedFile *loaded;
    const u8 *arena = ndsTaskmanArenaStart();
    uintptr_t arena_base = (uintptr_t)arena;
    size_t arena_size = ndsTaskmanArenaSize();
    u32 segment = (u32)(value >> 24);

    hash = ndsRendererOwnerHashU32(hash, 0x50545231u);
    if (value == 0u)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_NULL);
    }
    if (value == (uintptr_t)sNdsRendererAdapterEmptySegmentEDL)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT);
    }

    loaded = ndsRelocFindLoadedFileContaining(
        (const void *)value, 1u);
    if (loaded != NULL)
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_RELOC);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, loaded->owner_generation);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - (uintptr_t)loaded->data));
    }
    if ((arena != NULL) && (value >= arena_base) &&
        ((size_t)(value - arena_base) < arena_size))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_TASKMAN);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - arena_base));
    }
    if ((gSYTaskmanGraphicsHeap.start != NULL) &&
        (gSYTaskmanGraphicsHeap.end != NULL) &&
        (value >= (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
        (value < (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    if ((segment != 0u) && (segment <= 0x0fu))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_SEGMENTED);
        hash = ndsRendererOwnerHashU32(hash, segment);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value & 0x00ffffffu));
    }

    /* Valid renderer operands are reloc-, taskman-, or segment-backed. Keep
     * an explicit raw fallback so an unexpected operand mutation is still
     * visible instead of silently aliasing the null provenance. */
    hash = ndsRendererOwnerHashU32(
        hash, NDS_RENDERER_OWNER_POINTER_RAW);
    return ndsRendererOwnerHashU32(hash, (u32)value);
}

static s32 ndsRendererOwnerCommandUsesPointer(u32 op)
{
    return ((op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_SETTIMG) ||
            (op == 0xfeu) || /* G_SETZIMG */
            (op == NDS_FIGHTER_DL_OP_SETCIMG)) ? TRUE : FALSE;
}

static u32 ndsRendererOwnerHashDisplayList(
    u32 hash, const Gfx *dl, const NDSRendererConfig *config,
    u32 depth, u32 *remaining_commands)
{
    u32 i;

    hash = ndsRendererOwnerHashU32(hash, 0x4c495354u);
    hash = ndsRendererOwnerHashStablePointer(
        hash, (uintptr_t)dl);
    hash = ndsRendererOwnerHashU32(hash, depth);
    if ((dl == NULL) || (config == NULL) ||
        (remaining_commands == NULL))
    {
        return ndsRendererOwnerHashU32(hash, 0xffffffffu);
    }
    if (depth > config->max_depth)
    {
        return ndsRendererOwnerHashU32(hash, 0xfffffffeu);
    }

    for (i = 0u; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;

        if (*remaining_commands == 0u)
        {
            return ndsRendererOwnerHashU32(hash, 0xfffffffdu);
        }
        if ((config->validate_range != NULL) &&
            (config->validate_range(dl, sizeof(*dl), config->user) == FALSE))
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)dl);
            return ndsRendererOwnerHashU32(hash, 0xfffffffcu);
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
        (*remaining_commands)--;
        hash = ndsRendererOwnerHashU32(hash, 0x434d4431u);
        hash = ndsRendererOwnerHashU32(hash, i);
        hash = ndsRendererOwnerHashU32(hash, w0);
        if (ndsRendererOwnerCommandUsesPointer(op) != FALSE)
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)w1);
        }
        else
        {
            hash = ndsRendererOwnerHashU32(hash, w1);
        }

        if (op == NDS_FIGHTER_DL_OP_DL)
        {
            const Gfx *branch = (const Gfx *)(uintptr_t)w1;
            u32 resolve_kind = NDS_RENDERER_RESOLVE_NONE;
            u32 branch_is_jump =
                ((w0 & (1u << 16)) != 0u) ? TRUE : FALSE;

            if (config->resolve_branch != NULL)
            {
                branch = config->resolve_branch(
                    branch, &resolve_kind, config->user);
            }
            hash = ndsRendererOwnerHashU32(hash, 0x4252414eu);
            hash = ndsRendererOwnerHashU32(hash, resolve_kind);
            hash = ndsRendererOwnerHashU32(hash, branch_is_jump);
            hash = ndsRendererOwnerHashDisplayList(
                hash, branch, config, depth + 1u, remaining_commands);
            if (branch_is_jump != FALSE)
            {
                return hash;
            }
        }
        else if (op == NDS_FIGHTER_DL_OP_ENDDL)
        {
            return ndsRendererOwnerHashU32(hash, 0x454e444cu);
        }
    }
    return ndsRendererOwnerHashU32(hash, 0x4e4f454eu);
}

static u32 ndsRendererOwnerHashTileState(
    u32 hash, const NDSRendererTileState *tile)
{
#define NDS_RENDERER_HASH_TILE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, tile->field)

    NDS_RENDERER_HASH_TILE_FIELD(set_seen);
    NDS_RENDERER_HASH_TILE_FIELD(size_seen);
    NDS_RENDERER_HASH_TILE_FIELD(format);
    NDS_RENDERER_HASH_TILE_FIELD(size);
    NDS_RENDERER_HASH_TILE_FIELD(line);
    NDS_RENDERER_HASH_TILE_FIELD(tmem);
    NDS_RENDERER_HASH_TILE_FIELD(palette);
    NDS_RENDERER_HASH_TILE_FIELD(cms);
    NDS_RENDERER_HASH_TILE_FIELD(cmt);
    NDS_RENDERER_HASH_TILE_FIELD(masks);
    NDS_RENDERER_HASH_TILE_FIELD(maskt);
    NDS_RENDERER_HASH_TILE_FIELD(shifts);
    NDS_RENDERER_HASH_TILE_FIELD(shiftt);
    NDS_RENDERER_HASH_TILE_FIELD(uls);
    NDS_RENDERER_HASH_TILE_FIELD(ult);
    NDS_RENDERER_HASH_TILE_FIELD(lrs);
    NDS_RENDERER_HASH_TILE_FIELD(lrt);
    NDS_RENDERER_HASH_TILE_FIELD(width);
    NDS_RENDERER_HASH_TILE_FIELD(height);
    NDS_RENDERER_HASH_TILE_FIELD(flags);

#undef NDS_RENDERER_HASH_TILE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashTextureLoadState(
    u32 hash, const NDSRendererTextureLoadState *load)
{
    hash = ndsRendererOwnerHashU32(hash, load->image);
    hash = ndsRendererOwnerHashU32(hash, load->sequence);
    hash = ndsRendererOwnerHashU32(hash, load->image_width);
    hash = ndsRendererOwnerHashU32(hash, load->load_uls);
    hash = ndsRendererOwnerHashU32(hash, load->load_ult);
    hash = ndsRendererOwnerHashU32(hash, load->load_lrs);
    hash = ndsRendererOwnerHashU32(hash, load->load_dxt);
    hash = ndsRendererOwnerHashU32(hash, load->load_texels);
    hash = ndsRendererOwnerHashU32(hash, load->load_tmem);
    hash = ndsRendererOwnerHashU32(hash, load->valid);
    hash = ndsRendererOwnerHashU32(hash, load->image_format);
    hash = ndsRendererOwnerHashU32(hash, load->image_size);
    hash = ndsRendererOwnerHashU32(hash, load->load_kind);
    hash = ndsRendererOwnerHashU32(hash, load->load_tile);
    return hash;
}

static u32 ndsRendererOwnerHashRuntimeState(const NDSRendererStats *stats)
{
    u32 hash = 0u;
    u32 i;

    if (stats == NULL)
    {
        return 0u;
    }

    /* Serialize exactly the persistent renderer contract copied by
     * ndsFighterDLDrawCopyPersistentRendererState().  Do not hash the raw
     * tail: it interleaves proof counters and pointer-bearing diagnostics,
     * and struct padding is not semantic state. */
#define NDS_RENDERER_HASH_STATE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, (u32)stats->field)

    NDS_RENDERER_HASH_STATE_FIELD(othermode_h);
    NDS_RENDERER_HASH_STATE_FIELD(othermode_l);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_mode);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_clear_mask);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_set_mask);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_kind);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_s);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_t);
    NDS_RENDERER_HASH_STATE_FIELD(texture_level);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_on);
    NDS_RENDERER_HASH_STATE_FIELD(texture_xparam);
    NDS_RENDERER_HASH_STATE_FIELD(texture_state_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_count);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_line);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_tmem);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_palette);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cms);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cmt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_masks);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_maskt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shifts);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shiftt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_dxt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_texels);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_height);
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTileState(hash,
                                              &stats->texture_tiles[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_sequence);
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTextureLoadState(
            hash, &stats->texture_loads[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w0);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_count);
    NDS_RENDERER_HASH_STATE_FIELD(prim_color);
    NDS_RENDERER_HASH_STATE_FIELD(prim_min_level);
    NDS_RENDERER_HASH_STATE_FIELD(prim_lod_fraction);
    NDS_RENDERER_HASH_STATE_FIELD(env_color);
    NDS_RENDERER_HASH_STATE_FIELD(blend_color);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_1);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_2);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_mask);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_x);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_y);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_z);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_mask);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth_delta);
    NDS_RENDERER_HASH_STATE_FIELD(fog_color);
    NDS_RENDERER_HASH_STATE_FIELD(fog_min);
    NDS_RENDERER_HASH_STATE_FIELD(fog_max);
    NDS_RENDERER_HASH_STATE_FIELD(fog_status);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash2);

#undef NDS_RENDERER_HASH_STATE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashVertexCache(
    const NDSRendererVertexCache *cache)
{
    u32 hash = 0u;
    u32 i;
    u32 row;
    u32 col;
    u32 snapshot_count;
    u32 input_mask;
    u32 transformed_mask;
    u32 color_mask;

    if (cache == NULL)
    {
        return 0u;
    }
    input_mask = cache->input_valid_mask;
    transformed_mask = cache->transformed_valid_mask & input_mask;
    color_mask = cache->vertex_color_valid_mask & input_mask;
    hash = ndsRendererOwnerHashU32(hash, input_mask);
    hash = ndsRendererOwnerHashU32(
        hash, cache->raw_vertex_fit_mask & input_mask);
    hash = ndsRendererOwnerHashU32(hash, transformed_mask);
    hash = ndsRendererOwnerHashU32(hash, color_mask);
    snapshot_count = cache->matrix_snapshot_count;
    if (snapshot_count > NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY)
    {
        snapshot_count = NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
    }
    hash = ndsRendererOwnerHashU32(hash, snapshot_count);
    for (i = 0u; i < NDS_RENDERER_VERTEX_CACHE_SIZE; i++)
    {
        u32 bit = 1u << i;

        if ((input_mask & bit) != 0u)
        {
            const NDSRendererInputVertex *input = &cache->input_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->s);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->t);
            hash = ndsRendererOwnerHashU32(hash, input->r);
            hash = ndsRendererOwnerHashU32(hash, input->g);
            hash = ndsRendererOwnerHashU32(hash, input->b);
            hash = ndsRendererOwnerHashU32(hash, input->a);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_matrix_snapshot[i]);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_clip_snapshot[i]);
        }
        if ((transformed_mask & bit) != 0u)
        {
            const NDSRendererClipVertex20p12 *clip =
                &cache->transformed_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)clip->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->w);
        }
        if ((color_mask & bit) != 0u)
        {
            hash = ndsRendererOwnerHashU32(hash,
                                           cache->vertex_colors[i]);
        }
    }
    for (i = 0u; i < snapshot_count; i++)
    {
        const NDSRendererMatrixSnapshot *snapshot =
            &cache->matrix_snapshots[i];

        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)snapshot->matrix.m[row][col]);
            }
        }
        hash = ndsRendererOwnerHashU32(hash, snapshot->generation);
        hash = ndsRendererOwnerHashU32(hash, snapshot->signature);
    }
    return hash;
}

static u32 ndsRendererOwnerHashResolver(
    const NDSFighterDLDrawState *state)
{
    u32 hash = 0u;
    uintptr_t base;
    uintptr_t end;
    size_t bytes;
    size_t i;

    if (state == NULL)
    {
        return 0u;
    }
    if (state->primary_file != NULL)
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->data_size);
    }
    else
    {
        hash = ndsRendererOwnerHashU32(hash, 0u);
    }

    base = (uintptr_t)state->segment_e_base;
    end = (uintptr_t)state->segment_e_end;
    if ((base == 0u) || (end <= base))
    {
        return ndsRendererOwnerHashU32(hash, 0u);
    }
    bytes = (size_t)(end - base);
    if (((bytes % sizeof(Gfx)) != 0u) ||
        (ndsFighterDLScanRangeInTaskmanArena(
             state->segment_e_base, bytes) == FALSE))
    {
        hash = ndsRendererOwnerHashU32(hash, 0xffffffffu);
        return ndsRendererOwnerHashU32(hash, (u32)bytes);
    }

    hash = ndsRendererOwnerHashU32(hash, (u32)(bytes / sizeof(Gfx)));
    for (i = 0u; i < (bytes / sizeof(Gfx)); i++)
    {
        const Gfx *command = &state->segment_e_base[i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;

        hash = ndsRendererOwnerHashU32(
            hash, w0);
        if ((op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_SETTIMG))
        {
            const void *pointer = (const void *)(uintptr_t)w1;
            const NDSRelocLoadedFile *loaded = NULL;
            uintptr_t pointer_value = (uintptr_t)pointer;

            if ((pointer_value >= base) && (pointer_value < end))
            {
                hash = ndsRendererOwnerHashU32(hash, 1u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value - base));
                continue;
            }
            loaded = ndsRelocFindLoadedFileContaining(pointer, 1u);
            if (loaded != NULL)
            {
                hash = ndsRendererOwnerHashU32(hash, 2u);
                hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
                hash = ndsRendererOwnerHashU32(
                    hash, loaded->owner_generation);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)loaded->data));
                continue;
            }
            if ((gSYTaskmanGraphicsHeap.start != NULL) &&
                (gSYTaskmanGraphicsHeap.end != NULL) &&
                (pointer_value >=
                 (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
                (pointer_value <
                 (uintptr_t)gSYTaskmanGraphicsHeap.end))
            {
                hash = ndsRendererOwnerHashU32(hash, 3u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)gSYTaskmanGraphicsHeap.start));
                continue;
            }
            hash = ndsRendererOwnerHashU32(hash, 4u);
        }
        hash = ndsRendererOwnerHashU32(hash, w1);
    }
    return hash;
}


static void ndsRendererOwnerSnapshotStats(
    const NDSRendererStats *stats, NDSRendererOwnerStatsSnapshot *snapshot)
{
    snapshot->vertex_command_count = stats->vertex_command_count;
    snapshot->source_vertex_count = stats->source_vertex_count;
    snapshot->triangle_command_count = stats->triangle_command_count;
    snapshot->triangle_count = stats->triangle_count;
    snapshot->matrix_command_count = stats->matrix_command_count;
}

static void ndsRendererOwnerAccumulateList(
    NDSRendererProfileOwner owner_id,
    const NDSRelocLoadedFile *loaded,
    const Gfx *dl,
    u32 selected_event,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererConfig *config,
    const NDSRendererOwnerStatsSnapshot *before,
    const NDSRendererStats *after)
{
    volatile NDSRendererOwnerProfile *owner;
    u32 dl_offset;
    u32 remaining_commands;

    if ((u32)owner_id >= (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        return;
    }
    owner = &gNdsRendererProfileOwners[(u32)owner_id];
    owner->selected_count++;
    owner->source_command_count += after->command_count;
    owner->vertex_command_count +=
        after->vertex_command_count - before->vertex_command_count;
    owner->source_vertex_count +=
        after->source_vertex_count - before->source_vertex_count;
    owner->triangle_command_count +=
        after->triangle_command_count - before->triangle_command_count;
    owner->triangle_count += after->triangle_count - before->triangle_count;
    /* Every selected list binds its live camera/DObj matrix pair before the
     * source stream runs.  Source MTX commands, when present, are additional
     * changes rather than the whole owner-level matrix census. */
    owner->matrix_change_count += 1u +
        after->matrix_command_count - before->matrix_command_count;

    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        dl_offset = (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
    }
    else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
    {
        dl_offset = (u32)((uintptr_t)dl -
                          (uintptr_t)ndsTaskmanArenaStart());
    }
    else
    {
        dl_offset = (u32)((uintptr_t)dl & 0x00ffffffu);
    }
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, 0x4f574e31u);
    owner->topology_signature = ndsRendererOwnerHashStablePointer(
        owner->topology_signature, (uintptr_t)dl);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, after->command_count);
    remaining_commands = (config != NULL) ? config->max_commands : 0u;
    owner->topology_signature = ndsRendererOwnerHashDisplayList(
        owner->topology_signature, dl, config, 0u,
        &remaining_commands);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, remaining_commands);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, selected_event);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, dl_offset);
    if (projection != NULL)
    {
        owner->camera_signature = ndsRendererOwnerHashBytes(
            owner->camera_signature, projection, sizeof(*projection));
    }
    if (modelview != NULL)
    {
        owner->dobj_matrix_signature = ndsRendererOwnerHashBytes(
            owner->dobj_matrix_signature, modelview, sizeof(*modelview));
    }
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->prim_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->env_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->blend_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w0);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_2);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_x);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_y);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_z);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_tlut_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_s);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_t);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_render_tile);
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererAdapterAccumulateDepth(
    const NDSRendererStats *stats,
    volatile u32 *samples,
    volatile s32 *depth_min,
    volatile s32 *depth_max,
    volatile s32 *w_min,
    volatile s32 *w_max)
{
    if ((stats == NULL) || (samples == NULL) || (depth_min == NULL) ||
        (depth_max == NULL) || (w_min == NULL) || (w_max == NULL) ||
        (stats->hardware_projected_depth_sample_count == 0u))
    {
        return;
    }
    if (*samples == 0u)
    {
        *depth_min = stats->hardware_projected_depth_min;
        *depth_max = stats->hardware_projected_depth_max;
        *w_min = stats->hardware_projected_w_min;
        *w_max = stats->hardware_projected_w_max;
    }
    else
    {
        if (stats->hardware_projected_depth_min < *depth_min)
        {
            *depth_min = stats->hardware_projected_depth_min;
        }
        if (stats->hardware_projected_depth_max > *depth_max)
        {
            *depth_max = stats->hardware_projected_depth_max;
        }
        if (stats->hardware_projected_w_min < *w_min)
        {
            *w_min = stats->hardware_projected_w_min;
        }
        if (stats->hardware_projected_w_max > *w_max)
        {
            *w_max = stats->hardware_projected_w_max;
        }
    }
    *samples += stats->hardware_projected_depth_sample_count;
}
#endif

void ndsRendererAdapterResetDepthDiagnostics(void)
{
    gNdsRendererDepthStageSamples = 0u;
    gNdsRendererDepthFighterP0Samples = 0u;
    gNdsRendererDepthFighterP1Samples = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence = 0u;
    sNdsRendererAdapterStageNextOccurrence = 0u;
    sNdsRendererAdapterStageListOrdinal = 0u;
#endif
}

static sb32 ndsRendererAdapterStatsHasArmedTexture(
    const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->texture_image != 0u) ||
             (stats->texture_tlut_image != 0u) ||
             (stats->texture_on != 0u))) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterStatsHasArmedTile(
    const NDSRendererStats *stats)
{
    u32 i;

    if (stats == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        const NDSRendererTileState *tile = &stats->texture_tiles[i];

        if ((tile->set_seen != 0u) || (tile->size_seen != 0u) ||
            (tile->line != 0u) || (tile->width != 0u) ||
            (tile->height != 0u))
        {
            return TRUE;
        }
    }
    return FALSE;
}

void ndsRendererAdapterBeginStageTraversal(void)
{
    bzero(&sNdsRendererAdapterStagePersistentState,
          sizeof(sNdsRendererAdapterStagePersistentState));
    ndsRendererInitStats(&sNdsRendererAdapterStagePersistentStats);
    ndsRendererInitVertexCache(&sNdsRendererAdapterStageVertexCache);
    sNdsRendererAdapterStagePersistentActive = TRUE;
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence =
        sNdsRendererAdapterStageNextOccurrence++;
    sNdsRendererAdapterStageListOrdinal = 0u;
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
}

void ndsRendererAdapterEndStageTraversal(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif
    sNdsRendererAdapterStagePersistentActive = FALSE;
}

static s32 ndsRendererAdapterStageValidateRange(const Gfx *dl, size_t bytes,
                                                void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        return FALSE;
    }
    return TRUE;
}

static sb32 ndsRendererAdapterStageDObjDrawable(DObj *dobj, u32 kind)
{
    if (dobj == NULL)
    {
        return FALSE;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
    {
        return FALSE;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
        return ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0) ? TRUE : FALSE;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        return (dobj->flags == DOBJ_FLAG_NONE) ? TRUE : FALSE;

    default:
        return FALSE;
    }
}

static u32 ndsRendererAdapterMaterialFlags(const MObj *mobj)
{
    u32 flags;

    if (mobj == NULL)
    {
        return MOBJ_FLAG_NONE;
    }
    flags = mobj->sub.flags;
    return (flags == MOBJ_FLAG_NONE) ?
        (MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA) : flags;
}

static u32 ndsRendererAdapterMaterialPositiveOrOne(s32 value)
{
    return (value <= 0) ? 1u : (u32)value;
}

static void ndsRendererAdapterMaterialLoadBlock(const MObj *mobj,
                                                u32 *texels,
                                                u32 *dxt)
{
    s32 load_texels = 0;
    u32 divisor = 1u;

    if ((mobj == NULL) || (texels == NULL) || (dxt == NULL))
    {
        return;
    }

    switch (mobj->sub.block_siz)
    {
    case G_IM_SIZ_4b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 3) >> 2) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 16);
        break;
    case G_IM_SIZ_8b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 1) >> 1) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 8);
        break;
    case G_IM_SIZ_16b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 2) / 8);
        break;
    case G_IM_SIZ_32b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 4) / 8);
        break;
    default:
        break;
    }

    *texels = (load_texels > 0) ? (u32)load_texels : 0u;
    *dxt = (divisor + 0x7ffu) / divisor;
}

static u32 ndsRendererAdapterMaterialCommandCount(const MObj *mobj, u32 flags)
{
    u32 count = 1u;

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj != NULL) &&
        (mobj->sub.palettes != NULL))
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        count++;
    }
    if ((flags & 0x20u) != 0)
    {
        count++;
    }
    if ((flags & 0x40u) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        count++;
    }
    return count;
}

static sb32 ndsRendererAdapterCountMaterialCommands(DObj *dobj,
                                                    u32 *mobj_count,
                                                    u32 *branch_commands)
{
    MObj *mobj;
    u32 count = 0u;
    u32 commands = 0u;

    if ((dobj == NULL) || (mobj_count == NULL) ||
        (branch_commands == NULL))
    {
        return FALSE;
    }
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        count++;
        if (count > NDS_RENDERER_ADAPTER_MATERIAL_MOBJ_MAX)
        {
            return FALSE;
        }
        commands += ndsRendererAdapterMaterialCommandCount(
            mobj, ndsRendererAdapterMaterialFlags(mobj));
    }
    *mobj_count = count;
    *branch_commands = commands;
    return TRUE;
}

static void ndsRendererAdapterMaterialTextureState(
    const MObj *mobj,
    u32 flags,
    f32 *scau,
    f32 *scav,
    f32 *trau,
    f32 *trav,
    f32 *scrollu,
    f32 *scrollv)
{
    if ((mobj == NULL) || (scau == NULL) || (scav == NULL) ||
        (trau == NULL) || (trav == NULL) || (scrollu == NULL) ||
        (scrollv == NULL) ||
        ((flags & (MOBJ_FLAG_TEXTURE | 0x40u | 0x20u)) == 0))
    {
        return;
    }

    *scau = mobj->sub.scau;
    *scav = mobj->sub.scav;
    *trau = mobj->sub.trau;
    *trav = mobj->sub.trav;
    *scrollu = mobj->sub.scrollu;
    *scrollv = mobj->sub.scrollv;

    if (mobj->sub.unk10 == 1)
    {
        *scau *= 0.5F;
        *trau =
            ((*trau - mobj->sub.unk24) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        *scrollu =
            ((*scrollu - mobj->sub.unk44) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
    }
}

static u32 ndsRendererAdapterClampU8S32(s32 value)
{
    if (value < 0)
    {
        return 0u;
    }
    return (value > 0xff) ? 0xffu : (u32)value;
}

static u32 ndsRendererAdapterClampU8F32(f32 value)
{
    return ndsRendererAdapterClampU8S32((s32)value);
}

static u32 ndsRendererAdapterPackColor(const SYColorPack *color)
{
    if (color == NULL)
    {
        return 0u;
    }
    return ((u32)color->s.r << 24) |
           ((u32)color->s.g << 16) |
           ((u32)color->s.b << 8) |
           (u32)color->s.a;
}

static const void *ndsRendererAdapterReadPointerEntry(void **items,
                                                      s32 index)
{
    if ((items == NULL) || (index < 0))
    {
        return NULL;
    }
    return items[index];
}

static void ndsRendererAdapterEmitBranchTableCommand(Gfx *cmd,
                                                     const Gfx *branch)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = (NDS_FIGHTER_DL_OP_DL << 24) | (1u << 16);
    cmd->words.w1 = (u32)(uintptr_t)branch;
}

static void ndsRendererAdapterEmitEndDL(Gfx *cmd)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_ENDDL << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitSync(Gfx *cmd, u32 op)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitTextureImage(Gfx *cmd,
                                               u32 fmt,
                                               u32 siz,
                                               u32 width,
                                               const void *image)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    cmd->words.w1 = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterEmitSetTile(Gfx *cmd,
                                          u32 fmt,
                                          u32 siz,
                                          u32 line,
                                          u32 tmem,
                                          u32 tile,
                                          u32 palette,
                                          u32 cmt,
                                          u32 maskt,
                                          u32 shiftt,
                                          u32 cms,
                                          u32 masks,
                                          u32 shifts)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterEmitLoadTlut(Gfx *cmd, u32 tile, u32 count)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_LOADTLUT << 24;
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((count & 0x03ffu) << 14);
}

static void ndsRendererAdapterEmitMoveWord(Gfx *cmd,
                                           u32 index,
                                           u32 offset,
                                           u32 data)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_MOVEWORD << 24) |
        ((offset & 0xffffu) << 8) |
        (index & 0xffu);
    cmd->words.w1 = data;
}

static Gfx *ndsRendererAdapterEmitLightColor(Gfx *branch_dl,
                                             u32 light,
                                             u32 color)
{
    u32 offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_1;
    u32 offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_1;

    if (branch_dl == NULL)
    {
        return branch_dl;
    }
    if (light == 2u)
    {
        offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_2;
        offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_2;
    }
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_a,
                                   color);
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_b,
                                   color);
    return branch_dl;
}

static void ndsRendererAdapterEmitPrimColor(Gfx *cmd,
                                            u32 m,
                                            u32 l,
                                            u32 r,
                                            u32 g,
                                            u32 b,
                                            u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
        ((m & 0xffu) << 8) |
        (l & 0xffu);
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitColor(Gfx *cmd,
                                        u32 op,
                                        u32 r,
                                        u32 g,
                                        u32 b,
                                        u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitLoadBlock(Gfx *cmd,
                                            u32 tile,
                                            u32 uls,
                                            u32 ult,
                                            u32 lrs,
                                            u32 dxt)
{
    if (cmd == NULL)
    {
        return;
    }
    if (lrs > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
    {
        lrs = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_LOADBLOCK << 24) |
        ((uls & 0x0fffu) << 12) |
        (ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((lrs & 0x0fffu) << 12) |
        (dxt & 0x0fffu);
}

static void ndsRendererAdapterEmitTileSize(Gfx *cmd,
                                           u32 tile,
                                           s32 uls,
                                           s32 ult,
                                           s32 lrs,
                                           s32 lrt)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static void ndsRendererAdapterEmitTexture(Gfx *cmd,
                                          u32 s,
                                          u32 t,
                                          u32 level,
                                          u32 tile,
                                          u32 on)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
        ((level & 0x7u) << 11) |
        ((tile & 0x7u) << 8) |
        ((on & 0x7fu) << 1);
    cmd->words.w1 =
        ((s & 0xffffu) << 16) |
        (t & 0xffffu);
}

static void ndsRendererAdapterNativeMaterialImage(
    u32 fmt, u32 siz, u32 width, const void *image,
    u32 *out_w0, u32 *out_image)
{
    if ((out_w0 == NULL) || (out_image == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    *out_image = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterNativeMaterialTile(
    u32 fmt, u32 siz, u32 line, u32 tmem, u32 tile, u32 palette,
    u32 cmt, u32 maskt, u32 shiftt, u32 cms, u32 masks, u32 shifts,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterNativeMaterialTileSize(
    u32 tile, s32 uls, s32 ult, s32 lrs, s32 lrt,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static sb32 ndsRendererAdapterBuildNativeMaterial(
    MObj *mobj, NDSRendererNativeMaterial *out)
{
    u32 flags;
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;

    if ((mobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    bzero(out, sizeof(*out));
    flags = ndsRendererAdapterMaterialFlags(mobj);
    out->command_count = 1u; /* ENDDL */
    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
            ndsRendererAdapterNativeMaterialImage(
                G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette,
                &out->palette_image_w0, &out->palette_image);
            out->command_count++;
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id),
            &out->palette_image_w0, &out->palette_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0u)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT;
            ndsRendererAdapterNativeMaterialTile(
                G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u, 0u,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                &out->palette_tile_w0, &out->palette_tile_w1);
            out->palette_tlut_w1 =
                (5u << 24) |
                (((mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu) << 14);
            out->sync_count += 3u;
            out->command_count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT1;
        out->light1 = ndsRendererAdapterPackColor(&mobj->sub.light1color);
        out->command_count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT2;
        out->light2 = ndsRendererAdapterPackColor(&mobj->sub.light2color);
        out->command_count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0u)
    {
        u32 level;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PRIM;
        if ((flags & MOBJ_FLAG_FRAC) != 0u)
        {
            s32 trunc = (s32)mobj->lfrac;

            level = ndsRendererAdapterClampU8F32(
                (mobj->lfrac - (f32)trunc) * 256.0F);
            mobj->texture_id_curr = trunc;
            mobj->texture_id_next = trunc + 1;
        }
        else
        {
            level = ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F);
        }
        out->prim_w0 =
            (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
            (((u32)mobj->sub.prim_m & 0xffu) << 8) |
            (level & 0xffu);
        out->prim_w1 = ndsRendererAdapterPackColor(&mobj->sub.primcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_ENV;
        out->env_color = ndsRendererAdapterPackColor(&mobj->sub.envcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLEND;
        out->blend_color =
            ndsRendererAdapterPackColor(&mobj->sub.blendcolor);
        out->command_count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0u)
    {
        u32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.block_fmt, block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_next),
            &out->block_image_w0, &out->block_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            if (texels > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
            {
                texels = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
            }
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK;
            out->load_block_w0 = NDS_FIGHTER_DL_OP_LOADBLOCK << 24;
            out->load_block_w1 =
                (6u << 24) | ((texels & 0x0fffu) << 12) |
                (dxt & 0x0fffu);
            out->sync_count += 2u;
            out->command_count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_curr),
            &out->current_image_w0, &out->current_image);
        out->command_count++;
    }
    if ((flags & 0x20u) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0) { uls = 0; }
            if (ult < 0) { ult = 0; }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult,
            &out->render_tile_size_w0, &out->render_tile_size_w1);
        out->command_count++;
    }
    if ((flags & 0x40u) != 0u)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult,
            &out->scroll_tile_size_w0, &out->scroll_tile_size_w1);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff) { s = 0xffff; }
        if (t > 0xffff) { t = 0xffff; }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;
        out->texture_w0 =
            (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
            (NDS_RENDERER_ADAPTER_G_TX_RENDERTILE << 8) |
            (NDS_RENDERER_ADAPTER_G_ON << 1);
        out->texture_w1 =
            (((u32)s & 0xffffu) << 16) | ((u32)t & 0xffffu);
        out->command_count++;
    }
    return TRUE;
}

static sb32 ndsRendererAdapterPrepareNativeMaterials(
    DObj *dobj, NDSRendererNativeMaterial *materials,
    u32 capacity, u32 *out_count)
{
    MObj *mobj;
    u32 count = 0u;

    if ((materials == NULL) || (out_count == NULL))
    {
        return FALSE;
    }
    *out_count = 0u;
    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        return TRUE;
    }
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        count++;
        if (count > capacity)
        {
            return FALSE;
        }
    }
    count = 0u;
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        if (ndsRendererAdapterBuildNativeMaterial(
                mobj, &materials[count]) == FALSE)
        {
            return FALSE;
        }
        count++;
    }
    *out_count = count;
    return TRUE;
}

static u32 ndsRendererAdapterSaveNativeMaterialTextureIds(
    DObj *dobj,
    s32 *curr,
    s32 *next,
    u32 capacity)
{
    MObj *mobj;
    u32 count = 0u;

    if ((dobj == NULL) || (curr == NULL) || (next == NULL))
    {
        return 0u;
    }
    for (mobj = dobj->mobj;
         (mobj != NULL) && (count < capacity);
         mobj = mobj->next)
    {
        curr[count] = mobj->texture_id_curr;
        next[count] = mobj->texture_id_next;
        count++;
    }
    return count;
}

static void ndsRendererAdapterRestoreNativeMaterialTextureIds(
    DObj *dobj,
    const s32 *curr,
    const s32 *next,
    u32 count)
{
    MObj *mobj;
    u32 i = 0u;

    if ((dobj == NULL) || (curr == NULL) || (next == NULL))
    {
        return;
    }
    for (mobj = dobj->mobj;
         (mobj != NULL) && (i < count);
         mobj = mobj->next, i++)
    {
        mobj->texture_id_curr = curr[i];
        mobj->texture_id_next = next[i];
    }
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterValidateNativeOwnerMaterials(
    const NDSRendererNativeMaterial *materials,
    u32 material_count)
{
    u32 i;

    if ((material_count != 0u) && (materials == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < material_count; i++)
    {
        const NDSRendererNativeMaterial *material = &materials[i];
        u32 effects = material->effects;

        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->palette_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->block_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->current_image,
                 1u) == NULL))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
    DObj *const *material_dobjs,
    u32 root_count)
{
    if (material_dobjs == NULL)
    {
        return;
    }
    /* A contract may select one material DObj more than once. Roll back in
     * reverse event order so each saved pre-event state is restored and the
     * earliest snapshot remains live for the ordinary renderer fallback. */
    while (root_count != 0u)
    {
        u32 root_index = --root_count;

        ndsRendererAdapterRestoreNativeMaterialTextureIds(
            material_dobjs[root_index],
            sNdsRendererAdapterNativeOwnerTextureCurr[root_index],
            sNdsRendererAdapterNativeOwnerTextureNext[root_index],
            sNdsRendererAdapterNativeOwnerTextureCounts[root_index]);
    }
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterValidateNativeOwnerCached(
    u32 slot,
    const NDSRelocLoadedFile *owner_file,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
    NDSRendererAdapterNativeOwnerValidationCache *cache;
    u32 i;
    sb32 identity_matches;

    if ((slot > 1u) || (owner_file == NULL) ||
        (root_offsets == NULL) || (material_counts == NULL) ||
        (root_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    cache = &sNdsRendererAdapterNativeOwnerValidationCache[slot];
    identity_matches =
        ((cache->valid != 0u) &&
         (cache->data == owner_file->data) &&
         (cache->asset_id == owner_file->asset_id) &&
         (cache->owner_generation == owner_file->owner_generation) &&
         (cache->data_size == owner_file->data_size) &&
         (cache->root_count == root_count)) ? TRUE : FALSE;
    if (identity_matches != FALSE)
    {
        for (i = 0u; i < root_count; i++)
        {
            if ((cache->root_offsets[i] != root_offsets[i]) ||
                (cache->material_counts[i] != material_counts[i]))
            {
                identity_matches = FALSE;
                break;
            }
        }
    }
    if (identity_matches != FALSE)
    {
        return TRUE;
    }

    cache->valid = FALSE;
    if (ndsRendererValidateNativeFighterOwner(
            slot, owner_file->data_size, root_count,
            root_offsets, material_counts) == FALSE)
    {
        return FALSE;
    }
    cache->data = owner_file->data;
    cache->asset_id = owner_file->asset_id;
    cache->owner_generation = owner_file->owner_generation;
    cache->data_size = owner_file->data_size;
    cache->root_count = root_count;
    for (i = 0u; i < root_count; i++)
    {
        cache->root_offsets[i] = root_offsets[i];
        cache->material_counts[i] = material_counts[i];
    }
    cache->valid = TRUE;
    return TRUE;
}
#endif

static Gfx *ndsRendererAdapterEmitMaterialCommands(Gfx *branch_dl, MObj *mobj)
{
    u32 flags = ndsRendererAdapterMaterialFlags(mobj);
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;

    if ((branch_dl == NULL) || (mobj == NULL))
    {
        return branch_dl;
    }

    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            ndsRendererAdapterEmitTextureImage(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette);
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id));
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPTILESYNC);
            ndsRendererAdapterEmitSetTile(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u,
                0u, NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadTlut(
                branch_dl++, 5u,
                (mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPPIPESYNC);
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 1u,
            ndsRendererAdapterPackColor(&mobj->sub.light1color));
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 2u,
            ndsRendererAdapterPackColor(&mobj->sub.light2color));
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        if ((flags & MOBJ_FLAG_FRAC) != 0)
        {
            s32 trunc = (s32)mobj->lfrac;

            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(
                    (mobj->lfrac - (f32)trunc) * 256.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
            mobj->texture_id_curr = trunc;
            mobj->texture_id_next = trunc + 1;
        }
        else
        {
            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
        }
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETENVCOLOR,
            mobj->sub.envcolor.s.r, mobj->sub.envcolor.s.g,
            mobj->sub.envcolor.s.b, mobj->sub.envcolor.s.a);
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETBLENDCOLOR,
            mobj->sub.blendcolor.s.r, mobj->sub.blendcolor.s.g,
            mobj->sub.blendcolor.s.b, mobj->sub.blendcolor.s.a);
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        s32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.block_fmt, (u32)block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_next));
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadBlock(branch_dl++, 6u, 0u, 0u,
                                            texels, dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_curr));
    }
    if ((flags & 0x20u) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0)
            {
                uls = 0;
            }
            if (ult < 0)
            {
                ult = 0;
            }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        ndsRendererAdapterEmitTileSize(
            branch_dl++, NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult);
    }
    if ((flags & 0x40u) != 0)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        ndsRendererAdapterEmitTileSize(
            branch_dl++, 1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult);
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff)
        {
            s = 0xffff;
        }
        if (t > 0xffff)
        {
            t = 0xffff;
        }
        ndsRendererAdapterEmitTexture(
            branch_dl++, (u32)s, (u32)t, 0u,
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE,
            NDS_RENDERER_ADAPTER_G_ON);
    }

    ndsRendererAdapterEmitEndDL(branch_dl++);
    return branch_dl;
}

static sb32 ndsRendererAdapterPrepareMaterialSegment(
    DObj *dobj, NDSFighterDLDrawState *state)
{
    MObj *mobj;
    Gfx *table;
    Gfx *branch_dl;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_ptr;
    u32 mobj_count = 0u;
    u32 branch_commands = 0u;
    size_t heap_bytes;
    u32 i = 0u;

    if ((dobj == NULL) || (state == NULL) || (dobj->mobj == NULL))
    {
        return FALSE;
    }
    if (ndsRendererAdapterCountMaterialCommands(
            dobj, &mobj_count, &branch_commands) == FALSE)
    {
        return FALSE;
    }
    if ((mobj_count == 0u) ||
        (gSYTaskmanGraphicsHeap.ptr == NULL) ||
        (gSYTaskmanGraphicsHeap.start == NULL) ||
        (gSYTaskmanGraphicsHeap.end == NULL))
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileRecordMaterialOperations(mobj_count);
#endif

    heap_start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    heap_end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    heap_ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    heap_bytes = (size_t)(mobj_count + branch_commands) * sizeof(Gfx);
    if ((heap_ptr < heap_start) || (heap_ptr > heap_end) ||
        (heap_bytes > (size_t)(heap_end - heap_ptr)))
    {
        return FALSE;
    }

    table = (Gfx *)gSYTaskmanGraphicsHeap.ptr;
    branch_dl = table + mobj_count;
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next, i++)
    {
        ndsRendererAdapterEmitBranchTableCommand(&table[i], branch_dl);
        branch_dl = ndsRendererAdapterEmitMaterialCommands(branch_dl, mobj);
    }

    gSYTaskmanGraphicsHeap.ptr = branch_dl;
    state->segment_e_base = table;
    state->segment_e_end = branch_dl;
    return TRUE;
}

static void ndsRendererAdapterSubmitStageDL(DObj *dobj, const Gfx *dl,
                                            GObj *camera_gobj,
                                            u32 initial_geometry_mode)
{
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSRendererStats *render_stats;
    NDSFighterDLDrawState state;
    NDSRendererCommandCallback callback;
    void *callback_user;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
    void *saved_graphics_heap_ptr;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sb32 detailed_output;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 step_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 adapter_start;
    u32 adapter_ticks;
    NDSRendererOwnerStatsSnapshot owner_stats_before;
#endif
    sb32 inherited_texture = FALSE;
    sb32 inherited_tile = FALSE;
    sb32 inherited_segment = FALSE;
#endif

    if ((dobj == NULL) || (dl == NULL))
    {
        return;
    }

    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    detailed_output = (ndsRendererHardwareNoOracleEnabled() == FALSE) ?
        TRUE : FALSE;
    if (detailed_output != FALSE)
    {
        bzero(&state, sizeof(state));
    }
    else
    {
        /* Profile 0/1 submit with a null command callback. Only the compact
         * branch/data resolver context is live; software-preview vertices
         * are already retained by the renderer's persistent vertex cache. */
        state.segment_e_base = NULL;
        state.segment_e_end = NULL;
    }
#else
    bzero(&state, sizeof(state));
#endif
    state.primary_file = loaded;
    state.slot = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        inherited_texture = ndsRendererAdapterStatsHasArmedTexture(
            &sNdsRendererAdapterStagePersistentStats);
        inherited_tile = ndsRendererAdapterStatsHasArmedTile(
            &sNdsRendererAdapterStagePersistentStats);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawSeedPersistentState(
                &state, &sNdsRendererAdapterStagePersistentState);
        }
        else
        {
            state.segment_e_base =
                sNdsRendererAdapterStagePersistentState.segment_e_base;
            state.segment_e_end =
                sNdsRendererAdapterStagePersistentState.segment_e_end;
        }
#else
        ndsFighterDLDrawSeedPersistentState(
            &state, &sNdsRendererAdapterStagePersistentState);
#endif
        inherited_segment = (state.segment_e_base != NULL) ? TRUE : FALSE;
        gNdsStageGCDrawAllLoopHardwareCarrySeedCount++;
        if (inherited_texture != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount++;
        }
        if (inherited_tile != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount++;
        }
        if (inherited_segment != FALSE)
        {
            gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount++;
        }
    }
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    adapter_start = cpuGetTiming();
    step_start = adapter_start;
#elif NDS_RENDERER_PROFILE_LEVEL >= 1
    step_start = cpuGetTiming();
#endif
#endif
    ndsRendererAdapterPrepareMaterialSegment(dobj, &state);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
    step_start = cpuGetTiming();
#endif
    ndsRendererAdapterPrepareInitialMatrices(dobj,
                                             (camera_gobj != NULL) ?
                                                 CObjGetStruct(camera_gobj) :
                                                 ((gGCCurrentCamera != NULL) ?
                                                      CObjGetStruct(
                                                          gGCCurrentCamera) :
                                                      NULL),
                                             TRUE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif

    config.max_depth = 8u;
    config.max_commands = 8192u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsRendererAdapterStageValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;
    callback = (ndsRendererHardwareNoOracleEnabled() != FALSE) ?
        NULL : ndsFighterMarioFoxVisitDLDrawCommand;
    callback_user = &state;

    render_stats = &stats;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        render_stats = &sNdsRendererAdapterStagePersistentStats;
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawResetTransientRendererStats(render_stats);
        }
        else
        {
            ndsFighterDLDrawResetRuntimeRendererStats(render_stats);
        }
    }
    else
    {
        ndsRendererInitStats(render_stats);
    }
#else
    ndsRendererInitStats(render_stats);
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        ndsFighterDLDrawCopyPersistentRendererState(
            render_stats, &sNdsRendererAdapterStagePersistentStats);
    }
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererOwnerSnapshotStats(render_stats, &owner_stats_before);
    step_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileSetSourceProvenance(
        sNdsRendererAdapterStageOwnerOccurrence,
        sNdsRendererAdapterStageListOrdinal,
        ndsRendererOwnerRootBranchPath(
            loaded, dl, sNdsRendererAdapterStageListOrdinal));
    sNdsRendererAdapterStageListOrdinal++;
#endif
#endif
    ndsRendererExecuteDisplayListWithVertexCache(
        dl,
        &config,
        callback,
        callback_user,
        render_stats,
        (sNdsRendererAdapterStagePersistentActive != FALSE) ?
            &sNdsRendererAdapterStageVertexCache : NULL);
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererAdapterAccumulateDepth(
        render_stats,
        &gNdsRendererDepthStageSamples,
        &gNdsRendererDepthStageMin,
        &gNdsRendererDepthStageMax,
        &gNdsRendererDepthStageWMin,
        &gNdsRendererDepthStageWMax);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileDLTicks += cpuGetTiming() - step_start;
    adapter_ticks = cpuGetTiming() - adapter_start;
    gNdsRendererProfileStageAdapterTicks += adapter_ticks;
    ndsRendererOwnerAccumulateList(
        NDS_RENDERER_PROFILE_OWNER_STAGE, loaded, dl,
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].selected_count,
        initial_projection_ptr, initial_modelview_ptr,
        &config,
        &owner_stats_before, render_stats);
#endif
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawCapturePersistentState(
                &sNdsRendererAdapterStagePersistentState, &state);
        }
        else
        {
            sNdsRendererAdapterStagePersistentState.segment_e_base =
                state.segment_e_base;
            sNdsRendererAdapterStagePersistentState.segment_e_end =
                state.segment_e_end;
        }
#else
        ndsFighterDLDrawCapturePersistentState(
            &sNdsRendererAdapterStagePersistentState, &state);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsFighterDLDrawCopyPersistentRendererState(
            &sNdsRendererAdapterStagePersistentStats, render_stats);
#endif
        gNdsStageGCDrawAllLoopHardwareCarryCaptureCount++;
        if (render_stats->command_count <= 5u)
        {
            if (inherited_texture != FALSE)
            {
                gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount++;
            }
            if (inherited_tile != FALSE)
            {
                gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount++;
            }
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#endif
#endif
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        render_stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        render_stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        render_stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        render_stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        render_stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        render_stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        render_stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        render_stats->hardware_texture_reject_count;
    if (render_stats->hardware_texture_ready_count != 0u)
    {
        if (render_stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << render_stats->hardware_texture_format;
        }
        if (render_stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                render_stats->hardware_texture_width;
        }
        if (render_stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                render_stats->hardware_texture_height;
        }
    }
}

void ndsRendererAdapterSubmitStageDObj(void *dobj_ptr, u32 kind,
                                       void *camera_gobj_ptr,
                                       u32 initial_geometry_mode)
{
    DObj *dobj = dobj_ptr;
    GObj *camera_gobj = camera_gobj_ptr;
    DObjDLLink *dl_link;
    u32 i;

    if (ndsRendererAdapterStageDObjDrawable(dobj, kind) == FALSE)
    {
        return;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        if (dobj->dv != NULL)
        {
            ndsRendererAdapterSubmitStageDL(dobj, dobj->dl, camera_gobj,
                                            initial_geometry_mode);
        }
        break;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
        dl_link = dobj->dl_link;
        if (dl_link == NULL)
        {
            return;
        }
        for (i = 0u; i < GC_COMMON_MAX_DLLINKS; i++, dl_link++)
        {
            if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
            {
                break;
            }
            if ((dl_link->list_id >= 0) &&
                ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                (dl_link->dl != NULL))
            {
                ndsRendererAdapterSubmitStageDL(dobj, dl_link->dl,
                                                camera_gobj,
                                                initial_geometry_mode);
            }
        }
        break;

    default:
        break;
    }
}

#else
void ndsRendererAdapterBeginStageTraversal(void)
{
}

void ndsRendererAdapterEndStageTraversal(void)
{
}

void ndsRendererAdapterSubmitStageDObj(void *dobj, u32 kind,
                                       void *camera_gobj,
                                       u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

#endif

static s32 ndsFighterDLDrawAxisCoord(const NDSFighterDLDrawVtx *vtx,
                                     u32 axis, u32 coord)
{
    if (axis == 0u)
    {
        return (coord == 0u) ? vtx->x : vtx->y;
    }
    if (axis == 1u)
    {
        return (coord == 0u) ? vtx->x : vtx->z;
    }
    return (coord == 0u) ? vtx->y : vtx->z;
}

static void ndsFighterDLDrawRecordAxisPoint(
    const NDSFighterDLDrawVtx *vtx, u32 axis, u32 *bounds_valid,
    s32 *min_a, s32 *max_a, s32 *min_b, s32 *max_b)
{
    s32 a;
    s32 b;

    if ((vtx == NULL) || (bounds_valid == NULL) || (min_a == NULL) ||
        (max_a == NULL) || (min_b == NULL) || (max_b == NULL) ||
        (vtx->valid == FALSE))
    {
        return;
    }

    a = ndsFighterDLDrawAxisCoord(vtx, axis, 0u);
    b = ndsFighterDLDrawAxisCoord(vtx, axis, 1u);
    if (*bounds_valid == 0u)
    {
        *min_a = *max_a = a;
        *min_b = *max_b = b;
        *bounds_valid = 1u;
        return;
    }
    if (a < *min_a) { *min_a = a; }
    if (a > *max_a) { *max_a = a; }
    if (b < *min_b) { *min_b = b; }
    if (b > *max_b) { *max_b = b; }
}

static u16 ndsFighterDLDrawRGB15(u8 r, u8 g, u8 b)
{
    return (u16)(0x8000u | ((u16)(r >> 3)) |
                 ((u16)(g >> 3) << 5) | ((u16)(b >> 3) << 10));
}

static s32 ndsFighterDLDrawEdge(s32 ax, s32 ay, s32 bx, s32 by,
                                s32 px, s32 py)
{
    return ((px - ax) * (by - ay)) - ((py - ay) * (bx - ax));
}

static void ndsFighterDLDrawTriangle(u16 *pixels, u32 pitch,
                                     s32 x0, s32 y0,
                                     s32 x1, s32 y1,
                                     s32 x2, s32 y2,
                                     u16 fill, u16 edge,
                                     u32 *pixel_count)
{
    s32 min_x = x0;
    s32 max_x = x0;
    s32 min_y = y0;
    s32 max_y = y0;
    s32 area;
    s32 x;
    s32 y;

    if ((pixels == NULL) || (pixel_count == NULL))
    {
        return;
    }
    if (x1 < min_x) { min_x = x1; }
    if (x2 < min_x) { min_x = x2; }
    if (x1 > max_x) { max_x = x1; }
    if (x2 > max_x) { max_x = x2; }
    if (y1 < min_y) { min_y = y1; }
    if (y2 < min_y) { min_y = y2; }
    if (y1 > max_y) { max_y = y1; }
    if (y2 > max_y) { max_y = y2; }
    if (min_x < 0) { min_x = 0; }
    if (min_y < 0) { min_y = 0; }
    if (max_x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        max_x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (max_y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        max_y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
    if (area == 0)
    {
        return;
    }

    for (y = min_y; y <= max_y; y++)
    {
        for (x = min_x; x <= max_x; x++)
        {
            s32 w0 = ndsFighterDLDrawEdge(x1, y1, x2, y2, x, y);
            s32 w1 = ndsFighterDLDrawEdge(x2, y2, x0, y0, x, y);
            s32 w2 = ndsFighterDLDrawEdge(x0, y0, x1, y1, x, y);

            if (((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) ||
                ((w0 <= 0) && (w1 <= 0) && (w2 <= 0)))
            {
                pixels[(y * (s32)pitch) + x] =
                    ((w0 == 0) || (w1 == 0) || (w2 == 0)) ? edge : fill;
                (*pixel_count)++;
            }
        }
    }
}

static s32 ndsFighterDLDrawMapCoord(s32 value, s32 min_value, s32 max_value,
                                    s32 out_min, s32 out_max)
{
    s32 range = max_value - min_value;
    s32 out_range = out_max - out_min;

    if (range == 0)
    {
        return out_min;
    }
    return out_min + (((value - min_value) * out_range) / range);
}

static u16 ndsFighterDLDrawTriangleColor(
    const NDSFighterDLDrawState *state, const NDSFighterDLDrawTri *tri)
{
    const NDSFighterDLDrawVtx *v0 = &state->vertices[tri->v0];
    const NDSFighterDLDrawVtx *v1 = &state->vertices[tri->v1];
    const NDSFighterDLDrawVtx *v2 = &state->vertices[tri->v2];
    u32 r = ((u32)v0->r + v1->r + v2->r) / 3u;
    u32 g = ((u32)v0->g + v1->g + v2->g) / 3u;
    u32 b = ((u32)v0->b + v1->b + v2->b) / 3u;

    if ((r == 0u) && (g == 0u) && (b == 0u))
    {
        if (state->slot == 0u)
        {
            r = 255u; g = 96u; b = 32u;
        }
        else
        {
            r = 224u; g = 255u; b = 64u;
        }
    }
    return ndsFighterDLDrawRGB15((u8)r, (u8)g, (u8)b);
}

static void ndsFighterDLDrawRasterizeState(NDSFighterDLDrawState *state,
                                           u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (state->slot == 0u) ? 4 : 52;
    s32 box_max_x = (state->slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;

    if ((state == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < state->triangle_count; i++)
        {
            const NDSFighterDLDrawTri *tri = &state->tris[i];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            if ((tri->v0 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (tri->v1 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (tri->v2 < NDS_FIGHTER_DL_DRAW_MAX_VTX) &&
                (state->vertices[tri->v0].valid != FALSE) &&
                (state->vertices[tri->v1].valid != FALSE) &&
                (state->vertices[tri->v2].valid != FALSE))
            {
                v0 = &state->vertices[tri->v0];
                v1 = &state->vertices[tri->v1];
                v2 = &state->vertices[tri->v2];
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < state->triangle_count; i++)
        {
            const NDSFighterDLDrawTri *tri = &state->tris[i];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            s32 area;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &state->vertices[tri->v0];
            v1 = &state->vertices[tri->v1];
            v2 = &state->vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }
            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                axis_min_a, axis_max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                axis_min_b, axis_max_b, box_max_y, box_min_y);

            area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
            if (area == 0)
            {
                continue;
            }
            nondegenerate_count++;
            area_sum += (area < 0) ? (u32)-area : (u32)area;
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < state->triangle_count; i++)
    {
        const NDSFighterDLDrawTri *tri = &state->tris[i];
        const NDSFighterDLDrawVtx *v0;
        const NDSFighterDLDrawVtx *v1;
        const NDSFighterDLDrawVtx *v2;
        s32 x0;
        s32 y0;
        s32 x1;
        s32 y1;
        s32 x2;
        s32 y2;
        u32 before;
        u32 marker_drawn = 0u;
        u16 fill;
        u16 edge;

        if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
            (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
        {
            continue;
        }
        v0 = &state->vertices[tri->v0];
        v1 = &state->vertices[tri->v1];
        v2 = &state->vertices[tri->v2];
        if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
            (v2->valid == FALSE))
        {
            continue;
        }

        x0 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y0 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        x1 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y1 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        x2 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
            min_a, max_a, box_min_x, box_max_x);
        y2 = ndsFighterDLDrawMapCoord(
            ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
            min_b, max_b, box_max_y, box_min_y);
        fill = ndsFighterDLDrawTriangleColor(state, tri);
        edge = ndsFighterDLDrawRGB15(255, 255, 255);
        before = pixel_count;
        if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
        {
            s32 cx = (x0 + x1 + x2) / 3;
            s32 cy = (y0 + y1 + y2) / 3;

            ndsFighterDLDrawTriangle(pixels, pitch,
                                     cx - 4, cy - 3,
                                     cx + 4, cy - 3,
                                     cx, cy + 4,
                                     fill, edge, &pixel_count);
            marker_drawn = 1u;
            x0 = cx - 4;
            y0 = cy - 3;
            x1 = cx + 4;
            y1 = cy - 3;
            x2 = cx;
            y2 = cy + 4;
        }
        else
        {
            ndsFighterDLDrawTriangle(pixels, pitch, x0, y0, x1, y1, x2, y2,
                                     fill, edge, &pixel_count);
        }
        if (pixel_count != before)
        {
            drawn_count++;
            if (marker_drawn != 0u)
            {
                marker_drawn_count++;
            }
            else
            {
                real_drawn_count++;
            }
            if (screen_valid == 0u)
            {
                screen_min_x = screen_max_x = x0;
                screen_min_y = screen_max_y = y0;
                screen_valid = 1u;
            }
            if (x0 < screen_min_x) { screen_min_x = x0; }
            if (x1 < screen_min_x) { screen_min_x = x1; }
            if (x2 < screen_min_x) { screen_min_x = x2; }
            if (x0 > screen_max_x) { screen_max_x = x0; }
            if (x1 > screen_max_x) { screen_max_x = x1; }
            if (x2 > screen_max_x) { screen_max_x = x2; }
            if (y0 < screen_min_y) { screen_min_y = y0; }
            if (y1 < screen_min_y) { screen_min_y = y1; }
            if (y2 < screen_min_y) { screen_min_y = y2; }
            if (y0 > screen_max_y) { screen_max_y = y0; }
            if (y1 > screen_max_y) { screen_max_y = y1; }
            if (y2 > screen_max_y) { screen_max_y = y2; }
        }
    }

    if (state->slot == 0u)
    {
        gNdsFighterDLDrawP0Axis = best_axis;
        gNdsFighterDLDrawP0Area = best_area;
        gNdsFighterDLDrawP0MinA = min_a;
        gNdsFighterDLDrawP0MaxA = max_a;
        gNdsFighterDLDrawP0MinB = min_b;
        gNdsFighterDLDrawP0MaxB = max_b;
        gNdsFighterDLDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLDrawP0PixelCount = pixel_count;
        gNdsFighterDLDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
    }
    else
    {
        gNdsFighterDLDrawP1Axis = best_axis;
        gNdsFighterDLDrawP1Area = best_area;
        gNdsFighterDLDrawP1MinA = min_a;
        gNdsFighterDLDrawP1MaxA = max_a;
        gNdsFighterDLDrawP1MinB = min_b;
        gNdsFighterDLDrawP1MaxB = max_b;
        gNdsFighterDLDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLDrawP1PixelCount = pixel_count;
        gNdsFighterDLDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
    }
}

static void ndsFighterMarioFoxCopyDLDrawStats(
    u32 slot, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats)
{
    if ((state == NULL) || (stats == NULL))
    {
        return;
    }

    if (slot == 0u)
    {
        gNdsFighterDLDrawP0Blocker = stats->blocker;
        gNdsFighterDLDrawP0CommandCount = stats->command_count;
        gNdsFighterDLDrawP0FirstOpcode = stats->first_opcode;
        gNdsFighterDLDrawP0UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLDrawP0UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLDrawP0VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLDrawP0TriangleCount = state->triangle_count;
        gNdsFighterDLDrawP0TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLDrawP0ColorChecksum = state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLDrawP1Blocker = stats->blocker;
        gNdsFighterDLDrawP1CommandCount = stats->command_count;
        gNdsFighterDLDrawP1FirstOpcode = stats->first_opcode;
        gNdsFighterDLDrawP1UnsupportedOpcode =
            (stats->unsupported_opcode != 0u) ? stats->unsupported_opcode :
                state->unsupported_opcode;
        gNdsFighterDLDrawP1UnsupportedCommandCount =
            stats->unsupported_command_count + state->unsupported_command_count;
        gNdsFighterDLDrawP1VertexDecodedCount = state->vertex_decoded_count;
        gNdsFighterDLDrawP1TriangleCount = state->triangle_count;
        gNdsFighterDLDrawP1TriangleValidCount = state->triangle_valid_count;
        gNdsFighterDLDrawP1ColorChecksum = state->color_checksum;
    }
    gNdsFighterDLDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

static void ndsFighterMarioFoxDrawDLForSlot(u32 slot, FTStruct *fp,
                                            u16 *pixels, u32 pitch)
{
    DObj *root;
    DObj *selected;
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSFighterDLDrawState state;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
    void *saved_graphics_heap_ptr;
#endif
    u32 root_x_before;
    u32 root_x_after;
    u32 unused_index;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    selected = ndsFighterFindFirstDObjWithDL(root, &unused_index);
    if (selected == NULL)
    {
        return;
    }

    dl = selected->dl;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        return;
    }

    bzero(&state, sizeof(state));
    state.primary_file = loaded;
    state.slot = slot;
#if NDS_RENDERER_HW_TRIANGLES
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
    ndsRendererAdapterPrepareMaterialSegment(selected, &state);
#endif
    ndsRendererAdapterPrepareInitialMatrices(selected,
                                             (gGCCurrentCamera != NULL) ?
                                                 CObjGetStruct(
                                                     gGCCurrentCamera) :
                                                 NULL,
                                             FALSE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);

    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsFighterDLDrawValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;

    ndsRendererInitStats(&stats);
    ndsRendererExecuteDisplayList(dl,
                                  &config,
                                  ndsFighterMarioFoxVisitDLDrawCommand,
                                  &state,
                                  &stats);
#if NDS_RENDERER_HW_TRIANGLES
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
    ndsFighterMarioFoxCopyDLDrawStats(slot, &state, &stats);
    if ((stats.blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (state.unsupported_command_count == 0u) &&
        (state.vertex_range_reject_count == 0u))
    {
        ndsFighterDLDrawRasterizeState(&state, pixels, pitch);
    }

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLDrawP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLDrawP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLDrawCount++;
}

static void ndsFighterMarioFoxRunDLDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;

    if ((ndsFighterMarioFoxDLDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLDrawResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDLExecResult ==
            NDS_FIGHTER_MARIOFOX_DL_EXEC_PASS) &&
        (gNdsFighterMarioFoxDLExecSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_EXEC_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLExecMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLExecCount == 2u) &&
        (gNdsFighterDLExecP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLExecP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLExecP1UnsupportedCommandCount == 0u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLDrawMask = mask;
        return;
    }

    gNdsFighterDLDrawPreviewCommitBefore = gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(NDS_FIGHTER_DL_DRAW_WIDTH,
                                               NDS_FIGHTER_DL_DRAW_HEIGHT,
                                               &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLDrawPreviewWidth = NDS_FIGHTER_DL_DRAW_WIDTH;
        gNdsFighterDLDrawPreviewHeight = NDS_FIGHTER_DL_DRAW_HEIGHT;
        gNdsFighterDLDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    ndsFighterMarioFoxDrawDLForSlot(0u, &sNdsFighterStructPool[0],
                                    pixels, pitch);
    ndsFighterMarioFoxDrawDLForSlot(1u, &sNdsFighterStructPool[1],
                                    pixels, pitch);
    if (gNdsFighterMarioFoxDLDrawCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLDrawP0VertexDecodedCount > 0u) &&
        (gNdsFighterDLDrawP1VertexDecodedCount > 0u) &&
        (gNdsFighterDLDrawP0TriangleCount > 0u) &&
        (gNdsFighterDLDrawP1TriangleCount > 0u) &&
        (gNdsFighterDLDrawP0TriangleValidCount > 0u) &&
        (gNdsFighterDLDrawP1TriangleValidCount > 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLDrawP0Axis <= 2u) &&
        (gNdsFighterDLDrawP1Axis <= 2u) &&
        (gNdsFighterDLDrawP0Area > 0u) &&
        (gNdsFighterDLDrawP1Area > 0u) &&
        ((gNdsFighterDLDrawP0MaxA != gNdsFighterDLDrawP0MinA) ||
         (gNdsFighterDLDrawP0MaxB != gNdsFighterDLDrawP0MinB)) &&
        ((gNdsFighterDLDrawP1MaxA != gNdsFighterDLDrawP1MinA) ||
         (gNdsFighterDLDrawP1MaxB != gNdsFighterDLDrawP1MinB)))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLDrawP0RealTriangleDrawnCount > 0u) &&
        (gNdsFighterDLDrawP1RealTriangleDrawnCount > 0u))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLDrawTotalPixelCount =
        gNdsFighterDLDrawP0PixelCount + gNdsFighterDLDrawP1PixelCount;
    if ((gNdsFighterDLDrawP0PixelCount > 0u) &&
        (gNdsFighterDLDrawP1PixelCount > 0u) &&
        (gNdsFighterDLDrawTotalPixelCount > 0u) &&
        (gNdsFighterDLDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLDrawPreviewCommitDelta =
            gNdsFighterDLDrawPreviewCommitAfter -
            gNdsFighterDLDrawPreviewCommitBefore;
        gNdsFighterDLDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLDrawPreviewReady != 0u) &&
        (gNdsFighterDLDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLDrawP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLDrawP0StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLDrawP1StatusAfter == (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLDrawP0MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLDrawP1MotionAfter == (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLDrawP0RootXBeforeBits ==
            gNdsFighterDLDrawP0RootXAfterBits) &&
        (gNdsFighterDLDrawP1RootXBeforeBits ==
            gNdsFighterDLDrawP1RootXAfterBits) &&
        (gNdsFighterDLDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLDrawDrawCallCount == 0u) &&
        (gNdsFighterDLDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLDrawMask = mask;
    gNdsFighterMarioFoxDLDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_DRAW_PASS;
        gNdsFighterMarioFoxDLDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_DRAW_SAFE_PASS;
    }
}

typedef struct NDSFighterDLMultiDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLMultiDrawCollection;

static void ndsFighterCollectDObjsWithDLRecursive(
    DObj *dobj, NDSFighterDLMultiDrawCollection *collection,
    u32 *traversal_index)
{
    while (dobj != NULL)
    {
        u32 current_index = (traversal_index != NULL) ? *traversal_index : 0u;

        if (traversal_index != NULL)
        {
            (*traversal_index)++;
        }

        if ((collection != NULL) && (dobj->dl != NULL))
        {
            if (collection->selected_count <
                NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED)
            {
                u32 selected = collection->selected_count;
                collection->dobjs[selected] = dobj;
                collection->indices[selected] = current_index;
                collection->selected_count++;
                if (current_index < 32u)
                {
                    collection->selected_index_mask |= 1u << current_index;
                }
            }
            collection->total_count++;
        }

        ndsFighterCollectDObjsWithDLRecursive(dobj->child,
                                              collection,
                                              traversal_index);
        dobj = dobj->sib_next;
    }
}

static void ndsFighterCollectDObjsWithDL(
    DObj *root, NDSFighterDLMultiDrawCollection *collection)
{
    u32 traversal_index = 0u;

    if (collection == NULL)
    {
        return;
    }
    bzero(collection, sizeof(*collection));
    ndsFighterCollectDObjsWithDLRecursive(root, collection,
                                          &traversal_index);
}

static s32 ndsFighterDLMultiDrawValidateRange(const Gfx *dl, size_t bytes,
                                              void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLMultiDrawRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static const Gfx *ndsFighterDLMultiDrawResolveBranch(const Gfx *dl,
                                                     u32 *resolve_kind,
                                                     void *user)
{
    return ndsFighterDLDrawResolveBranch(dl, resolve_kind, user);
}

static void ndsFighterDLMultiDrawRecordScreenPoint(
    u32 slot, s32 x, s32 y, u32 *screen_valid,
    s32 *screen_min_x, s32 *screen_max_x,
    s32 *screen_min_y, s32 *screen_max_y)
{
    if ((screen_valid == NULL) || (screen_min_x == NULL) ||
        (screen_max_x == NULL) || (screen_min_y == NULL) ||
        (screen_max_y == NULL))
    {
        return;
    }

    (void)slot;
    if (x < 0) { x = 0; }
    if (x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (y < 0) { y = 0; }
    if (y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    if (*screen_valid == 0u)
    {
        *screen_min_x = *screen_max_x = x;
        *screen_min_y = *screen_max_y = y;
        *screen_valid = 1u;
        return;
    }
    if (x < *screen_min_x) { *screen_min_x = x; }
    if (x > *screen_max_x) { *screen_max_x = x; }
    if (y < *screen_min_y) { *screen_min_y = y; }
    if (y > *screen_max_y) { *screen_max_y = y; }
}

static void ndsFighterDLMultiDrawRasterizeStates(
    u32 slot, NDSFighterDLDrawState *states, const u8 *clean,
    u32 selected_count, u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (slot == 0u) ? 4 : 52;
    s32 box_max_x = (slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;
    u32 drawn_dobj_count = 0u;

    if ((states == NULL) || (clean == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;
                s32 x0;
                s32 y0;
                s32 x1;
                s32 y1;
                s32 x2;
                s32 y2;
                s32 area;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                x0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);

                area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
                if (area == 0)
                {
                    continue;
                }
                nondegenerate_count++;
                area_sum += (area < 0) ? (u32)-area : (u32)area;
            }
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < selected_count; i++)
    {
        u32 tri_index;
        u32 state_drawn = 0u;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            u32 before;
            u32 marker_drawn = 0u;
            u16 fill;
            u16 edge;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &states[i].vertices[tri->v0];
            v1 = &states[i].vertices[tri->v1];
            v2 = &states[i].vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }

            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
            edge = ndsFighterDLDrawRGB15(255, 255, 255);
            before = pixel_count;
            if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
            {
                s32 cx = (x0 + x1 + x2) / 3;
                s32 cy = (y0 + y1 + y2) / 3;

                ndsFighterDLDrawTriangle(pixels, pitch,
                                         cx - 4, cy - 3,
                                         cx + 4, cy - 3,
                                         cx, cy + 4,
                                         fill, edge, &pixel_count);
                marker_drawn = 1u;
                x0 = cx - 4;
                y0 = cy - 3;
                x1 = cx + 4;
                y1 = cy - 3;
                x2 = cx;
                y2 = cy + 4;
            }
            else
            {
                ndsFighterDLDrawTriangle(pixels, pitch,
                                         x0, y0, x1, y1, x2, y2,
                                         fill, edge, &pixel_count);
            }
            if (pixel_count != before)
            {
                drawn_count++;
                if (marker_drawn != 0u)
                {
                    marker_drawn_count++;
                }
                else
                {
                    real_drawn_count++;
                }
                state_drawn = 1u;
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x0, y0, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x1, y1, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
                ndsFighterDLMultiDrawRecordScreenPoint(
                    slot, x2, y2, &screen_valid, &screen_min_x,
                    &screen_max_x, &screen_min_y, &screen_max_y);
            }
        }
        if (state_drawn != 0u)
        {
            drawn_dobj_count++;
        }
    }

    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0Axis = best_axis;
        gNdsFighterDLMultiDrawP0Area = best_area;
        gNdsFighterDLMultiDrawP0MinA = min_a;
        gNdsFighterDLMultiDrawP0MaxA = max_a;
        gNdsFighterDLMultiDrawP0MinB = min_b;
        gNdsFighterDLMultiDrawP0MaxB = max_b;
        gNdsFighterDLMultiDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLMultiDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLMultiDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLMultiDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLMultiDrawP0PixelCount = pixel_count;
        gNdsFighterDLMultiDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLMultiDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLMultiDrawP0DrawnDObjCount = drawn_dobj_count;
    }
    else
    {
        gNdsFighterDLMultiDrawP1Axis = best_axis;
        gNdsFighterDLMultiDrawP1Area = best_area;
        gNdsFighterDLMultiDrawP1MinA = min_a;
        gNdsFighterDLMultiDrawP1MaxA = max_a;
        gNdsFighterDLMultiDrawP1MinB = min_b;
        gNdsFighterDLMultiDrawP1MaxB = max_b;
        gNdsFighterDLMultiDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLMultiDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLMultiDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLMultiDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLMultiDrawP1PixelCount = pixel_count;
        gNdsFighterDLMultiDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLMultiDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLMultiDrawP1DrawnDObjCount = drawn_dobj_count;
    }
}

static void ndsFighterDLMultiDrawAccumulateStats(
    u32 slot, u32 selected_index, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats, u8 *clean)
{
    u32 blocker = (stats != NULL) ? stats->blocker : 0xffffffffu;
    u32 unsupported_opcode = 0u;
    u32 unsupported_count = 0u;
    u32 clean_selected;

    if ((state == NULL) || (stats == NULL) || (clean == NULL))
    {
        return;
    }

    unsupported_opcode = (stats->unsupported_opcode != 0u) ?
        stats->unsupported_opcode : state->unsupported_opcode;
    unsupported_count = stats->unsupported_command_count +
        state->unsupported_command_count;
    clean_selected =
        (blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (unsupported_opcode == 0u) &&
        (unsupported_count == 0u) &&
        (state->vertex_range_reject_count == 0u) &&
        (state->vertex_decoded_count != 0u) &&
        (state->triangle_valid_count != 0u);
    clean[selected_index] = (u8)clean_selected;

    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLMultiDrawP0CleanCount++;
        }
        else
        {
            gNdsFighterDLMultiDrawP0FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLMultiDrawP0FirstBlocker == 0u))
        {
            gNdsFighterDLMultiDrawP0FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLMultiDrawP0BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLMultiDrawP0CommandCount += stats->command_count;
        if (gNdsFighterDLMultiDrawP0FirstOpcode == 0u)
        {
            gNdsFighterDLMultiDrawP0FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLMultiDrawP0UnsupportedOpcode == 0u))
        {
            gNdsFighterDLMultiDrawP0UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLMultiDrawP0UnsupportedCommandCount +=
            unsupported_count;
        gNdsFighterDLMultiDrawP0VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLMultiDrawP0TriangleCount += state->triangle_count;
        gNdsFighterDLMultiDrawP0TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLMultiDrawP0ColorChecksum =
            (gNdsFighterDLMultiDrawP0ColorChecksum * 33u) ^
            state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLMultiDrawP1AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLMultiDrawP1CleanCount++;
        }
        else
        {
            gNdsFighterDLMultiDrawP1FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLMultiDrawP1FirstBlocker == 0u))
        {
            gNdsFighterDLMultiDrawP1FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLMultiDrawP1BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLMultiDrawP1CommandCount += stats->command_count;
        if (gNdsFighterDLMultiDrawP1FirstOpcode == 0u)
        {
            gNdsFighterDLMultiDrawP1FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLMultiDrawP1UnsupportedOpcode == 0u))
        {
            gNdsFighterDLMultiDrawP1UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLMultiDrawP1UnsupportedCommandCount +=
            unsupported_count;
        gNdsFighterDLMultiDrawP1VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLMultiDrawP1TriangleCount += state->triangle_count;
        gNdsFighterDLMultiDrawP1TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLMultiDrawP1ColorChecksum =
            (gNdsFighterDLMultiDrawP1ColorChecksum * 33u) ^
            state->color_checksum;
    }

    gNdsFighterDLMultiDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

static void ndsFighterMarioFoxDLMultiDrawForSlot(u32 slot, FTStruct *fp,
                                                 u16 *pixels, u32 pitch)
{
    DObj *root;
    NDSFighterDLMultiDrawCollection collection;
    NDSFighterDLDrawState states[
        NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    NDSFighterDLDrawState persistent_state;
    NDSRendererStats stats;
    NDSRendererStats persistent_stats;
    u8 clean[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 root_x_before;
    u32 root_x_after;
    u32 i;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;

    ndsFighterCollectDObjsWithDL(root, &collection);
    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0CandidateCount = collection.total_count;
        gNdsFighterDLMultiDrawP0SelectedCount = collection.selected_count;
        gNdsFighterDLMultiDrawP0SelectedIndexMask =
            collection.selected_index_mask;
    }
    else
    {
        gNdsFighterDLMultiDrawP1CandidateCount = collection.total_count;
        gNdsFighterDLMultiDrawP1SelectedCount = collection.selected_count;
        gNdsFighterDLMultiDrawP1SelectedIndexMask =
            collection.selected_index_mask;
    }

    bzero(states, sizeof(states));
    bzero(&persistent_state, sizeof(persistent_state));
    bzero(&stats, sizeof(stats));
    ndsRendererInitStats(&persistent_stats);
    bzero(clean, sizeof(clean));

    for (i = 0u; i < collection.selected_count; i++)
    {
        const Gfx *dl = collection.dobjs[i]->dl;
        NDSRelocLoadedFile *loaded =
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
        NDSRendererConfig config;
        NDSRendererMatrix20p12 initial_projection;
        NDSRendererMatrix20p12 initial_modelview;
        const NDSRendererMatrix20p12 *initial_projection_ptr;
        const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
        void *saved_graphics_heap_ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        u32 step_start;
#endif
#endif

        if ((loaded == NULL) &&
            (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
        {
            continue;
        }

        states[i].primary_file = loaded;
        states[i].slot = slot;
        ndsFighterDLDrawSeedPersistentState(&states[i],
                                            &persistent_state);
#if NDS_RENDERER_HW_TRIANGLES
        saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        step_start = cpuGetTiming();
#endif
        ndsRendererAdapterPrepareMaterialSegment(collection.dobjs[i],
                                                 &states[i]);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
        step_start = cpuGetTiming();
#endif
#endif
        ndsRendererAdapterPrepareInitialMatrices(collection.dobjs[i],
                                                 (gGCCurrentCamera != NULL) ?
                                                     CObjGetStruct(
                                                         gGCCurrentCamera) :
                                                     NULL,
                                                 FALSE,
                                                 &initial_projection,
                                                 &initial_projection_ptr,
                                                 &initial_modelview,
                                                 &initial_modelview_ptr);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif
        config.max_depth = 8u;
        config.max_commands = 2048u;
        config.max_list_commands = 512u;
        config.initial_projection = initial_projection_ptr;
        config.initial_modelview = initial_modelview_ptr;
        config.initial_geometry_mode = 0u;
        config.texture_data_layout =
            NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
        config.validate_range = ndsFighterDLMultiDrawValidateRange;
        config.immutable_command_span =
            ndsRendererAdapterImmutableCommandSpan;
        config.resolve_branch = ndsFighterDLMultiDrawResolveBranch;
        config.resolve_data = ndsFighterDLDrawResolveRendererData;
        config.user = &states[i];

        ndsRendererInitStats(&stats);
        ndsFighterDLDrawCopyPersistentRendererState(&stats,
                                                    &persistent_stats);
        ndsRendererExecuteDisplayList(dl,
                                      &config,
                                      ndsFighterMarioFoxVisitDLDrawCommand,
                                      &states[i],
                                      &stats);
#if NDS_RENDERER_HW_TRIANGLES
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
        ndsFighterDLDrawCapturePersistentState(&persistent_state,
                                               &states[i]);
        ndsFighterDLDrawCopyPersistentRendererState(&persistent_stats,
                                                    &stats);
        ndsFighterDLMultiDrawAccumulateStats(slot, i, &states[i],
                                             &stats, clean);
    }

    ndsFighterDLMultiDrawRasterizeStates(slot, states, clean,
                                         collection.selected_count,
                                         pixels,
                                         pitch);

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLMultiDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLMultiDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLMultiDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLMultiDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLMultiDrawP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLMultiDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLMultiDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLMultiDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLMultiDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLMultiDrawP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLMultiDrawCount++;
}

static void ndsFighterMarioFoxRunDLMultiDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;

    if ((ndsFighterMarioFoxDLMultiDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLMultiDrawResult != 0u))
    {
        return;
    }

    if (
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
        (gNdsFighterManagerResult == NDS_FIGHTER_MANAGER_PASS) &&
        ((gNdsFighterManagerWaitMask & 0x3u) == 0x3u)
#else
        (gNdsFighterMarioFoxDLDrawResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_PASS) &&
        (gNdsFighterMarioFoxDLDrawSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLDrawMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLDrawCount == 2u) &&
        (gNdsFighterDLDrawP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedCommandCount == 0u)
#endif
        )
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLMultiDrawMask = mask;
        return;
    }

    gNdsFighterDLMultiDrawPreviewCommitBefore =
        gNdsOriginalDLPreviewCommitCount;
    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_DL_MULTI_DRAW_WIDTH,
        NDS_FIGHTER_DL_MULTI_DRAW_HEIGHT,
        &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLMultiDrawPreviewWidth =
            NDS_FIGHTER_DL_MULTI_DRAW_WIDTH;
        gNdsFighterDLMultiDrawPreviewHeight =
            NDS_FIGHTER_DL_MULTI_DRAW_HEIGHT;
        gNdsFighterDLMultiDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLMultiDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    ndsFighterMarioFoxDLMultiDrawForSlot(
        0u, ndsFighterMarioFoxProofStructForSlot(0u), pixels, pitch);
    ndsFighterMarioFoxDLMultiDrawForSlot(
        1u, ndsFighterMarioFoxProofStructForSlot(1u), pixels, pitch);
    if (gNdsFighterMarioFoxDLMultiDrawCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLMultiDrawP0CandidateCount == 14u) &&
        (gNdsFighterDLMultiDrawP1CandidateCount == 18u) &&
        (gNdsFighterDLMultiDrawP0SelectedCount ==
            NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED) &&
        (gNdsFighterDLMultiDrawP1SelectedCount ==
            NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED) &&
        (gNdsFighterDLMultiDrawP0SelectedIndexMask != 0u) &&
        (gNdsFighterDLMultiDrawP1SelectedIndexMask != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLMultiDrawP0AttemptCount == 4u) &&
        (gNdsFighterDLMultiDrawP1AttemptCount == 4u) &&
        (gNdsFighterDLMultiDrawP0CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP1CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP0FailedCount == 0u) &&
        (gNdsFighterDLMultiDrawP1FailedCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLMultiDrawP0VertexDecodedCount >
            gNdsFighterDLDrawP0VertexDecodedCount) &&
        (gNdsFighterDLMultiDrawP1VertexDecodedCount >
            gNdsFighterDLDrawP1VertexDecodedCount) &&
        (gNdsFighterDLMultiDrawP0TriangleCount >
            gNdsFighterDLDrawP0TriangleCount) &&
        (gNdsFighterDLMultiDrawP1TriangleCount >
            gNdsFighterDLDrawP1TriangleCount) &&
        (gNdsFighterDLMultiDrawP0TriangleValidCount >
            gNdsFighterDLDrawP0TriangleValidCount) &&
        (gNdsFighterDLMultiDrawP1TriangleValidCount >
            gNdsFighterDLDrawP1TriangleValidCount) &&
        (gNdsFighterDLMultiDrawP0RealTriangleDrawnCount >
            gNdsFighterDLDrawP0RealTriangleDrawnCount) &&
        (gNdsFighterDLMultiDrawP1RealTriangleDrawnCount >
            gNdsFighterDLDrawP1RealTriangleDrawnCount))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLMultiDrawTotalPixelCount =
        gNdsFighterDLMultiDrawP0PixelCount +
        gNdsFighterDLMultiDrawP1PixelCount;
    if ((gNdsFighterDLMultiDrawP0PixelCount >=
            gNdsFighterDLDrawP0PixelCount) &&
        (gNdsFighterDLMultiDrawP1PixelCount >=
            gNdsFighterDLDrawP1PixelCount) &&
        (gNdsFighterDLMultiDrawTotalPixelCount >=
            gNdsFighterDLDrawTotalPixelCount) &&
        (gNdsFighterDLMultiDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLMultiDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLMultiDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLMultiDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLMultiDrawPreviewCommitDelta =
            gNdsFighterDLMultiDrawPreviewCommitAfter -
            gNdsFighterDLMultiDrawPreviewCommitBefore;
        gNdsFighterDLMultiDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLMultiDrawPreviewReady != 0u) &&
        (gNdsFighterDLMultiDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLMultiDrawP0FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLMultiDrawP1FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLMultiDrawP0BlockerMask == 0u) &&
        (gNdsFighterDLMultiDrawP1BlockerMask == 0u) &&
        (gNdsFighterDLMultiDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLMultiDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLMultiDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLMultiDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLMultiDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLMultiDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLMultiDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLMultiDrawP0StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLMultiDrawP1StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLMultiDrawP0MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLMultiDrawP1MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLMultiDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLMultiDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLMultiDrawP0RootXBeforeBits ==
            gNdsFighterDLMultiDrawP0RootXAfterBits) &&
        (gNdsFighterDLMultiDrawP1RootXBeforeBits ==
            gNdsFighterDLMultiDrawP1RootXAfterBits) &&
        (gNdsFighterDLMultiDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLMultiDrawDrawCallCount == 0u) &&
        (gNdsFighterDLMultiDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLMultiDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLMultiDrawMask = mask;
    gNdsFighterMarioFoxDLMultiDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLMultiDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_PASS;
        gNdsFighterMarioFoxDLMultiDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_SAFE_PASS;
    }
}

typedef struct NDSFighterDLAllDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLAllDrawCollection;

#define NDS_FIGHTER_DL_ALL_FAIL_BLOCKER 0x1u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_OPCODE 0x2u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_COUNT 0x4u
#define NDS_FIGHTER_DL_ALL_FAIL_VERTEX_RANGE 0x8u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VERTS 0x10u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VALID_TRIS 0x20u
#define NDS_FIGHTER_DL_ALL_FAIL_UNKNOWN 0x80000000u

static void ndsFighterCollectAllDObjsWithDLRecursive(
    DObj *dobj, NDSFighterDLAllDrawCollection *collection,
    u32 *traversal_index)
{
    while (dobj != NULL)
    {
        u32 current_index = (traversal_index != NULL) ? *traversal_index : 0u;

        if (traversal_index != NULL)
        {
            (*traversal_index)++;
        }

        if ((collection != NULL) && (dobj->dl != NULL))
        {
            if (collection->selected_count <
                NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED)
            {
                u32 selected = collection->selected_count;
                collection->dobjs[selected] = dobj;
                collection->indices[selected] = current_index;
                collection->selected_count++;
                if (current_index < 32u)
                {
                    collection->selected_index_mask |= 1u << current_index;
                }
            }
            collection->total_count++;
        }

        ndsFighterCollectAllDObjsWithDLRecursive(dobj->child,
                                                 collection,
                                                 traversal_index);
        dobj = dobj->sib_next;
    }
}

typedef struct NDSFighterDisplayContractEvent {
    DObj *dobj;
    DObj *matrix_dobj;
    DObj *material_dobj;
    const Gfx *dl;
    u32 geometry_mode;
    u32 cycle_type;
    u32 render_mode;
    u32 prim_color;
    u32 env_color;
    Light light;
    u32 light_valid;
} NDSFighterDisplayContractEvent;

#define NDS_FIGHTER_DISPLAY_CYCLETYPE_MASK (3u << 20)
#define NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK (1u << 0)
#define NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK (1u << 1)

typedef struct NDSFighterDisplayContract {
    NDSFighterDisplayContractEvent events[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    Gfx scratch[4][128];
    Gfx *saved_heads[4];
    void *saved_graphics_heap_ptr;
    DObj *current_dobj;
    DObj *material_dobj;
    s32 pending_event;
    u32 event_count;
    u32 geometry_mode;
    u32 cycle_type;
    u32 render_mode;
    u32 prim_color;
    u32 env_color;
    u32 light_count;
    Light light;
    u32 light_valid;
    u32 active;
    u32 matrix_ready;
    u32 material_ready;
} NDSFighterDisplayContract;

static NDSFighterDisplayContract sNdsFighterDisplayContract;
static Light sNdsFighterDisplayCurrentLight;
static u32 sNdsFighterDisplayCurrentLightCount;
static u32 sNdsFighterDisplayCurrentLightValid;
static sb32 sNdsFighterDisplayContractPlayback;
static u32 sNdsFighterDisplayContractLastFrame[2] = {
    0xffffffffu, 0xffffffffu
};

static u32 ndsFighterDisplayContractPackColor(u8 r, u8 g, u8 b, u8 a)
{
    return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
}

static void ndsFighterDisplayContractCountFlags(DObj *dobj)
{
    while (dobj != NULL)
    {
        if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
        {
            gNdsFighterDisplayContractHiddenCount++;
        }
        if ((dobj->flags & DOBJ_FLAG_NOTEXTURE) != 0)
        {
            gNdsFighterDisplayContractNoTextureCount++;
        }
        ndsFighterDisplayContractCountFlags(dobj->child);
        dobj = dobj->sib_next;
    }
}

void ndsFighterDisplayContractSetGeometryMode(u32 clear_mask, u32 set_mask)
{
    if (sNdsFighterDisplayContract.active == 0u)
    {
        return;
    }
    sNdsFighterDisplayContract.geometry_mode &= ~clear_mask;
    sNdsFighterDisplayContract.geometry_mode |= set_mask;
    gNdsFighterDisplayContractGeometryMode =
        sNdsFighterDisplayContract.geometry_mode;
}

void ndsFighterDisplayContractSetCycleType(u32 cycle_type)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.cycle_type = cycle_type;
    }
}

void ndsFighterDisplayContractSetRenderMode(u32 mode1, u32 mode2)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.render_mode = mode1 | mode2;
    }
}

void ndsFighterDisplayContractSetEnvColor(u8 r, u8 g, u8 b, u8 a)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.env_color =
            ndsFighterDisplayContractPackColor(r, g, b, a);
    }
}

void ndsFighterDisplayContractSetPrimColor(u8 r, u8 g, u8 b, u8 a)
{
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.prim_color =
            ndsFighterDisplayContractPackColor(r, g, b, a);
    }
}

void ndsFighterDisplayContractSetLightCount(u32 count)
{
    sNdsFighterDisplayCurrentLightCount = count;
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.light_count = count;
        gNdsFighterDisplayContractLightCount += count;
    }
}

void ndsFighterDisplayContractSetLight(const Light *light, u32 slot)
{
    if ((light == NULL) || (slot != 1u))
    {
        return;
    }
    sNdsFighterDisplayCurrentLight = *light;
    sNdsFighterDisplayCurrentLightValid = TRUE;
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.light = *light;
        sNdsFighterDisplayContract.light_valid = TRUE;
        gNdsFighterDisplayContractLightDirectionCount++;
    }
}

void ndsFighterDisplayContractResetSceneLight(void)
{
    sNdsFighterDisplayCurrentLightCount = 0u;
    sNdsFighterDisplayCurrentLightValid = FALSE;
}

u8 ndsFighterDisplayContractSetStageEnvColor(Gfx **dls)
{
    (void)dls;
    /* mpCollisionInitGroundData sets this source color to opaque white. */
    ndsFighterDisplayContractSetEnvColor(0xffu, 0xffu, 0xffu, 0xffu);
    return 0xffu;
}

sb32 ndsFighterDisplayContractCheckTargetInBounds(f32 pos_x, f32 pos_y)
{
    extern sb32 gmCameraCheckTargetInBounds(f32 pos_x, f32 pos_y);
    sb32 is_in_bounds;

    gNdsFighterDisplayContractBoundsXBits = ndsFloatBits(pos_x);
    gNdsFighterDisplayContractBoundsYBits = ndsFloatBits(pos_y);
    is_in_bounds = gmCameraCheckTargetInBounds(pos_x, pos_y);
    if (is_in_bounds != FALSE)
    {
        gNdsFighterDisplayContractBoundsPassCount++;
    }
    else
    {
        gNdsFighterDisplayContractBoundsFailCount++;
    }
    return is_in_bounds;
}

void ndsFighterDisplayContractProjectTarget(CObj *cobj,
                                            Mtx44f matrix,
                                            Vec3f *pos,
                                            f32 *dist_x,
                                            f32 *dist_y)
{
    f32 x;
    f32 y;
    f32 z;
    f32 projected_x;
    f32 projected_y;
    f32 scale;

    if ((cobj == NULL) || (pos == NULL) || (dist_x == NULL) ||
        (dist_y == NULL))
    {
        return;
    }
    /* BattleShip ftparam.c:2421-2439, used by fighter magnify culling. */
    x = pos->x;
    y = pos->y;
    z = pos->z;
    projected_x = ((matrix[0][0] * x) + (matrix[1][0] * y) +
                   (matrix[2][0] * z)) + matrix[3][0];
    projected_y = ((matrix[0][1] * x) + (matrix[1][1] * y) +
                   (matrix[2][1] * z)) + matrix[3][1];
    scale = ((matrix[0][3] * x) + (matrix[1][3] * y) +
             (matrix[2][3] * z)) + matrix[3][3];
    if (ABSF(scale) < 0.1F)
    {
        scale = (scale < 0.0F) ? -0.1F : 0.1F;
    }
    scale = 1.0F / scale;
    *dist_x = (cobj->viewport.vp.vscale[0] / 4) *
              (projected_x * scale);
    *dist_y = (cobj->viewport.vp.vscale[1] / 4) *
              (projected_y * scale);
}

void ndsFighterDisplayContractSelectDL(const Gfx *dl)
{
    NDSFighterDisplayContractEvent *event;

    if ((sNdsFighterDisplayContract.active == 0u) || (dl == NULL) ||
        (sNdsFighterDisplayContract.event_count >=
            NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return;
    }
    event = &sNdsFighterDisplayContract.events[
        sNdsFighterDisplayContract.event_count++];
    event->dl = dl;
    event->dobj = (sNdsFighterDisplayContract.matrix_ready != 0u) ?
        sNdsFighterDisplayContract.current_dobj : NULL;
    event->matrix_dobj = event->dobj;
    event->material_dobj =
        (sNdsFighterDisplayContract.material_ready != 0u) ?
            sNdsFighterDisplayContract.material_dobj : NULL;
    event->geometry_mode = sNdsFighterDisplayContract.geometry_mode;
    event->cycle_type = sNdsFighterDisplayContract.cycle_type;
    event->render_mode = sNdsFighterDisplayContract.render_mode;
    event->prim_color = sNdsFighterDisplayContract.prim_color;
    event->env_color = sNdsFighterDisplayContract.env_color;
    event->light = sNdsFighterDisplayContract.light;
    event->light_valid = sNdsFighterDisplayContract.light_valid;
    if (gNdsFighterDisplayContractSelectedCount == 0u)
    {
        /* ftdisplaymain.c:1176-1178 establishes the normal fighter preamble. */
        gNdsFighterDisplayContractCycleType = event->cycle_type;
        gNdsFighterDisplayContractRenderMode = event->render_mode;
    }
    if (event->dobj == NULL)
    {
        sNdsFighterDisplayContract.pending_event =
            (s32)sNdsFighterDisplayContract.event_count - 1;
    }
    sNdsFighterDisplayContract.matrix_ready = FALSE;
    sNdsFighterDisplayContract.material_ready = FALSE;
    gNdsFighterDisplayContractSelectedCount++;
}

s32 gcPrepDObjMatrix(Gfx **dls, DObj *dobj)
{
    (void)dls;
    if (sNdsFighterDisplayContract.active == 0u)
    {
        return FALSE;
    }
    if ((sNdsFighterDisplayContract.pending_event >= 0) &&
        ((u32)sNdsFighterDisplayContract.pending_event <
            sNdsFighterDisplayContract.event_count))
    {
        NDSFighterDisplayContractEvent *event =
            &sNdsFighterDisplayContract.events[
                sNdsFighterDisplayContract.pending_event];

        event->dobj = dobj;
        event->matrix_dobj =
            (dobj->parent != DOBJ_PARENT_NULL) ? dobj->parent : NULL;
        sNdsFighterDisplayContract.pending_event = -1;
    }
    sNdsFighterDisplayContract.current_dobj = dobj;
    sNdsFighterDisplayContract.matrix_ready = TRUE;
    return FALSE;
}

void gcDrawMObjForDObj(DObj *dobj, Gfx **dls)
{
    (void)dls;
    if (sNdsFighterDisplayContract.active != 0u)
    {
        sNdsFighterDisplayContract.material_dobj = dobj;
        sNdsFighterDisplayContract.material_ready = TRUE;
    }
}

static void ndsFighterDisplayContractCapture(GObj *fighter_gobj)
{
    extern void ndsBaseFTDisplayMainProcDisplay(GObj *fighter_gobj);
    extern sb32 gmCameraLookAtFuncMatrix(Mtx *mtx, CObj *cobj, Gfx **dls);
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Mtx camera_mtx;
    u32 i;

    bzero(&sNdsFighterDisplayContract, sizeof(sNdsFighterDisplayContract));
    sNdsFighterDisplayContract.pending_event = -1;
    sNdsFighterDisplayContract.prim_color = 0xffffffffu;
    sNdsFighterDisplayContract.env_color = 0xffffffffu;
    sNdsFighterDisplayContract.light_count =
        sNdsFighterDisplayCurrentLightCount;
    if (sNdsFighterDisplayCurrentLightValid != 0u)
    {
        sNdsFighterDisplayContract.light = sNdsFighterDisplayCurrentLight;
        sNdsFighterDisplayContract.light_valid = TRUE;
    }
    sNdsFighterDisplayContract.saved_graphics_heap_ptr =
        gSYTaskmanGraphicsHeap.ptr;
    for (i = 0u; i < 4u; i++)
    {
        sNdsFighterDisplayContract.saved_heads[i] = gSYTaskmanDLHeads[i];
        gSYTaskmanDLHeads[i] = sNdsFighterDisplayContract.scratch[i];
    }
    sNdsFighterDisplayContract.active = TRUE;
    /* ftdisplaymain.c:1093-1129 only needs the battle visibility matrix for
     * player/CPU/game-key fighters. Results fighters are Demo fighters. */
    if ((fp != NULL) &&
        ((fp->pkind == nFTPlayerKindMan) ||
         (fp->pkind == nFTPlayerKindCom) ||
         (fp->pkind == nFTPlayerKindGameKey)) &&
        (gGMCameraGObj != NULL) &&
        (CObjGetStruct(gGMCameraGObj) != NULL))
    {
        /* BattleShip gmcamera.c:985-1015 prepares the visibility matrix. */
        gmCameraLookAtFuncMatrix(&camera_mtx,
                                 CObjGetStruct(gGMCameraGObj),
                                 gSYTaskmanDLHeads);
    }
    ndsFighterDisplayContractCountFlags(DObjGetStruct(fighter_gobj));
    ndsBaseFTDisplayMainProcDisplay(fighter_gobj);
    sNdsFighterDisplayContract.active = FALSE;
    for (i = 0u; i < 4u; i++)
    {
        gSYTaskmanDLHeads[i] = sNdsFighterDisplayContract.saved_heads[i];
    }
    gSYTaskmanGraphicsHeap.ptr =
        sNdsFighterDisplayContract.saved_graphics_heap_ptr;
}

static void ndsFighterCollectAllDObjsWithDL(
    DObj *root, NDSFighterDLAllDrawCollection *collection)
{
    u32 traversal_index = 0u;
    u32 i;

    if (collection == NULL)
    {
        return;
    }
    bzero(collection, sizeof(*collection));
    if (sNdsFighterDisplayContractPlayback != FALSE)
    {
        for (i = 0u; i < sNdsFighterDisplayContract.event_count; i++)
        {
            if (sNdsFighterDisplayContract.events[i].dobj == NULL)
            {
                continue;
            }
            collection->dobjs[collection->selected_count] =
                sNdsFighterDisplayContract.events[i].dobj;
            collection->indices[collection->selected_count] = i;
            collection->selected_count++;
            collection->total_count++;
        }
        return;
    }
    ndsFighterCollectAllDObjsWithDLRecursive(root, collection,
                                              &traversal_index);
}

static void ndsFighterDisplayContractSeedMaterialLights(
    NDSRendererStats *stats)
{
    u32 i;

    if ((stats == NULL) ||
        ((stats->light_color_mask &
          (NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK |
           NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK)) ==
         (NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK |
          NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK)))
    {
        return;
    }
    for (i = 0u; i < sNdsFighterDisplayContract.event_count; i++)
    {
        DObj *dobj = sNdsFighterDisplayContract.events[i].material_dobj;
        MObj *mobj = (dobj != NULL) ? dobj->mobj : NULL;

        if (mobj == NULL)
        {
            continue;
        }
        /* gcAddMObjForDObj copies the source MObjSub verbatim
         * (objman.c:1302-1335). Use its first selected material as the
         * initial RSP light state; later objdisplay.c:1289-1295 commands
         * remain authoritative and carry in original event order. */
        stats->light_color_1 = ndsFighterDisplayContractPackColor(
            mobj->sub.light1color.s.r, mobj->sub.light1color.s.g,
            mobj->sub.light1color.s.b, mobj->sub.light1color.s.a);
        stats->light_color_2 = ndsFighterDisplayContractPackColor(
            mobj->sub.light2color.s.r, mobj->sub.light2color.s.g,
            mobj->sub.light2color.s.b, mobj->sub.light2color.s.a);
        stats->light_color_mask |=
            NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK |
            NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
        gNdsFighterDisplayContractMaterialLightSeedCount++;
        gNdsFighterDisplayContractMaterialLight1 = stats->light_color_1;
        gNdsFighterDisplayContractMaterialLight2 = stats->light_color_2;
        return;
    }
}

static s32 ndsFighterDLAllDrawValidateRange(const Gfx *dl, size_t bytes,
                                            void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        gNdsFighterDLAllDrawRangeRejectCount++;
        return FALSE;
    }
    return TRUE;
}

static const Gfx *ndsFighterDLAllDrawResolveBranch(const Gfx *dl,
                                                   u32 *resolve_kind,
                                                   void *user)
{
    return ndsFighterDLDrawResolveBranch(dl, resolve_kind, user);
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterBuildNativeProductionInputs(
    u32 slot,
    NDSRelocLoadedFile *owner_file,
    const NDSFighterDLAllDrawCollection *collection,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *const *modelviews,
    NDSFighterDLDrawState *resolver,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    u32 i;

    if ((slot > 1u) || (owner_file == NULL) ||
        (owner_file->data == NULL) || (collection == NULL) ||
        (modelviews == NULL) || (resolver == NULL) ||
        (workspace == NULL) || (collection->selected_count == 0u) ||
        (collection->selected_count >
         NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }

    resolver->primary_file = owner_file;
    resolver->slot = slot;
    resolver->segment_e_base = NULL;
    resolver->segment_e_end = NULL;

    for (i = 0u; i < collection->selected_count; i++)
    {
        NDSRendererConfig *config = &workspace->production_configs[i];
        NDSRendererNativeFighterRoot *root =
            &workspace->production_roots[i];
        const NDSFighterDisplayContractEvent *event =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                &sNdsFighterDisplayContract.events[
                    collection->indices[i]] : NULL;

        *config = (NDSRendererConfig){0};
        *root = (NDSRendererNativeFighterRoot){0};

        config->max_depth = 8u;
        config->max_commands = 2048u;
        config->max_list_commands = 512u;
        config->initial_projection = projection;
        config->initial_modelview = modelviews[i];
        config->initial_geometry_mode =
            (event != NULL) ? event->geometry_mode : 0u;
        config->texture_data_layout =
            NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
        config->validate_range = ndsFighterDLAllDrawValidateRange;
        config->immutable_command_span =
            ndsRendererAdapterImmutableCommandSpan;
        config->resolve_branch = ndsFighterDLAllDrawResolveBranch;
        config->resolve_data = ndsFighterDLDrawResolveRendererData;
        config->user = resolver;

        if ((modelviews[i] == NULL) ||
            (ndsRendererAdapterComposeNativeRootMatrix(
                 modelviews[i], projection,
                 &workspace->composed_matrices[i]) == FALSE))
        {
            return FALSE;
        }

        root->root_offset = workspace->root_offsets[i];
        root->material_count = workspace->material_counts[i];
        root->composed_matrix = &workspace->composed_matrices[i];
        root->modelview_matrix = modelviews[i];
        root->materials = sNdsRendererAdapterNativeOwnerMaterials[i];
        root->config = config;

        if (event != NULL)
        {
            root->preamble.geometry_mode = event->geometry_mode;
            root->preamble.cycle_type = event->cycle_type;
            root->preamble.render_mode = event->render_mode;
            root->preamble.prim_color = event->prim_color;
            root->preamble.env_color = event->env_color;
            root->preamble.flags = NDS_RENDERER_NATIVE_PREAMBLE_VALID;
            if (event->light_valid != 0u)
            {
                root->preamble.light_dir_x = event->light.l.dir[0];
                root->preamble.light_dir_y = event->light.l.dir[1];
                root->preamble.light_dir_z = event->light.l.dir[2];
                root->preamble.flags |=
                    NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID;
            }
        }
    }
    return TRUE;
}
#endif

static void ndsFighterDLAllDrawRecordScreenPoint(
    s32 x, s32 y, u32 *screen_valid,
    s32 *screen_min_x, s32 *screen_max_x,
    s32 *screen_min_y, s32 *screen_max_y)
{
    if ((screen_valid == NULL) || (screen_min_x == NULL) ||
        (screen_max_x == NULL) || (screen_min_y == NULL) ||
        (screen_max_y == NULL))
    {
        return;
    }

    if (x < 0) { x = 0; }
    if (x >= (s32)NDS_FIGHTER_DL_DRAW_WIDTH)
    {
        x = (s32)NDS_FIGHTER_DL_DRAW_WIDTH - 1;
    }
    if (y < 0) { y = 0; }
    if (y >= (s32)NDS_FIGHTER_DL_DRAW_HEIGHT)
    {
        y = (s32)NDS_FIGHTER_DL_DRAW_HEIGHT - 1;
    }

    if (*screen_valid == 0u)
    {
        *screen_min_x = *screen_max_x = x;
        *screen_min_y = *screen_max_y = y;
        *screen_valid = 1u;
        return;
    }
    if (x < *screen_min_x) { *screen_min_x = x; }
    if (x > *screen_max_x) { *screen_max_x = x; }
    if (y < *screen_min_y) { *screen_min_y = y; }
    if (y > *screen_max_y) { *screen_max_y = y; }
}

static void ndsFighterDLAllDrawRasterizeStates(
    u32 slot, NDSFighterDLDrawState *states, const u8 *clean,
    u32 selected_count, u16 *pixels, u32 pitch)
{
    u32 axis;
    u32 best_axis = 0xffffffffu;
    u32 best_area = 0u;
    u32 best_nondegenerate_count = 0u;
    u32 i;
    s32 min_a = 0;
    s32 max_a = 0;
    s32 min_b = 0;
    s32 max_b = 0;
    u32 bounds_valid = 0u;
    s32 box_min_x = (slot == 0u) ? 4 : 52;
    s32 box_max_x = (slot == 0u) ? 43 : 91;
    s32 box_min_y = 4;
    s32 box_max_y = 67;
    s32 screen_min_x = 0;
    s32 screen_max_x = 0;
    s32 screen_min_y = 0;
    s32 screen_max_y = 0;
    u32 screen_valid = 0u;
    u32 pixel_count = 0u;
    u32 drawn_count = 0u;
    u32 real_drawn_count = 0u;
    u32 marker_drawn_count = 0u;
    u32 drawn_dobj_count = 0u;

    if ((states == NULL) || (clean == NULL) || (pixels == NULL))
    {
        return;
    }

    for (axis = 0u; axis < 3u; axis++)
    {
        s32 axis_min_a = 0;
        s32 axis_max_a = 0;
        s32 axis_min_b = 0;
        s32 axis_max_b = 0;
        u32 axis_bounds_valid = 0u;
        u32 area_sum = 0u;
        u32 nondegenerate_count = 0u;

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                ndsFighterDLDrawRecordAxisPoint(
                    v0, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v1, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
                ndsFighterDLDrawRecordAxisPoint(
                    v2, axis, &axis_bounds_valid, &axis_min_a, &axis_max_a,
                    &axis_min_b, &axis_max_b);
            }
        }
        if ((axis_bounds_valid == 0u) ||
            ((axis_min_a == axis_max_a) && (axis_min_b == axis_max_b)))
        {
            continue;
        }

        for (i = 0u; i < selected_count; i++)
        {
            u32 tri_index;

            if (clean[i] == FALSE)
            {
                continue;
            }
            for (tri_index = 0u; tri_index < states[i].triangle_count;
                 tri_index++)
            {
                const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
                const NDSFighterDLDrawVtx *v0;
                const NDSFighterDLDrawVtx *v1;
                const NDSFighterDLDrawVtx *v2;
                s32 x0;
                s32 y0;
                s32 x1;
                s32 y1;
                s32 x2;
                s32 y2;
                s32 area;

                if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                    (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
                {
                    continue;
                }
                v0 = &states[i].vertices[tri->v0];
                v1 = &states[i].vertices[tri->v1];
                v2 = &states[i].vertices[tri->v2];
                if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                    (v2->valid == FALSE))
                {
                    continue;
                }
                x0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y0 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v0, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y1 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v1, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);
                x2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 0u),
                    axis_min_a, axis_max_a, box_min_x, box_max_x);
                y2 = ndsFighterDLDrawMapCoord(
                    ndsFighterDLDrawAxisCoord(v2, axis, 1u),
                    axis_min_b, axis_max_b, box_max_y, box_min_y);

                area = ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2);
                if (area == 0)
                {
                    continue;
                }
                nondegenerate_count++;
                area_sum += (area < 0) ? (u32)-area : (u32)area;
            }
        }

        if ((best_axis > 2u) ||
            (nondegenerate_count > best_nondegenerate_count) ||
            ((nondegenerate_count == best_nondegenerate_count) &&
             (area_sum > best_area)))
        {
            best_area = area_sum;
            best_nondegenerate_count = nondegenerate_count;
            best_axis = axis;
            min_a = axis_min_a;
            max_a = axis_max_a;
            min_b = axis_min_b;
            max_b = axis_max_b;
            bounds_valid = 1u;
        }
    }

    if (best_axis > 2u)
    {
        return;
    }
    if ((bounds_valid == 0u) || ((min_a == max_a) && (min_b == max_b)))
    {
        return;
    }

    for (i = 0u; i < selected_count; i++)
    {
        u32 tri_index;
        u32 state_drawn = 0u;

        if (clean[i] == FALSE)
        {
            continue;
        }
        for (tri_index = 0u; tri_index < states[i].triangle_count;
             tri_index++)
        {
            const NDSFighterDLDrawTri *tri = &states[i].tris[tri_index];
            const NDSFighterDLDrawVtx *v0;
            const NDSFighterDLDrawVtx *v1;
            const NDSFighterDLDrawVtx *v2;
            s32 x0;
            s32 y0;
            s32 x1;
            s32 y1;
            s32 x2;
            s32 y2;
            u32 before;
            u32 marker_drawn = 0u;
            u16 fill;
            u16 edge;

            if ((tri->v0 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v1 >= NDS_FIGHTER_DL_DRAW_MAX_VTX) ||
                (tri->v2 >= NDS_FIGHTER_DL_DRAW_MAX_VTX))
            {
                continue;
            }
            v0 = &states[i].vertices[tri->v0];
            v1 = &states[i].vertices[tri->v1];
            v2 = &states[i].vertices[tri->v2];
            if ((v0->valid == FALSE) || (v1->valid == FALSE) ||
                (v2->valid == FALSE))
            {
                continue;
            }

            x0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y0 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v0, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y1 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v1, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            x2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 0u),
                min_a, max_a, box_min_x, box_max_x);
            y2 = ndsFighterDLDrawMapCoord(
                ndsFighterDLDrawAxisCoord(v2, best_axis, 1u),
                min_b, max_b, box_max_y, box_min_y);
            fill = ndsFighterDLDrawTriangleColor(&states[i], tri);
            edge = ndsFighterDLDrawRGB15(255, 255, 255);
            before = pixel_count;
            if (ndsFighterDLDrawEdge(x0, y0, x1, y1, x2, y2) == 0)
            {
                s32 cx = (x0 + x1 + x2) / 3;
                s32 cy = (y0 + y1 + y2) / 3;

                ndsFighterDLDrawTriangle(pixels, pitch,
                                         cx - 5, cy - 3,
                                         cx + 5, cy - 3,
                                         cx, cy + 5,
                                         fill, edge, &pixel_count);
                marker_drawn = 1u;
                x0 = cx - 5;
                y0 = cy - 3;
                x1 = cx + 5;
                y1 = cy - 3;
                x2 = cx;
                y2 = cy + 5;
            }
            else
            {
                ndsFighterDLDrawTriangle(pixels, pitch,
                                         x0, y0, x1, y1, x2, y2,
                                         fill, edge, &pixel_count);
            }
            if (pixel_count != before)
            {
                drawn_count++;
                if (marker_drawn != 0u)
                {
                    marker_drawn_count++;
                }
                else
                {
                    real_drawn_count++;
                }
                state_drawn = 1u;
                ndsFighterDLAllDrawRecordScreenPoint(
                    x0, y0, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
                ndsFighterDLAllDrawRecordScreenPoint(
                    x1, y1, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
                ndsFighterDLAllDrawRecordScreenPoint(
                    x2, y2, &screen_valid, &screen_min_x, &screen_max_x,
                    &screen_min_y, &screen_max_y);
            }
        }
        if (state_drawn != 0u)
        {
            drawn_dobj_count++;
        }
    }

    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0Axis = best_axis;
        gNdsFighterDLAllDrawP0Area = best_area;
        gNdsFighterDLAllDrawP0MinA = min_a;
        gNdsFighterDLAllDrawP0MaxA = max_a;
        gNdsFighterDLAllDrawP0MinB = min_b;
        gNdsFighterDLAllDrawP0MaxB = max_b;
        gNdsFighterDLAllDrawP0ScreenMinX = screen_min_x;
        gNdsFighterDLAllDrawP0ScreenMaxX = screen_max_x;
        gNdsFighterDLAllDrawP0ScreenMinY = screen_min_y;
        gNdsFighterDLAllDrawP0ScreenMaxY = screen_max_y;
        gNdsFighterDLAllDrawP0PixelCount = pixel_count;
        gNdsFighterDLAllDrawP0TriangleDrawnCount = drawn_count;
        gNdsFighterDLAllDrawP0RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLAllDrawP0MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLAllDrawP0DrawnDObjCount = drawn_dobj_count;
    }
    else
    {
        gNdsFighterDLAllDrawP1Axis = best_axis;
        gNdsFighterDLAllDrawP1Area = best_area;
        gNdsFighterDLAllDrawP1MinA = min_a;
        gNdsFighterDLAllDrawP1MaxA = max_a;
        gNdsFighterDLAllDrawP1MinB = min_b;
        gNdsFighterDLAllDrawP1MaxB = max_b;
        gNdsFighterDLAllDrawP1ScreenMinX = screen_min_x;
        gNdsFighterDLAllDrawP1ScreenMaxX = screen_max_x;
        gNdsFighterDLAllDrawP1ScreenMinY = screen_min_y;
        gNdsFighterDLAllDrawP1ScreenMaxY = screen_max_y;
        gNdsFighterDLAllDrawP1PixelCount = pixel_count;
        gNdsFighterDLAllDrawP1TriangleDrawnCount = drawn_count;
        gNdsFighterDLAllDrawP1RealTriangleDrawnCount = real_drawn_count;
        gNdsFighterDLAllDrawP1MarkerTriangleDrawnCount = marker_drawn_count;
        gNdsFighterDLAllDrawP1DrawnDObjCount = drawn_dobj_count;
    }
}

static void ndsFighterDLAllDrawResetFailureDiagnostics(void)
{
    gNdsFighterDLAllDrawP0FirstFailedSelectedIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP1FirstFailedSelectedIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP0FirstFailedTreeIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP1FirstFailedTreeIndex = 0xffffffffu;
    gNdsFighterDLAllDrawP0FirstFailedReason = 0u;
    gNdsFighterDLAllDrawP1FirstFailedReason = 0u;
    gNdsFighterDLAllDrawP0FirstFailedDObj = 0u;
    gNdsFighterDLAllDrawP1FirstFailedDObj = 0u;
    gNdsFighterDLAllDrawP0FirstFailedDL = 0u;
    gNdsFighterDLAllDrawP1FirstFailedDL = 0u;
    gNdsFighterDLAllDrawP0FirstFailedCommandCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedCommandCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedFirstOpcode = 0u;
    gNdsFighterDLAllDrawP1FirstFailedFirstOpcode = 0u;
    gNdsFighterDLAllDrawP0FirstFailedBlocker = 0u;
    gNdsFighterDLAllDrawP1FirstFailedBlocker = 0u;
    gNdsFighterDLAllDrawP0FirstFailedUnsupportedOpcode = 0u;
    gNdsFighterDLAllDrawP1FirstFailedUnsupportedOpcode = 0u;
    gNdsFighterDLAllDrawP0FirstFailedUnsupportedCommandCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedUnsupportedCommandCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedVertexRangeRejectCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedVertexRangeRejectCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedVertexDecodedCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedVertexDecodedCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedTriangleCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedTriangleCount = 0u;
    gNdsFighterDLAllDrawP0FirstFailedTriangleValidCount = 0u;
    gNdsFighterDLAllDrawP1FirstFailedTriangleValidCount = 0u;
}

static u32 ndsFighterDLAllDrawFailureReason(
    u32 blocker, u32 unsupported_opcode, u32 unsupported_count,
    const NDSFighterDLDrawState *state)
{
    u32 reason = 0u;

    if (blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        reason |= NDS_FIGHTER_DL_ALL_FAIL_BLOCKER;
    }
    if (unsupported_opcode != 0u)
    {
        reason |= NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_OPCODE;
    }
    if (unsupported_count != 0u)
    {
        reason |= NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_COUNT;
    }
    if (state != NULL)
    {
        if (state->vertex_range_reject_count != 0u)
        {
            reason |= NDS_FIGHTER_DL_ALL_FAIL_VERTEX_RANGE;
        }
        if (state->vertex_decoded_count == 0u)
        {
            reason |= NDS_FIGHTER_DL_ALL_FAIL_NO_VERTS;
        }
        if (state->triangle_valid_count == 0u)
        {
            reason |= NDS_FIGHTER_DL_ALL_FAIL_NO_VALID_TRIS;
        }
    }
    if (reason == 0u)
    {
        reason = NDS_FIGHTER_DL_ALL_FAIL_UNKNOWN;
    }
    return reason;
}

static void ndsFighterDLAllDrawRecordFirstFailure(
    u32 slot, u32 selected_index, u32 tree_index, const DObj *dobj,
    const Gfx *dl, u32 reason, u32 blocker, u32 unsupported_opcode,
    u32 unsupported_count, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats)
{
    if ((reason == 0u) || (state == NULL) || (stats == NULL))
    {
        return;
    }

    if ((slot == 0u) &&
        (gNdsFighterDLAllDrawP0FirstFailedReason == 0u))
    {
        gNdsFighterDLAllDrawP0FirstFailedSelectedIndex = selected_index;
        gNdsFighterDLAllDrawP0FirstFailedTreeIndex = tree_index;
        gNdsFighterDLAllDrawP0FirstFailedReason = reason;
        gNdsFighterDLAllDrawP0FirstFailedDObj = (u32)(uintptr_t)dobj;
        gNdsFighterDLAllDrawP0FirstFailedDL = (u32)(uintptr_t)dl;
        gNdsFighterDLAllDrawP0FirstFailedCommandCount =
            stats->command_count;
        gNdsFighterDLAllDrawP0FirstFailedFirstOpcode =
            stats->first_opcode;
        gNdsFighterDLAllDrawP0FirstFailedBlocker = blocker;
        gNdsFighterDLAllDrawP0FirstFailedUnsupportedOpcode =
            unsupported_opcode;
        gNdsFighterDLAllDrawP0FirstFailedUnsupportedCommandCount =
            unsupported_count;
        gNdsFighterDLAllDrawP0FirstFailedVertexRangeRejectCount =
            state->vertex_range_reject_count;
        gNdsFighterDLAllDrawP0FirstFailedVertexDecodedCount =
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP0FirstFailedTriangleCount =
            state->triangle_count;
        gNdsFighterDLAllDrawP0FirstFailedTriangleValidCount =
            state->triangle_valid_count;
    }
    else if ((slot == 1u) &&
             (gNdsFighterDLAllDrawP1FirstFailedReason == 0u))
    {
        gNdsFighterDLAllDrawP1FirstFailedSelectedIndex = selected_index;
        gNdsFighterDLAllDrawP1FirstFailedTreeIndex = tree_index;
        gNdsFighterDLAllDrawP1FirstFailedReason = reason;
        gNdsFighterDLAllDrawP1FirstFailedDObj = (u32)(uintptr_t)dobj;
        gNdsFighterDLAllDrawP1FirstFailedDL = (u32)(uintptr_t)dl;
        gNdsFighterDLAllDrawP1FirstFailedCommandCount =
            stats->command_count;
        gNdsFighterDLAllDrawP1FirstFailedFirstOpcode =
            stats->first_opcode;
        gNdsFighterDLAllDrawP1FirstFailedBlocker = blocker;
        gNdsFighterDLAllDrawP1FirstFailedUnsupportedOpcode =
            unsupported_opcode;
        gNdsFighterDLAllDrawP1FirstFailedUnsupportedCommandCount =
            unsupported_count;
        gNdsFighterDLAllDrawP1FirstFailedVertexRangeRejectCount =
            state->vertex_range_reject_count;
        gNdsFighterDLAllDrawP1FirstFailedVertexDecodedCount =
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP1FirstFailedTriangleCount =
            state->triangle_count;
        gNdsFighterDLAllDrawP1FirstFailedTriangleValidCount =
            state->triangle_valid_count;
    }
}

static void ndsFighterDLAllDrawAccumulateStats(
    u32 slot, u32 selected_index, u32 tree_index, const DObj *dobj,
    const Gfx *dl, const NDSFighterDLDrawState *state,
    const NDSRendererStats *stats, u8 *clean)
{
    u32 blocker = (stats != NULL) ? stats->blocker : 0xffffffffu;
    u32 unsupported_opcode = 0u;
    u32 unsupported_count = 0u;
    u32 clean_selected;
    u32 failure_reason = 0u;

    if ((state == NULL) || (stats == NULL) || (clean == NULL))
    {
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    if (slot == 0u)
    {
        ndsRendererAdapterAccumulateDepth(
            stats,
            &gNdsRendererDepthFighterP0Samples,
            &gNdsRendererDepthFighterP0Min,
            &gNdsRendererDepthFighterP0Max,
            &gNdsRendererDepthFighterP0WMin,
            &gNdsRendererDepthFighterP0WMax);
    }
    else if (slot == 1u)
    {
        ndsRendererAdapterAccumulateDepth(
            stats,
            &gNdsRendererDepthFighterP1Samples,
            &gNdsRendererDepthFighterP1Min,
            &gNdsRendererDepthFighterP1Max,
            &gNdsRendererDepthFighterP1WMin,
            &gNdsRendererDepthFighterP1WMax);
    }
#endif

    gNdsFighterDLAllDrawHardwareTextureBindCount +=
        stats->hardware_texture_bind_count;
    gNdsFighterDLAllDrawHardwareTextureUploadCount +=
        stats->hardware_texture_upload_count;
    gNdsFighterDLAllDrawHardwareTextureReadyCount +=
        stats->hardware_texture_ready_count;
    gNdsFighterDLAllDrawHardwareTextureRejectCount +=
        stats->hardware_texture_reject_count;
    if (stats->hardware_texture_ready_count != 0u)
    {
        if (stats->hardware_texture_format < 32u)
        {
            gNdsFighterDLAllDrawHardwareTextureFormatMask |=
                1u << stats->hardware_texture_format;
        }
        if (stats->hardware_texture_width >
            gNdsFighterDLAllDrawHardwareTextureMaxWidth)
        {
            gNdsFighterDLAllDrawHardwareTextureMaxWidth =
                stats->hardware_texture_width;
        }
        if (stats->hardware_texture_height >
            gNdsFighterDLAllDrawHardwareTextureMaxHeight)
        {
            gNdsFighterDLAllDrawHardwareTextureMaxHeight =
                stats->hardware_texture_height;
        }
    }

    unsupported_opcode = (stats->unsupported_opcode != 0u) ?
        stats->unsupported_opcode : state->unsupported_opcode;
    unsupported_count = stats->unsupported_command_count +
        state->unsupported_command_count;
    clean_selected =
        (blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (unsupported_opcode == 0u) &&
        (unsupported_count == 0u) &&
        (state->vertex_range_reject_count == 0u) &&
        (state->vertex_decoded_count != 0u) &&
        (state->triangle_valid_count != 0u);
    clean[selected_index] = (u8)clean_selected;
    if (clean_selected == FALSE)
    {
        failure_reason = ndsFighterDLAllDrawFailureReason(
            blocker, unsupported_opcode, unsupported_count, state);
        ndsFighterDLAllDrawRecordFirstFailure(
            slot, selected_index, tree_index, dobj, dl, failure_reason,
            blocker, unsupported_opcode, unsupported_count, state, stats);
    }

    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLAllDrawP0CleanCount++;
        }
        else
        {
            gNdsFighterDLAllDrawP0FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLAllDrawP0FirstBlocker == 0u))
        {
            gNdsFighterDLAllDrawP0FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLAllDrawP0BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLAllDrawP0CommandCount += stats->command_count;
        if (gNdsFighterDLAllDrawP0FirstOpcode == 0u)
        {
            gNdsFighterDLAllDrawP0FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLAllDrawP0UnsupportedOpcode == 0u))
        {
            gNdsFighterDLAllDrawP0UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLAllDrawP0UnsupportedCommandCount += unsupported_count;
        gNdsFighterDLAllDrawP0VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP0MatrixMvpRecalcCount +=
            stats->matrix_mvp_recalc_count;
        gNdsFighterDLAllDrawP0MatrixMoveWordCount +=
            stats->matrix_move_word_count;
        gNdsFighterDLAllDrawP0HardwareTriangleCount +=
            stats->hardware_triangle_count;
        gNdsFighterDLAllDrawP0HardwareZBufferTriangleCount +=
            stats->hardware_zbuffer_triangle_count;
        gNdsFighterDLAllDrawP0HardwareProjectedDepthTriangleCount +=
            stats->hardware_projected_depth_triangle_count;
        gNdsFighterDLAllDrawP0HardwareDecalDepthTriangleCount +=
            stats->hardware_decal_depth_triangle_count;
        gNdsFighterDLAllDrawP0HardwareOracleTriangleCount +=
            stats->hardware_oracle_triangle_count;
        gNdsFighterDLAllDrawP0HardwareOracleRejectCount +=
            stats->hardware_oracle_reject_count;
        gNdsFighterDLAllDrawP0HardwareMatrixSeedCount +=
            stats->hardware_matrix_seed_count;
        gNdsFighterDLAllDrawP0TriangleCount += state->triangle_count;
        gNdsFighterDLAllDrawP0TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLAllDrawP0ColorChecksum =
            (gNdsFighterDLAllDrawP0ColorChecksum * 33u) ^
            state->color_checksum;
    }
    else if (slot == 1u)
    {
        gNdsFighterDLAllDrawP1AttemptCount++;
        if (clean_selected != FALSE)
        {
            gNdsFighterDLAllDrawP1CleanCount++;
        }
        else
        {
            gNdsFighterDLAllDrawP1FailedCount++;
        }
        if ((blocker != NDS_RENDERER_BLOCKER_NONE) &&
            (gNdsFighterDLAllDrawP1FirstBlocker == 0u))
        {
            gNdsFighterDLAllDrawP1FirstBlocker = blocker;
        }
        if (blocker != NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsFighterDLAllDrawP1BlockerMask |=
                1u << (selected_index & 31u);
        }
        gNdsFighterDLAllDrawP1CommandCount += stats->command_count;
        if (gNdsFighterDLAllDrawP1FirstOpcode == 0u)
        {
            gNdsFighterDLAllDrawP1FirstOpcode = stats->first_opcode;
        }
        if ((unsupported_opcode != 0u) &&
            (gNdsFighterDLAllDrawP1UnsupportedOpcode == 0u))
        {
            gNdsFighterDLAllDrawP1UnsupportedOpcode = unsupported_opcode;
        }
        gNdsFighterDLAllDrawP1UnsupportedCommandCount += unsupported_count;
        gNdsFighterDLAllDrawP1VertexDecodedCount +=
            state->vertex_decoded_count;
        gNdsFighterDLAllDrawP1MatrixMvpRecalcCount +=
            stats->matrix_mvp_recalc_count;
        gNdsFighterDLAllDrawP1MatrixMoveWordCount +=
            stats->matrix_move_word_count;
        gNdsFighterDLAllDrawP1HardwareTriangleCount +=
            stats->hardware_triangle_count;
        gNdsFighterDLAllDrawP1HardwareZBufferTriangleCount +=
            stats->hardware_zbuffer_triangle_count;
        gNdsFighterDLAllDrawP1HardwareProjectedDepthTriangleCount +=
            stats->hardware_projected_depth_triangle_count;
        gNdsFighterDLAllDrawP1HardwareDecalDepthTriangleCount +=
            stats->hardware_decal_depth_triangle_count;
        gNdsFighterDLAllDrawP1HardwareOracleTriangleCount +=
            stats->hardware_oracle_triangle_count;
        gNdsFighterDLAllDrawP1HardwareOracleRejectCount +=
            stats->hardware_oracle_reject_count;
        gNdsFighterDLAllDrawP1HardwareMatrixSeedCount +=
            stats->hardware_matrix_seed_count;
        gNdsFighterDLAllDrawP1TriangleCount += state->triangle_count;
        gNdsFighterDLAllDrawP1TriangleValidCount +=
            state->triangle_valid_count;
        gNdsFighterDLAllDrawP1ColorChecksum =
            (gNdsFighterDLAllDrawP1ColorChecksum * 33u) ^
            state->color_checksum;
    }

    gNdsFighterDLAllDrawVertexRangeRejectCount +=
        state->vertex_range_reject_count;
}

static void ndsFighterMarioFoxDLAllDrawForSlot(u32 slot, FTStruct *fp,
                                               u16 *pixels, u32 pitch)
{
    DObj *root;
    NDSFighterDLAllDrawCollection collection;
    NDSFighterDLDrawState *states;
    NDSFighterDLDrawState persistent_state;
    /* The snapshot table is traversal-owned but too large for BattleShip's
     * nested task stack. Draw callbacks are serialized, so one reset fixed
     * cache preserves the same per-fighter lifetime without caching output. */
    static NDSRendererVertexCache persistent_renderer_vertices;
#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
    NDSRendererStats *stats;
#endif
    NDSRendererStats persistent_stats;
    u8 *clean;
    sb32 no_oracle;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    sb32 detailed_output;
    u32 runtime_hardware_triangle_count = 0u;
#endif
    u32 root_x_before;
    u32 root_x_after;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererProfileOwner owner_id;
    NDSRelocLoadedFile *native_owner_file = NULL;
    NDSRelocLoadedFile **native_owner_loaded =
        sNdsRendererAdapterNativeOwnerWorkspace.loaded;
    u32 *native_owner_root_offsets =
        sNdsRendererAdapterNativeOwnerWorkspace.root_offsets;
    u32 *native_owner_material_counts =
        sNdsRendererAdapterNativeOwnerWorkspace.material_counts;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    NDSRendererNativeMaterial *native_materials =
        &sNdsRendererAdapterNativeOwnerMaterials[0u][0u];
    DObj **native_owner_matrix_bindings =
        sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings;
    DObj **native_owner_material_dobjs =
        sNdsRendererAdapterNativeOwnerWorkspace.material_dobjs;
    const NDSRendererMatrix20p12 **native_owner_modelviews =
        sNdsRendererAdapterNativeOwnerWorkspace.modelviews;
    const NDSRendererMatrix20p12 *native_owner_projection = NULL;
    u32 native_owner_material_saved_root_count = 0u;
    sb32 native_owner_started = FALSE;
    sb32 native_owner_failed = FALSE;
    sb32 native_owner_production_attempted = FALSE;
    sb32 native_owner_production_mode;
#else
    NDSRendererNativeMaterial *native_materials =
        sNdsRendererAdapterNativeMaterials;
#endif
    u32 native_material_count = 0u;
    sb32 native_owner_enabled;
#endif

    if ((slot > 1u) ||
        (ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        ((pixels != NULL) &&
         ((fp->status_id != nFTCommonStatusWait) ||
          (fp->motion_id != nFTCommonMotionWait) ||
          (fp->ga != nMPKineticsGround))))
    {
        return;
    }

    root = fp->joints[nFTPartsJointTopN];
    root_x_before = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;

    ndsFighterCollectAllDObjsWithDL(root, &collection);
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0CandidateCount = collection.total_count;
        gNdsFighterDLAllDrawP0SelectedCount = collection.selected_count;
        gNdsFighterDLAllDrawP0SelectedIndexMask =
            collection.selected_index_mask;
    }
    else
    {
        gNdsFighterDLAllDrawP1CandidateCount = collection.total_count;
        gNdsFighterDLAllDrawP1SelectedCount = collection.selected_count;
        gNdsFighterDLAllDrawP1SelectedIndexMask =
            collection.selected_index_mask;
    }

    states = sNdsFighterDLAllDrawStates[slot];
#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
    stats = sNdsFighterDLAllDrawStats[slot];
#endif
    clean = sNdsFighterDLAllDrawClean[slot];
    no_oracle = (ndsRendererHardwareNoOracleEnabled() != FALSE) ? TRUE :
                                                                    FALSE;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    detailed_output = ((pixels != NULL) || (no_oracle == FALSE)) ? TRUE :
                                                                       FALSE;
    if (detailed_output != FALSE)
    {
        bzero(states, sizeof(sNdsFighterDLAllDrawStates[slot]));
        bzero(&persistent_state, sizeof(persistent_state));
        bzero(clean, sizeof(sNdsFighterDLAllDrawClean[slot]));
    }
    else
    {
        /* The shipping callback is null, so only the resolver's segment
         * ownership crosses source part lists. Do not clear/copy the
         * software-preview vertices or update its historical proof ledger. */
        persistent_state.segment_e_base = NULL;
        persistent_state.segment_e_end = NULL;
    }
#else
    bzero(states, sizeof(sNdsFighterDLAllDrawStates[slot]));
    bzero(&persistent_state, sizeof(persistent_state));
    bzero(clean, sizeof(sNdsFighterDLAllDrawClean[slot]));
#endif
    ndsRendererInitVertexCache(&persistent_renderer_vertices);
    ndsRendererInitStats(&persistent_stats);
    if (sNdsFighterDisplayContractPlayback != FALSE)
    {
        persistent_stats.geometry_mode =
            sNdsFighterDisplayContract.geometry_mode;
        persistent_stats.prim_color = sNdsFighterDisplayContract.prim_color;
        persistent_stats.env_color = sNdsFighterDisplayContract.env_color;
        if (sNdsFighterDisplayContract.light_valid != 0u)
        {
            persistent_stats.light_dir_x =
                sNdsFighterDisplayContract.light.l.dir[0];
            persistent_stats.light_dir_y =
                sNdsFighterDisplayContract.light.l.dir[1];
            persistent_stats.light_dir_z =
                sNdsFighterDisplayContract.light.l.dir[2];
            persistent_stats.light_dir_mask = 1u;
        }
        ndsFighterDisplayContractSeedMaterialLights(&persistent_stats);
    }
#if NDS_RENDERER_HW_TRIANGLES
    owner_id = (slot == 0u) ? NDS_RENDERER_PROFILE_OWNER_MARIO :
                              NDS_RENDERER_PROFILE_OWNER_FOX;
    native_owner_enabled =
        (((gNdsRendererFastRunMode ==
           NDS_RENDERER_FAST_RUN_NATIVE_MARIO) && (slot == 0u)) ||
         ((gNdsRendererFastRunMode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FOX) && (slot == 1u)) ||
         (gNdsRendererFastRunMode ==
          NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ||
         (gNdsRendererFastRunMode ==
          NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION)) ? TRUE :
                                                                    FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    native_owner_production_mode =
        (gNdsRendererFastRunMode ==
         NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) ? TRUE :
                                                                  FALSE;
#endif
    ndsRendererProfileSetOwner(owner_id);
    if (native_owner_enabled != FALSE)
    {
        u32 expected_asset_id = (slot == 0u) ? 0x128u : 0x139u;

        if ((collection.selected_count == 0u) ||
            (collection.selected_count >
             NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
        {
            native_owner_enabled = FALSE;
        }
        for (i = 0u;
             (native_owner_enabled != FALSE) &&
             (i < collection.selected_count);
             i++)
        {
            const NDSFighterDisplayContractEvent *event =
                (sNdsFighterDisplayContractPlayback != FALSE) ?
                    &sNdsFighterDisplayContract.events[
                        collection.indices[i]] : NULL;
            const Gfx *native_dl =
                (event != NULL) ? event->dl : collection.dobjs[i]->dl;
            DObj *material_dobj =
                (event != NULL) ? event->material_dobj :
                                  collection.dobjs[i];
            NDSRelocLoadedFile *loaded =
                ndsRelocFindLoadedFileContaining(
                    native_dl, sizeof(*native_dl));
            MObj *mobj;
            u32 material_count = 0u;

            if ((native_dl == NULL) || (loaded == NULL) ||
                (loaded->data == NULL) ||
                (loaded->asset_id != expected_asset_id) ||
                ((native_owner_file != NULL) &&
                 (loaded != native_owner_file)) ||
                (loaded->data_size < sizeof(*native_dl)) ||
                ((uintptr_t)native_dl < (uintptr_t)loaded->data) ||
                (((uintptr_t)native_dl - (uintptr_t)loaded->data) >
                 (loaded->data_size - sizeof(*native_dl))))
            {
                native_owner_enabled = FALSE;
                break;
            }
            native_owner_file = loaded;
            native_owner_loaded[i] = loaded;
            native_owner_root_offsets[i] =
                (u32)((uintptr_t)native_dl - (uintptr_t)loaded->data);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            native_owner_matrix_bindings[i] =
                (event != NULL) ? event->matrix_dobj :
                                  collection.dobjs[i];
            native_owner_material_dobjs[i] = material_dobj;
#endif
            for (mobj = (material_dobj != NULL) ? material_dobj->mobj :
                                                  NULL;
                 mobj != NULL;
                 mobj = mobj->next)
            {
                material_count++;
                if (material_count >
                    NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX)
                {
                    native_owner_enabled = FALSE;
                    break;
                }
            }
            native_owner_material_counts[i] = material_count;
        }
        if ((native_owner_enabled != FALSE) &&
            ((native_owner_file == NULL) ||
#if NDS_RENDERER_PROFILE_LEVEL < 2
            (ndsRendererAdapterValidateNativeOwnerCached(
                 slot, native_owner_file,
                 collection.selected_count,
                 native_owner_root_offsets,
                 native_owner_material_counts) == FALSE)))
#else
            (ndsRendererValidateNativeFighterOwner(
                 slot, native_owner_file->data_size,
                 collection.selected_count,
                 native_owner_root_offsets,
                 native_owner_material_counts) == FALSE)))
#endif
        {
            native_owner_enabled = FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_owner_enabled != FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            u32 owner_matrix_start = cpuGetTiming();
#endif
            if (ndsRendererAdapterPrepareNativeOwnerMatrices(
                    root, native_owner_matrix_bindings,
                    collection.selected_count,
                    (gGCCurrentCamera != NULL) ?
                        CObjGetStruct(gGCCurrentCamera) : NULL,
                    &native_owner_projection,
                    native_owner_modelviews) == FALSE)
            {
                native_owner_enabled = FALSE;
            }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            {
                u32 owner_matrix_ticks =
                    cpuGetTiming() - owner_matrix_start;

                gNdsRendererProfileMatrixTicks += owner_matrix_ticks;
            }
#endif
        }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_owner_enabled != FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            u32 owner_material_start = cpuGetTiming();
#endif
            native_owner_material_saved_root_count = 0u;
            for (i = 0u; i < collection.selected_count; i++)
            {
                u32 prepared_material_count = 0u;

                sNdsRendererAdapterNativeOwnerTextureCounts[i] =
                    ndsRendererAdapterSaveNativeMaterialTextureIds(
                        native_owner_material_dobjs[i],
                        sNdsRendererAdapterNativeOwnerTextureCurr[i],
                        sNdsRendererAdapterNativeOwnerTextureNext[i],
                        NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX);
                native_owner_material_saved_root_count = i + 1u;
                if ((sNdsRendererAdapterNativeOwnerTextureCounts[i] !=
                     native_owner_material_counts[i]) ||
                    (ndsRendererAdapterPrepareNativeMaterials(
                         native_owner_material_dobjs[i],
                         sNdsRendererAdapterNativeOwnerMaterials[i],
                         NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX,
                         &prepared_material_count) == FALSE) ||
                    (prepared_material_count !=
                     native_owner_material_counts[i]) ||
                    (ndsRendererAdapterValidateNativeOwnerMaterials(
                         sNdsRendererAdapterNativeOwnerMaterials[i],
                         prepared_material_count) == FALSE))
                {
                    native_owner_enabled = FALSE;
                    break;
                }
            }
            if (native_owner_enabled == FALSE)
            {
                ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                    native_owner_material_dobjs,
                    native_owner_material_saved_root_count);
                native_owner_material_saved_root_count = 0u;
            }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            {
                u32 owner_material_ticks =
                    cpuGetTiming() - owner_material_start;

                gNdsRendererProfileMaterialTicks += owner_material_ticks;
            }
#endif
        }
#endif
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if ((native_owner_enabled != FALSE) &&
        (native_owner_production_mode != FALSE) &&
        (detailed_output == FALSE) && (no_oracle != FALSE))
    {
        if (ndsRendererAdapterBuildNativeProductionInputs(
                slot, native_owner_file, &collection,
                native_owner_projection, native_owner_modelviews,
                &persistent_state,
                &sNdsRendererAdapterNativeOwnerWorkspace) == FALSE)
        {
            ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                native_owner_material_dobjs,
                native_owner_material_saved_root_count);
            native_owner_material_saved_root_count = 0u;
            native_owner_enabled = FALSE;
        }
        if (native_owner_enabled != FALSE)
        {
            s32 production_result;
            u32 production_hardware_started = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            u32 owner_dl_start = cpuGetTiming();
#endif
            ndsFighterDLDrawResetRuntimeRendererStats(&persistent_stats);
            native_owner_production_attempted = TRUE;
            production_result =
                ndsRendererExecuteNativeFighterOwnerProduction(
                    slot, native_owner_file->data,
                    sNdsRendererAdapterNativeOwnerWorkspace.production_roots,
                    collection.selected_count,
                    NULL, NULL, &persistent_stats,
                    &production_hardware_started);
            if (production_result != FALSE)
            {
                runtime_hardware_triangle_count =
                    persistent_stats.hardware_triangle_count;
                /* Native material preparation advances every live MObj once,
                 * matching the generic path. A successful owner consumes that
                 * advancement and must not restore it. */
                native_owner_material_saved_root_count = 0u;
            }
            else if (production_hardware_started == FALSE)
            {
                /* The production owner rejected its complete input contract
                 * before touching GX. Restore the FRAC fields serialized by
                 * native material preparation and let the ordinary path
                 * advance and draw them exactly once. */
                ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                    native_owner_material_dobjs,
                    native_owner_material_saved_root_count);
                native_owner_material_saved_root_count = 0u;
                native_owner_enabled = FALSE;
                native_owner_production_attempted = FALSE;
            }
            else
            {
                native_owner_failed = TRUE;
                if (persistent_stats.blocker == NDS_RENDERER_BLOCKER_NONE)
                {
                    persistent_stats.blocker =
                        NDS_RENDERER_BLOCKER_UNSUPPORTED;
                }
            }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileDLTicks += cpuGetTiming() - owner_dl_start;
#endif
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    gNdsRendererProfileOwners[(u32)owner_id].entry_state_hash =
        ndsRendererOwnerHashRuntimeState(&persistent_stats);
    gNdsRendererProfileOwners[(u32)owner_id].entry_vertex_cache_hash =
        ndsRendererOwnerHashVertexCache(&persistent_renderer_vertices);
    gNdsRendererProfileOwners[(u32)owner_id].entry_resolver_hash =
        ndsRendererOwnerHashResolver(&persistent_state);
    gNdsRendererProfileOwners[(u32)owner_id].entry_global_hash =
        ndsRendererProfileGlobalStateHash();
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if ((native_owner_enabled != FALSE) &&
        (native_owner_production_attempted == FALSE))
    {
        if (ndsRendererBeginNativeFighterOwner(
                slot, &persistent_stats,
                &persistent_renderer_vertices) != FALSE)
        {
            native_owner_started = TRUE;
        }
        else
        {
            ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
                native_owner_material_dobjs,
                native_owner_material_saved_root_count);
            native_owner_material_saved_root_count = 0u;
            native_owner_enabled = FALSE;
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (native_owner_production_attempted == FALSE)
#endif
    {
    for (i = 0u; i < collection.selected_count; i++)
    {
        const NDSFighterDisplayContractEvent *contract_event =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                &sNdsFighterDisplayContract.events[collection.indices[i]] :
                NULL;
        const Gfx *dl = (sNdsFighterDisplayContractPlayback != FALSE) ?
            contract_event->dl :
            collection.dobjs[i]->dl;
        NDSRelocLoadedFile *loaded;
        NDSFighterDLDrawState *current_state;
        NDSRendererConfig config;
        NDSRendererStats *current_stats;
        NDSRendererMatrix20p12 initial_projection;
        NDSRendererMatrix20p12 initial_modelview;
        const NDSRendererMatrix20p12 *initial_projection_ptr;
        const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
        void *saved_graphics_heap_ptr;
        DObj *material_dobj;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        s32 native_saved_texture_curr[
            NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
        s32 native_saved_texture_next[
            NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX];
        u32 native_saved_texture_count = 0u;
        u32 native_prepared_material_count = 0u;
        sb32 native_material_built = FALSE;
#endif
        u32 native_root_offset = 0u;
        sb32 native_root_enabled = FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        u32 step_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        NDSRendererOwnerStatsSnapshot owner_stats_before;
#endif
#endif

#if NDS_RENDERER_HW_TRIANGLES
        loaded = (native_owner_enabled != FALSE) ?
            native_owner_loaded[i] :
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
#else
        loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
#endif

        if ((loaded == NULL) &&
            (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
        {
            continue;
        }

#if NDS_RENDERER_HW_TRIANGLES
        if ((native_owner_enabled != FALSE) && (loaded != NULL) &&
            (loaded->data != NULL) &&
            (((slot == 0u) && (loaded->asset_id == 0x128u)) ||
             ((slot == 1u) && (loaded->asset_id == 0x139u))) &&
            (loaded->data_size >= sizeof(*dl)) &&
            ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
            (((uintptr_t)dl - (uintptr_t)loaded->data) <=
             (loaded->data_size - sizeof(*dl))))
        {
            native_root_offset =
                (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
            native_root_enabled =
                (native_root_offset == native_owner_root_offsets[i]) ?
                    TRUE : FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_root_enabled != FALSE)
        {
            native_materials =
                sNdsRendererAdapterNativeOwnerMaterials[i];
            native_material_count = native_owner_material_counts[i];
        }
#endif
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        current_state = (detailed_output != FALSE) ? &states[i] :
                                                    &persistent_state;
#else
        current_state = &states[i];
#endif
        current_state->primary_file = loaded;
        current_state->slot = slot;
        if (contract_event != NULL)
        {
            persistent_stats.geometry_mode =
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
                (native_root_enabled != FALSE) ?
                    ndsRendererAdapterNormalizeNativeGeometryMode(
                        contract_event->geometry_mode) :
#endif
                    contract_event->geometry_mode;
            persistent_stats.othermode_h =
                (persistent_stats.othermode_h &
                 ~NDS_FIGHTER_DISPLAY_CYCLETYPE_MASK) |
                contract_event->cycle_type;
            persistent_stats.othermode_l = contract_event->render_mode;
            persistent_stats.prim_color = contract_event->prim_color;
            persistent_stats.env_color = contract_event->env_color;
            if (contract_event->light_valid != 0u)
            {
                persistent_stats.light_dir_x = contract_event->light.l.dir[0];
                persistent_stats.light_dir_y = contract_event->light.l.dir[1];
                persistent_stats.light_dir_z = contract_event->light.l.dir[2];
                persistent_stats.light_dir_mask = 1u;
            }
        }
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawSeedPersistentState(current_state,
                                                &persistent_state);
        }
#else
        ndsFighterDLDrawSeedPersistentState(current_state,
                                            &persistent_state);
#endif
#if NDS_RENDERER_HW_TRIANGLES
        saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        step_start = cpuGetTiming();
#endif
        material_dobj =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                contract_event->material_dobj : collection.dobjs[i];
        if ((sNdsFighterDisplayContractPlayback == FALSE) ||
            (contract_event->material_dobj != NULL))
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            if ((native_root_enabled != FALSE) &&
                (material_dobj != NULL) &&
                (material_dobj->mobj != NULL))
            {
                native_saved_texture_count =
                    ndsRendererAdapterSaveNativeMaterialTextureIds(
                        material_dobj,
                        native_saved_texture_curr,
                        native_saved_texture_next,
                        NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX);
                if (ndsRendererAdapterPrepareNativeMaterials(
                        material_dobj, native_materials,
                        NDS_RENDERER_ADAPTER_NATIVE_MATERIAL_MAX,
                        &native_prepared_material_count) != FALSE)
                {
                    native_material_count =
                        native_prepared_material_count;
                    native_material_built = TRUE;
                }
                else
                {
                    ndsRendererAdapterRestoreNativeMaterialTextureIds(
                        material_dobj,
                        native_saved_texture_curr,
                        native_saved_texture_next,
                        native_saved_texture_count);
                    native_root_enabled = FALSE;
                }
            }
            if (native_material_built != FALSE)
            {
                /* Forensic builds retain the exact resolver-boundary oracle.
                 * Restore the two FRAC fields before its ordinary material
                 * builder so the live MObj advances exactly once. */
                ndsRendererAdapterRestoreNativeMaterialTextureIds(
                    material_dobj,
                    native_saved_texture_curr,
                    native_saved_texture_next,
                    native_saved_texture_count);
            }
            ndsRendererAdapterPrepareMaterialSegment(
                material_dobj, current_state);
#else
            /* The owner preflight already advanced every live MObj once and
             * serialized this root's descriptors. Only the ordinary fallback
             * prepares the material segment in the selected-list loop. */
            if (native_root_enabled == FALSE)
            {
                ndsRendererAdapterPrepareMaterialSegment(
                    material_dobj, current_state);
            }
#endif
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start;
        step_start = cpuGetTiming();
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (native_root_enabled != FALSE)
        {
            initial_projection_ptr = native_owner_projection;
            initial_modelview_ptr = native_owner_modelviews[i];
        }
        else
#endif
        {
            ndsRendererAdapterPrepareInitialMatrices(
                (sNdsFighterDisplayContractPlayback != FALSE) ?
                    contract_event->matrix_dobj : collection.dobjs[i],
                (gGCCurrentCamera != NULL) ?
                    CObjGetStruct(gGCCurrentCamera) : NULL,
                FALSE,
                &initial_projection,
                &initial_projection_ptr,
                &initial_modelview,
                &initial_modelview_ptr);
        }
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
        gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start;
#endif
        config.max_depth = 8u;
        config.max_commands = 2048u;
        config.max_list_commands = 512u;
        config.initial_projection = initial_projection_ptr;
        config.initial_modelview = initial_modelview_ptr;
        config.initial_geometry_mode =
            (sNdsFighterDisplayContractPlayback != FALSE) ?
                contract_event->geometry_mode : 0u;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (native_root_enabled != FALSE)
        {
            config.initial_geometry_mode =
                ndsRendererAdapterNormalizeNativeGeometryMode(
                    config.initial_geometry_mode);
        }
#endif
        config.texture_data_layout =
            NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
        config.validate_range = ndsFighterDLAllDrawValidateRange;
        config.immutable_command_span =
            ndsRendererAdapterImmutableCommandSpan;
        config.resolve_branch = ndsFighterDLAllDrawResolveBranch;
        config.resolve_data = ndsFighterDLDrawResolveRendererData;
        config.user = current_state;

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawResetTransientRendererStats(&persistent_stats);
        }
        else
        {
            ndsFighterDLDrawResetRuntimeRendererStats(&persistent_stats);
        }
        current_stats = &persistent_stats;
#else
        ndsRendererInitStats(&stats[i]);
        ndsFighterDLDrawCopyPersistentRendererState(&stats[i],
                                                    &persistent_stats);
        current_stats = &stats[i];
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererOwnerSnapshotStats(current_stats, &owner_stats_before);
        step_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileSetSourceProvenance(
            0u, i,
            ndsRendererOwnerRootBranchPath(
                loaded, dl, collection.indices[i]));
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (native_owner_failed != FALSE)
        {
            current_stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        }
        else if (native_owner_started != FALSE)
        {
            if ((native_root_enabled == FALSE) ||
                (ndsRendererExecuteNativeFighterRoot(
                     slot, i, loaded->data, native_root_offset,
                     native_materials, native_material_count,
                     &config,
                     (no_oracle != FALSE) ?
                         NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                     current_state,
                     current_stats,
                     &persistent_renderer_vertices) == FALSE))
            {
                /* The compact owner may have submitted earlier runs or
                 * updated its dense slot map. Never replay this root through
                 * the generic cache on top of that partial state. */
                if (current_stats->blocker == NDS_RENDERER_BLOCKER_NONE)
                {
                    current_stats->blocker =
                        NDS_RENDERER_BLOCKER_UNSUPPORTED;
                }
                ndsRendererAbortNativeFighterOwner();
                native_owner_started = FALSE;
                native_owner_failed = TRUE;
            }
        }
        else
        {
            ndsRendererExecuteDisplayListWithVertexCache(
                dl, &config,
                (no_oracle != FALSE) ?
                    NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                current_state, current_stats,
                &persistent_renderer_vertices);
        }
#else
        if ((native_root_enabled == FALSE) ||
            (ndsRendererExecuteNativeFighterRoot(
                 slot, i, loaded->data, native_root_offset,
                 native_materials, native_material_count,
                 &config,
                 (no_oracle != FALSE) ?
                     NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                 current_state,
                 current_stats,
                 &persistent_renderer_vertices) == FALSE))
        {
            ndsRendererExecuteDisplayListWithVertexCache(
                dl, &config,
                (no_oracle != FALSE) ?
                    NULL : ndsFighterMarioFoxVisitDLDrawCommand,
                current_state, current_stats,
                &persistent_renderer_vertices);
        }
#endif
#else
        ndsRendererExecuteDisplayListWithVertexCache(
            dl, &config,
            (no_oracle != FALSE) ?
                NULL : ndsFighterMarioFoxVisitDLDrawCommand,
            current_state, current_stats,
            &persistent_renderer_vertices);
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileDLTicks += cpuGetTiming() - step_start;
        ndsRendererOwnerAccumulateList(
            owner_id, loaded, dl, collection.indices[i],
            initial_projection_ptr, initial_modelview_ptr,
            &config,
            &owner_stats_before, current_stats);
#endif
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawCapturePersistentState(&persistent_state,
                                                   current_state);
        }
#else
        ndsFighterDLDrawCapturePersistentState(&persistent_state,
                                               current_state);
#endif
#if !NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL >= 2)
        ndsFighterDLDrawCopyPersistentRendererState(&persistent_stats,
                                                    current_stats);
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        if (detailed_output != FALSE)
        {
            ndsFighterDLAllDrawAccumulateStats(
                slot, i, collection.indices[i], collection.dobjs[i], dl,
                current_state, current_stats, clean);
        }
        else
        {
            runtime_hardware_triangle_count +=
                current_stats->hardware_triangle_count;
        }
#else
        ndsFighterDLAllDrawAccumulateStats(slot, i, collection.indices[i],
                                           collection.dobjs[i], dl,
                                           current_state, current_stats, clean);
#endif
    }
    }

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (native_owner_started != FALSE)
    {
        if (ndsRendererEndNativeFighterOwner(
                slot, &persistent_stats,
                &persistent_renderer_vertices) == FALSE)
        {
            persistent_stats.blocker =
                NDS_RENDERER_BLOCKER_UNSUPPORTED;
            native_owner_failed = TRUE;
            ndsRendererAbortNativeFighterOwner();
        }
        native_owner_started = FALSE;
    }
    if (native_owner_failed != FALSE)
    {
        u32 failure_index = (collection.selected_count != 0u) ?
            collection.selected_count - 1u : 0u;

        /* Make an unreachable integrity failure visible to both the detailed
         * proof ledger and the null-callback performance verifier. */
        runtime_hardware_triangle_count = 0u;
        if (slot == 0u)
        {
            if (gNdsFighterDLAllDrawP0FirstBlocker == 0u)
            {
                gNdsFighterDLAllDrawP0FirstBlocker =
                    NDS_RENDERER_BLOCKER_UNSUPPORTED;
            }
            gNdsFighterDLAllDrawP0BlockerMask |=
                1u << (failure_index & 31u);
        }
        else
        {
            if (gNdsFighterDLAllDrawP1FirstBlocker == 0u)
            {
                gNdsFighterDLAllDrawP1FirstBlocker =
                    NDS_RENDERER_BLOCKER_UNSUPPORTED;
            }
            gNdsFighterDLAllDrawP1BlockerMask |=
                1u << (failure_index & 31u);
        }
    }
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    gNdsRendererProfileOwners[(u32)owner_id].exit_state_hash =
        ndsRendererOwnerHashRuntimeState(&persistent_stats);
    gNdsRendererProfileOwners[(u32)owner_id].exit_vertex_cache_hash =
        ndsRendererOwnerHashVertexCache(&persistent_renderer_vertices);
    gNdsRendererProfileOwners[(u32)owner_id].exit_resolver_hash =
        ndsRendererOwnerHashResolver(&persistent_state);
    gNdsRendererProfileOwners[(u32)owner_id].exit_global_hash =
        ndsRendererProfileGlobalStateHash();
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (detailed_output == FALSE)
    {
        if (slot == 0u)
        {
            gNdsFighterDLAllDrawP0HardwareTriangleCount +=
                runtime_hardware_triangle_count;
        }
        else
        {
            gNdsFighterDLAllDrawP1HardwareTriangleCount +=
                runtime_hardware_triangle_count;
        }
    }
#endif
    if (pixels != NULL)
    {
        ndsFighterDLAllDrawRasterizeStates(slot, states, clean,
                                           collection.selected_count,
                                           pixels,
                                           pitch);
    }

    root_x_after = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) :
        0u;
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP0GAAfter = (u32)fp->ga;
        gNdsFighterDLAllDrawP0RootXBeforeBits = root_x_before;
        gNdsFighterDLAllDrawP0RootXAfterBits = root_x_after;
    }
    else
    {
        gNdsFighterDLAllDrawP1StatusAfter = (u32)fp->status_id;
        gNdsFighterDLAllDrawP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterDLAllDrawP1GAAfter = (u32)fp->ga;
        gNdsFighterDLAllDrawP1RootXBeforeBits = root_x_before;
        gNdsFighterDLAllDrawP1RootXAfterBits = root_x_after;
    }

    gNdsFighterMarioFoxDLAllDrawCount++;
}

void ndsFighterDisplayContractSubmit(GObj *fighter_gobj)
{
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    extern volatile u32 gNdsVSResultsFighterSubmitCount;
#endif
    FTStruct *fp;
    u32 submitted_before;
    u32 triangles_before;
    u32 triangles_after;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 owner_start;
    NDSRendererProfileOwner owner_id;
#endif

    if (fighter_gobj == NULL)
    {
        return;
    }
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        gNdsVSResultsFighterSubmitCount++;
    }
#endif
    fp = ftGetStruct(fighter_gobj);
    if ((ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
        ((u32)fp->nds_slot > 1u))
    {
        return;
    }
    if (sNdsFighterDisplayContractLastFrame[(u32)fp->nds_slot] ==
        gNdsRendererProfileFrameCount)
    {
        return;
    }
    sNdsFighterDisplayContractLastFrame[(u32)fp->nds_slot] =
        gNdsRendererProfileFrameCount;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    owner_id = ((u32)fp->nds_slot == 0u) ?
        NDS_RENDERER_PROFILE_OWNER_MARIO : NDS_RENDERER_PROFILE_OWNER_FOX;
    owner_start = cpuGetTiming();
#endif
    ndsFighterDisplayContractCapture(fighter_gobj);
    if (sNdsFighterDisplayContract.event_count == 0u)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileOwners[(u32)owner_id].exclusive_ticks +=
            cpuGetTiming() - owner_start;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        ndsRendererBenchmarkSinkEndOwner(owner_id);
#endif
        return;
    }
    submitted_before = gNdsFighterMarioFoxDLAllDrawCount;
    triangles_before =
        gNdsFighterDLAllDrawP0HardwareTriangleCount +
        gNdsFighterDLAllDrawP1HardwareTriangleCount;
    sNdsFighterDisplayContractPlayback = TRUE;
    ndsFighterMarioFoxDLAllDrawForSlot((u32)fp->nds_slot, fp, NULL, 0u);
    sNdsFighterDisplayContractPlayback = FALSE;
    if (gNdsFighterMarioFoxDLAllDrawCount != submitted_before)
    {
        gNdsFighterDisplayContractSubmittedCount +=
            sNdsFighterDisplayContract.event_count;
        gNdsStageGCDrawAllLoopHardwareFighterSubmitCount++;
        triangles_after =
            gNdsFighterDLAllDrawP0HardwareTriangleCount +
            gNdsFighterDLAllDrawP1HardwareTriangleCount;
        if (triangles_after > triangles_before)
        {
            gNdsStageGCDrawAllLoopHardwareFighterTriangleCount +=
                triangles_after - triangles_before;
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileOwners[(u32)owner_id].exclusive_ticks +=
        cpuGetTiming() - owner_start;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    ndsRendererBenchmarkSinkEndOwner(owner_id);
#endif
#else
    (void)fighter_gobj;
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static void ndsFighterDisplayContractSubmitStageFighters(void)
{
    GObj *saved_camera = gGCCurrentCamera;
    u32 slot;

    gGCCurrentCamera = ndsBattleCompatMainCameraGObj();
    for (slot = 0u; slot < 2u; slot++)
    {
        FTStruct *fp = &sNdsFighterStructPool[slot];

        if ((ndsFighterStructIsTrackedPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL))
        {
            continue;
        }
        ndsFighterDisplayContractSubmit(fp->fighter_gobj);
    }
    gGCCurrentCamera = saved_camera;
}
#endif

static void ndsFighterMarioFoxRecordDLAllDrawFromDisplayCallback(
    GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 slot;

    if ((fighter_gobj == NULL) || (sNdsFighterDLAllDrawPixels == NULL))
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if (ndsFighterStructIsTrackedPointer(fp) == FALSE)
    {
        return;
    }

    slot = (u32)fp->nds_slot;
    if (slot > 1u)
    {
        return;
    }

    gNdsFighterDLAllDrawDisplayCallbackCount++;
    if (slot == 0u)
    {
        gNdsFighterDLAllDrawP0DisplayCallbackCount++;
    }
    else
    {
        gNdsFighterDLAllDrawP1DisplayCallbackCount++;
    }

    ndsFighterMarioFoxDLAllDrawForSlot(slot, fp,
                                       sNdsFighterDLAllDrawPixels,
                                       sNdsFighterDLAllDrawPitch);
}

static void ndsFighterMarioFoxRunDLAllDrawProbe(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 pitch = 0u;
    u16 *pixels;
    GObj *saved_camera;

    if ((ndsFighterMarioFoxDLAllDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLAllDrawResult != 0u))
    {
        return;
    }

    if (
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
        (gNdsFighterMarioFoxDLMultiDrawCount == 2u) &&
        (gNdsFighterDLMultiDrawP0CandidateCount > 0u) &&
        (gNdsFighterDLMultiDrawP1CandidateCount > 0u)
#else
        (gNdsFighterMarioFoxDLMultiDrawResult ==
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_PASS) &&
        (gNdsFighterMarioFoxDLMultiDrawSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_MULTI_DRAW_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLMultiDrawMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLMultiDrawCount == 2u) &&
        (gNdsFighterDLMultiDrawP0CandidateCount == 14u) &&
        (gNdsFighterDLMultiDrawP1CandidateCount == 18u) &&
        (gNdsFighterDLMultiDrawP0SelectedCount == 4u) &&
        (gNdsFighterDLMultiDrawP1SelectedCount == 4u) &&
        (gNdsFighterDLMultiDrawP0CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP1CleanCount == 4u) &&
        (gNdsFighterDLMultiDrawP0FailedCount == 0u) &&
        (gNdsFighterDLMultiDrawP1FailedCount == 0u)
#endif
        )
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxDLAllDrawMask = mask;
        return;
    }

    gNdsFighterDLAllDrawPreviewCommitBefore =
        gNdsOriginalDLPreviewCommitCount;
    ndsFighterDLAllDrawResetFailureDiagnostics();
    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_DL_ALL_DRAW_WIDTH,
        NDS_FIGHTER_DL_ALL_DRAW_HEIGHT,
        &pitch);
    if (pixels != NULL)
    {
        gNdsFighterDLAllDrawPreviewWidth = NDS_FIGHTER_DL_ALL_DRAW_WIDTH;
        gNdsFighterDLAllDrawPreviewHeight = NDS_FIGHTER_DL_ALL_DRAW_HEIGHT;
        gNdsFighterDLAllDrawPreviewPitch = pitch;
        mask |= 1u << 1;
    }
    else
    {
        gNdsFighterMarioFoxDLAllDrawMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();
    sNdsFighterDLAllDrawProbeActive = TRUE;
    sNdsFighterDLAllDrawPixels = pixels;
    sNdsFighterDLAllDrawPitch = pitch;
    saved_camera = gGCCurrentCamera;
    gGCCurrentCamera = ndsBattleCompatMainCameraGObj();
    ftDisplayMainProcDisplay(ndsFighterMarioFoxProofGObjForSlot(0u));
    ftDisplayMainProcDisplay(ndsFighterMarioFoxProofGObjForSlot(1u));
    gGCCurrentCamera = saved_camera;
    sNdsFighterDLAllDrawProbeActive = FALSE;
    sNdsFighterDLAllDrawPixels = NULL;
    sNdsFighterDLAllDrawPitch = 0u;

    if ((gNdsFighterDLAllDrawDisplayCallbackCount == 2u) &&
        (gNdsFighterDLAllDrawP0DisplayCallbackCount == 1u) &&
        (gNdsFighterDLAllDrawP1DisplayCallbackCount == 1u) &&
        (gNdsFighterMarioFoxDLAllDrawCount == 2u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDLAllDrawP0CandidateCount == 14u) &&
        (gNdsFighterDLAllDrawP1CandidateCount == 18u) &&
        (gNdsFighterDLAllDrawP0SelectedCount == 14u) &&
        (gNdsFighterDLAllDrawP1SelectedCount == 18u) &&
        (gNdsFighterDLAllDrawP0SelectedIndexMask != 0u) &&
        (gNdsFighterDLAllDrawP1SelectedIndexMask != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDLAllDrawP0AttemptCount == 14u) &&
        (gNdsFighterDLAllDrawP1AttemptCount == 18u) &&
        (gNdsFighterDLAllDrawP0CleanCount == 14u) &&
        (gNdsFighterDLAllDrawP1CleanCount == 18u) &&
        (gNdsFighterDLAllDrawP0DrawnDObjCount ==
            gNdsFighterDLAllDrawP0CleanCount) &&
        (gNdsFighterDLAllDrawP1DrawnDObjCount ==
            gNdsFighterDLAllDrawP1CleanCount) &&
        (gNdsFighterDLAllDrawP0FailedCount == 0u) &&
        (gNdsFighterDLAllDrawP1FailedCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDLAllDrawP0VertexDecodedCount >
            gNdsFighterDLMultiDrawP0VertexDecodedCount) &&
        (gNdsFighterDLAllDrawP1VertexDecodedCount >
            gNdsFighterDLMultiDrawP1VertexDecodedCount) &&
        (gNdsFighterDLAllDrawP0TriangleCount >
            gNdsFighterDLMultiDrawP0TriangleCount) &&
        (gNdsFighterDLAllDrawP1TriangleCount >
            gNdsFighterDLMultiDrawP1TriangleCount) &&
        (gNdsFighterDLAllDrawP0TriangleValidCount >
            gNdsFighterDLMultiDrawP0TriangleValidCount) &&
        (gNdsFighterDLAllDrawP1TriangleValidCount >
            gNdsFighterDLMultiDrawP1TriangleValidCount) &&
        (gNdsFighterDLAllDrawP0RealTriangleDrawnCount >
            gNdsFighterDLMultiDrawP0RealTriangleDrawnCount) &&
        (gNdsFighterDLAllDrawP1RealTriangleDrawnCount >
            gNdsFighterDLMultiDrawP1RealTriangleDrawnCount))
    {
        mask |= 1u << 5;
    }

    gNdsFighterDLAllDrawTotalPixelCount =
        gNdsFighterDLAllDrawP0PixelCount +
        gNdsFighterDLAllDrawP1PixelCount;
    if ((gNdsFighterDLAllDrawP0PixelCount >=
            gNdsFighterDLMultiDrawP0PixelCount) &&
        (gNdsFighterDLAllDrawP1PixelCount >=
            gNdsFighterDLMultiDrawP1PixelCount) &&
        (gNdsFighterDLAllDrawTotalPixelCount >=
            gNdsFighterDLMultiDrawTotalPixelCount) &&
        (gNdsFighterDLAllDrawP0ColorChecksum != 0u) &&
        (gNdsFighterDLAllDrawP1ColorChecksum != 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterDLAllDrawTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterDLAllDrawPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterDLAllDrawPreviewCommitDelta =
            gNdsFighterDLAllDrawPreviewCommitAfter -
            gNdsFighterDLAllDrawPreviewCommitBefore;
        gNdsFighterDLAllDrawPreviewReady = gNdsOriginalDLPreviewReady;
    }
    if ((gNdsFighterDLAllDrawPreviewReady != 0u) &&
        (gNdsFighterDLAllDrawPreviewCommitDelta == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDLAllDrawP0FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLAllDrawP1FirstBlocker ==
            NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLAllDrawP0BlockerMask == 0u) &&
        (gNdsFighterDLAllDrawP1BlockerMask == 0u) &&
        (gNdsFighterDLAllDrawP0UnsupportedOpcode == 0u) &&
        (gNdsFighterDLAllDrawP1UnsupportedOpcode == 0u) &&
        (gNdsFighterDLAllDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLAllDrawP1UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLAllDrawRangeRejectCount == 0u) &&
        (gNdsFighterDLAllDrawVertexRangeRejectCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDLAllDrawGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDLAllDrawP0StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLAllDrawP1StatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterDLAllDrawP0MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLAllDrawP1MotionAfter ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterDLAllDrawP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLAllDrawP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterDLAllDrawP0RootXBeforeBits ==
            gNdsFighterDLAllDrawP0RootXAfterBits) &&
        (gNdsFighterDLAllDrawP1RootXBeforeBits ==
            gNdsFighterDLAllDrawP1RootXAfterBits) &&
        (gNdsFighterDLAllDrawGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDLAllDrawDrawCallCount == 0u) &&
        (gNdsFighterDLAllDrawMatrixCallCount == 0u) &&
        (gNdsFighterDLAllDrawGameplayUpdateCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxDLAllDrawMask = mask;
    gNdsFighterMarioFoxDLAllDrawDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxDLAllDrawResult =
            NDS_FIGHTER_MARIOFOX_DL_ALL_DRAW_PASS;
        gNdsFighterMarioFoxDLAllDrawSafeResult =
            NDS_FIGHTER_MARIOFOX_DL_ALL_DRAW_SAFE_PASS;
    }
}
