/* Source EFCommonEffects1 DamageSlash native GX owner.  TEXTUAL INCLUDE ONLY.
 *
 * File 83 roots 0x75A0/0x7668 are the two drawable children of
 * dEFManagerDamageSlashEffectDesc.  The generated packet owns their immutable
 * quad geometry, fixed TLUT and fixed display-list state.  The source MObj is
 * still authoritative for its animated CURRENT_IMAGE, PRIM and light colours;
 * the adapter snapshots that live state and this owner applies it at the exact
 * segment-E position in the source command stream.
 *
 * No display-list scan, generic list executor or software rasterizer is used.
 */
#if NDS_RENDERER_HW_TRIANGLES

volatile u32 gNdsDamageSlashRootMask;
volatile u32 gNdsDamageSlashEffectsSeen;
volatile u32 gNdsDamageSlashEffectsRejected;
volatile u32 gNdsDamageSlashCandidateStep;
volatile u32 gNdsDamageSlashSnapshotFailCount;
volatile u32 gNdsDamageSlashDrawCount;
volatile u32 gNdsDamageSlashSubmitFailCount;
volatile u32 gNdsDamageSlashSubmitStep;
volatile u32 gNdsDamageSlashAlphaZeroCount;
volatile u32 gNdsDamageSlashTriangleDrawCount;
volatile u32 gNdsDamageSlashTexturePrepareCount;
volatile u32 gNdsDamageSlashTexturePrepareFailCount;
volatile u32 gNdsDamageSlashTextureUpdateCount;
volatile u32 gNdsDamageSlashTextureBindCount;
volatile u32 gNdsDamageSlashBadImageCount;

static u32 sNdsDamageSlashTextureNames[2];
static u32 sNdsDamageSlashTextureLoadedSlot[2] = { 0xffffffffu, 0xffffffffu };
static u32 sNdsDamageSlashTextureGeneration;

#define NDS_DAMAGE_SLASH_MATERIAL_EFFECTS \
    (NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 | \
     NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 | \
     NDS_RENDERER_NATIVE_MATERIAL_PRIM | \
     NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)

typedef struct NDSDamageSlashTextureFill
{
    const NDSRendererConfig *config;
    const u8 *source;
    u32 source_width;
    u32 source_height;
    u32 upload_width;
    u32 upload_height;
    u32 mirror_s;
} NDSDamageSlashTextureFill;

static s32 ndsDamageSlashTextureFill(u8 *pixels, u32 bytes, void *user_data)
{
    const NDSDamageSlashTextureFill *fill =
        (const NDSDamageSlashTextureFill *)user_data;
    u32 expected;
    u32 x;
    u32 y;

    if ((pixels == NULL) || (fill == NULL) || (fill->config == NULL) ||
        (fill->source == NULL) || (fill->source_width == 0u) ||
        (fill->source_height == 0u) || (fill->upload_width == 0u) ||
        (fill->upload_height == 0u) ||
        ((fill->upload_width * fill->upload_height) & 1u) != 0u)
    {
        return FALSE;
    }
    expected = (fill->upload_width * fill->upload_height) >> 1;
    if (bytes < expected)
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    for (y = 0u; y < fill->upload_height; y++)
    {
        u32 source_y = (y < fill->source_height) ?
            y : (fill->source_height - 1u);

        for (x = 0u; x < fill->upload_width; x++)
        {
            u32 source_x = x;
            u32 dst = ((y * fill->upload_width) + x) >> 1;
            u32 index;

            if (fill->mirror_s != 0u)
            {
                u32 period = x / fill->source_width;

                source_x = x % fill->source_width;
                if ((period & 1u) != 0u)
                {
                    source_x = fill->source_width - 1u - source_x;
                }
            }
            else if (source_x >= fill->source_width)
            {
                source_x = fill->source_width - 1u;
            }
            index = ndsRendererReadTexturePackedNibble(
                fill->config, fill->source,
                (source_y * fill->source_width) + source_x,
                NDS_RENDERER_HW_TEXTURE_FMT_CI,
                NDS_RENDERER_HW_TEXTURE_SIZ_4B);
            if ((x & 1u) != 0u)
            {
                pixels[dst] = (u8)((pixels[dst] & 0x0fu) | (index << 4));
            }
            else
            {
                pixels[dst] = (u8)((pixels[dst] & 0xf0u) | index);
            }
        }
    }
    return TRUE;
}

static s32 ndsDamageSlashEnsureTexture(
    const u8 *asset_base, u32 asset_bytes, u32 child, u32 slot,
    const NDSRendererConfig *config)
{
    const u8 *shape;
    u32 offset;
    u32 source_bytes;
    NDSDamageSlashTextureFill fill;
    int size_x;
    int size_y;
    u32 upload_bytes;

    if ((asset_base == NULL) || (config == NULL) || (child > 1u) ||
        (slot >= NDS_NATIVE_DAMAGE_SLASH_TEXTURE_COUNT))
    {
        return FALSE;
    }
    if (sNdsDamageSlashTextureGeneration != gNdsTaskmanHeapGeneration)
    {
        memset(sNdsDamageSlashTextureNames, 0,
               sizeof(sNdsDamageSlashTextureNames));
        sNdsDamageSlashTextureLoadedSlot[0] = 0xffffffffu;
        sNdsDamageSlashTextureLoadedSlot[1] = 0xffffffffu;
        sNdsDamageSlashTextureGeneration = gNdsTaskmanHeapGeneration;
    }
    if (sNdsDamageSlashTextureLoadedSlot[child] == slot)
    {
        return TRUE;
    }
    shape = sNdsDamageSlashTextureShape[slot];
    offset = sNdsDamageSlashTextureOffsets[slot];
    source_bytes = ((u32)shape[0] * shape[1]) >> 1;
    if ((offset > asset_bytes) || (source_bytes > asset_bytes - offset))
    {
        gNdsDamageSlashTexturePrepareFailCount++;
        return FALSE;
    }
    fill.config = config;
    fill.source = asset_base + offset;
    fill.source_width = shape[0];
    fill.source_height = shape[1];
    fill.upload_width = shape[2];
    fill.upload_height = shape[3];
    fill.mirror_s = shape[4];

    if (sNdsDamageSlashTextureNames[child] == 0u)
    {
        if (ndsRendererHardwarePrepareIFCommonPal16Atlas(
                fill.upload_width, fill.upload_height,
                sNdsDamageSlashPalette, ndsDamageSlashTextureFill, &fill,
                &sNdsDamageSlashTextureNames[child]) == FALSE)
        {
            gNdsDamageSlashTexturePrepareFailCount++;
            return FALSE;
        }
        gNdsDamageSlashTexturePrepareCount++;
        sNdsDamageSlashTextureLoadedSlot[child] = slot;
        return TRUE;
    }

    /* libnds reuses an existing allocation when glTexImage2D sees the same
     * format and extent on the same name (libnds 2.0.2 videoGL.o checks the
     * stored byte size/type before its allocator path).  Keep one name per
     * simultaneously drawable child and replace only its texels as the source
     * material animation advances.  This caps DamageSlash at 1.5 KiB texture
     * VRAM instead of asking the already-full battle allocator for 13 names. */
    upload_bytes = ((u32)shape[2] * shape[3]) >> 1;
    if ((upload_bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
        (ndsDamageSlashTextureFill(
             (u8 *)sNdsRendererHardwareTextureScratch, upload_bytes,
             &fill) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(shape[2], &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(shape[3], &size_y) == FALSE))
    {
        gNdsDamageSlashTexturePrepareFailCount++;
        return FALSE;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareBindTextureState((int)sNdsDamageSlashTextureNames[child]);
    if (ndsRendererHardwareFencedGlTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16, size_x, size_y, 0,
            TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT,
            sNdsRendererHardwareTextureScratch) == 0)
    {
        gNdsDamageSlashTexturePrepareFailCount++;
        return FALSE;
    }
    sNdsRendererHardwareBoundTextureName =
        sNdsDamageSlashTextureNames[child];
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsDamageSlashTextureLoadedSlot[child] = slot;
    gNdsDamageSlashTextureUpdateCount++;
    return TRUE;
}

static s32 ndsDamageSlashTextureSlot(
    const u8 *asset_base, u32 asset_bytes, u32 child,
    const void *image_ptr, u32 *slot_out)
{
    uintptr_t base;
    uintptr_t image;
    u32 offset;
    u32 first;
    u32 end;
    u32 i;

    if ((asset_base == NULL) || (image_ptr == NULL) || (slot_out == NULL) ||
        (child > 1u))
    {
        return FALSE;
    }
    base = (uintptr_t)asset_base;
    image = (uintptr_t)image_ptr;
    if ((image < base) || ((image - base) >= asset_bytes))
    {
        return FALSE;
    }
    offset = (u32)(image - base);
    first = (child == 0u) ? NDS_NATIVE_DAMAGE_SLASH_CHILD0_TEXTURE_FIRST :
        NDS_NATIVE_DAMAGE_SLASH_CHILD1_TEXTURE_FIRST;
    end = (child == 0u) ? NDS_NATIVE_DAMAGE_SLASH_CHILD0_TEXTURE_END :
        NDS_NATIVE_DAMAGE_SLASH_CHILD1_TEXTURE_END;
    for (i = first; i < end; i++)
    {
        if (sNdsDamageSlashTextureOffsets[i] == offset)
        {
            *slot_out = i;
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsDamageSlashBindTexture(
    u32 child, u32 slot, NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    const u8 *shape;
    const NDSRendererTileState *tile;
    u32 name;
    u32 params;

    if ((child > 1u) || (slot >= NDS_NATIVE_DAMAGE_SLASH_TEXTURE_COUNT) ||
        (stats == NULL) || (state == NULL))
    {
        return FALSE;
    }
    name = sNdsDamageSlashTextureNames[child];
    if (name == 0u)
    {
        return FALSE;
    }
    shape = sNdsDamageSlashTextureShape[slot];
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    params = ndsRendererHardwareTextureParams(
        stats, tile, shape[2], shape[3]);
    ndsRendererHardwareBindTextureName(stats, name);
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams(params));
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
    stats->hardware_texture_width = tile->width;
    stats->hardware_texture_height = tile->height;
    state->texture_prepare_valid = TRUE;
    state->texture_prepare_enabled = TRUE;
    state->texture_prepare_name = name;
    state->texture_prepare_scale_s = stats->texture_scale_s;
    state->texture_prepare_scale_t = stats->texture_scale_t;
    state->texture_prepare_origin_s = tile->uls;
    state->texture_prepare_origin_t = tile->ult;
    state->texture_prepare_offset = ndsRendererHardwareTextureFilterOffset(stats);
    state->texture_prepare_vertex_flags |=
        NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    ndsRendererProfileRecordTexturePrepare();
    gNdsDamageSlashTextureBindCount++;
    return TRUE;
}

sb32 ndsRendererSubmitNativeDamageSlash(
    const void *asset_base_ptr, u32 asset_bytes, u32 root_offset,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *asset_base = (const u8 *)asset_base_ptr;
    const s16 *flat_vertices;
    const void *palette;
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_DAMAGE_SLASH_VERTEX_COUNT];
    const NDSRendererTileState *tile;
    u32 child;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    u32 texture_slot;
    s32 texture_offset;
    u32 corner;
    u32 i;

    if ((asset_base == NULL) || (material == NULL) || (config == NULL) ||
        (stats == NULL) || (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsDamageSlashSubmitStep = 1u;
        return FALSE;
    }
    if ((asset_bytes < NDS_NATIVE_DAMAGE_SLASH_PALETTE_END) ||
        (root_offset + NDS_NATIVE_DAMAGE_SLASH_DL_BYTES > asset_bytes))
    {
        gNdsDamageSlashSubmitStep = 2u;
        return FALSE;
    }
    if (root_offset == NDS_NATIVE_DAMAGE_SLASH_ROOT0)
    {
        child = 0u;
        flat_vertices = sNdsDamageSlashVerts0;
        palette = asset_base + NDS_NATIVE_DAMAGE_SLASH_PALETTE0_OFFSET;
    }
    else if (root_offset == NDS_NATIVE_DAMAGE_SLASH_ROOT1)
    {
        child = 1u;
        flat_vertices = sNdsDamageSlashVerts1;
        palette = asset_base + NDS_NATIVE_DAMAGE_SLASH_PALETTE1_OFFSET;
    }
    else
    {
        gNdsDamageSlashSubmitStep = 3u;
        return FALSE;
    }
    if (material->effects != NDS_DAMAGE_SLASH_MATERIAL_EFFECTS)
    {
        gNdsDamageSlashEffectsRejected |= material->effects;
        gNdsDamageSlashSubmitStep = 4u;
        return FALSE;
    }

    for (i = 0u; i < NDS_NATIVE_DAMAGE_SLASH_VERTEX_COUNT; i++)
    {
        const s16 *v = &flat_vertices[i * 5u];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4], 0xffu, 0xffu, 0xffu, 0xffu
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (child == 0u)
    {
        ndsDamageSlashSetup0(stats, palette);
    }
    else
    {
        ndsDamageSlashSetup1(stats, palette);
    }
    ndsRendererNativeApplyMaterial(material, stats, &state);
    if (child == 0u)
    {
        ndsDamageSlashAfterMaterial0(stats);
    }
    else
    {
        ndsDamageSlashAfterMaterial1(stats);
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);

    /* POLY_ALPHA(0) is wireframe on DS. Source PRIM alpha 0 means this frame
     * is invisible, so consume it before texture resolution too. The source
     * MaterialAnim deliberately holds/fades at alpha zero; requiring texture
     * VRAM for a frame that submits no pixels turned late-match cache pressure
     * into a false native-render rejection. */
    if (poly_alpha == 0u)
    {
        gNdsDamageSlashAlphaZeroCount++;
        if (child == 0u)
        {
            ndsDamageSlashFinish0(stats);
        }
        else
        {
            ndsDamageSlashFinish1(stats);
        }
        gNdsDamageSlashSubmitStep = 9u;
        return TRUE;
    }

    if (ndsDamageSlashTextureSlot(
            asset_base, asset_bytes, child,
            (const void *)(uintptr_t)material->current_image,
            &texture_slot) == FALSE)
    {
        gNdsDamageSlashBadImageCount++;
        gNdsDamageSlashSubmitStep = 5u;
        return FALSE;
    }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsDamageSlashEnsureTexture(
            asset_base, asset_bytes, child, texture_slot, config) == FALSE)
    {
        gNdsDamageSlashSubmitStep = 6u;
        return FALSE;
    }
    if (ndsDamageSlashBindTexture(child, texture_slot, stats, &state) == FALSE)
    {
        gNdsDamageSlashSubmitStep = 7u;
        return FALSE;
    }
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
    generation = ndsRendererNextMatrixGeneration();
    ndsRendererLoadHardwareSplitMatrices(
        config->initial_projection, config->initial_modelview, generation);
    ndsRendererHardwareBeginTriangleBatch(
        stats, TRUE, sNdsRendererHardwareBoundTextureName,
        ndsRendererHardwarePolyFmt(stats, poly_alpha),
        sNdsRendererHardwareMatrixMode, generation);

    for (corner = 0u; corner < NDS_NATIVE_DAMAGE_SLASH_CORNER_COUNT; corner++)
    {
        const NDSRendererInputVertex *v =
            &vertices[sNdsDamageSlashTriIndices[corner]];

        glColor(ndsRendererHardwarePackedVertexColor(
            stats, v, material_color, use_material, use_vertex,
            0u, FALSE, 0u));
        glTexCoord2t16(
            ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s,
                tile->uls, texture_offset),
            ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t,
                tile->ult, texture_offset));
        glVertex3v16(
            ndsRendererHardwareVertexCoord(v->x, TRUE),
            ndsRendererHardwareVertexCoord(v->y, TRUE),
            ndsRendererHardwareVertexCoord(v->z, TRUE));
    }
    for (i = 0u; i < NDS_NATIVE_DAMAGE_SLASH_TRIANGLE_COUNT; i++)
    {
        ndsRendererProfileRecordHardwareTriangle();
    }
    stats->hardware_triangle_count += NDS_NATIVE_DAMAGE_SLASH_TRIANGLE_COUNT;
    stats->hardware_zbuffer_triangle_count += NDS_NATIVE_DAMAGE_SLASH_TRIANGLE_COUNT;
    stats->hardware_vertex_count += NDS_NATIVE_DAMAGE_SLASH_CORNER_COUNT;
    gNdsDamageSlashTriangleDrawCount++;
    sNdsRendererHardwareSubmitted = TRUE;
    ndsRendererHardwareEndBatch();

    if (child == 0u)
    {
        ndsDamageSlashFinish0(stats);
    }
    else
    {
        ndsDamageSlashFinish1(stats);
    }
    gNdsDamageSlashSubmitStep = 9u;
    return TRUE;
}

#undef NDS_DAMAGE_SLASH_MATERIAL_EFFECTS
#endif
