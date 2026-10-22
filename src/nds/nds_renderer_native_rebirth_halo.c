/* Native renderer, the rebirth halo: its texture, its groups and their packed GX
 * submission. A slice of nds_renderer.c, after nds_renderer_native_common.c. */

#define NDS_REBIRTH_PHASE_MARK(slot, before) do { (void)(before); } while (0)
static s32 ndsRendererHardwareBindRebirthHaloTexture(
    NDSRendererStats *stats, const NDSRendererTileState *render_tile,
    u32 texture_slot, u32 upload_width, u32 upload_height, u32 format)
{
    u32 name;
    u32 params;

    if ((stats == NULL) || (render_tile == NULL) ||
        (texture_slot >= NDS_REBIRTH_HALO_TEXTURE_COUNT))
    {
        return FALSE;
    }
    name = sNdsRendererRebirthHaloTextureName[texture_slot];
    if (name == 0u)
    {
        return FALSE;
    }
    params = ndsRendererHardwareTextureParams(
        stats, render_tile, upload_width, upload_height);
    ndsRendererHardwareBindTextureName(stats, name);
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams(params));
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = upload_width;
    stats->hardware_texture_height = upload_height;
    NDS_DIAG(gNdsRebirthHaloNativeTextureBindCount++);
    return TRUE;
}

static void ndsRendererRebirthHaloGeometry(NDSRendererStats *stats, u32 mode)
{
    stats->geometry_mode = mode;
    stats->geometry_clear_mask = 0u;
    stats->geometry_set_mask = mode;
    stats->geometry_command_count++;
}

static void ndsRendererRebirthHaloSetLightColors(
    NDSRendererStats *stats, u32 light1, u32 light2)
{
    ndsRendererRecordLightColor(stats, 1u, light1);
    ndsRendererRecordLightColor(stats, 2u, light2);
}

static s32 ndsRendererRebirthHaloPrepareGroup(
    u32 group_index,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 *use_texture_out)
{
    const NDSRendererTileState *render_tile = NULL;
    u32 texture_slot = NDS_REBIRTH_HALO_TEXTURE_COUNT;
    u32 upload_width = 0u;
    u32 upload_height = 0u;
    u32 format = 0u;
    u32 material_color;
    u32 use_material_color;
    u32 use_vertex_color;
    u32 implicit_texture_on;
    u32 use_texture = FALSE;
    u32 texture_scale_s;
    u32 texture_scale_t;
    s32 texture_offset;
    u32 poly_alpha;

    if ((stats == NULL) || (state == NULL) || (use_texture_out == NULL) ||
        (group_index >= NDS_REBIRTH_HALO_GROUP_COUNT))
    {
        return FALSE;
    }
    NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);

    switch (group_index)
    {
    case 0u: /* main untextured lit body */
        ndsRendererRebirthHaloGeometry(stats, 0x00220404u);
        ndsRendererRebirthHaloSetLightColors(
            stats, 0xffffff00u, 0xb3b3b300u);
        ndsRendererRecordSetCombine(stats, 0xfcffffffu, 0xfffe7d3eu);
        ndsRendererRecordTextureState(stats, 0xd7000000u, 0x00000000u);
        break;
    case 1u: /* main CI4 E58 */
        ndsRendererRebirthHaloGeometry(stats, 0x00220404u);
        ndsRendererRebirthHaloSetLightColors(
            stats, 0xffffff00u, 0xb3b3b300u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        ndsRendererRecordSetTile(stats, 0xf5400200u, 0x0008c230u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0009c01cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_E58;
        upload_width = NDS_REBIRTH_HALO_E58_WIDTH;
        upload_height = NDS_REBIRTH_HALO_E58_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    case 2u: /* main CI4 DD0 */
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
        ndsRendererRebirthHaloSetLightColors(
            stats, 0xffffff00u, 0xcccccc00u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        ndsRendererRecordSetTile(stats, 0xf5400200u, 0x00090030u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x004fc03cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_DD0;
        upload_width = NDS_REBIRTH_HALO_DD0_WIDTH;
        upload_height = NDS_REBIRTH_HALO_DD0_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    case 3u: /* main CI4 BC8, source lighting already cleared */
        ndsRendererRebirthHaloGeometry(stats, 0x00000004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        ndsRendererRecordSetTile(stats, 0xf5400400u, 0x000d4350u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x000fc0fcu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_BC8;
        upload_width = NDS_REBIRTH_HALO_BC8_WIDTH;
        upload_height = NDS_REBIRTH_HALO_BC8_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    case 4u: /* white beam: I4 source coverage -> A5I3 */
        ndsRendererRebirthHaloGeometry(stats, 0x00000000u);
        ndsRendererRecordSetCombine(stats, 0xfcffffffu, 0xfffdf2f9u);
        stats->prim_color = 0xffffffffu;
        stats->color_command_count++;
        ndsRendererRecordSetTile(stats, 0xf5800200u, 0x00090230u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0001c03cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_BEAM;
        upload_width = NDS_REBIRTH_HALO_BEAM_WIDTH;
        upload_height = NDS_REBIRTH_HALO_BEAM_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_I16;
        break;
    case 5u: /* four CI4 edge leaves, collapsed into one native group */
        ndsRendererRebirthHaloGeometry(stats, 0x00000004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00008000u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe2001e01u, 0x00000001u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe200001cu, 0x00553078u);
        ndsRendererRecordSetCombine(stats, 0xfc127e24u, 0xfffff3f9u);
        stats->blend_color = 0x00000020u;
        stats->color_command_count++;
        ndsRendererRecordSetTile(stats, 0xf5400600u, 0x00090260u);
        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);
        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0009c03cu);
        texture_slot = NDS_REBIRTH_HALO_TEXTURE_A40;
        upload_width = NDS_REBIRTH_HALO_A40_WIDTH;
        upload_height = NDS_REBIRTH_HALO_A40_HEIGHT;
        format = NDS_RENDERER_HW_TEXTURE_FMT_CI;
        break;
    default:
        return FALSE;
    }

    material_color = ndsRendererHardwareColorSource(stats);
    use_material_color =
        (ndsRendererHardwareUseMaterialColor(stats) != FALSE) ? TRUE : FALSE;
    use_vertex_color =
        (ndsRendererHardwareUseVertexColor(stats) != FALSE) ? TRUE : FALSE;
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);

    poly_alpha = ndsRendererHardwareAlpha(stats, &state->input_vertices[0]);
    state->texture_prepare_alpha_constant =
        (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ? TRUE : FALSE;
    state->texture_prepare_poly_alpha = poly_alpha;
    state->texture_prepare_poly_fmt = ndsRendererHardwarePolyFmt(stats, poly_alpha);
    implicit_texture_on =
        ndsRendererHardwareTextureImplicitStateOn(stats) ? TRUE : FALSE;
    if (texture_slot < NDS_REBIRTH_HALO_TEXTURE_COUNT)
    {
        render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        use_texture = ndsRendererHardwareBindRebirthHaloTexture(
            stats, render_tile, texture_slot, upload_width, upload_height,
            format);
        if (use_texture == FALSE)
        {
            return FALSE;
        }
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = TRUE;
        state->texture_prepare_name = sNdsRendererHardwareBoundTextureName;
        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
        if (implicit_texture_on != FALSE)
        {
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
            {
                texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
            {
                texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
        }
        texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = render_tile->uls;
        state->texture_prepare_origin_t = render_tile->ult;
        state->texture_prepare_offset = texture_offset;
        state->texture_prepare_vertex_flags |=
            NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        ndsRendererProfileRecordTexturePrepare();
    }
    else
    {
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled = FALSE;
        state->texture_prepare_name = 0u;
    }
    *use_texture_out = use_texture;
    return TRUE;
}

static void ndsRendererRebirthHaloFinishRoot(
    u32 root_offset, NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    if (root_offset == 0x2378u)
    {
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00000000u);
    }
    else if (root_offset == 0x2a88u)
    {
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
    }
    else if (root_offset == 0x27e8u)
    {
        ndsRendererRebirthHaloGeometry(stats, 0x00220004u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_H,
            0xe3001001u, 0x00000000u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe2001e01u, 0x00000000u);
        ndsRendererRecordOtherMode(
            stats, NDS_RENDERER_OP_SETOTHERMODE_L,
            0xe200001cu, 0x00552078u);
    }
    NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
}

static s32 ndsRendererRebirthHaloBoundsInsideNearPlane(
    const NDSRendererMatrix20p12 *matrix,
    const NDSRebirthHaloBounds *bounds)
{
    u32 corner;

    for (corner = 0u; corner < 8u; corner++)
    {
        NDSRendererInputVertex input;
        NDSRendererClipVertex20p12 clip;

        /* The transform reads only XYZ. Avoid clearing the texture/normal tail
         * of this temporary eight times per admission test. */
        input.x = ((corner & 1u) != 0u) ? bounds->max_x : bounds->min_x;
        input.y = ((corner & 2u) != 0u) ? bounds->max_y : bounds->min_y;
        input.z = ((corner & 4u) != 0u) ? bounds->max_z : bounds->min_z;
        ndsRendererTransformVertex20p12(matrix, &input, &clip);
        NDS_DIAG(gNdsRebirthHaloFullOffloadBoundCornerCount++);
        if (ndsRendererHardwareClipZWInsideNearPlane(clip.z, clip.w) == FALSE)
        {
            NDS_DIAG(gNdsRebirthHaloFullOffloadBoundRejectCount++);
            return FALSE;
        }
    }
    return TRUE;
}

/* Conservative union per native RebirthHalo root.  Groups 0..3 share the
 * 0x2378 child transform, so their four AABBs do not need four separate
 * eight-corner clip tests.  The other two roots each own one generated group. */
static const NDSRebirthHaloBounds sNdsRebirthHaloRootBounds[3] = {
    { -276, -90, -239, 276, 60, 239 },
    { -242, 60, -209, 242, 240, 209 },
    { -310, -59, -310, 310, 0, 310 },
};


#define NDS_REBIRTH_HALO_NORMAL_PACK(x, y, z) \
    ((((u32)(x)) & 0x3ffu) | \
     (((((u32)(y)) & 0x3ffu)) << 10) | \
     (((((u32)(z)) & 0x3ffu)) << 20))

static s32 ndsRendererRebirthHaloNormalComponent(s32 source)
{
    s32 scaled = (source * 0x1ff) / 127;

    if (scaled > 511) { scaled = 511; }
    if (scaled < -512) { scaled = -512; }
    return scaled;
}

static u16 ndsRendererRebirthHaloMaterialColor15(
    u32 light_color, u32 material_color, u32 use_material,
    u32 color_modulate)
{
    u32 r = (use_material != 0u) ?
        ndsRendererHardwareScaleMaterialChannel5(
            (light_color >> 24) & 0xffu,
            (material_color >> 24) & 0xffu) :
        ((light_color >> 27) & 0x1fu);
    u32 g = (use_material != 0u) ?
        ndsRendererHardwareScaleMaterialChannel5(
            (light_color >> 16) & 0xffu,
            (material_color >> 16) & 0xffu) :
        ((light_color >> 19) & 0x1fu);
    u32 b = (use_material != 0u) ?
        ndsRendererHardwareScaleMaterialChannel5(
            (light_color >> 8) & 0xffu,
            (material_color >> 8) & 0xffu) :
        ((light_color >> 11) & 0x1fu);

    return ndsRendererHardwareModulatePackedColor(
        RGB15(r, g, b), color_modulate);
}

#define NDS_REBIRTH_HALO_PACKET_WORDS 256u
typedef struct NDSRebirthHaloPacket
{
    u32 words[NDS_REBIRTH_HALO_PACKET_WORDS];
    u16 word_count;
    u8 valid;
    u8 pending_count;
    u8 pending_opcode[4];
    u8 pending_param_count[4];
    u8 pending_dynamic_color[4];
    u8 dynamic_color_count;
    u16 dynamic_color_word_offset[36];
    u32 pending_param[4][2];
} NDSRebirthHaloPacket;

static NDSRebirthHaloPacket
    sNdsRebirthHaloPackets[NDS_REBIRTH_HALO_GROUP_COUNT]
    __attribute__((aligned(32)));
volatile u32 gNdsRebirthHaloPackedBuildCount;
volatile u32 gNdsRebirthHaloPackedSubmitCount;
volatile u32 gNdsRebirthHaloPackedWordCount;

static s32 ndsRendererRebirthHaloPacketPushWord(
    NDSRebirthHaloPacket *packet, u32 word)
{
    if ((packet == NULL) ||
        ((u32)packet->word_count >= NDS_REBIRTH_HALO_PACKET_WORDS))
    {
        return FALSE;
    }
    packet->words[packet->word_count++] = word;
    return TRUE;
}

static s32 ndsRendererRebirthHaloPacketFlush(NDSRebirthHaloPacket *packet)
{
    u32 command_word = 0u;
    u32 command;

    if ((packet == NULL) || (packet->pending_count == 0u))
    {
        return TRUE;
    }
    for (command = 0u; command < (u32)packet->pending_count; command++)
    {
        command_word |=
            (u32)packet->pending_opcode[command] << (command * 8u);
    }
    if (ndsRendererRebirthHaloPacketPushWord(packet, command_word) == FALSE)
    {
        return FALSE;
    }
    for (command = 0u; command < (u32)packet->pending_count; command++)
    {
        u32 parameter;
        for (parameter = 0u;
             parameter < (u32)packet->pending_param_count[command];
             parameter++)
        {
            if ((parameter == 0u) &&
                (packet->pending_dynamic_color[command] != 0u))
            {
                if ((u32)packet->dynamic_color_count >= 36u)
                {
                    return FALSE;
                }
                packet->dynamic_color_word_offset[packet->dynamic_color_count++] =
                    packet->word_count;
            }
            if (ndsRendererRebirthHaloPacketPushWord(
                    packet, packet->pending_param[command][parameter]) == FALSE)
            {
                return FALSE;
            }
        }
    }
    packet->pending_count = 0u;
    return TRUE;
}

static s32 ndsRendererRebirthHaloPacketCommand(
    NDSRebirthHaloPacket *packet, u8 opcode,
    u32 parameter_count, u32 parameter0, u32 parameter1,
    u32 dynamic_color)
{
    u32 slot;

    if ((packet == NULL) || (parameter_count > 2u))
    {
        return FALSE;
    }
    slot = packet->pending_count;
    if (slot >= 4u)
    {
        if (ndsRendererRebirthHaloPacketFlush(packet) == FALSE)
        {
            return FALSE;
        }
        slot = 0u;
    }
    packet->pending_opcode[slot] = opcode;
    packet->pending_param_count[slot] = (u8)parameter_count;
    packet->pending_dynamic_color[slot] =
        (dynamic_color != FALSE) ? 1u : 0u;
    packet->pending_param[slot][0] = parameter0;
    packet->pending_param[slot][1] = parameter1;
    packet->pending_count = (u8)(slot + 1u);
    if (packet->pending_count == 4u)
    {
        return ndsRendererRebirthHaloPacketFlush(packet);
    }
    return TRUE;
}

static s32 ndsRendererRebirthHaloBuildPackedGroup(
    u32 group_index, const NDSRebirthHaloGroup *group,
    NDSRendererStats *stats, NDSRendererTraversalState *state,
    const NDSRendererHardwareLightDirection *prepared_direction,
    u32 use_texture, u32 use_hw_light)
{
    NDSRebirthHaloPacket *packet;
    u32 context_flags;
    u32 corner_count;
    u32 corner;

    if ((group_index >= NDS_REBIRTH_HALO_GROUP_COUNT) ||
        (group == NULL) || (stats == NULL) || (state == NULL))
    {
        return FALSE;
    }
    packet = &sNdsRebirthHaloPackets[group_index];
    if (packet->valid != 0u)
    {
        return TRUE;
    }
    packet->word_count = 0u;
    packet->pending_count = 0u;
    packet->dynamic_color_count = 0u;
    context_flags = state->texture_prepare_vertex_flags;
    corner_count = (u32)group->triangle_count * 3u;

    for (corner = 0u; corner < corner_count; corner++)
    {
        u32 source_index = (u32)group->first_vertex + corner;
        const NDSRendererInputVertex *vtx =
            &sNdsRebirthHaloVertices[source_index];
        v16 x = ndsRendererHardwareVertexCoord(vtx->x, TRUE);
        v16 y = ndsRendererHardwareVertexCoord(vtx->y, TRUE);
        v16 z = ndsRendererHardwareVertexCoord(vtx->z, TRUE);

        if (use_hw_light != FALSE)
        {
            u32 normal = NDS_REBIRTH_HALO_NORMAL_PACK(
                ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->r),
                ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->g),
                ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->b));
            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_NORMAL, 1u, normal, 0u, FALSE) == FALSE)
            {
                return FALSE;
            }
        }
        else
        {
            u32 shade = ndsRendererHardwareLitShadeColorPrepared(
                stats, vtx, prepared_direction);
            u16 packed_color = ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                shade, TRUE, state->color_modulate);
            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_COLOR, 1u, (u32)packed_color, 0u,
                    (prepared_direction != NULL) ? TRUE : FALSE) == FALSE)
            {
                return FALSE;
            }
        }
        if (use_texture != FALSE)
        {
            t16 s = ndsRendererHardwareTexCoord(
                vtx->s, state->texture_prepare_scale_s,
                state->texture_prepare_origin_s,
                state->texture_prepare_offset);
            t16 t = ndsRendererHardwareTexCoord(
                vtx->t, state->texture_prepare_scale_t,
                state->texture_prepare_origin_t,
                state->texture_prepare_offset);
            if (ndsRendererRebirthHaloPacketCommand(
                    packet, FIFO_TEX_COORD, 1u,
                    (u32)TEXTURE_PACK(s, t), 0u, FALSE) == FALSE)
            {
                return FALSE;
            }
        }
        if (ndsRendererRebirthHaloPacketCommand(
                packet, FIFO_VERTEX16, 2u,
                ((u32)(u16)x) | ((u32)(u16)y << 16),
                (u32)(s32)z, FALSE) == FALSE)
        {
            return FALSE;
        }
    }
    if (ndsRendererRebirthHaloPacketFlush(packet) == FALSE)
    {
        return FALSE;
    }
    DC_FlushRange(packet->words, (u32)packet->word_count * sizeof(u32));
    packet->valid = 1u;
    NDS_DIAG(gNdsRebirthHaloPackedBuildCount++);
    NDS_DIAG(gNdsRebirthHaloPackedWordCount += packet->word_count);
    return TRUE;
}

static s32 ndsRendererRebirthHaloPatchPackedColors(
    u32 group_index, const NDSRebirthHaloGroup *group,
    NDSRendererStats *stats, NDSRendererTraversalState *state,
    const NDSRendererHardwareLightDirection *prepared_direction)
{
    NDSRebirthHaloPacket *packet;
    u32 context_flags;
    u32 corner_count;
    u32 corner;

    if ((group_index >= NDS_REBIRTH_HALO_GROUP_COUNT) ||
        (group == NULL) || (stats == NULL) || (state == NULL))
    {
        return FALSE;
    }
    packet = &sNdsRebirthHaloPackets[group_index];
    corner_count = (u32)group->triangle_count * 3u;
    if (packet->dynamic_color_count == 0u)
    {
        return TRUE;
    }
    if ((prepared_direction == NULL) ||
        ((u32)packet->dynamic_color_count != corner_count))
    {
        return FALSE;
    }
    context_flags = state->texture_prepare_vertex_flags;
    for (corner = 0u; corner < corner_count; corner++)
    {
        u32 source_index = (u32)group->first_vertex + corner;
        const NDSRendererInputVertex *vtx =
            &sNdsRebirthHaloVertices[source_index];
        u32 shade = ndsRendererHardwareLitShadeColorPrepared(
            stats, vtx, prepared_direction);
        u16 packed_color = ndsRendererHardwarePackedVertexColor(
            stats, vtx, state->texture_prepare_material_color,
            (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
            (s32)(context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
            shade, TRUE, state->color_modulate);
        packet->words[packet->dynamic_color_word_offset[corner]] =
            (u32)packed_color;
    }
    DC_FlushRange(packet->words, (u32)packet->word_count * sizeof(u32));
    return TRUE;
}


static void ndsRendererRebirthHaloSubmitPackedGroup(u32 group_index)
{
    const NDSRebirthHaloPacket *packet = &sNdsRebirthHaloPackets[group_index];
    NDS_GX_DMA0_WAIT(2u);
    DMA_SRC(0) = (u32)packet->words;
    DMA_DEST(0) = (u32)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | packet->word_count;
    NDS_GX_DMA0_WAIT(3u);
    NDS_DIAG(gNdsRebirthHaloPackedSubmitCount++);
}

s32 ndsRendererSubmitNativeRebirthHalo(
    u32 root_offset,
    const NDSRendererConfig *config,
    NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    NDSRendererTraversalVertexStorage vertex_storage;
    NDSRendererTraversalState state;
    u32 first_group;
    u32 last_group;
    u32 group_index;
    u32 root_bounds_index;

    if ((config == NULL) || (stats == NULL))
    {
        return FALSE;
    }
    if (root_offset == 0x2378u)
    {
        first_group = 0u;
        last_group = 4u;
        root_bounds_index = 0u;
    }
    else if (root_offset == 0x2a88u)
    {
        first_group = 4u;
        last_group = 5u;
        root_bounds_index = 1u;
    }
    else if (root_offset == 0x27e8u)
    {
        first_group = 5u;
        last_group = 6u;
        root_bounds_index = 2u;
    }
    else
    {
        return FALSE;
    }
    for (group_index = first_group; group_index < last_group; group_index++)
    {
        u32 slot = (group_index == 1u) ? NDS_REBIRTH_HALO_TEXTURE_E58 :
                   (group_index == 2u) ? NDS_REBIRTH_HALO_TEXTURE_DD0 :
                   (group_index == 3u) ? NDS_REBIRTH_HALO_TEXTURE_BC8 :
                   (group_index == 4u) ? NDS_REBIRTH_HALO_TEXTURE_BEAM :
                   (group_index == 5u) ? NDS_REBIRTH_HALO_TEXTURE_A40 :
                                         NDS_REBIRTH_HALO_TEXTURE_COUNT;
        if ((slot < NDS_REBIRTH_HALO_TEXTURE_COUNT) &&
            (sNdsRendererRebirthHaloTextureName[slot] == 0u))
        {
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(
        &state, config, stats, &vertex_storage, NULL, 0u);
    if (state.matrix_valid == 0u)
    {
        return FALSE;
    }

    /* Preflight before the first GX side effect.  The accepted native path
     * tested every triangle corner here, then transformed every corner a
     * second time to draw it.  The full-offload path proves a conservative
     * immutable AABB instead: the near predicate is linear in clip space, so
     * all eight AABB corners inside implies every enclosed source vertex is
     * inside.  Anything touching the danger band still falls back before GX. */
    /* Split source matrices, natural Z, every group transformed by the GX:
     * the hardware owns the multiply, divide and near-plane clip, and no CPU
     * projection or synthetic depth is made, so the danger band does not
     * exist on this route. A halo crossing the near plane (a 1P respawn right
     * at the camera: Fox's Sector Z walk, 2026-10-04, 12 NO_PROGRAM failures
     * over three frames) now draws as the N64 clips it instead of failing. */
    if (ndsRendererRebirthHaloBoundsInsideNearPlane(
            &state.matrix, &sNdsRebirthHaloRootBounds[root_bounds_index]) == FALSE)
    {
        NDS_DIAG(gNdsRebirthHaloNearPlaneGxCount++);
    }


    ndsRendererHardwareEndBatch();
    /* Two lab routes are deliberately kept side by side.  The composed route
     * preserves the accepted owner's CPU multiply and synthetic no-Z contract.
     * SPLIT_MTX retries the earlier experiment whose screenshot was externally
     * contaminated: the DS receives source projection and modelview separately
     * and owns their multiply, projection, divide, clipping, and natural Z. */
    if ((state.projection_valid == 0u) || (state.modelview_valid == 0u))
    {
        return FALSE;
    }
    ndsRendererLoadHardwareSplitMatrices(
        &state.projection, &state.modelview, state.matrix_generation);
    NDS_DIAG(gNdsRebirthHaloFullOffloadRootCount++);
    for (group_index = first_group; group_index < last_group; group_index++)
    {
        const NDSRebirthHaloGroup *group = &sNdsRebirthHaloGroups[group_index];
        const NDSRendererHardwareLightDirection *prepared_direction = NULL;
        NDSRendererHardwareLightDirection direction;
        u32 use_texture;
        u32 triangle;
        u32 use_gx_group =
            ((NDS_R2_REBIRTH_HALO_GX_GROUP_MASK & (1u << group_index)) != 0u) ?
                TRUE : FALSE;
        u32 use_hw_light = FALSE;

        /* Seed slot 0 before deriving the group's alpha/material preparation. */
        state.input_vertices[0] =
            sNdsRebirthHaloVertices[(u32)group->first_vertex];
        if (ndsRendererRebirthHaloPrepareGroup(
                group_index, stats, &state, &use_texture) == FALSE)
        {
            return FALSE;
        }
        if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
            ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
        {
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state.modelview_valid != 0u) ? &state.modelview : NULL,
                &direction);
            prepared_direction = &direction;
        }
        if (use_gx_group == FALSE)
        {
            ndsRendererLoadHardwareMatrices(NULL, FALSE);
        }
        {
            use_hw_light = ((group_index <= 1u) &&
                            (prepared_direction != NULL)) ? TRUE : FALSE;
            if (use_hw_light != FALSE)
            {
                s32 lx = ndsRendererRebirthHaloNormalComponent(-prepared_direction->x);
                s32 ly = ndsRendererRebirthHaloNormalComponent(-prepared_direction->y);
                s32 lz = ndsRendererRebirthHaloNormalComponent(-prepared_direction->z);
                u32 use_material =
                    (state.texture_prepare_vertex_flags &
                     NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL) != 0u;
                u32 diffuse = ndsRendererRebirthHaloMaterialColor15(
                    stats->light_color_1, state.texture_prepare_material_color,
                    use_material, state.color_modulate);
                u32 ambient = ndsRendererRebirthHaloMaterialColor15(
                    stats->light_color_2, state.texture_prepare_material_color,
                    use_material, state.color_modulate);

                /* prepared_direction is already transformed/normalised exactly
                 * like the accepted software shade.  GFX_LIGHT_VECTOR itself is
                 * transformed by the current vector matrix when written, so
                 * bracket the write with identity and restore the split
                 * modelview before any NORMAL command is submitted. */
                ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();
                GFX_LIGHT_VECTOR = NDS_REBIRTH_HALO_NORMAL_PACK(lx, ly, lz);
                glPopMatrix(1);
                ndsRendererHardwareWriteDiffuseAmbient(
                    diffuse | (ambient << 16));
                state.texture_prepare_poly_fmt |= POLY_FORMAT_LIGHT0;
            }
        }
        ndsRendererHardwareBeginTriangleBatch(
            stats, use_texture, state.texture_prepare_name,
            state.texture_prepare_poly_fmt,
            sNdsRendererHardwareMatrixMode,
            sNdsRendererHardwareMatrixGeneration);

        /* Packet topology/geometry/texcoords are immutable. Software-lit groups
         * record the COLOR parameter offsets once, then patch only those live
         * words before DMA; hardware-lit packets need no per-frame patching. */
        if ((use_gx_group != FALSE) &&
            ndsRendererRebirthHaloBuildPackedGroup(
                group_index, group, stats, &state, prepared_direction,
                use_texture, use_hw_light) != FALSE &&
            ndsRendererRebirthHaloPatchPackedColors(
                group_index, group, stats, &state, prepared_direction) != FALSE)
        {
            ndsRendererRebirthHaloSubmitPackedGroup(group_index);
            sNdsRendererHardwareSubmitted = TRUE;
            stats->triangle_count += group->triangle_count;
            stats->transformed_triangle_count += group->triangle_count;
            stats->hardware_triangle_count += group->triangle_count;
            stats->hardware_vertex_count += (u32)group->triangle_count * 3u;
            ndsRendererHardwareEndBatch();
            continue;
        }

        for (triangle = 0u; triangle < (u32)group->triangle_count; triangle++)
        {
            if (use_gx_group != FALSE)
            {
                u32 context_flags = state.texture_prepare_vertex_flags;
                u32 corner;

                /* Every source corner is already immutable/AOT and the group
                 * state above is constant for the whole run.  Bypass the
                 * generic three-slot cache/mask machinery completely. */
                for (corner = 0u; corner < 3u; corner++)
                {
                    u32 source_index =
                        (u32)group->first_vertex + triangle * 3u + corner;
                    const NDSRendererInputVertex *vtx =
                        &sNdsRebirthHaloVertices[source_index];
                    if (use_hw_light != FALSE)
                    {
                        glNormal(NDS_REBIRTH_HALO_NORMAL_PACK(
                            ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->r),
                            ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->g),
                            ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->b)));
                    }
                    else
                    {
                    u32 shade = ndsRendererHardwareLitShadeColorPrepared(
                        stats, vtx, prepared_direction);
                    u16 packed_color = ndsRendererHardwarePackedVertexColor(
                        stats, vtx, state.texture_prepare_material_color,
                        (s32)(context_flags &
                              NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                        (s32)(context_flags &
                              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                        shade, TRUE, state.color_modulate);

                    glColor(packed_color);
                    }
                    if (use_texture != FALSE)
                    {
                        glTexCoord2t16(
                            ndsRendererHardwareTexCoord(
                                vtx->s, state.texture_prepare_scale_s,
                                state.texture_prepare_origin_s,
                                state.texture_prepare_offset),
                            ndsRendererHardwareTexCoord(
                                vtx->t, state.texture_prepare_scale_t,
                                state.texture_prepare_origin_t,
                                state.texture_prepare_offset));
                    }
                    glVertex3v16(
                        ndsRendererHardwareVertexCoord(vtx->x, TRUE),
                        ndsRendererHardwareVertexCoord(vtx->y, TRUE),
                        ndsRendererHardwareVertexCoord(vtx->z, TRUE));
                }
            }
            else
            {
            v16 projected_x[3];
            v16 projected_y[3];
            u16 packed_color[3];
            u32 packed_normal[3];
            t16 prepared_s[3];
            t16 prepared_t[3];
            u32 context_flags = state.texture_prepare_vertex_flags;
            u32 corner;
            s32 depth = ndsRendererHardwareNextProjectedDepth();

            for (corner = 0u; corner < 3u; corner++)
            {
                u32 source_index = (u32)group->first_vertex + triangle * 3u + corner;
                const NDSRendererInputVertex *vtx =
                    &sNdsRebirthHaloVertices[source_index];
                NDSRendererClipVertex20p12 clip;
                u32 shade = ndsRendererHardwareLitShadeColorPrepared(
                    stats, vtx, prepared_direction);

                packed_color[corner] = ndsRendererHardwarePackedVertexColor(
                    stats, vtx, state.texture_prepare_material_color,
                    (s32)(context_flags &
                          NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                    (s32)(context_flags &
                          NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                    shade, TRUE, state.color_modulate);
                if (use_hw_light != FALSE)
                {
                    packed_normal[corner] = NDS_REBIRTH_HALO_NORMAL_PACK(
                        ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->r),
                        ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->g),
                        ndsRendererRebirthHaloNormalComponent((s32)(s8)vtx->b));
                }
                if (use_texture != FALSE)
                {
                    prepared_s[corner] = ndsRendererHardwareTexCoord(
                        vtx->s, state.texture_prepare_scale_s,
                        state.texture_prepare_origin_s,
                        state.texture_prepare_offset);
                    prepared_t[corner] = ndsRendererHardwareTexCoord(
                        vtx->t, state.texture_prepare_scale_t,
                        state.texture_prepare_origin_t,
                        state.texture_prepare_offset);
                }
                ndsRendererTransformVertex20p12(&state.matrix, vtx, &clip);
                projected_x[corner] = ndsRendererHardwareProjectToV16(
                    (s64)clip.x * NDS_RENDERER_HW_PROJECTED_VERTEX, clip.w);
                projected_y[corner] = ndsRendererHardwareProjectToV16(
                    (s64)clip.y * NDS_RENDERER_HW_PROJECTED_VERTEX, clip.w);
                stats->matrix_transform_count++;
                stats->transformed_vertex_count++;
                ndsRendererProfileRecordCPUTransform();
                ndsRendererProfileRecordSourceVertexLoad();
            }
            for (corner = 0u; corner < 3u; corner++)
            {
                v16 out_z = ndsRendererHardwareClampS64ToV16(depth);

                if (use_hw_light != FALSE)
                {
                    glNormal(packed_normal[corner]);
                }
                else
                {
                    glColor(packed_color[corner]);
                }
                if (use_texture != FALSE)
                {
                    glTexCoord2t16(prepared_s[corner], prepared_t[corner]);
                }
                ndsRendererProfileHWVertexRange(
                    projected_x[corner], projected_y[corner], out_z);
                glVertex3v16(projected_x[corner], projected_y[corner], out_z);
            }
            }
            sNdsRendererHardwareSubmitted = TRUE;
            stats->triangle_count++;
            stats->transformed_triangle_count++;
            stats->hardware_triangle_count++;
            stats->hardware_vertex_count += 3u;
            if (use_gx_group == FALSE)
            {
                stats->hardware_projected_depth_triangle_count++;
                ndsRendererProfileRecordProjectedSubmit();
            }
            ndsRendererProfileRecordHardwareTriangle();
        }
        ndsRendererHardwareEndBatch();
    }
    ndsRendererRebirthHaloFinishRoot(root_offset, stats, &state);
    stats->end_command_count++;
    return TRUE;
}
#undef NDS_REBIRTH_PHASE_MARK
