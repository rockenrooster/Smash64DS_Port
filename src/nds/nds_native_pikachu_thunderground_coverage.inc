/* Pikachu ground Thunder Jolt soft-coverage texture owner. TEXTUAL INCLUDE
 * ONLY -- it reads the generated palette/LUTs above it and the shared texture
 * statics in nds_renderer_textures_effects.c, both of which live in the single
 * nds_renderer.c translation unit.
 *
 * The contract, the format choice and the measured error that picked it are in
 * include/nds/nds_native_pikachu_thunderground_coverage.h.  What follows is
 * only how it is cached and uploaded. */
#include <nds/nds_native_pikachu_thunderground_coverage.h>

#if NDS_RENDERER_HW_TRIANGLES

volatile u32 gNdsThunderGroundCoveragePrepareCount;
volatile u32 gNdsThunderGroundCoverageBindCount;
volatile u32 gNdsThunderGroundCoverageHitCount;
volatile u32 gNdsThunderGroundCoverageFallbackCount;
volatile u32 gNdsThunderGroundCoverageRejectStep;
/* Which slots have carried a converted image this scene. There are exactly as
 * many slots as live images and each takes its own, so 0x7 means all three
 * were reached -- a soft edge on one image proves nothing about the other two,
 * so acceptance reads this beside the draw count rather than inferring
 * coverage from it. */
volatile u32 gNdsThunderGroundCoverageImageMask;
volatile u32 gNdsThunderGroundCoverageVramBytes;

typedef struct NDSNativeThunderGroundCoverageSlot
{
    u32 name;   /* GL texture name; 0 when the slot is empty */
    u32 image;  /* relocated source address this name was converted from */
    u32 key;    /* everything else the conversion depends on */
} NDSNativeThunderGroundCoverageSlot;

static NDSNativeThunderGroundCoverageSlot
    sNdsNativeThunderGroundCoverage[NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS];
/* glResetTextures invalidates every name without telling anyone who holds one.
 * ndsRendererHardwareResetSceneTextureVram bumps this counter immediately
 * afterwards, so a disagreement here means the names below are gone: forget
 * them WITHOUT deleting, because the reset already reclaimed their blocks. */
static u32 sNdsNativeThunderGroundCoverageReset;
static u32 sNdsNativeThunderGroundCoverageNext;

typedef struct NDSNativeThunderGroundCoverageFill
{
    const NDSRendererConfig *config;
    const u8 *texels;
} NDSNativeThunderGroundCoverageFill;

static s32 ndsNativeThunderGroundCoverageFill(u8 *pixels, u32 bytes,
                                              void *user_data)
{
    const NDSNativeThunderGroundCoverageFill *fill =
        (const NDSNativeThunderGroundCoverageFill *)user_data;
    u32 index;

    if ((pixels == NULL) || (fill == NULL) || (fill->texels == NULL) ||
        (bytes < NDS_NATIVE_THUNDERGROUND_COVERAGE_UPLOAD_BYTES))
    {
        return FALSE;
    }
    /* The upload extent IS the tile extent -- 32x32, both axes already a power
     * of two -- so there is no clamp padding to replicate and no tail to clear:
     * every byte below is written from a source texel. */
    for (index = 0u;
         index < NDS_NATIVE_THUNDERGROUND_COVERAGE_UPLOAD_BYTES; index++)
    {
        /* The O2R payload is word-swapped in place at load, so a raw [] index
         * would scramble the texels within each word. Read through the shared
         * lane reader, which owns that layout. */
        u32 value = ndsRendererReadTextureByte(
            fill->config, fill->texels, index,
            NDS_RENDERER_HW_TEXTURE_FMT_IA, NDS_RENDERER_HW_TEXTURE_SIZ_8B);

        /* GL_RGB8_A5: alpha5 in bits 3-7, palette index in bits 0-2. The alpha
         * nibble drives the alpha and the intensity nibble drives the index --
         * independently, which is exactly what the I-format graded arm cannot
         * express. */
        pixels[index] = (u8)(
            (sNdsNativeThunderGroundCoverageAlpha5[value & 0x0fu] << 3) |
            sNdsNativeThunderGroundCoverageIndex3[(value >> 4) & 0x0fu]);
    }
    return TRUE;
}

/* Everything the converted texels depend on EXCEPT the image address, which is
 * the slot's own tag. The coverage class is in here so a rebuild that changes
 * only the encoding cannot be served a hard-alpha name left over from before,
 * and the taskman heap generation is in here so a rewind that reuses an address
 * for a different image cannot either. */
static u32 ndsNativeThunderGroundCoverageKey(
    const NDSRendererStats *stats, const NDSRendererTileState *tile)
{
    u32 key = NDS_NATIVE_THUNDERGROUND_COVERAGE_CLASS;

    key = (key * 16777619u) ^ (stats->prim_color & 0xffffff00u);
    key = (key * 16777619u) ^ (stats->env_color & 0xffffff00u);
    key = (key * 16777619u) ^
        ((tile->format << 24) | (tile->size << 20) |
         (tile->cms << 18) | (tile->cmt << 16) |
         (tile->masks << 12) | (tile->maskt << 8));
    key = (key * 16777619u) ^
        ((NDS_NATIVE_THUNDERGROUND_IMAGE_WIDTH << 16) |
         NDS_NATIVE_THUNDERGROUND_IMAGE_HEIGHT);
    key = (key * 16777619u) ^ gNdsTaskmanHeapGeneration;
    return key;
}

static void ndsNativeThunderGroundCoverageForgetAfterReset(void)
{
    u32 slot;

    if (sNdsNativeThunderGroundCoverageReset ==
        gNdsRendererSceneTextureVramResetCount)
    {
        return;
    }
    for (slot = 0u; slot < NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS; slot++)
    {
        sNdsNativeThunderGroundCoverage[slot].name = 0u;
        sNdsNativeThunderGroundCoverage[slot].image = 0u;
        sNdsNativeThunderGroundCoverage[slot].key = 0u;
    }
    sNdsNativeThunderGroundCoverageNext = 0u;
    sNdsNativeThunderGroundCoverageReset =
        gNdsRendererSceneTextureVramResetCount;
    gNdsThunderGroundCoverageImageMask = 0u;
    gNdsThunderGroundCoverageVramBytes = 0u;
}

/* Released explicitly at scene teardown once the integrator wires this into
 * ndsRendererHardwareDiscardTextureCache; harmless and idempotent otherwise,
 * because the reset guard above already refuses a stale name. External linkage
 * deliberately: that caller sits earlier in the same translation unit and
 * reaches this through a forward declaration, and a static definition nothing
 * had called yet would warn. */
void ndsNativeThunderGroundReleaseCoverageTextures(void)
{
    u32 slot;

    for (slot = 0u; slot < NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS; slot++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsNativeThunderGroundCoverage[slot].name);
        sNdsNativeThunderGroundCoverage[slot].name = 0u;
        sNdsNativeThunderGroundCoverage[slot].image = 0u;
        sNdsNativeThunderGroundCoverage[slot].key = 0u;
    }
    sNdsNativeThunderGroundCoverageNext = 0u;
    sNdsNativeThunderGroundCoverageReset =
        gNdsRendererSceneTextureVramResetCount;
    gNdsThunderGroundCoverageImageMask = 0u;
    gNdsThunderGroundCoverageVramBytes = 0u;
}

/* TRUE when the segment now has a soft-alpha texture bound. FALSE means the
 * caller must use the generic bind, which is exactly what it used before. */
static s32 ndsNativeThunderGroundBindCoverageTexture(
    NDSRendererStats *stats, const NDSRendererConfig *config,
    const NDSRendererNativeMaterial *material)
{
    const NDSRendererTileState *tile;
    NDSNativeThunderGroundCoverageFill fill;
    NDSNativeThunderGroundCoverageSlot *slot;
    const void *texels;
    u32 image;
    u32 key;
    u32 index;

    if ((stats == NULL) || (config == NULL) || (material == NULL))
    {
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_ARGS;
        return FALSE;
    }
    /* THE LIVE SELECTOR, TAKEN FROM THE MATERIAL RATHER THAN FROM stats.
     * The baked setup records this root's LOADBLOCK before the material records
     * its G_SETTIMG, so the per-TMEM load record snapshots whatever image was
     * current BEFORE this segment ran. material->current_image is
     * mobj->sub.sprites[texture_id_curr] itself and is never a segment late. */
    image = material->current_image;
    if (image == 0u)
    {
        /* One (child, texture_id) pair in the source genuinely selects a NULL
         * sprite -- MObjSub 2 slot 1, pinned by the generator. Decline it here
         * rather than convert address zero; the generic bind then applies its
         * own missing-state policy, unchanged. */
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_NO_IMAGE;
        return FALSE;
    }
    if ((((material->current_image_w0 >> 21) & 0x7u) !=
             NDS_NATIVE_THUNDERGROUND_TILE_FMT) ||
        (((material->current_image_w0 >> 19) & 0x3u) !=
             NDS_RENDERER_HW_TEXTURE_SIZ_16B))
    {
        /* G_SETTIMG addresses this LOADBLOCK at the load tile's 16-bit stride;
         * the render tile below is what says IA8. Both are pinned, so a change
         * in either means this specialization no longer describes the source. */
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_IMAGE_WORD;
        return FALSE;
    }

    ndsRendererSyncTextureTile(stats);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    if ((tile->set_seen == 0u) ||
        (tile->format != NDS_NATIVE_THUNDERGROUND_TILE_FMT) ||
        (tile->size != NDS_NATIVE_THUNDERGROUND_TILE_SIZ) ||
        (tile->masks != NDS_NATIVE_THUNDERGROUND_TILE_MASK) ||
        (tile->maskt != NDS_NATIVE_THUNDERGROUND_TILE_MASK) ||
        (tile->cms != NDS_NATIVE_THUNDERGROUND_TILE_CLAMP) ||
        (tile->cmt != NDS_NATIVE_THUNDERGROUND_TILE_CLAMP) ||
        (tile->width != NDS_NATIVE_THUNDERGROUND_IMAGE_WIDTH) ||
        (tile->height != NDS_NATIVE_THUNDERGROUND_IMAGE_HEIGHT))
    {
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_TILE;
        return FALSE;
    }
    /* The palette below bakes the endpoint lerp, so the combine that reads it
     * and the two endpoints themselves are part of the contract, not context. */
    if (ndsRendererHardwarePrimEnvTexel0BlendMode(stats) !=
        NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA)
    {
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_COMBINE;
        return FALSE;
    }
    if ((stats->prim_color != NDS_NATIVE_THUNDERGROUND_PRIM) ||
        (stats->env_color != NDS_NATIVE_THUNDERGROUND_ENV))
    {
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_ENDPOINTS;
        return FALSE;
    }

    ndsNativeThunderGroundCoverageForgetAfterReset();
    key = ndsNativeThunderGroundCoverageKey(stats, tile);
    for (index = 0u; index < NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS; index++)
    {
        slot = &sNdsNativeThunderGroundCoverage[index];
        if ((slot->name != 0u) && (slot->image == image) && (slot->key == key))
        {
            ndsRendererHardwareBindTextureName(stats, slot->name);
            ndsRendererHardwareApplyTextureParams(
                ndsRendererHardwareMergeTextureParams((u32)TEXGEN_TEXCOORD));
            sNdsRendererHardwareActiveTextureEntry = NULL;
            stats->hardware_texture_ready_count++;
            stats->hardware_texture_format = NDS_NATIVE_THUNDERGROUND_TILE_FMT;
            stats->hardware_texture_width =
                NDS_NATIVE_THUNDERGROUND_IMAGE_WIDTH;
            stats->hardware_texture_height =
                NDS_NATIVE_THUNDERGROUND_IMAGE_HEIGHT;
            gNdsThunderGroundCoverageHitCount++;
            gNdsThunderGroundCoverageBindCount++;
            gNdsThunderGroundCoverageImageMask |= 1u << index;
            return TRUE;
        }
    }

    texels = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)image,
        NDS_NATIVE_THUNDERGROUND_IMAGE_BYTES);
    if (texels == NULL)
    {
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_SOURCE_PTR;
        return FALSE;
    }
    /* Prefer an empty slot: there are exactly as many slots as live images, so
     * once all three are converted every later request is a hit and no name is
     * ever deleted to make room for one of its own siblings. Round-robin only
     * decides which one to displace after a key change invalidated them all. */
    slot = NULL;
    for (index = 0u; index < NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS; index++)
    {
        if (sNdsNativeThunderGroundCoverage[index].name == 0u)
        {
            slot = &sNdsNativeThunderGroundCoverage[index];
            break;
        }
    }
    if (slot == NULL)
    {
        index = sNdsNativeThunderGroundCoverageNext %
            NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS;
        sNdsNativeThunderGroundCoverageNext = index + 1u;
        slot = &sNdsNativeThunderGroundCoverage[index];
    }
    fill.config = config;
    fill.texels = (const u8 *)texels;
    /* Prepare releases the slot's previous name before allocating, binds the
     * new one and attaches the palette to it, so a hit above needs no palette
     * work at all. */
    if (ndsRendererHardwarePrepareIFCommonCloudAtlas(
            NDS_NATIVE_THUNDERGROUND_IMAGE_WIDTH,
            NDS_NATIVE_THUNDERGROUND_IMAGE_HEIGHT,
            sNdsNativeThunderGroundCoveragePalette,
            ndsNativeThunderGroundCoverageFill, &fill, &slot->name) == FALSE)
    {
        slot->name = 0u;
        slot->image = 0u;
        slot->key = 0u;
        gNdsThunderGroundCoverageRejectStep =
            NDS_THUNDERGROUND_COVERAGE_REJECT_UPLOAD;
        return FALSE;
    }
    slot->image = image;
    slot->key = key;
    ndsRendererHardwareApplyTextureParams(
        ndsRendererHardwareMergeTextureParams((u32)TEXGEN_TEXCOORD));
    sNdsRendererHardwareActiveTextureEntry = NULL;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = NDS_NATIVE_THUNDERGROUND_TILE_FMT;
    stats->hardware_texture_width = NDS_NATIVE_THUNDERGROUND_IMAGE_WIDTH;
    stats->hardware_texture_height = NDS_NATIVE_THUNDERGROUND_IMAGE_HEIGHT;
    gNdsThunderGroundCoveragePrepareCount++;
    gNdsThunderGroundCoverageBindCount++;
    gNdsThunderGroundCoverageImageMask |= 1u << index;
    /* What this owner is actually holding right now, not what it may hold:
     * VRAM is the scarce resource, so report the live sum. */
    gNdsThunderGroundCoverageVramBytes = 0u;
    for (index = 0u; index < NDS_NATIVE_THUNDERGROUND_COVERAGE_SLOTS; index++)
    {
        if (sNdsNativeThunderGroundCoverage[index].name != 0u)
        {
            gNdsThunderGroundCoverageVramBytes +=
                NDS_NATIVE_THUNDERGROUND_COVERAGE_UPLOAD_BYTES;
        }
    }
    return TRUE;
}

#endif
