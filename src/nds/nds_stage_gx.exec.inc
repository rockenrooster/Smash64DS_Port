/* Phase 2: native GX templates for the nine VS stages and (P2-6) every
 * 1P-only venue.
 * Included after the stage helpers. No N64 decoder or runtime GX recorder. */
#include <nds/nds_stage_gx.h>

_Static_assert(sizeof(NDSStageGxHeader) == 48, "stage GX header ABI");
_Static_assert(sizeof(NDSStageGxRun) == 32, "stage GX run ABI");
_Static_assert(sizeof(NDSStageGxPatch) == 8, "stage GX patch ABI");
#define NDS_STAGE_GX_HEAP_KEEP_FREE (25600u + 36420u)
#define NDS_STAGE_GX_NOZ_LOCAL 16u

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
/* The last span handed to DMA0, [first, end) in words. While that DMA can
 * still be running, only a run whose own words lie inside it has to wait
 * before patching them.
 *
 * P2-2p8 (2026-09-28): every span flush used to be followed by a full drain,
 * because the next span's start waited for DMA0 before setting texture, alpha
 * test and fog. Those are DISP3DCNT / ALPHA_TEST_REF, rendering-engine
 * registers outside the FIFO: draining first changes nothing about the
 * command stream. The whole-match gate profile charged that one wait 9.2K
 * ticks a frame (a 968-cycle DMA0CNT read, 19 a frame). Now the CPU patches
 * the next runs while the geometry engine drains the previous span, and a
 * wait remains only where a FIFO command follows (an open Task 36 segment's
 * matrix pop) or a DMA would be re-armed (ndsStageGxFlush). Same-ROM A/B
 * word: gNdsStageGxOverlap (0 = the old drain at every span start). */
static u32 sNdsStageGxDmaFirst, sNdsStageGxDmaEnd;
volatile u32 gNdsStageGxOverlap __attribute__((used, section(".data"))) = 1u;

/* Words that outlive the frame (P2-2p8, 2026-09-29). The program's words are
 * a persistent buffer nothing but these patches writes, so a patch whose
 * inputs did not move since it last wrote them has nothing to write:
 *  - MATERIAL (three words a run): the run's polygon format and texture name
 *    and parameters, resolved through libnds's texture and palette objects,
 *    which only a texture-cache event moves (the key generation the prepared
 *    table's proof already trusts). Dream Land: 53 a frame, ~130 ticks each.
 *  - NOZ and PROJECTION (sixteen words a patch): the camera projection, which
 *    is constant within a match unless the FOV moves; the per-triangle depth
 *    column the painter pass writes is still rewritten every frame.
 * Each run remembers what its words hold; a run that returned early keeps its
 * old record, so a skipped frame never vouches for words it did not write.
 * Same-ROM A/B word gNdsStageGxMemo (0 = patch every word every frame). */
volatile u32 gNdsStageGxMemo __attribute__((used, section(".data"))) = 1u;
typedef struct NDSStageGxRunMemo
{
    u32 poly_fmt;
    u32 texture_name;
    u32 texture_params;
    u32 textured;
    u32 material_gen;           /* texture key generation + 1; 0 = unwritten */
    u32 projection_serial;      /* sNdsStageGxProjSerial its NOZ words hold */
} NDSStageGxRunMemo;
static NDSStageGxRunMemo *sNdsStageGxRunMemo;
static NDSRendererMatrix20p12 sNdsStageGxProjCopy;
static u32 sNdsStageGxProjSerial, sNdsStageGxProjGen;

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
        for (segment = 0u; segment < NDS_NATIVE_STAGE_SEGMENT_COUNT; segment++)
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
                if (op == 0x18)
                {
                    const NDSNativeStageRun *native = &sNdsNativeStageRuns[i];
                    u32 shift;
                    m4x4 expected;
                    if ((static_world & ((u64)1u << native->binding_index)) == 0u) { return FALSE; }
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
                          q->kind == NDS_STAGE_GX_COMPOSED_NOZ || q->kind == NDS_STAGE_GX_CORNER_NOZ ||
                          q->kind == NDS_STAGE_GX_COMPOSED || q->kind == NDS_STAGE_GX_CORNER_SOURCE))) ||
                        ((q->kind == NDS_STAGE_GX_WORLD || q->kind == NDS_STAGE_GX_COMPOSED_NOZ ||
                          q->kind == NDS_STAGE_GX_COMPOSED) &&
                         (q->index >= NDS_NATIVE_STAGE_BINDING_COUNT || q->aux > 6u)) ||
                        (q->kind == NDS_STAGE_GX_CORNER_NOZ &&
                         (q->index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT ||
                          (q->aux & 7u) > 6u || (q->aux >> 3) >= r->triangles)) ||
                        (q->kind == NDS_STAGE_GX_CORNER_SOURCE &&
                         (q->index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT || q->aux >= r->triangles)) ||
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

/* One template per native stage gkind; NULL where none is compiled (10 and 11
 * have no native program). P2-6: the 1P campaign's venues -- the team stages,
 * Metal, Polygon, Final Destination, the Race and both boards per fighter --
 * joined the nine VS stages; a build without 1P content stages none of them,
 * and a missing file declines to the uncompiled draw. */
static const char *const sNdsStageGxPaths[] = {
    "nitro:/stages/castle.gxp", "nitro:/stages/sector.gxp",
    "nitro:/stages/jungle.gxp", "nitro:/stages/zebes.gxp",
    "nitro:/stages/hyrule.gxp", "nitro:/stages/yoster.gxp",
    "nitro:/stages/dreamland.gxp", "nitro:/stages/yamabuki.gxp",
    "nitro:/stages/inishie.gxp", "nitro:/stages/pupupusmall.gxp",
    NULL, NULL,
    "nitro:/stages/yostersmall.gxp", "nitro:/stages/metal.gxp",
    "nitro:/stages/zako.gxp", "nitro:/stages/bonus3.gxp",
    "nitro:/stages/last.gxp",
    "nitro:/stages/bonus1_mario.gxp", "nitro:/stages/bonus1_fox.gxp",
    "nitro:/stages/bonus1_donkey.gxp", "nitro:/stages/bonus1_samus.gxp",
    "nitro:/stages/bonus1_luigi.gxp", "nitro:/stages/bonus1_link.gxp",
    "nitro:/stages/bonus1_yoshi.gxp", "nitro:/stages/bonus1_captain.gxp",
    "nitro:/stages/bonus1_kirby.gxp", "nitro:/stages/bonus1_pikachu.gxp",
    "nitro:/stages/bonus1_purin.gxp", "nitro:/stages/bonus1_ness.gxp",
    "nitro:/stages/bonus2_mario.gxp", "nitro:/stages/bonus2_fox.gxp",
    "nitro:/stages/bonus2_donkey.gxp", "nitro:/stages/bonus2_samus.gxp",
    "nitro:/stages/bonus2_luigi.gxp", "nitro:/stages/bonus2_link.gxp",
    "nitro:/stages/bonus2_yoshi.gxp", "nitro:/stages/bonus2_captain.gxp",
    "nitro:/stages/bonus2_kirby.gxp", "nitro:/stages/bonus2_pikachu.gxp",
    "nitro:/stages/bonus2_purin.gxp", "nitro:/stages/bonus2_ness.gxp"
};
#define NDS_STAGE_GX_PATH_COUNT \
    (sizeof(sNdsStageGxPaths) / sizeof(sNdsStageGxPaths[0]))

static sb32 __attribute__((noinline, cold, optimize("Os"))) ndsStageGxLoad(void)
{
    NdsRelocAssetStream stream = { NULL };
    NDSStageGxHeader h;
    u8 *body;
    u32 reason = 1u;
    u32 gkind = sNdsNativeStagePacketActive->gkind;
    if ((gkind >= NDS_STAGE_GX_PATH_COUNT) || (sNdsStageGxPaths[gkind] == NULL))
    { return FALSE; }
    if (sNdsStageGxGeneration != gNdsTaskmanHeapGeneration)
    {
        sNdsStageGxGeneration = gNdsTaskmanHeapGeneration;
        sNdsStageGxRuns = NULL; sNdsStageGxAttempted = FALSE;
    }
    if (sNdsStageGxRuns != NULL) { return TRUE; }
    if (sNdsStageGxAttempted != FALSE) { return FALSE; }
    sNdsStageGxAttempted = TRUE;
    if (!ndsRelocAssetStreamOpen(&stream, sNdsStageGxPaths[gkind]) ||
        !ndsRelocAssetStreamRead(&stream, 0u, &h, sizeof(h))) { goto failed; }
    reason = 2u;
    if (h.magic != 0x31505847u || h.version != 5u || h.gkind != gkind ||
        h.run_count != NDS_NATIVE_STAGE_RUN_COUNT || h.word_count == 0u || h.word_count > 16384u ||
        h.patch_count > 4096u || NDS_NATIVE_STAGE_SEGMENT_COUNT >= 32u ||
        h.segment_mask != (1u << NDS_NATIVE_STAGE_SEGMENT_COUNT) - 1u ||
        (((u64)h.static_world_lo | ((u64)h.static_world_hi << 32)) &
         ~sNdsNativeStageOwnerExecution.rigid_binding_mask) != 0u ||
        h.body_bytes != h.run_count * sizeof(NDSStageGxRun) + h.patch_count * 8u + h.word_count * 4u ||
        h.source_hash != ndsStageGxSourceHash()) { goto failed; }
    reason = 3u;
    /* The body is battle-lifetime heap. It loads at the first stage draw, and
     * the match still grows the heap after that: 36,420 B on the heavy roster
     * (phase3-residency, probe 92984). Keep the 25,600 B floor above both, or
     * decline and draw the stage uncompiled. */
    if ((uintptr_t)gSYTaskmanGeneralHeap.end < (uintptr_t)gSYTaskmanGeneralHeap.ptr ||
        (uintptr_t)gSYTaskmanGeneralHeap.end - (uintptr_t)gSYTaskmanGeneralHeap.ptr <
            h.body_bytes + (h.run_count * sizeof(NDSStageGxRunMemo)) +
            NDS_STAGE_GX_HEAP_KEEP_FREE)
    { goto failed; }
    body = syTaskmanMalloc(
        h.body_bytes + (h.run_count * sizeof(NDSStageGxRunMemo)), 32u);
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
    sNdsStageGxRunMemo = (NDSStageGxRunMemo *)(void *)(body + h.body_bytes);
    memset(sNdsStageGxRunMemo, 0, h.run_count * sizeof(NDSStageGxRunMemo));
    sNdsStageGxProjSerial++;
    sNdsStageGxProjGen = 0u;
    gNdsP2StageProgLoads++; gNdsP2StageProgBytes = h.body_bytes;
    return TRUE;
failed:
    ndsRelocAssetStreamClose(&stream);
    gNdsP2StageProgReason = reason;
    return FALSE;
}

static sb32 ndsStageGxEligible(u32 segment)
{
    u32 gkind = sNdsNativeStagePacketActive->gkind;
    if (gkind >= NDS_STAGE_GX_PATH_COUNT || sNdsStageGxPaths[gkind] == NULL ||
        segment >= NDS_NATIVE_STAGE_SEGMENT_COUNT) { return FALSE; }
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
    /* Source-Z cross corners also require nonzero W after rounding to /256. */
    return w >= 128 && (s64)ndsRendererClampS64ToS32(z) + ndsRendererClampS64ToS32(w) >= 0;
}

/* P2-6 (2026-10-01): a run whose bounds lie wholly outside the view draws
 * nothing, so it is skipped before its patch and DMA. The 1P courses (the
 * Race, the boards) submit their whole map a frame while the camera shows a
 * fraction of it. The bounds are the template's object-space box and the
 * transform the run's own clip matrix: a dynamic binding's composed product,
 * a rigid binding's world times the camera (one product per binding switch;
 * a segment's runs of one binding are adjacent). Each side's plane distance
 * (w - x, w + x, w - y, w + y) is maximised over the box's corners like the
 * near test above; a box with no corner inside one side is invisible.
 * Multi-binding runs are never skipped. The nine VS venues keep the old path
 * until measured. Same-ROM A/B word gNdsStageGxCull (0 = draw every run). */
#define NDS_STAGE_GX_CULL_FIRST_GKIND 9u /* nGRKindPupupuSmall: 1P venues on */
volatile u32 gNdsStageGxCull __attribute__((used, section(".data"))) = 1u;
volatile u32 gNdsStageGxCulledRuns;
static NDSRendererMatrix20p12 sNdsStageGxCullRigid;
static u32 sNdsStageGxCullRigidBinding = UINT_MAX;
static u32 sNdsStageGxCullRigidGen;

static sb32 ndsStageGxRunOutsideView(u32 index)
{
    const NDSStageGxRun *r = &sNdsStageGxRuns[index];
    const NDSRendererMatrix20p12 *m;
    u32 binding;
    u32 plane;
    u32 axis;

    if ((gNdsStageGxCull == 0u) ||
        (sNdsNativeStagePacketActive->gkind < NDS_STAGE_GX_CULL_FIRST_GKIND) ||
        ((r->binding_lo != 0u) == (r->binding_hi != 0u)) ||
        ((r->binding_lo & (r->binding_lo - 1u)) != 0u) ||
        ((r->binding_hi & (r->binding_hi - 1u)) != 0u))
    {
        return FALSE;
    }
    binding = (r->binding_lo != 0u) ? (u32)__builtin_ctz(r->binding_lo) :
        32u + (u32)__builtin_ctz(r->binding_hi);
    if (binding >= NDS_NATIVE_STAGE_BINDING_COUNT)
    {
        return FALSE;
    }
    if ((sNdsNativeStageOwnerExecution.rigid_binding_mask &
         ((u64)1u << binding)) != 0u)
    {
        if ((sNdsNativeStageOwnerExecution.binding_world == NULL) ||
            (sNdsNativeStageOwnerExecution.camera_modelview == NULL) ||
            (sNdsNativeStageOwnerExecution.projection == NULL))
        {
            return FALSE;
        }
        if ((sNdsStageGxCullRigidBinding != binding) ||
            (sNdsStageGxCullRigidGen != sNdsNativeStageMatrixGen))
        {
            ndsRendererMtxMul20p12(
                &sNdsNativeStageOwnerExecution.binding_world[binding],
                sNdsNativeStageOwnerExecution.camera_modelview,
                &sNdsStageGxCullRigid);
            ndsRendererMtxMul20p12(&sNdsStageGxCullRigid,
                                   sNdsNativeStageOwnerExecution.projection,
                                   &sNdsStageGxCullRigid);
            sNdsStageGxCullRigidBinding = binding;
            sNdsStageGxCullRigidGen = sNdsNativeStageMatrixGen;
        }
        m = &sNdsStageGxCullRigid;
    }
    else
    {
        if (sNdsNativeStageOwnerExecution.binding_composed == NULL)
        {
            return FALSE;
        }
        m = &sNdsNativeStageOwnerExecution.binding_composed[binding];
    }
    for (plane = 0u; plane < 4u; plane++)
    {
        u32 col = plane >> 1;
        s64 sign = ((plane & 1u) != 0u) ? 1 : -1;
        s64 d = (s64)m->m[3][3] + (sign * (s64)m->m[3][col]);

        for (axis = 0u; axis < 3u; axis++)
        {
            s64 c = (s64)m->m[axis][3] + (sign * (s64)m->m[axis][col]);

            d += c * (s64)((c >= 0) ? r->maximum[axis] : r->minimum[axis]);
        }
        if (d < 0)
        {
            gNdsStageGxCulledRuns++;
            return TRUE;
        }
    }
    return FALSE;
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
    if (sNdsNativeStagePacketActive->gkind == NDS_NATIVE_STAGE_GKIND_PUPUPU &&
        submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z &&
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
    sNdsStageGxDmaFirst = sNdsStageGxFirstWord;
    sNdsStageGxDmaEnd = sNdsStageGxFirstWord + sNdsStageGxWordCount;
    sNdsStageGxWordCount = 0u;
    gNdsP2StageProgDmas++;
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareBoundTextureName = 0u;
    /* The packet set TEXIMAGE_PARAM / PLTT_BASE behind libnds: forget its active
     * names too, or a later glBindTexture of the name it still holds returns
     * without writing the registers and draws with this packet's texture. */
    glGlobalData.activeTexture = 0;
    glGlobalData.activePalette = 0;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsR2GxLastProjection = NULL;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareTriangleBatchOpen = FALSE;
}

/* The span's texture, alpha test and fog: DISP3DCNT and ALPHA_TEST_REF,
 * rendering-engine registers outside the FIFO. */
static inline void ndsStageGxSetSpanState(u32 alpha,
                                          const NDSNativeStagePreparedRun *prepared)
{
    glEnable(GL_TEXTURE_2D);
    if (alpha != 0u) { glEnable(GL_ALPHA_TEST); glAlphaFunc(prepared->alpha_ref); }
    else { glDisable(GL_ALPHA_TEST); }
    glDisable(GL_FOG);
}

/* P2-2p8 (2026-09-29): the next span's state is written BEFORE the flush arms
 * DMA0 for the previous one. Written after it, its first register read
 * (`GFX_CONTROL |= bits`) waited on the bus for the whole span transfer: 595
 * cycles, 27 spans a frame, 8K ticks of the whole-match gate profile on that
 * one instruction. The writes and their order are unchanged -- after a flush
 * no batch is open, so neither the Task 36 end nor EndBatch below writes
 * these registers -- only the DMA they no longer wait behind has moved.
 * Same-ROM A/B word gNdsStageGxStateFirst (0 = after the flush). */
volatile u32 gNdsStageGxStateFirst __attribute__((used, section(".data"))) = 1u;

static void ndsStageGxAppend(const NDSStageGxRun *run,
                           const NDSNativeStagePreparedRun *prepared)
{
    u32 alpha = prepared->alpha_test != 0u ? 0x100u | prepared->alpha_ref : 0u;
    sb32 state_written = FALSE;

    if (sNdsStageGxWordCount != 0u &&
        (run->first_word != sNdsStageGxFirstWord + sNdsStageGxWordCount ||
         alpha != sNdsStageGxAlpha))
    {
        if (gNdsStageGxStateFirst != 0u)
        {
            ndsStageGxSetSpanState(alpha, prepared);
            state_written = TRUE;
        }
        ndsStageGxFlush();
    }
    if (sNdsStageGxWordCount == 0u)
    {
        /* Alpha-test registers are outside the FIFO; never merge across a
         * hidden run or state edge. Only an open Task 36 segment writes the
         * FIFO here (its matrix mode and pop), and only that has to follow
         * the previous span's DMA. */
        if ((gNdsStageGxOverlap == 0u) ||
            (sNdsNativeStageOwnerExecution.task36_segment_active != FALSE))
        {
            NDS_FIGHTER_PACKET_DMA_WAIT();
        }
        ndsRendererNativeStageTask36EndSegment();
        ndsRendererHardwareEndBatch();
        if (state_written == FALSE)
        {
            ndsStageGxSetSpanState(alpha, prepared);
        }
        sNdsStageGxFirstWord = run->first_word;
        sNdsStageGxAlpha = alpha;
    }
    sNdsStageGxWordCount += run->word_count;
}

/* Per stage-matrix generation (sNdsNativeStageMatrixGen): the VIEW patch's
 * hardware affine and each (binding, shift) composed matrix depend only on
 * that generation's camera and binding matrices, yet Dream Land's program
 * rebuilt the affine for each of its 22 VIEW patches and a composed matrix at
 * ~52 binding changes a frame (23 distinct). Same-ROM A/B: STG -4.9K. */
#define NDS_STAGE_GX_MEMO_SLOTS 32u
typedef struct NDSStageGxMatMemo
{
    u32 gen;
    u16 binding;
    u8 shift;
    u8 ok;
    NDSRendererMatrix20p12 m;
} NDSStageGxMatMemo;
static NDSStageGxMatMemo sNdsStageGxMatMemo[NDS_STAGE_GX_MEMO_SLOTS];
static m4x4 sNdsStageGxViewMemo;
static u32 sNdsStageGxViewGen;

static sb32 ndsStageGxComposedFor(u32 binding, u32 shift,
                                  NDSRendererMatrix20p12 *out)
{
    NDSStageGxMatMemo *m = &sNdsStageGxMatMemo[((binding << 3) | shift) &
                                               (NDS_STAGE_GX_MEMO_SLOTS - 1u)];
    sb32 ok;

    if ((m->gen == sNdsNativeStageMatrixGen) &&
        (m->binding == (u16)binding) && (m->shift == (u8)shift))
    {
        *out = m->m;
        return (m->ok != 0u) ? TRUE : FALSE;
    }
    ok = ndsRendererBuildShiftedRawHardwareMatrix(
        &sNdsNativeStageOwnerExecution.binding_composed[binding], out, shift);
    m->gen = sNdsNativeStageMatrixGen;
    m->binding = (u16)binding;
    m->shift = (u8)shift;
    m->ok = (ok != FALSE) ? 1u : 0u;
    if (ok != FALSE)
    {
        m->m = *out;
    }
    return ok;
}

/* ITCM since 2026-09-27: the fill census charged it ~8.5M cycles of
 * instruction-fetch stall in main RAM; low-rent renderer residents made room. */
/* Out of ITCM: never on Dream Land's measured match, rare elsewhere. */
static void __attribute__((noinline, cold)) ndsStageGxRefreshNear(
    const NDSNativeStageRun *native)
{
    u32 p;

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
}

static void __attribute__((noinline, cold)) ndsStageGxCornerSource(
    u32 *dst, const NDSStageGxPatch *q)
{
    const NDSNativeStageDenseVertex *v = &sNdsNativeStageVertices[q->index];
    NDSRendererInputVertex input;
    NDSRendererClipVertex20p12 clip;
    NDSRendererMatrix20p12 projected;
    ndsRendererNativeStageInputVertex(v, &input);
    ndsRendererTransformVertex20p12(
        &sNdsNativeStageOwnerExecution.binding_composed[v->matrix_binding], &input, &clip);
    ndsRendererNativeStageProjectedDepthMatrix(&clip, &projected);
    memcpy(dst, &projected, sizeof(projected));
}

/* The painter pass for a run with more no-Z patches than the draw keeps W
 * columns for: read W back from the words the first pass wrote. */
static void __attribute__((noinline, cold)) ndsStageGxNoZReadBack(
    const NDSStageGxRun *r, u32 *corner_triangle, s16 *corner_depth)
{
    u32 p;

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
                if (*corner_triangle != triangle)
                { *corner_triangle = triangle; *corner_depth = ndsRendererHardwareNextProjectedDepth(); }
                depth = *corner_depth;
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
}

static void __attribute__((noinline, cold)) ndsStageGxProjection(u32 *dst)
{
    NDSRendererMatrix20p12 projection = *sNdsNativeStageOwnerExecution.projection;
    m4x4 matrix;
    u32 col;
    for (col = 0u; col < 4u; col++)
    { projection.m[3][col] = ndsRendererRoundShiftS32Signed(projection.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT); }
    ndsRendererCopyMtx20p12ToM4x4(&projection, &matrix);
    memcpy(dst, &matrix, sizeof(matrix));
}

/* The frame's projection against the one the NOZ/PROJECTION words hold:
 * compared once per stage-matrix generation, a new serial when it moved. */
static inline u32 ndsStageGxProjectionSerial(void)
{
    if (sNdsStageGxProjGen != sNdsNativeStageMatrixGen)
    {
        const NDSRendererMatrix20p12 *projection =
            sNdsNativeStageOwnerExecution.projection;

        if (memcmp(projection, &sNdsStageGxProjCopy,
                   sizeof(sNdsStageGxProjCopy)) != 0)
        {
            sNdsStageGxProjCopy = *projection;
            sNdsStageGxProjSerial++;
        }
        sNdsStageGxProjGen = sNdsNativeStageMatrixGen;
    }
    return sNdsStageGxProjSerial;
}

/* gNdsStageGxMemo = 2 (lab): the memo still decides, and every word it keeps
 * is compared with the word the patch would have written. [0] material words
 * checked, [1] of them different, [2] projection words checked, [3] different
 * (the painter's z column excluded: it is rewritten every frame). */
volatile u32 gNdsStageGxMemoVerify[4] __attribute__((used));

static void __attribute__((noinline, cold)) ndsStageGxMemoVerifyRun(
    const NDSStageGxRun *r, const NDSNativeStagePreparedRun *prepared,
    u32 submit_class, u32 owner, sb32 projection_kept, sb32 material_kept)
{
    u32 p;

    for (p = r->first_patch; p < (u32)r->first_patch + r->patch_count; p++)
    {
        const NDSStageGxPatch *q = &sNdsStageGxPatches[p];
        const u32 *dst = &sNdsStageGxWords[q->word];
        u32 fresh[16];
        u32 i;

        if ((q->kind == NDS_STAGE_GX_MATERIAL) && (material_kept != FALSE) &&
            (ndsStageGxMaterial(fresh, prepared, submit_class, owner) != FALSE))
        {
            gNdsStageGxMemoVerify[0] += 3u;
            for (i = 0u; i < 3u; i++)
            {
                gNdsStageGxMemoVerify[1] += (fresh[i] != dst[i]) ? 1u : 0u;
            }
        }
        else if (((q->kind == NDS_STAGE_GX_NOZ) ||
                  (q->kind == NDS_STAGE_GX_PROJECTION)) &&
                 (projection_kept != FALSE))
        {
            if (q->kind == NDS_STAGE_GX_NOZ)
            {
                memcpy(fresh, sNdsNativeStageOwnerExecution.projection,
                       sizeof(fresh));
            }
            else
            {
                ndsStageGxProjection(fresh);
            }
            for (i = 0u; i < 16u; i++)
            {
                if ((q->kind == NDS_STAGE_GX_NOZ) && ((i & 3u) == 2u))
                {
                    continue;
                }
                gNdsStageGxMemoVerify[2]++;
                gNdsStageGxMemoVerify[3] += (fresh[i] != dst[i]) ? 1u : 0u;
            }
        }
    }
}

static sb32 __attribute__((noinline, section(".itcm"))) ndsStageGxDraw(u32 index, u32 owner, NDSRendererStats *stats)
{
    const NDSStageGxRun *r = &sNdsStageGxRuns[index];
    const NDSNativeStageRun *native = &sNdsNativeStageRuns[index];
    const NDSNativeStagePreparedRun *prepared = &sNdsNativeStageOwnerExecution.runs[index];
    NDSRendererMatrix20p12 composed;
    s32 composed_shift = -1;
    u32 p, composed_binding = UINT_MAX, corner_triangle = UINT_MAX;
    s16 corner_depth = 0;
    /* W columns of this run's no-Z matrices, kept on the stack so the painter
     * pass below never reads back the body lines the first pass just wrote
     * (a main-RAM line fill per matrix row pair). The stages carry at most 12
     * per run; a longer run takes the read-back path. */
    s32 noz_w[NDS_STAGE_GX_NOZ_LOCAL][4];
    u16 noz_patch[NDS_STAGE_GX_NOZ_LOCAL];
    u32 noz_count = 0u;
    sb32 noz_local = TRUE;
    u64 near_bindings = ((u64)r->binding_lo | ((u64)r->binding_hi << 32)) &
        ~sNdsNativeStageOwnerExecution.rigid_binding_mask;
    sb32 near_boundary = FALSE;
    sb32 projection_kept = FALSE, material_kept = FALSE;
    u32 projection_serial = 0u;
    /* This function writes nothing to the FIFO itself (its words reach it by
     * ndsStageGxFlush's DMA, and the cold path's caller drains first), so it
     * waits only when the DMA in flight may still be reading the words it is
     * about to patch. */
    if ((gNdsStageGxOverlap == 0u) ||
        ((r->first_word < sNdsStageGxDmaEnd) &&
         ((u32)r->first_word + r->word_count > sNdsStageGxDmaFirst)))
    {
        NDS_FIGHTER_PACKET_DMA_WAIT();
    }
    while ((native->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z ||
            (native->flags & NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) != 0u) &&
           near_bindings != 0u)
    {
        u32 low = (u32)near_bindings;
        u32 binding = low != 0u ? (u32)__builtin_ctz(low) : 32u + (u32)__builtin_ctz((u32)(near_bindings >> 32));
        if (!ndsStageGxInsideNear(r, &sNdsNativeStageOwnerExecution.binding_composed[binding]))
        { near_boundary = TRUE; break; }
        near_bindings &= near_bindings - 1u;
    }
    if (near_boundary != FALSE)
    {
        ndsStageGxRefreshNear(native);
        return FALSE;
    }
    if (gNdsStageGxMemo != 0u)
    {
        NDSStageGxRunMemo *m = &sNdsStageGxRunMemo[index];

        projection_serial = ndsStageGxProjectionSerial();
        projection_kept = (m->projection_serial == projection_serial) ?
            TRUE : FALSE;
        material_kept =
            ((m->material_gen ==
              sNdsRendererHardwareTextureKeyGeneration + 1u) &&
             (m->poly_fmt == prepared->poly_fmt) &&
             (m->texture_name == prepared->texture_name) &&
             (m->texture_params == prepared->texture_params) &&
             (m->textured == prepared->textured)) ? TRUE : FALSE;
        if (gNdsStageGxMemo == 2u)
        {
            ndsStageGxMemoVerifyRun(r, prepared, native->submit_class, owner,
                                    projection_kept, material_kept);
        }
    }
    /* Validate/patch non-painter inputs before consuming depth or emitting vertices. */
    for (p = r->first_patch; p < (u32)r->first_patch + r->patch_count; p++)
    {
        const NDSStageGxPatch *q = &sNdsStageGxPatches[p];
        u32 *dst = &sNdsStageGxWords[q->word];
        if (q->kind == NDS_STAGE_GX_VIEW)
        {
            if (sNdsStageGxViewGen != sNdsNativeStageMatrixGen)
            {
                ndsRendererNativeBuildHierarchyHardwareAffine(
                    sNdsNativeStageOwnerExecution.camera_modelview,
                    &sNdsStageGxViewMemo);
                sNdsStageGxViewGen = sNdsNativeStageMatrixGen;
            }
            memcpy(dst, &sNdsStageGxViewMemo, sizeof(sNdsStageGxViewMemo));
        }
        else if (q->kind == NDS_STAGE_GX_PROJECTION && projection_kept == FALSE)
        {
            ndsStageGxProjection(dst);
        }
        else if (q->kind == NDS_STAGE_GX_NOZ)
        {
            const NDSRendererMatrix20p12 *projection = sNdsNativeStageOwnerExecution.projection;
            if (projection_kept == FALSE) { memcpy(dst, projection, sizeof(composed)); }
            if (noz_count < NDS_STAGE_GX_NOZ_LOCAL)
            {
                noz_w[noz_count][0] = projection->m[0][3]; noz_w[noz_count][1] = projection->m[1][3];
                noz_w[noz_count][2] = projection->m[2][3]; noz_w[noz_count][3] = projection->m[3][3];
                noz_patch[noz_count++] = (u16)p;
            }
            else { noz_local = FALSE; }
        }
        else if (q->kind == NDS_STAGE_GX_COMPOSED_NOZ || q->kind == NDS_STAGE_GX_COMPOSED ||
                 q->kind == NDS_STAGE_GX_CORNER_NOZ)
        {
            u32 binding = q->kind == NDS_STAGE_GX_CORNER_NOZ ?
                sNdsNativeStageVertices[q->index].matrix_binding : q->index;
            u32 shift = q->kind == NDS_STAGE_GX_CORNER_NOZ ? q->aux & 7u : q->aux;
            if (composed_shift != (s32)shift || composed_binding != binding)
            {
                if (!ndsStageGxComposedFor(binding, shift, &composed))
                { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 8u; return FALSE; }
                composed_shift = (s32)shift;
                composed_binding = binding;
            }
            memcpy(dst, &composed, sizeof(composed));
            if (q->kind != NDS_STAGE_GX_COMPOSED)
            {
                if (noz_count < NDS_STAGE_GX_NOZ_LOCAL)
                {
                    noz_w[noz_count][0] = composed.m[0][3]; noz_w[noz_count][1] = composed.m[1][3];
                    noz_w[noz_count][2] = composed.m[2][3]; noz_w[noz_count][3] = composed.m[3][3];
                    noz_patch[noz_count++] = (u16)p;
                }
                else { noz_local = FALSE; }
            }
        }
        else if (q->kind == NDS_STAGE_GX_CORNER_SOURCE)
        {
            ndsStageGxCornerSource(dst, q);
        }
        else if (q->kind == NDS_STAGE_GX_COLOR) { *dst = sNdsNativeStagePreparedDense[q->index].packed_color; }
        else if (q->kind == NDS_STAGE_GX_UV)
        {
            const NDSNativeStagePreparedDense *v = &sNdsNativeStagePreparedDense[q->index];
            *dst = (u32)(u16)v->s | ((u32)(u16)v->t << 16);
        }
        else if (q->kind == NDS_STAGE_GX_MATERIAL && material_kept == FALSE &&
                 !ndsStageGxMaterial(dst, prepared, native->submit_class, owner))
        { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 6u; return FALSE; }
    }
    if (gNdsStageGxMemo != 0u)
    {
        /* Every patch of the run was written or proven current. */
        NDSStageGxRunMemo *m = &sNdsStageGxRunMemo[index];

        m->projection_serial = projection_serial;
        m->poly_fmt = prepared->poly_fmt;
        m->texture_name = prepared->texture_name;
        m->texture_params = prepared->texture_params;
        m->textured = prepared->textured;
        m->material_gen = sNdsRendererHardwareTextureKeyGeneration + 1u;
    }
    if (native->submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
    { ndsRendererHardwareEnterProjectedForeground(); }
    if (noz_local != FALSE)
    {
        u32 k;
        for (k = 0u; k < noz_count; k++)
        {
            const NDSStageGxPatch *q = &sNdsStageGxPatches[noz_patch[k]];
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
            for (row = 0u; row < 4u; row++)
            {
                dst[row * 4u + 2u] = (u32)(s32)ndsRendererRoundShiftS64(
                    (s64)noz_w[k][row] * depth, 12u);
            }
        }
    }
    else
    {
        ndsStageGxNoZReadBack(r, &corner_triangle, &corner_depth);
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
