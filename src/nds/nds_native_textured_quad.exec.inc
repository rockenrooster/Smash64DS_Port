/* Fixed four-vertex/six-index GX kernel shared by the baked effect owners.
 * Keep one out-of-line copy: duplicated texture/color submission exhausted
 * the all-content arena when Sing was added. No display-list execution.
 * Return polygon alpha (0 means invisible), or -1 for a failed texture bind. */
#if NDS_RENDERER_HW_TRIANGLES
/* GRADED ALPHA FOR IA8 QUADS. The generic cache uploads every entry as
 * RGB555 + ONE alpha bit. An IA8 glow (Pikachu's Thunder, Ness's PK Thunder) is
 * a four-texel bright core inside a twelve-texel alpha ramp, so the threshold
 * kept only the core: a bolt 300 units wide drew as a sub-pixel hairline and
 * Thunder was invisible with every draw counted. A3I5 is one byte a texel --
 * five index bits, three alpha bits -- so the 4-bit intensity lands exactly in
 * a 32-step palette and the ramp keeps eight coverage levels. Every owner here
 * uses the one combiner (PRIM - ENV) * TEXEL0 + ENV with TEXEL0 alpha, which
 * modulate cannot express: the palette IS that ramp, ENV at index 0 to PRIM at
 * 31, and the polygon colour is white. A grey palette drew the yellow fringe of
 * Thunder as grey. The ramp follows live material colours per entry. Dedicated
 * names, following the rebirth beam: no per-entry cache format, and half the
 * VRAM of the RGBA copy they replace. Names die with glResetTextures, so an
 * entry is only trusted inside the scene-texture generation that built it.
 *
 * CI4 OVER A 1-BIT TLUT takes the same road for a different reason. The source
 * draws those quads G_RM_AA_TEX_EDGE with bilinear filtering, so the one-bit
 * alpha arrives on screen as a one-texel coverage ramp; nearest sampling of the
 * same bit is a staircase. The sixteen colours fit the 32-entry palette as they
 * are, an opaque texel beside a clear one drops to 5/7 and the clear texel
 * beside it takes that neighbour's colour at 2/7 -- the ramp's integral, split
 * across the two texel cells the DS can address.
 *
 * EVERY TAP GOES THROUGH THE LANE. Fighter and effect files are O2R word
 * swapped in memory: logical byte i lives at i ^ 3. A fill that indexes the
 * image raw mirrors every four-texel group -- the defect family that has already
 * cost this port five sessions -- so both fills and the TLUT reader take the
 * lane from the config that names the file's layout. */
#define NDS_GRADED_QUAD_TEXTURES 8u
typedef struct NDSGradedQuadTexture
{
    const u8 *image;
    u32 name;
    u32 generation;
    u32 prim; /* RGB the palette was built for */
    u32 env;
    u32 frame; /* sNdsRendererHardwareFrameSerial + 1 at the last bind */
} NDSGradedQuadTexture;
static NDSGradedQuadTexture sNdsGradedQuadTextures[NDS_GRADED_QUAD_TEXTURES];
static u16 sNdsGradedQuadPalette[32];
volatile u32 gNdsGradedQuadTextureBuilds;
volatile u32 gNdsGradedQuadTextureFails;
volatile u32 gNdsGradedQuadPaletteUpdates;
/* Non-zero means the table filled and a slot had to be taken back. Reading it
 * beside gNdsGradedQuadTextureFails separates "recycled, drew" from "gave up". */
volatile u32 gNdsGradedQuadTextureRecycles;

static void ndsGradedQuadRamp(u32 prim, u32 env)
{
    u32 i;

    for (i = 0u; i < 32u; i++)
    {
        u32 shift;
        u32 color = 0u;

        /* RGBA8888 r,g,b at 24,16,8 -> RGB15 r,g,b at 0,5,10. */
        for (shift = 24u; shift >= 8u; shift -= 8u)
        {
            u32 bright = (prim >> shift) & 0xffu;
            u32 dark = (env >> shift) & 0xffu;
            u32 value = ((dark * (31u - i)) + (bright * i) + 15u) / 31u;

            color |= (value >> 3) << (((24u - shift) >> 3) * 5u);
        }
        sNdsGradedQuadPalette[i] = (u16)color;
    }
}

typedef struct NDSGradedQuadFill
{
    const u8 *image;
    const u8 *tlut; /* NULL: IA8. Else CI4 over a 16-entry RGBA5551 TLUT. */
    u32 width;
    u32 height;
    u32 lane; /* 3 for an O2R word-swapped file, else 0 */
} NDSGradedQuadFill;

static s32 ndsGradedQuadIa8Fill(u8 *pixels, u32 bytes, void *user_data)
{
    const NDSGradedQuadFill *fill = user_data;
    u32 i;

    for (i = 0u; i < bytes; i++)
    {
        u32 texel = fill->image[i ^ fill->lane];
        u32 intensity = texel >> 4;
        u32 alpha = texel & 0x0fu;

        /* I4 -> I5 by bit replication (0..15 -> 0..31), A4 -> A3. */
        pixels[i] = (u8)(((alpha >> 1) << 5) | (intensity << 1) | (intensity >> 3));
    }
    return TRUE;
}

static u32 ndsGradedQuadTlutEntry(const NDSGradedQuadFill *fill, u32 index)
{
    /* Big-endian halfword `index`, read through the lane one byte at a time. */
    return ((u32)fill->tlut[(index << 1) ^ fill->lane] << 8) |
           fill->tlut[((index << 1) + 1u) ^ fill->lane];
}

/* 0..15: opaque palette index. 16: clear. 17: outside the image, which is not
 * an edge -- the tile clamps or wraps there. */
static u32 ndsGradedQuadCi4Index(const NDSGradedQuadFill *fill, s32 x, s32 y)
{
    u32 texel;
    u32 index;

    if ((x < 0) || (y < 0) || ((u32)x >= fill->width) || ((u32)y >= fill->height))
    {
        return 17u;
    }
    texel = ((u32)y * fill->width) + (u32)x;
    index = (fill->image[(texel >> 1) ^ fill->lane] >> (((texel & 1u) != 0u) ? 0u : 4u)) & 0x0fu;
    return ((ndsGradedQuadTlutEntry(fill, index) & 1u) != 0u) ? index : 16u;
}

static s32 ndsGradedQuadCi4Fill(u8 *pixels, u32 bytes, void *user_data)
{
    const NDSGradedQuadFill *fill = user_data;
    s32 x;
    s32 y;

    (void)bytes;
    for (y = 0; y < (s32)fill->height; y++)
    {
        for (x = 0; x < (s32)fill->width; x++)
        {
            u32 around[4];
            u32 self = ndsGradedQuadCi4Index(fill, x, y);
            u32 value = 0u;
            u32 k;

            around[0] = ndsGradedQuadCi4Index(fill, x - 1, y);
            around[1] = ndsGradedQuadCi4Index(fill, x + 1, y);
            around[2] = ndsGradedQuadCi4Index(fill, x, y - 1);
            around[3] = ndsGradedQuadCi4Index(fill, x, y + 1);
            if (self < 16u)
            {
                value = (7u << 5) | self;
                for (k = 0u; k < 4u; k++)
                {
                    if (around[k] == 16u) value = (5u << 5) | self;
                }
            }
            else
            {
                for (k = 0u; (k < 4u) && (value == 0u); k++)
                {
                    if (around[k] < 16u) value = (2u << 5) | around[k];
                }
            }
            *pixels++ = (u8)value;
        }
    }
    return TRUE;
}

static void ndsGradedQuadTlutPalette(const NDSGradedQuadFill *fill)
{
    u32 i;

    for (i = 0u; i < 32u; i++)
    {
        u32 entry = (i < 16u) ? ndsGradedQuadTlutEntry(fill, i) : 0u;

        /* RGBA5551 r,g,b at 11,6,1 -> RGB15 r,g,b at 0,5,10. */
        sNdsGradedQuadPalette[i] = (u16)(((entry >> 11) & 31u) |
            (((entry >> 6) & 31u) << 5) | (((entry >> 1) & 31u) << 10));
    }
}

static s32 ndsRendererNativeBindGraded(NDSGradedQuadFill *fill, u32 prim, u32 env)
{
    const u8 *image = fill->image;
    u32 width = fill->width;
    u32 height = fill->height;
    u32 generation = gNdsRendererSceneTextureVramResetCount + 1u;
    u32 i;
    u32 slot = NDS_GRADED_QUAD_TEXTURES;

    if ((image == NULL) || (width == 0u) || (((width * height) & 3u) != 0u) ||
        (width * height > sizeof(sNdsRendererHardwareTextureScratch)))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_GRADED_QUAD_TEXTURES; i++)
    {
        NDSGradedQuadTexture *entry = &sNdsGradedQuadTextures[i];

        if (entry->generation != generation)
        {
            entry->image = NULL; /* the name died with the scene reset */
            entry->name = 0u;
        }
        if ((entry->image == image) && (entry->name != 0u))
        {
            ndsRendererHardwareEndBatch();
            ndsRendererHardwareBindTextureState((int)entry->name);
            sNdsRendererHardwareBoundTextureName = entry->name;
            sNdsRendererHardwareActiveTextureEntry = NULL;
            entry->frame = sNdsRendererHardwareFrameSerial + 1u;
            if ((entry->prim != prim) || (entry->env != env))
            {
                ndsGradedQuadRamp(prim, env);
                glColorTableEXT(GL_TEXTURE_2D, 0, 32, 0, 0, sNdsGradedQuadPalette);
                entry->prim = prim;
                entry->env = env;
                gNdsGradedQuadPaletteUpdates++;
            }
            return TRUE;
        }
        if ((slot == NDS_GRADED_QUAD_TEXTURES) && (entry->name == 0u))
        {
            slot = i;
        }
    }
    if (slot == NDS_GRADED_QUAD_TEXTURES)
    {
        /* EIGHT SLOTS, TWELVE OWNERS, AND NOTHING EVER GAVE ONE BACK.
         *
         * Until now a slot was freed only when `generation` changed, i.e. at a
         * whole-scene texture reset. But several owners pass a LIVE
         * `material->current_image` -- the Thunder Jolt family and Ness's PK
         * tail among them -- so their image pointer changes WITHIN a scene.
         * The old entry then matched nothing (its `image` is stale) and could
         * never be chosen as a free slot (its `name` is non-zero), so its VRAM
         * leaked and the table filled permanently. From that point every
         * graded request in the match returned FALSE and the owner fell to the
         * 1-bit-alpha path the helper exists to avoid, or was not drawn at all.
         *
         * Take the least-recently-bound resident slot instead -- never one
         * bound this frame, the same rule ndsRendererHardwareEvictTexture
         * applies to its own entries. This branch previously always failed, so
         * nothing that works today can be made worse: the only paths that
         * change behaviour are ones that were already returning FALSE. The old
         * name is NOT zeroed below, so PrepareIFCommonA3I5Atlas releases it and
         * the VRAM actually comes back. */
        u32 oldest_frame = 0u;
        u32 this_frame = sNdsRendererHardwareFrameSerial + 1u;

        for (i = 0u; i < NDS_GRADED_QUAD_TEXTURES; i++)
        {
            NDSGradedQuadTexture *entry = &sNdsGradedQuadTextures[i];
            u32 age;

            if ((entry->name == 0u) || (entry->frame == this_frame))
            {
                continue;
            }
            age = this_frame - entry->frame;
            if (age >= oldest_frame)
            {
                oldest_frame = age;
                slot = i;
            }
        }
        if (slot == NDS_GRADED_QUAD_TEXTURES)
        {
            gNdsGradedQuadTextureFails++;
            return FALSE;
        }
        sNdsGradedQuadTextures[slot].image = NULL;
        gNdsGradedQuadTextureRecycles++;
    }
    if (fill->tlut != NULL) ndsGradedQuadTlutPalette(fill);
    else ndsGradedQuadRamp(prim, env);
    if (ndsRendererHardwarePrepareIFCommonA3I5Atlas(
            width, height, sNdsGradedQuadPalette,
            (fill->tlut != NULL) ? ndsGradedQuadCi4Fill : ndsGradedQuadIa8Fill,
            fill, &sNdsGradedQuadTextures[slot].name) == FALSE)
    {
        sNdsGradedQuadTextures[slot].name = 0u;
        gNdsGradedQuadTextureFails++;
        return FALSE;
    }
    sNdsRendererHardwareBoundTextureName = sNdsGradedQuadTextures[slot].name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsGradedQuadTextures[slot].image = image;
    sNdsGradedQuadTextures[slot].generation = generation;
    sNdsGradedQuadTextures[slot].prim = prim;
    sNdsGradedQuadTextures[slot].env = env;
    sNdsGradedQuadTextures[slot].frame = sNdsRendererHardwareFrameSerial + 1u;
    gNdsGradedQuadTextureBuilds++;
    return TRUE;
}

/* Match the live guard renderer's max(150, half live radius) camera bias.
 * View-space eye is the origin; only translation changes, never source pose,
 * material animation or lifetime. Use the existing DS integer math units.
 * Shared: Sing's rings were the first user, Samus's Charge Shot the second. */
static void __attribute__((unused)) ndsRendererNativeQuadGuardDepth(
    NDSRendererMatrix20p12 *view, s32 radius)
{
    s32 position[3];
    u64 distance_squared = 0u;
    u32 scale = 0u, distance, bias, axis, i;
    for (i = 0u; i < 3u; i++)
    {
        position[i] = view->m[3][i] >> 12;
        distance_squared += (u64)((s64)position[i] * position[i]);
    }
    distance = ndsR2HwMathSqrt64(distance_squared);
    if (distance <= 1u) return;
    for (axis = 0u; axis < 2u; axis++)
    {
        u64 length_squared = 0u;
        u32 length;
        for (i = 0u; i < 3u; i++)
            length_squared += (u64)((s64)view->m[axis][i] * view->m[axis][i]);
        length = ndsR2HwMathSqrt64(length_squared);
        if (length > scale) scale = length;
    }
    bias = (u32)(((u64)scale * (u32)radius) >> 13);
    if (bias < 150u) bias = 150u;
    for (i = 0u; i < 3u; i++)
        view->m[3][i] -= (s32)ndsR2HwMathDiv64(
            (s64)position[i] * bias * 4096, (s32)distance);
}

/* `image` non-NULL selects the graded copy -- IA8, or CI4 when `tlut` names its
 * fixed palette; the generic cache stays the fallback so a full table degrades,
 * never fails. */
static s32 __attribute__((noinline, noclone)) ndsRendererNativeTexturedQuadGraded(
    const NDSRendererInputVertex vertices[4], const u16 indices[6],
    const u8 *image, const u8 *tlut, const NDSRendererConfig *config,
    NDSRendererStats *stats, NDSRendererTraversalState *state)
{
    const NDSRendererTileState *tile;
    u32 material_color, use_material, use_vertex, alpha, tri, poly_fmt;
    u32 generation;
    u32 graded = FALSE;
    s32 texture_offset;

    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    if (image != NULL)
    {
        NDSGradedQuadFill fill;

        fill.image = image;
        fill.tlut = tlut;
        fill.width = tile->width;
        fill.height = tile->height;
        fill.lane = (config->texture_data_layout ==
                     NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ? 3u : 0u;
        graded = (u32)ndsRendererNativeBindGraded(&fill,
            (tlut != NULL) ? 0u : (stats->prim_color & 0xffffff00u),
            (tlut != NULL) ? 0u : (stats->env_color & 0xffffff00u));
    }
    if ((graded == FALSE) &&
        (ndsRendererHardwareBindTexture(stats, config, state) == FALSE)) return -1;
    alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    if (alpha == 0u) return 0;
    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    generation = ndsRendererNextMatrixGeneration();
    ndsRendererLoadHardwareSplitMatrices(config->initial_projection,
        config->initial_modelview, generation);
    poly_fmt = ndsRendererHardwarePolyFmt(stats, alpha);
    ndsRendererHardwareBeginTriangleBatch(stats, TRUE,
        sNdsRendererHardwareBoundTextureName,
        poly_fmt,
        sNdsRendererHardwareMatrixMode, sNdsRendererHardwareMatrixGeneration);
    for (tri = 0u; tri < 2u; tri++)
    {
        u32 corner;
        for (corner = 0u; corner < 3u; corner++)
        {
            const NDSRendererInputVertex *v = &vertices[indices[tri * 3u + corner]];
            /* The IA8 ramp already holds the combiner colour; CI4 is still
             * TEXEL0 * SHADE. */
            glColor(((graded != FALSE) && (tlut == NULL)) ? 0x7fffu :
                ndsRendererHardwarePackedVertexColor(stats, v, material_color,
                    use_material, use_vertex, 0u, FALSE, 0u));
            glTexCoord2t16(
                ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s, tile->uls, texture_offset),
                ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t, tile->ult, texture_offset));
            glVertex3v16(ndsRendererHardwareVertexCoord(v->x, TRUE),
                ndsRendererHardwareVertexCoord(v->y, TRUE),
                ndsRendererHardwareVertexCoord(v->z, TRUE));
        }
        ndsRendererProfileRecordHardwareTriangle();
    }
    stats->hardware_triangle_count += 2u;
    if ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) != 0u)
        stats->hardware_zbuffer_triangle_count += 2u;
    stats->hardware_vertex_count += 6u;
    sNdsRendererHardwareSubmitted = TRUE;
    ndsRendererHardwareEndBatch();
    return (s32)alpha;
}
static s32 ndsRendererNativeTexturedQuad(
    const NDSRendererInputVertex vertices[4], const u16 indices[6],
    const NDSRendererConfig *config, NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    return ndsRendererNativeTexturedQuadGraded(vertices, indices, NULL, NULL, config, stats, state);
}
static s32 __attribute__((unused)) ndsRendererNativeTexturedQuadIa8(
    const NDSRendererInputVertex vertices[4], const u16 indices[6],
    const u8 *ia8_image, const NDSRendererConfig *config,
    NDSRendererStats *stats, NDSRendererTraversalState *state)
{
    return ndsRendererNativeTexturedQuadGraded(vertices, indices, ia8_image, NULL, config, stats, state);
}
static s32 __attribute__((unused)) ndsRendererNativeTexturedQuadCi4(
    const NDSRendererInputVertex vertices[4], const u16 indices[6],
    const void *ci4_image, const void *tlut, const NDSRendererConfig *config,
    NDSRendererStats *stats, NDSRendererTraversalState *state)
{
    return ndsRendererNativeTexturedQuadGraded(vertices, indices, ci4_image, tlut, config, stats, state);
}
#endif
