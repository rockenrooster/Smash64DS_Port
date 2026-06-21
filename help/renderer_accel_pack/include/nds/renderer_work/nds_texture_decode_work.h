#ifndef SSB64_NDS_TEXTURE_DECODE_WORK_H
#define SSB64_NDS_TEXTURE_DECODE_WORK_H

#include <stddef.h>
#include <PR/ultratypes.h>
#include <nds/renderer_work/nds_gbi_work.h>

#define NDS_TEX_DECODE_OK 0
#define NDS_TEX_DECODE_BAD_ARG -1
#define NDS_TEX_DECODE_UNSUPPORTED -2
#define NDS_TEX_DECODE_OUTPUT_TOO_SMALL -3
#define NDS_TEX_DECODE_NEEDS_PALETTE -4

typedef struct NdsTextureSource
{
    const void *pixels;
    const void *palette;
    u16 width;
    u16 height;
    u8 fmt;
    u8 siz;
    u8 palette_fmt; /* usually RGBA */
    u8 palette_siz; /* usually 16b */
    u16 palette_count;
} NdsTextureSource;

typedef struct NdsTextureDecodeStats
{
    u32 decoded_texels;
    u32 transparent_texels;
    u32 nonzero_texels;
    u32 first_pixel;
    u32 last_error;
} NdsTextureDecodeStats;

u16 ndsTexRgba16ToRgb5551(u16 be_value);
int ndsTexDecodeToRgb5551(const NdsTextureSource *src,
                          u16 *out,
                          size_t out_texel_capacity,
                          NdsTextureDecodeStats *stats);

#endif
