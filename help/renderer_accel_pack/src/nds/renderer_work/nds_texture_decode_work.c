#include <string.h>

#include <nds/renderer_work/nds_texture_decode_work.h>

static u16 be16(const u8 *p)
{
    return (u16)(((u16)p[0] << 8) | (u16)p[1]);
}

static u32 be32(const u8 *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

static u16 rgb5551(u8 r, u8 g, u8 b, u8 a)
{
    return (u16)(((a >= 0x80u) ? 0x8000u : 0u) |
                 ((u16)(r >> 3)) |
                 ((u16)(g >> 3) << 5) |
                 ((u16)(b >> 3) << 10));
}

static u8 expand3(u8 v) { return (u8)((v << 5) | (v << 2) | (v >> 1)); }
static u8 expand4(u8 v) { return (u8)((v << 4) | v); }
static u8 expand5(u8 v) { return (u8)((v << 3) | (v >> 2)); }

u16 ndsTexRgba16ToRgb5551(u16 be_value)
{
    u8 r = expand5((u8)((be_value >> 11) & 0x1Fu));
    u8 g = expand5((u8)((be_value >> 6) & 0x1Fu));
    u8 b = expand5((u8)((be_value >> 1) & 0x1Fu));
    u8 a = (be_value & 1u) ? 0xFFu : 0u;
    return rgb5551(r, g, b, a);
}

static void record(NdsTextureDecodeStats *stats, u32 texels, u16 color)
{
    if (stats == NULL) return;
    if (stats->decoded_texels == 0u) stats->first_pixel = color;
    stats->decoded_texels++;
    if ((color & 0x7FFFu) != 0u) stats->nonzero_texels++;
    if ((color & 0x8000u) == 0u) stats->transparent_texels++;
    (void)texels;
}

static u16 decode_ia(u8 intensity, u8 alpha)
{
    return rgb5551(intensity, intensity, intensity, alpha);
}

static u16 palette_lookup_rgba16(const NdsTextureSource *src, u32 index)
{
    const u8 *pal;
    if ((src == NULL) || (src->palette == NULL) || (index >= src->palette_count))
    {
        return 0u;
    }
    pal = (const u8 *)src->palette;
    return ndsTexRgba16ToRgb5551(be16(&pal[index * 2u]));
}

int ndsTexDecodeToRgb5551(const NdsTextureSource *src,
                          u16 *out,
                          size_t out_texel_capacity,
                          NdsTextureDecodeStats *stats)
{
    const u8 *p;
    size_t total;
    size_t i;

    if (stats != NULL) memset(stats, 0, sizeof(*stats));
    if ((src == NULL) || (src->pixels == NULL) || (out == NULL) ||
        (src->width == 0u) || (src->height == 0u))
    {
        if (stats != NULL) stats->last_error = (u32)NDS_TEX_DECODE_BAD_ARG;
        return NDS_TEX_DECODE_BAD_ARG;
    }

    total = (size_t)src->width * (size_t)src->height;
    if (out_texel_capacity < total)
    {
        if (stats != NULL) stats->last_error = (u32)NDS_TEX_DECODE_OUTPUT_TOO_SMALL;
        return NDS_TEX_DECODE_OUTPUT_TOO_SMALL;
    }

    p = (const u8 *)src->pixels;

    if ((src->fmt == NDS_GBI_FMT_CI) && (src->palette == NULL))
    {
        if (stats != NULL) stats->last_error = (u32)NDS_TEX_DECODE_NEEDS_PALETTE;
        return NDS_TEX_DECODE_NEEDS_PALETTE;
    }

    switch (src->fmt)
    {
    case NDS_GBI_FMT_RGBA:
        if (src->siz == NDS_GBI_SIZ_16B)
        {
            for (i = 0; i < total; i++)
            {
                u16 c = ndsTexRgba16ToRgb5551(be16(&p[i * 2u]));
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        if (src->siz == NDS_GBI_SIZ_32B)
        {
            for (i = 0; i < total; i++)
            {
                u32 c32 = be32(&p[i * 4u]);
                u16 c = rgb5551((u8)(c32 >> 24), (u8)(c32 >> 16),
                                (u8)(c32 >> 8), (u8)c32);
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        break;

    case NDS_GBI_FMT_IA:
        if (src->siz == NDS_GBI_SIZ_4B)
        {
            for (i = 0; i < total; i++)
            {
                u8 nib = (i & 1u) ? (p[i >> 1] & 0x0Fu) : (p[i >> 1] >> 4);
                u8 intensity = expand3((u8)((nib >> 1) & 0x7u));
                u8 alpha = (nib & 1u) ? 0xFFu : 0u;
                u16 c = decode_ia(intensity, alpha);
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        if (src->siz == NDS_GBI_SIZ_8B)
        {
            for (i = 0; i < total; i++)
            {
                u8 v = p[i];
                u16 c = decode_ia(expand4((u8)(v >> 4)), expand4((u8)(v & 0x0Fu)));
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        if (src->siz == NDS_GBI_SIZ_16B)
        {
            for (i = 0; i < total; i++)
            {
                u16 v = be16(&p[i * 2u]);
                u16 c = decode_ia((u8)(v >> 8), (u8)v);
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        break;

    case NDS_GBI_FMT_I:
        if (src->siz == NDS_GBI_SIZ_4B)
        {
            for (i = 0; i < total; i++)
            {
                u8 nib = (i & 1u) ? (p[i >> 1] & 0x0Fu) : (p[i >> 1] >> 4);
                u8 v = expand4(nib);
                u16 c = decode_ia(v, v);
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        if (src->siz == NDS_GBI_SIZ_8B)
        {
            for (i = 0; i < total; i++)
            {
                u8 v = p[i];
                u16 c = decode_ia(v, v);
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        break;

    case NDS_GBI_FMT_CI:
        if (src->siz == NDS_GBI_SIZ_4B)
        {
            for (i = 0; i < total; i++)
            {
                u8 idx = (i & 1u) ? (p[i >> 1] & 0x0Fu) : (p[i >> 1] >> 4);
                u16 c = palette_lookup_rgba16(src, idx);
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        if (src->siz == NDS_GBI_SIZ_8B)
        {
            for (i = 0; i < total; i++)
            {
                u16 c = palette_lookup_rgba16(src, p[i]);
                out[i] = c;
                record(stats, (u32)i, c);
            }
            return NDS_TEX_DECODE_OK;
        }
        break;

    default:
        break;
    }

    if (stats != NULL) stats->last_error = (u32)NDS_TEX_DECODE_UNSUPPORTED;
    return NDS_TEX_DECODE_UNSUPPORTED;
}
