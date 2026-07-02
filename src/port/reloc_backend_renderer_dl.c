#include <sys/matrix.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#define NDS_RENDERER_ADAPTER_MTX_FRAC_BITS 12
#define NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX 32u
#define NDS_RENDERER_ADAPTER_HW_WORLD_SCALE 256.0F

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

static void ndsRendererAdapterMtxFromN64(
    const Mtx *src, NDSRendererMatrix20p12 *dst)
{
    if ((src == NULL) || (dst == NULL))
    {
        return;
    }
    ndsRendererMtxLoadN64ToDS20p12(src, dst);
}

static f32 ndsRendererAdapterProjectionDepth(f32 value)
{
#if NDS_RENDERER_HW_TRIANGLES
    return value / NDS_RENDERER_ADAPTER_HW_WORLD_SCALE;
#else
    return value;
#endif
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

static sb32 ndsRendererAdapterBuildDObjWorldMatrix(
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
        if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u],
                                                   &local) != FALSE)
        {
            ndsRendererMtxMul20p12(out, &local, out);
        }
    }
    return TRUE;
}

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
        case nGCMatrixKindPerspFastF:
            syMatrixPerspFast(&mtx, &cobj->projection.persp.norm,
                              cobj->projection.persp.fovy,
                              cobj->projection.persp.aspect,
                              ndsRendererAdapterProjectionDepth(
                                  cobj->projection.persp.near),
                              ndsRendererAdapterProjectionDepth(
                                  cobj->projection.persp.far),
                              cobj->projection.persp.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case nGCMatrixKindPerspF:
            syMatrixPersp(&mtx, &cobj->projection.persp.norm,
                          cobj->projection.persp.fovy,
                          cobj->projection.persp.aspect,
                          ndsRendererAdapterProjectionDepth(
                              cobj->projection.persp.near),
                          ndsRendererAdapterProjectionDepth(
                              cobj->projection.persp.far),
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
                          ndsRendererAdapterProjectionDepth(
                              cobj->projection.ortho.n),
                          ndsRendererAdapterProjectionDepth(
                              cobj->projection.ortho.f),
                          cobj->projection.ortho.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case 6:
        case 7:
            syMatrixLookAt(&mtx, cobj->vec.eye.x, cobj->vec.eye.y,
                           cobj->vec.eye.z, cobj->vec.at.x,
                           cobj->vec.at.y, cobj->vec.at.z,
                           cobj->vec.up.x, cobj->vec.up.y,
                           cobj->vec.up.z);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 6)
            {
                ndsRendererAdapterMulInto(projection, &incoming,
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
            syMatrixModLookAt(&mtx, cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 1.0F, 0.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 8)
            {
                ndsRendererAdapterMulInto(projection, &incoming,
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
            syMatrixModLookAt(&mtx, cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 0.0F, 1.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 10)
            {
                ndsRendererAdapterMulInto(projection, &incoming,
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
                      ndsRendererAdapterProjectionDepth(256.0F),
                      ndsRendererAdapterProjectionDepth(39936.0F),
                      1.0F);
    ndsRendererAdapterMtxFromN64(&mtx, projection);
    *projection_valid = TRUE;

    syMatrixLookAt(&mtx,
                   0.0F, 300.0F, 10000.0F,
                   0.0F, 300.0F, 0.0F,
                   0.0F, 1.0F, 0.0F);
    ndsRendererAdapterMtxFromN64(&mtx, modelview);
    *modelview_valid = TRUE;
}
#endif

static void ndsRendererAdapterPrepareInitialMatrices(
    DObj *dobj,
    CObj *cobj,
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

    ndsRendererAdapterBuildCameraMatrices(cobj, &camera_projection,
                                          &camera_projection_valid,
                                          &camera_modelview,
                                          &camera_modelview_valid);
#if NDS_RENDERER_HW_TRIANGLES
    if ((camera_projection_valid == FALSE) &&
        (camera_modelview_valid == FALSE))
    {
        ndsRendererAdapterBuildDefaultBattleCameraMatrices(
            &camera_projection, &camera_projection_valid,
            &camera_modelview, &camera_modelview_valid);
    }
#endif
    if (dobj != NULL)
    {
        dobj_world_valid =
            ndsRendererAdapterBuildDObjWorldMatrix(dobj, &dobj_world);
    }

    if (camera_projection_valid != FALSE)
    {
        *projection = camera_projection;
        *projection_ptr = projection;
    }

    if ((camera_modelview_valid != FALSE) && (dobj_world_valid != FALSE))
    {
        ndsRendererMtxMul20p12(&camera_modelview, &dobj_world, modelview);
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

static s32 ndsFighterDLScanValidateRange(const Gfx *dl, size_t bytes,
                                         void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE)))
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
    config.validate_range = ndsFighterDLScanValidateRange;
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
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE)))
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
        return TRUE;

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
    config.validate_range = ndsFighterDLExecValidateRange;
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
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE)))
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
    u32 root_x_before;
    u32 root_x_after;
    u32 unused_index;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
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
    ndsRendererAdapterPrepareInitialMatrices(selected,
                                             (gGCCurrentCamera != NULL) ?
                                                 CObjGetStruct(
                                                     gGCCurrentCamera) :
                                                 NULL,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);

    config.max_depth = 8u;
    config.max_commands = 2048u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.validate_range = ndsFighterDLDrawValidateRange;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;

    ndsRendererInitStats(&stats);
    ndsRendererExecuteDisplayList(dl,
                                  &config,
                                  ndsFighterMarioFoxVisitDLDrawCommand,
                                  &state,
                                  &stats);
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
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE)))
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
    NDSRendererStats stats[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u8 clean[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 root_x_before;
    u32 root_x_after;
    u32 i;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
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
    bzero(stats, sizeof(stats));
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

        if ((loaded == NULL) &&
            (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
        {
            continue;
        }

        states[i].primary_file = loaded;
        states[i].slot = slot;
        ndsRendererAdapterPrepareInitialMatrices(collection.dobjs[i],
                                                 (gGCCurrentCamera != NULL) ?
                                                     CObjGetStruct(
                                                         gGCCurrentCamera) :
                                                     NULL,
                                                 &initial_projection,
                                                 &initial_projection_ptr,
                                                 &initial_modelview,
                                                 &initial_modelview_ptr);
        config.max_depth = 8u;
        config.max_commands = 2048u;
        config.max_list_commands = 512u;
        config.initial_projection = initial_projection_ptr;
        config.initial_modelview = initial_modelview_ptr;
        config.validate_range = ndsFighterDLMultiDrawValidateRange;
        config.resolve_branch = ndsFighterDLMultiDrawResolveBranch;
        config.resolve_data = ndsFighterDLDrawResolveRendererData;
        config.user = &states[i];

        ndsRendererInitStats(&stats[i]);
        ndsRendererExecuteDisplayList(dl,
                                      &config,
                                      ndsFighterMarioFoxVisitDLDrawCommand,
                                      &states[i],
                                      &stats[i]);
        ndsFighterDLMultiDrawAccumulateStats(slot, i, &states[i],
                                             &stats[i], clean);
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

    if ((gNdsFighterMarioFoxDLDrawResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_PASS) &&
        (gNdsFighterMarioFoxDLDrawSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_DRAW_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLDrawMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLDrawCount == 2u) &&
        (gNdsFighterDLDrawP0Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP1Blocker == NDS_RENDERER_BLOCKER_NONE) &&
        (gNdsFighterDLDrawP0UnsupportedCommandCount == 0u) &&
        (gNdsFighterDLDrawP1UnsupportedCommandCount == 0u))
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
    ndsFighterMarioFoxDLMultiDrawForSlot(0u, &sNdsFighterStructPool[0],
                                         pixels, pitch);
    ndsFighterMarioFoxDLMultiDrawForSlot(1u, &sNdsFighterStructPool[1],
                                         pixels, pitch);
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

static void ndsFighterCollectAllDObjsWithDL(
    DObj *root, NDSFighterDLAllDrawCollection *collection)
{
    u32 traversal_index = 0u;

    if (collection == NULL)
    {
        return;
    }
    bzero(collection, sizeof(*collection));
    ndsFighterCollectAllDObjsWithDLRecursive(root, collection,
                                             &traversal_index);
}

static s32 ndsFighterDLAllDrawValidateRange(const Gfx *dl, size_t bytes,
                                            void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE)))
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
    NDSRendererStats *stats;
    u8 *clean;
    u32 root_x_before;
    u32 root_x_after;
    u32 i;

    if ((slot > 1u) || (pixels == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
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
    stats = sNdsFighterDLAllDrawStats[slot];
    clean = sNdsFighterDLAllDrawClean[slot];
    bzero(states, sizeof(sNdsFighterDLAllDrawStates[slot]));
    bzero(stats, sizeof(sNdsFighterDLAllDrawStats[slot]));
    bzero(clean, sizeof(sNdsFighterDLAllDrawClean[slot]));

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

        if ((loaded == NULL) &&
            (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
        {
            continue;
        }

        states[i].primary_file = loaded;
        states[i].slot = slot;
        ndsRendererAdapterPrepareInitialMatrices(collection.dobjs[i],
                                                 (gGCCurrentCamera != NULL) ?
                                                     CObjGetStruct(
                                                         gGCCurrentCamera) :
                                                     NULL,
                                                 &initial_projection,
                                                 &initial_projection_ptr,
                                                 &initial_modelview,
                                                 &initial_modelview_ptr);
        config.max_depth = 8u;
        config.max_commands = 2048u;
        config.max_list_commands = 512u;
        config.initial_projection = initial_projection_ptr;
        config.initial_modelview = initial_modelview_ptr;
        config.validate_range = ndsFighterDLAllDrawValidateRange;
        config.resolve_branch = ndsFighterDLAllDrawResolveBranch;
        config.resolve_data = ndsFighterDLDrawResolveRendererData;
        config.user = &states[i];

        ndsRendererInitStats(&stats[i]);
        ndsRendererExecuteDisplayList(dl,
                                      &config,
                                      ndsFighterMarioFoxVisitDLDrawCommand,
                                      &states[i],
                                      &stats[i]);
        ndsFighterDLAllDrawAccumulateStats(slot, i, collection.indices[i],
                                           collection.dobjs[i], dl,
                                           &states[i], &stats[i], clean);
    }

    ndsFighterDLAllDrawRasterizeStates(slot, states, clean,
                                       collection.selected_count,
                                       pixels,
                                       pitch);

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
    if (ndsFighterStructIsPoolPointer(fp) == FALSE)
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

    if ((ndsFighterMarioFoxDLAllDrawProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDLAllDrawResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDLMultiDrawResult ==
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
        (gNdsFighterDLMultiDrawP1FailedCount == 0u))
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
    ftDisplayMainProcDisplay(sNdsFighterStructPool[0].fighter_gobj);
    ftDisplayMainProcDisplay(sNdsFighterStructPool[1].fighter_gobj);
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

