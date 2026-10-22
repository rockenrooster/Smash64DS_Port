/* Native renderer, the fighter owners' runtime tables: each fighter's high
 * and low detail owner (tables, roots, cross links, light preambles, image
 * bytes). A slice of nds_renderer.c, after nds_renderer_assets.c. */

static const u32 sNdsNativeMarioFoxRootLightPreambles[3][2] =
{
    { 0u, 0u },
    { NDS_NATIVE_ROOT_LIGHT1, NDS_NATIVE_ROOT_LIGHT2_1 },
    { NDS_NATIVE_ROOT_LIGHT1, NDS_NATIVE_ROOT_LIGHT2_2 },
};

#define NDS_FTR_OWNER_RUNTIME(name_, tables_, roots_, cross_, preambles_, bytes_) \
    static const NDSNativeFighterOwnerRuntime name_ = { \
        (tables_), (roots_), NDS_FTR_COUNT(roots_), (cross_), (preambles_), \
        NDS_FTR_COUNT(preambles_), (bytes_) \
    }

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeMarioHighOwner, &sNdsNativeFighterHighTables,
    sNdsNativeMarioRoots, sNdsNativeMarioCrossPaletteSlots,
    sNdsNativeMarioFoxRootLightPreambles, 0x7510u);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeFoxHighOwner, &sNdsNativeFighterHighTables,
    sNdsNativeFoxRoots, sNdsNativeFoxCrossPaletteSlots,
    sNdsNativeMarioFoxRootLightPreambles, 0x7e50u);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeMarioLowOwner, &sNdsNativeFighterLowTables,
    sNdsNativeMarioRootsLow, sNdsNativeMarioCrossPaletteSlotsLow,
    sNdsNativeMarioFoxRootLightPreambles, 0x7510u);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeFoxLowOwner, &sNdsNativeFighterLowTables,
    sNdsNativeFoxRootsLow, sNdsNativeFoxCrossPaletteSlotsLow,
    sNdsNativeMarioFoxRootLightPreambles, 0x7e50u);

#if NDS_P2_LUIGI
#if NDS_NATIVE_OWNER_IMAGE_LUIGI
/* P2-3r4: these tables are FILLED AT LOAD from the owner image, so they are
 * mutable and start empty.  The arrays that used to initialise them are
 * guarded out of this binary by the same flag; there is exactly one copy of
 * the bytes and it is the NitroFS one.  Every reader goes through the owner
 * runtime, which is only reachable after ndsRendererNativeEnsureOwnerImage
 * has bound this struct. */
static NDSNativeFighterRuntimeTables sNdsNativeLuigiFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeLuigiFighterHighTables =
{
    sNdsNativeLuigiFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterStateDeltas),
    sNdsNativeLuigiFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterStateSequence),
    sNdsNativeLuigiFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterVertexActions),
    sNdsNativeLuigiFighterEpochDirectPolicy,
    sNdsNativeLuigiFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterDenseVertices),
    sNdsNativeLuigiFighterDenseNormals,
    sNdsNativeLuigiFighterPreparedDense,
    sNdsNativeLuigiFighterActionDenseSpans,
    sNdsNativeLuigiFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterPackedCorners),
    sNdsNativeLuigiFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterRunFirstCorner),
    sNdsNativeLuigiFighterRunFirstUnique,
    sNdsNativeLuigiFighterRunUniqueCount,
    sNdsNativeLuigiFighterRunUniqueDense,
    sNdsNativeLuigiFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterTriangles),
    sNdsNativeLuigiFighterRuns,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterRuns),
    sNdsNativeLuigiFighterPrimitiveGroupFirst,
    sNdsNativeLuigiFighterPrimitiveGroupCount,
    sNdsNativeLuigiFighterPrimitiveGroupType,
    sNdsNativeLuigiFighterPrimitiveGroupFirstVertex,
    sNdsNativeLuigiFighterPrimitiveGroupVertexCount,
    sNdsNativeLuigiFighterPrimitiveVertices,
    sNdsNativeLuigiFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_LUIGI
/* P2-3r4: these tables are FILLED AT LOAD from the owner image, so they are
 * mutable and start empty.  The arrays that used to initialise them are
 * guarded out of this binary by the same flag; there is exactly one copy of
 * the bytes and it is the NitroFS one.  Every reader goes through the owner
 * runtime, which is only reachable after ndsRendererNativeEnsureOwnerImage
 * has bound this struct. */
static NDSNativeFighterRuntimeTables sNdsNativeLuigiFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeLuigiFighterLowTables =
{
    sNdsNativeLuigiFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterStateDeltasLow),
    sNdsNativeLuigiFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterStateSequenceLow),
    sNdsNativeLuigiFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterVertexActionsLow),
    sNdsNativeLuigiFighterEpochDirectPolicyLow,
    sNdsNativeLuigiFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterDenseVerticesLow),
    sNdsNativeLuigiFighterDenseNormalsLow,
    sNdsNativeLuigiFighterPreparedDenseLow,
    sNdsNativeLuigiFighterActionDenseSpansLow,
    sNdsNativeLuigiFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterPackedCornersLow),
    sNdsNativeLuigiFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterRunFirstCornerLow),
    sNdsNativeLuigiFighterRunFirstUniqueLow,
    sNdsNativeLuigiFighterRunUniqueCountLow,
    sNdsNativeLuigiFighterRunUniqueDenseLow,
    sNdsNativeLuigiFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterTrianglesLow),
    sNdsNativeLuigiFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterRunsLow),
    sNdsNativeLuigiFighterPrimitiveGroupFirstLow,
    sNdsNativeLuigiFighterPrimitiveGroupCountLow,
    sNdsNativeLuigiFighterPrimitiveGroupTypeLow,
    sNdsNativeLuigiFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeLuigiFighterPrimitiveGroupVertexCountLow,
    sNdsNativeLuigiFighterPrimitiveVerticesLow,
    sNdsNativeLuigiFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeLuigiFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLuigiHighOwner, &sNdsNativeLuigiFighterHighTables,
    sNdsNativeLuigiRoots, sNdsNativeLuigiCrossPaletteSlots,
    sNdsNativeLuigiRootLightPreambles, NDS_NATIVE_LUIGI_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLuigiLowOwner, &sNdsNativeLuigiFighterLowTables,
    sNdsNativeLuigiRootsLow, sNdsNativeLuigiCrossPaletteSlotsLow,
    sNdsNativeLuigiRootLightPreambles, NDS_NATIVE_LUIGI_MODEL_DATA_SIZE);
#endif

#if NDS_P2_DONKEY
#if NDS_NATIVE_OWNER_IMAGE_DONKEY
/* P2-3r4: these tables are FILLED AT LOAD from the owner image, so they are
 * mutable and start empty.  The arrays that used to initialise them are
 * guarded out of this binary by the same flag; there is exactly one copy of
 * the bytes and it is the NitroFS one.  Every reader goes through the owner
 * runtime, which is only reachable after ndsRendererNativeEnsureOwnerImage
 * has bound this struct. */
static NDSNativeFighterRuntimeTables sNdsNativeDonkeyFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeDonkeyFighterHighTables =
{
    sNdsNativeDonkeyFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterStateDeltas),
    sNdsNativeDonkeyFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterStateSequence),
    sNdsNativeDonkeyFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterVertexActions),
    sNdsNativeDonkeyFighterEpochDirectPolicy,
    sNdsNativeDonkeyFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterDenseVertices),
    sNdsNativeDonkeyFighterDenseNormals,
    sNdsNativeDonkeyFighterPreparedDense,
    sNdsNativeDonkeyFighterActionDenseSpans,
    sNdsNativeDonkeyFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterPackedCorners),
    sNdsNativeDonkeyFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterRunFirstCorner),
    sNdsNativeDonkeyFighterRunFirstUnique,
    sNdsNativeDonkeyFighterRunUniqueCount,
    sNdsNativeDonkeyFighterRunUniqueDense,
    sNdsNativeDonkeyFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterTriangles),
    sNdsNativeDonkeyFighterRuns,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterRuns),
    sNdsNativeDonkeyFighterPrimitiveGroupFirst,
    sNdsNativeDonkeyFighterPrimitiveGroupCount,
    sNdsNativeDonkeyFighterPrimitiveGroupType,
    sNdsNativeDonkeyFighterPrimitiveGroupFirstVertex,
    sNdsNativeDonkeyFighterPrimitiveGroupVertexCount,
    sNdsNativeDonkeyFighterPrimitiveVertices,
    sNdsNativeDonkeyFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_DONKEY
/* P2-3r4: these tables are FILLED AT LOAD from the owner image, so they are
 * mutable and start empty.  The arrays that used to initialise them are
 * guarded out of this binary by the same flag; there is exactly one copy of
 * the bytes and it is the NitroFS one.  Every reader goes through the owner
 * runtime, which is only reachable after ndsRendererNativeEnsureOwnerImage
 * has bound this struct. */
static NDSNativeFighterRuntimeTables sNdsNativeDonkeyFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeDonkeyFighterLowTables =
{
    sNdsNativeDonkeyFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterStateDeltasLow),
    sNdsNativeDonkeyFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterStateSequenceLow),
    sNdsNativeDonkeyFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterVertexActionsLow),
    sNdsNativeDonkeyFighterEpochDirectPolicyLow,
    sNdsNativeDonkeyFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterDenseVerticesLow),
    sNdsNativeDonkeyFighterDenseNormalsLow,
    sNdsNativeDonkeyFighterPreparedDenseLow,
    sNdsNativeDonkeyFighterActionDenseSpansLow,
    sNdsNativeDonkeyFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterPackedCornersLow),
    sNdsNativeDonkeyFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterRunFirstCornerLow),
    sNdsNativeDonkeyFighterRunFirstUniqueLow,
    sNdsNativeDonkeyFighterRunUniqueCountLow,
    sNdsNativeDonkeyFighterRunUniqueDenseLow,
    sNdsNativeDonkeyFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterTrianglesLow),
    sNdsNativeDonkeyFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterRunsLow),
    sNdsNativeDonkeyFighterPrimitiveGroupFirstLow,
    sNdsNativeDonkeyFighterPrimitiveGroupCountLow,
    sNdsNativeDonkeyFighterPrimitiveGroupTypeLow,
    sNdsNativeDonkeyFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeDonkeyFighterPrimitiveGroupVertexCountLow,
    sNdsNativeDonkeyFighterPrimitiveVerticesLow,
    sNdsNativeDonkeyFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeDonkeyFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeDonkeyHighOwner, &sNdsNativeDonkeyFighterHighTables,
    sNdsNativeDonkeyRoots, sNdsNativeDonkeyCrossPaletteSlots,
    sNdsNativeDonkeyRootLightPreambles, NDS_NATIVE_DONKEY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeDonkeyLowOwner, &sNdsNativeDonkeyFighterLowTables,
    sNdsNativeDonkeyRootsLow, sNdsNativeDonkeyCrossPaletteSlotsLow,
    sNdsNativeDonkeyRootLightPreambles, NDS_NATIVE_DONKEY_MODEL_DATA_SIZE);
#endif

#if NDS_P2_CAPTAIN
#if NDS_NATIVE_OWNER_IMAGE_CAPTAIN
/* P2-3r4: these tables are FILLED AT LOAD from the owner image, so they are
 * mutable and start empty.  The arrays that used to initialise them are
 * guarded out of this binary by the same flag; there is exactly one copy of
 * the bytes and it is the NitroFS one.  Every reader goes through the owner
 * runtime, which is only reachable after ndsRendererNativeEnsureOwnerImage
 * has bound this struct. */
static NDSNativeFighterRuntimeTables sNdsNativeCaptainFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeCaptainFighterHighTables =
{
    sNdsNativeCaptainFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterStateDeltas),
    sNdsNativeCaptainFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterStateSequence),
    sNdsNativeCaptainFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterVertexActions),
    sNdsNativeCaptainFighterEpochDirectPolicy,
    sNdsNativeCaptainFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterDenseVertices),
    sNdsNativeCaptainFighterDenseNormals,
    sNdsNativeCaptainFighterPreparedDense,
    sNdsNativeCaptainFighterActionDenseSpans,
    sNdsNativeCaptainFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterPackedCorners),
    sNdsNativeCaptainFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterRunFirstCorner),
    sNdsNativeCaptainFighterRunFirstUnique,
    sNdsNativeCaptainFighterRunUniqueCount,
    sNdsNativeCaptainFighterRunUniqueDense,
    sNdsNativeCaptainFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterTriangles),
    sNdsNativeCaptainFighterRuns,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterRuns),
    sNdsNativeCaptainFighterPrimitiveGroupFirst,
    sNdsNativeCaptainFighterPrimitiveGroupCount,
    sNdsNativeCaptainFighterPrimitiveGroupType,
    sNdsNativeCaptainFighterPrimitiveGroupFirstVertex,
    sNdsNativeCaptainFighterPrimitiveGroupVertexCount,
    sNdsNativeCaptainFighterPrimitiveVertices,
    sNdsNativeCaptainFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_CAPTAIN
/* P2-3r4: these tables are FILLED AT LOAD from the owner image, so they are
 * mutable and start empty.  The arrays that used to initialise them are
 * guarded out of this binary by the same flag; there is exactly one copy of
 * the bytes and it is the NitroFS one.  Every reader goes through the owner
 * runtime, which is only reachable after ndsRendererNativeEnsureOwnerImage
 * has bound this struct. */
static NDSNativeFighterRuntimeTables sNdsNativeCaptainFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeCaptainFighterLowTables =
{
    sNdsNativeCaptainFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterStateDeltasLow),
    sNdsNativeCaptainFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterStateSequenceLow),
    sNdsNativeCaptainFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterVertexActionsLow),
    sNdsNativeCaptainFighterEpochDirectPolicyLow,
    sNdsNativeCaptainFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterDenseVerticesLow),
    sNdsNativeCaptainFighterDenseNormalsLow,
    sNdsNativeCaptainFighterPreparedDenseLow,
    sNdsNativeCaptainFighterActionDenseSpansLow,
    sNdsNativeCaptainFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterPackedCornersLow),
    sNdsNativeCaptainFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterRunFirstCornerLow),
    sNdsNativeCaptainFighterRunFirstUniqueLow,
    sNdsNativeCaptainFighterRunUniqueCountLow,
    sNdsNativeCaptainFighterRunUniqueDenseLow,
    sNdsNativeCaptainFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterTrianglesLow),
    sNdsNativeCaptainFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterRunsLow),
    sNdsNativeCaptainFighterPrimitiveGroupFirstLow,
    sNdsNativeCaptainFighterPrimitiveGroupCountLow,
    sNdsNativeCaptainFighterPrimitiveGroupTypeLow,
    sNdsNativeCaptainFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeCaptainFighterPrimitiveGroupVertexCountLow,
    sNdsNativeCaptainFighterPrimitiveVerticesLow,
    sNdsNativeCaptainFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeCaptainFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeCaptainHighOwner, &sNdsNativeCaptainFighterHighTables,
    sNdsNativeCaptainRoots, sNdsNativeCaptainCrossPaletteSlots,
    sNdsNativeCaptainRootLightPreambles, NDS_NATIVE_CAPTAIN_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeCaptainLowOwner, &sNdsNativeCaptainFighterLowTables,
    sNdsNativeCaptainRootsLow, sNdsNativeCaptainCrossPaletteSlotsLow,
    sNdsNativeCaptainRootLightPreambles, NDS_NATIVE_CAPTAIN_MODEL_DATA_SIZE);
#endif

static NDSNativeFighterRuntimeTables sNdsNativeSamusFighterHighTables;

static NDSNativeFighterRuntimeTables sNdsNativeSamusFighterLowTables;

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusHighOwner, &sNdsNativeSamusFighterHighTables,
    sNdsNativeSamusRoots, sNdsNativeSamusCrossPaletteSlots,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusLowOwner, &sNdsNativeSamusFighterLowTables,
    sNdsNativeSamusRootsLow, sNdsNativeSamusCrossPaletteSlotsLow,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_SAMUS_ROOT_PROGRAMS_PRESENT)
/* Catch enables Samus's hidden grapple chain, growing the live source root
 * vector from 14 to 21.  The program-only appendix bakes the three new source
 * DL identities; five chain links intentionally reuse the self-contained
 * 0x9140 root under five different live DObj matrices. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusCatchHighOwner, &sNdsNativeSamusFighterHighTables,
    sNdsNativeSamusCatchRoots, sNdsNativeSamusCatchCrossPaletteSlots,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusCatchLowOwner, &sNdsNativeSamusFighterLowTables,
    sNdsNativeSamusCatchRootsLow, sNdsNativeSamusCatchCrossPaletteSlotsLow,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusMorphUnfoldHighOwner, &sNdsNativeSamusFighterHighTables,
    sNdsNativeSamusMorphUnfoldRoots, sNdsNativeSamusMorphUnfoldCrossPaletteSlots,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusMorphUnfoldLowOwner, &sNdsNativeSamusFighterLowTables,
    sNdsNativeSamusMorphUnfoldRootsLow, sNdsNativeSamusMorphUnfoldCrossPaletteSlotsLow,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusMorphBallHighOwner, &sNdsNativeSamusFighterHighTables,
    sNdsNativeSamusMorphBallRoots, sNdsNativeSamusMorphBallCrossPaletteSlots,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusMorphBallLowOwner, &sNdsNativeSamusFighterLowTables,
    sNdsNativeSamusMorphBallRootsLow, sNdsNativeSamusMorphBallCrossPaletteSlotsLow,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
/* The forward smash carries 0x00180000, which installs the same hidden joints
 * 24 and 25 Catch does -- but it issues no model-part command, so they draw
 * 0x2c20 and 0x2ce8 instead of Catch's replacement and hide. That is a 16-root
 * vector against a canonical 14, and neither offset was resident before,
 * because Catch's motion overrides both joints away. All five FSmash motions
 * share this one program. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusFSmashHighOwner, &sNdsNativeSamusFighterHighTables,
    sNdsNativeSamusFSmashRoots, sNdsNativeSamusFSmashCrossPaletteSlots,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusFSmashLowOwner, &sNdsNativeSamusFighterLowTables,
    sNdsNativeSamusFSmashRootsLow, sNdsNativeSamusFSmashCrossPaletteSlotsLow,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
#endif

/* Link's hand-held Neutral-B model and Kirby CopyLink's hidden joint 12 both
 * resolve to LinkBoomerangModel asset 0x146 root 0xF8.  Its compact indices
 * belong to this tiny donor table set, never to LinkModel/KirbyModel tables. */
static const NDSNativeFighterRuntimeTables
sNdsNativeLinkBoomerangFighterHighTables =
{
    sNdsNativeLinkBoomerangFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterStateDeltas),
    sNdsNativeLinkBoomerangFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterStateSequence),
    sNdsNativeLinkBoomerangFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterVertexActions),
    sNdsNativeLinkBoomerangFighterEpochDirectPolicy,
    sNdsNativeLinkBoomerangFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterDenseVertices),
    sNdsNativeLinkBoomerangFighterDenseNormals,
    sNdsNativeLinkBoomerangFighterPreparedDense,
    sNdsNativeLinkBoomerangFighterActionDenseSpans,
    sNdsNativeLinkBoomerangFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterPackedCorners),
    sNdsNativeLinkBoomerangFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterRunFirstCorner),
    sNdsNativeLinkBoomerangFighterRunFirstUnique,
    sNdsNativeLinkBoomerangFighterRunUniqueCount,
    sNdsNativeLinkBoomerangFighterRunUniqueDense,
    sNdsNativeLinkBoomerangFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterTriangles),
    sNdsNativeLinkBoomerangFighterRuns,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterRuns),
    sNdsNativeLinkBoomerangFighterPrimitiveGroupFirst,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupCount,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupType,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupFirstVertex,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupVertexCount,
    sNdsNativeLinkBoomerangFighterPrimitiveVertices,
    sNdsNativeLinkBoomerangFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterEpochs)
};

static const NDSNativeFighterRuntimeTables
sNdsNativeLinkBoomerangFighterLowTables =
{
    sNdsNativeLinkBoomerangFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterStateDeltasLow),
    sNdsNativeLinkBoomerangFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterStateSequenceLow),
    sNdsNativeLinkBoomerangFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterVertexActionsLow),
    sNdsNativeLinkBoomerangFighterEpochDirectPolicyLow,
    sNdsNativeLinkBoomerangFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterDenseVerticesLow),
    sNdsNativeLinkBoomerangFighterDenseNormalsLow,
    sNdsNativeLinkBoomerangFighterPreparedDenseLow,
    sNdsNativeLinkBoomerangFighterActionDenseSpansLow,
    sNdsNativeLinkBoomerangFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterPackedCornersLow),
    sNdsNativeLinkBoomerangFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterRunFirstCornerLow),
    sNdsNativeLinkBoomerangFighterRunFirstUniqueLow,
    sNdsNativeLinkBoomerangFighterRunUniqueCountLow,
    sNdsNativeLinkBoomerangFighterRunUniqueDenseLow,
    sNdsNativeLinkBoomerangFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterTrianglesLow),
    sNdsNativeLinkBoomerangFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterRunsLow),
    sNdsNativeLinkBoomerangFighterPrimitiveGroupFirstLow,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupCountLow,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupTypeLow,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeLinkBoomerangFighterPrimitiveGroupVertexCountLow,
    sNdsNativeLinkBoomerangFighterPrimitiveVerticesLow,
    sNdsNativeLinkBoomerangFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeLinkBoomerangFighterEpochsLow)
};

#if NDS_P2_LINK
#if NDS_NATIVE_OWNER_IMAGE_LINK
static NDSNativeFighterRuntimeTables sNdsNativeLinkFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeLinkFighterHighTables =
{
    sNdsNativeLinkFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeLinkFighterStateDeltas),
    sNdsNativeLinkFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeLinkFighterStateSequence),
    sNdsNativeLinkFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeLinkFighterVertexActions),
    sNdsNativeLinkFighterEpochDirectPolicy,
    sNdsNativeLinkFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeLinkFighterDenseVertices),
    sNdsNativeLinkFighterDenseNormals,
    sNdsNativeLinkFighterPreparedDense,
    sNdsNativeLinkFighterActionDenseSpans,
    sNdsNativeLinkFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeLinkFighterPackedCorners),
    sNdsNativeLinkFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeLinkFighterRunFirstCorner),
    sNdsNativeLinkFighterRunFirstUnique,
    sNdsNativeLinkFighterRunUniqueCount,
    sNdsNativeLinkFighterRunUniqueDense,
    sNdsNativeLinkFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeLinkFighterTriangles),
    sNdsNativeLinkFighterRuns,
    NDS_FTR_COUNT(sNdsNativeLinkFighterRuns),
    sNdsNativeLinkFighterPrimitiveGroupFirst,
    sNdsNativeLinkFighterPrimitiveGroupCount,
    sNdsNativeLinkFighterPrimitiveGroupType,
    sNdsNativeLinkFighterPrimitiveGroupFirstVertex,
    sNdsNativeLinkFighterPrimitiveGroupVertexCount,
    sNdsNativeLinkFighterPrimitiveVertices,
    sNdsNativeLinkFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeLinkFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_LINK
static NDSNativeFighterRuntimeTables sNdsNativeLinkFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeLinkFighterLowTables =
{
    sNdsNativeLinkFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterStateDeltasLow),
    sNdsNativeLinkFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterStateSequenceLow),
    sNdsNativeLinkFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterVertexActionsLow),
    sNdsNativeLinkFighterEpochDirectPolicyLow,
    sNdsNativeLinkFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterDenseVerticesLow),
    sNdsNativeLinkFighterDenseNormalsLow,
    sNdsNativeLinkFighterPreparedDenseLow,
    sNdsNativeLinkFighterActionDenseSpansLow,
    sNdsNativeLinkFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterPackedCornersLow),
    sNdsNativeLinkFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterRunFirstCornerLow),
    sNdsNativeLinkFighterRunFirstUniqueLow,
    sNdsNativeLinkFighterRunUniqueCountLow,
    sNdsNativeLinkFighterRunUniqueDenseLow,
    sNdsNativeLinkFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterTrianglesLow),
    sNdsNativeLinkFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterRunsLow),
    sNdsNativeLinkFighterPrimitiveGroupFirstLow,
    sNdsNativeLinkFighterPrimitiveGroupCountLow,
    sNdsNativeLinkFighterPrimitiveGroupTypeLow,
    sNdsNativeLinkFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeLinkFighterPrimitiveGroupVertexCountLow,
    sNdsNativeLinkFighterPrimitiveVerticesLow,
    sNdsNativeLinkFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeLinkFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkHighOwner, &sNdsNativeLinkFighterHighTables,
    sNdsNativeLinkRoots, sNdsNativeLinkCrossPaletteSlots,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkLowOwner, &sNdsNativeLinkFighterLowTables,
    sNdsNativeLinkRootsLow, sNdsNativeLinkCrossPaletteSlotsLow,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkEntryHighOwner, &sNdsNativeLinkFighterHighTables,
    sNdsNativeLinkEntryRoots, sNdsNativeLinkEntryCrossPaletteSlots,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkEntryLowOwner, &sNdsNativeLinkFighterLowTables,
    sNdsNativeLinkEntryRootsLow, sNdsNativeLinkEntryCrossPaletteSlotsLow,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkSpecialNHighOwner, &sNdsNativeLinkFighterHighTables,
    sNdsNativeLinkSpecialNRoots, sNdsNativeLinkSpecialNCrossPaletteSlots,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkSpecialNLowOwner, &sNdsNativeLinkFighterLowTables,
    sNdsNativeLinkSpecialNRootsLow, sNdsNativeLinkSpecialNCrossPaletteSlotsLow,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkCatchHighOwner, &sNdsNativeLinkFighterHighTables,
    sNdsNativeLinkCatchRoots, sNdsNativeLinkCatchCrossPaletteSlots,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkCatchLowOwner, &sNdsNativeLinkFighterLowTables,
    sNdsNativeLinkCatchRootsLow, sNdsNativeLinkCatchCrossPaletteSlotsLow,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
/* VS Results, the loser's pose: Entry's roots with the shield on the back. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkClapsHighOwner, &sNdsNativeLinkFighterHighTables,
    sNdsNativeLinkClapsRoots, sNdsNativeLinkClapsCrossPaletteSlots,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkClapsLowOwner, &sNdsNativeLinkFighterLowTables,
    sNdsNativeLinkClapsRootsLow, sNdsNativeLinkClapsCrossPaletteSlotsLow,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
/* Neutral-B with an item in hand: SpecialN's boomerang on Claps's
 * shield-on-back tree. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkSpecialNItemHighOwner, &sNdsNativeLinkFighterHighTables,
    sNdsNativeLinkSpecialNItemRoots, sNdsNativeLinkSpecialNItemCrossPaletteSlots,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkSpecialNItemLowOwner, &sNdsNativeLinkFighterLowTables,
    sNdsNativeLinkSpecialNItemRootsLow,
    sNdsNativeLinkSpecialNItemCrossPaletteSlotsLow,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_LINK_INTRO_PROGRAM_PRESENT)
/* The 1P intro poses: canonical's tree with four display lists replaced.
 * Generated only for the intro-still bake (NDS_1P_INTRO_BAKE). */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkIntroHighOwner, &sNdsNativeLinkFighterHighTables,
    sNdsNativeLinkIntroRoots, sNdsNativeLinkIntroCrossPaletteSlots,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeLinkIntroLowOwner, &sNdsNativeLinkFighterLowTables,
    sNdsNativeLinkIntroRootsLow, sNdsNativeLinkIntroCrossPaletteSlotsLow,
    sNdsNativeLinkRootLightPreambles, NDS_NATIVE_LINK_MODEL_DATA_SIZE);
#endif
#endif
#endif

static NDSNativeFighterRuntimeTables sNdsNativePikachuFighterHighTables;

static NDSNativeFighterRuntimeTables sNdsNativePikachuFighterLowTables;

NDS_FTR_OWNER_RUNTIME(
    sNdsNativePikachuHighOwner, &sNdsNativePikachuFighterHighTables,
    sNdsNativePikachuRoots, sNdsNativePikachuCrossPaletteSlots,
    sNdsNativePikachuRootLightPreambles, NDS_NATIVE_PIKACHU_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePikachuLowOwner, &sNdsNativePikachuFighterLowTables,
    sNdsNativePikachuRootsLow, sNdsNativePikachuCrossPaletteSlotsLow,
    sNdsNativePikachuRootLightPreambles, NDS_NATIVE_PIKACHU_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_PIKACHU_ROOT_PROGRAMS_PRESENT)
/* A costume's hat: canonical with the accessory list after joint 11's. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePikachuAccessoryHighOwner, &sNdsNativePikachuFighterHighTables,
    sNdsNativePikachuAccessoryRoots, sNdsNativePikachuAccessoryCrossPaletteSlots,
    sNdsNativePikachuRootLightPreambles, NDS_NATIVE_PIKACHU_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePikachuAccessoryLowOwner, &sNdsNativePikachuFighterLowTables,
    sNdsNativePikachuAccessoryRootsLow,
    sNdsNativePikachuAccessoryCrossPaletteSlotsLow,
    sNdsNativePikachuRootLightPreambles, NDS_NATIVE_PIKACHU_MODEL_DATA_SIZE);
#endif

static NDSNativeFighterRuntimeTables sNdsNativeYoshiFighterHighTables;

static NDSNativeFighterRuntimeTables sNdsNativeYoshiFighterLowTables;

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiHighOwner, &sNdsNativeYoshiFighterHighTables,
    sNdsNativeYoshiRoots, sNdsNativeYoshiCrossPaletteSlots,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiLowOwner, &sNdsNativeYoshiFighterLowTables,
    sNdsNativeYoshiRootsLow, sNdsNativeYoshiCrossPaletteSlotsLow,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_YOSHI_ROOT_PROGRAMS_PRESENT)
/* The grab family's 0x18000000 anim-desc mask installs hidden part 4, whose
 * joint 9 draws 0x2800 in both details. That ADDS a root, so the live vector
 * is 19 against a canonical 18 and no per-binding variant can represent it --
 * the resolver rejects on root_count before it ever compares offsets. Catch
 * keeps joint 7 canonical (Catch/CatchPull and EggLay 202-206); Throw is the
 * same vector with joint 7 on model part 1, for the ThrowF/ThrowB window. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiCatchHighOwner, &sNdsNativeYoshiFighterHighTables,
    sNdsNativeYoshiCatchRoots, sNdsNativeYoshiCatchCrossPaletteSlots,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiCatchLowOwner, &sNdsNativeYoshiFighterLowTables,
    sNdsNativeYoshiCatchRootsLow, sNdsNativeYoshiCatchCrossPaletteSlotsLow,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiThrowHighOwner, &sNdsNativeYoshiFighterHighTables,
    sNdsNativeYoshiThrowRoots, sNdsNativeYoshiThrowCrossPaletteSlots,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiThrowLowOwner, &sNdsNativeYoshiFighterLowTables,
    sNdsNativeYoshiThrowRootsLow, sNdsNativeYoshiThrowCrossPaletteSlotsLow,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
#endif

static NDSNativeFighterRuntimeTables sNdsNativeNessFighterHighTables;

static NDSNativeFighterRuntimeTables sNdsNativeNessFighterLowTables;

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessHighOwner, &sNdsNativeNessFighterHighTables,
    sNdsNativeNessRoots, sNdsNativeNessCrossPaletteSlots,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessLowOwner, &sNdsNativeNessFighterLowTables,
    sNdsNativeNessRootsLow, sNdsNativeNessCrossPaletteSlotsLow,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_NESS_ROOT_PROGRAMS_PRESENT)
/* VS Results, the winner's third pose. scsubsysdataness.c D_ovl1_8039272C is
 * dFTNessSubMotionDescs rows 3 and 4, and its first word is the raw
 * 0xA0880001 -- SetModelPartID(17, 1), never restored. dNessMain_setup_parts
 * 0xFFFFFFC0 SELECTS descriptor 13, but the JointTree gives joint 17 no
 * display list in either detail, so the write ADDS a root instead of
 * replacing one: the live vector is 15 against a canonical 14 and the
 * resolver rejects on root_count before it ever compares offsets. No
 * per-binding variant can carry it. Without this program the owner declines,
 * and a packed fighter that declines is halt 20 -- the same freeze Link's
 * Claps pose caused when he lost a match. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessWin3HighOwner, &sNdsNativeNessFighterHighTables,
    sNdsNativeNessWin3Roots, sNdsNativeNessWin3CrossPaletteSlots,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessWin3LowOwner, &sNdsNativeNessFighterLowTables,
    sNdsNativeNessWin3RootsLow, sNdsNativeNessWin3CrossPaletteSlotsLow,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
/* P2-2p8 Phase 1 slice 7: the yo-yo smashes. USmash and DSmash carry the
 * 0x10000000 anim-desc mask, so ftMainSetStatus installs hidden part 3 --
 * joint 30, the last child of joint 4, with no display list -- and the
 * motion's SetModelPartID(30, 0) two frames in draws the yo-yo (0x69E0, both
 * details) on it. That ADDS a root, last in the walk: 15 against a canonical
 * 14. Without this program the owner declined for the rest of either smash
 * and Ness drew nothing there, in both renderer routes. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessYoYoHighOwner, &sNdsNativeNessFighterHighTables,
    sNdsNativeNessYoYoRoots, sNdsNativeNessYoYoCrossPaletteSlots,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessYoYoLowOwner, &sNdsNativeNessFighterLowTables,
    sNdsNativeNessYoYoRootsLow, sNdsNativeNessYoYoCrossPaletteSlotsLow,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
/* Slice 7: the forward smash. dNessMainMotion_FSmash's SetModelPartID(17, 0)
 * puts the bat (0x6B50, both details) on joint 17 -- Win3's selected-but-blank
 * joint -- so it ADDS a root at joint 17's place, 15 against 14, and Ness
 * drew nothing for the rest of the smash until this program. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessFSmashHighOwner, &sNdsNativeNessFighterHighTables,
    sNdsNativeNessFSmashRoots, sNdsNativeNessFSmashCrossPaletteSlots,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessFSmashLowOwner, &sNdsNativeNessFighterLowTables,
    sNdsNativeNessFSmashRootsLow, sNdsNativeNessFSmashCrossPaletteSlotsLow,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
#endif

static NDSNativeFighterRuntimeTables sNdsNativePurinFighterHighTables;

static NDSNativeFighterRuntimeTables sNdsNativePurinFighterLowTables;

NDS_FTR_OWNER_RUNTIME(
    sNdsNativePurinHighOwner, &sNdsNativePurinFighterHighTables,
    sNdsNativePurinRoots, sNdsNativePurinCrossPaletteSlots,
    sNdsNativePurinRootLightPreambles, NDS_NATIVE_PURIN_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePurinLowOwner, &sNdsNativePurinFighterLowTables,
    sNdsNativePurinRootsLow, sNdsNativePurinCrossPaletteSlotsLow,
    sNdsNativePurinRootLightPreambles, NDS_NATIVE_PURIN_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_PURIN_ROOT_PROGRAMS_PRESENT)
/* A costume's bow or hat: canonical with the accessory list before joint
 * 6's. */
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePurinAccessoryHighOwner, &sNdsNativePurinFighterHighTables,
    sNdsNativePurinAccessoryRoots, sNdsNativePurinAccessoryCrossPaletteSlots,
    sNdsNativePurinRootLightPreambles, NDS_NATIVE_PURIN_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePurinAccessoryLowOwner, &sNdsNativePurinFighterLowTables,
    sNdsNativePurinAccessoryRootsLow,
    sNdsNativePurinAccessoryCrossPaletteSlotsLow,
    sNdsNativePurinRootLightPreambles, NDS_NATIVE_PURIN_MODEL_DATA_SIZE);
#endif

static NDSNativeFighterRuntimeTables sNdsNativeKirbyFighterHighTables;

static NDSNativeFighterRuntimeTables sNdsNativeKirbyFighterLowTables;

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyHighOwner, &sNdsNativeKirbyFighterHighTables,
    sNdsNativeKirbyRoots, sNdsNativeKirbyCrossPaletteSlots,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyLowOwner, &sNdsNativeKirbyFighterLowTables,
    sNdsNativeKirbyRootsLow, sNdsNativeKirbyCrossPaletteSlotsLow,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
#if defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)
/* Kirby has complete alternate source root vectors, not per-root exceptions:
 * SpecialN is 9 roots (head1 + body + joint19), Link-copy is 10 roots
 * (head14 + body + joint18 + joint19), and Stone hides the ordinary body then
 * exposes only joint-6 modelpart 2. All geometry/state tables remain the same
 * loaded Kirby owner image; only these tiny root/cross vectors select the live
 * source program. */
/* One owner pair per reachable joint-6 head, expanded from the generated
 * NDS_NATIVE_KIRBY_TRIO_HEAD_LIST so a head admitted in the bake cannot be
 * left half-wired here. Heads 1 and 14 are Kirby's own faces; the rest are
 * copy hats, whose root 0 resolves into the per-slot hat image through
 * their SourceOwners table exactly as CopyLink's does. */
#define NDS_KIRBY_TRIO_OWNER_PAIR(head_)                                           NDS_FTR_OWNER_RUNTIME(                                                             sNdsNativeKirbyTrioHead##head_##HighOwner,                                     &sNdsNativeKirbyFighterHighTables,                                             sNdsNativeKirbyTrioHead##head_##Roots,                                         sNdsNativeKirbyTrioHead##head_##CrossPaletteSlots,                             sNdsNativeKirbyRootLightPreambles,                                             NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);                                         NDS_FTR_OWNER_RUNTIME(                                                             sNdsNativeKirbyTrioHead##head_##LowOwner,                                      &sNdsNativeKirbyFighterLowTables,                                              sNdsNativeKirbyTrioHead##head_##RootsLow,                                      sNdsNativeKirbyTrioHead##head_##CrossPaletteSlotsLow,                          sNdsNativeKirbyRootLightPreambles,                                             NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_OWNER_PAIR)
#undef NDS_KIRBY_TRIO_OWNER_PAIR

/* Program p in 1..NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT is trio head
 * sNdsKirbyTrioProgramHead[p - 1]; the three arrays share one expansion so
 * their order is the generated order by construction. */
static const NDSNativeFighterOwnerRuntime *const
sNdsKirbyTrioHighOwners[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) &sNdsNativeKirbyTrioHead##head_##HighOwner,
    NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
};
static const NDSNativeFighterOwnerRuntime *const
sNdsKirbyTrioLowOwners[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) &sNdsNativeKirbyTrioHead##head_##LowOwner,
    NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
};
static const u8 sNdsKirbyTrioProgramHead[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) (u8)(head_),
    NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
};
/* Root 0 of a copy hat's program lives in the deferred hat image, so the hat
 * heads carry a SourceOwners table and the two face heads do not. NULL means
 * "single-file program"; the resolve below then keeps the owner's tables. */
static const u8 *const
sNdsKirbyTrioSourceOwners[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) NDS_NATIVE_KIRBY_TRIO_SOURCE_OWNERS_##head_,
    NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
};
static const u8
sNdsKirbyTrioSourceOwnerCount[NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT] = {
#define NDS_KIRBY_TRIO_X(head_) NDS_NATIVE_KIRBY_TRIO_SOURCE_OWNER_COUNT_##head_,
    NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(NDS_KIRBY_TRIO_X)
#undef NDS_KIRBY_TRIO_X
};
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyStoneHighOwner, &sNdsNativeKirbyFighterHighTables,
    sNdsNativeKirbyStoneRoots, sNdsNativeKirbyStoneCrossPaletteSlots,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyStoneLowOwner, &sNdsNativeKirbyFighterLowTables,
    sNdsNativeKirbyStoneRootsLow, sNdsNativeKirbyStoneCrossPaletteSlotsLow,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyCopyLinkHighOwner, &sNdsNativeKirbyFighterHighTables,
    sNdsNativeKirbyCopyLinkRoots, sNdsNativeKirbyCopyLinkCrossPaletteSlots,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyCopyLinkLowOwner, &sNdsNativeKirbyFighterLowTables,
    sNdsNativeKirbyCopyLinkRootsLow, sNdsNativeKirbyCopyLinkCrossPaletteSlotsLow,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyCopyTransitionHighOwner, &sNdsNativeKirbyFighterHighTables,
    sNdsNativeKirbyCopyTransitionRoots,
    sNdsNativeKirbyCopyTransitionCrossPaletteSlots,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyCopyTransitionLowOwner, &sNdsNativeKirbyFighterLowTables,
    sNdsNativeKirbyCopyTransitionRootsLow,
    sNdsNativeKirbyCopyTransitionCrossPaletteSlotsLow,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
#endif

#if NDS_P2_MMARIO
#if NDS_NATIVE_OWNER_IMAGE_MMARIO
static NDSNativeFighterRuntimeTables sNdsNativeMMarioFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeMMarioFighterHighTables =
{
    sNdsNativeMMarioFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterStateDeltas),
    sNdsNativeMMarioFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterStateSequence),
    sNdsNativeMMarioFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterVertexActions),
    sNdsNativeMMarioFighterEpochDirectPolicy,
    sNdsNativeMMarioFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterDenseVertices),
    sNdsNativeMMarioFighterDenseNormals,
    sNdsNativeMMarioFighterPreparedDense,
    sNdsNativeMMarioFighterActionDenseSpans,
    sNdsNativeMMarioFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterPackedCorners),
    sNdsNativeMMarioFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterRunFirstCorner),
    sNdsNativeMMarioFighterRunFirstUnique,
    sNdsNativeMMarioFighterRunUniqueCount,
    sNdsNativeMMarioFighterRunUniqueDense,
    sNdsNativeMMarioFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterTriangles),
    sNdsNativeMMarioFighterRuns,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterRuns),
    sNdsNativeMMarioFighterPrimitiveGroupFirst,
    sNdsNativeMMarioFighterPrimitiveGroupCount,
    sNdsNativeMMarioFighterPrimitiveGroupType,
    sNdsNativeMMarioFighterPrimitiveGroupFirstVertex,
    sNdsNativeMMarioFighterPrimitiveGroupVertexCount,
    sNdsNativeMMarioFighterPrimitiveVertices,
    sNdsNativeMMarioFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_MMARIO
static NDSNativeFighterRuntimeTables sNdsNativeMMarioFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeMMarioFighterLowTables =
{
    sNdsNativeMMarioFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterStateDeltasLow),
    sNdsNativeMMarioFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterStateSequenceLow),
    sNdsNativeMMarioFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterVertexActionsLow),
    sNdsNativeMMarioFighterEpochDirectPolicyLow,
    sNdsNativeMMarioFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterDenseVerticesLow),
    sNdsNativeMMarioFighterDenseNormalsLow,
    sNdsNativeMMarioFighterPreparedDenseLow,
    sNdsNativeMMarioFighterActionDenseSpansLow,
    sNdsNativeMMarioFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterPackedCornersLow),
    sNdsNativeMMarioFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterRunFirstCornerLow),
    sNdsNativeMMarioFighterRunFirstUniqueLow,
    sNdsNativeMMarioFighterRunUniqueCountLow,
    sNdsNativeMMarioFighterRunUniqueDenseLow,
    sNdsNativeMMarioFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterTrianglesLow),
    sNdsNativeMMarioFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterRunsLow),
    sNdsNativeMMarioFighterPrimitiveGroupFirstLow,
    sNdsNativeMMarioFighterPrimitiveGroupCountLow,
    sNdsNativeMMarioFighterPrimitiveGroupTypeLow,
    sNdsNativeMMarioFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeMMarioFighterPrimitiveGroupVertexCountLow,
    sNdsNativeMMarioFighterPrimitiveVerticesLow,
    sNdsNativeMMarioFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeMMarioFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeMMarioHighOwner, &sNdsNativeMMarioFighterHighTables,
    sNdsNativeMMarioRoots, sNdsNativeMMarioCrossPaletteSlots,
    sNdsNativeMMarioRootLightPreambles, NDS_NATIVE_MMARIO_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeMMarioLowOwner, &sNdsNativeMMarioFighterLowTables,
    sNdsNativeMMarioRootsLow, sNdsNativeMMarioCrossPaletteSlotsLow,
    sNdsNativeMMarioRootLightPreambles, NDS_NATIVE_MMARIO_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NMARIO
#if NDS_NATIVE_OWNER_IMAGE_NMARIO
static NDSNativeFighterRuntimeTables sNdsNativeNMarioFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNMarioFighterHighTables =
{
    sNdsNativeNMarioFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterStateDeltas),
    sNdsNativeNMarioFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterStateSequence),
    sNdsNativeNMarioFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterVertexActions),
    sNdsNativeNMarioFighterEpochDirectPolicy,
    sNdsNativeNMarioFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterDenseVertices),
    sNdsNativeNMarioFighterDenseNormals,
    sNdsNativeNMarioFighterPreparedDense,
    sNdsNativeNMarioFighterActionDenseSpans,
    sNdsNativeNMarioFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterPackedCorners),
    sNdsNativeNMarioFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterRunFirstCorner),
    sNdsNativeNMarioFighterRunFirstUnique,
    sNdsNativeNMarioFighterRunUniqueCount,
    sNdsNativeNMarioFighterRunUniqueDense,
    sNdsNativeNMarioFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterTriangles),
    sNdsNativeNMarioFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterRuns),
    sNdsNativeNMarioFighterPrimitiveGroupFirst,
    sNdsNativeNMarioFighterPrimitiveGroupCount,
    sNdsNativeNMarioFighterPrimitiveGroupType,
    sNdsNativeNMarioFighterPrimitiveGroupFirstVertex,
    sNdsNativeNMarioFighterPrimitiveGroupVertexCount,
    sNdsNativeNMarioFighterPrimitiveVertices,
    sNdsNativeNMarioFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NMARIO
static NDSNativeFighterRuntimeTables sNdsNativeNMarioFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNMarioFighterLowTables =
{
    sNdsNativeNMarioFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterStateDeltasLow),
    sNdsNativeNMarioFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterStateSequenceLow),
    sNdsNativeNMarioFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterVertexActionsLow),
    sNdsNativeNMarioFighterEpochDirectPolicyLow,
    sNdsNativeNMarioFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterDenseVerticesLow),
    sNdsNativeNMarioFighterDenseNormalsLow,
    sNdsNativeNMarioFighterPreparedDenseLow,
    sNdsNativeNMarioFighterActionDenseSpansLow,
    sNdsNativeNMarioFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterPackedCornersLow),
    sNdsNativeNMarioFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterRunFirstCornerLow),
    sNdsNativeNMarioFighterRunFirstUniqueLow,
    sNdsNativeNMarioFighterRunUniqueCountLow,
    sNdsNativeNMarioFighterRunUniqueDenseLow,
    sNdsNativeNMarioFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterTrianglesLow),
    sNdsNativeNMarioFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterRunsLow),
    sNdsNativeNMarioFighterPrimitiveGroupFirstLow,
    sNdsNativeNMarioFighterPrimitiveGroupCountLow,
    sNdsNativeNMarioFighterPrimitiveGroupTypeLow,
    sNdsNativeNMarioFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNMarioFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNMarioFighterPrimitiveVerticesLow,
    sNdsNativeNMarioFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNMarioFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNMarioHighOwner, &sNdsNativeNMarioFighterHighTables,
    sNdsNativeNMarioRoots, sNdsNativeNMarioCrossPaletteSlots,
    sNdsNativeNMarioRootLightPreambles, NDS_NATIVE_NMARIO_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNMarioLowOwner, &sNdsNativeNMarioFighterLowTables,
    sNdsNativeNMarioRootsLow, sNdsNativeNMarioCrossPaletteSlotsLow,
    sNdsNativeNMarioRootLightPreambles, NDS_NATIVE_NMARIO_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NFOX
#if NDS_NATIVE_OWNER_IMAGE_NFOX
static NDSNativeFighterRuntimeTables sNdsNativeNFoxFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNFoxFighterHighTables =
{
    sNdsNativeNFoxFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterStateDeltas),
    sNdsNativeNFoxFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterStateSequence),
    sNdsNativeNFoxFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterVertexActions),
    sNdsNativeNFoxFighterEpochDirectPolicy,
    sNdsNativeNFoxFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterDenseVertices),
    sNdsNativeNFoxFighterDenseNormals,
    sNdsNativeNFoxFighterPreparedDense,
    sNdsNativeNFoxFighterActionDenseSpans,
    sNdsNativeNFoxFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterPackedCorners),
    sNdsNativeNFoxFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterRunFirstCorner),
    sNdsNativeNFoxFighterRunFirstUnique,
    sNdsNativeNFoxFighterRunUniqueCount,
    sNdsNativeNFoxFighterRunUniqueDense,
    sNdsNativeNFoxFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterTriangles),
    sNdsNativeNFoxFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterRuns),
    sNdsNativeNFoxFighterPrimitiveGroupFirst,
    sNdsNativeNFoxFighterPrimitiveGroupCount,
    sNdsNativeNFoxFighterPrimitiveGroupType,
    sNdsNativeNFoxFighterPrimitiveGroupFirstVertex,
    sNdsNativeNFoxFighterPrimitiveGroupVertexCount,
    sNdsNativeNFoxFighterPrimitiveVertices,
    sNdsNativeNFoxFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NFOX
static NDSNativeFighterRuntimeTables sNdsNativeNFoxFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNFoxFighterLowTables =
{
    sNdsNativeNFoxFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterStateDeltasLow),
    sNdsNativeNFoxFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterStateSequenceLow),
    sNdsNativeNFoxFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterVertexActionsLow),
    sNdsNativeNFoxFighterEpochDirectPolicyLow,
    sNdsNativeNFoxFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterDenseVerticesLow),
    sNdsNativeNFoxFighterDenseNormalsLow,
    sNdsNativeNFoxFighterPreparedDenseLow,
    sNdsNativeNFoxFighterActionDenseSpansLow,
    sNdsNativeNFoxFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterPackedCornersLow),
    sNdsNativeNFoxFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterRunFirstCornerLow),
    sNdsNativeNFoxFighterRunFirstUniqueLow,
    sNdsNativeNFoxFighterRunUniqueCountLow,
    sNdsNativeNFoxFighterRunUniqueDenseLow,
    sNdsNativeNFoxFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterTrianglesLow),
    sNdsNativeNFoxFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterRunsLow),
    sNdsNativeNFoxFighterPrimitiveGroupFirstLow,
    sNdsNativeNFoxFighterPrimitiveGroupCountLow,
    sNdsNativeNFoxFighterPrimitiveGroupTypeLow,
    sNdsNativeNFoxFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNFoxFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNFoxFighterPrimitiveVerticesLow,
    sNdsNativeNFoxFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNFoxFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNFoxHighOwner, &sNdsNativeNFoxFighterHighTables,
    sNdsNativeNFoxRoots, sNdsNativeNFoxCrossPaletteSlots,
    sNdsNativeNFoxRootLightPreambles, NDS_NATIVE_NFOX_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNFoxLowOwner, &sNdsNativeNFoxFighterLowTables,
    sNdsNativeNFoxRootsLow, sNdsNativeNFoxCrossPaletteSlotsLow,
    sNdsNativeNFoxRootLightPreambles, NDS_NATIVE_NFOX_MODEL_DATA_SIZE);
#endif

/* P4 donor owners (nds_p4_contents.h): image-backed. Each content's two
 * tables structs are bound to its loaded image (ndsRendererNativeBindOwnerImage)
 * and so are its root tables -- roots, light preambles, binding parents and
 * joints, the joint schedule, root variants and each root program's -- which
 * lead both of its images (owner_words: scripts/p4/p4_native_owner.py
 * split_owner_tables) and which nds_p4_native_owner.generated.inc names through
 * sNdsP4<Title>Owner. Its runtime owners and program rows are therefore
 * writable and read the shared zero tables until an image binds them
 * (ndsP4Bind<Title>OwnerTables); their counts and cross-palette slots are
 * resident constants. Twenty contents' root tables were ~31 KB of every
 * scene's heap; a match now loads its fighters'. */
#define NDS_P4_OWNER_UNBOUND(T_)                                               \
    ((const NDSP4##T_##OwnerTables *)(const void *)sNdsP4OwnerZero)
#define NDS_P4_FTR_OWNER_RUNTIME(name_, tables_, T_, roots_, cross_, bytes_)   \
    static NDSNativeFighterOwnerRuntime name_ = {                              \
        (tables_), NDS_P4_OWNER_UNBOUND(T_)->roots_,                           \
        NDS_FTR_COUNT(NDS_P4_OWNER_UNBOUND(T_)->roots_), (cross_),             \
        NDS_P4_OWNER_UNBOUND(T_)->RootLightPreambles,                          \
        NDS_FTR_COUNT(NDS_P4_OWNER_UNBOUND(T_)->RootLightPreambles), (bytes_)  \
    }
#define NDS_P4_OWNER_RUNTIME(id_, T_, N_, n_, parent_, model_, main_)          \
    static NDSNativeFighterRuntimeTables sNdsNative##T_##FighterHighTables;    \
    static NDSNativeFighterRuntimeTables sNdsNative##T_##FighterLowTables;     \
    NDS_P4_FTR_OWNER_RUNTIME(                                                  \
        sNdsNative##T_##HighOwner, &sNdsNative##T_##FighterHighTables, T_,     \
        Roots, sNdsNative##T_##CrossPaletteSlots,                              \
        NDS_NATIVE_##N_##_MODEL_DATA_SIZE);                                    \
    NDS_P4_FTR_OWNER_RUNTIME(                                                  \
        sNdsNative##T_##LowOwner, &sNdsNative##T_##FighterLowTables, T_,       \
        RootsLow, sNdsNative##T_##CrossPaletteSlotsLow,                        \
        NDS_NATIVE_##N_##_MODEL_DATA_SIZE);
NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_RUNTIME)
#undef NDS_P4_OWNER_RUNTIME

/* A content's root programs (scripts/p4/p4_native_owner.py donor_superset).
 * Its canonical vector carries every joint its motions make draw -- Banjo's
 * Kazooie (hidden parts his motions' anim-desc masks install), the articles
 * Crash, Peach, Lanky and Dedede hold (model parts on list-less joints) -- and
 * each live vector with some of them absent is a program. A program root's
 * `bindings` entry is the canonical binding its model-part variants name. */
typedef struct NDSP4RootProgram
{
    const NDSNativeFighterOwnerRuntime *owners[2];
    const u8 *parents;
    const u8 *cross;
    const u8 *bindings[2];
} NDSP4RootProgram;

#define NDS_P4_PROGRAM_OWNERS(T_, N_, P_)                                      \
    NDS_P4_FTR_OWNER_RUNTIME(                                                  \
        sNdsNative##T_##P_##HighOwner, &sNdsNative##T_##FighterHighTables, T_, \
        P_##Roots, sNdsNative##T_##P_##CrossPaletteSlots,                      \
        NDS_NATIVE_##N_##_MODEL_DATA_SIZE);                                    \
    NDS_P4_FTR_OWNER_RUNTIME(                                                  \
        sNdsNative##T_##P_##LowOwner, &sNdsNative##T_##FighterLowTables, T_,   \
        P_##RootsLow, sNdsNative##T_##P_##CrossPaletteSlotsLow,                \
        NDS_NATIVE_##N_##_MODEL_DATA_SIZE);
#define NDS_P4_PROGRAM_ROW(T_, N_, P_)                                         \
    { { &sNdsNative##T_##P_##HighOwner, &sNdsNative##T_##P_##LowOwner },       \
      NDS_P4_OWNER_UNBOUND(T_)->P_##BindingParents,                            \
      sNdsNative##T_##P_##CrossPaletteSlots,                                   \
      { NDS_P4_OWNER_UNBOUND(T_)->P_##RootBindings,                            \
        NDS_P4_OWNER_UNBOUND(T_)->P_##RootBindingsLow } },
#define NDS_P4_OWNER_PROGRAMS(id_, T_, N_, n_, parent_, model_, main_)        \
    NDS_P4_NATIVE_##N_##_PROGRAMS(NDS_P4_PROGRAM_OWNERS)                       \
    static NDSP4RootProgram                                                    \
        sNdsP4##T_##RootPrograms[NDS_P4_NATIVE_##N_##_PROGRAM_COUNT + 1u] =   \
    {                                                                          \
        NDS_P4_NATIVE_##N_##_PROGRAMS(NDS_P4_PROGRAM_ROW)                      \
        { { NULL, NULL }, NULL, NULL, { NULL, NULL } }                         \
    };
NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_PROGRAMS)
#undef NDS_P4_OWNER_PROGRAMS
#undef NDS_P4_PROGRAM_ROW
#undef NDS_P4_PROGRAM_OWNERS

/* Points a content's root tables at an image's owner_words (NULL: the zero
 * tables), then every runtime owner and program row that names them. */
static void ndsP4OwnerBindRuntime(NDSNativeFighterOwnerRuntime *owner,
                                  const NDSNativeRoot *roots,
                                  const u32 (*preambles)[2])
{
    owner->roots = roots;
    owner->root_light_preambles = preambles;
}

#define NDS_P4_PROGRAM_BIND(T_, N_, P_)                                        \
    ndsP4OwnerBindRuntime(&sNdsNative##T_##P_##HighOwner,                      \
                          sNdsNative##T_##P_##Roots,                           \
                          sNdsNative##T_##RootLightPreambles);                 \
    ndsP4OwnerBindRuntime(&sNdsNative##T_##P_##LowOwner,                       \
                          sNdsNative##T_##P_##RootsLow,                        \
                          sNdsNative##T_##RootLightPreambles);                 \
    program->parents = sNdsNative##T_##P_##BindingParents;                     \
    program->bindings[0] = sNdsNative##T_##P_##RootBindings;                   \
    program->bindings[1] = sNdsNative##T_##P_##RootBindingsLow;                \
    program++;
#define NDS_P4_OWNER_TABLES_BIND(id_, T_, N_, n_, parent_, model_, main_)      \
    static void ndsP4Bind##T_##OwnerTables(const void *tables)                 \
    {                                                                          \
        NDSP4RootProgram *program = sNdsP4##T_##RootPrograms;                  \
                                                                               \
        sNdsP4##T_##Owner = (tables != NULL) ?                                 \
            (const NDSP4##T_##OwnerTables *)tables : NDS_P4_OWNER_UNBOUND(T_); \
        ndsP4OwnerBindRuntime(&sNdsNative##T_##HighOwner,                      \
                              sNdsNative##T_##Roots,                           \
                              sNdsNative##T_##RootLightPreambles);             \
        ndsP4OwnerBindRuntime(&sNdsNative##T_##LowOwner,                       \
                              sNdsNative##T_##RootsLow,                        \
                              sNdsNative##T_##RootLightPreambles);             \
        NDS_P4_NATIVE_##N_##_PROGRAMS(NDS_P4_PROGRAM_BIND)                     \
        (void)program;                                                         \
    }
NDS_P4_CONTENT_ROWS(NDS_P4_OWNER_TABLES_BIND)
#undef NDS_P4_OWNER_TABLES_BIND
#undef NDS_P4_PROGRAM_BIND

/* Programs 1..*count of a P4 owner slot; NULL for any other slot. */
static const NDSP4RootProgram *ndsP4RootPrograms(u32 slot, u32 *count)
{
#define NDS_P4_PROGRAM_TABLE(id_, T_, N_, n_, parent_, model_, main_)         \
    if (slot == NDS_P4_RENDERER_OWNER_SLOT(id_))                                \
    {                                                                           \
        *count = NDS_P4_NATIVE_##N_##_PROGRAM_COUNT;                            \
        return sNdsP4##T_##RootPrograms;                                        \
    }
    NDS_P4_CONTENT_ROWS(NDS_P4_PROGRAM_TABLE)
#undef NDS_P4_PROGRAM_TABLE
    *count = 0u;
    return NULL;
}

#if NDS_P2_NDONKEY
#if NDS_NATIVE_OWNER_IMAGE_NDONKEY
static NDSNativeFighterRuntimeTables sNdsNativeNDonkeyFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNDonkeyFighterHighTables =
{
    sNdsNativeNDonkeyFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterStateDeltas),
    sNdsNativeNDonkeyFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterStateSequence),
    sNdsNativeNDonkeyFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterVertexActions),
    sNdsNativeNDonkeyFighterEpochDirectPolicy,
    sNdsNativeNDonkeyFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterDenseVertices),
    sNdsNativeNDonkeyFighterDenseNormals,
    sNdsNativeNDonkeyFighterPreparedDense,
    sNdsNativeNDonkeyFighterActionDenseSpans,
    sNdsNativeNDonkeyFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterPackedCorners),
    sNdsNativeNDonkeyFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterRunFirstCorner),
    sNdsNativeNDonkeyFighterRunFirstUnique,
    sNdsNativeNDonkeyFighterRunUniqueCount,
    sNdsNativeNDonkeyFighterRunUniqueDense,
    sNdsNativeNDonkeyFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterTriangles),
    sNdsNativeNDonkeyFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterRuns),
    sNdsNativeNDonkeyFighterPrimitiveGroupFirst,
    sNdsNativeNDonkeyFighterPrimitiveGroupCount,
    sNdsNativeNDonkeyFighterPrimitiveGroupType,
    sNdsNativeNDonkeyFighterPrimitiveGroupFirstVertex,
    sNdsNativeNDonkeyFighterPrimitiveGroupVertexCount,
    sNdsNativeNDonkeyFighterPrimitiveVertices,
    sNdsNativeNDonkeyFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NDONKEY
static NDSNativeFighterRuntimeTables sNdsNativeNDonkeyFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNDonkeyFighterLowTables =
{
    sNdsNativeNDonkeyFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterStateDeltasLow),
    sNdsNativeNDonkeyFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterStateSequenceLow),
    sNdsNativeNDonkeyFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterVertexActionsLow),
    sNdsNativeNDonkeyFighterEpochDirectPolicyLow,
    sNdsNativeNDonkeyFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterDenseVerticesLow),
    sNdsNativeNDonkeyFighterDenseNormalsLow,
    sNdsNativeNDonkeyFighterPreparedDenseLow,
    sNdsNativeNDonkeyFighterActionDenseSpansLow,
    sNdsNativeNDonkeyFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterPackedCornersLow),
    sNdsNativeNDonkeyFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterRunFirstCornerLow),
    sNdsNativeNDonkeyFighterRunFirstUniqueLow,
    sNdsNativeNDonkeyFighterRunUniqueCountLow,
    sNdsNativeNDonkeyFighterRunUniqueDenseLow,
    sNdsNativeNDonkeyFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterTrianglesLow),
    sNdsNativeNDonkeyFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterRunsLow),
    sNdsNativeNDonkeyFighterPrimitiveGroupFirstLow,
    sNdsNativeNDonkeyFighterPrimitiveGroupCountLow,
    sNdsNativeNDonkeyFighterPrimitiveGroupTypeLow,
    sNdsNativeNDonkeyFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNDonkeyFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNDonkeyFighterPrimitiveVerticesLow,
    sNdsNativeNDonkeyFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNDonkeyFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNDonkeyHighOwner, &sNdsNativeNDonkeyFighterHighTables,
    sNdsNativeNDonkeyRoots, sNdsNativeNDonkeyCrossPaletteSlots,
    sNdsNativeNDonkeyRootLightPreambles, NDS_NATIVE_NDONKEY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNDonkeyLowOwner, &sNdsNativeNDonkeyFighterLowTables,
    sNdsNativeNDonkeyRootsLow, sNdsNativeNDonkeyCrossPaletteSlotsLow,
    sNdsNativeNDonkeyRootLightPreambles, NDS_NATIVE_NDONKEY_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NSAMUS
#if NDS_NATIVE_OWNER_IMAGE_NSAMUS
static NDSNativeFighterRuntimeTables sNdsNativeNSamusFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNSamusFighterHighTables =
{
    sNdsNativeNSamusFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterStateDeltas),
    sNdsNativeNSamusFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterStateSequence),
    sNdsNativeNSamusFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterVertexActions),
    sNdsNativeNSamusFighterEpochDirectPolicy,
    sNdsNativeNSamusFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterDenseVertices),
    sNdsNativeNSamusFighterDenseNormals,
    sNdsNativeNSamusFighterPreparedDense,
    sNdsNativeNSamusFighterActionDenseSpans,
    sNdsNativeNSamusFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterPackedCorners),
    sNdsNativeNSamusFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterRunFirstCorner),
    sNdsNativeNSamusFighterRunFirstUnique,
    sNdsNativeNSamusFighterRunUniqueCount,
    sNdsNativeNSamusFighterRunUniqueDense,
    sNdsNativeNSamusFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterTriangles),
    sNdsNativeNSamusFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterRuns),
    sNdsNativeNSamusFighterPrimitiveGroupFirst,
    sNdsNativeNSamusFighterPrimitiveGroupCount,
    sNdsNativeNSamusFighterPrimitiveGroupType,
    sNdsNativeNSamusFighterPrimitiveGroupFirstVertex,
    sNdsNativeNSamusFighterPrimitiveGroupVertexCount,
    sNdsNativeNSamusFighterPrimitiveVertices,
    sNdsNativeNSamusFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NSAMUS
static NDSNativeFighterRuntimeTables sNdsNativeNSamusFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNSamusFighterLowTables =
{
    sNdsNativeNSamusFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterStateDeltasLow),
    sNdsNativeNSamusFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterStateSequenceLow),
    sNdsNativeNSamusFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterVertexActionsLow),
    sNdsNativeNSamusFighterEpochDirectPolicyLow,
    sNdsNativeNSamusFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterDenseVerticesLow),
    sNdsNativeNSamusFighterDenseNormalsLow,
    sNdsNativeNSamusFighterPreparedDenseLow,
    sNdsNativeNSamusFighterActionDenseSpansLow,
    sNdsNativeNSamusFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterPackedCornersLow),
    sNdsNativeNSamusFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterRunFirstCornerLow),
    sNdsNativeNSamusFighterRunFirstUniqueLow,
    sNdsNativeNSamusFighterRunUniqueCountLow,
    sNdsNativeNSamusFighterRunUniqueDenseLow,
    sNdsNativeNSamusFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterTrianglesLow),
    sNdsNativeNSamusFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterRunsLow),
    sNdsNativeNSamusFighterPrimitiveGroupFirstLow,
    sNdsNativeNSamusFighterPrimitiveGroupCountLow,
    sNdsNativeNSamusFighterPrimitiveGroupTypeLow,
    sNdsNativeNSamusFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNSamusFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNSamusFighterPrimitiveVerticesLow,
    sNdsNativeNSamusFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNSamusFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNSamusHighOwner, &sNdsNativeNSamusFighterHighTables,
    sNdsNativeNSamusRoots, sNdsNativeNSamusCrossPaletteSlots,
    sNdsNativeNSamusRootLightPreambles, NDS_NATIVE_NSAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNSamusLowOwner, &sNdsNativeNSamusFighterLowTables,
    sNdsNativeNSamusRootsLow, sNdsNativeNSamusCrossPaletteSlotsLow,
    sNdsNativeNSamusRootLightPreambles, NDS_NATIVE_NSAMUS_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NLINK
#if NDS_NATIVE_OWNER_IMAGE_NLINK
static NDSNativeFighterRuntimeTables sNdsNativeNLinkFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNLinkFighterHighTables =
{
    sNdsNativeNLinkFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterStateDeltas),
    sNdsNativeNLinkFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterStateSequence),
    sNdsNativeNLinkFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterVertexActions),
    sNdsNativeNLinkFighterEpochDirectPolicy,
    sNdsNativeNLinkFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterDenseVertices),
    sNdsNativeNLinkFighterDenseNormals,
    sNdsNativeNLinkFighterPreparedDense,
    sNdsNativeNLinkFighterActionDenseSpans,
    sNdsNativeNLinkFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterPackedCorners),
    sNdsNativeNLinkFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterRunFirstCorner),
    sNdsNativeNLinkFighterRunFirstUnique,
    sNdsNativeNLinkFighterRunUniqueCount,
    sNdsNativeNLinkFighterRunUniqueDense,
    sNdsNativeNLinkFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterTriangles),
    sNdsNativeNLinkFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterRuns),
    sNdsNativeNLinkFighterPrimitiveGroupFirst,
    sNdsNativeNLinkFighterPrimitiveGroupCount,
    sNdsNativeNLinkFighterPrimitiveGroupType,
    sNdsNativeNLinkFighterPrimitiveGroupFirstVertex,
    sNdsNativeNLinkFighterPrimitiveGroupVertexCount,
    sNdsNativeNLinkFighterPrimitiveVertices,
    sNdsNativeNLinkFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NLINK
static NDSNativeFighterRuntimeTables sNdsNativeNLinkFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNLinkFighterLowTables =
{
    sNdsNativeNLinkFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterStateDeltasLow),
    sNdsNativeNLinkFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterStateSequenceLow),
    sNdsNativeNLinkFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterVertexActionsLow),
    sNdsNativeNLinkFighterEpochDirectPolicyLow,
    sNdsNativeNLinkFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterDenseVerticesLow),
    sNdsNativeNLinkFighterDenseNormalsLow,
    sNdsNativeNLinkFighterPreparedDenseLow,
    sNdsNativeNLinkFighterActionDenseSpansLow,
    sNdsNativeNLinkFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterPackedCornersLow),
    sNdsNativeNLinkFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterRunFirstCornerLow),
    sNdsNativeNLinkFighterRunFirstUniqueLow,
    sNdsNativeNLinkFighterRunUniqueCountLow,
    sNdsNativeNLinkFighterRunUniqueDenseLow,
    sNdsNativeNLinkFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterTrianglesLow),
    sNdsNativeNLinkFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterRunsLow),
    sNdsNativeNLinkFighterPrimitiveGroupFirstLow,
    sNdsNativeNLinkFighterPrimitiveGroupCountLow,
    sNdsNativeNLinkFighterPrimitiveGroupTypeLow,
    sNdsNativeNLinkFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNLinkFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNLinkFighterPrimitiveVerticesLow,
    sNdsNativeNLinkFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNLinkFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNLinkHighOwner, &sNdsNativeNLinkFighterHighTables,
    sNdsNativeNLinkRoots, sNdsNativeNLinkCrossPaletteSlots,
    sNdsNativeNLinkRootLightPreambles, NDS_NATIVE_NLINK_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNLinkLowOwner, &sNdsNativeNLinkFighterLowTables,
    sNdsNativeNLinkRootsLow, sNdsNativeNLinkCrossPaletteSlotsLow,
    sNdsNativeNLinkRootLightPreambles, NDS_NATIVE_NLINK_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NYOSHI
#if NDS_NATIVE_OWNER_IMAGE_NYOSHI
static NDSNativeFighterRuntimeTables sNdsNativeNYoshiFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNYoshiFighterHighTables =
{
    sNdsNativeNYoshiFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterStateDeltas),
    sNdsNativeNYoshiFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterStateSequence),
    sNdsNativeNYoshiFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterVertexActions),
    sNdsNativeNYoshiFighterEpochDirectPolicy,
    sNdsNativeNYoshiFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterDenseVertices),
    sNdsNativeNYoshiFighterDenseNormals,
    sNdsNativeNYoshiFighterPreparedDense,
    sNdsNativeNYoshiFighterActionDenseSpans,
    sNdsNativeNYoshiFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterPackedCorners),
    sNdsNativeNYoshiFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterRunFirstCorner),
    sNdsNativeNYoshiFighterRunFirstUnique,
    sNdsNativeNYoshiFighterRunUniqueCount,
    sNdsNativeNYoshiFighterRunUniqueDense,
    sNdsNativeNYoshiFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterTriangles),
    sNdsNativeNYoshiFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterRuns),
    sNdsNativeNYoshiFighterPrimitiveGroupFirst,
    sNdsNativeNYoshiFighterPrimitiveGroupCount,
    sNdsNativeNYoshiFighterPrimitiveGroupType,
    sNdsNativeNYoshiFighterPrimitiveGroupFirstVertex,
    sNdsNativeNYoshiFighterPrimitiveGroupVertexCount,
    sNdsNativeNYoshiFighterPrimitiveVertices,
    sNdsNativeNYoshiFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NYOSHI
static NDSNativeFighterRuntimeTables sNdsNativeNYoshiFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNYoshiFighterLowTables =
{
    sNdsNativeNYoshiFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterStateDeltasLow),
    sNdsNativeNYoshiFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterStateSequenceLow),
    sNdsNativeNYoshiFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterVertexActionsLow),
    sNdsNativeNYoshiFighterEpochDirectPolicyLow,
    sNdsNativeNYoshiFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterDenseVerticesLow),
    sNdsNativeNYoshiFighterDenseNormalsLow,
    sNdsNativeNYoshiFighterPreparedDenseLow,
    sNdsNativeNYoshiFighterActionDenseSpansLow,
    sNdsNativeNYoshiFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterPackedCornersLow),
    sNdsNativeNYoshiFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterRunFirstCornerLow),
    sNdsNativeNYoshiFighterRunFirstUniqueLow,
    sNdsNativeNYoshiFighterRunUniqueCountLow,
    sNdsNativeNYoshiFighterRunUniqueDenseLow,
    sNdsNativeNYoshiFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterTrianglesLow),
    sNdsNativeNYoshiFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterRunsLow),
    sNdsNativeNYoshiFighterPrimitiveGroupFirstLow,
    sNdsNativeNYoshiFighterPrimitiveGroupCountLow,
    sNdsNativeNYoshiFighterPrimitiveGroupTypeLow,
    sNdsNativeNYoshiFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNYoshiFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNYoshiFighterPrimitiveVerticesLow,
    sNdsNativeNYoshiFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNYoshiFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNYoshiHighOwner, &sNdsNativeNYoshiFighterHighTables,
    sNdsNativeNYoshiRoots, sNdsNativeNYoshiCrossPaletteSlots,
    sNdsNativeNYoshiRootLightPreambles, NDS_NATIVE_NYOSHI_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNYoshiLowOwner, &sNdsNativeNYoshiFighterLowTables,
    sNdsNativeNYoshiRootsLow, sNdsNativeNYoshiCrossPaletteSlotsLow,
    sNdsNativeNYoshiRootLightPreambles, NDS_NATIVE_NYOSHI_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NCAPTAIN
#if NDS_NATIVE_OWNER_IMAGE_NCAPTAIN
static NDSNativeFighterRuntimeTables sNdsNativeNCaptainFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNCaptainFighterHighTables =
{
    sNdsNativeNCaptainFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterStateDeltas),
    sNdsNativeNCaptainFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterStateSequence),
    sNdsNativeNCaptainFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterVertexActions),
    sNdsNativeNCaptainFighterEpochDirectPolicy,
    sNdsNativeNCaptainFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterDenseVertices),
    sNdsNativeNCaptainFighterDenseNormals,
    sNdsNativeNCaptainFighterPreparedDense,
    sNdsNativeNCaptainFighterActionDenseSpans,
    sNdsNativeNCaptainFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterPackedCorners),
    sNdsNativeNCaptainFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterRunFirstCorner),
    sNdsNativeNCaptainFighterRunFirstUnique,
    sNdsNativeNCaptainFighterRunUniqueCount,
    sNdsNativeNCaptainFighterRunUniqueDense,
    sNdsNativeNCaptainFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterTriangles),
    sNdsNativeNCaptainFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterRuns),
    sNdsNativeNCaptainFighterPrimitiveGroupFirst,
    sNdsNativeNCaptainFighterPrimitiveGroupCount,
    sNdsNativeNCaptainFighterPrimitiveGroupType,
    sNdsNativeNCaptainFighterPrimitiveGroupFirstVertex,
    sNdsNativeNCaptainFighterPrimitiveGroupVertexCount,
    sNdsNativeNCaptainFighterPrimitiveVertices,
    sNdsNativeNCaptainFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NCAPTAIN
static NDSNativeFighterRuntimeTables sNdsNativeNCaptainFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNCaptainFighterLowTables =
{
    sNdsNativeNCaptainFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterStateDeltasLow),
    sNdsNativeNCaptainFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterStateSequenceLow),
    sNdsNativeNCaptainFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterVertexActionsLow),
    sNdsNativeNCaptainFighterEpochDirectPolicyLow,
    sNdsNativeNCaptainFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterDenseVerticesLow),
    sNdsNativeNCaptainFighterDenseNormalsLow,
    sNdsNativeNCaptainFighterPreparedDenseLow,
    sNdsNativeNCaptainFighterActionDenseSpansLow,
    sNdsNativeNCaptainFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterPackedCornersLow),
    sNdsNativeNCaptainFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterRunFirstCornerLow),
    sNdsNativeNCaptainFighterRunFirstUniqueLow,
    sNdsNativeNCaptainFighterRunUniqueCountLow,
    sNdsNativeNCaptainFighterRunUniqueDenseLow,
    sNdsNativeNCaptainFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterTrianglesLow),
    sNdsNativeNCaptainFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterRunsLow),
    sNdsNativeNCaptainFighterPrimitiveGroupFirstLow,
    sNdsNativeNCaptainFighterPrimitiveGroupCountLow,
    sNdsNativeNCaptainFighterPrimitiveGroupTypeLow,
    sNdsNativeNCaptainFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNCaptainFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNCaptainFighterPrimitiveVerticesLow,
    sNdsNativeNCaptainFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNCaptainFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNCaptainHighOwner, &sNdsNativeNCaptainFighterHighTables,
    sNdsNativeNCaptainRoots, sNdsNativeNCaptainCrossPaletteSlots,
    sNdsNativeNCaptainRootLightPreambles, NDS_NATIVE_NCAPTAIN_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNCaptainLowOwner, &sNdsNativeNCaptainFighterLowTables,
    sNdsNativeNCaptainRootsLow, sNdsNativeNCaptainCrossPaletteSlotsLow,
    sNdsNativeNCaptainRootLightPreambles, NDS_NATIVE_NCAPTAIN_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NKIRBY
#if NDS_NATIVE_OWNER_IMAGE_NKIRBY
static NDSNativeFighterRuntimeTables sNdsNativeNKirbyFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNKirbyFighterHighTables =
{
    sNdsNativeNKirbyFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterStateDeltas),
    sNdsNativeNKirbyFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterStateSequence),
    sNdsNativeNKirbyFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterVertexActions),
    sNdsNativeNKirbyFighterEpochDirectPolicy,
    sNdsNativeNKirbyFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterDenseVertices),
    sNdsNativeNKirbyFighterDenseNormals,
    sNdsNativeNKirbyFighterPreparedDense,
    sNdsNativeNKirbyFighterActionDenseSpans,
    sNdsNativeNKirbyFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterPackedCorners),
    sNdsNativeNKirbyFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterRunFirstCorner),
    sNdsNativeNKirbyFighterRunFirstUnique,
    sNdsNativeNKirbyFighterRunUniqueCount,
    sNdsNativeNKirbyFighterRunUniqueDense,
    sNdsNativeNKirbyFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterTriangles),
    sNdsNativeNKirbyFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterRuns),
    sNdsNativeNKirbyFighterPrimitiveGroupFirst,
    sNdsNativeNKirbyFighterPrimitiveGroupCount,
    sNdsNativeNKirbyFighterPrimitiveGroupType,
    sNdsNativeNKirbyFighterPrimitiveGroupFirstVertex,
    sNdsNativeNKirbyFighterPrimitiveGroupVertexCount,
    sNdsNativeNKirbyFighterPrimitiveVertices,
    sNdsNativeNKirbyFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NKIRBY
static NDSNativeFighterRuntimeTables sNdsNativeNKirbyFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNKirbyFighterLowTables =
{
    sNdsNativeNKirbyFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterStateDeltasLow),
    sNdsNativeNKirbyFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterStateSequenceLow),
    sNdsNativeNKirbyFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterVertexActionsLow),
    sNdsNativeNKirbyFighterEpochDirectPolicyLow,
    sNdsNativeNKirbyFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterDenseVerticesLow),
    sNdsNativeNKirbyFighterDenseNormalsLow,
    sNdsNativeNKirbyFighterPreparedDenseLow,
    sNdsNativeNKirbyFighterActionDenseSpansLow,
    sNdsNativeNKirbyFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterPackedCornersLow),
    sNdsNativeNKirbyFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterRunFirstCornerLow),
    sNdsNativeNKirbyFighterRunFirstUniqueLow,
    sNdsNativeNKirbyFighterRunUniqueCountLow,
    sNdsNativeNKirbyFighterRunUniqueDenseLow,
    sNdsNativeNKirbyFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterTrianglesLow),
    sNdsNativeNKirbyFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterRunsLow),
    sNdsNativeNKirbyFighterPrimitiveGroupFirstLow,
    sNdsNativeNKirbyFighterPrimitiveGroupCountLow,
    sNdsNativeNKirbyFighterPrimitiveGroupTypeLow,
    sNdsNativeNKirbyFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNKirbyFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNKirbyFighterPrimitiveVerticesLow,
    sNdsNativeNKirbyFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNKirbyFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNKirbyHighOwner, &sNdsNativeNKirbyFighterHighTables,
    sNdsNativeNKirbyRoots, sNdsNativeNKirbyCrossPaletteSlots,
    sNdsNativeNKirbyRootLightPreambles, NDS_NATIVE_NKIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNKirbyLowOwner, &sNdsNativeNKirbyFighterLowTables,
    sNdsNativeNKirbyRootsLow, sNdsNativeNKirbyCrossPaletteSlotsLow,
    sNdsNativeNKirbyRootLightPreambles, NDS_NATIVE_NKIRBY_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NPIKACHU
#if NDS_NATIVE_OWNER_IMAGE_NPIKACHU
static NDSNativeFighterRuntimeTables sNdsNativeNPikachuFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNPikachuFighterHighTables =
{
    sNdsNativeNPikachuFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterStateDeltas),
    sNdsNativeNPikachuFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterStateSequence),
    sNdsNativeNPikachuFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterVertexActions),
    sNdsNativeNPikachuFighterEpochDirectPolicy,
    sNdsNativeNPikachuFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterDenseVertices),
    sNdsNativeNPikachuFighterDenseNormals,
    sNdsNativeNPikachuFighterPreparedDense,
    sNdsNativeNPikachuFighterActionDenseSpans,
    sNdsNativeNPikachuFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterPackedCorners),
    sNdsNativeNPikachuFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterRunFirstCorner),
    sNdsNativeNPikachuFighterRunFirstUnique,
    sNdsNativeNPikachuFighterRunUniqueCount,
    sNdsNativeNPikachuFighterRunUniqueDense,
    sNdsNativeNPikachuFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterTriangles),
    sNdsNativeNPikachuFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterRuns),
    sNdsNativeNPikachuFighterPrimitiveGroupFirst,
    sNdsNativeNPikachuFighterPrimitiveGroupCount,
    sNdsNativeNPikachuFighterPrimitiveGroupType,
    sNdsNativeNPikachuFighterPrimitiveGroupFirstVertex,
    sNdsNativeNPikachuFighterPrimitiveGroupVertexCount,
    sNdsNativeNPikachuFighterPrimitiveVertices,
    sNdsNativeNPikachuFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NPIKACHU
static NDSNativeFighterRuntimeTables sNdsNativeNPikachuFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNPikachuFighterLowTables =
{
    sNdsNativeNPikachuFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterStateDeltasLow),
    sNdsNativeNPikachuFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterStateSequenceLow),
    sNdsNativeNPikachuFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterVertexActionsLow),
    sNdsNativeNPikachuFighterEpochDirectPolicyLow,
    sNdsNativeNPikachuFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterDenseVerticesLow),
    sNdsNativeNPikachuFighterDenseNormalsLow,
    sNdsNativeNPikachuFighterPreparedDenseLow,
    sNdsNativeNPikachuFighterActionDenseSpansLow,
    sNdsNativeNPikachuFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterPackedCornersLow),
    sNdsNativeNPikachuFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterRunFirstCornerLow),
    sNdsNativeNPikachuFighterRunFirstUniqueLow,
    sNdsNativeNPikachuFighterRunUniqueCountLow,
    sNdsNativeNPikachuFighterRunUniqueDenseLow,
    sNdsNativeNPikachuFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterTrianglesLow),
    sNdsNativeNPikachuFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterRunsLow),
    sNdsNativeNPikachuFighterPrimitiveGroupFirstLow,
    sNdsNativeNPikachuFighterPrimitiveGroupCountLow,
    sNdsNativeNPikachuFighterPrimitiveGroupTypeLow,
    sNdsNativeNPikachuFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNPikachuFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNPikachuFighterPrimitiveVerticesLow,
    sNdsNativeNPikachuFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNPikachuFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNPikachuHighOwner, &sNdsNativeNPikachuFighterHighTables,
    sNdsNativeNPikachuRoots, sNdsNativeNPikachuCrossPaletteSlots,
    sNdsNativeNPikachuRootLightPreambles, NDS_NATIVE_NPIKACHU_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNPikachuLowOwner, &sNdsNativeNPikachuFighterLowTables,
    sNdsNativeNPikachuRootsLow, sNdsNativeNPikachuCrossPaletteSlotsLow,
    sNdsNativeNPikachuRootLightPreambles, NDS_NATIVE_NPIKACHU_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NPURIN
#if NDS_NATIVE_OWNER_IMAGE_NPURIN
static NDSNativeFighterRuntimeTables sNdsNativeNPurinFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNPurinFighterHighTables =
{
    sNdsNativeNPurinFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterStateDeltas),
    sNdsNativeNPurinFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterStateSequence),
    sNdsNativeNPurinFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterVertexActions),
    sNdsNativeNPurinFighterEpochDirectPolicy,
    sNdsNativeNPurinFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterDenseVertices),
    sNdsNativeNPurinFighterDenseNormals,
    sNdsNativeNPurinFighterPreparedDense,
    sNdsNativeNPurinFighterActionDenseSpans,
    sNdsNativeNPurinFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterPackedCorners),
    sNdsNativeNPurinFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterRunFirstCorner),
    sNdsNativeNPurinFighterRunFirstUnique,
    sNdsNativeNPurinFighterRunUniqueCount,
    sNdsNativeNPurinFighterRunUniqueDense,
    sNdsNativeNPurinFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterTriangles),
    sNdsNativeNPurinFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterRuns),
    sNdsNativeNPurinFighterPrimitiveGroupFirst,
    sNdsNativeNPurinFighterPrimitiveGroupCount,
    sNdsNativeNPurinFighterPrimitiveGroupType,
    sNdsNativeNPurinFighterPrimitiveGroupFirstVertex,
    sNdsNativeNPurinFighterPrimitiveGroupVertexCount,
    sNdsNativeNPurinFighterPrimitiveVertices,
    sNdsNativeNPurinFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NPURIN
static NDSNativeFighterRuntimeTables sNdsNativeNPurinFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNPurinFighterLowTables =
{
    sNdsNativeNPurinFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterStateDeltasLow),
    sNdsNativeNPurinFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterStateSequenceLow),
    sNdsNativeNPurinFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterVertexActionsLow),
    sNdsNativeNPurinFighterEpochDirectPolicyLow,
    sNdsNativeNPurinFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterDenseVerticesLow),
    sNdsNativeNPurinFighterDenseNormalsLow,
    sNdsNativeNPurinFighterPreparedDenseLow,
    sNdsNativeNPurinFighterActionDenseSpansLow,
    sNdsNativeNPurinFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterPackedCornersLow),
    sNdsNativeNPurinFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterRunFirstCornerLow),
    sNdsNativeNPurinFighterRunFirstUniqueLow,
    sNdsNativeNPurinFighterRunUniqueCountLow,
    sNdsNativeNPurinFighterRunUniqueDenseLow,
    sNdsNativeNPurinFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterTrianglesLow),
    sNdsNativeNPurinFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterRunsLow),
    sNdsNativeNPurinFighterPrimitiveGroupFirstLow,
    sNdsNativeNPurinFighterPrimitiveGroupCountLow,
    sNdsNativeNPurinFighterPrimitiveGroupTypeLow,
    sNdsNativeNPurinFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNPurinFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNPurinFighterPrimitiveVerticesLow,
    sNdsNativeNPurinFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNPurinFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNPurinHighOwner, &sNdsNativeNPurinFighterHighTables,
    sNdsNativeNPurinRoots, sNdsNativeNPurinCrossPaletteSlots,
    sNdsNativeNPurinRootLightPreambles, NDS_NATIVE_NPURIN_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNPurinLowOwner, &sNdsNativeNPurinFighterLowTables,
    sNdsNativeNPurinRootsLow, sNdsNativeNPurinCrossPaletteSlotsLow,
    sNdsNativeNPurinRootLightPreambles, NDS_NATIVE_NPURIN_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NNESS
#if NDS_NATIVE_OWNER_IMAGE_NNESS
static NDSNativeFighterRuntimeTables sNdsNativeNNessFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNNessFighterHighTables =
{
    sNdsNativeNNessFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNNessFighterStateDeltas),
    sNdsNativeNNessFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNNessFighterStateSequence),
    sNdsNativeNNessFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNNessFighterVertexActions),
    sNdsNativeNNessFighterEpochDirectPolicy,
    sNdsNativeNNessFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNNessFighterDenseVertices),
    sNdsNativeNNessFighterDenseNormals,
    sNdsNativeNNessFighterPreparedDense,
    sNdsNativeNNessFighterActionDenseSpans,
    sNdsNativeNNessFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNNessFighterPackedCorners),
    sNdsNativeNNessFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNNessFighterRunFirstCorner),
    sNdsNativeNNessFighterRunFirstUnique,
    sNdsNativeNNessFighterRunUniqueCount,
    sNdsNativeNNessFighterRunUniqueDense,
    sNdsNativeNNessFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNNessFighterTriangles),
    sNdsNativeNNessFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNNessFighterRuns),
    sNdsNativeNNessFighterPrimitiveGroupFirst,
    sNdsNativeNNessFighterPrimitiveGroupCount,
    sNdsNativeNNessFighterPrimitiveGroupType,
    sNdsNativeNNessFighterPrimitiveGroupFirstVertex,
    sNdsNativeNNessFighterPrimitiveGroupVertexCount,
    sNdsNativeNNessFighterPrimitiveVertices,
    sNdsNativeNNessFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNNessFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NNESS
static NDSNativeFighterRuntimeTables sNdsNativeNNessFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNNessFighterLowTables =
{
    sNdsNativeNNessFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterStateDeltasLow),
    sNdsNativeNNessFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterStateSequenceLow),
    sNdsNativeNNessFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterVertexActionsLow),
    sNdsNativeNNessFighterEpochDirectPolicyLow,
    sNdsNativeNNessFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterDenseVerticesLow),
    sNdsNativeNNessFighterDenseNormalsLow,
    sNdsNativeNNessFighterPreparedDenseLow,
    sNdsNativeNNessFighterActionDenseSpansLow,
    sNdsNativeNNessFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterPackedCornersLow),
    sNdsNativeNNessFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterRunFirstCornerLow),
    sNdsNativeNNessFighterRunFirstUniqueLow,
    sNdsNativeNNessFighterRunUniqueCountLow,
    sNdsNativeNNessFighterRunUniqueDenseLow,
    sNdsNativeNNessFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterTrianglesLow),
    sNdsNativeNNessFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterRunsLow),
    sNdsNativeNNessFighterPrimitiveGroupFirstLow,
    sNdsNativeNNessFighterPrimitiveGroupCountLow,
    sNdsNativeNNessFighterPrimitiveGroupTypeLow,
    sNdsNativeNNessFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNNessFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNNessFighterPrimitiveVerticesLow,
    sNdsNativeNNessFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNNessFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNNessHighOwner, &sNdsNativeNNessFighterHighTables,
    sNdsNativeNNessRoots, sNdsNativeNNessCrossPaletteSlots,
    sNdsNativeNNessRootLightPreambles, NDS_NATIVE_NNESS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNNessLowOwner, &sNdsNativeNNessFighterLowTables,
    sNdsNativeNNessRootsLow, sNdsNativeNNessCrossPaletteSlotsLow,
    sNdsNativeNNessRootLightPreambles, NDS_NATIVE_NNESS_MODEL_DATA_SIZE);
#endif

#if NDS_P2_1P_GAME
#if NDS_NATIVE_OWNER_IMAGE_BOSS
static NDSNativeFighterRuntimeTables sNdsNativeBossFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeBossFighterHighTables =
{
    sNdsNativeBossFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeBossFighterStateDeltas),
    sNdsNativeBossFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeBossFighterStateSequence),
    sNdsNativeBossFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeBossFighterVertexActions),
    sNdsNativeBossFighterEpochDirectPolicy,
    sNdsNativeBossFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeBossFighterDenseVertices),
    sNdsNativeBossFighterDenseNormals,
    sNdsNativeBossFighterPreparedDense,
    sNdsNativeBossFighterActionDenseSpans,
    sNdsNativeBossFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeBossFighterPackedCorners),
    sNdsNativeBossFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeBossFighterRunFirstCorner),
    sNdsNativeBossFighterRunFirstUnique,
    sNdsNativeBossFighterRunUniqueCount,
    sNdsNativeBossFighterRunUniqueDense,
    sNdsNativeBossFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeBossFighterTriangles),
    sNdsNativeBossFighterRuns,
    NDS_FTR_COUNT(sNdsNativeBossFighterRuns),
    sNdsNativeBossFighterPrimitiveGroupFirst,
    sNdsNativeBossFighterPrimitiveGroupCount,
    sNdsNativeBossFighterPrimitiveGroupType,
    sNdsNativeBossFighterPrimitiveGroupFirstVertex,
    sNdsNativeBossFighterPrimitiveGroupVertexCount,
    sNdsNativeBossFighterPrimitiveVertices,
    sNdsNativeBossFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeBossFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_BOSS
static NDSNativeFighterRuntimeTables sNdsNativeBossFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeBossFighterLowTables =
{
    sNdsNativeBossFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterStateDeltasLow),
    sNdsNativeBossFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterStateSequenceLow),
    sNdsNativeBossFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterVertexActionsLow),
    sNdsNativeBossFighterEpochDirectPolicyLow,
    sNdsNativeBossFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterDenseVerticesLow),
    sNdsNativeBossFighterDenseNormalsLow,
    sNdsNativeBossFighterPreparedDenseLow,
    sNdsNativeBossFighterActionDenseSpansLow,
    sNdsNativeBossFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterPackedCornersLow),
    sNdsNativeBossFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterRunFirstCornerLow),
    sNdsNativeBossFighterRunFirstUniqueLow,
    sNdsNativeBossFighterRunUniqueCountLow,
    sNdsNativeBossFighterRunUniqueDenseLow,
    sNdsNativeBossFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterTrianglesLow),
    sNdsNativeBossFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterRunsLow),
    sNdsNativeBossFighterPrimitiveGroupFirstLow,
    sNdsNativeBossFighterPrimitiveGroupCountLow,
    sNdsNativeBossFighterPrimitiveGroupTypeLow,
    sNdsNativeBossFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeBossFighterPrimitiveGroupVertexCountLow,
    sNdsNativeBossFighterPrimitiveVerticesLow,
    sNdsNativeBossFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeBossFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeBossHighOwner, &sNdsNativeBossFighterHighTables,
    sNdsNativeBossRoots, sNdsNativeBossCrossPaletteSlots,
    sNdsNativeBossRootLightPreambles, NDS_NATIVE_BOSS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeBossLowOwner, &sNdsNativeBossFighterLowTables,
    sNdsNativeBossRootsLow, sNdsNativeBossCrossPaletteSlotsLow,
    sNdsNativeBossRootLightPreambles, NDS_NATIVE_BOSS_MODEL_DATA_SIZE);
#endif

#undef NDS_FTR_OWNER_RUNTIME
