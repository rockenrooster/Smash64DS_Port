typedef struct NDSRendererTraversalVertexStorage
{
    NDSRendererClipVertex20p12 vertices[NDS_RENDERER_MAX_VTX];
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererInputVertex input_vertices[NDS_RENDERER_MAX_VTX];
    u32 vertex_colors[NDS_RENDERER_MAX_VTX];
    u8 vertex_matrix_snapshot[NDS_RENDERER_MAX_VTX];
    u8 vertex_clip_snapshot[NDS_RENDERER_MAX_VTX];
#endif
} NDSRendererTraversalVertexStorage;

typedef struct NDSRendererTraversalState
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 matrix;
    Mtx matrix_word_raw;
    NDSRendererMatrix20p12 modelview_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    NDSRendererClipVertex20p12 *vertices;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererInputVertex *input_vertices;
    u32 *vertex_colors;
    NDSRendererMatrixSnapshot *matrix_snapshots;
    u8 *vertex_matrix_snapshot;
    u8 *vertex_clip_snapshot;
    const Gfx *source_command_site;
#endif
    u32 modelview_valid_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    u32 modelview_stack_depth;
    u32 vertex_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
    u32 input_vertex_valid_mask;
    u32 vertex_color_valid_mask;
    u32 current_transform_vertex_mask;
    u32 matrix_generation;
    u32 matrix_snapshot_count;
    u32 current_matrix_snapshot;
    u32 color_modulate;
    NDSRendererHardwareLightDirection prepared_light_direction;
    u32 prepared_light_direction_valid;
    u32 texture_prepare_valid;
    u32 texture_prepare_enabled;
    u32 texture_prepare_name;
    u32 texture_prepare_material_color;
    u32 texture_prepare_scale_s;
    u32 texture_prepare_scale_t;
    u32 texture_prepare_origin_s;
    u32 texture_prepare_origin_t;
    s32 texture_prepare_offset;
    u32 texture_prepare_vertex_flags;
    u32 texture_prepare_source_zbuffered;
    u32 texture_prepare_decal_depth;
    u32 texture_prepare_prim_depth;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 texture_prepare_key_hash;
    u32 texture_prepare_params;
    u32 semantic_branch_path;
    u32 semantic_command_index;
    u32 semantic_tri2_half;
#endif
    u32 texture_prepare_alpha_constant;
    u32 texture_prepare_poly_alpha;
    u32 texture_prepare_poly_fmt;
    u16 prepared_vertex_colors[NDS_RENDERER_MAX_VTX];
    s16 prepared_texcoord_s[NDS_RENDERER_MAX_VTX];
    s16 prepared_texcoord_t[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_x[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_y[NDS_RENDERER_MAX_VTX];
    v16 prepared_projected_source_z[NDS_RENDERER_MAX_VTX];
    u32 prepared_vertex_color_valid_mask;
    u32 prepared_texcoord_valid_mask;
    u32 prepared_projected_xy_valid_mask;
    u32 prepared_projected_source_z_valid_mask;
#endif
    u32 projection_valid;
    u32 modelview_valid;
    u32 matrix_valid;
    u32 matrix_word_valid;
#if NDS_RENDERER_HW_TRIANGLES
    u32 raw_vertex_fit_mask;
#endif
} NDSRendererTraversalState;

#if NDS_RENDERER_HW_TRIANGLES
#define NDS_NATIVE_STATE_OTHERMODE 2u
#define NDS_NATIVE_STATE_COMBINE 3u
#define NDS_NATIVE_STATE_TEXTURE 4u
#define NDS_NATIVE_STATE_GEOMETRY 5u
#define NDS_NATIVE_STATE_IMAGE 6u
#define NDS_NATIVE_STATE_TILE 7u
#define NDS_NATIVE_STATE_LOAD_TLUT 8u
#define NDS_NATIVE_STATE_LOAD_BLOCK 9u
#define NDS_NATIVE_STATE_TILE_SIZE 10u
#define NDS_NATIVE_STATE_PRIM 11u
#define NDS_NATIVE_STATE_BLEND 12u
#define NDS_NATIVE_STATE_MATERIAL 13u
#define NDS_NATIVE_STATE_LIGHT_COLOR 14u
#define NDS_NATIVE_STATE_NONE 0xffffu
#define NDS_NATIVE_MATERIAL_NONE 0xffu
#define NDS_NATIVE_VERTEX_BLOCK 0u
#define NDS_NATIVE_MODIFY_ST 1u
#define NDS_NATIVE_RUN_RAW_CURRENT 0u
#define NDS_NATIVE_RUN_CROSS_MATRIX 1u
#define NDS_NATIVE_DENSE_VERTEX_COUNT 541u
#define NDS_NATIVE_ROOT_BINDING_COUNT 32u
#define NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK 0x03u
#define NDS_NATIVE_DIRECT_POLICY_TEXTURED_LIT 0u
#define NDS_NATIVE_DIRECT_POLICY_LIT_PRIM 1u
#define NDS_NATIVE_DIRECT_POLICY_LIT_ONLY 2u
#define NDS_NATIVE_DIRECT_POLICY_LIT_PRIM_ALT_ALPHA 3u
#define NDS_NATIVE_DIRECT_POLICY_CULL_NONE 0x80u
#define NDS_NATIVE_DENSE_ID_MASK 0x03ffu
#define NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT 10u
#define NDS_NATIVE_PACKED_CORNER_MATRIX_SHIFT 10u
#define NDS_NATIVE_GX_MATRIX_CURRENT 31u
#define NDS_NATIVE_GX_MATRIX_SLOT_MAX 30u
#define NDS_NATIVE_SOURCE_GEOM_CULL_FRONT 0x00001000u
#define NDS_NATIVE_SOURCE_GEOM_CULL_BACK 0x00002000u
/* P2-3r4: the ungated table ELEMENT types moved to a shared header so the
 * generated owner images and this renderer cannot disagree about their
 * layout. `NDSNativePreparedDenseVertex` stays below: it is build-gated
 * draw scratch, never image content. */
#include <nds/nds_native_fighter_tables.h>
#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_MMARIO
/* The arena the image buffers come from; the renderer does not otherwise
 * allocate, so the declaration arrives with the feature that needs it. */
extern void *syTaskmanMalloc(size_t size, u32 align);
/* The image types AND the owner-slot ids: one generated ABI, so the
 * renderer and the fighter manager cannot number owners differently. */
#include <nds/generated/nds_native_fighter_image.generated.h>
#endif


#if NDS_RENDERER_PROFILE_LEVEL < 2
/* R2-03 E29. Under NDS_R2_FIGHTER_HW_LIGHT the geometry engine lights the
 * fighter: all four emit paths write GFX_NORMAL from
 * sNdsNativeFighterDenseNormals and never read packed_color, and every epoch is
 * lit (gNdsR2ShadeLitEpochs == gNdsR2ExecEpochCalls == 22,296 over 480 frames),
 * so the per-dense-vertex loop that writes these two fields never runs. They are
 * dead weight in the middle of the struct the hot emit loop reads, and this
 * table is randomly indexed by 1,878 corners a frame against a 4 KB dcache --
 * 16 bytes x 541 is 8,656, more than twice the cache. Dropping them takes the
 * struct to 12 bytes and the table to 6,492. */
typedef struct NDSNativePreparedDenseVertex
{
    u32 gx_xy;
#if !NDS_R2_FIGHTER_HW_LIGHT
    u32 shaded_rgba;
#endif
    u16 gx_z;
#if !NDS_R2_FIGHTER_HW_LIGHT
    u16 packed_color;
#endif
    s16 s;
    s16 t;
} NDSNativePreparedDenseVertex;
#endif


typedef struct NDSNativeRoot
{
    u32 root_offset;
    u16 first_epoch;
    u16 tail_state_first;
    u16 source_command_count;
    u8 epoch_count;
    u8 tail_state_count;
    u8 tail_sync_count;
    u8 light_preamble;
} NDSNativeRoot;

/* Passive fighter model parts replace one live DObj display list while
 * keeping that joint's matrix binding.  Generated variants therefore carry
 * the logical canonical binding next to a complete native root descriptor;
 * their executable epochs/runs/vertices live in the same owner table image. */
typedef struct NDSNativeRootVariant
{
    u32 binding;
    NDSNativeRoot root;
} NDSNativeRootVariant;

typedef struct NDSNativeDirectPolicy
{
    u32 combine_w0;
    u32 combine_w1;
    u8 vertex_flags;
    u8 textured;
    u8 reserved[2];
} NDSNativeDirectPolicy;

_Static_assert(sizeof(NDSNativeStateDelta) == 12u,
               "native state effect ABI must stay compact");
_Static_assert(sizeof(NDSNativeDenseVertex) == 12u,
               "native dense vertex ABI must stay cache-line friendly");
#if NDS_RENDERER_PROFILE_LEVEL < 2
#if NDS_R2_FIGHTER_HW_LIGHT
/* R2-03 E29. 16 bytes bought exactly two per 32-byte cache line and no
 * straddling; 12 bytes buys a smaller table (6,492 against 8,656, both over the
 * 4 KB dcache) at the cost of one access in three spanning two lines. Which wins
 * is measured, not assumed -- see the E29 write-up. */
_Static_assert(sizeof(NDSNativePreparedDenseVertex) == 12u,
               "native prepared dense vertex ABI must stay compact");
#else
_Static_assert(sizeof(NDSNativePreparedDenseVertex) == 16u,
               "native prepared dense vertex ABI must stay power-of-two");
#endif
#endif
_Static_assert(sizeof(NDSNativeRun) == 8u,
               "native run ABI must stay compact");
_Static_assert(sizeof(NDSNativeEpoch) == 16u,
               "native epoch ABI must stay compact");
_Static_assert(sizeof(NDSNativeRoot) == 16u,
               "native root ABI must stay compact");
_Static_assert(sizeof(NDSNativeRootVariant) == 20u,
               "native root variant ABI must stay compact");
_Static_assert(sizeof(NDSNativeDirectPolicy) == 12u,
               "native direct policy ABI must stay compact");
#include "nds_native_fighter_owner.generated.inc"
#include "nds_native_stage_owner.generated.inc"
#if defined(NDS_P2_STAGE_YOSTER) && (NDS_P2_STAGE_YOSTER == 1)
#include "nds_native_stage_yoster.generated.inc"
#endif
#if defined(NDS_P2_STAGE_JUNGLE) && (NDS_P2_STAGE_JUNGLE == 1)
#include "nds_native_stage_jungle.generated.inc"
#endif
#if defined(NDS_P2_STAGE_CASTLE) && (NDS_P2_STAGE_CASTLE == 1)
#include "nds_native_stage_castle.generated.inc"
#endif
#if defined(NDS_P2_STAGE_SECTOR) && (NDS_P2_STAGE_SECTOR == 1)
#include "nds_native_stage_sector.generated.inc"
#endif
#if defined(NDS_P2_STAGE_HYRULE) && (NDS_P2_STAGE_HYRULE == 1)
#include "nds_native_stage_hyrule.generated.inc"
#endif
#if defined(NDS_P2_STAGE_INISHIE) && (NDS_P2_STAGE_INISHIE == 1)
#include "nds_native_stage_inishie.generated.inc"
#endif
#if defined(NDS_P2_STAGE_ZEBES) && (NDS_P2_STAGE_ZEBES == 1)
#include "nds_native_stage_zebes.generated.inc"
#endif
#if defined(NDS_P2_STAGE_YAMABUKI) && (NDS_P2_STAGE_YAMABUKI == 1)
#include "nds_native_stage_yamabuki.generated.inc"
#endif
#if defined(NDS_P2_STAGE_PUPUPUSMALL) && (NDS_P2_STAGE_PUPUPUSMALL == 1)
#include "nds_native_stage_pupupusmall.generated.inc"
#endif
#if defined(NDS_P2_STAGE_YOSTERSMALL) && (NDS_P2_STAGE_YOSTERSMALL == 1)
#include "nds_native_stage_yostersmall.generated.inc"
#endif
#if defined(NDS_P2_STAGE_METAL) && (NDS_P2_STAGE_METAL == 1)
#include "nds_native_stage_metal.generated.inc"
#endif
#if defined(NDS_P2_STAGE_ZAKO) && (NDS_P2_STAGE_ZAKO == 1)
#include "nds_native_stage_zako.generated.inc"
#endif
#if defined(NDS_P2_STAGE_LAST) && (NDS_P2_STAGE_LAST == 1)
#include "nds_native_stage_last.generated.inc"
#endif
/* Must follow every generated packet: it names their tables. */
#include "nds_native_stage_select.inc"
#include "dreamland_ds_mesh.generated.inc"

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* P2-2: BattleShip switches every VS fighter to the Low JointTree when three
 * or more fighters are present (scvsbattle.c:188/:460).  The generated owner
 * therefore has two immutable programs with the same ABI but different roots
 * and cardinalities.  Keep one file-scope view selected once per owner execute;
 * draw callbacks are serialized, so the hot inner loops can consume the view
 * without carrying a detail argument through every call.
 *
 * This is deliberately a view over generated constants, not a second runtime
 * representation.  High-detail remains the default for the legacy/root and
 * hierarchy entry points; mode 9 selects the view from FTStruct::detail_curr. */
typedef struct NDSNativeFighterRuntimeTables
{
    const NDSNativeStateDelta *state_deltas;
    u32 state_delta_count;
    const u8 *state_sequence;
    u32 state_sequence_count;
    const NDSNativeVertexAction *vertex_actions;
    u32 vertex_action_count;
    const u8 *epoch_direct_policy;
    const NDSNativeDenseVertex *dense_vertices;
    u32 dense_count;
    /* P2-3f49: precomputed GFX_NORMAL words, one per dense vertex. Imaged
     * owners bind this into the loaded NitroFS image (resident before first
     * draw via EnsureOwnerImage); Mario/Fox keep their baked static arrays. */
    const u32 *dense_normals;
    NDSNativePreparedDenseVertex *prepared_dense;
    const u16 *action_dense_spans;
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    const u16 *dense_color_source;
#endif
    const u16 *packed_corners;
    u32 packed_corner_count;
    const u16 *run_first_corner;
    u32 run_first_corner_count;
    const u16 *run_first_unique;
    const u8 *run_unique_count;
    const u16 *run_unique_dense;
    const u16 *triangles;
    u32 triangle_count;
    const NDSNativeRun *runs;
    u32 run_count;
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    const u16 *primitive_group_first;
    const u8 *primitive_group_count;
    const u8 *primitive_group_type;
    const u16 *primitive_group_first_vertex;
    const u8 *primitive_group_vertex_count;
    const u16 *primitive_vertices;
#endif
    const NDSNativeEpoch *epochs;
    u32 epoch_count;
} NDSNativeFighterRuntimeTables;

/* Mario's pipe and Fox's Arwing are short-lived source DObj trees, but their
 * mesh/display-list/texture payloads are immutable.  Keep BattleShip's live
 * DObj animation (that is what makes Mario rise from the pipe and Fox leave the
 * ship) and replace only the expensive presentation half with an AOT packet.
 * The generated vertices already contain final DS t16 texcoords and every
 * texture is already PAL16/A5I3/RGBA, so this owner never decodes an N64 display list
 * or converts an N64 texture at runtime.
 *
 * geometry_mode/geometry_clear are the bits the list set and cleared; a bit
 * in neither is inherited from the battle display's state at submit. Both
 * models inherit G_LIGHTING that way, so a SHADE-combined group's vertex
 * r,g,b are unit normals. light_color_1/2 (valid per light_mask bit 0/1)
 * are the lists' own gSPLightColor words; the direction is the battle's. */
typedef struct NDSEntryEffectGroup
{
    u16 first_vertex;
    u16 triangle_count;
    /* Sparse RSP load-time-matrix provenance.  Almost every corner was loaded
     * under this group's root; only Mario's pipe body reuses earlier cache
     * slots.  These two fields occupy padding that the old 56-byte row already
     * paid, so source-correct matrix lifetime no longer needs one byte/corner. */
    u16 matrix_override_first;
    u8 texture_slot;
    u8 root_index;
    u8 geometry_state;
    u8 combine_state;
    u8 othermode_state;
    u8 prim_color_index;
    u8 env_color_index;
    u8 light_state;
    u8 cms;
    u8 cmt;
    u8 masks;
    u8 maskt;
    u8 matrix_override_count;
    /* Source segment-0xE material branch selected immediately before this
     * geometry group. 0xFF means the immutable list had no dynamic MObj
     * branch. Link's grounded Spin weapon is the first owner to use this: its
     * LinkModel root selects nine live PRIM-only MObjs by 8-byte branch slot. */
    u8 material_slot;
} NDSEntryEffectGroup;

typedef struct NDSEntryEffectPosition
{
    s16 x;
    s16 y;
    s16 z;
} NDSEntryEffectPosition;

typedef struct NDSEntryEffectColor
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} NDSEntryEffectColor;

typedef struct NDSEntryEffectPairState
{
    u32 a;
    u32 b;
} NDSEntryEffectPairState;

typedef struct NDSEntryEffectRoot
{
    u32 source_offset;
    u16 first_group;
    u8 group_count;
    u8 reserved;
} NDSEntryEffectRoot;

typedef struct NDSEntryEffectTexture
{
    const u8 *texels;
    /* Stored byte count. For LZ10 this is the encoded count; decoded size
     * is derived exactly from width/height/ds_format by the setup-time fill. */
    u32 texel_bytes;
    const u16 *palette;
    u16 palette_entries;
    u16 width;
    u16 height;
    u8 ds_format;
    /* DS PAL16 has no per-palette-entry alpha.  Index 0 becomes transparent
     * only when TEXIMAGE_PARAM.COLOR0 is enabled, so this is part of the
     * converted asset, not a property of the format.  The generator derives it
     * from the canonical RGBA5551 image: transparent sources are normalized to
     * palette[0] == 0, while opaque images may use index 0 as a real colour. */
    u8 color0_transparent;
    u8 compression;
} NDSEntryEffectTexture;

#define NDS_ENTRY_EFFECT_TEXTURE_PAL16 0u
#define NDS_ENTRY_EFFECT_TEXTURE_A5I3 1u
#define NDS_ENTRY_EFFECT_TEXTURE_RGBA 2u
#define NDS_ENTRY_EFFECT_TEXTURE_NONE 0xffu
#define NDS_ENTRY_EFFECT_COMPRESSION_RAW 0u
#define NDS_ENTRY_EFFECT_COMPRESSION_LZ10 1u

#include "nds_entry_effects.generated.inc"

/* The source RSP vertex cache is transformed at gSPVertex load time, not at
 * triangle emission time. Mario's pipe body reuses six cache slots loaded by
 * the preceding rim DObj and changes only their ST with gSPModifyVertex; 18
 * emitted body corners therefore belong to the RIM matrix while the other
 * corners belong to the BODY matrix. The AOT generator records that load-root
 * per corner. Keep each root's live modelview/composed matrix so mixed-cache
 * groups can use the generic renderer's CPU-projected cross-matrix contract. */
_Static_assert(NDS_ENTRY_EFFECT_CROSS_MATRIX_CORNER_COUNT != 0u,
               "entry-effect matrix provenance unexpectedly became trivial");

static u32 sNdsRendererEntryEffectTextureName[NDS_ENTRY_EFFECT_TEXTURE_COUNT];
static NDSRendererMatrix20p12
    sNdsRendererEntryEffectModelview[NDS_ENTRY_EFFECT_ROOT_COUNT];
static NDSRendererMatrix20p12
    sNdsRendererEntryEffectComposed[NDS_ENTRY_EFFECT_ROOT_COUNT];
static u32 sNdsRendererEntryEffectModelviewValidMask;
volatile u32 gNdsEntryEffectNativeDrawCount;
volatile u32 gNdsEntryEffectNativeFallbackCount;
volatile u32 gNdsEntryEffectNativeTexturePrepareCount;
volatile u32 gNdsEntryEffectNativeTextureBindCount;
/* P2-3 (owner: "the Mario intro green tube still doesn't render the full pipe,
 * I just see the rim"). The aggregate draw count cannot tell a rim from a
 * body: Mario's pipe is TWO roots -- 0x03c0 is the 12-triangle ring and
 * 0x04c0 the 32-triangle, 120-tall barrel -- and Fox's Arwing is eight more.
 * Count per root so a missing piece names itself. */
volatile u32 gNdsEntryEffectNativeRootDraws[NDS_ENTRY_EFFECT_ROOT_COUNT];

typedef struct NDSNativeFighterOwnerRuntime
{
    const NDSNativeFighterRuntimeTables *tables;
    const NDSNativeRoot *roots;
    u32 root_count;
    const u8 *cross_palette_slots;
    const u32 (*root_light_preambles)[2];
    u32 root_light_preamble_count;
    u32 asset_data_size;
} NDSNativeFighterOwnerRuntime;

#define NDS_NATIVE_FIGHTER_OWNER_COUNT \
    NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT

#define NDS_FTR_COUNT(a) ((u32)(sizeof(a) / sizeof((a)[0])))

static const NDSNativeFighterRuntimeTables sNdsNativeFighterHighTables =
{
    sNdsNativeFighterStateDeltas, NDS_FTR_COUNT(sNdsNativeFighterStateDeltas),
    sNdsNativeFighterStateSequence, NDS_FTR_COUNT(sNdsNativeFighterStateSequence),
    sNdsNativeFighterVertexActions, NDS_FTR_COUNT(sNdsNativeFighterVertexActions),
    sNdsNativeFighterEpochDirectPolicy,
    sNdsNativeFighterDenseVertices, NDS_FTR_COUNT(sNdsNativeFighterDenseVertices),
    /* P2-3f49: NULL for Mario/Fox on purpose. This field exists so an imaged
     * owner can hand the emit a table that arrived inside its NitroFS image;
     * Mario/Fox have no image, still bake at load, and reach their words
     * through sNdsNativeFighterActiveDenseNormals, which the selector points
     * at the static arrays. Those arrays are defined in
     * nds_renderer_native_common.c, which this file precedes in the
     * translation unit, so naming them here does not even compile. */
    NULL,
    sNdsNativeFighterPreparedDense,
    sNdsNativeFighterActionDenseSpans,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeFighterDenseColorSource,
#endif
    sNdsNativeFighterPackedCorners, NDS_FTR_COUNT(sNdsNativeFighterPackedCorners),
    sNdsNativeFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeFighterRunFirstCorner),
    sNdsNativeFighterRunFirstUnique,
    sNdsNativeFighterRunUniqueCount,
    sNdsNativeFighterRunUniqueDense,
    sNdsNativeFighterTriangles, NDS_FTR_COUNT(sNdsNativeFighterTriangles),
    sNdsNativeFighterRuns, NDS_FTR_COUNT(sNdsNativeFighterRuns),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeFighterPrimitiveGroupFirst,
    sNdsNativeFighterPrimitiveGroupCount,
    sNdsNativeFighterPrimitiveGroupType,
    sNdsNativeFighterPrimitiveGroupFirstVertex,
    sNdsNativeFighterPrimitiveGroupVertexCount,
    sNdsNativeFighterPrimitiveVertices,
#endif
    sNdsNativeFighterEpochs, NDS_FTR_COUNT(sNdsNativeFighterEpochs)
};

static const NDSNativeFighterRuntimeTables sNdsNativeFighterLowTables =
{
    sNdsNativeFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeFighterStateDeltasLow),
    sNdsNativeFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeFighterStateSequenceLow),
    sNdsNativeFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeFighterVertexActionsLow),
    sNdsNativeFighterEpochDirectPolicyLow,
    sNdsNativeFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeFighterDenseVerticesLow),
    /* NULL for the same reason as the high table above. */
    NULL,
    sNdsNativeFighterPreparedDenseLow,
    sNdsNativeFighterActionDenseSpansLow,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeFighterDenseColorSourceLow,
#endif
    sNdsNativeFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeFighterPackedCornersLow),
    sNdsNativeFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeFighterRunFirstCornerLow),
    sNdsNativeFighterRunFirstUniqueLow,
    sNdsNativeFighterRunUniqueCountLow,
    sNdsNativeFighterRunUniqueDenseLow,
    sNdsNativeFighterTrianglesLow, NDS_FTR_COUNT(sNdsNativeFighterTrianglesLow),
    sNdsNativeFighterRunsLow, NDS_FTR_COUNT(sNdsNativeFighterRunsLow),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeFighterPrimitiveGroupFirstLow,
    sNdsNativeFighterPrimitiveGroupCountLow,
    sNdsNativeFighterPrimitiveGroupTypeLow,
    sNdsNativeFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeFighterPrimitiveGroupVertexCountLow,
    sNdsNativeFighterPrimitiveVerticesLow,
#endif
    sNdsNativeFighterEpochsLow, NDS_FTR_COUNT(sNdsNativeFighterEpochsLow)
};

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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeLuigiFighterDenseColorSource,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeLuigiFighterPrimitiveGroupFirst,
    sNdsNativeLuigiFighterPrimitiveGroupCount,
    sNdsNativeLuigiFighterPrimitiveGroupType,
    sNdsNativeLuigiFighterPrimitiveGroupFirstVertex,
    sNdsNativeLuigiFighterPrimitiveGroupVertexCount,
    sNdsNativeLuigiFighterPrimitiveVertices,
#endif
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeLuigiFighterDenseColorSourceLow,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeLuigiFighterPrimitiveGroupFirstLow,
    sNdsNativeLuigiFighterPrimitiveGroupCountLow,
    sNdsNativeLuigiFighterPrimitiveGroupTypeLow,
    sNdsNativeLuigiFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeLuigiFighterPrimitiveGroupVertexCountLow,
    sNdsNativeLuigiFighterPrimitiveVerticesLow,
#endif
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeDonkeyFighterDenseColorSource,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeDonkeyFighterPrimitiveGroupFirst,
    sNdsNativeDonkeyFighterPrimitiveGroupCount,
    sNdsNativeDonkeyFighterPrimitiveGroupType,
    sNdsNativeDonkeyFighterPrimitiveGroupFirstVertex,
    sNdsNativeDonkeyFighterPrimitiveGroupVertexCount,
    sNdsNativeDonkeyFighterPrimitiveVertices,
#endif
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeDonkeyFighterDenseColorSourceLow,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeDonkeyFighterPrimitiveGroupFirstLow,
    sNdsNativeDonkeyFighterPrimitiveGroupCountLow,
    sNdsNativeDonkeyFighterPrimitiveGroupTypeLow,
    sNdsNativeDonkeyFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeDonkeyFighterPrimitiveGroupVertexCountLow,
    sNdsNativeDonkeyFighterPrimitiveVerticesLow,
#endif
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeCaptainFighterDenseColorSource,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeCaptainFighterPrimitiveGroupFirst,
    sNdsNativeCaptainFighterPrimitiveGroupCount,
    sNdsNativeCaptainFighterPrimitiveGroupType,
    sNdsNativeCaptainFighterPrimitiveGroupFirstVertex,
    sNdsNativeCaptainFighterPrimitiveGroupVertexCount,
    sNdsNativeCaptainFighterPrimitiveVertices,
#endif
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeCaptainFighterDenseColorSourceLow,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeCaptainFighterPrimitiveGroupFirstLow,
    sNdsNativeCaptainFighterPrimitiveGroupCountLow,
    sNdsNativeCaptainFighterPrimitiveGroupTypeLow,
    sNdsNativeCaptainFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeCaptainFighterPrimitiveGroupVertexCountLow,
    sNdsNativeCaptainFighterPrimitiveVerticesLow,
#endif
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

#if NDS_P2_SAMUS
#if NDS_NATIVE_OWNER_IMAGE_SAMUS
static NDSNativeFighterRuntimeTables sNdsNativeSamusFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeSamusFighterHighTables =
{
    sNdsNativeSamusFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeSamusFighterStateDeltas),
    sNdsNativeSamusFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeSamusFighterStateSequence),
    sNdsNativeSamusFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeSamusFighterVertexActions),
    sNdsNativeSamusFighterEpochDirectPolicy,
    sNdsNativeSamusFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeSamusFighterDenseVertices),
    sNdsNativeSamusFighterDenseNormals,
    sNdsNativeSamusFighterPreparedDense,
    sNdsNativeSamusFighterActionDenseSpans,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeSamusFighterDenseColorSource,
#endif
    sNdsNativeSamusFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeSamusFighterPackedCorners),
    sNdsNativeSamusFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeSamusFighterRunFirstCorner),
    sNdsNativeSamusFighterRunFirstUnique,
    sNdsNativeSamusFighterRunUniqueCount,
    sNdsNativeSamusFighterRunUniqueDense,
    sNdsNativeSamusFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeSamusFighterTriangles),
    sNdsNativeSamusFighterRuns,
    NDS_FTR_COUNT(sNdsNativeSamusFighterRuns),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeSamusFighterPrimitiveGroupFirst,
    sNdsNativeSamusFighterPrimitiveGroupCount,
    sNdsNativeSamusFighterPrimitiveGroupType,
    sNdsNativeSamusFighterPrimitiveGroupFirstVertex,
    sNdsNativeSamusFighterPrimitiveGroupVertexCount,
    sNdsNativeSamusFighterPrimitiveVertices,
#endif
    sNdsNativeSamusFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeSamusFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_SAMUS
static NDSNativeFighterRuntimeTables sNdsNativeSamusFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeSamusFighterLowTables =
{
    sNdsNativeSamusFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterStateDeltasLow),
    sNdsNativeSamusFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterStateSequenceLow),
    sNdsNativeSamusFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterVertexActionsLow),
    sNdsNativeSamusFighterEpochDirectPolicyLow,
    sNdsNativeSamusFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterDenseVerticesLow),
    sNdsNativeSamusFighterDenseNormalsLow,
    sNdsNativeSamusFighterPreparedDenseLow,
    sNdsNativeSamusFighterActionDenseSpansLow,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeSamusFighterDenseColorSourceLow,
#endif
    sNdsNativeSamusFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterPackedCornersLow),
    sNdsNativeSamusFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterRunFirstCornerLow),
    sNdsNativeSamusFighterRunFirstUniqueLow,
    sNdsNativeSamusFighterRunUniqueCountLow,
    sNdsNativeSamusFighterRunUniqueDenseLow,
    sNdsNativeSamusFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterTrianglesLow),
    sNdsNativeSamusFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterRunsLow),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeSamusFighterPrimitiveGroupFirstLow,
    sNdsNativeSamusFighterPrimitiveGroupCountLow,
    sNdsNativeSamusFighterPrimitiveGroupTypeLow,
    sNdsNativeSamusFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeSamusFighterPrimitiveGroupVertexCountLow,
    sNdsNativeSamusFighterPrimitiveVerticesLow,
#endif
    sNdsNativeSamusFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeSamusFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusHighOwner, &sNdsNativeSamusFighterHighTables,
    sNdsNativeSamusRoots, sNdsNativeSamusCrossPaletteSlots,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeSamusLowOwner, &sNdsNativeSamusFighterLowTables,
    sNdsNativeSamusRootsLow, sNdsNativeSamusCrossPaletteSlotsLow,
    sNdsNativeSamusRootLightPreambles, NDS_NATIVE_SAMUS_MODEL_DATA_SIZE);
#endif

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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeLinkFighterDenseColorSource,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeLinkFighterPrimitiveGroupFirst,
    sNdsNativeLinkFighterPrimitiveGroupCount,
    sNdsNativeLinkFighterPrimitiveGroupType,
    sNdsNativeLinkFighterPrimitiveGroupFirstVertex,
    sNdsNativeLinkFighterPrimitiveGroupVertexCount,
    sNdsNativeLinkFighterPrimitiveVertices,
#endif
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeLinkFighterDenseColorSourceLow,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeLinkFighterPrimitiveGroupFirstLow,
    sNdsNativeLinkFighterPrimitiveGroupCountLow,
    sNdsNativeLinkFighterPrimitiveGroupTypeLow,
    sNdsNativeLinkFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeLinkFighterPrimitiveGroupVertexCountLow,
    sNdsNativeLinkFighterPrimitiveVerticesLow,
#endif
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
#endif

#if NDS_P2_PIKACHU
#if NDS_NATIVE_OWNER_IMAGE_PIKACHU
static NDSNativeFighterRuntimeTables sNdsNativePikachuFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativePikachuFighterHighTables =
{
    sNdsNativePikachuFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativePikachuFighterStateDeltas),
    sNdsNativePikachuFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativePikachuFighterStateSequence),
    sNdsNativePikachuFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativePikachuFighterVertexActions),
    sNdsNativePikachuFighterEpochDirectPolicy,
    sNdsNativePikachuFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativePikachuFighterDenseVertices),
    sNdsNativePikachuFighterDenseNormals,
    sNdsNativePikachuFighterPreparedDense,
    sNdsNativePikachuFighterActionDenseSpans,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativePikachuFighterDenseColorSource,
#endif
    sNdsNativePikachuFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativePikachuFighterPackedCorners),
    sNdsNativePikachuFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativePikachuFighterRunFirstCorner),
    sNdsNativePikachuFighterRunFirstUnique,
    sNdsNativePikachuFighterRunUniqueCount,
    sNdsNativePikachuFighterRunUniqueDense,
    sNdsNativePikachuFighterTriangles,
    NDS_FTR_COUNT(sNdsNativePikachuFighterTriangles),
    sNdsNativePikachuFighterRuns,
    NDS_FTR_COUNT(sNdsNativePikachuFighterRuns),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativePikachuFighterPrimitiveGroupFirst,
    sNdsNativePikachuFighterPrimitiveGroupCount,
    sNdsNativePikachuFighterPrimitiveGroupType,
    sNdsNativePikachuFighterPrimitiveGroupFirstVertex,
    sNdsNativePikachuFighterPrimitiveGroupVertexCount,
    sNdsNativePikachuFighterPrimitiveVertices,
#endif
    sNdsNativePikachuFighterEpochs,
    NDS_FTR_COUNT(sNdsNativePikachuFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_PIKACHU
static NDSNativeFighterRuntimeTables sNdsNativePikachuFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativePikachuFighterLowTables =
{
    sNdsNativePikachuFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterStateDeltasLow),
    sNdsNativePikachuFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterStateSequenceLow),
    sNdsNativePikachuFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterVertexActionsLow),
    sNdsNativePikachuFighterEpochDirectPolicyLow,
    sNdsNativePikachuFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterDenseVerticesLow),
    sNdsNativePikachuFighterDenseNormalsLow,
    sNdsNativePikachuFighterPreparedDenseLow,
    sNdsNativePikachuFighterActionDenseSpansLow,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativePikachuFighterDenseColorSourceLow,
#endif
    sNdsNativePikachuFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterPackedCornersLow),
    sNdsNativePikachuFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterRunFirstCornerLow),
    sNdsNativePikachuFighterRunFirstUniqueLow,
    sNdsNativePikachuFighterRunUniqueCountLow,
    sNdsNativePikachuFighterRunUniqueDenseLow,
    sNdsNativePikachuFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterTrianglesLow),
    sNdsNativePikachuFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterRunsLow),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativePikachuFighterPrimitiveGroupFirstLow,
    sNdsNativePikachuFighterPrimitiveGroupCountLow,
    sNdsNativePikachuFighterPrimitiveGroupTypeLow,
    sNdsNativePikachuFighterPrimitiveGroupFirstVertexLow,
    sNdsNativePikachuFighterPrimitiveGroupVertexCountLow,
    sNdsNativePikachuFighterPrimitiveVerticesLow,
#endif
    sNdsNativePikachuFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativePikachuFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativePikachuHighOwner, &sNdsNativePikachuFighterHighTables,
    sNdsNativePikachuRoots, sNdsNativePikachuCrossPaletteSlots,
    sNdsNativePikachuRootLightPreambles, NDS_NATIVE_PIKACHU_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePikachuLowOwner, &sNdsNativePikachuFighterLowTables,
    sNdsNativePikachuRootsLow, sNdsNativePikachuCrossPaletteSlotsLow,
    sNdsNativePikachuRootLightPreambles, NDS_NATIVE_PIKACHU_MODEL_DATA_SIZE);
#endif

#if NDS_P2_YOSHI
#if NDS_NATIVE_OWNER_IMAGE_YOSHI
static NDSNativeFighterRuntimeTables sNdsNativeYoshiFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeYoshiFighterHighTables =
{
    sNdsNativeYoshiFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterStateDeltas),
    sNdsNativeYoshiFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterStateSequence),
    sNdsNativeYoshiFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterVertexActions),
    sNdsNativeYoshiFighterEpochDirectPolicy,
    sNdsNativeYoshiFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterDenseVertices),
    sNdsNativeYoshiFighterDenseNormals,
    sNdsNativeYoshiFighterPreparedDense,
    sNdsNativeYoshiFighterActionDenseSpans,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeYoshiFighterDenseColorSource,
#endif
    sNdsNativeYoshiFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterPackedCorners),
    sNdsNativeYoshiFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterRunFirstCorner),
    sNdsNativeYoshiFighterRunFirstUnique,
    sNdsNativeYoshiFighterRunUniqueCount,
    sNdsNativeYoshiFighterRunUniqueDense,
    sNdsNativeYoshiFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterTriangles),
    sNdsNativeYoshiFighterRuns,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterRuns),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeYoshiFighterPrimitiveGroupFirst,
    sNdsNativeYoshiFighterPrimitiveGroupCount,
    sNdsNativeYoshiFighterPrimitiveGroupType,
    sNdsNativeYoshiFighterPrimitiveGroupFirstVertex,
    sNdsNativeYoshiFighterPrimitiveGroupVertexCount,
    sNdsNativeYoshiFighterPrimitiveVertices,
#endif
    sNdsNativeYoshiFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_YOSHI
static NDSNativeFighterRuntimeTables sNdsNativeYoshiFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeYoshiFighterLowTables =
{
    sNdsNativeYoshiFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterStateDeltasLow),
    sNdsNativeYoshiFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterStateSequenceLow),
    sNdsNativeYoshiFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterVertexActionsLow),
    sNdsNativeYoshiFighterEpochDirectPolicyLow,
    sNdsNativeYoshiFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterDenseVerticesLow),
    sNdsNativeYoshiFighterDenseNormalsLow,
    sNdsNativeYoshiFighterPreparedDenseLow,
    sNdsNativeYoshiFighterActionDenseSpansLow,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeYoshiFighterDenseColorSourceLow,
#endif
    sNdsNativeYoshiFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterPackedCornersLow),
    sNdsNativeYoshiFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterRunFirstCornerLow),
    sNdsNativeYoshiFighterRunFirstUniqueLow,
    sNdsNativeYoshiFighterRunUniqueCountLow,
    sNdsNativeYoshiFighterRunUniqueDenseLow,
    sNdsNativeYoshiFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterTrianglesLow),
    sNdsNativeYoshiFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterRunsLow),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeYoshiFighterPrimitiveGroupFirstLow,
    sNdsNativeYoshiFighterPrimitiveGroupCountLow,
    sNdsNativeYoshiFighterPrimitiveGroupTypeLow,
    sNdsNativeYoshiFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeYoshiFighterPrimitiveGroupVertexCountLow,
    sNdsNativeYoshiFighterPrimitiveVerticesLow,
#endif
    sNdsNativeYoshiFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeYoshiFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiHighOwner, &sNdsNativeYoshiFighterHighTables,
    sNdsNativeYoshiRoots, sNdsNativeYoshiCrossPaletteSlots,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeYoshiLowOwner, &sNdsNativeYoshiFighterLowTables,
    sNdsNativeYoshiRootsLow, sNdsNativeYoshiCrossPaletteSlotsLow,
    sNdsNativeYoshiRootLightPreambles, NDS_NATIVE_YOSHI_MODEL_DATA_SIZE);
#endif

#if NDS_P2_NESS
#if NDS_NATIVE_OWNER_IMAGE_NESS
static NDSNativeFighterRuntimeTables sNdsNativeNessFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNessFighterHighTables =
{
    sNdsNativeNessFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeNessFighterStateDeltas),
    sNdsNativeNessFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeNessFighterStateSequence),
    sNdsNativeNessFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeNessFighterVertexActions),
    sNdsNativeNessFighterEpochDirectPolicy,
    sNdsNativeNessFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeNessFighterDenseVertices),
    sNdsNativeNessFighterDenseNormals,
    sNdsNativeNessFighterPreparedDense,
    sNdsNativeNessFighterActionDenseSpans,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeNessFighterDenseColorSource,
#endif
    sNdsNativeNessFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeNessFighterPackedCorners),
    sNdsNativeNessFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeNessFighterRunFirstCorner),
    sNdsNativeNessFighterRunFirstUnique,
    sNdsNativeNessFighterRunUniqueCount,
    sNdsNativeNessFighterRunUniqueDense,
    sNdsNativeNessFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeNessFighterTriangles),
    sNdsNativeNessFighterRuns,
    NDS_FTR_COUNT(sNdsNativeNessFighterRuns),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeNessFighterPrimitiveGroupFirst,
    sNdsNativeNessFighterPrimitiveGroupCount,
    sNdsNativeNessFighterPrimitiveGroupType,
    sNdsNativeNessFighterPrimitiveGroupFirstVertex,
    sNdsNativeNessFighterPrimitiveGroupVertexCount,
    sNdsNativeNessFighterPrimitiveVertices,
#endif
    sNdsNativeNessFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeNessFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_NESS
static NDSNativeFighterRuntimeTables sNdsNativeNessFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeNessFighterLowTables =
{
    sNdsNativeNessFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterStateDeltasLow),
    sNdsNativeNessFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterStateSequenceLow),
    sNdsNativeNessFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterVertexActionsLow),
    sNdsNativeNessFighterEpochDirectPolicyLow,
    sNdsNativeNessFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterDenseVerticesLow),
    sNdsNativeNessFighterDenseNormalsLow,
    sNdsNativeNessFighterPreparedDenseLow,
    sNdsNativeNessFighterActionDenseSpansLow,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeNessFighterDenseColorSourceLow,
#endif
    sNdsNativeNessFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterPackedCornersLow),
    sNdsNativeNessFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterRunFirstCornerLow),
    sNdsNativeNessFighterRunFirstUniqueLow,
    sNdsNativeNessFighterRunUniqueCountLow,
    sNdsNativeNessFighterRunUniqueDenseLow,
    sNdsNativeNessFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterTrianglesLow),
    sNdsNativeNessFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterRunsLow),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeNessFighterPrimitiveGroupFirstLow,
    sNdsNativeNessFighterPrimitiveGroupCountLow,
    sNdsNativeNessFighterPrimitiveGroupTypeLow,
    sNdsNativeNessFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeNessFighterPrimitiveGroupVertexCountLow,
    sNdsNativeNessFighterPrimitiveVerticesLow,
#endif
    sNdsNativeNessFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeNessFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessHighOwner, &sNdsNativeNessFighterHighTables,
    sNdsNativeNessRoots, sNdsNativeNessCrossPaletteSlots,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeNessLowOwner, &sNdsNativeNessFighterLowTables,
    sNdsNativeNessRootsLow, sNdsNativeNessCrossPaletteSlotsLow,
    sNdsNativeNessRootLightPreambles, NDS_NATIVE_NESS_MODEL_DATA_SIZE);
#endif

#if NDS_P2_PURIN
#if NDS_NATIVE_OWNER_IMAGE_PURIN
static NDSNativeFighterRuntimeTables sNdsNativePurinFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativePurinFighterHighTables =
{
    sNdsNativePurinFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativePurinFighterStateDeltas),
    sNdsNativePurinFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativePurinFighterStateSequence),
    sNdsNativePurinFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativePurinFighterVertexActions),
    sNdsNativePurinFighterEpochDirectPolicy,
    sNdsNativePurinFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativePurinFighterDenseVertices),
    sNdsNativePurinFighterDenseNormals,
    sNdsNativePurinFighterPreparedDense,
    sNdsNativePurinFighterActionDenseSpans,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativePurinFighterDenseColorSource,
#endif
    sNdsNativePurinFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativePurinFighterPackedCorners),
    sNdsNativePurinFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativePurinFighterRunFirstCorner),
    sNdsNativePurinFighterRunFirstUnique,
    sNdsNativePurinFighterRunUniqueCount,
    sNdsNativePurinFighterRunUniqueDense,
    sNdsNativePurinFighterTriangles,
    NDS_FTR_COUNT(sNdsNativePurinFighterTriangles),
    sNdsNativePurinFighterRuns,
    NDS_FTR_COUNT(sNdsNativePurinFighterRuns),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativePurinFighterPrimitiveGroupFirst,
    sNdsNativePurinFighterPrimitiveGroupCount,
    sNdsNativePurinFighterPrimitiveGroupType,
    sNdsNativePurinFighterPrimitiveGroupFirstVertex,
    sNdsNativePurinFighterPrimitiveGroupVertexCount,
    sNdsNativePurinFighterPrimitiveVertices,
#endif
    sNdsNativePurinFighterEpochs,
    NDS_FTR_COUNT(sNdsNativePurinFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_PURIN
static NDSNativeFighterRuntimeTables sNdsNativePurinFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativePurinFighterLowTables =
{
    sNdsNativePurinFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterStateDeltasLow),
    sNdsNativePurinFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterStateSequenceLow),
    sNdsNativePurinFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterVertexActionsLow),
    sNdsNativePurinFighterEpochDirectPolicyLow,
    sNdsNativePurinFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterDenseVerticesLow),
    sNdsNativePurinFighterDenseNormalsLow,
    sNdsNativePurinFighterPreparedDenseLow,
    sNdsNativePurinFighterActionDenseSpansLow,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativePurinFighterDenseColorSourceLow,
#endif
    sNdsNativePurinFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterPackedCornersLow),
    sNdsNativePurinFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterRunFirstCornerLow),
    sNdsNativePurinFighterRunFirstUniqueLow,
    sNdsNativePurinFighterRunUniqueCountLow,
    sNdsNativePurinFighterRunUniqueDenseLow,
    sNdsNativePurinFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterTrianglesLow),
    sNdsNativePurinFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterRunsLow),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativePurinFighterPrimitiveGroupFirstLow,
    sNdsNativePurinFighterPrimitiveGroupCountLow,
    sNdsNativePurinFighterPrimitiveGroupTypeLow,
    sNdsNativePurinFighterPrimitiveGroupFirstVertexLow,
    sNdsNativePurinFighterPrimitiveGroupVertexCountLow,
    sNdsNativePurinFighterPrimitiveVerticesLow,
#endif
    sNdsNativePurinFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativePurinFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativePurinHighOwner, &sNdsNativePurinFighterHighTables,
    sNdsNativePurinRoots, sNdsNativePurinCrossPaletteSlots,
    sNdsNativePurinRootLightPreambles, NDS_NATIVE_PURIN_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativePurinLowOwner, &sNdsNativePurinFighterLowTables,
    sNdsNativePurinRootsLow, sNdsNativePurinCrossPaletteSlotsLow,
    sNdsNativePurinRootLightPreambles, NDS_NATIVE_PURIN_MODEL_DATA_SIZE);
#endif

#if NDS_P2_KIRBY
#if NDS_NATIVE_OWNER_IMAGE_KIRBY
static NDSNativeFighterRuntimeTables sNdsNativeKirbyFighterHighTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeKirbyFighterHighTables =
{
    sNdsNativeKirbyFighterStateDeltas,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterStateDeltas),
    sNdsNativeKirbyFighterStateSequence,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterStateSequence),
    sNdsNativeKirbyFighterVertexActions,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterVertexActions),
    sNdsNativeKirbyFighterEpochDirectPolicy,
    sNdsNativeKirbyFighterDenseVertices,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterDenseVertices),
    sNdsNativeKirbyFighterDenseNormals,
    sNdsNativeKirbyFighterPreparedDense,
    sNdsNativeKirbyFighterActionDenseSpans,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeKirbyFighterDenseColorSource,
#endif
    sNdsNativeKirbyFighterPackedCorners,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterPackedCorners),
    sNdsNativeKirbyFighterRunFirstCorner,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterRunFirstCorner),
    sNdsNativeKirbyFighterRunFirstUnique,
    sNdsNativeKirbyFighterRunUniqueCount,
    sNdsNativeKirbyFighterRunUniqueDense,
    sNdsNativeKirbyFighterTriangles,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterTriangles),
    sNdsNativeKirbyFighterRuns,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterRuns),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeKirbyFighterPrimitiveGroupFirst,
    sNdsNativeKirbyFighterPrimitiveGroupCount,
    sNdsNativeKirbyFighterPrimitiveGroupType,
    sNdsNativeKirbyFighterPrimitiveGroupFirstVertex,
    sNdsNativeKirbyFighterPrimitiveGroupVertexCount,
    sNdsNativeKirbyFighterPrimitiveVertices,
#endif
    sNdsNativeKirbyFighterEpochs,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterEpochs)
};
#endif

#if NDS_NATIVE_OWNER_IMAGE_KIRBY
static NDSNativeFighterRuntimeTables sNdsNativeKirbyFighterLowTables;
#else
static const NDSNativeFighterRuntimeTables sNdsNativeKirbyFighterLowTables =
{
    sNdsNativeKirbyFighterStateDeltasLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterStateDeltasLow),
    sNdsNativeKirbyFighterStateSequenceLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterStateSequenceLow),
    sNdsNativeKirbyFighterVertexActionsLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterVertexActionsLow),
    sNdsNativeKirbyFighterEpochDirectPolicyLow,
    sNdsNativeKirbyFighterDenseVerticesLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterDenseVerticesLow),
    sNdsNativeKirbyFighterDenseNormalsLow,
    sNdsNativeKirbyFighterPreparedDenseLow,
    sNdsNativeKirbyFighterActionDenseSpansLow,
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeKirbyFighterDenseColorSourceLow,
#endif
    sNdsNativeKirbyFighterPackedCornersLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterPackedCornersLow),
    sNdsNativeKirbyFighterRunFirstCornerLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterRunFirstCornerLow),
    sNdsNativeKirbyFighterRunFirstUniqueLow,
    sNdsNativeKirbyFighterRunUniqueCountLow,
    sNdsNativeKirbyFighterRunUniqueDenseLow,
    sNdsNativeKirbyFighterTrianglesLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterTrianglesLow),
    sNdsNativeKirbyFighterRunsLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterRunsLow),
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeKirbyFighterPrimitiveGroupFirstLow,
    sNdsNativeKirbyFighterPrimitiveGroupCountLow,
    sNdsNativeKirbyFighterPrimitiveGroupTypeLow,
    sNdsNativeKirbyFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeKirbyFighterPrimitiveGroupVertexCountLow,
    sNdsNativeKirbyFighterPrimitiveVerticesLow,
#endif
    sNdsNativeKirbyFighterEpochsLow,
    NDS_FTR_COUNT(sNdsNativeKirbyFighterEpochsLow)
};
#endif

NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyHighOwner, &sNdsNativeKirbyFighterHighTables,
    sNdsNativeKirbyRoots, sNdsNativeKirbyCrossPaletteSlots,
    sNdsNativeKirbyRootLightPreambles, NDS_NATIVE_KIRBY_MODEL_DATA_SIZE);
NDS_FTR_OWNER_RUNTIME(
    sNdsNativeKirbyLowOwner, &sNdsNativeKirbyFighterLowTables,
    sNdsNativeKirbyRootsLow, sNdsNativeKirbyCrossPaletteSlotsLow,
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeMMarioFighterDenseColorSource,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeMMarioFighterPrimitiveGroupFirst,
    sNdsNativeMMarioFighterPrimitiveGroupCount,
    sNdsNativeMMarioFighterPrimitiveGroupType,
    sNdsNativeMMarioFighterPrimitiveGroupFirstVertex,
    sNdsNativeMMarioFighterPrimitiveGroupVertexCount,
    sNdsNativeMMarioFighterPrimitiveVertices,
#endif
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
#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
    sNdsNativeMMarioFighterDenseColorSourceLow,
#endif
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
#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
    sNdsNativeMMarioFighterPrimitiveGroupFirstLow,
    sNdsNativeMMarioFighterPrimitiveGroupCountLow,
    sNdsNativeMMarioFighterPrimitiveGroupTypeLow,
    sNdsNativeMMarioFighterPrimitiveGroupFirstVertexLow,
    sNdsNativeMMarioFighterPrimitiveGroupVertexCountLow,
    sNdsNativeMMarioFighterPrimitiveVerticesLow,
#endif
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

#undef NDS_FTR_OWNER_RUNTIME

static const NDSNativeFighterRuntimeTables *sNdsNativeFighterActiveTables =
    &sNdsNativeFighterHighTables;
static const NDSNativeFighterOwnerRuntime *sNdsNativeFighterActiveOwner =
    &sNdsNativeMarioHighOwner;

#if NDS_P2_LUIGI || NDS_P2_DONKEY || NDS_P2_CAPTAIN || NDS_P2_SAMUS || NDS_P2_LINK || NDS_P2_PIKACHU || NDS_P2_YOSHI || NDS_P2_NESS || NDS_P2_PURIN || NDS_P2_KIRBY || NDS_P2_MMARIO
/* --- P2-3r4: image-backed owner tables ------------------------------------
 *
 * A P2-3 owner's generated geometry ships as a NitroFS image rather than as
 * arrays in the ARM9 binary, because on this hardware the binary costs the
 * taskman arena one byte for one byte and the roster is going to grow by ten
 * more fighters. The image is one struct, so the layout is the compiler's and
 * these offsets cannot drift from the bytes.
 *
 * The buffer comes from the taskman arena, which the scene manager rewinds
 * between scenes -- hence the heap generation stamp: a pointer from the
 * previous scene is not reloaded, it is re-read. Loading happens where a
 * fighter is CREATED, never inside a draw: a NitroFS read mid-frame is exactly
 * the stall that cost the BGM its seam on the character select. */
#define NDS_NATIVE_IMAGE_DETAILS 2u

typedef struct NDSNativeOwnerImageSlot
{
    const void *base;
    u32 heap_generation;
    u32 bytes;
} NDSNativeOwnerImageSlot;

static NDSNativeOwnerImageSlot
    sNdsNativeOwnerImage[NDS_NATIVE_IMAGE_OWNER_SLOTS][NDS_NATIVE_IMAGE_DETAILS];

__attribute__((used)) volatile u32 gNdsNativeOwnerImageLoadCount;
__attribute__((used)) volatile u32 gNdsNativeOwnerImageFailCount;
__attribute__((used)) volatile u32 gNdsNativeOwnerImageBytes;
__attribute__((used)) volatile u32 gNdsNativeOwnerImageMatchCount;
__attribute__((used)) volatile u32 gNdsNativeOwnerImageMismatchCount;

static const char *ndsRendererNativeOwnerImagePath(u32 owner_slot,
                                                   u32 use_low_detail)
{
#if NDS_P2_LUIGI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LUIGI)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/luigi_low.bin" :
                                        "nitro:/fighters/luigi_high.bin";
    }
#endif
#if NDS_P2_DONKEY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_DONKEY)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/donkey_low.bin" :
                                        "nitro:/fighters/donkey_high.bin";
    }
#endif
#if NDS_P2_CAPTAIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_CAPTAIN)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/captain_low.bin" :
                                        "nitro:/fighters/captain_high.bin";
    }
#endif
#if NDS_P2_SAMUS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_SAMUS)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/samus_low.bin" :
                                        "nitro:/fighters/samus_high.bin";
    }
#endif
#if NDS_P2_LINK
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LINK)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/link_low.bin" :
                                        "nitro:/fighters/link_high.bin";
    }
#endif
#if NDS_P2_PIKACHU
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PIKACHU)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/pikachu_low.bin" :
                                        "nitro:/fighters/pikachu_high.bin";
    }
#endif
#if NDS_P2_YOSHI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_YOSHI)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/yoshi_low.bin" :
                                        "nitro:/fighters/yoshi_high.bin";
    }
#endif
#if NDS_P2_NESS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NESS)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/ness_low.bin" :
                                        "nitro:/fighters/ness_high.bin";
    }
#endif
#if NDS_P2_PURIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PURIN)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/purin_low.bin" :
                                        "nitro:/fighters/purin_high.bin";
    }
#endif
#if NDS_P2_KIRBY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_KIRBY)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/kirby_low.bin" :
                                        "nitro:/fighters/kirby_high.bin";
    }
#endif
#if NDS_P2_MMARIO
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_MMARIO)
    {
        return (use_low_detail != 0u) ? "nitro:/fighters/mmario_low.bin" :
                                        "nitro:/fighters/mmario_high.bin";
    }
#endif
    (void)use_low_detail;
    return NULL;
}

static u32 ndsRendererNativeOwnerImageBytes(u32 owner_slot, u32 use_low_detail)
{
#if NDS_P2_LUIGI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LUIGI)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeLuigiLowImage) :
            (u32)sizeof(NDSNativeLuigiHighImage);
    }
#endif
#if NDS_P2_DONKEY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_DONKEY)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeDonkeyLowImage) :
            (u32)sizeof(NDSNativeDonkeyHighImage);
    }
#endif
#if NDS_P2_CAPTAIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_CAPTAIN)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeCaptainLowImage) :
            (u32)sizeof(NDSNativeCaptainHighImage);
    }
#endif
#if NDS_P2_SAMUS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_SAMUS)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeSamusLowImage) :
            (u32)sizeof(NDSNativeSamusHighImage);
    }
#endif
#if NDS_P2_LINK
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LINK)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeLinkLowImage) :
            (u32)sizeof(NDSNativeLinkHighImage);
    }
#endif
#if NDS_P2_PIKACHU
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PIKACHU)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativePikachuLowImage) :
            (u32)sizeof(NDSNativePikachuHighImage);
    }
#endif
#if NDS_P2_YOSHI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_YOSHI)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeYoshiLowImage) :
            (u32)sizeof(NDSNativeYoshiHighImage);
    }
#endif
#if NDS_P2_NESS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NESS)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeNessLowImage) :
            (u32)sizeof(NDSNativeNessHighImage);
    }
#endif
#if NDS_P2_PURIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PURIN)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativePurinLowImage) :
            (u32)sizeof(NDSNativePurinHighImage);
    }
#endif
#if NDS_P2_KIRBY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_KIRBY)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeKirbyLowImage) :
            (u32)sizeof(NDSNativeKirbyHighImage);
    }
#endif
#if NDS_P2_MMARIO
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_MMARIO)
    {
        return (use_low_detail != 0u) ?
            (u32)sizeof(NDSNativeMMarioLowImage) :
            (u32)sizeof(NDSNativeMMarioHighImage);
    }
#endif
    (void)use_low_detail;
    return 0u;
}

/* --- P2-3r4: binding an owner's tables to its loaded image ----------------
 *
 * The runtime tables struct is a set of pointers plus counts. Binding is
 * therefore assigning each pointer into the loaded image and each count from
 * the generated `_COUNT` macro that produced that array. Both come out of
 * `nds_native_fighter_image.generated.h`, so a table this ROM reads and the
 * bytes this ROM loads are described by one file.
 *
 * `prepared_dense` is the exception and stays a resident array: it is the
 * GX-packed vertex scratch the draw path WRITES, not content. */
#if NDS_TASK56_FIGHTER_PRIMITIVES == 1
#define NDS_IMG_PRIM(image_, member_) ((image_)->member_##_m1)
#elif NDS_TASK56_FIGHTER_PRIMITIVES == 2
#define NDS_IMG_PRIM(image_, member_) ((image_)->member_##_m2)
#endif

#if !NDS_R2_FIGHTER_HW_LIGHT || NDS_RENDERER_M2_DETAILED_LEDGER
#define NDS_IMG_BIND_COLOR(tables_, img_)                                      \
    (tables_).dense_color_source = (img_)->dense_color_source;
#else
#define NDS_IMG_BIND_COLOR(tables_, img_)
#endif

#if NDS_TASK56_FIGHTER_PRIMITIVES >= 1
#define NDS_IMG_BIND_PRIMITIVES(tables_, img_)                                 \
    (tables_).primitive_group_first =                                          \
        NDS_IMG_PRIM(img_, primitive_group_first);                             \
    (tables_).primitive_group_count =                                          \
        NDS_IMG_PRIM(img_, primitive_group_count);                             \
    (tables_).primitive_group_type =                                           \
        NDS_IMG_PRIM(img_, primitive_group_type);                              \
    (tables_).primitive_group_first_vertex =                                   \
        NDS_IMG_PRIM(img_, primitive_group_first_vertex);                      \
    (tables_).primitive_group_vertex_count =                                   \
        NDS_IMG_PRIM(img_, primitive_group_vertex_count);                      \
    (tables_).primitive_vertices =                                             \
        NDS_IMG_PRIM(img_, primitive_vertices);
#else
#define NDS_IMG_BIND_PRIMITIVES(tables_, img_)
#endif

#define NDS_IMG_BIND(tables_, type_, base_, prefix_, prepared_)                \
    do                                                                         \
    {                                                                          \
        const type_ *img_ = (const type_ *)(base_);                            \
        (tables_).state_deltas = img_->state_deltas;                           \
        (tables_).state_delta_count = prefix_##_STATE_DELTAS_COUNT;            \
        (tables_).state_sequence = img_->state_sequence;                       \
        (tables_).state_sequence_count = prefix_##_STATE_SEQUENCE_COUNT;       \
        (tables_).vertex_actions = img_->vertex_actions;                       \
        (tables_).vertex_action_count = prefix_##_VERTEX_ACTIONS_COUNT;        \
        (tables_).epoch_direct_policy = img_->epoch_direct_policy;             \
        (tables_).dense_vertices = img_->dense_vertices;                       \
        (tables_).dense_count = prefix_##_DENSE_VERTICES_COUNT;                \
        (tables_).dense_normals = img_->dense_normals;                         \
        (tables_).prepared_dense = (prepared_);                                \
        (tables_).action_dense_spans = img_->action_dense_spans;               \
        NDS_IMG_BIND_COLOR(tables_, img_)                                      \
        (tables_).packed_corners = img_->packed_corners;                       \
        (tables_).packed_corner_count = prefix_##_PACKED_CORNERS_COUNT;        \
        (tables_).run_first_corner = img_->run_first_corner;                   \
        (tables_).run_first_corner_count = prefix_##_RUN_FIRST_CORNER_COUNT;   \
        (tables_).run_first_unique = img_->run_first_unique;                   \
        (tables_).run_unique_count = img_->run_unique_count;                   \
        (tables_).run_unique_dense = img_->run_unique_dense;                   \
        (tables_).triangles = img_->triangles;                                 \
        (tables_).triangle_count = prefix_##_TRIANGLES_COUNT;                  \
        (tables_).runs = img_->runs;                                           \
        (tables_).run_count = prefix_##_RUNS_COUNT;                            \
        NDS_IMG_BIND_PRIMITIVES(tables_, img_)                                 \
        (tables_).epochs = img_->epochs;                                       \
        (tables_).epoch_count = prefix_##_EPOCHS_COUNT;                        \
    } while (0)

/* Bind every owner whose tables this build takes from an image. Called once
 * per successful load, with the buffer this scene's arena generation owns. */
static void ndsRendererNativeBindOwnerImage(u32 owner_slot, u32 use_low_detail,
                                            const void *base)
{
#if NDS_NATIVE_OWNER_IMAGE_LUIGI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LUIGI)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeLuigiFighterLowTables,
                         NDSNativeLuigiLowImage, base,
                         NDS_NATIVE_IMAGE_LUIGI_LOW,
                         sNdsNativeLuigiFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeLuigiFighterHighTables,
                         NDSNativeLuigiHighImage, base,
                         NDS_NATIVE_IMAGE_LUIGI_HIGH,
                         sNdsNativeLuigiFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_DONKEY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_DONKEY)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeDonkeyFighterLowTables,
                         NDSNativeDonkeyLowImage, base,
                         NDS_NATIVE_IMAGE_DONKEY_LOW,
                         sNdsNativeDonkeyFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeDonkeyFighterHighTables,
                         NDSNativeDonkeyHighImage, base,
                         NDS_NATIVE_IMAGE_DONKEY_HIGH,
                         sNdsNativeDonkeyFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_CAPTAIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_CAPTAIN)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeCaptainFighterLowTables,
                         NDSNativeCaptainLowImage, base,
                         NDS_NATIVE_IMAGE_CAPTAIN_LOW,
                         sNdsNativeCaptainFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeCaptainFighterHighTables,
                         NDSNativeCaptainHighImage, base,
                         NDS_NATIVE_IMAGE_CAPTAIN_HIGH,
                         sNdsNativeCaptainFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_SAMUS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_SAMUS)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeSamusFighterLowTables,
                         NDSNativeSamusLowImage, base,
                         NDS_NATIVE_IMAGE_SAMUS_LOW,
                         sNdsNativeSamusFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeSamusFighterHighTables,
                         NDSNativeSamusHighImage, base,
                         NDS_NATIVE_IMAGE_SAMUS_HIGH,
                         sNdsNativeSamusFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_LINK
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LINK)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeLinkFighterLowTables,
                         NDSNativeLinkLowImage, base,
                         NDS_NATIVE_IMAGE_LINK_LOW,
                         sNdsNativeLinkFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeLinkFighterHighTables,
                         NDSNativeLinkHighImage, base,
                         NDS_NATIVE_IMAGE_LINK_HIGH,
                         sNdsNativeLinkFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_PIKACHU
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PIKACHU)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativePikachuFighterLowTables,
                         NDSNativePikachuLowImage, base,
                         NDS_NATIVE_IMAGE_PIKACHU_LOW,
                         sNdsNativePikachuFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativePikachuFighterHighTables,
                         NDSNativePikachuHighImage, base,
                         NDS_NATIVE_IMAGE_PIKACHU_HIGH,
                         sNdsNativePikachuFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_YOSHI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_YOSHI)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeYoshiFighterLowTables,
                         NDSNativeYoshiLowImage, base,
                         NDS_NATIVE_IMAGE_YOSHI_LOW,
                         sNdsNativeYoshiFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeYoshiFighterHighTables,
                         NDSNativeYoshiHighImage, base,
                         NDS_NATIVE_IMAGE_YOSHI_HIGH,
                         sNdsNativeYoshiFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_NESS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NESS)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeNessFighterLowTables,
                         NDSNativeNessLowImage, base,
                         NDS_NATIVE_IMAGE_NESS_LOW,
                         sNdsNativeNessFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeNessFighterHighTables,
                         NDSNativeNessHighImage, base,
                         NDS_NATIVE_IMAGE_NESS_HIGH,
                         sNdsNativeNessFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_PURIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PURIN)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativePurinFighterLowTables,
                         NDSNativePurinLowImage, base,
                         NDS_NATIVE_IMAGE_PURIN_LOW,
                         sNdsNativePurinFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativePurinFighterHighTables,
                         NDSNativePurinHighImage, base,
                         NDS_NATIVE_IMAGE_PURIN_HIGH,
                         sNdsNativePurinFighterPreparedDense);
        }
        return;
    }
#endif
#if NDS_NATIVE_OWNER_IMAGE_KIRBY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_KIRBY)
    {
        if (use_low_detail != 0u)
        {
            NDS_IMG_BIND(sNdsNativeKirbyFighterLowTables,
                         NDSNativeKirbyLowImage, base,
                         NDS_NATIVE_IMAGE_KIRBY_LOW,
                         sNdsNativeKirbyFighterPreparedDenseLow);
        }
        else
        {
            NDS_IMG_BIND(sNdsNativeKirbyFighterHighTables,
                         NDSNativeKirbyHighImage, base,
                         NDS_NATIVE_IMAGE_KIRBY_HIGH,
                         sNdsNativeKirbyFighterPreparedDense);
        }
        return;
    }
#endif
    (void)owner_slot;
    (void)use_low_detail;
    (void)base;
}

/* Load one owner image for this scene, or report that it is already resident.
 * Called from fighter creation; never from a draw. */
s32 ndsRendererNativeEnsureOwnerImage(u32 owner_slot, u32 use_low_detail)
{
    NdsRelocAssetStream stream;
    NDSNativeOwnerImageSlot *slot;
    const char *path;
    u32 bytes;
    void *buffer;

    if ((owner_slot >= NDS_NATIVE_IMAGE_OWNER_SLOTS) ||
        (use_low_detail >= NDS_NATIVE_IMAGE_DETAILS))
    {
        return FALSE;
    }
    slot = &sNdsNativeOwnerImage[owner_slot][use_low_detail];
    if ((slot->base != NULL) &&
        (slot->heap_generation == gNdsTaskmanHeapGeneration))
    {
        return TRUE;
    }
    path = ndsRendererNativeOwnerImagePath(owner_slot, use_low_detail);
    bytes = ndsRendererNativeOwnerImageBytes(owner_slot, use_low_detail);
    if ((path == NULL) || (bytes == 0u))
    {
        return FALSE;
    }
    buffer = syTaskmanMalloc(bytes, 0x10u);
    if (buffer == NULL)
    {
        gNdsNativeOwnerImageFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamOpen(&stream, path) == FALSE)
    {
        gNdsNativeOwnerImageFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamRead(&stream, 0u, buffer, bytes) == FALSE)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeOwnerImageFailCount++;
        return FALSE;
    }
    ndsRelocAssetStreamClose(&stream);
    slot->base = buffer;
    slot->heap_generation = gNdsTaskmanHeapGeneration;
    slot->bytes = bytes;
    gNdsNativeOwnerImageLoadCount++;
    gNdsNativeOwnerImageBytes += bytes;
    ndsRendererNativeBindOwnerImage(owner_slot, use_low_detail, buffer);
    return TRUE;
}

#if NDS_NATIVE_OWNER_IMAGE_VERIFY
/* THE EQUIVALENCE PROOF, RUN ON THE CONSOLE RATHER THAN ARGUED ON PAPER.
 *
 * Moving a table out of the binary is only safe if the bytes that arrive are
 * the bytes that left. This compares the loaded image against the arrays it
 * replaces, member by member, using the pairing list the image generator emits
 * from the same description that produced both. It needs a build where BOTH
 * exist, which is `NDS_NATIVE_OWNER_IMAGE=0 NDS_NATIVE_OWNER_IMAGE_VERIFY=1`:
 * arrays compiled in, image staged and loaded, nothing reading the image yet.
 * That build is the proof; the shipping build is the one this proof licenses.
 *
 * A size disagreement is reported as a mismatch rather than clamped -- a short
 * compare that passes is exactly the false green this exists to prevent. */
static void ndsRendererNativeVerifyMember(const void *image_member,
                                          u32 image_bytes,
                                          const void *array, u32 array_bytes)
{
    const u8 *a = (const u8 *)image_member;
    const u8 *b = (const u8 *)array;
    u32 i;

    if (image_bytes != array_bytes)
    {
        gNdsNativeOwnerImageMismatchCount++;
        return;
    }
    for (i = 0u; i < array_bytes; i++)
    {
        if (a[i] != b[i])
        {
            gNdsNativeOwnerImageMismatchCount++;
            return;
        }
    }
    gNdsNativeOwnerImageMatchCount++;
}

#define NDS_IMG_VERIFY(type_, member_, array_)                                 \
    ndsRendererNativeVerifyMember(&img_->member_, (u32)sizeof(img_->member_),  \
                                  (array_), (u32)sizeof(array_));

/* P2-3f49: normals are baked, not copied, so the byte compare above cannot
 * cover them: at VERIFY time (fighter creation, before first draw) the bake
 * arrays are still empty. Re-bake from the in-binary dense_vertices -- the
 * bake's own input, already proven equal to the image's by the vertices row
 * -- with the bake's own arithmetic, and compare word for word. */
static s32 ndsRendererNativeVerifyNormalComponent(s32 source)
{
    s32 scaled = (source * 0x1ff) / 127;

    if (scaled > 511) { scaled = 511; }
    if (scaled < -512) { scaled = -512; }
    return scaled;
}

static void ndsRendererNativeVerifyDenseNormals(
    const NDSNativeDenseVertex *vertices, u32 vertex_count,
    const u32 *image_normals, u32 image_bytes)
{
    u32 i;

    if (image_bytes != vertex_count * (u32)sizeof(u32))
    {
        gNdsNativeOwnerImageMismatchCount++;
        return;
    }
    for (i = 0u; i < vertex_count; i++)
    {
        u32 rgba = vertices[i].rgba;
        s32 nx = ndsRendererNativeVerifyNormalComponent((s32)(s8)(rgba >> 24));
        s32 ny = ndsRendererNativeVerifyNormalComponent((s32)(s8)(rgba >> 16));
        s32 nz = ndsRendererNativeVerifyNormalComponent((s32)(s8)(rgba >> 8));
        u32 expected = ((((u32)nx) & 0x3ffu) |
                        (((((u32)ny) & 0x3ffu)) << 10) |
                        (((((u32)nz) & 0x3ffu)) << 20));

        if (expected != image_normals[i])
        {
            gNdsNativeOwnerImageMismatchCount++;
            return;
        }
    }
    gNdsNativeOwnerImageMatchCount++;
}

#define NDS_IMG_VERIFY_NORMALS(type_, member_, vertices_)                      \
    ndsRendererNativeVerifyDenseNormals((vertices_),                           \
                                        NDS_FTR_COUNT(vertices_),              \
                                        img_->member_,                        \
                                        (u32)sizeof(img_->member_));

s32 ndsRendererNativeVerifyOwnerImage(u32 owner_slot, u32 use_low_detail)
{
    const NDSNativeOwnerImageSlot *slot;
    u32 before;

    if ((owner_slot >= NDS_NATIVE_IMAGE_OWNER_SLOTS) ||
        (use_low_detail >= NDS_NATIVE_IMAGE_DETAILS))
    {
        return FALSE;
    }
    slot = &sNdsNativeOwnerImage[owner_slot][use_low_detail];
    if (slot->base == NULL)
    {
        gNdsNativeOwnerImageMismatchCount++;
        return FALSE;
    }
    before = gNdsNativeOwnerImageMismatchCount;
#if NDS_P2_LUIGI && !NDS_NATIVE_OWNER_IMAGE_LUIGI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LUIGI)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeLuigiLowImage *img_ =
                (const NDSNativeLuigiLowImage *)slot->base;
            NDS_NATIVE_IMAGE_LUIGI_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_LUIGI_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeLuigiHighImage *img_ =
                (const NDSNativeLuigiHighImage *)slot->base;
            NDS_NATIVE_IMAGE_LUIGI_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_LUIGI_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_DONKEY && !NDS_NATIVE_OWNER_IMAGE_DONKEY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_DONKEY)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeDonkeyLowImage *img_ =
                (const NDSNativeDonkeyLowImage *)slot->base;
            NDS_NATIVE_IMAGE_DONKEY_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_DONKEY_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeDonkeyHighImage *img_ =
                (const NDSNativeDonkeyHighImage *)slot->base;
            NDS_NATIVE_IMAGE_DONKEY_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_DONKEY_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_CAPTAIN && !NDS_NATIVE_OWNER_IMAGE_CAPTAIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_CAPTAIN)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeCaptainLowImage *img_ =
                (const NDSNativeCaptainLowImage *)slot->base;
            NDS_NATIVE_IMAGE_CAPTAIN_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_CAPTAIN_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeCaptainHighImage *img_ =
                (const NDSNativeCaptainHighImage *)slot->base;
            NDS_NATIVE_IMAGE_CAPTAIN_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_CAPTAIN_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_SAMUS && !NDS_NATIVE_OWNER_IMAGE_SAMUS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_SAMUS)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeSamusLowImage *img_ =
                (const NDSNativeSamusLowImage *)slot->base;
            NDS_NATIVE_IMAGE_SAMUS_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_SAMUS_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeSamusHighImage *img_ =
                (const NDSNativeSamusHighImage *)slot->base;
            NDS_NATIVE_IMAGE_SAMUS_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_SAMUS_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_LINK && !NDS_NATIVE_OWNER_IMAGE_LINK
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_LINK)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeLinkLowImage *img_ =
                (const NDSNativeLinkLowImage *)slot->base;
            NDS_NATIVE_IMAGE_LINK_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_LINK_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeLinkHighImage *img_ =
                (const NDSNativeLinkHighImage *)slot->base;
            NDS_NATIVE_IMAGE_LINK_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_LINK_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_PIKACHU && !NDS_NATIVE_OWNER_IMAGE_PIKACHU
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PIKACHU)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativePikachuLowImage *img_ =
                (const NDSNativePikachuLowImage *)slot->base;
            NDS_NATIVE_IMAGE_PIKACHU_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_PIKACHU_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativePikachuHighImage *img_ =
                (const NDSNativePikachuHighImage *)slot->base;
            NDS_NATIVE_IMAGE_PIKACHU_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_PIKACHU_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_YOSHI && !NDS_NATIVE_OWNER_IMAGE_YOSHI
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_YOSHI)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeYoshiLowImage *img_ =
                (const NDSNativeYoshiLowImage *)slot->base;
            NDS_NATIVE_IMAGE_YOSHI_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_YOSHI_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeYoshiHighImage *img_ =
                (const NDSNativeYoshiHighImage *)slot->base;
            NDS_NATIVE_IMAGE_YOSHI_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_YOSHI_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_NESS && !NDS_NATIVE_OWNER_IMAGE_NESS
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_NESS)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeNessLowImage *img_ =
                (const NDSNativeNessLowImage *)slot->base;
            NDS_NATIVE_IMAGE_NESS_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_NESS_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeNessHighImage *img_ =
                (const NDSNativeNessHighImage *)slot->base;
            NDS_NATIVE_IMAGE_NESS_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_NESS_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_PURIN && !NDS_NATIVE_OWNER_IMAGE_PURIN
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_PURIN)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativePurinLowImage *img_ =
                (const NDSNativePurinLowImage *)slot->base;
            NDS_NATIVE_IMAGE_PURIN_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_PURIN_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativePurinHighImage *img_ =
                (const NDSNativePurinHighImage *)slot->base;
            NDS_NATIVE_IMAGE_PURIN_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_PURIN_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_KIRBY && !NDS_NATIVE_OWNER_IMAGE_KIRBY
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_KIRBY)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeKirbyLowImage *img_ =
                (const NDSNativeKirbyLowImage *)slot->base;
            NDS_NATIVE_IMAGE_KIRBY_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_KIRBY_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeKirbyHighImage *img_ =
                (const NDSNativeKirbyHighImage *)slot->base;
            NDS_NATIVE_IMAGE_KIRBY_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_KIRBY_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
#if NDS_P2_MMARIO && !NDS_NATIVE_OWNER_IMAGE_MMARIO
    if (owner_slot == NDS_NATIVE_IMAGE_SLOT_MMARIO)
    {
        if (use_low_detail != 0u)
        {
            const NDSNativeMMarioLowImage *img_ =
                (const NDSNativeMMarioLowImage *)slot->base;
            NDS_NATIVE_IMAGE_MMARIO_LOW_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_MMARIO_LOW_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
        else
        {
            const NDSNativeMMarioHighImage *img_ =
                (const NDSNativeMMarioHighImage *)slot->base;
            NDS_NATIVE_IMAGE_MMARIO_HIGH_MEMBERS(NDS_IMG_VERIFY)
            NDS_NATIVE_IMAGE_MMARIO_HIGH_MEMBERS_DENSE_NORMALS(NDS_IMG_VERIFY_NORMALS)
        }
    }
#endif
    return (gNdsNativeOwnerImageMismatchCount == before) ? TRUE : FALSE;
}
#endif /* NDS_NATIVE_OWNER_IMAGE_VERIFY */

#endif /* P2-3 image-backed owners */

static const NDSNativeFighterOwnerRuntime *
ndsRendererNativeFighterOwnerForDetail(u32 slot, u32 use_low_detail)
{
    if (slot == 0u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeMarioLowOwner : &sNdsNativeMarioHighOwner;
    }
    if (slot == 1u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeFoxLowOwner : &sNdsNativeFoxHighOwner;
    }
#if NDS_P2_LUIGI
    if (slot == 2u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeLuigiLowOwner : &sNdsNativeLuigiHighOwner;
    }
#endif
#if NDS_P2_DONKEY
    if (slot == 3u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeDonkeyLowOwner : &sNdsNativeDonkeyHighOwner;
    }
#endif
#if NDS_P2_CAPTAIN
    if (slot == 4u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeCaptainLowOwner : &sNdsNativeCaptainHighOwner;
    }
#endif
#if NDS_P2_SAMUS
    if (slot == 5u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeSamusLowOwner : &sNdsNativeSamusHighOwner;
    }
#endif
#if NDS_P2_LINK
    if (slot == 6u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeLinkLowOwner : &sNdsNativeLinkHighOwner;
    }
#endif
#if NDS_P2_PIKACHU
    if (slot == 7u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativePikachuLowOwner : &sNdsNativePikachuHighOwner;
    }
#endif
#if NDS_P2_YOSHI
    if (slot == 8u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeYoshiLowOwner : &sNdsNativeYoshiHighOwner;
    }
#endif
#if NDS_P2_NESS
    if (slot == 9u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeNessLowOwner : &sNdsNativeNessHighOwner;
    }
#endif
#if NDS_P2_PURIN
    if (slot == 10u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativePurinLowOwner : &sNdsNativePurinHighOwner;
    }
#endif
#if NDS_P2_KIRBY
    if (slot == 11u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeKirbyLowOwner : &sNdsNativeKirbyHighOwner;
    }
#endif
#if NDS_P2_MMARIO
    if (slot == 12u)
    {
        return (use_low_detail != 0u) ?
            &sNdsNativeMMarioLowOwner : &sNdsNativeMMarioHighOwner;
    }
#endif
    return NULL;
}

/* Resolve the exact executable native root for one logical JointTree binding.
 * Canonical roots are the common case.  BattleShip passive model parts keep
 * the same DObj/joint matrix but replace its DL, so only an explicitly
 * generated `(binding, root_offset)` pair may select a variant.  Unknown
 * offsets remain a hard decline to the caller; never reinterpret arbitrary
 * fighter DLs as one of these source-qualified programs. */
static const NDSNativeRoot *ndsRendererNativeFighterResolveRoot(
    const NDSNativeFighterOwnerRuntime *owner,
    u32 slot,
    u32 use_low_detail,
    u32 binding,
    u32 root_offset)
{
    const NDSNativeRootVariant *variants = NULL;
    u32 variant_count = 0u;
    u32 i;

    if ((owner == NULL) || (binding >= owner->root_count))
    {
        return NULL;
    }
    if (owner->roots[binding].root_offset == root_offset)
    {
        return &owner->roots[binding];
    }
    /* BattleShip's Fox Results Lose motion (scsubsysdatafox.c) switches model
     * part 1 onto joints 10 and 16.  The generated rows below are the complete
     * source DL programs for those replacements at their original logical
     * bindings; unknown Fox offsets still fail closed. */
    if (slot == 1u)
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeFoxRootVariantsLow : sNdsNativeFoxRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeFoxRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeFoxRootVariants);
    }
#if NDS_P2_DONKEY
    if (slot == ((u32)NDS_RENDERER_PROFILE_OWNER_DONKEY - 1u))
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeDonkeyRootVariantsLow : sNdsNativeDonkeyRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeDonkeyRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeDonkeyRootVariants);
    }
#endif
#if NDS_P2_CAPTAIN
    if (slot == ((u32)NDS_RENDERER_PROFILE_OWNER_CAPTAIN - 1u))
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeCaptainRootVariantsLow : sNdsNativeCaptainRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeCaptainRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeCaptainRootVariants);
    }
#endif
#if NDS_P2_SAMUS
    if (slot == ((u32)NDS_RENDERER_PROFILE_OWNER_SAMUS - 1u))
    {
        variants = (use_low_detail != 0u) ?
            sNdsNativeSamusRootVariantsLow : sNdsNativeSamusRootVariants;
        variant_count = (use_low_detail != 0u) ?
            NDS_FTR_COUNT(sNdsNativeSamusRootVariantsLow) :
            NDS_FTR_COUNT(sNdsNativeSamusRootVariants);
    }
#endif
    for (i = 0u; i < variant_count; i++)
    {
        if ((variants[i].binding == binding) &&
            (variants[i].root.root_offset == root_offset))
        {
            return &variants[i].root;
        }
    }
    return NULL;
}

#undef NDS_FTR_COUNT
#endif

#if NDS_TASK93_TEXKEY_CENSUS
/* Task 93 E0. Sizes the texture-key rebuild in
 * ndsRendererHardwareResolveOrBindTexture, the largest renderer symbol left in
 * the frame. 256 entries covers several frames at the expected bind rate. */
#define NDS_TASK93_KEY_TRACE_COUNT 256u
u32 gNdsTask93BindCalls;
u32 gNdsTask93PreflightCalls;
u32 gNdsTask93ConsecutiveRepeat;
u32 gNdsTask93LastKeyHash;
u32 gNdsTask93KeyTrace[NDS_TASK93_KEY_TRACE_COUNT];
u32 gNdsTask93KeyTraceNext;
#endif

#if NDS_TASK107_RENDER_STATE_CENSUS
/* Task 107. Observation only: these counters size two renderer-state candidates
 * before any production path is changed. The stats tracker compares exact tile
 * source bytes against the previous sync of that SAME stats object/tile. A
 * stats init retires the object's history, so stack-address reuse cannot create
 * a false repeat. The load tile's set_seen bit is tracked separately because it
 * contributes NDS_RENDERER_TILE_LOAD_SEEN to the published flags word. */
#define NDS_TASK107_SYNC_SITE_COUNT 4u
#define NDS_TASK107_SYNC_TRACK_COUNT 32u
#define NDS_TASK107_BIND_NAME_CAPACITY 64u
enum
{
    NDS_TASK107_SYNC_TEXTURE = 0,
    NDS_TASK107_SYNC_SETTILE = 1,
    NDS_TASK107_SYNC_SETTILESIZE = 2,
    NDS_TASK107_SYNC_RESOLVE = 3
};
typedef struct NDSRendererTask107SyncTrack
{
    const NDSRendererStats *stats;
    u32 valid_mask;
    u32 load_seen[NDS_RENDERER_TILE_COUNT];
    NDSRendererTileState tile[NDS_RENDERER_TILE_COUNT];
} NDSRendererTask107SyncTrack;

__attribute__((used)) volatile u32
    gNdsTask107SyncCalls[NDS_TASK107_SYNC_SITE_COUNT];
__attribute__((used)) volatile u32
    gNdsTask107SyncUnchanged[NDS_TASK107_SYNC_SITE_COUNT];
__attribute__((used)) volatile u32 gNdsTask107SyncTrackerOverflow;
__attribute__((used)) volatile u32 gNdsTask107BindRequests;
__attribute__((used)) volatile u32 gNdsTask107BindZeroNameExits;
__attribute__((used)) volatile u32 gNdsTask107BindCurrentNameElisions;
__attribute__((used)) volatile u32 gNdsTask107BindIssues;
__attribute__((used)) volatile u32 gNdsTask107BindRevisitIssues;
__attribute__((used)) volatile u32 gNdsTask107BindNameSetOverflow;

static NDSRendererTask107SyncTrack
    sNdsTask107SyncTrack[NDS_TASK107_SYNC_TRACK_COUNT];
static u32 sNdsTask107BindFrameSerial;
static u32 sNdsTask107BindNameCount;
static u32 sNdsTask107BindNames[NDS_TASK107_BIND_NAME_CAPACITY];
#endif

#if NDS_R2_TILESYNC_ROUTE
/* Lab SAME-BINARY route for the tile-sync memo. One `.data` word selects
 * whether the proven-redundant republish is skipped; both arms evaluate the
 * predicate and both keep texture_tile_sync_serial in step, so the ONLY
 * difference between them is the 19-field store burst, and the two counters
 * below must read IDENTICALLY on both arms -- that equality is the control.
 *
 * .data AND NOT .bss, for the reason nds_r2_sqrtf.c states: a zero-initialised
 * route word without an explicit section attribute lands in .bss and drags a
 * ~10,000 tk/fr placement floor. */
volatile u32 gNdsR2TileSyncRoute
    __attribute__((used, section(".data"))) = 1u;
__attribute__((used)) volatile u32 gNdsR2TileSyncSkips;
__attribute__((used)) volatile u32 gNdsR2TileSyncRuns;
#define NDS_R2_TILESYNC_MEMO_ON() (gNdsR2TileSyncRoute != 0u)
#define NDS_R2_TILESYNC_COUNT_SKIP() (gNdsR2TileSyncSkips++)
#define NDS_R2_TILESYNC_COUNT_RUN() (gNdsR2TileSyncRuns++)
#else
#define NDS_R2_TILESYNC_MEMO_ON() (1)
#define NDS_R2_TILESYNC_COUNT_SKIP() ((void)0)
#define NDS_R2_TILESYNC_COUNT_RUN() ((void)0)
#endif

#if NDS_TASK90_SHADE_CENSUS
/* Task 90. The instrument behind NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT: it
 * records every light-shade request in order, so the cache size is set from the
 * measured working set rather than from a hit rate. Keep it alive -- the size
 * constant's comment names scripts/census-light-shade-lut.ps1 as the
 * re-check when the fighter or stage set changes, and that script reads these.
 * Lab only, default off; the trace array is 1 KiB of BSS.
 *
 * 128 entries covers ~2.6 frames at the measured 49 requests/frame. */
#define NDS_TASK90_LUT_TRACE_COUNT 128u
u32 gNdsTask90LutGetCalls;
u32 gNdsTask90LutBuilds;
u32 gNdsTask90LutTraceDiffuse[NDS_TASK90_LUT_TRACE_COUNT];
u32 gNdsTask90LutTraceAmbient[NDS_TASK90_LUT_TRACE_COUNT];
u32 gNdsTask90LutTraceNext;
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
#define NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX 27u
#define NDS_NATIVE_FIGHTER_HIERARCHY_BINDING_MAX 18u
#define NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT 49u

typedef struct NDSNativeMatrix3x3
{
    s32 m[3][3];
} NDSNativeMatrix3x3;

typedef struct NDSNativeHierarchyPreparedEpoch
{
    NDSRendererHardwareLightDirection light_direction;
    u32 light_direction_valid;
} NDSNativeHierarchyPreparedEpoch;

typedef struct NDSNativeHierarchyPreparedRun
{
    NDSRendererHardwareTextureCacheEntry *texture_entry;
    u32 texture_name;
    u32 texture_params;
    u32 texture_format;
    u32 texture_width;
    u32 texture_height;
    u32 poly_fmt;
    u32 scale_s;
    u32 scale_t;
    u32 origin_s;
    u32 origin_t;
    s32 texture_offset;
    u32 vertex_flags;
    u32 textured;
} NDSNativeHierarchyPreparedRun;

typedef struct NDSNativeFighterOwnerExecution
{
    NDSRendererTraversalState traversal;
    NDSRendererStats preflight_stats;
    NDSNativeMatrix3x3 hierarchy_world[
        NDS_NATIVE_FIGHTER_HIERARCHY_JOINT_MAX];
    NDSNativeHierarchyPreparedEpoch hierarchy_epochs[
        NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
    NDSNativeHierarchyPreparedRun hierarchy_runs[
        NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
    NDSRendererStats *stats;
    NDSRendererVertexCache *vertex_cache;
    u32 slot;
    /* Battle player slot (0..3), distinct from the generated owner slot above.
     * The production run texture memo needs this identity because two live
     * instances of the same fighter execute the same generated run indices but
     * can carry different source materials/costumes.  `appearance_id` is the
     * source costume/shade pair for the same reason: CSS reuses one live player
     * slot while changing costumes, so instance identity alone is not enough.
     * Owner execution is serialized, just like sNdsNativeFighterActiveTables,
     * so current values keep the hot run helper ABI unchanged. */
    /* One hot-path identity word for the run-texture memo.  The resolver's
     * result can vary by generated owner, high/low detail program, live fighter
     * instance, costume and shade.  Packing those facts once at owner entry is
     * materially cheaper than re-reading four independent fences for every run
     * (four fighters execute this path thousands of times per gate match). */
    u32 texture_memo_owner_key;
    u32 active;
} NDSNativeFighterOwnerExecution;

#if NDS_RENDERER_M2_DETAILED_LEDGER
typedef struct NDSNativeFighterShadeKey
{
    u32 owner_generation;
    u32 slot;
    u32 epoch_index;
    u32 epoch_policy;
    u32 combine_w0;
    u32 combine_w1;
    u32 policy_flags;
    u32 geometry_mode;
    u32 prim_color;
    u32 light_color_1;
    u32 light_color_2;
    u32 light_masks;
    s32 light_dir_x;
    s32 light_dir_y;
    s32 light_dir_z;
    u32 light_dir_valid;
} NDSNativeFighterShadeKey;

typedef struct NDSNativeFighterShadeCensusEntry
{
    NDSNativeFighterShadeKey key;
    u32 hash;
    u32 valid;
} NDSNativeFighterShadeCensusEntry;

static NDSNativeFighterShadeCensusEntry sNdsNativeFighterShadeCensus[2][
    NDS_NATIVE_FIGHTER_HIERARCHY_EPOCH_COUNT];
static u32 sNdsNativeFighterShadeProducerGeneration[
    sizeof(sNdsNativeFighterDenseVertices) /
        sizeof(sNdsNativeFighterDenseVertices[0])];
static u8 sNdsNativeFighterShadeProducerTag[
    sizeof(sNdsNativeFighterDenseVertices) /
        sizeof(sNdsNativeFighterDenseVertices[0])];

static u32 ndsRendererM2ShadeHash(
    const NDSNativeFighterShadeKey *key)
{
    const u32 *word = (const u32 *)key;
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < sizeof(*key) / sizeof(*word); i++)
    {
        u32 value = word[i];
        u32 byte;

        for (byte = 0u; byte < sizeof(value); byte++)
        {
            hash ^= (value >> (byte * 8u)) & 0xffu;
            hash *= 16777619u;
        }
    }
    return (hash != 0u) ? hash : 1u;
}

static s32 ndsRendererM2ShadeOutputsResident(
    u32 slot,
    u32 epoch_index,
    u32 owner_generation,
    const NDSNativeEpoch *epoch)
{
    u8 producer_tag = (u8)((slot << 7) | (epoch_index + 1u));
    u32 action_offset;

    for (action_offset = 0u;
         action_offset < epoch->action_count;
         action_offset++)
    {
        u32 action_index = epoch->first_action + action_offset;
        u32 span = sNdsNativeFighterActiveTables->action_dense_spans[action_index];
        u32 dense_first = span & NDS_NATIVE_DENSE_ID_MASK;
        u32 dense_count = span >> NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT;
        u32 dense_offset;

        for (dense_offset = 0u;
             dense_offset < dense_count;
             dense_offset++)
        {
            u32 dense_id = dense_first + dense_offset;

            if ((sNdsNativeFighterShadeProducerGeneration[dense_id] !=
                 owner_generation) ||
                (sNdsNativeFighterShadeProducerTag[dense_id] !=
                 producer_tag))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static void __attribute__((noinline)) ndsRendererM2ShadeCensusEpoch(
    u32 slot,
    u32 owner_generation,
    u32 epoch_index,
    const NDSNativeEpoch *epoch,
    const NDSRendererHardwareLightDirection *prepared_direction,
    u32 prepared_direction_valid,
    const NDSRendererStats *stats)
{
    const u32 epoch_policy =
        sNdsNativeFighterActiveTables->epoch_direct_policy[epoch_index];
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    NDSNativeFighterShadeCensusEntry *entry =
        &sNdsNativeFighterShadeCensus[slot][epoch_index];
    NDSNativeFighterShadeKey key = {
        owner_generation,
        slot,
        epoch_index,
        epoch_policy,
        policy->combine_w0,
        policy->combine_w1,
        (u32)policy->vertex_flags | ((u32)policy->textured << 8),
        stats->geometry_mode,
        stats->prim_color,
        stats->light_color_1,
        stats->light_color_2,
        stats->light_color_mask | (stats->light_dir_mask << 16),
        (prepared_direction != NULL) ? prepared_direction->x : 0,
        (prepared_direction != NULL) ? prepared_direction->y : 0,
        (prepared_direction != NULL) ? prepared_direction->z : 0,
        prepared_direction_valid
    };
    u32 hash;

    sNdsRendererM2ShadeEpochCount++;
    sNdsRendererM2ShadeOwnerEpochCount[slot]++;
    if (epoch->action_count == 0u)
    {
        return;
    }
    hash = ndsRendererM2ShadeHash(&key);
    if ((entry->valid != 0u) && (entry->hash == hash))
    {
        if (memcmp(&entry->key, &key, sizeof(key)) == 0)
        {
            sNdsRendererM2ShadeKeyHitCount++;
            sNdsRendererM2ShadeOwnerKeyHitCount[slot]++;
            if (ndsRendererM2ShadeOutputsResident(
                    slot, epoch_index, owner_generation, epoch) != FALSE)
            {
                sNdsRendererM2ShadeResidentHitCount++;
                sNdsRendererM2ShadeOwnerResidentHitCount[slot]++;
            }
        }
        else
        {
            sNdsRendererM2ShadeHashCollisionCount++;
        }
    }
    entry->key = key;
    entry->hash = hash;
    entry->valid = TRUE;
}

static void __attribute__((noinline)) ndsRendererM2ShadeRecordProduced(
    u32 slot,
    u32 owner_generation,
    u32 epoch_index,
    const NDSNativeEpoch *epoch,
    u32 epoch_policy,
    const NDSRendererStats *stats)
{
    const NDSNativeDirectPolicy *policy =
        &sNdsNativeFighterDirectPolicies[
            epoch_policy & NDS_NATIVE_DIRECT_POLICY_FAMILY_MASK];
    u32 use_material =
        policy->vertex_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    u32 use_lut =
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u) &&
        ((stats->light_color_mask &
          (NDS_RENDERER_LIGHT_COLOR_1_MASK |
           NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
         (NDS_RENDERER_LIGHT_COLOR_1_MASK |
          NDS_RENDERER_LIGHT_COLOR_2_MASK));
    u32 action_offset;

    for (action_offset = 0u;
         action_offset < epoch->action_count;
         action_offset++)
    {
        u32 action_index = epoch->first_action + action_offset;
        u32 span = sNdsNativeFighterActiveTables->action_dense_spans[action_index];
        u32 dense_first = span & NDS_NATIVE_DENSE_ID_MASK;
        u32 dense_count = span >> NDS_NATIVE_DENSE_SPAN_COUNT_SHIFT;
        u32 dense_offset;

        sNdsRendererM2ShadeDenseVisitCount += dense_count;
        if (use_material != 0u)
        {
            sNdsRendererM2ShadeMaterialPackCount += dense_count;
        }
        for (dense_offset = 0u;
             dense_offset < dense_count;
             dense_offset++)
        {
            u32 dense_id = dense_first + dense_offset;

            if (sNdsNativeFighterActiveTables->dense_color_source[dense_id] !=
                dense_id)
            {
                sNdsRendererM2ShadeAliasCopyCount++;
            }
            else
            {
                sNdsRendererM2ShadeComputeCount++;
                if (use_lut != 0u)
                {
                    sNdsRendererM2ShadeLutComputeCount++;
                }
                else
                {
                    sNdsRendererM2ShadePreparedComputeCount++;
                }
            }
            sNdsNativeFighterShadeProducerGeneration[dense_id] =
                owner_generation;
            sNdsNativeFighterShadeProducerTag[dense_id] =
                (u8)((slot << 7) | (epoch_index + 1u));
        }
    }
}
#endif

typedef struct NDSNativeStagePreparedDense
{
    u16 packed_color;
    s16 s;
    s16 t;
    s16 near_inside;
} NDSNativeStagePreparedDense;

typedef struct NDSRendererProjectedClipVertex
{
    NDSRendererClipVertex20p12 clip;
    s32 s;
    s32 t;
    u16 packed_color;
} NDSRendererProjectedClipVertex;

typedef struct NDSNativeStagePreparedRun
{
    NDSRendererHardwareTextureCacheEntry *texture_entry;
    u32 texture_name;
    u32 texture_params;
    u32 texture_generation;
    u32 poly_fmt;
    u16 texture_width;
    u16 texture_height;
    u8 texture_format;
    u8 textured;
    u8 alpha_test;
    u8 alpha_ref;
} NDSNativeStagePreparedRun;

static s32 ndsRendererNativeStagePreparedTextureValid(
    const NDSNativeStagePreparedRun *prepared)
{
    const NDSRendererHardwareTextureCacheEntry *entry;

    if (prepared == NULL)
    {
        return FALSE;
    }
    entry = prepared->texture_entry;
    if (prepared->textured == FALSE)
    {
        return ((entry == NULL) && (prepared->texture_name == 0u) &&
                (prepared->texture_generation == 0u)) ? TRUE : FALSE;
    }
    return ((entry != NULL) && (entry->ready != FALSE) &&
            ((u32)entry->name == prepared->texture_name) &&
            (entry->key_generation == prepared->texture_generation)) ?
        TRUE : FALSE;
}

typedef struct NDSNativeStageOwnerExecution
{
    NDSRendererTraversalState traversal;
    NDSRendererStats preflight_stats;
    NDSNativeStagePreparedRun runs[NDS_NATIVE_STAGE_MAX_RUN_COUNT];
    const NDSRendererMatrix20p12 *binding_composed;
#if NDS_TASK36_HW_COMPOSE
    const NDSRendererMatrix20p12 *projection;
    const NDSRendererMatrix20p12 *camera_modelview;
    const NDSRendererMatrix20p12 *binding_world;
    u64 rigid_binding_mask;
    u64 task36_seen_binding_mask;
    u32 task36_binding;
    u32 task36_coordinate_shift;
    u32 task36_local_pushed;
    u32 task36_segment_active;
#endif
    NDSRendererStats *stats;
    u32 next_segment;
    u32 active;
#if NDS_R2_STAGE_DIRECT
    /* R2-02 E1a. `runs[]` is a pure function of the generated run/epoch/policy
     * tables and the traversal state, and Task 44 already proves the traversal
     * state unchanged every frame. Once built it is reusable, so the whole
     * PrepareRun phase can leave the frame.
     *
     * Safe with respect to texture binding: ndsRendererNativeStageBeginRun
     * binds from this record (`texture_name`, `texture_params`,
     * `texture_entry`) and sets `last_used_frame` itself, so the resolve inside
     * PrepareRun is a producer of the record rather than a side effect the
     * commit path depends on.
     *
     * Keyed on the frame config and asset bases so a scene reload cannot reuse
     * a table built against different assets -- and, since R2-07 E2, on the
     * topology generation and stamp as well.
     *
     * The original key assumed "the topology validation and Task 44's
     * generation compare should both reject first". Neither did. Re-entering
     * the SAME stage keeps the config POINTER and every asset base identical
     * (the bump allocator hands back the same addresses in the same order), so
     * the key could not see a scene boundary at all: E2 measured
     * PrepareBuildCount frozen at 2 across the Sudden Death entry while
     * PrepareReuseCount ran 195 -> 303, i.e. the second scene drew its stage
     * from run data prepared for the first. That is the whole second-entry
     * corruption -- the routing arms agree, replay correct / reuse corrupt /
     * rebuild-from-source correct.
     *
     * topology_generation + topology_stamp are the pair the Task 36 replay
     * owner already keys on, and that owner resets correctly across the entry;
     * reusing them here makes the two caches invalidate together rather than
     * leaving this one keyed on quantities a re-entry cannot move. */
    const NDSRendererConfig *r2_prepared_config;
    const void *r2_prepared_asset_bases[NDS_RENDERER_NATIVE_STAGE_ASSET_COUNT];
    u32 r2_prepared_topology_generation;
    u32 r2_prepared_topology_stamp;
    u64 r2_prepared_epoch_mask;
    u32 r2_prepared_valid;
    /* R2-02 E8. The one member of `preflight_stats` that outlives the segment
     * loop. Memoised with `epoch_mask` so eliding the loop body cannot depend
     * on which segment happens to run last. */
    u32 r2_prepared_sync_count;
#endif
} NDSNativeStageOwnerExecution;

typedef struct NDSNativeStageTopologySummary
{
    u32 raw_triangles;
    u32 projected_no_z_triangles;
    u32 projected_range_triangles;
    u32 cross_runs;
    u32 cross_triangles;
    u32 cross_foreign_corners;
} NDSNativeStageTopologySummary;

typedef struct NDSNativeStageValidationCache
{
    NDSNativeStageTopologySummary summary;
    u16 prepared_dense_offsets[NDS_NATIVE_STAGE_MAX_RUN_COUNT + 1u];
    u16 prepared_dense_indices[NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT];
    u32 generation;
    u32 stamp;
    u32 valid;
} NDSNativeStageValidationCache;

/* Stage segments straddle the fighter display links, so their accepted
 * preflight must survive the two complete fighter-owner submissions. */
static NDSNativeFighterOwnerExecution sNdsNativeFighterOwnerExecution;
static NDSNativeStageOwnerExecution sNdsNativeStageOwnerExecution;
/* R2-07 leg A. The prepared run table was re-proved against the texture cache
 * ~195 times a frame (54 runs x the reuse key, the commit gate and the replay
 * gate, plus BeginRun's per-run last gate), and the c123 per-line attribution
 * says that cost is not the compares: 5.34 cycles per instruction, i.e. two
 * cold arrays -- runs[] and then the cache entries it names -- dragged through
 * a 4 KB D-cache every frame. Slice 44's shape exactly, whose lever was "stop
 * touching the objects".
 *
 * So prove it where the invariant breaks (slice 30) rather than where it is
 * read. sNdsRendererHardwareTextureKeyGeneration is already stamped by every
 * (re-)key, and ndsRendererHardwareReleaseTexture now stamps it as well -- that
 * function is the ONE seam that destroys an entry's identity (Evict, Alloc's
 * recycle, Discard and the static teardown all route through it, and its memset
 * is what clears `ready` and `name`). Nothing else writes entry->ready,
 * entry->name or entry->key_generation: names are only ever produced by
 * glGenTextures into a slot whose name is already 0, and ready is only ever set
 * TRUE beside a key stamp. The epoch is therefore a COMPLETE invalidation event
 * for the (entry, ready, name, key_generation) quadruple the proof reads, and a
 * table proved at epoch E is still proved for as long as the epoch reads E.
 *
 * Only the POSITIVE answer is cached. A failing table becomes valid again by
 * being rebuilt, which is not an epoch event, so a miss re-sweeps every frame
 * exactly as before; and the rebuild path clears the stamp so a fresh table is
 * never covered by its predecessor's proof.
 *
 * Measured basis for the design, from the same profile: the cache is static
 * across a match -- ndsRendererHardwareAllocTexture executes 3 times in the
 * whole 1,600-frame run and Release/Evict/Discard do not appear at all -- so
 * the epoch is expected to be motionless mid-match and the fast path is the
 * path. A scene boundary is the opposite case and is covered: the battle
 * prepare discards the whole cache, which releases every entry and therefore
 * moves the epoch, and the topology generation/stamp key rebuilds the table
 * independently (R2-07 E2). */
static u32 sNdsNativeStagePreparedTextureProofEpoch;
static u32 sNdsNativeStagePreparedTextureProofValid;
/* The Task 36 replay owner keeps its OWN copy of each prepared run, so it needs
 * its own certificate: proving the stage table says nothing about the replay
 * table, and conflating them is how a proof covers a structure it never read. */
static u32 sNdsTask36ReplayTextureProofEpoch;
static u32 sNdsTask36ReplayTextureProofValid;

/* Engagement, both sides, read with -ExtraGlobals from the same run that
 * produces the buckets. Predicted on the gate arm: Fast ~= one per consult per
 * frame, Sweep ~= 0 after the first, EpochBump ~= 0 mid-match. A per-frame
 * Sweep count means the epoch is moving and this lever did not engage. */
__attribute__((used)) volatile u32 gNdsR2TexProofFastCount;
__attribute__((used)) volatile u32 gNdsR2TexProofSweepCount;
__attribute__((used)) volatile u32 gNdsR2TexProofSweepFailCount;
__attribute__((used)) volatile u32 gNdsR2TextureEpochBumpCount;

static void ndsRendererNativeStagePreparedTextureProofDrop(void)
{
    sNdsNativeStagePreparedTextureProofValid = FALSE;
}

/* Pure reads of the two certificates -- "is the standing proof still current",
 * with no sweep behind them. These are what let the point-of-use gate stop
 * touching the objects: its caller has already proved the table this run
 * belongs to, so re-reading prepared[] and the cache entry can only reach the
 * same answer. When no certificate is current the original per-run proof runs
 * unchanged, so the gate never weakens -- it is skipped only while something
 * else has proved the whole table at the current epoch. */
static s32 ndsRendererNativeStagePreparedTextureProofCurrent(void)
{
    return ((sNdsNativeStagePreparedTextureProofValid != FALSE) &&
            (sNdsNativeStagePreparedTextureProofEpoch ==
             sNdsRendererHardwareTextureKeyGeneration)) ? TRUE : FALSE;
}

static s32 ndsRendererTask36ReplayTextureProofCurrent(void)
{
    return ((sNdsTask36ReplayTextureProofValid != FALSE) &&
            (sNdsTask36ReplayTextureProofEpoch ==
             sNdsRendererHardwareTextureKeyGeneration)) ? TRUE : FALSE;
}

static s32 ndsRendererNativeStagePreparedTexturesProven(void)
{
    u32 run_index;

    if ((sNdsNativeStagePreparedTextureProofValid != FALSE) &&
        (sNdsNativeStagePreparedTextureProofEpoch ==
         sNdsRendererHardwareTextureKeyGeneration))
    {
        gNdsR2TexProofFastCount++;
        return TRUE;
    }
    gNdsR2TexProofSweepCount++;
    for (run_index = 0u; run_index < NDS_NATIVE_STAGE_RUN_COUNT; run_index++)
    {
        if (ndsRendererNativeStagePreparedTextureValid(
                &sNdsNativeStageOwnerExecution.runs[run_index]) == FALSE)
        {
            sNdsNativeStagePreparedTextureProofValid = FALSE;
            gNdsR2TexProofSweepFailCount++;
            return FALSE;
        }
    }
    sNdsNativeStagePreparedTextureProofEpoch =
        sNdsRendererHardwareTextureKeyGeneration;
    sNdsNativeStagePreparedTextureProofValid = TRUE;
    return TRUE;
}
#if NDS_R2_STAGE_DIRECT
/* R2-02 E1a engagement counters. Non-static so a GDB stop can read them and
 * prove the elision actually engaged -- a flag that silently never fires is
 * indistinguishable from a null result, and this campaign has shipped that
 * mistake (Task 52 found the Task 36 replay structurally disabled). */
volatile u32 gNdsR2StagePrepareReuseCount;
volatile u32 gNdsR2StagePrepareBuildCount;
#if NDS_R2_STAGE_ROUTE_PROBE
/* Which of the five reuse-key components missed, COUNTED rather than latched.
 * See the write site in ndsRendererPrepareNativeStageOwner. Measured
 * 2026-08-01 with NDS_R2_PARTICLE_DRAW=1: Invalid 197, every other 0 -- so the
 * topology, config and asset bases never move and all 197 rebuilds are the
 * PREVIOUS frame's owner having rejected. Which is what the six counters below
 * then have to name. */
volatile u32 gNdsR2StageKeyMissInvalid;
volatile u32 gNdsR2StageKeyMissGeneration;
volatile u32 gNdsR2StageKeyMissStamp;
volatile u32 gNdsR2StageKeyMissConfig;
volatile u32 gNdsR2StageKeyMissAssets;
/* One per PrepareRun refusal site, indexed by the same reason code the latch
 * carries. The LATCH is reset at the top of every prepare, so an end-of-run
 * read reports the last frame -- both reason words read 0 on a run whose
 * battle rebuilt 197 times, which is the "a counter identical before and after
 * the window is not evidence about the window" rule in its purest form. */
volatile u32 gNdsR2StageRejectCounts[7];
#endif
#if NDS_R2_STAGE_PREFLIGHT && (NDS_TASK36_HW_COMPOSE == 2)
/* R2-02 E8 engagement, for the same reason: five per frame is the elision
 * working, zero is a flag that compiled but never fired. */
volatile u32 gNdsR2StagePreflightElideCount;
#endif
#endif
static NDSNativeStagePreparedDense sNdsNativeStagePreparedDense[
    NDS_NATIVE_STAGE_MAX_DENSE_VERTEX_COUNT];
static NDSNativeStageValidationCache sNdsNativeStageValidationCache;
#if NDS_TASK36_HW_COMPOSE == 2
/* Segments 5 (layer2) and 7 (layer3) have only rigid affine bindings. Layer0
 * also contains source MVP-recalc nodes and must retain its live camera rows.
 * Every replayed binding must be in the effective rigid mask. That equality is
 * the contract, not a coincidence: a rigid binding's captured stream is PUSH +
 * MULT4x4 of a constant world under the camera the segment bracket loads live
 * each frame, so it replays; a dynamic binding's stream is a LOAD4x4 per
 * triangle of projection x view x model, so replaying it pins that geometry to
 * the camera the capture frame happened to have.
 *
 * R2-02 E3 widened this to Whispy's two segments and the two flower segments
 * without widening the rigid mask, and those four duly stayed where the capture
 * frame's camera had left them -- a smear of specks across the trunk. It read as
 * -51,200 ticks of stage time that was never saved. E4 then widened the rigid
 * mask to match and lost the flower beds entirely. Both arms are in
 * docs/optimization/ClaudeOpus5_R202_E4_ActorSegmentsRefuted_20260728.md; the
 * next attempt starts in the generator, not here. */
#define NDS_TASK36_REPLAY_WORD_CAPACITY 4608u
/* This captured GX program is a Dream Land specialization. Other packets
 * execute their native runs live; their segment numbers and animated roots
 * cannot inherit Dream Land's replay slots. */
#define NDS_TASK36_REPLAY_SEGMENT_MASK \
    ((sNdsNativeStagePacketActive->gkind == NDS_NATIVE_STAGE_GKIND_PUPUPU) ? \
        ((1u << 5u) | (1u << 7u)) : 0u)

typedef enum NDSRendererTask36ReplayState
{
    NDS_TASK36_REPLAY_UNSEEDED = 0,
    NDS_TASK36_REPLAY_CAPTURING,
    NDS_TASK36_REPLAY_READY,
    NDS_TASK36_REPLAY_DISABLED
} NDSRendererTask36ReplayState;

typedef struct NDSRendererTask36ReplayRun
{
    NDSNativeStagePreparedRun prepared;
    u16 word_offset;
    u16 word_count;
    u8 valid;
    u8 world_mult_count;
    /* R2-02 E3: does this run's stream leave the modelview stack one push deep?
     * The rigid runs do -- Task 36's EnsureWorld records PUSH + MULT before the
     * vertices, so the segment's EndSegment owes a POP. The actor runs
     * (whispy_eyes, whispy_mouth, flowers_back, flowers_front) do not: their
     * generic emit records LOAD4x4 per triangle and never pushes. Replay used to
     * assert task36_local_pushed = TRUE for every run, so admitting the actor
     * segments to the mask bought four unmatched glPopMatrix(1) calls a frame. */
    u8 local_pushed;
    u8 reserved[1];
} NDSRendererTask36ReplayRun;

typedef struct NDSRendererTask36ReplayOwner
{
    u32 words[NDS_TASK36_REPLAY_WORD_CAPACITY] __attribute__((aligned(32)));
    NDSRendererTask36ReplayRun runs[NDS_NATIVE_STAGE_MAX_RUN_COUNT];
    NDSRendererStats segment_stats[NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT];
    NDSRendererConfig config;
    u64 segment_epoch_mask[NDS_NATIVE_STAGE_MAX_SEGMENT_COUNT];
    u32 topology_generation;
    u32 topology_stamp;
    u32 word_count;
    u32 command_word_index;
    u32 command_slot;
    u32 current_run;
    u32 captured_segment_mask;
    /* Task 44 item 2: capture_active moved out to sNdsRendererTask36CaptureActive
     * so the wrapped GX sites can test it without reaching into this owner. */
    u32 capture_fault;
    /* R2-02 E3: signed PUSH/POP balance of the run being captured, so
     * CaptureEndRun can record whether the stream owes the segment a POP
     * instead of replay assuming every run does. */
    s32 capture_push_balance;
    u32 frame_capture;
    u32 frame_replay;
    /* BUGS.md #9. The capture bracket is per RUN
     * (ndsRendererTask36ReplayCaptureBeginRun/EndRun), and every rigid
     * PROJECTED_NO_Z run calls ndsRendererNativeStageTask36LoadNoZProjection
     * inside it -- so the capture frame's PROJECTION matrix is baked into the
     * word stream. Only the camera modelview is reloaded live, by
     * Task36BeginSegment, which sits outside the bracket. That is invisible
     * during a match because fovy is pinned at 38.0, but the paused
     * player-zoom camera calls gmCameraAdjustFOV(pzoom_fov)
     * (decomp gm/gmcamera.c:713 -> :614), so the live projection moves while
     * the replayed rigid slabs keep the stale one and the middle slab --
     * binding 29, which is not in a replayed segment -- keeps the live one.
     * The floor then reads as the front and rear slabs sitting at a different
     * height from the middle path.
     *
     * The existing staleness guards cover materials, textures and topology;
     * this adds the projection to that set. On a mismatch replay is declined
     * for the frame and the live path runs, which is what the whole ROM does
     * at NDS_TASK36_HW_COMPOSE=1 -- both arms of the 2026-08-09 A/B pair were
     * clean there. Declining rather than re-capturing is deliberate: an orbit
     * moves the FOV every frame, and re-capturing each time would thrash the
     * arena for a screen that is already paused. Gameplay is unaffected --
     * a constant fovy means this compare never fires during a match. */
    NDSRendererMatrix20p12 projection;
    NDSRendererTask36ReplayState state;
#if NDS_TASK55_STAGE_GEOM
    /* Task 55: redundant state-write elision. GFX_COLOR/GFX_TEX_COORD are
     * persistent geometry-engine state, so a COLOR/TEX_COORD word whose value
     * equals the last recorded one changes nothing. Tracking the last value
     * during capture lets us skip recording it -> owner->words[] shrinks
     * ~20.6% losslessly. The valid flag clears at capture start so the first
     * value of each class is always recorded. */
    u32 task55_last_color;
    u32 task55_last_texcoord;
    u32 task55_state_valid;
#endif
} NDSRendererTask36ReplayOwner;

static NDSRendererTask36ReplayOwner sNdsRendererTask36ReplayOwner;

/* Written once, by ndsRendererTask36ReplayFinishFrame. See the comment there. */
volatile u32 gNdsRendererTask36CaptureSegmentMask;
volatile u32 gNdsRendererTask36CaptureWordCount;
volatile u32 gNdsRendererTask36CaptureOutcome;

/* Task 53: arena admission macros. Default-off keeps the legacy exact
 * guard byte-identical so the published ROM hashes the same as master
 * (1818AA77+). When NDS_TASK53_REPLAY_ARENA_FIX is 1, admit any arena
 * of >= 0x130000 bytes (the documented floor of the robust downward-
 * stepping allocator at src/port/diagnostics.c:7354) regardless of
 * the alloc-fail count; the captured words live in a static BSS buffer
 * (no arena-layout dependency) and the per-frame rigid_binding_mask,
 * config memcmp, and texture-validity walkers at :4185 / :4217 / :4139
 * are the real per-frame correctness envelope. The legacy macro is
 * retained for the staleness detector so a future re-tightening of
 * the gate is visible to the verifier. */
#define NDS_TASK36_REPLAY_ARENA_STRICT_LEGACY_BLOCKED() \
    ((gNdsTaskmanArenaChosenSize != 0x150000u) || \
     (gNdsTaskmanArenaAllocFailCount != 0u))
#define NDS_TASK53_REPLAY_ARENA_RELAXED_BLOCKED() \
    (gNdsTaskmanArenaChosenSize < 0x130000u)
#if NDS_TASK53_REPLAY_ARENA_FIX
#define NDS_TASK36_REPLAY_ARENA_BLOCKED() \
    NDS_TASK53_REPLAY_ARENA_RELAXED_BLOCKED()
#else
#define NDS_TASK36_REPLAY_ARENA_BLOCKED() \
    NDS_TASK36_REPLAY_ARENA_STRICT_LEGACY_BLOCKED()
#endif

#if NDS_R2_STAGE_ROUTE_PROBE
/* R2-07 E2 -- route each stage segment independently, in ONE binary.
 *
 * The second-entry stage corruption has a structural clue nothing else
 * explains: Task 36 replays segments 0/5/7 and those look right, while the live
 * path owns 1/2/3/4/6 and those are wrong. That argues against every wholesale
 * story (asset, camera, projection, every texture) and for something in the live
 * payload -- but only if the route can actually be moved and the move observed.
 *
 * Separately linked A/B ROMs cannot answer it: this ROM's pacing is
 * cache-placement sensitive, which is already recorded as having confused two
 * earlier comparisons. Hence a runtime override rather than a build flag.
 *
 * `Enable` off means the compile-time mask decides, so the probe build behaves
 * exactly like the shipping one until gdb writes the variables.
 *
 * There are THREE routes here, not two, and the first draft of this probe could
 * only express two -- which would have made the experiment unfalsifiable. A
 * segment's preparation can:
 *
 *   REPLAY  -- ReplayUsePreparedSegment hits and the segment is skipped on the
 *              prepare side entirely; commit replays its captured GX stream.
 *   R2_LIVE -- R2-02 E8's elision: skip the preflight body and reuse the
 *              memoised prepared-run table built on an earlier frame.
 *   GENERIC -- run the full preflight body and PrepareRun, rebuilding the run
 *              table from source data. This is exactly what every segment does
 *              on the first frame after a config change (r2_reuse == 0), so it
 *              is a route the shipping binary already takes, not a new one.
 *
 * A single replay mask collapses R2_LIVE and GENERIC: with it, "not replay"
 * always meant "reuse the memo", and the arm that reruns preparation from
 * source could not be requested at all. Hence force_replay + force_generic,
 * with anything in neither mask taking R2_LIVE.
 *
 * The observed masks exist because a picture change is not evidence the
 * intended path ran -- set the override wrong and you get a different picture
 * for the wrong reason. A bit appears in a bucket only when that route was
 * actually taken for that segment. They are masks, not counters, because this
 * gate is queried several times per segment per frame and a count would measure
 * query traffic.
 *
 * Mixed catches the failure those three masks would otherwise hide: the gate is
 * queried from the prepare side and again from the commit side, so a segment
 * that took REPLAY during preparation and R2_LIVE at commit would show up in
 * both buckets and read as "engaged" in each. A bit in Mixed means that segment
 * disagreed with itself within one frame and its arm is void.
 *
 * Observed masks are per frame, snapshotted into Last* by BeginFrame. Masks
 * accumulated from boot would let match one contaminate the second-entry
 * evidence, and a gdb read landing mid-frame would show a partial one; the
 * Last* triple is always exactly one completed frame. */
#define NDS_TASK36_ROUTE_REPLAY 0u
#define NDS_TASK36_ROUTE_R2_LIVE 1u
#define NDS_TASK36_ROUTE_GENERIC 2u

volatile u32 gNdsRendererTask36RouteOverrideEnable;
volatile u32 gNdsRendererTask36RouteForceReplayMask;
volatile u32 gNdsRendererTask36RouteForceGenericMask;
volatile u32 gNdsRendererTask36RouteObservedReplayMask;
volatile u32 gNdsRendererTask36RouteObservedLiveMask;
volatile u32 gNdsRendererTask36RouteObservedGenericMask;
volatile u32 gNdsRendererTask36RouteObservedMixedMask;
volatile u32 gNdsRendererTask36RouteLastReplayMask;
volatile u32 gNdsRendererTask36RouteLastLiveMask;
volatile u32 gNdsRendererTask36RouteLastGenericMask;
volatile u32 gNdsRendererTask36RouteLastMixedMask;
volatile u32 gNdsRendererTask36RouteFrameCount;

static u32 ndsRendererTask36SegmentRoute(u32 segment_index)
{
    const u32 bit = 1u << segment_index;
    u32 route;
    u32 others;

    if (gNdsRendererTask36RouteOverrideEnable != 0u)
    {
        if ((gNdsRendererTask36RouteForceGenericMask & bit) != 0u)
        {
            route = NDS_TASK36_ROUTE_GENERIC;
        }
        else if ((gNdsRendererTask36RouteForceReplayMask & bit) != 0u)
        {
            route = NDS_TASK36_ROUTE_REPLAY;
        }
        else
        {
            route = NDS_TASK36_ROUTE_R2_LIVE;
        }
    }
    else
    {
        route = (((u32)NDS_TASK36_REPLAY_SEGMENT_MASK & bit) != 0u) ?
            NDS_TASK36_ROUTE_REPLAY : NDS_TASK36_ROUTE_R2_LIVE;
    }
    if (route == NDS_TASK36_ROUTE_REPLAY)
    {
        others = gNdsRendererTask36RouteObservedLiveMask |
                 gNdsRendererTask36RouteObservedGenericMask;
        gNdsRendererTask36RouteObservedReplayMask |= bit;
    }
    else if (route == NDS_TASK36_ROUTE_GENERIC)
    {
        others = gNdsRendererTask36RouteObservedReplayMask |
                 gNdsRendererTask36RouteObservedLiveMask;
        gNdsRendererTask36RouteObservedGenericMask |= bit;
    }
    else
    {
        others = gNdsRendererTask36RouteObservedReplayMask |
                 gNdsRendererTask36RouteObservedGenericMask;
        gNdsRendererTask36RouteObservedLiveMask |= bit;
    }
    if ((others & bit) != 0u)
    {
        gNdsRendererTask36RouteObservedMixedMask |= bit;
    }
    return route;
}

static void ndsRendererTask36RouteBeginFrame(void)
{
    gNdsRendererTask36RouteLastReplayMask =
        gNdsRendererTask36RouteObservedReplayMask;
    gNdsRendererTask36RouteLastLiveMask =
        gNdsRendererTask36RouteObservedLiveMask;
    gNdsRendererTask36RouteLastGenericMask =
        gNdsRendererTask36RouteObservedGenericMask;
    gNdsRendererTask36RouteLastMixedMask =
        gNdsRendererTask36RouteObservedMixedMask;
    gNdsRendererTask36RouteObservedReplayMask = 0u;
    gNdsRendererTask36RouteObservedLiveMask = 0u;
    gNdsRendererTask36RouteObservedGenericMask = 0u;
    gNdsRendererTask36RouteObservedMixedMask = 0u;
    gNdsRendererTask36RouteFrameCount++;
}

static s32 ndsRendererTask36ReplaySegmentEligible(u32 segment_index)
{
    if (segment_index >= NDS_NATIVE_STAGE_SEGMENT_COUNT)
    {
        return FALSE;
    }
    return (ndsRendererTask36SegmentRoute(segment_index) ==
            NDS_TASK36_ROUTE_REPLAY) ? TRUE : FALSE;
}

static s32 ndsRendererTask36SegmentForcedGeneric(u32 segment_index)
{
    if (segment_index >= NDS_NATIVE_STAGE_SEGMENT_COUNT)
    {
        return FALSE;
    }
    return (ndsRendererTask36SegmentRoute(segment_index) ==
            NDS_TASK36_ROUTE_GENERIC) ? TRUE : FALSE;
}
#else
static s32 ndsRendererTask36ReplaySegmentEligible(u32 segment_index)
{
    return ((segment_index < NDS_NATIVE_STAGE_SEGMENT_COUNT) &&
            ((NDS_TASK36_REPLAY_SEGMENT_MASK &
              (1u << segment_index)) != 0u)) ? TRUE : FALSE;
}
/* Macros, not empty statics: without the probe nothing else references these
 * and a static definition would be an unused-function diagnostic. */
#define ndsRendererTask36RouteBeginFrame() ((void)0)
#define ndsRendererTask36SegmentForcedGeneric(seg) (((void)(seg)), FALSE)
#endif

static void ndsRendererTask36ReplayReset(void)
{
    memset(&sNdsRendererTask36ReplayOwner, 0,
           sizeof(sNdsRendererTask36ReplayOwner));
    /* Kept in step with the memset that used to clear the owner's copy. */
    sNdsRendererTask36CaptureActive = FALSE;
    /* R2-07 leg A: the table this certificate describes has just been zeroed. */
    sNdsTask36ReplayTextureProofValid = FALSE;
    sNdsRendererTask36ReplayOwner.current_run = UINT_MAX;
    sNdsRendererTask36ReplayOwner.command_word_index = UINT_MAX;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_UNSEEDED;
#endif
}

static s32 ndsRendererTask36ReplayTexturesValid(void)
{
    u32 run_index;

    /* R2-07 leg A, replay half. Same epoch, separate certificate -- see
     * ndsRendererNativeStagePreparedTexturesProven. Dropped wherever this
     * table is written: ndsRendererTask36ReplayReset and the per-run
     * `run->prepared = ...` capture. */
    if ((sNdsTask36ReplayTextureProofValid != FALSE) &&
        (sNdsTask36ReplayTextureProofEpoch ==
         sNdsRendererHardwareTextureKeyGeneration))
    {
        gNdsR2TexProofFastCount++;
        return TRUE;
    }
    gNdsR2TexProofSweepCount++;
    for (run_index = 0u; run_index < NDS_NATIVE_STAGE_RUN_COUNT; run_index++)
    {
        const NDSRendererTask36ReplayRun *run =
            &sNdsRendererTask36ReplayOwner.runs[run_index];

        if (run->valid == FALSE)
        {
            continue;
        }
        if (ndsRendererNativeStagePreparedTextureValid(
                &run->prepared) == FALSE)
        {
            sNdsTask36ReplayTextureProofValid = FALSE;
            gNdsR2TexProofSweepFailCount++;
            return FALSE;
        }
    }
    sNdsTask36ReplayTextureProofEpoch =
        sNdsRendererHardwareTextureKeyGeneration;
    sNdsTask36ReplayTextureProofValid = TRUE;
    return TRUE;
}

static void ndsRendererTask36ReplayBeginFrame(
    const NDSRendererNativeStageFrame *frame)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

    /* First statement in the frame: every later route query in this frame lands
     * in the freshly cleared observed masks. */
    ndsRendererTask36RouteBeginFrame();
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36ReplaySegmentCount = 0u;
    gNdsRendererTask36ReplayRunCount = 0u;
    gNdsRendererTask36ReplayWordCount = 0u;
#endif
    owner->frame_capture = FALSE;
    owner->frame_replay = FALSE;
    if ((owner->topology_generation != frame->topology_generation) ||
        (owner->topology_stamp != frame->topology_stamp))
    {
        ndsRendererTask36ReplayReset();
        owner->topology_generation = frame->topology_generation;
        owner->topology_stamp = frame->topology_stamp;
    }
    if (NDS_TASK36_REPLAY_SEGMENT_MASK == 0u)
    {
        return;
    }
    if (frame->rigid_binding_mask != NDS_NATIVE_STAGE_RIGID_BINDING_MASK)
    {
        if (owner->state == NDS_TASK36_REPLAY_READY)
        {
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36ReplayFallbackCount++;
#endif
        }
        return;
    }
    if (NDS_TASK36_REPLAY_ARENA_BLOCKED())
    {
        if ((gNdsTaskmanArenaChosenSize != 0u) &&
            (owner->state != NDS_TASK36_REPLAY_DISABLED))
        {
            owner->state = NDS_TASK36_REPLAY_DISABLED;
#if NDS_RENDERER_PROFILE_LEVEL == 1
            gNdsRendererTask36ReplayArenaRejectCount++;
            gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_DISABLED;
#endif
        }
        return;
    }
#if NDS_TASK53_REPLAY_ARENA_FIX
    /* Task 53: staleness detector. Each frame the relaxed guard admits
     * and the legacy strict guard would have DISABLED -- proof that the
     * fix is doing its job, and a hook that a future re-tightening of
     * the gate would push to zero (alerting the verifier). */
    if (NDS_TASK36_REPLAY_ARENA_STRICT_LEGACY_BLOCKED())
    {
        gNdsRendererTask36ReplayArenaStaleCount++;
    }
#endif
    if (owner->state == NDS_TASK36_REPLAY_UNSEEDED)
    {
        return;
    }
    if (owner->state != NDS_TASK36_REPLAY_READY)
    {
        return;
    }
    if (memcmp(&owner->config, frame->config, sizeof(owner->config)) != 0)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36ReplayMaterialRejectCount++;
        gNdsRendererTask36ReplayFallbackCount++;
#endif
        return;
    }
    if (ndsRendererTask36ReplayTexturesValid() == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36ReplayFallbackCount++;
#endif
        return;
    }
    /* BUGS.md #9 -- see the projection field on the owner. */
    if ((frame->projection == NULL) ||
        (memcmp(&owner->projection, frame->projection,
                sizeof(owner->projection)) != 0))
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36ReplayProjectionRejectCount++;
        gNdsRendererTask36ReplayFallbackCount++;
#endif
        return;
    }
    owner->frame_replay = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36ReplayFrameCount++;
#endif
}

static void ndsRendererTask36ReplayStartCapture(
    const NDSRendererNativeStageFrame *frame)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

    if ((NDS_TASK36_REPLAY_SEGMENT_MASK == 0u) ||
        (owner->state != NDS_TASK36_REPLAY_UNSEEDED) ||
        (frame->rigid_binding_mask != NDS_NATIVE_STAGE_RIGID_BINDING_MASK) ||
        (frame->projection == NULL) ||
        NDS_TASK36_REPLAY_ARENA_BLOCKED())
    {
        return;
    }
    owner->state = NDS_TASK36_REPLAY_CAPTURING;
    owner->frame_capture = TRUE;
    owner->word_count = 0u;
    owner->captured_segment_mask = 0u;
    owner->capture_fault = FALSE;
    owner->config = *frame->config;
    /* BUGS.md #9. Record the projection this stream is being baked against, so
     * BeginFrame can decline the stream once the live one moves. */
    owner->projection = *frame->projection;
#if NDS_TASK55_STAGE_GEOM
    owner->task55_state_valid = 0u;
    owner->task55_last_color = 0u;
    owner->task55_last_texcoord = 0u;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36BakeAttemptCount++;
    gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_CAPTURING;
#endif
}

static void ndsRendererTask36ReplayCapturePreparedSegment(
    u32 segment_index,
    const NDSRendererStats *stats,
    u64 epoch_mask)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

    if (((owner->frame_capture == FALSE) &&
         (owner->state != NDS_TASK36_REPLAY_UNSEEDED)) ||
        (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE))
    {
        return;
    }
    owner->segment_stats[segment_index] = *stats;
    owner->segment_epoch_mask[segment_index] = epoch_mask;
}

static s32 ndsRendererTask36ReplayUsePreparedSegment(
    u32 segment_index,
    NDSRendererStats *stats,
    u64 *epoch_mask)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;

    if ((owner->frame_replay == FALSE) || (stats == NULL) ||
        (epoch_mask == NULL) ||
        (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE))
    {
        return FALSE;
    }
#if NDS_TASK104_STAGE_STATS_ELISION
    /* Task 104: the whole-struct assignment this replaces transported exactly
     * four live bytes. `preflight_stats` is dead across a replay hit — the next
     * segment's `ndsRendererInitStats` clears it, and once the segment loop
     * ends the only member anything reads is `sync_command_count`, which is
     * assigned into the caller's stats. Every other member is overwritten
     * before its next read, so copying 1,292 bytes moved one u32.
     *
     * The clear that preceded the copy was dead for the same reason, and the
     * pair cost ~3,876 bytes of cold traffic on each of the three hit segments
     * per frame. Task 84 E1 priced this struct at 2.74 ticks/byte: 1,292 bytes
     * span ~41 cache lines and the renderer evicts them between segments.
     *
     * Task 103 E7 removed the clear on its own and realised only 28% of the
     * predicted saving, because the copy still touched the same lines and the
     * misses relocated into it rather than disappearing — the mechanism Task 84
     * E1.4 predicted. Both accesses have to go together or neither is worth
     * removing, which is why the caller hoists this check above the clear. */
    stats->sync_command_count =
        owner->segment_stats[segment_index].sync_command_count;
#else
    *stats = owner->segment_stats[segment_index];
#endif
    *epoch_mask |= owner->segment_epoch_mask[segment_index];
    return TRUE;
}

static void ndsRendererTask36ReplayCaptureBeginRun(u32 run_index)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    NDSRendererTask36ReplayRun *run;

    if ((owner->frame_capture == FALSE) ||
        (run_index >= NDS_NATIVE_STAGE_RUN_COUNT) ||
        (sNdsRendererTask36CaptureActive != FALSE))
    {
        owner->capture_fault = TRUE;
        return;
    }
    run = &owner->runs[run_index];
    memset(run, 0, sizeof(*run));
    run->prepared = sNdsNativeStageOwnerExecution.runs[run_index];
    /* R2-07 leg A: this run's copy just changed, so the replay table's standing
     * certificate no longer describes it. */
    sNdsTask36ReplayTextureProofValid = FALSE;
    run->word_offset = (u16)owner->word_count;
    owner->current_run = run_index;
    owner->command_word_index = UINT_MAX;
    owner->command_slot = 4u;
    owner->capture_push_balance = 0;
    sNdsRendererTask36CaptureActive = TRUE;
}

static void ndsRendererTask36ReplayCaptureEndRun(u32 run_index)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    NDSRendererTask36ReplayRun *run;
    u32 word_count;

    if ((sNdsRendererTask36CaptureActive == FALSE) ||
        (owner->current_run != run_index) ||
        (run_index >= NDS_NATIVE_STAGE_RUN_COUNT))
    {
        owner->capture_fault = TRUE;
        return;
    }
    run = &owner->runs[run_index];
    word_count = owner->word_count - run->word_offset;
    if ((word_count == 0u) || (word_count > USHRT_MAX))
    {
        owner->capture_fault = TRUE;
    }
    else if ((owner->capture_push_balance < 0) ||
             (owner->capture_push_balance > 1))
    {
        /* A run may leave the stack level (the actor segments) or one push deep
         * (the rigid segments, whose EndSegment pops it). Anything else means the
         * recorded stream does not describe a stack state replay can restore. */
        owner->capture_fault = TRUE;
    }
    else
    {
        run->word_count = (u16)word_count;
        /* THE STATE THE STREAM LEAVES, NOT THE STREAM'S OWN DELTA. This line
         * used to read capture_push_balance, which is a per-run DELTA, and the
         * two differ for every run after the first in a segment: Task 36's
         * EnsureWorld pops the previous binding's world before pushing its own
         * (:30290-30294), so run 2..N record balance 0 while the stack is still
         * one push deep. Replay assigns this flag verbatim (:30783, last run
         * wins) and EndSegment pops on it (:30424), so a segment with two or
         * more runs left its push on the GX position/vector stack forever.
         * Measured on build-c173-cfxcount-bp1 at NDS_R2_FIGHTER_GX_COMPOSE=0:
         * the stack level advanced +3.000 per presented frame wrapping mod 32
         * with GXSTAT's error bit set on all 128 sampled frames, against
         * EXACTLY 3.000 Task36 segments a frame -- and every push/pop the ARM9
         * writes itself balanced to zero over 1,600 frames (5,143 each), so the
         * replayed streams were the only unbalanced producer. The live flag is
         * already correct here because EnsureWorld runs inside the capture
         * bracket, so recording it costs nothing and cannot underflow: it is
         * TRUE only where a push is genuinely outstanding. */
        run->local_pushed =
            (sNdsNativeStageOwnerExecution.task36_local_pushed != FALSE) ?
                TRUE : FALSE;
        run->valid = TRUE;
    }
    sNdsRendererTask36CaptureActive = FALSE;
    owner->current_run = UINT_MAX;
    owner->command_word_index = UINT_MAX;
}

static s32 ndsRendererTask36ReplayOpcode(
    NDSRendererTask29GXClass command_class,
    u32 *opcode,
    u32 *parameter_count)
{
    switch (command_class)
    {
    case NDS_TASK29_GX_MATRIX_MODE:
        *opcode = REG2ID(MATRIX_CONTROL); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_MATRIX_IDENTITY:
        *opcode = REG2ID(MATRIX_IDENTITY); *parameter_count = 0u; return TRUE;
    case NDS_TASK29_GX_MATRIX_LOAD4X4:
        *opcode = REG2ID(MATRIX_LOAD4x4); *parameter_count = 16u; return TRUE;
    case NDS_TASK29_GX_MATRIX_MULT4X4:
        *opcode = REG2ID(MATRIX_MULT4x4); *parameter_count = 16u; return TRUE;
    case NDS_TASK29_GX_MATRIX_MULT4x3:
        *opcode = REG2ID(MATRIX_MULT4x3); *parameter_count = 12u; return TRUE;
    case NDS_TASK29_GX_MATRIX_PUSH:
        *opcode = REG2ID(MATRIX_PUSH); *parameter_count = 0u; return TRUE;
    case NDS_TASK29_GX_MATRIX_POP:
        *opcode = REG2ID(MATRIX_POP); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_MATRIX_STORE:
        *opcode = REG2ID(MATRIX_STORE); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_MATRIX_RESTORE:
        *opcode = REG2ID(MATRIX_RESTORE); *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_BEGIN:
        *opcode = FIFO_BEGIN; *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_COLOR:
        *opcode = FIFO_COLOR; *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_TEX_COORD:
        *opcode = FIFO_TEX_COORD; *parameter_count = 1u; return TRUE;
    case NDS_TASK29_GX_VERTEX16:
        *opcode = FIFO_VERTEX16; *parameter_count = 2u; return TRUE;
    default:
        return FALSE;
    }
}

static void ndsRendererTask36ReplayRecord(
    NDSRendererTask29GXClass command_class,
    const u32 *words,
    u32 word_count)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    u32 opcode;
    u32 parameter_count;
    u32 i;

    if (sNdsRendererTask36CaptureActive == FALSE)
    {
        return;
    }
    if (ndsRendererTask36ReplayOpcode(
            command_class, &opcode, &parameter_count) == FALSE)
    {
        /* Dropping an unencoded class is deliberate: every one of them is
         * state that ndsRendererNativeStageBeginRun re-issues live at replay
         * (DISP3DCNT, texture bind and params, poly format, alpha test), so
         * baking it would only duplicate a write. A *matrix* class is not in
         * that set -- nothing re-issues it, and it decides where the geometry
         * lands -- so silently dropping one bakes a stream that draws in the
         * wrong place. R2-02 E3 hit exactly that: Task 51 appended
         * MATRIX_MULT4x3 for the Task 49 differ and did not add it to the
         * table above, and the four segments that emit it were outside the
         * replay mask, so the omission stayed invisible until the mask
         * widened. Refuse the capture instead of baking a wrong stream. */
        if (((command_class >= NDS_TASK29_GX_MATRIX_MODE) &&
             (command_class <= NDS_TASK29_GX_MATRIX_RESTORE)) ||
            (command_class == NDS_TASK29_GX_MATRIX_MULT4x3))
        {
            owner->capture_fault = TRUE;
        }
        return;
    }
    /* MULT4x3 is Task 51's 12-word form of the same per-binding world matrix,
     * so it counts as a world mult exactly like MULT4X4 -- the generic path
     * bumps gNdsRendererTask36WorldMultCount for both. */
    if (command_class == NDS_TASK29_GX_MATRIX_PUSH)
    {
        owner->capture_push_balance++;
    }
    else if ((command_class == NDS_TASK29_GX_MATRIX_POP) &&
             (word_count != 0u) && (words != NULL))
    {
        owner->capture_push_balance -= (s32)words[0u];
    }
    if ((command_class == NDS_TASK29_GX_MATRIX_MULT4X4) ||
        (command_class == NDS_TASK29_GX_MATRIX_MULT4x3))
    {
        NDSRendererTask36ReplayRun *run =
            &owner->runs[owner->current_run];

        if (run->world_mult_count == UCHAR_MAX)
        {
            owner->capture_fault = TRUE;
            return;
        }
        run->world_mult_count++;
    }
    if ((parameter_count > word_count) ||
        ((parameter_count != 0u) && (words == NULL)))
    {
        owner->capture_fault = TRUE;
        return;
    }
#if NDS_TASK55_STAGE_GEOM
    /* Task 55: elide a COLOR/TEX_COORD word that is identical to the last one
     * recorded this capture frame. GFX_COLOR/GFX_TEX_COORD are persistent
     * geometry-engine state (a vertex uses the held value until rewritten), so
     * a redundant write changes nothing about the render. Omitting it from
     * owner->words[] shrinks the replay stream losslessly. Both classes carry
     * exactly one parameter word (ndsRendererTask36ReplayOpcode). */
    if (parameter_count == 1u)
    {
        if (command_class == NDS_TASK29_GX_COLOR)
        {
            if ((owner->task55_state_valid != 0u) &&
                (owner->task55_last_color == words[0u]))
            {
                return;
            }
            owner->task55_last_color = words[0u];
            owner->task55_state_valid |= 0x1u;
        }
        else if (command_class == NDS_TASK29_GX_TEX_COORD)
        {
            if ((owner->task55_state_valid & 0x2u) != 0u &&
                (owner->task55_last_texcoord == words[0u]))
            {
                return;
            }
            owner->task55_last_texcoord = words[0u];
            owner->task55_state_valid |= 0x2u;
        }
    }
#endif
    if (owner->command_slot >= 4u)
    {
        if (owner->word_count >= NDS_TASK36_REPLAY_WORD_CAPACITY)
        {
            owner->capture_fault = TRUE;
            return;
        }
        owner->command_word_index = owner->word_count++;
        owner->words[owner->command_word_index] = 0u;
        owner->command_slot = 0u;
    }
    if (owner->word_count + parameter_count >
        NDS_TASK36_REPLAY_WORD_CAPACITY)
    {
        owner->capture_fault = TRUE;
        return;
    }
    owner->words[owner->command_word_index] |=
        opcode << (owner->command_slot * 8u);
    owner->command_slot++;
    for (i = 0u; i < parameter_count; i++)
    {
        owner->words[owner->word_count++] = words[i];
    }
}

static void ndsRendererTask36ReplayFinishFrame(void)
{
    NDSRendererTask36ReplayOwner *owner =
        &sNdsRendererTask36ReplayOwner;
    u32 segment_index;
    u32 valid = TRUE;

    if (owner->frame_capture == FALSE)
    {
        return;
    }
    if ((sNdsRendererTask36CaptureActive != FALSE) ||
        (owner->capture_fault != FALSE) ||
        (owner->captured_segment_mask != NDS_TASK36_REPLAY_SEGMENT_MASK) ||
        (owner->word_count == 0u) ||
        (owner->word_count > NDS_TASK36_REPLAY_WORD_CAPACITY))
    {
        valid = FALSE;
    }
    for (segment_index = 0u;
         (segment_index < NDS_NATIVE_STAGE_SEGMENT_COUNT) && valid;
         segment_index++)
    {
        const NDSNativeStageSegment *segment;
        u32 run_offset;

        if (ndsRendererTask36ReplaySegmentEligible(segment_index) == FALSE)
        {
            continue;
        }
        segment = &sNdsNativeStageSegments[segment_index];
        for (run_offset = 0u; run_offset < segment->run_count; run_offset++)
        {
            u32 run_index = (u32)segment->first_run + run_offset;

            if (owner->runs[run_index].valid == FALSE)
            {
                valid = FALSE;
                break;
            }
        }
    }
    owner->frame_capture = FALSE;
    /* Engagement publication. The capture runs once, so these two stores are
     * one-shot, but a widened segment mask that silently falls back is
     * indistinguishable from one that engaged and saved nothing -- the mistake
     * Task 52 shipped. `outcome` is the accepted state, so a fallback reads as
     * DISABLED rather than as a missing line. Non-static and not profile-gated
     * because the tick-HUD ROM must be able to read them from the same run that
     * produced its buckets. */
    gNdsRendererTask36CaptureSegmentMask = owner->captured_segment_mask;
    gNdsRendererTask36CaptureWordCount = owner->word_count;
    if (valid == FALSE)
    {
        owner->state = NDS_TASK36_REPLAY_DISABLED;
        gNdsRendererTask36CaptureOutcome = (u32)NDS_TASK36_REPLAY_DISABLED;
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36BakeFailureCount++;
        gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_DISABLED;
#endif
        return;
    }
    DC_FlushRange(owner->words, owner->word_count * sizeof(owner->words[0]));
    owner->state = NDS_TASK36_REPLAY_READY;
    gNdsRendererTask36CaptureOutcome = (u32)NDS_TASK36_REPLAY_READY;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask36BakeSuccessCount++;
    gNdsRendererTask36ReplayCaptureWordCount = owner->word_count;
    gNdsRendererTask36ReplayState = NDS_TASK36_REPLAY_READY;
#endif
}
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
static u32 sNdsNativeStageTopologyFaultInjected;
#endif
#endif
#endif
