/* BattleShip EFCommonEffects1 DamageFlyMDust native quad owner.
 * TEXTUAL INCLUDE ONLY, after nds_native_textured_quad.exec.inc.
 *
 * This replaces execution of the one source DObjDLLink root.  BattleShip's
 * MObj animation and effect lifetime still run normally. Seven integer TEXID
 * steps select resident source frames. Four disjoint A5I3 intensity bands keep
 * full native RGB5/alpha5 precision; there is no binary-alpha substitute. */
#if NDS_RENDERER_HW_TRIANGLES
#include <nds/nds_damage_fly_mdust_texture.h>
extern volatile u32 gNdsTaskmanHeapGeneration;

volatile u32 gNdsDamageFlyMDustNativeSubmitCount;
volatile u32 gNdsDamageFlyMDustNativeRejectCount;
volatile u32 gNdsDamageFlyMDustNativeAlphaZeroCount;
volatile u32 gNdsDamageFlyMDustNativeTriangleCount;
volatile u32 gNdsDamageFlyMDustNativeCurrentFrameMask;
volatile u32 gNdsDamageFlyMDustNativeFirstFrame;
volatile u32 gNdsDamageFlyMDustNativeLastFrame;
volatile u32 gNdsDamageFlyMDustTexturePrepareCount;
volatile u32 gNdsDamageFlyMDustTexturePrepareFailCount;
volatile u32 gNdsDamageFlyMDustTextureBytes;
static u32 sNdsDamageFlyMDustTextureNames[7][4];
static u32 sNdsDamageFlyMDustTextureGeneration;

typedef struct NDSDamageFlyMDustFill
{
    const u8 *image;
    u32 band;
} NDSDamageFlyMDustFill;

static s32 ndsDamageFlyMDustFill(u8 *pixels, u32 bytes, void *user)
{
    const NDSDamageFlyMDustFill *fill = user;
    u32 i;

    if (bytes != 32u * 32u) { return FALSE; }
    for (i = 0u; i < bytes; i++)
    {
        pixels[i] = ndsDamageFlyMDustLayerTexel(fill->image, i, 3u, fill->band);
    }
    return TRUE;
}

s32 ndsRendererHardwarePrepareDamageFlyMDustTextures(void)
{
    const void *asset_view = NULL;
    u32 asset_bytes = 0u;
    u32 frame;

    if (ndsRelocGetLoadedAssetView(NDS_NATIVE_DAMAGE_FLY_MDUST_ASSET,
                                  &asset_view, &asset_bytes) == FALSE)
    {
        gNdsDamageFlyMDustTexturePrepareFailCount++;
        return FALSE;
    }
    if (sNdsDamageFlyMDustTextureGeneration != gNdsTaskmanHeapGeneration)
    {
        memset(sNdsDamageFlyMDustTextureNames, 0,
               sizeof(sNdsDamageFlyMDustTextureNames));
        sNdsDamageFlyMDustTextureGeneration = gNdsTaskmanHeapGeneration;
        gNdsDamageFlyMDustTextureBytes = 0u;
    }
    for (frame = 0u; frame < 7u; frame++)
    {
        u32 offset = sNdsDamageFlyMDustFrameOffsets[frame];
        u32 band;
        if ((offset > asset_bytes) || (2048u > asset_bytes - offset))
        {
            gNdsDamageFlyMDustTexturePrepareFailCount++;
            return FALSE;
        }
        for (band = 0u; band < 4u; band++)
        {
            u16 palette[8];
            u32 index;
            NDSDamageFlyMDustFill fill;
            if (((sNdsDamageFlyMDustBandMasks[frame] & (1u << band)) == 0u) ||
                (sNdsDamageFlyMDustTextureNames[frame][band] != 0u)) { continue; }
            for (index = 0u; index < 8u; index++)
            {
                u32 intensity = band * 8u + index;
                palette[index] = (u16)(intensity | (intensity << 5) |
                                       (intensity << 10));
            }
            fill.image = (const u8 *)asset_view + offset;
            fill.band = band;
            if (ndsRendererHardwarePrepareIFCommonCloudAtlas(
                    32u, 32u, palette, ndsDamageFlyMDustFill, &fill,
                    &sNdsDamageFlyMDustTextureNames[frame][band]) == FALSE)
            {
                gNdsDamageFlyMDustTexturePrepareFailCount++;
                return FALSE;
            }
            gNdsDamageFlyMDustTexturePrepareCount++;
            gNdsDamageFlyMDustTextureBytes += 1024u;
        }
    }
    return TRUE;
}

static s32 ndsDamageFlyMDustFrameIndex(
    const u8 *asset_base, u32 asset_bytes, u32 image, u32 *index_out)
{
    uintptr_t base;
    uintptr_t address;
    u32 i;

    if ((asset_base == NULL) || (image == 0u) || (index_out == NULL))
    {
        return FALSE;
    }
    base = (uintptr_t)asset_base;
    address = (uintptr_t)image;
    if ((address < base) ||
        ((size_t)(address - base) >= (size_t)asset_bytes))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_DAMAGE_FLY_MDUST_FRAME_COUNT; i++)
    {
        if ((u32)(address - base) == sNdsDamageFlyMDustFrameOffsets[i])
        {
            *index_out = i;
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsDamageFlyMDustGeometryMode(
    NDSRendererStats *stats, u32 command_index)
{
    const u32 *words = &sNdsDamageFlyMDustRootWords[command_index * 2u];

    stats->geometry_mode = (stats->geometry_mode & words[0]) | words[1];
    stats->geometry_clear_mask = words[0];
    stats->geometry_set_mask = words[1];
    stats->geometry_command_count++;
}

static void ndsDamageFlyMDustRootSetupBeforeMaterial(
    NDSRendererStats *stats)
{
    const u32 *words;

    /* Source E7, D9, FC. The material DE branch is applied by the typed
     * NativeApplyMaterial call between this setup and the remaining commands. */
    ndsRendererHardwareEndBatch();
    stats->command_count++;
    stats->sync_command_count++;

    ndsDamageFlyMDustGeometryMode(stats, 1u);
    stats->command_count++;

    words = &sNdsDamageFlyMDustRootWords[2u * 2u];
    ndsRendererRecordSetCombine(stats, words[0], words[1]);
    stats->command_count++;
}

static void ndsDamageFlyMDustRootSetupAfterMaterial(
    NDSRendererStats *stats)
{
    const u32 *words;

    /* Source F5, E6, F3, E7, F5 after the live current-image material. */
    words = &sNdsDamageFlyMDustRootWords[4u * 2u];
    ndsRendererRecordSetTile(stats, words[0], words[1]);
    stats->command_count++;

    ndsRendererHardwareEndBatch();
    stats->command_count++;
    stats->sync_command_count++;

    words = &sNdsDamageFlyMDustRootWords[6u * 2u];
    ndsRendererRecordLoadBlock(stats, words[0], words[1]);
    stats->command_count++;

    ndsRendererHardwareEndBatch();
    stats->command_count++;
    stats->sync_command_count++;

    words = &sNdsDamageFlyMDustRootWords[8u * 2u];
    ndsRendererRecordSetTile(stats, words[0], words[1]);
    stats->command_count++;
}

static void ndsDamageFlyMDustRootFinish(NDSRendererStats *stats)
{
    /* Source E7, D9, DF. The textured-quad kernel closes its batch before
     * returning; keep the source pipe boundary here for alpha-zero frames too. */
    ndsRendererHardwareEndBatch();
    stats->command_count++;
    stats->sync_command_count++;

    ndsDamageFlyMDustGeometryMode(stats, 12u);
    stats->command_count++;

    stats->command_count++;
    stats->end_command_count++;
}

sb32 ndsRendererPreflightNativeDamageFlyMDust(
    const void *asset_base_ptr, u32 asset_bytes, u32 root_offset,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config)
{
    const u8 *asset_base = (const u8 *)asset_base_ptr;
    u32 current_frame;
    u32 band;

    if ((asset_base == NULL) || (material == NULL) || (config == NULL) ||
        (config->initial_projection == NULL) ||
        (config->initial_modelview == NULL) ||
        (config->texture_data_layout !=
         NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ||
        (root_offset != NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT) ||
        (root_offset > asset_bytes) ||
        (NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT_BYTES >
         asset_bytes - root_offset) ||
        ((u32)NDS_NATIVE_DAMAGE_FLY_MDUST_MOBJ_SUB +
         NDS_NATIVE_DAMAGE_FLY_MDUST_MOBJ_SUB_BYTES > asset_bytes) ||
        (material->effects != NDS_NATIVE_DAMAGE_FLY_MDUST_MATERIAL_EFFECTS))
    {
        return FALSE;
    }
    if ((ndsDamageFlyMDustFrameIndex(
             asset_base, asset_bytes, material->current_image,
             &current_frame) == FALSE) ||
        (sNdsDamageFlyMDustTextureGeneration != gNdsTaskmanHeapGeneration))
    {
        return FALSE;
    }
    for (band = 0u; band < 4u; band++)
    {
        if (((sNdsDamageFlyMDustBandMasks[current_frame] & (1u << band)) != 0u) &&
            (sNdsDamageFlyMDustTextureNames[current_frame][band] == 0u))
        {
            return FALSE;
        }
    }
    return TRUE;
}

sb32 ndsRendererSubmitNativeDamageFlyMDust(
    const void *asset_base_ptr, u32 asset_bytes, u32 root_offset,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *asset_base = (const u8 *)asset_base_ptr;
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_DAMAGE_FLY_MDUST_VERTEX_COUNT];
    u32 current_frame;
    u32 band;
    u32 triangles_before;
    s32 result;
    u32 i;

    if ((stats == NULL) ||
        (ndsRendererPreflightNativeDamageFlyMDust(
             asset_base, asset_bytes, root_offset, material,
             config) == FALSE))
    {
        gNdsDamageFlyMDustNativeRejectCount++;
        return FALSE;
    }
    if (ndsDamageFlyMDustFrameIndex(
            asset_base, asset_bytes, material->current_image, &current_frame) == FALSE)
    {
        gNdsDamageFlyMDustNativeRejectCount++;
        return FALSE;
    }
    gNdsDamageFlyMDustNativeCurrentFrameMask |= 1u << current_frame;

    for (i = 0u; i < NDS_NATIVE_DAMAGE_FLY_MDUST_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsDamageFlyMDustVertices[i * 5u];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4], 0xffu, 0xffu, 0xffu, 0xffu
        };
    }
    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsDamageFlyMDustRootSetupBeforeMaterial(stats);
    ndsRendererNativeApplyMaterial(material, stats, &state);
    ndsDamageFlyMDustRootSetupAfterMaterial(stats);

    /* Source VTX and TRI2 command accounting; the shared quad kernel emits the
     * exact two source triangles and owns all six hardware vertex writes. */
    stats->vertex_command_count++;
    stats->command_count++;
    stats->triangle_command_count++;
    stats->command_count++;
    triangles_before = stats->hardware_triangle_count;
    result = 0;
    for (band = 0u; band < 4u; band++)
    {
        if ((sNdsDamageFlyMDustBandMasks[current_frame] & (1u << band)) == 0u)
        {
            continue;
        }
        result = ndsRendererNativeTexturedQuadGraded(
            vertices, sNdsDamageFlyMDustTriIndices, NULL, NULL,
            config, stats, &state, sNdsDamageFlyMDustTextureNames[current_frame][band]);
        if (result < 0) { break; }
    }
    ndsDamageFlyMDustRootFinish(stats);
    if (result < 0)
    {
        gNdsDamageFlyMDustNativeRejectCount++;
        return FALSE;
    }

    gNdsDamageFlyMDustNativeSubmitCount++;
    if (gNdsDamageFlyMDustNativeFirstFrame == 0u)
    {
        gNdsDamageFlyMDustNativeFirstFrame = sNdsRendererHardwareFrameSerial;
    }
    gNdsDamageFlyMDustNativeLastFrame = sNdsRendererHardwareFrameSerial;
    if (result == 0)
    {
        gNdsDamageFlyMDustNativeAlphaZeroCount++;
    }
    else
    {
        gNdsDamageFlyMDustNativeTriangleCount +=
            stats->hardware_triangle_count - triangles_before;
    }
    return TRUE;
}

#endif /* NDS_RENDERER_HW_TRIANGLES */
