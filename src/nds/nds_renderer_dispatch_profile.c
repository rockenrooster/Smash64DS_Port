void ndsRendererInitStats(NDSRendererStats *stats)
{
    if (stats != NULL)
    {
#if NDS_TASK107_RENDER_STATE_CENSUS
        ndsRendererTask107ForgetSyncTrack(stats);
#endif
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

void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats)
{
    NDSRendererTraversalState state;
    NDSRendererTraversalVertexStorage vertex_storage;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererMatrixSnapshot
        matrix_snapshot_storage[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    NDSRendererMatrixSnapshot *matrix_snapshots = matrix_snapshot_storage;
#else
    NDSRendererMatrixSnapshot *matrix_snapshots = NULL;
#endif

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

    ndsRendererInitTraversalState(&state, config, stats, &vertex_storage,
                                  matrix_snapshots, 0u);
    ndsRendererScanList(dl, config, stats, &state, 0, NULL, NULL);
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererHardwareEndBatch();
#endif
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->vertex_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_VERTICES;
        return;
    }
    if (stats->triangle_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_TRIANGLES;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}

void ndsRendererHardwareResetSourceCaches(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    memset(sNdsRendererDirectRawPlans, 0,
           sizeof(sNdsRendererDirectRawPlans));
    sNdsRendererDirectRawEntryCount = 0u;
    memset(sNdsRendererStageTextureSites, 0,
           sizeof(sNdsRendererStageTextureSites));
    sNdsRendererStageTextureSiteNext = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    memset(sNdsRendererHardwareCi4IndexCache, 0,
           sizeof(sNdsRendererHardwareCi4IndexCache));
    sNdsRendererHardwareCi4IndexCacheNext = 0u;
#endif
}

void ndsRendererHardwareSetNoOracle(u32 enabled)
{
#if NDS_RENDERER_HW_TRIANGLES
    sNdsRendererHardwareNoOracle = (enabled != 0u) ? TRUE : FALSE;
#else
    (void)enabled;
#endif
}

u32 ndsRendererHardwareNoOracleEnabled(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    return sNdsRendererHardwareNoOracle;
#else
    return FALSE;
#endif
}

void ndsRendererProfileSetOwner(NDSRendererProfileOwner owner)
{
#if NDS_TASK29_GX_CENSUS
    ndsRendererTask29GXSetOwner(owner);
#endif
#if NDS_RENDERER_HW_TRIANGLES
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
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileOwner =
        ((u32)owner < (u32)NDS_RENDERER_PROFILE_OWNER_COUNT) ? owner :
        NDS_RENDERER_PROFILE_OWNER_NONE;
    memset(&sNdsRendererSemanticSourceProvenance, 0,
           sizeof(sNdsRendererSemanticSourceProvenance));
#else
#if !NDS_RENDERER_HW_TRIANGLES
    (void)owner;
#endif
#endif
}

void ndsRendererProfileSetSourceProvenance(u32 owner_occurrence,
                                           u32 list_ordinal,
                                           u32 root_branch_path)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererProfileOwnerHotLedger *owner;
    u32 owner_index = (u32)sNdsRendererProfileOwner;
    u32 owner_mask;

    sNdsRendererSemanticSourceProvenance.owner_occurrence =
        owner_occurrence;
    sNdsRendererSemanticSourceProvenance.list_ordinal = list_ordinal;
    sNdsRendererSemanticSourceProvenance.root_branch_path = root_branch_path;
    owner = ndsRendererProfileCurrentOwner();
    if (owner == NULL)
    {
        return;
    }
    owner_mask = 1u << owner_index;
    if (((sNdsRendererSemanticOwnerOccurrenceValidMask & owner_mask) == 0u) ||
        (sNdsRendererSemanticOwnerLastOccurrence[owner_index] !=
         owner_occurrence))
    {
        sNdsRendererSemanticOwnerOccurrenceValidMask |= owner_mask;
        sNdsRendererSemanticOwnerLastOccurrence[owner_index] =
            owner_occurrence;
        owner->semantic_occurrence_count++;
    }
#else
    (void)owner_occurrence;
    (void)list_ordinal;
    (void)root_branch_path;
#endif
}

void ndsRendererProfileRecordFrameBoundaryGXState(u32 status, u32 control)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsRendererProfileGXStatusPostVBlank = status;
    sNdsRendererProfileGXControlPostVBlank = control;
#else
    (void)status;
    (void)control;
#endif
}

void ndsRendererProfileRecordMaterialOperations(u32 count)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererProfileOwnerHotLedger *owner =
        ndsRendererProfileCurrentOwner();

    if (owner != NULL)
    {
        owner->material_operation_count += count;
    }
#else
    (void)count;
#endif
}

u32 ndsRendererProfileGlobalStateHash(void)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    u32 hash = 0u;

    hash = ndsRendererProfileHashU32(
        hash, (u32)sNdsRendererHardwareProjectedDepth);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareProjectedBackground);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererMatrixGenerationSerial);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixLoaded);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixMode);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixGeneration);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareMatrixSignature);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareBoundTextureName);
    hash = ndsRendererProfileHashU32(
        hash, (u32)sNdsRendererHardwareNoTextureName);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchOpen);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchTextured);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchTextureName);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchPolyFmt);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchAlphaKey);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchFogKey);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchMatrixMode);
    hash = ndsRendererProfileHashU32(
        hash, sNdsRendererHardwareTriangleBatchMatrixGeneration);
    if (sNdsRendererHardwareActiveTextureEntry != NULL)
    {
        NDSRendererHardwareTextureKey active;

        ndsRendererHardwareEntryCopyKey(
            sNdsRendererHardwareActiveTextureEntry, &active);
        hash = ndsRendererProfileHashU32(hash, 1u);
        hash = ndsRendererProfileHashU32(
            hash, (u32)sNdsRendererHardwareActiveTextureEntry->name);
        hash = ndsRendererProfileHashU32(
            hash, sNdsRendererHardwareActiveTextureEntry->ready);
        hash = ndsRendererProfileHashU32(
            hash, sNdsRendererHardwareActiveTextureEntry->params);
        hash = ndsRendererProfileHashU32(
            hash, ndsRendererProfileTextureKeyHashFull(&active));
    }
    else
    {
        hash = ndsRendererProfileHashU32(hash, 0u);
    }
    return hash;
#else
    return 0u;
#endif
}

#if NDS_RENDER_ECONOMY
void ndsRendererProfileFrameBegin(u32 render_economy_allowed)
#else
void ndsRendererProfileFrameBegin(void)
#endif
{
    gNdsRendererIFCommonCloudQueuedCount = 0u;
    gNdsRendererIFCommonCloudEmittedCount = 0u;
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
    if (gNdsRendererScreenSpaceCensusResetRequested != 0u)
    {
        ndsRendererScreenSpaceCensusReset();
        gNdsRendererScreenSpaceCensusResetRequested = 0u;
    }
    if (gNdsRendererScreenSpaceCensusArmed != 0u)
    {
        gNdsRendererScreenSpaceCensusFrameCount++;
    }
#endif
#if NDS_RENDER_ECONOMY
    gNdsRendererEconomyActiveOwnerMask = 0u;
    gNdsRendererEconomyAppliedOwnerMask = 0u;
    gNdsRendererEconomySkippedRunCount = 0u;
    gNdsRendererEconomySkippedTriangleCount = 0u;
    if (render_economy_allowed != 0u)
    {
        gNdsRendererEconomyActiveOwnerMask =
            gNdsRendererEconomyConfiguredOwnerMask;
    }
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if (gNdsRendererBenchmarkSinkCalibrationWords == 0u)
    {
        u32 calibration_start = cpuGetTiming();
        u32 i;

        for (i = 0u; i < NDS_RENDERER_BENCHMARK_SINK_WORDS; i++)
        {
            ndsRendererBenchmarkSinkWord(i);
        }
        gNdsRendererBenchmarkSinkCalibrationTicks =
            cpuGetTiming() - calibration_start;
        gNdsRendererBenchmarkSinkCalibrationWords =
            NDS_RENDERER_BENCHMARK_SINK_WORDS;
    }
    sNdsRendererBenchmarkSinkCursor = 0u;
    sNdsRendererBenchmarkSinkWordCount = 0u;
    sNdsRendererBenchmarkSinkLastOwnerCursor = 0u;
    sNdsRendererBenchmarkSinkHashA = 2166136261u;
    sNdsRendererBenchmarkSinkHashB = 0x9e3779b9u;
    sNdsRendererBenchmarkSegment0SinkWords = 0u;
    sNdsRendererBenchmarkSegment0SinkHashA = 2166136261u;
    sNdsRendererBenchmarkSegment0SinkHashB = 0x9e3779b9u;
    sNdsRendererBenchmarkSegment0SinkActive = FALSE;
    sNdsRendererBenchmarkSegment0SinkArmFaults = 0u;
    memset(sNdsRendererBenchmarkSinkOwnerWords, 0,
           sizeof(sNdsRendererBenchmarkSinkOwnerWords));
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    memset((void *)gNdsRendererProfileOwners, 0,
           sizeof(gNdsRendererProfileOwners));
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsRendererM2ShadeEpochCount = 0u;
    sNdsRendererM2ShadeKeyHitCount = 0u;
    sNdsRendererM2ShadeResidentHitCount = 0u;
    sNdsRendererM2ShadeHashCollisionCount = 0u;
    sNdsRendererM2ShadeDenseVisitCount = 0u;
    sNdsRendererM2ShadeComputeCount = 0u;
    sNdsRendererM2ShadeLutComputeCount = 0u;
    sNdsRendererM2ShadePreparedComputeCount = 0u;
    sNdsRendererM2ShadeAliasCopyCount = 0u;
    sNdsRendererM2ShadeMaterialPackCount = 0u;
    memset(sNdsRendererM2ShadeOwnerEpochCount, 0,
           sizeof(sNdsRendererM2ShadeOwnerEpochCount));
    memset(sNdsRendererM2ShadeOwnerKeyHitCount, 0,
           sizeof(sNdsRendererM2ShadeOwnerKeyHitCount));
    memset(sNdsRendererM2ShadeOwnerResidentHitCount, 0,
           sizeof(sNdsRendererM2ShadeOwnerResidentHitCount));
#endif
#if NDS_RENDERER_HW_TRIANGLES
    gNdsRendererProfileTexturePairOracleChecks = 0u;
    gNdsRendererProfileTexturePairOracleMismatches = 0u;
    gNdsRendererProfileTextureVBlankQueuedUploads = 0u;
    gNdsRendererProfileTextureVBlankQueuedBytes = 0u;
    gNdsRendererProfileTextureVBlankCommittedUploads = 0u;
    gNdsRendererProfileTextureVBlankCommitTicks = 0u;
    gNdsRendererProfileTextureVBlankOutsideCount = 0u;
    gNdsRendererProfileTextureVBlankFallbackCount = 0u;
    gNdsRendererProfileTextureVBlankStartLine = 0u;
    gNdsRendererProfileTextureVBlankEndLine = 0u;
#endif
    sNdsRendererProfileGXStatusPostVBlank = 0u;
    sNdsRendererProfileGXControlPostVBlank = 0u;
    gNdsRendererProfileGXStatusPostVBlank = 0u;
    gNdsRendererProfileGXControlPostVBlank = 0u;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    memset(sNdsRendererProfileOwnerHot, 0,
           sizeof(sNdsRendererProfileOwnerHot));
    sNdsRendererProfileOwner = NDS_RENDERER_PROFILE_OWNER_NONE;
    sNdsRendererProfileImmutableListCount = 0u;
    sNdsRendererProfileTrustedCommandCount = 0u;
    sNdsRendererProfileValidatedCommandCount = 0u;
    sNdsRendererProfileTriangleRunReuseCount = 0u;
    sNdsRendererProfileTriangleSubmitTicks = 0u;
    sNdsRendererProfileVertexSubmitTicks = 0u;
    sNdsRendererProfileCi4LutBuildCount = 0u;
    sNdsRendererProfileCi4LutReuseCount = 0u;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    memset(&sNdsRendererSemanticSourceProvenance, 0,
           sizeof(sNdsRendererSemanticSourceProvenance));
    memset(sNdsRendererSemanticOwnerLastOccurrence, 0,
           sizeof(sNdsRendererSemanticOwnerLastOccurrence));
    sNdsRendererSemanticOwnerOccurrenceValidMask = 0u;
    sNdsRendererSemanticOutputHash = 0u;
    sNdsRendererSemanticOutputHash2 = 0u;
    sNdsRendererSemanticEventCount = 0u;
    sNdsRendererSemanticOverflowCount = 0u;
    sNdsRendererSemanticLastTextureKeyHash = 0u;
    sNdsRendererSemanticLastTextureParams = 0u;
    gNdsRendererSemanticOutputHash = 0u;
    gNdsRendererSemanticOutputHash2 = 0u;
    gNdsRendererSemanticEventCount = 0u;
    gNdsRendererSemanticOverflowCount = 0u;
    memset((void *)gNdsRendererStageDepthTrace, 0,
           sizeof(gNdsRendererStageDepthTrace));
    gNdsRendererStageDepthTraceCount = 0u;
    gNdsRendererStageDepthTraceOverflowCount = 0u;
    gNdsRendererStageDepthTraceHash = 0u;
    memset((void *)gNdsRendererStageDepthTraceClassCount, 0,
           sizeof(gNdsRendererStageDepthTraceClassCount));
    gNdsRendererStageDepthTraceNoZCollisionCount = 0u;
    gNdsRendererStageDepthTraceBackgroundCount = 0u;
    gNdsRendererStageDepthTraceBackgroundMin = 0;
    gNdsRendererStageDepthTraceBackgroundMax = 0;
    gNdsRendererStageDepthTraceForegroundCount = 0u;
    gNdsRendererStageDepthTraceForegroundMin = 0;
    gNdsRendererStageDepthTraceForegroundMax = 0;
    sNdsRendererStageDepthTraceLastBackground = 0;
    sNdsRendererStageDepthTraceLastForeground = 0;
    sNdsRendererStageDepthTraceLastValidMask = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileResetSubmitSummary();
#endif
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount = 0u;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
    sNdsRendererBenchmarkSuppressedTextureUploads = 0u;
    sNdsRendererBenchmarkSuppressedTextureUploadBytes = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
    sNdsRendererFastRunCount = 0u;
    sNdsRendererFastTriangleCount = 0u;
    memset(sNdsRendererFastOwnerTriangleCount, 0,
           sizeof(sNdsRendererFastOwnerTriangleCount));
    memset(sNdsRendererFastFallbackCount, 0,
           sizeof(sNdsRendererFastFallbackCount));
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
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
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    sNdsRendererHardwarePendingPosTestCount = 0u;
    sNdsRendererHardwarePendingPosTestLastGeneration = 0u;
#endif
}

void ndsRendererProfileFramePublish(void)
{
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    gNdsRendererM2ShadeEpochCount = sNdsRendererM2ShadeEpochCount;
    gNdsRendererM2ShadeKeyHitCount = sNdsRendererM2ShadeKeyHitCount;
    gNdsRendererM2ShadeResidentHitCount =
        sNdsRendererM2ShadeResidentHitCount;
    gNdsRendererM2ShadeHashCollisionCount =
        sNdsRendererM2ShadeHashCollisionCount;
    gNdsRendererM2ShadeDenseVisitCount =
        sNdsRendererM2ShadeDenseVisitCount;
    gNdsRendererM2ShadeComputeCount = sNdsRendererM2ShadeComputeCount;
    gNdsRendererM2ShadeLutComputeCount =
        sNdsRendererM2ShadeLutComputeCount;
    gNdsRendererM2ShadePreparedComputeCount =
        sNdsRendererM2ShadePreparedComputeCount;
    gNdsRendererM2ShadeAliasCopyCount =
        sNdsRendererM2ShadeAliasCopyCount;
    gNdsRendererM2ShadeMaterialPackCount =
        sNdsRendererM2ShadeMaterialPackCount;
    memcpy((void *)gNdsRendererM2ShadeOwnerEpochCount,
           sNdsRendererM2ShadeOwnerEpochCount,
           sizeof(gNdsRendererM2ShadeOwnerEpochCount));
    memcpy((void *)gNdsRendererM2ShadeOwnerKeyHitCount,
           sNdsRendererM2ShadeOwnerKeyHitCount,
           sizeof(gNdsRendererM2ShadeOwnerKeyHitCount));
    memcpy((void *)gNdsRendererM2ShadeOwnerResidentHitCount,
           sNdsRendererM2ShadeOwnerResidentHitCount,
           sizeof(gNdsRendererM2ShadeOwnerResidentHitCount));
#endif
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    gNdsRendererBenchmarkTriangleCount =
        sNdsRendererBenchmarkTriangleCount;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    gNdsRendererBenchmarkSinkCursor =
        sNdsRendererBenchmarkSinkCursor;
    gNdsRendererBenchmarkSinkWordCount =
        sNdsRendererBenchmarkSinkWordCount;
    gNdsRendererBenchmarkSinkHashA = sNdsRendererBenchmarkSinkHashA;
    gNdsRendererBenchmarkSinkHashB = sNdsRendererBenchmarkSinkHashB;
    gNdsRendererBenchmarkSegment0SinkWords =
        sNdsRendererBenchmarkSegment0SinkWords;
    gNdsRendererBenchmarkSegment0SinkHashA =
        sNdsRendererBenchmarkSegment0SinkHashA;
    gNdsRendererBenchmarkSegment0SinkHashB =
        sNdsRendererBenchmarkSegment0SinkHashB;
    gNdsRendererBenchmarkSegment0SinkArmFaults =
        sNdsRendererBenchmarkSegment0SinkArmFaults;
    memcpy((void *)gNdsRendererBenchmarkSinkOwnerWords,
           sNdsRendererBenchmarkSinkOwnerWords,
           sizeof(gNdsRendererBenchmarkSinkOwnerWords));
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
    gNdsRendererBenchmarkSuppressedTextureUploads =
        sNdsRendererBenchmarkSuppressedTextureUploads;
    gNdsRendererBenchmarkSuppressedTextureUploadBytes =
        sNdsRendererBenchmarkSuppressedTextureUploadBytes;
#endif
#if NDS_RENDERER_HW_TRIANGLES
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
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileGXStatusPostVBlank =
        sNdsRendererProfileGXStatusPostVBlank;
    gNdsRendererProfileGXControlPostVBlank =
        sNdsRendererProfileGXControlPostVBlank;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 owner_index;

    gNdsRendererProfileImmutableListCount =
        sNdsRendererProfileImmutableListCount;
    gNdsRendererProfileTrustedCommandCount =
        sNdsRendererProfileTrustedCommandCount;
    gNdsRendererProfileValidatedCommandCount =
        sNdsRendererProfileValidatedCommandCount;
    gNdsRendererProfileTriangleRunReuseCount =
        sNdsRendererProfileTriangleRunReuseCount;
    gNdsRendererProfileTriangleSubmitTicks =
        sNdsRendererProfileTriangleSubmitTicks;
    gNdsRendererProfileVertexSubmitTicks =
        sNdsRendererProfileVertexSubmitTicks;
    gNdsRendererProfileCi4LutBuildCount =
        sNdsRendererProfileCi4LutBuildCount;
    gNdsRendererProfileCi4LutReuseCount =
        sNdsRendererProfileCi4LutReuseCount;
    for (owner_index = 0u;
         owner_index < NDS_RENDERER_PROFILE_OWNER_COUNT;
         owner_index++)
    {
        u32 submit_class;
        volatile NDSRendererOwnerProfile *owner =
            &gNdsRendererProfileOwners[owner_index];
        const NDSRendererProfileOwnerHotLedger *hot =
            &sNdsRendererProfileOwnerHot[owner_index];

        owner->material_operation_count = hot->material_operation_count;
        owner->texture_change_count = hot->texture_change_count;
        owner->run_count = hot->run_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        owner->semantic_output_hash = hot->semantic_output_hash;
        owner->semantic_output_hash2 = hot->semantic_output_hash2;
        owner->semantic_event_count = hot->semantic_event_count;
        owner->semantic_overflow_count = hot->semantic_overflow_count;
        owner->semantic_occurrence_count = hot->semantic_occurrence_count;
        owner->semantic_first_owner_occurrence =
            hot->semantic_first_owner_occurrence;
        owner->semantic_first_list_ordinal =
            hot->semantic_first_list_ordinal;
        owner->semantic_first_branch_path =
            hot->semantic_first_branch_path;
        owner->semantic_first_command_index =
            hot->semantic_first_command_index;
        owner->semantic_first_tri2_half = hot->semantic_first_tri2_half;
        owner->semantic_first_outcome = hot->semantic_first_outcome;
#endif
        for (submit_class = 0u; submit_class < 8u; submit_class++)
        {
            owner->submit_class_count[submit_class] =
                hot->submit_class_count[submit_class];
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererSemanticOutputHash = sNdsRendererSemanticOutputHash;
    gNdsRendererSemanticOutputHash2 = sNdsRendererSemanticOutputHash2;
    gNdsRendererSemanticEventCount = sNdsRendererSemanticEventCount;
    gNdsRendererSemanticOverflowCount =
        sNdsRendererSemanticOverflowCount;
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
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
#endif
}

u32 ndsRendererHardwareConsumeSubmittedFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 submitted;

    ndsRendererHardwareEmitIFCommonClouds();
    submitted = sNdsRendererHardwareSubmitted;
    ndsRendererHardwareEndBatch();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererHardwareRunRawMatrixPosTests();
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererHardwareMatrixSignature = 0u;
#endif
    sNdsRendererHardwareBoundTextureName = 0;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
#if NDS_R2_FIGHTER_SHADE_PROOF
    ndsRendererR2FighterShadeProofFrame();
#endif
#if NDS_R2_FIGHTER_RUN_PROOF
    ndsRendererR2FighterRunProofFrame();
#endif
#if NDS_R2_STAGE_ROUTE_PROBE
    {
        u32 texture_live = 0u;
        u32 texture_touched = 0u;
        u32 texture_slot;

        for (texture_slot = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT;
             texture_slot < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT;
             texture_slot++)
        {
            const NDSRendererHardwareTextureCacheEntry *texture_entry =
                &sNdsRendererHardwareTextureCache[texture_slot];

            if (texture_entry->name != 0)
            {
                texture_live++;
                if (texture_entry->last_used_frame ==
                    (sNdsRendererHardwareFrameSerial + 1u))
                {
                    texture_touched++;
                }
            }
        }
        if (texture_live > gNdsR2TextureLiveHighWater)
        {
            gNdsR2TextureLiveHighWater = texture_live;
        }
        if (texture_touched > gNdsR2TextureTouchedHighWater)
        {
            gNdsR2TextureTouchedHighWater = texture_touched;
        }
    }
#endif
    sNdsRendererHardwareFrameSerial++;
    return submitted;
#else
    return FALSE;
#endif
}

#if NDS_R2_STAGE_ROUTE_PROBE
/* The sparse stress probe stops in GDB after the guest has completed a frame.
 * GDB reads main RAM and cannot see dirty ARM9 D-cache lines.  The pacing
 * counters already obey the repo-wide publish rule; the route probe was added
 * later and accidentally read its hot renderer state without doing so.  That
 * made a one-frame-old reject look current and, worse, made cache-slot scans
 * mix generations.  Publish exactly the objects the StageRouteProbe reads.
 * Lab flag only: no shipping code or steady-state production cost. */
void ndsRendererPublishStageRouteProbeDiagnostics(void)
{
#if NDS_TASK91_DRAW_PHASE_CENSUS
    extern u32 gNdsTask91WalkTicks;
    extern u32 gNdsTask91ValidateTicks;
    extern u32 gNdsTask91TotalTicks;
    extern u32 gNdsTask91ResetTicks;
    extern u32 gNdsTask91OwnerPrepTicks;
    extern u32 gNdsTask91MatrixPrepTicks;
    extern u32 gNdsTask91MaterialPrepTicks;
    extern u32 gNdsTask91InputsTicks;
    extern u32 gNdsTask91ExecuteTicks;
    extern u32 gNdsTask91MtxCameraTicks;
    extern u32 gNdsTask91MtxWorldTicks;
    extern u32 gNdsTask91MtxMulTicks;
#endif
    DC_FlushRange((const void *)&sNdsRendererHardwareTextureCache,
                  sizeof(sNdsRendererHardwareTextureCache));
    DC_FlushRange((const void *)&sNdsRendererHardwareFrameSerial,
                  sizeof(sNdsRendererHardwareFrameSerial));
    DC_FlushRange((const void *)&gNdsR2TextureLiveHighWater,
                  sizeof(gNdsR2TextureLiveHighWater));
    DC_FlushRange((const void *)&gNdsR2TextureTouchedHighWater,
                  sizeof(gNdsR2TextureTouchedHighWater));
    DC_FlushRange((const void *)&gNdsR2StageKeyMissInvalid,
                  sizeof(gNdsR2StageKeyMissInvalid));
    DC_FlushRange((const void *)&gNdsR2StageKeyMissGeneration,
                  sizeof(gNdsR2StageKeyMissGeneration));
    DC_FlushRange((const void *)&gNdsR2StageKeyMissStamp,
                  sizeof(gNdsR2StageKeyMissStamp));
    DC_FlushRange((const void *)&gNdsR2StageKeyMissConfig,
                  sizeof(gNdsR2StageKeyMissConfig));
    DC_FlushRange((const void *)&gNdsR2StageKeyMissAssets,
                  sizeof(gNdsR2StageKeyMissAssets));
    DC_FlushRange((const void *)&gNdsR2StageRejectCounts,
                  sizeof(gNdsR2StageRejectCounts));
    DC_FlushRange((const void *)&gNdsR2StagePrepareBuildCount,
                  sizeof(gNdsR2StagePrepareBuildCount));
    DC_FlushRange((const void *)&gNdsR2StagePrepareReuseCount,
                  sizeof(gNdsR2StagePrepareReuseCount));
    DC_FlushRange((const void *)&gNdsRendererProfileTextureRejectReasonMask,
                  sizeof(gNdsRendererProfileTextureRejectReasonMask));
    DC_FlushRange((const void *)&gNdsRendererStageOwnerRejectCount,
                  sizeof(gNdsRendererStageOwnerRejectCount));
    DC_FlushRange((const void *)&gNdsRendererStageOwnerFirstRejectReason,
                  sizeof(gNdsRendererStageOwnerFirstRejectReason));
    DC_FlushRange((const void *)&gNdsRendererStageOwnerLastRejectReason,
                  sizeof(gNdsRendererStageOwnerLastRejectReason));
    DC_FlushRange((const void *)&gNdsRendererTask36RendererRejectReason,
                  sizeof(gNdsRendererTask36RendererRejectReason));
    DC_FlushRange((const void *)&gNdsRendererTask36PrepareRunRejectReason,
                  sizeof(gNdsRendererTask36PrepareRunRejectReason));
    DC_FlushRange((const void *)&gNdsR2TexRejectCensusValid,
                  sizeof(gNdsR2TexRejectCensusValid));
    DC_FlushRange((const void *)&gNdsR2TexRejectCensusFree,
                  sizeof(gNdsR2TexRejectCensusFree));
    DC_FlushRange((const void *)&gNdsR2TexRejectCensusLive,
                  sizeof(gNdsR2TexRejectCensusLive));
    DC_FlushRange((const void *)&gNdsR2TexRejectCensusPinned,
                  sizeof(gNdsR2TexRejectCensusPinned));
    DC_FlushRange((const void *)&gNdsR2TexRejectCensusThisFrame,
                  sizeof(gNdsR2TexRejectCensusThisFrame));
    DC_FlushRange((const void *)&gNdsR2TexRejectCensusEvictable,
                  sizeof(gNdsR2TexRejectCensusEvictable));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissCount,
                  sizeof(gNdsR2StageTextureMissCount));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissRun,
                  sizeof(gNdsR2StageTextureMissRun));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissHash,
                  sizeof(gNdsR2StageTextureMissHash));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissArmed,
                  sizeof(gNdsR2StageTextureMissArmed));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissSourceFrameTried,
                  sizeof(gNdsR2StageTextureMissSourceFrameTried));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissKeyWords,
                  sizeof(gNdsR2StageTextureMissKeyWords));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissImageAsset,
                  sizeof(gNdsR2StageTextureMissImageAsset));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissImageOffset,
                  sizeof(gNdsR2StageTextureMissImageOffset));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissTlutAsset,
                  sizeof(gNdsR2StageTextureMissTlutAsset));
    DC_FlushRange((const void *)&gNdsR2StageTextureMissTlutOffset,
                  sizeof(gNdsR2StageTextureMissTlutOffset));
    DC_FlushRange((const void *)&gNdsMiscWeaponDrawTicks,
                  sizeof(gNdsMiscWeaponDrawTicks));
    DC_FlushRange((const void *)&gNdsMiscEffectDrawTicks,
                  sizeof(gNdsMiscEffectDrawTicks));
    DC_FlushRange((const void *)&gNdsMiscParticleDrawTicks,
                  sizeof(gNdsMiscParticleDrawTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseColorTicks,
                  sizeof(gNdsEffectPhaseColorTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseTreeTicks,
                  sizeof(gNdsEffectPhaseTreeTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseDLTicks,
                  sizeof(gNdsEffectPhaseDLTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseFindTicks,
                  sizeof(gNdsEffectPhaseFindTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseMaterialTicks,
                  sizeof(gNdsEffectPhaseMaterialTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseMatrixTicks,
                  sizeof(gNdsEffectPhaseMatrixTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseExecTicks,
                  sizeof(gNdsEffectPhaseExecTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseTexTicks,
                  sizeof(gNdsEffectPhaseTexTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseVtxTicks,
                  sizeof(gNdsEffectPhaseVtxTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseTriTicks,
                  sizeof(gNdsEffectPhaseTriTicks));
    DC_FlushRange((const void *)&gNdsEffectPhaseTexInExecTicks,
                  sizeof(gNdsEffectPhaseTexInExecTicks));
#if NDS_TASK91_DRAW_PHASE_CENSUS
    DC_FlushRange((const void *)&gNdsTask91WalkTicks,
                  sizeof(gNdsTask91WalkTicks));
    DC_FlushRange((const void *)&gNdsTask91ValidateTicks,
                  sizeof(gNdsTask91ValidateTicks));
    DC_FlushRange((const void *)&gNdsTask91TotalTicks,
                  sizeof(gNdsTask91TotalTicks));
    DC_FlushRange((const void *)&gNdsTask91ResetTicks,
                  sizeof(gNdsTask91ResetTicks));
    DC_FlushRange((const void *)&gNdsTask91OwnerPrepTicks,
                  sizeof(gNdsTask91OwnerPrepTicks));
    DC_FlushRange((const void *)&gNdsTask91MatrixPrepTicks,
                  sizeof(gNdsTask91MatrixPrepTicks));
    DC_FlushRange((const void *)&gNdsTask91MaterialPrepTicks,
                  sizeof(gNdsTask91MaterialPrepTicks));
    DC_FlushRange((const void *)&gNdsTask91InputsTicks,
                  sizeof(gNdsTask91InputsTicks));
    DC_FlushRange((const void *)&gNdsTask91ExecuteTicks,
                  sizeof(gNdsTask91ExecuteTicks));
    DC_FlushRange((const void *)&gNdsTask91MtxCameraTicks,
                  sizeof(gNdsTask91MtxCameraTicks));
    DC_FlushRange((const void *)&gNdsTask91MtxWorldTicks,
                  sizeof(gNdsTask91MtxWorldTicks));
    DC_FlushRange((const void *)&gNdsTask91MtxMulTicks,
                  sizeof(gNdsTask91MtxMulTicks));
#endif
}
#endif

void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    NDSRendererTraversalState state;
    NDSRendererTraversalVertexStorage vertex_storage;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererMatrixSnapshot
        local_matrix_snapshots[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    NDSRendererMatrixSnapshot *matrix_snapshots = local_matrix_snapshots;
#else
    NDSRendererMatrixSnapshot *matrix_snapshots = NULL;
#endif
    u32 matrix_snapshot_count = 0u;

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES
    if (vertex_cache != NULL)
    {
        matrix_snapshots = vertex_cache->matrix_snapshots;
        matrix_snapshot_count = vertex_cache->matrix_snapshot_count;
    }
#endif
    ndsRendererInitTraversalState(&state, config, stats, &vertex_storage,
                                  matrix_snapshots, matrix_snapshot_count);
    if (vertex_cache != NULL)
    {
        /* BattleShip submits the selected lists through one persistent RSP
         * stream. Back traversal directly with that stream instead of
         * clearing and copying every 32-slot plane around each list. */
        state.vertices = vertex_cache->transformed_vertices;
        state.vertex_valid_mask = vertex_cache->transformed_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        state.input_vertices = vertex_cache->input_vertices;
        state.input_vertex_valid_mask = vertex_cache->input_valid_mask;
        state.raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
        state.vertex_colors = vertex_cache->vertex_colors;
        state.vertex_color_valid_mask =
            vertex_cache->vertex_color_valid_mask;
        state.vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;
        state.vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;
#endif
    }
    ndsRendererScanList(dl, config, stats, &state, 0, callback,
                        callback_user);
    if (vertex_cache != NULL)
    {
        vertex_cache->transformed_valid_mask = state.vertex_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        vertex_cache->input_valid_mask = state.input_vertex_valid_mask;
        vertex_cache->raw_vertex_fit_mask = state.raw_vertex_fit_mask;
        vertex_cache->vertex_color_valid_mask =
            state.vertex_color_valid_mask;
        vertex_cache->matrix_snapshot_count = state.matrix_snapshot_count;
#endif
    }
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererHardwareEndBatch();
#endif
    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (stats->unsupported_command_count != 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
        return;
    }
    if (stats->end_command_count == 0)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_NO_END;
        return;
    }
}

void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererExecuteDisplayListWithVertexCache(dl, config, callback,
                                                 callback_user, stats, NULL);
}
