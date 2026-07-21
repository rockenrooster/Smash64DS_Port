#ifndef SSB64_NDS_RENDERER_H
#define SSB64_NDS_RENDERER_H

#include <stddef.h>
#include <PR/gbi.h>

#ifndef NDS_RENDERER_PROFILE_LEVEL
#define NDS_RENDERER_PROFILE_LEVEL 2
#endif

#if (NDS_RENDERER_PROFILE_LEVEL < 0) || (NDS_RENDERER_PROFILE_LEVEL > 2)
#error "NDS_RENDERER_PROFILE_LEVEL must be 0, 1, or 2"
#endif

#ifndef NDS_RENDERER_M2_DETAILED_LEDGER
#define NDS_RENDERER_M2_DETAILED_LEDGER 0
#endif

#if (NDS_RENDERER_M2_DETAILED_LEDGER != 0) && \
    (NDS_RENDERER_M2_DETAILED_LEDGER != 1)
#error "NDS_RENDERER_M2_DETAILED_LEDGER must be 0 or 1"
#endif

#if NDS_RENDERER_M2_DETAILED_LEDGER && \
    (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_RENDERER_M2_DETAILED_LEDGER requires profile level 1"
#endif

#ifndef NDS_RENDERER_M3_PHASE0_PROFILE
#define NDS_RENDERER_M3_PHASE0_PROFILE 0
#endif

#if (NDS_RENDERER_M3_PHASE0_PROFILE != 0) && \
    (NDS_RENDERER_M3_PHASE0_PROFILE != 1)
#error "NDS_RENDERER_M3_PHASE0_PROFILE must be 0 or 1"
#endif

#if NDS_RENDERER_M3_PHASE0_PROFILE && \
    (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_RENDERER_M3_PHASE0_PROFILE requires profile level 1"
#endif

/* Task 26 is compile-selected so disabled control builds retain the exact
 * existing stage executor and linked layout. */
#ifndef NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
#define NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE 0
#endif

#if (NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE != 0) && \
    (NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE != 1)
#error "NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE must be 0 or 1"
#endif

/* Task 29's no-behavior GX census is a profile-1 lab surface only.  Shipping
 * builds retain no counters, wrappers, or state. */
#ifndef NDS_TASK29_GX_CENSUS
#define NDS_TASK29_GX_CENSUS 0
#endif

#if (NDS_TASK29_GX_CENSUS != 0) && (NDS_TASK29_GX_CENSUS != 1)
#error "NDS_TASK29_GX_CENSUS must be 0 or 1"
#endif

#if NDS_TASK29_GX_CENSUS && (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_TASK29_GX_CENSUS requires profile level 1"
#endif

/* Task 34 E1 records the exact native-stage GX stream without changing it. */
#ifndef NDS_TASK34_STAGE_STREAM_CENSUS
#define NDS_TASK34_STAGE_STREAM_CENSUS 0
#endif

#if (NDS_TASK34_STAGE_STREAM_CENSUS != 0) && \
    (NDS_TASK34_STAGE_STREAM_CENSUS != 1)
#error "NDS_TASK34_STAGE_STREAM_CENSUS must be 0 or 1"
#endif

#if NDS_TASK34_STAGE_STREAM_CENSUS && !NDS_TASK29_GX_CENSUS
#error "NDS_TASK34_STAGE_STREAM_CENSUS requires NDS_TASK29_GX_CENSUS"
#endif

#ifndef NDS_TASK36_HW_COMPOSE
#define NDS_TASK36_HW_COMPOSE 0
#endif

#if (NDS_TASK36_HW_COMPOSE != 0) && (NDS_TASK36_HW_COMPOSE != 1)
#error "NDS_TASK36_HW_COMPOSE must be 0 or 1"
#endif

#ifndef NDS_RENDERER_SCREEN_SPACE_CENSUS
#define NDS_RENDERER_SCREEN_SPACE_CENSUS 0
#endif

#if (NDS_RENDERER_SCREEN_SPACE_CENSUS != 0) && \
    (NDS_RENDERER_SCREEN_SPACE_CENSUS != 1)
#error "NDS_RENDERER_SCREEN_SPACE_CENSUS must be 0 or 1"
#endif

#if NDS_RENDERER_SCREEN_SPACE_CENSUS && \
    (NDS_RENDERER_PROFILE_LEVEL != 1)
#error "NDS_RENDERER_SCREEN_SPACE_CENSUS requires profile level 1"
#endif

#ifndef NDS_RENDER_ECONOMY
#define NDS_RENDER_ECONOMY 0
#endif

#ifndef NDS_RENDER_ECONOMY_OWNER_MASK
#define NDS_RENDER_ECONOMY_OWNER_MASK 32
#endif

#if (NDS_RENDER_ECONOMY != 0) && (NDS_RENDER_ECONOMY != 1)
#error "NDS_RENDER_ECONOMY must be 0 or 1"
#endif

#if (NDS_RENDER_ECONOMY_OWNER_MASK < 0) || \
    (NDS_RENDER_ECONOMY_OWNER_MASK > 255)
#error "NDS_RENDER_ECONOMY_OWNER_MASK must fit the eight Dream Land owners"
#endif

#define NDS_RENDERER_BENCHMARK_NONE 0
#define NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP 1
#define NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX 2
#define NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD 4

#define NDS_RENDERER_FAST_RUN_GENERIC 0u
#define NDS_RENDERER_FAST_RUN_MARIO_ONLY 1u
#define NDS_RENDERER_FAST_RUN_FIGHTERS 2u
#define NDS_RENDERER_FAST_RUN_ALL_RAW_CURRENT 3u
#define NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES 4u
#define NDS_RENDERER_FAST_RUN_NATIVE_MARIO 5u
#define NDS_RENDERER_FAST_RUN_NATIVE_FOX 6u
#define NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS 7u
#define NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION 8u
#define NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE 9u

#ifndef NDS_RENDERER_BENCHMARK_MODE
#define NDS_RENDERER_BENCHMARK_MODE NDS_RENDERER_BENCHMARK_NONE
#endif

#if NDS_TASK29_GX_CENSUS && \
    (NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE)
#error "NDS_TASK29_GX_CENSUS requires real GX emission"
#endif

#if (NDS_RENDERER_BENCHMARK_MODE < NDS_RENDERER_BENCHMARK_NONE) || \
    (NDS_RENDERER_BENCHMARK_MODE > NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD)
#error "NDS_RENDERER_BENCHMARK_MODE must be 0 through 4"
#endif

#define NDS_RENDERER_BLOCKER_NONE 0u
#define NDS_RENDERER_BLOCKER_BAD_BRANCH 1u
#define NDS_RENDERER_BLOCKER_TOO_DEEP 2u
#define NDS_RENDERER_BLOCKER_BUDGET 3u
#define NDS_RENDERER_BLOCKER_UNSUPPORTED 4u
#define NDS_RENDERER_BLOCKER_NO_VERTICES 5u
#define NDS_RENDERER_BLOCKER_NO_TRIANGLES 6u
#define NDS_RENDERER_BLOCKER_NO_END 7u

#define NDS_RENDERER_RESOLVE_NONE 0u
#define NDS_RENDERER_RESOLVE_SEGMENT 1u

#define NDS_RENDERER_TEXTURE_SETCOMBINE (1u << 0)
#define NDS_RENDERER_TEXTURE_SETTILE (1u << 1)
#define NDS_RENDERER_TEXTURE_TEXTURE (1u << 2)
#define NDS_RENDERER_TEXTURE_SETTILESIZE (1u << 3)
#define NDS_RENDERER_TEXTURE_SETTIMG (1u << 4)
#define NDS_RENDERER_TEXTURE_LOADBLOCK (1u << 5)
#define NDS_RENDERER_TEXTURE_LOADTILE (1u << 6)

#define NDS_RENDERER_TEXTURE_STATE_SEEN (1u << 0)
#define NDS_RENDERER_TEXTURE_STATE_ON (1u << 1)
#define NDS_RENDERER_TEXTURE_STATE_SCALE_S (1u << 2)
#define NDS_RENDERER_TEXTURE_STATE_SCALE_T (1u << 3)

#define NDS_RENDERER_TILE_RENDER_SEEN (1u << 0)
#define NDS_RENDERER_TILE_LOAD_SEEN (1u << 1)
#define NDS_RENDERER_TILE_S_CLAMP (1u << 2)
#define NDS_RENDERER_TILE_S_MIRROR (1u << 3)
#define NDS_RENDERER_TILE_S_MASKED (1u << 4)
#define NDS_RENDERER_TILE_T_CLAMP (1u << 5)
#define NDS_RENDERER_TILE_T_MIRROR (1u << 6)
#define NDS_RENDERER_TILE_T_MASKED (1u << 7)

#define NDS_RENDERER_VERTEX_CACHE_SIZE 32u
#define NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY 64u
#define NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX 27u
#define NDS_RENDERER_TILE_COUNT 8u
#define NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT 2u
#define NDS_RENDERER_SEMANTIC_TRACE_CAPACITY 832u

#define NDS_RENDERER_GEOM_ZBUFFER 0x00000001u
#define NDS_RENDERER_GEOM_SHADE 0x00000004u
#define NDS_RENDERER_GEOM_CULL_FRONT 0x00000200u
#define NDS_RENDERER_GEOM_CULL_BACK 0x00000400u
#define NDS_RENDERER_GEOM_FOG 0x00010000u
#define NDS_RENDERER_GEOM_LIGHTING 0x00020000u
#define NDS_RENDERER_GEOM_TEXTURE_GEN 0x00040000u
#define NDS_RENDERER_GEOM_TEXTURE_GEN_LINEAR 0x00080000u
#define NDS_RENDERER_GEOM_SHADING_SMOOTH 0x00200000u
#define NDS_RENDERER_GEOM_RESET_MODE \
    (NDS_RENDERER_GEOM_ZBUFFER | NDS_RENDERER_GEOM_SHADE | \
     NDS_RENDERER_GEOM_CULL_BACK | NDS_RENDERER_GEOM_SHADING_SMOOTH)

typedef s32 (*NDSRendererValidateRange)(const Gfx *dl, size_t bytes,
                                        void *user);
typedef size_t (*NDSRendererImmutableCommandSpan)(const Gfx *dl, void *user);
typedef const Gfx *(*NDSRendererResolveBranch)(const Gfx *dl,
                                               u32 *resolve_kind,
                                               void *user);
typedef const void *(*NDSRendererResolveData)(const void *ptr, size_t bytes,
                                              void *user);

typedef struct NDSRendererMatrix20p12
{
    s32 m[4][4];
} NDSRendererMatrix20p12;

typedef struct NDSRendererMatrixSnapshot
{
    NDSRendererMatrix20p12 matrix;
    u32 generation;
    u32 signature;
} NDSRendererMatrixSnapshot;

typedef struct NDSRendererInputVertex
{
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} NDSRendererInputVertex;

typedef struct NDSRendererClipVertex20p12
{
    s32 x;
    s32 y;
    s32 z;
    s32 w;
} NDSRendererClipVertex20p12;

typedef struct NDSRendererVertexCache
{
    NDSRendererInputVertex input_vertices[NDS_RENDERER_VERTEX_CACHE_SIZE];
    NDSRendererClipVertex20p12
        transformed_vertices[NDS_RENDERER_VERTEX_CACHE_SIZE];
    u32 vertex_colors[NDS_RENDERER_VERTEX_CACHE_SIZE];
    NDSRendererMatrixSnapshot
        matrix_snapshots[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY];
    u8 vertex_matrix_snapshot[NDS_RENDERER_VERTEX_CACHE_SIZE];
    u8 vertex_clip_snapshot[NDS_RENDERER_VERTEX_CACHE_SIZE];
    u32 input_valid_mask;
    u32 transformed_valid_mask;
    u32 vertex_color_valid_mask;
    u32 matrix_snapshot_count;
    u32 raw_vertex_fit_mask;
} NDSRendererVertexCache;

typedef enum NDSRendererProfileOwner
{
    NDS_RENDERER_PROFILE_OWNER_STAGE = 0,
    NDS_RENDERER_PROFILE_OWNER_MARIO,
    NDS_RENDERER_PROFILE_OWNER_FOX,
    NDS_RENDERER_PROFILE_OWNER_COUNT,
    NDS_RENDERER_PROFILE_OWNER_NONE = NDS_RENDERER_PROFILE_OWNER_COUNT
} NDSRendererProfileOwner;

#if NDS_TASK29_GX_CENSUS
typedef enum NDSRendererTask29GXClass
{
    NDS_TASK29_GX_CONTROL = 0,
    NDS_TASK29_GX_ALPHA_TEST,
    NDS_TASK29_GX_FOG_TABLE,
    NDS_TASK29_GX_FOG_OFFSET,
    NDS_TASK29_GX_FOG_COLOR,
    NDS_TASK29_GX_TEXTURE_PARAM,
    NDS_TASK29_GX_TEXTURE_BIND,
    NDS_TASK29_GX_MATRIX_MODE,
    NDS_TASK29_GX_MATRIX_IDENTITY,
    NDS_TASK29_GX_MATRIX_LOAD4X4,
    NDS_TASK29_GX_MATRIX_MULT4X4,
    NDS_TASK29_GX_MATRIX_PUSH,
    NDS_TASK29_GX_MATRIX_POP,
    NDS_TASK29_GX_MATRIX_STORE,
    NDS_TASK29_GX_MATRIX_RESTORE,
    NDS_TASK29_GX_POLY_FORMAT,
    NDS_TASK29_GX_BEGIN,
    NDS_TASK29_GX_END,
    NDS_TASK29_GX_COLOR,
    NDS_TASK29_GX_TEX_COORD,
    NDS_TASK29_GX_VERTEX16,
    NDS_TASK29_GX_FLUSH,
    NDS_TASK29_GX_CLASS_COUNT
} NDSRendererTask29GXClass;

#define NDS_TASK29_GX_OWNER_COUNT \
    (NDS_RENDERER_PROFILE_OWNER_COUNT + 1u)

extern volatile u32 gNdsTask29GXFrame;
extern volatile u32 gNdsTask29GXCommandCount[NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXWordCount[NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXRepeatCount[NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXOwnerCommandCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXOwnerWordCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXOwnerRepeatCount
    [NDS_TASK29_GX_OWNER_COUNT][NDS_TASK29_GX_CLASS_COUNT];
extern volatile u32 gNdsTask29GXTotalCommandCount;
extern volatile u32 gNdsTask29GXTotalWordCount;
extern volatile u32 gNdsTask29GXTotalRepeatCount;
extern volatile u32 gNdsTask29GXStreamHashA;
extern volatile u32 gNdsTask29GXStreamHashB;
extern volatile u32 gNdsTask29GXOwnerHashA[NDS_TASK29_GX_OWNER_COUNT];
extern volatile u32 gNdsTask29GXOwnerHashB[NDS_TASK29_GX_OWNER_COUNT];
extern volatile u32 gNdsTask29GXBoundaryHashA;
extern volatile u32 gNdsTask29GXBoundaryHashB;
extern volatile u32 gNdsTask29GXBoundaryCount;
extern volatile u32 gNdsTask29GXFaultCount;
extern volatile u32 gNdsTask29GXNeverSuppressMask;

#if NDS_TASK34_STAGE_STREAM_CENSUS
#define NDS_TASK34_STAGE_STREAM_ENTRY_CAPACITY 4096u
#define NDS_TASK34_STAGE_STREAM_WORD_CAPACITY 8192u
#define NDS_TASK34_STAGE_STREAM_DOBJ_NONE 0xffffu

typedef struct NDSRendererTask34StageStreamEntry
{
    u16 word_offset;
    u16 dobj_index;
    u8 command_class;
    u8 word_count;
    u8 segment_index;
    u8 reserved;
} NDSRendererTask34StageStreamEntry;

extern volatile u32 gNdsTask34StageStreamFrame;
extern volatile u32 gNdsTask34StageStreamCaptureEnabled;
extern volatile u32 gNdsTask34StageStreamEntryCount;
extern volatile u32 gNdsTask34StageStreamWordCount;
extern volatile u32 gNdsTask34StageStreamOverflowCount;
extern volatile u32 gNdsTask34StageStreamFaultCount;
extern volatile NDSRendererTask34StageStreamEntry
    gNdsTask34StageStreamEntries[NDS_TASK34_STAGE_STREAM_ENTRY_CAPACITY];
extern volatile u32
    gNdsTask34StageStreamWords[NDS_TASK34_STAGE_STREAM_WORD_CAPACITY];

void ndsRendererTask34StageStreamBeginSegment(u32 segment_index);
void ndsRendererTask34StageStreamSetDObj(u32 dobj_index);
void ndsRendererTask34StageStreamEndSegment(void);
#endif

void ndsRendererTask29GXRecordFlush(u32 mode);
void ndsRendererTask29GXSetOwner(NDSRendererProfileOwner owner);
void ndsRendererTask29GXPublishFrame(void);
#endif

#if NDS_RENDERER_SCREEN_SPACE_CENSUS
#define NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT 42u
#define NDS_RENDERER_SCREEN_SPACE_CENSUS_STAGE_OWNER_COUNT 8u

typedef struct NDSRendererScreenSpaceCensusRow
{
    u32 identity;
    u32 triangle_count;
    u32 area_lt_1px_count;
    u32 area_lt_4px_count;
    u32 invalid_count;
    u64 area2_q8_sum;
} NDSRendererScreenSpaceCensusRow;
#endif

/* Diagnostic-only owner census.  Profile 0 never allocates or touches this
 * ledger; profiles 1/2 publish it once per synchronized frame. */
typedef struct NDSRendererOwnerProfile
{
    u32 exclusive_ticks;
    u32 selected_count;
    u32 source_command_count;
    u32 vertex_command_count;
    u32 source_vertex_count;
    u32 triangle_command_count;
    u32 triangle_count;
    u32 submit_class_count[8];
    u32 material_operation_count;
    u32 matrix_change_count;
    u32 texture_change_count;
    u32 run_count;
    u32 entry_state_hash;
    u32 exit_state_hash;
    u32 entry_vertex_cache_hash;
    u32 exit_vertex_cache_hash;
    u32 entry_resolver_hash;
    u32 exit_resolver_hash;
    u32 entry_global_hash;
    u32 exit_global_hash;
    u32 topology_signature;
    u32 selected_event_signature;
    u32 camera_signature;
    u32 dobj_matrix_signature;
    u32 material_signature;
    u32 light_signature;
    u32 texture_signature;
    u32 semantic_output_hash;
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER
    /* M2 decision ledger.  These fields are deliberately absent from the
     * shipping profile-0 owner and from the independent profile-2 oracle. */
    u32 m2_contract_capture_ticks;
    u32 m2_collection_ticks;
    u32 m2_owner_validation_ticks;
    u32 m2_census_ticks;
    u32 m2_camera_fetch_ticks;
    u32 m2_hash_parent_lookup_ticks;
    u32 m2_local_matrix_ticks;
    u32 m2_world_affine_ticks;
    u32 m2_world_camera_ticks;
    u32 m2_final_compose_ticks;
    u32 m2_material_ticks;
    u32 m2_production_total_ticks;
    u32 m2_production_preflight_state_ticks;
    u32 m2_lighting_shading_ticks;
    u32 m2_root_gx_ticks;
    u32 m2_run_prepare_ticks;
    u32 m2_corner_emit_account_ticks;
    u32 m2_owner_residual_ticks;
    u32 m2_production_success_count;
    u32 m2_production_failure_count;
    u32 m2_production_phase_overlap_count;
    u32 m2_owner_phase_overlap_count;
    u32 m2_schedule_joint_count;
    u32 m2_schedule_match_count;
    u32 m2_binding_count;
    u32 m2_binding_match_count;
    u32 m2_xobj_count;
    u32 m2_xobj_kind_4b_count;
    u32 m2_xobj_kind_2_count;
    u32 m2_xobj_other_count;
    u32 m2_xobj_null_count;
    u32 m2_parts_count;
    u32 m2_parts_matrix_mode0_count;
    u32 m2_parts_matrix_mode1_count;
    u32 m2_parts_matrix_mode3_count;
    u32 m2_parts_matrix_other_count;
    u32 m2_animlock_active;
    u32 m2_camera_fetch_count;
    u32 m2_world_matrix_request_count;
    u32 m2_world_matrix_cache_hit_count;
    u32 m2_local_matrix_build_count;
    u32 m2_world_affine_count;
    u32 m2_world_camera_count;
    u32 m2_final_compose_count;
    u32 m2_root_gx_count;
    u32 m2_lighting_epoch_count;
    u32 m2_run_prepare_count;
    u32 m2_corner_emit_run_count;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 semantic_output_hash2;
    u32 semantic_event_count;
    u32 semantic_overflow_count;
    u32 semantic_occurrence_count;
    u32 semantic_first_owner_occurrence;
    u32 semantic_first_list_ordinal;
    u32 semantic_first_branch_path;
    u32 semantic_first_command_index;
    u32 semantic_first_tri2_half;
    u32 semantic_first_outcome;
#endif
} NDSRendererOwnerProfile;

#if NDS_RENDERER_PROFILE_LEVEL >= 1
extern volatile NDSRendererOwnerProfile
    gNdsRendererProfileOwners[NDS_RENDERER_PROFILE_OWNER_COUNT];
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
extern volatile u32 gNdsRendererSemanticOutputHash;
extern volatile u32 gNdsRendererSemanticOutputHash2;
extern volatile u32 gNdsRendererSemanticEventCount;
extern volatile u32 gNdsRendererSemanticOverflowCount;
extern volatile u32 gNdsRendererSemanticPrefixHash[
    NDS_RENDERER_SEMANTIC_TRACE_CAPACITY];
extern volatile u32 gNdsRendererSemanticPrefixHash2[
    NDS_RENDERER_SEMANTIC_TRACE_CAPACITY];

#define NDS_RENDERER_STAGE_DEPTH_TRACE_CAPACITY 202u
typedef struct NDSRendererStageDepthTrace
{
    u32 owner_occurrence;
    u32 list_ordinal;
    u32 branch_path;
    u32 command_index;
    s32 projected_z[3];
    s16 submitted_z[3];
    u8 submit_class;
    u8 source_zbuffered;
    u8 no_z_phase;
    u8 tri2_half;
} NDSRendererStageDepthTrace;

extern volatile NDSRendererStageDepthTrace gNdsRendererStageDepthTrace[
    NDS_RENDERER_STAGE_DEPTH_TRACE_CAPACITY];
extern volatile u32 gNdsRendererStageDepthTraceCount;
extern volatile u32 gNdsRendererStageDepthTraceOverflowCount;
extern volatile u32 gNdsRendererStageDepthTraceHash;
extern volatile u32 gNdsRendererStageDepthTraceClassCount[8];
extern volatile u32 gNdsRendererStageDepthTraceNoZCollisionCount;
extern volatile u32 gNdsRendererStageDepthTraceBackgroundCount;
extern volatile s32 gNdsRendererStageDepthTraceBackgroundMin;
extern volatile s32 gNdsRendererStageDepthTraceBackgroundMax;
extern volatile u32 gNdsRendererStageDepthTraceForegroundCount;
extern volatile s32 gNdsRendererStageDepthTraceForegroundMin;
extern volatile s32 gNdsRendererStageDepthTraceForegroundMax;
#endif

typedef struct NDSRendererCommand
{
    const Gfx *dl;
    u32 w0;
    u32 w1;
    u32 op;
    u32 depth;
    u32 list_index;
    const Gfx *raw_branch_dl;
    const Gfx *resolved_branch_dl;
    u32 branch_resolve_kind;
    u32 branch_is_jump;
    const NDSRendererClipVertex20p12 *transformed_vertices;
    u32 transformed_vertex_valid_mask;
    u32 matrix_valid;
} NDSRendererCommand;

typedef struct NDSRendererTileState
{
    u32 set_seen;
    u32 size_seen;
    u32 format;
    u32 size;
    u32 line;
    u32 tmem;
    u32 palette;
    u32 cms;
    u32 cmt;
    u32 masks;
    u32 maskt;
    u32 shifts;
    u32 shiftt;
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    u32 width;
    u32 height;
    u32 flags;
} NDSRendererTileState;

typedef struct NDSRendererTextureLoadState
{
    u32 image;
    u32 sequence;
    u16 image_width;
    u16 load_uls;
    u16 load_ult;
    u16 load_lrs;
    u16 load_dxt;
    u16 load_texels;
    u16 load_tmem;
    u8 valid;
    u8 image_format;
    u8 image_size;
    u8 load_kind;
    u8 load_tile;
} NDSRendererTextureLoadState;

typedef s32 (*NDSRendererCommandCallback)(const NDSRendererCommand *command,
                                          void *user);

typedef enum NDSRendererTextureDataLayout
{
    NDS_RENDERER_TEXTURE_DATA_NATIVE = 0,
    NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED = 1
} NDSRendererTextureDataLayout;

typedef struct NDSRendererConfig
{
    u32 max_depth;
    u32 max_commands;
    u32 max_list_commands;
    const NDSRendererMatrix20p12 *initial_projection;
    const NDSRendererMatrix20p12 *initial_modelview;
    u32 initial_geometry_mode;
    NDSRendererTextureDataLayout texture_data_layout;
    NDSRendererValidateRange validate_range;
    NDSRendererImmutableCommandSpan immutable_command_span;
    NDSRendererResolveBranch resolve_branch;
    NDSRendererResolveData resolve_data;
    void *user;
} NDSRendererConfig;

#define NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE (1u << 0)
#define NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT (1u << 1)
#define NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 (1u << 2)
#define NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 (1u << 3)
#define NDS_RENDERER_NATIVE_MATERIAL_PRIM (1u << 4)
#define NDS_RENDERER_NATIVE_MATERIAL_ENV (1u << 5)
#define NDS_RENDERER_NATIVE_MATERIAL_BLEND (1u << 6)
#define NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE (1u << 7)
#define NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK (1u << 8)
#define NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE (1u << 9)
#define NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE (1u << 10)
#define NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE (1u << 11)
#define NDS_RENDERER_NATIVE_MATERIAL_TEXTURE (1u << 12)

/* Typed BattleShip material effect. The adapter derives this directly from
 * the live MObj; the native owner never builds or executes a segment-E Gfx
 * mini display list. */
typedef struct NDSRendererNativeMaterial
{
    u32 effects;
    u16 command_count;
    u16 sync_count;
    u32 palette_image_w0;
    u32 palette_image;
    u32 palette_tile_w0;
    u32 palette_tile_w1;
    u32 palette_tlut_w1;
    u32 light1;
    u32 light2;
    u32 prim_w0;
    u32 prim_w1;
    u32 env_color;
    u32 blend_color;
    u32 block_image_w0;
    u32 block_image;
    u32 load_block_w0;
    u32 load_block_w1;
    u32 current_image_w0;
    u32 current_image;
    u32 render_tile_size_w0;
    u32 render_tile_size_w1;
    u32 scroll_tile_size_w0;
    u32 scroll_tile_size_w1;
    u32 texture_w0;
    u32 texture_w1;
} NDSRendererNativeMaterial;

#define NDS_RENDERER_NATIVE_PREAMBLE_VALID (1u << 0)
#define NDS_RENDERER_NATIVE_PREAMBLE_LIGHT_VALID (1u << 1)

typedef struct NDSRendererNativeFighterPreamble
{
    u32 geometry_mode;
    u32 cycle_type;
    u32 render_mode;
    u32 prim_color;
    u32 env_color;
    s8 light_dir_x;
    s8 light_dir_y;
    s8 light_dir_z;
    u8 flags;
} NDSRendererNativeFighterPreamble;

/* One preflighted root row for the production native fighter owner.  The
 * adapter retains ownership of every pointer for the duration of the call.
 * composed_matrix is the exact CPU-composed matrix used by the generic
 * renderer; modelview_matrix is retained separately for source lighting. */
typedef struct NDSRendererNativeFighterRoot
{
    u32 root_offset;
    u32 material_count;
    const NDSRendererMatrix20p12 *composed_matrix;
    const NDSRendererMatrix20p12 *modelview_matrix;
    const NDSRendererNativeMaterial *materials;
    const NDSRendererConfig *config;
#if NDS_RENDERER_M2_DETAILED_LEDGER
    u32 owner_generation;
#endif
    NDSRendererNativeFighterPreamble preamble;
} NDSRendererNativeFighterRoot;

/* Mode-7 laboratory candidate.  The adapter supplies exact BattleShip local
 * matrices and live topology; the DS backend validates the complete owner
 * before it mutates GX, then owns the hierarchy and direct corner stream. */
typedef struct NDSRendererNativeFighterHierarchy
{
    const NDSRendererMatrix20p12 *projection;
    const NDSRendererMatrix20p12 *camera_modelview;
    const NDSRendererMatrix20p12 *joint_locals;
    const u8 *joint_parents;
    const u8 *joint_bindings;
    const NDSRendererNativeFighterRoot *roots;
    const NDSRendererConfig *config;
    u32 joint_count;
    u32 root_count;
} NDSRendererNativeFighterHierarchy;

#define NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT 4u
#define NDS_RENDERER_NATIVE_STAGE_BINDING_COUNT 42u
#define NDS_RENDERER_NATIVE_STAGE_MATERIAL_COUNT 4u
#define NDS_RENDERER_NATIVE_STAGE_DOBJ_COUNT 57u

typedef struct NDSRendererNativeStageDObj
{
    const void *identity;
    u16 parent_index;
    u16 binding_index;
    u16 transform_flags;
    u8 owner;
    u8 depth;
} NDSRendererNativeStageDObj;

/* Mode-9 Dream Land owner input. Preflight consumes all fields synchronously,
 * but successful execution retains binding_composed through the eight later
 * display-link commits; the adapter workspace must remain live until finish. */
typedef struct NDSRendererNativeStageFrame
{
    const void *asset_bases[NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT];
    const NDSRendererNativeStageDObj *dobjs;
    const void *const *binding_display_lists;
    const NDSRendererMatrix20p12 *projection;
    const NDSRendererMatrix20p12 *camera_modelview;
    const NDSRendererMatrix20p12 *binding_world;
    const NDSRendererMatrix20p12 *binding_composed;
    const NDSRendererNativeMaterial *materials;
    const NDSRendererConfig *config;
    u32 topology_generation;
    u32 topology_stamp;
    u64 rigid_binding_mask;
} NDSRendererNativeStageFrame;

typedef struct NDSRendererStats
{
    u32 blocker;
    u32 command_count;
    u32 vertex_count;
    u32 triangle_count;
    u32 first_opcode;
    u32 unsupported_opcode;
    u32 vertex_command_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 source_vertex_count;
#endif
    u32 triangle_command_count;
    u32 matrix_command_count;
    u32 matrix_load_count;
    u32 matrix_mul_count;
    u32 matrix_projection_count;
    u32 matrix_modelview_count;
    u32 matrix_push_count;
    u32 matrix_pop_count;
    u32 matrix_transform_count;
    u32 matrix_mvp_recalc_count;
    u32 matrix_move_word_count;
    u32 transformed_vertex_count;
    u32 transformed_triangle_count;
    u32 hardware_vertex_count;
    u32 hardware_triangle_count;
    u32 hardware_zbuffer_triangle_count;
    u32 hardware_projected_depth_triangle_count;
    u32 hardware_projected_depth_sample_count;
    s32 hardware_projected_depth_min;
    s32 hardware_projected_depth_max;
    s32 hardware_projected_w_min;
    s32 hardware_projected_w_max;
    u32 hardware_decal_depth_triangle_count;
    u32 hardware_prim_depth_triangle_count;
    u32 hardware_oracle_triangle_count;
    u32 hardware_oracle_reject_count;
    u32 hardware_matrix_seed_count;
    u32 hardware_texture_bind_count;
    u32 hardware_texture_upload_count;
    u32 hardware_texture_ready_count;
    u32 hardware_texture_reject_count;
    u32 hardware_texture_format;
    u32 hardware_texture_width;
    u32 hardware_texture_height;
    u32 first_transformed_tri_v0;
    u32 first_transformed_tri_v1;
    u32 first_transformed_tri_v2;
    u32 matrix_flags;
    s32 first_transformed_x;
    s32 first_transformed_y;
    s32 first_transformed_z;
    s32 first_transformed_w;
    u32 sync_command_count;
    u32 end_command_count;
    u32 branch_command_count;
    u32 branch_call_count;
    u32 branch_jump_count;
    u32 segment_resolve_count;
    u32 othermode_command_count;
    u32 color_command_count;
    u32 light_color_command_count;
    u32 light_direction_command_count;
    u32 light_fallback_count;
    u32 unsupported_command_count;
    u32 state_command_count;
    u32 skip_command_count;
    u32 render_command_count;
    u32 max_depth_seen;
    u32 cull_command_count;
    u32 ignored_state_command_count;
    u32 first_othermode_opcode;
    u32 first_othermode_w0;
    u32 first_othermode_w1;
    u32 othermode_h;
    u32 othermode_l;
    u32 first_cull_w0;
    u32 first_cull_w1;
    const Gfx *first_branch_dl;
    const Gfx *first_resolved_branch_dl;
    u32 geometry_mode;
    u32 geometry_clear_mask;
    u32 geometry_set_mask;
    u32 geometry_command_count;
    u32 texture_mask;
    u32 texture_load_kind;
    u32 texture_command_count;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 texture_level;
    u32 texture_tile;
    u32 texture_on;
    u32 texture_xparam;
    u32 texture_state_flags;
    u32 texture_image;
    u32 texture_format;
    u32 texture_size;
    u32 texture_image_width;
    u32 texture_set_tile_count;
    u32 texture_tlut_image;
    u32 texture_tlut_count;
    u32 texture_tlut_tile;
    u32 texture_render_tile;
    u32 texture_render_tile_format;
    u32 texture_render_tile_size;
    u32 texture_render_tile_line;
    u32 texture_render_tile_tmem;
    u32 texture_render_tile_palette;
    u32 texture_render_tile_cms;
    u32 texture_render_tile_cmt;
    u32 texture_render_tile_masks;
    u32 texture_render_tile_maskt;
    u32 texture_render_tile_shifts;
    u32 texture_render_tile_shiftt;
    u32 texture_render_tile_flags;
    u32 texture_load_tile;
    u32 texture_load_block_uls;
    u32 texture_load_block_ult;
    u32 texture_load_block_lrs;
    u32 texture_load_block_dxt;
    u32 texture_load_texels;
    u32 texture_tile_size_tile;
    u32 texture_tile_size_uls;
    u32 texture_tile_size_ult;
    u32 texture_tile_size_lrs;
    u32 texture_tile_size_lrt;
    u32 texture_tile_width;
    u32 texture_tile_height;
    NDSRendererTileState texture_tiles[NDS_RENDERER_TILE_COUNT];
    u32 texture_load_sequence;
    NDSRendererTextureLoadState
        texture_loads[NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT];
    u32 texture_combine_w0;
    u32 texture_combine_w1;
    u32 texture_combine_count;
    u32 prim_color;
    u32 prim_min_level;
    u32 prim_lod_fraction;
    u32 env_color;
    u32 blend_color;
    u32 light_color_1;
    u32 light_color_2;
    u32 light_color_mask;
    s32 light_dir_x;
    s32 light_dir_y;
    s32 light_dir_z;
    u32 light_dir_mask;
    u32 prim_depth;
    u32 prim_depth_delta;
    u32 prim_depth_command_count;
    u32 fog_color;
    s32 fog_min;
    s32 fog_max;
    u32 fog_status;
    u32 texture_source_hash1;
    u32 texture_source_hash2;
} NDSRendererStats;

s32 ndsRendererMtxCellS16p16(const Mtx *mtx, u32 row, u32 col);
void ndsRendererMtxLoadN64ToDS20p12(const Mtx *src,
                                    NDSRendererMatrix20p12 *dst);
void ndsRendererMtxMul20p12(const NDSRendererMatrix20p12 *lhs,
                            const NDSRendererMatrix20p12 *rhs,
                            NDSRendererMatrix20p12 *out);
void ndsRendererMtxMulAffine20p12(const NDSRendererMatrix20p12 *lhs,
                                  const NDSRendererMatrix20p12 *rhs,
                                  NDSRendererMatrix20p12 *out);
void ndsRendererTransformVertex20p12(const NDSRendererMatrix20p12 *mtx,
                                     const NDSRendererInputVertex *vtx,
                                     NDSRendererClipVertex20p12 *out);
void ndsRendererInitStats(NDSRendererStats *stats);
void ndsRendererInitVertexCache(NDSRendererVertexCache *vertex_cache);
void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats);
void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats);
void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
s32 ndsRendererExecuteNativeFighterRoot(
    u32 slot,
    u32 root_ordinal,
    const void *asset_base,
    u32 root_offset,
    const NDSRendererNativeMaterial *materials,
    u32 material_count,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
s32 ndsRendererExecuteNativeFighterOwnerProduction(
    u32 slot,
    const void *asset_base,
    const NDSRendererNativeFighterRoot *roots,
    u32 root_count,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started);
s32 ndsRendererExecuteNativeFighterOwnerHierarchy(
    u32 slot,
    const void *asset_base,
    const NDSRendererNativeFighterHierarchy *hierarchy,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    u32 *out_hardware_started);
s32 ndsRendererPrepareNativeStageOwner(
    const NDSRendererNativeStageFrame *frame,
    NDSRendererStats *stats);
s32 ndsRendererCommitNativeStageSegment(u32 segment_index);
void ndsRendererFinishNativeStageOwner(void);
void ndsRendererResetNativeStageValidationCache(void);
s32 ndsRendererBeginNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
s32 ndsRendererEndNativeFighterOwner(
    u32 slot,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache);
void ndsRendererAbortNativeFighterOwner(void);
s32 ndsRendererValidateNativeFighterOwner(
    u32 slot,
    u32 asset_data_size,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts);
#if (NDS_RENDERER_PROFILE_LEVEL == 1) && \
    NDS_RENDERER_M2_DETAILED_LEDGER && NDS_RENDERER_HW_TRIANGLES
void ndsRendererProfileCensusNativeFighterSchedule(
    u32 slot,
    const u8 *joint_parents,
    const u8 *joint_bindings,
    u32 joint_count,
    u32 binding_count,
    u32 *schedule_match_count,
    u32 *binding_match_count);
#endif
void ndsRendererHardwareResetSourceCaches(void);
void ndsRendererHardwareDiscardTextureCache(void);
s32 ndsRendererHardwarePrepareBattleStaticTextures(void);
void ndsRendererHardwareArmBattleStaticTextures(void);
void ndsRendererHardwareDiscardBattleStaticTextures(void);
void ndsRendererHardwareAbortBattleStaticTextures(void);
s32 ndsRendererHardwareUploadSceneMipCache(const u16 *mip0,
                                             const u16 *mip1,
                                             const u16 *mip2);
s32 ndsRendererHardwareDrawSceneMipCache(u32 mip_index,
                                           const s32 *tex_s_q4,
                                           const s32 *tex_t_q4,
                                           u32 columns,
                                           u32 rows);
typedef s32 (*NDSRendererTextureFillCallback)(u8 *pixels, u32 bytes,
                                               void *user_data);
s32 ndsRendererHardwarePrepareIFCommonCloudAtlas(
    u32 width, u32 height, const u16 palette[8],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name);
void ndsRendererHardwareReleaseIFCommonCloudAtlas(u32 *texture_name);
s32 ndsRendererHardwareDrawIFCommonCloudAtlas(
    u32 texture_name, s32 x_q16, s32 y_q16,
    s32 width_q16, s32 height_q16,
    u32 texture_x, u32 texture_y, u32 texture_width,
    u32 texture_height, u32 poly_id);
void ndsRendererHardwareSetNoOracle(u32 enabled);
u32 ndsRendererHardwareNoOracleEnabled(void);
u32 ndsRendererHardwareConsumeSubmittedFrame(void);
u32 ndsRendererHardwareCommitPendingTextureRefreshes(void);
void ndsRendererProfileSetOwner(NDSRendererProfileOwner owner);
void ndsRendererProfileSetSourceProvenance(u32 owner_occurrence,
                                           u32 list_ordinal,
                                           u32 root_branch_path);
void ndsRendererProfileRecordFrameBoundaryGXState(u32 status, u32 control);
void ndsRendererProfileRecordMaterialOperations(u32 count);
u32 ndsRendererProfileGlobalStateHash(void);
#if NDS_RENDER_ECONOMY
void ndsRendererProfileFrameBegin(u32 render_economy_allowed);
#else
void ndsRendererProfileFrameBegin(void);
#endif
void ndsRendererProfileFramePublish(void);
extern volatile u32 gNdsRendererFastRunMode;
extern volatile u32 gNdsRendererFastRunCount;
extern volatile u32 gNdsRendererFastTriangleCount;
extern volatile u32 gNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
extern volatile u32 gNdsRendererFastFallbackCount[3];
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
extern volatile NDSRendererScreenSpaceCensusRow
    gNdsRendererScreenSpaceCensus[NDS_RENDERER_PROFILE_OWNER_COUNT]
                                   [NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT];
extern volatile u64 gNdsRendererScreenSpaceStageOwnerTicks[
    NDS_RENDERER_SCREEN_SPACE_CENSUS_STAGE_OWNER_COUNT];
extern volatile u32 gNdsRendererScreenSpaceCensusArmed;
extern volatile u32 gNdsRendererScreenSpaceCensusResetRequested;
extern volatile u32 gNdsRendererScreenSpaceCensusFrameCount;
extern volatile u32 gNdsRendererScreenSpaceCensusOverflowCount;
#endif
#if NDS_RENDER_ECONOMY
extern volatile u32 gNdsRendererEconomyConfiguredOwnerMask;
extern volatile u32 gNdsRendererEconomyActiveOwnerMask;
extern volatile u32 gNdsRendererEconomyAppliedOwnerMask;
extern volatile u32 gNdsRendererEconomySkippedRunCount;
extern volatile u32 gNdsRendererEconomySkippedTriangleCount;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
extern volatile u32 gNdsRendererM3PreflightAttemptCount;
extern volatile u32 gNdsRendererM3PreflightSuccessCount;
extern volatile u32 gNdsRendererM3PreflightFallbackCount;
extern volatile u32 gNdsRendererM3SegmentCount;
extern volatile u32 gNdsRendererM3SegmentMask;
extern volatile u32 gNdsRendererM3PostArmFailureCount;
extern volatile u32 gNdsRendererM3DObjCount;
extern volatile u32 gNdsRendererM3BindingCount;
extern volatile u32 gNdsRendererM3RunCount;
extern volatile u32 gNdsRendererM3TriangleCount;
extern volatile u32 gNdsRendererM3ResidentEpochCount;
extern volatile u32 gNdsRendererM3MaterialShadowCount;
extern volatile u32 gNdsRendererM3MaterialCommitCount;
extern volatile u32 gNdsRendererM3CrossRunCount;
extern volatile u32 gNdsRendererM3CrossTriangleCount;
extern volatile u32 gNdsRendererM3CrossForeignCornerCount;
extern volatile u32 gNdsRendererM3TopologyFullValidationCount;
extern volatile u32 gNdsRendererM3TopologyCacheHitCount;
extern volatile u32 gNdsRendererM3TopologyStampMismatchCount;
#if NDS_TASK36_HW_COMPOSE
extern volatile u32 gNdsRendererTask36HardwareComposedDObjCount;
extern volatile u32 gNdsRendererTask36CameraLoadCount;
extern volatile u32 gNdsRendererTask36WorldMultCount;
extern volatile u32 gNdsRendererTask36AdapterRejectReason;
extern volatile u32 gNdsRendererTask36RendererRejectReason;
extern volatile u32 gNdsRendererTask36PrepareRunRejectReason;
extern volatile u32 gNdsRendererTask36RigidConstancyMismatchCount;
extern volatile u32 gNdsRendererTask36ObservedDynamicMaskLo;
extern volatile u32 gNdsRendererTask36ObservedDynamicMaskHi;
#endif
#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE
extern volatile u32 gNdsRendererM3GeneratedSegment0AttemptCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0SuccessCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0PreGxFallbackCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0RunCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0TriangleCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0EpochCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0MaterialCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0CertificateValidationCount;
#if NDS_RENDERER_M3_PHASE0_PROFILE
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowDenseCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowStateEntryCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowSyncCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowFieldComparisonCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowMismatchCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowFaultInjectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowFaultRejectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultInjectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultRejectedCount;
extern volatile u32 gNdsRendererM3GeneratedSegment0ShadowLiveFaultRevalidatedCount;
#endif
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
extern volatile u32 gNdsRendererM3TopologyFaultInjectionCount;
extern volatile u32 gNdsRendererM3TopologyFaultRevalidationCount;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
extern volatile u32 gNdsRendererM3Phase0PreflightTicks;
extern volatile u32 gNdsRendererM3Phase0PrepareRunTicks;
extern volatile u32 gNdsRendererM3Phase0VertexPrepareTicks;
extern volatile u32 gNdsRendererM3Phase0NearTransformTicks;
extern volatile u32 gNdsRendererM3Phase0RunTransitionTicks;
extern volatile u32 gNdsRendererM3Phase0RawEmitTicks;
extern volatile u32 gNdsRendererM3Phase0RangeEmitTicks;
extern volatile u32 gNdsRendererM3Phase0NoZEmitTicks;
extern volatile u32 gNdsRendererM3Phase0NoZMatrixTicks;
extern volatile u32 gNdsRendererM3Phase0AccountingTicks;
extern volatile u32 gNdsRendererM3Phase0CommitTicks;
extern volatile u32 gNdsRendererM3Phase0TimerReadCount;
extern volatile u32 gNdsRendererM3Phase0TimerSpanCount;
extern volatile u32 gNdsRendererM3Phase0CalibrationTicks;
extern volatile u32 gNdsRendererM3Phase0CalibrationIntervals;
extern volatile u32 gNdsRendererM3Phase0PreparedDenseCount;
extern volatile u32 gNdsRendererM3Phase0NearTransformCount;
extern volatile u32 gNdsRendererM3Phase0NoZMatrixCount;
extern volatile u32 gNdsRendererM3ResidualPrepareTicks;
extern volatile u32 gNdsRendererM3ResidualVertexTicks;
extern volatile u32 gNdsRendererM3ResidualNearTicks;
extern volatile u32 gNdsRendererM3ResidualKeyTicks;
extern volatile u32 gNdsRendererM3ResidualKeyHitCount;
extern volatile u32 gNdsRendererM3ResidualKeyMissCount;
extern volatile u32 gNdsRendererM3ResidualKeyByteCount;
extern volatile u32 gNdsRendererM3ResidualRunCount;
extern volatile u32 gNdsRendererM3ResidualDenseCount;
extern volatile u32 gNdsRendererM3ResidualNearCount;
extern volatile u32 gNdsRendererM3G2TextureParamWriteCount;
extern volatile u32 gNdsRendererM3G2TextureParamSkipCount;
extern volatile u32 gNdsRendererM3G2MatrixModeWriteCount;
extern volatile u32 gNdsRendererM3G2MatrixModeSkipCount;
extern volatile u32 gNdsRendererM3G2PolyFmtWriteCount;
extern volatile u32 gNdsRendererM3G2PolyFmtSkipCount;
extern volatile u32 gNdsRendererPhase05WallpaperSetupTicks;
extern volatile u32 gNdsRendererPhase05WallpaperXMapTicks;
extern volatile u32 gNdsRendererPhase05WallpaperYMapTicks;
extern volatile u32 gNdsRendererPhase05WallpaperWriteTicks;
extern volatile u32 gNdsRendererPhase05WallpaperCommitTicks;
extern volatile u32 gNdsRendererPhase05PresentHardwareTicks;
extern volatile u32 gNdsRendererPhase05GCDrawAllTicks;
extern volatile u32 gNdsRendererPhase05StageTransitionTicks;
extern volatile u32 gNdsRendererPhase05FighterWrapperTicks;
extern volatile u32 gNdsRendererPhase05FrameResetTicks;
extern volatile u32 gNdsRendererPhase05PresentTailTicks;
extern volatile u32 gNdsRendererPhase05ProfileBookkeepingTicks;
extern volatile u32 gNdsRendererPhase05ProfilePublishTicks;
extern volatile u32 gNdsRendererPhase05FlushPrepTicks;
extern volatile u32 gNdsRendererPhase05TimerReadCount;
extern volatile u32 gNdsRendererPhase05TimerSpanCount;
extern volatile u32 gNdsRendererPhase05CalibrationTicks;
extern volatile u32 gNdsRendererPhase05CalibrationIntervals;
extern volatile u32 gNdsRendererPhase05WallpaperRowCount;
extern volatile u32 gNdsRendererPhase05WallpaperPixelWriteCount;
extern volatile u32 gNdsRendererPhase05WallpaperFullRowCount;
extern volatile u32 gNdsRendererPhase05WallpaperIncrementalRowCount;
extern volatile u32 gNdsRendererPhase05WallpaperChangedXCount;
extern volatile u32 gNdsRendererPhase05WallpaperChangedRunCount;
extern volatile u32 gNdsRendererPhase05WallpaperLongestChangedRun;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE2Count;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE2Pixels;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE4Count;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE4Pixels;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE8Count;
extern volatile u32 gNdsRendererPhase05WallpaperRunGE8Pixels;
extern volatile u32 gNdsRendererPhase05WallpaperScalarStoreCount;
extern volatile u32 gNdsRendererPhase05WallpaperPackedStoreCount;
extern volatile u32 gNdsRendererPhase05WallpaperDmaPixelCount;
extern volatile u32 gNdsRendererPhase05WallpaperCopyPixelCount;
#endif
#endif
extern volatile u32 gNdsRendererBattleStaticTextureEnabled;
extern volatile u32 gNdsRendererBattleStaticTexturePrepareCount;
extern volatile u32 gNdsRendererBattleStaticTexturePrepareFailCount;
extern volatile u32 gNdsRendererBattleStaticTexturePreparedCount;
extern volatile u32 gNdsRendererBattleStaticTexturePreparedBytes;
extern volatile u32 gNdsRendererBattleStaticTextureArmCount;
extern volatile u32 gNdsRendererBattleStaticTexturePinnedHitCount;
extern volatile u32 gNdsRendererBattleStaticTextureSeenMask;
extern volatile u32 gNdsRendererBattleStaticTextureOwnerMask;
extern volatile u32 gNdsRendererBattleStaticTextureViolationCount;
extern volatile u32 gNdsRendererBattleStaticTextureTeardownCount;
extern volatile u32 gNdsRendererBattleStaticTextureFirstAddress;
extern volatile u32 gNdsRendererBattleStaticTextureEndAddress;
extern volatile u32 gNdsRendererBattleStaticTextureAllocationSpanBytes;
extern volatile u32 gNdsRendererBattleStaticTextureBankMask;
typedef enum NDSRendererBattleTextureFenceClass
{
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_CONVERT = 0,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_PALETTE_DECODE,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_ALLOC,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_FILE_IO,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_CREATE,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_UPLOAD,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_GL_DELETE,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_EVICT,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_MANIFEST_FALLBACK,
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT
} NDSRendererBattleTextureFenceClass;
extern volatile u32 gNdsRendererBattleTextureFenceCounts[
    NDS_RENDERER_BATTLE_TEXTURE_FENCE_COUNT];
extern volatile u32 gNdsRendererBattleTextureFenceFirstClassPlus1;
extern volatile u32 gNdsRendererBattleTextureFenceFirstFrame;
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
extern volatile u32 gNdsRendererBenchmarkSinkHashA;
extern volatile u32 gNdsRendererBenchmarkSinkHashB;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkWords;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkHashA;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkHashB;
extern volatile u32 gNdsRendererBenchmarkSegment0SinkArmFaults;
extern u32 gNdsRendererBenchmarkSegment0Trace[3072];
extern volatile u32 gNdsRendererBenchmarkSegment0RunWords[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunHashA[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunHashB[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunRawTextureName[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureEpochPlus1[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureImage[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureTlut[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureKeyHashA[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureKeyHashB[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureDescriptor[26];
extern volatile u32 gNdsRendererBenchmarkSegment0RunTextureParams[26];
void ndsRendererBenchmarkSinkEndOwner(NDSRendererProfileOwner owner);
#endif
#endif
