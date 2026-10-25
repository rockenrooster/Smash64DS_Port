/* Native renderer, entry effects: the lit corners (the normal and light
 * words, the fast-corner tables) and the packet arena, recorder and DMA
 * replay. A slice of nds_renderer.c. */

/* libnds's NORMAL_PACK does not mask its z argument -- it is
 * `(x & 0x3FF) | ((y & 0x3FF) << 10) | (z << 20)` -- so a negative z sign-
 * extends into bits 30 and 31. Those bits are unused in GFX_NORMAL but are the
 * *light number* in GFX_LIGHT_VECTOR, so NORMAL_PACK(0, 0, -511) writes
 * 0xE0100000: light 3, which POLY_FORMAT never enables. Light 0 kept its
 * power-on zero vector, every dot product was zero, and the fighters rendered
 * with ambient only. Mask all three components. */
#define NDS_NORMAL_PACK(x, y, z) \
    ((((u32)(x)) & 0x3ffu) | \
     (((((u32)(y)) & 0x3ffu)) << 10) | \
     (((((u32)(z)) & 0x3ffu)) << 20))

/* Source normals are s8 with 1.0 == 127; the DS normal is 10-bit signed with
 * 1.0 == 0x1ff. Load time, so the exact scale costs nothing. */
static s32 ndsRendererNormalComponent(s32 source)
{
    s32 scaled = (source * 0x1ff) / 127;

    if (scaled > 511) { scaled = 511; }
    if (scaled < -512) { scaled = -512; }
    return scaled;
}

/* The entry-effect owner's two words in the same units: a corner's source
 * normal, and light 0 from a prepared direction (model space, normalised to
 * 127), negated for the engine's max(0, -L.N). */
static inline u32 ndsRendererEntryEffectNormalWord(
    const NDSRendererInputVertex *vtx)
{
    return NDS_NORMAL_PACK(
        (int)ndsRendererNormalComponent((s32)(s8)vtx->r),
        (int)ndsRendererNormalComponent((s32)(s8)vtx->g),
        (int)ndsRendererNormalComponent((s32)(s8)vtx->b));
}

static inline u32 ndsRendererEntryEffectLightWord(
    const NDSRendererHardwareLightDirection *direction)
{
    return NDS_NORMAL_PACK(
        (int)ndsRendererNormalComponent(-direction->x),
        (int)ndsRendererNormalComponent(-direction->y),
        (int)ndsRendererNormalComponent(-direction->z));
}

/* The GFX_NORMAL word of every source colour entry (the corners' normals are
 * colour entries). The table is const, so the words are built once. */
static u32 sNdsEntryEffectNormalWords[NDS_ENTRY_EFFECT_COLOR_COUNT];
static u8 sNdsEntryEffectFastCornersState;

static u32 __attribute__((noinline)) ndsRendererEntryEffectFastCornersBuild(void)
{
    u32 i;

    for (i = 0u; i < NDS_ENTRY_EFFECT_COLOR_COUNT; i++)
    {
        NDSRendererInputVertex vertex = {0};

        vertex.r = sNdsEntryEffectColors[i].r;
        vertex.g = sNdsEntryEffectColors[i].g;
        vertex.b = sNdsEntryEffectColors[i].b;
        sNdsEntryEffectNormalWords[i] = ndsRendererEntryEffectNormalWord(&vertex);
    }
    sNdsEntryEffectFastCornersState = 1u;
    return 1u;
}

static u32 ndsRendererEntryEffectFastCornersReady(void)
{
    if (sNdsEntryEffectFastCornersState == 0u)
    {
        return ndsRendererEntryEffectFastCornersBuild();
    }
    return 1u;
}

/* One corner run; `lit` and `textured` are constants at each call below, so
 * each of the four loops carries only its own writes. */
static inline __attribute__((always_inline)) void
ndsRendererEntryEffectFastCornerLoop(u32 first, u32 count, u32 lit,
                                     u32 textured)
{
    const u32 shift = 12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT;
    const u16 *position_index = &sNdsEntryCornerPosition[first];
    const u16 *color_index = &sNdsEntryCornerColor[first];
    const NDS_ENTRY_EFFECT_UV_INDEX_T *s_index = &sNdsEntryCornerS[first];
    const NDS_ENTRY_EFFECT_UV_INDEX_T *t_index = &sNdsEntryCornerT[first];
    const u16 *end = position_index + count;

    while (position_index != end)
    {
        const NDSEntryEffectPosition *position =
            &sNdsEntryEffectPositions[*position_index++];

        if (lit != 0u)
        {
            ndsRendererHardwareWriteNormalWord(
                sNdsEntryEffectNormalWords[*color_index]);
        }
        color_index++;
        if (textured != 0u)
        {
            glTexCoord2t16((t16)sNdsEntryEffectS[*s_index],
                           (t16)sNdsEntryEffectT[*t_index]);
        }
        s_index++;
        t_index++;
        glVertex3v16((v16)((s32)position->x << shift),
                     (v16)((s32)position->y << shift),
                     (v16)((s32)position->z << shift));
    }
}

/* A SHADE group's corners with their own vertex colours, packed as the corner
 * loop in ndsRendererSubmitNativeEntryEffect packs an unlit corner's
 * (ndsRendererHardwarePackedValidVertexColor with a vertex colour). One loop
 * for both texture cases: two cost 776 B of .main for a branch a corner. */
static void __attribute__((noinline)) ndsRendererEntryEffectEmitShadedCorners(
    u32 first, u32 count, u32 material_color, s32 use_material_color,
    u32 textured)
{
    const u32 shift = 12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT;
    const u16 *position_index = &sNdsEntryCornerPosition[first];
    const u16 *color_index = &sNdsEntryCornerColor[first];
    const NDS_ENTRY_EFFECT_UV_INDEX_T *s_index = &sNdsEntryCornerS[first];
    const NDS_ENTRY_EFFECT_UV_INDEX_T *t_index = &sNdsEntryCornerT[first];
    const u16 *end = position_index + count;

    while (position_index != end)
    {
        const NDSEntryEffectPosition *position =
            &sNdsEntryEffectPositions[*position_index++];
        const NDSEntryEffectColor *color =
            &sNdsEntryEffectColors[*color_index++];

        glColor(ndsRendererHardwarePackedResolvedColor(
            ((u32)color->r << 24) | ((u32)color->g << 16) |
            ((u32)color->b << 8) | (u32)color->a,
            material_color, use_material_color, 0u));
        if (textured != 0u)
        {
            glTexCoord2t16((t16)sNdsEntryEffectS[*s_index],
                           (t16)sNdsEntryEffectT[*t_index]);
        }
        s_index++;
        t_index++;
        glVertex3v16((v16)((s32)position->x << shift),
                     (v16)((s32)position->y << shift),
                     (v16)((s32)position->z << shift));
    }
}

static void __attribute__((noinline)) ndsRendererEntryEffectEmitFastCorners(
    u32 first, u32 count, u32 lit, u32 textured)
{
    if (lit != 0u)
    {
        if (textured != 0u)
        {
            ndsRendererEntryEffectFastCornerLoop(first, count, 1u, 1u);
        }
        else
        {
            ndsRendererEntryEffectFastCornerLoop(first, count, 1u, 0u);
        }
    }
    else if (textured != 0u)
    {
        ndsRendererEntryEffectFastCornerLoop(first, count, 0u, 1u);
    }
    else
    {
        ndsRendererEntryEffectFastCornerLoop(first, count, 0u, 0u);
    }
}

/* The Arwing packet's bodies; described above ndsRendererSubmitNativeEntryEffect. */
/* The most words a root's record takes: per group TEX_FORMAT, PAL_FORMAT,
 * DIF_AMB, POLY_FORMAT, BEGIN and COLOR; per corner NORMAL, TEX_COORD and
 * VERTEX16's two words; a header per four commands, and one more for the
 * header the split closes early. Rounded to 32 bytes. */
static u32 ndsRendererEntryEffectPacketBound(u32 slot)
{
    const NDSEntryEffectRoot *root =
        &sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_FOX_ROOT_FIRST + slot];
    u32 params = 0u;
    u32 commands = 0u;
    u32 g;

    for (g = 0u; g < root->group_count; g++)
    {
        u32 corners = (u32)sNdsEntryEffectGroups[
            (u32)root->first_group + g].triangle_count * 3u;

        params += 6u + corners * 4u;
        commands += 6u + corners * 3u;
    }
    return (params + (commands + 3u) / 4u + 1u + 7u) & ~7u;
}

u32 ndsRendererEntryEffectPacketArenaWords(void)
{
    u32 words = 0u;
    u32 slot;

    for (slot = 0u; slot < NDS_ENTRY_STATE_ROOTS; slot++)
    {
        words += ndsRendererEntryEffectPacketBound(slot);
    }
    return words;
}

/* Sector Z's stage setup hands over an arena from the scene heap, or NULL; it
 * is used while the heap generation it was set in lasts. */
void ndsRendererEntryEffectPacketArenaSet(u32 *arena, u32 words)
{
    u32 used = 0u;
    u32 slot;

    memset(sNdsEntryEffectPackets, 0, sizeof(sNdsEntryEffectPackets));
    sNdsEntryEffectPacketArenaGeneration = gNdsTaskmanHeapGeneration;
    gNdsEntryEffectPacketArenaWords = (arena != NULL) ? words : 0u;
    if (arena == NULL)
    {
        return;
    }
    for (slot = 0u; slot < NDS_ENTRY_STATE_ROOTS; slot++)
    {
        u32 bound = ndsRendererEntryEffectPacketBound(slot);

        if (used + bound > words)
        {
            break;
        }
        sNdsEntryEffectPackets[slot].words = &arena[used];
        sNdsEntryEffectPackets[slot].capacity = bound;
        used += bound;
    }
}

static NDSEntryEffectPacket *ndsRendererEntryEffectPacketFor(u32 slot)
{
    NDSEntryEffectPacket *packet;

    if (sNdsEntryEffectPacketArenaGeneration != gNdsTaskmanHeapGeneration)
    {
        return NULL;
    }
    packet = &sNdsEntryEffectPackets[slot];
    return ((packet->words != NULL) && (packet->disabled == 0u)) ?
        packet : NULL;
}

static u32 ndsRendererEntryEffectPacketCurrent(
    const NDSEntryEffectPacket *packet, const NDSEntryEffectRoot *root,
    u32 first, const u32 *ramp_names)
{
    u32 g;

    if ((packet->valid == 0u) ||
        (packet->scene_generation != gNdsRendererSceneTextureVramResetCount) ||
        (packet->ramp_bakes != gNdsEntryRampPaletteBakes) ||
        (packet->texture_prepares != gNdsEntryEffectNativeTexturePrepareCount) ||
        (packet->texture_releases != gNdsEntryEffectStartupTextureReleaseCount))
    {
        return 0u;
    }
    for (g = 0u; g < root->group_count; g++)
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[(u32)root->first_group + g];
        u32 name = (group->texture_slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) ?
            sNdsRendererEntryEffectTextureName[group->texture_slot] : 0u;

        if ((sNdsEntryEffectPacketNames[first + g] != name) ||
            (sNdsEntryEffectPacketRamps[first + g] != ramp_names[g]))
        {
            return 0u;
        }
    }
    return 1u;
}

static void ndsEntryPacketCounters(const NDSRendererStats *stats, u32 *out)
{
    out[0] = stats->triangle_count;
    out[1] = stats->transformed_triangle_count;
    out[2] = stats->hardware_triangle_count;
    out[3] = stats->hardware_vertex_count;
    out[4] = stats->hardware_zbuffer_triangle_count;
    out[5] = stats->hardware_texture_ready_count;
    out[6] = stats->hardware_texture_bind_count;
    out[7] = gNdsEntryEffectFastCornerGroups;
    out[8] = gNdsEntryEffectHwLitGroups;
    out[9] = gNdsEntryEffectCpuLitGroups;
    out[10] = gNdsEntryEffectNativeAlphaSkipCount;
    out[11] = gNdsEntryEffectNativeTextureBindCount;
    out[12] = gNdsEntryRampPaletteDraws;
}

/* One packed command: a header word carries four opcodes and their parameters
 * follow in order. Every command recorded here takes a parameter, so no header
 * needs the zero-parameter dummy word (ndsFtrLeanPack). */
static void ndsEntryPacketCmd(NDSEntryEffectPacketRecorder *rec, u32 opcode,
                              u32 param_count, u32 p0, u32 p1)
{
    NDSEntryEffectPacket *packet = rec->packet;

    if (packet->count + param_count + 1u > packet->capacity)
    {
        rec->fault = 1u;
        return;
    }
    if (rec->cmd_slot >= 4u)
    {
        rec->cmd_word = packet->count++;
        packet->words[rec->cmd_word] = 0u;
        rec->cmd_slot = 0u;
    }
    packet->words[rec->cmd_word] |= opcode << (rec->cmd_slot * 8u);
    rec->cmd_slot++;
    packet->words[packet->count++] = p0;
    if (param_count > 1u)
    {
        packet->words[packet->count++] = p1;
    }
}

/* The CPU's lit-matrix load goes here: the words so far end on a whole
 * command (a header's unused slots are NOPs), and the next opens a header. */
static void ndsEntryPacketSplit(NDSEntryEffectPacketRecorder *rec)
{
    rec->cmd_slot = 4u;
    rec->packet->split = rec->packet->count;
    rec->packet->lit = 1u;
}

static void ndsRendererEntryEffectPacketBegin(
    NDSEntryEffectPacketRecorder *rec, NDSEntryEffectPacket *packet,
    const NDSRendererStats *stats)
{
    rec->packet = packet;
    rec->cmd_word = 0u;
    rec->cmd_slot = 4u;
    rec->fault = 0u;
    packet->valid = 0u;
    packet->count = 0u;
    packet->split = 0u;
    packet->lit = 0u;
    packet->batches = 0u;
    packet->alpha_valid = 0u;
    ndsEntryPacketCounters(stats, rec->before);
}

/* After the group's batch began: its texture state, POLYGON_ATTR, BEGIN, the
 * unlit COLOR and ndsRendererEntryEffectFastCornerLoop's words, each packed
 * from the expression libnds's own inline writes. */
static void ndsEntryPacketRecordGroup(NDSEntryEffectPacketRecorder *rec,
                                      const NDSEntryEffectGroup *group,
                                      const NDSEntryEffectGroupState *state,
                                      u32 textured, u32 texture_name,
                                      u32 hw_lit)
{
    NDSEntryEffectPacket *packet = rec->packet;
    const u32 shift = 12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT;
    u32 expected = (textured != 0u) ? texture_name :
        (u32)sNdsRendererHardwareNoTextureName;
    u32 first = (u32)group->first_vertex;
    u32 end = first + (u32)group->triangle_count * 3u;
    int palette = -1;
    u32 i;

    if ((expected == 0u) || ((u32)glGlobalData.activeTexture != expected))
    {
        rec->fault = 1u;
        return;
    }
    ndsEntryPacketCmd(rec, FIFO_TEX_FORMAT, 1u, glGetTexParameter(), 0u);
    glGetColorTableParameterEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT,
                                &palette);
    ndsEntryPacketCmd(rec, FIFO_PAL_FORMAT, 1u,
                      (palette >= 0) ? (u32)palette : 0u, 0u);
    ndsEntryPacketCmd(rec, FIFO_POLY_FORMAT, 1u, state->poly_fmt, 0u);
    ndsEntryPacketCmd(rec, FIFO_BEGIN, 1u, (u32)GL_TRIANGLE, 0u);
    if (hw_lit == 0u)
    {
        ndsEntryPacketCmd(rec, FIFO_COLOR, 1u, 0x7fffu, 0u);
    }
    for (i = first; i < end; i++)
    {
        const NDSEntryEffectPosition *position =
            &sNdsEntryEffectPositions[sNdsEntryEffectCornerPosition[i]];
        v16 x = (v16)((s32)position->x << shift);
        v16 y = (v16)((s32)position->y << shift);
        v16 z = (v16)((s32)position->z << shift);

        if (hw_lit != 0u)
        {
            ndsEntryPacketCmd(rec, FIFO_NORMAL, 1u,
                sNdsEntryEffectNormalWords[sNdsEntryEffectCornerColor[i]], 0u);
        }
        if (textured != 0u)
        {
            t16 u = (t16)sNdsEntryEffectS[sNdsEntryEffectCornerS[i]];
            t16 v = (t16)sNdsEntryEffectT[sNdsEntryEffectCornerT[i]];

            ndsEntryPacketCmd(rec, FIFO_TEX_COORD, 1u,
                              (u32)TEXTURE_PACK(u, v), 0u);
        }
        ndsEntryPacketCmd(rec, FIFO_VERTEX16, 2u,
                          (u32)((y << 16) | (x & 0xFFFF)), (u32)(s32)z);
    }
    packet->batches++;
    packet->last_othermode_l = state->othermode_l;
    if ((state->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
        NDS_RENDERER_ALPHA_COMPARE_THRESHOLD)
    {
        packet->alpha_valid = 1u;
        packet->alpha_othermode_l = state->othermode_l;
    }
}

static void ndsRendererEntryEffectPacketEnd(
    NDSEntryEffectPacketRecorder *rec, const NDSEntryEffectRoot *root,
    u32 first, const u32 *ramp_names, const NDSRendererStats *stats)
{
    NDSEntryEffectPacket *packet = rec->packet;
    u32 after[NDS_ENTRY_PACKET_COUNTERS];
    u32 g;

    if (rec->fault != 0u)
    {
        /* Nothing a fault depends on changes within the scene. */
        packet->disabled = 1u;
        NDS_DIAG(gNdsEntryEffectPacketFaults++);
        return;
    }
    ndsEntryPacketCounters(stats, after);
    for (g = 0u; g < NDS_ENTRY_PACKET_COUNTERS; g++)
    {
        packet->delta[g] = after[g] - rec->before[g];
    }
    for (g = 0u; g < root->group_count; g++)
    {
        const NDSEntryEffectGroup *group =
            &sNdsEntryEffectGroups[(u32)root->first_group + g];

        sNdsEntryEffectPacketNames[first + g] =
            (group->texture_slot != NDS_ENTRY_EFFECT_TEXTURE_NONE) ?
            sNdsRendererEntryEffectTextureName[group->texture_slot] : 0u;
        sNdsEntryEffectPacketRamps[first + g] = ramp_names[g];
    }
    packet->scene_generation = gNdsRendererSceneTextureVramResetCount;
    packet->ramp_bakes = gNdsEntryRampPaletteBakes;
    packet->texture_prepares = gNdsEntryEffectNativeTexturePrepareCount;
    packet->texture_releases = gNdsEntryEffectStartupTextureReleaseCount;
    DC_FlushRange(packet->words, packet->count * 4u);
    packet->valid = 1u;
    NDS_DIAG(gNdsEntryEffectPacketRecords++);
}

static void ndsEntryPacketDmaStart(const u32 *words, u32 count)
{
    DMA_SRC(0) = (u32)(uintptr_t)words;
    DMA_DEST(0) = (u32)(uintptr_t)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | count;
}

static void ndsRendererForgetLibndsTexture(void);

/* The recorded loop, sent. `direction` is this root's light direction,
 * prepared here (while the first segment drains) unless `direction_ready`. */
static void __attribute__((noinline)) ndsRendererEntryEffectPacketReplay(
    const NDSEntryEffectPacket *packet, NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *direction, u32 direction_ready)
{
    const u32 *delta = packet->delta;

    ndsRendererHardwareEndBatch();
    NDS_GX_DMA0_WAIT(4u);
    if (packet->lit == 0u)
    {
        if (packet->count != 0u)
        {
            ndsEntryPacketDmaStart(packet->words, packet->count);
        }
    }
    else
    {
        if (packet->split != 0u)
        {
            ndsEntryPacketDmaStart(packet->words, packet->split);
        }
        if (direction_ready == 0u)
        {
            ndsRendererHardwarePrepareLitDirection(stats, modelview, direction);
        }
        NDS_GX_DMA0_WAIT(5u);
        ndsRendererEntryEffectLoadLitMatrices(modelview, direction);
        if (packet->count > packet->split)
        {
            ndsEntryPacketDmaStart(packet->words + packet->split,
                                   packet->count - packet->split);
        }
    }
    /* Not waited here: the next FIFO writer waits (P2-2p3). */
    sNdsFighterPacketDmaPending = 1u;
    /* The words moved the engine's texture and polygon state behind every
     * tracker's back. */
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareBoundTextureName = 0u;
    ndsRendererForgetLibndsTexture();
    sNdsRendererHardwareActiveTextureEntry = NULL;
    if (packet->batches != 0u)
    {
        if (packet->alpha_valid != 0u)
        {
            stats->othermode_l = packet->alpha_othermode_l;
            ndsRendererHardwareApplyAlphaTest(stats);
        }
        stats->othermode_l = packet->last_othermode_l;
        glEnable(GL_TEXTURE_2D);
        ndsRendererHardwareApplyFog(stats);
        glDisable(GL_ALPHA_TEST);
        sNdsRendererHardwareSubmitted = TRUE;
    }
    stats->triangle_count += delta[0];
    stats->transformed_triangle_count += delta[1];
    stats->hardware_triangle_count += delta[2];
    stats->hardware_vertex_count += delta[3];
    stats->hardware_zbuffer_triangle_count += delta[4];
    stats->hardware_texture_ready_count += delta[5];
    stats->hardware_texture_bind_count += delta[6];
    gNdsEntryEffectFastCornerGroups += delta[7];
    gNdsEntryEffectHwLitGroups += delta[8];
    gNdsEntryEffectCpuLitGroups += delta[9];
    gNdsEntryEffectNativeAlphaSkipCount += delta[10];
    gNdsEntryEffectNativeTextureBindCount += delta[11];
    gNdsEntryRampPaletteDraws += delta[12];
    NDS_DIAG(gNdsEntryEffectPacketReplays++);
}
