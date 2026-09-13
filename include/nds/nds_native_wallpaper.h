#ifndef NDS_NATIVE_WALLPAPER_H
#define NDS_NATIVE_WALLPAPER_H

#include <ssb_types.h>

/* Asset pixels are converted on the host; these calls never read source
 * graphics commands or decode a source bitmap. */
s32 ndsNativeWallpaperDraw(u32 asset_id, u32 bitmap_offset,
                          s32 origin_x, s32 origin_y,
                          u32 scale_x_q16, u32 scale_y_q16,
                          const u16 *palette);
/* Native battle owner for the eight VS wallpapers outside Dream Land.
 * gkind follows BattleShip gr/grdef.h (Castle=0 .. Inishie=8). Dream Land
 * (6) deliberately returns success without touching BG2: its accepted DObj
 * sky/cloud/Whispy backdrop remains the sole background owner there. */
s32 ndsNativeBattleWallpaperDraw(u32 gkind,
                                 f32 eye_x, f32 eye_y, f32 eye_z,
                                 f32 at_x, f32 at_y, f32 at_z);
s32 ndsNativeBattleWallpaperPreload(u32 gkind);
void ndsNativeWallpaperInvalidate(void);

#endif
