/* Native renderer, materials and the texture cache: the RDP combiner and
 * blender classification (decal, alpha source, lit shade, packed colours) and
 * the texture cache's entries, keys, replacement and discard. A slice of
 * nds_renderer.c; its topic slices follow it there: texture_scene, particles,
 * world_quads, texture_convert, texture_resolve, submit, fighter_vram. */

#include <nds/nds_particle_runtime.h>
#include <nds/nds_preview_pack.h>

/* The dedicated I4 -> A5I3 owner is shared by the rebirth beam and Yoster's
 * clouds. Its preamble-owned identity only records the image and upload extent;
 * keep the source-row/sampler identity here so a masked 32x32 source baked into
 * a 64x64 render tile cannot be mistaken for a different sampling contract. */
static u32 sNdsRendererPrimRgbTexel0AlphaSourceWidth;
static u32 sNdsRendererPrimRgbTexel0AlphaSourceOrigin;
static u32 sNdsRendererPrimRgbTexel0AlphaSampler;
/* Bumped whenever the dedicated name is (re)prepared or released, and the
 * words its last bind applied: an owner texture memo may replay a bind of the
 * dedicated name while the generation holds (ndsRendererHardwareBindTexture-
 * OwnerMemo). */
static u32 sNdsRendererPrimRgbTexel0AlphaGen = 1u;
static u32 sNdsRendererPrimRgbTexel0AlphaLastParams;
static u32 sNdsRendererPrimRgbTexel0AlphaLastFormat;
static u32 sNdsRendererPrimRgbTexel0AlphaLastWidth;
static u32 sNdsRendererPrimRgbTexel0AlphaLastHeight;
__attribute__((used)) volatile u32 gNdsRendererPrimEnvMaskedBakeCount;

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

static s32 ndsRendererRoundShiftS32Signed(s32 value, u32 shift)
{
    u32 bits;
    u32 magnitude;
    u32 rounded;

    if (shift == 0u)
    {
        return value;
    }
    if (shift >= 32u)
    {
        return (s32)ndsRendererRoundShiftS64(value, shift);
    }

    /* Every target caller currently uses shift 8. Keep the helper generic for
     * its existing ABI, but stay in the input's natural 32-bit domain: the
     * largest magnitude is 0x80000000 and the largest legal bias below is
     * 0x40000000, so unsigned magnitude+bias cannot overflow for shifts 1..31.
     * This is the same nearest/half-away-from-zero rule as RoundShiftS64,
     * including INT_MIN, without promoting four fighter row-3 words to s64. */
    bits = (u32)value;
    magnitude = ((bits & 0x80000000u) != 0u) ? (0u - bits) : bits;
    rounded = (magnitude + (1u << (shift - 1u))) >> shift;
    return ((bits & 0x80000000u) != 0u) ? -(s32)rounded : (s32)rounded;
}

static s32 ndsRendererNativeStageVertexShift(s16 value, u32 shift)
{
    u32 magnitude;

    if (shift == 0u)
    {
        return value;
    }
    magnitude = (value < 0) ? (u32)-(s32)value : (u32)value;
    magnitude = (magnitude + (1u << (shift - 1u))) >> shift;
    return (value < 0) ? -(s32)magnitude : (s32)magnitude;
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

static inline v16 ndsRendererHardwareProjectToV16(
    s64 numerator, s32 denominator)
{
    s64 low_product;
    s64 high_product;
    v16 result;

    if (denominator == 0)
    {
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
    }
    else if (((denominator > 0) && (numerator > high_product)) ||
             ((denominator < 0) && (numerator < high_product)))
    {
        result = (v16)32767;
    }
    else
    {
#if defined(__arm__)
        /* ndsR2HwMathDiv64, not libnds's div64: identical DIV_64_32 sequence
         * minus the leading poll, which waits out a stale quotient nobody
         * reads. Graded bit-identical over 65,536 operands on four builds. */
        result = (v16)ndsR2HwMathDiv64(numerator, denominator);
#else
        result = ndsRendererHardwareClampS64ToV16(
            numerator / denominator);
#endif
    }

    return result;
}


static inline v16 ndsRendererHardwareSourceDepthToV16(
    s64 numerator, s32 denominator)
{
    v16 depth = ndsRendererHardwareProjectToV16(numerator, denominator);

    /* Reserve 128 strictly ordered v16 depths at each endpoint for no-Z
     * painter primitives.  Canonical source Z is already inside this central
     * range; the clamp only prevents camera extremes from entering a painter
     * band in the DS's otherwise shared depth channel. */
    if (depth < NDS_RENDERER_HW_SOURCE_DEPTH_MIN)
    {
        return (v16)NDS_RENDERER_HW_SOURCE_DEPTH_MIN;
    }
    if (depth > NDS_RENDERER_HW_SOURCE_DEPTH_MAX)
    {
        return (v16)NDS_RENDERER_HW_SOURCE_DEPTH_MAX;
    }
    return depth;
}

static void ndsRendererProfileVertexRange(
    const NDSRendererInputVertex *vtx, v16 x, v16 y, v16 z)
{
    (void)vtx;
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}

static void ndsRendererProfileHWVertexRange(v16 x, v16 y, v16 z)
{
    if ((x == (v16)32767) || (x == (v16)-32768) ||
        (y == (v16)32767) || (y == (v16)-32768) ||
        (z == (v16)32767) || (z == (v16)-32768))
    {
        ndsRendererProfileRecordVertexSaturate();
    }
}


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

/* A 2-CYCLE LIST WHOSE SECOND CYCLE ONLY MODULATES BY SHADE.
 *
 * The classifier below reads the cycle-1 slot on a 2-cycle list, because that
 * is the slot that produces the pixel. Yoster's cloud breaks that assumption:
 * its cycle 0 is the BLENDPE lerp with alpha TEXEL0 * PRIM, and its cycle 1 is
 * `(COMBINED - 0) * SHADE + 0` for colour with alpha `COMBINED` -- it multiplies
 * the first cycle by the vertex colour and passes its alpha through untouched.
 * Reading cycle 1 therefore saw a shade modulate, classified NONE, and the
 * clouds took the one-bit I4 conversion while the dedicated graded-alpha
 * upload sat unreachable (2026-09-07: prepare counter 0 on two cameras with
 * the cloud executor running; review agents-0906/review_cloud_alpha).
 *
 * The DS applies vertex colour as a modulate over the bound texture, so the
 * second cycle costs nothing to reproduce: bake cycle 0 into the texture and
 * the hardware performs the shade multiply. That is only true for THIS second
 * cycle, so the test is exact rather than a loosened gate -- any other cycle 1
 * still decodes as itself. */
static sb32 ndsRendererHardwareSecondCycleIsShadePassThrough(u32 w0, u32 w1)
{
    return ((((w0 >> 5) & 0x0fu) == NDS_RENDERER_CCMUX_COMBINED) &&
            (((w1 >> 24) & 0x0fu) == NDS_RENDERER_CCMUX_ZERO_AB) &&
            ((w0 & 0x1fu) == NDS_RENDERER_CCMUX_SHADE) &&
            (((w1 >> 6) & 0x07u) == NDS_RENDERER_CCMUX_ZERO_D) &&
            (((w1 >> 21) & 0x07u) == NDS_RENDERER_ACMUX_0) &&
            (((w1 >> 3) & 0x07u) == NDS_RENDERER_ACMUX_0) &&
            (((w1 >> 18) & 0x07u) == NDS_RENDERER_ACMUX_0) &&
            ((w1 & 0x07u) == NDS_RENDERER_ACMUX_COMBINED)) ? TRUE : FALSE;
}

static u32 ndsRendererHardwarePrimEnvTexel0BlendMode(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;
    u32 color_a;
    u32 color_b;
    u32 color_c;
    u32 color_d;
    u32 alpha_a;
    u32 alpha_b;
    u32 alpha_c;
    u32 alpha_d;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return NDS_RENDERER_PRIM_ENV_BLEND_NONE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if ((ndsRendererHardwareUseSecondCycle(stats) != FALSE) &&
        (ndsRendererHardwareSecondCycleIsShadePassThrough(w0, w1) == FALSE))
    {
        color_a = (w0 >> 5) & 0x0fu;
        color_b = (w1 >> 24) & 0x0fu;
        color_c = w0 & 0x1fu;
        color_d = (w1 >> 6) & 0x07u;
        alpha_a = (w1 >> 21) & 0x07u;
        alpha_b = (w1 >> 3) & 0x07u;
        alpha_c = (w1 >> 18) & 0x07u;
        alpha_d = w1 & 0x07u;
    }
    else
    {
        color_a = (w0 >> 20) & 0x0fu;
        color_b = (w1 >> 28) & 0x0fu;
        color_c = (w0 >> 15) & 0x1fu;
        color_d = (w1 >> 15) & 0x07u;
        alpha_a = (w0 >> 12) & 0x07u;
        alpha_b = (w1 >> 12) & 0x07u;
        alpha_c = (w0 >> 9) & 0x07u;
        alpha_d = (w1 >> 9) & 0x07u;
    }
    /* BattleShip's G_CC_BLENDPE family is a texture-weighted endpoint lerp:
     * (PRIMITIVE - ENVIRONMENT) * TEXEL0 + ENVIRONMENT. Decode the active
     * cycle so both raw forms remain data-independent: one multiplies source
     * coverage by primitive alpha, while the other passes source alpha. */
    if ((color_a == NDS_RENDERER_CCMUX_PRIMITIVE) &&
        (color_b == NDS_RENDERER_CCMUX_ENVIRONMENT) &&
        (color_c == NDS_RENDERER_CCMUX_TEXEL0) &&
        (color_d == NDS_RENDERER_CCMUX_ENVIRONMENT))
    {
        if ((alpha_a == NDS_RENDERER_ACMUX_TEXEL0) &&
            (alpha_b == NDS_RENDERER_ACMUX_0) &&
            (alpha_c == NDS_RENDERER_ACMUX_PRIMITIVE) &&
            (alpha_d == NDS_RENDERER_ACMUX_0))
        {
            return NDS_RENDERER_PRIM_ENV_BLEND_PRIM_ALPHA;
        }
        if ((alpha_a == NDS_RENDERER_ACMUX_0) &&
            (alpha_b == NDS_RENDERER_ACMUX_0) &&
            (alpha_c == NDS_RENDERER_ACMUX_0) &&
            (alpha_d == NDS_RENDERER_ACMUX_TEXEL0))
        {
            return NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA;
        }
        return NDS_RENDERER_PRIM_ENV_BLEND_NONE;
    }
    /* THE DEGENERATE CONSTANT FORM, and it is a separate branch rather than a
     * loosened gate above. (0,0,0,PRIMITIVE) for colour multiplies nothing --
     * it selects PRIM flat -- while (0,0,0,TEXEL0) for alpha passes source
     * coverage through untouched. The lerp gate cannot be widened to admit it
     * without also admitting every other constant-d combine in the game. */
    if ((color_a == NDS_RENDERER_CCMUX_ZERO_AB) &&
        (color_b == NDS_RENDERER_CCMUX_ZERO_AB) &&
        (color_c == NDS_RENDERER_CCMUX_ZERO_C) &&
        (color_d == NDS_RENDERER_CCMUX_PRIMITIVE) &&
        (alpha_a == NDS_RENDERER_ACMUX_0) &&
        (alpha_b == NDS_RENDERER_ACMUX_0) &&
        (alpha_c == NDS_RENDERER_ACMUX_0) &&
        (alpha_d == NDS_RENDERER_ACMUX_TEXEL0))
    {
        return NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA;
    }
    return NDS_RENDERER_PRIM_ENV_BLEND_NONE;
}

/* TRUE when the recognised combine's ALPHA mux is (0,0,0,TEXEL0): source
 * coverage passes through untouched, and the bake has already placed it in the
 * texel's alpha bit.
 *
 * SOURCE_ALPHA and PRIM_RGB_TEXEL0_ALPHA differ ONLY in their colour mux --
 * their alpha muxes are byte-identical -- so every alpha-side decision has to
 * treat them the same. Keying an alpha site on one mode alone would let the
 * other multiply vertex alpha into coverage the texel already carries. The two
 * call sites below are both unreachable for the rebirth-halo beam (its
 * othermode_l 0x552078 takes their opaque early return first), so this closes a
 * latent asymmetry rather than fixing a measured defect. */
/* Set by an owner around a draw whose combine reads TEXEL0 alpha from an I
 * tile outside the recognised bakes above: rgb = TEXEL0 * SHADE with alpha
 * TEXEL0 * PRIM, Koffing's smog (generate_nds_native_item_baked.py marks the
 * group). ndsRendererHardwareConvertI forces every I texel opaque, which is
 * right for the combines that ignore texel alpha and drew that smog as solid
 * black-and-yellow squares; with this set an I4/I8 tile bakes its intensity
 * as GL_RGB8_A5 coverage over a grey ramp, and the DS modulates the grey by
 * the vertex colour and the coverage by the polygon's PRIM alpha, as the
 * combine does. An owner request rather than a combine rule, so no other
 * surface's texture key or static corpus entry moves (2026-09-30). */
static u32 sNdsRendererHardwareIntensityCoverage;
/* Set by the native fighter prepare around its texture resolve. An IA8 tile
 * whose alpha the blender reads (no alpha compare, no PRIM/ENV bake) converts
 * through ndsRendererHardwareConvertIA with one-bit alpha, so a mask whose
 * alpha is graded drew as a solid shape: Meta Knight's Mach Tornado (P4,
 * 64x64 IA8, alpha 5-15 of 15 over most of the swirl) was an opaque cone.
 * With this set such a tile bakes as GL_RGB8_A5: the nibble as five-bit
 * alpha, the intensity as an eight-step grey the vertex colour modulates.
 * A request rather than a combine rule, so effect, item and stage keys and
 * the static corpus stay as they are; BattleShip's own fighters carry no IA
 * image (fighters/admission.bin is all CI), so only P4 models reach it. */
static u32 sNdsRendererHardwareTexelAlphaGraded;

static s32 ndsRendererHardwareBlendModeKeepsTexelCoverage(u32 mode)
{
    return ((mode == NDS_RENDERER_PRIM_ENV_BLEND_SOURCE_ALPHA) ||
            (mode == NDS_RENDERER_PRIM_ENV_BLEND_PRIM_RGB_TEXEL0_ALPHA)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareUsesTexel01Lerp(
    const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;
    u32 second_alpha_c;

    if ((stats == NULL) ||
        (stats->texture_combine_count == 0u) ||
        (ndsRendererHardwareUseSecondCycle(stats) == FALSE))
    {
        return FALSE;
    }

    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    second_alpha_c = (w1 >> 18) & 0x07u;
    /* BattleShip's animated Pupupu water uses G_CC_TEMPLERP for color and
     * alpha, followed by COMBINED * SHADE / COMBINED * PRIMITIVE.
     * Decode the semantic mux rather than keying this DS adaptation to a
     * stage address or raw display-list pointer. Planet Zebes acid uses the
     * same exact first-cycle TEMPLERP word but its final alpha is
     * COMBINED * SHADE, so admit that alpha mux only for FC272C04. */
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
            ((second_alpha_c == NDS_RENDERER_ACMUX_PRIMITIVE) ||
             ((w0 == 0xfc272c04u) &&
              (second_alpha_c == NDS_RENDERER_ACMUX_SHADE))) &&
            (((w1 >> 0) & 0x07u) == NDS_RENDERER_ACMUX_0)) ? TRUE : FALSE;
}

__attribute__((used)) volatile u32 gNdsRendererZebesAcidBindCount;
__attribute__((used)) volatile u32 gNdsRendererZebesAcidWantsTexel1TrueCount;
__attribute__((used)) volatile u32 gNdsRendererZebesAcidWantsTexel1FalseCount;

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

/* Whether the RDP's blender reads the combiner's alpha: coverage times alpha
 * (CVG_X_ALPHA), the translucent z mode, or a forced blend whose alpha is not
 * the coverage (FORCE_BL without ALPHA_CVG_SEL, 0x2000). ALPHA_CVG_SEL alone
 * is every antialiased opaque mode (AA_ZB_OPA_SURF, the fighters'
 * AA_ZB_OPA_SURF2): its blender alpha is the coverage, so those modes draw
 * every texel whatever its alpha, and the pinned stage and fighter keys
 * (generate_battle_playable_static_textures.py) keep the C/D test for them. */
static s32 ndsRendererHardwareBlenderReadsAlpha(const NDSRendererStats *stats)
{
    const u32 mode = stats->othermode_l;

    return (((mode & NDS_RENDERER_CVG_X_ALPHA) != 0u) ||
            ((mode & NDS_RENDERER_ZMODE_MASK) == NDS_RENDERER_ZMODE_XLU) ||
            (((mode & NDS_RENDERER_FORCE_BL) != 0u) &&
             ((mode & 0x00002000u) == 0u))) ?
        TRUE : FALSE;
}

/* One cycle's alpha (A - B) * C + D reads `source`: in C or D, or in A or B
 * while C is not zero. */
static s32 ndsRendererCombineCycleAlphaReads(u32 a, u32 b, u32 c, u32 d, u32 source)
{
    return ((c == source) || (d == source) ||
            (((a == source) || (b == source)) && (c != NDS_RENDERER_ACMUX_0))) ? TRUE : FALSE;
}

/* The combiner's output alpha reads `source` through any slot, following
 * COMBINED from the second cycle into the first (COMBINED is 0 in the A, B and
 * D slots; C's 0 is LOD_FRACTION). ndsRendererHardwareOutputUsesAlpha below
 * reads only the C and D slots, on purpose for opaque surfaces. */
static s32 ndsRendererHardwareOutputReadsAlphaFull(const NDSRendererStats *stats,
                                                   u32 source)
{
    const u32 w0 = stats->texture_combine_w0;
    const u32 w1 = stats->texture_combine_w1;
    const u32 a0 = (w0 >> 12) & 0x07u;
    const u32 c0 = (w0 >> 9) & 0x07u;
    const u32 b0 = (w1 >> 12) & 0x07u;
    const u32 d0 = (w1 >> 9) & 0x07u;
    const u32 a1 = (w1 >> 21) & 0x07u;
    const u32 c1 = (w1 >> 18) & 0x07u;
    const u32 b1 = (w1 >> 3) & 0x07u;
    const u32 d1 = w1 & 0x07u;

    if (stats->texture_combine_count == 0u)
    {
        return FALSE;
    }
    if (ndsRendererHardwareUseSecondCycle(stats) == FALSE)
    {
        return ndsRendererCombineCycleAlphaReads(a0, b0, c0, d0, source);
    }
    if (ndsRendererCombineCycleAlphaReads(a1, b1, c1, d1, source) != FALSE)
    {
        return TRUE;
    }
    return (((((a1 == NDS_RENDERER_ACMUX_COMBINED) || (b1 == NDS_RENDERER_ACMUX_COMBINED)) &&
              (c1 != NDS_RENDERER_ACMUX_0)) ||
             (d1 == NDS_RENDERER_ACMUX_COMBINED)) &&
            (ndsRendererCombineCycleAlphaReads(a0, b0, c0, d0, source) != FALSE)) ?
        TRUE : FALSE;
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

/* b == d IS NOT A DECAL WHEN THE TEXTURE IS THE WEIGHT.
 *
 * The `b0 == d0` test is sm64-nds's g_setcombine rule, transcribed whole
 * (decomp/sm64-nds/src/nds/nds_renderer.c:928): it forces POLY_DECAL, and when
 * `a0` is PRIMITIVE it additionally sets `use_texture = false`. Both halves are
 * wrong for `(a - b) * TEXEL0 + b`, which is not a decal at all but a LERP from
 * b to a with the texture as its weight -- dropping the texture collapses it to
 * the flat colour b, and DS decal mode replaces the texel-weighted blend with
 * "texel where opaque, polygon colour where not".
 *
 * SSB64 writes exactly that shape as G_CC_BLENDPE -- (PRIMITIVE - ENVIRONMENT)
 * * TEXEL0 + ENVIRONMENT -- and it is the standard translucent-effect combine,
 * not one asset's quirk: the same two word pairs (0xFC309661/0x552EFF7F and
 * 0xFC30FE61/0x55FEF379) appear in FTManagerCommon (the shield),
 * EFCommonEffects1/2/3 (rebirth halo, impact wave), FoxSpecial3 and
 * ITCommonObject. So the shield's 16x32 IA8 circular alpha never reached its
 * polygon: gNdsEffectRendererTextureReadyCount AND TextureRejectCount both read
 * 0, because ndsRendererHardwareUseTexture refused before any bind was
 * attempted, and the quad drew as a flat env-coloured SQUARE.
 *
 * Requiring the multiplier to be TEXEL0/TEXEL1 keeps every genuinely untextured
 * decal (c = SHADE, PRIM_A, TEXEL0_A, LOD_FRAC, ...) on the sm64-nds rule. The
 * relocData corpus was censused for the blast radius: 284 of the 3,210 b0 == d0
 * combines in 2,132 files change, and inside the P1 battle only FTManagerCommon,
 * EFCommonEffects1/2/3, FoxSpecial3 and ITCommonObject carry one -- the Mario
 * and Fox models and the Dream Land stage carry none. */
static s32 ndsRendererHardwareUseDecal(const NDSRendererStats *stats)
{
    u32 w0;
    u32 w1;
    u32 multiplier;

    if ((stats == NULL) || (stats->texture_combine_count == 0u))
    {
        return FALSE;
    }
    w0 = stats->texture_combine_w0;
    w1 = stats->texture_combine_w1;
    if (((w1 >> 28) & 0x0fu) != ((w1 >> 15) & 0x07u))
    {
        return FALSE;
    }
    multiplier = (w0 >> 15) & 0x1fu;
    return (((multiplier == NDS_RENDERER_CCMUX_TEXEL0) ||
             (multiplier == NDS_RENDERER_CCMUX_TEXEL1)) ? FALSE : TRUE);
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
    (void)stats;
    (void)reason;
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
    if (ndsRendererHardwarePrimEnvTexel0BlendMode(stats) !=
        NDS_RENDERER_PRIM_ENV_BLEND_NONE)
    {
        /* The converted texture already contains both captured endpoints. */
        return 0xffffffffu;
    }
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
        if (ndsRendererHardwareBlendModeKeepsTexelCoverage(
                ndsRendererHardwarePrimEnvTexel0BlendMode(stats)) != FALSE)
        {
            /* GL_RGBA retains TEXEL0 coverage; do not multiply vertex alpha. */
            return 31u;
        }
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
        if (ndsRendererHardwareBlendModeKeepsTexelCoverage(
                ndsRendererHardwarePrimEnvTexel0BlendMode(stats)) != FALSE)
        {
            return FALSE;
        }
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

/* Task 37: 100 bytes, 3.60 cycles per instruction, on the per-polygon submit
 * path. */
static u32 NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE ndsRendererHardwarePolyFmt(
    const NDSRendererStats *stats, u32 alpha)
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

static s32 ndsRendererHardwareTextureFilterOffsetForSourceFrame(
    const NDSRendererStats *stats, s32 live_source_frame)
{
    if ((stats != NULL) &&
        ((stats->othermode_h & NDS_RENDERER_TEXTFILT_MASK) !=
         NDS_RENDERER_TF_POINT))
    {
        /* DS texture coordinates are 12.4 and the hardware samples a point.
         * A live source-frame texture therefore approximates the RDP's
         * non-point filter by rounding to the nearest source texel: +8 is a
         * half-texel bias. +16 advances one complete texel and skips the texel
         * at an integer source coordinate. The linked/pinned representation
         * keeps its established phase because its accepted textures were
         * authored and verified against that path. */
        return (live_source_frame != FALSE) ?
            (NDS_RENDERER_TEXCOORD_FILTER_OFFSET >> 1) :
            NDS_RENDERER_TEXCOORD_FILTER_OFFSET;
    }
    return 0;
}

static s32 ndsRendererHardwareTextureFilterOffset(
    const NDSRendererStats *stats)
{
    return ndsRendererHardwareTextureFilterOffsetForSourceFrame(stats, FALSE);
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


static u32 ndsRendererHardwareColorByte(u32 color, u32 shift)
{
    return (color >> shift) & 0xffu;
}

static inline u32 ndsRendererHardwareScaleMaterialChannel5(
    u32 shaded, u32 material)
{
    u32 numerator = (shaded * material) + 127u;

    /* For every 16-bit numerator, floor(n / 255) is exactly
     * (n + 1 + (n >> 8)) >> 8.  Fold the following RGB15 divide by eight
     * into the same shift so the native owner does not issue three wide
     * constant divisions for every prepared vertex. */
    return (numerator + 1u + (numerator >> 8)) >> 11;
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

static const u32 *
ndsRendererHardwareFindLightShadeLut(u32 diffuse, u32 ambient)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT; i++)
    {
        const NDSRendererHardwareLightShadeCacheEntry *entry =
            &sNdsRendererHardwareLightShadeCache[i];

        if ((entry->valid != 0u) &&
            (entry->diffuse == diffuse) &&
            (entry->ambient == ambient))
        {
            return entry->rgb;
        }
    }
    return NULL;
}

static const u32 * __attribute__((noinline))
ndsRendererHardwareGetLightShadeLut(u32 diffuse, u32 ambient)
{
    NDSRendererHardwareLightShadeCacheEntry *entry;
    const u32 *resident;
    u32 i;

    /* Cache only the exact RGB function of the two source light colors and
     * diffuse numerator. Vertex normals, direction, and alpha stay live. */
    resident = ndsRendererHardwareFindLightShadeLut(diffuse, ambient);
    if (resident != NULL)
    {
        return resident;
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

static u32 ndsRendererHardwareLightColor(NDSRendererStats *stats, u32 mask,
                                         u32 color, u32 fallback)
{
    if ((stats == NULL) || ((stats->light_color_mask & mask) == 0u))
    {
        if (stats != NULL)
        {
            stats->light_fallback_count++;
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
    return ndsRendererHardwareDivideLitDotBy127((u32)dot);
}


static u32 NDS_R2_ITCM_PACK2_EVICTED_CODE
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

static inline u16 ndsRendererHardwareModulatePackedColor(
    u16 color, u32 color_modulate)
{
    u32 alpha = color_modulate & 0xffu;

    if (alpha != 0u)
    {
        u32 inverse = 255u - alpha;
        u32 red = (((u32)(color & 0x1fu) * inverse) +
                   (((color_modulate >> 27) & 0x1fu) * alpha) + 127u) /
                  255u;
        u32 green = (((u32)((color >> 5) & 0x1fu) * inverse) +
                     (((color_modulate >> 19) & 0x1fu) * alpha) + 127u) /
                    255u;
        u32 blue = (((u32)((color >> 10) & 0x1fu) * inverse) +
                    (((color_modulate >> 11) & 0x1fu) * alpha) + 127u) /
                   255u;

        color = RGB15(red, green, blue);
    }
    return color;
}


static inline u16 ndsRendererHardwarePackedResolvedColor(
    u32 color,
    u32 material_color,
    s32 use_material_color,
    u32 color_modulate)
{
    u16 packed;

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

        packed = RGB15((u8)(r >> 3), (u8)(g >> 3), (u8)(b >> 3));
    }
    else
    {
        packed = RGB15((u8)((color >> 27) & 0x1fu),
                       (u8)((color >> 19) & 0x1fu),
                       (u8)((color >> 11) & 0x1fu));
    }
    return ndsRendererHardwareModulatePackedColor(packed,
                                                   color_modulate);
}

static inline u16 ndsRendererHardwarePackedValidVertexColor(
    u32 material_color,
    s32 use_material_color,
    s32 use_vertex_color,
    u32 vertex_color,
    u32 color_modulate)
{
    if ((use_material_color != FALSE) && (use_vertex_color == FALSE))
    {
        u16 packed = ndsRendererHardwareModulatePackedColor(
            RGB15((u8)((material_color >> 27) & 0x1fu),
                  (u8)((material_color >> 19) & 0x1fu),
                  (u8)((material_color >> 11) & 0x1fu)),
            color_modulate);

        return packed;
    }
    if (use_vertex_color == FALSE)
    {
        u16 packed = ndsRendererHardwareModulatePackedColor(
            RGB15(31u, 31u, 31u), color_modulate);

        return packed;
    }
    return ndsRendererHardwarePackedResolvedColor(
        vertex_color, material_color, use_material_color,
        color_modulate);
}

static u16 ndsRendererHardwarePackedVertexColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    u32 material_color,
    s32 use_material_color,
    s32 use_vertex_color,
    u32 vertex_color,
    s32 vertex_color_valid,
    u32 color_modulate)
{
    u32 color;

    if (vertex_color_valid != FALSE)
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color, color_modulate);
    }
    if ((use_material_color != FALSE) && (use_vertex_color == FALSE))
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color, color_modulate);
    }
    if (use_vertex_color == FALSE)
    {
        return ndsRendererHardwarePackedValidVertexColor(
            material_color, use_material_color,
            use_vertex_color, vertex_color, color_modulate);
    }
    color = ndsRendererHardwareLitShadeColor(stats, vtx, NULL);
    return ndsRendererHardwarePackedResolvedColor(
        color, material_color, use_material_color, color_modulate);
}

/* reloc_preview_pack.c: a P4 battle pack's half-resolution material frame,
 * repeated to its full size in the pack's scratch; anything else unchanged. */
const void *ndsRelocPreviewHalfResSource(const void *ptr);

static const void *ndsRendererResolveTextureDataPointer(
    const NDSRendererConfig *config, const void *ptr, size_t bytes)
{
    if (ptr == NULL)
    {
        return NULL;
    }
    if ((config != NULL) && (config->resolve_data != NULL))
    {
        ptr = config->resolve_data(ptr, bytes, config->user);
    }
    else if ((config != NULL) && (config->validate_range != NULL) &&
             (config->validate_range((const Gfx *)ptr, bytes, config->user) ==
              FALSE))
    {
        return NULL;
    }
    ptr = ndsRelocPreviewHalfResSource(ptr);
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
             * over the complete timed match. The table is twice the cache size,
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


static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexture(const NDSRendererHardwareTextureKey *key,
                               u32 key_hash)
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 slot;
    u32 probe;

    if (key == NULL)
    {
        return NULL;
    }
    sNdsRendererRuntimeFrameSummary.texture_lookup_call_count++;
    entry = (NDSRendererHardwareTextureCacheEntry *)
        sNdsRendererHardwareActiveTextureEntry;
    if ((entry != NULL) && (entry->ready != 0u) &&
        (entry->key_hash == key_hash) &&
        (ndsRendererHardwareEntryKeyEqual(entry, key) != FALSE))
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
            (ndsRendererHardwareEntryKeyEqual(entry, key) != FALSE))
        {
            sNdsRendererRuntimeFrameSummary
                .texture_lookup_table_hit_count++;
            return entry;
        }
        slot = (slot + 1u) & (NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT - 1u);
    }
    sNdsRendererRuntimeFrameSummary.texture_lookup_miss_count++;
    return NULL;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindStageSourceFrameTexture(
    const NDSRendererHardwareTextureKey *key)
{
    u32 id_a;
    u32 id_b;
    u32 i;

    if (key == NULL)
    {
        return NULL;
    }
    /* Every word but the image: the identity's own cut (slice 2c). */
    ndsRendererHardwareTextureIdentOf(key, &id_a, &id_b);
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        const NDSRendererHardwareTextureIdent *ident;

        if ((entry->ready == 0u) || (entry->pinned != 0u) ||
            (entry->admitted != 0u))
        {
            continue;
        }
        ident = ndsRendererHardwareEntryIdent(entry);
        if (ident == NULL)
        {
            NDSRendererHardwareTextureKey source_frame;

            ndsRendererHardwareEntryCopyStaticKey(entry, &source_frame);
            source_frame.image = key->image;
            if (ndsRendererHardwareTextureKeyEqual(&source_frame, key) !=
                FALSE)
            {
                return entry;
            }
            continue;
        }
        if ((ident->id_a == id_a) && (ident->id_b == id_b) &&
            (ident->tlut_image == key->tlut_image) &&
            (ident->texel1_image == key->texel1_image))
        {
            return entry;
        }
    }
    return NULL;
}

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareFindTexel1RefreshTexture(
    const NDSRendererHardwareTextureKey *key)
{
    u32 i;

    u32 refresh;

    if ((key == NULL) || (key->texel1_image == 0u))
    {
        return NULL;
    }
    /* Texel1RefreshCompatible's seven words, as the identity's class. */
    refresh = ndsRendererHardwareTextureRefreshClass(key);
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        /* Only ever a dynamic slot: this refuses pinned entries, and the static
         * partition holds nothing else. */
        const NDSRendererHardwareTextureIdent *resident =
            ndsRendererHardwareEntryIdent(entry);

        if ((resident != NULL) && (entry->ready != 0u) &&
            (entry->pinned == 0u) && (entry->admitted == 0u) &&
            (resident->texel1_image != 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)) &&
            (resident->refresh == refresh))
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
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH);
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
    /* DMA 3, never 0 (2026-10-18). Channel 0 is the geometry FIFO's: the
     * fighter replay and the stage program leave their FIFO transfer running
     * for the next writer to wait on (NDS_GX_DMA0_WAIT), and a copy started
     * on channel 0 meanwhile rewrote that transfer's registers mid-list. The
     * engine was left waiting on a command's missing parameters, and the next
     * CPU write to a full FIFO stalled the ARM9 on the bus, IRQs and the
     * freeze watchdog with it: the owner's silent host freezes (music still
     * playing), and the 10/07 report's DMA0 that never finished. Channel 3's
     * other users (the menu framebuffer and UI kit copies) are synchronous. */
    if (row_map == NULL)
    {
        for (offset = 0u; offset < texture_bytes; offset += staged_bytes)
        {
            dmaCopyWords(3, texture, (u8 *)vram_address + offset,
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
            dmaCopyWords(3, (const u8 *)texture + (source_row * row_bytes),
                         (u8 *)vram_address + (i * row_bytes),
                         run * row_bytes);
            i += run;
        }
    }
    vramRestorePrimaryBanks(vram_state);
    return TRUE;
}

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
    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_REPLACE_REFRESH);
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
    return TRUE;
}

u32 ndsRendererHardwareCommitPendingTextureRefreshes(void)
{
    u32 committed = 0u;
    u32 i;

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
        refresh->entry = NULL;
        refresh->pixels = NULL;
        refresh->staged_bytes = 0u;
        refresh->texture_bytes = 0u;
        refresh->row_bytes = 0u;
        refresh->row_count = 0u;
    }
    sNdsRendererHardwareTextureRefreshCount = 0u;
    return committed;
}

static void ndsFtrCarveNoteRelease(
    const NDSRendererHardwareTextureCacheEntry *entry);

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareReleaseTexture(
    NDSRendererHardwareTextureCacheEntry *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
    ndsRendererHardwareTextureLookupRemove(entry);
    ndsRendererHardwareEndBatch();
    /* Unconditional. This is hygiene, NOT the second-entry texture failure --
     * making it unconditional was tried against that failure and changed
     * nothing, so do not cite it as the cause.
     *
     * It is still correct on its own terms: the cache holds a NAME, libnds
     * recycles names through its dealloc pool once deleted (measured
     * deallocTexSize 0 on a first entry, 19 on a second), and any delete can
     * move libnds's own active texture. Clearing only on a match can therefore
     * leave the skip armed for a number that now belongs to a different
     * texture -- the same shape as the prepared-run cache and the
     * sNdsIFCommonPreparedFile latch. Cost is one redundant bind per release. */
    sNdsRendererHardwareBoundTextureName = 0u;
    if (sNdsRendererHardwareActiveTextureEntry == entry)
    {
        sNdsRendererHardwareActiveTextureEntry = NULL;
    }
    if ((entry->name != 0) && (NDS_FTR_CARVE_IS_NAME(entry->name) == FALSE))
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &entry->name);
    }
    /* A carved name (slice 2c) has no libnds record: its VRAM belongs to the
     * admission's carve regions, which go back to libnds with their last
     * entry. */
    if ((entry->carve_flags & NDS_FTR_CARVE_F_CARVED) != 0u)
    {
        ndsFtrCarveNoteRelease(entry);
    }
    /* The key is no longer inside the entry, so zeroing the entry no longer
     * zeroes it. Every reader gates on `ready`, but the particle atlas keeps a
     * slot ready with a deliberately blank key, so a released slot must really
     * be blank rather than merely unreachable. */
    ndsRendererHardwareEntryClearKey(entry);
    /* R2-07 leg A. This is the moment an entry stops being what a prepared run
     * recorded, so it is where the invalidation is published -- see the note on
     * ndsRendererNativeStagePreparedTexturesProven. Stamping the same counter a
     * re-key stamps makes it a complete epoch rather than a partial one; every
     * existing `entry->key_generation == recorded` compare keeps its meaning
     * because entries carry their own stamped value and the counter is only
     * ever advanced. */
    sNdsRendererHardwareTextureKeyGeneration++;
    if (sNdsRendererHardwareTextureKeyGeneration == 0u)
    {
        sNdsRendererHardwareTextureKeyGeneration++;
    }
    gNdsR2TextureEpochBumpCount++;
    memset(entry, 0, sizeof(*entry));
    return entry;
}

/* The stage select's preview warms each stage it shows the way a loaded
 * stage packet does (stage_warm: eviction skips the entry for the scene), so
 * its next stage would find the VRAM held by the stages before it and never
 * finish warming. The preview releases them when it drops a stage. */
void ndsRendererHardwareReleaseStageWarmTextures(void)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if ((entry->name != 0) && (entry->stage_warm != 0u) &&
            (entry->pinned == 0u) && (entry->admitted == 0u))
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
        }
    }
}

/* P01. The ground Thunder Jolt's three A5I3 coverage names are dedicated GL
 * names, not cache entries, so the sweep below cannot reach them. Defined in
 * src/nds/nds_native_pikachu_thunderground_coverage.inc. */
s32 ndsNativeThunderGroundReleaseOneCoverageTexture(void);

s32 ndsFtrLeanAdmitOpenHoles(void);
void ndsFtrLeanAdmitCloseHoles(void);
/* Uploads that took A+B's holes (ndsFtrLeanAdmitOpenHoles). */
volatile u32 gNdsFtrAdmitHoleUploads;

static s32 ndsRendererHardwareEvictTexture(
    const NDSRendererHardwareTextureCacheEntry *exclude)
{
    u32 i;

    /* The static partition is pinned by construction, so an eviction sweep that
     * walked it would only ever be counting slots it may not take. */
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT; i++)
    {
        u32 index = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT +
            (sNdsRendererHardwareTextureCacheNext %
             NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT);
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[index];

        sNdsRendererHardwareTextureCacheNext++;
        if ((entry != exclude) && (entry->name != 0) &&
            (entry->pinned == 0u) && (entry->stage_warm == 0u) &&
            (entry->admitted == 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
        {
            ndsRendererProfileRecordTextureEvict();
            (void)ndsRendererHardwareReleaseTexture(entry);
            return TRUE;
        }
    }
    /* P01, and it is the AIR Thunder Jolt regression. Dedicated names come
     * LAST, so the ordinary cache stays the preferred victim and normal
     * operation is unchanged; these are surrendered only when nothing else
     * can be.
     *
     * The ground-jolt repair moved three images off this cache and onto their
     * own names. It was priced in total bytes -- 6,144 to 3,120, a NEGATIVE
     * 3,024 -- and that was the wrong unit. Reclaimable bytes went from 6,144
     * to ZERO, because this sweep is the only reclaim either upload-retry loop
     * has, and it walks sNdsRendererHardwareTextureCache alone. The air jolt's
     * single 4,096-byte A3I5 upload (its render tile is 64x64) then had
     * nothing to evict and fell to the hard-alpha path or failed outright. */
    return ndsNativeThunderGroundReleaseOneCoverageTexture();
}

volatile u32 gNdsRendererTextureRangeReleaseCalls;
volatile u32 gNdsRendererTextureRangeReleaseEntries;

/* A FREED IMAGE'S CACHE ENTRY OUTLIVES THE IMAGE, AND THE KEY IS ITS ADDRESS.
 *
 * ndsRendererHardwareTextureKey::image, ::tlut_image and ::texel1_image are
 * POINTERS, and TextureKeyEqual is a memcmp of the whole key. So an entry
 * built from a character-select preview's bytes stays in the cache after that
 * preview is retired and its arena is reset, holding VRAM and -- worse --
 * staying eligible for a hit. The next preview's pack is allocated out of the
 * same resettable arena, so a later fighter's image can land at the same
 * address with the same format, size and tile window and take that entry as
 * its own, drawing the retired fighter's texels.
 *
 * ndsMNPlayersVSPreviewRetireResidentBlock already does exactly this for owner
 * images through ndsRendererNativeReleaseOwnerImagesInRange, and says why in
 * its comment: clear the aliases before the arena can reuse the bytes, so a
 * stale draw fails closed instead of following freed RAM. The hardware texture
 * cache is the one alias that was left behind. This is its twin.
 *
 * It also relieves the pressure that makes the character select the worst case
 * for this pool: it is the only screen that draws up to four different
 * fighters' textures in one frame with no pinned corpus, and both the eviction
 * sweep and AllocTexture refuse any entry touched in the current frame. With
 * retired previews still occupying slots, whether a takeable victim exists
 * depends on browsing history -- which is what an intermittent symptom that
 * tracks hover order looks like.
 *
 * Pinned and static entries are never touched: those are the generated corpus
 * and the deliberately retained residents, and neither lives in a preview
 * arena. */
u32 ndsRendererHardwareReleaseTexturesInRange(const void *base, size_t size)
{
    uintptr_t start;
    uintptr_t end;
    u32 released = 0u;
    u32 i;

    if ((base == NULL) || (size == 0u))
    {
        return 0u;
    }
    start = (uintptr_t)base;
    end = start + (uintptr_t)size;
    if (end < start)
    {
        return 0u;
    }
    NDS_DIAG(gNdsRendererTextureRangeReleaseCalls++);
    for (i = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT;
         i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        const NDSRendererHardwareTextureIdent *key;
        uintptr_t image;
        uintptr_t tlut;
        uintptr_t texel1;

        if ((entry->name == 0) || (entry->pinned != 0u))
        {
            continue;
        }
        /* Dynamic slots only: a static slot's key belongs to the generated
         * corpus, which no preview arena can own. The identity keeps the three
         * pointer words exactly. */
        key = ndsRendererHardwareEntryIdent(entry);
        if (key == NULL)
        {
            continue;
        }
        image = (uintptr_t)key->image;
        tlut = (uintptr_t)key->tlut_image;
        texel1 = (uintptr_t)key->texel1_image;
        if (((image >= start) && (image < end)) ||
            ((tlut >= start) && (tlut < end)) ||
            ((texel1 >= start) && (texel1 < end)))
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
            released++;
        }
    }
    NDS_DIAG(gNdsRendererTextureRangeReleaseEntries += released);
    return released;
}

#if NDS_VRAM_CENSUS_LIVE
/* P2-2p8 Phase 1 slice 2b (lab): dynamic-partition occupancy seen by every
 * allocation -- ready slots, admitted slots -- and allocations that found no
 * takeable slot at all (the ALLOC reject the admitted set could cause). */
volatile u32 gNdsFtrAdmitDynReadyHigh __attribute__((used));
volatile u32 gNdsFtrAdmitDynAdmittedHigh __attribute__((used));
/* Non-admitted, non-pinned entries touched in the current frame: with the
 * admitted ones, the slots an allocation can never take (the sizing input). */
volatile u32 gNdsFtrAdmitDynTouchedHigh __attribute__((used));
volatile u32 gNdsFtrAdmitDynFull __attribute__((used));
/* The last texture reject reason (NDS_RENDERER_HW_TEXREJECT_*), so the
 * admission can name why the cache refused one of its records. */
volatile u32 gNdsFtrAdmitLastReject __attribute__((used));
#endif

static NDSRendererHardwareTextureCacheEntry *
ndsRendererHardwareAllocTexture(void)
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 i;
    u32 index;

    ndsRendererHardwareRecordBattleTextureFence(
        NDS_RENDERER_BATTLE_TEXTURE_FENCE_ALLOC);
#if NDS_VRAM_CENSUS_LIVE
    {
        u32 ready = 0u;
        u32 admitted = 0u;
        u32 touched = 0u;

        for (i = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT;
             i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
        {
            const NDSRendererHardwareTextureCacheEntry *probe =
                &sNdsRendererHardwareTextureCache[i];

            ready += (probe->ready != 0u) ? 1u : 0u;
            admitted += probe->admitted;
            if ((probe->ready != 0u) && (probe->admitted == 0u) &&
                (probe->last_used_frame ==
                 (sNdsRendererHardwareFrameSerial + 1u)))
            {
                touched++;
            }
        }
        if (ready > gNdsFtrAdmitDynReadyHigh)
        {
            gNdsFtrAdmitDynReadyHigh = ready;
        }
        if (admitted > gNdsFtrAdmitDynAdmittedHigh)
        {
            gNdsFtrAdmitDynAdmittedHigh = admitted;
        }
        if (touched > gNdsFtrAdmitDynTouchedHigh)
        {
            gNdsFtrAdmitDynTouchedHigh = touched;
        }
    }
#endif

    /* Dynamic slots only. Slots below STATIC_COUNT belong to the generated
     * corpus one-for-one with its record indices and have no pool key to write,
     * so handing one to a runtime texture would leave it keyless. The static
     * prepare binds its own slots directly. */
    for (i = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT;
         i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if (sNdsRendererHardwareTextureCache[i].ready == 0u)
        {
            entry = &sNdsRendererHardwareTextureCache[i];
            return (entry->name != 0) ?
                ndsRendererHardwareReleaseTexture(entry) : entry;
        }
    }
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT; i++)
    {
        index = NDS_RENDERER_HW_TEXTURE_STATIC_COUNT +
            (sNdsRendererHardwareTextureCacheNext %
             NDS_RENDERER_HW_TEXTURE_DYNAMIC_COUNT);
        sNdsRendererHardwareTextureCacheNext++;
        entry = &sNdsRendererHardwareTextureCache[index];
        if ((entry->pinned == 0u) && (entry->stage_warm == 0u) &&
            (entry->admitted == 0u) &&
            (entry->last_used_frame !=
             (sNdsRendererHardwareFrameSerial + 1u)))
        {
            /* libnds allocates texture VRAM in glTexImage2D. Delete the old
             * allocation, but never recycle a texture referenced this frame. */
            ndsRendererProfileRecordTextureEvict();
            return ndsRendererHardwareReleaseTexture(entry);
        }
    }
#if NDS_VRAM_CENSUS_LIVE
    NDS_DIAG(gNdsFtrAdmitDynFull++);
#endif
    return NULL;
}

#if NDS_P2_STAGE_HYRULE
static u32 sNdsHyruleNativeNames[NDS_HYRULE_NATIVE_FRAME_COUNT];
volatile u32 gNdsHyruleNativeTexturePrepareCount;
volatile u32 gNdsHyruleNativeTextureFailCount;
#endif

static void ndsRendererHardwareResetFoxGunTextureState(void);
static void ndsRendererHardwareReleaseFoxGunTexture(void);

/* The ground Thunder Jolt's three A5I3 coverage names are not cache entries, so
 * the loops below do not reach them -- and glResetTextures runs immediately
 * after this in ndsRendererHardwareResetSceneTextureVram. Defined in
 * src/nds/nds_native_pikachu_thunderground_coverage.inc, which is pulled into
 * this translation unit further down nds_renderer.c. */
void ndsNativeThunderGroundReleaseCoverageTextures(void);

void ndsRendererHardwareDiscardTextureCache(void)
{
    if ((sNdsRendererBattleStaticTexturePrepared != 0u) ||
        (sNdsRendererBattleStaticTextureArmed != 0u))
    {
        gNdsRendererBattleStaticTextureViolationCount++;
    }
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    u32 i;

    ndsRendererHardwareEndBatch();
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        if ((sNdsRendererHardwareTextureCache[i].name != 0) ||
            (sNdsRendererHardwareTextureCache[i].ready != 0u))
        {
            (void)ndsRendererHardwareReleaseTexture(
                &sNdsRendererHardwareTextureCache[i]);
        }
    }
#if NDS_P2_STAGE_HYRULE
    ndsRendererHardwareReleaseHyruleTextures();
#endif
    memset(sNdsRendererHardwareTextureLookup, 0,
           sizeof(sNdsRendererHardwareTextureLookup));
    memset(sNdsRendererHardwareTextureRefreshQueue, 0,
           sizeof(sNdsRendererHardwareTextureRefreshQueue));
    sNdsRendererHardwareTextureRefreshCount = 0u;
    if (sNdsRendererHardwareNoTextureName != 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(
            1, &sNdsRendererHardwareNoTextureName);
        sNdsRendererHardwareNoTextureName = 0;
    }
    /* The beam's dedicated name is not a cache entry, so the loop above does
     * not reach it -- and glResetTextures runs immediately after this in
     * ndsRendererHardwareResetSceneTextureVram, which would leave the identity
     * below claiming residency for a name that no longer exists. */
    ndsRendererHardwareReleaseIFCommonCloudAtlas(
        &sNdsRendererHardwarePrimRgbTexel0AlphaName);
    sNdsRendererPrimRgbTexel0AlphaGen++;
    sNdsRendererHardwarePrimRgbTexel0AlphaImage = 0u;
    sNdsRendererHardwarePrimRgbTexel0AlphaExtent = 0u;
    sNdsRendererHardwarePrimRgbTexel0AlphaPrim = 0u;
    sNdsRendererHardwarePrimAlphaTexel0Mode = 0u;
    sNdsRendererHardwarePrimAlphaTexel0Env = 0u;
    sNdsRendererPrimRgbTexel0AlphaSourceWidth = 0u;
    sNdsRendererPrimRgbTexel0AlphaSourceOrigin = 0u;
    sNdsRendererPrimRgbTexel0AlphaSampler = 0u;
    gNdsRendererPrimEnvMaskedBakeCount = 0u;
    for (i = 0u; i < NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsRendererImpactWaveTextureName[i]);
        sNdsRendererImpactWaveTextureName[i] = 0u;
    }
    for (i = 0u; i < NDS_REBIRTH_HALO_TEXTURE_COUNT; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsRendererRebirthHaloTextureName[i]);
        sNdsRendererRebirthHaloTextureName[i] = 0u;
    }
    ndsNativeThunderGroundReleaseCoverageTextures();
    for (i = 0u; i < NDS_ENTRY_EFFECT_TEXTURE_COUNT; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsRendererEntryEffectTextureName[i]);
        sNdsRendererEntryEffectTextureName[i] = 0u;
    }
    for (i = 0u; i < 5u; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(&sNdsEntryShieldTextureName[i]);
        sNdsEntryShieldTextureName[i] = 0u;
    }
    for (i = 0u; i < 12u; i++)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(&sNdsEntryKoPaletteName[i / 4u][i % 4u]);
        sNdsEntryKoPaletteName[i / 4u][i % 4u] = 0u;
    }
    sNdsRendererHardwareTextureCacheNext = 0u;
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    /* Fox's gun is a direct scene-owned GL name, not a material-cache entry.
     * Delete it before glResetTextures and clear the fast-path identity so a
     * rematch cannot accept a stale/recycled name. */
    ndsRendererHardwareReleaseFoxGunTexture();
    ndsRendererHardwareResetSourceCaches();
    sNdsRendererBattleStaticTextureArmed = FALSE;
}
