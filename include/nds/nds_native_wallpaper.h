/* Native wallpapers on BG2, or in a VS battle as texture tiles in bank C
 * (src/nds/nds_native_wallpaper.c). */

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
/* grWallpaperCalcPersp (motion 0) / grWallpaperSectorProcUpdate (1) /
 * grWallpaperMakeStatic (2) in integers: the wallpaper's position and scale
 * at Q16 from the camera's eye and target. FALSE past the fixed range. */
s32 ndsWallpaperPerspQ(u32 motion, const Vec3f *eye, const Vec3f *at,
                       s32 *pos_x_q16, s32 *pos_y_q16, s32 *scale_q16);
void ndsNativeWallpaperInvalidate(void);

/* P8 V5: in a VS battle the wallpaper is three direct-colour textures --
 * image rows 0-127, 128-159 and 160-175, each 256 texels wide -- in bank C,
 * lent to the 3D engine from the battle's first submitted frame
 * (ndsPlatformVramTakeBankC), drawn by the renderer behind the scene through
 * BG2's own affine (nds_renderer_wallpaper.c). NULL when the frame has none:
 * another scene, no wallpaper asked for, or this battle's BG2 fallback after
 * a failed take. Release: the battle exit, after the renderer locked C and
 * let go of its other names. Forget: glResetTextures is about to free it. */
#define NDS_NATIVE_WALLPAPER_TILE_COUNT 3u
typedef struct NDSNativeWallpaperTiles
{
    u32 params[NDS_NATIVE_WALLPAPER_TILE_COUNT];  /* TEXIMAGE_PARAM words */
    u16 rows[NDS_NATIVE_WALLPAPER_TILE_COUNT + 1u]; /* first image row each */
    s32 pa;                                         /* texel = (d + p * px) */
    s32 pd;                                         /* >> 8, BG2's affine */
    s32 dx;
    s32 dy;
} NDSNativeWallpaperTiles;
const NDSNativeWallpaperTiles *ndsNativeWallpaperTilesForFrame(void);
void ndsNativeWallpaperTilesRelease(void);
void ndsNativeWallpaperTilesForget(void);

/* P7 C4: Training's backdrop, named by battleship_sc1ptrainingmode.c
 * sc1PTrainingModeLoadWallpaper (0 before a Training battle): the converted
 * GRWallpaperTraining image the static wallpaper draws in the map's place.
 * Ids are the files' (Black, Yellow, Blue); the bitmap offset is the
 * generator's key for all three. */
#define NDS_TRAINING_WALLPAPER_ASSET_BLACK 0x1Au
#define NDS_TRAINING_WALLPAPER_BITMAP 0x204C8u
extern u32 gNdsTrainingWallpaperAsset;

#endif
