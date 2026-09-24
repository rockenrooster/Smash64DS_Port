/* Phase 2: native GX templates, initially Dream Land's rigid no-Z segments.
 * Included after the stage helpers. No N64 decoder or runtime GX recorder. */
#include <nds/nds_stage_gx.h>

_Static_assert(sizeof(NDSStageGxHeader) == 48, "stage GX header ABI");
_Static_assert(sizeof(NDSStageGxRun) == 12, "stage GX run ABI");
_Static_assert(sizeof(NDSStageGxPatch) == 8, "stage GX patch ABI");

volatile u32 gNdsP2StageProg __attribute__((used, section(".dtcm.bss"), aligned(4))) = 0u;
volatile u32 gNdsP2StageProgDraws, gNdsP2StageProgWords;
volatile u32 gNdsP2StageProgLoads, gNdsP2StageProgBytes;
volatile u32 gNdsP2StageProgDeclines, gNdsP2StageProgReason;
static u32 sNdsStageGxGeneration, sNdsStageGxAttempted;
static NDSStageGxRun *sNdsStageGxRuns;
static NDSStageGxPatch *sNdsStageGxPatches;
static u32 *sNdsStageGxWords;

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
    for (i = 0u; i < h->run_count; i++)
    {
        const NDSStageGxRun *r = &runs[i];
        u32 segment, wanted = FALSE, p, cursor, end, depth = 0u, vertices = 0u;
        for (segment = 5u; segment <= 7u; segment += 2u)
        {
            const NDSNativeStageSegment *s = &sNdsNativeStageSegments[segment];
            if (i >= s->first_run && i - s->first_run < s->run_count)
            { wanted = TRUE; break; }
        }
        if (wanted == FALSE) { if (r->word_count != 0u) { return FALSE; } continue; }
        if (r->word_count == 0u || r->segment != segment ||
            r->triangles != sNdsNativeStageRuns[i].triangle_count ||
            (u32)r->first_word + r->word_count > h->word_count ||
            (u32)r->first_patch + r->patch_count > h->patch_count) { return FALSE; }
        p = r->first_patch; cursor = r->first_word; end = cursor + r->word_count;
        while (cursor < end)
        {
            u32 ops = words[cursor++], lane;
            for (lane = 0u; lane < 4u; lane++)
            {
                u32 op = (ops >> (lane * 8u)) & 255u, n, kind = 0u;
                switch (op)
                {
                case 0: case 0x41: n = 0u; break;
                case 0x11: n = 0u; if (++depth > 1u) { return FALSE; } break;
                case 0x12: n = 1u; if (depth-- != 1u) { return FALSE; } break;
                case 0x10: case 0x40: n = 1u; break;
                case 0x16: n = 16u; kind = NDS_STAGE_GX_VIEW; break;
                case 0x18: n = 16u; kind = NDS_STAGE_GX_WORLD; break;
                case 0x20: n = 1u; kind = NDS_STAGE_GX_COLOR; break;
                case 0x22: n = 1u; kind = NDS_STAGE_GX_UV; break;
                case 0x23: n = 2u; vertices++; if (depth != 1u) { return FALSE; } break;
                default: return FALSE;
                }
                if (cursor + n > end || (op == 0x12 && words[cursor] != 1u) ||
                    (op == 0x10 && words[cursor] != 0u && words[cursor] != 2u) ||
                    (op == 0x40 && words[cursor] != 0u)) { return FALSE; }
                if (kind != 0u)
                {
                    const NDSStageGxPatch *q;
                    if (p >= (u32)r->first_patch + r->patch_count) { return FALSE; }
                    q = &patches[p++];
                    if (q->word != cursor ||
                        (q->kind != kind && !(op == 0x16 && q->kind == NDS_STAGE_GX_NOZ)) ||
                        (q->kind == NDS_STAGE_GX_WORLD &&
                         (q->index >= NDS_NATIVE_STAGE_BINDING_COUNT || q->aux > 6u)) ||
                        (q->kind >= NDS_STAGE_GX_COLOR && q->index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT))
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
    if (h.magic != 0x31505847u || h.version != 1u || h.gkind != NDS_NATIVE_STAGE_GKIND_PUPUPU ||
        h.run_count != NDS_NATIVE_STAGE_RUN_COUNT || h.word_count == 0u || h.word_count > 16384u ||
        h.patch_count > 4096u || h.segment_mask != 0xa0u ||
        h.body_bytes != h.run_count * 12u + h.patch_count * 8u + h.word_count * 4u ||
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
    gNdsP2StageProgLoads++; gNdsP2StageProgBytes = h.body_bytes;
    return TRUE;
failed:
    ndsRelocAssetStreamClose(&stream);
    gNdsP2StageProgReason = reason;
    return FALSE;
}

static sb32 ndsStageGxEligible(u32 segment)
{
    if (gNdsP2StageProg == 0u || sNdsNativeStagePacketActive->gkind != NDS_NATIVE_STAGE_GKIND_PUPUPU ||
        (segment != 5u && segment != 7u)) { return FALSE; }
    if (!ndsStageGxLoad()) { gNdsP2StageProgDeclines++; return FALSE; }
    return TRUE;
}

/* Keep the patcher out of the nearly full ITCM executor during coexistence. */
static sb32 __attribute__((noinline)) ndsStageGxDraw(u32 index, u32 owner, NDSRendererStats *stats)
{
    const NDSStageGxRun *r = &sNdsStageGxRuns[index];
    const NDSNativeStageRun *native = &sNdsNativeStageRuns[index];
    const NDSNativeStagePreparedRun *prepared = &sNdsNativeStageOwnerExecution.runs[index];
    u32 p;
    NDS_FIGHTER_PACKET_DMA_WAIT();
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
        else if (q->kind == NDS_STAGE_GX_COLOR) { *dst = sNdsNativeStagePreparedDense[q->index].packed_color; }
        else if (q->kind == NDS_STAGE_GX_UV)
        {
            const NDSNativeStagePreparedDense *v = &sNdsNativeStagePreparedDense[q->index];
            *dst = (u32)(u16)v->s | ((u32)(u16)v->t << 16);
        }
    }
    if (!ndsRendererNativeStageBeginRun(native, prepared, native->submit_class, owner, stats, TRUE))
    { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 6u; return FALSE; }
    for (p = r->first_patch; p < (u32)r->first_patch + r->patch_count; p++)
    {
        const NDSStageGxPatch *q = &sNdsStageGxPatches[p];
        if (q->kind == NDS_STAGE_GX_NOZ)
        {
            NDSRendererMatrix20p12 projection = *sNdsNativeStageOwnerExecution.projection;
            m4x4 matrix;
            ndsRendererNativeStageSetNoZColumn(&projection, ndsRendererHardwareNextProjectedDepth());
            ndsRendererCopyMtx20p12ToM4x4(&projection, &matrix);
            memcpy(&sNdsStageGxWords[q->word], &matrix, sizeof(matrix));
        }
    }
    DC_FlushRange(&sNdsStageGxWords[r->first_word], (u32)r->word_count * 4u);
    DMA_SRC(0) = (u32)&sNdsStageGxWords[r->first_word];
    DMA_DEST(0) = (u32)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | r->word_count;
    /* Bring-up keeps the existing synchronous boundary. Whole-segment async
     * submission follows once the remaining run classes are compiled. */
    while ((DMA_CR(0) & DMA_BUSY) != 0u) { }
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareTriangleBatchOpen = FALSE;
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    gNdsP2StageProgDraws++; gNdsP2StageProgWords += r->word_count;
    return TRUE;
}
