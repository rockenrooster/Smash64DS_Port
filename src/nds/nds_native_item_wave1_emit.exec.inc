/* Shared fixed-geometry submit primitive for the five common-item owners in
 * this batch. TEXTUAL INCLUDE ONLY. State/material ownership remains in the
 * per-item executors; this helper only converts an already-baked vertex/index
 * packet into DS GX triangles under the supplied traversal state. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

#include <nds/nds_item_replay.h>

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY: the foreign-platform submit split (Board the Platforms).
 * [0] fast-lane offscreen test, [1] stats reset + material snapshots,
 * [2] baked submit total, [3] traversal init, [4] state/material/env ops,
 * [5] emit total, [6] emit entry (dma wait, end batch, setup), [7] texture
 * bind, [8] matrix load, [9] batch begin, [10] corners, [11] batch end,
 * [12] visible submits, [13] emitted groups, [14] corners, [15] culled. */
__attribute__((used)) volatile u32 gNdsLabBakedAcc[16];
#define NDS_LAB_BAKED_MARK(v) ((v) = cpuGetTiming())
#define NDS_LAB_BAKED_ADD(i, v) (gNdsLabBakedAcc[(i)] += cpuGetTiming() - (v))
#define NDS_LAB_BAKED_INC(i, n) (gNdsLabBakedAcc[(i)] += (n))
#else
#define NDS_LAB_BAKED_MARK(v) ((void)0)
#define NDS_LAB_BAKED_ADD(i, v) ((void)0)
#define NDS_LAB_BAKED_INC(i, n) ((void)0)
#endif

/* Texture memos for the baked runs (ndsRendererHardwareBindTextureOwnerMemo).
 * One vertex table is drawn under several material states -- Board the
 * Platforms draws one platform mesh in several colours and textures -- so a
 * memo per table missed on every alternation (and two tables could share a
 * direct-mapped slot): ~12 full resolves a frame on that stage, more when the
 * platforms' material animation steps. Each (table, texture, prim) picks a
 * two-way set; a hit or a refill makes the other way the next victim. A
 * collision only refills. */
#define NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SETS 32u
static NDSRendererOwnerTextureMemo
    sNdsNativeItemWave1TexMemo[NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SETS * 2u];
static u8 sNdsNativeItemWave1TexMemoVictim[NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SETS];

/* P2-6 (2026-10-03): a baked root names its group's set directly (the
 * executor sets this around each EMIT; NONE elsewhere). Board the Platforms
 * binds ~20 platform groups a frame, consecutive in the group table, and the
 * hashed set put three of them in one two-way set often enough to refill
 * ~5 times a frame -- whether it did depended on the heap addresses of the
 * texture images, i.e. on the build's layout. Consecutive groups now take
 * consecutive sets. */
#define NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SET_NONE 0xffffffffu
static u32 sNdsNativeItemWave1TexMemoSet = NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SET_NONE;

/* P2-6 (2026-10-03): a baked root's groups share its matrices. The executor
 * names one matrix generation for the whole root (0 = none: each emit takes
 * its own, as before), so the loader elides the reload for the root's later
 * groups unless something else loaded a matrix in between. */
static u32 sNdsNativeItemWave1MatrixGeneration;

static NDSRendererOwnerTextureMemo *
ndsNativeItemWave1TexMemoFor(u32 id, const NDSRendererStats *stats)
{
    const u32 set =
        (sNdsNativeItemWave1TexMemoSet != NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SET_NONE) ?
            (sNdsNativeItemWave1TexMemoSet &
             (NDS_NATIVE_ITEM_WAVE1_TEXMEMO_SETS - 1u)) :
            (((id ^ (stats->texture_image * 0x9E3779B1u) ^
               ((stats->prim_color & 0xffffff00u) * 0x85EBCA6Bu)) *
              2654435761u) >> 27);
    NDSRendererOwnerTextureMemo *ways = &sNdsNativeItemWave1TexMemo[set * 2u];
    u32 way;

    if (ndsRendererOwnerTexMemoKeyMatches(&ways[0], id, stats) != FALSE)
    {
        way = 0u;
    }
    else if (ndsRendererOwnerTexMemoKeyMatches(&ways[1], id, stats) != FALSE)
    {
        way = 1u;
    }
    else
    {
        way = sNdsNativeItemWave1TexMemoVictim[set];
    }
    sNdsNativeItemWave1TexMemoVictim[set] = (u8)(way ^ 1u);
    return &ways[way];
}

/* P2-2p8 (2026-10-04): the item draw replay's recording side
 * (include/nds/nds_item_replay.h). While the adapter holds a sink, every emit
 * also leaves what its GX output depended on besides the matrices: the
 * texture the bind landed on, the batch's polygon format and alpha-test
 * words, and the per-vertex words. Anything this replay cannot reproduce --
 * a shared matrix generation, fog, a bind outside the texture cache -- marks
 * the sink failed and the draw is simply not cached. */
static NDSItemReplaySink *sNdsItemReplaySink;
volatile u32 gNdsItemReplayRecordFails;

void ndsNativeItemReplaySetSink(NDSItemReplaySink *sink)
{
    sNdsItemReplaySink = sink;
}

static sb32 ndsNativeItemWave1Emit(
    const s16 *verts, const u32 *colors, const u16 *indices,
    u32 vertex_count, u32 triangle_count, u32 corner_count,
    const NDSRendererConfig *config, NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    volatile u32 *submit_step, volatile u32 *alpha_witness)
{
    /* Per vertex, not per corner: the COLOR, TEX_COORD and VTX_16 words a
     * corner sends depend only on its vertex and this group's state, and the
     * corners name each vertex about twice. */
    u32 color_words[24];
    u32 tex_words[24];
    u32 xy_words[24];
    s32 z_words[24];
    NDSRendererInputVertex vertex0;
    const NDSRendererTileState *tile;
    NDSItemReplaySink *const sink = sNdsItemReplaySink;
    NDSItemReplayEmit *rec = NULL;
    u32 material_color;
    u32 use_material;
    u32 use_texture;
    u32 use_vertex;
    u32 poly_alpha;
    u32 poly_fmt;
    u32 generation;
    s32 texture_offset;
    u32 corner;
    u32 i;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_mark = cpuGetTiming();
#endif

    if ((verts == NULL) || (colors == NULL) || (indices == NULL) ||
        (config == NULL) || (stats == NULL) || (state == NULL) ||
        (submit_step == NULL) || (alpha_witness == NULL) ||
        (vertex_count == 0u) || (vertex_count > 24u))
    {
        if (submit_step != NULL)
        {
            *submit_step = 4u;
        }
        if (sink != NULL)
        {
            sink->failed = 1u;
        }
        return FALSE;
    }
    if (sink != NULL)
    {
        if ((sink->failed == 0u) &&
            (sink->emit_count < NDS_ITEM_REPLAY_EMITS) &&
            ((sink->word_count + (vertex_count * 4u)) <=
             NDS_ITEM_REPLAY_POOL_WORDS) &&
            (corner_count <= 0xffffu) && (triangle_count <= 0xffffu) &&
            (sNdsNativeItemWave1MatrixGeneration == 0u))
        {
            rec = &sink->emits[sink->emit_count];
        }
        else
        {
            sink->failed = 1u;
        }
    }

    vertex0 = (NDSRendererInputVertex){
        verts[0], verts[1], verts[2], verts[3], verts[4],
        colors[0] >> 24, colors[0] >> 16, colors[0] >> 8, colors[0]
    };

    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    /* Some baked item runs are source-untextured.  Beam Sword joint 1 is the
     * first natural witness: its source DL explicitly issues G_TEXTURE off and
     * combines SHADE only.  Requiring a texture bind here rejects that valid
     * run even though there is deliberately no image to bind.  Match the
     * generic native prepare path: bind only when the decoded source state
     * actually uses a texture, and submit an untextured DS batch otherwise. */
    use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ? TRUE : FALSE;
    NDS_LAB_BAKED_ADD(6, lab_mark);
    NDS_LAB_BAKED_MARK(lab_mark);
    if ((use_texture != FALSE) &&
        (ndsRendererHardwareBindTextureOwnerMemo(
             ndsNativeItemWave1TexMemoFor((u32)(uintptr_t)verts, stats),
             (u32)(uintptr_t)verts, stats, config, state) == FALSE))
    {
        *submit_step = 3u;
        if (sink != NULL)
        {
            sink->failed = 1u;
        }
        return FALSE;
    }
    if (rec != NULL)
    {
        rec->use_texture = (u8)use_texture;
        if (use_texture != FALSE)
        {
            const NDSRendererHardwareTextureCacheEntry *entry =
                sNdsRendererHardwareActiveTextureEntry;
            const u32 slot = (entry != NULL) ?
                (u32)(entry - sNdsRendererHardwareTextureCache) : 0xffffffffu;

            if ((entry == NULL) || (slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
                (entry->ready == FALSE) || (entry->name == 0) ||
                ((u32)entry->name != sNdsRendererHardwareBoundTextureName))
            {
                sink->failed = 1u;
                rec = NULL;
            }
            else
            {
                rec->tex_slot = (u16)slot;
                rec->tex_name = (u32)entry->name;
                rec->tex_generation = entry->key_generation;
                rec->tex_format = stats->hardware_texture_format;
                rec->tex_width = stats->hardware_texture_width;
                rec->tex_height = stats->hardware_texture_height;
            }
        }
    }

    NDS_LAB_BAKED_ADD(7, lab_mark);
    NDS_LAB_BAKED_MARK(lab_mark);
    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertex0);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    *alpha_witness = poly_alpha;

    if (poly_alpha != 0u)
    {
        NDS_LAB_BAKED_ADD(6, lab_mark);
        NDS_LAB_BAKED_MARK(lab_mark);
        generation = (sNdsNativeItemWave1MatrixGeneration != 0u) ?
            sNdsNativeItemWave1MatrixGeneration :
            ndsRendererNextMatrixGeneration();
        ndsRendererLoadHardwareSplitMatrices(
            config->initial_projection, config->initial_modelview, generation);
        NDS_LAB_BAKED_ADD(8, lab_mark);
        NDS_LAB_BAKED_MARK(lab_mark);
        poly_fmt = ndsRendererHardwarePolyFmt(stats, poly_alpha);
        if (rec != NULL)
        {
            if (ndsRendererHardwareFogStateKey(stats) != 0u)
            {
                sink->failed = 1u;
                rec = NULL;
            }
            else
            {
                rec->poly_fmt = poly_fmt;
                rec->othermode_l = stats->othermode_l;
                rec->blend_color = stats->blend_color;
            }
        }
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture,
            (use_texture != FALSE) ? sNdsRendererHardwareBoundTextureName : 0u,
            poly_fmt, sNdsRendererHardwareMatrixMode, generation);
        NDS_LAB_BAKED_ADD(9, lab_mark);
        NDS_LAB_BAKED_MARK(lab_mark);
        for (i = 0u; i < vertex_count; i++)
        {
            const s16 *v = &verts[i * 5u];
            const u32 rgba = colors[i];
            const NDSRendererInputVertex vtx = {
                v[0], v[1], v[2], v[3], v[4],
                rgba >> 24, rgba >> 16, rgba >> 8, rgba
            };
            const v16 x = ndsRendererHardwareVertexCoord(v[0], TRUE);
            const v16 y = ndsRendererHardwareVertexCoord(v[1], TRUE);

            /* The words glColor, glTexCoord2t16 and glVertex3v16 write. */
            color_words[i] = (u32)ndsRendererHardwarePackedVertexColor(
                stats, &vtx, material_color, use_material, use_vertex,
                0u, FALSE, 0u);
            if (use_texture != FALSE)
            {
                tex_words[i] = TEXTURE_PACK(
                    ndsRendererHardwareTexCoord(v[3], stats->texture_scale_s,
                        tile->uls, texture_offset),
                    ndsRendererHardwareTexCoord(v[4], stats->texture_scale_t,
                        tile->ult, texture_offset));
            }
            xy_words[i] = (u32)((y << 16) | (x & 0xFFFF));
            z_words[i] = ndsRendererHardwareVertexCoord(v[2], TRUE);
        }
        for (corner = 0u; corner < corner_count; corner++)
        {
            const u32 k = indices[corner];

            GFX_COLOR = color_words[k];
            if (use_texture != FALSE)
            {
                GFX_TEX_COORD = tex_words[k];
            }
            GFX_VERTEX16 = xy_words[k];
            GFX_VERTEX16 = (u32)z_words[k];
        }
        for (i = 0u; i < triangle_count; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count += triangle_count;
        stats->hardware_zbuffer_triangle_count += triangle_count;
        stats->hardware_vertex_count += corner_count;
        sNdsRendererHardwareSubmitted = TRUE;
        if (rec != NULL)
        {
            u32 *words = &sink->pool[sink->word_count];

            for (i = 0u; i < vertex_count; i++)
            {
                words[(i * 4u) + 0u] = color_words[i];
                words[(i * 4u) + 1u] =
                    (use_texture != FALSE) ? tex_words[i] : 0u;
                words[(i * 4u) + 2u] = xy_words[i];
                words[(i * 4u) + 3u] = (u32)z_words[i];
            }
        }
        NDS_LAB_BAKED_ADD(10, lab_mark);
        NDS_LAB_BAKED_MARK(lab_mark);
    }
    ndsRendererHardwareEndBatch();
    if (rec != NULL)
    {
        rec->indices = indices;
        rec->vertex_count = (u8)vertex_count;
        rec->corner_count = (u16)corner_count;
        rec->triangle_count = (u16)triangle_count;
        rec->drawn = (poly_alpha != 0u) ? 1u : 0u;
        rec->word_first = (u16)sink->word_count;
        if (poly_alpha != 0u)
        {
            sink->word_count += vertex_count * 4u;
        }
        sink->emit_count++;
    }
    else if ((sink != NULL) && (sink->failed != 0u))
    {
        gNdsItemReplayRecordFails++;
    }
    NDS_LAB_BAKED_ADD(11, lab_mark);
    return TRUE;
}

/* TRUE when every textured emit's cache entry still holds the texture it
 * recorded (ready, same GL name, same key generation): an evicted or
 * recycled slot sends the draw back to its owners before any GX write. */
sb32 ndsNativeItemReplayEmitsResident(const NDSItemReplayEmit *emits,
                                      u32 count)
{
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const NDSItemReplayEmit *e = &emits[i];
        const NDSRendererHardwareTextureCacheEntry *entry;

        if (e->use_texture == 0u)
        {
            continue;
        }
        if (e->tex_slot >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
        {
            return FALSE;
        }
        entry = &sNdsRendererHardwareTextureCache[e->tex_slot];
        if ((entry->ready == FALSE) || ((u32)entry->name != e->tex_name) ||
            (entry->key_generation != e->tex_generation))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* One root's recorded emits under this draw's matrices, in the emit's own
 * order: the DMA wait and batch close, the owner-memo hit tail of the bind
 * (the LRU stamp, the name and parameter binds, the active entry, the pinned
 * hit), the split matrix load, the batch open with the recorded format and
 * alpha-test words, and the corners. Returns the triangles drawn. The batch
 * open reads only the alpha-test and fog words of `scratch`; fog is set off,
 * as every recording had it (a recording with fog fails). */
u32 ndsNativeItemReplayEmits(const NDSItemReplayEmit *emits, u32 count,
                             const u32 *pool, const void *projection,
                             const void *modelview,
                             struct NDSRendererStats *scratch)
{
    u32 triangles = 0u;
    u32 i;

    scratch->fog_status = 0u;
    for (i = 0u; i < count; i++)
    {
        const NDSItemReplayEmit *e = &emits[i];

        NDS_FIGHTER_PACKET_DMA_WAIT();
        ndsRendererHardwareEndBatch();
        if (e->use_texture != 0u)
        {
            NDSRendererHardwareTextureCacheEntry *entry =
                &sNdsRendererHardwareTextureCache[e->tex_slot];

            entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
            if (entry->pinned != 0u)
            {
                ndsRendererHardwareBindTextureName(scratch,
                                                   (u32)entry->name);
                ndsRendererHardwareApplyTextureParams(entry->params);
                sNdsRendererHardwareActiveTextureEntry = entry;
                ndsRendererHardwareRecordBattleStaticTextureHit(entry);
            }
            else if (sNdsRendererHardwareActiveTextureEntry != entry)
            {
                ndsRendererHardwareBindTextureName(scratch,
                                                   (u32)entry->name);
                ndsRendererHardwareApplyTextureParams(entry->params);
                sNdsRendererHardwareActiveTextureEntry = entry;
            }
        }
        if (e->drawn != 0u)
        {
            const u32 generation = ndsRendererNextMatrixGeneration();
            const u32 *words = &pool[e->word_first];
            const u32 textured = e->use_texture;
            u32 corner;
            u32 t;

            ndsRendererLoadHardwareSplitMatrices(
                (const NDSRendererMatrix20p12 *)projection,
                (const NDSRendererMatrix20p12 *)modelview, generation);
            scratch->othermode_l = e->othermode_l;
            scratch->blend_color = e->blend_color;
            ndsRendererHardwareBeginTriangleBatch(
                scratch, textured,
                (textured != 0u) ? sNdsRendererHardwareBoundTextureName : 0u,
                e->poly_fmt, sNdsRendererHardwareMatrixMode, generation);
            for (corner = 0u; corner < e->corner_count; corner++)
            {
                const u32 *v = &words[(u32)e->indices[corner] * 4u];

                GFX_COLOR = v[0];
                if (textured != 0u)
                {
                    GFX_TEX_COORD = v[1];
                }
                GFX_VERTEX16 = v[2];
                GFX_VERTEX16 = v[3];
            }
            for (t = 0u; t < e->triangle_count; t++)
            {
                ndsRendererProfileRecordHardwareTriangle();
            }
            sNdsRendererHardwareSubmitted = TRUE;
            triangles += e->triangle_count;
        }
        ndsRendererHardwareEndBatch();
    }
    return triangles;
}

#endif
