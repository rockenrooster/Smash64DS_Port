#ifndef NDS_NATIVE_WALLPAPER_H
#define NDS_NATIVE_WALLPAPER_H

#include <ssb_types.h>

/* Asset pixels are converted on the host; these calls never read source
 * graphics commands or decode a source bitmap. */
s32 ndsNativeWallpaperDraw(u32 asset_id, u32 bitmap_offset,
                          s32 origin_x, s32 origin_y,
                          u32 scale_x_q16, u32 scale_y_q16,
                          const u16 *palette);
void ndsNativeWallpaperInvalidate(void);

#endif
