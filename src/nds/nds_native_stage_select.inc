/* Native stage packet selection (P2-4n1 step 5).  Hand written; the packets
 * it selects between are generated.
 *
 * Included by nds_renderer_assets.c immediately after the generated stage
 * packets, i.e. after every `sNdsNativeStage*` table exists and before any
 * consumer reads one.
 *
 * WHY A PACKET OF POINTERS RATHER THAN CONCATENATED TABLES
 * -------------------------------------------------------
 * The alternative shape -- one symbol set holding every stage's rows with a
 * per-stage base offset -- is not implementable against the packed row
 * structs.  `NDSNativeStageSegment.first_binding` and
 * `NDSNativeStageRun.binding_index` are u8, and `NDSNativeStageDenseVertex.
 * matrix_binding` is u8; Dream Land alone spends 42 of those 256 values and
 * the eight VS stages together would overflow them.  Widening the fields
 * grows every stage's const slab and moves Dream Land's frozen bytes, which
 * is the one thing this change may not do.  Concatenation would also force
 * every stage into one generated artifact, so a per-stage build flag could no
 * longer add or remove a stage without rewriting the shared arrays -- and it
 * would put a base add on every index in the corner and vertex loops.
 *
 * Per-stage namespaced tables plus a selected packet costs one pointer load
 * where a literal used to be, and nothing at all in a single-stage build (see
 * NDS_NATIVE_STAGE_MULTI below).
 *
 * BYTES
 * -----
 * Pointer/count metadata is resident per linked stage. The payload dominates:
 * Dream Land's slab is 12,663 bytes, Yoster's 11,236, Castle's 8,867 and
 * Jungle's 12,479. Workspaces use the maxima across the linked packets.
 */

/* At 0 consumers retain direct access to Dream Land's generated constants. */
#if (defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)) || \
    (defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1)) || \
    (defined(NDS_P2_STAGE_CASTLE) && (NDS_P2_STAGE_CASTLE == 1))
#define NDS_NATIVE_STAGE_MULTI 1
#else
#define NDS_NATIVE_STAGE_MULTI 0
#endif

#define NDS_NATIVE_STAGE_MAX2(a, b) (((a) > (b)) ? (a) : (b))

/* VS starter kinds, in decomp gr/grdef.h:11-18 order (nGRKindCastle is the
 * base).  Mirrored here rather than included because this file is compiled
 * into the port renderer, not a decomp translation unit. */
#define NDS_NATIVE_STAGE_GKIND_YOSTER 5u
#define NDS_NATIVE_STAGE_GKIND_JUNGLE 2u
#define NDS_NATIVE_STAGE_GKIND_CASTLE 0u
#define NDS_NATIVE_STAGE_GKIND_PUPUPU 6u
#define NDS_NATIVE_STAGE_GKIND_COUNT 8u

/* Written by the scene loader (src/port/diagnostics_renderer_census.c). */
extern volatile u32 gNdsSCVSBattleStageGKind;

/* Workspace maxima over the linked stages.  Every array whose length must be
 * a compile-time constant is sized from these, never from the active packet's
 * counts, so a second stage cannot overflow a buffer sized for the first.
 * With one stage linked MAX == Dream Land's generated literal exactly.
 *
 * These are ENUM CONSTANTS, not macros, and that is load-bearing: macro
 * bodies expand at USE, and the redirect below redefines
 * NDS_NATIVE_STAGE_RUN_COUNT and friends as struct reads.  A macro-defined
 * maximum would therefore expand to a runtime value at the array
 * declarations further down the translation unit -- "variably modified 'runs'
 * at file scope", seven of them, which is exactly how this was found.  An
 * enum binds its value here, while the generated literals still mean
 * literals. */
enum
{
#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)
    NDS_NATIVE_STAGE_BASE_SEGMENT_COUNT =
        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_SEGMENT_COUNT,
                              NDS_NATIVE_STAGE_YOSTER_SEGMENT_COUNT),
    NDS_NATIVE_STAGE_BASE_RUN_COUNT =
        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_RUN_COUNT,
                              NDS_NATIVE_STAGE_YOSTER_RUN_COUNT),
    NDS_NATIVE_STAGE_BASE_DENSE_VERTEX_COUNT =
        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT,
                              NDS_NATIVE_STAGE_YOSTER_DENSE_VERTEX_COUNT),
#else
    NDS_NATIVE_STAGE_BASE_SEGMENT_COUNT = NDS_NATIVE_STAGE_SEGMENT_COUNT,
    NDS_NATIVE_STAGE_BASE_RUN_COUNT = NDS_NATIVE_STAGE_RUN_COUNT,
    NDS_NATIVE_STAGE_BASE_DENSE_VERTEX_COUNT = NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT,
#endif
#if defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1)
    NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT =
        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_BASE_SEGMENT_COUNT,
                              NDS_NATIVE_STAGE_JUNGLE_SEGMENT_COUNT),
    NDS_NATIVE_STAGE_MAX_RUN_COUNT =
        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_BASE_RUN_COUNT,
                              NDS_NATIVE_STAGE_JUNGLE_RUN_COUNT),
    NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT =
        NDS_NATIVE_STAGE_MAX2(NDS_NATIVE_STAGE_BASE_DENSE_VERTEX_COUNT,
                              NDS_NATIVE_STAGE_JUNGLE_DENSE_VERTEX_COUNT)
#else
    NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT = NDS_NATIVE_STAGE_BASE_SEGMENT_COUNT,
    NDS_NATIVE_STAGE_MAX_RUN_COUNT = NDS_NATIVE_STAGE_BASE_RUN_COUNT,
    NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT = NDS_NATIVE_STAGE_BASE_DENSE_VERTEX_COUNT
#endif
};

/* Dream Land's submit-class census, the values ndsRendererNativeStageValidate-
 * TopologyFull compared as bare 66u/126u/10u literals before this file
 * existed.  Declared once here and used both in Dream Land's packet row and
 * in the single-stage macro arm below, so there is one source of truth.
 * (They match scripts/stages/native_stage_descriptors/dreamland.py:42
 * "submit_classes": (66, 126, 10).) */
#define NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_RAW 66u
#define NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_NO_Z 126u
#define NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_RANGE 10u
/* Source transform flags 2/4 are MVP recalc kinds 48/46. */
#define NDS_NATIVE_STAGE_DREAMLAND_CAMERA_BINDINGS 0x7ec38ULL
/* Dream Land's rigid-binding mask, the value nds_renderer.h has carried as
 * NDS_RENDERER_TASK36_RIGID_BINDING_MASK: bindings 0-19, 22-23 and 31-33 are
 * the four static layer segments, everything else is an animated map GObj.
 * Honour a -D override of that macro exactly as the renderer always has. */
#ifdef NDS_RENDERER_TASK36_RIGID_BINDING_MASK
#define NDS_NATIVE_STAGE_DREAMLAND_RIGID_BINDING_MASK \
    NDS_RENDERER_TASK36_RIGID_BINDING_MASK
#else
#define NDS_NATIVE_STAGE_DREAMLAND_RIGID_BINDING_MASK 0x00000381c00fffffULL
#endif

typedef struct NDSNativeStagePacket
{
    const NDSNativeStageAsset *assets;
    const NDSNativeStageSegment *segments;
    const NDSNativeStageDObj *dobjs;
    const NDSNativeStageBinding *bindings;
    const NDSNativeStageRun *runs;
    const NDSNativeStageDenseVertex *vertices;
    const u16 *corners;
    const NDSNativeStageTextureEpoch *epochs;
    const NDSNativeStageMaterialEvent *materials;
    const NDSNativeStageStatePolicy *policies;
    const NDSNativeStageStateDelta *state_deltas;
    const u8 *state_sequence;
    const NDSNativeStageStateSpan *state_spans;
#if NDS_TASK51_STAGE_NATIVE
    const s32 (*baked_world)[16];
#endif
    u16 asset_count;
    u16 segment_count;
    u16 dobj_count;
    u16 binding_count;
    u16 run_count;
    u16 epoch_count;
    u16 material_event_count;
    u16 policy_count;
    u16 state_delta_count;
    u16 state_sequence_count;
    u16 state_span_count;
    u16 dense_vertex_count;
    u16 corner_count;
    u16 triangle_count;
    u16 source_command_count;
    u16 source_vertex_count;
    u16 vertex_command_count;
    u16 triangle_command_count;
    u16 baked_world_count;
    /* The submit-class census ndsRendererNativeStageValidateTopologyFull
     * re-derives and compares.  These were the hand-mirrored literals
     * 66/126/10 in that function; they are Dream Land's, so they had to
     * become per-stage before a second stage could validate. */
    u16 raw_triangles;
    u16 projected_no_z_triangles;
    u16 projected_range_triangles;
    u16 cross_run_count;
    u16 cross_triangle_count;
    u16 cross_corner_count;
    /* Which bindings never move between frames, bit per binding index. The
     * Task 36 replay contract is that every binding in its replayed segments
     * is in this mask; the adapter classifies the live bindings the same way
     * and the renderer refuses to seed a replay when the two disagree. It
     * was a single Dream Land literal in nds_renderer.h before this field. */
    u64 rigid_binding_mask;
    /* Source MVP recalc bindings need live camera-dependent composed rows,
     * even when their own DObj transform and animation are constant. */
    u64 camera_binding_mask;
    /* 1 only for the stage whose straight-line segment-0 program the shared
     * renderer carries (Dream Land).  A stage without one runs the general
     * per-run path for every segment: correct, just not specialised. */
    u8 has_generated_segment0;
    u8 gkind;
    u8 reserved[2];
} NDSNativeStagePacket;

static const NDSNativeStagePacket sNdsNativeStagePacketDreamLand = {
    sNdsNativeStageAssets,
    sNdsNativeStageSegments,
    sNdsNativeStageDObjs,
    sNdsNativeStageBindings,
    sNdsNativeStageRuns,
    sNdsNativeStageVertices,
    sNdsNativeStageCorners,
    sNdsNativeStageTextureEpochs,
    sNdsNativeStageMaterialEvents,
    sNdsNativeStageStatePolicies,
    sNdsNativeStageStateDeltas,
    sNdsNativeStageStateSequence,
    sNdsNativeStageStateSpans,
#if NDS_TASK51_STAGE_NATIVE
    sNdsNativeStageBakedWorldMatrices,
#endif
    NDS_NATIVE_STAGE_ASSET_COUNT,
    NDS_NATIVE_STAGE_SEGMENT_COUNT,
    NDS_NATIVE_STAGE_DOBJ_COUNT,
    NDS_NATIVE_STAGE_BINDING_COUNT,
    NDS_NATIVE_STAGE_RUN_COUNT,
    NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT,
    NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT,
    NDS_NATIVE_STAGE_STATE_POLICY_COUNT,
    NDS_NATIVE_STAGE_STATE_DELTA_COUNT,
    NDS_NATIVE_STAGE_STATE_SEQUENCE_COUNT,
    NDS_NATIVE_STAGE_STATE_SPAN_COUNT,
    NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_CORNER_COUNT,
    NDS_NATIVE_STAGE_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_SOURCE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_SOURCE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_VERTEX_COMMAND_COUNT,
    NDS_NATIVE_STAGE_TRIANGLE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_BAKED_WORLD_COUNT,
    NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_RAW,
    NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_NO_Z,
    NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_RANGE,
    NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT,
    NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT,
    NDS_NATIVE_STAGE_DREAMLAND_RIGID_BINDING_MASK,
    NDS_NATIVE_STAGE_DREAMLAND_CAMERA_BINDINGS,
    1u,
    NDS_NATIVE_STAGE_GKIND_PUPUPU,
    { 0u, 0u }
};

#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)
static const NDSNativeStagePacket sNdsNativeStagePacketYoster = {
    sNdsNativeStageYosterAssets,
    sNdsNativeStageYosterSegments,
    sNdsNativeStageYosterDObjs,
    sNdsNativeStageYosterBindings,
    sNdsNativeStageYosterRuns,
    sNdsNativeStageYosterVertices,
    sNdsNativeStageYosterCorners,
    sNdsNativeStageYosterTextureEpochs,
    sNdsNativeStageYosterMaterialEvents,
    sNdsNativeStageYosterStatePolicies,
    sNdsNativeStageYosterStateDeltas,
    sNdsNativeStageYosterStateSequence,
    sNdsNativeStageYosterStateSpans,
#if NDS_TASK51_STAGE_NATIVE
    sNdsNativeStageYosterBakedWorldMatrices,
#endif
    NDS_NATIVE_STAGE_YOSTER_ASSET_COUNT,
    NDS_NATIVE_STAGE_YOSTER_SEGMENT_COUNT,
    NDS_NATIVE_STAGE_YOSTER_DOBJ_COUNT,
    NDS_NATIVE_STAGE_YOSTER_BINDING_COUNT,
    NDS_NATIVE_STAGE_YOSTER_RUN_COUNT,
    NDS_NATIVE_STAGE_YOSTER_TEXTURE_EPOCH_COUNT,
    NDS_NATIVE_STAGE_YOSTER_MATERIAL_EVENT_COUNT,
    NDS_NATIVE_STAGE_YOSTER_STATE_POLICY_COUNT,
    NDS_NATIVE_STAGE_YOSTER_STATE_DELTA_COUNT,
    NDS_NATIVE_STAGE_YOSTER_STATE_SEQUENCE_COUNT,
    NDS_NATIVE_STAGE_YOSTER_STATE_SPAN_COUNT,
    NDS_NATIVE_STAGE_YOSTER_DENSE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_YOSTER_CORNER_COUNT,
    NDS_NATIVE_STAGE_YOSTER_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_YOSTER_SOURCE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_YOSTER_SOURCE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_YOSTER_VERTEX_COMMAND_COUNT,
    NDS_NATIVE_STAGE_YOSTER_TRIANGLE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_YOSTER_BAKED_WORLD_COUNT,
    /* Measured by the generator, recorded in
     * scripts/stages/native_stage_descriptors/yoster.py
     * ("submit classes (raw 35, no-Z 87, range/matrix 42)"). */
    35u, 87u, 42u,
    NDS_NATIVE_STAGE_YOSTER_CROSS_MATRIX_RUN_COUNT,
    NDS_NATIVE_STAGE_YOSTER_CROSS_MATRIX_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_YOSTER_CROSS_MATRIX_FOREIGN_CORNER_COUNT,
    /* Layer0 is animated: 111_StageYosterFile2.c's 18-entry AnimJoint table
     * at 0x1150 leaves only drawable DObjs 3/5 (bindings 2/4) without a
     * script or animated ancestor. Layers 1-3 have no joint animation
     * (263_GRYosterMap.c), so bindings 15-18 are also rigid. The separate
     * cloud actors are outside this packet. */
    0x78014ULL,
    0u,
    0u,
    NDS_NATIVE_STAGE_GKIND_YOSTER,
    { 0u, 0u }
};
#endif

#if defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1)
static const NDSNativeStagePacket sNdsNativeStagePacketJungle = {
    sNdsNativeStageJungleAssets,
    sNdsNativeStageJungleSegments,
    sNdsNativeStageJungleDObjs,
    sNdsNativeStageJungleBindings,
    sNdsNativeStageJungleRuns,
    sNdsNativeStageJungleVertices,
    sNdsNativeStageJungleCorners,
    sNdsNativeStageJungleTextureEpochs,
    sNdsNativeStageJungleMaterialEvents,
    sNdsNativeStageJungleStatePolicies,
    sNdsNativeStageJungleStateDeltas,
    sNdsNativeStageJungleStateSequence,
    sNdsNativeStageJungleStateSpans,
#if NDS_TASK51_STAGE_NATIVE
    sNdsNativeStageJungleBakedWorldMatrices,
#endif
    NDS_NATIVE_STAGE_JUNGLE_ASSET_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_SEGMENT_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_DOBJ_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_BINDING_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_RUN_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_TEXTURE_EPOCH_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_MATERIAL_EVENT_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_STATE_POLICY_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_STATE_DELTA_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_STATE_SEQUENCE_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_STATE_SPAN_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_DENSE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_CORNER_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_SOURCE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_SOURCE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_VERTEX_COMMAND_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_TRIANGLE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_BAKED_WORLD_COUNT,
    26u, 136u, 20u,
    NDS_NATIVE_STAGE_JUNGLE_CROSS_MATRIX_RUN_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_CROSS_MATRIX_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_JUNGLE_CROSS_MATRIX_FOREIGN_CORNER_COUNT,
    /* Layer1 DObjs 1/2 (bindings 17/18) are the animated platforms. */
    0x3ff9ffffULL,
    0x1ec03c7cULL,
    0u,
    NDS_NATIVE_STAGE_GKIND_JUNGLE,
    { 0u, 0u }
};
#endif

#if defined(NDS_P2_STAGE_CASTLE) && (NDS_P2_STAGE_CASTLE == 1)
_Static_assert(NDS_NATIVE_STAGE_CASTLE_SEGMENT_COUNT <= NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT &&
               NDS_NATIVE_STAGE_CASTLE_RUN_COUNT <= NDS_NATIVE_STAGE_MAX_RUN_COUNT &&
               NDS_NATIVE_STAGE_CASTLE_DENSE_VERTEX_COUNT <= NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT,
               "Castle packet exceeds the native stage workspace");
static const NDSNativeStagePacket sNdsNativeStagePacketCastle = {
    sNdsNativeStageCastleAssets,
    sNdsNativeStageCastleSegments,
    sNdsNativeStageCastleDObjs,
    sNdsNativeStageCastleBindings,
    sNdsNativeStageCastleRuns,
    sNdsNativeStageCastleVertices,
    sNdsNativeStageCastleCorners,
    sNdsNativeStageCastleTextureEpochs,
    sNdsNativeStageCastleMaterialEvents,
    sNdsNativeStageCastleStatePolicies,
    sNdsNativeStageCastleStateDeltas,
    sNdsNativeStageCastleStateSequence,
    sNdsNativeStageCastleStateSpans,
#if NDS_TASK51_STAGE_NATIVE
    sNdsNativeStageCastleBakedWorldMatrices,
#endif
    NDS_NATIVE_STAGE_CASTLE_ASSET_COUNT,
    NDS_NATIVE_STAGE_CASTLE_SEGMENT_COUNT,
    NDS_NATIVE_STAGE_CASTLE_DOBJ_COUNT,
    NDS_NATIVE_STAGE_CASTLE_BINDING_COUNT,
    NDS_NATIVE_STAGE_CASTLE_RUN_COUNT,
    NDS_NATIVE_STAGE_CASTLE_TEXTURE_EPOCH_COUNT,
    NDS_NATIVE_STAGE_CASTLE_MATERIAL_EVENT_COUNT,
    NDS_NATIVE_STAGE_CASTLE_STATE_POLICY_COUNT,
    NDS_NATIVE_STAGE_CASTLE_STATE_DELTA_COUNT,
    NDS_NATIVE_STAGE_CASTLE_STATE_SEQUENCE_COUNT,
    NDS_NATIVE_STAGE_CASTLE_STATE_SPAN_COUNT,
    NDS_NATIVE_STAGE_CASTLE_DENSE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_CASTLE_CORNER_COUNT,
    NDS_NATIVE_STAGE_CASTLE_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_CASTLE_SOURCE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_CASTLE_SOURCE_VERTEX_COUNT,
    NDS_NATIVE_STAGE_CASTLE_VERTEX_COMMAND_COUNT,
    NDS_NATIVE_STAGE_CASTLE_TRIANGLE_COMMAND_COUNT,
    NDS_NATIVE_STAGE_CASTLE_BAKED_WORLD_COUNT,
    56u, 62u, 18u,
    NDS_NATIVE_STAGE_CASTLE_CROSS_MATRIX_RUN_COUNT,
    NDS_NATIVE_STAGE_CASTLE_CROSS_MATRIX_TRIANGLE_COUNT,
    NDS_NATIVE_STAGE_CASTLE_CROSS_MATRIX_FOREIGN_CORNER_COUNT,
    /* Geometry-file joint scripts animate bindings 6/7/8/9. */
    0xc3fULL,
    0x6ULL,
    0u,
    NDS_NATIVE_STAGE_GKIND_CASTLE,
    { 0u, 0u }
};
#endif

/* The small resident registry.  One row per VS starter kind, in gr/grdef.h
 * order: Castle 0, Sector 1, Jungle 2, Zebes 3, Hyrule 4, Yoster 5,
 * Pupupu 6, Yamabuki 7.  A kind with no generated packet is NULL and MUST
 * stay NULL: resolving an unbaked stage to Dream Land's geometry is the
 * masquerade this table used to perform. */
static const NDSNativeStagePacket *const
    sNdsNativeStagePacketTable[NDS_NATIVE_STAGE_GKIND_COUNT] = {
#if defined(NDS_P2_STAGE_CASTLE) && (NDS_P2_STAGE_CASTLE == 1)
        &sNdsNativeStagePacketCastle,
#else
        NULL,
#endif
        NULL,
#if defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1)
        &sNdsNativeStagePacketJungle,
#else
        NULL,
#endif
        NULL,
        NULL,
#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)
        &sNdsNativeStagePacketYoster,
#else
        NULL,
#endif
        &sNdsNativeStagePacketDreamLand,
        NULL
    };

/* Resolved once per stage-owner preparation, never per run.  NULL means "this
 * kind has no native packet"; every caller must decline rather than draw
 * another stage's geometry. */
static const NDSNativeStagePacket *sNdsNativeStagePacketActive =
    &sNdsNativeStagePacketDreamLand;

/* Declining is not silent.  A stage that drops to the generic renderer because
 * its kind has no packet is indistinguishable, from the outside, from one that
 * drops for any of the other reject reasons -- and that ambiguity has cost
 * this project multi-hour hunts before.  These two are one GDB read.
 * gc-sections drops unreferenced diagnostic globals, hence `used`. */
__attribute__((used)) volatile u32 gNdsNativeStagePacketUnresolvedCount;
__attribute__((used)) volatile u32 gNdsNativeStagePacketUnresolvedKind = 0xffu;

static const NDSNativeStagePacket *ndsRendererNativeStageResolvePacket(void)
{
    u32 kind = gNdsSCVSBattleStageGKind;

    return (kind < NDS_NATIVE_STAGE_GKIND_COUNT) ?
        sNdsNativeStagePacketTable[kind] : NULL;
}

/* Called at the top of every native stage entry point.  Returns FALSE when
 * the selected kind has no packet, which is the signal to decline. */
static s32 ndsRendererNativeStageSelectPacket(void)
{
    const NDSNativeStagePacket *packet = ndsRendererNativeStageResolvePacket();

    if (packet == NULL)
    {
        gNdsNativeStagePacketUnresolvedCount++;
        gNdsNativeStagePacketUnresolvedKind = gNdsSCVSBattleStageGKind;
        return FALSE;
    }
    sNdsNativeStagePacketActive = packet;
    return TRUE;
}

#if NDS_NATIVE_STAGE_MULTI
/* Route every consumer through the selected packet.  The generated names are
 * kept deliberately: a reader of nds_renderer_native_owners.c sees
 * `sNdsNativeStageRuns` and gets the runs of the stage that is actually
 * loaded, which is what that code always meant.  Declarations that need a
 * compile-time length use NDS_NATIVE_STAGE_MAX_* instead. */
#undef NDS_NATIVE_STAGE_ASSET_COUNT
#undef NDS_NATIVE_STAGE_SEGMENT_COUNT
#undef NDS_NATIVE_STAGE_DOBJ_COUNT
#undef NDS_NATIVE_STAGE_BINDING_COUNT
#undef NDS_NATIVE_STAGE_RUN_COUNT
#undef NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT
#undef NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT
#undef NDS_NATIVE_STAGE_STATE_POLICY_COUNT
#undef NDS_NATIVE_STAGE_STATE_DELTA_COUNT
#undef NDS_NATIVE_STAGE_STATE_SEQUENCE_COUNT
#undef NDS_NATIVE_STAGE_STATE_SPAN_COUNT
#undef NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT
#undef NDS_NATIVE_STAGE_CORNER_COUNT
#undef NDS_NATIVE_STAGE_TRIANGLE_COUNT
#undef NDS_NATIVE_STAGE_SOURCE_COMMAND_COUNT
#undef NDS_NATIVE_STAGE_SOURCE_VERTEX_COUNT
#undef NDS_NATIVE_STAGE_VERTEX_COMMAND_COUNT
#undef NDS_NATIVE_STAGE_TRIANGLE_COMMAND_COUNT
#undef NDS_NATIVE_STAGE_BAKED_WORLD_COUNT
#undef NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT
#undef NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT
#undef NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT

#define NDS_NATIVE_STAGE_ASSET_COUNT (sNdsNativeStagePacketActive->asset_count)
#define NDS_NATIVE_STAGE_SEGMENT_COUNT (sNdsNativeStagePacketActive->segment_count)
#define NDS_NATIVE_STAGE_DOBJ_COUNT (sNdsNativeStagePacketActive->dobj_count)
#define NDS_NATIVE_STAGE_BINDING_COUNT (sNdsNativeStagePacketActive->binding_count)
#define NDS_NATIVE_STAGE_RUN_COUNT (sNdsNativeStagePacketActive->run_count)
#define NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT (sNdsNativeStagePacketActive->epoch_count)
#define NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT \
    (sNdsNativeStagePacketActive->material_event_count)
#define NDS_NATIVE_STAGE_STATE_POLICY_COUNT (sNdsNativeStagePacketActive->policy_count)
#define NDS_NATIVE_STAGE_STATE_DELTA_COUNT (sNdsNativeStagePacketActive->state_delta_count)
#define NDS_NATIVE_STAGE_STATE_SEQUENCE_COUNT \
    (sNdsNativeStagePacketActive->state_sequence_count)
#define NDS_NATIVE_STAGE_STATE_SPAN_COUNT (sNdsNativeStagePacketActive->state_span_count)
#define NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT \
    (sNdsNativeStagePacketActive->dense_vertex_count)
#define NDS_NATIVE_STAGE_CORNER_COUNT (sNdsNativeStagePacketActive->corner_count)
#define NDS_NATIVE_STAGE_TRIANGLE_COUNT (sNdsNativeStagePacketActive->triangle_count)
#define NDS_NATIVE_STAGE_SOURCE_COMMAND_COUNT \
    (sNdsNativeStagePacketActive->source_command_count)
#define NDS_NATIVE_STAGE_SOURCE_VERTEX_COUNT \
    (sNdsNativeStagePacketActive->source_vertex_count)
#define NDS_NATIVE_STAGE_VERTEX_COMMAND_COUNT \
    (sNdsNativeStagePacketActive->vertex_command_count)
#define NDS_NATIVE_STAGE_TRIANGLE_COMMAND_COUNT \
    (sNdsNativeStagePacketActive->triangle_command_count)
#define NDS_NATIVE_STAGE_BAKED_WORLD_COUNT (sNdsNativeStagePacketActive->baked_world_count)
#define NDS_NATIVE_STAGE_CROSS_MATRIX_RUN_COUNT \
    (sNdsNativeStagePacketActive->cross_run_count)
#define NDS_NATIVE_STAGE_CROSS_MATRIX_TRIANGLE_COUNT \
    (sNdsNativeStagePacketActive->cross_triangle_count)
#define NDS_NATIVE_STAGE_CROSS_MATRIX_FOREIGN_CORNER_COUNT \
    (sNdsNativeStagePacketActive->cross_corner_count)

#define sNdsNativeStageAssets (sNdsNativeStagePacketActive->assets)
#define sNdsNativeStageSegments (sNdsNativeStagePacketActive->segments)
#define sNdsNativeStageDObjs (sNdsNativeStagePacketActive->dobjs)
#define sNdsNativeStageBindings (sNdsNativeStagePacketActive->bindings)
#define sNdsNativeStageRuns (sNdsNativeStagePacketActive->runs)
#define sNdsNativeStageVertices (sNdsNativeStagePacketActive->vertices)
#define sNdsNativeStageCorners (sNdsNativeStagePacketActive->corners)
#define sNdsNativeStageTextureEpochs (sNdsNativeStagePacketActive->epochs)
#define sNdsNativeStageMaterialEvents (sNdsNativeStagePacketActive->materials)
#define sNdsNativeStageStatePolicies (sNdsNativeStagePacketActive->policies)
#define sNdsNativeStageStateDeltas (sNdsNativeStagePacketActive->state_deltas)
#define sNdsNativeStageStateSequence (sNdsNativeStagePacketActive->state_sequence)
#define sNdsNativeStageStateSpans (sNdsNativeStagePacketActive->state_spans)
#if NDS_TASK51_STAGE_NATIVE
#define sNdsNativeStageBakedWorldMatrices (sNdsNativeStagePacketActive->baked_world)
#endif
#endif /* NDS_NATIVE_STAGE_MULTI */

/* The census the topology validator compares against, and the segment-program
 * gate.  A load through a mutable global pointer is NOT constant-foldable, so
 * a single-stage build takes the literals it always had -- otherwise this file
 * would silently turn four compile-time comparisons into runtime loads in the
 * Dream-Land-only ROMs, including the frozen P1 artifact. */
#if NDS_NATIVE_STAGE_MULTI
#define NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES \
    (sNdsNativeStagePacketActive->raw_triangles)
#define NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES \
    (sNdsNativeStagePacketActive->projected_no_z_triangles)
#define NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES \
    (sNdsNativeStagePacketActive->projected_range_triangles)
#define NDS_NATIVE_STAGE_HAS_GENERATED_SEGMENT0 \
    (sNdsNativeStagePacketActive->has_generated_segment0)
#define NDS_NATIVE_STAGE_RIGID_BINDING_MASK \
    (sNdsNativeStagePacketActive->rigid_binding_mask & \
     ~sNdsNativeStagePacketActive->camera_binding_mask)
#else
#define NDS_NATIVE_STAGE_SUBMIT_RAW_TRIANGLES \
    NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_RAW
#define NDS_NATIVE_STAGE_SUBMIT_NO_Z_TRIANGLES \
    NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_NO_Z
#define NDS_NATIVE_STAGE_SUBMIT_RANGE_TRIANGLES \
    NDS_NATIVE_STAGE_DREAMLAND_SUBMIT_RANGE
#define NDS_NATIVE_STAGE_HAS_GENERATED_SEGMENT0 1u
#define NDS_NATIVE_STAGE_RIGID_BINDING_MASK \
    (NDS_NATIVE_STAGE_DREAMLAND_RIGID_BINDING_MASK & \
     ~NDS_NATIVE_STAGE_DREAMLAND_CAMERA_BINDINGS)
#endif

/* Read by the stage ADAPTER (a separate translation unit) so its capture
 * side can classify the live bindings against the stage the renderer
 * selected, instead of against Dream Land's literals. */
u64 ndsRendererNativeStageRigidBindingMask(void)
{
    /* Capture runs before PrepareNativeStageOwner selects the draw packet.
     * Read the loaded kind here; the active pointer may still name the last
     * scene (or the initial Dream Land packet on the first Yoster frame). */
    const NDSNativeStagePacket *packet = ndsRendererNativeStageResolvePacket();

    return (packet != NULL) ?
        packet->rigid_binding_mask & ~packet->camera_binding_mask : 0u;
}

/* The material shadow the adapter keeps per material slot is bound to one
 * stage binding and expects one MObj flag word. Both live in the packet's
 * material-event table (binding_index / flags per material_slot), which is
 * where the adapter's hand-written {20,22,31,32} / {1,1,0x6b,0x6b} came from.
 * FALSE when the active stage has no such slot. */
s32 ndsRendererNativeStageMaterialBinding(u32 material_slot,
                                          u32 *binding_index, u32 *flags)
{
    const NDSNativeStagePacket *packet = ndsRendererNativeStageResolvePacket();
    u32 i;

    if (packet == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < packet->material_event_count; i++)
    {
        const NDSNativeStageMaterialEvent *event =
            &packet->materials[i];

        if (event->material_slot == material_slot)
        {
            *binding_index = event->binding_index;
            *flags = event->flags;
            return TRUE;
        }
    }
    return FALSE;
}

u32 ndsRendererNativeStageMaterialMask(u32 segment_index)
{
    const NDSNativeStagePacket *packet = ndsRendererNativeStageResolvePacket();
    u32 mask = 0u;
    u32 i;

    if ((packet == NULL) || (segment_index >= packet->segment_count))
    {
        return 0u;
    }
    for (i = 0u; i < packet->material_event_count; i++)
    {
        const NDSNativeStageMaterialEvent *event = &packet->materials[i];

        if (event->segment_index == segment_index)
        {
            mask |= 1u << event->material_slot;
        }
    }
    return mask;
}
