/* The renderer's failure record and dispatch state: the first native
 * failure (gNdsRendererNativeFailure, written out of the cache only when a
 * failure happens), the lab's first sixteen distinct causes, the hardware
 * path's per-frame consume and source-cache resets, and the fighter draw's
 * texel-alpha bracket. A slice of nds_renderer.c. */

volatile NDSRendererNativeFailure gNdsRendererNativeFailure
    __attribute__((aligned(32)));
_Static_assert(sizeof(NDSRendererNativeFailure) == 32,
               "Native failure record must own one cache line");

#if defined(NDS_TICK_HUD) && NDS_TICK_HUD
/* P2-2p8 Phase 1 slice 2c (lab): the record above keeps only the FIRST
 * failure, so a count can hide several causes. The first 16 distinct
 * (identity, root, reason) with their counts and first frame. 2026-09-30:
 * keyed on the root, not the status: the stage domain's status is the gkind
 * (one value a run), and an owner is written per root. */
volatile u32 gNdsNativeFailureLabCount __attribute__((used));
volatile u32 gNdsNativeFailureLab[16][5] __attribute__((used));
#endif

void ndsRendererRecordNativeFailure(u32 domain, u32 scene, u32 identity,
    u32 status, u32 root, u32 material, u32 reason)
{
#if defined(NDS_TICK_HUD) && NDS_TICK_HUD
    {
        u32 n = gNdsNativeFailureLabCount;
        u32 i;

        for (i = 0u; (i < n) && (i < 16u); i++)
        {
            if ((gNdsNativeFailureLab[i][0] == identity) &&
                (gNdsNativeFailureLab[i][1] == root) &&
                (gNdsNativeFailureLab[i][2] == reason))
            {
                gNdsNativeFailureLab[i][3]++;
                break;
            }
        }
        if ((i == n) && (n < 16u))
        {
            gNdsNativeFailureLab[n][0] = identity;
            gNdsNativeFailureLab[n][1] = root;
            gNdsNativeFailureLab[n][2] = reason;
            gNdsNativeFailureLab[n][3] = 1u;
            gNdsNativeFailureLab[n][4] = gNdsRendererProfileFrameCount;
            gNdsNativeFailureLabCount = n + 1u;
        }
#if defined(ARM9)
        DC_FlushRange((const void *)gNdsNativeFailureLab,
                      sizeof(gNdsNativeFailureLab));
        DC_FlushRange((const void *)&gNdsNativeFailureLabCount,
                      sizeof(gNdsNativeFailureLabCount));
#endif
    }
#endif
    if (gNdsRendererNativeFailure.count == 0u)
    {
        gNdsRendererNativeFailure.domain = domain;
        gNdsRendererNativeFailure.scene = scene;
        gNdsRendererNativeFailure.identity = identity;
        gNdsRendererNativeFailure.status = status;
        gNdsRendererNativeFailure.root = root;
        gNdsRendererNativeFailure.material = material;
        gNdsRendererNativeFailure.reason = reason;
    }
    if (gNdsRendererNativeFailure.count != 0xffffffffu)
    {
        gNdsRendererNativeFailure.count++;
    }
#if defined(ARM9)
    /* Publish only on failure. Debugger reads must not lose the first cause
     * behind ARM9 data cache, and normal rendering pays no flush cost. */
    DC_FlushRange((const void *)&gNdsRendererNativeFailure,
                  sizeof(gNdsRendererNativeFailure));
#endif
}

volatile NDSRendererNativeDirectReject gNdsRendererNativeDirectReject
    __attribute__((aligned(32)));
_Static_assert(sizeof(NDSRendererNativeDirectReject) == 32,
               "Native direct-reject record must own one cache line");

void ndsRendererRecordNativeDirectReject(u32 site,
    const NDSRendererStats *stats)
{
    if (gNdsRendererNativeDirectReject.count == 0u)
    {
        gNdsRendererNativeDirectReject.site = site;
        if (stats != NULL)
        {
            gNdsRendererNativeDirectReject.othermode_l = stats->othermode_l;
            gNdsRendererNativeDirectReject.combine_w0 =
                stats->texture_combine_w0;
            gNdsRendererNativeDirectReject.combine_w1 =
                stats->texture_combine_w1;
            gNdsRendererNativeDirectReject.env_color = stats->env_color;
            gNdsRendererNativeDirectReject.geometry_mode =
                stats->geometry_mode;
            gNdsRendererNativeDirectReject.command_count =
                stats->command_count;
        }
    }
    if (gNdsRendererNativeDirectReject.count != 0xffffffffu)
    {
        gNdsRendererNativeDirectReject.count++;
    }
#if defined(ARM9)
    DC_FlushRange((const void *)&gNdsRendererNativeDirectReject,
                  sizeof(gNdsRendererNativeDirectReject));
#endif
}

void ndsRendererInitStats(NDSRendererStats *stats)
{
    if (stats != NULL)
    {
        memset(stats, 0, sizeof(*stats));
        stats->geometry_mode = NDS_RENDERER_GEOM_RESET_MODE;
        stats->othermode_h = NDS_RENDERER_TP_PERSP | NDS_RENDERER_TF_BILERP;
        stats->texture_source_hash1 = 2166136261u;
        stats->texture_source_hash2 = 0x9e3779b9u;
    }
}

void ndsRendererInitVertexCache(NDSRendererVertexCache *vertex_cache)
{
    if (vertex_cache == NULL)
    {
        return;
    }

    vertex_cache->input_valid_mask = 0u;
    vertex_cache->raw_vertex_fit_mask = 0u;
    vertex_cache->transformed_valid_mask = 0u;
    vertex_cache->vertex_color_valid_mask = 0u;
    vertex_cache->matrix_snapshot_count = 0u;
    memset(vertex_cache->vertex_matrix_snapshot, 0,
           sizeof(vertex_cache->vertex_matrix_snapshot));
    memset(vertex_cache->vertex_clip_snapshot, 0,
           sizeof(vertex_cache->vertex_clip_snapshot));
}



void ndsRendererHardwareResetSourceCaches(void)
{
    memset(sNdsRendererStageTextureSites, 0,
           sizeof(sNdsRendererStageTextureSites));
    sNdsRendererStageTextureSiteNext = 0u;
    memset(sNdsRendererHardwareCi4IndexCache, 0,
           sizeof(sNdsRendererHardwareCi4IndexCache));
    sNdsRendererHardwareCi4IndexCacheNext = 0u;
}

void ndsRendererHardwareSetNoOracle(u32 enabled)
{
    sNdsRendererHardwareNoOracle = (enabled != 0u) ? TRUE : FALSE;
}

/* The fighter draw's bracket for sNdsRendererHardwareTexelAlphaGraded
 * (renderer_adapter_fighter.c): set across one fighter's draw, cleared after,
 * so no effect, item or stage texture sees the request. */
void ndsRendererHardwareSetTexelAlphaGraded(u32 enabled)
{
    sNdsRendererHardwareTexelAlphaGraded = (enabled != 0u) ? 1u : 0u;
}

u32 ndsRendererHardwareNoOracleEnabled(void)
{
    return sNdsRendererHardwareNoOracle;
}

void ndsRendererProfileSetOwner(NDSRendererProfileOwner owner)
{
    u32 mode = gNdsRendererFastRunMode;

    sNdsRendererRuntimeOwner =
        ((u32)owner < (u32)NDS_RENDERER_PROFILE_OWNER_COUNT) ? owner :
        NDS_RENDERER_PROFILE_OWNER_NONE;
    /* G1: mode 9 (NATIVE_COMPLETE_STAGE) was added after this list and never
     * joined it, so the memo has never run in any measured ROM. Route 1 adds
     * it; route 0 leaves the shipped condition untouched. */
    sNdsRendererStageTextureSitesEnabled =
        (((mode == NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES) ||
          (mode == NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ||
          (mode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) ||
          ((mode == NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE) &&
           (gNdsG1SiteCacheRoute != 0u))) &&
         (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_STAGE)) ?
            TRUE : FALSE;
    sNdsRendererFastOwnerEnabled =
        ((mode == NDS_RENDERER_FAST_RUN_MARIO_ONLY) &&
         (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_MARIO)) ||
        ((mode == NDS_RENDERER_FAST_RUN_FIGHTERS) &&
         ((sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_MARIO) ||
          (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_FOX))) ||
        (((mode == NDS_RENDERER_FAST_RUN_ALL_RAW_CURRENT) ||
          (mode == NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES) ||
          (mode == NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ||
          (mode ==
           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION)) &&
         ((u32)sNdsRendererRuntimeOwner <
          (u32)NDS_RENDERER_PROFILE_OWNER_COUNT));
}

void ndsRendererProfileSetSourceProvenance(u32 owner_occurrence,
                                           u32 list_ordinal,
                                           u32 root_branch_path)
{
    (void)owner_occurrence;
    (void)list_ordinal;
    (void)root_branch_path;
}

void ndsRendererProfileFrameBegin(void)
{
    gNdsRendererIFCommonCloudQueuedCount = 0u;
    gNdsRendererIFCommonCloudEmittedCount = 0u;
    gNdsRendererProfileLevel = NDS_RENDERER_PROFILE_LEVEL;
    gNdsRendererProfileNearPlaneTriangleRejectCount = 0u;
    gNdsRendererProfileRawCurrentCandidateCount = 0u;
    gNdsRendererProfileRawCurrentRangeRejectCount = 0u;
    gNdsRendererProfileRawCrossMatrixCount = 0u;
    gNdsRendererProfileMatrixPosTestSamples = 0u;
    gNdsRendererProfileMatrixPosTestMismatches = 0u;
    gNdsRendererProfileMatrixPosTestMaxError = 0u;
    gNdsRendererProfileMatrixPosTestWSignMismatches = 0u;
    gNdsRendererProfileMatrixPosTestClipMismatches = 0u;
    gNdsRendererProfileMatrixPosTestMatrixWordSamples = 0u;
    gNdsRendererProfileMatrixPosTestDropped = 0u;
    ndsRendererProfileResetSubmitSummary();
    sNdsRendererFastRunCount = 0u;
    sNdsRendererFastTriangleCount = 0u;
    memset(sNdsRendererFastOwnerTriangleCount, 0,
           sizeof(sNdsRendererFastOwnerTriangleCount));
    memset(sNdsRendererFastFallbackCount, 0,
           sizeof(sNdsRendererFastFallbackCount));
    memset(&sNdsRendererRuntimeFrameSummary, 0,
           sizeof(sNdsRendererRuntimeFrameSummary));
    if (gNdsRendererProfileFrameCount == 1u)
    {
        sNdsRendererRuntimeTexel1FractionRefreshCount = 0u;
        sNdsRendererRuntimeTextureCacheEvictCount = 0u;
        sNdsRendererRuntimeTextureCi4DirectPixels = 0u;
        sNdsRendererRuntimeCi4IndexCacheBuildCount = 0u;
        sNdsRendererRuntimeCi4IndexCacheReuseCount = 0u;
        sNdsRendererRuntimeCi4RepresentativePixelCount = 0u;
        sNdsRendererRuntimeCi4ReusePixelCount = 0u;
    }
}

void ndsRendererProfileFramePublish(void)
{
    gNdsRendererFastRunCount = sNdsRendererFastRunCount;
    gNdsRendererFastTriangleCount = sNdsRendererFastTriangleCount;
    memcpy((void *)gNdsRendererFastOwnerTriangleCount,
           sNdsRendererFastOwnerTriangleCount,
           sizeof(gNdsRendererFastOwnerTriangleCount));
    memcpy((void *)gNdsRendererFastFallbackCount,
           sNdsRendererFastFallbackCount,
           sizeof(gNdsRendererFastFallbackCount));
    /* melonDS GDB reads main RAM, not dirty ARM9 D-cache lines.  Publish this
     * cross-checked group coherently using the same rule as platform proof
     * counters; otherwise owner[0..2] can be one cache line behind Link/total. */
    NDS_PUBLISH_DEBUGGER_GROUP(NDS_RENDERER_FAST_DEBUGGER_GROUP);
    /* One compact publication replaces hot-loop volatile diagnostic writes. */
    gNdsRendererProfileTextureBinds =
        sNdsRendererRuntimeFrameSummary.texture_binds;
    gNdsRendererProfileTextureUploads =
        sNdsRendererRuntimeFrameSummary.texture_uploads;
    gNdsRendererProfileTextureUploadBytes =
        sNdsRendererRuntimeFrameSummary.texture_upload_bytes;
    gNdsRendererProfileTextureCi4DirectPixels =
        sNdsRendererRuntimeTextureCi4DirectPixels;
    gNdsRendererProfileCi4IndexCacheBuildCount =
        sNdsRendererRuntimeCi4IndexCacheBuildCount;
    gNdsRendererProfileCi4IndexCacheReuseCount =
        sNdsRendererRuntimeCi4IndexCacheReuseCount;
    gNdsRendererProfileCi4RepresentativePixelCount =
        sNdsRendererRuntimeCi4RepresentativePixelCount;
    gNdsRendererProfileCi4ReusePixelCount =
        sNdsRendererRuntimeCi4ReusePixelCount;
    gNdsRendererProfileTextureCacheAliasAvoidCount =
        sNdsRendererRuntimeFrameSummary.texture_cache_alias_avoid_count;
    gNdsRendererProfileTextureLookupCallCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_call_count;
    gNdsRendererProfileTextureLookupProbeCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_probe_count;
    gNdsRendererProfileTextureLookupActiveHitCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_active_hit_count;
    gNdsRendererProfileTextureLookupTableHitCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_table_hit_count;
    gNdsRendererProfileTextureLookupMissCount =
        sNdsRendererRuntimeFrameSummary.texture_lookup_miss_count;
    gNdsRendererProfileTexel1CompositeCount =
        sNdsRendererRuntimeFrameSummary.texel1_composite_count;
    gNdsRendererProfileTexel1LoadMatchCount =
        sNdsRendererRuntimeFrameSummary.texel1_load_match_count;
    gNdsRendererProfileTexel1RejectCount =
        sNdsRendererRuntimeFrameSummary.texel1_reject_count;
    gNdsRendererProfileTexel1FractionRefreshCount =
        sNdsRendererRuntimeTexel1FractionRefreshCount;
    gNdsRendererProfileTextureCacheEvictCount =
        sNdsRendererRuntimeTextureCacheEvictCount;
    gNdsRendererProfileProjectedSubmitFallbackCount =
        sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count;
    gNdsRendererProfileMatrixLoadCount =
        sNdsRendererRuntimeFrameSummary.matrix_load_count;
    gNdsRendererProfileHardwareVertices =
        sNdsRendererRuntimeFrameSummary.hardware_vertices;
    gNdsRendererProfileHardwareTriangles =
        sNdsRendererRuntimeFrameSummary.hardware_triangles;
    gNdsRendererProfileHardwareBatchBeginCount =
        sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count;
    gNdsRendererProfileHardwareBatchReuseCount =
        sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count;
    gNdsRendererProfileHardwareBatchEndCount =
        sNdsRendererRuntimeFrameSummary.hardware_batch_end_count;
    gNdsRendererProfileTexturePrepareCount =
        sNdsRendererRuntimeFrameSummary.texture_prepare_count;
    gNdsRendererProfileTexturePrepareReuseCount =
        sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count;
    gNdsRendererProfileHardwareOverLimit =
        sNdsRendererRuntimeFrameSummary.hardware_over_limit;
    gNdsRendererProfileHWVertexSaturateCount =
        sNdsRendererRuntimeFrameSummary.hardware_vertex_saturate_count;
    gNdsRendererProfileNearPlaneTriangleRejectCount =
        sNdsRendererRuntimeFrameSummary.near_plane_triangle_reject_count;
    gNdsRendererProfileRawCurrentCandidateCount =
        sNdsRendererRuntimeFrameSummary.raw_current_candidate_count;
    gNdsRendererProfileRawCurrentRangeRejectCount =
        sNdsRendererRuntimeFrameSummary.raw_current_range_reject_count;
    gNdsRendererProfileRawCrossMatrixCount =
        sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count;
    gNdsRendererProfileOracleSamples = 0u;
    gNdsRendererProfileOracleMismatches = 0u;
    gNdsRendererProfileOracleMaxDelta = 0u;
}

u32 ndsRendererHardwareConsumeSubmittedFrame(void)
{
    u32 submitted;

    ndsRendererHardwareEmitIFCommonClouds();
    submitted = sNdsRendererHardwareSubmitted;
    ndsRendererHardwareEndBatch();
    if ((submitted != FALSE) ||
        (sNdsRendererHardwareSubmitClassCounts[
             NDS_RENDERER_HW_SUBMIT_REJECT] != 0u))
    {
        /* The accelerated mode-163 proof does not use the realtime presentation
         * wrapper. Publish at the renderer-owned hardware-frame boundary so
         * every configuration reports the same completed-frame contract. */
        ndsRendererProfilePublishSubmitSummary();
    }
    ndsRendererProfileResetSubmitSummary();
    sNdsRendererHardwareSubmitted = FALSE;
#if NDS_TICK_HUD
    ndsRendererHardwarePainterSlotFoldFrame();
#endif
    sNdsRendererHardwareProjectedDepth =
        NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START;
    sNdsRendererHardwareProjectedBackground = TRUE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    /* Tint tiles the frame just asked for are created HERE: the frame's batch
     * is closed and no draw walk is running, and the tracker reset right below
     * cleans up after the upload's own bind. See
     * ndsRendererR2FighterTintLookup for why never inside a walk. */
    ndsRendererHardwareServiceFighterTintTiles();
    /* Same reason: one particle animation sheet a frame, once the countdown
     * has released its atlases (nds_renderer_particles.c). */
    ndsRendererParticleAnimationPump();
    sNdsRendererHardwareBoundTextureName = 0;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
#if NDS_TICK_HUD
    /* P2-2 capacity witness: one 79-entry scan per presented frame, after
     * every activation of the frame has marked its entry ready. */
    ndsRendererRecordTextureKeyPoolUse();
#endif
    sNdsRendererHardwareFrameSerial++;
    return submitted;
}

