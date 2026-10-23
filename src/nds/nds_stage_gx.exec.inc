/* Phase 2: native GX templates for the nine VS stages and (P2-6) every
 * 1P-only venue.
 * Included after the stage helpers. No N64 decoder or runtime GX recorder. */
#include <nds/nds_stage_gx.h>

_Static_assert(sizeof(NDSStageGxHeader) == 48, "stage GX header ABI");
_Static_assert(sizeof(NDSStageGxRun) == 32, "stage GX run ABI");
_Static_assert(sizeof(NDSStageGxPatch) == 8, "stage GX patch ABI");
_Static_assert(sizeof(NDSStageGxSegment) == 16, "stage GX segment ABI");
/* The body loads at the first stage draw. What the match allocates after
 * that is draw caches that check their own floors and decline (the lean
 * spares, the world caches; the elastic motion cache yields): Hyrule
 * Temple's cast match grew 36 KB, 17.7 KB of it a lean spare and 13.3 KB
 * the world caches (2026-10-18 census). So the body keeps the 25,600 B
 * floor (the GObj cap latches under it) and a margin (12 KB until P7) for
 * the allocations that cannot decline, not the whole 36,420 B growth: a
 * stage program is worth more than the caches that would take its bytes
 * (Hyrule Temple drew uncompiled, 409K ticks a frame).
 * P7 (2026-10-21): the margin no longer guards the floor -- a program
 * yields its bytes before an allocation would take the heap under it
 * (ndsStageGxYield) -- only how often a match keeps its program to the end:
 * 8 KB covers a match's usual non-declining growth (2-5 KB in a gate
 * window); a Kirby copy hat (7.6 KB) or a five-minute match's object pools
 * (15-20 KB) end in a yield. */
#define NDS_STAGE_GX_HEAP_KEEP_FREE (25600u + 8192u)
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
 * matrix pop) or a DMA would be re-armed (ndsStageGxFlush). */
static u32 sNdsStageGxDmaFirst, sNdsStageGxDmaEnd;

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
 * old record, so a skipped frame never vouches for words it did not write. */
typedef struct NDSStageGxRunMemo
{
    u32 poly_fmt;
    u32 texture_name;
    u32 texture_params;
    u32 textured;
    u32 material_gen;           /* texture key generation + 1; 0 = unwritten */
    u32 projection_serial;      /* sNdsStageGxProjSerial its NOZ words hold */
} NDSStageGxRunMemo;
/* A rigid run's world-space box for the cull (ndsStageGxRunOutsideView),
 * valid while world_serial is gNdsStageRigidWorldSerial + 1. Only a program
 * that culls carries them (ndsStageGxCullsRuns: never a VS battle, where
 * they were 28 B of every run's memo: 4,956 B of Hyrule Temple's). */
typedef struct NDSStageGxRunWorld
{
    s32 world_lo[3];
    s32 world_hi[3];
    u32 world_serial;
} NDSStageGxRunWorld;
static NDSStageGxRunMemo *sNdsStageGxRunMemo;
static NDSStageGxRunWorld *sNdsStageGxRunWorld;
/* The segments whose slices this battle loaded (bit per segment): a heap
 * short of the whole program takes the best consecutive range of them
 * (ndsStageGxLoad), and the rest draw uncompiled. */
static u32 sNdsStageGxSegmentMask;
volatile u32 gNdsP2StageProgSegments;
static NDSRendererMatrix20p12 sNdsStageGxProjCopy;
static u32 sNdsStageGxProjSerial, sNdsStageGxProjGen;
/* Bumped by every successful program load (the segment fast path's tables
 * are rebuilt against it). */
static u32 sNdsStageGxLoadSerial;
/* The program's one allocation, which ndsStageGxYield gives back. */
static u8 *sNdsStageGxBody;
static u32 sNdsStageGxBodyBytes;
/* Set from ndsStageGxEligible's yes to the segment commit's last flush: a
 * program never yields under its own draw. */
static u32 sNdsStageGxInSegment;
volatile u32 gNdsP2StageProgYields;

NDS_RENDERER_COLD static u32 ndsStageGxHashBytes(const void *data, u32 bytes, u32 hash)
{
    const u8 *p = data;
    while (bytes-- != 0u) { hash = (hash ^ *p++) * 16777619u; }
    return hash;
}

NDS_RENDERER_COLD static u32 ndsStageGxSourceHash(void)
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

/* The loaded runs [run_lo, run_hi), whose patches and words must lie in the
 * loaded slices [patch_lo, patch_hi) and [word_lo, word_hi). The tables are
 * indexed as the whole program's: a partial load's pointers are biased by
 * its slices' first rows (ndsStageGxLoad). */
NDS_RENDERER_COLD static sb32 ndsStageGxValidate(const NDSStageGxHeader *h,
                               const NDSStageGxRun *runs,
                               const NDSStageGxPatch *patches, const u32 *words,
                               u32 run_lo, u32 run_hi, u32 patch_lo,
                               u32 patch_hi, u32 word_lo, u32 word_hi)
{
    u32 i;
    u64 static_world = (u64)h->static_world_lo | ((u64)h->static_world_hi << 32);
    for (i = run_lo; i < run_hi; i++)
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
            r->first_word < word_lo || (u32)r->first_word + r->word_count > word_hi ||
            r->first_patch < patch_lo ||
            (u32)r->first_patch + r->patch_count > patch_hi) { return FALSE; }
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
                /* The translated painter's depth step, between triangles. */
                case 0x1C: n = 3u;
                    if (vertices == 0u || (vertices % 3u) != 0u || depth != 1u) { return FALSE; } break;
                case 0x16: n = 16u; kind = NDS_STAGE_GX_VIEW; break;
                case 0x18: n = 16u; kind = NDS_STAGE_GX_WORLD; break;
                /* Template 8: an affine baked world as MTX_MULT_4x3. */
                case 0x19: n = 12u; kind = NDS_STAGE_GX_WORLD; break;
                case 0x20: n = 1u; kind = NDS_STAGE_GX_COLOR; break;
                case 0x22: n = 1u; kind = NDS_STAGE_GX_UV; break;
                case 0x23: n = 2u; vertices++; if (depth != 1u) { return FALSE; } break;
                /* Format 10: VTX_XY / XZ / YZ take the other coordinate from
                 * the previous vertex, so never a run's first one. */
                case 0x25: case 0x26: case 0x27: n = 1u;
                    if (vertices++ == 0u || depth != 1u) { return FALSE; } break;
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
                    (op == 0x40 && words[cursor] != 0u) ||
                    (op == 0x1C && (words[cursor] != 0u || words[cursor + 1u] != 0u ||
                                    words[cursor + 2u] != 0xFFFFFFFFu))) { return FALSE; }
                if ((op == 0x18) || (op == 0x19))
                {
                    const NDSNativeStageRun *native = &sNdsNativeStageRuns[i];
                    u32 shift;
                    m4x4 expected;
                    if ((static_world & ((u64)1u << native->binding_index)) == 0u) { return FALSE; }
                    if (vertices >= (u32)r->triangles * 3u || depth != 1u) { return FALSE; }
                    shift = native->submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX ? 0u :
                        native->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX ?
                        sNdsNativeStageWs.runs[i].coordinate_shift :
                        ndsRendererNativeStageTask36TriangleShift(native, vertices / 3u);
                    if (!ndsRendererNativeStageTask36BuildWorld(native->binding_index, shift, &expected)) { return FALSE; }
                    if (op == 0x18)
                    {
                        if (memcmp(&words[cursor], &expected, sizeof(expected)) != 0) { return FALSE; }
                    }
                    else
                    {
                        /* The 4x3 form is the 4x4 less its column 3, which
                         * must be (0, 0, 0, 1.0). */
                        u32 row;
                        if (expected.m[3] != 0 || expected.m[7] != 0 || expected.m[11] != 0 ||
                            expected.m[15] != (1 << 12)) { return FALSE; }
                        for (row = 0u; row < 4u; row++)
                        {
                            if (memcmp(&words[cursor + row * 3u], &expected.m[row * 4u],
                                       3u * sizeof(u32)) != 0) { return FALSE; }
                        }
                    }
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
                          q->kind == NDS_STAGE_GX_COMPOSED || q->kind == NDS_STAGE_GX_CORNER_SOURCE ||
                          q->kind == NDS_STAGE_GX_NOZ_VIEW_RUN ||
                          q->kind == NDS_STAGE_GX_NOZ_COMPOSED_RUN))) ||
                        /* A translated painter run's one matrix consumes
                         * every depth of the run (aux = its triangles). */
                        ((q->kind == NDS_STAGE_GX_NOZ_VIEW_RUN ||
                          q->kind == NDS_STAGE_GX_NOZ_COMPOSED_RUN) &&
                         (sNdsNativeStageRuns[i].submit_class !=
                              NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z ||
                          r->triangles == 0u)) ||
                        (q->kind == NDS_STAGE_GX_NOZ_VIEW_RUN &&
                         (q->index != 0u || q->aux != r->triangles)) ||
                        (q->kind == NDS_STAGE_GX_NOZ_COMPOSED_RUN &&
                         (q->index >= NDS_NATIVE_STAGE_BINDING_COUNT || (q->aux & 7u) > 6u ||
                          (u32)(q->aux >> 3) != r->triangles)) ||
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
    "nitro:/stages/bonus2_purin.gxp", "nitro:/stages/bonus2_ness.gxp",
#if NDS_P5
    /* P5: the Remix stages' templates (scripts/p5/stage_generate.py). */
#define NDS_P5_GX_PATH(g, cname, name, map_file, map_offset) \
    [g] = "nitro:/stages/" #name ".gxp",
    NDS_P5_NATIVE_STAGE_ROWS(NDS_P5_GX_PATH)
#undef NDS_P5_GX_PATH
#endif
};
#define NDS_STAGE_GX_PATH_COUNT \
    (sizeof(sNdsStageGxPaths) / sizeof(sNdsStageGxPaths[0]))

static inline sb32 ndsStageGxCullsRuns(void);

/* Battle-lifetime bytes for a program or a part of one: a VS battle's idle
 * overlay tail first (2026-10-13: a vanilla roster leaves ~170 KB of the
 * loan idle, and Hyrule Temple's 61,936 B body never fit the heap, so the
 * stage drew uncompiled at 20 FPS), otherwise heap with
 * NDS_STAGE_GX_HEAP_KEEP_FREE left over, or NULL. */
NDS_RENDERER_COLD static u8 *ndsStageGxAllocBody(u32 bytes)
{
    u8 *body = ndsVSBattleOverlayTryAlloc(bytes, 32u);

    if (body == NULL)
    {
        if ((uintptr_t)gSYTaskmanGeneralHeap.end < (uintptr_t)gSYTaskmanGeneralHeap.ptr ||
            (uintptr_t)gSYTaskmanGeneralHeap.end - (uintptr_t)gSYTaskmanGeneralHeap.ptr <
                bytes + NDS_STAGE_GX_HEAP_KEEP_FREE)
        { return NULL; }
        body = syTaskmanMalloc(bytes, 32u);
    }
    return body;
}

/* A PROGRAM A SEGMENT RANGE AT A TIME (P7, 2026-10-21). The heaviest mixes
 * of four contents leave Hyrule Temple's frame 0 60,668 B of heap (with the
 * stage's dead bytes lent), and its whole program needs 66,136 B beside the
 * 37,888 B keep-free: it drew uncompiled, ~330K ticks a frame. Format 9's
 * trailer gives each segment's slices and hash, so a heap short of the whole
 * program loads the consecutive segment range worth the most (no-Z
 * triangles, the uncompiled path's expensive ones, weigh three) that fits,
 * and the others draw uncompiled. At most this many segments are ranked. */
#define NDS_STAGE_GX_PARTIAL_SEGMENTS 8u

static sb32 __attribute__((noinline, cold, optimize("Os"))) ndsStageGxLoad(void)
{
    NdsRelocAssetStream stream = { NULL };
    NDSStageGxHeader h;
    NDSStageGxSegment seg[NDS_STAGE_GX_PARTIAL_SEGMENTS];
    u8 *body;
    u32 reason = 1u;
    u32 gkind = sNdsNativeStagePacketActive->gkind;
    u32 memo_unit;
    u32 run_lo, run_hi, patch_lo, patch_hi, word_lo, word_hi;
    u32 mask;
    if ((gkind >= NDS_STAGE_GX_PATH_COUNT) || (sNdsStageGxPaths[gkind] == NULL))
    { return FALSE; }
    if (sNdsStageGxGeneration != gNdsTaskmanHeapGeneration)
    {
        sNdsStageGxGeneration = gNdsTaskmanHeapGeneration;
        sNdsStageGxRuns = NULL; sNdsStageGxAttempted = FALSE;
        sNdsStageGxSegmentMask = 0u;
        sNdsStageGxBody = NULL; sNdsStageGxInSegment = 0u;
    }
    if (sNdsStageGxRuns != NULL) { return TRUE; }
    if (sNdsStageGxAttempted != FALSE) { return FALSE; }
    sNdsStageGxAttempted = TRUE;
    if (!ndsRelocAssetStreamOpen(&stream, sNdsStageGxPaths[gkind]) ||
        !ndsRelocAssetStreamRead(&stream, 0u, &h, sizeof(h))) { goto failed; }
    reason = 2u;
    if (h.magic != 0x31505847u || h.version != 10u || h.gkind != gkind ||
        h.run_count != NDS_NATIVE_STAGE_RUN_COUNT || h.word_count == 0u || h.word_count > 16384u ||
        h.patch_count > 4096u || NDS_NATIVE_STAGE_SEGMENT_COUNT >= 32u ||
        h.segment_mask != (1u << NDS_NATIVE_STAGE_SEGMENT_COUNT) - 1u ||
        (((u64)h.static_world_lo | ((u64)h.static_world_hi << 32)) &
         ~sNdsNativeStageOwnerExecution.rigid_binding_mask) != 0u ||
        h.body_bytes != h.run_count * sizeof(NDSStageGxRun) + h.patch_count * 8u + h.word_count * 4u ||
        h.source_hash != ndsStageGxSourceHash()) { goto failed; }
    reason = 3u;
    memo_unit = sizeof(NDSStageGxRunMemo) +
        ((ndsStageGxCullsRuns() != FALSE) ? sizeof(NDSStageGxRunWorld) : 0u);
    run_lo = 0u; run_hi = h.run_count;
    patch_lo = 0u; patch_hi = h.patch_count;
    word_lo = 0u; word_hi = h.word_count;
    mask = h.segment_mask;
    body = ndsStageGxAllocBody(h.body_bytes + h.run_count * memo_unit);
    if (body != NULL)
    {
        sNdsStageGxBody = body;
        sNdsStageGxBodyBytes = h.body_bytes + h.run_count * memo_unit;
        reason = 4u;
        if (!ndsRelocAssetStreamRead(&stream, sizeof(h), body, h.body_bytes) ||
            ndsStageGxHashBytes(body, h.body_bytes, 2166136261u) != h.body_hash)
        { goto failed; }
        sNdsStageGxRuns = (NDSStageGxRun *)(void *)body;
        sNdsStageGxPatches = (NDSStageGxPatch *)(sNdsStageGxRuns + h.run_count);
        sNdsStageGxWords = (u32 *)(void *)(sNdsStageGxPatches + h.patch_count);
        body += h.body_bytes;
    }
    else
    {
        const u32 table_at = sizeof(h) + h.body_bytes;
        const u32 count = NDS_NATIVE_STAGE_SEGMENT_COUNT;
        u32 value[NDS_STAGE_GX_PARTIAL_SEGMENTS];
        u64 tried = 0u;
        u32 stored = 0u;
        u32 i, a, b;

        if ((count > NDS_STAGE_GX_PARTIAL_SEGMENTS) ||
            !ndsRelocAssetStreamRead(&stream, table_at, &stored, sizeof(stored)) ||
            (stored != count) ||
            !ndsRelocAssetStreamRead(&stream, table_at + sizeof(stored), seg,
                                     count * sizeof(seg[0]))) { goto failed; }
        for (i = 0u; i < count; i++)
        {
            const NDSNativeStageSegment *s = &sNdsNativeStageSegments[i];
            u32 r;

            if ((seg[i].first_run != s->first_run) || (seg[i].run_count != s->run_count) ||
                (seg[i].first_patch != ((i == 0u) ? 0u :
                    (u32)seg[i - 1u].first_patch + seg[i - 1u].patch_count)) ||
                (seg[i].first_word != ((i == 0u) ? 0u :
                    (u32)seg[i - 1u].first_word + seg[i - 1u].word_count)) ||
                ((u32)seg[i].first_run + seg[i].run_count > h.run_count) ||
                ((u32)seg[i].first_patch + seg[i].patch_count > h.patch_count) ||
                ((u32)seg[i].first_word + seg[i].word_count > h.word_count))
            { goto failed; }
            value[i] = 0u;
            for (r = s->first_run; r < (u32)s->first_run + s->run_count; r++)
            {
                const NDSNativeStageRun *native = &sNdsNativeStageRuns[r];

                value[i] += 2u + (u32)native->triangle_count *
                    ((native->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ? 3u : 1u);
            }
        }
        /* The ranges by value, the best that fits first; a refused try
         * allocates nothing. */
        body = NULL;
        while (body == NULL)
        {
            u32 best = 0u, best_a = 0u, best_b = 0u, bytes;

            for (a = 0u; a < count; a++)
            {
                u32 sum = 0u;

                for (b = a; b < count; b++)
                {
                    sum += value[b];
                    if ((((tried >> (a * NDS_STAGE_GX_PARTIAL_SEGMENTS + b)) & 1u) == 0u) &&
                        (sum > best))
                    {
                        best = sum; best_a = a; best_b = b;
                    }
                }
            }
            if (best == 0u) { goto failed; }
            tried |= (u64)1u << (best_a * NDS_STAGE_GX_PARTIAL_SEGMENTS + best_b);
            run_lo = seg[best_a].first_run;
            run_hi = (u32)seg[best_b].first_run + seg[best_b].run_count;
            patch_lo = seg[best_a].first_patch;
            patch_hi = (u32)seg[best_b].first_patch + seg[best_b].patch_count;
            word_lo = seg[best_a].first_word;
            word_hi = (u32)seg[best_b].first_word + seg[best_b].word_count;
            bytes = (run_hi - run_lo) * (sizeof(NDSStageGxRun) + memo_unit) +
                (patch_hi - patch_lo) * sizeof(NDSStageGxPatch) +
                (word_hi - word_lo) * 4u;
            body = ndsStageGxAllocBody(bytes);
            mask = ((1u << (best_b + 1u)) - 1u) & ~((1u << best_a) - 1u);
            sNdsStageGxBody = body;
            sNdsStageGxBodyBytes = bytes;
        }
        reason = 4u;
        {
            u8 *runs = body;
            u8 *patches = runs + (run_hi - run_lo) * sizeof(NDSStageGxRun);
            u8 *words = patches + (patch_hi - patch_lo) * sizeof(NDSStageGxPatch);

            if (!ndsRelocAssetStreamRead(&stream, sizeof(h) + run_lo * sizeof(NDSStageGxRun),
                                         runs, (run_hi - run_lo) * sizeof(NDSStageGxRun)) ||
                !ndsRelocAssetStreamRead(&stream, sizeof(h) + h.run_count * sizeof(NDSStageGxRun) +
                                             patch_lo * sizeof(NDSStageGxPatch),
                                         patches, (patch_hi - patch_lo) * sizeof(NDSStageGxPatch)) ||
                !ndsRelocAssetStreamRead(&stream, sizeof(h) + h.run_count * sizeof(NDSStageGxRun) +
                                             h.patch_count * sizeof(NDSStageGxPatch) + word_lo * 4u,
                                         words, (word_hi - word_lo) * 4u)) { goto failed; }
            for (i = 0u; i < count; i++)
            {
                u32 hash = 2166136261u;

                if ((mask & (1u << i)) == 0u) { continue; }
                hash = ndsStageGxHashBytes(runs + (seg[i].first_run - run_lo) * sizeof(NDSStageGxRun),
                                           seg[i].run_count * sizeof(NDSStageGxRun), hash);
                hash = ndsStageGxHashBytes(patches + (seg[i].first_patch - patch_lo) *
                                               sizeof(NDSStageGxPatch),
                                           seg[i].patch_count * sizeof(NDSStageGxPatch), hash);
                hash = ndsStageGxHashBytes(words + (seg[i].first_word - word_lo) * 4u,
                                           seg[i].word_count * 4u, hash);
                if (hash != seg[i].hash) { goto failed; }
            }
            /* Indexed as the whole program's tables: each pointer is biased
             * by its slice's first row, which only the loaded segments'
             * runs ever index past. */
            sNdsStageGxRuns = (NDSStageGxRun *)(void *)runs - run_lo;
            sNdsStageGxPatches = (NDSStageGxPatch *)(void *)patches - patch_lo;
            sNdsStageGxWords = (u32 *)(void *)words - word_lo;
            body = words + (word_hi - word_lo) * 4u;
        }
    }
    if (!ndsStageGxValidate(&h, sNdsStageGxRuns, sNdsStageGxPatches, sNdsStageGxWords,
                            run_lo, run_hi, patch_lo, patch_hi, word_lo, word_hi))
    { sNdsStageGxRuns = NULL; goto failed; }
    ndsRelocAssetStreamClose(&stream);
    sNdsStageGxStaticWorld = (u64)h.static_world_lo | ((u64)h.static_world_hi << 32);
    /* The memos (and a culling program's world boxes) follow the tables. */
    memset(body, 0, (run_hi - run_lo) * memo_unit);
    sNdsStageGxRunMemo = (NDSStageGxRunMemo *)(void *)body - run_lo;
    sNdsStageGxRunWorld = (memo_unit > sizeof(NDSStageGxRunMemo)) ?
        (NDSStageGxRunWorld *)(void *)(body + (run_hi - run_lo) * sizeof(NDSStageGxRunMemo)) - run_lo :
        NULL;
    sNdsStageGxSegmentMask = mask;
    sNdsStageGxProjSerial++;
    sNdsStageGxProjGen = 0u;
    sNdsStageGxLoadSerial++;
    gNdsP2StageProgLoads++;
    gNdsP2StageProgBytes = (run_hi - run_lo) * (sizeof(NDSStageGxRun) + memo_unit) +
        (patch_hi - patch_lo) * sizeof(NDSStageGxPatch) + (word_hi - word_lo) * 4u;
    gNdsP2StageProgSegments = mask;
    return TRUE;
failed:
    ndsRelocAssetStreamClose(&stream);
    gNdsP2StageProgReason = reason;
    return FALSE;
}

/* Set around the stage select's preview draws (renderer_adapter_stage.c). */
extern volatile u32 gNdsRendererAdapterStagePreviewArmed;

static sb32 ndsStageGxEligible(u32 segment)
{
    u32 gkind = sNdsNativeStagePacketActive->gkind;
    if (gkind >= NDS_STAGE_GX_PATH_COUNT || sNdsStageGxPaths[gkind] == NULL ||
        segment >= NDS_NATIVE_STAGE_SEGMENT_COUNT) { return FALSE; }
    /* The stage select's preview draws uncompiled: a program's static worlds
     * are the battle's (the preview scales each layer's root), its body is
     * battle-lifetime heap, and the select shows one stage after another
     * within one heap generation. */
    if (gNdsRendererAdapterStagePreviewArmed != 0u) { return FALSE; }
    if (!ndsStageGxLoad()) { NDS_DIAG(gNdsP2StageProgDeclines++); return FALSE; }
    /* A partial load's other segments draw uncompiled. */
    if ((sNdsStageGxSegmentMask & (1u << segment)) == 0u) { return FALSE; }
    if ((sNdsStageGxStaticWorld & ~sNdsNativeStageOwnerExecution.rigid_binding_mask) != 0u)
    { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 7u; return FALSE; }
    sNdsStageGxInSegment = 1u;
    return TRUE;
}

/* THE PROGRAM GIVES WAY TO THE FLOOR (P7, 2026-10-21). A program loads at
 * the first stage draw with 12 KB over the 25,600 B floor, and cannot know
 * what the match will ask for later: Congo Jungle with Ganondorf, Peach,
 * Snake and Kirby fell to 24,788 B (Kirby's copy hat, 7,636 B at f1401,
 * then the source's object pools, 10 KB), and those pools keep growing
 * through a five-minute match (14.8-19.8 KB). syTaskmanMalloc asks this
 * before the heap would fall under the floor: outside a segment's draw, and
 * once the DMA that may still read the words is done, the program's bytes
 * become a lent span, which the allocation takes, and the stage draws
 * uncompiled for the rest of the battle. */
void ndsBattleDeadSpanAdd(void *base, u32 bytes);
extern volatile u32 gNdsBattleDeadSpanLentBytes;

s32 __attribute__((noinline, cold)) ndsStageGxYield(size_t size)
{
    u32 lent;

    /* Only for an allocation the program's bytes can hold (32 B for the
     * span's alignment). */
    if ((sNdsStageGxRuns == NULL) || (sNdsStageGxBody == NULL) ||
        (sNdsStageGxGeneration != gNdsTaskmanHeapGeneration) ||
        (sNdsStageGxInSegment != 0u) || (sNdsStageGxWordCount != 0u) ||
        (size + 32u > sNdsStageGxBodyBytes))
    {
        return FALSE;
    }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    lent = gNdsBattleDeadSpanLentBytes;
    ndsBattleDeadSpanAdd(sNdsStageGxBody, sNdsStageGxBodyBytes);
    if (gNdsBattleDeadSpanLentBytes == lent)
    {
        return FALSE;
    }
    /* sNdsStageGxAttempted stays set: the battle never reloads it. */
    sNdsStageGxRuns = NULL;
    sNdsStageGxPatches = NULL;
    sNdsStageGxWords = NULL;
    sNdsStageGxRunMemo = NULL;
    sNdsStageGxRunWorld = NULL;
    sNdsStageGxSegmentMask = 0u;
    sNdsStageGxDmaFirst = 0u;
    sNdsStageGxDmaEnd = 0u;
    sNdsStageGxLoadSerial++;
    sNdsStageGxBody = NULL;
    gNdsP2StageProgYields++;
    return TRUE;
}

static inline __attribute__((always_inline)) sb32 ndsStageGxInsideNearBox(
    const s16 *minimum, const s16 *maximum, const NDSRendererMatrix20p12 *matrix)
{
    s64 z = matrix->m[3][2], w = matrix->m[3][3];
    u32 axis;
    /* Independent minima are conservative, including the CPU clipper's s32
     * saturation. Six multiplies replace a per-vertex test on the common path. */
    for (axis = 0u; axis < 3u; axis++)
    {
        s32 cz = matrix->m[axis][2], cw = matrix->m[axis][3];
        z += (s64)cz * (cz >= 0 ? minimum[axis] : maximum[axis]);
        w += (s64)cw * (cw >= 0 ? minimum[axis] : maximum[axis]);
    }
    /* Source-Z cross corners also require nonzero W after rounding to /256. */
    return w >= 128 && (s64)ndsRendererClampS64ToS32(z) + ndsRendererClampS64ToS32(w) >= 0;
}

static inline sb32 ndsStageGxInsideNear(const NDSStageGxRun *run, const NDSRendererMatrix20p12 *matrix)
{
    return ndsStageGxInsideNearBox(run->minimum, run->maximum, matrix);
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
 * until measured. */
#define NDS_STAGE_GX_CULL_FIRST_GKIND 9u /* nGRKindPupupuSmall: 1P venues on */
/* The 1P venues cull their runs and take the per-run path. A VS battle frames
 * its whole stage, so it takes the segment's one-DMA path whatever its gkind:
 * the P5 stages reuse 1P venues' gkinds (13, 14, 16) or come after them
 * (41-44), and Dinosaur Land's per-run draws and NoZ read-backs cost ~106K
 * cycles a frame over Dream Land's (2026-10-18 profile). */
static inline sb32 ndsStageGxCullsRuns(void)
{
    return ((sNdsNativeStagePacketActive->gkind >= NDS_STAGE_GX_CULL_FIRST_GKIND) &&
            (ndsSceneIsVSBattle() == FALSE)) ? TRUE : FALSE;
}
volatile u32 gNdsStageGxCulledRuns;
/* P2-6 (2026-10-02): A RIGID RUN TESTS ITS WORLD BOX. A rigid binding's world
 * is constant until the next capture, so its runs' object-space boxes go to
 * world space once (the eight corners through the world, floor and ceiling
 * at the integer unit: never smaller) and the cull tests them against one
 * camera x projection a frame, where it used to form world x camera x
 * projection, two products, at every binding switch. The Race's 18 static
 * bindings made rigid (generate_nds_native_stage.py) would otherwise have
 * paid that ~36 times a frame. */
extern volatile u32 gNdsStageRigidWorldSerial;
static NDSRendererMatrix20p12 sNdsStageGxCullCameraProjection;
static u32 sNdsStageGxCullCameraGen = UINT_MAX;

static void ndsStageGxRigidWorldBox(const NDSStageGxRun *r,
                                    const NDSRendererMatrix20p12 *w,
                                    NDSStageGxRunWorld *memo)
{
    u32 corner;
    u32 axis;
    s64 lo[3] = { INT64_MAX, INT64_MAX, INT64_MAX };
    s64 hi[3] = { INT64_MIN, INT64_MIN, INT64_MIN };

    for (corner = 0u; corner < 8u; corner++)
    {
        const s64 x = ((corner & 1u) != 0u) ? r->maximum[0] : r->minimum[0];
        const s64 y = ((corner & 2u) != 0u) ? r->maximum[1] : r->minimum[1];
        const s64 z = ((corner & 4u) != 0u) ? r->maximum[2] : r->minimum[2];

        for (axis = 0u; axis < 3u; axis++)
        {
            const s64 v = (x * w->m[0][axis]) + (y * w->m[1][axis]) +
                (z * w->m[2][axis]) + (s64)w->m[3][axis];

            lo[axis] = (v < lo[axis]) ? v : lo[axis];
            hi[axis] = (v > hi[axis]) ? v : hi[axis];
        }
    }
    for (axis = 0u; axis < 3u; axis++)
    {
        /* Q12 to units, outward: floor the low side, ceiling the high. */
        memo->world_lo[axis] = (s32)(lo[axis] >> 12);
        memo->world_hi[axis] = (s32)((hi[axis] + 4095) >> 12);
    }
    memo->world_serial = gNdsStageRigidWorldSerial + 1u;
}

static sb32 ndsStageGxRunOutsideView(u32 index)
{
    const NDSStageGxRun *r = &sNdsStageGxRuns[index];
    const NDSRendererMatrix20p12 *m;
    const s32 *box_lo;
    const s32 *box_hi;
    s32 local_lo[3];
    s32 local_hi[3];
    u32 binding;
    u32 plane;
    u32 axis;

    if ((ndsStageGxCullsRuns() == FALSE) ||
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
        NDSStageGxRunWorld *memo;

        if ((sNdsStageGxRunWorld == NULL) ||
            (sNdsNativeStageOwnerExecution.binding_world == NULL) ||
            (sNdsNativeStageOwnerExecution.camera_modelview == NULL) ||
            (sNdsNativeStageOwnerExecution.projection == NULL))
        {
            return FALSE;
        }
        memo = &sNdsStageGxRunWorld[index];
        if (memo->world_serial != gNdsStageRigidWorldSerial + 1u)
        {
            ndsStageGxRigidWorldBox(
                r, &sNdsNativeStageOwnerExecution.binding_world[binding],
                memo);
        }
        if (sNdsStageGxCullCameraGen != sNdsNativeStageMatrixGen)
        {
            ndsRendererMtxMul20p12(sNdsNativeStageOwnerExecution.camera_modelview,
                                   sNdsNativeStageOwnerExecution.projection,
                                   &sNdsStageGxCullCameraProjection);
            sNdsStageGxCullCameraGen = sNdsNativeStageMatrixGen;
        }
        m = &sNdsStageGxCullCameraProjection;
        box_lo = memo->world_lo;
        box_hi = memo->world_hi;
    }
    else
    {
        if (sNdsNativeStageOwnerExecution.binding_composed == NULL)
        {
            return FALSE;
        }
        m = &sNdsNativeStageOwnerExecution.binding_composed[binding];
        for (axis = 0u; axis < 3u; axis++)
        {
            local_lo[axis] = r->minimum[axis];
            local_hi[axis] = r->maximum[axis];
        }
        box_lo = local_lo;
        box_hi = local_hi;
    }
    for (plane = 0u; plane < 4u; plane++)
    {
        u32 col = plane >> 1;
        s64 sign = ((plane & 1u) != 0u) ? 1 : -1;
        s64 d = (s64)m->m[3][3] + (sign * (s64)m->m[3][col]);

        for (axis = 0u; axis < 3u; axis++)
        {
            s64 c = (s64)m->m[axis][3] + (sign * (s64)m->m[axis][col]);

            d += c * (s64)((c >= 0) ? box_hi[axis] : box_lo[axis]);
        }
        if (d < 0)
        {
            NDS_DIAG(gNdsStageGxCulledRuns++);
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

/* The stage hash below -- lab ROMs only, and not profile ROMs: hashing every
 * FIFO word dominated the stage rows of the late-window lab profile. */
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP && \
    !(defined(NDS_TASK37_PROFILE) && NDS_TASK37_PROFILE)
#define NDS_LAB_STAGE_GX_HASH 1
#else
#define NDS_LAB_STAGE_GX_HASH 0
#endif

#if NDS_LAB_STAGE_GX_HASH
/* LAB: every word the stage program sends to the FIFO, every span state and
 * each segment's closing depth and counters, hashed in order. Read and reset
 * once per logic frame by a gdb harness; the per-run loop and the segment fast
 * path must produce the same value. */
volatile u32 gNdsLabStageGxHash = 2166136261u;
volatile u32 gNdsLabStageGxHashWords;
/* The span states apart: they are registers outside the FIFO, written in the
 * same order by both paths but interleaved differently with the words. */
volatile u32 gNdsLabStageGxStateHash = 2166136261u;
static inline void ndsLabStageGxHashWord(u32 w)
{
    gNdsLabStageGxHash = (gNdsLabStageGxHash ^ w) * 16777619u;
}
#endif

static void ndsStageGxFlush(void)
{
    if (sNdsStageGxWordCount == 0u) { return; }
#if NDS_LAB_STAGE_GX_HASH
    {
        u32 lab_i;

        for (lab_i = 0u; lab_i < sNdsStageGxWordCount; lab_i++)
        {
            ndsLabStageGxHashWord(sNdsStageGxWords[sNdsStageGxFirstWord + lab_i]);
        }
        NDS_DIAG(gNdsLabStageGxHashWords += sNdsStageGxWordCount);
    }
#endif
    NDS_FIGHTER_PACKET_DMA_WAIT();
    DC_FlushRange(&sNdsStageGxWords[sNdsStageGxFirstWord], sNdsStageGxWordCount * 4u);
    DMA_SRC(0) = (u32)(uintptr_t)&sNdsStageGxWords[sNdsStageGxFirstWord];
    DMA_DEST(0) = (u32)(uintptr_t)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | sNdsStageGxWordCount;
    sNdsFighterPacketDmaPending = 1u;
    sNdsStageGxDmaFirst = sNdsStageGxFirstWord;
    sNdsStageGxDmaEnd = sNdsStageGxFirstWord + sNdsStageGxWordCount;
    sNdsStageGxWordCount = 0u;
    NDS_DIAG(gNdsP2StageProgDmas++);
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
#if NDS_LAB_STAGE_GX_HASH
    gNdsLabStageGxStateHash = (gNdsLabStageGxStateHash ^ alpha) * 16777619u;
#endif
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
 * these registers -- only the DMA they no longer wait behind has moved. */

static void ndsStageGxAppend(const NDSStageGxRun *run,
                           const NDSNativeStagePreparedRun *prepared)
{
    u32 alpha = prepared->alpha_test != 0u ? 0x100u | prepared->alpha_ref : 0u;
    sb32 state_written = FALSE;

    if (sNdsStageGxWordCount != 0u &&
        (run->first_word != sNdsStageGxFirstWord + sNdsStageGxWordCount ||
         alpha != sNdsStageGxAlpha))
    {
        ndsStageGxSetSpanState(alpha, prepared);
        state_written = TRUE;
        ndsStageGxFlush();
    }
    if (sNdsStageGxWordCount == 0u)
    {
        /* Alpha-test registers are outside the FIFO; never merge across a
         * hidden run or state edge. Only an open Task 36 segment writes the
         * FIFO here (its matrix mode and pop), and only that has to follow
         * the previous span's DMA. */
        if (sNdsNativeStageOwnerExecution.task36_segment_active != FALSE)
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

/* P2-2p8 (2026-10-04): the translated painter (template version 6,
 * compile_nds_stage_gx.py NOZ_VIEW_RUN). A no-Z run loads one clip transform
 * into the position matrix -- a static run the camera's VIEW affine times the
 * projection (its baked world multiplies onto it in the FIFO, as onto VIEW),
 * a composed run its composed matrix -- with the z column at w x the run's
 * first depth. The projection matrix is identity, and each later triangle's
 * MTX_TRANS(0, 0, -1) makes it a clip-space translation one depth step
 * nearer: z' = z + tz * w leaves x, y and w alone. The run consumes all its
 * depths at the load, as its per-triangle loads did. View x projection is
 * built once per stage-matrix generation, like the VIEW affine. */
static NDSRendererMatrix20p12 sNdsStageGxViewProjMemo;
static u32 sNdsStageGxViewProjGen;

static const NDSRendererMatrix20p12 *ndsStageGxViewProjection(void)
{
    if (sNdsStageGxViewProjGen != sNdsNativeStageMatrixGen + 1u)
    {
        NDSRendererMatrix20p12 view =
            *sNdsNativeStageOwnerExecution.camera_modelview;
        u32 col;

        /* ndsRendererNativeBuildHierarchyHardwareAffine's scaling. */
        for (col = 0u; col < 3u; col++)
        {
            view.m[3][col] = ndsRendererRoundShiftS32Signed(
                view.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
        }
        ndsRendererMtxMul20p12(&view, sNdsNativeStageOwnerExecution.projection,
                               &sNdsStageGxViewProjMemo);
        sNdsStageGxViewProjGen = sNdsNativeStageMatrixGen + 1u;
    }
    return &sNdsStageGxViewProjMemo;
}

/* The run's matrix without its z column (written with the depth). */
static sb32 __attribute__((noinline, cold)) ndsStageGxNoZRunMatrix(
    u32 *dst, const NDSStageGxPatch *q)
{
    NDSRendererMatrix20p12 composed;

    if (q->kind == NDS_STAGE_GX_NOZ_VIEW_RUN)
    {
        memcpy(dst, ndsStageGxViewProjection(), sizeof(composed));
        return TRUE;
    }
    if (!ndsStageGxComposedFor(q->index, q->aux & 7u, &composed))
    {
        return FALSE;
    }
    memcpy(dst, &composed, sizeof(composed));
    return TRUE;
}

/* The triangles a translated painter patch steps through. */
static inline u32 ndsStageGxNoZRunTriangles(const NDSStageGxPatch *q)
{
    return (q->kind == NDS_STAGE_GX_NOZ_VIEW_RUN) ? (u32)q->aux :
        ((u32)q->aux >> 3);
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
        if (q->kind == NDS_STAGE_GX_NOZ || q->kind == NDS_STAGE_GX_COMPOSED_NOZ || q->kind == NDS_STAGE_GX_CORNER_NOZ ||
            q->kind >= NDS_STAGE_GX_NOZ_VIEW_RUN)
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
            if (q->kind >= NDS_STAGE_GX_NOZ_VIEW_RUN)
            {
                /* The run's later triangles step by MTX_TRANS. */
                sNdsRendererHardwareProjectedDepth -=
                    NDS_RENDERER_HW_PROJECTED_DEPTH_STEP *
                    (s32)(ndsStageGxNoZRunTriangles(q) - 1u);
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

/* Main RAM since 2026-10-04: every stage program is head-ordered (format 7)
 * and its segments take ndsStageGxCommitFast, so this runs only for a
 * declined segment; its 3,576 B of ITCM went to the fighter joint walk. */
static sb32 __attribute__((noinline)) ndsStageGxDraw(u32 index, u32 owner, NDSRendererStats *stats)
{
    const NDSStageGxRun *r = &sNdsStageGxRuns[index];
    const NDSNativeStageRun *native = &sNdsNativeStageRuns[index];
    const NDSNativeStagePreparedRun *prepared = &sNdsNativeStageWs.runs[index];
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
    if ((r->first_word < sNdsStageGxDmaEnd) &&
        ((u32)r->first_word + r->word_count > sNdsStageGxDmaFirst))
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
        else if (q->kind >= NDS_STAGE_GX_NOZ_VIEW_RUN)
        {
            /* A translated painter run: its depth is written by the
             * read-back pass, which steps the run's other triangles. */
            if (!ndsStageGxNoZRunMatrix(dst, q))
            { gNdsP2StageProgDeclines++; gNdsP2StageProgReason = 8u; return FALSE; }
            noz_local = FALSE;
        }
    }
    /* Every patch of the run was written or proven current. */
    m->projection_serial = projection_serial;
    m->poly_fmt = prepared->poly_fmt;
    m->texture_name = prepared->texture_name;
    m->texture_params = prepared->texture_params;
    m->textured = prepared->textured;
    m->material_gen = sNdsRendererHardwareTextureKeyGeneration + 1u;
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

/* ---- P2-2p8 (2026-10-04): the segment fast path ----------------------------
 *
 * ndsStageGxDraw runs once per run. On Dream Land's 54 runs a frame most of
 * its ~50K ticks were the run's own records -- the run, the prepared run, the
 * memo, the texture entry, each a main-RAM line, fetched while DMA0 streamed
 * the previous span -- and 27 span flushes. A segment whose runs are
 * contiguous in words and patches, with no hidden binding, no cull and no
 * near-plane run, is committed here in one pass over its patches instead:
 *  - a run starts at its MATERIAL patch (the validator puts that word second
 *    in the run, so it is the run's first patch), and a non-no-Z run makes
 *    its foreground flip there, before any of its depths;
 *  - every other patch is written as ndsStageGxDraw writes it, and a painter
 *    patch's depth column right after its matrix: depths are taken in patch
 *    order, as ndsStageGxDraw's second loop takes them;
 *  - material and projection words are proven current by one compare each
 *    (the prepared table's serial, the texture key generation, the projection
 *    serial) once a pass has verified every run's memo against them; the
 *    per-run loop forgets that proof whenever it commits the segment;
 *  - the texture stamps, the accounting and the span states follow in run
 *    order, and the segment's words reach the FIFO in one DMA: the spans of
 *    ndsStageGxAppend split only where the alpha state changes, and those
 *    registers are outside the FIFO, so the word stream is the same.
 * A failure once the depth counter has moved restores it and returns FALSE;
 * the per-run loop then commits the segment from the same state. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
#define NDS_STAGE_GX_FAST_LIVE 1
#else
#define NDS_STAGE_GX_FAST_LIVE 0
#endif

#if NDS_STAGE_GX_FAST_LIVE
/* P2-2p8 (2026-10-05): the fast commit's per-run texture accounting is summed
 * once after the run walk, and its class accounting adds only the (at most
 * two) classes the segment's runs hold instead of all eight -- the same
 * totals, without a read-modify-write per run and per class (accounting and
 * run loop were ~19K cycles of the commit's ~72K a frame on Dream Land). */
volatile u32 gNdsStageGxFastCommits;
volatile u32 gNdsStageGxFastDeclines;
#define NDS_STAGE_GX_FAST_SEGMENTS 32u
#define NDS_STAGE_GX_FAST_SPANS 4u
/* A segment whose hidden subset keeps moving goes back to declining after
 * this many rebuilds (a build is a cold walk of the segment's runs). */
#define NDS_STAGE_GX_FAST_VARIANT_BUILDS 8u
#define NDS_STAGE_GX_FAST_NEAR_MAX 96u
typedef struct NDSStageGxFastSeg
{
    u32 load_serial;        /* sNdsStageGxLoadSerial + 1 these were built for */
    u32 fresh_projection;   /* projection serial + 1 every run's words hold */
    u32 fresh_prepared;     /* prepared serial + 1 ... */
    u32 fresh_texgen;       /* texture key generation + 1 ... */
    u64 binding_mask;       /* every run's binding: the hidden test */
    u32 triangles;
    u32 reuse;              /* sum of (triangles - 1) over the runs */
    u32 class_triangles[NDS_RENDERER_HW_SUBMIT_CLASS_COUNT];
    u32 raw_triangles;      /* class_triangles[RAW_Z_CURRENT_MATRIX] */
    u8 class_nz[2];         /* the classes with triangles, when at most two */
    u8 class_nz_count;      /* ... how many; 0xff: more than two */
    u8 pad_nz;
    u16 first_patch;
    u16 patch_end;
    u16 first_word;
    u16 word_end;
    u16 near_first;
    u8 near_count;
    u8 eligible;
    /* P2-2p8 (2026-10-05): hidden bindings. The per-run path skips a hidden
     * binding's runs whole (no words, no state, no depths); a segment built
     * for that hidden subset leaves the same runs out -- their patches are
     * skipped and their words cut out of the DMA as span breaks -- so the
     * fast commit no longer declines a segment with a hidden binding. Every
     * run's words open with its own material state (MATERIAL is each run's
     * first patch), so a cut-out run leaves nothing the next run reads.
     * Sector Z's segment 0 holds the owner-hidden wing-platform proxy on
     * every frame. */
    u64 all_binding_mask;   /* every run's binding, hidden or not */
    u64 skip_mask;          /* the hidden bindings this build left out */
    u8 span_count;          /* 1 = [first_word, word_end) whole */
    u8 variant_builds;      /* hidden-subset rebuilds since the load */
    u16 span_word[NDS_STAGE_GX_FAST_SPANS][2];
} NDSStageGxFastSeg;
/* One near-plane box per (segment, binding) over the no-Z and cross-matrix
 * runs that read the binding: inside for the union is inside for each run. */
typedef struct NDSStageGxFastNear
{
    s16 minimum[3];
    s16 maximum[3];
    u8 binding;
    u8 pad;
} NDSStageGxFastNear;
static NDSStageGxFastSeg sNdsStageGxFastSegs[NDS_STAGE_GX_FAST_SEGMENTS];
static NDSStageGxFastNear sNdsStageGxFastNear[NDS_STAGE_GX_FAST_NEAR_MAX];
static u32 sNdsStageGxFastNearUsed;
static u32 sNdsStageGxFastNearSerial;

static void ndsStageGxFastForget(u32 segment_index)
{
    if (segment_index < NDS_STAGE_GX_FAST_SEGMENTS)
    {
        sNdsStageGxFastSegs[segment_index].fresh_projection = 0u;
        sNdsStageGxFastSegs[segment_index].fresh_prepared = 0u;
        sNdsStageGxFastSegs[segment_index].fresh_texgen = 0u;
    }
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsStageGxFastBuild(u32 segment_index, const NDSNativeStageSegment *segment,
                    u64 hidden)
{
    /* A DLLink segment's runs in the head passes' order (0, 2, 1, 3), the
     * order scripts/stages/compile_nds_stage_gx.py (format 7) lays their words
     * out in; a layer packet's in index order. */
    static const u8 head_order[4] = { 0u, 2u, 1u, 3u };
    const u8 *heads = NDS_NATIVE_STAGE_BINDING_HEADS;
    const u32 passes = (heads != NULL) ? 4u : 1u;
    NDSStageGxFastSeg *s = &sNdsStageGxFastSegs[segment_index];
    const u32 variant_builds = (s->load_serial == sNdsStageGxLoadSerial + 1u) ?
        (u32)s->variant_builds + 1u : 0u;
    u32 built = 0u;
    u32 open = 0u;          /* the last run seen was included */
    u32 pass;
    u32 i;

    if (sNdsStageGxFastNearSerial != sNdsStageGxLoadSerial + 1u)
    {
        sNdsStageGxFastNearSerial = sNdsStageGxLoadSerial + 1u;
        sNdsStageGxFastNearUsed = 0u;
    }
    memset(s, 0, sizeof(*s));
    s->load_serial = sNdsStageGxLoadSerial + 1u;
    s->variant_builds = (u8)((variant_builds > 255u) ? 255u : variant_builds);
    s->near_first = (u16)sNdsStageGxFastNearUsed;
    if (segment->run_count == 0u)
    {
        return;
    }
    for (pass = 0u; pass < passes; pass++)
    for (i = 0u; i < segment->run_count; i++)
    {
        const u32 run_index = (u32)segment->first_run + i;
        const NDSStageGxRun *r = &sNdsStageGxRuns[run_index];
        const NDSNativeStageRun *native = &sNdsNativeStageRuns[run_index];
        const u32 submit_class =
            (native->submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX :
                native->submit_class;
        u64 bindings;

        if (heads != NULL)
        {
            /* A head the passes never draw: not this path's segment. */
            if (heads[native->binding_index] > 3u)
            {
                return;
            }
            if (heads[native->binding_index] != head_order[pass])
            {
                continue;
            }
        }
        if ((r->word_count < 4u) || (r->patch_count == 0u) ||
            (native->submit_class >= NDS_RENDERER_HW_SUBMIT_CLASS_COUNT) ||
            (native->binding_index >= 64u) ||
            (sNdsStageGxPatches[r->first_patch].kind != NDS_STAGE_GX_MATERIAL) ||
            (sNdsStageGxPatches[r->first_patch].index != run_index))
        {
            return;
        }
        if (built++ == 0u)
        {
            s->first_patch = r->first_patch;
            s->first_word = r->first_word;
        }
        else if ((r->first_patch != s->patch_end) ||
                 (r->first_word != s->word_end))
        {
            return;
        }
        s->patch_end = (u16)(r->first_patch + r->patch_count);
        s->word_end = (u16)(r->first_word + r->word_count);
        s->all_binding_mask |= (u64)1u << native->binding_index;
        if (((hidden >> native->binding_index) & 1u) != 0u)
        {
            /* Left out: its words end the open span. */
            s->skip_mask |= (u64)1u << native->binding_index;
            open = 0u;
            continue;
        }
        if (open == 0u)
        {
            if (s->span_count >= NDS_STAGE_GX_FAST_SPANS)
            {
                return;
            }
            s->span_word[s->span_count][0] = r->first_word;
            s->span_count++;
            open = 1u;
        }
        s->span_word[s->span_count - 1u][1] =
            (u16)(r->first_word + r->word_count);
        s->binding_mask |= (u64)1u << native->binding_index;
        s->triangles += native->triangle_count;
        s->reuse += (native->triangle_count != 0u) ?
            (u32)native->triangle_count - 1u : 0u;
        s->class_triangles[submit_class] += native->triangle_count;
        if ((native->submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) &&
            ((native->flags & NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) ==
             0u))
        {
            continue;
        }
        bindings = (u64)r->binding_lo | ((u64)r->binding_hi << 32);
        while (bindings != 0u)
        {
            const u32 low = (u32)bindings;
            const u32 binding = (low != 0u) ? (u32)__builtin_ctz(low) :
                32u + (u32)__builtin_ctz((u32)(bindings >> 32));
            NDSStageGxFastNear *n = NULL;
            u32 k;
            u32 axis;

            bindings &= bindings - 1u;
            for (k = 0u; k < s->near_count; k++)
            {
                if (sNdsStageGxFastNear[s->near_first + k].binding == binding)
                {
                    n = &sNdsStageGxFastNear[s->near_first + k];
                    break;
                }
            }
            if (n == NULL)
            {
                if ((sNdsStageGxFastNearUsed >= NDS_STAGE_GX_FAST_NEAR_MAX) ||
                    (s->near_count == 0xffu))
                {
                    return;
                }
                n = &sNdsStageGxFastNear[sNdsStageGxFastNearUsed++];
                s->near_count++;
                n->binding = (u8)binding;
                for (axis = 0u; axis < 3u; axis++)
                {
                    n->minimum[axis] = r->minimum[axis];
                    n->maximum[axis] = r->maximum[axis];
                }
                continue;
            }
            for (axis = 0u; axis < 3u; axis++)
            {
                if (r->minimum[axis] < n->minimum[axis]) { n->minimum[axis] = r->minimum[axis]; }
                if (r->maximum[axis] > n->maximum[axis]) { n->maximum[axis] = r->maximum[axis]; }
            }
        }
    }
    /* The commit's accounting adds only the classes that hold triangles. */
    for (i = 0u; i < NDS_RENDERER_HW_SUBMIT_CLASS_COUNT; i++)
    {
        if (s->class_triangles[i] == 0u)
        {
            continue;
        }
        if (s->class_nz_count >= 2u)
        {
            s->class_nz_count = 0xffu;
            break;
        }
        s->class_nz[s->class_nz_count++] = (u8)i;
    }
    s->raw_triangles =
        s->class_triangles[NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX];
    s->eligible = 1u;
}

/* ndsStageGxDraw's painter column: the four rounded Z-column writes. */
static inline void ndsStageGxFastDepthColumn(u32 *dst,
                                             const NDSRendererMatrix20p12 *w,
                                             s16 depth)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        dst[row * 4u + 2u] = (u32)(s32)ndsRendererRoundShiftS64(
            (s64)w->m[row][3] * depth, 12u);
    }
}

/* LAB (stage/roster words builds only): why a segment's fast commit declined,
 * one counter a reason, read by gdb. 1 eligible, 2 hidden binding, 3 near
 * box, 4 material, 5 composed, 6 painter composed. */
#if defined(NDS_LAB_FOURCPU_WORDS) && NDS_LAB_FOURCPU_WORDS
volatile u32 gNdsLabStageGxFastWhy[8] __attribute__((used));
#define NDS_STAGE_GX_FAST_WHY(n) (gNdsLabStageGxFastWhy[(n)]++)
#else
#define NDS_STAGE_GX_FAST_WHY(n) ((void)0)
#endif

static sb32 __attribute__((noinline)) ndsStageGxCommitFast(
    u32 segment_index, const NDSNativeStageSegment *segment,
    NDSRendererStats *stats, u32 *segment_triangles)
{
    const NDSRendererMatrix20p12 *projection =
        sNdsNativeStageOwnerExecution.projection;
    const NDSRendererMatrix20p12 *composed_all =
        sNdsNativeStageOwnerExecution.binding_composed;
    NDSStageGxFastSeg *s;
    NDSRendererMatrix20p12 composed;
    u64 rigid;
    u32 projection_serial;
    u32 prepared_serial;
    u32 texgen;
    sb32 projection_fresh;
    sb32 material_fresh;
    s32 depth_saved;
    u32 background_saved;
    s32 composed_shift = -1;
    u32 composed_binding = UINT_MAX;
    u32 corner_triangle = UINT_MAX;
    s16 corner_depth = 0;
    u32 span_alpha = UINT_MAX;
    u32 p;
    u32 i;

    if ((gNdsNativeStageRunWitness != 0u) ||
        (segment_index >= NDS_STAGE_GX_FAST_SEGMENTS) || (stats == NULL) ||
        (projection == NULL) || (composed_all == NULL) ||
        (sNdsNativeStageOwnerExecution.camera_modelview == NULL) ||
        (ndsStageGxCullsRuns() != FALSE))
    {
        return FALSE;
    }
    s = &sNdsStageGxFastSegs[segment_index];
    if (s->load_serial != sNdsStageGxLoadSerial + 1u)
    {
        ndsStageGxFastBuild(segment_index, segment,
            sNdsNativeStageOwnerExecution.hidden_binding_mask);
    }
    else if ((s->eligible != 0u) &&
             ((sNdsNativeStageOwnerExecution.hidden_binding_mask &
               s->all_binding_mask) != s->skip_mask) &&
             (s->variant_builds < NDS_STAGE_GX_FAST_VARIANT_BUILDS))
    {
        ndsStageGxFastBuild(segment_index, segment,
                            sNdsNativeStageOwnerExecution.hidden_binding_mask);
    }
    if ((s->eligible == 0u) ||
        ((s->skip_mask != 0u) &&
         ((sNdsNativeStageOwnerExecution.hidden_binding_mask &
           s->all_binding_mask) != s->skip_mask)) ||
        ((sNdsNativeStageOwnerExecution.hidden_binding_mask & s->binding_mask) !=
         0u))
    {
        NDS_STAGE_GX_FAST_WHY((s->eligible == 0u) ? 1u : 2u);
        NDS_DIAG(gNdsStageGxFastDeclines++);
        return FALSE;
    }
    rigid = sNdsNativeStageOwnerExecution.rigid_binding_mask;
    for (i = 0u; i < s->near_count; i++)
    {
        const NDSStageGxFastNear *n = &sNdsStageGxFastNear[s->near_first + i];

        if ((((rigid >> n->binding) & 1u) == 0u) &&
            (ndsStageGxInsideNearBox(n->minimum, n->maximum,
                                     &composed_all[n->binding]) == FALSE))
        {
            NDS_STAGE_GX_FAST_WHY(3u);
            NDS_DIAG(gNdsStageGxFastDeclines++);
            return FALSE;
        }
    }
    projection_serial = ndsStageGxProjectionSerial();
    prepared_serial = sNdsNativeStagePreparedSerial;
    texgen = sNdsRendererHardwareTextureKeyGeneration;
    projection_fresh = (s->fresh_projection == projection_serial + 1u) ?
        TRUE : FALSE;
    material_fresh = ((s->fresh_prepared == prepared_serial + 1u) &&
                      (s->fresh_texgen == texgen + 1u)) ? TRUE : FALSE;
    depth_saved = sNdsRendererHardwareProjectedDepth;
    background_saved = sNdsRendererHardwareProjectedBackground;

    for (p = s->first_patch; p < s->patch_end; p++)
    {
        const NDSStageGxPatch *q = &sNdsStageGxPatches[p];
        u32 *dst = &sNdsStageGxWords[q->word];
        const u32 kind = q->kind;

        if (kind == NDS_STAGE_GX_MATERIAL)
        {
            const u32 run_index = q->index;
            const NDSNativeStageRun *native = &sNdsNativeStageRuns[run_index];

            if ((s->skip_mask != 0u) &&
                (((s->skip_mask >> native->binding_index) & 1u) != 0u))
            {
                /* A left-out run: past its patches (MATERIAL is its first). */
                p = (u32)sNdsStageGxRuns[run_index].first_patch +
                    sNdsStageGxRuns[run_index].patch_count - 1u;
                continue;
            }
            composed_shift = -1;
            composed_binding = UINT_MAX;
            corner_triangle = UINT_MAX;
            if (native->submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                ndsRendererHardwareEnterProjectedForeground();
            }
            if (material_fresh == FALSE)
            {
                const NDSStageGxRunMemo *m = &sNdsStageGxRunMemo[run_index];
                const NDSNativeStagePreparedRun *prepared =
                    &sNdsNativeStageWs.runs[run_index];

                if (((m->material_gen != texgen + 1u) ||
                     (m->poly_fmt != prepared->poly_fmt) ||
                     (m->texture_name != prepared->texture_name) ||
                     (m->texture_params != prepared->texture_params) ||
                     (m->textured != prepared->textured)) &&
                    (ndsStageGxMaterial(dst, prepared, native->submit_class,
                                        segment->owner) == FALSE))
                {
                    NDS_STAGE_GX_FAST_WHY(4u);
                    goto declined;
                }
            }
        }
        else if (kind == NDS_STAGE_GX_VIEW)
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
        else if (kind == NDS_STAGE_GX_NOZ)
        {
            if (projection_fresh == FALSE)
            {
                memcpy(dst, projection, sizeof(composed));
            }
            ndsStageGxFastDepthColumn(dst, projection,
                                      ndsRendererHardwareNextProjectedDepth());
        }
        else if ((kind == NDS_STAGE_GX_COMPOSED_NOZ) ||
                 (kind == NDS_STAGE_GX_COMPOSED) ||
                 (kind == NDS_STAGE_GX_CORNER_NOZ))
        {
            const u32 binding = (kind == NDS_STAGE_GX_CORNER_NOZ) ?
                sNdsNativeStageVertices[q->index].matrix_binding : q->index;
            const u32 shift = (kind == NDS_STAGE_GX_CORNER_NOZ) ?
                (q->aux & 7u) : q->aux;

            if ((composed_shift != (s32)shift) || (composed_binding != binding))
            {
                if (ndsStageGxComposedFor(binding, shift, &composed) == FALSE)
                {
                    NDS_STAGE_GX_FAST_WHY(5u);
                    goto declined;
                }
                composed_shift = (s32)shift;
                composed_binding = binding;
            }
            memcpy(dst, &composed, sizeof(composed));
            if (kind == NDS_STAGE_GX_CORNER_NOZ)
            {
                const u32 triangle = (u32)q->aux >> 3;

                if (corner_triangle != triangle)
                {
                    corner_triangle = triangle;
                    corner_depth = ndsRendererHardwareNextProjectedDepth();
                }
                ndsStageGxFastDepthColumn(dst, &composed, corner_depth);
            }
            else if (kind == NDS_STAGE_GX_COMPOSED_NOZ)
            {
                ndsStageGxFastDepthColumn(
                    dst, &composed, ndsRendererHardwareNextProjectedDepth());
            }
        }
        else if (kind >= NDS_STAGE_GX_NOZ_VIEW_RUN)
        {
            /* The translated painter: one matrix and the run's depths. */
            const NDSRendererMatrix20p12 *m;

            if (kind == NDS_STAGE_GX_NOZ_VIEW_RUN)
            {
                m = ndsStageGxViewProjection();
            }
            else
            {
                const u32 shift = q->aux & 7u;

                if ((composed_shift != (s32)shift) ||
                    (composed_binding != q->index))
                {
                    if (ndsStageGxComposedFor(q->index, shift, &composed) ==
                        FALSE)
                    {
                        NDS_STAGE_GX_FAST_WHY(6u);
                        goto declined;
                    }
                    composed_shift = (s32)shift;
                    composed_binding = q->index;
                }
                m = &composed;
            }
            memcpy(dst, m, sizeof(*m));
            ndsStageGxFastDepthColumn(dst, m,
                                      ndsRendererHardwareNextProjectedDepth());
            sNdsRendererHardwareProjectedDepth -=
                NDS_RENDERER_HW_PROJECTED_DEPTH_STEP *
                (s32)(ndsStageGxNoZRunTriangles(q) - 1u);
        }
        else if (kind == NDS_STAGE_GX_UV)
        {
            const NDSNativeStagePreparedDense *v =
                &sNdsNativeStagePreparedDense[q->index];

            *dst = (u32)(u16)v->s | ((u32)(u16)v->t << 16);
        }
        else if (kind == NDS_STAGE_GX_COLOR)
        {
            *dst = sNdsNativeStagePreparedDense[q->index].packed_color;
        }
        else if (kind == NDS_STAGE_GX_PROJECTION)
        {
            if (projection_fresh == FALSE)
            {
                ndsStageGxProjection(dst);
            }
        }
        else if (kind == NDS_STAGE_GX_CORNER_SOURCE)
        {
            ndsStageGxCornerSource(dst, q);
        }
    }

    /* Every word is written: the memos now vouch for this pass's inputs. */
    if ((material_fresh == FALSE) || (projection_fresh == FALSE))
    {
        for (i = 0u; i < segment->run_count; i++)
        {
            const u32 run_index = (u32)segment->first_run + i;
            NDSStageGxRunMemo *m = &sNdsStageGxRunMemo[run_index];
            const NDSNativeStagePreparedRun *prepared =
                &sNdsNativeStageWs.runs[run_index];

            m->projection_serial = projection_serial;
            m->poly_fmt = prepared->poly_fmt;
            m->texture_name = prepared->texture_name;
            m->texture_params = prepared->texture_params;
            m->textured = prepared->textured;
            m->material_gen = texgen + 1u;
        }
        s->fresh_projection = projection_serial + 1u;
        s->fresh_prepared = prepared_serial + 1u;
        s->fresh_texgen = texgen + 1u;
    }

    /* The first span's start (ndsStageGxAppend with nothing pending): an
     * open Task 36 segment's FIFO pop follows any DMA still running. */
    if (sNdsNativeStageOwnerExecution.task36_segment_active != FALSE)
    {
        NDS_FIGHTER_PACKET_DMA_WAIT();
    }
    ndsRendererNativeStageTask36EndSegment();
    ndsRendererHardwareEndBatch();
    /* The per-run texture accounting summed once after the walk: the
     * stats keep the last textured run's format, as the per-run writes
     * left them, and nothing between reads them. */
    const NDSNativeStagePreparedRun *prepared =
        &sNdsNativeStageWs.runs[segment->first_run];
    const NDSNativeStagePreparedRun *const prepared_end =
        prepared + segment->run_count;
    const NDSNativeStagePreparedRun *last_textured = NULL;
    const u32 frame_stamp = sNdsRendererHardwareFrameSerial + 1u;
    u32 textured_runs = 0u;

    for (; prepared < prepared_end; prepared++)
    {
        const u32 alpha = (prepared->alpha_test != 0u) ?
            (0x100u | prepared->alpha_ref) : 0u;

        if ((s->skip_mask != 0u) &&
            (((s->skip_mask >> sNdsNativeStageRuns[
                   (u32)(prepared - sNdsNativeStageWs.runs)]
                   .binding_index) & 1u) != 0u))
        {
            continue;
        }
        if (prepared->textured != 0u)
        {
            NDSRendererHardwareTextureCacheEntry *entry =
                prepared->texture_entry;

            entry->last_used_frame = frame_stamp;
            entry->params = prepared->texture_params;
            if (entry->pinned != 0u)
            {
                ndsRendererHardwareRecordBattleStaticTextureHit(entry);
            }
            textured_runs++;
            last_textured = prepared;
        }
        if (alpha != span_alpha)
        {
            ndsStageGxSetSpanState(alpha, prepared);
            span_alpha = alpha;
        }
    }
    if (last_textured != NULL)
    {
        stats->hardware_texture_ready_count += textured_runs;
        stats->hardware_texture_bind_count += textured_runs;
        stats->hardware_texture_format = last_textured->texture_format;
        stats->hardware_texture_width = last_textured->texture_width;
        stats->hardware_texture_height = last_textured->texture_height;
        ndsRendererProfileRecordTextureBinds(textured_runs);
    }
    if (s->skip_mask == 0u)
    {
        sNdsStageGxFirstWord = s->first_word;
        sNdsStageGxWordCount = (u32)s->word_end - s->first_word;
        sNdsStageGxAlpha = span_alpha;
        ndsStageGxFlush();
    }
    else
    {
        for (i = 0u; i < s->span_count; i++)
        {
            sNdsStageGxFirstWord = s->span_word[i][0];
            sNdsStageGxWordCount =
                (u32)s->span_word[i][1] - s->span_word[i][0];
            sNdsStageGxAlpha = span_alpha;
            ndsStageGxFlush();
        }
    }

    /* ndsRendererNativeStageAccountRun over the segment's runs. */
    if (s->class_nz_count != 0xffu)
    {
        for (i = 0u; i < s->class_nz_count; i++)
        {
            const u32 klass = s->class_nz[i];

            sNdsRendererHardwareSubmitClassCounts[klass] +=
                s->class_triangles[klass];
        }
    }
    else
    for (i = 0u; i < NDS_RENDERER_HW_SUBMIT_CLASS_COUNT; i++)
    {
        sNdsRendererHardwareSubmitClassCounts[i] += s->class_triangles[i];
    }
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count += s->reuse;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count += s->reuse;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += s->triangles;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += s->triangles * 3u;
    {
        const u32 raw = s->raw_triangles;

        sNdsRendererRuntimeFrameSummary.raw_current_candidate_count += raw;
        stats->hardware_zbuffer_triangle_count += raw;
        sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count +=
            s->triangles - raw;
        stats->hardware_projected_depth_triangle_count += s->triangles - raw;
    }
    stats->hardware_triangle_count += s->triangles;
    stats->hardware_vertex_count += s->triangles * 3u;
    sNdsRendererHardwareSubmitted = TRUE;
    stats->triangle_count += s->triangles;
    *segment_triangles = s->triangles;
    NDS_DIAG(gNdsP2StageProgDraws += segment->run_count);
    NDS_DIAG(gNdsP2StageProgWords += (u32)s->word_end - s->first_word);
    NDS_DIAG(gNdsStageGxFastCommits++);
    return TRUE;

declined:
    sNdsRendererHardwareProjectedDepth = depth_saved;
    sNdsRendererHardwareProjectedBackground = background_saved;
    NDS_DIAG(gNdsStageGxFastDeclines++);
    return FALSE;
}
#endif
