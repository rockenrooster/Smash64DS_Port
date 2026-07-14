#include <nds/nds_gbi_decode.h>
#include <nds/nds_platform.h>
#include <sys/objdef.h>

#define NDS_DL_PREVIEW_WIDTH 96u
#define NDS_DL_PREVIEW_HEIGHT 72u
#define NDS_DL_PREVIEW_MAX_VTX 32u
#define NDS_DL_PREVIEW_MAX_TRIS 64u

#define NDS_GBI_OP_VTX 0x01u
#define NDS_GBI_OP_CULLDL 0x03u
#define NDS_GBI_OP_TRI1 0x05u
#define NDS_GBI_OP_TRI2 0x06u
#define NDS_GBI_OP_TEXTURE 0xd7u
#define NDS_GBI_OP_POPMTX 0xd8u
#define NDS_GBI_OP_SPECIAL_1 0xd5u
#define NDS_GBI_OP_MTX 0xdau
#define NDS_GBI_OP_GEOMETRYMODE 0xd9u
#define NDS_GBI_OP_MOVEWORD 0xdbu
#define NDS_GBI_OP_DL 0xdeu
#define NDS_GBI_OP_ENDDL 0xdfu
#define NDS_GBI_OP_SETOTHERMODE_H 0xe3u
#define NDS_GBI_OP_SETOTHERMODE_L 0xe2u
#define NDS_GBI_OP_SETSCISSOR 0xedu
#define NDS_GBI_OP_SETCOMBINE 0xfcu
#define NDS_GBI_OP_SETFOGCOLOR 0xf8u
#define NDS_GBI_OP_SETBLENDCOLOR 0xf9u
#define NDS_GBI_OP_SETENVCOLOR 0xfbu
#define NDS_GBI_OP_SETPRIMCOLOR 0xfau
#define NDS_GBI_OP_SETTIMG 0xfdu
#define NDS_GBI_OP_SETTILE 0xf5u
#define NDS_GBI_OP_LOADBLOCK 0xf3u
#define NDS_GBI_OP_SETTILESIZE 0xf2u
#define NDS_GBI_OP_RDPSETOTHERMODE 0xefu
#define NDS_GBI_OP_RDPPIPESYNC 0xe7u
#define NDS_GBI_OP_RDPLOADSYNC 0xe6u
#define NDS_GBI_OP_RDPTILESYNC 0xe8u
#define NDS_GBI_OP_RDPFULLSYNC 0xe9u

#define NDS_GBI_GEOMETRY_ZBUFFER 0x00000001u
#define NDS_GBI_GEOMETRY_SHADE 0x00000004u
#define NDS_GBI_GEOMETRY_CULL_FRONT 0x00000200u
#define NDS_GBI_GEOMETRY_CULL_BACK 0x00000400u
#define NDS_GBI_GEOMETRY_LIGHTING 0x00020000u
#define NDS_GBI_GEOMETRY_SHADING_SMOOTH 0x00200000u

#define NDS_GBI_TX_MIRROR 0x1u
#define NDS_GBI_TX_CLAMP 0x2u

#define NDS_GBI_CCMUX_TEXEL0 1u
#define NDS_GBI_CCMUX_SHADE 4u
#define NDS_GBI_CCMUX_0_4BIT 15u
#define NDS_GBI_CCMUX_0_3BIT 7u
#define NDS_GBI_ACMUX_TEXEL0 1u
#define NDS_GBI_ACMUX_0 7u

typedef struct NDSDLPreviewVtx
{
    s16 x;
    s16 y;
    s16 z;
    s16 s;
    s16 t;
    s32 wx;
    s32 wy;
    s32 wz;
    s32 px;
    s32 py;
    s32 depth100;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    ub8 valid;
    ub8 projected;
} NDSDLPreviewVtx;

typedef struct NDSDLPreviewTri
{
    u8 v0;
    u8 v1;
    u8 v2;
} NDSDLPreviewTri;

typedef struct NDSDLPreviewTileState
{
    u32 fmt;
    u32 siz;
    u32 line;
    u32 tmem;
    u32 tile;
    u32 palette;
    u32 cmt;
    u32 maskt;
    u32 shiftt;
    u32 cms;
    u32 masks;
    u32 shifts;
    ub8 valid;
} NDSDLPreviewTileState;

typedef struct NDSDLPreviewTextureState
{
    u32 scale_s;
    u32 scale_t;
    u32 level;
    u32 tile;
    u32 on;
    u32 xparam;
    ub8 valid;
} NDSDLPreviewTextureState;

typedef struct NDSDLPreviewParseState
{
    NDSDLPreviewVtx *vertices;
    NDSDLPreviewTri *tris;
    NDSDLPreviewTextureState *texture_state;
    NDSDLPreviewTileState *render_tile_state;
    NDSDLPreviewTileState *load_tile_state;
    u32 *vertex_count;
    u32 *tri_count;
    u32 *command_count;
    u32 *unsupported;
    u32 *geometry_mode;
    u32 *combine_mode;
    u32 *combine_flags;
} NDSDLPreviewParseState;

typedef struct NDSOpeningRoomMaterialProbe
{
    u32 count;
    u32 flags;
    u32 effective_flags;
    u32 mask;
    u32 texture_curr;
    u32 texture_next;
    u32 palette_index;
    s32 lfrac100;
    u32 format;
    u32 size;
    u32 block_format;
    u32 block_size;
    u32 tile_width;
    u32 tile_height;
    u32 scroll_width;
    u32 scroll_height;
    s32 scale_s100;
    s32 scale_t100;
    s32 translate_s100;
    s32 translate_t100;
    u32 sprite_array;
    u32 palette_array;
    u32 sprite_curr;
    u32 sprite_next;
    u32 palette_ptr;
} NDSOpeningRoomMaterialProbe;

static u16 ndsPreviewRGB15(u8 r, u8 g, u8 b)
{
    return (u16)(0x8000u |
                 (((u16)(r >> 3)) & 0x1Fu) |
                 ((((u16)(g >> 3)) & 0x1Fu) << 5) |
                 ((((u16)(b >> 3)) & 0x1Fu) << 10));
}

static u8 ndsPreviewClampU8(s64 value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 255)
    {
        return 255;
    }
    return (u8)value;
}

static u8 ndsPreviewInterpolateU8(u8 a, u8 b, u8 c,
                                  s32 w0, s32 w1, s32 w2, s32 area)
{
    s64 value;

    if (area == 0)
    {
        return a;
    }

    value = (((s64)a * (s64)w0) +
             ((s64)b * (s64)w1) +
             ((s64)c * (s64)w2)) / (s64)area;
    return ndsPreviewClampU8(value);
}

static u16 ndsPreviewModulateRGB15(u16 color, u8 r, u8 g, u8 b)
{
    u32 out_r;
    u32 out_g;
    u32 out_b;

    if ((color & 0x8000u) == 0)
    {
        return 0;
    }

    out_r = (((u32)(color & 0x001Fu) * (u32)r) + 127u) / 255u;
    out_g = (((u32)((color >> 5) & 0x001Fu) * (u32)g) + 127u) / 255u;
    out_b = (((u32)((color >> 10) & 0x001Fu) * (u32)b) + 127u) / 255u;

    if (out_r > 31u) out_r = 31u;
    if (out_g > 31u) out_g = 31u;
    if (out_b > 31u) out_b = 31u;

    return (u16)(0x8000u | out_r | (out_g << 5) | (out_b << 10));
}

static u32 ndsPreviewDecodeCombineMode(u32 w0, u32 w1, u32 *out_flags)
{
    u32 flags = NDS_OPENING_ROOM_DL_COMBINE_SEEN;
    u32 c0_a = (w0 >> 20) & 0xFu;
    u32 c0_c = (w0 >> 15) & 0x1Fu;
    u32 c0_aa = (w0 >> 12) & 0x7u;
    u32 c0_ac = (w0 >> 9) & 0x7u;
    u32 c1_a = (w0 >> 5) & 0xFu;
    u32 c1_c = w0 & 0x1Fu;
    u32 c0_b = (w1 >> 28) & 0xFu;
    u32 c0_d = (w1 >> 15) & 0x7u;
    u32 c0_ab = (w1 >> 12) & 0x7u;
    u32 c0_ad = (w1 >> 9) & 0x7u;
    u32 c1_b = (w1 >> 24) & 0xFu;
    u32 c1_aa = (w1 >> 21) & 0x7u;
    u32 c1_ac = (w1 >> 18) & 0x7u;
    u32 c1_d = (w1 >> 6) & 0x7u;
    u32 c1_ab = (w1 >> 3) & 0x7u;
    u32 c1_ad = w1 & 0x7u;
    ub8 rgb_modulate =
        (c0_a == NDS_GBI_CCMUX_TEXEL0) &&
        (c0_b == NDS_GBI_CCMUX_0_4BIT) &&
        (c0_c == NDS_GBI_CCMUX_SHADE) &&
        (c0_d == NDS_GBI_CCMUX_0_3BIT) &&
        (c1_a == NDS_GBI_CCMUX_TEXEL0) &&
        (c1_b == NDS_GBI_CCMUX_0_4BIT) &&
        (c1_c == NDS_GBI_CCMUX_SHADE) &&
        (c1_d == NDS_GBI_CCMUX_0_3BIT);
    ub8 alpha_texel =
        (c0_aa == NDS_GBI_ACMUX_0) &&
        (c0_ab == NDS_GBI_ACMUX_0) &&
        (c0_ac == NDS_GBI_ACMUX_0) &&
        (c0_ad == NDS_GBI_ACMUX_TEXEL0) &&
        (c1_aa == NDS_GBI_ACMUX_0) &&
        (c1_ab == NDS_GBI_ACMUX_0) &&
        (c1_ac == NDS_GBI_ACMUX_0) &&
        (c1_ad == NDS_GBI_ACMUX_TEXEL0);

    if (rgb_modulate != FALSE)
    {
        flags |= NDS_OPENING_ROOM_DL_COMBINE_RGB_TEXEL0 |
                 NDS_OPENING_ROOM_DL_COMBINE_RGB_SHADE |
                 NDS_OPENING_ROOM_DL_COMBINE_RGB_MODULATE;
    }
    if (alpha_texel != FALSE)
    {
        flags |= NDS_OPENING_ROOM_DL_COMBINE_ALPHA_TEXEL0;
    }
    if ((rgb_modulate != FALSE) && (alpha_texel != FALSE))
    {
        flags |= NDS_OPENING_ROOM_DL_COMBINE_EXACT_MODULATERGBDECALA;
        if (out_flags != NULL)
        {
            *out_flags = flags;
        }
        return NDS_OPENING_ROOM_DL_COMBINE_MODE_MODULATERGBDECALA;
    }

    if (out_flags != NULL)
    {
        *out_flags = flags;
    }
    return NDS_OPENING_ROOM_DL_COMBINE_MODE_UNKNOWN;
}

static u32 ndsReadU32(const void *ptr)
{
    const u8 *bytes = ptr;

    return ((u32)bytes[0]) |
           (((u32)bytes[1]) << 8) |
           (((u32)bytes[2]) << 16) |
           (((u32)bytes[3]) << 24);
}

static void ndsDecodeSwappedVtx(NDSDLPreviewVtx *dst, const u8 *src)
{
    u32 xy = ndsReadU32(src + 0);
    u32 zf = ndsReadU32(src + 4);
    u32 st = ndsReadU32(src + 8);
    u32 rgba = ndsReadU32(src + 12);

    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xFFFFu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xFFFFu);
    dst->wx = dst->x;
    dst->wy = dst->y;
    dst->wz = dst->z;
    dst->px = 0;
    dst->py = 0;
    dst->depth100 = 0;
    dst->r = (u8)(rgba >> 24);
    dst->g = (u8)(rgba >> 16);
    dst->b = (u8)(rgba >> 8);
    dst->a = (u8)rgba;
    dst->valid = TRUE;
    dst->projected = FALSE;

    if (dst->a == 0)
    {
        dst->a = 0xFF;
    }
}

static void ndsDLPreviewAddTriangle(NDSDLPreviewTri *tris,
                                    u32 *tri_count,
                                    u32 packed)
{
    u32 v0;
    u32 v1;
    u32 v2;

    ndsGBIDecodePackedTriIndices(packed, &v0, &v1, &v2);

    if ((*tri_count >= NDS_DL_PREVIEW_MAX_TRIS) ||
        (v0 >= NDS_DL_PREVIEW_MAX_VTX) ||
        (v1 >= NDS_DL_PREVIEW_MAX_VTX) ||
        (v2 >= NDS_DL_PREVIEW_MAX_VTX))
    {
        return;
    }

    tris[*tri_count].v0 = (u8)v0;
    tris[*tri_count].v1 = (u8)v1;
    tris[*tri_count].v2 = (u8)v2;
    (*tri_count)++;
}

static void ndsDLPreviewRecordUnsupported(u32 *first_unsupported, u32 op)
{
    if (first_unsupported != NULL)
    {
        if (*first_unsupported == 0)
        {
            *first_unsupported = op;
        }
    }
    gNdsOpeningRoomDLPreviewUnsupportedCommandCount++;
}

static s32 ndsPreviewEdge(s32 ax, s32 ay, s32 bx, s32 by, s32 px, s32 py)
{
    return ((px - ax) * (by - ay)) - ((py - ay) * (bx - ax));
}

static void ndsPreviewPutPixel(u16 *pixels, u32 pitch, s32 x, s32 y,
                               u16 color, u32 *pixel_count)
{
    u16 *dst;

    if ((x < 0) || (y < 0) ||
        (x >= (s32)NDS_DL_PREVIEW_WIDTH) ||
        (y >= (s32)NDS_DL_PREVIEW_HEIGHT))
    {
        return;
    }

    dst = pixels + (y * (s32)pitch) + x;
    if (*dst == 0)
    {
        (*pixel_count)++;
    }
    *dst = color;
}

static void ndsPreviewDrawLine(u16 *pixels, u32 pitch,
                               s32 x0, s32 y0, s32 x1, s32 y1,
                               u16 color, u32 *pixel_count)
{
    s32 dx = abs(x1 - x0);
    s32 sx = (x0 < x1) ? 1 : -1;
    s32 dy = -abs(y1 - y0);
    s32 sy = (y0 < y1) ? 1 : -1;
    s32 err = dx + dy;

    while (TRUE)
    {
        s32 e2;

        ndsPreviewPutPixel(pixels, pitch, x0, y0, color, pixel_count);
        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }
        e2 = err * 2;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static void ndsPreviewDrawTriangle(u16 *pixels, u32 pitch,
                                   s32 x0, s32 y0, s32 x1, s32 y1,
                                   s32 x2, s32 y2, u16 fill, u16 edge,
                                   u32 *pixel_count)
{
    s32 min_x = x0;
    s32 max_x = x0;
    s32 min_y = y0;
    s32 max_y = y0;
    s32 y;
    s32 area;

    if (x1 < min_x) min_x = x1;
    if (x2 < min_x) min_x = x2;
    if (x1 > max_x) max_x = x1;
    if (x2 > max_x) max_x = x2;
    if (y1 < min_y) min_y = y1;
    if (y2 < min_y) min_y = y2;
    if (y1 > max_y) max_y = y1;
    if (y2 > max_y) max_y = y2;

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= (s32)NDS_DL_PREVIEW_WIDTH) max_x = NDS_DL_PREVIEW_WIDTH - 1;
    if (max_y >= (s32)NDS_DL_PREVIEW_HEIGHT) max_y = NDS_DL_PREVIEW_HEIGHT - 1;

    area = ndsPreviewEdge(x0, y0, x1, y1, x2, y2);
    if (area != 0)
    {
        for (y = min_y; y <= max_y; y++)
        {
            s32 x;

            for (x = min_x; x <= max_x; x++)
            {
                s32 w0 = ndsPreviewEdge(x1, y1, x2, y2, x, y);
                s32 w1 = ndsPreviewEdge(x2, y2, x0, y0, x, y);
                s32 w2 = ndsPreviewEdge(x0, y0, x1, y1, x, y);

                if (((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) ||
                    ((w0 <= 0) && (w1 <= 0) && (w2 <= 0)))
                {
                    ndsPreviewPutPixel(pixels, pitch, x, y, fill,
                                       pixel_count);
                }
            }
        }
    }

    ndsPreviewDrawLine(pixels, pitch, x0, y0, x1, y1, edge, pixel_count);
    ndsPreviewDrawLine(pixels, pitch, x1, y1, x2, y2, edge, pixel_count);
    ndsPreviewDrawLine(pixels, pitch, x2, y2, x0, y0, edge, pixel_count);
}

static s32 ndsPreviewWrapCoord(s32 value, s32 limit)
{
    if (limit <= 0)
    {
        return 0;
    }
    value %= limit;
    if (value < 0)
    {
        value += limit;
    }
    return value;
}

static void ndsPreviewDecodeTileState(NDSDLPreviewTileState *tile,
                                      u32 w0, u32 w1)
{
    if (tile == NULL)
    {
        return;
    }

    tile->fmt = (w0 >> 21) & 0x7u;
    tile->siz = (w0 >> 19) & 0x3u;
    tile->line = (w0 >> 9) & 0x1FFu;
    tile->tmem = w0 & 0x1FFu;
    tile->tile = (w1 >> 24) & 0x7u;
    tile->palette = (w1 >> 20) & 0xFu;
    tile->cmt = (w1 >> 18) & 0x3u;
    tile->maskt = (w1 >> 14) & 0xFu;
    tile->shiftt = (w1 >> 10) & 0xFu;
    tile->cms = (w1 >> 8) & 0x3u;
    tile->masks = (w1 >> 4) & 0xFu;
    tile->shifts = w1 & 0xFu;
    tile->valid = TRUE;
}

static void ndsPreviewDecodeTextureState(NDSDLPreviewTextureState *texture,
                                         u32 w0, u32 w1)
{
    if (texture == NULL)
    {
        return;
    }

    texture->xparam = (w0 >> 16) & 0xFFu;
    texture->level = (w0 >> 11) & 0x7u;
    texture->tile = (w0 >> 8) & 0x7u;
    texture->on = (w0 >> 1) & 0x7Fu;
    texture->scale_s = (w1 >> 16) & 0xFFFFu;
    texture->scale_t = w1 & 0xFFFFu;
    texture->valid = TRUE;
}

static u32 ndsPreviewTextureFlagsFromState(
    const NDSDLPreviewTextureState *texture)
{
    u32 flags = 0;

    if ((texture != NULL) && (texture->valid != FALSE))
    {
        flags |= NDS_OPENING_ROOM_DL_TEXTURE_STATE_SEEN;
        if (texture->on != 0)
        {
            flags |= NDS_OPENING_ROOM_DL_TEXTURE_STATE_ON;
        }
        if (texture->scale_s != 0)
        {
            flags |= NDS_OPENING_ROOM_DL_TEXTURE_STATE_SCALE_S;
        }
        if (texture->scale_t != 0)
        {
            flags |= NDS_OPENING_ROOM_DL_TEXTURE_STATE_SCALE_T;
        }
    }
    return flags;
}

static u32 ndsPreviewTileFlagsFromState(const NDSDLPreviewTileState *render,
                                        const NDSDLPreviewTileState *load)
{
    u32 flags = 0;

    if ((render != NULL) && (render->valid != FALSE))
    {
        flags |= NDS_OPENING_ROOM_DL_TILE_RENDER_SEEN;
        if (render->cms & NDS_GBI_TX_CLAMP)
        {
            flags |= NDS_OPENING_ROOM_DL_TILE_S_CLAMP;
        }
        if (render->cms & NDS_GBI_TX_MIRROR)
        {
            flags |= NDS_OPENING_ROOM_DL_TILE_S_MIRROR;
        }
        if (render->masks != 0)
        {
            flags |= NDS_OPENING_ROOM_DL_TILE_S_MASKED;
        }
        if (render->cmt & NDS_GBI_TX_CLAMP)
        {
            flags |= NDS_OPENING_ROOM_DL_TILE_T_CLAMP;
        }
        if (render->cmt & NDS_GBI_TX_MIRROR)
        {
            flags |= NDS_OPENING_ROOM_DL_TILE_T_MIRROR;
        }
        if (render->maskt != 0)
        {
            flags |= NDS_OPENING_ROOM_DL_TILE_T_MASKED;
        }
    }
    if ((load != NULL) && (load->valid != FALSE))
    {
        flags |= NDS_OPENING_ROOM_DL_TILE_LOAD_SEEN;
    }
    return flags;
}

static s32 ndsPreviewScaleTextureCoord(s32 coord, u32 scale)
{
    s64 scaled = (s64)coord * (s64)(scale & 0xFFFFu);

    scaled >>= 16;
    if (scaled > 0x7FFFFFFFLL)
    {
        return 0x7FFFFFFF;
    }
    if (scaled < -0x80000000LL)
    {
        return (s32)0x80000000;
    }
    return (s32)scaled;
}

static s32 ndsPreviewAddressTextureCoord(s32 coord, s32 limit, u32 mode,
                                         u32 mask)
{
    s32 repeat_limit = limit;

    if (limit <= 0)
    {
        return 0;
    }
    if (mode & NDS_GBI_TX_CLAMP)
    {
        if (coord < 0)
        {
            return 0;
        }
        if (coord >= limit)
        {
            return limit - 1;
        }
        return coord;
    }

    if ((mask != 0) && (mask < 31u))
    {
        repeat_limit = (s32)(1u << mask);
    }
    if (repeat_limit <= 0)
    {
        repeat_limit = limit;
    }

    if (mode & NDS_GBI_TX_MIRROR)
    {
        s32 mirror_limit = repeat_limit * 2;
        s32 wrapped = ndsPreviewWrapCoord(coord, mirror_limit);

        if (wrapped >= repeat_limit)
        {
            wrapped = mirror_limit - 1 - wrapped;
        }
        return ndsPreviewWrapCoord(wrapped, limit);
    }
    return ndsPreviewWrapCoord(ndsPreviewWrapCoord(coord, repeat_limit),
                               limit);
}

static u16 ndsPreviewSampleRgba16Texture(const u16 *texels,
                                         u32 texel_width,
                                         u32 texel_height,
                                         const NDSDLPreviewTextureState *texture,
                                         const NDSDLPreviewTileState *tile,
                                         s32 s, s32 t)
{
    s32 u;
    s32 v;
    u16 color;

    if ((texels == NULL) || (texel_width == 0) || (texel_height == 0))
    {
        return 0;
    }

    if ((texture != NULL) && (texture->valid != FALSE))
    {
        s = ndsPreviewScaleTextureCoord(s, texture->scale_s);
        t = ndsPreviewScaleTextureCoord(t, texture->scale_t);
    }
    u = s >> 5;
    v = t >> 5;
    if ((tile != NULL) && (tile->valid != FALSE))
    {
        u = ndsPreviewAddressTextureCoord(u, (s32)texel_width,
                                          tile->cms, tile->masks);
        v = ndsPreviewAddressTextureCoord(v, (s32)texel_height,
                                          tile->cmt, tile->maskt);
    }
    else
    {
        u = ndsPreviewWrapCoord(u, (s32)texel_width);
        if (v < 0)
        {
            v = 0;
        }
        if (v >= (s32)texel_height)
        {
            v = (s32)texel_height - 1;
        }
    }

    /* Opening Room O2R payloads currently receive a blanket u32 endian pass.
     * RGBA16 texels are byte-correct but swapped in halfword pairs. */
    color = texels[(((u32)v * texel_width) + (u32)u) ^ 1u];
    return ndsStartupLogoConvertRgba16(color);
}

static u16 ndsPreviewSampleI16Texture(const u16 *texels,
                                      u32 texel_width,
                                      u32 texel_height,
                                      const NDSDLPreviewTextureState *texture,
                                      const NDSDLPreviewTileState *tile,
                                      s32 s, s32 t)
{
    s32 u;
    s32 v;
    u16 raw;
    u8 intensity;

    if ((texels == NULL) || (texel_width == 0) || (texel_height == 0))
    {
        return 0;
    }

    if ((texture != NULL) && (texture->valid != FALSE))
    {
        s = ndsPreviewScaleTextureCoord(s, texture->scale_s);
        t = ndsPreviewScaleTextureCoord(t, texture->scale_t);
    }
    u = s >> 5;
    v = t >> 5;
    if ((tile != NULL) && (tile->valid != FALSE))
    {
        u = ndsPreviewAddressTextureCoord(u, (s32)texel_width,
                                          tile->cms, tile->masks);
        v = ndsPreviewAddressTextureCoord(v, (s32)texel_height,
                                          tile->cmt, tile->maskt);
    }
    else
    {
        u = ndsPreviewWrapCoord(u, (s32)texel_width);
        if (v < 0)
        {
            v = 0;
        }
        if (v >= (s32)texel_height)
        {
            v = (s32)texel_height - 1;
        }
    }

    raw = texels[(((u32)v * texel_width) + (u32)u) ^ 1u];
    intensity = (u8)(raw >> 8);
    if (intensity == 0)
    {
        intensity = (u8)raw;
    }
    return ndsPreviewRGB15(intensity, intensity, intensity);
}

static u16 ndsPreviewSampleTexture(const u16 *texels,
                                   u32 texel_width,
                                   u32 texel_height,
                                   const NDSDLPreviewTextureState *texture,
                                   const NDSDLPreviewTileState *tile,
                                   s32 s, s32 t,
                                   u32 sample_mode)
{
    switch (sample_mode)
    {
    case NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_RGBA16:
        return ndsPreviewSampleRgba16Texture(texels, texel_width,
                                             texel_height, texture, tile,
                                             s, t);

    case NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_I16:
        return ndsPreviewSampleI16Texture(texels, texel_width,
                                          texel_height, texture, tile,
                                          s, t);

    case NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_NONE:
    default:
        return 0;
    }
}

static void ndsPreviewDrawTexturedTriangle(u16 *pixels, u32 pitch,
                                           const NDSDLPreviewVtx *a,
                                           const NDSDLPreviewVtx *b,
                                           const NDSDLPreviewVtx *c,
                                           s32 x0, s32 y0, s32 x1, s32 y1,
                                           s32 x2, s32 y2,
                                           const u16 *texels,
                                           u32 texel_width,
                                           u32 texel_height,
                                           const NDSDLPreviewTextureState *texture,
                                           const NDSDLPreviewTileState *tile,
                                           u32 sample_mode,
                                           u16 fallback,
                                           u16 edge,
                                           u32 combine_mode,
                                           u32 *pixel_count,
                                           u32 *texture_sample_count,
                                           u32 *texture_modulated_count)
{
    s32 min_x = x0;
    s32 max_x = x0;
    s32 min_y = y0;
    s32 max_y = y0;
    s32 area;
    s32 y;

    if (x1 < min_x) min_x = x1;
    if (x2 < min_x) min_x = x2;
    if (x1 > max_x) max_x = x1;
    if (x2 > max_x) max_x = x2;
    if (y1 < min_y) min_y = y1;
    if (y2 < min_y) min_y = y2;
    if (y1 > max_y) max_y = y1;
    if (y2 > max_y) max_y = y2;

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= (s32)NDS_DL_PREVIEW_WIDTH) max_x = NDS_DL_PREVIEW_WIDTH - 1;
    if (max_y >= (s32)NDS_DL_PREVIEW_HEIGHT) max_y = NDS_DL_PREVIEW_HEIGHT - 1;

    area = ndsPreviewEdge(x0, y0, x1, y1, x2, y2);
    if (area != 0)
    {
        for (y = min_y; y <= max_y; y++)
        {
            s32 x;

            for (x = min_x; x <= max_x; x++)
            {
                s32 w0 = ndsPreviewEdge(x1, y1, x2, y2, x, y);
                s32 w1 = ndsPreviewEdge(x2, y2, x0, y0, x, y);
                s32 w2 = ndsPreviewEdge(x0, y0, x1, y1, x, y);

                if (((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) ||
                    ((w0 <= 0) && (w1 <= 0) && (w2 <= 0)))
                {
                    s32 s = ((s32)a->s * w0) +
                            ((s32)b->s * w1) +
                            ((s32)c->s * w2);
                    s32 t = ((s32)a->t * w0) +
                            ((s32)b->t * w1) +
                            ((s32)c->t * w2);
                    u16 texel;

                    s /= area;
                    t /= area;
                    texel = ndsPreviewSampleTexture(
                        texels, texel_width, texel_height, texture, tile,
                        s, t, sample_mode);
                    if (texel != 0)
                    {
                        if (combine_mode ==
                            NDS_OPENING_ROOM_DL_COMBINE_MODE_MODULATERGBDECALA)
                        {
                            u8 shade_r = ndsPreviewInterpolateU8(
                                a->r, b->r, c->r, w0, w1, w2, area);
                            u8 shade_g = ndsPreviewInterpolateU8(
                                a->g, b->g, c->g, w0, w1, w2, area);
                            u8 shade_b = ndsPreviewInterpolateU8(
                                a->b, b->b, c->b, w0, w1, w2, area);

                            texel = ndsPreviewModulateRGB15(
                                texel, shade_r, shade_g, shade_b);
                            (*texture_modulated_count)++;
                        }
                        ndsPreviewPutPixel(pixels, pitch, x, y, texel,
                                           pixel_count);
                        (*texture_sample_count)++;
                    }
                    else
                    {
                        ndsPreviewPutPixel(pixels, pitch, x, y, fallback,
                                           pixel_count);
                    }
                }
            }
        }
    }

    if (edge != 0)
    {
        ndsPreviewDrawLine(pixels, pitch, x0, y0, x1, y1, edge, pixel_count);
        ndsPreviewDrawLine(pixels, pitch, x1, y1, x2, y2, edge, pixel_count);
        ndsPreviewDrawLine(pixels, pitch, x2, y2, x0, y0, edge, pixel_count);
    }
}

static u32 ndsPreviewGeometryFlagsFromMode(u32 geometry_mode)
{
    u32 flags = 0;

    if (geometry_mode & NDS_GBI_GEOMETRY_ZBUFFER)
    {
        flags |= NDS_OPENING_ROOM_DL_GEOMETRY_ZBUFFER;
    }
    if (geometry_mode & NDS_GBI_GEOMETRY_SHADE)
    {
        flags |= NDS_OPENING_ROOM_DL_GEOMETRY_SHADE;
    }
    if (geometry_mode & NDS_GBI_GEOMETRY_CULL_FRONT)
    {
        flags |= NDS_OPENING_ROOM_DL_GEOMETRY_CULL_FRONT;
    }
    if (geometry_mode & NDS_GBI_GEOMETRY_CULL_BACK)
    {
        flags |= NDS_OPENING_ROOM_DL_GEOMETRY_CULL_BACK;
    }
    if (geometry_mode & NDS_GBI_GEOMETRY_LIGHTING)
    {
        flags |= NDS_OPENING_ROOM_DL_GEOMETRY_LIGHTING;
    }
    if (geometry_mode & NDS_GBI_GEOMETRY_SHADING_SMOOTH)
    {
        flags |= NDS_OPENING_ROOM_DL_GEOMETRY_SHADING_SMOOTH;
    }
    return flags;
}

static s32 ndsPreviewFallbackCoordA(const NDSDLPreviewVtx *vtx, u32 axis)
{
    switch (axis)
    {
    case NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XZ:
        return vtx->wx;

    case NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_YZ:
        return vtx->wy;

    case NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XY:
    default:
        return vtx->wx;
    }
}

static s32 ndsPreviewFallbackCoordB(const NDSDLPreviewVtx *vtx, u32 axis)
{
    switch (axis)
    {
    case NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XZ:
    case NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_YZ:
        return vtx->wz;

    case NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XY:
    default:
        return vtx->wy;
    }
}

static u32 ndsPreviewFallbackAxisArea(const NDSDLPreviewVtx *vertices,
                                      u32 vertex_count,
                                      const NDSDLPreviewTri *tris,
                                      u32 tri_count,
                                      u32 axis)
{
    u32 i;
    s64 total = 0;

    for (i = 0; i < tri_count; i++)
    {
        const NDSDLPreviewVtx *a;
        const NDSDLPreviewVtx *b;
        const NDSDLPreviewVtx *c;
        s64 x0;
        s64 y0;
        s64 x1;
        s64 y1;
        s64 x2;
        s64 y2;
        s64 area;

        if ((tris[i].v0 >= vertex_count) ||
            (tris[i].v1 >= vertex_count) ||
            (tris[i].v2 >= vertex_count))
        {
            continue;
        }

        a = &vertices[tris[i].v0];
        b = &vertices[tris[i].v1];
        c = &vertices[tris[i].v2];
        if ((a->valid == FALSE) || (b->valid == FALSE) ||
            (c->valid == FALSE))
        {
            continue;
        }

        x0 = ndsPreviewFallbackCoordA(a, axis);
        y0 = ndsPreviewFallbackCoordB(a, axis);
        x1 = ndsPreviewFallbackCoordA(b, axis);
        y1 = ndsPreviewFallbackCoordB(b, axis);
        x2 = ndsPreviewFallbackCoordA(c, axis);
        y2 = ndsPreviewFallbackCoordB(c, axis);
        area = ((x2 - x0) * (y1 - y0)) -
               ((y2 - y0) * (x1 - x0));
        if (area < 0)
        {
            area = -area;
        }
        total += area;
        if (total > 0xFFFFFFFFLL)
        {
            return 0xFFFFFFFFu;
        }
    }
    return (u32)total;
}

static u32 ndsPreviewChooseFallbackAxis(const NDSDLPreviewVtx *vertices,
                                        u32 vertex_count,
                                        const NDSDLPreviewTri *tris,
                                        u32 tri_count,
                                        u32 *area_out)
{
    u32 best_axis = NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XY;
    u32 best_area = ndsPreviewFallbackAxisArea(
        vertices, vertex_count, tris, tri_count, best_axis);
    u32 area = ndsPreviewFallbackAxisArea(
        vertices, vertex_count, tris, tri_count,
        NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XZ);

    if (area > best_area)
    {
        best_axis = NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XZ;
        best_area = area;
    }

    area = ndsPreviewFallbackAxisArea(
        vertices, vertex_count, tris, tri_count,
        NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_YZ);
    if (area > best_area)
    {
        best_axis = NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_YZ;
        best_area = area;
    }

    if (area_out != NULL)
    {
        *area_out = best_area;
    }
    return best_axis;
}

static s32 ndsPreviewFloatToS32(f32 value)
{
    return (s32)((value >= 0.0F) ? (value + 0.5F) : (value - 0.5F));
}

static s32 ndsPreviewFloatToCenti(f32 value)
{
    return ndsPreviewFloatToS32(value * 100.0F);
}

static void ndsOpeningRoomApplyDLPreviewTransform(DObj *dobj,
                                                  NDSDLPreviewVtx *vertices,
                                                  u32 vertex_count)
{
    f32 tx = 0.0F;
    f32 ty = 0.0F;
    f32 tz = 0.0F;
    f32 rx = 0.0F;
    f32 ry = 0.0F;
    f32 rz = 0.0F;
    f32 sx = 1.0F;
    f32 sy = 1.0F;
    f32 sz = 1.0F;
    f32 sinr;
    f32 cosr;
    f32 sinp;
    f32 cosp;
    f32 siny;
    f32 cosy;
    u32 mask = 0;
    u32 i;

    gNdsOpeningRoomDLPreviewFirstXObjKind = 0xffffffffu;

    if (dobj != NULL)
    {
        mask |= NDS_OPENING_ROOM_DL_PREVIEW_TRANSFORM_DOBJ;

        if (dobj->translate.xobj != NULL)
        {
            tx = dobj->translate.vec.f.x;
            ty = dobj->translate.vec.f.y;
            tz = dobj->translate.vec.f.z;
            mask |= NDS_OPENING_ROOM_DL_PREVIEW_TRANSFORM_TRANSLATE;
        }
        if (dobj->rotate.xobj != NULL)
        {
            rx = dobj->rotate.vec.f.x;
            ry = dobj->rotate.vec.f.y;
            rz = dobj->rotate.vec.f.z;
            mask |= NDS_OPENING_ROOM_DL_PREVIEW_TRANSFORM_ROTATE;
        }
        if (dobj->scale.xobj != NULL)
        {
            sx = dobj->scale.vec.f.x;
            sy = dobj->scale.vec.f.y;
            sz = dobj->scale.vec.f.z;
            mask |= NDS_OPENING_ROOM_DL_PREVIEW_TRANSFORM_SCALE;
        }
        gNdsOpeningRoomDLPreviewXObjCount = dobj->xobjs_num;
        if ((dobj->xobjs_num != 0) && (dobj->xobjs[0] != NULL))
        {
            gNdsOpeningRoomDLPreviewFirstXObjKind = dobj->xobjs[0]->kind;
            mask |= NDS_OPENING_ROOM_DL_PREVIEW_TRANSFORM_XOBJ;
        }
    }

    gNdsOpeningRoomDLPreviewTransformMask = mask;
    gNdsOpeningRoomDLPreviewTranslateX100 = ndsPreviewFloatToCenti(tx);
    gNdsOpeningRoomDLPreviewTranslateY100 = ndsPreviewFloatToCenti(ty);
    gNdsOpeningRoomDLPreviewTranslateZ100 = ndsPreviewFloatToCenti(tz);
    gNdsOpeningRoomDLPreviewRotateX100 = ndsPreviewFloatToCenti(rx);
    gNdsOpeningRoomDLPreviewRotateY100 = ndsPreviewFloatToCenti(ry);
    gNdsOpeningRoomDLPreviewRotateZ100 = ndsPreviewFloatToCenti(rz);
    gNdsOpeningRoomDLPreviewScaleX100 = ndsPreviewFloatToCenti(sx);
    gNdsOpeningRoomDLPreviewScaleY100 = ndsPreviewFloatToCenti(sy);
    gNdsOpeningRoomDLPreviewScaleZ100 = ndsPreviewFloatToCenti(sz);

    sinr = sinf(rx);
    cosr = cosf(rx);
    sinp = sinf(ry);
    cosp = cosf(ry);
    siny = sinf(rz);
    cosy = cosf(rz);

    for (i = 0; i < vertex_count; i++)
    {
        f32 x;
        f32 y;
        f32 z;

        if (vertices[i].valid == FALSE)
        {
            continue;
        }

        x = (f32)vertices[i].x;
        y = (f32)vertices[i].y;
        z = (f32)vertices[i].z;

        vertices[i].wx = ndsPreviewFloatToS32(
            (x * ((cosp * cosy) * sx)) +
            (y * (((sinr * sinp * cosy) - (cosr * siny)) * sy)) +
            (z * (((cosr * sinp * cosy) + (sinr * siny)) * sz)) +
            tx);
        vertices[i].wy = ndsPreviewFloatToS32(
            (x * ((cosp * siny) * sx)) +
            (y * (((sinr * sinp * siny) + (cosr * cosy)) * sy)) +
            (z * (((cosr * sinp * siny) - (sinr * cosy)) * sz)) +
            ty);
        vertices[i].wz = ndsPreviewFloatToS32(
            (x * (-sinp * sx)) +
            (y * ((sinr * cosp) * sy)) +
            (z * ((cosr * cosp) * sz)) +
            tz);
    }
}

static void ndsOpeningRoomInitMaterialProbe(
    NDSOpeningRoomMaterialProbe *probe)
{
    if (probe == NULL)
    {
        return;
    }

    probe->count = 0;
    probe->flags = 0;
    probe->effective_flags = 0;
    probe->mask = 0;
    probe->texture_curr = 0xffffffffu;
    probe->texture_next = 0xffffffffu;
    probe->palette_index = 0xffffffffu;
    probe->lfrac100 = 0;
    probe->format = 0xffffffffu;
    probe->size = 0xffffffffu;
    probe->block_format = 0xffffffffu;
    probe->block_size = 0xffffffffu;
    probe->tile_width = 0;
    probe->tile_height = 0;
    probe->scroll_width = 0;
    probe->scroll_height = 0;
    probe->scale_s100 = 0;
    probe->scale_t100 = 0;
    probe->translate_s100 = 0;
    probe->translate_t100 = 0;
    probe->sprite_array = 0;
    probe->palette_array = 0;
    probe->sprite_curr = 0;
    probe->sprite_next = 0;
    probe->palette_ptr = 0;
}

static u32 ndsOpeningRoomGetEffectiveMObjFlags(const MObj *mobj)
{
    u32 flags;

    if (mobj == NULL)
    {
        return MOBJ_FLAG_NONE;
    }

    flags = mobj->sub.flags;
    if (flags == MOBJ_FLAG_NONE)
    {
        flags = MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA;
    }
    return flags;
}

static s32 ndsOpeningRoomMaterialTrunc(f32 value)
{
    return (s32)value;
}

static u32 ndsOpeningRoomMaterialPositiveOrOne(s32 value)
{
    return (value <= 0) ? 1u : (u32)value;
}

static void ndsOpeningRoomComputeMaterialLoadBlock(
    const MObj *mobj,
    u32 *texels,
    u32 *dxt)
{
    s32 load_texels = 0;
    u32 divisor = 1;

    if ((mobj == NULL) || (texels == NULL) || (dxt == NULL))
    {
        return;
    }

    switch (mobj->sub.block_siz)
    {
    case G_IM_SIZ_4b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 3) >> 2) -
            1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 16);
        break;

    case G_IM_SIZ_8b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 1) >> 1) -
            1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 8);
        break;

    case G_IM_SIZ_16b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 2) / 8);
        break;

    case G_IM_SIZ_32b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsOpeningRoomMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 4) / 8);
        break;

    default:
        break;
    }

    *texels = (load_texels > 0) ? (u32)load_texels : 0u;
    *dxt = (divisor + 0x7ffu) / divisor;
}

static void ndsOpeningRoomMaterialTextureState(
    const MObj *mobj,
    u32 flags,
    f32 *scau,
    f32 *scav,
    f32 *trau,
    f32 *trav,
    f32 *scrollu,
    f32 *scrollv,
    u32 *mask)
{
    if ((mobj == NULL) || (scau == NULL) || (scav == NULL) ||
        (trau == NULL) || (trav == NULL) || (scrollu == NULL) ||
        (scrollv == NULL))
    {
        return;
    }

    if ((flags & (MOBJ_FLAG_TEXTURE | 0x40u | 0x20u)) == 0)
    {
        return;
    }

    *scau = mobj->sub.scau;
    *scav = mobj->sub.scav;
    *trau = mobj->sub.trau;
    *trav = mobj->sub.trav;
    *scrollu = mobj->sub.scrollu;
    *scrollv = mobj->sub.scrollv;

    if (mobj->sub.unk10 == 1)
    {
        *scau *= 0.5F;
        *trau =
            ((*trau - mobj->sub.unk24) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        *scrollu =
            ((*scrollu - mobj->sub.unk44) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        if (mask != NULL)
        {
            *mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_ADJUSTED_ST;
        }
    }
}

static void ndsOpeningRoomRecordMaterialBranch(DObj *dobj)
{
    MObj *mobj;
    u32 count = 0;
    u32 table_commands = 0;
    u32 generated_commands = 0;

    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        return;
    }

    for (mobj = dobj->mobj; (mobj != NULL) && (count < 64u);
         mobj = mobj->next)
    {
        f32 scau = 0.0F;
        f32 scav = 0.0F;
        f32 trau = 0.0F;
        f32 trav = 0.0F;
        f32 scrollu = 0.0F;
        f32 scrollv = 0.0F;
        u32 flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);
        u32 mask = NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TABLE;
        u32 mobj_generated = 0;
        s32 uls;
        s32 ult;
        s32 s;
        s32 t;

        count++;
        table_commands++;
        if (count == 1u)
        {
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_SEGMENT;
        }

        ndsOpeningRoomMaterialTextureState(
            mobj,
            flags,
            &scau,
            &scav,
            &trau,
            &trav,
            &scrollu,
            &scrollv,
            &mask);

        if ((flags & MOBJ_FLAG_PALETTE) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_IMAGE;
            if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
            {
                mobj_generated += 5u;
                mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_TLUT;
            }
        }
        if ((flags & MOBJ_FLAG_LIGHT1) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LIGHT;
        }
        if ((flags & MOBJ_FLAG_LIGHT2) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LIGHT;
        }
        if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PRIMCOLOR;
        }
        if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_ENVCOLOR;
        }
        if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_BLENDCOLOR;
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_NEXT_IMAGE;
            if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
            {
                u32 texels = 0;
                u32 dxt = 0;

                mobj_generated += 3u;
                mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LOADBLOCK;
                ndsOpeningRoomComputeMaterialLoadBlock(mobj, &texels, &dxt);
                if (count == 1u)
                {
                    gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockTexels =
                        texels;
                    gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockDxt = dxt;
                }
            }
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_CURRENT_IMAGE;
        }
        if ((flags & 0x20u) != 0)
        {
            if (mobj->sub.unk10 == 2)
            {
                uls = (fabsf(scau) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        (((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) :
                    0;
                ult = (fabsf(scav) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        (((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) :
                    0;
                if (uls < 0)
                {
                    uls = 0;
                }
                if (ult < 0)
                {
                    ult = 0;
                }
            }
            else
            {
                uls = (fabsf(scau) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        ((((f32)mobj->sub.unk0C * trau) +
                          (f32)mobj->sub.unk0A) /
                         scau) *
                        4.0F) :
                    0;
                ult = (fabsf(scav) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        (((((1.0F - scav) - trav) *
                           (f32)mobj->sub.unk0E) +
                          (f32)mobj->sub.unk0A) /
                         scav) *
                        4.0F) :
                    0;
            }
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TILESIZE;
            if (count == 1u)
            {
                gNdsOpeningRoomDrawMaterialBranchFirstTileUls = uls;
                gNdsOpeningRoomDrawMaterialBranchFirstTileUlt = ult;
                gNdsOpeningRoomDrawMaterialBranchFirstTileLrs =
                    (((s32)mobj->sub.unk0C - 1) << 2) + uls;
                gNdsOpeningRoomDrawMaterialBranchFirstTileLrt =
                    (((s32)mobj->sub.unk0E - 1) << 2) + ult;
            }
        }
        if ((flags & 0x40u) != 0)
        {
            uls = (fabsf(scau) > (1.0F / 65535.0F)) ?
                ndsOpeningRoomMaterialTrunc(
                    ((((f32)mobj->sub.unk38 * scrollu) +
                      (f32)mobj->sub.unk0A) /
                     scau) *
                    4.0F) :
                0;
            ult = (fabsf(scav) > (1.0F / 65535.0F)) ?
                ndsOpeningRoomMaterialTrunc(
                    (((((1.0F - scav) - scrollv) *
                       (f32)mobj->sub.unk3A) +
                      (f32)mobj->sub.unk0A) /
                     scav) *
                    4.0F) :
                0;
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_SCROLL_TILESIZE;
            if (count == 1u)
            {
                gNdsOpeningRoomDrawMaterialBranchFirstScrollUls = uls;
                gNdsOpeningRoomDrawMaterialBranchFirstScrollUlt = ult;
                gNdsOpeningRoomDrawMaterialBranchFirstScrollLrs =
                    (((s32)mobj->sub.unk38 - 1) << 2) + uls;
                gNdsOpeningRoomDrawMaterialBranchFirstScrollLrt =
                    (((s32)mobj->sub.unk3A - 1) << 2) + ult;
            }
        }
        if ((flags & MOBJ_FLAG_TEXTURE) != 0)
        {
            if (mobj->sub.unk10 == 2)
            {
                s = (fabsf(scau) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        ((f32)mobj->sub.unk0C * 64.0F) / scau) :
                    0;
                t = (fabsf(scav) > (1.0F / 65535.0F)) ?
                    ndsOpeningRoomMaterialTrunc(
                        ((f32)mobj->sub.unk0E * 64.0F) / scav) :
                    0;
            }
            else
            {
                s = ((mobj->sub.unk08 != 0) &&
                     (fabsf(scau) > (1.0F / 65535.0F))) ?
                    ndsOpeningRoomMaterialTrunc(
                        (2097152.0F / (f32)mobj->sub.unk08) / scau) :
                    0;
                t = ((mobj->sub.unk08 != 0) &&
                     (fabsf(scav) > (1.0F / 65535.0F))) ?
                    ndsOpeningRoomMaterialTrunc(
                        (2097152.0F / (f32)mobj->sub.unk08) / scav) :
                    0;
            }
            if (s > 0xffff)
            {
                s = 0xffff;
            }
            if (t > 0xffff)
            {
                t = 0xffff;
            }
            mobj_generated++;
            mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TEXTURE;
            if (count == 1u)
            {
                gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleS = (u32)s;
                gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleT = (u32)t;
            }
        }

        mobj_generated++;
        mask |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_END;
        generated_commands += mobj_generated;

        if (count == 1u)
        {
            gNdsOpeningRoomDrawMaterialBranchFirstMask = mask;
            gNdsOpeningRoomDrawMaterialBranchFirstGeneratedCommands =
                mobj_generated;
        }
    }

    if (count != 0)
    {
        gNdsOpeningRoomDrawMaterialBranchResult =
            NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PASS;
        gNdsOpeningRoomDrawMaterialBranchMObjCount = count;
        gNdsOpeningRoomDrawMaterialBranchSegmentCommands = 1;
        gNdsOpeningRoomDrawMaterialBranchTableCommands = table_commands;
        gNdsOpeningRoomDrawMaterialBranchGeneratedCommands =
            generated_commands;
    }
}

static void ndsOpeningRoomEmitBranchTableCommand(Gfx *cmd, const Gfx *branch)
{
    cmd->words.w0 = NDS_GBI_OP_DL << 24;
    cmd->words.w1 = (u32)(uintptr_t)branch;
}

static void ndsOpeningRoomEmitEndDL(Gfx *cmd)
{
    cmd->words.w0 = NDS_GBI_OP_ENDDL << 24;
    cmd->words.w1 = 0;
}

static void ndsOpeningRoomEmitPrimColor(Gfx *cmd, const MObj *mobj)
{
    u32 lfrac;

    lfrac = (u32)ndsPreviewFloatToS32(mobj->lfrac * 255.0F);
    if (lfrac > 0xffu)
    {
        lfrac = 0xffu;
    }

    cmd->words.w0 =
        (NDS_GBI_OP_SETPRIMCOLOR << 24) |
        (((u32)mobj->sub.prim_m & 0xffu) << 8) |
        (lfrac & 0xffu);
    cmd->words.w1 =
        ((u32)mobj->sub.primcolor.s.r << 24) |
        ((u32)mobj->sub.primcolor.s.g << 16) |
        ((u32)mobj->sub.primcolor.s.b << 8) |
        (u32)mobj->sub.primcolor.s.a;
}

static void ndsOpeningRoomEmitEnvColor(Gfx *cmd, const MObj *mobj)
{
    cmd->words.w0 = NDS_GBI_OP_SETENVCOLOR << 24;
    cmd->words.w1 =
        ((u32)mobj->sub.envcolor.s.r << 24) |
        ((u32)mobj->sub.envcolor.s.g << 16) |
        ((u32)mobj->sub.envcolor.s.b << 8) |
        (u32)mobj->sub.envcolor.s.a;
}

static void ndsOpeningRoomEmitTextureImage(Gfx *cmd, const MObj *mobj)
{
    cmd->words.w0 =
        (NDS_GBI_OP_SETTIMG << 24) |
        (((u32)mobj->sub.fmt & 0x7u) << 21) |
        (((u32)mobj->sub.siz & 0x3u) << 19);
    cmd->words.w1 =
        (mobj->sub.sprites != NULL) ?
            (u32)(uintptr_t)mobj->sub.sprites[mobj->texture_id_curr] :
            0u;
}

static u32 ndsOpeningRoomMaterialEmitUnsupportedMask(u32 flags)
{
    u32 unsupported = 0;

    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_IMAGE;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PALETTE_TLUT;
        }
    }
    if ((flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LIGHT;
    }
    if ((flags & (MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_PRIMCOLOR;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_BLENDCOLOR;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_NEXT_IMAGE;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_LOADBLOCK;
        }
    }
    if ((flags & 0x20u) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TILESIZE;
    }
    if ((flags & 0x40u) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_SCROLL_TILESIZE;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        unsupported |= NDS_OPENING_ROOM_DRAW_MATERIAL_BRANCH_TEXTURE;
    }
    return unsupported;
}

static void ndsOpeningRoomCaptureFirstEmittedBranch(const Gfx *branch,
                                                    u32 commands)
{
    if ((branch == NULL) || (commands == 0))
    {
        return;
    }
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_0 = branch[0].words.w0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_0 = branch[0].words.w1;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp0 =
        branch[0].words.w0 >> 24;
    if (commands < 2u)
    {
        return;
    }
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_1 = branch[1].words.w0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_1 = branch[1].words.w1;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp1 =
        branch[1].words.w0 >> 24;
    if (commands < 3u)
    {
        return;
    }
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_2 = branch[2].words.w0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_2 = branch[2].words.w1;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp2 =
        branch[2].words.w0 >> 24;
}

static void ndsOpeningRoomCaptureMaterialBranchPreviewState(void)
{
    const Gfx *table;
    uintptr_t heap_start;
    uintptr_t heap_after;
    u32 table_commands;
    u32 generated_commands;
    u32 total_commands = 0;
    u32 prim_count = 0;
    u32 end_count = 0;
    u32 i;

    if (gNdsOpeningRoomDLPreviewMaterialBranchResult != 0)
    {
        return;
    }
    if (gNdsOpeningRoomDrawMaterialEmitResult !=
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_PASS)
    {
        gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NO_EMIT;
        return;
    }

    heap_start = (uintptr_t)gNdsOpeningRoomDrawMaterialEmitHeapStart;
    heap_after = (uintptr_t)gNdsOpeningRoomDrawMaterialEmitHeapAfter;
    table_commands = gNdsOpeningRoomDrawMaterialEmitTableCommands;
    generated_commands = gNdsOpeningRoomDrawMaterialEmitGeneratedCommands;
    if ((heap_start == 0) || (heap_after <= heap_start) ||
        (table_commands == 0) || (table_commands > 16u) ||
        (generated_commands == 0) || (generated_commands > 64u) ||
        (((uintptr_t)(table_commands + generated_commands) * sizeof(Gfx)) >
         (heap_after - heap_start)))
    {
        gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NO_BRANCH;
        return;
    }

    table = (const Gfx *)heap_start;
    for (i = 0; i < table_commands; i++)
    {
        const Gfx *branch;
        uintptr_t branch_start;
        u32 branch_budget = generated_commands;
        u32 saw_end = FALSE;

        if ((table[i].words.w0 >> 24) != NDS_GBI_OP_DL)
        {
            gNdsOpeningRoomDLPreviewMaterialBranchUnsupportedOp =
                table[i].words.w0 >> 24;
            gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
                NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_UNSUPPORTED;
            return;
        }

        branch_start = (uintptr_t)table[i].words.w1;
        if ((branch_start < heap_start) || (branch_start >= heap_after) ||
            ((branch_start & (sizeof(Gfx) - 1u)) != 0))
        {
            gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
                NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NO_BRANCH;
            return;
        }
        branch = (const Gfx *)branch_start;

        while (branch_budget-- != 0)
        {
            u32 w0 = branch->words.w0;
            u32 op = w0 >> 24;

            if ((((uintptr_t)(branch + 1)) > heap_after) ||
                (total_commands >= generated_commands))
            {
                gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
                    NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NO_BRANCH;
                return;
            }

            total_commands++;
            if (total_commands == 1u)
            {
                gNdsOpeningRoomDLPreviewMaterialBranchFirstOp = op;
            }
            else if (total_commands == 2u)
            {
                gNdsOpeningRoomDLPreviewMaterialBranchSecondOp = op;
            }

            switch (op)
            {
            case NDS_GBI_OP_SETPRIMCOLOR:
                prim_count++;
                gNdsOpeningRoomDLPreviewMaterialBranchPrimColor =
                    branch->words.w1;
                gNdsOpeningRoomDLPreviewMaterialBranchPrimLod = w0 & 0xffu;
                gNdsOpeningRoomDLPreviewMaterialBranchPrimM =
                    (w0 >> 8) & 0xffu;
                branch++;
                break;

            case NDS_GBI_OP_ENDDL:
                end_count++;
                saw_end = TRUE;
                branch_budget = 0;
                branch++;
                break;

            default:
                gNdsOpeningRoomDLPreviewMaterialBranchUnsupportedOp = op;
                gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
                    NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_UNSUPPORTED;
                return;
            }
        }

        if (saw_end == FALSE)
        {
            gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
                NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NO_END;
            return;
        }
    }

    gNdsOpeningRoomDLPreviewMaterialBranchCommandCount = total_commands;
    gNdsOpeningRoomDLPreviewMaterialBranchPrimCount = prim_count;
    gNdsOpeningRoomDLPreviewMaterialBranchEndCount = end_count;

    if (prim_count == 0)
    {
        gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NO_PRIM;
        return;
    }
    if (end_count != table_commands)
    {
        gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NO_END;
        return;
    }

    gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NONE;
    gNdsOpeningRoomDLPreviewMaterialBranchResult =
        NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_PASS;
}

static void ndsOpeningRoomProbeMaterialDLShape(void)
{
    const Gfx *dl = sNdsOpeningRoomMaterialPreviewDL;
    u32 command_count = 0;
    u32 vertex_count = 0;
    u32 triangle_count = 0;
    u32 unsupported = 0;
    u32 saw_end = FALSE;
    u32 i;

    if (gNdsOpeningRoomMaterialDLProbeResult != 0)
    {
        return;
    }

    gNdsOpeningRoomMaterialDLProbeBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NONE;
    gNdsOpeningRoomMaterialDLProbeFirstDL = (u32)(uintptr_t)dl;
    gNdsOpeningRoomMaterialDLProbeCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeVertexCount = 0;
    gNdsOpeningRoomMaterialDLProbeTriangleCount = 0;
    gNdsOpeningRoomMaterialDLProbeFirstOpcode = 0;
    gNdsOpeningRoomMaterialDLProbeUnsupportedOpcode = 0;
    gNdsOpeningRoomMaterialDLProbeVertexCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeTriangleCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeSyncCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeEndCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeBranchCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeOtherModeCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeUnsupportedCommandCount = 0;

    if (sNdsOpeningRoomMaterialPreviewDObj == NULL)
    {
        gNdsOpeningRoomMaterialDLProbeBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NO_CANDIDATE;
        return;
    }
    if (dl == NULL)
    {
        gNdsOpeningRoomMaterialDLProbeBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NO_DL;
        return;
    }

    for (i = 0; i < 128u; i++, dl++)
    {
        u32 w0 = dl->words.w0;
        u32 op = w0 >> 24;

        if (gNdsOpeningRoomMaterialDLProbeFirstOpcode == 0)
        {
            gNdsOpeningRoomMaterialDLProbeFirstOpcode = op;
        }
        command_count++;

        switch (op)
        {
        case NDS_GBI_OP_VTX:
        {
            u32 v0;
            u32 count;

            gNdsOpeningRoomMaterialDLProbeVertexCommandCount++;
            if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_DL_PREVIEW_MAX_VTX, &v0,
                                      &count) == FALSE)
            {
                break;
            }
            if ((v0 + count) > vertex_count)
            {
                vertex_count = v0 + count;
            }
            break;
        }

        case NDS_GBI_OP_TRI1:
            gNdsOpeningRoomMaterialDLProbeTriangleCommandCount++;
            triangle_count++;
            break;

        case NDS_GBI_OP_TRI2:
            gNdsOpeningRoomMaterialDLProbeTriangleCommandCount++;
            triangle_count += 2u;
            break;

        case NDS_GBI_OP_ENDDL:
            gNdsOpeningRoomMaterialDLProbeEndCommandCount++;
            saw_end = TRUE;
            i = 128u;
            break;

        case NDS_GBI_OP_TEXTURE:
        case NDS_GBI_OP_POPMTX:
        case NDS_GBI_OP_SPECIAL_1:
        case NDS_GBI_OP_MTX:
        case NDS_GBI_OP_MOVEWORD:
        case NDS_GBI_OP_GEOMETRYMODE:
        case NDS_GBI_OP_SETCOMBINE:
        case NDS_GBI_OP_SETTIMG:
        case NDS_GBI_OP_SETTILE:
        case NDS_GBI_OP_LOADBLOCK:
        case NDS_GBI_OP_SETTILESIZE:
            break;

        case NDS_GBI_OP_RDPPIPESYNC:
        case NDS_GBI_OP_RDPLOADSYNC:
        case NDS_GBI_OP_RDPTILESYNC:
        case NDS_GBI_OP_RDPFULLSYNC:
            gNdsOpeningRoomMaterialDLProbeSyncCommandCount++;
            break;

        case NDS_GBI_OP_DL:
        case NDS_GBI_OP_CULLDL:
            gNdsOpeningRoomMaterialDLProbeBranchCommandCount++;
            if (unsupported == 0)
            {
                unsupported = op;
            }
            gNdsOpeningRoomMaterialDLProbeUnsupportedCommandCount++;
            break;

        case NDS_GBI_OP_SETOTHERMODE_H:
        case NDS_GBI_OP_SETOTHERMODE_L:
        case NDS_GBI_OP_RDPSETOTHERMODE:
            gNdsOpeningRoomMaterialDLProbeOtherModeCommandCount++;
            if (unsupported == 0)
            {
                unsupported = op;
            }
            gNdsOpeningRoomMaterialDLProbeUnsupportedCommandCount++;
            break;

        default:
            if (unsupported == 0)
            {
                unsupported = op;
            }
            gNdsOpeningRoomMaterialDLProbeUnsupportedCommandCount++;
            break;
        }
    }

    gNdsOpeningRoomMaterialDLProbeCommandCount = command_count;
    gNdsOpeningRoomMaterialDLProbeVertexCount = vertex_count;
    gNdsOpeningRoomMaterialDLProbeTriangleCount = triangle_count;
    gNdsOpeningRoomMaterialDLProbeUnsupportedOpcode = unsupported;

    if (unsupported != 0)
    {
        gNdsOpeningRoomMaterialDLProbeBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_UNSUPPORTED;
        return;
    }
    if (vertex_count == 0)
    {
        gNdsOpeningRoomMaterialDLProbeBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NO_VERTICES;
        return;
    }
    if (triangle_count == 0)
    {
        gNdsOpeningRoomMaterialDLProbeBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NO_TRIANGLES;
        return;
    }
    if (saw_end == FALSE)
    {
        gNdsOpeningRoomMaterialDLProbeBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NO_END;
        return;
    }

    gNdsOpeningRoomMaterialDLProbeResult =
        NDS_OPENING_ROOM_MATERIAL_DL_PROBE_PASS;
}

#define NDS_MATERIAL_DL_EXPAND_MAX_DEPTH 4u
#define NDS_MATERIAL_DL_EXPAND_MAX_COMMANDS 256u
#define NDS_MATERIAL_DL_EXPAND_MAX_LIST_COMMANDS 128u

static s32 ndsOpeningRoomPointerRangeInTaskmanGraphicsHeap(const void *ptr,
                                                           size_t size)
{
    uintptr_t start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    uintptr_t end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    uintptr_t addr = (uintptr_t)ptr;

    if ((ptr == NULL) || (start == 0) || (end <= start) ||
        (addr < start) || (addr > end) || (size > (size_t)(end - addr)))
    {
        return FALSE;
    }
    return TRUE;
}

static s32 ndsOpeningRoomPointerRangeInLoadedFiles(const void *ptr,
                                                   size_t size)
{
    u32 i;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(
                &sNdsRelocLoadedFiles[i], ptr, size) != FALSE)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsOpeningRoomRendererValidateRange(const Gfx *dl,
                                               size_t bytes,
                                               void *user)
{
    uintptr_t addr = (uintptr_t)dl;

    (void)user;

    if ((dl == NULL) || ((addr & 0x3u) != 0))
    {
        return FALSE;
    }
    if (ndsOpeningRoomPointerRangeInLoadedFiles(dl, bytes) != FALSE)
    {
        return TRUE;
    }
    return ndsOpeningRoomPointerRangeInTaskmanGraphicsHeap(dl, bytes);
}

static const Gfx *ndsOpeningRoomRendererResolveBranch(const Gfx *dl,
                                                      u32 *resolve_kind,
                                                      void *user)
{
    uintptr_t raw = (uintptr_t)dl;
    uintptr_t heap_start;
    uintptr_t heap_after;
    uintptr_t offset;

    (void)user;

    if (resolve_kind != NULL)
    {
        *resolve_kind = NDS_RENDERER_RESOLVE_NONE;
    }
    if ((raw >> 24) != 0x0Eu)
    {
        return dl;
    }
    if (gNdsOpeningRoomDrawMaterialEmitResult !=
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_PASS)
    {
        return dl;
    }

    heap_start = (uintptr_t)gNdsOpeningRoomDrawMaterialEmitHeapStart;
    heap_after = (uintptr_t)gNdsOpeningRoomDrawMaterialEmitHeapAfter;
    offset = raw & 0x00FFFFFFu;
    if ((heap_start == 0) || (heap_after <= heap_start) ||
        (offset > (heap_after - heap_start)) ||
        (sizeof(Gfx) > (size_t)(heap_after - heap_start - offset)))
    {
        return dl;
    }

    if (resolve_kind != NULL)
    {
        *resolve_kind = NDS_RENDERER_RESOLVE_SEGMENT;
    }
    return (const Gfx *)(heap_start + offset);
}

static const void *ndsOpeningRoomRendererResolveData(const void *ptr,
                                                     size_t bytes,
                                                     void *user)
{
    uintptr_t raw = (uintptr_t)ptr;
    uintptr_t heap_start;
    uintptr_t heap_after;
    uintptr_t offset;

    (void)user;

    if ((ndsOpeningRoomPointerRangeInLoadedFiles(ptr, bytes) != FALSE) ||
        (ndsOpeningRoomPointerRangeInTaskmanGraphicsHeap(ptr, bytes) != FALSE))
    {
        return ptr;
    }
    if ((raw >> 24) != 0x0Eu)
    {
        return NULL;
    }
    if (gNdsOpeningRoomDrawMaterialEmitResult !=
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_PASS)
    {
        return NULL;
    }

    heap_start = (uintptr_t)gNdsOpeningRoomDrawMaterialEmitHeapStart;
    heap_after = (uintptr_t)gNdsOpeningRoomDrawMaterialEmitHeapAfter;
    offset = raw & 0x00FFFFFFu;
    if ((heap_start == 0) || (heap_after <= heap_start) ||
        (offset > (heap_after - heap_start)) ||
        (bytes > (size_t)(heap_after - heap_start - offset)))
    {
        return NULL;
    }

    return (const void *)(heap_start + offset);
}

static u32 ndsOpeningRoomMaterialDLExpandMapRendererBlocker(u32 blocker)
{
    switch (blocker)
    {
    case NDS_RENDERER_BLOCKER_NONE:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NONE;
    case NDS_RENDERER_BLOCKER_BAD_BRANCH:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_BAD_BRANCH;
    case NDS_RENDERER_BLOCKER_TOO_DEEP:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_TOO_DEEP;
    case NDS_RENDERER_BLOCKER_BUDGET:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_BUDGET;
    case NDS_RENDERER_BLOCKER_UNSUPPORTED:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_UNSUPPORTED;
    case NDS_RENDERER_BLOCKER_NO_VERTICES:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NO_VERTICES;
    case NDS_RENDERER_BLOCKER_NO_TRIANGLES:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NO_TRIANGLES;
    case NDS_RENDERER_BLOCKER_NO_END:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NO_END;
    default:
        return NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_UNSUPPORTED;
    }
}

static void ndsOpeningRoomProbeMaterialDLExpandedShape(void)
{
    const Gfx *dl = sNdsOpeningRoomMaterialPreviewDL;
    NDSRendererConfig config;
    NDSRendererStats stats;

    if (gNdsOpeningRoomMaterialDLExpandResult != 0)
    {
        return;
    }

    gNdsOpeningRoomMaterialDLExpandBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NONE;
    gNdsOpeningRoomMaterialDLExpandFirstDL = (u32)(uintptr_t)dl;
    gNdsOpeningRoomMaterialDLExpandFirstBranchDL = 0;
    gNdsOpeningRoomMaterialDLExpandFirstResolvedBranchDL = 0;
    gNdsOpeningRoomMaterialDLExpandCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandVertexCount = 0;
    gNdsOpeningRoomMaterialDLExpandTriangleCount = 0;
    gNdsOpeningRoomMaterialDLExpandFirstOpcode = 0;
    gNdsOpeningRoomMaterialDLExpandUnsupportedOpcode = 0;
    gNdsOpeningRoomMaterialDLExpandVertexCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandTriangleCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandSyncCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandEndCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandBranchCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandBranchCallCount = 0;
    gNdsOpeningRoomMaterialDLExpandBranchJumpCount = 0;
    gNdsOpeningRoomMaterialDLExpandSegmentResolveCount = 0;
    gNdsOpeningRoomMaterialDLExpandOtherModeCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandColorCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandUnsupportedCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandMaxDepth = 0;

    if (sNdsOpeningRoomMaterialPreviewDObj == NULL)
    {
        gNdsOpeningRoomMaterialDLExpandBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NO_CANDIDATE;
        return;
    }
    if (dl == NULL)
    {
        gNdsOpeningRoomMaterialDLExpandBlocker =
            NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NO_DL;
        return;
    }

    config.max_depth = NDS_MATERIAL_DL_EXPAND_MAX_DEPTH;
    config.max_commands = NDS_MATERIAL_DL_EXPAND_MAX_COMMANDS;
    config.max_list_commands = NDS_MATERIAL_DL_EXPAND_MAX_LIST_COMMANDS;
    config.initial_projection = NULL;
    config.initial_modelview = NULL;
    config.initial_geometry_mode = 0u;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsOpeningRoomRendererValidateRange;
    config.immutable_command_span = NULL;
    config.resolve_branch = ndsOpeningRoomRendererResolveBranch;
    config.resolve_data = ndsOpeningRoomRendererResolveData;
    config.user = NULL;

    ndsRendererInitStats(&stats);
    ndsRendererScanDisplayList(dl, &config, &stats);

    gNdsOpeningRoomMaterialDLExpandFirstBranchDL =
        (u32)(uintptr_t)stats.first_branch_dl;
    gNdsOpeningRoomMaterialDLExpandFirstResolvedBranchDL =
        (u32)(uintptr_t)stats.first_resolved_branch_dl;
    gNdsOpeningRoomMaterialDLExpandCommandCount = stats.command_count;
    gNdsOpeningRoomMaterialDLExpandVertexCount = stats.vertex_count;
    gNdsOpeningRoomMaterialDLExpandTriangleCount = stats.triangle_count;
    gNdsOpeningRoomMaterialDLExpandFirstOpcode = stats.first_opcode;
    gNdsOpeningRoomMaterialDLExpandUnsupportedOpcode =
        stats.unsupported_opcode;
    gNdsOpeningRoomMaterialDLExpandVertexCommandCount =
        stats.vertex_command_count;
    gNdsOpeningRoomMaterialDLExpandTriangleCommandCount =
        stats.triangle_command_count;
    gNdsOpeningRoomMaterialDLExpandSyncCommandCount =
        stats.sync_command_count;
    gNdsOpeningRoomMaterialDLExpandEndCommandCount = stats.end_command_count;
    gNdsOpeningRoomMaterialDLExpandBranchCommandCount =
        stats.branch_command_count;
    gNdsOpeningRoomMaterialDLExpandBranchCallCount =
        stats.branch_call_count;
    gNdsOpeningRoomMaterialDLExpandBranchJumpCount =
        stats.branch_jump_count;
    gNdsOpeningRoomMaterialDLExpandSegmentResolveCount =
        stats.segment_resolve_count;
    gNdsOpeningRoomMaterialDLExpandOtherModeCommandCount =
        stats.othermode_command_count;
    gNdsOpeningRoomMaterialDLExpandColorCommandCount =
        stats.color_command_count;
    gNdsOpeningRoomMaterialDLExpandUnsupportedCommandCount =
        stats.unsupported_command_count;
    gNdsOpeningRoomMaterialDLExpandMaxDepth = stats.max_depth_seen;
    gNdsOpeningRoomMaterialDLExpandBlocker =
        ndsOpeningRoomMaterialDLExpandMapRendererBlocker(stats.blocker);

    if (gNdsOpeningRoomMaterialDLExpandBlocker !=
        NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NONE)
    {
        return;
    }

    gNdsOpeningRoomMaterialDLExpandResult =
        NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_PASS;
}

static void ndsOpeningRoomEmitMaterialBranch(DObj *dobj)
{
    MObj *mobj;
    Gfx *new_dl;
    Gfx *branch_dl;
    Gfx *first_branch;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_ptr;
    size_t heap_bytes;
    u32 count = 0;
    u32 generated = 0;
    u32 first_generated = 0;
    u32 unsupported = 0;

    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NO_MOBJ;
        return;
    }

    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        u32 flags;
        u32 mobj_generated = 0;

        count++;
        if (count > 64u)
        {
            gNdsOpeningRoomDrawMaterialEmitBlocker =
                NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_TOO_MANY_MOBJS;
            return;
        }

        flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);
        unsupported |= ndsOpeningRoomMaterialEmitUnsupportedMask(flags);
        if ((flags & MOBJ_FLAG_PRIMCOLOR) != 0)
        {
            mobj_generated++;
        }
        if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
        {
            mobj_generated++;
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            mobj_generated++;
        }
        mobj_generated++;
        generated += mobj_generated;
        if (count == 1u)
        {
            first_generated = mobj_generated;
        }
    }

    if (unsupported != 0)
    {
        gNdsOpeningRoomDrawMaterialEmitUnsupportedMask = unsupported;
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_UNSUPPORTED_FAMILY;
        return;
    }
    if ((gSYTaskmanGraphicsHeap.ptr == NULL) ||
        (gSYTaskmanGraphicsHeap.start == NULL) ||
        (gSYTaskmanGraphicsHeap.end == NULL))
    {
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NO_HEAP;
        return;
    }

    heap_start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    heap_end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    heap_ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    heap_bytes = (size_t)(count + generated) * sizeof(Gfx);

    if ((heap_ptr < heap_start) || (heap_ptr > heap_end) ||
        (heap_bytes > (size_t)(heap_end - heap_ptr)))
    {
        gNdsOpeningRoomDrawMaterialEmitBlocker =
            NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_HEAP_RANGE;
        return;
    }

    new_dl = (Gfx *)gSYTaskmanGraphicsHeap.ptr;
    branch_dl = new_dl + count;
    first_branch = branch_dl;

    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        u32 flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);

        ndsOpeningRoomEmitBranchTableCommand(new_dl++, branch_dl);
        if ((flags & MOBJ_FLAG_PRIMCOLOR) != 0)
        {
            ndsOpeningRoomEmitPrimColor(branch_dl++, mobj);
        }
        if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
        {
            ndsOpeningRoomEmitEnvColor(branch_dl++, mobj);
        }
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            ndsOpeningRoomEmitTextureImage(branch_dl++, mobj);
        }
        ndsOpeningRoomEmitEndDL(branch_dl++);
    }

    gSYTaskmanGraphicsHeap.ptr = branch_dl;
    gNdsOpeningRoomDrawMaterialEmitResult =
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_PASS;
    gNdsOpeningRoomDrawMaterialEmitBlocker =
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NONE;
    gNdsOpeningRoomDrawMaterialEmitMObjCount = count;
    gNdsOpeningRoomDrawMaterialEmitTableCommands = count;
    gNdsOpeningRoomDrawMaterialEmitGeneratedCommands = generated;
    gNdsOpeningRoomDrawMaterialEmitHeapStart = (u32)heap_ptr;
    gNdsOpeningRoomDrawMaterialEmitBranchStart =
        (u32)(uintptr_t)(new_dl);
    gNdsOpeningRoomDrawMaterialEmitHeapAfter = (u32)(uintptr_t)branch_dl;
    gNdsOpeningRoomDrawMaterialEmitHeapBytes =
        (u32)((uintptr_t)branch_dl - heap_ptr);
    gNdsOpeningRoomDrawMaterialEmitFirstTableOp =
        ((Gfx *)heap_ptr)[0].words.w0 >> 24;
    ndsOpeningRoomCaptureFirstEmittedBranch(first_branch, first_generated);
}

static u32 ndsOpeningRoomReadPointerArrayEntry(void **array, u32 index)
{
    if ((array == NULL) || (index >= 64u) ||
        (ndsOpeningRoomPointerRangeInLoadedFiles(
             array, ((size_t)index + 1u) * sizeof(*array)) == FALSE))
    {
        return 0;
    }
    return (u32)(uintptr_t)array[index];
}

static u32 ndsOpeningRoomMakeTextureMObjMask(const MObj *mobj,
                                             u32 effective_flags,
                                             u32 *out_sprite_curr,
                                             u32 *out_sprite_next)
{
    u32 mask = NDS_OPENING_ROOM_DL_MATERIAL_HAS_MOBJ;
    u32 sprite_curr = 0;
    u32 sprite_next = 0;

    if (mobj == NULL)
    {
        return 0;
    }
    if (mobj->sub.flags == MOBJ_FLAG_NONE)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_DEFAULT_FLAGS;
    }
    if ((effective_flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_TEXTURE;
    }
    if ((effective_flags & MOBJ_FLAG_ALPHA) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_ALPHA;
    }
    if ((effective_flags & 0x20u) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_TILESIZE;
    }
    if ((effective_flags & 0x40u) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_SCROLL_TILESIZE;
    }
    if ((effective_flags & MOBJ_FLAG_PALETTE) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_PALETTE;
    }
    if ((effective_flags & MOBJ_FLAG_FRAC) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_FRAC;
    }
    if ((effective_flags & MOBJ_FLAG_SPLIT) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_SPLIT;
    }
    if ((effective_flags &
         (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_ENVCOLOR |
          MOBJ_FLAG_BLENDCOLOR)) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_COLOR;
    }
    if ((effective_flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_LIGHT;
    }
    if (mobj->sub.sprites != NULL)
    {
        mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_SPRITE_ARRAY;
        if ((effective_flags & (MOBJ_FLAG_ALPHA | MOBJ_FLAG_FRAC)) != 0)
        {
            sprite_curr = ndsOpeningRoomReadPointerArrayEntry(
                mobj->sub.sprites, mobj->texture_id_curr);
            if (sprite_curr != 0)
            {
                mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_CURR_SPRITE;
            }
        }
        if ((effective_flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            sprite_next = ndsOpeningRoomReadPointerArrayEntry(
                mobj->sub.sprites, mobj->texture_id_next);
            if (sprite_next != 0)
            {
                mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_NEXT_SPRITE;
            }
        }
    }

    if (out_sprite_curr != NULL)
    {
        *out_sprite_curr = sprite_curr;
    }
    if (out_sprite_next != NULL)
    {
        *out_sprite_next = sprite_next;
    }
    return mask;
}

static void ndsOpeningRoomProbeDObjMaterial(
    DObj *dobj,
    NDSOpeningRoomMaterialProbe *probe)
{
    MObj *mobj;
    MObj *first_mobj = NULL;

    ndsOpeningRoomInitMaterialProbe(probe);
    if ((dobj == NULL) || (probe == NULL))
    {
        return;
    }

    probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_DOBJ_SEEN;

    for (mobj = dobj->mobj; (mobj != NULL) && (probe->count < 64u);
         mobj = mobj->next)
    {
        if (first_mobj == NULL)
        {
            first_mobj = mobj;
        }
        probe->count++;
    }

    if (first_mobj == NULL)
    {
        return;
    }

    probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_MOBJ;
    probe->flags = first_mobj->sub.flags;
    probe->effective_flags = ndsOpeningRoomGetEffectiveMObjFlags(first_mobj);
    if (probe->flags == MOBJ_FLAG_NONE)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_DEFAULT_FLAGS;
    }
    if ((probe->effective_flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_TEXTURE;
    }
    if ((probe->effective_flags & MOBJ_FLAG_ALPHA) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_ALPHA;
    }
    if ((probe->effective_flags & 0x20u) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_TILESIZE;
    }
    if ((probe->effective_flags & 0x40u) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_SCROLL_TILESIZE;
    }
    if ((probe->effective_flags & MOBJ_FLAG_PALETTE) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_PALETTE;
    }
    if ((probe->effective_flags & MOBJ_FLAG_FRAC) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_FRAC;
    }
    if ((probe->effective_flags & MOBJ_FLAG_SPLIT) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_SPLIT;
    }
    if ((probe->effective_flags &
         (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_ENVCOLOR |
          MOBJ_FLAG_BLENDCOLOR)) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_COLOR;
    }
    if ((probe->effective_flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_LIGHT;
    }

    probe->texture_curr = first_mobj->texture_id_curr;
    probe->texture_next = first_mobj->texture_id_next;
    if (first_mobj->palette_id >= 0.0F)
    {
        probe->palette_index = (u32)ndsPreviewFloatToS32(first_mobj->palette_id);
    }
    probe->lfrac100 = ndsPreviewFloatToCenti(first_mobj->lfrac);
    probe->format = first_mobj->sub.fmt;
    probe->size = first_mobj->sub.siz;
    probe->block_format = first_mobj->sub.block_fmt;
    probe->block_size = first_mobj->sub.block_siz;
    probe->tile_width = first_mobj->sub.unk0C;
    probe->tile_height = first_mobj->sub.unk0E;
    probe->scroll_width = first_mobj->sub.unk38;
    probe->scroll_height = first_mobj->sub.unk3A;
    probe->scale_s100 = ndsPreviewFloatToCenti(first_mobj->sub.scau);
    probe->scale_t100 = ndsPreviewFloatToCenti(first_mobj->sub.scav);
    probe->translate_s100 = ndsPreviewFloatToCenti(first_mobj->sub.trau);
    probe->translate_t100 = ndsPreviewFloatToCenti(first_mobj->sub.trav);
    probe->sprite_array = (u32)(uintptr_t)first_mobj->sub.sprites;
    probe->palette_array = (u32)(uintptr_t)first_mobj->sub.palettes;

    if (first_mobj->sub.sprites != NULL)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_SPRITE_ARRAY;
        if ((probe->effective_flags &
             (MOBJ_FLAG_ALPHA | MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            probe->sprite_curr =
                (u32)(uintptr_t)
                    first_mobj->sub.sprites[first_mobj->texture_id_curr];
            if (probe->sprite_curr != 0)
            {
                probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_CURR_SPRITE;
            }
        }
        if ((probe->effective_flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
        {
            probe->sprite_next =
                (u32)(uintptr_t)
                    first_mobj->sub.sprites[first_mobj->texture_id_next];
            if (probe->sprite_next != 0)
            {
                probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_NEXT_SPRITE;
            }
        }
    }
    if (first_mobj->sub.palettes != NULL)
    {
        probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_PALETTE_ARRAY;
        if (((probe->effective_flags & MOBJ_FLAG_PALETTE) != 0) &&
            (probe->palette_index != 0xffffffffu))
        {
            probe->palette_ptr =
                (u32)(uintptr_t)
                    first_mobj->sub.palettes[probe->palette_index];
            if (probe->palette_ptr != 0)
            {
                probe->mask |= NDS_OPENING_ROOM_DL_MATERIAL_HAS_PALETTE_PTR;
            }
        }
    }
}

static void ndsOpeningRoomStoreDLPreviewMaterial(
    const NDSOpeningRoomMaterialProbe *probe)
{
    if (probe == NULL)
    {
        return;
    }

    gNdsOpeningRoomDLPreviewMaterialCount = probe->count;
    gNdsOpeningRoomDLPreviewMaterialFlags = probe->flags;
    gNdsOpeningRoomDLPreviewMaterialEffectiveFlags = probe->effective_flags;
    gNdsOpeningRoomDLPreviewMaterialMask = probe->mask;
    gNdsOpeningRoomDLPreviewMaterialTextureCurr = probe->texture_curr;
    gNdsOpeningRoomDLPreviewMaterialTextureNext = probe->texture_next;
    gNdsOpeningRoomDLPreviewMaterialPaletteIndex = probe->palette_index;
    gNdsOpeningRoomDLPreviewMaterialLfrac100 = probe->lfrac100;
    gNdsOpeningRoomDLPreviewMaterialFormat = probe->format;
    gNdsOpeningRoomDLPreviewMaterialSize = probe->size;
    gNdsOpeningRoomDLPreviewMaterialBlockFormat = probe->block_format;
    gNdsOpeningRoomDLPreviewMaterialBlockSize = probe->block_size;
    gNdsOpeningRoomDLPreviewMaterialTileWidth = probe->tile_width;
    gNdsOpeningRoomDLPreviewMaterialTileHeight = probe->tile_height;
    gNdsOpeningRoomDLPreviewMaterialScrollWidth = probe->scroll_width;
    gNdsOpeningRoomDLPreviewMaterialScrollHeight = probe->scroll_height;
    gNdsOpeningRoomDLPreviewMaterialScaleS100 = probe->scale_s100;
    gNdsOpeningRoomDLPreviewMaterialScaleT100 = probe->scale_t100;
    gNdsOpeningRoomDLPreviewMaterialTranslateS100 = probe->translate_s100;
    gNdsOpeningRoomDLPreviewMaterialTranslateT100 = probe->translate_t100;
    gNdsOpeningRoomDLPreviewMaterialSpriteCurr = probe->sprite_curr;
    gNdsOpeningRoomDLPreviewMaterialSpriteNext = probe->sprite_next;
    gNdsOpeningRoomDLPreviewMaterialPalettePtr = probe->palette_ptr;
}

static void ndsOpeningRoomStoreCandidateMaterial(
    const NDSOpeningRoomMaterialProbe *probe)
{
    if (probe == NULL)
    {
        return;
    }

    gNdsOpeningRoomDrawMaterialCandidateMObjCount = probe->count;
    gNdsOpeningRoomDrawMaterialCandidateMObjFlags = probe->flags;
    gNdsOpeningRoomDrawMaterialCandidateMObjEffectiveFlags =
        probe->effective_flags;
    gNdsOpeningRoomDrawMaterialCandidateMObjMask = probe->mask;
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureCurr = probe->texture_curr;
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureNext = probe->texture_next;
    gNdsOpeningRoomDrawMaterialCandidateMObjPaletteIndex =
        probe->palette_index;
    gNdsOpeningRoomDrawMaterialCandidateMObjLfrac100 = probe->lfrac100;
    gNdsOpeningRoomDrawMaterialCandidateMObjFormat = probe->format;
    gNdsOpeningRoomDrawMaterialCandidateMObjSize = probe->size;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockFormat =
        probe->block_format;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockSize = probe->block_size;
    gNdsOpeningRoomDrawMaterialCandidateMObjTileWidth = probe->tile_width;
    gNdsOpeningRoomDrawMaterialCandidateMObjTileHeight = probe->tile_height;
    gNdsOpeningRoomDrawMaterialCandidateMObjScrollWidth =
        probe->scroll_width;
    gNdsOpeningRoomDrawMaterialCandidateMObjScrollHeight =
        probe->scroll_height;
    gNdsOpeningRoomDrawMaterialCandidateMObjScaleS100 = probe->scale_s100;
    gNdsOpeningRoomDrawMaterialCandidateMObjScaleT100 = probe->scale_t100;
    gNdsOpeningRoomDrawMaterialCandidateMObjTranslateS100 =
        probe->translate_s100;
    gNdsOpeningRoomDrawMaterialCandidateMObjTranslateT100 =
        probe->translate_t100;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteArray =
        probe->sprite_array;
    gNdsOpeningRoomDrawMaterialCandidateMObjPaletteArray =
        probe->palette_array;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteCurr = probe->sprite_curr;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteNext = probe->sprite_next;
    gNdsOpeningRoomDrawMaterialCandidateMObjPalettePtr = probe->palette_ptr;
}

static void ndsOpeningRoomRecordDLPreviewMaterial(DObj *dobj)
{
    NDSOpeningRoomMaterialProbe probe;

    ndsOpeningRoomProbeDObjMaterial(dobj, &probe);
    ndsOpeningRoomStoreDLPreviewMaterial(&probe);
}

static f32 ndsPreviewDot3(f32 ax, f32 ay, f32 az, f32 bx, f32 by, f32 bz)
{
    return (ax * bx) + (ay * by) + (az * bz);
}

static ub8 ndsPreviewNormalize3(f32 *x, f32 *y, f32 *z)
{
    f32 len = sqrtf((*x * *x) + (*y * *y) + (*z * *z));

    if (len <= 0.0001F)
    {
        return FALSE;
    }
    *x /= len;
    *y /= len;
    *z /= len;
    return TRUE;
}

static void ndsPreviewRotateAroundAxis(f32 *vx, f32 *vy, f32 *vz,
                                       f32 ax, f32 ay, f32 az, f32 angle)
{
    f32 sin_angle = sinf(angle);
    f32 cos_angle = cosf(angle);
    f32 dot = ndsPreviewDot3(*vx, *vy, *vz, ax, ay, az);
    f32 cross_x = (ay * *vz) - (az * *vy);
    f32 cross_y = (az * *vx) - (ax * *vz);
    f32 cross_z = (ax * *vy) - (ay * *vx);
    f32 one_minus_cos = 1.0F - cos_angle;
    f32 out_x = (*vx * cos_angle) + (cross_x * sin_angle) +
                (ax * dot * one_minus_cos);
    f32 out_y = (*vy * cos_angle) + (cross_y * sin_angle) +
                (ay * dot * one_minus_cos);
    f32 out_z = (*vz * cos_angle) + (cross_z * sin_angle) +
                (az * dot * one_minus_cos);

    *vx = out_x;
    *vy = out_y;
    *vz = out_z;
}

static s32 ndsPreviewClampProjectionCoord(s32 value)
{
    if (value < -1024) return -1024;
    if (value > 1024) return 1024;
    return value;
}

static CObj *ndsOpeningRoomGetCurrentDrawCameraCObj(void)
{
    if (sNdsOpeningRoomCurrentDrawCameraGObj != NULL)
    {
        return CObjGetStruct(sNdsOpeningRoomCurrentDrawCameraGObj);
    }
    return (sNdsOpeningRoomPreviewCameraGObj != NULL) ?
        CObjGetStruct(sNdsOpeningRoomPreviewCameraGObj) : NULL;
}

static ub8 ndsOpeningRoomProjectedTriIntersectsPreview(
    const NDSDLPreviewVtx *a,
    const NDSDLPreviewVtx *b,
    const NDSDLPreviewVtx *c)
{
    s32 min_x = a->px;
    s32 max_x = a->px;
    s32 min_y = a->py;
    s32 max_y = a->py;

    if ((a->projected == FALSE) ||
        (b->projected == FALSE) ||
        (c->projected == FALSE))
    {
        return FALSE;
    }

    if (b->px < min_x) min_x = b->px;
    if (c->px < min_x) min_x = c->px;
    if (b->px > max_x) max_x = b->px;
    if (c->px > max_x) max_x = c->px;
    if (b->py < min_y) min_y = b->py;
    if (c->py < min_y) min_y = c->py;
    if (b->py > max_y) max_y = b->py;
    if (c->py > max_y) max_y = c->py;

    return (max_x >= 0) &&
           (max_y >= 0) &&
           (min_x < (s32)NDS_DL_PREVIEW_WIDTH) &&
           (min_y < (s32)NDS_DL_PREVIEW_HEIGHT);
}

static void ndsOpeningRoomProjectDLPreview(CObj *cobj,
                                           NDSDLPreviewVtx *vertices,
                                           u32 vertex_count,
                                           const NDSDLPreviewTri *tris,
                                           u32 tri_count)
{
    f32 look_x;
    f32 look_y;
    f32 look_z;
    f32 right_x;
    f32 right_y;
    f32 right_z;
    f32 up_x = 0.0F;
    f32 up_y = 1.0F;
    f32 up_z = 0.0F;
    f32 roll = 0.0F;
    f32 cot;
    f32 sin_half;
    f32 cos_half;
    f32 aspect;
    s32 vp_left;
    s32 vp_top;
    s32 vp_width;
    s32 vp_height;
    u32 projected_vertices = 0;
    u32 projected_tris = 0;
    u32 i;
    ub8 bounds_set = FALSE;
    ub8 depth_set = FALSE;

    gNdsOpeningRoomDLPreviewProjectionMask = 0;
    gNdsOpeningRoomDLPreviewProjectionMode =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_MODE_NONE;
    gNdsOpeningRoomDLPreviewProjectionBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NONE;

    if (cobj == NULL)
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NO_CAMERA;
        return;
    }
    gNdsOpeningRoomDLPreviewProjectionMask |=
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_CAMERA;

    vp_left = (cobj->viewport.vp.vtrans[0] -
               cobj->viewport.vp.vscale[0]) / 4;
    vp_top = (cobj->viewport.vp.vtrans[1] -
              cobj->viewport.vp.vscale[1]) / 4;
    vp_width = cobj->viewport.vp.vscale[0] / 2;
    vp_height = cobj->viewport.vp.vscale[1] / 2;
    if ((vp_width <= 0) || (vp_height <= 0))
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_BAD_VIEWPORT;
        return;
    }
    gNdsOpeningRoomDLPreviewProjectionMask |=
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_VIEWPORT;

    aspect = cobj->projection.persp.aspect;
    if (aspect <= 0.0001F)
    {
        aspect = (f32)vp_width / (f32)vp_height;
    }
    sin_half = sinf(cobj->projection.persp.fovy * 0.008726646F);
    cos_half = cosf(cobj->projection.persp.fovy * 0.008726646F);
    if ((cobj->projection.persp.fovy <= 0.0F) ||
        (aspect <= 0.0001F) ||
        (sin_half <= 0.0001F))
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_BAD_PERSP;
        return;
    }
    cot = cos_half / sin_half;
    gNdsOpeningRoomDLPreviewProjectionMask |=
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_PERSP;

    look_x = cobj->vec.at.x - cobj->vec.eye.x;
    look_y = cobj->vec.at.y - cobj->vec.eye.y;
    look_z = cobj->vec.at.z - cobj->vec.eye.z;
    if (ndsPreviewNormalize3(&look_x, &look_y, &look_z) == FALSE)
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_BAD_LOOKAT;
        return;
    }
    look_x = -look_x;
    look_y = -look_y;
    look_z = -look_z;

    if ((cobj->xobjs_num > 1) && (cobj->xobjs[1] != NULL))
    {
        switch (cobj->xobjs[1]->kind)
        {
        case 6:
        case 7:
            up_x = cobj->vec.up.x;
            up_y = cobj->vec.up.y;
            up_z = cobj->vec.up.z;
            break;

        case 10:
        case 11:
            up_x = 0.0F;
            up_y = 0.0F;
            up_z = 1.0F;
            roll = cobj->vec.up.x;
            break;

        case 8:
        case 9:
        default:
            up_x = 0.0F;
            up_y = 1.0F;
            up_z = 0.0F;
            roll = cobj->vec.up.x;
            break;
        }
    }

    right_x = (up_y * look_z) - (up_z * look_y);
    right_y = (up_z * look_x) - (up_x * look_z);
    right_z = (up_x * look_y) - (up_y * look_x);
    if (ndsPreviewNormalize3(&right_x, &right_y, &right_z) == FALSE)
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_BAD_LOOKAT;
        return;
    }
    if (roll != 0.0F)
    {
        ndsPreviewRotateAroundAxis(&right_x, &right_y, &right_z,
                                   look_x, look_y, look_z, roll);
        if (ndsPreviewNormalize3(&right_x, &right_y, &right_z) == FALSE)
        {
            gNdsOpeningRoomDLPreviewProjectionBlocker =
                NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_BAD_LOOKAT;
            return;
        }
    }

    up_x = (look_y * right_z) - (look_z * right_y);
    up_y = (look_z * right_x) - (look_x * right_z);
    up_z = (look_x * right_y) - (look_y * right_x);
    if (ndsPreviewNormalize3(&up_x, &up_y, &up_z) == FALSE)
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_BAD_LOOKAT;
        return;
    }
    gNdsOpeningRoomDLPreviewProjectionMask |=
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_LOOKAT;

    for (i = 0; i < vertex_count; i++)
    {
        f32 rel_x;
        f32 rel_y;
        f32 rel_z;
        f32 view_x;
        f32 view_y;
        f32 view_z;
        f32 depth;
        f32 ndc_x;
        f32 ndc_y;
        f32 screen_x;
        f32 screen_y;
        s32 preview_x;
        s32 preview_y;
        s32 depth100;

        vertices[i].projected = FALSE;

        if (vertices[i].valid == FALSE)
        {
            continue;
        }

        rel_x = (f32)vertices[i].wx - cobj->vec.eye.x;
        rel_y = (f32)vertices[i].wy - cobj->vec.eye.y;
        rel_z = (f32)vertices[i].wz - cobj->vec.eye.z;
        view_x = ndsPreviewDot3(rel_x, rel_y, rel_z,
                                right_x, right_y, right_z);
        view_y = ndsPreviewDot3(rel_x, rel_y, rel_z,
                                up_x, up_y, up_z);
        view_z = ndsPreviewDot3(rel_x, rel_y, rel_z,
                                look_x, look_y, look_z);
        depth = -view_z;
        if (depth <= 1.0F)
        {
            continue;
        }

        ndc_x = (view_x * cot) / (aspect * depth);
        ndc_y = (view_y * cot) / depth;
        screen_x = ((f32)cobj->viewport.vp.vtrans[0] * 0.25F) +
                   (ndc_x * ((f32)cobj->viewport.vp.vscale[0] * 0.25F));
        screen_y = ((f32)cobj->viewport.vp.vtrans[1] * 0.25F) -
                   (ndc_y * ((f32)cobj->viewport.vp.vscale[1] * 0.25F));
        preview_x = ndsPreviewFloatToS32(
            ((screen_x - (f32)vp_left) *
             ((f32)NDS_DL_PREVIEW_WIDTH - 1.0F)) / (f32)vp_width);
        preview_y = ndsPreviewFloatToS32(
            ((screen_y - (f32)vp_top) *
             ((f32)NDS_DL_PREVIEW_HEIGHT - 1.0F)) / (f32)vp_height);
        vertices[i].px = ndsPreviewClampProjectionCoord(preview_x);
        vertices[i].py = ndsPreviewClampProjectionCoord(preview_y);
        depth100 = ndsPreviewFloatToCenti(depth);
        vertices[i].depth100 = depth100;
        vertices[i].projected = TRUE;
        projected_vertices++;

        if (bounds_set == FALSE)
        {
            gNdsOpeningRoomDLPreviewProjectedMinX = vertices[i].px;
            gNdsOpeningRoomDLPreviewProjectedMaxX = vertices[i].px;
            gNdsOpeningRoomDLPreviewProjectedMinY = vertices[i].py;
            gNdsOpeningRoomDLPreviewProjectedMaxY = vertices[i].py;
            bounds_set = TRUE;
        }
        else
        {
            if (vertices[i].px < gNdsOpeningRoomDLPreviewProjectedMinX)
                gNdsOpeningRoomDLPreviewProjectedMinX = vertices[i].px;
            if (vertices[i].px > gNdsOpeningRoomDLPreviewProjectedMaxX)
                gNdsOpeningRoomDLPreviewProjectedMaxX = vertices[i].px;
            if (vertices[i].py < gNdsOpeningRoomDLPreviewProjectedMinY)
                gNdsOpeningRoomDLPreviewProjectedMinY = vertices[i].py;
            if (vertices[i].py > gNdsOpeningRoomDLPreviewProjectedMaxY)
                gNdsOpeningRoomDLPreviewProjectedMaxY = vertices[i].py;
        }

        if (depth_set == FALSE)
        {
            gNdsOpeningRoomDLPreviewProjectedMinDepth100 = depth100;
            gNdsOpeningRoomDLPreviewProjectedMaxDepth100 = depth100;
            depth_set = TRUE;
        }
        else
        {
            if (depth100 < gNdsOpeningRoomDLPreviewProjectedMinDepth100)
                gNdsOpeningRoomDLPreviewProjectedMinDepth100 = depth100;
            if (depth100 > gNdsOpeningRoomDLPreviewProjectedMaxDepth100)
                gNdsOpeningRoomDLPreviewProjectedMaxDepth100 = depth100;
        }
    }

    gNdsOpeningRoomDLPreviewProjectedVertexCount = projected_vertices;
    if (projected_vertices == 0)
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NO_VERTICES;
        return;
    }
    gNdsOpeningRoomDLPreviewProjectionMask |=
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_VERTICES;

    for (i = 0; i < tri_count; i++)
    {
        const NDSDLPreviewVtx *a = &vertices[tris[i].v0];
        const NDSDLPreviewVtx *b = &vertices[tris[i].v1];
        const NDSDLPreviewVtx *c = &vertices[tris[i].v2];

        if (ndsOpeningRoomProjectedTriIntersectsPreview(a, b, c) != FALSE)
        {
            projected_tris++;
        }
    }
    gNdsOpeningRoomDLPreviewProjectedTriangleCount = projected_tris;
    if (projected_tris == 0)
    {
        gNdsOpeningRoomDLPreviewProjectionBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NO_TRIANGLES;
        return;
    }

    gNdsOpeningRoomDLPreviewProjectionMask |=
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_TRIANGLES;
    gNdsOpeningRoomDLPreviewProjectionMode =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_MODE_CAMERA;
}

static s32 ndsOpeningRoomVisitDLPreviewCommand(
    const NDSRendererCommand *command, void *user)
{
    NDSDLPreviewParseState *state = user;
    u32 w0;
    u32 w1;
    u32 op;

    if ((command == NULL) || (state == NULL))
    {
        return FALSE;
    }

    w0 = command->w0;
    w1 = command->w1;
    op = command->op;

    if (gNdsOpeningRoomDLPreviewFirstOpcode == 0)
    {
        gNdsOpeningRoomDLPreviewFirstOpcode = op;
    }
    (*(state->command_count))++;

    switch (op)
    {
    case NDS_GBI_OP_VTX:
    {
        const u8 *src = (const u8 *)(uintptr_t)w1;
        u32 v0;
        u32 count;
        u32 v;

        gNdsOpeningRoomDLPreviewVertexCommandCount++;
        if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_DL_PREVIEW_MAX_VTX, &v0,
                                  &count) == FALSE)
        {
            break;
        }
        for (v = 0; v < count; v++)
        {
            ndsDecodeSwappedVtx(&state->vertices[v0 + v],
                                src + (v * 16u));
        }
        if ((v0 + count) > *(state->vertex_count))
        {
            *(state->vertex_count) = v0 + count;
        }
        break;
    }

    case NDS_GBI_OP_TRI1:
        gNdsOpeningRoomDLPreviewTriangleCommandCount++;
        ndsDLPreviewAddTriangle(state->tris, state->tri_count,
                                ndsGBIDecodeF3DEX2Tri1(w0));
        break;

    case NDS_GBI_OP_TRI2:
        gNdsOpeningRoomDLPreviewTriangleCommandCount++;
        ndsDLPreviewAddTriangle(state->tris, state->tri_count,
                                ndsGBIDecodeF3DEX2Tri2First(w0));
        ndsDLPreviewAddTriangle(state->tris, state->tri_count,
                                ndsGBIDecodeF3DEX2Tri2Second(w1));
        break;

    case NDS_GBI_OP_ENDDL:
        gNdsOpeningRoomDLPreviewEndCommandCount++;
        break;

    case NDS_GBI_OP_DL:
        gNdsOpeningRoomDLPreviewBranchCommandCount++;
        if (command->branch_resolve_kind == NDS_RENDERER_RESOLVE_SEGMENT)
        {
            gNdsOpeningRoomDLPreviewSegmentResolveCount++;
        }
        if (command->branch_is_jump != FALSE)
        {
            gNdsOpeningRoomDLPreviewBranchJumpCount++;
        }
        else
        {
            gNdsOpeningRoomDLPreviewBranchCallCount++;
        }
        break;

    case NDS_GBI_OP_TEXTURE:
        ndsPreviewDecodeTextureState(state->texture_state, w0, w1);
        gNdsOpeningRoomDLPreviewTextureCommandCount++;
        gNdsOpeningRoomDLPreviewTextureMask |=
            NDS_OPENING_ROOM_DL_TEXTURE_TEXTURE;
        break;

    case NDS_GBI_OP_POPMTX:
        break;

    case NDS_GBI_OP_SPECIAL_1:
        break;

    case NDS_GBI_OP_MTX:
        break;

    case NDS_GBI_OP_MOVEWORD:
        break;

    case NDS_GBI_OP_GEOMETRYMODE:
        *(state->geometry_mode) = (*(state->geometry_mode) & w0) | w1;
        gNdsOpeningRoomDLPreviewGeometryCommandCount++;
        gNdsOpeningRoomDLPreviewGeometryClearMask = w0;
        gNdsOpeningRoomDLPreviewGeometrySetMask = w1;
        gNdsOpeningRoomDLPreviewGeometryFinalMode =
            *(state->geometry_mode);
        break;

    case NDS_GBI_OP_SETCOMBINE:
        gNdsOpeningRoomDLPreviewTextureMask |=
            NDS_OPENING_ROOM_DL_TEXTURE_SETCOMBINE;
        if (gNdsOpeningRoomDLPreviewTextureCombineW0 == 0)
        {
            gNdsOpeningRoomDLPreviewTextureCombineW0 = w0;
            gNdsOpeningRoomDLPreviewTextureCombineW1 = w1;
            *(state->combine_mode) = ndsPreviewDecodeCombineMode(
                w0, w1, state->combine_flags);
        }
        break;

    case NDS_GBI_OP_SETTIMG:
        gNdsOpeningRoomDLPreviewTextureMask |=
            NDS_OPENING_ROOM_DL_TEXTURE_SETTIMG;
        if (gNdsOpeningRoomDLPreviewTextureImage == 0)
        {
            gNdsOpeningRoomDLPreviewTextureFormat = (w0 >> 21) & 0x7u;
            gNdsOpeningRoomDLPreviewTextureSize = (w0 >> 19) & 0x3u;
            gNdsOpeningRoomDLPreviewTextureImageWidth =
                (w0 & 0x0FFFu) + 1u;
            gNdsOpeningRoomDLPreviewTextureImage = w1;
        }
        break;

    case NDS_GBI_OP_SETTILE:
    {
        NDSDLPreviewTileState tile_state;
        u32 tile = (w1 >> 24) & 0x7u;

        memset(&tile_state, 0, sizeof(tile_state));
        ndsPreviewDecodeTileState(&tile_state, w0, w1);
        gNdsOpeningRoomDLPreviewTextureMask |=
            NDS_OPENING_ROOM_DL_TEXTURE_SETTILE;
        gNdsOpeningRoomDLPreviewTextureSetTileCount++;
        if (tile == 0)
        {
            *(state->render_tile_state) = tile_state;
        }
        else if (tile == 7)
        {
            *(state->load_tile_state) = tile_state;
        }
        break;
    }

    case NDS_GBI_OP_LOADBLOCK:
        gNdsOpeningRoomDLPreviewTextureMask |=
            NDS_OPENING_ROOM_DL_TEXTURE_LOADBLOCK;
        if (gNdsOpeningRoomDLPreviewTextureLoadTexels == 0)
        {
            gNdsOpeningRoomDLPreviewTextureLoadBlockTile =
                (w1 >> 24) & 0x7u;
            gNdsOpeningRoomDLPreviewTextureLoadBlockUls =
                (w0 >> 12) & 0x0FFFu;
            gNdsOpeningRoomDLPreviewTextureLoadBlockUlt =
                w0 & 0x0FFFu;
            gNdsOpeningRoomDLPreviewTextureLoadBlockLrs =
                (w1 >> 12) & 0x0FFFu;
            gNdsOpeningRoomDLPreviewTextureLoadBlockDxt =
                w1 & 0x0FFFu;
            gNdsOpeningRoomDLPreviewTextureLoadTexels =
                gNdsOpeningRoomDLPreviewTextureLoadBlockLrs + 1u;
        }
        break;

    case NDS_GBI_OP_SETTILESIZE:
        gNdsOpeningRoomDLPreviewTextureMask |=
            NDS_OPENING_ROOM_DL_TEXTURE_SETTILESIZE;
        if ((gNdsOpeningRoomDLPreviewTextureTileWidth == 0) ||
            (gNdsOpeningRoomDLPreviewTextureTileHeight == 0))
        {
            u32 tile = (w1 >> 24) & 0x7u;
            u32 uls = (w0 >> 12) & 0x0FFFu;
            u32 ult = w0 & 0x0FFFu;
            u32 lrs = (w1 >> 12) & 0x0FFFu;
            u32 lrt = w1 & 0x0FFFu;

            gNdsOpeningRoomDLPreviewTextureTileSizeTile = tile;
            gNdsOpeningRoomDLPreviewTextureTileSizeUls = uls;
            gNdsOpeningRoomDLPreviewTextureTileSizeUlt = ult;
            gNdsOpeningRoomDLPreviewTextureTileSizeLrs = lrs;
            gNdsOpeningRoomDLPreviewTextureTileSizeLrt = lrt;
            if (lrs >= uls)
            {
                gNdsOpeningRoomDLPreviewTextureTileWidth =
                    ((lrs - uls) >> 2) + 1u;
            }
            if (lrt >= ult)
            {
                gNdsOpeningRoomDLPreviewTextureTileHeight =
                    ((lrt - ult) >> 2) + 1u;
            }
        }
        break;

    case NDS_GBI_OP_RDPPIPESYNC:
    case NDS_GBI_OP_RDPLOADSYNC:
    case NDS_GBI_OP_RDPTILESYNC:
    case NDS_GBI_OP_RDPFULLSYNC:
        gNdsOpeningRoomDLPreviewSyncCommandCount++;
        break;

    case NDS_GBI_OP_SETSCISSOR:
        break;

    case NDS_GBI_OP_SETFOGCOLOR:
    case NDS_GBI_OP_SETBLENDCOLOR:
    case NDS_GBI_OP_SETENVCOLOR:
        gNdsOpeningRoomDLPreviewColorCommandCount++;
        break;

    case NDS_GBI_OP_SETPRIMCOLOR:
        gNdsOpeningRoomDLPreviewColorCommandCount++;
        gNdsOpeningRoomDLPreviewPrimColor = w1;
        break;

    case NDS_GBI_OP_CULLDL:
        gNdsOpeningRoomDLPreviewBranchCommandCount++;
        ndsDLPreviewRecordUnsupported(state->unsupported, op);
        return FALSE;

    case NDS_GBI_OP_SETOTHERMODE_H:
    case NDS_GBI_OP_SETOTHERMODE_L:
    case NDS_GBI_OP_RDPSETOTHERMODE:
        gNdsOpeningRoomDLPreviewOtherModeCommandCount++;
        ndsDLPreviewRecordUnsupported(state->unsupported, op);
        return FALSE;

    default:
        ndsDLPreviewRecordUnsupported(state->unsupported, op);
        return FALSE;
    }

    return TRUE;
}

static void ndsOpeningRoomRenderDLPreview(DObj *dobj, Gfx *dl)
{
    NDSDLPreviewVtx vertices[NDS_DL_PREVIEW_MAX_VTX];
    NDSDLPreviewTri tris[NDS_DL_PREVIEW_MAX_TRIS];
    u32 tri_count = 0;
    u32 command_count = 0;
    u32 vertex_count = 0;
    u32 unsupported = 0;
    u32 i;
    s32 min_x = 0;
    s32 max_x = 0;
    s32 min_y = 0;
    s32 max_y = 0;
    u16 *pixels;
    u32 pitch = 0;
    u32 pixel_count = 0;
    u32 texture_sample_count = 0;
    u32 texture_modulated_count = 0;
    NDSDLPreviewTextureState texture_state;
    NDSDLPreviewTileState render_tile_state;
    NDSDLPreviewTileState load_tile_state;
    u32 texture_texel_width = 0;
    u32 texture_texel_height = 0;
    const u16 *texture_texels = NULL;
    ub8 texture_ready = FALSE;
    u32 texture_sample_mode = NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_NONE;
    CObj *preview_cobj = NULL;
    ub8 use_camera_projection = FALSE;
    u32 fallback_axis = NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XY;
    u32 fallback_area = 0;
    u32 geometry_mode = 0;
    u32 geometry_flags = 0;
    u32 geometry_positive_winding = 0;
    u32 geometry_negative_winding = 0;
    u32 geometry_zero_area = 0;
    u32 geometry_drawn_triangles = 0;
    u32 combine_mode = NDS_OPENING_ROOM_DL_COMBINE_MODE_UNKNOWN;
    u32 combine_flags = 0;
    NDSDLPreviewParseState parse_state;
    NDSRendererConfig renderer_config;
    NDSRendererStats renderer_stats;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;

    if (gNdsOpeningRoomDLPreviewResult != 0)
    {
        return;
    }

    gNdsOpeningRoomDLPreviewFirstDL = (u32)(uintptr_t)dl;
    ndsOpeningRoomRecordDLPreviewMaterial(dobj);

    if (dl == NULL)
    {
        gNdsOpeningRoomDLPreviewBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_DL_LINK;
        return;
    }

    memset(vertices, 0, sizeof(vertices));
    memset(tris, 0, sizeof(tris));
    memset(&texture_state, 0, sizeof(texture_state));
    memset(&render_tile_state, 0, sizeof(render_tile_state));
    memset(&load_tile_state, 0, sizeof(load_tile_state));

    parse_state.vertices = vertices;
    parse_state.tris = tris;
    parse_state.texture_state = &texture_state;
    parse_state.render_tile_state = &render_tile_state;
    parse_state.load_tile_state = &load_tile_state;
    parse_state.vertex_count = &vertex_count;
    parse_state.tri_count = &tri_count;
    parse_state.command_count = &command_count;
    parse_state.unsupported = &unsupported;
    parse_state.geometry_mode = &geometry_mode;
    parse_state.combine_mode = &combine_mode;
    parse_state.combine_flags = &combine_flags;

    preview_cobj = ndsOpeningRoomGetCurrentDrawCameraCObj();
    ndsRendererAdapterPrepareInitialMatrices(dobj, preview_cobj, FALSE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);
    renderer_config.max_depth = NDS_MATERIAL_DL_EXPAND_MAX_DEPTH;
    renderer_config.max_commands = NDS_MATERIAL_DL_EXPAND_MAX_COMMANDS;
    renderer_config.max_list_commands =
        NDS_MATERIAL_DL_EXPAND_MAX_LIST_COMMANDS;
    renderer_config.initial_projection = initial_projection_ptr;
    renderer_config.initial_modelview = initial_modelview_ptr;
    renderer_config.initial_geometry_mode = 0u;
    renderer_config.texture_data_layout =
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    renderer_config.validate_range = ndsOpeningRoomRendererValidateRange;
    renderer_config.immutable_command_span = NULL;
    renderer_config.resolve_branch = ndsOpeningRoomRendererResolveBranch;
    renderer_config.resolve_data = ndsOpeningRoomRendererResolveData;
    renderer_config.user = NULL;
    ndsRendererInitStats(&renderer_stats);
    ndsRendererExecuteDisplayList(dl, &renderer_config,
                                  ndsOpeningRoomVisitDLPreviewCommand,
                                  &parse_state,
                                  &renderer_stats);
    gNdsOpeningRoomDLPreviewRendererParsedCommandCount =
        renderer_stats.command_count;
    gNdsOpeningRoomDLPreviewRendererStateCommandCount =
        renderer_stats.state_command_count;
    gNdsOpeningRoomDLPreviewRendererSkippedCommandCount =
        renderer_stats.skip_command_count;
    gNdsOpeningRoomDLPreviewRendererRenderedCommandCount =
        renderer_stats.render_command_count;
    gNdsOpeningRoomDLPreviewRendererTextureMask =
        renderer_stats.texture_mask;
    gNdsOpeningRoomDLPreviewRendererTextureImage =
        renderer_stats.texture_image;
    gNdsOpeningRoomDLPreviewRendererTextureFormat =
        renderer_stats.texture_format;
    gNdsOpeningRoomDLPreviewRendererTextureSize =
        renderer_stats.texture_size;
    gNdsOpeningRoomDLPreviewRendererTextureImageWidth =
        renderer_stats.texture_image_width;
    gNdsOpeningRoomDLPreviewRendererTextureLoadTexels =
        renderer_stats.texture_load_texels;
    gNdsOpeningRoomDLPreviewRendererTextureSetTileCount =
        renderer_stats.texture_set_tile_count;
    gNdsOpeningRoomDLPreviewRendererTextureCommandCount =
        renderer_stats.texture_command_count;
    gNdsOpeningRoomDLPreviewRendererTextureStateFlags =
        renderer_stats.texture_state_flags;
    gNdsOpeningRoomDLPreviewRendererTextureTileWidth =
        renderer_stats.texture_tile_width;
    gNdsOpeningRoomDLPreviewRendererTextureTileHeight =
        renderer_stats.texture_tile_height;
    gNdsOpeningRoomDLPreviewRendererTextureRenderTile =
        renderer_stats.texture_render_tile;
    gNdsOpeningRoomDLPreviewRendererTextureRenderTileLine =
        renderer_stats.texture_render_tile_line;
    gNdsOpeningRoomDLPreviewRendererTextureRenderTileFlags =
        renderer_stats.texture_render_tile_flags;
    gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs =
        renderer_stats.texture_load_block_lrs;
    gNdsOpeningRoomDLPreviewRendererTextureLoadBlockDxt =
        renderer_stats.texture_load_block_dxt;
    if ((renderer_stats.blocker != NDS_RENDERER_BLOCKER_NONE) &&
        (unsupported == 0))
    {
        if (renderer_stats.unsupported_opcode != 0)
        {
            unsupported = renderer_stats.unsupported_opcode;
        }
        else
        {
            unsupported = NDS_GBI_OP_DL;
        }
    }

    gNdsOpeningRoomDLPreviewCommandCount = command_count;
    gNdsOpeningRoomDLPreviewVertexCount = vertex_count;
    gNdsOpeningRoomDLPreviewTriangleCount = tri_count;
    gNdsOpeningRoomDLPreviewUnsupportedOpcode = unsupported;
    gNdsOpeningRoomDLPreviewTextureCombineMode = combine_mode;
    gNdsOpeningRoomDLPreviewTextureCombineFlags = combine_flags;
    geometry_flags = ndsPreviewGeometryFlagsFromMode(geometry_mode);
    if (gNdsOpeningRoomDLPreviewGeometryCommandCount != 0)
    {
        geometry_flags |= NDS_OPENING_ROOM_DL_GEOMETRY_SEEN;
    }
    gNdsOpeningRoomDLPreviewGeometryFlags = geometry_flags;
    if (texture_state.valid != FALSE)
    {
        gNdsOpeningRoomDLPreviewTextureScaleS = texture_state.scale_s;
        gNdsOpeningRoomDLPreviewTextureScaleT = texture_state.scale_t;
        gNdsOpeningRoomDLPreviewTextureLevel = texture_state.level;
        gNdsOpeningRoomDLPreviewTextureTile = texture_state.tile;
        gNdsOpeningRoomDLPreviewTextureOn = texture_state.on;
        gNdsOpeningRoomDLPreviewTextureXParam = texture_state.xparam;
    }
    gNdsOpeningRoomDLPreviewTextureStateFlags =
        ndsPreviewTextureFlagsFromState(&texture_state);
    if (render_tile_state.valid != FALSE)
    {
        gNdsOpeningRoomDLPreviewTextureRenderTile =
            render_tile_state.tile;
        gNdsOpeningRoomDLPreviewTextureRenderTileLine =
            render_tile_state.line;
        gNdsOpeningRoomDLPreviewTextureRenderTileTmem =
            render_tile_state.tmem;
        gNdsOpeningRoomDLPreviewTextureRenderTilePalette =
            render_tile_state.palette;
        gNdsOpeningRoomDLPreviewTextureRenderTileCms =
            render_tile_state.cms;
        gNdsOpeningRoomDLPreviewTextureRenderTileCmt =
            render_tile_state.cmt;
        gNdsOpeningRoomDLPreviewTextureRenderTileMasks =
            render_tile_state.masks;
        gNdsOpeningRoomDLPreviewTextureRenderTileMaskt =
            render_tile_state.maskt;
        gNdsOpeningRoomDLPreviewTextureRenderTileShifts =
            render_tile_state.shifts;
        gNdsOpeningRoomDLPreviewTextureRenderTileShiftt =
            render_tile_state.shiftt;
    }
    gNdsOpeningRoomDLPreviewTextureRenderTileFlags =
        ndsPreviewTileFlagsFromState(&render_tile_state, &load_tile_state);

    if (vertex_count == 0)
    {
        gNdsOpeningRoomDLPreviewBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_VERTEX_LOAD;
        return;
    }
    if (tri_count == 0)
    {
        gNdsOpeningRoomDLPreviewBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_TRIANGLES;
        return;
    }

    if (((gNdsOpeningRoomDLPreviewTextureMask &
          NDS_OPENING_ROOM_DL_TEXTURE_READY_MASK) ==
        NDS_OPENING_ROOM_DL_TEXTURE_READY_MASK) &&
        (gNdsOpeningRoomDLPreviewTextureSize == G_IM_SIZ_16b) &&
        (texture_state.valid != FALSE) &&
        (texture_state.on != 0) &&
        (render_tile_state.line != 0) &&
        (gNdsOpeningRoomDLPreviewTextureLoadTexels != 0) &&
        (gNdsOpeningRoomDLPreviewTextureImage != 0))
    {
        if (gNdsOpeningRoomDLPreviewTextureFormat == G_IM_FMT_RGBA)
        {
            texture_sample_mode =
                NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_RGBA16;
        }
        else if (gNdsOpeningRoomDLPreviewTextureFormat == G_IM_FMT_I)
        {
            texture_sample_mode =
                NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_I16;
        }

        if (texture_sample_mode != NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_NONE)
        {
            texture_texel_width = render_tile_state.line * 4u;
            texture_texel_height =
                gNdsOpeningRoomDLPreviewTextureLoadTexels /
                texture_texel_width;
            if ((texture_texel_width != 0) && (texture_texel_height != 0))
            {
                texture_ready = TRUE;
                texture_texels = (const u16 *)(uintptr_t)
                    gNdsOpeningRoomDLPreviewTextureImage;
            }
        }
    }
    gNdsOpeningRoomDLPreviewTextureReady =
        (texture_ready != FALSE) ? 1u : 0u;
    gNdsOpeningRoomDLPreviewTextureSampleMode = texture_sample_mode;
    gNdsOpeningRoomDLPreviewTextureTexelWidth = texture_texel_width;
    gNdsOpeningRoomDLPreviewTextureTexelHeight = texture_texel_height;

    ndsOpeningRoomApplyDLPreviewTransform(dobj, vertices, vertex_count);
    fallback_axis = ndsPreviewChooseFallbackAxis(vertices, vertex_count,
                                                tris, tri_count,
                                                &fallback_area);
    gNdsOpeningRoomDLPreviewFallbackAxis = fallback_axis;
    gNdsOpeningRoomDLPreviewFallbackArea = fallback_area;

    for (i = 0; i < vertex_count; i++)
    {
        if (vertices[i].valid != FALSE)
        {
            min_x = max_x =
                ndsPreviewFallbackCoordA(&vertices[i], fallback_axis);
            min_y = max_y =
                ndsPreviewFallbackCoordB(&vertices[i], fallback_axis);
            break;
        }
    }
    for (; i < vertex_count; i++)
    {
        if (vertices[i].valid == FALSE)
        {
            continue;
        }
        {
            s32 coord_a =
                ndsPreviewFallbackCoordA(&vertices[i], fallback_axis);
            s32 coord_b =
                ndsPreviewFallbackCoordB(&vertices[i], fallback_axis);

            if (coord_a < min_x) min_x = coord_a;
            if (coord_a > max_x) max_x = coord_a;
            if (coord_b < min_y) min_y = coord_b;
            if (coord_b > max_y) max_y = coord_b;
        }
    }
    if (min_x == max_x) max_x = min_x + 1;
    if (min_y == max_y) max_y = min_y + 1;
    gNdsOpeningRoomDLPreviewMinX = min_x;
    gNdsOpeningRoomDLPreviewMaxX = max_x;
    gNdsOpeningRoomDLPreviewMinY = min_y;
    gNdsOpeningRoomDLPreviewMaxY = max_y;

    ndsOpeningRoomProjectDLPreview(preview_cobj, vertices, vertex_count,
                                   tris, tri_count);
    if (gNdsOpeningRoomDLPreviewProjectionMode ==
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_MODE_CAMERA)
    {
        use_camera_projection = TRUE;
    }
    else if (gNdsOpeningRoomDLPreviewProjectionBlocker !=
             NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NONE)
    {
        gNdsOpeningRoomDLPreviewProjectionMode =
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_MODE_FALLBACK;
    }

    pixels = ndsPlatformBeginOriginalDLPreview(NDS_DL_PREVIEW_WIDTH,
                                               NDS_DL_PREVIEW_HEIGHT,
                                               &pitch);
    if (pixels == NULL)
    {
        gNdsOpeningRoomDLPreviewBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_PIXELS;
        return;
    }

    for (i = 0; i < tri_count; i++)
    {
        const NDSDLPreviewVtx *a = &vertices[tris[i].v0];
        const NDSDLPreviewVtx *b = &vertices[tris[i].v1];
        const NDSDLPreviewVtx *c = &vertices[tris[i].v2];
        s32 x0;
        s32 y0;
        s32 x1;
        s32 y1;
        s32 x2;
        s32 y2;
        u16 fill;

        if ((a->valid == FALSE) || (b->valid == FALSE) ||
            (c->valid == FALSE))
        {
            continue;
        }

        if (use_camera_projection != FALSE)
        {
            if (ndsOpeningRoomProjectedTriIntersectsPreview(a, b, c) == FALSE)
            {
                continue;
            }
            x0 = a->px;
            y0 = a->py;
            x1 = b->px;
            y1 = b->py;
            x2 = c->px;
            y2 = c->py;
        }
        else
        {
            s32 a_x = ndsPreviewFallbackCoordA(a, fallback_axis);
            s32 a_y = ndsPreviewFallbackCoordB(a, fallback_axis);
            s32 b_x = ndsPreviewFallbackCoordA(b, fallback_axis);
            s32 b_y = ndsPreviewFallbackCoordB(b, fallback_axis);
            s32 c_x = ndsPreviewFallbackCoordA(c, fallback_axis);
            s32 c_y = ndsPreviewFallbackCoordB(c, fallback_axis);

            x0 = 4 + ((a_x - min_x) *
                      ((s32)NDS_DL_PREVIEW_WIDTH - 9)) / (max_x - min_x);
            y0 = 4 + ((max_y - a_y) *
                      ((s32)NDS_DL_PREVIEW_HEIGHT - 9)) / (max_y - min_y);
            x1 = 4 + ((b_x - min_x) *
                      ((s32)NDS_DL_PREVIEW_WIDTH - 9)) / (max_x - min_x);
            y1 = 4 + ((max_y - b_y) *
                      ((s32)NDS_DL_PREVIEW_HEIGHT - 9)) / (max_y - min_y);
            x2 = 4 + ((c_x - min_x) *
                      ((s32)NDS_DL_PREVIEW_WIDTH - 9)) / (max_x - min_x);
            y2 = 4 + ((max_y - c_y) *
                      ((s32)NDS_DL_PREVIEW_HEIGHT - 9)) / (max_y - min_y);
        }

        {
            s32 winding = ndsPreviewEdge(x0, y0, x1, y1, x2, y2);

            if (winding > 0)
            {
                geometry_positive_winding++;
            }
            else if (winding < 0)
            {
                geometry_negative_winding++;
            }
            else
            {
                geometry_zero_area++;
            }
        }

        if (gNdsOpeningRoomDLPreviewPrimColor != 0)
        {
            fill = ndsPreviewRGB15(
                (u8)(gNdsOpeningRoomDLPreviewPrimColor >> 24),
                (u8)(gNdsOpeningRoomDLPreviewPrimColor >> 16),
                (u8)(gNdsOpeningRoomDLPreviewPrimColor >> 8));
        }
        else
        {
            fill = ndsPreviewRGB15((u8)((a->r + b->r + c->r) / 3u),
                                   (u8)((a->g + b->g + c->g) / 3u),
                                   (u8)((a->b + b->b + c->b) / 3u));
        }
        if ((fill & 0x7FFFu) == 0)
        {
            fill = ndsPreviewRGB15(0, 180, 200);
        }
        if (texture_ready != FALSE)
        {
            ndsPreviewDrawTexturedTriangle(pixels, pitch, a, b, c,
                                           x0, y0, x1, y1, x2, y2,
                                           texture_texels,
                                           texture_texel_width,
                                           texture_texel_height,
                                           &texture_state,
                                           &render_tile_state,
                                           texture_sample_mode,
                                           fill,
                                           ndsPreviewRGB15(255, 220, 0),
                                           combine_mode,
                                           &pixel_count,
                                           &texture_sample_count,
                                           &texture_modulated_count);
        }
        else
        {
            ndsPreviewDrawTriangle(pixels, pitch, x0, y0, x1, y1, x2, y2,
                                   fill, ndsPreviewRGB15(255, 220, 0),
                                   &pixel_count);
        }
        geometry_drawn_triangles++;
    }

    gNdsOpeningRoomDLPreviewPixelCount = pixel_count;
    gNdsOpeningRoomDLPreviewTextureSamplePixels = texture_sample_count;
    gNdsOpeningRoomDLPreviewTextureModulatedPixels =
        texture_modulated_count;
    gNdsOpeningRoomDLPreviewGeometryPositiveWinding =
        geometry_positive_winding;
    gNdsOpeningRoomDLPreviewGeometryNegativeWinding =
        geometry_negative_winding;
    gNdsOpeningRoomDLPreviewGeometryZeroArea = geometry_zero_area;
    gNdsOpeningRoomDLPreviewGeometryDrawnTriangles =
        geometry_drawn_triangles;
    if ((pixel_count != 0) && (use_camera_projection != FALSE))
    {
        gNdsOpeningRoomDLPreviewProjectionMask |=
            NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_DRAWN;
    }
    if (pixel_count == 0)
    {
        ndsPlatformClearOriginalDLPreview();
        gNdsOpeningRoomDLPreviewBlocker =
            NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_PIXELS;
        return;
    }

    ndsPlatformCommitOriginalDLPreview();
    gNdsOpeningRoomDLPreviewBlocker =
        (unsupported != 0) ?
            NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_UNSUPPORTED_CMD :
            NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NONE;
    gNdsOpeningRoomDLPreviewResult = NDS_OPENING_ROOM_DL_PREVIEW_PASS;
}

static Gfx *ndsOpeningRoomGetFirstDObjDLLink(DObj *dobj)
{
    DObjDLLink *dl_link;
    u32 i;

    if (dobj == NULL)
    {
        return NULL;
    }

    dl_link = dobj->dl_link;
    if (dl_link == NULL)
    {
        return NULL;
    }

    for (i = 0; i < 4u; i++, dl_link++)
    {
        if (dl_link->list_id == 4)
        {
            break;
        }
        if (dl_link->dl != NULL)
        {
            return dl_link->dl;
        }
    }
    return NULL;
}

static Gfx *ndsOpeningRoomGetDObjDLForCallback(DObj *dobj,
                                               u32 callback_marker)
{
    if (dobj == NULL)
    {
        return NULL;
    }

    switch (callback_marker)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        return dobj->dl;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    default:
        return ndsOpeningRoomGetFirstDObjDLLink(dobj);
    }
}

static ub8 ndsOpeningRoomDObjDrawableForCallback(DObj *dobj,
                                                 u32 callback_marker)
{
    if (dobj == NULL)
    {
        return FALSE;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
    {
        return FALSE;
    }

    switch (callback_marker)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
        return ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0) ? TRUE : FALSE;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
    default:
        return (dobj->flags == DOBJ_FLAG_NONE) ? TRUE : FALSE;
    }
}

static u32 ndsOpeningRoomMakeDObjMeta(DObj *dobj, Gfx *dl)
{
    u32 meta = 0;

    if (dobj == NULL)
    {
        return 0;
    }
    if (dl != NULL)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_DL;
    }
    if (dobj->child != NULL)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_CHILD;
    }
    if ((dobj->sib_next != NULL) || (dobj->sib_prev != NULL))
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_SIBLING;
    }
    if (dobj->mobj != NULL)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_MOBJ;
    }
    if (dobj->xobjs_num != 0)
    {
        meta |= NDS_OPENING_ROOM_DRAW_DOBJ_HAS_XOBJ;
    }
    return meta;
}

static ub8 ndsOpeningRoomCallbackWalksDObjTree(u32 callback_marker)
{
    return ((callback_marker == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
            (callback_marker ==
             NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS)) ?
        TRUE : FALSE;
}

static DObj *ndsOpeningRoomFindDObjCandidateRecurse(DObj *dobj,
                                                    u32 callback_marker,
                                                    ub8 require_mobj,
                                                    Gfx **out_dl,
                                                    u32 *visited)
{
    while ((dobj != NULL) && (*visited < 64u))
    {
        Gfx *dl;

        (*visited)++;
        dl = ndsOpeningRoomGetDObjDLForCallback(dobj, callback_marker);
        if ((dl != NULL) &&
            (ndsOpeningRoomDObjDrawableForCallback(dobj, callback_marker) !=
             FALSE) &&
            ((require_mobj == FALSE) || (dobj->mobj != NULL)))
        {
            if (out_dl != NULL)
            {
                *out_dl = dl;
            }
            return dobj;
        }

        if ((ndsOpeningRoomCallbackWalksDObjTree(callback_marker) != FALSE) &&
            (dobj->child != NULL))
        {
            DObj *found = ndsOpeningRoomFindDObjCandidateRecurse(
                dobj->child,
                callback_marker,
                require_mobj,
                out_dl,
                visited);

            if (found != NULL)
            {
                return found;
            }
        }

        if (ndsOpeningRoomCallbackWalksDObjTree(callback_marker) == FALSE)
        {
            break;
        }
        dobj = dobj->sib_next;
    }
    return NULL;
}

static DObj *ndsOpeningRoomFindDObjDrawCandidate(DObj *dobj,
                                                 u32 callback_marker,
                                                 ub8 require_mobj,
                                                 Gfx **out_dl)
{
    u32 visited = 0;

    if (out_dl != NULL)
    {
        *out_dl = NULL;
    }
    return ndsOpeningRoomFindDObjCandidateRecurse(
        dobj,
        callback_marker,
        require_mobj,
        out_dl,
        &visited);
}

static void ndsOpeningRoomRecordCapturedDisplay(GObj *camera_gobj,
                                                GObj *display_gobj,
                                                s32 link_id)
{
    (void)camera_gobj;

    if (gSCManagerSceneData.scene_curr != nSCKindOpeningRoom)
    {
        return;
    }
    gNdsOpeningRoomDrawDisplayCallbackCount++;

    if ((display_gobj != NULL) &&
        (gNdsOpeningRoomDrawFirstObjectDLLink == 0xffffffffu))
    {
        gNdsOpeningRoomDrawFirstObjectDLLink = (u32)link_id;
        gNdsOpeningRoomDrawFirstObjectID = display_gobj->id;
        gNdsOpeningRoomDrawFirstObjectKind = display_gobj->obj_kind;
    }
}

static void ndsOpeningRoomStoreTextureMaterialCandidate(GObj *gobj,
                                                        DObj *dobj,
                                                        Gfx *dl,
                                                        u32 callback_marker,
                                                        const MObj *mobj,
                                                        u32 mobj_count,
                                                        u32 effective_flags)
{
    u32 sprite_curr = 0;
    u32 sprite_next = 0;
    u32 mask;

    if ((dobj == NULL) || (mobj == NULL) ||
        (gNdsOpeningRoomDrawTextureMaterialResult ==
         NDS_OPENING_ROOM_DRAW_TEXTURE_MATERIAL_PASS))
    {
        return;
    }

    mask = ndsOpeningRoomMakeTextureMObjMask(
        mobj, effective_flags, &sprite_curr, &sprite_next);

    gNdsOpeningRoomDrawTextureMaterialResult =
        NDS_OPENING_ROOM_DRAW_TEXTURE_MATERIAL_PASS;
    gNdsOpeningRoomDrawTextureMaterialObjectDLLink =
        (gobj != NULL) ? gobj->dl_link_id : 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectID =
        (gobj != NULL) ? gobj->id : 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectKind =
        (gobj != NULL) ? gobj->obj_kind : 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialCallback = callback_marker;
    gNdsOpeningRoomDrawTextureMaterialDObjDL = (u32)(uintptr_t)dl;
    gNdsOpeningRoomDrawTextureMaterialDObjMeta =
        ndsOpeningRoomMakeDObjMeta(dobj, dl);
    gNdsOpeningRoomDrawTextureMaterialMObjFlags = mobj->sub.flags;
    gNdsOpeningRoomDrawTextureMaterialMObjEffectiveFlags = effective_flags;
    gNdsOpeningRoomDrawTextureMaterialMObjMask = mask;
    gNdsOpeningRoomDrawTextureMaterialMObjTextureCurr =
        mobj->texture_id_curr;
    gNdsOpeningRoomDrawTextureMaterialMObjTextureNext =
        mobj->texture_id_next;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteArray =
        (u32)(uintptr_t)mobj->sub.sprites;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteCurr = sprite_curr;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteNext = sprite_next;
    (void)mobj_count;
}

static void ndsOpeningRoomRecordTextureMaterialDObj(GObj *gobj,
                                                    DObj *dobj,
                                                    Gfx *dl,
                                                    u32 callback_marker)
{
    MObj *mobj;
    u32 mobj_count = 0;
    ub8 counted_dobj = FALSE;

    if (dobj == NULL)
    {
        return;
    }

    for (mobj = dobj->mobj; (mobj != NULL) && (mobj_count < 64u);
         mobj = mobj->next)
    {
        mobj_count++;
    }

    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        u32 effective_flags = ndsOpeningRoomGetEffectiveMObjFlags(mobj);
        u32 sprite_curr = 0;
        u32 sprite_next = 0;
        u32 mask;

        if ((effective_flags & MOBJ_FLAG_TEXTURE) == 0)
        {
            continue;
        }

        if (counted_dobj == FALSE)
        {
            gNdsOpeningRoomDrawTextureMaterialCandidateCount++;
            counted_dobj = TRUE;
        }
        gNdsOpeningRoomDrawTextureMaterialMObjCount++;

        mask = ndsOpeningRoomMakeTextureMObjMask(
            mobj, effective_flags, &sprite_curr, &sprite_next);
        if ((mask & NDS_OPENING_ROOM_DL_MATERIAL_HAS_SPRITE_ARRAY) != 0)
        {
            gNdsOpeningRoomDrawTextureMaterialSpriteArrayCount++;
        }
        if ((mask & NDS_OPENING_ROOM_DL_MATERIAL_HAS_CURR_SPRITE) != 0)
        {
            gNdsOpeningRoomDrawTextureMaterialSpriteCurrCount++;
        }
        if ((mask & NDS_OPENING_ROOM_DL_MATERIAL_HAS_NEXT_SPRITE) != 0)
        {
            gNdsOpeningRoomDrawTextureMaterialSpriteNextCount++;
        }

        ndsOpeningRoomStoreTextureMaterialCandidate(
            gobj, dobj, dl, callback_marker, mobj, mobj_count,
            effective_flags);
    }
}

static void ndsOpeningRoomRecordTextureMaterialRecurse(GObj *gobj,
                                                       DObj *dobj,
                                                       u32 callback_marker,
                                                       u32 *visited)
{
    while ((dobj != NULL) && (*visited < 64u))
    {
        Gfx *dl;

        (*visited)++;
        dl = ndsOpeningRoomGetDObjDLForCallback(dobj, callback_marker);
        if ((dl != NULL) &&
            (ndsOpeningRoomDObjDrawableForCallback(dobj, callback_marker) !=
             FALSE) &&
            (dobj->mobj != NULL))
        {
            ndsOpeningRoomRecordTextureMaterialDObj(
                gobj, dobj, dl, callback_marker);
        }

        if ((ndsOpeningRoomCallbackWalksDObjTree(callback_marker) != FALSE) &&
            (dobj->child != NULL))
        {
            ndsOpeningRoomRecordTextureMaterialRecurse(
                gobj, dobj->child, callback_marker, visited);
        }

        if (ndsOpeningRoomCallbackWalksDObjTree(callback_marker) == FALSE)
        {
            break;
        }
        dobj = dobj->sib_next;
    }
}

static void ndsOpeningRoomRecordTextureMaterialCandidates(GObj *gobj,
                                                         DObj *dobj,
                                                         u32 callback_marker)
{
    u32 visited = 0;

    ndsOpeningRoomRecordTextureMaterialRecurse(
        gobj, dobj, callback_marker, &visited);
}

static void ndsOpeningRoomRecordDObjDraw(GObj *gobj, u32 callback_marker)
{
    DObj *dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
    Gfx *first_dl = NULL;
    Gfx *material_dl = NULL;
    DObj *first_dobj;
    DObj *material_dobj;

    if (gSCManagerSceneData.scene_curr != nSCKindOpeningRoom)
    {
        return;
    }
    gNdsOpeningRoomDrawDObjCallbackCount++;
    gNdsOpeningRoomDrawBlocker =
        NDS_OPENING_ROOM_DRAW_BLOCKER_DOBJ_DISPLAY_LIST;

    first_dobj = ndsOpeningRoomFindDObjDrawCandidate(
        dobj,
        callback_marker,
        FALSE,
        &first_dl);
    material_dobj = ndsOpeningRoomFindDObjDrawCandidate(
        dobj,
        callback_marker,
        TRUE,
        &material_dl);
    ndsOpeningRoomRecordTextureMaterialCandidates(
        gobj,
        dobj,
        callback_marker);

    if ((first_dobj != NULL) &&
        (sNdsOpeningRoomFallbackPreviewDObj == NULL))
    {
        sNdsOpeningRoomFallbackPreviewCameraGObj =
            sNdsOpeningRoomCurrentDrawCameraGObj;
        sNdsOpeningRoomFallbackPreviewGObj = gobj;
        sNdsOpeningRoomFallbackPreviewDObj = first_dobj;
        sNdsOpeningRoomFallbackPreviewDL = first_dl;
    }

    if (material_dobj != NULL)
    {
        gNdsOpeningRoomDrawMaterialCandidateCount++;
        if (sNdsOpeningRoomMaterialPreviewDObj == NULL)
        {
            sNdsOpeningRoomMaterialPreviewCameraGObj =
                sNdsOpeningRoomCurrentDrawCameraGObj;
            sNdsOpeningRoomMaterialPreviewGObj = gobj;
            sNdsOpeningRoomMaterialPreviewDObj = material_dobj;
            sNdsOpeningRoomMaterialPreviewDL = material_dl;
            gNdsOpeningRoomDrawMaterialCandidateResult =
                NDS_OPENING_ROOM_DRAW_MATERIAL_CANDIDATE_PASS;
            gNdsOpeningRoomDrawMaterialCandidateCameraMaskLow =
                (sNdsOpeningRoomCurrentDrawCameraGObj != NULL) ?
                    (u32)(sNdsOpeningRoomCurrentDrawCameraGObj->camera_mask &
                          0xffffffffu) :
                    0u;
            gNdsOpeningRoomDrawMaterialCandidateCameraPriority =
                (sNdsOpeningRoomCurrentDrawCameraGObj != NULL) ?
                    sNdsOpeningRoomCurrentDrawCameraGObj->dl_link_priority :
                    0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateObjectDLLink =
                (gobj != NULL) ? gobj->dl_link_id : 0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateObjectID =
                (gobj != NULL) ? gobj->id : 0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateObjectKind =
                (gobj != NULL) ? gobj->obj_kind : 0xffffffffu;
            gNdsOpeningRoomDrawMaterialCandidateCallback = callback_marker;
            gNdsOpeningRoomDrawMaterialCandidateDObjDL =
                (u32)(uintptr_t)material_dl;
            gNdsOpeningRoomDrawMaterialCandidateDObjMeta =
                ndsOpeningRoomMakeDObjMeta(material_dobj, material_dl);
            {
                NDSOpeningRoomMaterialProbe probe;

                ndsOpeningRoomProbeDObjMaterial(material_dobj, &probe);
                ndsOpeningRoomStoreCandidateMaterial(&probe);
                ndsOpeningRoomRecordMaterialBranch(material_dobj);
                ndsOpeningRoomEmitMaterialBranch(material_dobj);
            }
        }
    }

    if (gNdsOpeningRoomDrawFirstCallback != 0)
    {
        return;
    }

    gNdsOpeningRoomDrawFirstCallback = callback_marker;
    if (gNdsOpeningRoomDrawFirstObjectDLLink == 0xffffffffu)
    {
        gNdsOpeningRoomDrawFirstObjectDLLink =
            (gobj != NULL) ? gobj->dl_link_id : 0xffffffffu;
        gNdsOpeningRoomDrawFirstObjectID =
            (gobj != NULL) ? gobj->id : 0xffffffffu;
        gNdsOpeningRoomDrawFirstObjectKind =
            (gobj != NULL) ? gobj->obj_kind : 0xffffffffu;
    }
    if (first_dobj != NULL)
    {
        gNdsOpeningRoomDrawFirstDObjDL = (u32)(uintptr_t)first_dl;
        gNdsOpeningRoomDrawFirstDObjMeta =
            ndsOpeningRoomMakeDObjMeta(first_dobj, first_dl);
    }
}

static void ndsOpeningRoomRenderSelectedDLPreview(void)
{
    GObj *preview_camera = sNdsOpeningRoomFallbackPreviewCameraGObj;
    DObj *preview_dobj = sNdsOpeningRoomFallbackPreviewDObj;
    Gfx *preview_dl = sNdsOpeningRoomFallbackPreviewDL;
    GObj *saved_preview_camera;

    ndsOpeningRoomCaptureMaterialBranchPreviewState();
    ndsOpeningRoomProbeMaterialDLShape();
    ndsOpeningRoomProbeMaterialDLExpandedShape();

    if ((gNdsOpeningRoomMaterialDLExpandResult ==
         NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_PASS) &&
        (sNdsOpeningRoomMaterialPreviewDObj != NULL))
    {
        preview_camera = sNdsOpeningRoomMaterialPreviewCameraGObj;
        preview_dobj = sNdsOpeningRoomMaterialPreviewDObj;
        preview_dl = sNdsOpeningRoomMaterialPreviewDL;
    }
    if (preview_dobj == NULL)
    {
        return;
    }

    saved_preview_camera = sNdsOpeningRoomPreviewCameraGObj;
    sNdsOpeningRoomPreviewCameraGObj = preview_camera;
    ndsOpeningRoomRenderDLPreview(preview_dobj, preview_dl);
    sNdsOpeningRoomPreviewCameraGObj = saved_preview_camera;
}

void gcCaptureCameraGObj(GObj *camera_gobj, sb32 is_tag_mask_or_id)
{
    u64 camera_mask;
    s32 link_id = 0;

    if (camera_gobj == NULL)
    {
        return;
    }

    camera_mask = camera_gobj->camera_mask;
    while (camera_mask != 0)
    {
        if ((camera_mask & 1u) != 0)
        {
            GObj *current_gobj = gGCCommonDLLinks[link_id];

            while (current_gobj != NULL)
            {
                if (((current_gobj->flags & GOBJ_FLAG_HIDDEN) == 0) &&
                    (current_gobj->proc_display != NULL) &&
                    (((is_tag_mask_or_id == FALSE) &&
                      ((camera_gobj->camera_tag &
                        current_gobj->camera_tag) != 0)) ||
                     ((is_tag_mask_or_id != FALSE) &&
                      (camera_gobj->camera_tag ==
                       current_gobj->camera_tag))))
                {
                    GObj *prev_camera_gobj =
                        sNdsOpeningRoomCurrentDrawCameraGObj;

                    dGCCurrentStatus = nGCStatusDisplaying;
                    gGCCurrentDisplay = current_gobj;

                    ndsStageGCDrawAllLoopRecordCapturedDisplay(camera_gobj,
                                                               current_gobj,
                                                               link_id);
                    ndsOpeningRoomRecordCapturedDisplay(camera_gobj,
                                                        current_gobj,
                                                        link_id);
                    sNdsOpeningRoomCurrentDrawCameraGObj = camera_gobj;
                    current_gobj->proc_display(current_gobj);
                    sNdsOpeningRoomCurrentDrawCameraGObj = prev_camera_gobj;

                    dGCCurrentStatus = nGCStatusCapturing;
                    current_gobj->frame_draw_last = dSYTaskmanFrameCount;
                }
                current_gobj = current_gobj->dl_link_next;
            }
        }
        camera_mask >>= 1;
        link_id++;
    }
}

static void ndsOpeningRoomRecordDrawCamera(GObj *gobj, CObj *cobj)
{
    if ((gobj == NULL) || (cobj == NULL))
    {
        return;
    }
    if (gNdsOpeningRoomDrawFirstCameraPriority != 0xffffffffu)
    {
        return;
    }

    gNdsOpeningRoomDrawFirstCameraMaskLow =
        (u32)(gobj->camera_mask & 0xffffffffu);
    gNdsOpeningRoomDrawFirstCameraPriority = gobj->dl_link_priority;
    gNdsOpeningRoomDrawFirstCameraFlags = cobj->flags;
    gNdsOpeningRoomDrawFirstCameraXObjCount = cobj->xobjs_num;
    gNdsOpeningRoomDrawFirstCameraXObjKind0 =
        ((cobj->xobjs_num > 0) && (cobj->xobjs[0] != NULL)) ?
            cobj->xobjs[0]->kind : 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjKind1 =
        ((cobj->xobjs_num > 1) && (cobj->xobjs[1] != NULL)) ?
            cobj->xobjs[1]->kind : 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraViewportScaleX =
        cobj->viewport.vp.vscale[0];
    gNdsOpeningRoomDrawFirstCameraViewportScaleY =
        cobj->viewport.vp.vscale[1];
    gNdsOpeningRoomDrawFirstCameraViewportTransX =
        cobj->viewport.vp.vtrans[0];
    gNdsOpeningRoomDrawFirstCameraViewportTransY =
        cobj->viewport.vp.vtrans[1];
    gNdsOpeningRoomDrawFirstCameraNear100 =
        ndsPreviewFloatToCenti(cobj->projection.persp.near);
    gNdsOpeningRoomDrawFirstCameraFar100 =
        ndsPreviewFloatToCenti(cobj->projection.persp.far);
    gNdsOpeningRoomDrawFirstCameraFovY100 =
        ndsPreviewFloatToCenti(cobj->projection.persp.fovy);
    gNdsOpeningRoomDrawFirstCameraEyeX100 =
        ndsPreviewFloatToCenti(cobj->vec.eye.x);
    gNdsOpeningRoomDrawFirstCameraEyeY100 =
        ndsPreviewFloatToCenti(cobj->vec.eye.y);
    gNdsOpeningRoomDrawFirstCameraEyeZ100 =
        ndsPreviewFloatToCenti(cobj->vec.eye.z);
    gNdsOpeningRoomDrawFirstCameraAtX100 =
        ndsPreviewFloatToCenti(cobj->vec.at.x);
    gNdsOpeningRoomDrawFirstCameraAtY100 =
        ndsPreviewFloatToCenti(cobj->vec.at.y);
    gNdsOpeningRoomDrawFirstCameraAtZ100 =
        ndsPreviewFloatToCenti(cobj->vec.at.z);
}

void func_80017DBC(GObj *gobj)
{
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    extern volatile u32 gNdsVSResultsCameraProcCount;
#endif
    CObj *cobj;

    if (gobj == NULL)
    {
        return;
    }
    cobj = CObjGetStruct(gobj);
    if (cobj == NULL)
    {
        return;
    }
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        gNdsVSResultsCameraProcCount++;
    }
#endif
    gcCaptureCameraGObj(
        gobj, (cobj->flags & COBJ_FLAG_IDENTIFIER) ? TRUE : FALSE);
}

void func_80017EC0(GObj *gobj)
{
    CObj *cobj;

    if (gobj == NULL)
    {
        return;
    }

    cobj = CObjGetStruct(gobj);
    if (cobj == NULL)
    {
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomDrawCameraCallbackCount++;
        ndsOpeningRoomRecordDrawCamera(gobj, cobj);
        if (gNdsOpeningRoomDrawBlocker == NDS_OPENING_ROOM_DRAW_BLOCKER_NONE)
        {
            gNdsOpeningRoomDrawBlocker =
                NDS_OPENING_ROOM_DRAW_BLOCKER_CAMERA_BACKEND;
        }
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gNdsFighterGCDrawAllLoopPrepared != 0u) &&
        (gNdsFighterMarioFoxGCDrawAllLoopResult == 0u))
    {
        gNdsFighterGCDrawAllLoopCameraCallbackCount++;
    }
    ndsStageGCDrawAllLoopRecordCameraCallback();

    gcCaptureCameraGObj(gobj,
                        (cobj->flags & COBJ_FLAG_IDENTIFIER) ? TRUE : FALSE);
}

void gcDrawDObjTreeForGObj(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE);
}

void gcDrawDObjTreeDLLinksForGObj(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS);
}

void gcDrawDObjDLLinksForGObj(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS);
}

void gcDrawDObjDLHead0(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0);
}

void gcDrawDObjDLHead1(GObj *gobj)
{
    ndsStageGCDrawAllLoopRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1);
    ndsOpeningRoomRecordDObjDraw(
        gobj,
        NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1);
}

void scManagerRunPrintGObjStatus(void)
{
}
