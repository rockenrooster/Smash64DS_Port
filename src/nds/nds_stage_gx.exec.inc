/* Phase 2: native GX templates for all eight Dream Land segments.
 * Included after the stage helpers. No N64 decoder or runtime GX recorder. */
#include <nds/nds_stage_gx.h>

_Static_assert(sizeof(NDSStageGxHeader) == 48, "stage GX header ABI");
_Static_assert(sizeof(NDSStageGxRun) == 32, "stage GX run ABI");
_Static_assert(sizeof(NDSStageGxPatch) == 8, "stage GX patch ABI");

volatile u32 gNdsP2StageProgDraws, gNdsP2StageProgWords;
volatile u32 gNdsP2StageProgLoads, gNdsP2StageProgBytes;
volatile u32 gNdsP2StageProgDeclines, gNdsP2StageProgReason;
volatile u32 gNdsP2StageProgNearRuns;
volatile u32 gNdsP2StageProgDmas;
static u32 sNdsStageGxGeneration, sNdsStageGxAttempted;
static u64 sNdsStageGxStaticWorld;
static NDSStageGxRun *sNdsStageGxRuns;
static NDSStageGxPatch *sNdsStageGxPatches;
static u32 *sNdsStageGxWords;
/* Only the unsent span; ownership of DMA0 uses the existing renderer fence. */
static u32 sNdsStageGxFirstWord, sNdsStageGxWordCount, sNdsStageGxAlpha;

static u32 ndsStageGxHashBytes(const void *data, u32 bytes, u32 hash)
{
    const u8 *p = data;
    while (bytes-- != 0u) { hash = (hash ^ *p++) * 16777619u; }
    return hash;
}

static u32 ndsStageGxSourceHash(void)
{
    u32 hash = 2166136261u, i;
    u32 counts[] = { NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT,
                     NDS_NATIVE_STAGE_RUN_COUNT, NDS_NATIVE_STAGE_CORNER_COUNT };
    hash = ndsStageGxHashBytes(counts, sizeof(counts), hash);
    for (i = 0u; i < NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT; i++)
    {
        const NDSNativeStageDenseVertex *v = &sNdsNativeStageVertices[i];
        u32 row[] = { (u32)(s32)v->x, (u32)(s32)v->y, (u32)(s32)v->z,
                      v->matrix_binding, v->packed_cache_shift >> NDS_NATIVE_STAGE_COORDINATE_SHIFT };
        hash = ndsStageGxHashBytes(row, sizeof(row), hash);
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_CORNER_COUNT; i++)
    {
        u32 corner = sNdsNativeStageCorners[i];
        hash = ndsStageGxHashBytes(&corner, sizeof(corner), hash);
    }
    return hash;
}

static sb32 ndsStageGxValidate(const NDSStageGxHeader *h, const u8 *body)
{
    const NDSStageGxRun *runs = (const NDSStageGxRun *)body;
    const NDSStageGxPatch *patches = (const NDSStageGxPatch *)(runs + h->run_count);
    const u32 *words = (const u32 *)(patches + h->patch_count);
    u32 i;
    u64 static_world = (u64)h->static_world_lo | ((u64)h->static_world_hi << 32);
    for (i = 0u; i < h->run_count; i++)
    {
        const NDSStageGxRun *r = &runs[i];
        u32 segment, wanted = FALSE, p, cursor, end, depth = 0u, vertices = 0u;
        u64 binding_mask = 0u;
        for (segment = 0u; segment < 8u; segment++)
        {
            const NDSNativeStageSegment *s = &sNdsNativeStageSegments[segment];
            if ((h->segment_mask & (1u << segment)) != 0u &&
                i >= s->first_run && i - s->first_run < s->run_count)
            { wanted = TRUE; break; }
        }
        if (wanted == FALSE) { if (r->word_count != 0u) { return FALSE; } continue; }
        if (r->word_count < 4u || r->segment != segment ||
            r->triangles != sNdsNativeStageRuns[i].triangle_count ||
            (u32)r->first_word + r->word_count > h->word_count ||
            (u32)r->first_patch + r->patch_count > h->patch_count) { return FALSE; }
        if (words[r->first_word] != 0x002B2A29u) { return FALSE; }
        /* The bounds admit the GPU path only when the entire run is inside.
         * Refuse a producer whose bounds omit any source corner. */
        for (p = 0u; p < (u32)r->triangles * 3u; p++)
        {
            const NDSNativeStageDenseVertex *v = &sNdsNativeStageVertices[
                sNdsNativeStageCorners[sNdsNativeStageRuns[i].first_corner + p]];
            s16 xyz[] = { v->x, v->y, v->z };
            u32 axis;
            binding_mask |= (u64)1u << v->matrix_binding;
            for (axis = 0u; axis < 3u; axis++)
            {
                if (xyz[axis] < r->minimum[axis] || xyz[axis] > r->maximum[axis]) { return FALSE; }
            }
        }
        if (binding_mask != ((u64)r->binding_lo | ((u64)r->binding_hi << 32))) { return FALSE; }
        p = r->first_patch; cursor = r->first_word; end = cursor + r->word_count;
        while (cursor < end)
        {
            u32 ops = words[cursor++], lane;
            for (lane = 0u; lane < 4u; lane++)
            {
                u32 op = (ops >> (lane * 8u)) & 255u, n, kind = 0u;
                switch (op)
                {
                case 0: case 0x15: case 0x41: n = 0u; break;
                case 0x11: n = 0u; if (++depth > 1u) { return FALSE; } break;
                case 0x12: n = 1u; if (depth-- != 1u) { return FALSE; } break;
                case 0x10: case 0x40: n = 1u; break;
                case 0x16: n = 16u; kind = NDS_STAGE_GX_VIEW; break;
                case 0x18: n = 16u; kind = NDS_STAGE_GX_WORLD; break;
                case 0x20: n = 1u; kind = NDS_STAGE_GX_COLOR; break;
                case 0x22: n = 1u; kind = NDS_STAGE_GX_UV; break;
                case 0x23: n = 2u; vertices++; if (depth != 1u) { return FALSE; } break;
                case 0x29: n = 1u; kind = NDS_STAGE_GX_MATERIAL;
                    if (cursor != (u32)r->first_word + 1u) { return FALSE; } break;
                case 0x2A: n = 1u;
                    if (cursor != (u32)r->first_word + 2u) { return FALSE; } break;
                case 0x2B: n = 1u;
                    if (cursor != (u32)r->first_word + 3u) { return FALSE; } break;
                default: return FALSE;
                }
                if (cursor + n > end || (op == 0x12 && words[cursor] != 1u) ||
                    (op == 0x10 && words[cursor] != 0u && words[cursor] != 2u) ||
                    (op == 0x40 && words[cursor] != 0u)) { return FALSE; }
                if (op == 0x18 && (static_world & ((u64)1u << sNdsNativeStageRuns[i].binding_index)) != 0u)
                {
                    const NDSNativeStageRun *native = &sNdsNativeStageRuns[i];
                    u32 shift;
                    m4x4 expected;
                    if (vertices >= (u32)r->triangles * 3u || depth != 1u) { return FALSE; }
                    shift = native->submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX ? 0u :
                        native->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX ?
                        sNdsNativeStageOwnerExecution.runs[i].coordinate_shift :
                        ndsRendererNativeStageTask36TriangleShift(native, vertices / 3u);
                    if (!ndsRendererNativeStageTask36BuildWorld(native->binding_index, shift, &expected) ||
                        memcmp(&words[cursor], &expected, sizeof(expected)) != 0) { return FALSE; }
                    kind = 0u;
                }
                if ((kind == NDS_STAGE_GX_COLOR || kind == NDS_STAGE_GX_UV) &&
                    (p == (u32)r->first_patch + r->patch_count || patches[p].word != cursor))
                {
                    const NDSNativeStagePreparedDense *v;
                    u32 expected;
                    if (vertices >= (u32)r->triangles * 3u) { return FALSE; }
                    v = &sNdsNativeStagePreparedDense[sNdsNativeStageCorners[
                        sNdsNativeStageRuns[i].first_corner + vertices]];
                    expected = kind == NDS_STAGE_GX_COLOR ? v->packed_color :
                        (u32)(u16)v->s | ((u32)(u16)v->t << 16);
                    if (words[cursor] != expected) { return FALSE; }
                    kind = 0u;
                }
                if (kind != 0u)
                {
                    const NDSStageGxPatch *q;
                    if (p >= (u32)r->first_patch + r->patch_count) { return FALSE; }
                    q = &patches[p++];
                    if (q->word != cursor ||
                        (q->kind != kind && !(op == 0x16 &&
                         (q->kind == NDS_STAGE_GX_NOZ || q->kind == NDS_STAGE_GX_PROJECTION ||
                          q->kind == NDS_STAGE_GX_COMPOSED_NOZ || q->kind == NDS_STAGE_GX_CORNER_NOZ))) ||
                        ((q->kind == NDS_STAGE_GX_WORLD || q->kind == NDS_STAGE_GX_COMPOSED_NOZ) &&
                         (q->index >= NDS_NATIVE_STAGE_BINDING_COUNT || q->aux > 6u)) ||
                        (q->kind == NDS_STAGE_GX_CORNER_NOZ &&
                         (q->index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT ||
                          (q->aux & 7u) > 6u || (q->aux >> 3) >= r->triangles)) ||
                        ((q->kind == NDS_STAGE_GX_COLOR || q->kind == NDS_STAGE_GX_UV) &&
                         q->index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) ||
                        (q->kind == NDS_STAGE_GX_MATERIAL && (q->index != i || q->aux != 0u)))
                    { return FALSE; }
                }
                cursor += n;
            }
        }
        if (depth != 0u || vertices != (u32)r->triangles * 3u ||
            p != (u32)r->first_patch + r->patch_count) { return FALSE; }
    }
    return TRUE;
}

static sb32 __attribute__((noinline, cold, optimize("Os"))) ndsStageGxLoad(void)
{
    NdsRelocAssetStream stream = { NULL };
    NDSStageGxHeader h;
    u8 *body;
    u32 reason = 1u;
    if (sNdsStageGxGeneration != gNdsTaskmanHeapGeneration)
    {
        sNdsStageGxGeneration = gNdsTaskmanHeapGeneration;
        sNdsStageGxRuns = NULL; sNdsStageGxAttempted = FALSE;
    }
    if (sNdsStageGxRuns != NULL) { return TRUE; }
    if (sNdsStageGxAttempted != FALSE) { return FALSE; }
    sNdsStageGxAttempted = TRUE;
    if (!ndsRelocAssetStreamOpen(&stream, "nitro:/stages/dreamland.gxp") ||
        !ndsRelocAssetStreamRead(&stream, 0u, &h, sizeof(h))) { goto failed; }
    reason = 2u;
    if (h.magic != 0x31505847u || h.version != 4u || h.gkind != NDS_NATIVE_STAGE_GKIND_PUPUPU ||
        h.run_count != NDS_NATIVE_STAGE_RUN_COUNT || h.word_count == 0u || h.word_count > 16384u ||
        h.patch_count > 4096u || h.segment_mask != 0xffu ||
        (((u64)h.static_world_lo | ((u64)h.static_world_hi << 32)) &
         ~sNdsNativeStageOwnerExecution.rigid_binding_mask) != 0u ||
        h.body_bytes != h.run_count * sizeof(NDSStageGxRun) + h.patch_count * 8u + h.word_count * 4u ||
        h.source_hash != ndsStageGxSourceHash()) { goto failed; }
    reason = 3u;
    if ((uintptr_t)gSYTaskmanGeneralHeap.end < (uintptr_t)gSYTaskmanGeneralHeap.ptr ||
        (uintptr_t)gSYTaskmanGeneralHeap.end - (uintptr_t)gSYTaskmanGeneralHeap.ptr < h.body_bytes + 25632u)
    { goto failed; }
    body = syTaskmanMalloc(h.body_bytes, 32u);
    if (body == NULL) { goto failed; }
    reason = 4u;
    if (!ndsRelocAssetStreamRead(&stream, sizeof(h), body, h.body_bytes) ||
        ndsStageGxHashBytes(body, h.body_bytes, 2166136261u) != h.body_hash ||
        !ndsStageGxValidate(&h, body)) { goto failed; }
    ndsRelocAssetStreamClose(&stream);
    sNdsStageGxRuns = (NDSStageGxRun *)body;
    sNdsStageGxPatches = (NDSStageGxPatch *)(sNdsStageGxRuns + h.run_count);
    sNdsStageGxWords = (u32 *)(sNdsStageGxPatches + h.patch_count);
    sNdsStageGxStaticWorld = (u64)h.static_world_lo | ((u64)h.static_world_hi << 32);
    gNdsP2StageProgLoads++; gNdsP2StageProgBytes = h.body_bytes;
    return TRUE;
failed:
    ndsRelocAssetStreamClose(&stream);
    gNdsP2StageProgReason = reason;
    return FALSE;
}

static sb32 ndsStageGxEligible(u32 segment)
{
    if (sNdsNativeStagePacketActive->gkind != NDS_NATIVE_STAGE_GKIND_PUPUPU ||
        segment >= 8u) { return FALSE; }
    if (!ndsStageGxLoad()) { gNdsP2StageProgDeclines++; return FALSE; }
    if ((sNdsStageGxStaticWorld & ~sNdsNativeStageOwnerExecution.rigid_binding_mask) != 0u)
    { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 7u; return FALSE; }
    return TRUE;
}

static sb32 ndsStageGxInsideNear(const NDSStageGxRun *run, const NDSRendererMatrix20p12 *matrix)
{
    s64 z = matrix->m[3][2], w = matrix->m[3][3];
    u32 axis;
    /* Independent minima are conservative, including the CPU clipper's s32
     * saturation. Six multiplies replace a per-vertex test on the common path. */
    for (axis = 0u; axis < 3u; axis++)
    {
        s32 cz = matrix->m[axis][2], cw = matrix->m[axis][3];
        z += (s64)cz * (cz >= 0 ? run->minimum[axis] : run->maximum[axis]);
        w += (s64)cw * (cw >= 0 ? run->minimum[axis] : run->maximum[axis]);
    }
    return w > 0 && (s64)ndsRendererClampS64ToS32(z) + ndsRendererClampS64ToS32(w) >= 0;
}

static sb32 ndsStageGxMaterial(u32 *dst, const NDSNativeStagePreparedRun *run,
                             u32 submit_class, u32 owner)
{
    u32 poly = run->poly_fmt, texture = 0u, palette = 0u;
    if (run->textured != 0u)
    {
        const DynamicArray *textures = &glGlobalData.texturePtrs;
        const DynamicArray *palettes = &glGlobalData.palettePtrs;
        const gl_texture_data *tex;
        if (run->texture_name == 0u || run->texture_name >= textures->cur_size ||
            (tex = textures->data[run->texture_name]) == NULL) { return FALSE; }
        /* The same immutable address/format and mutable sampler split as
         * glTexParameter. The current whole-table proof owns the texture. */
        texture = (tex->texFormat & ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK) |
                  (run->texture_params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK);
        if (tex->palIndex != 0)
        {
            const gl_palette_data *pal;
            if ((u32)tex->palIndex >= palettes->cur_size ||
                (pal = palettes->data[tex->palIndex]) == NULL) { return FALSE; }
            /* libnds GL_COLOR_TABLE_FORMAT_EXT returns this encoded word. */
            palette = pal->addr;
        }
    }
    if (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z &&
        owner < NDS_RENDERER_NATIVE_STAGE_STATIC_OWNER_COUNT &&
        (poly & POLY_CULL_NONE) == POLY_CULL_NONE)
    { poly = (poly & ~POLY_CULL_NONE) | POLY_CULL_BACK; }
    dst[0] = poly; dst[1] = texture; dst[2] = palette;
    return TRUE;
}

static void ndsStageGxFlush(void)
{
    if (sNdsStageGxWordCount == 0u) { return; }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    DC_FlushRange(&sNdsStageGxWords[sNdsStageGxFirstWord], sNdsStageGxWordCount * 4u);
    DMA_SRC(0) = (u32)(uintptr_t)&sNdsStageGxWords[sNdsStageGxFirstWord];
    DMA_DEST(0) = (u32)(uintptr_t)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | sNdsStageGxWordCount;
    sNdsFighterPacketDmaPending = 1u;
    sNdsStageGxWordCount = 0u;
    gNdsP2StageProgDmas++;
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsR2GxLastProjection = NULL;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareTriangleBatchOpen = FALSE;
}

static void ndsStageGxAppend(const NDSStageGxRun *run,
                           const NDSNativeStagePreparedRun *prepared)
{
    u32 alpha = prepared->alpha_test != 0u ? 0x100u | prepared->alpha_ref : 0u;
    if (sNdsStageGxWordCount != 0u &&
        (run->first_word != sNdsStageGxFirstWord + sNdsStageGxWordCount ||
         alpha != sNdsStageGxAlpha))
    { ndsStageGxFlush(); }
    if (sNdsStageGxWordCount == 0u)
    {
        /* Alpha-test registers are outside the FIFO. Drain the previous span
         * before changing them; never merge across a hidden run or state edge. */
        NDS_FIGHTER_PACKET_DMA_WAIT();
        ndsRendererNativeStageTask36EndSegment();
        ndsRendererHardwareEndBatch();
        glEnable(GL_TEXTURE_2D);
        if (alpha != 0u) { glEnable(GL_ALPHA_TEST); glAlphaFunc(prepared->alpha_ref); }
        else { glDisable(GL_ALPHA_TEST); }
        glDisable(GL_FOG);
        sNdsStageGxFirstWord = run->first_word;
        sNdsStageGxAlpha = alpha;
    }
    sNdsStageGxWordCount += run->word_count;
}

/* Keep the patcher out of the nearly full ITCM executor during coexistence. */
static sb32 __attribute__((noinline)) ndsStageGxDraw(u32 index, u32 owner, NDSRendererStats *stats)
{
    const NDSStageGxRun *r = &sNdsStageGxRuns[index];
    const NDSNativeStageRun *native = &sNdsNativeStageRuns[index];
    const NDSNativeStagePreparedRun *prepared = &sNdsNativeStageOwnerExecution.runs[index];
    NDSRendererMatrix20p12 composed;
    s32 composed_shift = -1;
    u32 p, composed_binding = UINT_MAX, corner_triangle = UINT_MAX;
    s16 corner_depth = 0;
    u64 near_bindings = ((u64)r->binding_lo | ((u64)r->binding_hi << 32)) &
        ~sNdsNativeStageOwnerExecution.rigid_binding_mask;
    sb32 near_boundary = FALSE;
    NDS_FIGHTER_PACKET_DMA_WAIT();
    while (native->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z && near_bindings != 0u)
    {
        u32 low = (u32)near_bindings;
        u32 binding = low != 0u ? (u32)__builtin_ctz(low) : 32u + (u32)__builtin_ctz((u32)(near_bindings >> 32));
        if (!ndsStageGxInsideNear(r, &sNdsNativeStageOwnerExecution.binding_composed[binding]))
        { near_boundary = TRUE; break; }
        near_bindings &= near_bindings - 1u;
    }
    if (near_boundary != FALSE)
    {
        /* The old prepared-near cache has no camera in its key. Refresh only
         * these rare boundary runs before taking the existing cold clipper. */
        for (p = 0u; p < (u32)native->triangle_count * 3u; p++)
        {
            u32 dense = sNdsNativeStageCorners[native->first_corner + p];
            NDSRendererInputVertex input;
            NDSRendererClipVertex20p12 clip;
            ndsRendererNativeStageInputVertex(&sNdsNativeStageVertices[dense], &input);
            ndsRendererTransformVertex20p12(&sNdsNativeStageOwnerExecution.binding_composed[
                sNdsNativeStageVertices[dense].matrix_binding], &input, &clip);
            sNdsNativeStagePreparedDense[dense].near_inside = ndsRendererHardwareClipZWInsideNearPlane(clip.z, clip.w);
        }
        gNdsP2StageProgNearRuns++;
        return FALSE;
    }
    /* Validate/patch non-painter inputs before consuming depth or emitting vertices. */
    for (p = r->first_patch; p < (u32)r->first_patch + r->patch_count; p++)
    {
        const NDSStageGxPatch *q = &sNdsStageGxPatches[p];
        u32 *dst = &sNdsStageGxWords[q->word];
        m4x4 matrix;
        if (q->kind == NDS_STAGE_GX_VIEW)
        {
            ndsRendererNativeBuildHierarchyHardwareAffine(sNdsNativeStageOwnerExecution.camera_modelview, &matrix);
            memcpy(dst, &matrix, sizeof(matrix));
        }
        else if (q->kind == NDS_STAGE_GX_WORLD)
        {
            if (!ndsRendererNativeStageTask36BuildWorld(q->index, q->aux, &matrix))
            { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 5u; return FALSE; }
            memcpy(dst, &matrix, sizeof(matrix));
        }
        else if (q->kind == NDS_STAGE_GX_PROJECTION)
        {
            NDSRendererMatrix20p12 projection = *sNdsNativeStageOwnerExecution.projection;
            u32 col;
            for (col = 0u; col < 4u; col++)
            { projection.m[3][col] = ndsRendererRoundShiftS32Signed(projection.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT); }
            ndsRendererCopyMtx20p12ToM4x4(&projection, &matrix);
            memcpy(dst, &matrix, sizeof(matrix));
        }
        else if (q->kind == NDS_STAGE_GX_NOZ)
        {
            memcpy(dst, sNdsNativeStageOwnerExecution.projection, sizeof(composed));
        }
        else if (q->kind == NDS_STAGE_GX_COMPOSED_NOZ || q->kind == NDS_STAGE_GX_CORNER_NOZ)
        {
            u32 binding = q->kind == NDS_STAGE_GX_COMPOSED_NOZ ? q->index :
                sNdsNativeStageVertices[q->index].matrix_binding;
            u32 shift = q->kind == NDS_STAGE_GX_COMPOSED_NOZ ? q->aux : q->aux & 7u;
            if (composed_shift != (s32)shift || composed_binding != binding)
            {
                if (!ndsRendererBuildShiftedRawHardwareMatrix(
                    &sNdsNativeStageOwnerExecution.binding_composed[binding], &composed, shift))
                { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 8u; return FALSE; }
                composed_shift = (s32)shift;
                composed_binding = binding;
            }
            memcpy(dst, &composed, sizeof(composed));
        }
        else if (q->kind == NDS_STAGE_GX_COLOR) { *dst = sNdsNativeStagePreparedDense[q->index].packed_color; }
        else if (q->kind == NDS_STAGE_GX_UV)
        {
            const NDSNativeStagePreparedDense *v = &sNdsNativeStagePreparedDense[q->index];
            *dst = (u32)(u16)v->s | ((u32)(u16)v->t << 16);
        }
        else if (q->kind == NDS_STAGE_GX_MATERIAL &&
                 !ndsStageGxMaterial(dst, prepared, native->submit_class, owner))
        { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 6u; return FALSE; }
    }
    if (native->submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
    { ndsRendererHardwareEnterProjectedForeground(); }
    for (p = r->first_patch; p < (u32)r->first_patch + r->patch_count; p++)
    {
        const NDSStageGxPatch *q = &sNdsStageGxPatches[p];
        if (q->kind == NDS_STAGE_GX_NOZ || q->kind == NDS_STAGE_GX_COMPOSED_NOZ || q->kind == NDS_STAGE_GX_CORNER_NOZ)
        {
            u32 *dst = &sNdsStageGxWords[q->word], row;
            s16 depth;
            if (q->kind == NDS_STAGE_GX_CORNER_NOZ)
            {
                u32 triangle = q->aux >> 3;
                if (corner_triangle != triangle)
                { corner_triangle = triangle; corner_depth = ndsRendererHardwareNextProjectedDepth(); }
                depth = corner_depth;
            }
            else { depth = ndsRendererHardwareNextProjectedDepth(); }
            /* Same four rounded Z-column writes as SetNoZColumn, directly in
             * the DMA buffer. X/Y/W were patched before consuming any depth. */
            for (row = 0u; row < 4u; row++)
            {
                dst[row * 4u + 2u] = (u32)(s32)ndsRendererRoundShiftS64(
                    (s64)(s32)dst[row * 4u + 3u] * depth, 12u);
            }
        }
    }
    if (prepared->textured != 0u)
    {
        prepared->texture_entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        prepared->texture_entry->params = prepared->texture_params;
        if (prepared->texture_entry->pinned != 0u)
        { ndsRendererHardwareRecordBattleStaticTextureHit(prepared->texture_entry); }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_bind_count++;
        stats->hardware_texture_format = prepared->texture_format;
        stats->hardware_texture_width = prepared->texture_width;
        stats->hardware_texture_height = prepared->texture_height;
        ndsRendererProfileRecordTextureBind();
    }
    ndsStageGxAppend(r, prepared);
    gNdsP2StageProgDraws++; gNdsP2StageProgWords += r->word_count;
    return TRUE;
}
