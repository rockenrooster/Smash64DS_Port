#include <nds.h>
#include <string.h>

#include <nds/nds_native_wallpaper.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>

#include "generated/native_wallpapers.generated.inc"

/* Only one BG2 image is resident. Row staging is main RAM and all VRAM
 * stores are halfwords; fread must never write directly to VRAM. */
static u16 sWallpaperRow[240] __attribute__((aligned(32)));
static const NDSNativeWallpaper *sWallpaperAsset;
static u32 sWallpaperEpoch;
static u16 sWallpaperPalette[16];

volatile u32 gNdsNativeWallpaperLoadCount;
volatile u32 gNdsNativeWallpaperReuseCount;
volatile u32 gNdsNativeWallpaperReadFailureCount;

void ndsNativeWallpaperInvalidate(void)
{
    sWallpaperAsset = NULL;
    sWallpaperEpoch = 0u;
    (void)ndsPlatformQueueNativeWallpaperAffine(256, 256, 0, 0);
}

static s32 ndsNativeWallpaperUpload(const NDSNativeWallpaper *asset,
                                   u16 *dest, u32 pitch, const u16 *palette)
{
    NdsRelocAssetStream stream = { NULL };
    char path[96];
    u32 y;
    u32 bytes_per_row = asset->native_w * ((asset->format == 0u) ? 2u : 1u);
    const char prefix[] = "nitro:/wallpapers/";
    size_t name_bytes = strlen(asset->nitro_filename) + 1u;

    if (name_bytes > sizeof(path) - (sizeof(prefix) - 1u))
    {
        return FALSE;
    }
    memcpy(path, prefix, sizeof(prefix) - 1u);
    memcpy(path + sizeof(prefix) - 1u, asset->nitro_filename, name_bytes);
    if (ndsRelocAssetStreamOpen(&stream, path) == FALSE)
    {
        return FALSE;
    }
    dmaFillHalfWords(0x8000u, dest, 256u * 192u * sizeof(u16));
    for (y = 0u; y < asset->native_h; y++)
    {
        u32 x;
        if (ndsRelocAssetStreamRead(&stream, y * bytes_per_row,
                                    sWallpaperRow, bytes_per_row) == FALSE)
        {
            ndsRelocAssetStreamClose(&stream);
            return FALSE;
        }
        if (asset->format == 1u)
        {
            /* The converted file contains intensity bytes, not source I4
             * strips. Expand backwards so input and output share one row. */
            const u8 *indices = (const u8 *)sWallpaperRow;
            for (x = asset->native_w; x != 0u; x--)
            {
                u32 intensity = indices[x - 1u];
                if ((intensity % 17u) != 0u)
                {
                    ndsRelocAssetStreamClose(&stream);
                    return FALSE;
                }
                sWallpaperRow[x - 1u] = palette[intensity / 17u];
            }
        }
        for (x = 0u; x < asset->native_w; x++)
        {
            if ((sWallpaperRow[x] & 0x8000u) == 0u)
            {
                ndsRelocAssetStreamClose(&stream);
                return FALSE;
            }
            ((volatile u16 *)dest)[y * pitch + x] = sWallpaperRow[x];
        }
    }
    ndsRelocAssetStreamClose(&stream);
    return TRUE;
}

/* Converted texels already include the 4/5 source-to-DS scale. For battle,
 * BG source x = (screen x - source_origin * 4/5) / source_scale.
 * Match pixel centres when converting that inverse transform to Q8.
 * Results uses its existing (10,10)-(310,230) full-screen crop. */
static s32 ndsNativeWallpaperAffine(const NDSNativeWallpaper *asset,
                                   s32 origin_x, s32 origin_y,
                                   u32 scale_x_q16, u32 scale_y_q16,
                                   s32 *pa, s32 *pd, s32 *dx, s32 *dy)
{
    s64 x;
    s64 y;
    if ((scale_x_q16 == 0u) || (scale_y_q16 == 0u) ||
        (origin_x < -32768) || (origin_x > 32767) ||
        (origin_y < -32768) || (origin_y > 32767))
    {
        return FALSE;
    }
    if (asset->format == 1u)
    {
        *pa = ((u32)asset->native_w * 256u + 128u) / 256u;
        *pd = ((u32)asset->native_h * 256u + 96u) / 192u;
        *dx = *pa / 2;
        *dy = *pd / 2;
        return TRUE;
    }
    *pa = (s32)(((1u << 24) + scale_x_q16 / 2u) / scale_x_q16);
    *pd = (s32)(((1u << 24) + scale_y_q16 / 2u) / scale_y_q16);
    if ((*pa <= 0) || (*pa > 32767) || (*pd <= 0) || (*pd > 32767))
    {
        return FALSE;
    }
    x = (s64)*pa / 2 - ((s64)origin_x * 4 * *pa) / 5;
    y = (s64)*pd / 2 - ((s64)origin_y * 4 * *pd) / 5;
    if ((x < -134217728) || (x > 134217727) ||
        (y < -134217728) || (y > 134217727))
    {
        return FALSE;
    }
    *dx = (s32)x;
    *dy = (s32)y;
    return TRUE;
}

s32 ndsNativeWallpaperDraw(u32 asset_id, u32 bitmap_offset,
                          s32 origin_x, s32 origin_y,
                          u32 scale_x_q16, u32 scale_y_q16,
                          const u16 *palette)
{
    const NDSNativeWallpaper *asset = NULL;
    u32 i, pitch, width, height, epoch;
    u16 *dest;
    s32 pa, pd, dx, dy;
    for (i = 0u; i < kNDSNativeWallpaperCount; i++)
    {
        if ((kNDSNativeWallpapers[i].asset_id == asset_id) &&
            (kNDSNativeWallpapers[i].src_offset == bitmap_offset))
        {
            asset = &kNDSNativeWallpapers[i];
            break;
        }
    }
    if ((asset == NULL) || (asset->native_w != 240u) ||
        (asset->native_h != 176u) || (asset->format > 1u) ||
        ((asset->format == 1u) != (palette != NULL)) ||
        (ndsNativeWallpaperAffine(asset, origin_x, origin_y,
                                  scale_x_q16, scale_y_q16,
                                  &pa, &pd, &dx, &dy) == FALSE))
    {
        return FALSE;
    }
    dest = ndsPlatformGetOriginalSpriteOverlayLayer(FALSE, &pitch, &width,
                                                   &height, &epoch);
    if ((dest == NULL) || (pitch != 256u) || (width != 256u) || (height != 192u))
    {
        return FALSE;
    }
    if ((sWallpaperAsset != asset) || (sWallpaperEpoch != epoch) ||
        ((palette != NULL) &&
         (memcmp(palette, sWallpaperPalette, sizeof(sWallpaperPalette)) != 0)))
    {
        sWallpaperAsset = NULL;
        if (ndsNativeWallpaperUpload(asset, dest, pitch, palette) == FALSE)
        {
            gNdsNativeWallpaperReadFailureCount++;
            return FALSE;
        }
        epoch = ndsPlatformCommitOriginalSpriteFinalLayer(FALSE, 256u * 192u);
        if (epoch == 0u)
        {
            return FALSE;
        }
        sWallpaperAsset = asset;
        sWallpaperEpoch = epoch;
        if (palette != NULL)
        {
            memcpy(sWallpaperPalette, palette, sizeof(sWallpaperPalette));
        }
        gNdsNativeWallpaperLoadCount++;
    }
    else
    {
        gNdsNativeWallpaperReuseCount++;
    }
    return ndsPlatformQueueNativeWallpaperAffine(pa, pd, dx, dy);
}
