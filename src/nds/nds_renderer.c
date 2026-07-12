#include <limits.h>
#include <string.h>

#include <nds/nds_gbi_decode.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#ifndef NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
#define NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY 0
#endif

/* libnds builds default to compact Thumb code, but the DS reference renderer
 * keeps its display-list interpreter and GX submission loops in ARM state.
 * These four measured loops favor ARM's full register file and conditional
 * execution; retain the ordinary hot/O3 annotation for host-side fixtures. */
#if defined(__arm__)
#define NDS_RENDERER_HOT_CODE \
    __attribute__((hot, optimize("O3"), target("arm"), section(".itcm")))
#else
#define NDS_RENDERER_HOT_CODE __attribute__((hot, optimize("O3")))
#endif

#if NDS_RENDERER_HW_TRIANGLES
#include <math.h>
#include <nds.h>
#include <nds/arm9/postest.h>
#endif

#define NDS_RENDERER_OP_NOOP 0x00u
#define NDS_RENDERER_OP_VTX 0x01u
#define NDS_RENDERER_OP_MODIFYVTX 0x02u
#define NDS_RENDERER_OP_CULLDL 0x03u
#define NDS_RENDERER_OP_TRI1 0x05u
#define NDS_RENDERER_OP_TRI2 0x06u
#define NDS_RENDERER_OP_TEXTURE 0xd7u
#define NDS_RENDERER_OP_POPMTX 0xd8u
#define NDS_RENDERER_OP_MTX 0xdau
#define NDS_RENDERER_OP_GEOMETRYMODE 0xd9u
#define NDS_RENDERER_OP_MOVEWORD 0xdbu
#define NDS_RENDERER_OP_MOVEMEM 0xdcu
#define NDS_RENDERER_OP_SPECIAL_1 0xd5u
#define NDS_RENDERER_OP_DL 0xdeu
#define NDS_RENDERER_OP_ENDDL 0xdfu
#define NDS_RENDERER_OP_SETOTHERMODE_H 0xe3u
#define NDS_RENDERER_OP_SETOTHERMODE_L 0xe2u
#define NDS_RENDERER_OP_SETSCISSOR 0xedu
#define NDS_RENDERER_OP_SETPRIMDEPTH 0xeeu
#define NDS_RENDERER_OP_SETCOMBINE 0xfcu
#define NDS_RENDERER_OP_SETCIMG 0xffu
#define NDS_RENDERER_OP_SETFOGCOLOR 0xf8u
#define NDS_RENDERER_OP_SETBLENDCOLOR 0xf9u
#define NDS_RENDERER_OP_SETENVCOLOR 0xfbu
#define NDS_RENDERER_OP_SETPRIMCOLOR 0xfau
#define NDS_RENDERER_OP_SETTIMG 0xfdu
#define NDS_RENDERER_OP_SETTILE 0xf5u
#define NDS_RENDERER_OP_LOADTILE 0xf4u
#define NDS_RENDERER_OP_LOADBLOCK 0xf3u
#define NDS_RENDERER_OP_LOADTLUT 0xf0u
#define NDS_RENDERER_OP_SETTILESIZE 0xf2u
#define NDS_RENDERER_OP_RDPSETOTHERMODE 0xefu
#define NDS_RENDERER_OP_RDPPIPESYNC 0xe7u
#define NDS_RENDERER_OP_RDPLOADSYNC 0xe6u
#define NDS_RENDERER_OP_RDPTILESYNC 0xe8u
#define NDS_RENDERER_OP_RDPFULLSYNC 0xe9u

#define NDS_RENDERER_TX_CLAMP 0x2u
#define NDS_RENDERER_TX_MIRROR 0x1u
#define NDS_RENDERER_RENDER_TILE 0u
#define NDS_RENDERER_RENDER_TILE_1 1u
#define NDS_RENDERER_LOAD_TILE 7u

#define NDS_RENDERER_MAX_VTX NDS_RENDERER_VERTEX_CACHE_SIZE
#define NDS_RENDERER_MODELVIEW_STACK_SIZE 32u
#define NDS_RENDERER_N64_MTX_FRAC_BITS 16u
#define NDS_RENDERER_DS_MTX_FRAC_BITS 12u
#define NDS_RENDERER_MTX_PUSH_XOR 0x01u
#define NDS_RENDERER_MTX_PUSH 0x01u
#define NDS_RENDERER_MTX_LOAD 0x02u
#define NDS_RENDERER_MTX_PROJECTION 0x04u
#define NDS_RENDERER_MOVEWORD_MATRIX 0x00u
#define NDS_RENDERER_MOVEWORD_FOG 0x08u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL 0x0au
#define NDS_RENDERER_MOVEWORD_FOG_OFFSET 0x00u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A 0x00u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B 0x04u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A 0x18u
#define NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B 0x1cu
#define NDS_RENDERER_MOVEWORD_OFFSET_SHIFT 8u
#define NDS_RENDERER_MOVEWORD_OFFSET_MASK 0xffffu
#define NDS_RENDERER_MOVEWORD_INDEX_MASK 0xffu
#define NDS_RENDERER_MWO_POINT_ST 0x14u
#define NDS_RENDERER_MOVEMEM_LIGHT 10u
#define NDS_RENDERER_MOVEMEM_OFFSET_SHIFT 8u
#define NDS_RENDERER_MOVEMEM_OFFSET_MASK 0xffu
#define NDS_RENDERER_MOVEMEM_LENGTH_SHIFT 19u
#define NDS_RENDERER_MOVEMEM_LENGTH_MASK 0x1fu
#define NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET 24u
#define NDS_RENDERER_MOVEMEM_LIGHT_STRIDE 24u
#define NDS_RENDERER_MATRIX_WORD_BYTES 4u
#define NDS_RENDERER_MATRIX_WORD_COUNT 16u
#define NDS_RENDERER_HW_TEXTURE_MAX_WIDTH 128u
#define NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT 128u
#define NDS_RENDERER_HW_TEXTURE_MAX_TEXELS \
    (NDS_RENDERER_HW_TEXTURE_MAX_WIDTH * NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT)
#define NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT (16u * 16u)
#define NDS_RENDERER_HW_TEXEL01_CI4_PHASE_COUNT 16u
#define NDS_RENDERER_HW_TEXEL01_CI4_PHASE_LUT_COUNT \
    (NDS_RENDERER_HW_TEXEL01_CI4_PHASE_COUNT * \
     NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT)
#define NDS_RENDERER_HW_TEXEL01_RGB_MASK 0x7fffu
#define NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT 15u
#define NDS_RENDERER_HW_TEXTURE_FMT_RGBA16 0u
#define NDS_RENDERER_HW_TEXTURE_FMT_CI 2u
#define NDS_RENDERER_HW_TEXTURE_FMT_IA 3u
#define NDS_RENDERER_HW_TEXTURE_FMT_I16 4u
#define NDS_RENDERER_HW_TEXTURE_SIZ_4B 0u
#define NDS_RENDERER_HW_TEXTURE_SIZ_8B 1u
#define NDS_RENDERER_HW_TEXTURE_SIZ_16B 2u
#define NDS_RENDERER_HW_TEXTURE_SIZ_32B 3u
#define NDS_RENDERER_G_TX_DXT_ONE (1u << 11)
#define NDS_RENDERER_HW_TEXREJECT_MISSING_STATE (1u << 0)
#define NDS_RENDERER_HW_TEXREJECT_BAD_CI_SIZE (1u << 1)
#define NDS_RENDERER_HW_TEXREJECT_UNSUPPORTED_FORMAT (1u << 2)
#define NDS_RENDERER_HW_TEXREJECT_BAD_DIMENSIONS (1u << 3)
#define NDS_RENDERER_HW_TEXREJECT_BAD_UPLOAD_SIZE (1u << 4)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_RANGE (1u << 5)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES (1u << 6)
#define NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_PTR (1u << 7)
#define NDS_RENDERER_HW_TEXREJECT_BAD_TLUT (1u << 8)
#define NDS_RENDERER_HW_TEXREJECT_BAD_TLUT_PTR (1u << 9)
#define NDS_RENDERER_HW_TEXREJECT_ALLOC (1u << 10)
#define NDS_RENDERER_HW_TEXREJECT_GENTEX (1u << 11)
#define NDS_RENDERER_HW_TEXREJECT_TEXIMAGE (1u << 12)
#define NDS_RENDERER_HW_TEXEL1_REJECT_ACTIVE_TILE (1u << 0)
#define NDS_RENDERER_HW_TEXEL1_REJECT_TILE_STATE (1u << 1)
#define NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE (1u << 2)
#define NDS_RENDERER_HW_TEXEL1_REJECT_DIMENSIONS (1u << 3)
#define NDS_RENDERER_HW_TEXEL1_REJECT_PAIR_SIZE (1u << 4)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_RANGE (1u << 5)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_BYTES (1u << 6)
#define NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_PTR (1u << 7)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_STATS (1u << 0)
#define NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF (1u << 1)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE (1u << 2)
#define NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL (1u << 3)
#define NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0 (1u << 4)
#define NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE 0xffffu
#define NDS_RENDERER_HW_WORLD_UNIT_SHIFT 8u
#define NDS_RENDERER_HW_RAW_COORD_MIN (-2048)
#define NDS_RENDERER_HW_RAW_COORD_MAX 2047
#define NDS_RENDERER_HW_MATRIX_MODE_NONE 0u
#define NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY 1u
#define NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED 2u
#define NDS_RENDERER_HW_POS_TEST_MAX 64u
#define NDS_RENDERER_HW_POS_TEST_TOLERANCE 16u
#define NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA 4352
#define NDS_RENDERER_MATRIX_SNAPSHOT_INVALID 0u
#define NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START (0x1000 * 6)
#define NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START \
    ((128 - 0x1000) * 6)
#define NDS_RENDERER_HW_PROJECTED_DEPTH_STEP 6
#define NDS_RENDERER_HW_PROJECTED_VERTEX (1 << 12)
#define NDS_RENDERER_HW_DECAL_DEPTH_BIAS (3 << 4)
#define NDS_RENDERER_HW_ORACLE_EPSILON 0u
#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 64u
#define NDS_RENDERER_CCMUX_COMBINED 0u
#define NDS_RENDERER_CCMUX_TEXEL0 1u
#define NDS_RENDERER_CCMUX_TEXEL1 2u
#define NDS_RENDERER_CCMUX_PRIMITIVE 3u
#define NDS_RENDERER_CCMUX_SHADE 4u
#define NDS_RENDERER_CCMUX_ENVIRONMENT 5u
#define NDS_RENDERER_CCMUX_PRIM_LOD_FRAC 14u
#define NDS_RENDERER_CCMUX_ZERO_AB 15u
#define NDS_RENDERER_CCMUX_ZERO_D 7u
#define NDS_RENDERER_ACMUX_COMBINED 0u
#define NDS_RENDERER_ACMUX_TEXEL0 1u
#define NDS_RENDERER_ACMUX_TEXEL1 2u
#define NDS_RENDERER_ACMUX_PRIMITIVE 3u
#define NDS_RENDERER_ACMUX_SHADE 4u
#define NDS_RENDERER_ACMUX_ENVIRONMENT 5u
#define NDS_RENDERER_ACMUX_1 6u
#define NDS_RENDERER_ACMUX_0 7u
#define NDS_RENDERER_MDSFT_CYCLETYPE 20u
#define NDS_RENDERER_CYCLETYPE_MASK (3u << NDS_RENDERER_MDSFT_CYCLETYPE)
#define NDS_RENDERER_CYC_2CYCLE (1u << NDS_RENDERER_MDSFT_CYCLETYPE)
#define NDS_RENDERER_MDSFT_TEXTPERSP 19u
#define NDS_RENDERER_TP_PERSP (1u << NDS_RENDERER_MDSFT_TEXTPERSP)
#define NDS_RENDERER_TEXTPERSP_MASK (1u << NDS_RENDERER_MDSFT_TEXTPERSP)
#define NDS_RENDERER_MDSFT_TEXTFILT 12u
#define NDS_RENDERER_TF_POINT (0u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TF_BILERP (2u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TEXTFILT_MASK (3u << NDS_RENDERER_MDSFT_TEXTFILT)
#define NDS_RENDERER_TEXCOORD_FILTER_OFFSET (1 << 4)
#define NDS_RENDERER_ALPHA_COMPARE_MASK 0x3u
#define NDS_RENDERER_ALPHA_COMPARE_THRESHOLD 0x1u
#define NDS_RENDERER_ZSOURCE_PRIM 0x00000004u
#define NDS_RENDERER_ZSOURCE_MASK 0x00000004u
#define NDS_RENDERER_ZMODE_MASK 0x00000c00u
#define NDS_RENDERER_ZMODE_XLU 0x00000800u
#define NDS_RENDERER_ZMODE_DEC 0x00000c00u
#define NDS_RENDERER_CVG_X_ALPHA 0x00001000u
#define NDS_RENDERER_FORCE_BL 0x00004000u
#define NDS_RENDERER_G_BL_A_MEM 1u
#define NDS_RENDERER_BLEND_ALPHA_BITS_MASK 0x3u
#define NDS_RENDERER_BLEND_ALPHA_CYCLE1_SHIFT 18u
#define NDS_RENDERER_BLEND_ALPHA_CYCLE2_SHIFT 16u
#define NDS_RENDERER_POLY_ID_MASK 0x3fu
#define NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK \
    ((3u << 30) | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | \
     GL_TEXTURE_FLIP_S | GL_TEXTURE_FLIP_T)
#define NDS_RENDERER_LIGHT_COLOR_1_MASK (1u << 0)
#define NDS_RENDERER_LIGHT_COLOR_2_MASK (1u << 1)
#define NDS_RENDERER_LIGHT_DIR_1_MASK (1u << 0)
/* BattleShip's fighter display seed emits these light colors before its
 * collision overlay DL: ftdisplaymain.c:205-206. They are a source-shaped
 * fallback for lit lists whose scene callback supplied only light direction. */
#define NDS_RENDERER_LIGHT_COLOR_1_FALLBACK 0x40404000u
#define NDS_RENDERER_LIGHT_COLOR_2_FALLBACK 0xc0c0c000u
#define NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL (1u << 0)
#define NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX (1u << 1)
#define NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE (1u << 2)
#define NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD (1u << 3)
#define NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED (1u << 4)
#define NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH (1u << 5)
#define NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH (1u << 6)
#define NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH (1u << 7)
#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state) \
    ((state)->texture_prepare_valid = FALSE)
#else
#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state) ((void)(state))
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 1
static u32 sNdsRendererProfileImmutableListCount;
static u32 sNdsRendererProfileTrustedCommandCount;
static u32 sNdsRendererProfileValidatedCommandCount;
static u32 sNdsRendererProfileTriangleRunReuseCount;
static u32 sNdsRendererProfileTriangleSubmitTicks;
static u32 sNdsRendererProfileVertexSubmitTicks;
static u32 sNdsRendererProfileCi4LutBuildCount;
static u32 sNdsRendererProfileCi4LutReuseCount;
#endif

#if NDS_RENDERER_HW_TRIANGLES
typedef enum NDSRendererHWSubmitClass
{
    NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX = 0,
    NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH,
    NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX,
    NDS_RENDERER_HW_SUBMIT_REJECT,
    NDS_RENDERER_HW_SUBMIT_CLASS_COUNT
} NDSRendererHWSubmitClass;

static u32 sNdsRendererHardwareSubmitted;
static u32 sNdsRendererHardwareNoOracle;
static u32 sNdsRendererHardwareTriangleBatchOpen;
static u32 sNdsRendererHardwareTriangleBatchTextured;
static u32 sNdsRendererHardwareTriangleBatchTextureName;
static u32 sNdsRendererHardwareTriangleBatchPolyFmt;
static u32 sNdsRendererHardwareTriangleBatchAlphaKey;
static u32 sNdsRendererHardwareTriangleBatchFogKey;
static u32 sNdsRendererHardwareTriangleBatchMatrixMode;
static u32 sNdsRendererHardwareTriangleBatchMatrixGeneration;
static u32 sNdsRendererHardwareBoundTextureName;
static int sNdsRendererHardwareNoTextureName;
static s32 sNdsRendererHardwareProjectedDepth =
    NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START;
static u32 sNdsRendererHardwareProjectedBackground = TRUE;
static u32 sNdsRendererHardwareMatrixLoaded;
static u32 sNdsRendererHardwareMatrixMode;
static u32 sNdsRendererHardwareMatrixGeneration;
static u32 sNdsRendererMatrixGenerationSerial;
static u32 sNdsRendererHardwareSubmitClassCounts[
    NDS_RENDERER_HW_SUBMIT_CLASS_COUNT];
static u32 sNdsRendererHardwareProjectedDivisionCount;
static u32 sNdsRendererHardwareSourceVertexLoadCount;
static u32 sNdsRendererHardwareCPUTransformCount;
static u32 sNdsRendererHardwareTransformCacheHitCount;
static u32 sNdsRendererHardwareMatrixSnapshotCreateCount;
static u32 sNdsRendererHardwareMatrixSnapshotReuseCount;
static u32 sNdsRendererHardwareMatrixSnapshotOverflowCount;

/* Profile levels 0/1 accumulate the small runtime health summary in ordinary
 * memory and publish it once at frame completion. The performance build never
 * writes the exported volatile profile globals from command, triangle, or
 * vertex loops. Level 2 retains the exact forensic counters below. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
typedef struct NDSRendererRuntimeFrameSummary
{
    u32 texture_binds;
    u32 texture_uploads;
    u32 texture_upload_bytes;
    u32 texture_cache_alias_avoid_count;
    u32 texel1_composite_count;
    u32 texel1_load_match_count;
    u32 texel1_reject_count;
    u32 projected_submit_fallback_count;
    u32 matrix_load_count;
    u32 hardware_vertices;
    u32 hardware_triangles;
    u32 hardware_batch_begin_count;
    u32 hardware_batch_reuse_count;
    u32 hardware_batch_end_count;
    u32 texture_prepare_count;
    u32 texture_prepare_reuse_count;
    u32 hardware_over_limit;
    u32 hardware_vertex_saturate_count;
    u32 raw_current_candidate_count;
    u32 raw_current_range_reject_count;
    u32 raw_cross_matrix_count;
} NDSRendererRuntimeFrameSummary;

static NDSRendererRuntimeFrameSummary sNdsRendererRuntimeFrameSummary;
static u32 sNdsRendererRuntimeTexel1FractionRefreshCount;
static u32 sNdsRendererRuntimeTextureCacheEvictCount;
static u32 sNdsRendererRuntimeTextureCi4DirectPixels;
#endif

static inline void ndsRendererProfileRecordTextureBind(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureBinds++;
#else
    sNdsRendererRuntimeFrameSummary.texture_binds++;
#endif
}

static inline void ndsRendererProfileRecordTextureUpload(u32 bytes)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureUploads++;
    gNdsRendererProfileTextureUploadBytes += bytes;
#else
    sNdsRendererRuntimeFrameSummary.texture_uploads++;
    sNdsRendererRuntimeFrameSummary.texture_upload_bytes += bytes;
#endif
}

static inline void ndsRendererProfileRecordTextureCi4Direct(u32 pixels)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureCi4DirectPixels += pixels;
#else
    sNdsRendererRuntimeTextureCi4DirectPixels += pixels;
#endif
}

static inline void ndsRendererProfileRecordTextureAliasAvoid(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureCacheAliasAvoidCount++;
#else
    sNdsRendererRuntimeFrameSummary.texture_cache_alias_avoid_count++;
#endif
}

static inline void ndsRendererProfileRecordTextureEvict(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureCacheEvictCount++;
#else
    sNdsRendererRuntimeTextureCacheEvictCount++;
#endif
}

static inline void ndsRendererProfileRecordTexel1Composite(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1CompositeCount++;
    gNdsRendererProfileTexel1LoadMatchCount++;
#else
    sNdsRendererRuntimeFrameSummary.texel1_composite_count++;
    sNdsRendererRuntimeFrameSummary.texel1_load_match_count++;
#endif
}

static inline void ndsRendererProfileRecordTexel1Reject(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1RejectCount++;
#else
    sNdsRendererRuntimeFrameSummary.texel1_reject_count++;
#endif
}

static inline void ndsRendererProfileRecordTexel1RejectReason(u32 reason)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1RejectReasonMask |= reason;
#else
    (void)reason;
#endif
}

static inline void ndsRendererProfileRecordTexel1Refresh(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1FractionRefreshCount++;
#else
    sNdsRendererRuntimeTexel1FractionRefreshCount++;
#endif
}

static inline void ndsRendererProfileRecordMatrixLoad(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileMatrixLoadCount++;
#else
    sNdsRendererRuntimeFrameSummary.matrix_load_count++;
#endif
}

static inline void ndsRendererProfileRecordBatchBegin(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchBeginCount++;
#else
    sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count++;
#endif
}

static inline void ndsRendererProfileRecordBatchReuse(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchReuseCount++;
#else
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count++;
#endif
}

static inline void ndsRendererProfileRecordBatchEnd(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareBatchEndCount++;
#else
    sNdsRendererRuntimeFrameSummary.hardware_batch_end_count++;
#endif
}

static inline void ndsRendererProfileRecordTexturePrepare(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexturePrepareCount++;
#else
    sNdsRendererRuntimeFrameSummary.texture_prepare_count++;
#endif
}

static inline void ndsRendererProfileRecordTexturePrepareReuse(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexturePrepareReuseCount++;
#else
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count++;
#endif
}

static inline void ndsRendererProfileRecordVertexSaturate(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHWVertexSaturateCount++;
#else
    sNdsRendererRuntimeFrameSummary.hardware_vertex_saturate_count++;
#endif
}

static inline void ndsRendererProfileRecordProjectedSubmit(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileProjectedSubmitFallbackCount++;
#else
    sNdsRendererRuntimeFrameSummary.projected_submit_fallback_count++;
#endif
}

static inline void ndsRendererProfileRecordRawCurrentCandidate(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileRawCurrentCandidateCount++;
#else
    sNdsRendererRuntimeFrameSummary.raw_current_candidate_count++;
#endif
}

static inline void ndsRendererProfileRecordRawCurrentRangeReject(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileRawCurrentRangeRejectCount++;
#else
    sNdsRendererRuntimeFrameSummary.raw_current_range_reject_count++;
#endif
}

static inline void ndsRendererProfileRecordRawCrossMatrix(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileRawCrossMatrixCount++;
#else
    sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count++;
#endif
}

static inline void ndsRendererProfileRecordSubmitClass(
    NDSRendererHWSubmitClass submit_class)
{
    if ((u32)submit_class < NDS_RENDERER_HW_SUBMIT_CLASS_COUNT)
    {
        sNdsRendererHardwareSubmitClassCounts[submit_class]++;
    }
}

static inline void ndsRendererProfileRecordProjectedDivisions(u32 count)
{
    sNdsRendererHardwareProjectedDivisionCount += count;
}

static inline void ndsRendererProfileRecordSourceVertexLoad(void)
{
    sNdsRendererHardwareSourceVertexLoadCount++;
}

static inline void ndsRendererProfileRecordCPUTransform(void)
{
    sNdsRendererHardwareCPUTransformCount++;
}

static inline void ndsRendererProfileRecordTransformCacheHit(void)
{
    sNdsRendererHardwareTransformCacheHitCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotCreate(void)
{
    sNdsRendererHardwareMatrixSnapshotCreateCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotReuse(void)
{
    sNdsRendererHardwareMatrixSnapshotReuseCount++;
}

static inline void ndsRendererProfileRecordMatrixSnapshotOverflow(void)
{
    sNdsRendererHardwareMatrixSnapshotOverflowCount++;
}

static void ndsRendererProfileResetSubmitSummary(void)
{
    memset(sNdsRendererHardwareSubmitClassCounts, 0,
           sizeof(sNdsRendererHardwareSubmitClassCounts));
    sNdsRendererHardwareProjectedDivisionCount = 0u;
    sNdsRendererHardwareSourceVertexLoadCount = 0u;
    sNdsRendererHardwareCPUTransformCount = 0u;
    sNdsRendererHardwareTransformCacheHitCount = 0u;
    sNdsRendererHardwareMatrixSnapshotCreateCount = 0u;
    sNdsRendererHardwareMatrixSnapshotReuseCount = 0u;
    sNdsRendererHardwareMatrixSnapshotOverflowCount = 0u;
}

static void ndsRendererProfilePublishSubmitSummary(void)
{
    gNdsRendererProfileLevel = NDS_RENDERER_PROFILE_LEVEL;
    gNdsRendererProfileSubmitRawCurrentCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX];
    gNdsRendererProfileSubmitRawSnapshotCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX];
    gNdsRendererProfileSubmitProjectedCrossCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX];
    gNdsRendererProfileSubmitProjectedNoZCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z];
    gNdsRendererProfileSubmitProjectedDecalCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL];
    gNdsRendererProfileSubmitProjectedPrimDepthCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH];
    gNdsRendererProfileSubmitProjectedRangeOrMatrixCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX];
    gNdsRendererProfileSubmitRejectCount =
        sNdsRendererHardwareSubmitClassCounts[
            NDS_RENDERER_HW_SUBMIT_REJECT];
    gNdsRendererProfileProjectedDivisionCount =
        sNdsRendererHardwareProjectedDivisionCount;
    gNdsRendererProfileSourceVertexLoadCount =
        sNdsRendererHardwareSourceVertexLoadCount;
    gNdsRendererProfileCPUTransformCount =
        sNdsRendererHardwareCPUTransformCount;
    gNdsRendererProfileTransformCacheHitCount =
        sNdsRendererHardwareTransformCacheHitCount;
    gNdsRendererProfileMatrixSnapshotCreateCount =
        sNdsRendererHardwareMatrixSnapshotCreateCount;
    gNdsRendererProfileMatrixSnapshotReuseCount =
        sNdsRendererHardwareMatrixSnapshotReuseCount;
    gNdsRendererProfileMatrixSnapshotOverflowCount =
        sNdsRendererHardwareMatrixSnapshotOverflowCount;
}

static inline void ndsRendererProfileRecordHardwareTriangle(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileHardwareTriangles++;
    gNdsRendererProfileHardwareVertices += 3u;
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#else
    sNdsRendererRuntimeFrameSummary.hardware_triangles++;
    sNdsRendererRuntimeFrameSummary.hardware_vertices += 3u;
    if ((sNdsRendererRuntimeFrameSummary.hardware_triangles > 2048u) ||
        (sNdsRendererRuntimeFrameSummary.hardware_vertices > 6144u))
    {
        sNdsRendererRuntimeFrameSummary.hardware_over_limit = 1u;
    }
#endif
}

typedef struct NDSRendererHardwareTextureKey
{
    u32 image;
    u32 image_format;
    u32 image_size;
    u32 image_width;
    u32 tlut_image;
    u32 tlut_count;
    u32 data_layout;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 render_tile;
    u32 render_tmem;
    u32 render_palette;
    u32 render_tile_cms;
    u32 render_tile_cmt;
    u32 render_tile_masks;
    u32 render_tile_maskt;
    u32 render_tile_shifts;
    u32 render_tile_shiftt;
    u32 load_tile;
    u32 load_uls;
    u32 load_ult;
    u32 load_lrs;
    u32 load_dxt;
    u32 load_texels;
    u32 tile_uls;
    u32 tile_ult;
    u32 tile_lrs;
    u32 tile_lrt;
    u32 line;
    u32 flags;
    u32 texel1_image;
    u32 texel1_image_format;
    u32 texel1_image_size;
    u32 texel1_image_width;
    u32 texel1_load_kind;
    u32 texel1_render_tmem;
    u32 texel1_render_line;
    u32 texel1_render_palette;
    u32 texel1_render_tile_cms;
    u32 texel1_render_tile_cmt;
    u32 texel1_render_tile_masks;
    u32 texel1_render_tile_maskt;
    u32 texel1_render_tile_shifts;
    u32 texel1_render_tile_shiftt;
    u32 texel1_load_tile;
    u32 texel1_load_uls;
    u32 texel1_load_ult;
    u32 texel1_load_lrs;
    u32 texel1_load_dxt;
    u32 texel1_load_texels;
    u32 texel1_tile_uls;
    u32 texel1_tile_ult;
    u32 texel1_tile_lrs;
    u32 texel1_tile_lrt;
    u32 prim_lod_fraction;
    u32 combine_w0;
    u32 combine_w1;
} NDSRendererHardwareTextureKey;

typedef struct NDSRendererHardwareTextureCacheEntry
{
    int name;
    u32 ready;
    u32 params;
    u32 source_texels;
    u32 green_texels;
    u32 nonwhite_texels;
    u32 profile_width;
    u32 profile_height;
    u32 last_used_frame;
    NDSRendererHardwareTextureKey key;
} NDSRendererHardwareTextureCacheEntry;

typedef struct NDSRendererHardwareTexel1Source
{
    const NDSRendererTextureLoadState *load;
    const NDSRendererTileState *render_tile;
    const u8 *texels;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 source_width;
    u32 source_extent_width;
    u32 source_extent_height;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 palette_base;
    s32 materialize_s;
    s32 materialize_t;
} NDSRendererHardwareTexel1Source;

typedef struct NDSRendererHardwareLightDirection
{
    s32 x;
    s32 y;
    s32 z;
} NDSRendererHardwareLightDirection;

static NDSRendererHardwareTextureCacheEntry
    sNdsRendererHardwareTextureCache[NDS_RENDERER_HW_TEXTURE_CACHE_COUNT];
static u32 sNdsRendererHardwareTextureCacheNext;
static u32 sNdsRendererHardwareFrameSerial;
static const NDSRendererHardwareTextureCacheEntry
    *sNdsRendererHardwareActiveTextureEntry;
static u16 sNdsRendererHardwareTextureScratch[
    NDS_RENDERER_HW_TEXTURE_MAX_TEXELS];
static u8 sNdsRendererHardwareTexel01Ci4Source0S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4Source1S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u16 sNdsRendererHardwareTexel01Ci4PhaseLut[
    NDS_RENDERER_HW_TEXEL01_CI4_PHASE_LUT_COUNT];
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareTexel01Ci4PhaseLut) == 8192u,
               "phase-resolved CI4 lookup must stay within 8 KiB");
#endif
static u16 sNdsRendererHardwareTexel01Ci4LutPalette0[16];
static u16 sNdsRendererHardwareTexel01Ci4LutPalette1[16];
static u32 sNdsRendererHardwareTexel01Ci4LutFraction;
static u32 sNdsRendererHardwareTexel01Ci4LutKeyValid;

static void ndsRendererHardwareEndBatch(void);

static inline s32 ndsRendererHardwareRawVertexFits(
    const NDSRendererInputVertex *vtx)
{
    if (vtx == NULL)
    {
        return FALSE;
    }
    return ((vtx->x >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->x <= NDS_RENDERER_HW_RAW_COORD_MAX) &&
            (vtx->y >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->y <= NDS_RENDERER_HW_RAW_COORD_MAX) &&
            (vtx->z >= NDS_RENDERER_HW_RAW_COORD_MIN) &&
            (vtx->z <= NDS_RENDERER_HW_RAW_COORD_MAX)) ? TRUE : FALSE;
}
#endif

typedef struct NDSRendererTraversalState
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 matrix;
    Mtx matrix_word_raw;
    NDSRendererMatrix20p12 modelview_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    NDSRendererClipVertex20p12 vertices[NDS_RENDERER_MAX_VTX];
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererInputVertex input_vertices[NDS_RENDERER_MAX_VTX];
    u32 vertex_colors[NDS_RENDERER_MAX_VTX];
    NDSRendererMatrixSnapshot *matrix_snapshots;
    u8 vertex_matrix_snapshot[NDS_RENDERER_MAX_VTX];
    u8 vertex_clip_snapshot[NDS_RENDERER_MAX_VTX];
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
    u32 texture_prepare_valid;
    u32 texture_prepare_enabled;
    u32 texture_prepare_implicit_on;
    u32 texture_prepare_name;
    u32 texture_prepare_material_color;
    u32 texture_prepare_scale_s;
    u32 texture_prepare_scale_t;
    s32 texture_prepare_offset;
    u32 texture_prepare_use_material_color;
    u32 texture_prepare_use_vertex_color;
    u32 texture_prepare_source_zbuffered;
    u32 texture_prepare_decal_depth;
    u32 texture_prepare_prim_depth;
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
typedef struct NDSRendererHardwareVertexContext
{
    NDSRendererStats *stats;
    NDSRendererTraversalState *state;
    const NDSRendererTileState *render_tile;
    u32 material_color;
    u32 scale_s;
    u32 scale_t;
    u32 texture_origin_s;
    u32 texture_origin_t;
    u32 flags;
    s32 texture_offset;
} NDSRendererHardwareVertexContext;
#if defined(__arm__)
_Static_assert(sizeof(NDSRendererHardwareVertexContext) == 40u,
               "DS vertex-submit context must stay compact");
#endif
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
typedef struct NDSRendererHardwarePendingPosTest
{
    NDSRendererMatrix20p12 matrix;
    NDSRendererInputVertex input;
    NDSRendererClipVertex20p12 clip;
    u32 generation;
    u32 matrix_word;
} NDSRendererHardwarePendingPosTest;

static NDSRendererHardwarePendingPosTest
    sNdsRendererHardwarePendingPosTests[NDS_RENDERER_HW_POS_TEST_MAX];
static u32 sNdsRendererHardwarePendingPosTestCount;
static u32 sNdsRendererHardwarePendingPosTestLastGeneration;
#endif

#if NDS_RENDERER_HW_TRIANGLES
static void ndsRendererHardwarePrepareLitDirection(
    const NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *out);
static u32 ndsRendererHardwareLitShadeColorPrepared(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction);
static u32 ndsRendererHardwareLitShadeColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererMatrix20p12 *modelview);
#endif

static s32 ndsRendererClampS64ToS32(s64 value)
{
    if (value > (s64)INT_MAX)
    {
        return INT_MAX;
    }
    if (value < (s64)INT_MIN)
    {
        return INT_MIN;
    }
    return (s32)value;
}

static s32 ndsRendererRoundShiftS32(s32 value, u32 shift)
{
    s64 wide;
    s64 bias;

    if (shift == 0)
    {
        return value;
    }

    wide = value;
    bias = (s64)(1u << (shift - 1u));
    if (wide < 0)
    {
        return (s32)(-(((-wide) + bias) >> shift));
    }
    return (s32)((wide + bias) >> shift);
}

static s64 ndsRendererRoundShiftS64(s64 value, u32 shift)
{
    s64 bias;

    if (shift == 0)
    {
        return value;
    }

    bias = (s64)(1u << (shift - 1u));
    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

s32 ndsRendererMtxCellS16p16(const Mtx *mtx, u32 row, u32 col)
{
    const u32 *ai;
    const u32 *af;
    u32 pair;
    u32 hi;
    u32 lo;

    if ((mtx == NULL) || (row >= 4u) || (col >= 4u))
    {
        return 0;
    }

    ai = (const u32 *)&mtx->m[0][0];
    af = (const u32 *)&mtx->m[2][0];
    pair = (row * 2u) + (col / 2u);
    hi = ai[pair];
    lo = af[pair];

    if ((col & 1u) == 0)
    {
        return (s32)((hi & 0xffff0000u) | ((lo >> 16) & 0xffffu));
    }
    return (s32)(((hi << 16) & 0xffff0000u) | (lo & 0xffffu));
}

static s32 ndsRendererMtxCell20p12ToS16p16(s32 value)
{
    return ndsRendererClampS64ToS32(
        (s64)value << (NDS_RENDERER_N64_MTX_FRAC_BITS -
                       NDS_RENDERER_DS_MTX_FRAC_BITS));
}

static void ndsRendererMtxStoreCellS16p16(Mtx *mtx, u32 row, u32 col,
                                          s32 value)
{
    u32 *ai;
    u32 *af;
    u32 pair;
    u32 ui;

    if ((mtx == NULL) || (row >= 4u) || (col >= 4u))
    {
        return;
    }

    ai = (u32 *)&mtx->m[0][0];
    af = (u32 *)&mtx->m[2][0];
    pair = (row * 2u) + (col / 2u);
    ui = (u32)value;
    if ((col & 1u) == 0)
    {
        ai[pair] = (ai[pair] & 0x0000ffffu) | (ui & 0xffff0000u);
        af[pair] = (af[pair] & 0x0000ffffu) |
            ((ui << 16) & 0xffff0000u);
    }
    else
    {
        ai[pair] = (ai[pair] & 0xffff0000u) | ((ui >> 16) & 0xffffu);
        af[pair] = (af[pair] & 0xffff0000u) | (ui & 0xffffu);
    }
}

static void ndsRendererMtxStoreDS20p12ToN64(
    const NDSRendererMatrix20p12 *src, Mtx *dst)
{
    u32 row;
    u32 col;

    if (dst == NULL)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    if (src == NULL)
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            ndsRendererMtxStoreCellS16p16(
                dst, row, col,
                ndsRendererMtxCell20p12ToS16p16(src->m[row][col]));
        }
    }
}

void ndsRendererMtxLoadN64ToDS20p12(const Mtx *src,
                                    NDSRendererMatrix20p12 *dst)
{
    u32 row;
    u32 col;
    const u32 shift =
        NDS_RENDERER_N64_MTX_FRAC_BITS - NDS_RENDERER_DS_MTX_FRAC_BITS;

    if (dst == NULL)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    if (src == NULL)
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            dst->m[row][col] =
                ndsRendererRoundShiftS32(
                    ndsRendererMtxCellS16p16(src, row, col), shift);
        }
    }
}

void ndsRendererTransformVertex20p12(const NDSRendererMatrix20p12 *mtx,
                                     const NDSRendererInputVertex *vtx,
                                     NDSRendererClipVertex20p12 *out)
{
    s64 x;
    s64 y;
    s64 z;

    if ((mtx == NULL) || (vtx == NULL) || (out == NULL))
    {
        return;
    }

    x = vtx->x;
    y = vtx->y;
    z = vtx->z;

    out->x = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][0] * x + (s64)mtx->m[1][0] * y +
        (s64)mtx->m[2][0] * z + mtx->m[3][0]);
    out->y = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][1] * x + (s64)mtx->m[1][1] * y +
        (s64)mtx->m[2][1] * z + mtx->m[3][1]);
    out->z = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][2] * x + (s64)mtx->m[1][2] * y +
        (s64)mtx->m[2][2] * z + mtx->m[3][2]);
    out->w = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][3] * x + (s64)mtx->m[1][3] * y +
        (s64)mtx->m[2][3] * z + mtx->m[3][3]);
}

void ndsRendererMtxMul20p12(const NDSRendererMatrix20p12 *lhs,
                            const NDSRendererMatrix20p12 *rhs,
                            NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 temp;
    u32 row;
    u32 col;
    u32 k;

    if ((lhs == NULL) || (rhs == NULL) || (out == NULL))
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            s64 sum = 0;

            for (k = 0; k < 4u; k++)
            {
                sum += (s64)lhs->m[row][k] * rhs->m[k][col];
            }
            temp.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
    }

    *out = temp;
}

static void ndsRendererMtxIdentity20p12(NDSRendererMatrix20p12 *out)
{
    u32 i;

    if (out == NULL)
    {
        return;
    }

    memset(out, 0, sizeof(*out));
    for (i = 0; i < 4u; i++)
    {
        out->m[i][i] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
    }
}

#if NDS_RENDERER_HW_TRIANGLES
static u32 ndsRendererNextMatrixGeneration(void)
{
    sNdsRendererMatrixGenerationSerial++;
    if (sNdsRendererMatrixGenerationSerial == 0u)
    {
        sNdsRendererMatrixGenerationSerial = 1u;
        sNdsRendererHardwareMatrixLoaded = FALSE;
        sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
        sNdsRendererHardwareMatrixGeneration = 0u;
    }
    return sNdsRendererMatrixGenerationSerial;
}
#endif

static u32 ndsRendererReadU32(const void *ptr)
{
    const u8 *bytes = ptr;

    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

static void ndsRendererDecodeInputVertex(NDSRendererInputVertex *dst,
                                         const void *src)
{
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    xy = ndsRendererReadU32(src);
    zf = ndsRendererReadU32((const u8 *)src + 4);
    st = ndsRendererReadU32((const u8 *)src + 8);
    rgba = ndsRendererReadU32((const u8 *)src + 12);
    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = (u8)(rgba >> 24);
    dst->g = (u8)(rgba >> 16);
    dst->b = (u8)(rgba >> 8);
    dst->a = (u8)rgba;
    if (dst->a == 0u)
    {
        dst->a = 0xffu;
    }
}

static s32 ndsRendererValidateCommand(const Gfx *dl,
                                       const NDSRendererConfig *config)
{
    uintptr_t addr = (uintptr_t)dl;

#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsRendererProfileValidatedCommandCount++;
#endif

    if ((dl == NULL) || ((addr & 0x3u) != 0))
    {
        return FALSE;
    }
    if (config->validate_range == NULL)
    {
        return TRUE;
    }
    return config->validate_range(dl, sizeof(*dl), config->user);
}

static const void *ndsRendererResolveDataPointer(
    const NDSRendererConfig *config, const void *ptr, size_t bytes)
{
    uintptr_t addr = (uintptr_t)ptr;

    if ((ptr == NULL) || ((addr & 0x3u) != 0))
    {
        return NULL;
    }
    if ((config != NULL) && (config->resolve_data != NULL))
    {
        return config->resolve_data(ptr, bytes, config->user);
    }
    if ((config != NULL) && (config->validate_range != NULL) &&
        (config->validate_range((const Gfx *)ptr, bytes, config->user) ==
         FALSE))
    {
        return NULL;
    }
    return ptr;
}

static void ndsRendererRecordUnsupported(NDSRendererStats *stats, u32 op)
{
    if (stats->unsupported_opcode == 0)
    {
        stats->unsupported_opcode = op;
    }
    stats->unsupported_command_count++;
}

static void ndsRendererRecordOtherMode(NDSRendererStats *stats,
                                       u32 op, u32 w0, u32 w1)
{
    u32 bits;
    u32 pos;
    u32 shift;
    u32 mask;

    stats->state_command_count++;
    stats->othermode_command_count++;
    stats->ignored_state_command_count++;
    if (stats->first_othermode_opcode == 0)
    {
        stats->first_othermode_opcode = op;
        stats->first_othermode_w0 = w0;
        stats->first_othermode_w1 = w1;
    }

    if (op == NDS_RENDERER_OP_RDPSETOTHERMODE)
    {
        stats->othermode_h = w0 & 0x00ffffffu;
        stats->othermode_l = w1;
        return;
    }
    if ((op != NDS_RENDERER_OP_SETOTHERMODE_H) &&
        (op != NDS_RENDERER_OP_SETOTHERMODE_L))
    {
        return;
    }

    bits = (w0 & 0xffu) + 1u;
    pos = (w0 >> 8) & 0xffu;
    if ((bits > 32u) || (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    if (op == NDS_RENDERER_OP_SETOTHERMODE_H)
    {
        stats->othermode_h = (stats->othermode_h & ~mask) | (w1 & mask);
    }
    else
    {
        stats->othermode_l = (stats->othermode_l & ~mask) | (w1 & mask);
    }
}

static void ndsRendererRecordPrimDepth(NDSRendererStats *stats, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->prim_depth = (w1 >> 16) & 0xffffu;
    stats->prim_depth_delta = w1 & 0xffffu;
    stats->prim_depth_command_count++;
    stats->state_command_count++;
}

static void ndsRendererRecordCull(NDSRendererStats *stats, u32 w0, u32 w1)
{
    stats->cull_command_count++;
    stats->skip_command_count++;
    if ((stats->first_cull_w0 == 0) && (stats->first_cull_w1 == 0))
    {
        stats->first_cull_w0 = w0;
        stats->first_cull_w1 = w1;
    }
}

static void ndsRendererSyncTextureTile(NDSRendererStats *stats);

static void ndsRendererRecordTextureState(NDSRendererStats *stats,
                                          u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_TEXTURE;
    stats->texture_command_count++;
    stats->texture_xparam = (w0 >> 16) & 0xFFu;
    stats->texture_level = (w0 >> 11) & 0x7u;
    stats->texture_tile = (w0 >> 8) & 0x7u;
    stats->texture_on = (w0 >> 1) & 0x7Fu;
    stats->texture_scale_s = (w1 >> 16) & 0xFFFFu;
    stats->texture_scale_t = w1 & 0xFFFFu;
    stats->texture_state_flags = NDS_RENDERER_TEXTURE_STATE_SEEN;
    if (stats->texture_on != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_ON;
    }
    if (stats->texture_scale_s != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_S;
    }
    if (stats->texture_scale_t != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_T;
    }
    ndsRendererSyncTextureTile(stats);
}

static u32 ndsRendererTileFlags(u32 cms, u32 cmt, u32 masks, u32 maskt)
{
    u32 flags = 0u;

    if ((cms & NDS_RENDERER_TX_CLAMP) != 0) { flags |= NDS_RENDERER_TILE_S_CLAMP; }
    if ((cms & NDS_RENDERER_TX_MIRROR) != 0) { flags |= NDS_RENDERER_TILE_S_MIRROR; }
    if (masks != 0u) { flags |= NDS_RENDERER_TILE_S_MASKED; }
    if ((cmt & NDS_RENDERER_TX_CLAMP) != 0) { flags |= NDS_RENDERER_TILE_T_CLAMP; }
    if ((cmt & NDS_RENDERER_TX_MIRROR) != 0) { flags |= NDS_RENDERER_TILE_T_MIRROR; }
    if (maskt != 0u) { flags |= NDS_RENDERER_TILE_T_MASKED; }
    return flags;
}

static u32 ndsRendererActiveTextureTile(const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_SEEN) != 0u))
    {
        return stats->texture_tile & 0x7u;
    }
    return NDS_RENDERER_RENDER_TILE;
}

static void ndsRendererSyncTextureTile(NDSRendererStats *stats)
{
    u32 tile_index;
    const NDSRendererTileState *tile;
    u32 flags = 0u;

    if (stats == NULL)
    {
        return;
    }

    tile_index = ndsRendererActiveTextureTile(stats);
    tile = &stats->texture_tiles[tile_index];

    stats->texture_render_tile = tile_index;
    stats->texture_render_tile_format = tile->format;
    stats->texture_render_tile_size = tile->size;
    stats->texture_render_tile_line = tile->line;
    stats->texture_render_tile_tmem = tile->tmem;
    stats->texture_render_tile_palette = tile->palette;
    stats->texture_render_tile_cms = tile->cms;
    stats->texture_render_tile_cmt = tile->cmt;
    stats->texture_render_tile_masks = tile->masks;
    stats->texture_render_tile_maskt = tile->maskt;
    stats->texture_render_tile_shifts = tile->shifts;
    stats->texture_render_tile_shiftt = tile->shiftt;
    stats->texture_tile_size_tile = tile_index;
    stats->texture_tile_size_uls = tile->uls;
    stats->texture_tile_size_ult = tile->ult;
    stats->texture_tile_size_lrs = tile->lrs;
    stats->texture_tile_size_lrt = tile->lrt;
    stats->texture_tile_width = tile->width;
    stats->texture_tile_height = tile->height;
    if (tile->set_seen != 0u)
    {
        flags |= NDS_RENDERER_TILE_RENDER_SEEN | tile->flags;
    }
    if (stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen != 0u)
    {
        flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }
    stats->texture_render_tile_flags = flags;
}

static void ndsRendererRecordSetTile(NDSRendererStats *stats,
                                     u32 w0, u32 w1)
{
    u32 tile;
    u32 fmt;
    u32 siz;
    u32 line;
    u32 tmem;
    u32 palette;
    u32 cmt;
    u32 maskt;
    u32 shiftt;
    u32 cms;
    u32 masks;
    u32 shifts;
    NDSRendererTileState *tile_state;

    if (stats == NULL)
    {
        return;
    }

    tile = (w1 >> 24) & 0x7u;
    fmt = (w0 >> 21) & 0x7u;
    siz = (w0 >> 19) & 0x3u;
    line = (w0 >> 9) & 0x1FFu;
    tmem = w0 & 0x1FFu;
    palette = (w1 >> 20) & 0xFu;
    cmt = (w1 >> 18) & 0x3u;
    maskt = (w1 >> 14) & 0xFu;
    shiftt = (w1 >> 10) & 0xFu;
    cms = (w1 >> 8) & 0x3u;
    masks = (w1 >> 4) & 0xFu;
    shifts = w1 & 0xFu;

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILE;
    stats->texture_set_tile_count++;

    tile_state = &stats->texture_tiles[tile];
    tile_state->set_seen = 1u;
    tile_state->format = fmt;
    tile_state->size = siz;
    tile_state->line = line;
    tile_state->tmem = tmem;
    tile_state->palette = palette;
    tile_state->cms = cms;
    tile_state->cmt = cmt;
    tile_state->masks = masks;
    tile_state->maskt = maskt;
    tile_state->shifts = shifts;
    tile_state->shiftt = shiftt;
    tile_state->flags = ndsRendererTileFlags(cms, cmt, masks, maskt);

    if (tile == NDS_RENDERER_LOAD_TILE)
    {
        stats->texture_load_tile = tile;
    }

    ndsRendererSyncTextureTile(stats);
}

static void ndsRendererCaptureTextureLoad(NDSRendererStats *stats)
{
    NDSRendererTextureLoadState *load;
    u32 tile;

    if (stats == NULL)
    {
        return;
    }

    tile = stats->texture_load_tile & 0x7u;
    stats->texture_load_sequence++;
    load = &stats->texture_loads[
        (stats->texture_load_sequence - 1u) %
        NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT];
    memset(load, 0, sizeof(*load));
    load->sequence = stats->texture_load_sequence;
    load->image = stats->texture_image;
    load->image_format = stats->texture_format;
    load->image_size = stats->texture_size;
    load->image_width = stats->texture_image_width;
    load->load_kind = stats->texture_load_kind;
    load->load_tile = tile;
    load->load_uls = stats->texture_load_block_uls;
    load->load_ult = stats->texture_load_block_ult;
    load->load_lrs = stats->texture_load_block_lrs;
    load->load_dxt = stats->texture_load_block_dxt;
    /* The compact per-TMEM record deliberately stores only bounded texture
     * loads. A large LOADTILE rectangle must not wrap into a plausible small
     * source span and later pass the residency checks. */
    load->load_texels = (stats->texture_load_texels <= 0xffffu) ?
        (u16)stats->texture_load_texels : 0u;
    load->load_tmem = stats->texture_tiles[tile].tmem;
    load->valid = ((load->image != 0u) &&
                   (load->load_texels != 0u) &&
                   (stats->texture_tiles[tile].set_seen != 0u)) ? TRUE : FALSE;
}

static void ndsRendererRecordLoadBlock(NDSRendererStats *stats,
                                       u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_kind = NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_tile = (w1 >> 24) & 0x7u;
    stats->texture_load_block_uls = (w0 >> 12) & 0x0FFFu;
    stats->texture_load_block_ult = w0 & 0x0FFFu;
    stats->texture_load_block_lrs = (w1 >> 12) & 0x0FFFu;
    stats->texture_load_block_dxt = w1 & 0x0FFFu;
    stats->texture_load_texels = stats->texture_load_block_lrs + 1u;
    ndsRendererCaptureTextureLoad(stats);
}

static void ndsRendererRecordLoadTile(NDSRendererStats *stats,
                                      u32 w0, u32 w1)
{
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    u32 width;
    u32 height;

    if (stats == NULL)
    {
        return;
    }

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADTILE;
    stats->texture_load_kind = NDS_RENDERER_TEXTURE_LOADTILE;
    stats->texture_load_tile = (w1 >> 24) & 0x7u;
    stats->texture_load_block_uls = uls;
    stats->texture_load_block_ult = ult;
    stats->texture_load_block_lrs = lrs;
    stats->texture_load_block_dxt = lrt;
    stats->texture_load_texels = 0u;
    if ((lrs >= uls) && (lrt >= ult))
    {
        width = ((lrs - uls) >> 2) + 1u;
        height = ((lrt - ult) >> 2) + 1u;
        stats->texture_load_texels = width * height;
    }
    ndsRendererCaptureTextureLoad(stats);
}

static void ndsRendererRecordSetTileSize(NDSRendererStats *stats,
                                         u32 w0, u32 w1)
{
    u32 tile_index;
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    NDSRendererTileState *tile;

    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILESIZE;

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    tile_index = (w1 >> 24) & 0x7u;
    tile = &stats->texture_tiles[tile_index];
    tile->size_seen = 1u;
    tile->uls = uls;
    tile->ult = ult;
    tile->lrs = lrs;
    tile->lrt = lrt;
    tile->width = 0u;
    tile->height = 0u;
    if (lrs >= uls)
    {
        tile->width = ((lrs - uls) >> 2) + 1u;
    }
    if (lrt >= ult)
    {
        tile->height = ((lrt - ult) >> 2) + 1u;
    }
    ndsRendererSyncTextureTile(stats);
}

static void ndsRendererRecordSetImage(NDSRendererStats *stats,
                                      u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTIMG;
    stats->texture_format = (w0 >> 21) & 0x7u;
    stats->texture_size = (w0 >> 19) & 0x3u;
    stats->texture_image_width = (w0 & 0x0FFFu) + 1u;
    stats->texture_image = w1;
}

static void ndsRendererRecordLoadTlut(NDSRendererStats *stats, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_command_count++;
    stats->texture_tlut_tile = (w1 >> 24) & 0x7u;
    stats->texture_tlut_count = ((w1 >> 14) & 0x3ffu) + 1u;
    if (stats->texture_image != 0u)
    {
        stats->texture_tlut_image = stats->texture_image;
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileCombineMode(u32 w0, u32 w1)
{
    gNdsRendererProfileCombineModeCount++;
    if (((gNdsRendererProfileCombineMode0W0 == w0) &&
         (gNdsRendererProfileCombineMode0W1 == w1)) ||
        ((gNdsRendererProfileCombineMode1W0 == w0) &&
         (gNdsRendererProfileCombineMode1W1 == w1)) ||
        ((gNdsRendererProfileCombineMode2W0 == w0) &&
         (gNdsRendererProfileCombineMode2W1 == w1)) ||
        ((gNdsRendererProfileCombineMode3W0 == w0) &&
         (gNdsRendererProfileCombineMode3W1 == w1)))
    {
        return;
    }

    switch (gNdsRendererProfileCombineModeDistinctCount)
    {
    case 0:
        gNdsRendererProfileCombineMode0W0 = w0;
        gNdsRendererProfileCombineMode0W1 = w1;
        break;
    case 1:
        gNdsRendererProfileCombineMode1W0 = w0;
        gNdsRendererProfileCombineMode1W1 = w1;
        break;
    case 2:
        gNdsRendererProfileCombineMode2W0 = w0;
        gNdsRendererProfileCombineMode2W1 = w1;
        break;
    case 3:
        gNdsRendererProfileCombineMode3W0 = w0;
        gNdsRendererProfileCombineMode3W1 = w1;
        break;
    default:
        break;
    }
    gNdsRendererProfileCombineModeDistinctCount++;
}
#endif

static void ndsRendererRecordSetCombine(NDSRendererStats *stats,
                                        u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETCOMBINE;
    stats->texture_combine_count++;
    stats->texture_combine_w0 = w0;
    stats->texture_combine_w1 = w1;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileCombineMode(w0, w1);
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static u32 ndsRendererMatrixSnapshotSignature(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 signature = 2166136261u;
    u32 row;
    u32 col;

    if (matrix == NULL)
    {
        return 0u;
    }
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            signature ^= (u32)matrix->m[row][col];
            signature *= 16777619u;
        }
    }
    return signature;
}

static const NDSRendererMatrixSnapshot *ndsRendererGetMatrixSnapshot(
    const NDSRendererTraversalState *state, u32 snapshot_id)
{
    if ((state == NULL) || (state->matrix_snapshots == NULL) ||
        (snapshot_id == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID) ||
        (snapshot_id > state->matrix_snapshot_count))
    {
        return NULL;
    }
    return &state->matrix_snapshots[snapshot_id - 1u];
}

static u32 ndsRendererAcquireCurrentMatrixSnapshot(
    NDSRendererTraversalState *state)
{
    NDSRendererMatrixSnapshot *snapshot;
    u32 signature;
    u32 i;

    if ((state == NULL) || (state->matrix_valid == 0u) ||
        (state->matrix_snapshots == NULL))
    {
        return NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }
    if (state->current_matrix_snapshot !=
        NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
    {
        return state->current_matrix_snapshot;
    }

    signature = ndsRendererMatrixSnapshotSignature(&state->matrix);
    for (i = 0u; i < state->matrix_snapshot_count; i++)
    {
        snapshot = &state->matrix_snapshots[i];
        if ((snapshot->signature == signature) &&
            (memcmp(&snapshot->matrix, &state->matrix,
                    sizeof(snapshot->matrix)) == 0))
        {
            state->current_matrix_snapshot = i + 1u;
            ndsRendererProfileRecordMatrixSnapshotReuse();
            return state->current_matrix_snapshot;
        }
    }
    if (state->matrix_snapshot_count >=
        NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY)
    {
        ndsRendererProfileRecordMatrixSnapshotOverflow();
        return NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }

    snapshot = &state->matrix_snapshots[state->matrix_snapshot_count++];
    snapshot->matrix = state->matrix;
    snapshot->generation = state->matrix_generation;
    snapshot->signature = signature;
    state->current_matrix_snapshot = state->matrix_snapshot_count;
    ndsRendererProfileRecordMatrixSnapshotCreate();
    return state->current_matrix_snapshot;
}

static s32 ndsRendererTransformCachedVertex(
    NDSRendererStats *stats, NDSRendererTraversalState *state, u32 index,
    const NDSRendererMatrix20p12 *matrix, u32 snapshot_id)
{
    NDSRendererClipVertex20p12 *out;
    u32 mask;

    if ((stats == NULL) || (state == NULL) || (matrix == NULL) ||
        (index >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    mask = 1u << index;
    if ((state->input_vertex_valid_mask & mask) == 0u)
    {
        return FALSE;
    }

    out = &state->vertices[index];
    ndsRendererTransformVertex20p12(matrix, &state->input_vertices[index], out);
    state->vertex_valid_mask |= mask;
    state->vertex_clip_snapshot[index] = (u8)snapshot_id;
    stats->matrix_transform_count++;
    stats->transformed_vertex_count++;
    if (stats->transformed_vertex_count == 1u)
    {
        stats->first_transformed_x = out->x;
        stats->first_transformed_y = out->y;
        stats->first_transformed_z = out->z;
        stats->first_transformed_w = out->w;
    }
    ndsRendererProfileRecordCPUTransform();
    return TRUE;
}

static s32 ndsRendererEnsureTransformedVertex(
    NDSRendererStats *stats, NDSRendererTraversalState *state, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot;
    u32 snapshot_id;
    u32 mask;

    if ((state == NULL) || (index >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    mask = 1u << index;
    snapshot_id = state->vertex_matrix_snapshot[index];
    if (((state->vertex_valid_mask & mask) != 0u) &&
        (state->vertex_clip_snapshot[index] == snapshot_id))
    {
        ndsRendererProfileRecordTransformCacheHit();
        return TRUE;
    }

    snapshot = ndsRendererGetMatrixSnapshot(state, snapshot_id);
    if (snapshot == NULL)
    {
        return FALSE;
    }
    return ndsRendererTransformCachedVertex(
        stats, state, index, &snapshot->matrix, snapshot_id);
}
#endif

static void ndsRendererComposeMatrix(NDSRendererTraversalState *state)
{
    NDSRendererMatrix20p12 identity;

    if (state == NULL)
    {
        return;
    }

    if ((state->projection_valid != 0u) &&
        (state->modelview_valid != 0u))
    {
        ndsRendererMtxMul20p12(&state->modelview,
                               &state->projection,
                               &state->matrix);
    }
    else if (state->modelview_valid != 0u)
    {
        state->matrix = state->modelview;
    }
    else if (state->projection_valid != 0u)
    {
        state->matrix = state->projection;
    }
    else
    {
        ndsRendererMtxIdentity20p12(&identity);
        state->matrix = identity;
    }
    state->matrix_valid =
        ((state->projection_valid != 0u) ||
         (state->modelview_valid != 0u)) ? TRUE : FALSE;
    state->matrix_word_valid = FALSE;
#if NDS_RENDERER_HW_TRIANGLES
    /* Cached RSP vertices retain the transform active when they were loaded. */
    state->current_transform_vertex_mask = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
#endif
}

static void ndsRendererInitMatrixWordRaw(NDSRendererTraversalState *state)
{
    NDSRendererMatrix20p12 identity;

    if (state == NULL)
    {
        return;
    }

    if (state->matrix_valid == 0u)
    {
        ndsRendererMtxIdentity20p12(&identity);
        ndsRendererMtxStoreDS20p12ToN64(&identity, &state->matrix_word_raw);
    }
    else
    {
        ndsRendererMtxStoreDS20p12ToN64(&state->matrix,
                                        &state->matrix_word_raw);
    }
    state->matrix_word_valid = TRUE;
}

static void ndsRendererInitTraversalState(NDSRendererTraversalState *state,
                                          const NDSRendererConfig *config,
                                          NDSRendererStats *stats,
                                          NDSRendererMatrixSnapshot *snapshots,
                                          u32 snapshot_count)
{
    if (state == NULL)
    {
        return;
    }

    memset(state, 0, sizeof(*state));
#if NDS_RENDERER_HW_TRIANGLES
    state->matrix_snapshots = snapshots;
    state->matrix_snapshot_count =
        (snapshot_count <= NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY) ?
            snapshot_count : NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
#else
    (void)snapshots;
    (void)snapshot_count;
#endif
    if (config == NULL)
    {
        return;
    }
    if ((stats != NULL) && (config->initial_geometry_mode != 0u))
    {
        stats->geometry_mode = config->initial_geometry_mode;
    }
    if (config->initial_projection != NULL)
    {
        state->projection = *config->initial_projection;
        state->projection_valid = TRUE;
    }
    if (config->initial_modelview != NULL)
    {
        state->modelview = *config->initial_modelview;
        state->modelview_valid = TRUE;
    }
    ndsRendererComposeMatrix(state);
    if ((stats != NULL) && (state->matrix_valid != 0u))
    {
        stats->hardware_matrix_seed_count++;
    }
}

static void ndsRendererPushModelview(NDSRendererStats *stats,
                                     NDSRendererTraversalState *state)
{
    u32 depth;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    depth = state->modelview_stack_depth;
    if (depth >= NDS_RENDERER_MODELVIEW_STACK_SIZE)
    {
        stats->skip_command_count++;
        return;
    }

    state->modelview_stack[depth] = state->modelview;
    state->modelview_valid_stack[depth] = state->modelview_valid;
    state->modelview_stack_depth = depth + 1u;
}

static void ndsRendererApplyMatrixCommand(
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    const Mtx *src;
    NDSRendererMatrix20p12 incoming;
    NDSRendererMatrix20p12 *target;
    u32 *target_valid;
    u32 flags;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    flags = (w0 & 0xffu) ^ NDS_RENDERER_MTX_PUSH_XOR;
    stats->state_command_count++;
    stats->matrix_command_count++;
    stats->matrix_flags = flags;
    if ((flags & NDS_RENDERER_MTX_PROJECTION) != 0u)
    {
        stats->matrix_projection_count++;
    }
    else
    {
        stats->matrix_modelview_count++;
    }
    if ((flags & NDS_RENDERER_MTX_PUSH) != 0u)
    {
        stats->matrix_push_count++;
    }

    src = ndsRendererResolveDataPointer(config,
                                        (const void *)(uintptr_t)w1,
                                        sizeof(Mtx));
    if (src == NULL)
    {
        stats->skip_command_count++;
        return;
    }

    ndsRendererMtxLoadN64ToDS20p12(src, &incoming);
    if ((flags & NDS_RENDERER_MTX_PROJECTION) != 0u)
    {
        target = &state->projection;
        target_valid = &state->projection_valid;
    }
    else
    {
        target = &state->modelview;
        target_valid = &state->modelview_valid;
        if ((flags & NDS_RENDERER_MTX_PUSH) != 0u)
        {
            ndsRendererPushModelview(stats, state);
        }
    }

    if ((flags & NDS_RENDERER_MTX_LOAD) != 0u)
    {
        *target = incoming;
        *target_valid = TRUE;
        stats->matrix_load_count++;
    }
    else
    {
        if (*target_valid != 0u)
        {
            ndsRendererMtxMul20p12(target, &incoming, target);
        }
        else
        {
            *target = incoming;
            *target_valid = TRUE;
        }
        stats->matrix_mul_count++;
    }
    ndsRendererComposeMatrix(state);
}

static void ndsRendererApplyMvpRecalcCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    stats->state_command_count++;
    if ((((w0 >> NDS_RENDERER_MOVEWORD_OFFSET_SHIFT) &
          NDS_RENDERER_MOVEWORD_OFFSET_MASK) != 0u) ||
        ((w0 & NDS_RENDERER_MOVEWORD_INDEX_MASK) != 1u) ||
        (w1 != 0u))
    {
        ndsRendererRecordUnsupported(stats, NDS_RENDERER_OP_SPECIAL_1);
        return;
    }

    stats->matrix_command_count++;
    stats->matrix_mvp_recalc_count++;
    ndsRendererInitMatrixWordRaw(state);
}

static void ndsRendererRecordFogMoveWord(NDSRendererStats *stats, u32 w1)
{
    s32 mul;
    s32 ofs;
    s32 fog_min;
    s32 fog_max;

    if ((stats == NULL) || (stats->fog_status >= 2u))
    {
        return;
    }

    mul = (s16)(w1 >> 16);
    ofs = (s16)w1;
    if (mul == 0)
    {
        stats->ignored_state_command_count++;
        return;
    }

    fog_min = 500 - ((ofs * 500) / mul);
    fog_max = (128000 / mul) + fog_min;
    if ((stats->fog_status == 0u) ||
        (stats->fog_min != fog_min) ||
        (stats->fog_max != fog_max))
    {
        stats->fog_status++;
        stats->fog_min = fog_min;
        stats->fog_max = fog_max;
    }
}

static void ndsRendererRecordFogColor(NDSRendererStats *stats, u32 w1)
{
    if ((stats != NULL) && (stats->fog_status < 2u))
    {
        stats->fog_color = w1;
    }
}

static u32 ndsRendererPackLightColor(const u8 color[3])
{
    return ((u32)color[0] << 24) |
        ((u32)color[1] << 16) |
        ((u32)color[2] << 8);
}

static void ndsRendererRecordLightColor(NDSRendererStats *stats,
                                        u32 light, u32 color)
{
    if (stats == NULL)
    {
        return;
    }
    if (light == 1u)
    {
        stats->light_color_1 = color;
        stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK;
        stats->light_color_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightColorCommands++;
#endif
    }
    else if (light == 2u)
    {
        stats->light_color_2 = color;
        stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_2_MASK;
        stats->light_color_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightColorCommands++;
#endif
    }
}

static void ndsRendererRecordLightColorMoveWord(NDSRendererStats *stats,
                                                u32 offset, u32 color)
{
    switch (offset)
    {
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A:
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B:
        ndsRendererRecordLightColor(stats, 1u, color);
        break;

    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A:
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B:
        ndsRendererRecordLightColor(stats, 2u, color);
        break;

    default:
        if (stats != NULL)
        {
            stats->ignored_state_command_count++;
        }
        break;
    }
}

static void ndsRendererRecordLightMoveMem(
    const NDSRendererConfig *config, NDSRendererStats *stats, u32 w0, u32 w1)
{
    u32 index = w0 & 0xffu;
    u32 offset =
        ((w0 >> NDS_RENDERER_MOVEMEM_OFFSET_SHIFT) &
         NDS_RENDERER_MOVEMEM_OFFSET_MASK) * 8u;
    u32 length =
        (((w0 >> NDS_RENDERER_MOVEMEM_LENGTH_SHIFT) &
          NDS_RENDERER_MOVEMEM_LENGTH_MASK) + 1u) * 8u;
    u32 light;
    const Light *src;

    if (stats == NULL)
    {
        return;
    }
    stats->state_command_count++;
    if ((index != NDS_RENDERER_MOVEMEM_LIGHT) ||
        (offset < NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) ||
        (((offset - NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) %
          NDS_RENDERER_MOVEMEM_LIGHT_STRIDE) != 0u) ||
        (length < sizeof(Light)))
    {
        stats->ignored_state_command_count++;
        return;
    }

    light = (offset - NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) /
        NDS_RENDERER_MOVEMEM_LIGHT_STRIDE;
    src = ndsRendererResolveDataPointer(
        config, (const void *)(uintptr_t)w1, sizeof(*src));
    if (src == NULL)
    {
        stats->ignored_state_command_count++;
        return;
    }

    ndsRendererRecordLightColor(stats, light,
                                ndsRendererPackLightColor(src->l.col));
    if (light == 1u)
    {
        stats->light_dir_x = src->l.dir[0];
        stats->light_dir_y = src->l.dir[1];
        stats->light_dir_z = src->l.dir[2];
        stats->light_dir_mask |= NDS_RENDERER_LIGHT_DIR_1_MASK;
        stats->light_direction_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightDirectionCommands++;
#endif
    }
}

static void ndsRendererApplyMatrixMoveWordCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 index;
    u32 offset;
    u32 word_index;
    u32 *words;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    stats->state_command_count++;
    index = w0 & NDS_RENDERER_MOVEWORD_INDEX_MASK;
    offset = (w0 >> NDS_RENDERER_MOVEWORD_OFFSET_SHIFT) &
        NDS_RENDERER_MOVEWORD_OFFSET_MASK;
    if ((index == NDS_RENDERER_MOVEWORD_FOG) &&
        (offset == NDS_RENDERER_MOVEWORD_FOG_OFFSET))
    {
        ndsRendererRecordFogMoveWord(stats, w1);
        return;
    }
    if (index == NDS_RENDERER_MOVEWORD_LIGHTCOL)
    {
        ndsRendererRecordLightColorMoveWord(stats, offset, w1);
        return;
    }
    if ((index != NDS_RENDERER_MOVEWORD_MATRIX) ||
        ((offset % NDS_RENDERER_MATRIX_WORD_BYTES) != 0u) ||
        ((offset / NDS_RENDERER_MATRIX_WORD_BYTES) >=
         NDS_RENDERER_MATRIX_WORD_COUNT))
    {
        stats->ignored_state_command_count++;
        return;
    }

    if (state->matrix_word_valid == 0u)
    {
        ndsRendererInitMatrixWordRaw(state);
    }

    word_index = offset / NDS_RENDERER_MATRIX_WORD_BYTES;
    words = (u32 *)&state->matrix_word_raw.m[0][0];
    words[word_index] = w1;
    ndsRendererMtxLoadN64ToDS20p12(&state->matrix_word_raw, &state->matrix);
    state->matrix_valid = TRUE;
#if NDS_RENDERER_HW_TRIANGLES
    state->current_transform_vertex_mask = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
#endif
    stats->matrix_command_count++;
    stats->matrix_move_word_count++;
}

static void ndsRendererApplyPopMatrixCommand(NDSRendererStats *stats,
                                             NDSRendererTraversalState *state,
                                             u32 w1)
{
    u32 count;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    count = w1 / 64u;
    if (count == 0u)
    {
        count = 1u;
    }

    stats->state_command_count++;
    stats->matrix_command_count++;
    stats->matrix_modelview_count++;
    stats->matrix_pop_count += count;

    while (count != 0u)
    {
        u32 depth = state->modelview_stack_depth;

        if (depth == 0u)
        {
            stats->skip_command_count++;
            break;
        }

        depth--;
        state->modelview = state->modelview_stack[depth];
        state->modelview_valid = state->modelview_valid_stack[depth];
        state->modelview_stack_depth = depth;
        count--;
    }
    ndsRendererComposeMatrix(state);
}

static void ndsRendererApplyVertexCommand(
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 v0;
    u32 count;
    const u8 *src;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererHardwareLightDirection light_direction;
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
#endif

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    stats->vertex_command_count++;
    stats->state_command_count++;
    if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_RENDERER_MAX_VTX, &v0,
                              &count) == FALSE)
    {
        stats->skip_command_count++;
        return;
    }
    if ((v0 + count) > stats->vertex_count)
    {
        stats->vertex_count = v0 + count;
    }
#if !NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid == 0u)
    {
        return;
    }
#endif

    src = ndsRendererResolveDataPointer(config,
                                        (const void *)(uintptr_t)w1,
                                        (size_t)count * 16u);
    if (src == NULL)
    {
        stats->skip_command_count++;
        return;
    }

#if NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid != 0u)
    {
        matrix_snapshot = ndsRendererAcquireCurrentMatrixSnapshot(state);
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        ndsRendererHardwarePrepareLitDirection(
            stats,
            (state->modelview_valid != 0u) ? &state->modelview : NULL,
            &light_direction);
        prepared_light_direction = &light_direction;
    }
#endif

    for (i = 0u; i < count; i++)
    {
        NDSRendererInputVertex input;
        u32 index = v0 + i;
#if NDS_RENDERER_HW_TRIANGLES
        u32 mask = 1u << index;
#else
        NDSRendererClipVertex20p12 *out = &state->vertices[index];
#endif

        ndsRendererDecodeInputVertex(&input, src + (i * 16u));
#if NDS_RENDERER_HW_TRIANGLES
        ndsRendererProfileRecordSourceVertexLoad();
        state->input_vertices[index] = input;
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(&input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
        state->vertex_colors[index] =
            ndsRendererHardwareLitShadeColorPrepared(
                stats, &input, prepared_light_direction);
        state->vertex_color_valid_mask |= mask;
        state->vertex_matrix_snapshot[index] = (u8)matrix_snapshot;
        state->vertex_clip_snapshot[index] =
            NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
        state->vertex_valid_mask &= ~mask;
        state->current_transform_vertex_mask &= ~mask;
        if (state->matrix_valid != 0u)
        {
            state->current_transform_vertex_mask |= mask;
        }
#endif
        if (state->matrix_valid == 0u)
        {
            continue;
        }
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        (void)ndsRendererTransformCachedVertex(
            stats, state, index, &state->matrix, matrix_snapshot);
#else
        /* Profile 0/1 keep source vertices raw for GX. Only an exhausted
         * snapshot table needs the eager clip fallback retained here. */
        if (matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#endif
#else
        ndsRendererTransformVertex20p12(&state->matrix, &input, out);
        state->vertex_valid_mask |= 1u << index;
        stats->matrix_transform_count++;
        stats->transformed_vertex_count++;
        if (stats->transformed_vertex_count == 1u)
        {
            stats->first_transformed_x = out->x;
            stats->first_transformed_y = out->y;
            stats->first_transformed_z = out->z;
            stats->first_transformed_w = out->w;
        }
#endif
    }
}

static void ndsRendererApplyModifyVertexCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 where;
    u32 packed_index;
    u32 index;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    stats->state_command_count++;
    where = (w0 >> 16) & 0xffu;
    packed_index = w0 & 0xffffu;
    index = packed_index / 2u;

#if NDS_RENDERER_HW_TRIANGLES
    if ((where == NDS_RENDERER_MWO_POINT_ST) &&
        ((packed_index & 1u) == 0u) &&
        (index < NDS_RENDERER_MAX_VTX) &&
        ((state->input_vertex_valid_mask & (1u << index)) != 0u))
    {
        state->input_vertices[index].s = (s16)(w1 >> 16);
        state->input_vertices[index].t = (s16)(w1 & 0xffffu);
        return;
    }
#else
    (void)where;
    (void)packed_index;
    (void)index;
    (void)w1;
#endif

    stats->skip_command_count++;
}

static s32 ndsRendererTransformedTriangleReady(
    const NDSRendererTraversalState *state, u32 packed,
    u32 *out_i0, u32 *out_i1, u32 *out_i2)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if (out_i0 != NULL) { *out_i0 = i0; }
    if (out_i1 != NULL) { *out_i1 = i1; }
    if (out_i2 != NULL) { *out_i2 = i2; }

    if ((state == NULL) ||
        (i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static void ndsRendererRecordTransformedTriangle(
    NDSRendererStats *stats,
    const NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;

    if (stats == NULL)
    {
        return;
    }
    if (ndsRendererTransformedTriangleReady(state, packed, &i0, &i1, &i2) ==
        FALSE)
    {
        return;
    }

    if (stats->transformed_triangle_count == 0u)
    {
        stats->first_transformed_tri_v0 = i0;
        stats->first_transformed_tri_v1 = i1;
        stats->first_transformed_tri_v2 = i2;
    }
    stats->transformed_triangle_count++;
}

#if NDS_RENDERER_HW_TRIANGLES
static s32 ndsRendererRoundShiftS32Signed(s32 value, u32 shift)
{
    if (shift == 0u)
    {
        return value;
    }
    return (s32)ndsRendererRoundShiftS64(value, shift);
}

static v16 ndsRendererHardwareCoordToV16(s16 value)
{
    const u32 shift = 12u - NDS_RENDERER_HW_WORLD_UNIT_SHIFT;
    s32 scaled = (s32)value << shift;

    if (scaled > 32767)
    {
        return (v16)32767;
    }
    if (scaled < -32768)
    {
        return (v16)-32768;
    }
    return (v16)scaled;
}

static v16 ndsRendererHardwareVertexCoord(s16 value, u32 scale_world)
{
    if (scale_world == 0u)
    {
        return (v16)value;
    }
    return ndsRendererHardwareCoordToV16(value);
}

static v16 ndsRendererHardwareClampS64ToV16(s64 value)
{
    if (value > 32767)
    {
        return (v16)32767;
    }
    if (value < -32768)
    {
        return (v16)-32768;
    }
    return (v16)value;
}

static void ndsRendererProfileVertexRange(
    const NDSRendererInputVertex *vtx, v16 x, v16 y, v16 z)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (vtx == NULL)
    {
        return;
    }

    if (vtx->x < gNdsRendererProfileRawVertexMinX) { gNdsRendererProfileRawVertexMinX = vtx->x; }
    if (vtx->x > gNdsRendererProfileRawVertexMaxX) { gNdsRendererProfileRawVertexMaxX = vtx->x; }
    if (vtx->y < gNdsRendererProfileRawVertexMinY) { gNdsRendererProfileRawVertexMinY = vtx->y; }
    if (vtx->y > gNdsRendererProfileRawVertexMaxY) { gNdsRendererProfileRawVertexMaxY = vtx->y; }
    if (vtx->z < gNdsRendererProfileRawVertexMinZ) { gNdsRendererProfileRawVertexMinZ = vtx->z; }
    if (vtx->z > gNdsRendererProfileRawVertexMaxZ) { gNdsRendererProfileRawVertexMaxZ = vtx->z; }
    if ((s32)x < gNdsRendererProfileHWVertexMinX) { gNdsRendererProfileHWVertexMinX = x; }
    if ((s32)x > gNdsRendererProfileHWVertexMaxX) { gNdsRendererProfileHWVertexMaxX = x; }
    if ((s32)y < gNdsRendererProfileHWVertexMinY) { gNdsRendererProfileHWVertexMinY = y; }
    if ((s32)y > gNdsRendererProfileHWVertexMaxY) { gNdsRendererProfileHWVertexMaxY = y; }
    if ((s32)z < gNdsRendererProfileHWVertexMinZ) { gNdsRendererProfileHWVertexMinZ = z; }
    if ((s32)z > gNdsRendererProfileHWVertexMaxZ) { gNdsRendererProfileHWVertexMaxZ = z; }
#else
    (void)vtx;
#endif
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}

static void ndsRendererProfileHWVertexRange(v16 x, v16 y, v16 z)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((s32)x < gNdsRendererProfileHWVertexMinX) { gNdsRendererProfileHWVertexMinX = x; }
    if ((s32)x > gNdsRendererProfileHWVertexMaxX) { gNdsRendererProfileHWVertexMaxX = x; }
    if ((s32)y < gNdsRendererProfileHWVertexMinY) { gNdsRendererProfileHWVertexMinY = y; }
    if ((s32)y > gNdsRendererProfileHWVertexMaxY) { gNdsRendererProfileHWVertexMaxY = y; }
    if ((s32)z < gNdsRendererProfileHWVertexMinZ) { gNdsRendererProfileHWVertexMinZ = z; }
    if ((s32)z > gNdsRendererProfileHWVertexMaxZ) { gNdsRendererProfileHWVertexMaxZ = z; }
#endif
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererAbsDiffS32(s32 lhs, s32 rhs)
{
    s64 diff = (s64)lhs - (s64)rhs;

    if (diff < 0)
    {
        diff = -diff;
    }
    return (diff > (s64)0xffffffffu) ? 0xffffffffu : (u32)diff;
}

static void ndsRendererHardwareRecordOracleVertex(
    const NDSRendererTraversalState *state, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot;
    const NDSRendererMatrix20p12 *matrix = NULL;
    NDSRendererClipVertex20p12 expected;
    const NDSRendererClipVertex20p12 *actual;
    u32 dx;
    u32 dy;
    u32 dz;
    u32 dw;
    u32 max_delta;

    if ((state == NULL) ||
        (index >= NDS_RENDERER_MAX_VTX) ||
        ((state->input_vertex_valid_mask & (1u << index)) == 0u) ||
        ((state->vertex_valid_mask & (1u << index)) == 0u) ||
        (state->vertex_clip_snapshot[index] !=
         state->vertex_matrix_snapshot[index]))
    {
        return;
    }

    snapshot = ndsRendererGetMatrixSnapshot(
        state, state->vertex_matrix_snapshot[index]);
    if (snapshot != NULL)
    {
        matrix = &snapshot->matrix;
    }
    else if (((state->current_transform_vertex_mask & (1u << index)) != 0u) &&
             (state->matrix_valid != 0u))
    {
        /* Bounded-table overflow is eagerly transformed at VTX load. */
        matrix = &state->matrix;
    }
    if (matrix == NULL)
    {
        return;
    }

    ndsRendererTransformVertex20p12(matrix,
                                    &state->input_vertices[index],
                                    &expected);
    actual = &state->vertices[index];
    dx = ndsRendererAbsDiffS32(expected.x, actual->x);
    dy = ndsRendererAbsDiffS32(expected.y, actual->y);
    dz = ndsRendererAbsDiffS32(expected.z, actual->z);
    dw = ndsRendererAbsDiffS32(expected.w, actual->w);
    max_delta = dx;
    if (dy > max_delta) { max_delta = dy; }
    if (dz > max_delta) { max_delta = dz; }
    if (dw > max_delta) { max_delta = dw; }

    gNdsRendererProfileOracleSamples++;
    if (max_delta > gNdsRendererProfileOracleMaxDelta)
    {
        gNdsRendererProfileOracleMaxDelta = max_delta;
    }
    if (max_delta > NDS_RENDERER_HW_ORACLE_EPSILON)
    {
        gNdsRendererProfileOracleMismatches++;
    }
}

static void ndsRendererHardwareRecordOracleTriangle(
    const NDSRendererTraversalState *state, u32 i0, u32 i1, u32 i2)
{
    ndsRendererHardwareRecordOracleVertex(state, i0);
    ndsRendererHardwareRecordOracleVertex(state, i1);
    ndsRendererHardwareRecordOracleVertex(state, i2);
}
#endif

static s32 ndsRendererCombineUsesColor(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 20) & 0x0fu) == source) ||
            (((w1 >> 28) & 0x0fu) == source) ||
            (((w0 >> 15) & 0x1fu) == source) ||
            (((w1 >> 15) & 0x07u) == source) ||
            (((w0 >> 5) & 0x0fu) == source) ||
            (((w1 >> 24) & 0x0fu) == source) ||
            (((w0 >> 0) & 0x1fu) == source) ||
            (((w1 >> 6) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineOutputUsesColor(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 15) & 0x1fu) == source) ||
            (((w1 >> 15) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineSecondOutputUsesColor(u32 w0, u32 w1,
                                                   u32 source)
{
    return ((((w0 >> 0) & 0x1fu) == source) ||
            (((w1 >> 6) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUseSecondCycle(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->othermode_h & NDS_RENDERER_CYCLETYPE_MASK) ==
             NDS_RENDERER_CYC_2CYCLE)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUsesTexel01Lerp(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) ||
        (stats->texture_combine_count == 0u) ||
        (ndsRendererHardwareUseSecondCycle(stats) == FALSE))
    {
        return FALSE;
    }

    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    /* BattleShip's animated Pupupu water uses G_CC_TEMPLERP for color and
     * alpha, followed by COMBINED * SHADE / COMBINED * PRIMITIVE.
     * Decode the semantic mux rather than keying this DS adaptation to a
     * stage address or raw display-list pointer. */
    return ((((w0 >> 20) & 0x0fu) == NDS_RENDERER_CCMUX_TEXEL1) &&
            (((w1 >> 28) & 0x0fu) == NDS_RENDERER_CCMUX_TEXEL0) &&
            (((w0 >> 15) & 0x1fu) ==
             NDS_RENDERER_CCMUX_PRIM_LOD_FRAC) &&
            (((w1 >> 15) & 0x07u) == NDS_RENDERER_CCMUX_TEXEL0) &&
            (((w0 >> 12) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL1) &&
            (((w1 >> 12) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL0) &&
            (((w0 >> 9) & 0x07u) == NDS_RENDERER_ACMUX_1) &&
            (((w1 >> 9) & 0x07u) == NDS_RENDERER_ACMUX_TEXEL0) &&
            (((w0 >> 5) & 0x0fu) == NDS_RENDERER_CCMUX_COMBINED) &&
            (((w1 >> 24) & 0x0fu) == NDS_RENDERER_CCMUX_ZERO_AB) &&
            (((w0 >> 0) & 0x1fu) == NDS_RENDERER_CCMUX_SHADE) &&
            (((w1 >> 6) & 0x07u) == NDS_RENDERER_CCMUX_ZERO_D) &&
            (((w1 >> 21) & 0x07u) == NDS_RENDERER_ACMUX_COMBINED) &&
            (((w1 >> 3) & 0x07u) == NDS_RENDERER_ACMUX_0) &&
            (((w1 >> 18) & 0x07u) == NDS_RENDERER_ACMUX_PRIMITIVE) &&
            (((w1 >> 0) & 0x07u) == NDS_RENDERER_ACMUX_0)) ? TRUE : FALSE;
}

static const NDSRendererTextureLoadState *
ndsRendererHardwareFindTextureLoadForTmem(const NDSRendererStats *stats,
                                           u32 tmem)
{
    const NDSRendererTextureLoadState *best = NULL;
    u32 i;

    if (stats == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        const NDSRendererTextureLoadState *load = &stats->texture_loads[i];

        if ((load->valid != 0u) && (load->load_tmem == tmem) &&
            ((best == NULL) || (load->sequence > best->sequence)))
        {
            best = load;
        }
    }
    return best;
}

static s32 ndsRendererHardwareSecondCyclePassesCombined(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;

    if (stats == NULL)
    {
        return FALSE;
    }
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return TRUE;
    }
    if (stats->env_color != 0xffffffffu)
    {
        return FALSE;
    }

    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    /* BattleShip's normal fighter mode uses (COMBINED - 0) * ENV + 0.
     * With the opaque-white environment selected at ftdisplaymain.c:1192-1196,
     * cycle 2 preserves the source material/shade result from cycle 1. */
    return ((((w0 >> 5) & 0x0fu) == NDS_RENDERER_CCMUX_COMBINED) &&
            (((w1 >> 24) & 0x0fu) == NDS_RENDERER_CCMUX_ZERO_AB) &&
            (((w0 >> 0) & 0x1fu) == NDS_RENDERER_CCMUX_ENVIRONMENT) &&
            (((w1 >> 6) & 0x07u) == NDS_RENDERER_CCMUX_ZERO_D)) ? TRUE :
                                                                  FALSE;
}

static s32 ndsRendererHardwareOutputUsesColor(const NDSRendererStats *stats,
                                              u32 source)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return ndsRendererCombineOutputUsesColor(w0, w1, source);
    }
    if (ndsRendererCombineSecondOutputUsesColor(w0, w1, source) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererCombineSecondOutputUsesColor(
            w0, w1, NDS_RENDERER_CCMUX_COMBINED) != FALSE)
    {
        return ndsRendererCombineOutputUsesColor(w0, w1, source);
    }
    return FALSE;
}

static s32 ndsRendererCombineUsesAlpha(u32 w0, u32 w1, u32 source)
{
    return ((((w0 >> 9) & 0x07u) == source) ||
            (((w1 >> 9) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererCombineSecondOutputUsesAlpha(u32 w1, u32 source)
{
    return ((((w1 >> 18) & 0x07u) == source) ||
            (((w1 >> 0) & 0x07u) == source)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareOutputUsesAlpha(const NDSRendererStats *stats,
                                              u32 source)
{
    u32 w0;
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return ndsRendererCombineUsesAlpha(w0, w1, source);
    }
    if (ndsRendererCombineSecondOutputUsesAlpha(w1, source) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererCombineSecondOutputUsesAlpha(
            w1, NDS_RENDERER_ACMUX_COMBINED) != FALSE)
    {
        return ndsRendererCombineUsesAlpha(w0, w1, source);
    }
    return FALSE;
}

static s32 ndsRendererHardwareUseDecal(const NDSRendererStats *stats)
{
    u32 w1;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w1 = stats->texture_combine_w1;
    return ((((w1 >> 28) & 0x0fu) == ((w1 >> 15) & 0x07u)) ?
        TRUE : FALSE);
}

static s32 ndsRendererHardwareUsePrimDepth(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->othermode_l & NDS_RENDERER_ZSOURCE_MASK) ==
             NDS_RENDERER_ZSOURCE_PRIM)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwarePrimitiveDecal(const NDSRendererStats *stats)
{
    if (ndsRendererHardwareUseDecal(stats) == FALSE)
    {
        return FALSE;
    }
    return ((((stats->texture_combine_w0 >> 20) & 0x0fu) ==
             NDS_RENDERER_CCMUX_PRIMITIVE) ? TRUE : FALSE);
}

static void ndsRendererHardwareRecordUseTextureReject(
    const NDSRendererStats *stats,
    u32 reason)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    switch (reason)
    {
    case NDS_RENDERER_HW_USETEX_REJECT_NO_STATS:
        gNdsRendererProfileUseTextureRejectNoStatsCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF:
        gNdsRendererProfileUseTextureRejectStateOffCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE:
        gNdsRendererProfileUseTextureRejectNoCombineCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL:
        gNdsRendererProfileUseTextureRejectPrimitiveDecalCount++;
        break;
    case NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0:
        gNdsRendererProfileUseTextureRejectNoTexel0Count++;
        break;
    default:
        break;
    }
    if (gNdsRendererProfileUseTextureRejectFirstReason == 0u)
    {
        gNdsRendererProfileUseTextureRejectFirstReason = reason;
        if (stats != NULL)
        {
            gNdsRendererProfileUseTextureRejectFirstFlags =
                stats->texture_state_flags;
            gNdsRendererProfileUseTextureRejectFirstW0 =
                stats->texture_combine_w0;
            gNdsRendererProfileUseTextureRejectFirstW1 =
                stats->texture_combine_w1;
            gNdsRendererProfileUseTextureRejectFirstGeometry =
                stats->geometry_mode;
        }
    }
#else
    (void)stats;
    (void)reason;
#endif
}

static s32 ndsRendererHardwareTextureImplicitStateOn(
    const NDSRendererStats *stats)
{
    const NDSRendererTileState *render_tile;
    u32 required_mask;

    if ((stats == NULL) ||
        ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) != 0u))
    {
        return FALSE;
    }

    required_mask = NDS_RENDERER_TEXTURE_SETTIMG |
        NDS_RENDERER_TEXTURE_SETTILE |
        NDS_RENDERER_TEXTURE_SETTILESIZE;
    if (((stats->texture_mask & required_mask) != required_mask) ||
        ((stats->texture_mask &
          (NDS_RENDERER_TEXTURE_LOADBLOCK | NDS_RENDERER_TEXTURE_LOADTILE)) ==
         0u) ||
        (stats->texture_image == 0u) ||
        (stats->texture_load_texels == 0u))
    {
        return FALSE;
    }

    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
    return ((render_tile->set_seen != 0u) &&
            (render_tile->size_seen != 0u) &&
            (render_tile->line != 0u) &&
            (render_tile->width != 0u) &&
            (render_tile->height != 0u)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareUseTexture(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        ndsRendererHardwareRecordUseTextureReject(
            NULL, NDS_RENDERER_HW_USETEX_REJECT_NO_STATS);
        return FALSE;
    }
    if ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) == 0u)
    {
        if (ndsRendererHardwareTextureImplicitStateOn(stats) == FALSE)
        {
            ndsRendererHardwareRecordUseTextureReject(
                stats, NDS_RENDERER_HW_USETEX_REJECT_STATE_OFF);
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileUseTextureImplicitOnCount++;
#endif
    }
    if (stats->texture_combine_count == 0u)
    {
        ndsRendererHardwareRecordUseTextureReject(
            stats, NDS_RENDERER_HW_USETEX_REJECT_NO_COMBINE);
        return FALSE;
    }
    if (ndsRendererHardwarePrimitiveDecal(stats) != FALSE)
    {
        ndsRendererHardwareRecordUseTextureReject(
            stats, NDS_RENDERER_HW_USETEX_REJECT_PRIMITIVE_DECAL);
        return FALSE;
    }
    if (ndsRendererCombineUsesColor(
            stats->texture_combine_w0, stats->texture_combine_w1,
            NDS_RENDERER_CCMUX_TEXEL0) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererHardwareOutputUsesAlpha(
            stats, NDS_RENDERER_ACMUX_TEXEL0) != FALSE)
    {
        return TRUE;
    }
    ndsRendererHardwareRecordUseTextureReject(
        stats, NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0);
    return FALSE;
}

static s32 ndsRendererHardwareUsesLitPrimitiveModulate(
    const NDSRendererStats *stats)
{
    u32 a;
    u32 b;
    u32 c;
    u32 d;

    if ((stats == NULL) || (stats->texture_combine_count == 0u) ||
        (ndsRendererHardwareSecondCyclePassesCombined(stats) == FALSE))
    {
        return FALSE;
    }

    a = (stats->texture_combine_w0 >> 20) & 0x0fu;
    b = (stats->texture_combine_w1 >> 28) & 0x0fu;
    c = (stats->texture_combine_w0 >> 15) & 0x1fu;
    d = (stats->texture_combine_w1 >> 15) & 0x07u;
    if ((b != NDS_RENDERER_CCMUX_ZERO_AB) ||
        (d != NDS_RENDERER_CCMUX_ZERO_D))
    {
        return FALSE;
    }
    return (((a == NDS_RENDERER_CCMUX_PRIMITIVE) &&
             (c == NDS_RENDERER_CCMUX_SHADE)) ||
            ((a == NDS_RENDERER_CCMUX_SHADE) &&
             (c == NDS_RENDERER_CCMUX_PRIMITIVE))) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareColorSource(const NDSRendererStats *stats)
{
    if (ndsRendererHardwareUsesLitPrimitiveModulate(stats) != FALSE)
    {
        return stats->prim_color;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareOutputUsesColor(
                stats, NDS_RENDERER_CCMUX_ENVIRONMENT) != FALSE)
        {
            return stats->env_color;
        }
        if (ndsRendererHardwareOutputUsesColor(
                stats, NDS_RENDERER_CCMUX_PRIMITIVE) != FALSE)
        {
            return stats->prim_color;
        }
    }
    return 0u;
}

static s32 ndsRendererHardwareLitShadeCombine(const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            (stats->texture_combine_count != 0u) &&
            ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
            (ndsRendererCombineUsesColor(stats->texture_combine_w0,
                                         stats->texture_combine_w1,
                                         NDS_RENDERER_CCMUX_SHADE) != FALSE)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareUseMaterialColor(const NDSRendererStats *stats)
{
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
        {
            return ndsRendererHardwareUsesLitPrimitiveModulate(stats);
        }
        return ((ndsRendererHardwareOutputUsesColor(
                     stats, NDS_RENDERER_CCMUX_ENVIRONMENT) != FALSE) ||
                (ndsRendererHardwareOutputUsesColor(
                     stats, NDS_RENDERER_CCMUX_PRIMITIVE) != FALSE)) ?
            TRUE : FALSE;
    }
    return FALSE;
}

static s32 ndsRendererHardwareUseVertexColor(const NDSRendererStats *stats)
{
    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return TRUE;
    }
    if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
    {
        return TRUE;
    }
    if (ndsRendererHardwareUseMaterialColor(stats) != FALSE)
    {
        return FALSE;
    }
    return ndsRendererCombineUsesColor(
        stats->texture_combine_w0, stats->texture_combine_w1,
        NDS_RENDERER_CCMUX_SHADE);
}

static s32 ndsRendererHardwareBlendAlphaUsesMemory(
    const NDSRendererStats *stats)
{
    u32 mode;

    if (stats == NULL)
    {
        return FALSE;
    }
    mode = stats->othermode_l;
    if (((mode >> NDS_RENDERER_BLEND_ALPHA_CYCLE1_SHIFT) &
         NDS_RENDERER_BLEND_ALPHA_BITS_MASK) == NDS_RENDERER_G_BL_A_MEM)
    {
        return TRUE;
    }
    return ((ndsRendererHardwareUseSecondCycle(stats) != FALSE) &&
            (((mode >> NDS_RENDERER_BLEND_ALPHA_CYCLE2_SHIFT) &
              NDS_RENDERER_BLEND_ALPHA_BITS_MASK) ==
             NDS_RENDERER_G_BL_A_MEM)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareAlpha(const NDSRendererStats *stats,
                                    const NDSRendererInputVertex *vtx)
{
    u32 alpha = 0xffu;

    if (vtx != NULL)
    {
        alpha = vtx->a;
    }
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) &&
        ((stats->othermode_l & NDS_RENDERER_FORCE_BL) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_CVG_X_ALPHA) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) !=
         NDS_RENDERER_ZMODE_XLU))
    {
        return 31u;
    }
    if (ndsRendererHardwareBlendAlphaUsesMemory(stats) != FALSE)
    {
        return 31u;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if (ndsRendererHardwareOutputUsesAlpha(
                stats, NDS_RENDERER_ACMUX_PRIMITIVE) != FALSE)
        {
            alpha = stats->prim_color & 0xffu;
        }
        else if (ndsRendererHardwareOutputUsesAlpha(
                     stats, NDS_RENDERER_ACMUX_ENVIRONMENT) != FALSE)
        {
            alpha = stats->env_color & 0xffu;
        }
        else if ((ndsRendererHardwareOutputUsesAlpha(
                      stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
                 (ndsRendererHardwareOutputUsesAlpha(
                      stats, NDS_RENDERER_ACMUX_SHADE) == FALSE))
        {
            alpha = (ndsRendererHardwareOutputUsesAlpha(
                         stats, NDS_RENDERER_ACMUX_0) != FALSE) ?
                0u : 0xffu;
        }
    }
    return alpha >> 3;
}

static u32 ndsRendererHardwarePolyFmt(const NDSRendererStats *stats, u32 alpha)
{
    u32 poly_id = (stats != NULL) ?
        (stats->texture_combine_count & NDS_RENDERER_POLY_ID_MASK) : 0u;
    u32 poly_fmt = POLY_CULL_NONE | POLY_ALPHA(alpha) | POLY_ID(poly_id);
    u32 mode = (stats != NULL) ? stats->geometry_mode : 0u;

    if (ndsRendererHardwareUseDecal(stats) != FALSE)
    {
        poly_fmt |= POLY_DECAL;
    }
    if ((mode & NDS_RENDERER_GEOM_FOG) != 0u)
    {
        poly_fmt |= POLY_FOG;
    }
    if ((mode & NDS_RENDERER_GEOM_CULL_FRONT) != 0u)
    {
        poly_fmt &= ~((u32)POLY_CULL_BACK);
    }
    if ((mode & NDS_RENDERER_GEOM_CULL_BACK) != 0u)
    {
        poly_fmt &= ~((u32)POLY_CULL_FRONT);
    }
    return poly_fmt;
}

static void ndsRendererHardwareApplyAlphaTest(const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) ==
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD))
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc((stats->blend_color & 0xffu) >> 4);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
}

static void ndsRendererHardwareApplyFog(const NDSRendererStats *stats)
{
    s32 range;
    s32 shift;
    s32 density;
    s32 inc;
    s32 i;
    u32 color;

    if ((stats == NULL) ||
        (stats->fog_status == 0u) ||
        (stats->fog_max <= stats->fog_min))
    {
        glDisable(GL_FOG);
        return;
    }

    range = stats->fog_max - stats->fog_min;
    shift = 0;
    for (i = 500; (i >= range) && (i > 0); i >>= 1)
    {
        shift++;
    }

    density = 0;
    inc = (((128 * 1000) << 1) / (range * 32) + 1) >> (shift + 1);
    if (inc < 1)
    {
        inc = 1;
    }
    for (i = 0; i < 32; i++)
    {
        glFogDensity(i, density);
        density += inc;
        if (density > 127)
        {
            density = 127;
        }
    }

    color = stats->fog_color;
    glFogShift(shift);
    glFogOffset((stats->fog_min * 0x7fff / 1000) - (0x400 >> shift));
    glFogColor((color >> 27) & 0x1fu, (color >> 19) & 0x1fu,
               (color >> 11) & 0x1fu, (color >> 3) & 0x1fu);
    glEnable(GL_FOG);
}

static s32 ndsRendererHardwareTextureFilterOffset(
    const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
         NDS_RENDERER_TF_POINT))
    {
        return NDS_RENDERER_TEXCOORD_FILTER_OFFSET;
    }
    return 0;
}

static s32 ndsRendererHardwareUseTextureMatrix(
    const NDSRendererStats *stats)
{
    return ((stats == NULL) ||
            ((stats->othermode_h & NDS_RENDERER_TEXTPERSP_MASK) ==
             NDS_RENDERER_TP_PERSP)) ? TRUE : FALSE;
}

static s16 ndsRendererHardwareTexCoord(s16 coord, u32 scale, u32 origin,
                                       s32 offset)
{
    s64 scaled_t16 = ((s64)coord * (s64)scale) >> 17;
    s64 origin_t16 = (s64)origin << 2;

    return (s16)(scaled_t16 - origin_t16 + offset);
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileTextureCoord(s16 s, s16 t)
{
    gNdsRendererProfileTexturedVertexCount++;
    if (s < gNdsRendererProfileTextureCoordMinS)
    {
        gNdsRendererProfileTextureCoordMinS = s;
    }
    if (s > gNdsRendererProfileTextureCoordMaxS)
    {
        gNdsRendererProfileTextureCoordMaxS = s;
    }
    if (t < gNdsRendererProfileTextureCoordMinT)
    {
        gNdsRendererProfileTextureCoordMinT = t;
    }
    if (t > gNdsRendererProfileTextureCoordMaxT)
    {
        gNdsRendererProfileTextureCoordMaxT = t;
    }
}
#endif

static u32 ndsRendererHardwareColorByte(u32 color, u32 shift)
{
    return (color >> shift) & 0xffu;
}

static u8 ndsRendererHardwareClampColor(s32 value)
{
    if (value < 0)
    {
        return 0u;
    }
    if (value > 255)
    {
        return 0xffu;
    }
    return (u8)value;
}

static u32 ndsRendererHardwareLightColor(NDSRendererStats *stats, u32 mask,
                                         u32 color, u32 fallback)
{
    if ((stats == NULL) || ((stats->light_color_mask & mask) == 0u))
    {
        if (stats != NULL)
        {
            stats->light_fallback_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileLightFallbackCount++;
#endif
        }
        return fallback;
    }
    return color;
}

static void ndsRendererHardwarePrepareLitDirection(
    const NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *out)
{
    s32 light_x;
    s32 light_y;
    s32 light_z;

    if (out == NULL)
    {
        return;
    }

    light_x = (stats != NULL) ? stats->light_dir_x : 0;
    light_y = (stats != NULL) ? stats->light_dir_y : 0;
    light_z = (stats != NULL) ? stats->light_dir_z : 0;
    if ((stats != NULL) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u) &&
        (modelview != NULL))
    {
        f32 transformed_x = (f32)(
            (s64)light_x * modelview->m[0][0] +
            (s64)light_y * modelview->m[0][1] +
            (s64)light_z * modelview->m[0][2]);
        f32 transformed_y = (f32)(
            (s64)light_x * modelview->m[1][0] +
            (s64)light_y * modelview->m[1][1] +
            (s64)light_z * modelview->m[1][2]);
        f32 transformed_z = (f32)(
            (s64)light_x * modelview->m[2][0] +
            (s64)light_y * modelview->m[2][1] +
            (s64)light_z * modelview->m[2][2]);
        f32 length = sqrtf((transformed_x * transformed_x) +
                           (transformed_y * transformed_y) +
                           (transformed_z * transformed_z));

        if (length > 0.0F)
        {
            light_x = (s32)((transformed_x * 127.0F) / length);
            light_y = (s32)((transformed_y * 127.0F) / length);
            light_z = (s32)((transformed_z * 127.0F) / length);
        }
    }

    out->x = light_x;
    out->y = light_y;
    out->z = light_z;
}

static u32 ndsRendererHardwareLitDiffuseNumer(
    const NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction)
{
    s32 light_x;
    s32 light_y;
    s32 light_z;
    s32 dot;

    if ((stats == NULL) || (vtx == NULL) ||
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) == 0u))
    {
        return 127u;
    }

    light_x = (direction != NULL) ? direction->x : stats->light_dir_x;
    light_y = (direction != NULL) ? direction->y : stats->light_dir_y;
    light_z = (direction != NULL) ? direction->z : stats->light_dir_z;

    dot = ((s32)(s8)vtx->r * light_x) +
        ((s32)(s8)vtx->g * light_y) +
        ((s32)(s8)vtx->b * light_z);
    if (dot <= 0)
    {
        return 0u;
    }
    if (dot > (127 * 127))
    {
        return 127u;
    }
    return (u32)((dot * 127) / (127 * 127));
}

static u32 ndsRendererHardwareLitShadeColorPrepared(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction)
{
    u32 light_1;
    u32 light_2;
    u32 ambient;
    u32 diffuse;
    u32 diffuse_numer;
    s32 r;
    s32 g;
    s32 b;

    if (vtx == NULL)
    {
        return 0xffffffffu;
    }
    if ((stats == NULL) ||
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) == 0u))
    {
        return ((u32)vtx->r << 24) | ((u32)vtx->g << 16) |
            ((u32)vtx->b << 8) | (u32)vtx->a;
    }

    light_1 = ndsRendererHardwareLightColor(
        stats, NDS_RENDERER_LIGHT_COLOR_1_MASK, stats->light_color_1,
        NDS_RENDERER_LIGHT_COLOR_1_FALLBACK);
    light_2 = ndsRendererHardwareLightColor(
        stats, NDS_RENDERER_LIGHT_COLOR_2_MASK, stats->light_color_2,
        NDS_RENDERER_LIGHT_COLOR_2_FALLBACK);
    diffuse = light_1;
    ambient = light_2;

    diffuse_numer = ndsRendererHardwareLitDiffuseNumer(stats, vtx, direction);
    r = (s32)ndsRendererHardwareColorByte(ambient, 24) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 24) * diffuse_numer) /
              127u);
    g = (s32)ndsRendererHardwareColorByte(ambient, 16) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 16) * diffuse_numer) /
              127u);
    b = (s32)ndsRendererHardwareColorByte(ambient, 8) +
        (s32)((ndsRendererHardwareColorByte(diffuse, 8) * diffuse_numer) /
              127u);

    return ((u32)ndsRendererHardwareClampColor(r) << 24) |
        ((u32)ndsRendererHardwareClampColor(g) << 16) |
        ((u32)ndsRendererHardwareClampColor(b) << 8) | (u32)vtx->a;
}

static u32 ndsRendererHardwareLitShadeColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererMatrix20p12 *modelview)
{
    NDSRendererHardwareLightDirection direction;
    const NDSRendererHardwareLightDirection *prepared_direction = NULL;

    if ((stats != NULL) &&
        ((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        ndsRendererHardwarePrepareLitDirection(stats, modelview, &direction);
        prepared_direction = &direction;
    }
    return ndsRendererHardwareLitShadeColorPrepared(
        stats, vtx, prepared_direction);
}

static u16 ndsRendererHardwarePackedVertexColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    u32 material_color,
    s32 use_material_color,
    s32 use_vertex_color,
    u32 vertex_color,
    s32 vertex_color_valid)
{
    u32 color;

    if ((use_material_color != FALSE) && (use_vertex_color == FALSE))
    {
        return RGB15((u8)((material_color >> 27) & 0x1fu),
                     (u8)((material_color >> 19) & 0x1fu),
                     (u8)((material_color >> 11) & 0x1fu));
    }
    if (use_vertex_color == FALSE)
    {
        return RGB15(31u, 31u, 31u);
    }
    color = (vertex_color_valid != FALSE) ? vertex_color :
        ndsRendererHardwareLitShadeColor(stats, vtx, NULL);
    if (use_material_color != FALSE)
    {
        u32 r = ((ndsRendererHardwareColorByte(color, 24) *
                  ndsRendererHardwareColorByte(material_color, 24)) + 127u) /
            255u;
        u32 g = ((ndsRendererHardwareColorByte(color, 16) *
                  ndsRendererHardwareColorByte(material_color, 16)) + 127u) /
            255u;
        u32 b = ((ndsRendererHardwareColorByte(color, 8) *
                  ndsRendererHardwareColorByte(material_color, 8)) + 127u) /
            255u;

        return RGB15((u8)(r >> 3), (u8)(g >> 3), (u8)(b >> 3));
    }
    return RGB15((u8)((color >> 27) & 0x1fu),
                 (u8)((color >> 19) & 0x1fu),
                 (u8)((color >> 11) & 0x1fu));
}

static const void *ndsRendererResolveTextureDataPointer(
    const NDSRendererConfig *config, const void *ptr, size_t bytes)
{
    if (ptr == NULL)
    {
        return NULL;
    }
    if ((config != NULL) && (config->resolve_data != NULL))
    {
        return config->resolve_data(ptr, bytes, config->user);
    }
    if ((config != NULL) && (config->validate_range != NULL) &&
        (config->validate_range((const Gfx *)ptr, bytes, config->user) ==
         FALSE))
    {
        return NULL;
    }
    return ptr;
}

static s32 ndsRendererHardwareTextureKeyEqual(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    return (memcmp(a, b, sizeof(*a)) == 0) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareTexel1RefreshCompatible(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    /* The exact Pupupu material animates fraction, image IDs, and tile
     * origins. All of those change converted pixels, but not the resident DS
     * RGBA allocation. Width/height and format distinguish its large and
     * small water surfaces; same-frame reuse is excluded by the caller. */
    return ((a->data_layout == b->data_layout) &&
            (a->format == b->format) &&
            (a->size == b->size) &&
            (a->width == b->width) &&
            (a->height == b->height) &&
            (a->combine_w0 == b->combine_w0) &&
            (a->combine_w1 == b->combine_w1)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareTextureKeyWouldLegacyAlias(
    const NDSRendererHardwareTextureKey *a,
    const NDSRendererHardwareTextureKey *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    return ((a->image == b->image) &&
            (a->image_width == b->image_width) &&
            (a->tlut_image == b->tlut_image) &&
            (a->tlut_count == b->tlut_count) &&
            (a->format == b->format) &&
            (a->size == b->size) &&
            (a->width == b->width) &&
            (a->height == b->height) &&
            (a->render_tile == b->render_tile) &&
            (a->render_tmem == b->render_tmem) &&
            (a->render_palette == b->render_palette) &&
            (a->render_tile_cms == b->render_tile_cms) &&
            (a->render_tile_cmt == b->render_tile_cmt) &&
            (a->render_tile_masks == b->render_tile_masks) &&
            (a->render_tile_maskt == b->render_tile_maskt) &&
            (a->render_tile_shifts == b->render_tile_shifts) &&
            (a->render_tile_shiftt == b->render_tile_shiftt) &&
            (a->load_tile == b->load_tile) &&
            (a->load_uls == b->load_uls) &&
            (a->load_ult == b->load_ult) &&
            (a->load_lrs == b->load_lrs) &&
            (a->load_dxt == b->load_dxt) &&
            (a->load_texels == b->load_texels) &&
            (a->tile_uls == b->tile_uls) &&
            (a->tile_ult == b->tile_ult) &&
            (a->line == b->line) &&
            (a->flags == b->flags) &&
            (a->texel1_image == b->texel1_image) &&
            (a->texel1_image_format == b->texel1_image_format) &&
            (a->texel1_image_size == b->texel1_image_size) &&
            (a->texel1_image_width == b->texel1_image_width) &&
            (a->texel1_load_kind == b->texel1_load_kind) &&
            (a->texel1_render_tmem == b->texel1_render_tmem) &&
            (a->texel1_render_line == b->texel1_render_line) &&
            (a->texel1_render_palette == b->texel1_render_palette) &&
            (a->texel1_render_tile_cms == b->texel1_render_tile_cms) &&
            (a->texel1_render_tile_cmt == b->texel1_render_tile_cmt) &&
            (a->texel1_render_tile_masks == b->texel1_render_tile_masks) &&
            (a->texel1_render_tile_maskt == b->texel1_render_tile_maskt) &&
            (a->texel1_render_tile_shifts == b->texel1_render_tile_shifts) &&
            (a->texel1_render_tile_shiftt == b->texel1_render_tile_shiftt) &&
            (a->texel1_load_tile == b->texel1_load_tile) &&
            (a->texel1_load_uls == b->texel1_load_uls) &&
            (a->texel1_load_ult == b->texel1_load_ult) &&
            (a->texel1_load_lrs == b->texel1_load_lrs) &&
            (a->texel1_load_dxt == b->texel1_load_dxt) &&
            (a->texel1_load_texels == b->texel1_load_texels) &&
            (a->texel1_tile_uls == b->texel1_tile_uls) &&
            (a->texel1_tile_ult == b->texel1_tile_ult) &&
            (a->prim_lod_fraction == b->prim_lod_fraction) &&
            (a->combine_w0 == b->combine_w0) &&
            (a->combine_w1 == b->combine_w1)) ? TRUE : FALSE;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexture(const NDSRendererHardwareTextureKey *key)
{
    u32 i;

    if (key == NULL)
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if ((sNdsRendererHardwareTextureCache[i].ready != 0u) &&
            (ndsRendererHardwareTextureKeyEqual(
                 &sNdsRendererHardwareTextureCache[i].key, key) != FALSE))
        {
            return &sNdsRendererHardwareTextureCache[i];
        }
        if ((sNdsRendererHardwareTextureCache[i].ready != 0u) &&
            (ndsRendererHardwareTextureKeyWouldLegacyAlias(
                 &sNdsRendererHardwareTextureCache[i].key, key) != FALSE))
        {
            ndsRendererProfileRecordTextureAliasAvoid();
        }
    }
    return NULL;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexel1RefreshTexture(
    const NDSRendererHardwareTextureKey *key)
{
    u32 i;

    if ((key == NULL) || (key->texel1_image == 0u))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if ((entry->ready != 0u) && (entry->key.texel1_image != 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)) &&
            (ndsRendererHardwareTexel1RefreshCompatible(
                 &entry->key, key) != FALSE))
        {
            return entry;
        }
    }
    return NULL;
}

static s32 ndsRendererHardwareReplaceTextureData(
    NDSRendererHardwareTextureCacheEntry *entry,
    const void *texture,
    u32 texture_bytes)
{
    void *vram_address;
    uintptr_t vram_first;
    uintptr_t vram_last;
    u32 vram_state;

    if ((entry == NULL) || (entry->name == 0) ||
        (texture == NULL) || (texture_bytes == 0u))
    {
        return FALSE;
    }
    vram_address = glGetTexturePointer(entry->name);
    if (vram_address == NULL)
    {
        return FALSE;
    }

    /* sm64-nds updates allocated textures by temporarily exposing every
     * owning texture bank to the CPU, then restoring the primary mapping. */
    ndsRendererHardwareEndBatch();
    vram_state = VRAM_CR;
    vram_first = (uintptr_t)vram_address;
    vram_last = vram_first + texture_bytes - 1u;
    if ((vram_first < (uintptr_t)VRAM_B) &&
        (vram_last >= (uintptr_t)VRAM_A))
    {
        vramSetBankA(VRAM_A_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_C) &&
        (vram_last >= (uintptr_t)VRAM_B))
    {
        vramSetBankB(VRAM_B_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_D) &&
        (vram_last >= (uintptr_t)VRAM_C))
    {
        vramSetBankC(VRAM_C_LCD);
    }
    if ((vram_first < (uintptr_t)VRAM_E) &&
        (vram_last >= (uintptr_t)VRAM_D))
    {
        vramSetBankD(VRAM_D_LCD);
    }

    DC_FlushRange(texture, texture_bytes);
    dmaCopyWords(0, texture, vram_address, texture_bytes);
    vramRestorePrimaryBanks(vram_state);
    return TRUE;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareReleaseTexture(
    NDSRendererHardwareTextureCacheEntry *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
    ndsRendererHardwareEndBatch();
    if (sNdsRendererHardwareBoundTextureName == (u32)entry->name)
    {
        sNdsRendererHardwareBoundTextureName = 0u;
    }
    if (sNdsRendererHardwareActiveTextureEntry == entry)
    {
        sNdsRendererHardwareActiveTextureEntry = NULL;
    }
    if (entry->name != 0)
    {
        glDeleteTextures(1, &entry->name);
    }
    memset(entry, 0, sizeof(*entry));
    return entry;
}

static s32 ndsRendererHardwareEvictTexture(
    const NDSRendererHardwareTextureCacheEntry *exclude)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        u32 index = sNdsRendererHardwareTextureCacheNext %
            NDS_RENDERER_HW_TEXTURE_CACHE_COUNT;
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[index];

        sNdsRendererHardwareTextureCacheNext = index + 1u;
        if ((entry != exclude) && (entry->name != 0) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
        {
            ndsRendererProfileRecordTextureEvict();
            (void)ndsRendererHardwareReleaseTexture(entry);
            return TRUE;
        }
    }
    return FALSE;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareAllocTexture(void)
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 i;
    u32 index;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if (sNdsRendererHardwareTextureCache[i].ready == 0u)
        {
            entry = &sNdsRendererHardwareTextureCache[i];
            return (entry->name != 0) ?
                ndsRendererHardwareReleaseTexture(entry) : entry;
        }
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        index = sNdsRendererHardwareTextureCacheNext %
            NDS_RENDERER_HW_TEXTURE_CACHE_COUNT;
        sNdsRendererHardwareTextureCacheNext = index + 1u;
        entry = &sNdsRendererHardwareTextureCache[index];
        if (entry->last_used_frame !=
            (sNdsRendererHardwareFrameSerial + 1u))
        {
            /* libnds allocates texture VRAM in glTexImage2D. Delete the old
             * allocation, but never recycle a texture referenced this frame. */
            ndsRendererProfileRecordTextureEvict();
            return ndsRendererHardwareReleaseTexture(entry);
        }
    }
    return NULL;
}

static s32 ndsRendererHardwareTextureSizeEnum(u32 size, int *out)
{
    int value;

    if (out == NULL)
    {
        return FALSE;
    }
    switch (size)
    {
    case 8u: value = TEXTURE_SIZE_8; break;
    case 16u: value = TEXTURE_SIZE_16; break;
    case 32u: value = TEXTURE_SIZE_32; break;
    case 64u: value = TEXTURE_SIZE_64; break;
    case 128u: value = TEXTURE_SIZE_128; break;
    default:
        return FALSE;
    }
    *out = value;
    return TRUE;
}

static u32 ndsRendererHardwareTextureNextPow2(u32 value)
{
    u32 out = 8u;

    while ((out < value) && (out < NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        out <<= 1;
    }
    return out;
}

static s32 ndsRendererHardwareTextureMaskedClampNeedsWrap(
    u32 mode, u32 mask, u32 upload_extent, u32 tile_extent)
{
    u32 mask_extent;
    u32 sampler_extent;

    /* RDP mask repeat/mirror can occur before the logical tile clamp edge. */
    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (upload_extent == 0u) || (tile_extent == 0u))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    if (upload_extent != mask_extent)
    {
        return FALSE;
    }
    sampler_extent = upload_extent;
    if ((mode & NDS_RENDERER_TX_MIRROR) != 0u)
    {
        sampler_extent <<= 1;
    }
    return (((mode & NDS_RENDERER_TX_MIRROR) != 0u) ||
            (sampler_extent != tile_extent)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareTextureMaterializesMaskedClamp(
    u32 mode, u32 mask, u32 source_extent, u32 tile_extent)
{
    u32 mask_extent;

    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (source_extent == 0u) ||
        (tile_extent > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    return ((tile_extent > mask_extent) &&
            (source_extent >= mask_extent) &&
            (source_extent <= tile_extent)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwareTextureMaskedAddress(
    u32 coord, u32 mode, u32 mask)
{
    u32 mask_extent = 1u << mask;
    u32 period = coord >> mask;
    u32 local = coord & (mask_extent - 1u);

    if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
        ((period & 1u) != 0u))
    {
        local = mask_extent - 1u - local;
    }
    return local;
}

static u32 ndsRendererHardwareTextureParams(
    const NDSRendererStats *stats,
    const NDSRendererTileState *render_tile,
    u32 upload_width,
    u32 upload_height)
{
    u32 params;
    s32 wrap_s;
    s32 wrap_t;

    if (render_tile == NULL)
    {
        return TEXGEN_OFF;
    }

    params = (ndsRendererHardwareUseTextureMatrix(stats) != FALSE) ?
        TEXGEN_TEXCOORD : TEXGEN_OFF;
    wrap_s = ((render_tile->cms & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cms, render_tile->masks, upload_width,
            render_tile->width);
    wrap_t = ((render_tile->cmt & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cmt, render_tile->maskt, upload_height,
            render_tile->height);
    if (wrap_s != FALSE)
    {
        params |= GL_TEXTURE_WRAP_S;
    }
    if ((wrap_s != FALSE) &&
        ((render_tile->cms & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_S;
    }
    if (wrap_t != FALSE)
    {
        params |= GL_TEXTURE_WRAP_T;
    }
    if ((wrap_t != FALSE) &&
        ((render_tile->cmt & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_T;
    }
    return params;
}

static u32 ndsRendererHardwareMergeTextureParams(u32 params)
{
    u32 current = (u32)glGetTexParameter();

    current &= ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    current |= params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    return current;
}

static void ndsRendererHardwareApplyTextureParams(u32 params)
{
    glTexParameter(GL_TEXTURE_2D, (int)params);
}

static u32 ndsRendererHardwareAlphaStateKey(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return 0u;
    }
    return (stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) |
           ((stats->blend_color & 0xffu) << 8);
}

static u32 ndsRendererHardwareFogStateKey(const NDSRendererStats *stats)
{
    if ((stats == NULL) ||
        (stats->fog_status == 0u) ||
        (stats->fog_max <= stats->fog_min))
    {
        return 0u;
    }
    return ((u32)stats->fog_min & 0x3ffu) |
           (((u32)stats->fog_max & 0x3ffu) << 10) |
           ((stats->fog_color & 0xfffu) << 20);
}

static void ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name)
{
    if (texture_name == 0u)
    {
        return;
    }
    if (sNdsRendererHardwareBoundTextureName != texture_name)
    {
        ndsRendererHardwareEndBatch();
        glBindTexture(GL_TEXTURE_2D, texture_name);
        sNdsRendererHardwareBoundTextureName = texture_name;
        ndsRendererProfileRecordTextureBind();
        if (stats != NULL)
        {
            stats->hardware_texture_bind_count++;
        }
    }
}

static void ndsRendererHardwareBindNoTexture(NDSRendererStats *stats)
{
    if (sNdsRendererHardwareNoTextureName == 0)
    {
        ndsRendererHardwareEndBatch();
        if (glGenTextures(1, &sNdsRendererHardwareNoTextureName) == 0)
        {
            return;
        }
        glBindTexture(GL_TEXTURE_2D, sNdsRendererHardwareNoTextureName);
        sNdsRendererHardwareBoundTextureName =
            (u32)sNdsRendererHardwareNoTextureName;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_NOTEXTURE, 0, 0, 0,
                     TEXGEN_TEXCOORD, NULL);
    }
    else
    {
        ndsRendererHardwareBindTextureName(
            stats, (u32)sNdsRendererHardwareNoTextureName);
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
}

static u16 ndsRendererHardwareConvertRgba16(u16 n64_color,
                                            s32 preserve_transparent_rgb)
{
    u16 red;
    u16 green;
    u16 blue;
    u16 alpha;

    if (((n64_color & 1u) == 0u) &&
        (preserve_transparent_rgb == FALSE))
    {
        return 0u;
    }

    red = (u16)((n64_color >> 11) & 0x1fu);
    green = (u16)((n64_color >> 6) & 0x1fu);
    blue = (u16)((n64_color >> 1) & 0x1fu);
    alpha = (n64_color & 1u) ? (1u << 15) : 0u;
    return (u16)(alpha | red | (green << 5) | (blue << 10));
}

static u16 ndsRendererHardwareConvertRgba32(u32 rgba)
{
    u8 red = (u8)(rgba >> 24);
    u8 green = (u8)(rgba >> 16);
    u8 blue = (u8)(rgba >> 8);
    u8 alpha = (u8)rgba;

    if (alpha == 0u)
    {
        return 0u;
    }
    return (u16)((1u << 15) |
                 ((u16)(red >> 3)) |
                 ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static u16 ndsRendererHardwareConvertI(u8 intensity)
{
    u16 v;

    v = (u16)(intensity >> 3);
    return (u16)((1u << 15) | v | (v << 5) | (v << 10));
}

static u16 ndsRendererHardwareConvertI16(u16 value)
{
    u8 intensity = (u8)(value >> 8);

    if (intensity == 0u)
    {
        intensity = (u8)value;
    }
    return ndsRendererHardwareConvertI(intensity);
}

static u16 ndsRendererHardwareConvertIA(u8 intensity, u8 alpha)
{
    u16 v;

    if (alpha == 0u)
    {
        return 0u;
    }
    v = (u16)(intensity >> 3);
    return (u16)((1u << 15) | v | (v << 5) | (v << 10));
}

static u32 ndsRendererHardwareTextureLinePixels(u32 size, u32 line)
{
    switch (size)
    {
    case NDS_RENDERER_HW_TEXTURE_SIZ_4B:
        return line * 16u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_8B:
        return line * 8u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_16B:
        return line * 4u;
    case NDS_RENDERER_HW_TEXTURE_SIZ_32B:
        return line * 2u;
    default:
        return 0u;
    }
}

static u32 ndsRendererHardwareTextureSourceBytes(u32 format, u32 size,
                                                 u32 texels)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return texels * sizeof(u16);
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
            texels * sizeof(u32) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            return (texels + 1u) >> 1;
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return texels;
        }
        return (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ?
            texels * sizeof(u16) : 0u;
    }
    return 0u;
}

static u32 ndsRendererHardwareTextureSourceWidthPixels(u32 render_size,
                                                       u32 image_size,
                                                       u32 image_width)
{
    if ((render_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (image_size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))
    {
        return image_width * 2u;
    }
    return image_width;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererHardwareTextureFormatBit(u32 format, u32 size);
#endif

static NDSRendererTextureDataLayout ndsRendererTextureDataLayout(
    const NDSRendererConfig *config)
{
    if (config == NULL)
    {
        return NDS_RENDERER_TEXTURE_DATA_NATIVE;
    }
    if (config->texture_data_layout ==
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED)
    {
        return NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    }
    return NDS_RENDERER_TEXTURE_DATA_NATIVE;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererTexturePackMap(
    NDSRendererTextureDataLayout layout, u32 stride)
{
    u32 map = 0u;
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        u32 physical = i;

        if (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED)
        {
            physical = i ^ stride;
        }
        map |= (physical & 0xffu) << (i * 8u);
    }
    return map;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererRecordTextureLaneCount(
    NDSRendererTextureDataLayout layout, u32 is_halfword,
    u32 format, u32 size, u32 count)
{
    u32 format_bit = ndsRendererHardwareTextureFormatBit(format, size);

    gNdsRendererProfileTextureLaneLayoutMask |= 1u << (u32)layout;
    if (is_halfword != 0u)
    {
        gNdsRendererProfileTextureLaneHalfwordAccessCount += count;
        gNdsRendererProfileTextureLaneHalfwordFormatMask |= format_bit;
        gNdsRendererProfileTextureLaneHalfwordMap =
            ndsRendererTexturePackMap(layout, 1u);
    }
    else
    {
        gNdsRendererProfileTextureLaneByteAccessCount += count;
        gNdsRendererProfileTextureLaneByteFormatMask |= format_bit;
        gNdsRendererProfileTextureLaneByteMap =
            ndsRendererTexturePackMap(layout, 3u);
    }
}
#endif

static u32 ndsRendererTextureLogicalByteIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 3u) : logical_index;
}

static u32 ndsRendererTextureLogicalHalfwordIndex(
    NDSRendererTextureDataLayout layout, u32 logical_index)
{
    return (layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ?
        (logical_index ^ 1u) : logical_index;
}

static u32 ndsRendererTexturePhysicalByteSpan(
    NDSRendererTextureDataLayout layout, u32 logical_bytes)
{
    if ((layout == NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) &&
        (logical_bytes != 0u))
    {
        return (logical_bytes + 3u) & ~3u;
    }
    return logical_bytes;
}

static u8 ndsRendererReadTextureByte(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    /* The conversion loop aggregates this access after rasterization. Do not
     * update volatile diagnostics for every converted texel: animated
     * TEXEL0/TEXEL1 water reads this path tens of thousands of times per
     * frame, while the lane contract is invariant for the conversion. */
    (void)format;
    (void)size;
    return texels[ndsRendererTextureLogicalByteIndex(layout, logical_index)];
}

static u8 ndsRendererReadTexturePackedNibble(
    const NDSRendererConfig *config, const u8 *texels, u32 logical_texel_index,
    u32 format, u32 size)
{
    u8 packed = ndsRendererReadTextureByte(
        config, texels, logical_texel_index >> 1, format, size);

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

static u16 ndsRendererReadTextureHalfword(
    const NDSRendererConfig *config, const u16 *data, u32 logical_index,
    u32 format, u32 size)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    (void)format;
    (void)size;
    return data[ndsRendererTextureLogicalHalfwordIndex(layout, logical_index)];
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererRecordTextureLaneUseCount(
    const NDSRendererConfig *config, u32 format, u32 size, u32 count)
{
    NDSRendererTextureDataLayout layout = ndsRendererTextureDataLayout(config);

    if ((size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B))
    {
        ndsRendererRecordTextureLaneCount(
            layout, FALSE, format, size, count);
    }
    else if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
    {
        ndsRendererRecordTextureLaneCount(
            layout, TRUE, format, size, count);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        ndsRendererRecordTextureLaneCount(
            layout, TRUE, format, NDS_RENDERER_HW_TEXTURE_SIZ_16B, count);
    }
}

static void ndsRendererRecordTextureLaneUse(
    const NDSRendererConfig *config, u32 format, u32 size)
{
    ndsRendererRecordTextureLaneUseCount(config, format, size, 1u);
}
#endif

static u16 ndsRendererHardwarePaletteColor(
    const NDSRendererConfig *config, const u16 *palette, u32 index, u32 count,
    s32 preserve_transparent_rgb)
{
    if ((palette == NULL) || (index >= count))
    {
        return 0u;
    }
    return ndsRendererHardwareConvertRgba16(
        ndsRendererReadTextureHalfword(
            config, palette, index, NDS_RENDERER_HW_TEXTURE_FMT_CI,
            NDS_RENDERER_HW_TEXTURE_SIZ_16B),
        preserve_transparent_rgb);
}

static u16 ndsRendererHardwareTextureColor(
    const NDSRendererConfig *config,
    u32 format,
    u32 size,
    const u8 *texels,
    const u16 *palette,
    u32 palette_count,
    u32 palette_base,
    u32 index,
    s32 preserve_transparent_rgb)
{
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_index;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_index = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
        }
        else
        {
            palette_index = ndsRendererReadTextureByte(
                config, texels, index, format, size);
        }
        palette_index += palette_base;
        return ndsRendererHardwarePaletteColor(config, palette, palette_index,
                                               palette_count,
                                               preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_RGBA16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B)
        {
            u32 rgba;

            memcpy(&rgba, &texels[index * sizeof(rgba)], sizeof(rgba));
            return ndsRendererHardwareConvertRgba32(rgba);
        }
        return ndsRendererHardwareConvertRgba16(
            ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size),
            preserve_transparent_rgb);
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_IA)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 value = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);
            u8 intensity = (u8)(((value >> 1) & 0x07u) * 0x24u);
            u8 alpha = (value & 1u) ? 0xffu : 0u;

            return ndsRendererHardwareConvertIA(intensity, alpha);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            u8 value = ndsRendererReadTextureByte(
                config, texels, index, format, size);
            u8 intensity = (u8)((value >> 4) * 0x11u);
            u8 alpha = (u8)((value & 0x0fu) * 0x11u);

            return ndsRendererHardwareConvertIA(intensity, alpha);
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            u16 value = ndsRendererReadTextureHalfword(
                config, (const u16 *)texels, index, format, size);

            return ndsRendererHardwareConvertIA((u8)(value >> 8),
                                                (u8)value);
        }
        return 0u;
    }
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_I16)
    {
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            u8 intensity4 = ndsRendererReadTexturePackedNibble(
                config, texels, index, format, size);

            return ndsRendererHardwareConvertI((u8)(intensity4 * 0x11u));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_8B)
        {
            return ndsRendererHardwareConvertI(
                ndsRendererReadTextureByte(config, texels, index,
                                           format, size));
        }
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_16B)
        {
            return ndsRendererHardwareConvertI16(
                ndsRendererReadTextureHalfword(
                    config, (const u16 *)texels, index, format, size));
        }
    }
    return 0u;
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static s32 ndsRendererTextureColorNonWhite(u16 color)
{
    u32 r;
    u32 g;
    u32 b;

    if ((color & (1u << 15)) == 0u)
    {
        return FALSE;
    }
    r = color & 0x1fu;
    g = (color >> 5) & 0x1fu;
    b = (color >> 10) & 0x1fu;
    return ((r < 29u) || (g < 29u) || (b < 29u)) ? TRUE : FALSE;
}

static s32 ndsRendererTextureColorDominantGreen(u16 color)
{
    u32 r;
    u32 g;
    u32 b;

    if ((color & (1u << 15)) == 0u)
    {
        return FALSE;
    }
    r = color & 0x1fu;
    g = (color >> 5) & 0x1fu;
    b = (color >> 10) & 0x1fu;
    return ((g >= 10u) && (g > (r + 2u)) && (g > (b + 2u))) ?
        TRUE : FALSE;
}

static void ndsRendererProfileTexturePixel(u16 color, u32 *green_texels,
                                           u32 *nonwhite_texels)
{
    if (ndsRendererTextureColorNonWhite(color) != FALSE)
    {
        if (nonwhite_texels != NULL)
        {
            (*nonwhite_texels)++;
        }
    }
    if (ndsRendererTextureColorDominantGreen(color) != FALSE)
    {
        if (green_texels != NULL)
        {
            (*green_texels)++;
        }
    }
}

static void ndsRendererProfileTextureCacheEntry(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    if ((entry == NULL) || (entry->ready == FALSE))
    {
        return;
    }
    gNdsRendererProfileTextureSourceTexels += entry->source_texels;
    gNdsRendererProfileTextureGreenTexels += entry->green_texels;
    gNdsRendererProfileTextureNonWhiteTexels += entry->nonwhite_texels;
}

static s32 ndsRendererHardwareTextureWrapCoord(s32 coord, u32 size,
                                               u32 wrap, u32 mirror)
{
    s32 period;

    if (size == 0u)
    {
        return 0;
    }
    if (wrap == 0u)
    {
        if (coord < 0)
        {
            return 0;
        }
        if ((u32)coord >= size)
        {
            return (s32)size - 1;
        }
        return coord;
    }

    period = (s32)((mirror != 0u) ? size * 2u : size);
    if (period <= 0)
    {
        return 0;
    }
    coord %= period;
    if (coord < 0)
    {
        coord += period;
    }
    if ((mirror != 0u) && ((u32)coord >= size))
    {
        coord = ((s32)size * 2) - 1 - coord;
    }
    return coord;
}

static void ndsRendererProfileTextureSample(s16 s, s16 t)
{
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    s32 sample_s;
    s32 sample_t;

    if ((entry == NULL) ||
        (entry->profile_width == 0u) ||
        (entry->profile_height == 0u))
    {
        return;
    }

    sample_s = ndsRendererHardwareTextureWrapCoord(
        ((s32)s) >> 4, entry->profile_width,
        (entry->params & GL_TEXTURE_WRAP_S) != 0u,
        (entry->params & GL_TEXTURE_FLIP_S) != 0u);
    sample_t = ndsRendererHardwareTextureWrapCoord(
        ((s32)t) >> 4, entry->profile_height,
        (entry->params & GL_TEXTURE_WRAP_T) != 0u,
        (entry->params & GL_TEXTURE_FLIP_T) != 0u);
    if (((u32)sample_s >= entry->profile_width) ||
        ((u32)sample_t >= entry->profile_height))
    {
        return;
    }

    gNdsRendererProfileTextureSampleCount++;
    if (entry->nonwhite_texels != 0u)
    {
        gNdsRendererProfileTextureSampleNonWhiteCount++;
    }
    if (entry->green_texels != 0u)
    {
        gNdsRendererProfileTextureSampleGreenCount++;
    }
}

static u32 ndsRendererHardwareTextureFormatBit(u32 format, u32 size)
{
    u32 index = (format * 4u) + size;

    return (index < 32u) ? (1u << index) : 0u;
}

static void ndsRendererProfileTextureFormat(volatile u32 *mask,
                                            u32 format, u32 size)
{
    if (mask != NULL)
    {
        *mask |= ndsRendererHardwareTextureFormatBit(format, size);
    }
}
#endif

static void ndsRendererHardwareRejectTexture(NDSRendererStats *stats,
                                             u32 format, u32 size,
                                             u32 reason)
{
    if (stats != NULL)
    {
        stats->hardware_texture_reject_count++;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureRejectFormatMask, format, size);
    gNdsRendererProfileTextureRejectReasonMask |= reason;
#else
    (void)format;
    (void)size;
    (void)reason;
#endif
}

static s32 ndsRendererHardwarePrepareTexel1Source(
    const NDSRendererStats *stats,
    const NDSRendererConfig *config,
    u32 primary_format,
    u32 primary_size,
    u32 primary_width,
    u32 primary_height,
    NDSRendererHardwareTexel1Source *out)
{
    const NDSRendererTileState *tile;
    const NDSRendererTextureLoadState *load;
    u32 loaded_bytes;
    u32 width;
    u32 height;
    u32 texels;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_last_index;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 source_width;
    u32 source_origin_s;
    u32 source_origin_t;
    s32 materialize_s;
    s32 materialize_t;

    if (out == NULL)
    {
        return FALSE;
    }
    memset(out, 0, sizeof(*out));
    if ((stats == NULL) ||
        (ndsRendererHardwareUsesTexel01Lerp(stats) == FALSE) ||
        (ndsRendererActiveTextureTile(stats) != NDS_RENDERER_RENDER_TILE))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_ACTIVE_TILE);
        return FALSE;
    }

    tile = &stats->texture_tiles[NDS_RENDERER_RENDER_TILE_1];
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTexel1LastTileState =
        (tile->set_seen & 1u) |
        ((tile->size_seen & 1u) << 1) |
        ((tile->line & 0x1ffu) << 2) |
        ((tile->format & 0x7u) << 11) |
        ((tile->size & 0x3u) << 14) |
        ((tile->shifts & 0xfu) << 16) |
        ((tile->shiftt & 0xfu) << 20);
    gNdsRendererProfileTexel1LastPrimaryState =
        (primary_format & 0x7u) |
        ((primary_size & 0x3u) << 3) |
        ((primary_width & 0xffu) << 8) |
        ((primary_height & 0xffu) << 16);
#endif
    if ((primary_format != NDS_RENDERER_HW_TEXTURE_FMT_CI) ||
        (primary_size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) ||
        (tile->set_seen == 0u) || (tile->size_seen == 0u) ||
        (tile->line == 0u) || (tile->shifts != 0u) ||
        (tile->shiftt != 0u) ||
        (tile->format != primary_format) || (tile->size != primary_size))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_TILE_STATE);
        return FALSE;
    }
    load = ndsRendererHardwareFindTextureLoadForTmem(stats, tile->tmem);
    if ((load == NULL) || (load->image == 0u) ||
        (load->load_texels == 0u))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
        return FALSE;
    }

    loaded_bytes = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        load->load_texels * sizeof(u32) :
        load->load_texels * sizeof(u16);
    width = tile->width;
    height = tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(
             primary_format, primary_size, width * height) > loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(primary_size,
                                                     tile->line);
        texels = load->load_texels * sizeof(u16);
        if (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_DIMENSIONS);
        return FALSE;
    }

    out->source_extent_width = width;
    out->source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cms, tile->masks, width, tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        tile->cmt, tile->maskt, height, tile->height);
    if (materialize_s != FALSE)
    {
        width = tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = tile->height;
    }
    if ((width != primary_width) || (height != primary_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_PAIR_SIZE);
        return FALSE;
    }

    if (load->load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = load->load_uls >> 2;
        source_origin_t = load->load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            primary_size, load->image_size, load->image_width);
    }
    else
    {
        u32 dxt = load->load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = out->source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords =
                (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            source_width = ndsRendererHardwareTextureLinePixels(
                primary_size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)) ||
        (source_read_width > out->source_extent_width) ||
        (source_read_height > out->source_extent_height))
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_RANGE);
        return FALSE;
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUse(config, primary_format, primary_size);
#endif
    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(
        primary_format, primary_size, source_last_index + 1u);
    if (source_bytes == 0u)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    out->texels = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)load->image,
        source_physical_bytes);
    if (out->texels == NULL)
    {
        ndsRendererProfileRecordTexel1RejectReason(
            NDS_RENDERER_HW_TEXEL1_REJECT_SOURCE_PTR);
        return FALSE;
    }

    out->load = load;
    out->render_tile = tile;
    out->format = primary_format;
    out->size = primary_size;
    out->width = width;
    out->height = height;
    out->source_width = source_width;
    out->source_origin_s = source_origin_s;
    out->source_origin_t = source_origin_t;
    out->palette_base = (primary_size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
        tile->palette * 16u : 0u;
    out->materialize_s = materialize_s;
    out->materialize_t = materialize_t;
    return TRUE;
}

static s32 ndsRendererHardwareTileOriginDelta(u32 primary, u32 secondary)
{
    s32 delta = (s32)((primary - secondary) & 0x0fffu);

    if ((delta & 0x0800) != 0)
    {
        delta -= 0x1000;
    }
    return delta;
}

static s32 ndsRendererHardwareQuarterToTexel(s32 coord)
{
    if (coord < 0)
    {
        return -(((-coord) + 2) >> 2);
    }
    return (coord + 2) >> 2;
}

static u32 ndsRendererHardwareTextureAddressCoord(
    s32 coord, u32 logical_extent, u32 source_extent, u32 mode, u32 mask)
{
    s32 period;
    s32 local;
    u32 mask_extent;

    if (source_extent == 0u)
    {
        return 0u;
    }
    /* The common interior case is already in the first source/mask period.
     * Return it directly instead of paying signed divide/modulo for every
     * TEXEL1 pixel; edge, wrap and mirror coordinates retain the full path. */
    if ((coord >= 0) && ((u32)coord < source_extent) &&
        ((logical_extent == 0u) || ((u32)coord < logical_extent)) &&
        ((mask == 0u) || (mask >= 31u) ||
         ((u32)coord < (1u << mask))))
    {
        return (u32)coord;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        if (coord < 0)
        {
            coord = 0;
        }
        else if ((logical_extent != 0u) &&
                 ((u32)coord >= logical_extent))
        {
            coord = (s32)logical_extent - 1;
        }
    }
    if ((mask != 0u) && (mask < 31u))
    {
        mask_extent = 1u << mask;
        period = coord / (s32)mask_extent;
        local = coord % (s32)mask_extent;
        if (local < 0)
        {
            local += (s32)mask_extent;
            period--;
        }
        if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
            ((period & 1) != 0))
        {
            local = (s32)mask_extent - 1 - local;
        }
        return ((u32)local < source_extent) ? (u32)local :
            (source_extent - 1u);
    }
    local = coord % (s32)source_extent;
    if (local < 0)
    {
        local += (s32)source_extent;
    }
    if ((mode & NDS_RENDERER_TX_CLAMP) != 0u)
    {
        return ((u32)coord < source_extent) ? (u32)coord :
            (source_extent - 1u);
    }
    return (u32)local;
}

static u32 ndsRendererHardwareTexel1SourceIndex(
    const NDSRendererHardwareTexel1Source *source,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    s32 source_x;
    s32 source_y;
    u32 addressed_x;
    u32 addressed_y;
    u32 index;

    source_x = (s32)x + origin_delta_s;
    source_y = (s32)y + origin_delta_t;
    addressed_x = ndsRendererHardwareTextureAddressCoord(
        source_x, source->render_tile->width,
        source->source_extent_width, source->render_tile->cms,
        source->render_tile->masks);
    addressed_y = ndsRendererHardwareTextureAddressCoord(
        source_y, source->render_tile->height,
        source->source_extent_height, source->render_tile->cmt,
        source->render_tile->maskt);
    index = ((source->source_origin_t + addressed_y) *
             source->source_width) + source->source_origin_s + addressed_x;
    return index;
}

static u16 ndsRendererHardwareTexel1Color(
    const NDSRendererHardwareTexel1Source *source,
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    s32 origin_delta_s,
    s32 origin_delta_t,
    u32 x,
    u32 y)
{
    u32 index;

    if ((source == NULL) || (source->render_tile == NULL) ||
        (source->texels == NULL))
    {
        return 0u;
    }
    index = ndsRendererHardwareTexel1SourceIndex(
        source, origin_delta_s, origin_delta_t, x, y);
    return ndsRendererHardwareTextureColor(
        config, source->format, source->size, source->texels, palette,
        palette_count, source->palette_base, index, TRUE);
}

static u32 ndsRendererHardwareAlphaCoverageThreshold(u32 x, u32 y)
{
    static const u8 bayer4x4[16] = {
        0u, 8u, 2u, 10u,
        12u, 4u, 14u, 6u,
        3u, 11u, 1u, 9u,
        15u, 7u, 13u, 5u
    };

    return ((u32)bayer4x4[((y & 3u) << 2) | (x & 3u)] << 4) + 8u;
}

static u32 ndsRendererHardwareExpand5To8(u32 value)
{
    value &= 0x1fu;
    return (value << 3) | (value >> 2);
}

static u32 ndsRendererHardwareBlendTexel01Value(u16 texel0, u16 texel1,
                                                u32 fraction)
{
    u32 inverse;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha_coverage;
    u32 texel0_red;
    u32 texel0_green;
    u32 texel0_blue;
    u32 texel1_red;
    u32 texel1_green;
    u32 texel1_blue;

    if (fraction > 0xffu)
    {
        fraction = 0xffu;
    }
    inverse = 0x100u - fraction;
    texel0_red = ndsRendererHardwareExpand5To8(texel0 >> 0);
    texel0_green = ndsRendererHardwareExpand5To8(texel0 >> 5);
    texel0_blue = ndsRendererHardwareExpand5To8(texel0 >> 10);
    texel1_red = ndsRendererHardwareExpand5To8(texel1 >> 0);
    texel1_green = ndsRendererHardwareExpand5To8(texel1 >> 5);
    texel1_blue = ndsRendererHardwareExpand5To8(texel1 >> 10);
    red = (((texel0_red * inverse) + (texel1_red * fraction)) >> 8) >> 3;
    green = (((texel0_green * inverse) +
              (texel1_green * fraction)) >> 8) >> 3;
    blue = (((texel0_blue * inverse) +
             (texel1_blue * fraction)) >> 8) >> 3;
    /* G_RM_AA_TEX_EDGE2 converts the fractional alpha lerp to coverage. DS
     * direct-color textures expose A1 only, so retain the same mean coverage
     * with an ordered 4x4 decision instead of unioning both silhouettes. */
    alpha_coverage = ((((texel0 >> 15) & 1u) * 0x100u * inverse) +
                      (((texel1 >> 15) & 1u) * 0x100u * fraction)) >> 8;
    return red | (green << 5) | (blue << 10) |
        (alpha_coverage << NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT);
}

static u16 ndsRendererHardwareResolveTexel01Value(u32 value, u32 x, u32 y)
{
    u32 alpha_coverage =
        value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
    u32 alpha = (alpha_coverage >
                 ndsRendererHardwareAlphaCoverageThreshold(x, y)) ? 1u : 0u;

    return (u16)((value & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
                 (alpha << 15));
}

static u16 ndsRendererHardwareBlendTexel01(u16 texel0, u16 texel1,
                                           u32 fraction, u32 x, u32 y)
{
    return ndsRendererHardwareResolveTexel01Value(
        ndsRendererHardwareBlendTexel01Value(texel0, texel1, fraction),
        x, y);
}

static void __attribute__((noinline))
ndsRendererHardwareBuildTexel01Ci4Lut(
    const NDSRendererConfig *config,
    const u16 *palette,
    u32 palette_count,
    u32 palette0_base,
    u32 palette1_base,
    u32 fraction)
{
    static const u16 alpha_phase_prefix[17] = {
        0x0000u, 0x0001u, 0x0401u, 0x0405u, 0x0505u, 0x0525u,
        0x8525u, 0x85a5u, 0xa5a5u, 0xa5a7u, 0xada7u, 0xadafu,
        0xafafu, 0xafbfu, 0xefbfu, 0xefffu, 0xffffu
    };
    u16 palette0[16];
    u16 palette1[16];
    u32 index0;
    u32 index1;

    for (index0 = 0u; index0 < 16u; index0++)
    {
        palette0[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette0_base + index0, palette_count, TRUE);
        palette1[index0] = ndsRendererHardwarePaletteColor(
            config, palette, palette1_base + index0, palette_count, TRUE);
    }
    if ((sNdsRendererHardwareTexel01Ci4LutKeyValid != 0u) &&
        (sNdsRendererHardwareTexel01Ci4LutFraction == fraction) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette0,
                palette0, sizeof(palette0)) == 0) &&
        (memcmp(sNdsRendererHardwareTexel01Ci4LutPalette1,
                palette1, sizeof(palette1)) == 0))
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        sNdsRendererProfileCi4LutReuseCount++;
#endif
        return;
    }
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette0,
           palette0, sizeof(palette0));
    memcpy(sNdsRendererHardwareTexel01Ci4LutPalette1,
           palette1, sizeof(palette1));
    sNdsRendererHardwareTexel01Ci4LutFraction = fraction;
    sNdsRendererHardwareTexel01Ci4LutKeyValid = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsRendererProfileCi4LutBuildCount++;
#endif
    for (index0 = 0u; index0 < 16u; index0++)
    {
        for (index1 = 0u; index1 < 16u; index1++)
        {
            u32 value = ndsRendererHardwareBlendTexel01Value(
                palette0[index0], palette1[index1], fraction);
            u32 alpha_coverage =
                value >> NDS_RENDERER_HW_TEXEL01_COVERAGE_SHIFT;
            u32 alpha_prefix_count = (alpha_coverage + 7u) >> 4;
            u32 lut_index = (index0 << 4) | index1;
            u16 rgb;
            u16 alpha_phase_mask;
            u32 phase;

            if (alpha_prefix_count > 16u)
            {
                alpha_prefix_count = 16u;
            }
            rgb = (u16)(value & NDS_RENDERER_HW_TEXEL01_RGB_MASK);
            alpha_phase_mask = alpha_phase_prefix[alpha_prefix_count];
            for (phase = 0u;
                 phase < NDS_RENDERER_HW_TEXEL01_CI4_PHASE_COUNT;
                 phase++)
            {
                u32 alpha = (alpha_phase_mask >> phase) & 1u;

                sNdsRendererHardwareTexel01Ci4PhaseLut[
                    (phase * NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT) +
                    lut_index] =
                        (u16)(rgb | (alpha << 15));
            }
        }
    }
}

static inline u16 ndsRendererHardwareResolveTexel01Ci4Lut(
    u32 index0, u32 index1, u32 x, u32 y)
{
    u32 phase = ((y & 3u) << 2) | (x & 3u);
    u32 lut_index = (index0 << 4) | index1;

    return sNdsRendererHardwareTexel01Ci4PhaseLut[
        (phase * NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT) + lut_index];
}

static inline u8 ndsRendererHardwareReadCi4Direct(
    const u8 *texels, u32 logical_texel_index, u32 byte_lane_xor)
{
    u8 packed = texels[(logical_texel_index >> 1) ^ byte_lane_xor];

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

static void NDS_RENDERER_HOT_CODE
ndsRendererHardwareConvertTexel01Ci4Direct(
    const NDSRendererConfig *config,
    const u8 *texels0,
    u32 source0_width,
    u32 source0_origin_s,
    u32 source0_origin_t,
    const NDSRendererTileState *render_tile0,
    s32 materialize0_s,
    s32 materialize0_t,
    const NDSRendererHardwareTexel1Source *source1,
    s32 origin1_delta_s,
    s32 origin1_delta_t,
    u32 width,
    u32 height,
    u32 upload_width,
    u32 *green_texels,
    u32 *nonwhite_texels)
{
    u32 byte_lane_xor =
        (ndsRendererTextureDataLayout(config) ==
         NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ? 3u : 0u;
    const NDSRendererTileState *render_tile1 = source1->render_tile;
    u32 x;
    u32 y;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    (void)green_texels;
    (void)nonwhite_texels;
#endif
    /* Animated tile origins can wrap or mirror TEXEL1. Resolve those exact
     * addressing rules once per S coordinate, then reuse them for every row. */
    for (x = 0u; x < width; x++)
    {
        sNdsRendererHardwareTexel01Ci4Source0S[x] = (u8)(
            (materialize0_s != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    x, render_tile0->cms, render_tile0->masks) : x);
        sNdsRendererHardwareTexel01Ci4Source1S[x] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)x + origin1_delta_s, render_tile1->width,
                source1->source_extent_width, render_tile1->cms,
                render_tile1->masks);
    }
    for (y = 0u; y < height; y++)
    {
        u32 source0_y = (materialize0_t != FALSE) ?
            ndsRendererHardwareTextureMaskedAddress(
                y, render_tile0->cmt, render_tile0->maskt) : y;
        u32 source1_y = ndsRendererHardwareTextureAddressCoord(
            (s32)y + origin1_delta_t, render_tile1->height,
            source1->source_extent_height, render_tile1->cmt,
            render_tile1->maskt);
        u32 source0_row =
            ((source0_origin_t + source0_y) * source0_width) +
            source0_origin_s;
        u32 source1_row =
            ((source1->source_origin_t + source1_y) *
             source1->source_width) + source1->source_origin_s;
        u32 dst_index = y * upload_width;
        u32 phase_row = (y & 3u) << 10;

        for (x = 0u; x < width; x++)
        {
            u32 source0_index = source0_row +
                sNdsRendererHardwareTexel01Ci4Source0S[x];
            u32 source1_index = source1_row +
                sNdsRendererHardwareTexel01Ci4Source1S[x];
            u32 index0 = ndsRendererHardwareReadCi4Direct(
                texels0, source0_index, byte_lane_xor);
            u32 index1 = ndsRendererHardwareReadCi4Direct(
                source1->texels, source1_index, byte_lane_xor);
            u32 phase_lut_index = phase_row | ((x & 3u) << 8) |
                (index0 << 4) | index1;
            u16 color =
                sNdsRendererHardwareTexel01Ci4PhaseLut[phase_lut_index];

            sNdsRendererHardwareTextureScratch[dst_index + x] = color;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            ndsRendererProfileTexturePixel(
                color, green_texels, nonwhite_texels);
#endif
        }
    }
}

static s32 ndsRendererHardwareBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 texture_start = cpuGetTiming();
    u32 convert_start;
    u32 upload_start;
#endif
    NDSRendererHardwareTextureKey key;
    NDSRendererHardwareTextureCacheEntry *entry;
    NDSRendererHardwareTextureCacheEntry *fraction_entry;
    NDSRendererHardwareTexel1Source texel1_source;
    const NDSRendererTextureLoadState *primary_load;
    const u8 *texels_src;
    const u16 *tlut_src;
    u32 width;
    u32 height;
    u32 format;
    u32 size;
    u32 upload_width;
    u32 upload_height;
    u32 texels;
    u32 bytes;
    u32 loaded_bytes;
    u32 source_extent_width;
    u32 source_extent_height;
    u32 source_width;
    u32 source_read_width;
    u32 source_read_height;
    u32 source_origin_s;
    u32 source_origin_t;
    u32 source_last_index;
    u32 source_texels;
    u32 source_bytes;
    u32 source_physical_bytes;
    u32 palette_base;
    u32 tlut_physical_bytes;
    u32 params;
    u32 render_tile_index;
    u32 render_tile_flags;
    u32 upload_attempts;
    u32 primary_image;
    u32 primary_image_format;
    u32 primary_image_size;
    u32 primary_image_width;
    u32 primary_load_kind;
    u32 primary_load_tile;
    u32 primary_load_uls;
    u32 primary_load_ult;
    u32 primary_load_lrs;
    u32 primary_load_dxt;
    u32 primary_load_texels;
    s32 materialize_s;
    s32 materialize_t;
    s32 wants_texel1;
    s32 use_texel1 = FALSE;
    s32 use_texel1_ci4_lut = FALSE;
    s32 use_texel1_ci4_direct = FALSE;
    s32 texel1_origin_delta_s = 0;
    s32 texel1_origin_delta_t = 0;
    const NDSRendererTileState *render_tile;
    u32 green_texels = 0u;
    u32 nonwhite_texels = 0u;
    int size_x;
    int size_y;
    u32 x;
    u32 y;

    if (stats == NULL)
    {
        return FALSE;
    }
    ndsRendererSyncTextureTile(stats);
    render_tile_index = ndsRendererActiveTextureTile(stats);
    render_tile = &stats->texture_tiles[render_tile_index];
    wants_texel1 = ndsRendererHardwareUsesTexel01Lerp(stats);
    primary_load = NULL;
    primary_image = stats->texture_image;
    primary_image_format = stats->texture_format;
    primary_image_size = stats->texture_size;
    primary_image_width = stats->texture_image_width;
    primary_load_kind = stats->texture_load_kind;
    primary_load_tile = stats->texture_load_tile;
    primary_load_uls = stats->texture_load_block_uls;
    primary_load_ult = stats->texture_load_block_ult;
    primary_load_lrs = stats->texture_load_block_lrs;
    primary_load_dxt = stats->texture_load_block_dxt;
    primary_load_texels = stats->texture_load_texels;
    if (wants_texel1 != FALSE)
    {
        /* A two-texture combiner consumes the images resident at each render
         * tile's TMEM address. SETTIMG is mutable, so the last global image is
         * not necessarily TEXEL0 after both LOADBLOCK commands have run. */
        primary_load = ndsRendererHardwareFindTextureLoadForTmem(
            stats, render_tile->tmem);
        if ((primary_load == NULL) || (primary_load->image == 0u) ||
            (primary_load->load_texels == 0u))
        {
            ndsRendererProfileRecordTexel1Reject();
            ndsRendererProfileRecordTexel1RejectReason(
                NDS_RENDERER_HW_TEXEL1_REJECT_LOAD_STATE);
            ndsRendererHardwareRejectTexture(
                stats, stats->texture_format, stats->texture_size,
                NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
            return FALSE;
        }
        primary_image = primary_load->image;
        primary_image_format = primary_load->image_format;
        primary_image_size = primary_load->image_size;
        primary_image_width = primary_load->image_width;
        primary_load_kind = primary_load->load_kind;
        primary_load_tile = primary_load->load_tile;
        primary_load_uls = primary_load->load_uls;
        primary_load_ult = primary_load->load_ult;
        primary_load_lrs = primary_load->load_lrs;
        primary_load_dxt = primary_load->load_dxt;
        primary_load_texels = primary_load->load_texels;
    }
    render_tile_flags = 0u;
    if (render_tile->set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_RENDER_SEEN |
            render_tile->flags;
    }
    if (stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen != 0u)
    {
        render_tile_flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }
    if ((((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_ON) == 0u) &&
         (ndsRendererHardwareTextureImplicitStateOn(stats) == FALSE)) ||
        (primary_image == 0u) ||
        (render_tile->line == 0u) ||
        (primary_load_texels == 0u))
    {
        ndsRendererHardwareRejectTexture(
            stats, stats->texture_format, stats->texture_size,
            NDS_RENDERER_HW_TEXREJECT_MISSING_STATE);
        return FALSE;
    }

    if (render_tile->set_seen != 0u)
    {
        format = render_tile->format;
        size = render_tile->size;
    }
    else
    {
        format = stats->texture_format;
        size = stats->texture_size;
    }

    if ((format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        ((size != NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
         (size != NDS_RENDERER_HW_TEXTURE_SIZ_8B)))
    {
        if (stats->texture_tlut_count <= 16u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_4B;
        }
        else if (stats->texture_tlut_count <= 256u)
        {
            size = NDS_RENDERER_HW_TEXTURE_SIZ_8B;
        }
        else
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_CI_SIZE);
            return FALSE;
        }
    }
    if ((format != NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_RGBA16) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_IA) &&
        (format != NDS_RENDERER_HW_TEXTURE_FMT_I16))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_UNSUPPORTED_FORMAT);
        return FALSE;
    }

    loaded_bytes = (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B) ?
        primary_load_texels * sizeof(u32) :
        primary_load_texels * sizeof(u16);
    width = render_tile->width;
    height = render_tile->height;
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        (ndsRendererHardwareTextureSourceBytes(format, size, width * height) >
         loaded_bytes))
    {
        width = ndsRendererHardwareTextureLinePixels(
            size, render_tile->line);
        texels = primary_load_texels * sizeof(u16);
        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            texels *= 2u;
        }
        else if ((size == NDS_RENDERER_HW_TEXTURE_SIZ_16B) ||
                 (size == NDS_RENDERER_HW_TEXTURE_SIZ_32B))
        {
            texels /= 2u;
        }
        height = (width != 0u) ? texels / width : 0u;
    }
    if ((width == 0u) || (height == 0u) ||
        (width > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH) ||
        (height > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_DIMENSIONS);
        return FALSE;
    }
    source_extent_width = width;
    source_extent_height = height;
    materialize_s = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cms, render_tile->masks, source_extent_width,
        render_tile->width);
    materialize_t = ndsRendererHardwareTextureMaterializesMaskedClamp(
        render_tile->cmt, render_tile->maskt, source_extent_height,
        render_tile->height);
    if (materialize_s != FALSE)
    {
        width = render_tile->width;
    }
    if (materialize_t != FALSE)
    {
        height = render_tile->height;
    }

    upload_width = ndsRendererHardwareTextureNextPow2(width);
    upload_height = ndsRendererHardwareTextureNextPow2(height);
    if ((upload_width < width) || (upload_height < height) ||
        (ndsRendererHardwareTextureSizeEnum(upload_width, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(upload_height, &size_y) == FALSE))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_UPLOAD_SIZE);
        return FALSE;
    }

    if (primary_load_kind == NDS_RENDERER_TEXTURE_LOADTILE)
    {
        source_origin_s = primary_load_uls >> 2;
        source_origin_t = primary_load_ult >> 2;
        source_width = ndsRendererHardwareTextureSourceWidthPixels(
            size, primary_image_size, primary_image_width);
    }
    else
    {
        u32 dxt = primary_load_dxt;

        source_origin_s = 0u;
        source_origin_t = 0u;
        source_width = source_extent_width;
        if (dxt != 0u)
        {
            u32 qwords = (NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u) / dxt;

            /* BattleShip gbi.h:3291,3309-3317 encodes DXT as the rounded
             * 1.11 reciprocal of 64-bit source words per row. LOADBLOCK's
             * SETTIMG width is one, so DXT owns the DRAM row stride. */
            source_width = ndsRendererHardwareTextureLinePixels(size, qwords);
        }
    }
    source_read_width = (materialize_s != FALSE) ?
        (1u << render_tile->masks) : width;
    source_read_height = (materialize_t != FALSE) ?
        (1u << render_tile->maskt) : height;
    if ((source_width == 0u) ||
        (source_origin_s >= source_width) ||
        (source_read_width > (source_width - source_origin_s)))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_RANGE);
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUse(config, format, size);
#endif
    source_last_index =
        ((source_origin_t + source_read_height - 1u) * source_width) +
        source_origin_s + source_read_width - 1u;
    source_texels = source_last_index + 1u;
    source_bytes = ndsRendererHardwareTextureSourceBytes(format, size,
                                                        source_texels);
    if (source_bytes == 0u)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    source_physical_bytes = ndsRendererTexturePhysicalByteSpan(
        ndsRendererTextureDataLayout(config), source_bytes);
    if (source_physical_bytes < source_bytes)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }

    if (wants_texel1 != FALSE)
    {
        if (ndsRendererHardwarePrepareTexel1Source(
                stats, config, format, size, width, height,
                &texel1_source) != FALSE)
        {
            use_texel1 = TRUE;
            texel1_origin_delta_s = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->uls, texel1_source.render_tile->uls));
            texel1_origin_delta_t = ndsRendererHardwareQuarterToTexel(
                ndsRendererHardwareTileOriginDelta(
                    render_tile->ult, texel1_source.render_tile->ult));
            ndsRendererProfileRecordTexel1Composite();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileTexel1LastFraction =
                stats->prim_lod_fraction;
            gNdsRendererProfileTexel1LastImage0 = primary_image;
            gNdsRendererProfileTexel1LastImage1 =
                texel1_source.load->image;
#endif
        }
        else
        {
            ndsRendererProfileRecordTexel1Reject();
        }
    }

    memset(&key, 0, sizeof(key));
    key.image = primary_image;
    key.image_format = primary_image_format;
    key.image_size = primary_image_size;
    key.image_width = primary_image_width;
    key.tlut_image = stats->texture_tlut_image;
    key.tlut_count = stats->texture_tlut_count;
    key.data_layout = (config != NULL) ?
        (u32)config->texture_data_layout :
        (u32)NDS_RENDERER_TEXTURE_DATA_NATIVE;
    key.format = format;
    key.size = size;
    key.width = width;
    key.height = height;
    key.render_tile = render_tile_index;
    key.render_tmem = render_tile->tmem;
    key.render_palette = render_tile->palette;
    key.render_tile_cms = render_tile->cms;
    key.render_tile_cmt = render_tile->cmt;
    key.render_tile_masks = render_tile->masks;
    key.render_tile_maskt = render_tile->maskt;
    key.render_tile_shifts = render_tile->shifts;
    key.render_tile_shiftt = render_tile->shiftt;
    key.load_tile = primary_load_tile;
    key.load_uls = primary_load_uls;
    key.load_ult = primary_load_ult;
    key.load_lrs = primary_load_lrs;
    key.load_dxt = primary_load_dxt;
    key.load_texels = primary_load_texels;
    key.tile_uls = render_tile->uls;
    key.tile_ult = render_tile->ult;
    key.tile_lrs = render_tile->lrs;
    key.tile_lrt = render_tile->lrt;
    key.line = render_tile->line;
    key.flags = render_tile_flags | (primary_load_kind << 8);
    if (use_texel1 != FALSE)
    {
        const NDSRendererTextureLoadState *load = texel1_source.load;
        const NDSRendererTileState *tile = texel1_source.render_tile;

        key.texel1_image = load->image;
        key.texel1_image_format = load->image_format;
        key.texel1_image_size = load->image_size;
        key.texel1_image_width = load->image_width;
        key.texel1_load_kind = load->load_kind;
        key.texel1_render_tmem = tile->tmem;
        key.texel1_render_line = tile->line;
        key.texel1_render_palette = tile->palette;
        key.texel1_render_tile_cms = tile->cms;
        key.texel1_render_tile_cmt = tile->cmt;
        key.texel1_render_tile_masks = tile->masks;
        key.texel1_render_tile_maskt = tile->maskt;
        key.texel1_render_tile_shifts = tile->shifts;
        key.texel1_render_tile_shiftt = tile->shiftt;
        key.texel1_load_tile = load->load_tile;
        key.texel1_load_uls = load->load_uls;
        key.texel1_load_ult = load->load_ult;
        key.texel1_load_lrs = load->load_lrs;
        key.texel1_load_dxt = load->load_dxt;
        key.texel1_load_texels = load->load_texels;
        key.texel1_tile_uls = tile->uls;
        key.texel1_tile_ult = tile->ult;
        key.texel1_tile_lrs = tile->lrs;
        key.texel1_tile_lrt = tile->lrt;
        key.prim_lod_fraction = stats->prim_lod_fraction;
        key.combine_w0 = stats->texture_combine_w0;
        key.combine_w1 = stats->texture_combine_w1;
    }

    fraction_entry = NULL;
    entry = ndsRendererHardwareFindTexture(&key);
    params = ndsRendererHardwareTextureParams(stats, render_tile,
                                               upload_width, upload_height);
    if (entry != NULL)
    {
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        if (sNdsRendererHardwareActiveTextureEntry != entry)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = format;
        stats->hardware_texture_width = width;
        stats->hardware_texture_height = height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureFormat(
            &gNdsRendererProfileTextureBindFormatMask, format, size);
        ndsRendererProfileTextureCacheEntry(entry);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }
    if (use_texel1 != FALSE)
    {
        fraction_entry =
            ndsRendererHardwareFindTexel1RefreshTexture(&key);
    }

    texels = width * height;
    bytes = ndsRendererHardwareTextureSourceBytes(
        format, size, source_read_width * source_read_height);
    if ((bytes == 0u) || (bytes > loaded_bytes))
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_BYTES);
        return FALSE;
    }
    texels_src = ndsRendererResolveTextureDataPointer(
        config, (const void *)(uintptr_t)primary_image,
        source_physical_bytes);
    if (texels_src == NULL)
    {
        ndsRendererHardwareRejectTexture(
            stats, format, size,
            NDS_RENDERER_HW_TEXREJECT_BAD_SOURCE_PTR);
        return FALSE;
    }
    tlut_src = NULL;
    palette_base = 0u;
    if (format == NDS_RENDERER_HW_TEXTURE_FMT_CI)
    {
        u32 palette_entries = (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) ?
            16u : 256u;

        if (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B)
        {
            palette_base = render_tile->palette * 16u;
            palette_entries += palette_base;
            if ((use_texel1 != FALSE) &&
                (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
                (texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B))
            {
                u32 texel1_palette_entries =
                    texel1_source.palette_base + 16u;

                if (texel1_palette_entries > palette_entries)
                {
                    palette_entries = texel1_palette_entries;
                }
            }
        }
        if ((stats->texture_tlut_image == 0u) ||
            (stats->texture_tlut_count < palette_entries))
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT);
            return FALSE;
        }
        tlut_physical_bytes = ndsRendererTexturePhysicalByteSpan(
            ndsRendererTextureDataLayout(config),
            palette_entries * sizeof(u16));
        tlut_src = ndsRendererResolveTextureDataPointer(
            config, (const void *)(uintptr_t)stats->texture_tlut_image,
            tlut_physical_bytes);
        if (tlut_src == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size,
                NDS_RENDERER_HW_TEXREJECT_BAD_TLUT_PTR);
            return FALSE;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureFormat(
            &gNdsRendererProfileTexturePaletteFormatMask, format, size);
#endif
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 1
    convert_start = cpuGetTiming();
#endif
    if ((use_texel1 != FALSE) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B))
    {
        /* The source pond's two CI4 inputs can produce only 16x16 palette
         * pairs for a given primitive LOD fraction. Precompute all 16 exact
         * ordered-coverage phases once so the animated pixel loop is one
         * pair/phase lookup instead of per-pixel blend or alpha resolution. */
        ndsRendererHardwareBuildTexel01Ci4Lut(
            config, tlut_src, stats->texture_tlut_count, palette_base,
            texel1_source.palette_base, stats->prim_lod_fraction);
        use_texel1_ci4_lut = TRUE;
        use_texel1_ci4_direct = TRUE;
    }

    /* Only the power-of-two rectangle handed to libnds is observable.  The
     * shared scratch arena is sized for the worst 128x128 texture, but smaller
     * animated uploads must not pay to clear the unused tail every frame. */
    memset(sNdsRendererHardwareTextureScratch, 0,
           upload_width * upload_height * sizeof(u16));
    if (use_texel1_ci4_direct != FALSE)
    {
        ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            width, height, upload_width, &green_texels, &nonwhite_texels);
        ndsRendererProfileRecordTextureCi4Direct(width * height);
    }
    else
    {
        for (y = 0u; y < height; y++)
        {
            u32 source_y = (materialize_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile->cmt, render_tile->maskt) : y;

            for (x = 0u; x < width; x++)
            {
                u32 source_x = (materialize_s != FALSE) ?
                    ndsRendererHardwareTextureMaskedAddress(
                        x, render_tile->cms, render_tile->masks) : x;
                u32 src_index =
                    ((source_origin_t + source_y) * source_width) +
                    source_origin_s + source_x;
                u32 dst_index = (y * upload_width) + x;
                u16 color;

                if (use_texel1_ci4_lut != FALSE)
                {
                    u32 index0 = ndsRendererReadTexturePackedNibble(
                        config, texels_src, src_index, format, size);
                    u32 source1_index = ndsRendererHardwareTexel1SourceIndex(
                        &texel1_source, texel1_origin_delta_s,
                        texel1_origin_delta_t, x, y);
                    u32 index1 = ndsRendererReadTexturePackedNibble(
                        config, texel1_source.texels, source1_index,
                        texel1_source.format, texel1_source.size);

                    color = ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y);
                }
                else
                {
                    color = ndsRendererHardwareTextureColor(
                        config, format, size, texels_src, tlut_src,
                        stats->texture_tlut_count, palette_base, src_index,
                        use_texel1);
                    if (use_texel1 != FALSE)
                    {
                        u16 color1 = ndsRendererHardwareTexel1Color(
                            &texel1_source, config, tlut_src,
                            stats->texture_tlut_count,
                            texel1_origin_delta_s, texel1_origin_delta_t,
                            x, y);

                        color = ndsRendererHardwareBlendTexel01(
                            color, color1, stats->prim_lod_fraction, x, y);
                    }
                }
                sNdsRendererHardwareTextureScratch[dst_index] = color;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                ndsRendererProfileTexturePixel(color, &green_texels,
                                               &nonwhite_texels);
#endif
            }
        }
    }
    /* Preserve the canonical lane-observation totals while paying one volatile
     * update per converted texture instead of one per texel. */
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererRecordTextureLaneUseCount(config, format, size, texels);
    if (use_texel1 != FALSE)
    {
        ndsRendererRecordTextureLaneUseCount(
            config, texel1_source.format, texel1_source.size, texels);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureConvertFormatMask, format, size);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureConvertTicks += cpuGetTiming() - convert_start;

    upload_start = cpuGetTiming();
#endif
    entry = fraction_entry;
    if (entry != NULL)
    {
        if (ndsRendererHardwareReplaceTextureData(
                entry, sNdsRendererHardwareTextureScratch,
                upload_width * upload_height * sizeof(u16)) == FALSE)
        {
            entry = ndsRendererHardwareReleaseTexture(entry);
            entry = NULL;
        }
        else
        {
            ndsRendererProfileRecordTexel1Refresh();
        }
    }
    if (entry == NULL)
    {
        entry = ndsRendererHardwareAllocTexture();
        if (entry == NULL)
        {
            ndsRendererHardwareRejectTexture(
                stats, format, size, NDS_RENDERER_HW_TEXREJECT_ALLOC);
            return FALSE;
        }
        if (entry->name == 0)
        {
            if (glGenTextures(1, &entry->name) == 0)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size, NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
        }

        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        upload_attempts = 0u;
        while (glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_x, size_y, 0,
                            params,
                            sNdsRendererHardwareTextureScratch) == 0)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
            upload_attempts++;
            if ((upload_attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
                (ndsRendererHardwareEvictTexture(entry) == FALSE))
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size,
                    NDS_RENDERER_HW_TEXREJECT_TEXIMAGE);
                return FALSE;
            }
            if (glGenTextures(1, &entry->name) == 0)
            {
                ndsRendererHardwareRejectTexture(
                    stats, format, size,
                    NDS_RENDERER_HW_TEXREJECT_GENTEX);
                return FALSE;
            }
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        }
    }
    ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureUploadTicks += cpuGetTiming() - upload_start;
#endif

    entry->key = key;
    entry->params = ndsRendererHardwareMergeTextureParams(params);
    entry->source_texels = texels;
    entry->green_texels = green_texels;
    entry->nonwhite_texels = nonwhite_texels;
    entry->profile_width = upload_width;
    entry->profile_height = upload_height;
    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    entry->ready = TRUE;
    sNdsRendererHardwareActiveTextureEntry = entry;
    stats->hardware_texture_upload_count++;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = format;
    stats->hardware_texture_width = width;
    stats->hardware_texture_height = height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureBindFormatMask, format, size);
#endif
    ndsRendererHardwareApplyTextureParams(entry->params);
    ndsRendererProfileRecordTextureUpload(
        upload_width * upload_height * sizeof(u16));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileTextureCacheEntry(entry);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
    return TRUE;
}

static void ndsRendererHardwareEndBatch(void);

static void ndsRendererCopyMtx20p12ToM4x4(
    const NDSRendererMatrix20p12 *src, m4x4 *dst)
{
    u32 row;
    u32 col;

    if ((src == NULL) || (dst == NULL))
    {
        return;
    }

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            dst->m[(row * 4u) + col] = src->m[row][col];
        }
    }
}

static void ndsRendererBuildRawHardwareMatrix(
    const NDSRendererMatrix20p12 *composed,
    NDSRendererMatrix20p12 *hardware)
{
    u32 col;

    if ((composed == NULL) || (hardware == NULL))
    {
        return;
    }

    /* Source coordinates are submitted as source / 256 in DS 4.12. Keep
     * composed rows 0..2 unchanged and divide the complete homogeneous row 3
     * by the same factor. The GX clip vector is then CPU clip / 256, preserving
     * X/W, Y/W, and Z/W for arbitrary composed/matrix-word state. */
    *hardware = *composed;
    for (col = 0u; col < 4u; col++)
    {
        hardware->m[3][col] = ndsRendererRoundShiftS32Signed(
            hardware->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
}

static void ndsRendererLoadHardwareMatrixPair(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 mode, u32 generation, u32 scale_world)
{
    m4x4 projection_hw;
    m4x4 modelview_hw;

    (void)scale_world;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode == mode) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererHardwareEndBatch();
    ndsRendererCopyMtx20p12ToM4x4(projection, &projection_hw);
    ndsRendererCopyMtx20p12ToM4x4(modelview, &modelview_hw);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hw);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&modelview_hw);

    ndsRendererProfileRecordMatrixLoad();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileMatrixScaleWorld = scale_world;
    gNdsRendererProfileProjectionM00 = projection->m[0][0];
    gNdsRendererProfileProjectionM11 = projection->m[1][1];
    gNdsRendererProfileProjectionM22 = projection->m[2][2];
    gNdsRendererProfileProjectionM32 = projection->m[3][2];
    gNdsRendererProfileModelviewM00 = modelview->m[0][0];
    gNdsRendererProfileModelviewM11 = modelview->m[1][1];
    gNdsRendererProfileModelviewM22 = modelview->m[2][2];
    gNdsRendererProfileModelviewM30 = modelview->m[3][0];
    gNdsRendererProfileModelviewM31 = modelview->m[3][1];
    gNdsRendererProfileModelviewM32 = modelview->m[3][2];
#endif

    sNdsRendererHardwareMatrixMode = mode;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = TRUE;
}

static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *composed, u32 generation)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererBuildRawHardwareMatrix(composed, &modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview, NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED,
        generation, TRUE);
}

static void ndsRendererLoadHardwareMatrices(
    const NDSRendererTraversalState *state, u32 scale_world)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((state != NULL) && (scale_world != 0u) &&
        (state->matrix_valid != 0u))
    {
        if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation))
        {
            return;
        }
        ndsRendererLoadHardwareRawComposedMatrix(
            &state->matrix, state->matrix_generation);
        return;
    }

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY) &&
        (sNdsRendererHardwareMatrixGeneration == 0u))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererMtxIdentity20p12(&modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview,
        NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY, 0u, FALSE);
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u64 ndsRendererHardwareAbsS64(s64 value)
{
    return (value < 0) ? (u64)(-(value + 1)) + 1u : (u64)value;
}

static u64 ndsRendererHardwareAbsDiffS64(s64 lhs, s64 rhs)
{
    if ((lhs < 0) == (rhs < 0))
    {
        return (lhs >= rhs) ? (u64)(lhs - rhs) : (u64)(rhs - lhs);
    }
    return ndsRendererHardwareAbsS64(lhs) + ndsRendererHardwareAbsS64(rhs);
}

static u32 ndsRendererHardwarePosTestCrossError(
    s32 hardware_axis, s32 hardware_w,
    s32 cpu_axis, s32 cpu_w)
{
    s64 lhs = (s64)hardware_axis * cpu_w;
    s64 rhs = (s64)cpu_axis * hardware_w;
    u64 error = ndsRendererHardwareAbsDiffS64(lhs, rhs);
    u64 scale = ndsRendererHardwareAbsS64(cpu_axis) +
                ndsRendererHardwareAbsS64(cpu_w);
    u64 normalized;

    if (scale == 0u)
    {
        scale = 1u;
    }
    normalized = (error / scale) + ((error % scale) != 0u);
    return (normalized > UINT_MAX) ? UINT_MAX : (u32)normalized;
}

static u32 ndsRendererHardwarePosTestInside(s32 axis, s32 w)
{
    return (ndsRendererHardwareAbsS64(axis) <=
            ndsRendererHardwareAbsS64(w)) ? TRUE : FALSE;
}

static void ndsRendererHardwareQueueRawMatrixPosTestValues(
    const NDSRendererMatrix20p12 *matrix, u32 generation,
    const NDSRendererInputVertex *input,
    const NDSRendererClipVertex20p12 *clip, u32 matrix_word)
{
    NDSRendererHardwarePendingPosTest *probe;

    if ((matrix == NULL) || (input == NULL) || (clip == NULL) ||
        (generation == 0u) ||
        (generation == sNdsRendererHardwarePendingPosTestLastGeneration))
    {
        return;
    }
    sNdsRendererHardwarePendingPosTestLastGeneration =
        generation;
    if (sNdsRendererHardwarePendingPosTestCount >=
        NDS_RENDERER_HW_POS_TEST_MAX)
    {
        gNdsRendererProfileMatrixPosTestDropped++;
        return;
    }

    probe = &sNdsRendererHardwarePendingPosTests[
        sNdsRendererHardwarePendingPosTestCount++];
    probe->matrix = *matrix;
    probe->input = *input;
    probe->clip = *clip;
    probe->generation = generation;
    probe->matrix_word = matrix_word;
}

static void ndsRendererHardwareQueueRawMatrixPosTest(
    const NDSRendererTraversalState *state, u32 index)
{
    if ((state == NULL) || (index >= NDS_RENDERER_MAX_VTX) ||
        (state->matrix_valid == 0u))
    {
        return;
    }
    ndsRendererHardwareQueueRawMatrixPosTestValues(
        &state->matrix, state->matrix_generation,
        &state->input_vertices[index], &state->vertices[index],
        state->matrix_word_valid);
}

static void ndsRendererHardwareQueueSnapshotMatrixPosTest(
    const NDSRendererTraversalState *state, u32 snapshot_id, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot =
        ndsRendererGetMatrixSnapshot(state, snapshot_id);

    if ((snapshot == NULL) || (index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    ndsRendererHardwareQueueRawMatrixPosTestValues(
        &snapshot->matrix, snapshot->generation,
        &state->input_vertices[index], &state->vertices[index], FALSE);
}

static void ndsRendererHardwareQueueMatrixWordPosTestFixture(void)
{
    NDSRendererHardwarePendingPosTest *base;
    NDSRendererHardwarePendingPosTest *probe;
    NDSRendererTraversalState state;
    NDSRendererStats stats;
    NDSRendererMatrix20p12 target_matrix;
    Mtx target_raw;
    const u32 *target_words;
    u32 *current_words;
    u32 i;

    if (sNdsRendererHardwarePendingPosTestCount == 0u)
    {
        return;
    }
    for (i = 0u; i < sNdsRendererHardwarePendingPosTestCount; i++)
    {
        if (sNdsRendererHardwarePendingPosTests[i].matrix_word != 0u)
        {
            return;
        }
    }

    /*
     * The current Pupupu frame does not naturally issue G_MW_MATRIX. Derive
     * one backend-only fixture from its first eligible matrix so profile 2
     * still proves the exact MVP-recalc + matrix-word reconstruction used by
     * BattleShip. This runs after the submitted triangle batch has closed and
     * cannot alter production geometry.
     */
    base = &sNdsRendererHardwarePendingPosTests[0];
    memset(&state, 0, sizeof(state));
    memset(&stats, 0, sizeof(stats));
    state.matrix = base->matrix;
    state.matrix_valid = TRUE;
    ndsRendererApplyMvpRecalcCommand(&stats, &state, 1u, 0u);

    target_matrix = state.matrix;
    if (target_matrix.m[3][0] <=
        (INT_MAX - NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA))
    {
        target_matrix.m[3][0] +=
            NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA;
    }
    else
    {
        target_matrix.m[3][0] -=
            NDS_RENDERER_HW_POS_TEST_MATRIX_WORD_DELTA;
    }
    ndsRendererMtxStoreDS20p12ToN64(&target_matrix, &target_raw);
    target_words = (const u32 *)&target_raw.m[0][0];
    current_words = (u32 *)&state.matrix_word_raw.m[0][0];
    for (i = 0u; i < NDS_RENDERER_MATRIX_WORD_COUNT; i++)
    {
        if (current_words[i] != target_words[i])
        {
            ndsRendererApplyMatrixMoveWordCommand(
                &stats, &state,
                (i * NDS_RENDERER_MATRIX_WORD_BYTES) <<
                    NDS_RENDERER_MOVEWORD_OFFSET_SHIFT,
                target_words[i]);
            current_words = (u32 *)&state.matrix_word_raw.m[0][0];
        }
    }
    if ((stats.matrix_mvp_recalc_count != 1u) ||
        (stats.matrix_move_word_count == 0u))
    {
        gNdsRendererProfileMatrixPosTestDropped++;
        return;
    }

    if (sNdsRendererHardwarePendingPosTestCount <
        NDS_RENDERER_HW_POS_TEST_MAX)
    {
        probe = &sNdsRendererHardwarePendingPosTests[
            sNdsRendererHardwarePendingPosTestCount++];
    }
    else
    {
        probe = &sNdsRendererHardwarePendingPosTests[
            NDS_RENDERER_HW_POS_TEST_MAX - 1u];
    }
    probe->matrix = state.matrix;
    probe->input = base->input;
    ndsRendererTransformVertex20p12(&probe->matrix, &probe->input,
                                    &probe->clip);
    probe->generation = state.matrix_generation;
    probe->matrix_word = TRUE;
}

static void ndsRendererHardwareRunRawMatrixPosTests(void)
{
    u32 i;

    ndsRendererHardwareQueueMatrixWordPosTestFixture();

    for (i = 0u; i < sNdsRendererHardwarePendingPosTestCount; i++)
    {
        const NDSRendererHardwarePendingPosTest *probe =
            &sNdsRendererHardwarePendingPosTests[i];
        v16 x = ndsRendererHardwareVertexCoord(probe->input.x, TRUE);
        v16 y = ndsRendererHardwareVertexCoord(probe->input.y, TRUE);
        v16 z = ndsRendererHardwareVertexCoord(probe->input.z, TRUE);
        s32 hardware_x;
        s32 hardware_y;
        s32 hardware_z;
        s32 hardware_w;
        u32 error_x;
        u32 error_y;
        u32 error_z;
        u32 max_error;
        u32 w_sign_mismatch;
        u32 clip_mismatch;

        ndsRendererLoadHardwareRawComposedMatrix(
            &probe->matrix, probe->generation);
        PosTest(x, y, z);
        hardware_x = PosTestXresult();
        hardware_y = PosTestYresult();
        hardware_z = PosTestZresult();
        hardware_w = PosTestWresult();
        error_x = ndsRendererHardwarePosTestCrossError(
            hardware_x, hardware_w, probe->clip.x, probe->clip.w);
        error_y = ndsRendererHardwarePosTestCrossError(
            hardware_y, hardware_w, probe->clip.y, probe->clip.w);
        error_z = ndsRendererHardwarePosTestCrossError(
            hardware_z, hardware_w, probe->clip.z, probe->clip.w);
        max_error = error_x;
        if (error_y > max_error) { max_error = error_y; }
        if (error_z > max_error) { max_error = error_z; }
        w_sign_mismatch = (((hardware_w < 0) != (probe->clip.w < 0)) ||
                           ((hardware_w == 0) != (probe->clip.w == 0))) ?
                              TRUE : FALSE;
        clip_mismatch =
            ((ndsRendererHardwarePosTestInside(hardware_x, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.x,
                                               probe->clip.w)) ||
             (ndsRendererHardwarePosTestInside(hardware_y, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.y,
                                               probe->clip.w)) ||
             (ndsRendererHardwarePosTestInside(hardware_z, hardware_w) !=
              ndsRendererHardwarePosTestInside(probe->clip.z,
                                               probe->clip.w))) ? TRUE : FALSE;

        gNdsRendererProfileMatrixPosTestSamples++;
        if (probe->matrix_word != 0u)
        {
            gNdsRendererProfileMatrixPosTestMatrixWordSamples++;
        }
        if (max_error > gNdsRendererProfileMatrixPosTestMaxError)
        {
            gNdsRendererProfileMatrixPosTestMaxError = max_error;
        }
        if (w_sign_mismatch != FALSE)
        {
            gNdsRendererProfileMatrixPosTestWSignMismatches++;
        }
        if (clip_mismatch != FALSE)
        {
            gNdsRendererProfileMatrixPosTestClipMismatches++;
        }
        if ((max_error > NDS_RENDERER_HW_POS_TEST_TOLERANCE) ||
            (w_sign_mismatch != FALSE) || (clip_mismatch != FALSE))
        {
            gNdsRendererProfileMatrixPosTestMismatches++;
        }
    }
    sNdsRendererHardwarePendingPosTestCount = 0u;
    sNdsRendererHardwarePendingPosTestLastGeneration = 0u;
}
#endif

static s32 ndsRendererHardwareNextProjectedDepth(void)
{
    sNdsRendererHardwareProjectedDepth--;
    return sNdsRendererHardwareProjectedDepth /
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
}

static void ndsRendererHardwareEnterProjectedForeground(void)
{
    if (sNdsRendererHardwareProjectedBackground == FALSE)
    {
        return;
    }

    /* The DS cannot disable depth testing per polygon. Mirror sm64-nds'
     * source G_ZBUFFER transition: early no-Z background draws count down
     * from the far endpoint, then the first source-Z triangle moves later
     * no-Z painter passes in front of the source depth range. */
    sNdsRendererHardwareProjectedDepth =
        NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START;
    sNdsRendererHardwareProjectedBackground = FALSE;
}

static void ndsRendererHardwareClipVertex(
    const NDSRendererClipVertex20p12 *vtx, s32 z)
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }

    x = ndsRendererHardwareClampS64ToV16(
        ((s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX) / vtx->w);
    y = ndsRendererHardwareClampS64ToV16(
        ((s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX) / vtx->w);
    out_z = ndsRendererHardwareClampS64ToV16(
        ((s64)z * NDS_RENDERER_HW_PROJECTED_VERTEX) / vtx->w);
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void ndsRendererHardwareClipVertexNdcDepth(
    const NDSRendererClipVertex20p12 *vtx, s32 z)
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }
    x = ndsRendererHardwareClampS64ToV16(
        ((s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX) / vtx->w);
    y = ndsRendererHardwareClampS64ToV16(
        ((s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX) / vtx->w);
    out_z = ndsRendererHardwareClampS64ToV16(z);
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void ndsRendererHardwareEndBatch(void)
{
    if (sNdsRendererHardwareTriangleBatchOpen != 0u)
    {
        /* libnds documents glEnd() as a dummy FIFO write. A later glBegin()
         * starts the next primitive group, so only restore state owned by the
         * logical source-triangle batch here. */
        glDisable(GL_ALPHA_TEST);
        ndsRendererProfileRecordBatchEnd();
        sNdsRendererHardwareTriangleBatchOpen = FALSE;
        sNdsRendererHardwareTriangleBatchTextured = FALSE;
        sNdsRendererHardwareTriangleBatchTextureName = 0u;
        sNdsRendererHardwareTriangleBatchPolyFmt = 0u;
        sNdsRendererHardwareTriangleBatchAlphaKey = 0u;
        sNdsRendererHardwareTriangleBatchFogKey = 0u;
        sNdsRendererHardwareTriangleBatchMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_NONE;
        sNdsRendererHardwareTriangleBatchMatrixGeneration = 0u;
    }
}

static void ndsRendererHardwareBeginTriangleBatch(
    const NDSRendererStats *stats,
    u32 textured,
    u32 texture_name,
    u32 poly_fmt,
    u32 matrix_mode,
    u32 matrix_generation)
{
    u32 alpha_key;
    u32 fog_key;

    /* An open GX batch can only contain adjacent TRI1/TRI2 source commands.
     * Every opcode capable of changing alpha, fog, texture, or matrix state
     * closes it in ndsRendererScanList before mutation. Keep the per-triangle
     * reuse check to the values that can differ through vertex selection. */
    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured == textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode == matrix_mode) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    fog_key = ndsRendererHardwareFogStateKey(stats);
    ndsRendererHardwareEndBatch();
    if (textured != 0u)
    {
        glEnable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ndsRendererHardwareBindNoTexture(NULL);
    }
    ndsRendererHardwareApplyAlphaTest(stats);
    ndsRendererHardwareApplyFog(stats);
    glPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = alpha_key;
    sNdsRendererHardwareTriangleBatchFogKey = fog_key;
    sNdsRendererHardwareTriangleBatchMatrixMode = matrix_mode;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

static void NDS_RENDERER_HOT_CODE
ndsRendererHardwareSubmitVertex(
    const NDSRendererHardwareVertexContext *context,
    u32 vertex_index,
    s32 projected_z)
{
    NDSRendererStats *stats;
    NDSRendererTraversalState *state;
    const NDSRendererInputVertex *vtx;
    const NDSRendererClipVertex20p12 *clip_vtx;
    const NDSRendererTileState *render_tile;
    u32 material_color;
    u32 scale_s;
    u32 scale_t;
    u32 texture_origin_s;
    u32 texture_origin_t;
    u32 context_flags;
    u32 scale_world;
    u32 vertex_color;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 vertex_color_valid;
    s32 use_texture;
    s32 texture_offset;
    s32 zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 source_clip_depth;
    u16 hardware_color;

    if (context == NULL)
    {
        return;
    }
    stats = context->stats;
    state = context->state;
    if ((state == NULL) || (vertex_index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    vtx = &state->input_vertices[vertex_index];
    clip_vtx = &state->vertices[vertex_index];
    render_tile = context->render_tile;
    material_color = context->material_color;
    scale_s = context->scale_s;
    scale_t = context->scale_t;
    texture_origin_s = context->texture_origin_s;
    texture_origin_t = context->texture_origin_t;
    context_flags = context->flags;
    scale_world = context_flags & NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD;
    vertex_color = state->vertex_colors[vertex_index];
    use_material_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    use_vertex_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX;
    vertex_color_valid =
        ((state->vertex_color_valid_mask & (1u << vertex_index)) != 0u) ?
            TRUE : FALSE;
    use_texture = context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    texture_offset = context->texture_offset;
    zbuffered = context_flags & NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    decal_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH;
    prim_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH;
    source_clip_depth =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((state != NULL) &&
        (vertex_index < NDS_RENDERER_MAX_VTX) &&
        ((state->prepared_vertex_color_valid_mask &
          (1u << vertex_index)) != 0u))
    {
        hardware_color = state->prepared_vertex_colors[vertex_index];
    }
    else
#endif
    {
        hardware_color = ndsRendererHardwarePackedVertexColor(
            stats, vtx, material_color, use_material_color,
            use_vertex_color, vertex_color, vertex_color_valid);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
        {
            state->prepared_vertex_colors[vertex_index] = hardware_color;
            state->prepared_vertex_color_valid_mask |= 1u << vertex_index;
        }
#endif
    }
    glColor(hardware_color);
    if ((use_texture != FALSE) && (render_tile != NULL))
    {
        s16 s;
        s16 t;

#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            ((state->prepared_texcoord_valid_mask &
              (1u << vertex_index)) != 0u))
        {
            s = state->prepared_texcoord_s[vertex_index];
            t = state->prepared_texcoord_t[vertex_index];
        }
        else
#endif
        {
            s = ndsRendererHardwareTexCoord(vtx->s, scale_s,
                                            texture_origin_s,
                                            texture_offset);
            t = ndsRendererHardwareTexCoord(vtx->t, scale_t,
                                            texture_origin_t,
                                            texture_offset);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
            {
                state->prepared_texcoord_s[vertex_index] = s;
                state->prepared_texcoord_t[vertex_index] = t;
                state->prepared_texcoord_valid_mask |= 1u << vertex_index;
            }
#endif
        }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererProfileTextureCoord(s, t);
        ndsRendererProfileTextureSample(s, t);
#endif
        glTexCoord2t16(s, t);
    }
    if ((zbuffered != FALSE) &&
        (decal_depth == FALSE) &&
        (prim_depth == FALSE))
    {
        v16 x = ndsRendererHardwareVertexCoord(vtx->x, scale_world);
        v16 y = ndsRendererHardwareVertexCoord(vtx->y, scale_world);
        v16 z = ndsRendererHardwareVertexCoord(vtx->z, scale_world);

        ndsRendererProfileVertexRange(vtx, x, y, z);
        glVertex3v16(x, y, z);
    }
    else if (prim_depth != FALSE)
    {
        ndsRendererHardwareClipVertex(clip_vtx, projected_z);
    }
    else if (decal_depth != FALSE)
    {
        if (clip_vtx != NULL)
        {
            ndsRendererHardwareClipVertex(
                clip_vtx, clip_vtx->z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS);
        }
    }
    else
    {
        /* X, Y, Z, and W must come from the same composed matrix snapshot.
         * Matrix-word updates can make projection/modelview fields stale. */
        if ((source_clip_depth != FALSE) && (clip_vtx != NULL) &&
            (clip_vtx->w != 0))
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            s32 depth = ndsRendererHardwareClampS64ToV16(
                ((s64)clip_vtx->z * NDS_RENDERER_HW_PROJECTED_VERTEX) /
                clip_vtx->w);

            if (stats->hardware_projected_depth_sample_count == 0u)
            {
                stats->hardware_projected_depth_min = depth;
                stats->hardware_projected_depth_max = depth;
                stats->hardware_projected_w_min = clip_vtx->w;
                stats->hardware_projected_w_max = clip_vtx->w;
            }
            else
            {
                if (depth < stats->hardware_projected_depth_min)
                {
                    stats->hardware_projected_depth_min = depth;
                }
                if (depth > stats->hardware_projected_depth_max)
                {
                    stats->hardware_projected_depth_max = depth;
                }
                if (clip_vtx->w < stats->hardware_projected_w_min)
                {
                    stats->hardware_projected_w_min = clip_vtx->w;
                }
                if (clip_vtx->w > stats->hardware_projected_w_max)
                {
                    stats->hardware_projected_w_max = clip_vtx->w;
                }
            }
            stats->hardware_projected_depth_sample_count++;
#endif
            projected_z = clip_vtx->z;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            (clip_vtx != NULL) && (clip_vtx->w != 0))
        {
            u32 vertex_mask = 1u << vertex_index;
            v16 x;
            v16 y;
            v16 z;

            if ((state->prepared_projected_xy_valid_mask & vertex_mask) !=
                0u)
            {
                x = state->prepared_projected_x[vertex_index];
                y = state->prepared_projected_y[vertex_index];
            }
            else
            {
                x = ndsRendererHardwareClampS64ToV16(
                    ((s64)clip_vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX) /
                    clip_vtx->w);
                y = ndsRendererHardwareClampS64ToV16(
                    ((s64)clip_vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX) /
                    clip_vtx->w);
                state->prepared_projected_x[vertex_index] = x;
                state->prepared_projected_y[vertex_index] = y;
                state->prepared_projected_xy_valid_mask |= vertex_mask;
            }
            if (source_clip_depth != FALSE)
            {
                if ((state->prepared_projected_source_z_valid_mask &
                     vertex_mask) != 0u)
                {
                    z = state->prepared_projected_source_z[vertex_index];
                }
                else
                {
                    z = ndsRendererHardwareClampS64ToV16(
                        ((s64)projected_z *
                         NDS_RENDERER_HW_PROJECTED_VERTEX) / clip_vtx->w);
                    state->prepared_projected_source_z[vertex_index] = z;
                    state->prepared_projected_source_z_valid_mask |=
                        vertex_mask;
                }
            }
            else
            {
                z = ndsRendererHardwareClampS64ToV16(projected_z);
            }
            glVertex3v16(x, y, z);
        }
        else if (source_clip_depth != FALSE)
        {
            ndsRendererHardwareClipVertex(clip_vtx, projected_z);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(clip_vtx, projected_z);
        }
#else
        if (source_clip_depth != FALSE)
        {
            ndsRendererHardwareClipVertex(clip_vtx, projected_z);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(clip_vtx, projected_z);
        }
#endif
    }
}

static s32 ndsRendererInputTriangleReady(
    const NDSRendererTraversalState *state, u32 packed,
    u32 *out_i0, u32 *out_i1, u32 *out_i2)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if (out_i0 != NULL) { *out_i0 = i0; }
    if (out_i1 != NULL) { *out_i1 = i1; }
    if (out_i2 != NULL) { *out_i2 = i2; }

    if ((state == NULL) ||
        (i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->input_vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareRawMatrixCompatible(
    const NDSRendererTraversalState *state)
{
    return ((state != NULL) && (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u)) ? TRUE : FALSE;
}

static NDSRendererHWSubmitClass ndsRendererHardwareClassifySubmit(
    const NDSRendererTraversalState *state,
    u32 i0, u32 i1, u32 i2,
    s32 source_zbuffered, s32 decal_depth, s32 prim_depth,
    u32 *out_snapshot_id)
{
    u32 mask;
    u32 snapshot_id;

    if (out_snapshot_id != NULL)
    {
        *out_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }

    if (source_zbuffered == FALSE)
    {
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z;
    }
    if (decal_depth != FALSE)
    {
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL;
    }
    if (prim_depth != FALSE)
    {
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH;
    }
    mask = (1u << i0) | (1u << i1) | (1u << i2);
    if ((state->raw_vertex_fit_mask & mask) != mask)
    {
        ndsRendererProfileRecordRawCurrentRangeReject();
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
    }

    if ((state->current_transform_vertex_mask & mask) == mask)
    {
        if (ndsRendererHardwareRawMatrixCompatible(state) == FALSE)
        {
            return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
        }
        ndsRendererProfileRecordRawCurrentCandidate();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererHardwareQueueRawMatrixPosTest(state, i0);
#endif
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX;
    }

    snapshot_id = state->vertex_matrix_snapshot[i0];
    if ((snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID) &&
        (state->vertex_matrix_snapshot[i1] == snapshot_id) &&
        (state->vertex_matrix_snapshot[i2] == snapshot_id) &&
        (ndsRendererGetMatrixSnapshot(state, snapshot_id) != NULL))
    {
        if (out_snapshot_id != NULL)
        {
            *out_snapshot_id = snapshot_id;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsRendererHardwareQueueSnapshotMatrixPosTest(
            state, snapshot_id, i0);
#endif
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX;
    }

    ndsRendererProfileRecordRawCrossMatrix();
    return NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX;
}

static void NDS_RENDERER_HOT_CODE
ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    const NDSRendererInputVertex *v0;
    const NDSRendererTileState *render_tile;
    s32 use_texture;
    s32 implicit_texture_on;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 scale_world;
    u32 material_color;
    u32 poly_alpha;
    u32 poly_fmt;
    u32 texture_name;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 texture_offset;
    s32 zbuffered;
    s32 source_zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 transformed_ready;
    s32 projected_submit;
    s32 raw_submit;
    s32 source_clip_depth;
    s32 no_oracle;
    u32 raw_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    const NDSRendererMatrixSnapshot *raw_snapshot = NULL;
    NDSRendererHWSubmitClass submit_class;
    NDSRendererHardwareVertexContext vertex_context;
    s32 projected_z[3] = { 0, 0, 0 };
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 vertex_submit_start;
#endif

    if (stats == NULL)
    {
        return;
    }
    if (ndsRendererInputTriangleReady(state, packed, &i0, &i1, &i2) == FALSE)
    {
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
    no_oracle = (sNdsRendererHardwareNoOracle != 0u) ? TRUE : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid != 0u)
    {
        source_zbuffered =
            (state->texture_prepare_source_zbuffered != 0u) ? TRUE : FALSE;
        zbuffered = source_zbuffered;
        decal_depth =
            (state->texture_prepare_decal_depth != 0u) ? TRUE : FALSE;
        prim_depth =
            (state->texture_prepare_prim_depth != 0u) ? TRUE : FALSE;
    }
    else
#endif
    {
        zbuffered =
            ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) != 0u) ?
                TRUE : FALSE;
        source_zbuffered = zbuffered;
        decal_depth = ((zbuffered != FALSE) &&
                       ((stats->othermode_l & NDS_RENDERER_ZMODE_DEC) ==
                        NDS_RENDERER_ZMODE_DEC)) ? TRUE : FALSE;
        prim_depth = ((zbuffered != FALSE) &&
                      (ndsRendererHardwareUsePrimDepth(stats) != FALSE)) ?
            TRUE : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        state->texture_prepare_source_zbuffered =
            (source_zbuffered != FALSE) ? TRUE : FALSE;
        state->texture_prepare_decal_depth =
            (decal_depth != FALSE) ? TRUE : FALSE;
        state->texture_prepare_prim_depth =
            (prim_depth != FALSE) ? TRUE : FALSE;
#endif
    }
    v0 = &state->input_vertices[i0];
    submit_class = ndsRendererHardwareClassifySubmit(
        state, i0, i1, i2, source_zbuffered, decal_depth, prim_depth,
        &raw_snapshot_id);
    raw_submit =
        ((submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ||
         (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX)) ?
             TRUE : FALSE;
    projected_submit =
        ((source_zbuffered != FALSE) && (raw_submit == FALSE)) ? TRUE : FALSE;
    source_clip_depth = projected_submit;
    transformed_ready = TRUE;
    if (raw_submit == FALSE)
    {
        transformed_ready =
            ((ndsRendererEnsureTransformedVertex(stats, state, i0) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i1) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i2) != FALSE)) ?
                 TRUE : FALSE;
    }
    if (transformed_ready == FALSE)
    {
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
    if (no_oracle == FALSE)
    {
        stats->hardware_oracle_triangle_count++;
    }
    if (raw_snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
    {
        raw_snapshot = ndsRendererGetMatrixSnapshot(state, raw_snapshot_id);
    }
    render_tile = &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid != 0u)
    {
        material_color = state->texture_prepare_material_color;
        use_material_color =
            (state->texture_prepare_use_material_color != 0u) ? TRUE : FALSE;
        use_vertex_color =
            (state->texture_prepare_use_vertex_color != 0u) ? TRUE : FALSE;
    }
    else
#endif
    {
        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        state->texture_prepare_material_color = material_color;
        state->texture_prepare_use_material_color =
            (use_material_color != FALSE) ? TRUE : FALSE;
        state->texture_prepare_use_vertex_color =
            (use_vertex_color != FALSE) ? TRUE : FALSE;
#endif
    }
    if (stats->texture_combine_count != 0u)
    {
        if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileLitShadeCombineCount++;
#endif
        }
        if (use_material_color != FALSE)
        {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            gNdsRendererProfileMaterialCombineCount++;
#endif
        }
    }
    poly_alpha = ndsRendererHardwareAlpha(stats, v0);
    if (poly_alpha == 0u)
    {
        return;
    }
    ndsRendererProfileRecordSubmitClass(submit_class);
    if (projected_submit != FALSE)
    {
        ndsRendererProfileRecordProjectedSubmit();
    }
    if ((raw_submit == FALSE) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH))
    {
        zbuffered = FALSE;
        decal_depth = FALSE;
        prim_depth = FALSE;
    }
    if (submit_class == NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z)
    {
        ndsRendererProfileRecordProjectedDivisions(6u);
    }
    else if (raw_submit == FALSE)
    {
        ndsRendererProfileRecordProjectedDivisions(9u);
    }
    scale_world = (raw_submit != FALSE) ? TRUE : FALSE;
    if (state->texture_prepare_valid == 0u)
    {
        implicit_texture_on =
            ndsRendererHardwareTextureImplicitStateOn(stats);
        use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
            ndsRendererHardwareBindTexture(stats, config) : FALSE;
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled =
            (use_texture != FALSE) ? TRUE : FALSE;
        state->texture_prepare_implicit_on =
            (implicit_texture_on != FALSE) ? TRUE : FALSE;
        state->texture_prepare_name = (use_texture != FALSE) ?
            sNdsRendererHardwareBoundTextureName : 0u;
        ndsRendererProfileRecordTexturePrepare();

        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
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
        texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_offset = texture_offset;
    }
    else
    {
        implicit_texture_on =
            (state->texture_prepare_implicit_on != 0u) ? TRUE : FALSE;
        use_texture =
            (state->texture_prepare_enabled != 0u) ? TRUE : FALSE;
        ndsRendererProfileRecordTexturePrepareReuse();
        texture_scale_s = state->texture_prepare_scale_s;
        texture_scale_t = state->texture_prepare_scale_t;
        texture_offset = state->texture_prepare_offset;
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        use_material_color = FALSE;
        use_vertex_color = FALSE;
    }
#endif
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererLoadHardwareMatrices(state, scale_world);
    }
    else if (raw_snapshot != NULL)
    {
        ndsRendererLoadHardwareRawComposedMatrix(
            &raw_snapshot->matrix, raw_snapshot->generation);
    }
    else
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
    }
    if (prim_depth != FALSE)
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            (s32)(stats->prim_depth & 0xffffu);
    }
    else if (source_zbuffered != FALSE)
    {
        /* Source-Z projected submissions use the composed clip Z below and
         * must not consume the synthetic no-Z painter counter. */
        projected_z[0] = projected_z[1] = projected_z[2] = 0;
    }
    else
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            ndsRendererHardwareNextProjectedDepth();
    }
    poly_fmt = ndsRendererHardwarePolyFmt(stats, poly_alpha);
    texture_name = state->texture_prepare_name;
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        texture_name, poly_fmt, sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    vertex_context.stats = stats;
    vertex_context.state = state;
    vertex_context.render_tile = render_tile;
    vertex_context.material_color = material_color;
    vertex_context.scale_s = texture_scale_s;
    vertex_context.scale_t = texture_scale_t;
    vertex_context.texture_origin_s = render_tile->uls;
    vertex_context.texture_origin_t = render_tile->ult;
    vertex_context.flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u) |
        ((use_texture != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE : 0u) |
        ((scale_world != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD : 0u) |
        ((zbuffered != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED : 0u) |
        ((decal_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH : 0u) |
        ((prim_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH : 0u) |
        ((source_clip_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH : 0u);
    vertex_context.texture_offset = texture_offset;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    vertex_submit_start = cpuGetTiming();
#endif
    ndsRendererHardwareSubmitVertex(&vertex_context, i0, projected_z[0]);
    ndsRendererHardwareSubmitVertex(&vertex_context, i1, projected_z[1]);
    ndsRendererHardwareSubmitVertex(&vertex_context, i2, projected_z[2]);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsRendererProfileVertexSubmitTicks +=
        cpuGetTiming() - vertex_submit_start;
#endif
    if (source_zbuffered != FALSE)
    {
        ndsRendererHardwareEnterProjectedForeground();
    }

    sNdsRendererHardwareSubmitted = TRUE;
    stats->hardware_triangle_count++;
    stats->hardware_vertex_count += 3u;
    ndsRendererProfileRecordHardwareTriangle();
    if (zbuffered != FALSE)
    {
        stats->hardware_zbuffer_triangle_count++;
        if (decal_depth != FALSE)
        {
            stats->hardware_decal_depth_triangle_count++;
        }
        if (prim_depth != FALSE)
        {
            stats->hardware_prim_depth_triangle_count++;
        }
    }
    else
    {
        stats->hardware_projected_depth_triangle_count++;
    }
}
#endif

static inline void ndsRendererExecuteTriangleCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 triangle_submit_start;
#endif
#if !NDS_RENDERER_HW_TRIANGLES
    (void)config;
#endif

    stats->triangle_command_count++;
    stats->render_command_count++;
    if (op == NDS_RENDERER_OP_TRI1)
    {
        u32 packed = ndsGBIDecodeF3DEX2Tri1(w0);

        stats->triangle_count++;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererHardwareNoOracle == 0u)
#endif
        ndsRendererRecordTransformedTriangle(stats, state, packed);
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        triangle_submit_start = cpuGetTiming();
#endif
        ndsRendererSubmitHardwareTriangle(stats, config, state, packed);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        sNdsRendererProfileTriangleSubmitTicks +=
            cpuGetTiming() - triangle_submit_start;
#endif
#endif
        return;
    }

    stats->triangle_count += 2u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererHardwareNoOracle == 0u)
    {
#endif
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2First(w0));
        ndsRendererRecordTransformedTriangle(
            stats, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_HW_TRIANGLES
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    triangle_submit_start = cpuGetTiming();
#endif
    ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2First(w0));
    ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsRendererProfileTriangleSubmitTicks +=
        cpuGetTiming() - triangle_submit_start;
#endif
#endif
}

static void NDS_RENDERER_HOT_CODE
ndsRendererScanList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats,
                                NDSRendererTraversalState *state,
                                u32 depth,
                                NDSRendererCommandCallback callback,
                                void *callback_user)
{
    u32 i;
    u32 immutable_command_count = 0u;

    if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
    {
        return;
    }
    if (depth > config->max_depth)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_TOO_DEEP;
        return;
    }
    if (ndsRendererValidateCommand(dl, config) == FALSE)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }
    if (depth > stats->max_depth_seen)
    {
        stats->max_depth_seen = depth;
    }
    if (config->immutable_command_span != NULL)
    {
        size_t immutable_bytes =
            config->immutable_command_span(dl, config->user);
        size_t immutable_count = immutable_bytes / sizeof(*dl);

        if (immutable_count > config->max_list_commands)
        {
            immutable_count = config->max_list_commands;
        }
        immutable_command_count = (u32)immutable_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        if (immutable_command_count != 0u)
        {
            sNdsRendererProfileImmutableListCount++;
        }
#endif
    }

    for (i = 0; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;
        NDSRendererCommand command;

        if ((i >= immutable_command_count) &&
            (ndsRendererValidateCommand(dl, config) == FALSE))
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        if (i < immutable_command_count)
        {
            sNdsRendererProfileTrustedCommandCount++;
        }
#endif
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            return;
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
#if NDS_RENDERER_HW_TRIANGLES
        /* Preserve the source-command boundary: only adjacent TRI1/TRI2
         * opcodes may share a GX triangle group. In particular, close before
         * VTX/MODIFYVTX mutate the cached vertices and before any matrix,
         * texture, state, branch, sync, or ENDDL command. */
        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            ndsRendererHardwareEndBatch();
            /* VTX and matrix commands end the GX primitive group but cannot
             * change the prepared texture/material/depth epoch. The exact
             * state opcodes below invalidate that epoch at their mutation. */
            state->prepared_vertex_color_valid_mask = 0u;
            state->prepared_texcoord_valid_mask = 0u;
            state->prepared_projected_xy_valid_mask = 0u;
            state->prepared_projected_source_z_valid_mask = 0u;
        }
#endif
        if ((callback != NULL) || (op == NDS_RENDERER_OP_DL))
        {
            memset(&command, 0, sizeof(command));
            command.dl = dl;
            command.w0 = w0;
            command.w1 = w1;
            command.op = op;
            command.depth = depth;
            command.list_index = i;
            command.transformed_vertices = state->vertices;
            command.transformed_vertex_valid_mask = state->vertex_valid_mask;
            command.matrix_valid = state->matrix_valid;

            if (op == NDS_RENDERER_OP_DL)
            {
                command.raw_branch_dl = (const Gfx *)(uintptr_t)w1;
                command.resolved_branch_dl = command.raw_branch_dl;
                if (config->resolve_branch != NULL)
                {
                    command.resolved_branch_dl = config->resolve_branch(
                        command.raw_branch_dl,
                        &command.branch_resolve_kind,
                        config->user);
                }
                command.branch_is_jump =
                    ((w0 & (1u << 16)) != 0) ? TRUE : FALSE;
            }
        }

        if (stats->first_opcode == 0)
        {
            stats->first_opcode = op;
        }
        stats->command_count++;

        if ((callback != NULL) &&
            (callback(&command, callback_user) == FALSE))
        {
            ndsRendererRecordUnsupported(stats, op);
            stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
            return;
        }

        switch (op)
        {
        case NDS_RENDERER_OP_NOOP:
            stats->skip_command_count++;
            break;

        case NDS_RENDERER_OP_MODIFYVTX:
            ndsRendererApplyModifyVertexCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_VTX:
            ndsRendererApplyVertexCommand(config, stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_TRI1:
        case NDS_RENDERER_OP_TRI2:
        {
            ndsRendererExecuteTriangleCommand(
                stats, config, state, op, w0, w1);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
            /* Immutable adjacent TRI commands have no intervening source
             * state transition. Profile 0/1 has no command callback, so
             * replay the remainder of the run without rebuilding a generic
             * command record or re-entering the full opcode switch. */
            while ((callback == NULL) &&
                   ((i + 1u) < immutable_command_count) &&
                   ((i + 1u) < config->max_list_commands))
            {
                const Gfx *next_dl = dl + 1;
                u32 next_w0 = next_dl->words.w0;
                u32 next_op = next_w0 >> 24;

                if ((next_op != NDS_RENDERER_OP_TRI1) &&
                    (next_op != NDS_RENDERER_OP_TRI2))
                {
                    break;
                }
                if (stats->command_count >= config->max_commands)
                {
                    stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
                    return;
                }
                i++;
                dl = next_dl;
                stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                sNdsRendererProfileTrustedCommandCount++;
                sNdsRendererProfileTriangleRunReuseCount++;
#endif
                ndsRendererExecuteTriangleCommand(
                    stats, config, state, next_op, next_w0,
                    next_dl->words.w1);
            }
#endif
            break;
        }

        case NDS_RENDERER_OP_ENDDL:
            stats->end_command_count++;
            stats->skip_command_count++;
            return;

        case NDS_RENDERER_OP_DL:
        {
            const Gfx *raw_branch = command.raw_branch_dl;
            const Gfx *branch = command.resolved_branch_dl;

            stats->branch_command_count++;
            if (stats->first_branch_dl == NULL)
            {
                stats->first_branch_dl = raw_branch;
            }
            if (stats->first_resolved_branch_dl == NULL)
            {
                stats->first_resolved_branch_dl = branch;
            }
            if (command.branch_resolve_kind == NDS_RENDERER_RESOLVE_SEGMENT)
            {
                stats->segment_resolve_count++;
            }
            if (ndsRendererValidateCommand(branch, config) == FALSE)
            {
                stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
                return;
            }
            if ((w0 & (1u << 16)) != 0)
            {
                stats->branch_jump_count++;
                ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                    callback, callback_user);
                return;
            }

            stats->branch_call_count++;
            ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                callback, callback_user);
            if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                return;
            }
            break;
        }

        case NDS_RENDERER_OP_TEXTURE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordTextureState(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_MTX:
            ndsRendererApplyMatrixCommand(config, stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_POPMTX:
            ndsRendererApplyPopMatrixCommand(stats, state, w1);
            break;

        case NDS_RENDERER_OP_MOVEWORD:
            ndsRendererApplyMatrixMoveWordCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_MOVEMEM:
            ndsRendererRecordLightMoveMem(config, stats, w0, w1);
            break;

        case NDS_RENDERER_OP_SPECIAL_1:
            ndsRendererApplyMvpRecalcCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_SETSCISSOR:
        case NDS_RENDERER_OP_SETCIMG:
            stats->state_command_count++;
            stats->ignored_state_command_count++;
            break;

        case NDS_RENDERER_OP_SETPRIMDEPTH:
            ndsRendererRecordPrimDepth(stats, w1);
            break;

        case NDS_RENDERER_OP_GEOMETRYMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            stats->geometry_mode = (stats->geometry_mode & w0) | w1;
            stats->geometry_clear_mask = w0;
            stats->geometry_set_mask = w1;
            stats->geometry_command_count++;
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETCOMBINE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordSetCombine(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTIMG:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordSetImage(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordSetTile(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_LOADTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordLoadTile(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_LOADBLOCK:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordLoadBlock(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_LOADTLUT:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordLoadTlut(stats, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTILESIZE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordSetTileSize(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETFOGCOLOR:
            ndsRendererRecordFogColor(stats, w1);
            stats->state_command_count++;
            stats->color_command_count++;
            break;

        case NDS_RENDERER_OP_SETBLENDCOLOR:
        case NDS_RENDERER_OP_SETENVCOLOR:
        case NDS_RENDERER_OP_SETPRIMCOLOR:
            if (op != NDS_RENDERER_OP_SETBLENDCOLOR)
            {
                NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            }
            stats->state_command_count++;
            stats->color_command_count++;
            if (op == NDS_RENDERER_OP_SETPRIMCOLOR)
            {
                stats->prim_color = w1;
                stats->prim_min_level = (w0 >> 8) & 0xffu;
                stats->prim_lod_fraction = w0 & 0xffu;
            }
            else if (op == NDS_RENDERER_OP_SETENVCOLOR)
            {
                stats->env_color = w1;
            }
            else
            {
                stats->blend_color = w1;
            }
            break;

        case NDS_RENDERER_OP_RDPPIPESYNC:
        case NDS_RENDERER_OP_RDPLOADSYNC:
        case NDS_RENDERER_OP_RDPTILESYNC:
        case NDS_RENDERER_OP_RDPFULLSYNC:
            stats->skip_command_count++;
            stats->sync_command_count++;
            break;

        case NDS_RENDERER_OP_SETOTHERMODE_H:
        case NDS_RENDERER_OP_SETOTHERMODE_L:
        case NDS_RENDERER_OP_RDPSETOTHERMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererRecordOtherMode(stats, op, w0, w1);
            break;

        case NDS_RENDERER_OP_CULLDL:
            ndsRendererRecordCull(stats, w0, w1);
            break;

        default:
            ndsRendererRecordUnsupported(stats, op);
            break;
        }
    }
}

void ndsRendererInitStats(NDSRendererStats *stats)
{
    if (stats != NULL)
    {
        memset(stats, 0, sizeof(*stats));
        stats->geometry_mode = NDS_RENDERER_GEOM_RESET_MODE;
        stats->othermode_h = NDS_RENDERER_TP_PERSP | NDS_RENDERER_TF_BILERP;
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

    ndsRendererInitTraversalState(&state, config, stats,
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

void ndsRendererProfileFrameBegin(void)
{
    gNdsRendererProfileLevel = NDS_RENDERER_PROFILE_LEVEL;
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
    sNdsRendererProfileImmutableListCount = 0u;
    sNdsRendererProfileTrustedCommandCount = 0u;
    sNdsRendererProfileValidatedCommandCount = 0u;
    sNdsRendererProfileTriangleRunReuseCount = 0u;
    sNdsRendererProfileTriangleSubmitTicks = 0u;
    sNdsRendererProfileVertexSubmitTicks = 0u;
    sNdsRendererProfileCi4LutBuildCount = 0u;
    sNdsRendererProfileCi4LutReuseCount = 0u;
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileResetSubmitSummary();
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    memset(&sNdsRendererRuntimeFrameSummary, 0,
           sizeof(sNdsRendererRuntimeFrameSummary));
    if (gNdsRendererProfileFrameCount == 1u)
    {
        sNdsRendererRuntimeTexel1FractionRefreshCount = 0u;
        sNdsRendererRuntimeTextureCacheEvictCount = 0u;
        sNdsRendererRuntimeTextureCi4DirectPixels = 0u;
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    sNdsRendererHardwarePendingPosTestCount = 0u;
    sNdsRendererHardwarePendingPosTestLastGeneration = 0u;
#endif
}

void ndsRendererProfileFramePublish(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
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
    gNdsRendererProfileTextureCacheAliasAvoidCount =
        sNdsRendererRuntimeFrameSummary.texture_cache_alias_avoid_count;
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
    u32 submitted = sNdsRendererHardwareSubmitted;

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
    sNdsRendererHardwareProjectedDepth =
        NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START;
    sNdsRendererHardwareProjectedBackground = TRUE;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    sNdsRendererHardwareBoundTextureName = 0;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererHardwareFrameSerial++;
    return submitted;
#else
    return FALSE;
#endif
}

void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl,
    const NDSRendererConfig *config,
    NDSRendererCommandCallback callback,
    void *callback_user,
    NDSRendererStats *stats,
    NDSRendererVertexCache *vertex_cache)
{
    NDSRendererTraversalState state;
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
    ndsRendererInitTraversalState(&state, config, stats, matrix_snapshots,
                                  matrix_snapshot_count);
    if (vertex_cache != NULL)
    {
        memcpy(state.vertices, vertex_cache->transformed_vertices,
               sizeof(state.vertices));
        state.vertex_valid_mask = vertex_cache->transformed_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        memcpy(state.input_vertices, vertex_cache->input_vertices,
               sizeof(state.input_vertices));
        state.input_vertex_valid_mask = vertex_cache->input_valid_mask;
        state.raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask;
        memcpy(state.vertex_colors, vertex_cache->vertex_colors,
               sizeof(state.vertex_colors));
        state.vertex_color_valid_mask =
            vertex_cache->vertex_color_valid_mask;
        memcpy(state.vertex_matrix_snapshot,
               vertex_cache->vertex_matrix_snapshot,
               sizeof(state.vertex_matrix_snapshot));
        memcpy(state.vertex_clip_snapshot,
               vertex_cache->vertex_clip_snapshot,
               sizeof(state.vertex_clip_snapshot));
#endif
    }
    ndsRendererScanList(dl, config, stats, &state, 0, callback,
                        callback_user);
    if (vertex_cache != NULL)
    {
        memcpy(vertex_cache->transformed_vertices, state.vertices,
               sizeof(vertex_cache->transformed_vertices));
        vertex_cache->transformed_valid_mask = state.vertex_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
        memcpy(vertex_cache->input_vertices, state.input_vertices,
               sizeof(vertex_cache->input_vertices));
        vertex_cache->input_valid_mask = state.input_vertex_valid_mask;
        vertex_cache->raw_vertex_fit_mask = state.raw_vertex_fit_mask;
        memcpy(vertex_cache->vertex_colors, state.vertex_colors,
               sizeof(vertex_cache->vertex_colors));
        vertex_cache->vertex_color_valid_mask =
            state.vertex_color_valid_mask;
        memcpy(vertex_cache->vertex_matrix_snapshot,
               state.vertex_matrix_snapshot,
               sizeof(vertex_cache->vertex_matrix_snapshot));
        memcpy(vertex_cache->vertex_clip_snapshot,
               state.vertex_clip_snapshot,
               sizeof(vertex_cache->vertex_clip_snapshot));
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
    ndsRendererExecuteDisplayListWithVertexCache(dl, config, callback,
                                                 callback_user, stats, NULL);
}
