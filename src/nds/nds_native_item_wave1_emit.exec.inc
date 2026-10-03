/* Shared fixed-geometry submit primitive for the five common-item owners in
 * this batch. TEXTUAL INCLUDE ONLY. State/material ownership remains in the
 * per-item executors; this helper only converts an already-baked vertex/index
 * packet into DS GX triangles under the supplied traversal state. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

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
    u32 material_color;
    u32 use_material;
    u32 use_texture;
    u32 use_vertex;
    u32 poly_alpha;
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
        return FALSE;
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
        return FALSE;
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
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture,
            (use_texture != FALSE) ? sNdsRendererHardwareBoundTextureName : 0u,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
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
        NDS_LAB_BAKED_ADD(10, lab_mark);
        NDS_LAB_BAKED_MARK(lab_mark);
    }
    ndsRendererHardwareEndBatch();
    NDS_LAB_BAKED_ADD(11, lab_mark);
    return TRUE;
}

#endif
