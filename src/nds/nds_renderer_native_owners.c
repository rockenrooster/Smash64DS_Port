/* Bumped each time the stage owner takes a frame's camera and binding
 * matrices (and when it drops them): nds_stage_gx.exec.inc memoizes values
 * derived only from those matrices for one generation. */
static u32 sNdsNativeStageMatrixGen = 1u;

#if NDS_P2_STAGE_YOSTER
#include <nds/nds_native_actor_yoster_cloud.h>
#include "generated/nds_native_actor_yoster_cloud.generated.inc"
#include "nds_native_actor_yoster_cloud.exec.inc"
#endif

#if NDS_P2_STAGE_JUNGLE
#include <nds/generated/nds_native_actor_tarucann.generated.h>
#include "generated/nds_native_actor_tarucann.generated.inc"

/* Barrel-cannon submit witnesses: the last emitted triangle's submitted v16
 * positions, resolved alpha/format, bound texture, and object-space signed
 * area x2 (winding sign; zero means degenerate). Stores only. */
volatile u32 gNdsNativeTaruCannWitnessX[3];
volatile u32 gNdsNativeTaruCannWitnessY[3];
volatile u32 gNdsNativeTaruCannWitnessZ[3];
volatile u32 gNdsNativeTaruCannWitnessAlpha;
volatile u32 gNdsNativeTaruCannWitnessPolyFmt;
volatile u32 gNdsNativeTaruCannWitnessTexName;
volatile u32 gNdsNativeTaruCannWitnessArea2;
/* Root-joint world-travel witnesses: joint_locals[0] translation row in 20.12,
 * the same local GX multiplies above via glMultMatrix4x4. Stores only. */
volatile u32 gNdsNativeTaruCannWitnessJoint0Tx;
volatile u32 gNdsNativeTaruCannWitnessJoint0Ty;
volatile u32 gNdsNativeTaruCannWitnessJoint0Tz;

typedef struct NDSNativeTaruCannAssetRange
{
    const u8 *base;
    u32 bytes;
} NDSNativeTaruCannAssetRange;

static const void *ndsNativeTaruCannResolveData(const void *ptr, size_t bytes,
                                               void *user)
{
    const NDSNativeTaruCannAssetRange *range = user;
    uintptr_t address = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)range->base;

    return ((address >= base) && (bytes <= range->bytes) &&
            (address - base <= range->bytes - bytes)) ? ptr : NULL;
}

static void ndsNativeTaruCannHardwareAffine(
    const NDSRendererMatrix20p12 *source, m4x4 *hardware)
{
    NDSRendererMatrix20p12 scaled = *source;
    u32 col;

    for (col = 0u; col < 3u; col++)
    {
        scaled.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&scaled, hardware);
}

/* grJungleMakeTaruCann owns this fixed two-joint tree. Its only drawable
 * child is a textured quad; the source explicitly disables lighting. Use
 * live source local matrices on GX and emit the generated corners directly.
 * The shared texture cache owns upload, reuse, eviction and scene reset. */
sb32 ndsRendererSubmitNativeTaruCann(const void *asset_base, u32 asset_bytes,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    u32 initial_geometry_mode, NDSRendererStats *stats)
{
    NDSNativeTaruCannAssetRange range = { asset_base, asset_bytes };
    NDSRendererConfig config = { 0 };
    NDSRendererTraversalState state;
    NDSRendererInputVertex vertices[NDS_NATIVE_ACTOR_TARUCANN_VERT_COUNT];
    const NDSRendererTileState *tile;
    u32 material_color;
    u32 use_material;
    u32 use_vertex;
    u32 poly_alpha;
    u32 generation;
    u32 i;
    s32 texture_offset;
    m4x4 hardware;

    if ((asset_base == NULL) || (stats == NULL) || (hierarchy == NULL) ||
        (hierarchy->joint_count != NDS_NATIVE_ACTOR_TARUCANN_JOINT_COUNT) ||
        (hierarchy->projection == NULL) ||
        (hierarchy->camera_modelview == NULL) ||
        (hierarchy->joint_locals == NULL) ||
        (hierarchy->joint_parents == NULL) ||
        (hierarchy->joint_bindings == NULL) ||
        (asset_bytes < sNdsNativeActorTaruCannTextureEpoch[0] +
                       sNdsNativeActorTaruCannTextureEpoch[1] * 2u) ||
        (asset_bytes < sNdsNativeActorTaruCannTextureEpoch[2] +
                       sNdsNativeActorTaruCannTextureEpoch[3]))
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_TARUCANN_JOINT_COUNT; i++)
    {
        if ((hierarchy->joint_parents[i] != sNdsNativeActorTaruCannJointParents[i]) ||
            (hierarchy->joint_bindings[i] != sNdsNativeActorTaruCannBindings[i]) ||
            (hierarchy->joint_locals[i].m[0][3] != 0) ||
            (hierarchy->joint_locals[i].m[1][3] != 0) ||
            (hierarchy->joint_locals[i].m[2][3] != 0) ||
            (hierarchy->joint_locals[i].m[3][3] != 4096))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_ACTOR_TARUCANN_VERT_COUNT; i++)
    {
        const s16 *v = &sNdsNativeActorTaruCannVerts[i * 5u];
        u32 rgba = sNdsNativeActorTaruCannVertColors[i];
        vertices[i] = (NDSRendererInputVertex){ v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba };
    }
    config.initial_projection = hierarchy->projection;
    config.initial_modelview = hierarchy->camera_modelview;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.resolve_data = ndsNativeTaruCannResolveData;
    config.user = &range;
    ndsRendererInitTraversalState(&state, &config, stats, NULL, NULL, 0u);
    ndsNativeActorTaruCannSetupState(stats, asset_base);
    if ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u)
    {
        return FALSE;
    }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareBindTexture(stats, &config, &state) == FALSE)
    {
        return FALSE;
    }
    material_color = ndsRendererHardwareColorSource(stats);
    use_material = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex = ndsRendererHardwareUseVertexColor(stats);
    poly_alpha = ndsRendererHardwareAlpha(stats, &vertices[0]);
    tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    texture_offset = ndsRendererHardwareTextureFilterOffset(stats);

    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    ndsRendererCopyMtx20p12ToM4x4(hierarchy->projection, &hardware);
    /* Vertices and affine translations below use source units / 256, with
     * affine w still 1. Scale the projection's constant row to the same unit;
     * otherwise its unscaled near-plane term clips the entire cannon quad. */
    for (i = 0u; i < 4u; i++)
    {
        hardware.m[12u + i] = ndsRendererRoundShiftS32Signed(
            hardware.m[12u + i], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    glLoadMatrix4x4(&hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    ndsNativeTaruCannHardwareAffine(hierarchy->camera_modelview, &hardware);
    glLoadMatrix4x4(&hardware);
    for (i = 0u; i < NDS_NATIVE_ACTOR_TARUCANN_JOINT_COUNT; i++)
    {
        ndsNativeTaruCannHardwareAffine(&hierarchy->joint_locals[i], &hardware);
        glMultMatrix4x4(&hardware);
    }
    gNdsNativeTaruCannWitnessJoint0Tx =
        (u32)hierarchy->joint_locals[0].m[3][0];
    gNdsNativeTaruCannWitnessJoint0Ty =
        (u32)hierarchy->joint_locals[0].m[3][1];
    gNdsNativeTaruCannWitnessJoint0Tz =
        (u32)hierarchy->joint_locals[0].m[3][2];
    generation = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = TRUE;
    ndsRendererProfileRecordMatrixLoad();
    if (poly_alpha != 0u)
    {
        const u16 *run = sNdsNativeActorTaruCannRuns;
        u32 corner;
        u32 witness_base = ((u32)run[1] + run[2]) * 3u - 3u;

        gNdsNativeTaruCannWitnessAlpha = poly_alpha;
        gNdsNativeTaruCannWitnessPolyFmt =
            ndsRendererHardwarePolyFmt(stats, poly_alpha);
        gNdsNativeTaruCannWitnessTexName =
            sNdsRendererHardwareBoundTextureName;
        ndsRendererHardwareBeginTriangleBatch(stats, TRUE,
            sNdsRendererHardwareBoundTextureName,
            ndsRendererHardwarePolyFmt(stats, poly_alpha),
            sNdsRendererHardwareMatrixMode, generation);
        for (corner = (u32)run[1] * 3u;
             corner < ((u32)run[1] + run[2]) * 3u; corner++)
        {
            const NDSRendererInputVertex *v =
                &vertices[sNdsNativeActorTaruCannTriIndices[corner]];
            v16 x = ndsRendererHardwareVertexCoord(v->x, TRUE);
            v16 y = ndsRendererHardwareVertexCoord(v->y, TRUE);
            v16 z = ndsRendererHardwareVertexCoord(v->z, TRUE);

            if (corner >= witness_base)
            {
                u32 slot = corner - witness_base;

                gNdsNativeTaruCannWitnessX[slot] = (u32)(s32)x;
                gNdsNativeTaruCannWitnessY[slot] = (u32)(s32)y;
                gNdsNativeTaruCannWitnessZ[slot] = (u32)(s32)z;
            }
            glColor(ndsRendererHardwarePackedVertexColor(stats, v,
                material_color, use_material, use_vertex, 0u, FALSE, 0u));
            glTexCoord2t16(
                ndsRendererHardwareTexCoord(v->s, stats->texture_scale_s,
                    tile->uls, texture_offset),
                ndsRendererHardwareTexCoord(v->t, stats->texture_scale_t,
                    tile->ult, texture_offset));
            glVertex3v16(x, y, z);
        }
        {
            s32 x0 = (s32)gNdsNativeTaruCannWitnessX[0];
            s32 x1 = (s32)gNdsNativeTaruCannWitnessX[1];
            s32 x2 = (s32)gNdsNativeTaruCannWitnessX[2];
            s32 y0 = (s32)gNdsNativeTaruCannWitnessY[0];
            s32 y1 = (s32)gNdsNativeTaruCannWitnessY[1];
            s32 y2 = (s32)gNdsNativeTaruCannWitnessY[2];
            s32 area2 = (s32)((s64)(x1 - x0) * (y2 - y0) -
                (s64)(x2 - x0) * (y1 - y0));

            gNdsNativeTaruCannWitnessArea2 = (u32)area2;
        }
        for (i = 0u; i < run[2]; i++)
        {
            ndsRendererProfileRecordHardwareTriangle();
        }
        stats->hardware_triangle_count += run[2];
        stats->hardware_zbuffer_triangle_count += run[2];
        stats->hardware_vertex_count += (u32)run[2] * 3u;
        sNdsRendererHardwareSubmitted = TRUE;
    }
    ndsRendererHardwareEndBatch();
    ndsNativeActorTaruCannFinishState(stats, asset_base);
    return TRUE;
}
#endif

s32 ndsRendererExecuteNativeFighterOwnerHierarchy(
    u32 slot,
    const void *asset_base_ptr,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started)
{
    const u8 *asset_base = asset_base_ptr;
    NDSNativeHierarchyTables tables;
    NDSRendererTraversalState *state =
        &sNdsNativeFighterOwnerExecution.traversal;
    u32 native_run_count = 0u;
    u32 native_triangle_count = 0u;
    u32 matrix_generation = 1u;
    u32 joint_index;

    (void)callback_user;
    if (out_hardware_started == NULL)
    {
        return FALSE;
    }
    *out_hardware_started = FALSE;
    /* Mode 7 is intentionally high-detail-only.  No shipping P2 target uses it;
     * forcing the historical view here is safer than silently feeding low
     * tables into a hierarchy schedule generated only for the high JointTree. */
    if (ndsRendererNativeSelectFighterRuntimeTables(slot, FALSE) == FALSE)
    {
        return FALSE;
    }
    if (ndsRendererNativePreflightFighterHierarchy(
            slot, asset_base, hierarchy, callback, stats, &tables) == FALSE)
    {
        return FALSE;
    }

    ndsRendererInitTraversalState(
        state, NULL, stats, NULL, NULL, 0u);
    {
        m4x4 camera_hardware;

        ndsRendererNativeBuildHierarchyHardwareAffine(
            hierarchy->camera_modelview, &camera_hardware);
        *out_hardware_started = TRUE;
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
        {
            m4x4 projection_hardware;
            ndsRendererCopyMtx20p12ToM4x4(
                hierarchy->projection, &projection_hardware);
            glLoadMatrix4x4(&projection_hardware);
        }
        ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
        glLoadMatrix4x4(&camera_hardware);
        ndsRendererProfileRecordMatrixLoad();
        sNdsRendererHardwareMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
        sNdsRendererHardwareMatrixGeneration = matrix_generation;
        sNdsRendererHardwareMatrixLoaded = TRUE;
        stats->hardware_matrix_seed_count++;
    }

    for (joint_index = 0u; joint_index < tables.joint_count; joint_index++)
    {
        u32 packed = tables.schedule[joint_index];
        u32 binding = (packed >> 5) & 31u;
        m4x4 local_hardware;

        ndsRendererHardwareEndBatch();
        ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
        if ((packed & 0x8000u) != 0u)
        {
            glPushMatrix();
            stats->matrix_push_count++;
        }
        ndsRendererNativeBuildHierarchyHardwareAffine(
            &hierarchy->joint_locals[joint_index], &local_hardware);
        glMultMatrix4x4(&local_hardware);
        matrix_generation = ndsRendererNextMatrixGeneration();
        sNdsRendererHardwareMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY;
        sNdsRendererHardwareMatrixGeneration = matrix_generation;
        sNdsRendererHardwareMatrixLoaded = TRUE;
        if (binding != 31u)
        {
            u32 palette_slot = tables.cross_slots[binding];

            if (palette_slot <= NDS_NATIVE_GX_MATRIX_SLOT_MAX)
            {
                glStoreMatrix((int)palette_slot);
            }
            ndsRendererNativeCommitHierarchyRoot(
                asset_base, &tables.roots[binding],
                &hierarchy->roots[binding],
                tables.binding_joints[binding], palette_slot,
                matrix_generation, stats, state,
                &native_run_count, &native_triangle_count);
        }
        {
            u32 next_parent = (joint_index + 1u < tables.joint_count) ?
                (tables.schedule[joint_index + 1u] & 31u) : 31u;
            u32 cursor = joint_index;

            while (cursor != next_parent)
            {
                u32 cursor_packed = tables.schedule[cursor];
                u32 parent = cursor_packed & 31u;

                if ((cursor_packed & 0x8000u) != 0u)
                {
                    ndsRendererHardwareEndBatch();
                    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
                    glPopMatrix(1);
                    stats->matrix_pop_count++;
                }
                cursor = parent;
            }
        }
    }
    ndsRendererHardwareEndBatch();
    sNdsRendererFastRunCount += native_run_count;
    sNdsRendererFastTriangleCount += native_triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += native_triangle_count;
    }
    return TRUE;
}
static s32 ndsRendererNativeStageValidateStateSpanTopology(
    const NDSNativeStageStateSpan *span)
{
    u32 i;

    if ((span == NULL) ||
        ((u32)span->first_state + span->state_count >
         NDS_NATIVE_STAGE_STATE_SEQUENCE_COUNT))
    {
        return FALSE;
    }
    for (i = 0u; i < span->state_count; i++)
    {
        u32 delta_index = sNdsNativeStageStateSequence[
            (u32)span->first_state + i];
        const NDSNativeStageStateDelta *delta;

        if (delta_index >= NDS_NATIVE_STAGE_STATE_DELTA_COUNT)
        {
            return FALSE;
        }
        delta = &sNdsNativeStageStateDeltas[delta_index];
        if (delta->effect == NDS_NATIVE_STATE_MATERIAL)
        {
            if ((delta->material_event >=
                 NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT) ||
                (delta->material_command >=
                 sNdsNativeStageMaterialEvents[
                     delta->material_event].source_command_count))
            {
                return FALSE;
            }
            continue;
        }
        if ((delta->effect == NDS_NATIVE_STATE_BLEND) ||
            (delta->effect == NDS_NATIVE_STATE_STAGE_ENVCOLOR) ||
            (delta->effect == NDS_NATIVE_STATE_STAGE_MOVEWORD) ||
            (delta->effect == NDS_NATIVE_STATE_LOAD_TILE))
        {
            /* Stage-generator ids 14/16/20 (Yoshi's Island, Sector Z,
             * Meta Crystal, Beta Dream Land); see the applier below. */
            continue;
        }
        if ((delta->effect < NDS_NATIVE_STATE_OTHERMODE) ||
            (delta->effect > NDS_NATIVE_STATE_PRIM))
        {
            return FALSE;
        }
        if ((delta->effect == NDS_NATIVE_STATE_IMAGE) &&
            ((delta->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
             (delta->w1 >= sNdsNativeStageAssets[
                 delta->asset_index].payload_size)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsRendererNativeStageGeneratedHashU32(u32 hash, u32 value)
{
    u32 shift;

    for (shift = 0u; shift < 32u; shift += 8u)
    {
        hash ^= (value >> shift) & 0xffu;
        hash *= 16777619u;
    }
    return hash;
}

/* This runs only behind the existing generation/stamp full validation.  It
 * binds the compact Task-26 rows to the exact production packet and to the
 * Task-14 dense first-visit plan consumed by PrepareRun. */
static s32 ndsRendererNativeStageValidateGeneratedSegment0(u32 inject_fault)
{
    const NDSNativeStageGeneratedCertificate *certificate =
        &sNdsNativeStageSegment0ColdCertificate;
    const NDSNativeStageSegment *segment = &sNdsNativeStageSegments[0];
    u32 hot_hash = 2166136261u;
    u32 dense_hash = 2166136261u;
    u32 state_count = 0u;
    u32 sync_count = 0u;
    u32 state_cursor = certificate->first_state;
    u32 triangle_count = 0u;
    u64 epoch_mask = 0u;
    u32 i;

    if ((certificate->source_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_SOURCE_CHECKSUM) ||
        (certificate->table_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_TABLE_CHECKSUM) ||
        (certificate->hot_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_HOT_CHECKSUM) ||
        (certificate->prepared_dense_checksum !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_CHECKSUM) ||
        (certificate->segment_index != 0u) ||
        (certificate->first_dobj != segment->first_dobj) ||
        (certificate->dobj_count != segment->dobj_count) ||
        (certificate->owner != segment->owner) ||
        (certificate->link != segment->link) ||
        (certificate->submit_class !=
         NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ||
        (certificate->first_binding != segment->first_binding) ||
        (certificate->binding_count != segment->binding_count) ||
        (certificate->first_run != segment->first_run) ||
        (certificate->run_count != segment->run_count) ||
        (certificate->first_texture_epoch != 0u) ||
        (certificate->triangle_count != 54u) ||
        (certificate->texture_epoch_count != 22u) ||
        (certificate->live_operand_mask !=
         (((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_ASSET_BASES) |
          ((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_BINDING_COMPOSED) |
          ((u32)1u << NDS_NATIVE_STAGE_LIVE_OPERAND_CONFIG))) ||
        (certificate->asset_base_mask != 3u) ||
        (certificate->material_count != 0u) ||
        (certificate->final_tail_span != 73u) ||
        (certificate->prepared_dense_count !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT) ||
        (certificate->prepared_dense_offset_count !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_OFFSET_COUNT) ||
        (sNdsNativeStageWs.dense_offsets[0] != 0u) ||
        (sNdsNativeStageWs.dense_offsets[
             NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT] !=
         NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT))
    {
        return FALSE;
    }

    hot_hash = ndsRendererNativeStageGeneratedHashU32(
        hot_hash, 0x4d335348u);
    hot_hash = ndsRendererNativeStageGeneratedHashU32(
        hot_hash, NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT);
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT; i++)
    {
        const NDSNativeStageGeneratedRun *generated =
            &sNdsNativeStageSegment0HotRuns[i];
        u32 run_index = generated->run_index;
        const NDSNativeStageRun *run;
        const NDSNativeStageStateSpan *span;

        if ((inject_fault != 0u) && (i == 0u))
        {
            run_index ^= 1u;
        }
        hot_hash = ndsRendererNativeStageGeneratedHashU32(
            hot_hash, run_index);
        hot_hash = ndsRendererNativeStageGeneratedHashU32(
            hot_hash, generated->binding_composed_index);
        if (run_index != (u32)certificate->first_run + i)
        {
            return FALSE;
        }
        run = &sNdsNativeStageRuns[run_index];
        span = &sNdsNativeStageStateSpans[run_index];
        if ((run->binding_index != generated->binding_composed_index) ||
            (run->submit_class != certificate->submit_class) ||
            (run->flags != 0u) ||
            (run->texture_epoch < certificate->first_texture_epoch) ||
            (run->texture_epoch >=
             (u32)certificate->first_texture_epoch +
                 certificate->texture_epoch_count))
        {
            return FALSE;
        }
        if ((span->state_count != 0u) &&
            (span->first_state != state_cursor))
        {
            return FALSE;
        }
        state_cursor += span->state_count;
        state_count += span->state_count;
        sync_count += span->sync_count;
        triangle_count += run->triangle_count;
        epoch_mask |= (u64)1u << run->texture_epoch;
    }
    {
        const NDSNativeStageStateSpan *tail =
            &sNdsNativeStageStateSpans[certificate->final_tail_span];

        if ((tail->state_count != 0u) &&
            (tail->first_state != state_cursor))
        {
            return FALSE;
        }
        state_cursor += tail->state_count;
        state_count += tail->state_count;
        sync_count += tail->sync_count;
    }
    if ((hot_hash != certificate->hot_checksum) ||
        (state_count != certificate->state_count) ||
        (sync_count != certificate->sync_count) ||
        (state_cursor !=
         (u32)certificate->first_state + certificate->state_count) ||
        (triangle_count != certificate->triangle_count) ||
        (epoch_mask != (((u64)1u << certificate->texture_epoch_count) - 1u)))
    {
        return FALSE;
    }

    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, 0x4d33535du);
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, certificate->prepared_dense_offset_count);
    for (i = 0u; i < certificate->prepared_dense_offset_count; i++)
    {
        dense_hash = ndsRendererNativeStageGeneratedHashU32(
            dense_hash,
            sNdsNativeStageWs.dense_offsets[i]);
    }
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, 0x4d33535eu);
    dense_hash = ndsRendererNativeStageGeneratedHashU32(
        dense_hash, certificate->prepared_dense_count);
    for (i = 0u; i < certificate->prepared_dense_count; i++)
    {
        dense_hash = ndsRendererNativeStageGeneratedHashU32(
            dense_hash,
            sNdsNativeStageWs.dense_indices[i]);
    }
    if (dense_hash != certificate->prepared_dense_checksum)
    {
        return FALSE;
    }
    return TRUE;
}

/* A run whose source corners carry a per-VERTEX alpha gradient on an
 * untextured material; it samples the alpha ramp that
 * ndsRendererNativeStageAlphaRampResolve owns. Mirror of
 * RUN_FLAG_VERTEX_ALPHA_RAMP in scripts/stages/generate_nds_native_stage.py,
 * defined here rather than in the generated includes so no other packet's
 * pinned include moves. */
#define NDS_NATIVE_STAGE_RUN_FLAG_ALPHA_RAMP (1u << 1)

/* A run on a Sec layer's display head 1 that draws under the translucent mode
 * the head entered with (ndsNativeStageHeadEntryOtherModeL): its I tile's
 * texel alpha is the intensity, so its texture is resolved with the intensity
 * coverage request (Race to the Finish's lights). Mirror of RUN_FLAG_HEAD_XLU,
 * scripts/stages/generate_nds_native_stage.py. */
#define NDS_NATIVE_STAGE_RUN_FLAG_HEAD_XLU (1u << 2)

/* The render mode a Sec layer's display head 1 enters with
 * (gr/grdisplay.c:66-145): G_RM_AA_ZB_XLU_SURF on layer 1 (link 6),
 * G_RM_AA_XLU_SURF on layers 0, 2 and 3. Head 0's opaque surface mode is
 * zero to every runtime test and stays zero. The generator seeds the same
 * word (head_entry_othermode_l), or a head-1 run's policy would not match. */
static inline u32 ndsNativeStageHeadEntryOtherModeL(u32 link, u32 head)
{
    if (head != 1u)
    {
        return 0u;
    }
    return (link == 6u) ? 0x005049D8u : 0x005041C8u;
}

/* Which check of ndsRendererNativeStageValidateTopologyFull declined last
 * (1-based, in source order; 0 = passed or never run). Every decline is one
 * store on the fail path, nothing on the pass path. Stage admission reports
 * only reason 6 for the whole prepare, which is where every non-Dream-Land
 * stage sat on 2026-09-07 with no way to tell which of the ~30 checks below
 * refused it. */
volatile u32 gNdsNativeStageValidateFullFailStep;
volatile u32 gNdsNativeStageValidateFullFailIndex;
/* Step 22 operands: the six summary values, then the packet's dense, raw,
 * no-z, range and cross-run counts they were compared against. */
volatile u32 gNdsNativeStageValidateFullCensus[12];

static s32 ndsRendererNativeStageValidateTopologyFull(
    const NDSRendererNativeStageFrame *frame,
    NDSNativeStageTopologySummary *summary)
{
    u32 prepared_dense_mask[
        (NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT + 31u) / 32u];
    u32 prepared_dense_count = 0u;
    u32 i;

    if ((frame == NULL) || (summary == NULL) ||
        (frame->topology_generation == 0u) ||
        (frame->topology_stamp == 0u))
    {
        gNdsNativeStageValidateFullFailStep = 1u;
        gNdsNativeStageValidateFullFailIndex = 0u;
        return FALSE;
    }
    memset(summary, 0, sizeof(*summary));
    memset(prepared_dense_mask, 0, sizeof(prepared_dense_mask));
    for (i = 0u; i < NDS_NATIVE_STAGE_DOBJ_COUNT; i++)
    {
        const NDSNativeStageDObj *expected = &sNdsNativeStageDObjs[i];
        const NDSRendererNativeStageDObj *live = &frame->dobjs[i];

        if ((live->identity == NULL) ||
            (live->parent_index != expected->parent_index) ||
            (live->binding_index != expected->binding_index) ||
            (live->transform_flags != expected->transform_flags) ||
            (live->owner != expected->owner) ||
            (live->depth != expected->depth))
        {
            gNdsNativeStageValidateFullFailStep = 2u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_ASSET_COUNT; i++)
    {
        if ((frame->asset_bases[i] == NULL) ||
            (sNdsNativeStageAssets[i].payload_size == 0u))
        {
            gNdsNativeStageValidateFullFailStep = 3u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_SEGMENT_COUNT; i++)
    {
        const NDSNativeStageSegment *segment = &sNdsNativeStageSegments[i];
        u32 binding_offset;

        if (((u32)segment->first_dobj + segment->dobj_count >
             NDS_NATIVE_STAGE_DOBJ_COUNT) ||
            ((u32)segment->first_binding + segment->binding_count >
             NDS_NATIVE_STAGE_BINDING_COUNT) ||
            ((u32)segment->first_run + segment->run_count >
             NDS_NATIVE_STAGE_RUN_COUNT) ||
            (segment->reserved != 0u))
        {
            gNdsNativeStageValidateFullFailStep = 4u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
        for (binding_offset = 0u;
             binding_offset < segment->binding_count;
             binding_offset++)
        {
            u32 binding_index =
                (u32)segment->first_binding + binding_offset;
            const NDSNativeStageBinding *binding =
                &sNdsNativeStageBindings[binding_index];

            if ((binding->first_run < segment->first_run) ||
                ((u32)binding->first_run + binding->run_count >
                 (u32)segment->first_run + segment->run_count))
            {
                gNdsNativeStageValidateFullFailStep = 5u;
                gNdsNativeStageValidateFullFailIndex = i;
                return FALSE;
            }
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_BINDING_COUNT; i++)
    {
        const NDSNativeStageBinding *binding = &sNdsNativeStageBindings[i];

        if ((binding->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (binding->root_offset >= sNdsNativeStageAssets[
                 binding->asset_index].payload_size) ||
            ((u32)binding->first_vertex + binding->source_vertex_count >
             NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) ||
            ((u32)binding->first_run + binding->run_count >
             NDS_NATIVE_STAGE_RUN_COUNT) ||
            ((u32)binding->first_epoch + binding->texture_epoch_count >
             NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) ||
            ((binding->material_event != 0xffu) &&
             (binding->material_event >=
              NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT)) ||
            (frame->binding_display_lists[i] !=
             (const void *)((const u8 *)frame->asset_bases[
                 binding->asset_index] + binding->root_offset)))
        {
            gNdsNativeStageValidateFullFailStep = 6u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
        {
            u32 run_offset;

            for (run_offset = 0u; run_offset < binding->run_count;
                 run_offset++)
            {
                if (sNdsNativeStageRuns[
                        (u32)binding->first_run + run_offset].binding_index != i)
                {
                    gNdsNativeStageValidateFullFailStep = 7u;
                    gNdsNativeStageValidateFullFailIndex = i;
                    return FALSE;
                }
            }
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT; i++)
    {
        const NDSNativeStageTextureEpoch *epoch =
            &sNdsNativeStageTextureEpochs[i];

        if ((epoch->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (epoch->source_command_offset >= sNdsNativeStageAssets[
                 epoch->asset_index].payload_size) ||
            (epoch->policy_index >= NDS_NATIVE_STAGE_STATE_POLICY_COUNT) ||
            ((epoch->material_event != 0xffu) &&
             (epoch->material_event >=
              NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT)))
        {
            gNdsNativeStageValidateFullFailStep = 8u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT; i++)
    {
        const NDSNativeStageMaterialEvent *event =
            &sNdsNativeStageMaterialEvents[i];

        if ((event->asset_index >= NDS_NATIVE_STAGE_ASSET_COUNT) ||
            (event->mobj_offset >= sNdsNativeStageAssets[
                 event->asset_index].payload_size) ||
            (event->binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
            /* segment_index is the Gfx-aligned branch slot of the material
             * inside its binding's list (generate_nds_native_stage.py
             * build_material_events), not a segment: Planet Zebes' acid
             * materials sit at 8, 16, ... and Dream Land's all at 0, which is
             * why the old `>= segment count` test only ever passed there. */
            ((event->segment_index & 7u) != 0u) ||
            (event->material_slot >=
             NDS_RENDERER_NATIVE_STAGE_MATERIAL_COUNT) ||
            (event->source_command_count == 0u))
        {
            gNdsNativeStageValidateFullFailStep = 9u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_STATE_SPAN_COUNT; i++)
    {
        if (ndsRendererNativeStageValidateStateSpanTopology(
                &sNdsNativeStageStateSpans[i]) == FALSE)
        {
            gNdsNativeStageValidateFullFailStep = 10u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT; i++)
    {
        if (sNdsNativeStageVertices[i].matrix_binding >=
            NDS_NATIVE_STAGE_BINDING_COUNT)
        {
            gNdsNativeStageValidateFullFailStep = 11u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_RUN_COUNT; i++)
    {
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[i];
        const NDSNativeStageTextureEpoch *epoch;
        u32 corner_count = (u32)run->triangle_count * 3u;
        u32 corner_offset;
        u32 run_alpha = UINT_MAX;

        if ((run->triangle_count == 0u) ||
            (run->binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
            (run->texture_epoch >= NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) ||
            (run->state_policy >= NDS_NATIVE_STAGE_STATE_POLICY_COUNT) ||
            ((run->flags &
              ~(NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX |
                NDS_NATIVE_STAGE_RUN_FLAG_ALPHA_RAMP |
                NDS_NATIVE_STAGE_RUN_FLAG_HEAD_XLU)) != 0u) ||
            ((u32)run->first_corner + corner_count >
             NDS_NATIVE_STAGE_CORNER_COUNT))
        {
            gNdsNativeStageValidateFullFailStep = 12u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
        epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
        if (epoch->policy_index != run->state_policy)
        {
            gNdsNativeStageValidateFullFailStep = 13u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
        if (run->submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
        {
            summary->raw_triangles += run->triangle_count;
        }
        else if (run->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
            summary->projected_no_z_triangles += run->triangle_count;
        }
        else if (run->submit_class ==
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
        {
            summary->projected_range_triangles += run->triangle_count;
        }
        else
        {
            gNdsNativeStageValidateFullFailStep = 14u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
        sNdsNativeStageWs.dense_offsets[i] =
            (u16)prepared_dense_count;
        for (corner_offset = 0u; corner_offset < corner_count; corner_offset++)
        {
            u32 dense_index = sNdsNativeStageCorners[
                (u32)run->first_corner + corner_offset];
            const NDSNativeStageDenseVertex *dense;

            if (dense_index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
            {
                gNdsNativeStageValidateFullFailStep = 15u;
                gNdsNativeStageValidateFullFailIndex = i;
                return FALSE;
            }
            dense = &sNdsNativeStageVertices[dense_index];
            if ((run->submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) &&
                (dense->matrix_binding != run->binding_index))
            {
                gNdsNativeStageValidateFullFailStep = 16u;
                gNdsNativeStageValidateFullFailIndex = i;
                return FALSE;
            }
            if (run_alpha == UINT_MAX)
            {
                run_alpha = dense->rgba & 0xffu;
            }
            else if (run_alpha != (dense->rgba & 0xffu))
            {
                gNdsNativeStageValidateFullFailStep = 17u;
                gNdsNativeStageValidateFullFailIndex = i;
                return FALSE;
            }
            if ((prepared_dense_mask[dense_index / 32u] &
                 ((u32)1u << (dense_index & 31u))) == 0u)
            {
                if (prepared_dense_count >=
                    NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
                {
                    gNdsNativeStageValidateFullFailStep = 18u;
                    gNdsNativeStageValidateFullFailIndex = i;
                    return FALSE;
                }
                prepared_dense_mask[dense_index / 32u] |=
                    (u32)1u << (dense_index & 31u);
                sNdsNativeStageWs.dense_indices[
                    prepared_dense_count++] = (u16)dense_index;
            }
            if (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
            {
                /* The run submits under max(1, its largest packed shift)
                 * (ndsRendererNativeStageRunRangeShift); a vertex fits when
                 * it fits under its own packed shift, which is at most that. */
                u32 vertex_shift = dense->packed_cache_shift >>
                    NDS_NATIVE_STAGE_COORDINATE_SHIFT;
                u32 shift = (vertex_shift > 1u) ? vertex_shift : 1u;
                s32 x = ndsRendererNativeStageVertexShift(dense->x, shift);
                s32 y = ndsRendererNativeStageVertexShift(dense->y, shift);
                s32 z = ndsRendererNativeStageVertexShift(dense->z, shift);

                if ((((run->flags &
                       NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) == 0u) &&
                     (dense->matrix_binding != run->binding_index)) ||
                    (x < -2048) || (x > 2047) ||
                    (y < -2048) || (y > 2047) ||
                    (z < -2048) || (z > 2047))
                {
                    gNdsNativeStageValidateFullFailStep = 19u;
                    gNdsNativeStageValidateFullFailIndex = i;
                    return FALSE;
                }
            }
        }
        if (run_alpha == UINT_MAX)
        {
            gNdsNativeStageValidateFullFailStep = 20u;
            gNdsNativeStageValidateFullFailIndex = i;
            return FALSE;
        }
        if ((run->flags &
             NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) != 0u)
        {
            if ((run->submit_class !=
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) &&
                (run->submit_class !=
                 NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ||
                (run->triangle_count != 2u))
            {
                gNdsNativeStageValidateFullFailStep = 21u;
                gNdsNativeStageValidateFullFailIndex = i;
                return FALSE;
            }
            summary->cross_runs++;
            summary->cross_triangles += run->triangle_count;
            for (corner_offset = 0u;
                 corner_offset < corner_count;
                 corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + corner_offset];

                if (sNdsNativeStageVertices[dense_index].matrix_binding !=
                    run->binding_index)
                {
                    summary->cross_foreign_corners++;
                }
            }
        }
    }
    sNdsNativeStageWs.dense_offsets[
        NDS_NATIVE_STAGE_RUN_COUNT] = (u16)prepared_dense_count;
    if ((NDS_NATIVE_STAGE_STATE_SPAN_COUNT !=
         NDS_NATIVE_STAGE_RUN_COUNT + NDS_NATIVE_STAGE_BINDING_COUNT) ||
        /* A packet may carry dense vertices no run references: the generator's
         * per-triangle alpha split clones a mixed triangle's vertices and the
         * originals it orphans stay in the table (Hyrule 12, Saffron 6,
         * Mushroom Kingdom 4, 2026-09-07). Only over-reference is a defect. */
        (prepared_dense_count > NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT) ||
        (summary->raw_triangles != NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES) ||
        (summary->projected_no_z_triangles !=
         NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES) ||
        (summary->projected_range_triangles !=
         NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES) ||
        (summary->cross_runs != NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT) ||
        (summary->cross_triangles !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT) ||
        (summary->cross_foreign_corners !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT))
    {
        gNdsNativeStageValidateFullFailStep = 22u;
        gNdsNativeStageValidateFullFailIndex = i;
        gNdsNativeStageValidateFullCensus[0] = prepared_dense_count;
        gNdsNativeStageValidateFullCensus[1] = summary->raw_triangles;
        gNdsNativeStageValidateFullCensus[2] =
            summary->projected_no_z_triangles;
        gNdsNativeStageValidateFullCensus[3] =
            summary->projected_range_triangles;
        gNdsNativeStageValidateFullCensus[4] = summary->cross_runs;
        gNdsNativeStageValidateFullCensus[5] = summary->cross_triangles;
        gNdsNativeStageValidateFullCensus[6] = summary->cross_foreign_corners;
        gNdsNativeStageValidateFullCensus[7] =
            NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT;
        gNdsNativeStageValidateFullCensus[8] =
            NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES;
        gNdsNativeStageValidateFullCensus[9] =
            NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES;
        gNdsNativeStageValidateFullCensus[10] =
            NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES;
        gNdsNativeStageValidateFullCensus[11] =
            NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT;
        return FALSE;
    }
    /* The straight-line segment-0 program is one stage's specialisation
     * (Dream Land's).  A stage without one is not invalid -- it just runs the
     * general per-run path -- so its certificate must not be checked against
     * another stage's tables. */
    return (NDS_NATIVE_STAGE_HAS_GENERATED_SEGMENT0 != 0u) ?
        ndsRendererNativeStageValidateGeneratedSegment0(FALSE) : TRUE;
}

static s32 ndsRendererNativeStageValidateTopology(
    const NDSRendererNativeStageFrame *frame,
    NDSNativeStageTopologySummary *summary)
{
    s32 injected_fault = FALSE;

    if ((sNdsNativeStageValidationCache.valid != FALSE) &&
        (sNdsNativeStageValidationCache.generation ==
         frame->topology_generation) &&
        (sNdsNativeStageValidationCache.stamp == frame->topology_stamp))
    {
        *summary = sNdsNativeStageValidationCache.summary;
        return TRUE;
    }
    if (sNdsNativeStageValidationCache.valid != FALSE)
    {
    }
    sNdsNativeStageValidationCache.valid = FALSE;
    if (ndsRendererNativeStageValidateTopologyFull(frame, summary) == FALSE)
    {
        return FALSE;
    }
    sNdsNativeStageValidationCache.summary = *summary;
    sNdsNativeStageValidationCache.generation = frame->topology_generation;
    sNdsNativeStageValidationCache.stamp = frame->topology_stamp;
    sNdsNativeStageValidationCache.valid = TRUE;
    (void)injected_fault;
    return TRUE;
}

static s32 ndsRendererNativeStageApplyStateSpan(
    const NDSNativeStageStateSpan *span,
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state)
{
    u32 i;

    if ((span == NULL) || (frame == NULL) || (stats == NULL) ||
        (state == NULL))
    {
        return FALSE;
    }
    stats->sync_command_count += span->sync_count;
    for (i = 0u; i < span->state_count; i++)
    {
        u32 delta_index = sNdsNativeStageStateSequence[
            (u32)span->first_state + i];
        const NDSNativeStageStateDelta *delta =
            &sNdsNativeStageStateDeltas[delta_index];
        if (delta->effect == NDS_NATIVE_STATE_MATERIAL)
        {
            if (delta->material_command == 0u)
            {
                ndsRendererNativeApplyMaterialPreflight(
                    &frame->materials[delta->material_event], stats, state);
            }
            continue;
        }
        if (delta->effect == NDS_NATIVE_STATE_BLEND)
        {
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            stats->blend_color = delta->w1;
            stats->color_command_count++;
            continue;
        }
        if (delta->effect == NDS_NATIVE_STATE_STAGE_ENVCOLOR)
        {
            /* The environment colour is texel state, not combiner state: an
             * I/IA texture is baked prim x env at upload and keyed on both
             * (NDS_RENDERER_HW_TEXTURE_KEY_PRIM_ENV_BLEND). Dropping it left
             * Yoshi's Island's I4 bushes baked against env 0 -- black squares
             * behind every bush (2026-09-07). */
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            stats->env_color = delta->w1;
            stats->color_command_count++;
            continue;
        }
        {
            NDSNativeStateDelta native_delta;
            const u8 *asset_base = frame->asset_bases[0];

            native_delta.w0 = delta->w0;
            native_delta.w1 = delta->w1;
            /* The stage generator numbers G_MOVEWORD 16; the shared applier
             * spends 14 on the same command (fighter LIGHT_COLOR). */
            native_delta.effect =
                (delta->effect == NDS_NATIVE_STATE_STAGE_MOVEWORD) ?
                NDS_NATIVE_STATE_LIGHT_COLOR : delta->effect;
            native_delta.reserved[0] = 0u;
            native_delta.reserved[1] = 0u;
            native_delta.reserved[2] = 0u;
            if (delta->effect == NDS_NATIVE_STATE_IMAGE)
            {
                asset_base = frame->asset_bases[delta->asset_index];
            }
            ndsRendererNativeApplyStateDelta(
                &native_delta, asset_base, stats, state);
        }
    }
    return TRUE;
}

static s32 ndsRendererNativeStagePolicyMatches(
    const NDSNativeStageStatePolicy *policy,
    const NDSRendererStats *stats)
{
    return ((policy != NULL) && (stats != NULL) &&
            (stats->texture_combine_w0 == policy->combine_w0) &&
            (stats->texture_combine_w1 == policy->combine_w1) &&
            (stats->othermode_h == policy->othermode_h) &&
            (stats->othermode_l == policy->othermode_l) &&
            (stats->geometry_mode == policy->geometry_mode)) ? TRUE : FALSE;
}

static void ndsRendererNativeStageInputVertex(
    const NDSNativeStageDenseVertex *dense,
    NDSRendererInputVertex *input)
{
    input->x = dense->x;
    input->y = dense->y;
    input->z = dense->z;
    input->s = dense->s;
    input->t = dense->t;
    input->r = (u8)(dense->rgba >> 24);
    input->g = (u8)(dense->rgba >> 16);
    input->b = (u8)(dense->rgba >> 8);
    input->a = (u8)dense->rgba;
}


/* Which decline of ndsRendererNativeStagePrepareRun ran last (1-based, source
 * order) and its run index; the owner reports only its own step 4-6. */
/* Does a run that is NOT no-Z ever carry a vertex behind the near plane?
 *
 * Only the no-Z class computes near_inside, so only it can reject or clip a
 * crossing triangle; raw and range runs hand theirs to the hardware whole.
 * That is the standing explanation for Castle's holed tower roof, and it is
 * still only an explanation: the near-fan witnesses that were read as
 * refuting it can never fire for those classes, because they count inside the
 * no-Z fan (2026-09-08). This measures the premise instead of assuming it.
 *
 * Off by default and free when off: the probe sets
 * gNdsNativeStageNearCensusEnabled, which turns on the same transform the
 * no-Z class already pays for, on the other two classes, and counts rather
 * than changes anything. Nothing here alters a submitted vertex. */
volatile u32 gNdsNativeStageNearCensusEnabled;
volatile u32 gNdsNativeStageNearCensusVertices;
volatile u32 gNdsNativeStageNearCensusOutside;
volatile u32 gNdsNativeStageNearCensusZeroW;
/* Near-plane fan witnesses: triangles emitted through the clipper and
 * fan corners refused for a zero w after the 8-bit shift. */
volatile u32 gNdsNativeStageNearFanCount;
volatile u32 gNdsNativeStageNearFanZeroWCount;
/* No-Z fast-path per-triangle cull witness: triangles dropped because all
 * three corners tested outside (inside_count == 0). The site previously
 * recorded only the shared profile reject counter, which the two clipper-path
 * rejects and the generic pipeline also increment, so a run culled entirely
 * here read as a SUCCESS -- every fail step stayed 0 and nothing named it. */
__attribute__((used)) volatile u32 gNdsNativeStageNoZInsideCullCount;
__attribute__((used)) volatile u32 gNdsNativeStageNoZForeignBindingCount;
__attribute__((used)) volatile u32 gNdsNativeStageNoZEnsureWorldCount;
__attribute__((used)) volatile u32 gNdsNativeStageEmitShortfallCount;
__attribute__((used)) volatile u32 gNdsNativeStageEmitShortfallResidue;
typedef struct NDSNativeStageEmitShortfallSnapshot
{
    u32 active;
    u32 run_index;
    u32 near_reject_count;
    u32 submit_reject_count;
    u32 no_z_inside_cull_count;
    u32 near_fan_zero_w_count;
    u32 near_fan_count;
    u32 no_z_foreign_binding_count;
    u32 no_z_ensure_world_count;
} NDSNativeStageEmitShortfallSnapshot;
static NDSNativeStageEmitShortfallSnapshot sNdsNativeStageEmitShortfallSnapshot;
volatile u32 gNdsNativeStagePrepareRunFailStep;
volatile u32 gNdsNativeStagePrepareRunFailRun;
/* Step 1 operands: live/expected combine w0, w1, othermode h, l, geometry. */
volatile u32 gNdsNativeStagePrepareRunPolicy[10];
/* Step 2 operands: image, load kind, state flags, format<<8|size, load
 * texels, uls<<16|ult, lrs<<16|dxt, render tile line. */
volatile u32 gNdsNativeStagePrepareRunTexture[8];
/* Set by ndsNativeStageBlobLoad; lets the next prepare upload textures the
 * preflight lookup misses (see PrepareRun step 2), cleared once a prepare
 * is accepted. Dream Land never sets it. */
volatile u32 gNdsNativeStageWarmUploads;
volatile u32 gNdsNativeStageWarmUploadCount;
/* The stage select's preview spreads a stage's first uploads over several
 * draws: once a prepare has uploaded one texture and spent this many ticks
 * since it began, it declines at the next miss (0: no limit, the battle's
 * warm). Both counts restart at every prepare. */
volatile u32 gNdsNativeStageWarmUploadLimit;
static u32 sNdsNativeStageWarmUploadsThisPrepare;
static u32 sNdsNativeStageWarmPrepareStart;
/* Non-point texture-filter phase witness, reset once per taskman heap/scene
 * generation. Keep these exported even when no C code reads them: the scene
 * probe is the consumer and --gc-sections has dropped unreferenced diagnostics
 * before. The packet snapshots make a zero run count distinguishable from a
 * false live-source predicate. */
__attribute__((used)) volatile u32 gNdsNativeStageFilterPhase8RunCount;
__attribute__((used)) volatile u32 gNdsNativeStageFilterPhase16RunCount;
__attribute__((used)) volatile u32 gNdsNativeStageFilterPhaseActivePacket;
__attribute__((used)) volatile u32 gNdsNativeStageFilterPhaseBlobPacket;
/* Per-run hardware-emission witness. Valid publishes the current serial only
 * after the commit loop has captured Emitted, so a skipped/declined run cannot
 * make a prior frame's emitted count look current to the scene probe. */
__attribute__((used)) volatile u32 gNdsNativeStageRoofSnapSerial;
/* Valid, Given and Emitted per run live in the stage workspace
 * (nds_renderer_assets.c), sized by the active packet. */
/* Castle source DL 0x1698 (currently run 9) GX position-test witness.  It
 * shares RoofSnapSerial: Valid is published only after all 27 corners have
 * been captured, so Valid != RoofSnapSerial is explicitly stale/partial. */
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipValid;
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipArm;
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipRun;
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipCornerCount;
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipProjectedZ[9];
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipShift[9];
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipDense[27];
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipSubmitV16[81];
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipResult[108];
__attribute__((used)) volatile u32 gNdsNativeStageCastleRoofClipFlags[27];
static u32 sNdsNativeStageFilterPhaseHeapGeneration = UINT_MAX;

/* THE DS CANNOT INTERPOLATE POLYGON ALPHA; IT DOES INTERPOLATE TEXEL ALPHA.
 *
 * POLYGON_ATTR carries one alpha per polygon, so an N64 per-vertex alpha
 * gradient on an untextured surface reached the screen as the generator's
 * per-triangle average. Zebes' three stage-light beams fade from alpha 160 at
 * the lamp to 0 along their top edge; averaged, each was one flat alpha-53
 * sheet with a hard top edge, and subdividing only turns one flat band into
 * several.
 *
 * A run flagged ALPHA_RAMP samples this texture instead: 8x32 A5I3, white,
 * row r holding alpha r of 31. The generator puts each corner's source alpha
 * on T (32 texels x alpha / peak) and the peak in the run's polygon alpha, so
 * modulate draws peak x the source's own linear interpolant, to 5-bit steps.
 * 256 bytes of texture VRAM and one 8-entry palette, made once per texture
 * VRAM generation.
 *
 * The name is a dedicated one, not a cache slot, so the cache's teardown never
 * reaches it; glResetTextures at scene entry invalidates it, and the VRAM reset
 * count is what moves when that happens. A stale name is forgotten, never
 * deleted -- libnds may already have re-issued the number. The entry record
 * exists only so the prepared-run texture proof (ready, name, key_generation)
 * reads this texture exactly as it reads a cache hit. */
#define NDS_NATIVE_STAGE_ALPHA_RAMP_WIDTH 8u
#define NDS_NATIVE_STAGE_ALPHA_RAMP_HEIGHT 32u
/* gSPTexture 1.0: TexCoord turns the packet's S10.5 into DS 12.4 unchanged. */
#define NDS_NATIVE_STAGE_ALPHA_RAMP_SCALE 0x10000u

static NDSRendererHardwareTextureCacheEntry sNdsNativeStageAlphaRampEntry;
static u32 sNdsNativeStageAlphaRampVramReset;
/* Tile origin 0/0: ramp T is authored in texels, against no source tile. */
static const NDSRendererTileState sNdsNativeStageAlphaRampTile;
/* Engagement: ramp uploads, and ramp runs prepared. */
__attribute__((used)) volatile u32 gNdsNativeStageAlphaRampUploadCount;
__attribute__((used)) volatile u32 gNdsNativeStageAlphaRampRunCount;

static s32 ndsRendererNativeStageAlphaRampFill(
    u8 *pixels, u32 bytes, void *user_data)
{
    u32 i;

    (void)user_data;
    if ((pixels == NULL) ||
        (bytes != (NDS_NATIVE_STAGE_ALPHA_RAMP_WIDTH *
                   NDS_NATIVE_STAGE_ALPHA_RAMP_HEIGHT)))
    {
        return FALSE;
    }
    /* A5I3: alpha in bits 3-7, palette index 0 (white) in bits 0-2. */
    for (i = 0u; i < bytes; i++)
    {
        pixels[i] = (u8)((i / NDS_NATIVE_STAGE_ALPHA_RAMP_WIDTH) << 3);
    }
    return TRUE;
}

static s32 __attribute__((noinline, cold))
ndsRendererNativeStageAlphaRampResolve(
    NDSRendererHardwareResolvedTexture *resolved)
{
    static const u16 white[8] = {
        0x7fffu, 0x7fffu, 0x7fffu, 0x7fffu,
        0x7fffu, 0x7fffu, 0x7fffu, 0x7fffu
    };
    NDSRendererHardwareTextureCacheEntry *entry =
        &sNdsNativeStageAlphaRampEntry;

    if ((entry->ready == 0u) ||
        (sNdsNativeStageAlphaRampVramReset !=
         gNdsRendererSceneTextureVramResetCount))
    {
        u32 name = 0u;

        memset(entry, 0, sizeof(*entry));
        if (ndsRendererHardwarePrepareIFCommonCloudAtlas(
                NDS_NATIVE_STAGE_ALPHA_RAMP_WIDTH,
                NDS_NATIVE_STAGE_ALPHA_RAMP_HEIGHT, white,
                ndsRendererNativeStageAlphaRampFill, NULL, &name) == FALSE)
        {
            return FALSE;
        }
        /* The prepare leaves the ramp bound, so the merge reads the ramp's own
         * format word: texcoord texgen, no repeat, so T past the ramp clamps.
         * A new identity is a key-generation event, exactly as a re-key is. */
        entry->name = (int)name;
        entry->params =
            ndsRendererHardwareMergeTextureParams((u32)TEXGEN_TEXCOORD);
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
        entry->ready = TRUE;
        sNdsNativeStageAlphaRampVramReset =
            gNdsRendererSceneTextureVramResetCount;
        NDS_DIAG(gNdsNativeStageAlphaRampUploadCount++);
    }
    resolved->entry = entry;
    resolved->name = (u32)entry->name;
    resolved->params = entry->params;
    resolved->format = NDS_RENDERER_HW_TEXTURE_FMT_IA;
    resolved->width = NDS_NATIVE_STAGE_ALPHA_RAMP_WIDTH;
    resolved->height = NDS_NATIVE_STAGE_ALPHA_RAMP_HEIGHT;
    return TRUE;
}

static u32 ndsRendererNativeStageRunRangeShift(const NDSNativeStageRun *run);

static s32 ndsRendererNativeStagePrepareRun(
    u32 run_index,
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u64 *epoch_mask)
{
    const NDSNativeStageRun *run = &sNdsNativeStageRuns[run_index];
    const NDSNativeStageTextureEpoch *epoch;
    const NDSNativeStageStatePolicy *policy;
    NDSNativeStagePreparedRun *prepared =
        &sNdsNativeStageWs.runs[run_index];
    const NDSRendererTileState *render_tile;
    NDSRendererHardwareResolvedTexture resolved;
    u32 implicit_texture_on;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 material_color;
    s32 alpha_uses_vertex;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 live_source_frame = FALSE;
    s32 texture_offset;
    u32 first_visit_offset;
    u32 first_visit_end;
    u32 dense_offset;
    u32 alpha = UINT_MAX;
    u32 use_texture;

    /* R2-07 leg A. THE write seam for runs[]: this function is its only writer,
     * so dropping the certificate here removes the class rather than an
     * instance (slice 30). The drop beside gNdsR2StagePrepareBuildCount is the
     * instance and is deliberately kept -- redundant but free, and deleting it
     * would be a second change with its own failure mode, which is slice 30's
     * own precedent for exactly this situation.
     *
     * The seam is the one that matters because the instance is not complete in
     * every configuration: with NDS_R2_STAGE_ROUTE_PROBE on, a segment can be
     * forced generic and re-prepared while r2_reuse is still 1, i.e. without
     * the rebuild branch ever running. That path is `#define ... FALSE` in
     * every shipping and gate target, so this line changes no measured
     * behaviour -- it stops the guard depending on a probe flag being off. */
    ndsRendererNativeStagePreparedTextureProofDrop();
    epoch = &sNdsNativeStageTextureEpochs[run->texture_epoch];
    policy = &sNdsNativeStageStatePolicies[run->state_policy];
    if (ndsRendererNativeStagePolicyMatches(policy, stats) == FALSE)
    {
        gNdsNativeStagePrepareRunFailStep = 1u;
        gNdsNativeStagePrepareRunFailRun = run_index;
        gNdsNativeStagePrepareRunPolicy[0] = stats->texture_combine_w0;
        gNdsNativeStagePrepareRunPolicy[1] = policy->combine_w0;
        gNdsNativeStagePrepareRunPolicy[2] = stats->texture_combine_w1;
        gNdsNativeStagePrepareRunPolicy[3] = policy->combine_w1;
        gNdsNativeStagePrepareRunPolicy[4] = stats->othermode_h;
        gNdsNativeStagePrepareRunPolicy[5] = policy->othermode_h;
        gNdsNativeStagePrepareRunPolicy[6] = stats->othermode_l;
        gNdsNativeStagePrepareRunPolicy[7] = policy->othermode_l;
        gNdsNativeStagePrepareRunPolicy[8] = stats->geometry_mode;
        gNdsNativeStagePrepareRunPolicy[9] = policy->geometry_mode;
        return FALSE;
    }

    memset(prepared, 0, sizeof(*prepared));
    memset(&resolved, 0, sizeof(resolved));
    use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
        TRUE : FALSE;
    implicit_texture_on = ndsRendererHardwareTextureImplicitStateOn(stats);
    texture_scale_s = stats->texture_scale_s;
    texture_scale_t = stats->texture_scale_t;
    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
#if NDS_NATIVE_STAGE_MULTI
    {
        const NDSNativeStageBlobPacket *blob = ndsNativeStageBlobPacket();

        live_source_frame =
            ((blob != NULL) &&
             (sNdsNativeStagePacketActive ==
              (const NDSNativeStagePacket *)(const void *)blob)) ?
                TRUE : FALSE;
    }
#endif
    texture_offset = ndsRendererHardwareTextureFilterOffsetForSourceFrame(
        stats, live_source_frame);
    if (texture_offset != 0)
    {
        if (live_source_frame != FALSE)
        {
            NDS_DIAG(gNdsNativeStageFilterPhase8RunCount++);
        }
        else
        {
            NDS_DIAG(gNdsNativeStageFilterPhase16RunCount++);
        }
    }
    /* Held only across this run's texture resolve below (and cleared on
     * every exit from it): the request keys the I tile's texture with its
     * intensity as coverage, for this run alone. */
    sNdsRendererHardwareIntensityCoverage =
        ((run->flags & NDS_NATIVE_STAGE_RUN_FLAG_HEAD_XLU) != 0u) ? 1u : 0u;
    if ((run->flags & NDS_NATIVE_STAGE_RUN_FLAG_ALPHA_RAMP) != 0u)
    {
        /* The source material samples no texel (the generator refuses a ramp
         * on one that does), so the ramp is this run's only texture and each
         * corner's T is its source alpha: see
         * ndsRendererNativeStageAlphaRampResolve. */
        if ((use_texture != FALSE) ||
            (ndsRendererNativeStageAlphaRampResolve(&resolved) == FALSE))
        {
            gNdsNativeStagePrepareRunFailStep = 7u;
            gNdsNativeStagePrepareRunFailRun = run_index;
            sNdsRendererHardwareIntensityCoverage = 0u;
            return FALSE;
        }
        use_texture = TRUE;
        implicit_texture_on = FALSE;
        texture_scale_s = NDS_NATIVE_STAGE_ALPHA_RAMP_SCALE;
        texture_scale_t = NDS_NATIVE_STAGE_ALPHA_RAMP_SCALE;
        render_tile = &sNdsNativeStageAlphaRampTile;
        texture_offset = 0;
        NDS_DIAG(gNdsNativeStageAlphaRampRunCount++);
    }
    else if ((use_texture != FALSE) &&
        (ndsRendererHardwareResolveStageSourceFrameTexture(
             stats, frame->config, state, &resolved) == FALSE) &&
        /* Dream Land's textures are pinned by the P1 static set, so its
         * preflight (a pure resident lookup) always hits. A blob stage has
         * no pin set and nothing else uploads its textures before the owner
         * runs, so every blob stage declined here forever (Sector Z, Yoshi's
         * Island, Saffron City, Planet Zebes, 2026-09-07). Until each packet
         * carries its own pin set, the first prepare after a blob load may
         * upload through the live path once and retry the lookup. */
        ((gNdsNativeStageWarmUploads == 0u) ||
         ((gNdsNativeStageWarmUploadLimit != 0u) &&
          (sNdsNativeStageWarmUploadsThisPrepare != 0u) &&
          ((cpuGetTiming() - sNdsNativeStageWarmPrepareStart) >=
           gNdsNativeStageWarmUploadLimit)) ||
         (ndsRendererHardwareEndBatch(),
          gNdsNativeStageWarmUploadCount++,
          sNdsNativeStageWarmUploadsThisPrepare++,
          ndsRendererHardwareResolveOrBindTexture(
              stats, frame->config, state, NULL, TRUE) == FALSE) ||
         (ndsRendererHardwareResolveStageSourceFrameTexture(
              stats, frame->config, state, &resolved) == FALSE)))
    {
        gNdsNativeStagePrepareRunFailStep = 2u;
        gNdsNativeStagePrepareRunFailRun = run_index;
        gNdsNativeStagePrepareRunTexture[0] = stats->texture_image;
        gNdsNativeStagePrepareRunTexture[1] = stats->texture_load_kind;
        gNdsNativeStagePrepareRunTexture[2] = stats->texture_state_flags;
        gNdsNativeStagePrepareRunTexture[3] =
            (stats->texture_format << 8) | stats->texture_size;
        gNdsNativeStagePrepareRunTexture[4] = stats->texture_load_texels;
        gNdsNativeStagePrepareRunTexture[5] =
            (stats->texture_load_block_uls << 16) |
            stats->texture_load_block_ult;
        gNdsNativeStagePrepareRunTexture[6] =
            (stats->texture_load_block_lrs << 16) |
            stats->texture_load_block_dxt;
        gNdsNativeStagePrepareRunTexture[7] =
            stats->texture_tiles[ndsRendererActiveTextureTile(stats)].line;
        sNdsRendererHardwareIntensityCoverage = 0u;
        return FALSE;
    }
    sNdsRendererHardwareIntensityCoverage = 0u;
    if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
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
    if ((gNdsNativeStageWarmUploads != 0u) && (resolved.entry != NULL))
    {
        resolved.entry->stage_warm = TRUE;
    }
    prepared->texture_entry = resolved.entry;
    prepared->texture_name = resolved.name;
    prepared->texture_params = resolved.params;
    prepared->texture_generation = (resolved.entry != NULL) ?
        resolved.entry->key_generation : 0u;
    prepared->texture_format = (u8)resolved.format;
    prepared->texture_width = (u16)resolved.width;
    prepared->texture_height = (u16)resolved.height;
    prepared->textured = (u8)use_texture;
    prepared->alpha_test =
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) ? TRUE : FALSE;
    prepared->alpha_ref = (u8)((stats->blend_color & 0xffu) >> 4);
    prepared->coordinate_shift =
        (run->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
        (u8)ndsRendererNativeStageRunRangeShift(run) : 0u;

    material_color = ndsRendererHardwareColorSource(stats);
    alpha_uses_vertex = ndsRendererHardwareAlphaUsesVertex(stats);
    use_material_color = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
    if (alpha_uses_vertex == FALSE)
    {
        alpha = ndsRendererHardwareAlpha(stats, NULL);
    }
    if (alpha_uses_vertex != FALSE)
    {
        u32 dense_index = sNdsNativeStageCorners[run->first_corner];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];

        alpha = (dense->rgba & 0xffu) >> 3;
    }
    first_visit_offset =
        sNdsNativeStageWs.dense_offsets[run_index];
    first_visit_end =
        sNdsNativeStageWs.dense_offsets[run_index + 1u];
    if ((first_visit_offset > first_visit_end) ||
        (first_visit_end > NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT))
    {
        gNdsNativeStagePrepareRunFailStep = 3u;
        gNdsNativeStagePrepareRunFailRun = run_index;
        return FALSE;
    }
    for (dense_offset = first_visit_offset;
         dense_offset < first_visit_end;
         dense_offset++)
    {
        u32 dense_index =
            sNdsNativeStageWs.dense_indices[
                dense_offset];
        const NDSNativeStageDenseVertex *dense;
        NDSNativeStagePreparedDense *prepared_dense;

        if (dense_index >= NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT)
        {
            gNdsNativeStagePrepareRunFailStep = 4u;
            gNdsNativeStagePrepareRunFailRun = run_index;
            return FALSE;
        }
        dense = &sNdsNativeStageVertices[dense_index];
        prepared_dense = &sNdsNativeStagePreparedDense[dense_index];
        prepared_dense->packed_color =
            ndsRendererHardwarePackedValidVertexColor(
                material_color, use_material_color,
                use_vertex_color, dense->rgba,
                frame->config->color_modulate);
        if (use_texture != FALSE)
        {
            prepared_dense->s = ndsRendererHardwareTexCoord(
                dense->s, texture_scale_s, render_tile->uls,
                texture_offset);
            prepared_dense->t = ndsRendererHardwareTexCoord(
                dense->t, texture_scale_t, render_tile->ult,
                texture_offset);
        }
        /* Skip rigid bindings. Under NDS_TASK36_HW_COMPOSE the GX composes
         * their world matrix, so binding_composed is not the transform the
         * hardware uses and the CPU-side clip w is meaningless -- it reads
         * 0 for every vertex. Without this the census reported all 155 of
         * Yoshi Island's binding-15 vertices as zero-w and outside, which
         * was read as a cause and is only an artefact. Same predicate the
         * no-Z branch below uses. The counters keep their meaning:
         * Vertices is the population whose CPU clip is authoritative, and
         * ZeroW and Outside are subsets of it. */
        if ((gNdsNativeStageNearCensusEnabled != 0u) &&
            (run->submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            && ((frame->rigid_binding_mask &
                 ((u64)1u << dense->matrix_binding)) == 0u)
           )
        {
            NDSRendererInputVertex census_input;
            NDSRendererClipVertex20p12 census_clip;

            ndsRendererNativeStageInputVertex(dense, &census_input);
            ndsRendererTransformVertex20p12(
                &frame->binding_composed[dense->matrix_binding],
                &census_input, &census_clip);
            NDS_DIAG(gNdsNativeStageNearCensusVertices++);
            if (census_clip.w == 0)
            {
                NDS_DIAG(gNdsNativeStageNearCensusZeroW++);
            }
            else if (ndsRendererHardwareClipZWInsideNearPlane(
                         census_clip.z, census_clip.w) == FALSE)
            {
                NDS_DIAG(gNdsNativeStageNearCensusOutside++);
            }
        }
        if (run->submit_class ==
                  NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
        {
            if ((frame->rigid_binding_mask &
                 ((u64)1u << dense->matrix_binding)) != 0u)
            {
                /* GX performs the rigid near-plane clip after hardware
                 * composition; dynamic bindings keep the exact CPU clip. */
                prepared_dense->near_inside = TRUE;
            }
            else
            {
            NDSRendererInputVertex input;
            NDSRendererClipVertex20p12 clip;

            ndsRendererNativeStageInputVertex(dense, &input);

            ndsRendererTransformVertex20p12(
                &frame->binding_composed[dense->matrix_binding],
                &input, &clip);
            if (clip.w == 0)
            {
                gNdsNativeStagePrepareRunFailStep = 5u;
                gNdsNativeStagePrepareRunFailRun = run_index;
                return FALSE;
            }
            if (ndsRendererHardwareClipZWInsideNearPlane(
                    clip.z, clip.w) != FALSE)
            {
                prepared_dense->near_inside = TRUE;
            }
            else
            {
                prepared_dense->near_inside = FALSE;
            }
            }
        }
    }
    if (alpha == UINT_MAX)
    {
        gNdsNativeStagePrepareRunFailStep = 6u;
        gNdsNativeStagePrepareRunFailRun = run_index;
        return FALSE;
    }
    prepared->poly_fmt = ndsRendererHardwarePolyFmt(stats, alpha);
    gNdsNativeStageRoofSnapGiven[run_index] = run->triangle_count;
    *epoch_mask |= (u64)1u << run->texture_epoch;
    return TRUE;
}

static s32 ndsRendererNativeStagePrepareGeneratedSegment0(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u64 *epoch_mask)
{
    const NDSNativeStageGeneratedCertificate *certificate =
        &sNdsNativeStageSegment0ColdCertificate;

    if ((frame == NULL) || (stats == NULL) || (state == NULL) ||
        (epoch_mask == NULL) ||
        (sNdsNativeStageValidationCache.valid == FALSE))
    {
        goto fail;
    }
#define NDS_TASK26_SYNC(count) \
    do { \
        stats->sync_command_count += (u32)(count); \
    } while (0)
#define NDS_TASK26_OTHERMODE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordOtherMode( \
            stats, ((u32)(w0)) >> 24, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_COMBINE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetCombine(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_TEXTURE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordTextureState(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_GEOMETRY(clear_mask, set_mask) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        stats->geometry_mode = \
            (stats->geometry_mode & (u32)(clear_mask)) | (u32)(set_mask); \
        stats->geometry_clear_mask = (u32)(clear_mask); \
        stats->geometry_set_mask = (u32)(set_mask); \
        stats->geometry_command_count++; \
    } while (0)
#define NDS_TASK26_IMAGE(asset_index, w0, offset) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetImage( \
            stats, (u32)(w0), \
            (u32)(uintptr_t)(frame->asset_bases[(u32)(asset_index)] + \
                             (u32)(offset))); \
    } while (0)
#define NDS_TASK26_TILE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetTile(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_LOAD_TLUT(w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordLoadTlut(stats, (u32)(w1)); \
    } while (0)
#define NDS_TASK26_LOAD_BLOCK(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordLoadBlock(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_TILE_SIZE(w0, w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        ndsRendererRecordSetTileSize(stats, (u32)(w0), (u32)(w1)); \
    } while (0)
#define NDS_TASK26_BLEND(w1) \
    do { \
        NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state); \
        stats->blend_color = (u32)(w1); \
        stats->color_command_count++; \
    } while (0)
#define NDS_TASK26_RUN(run_index) \
    do { \
        if (ndsRendererNativeStagePrepareRun( \
                (u32)(run_index), frame, stats, state, epoch_mask) == FALSE) \
        { \
            goto fail; \
        } \
    } while (0)
    NDS_NATIVE_STAGE_SEGMENT0_GENERATED_PROGRAM;
#undef NDS_TASK26_RUN
#undef NDS_TASK26_BLEND
#undef NDS_TASK26_TILE_SIZE
#undef NDS_TASK26_LOAD_BLOCK
#undef NDS_TASK26_LOAD_TLUT
#undef NDS_TASK26_TILE
#undef NDS_TASK26_IMAGE
#undef NDS_TASK26_GEOMETRY
#undef NDS_TASK26_TEXTURE
#undef NDS_TASK26_COMBINE
#undef NDS_TASK26_OTHERMODE
#undef NDS_TASK26_SYNC
    return TRUE;

fail:
    return FALSE;
}


/* The stage run witness (per-run RoofSnap publication and the emit-shortfall
 * snapshot/accounting) is lab visibility read by
 * scripts/diagnostics/probe-native-render-scene.ps1, which sets this word at
 * boot. Off by default: its per-run snapshot of seven counters, publication and
 * accounting ran in every frame's commit loop. */
__attribute__((used)) volatile u32 gNdsNativeStageRunWitness;

/* The shortfall witness is visibility/debug accounting, not part of the stage
 * emit hot path.  Keep its comparatively large attribution tree in cached
 * main RAM so adding a diagnostic cannot consume the fixed 32 KiB ITCM budget
 * used by the native stage/fighter owners. */
static void __attribute__((noinline, optimize("Os")))
ndsRendererNativeStageAccountShortfall(void)
{
    if (sNdsNativeStageEmitShortfallSnapshot.active != 0u)
    {
        u32 run_index = sNdsNativeStageEmitShortfallSnapshot.run_index;

        if ((run_index < sNdsNativeStageWs.run_capacity) &&
            (gNdsNativeStageRoofSnapValid[run_index] ==
             gNdsNativeStageRoofSnapSerial))
        {
            u32 given = gNdsNativeStageRoofSnapGiven[run_index];
            u32 emitted = gNdsNativeStageRoofSnapEmitted[run_index];

            if (emitted < given)
            {
                u32 near_reject_delta =
                    sNdsRendererRuntimeFrameSummary.near_plane_triangle_reject_count -
                    sNdsNativeStageEmitShortfallSnapshot.near_reject_count;
                u32 submit_reject_delta =
                    sNdsRendererHardwareSubmitClassCounts[
                        NDS_RENDERER_HW_SUBMIT_REJECT] -
                    sNdsNativeStageEmitShortfallSnapshot.submit_reject_count;
                u32 no_z_inside_cull_delta =
                    gNdsNativeStageNoZInsideCullCount -
                    sNdsNativeStageEmitShortfallSnapshot.no_z_inside_cull_count;
                u32 near_fan_zero_w_delta =
                    gNdsNativeStageNearFanZeroWCount -
                    sNdsNativeStageEmitShortfallSnapshot.near_fan_zero_w_count;
                u32 near_fan_delta =
                    gNdsNativeStageNearFanCount -
                    sNdsNativeStageEmitShortfallSnapshot.near_fan_count;
                u32 no_z_foreign_binding_delta =
                    gNdsNativeStageNoZForeignBindingCount -
                    sNdsNativeStageEmitShortfallSnapshot.no_z_foreign_binding_count;
                u32 no_z_ensure_world_delta =
                    gNdsNativeStageNoZEnsureWorldCount -
                    sNdsNativeStageEmitShortfallSnapshot.no_z_ensure_world_count;

                /* Existing clipping counters mix input-triangle and fan-output
                 * units, so exact subtraction is not sound. This conservative
                 * residue is the shortfall only when no legitimate loss/clip
                 * counter moved for this committed run. */
                if ((near_reject_delta == 0u) &&
                    (submit_reject_delta == 0u) &&
                    (no_z_inside_cull_delta == 0u) &&
                    (near_fan_zero_w_delta == 0u) &&
                    (no_z_foreign_binding_delta == 0u) &&
                    (no_z_ensure_world_delta == 0u) &&
                    ((near_fan_delta - near_fan_zero_w_delta) == 0u))
                {
                    NDS_DIAG(gNdsNativeStageEmitShortfallCount++);
                    NDS_DIAG(gNdsNativeStageEmitShortfallResidue += given - emitted);
                }
            }
        }
        sNdsNativeStageEmitShortfallSnapshot.active = 0u;
    }
}

static void ndsRendererNativeStageAccountRun(
    NDSRendererStats *stats, u32 submit_class, u32 triangle_count)
{
    u32 reuse_count = (triangle_count != 0u) ? triangle_count - 1u : 0u;

    if (gNdsNativeStageRunWitness != 0u)
    {
        ndsRendererNativeStageAccountShortfall();
    }

    sNdsRendererHardwareSubmitClassCounts[submit_class] += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count += reuse_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += triangle_count * 3u;
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        sNdsRendererRuntimeFrameSummary.raw_current_candidate_count +=
            triangle_count;
        stats->hardware_zbuffer_triangle_count += triangle_count;
    }
    else
    {
        sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count +=
            triangle_count;
        stats->hardware_projected_depth_triangle_count += triangle_count;
    }
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    sNdsRendererHardwareSubmitted = TRUE;
}

static inline void ndsRendererNativeStageWriteColor(u32 value)
{
    ndsRendererHardwareWriteColorWord(value);
}

static inline void ndsRendererNativeStageWriteTexCoord(u32 value)
{
    ndsRendererHardwareWriteTexCoordWord(value);
}

static inline void ndsRendererNativeStageWriteVertex16(u32 xy, u32 z)
{
    ndsRendererHardwareWriteVertex16Words(xy, z);
}


static s32 ndsRendererNativeStageTask36BindingIsRigid(u32 binding_index)
{
    return ((binding_index < NDS_NATIVE_STAGE_BINDING_COUNT) &&
            ((sNdsNativeStageOwnerExecution.rigid_binding_mask &
              ((u64)1u << binding_index)) != 0u)) ? TRUE : FALSE;
}

static s32 ndsRendererNativeStageTask36BuildWorld(
    u32 binding_index, u32 coordinate_shift, m4x4 *hardware)
{
    NDSRendererMatrix20p12 scaled;
    u32 row;
    u32 col;

    if ((binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
        (sNdsNativeStageOwnerExecution.binding_world == NULL) ||
        (hardware == NULL))
    {
        return FALSE;
    }
    scaled = sNdsNativeStageOwnerExecution.binding_world[binding_index];
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 value = (s64)scaled.m[row][col] << coordinate_shift;

            if ((value > INT_MAX) || (value < INT_MIN))
            {
                return FALSE;
            }
            scaled.m[row][col] = (s32)value;
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        scaled.m[3][col] = ndsRendererRoundShiftS32Signed(
            scaled.m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    ndsRendererCopyMtx20p12ToM4x4(&scaled, hardware);
    return TRUE;
}

static u32 ndsRendererNativeStageTask36TriangleShift(
    const NDSNativeStageRun *run, u32 triangle_offset)
{
    u32 first_corner = (u32)run->first_corner + triangle_offset * 3u;
    u32 coordinate_shift = 0u;
    u32 corner_offset;

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        u32 vertex_shift = sNdsNativeStageVertices[dense_index].packed_cache_shift >>
            NDS_NATIVE_STAGE_COORDINATE_SHIFT;

        if (vertex_shift > coordinate_shift)
        {
            coordinate_shift = vertex_shift;
        }
    }
    return coordinate_shift;
}

/* The shift a whole PROJECTED_RANGE_OR_MATRIX run submits under: the largest
 * per-vertex shift the generator packed over every corner, never below the 1
 * the class always used (which keeps Dream Land's emission identical). */
static u32 ndsRendererNativeStageRunRangeShift(const NDSNativeStageRun *run)
{
    u32 coordinate_shift = 1u;
    u32 triangle_offset;

    for (triangle_offset = 0u; triangle_offset < run->triangle_count;
         triangle_offset++)
    {
        u32 shift = ndsRendererNativeStageTask36TriangleShift(
            run, triangle_offset);

        if (shift > coordinate_shift)
        {
            coordinate_shift = shift;
        }
    }
    return coordinate_shift;
}

static s32 ndsRendererNativeStageTask36BeginSegment(void)
{
    m4x4 projection_hardware;
    m4x4 camera_hardware;

    if ((sNdsNativeStageOwnerExecution.projection == NULL) ||
        (sNdsNativeStageOwnerExecution.camera_modelview == NULL) ||
        (sNdsNativeStageOwnerExecution.binding_world == NULL))
    {
        return FALSE;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererCopyMtx20p12ToM4x4(
        sNdsNativeStageOwnerExecution.projection, &projection_hardware);
    /* The world and camera translations below are loaded in hardware units
     * (source / 256) with the homogeneous 1 left alone, so the position the
     * projection receives is (camera-space / 256, 1). Its own constant row has
     * to be in the same unit or clip z = z/256 * m22 + m32 lands far outside
     * -w..w and the geometry engine near-clips the whole polygon. The no-Z
     * runs never noticed -- LoadNoZProjection replaces the z column, and m32
     * with it -- and until Yoshi's Island no rigid binding drew with source
     * depth: its layer 1 (binding 15: the three platforms and the book's
     * pages, 77 triangles) emitted every triangle and showed none. This is
     * the same division ndsRendererBuildRawHardwareMatrix applies to the
     * CPU-composed row 3. */
    {
        u32 col;

        for (col = 12u; col < 16u; col++)
        {
            projection_hardware.m[col] = ndsRendererRoundShiftS32Signed(
                projection_hardware.m[col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
        }
    }
    ndsRendererNativeBuildHierarchyHardwareAffine(
        sNdsNativeStageOwnerExecution.camera_modelview, &camera_hardware);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&camera_hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
    sNdsNativeStageOwnerExecution.task36_binding = UINT_MAX;
    sNdsNativeStageOwnerExecution.task36_coordinate_shift = UINT_MAX;
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = TRUE;
    return TRUE;
}

static s32 ndsRendererNativeStageTask36EnsureWorld(
    u32 binding_index, u32 coordinate_shift)
{
    m4x4 world_hardware;

    if ((sNdsNativeStageOwnerExecution.task36_segment_active == FALSE) ||
        (sNdsNativeStageOwnerExecution.binding_world == NULL) ||
        (ndsRendererNativeStageTask36BindingIsRigid(binding_index) == FALSE))
    {
        return FALSE;
    }
    /* The loaded-world key is exact: the rigid binding table is immutable for
     * this prepared frame and coordinate_shift is the only transform applied
     * by BuildWorld.  The old order rebuilt/copy-shifted the same 64-byte world
     * before checking this key, then discarded it on a hit.  Check the key
     * first so consecutive runs/triangles on the same rigid binding pay no CPU
     * matrix work at all. */
    if ((sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE) &&
        (sNdsNativeStageOwnerExecution.task36_binding == binding_index) &&
        (sNdsNativeStageOwnerExecution.task36_coordinate_shift ==
         coordinate_shift))
    {
        return TRUE;
    }
    if (ndsRendererNativeStageTask36BuildWorld(
            binding_index, coordinate_shift, &world_hardware) == FALSE)
    {
        return FALSE;
    }
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    if (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE)
    {
        glPopMatrix(1);
    }
    glPushMatrix();
    glMultMatrix4x4(&world_hardware);
    sNdsNativeStageOwnerExecution.task36_binding = binding_index;
    sNdsNativeStageOwnerExecution.task36_coordinate_shift = coordinate_shift;
    sNdsNativeStageOwnerExecution.task36_local_pushed = TRUE;
    sNdsRendererHardwareMatrixMode =
        NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE;
    sNdsRendererHardwareMatrixGeneration = ndsRendererNextMatrixGeneration();
    sNdsRendererHardwareMatrixLoaded = TRUE;
    return TRUE;
}


static void ndsRendererNativeStageTask36LoadNoZProjection(s16 projected_z)
{
    NDSRendererMatrix20p12 projection =
        *sNdsNativeStageOwnerExecution.projection;
    m4x4 projection_hardware;
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        projection.m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)projection.m[row][3] * projected_z, 12u);
    }
    ndsRendererCopyMtx20p12ToM4x4(&projection, &projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hardware);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    ndsRendererProfileRecordMatrixLoad();
}

static void ndsRendererNativeStageTask36EndSegment(void)
{
    if (sNdsNativeStageOwnerExecution.task36_segment_active == FALSE)
    {
        return;
    }
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    if (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE)
    {
        glPopMatrix(1);
    }
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = FALSE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
}

/* always_inline (2026-10-04): its one call site is the commit loop, where GCC
 * inlined it until the segment fast path grew that function past the
 * called-once growth limit; the out-of-line copy then landed in .itcm (2 KB)
 * and overflowed it. */
static inline __attribute__((always_inline)) s32 NDS_R2_ITCM_PACK2_CODE
ndsRendererNativeStageBeginRun(
    const NDSNativeStageRun *native_run,
    const NDSNativeStagePreparedRun *run,
    u32 submit_class,
    u32 segment_owner,
    NDSRendererStats *stats)
{
    u32 poly_fmt;

    if ((native_run == NULL) || (stats == NULL) || (run == NULL))
    {
        return FALSE;
    }
    /* Commit proves this prepared table before its first GX write. */
    if (ndsRendererNativeStagePreparedTextureProofCurrent() == FALSE)
    {
        if (ndsRendererNativeStagePreparedTextureValid(run) == FALSE)
        {
            return FALSE;
        }
    }
    poly_fmt = run->poly_fmt;

    /* Dream Land's four static layer owners are closed front-facing stage
     * surfaces.  Keep actor owners (Whispy and flowers) two-sided. */
    if ((sNdsNativeStagePacketActive->gkind == NDS_NATIVE_STAGE_GKIND_PUPUPU) &&
        (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) &&
        (segment_owner < NDS_RENDERER_NATIVE_STAGE_STATIC_OWNER_COUNT) &&
        ((poly_fmt & POLY_CULL_NONE) == POLY_CULL_NONE))
    {
        poly_fmt &= ~((u32)POLY_CULL_NONE);
        poly_fmt |= POLY_CULL_BACK;
    }
    ndsRendererHardwareEndBatch();
    if (ndsRendererNativeStageTask36BindingIsRigid(
            native_run->binding_index) != FALSE)
    {
        u32 coordinate_shift =
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                (u32)run->coordinate_shift :
            (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ?
                ndsRendererNativeStageTask36TriangleShift(native_run, 0u) : 0u;

        if ((sNdsNativeStageOwnerExecution.task36_segment_active == FALSE) &&
            (ndsRendererNativeStageTask36BeginSegment() == FALSE))
        {
            return FALSE;
        }
        if (ndsRendererNativeStageTask36EnsureWorld(
                native_run->binding_index, coordinate_shift) == FALSE)
        {
            return FALSE;
        }
    }
    else
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        /* Each run owns its matrix. A shared literal generation would let
         * the hardware matrix cache reuse the preceding binding's transform. */
        ndsRendererLoadHardwareRawComposedMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[
                native_run->binding_index], ndsRendererNextMatrixGeneration());
    }
    else if (submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)
    {
        NDSRendererMatrix20p12 projection;
        NDSRendererMatrix20p12 modelview;
        /* The same shift the run's vertices are packed under (the emit at
         * ndsRendererNativeStageEmitVertex, `run->coordinate_shift` or
         * 1). A literal 1 here left the matrix compensating one bit while
         * Hyrule's shift-2/3 runs shed two or three, so that geometry drew at
         * a half or a quarter of its size (owner, 2026-09-07). The rigid
         * Task36 arm above already passes the run's shift. */
        u32 coordinate_shift = (run->coordinate_shift != 0u) ?
            (u32)run->coordinate_shift : 1u;

        if (ndsRendererBuildShiftedRawHardwareMatrix(
                &sNdsNativeStageOwnerExecution.binding_composed[
                    native_run->binding_index], &modelview,
                coordinate_shift) == FALSE)
        {
            return FALSE;
        }
        ndsRendererMtxIdentity20p12(&projection);
        ndsRendererLoadHardwareMatrixPair(
            &projection, &modelview, NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED,
            ndsRendererNextMatrixGeneration(), TRUE);
    }
    else
    {
        /* The actor runs land here. Every triangle of such a run then goes
         * through Task 51's EnsureWorld + LoadNoZProjection, which replaces
         * both matrices -- so measure whether this load survives to a vertex
         * at all. */
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
    }

    glEnable(GL_TEXTURE_2D);
    if (run->textured != 0u)
    {
        run->texture_entry->last_used_frame =
            sNdsRendererHardwareFrameSerial + 1u;
        run->texture_entry->params = run->texture_params;
        ndsRendererHardwareBindTextureName(stats, run->texture_name);
        ndsRendererHardwareApplyTextureParams(run->texture_params);
        sNdsRendererHardwareActiveTextureEntry = run->texture_entry;
        if (run->texture_entry->pinned != 0u)
        {
            ndsRendererHardwareRecordBattleStaticTextureHit(
                run->texture_entry);
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = run->texture_format;
        stats->hardware_texture_width = run->texture_width;
        stats->hardware_texture_height = run->texture_height;
    }
    else
    {
        ndsRendererHardwareBindNoTexture(NULL);
    }
    if (run->alpha_test != 0u)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(run->alpha_ref);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
    glDisable(GL_FOG);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();
    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = run->textured;
    sNdsRendererHardwareTriangleBatchTextureName = run->texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = run->alpha_test;
    sNdsRendererHardwareTriangleBatchFogKey = 0u;
    sNdsRendererHardwareTriangleBatchMatrixMode =
        (ndsRendererNativeStageTask36BindingIsRigid(
             native_run->binding_index) != FALSE) ?
            NDS_RENDERER_HW_MATRIX_MODE_STAGE_HW_COMPOSE :
        ((submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ||
         (submit_class ==
          NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX)) ?
            NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED :
            NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY;
    sNdsRendererHardwareTriangleBatchMatrixGeneration =
        (ndsRendererNativeStageTask36BindingIsRigid(
             native_run->binding_index) != FALSE) ?
            sNdsRendererHardwareMatrixGeneration :
        (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ?
            1u :
        (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
            2u : 0u;

    return TRUE;
}


static inline void ndsRendererNativeStageEmitVertex(
    const NDSNativeStageDenseVertex *dense,
    const NDSNativeStagePreparedDense *prepared,
    const NDSNativeStagePreparedRun *run,
    u32 submit_class)
{
    ndsRendererNativeStageWriteColor(prepared->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)prepared->s | ((u32)(u16)prepared->t << 16));
    }
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererNativeStageWriteVertex16(
            (u32)(u16)((s32)dense->x *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) |
            ((u32)(u16)((s32)dense->y *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) << 16),
            (u16)((s32)dense->z *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
    }
    else
    {
        u32 shift = (run->coordinate_shift != 0u) ?
            (u32)run->coordinate_shift : 1u;

        ndsRendererNativeStageWriteVertex16(
            (u32)(u16)(ndsRendererNativeStageVertexShift(dense->x, shift) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) |
            ((u32)(u16)(ndsRendererNativeStageVertexShift(dense->y, shift) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))) << 16),
            (u16)(ndsRendererNativeStageVertexShift(dense->z, shift) *
                (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
    }
}

/* A layer's entry geometry state. The packet byte carries only the
 * G_ZBUFFER bit (1 on display link 6); the RSP reset list every source
 * task starts from also sets G_CULL_BACK, and since 2026-09-07 the generator
 * walks every layer from that baseline (GEOMETRY_LAYER_ENTRY), so the
 * policies it records carry the cull bit and the runtime state must start
 * from the same word or run 0 fails the policy match (PrepareRun step 1,
 * hyrule-c2). */
static inline u32 ndsNativeStageSegmentEntryGeometry(u32 initial_geometry)
{
    /* Bit 0 = G_ZBUFFER (link 6), bit 1 = G_CULL_BACK (every packet except
     * Dream Land's frozen one, which the Pupupu no-Z force below covers).
     * Callers pass segment->initial_geometry itself so the consumed-field
     * census still sees the read in ndsRendererPrepareNativeStageOwner. */
    return (((initial_geometry & 1u) != 0u) ? NDS_RENDERER_GEOM_ZBUFFER : 0u) |
           (((initial_geometry & 2u) != 0u) ? NDS_RENDERER_GEOM_CULL_BACK : 0u);
}

static void ndsRendererNativeStageSetNoZColumn(
    NDSRendererMatrix20p12 *matrix,
    s16 projected_z)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        matrix->m[row][2] = (s32)ndsRendererRoundShiftS64(
            (s64)matrix->m[row][3] * projected_z, 12u);
    }
}

/* The stage select's preview (battleship_mnmaps.c) draws a packet whose
 * no-Z runs are the battle's: mnMapsModelPriProcDisplay and
 * mnMapsModelSecProcDisplay (mnmaps.c:970-990) set G_ZBUFFER and a ZB render
 * mode before every layer, so the source preview depth-tests the layers a
 * battle paints without Z (Saffron's layers 0 and 3). Painted, Saffron's
 * Silph front (layer 3) covered its roof walkway and the garage roof
 * (owner, 2026-10-12). While the preview is armed those runs keep their
 * depth. */
extern volatile u32 gNdsRendererAdapterStagePreviewArmed;

/* Keep the bounded no-Z matrix setup in main RAM. Expanded fighter owners
 * put the four-CPU build 336 bytes past the hard ITCM limit (2026-09-06);
 * this 512-byte helper returns room without moving the per-vertex loop.
 * Its math and GX writes are unchanged; cadence is checked by that arm. */
static void __attribute__((noinline)) ndsRendererNativeStageLoadNoZMatrix(
    u32 binding_index,
    u32 coordinate_shift,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix;
    m4x4 hardware;

    if (coordinate_shift == 0u)
    {
        ndsRendererBuildRawHardwareMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[binding_index],
            &matrix);
    }
    else
    {
        (void)ndsRendererBuildShiftedRawHardwareMatrix(
            &sNdsNativeStageOwnerExecution.binding_composed[binding_index],
            &matrix, coordinate_shift);
    }
    if (gNdsRendererAdapterStagePreviewArmed == 0u)
    {
        ndsRendererNativeStageSetNoZColumn(&matrix, projected_z);
    }
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;
}

/* 2026-09-27: out of ITCM (old path only; the four-CPU census never
 * executed it). Its bytes went to the census's small high-stall admissions. */
static void ndsRendererNativeStageEmitNoZVertex(
    const NDSNativeStageDenseVertex *dense,
    const NDSNativeStagePreparedDense *prepared,
    const NDSNativeStagePreparedRun *run,
    u32 coordinate_shift)
{
    s32 x = ndsRendererNativeStageVertexShift(dense->x, coordinate_shift);
    s32 y = ndsRendererNativeStageVertexShift(dense->y, coordinate_shift);
    s32 z = ndsRendererNativeStageVertexShift(dense->z, coordinate_shift);

    ndsRendererNativeStageWriteColor(prepared->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)prepared->s | ((u32)(u16)prepared->t << 16));
    }
    ndsRendererNativeStageWriteVertex16(
        (u32)(u16)(x * 16) | ((u32)(u16)(y * 16) << 16),
        (u16)(z * 16));
}

static void ndsRendererNativeStageEmitClippedVertex(
    const NDSRendererProjectedClipVertex *vertex,
    const NDSNativeStagePreparedRun *run,
    s16 projected_z)
{
    NDSRendererMatrix20p12 matrix;
    m4x4 hardware;

    memset(&matrix, 0, sizeof(matrix));
    matrix.m[3][0] = ndsRendererRoundShiftS32Signed(vertex->clip.x, 8u);
    matrix.m[3][1] = ndsRendererRoundShiftS32Signed(vertex->clip.y, 8u);
    matrix.m[3][3] = ndsRendererRoundShiftS32Signed(vertex->clip.w, 8u);
    if (gNdsRendererAdapterStagePreviewArmed != 0u)
    {
        matrix.m[3][2] = ndsRendererRoundShiftS32Signed(vertex->clip.z, 8u);
    }
    else
    {
        ndsRendererNativeStageSetNoZColumn(&matrix, projected_z);
    }
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;

    ndsRendererNativeStageWriteColor(vertex->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)vertex->s | ((u32)(u16)vertex->t << 16));
    }
    ndsRendererNativeStageWriteVertex16(0u, 0u);
}

/* A small native-only companion to the no-Z projected emitter. Inishie's
 * string quads intentionally share triangles across adjacent source DObjs:
 * the upper two corners keep one DObj matrix while the lower two corners use
 * the live string DObj translation. A single raw GX matrix cannot represent
 * that triangle, and flattening the vertices would freeze the moving string.
 * Project each corner with its own live binding matrix, then load its clip
 * x/y/z/w as a one-vertex matrix. Unlike the no-Z path, the clip z is kept in
 * the matrix translation so link-6 depth behavior remains source-owned. */
static void ndsRendererNativeStageProjectedDepthMatrix(
    const NDSRendererClipVertex20p12 *clip, NDSRendererMatrix20p12 *matrix)
{
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[3][0] = ndsRendererRoundShiftS32Signed(clip->x, 8u);
    matrix->m[3][1] = ndsRendererRoundShiftS32Signed(clip->y, 8u);
    matrix->m[3][2] = ndsRendererRoundShiftS32Signed(clip->z, 8u);
    matrix->m[3][3] = ndsRendererRoundShiftS32Signed(clip->w, 8u);
}

static void ndsRendererNativeStageEmitProjectedDepthVertex(
    const NDSRendererProjectedClipVertex *vertex,
    const NDSNativeStagePreparedRun *run)
{
    NDSRendererMatrix20p12 matrix;
    m4x4 hardware;

    ndsRendererNativeStageProjectedDepthMatrix(&vertex->clip, &matrix);
    ndsRendererCopyMtx20p12ToM4x4(&matrix, &hardware);
    glLoadMatrix4x4(&hardware);
    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixLoaded = FALSE;

    ndsRendererNativeStageWriteColor(vertex->packed_color);
    if (run->textured != 0u)
    {
        ndsRendererNativeStageWriteTexCoord(
            (u32)(u16)vertex->s | ((u32)(u16)vertex->t << 16));
    }
    ndsRendererNativeStageWriteVertex16(0u, 0u);
}

/* Out of line and out of ITCM on purpose. Source-Z cross runs (Inishie's
 * scale platforms) reach this branch; no-Z cross runs use their painter path.
 * This is a rare branch, and letting it
 * inline into ndsRendererCommitNativeStageSegment grew that ITCM function
 * by 552 bytes and overflowed the 32 KiB region by 448. Same treatment the
 * near-plane clipper below already has. */
static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererNativeStageEmitCrossMatrixTriangle(
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared_run,
    u32 triangle_offset)
{
    NDSRendererProjectedClipVertex input[3];
    NDSRendererProjectedClipVertex clipped[4];
    u32 corner_offset;
    u32 clipped_count;
    u32 fan_index;
    u32 emitted = 0u;

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[
            (u32)run->first_corner + triangle_offset * 3u + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];
        NDSRendererInputVertex source;

        ndsRendererNativeStageInputVertex(dense, &source);
        ndsRendererTransformVertex20p12(
            &sNdsNativeStageOwnerExecution.binding_composed[
                dense->matrix_binding],
            &source, &input[corner_offset].clip);
        input[corner_offset].s = prepared->s;
        input[corner_offset].t = prepared->t;
        input[corner_offset].packed_color = prepared->packed_color;
    }
    clipped_count = ndsRendererHardwareClipTriangleNearPlane(input, clipped);
    if (clipped_count < 3u)
    {
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return 0u;
    }
    for (fan_index = 1u; fan_index + 1u < clipped_count; fan_index++)
    {
        if ((ndsRendererRoundShiftS32Signed(clipped[0].clip.w, 8u) == 0) ||
            (ndsRendererRoundShiftS32Signed(
                 clipped[fan_index].clip.w, 8u) == 0) ||
            (ndsRendererRoundShiftS32Signed(
                 clipped[fan_index + 1u].clip.w, 8u) == 0))
        {
            gNdsNativeStageNearFanZeroWCount++;
            continue;
        }
        ndsRendererNativeStageEmitProjectedDepthVertex(
            &clipped[0], prepared_run);
        ndsRendererNativeStageEmitProjectedDepthVertex(
            &clipped[fan_index], prepared_run);
        ndsRendererNativeStageEmitProjectedDepthVertex(
            &clipped[fan_index + 1u], prepared_run);
        emitted++;
    }
    gNdsNativeStageNearFanCount += emitted;
    return emitted;
}

static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererNativeStageEmitNearClippedTriangle(
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared_run,
    u32 triangle_offset,
    s16 projected_z)
{
    NDSRendererProjectedClipVertex input[3];
    NDSRendererProjectedClipVertex clipped[4];
    u32 corner_offset;
    u32 clipped_count;
    u32 fan_index;

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[
            (u32)run->first_corner + triangle_offset * 3u + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];
        NDSRendererInputVertex source;

        ndsRendererNativeStageInputVertex(dense, &source);
        ndsRendererTransformVertex20p12(
            &sNdsNativeStageOwnerExecution.binding_composed[
                dense->matrix_binding],
            &source, &input[corner_offset].clip);
        input[corner_offset].s = prepared->s;
        input[corner_offset].t = prepared->t;
        input[corner_offset].packed_color = prepared->packed_color;
    }
    clipped_count = ndsRendererHardwareClipTriangleNearPlane(input, clipped);
    if (clipped_count < 3u)
    {
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return 0u;
    }
    {
        u32 emitted = 0u;

        for (fan_index = 1u; fan_index + 1u < clipped_count; fan_index++)
        {
            /* Same intent as the generic fan guard in
             * ndsRendererHardwareSubmitNearClippedTriangle
             * (nds_renderer_textures_effects.c): a corner whose w rounds to
             * zero after the 8-bit shift the clipped-vertex matrix takes
             * would load a degenerate matrix and the hardware drops or
             * smears the triangle. Steep faces crossing the near plane
             * during the entry pan -- Peach Castle's central roof -- are the
             * case that reaches it (agents-0906/castle_roof_drawtime). */
            if ((ndsRendererRoundShiftS32Signed(clipped[0].clip.w, 8u) == 0) ||
                (ndsRendererRoundShiftS32Signed(clipped[fan_index].clip.w,
                                                8u) == 0) ||
                (ndsRendererRoundShiftS32Signed(clipped[fan_index + 1u].clip.w,
                                                8u) == 0))
            {
                gNdsNativeStageNearFanZeroWCount++;
                continue;
            }
            ndsRendererNativeStageEmitClippedVertex(
                &clipped[0], prepared_run, projected_z);
            ndsRendererNativeStageEmitClippedVertex(
                &clipped[fan_index], prepared_run, projected_z);
            ndsRendererNativeStageEmitClippedVertex(
                &clipped[fan_index + 1u], prepared_run, projected_z);
            emitted++;
        }
        gNdsNativeStageNearFanCount += emitted;
        return emitted;
    }
}

/* Exact GX witness for the Castle roof's rigid PROJECTED_NO_Z path.  Run 9 is
 * deliberately not part of the identity: the source DL root offset survives
 * generated-table reordering, while the runtime run index is recorded below.
 * The extra topology/class checks make a future split/regeneration fail stale
 * instead of silently measuring an unrelated run at the old index. */
static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererNativeStageCaptureCastleRoofClip(
    const NDSNativeStageRun *run,
    u32 triangle_offset,
    u32 coordinate_shift,
    s16 projected_z)
{
    const NDSNativeStageBinding *binding;
    u32 run_index;
    u32 expected_corner;
    u32 corner_offset;

    if ((run == NULL) || (sNdsNativeStagePacketActive == NULL) ||
        (sNdsNativeStagePacketActive->gkind != NDS_NATIVE_STAGE_GKIND_CASTLE) ||
        (run->binding_index >= NDS_NATIVE_STAGE_BINDING_COUNT) ||
        (run->submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ||
        (run->triangle_count != 9u) || (triangle_offset >= 9u))
    {
        return;
    }
    binding = &sNdsNativeStageBindings[run->binding_index];
    if (binding->root_offset != 0x00001698u)
    {
        return;
    }
    run_index = (u32)(run - sNdsNativeStageRuns);
    if (run_index >= NDS_NATIVE_STAGE_RUN_COUNT)
    {
        return;
    }

    expected_corner = triangle_offset * 3u;
    if (triangle_offset == 0u)
    {
        gNdsNativeStageCastleRoofClipValid = 0u;
        gNdsNativeStageCastleRoofClipRun = run_index;
        gNdsNativeStageCastleRoofClipCornerCount = 0u;
    }
    if ((gNdsNativeStageCastleRoofClipRun != run_index) ||
        (gNdsNativeStageCastleRoofClipCornerCount != expected_corner))
    {
        gNdsNativeStageCastleRoofClipValid = 0u;
        return;
    }

    gNdsNativeStageCastleRoofClipProjectedZ[triangle_offset] =
        (u32)(s32)projected_z;
    gNdsNativeStageCastleRoofClipShift[triangle_offset] = coordinate_shift;
    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 corner_index = expected_corner + corner_offset;
        u32 dense_index = sNdsNativeStageCorners[
            (u32)run->first_corner + corner_index];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        s16 x = (s16)(ndsRendererNativeStageVertexShift(
            dense->x, coordinate_shift) * 16);
        s16 y = (s16)(ndsRendererNativeStageVertexShift(
            dense->y, coordinate_shift) * 16);
        s16 z = (s16)(ndsRendererNativeStageVertexShift(
            dense->z, coordinate_shift) * 16);
        s32 clip_x;
        s32 clip_y;
        s32 clip_z;
        s32 clip_w;
        u32 result_base = corner_index * 4u;
        u32 submit_base = corner_index * 3u;
        u32 flags = 0u;

        gNdsNativeStageCastleRoofClipDense[corner_index] = dense_index;
        gNdsNativeStageCastleRoofClipSubmitV16[submit_base + 0u] =
            (u32)(s32)x;
        gNdsNativeStageCastleRoofClipSubmitV16[submit_base + 1u] =
            (u32)(s32)y;
        gNdsNativeStageCastleRoofClipSubmitV16[submit_base + 2u] =
            (u32)(s32)z;

        /* PosTest uses the current GX position/projection matrices and the
         * exact v16 values emitted immediately below, avoiding a CPU-side
         * reconstruction of the Task-36 fixed-point composition. */
        GFX_POS_TEST = (u32)(u16)x | ((u32)(u16)y << 16);
        GFX_POS_TEST = (u32)(s32)z;
        while ((GFX_STATUS & BIT(0)) != 0u)
        {
        }
        clip_x = GFX_POS_RESULT[0];
        clip_y = GFX_POS_RESULT[1];
        clip_z = GFX_POS_RESULT[2];
        clip_w = GFX_POS_RESULT[3];
        gNdsNativeStageCastleRoofClipResult[result_base + 0u] = (u32)clip_x;
        gNdsNativeStageCastleRoofClipResult[result_base + 1u] = (u32)clip_y;
        gNdsNativeStageCastleRoofClipResult[result_base + 2u] = (u32)clip_z;
        gNdsNativeStageCastleRoofClipResult[result_base + 3u] = (u32)clip_w;

        /* Homogeneous clip-plane flags: bit 0 zero-W, bit 1 negative-W,
         * then x<(-w), x>w, y<(-w), y>w, z<(-w), z>w in bits 2..7. */
        if (clip_w == 0)
        {
            flags |= 1u;
        }
        else if (clip_w < 0)
        {
            flags |= 2u;
        }
        else
        {
            s64 w = clip_w;

            flags |= ((s64)clip_x < -w) ? 4u : 0u;
            flags |= ((s64)clip_x > w) ? 8u : 0u;
            flags |= ((s64)clip_y < -w) ? 16u : 0u;
            flags |= ((s64)clip_y > w) ? 32u : 0u;
            flags |= ((s64)clip_z < -w) ? 64u : 0u;
            flags |= ((s64)clip_z > w) ? 128u : 0u;
        }
        gNdsNativeStageCastleRoofClipFlags[corner_index] = flags;
    }
    gNdsNativeStageCastleRoofClipCornerCount = expected_corner + 3u;
    if ((triangle_offset == 8u) &&
        (gNdsNativeStageCastleRoofClipCornerCount == 27u))
    {
        gNdsNativeStageCastleRoofClipValid = gNdsNativeStageRoofSnapSerial;
    }
}

static u32 __attribute__((noinline))
ndsRendererNativeStageEmitNoZTriangle(
    const NDSNativeStageRun *run,
    const NDSNativeStagePreparedRun *prepared_run,
    u32 triangle_offset,
    s16 projected_z)
{
    u32 first_corner = (u32)run->first_corner + triangle_offset * 3u;
    u32 inside_count = 0u;
    u32 corner_offset;
    u32 first_dense_index;
    u32 binding_index;
    u32 coordinate_shift = 0u;
    s32 one_binding = TRUE;

    if (ndsRendererNativeStageTask36BindingIsRigid(run->binding_index) != FALSE)
    {
        coordinate_shift = ndsRendererNativeStageTask36TriangleShift(
            run, triangle_offset);
        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 dense_index =
                sNdsNativeStageCorners[first_corner + corner_offset];

            if (sNdsNativeStageVertices[dense_index].matrix_binding !=
                run->binding_index)
            {
                gNdsNativeStageNoZForeignBindingCount++;
                return 0u;
            }
        }
        if (ndsRendererNativeStageTask36EnsureWorld(
                run->binding_index, coordinate_shift) == FALSE)
        {
            gNdsNativeStageNoZEnsureWorldCount++;
            return 0u;
        }
        ndsRendererNativeStageTask36LoadNoZProjection(projected_z);
        if (gNdsNativeStageCastleRoofClipArm != 0u)
        {
            ndsRendererNativeStageCaptureCastleRoofClip(
                run, triangle_offset, coordinate_shift, projected_z);
        }
        for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
        {
            u32 dense_index =
                sNdsNativeStageCorners[first_corner + corner_offset];

            ndsRendererNativeStageEmitNoZVertex(
                &sNdsNativeStageVertices[dense_index],
                &sNdsNativeStagePreparedDense[dense_index],
                prepared_run, coordinate_shift);
        }
        return 1u;
    }

    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];

        inside_count +=
            (sNdsNativeStagePreparedDense[dense_index].near_inside != FALSE) ?
                1u : 0u;
    }
    if (inside_count == 0u)
    {
        gNdsNativeStageNoZInsideCullCount++;
        ndsRendererProfileRecordNearPlaneTriangleReject();
        ndsRendererProfileRecordSubmitClass(NDS_RENDERER_HW_SUBMIT_REJECT);
        return 0u;
    }
    if (inside_count != 3u)
    {
        return ndsRendererNativeStageEmitNearClippedTriangle(
            run, prepared_run, triangle_offset, projected_z);
    }

    first_dense_index = sNdsNativeStageCorners[first_corner];
    binding_index = sNdsNativeStageVertices[first_dense_index].matrix_binding;
    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        u32 vertex_shift = dense->packed_cache_shift >>
            NDS_NATIVE_STAGE_COORDINATE_SHIFT;

        if (dense->matrix_binding != binding_index)
        {
            one_binding = FALSE;
        }
        if (vertex_shift > coordinate_shift)
        {
            coordinate_shift = vertex_shift;
        }
    }
    if (one_binding != FALSE)
    {
        ndsRendererNativeStageLoadNoZMatrix(
            binding_index, coordinate_shift, projected_z);
    }
    for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
    {
        u32 dense_index = sNdsNativeStageCorners[first_corner + corner_offset];
        const NDSNativeStageDenseVertex *dense =
            &sNdsNativeStageVertices[dense_index];
        const NDSNativeStagePreparedDense *prepared =
            &sNdsNativeStagePreparedDense[dense_index];
        u32 vertex_shift = (one_binding != FALSE) ?
            coordinate_shift :
            (dense->packed_cache_shift >>
             NDS_NATIVE_STAGE_COORDINATE_SHIFT);

        if (one_binding == FALSE)
        {
            ndsRendererNativeStageLoadNoZMatrix(
                dense->matrix_binding, vertex_shift, projected_z);
        }
        ndsRendererNativeStageEmitNoZVertex(
            dense, prepared, prepared_run, vertex_shift);
    }
    return 1u;
}




/* Which decline site of ndsRendererPrepareNativeStageOwner ran last (1-based,
 * source order of its goto done sites; 0 = accepted or never run) and the
 * segment index at that point. Companion of gNdsNativeStageValidateFullFailStep:
 * the adapter reports only reason 6 for this whole function. */
volatile u32 gNdsNativeStageOwnerPrepareFailStep;
volatile u32 gNdsNativeStageOwnerPrepareFailSegment;
volatile u32 gNdsNativeStageOwnerPrepareGuardMask;
/* Parked per-head preflight stats for Sec-callback layers (see the binding
 * loop in ndsRendererPrepareNativeStageOwner). */
static NDSRendererStats sNdsNativeStageHeadStats[2];

s32 ndsRendererPrepareNativeStageOwner(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    NDSRendererTraversalState *state =
        &sNdsNativeStageOwnerExecution.traversal;
    NDSNativeStageTopologySummary topology;
    /* Resolve the packet for the loaded stage kind exactly once per
     * preparation.  A kind with no baked packet declines here rather than
     * drawing another stage's geometry. */
    const s32 packet_selected = ndsRendererNativeStageSelectPacket();
    u64 epoch_mask = 0u;
    u32 segment_index = 0u;
    u32 current_head = 0u;
    u32 head_valid[2] = { FALSE, FALSE };
    u32 roof_snap_run_index;
    s32 accepted = FALSE;

    /* A fresh serial for this owner preparation makes a commit witness
     * frame-local even when the stage run table itself is reused. */
    gNdsNativeStageRoofSnapSerial++;
    if (gNdsNativeStageRoofSnapSerial == 0u)
    {
        gNdsNativeStageRoofSnapSerial = 1u;
    }
    for (roof_snap_run_index = 0u;
         (gNdsNativeStageRunWitness != 0u) &&
         (roof_snap_run_index < sNdsNativeStageWs.run_capacity);
         roof_snap_run_index++)
    {
        gNdsNativeStageRoofSnapValid[roof_snap_run_index] = 0u;
    }
    sNdsNativeStageEmitShortfallSnapshot.active = 0u;
    sNdsNativeStageWarmUploadsThisPrepare = 0u;
    if (gNdsNativeStageWarmUploadLimit != 0u)
    {
        sNdsNativeStageWarmPrepareStart = cpuGetTiming();
    }
    if (sNdsNativeStageFilterPhaseHeapGeneration != gNdsTaskmanHeapGeneration)
    {
        gNdsNativeStageFilterPhase8RunCount = 0u;
        gNdsNativeStageFilterPhase16RunCount = 0u;
        sNdsNativeStageFilterPhaseHeapGeneration = gNdsTaskmanHeapGeneration;
    }
    gNdsNativeStageFilterPhaseActivePacket =
        (u32)(uintptr_t)sNdsNativeStagePacketActive;
#if NDS_NATIVE_STAGE_MULTI
    gNdsNativeStageFilterPhaseBlobPacket =
        (u32)(uintptr_t)ndsNativeStageBlobPacket();
#else
    gNdsNativeStageFilterPhaseBlobPacket = 0u;
#endif

    if (packet_selected == FALSE)
    {
        sNdsNativeStageOwnerExecution.active = FALSE;
        return FALSE;
    }
    /* R2-02 E1a. Reuse the prepared run table when it was built for this exact
     * config and asset set. epoch_mask must be restored with it: PrepareRun
     * accumulates it and the Task 36 replay capture and segment-0 hash consume
     * it after the loop. */
    u32 r2_reuse =
        ((sNdsNativeStageOwnerExecution.r2_prepared_valid != 0u) &&
         (ndsRendererNativeStagePreparedTexturesProven() != FALSE) &&
         (sNdsNativeStageOwnerExecution.r2_prepared_topology_generation ==
          frame->topology_generation) &&
         (sNdsNativeStageOwnerExecution.r2_prepared_topology_stamp ==
          frame->topology_stamp) &&
         (sNdsNativeStageOwnerExecution.r2_prepared_config == frame->config) &&
         (memcmp(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases,
                 frame->asset_bases,
                 sizeof(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases))
          == 0)) ? 1u : 0u;

    if (r2_reuse != 0u)
    {
        epoch_mask = sNdsNativeStageOwnerExecution.r2_prepared_epoch_mask;
        NDS_DIAG(gNdsR2StagePrepareReuseCount++);
    }
    else
    {
        NDS_DIAG(gNdsR2StagePrepareBuildCount++);
        /* R2-07 leg A. The table about to be written is not the table the
         * standing proof was taken on, and a rebuild is not an epoch event, so
         * the proof must be dropped here or a fresh run would inherit its
         * predecessor's certificate. The next consult re-sweeps and re-stamps. */
        ndsRendererNativeStagePreparedTextureProofDrop();
    }

    sNdsNativeStageOwnerExecution.active = FALSE;
    sNdsNativeStageOwnerExecution.binding_composed = NULL;
    sNdsNativeStageOwnerExecution.hidden_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.projection = NULL;
    sNdsNativeStageOwnerExecution.camera_modelview = NULL;
    sNdsNativeStageOwnerExecution.binding_world = NULL;
    sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.task36_seen_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.task36_local_pushed = FALSE;
    sNdsNativeStageOwnerExecution.task36_segment_active = FALSE;
    if ((gNdsRendererFastRunMode !=
         NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE) ||
        (frame == NULL) || (stats == NULL) ||
        (frame->dobjs == NULL) ||
        (frame->binding_display_lists == NULL) ||
        (frame->projection == NULL) ||
        (frame->camera_modelview == NULL) ||
        (frame->binding_world == NULL) ||
        (frame->binding_composed == NULL) ||
        (frame->materials == NULL) || (frame->config == NULL) ||
        (NDS_NATIVE_STAGE_PRODUCTION_PACKET_ABI != 0x4d335031u) ||
        (ndsRendererNativeStageValidateTopology(frame, &topology) == FALSE))
    {
        /* Which of the guard's operands failed; bits in operand order. */
        u32 guard = 0u;

        guard |= (gNdsRendererFastRunMode !=
                  NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE) ? 1u : 0u;
        guard |= (frame == NULL) ? 2u : 0u;
        guard |= (stats == NULL) ? 4u : 0u;
        if (frame != NULL)
        {
            guard |= (frame->dobjs == NULL) ? 8u : 0u;
            guard |= (frame->binding_display_lists == NULL) ? 16u : 0u;
            guard |= (frame->projection == NULL) ? 32u : 0u;
            guard |= (frame->camera_modelview == NULL) ? 64u : 0u;
            guard |= (frame->binding_world == NULL) ? 128u : 0u;
            guard |= (frame->binding_composed == NULL) ? 256u : 0u;
            guard |= (frame->materials == NULL) ? 512u : 0u;
            guard |= (frame->config == NULL) ? 1024u : 0u;
        }
        guard |= (NDS_NATIVE_STAGE_PRODUCTION_PACKET_ABI != 0x4d335031u) ?
            2048u : 0u;
        guard |= (guard == 0u) ? 4096u : 0u; /* only the validator failed */
        gNdsNativeStageOwnerPrepareGuardMask = guard;
        gNdsNativeStageOwnerPrepareFailStep = 1u;
        gNdsNativeStageOwnerPrepareFailSegment = segment_index;
        goto done;
    }


    for (segment_index = 0u;
         segment_index < NDS_NATIVE_STAGE_SEGMENT_COUNT;
         segment_index++)
    {
        const NDSNativeStageSegment *segment =
            &sNdsNativeStageSegments[segment_index];
        u32 binding_offset;

        /* Prepared runs, epoch mask and sync count already have owners;
         * no replay snapshot consumes these state/traversal temporaries. */
        if (r2_reuse != 0u)
        {
            NDS_DIAG(gNdsR2StagePreflightElideCount++);
            continue;
        }
        ndsRendererInitStats(&sNdsNativeStageOwnerExecution.preflight_stats);
        sNdsNativeStageOwnerExecution.preflight_stats.geometry_mode =
            ndsNativeStageSegmentEntryGeometry(segment->initial_geometry);
        ndsRendererInitTraversalState(
            state, frame->config,
            &sNdsNativeStageOwnerExecution.preflight_stats,
            NULL, NULL, 0u);
        head_valid[0] = FALSE;
        head_valid[1] = FALSE;
        current_head = 0u;
        if ((segment_index == 0u) &&
            (NDS_NATIVE_STAGE_HAS_GENERATED_SEGMENT0 != 0u))
        {
            if (ndsRendererNativeStagePrepareGeneratedSegment0(
                    frame,
                    &sNdsNativeStageOwnerExecution.preflight_stats,
                    state, &epoch_mask) == FALSE)
            {
                gNdsNativeStageOwnerPrepareFailStep = 3u;
                gNdsNativeStageOwnerPrepareFailSegment = segment_index;
                goto done;
            }
            continue;
        }
        for (binding_offset = 0u;
             binding_offset < segment->binding_count;
             binding_offset++)
        {
            u32 binding_index = (u32)segment->first_binding + binding_offset;
            const NDSNativeStageBinding *binding =
                &sNdsNativeStageBindings[binding_index];
            u32 run_offset;
            /* A Sec-callback layer draws every DObj's head-0 list into one
             * DL stream and every head-1 list into another
             * (grdisplay.c:66-85), so RDP state persists per HEAD, not in
             * packet order, and the generator models each head as its own
             * chain from the segment baseline (states_by_head). The packet
             * interleaves the heads per DObj; switching heads here parks the
             * live stats and resumes the other head's chain. Layer packets
             * carry no head table and never switch (2026-09-07: Sector Z run
             * 4 inherited head 0's othermode and missed its policy). */
            u32 head = (NDS_NATIVE_STAGE_BINDING_HEADS != NULL) ?
                (u32)NDS_NATIVE_STAGE_BINDING_HEADS[binding_index] : 0u;

            if ((head != current_head) && (head < 2u))
            {
                sNdsNativeStageHeadStats[current_head] =
                    sNdsNativeStageOwnerExecution.preflight_stats;
                head_valid[current_head] = TRUE;
                if (head_valid[head] != FALSE)
                {
                    sNdsNativeStageOwnerExecution.preflight_stats =
                        sNdsNativeStageHeadStats[head];
                }
                else
                {
                    ndsRendererInitStats(
                        &sNdsNativeStageOwnerExecution.preflight_stats);
                    sNdsNativeStageOwnerExecution.preflight_stats
                        .geometry_mode = ndsNativeStageSegmentEntryGeometry(segment->initial_geometry);
                    sNdsNativeStageOwnerExecution.preflight_stats
                        .othermode_l = ndsNativeStageHeadEntryOtherModeL(
                            segment->link, head);
                }
                NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
                current_head = head;
            }

            for (run_offset = 0u;
                 run_offset < binding->run_count;
                 run_offset++)
            {
                u32 run_index = (u32)binding->first_run + run_offset;

                if (ndsRendererNativeStageApplyStateSpan(
                         &sNdsNativeStageStateSpans[run_index], frame,
                         &sNdsNativeStageOwnerExecution.preflight_stats,
                         state) == FALSE)
                {
                    gNdsNativeStageOwnerPrepareFailStep = 4u;
                    gNdsNativeStageOwnerPrepareFailSegment = segment_index;
                    goto done;
                }
                if (r2_reuse == 0u)
                if (ndsRendererNativeStagePrepareRun(
                        run_index, frame,
                        &sNdsNativeStageOwnerExecution.preflight_stats,
                        state, &epoch_mask) == FALSE)
                {
                    gNdsNativeStageOwnerPrepareFailStep = 6u;
                    gNdsNativeStageOwnerPrepareFailSegment = segment_index;
                    goto done;
                }
            }
            if (ndsRendererNativeStageApplyStateSpan(
                    &sNdsNativeStageStateSpans[
                        NDS_NATIVE_STAGE_RUN_COUNT + binding_index],
                    frame, &sNdsNativeStageOwnerExecution.preflight_stats,
                    state) == FALSE)
            {
                gNdsNativeStageOwnerPrepareFailStep = 7u;
                gNdsNativeStageOwnerPrepareFailSegment = segment_index;
                goto done;
            }
        }
    }
    if ((epoch_mask != (((u64)1u <<
                         NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT) - 1u)) ||
        (topology.raw_triangles != NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES) ||
        (topology.projected_no_z_triangles != NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES) ||
        (topology.projected_range_triangles != NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES) ||
        (topology.cross_runs != NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT) ||
        (topology.cross_triangles !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT) ||
        (topology.cross_foreign_corners !=
         NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT))
    {
        gNdsNativeStageOwnerPrepareFailStep = 13u;
        gNdsNativeStageOwnerPrepareFailSegment = segment_index;
        goto done;
    }

    sNdsNativeStageOwnerExecution.binding_composed = frame->binding_composed;
    sNdsNativeStageOwnerExecution.hidden_binding_mask = frame->hidden_binding_mask;
    sNdsNativeStageMatrixGen++;
    sNdsNativeStageOwnerExecution.projection = frame->projection;
    sNdsNativeStageOwnerExecution.camera_modelview = frame->camera_modelview;
    sNdsNativeStageOwnerExecution.binding_world = frame->binding_world;
    sNdsNativeStageOwnerExecution.rigid_binding_mask =
        frame->rigid_binding_mask;
    ndsRendererInitStats(stats);
    stats->command_count = NDS_NATIVE_STAGE_SOURCE_COMMAND_COUNT;
    stats->vertex_count = NDS_NATIVE_STAGE_SOURCE_VERTEX_COUNT;
    stats->vertex_command_count = NDS_NATIVE_STAGE_VERTEX_COMMAND_COUNT;
    stats->triangle_command_count = NDS_NATIVE_STAGE_TRIANGLE_COMMAND_COUNT;
    /* R2-02 E8: taken from the memo when the loop body was elided, so the
     * result does not depend on whether the last segment was replay-served. */
    stats->sync_command_count = (r2_reuse != 0u) ?
        sNdsNativeStageOwnerExecution.r2_prepared_sync_count :
        sNdsNativeStageOwnerExecution.preflight_stats.sync_command_count;
    sNdsNativeStageOwnerExecution.stats = stats;
    sNdsNativeStageOwnerExecution.committed_segments = 0u;
    sNdsNativeStageOwnerExecution.active = TRUE;
    accepted = TRUE;
    gNdsNativeStageWarmUploads = 0u;
    /* The table is only publishable as reusable once the whole owner prepare
     * has accepted -- a run that rejected mid-loop leaves runs[] torn. */
    sNdsNativeStageOwnerExecution.r2_prepared_config = frame->config;
    sNdsNativeStageOwnerExecution.r2_prepared_topology_generation =
        frame->topology_generation;
    sNdsNativeStageOwnerExecution.r2_prepared_topology_stamp =
        frame->topology_stamp;
    memcpy(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases,
           frame->asset_bases,
           sizeof(sNdsNativeStageOwnerExecution.r2_prepared_asset_bases));
    sNdsNativeStageOwnerExecution.r2_prepared_epoch_mask = epoch_mask;
    sNdsNativeStageOwnerExecution.r2_prepared_sync_count =
        stats->sync_command_count;
    sNdsNativeStageOwnerExecution.r2_prepared_valid = 1u;

done:
    if (accepted == FALSE)
    {
        sNdsNativeStageOwnerExecution.stats = NULL;
        sNdsNativeStageOwnerExecution.binding_composed = NULL;
        sNdsNativeStageOwnerExecution.projection = NULL;
        sNdsNativeStageOwnerExecution.camera_modelview = NULL;
        sNdsNativeStageOwnerExecution.binding_world = NULL;
        sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
        sNdsNativeStageOwnerExecution.committed_segments = 0u;
        sNdsNativeStageOwnerExecution.active = FALSE;
        /* Any fallback invalidates the table: runs[] may be torn, and the
         * reason it rejected may be exactly that the topology moved. */
        sNdsNativeStageOwnerExecution.r2_prepared_valid = 0u;
    }
    return accepted;
}

void ndsRendererResetNativeStageValidationCache(void)
{
    memset(&sNdsNativeStageValidationCache, 0,
           sizeof(sNdsNativeStageValidationCache));
}


/* The visibility mask changes every frame, while prepared runs stay valid.
 * Keep the 64-bit dynamic bit test out of ITCM: GCC expands a variable u64
 * shift to ten ARM instructions in the commit loop, overflowing the 32 KiB
 * region. The call site remains the per-run COMMIT gate; this helper only
 * selects the current bit from the latched frame mask. */
static s32 __attribute__((noinline, optimize("Os")))
ndsRendererNativeStageBindingHidden(const NDSNativeStageRun *run)
{
    u32 binding_index = run->binding_index;
    s32 hidden =
        ((sNdsNativeStageOwnerExecution.hidden_binding_mask &
          ((u64)1u << binding_index)) != 0u) ? TRUE : FALSE;

    if ((hidden == FALSE) && (gNdsNativeStageRunWitness != 0u))
    {
        /* This existing non-ITCM call is immediately before the diagnostic
         * card-cull gate. The gate itself moves none of these counters, and a
         * skipped run never publishes RoofSnapValid, so this is equivalent to
         * the post-card-cull entry snapshot without growing the ITCM commit
         * function. */
        sNdsNativeStageEmitShortfallSnapshot.active = 1u;
        sNdsNativeStageEmitShortfallSnapshot.run_index =
            (u32)(run - sNdsNativeStageRuns);
        sNdsNativeStageEmitShortfallSnapshot.near_reject_count =
            sNdsRendererRuntimeFrameSummary.near_plane_triangle_reject_count;
        sNdsNativeStageEmitShortfallSnapshot.submit_reject_count =
            sNdsRendererHardwareSubmitClassCounts[NDS_RENDERER_HW_SUBMIT_REJECT];
        sNdsNativeStageEmitShortfallSnapshot.no_z_inside_cull_count =
            gNdsNativeStageNoZInsideCullCount;
        sNdsNativeStageEmitShortfallSnapshot.near_fan_zero_w_count =
            gNdsNativeStageNearFanZeroWCount;
        sNdsNativeStageEmitShortfallSnapshot.near_fan_count =
            gNdsNativeStageNearFanCount;
        sNdsNativeStageEmitShortfallSnapshot.no_z_foreign_binding_count =
            gNdsNativeStageNoZForeignBindingCount;
        sNdsNativeStageEmitShortfallSnapshot.no_z_ensure_world_count =
            gNdsNativeStageNoZEnsureWorldCount;
    }
    return hidden;
}

/* These two diagnostic stores are on every committed run, but they are not
 * part of the zero-wait emit loop itself. Keep the witness at the same commit
 * point while serving its bookkeeping from cached main RAM; otherwise recent
 * stage diagnostics push the four-CPU build past the 32 KiB ITCM limit. */
static void __attribute__((noinline, optimize("Os")))
ndsRendererNativeStagePublishRunEmission(u32 run_index, u32 emitted_triangles)
{
    gNdsNativeStageRoofSnapEmitted[run_index] = emitted_triangles;
    gNdsNativeStageRoofSnapValid[run_index] = gNdsNativeStageRoofSnapSerial;
}

#include "nds_stage_gx.exec.inc"

/* 2026-09-27: out of ITCM. 4,252 B renting 1,309 cycles a byte in the
 * four-CPU census; the bytes went to the census's section D pack. */
s32 ndsRendererCommitNativeStageSegment(u32 segment_index)
{
    NDSRendererStats *stats = sNdsNativeStageOwnerExecution.stats;
    const NDSNativeStageSegment *segment;

    NDS_FIGHTER_PACKET_DMA_WAIT();
    u32 run_offset;
    u32 segment_triangles = 0u;
    /* P2-4n1 step 7. A DLLink packet's runs sit in DObj preorder with each
     * DObj's links in array order, which is the order the source PARSES them;
     * the source DRAWS them per display head, 0 then 2 then 1 then 3
     * (syTaskmanUpdateDLBuffers chains the heads in that order at every
     * camera-group boundary). Every run is self-contained by the time it is
     * committed -- its matrices come from its binding and its state from
     * prepared_run, both resolved at prepare -- so emitting the segment's runs
     * head by head is a pure reordering. Layer packets have no head array and
     * take exactly one pass in their original order. What this does NOT
     * reproduce is the interleave with other owners drawn in the same camera
     * group; that delta is recorded on P2-4n1 and is measured at acceptance. */
    static const u8 head_order[4] = { 0u, 2u, 1u, 3u };
    const u8 *binding_heads = NDS_NATIVE_STAGE_BINDING_HEADS;
    const u32 head_pass_count = (binding_heads != NULL) ? 4u : 1u;
    u32 head_pass;
    u32 stage_gx_segment;

    if (sNdsNativeStageOwnerExecution.active == FALSE)
    {
        return FALSE;
    }
    /* 2026-10-02: any not-yet-committed segment, not only the next index.
     * The packet orders segments by owner, but the runtime presents display
     * GObjs by link and creation: Saffron's gate (segment 3, link 6) is drawn
     * before layer3 (segment 2, link 17), and the strict cursor refused it
     * every frame -- the adapter swallowed the refusal, so the door never drew
     * and the garage read as always open. Every run is self-contained (the
     * head_order note above), so committing segments in presentation order
     * is the source's own order and a pure reordering of the packet's. */
    if ((segment_index >= NDS_NATIVE_STAGE_SEGMENT_COUNT) ||
        (((sNdsNativeStageOwnerExecution.committed_segments >>
           segment_index) & 1u) != 0u) ||
        (stats == NULL))
    {
        return TRUE;
    }
    segment = &sNdsNativeStageSegments[segment_index];
    /* The prepared table outlives the cache entries it names. Validate before
     * this segment's first GX or renderer-state write so a recycled slot falls
     * back as one source-owned segment, never as a mixed native/source segment
     * after earlier runs have already emitted. BeginRun repeats this check as
     * the defensive last gate at the point of use.
     *
     * R2-07 leg A: the per-segment sweep is now the whole-table proof, which is
     * one word compare while the epoch holds. It is deliberately WIDER than the
     * loop it replaces -- a broken run in any segment now rejects every segment
     * rather than only its own. That is the conservative direction (the whole
     * stage falls back to source together, which is exactly the mixed-segment
     * outcome the check exists to prevent), and on the measured arm it is
     * unreachable: the cache does not move during a match. */
    if (ndsRendererNativeStagePreparedTexturesProven() == FALSE)
    {
        return FALSE;
    }
    stage_gx_segment = ndsStageGxEligible(segment_index);
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_STAGE;
#if NDS_STAGE_GX_FAST_LIVE
    /* The whole segment in one patch pass and one DMA when it qualifies
     * (ndsStageGxCommitFast); the per-run loop below otherwise, which then
     * owns the segment's memos until the fast path re-proves them. */
    if (stage_gx_segment != FALSE)
    {
        /* A DLLink segment too: its program's words are laid out in the
         * head passes' order (compile_nds_stage_gx.py format 7). */
        if (
            (ndsStageGxCommitFast(segment_index, segment, stats,
                                  &segment_triangles) != FALSE))
        {
            goto stage_gx_fast_done;
        }
        ndsStageGxFastForget(segment_index);
    }
#endif
    for (head_pass = 0u; head_pass < head_pass_count; head_pass++)
    for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
    {
        u32 run_index = (u32)segment->first_run + run_offset;
        const NDSNativeStageRun *run = &sNdsNativeStageRuns[run_index];
        const NDSNativeStagePreparedRun *prepared_run =
            &sNdsNativeStageWs.runs[run_index];
        u32 emitted_triangles = 0u;
        u32 triangle_offset;
        if ((binding_heads != NULL) &&
            (binding_heads[run->binding_index] != head_order[head_pass]))
        {
            continue;
        }
        if (ndsRendererNativeStageBindingHidden(run) != FALSE)
        {
            continue;
        }

        if ((stage_gx_segment != FALSE) &&
            (ndsStageGxRunOutsideView(run_index) != FALSE))
        {
            continue;
        }
        if (stage_gx_segment != FALSE && ndsStageGxDraw(run_index, segment->owner, stats) != FALSE)
        {
            emitted_triangles = run->triangle_count;
            goto stage_account_run;
        }
        /* A cold clipped run follows all earlier compiled runs in source order. */
        ndsStageGxFlush();
        NDS_FIGHTER_PACKET_DMA_WAIT();
        if (ndsRendererNativeStageBeginRun(
                run, prepared_run, run->submit_class, segment->owner, stats) == FALSE)
        {
            sNdsStageGxInSegment = 0u;
            return TRUE;
        }
        for (triangle_offset = 0u;
             triangle_offset < run->triangle_count;
             triangle_offset++)
        {
            s16 no_z = (run->submit_class ==
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z) ?
                ndsRendererHardwareNextProjectedDepth() : 0;
            u32 corner_offset;

            if ((run->flags &
                 NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX) != 0u &&
                run->submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                /* Cross binding describes vertex ownership, not depth mode.
                 * Dream Land's flowers must keep their no-Z painter path;
                 * source-Z projection is needed by Inishie's scale strings. */
                emitted_triangles +=
                    ndsRendererNativeStageEmitCrossMatrixTriangle(
                        run, prepared_run, triangle_offset);
                ndsRendererHardwareEnterProjectedForeground();
                continue;
            }
            if (run->submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                emitted_triangles += ndsRendererNativeStageEmitNoZTriangle(
                    run, prepared_run, triangle_offset, no_z);
                continue;
            }

            for (corner_offset = 0u; corner_offset < 3u; corner_offset++)
            {
                u32 dense_index = sNdsNativeStageCorners[
                    (u32)run->first_corner + triangle_offset * 3u +
                    corner_offset];
                const NDSNativeStageDenseVertex *dense =
                    &sNdsNativeStageVertices[dense_index];
                const NDSNativeStagePreparedDense *prepared =
                    &sNdsNativeStagePreparedDense[dense_index];
                ndsRendererNativeStageEmitVertex(
                    dense, prepared, prepared_run, run->submit_class);
            }
            emitted_triangles++;
            if (run->submit_class !=
                NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
            {
                ndsRendererHardwareEnterProjectedForeground();
            }
        }
        ndsRendererHardwareEndBatch();
stage_account_run:
        if (gNdsNativeStageRunWitness != 0u)
        {
            ndsRendererNativeStagePublishRunEmission(run_index, emitted_triangles);
        }
        ndsRendererNativeStageAccountRun(
            stats,
            (run->submit_class ==
             NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX) ?
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX :
                run->submit_class,
            emitted_triangles);
        stats->triangle_count += run->triangle_count;
        segment_triangles += run->triangle_count;
    }
#if NDS_STAGE_GX_FAST_LIVE
stage_gx_fast_done:
#endif
    ndsStageGxFlush();
    sNdsStageGxInSegment = 0u;
#if NDS_LAB_STAGE_GX_HASH
    /* LAB: the segment's closing painter state and counters (see
     * gNdsLabStageGxHash). */
    ndsLabStageGxHashWord((u32)sNdsRendererHardwareProjectedDepth);
    ndsLabStageGxHashWord(sNdsRendererHardwareProjectedBackground);
    ndsLabStageGxHashWord(segment_triangles);
    ndsLabStageGxHashWord(stats->hardware_triangle_count);
    ndsLabStageGxHashWord(stats->hardware_zbuffer_triangle_count);
    ndsLabStageGxHashWord(stats->hardware_texture_ready_count);
#endif
    ndsRendererNativeStageTask36EndSegment();
    sNdsRendererFastRunCount += segment->run_count;
    sNdsRendererFastTriangleCount += segment_triangles;
    sNdsRendererFastOwnerTriangleCount[
        NDS_RENDERER_PROFILE_OWNER_STAGE] += segment_triangles;
    sNdsNativeStageOwnerExecution.committed_segments |=
        (u32)1u << segment_index;
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
    return TRUE;
}

void ndsRendererFinishNativeStageOwner(void)
{
    if (sNdsNativeStageOwnerExecution.active != FALSE)
    {
        ndsRendererNativeStageTask36EndSegment();
        ndsRendererHardwareEndBatch();
        if ((sNdsNativeStageOwnerExecution.committed_segments !=
             (((u32)1u << NDS_NATIVE_STAGE_SEGMENT_COUNT) - 1u)) ||
            (sNdsNativeStageOwnerExecution.stats == NULL) ||
            (sNdsNativeStageOwnerExecution.stats->triangle_count !=
             NDS_NATIVE_STAGE_TRIANGLE_COUNT))
        {
        }
    }
    sNdsNativeStageOwnerExecution.stats = NULL;
    sNdsNativeStageOwnerExecution.binding_composed = NULL;
    sNdsNativeStageOwnerExecution.hidden_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.projection = NULL;
    sNdsNativeStageOwnerExecution.camera_modelview = NULL;
    sNdsNativeStageOwnerExecution.binding_world = NULL;
    sNdsNativeStageOwnerExecution.rigid_binding_mask = 0u;
    sNdsNativeStageOwnerExecution.committed_segments = 0u;
    sNdsNativeStageOwnerExecution.active = FALSE;
    sNdsRendererRuntimeOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
}
