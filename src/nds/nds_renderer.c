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
 * These six measured paths favor ARM's full register file and conditional
 * execution; retain the ordinary hot/O3 annotation for host-side fixtures. */
#if defined(__arm__)
#define NDS_RENDERER_HOT_CODE \
    __attribute__((hot, optimize("O3"), target("arm"), section(".itcm")))
#define NDS_RENDERER_FAST_RUN_CODE \
    __attribute__((noinline, optimize("O3"), target("arm")))
#else
#define NDS_RENDERER_HOT_CODE __attribute__((hot, optimize("O3")))
#define NDS_RENDERER_FAST_RUN_CODE \
    __attribute__((noinline, optimize("O3")))
#endif

/* Profiles 0/1 publish the shipping contract through the compact frame
 * summary.  Profile 1 is the low-frequency O2 coarse build, so it must not
 * maintain the generic per-command proof ledger either.  Profile 2 retains
 * the full forensic counters and semantic observer. */
#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_RECORD_PROOF_ONLY(statement) ((void)0)
#else
#define NDS_RENDERER_RECORD_PROOF_ONLY(statement) \
    do { statement; } while (0)
#endif

#if NDS_RENDERER_HW_TRIANGLES
#include <math.h>
#include <nds.h>
#include <nds/arm9/postest.h>
#endif

#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX)
#define NDS_RENDERER_BENCHMARK_SINK_WORDS 1024u
#define NDS_RENDERER_BENCHMARK_SINK_MASK \
    (NDS_RENDERER_BENCHMARK_SINK_WORDS - 1u)

u32 gNdsRendererBenchmarkSink[
    NDS_RENDERER_BENCHMARK_SINK_WORDS] __attribute__((aligned(32)));
volatile u32 gNdsRendererBenchmarkSinkCursor;
volatile u32 gNdsRendererBenchmarkSinkWordCount;
volatile u32 gNdsRendererBenchmarkSinkOwnerWords[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
volatile u32 gNdsRendererBenchmarkSinkCalibrationWords;
volatile u32 gNdsRendererBenchmarkSinkCalibrationTicks;
static u32 sNdsRendererBenchmarkSinkCursor;
static u32 sNdsRendererBenchmarkSinkWordCount;
static u32 sNdsRendererBenchmarkSinkLastOwnerCursor;
static u32 sNdsRendererBenchmarkSinkOwnerWords[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
static u32 sNdsRendererBenchmarkFakeTextureName = 1u;
static u32 sNdsRendererBenchmarkTextureParameter;

static inline void ndsRendererBenchmarkSinkWord(u32 value)
{
    gNdsRendererBenchmarkSink[
        sNdsRendererBenchmarkSinkCursor &
        NDS_RENDERER_BENCHMARK_SINK_MASK] = value;
    sNdsRendererBenchmarkSinkCursor++;
    sNdsRendererBenchmarkSinkWordCount++;
}

static inline void ndsRendererBenchmarkGlEnable(int bits)
{
    ndsRendererBenchmarkSinkWord(0xe1000000u | (u32)bits);
}

static inline void ndsRendererBenchmarkGlDisable(int bits)
{
    ndsRendererBenchmarkSinkWord(0xd1000000u | (u32)bits);
}

static inline void ndsRendererBenchmarkGlAlphaFunc(int threshold)
{
    ndsRendererBenchmarkSinkWord((u32)threshold);
}

static inline void ndsRendererBenchmarkGlFogDensity(int index, int density)
{
    ndsRendererBenchmarkSinkWord(
        ((u32)index & 0xffu) | (((u32)density & 0xffu) << 8));
}

static inline void ndsRendererBenchmarkGlFogShift(int shift)
{
    ndsRendererBenchmarkSinkWord((u32)shift);
}

static inline void ndsRendererBenchmarkGlFogOffset(int offset)
{
    ndsRendererBenchmarkSinkWord((u32)offset);
}

static inline void ndsRendererBenchmarkGlFogColor(
    u8 red, u8 green, u8 blue, u8 alpha)
{
    ndsRendererBenchmarkSinkWord(
        (u32)red | ((u32)green << 5) | ((u32)blue << 10) |
        ((u32)alpha << 15));
}

static inline void ndsRendererBenchmarkGlTexParameter(int target, int param)
{
    (void)target;
    sNdsRendererBenchmarkTextureParameter = (u32)param;
    ndsRendererBenchmarkSinkWord((u32)param);
}

static inline u32 ndsRendererBenchmarkGlGetTexParameter(void)
{
    return sNdsRendererBenchmarkTextureParameter;
}

static inline void ndsRendererBenchmarkGlBindTexture(int target, int name)
{
    (void)target;
    ndsRendererBenchmarkSinkWord((u32)name);
}

static inline int ndsRendererBenchmarkGlGenTextures(int count, int *names)
{
    int i;

    if ((count <= 0) || (names == NULL))
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        names[i] = (int)sNdsRendererBenchmarkFakeTextureName++;
        ndsRendererBenchmarkSinkWord((u32)names[i]);
    }
    return 1;
}

static inline int ndsRendererBenchmarkGlDeleteTextures(int count, int *names)
{
    int i;

    if ((count <= 0) || (names == NULL))
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        ndsRendererBenchmarkSinkWord((u32)names[i]);
    }
    return 1;
}

static inline int ndsRendererBenchmarkGlTexImage2D(
    int target, int empty1, GL_TEXTURE_TYPE_ENUM type,
    int size_x, int size_y, int empty2, int params, const void *texture)
{
    (void)target;
    (void)empty1;
    (void)empty2;
    (void)texture;
    sNdsRendererBenchmarkTextureParameter = (u32)params;
    ndsRendererBenchmarkSinkWord((u32)type);
    ndsRendererBenchmarkSinkWord(
        ((u32)size_x & 0xffffu) | ((u32)size_y << 16));
    ndsRendererBenchmarkSinkWord((u32)params);
    return 1;
}

static inline void ndsRendererBenchmarkGlMatrixMode(int mode)
{
    ndsRendererBenchmarkSinkWord((u32)mode);
}

static inline void ndsRendererBenchmarkGlLoadMatrix4x4(const m4x4 *matrix)
{
    const u32 *words = (const u32 *)matrix;
    u32 i;

    for (i = 0u; i < 16u; i++)
    {
        ndsRendererBenchmarkSinkWord(words[i]);
    }
}

static inline void ndsRendererBenchmarkGlVertex3v16(v16 x, v16 y, v16 z)
{
    ndsRendererBenchmarkSinkWord(
        (u32)(u16)x | ((u32)(u16)y << 16));
    ndsRendererBenchmarkSinkWord((u32)(u16)z);
}

static inline void ndsRendererBenchmarkGlPolyFmt(u32 params)
{
    ndsRendererBenchmarkSinkWord(params);
}

static inline void ndsRendererBenchmarkGlBegin(GL_GLBEGIN_ENUM mode)
{
    ndsRendererBenchmarkSinkWord((u32)mode);
}

static inline void ndsRendererBenchmarkGlColor(u16 color)
{
    ndsRendererBenchmarkSinkWord((u32)color);
}

static inline void ndsRendererBenchmarkGlTexCoord2t16(t16 s, t16 t)
{
    ndsRendererBenchmarkSinkWord(
        (u32)(u16)s | ((u32)(u16)t << 16));
}

#define glEnable ndsRendererBenchmarkGlEnable
#define glDisable ndsRendererBenchmarkGlDisable
#define glAlphaFunc ndsRendererBenchmarkGlAlphaFunc
#define glFogDensity ndsRendererBenchmarkGlFogDensity
#define glFogShift ndsRendererBenchmarkGlFogShift
#define glFogOffset ndsRendererBenchmarkGlFogOffset
#define glFogColor ndsRendererBenchmarkGlFogColor
#define glTexParameter ndsRendererBenchmarkGlTexParameter
#define glGetTexParameter ndsRendererBenchmarkGlGetTexParameter
#define glBindTexture ndsRendererBenchmarkGlBindTexture
#define glGenTextures ndsRendererBenchmarkGlGenTextures
#define glDeleteTextures ndsRendererBenchmarkGlDeleteTextures
#define glTexImage2D ndsRendererBenchmarkGlTexImage2D
#define glMatrixMode ndsRendererBenchmarkGlMatrixMode
#define glLoadMatrix4x4 ndsRendererBenchmarkGlLoadMatrix4x4
#define glVertex3v16 ndsRendererBenchmarkGlVertex3v16
#define glPolyFmt ndsRendererBenchmarkGlPolyFmt
#define glBegin ndsRendererBenchmarkGlBegin
#define glColor ndsRendererBenchmarkGlColor
#define glTexCoord2t16 ndsRendererBenchmarkGlTexCoord2t16
#endif

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
void ndsRendererBenchmarkSinkEndOwner(NDSRendererProfileOwner owner)
{
    u32 cursor = sNdsRendererBenchmarkSinkCursor;

    if ((u32)owner < NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererBenchmarkSinkOwnerWords[(u32)owner] +=
            cursor - sNdsRendererBenchmarkSinkLastOwnerCursor;
    }
    sNdsRendererBenchmarkSinkLastOwnerCursor = cursor;
}
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
#define NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT 4u
#define NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT 128u
#define NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT 2u
#define NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS 1024u
#define NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT 256u
#define NDS_RENDERER_HW_CI4_CLASS_KEY_MASK 0x7ffffu
#define NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT 19u
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
#define NDS_RENDERER_HW_POS_TEST_MAX 40u
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
#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 48u
#define NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT 128u
#define NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY 0u
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
#define NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK \
    (NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL | \
     NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX | \
     NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE)
#if NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state) \
    ((state)->texture_prepare_valid = FALSE)
#define NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state) \
    ((state)->prepared_light_direction_valid = FALSE)
#else
#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state) ((void)(state))
#define NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state) ((void)(state))
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 1
static u32 sNdsRendererProfileGXStatusPostVBlank;
static u32 sNdsRendererProfileGXControlPostVBlank;
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererProfileImmutableListCount;
static u32 sNdsRendererProfileTrustedCommandCount;
static u32 sNdsRendererProfileValidatedCommandCount;
static u32 sNdsRendererProfileTriangleRunReuseCount;
static u32 sNdsRendererProfileTriangleSubmitTicks;
static u32 sNdsRendererProfileVertexSubmitTicks;
static u32 sNdsRendererProfileCi4LutBuildCount;
static u32 sNdsRendererProfileCi4LutReuseCount;
static NDSRendererProfileOwner sNdsRendererProfileOwner =
    NDS_RENDERER_PROFILE_OWNER_NONE;

typedef enum NDSRendererSemanticOutcome
{
    NDS_RENDERER_SEMANTIC_INPUT_REJECT = 0,
    NDS_RENDERER_SEMANTIC_TRANSFORM_REJECT,
    NDS_RENDERER_SEMANTIC_ALPHA_ZERO,
    NDS_RENDERER_SEMANTIC_EMITTED
} NDSRendererSemanticOutcome;

#define NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID (1u << 0)
#define NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID (1u << 1)
#define NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID (1u << 2)

typedef struct NDSRendererSemanticVertex
{
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    u16 color;
    u32 valid_flags;
} NDSRendererSemanticVertex;

typedef struct NDSRendererSemanticEvent
{
    u32 owner;
    u32 owner_occurrence;
    u32 list_ordinal;
    u32 branch_path;
    u32 command_index;
    u32 tri2_half;
    u32 outcome;
    u32 packed;
    u32 vertex_index[3];
    u32 submit_class;
    u32 source_state_hash;
    u32 raw_snapshot_id;
    u32 vertex_matrix_snapshot[3];
    u32 vertex_clip_snapshot[3];
    u32 context_flags;
    u32 source_zbuffered;
    u32 zbuffered;
    u32 raw_submit;
    u32 projected_submit;
    u32 decal_depth;
    u32 prim_depth;
    u32 source_clip_depth;
    u32 poly_alpha;
    u32 poly_fmt;
    u32 alpha_key;
    u32 fog_key;
    u32 fog_color;
    s32 fog_min;
    s32 fog_max;
    u32 fog_status;
    u32 texture_name;
    u32 texture_key_hash;
    u32 texture_params;
    u32 matrix_loaded;
    u32 matrix_mode;
    u32 matrix_generation;
    u32 matrix_signature;
    s32 no_z_depth_before;
    s32 no_z_depth_after;
    u32 no_z_background_before;
    u32 no_z_background_after;
    s32 projected_z[3];
    NDSRendererSemanticVertex vertex[3];
} NDSRendererSemanticEvent;

typedef struct NDSRendererSemanticSourceProvenance
{
    u32 owner_occurrence;
    u32 list_ordinal;
    u32 root_branch_path;
} NDSRendererSemanticSourceProvenance;

static NDSRendererSemanticSourceProvenance
    sNdsRendererSemanticSourceProvenance;
static u32 sNdsRendererSemanticOwnerLastOccurrence[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
static u32 sNdsRendererSemanticOwnerOccurrenceValidMask;
static u32 sNdsRendererSemanticOutputHash;
static u32 sNdsRendererSemanticOutputHash2;
static u32 sNdsRendererSemanticEventCount;
static u32 sNdsRendererSemanticOverflowCount;
static u32 sNdsRendererSemanticLastTextureKeyHash;
static u32 sNdsRendererSemanticLastTextureParams;

typedef struct NDSRendererProfileOwnerHotLedger
{
    u32 submit_class_count[8];
    u32 material_operation_count;
    u32 texture_change_count;
    u32 run_count;
    u32 semantic_output_hash;
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
} NDSRendererProfileOwnerHotLedger;

static NDSRendererProfileOwnerHotLedger
    sNdsRendererProfileOwnerHot[NDS_RENDERER_PROFILE_OWNER_COUNT];

static inline NDSRendererProfileOwnerHotLedger *
ndsRendererProfileCurrentOwner(void)
{
    return ((u32)sNdsRendererProfileOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT) ?
        &sNdsRendererProfileOwnerHot[(u32)sNdsRendererProfileOwner] : NULL;
}

static void ndsRendererSemanticHash1Word(u32 *hash, u32 value)
{
    u32 i;

    for (i = 0u; i < sizeof(value); i++)
    {
        *hash ^= (value >> (i * 8u)) & 0xffu;
        *hash *= 16777619u;
    }
}

static void ndsRendererSemanticHash2Word(u32 *hash, u32 value)
{
    u32 mixed = *hash;

    mixed ^= value + 0x9e3779b9u + (mixed << 6) + (mixed >> 2);
    mixed = ((mixed << 13) | (mixed >> 19)) * 0x85ebca6bu;
    *hash = mixed;
}

static u32 ndsRendererSemanticBranchPath(u32 parent_path,
                                         u32 command_index,
                                         u32 depth,
                                         u32 branch_is_jump)
{
    u32 hash = 2166136261u;

    ndsRendererSemanticHash1Word(&hash, 0x4252414eu);
    ndsRendererSemanticHash1Word(&hash, parent_path);
    ndsRendererSemanticHash1Word(&hash, command_index);
    ndsRendererSemanticHash1Word(&hash, depth);
    ndsRendererSemanticHash1Word(&hash, branch_is_jump);
    return (hash != 0u) ? hash : 1u;
}

static u32 ndsRendererSemanticSourceStateHash(
    const NDSRendererStats *stats)
{
    u32 hash = 2166136261u;
    u32 i;

#define NDS_RENDERER_SEMANTIC_STATE_WORD(value) \
    ndsRendererSemanticHash1Word(&hash, (u32)(value))
    NDS_RENDERER_SEMANTIC_STATE_WORD(0x53524331u);
    if (stats == NULL)
    {
        NDS_RENDERER_SEMANTIC_STATE_WORD(0xffffffffu);
        return hash;
    }

    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->othermode_h);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->othermode_l);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->geometry_mode);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_mask);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_kind);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_scale_s);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_scale_t);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_level);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_on);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_xparam);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_state_flags);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_image);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_format);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_size);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_image_width);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_set_tile_count);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tlut_image);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tlut_count);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tlut_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_format);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_size);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_line);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_tmem);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_palette);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_cms);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_cmt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_masks);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_maskt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_shifts);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_shiftt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_render_tile_flags);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_uls);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_ult);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_lrs);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_block_dxt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_texels);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_tile);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_uls);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_ult);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_lrs);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_size_lrt);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_width);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_tile_height);
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        const NDSRendererTileState *tile = &stats->texture_tiles[i];

        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->set_seen);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->size_seen);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->format);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->size);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->line);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->tmem);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->palette);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->cms);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->cmt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->masks);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->maskt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->shifts);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->shiftt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->uls);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->ult);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->lrs);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->lrt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->width);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->height);
        NDS_RENDERER_SEMANTIC_STATE_WORD(tile->flags);
    }
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_load_sequence);
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        const NDSRendererTextureLoadState *load =
            &stats->texture_loads[i];

        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->sequence);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image_width);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_uls);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_ult);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_lrs);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_dxt);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_texels);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_tmem);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->valid);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image_format);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->image_size);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_kind);
        NDS_RENDERER_SEMANTIC_STATE_WORD(load->load_tile);
    }
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_combine_w0);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_combine_w1);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->texture_combine_count);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_min_level);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_lod_fraction);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->env_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->blend_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_color_1);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_color_2);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_color_mask);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_x);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_y);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_z);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->light_dir_mask);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_depth);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->prim_depth_delta);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_color);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_min);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_max);
    NDS_RENDERER_SEMANTIC_STATE_WORD(stats->fog_status);
#undef NDS_RENDERER_SEMANTIC_STATE_WORD
    return hash;
}

static void ndsRendererSemanticHashEvent(u32 *hash1,
                                         u32 *hash2,
                                         const NDSRendererSemanticEvent *event)
{
    u32 i;

#define NDS_RENDERER_SEMANTIC_EVENT_WORD(value) do { \
        u32 semantic_word = (u32)(value); \
        ndsRendererSemanticHash1Word(hash1, semantic_word); \
        ndsRendererSemanticHash2Word(hash2, semantic_word); \
    } while (0)
    NDS_RENDERER_SEMANTIC_EVENT_WORD(0x53454d31u);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->owner);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->owner_occurrence);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->list_ordinal);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->branch_path);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->command_index);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->tri2_half);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->outcome);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->packed);
    for (i = 0u; i < 3u; i++)
    {
        NDS_RENDERER_SEMANTIC_EVENT_WORD(event->vertex_index[i]);
    }
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->submit_class);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->source_state_hash);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->raw_snapshot_id);
    for (i = 0u; i < 3u; i++)
    {
        NDS_RENDERER_SEMANTIC_EVENT_WORD(
            event->vertex_matrix_snapshot[i]);
        NDS_RENDERER_SEMANTIC_EVENT_WORD(
            event->vertex_clip_snapshot[i]);
    }
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->context_flags);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->source_zbuffered);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->zbuffered);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->raw_submit);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->projected_submit);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->decal_depth);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->prim_depth);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->source_clip_depth);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->poly_alpha);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->poly_fmt);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->alpha_key);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_key);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_color);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_min);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_max);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->fog_status);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->texture_name);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->texture_key_hash);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->texture_params);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_loaded);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_mode);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_generation);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->matrix_signature);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_depth_before);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_depth_after);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_background_before);
    NDS_RENDERER_SEMANTIC_EVENT_WORD(event->no_z_background_after);
    for (i = 0u; i < 3u; i++)
    {
        const NDSRendererSemanticVertex *vertex = &event->vertex[i];

        NDS_RENDERER_SEMANTIC_EVENT_WORD(event->projected_z[i]);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->x);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->y);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->z);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->s);
        NDS_RENDERER_SEMANTIC_EVENT_WORD((u16)vertex->t);
        NDS_RENDERER_SEMANTIC_EVENT_WORD(vertex->color);
        NDS_RENDERER_SEMANTIC_EVENT_WORD(vertex->valid_flags);
    }
    NDS_RENDERER_SEMANTIC_EVENT_WORD(0x454e4431u);
#undef NDS_RENDERER_SEMANTIC_EVENT_WORD
}

static void ndsRendererSemanticCommitEvent(
    const NDSRendererSemanticEvent *event)
{
    NDSRendererProfileOwnerHotLedger *owner =
        ndsRendererProfileCurrentOwner();
    u32 owner_hash1;
    u32 owner_hash2;
    u32 frame_hash1;
    u32 frame_hash2;
    u32 frame_index;

    if ((event == NULL) || (owner == NULL))
    {
        return;
    }
    if (owner->semantic_event_count == 0u)
    {
        owner->semantic_first_owner_occurrence = event->owner_occurrence;
        owner->semantic_first_list_ordinal = event->list_ordinal;
        owner->semantic_first_branch_path = event->branch_path;
        owner->semantic_first_command_index = event->command_index;
        owner->semantic_first_tri2_half = event->tri2_half;
        owner->semantic_first_outcome = event->outcome;
    }

    owner_hash1 = (owner->semantic_event_count == 0u) ?
        2166136261u : owner->semantic_output_hash;
    owner_hash2 = (owner->semantic_event_count == 0u) ?
        0x9e3779b9u : owner->semantic_output_hash2;
    ndsRendererSemanticHashEvent(&owner_hash1, &owner_hash2, event);
    owner->semantic_output_hash = owner_hash1;
    owner->semantic_output_hash2 = owner_hash2;
    owner->semantic_event_count++;

    frame_index = sNdsRendererSemanticEventCount;
    frame_hash1 = (frame_index == 0u) ?
        2166136261u : sNdsRendererSemanticOutputHash;
    frame_hash2 = (frame_index == 0u) ?
        0x9e3779b9u : sNdsRendererSemanticOutputHash2;
    ndsRendererSemanticHashEvent(&frame_hash1, &frame_hash2, event);
    sNdsRendererSemanticOutputHash = frame_hash1;
    sNdsRendererSemanticOutputHash2 = frame_hash2;
    if (frame_index < NDS_RENDERER_SEMANTIC_TRACE_CAPACITY)
    {
        gNdsRendererSemanticPrefixHash[frame_index] = frame_hash1;
        gNdsRendererSemanticPrefixHash2[frame_index] = frame_hash2;
    }
    else
    {
        sNdsRendererSemanticOverflowCount++;
        owner->semantic_overflow_count++;
    }
    sNdsRendererSemanticEventCount++;
}
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

#if NDS_RENDERER_PROFILE_LEVEL == 0
volatile u32 gNdsRendererFastRunMode =
    NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES;
#else
volatile u32 gNdsRendererFastRunMode = NDS_RENDERER_FAST_RUN_GENERIC;
#endif
volatile u32 gNdsRendererFastRunCount;
volatile u32 gNdsRendererFastTriangleCount;
volatile u32 gNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
volatile u32 gNdsRendererFastFallbackCount[3];
static NDSRendererProfileOwner sNdsRendererRuntimeOwner =
    NDS_RENDERER_PROFILE_OWNER_NONE;
static u32 sNdsRendererFastOwnerEnabled;
static u32 sNdsRendererStageTextureSitesEnabled;
static u32 sNdsRendererFastRunCount;
static u32 sNdsRendererFastTriangleCount;
static u32 sNdsRendererFastOwnerTriangleCount[
    NDS_RENDERER_PROFILE_OWNER_COUNT];
static u32 sNdsRendererFastFallbackCount[3];

/* Direct immutable TRI-run records are topology only.  Live vertex, matrix,
 * material, texture, and light state remains in the traversal object and is
 * rebound by the existing exact path.  The cache is reset with reloc/source
 * caches at scene boundaries, so it never survives pointer ownership changes. */
#define NDS_RENDERER_DIRECT_RAW_PLAN_COUNT 128u
#define NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT 384u

typedef struct NDSRendererDirectRawEntry
{
    u32 required_mask;
    u8 indices[6];
    u8 triangle_count;
    u8 reserved;
} NDSRendererDirectRawEntry;

typedef struct NDSRendererDirectRawPlan
{
    const Gfx *source;
    u16 entry_offset;
    u16 command_count;
    u16 triangle_count;
    u16 reserved;
    u32 first_w0;
    u32 first_w1;
    u32 last_w0;
    u32 last_w1;
} NDSRendererDirectRawPlan;

static NDSRendererDirectRawPlan
    sNdsRendererDirectRawPlans[NDS_RENDERER_DIRECT_RAW_PLAN_COUNT];
static NDSRendererDirectRawEntry
    sNdsRendererDirectRawEntries[NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT];
static u32 sNdsRendererDirectRawEntryCount;

_Static_assert(
    (sizeof(sNdsRendererDirectRawPlans) +
     sizeof(sNdsRendererDirectRawEntries)) <= (8u * 1024u),
    "direct raw topology cache must remain within 8 KiB");

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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererHardwareMatrixSignature;

static inline u32 ndsRendererProfileHashU32(u32 hash, u32 value)
{
    u32 i;

    if (hash == 0u)
    {
        hash = 2166136261u;
    }
    for (i = 0u; i < sizeof(value); i++)
    {
        hash ^= (value >> (i * 8u)) & 0xffu;
        hash *= 16777619u;
    }
    if (hash == 0u)
    {
        hash = 1u;
    }
    return hash;
}

static u32 ndsRendererProfileHashMatrixPair(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 mode, u32 generation)
{
    u32 hash = 0u;
    u32 row;
    u32 col;

    hash = ndsRendererProfileHashU32(hash, mode);
    hash = ndsRendererProfileHashU32(hash, generation);
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            hash = ndsRendererProfileHashU32(
                hash, (u32)projection->m[row][col]);
            hash = ndsRendererProfileHashU32(
                hash, (u32)modelview->m[row][col]);
        }
    }
    return hash;
}
#endif
static u32 sNdsRendererHardwareSubmitClassCounts[
    NDS_RENDERER_HW_SUBMIT_CLASS_COUNT];
/* The logical demand is derived exactly from the submitted class totals.
 * Profiles 1/2 pack actual call/clamp counts plus error flags into this one
 * existing summary word, avoiding forensic BSS growth in the tight P1 build. */
#define NDS_RENDERER_HW_DIVISION_CALL_MASK          0x00000fffu
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_SHIFT 12u
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_ONE   (1u << 12)
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_MASK  0x000ff000u
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_SHIFT 20u
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_ONE  (1u << 20)
#define NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_MASK 0x0ff00000u
#define NDS_RENDERER_HW_DIVISION_ZERO_DENOMINATOR   (1u << 28)
#define NDS_RENDERER_HW_DIVISION_MISMATCH           (1u << 29)
static u32 sNdsRendererHardwareDivideSummary;
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
    u32 texture_lookup_call_count;
    u32 texture_lookup_probe_count;
    u32 texture_lookup_active_hit_count;
    u32 texture_lookup_table_hit_count;
    u32 texture_lookup_miss_count;
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
static u32 sNdsRendererRuntimeCi4IndexCacheBuildCount;
static u32 sNdsRendererRuntimeCi4IndexCacheReuseCount;
static u32 sNdsRendererRuntimeCi4RepresentativePixelCount;
static u32 sNdsRendererRuntimeCi4ReusePixelCount;
#endif

#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
volatile u32 gNdsRendererBenchmarkTriangleCount;
static u32 sNdsRendererBenchmarkTriangleCount;
#endif

/* Profile 2 compares every phase-masked palette-pair result against the
 * existing exact blend formula before the pixels are submitted to GX. Keep
 * symbols available in profile 1 so one synchronized benchmark marker can be
 * used for both coarse timing and forensic runs. */
volatile u32 gNdsRendererProfileTexturePairOracleChecks;
volatile u32 gNdsRendererProfileTexturePairOracleMismatches;
volatile u32 gNdsRendererProfileTextureVBlankQueuedUploads;
volatile u32 gNdsRendererProfileTextureVBlankQueuedBytes;
volatile u32 gNdsRendererProfileTextureVBlankCommittedUploads;
volatile u32 gNdsRendererProfileTextureVBlankCommitTicks;
volatile u32 gNdsRendererProfileTextureVBlankOutsideCount;
volatile u32 gNdsRendererProfileTextureVBlankFallbackCount;
volatile u32 gNdsRendererProfileTextureVBlankStartLine;
volatile u32 gNdsRendererProfileTextureVBlankEndLine;

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
volatile u32 gNdsRendererBenchmarkSuppressedTextureUploads;
volatile u32 gNdsRendererBenchmarkSuppressedTextureUploadBytes;
static u32 sNdsRendererBenchmarkSuppressedTextureUploads;
static u32 sNdsRendererBenchmarkSuppressedTextureUploadBytes;
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

static inline void ndsRendererProfileRecordCi4IndexCacheBuild(void)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeCi4IndexCacheBuildCount++;
#endif
}

static inline void ndsRendererProfileRecordCi4IndexCacheReuse(void)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeCi4IndexCacheReuseCount++;
#endif
}

static inline void ndsRendererProfileRecordCi4RepresentativeReuse(
    u32 representative_pixels, u32 reused_pixels)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeCi4RepresentativePixelCount += representative_pixels;
    sNdsRendererRuntimeCi4ReusePixelCount += reused_pixels;
#else
    (void)representative_pixels;
    (void)reused_pixels;
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    {
        NDSRendererProfileOwnerHotLedger *owner =
            ndsRendererProfileCurrentOwner();

        if (owner != NULL)
        {
            owner->run_count++;
        }
    }
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    {
        NDSRendererProfileOwnerHotLedger *owner =
            ndsRendererProfileCurrentOwner();

        if (owner != NULL)
        {
            owner->texture_change_count++;
        }
    }
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        {
            NDSRendererProfileOwnerHotLedger *owner =
                ndsRendererProfileCurrentOwner();

            if (owner != NULL)
            {
                owner->submit_class_count[(u32)submit_class]++;
            }
        }
#endif
    }
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
    sNdsRendererHardwareDivideSummary = 0u;
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
    gNdsRendererProfileHardwareDivideSummary =
        sNdsRendererHardwareDivideSummary;
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
    u32 key_generation;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 key_hash;
#endif
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
    u32 source_texels;
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

#if NDS_RENDERER_PROFILE_LEVEL < 2
typedef struct NDSRendererHardwareCi4IndexCacheEntry
{
    const u8 *source;
    u32 source_texels;
    u32 byte_lane_xor;
    u32 valid;
    u8 indices[NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS];
} NDSRendererHardwareCi4IndexCacheEntry;
#endif

typedef struct NDSRendererHardwareLightShadeCacheEntry
{
    u32 valid;
    u32 diffuse;
    u32 ambient;
    u32 rgb[NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT];
} NDSRendererHardwareLightShadeCacheEntry;

static NDSRendererHardwareTextureCacheEntry
    sNdsRendererHardwareTextureCache[NDS_RENDERER_HW_TEXTURE_CACHE_COUNT];
#if NDS_RENDERER_PROFILE_LEVEL < 2
static u8 sNdsRendererHardwareTextureLookup[
    NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT];
#if defined(__arm__)
_Static_assert(sizeof(NDSRendererHardwareTextureKey) == 236u,
               "texture key layout must remain exact-match stable");
_Static_assert(
    (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT &
     (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u)) == 0u,
    "texture lookup count must remain a power of two");
_Static_assert(NDS_RENDERER_HW_TEXTURE_CACHE_COUNT <
                   NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT,
               "texture lookup must retain an empty cluster terminator");
#endif
#endif
static u32 sNdsRendererHardwareTextureCacheNext;
static u32 sNdsRendererHardwareTextureKeyGeneration;
static u32 sNdsRendererHardwareFrameSerial;
static const NDSRendererHardwareTextureCacheEntry
    *sNdsRendererHardwareActiveTextureEntry;
static u16 sNdsRendererHardwareTextureScratch[
    NDS_RENDERER_HW_TEXTURE_MAX_TEXELS];
#if NDS_RENDERER_PROFILE_LEVEL < 2
#define NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT 2u
#define NDS_RENDERER_HW_TEXTURE_REFRESH_SMALL_TEXELS 2048u
#define NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS 64u
typedef struct NDSRendererHardwareTextureRefresh
{
    NDSRendererHardwareTextureCacheEntry *entry;
    const u16 *pixels;
    u32 staged_bytes;
    u32 texture_bytes;
    u32 row_bytes;
    u32 row_count;
    u8 row_map[NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
} NDSRendererHardwareTextureRefresh;
static NDSRendererHardwareTextureRefresh
    sNdsRendererHardwareTextureRefreshQueue[
        NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT];
static u32 sNdsRendererHardwareTextureRefreshCount;
static u16 sNdsRendererHardwareTextureRefreshSmall[
    NDS_RENDERER_HW_TEXTURE_REFRESH_SMALL_TEXELS];
static u16 sNdsRendererHardwareTextureRefreshLarge[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH *
    NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS];
#endif
static u8 sNdsRendererHardwareTexel01Ci4Source0S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4Source1S[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4Source0T[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
static u8 sNdsRendererHardwareTexel01Ci4Source1T[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
#if NDS_RENDERER_PROFILE_LEVEL < 2
static u8 sNdsRendererHardwareTexel01Ci4RepresentativeS[
    NDS_RENDERER_HW_TEXTURE_MAX_WIDTH];
static u8 sNdsRendererHardwareTexel01Ci4RepresentativeT[
    NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
static u32 sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid;
static u32 sNdsRendererHardwareTexel01Ci4ClassTable[
    NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT];
#if defined(__arm__)
_Static_assert(
    sizeof(sNdsRendererHardwareTexel01Ci4Source0T) +
    sizeof(sNdsRendererHardwareTexel01Ci4Source1T) +
    sizeof(sNdsRendererHardwareTexel01Ci4RepresentativeS) +
    sizeof(sNdsRendererHardwareTexel01Ci4RepresentativeT) == 512u,
    "CI4 representative maps must stay within 512 bytes");
_Static_assert(sizeof(sNdsRendererHardwareTexel01Ci4ClassTable) == 1024u,
               "CI4 representative class table must stay within 1 KiB");
_Static_assert(
    (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT &
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT - 1u)) == 0u,
    "CI4 representative class table must remain a power of two");
_Static_assert(
    (NDS_RENDERER_HW_TEXTURE_MAX_WIDTH <=
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT / 2u)) &&
    (NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT <=
     (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT / 2u)),
    "CI4 representative class table must remain at most half full");
#endif
#endif
static u32 sNdsRendererHardwareTexel01Ci4PairLut[
    NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT];
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareTexel01Ci4PairLut) == 1024u,
               "phase-masked CI4 pair lookup must stay within 1 KiB");
#endif
static u16 sNdsRendererHardwareTexel01Ci4LutPalette0[16];
static u16 sNdsRendererHardwareTexel01Ci4LutPalette1[16];
static u32 sNdsRendererHardwareTexel01Ci4LutFraction;
static u32 sNdsRendererHardwareTexel01Ci4LutKeyValid;
#if NDS_RENDERER_PROFILE_LEVEL < 2
static NDSRendererHardwareCi4IndexCacheEntry
    sNdsRendererHardwareCi4IndexCache[
        NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT];
static u32 sNdsRendererHardwareCi4IndexCacheNext;
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareCi4IndexCache) == 2080u,
               "CI4 index cache must stay within 2080 bytes");
#endif
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
static NDSRendererHardwareLightShadeCacheEntry
    sNdsRendererHardwareLightShadeCache[
        NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT];
static u32 sNdsRendererHardwareLightShadeCacheNext;
#if defined(__arm__)
_Static_assert(sizeof(sNdsRendererHardwareLightShadeCache) == 2096u,
               "light shade lookup cache must stay within 2096 bytes");
#endif
#endif

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

static inline u32 ndsRendererHardwareLitShadeColorLut(
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction,
    const u32 *rgb_lut)
{
    s32 dot;
    u32 diffuse_numer;

    dot = ((s32)(s8)vtx->r * direction->x) +
        ((s32)(s8)vtx->g * direction->y) +
        ((s32)(s8)vtx->b * direction->z);
    if (dot <= 0)
    {
        diffuse_numer = 0u;
    }
    else if (dot > (127 * 127))
    {
        diffuse_numer = 127u;
    }
    else
    {
        diffuse_numer = (u32)((dot * 127) / (127 * 127));
    }
    return rgb_lut[diffuse_numer] | (u32)vtx->a;
}
#endif

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
#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u32 *ndsRendererHardwareGetLightShadeLut(
    u32 diffuse, u32 ambient);
#endif
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

void ndsRendererMtxMulAffine20p12(const NDSRendererMatrix20p12 *lhs,
                                  const NDSRendererMatrix20p12 *rhs,
                                  NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 temp;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererMatrix20p12 oracle;
    u32 mismatch = FALSE;
#endif
    u32 row;
    u32 col;

    if ((lhs == NULL) || (rhs == NULL) || (out == NULL))
    {
        return;
    }
    if ((lhs->m[0][3] != 0) || (lhs->m[1][3] != 0) ||
        (lhs->m[2][3] != 0) ||
        (lhs->m[3][3] != (1 << NDS_RENDERER_DS_MTX_FRAC_BITS)) ||
        (rhs->m[0][3] != 0) || (rhs->m[1][3] != 0) ||
        (rhs->m[2][3] != 0) ||
        (rhs->m[3][3] != (1 << NDS_RENDERER_DS_MTX_FRAC_BITS)))
    {
        ndsRendererMtxMul20p12(lhs, rhs, out);
        return;
    }

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            s64 sum = (s64)lhs->m[row][0] * rhs->m[0][col] +
                (s64)lhs->m[row][1] * rhs->m[1][col] +
                (s64)lhs->m[row][2] * rhs->m[2][col];

            temp.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(
                    sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
        temp.m[row][3] = 0;
    }
    for (col = 0u; col < 3u; col++)
    {
        s64 sum = (s64)lhs->m[3][0] * rhs->m[0][col] +
            (s64)lhs->m[3][1] * rhs->m[1][col] +
            (s64)lhs->m[3][2] * rhs->m[2][col] +
            (s64)lhs->m[3][3] * rhs->m[3][col];

        temp.m[3][col] = ndsRendererClampS64ToS32(
            ndsRendererRoundShiftS64(sum,
                                     NDS_RENDERER_DS_MTX_FRAC_BITS));
    }
    temp.m[3][3] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererMtxMul20p12(lhs, rhs, &oracle);
    gNdsRendererProfileAffineMatrixSamples++;
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 delta = (s64)temp.m[row][col] - oracle.m[row][col];
            u32 magnitude;

            if (delta < 0)
            {
                delta = -delta;
            }
            magnitude = (delta > (s64)UINT_MAX) ? UINT_MAX : (u32)delta;
            if (magnitude != 0u)
            {
                mismatch = TRUE;
                if (magnitude > gNdsRendererProfileAffineMatrixMaxDelta)
                {
                    gNdsRendererProfileAffineMatrixMaxDelta = magnitude;
                }
            }
        }
    }
    if (mismatch != FALSE)
    {
        gNdsRendererProfileAffineMatrixMismatches++;
    }
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareMatrixSignature = 0u;
#endif
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

typedef u32 NDSRendererAliasedU32 __attribute__((__may_alias__));

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

    /* DS is little-endian, so an aligned may-alias word load is exactly the
     * bytewise payload decode below. Retain that fallback for arbitrary DLs. */
    if ((((uintptr_t)src) & 3u) == 0u)
    {
        const NDSRendererAliasedU32 *words =
            (const NDSRendererAliasedU32 *)src;

        xy = words[0];
        zf = words[1];
        st = words[2];
        rgba = words[3];
    }
    else
    {
        xy = ndsRendererReadU32(src);
        zf = ndsRendererReadU32((const u8 *)src + 4);
        st = ndsRendererReadU32((const u8 *)src + 8);
        rgba = ndsRendererReadU32((const u8 *)src + 12);
    }
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

#if NDS_RENDERER_PROFILE_LEVEL >= 2
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

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
}

static void ndsRendererRecordCull(NDSRendererStats *stats, u32 w0, u32 w1)
{
    stats->cull_command_count++;
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
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
    NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
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
                                          NDSRendererTraversalVertexStorage
                                              *vertex_storage,
                                          NDSRendererMatrixSnapshot *snapshots,
                                          u32 snapshot_count)
{
    if (state == NULL)
    {
        return;
    }

    /* Valid masks and stack depth own every scratch read. Initialize that
     * compact control plane instead of clearing matrix, stack, and derived
     * arrays that will be overwritten before their first valid use. */
    state->modelview_stack_depth = 0u;
    state->vertex_valid_mask = 0u;
    state->vertices = (vertex_storage != NULL) ?
        vertex_storage->vertices : NULL;
#if NDS_RENDERER_HW_TRIANGLES
    state->input_vertices = (vertex_storage != NULL) ?
        vertex_storage->input_vertices : NULL;
    state->vertex_colors = (vertex_storage != NULL) ?
        vertex_storage->vertex_colors : NULL;
    state->vertex_matrix_snapshot = (vertex_storage != NULL) ?
        vertex_storage->vertex_matrix_snapshot : NULL;
    state->vertex_clip_snapshot = (vertex_storage != NULL) ?
        vertex_storage->vertex_clip_snapshot : NULL;
    state->matrix_snapshots = snapshots;
    state->matrix_snapshot_count =
        (snapshot_count <= NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY) ?
            snapshot_count : NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->matrix_generation = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->texture_prepare_key_hash = 0u;
    state->texture_prepare_params = 0u;
    state->semantic_branch_path =
        sNdsRendererSemanticSourceProvenance.root_branch_path;
    if (state->semantic_branch_path == 0u)
    {
        state->semantic_branch_path = ndsRendererSemanticBranchPath(
            (u32)sNdsRendererProfileOwner,
            sNdsRendererSemanticSourceProvenance.owner_occurrence,
            sNdsRendererSemanticSourceProvenance.list_ordinal,
            FALSE);
    }
    state->semantic_command_index = 0u;
    state->semantic_tri2_half = 0u;
#endif
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->raw_vertex_fit_mask = 0u;
#else
    (void)snapshots;
    (void)snapshot_count;
#endif
    state->projection_valid = 0u;
    state->modelview_valid = 0u;
    state->matrix_valid = 0u;
    state->matrix_word_valid = 0u;
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
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
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
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
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

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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
    NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
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

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    stats->matrix_command_count++;
    stats->matrix_modelview_count++;
    stats->matrix_pop_count += count;

    while (count != 0u)
    {
        u32 depth = state->modelview_stack_depth;

        if (depth == 0u)
        {
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
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

static void NDS_RENDERER_HOT_CODE
ndsRendererApplyVertexCommand(
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
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u32 *prepared_light_shade_lut = NULL;
#endif
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
#endif

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->vertex_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_RENDERER_MAX_VTX, &v0,
                              &count) == FALSE)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    stats->source_vertex_count += count;
#endif
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
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
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
        if (state->prepared_light_direction_valid == 0u)
        {
            /* Matrix and MOVEMEM handlers invalidate this exact source-state
             * value; adjacent VTX commands can share its float normalization. */
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state->modelview_valid != 0u) ? &state->modelview : NULL,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_light_direction = &state->prepared_light_direction;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut(
                stats->light_color_1, stats->light_color_2);
        }
#endif
    }
#endif

    for (i = 0u; i < count; i++)
    {
        u32 index = v0 + i;
#if NDS_RENDERER_HW_TRIANGLES
        NDSRendererInputVertex *input = &state->input_vertices[index];
        u32 mask = 1u << index;
#else
        NDSRendererInputVertex input_storage;
        NDSRendererInputVertex *input = &input_storage;
        NDSRendererClipVertex20p12 *out = &state->vertices[index];
#endif

        ndsRendererDecodeInputVertex(input, src + (i * 16u));
#if NDS_RENDERER_HW_TRIANGLES
        ndsRendererProfileRecordSourceVertexLoad();
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (prepared_light_shade_lut != NULL)
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorLut(
                    input, prepared_light_direction,
                    prepared_light_shade_lut);
        }
        else
#endif
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorPrepared(
                    stats, input, prepared_light_direction);
        }
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
        ndsRendererTransformVertex20p12(&state->matrix, input, out);
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

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
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

#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static v16 __attribute__((noinline, optimize("Os")))
ndsRendererHardwareProjectToV16(s64 numerator, s32 denominator)
#else
static inline v16 ndsRendererHardwareProjectToV16(
    s64 numerator, s32 denominator)
#endif
{
    s64 low_product;
    s64 high_product;
    v16 result;

    if (denominator == 0)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary |=
            NDS_RENDERER_HW_DIVISION_ZERO_DENOMINATOR;
#endif
        return 0;
    }

    /* The DS 64/32 divider returns a signed 32-bit quotient. Pre-clamp the
     * exact C result into v16 range so the hardware operation cannot overflow
     * its result register. Negative W reverses both product comparisons. */
    low_product = (s64)-32768 * (s64)denominator;
    high_product = (s64)32767 * (s64)denominator;
    if (((denominator > 0) && (numerator < low_product)) ||
        ((denominator < 0) && (numerator > low_product)))
    {
        result = (v16)-32768;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary +=
            NDS_RENDERER_HW_DIVISION_PRECLAMP_LOW_ONE;
#endif
    }
    else if (((denominator > 0) && (numerator > high_product)) ||
             ((denominator < 0) && (numerator < high_product)))
    {
        result = (v16)32767;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary +=
            NDS_RENDERER_HW_DIVISION_PRECLAMP_HIGH_ONE;
#endif
    }
    else
    {
#if defined(__arm__)
        result = (v16)div64(numerator, denominator);
#else
        result = ndsRendererHardwareClampS64ToV16(
            numerator / denominator);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareDivideSummary++;
#endif
    }

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (result != ndsRendererHardwareClampS64ToV16(
                      numerator / denominator))
    {
        sNdsRendererHardwareDivideSummary |=
            NDS_RENDERER_HW_DIVISION_MISMATCH;
    }
#endif
    return result;
}
#endif

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

static s32 ndsRendererHardwareAlphaUsesVertex(
    const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) !=
         NDS_RENDERER_ALPHA_COMPARE_THRESHOLD) &&
        ((stats->othermode_l & NDS_RENDERER_FORCE_BL) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_CVG_X_ALPHA) == 0u) &&
        ((stats->othermode_l & NDS_RENDERER_ZMODE_MASK) !=
         NDS_RENDERER_ZMODE_XLU))
    {
        return FALSE;
    }
    if (ndsRendererHardwareBlendAlphaUsesMemory(stats) != FALSE)
    {
        return FALSE;
    }
    if ((stats != NULL) && (stats->texture_combine_count != 0u))
    {
        if ((ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_PRIMITIVE) != FALSE) ||
            (ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_ENVIRONMENT) != FALSE))
        {
            return FALSE;
        }
        if ((ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_TEXEL0) == FALSE) &&
            (ndsRendererHardwareOutputUsesAlpha(
                 stats, NDS_RENDERER_ACMUX_SHADE) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
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

#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u32 * __attribute__((noinline))
ndsRendererHardwareGetLightShadeLut(u32 diffuse, u32 ambient)
{
    NDSRendererHardwareLightShadeCacheEntry *entry;
    u32 i;

    /* Cache only the exact RGB function of the two source light colors and
     * diffuse numerator. Vertex normals, direction, and alpha stay live. */
    for (i = 0u; i < NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT; i++)
    {
        entry = &sNdsRendererHardwareLightShadeCache[i];
        if ((entry->valid != 0u) &&
            (entry->diffuse == diffuse) &&
            (entry->ambient == ambient))
        {
            return entry->rgb;
        }
    }

    entry = &sNdsRendererHardwareLightShadeCache[
        sNdsRendererHardwareLightShadeCacheNext];
    sNdsRendererHardwareLightShadeCacheNext =
        (sNdsRendererHardwareLightShadeCacheNext + 1u) &
        (NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT - 1u);
    entry->valid = FALSE;
    entry->diffuse = diffuse;
    entry->ambient = ambient;
    for (i = 0u; i < NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT; i++)
    {
        s32 r = (s32)ndsRendererHardwareColorByte(ambient, 24) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 24) * i) / 127u);
        s32 g = (s32)ndsRendererHardwareColorByte(ambient, 16) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 16) * i) / 127u);
        s32 b = (s32)ndsRendererHardwareColorByte(ambient, 8) +
            (s32)((ndsRendererHardwareColorByte(diffuse, 8) * i) / 127u);

        entry->rgb[i] =
            ((u32)ndsRendererHardwareClampColor(r) << 24) |
            ((u32)ndsRendererHardwareClampColor(g) << 16) |
            ((u32)ndsRendererHardwareClampColor(b) << 8);
    }
    entry->valid = TRUE;
    return entry->rgb;
}
#endif

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

static u32 NDS_RENDERER_HOT_CODE
ndsRendererHardwareLitShadeColorPrepared(
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

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererProfileTextureKeyHashFull(
    const NDSRendererHardwareTextureKey *key)
{
    const u32 *words = (const u32 *)key;
    u32 hash = 0u;
    u32 i;

    if (key == NULL)
    {
        return 0u;
    }
    for (i = 0u; i < (sizeof(*key) / sizeof(*words)); i++)
    {
        hash = ndsRendererProfileHashU32(hash, words[i]);
    }
    return hash;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static u32 ndsRendererHardwareTextureKeyHash(
    const NDSRendererHardwareTextureKey *key)
{
    u32 hash;

    if (key == NULL)
    {
        return 0u;
    }
    /* This is an index fingerprint, not the equality oracle.  Mix the
     * high-entropy image/material identity and animated tile state, then keep
     * the full 236-byte comparison on every candidate hit.  Hashing all 59
     * words cost more ARM9 time than the open-address lookup saved. */
    hash = key->image ^ (key->tlut_image * 0x9e3779b9u);
    hash ^= key->texel1_image * 0x85ebca6bu;
    hash ^= (key->width << 16) ^ key->height;
    hash ^= (key->format << 28) ^ (key->size << 24) ^ key->flags;
    hash ^= (key->load_uls << 16) ^ key->load_ult ^ key->load_lrs;
    hash ^= (key->tile_uls << 16) ^ key->tile_ult ^ key->tile_lrs;
    hash ^= (key->texel1_load_uls << 16) ^ key->texel1_load_ult ^
        key->texel1_load_lrs;
    hash ^= (key->texel1_tile_uls << 16) ^ key->texel1_tile_ult ^
        key->texel1_tile_lrs;
    hash ^= key->prim_lod_fraction * 0xc2b2ae35u;
    hash ^= key->combine_w0 ^ (key->combine_w1 * 0x27d4eb2fu);
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    return hash;
}

static void ndsRendererHardwareTextureLookupInsert(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot_value;
    u32 slot;
    u32 probe;

    if ((entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot_value = (u32)(entry - sNdsRendererHardwareTextureCache) + 1u;
    slot = entry->key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        if (value == slot_value)
        {
            return;
        }
        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            sNdsRendererHardwareTextureLookup[slot] = (u8)slot_value;
            return;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
}

static void ndsRendererHardwareTextureLookupRemove(
    const NDSRendererHardwareTextureCacheEntry *entry)
{
    u32 slot_value;
    u32 slot;
    u32 probe;

    if ((entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot_value = (u32)(entry - sNdsRendererHardwareTextureCache) + 1u;
    slot = entry->key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            return;
        }
        if (value == slot_value)
        {
            /* Repair the remainder of this linear-probe cluster instead of
             * leaving tombstones that animated water keys could accumulate
             * over a five-minute match.  The table is twice the cache size,
             * so an empty terminator is guaranteed. */
            sNdsRendererHardwareTextureLookup[slot] =
                NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY;
            slot = (slot + 1u) &
                (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
            while (sNdsRendererHardwareTextureLookup[slot] !=
                   NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
            {
                value = sNdsRendererHardwareTextureLookup[slot];
                sNdsRendererHardwareTextureLookup[slot] =
                    NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY;
                ndsRendererHardwareTextureLookupInsert(
                    &sNdsRendererHardwareTextureCache[value - 1u]);
                slot = (slot + 1u) &
                    (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
            }
            return;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
}
#endif

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

#if NDS_RENDERER_PROFILE_LEVEL >= 2
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
#endif

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexture(const NDSRendererHardwareTextureKey *key,
                               u32 key_hash)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 slot;
    u32 probe;
#else
    u32 i;
#endif

    if (key == NULL)
    {
        return NULL;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeFrameSummary.texture_lookup_call_count++;
    entry = (NDSRendererHardwareTextureCacheEntry *)
        sNdsRendererHardwareActiveTextureEntry;
    if ((entry != NULL) && (entry->ready != 0u) &&
        (entry->key_hash == key_hash) &&
        (ndsRendererHardwareTextureKeyEqual(&entry->key, key) != FALSE))
    {
        sNdsRendererRuntimeFrameSummary.texture_lookup_active_hit_count++;
        return entry;
    }

    slot = key_hash & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    for (probe = 0u; probe < NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT; probe++)
    {
        u32 value = sNdsRendererHardwareTextureLookup[slot];

        sNdsRendererRuntimeFrameSummary.texture_lookup_probe_count++;
        if (value == NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY)
        {
            break;
        }
        entry = &sNdsRendererHardwareTextureCache[value - 1u];
        if ((entry->ready != 0u) && (entry->key_hash == key_hash) &&
            (ndsRendererHardwareTextureKeyEqual(&entry->key, key) != FALSE))
        {
            sNdsRendererRuntimeFrameSummary
                .texture_lookup_table_hit_count++;
            return entry;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
#else
    (void)key_hash;
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
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererRuntimeFrameSummary.texture_lookup_miss_count++;
#endif
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
    u32 staged_bytes,
    u32 texture_bytes,
    const u8 *row_map,
    u32 row_bytes,
    u32 row_count)
{
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    if ((entry == NULL) || (entry->name == 0) ||
        (texture == NULL) || (staged_bytes == 0u) ||
        (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))))
    {
        return FALSE;
    }
    ndsRendererBenchmarkSinkWord((u32)entry->name);
    ndsRendererBenchmarkSinkWord(staged_bytes);
    ndsRendererBenchmarkSinkWord(texture_bytes);
    return TRUE;
#else
    void *vram_address;
    uintptr_t vram_first;
    uintptr_t vram_last;
    u32 vram_state;
    u32 offset;
    u32 i;

    if ((entry == NULL) || (entry->name == 0) ||
        (texture == NULL) || (staged_bytes == 0u) ||
        (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))))
    {
        return FALSE;
    }
    if (row_map != NULL)
    {
        u32 staged_rows = staged_bytes / row_bytes;

        for (i = 0u; i < row_count; i++)
        {
            if (row_map[i] >= staged_rows)
            {
                return FALSE;
            }
        }
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

    DC_FlushRange(texture, staged_bytes);
    if (row_map == NULL)
    {
        for (offset = 0u; offset < texture_bytes; offset += staged_bytes)
        {
            dmaCopyWords(0, texture, (u8 *)vram_address + offset,
                         staged_bytes);
        }
    }
    else
    {
        /* The first occurrence of every distinct rendered row is stored once.
         * Adjacent first occurrences remain adjacent in the staging buffer, so
         * coalesce those runs and issue individual copies only for repeats. */
        for (i = 0u; i < row_count; )
        {
            u32 run = 1u;
            u32 source_row = row_map[i];

            while (((i + run) < row_count) &&
                   (row_map[i + run] == (source_row + run)))
            {
                run++;
            }
            dmaCopyWords(0, (const u8 *)texture + (source_row * row_bytes),
                         (u8 *)vram_address + (i * row_bytes),
                         run * row_bytes);
            i += run;
        }
    }
    vramRestorePrimaryBanks(vram_state);
    return TRUE;
#endif
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererHardwareTextureRefreshUses(
    const u16 *pixels)
{
    u32 i;

    for (i = 0u; i < sNdsRendererHardwareTextureRefreshCount; i++)
    {
        if (sNdsRendererHardwareTextureRefreshQueue[i].pixels == pixels)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsRendererHardwareQueueTextureRefresh(
    NDSRendererHardwareTextureCacheEntry *entry,
    const u16 *pixels,
    u32 staged_bytes,
    u32 texture_bytes,
    const u8 *row_map,
    u32 row_bytes,
    u32 row_count)
{
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    (void)entry;
    (void)pixels;
    (void)staged_bytes;
    (void)texture_bytes;
    (void)row_map;
    (void)row_bytes;
    (void)row_count;
    return FALSE;
#else
    NDSRendererHardwareTextureRefresh *refresh;

    if ((entry == NULL) || (entry->name == 0) || (pixels == NULL) ||
        (staged_bytes == 0u) || (texture_bytes == 0u) ||
        ((row_map == NULL) && ((texture_bytes % staged_bytes) != 0u)) ||
        ((row_map != NULL) &&
         ((row_bytes == 0u) || (row_count == 0u) ||
          (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
          ((staged_bytes % row_bytes) != 0u) ||
          (texture_bytes != (row_bytes * row_count)))) ||
        (sNdsRendererHardwareTextureRefreshCount >=
         NDS_RENDERER_HW_TEXTURE_REFRESH_QUEUE_COUNT) ||
        (glGetTexturePointer(entry->name) == NULL))
    {
        return FALSE;
    }
    refresh = &sNdsRendererHardwareTextureRefreshQueue[
        sNdsRendererHardwareTextureRefreshCount++];
    refresh->entry = entry;
    refresh->pixels = pixels;
    refresh->staged_bytes = staged_bytes;
    refresh->texture_bytes = texture_bytes;
    refresh->row_bytes = row_bytes;
    refresh->row_count = row_count;
    if (row_map != NULL)
    {
        memcpy(refresh->row_map, row_map, row_count);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureVBlankQueuedUploads++;
    gNdsRendererProfileTextureVBlankQueuedBytes += texture_bytes;
#endif
    return TRUE;
#endif
}
#endif

u32 ndsRendererHardwareCommitPendingTextureRefreshes(void)
{
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 committed = 0u;
    u32 i;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 start;
    u32 start_line;

    start_line = REG_VCOUNT;
    start = cpuGetTiming();
    gNdsRendererProfileTextureVBlankStartLine = start_line;
#endif

    for (i = 0u; i < sNdsRendererHardwareTextureRefreshCount; i++)
    {
        NDSRendererHardwareTextureRefresh *refresh =
            &sNdsRendererHardwareTextureRefreshQueue[i];

        if (ndsRendererHardwareReplaceTextureData(
                refresh->entry, refresh->pixels, refresh->staged_bytes,
                refresh->texture_bytes,
                (refresh->row_count != 0u) ? refresh->row_map : NULL,
                refresh->row_bytes, refresh->row_count) != FALSE)
        {
            committed++;
        }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        else
        {
            gNdsRendererProfileTextureVBlankFallbackCount++;
        }
#endif
        refresh->entry = NULL;
        refresh->pixels = NULL;
        refresh->staged_bytes = 0u;
        refresh->texture_bytes = 0u;
        refresh->row_bytes = 0u;
        refresh->row_count = 0u;
    }
    sNdsRendererHardwareTextureRefreshCount = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileTextureVBlankCommittedUploads += committed;
    gNdsRendererProfileTextureVBlankCommitTicks += cpuGetTiming() - start;
    gNdsRendererProfileTextureVBlankEndLine = REG_VCOUNT;
    if ((committed != 0u) &&
        ((start_line < 192u) ||
         (gNdsRendererProfileTextureVBlankEndLine < 192u)))
    {
        gNdsRendererProfileTextureVBlankOutsideCount++;
    }
#endif
    return committed;
#else
    /* Profile 2 keeps its large semantic/oracle ledger resident. Shipping and
     * coarse builds own the compact VBlank staging buffers; the forensic build
     * retains the exact synchronous upload as its independent oracle route. */
    return 0u;
#endif
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareReleaseTexture(
    NDSRendererHardwareTextureCacheEntry *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupRemove(entry);
#endif
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

#define NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT 128u

typedef struct NDSRendererStageTextureSite
{
    const Gfx *site;
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 state_hash1;
    u32 state_hash2;
    u32 entry_generation;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 uses_texel1;
    u32 prim_lod_fraction;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 semantic_key_hash;
    u32 semantic_params;
    u32 texel1_tile_state;
    u32 texel1_primary_state;
    u32 texel1_image0;
    u32 texel1_image1;
#endif
} NDSRendererStageTextureSite;

static NDSRendererStageTextureSite
    sNdsRendererStageTextureSites[NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT];
static u32 sNdsRendererStageTextureSiteNext;

_Static_assert(
    sizeof(sNdsRendererStageTextureSites) <= (12u * 1024u),
    "stage texture-site plans must stay below 12 KiB");

static void ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileTextureCacheEntry(
    const NDSRendererHardwareTextureCacheEntry *entry);
static void ndsRendererProfileTextureFormat(
    volatile u32 *mask,
    u32 format,
    u32 size);
static void ndsRendererRecordTextureLaneUse(
    const NDSRendererConfig *config,
    u32 format,
    u32 size);
#endif

static u32 ndsRendererStageTextureSiteSlot(const Gfx *site)
{
    u32 value = (u32)(uintptr_t)site;

    value ^= value >> 11;
    value ^= value >> 19;
    return value & (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
}

static NDSRendererStageTextureSite *ndsRendererStageTextureSiteFind(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats)
{
    u32 slot;
    u32 probe;

    if ((state == NULL) || (stats == NULL) ||
        (state->source_command_site == NULL))
    {
        return NULL;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *plan =
            &sNdsRendererStageTextureSites[slot];

        if (plan->site == NULL)
        {
            return NULL;
        }
        if ((plan->site == state->source_command_site) &&
            (plan->state_hash1 == stats->texture_source_hash1) &&
            (plan->state_hash2 == stats->texture_source_hash2))
        {
            return plan;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    return NULL;
}

static s32 ndsRendererStageTextureSiteTryBind(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    const NDSRendererConfig *config)
{
    NDSRendererStageTextureSite *plan;
    NDSRendererHardwareTextureCacheEntry *entry;

    if (sNdsRendererStageTextureSitesEnabled == 0u)
    {
        return FALSE;
    }
    plan = ndsRendererStageTextureSiteFind(state, stats);
    if (plan == NULL)
    {
        return FALSE;
    }
    entry = plan->entry;
    if ((entry == NULL) || (entry->ready == 0u) ||
        (entry->key_generation != plan->entry_generation) ||
        ((plan->uses_texel1 != 0u) &&
         (plan->prim_lod_fraction != stats->prim_lod_fraction)))
    {
        return FALSE;
    }

    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    if (sNdsRendererHardwareActiveTextureEntry != entry)
    {
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
    }
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = plan->format;
    stats->hardware_texture_width = plan->width;
    stats->hardware_texture_height = plan->height;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash = plan->semantic_key_hash;
    sNdsRendererSemanticLastTextureParams = plan->semantic_params;
    ndsRendererRecordTextureLaneUse(config, plan->format, plan->size);
    if (plan->uses_texel1 != 0u)
    {
        ndsRendererRecordTextureLaneUse(config, plan->format, plan->size);
        ndsRendererProfileRecordTexel1Composite();
        gNdsRendererProfileTexel1LastTileState = plan->texel1_tile_state;
        gNdsRendererProfileTexel1LastPrimaryState =
            plan->texel1_primary_state;
        gNdsRendererProfileTexel1LastFraction = plan->prim_lod_fraction;
        gNdsRendererProfileTexel1LastImage0 = plan->texel1_image0;
        gNdsRendererProfileTexel1LastImage1 = plan->texel1_image1;
    }
    ndsRendererProfileTextureFormat(
        &gNdsRendererProfileTextureBindFormatMask,
        plan->format, plan->size);
    ndsRendererProfileTextureCacheEntry(entry);
#endif
    return TRUE;
}

static void ndsRendererStageTextureSiteRemember(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats,
    NDSRendererHardwareTextureCacheEntry *entry,
    u32 format,
    u32 size,
    u32 width,
    u32 height)
{
    NDSRendererStageTextureSite *plan = NULL;
    NDSRendererStageTextureSite *empty = NULL;
    u32 slot;
    u32 probe;

    if ((sNdsRendererStageTextureSitesEnabled == 0u) ||
        (state == NULL) || (state->source_command_site == NULL) ||
        (stats == NULL) || (entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *candidate =
            &sNdsRendererStageTextureSites[slot];

        if (candidate->site == state->source_command_site)
        {
            plan = candidate;
            break;
        }
        if ((empty == NULL) && (candidate->site == NULL))
        {
            empty = candidate;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    if (plan == NULL)
    {
        plan = empty;
    }
    if (plan == NULL)
    {
        plan = &sNdsRendererStageTextureSites[
            sNdsRendererStageTextureSiteNext++ &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u)];
    }
    plan->site = state->source_command_site;
    plan->entry = entry;
    plan->state_hash1 = stats->texture_source_hash1;
    plan->state_hash2 = stats->texture_source_hash2;
    plan->entry_generation = entry->key_generation;
    plan->format = format;
    plan->size = size;
    plan->width = width;
    plan->height = height;
    plan->uses_texel1 = (entry->key.texel1_image != 0u) ? TRUE : FALSE;
    plan->prim_lod_fraction = entry->key.prim_lod_fraction;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    plan->semantic_key_hash = sNdsRendererSemanticLastTextureKeyHash;
    plan->semantic_params = sNdsRendererSemanticLastTextureParams;
    plan->texel1_tile_state = gNdsRendererProfileTexel1LastTileState;
    plan->texel1_primary_state = gNdsRendererProfileTexel1LastPrimaryState;
    plan->texel1_image0 = gNdsRendererProfileTexel1LastImage0;
    plan->texel1_image1 = gNdsRendererProfileTexel1LastImage1;
#endif
}

static void ndsRendererTextureSourceHashCommand(
    NDSRendererStats *stats,
    u32 w0,
    u32 w1)
{
    u32 hash1;
    u32 hash2;

    if ((stats == NULL) ||
        (sNdsRendererRuntimeOwner != NDS_RENDERER_PROFILE_OWNER_STAGE))
    {
        return;
    }

    hash1 = stats->texture_source_hash1;
    hash1 ^= w0 ^ ((w1 << 16) | (w1 >> 16));
    hash1 *= 16777619u;
    stats->texture_source_hash1 = hash1;

    hash2 = stats->texture_source_hash2 + w1 + 0x9e3779b9u;
    hash2 = (hash2 << 7) | (hash2 >> 25);
    stats->texture_source_hash2 = hash2 ^ w0 ^ 0x85ebca6bu;
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
    out->source_texels = source_last_index + 1u;
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
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

            if (alpha_prefix_count > 16u)
            {
                alpha_prefix_count = 16u;
            }
            rgb = (u16)(value & NDS_RENDERER_HW_TEXEL01_RGB_MASK);
            alpha_phase_mask = alpha_phase_prefix[alpha_prefix_count];
            sNdsRendererHardwareTexel01Ci4PairLut[lut_index] =
                (u32)rgb | ((u32)alpha_phase_mask << 16);
        }
    }
}

static inline u16 ndsRendererHardwareResolveTexel01Ci4Lut(
    u32 index0, u32 index1, u32 x, u32 y)
{
    u32 phase = ((y & 3u) << 2) | (x & 3u);
    u32 lut_index = (index0 << 4) | index1;
    u32 pair = sNdsRendererHardwareTexel01Ci4PairLut[lut_index];

    return (u16)((pair & NDS_RENDERER_HW_TEXEL01_RGB_MASK) |
        (((pair >> (16u + phase)) & 1u) << 15));
}

static inline u8 ndsRendererHardwareReadCi4Direct(
    const u8 *texels, u32 logical_texel_index, u32 byte_lane_xor)
{
    u8 packed = texels[(logical_texel_index >> 1) ^ byte_lane_xor];

    return ((logical_texel_index & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
}

#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u8 *ndsRendererHardwareGetCi4Indices(
    const u8 *source, u32 source_texels, u32 byte_lane_xor,
    const u8 *protected_indices)
{
    NDSRendererHardwareCi4IndexCacheEntry *entry;
    u32 replace_index;
    u32 i;

    if ((source == NULL) || (source_texels == 0u) ||
        (source_texels > NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS))
    {
        return NULL;
    }
    for (i = 0u; i < NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT; i++)
    {
        entry = &sNdsRendererHardwareCi4IndexCache[i];
        if ((entry->valid != 0u) &&
            (entry->source == source) &&
            (entry->source_texels == source_texels) &&
            (entry->byte_lane_xor == byte_lane_xor))
        {
            ndsRendererProfileRecordCi4IndexCacheReuse();
            return entry->indices;
        }
    }

    replace_index = sNdsRendererHardwareCi4IndexCacheNext;
    if (sNdsRendererHardwareCi4IndexCache[replace_index].indices ==
        protected_indices)
    {
        replace_index = (replace_index + 1u) &
            (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    }
    entry = &sNdsRendererHardwareCi4IndexCache[replace_index];
    sNdsRendererHardwareCi4IndexCacheNext =
        (replace_index + 1u) &
        (NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT - 1u);
    entry->valid = FALSE;
    entry->source = source;
    entry->source_texels = source_texels;
    entry->byte_lane_xor = byte_lane_xor;
    for (i = 0u; i < source_texels; i++)
    {
        entry->indices[i] = ndsRendererHardwareReadCi4Direct(
            source, i, byte_lane_xor);
    }
    entry->valid = TRUE;
    ndsRendererProfileRecordCi4IndexCacheBuild();
    return entry->indices;
}

static u32 ndsRendererHardwareBuildCi4RepresentativeMap(
    const u8 *source0, const u8 *source1, u8 *representative, u32 count)
{
    u32 i;
    u32 unique = 0u;

    /* The exact 18-bit class key plus one reserves zero as empty; the upper
     * field stores the first coordinate. At most 128 coordinates enter the
     * 256-slot table, so linear probing always reaches an empty terminator. */
    memset(sNdsRendererHardwareTexel01Ci4ClassTable, 0,
           sizeof(sNdsRendererHardwareTexel01Ci4ClassTable));
    for (i = 0u; i < count; i++)
    {
        u32 key = ((i & 3u) << 16) |
            ((u32)source1[i] << 8) | source0[i];
        u32 stored_key = key + 1u;
        u32 slot = (key * 0x9e3779b1u) >> 24;

        while (sNdsRendererHardwareTexel01Ci4ClassTable[slot] != 0u)
        {
            u32 entry = sNdsRendererHardwareTexel01Ci4ClassTable[slot];

            if ((entry & NDS_RENDERER_HW_CI4_CLASS_KEY_MASK) == stored_key)
            {
                representative[i] =
                    (u8)(entry >> NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT);
                break;
            }
            slot = (slot + 1u) &
                (NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT - 1u);
        }
        if (sNdsRendererHardwareTexel01Ci4ClassTable[slot] == 0u)
        {
            sNdsRendererHardwareTexel01Ci4ClassTable[slot] =
                (i << NDS_RENDERER_HW_CI4_CLASS_INDEX_SHIFT) | stored_key;
            representative[i] = (u8)i;
            unique++;
        }
    }
    return unique;
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL < 2
static s32 ndsRendererHardwareStageUniqueTextureRows(
    const u16 *source,
    u16 *staging,
    u32 staging_bytes,
    u8 *row_map,
    u32 row_bytes,
    u32 row_count,
    u32 *staged_bytes)
{
    u32 staging_rows;
    u32 unique_rows = 0u;
    u32 y;

    if ((source == NULL) || (staging == NULL) || (row_map == NULL) ||
        (staged_bytes == NULL) || (row_bytes == 0u) || (row_count == 0u) ||
        (row_count > NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) ||
        ((row_bytes & (sizeof(u16) - 1u)) != 0u))
    {
        return FALSE;
    }
    staging_rows = staging_bytes / row_bytes;
    if ((staging_rows == 0u) ||
        (staging_rows > NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS))
    {
        return FALSE;
    }

    if (sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid == 0u)
    {
        return FALSE;
    }
    for (y = 0u; y < row_count; y++)
    {
        u32 representative =
            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];

        if ((representative >= row_count) || (representative > y))
        {
            return FALSE;
        }
        if (representative == y)
        {
            if (unique_rows >= staging_rows)
            {
                return FALSE;
            }
            memcpy((u8 *)staging + (unique_rows * row_bytes),
                   (const u8 *)source + (y * row_bytes), row_bytes);
            row_map[y] = (u8)unique_rows;
            unique_rows++;
        }
        else
        {
            row_map[y] = row_map[representative];
        }
    }
    *staged_bytes = unique_rows * row_bytes;
    return TRUE;
}
#endif

static s32 NDS_RENDERER_HOT_CODE
ndsRendererHardwareConvertTexel01Ci4Direct(
    const NDSRendererConfig *config,
    const u8 *texels0,
    u32 source0_texels,
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
    u8 *compact_row_map,
    u32 *compact_staged_bytes,
    u32 *green_texels,
    u32 *nonwhite_texels)
{
    u32 byte_lane_xor =
        (ndsRendererTextureDataLayout(config) ==
         NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED) ? 3u : 0u;
    const NDSRendererTileState *render_tile1 = source1->render_tile;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u8 *indices0 = ndsRendererHardwareGetCi4Indices(
        texels0, source0_texels, byte_lane_xor, NULL);
    const u8 *indices1 = ndsRendererHardwareGetCi4Indices(
        source1->texels, source1->source_texels, byte_lane_xor, indices0);
#else
    (void)source0_texels;
#endif
    u32 x;
    u32 y;

#if NDS_RENDERER_PROFILE_LEVEL < 2
    sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = FALSE;
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    (void)green_texels;
    (void)nonwhite_texels;
#else
    (void)compact_row_map;
    (void)compact_staged_bytes;
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
        sNdsRendererHardwareTexel01Ci4Source0T[y] = (u8)(
            (materialize0_t != FALSE) ?
                ndsRendererHardwareTextureMaskedAddress(
                    y, render_tile0->cmt, render_tile0->maskt) : y);
        sNdsRendererHardwareTexel01Ci4Source1T[y] = (u8)
            ndsRendererHardwareTextureAddressCoord(
                (s32)y + origin1_delta_t, render_tile1->height,
                source1->source_extent_height, render_tile1->cmt,
                render_tile1->maskt);
    }

#if NDS_RENDERER_PROFILE_LEVEL < 2
    if ((indices0 != NULL) && (indices1 != NULL))
    {
        u32 texels = width * height;

        /* Large clamped/masked animated tiles revisit the same pair of source
         * coordinates and ordered-coverage phase many times. Index the first
         * exact representative of each separable S/T class; forward X expansion
         * reads only earlier representatives. Cold output copies repeated rows
         * in reverse Y order, while warm staging records their exact row map. */
        if (texels >= 4096u)
        {
            u32 unique_s = 0u;
            u32 unique_t = 0u;
            u32 unique_texels;

            unique_s = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0S,
                sNdsRendererHardwareTexel01Ci4Source1S,
                sNdsRendererHardwareTexel01Ci4RepresentativeS, width);
            unique_t = ndsRendererHardwareBuildCi4RepresentativeMap(
                sNdsRendererHardwareTexel01Ci4Source0T,
                sNdsRendererHardwareTexel01Ci4Source1T,
                sNdsRendererHardwareTexel01Ci4RepresentativeT, height);

            unique_texels = unique_s * unique_t;
            if ((unique_texels * 2u) <= texels)
            {
                s32 compact_output =
                    (compact_row_map != NULL) &&
                    (compact_staged_bytes != NULL) &&
                    (width == upload_width) &&
                    (height <= NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT) &&
                    (unique_t <=
                     NDS_RENDERER_HW_TEXTURE_REFRESH_LARGE_ROWS) &&
                    ((unique_t * upload_width * sizeof(u16)) <=
                     sizeof(sNdsRendererHardwareTextureRefreshLarge));
                u16 *destination = (compact_output != FALSE) ?
                    sNdsRendererHardwareTextureRefreshLarge :
                    sNdsRendererHardwareTextureScratch;
                u32 unique_row = 0u;

                /* Warm animated-water refreshes already upload a compact set
                 * of unique rows during VBlank.  Produce that exact compact
                 * representation here instead of expanding repeated rows into
                 * the 32 KiB scratch arena and immediately copying the unique
                 * rows back into the 16 KiB refresh staging buffer.  Cold
                 * uploads use the same loop with the full scratch destination. */
                for (y = 0u; y < height; y++)
                {
                    u32 representative_y =
                        sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                    u32 source0_row;
                    u32 source1_row;
                    u32 dst_index;

                    if (representative_y != y)
                    {
                        if (compact_output != FALSE)
                        {
                            compact_row_map[y] =
                                compact_row_map[representative_y];
                        }
                        continue;
                    }
                    if (compact_output != FALSE)
                    {
                        compact_row_map[y] = (u8)unique_row;
                    }
                    source0_row = ((source0_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source0T[y]) *
                        source0_width) + source0_origin_s;
                    source1_row = ((source1->source_origin_t +
                        sNdsRendererHardwareTexel01Ci4Source1T[y]) *
                        source1->source_width) + source1->source_origin_s;
                    dst_index = ((compact_output != FALSE) ?
                        unique_row : y) * upload_width;

                    for (x = 0u; x < width; x++)
                    {
                        u32 index0;
                        u32 index1;
                        u32 representative_x =
                            sNdsRendererHardwareTexel01Ci4RepresentativeS[x];

                        if (representative_x != x)
                        {
                            destination[dst_index + x] =
                                destination[dst_index + representative_x];
                            continue;
                        }
                        index0 = indices0[source0_row +
                            sNdsRendererHardwareTexel01Ci4Source0S[x]];
                        index1 = indices1[source1_row +
                            sNdsRendererHardwareTexel01Ci4Source1S[x]];
                        destination[dst_index + x] =
                            ndsRendererHardwareResolveTexel01Ci4Lut(
                                index0, index1, x, y);
                    }
                    unique_row++;
                }
                if (compact_output != FALSE)
                {
                    *compact_staged_bytes =
                        unique_row * upload_width * sizeof(u16);
                }
                else
                {
                    for (y = height; y != 0u; )
                    {
                        u32 representative_y;

                        y--;
                        representative_y =
                            sNdsRendererHardwareTexel01Ci4RepresentativeT[y];
                        if (representative_y != y)
                        {
                            memcpy(&sNdsRendererHardwareTextureScratch[
                                       y * upload_width],
                                   &sNdsRendererHardwareTextureScratch[
                                       representative_y * upload_width],
                                   width * sizeof(u16));
                        }
                    }
                }
                ndsRendererProfileRecordCi4RepresentativeReuse(
                    unique_texels, texels - unique_texels);
                sNdsRendererHardwareTexel01Ci4RepresentativeRowsValid = TRUE;
                return compact_output;
            }
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

            for (x = 0u; x < width; x++)
            {
                u32 index0 = indices0[source0_row +
                    sNdsRendererHardwareTexel01Ci4Source0S[x]];
                u32 index1 = indices1[source1_row +
                    sNdsRendererHardwareTexel01Ci4Source1S[x]];

                sNdsRendererHardwareTextureScratch[dst_index + x] =
                    ndsRendererHardwareResolveTexel01Ci4Lut(
                        index0, index1, x, y);
            }
        }
        return FALSE;
    }
#endif
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
            u16 color = ndsRendererHardwareResolveTexel01Ci4Lut(
                index0, index1, x, y);

            sNdsRendererHardwareTextureScratch[dst_index + x] = color;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
            {
                u16 reference = ndsRendererHardwareBlendTexel01(
                    sNdsRendererHardwareTexel01Ci4LutPalette0[index0],
                    sNdsRendererHardwareTexel01Ci4LutPalette1[index1],
                    sNdsRendererHardwareTexel01Ci4LutFraction, x, y);

                gNdsRendererProfileTexturePairOracleChecks++;
                if (color != reference)
                {
                    gNdsRendererProfileTexturePairOracleMismatches++;
                }
            }
            ndsRendererProfileTexturePixel(
                color, green_texels, nonwhite_texels);
#endif
        }
    }
    return FALSE;
}

static s32 ndsRendererHardwareBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 texture_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
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
    u32 upload_bytes;
    u32 staged_bytes;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    u32 staged_row_bytes = 0u;
    u32 staged_row_count = 0u;
    u8 staged_row_map[NDS_RENDERER_HW_TEXTURE_MAX_HEIGHT];
#endif
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
    u32 key_hash;
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
#if NDS_RENDERER_PROFILE_LEVEL < 2
    s32 queue_texture_refresh = FALSE;
    s32 compact_row_output = FALSE;
#endif
    s32 texel1_origin_delta_s = 0;
    s32 texel1_origin_delta_t = 0;
    const NDSRendererTileState *render_tile;
    u32 green_texels = 0u;
    u32 nonwhite_texels = 0u;
    int size_x;
    int size_y;
    u32 x;
    u32 y;
    u16 *upload_buffer = sNdsRendererHardwareTextureScratch;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash = 0u;
    sNdsRendererSemanticLastTextureParams = 0u;
#endif
    if (stats == NULL)
    {
        return FALSE;
    }
    if (ndsRendererStageTextureSiteTryBind(stats, state, config) != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
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

#if NDS_RENDERER_PROFILE_LEVEL < 2
    key_hash = ndsRendererHardwareTextureKeyHash(&key);
#else
    key_hash = 0u;
#endif

    fraction_entry = NULL;
    entry = ndsRendererHardwareFindTexture(&key, key_hash);
    params = ndsRendererHardwareTextureParams(stats, render_tile,
                                               upload_width, upload_height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererSemanticLastTextureKeyHash =
        ndsRendererProfileTextureKeyHashFull(&key);
    sNdsRendererSemanticLastTextureParams = params;
#endif
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
        ndsRendererStageTextureSiteRemember(
            state, stats, entry, format, size, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileTextureTicks += cpuGetTiming() - texture_start;
#endif
        return TRUE;
    }
    if (use_texel1 != FALSE)
    {
        fraction_entry =
            ndsRendererHardwareFindTexel1RefreshTexture(&key);
    }

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD
    if ((sNdsRendererHardwareFrameSerial != 0u) &&
        (fraction_entry != NULL))
    {
        /* The first completed frame populates the exact resident allocations.
         * Subsequent animated-water key changes retain normal lookup, live
         * params, texture binding, batches, and geometry while deliberately
         * keeping the resident pixels from that warm frame. This benchmark
         * branch occurs before source resolution, conversion, or VRAM upload. */
        entry = fraction_entry;
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupRemove(entry);
#endif
        entry->key = key;
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        entry->key_hash = key_hash;
#endif
        entry->params = ndsRendererHardwareMergeTextureParams(params);
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        ndsRendererHardwareTextureLookupInsert(entry);
#endif
        sNdsRendererHardwareActiveTextureEntry = entry;
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = format;
        stats->hardware_texture_width = width;
        stats->hardware_texture_height = height;
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererBenchmarkSuppressedTextureUploads++;
        sNdsRendererBenchmarkSuppressedTextureUploadBytes +=
            upload_width * upload_height * sizeof(u16);
        ndsRendererStageTextureSiteRemember(
            state, stats, entry, format, size, width, height);
        return TRUE;
    }
#endif

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
    upload_bytes = upload_width * upload_height * sizeof(u16);
    staged_bytes = upload_bytes;
#if (NDS_RENDERER_PROFILE_LEVEL < 2) && \
    (NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_NONE)
    if (fraction_entry != NULL)
    {
        if ((upload_bytes <=
             sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
            (ndsRendererHardwareTextureRefreshUses(
                 sNdsRendererHardwareTextureRefreshSmall) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshSmall;
            queue_texture_refresh = TRUE;
        }
        else if ((upload_bytes >
                  sizeof(sNdsRendererHardwareTextureRefreshSmall)) &&
                 (ndsRendererHardwareTextureRefreshUses(
                      sNdsRendererHardwareTextureRefreshLarge) == FALSE))
        {
            upload_buffer =
                sNdsRendererHardwareTextureRefreshLarge;
            queue_texture_refresh = TRUE;
        }
    }
#endif
    if ((use_texel1 != FALSE) &&
        (format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (size == NDS_RENDERER_HW_TEXTURE_SIZ_4B) &&
        (texel1_source.format == NDS_RENDERER_HW_TEXTURE_FMT_CI) &&
        (texel1_source.size == NDS_RENDERER_HW_TEXTURE_SIZ_4B))
    {
        /* The source pond's two CI4 inputs can produce only 16x16 palette
         * pairs for a given primitive LOD fraction. Precompute exact RGB and
         * the 16-bit ordered-coverage mask once per pair so the animated pixel
         * loop avoids per-pixel color blending. */
        ndsRendererHardwareBuildTexel01Ci4Lut(
            config, tlut_src, stats->texture_tlut_count, palette_base,
            texel1_source.palette_base, stats->prim_lod_fraction);
        use_texel1_ci4_lut = TRUE;
        use_texel1_ci4_direct = TRUE;
    }

    /* Only the power-of-two rectangle handed to libnds is observable.  The
     * shared scratch arena is sized for the worst 128x128 texture, but smaller
     * animated uploads must not pay to clear the unused tail every frame. */
    if ((width != upload_width) || (height != upload_height))
    {
        memset(sNdsRendererHardwareTextureScratch, 0, upload_bytes);
    }
    if (use_texel1_ci4_direct != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        compact_row_output = ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_texels, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            width, height, upload_width,
            ((upload_buffer == sNdsRendererHardwareTextureRefreshLarge) &&
             (queue_texture_refresh != FALSE) &&
             (width == upload_width) && (height == upload_height)) ?
                staged_row_map : NULL,
            &staged_bytes, &green_texels, &nonwhite_texels);
#else
        (void)ndsRendererHardwareConvertTexel01Ci4Direct(
            config, texels_src, source_texels, source_width, source_origin_s,
            source_origin_t, render_tile, materialize_s, materialize_t,
            &texel1_source, texel1_origin_delta_s, texel1_origin_delta_t,
            width, height, upload_width, NULL, NULL,
            &green_texels, &nonwhite_texels);
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (upload_buffer == sNdsRendererHardwareTextureRefreshLarge)
    {
        staged_row_bytes = upload_width * sizeof(u16);
        staged_row_count = upload_height;
        if ((compact_row_output == FALSE) &&
            (ndsRendererHardwareStageUniqueTextureRows(
                sNdsRendererHardwareTextureScratch, upload_buffer,
                sizeof(sNdsRendererHardwareTextureRefreshLarge),
                staged_row_map, staged_row_bytes, staged_row_count,
                &staged_bytes) == FALSE))
        {
            upload_buffer = sNdsRendererHardwareTextureScratch;
            queue_texture_refresh = FALSE;
            staged_bytes = upload_bytes;
            staged_row_bytes = 0u;
            staged_row_count = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileTextureVBlankFallbackCount++;
#endif
        }
    }
    else if (upload_buffer != sNdsRendererHardwareTextureScratch)
    {
        memcpy(upload_buffer, sNdsRendererHardwareTextureScratch,
               staged_bytes);
    }
#endif
    entry = fraction_entry;
    if (entry != NULL)
    {
        s32 refresh_ready = FALSE;

#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (queue_texture_refresh != FALSE)
        {
            refresh_ready = ndsRendererHardwareQueueTextureRefresh(
                entry, upload_buffer, staged_bytes, upload_bytes,
                (staged_row_count != 0u) ? staged_row_map : NULL,
                staged_row_bytes, staged_row_count);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            if (refresh_ready == FALSE)
            {
                gNdsRendererProfileTextureVBlankFallbackCount++;
            }
#endif
        }
#endif
        if ((refresh_ready == FALSE) &&
            (ndsRendererHardwareReplaceTextureData(
                 entry, upload_buffer, staged_bytes, upload_bytes, NULL, 0u,
                 0u) != FALSE))
        {
            refresh_ready = TRUE;
        }
        if (refresh_ready == FALSE)
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

#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupRemove(entry);
#endif
    entry->key = key;
    sNdsRendererHardwareTextureKeyGeneration++;
    if (sNdsRendererHardwareTextureKeyGeneration == 0u)
    {
        sNdsRendererHardwareTextureKeyGeneration++;
    }
    entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    entry->key_hash = key_hash;
#endif
    entry->params = ndsRendererHardwareMergeTextureParams(params);
    entry->source_texels = texels;
    entry->green_texels = green_texels;
    entry->nonwhite_texels = nonwhite_texels;
    entry->profile_width = upload_width;
    entry->profile_height = upload_height;
    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    entry->ready = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareTextureLookupInsert(entry);
#endif
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
    ndsRendererStageTextureSiteRemember(
        state, stats, entry, format, size, width, height);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererHardwareMatrixSignature =
        ndsRendererProfileHashMatrixPair(
            projection, modelview, mode, generation);
#endif
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
    const NDSRendererClipVertex20p12 *vtx, s32 z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }

    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareProjectToV16(
        (s64)z * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->x = x;
        semantic_vertex->y = y;
        semantic_vertex->z = out_z;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
    }
#endif
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void ndsRendererHardwareClipVertexNdcDepth(
    const NDSRendererClipVertex20p12 *vtx, s32 z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }
    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareClampS64ToV16(z);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->x = x;
        semantic_vertex->y = y;
        semantic_vertex->z = out_z;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
    }
#endif
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
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 vertex_index,
    s32 projected_z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    , NDSRendererSemanticVertex *semantic_vertex
#endif
    )
{
    const NDSRendererInputVertex *vtx;
    const NDSRendererClipVertex20p12 *clip_vtx;
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

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        memset(semantic_vertex, 0, sizeof(*semantic_vertex));
    }
#endif
    if ((stats == NULL) || (state == NULL) ||
        (vertex_index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    vtx = &state->input_vertices[vertex_index];
    clip_vtx = &state->vertices[vertex_index];
    material_color = state->texture_prepare_material_color;
    scale_s = state->texture_prepare_scale_s;
    scale_t = state->texture_prepare_scale_t;
    texture_origin_s = state->texture_prepare_origin_s;
    texture_origin_t = state->texture_prepare_origin_t;
    context_flags = state->texture_prepare_vertex_flags;
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
    texture_offset = state->texture_prepare_offset;
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (semantic_vertex != NULL)
    {
        semantic_vertex->color = hardware_color;
        semantic_vertex->valid_flags |=
            NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID;
    }
#endif
    glColor(hardware_color);
    if (use_texture != FALSE)
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
        if (semantic_vertex != NULL)
        {
            semantic_vertex->s = s;
            semantic_vertex->t = t;
            semantic_vertex->valid_flags |=
                NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID;
        }
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

#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (semantic_vertex != NULL)
        {
            semantic_vertex->x = x;
            semantic_vertex->y = y;
            semantic_vertex->z = z;
            semantic_vertex->valid_flags |=
                NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID;
        }
#endif
        ndsRendererProfileVertexRange(vtx, x, y, z);
        glVertex3v16(x, y, z);
    }
    else if (prim_depth != FALSE)
    {
        ndsRendererHardwareClipVertex(clip_vtx, projected_z
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                      , semantic_vertex
#endif
                                      );
    }
    else if (decal_depth != FALSE)
    {
        if (clip_vtx != NULL)
        {
            ndsRendererHardwareClipVertex(
                clip_vtx, clip_vtx->z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                , semantic_vertex
#endif
                );
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
            s32 depth = ndsRendererHardwareProjectToV16(
                (s64)clip_vtx->z * NDS_RENDERER_HW_PROJECTED_VERTEX,
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
                x = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX,
                    clip_vtx->w);
                y = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX,
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
                    z = ndsRendererHardwareProjectToV16(
                        (s64)projected_z *
                            NDS_RENDERER_HW_PROJECTED_VERTEX,
                        clip_vtx->w);
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
            ndsRendererHardwareClipVertex(
                clip_vtx, projected_z, semantic_vertex);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(
                clip_vtx, projected_z, semantic_vertex);
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

#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_TRIANGLE_NOOP
static void __attribute__((noinline)) NDS_RENDERER_HOT_CODE
ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 packed)
{
    (void)stats;
    (void)config;
    (void)state;
    (void)packed;
    sNdsRendererBenchmarkTriangleCount++;
}
#else
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
    s32 projected_z[3] = { 0, 0, 0 };
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 vertex_submit_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererSemanticEvent semantic_event;
#endif

    if (stats == NULL)
    {
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    memset(&semantic_event, 0, sizeof(semantic_event));
    semantic_event.owner = (u32)sNdsRendererProfileOwner;
    semantic_event.owner_occurrence =
        sNdsRendererSemanticSourceProvenance.owner_occurrence;
    semantic_event.list_ordinal =
        sNdsRendererSemanticSourceProvenance.list_ordinal;
    semantic_event.branch_path = (state != NULL) ?
        state->semantic_branch_path :
        sNdsRendererSemanticSourceProvenance.root_branch_path;
    semantic_event.command_index = (state != NULL) ?
        state->semantic_command_index : 0u;
    semantic_event.tri2_half = (state != NULL) ?
        state->semantic_tri2_half : 0u;
    semantic_event.outcome = NDS_RENDERER_SEMANTIC_INPUT_REJECT;
    semantic_event.packed = packed;
    semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
    semantic_event.source_state_hash =
        ndsRendererSemanticSourceStateHash(stats);
    semantic_event.raw_snapshot_id =
        NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    semantic_event.no_z_depth_before =
        sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_depth_after =
        sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_background_before =
        sNdsRendererHardwareProjectedBackground;
    semantic_event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;
    semantic_event.fog_color = stats->fog_color;
    semantic_event.fog_min = stats->fog_min;
    semantic_event.fog_max = stats->fog_max;
    semantic_event.fog_status = stats->fog_status;
#endif
    if (ndsRendererInputTriangleReady(state, packed, &i0, &i1, &i2) == FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.vertex_index[0] = i0;
        semantic_event.vertex_index[1] = i1;
        semantic_event.vertex_index[2] = i2;
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.vertex_index[0] = i0;
    semantic_event.vertex_index[1] = i1;
    semantic_event.vertex_index[2] = i2;
    semantic_event.vertex_matrix_snapshot[0] =
        state->vertex_matrix_snapshot[i0];
    semantic_event.vertex_matrix_snapshot[1] =
        state->vertex_matrix_snapshot[i1];
    semantic_event.vertex_matrix_snapshot[2] =
        state->vertex_matrix_snapshot[i2];
    semantic_event.vertex_clip_snapshot[0] =
        state->vertex_clip_snapshot[i0];
    semantic_event.vertex_clip_snapshot[1] =
        state->vertex_clip_snapshot[i1];
    semantic_event.vertex_clip_snapshot[2] =
        state->vertex_clip_snapshot[i2];
#endif
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
        state->texture_prepare_source_zbuffered =
            (source_zbuffered != FALSE) ? TRUE : FALSE;
        state->texture_prepare_decal_depth =
            (decal_depth != FALSE) ? TRUE : FALSE;
        state->texture_prepare_prim_depth =
            (prim_depth != FALSE) ? TRUE : FALSE;
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.submit_class = (u32)submit_class;
    semantic_event.raw_snapshot_id = raw_snapshot_id;
    semantic_event.source_zbuffered =
        (source_zbuffered != FALSE) ? TRUE : FALSE;
    semantic_event.zbuffered = (zbuffered != FALSE) ? TRUE : FALSE;
    semantic_event.raw_submit = (raw_submit != FALSE) ? TRUE : FALSE;
    semantic_event.projected_submit =
        (projected_submit != FALSE) ? TRUE : FALSE;
    semantic_event.decal_depth = (decal_depth != FALSE) ? TRUE : FALSE;
    semantic_event.prim_depth = (prim_depth != FALSE) ? TRUE : FALSE;
    semantic_event.source_clip_depth =
        (source_clip_depth != FALSE) ? TRUE : FALSE;
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_TRANSFORM_REJECT;
        semantic_event.submit_class = NDS_RENDERER_HW_SUBMIT_REJECT;
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if (sNdsRendererHardwareNoOracle == 0u)
    {
        ndsRendererHardwareRecordOracleTriangle(state, i0, i1, i2);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL < 2
    if (state->texture_prepare_valid == 0u)
    {
        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        state->texture_prepare_material_color = material_color;
        state->texture_prepare_vertex_flags =
            ((use_material_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
            ((use_vertex_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    }
#else
    material_color = ndsRendererHardwareColorSource(stats);
    use_material_color = ndsRendererHardwareUseMaterialColor(stats);
    use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
    state->texture_prepare_material_color = material_color;
    state->texture_prepare_vertex_flags =
        ((use_material_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
        ((use_vertex_color != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    if (stats->texture_combine_count != 0u)
    {
        if (ndsRendererHardwareLitShadeCombine(stats) != FALSE)
        {
            gNdsRendererProfileLitShadeCombineCount++;
        }
        if (use_material_color != FALSE)
        {
            gNdsRendererProfileMaterialCombineCount++;
        }
    }
#endif
    if (state->texture_prepare_valid == 0u)
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
        state->texture_prepare_alpha_constant =
            (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ?
                TRUE : FALSE;
        if (state->texture_prepare_alpha_constant != 0u)
        {
            state->texture_prepare_poly_alpha = poly_alpha;
            state->texture_prepare_poly_fmt =
                ndsRendererHardwarePolyFmt(stats, poly_alpha);
        }
    }
    else if (state->texture_prepare_alpha_constant != 0u)
    {
        poly_alpha = state->texture_prepare_poly_alpha;
    }
    else
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.poly_alpha = poly_alpha;
    semantic_event.alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    semantic_event.fog_key = ndsRendererHardwareFogStateKey(stats);
#endif
    if (poly_alpha == 0u)
    {
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        semantic_event.outcome = NDS_RENDERER_SEMANTIC_ALPHA_ZERO;
        semantic_event.poly_fmt =
            ndsRendererHardwarePolyFmt(stats, poly_alpha);
        ndsRendererSemanticCommitEvent(&semantic_event);
#endif
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
    scale_world = (raw_submit != FALSE) ? TRUE : FALSE;
    if (state->texture_prepare_valid == 0u)
    {
        render_tile =
            &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        implicit_texture_on =
            ndsRendererHardwareTextureImplicitStateOn(stats);
        use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
            ndsRendererHardwareBindTexture(stats, config, state) : FALSE;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->texture_prepare_key_hash = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureKeyHash : 0u;
        state->texture_prepare_params = (use_texture != FALSE) ?
            sNdsRendererSemanticLastTextureParams : 0u;
#endif
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled =
            (use_texture != FALSE) ? TRUE : FALSE;
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
        state->texture_prepare_origin_s = render_tile->uls;
        state->texture_prepare_origin_t = render_tile->ult;
        state->texture_prepare_offset = texture_offset;
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
    }
    else
    {
        use_texture =
            (state->texture_prepare_enabled != 0u) ? TRUE : FALSE;
        ndsRendererProfileRecordTexturePrepareReuse();
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
#endif
    }
#if NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
    if (use_texture != FALSE)
    {
        state->texture_prepare_vertex_flags &=
            ~(NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL |
              NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX);
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
    poly_fmt = (state->texture_prepare_alpha_constant != 0u) ?
        state->texture_prepare_poly_fmt :
        ndsRendererHardwarePolyFmt(stats, poly_alpha);
    texture_name = state->texture_prepare_name;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.poly_fmt = poly_fmt;
    semantic_event.texture_name = texture_name;
    semantic_event.texture_key_hash = state->texture_prepare_key_hash;
    semantic_event.texture_params = state->texture_prepare_params;
    semantic_event.matrix_loaded = sNdsRendererHardwareMatrixLoaded;
    semantic_event.matrix_mode = sNdsRendererHardwareMatrixMode;
    semantic_event.matrix_generation =
        sNdsRendererHardwareMatrixGeneration;
    semantic_event.matrix_signature =
        sNdsRendererHardwareMatrixSignature;
    semantic_event.projected_z[0] = projected_z[0];
    semantic_event.projected_z[1] = projected_z[1];
    semantic_event.projected_z[2] = projected_z[2];
#endif
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        texture_name, poly_fmt, sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    state->texture_prepare_vertex_flags =
        (state->texture_prepare_vertex_flags &
         NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK) |
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.context_flags = state->texture_prepare_vertex_flags;
    semantic_event.zbuffered = (zbuffered != FALSE) ? TRUE : FALSE;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    vertex_submit_start = cpuGetTiming();
#endif
    ndsRendererHardwareSubmitVertex(stats, state, i0, projected_z[0]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[0]
#endif
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i1, projected_z[1]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[1]
#endif
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i2, projected_z[2]
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                                    , &semantic_event.vertex[2]
#endif
                                    );
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileVertexSubmitTicks +=
        cpuGetTiming() - vertex_submit_start;
#endif
    if (source_zbuffered != FALSE)
    {
        ndsRendererHardwareEnterProjectedForeground();
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    semantic_event.outcome = NDS_RENDERER_SEMANTIC_EMITTED;
    semantic_event.no_z_depth_after = sNdsRendererHardwareProjectedDepth;
    semantic_event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;
    ndsRendererSemanticCommitEvent(&semantic_event);
#endif

    sNdsRendererHardwareSubmitted = TRUE;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount++;
#endif
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
#endif

static inline void ndsRendererExecuteTriangleCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
    u32 triangle_submit_start;
#endif
#if !NDS_RENDERER_HW_TRIANGLES
    (void)config;
#endif

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->triangle_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->render_command_count++);
    if (op == NDS_RENDERER_OP_TRI1)
    {
        u32 packed = ndsGBIDecodeF3DEX2Tri1(w0);

        stats->triangle_count++;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererHardwareNoOracle == 0u)
#endif
        ndsRendererRecordTransformedTriangle(stats, state, packed);
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_tri2_half = 0u;
#endif
        ndsRendererSubmitHardwareTriangle(stats, config, state, packed);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    triangle_submit_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 0u;
#endif
    ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2First(w0));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->semantic_tri2_half = 1u;
#endif
    ndsRendererSubmitHardwareTriangle(
        stats, config, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileTriangleSubmitTicks +=
        cpuGetTiming() - triangle_submit_start;
#endif
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static inline s32 ndsRendererFastRawStateEligible(
    const NDSRendererTraversalState *state)
{
    const u32 required_flags =
        NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD |
        NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    const u32 forbidden_flags =
        NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH |
        NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

    return ((state != NULL) &&
            (state->texture_prepare_valid != 0u) &&
            (state->texture_prepare_source_zbuffered != 0u) &&
            (state->texture_prepare_decal_depth == 0u) &&
            (state->texture_prepare_prim_depth == 0u) &&
            (state->texture_prepare_alpha_constant != 0u) &&
            (state->texture_prepare_poly_alpha != 0u) &&
            ((state->texture_prepare_vertex_flags & required_flags) ==
             required_flags) &&
            ((state->texture_prepare_vertex_flags & forbidden_flags) == 0u) &&
            (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u) &&
            (sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation) &&
            (sNdsRendererHardwareTriangleBatchOpen != 0u) &&
            (sNdsRendererHardwareTriangleBatchTextured ==
             state->texture_prepare_enabled) &&
            (sNdsRendererHardwareTriangleBatchTextureName ==
             state->texture_prepare_name) &&
            (sNdsRendererHardwareTriangleBatchPolyFmt ==
             state->texture_prepare_poly_fmt) &&
            (sNdsRendererHardwareTriangleBatchMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
             state->matrix_generation)) ? TRUE : FALSE;
}

static inline s32 ndsRendererFastDecodeTriangle(
    u32 packed, u32 *indices, u32 *required_mask)
{
    u32 i0;
    u32 i1;
    u32 i2;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if ((i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    indices[0] = i0;
    indices[1] = i1;
    indices[2] = i2;
    *required_mask |= (1u << i0) | (1u << i1) | (1u << i2);
    return TRUE;
}

static u32 ndsRendererDirectRawPlanHash(const Gfx *source)
{
    uintptr_t value = (uintptr_t)source;

    return (u32)(((value >> 3) ^ (value >> 11)) &
                 (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u));
}

static NDSRendererDirectRawPlan *ndsRendererDirectRawFindPlan(
    const Gfx *source, u32 max_commands)
{
    NDSRendererDirectRawPlan *empty = NULL;
    u32 slot = ndsRendererDirectRawPlanHash(source);
    u32 probe;
    u32 command_count = 0u;
    u32 triangle_count = 0u;
    u32 entry_offset;
    u32 i;

    if ((source == NULL) || (max_commands == 0u))
    {
        return NULL;
    }
    for (probe = 0u; probe < NDS_RENDERER_DIRECT_RAW_PLAN_COUNT; probe++)
    {
        NDSRendererDirectRawPlan *plan =
            &sNdsRendererDirectRawPlans[
                (slot + probe) & (NDS_RENDERER_DIRECT_RAW_PLAN_COUNT - 1u)];

        if (plan->source == source)
        {
            const Gfx *last;

            if ((plan->command_count == 0u) ||
                (plan->command_count > max_commands))
            {
                return NULL;
            }
            last = source + plan->command_count - 1u;
            if ((source->words.w0 != plan->first_w0) ||
                (source->words.w1 != plan->first_w1) ||
                (last->words.w0 != plan->last_w0) ||
                (last->words.w1 != plan->last_w1))
            {
                return NULL;
            }
            return plan;
        }
        if (plan->source == NULL)
        {
            empty = plan;
            break;
        }
    }
    if (empty == NULL)
    {
        return NULL;
    }

    while (command_count < max_commands)
    {
        u32 op = source[command_count].words.w0 >> 24;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        triangle_count += (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        command_count++;
    }
    if ((command_count == 0u) ||
        (command_count > 0xffffu) ||
        (triangle_count > 0xffffu) ||
        ((sNdsRendererDirectRawEntryCount + command_count) >
         NDS_RENDERER_DIRECT_RAW_ENTRY_COUNT))
    {
        return NULL;
    }

    entry_offset = sNdsRendererDirectRawEntryCount;
    for (i = 0u; i < command_count; i++)
    {
        const Gfx *command = source + i;
        NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[entry_offset + i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 command_triangles =
            (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        u32 index;

        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (command_triangles == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        if ((ndsRendererFastDecodeTriangle(
                 packed[0], &indices[0], &required_mask) == FALSE) ||
            ((command_triangles == 2u) &&
             (ndsRendererFastDecodeTriangle(
                  packed[1], &indices[3], &required_mask) == FALSE)))
        {
            return NULL;
        }
        entry->required_mask = required_mask;
        entry->triangle_count = (u8)command_triangles;
        entry->reserved = 0u;
        for (index = 0u; index < (command_triangles * 3u); index++)
        {
            entry->indices[index] = (u8)indices[index];
        }
        for (; index < 6u; index++)
        {
            entry->indices[index] = 0u;
        }
    }

    sNdsRendererDirectRawEntryCount += command_count;
    empty->entry_offset = (u16)entry_offset;
    empty->command_count = (u16)command_count;
    empty->triangle_count = (u16)triangle_count;
    empty->reserved = 0u;
    empty->first_w0 = source->words.w0;
    empty->first_w1 = source->words.w1;
    empty->last_w0 = source[command_count - 1u].words.w0;
    empty->last_w1 = source[command_count - 1u].words.w1;
    empty->source = source;
    return empty;
}

static void NDS_RENDERER_FAST_RUN_CODE ndsRendererFastPrepareRawSlots(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 required_mask,
    u32 textured)
{
    u32 missing_color = required_mask &
        ~state->prepared_vertex_color_valid_mask;
    u32 context_flags = state->texture_prepare_vertex_flags;

    while (missing_color != 0u)
    {
        u32 vertex_index = (u32)__builtin_ctz(missing_color);
        u32 vertex_mask = 1u << vertex_index;
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];

        state->prepared_vertex_colors[vertex_index] =
            ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL,
                context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX,
                state->vertex_colors[vertex_index],
                ((state->vertex_color_valid_mask & vertex_mask) != 0u) ?
                    TRUE : FALSE);
        state->prepared_vertex_color_valid_mask |= vertex_mask;
        missing_color &= ~vertex_mask;
    }

    if (textured != 0u)
    {
        u32 missing_texcoord = required_mask &
            ~state->prepared_texcoord_valid_mask;

        while (missing_texcoord != 0u)
        {
            u32 vertex_index = (u32)__builtin_ctz(missing_texcoord);
            u32 vertex_mask = 1u << vertex_index;
            const NDSRendererInputVertex *vtx =
                &state->input_vertices[vertex_index];

            state->prepared_texcoord_s[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->s, state->texture_prepare_scale_s,
                    state->texture_prepare_origin_s,
                    state->texture_prepare_offset);
            state->prepared_texcoord_t[vertex_index] =
                ndsRendererHardwareTexCoord(
                    vtx->t, state->texture_prepare_scale_t,
                    state->texture_prepare_origin_t,
                    state->texture_prepare_offset);
            state->prepared_texcoord_valid_mask |= vertex_mask;
            missing_texcoord &= ~vertex_mask;
        }
    }
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererFastRawFallbackCommand(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 op,
    u32 w0,
    u32 w1)
{
    ndsRendererExecuteTriangleCommand(stats, config, state, op, w0, w1);
}

static inline void ndsRendererFastEmitRawUntexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawTexturedVertex(
    const NDSRendererTraversalState *state, u32 vertex_index)
{
    const NDSRendererInputVertex *vtx =
        &state->input_vertices[vertex_index];

    glColor(state->prepared_vertex_colors[vertex_index]);
    glTexCoord2t16(state->prepared_texcoord_s[vertex_index],
                  state->prepared_texcoord_t[vertex_index]);
    glVertex3v16(
        (v16)((s32)vtx->x * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->y * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))),
        (v16)((s32)vtx->z * (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT))));
}

static inline void ndsRendererFastEmitRawCommand(
    const NDSRendererTraversalState *state,
    const u32 *indices,
    u32 triangle_count,
    u32 textured)
{
    u32 triangle_index;

    if (textured != 0u)
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawTexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawTexturedVertex(state, triangle[2]);
        }
    }
    else
    {
        for (triangle_index = 0u;
             triangle_index < triangle_count;
             triangle_index++)
        {
            const u32 *triangle = &indices[triangle_index * 3u];

            ndsRendererFastEmitRawUntexturedVertex(state, triangle[0]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[1]);
            ndsRendererFastEmitRawUntexturedVertex(state, triangle[2]);
        }
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererFastCommitRawSemanticTriangle(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 packed,
    const u32 *indices)
{
    NDSRendererSemanticEvent event;
    u32 vertex_number;

    memset(&event, 0, sizeof(event));
    event.owner = (u32)sNdsRendererProfileOwner;
    event.owner_occurrence =
        sNdsRendererSemanticSourceProvenance.owner_occurrence;
    event.list_ordinal =
        sNdsRendererSemanticSourceProvenance.list_ordinal;
    event.branch_path = state->semantic_branch_path;
    event.command_index = state->semantic_command_index;
    event.tri2_half = state->semantic_tri2_half;
    event.outcome = NDS_RENDERER_SEMANTIC_EMITTED;
    event.packed = packed;
    event.submit_class =
        NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX;
    event.source_state_hash = ndsRendererSemanticSourceStateHash(stats);
    event.raw_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    event.context_flags = state->texture_prepare_vertex_flags;
    event.source_zbuffered = TRUE;
    event.zbuffered = TRUE;
    event.raw_submit = TRUE;
    event.poly_alpha = state->texture_prepare_poly_alpha;
    event.poly_fmt = state->texture_prepare_poly_fmt;
    event.alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    event.fog_key = ndsRendererHardwareFogStateKey(stats);
    event.fog_color = stats->fog_color;
    event.fog_min = stats->fog_min;
    event.fog_max = stats->fog_max;
    event.fog_status = stats->fog_status;
    event.texture_name = state->texture_prepare_name;
    event.texture_key_hash = state->texture_prepare_key_hash;
    event.texture_params = state->texture_prepare_params;
    event.matrix_loaded = sNdsRendererHardwareMatrixLoaded;
    event.matrix_mode = sNdsRendererHardwareMatrixMode;
    event.matrix_generation = sNdsRendererHardwareMatrixGeneration;
    event.matrix_signature = sNdsRendererHardwareMatrixSignature;
    event.no_z_depth_before = sNdsRendererHardwareProjectedDepth;
    event.no_z_depth_after = sNdsRendererHardwareProjectedDepth;
    event.no_z_background_before =
        sNdsRendererHardwareProjectedBackground;
    event.no_z_background_after =
        sNdsRendererHardwareProjectedBackground;

    ndsRendererHardwareRecordOracleTriangle(
        state, indices[0], indices[1], indices[2]);
    stats->hardware_oracle_triangle_count++;
    for (vertex_number = 0u; vertex_number < 3u; vertex_number++)
    {
        u32 vertex_index = indices[vertex_number];
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];
        NDSRendererSemanticVertex *vertex = &event.vertex[vertex_number];

        event.vertex_index[vertex_number] = vertex_index;
        event.vertex_matrix_snapshot[vertex_number] =
            state->vertex_matrix_snapshot[vertex_index];
        event.vertex_clip_snapshot[vertex_number] =
            state->vertex_clip_snapshot[vertex_index];
        vertex->x = (v16)((s32)vtx->x *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->y = (v16)((s32)vtx->y *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->z = (v16)((s32)vtx->z *
            (1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT)));
        vertex->color = state->prepared_vertex_colors[vertex_index];
        vertex->valid_flags =
            NDS_RENDERER_SEMANTIC_VERTEX_XYZ_VALID |
            NDS_RENDERER_SEMANTIC_VERTEX_COLOR_VALID;
        if (state->texture_prepare_enabled != 0u)
        {
            vertex->s = state->prepared_texcoord_s[vertex_index];
            vertex->t = state->prepared_texcoord_t[vertex_index];
            vertex->valid_flags |= NDS_RENDERER_SEMANTIC_VERTEX_ST_VALID;
            ndsRendererProfileTextureCoord(vertex->s, vertex->t);
            ndsRendererProfileTextureSample(vertex->s, vertex->t);
        }
        ndsRendererProfileVertexRange(
            vtx, vertex->x, vertex->y, vertex->z);
    }
    ndsRendererSemanticCommitEvent(&event);
}
#endif

static inline void ndsRendererFastAccountRawTriangles(
    NDSRendererStats *stats, u32 triangle_count)
{
    sNdsRendererHardwareSubmitClassCounts[
        NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] += triangle_count;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    {
        NDSRendererProfileOwnerHotLedger *owner =
            ndsRendererProfileCurrentOwner();

        if (owner != NULL)
        {
            owner->submit_class_count[
                NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX] +=
                    triangle_count;
        }
    }
    gNdsRendererProfileRawCurrentCandidateCount += triangle_count;
    gNdsRendererProfileHardwareBatchReuseCount += triangle_count;
    gNdsRendererProfileTexturePrepareReuseCount += triangle_count;
    gNdsRendererProfileHardwareTriangles += triangle_count;
    gNdsRendererProfileHardwareVertices += triangle_count * 3u;
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#else
    sNdsRendererRuntimeFrameSummary.raw_current_candidate_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_batch_reuse_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.texture_prepare_reuse_count +=
        triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_triangles += triangle_count;
    sNdsRendererRuntimeFrameSummary.hardware_vertices +=
        triangle_count * 3u;
#endif
    stats->hardware_triangle_count += triangle_count;
    stats->hardware_vertex_count += triangle_count * 3u;
    stats->hardware_zbuffer_triangle_count += triangle_count;
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    sNdsRendererBenchmarkTriangleCount += triangle_count;
#endif
    sNdsRendererHardwareSubmitted = TRUE;
}

static inline void ndsRendererFastEmitDirectRawEntry(
    const NDSRendererTraversalState *state,
    const NDSRendererDirectRawEntry *entry,
    u32 textured)
{
    u32 vertex_count = (u32)entry->triangle_count * 3u;
    u32 vertex_index;

    if (textured != 0u)
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawTexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
    else
    {
        for (vertex_index = 0u; vertex_index < vertex_count; vertex_index++)
        {
            ndsRendererFastEmitRawUntexturedVertex(
                state, entry->indices[vertex_index]);
        }
    }
}

static s32 NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteDirectRawRemainder(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    NDSRendererCommandCallback callback)
{
    const Gfx *source;
    NDSRendererDirectRawPlan *plan;
    u32 first_index;
    u32 remaining_commands;
    u32 available_mask;
    u32 required_mask = 0u;
    u32 entry_number;
    u32 textured;

    if ((callback != NULL) ||
        (ndsRendererFastRawStateEligible(state) == FALSE))
    {
        return FALSE;
    }
    first_index = *list_index_io + 1u;
    if ((first_index >= immutable_command_count) ||
        (first_index >= config->max_list_commands))
    {
        return FALSE;
    }
    remaining_commands = immutable_command_count - first_index;
    if (remaining_commands > (config->max_list_commands - first_index))
    {
        remaining_commands = config->max_list_commands - first_index;
    }
    source = *dl_io + 1;
    plan = ndsRendererDirectRawFindPlan(source, remaining_commands);
    if ((plan == NULL) ||
        ((stats->command_count + plan->command_count) > config->max_commands))
    {
        return FALSE;
    }

    available_mask = state->input_vertex_valid_mask &
        state->raw_vertex_fit_mask & state->current_transform_vertex_mask;
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        if ((available_mask & entry->required_mask) != entry->required_mask)
        {
            return FALSE;
        }
        required_mask |= entry->required_mask;
    }

    textured = state->texture_prepare_enabled;
    ndsRendererFastPrepareRawSlots(stats, state, required_mask, textured);
    for (entry_number = 0u;
         entry_number < plan->command_count;
         entry_number++)
    {
        const Gfx *command = source + entry_number;
        const NDSRendererDirectRawEntry *entry =
            &sNdsRendererDirectRawEntries[
                plan->entry_offset + entry_number];

        stats->command_count++;
        NDS_RENDERER_RECORD_PROOF_ONLY(
            stats->triangle_command_count++;
            stats->render_command_count++;
        );
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = first_index + entry_number;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        ndsRendererFastEmitDirectRawEntry(state, entry, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        {
            u32 w0 = command->words.w0;
            u32 w1 = command->words.w1;
            u32 packed[2];
            u32 triangle_index;

            packed[0] = (entry->triangle_count == 1u) ?
                ndsGBIDecodeF3DEX2Tri1(w0) :
                ndsGBIDecodeF3DEX2Tri2First(w0);
            packed[1] = (entry->triangle_count == 2u) ?
                ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
            for (triangle_index = 0u;
                 triangle_index < entry->triangle_count;
                 triangle_index++)
            {
                u32 indices[3];
                u32 base = triangle_index * 3u;

                indices[0] = entry->indices[base];
                indices[1] = entry->indices[base + 1u];
                indices[2] = entry->indices[base + 2u];
                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], indices);
            }
        }
#else
        (void)command;
#endif
    }

    stats->triangle_count += plan->triangle_count;
    ndsRendererFastAccountRawTriangles(stats, plan->triangle_count);
    sNdsRendererFastRunCount++;
    sNdsRendererFastTriangleCount += plan->triangle_count;
    if ((u32)sNdsRendererRuntimeOwner <
        (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        sNdsRendererFastOwnerTriangleCount[
            (u32)sNdsRendererRuntimeOwner] += plan->triangle_count;
    }
    *dl_io = source + plan->command_count - 1u;
    *list_index_io = first_index + plan->command_count - 1u;
    return TRUE;
}

static void NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteFastRawCurrentRun(
    const Gfx **dl_io,
    u32 *list_index_io,
    u32 immutable_command_count,
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 depth,
    NDSRendererCommandCallback callback,
    void *callback_user)
{
    const Gfx *dl = *dl_io;
    u32 list_index = *list_index_io;
    u32 fast_triangles = 0u;
    u32 fallback_state = 0u;
    u32 fallback_vertex = 0u;
    u32 fallback_command = 0u;
    u32 fast_command_count = 0u;

    if (ndsRendererExecuteDirectRawRemainder(
            dl_io, list_index_io, immutable_command_count,
            config, stats, state, callback) != FALSE)
    {
        return;
    }

    while (((list_index + 1u) < immutable_command_count) &&
           ((list_index + 1u) < config->max_list_commands))
    {
        const Gfx *next_dl = dl + 1;
        u32 w0 = next_dl->words.w0;
        u32 w1 = next_dl->words.w1;
        u32 op = w0 >> 24;
        u32 packed[2];
        u32 indices[6];
        u32 required_mask = 0u;
        u32 triangle_count;
        s32 decode_ok;
        s32 state_ok;
        s32 vertex_ok;
        NDSRendererCommand command;

        if ((op != NDS_RENDERER_OP_TRI1) &&
            (op != NDS_RENDERER_OP_TRI2))
        {
            break;
        }
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            break;
        }

        list_index++;
        dl = next_dl;
        stats->command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        state->semantic_command_index = list_index;
        state->semantic_tri2_half = 0u;
        sNdsRendererProfileTrustedCommandCount++;
        sNdsRendererProfileTriangleRunReuseCount++;
#endif
        if (callback != NULL)
        {
            memset(&command, 0, sizeof(command));
            command.dl = dl;
            command.w0 = w0;
            command.w1 = w1;
            command.op = op;
            command.depth = depth;
            command.list_index = list_index;
            command.transformed_vertices = state->vertices;
            command.transformed_vertex_valid_mask = state->vertex_valid_mask;
            command.matrix_valid = state->matrix_valid;
            if (callback(&command, callback_user) == FALSE)
            {
                ndsRendererRecordUnsupported(stats, op);
                stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
                break;
            }
        }
        triangle_count = (op == NDS_RENDERER_OP_TRI1) ? 1u : 2u;
        packed[0] = (op == NDS_RENDERER_OP_TRI1) ?
            ndsGBIDecodeF3DEX2Tri1(w0) :
            ndsGBIDecodeF3DEX2Tri2First(w0);
        packed[1] = (triangle_count == 2u) ?
            ndsGBIDecodeF3DEX2Tri2Second(w1) : 0u;
        decode_ok = ndsRendererFastDecodeTriangle(
            packed[0], &indices[0], &required_mask);
        if ((decode_ok != FALSE) && (triangle_count == 2u))
        {
            decode_ok = ndsRendererFastDecodeTriangle(
                packed[1], &indices[3], &required_mask);
        }
        state_ok = ndsRendererFastRawStateEligible(state);
        vertex_ok = ((decode_ok != FALSE) &&
                     ((state->input_vertex_valid_mask & required_mask) ==
                      required_mask) &&
                     ((state->raw_vertex_fit_mask & required_mask) ==
                      required_mask) &&
                     ((state->current_transform_vertex_mask & required_mask) ==
                      required_mask)) ? TRUE : FALSE;

        if ((state_ok != FALSE) && (vertex_ok != FALSE))
        {
            u32 textured = state->texture_prepare_enabled;
            u32 triangle_index;

            NDS_RENDERER_RECORD_PROOF_ONLY(
                stats->triangle_command_count++;
                stats->render_command_count++;
            );
            stats->triangle_count += triangle_count;
            ndsRendererFastPrepareRawSlots(
                stats, state, required_mask, textured);
            for (triangle_index = 0u;
                 triangle_index < triangle_count;
                 triangle_index++)
            {
                const u32 *triangle = &indices[triangle_index * 3u];

                if (sNdsRendererHardwareNoOracle == 0u)
                {
                    ndsRendererRecordTransformedTriangle(
                        stats, state, packed[triangle_index]);
                }
                ndsRendererFastEmitRawCommand(
                    state, triangle, 1u, textured);
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                state->semantic_tri2_half = triangle_index;
                ndsRendererFastCommitRawSemanticTriangle(
                    stats, state, packed[triangle_index], triangle);
#endif
            }
            fast_triangles += triangle_count;
            fast_command_count++;
        }
        else
        {
            if (decode_ok == FALSE)
            {
                fallback_command++;
            }
            else if (state_ok == FALSE)
            {
                fallback_state++;
            }
            else
            {
                fallback_vertex++;
            }
            ndsRendererFastRawFallbackCommand(
                stats, config, state, op, w0, w1);
        }
    }

    if (fast_triangles != 0u)
    {
        ndsRendererFastAccountRawTriangles(stats, fast_triangles);
        sNdsRendererFastRunCount++;
        sNdsRendererFastTriangleCount += fast_triangles;
        if ((u32)sNdsRendererRuntimeOwner <
            (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
        {
            sNdsRendererFastOwnerTriangleCount[
                (u32)sNdsRendererRuntimeOwner] += fast_triangles;
        }
    }
    (void)fast_command_count;
    sNdsRendererFastFallbackCount[0] += fallback_state;
    sNdsRendererFastFallbackCount[1] += fallback_vertex;
    sNdsRendererFastFallbackCount[2] += fallback_command;
    *dl_io = dl;
    *list_index_io = list_index;
}
#endif

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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
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
        state->source_command_site = dl;
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
        state->semantic_command_index = i;
        state->semantic_tri2_half = 0u;
#endif
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
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
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
#if NDS_RENDERER_HW_TRIANGLES
            /* Immutable adjacent TRI commands have no intervening source
             * state transition. Profile 0/1 has no command callback, so
             * replay the remainder of the run without rebuilding a generic
             * command record or re-entering the full opcode switch. */
#if NDS_RENDERER_PROFILE_LEVEL < 2
            if ((callback == NULL) && (sNdsRendererFastOwnerEnabled != 0u))
#else
            if (sNdsRendererFastOwnerEnabled != 0u)
#endif
            {
                ndsRendererExecuteFastRawCurrentRun(
                    &dl, &i, immutable_command_count,
                    config, stats, state, depth, callback, callback_user);
            }
#if NDS_RENDERER_PROFILE_LEVEL < 2
            else while ((callback == NULL) &&
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
                sNdsRendererProfileTrustedCommandCount++;
                sNdsRendererProfileTriangleRunReuseCount++;
#endif
                ndsRendererExecuteTriangleCommand(
                    stats, config, state, next_op, next_w0,
                    next_dl->words.w1);
            }
#endif
#endif
            break;
        }

        case NDS_RENDERER_OP_ENDDL:
            stats->end_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            return;

        case NDS_RENDERER_OP_DL:
        {
            const Gfx *raw_branch = command.raw_branch_dl;
            const Gfx *branch = command.resolved_branch_dl;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            u32 parent_branch_path = state->semantic_branch_path;
            u32 child_branch_path;
#endif

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
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
                child_branch_path = ndsRendererSemanticBranchPath(
                    parent_branch_path, i, depth + 1u, TRUE);
                state->semantic_branch_path = child_branch_path;
#endif
                ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                    callback, callback_user);
                return;
            }

            stats->branch_call_count++;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            child_branch_path = ndsRendererSemanticBranchPath(
                parent_branch_path, i, depth + 1u, FALSE);
            state->semantic_branch_path = child_branch_path;
#endif
            ndsRendererScanList(branch, config, stats, state, depth + 1u,
                                callback, callback_user);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
            state->semantic_branch_path = parent_branch_path;
#endif
            if (stats->blocker != NDS_RENDERER_BLOCKER_NONE)
            {
                return;
            }
            break;
        }

        case NDS_RENDERER_OP_TEXTURE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordTextureState(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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
            NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
            break;

        case NDS_RENDERER_OP_SPECIAL_1:
            ndsRendererApplyMvpRecalcCommand(stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_SETSCISSOR:
        case NDS_RENDERER_OP_SETCIMG:
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->ignored_state_command_count++;
            break;

        case NDS_RENDERER_OP_SETPRIMDEPTH:
            ndsRendererRecordPrimDepth(stats, w1);
            break;

        case NDS_RENDERER_OP_GEOMETRYMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            stats->geometry_mode = (stats->geometry_mode & w0) | w1;
            stats->geometry_clear_mask = w0;
            stats->geometry_set_mask = w1;
            stats->geometry_command_count++;
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETCOMBINE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetCombine(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTIMG:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetImage(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTILE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTile(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADBLOCK:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadBlock(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_LOADTLUT:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordLoadTlut(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETTILESIZE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
            ndsRendererRecordSetTileSize(stats, w0, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            break;

        case NDS_RENDERER_OP_SETFOGCOLOR:
            ndsRendererRecordFogColor(stats, w1);
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
            stats->color_command_count++;
            break;

        case NDS_RENDERER_OP_SETBLENDCOLOR:
        case NDS_RENDERER_OP_SETENVCOLOR:
        case NDS_RENDERER_OP_SETPRIMCOLOR:
            if (op != NDS_RENDERER_OP_SETBLENDCOLOR)
            {
                NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            }
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
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
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            stats->sync_command_count++;
            break;

        case NDS_RENDERER_OP_SETOTHERMODE_H:
        case NDS_RENDERER_OP_SETOTHERMODE_L:
        case NDS_RENDERER_OP_RDPSETOTHERMODE:
            NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE(state);
            ndsRendererTextureSourceHashCommand(stats, w0, w1);
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
#if NDS_RENDERER_HW_TRIANGLES
    u32 mode = gNdsRendererFastRunMode;

    sNdsRendererRuntimeOwner =
        ((u32)owner < (u32)NDS_RENDERER_PROFILE_OWNER_COUNT) ? owner :
        NDS_RENDERER_PROFILE_OWNER_NONE;
    sNdsRendererStageTextureSitesEnabled =
        ((mode == NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES) &&
         (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_STAGE)) ?
            TRUE : FALSE;
    sNdsRendererFastOwnerEnabled =
        ((mode == NDS_RENDERER_FAST_RUN_MARIO_ONLY) &&
         (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_MARIO)) ||
        ((mode == NDS_RENDERER_FAST_RUN_FIGHTERS) &&
         ((sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_MARIO) ||
          (sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_FOX))) ||
        (((mode == NDS_RENDERER_FAST_RUN_ALL_RAW_CURRENT) ||
          (mode == NDS_RENDERER_FAST_RUN_STAGE_TEXTURE_SITES)) &&
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
        hash = ndsRendererProfileHashU32(hash, 1u);
        hash = ndsRendererProfileHashU32(
            hash, (u32)sNdsRendererHardwareActiveTextureEntry->name);
        hash = ndsRendererProfileHashU32(
            hash, sNdsRendererHardwareActiveTextureEntry->ready);
        hash = ndsRendererProfileHashU32(
            hash, sNdsRendererHardwareActiveTextureEntry->params);
        hash = ndsRendererProfileHashU32(
            hash, ndsRendererProfileTextureKeyHashFull(
                &sNdsRendererHardwareActiveTextureEntry->key));
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

void ndsRendererProfileFrameBegin(void)
{
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
    memset(sNdsRendererBenchmarkSinkOwnerWords, 0,
           sizeof(sNdsRendererBenchmarkSinkOwnerWords));
#endif
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
    memset((void *)gNdsRendererProfileOwners, 0,
           sizeof(gNdsRendererProfileOwners));
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
#if NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE
    gNdsRendererBenchmarkTriangleCount =
        sNdsRendererBenchmarkTriangleCount;
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
    gNdsRendererBenchmarkSinkCursor =
        sNdsRendererBenchmarkSinkCursor;
    gNdsRendererBenchmarkSinkWordCount =
        sNdsRendererBenchmarkSinkWordCount;
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererHardwareMatrixSignature = 0u;
#endif
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
    ndsRendererExecuteDisplayListWithVertexCache(dl, config, callback,
                                                 callback_user, stats, NULL);
}
