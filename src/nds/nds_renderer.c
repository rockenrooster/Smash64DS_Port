#include <limits.h>
#include <string.h>

#include <nds/nds_gbi_decode.h>
#include <nds/nds_renderer.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#if NDS_RENDERER_HW_TRIANGLES
#include <nds.h>
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
#define NDS_RENDERER_OP_DL 0xdeu
#define NDS_RENDERER_OP_ENDDL 0xdfu
#define NDS_RENDERER_OP_SETOTHERMODE_H 0xe3u
#define NDS_RENDERER_OP_SETOTHERMODE_L 0xe2u
#define NDS_RENDERER_OP_SETSCISSOR 0xedu
#define NDS_RENDERER_OP_SETCOMBINE 0xfcu
#define NDS_RENDERER_OP_SETCIMG 0xffu
#define NDS_RENDERER_OP_SETFOGCOLOR 0xf8u
#define NDS_RENDERER_OP_SETBLENDCOLOR 0xf9u
#define NDS_RENDERER_OP_SETENVCOLOR 0xfbu
#define NDS_RENDERER_OP_SETPRIMCOLOR 0xfau
#define NDS_RENDERER_OP_SETTIMG 0xfdu
#define NDS_RENDERER_OP_SETTILE 0xf5u
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
#define NDS_RENDERER_LOAD_TILE 7u

#define NDS_RENDERER_MAX_VTX NDS_RENDERER_VERTEX_CACHE_SIZE
#define NDS_RENDERER_MODELVIEW_STACK_SIZE 32u
#define NDS_RENDERER_N64_MTX_FRAC_BITS 16u
#define NDS_RENDERER_DS_MTX_FRAC_BITS 12u
#define NDS_RENDERER_MTX_PUSH_XOR 0x01u
#define NDS_RENDERER_MTX_PUSH 0x01u
#define NDS_RENDERER_MTX_LOAD 0x02u
#define NDS_RENDERER_MTX_PROJECTION 0x04u
#define NDS_RENDERER_HW_FALLBACK_SCALE 16

#if NDS_RENDERER_HW_TRIANGLES
static u32 sNdsRendererHardwareSubmitted;
#endif

typedef struct NDSRendererTraversalState
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 matrix;
    NDSRendererMatrix20p12 modelview_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    NDSRendererClipVertex20p12 vertices[NDS_RENDERER_MAX_VTX];
#if NDS_RENDERER_HW_TRIANGLES
    NDSRendererInputVertex input_vertices[NDS_RENDERER_MAX_VTX];
#endif
    u32 modelview_valid_stack[NDS_RENDERER_MODELVIEW_STACK_SIZE];
    u32 modelview_stack_depth;
    u32 vertex_valid_mask;
#if NDS_RENDERER_HW_TRIANGLES
    u32 input_vertex_valid_mask;
#endif
    u32 projection_valid;
    u32 modelview_valid;
    u32 matrix_valid;
} NDSRendererTraversalState;

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

static void ndsRendererMtxMul20p12(const NDSRendererMatrix20p12 *lhs,
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

    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    xy = ndsRendererReadU32(src);
    zf = ndsRendererReadU32((const u8 *)src + 4);
    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
}

static s32 ndsRendererValidateCommand(const Gfx *dl,
                                      const NDSRendererConfig *config)
{
    uintptr_t addr = (uintptr_t)dl;

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

static void ndsRendererRecordIgnoredOtherMode(NDSRendererStats *stats,
                                              u32 op, u32 w0, u32 w1)
{
    stats->state_command_count++;
    stats->othermode_command_count++;
    stats->ignored_state_command_count++;
    if (stats->first_othermode_opcode == 0)
    {
        stats->first_othermode_opcode = op;
        stats->first_othermode_w0 = w0;
        stats->first_othermode_w1 = w1;
    }
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

    if (tile == NDS_RENDERER_RENDER_TILE)
    {
        u32 flags = NDS_RENDERER_TILE_RENDER_SEEN;

        stats->texture_render_tile = tile;
        stats->texture_render_tile_line = line;
        stats->texture_render_tile_tmem = tmem;
        stats->texture_render_tile_palette = palette;
        stats->texture_render_tile_cms = cms;
        stats->texture_render_tile_cmt = cmt;
        stats->texture_render_tile_masks = masks;
        stats->texture_render_tile_maskt = maskt;
        stats->texture_render_tile_shifts = shifts;
        stats->texture_render_tile_shiftt = shiftt;

        if ((cms & NDS_RENDERER_TX_CLAMP) != 0)
        {
            flags |= NDS_RENDERER_TILE_S_CLAMP;
        }
        if ((cms & NDS_RENDERER_TX_MIRROR) != 0)
        {
            flags |= NDS_RENDERER_TILE_S_MIRROR;
        }
        if (masks != 0)
        {
            flags |= NDS_RENDERER_TILE_S_MASKED;
        }
        if ((cmt & NDS_RENDERER_TX_CLAMP) != 0)
        {
            flags |= NDS_RENDERER_TILE_T_CLAMP;
        }
        if ((cmt & NDS_RENDERER_TX_MIRROR) != 0)
        {
            flags |= NDS_RENDERER_TILE_T_MIRROR;
        }
        if (maskt != 0)
        {
            flags |= NDS_RENDERER_TILE_T_MASKED;
        }
        stats->texture_render_tile_flags =
            (stats->texture_render_tile_flags & NDS_RENDERER_TILE_LOAD_SEEN) |
            flags;
    }
    else if (tile == NDS_RENDERER_LOAD_TILE)
    {
        stats->texture_load_tile = tile;
        stats->texture_render_tile_flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }

    (void)fmt;
    (void)siz;
}

static void ndsRendererRecordLoadBlock(NDSRendererStats *stats,
                                       u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADBLOCK;
    if (stats->texture_load_texels == 0)
    {
        stats->texture_load_tile = (w1 >> 24) & 0x7u;
        stats->texture_load_block_uls = (w0 >> 12) & 0x0FFFu;
        stats->texture_load_block_ult = w0 & 0x0FFFu;
        stats->texture_load_block_lrs = (w1 >> 12) & 0x0FFFu;
        stats->texture_load_block_dxt = w1 & 0x0FFFu;
        stats->texture_load_texels =
            stats->texture_load_block_lrs + 1u;
    }
}

static void ndsRendererRecordSetTileSize(NDSRendererStats *stats,
                                         u32 w0, u32 w1)
{
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;

    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILESIZE;
    if ((stats->texture_tile_width != 0) &&
        (stats->texture_tile_height != 0))
    {
        return;
    }

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    stats->texture_tile_size_tile = (w1 >> 24) & 0x7u;
    stats->texture_tile_size_uls = uls;
    stats->texture_tile_size_ult = ult;
    stats->texture_tile_size_lrs = lrs;
    stats->texture_tile_size_lrt = lrt;
    if (lrs >= uls)
    {
        stats->texture_tile_width = ((lrs - uls) >> 2) + 1u;
    }
    if (lrt >= ult)
    {
        stats->texture_tile_height = ((lrt - ult) >> 2) + 1u;
    }
}

static void ndsRendererRecordSetImage(NDSRendererStats *stats,
                                      u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTIMG;
    if (stats->texture_image == 0)
    {
        stats->texture_format = (w0 >> 21) & 0x7u;
        stats->texture_size = (w0 >> 19) & 0x3u;
        stats->texture_image_width = (w0 & 0x0FFFu) + 1u;
        stats->texture_image = w1;
    }
}

static void ndsRendererRecordSetCombine(NDSRendererStats *stats,
                                        u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETCOMBINE;
    stats->texture_combine_count++;
    if (stats->texture_combine_w0 == 0)
    {
        stats->texture_combine_w0 = w0;
        stats->texture_combine_w1 = w1;
    }
}

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

    for (i = 0u; i < count; i++)
    {
        NDSRendererInputVertex input;
        NDSRendererClipVertex20p12 *out = &state->vertices[v0 + i];

        ndsRendererDecodeInputVertex(&input, src + (i * 16u));
#if NDS_RENDERER_HW_TRIANGLES
        state->input_vertices[v0 + i] = input;
        state->input_vertex_valid_mask |= 1u << (v0 + i);
#endif
        if (state->matrix_valid == 0u)
        {
            continue;
        }
        ndsRendererTransformVertex20p12(&state->matrix, &input, out);
        state->vertex_valid_mask |= 1u << (v0 + i);
        stats->matrix_transform_count++;
        stats->transformed_vertex_count++;
        if (stats->transformed_vertex_count == 1u)
        {
            stats->first_transformed_x = out->x;
            stats->first_transformed_y = out->y;
            stats->first_transformed_z = out->z;
            stats->first_transformed_w = out->w;
        }
    }
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

static void ndsRendererLoadHardwareMatrices(
    const NDSRendererTraversalState *state)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    m4x4 projection_hw;
    m4x4 modelview_hw;

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererMtxIdentity20p12(&modelview);

    if (state != NULL)
    {
        if (state->projection_valid != 0u)
        {
            projection = state->projection;
        }
        if (state->modelview_valid != 0u)
        {
            modelview = state->modelview;
        }
        else
        {
            /*
             * ponytail: keeps no-matrix DL draw probes visible until the
             * original DObj matrix prep path feeds renderer-owned G_MTX.
             */
            modelview.m[0][0] = NDS_RENDERER_HW_FALLBACK_SCALE <<
                                NDS_RENDERER_DS_MTX_FRAC_BITS;
            modelview.m[1][1] = NDS_RENDERER_HW_FALLBACK_SCALE <<
                                NDS_RENDERER_DS_MTX_FRAC_BITS;
            modelview.m[2][2] = NDS_RENDERER_HW_FALLBACK_SCALE <<
                                NDS_RENDERER_DS_MTX_FRAC_BITS;
        }
    }

    ndsRendererCopyMtx20p12ToM4x4(&projection, &projection_hw);
    ndsRendererCopyMtx20p12ToM4x4(&modelview, &modelview_hw);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&projection_hw);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(&modelview_hw);
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

static void ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    const NDSRendererInputVertex *v0;
    const NDSRendererInputVertex *v1;
    const NDSRendererInputVertex *v2;

    if (stats == NULL)
    {
        return;
    }
    if (ndsRendererInputTriangleReady(state, packed, &i0, &i1, &i2) == FALSE)
    {
        stats->hardware_oracle_reject_count++;
        return;
    }
    if ((state != NULL) && (state->matrix_valid != 0u))
    {
        if (ndsRendererTransformedTriangleReady(state, packed, NULL, NULL,
                                                NULL) == FALSE)
        {
            stats->hardware_oracle_reject_count++;
            return;
        }
        stats->hardware_oracle_triangle_count++;
    }

    v0 = &state->input_vertices[i0];
    v1 = &state->input_vertices[i1];
    v2 = &state->input_vertices[i2];

    ndsRendererLoadHardwareMatrices(state);
    glPolyFmt(POLY_CULL_NONE | POLY_ALPHA(31));
    glColor3b(255, 224, 48);
    glBegin(GL_TRIANGLE);
    glVertex3v16(v0->x, v0->y, v0->z);
    glVertex3v16(v1->x, v1->y, v1->z);
    glVertex3v16(v2->x, v2->y, v2->z);
    glEnd();

    sNdsRendererHardwareSubmitted = TRUE;
    stats->hardware_triangle_count++;
    stats->hardware_vertex_count += 3u;
}
#endif

static void ndsRendererScanList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats,
                                NDSRendererTraversalState *state,
                                u32 depth,
                                NDSRendererCommandCallback callback,
                                void *callback_user)
{
    u32 i;

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

    for (i = 0; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;
        NDSRendererCommand command;

        if (ndsRendererValidateCommand(dl, config) == FALSE)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
            return;
        }
        if (stats->command_count >= config->max_commands)
        {
            stats->blocker = NDS_RENDERER_BLOCKER_BUDGET;
            return;
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
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
            stats->state_command_count++;
            stats->skip_command_count++;
            break;

        case NDS_RENDERER_OP_VTX:
            ndsRendererApplyVertexCommand(config, stats, state, w0, w1);
            break;

        case NDS_RENDERER_OP_TRI1:
            stats->triangle_command_count++;
            stats->triangle_count++;
            stats->render_command_count++;
            ndsRendererRecordTransformedTriangle(
                stats, state, ndsGBIDecodeF3DEX2Tri1(w0));
#if NDS_RENDERER_HW_TRIANGLES
            ndsRendererSubmitHardwareTriangle(
                stats, state, ndsGBIDecodeF3DEX2Tri1(w0));
#endif
            break;

        case NDS_RENDERER_OP_TRI2:
            stats->triangle_command_count++;
            stats->triangle_count += 2u;
            stats->render_command_count++;
            ndsRendererRecordTransformedTriangle(
                stats, state, ndsGBIDecodeF3DEX2Tri2First(w0));
            ndsRendererRecordTransformedTriangle(
                stats, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#if NDS_RENDERER_HW_TRIANGLES
            ndsRendererSubmitHardwareTriangle(
                stats, state, ndsGBIDecodeF3DEX2Tri2First(w0));
            ndsRendererSubmitHardwareTriangle(
                stats, state, ndsGBIDecodeF3DEX2Tri2Second(w1));
#endif
            break;

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
        case NDS_RENDERER_OP_SETSCISSOR:
        case NDS_RENDERER_OP_SETCIMG:
            stats->state_command_count++;
            stats->ignored_state_command_count++;
            break;

        case NDS_RENDERER_OP_GEOMETRYMODE:
            stats->geometry_mode = (stats->geometry_mode & w0) | w1;
            stats->geometry_clear_mask = w0;
            stats->geometry_set_mask = w1;
            stats->geometry_command_count++;
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETCOMBINE:
            ndsRendererRecordSetCombine(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTIMG:
            ndsRendererRecordSetImage(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTILE:
            ndsRendererRecordSetTile(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_LOADBLOCK:
            ndsRendererRecordLoadBlock(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_LOADTLUT:
            stats->texture_command_count++;
            stats->state_command_count++;
            stats->ignored_state_command_count++;
            break;

        case NDS_RENDERER_OP_SETTILESIZE:
            ndsRendererRecordSetTileSize(stats, w0, w1);
            stats->state_command_count++;
            break;

        case NDS_RENDERER_OP_SETFOGCOLOR:
        case NDS_RENDERER_OP_SETBLENDCOLOR:
        case NDS_RENDERER_OP_SETENVCOLOR:
        case NDS_RENDERER_OP_SETPRIMCOLOR:
            stats->state_command_count++;
            stats->color_command_count++;
            if (op == NDS_RENDERER_OP_SETPRIMCOLOR)
            {
                stats->prim_color = w1;
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
            ndsRendererRecordIgnoredOtherMode(stats, op, w0, w1);
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
    }
}

void ndsRendererScanDisplayList(const Gfx *dl,
                                const NDSRendererConfig *config,
                                NDSRendererStats *stats)
{
    NDSRendererTraversalState state;

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

    memset(&state, 0, sizeof(state));
    ndsRendererScanList(dl, config, stats, &state, 0, NULL, NULL);
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

u32 ndsRendererHardwareConsumeSubmittedFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 submitted = sNdsRendererHardwareSubmitted;

    sNdsRendererHardwareSubmitted = FALSE;
    return submitted;
#else
    return FALSE;
#endif
}

void ndsRendererExecuteDisplayList(const Gfx *dl,
                                   const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user,
                                   NDSRendererStats *stats)
{
    NDSRendererTraversalState state;

    if (stats == NULL)
    {
        return;
    }

    if (config == NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_BAD_BRANCH;
        return;
    }

    memset(&state, 0, sizeof(state));
    ndsRendererScanList(dl, config, stats, &state, 0, callback,
                        callback_user);
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
