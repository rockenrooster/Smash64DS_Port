/* Native renderer, the VS battle's wallpaper (P8 V5): the frame's wallpaper
 * tiles (nds_native_wallpaper.c: three direct-colour textures in a lent bank
 * C) drawn at the frame boundary as screen quads behind the scene. A slice
 * of nds_renderer.c; ndsRendererHardwareConsumeSubmittedFrame calls it for
 * every submitted frame.
 *
 * BG2 mapped screen pixel (x, y) to texel ((dx + pa * x) >> 8,
 * (dy + pd * y) >> 8). Each tile is one quad over the screen's width and the
 * rows whose texel row falls in its band, its texture coordinates that
 * affine at the corners (the DS samples a pixel at its top-left edge, where
 * BG2's affine is evaluated too). The projection's last row makes w = 3, so
 * every screen corner is an exact vertex: x = 96 px - 12288, y = 12288 -
 * 128 py at 4.12. Depth is the far plane (z = w) and the quads test Equal:
 * the DS passes Equal within 0x200 of the 24-bit depth (GBATEK), and the
 * rear plane's 0xFFFFFF is 0x1FF from the far plane's 0xFFFE00, so a tile
 * draws exactly where no opaque polygon did -- however close to the far
 * plane the stage's own geometry lies -- and every translucent polygon
 * blends over it. Front-facing and axis-aligned, its edges take no
 * anti-aliasing coverage. */

#define NDS_RENDERER_WALLPAPER_W 12288
#define NDS_RENDERER_WALLPAPER_DEPTH_EQUAL (1u << 14)
#define NDS_RENDERER_WALLPAPER_FAR_POLYS (1u << 12)

/* The first screen pixel whose texel (d + step * p) >> 8 reaches `texel`. */
static s32 ndsRendererWallpaperFirstPixel(u32 texel, s32 d, s32 step)
{
    const s32 n = (s32)(texel << 8) - d;

    return (n <= 0) ? -((-n) / step) : ((n + step - 1) / step);
}

static void ndsRendererHardwareEmitWallpaperTiles(void)
{
    static const m4x4 kProjection = { {
        4096, 0, 0, 0,
        0, 4096, 0, 0,
        0, 0, 4096, 0,
        0, 0, 0, NDS_RENDERER_WALLPAPER_W
    } };
    const NDSNativeWallpaperTiles *tiles = ndsNativeWallpaperTilesForFrame();
    s32 rows[NDS_NATIVE_WALLPAPER_TILE_COUNT + 1u];
    t16 s_left;
    t16 s_right;
    u32 i;

    if ((tiles == NULL) || (tiles->pa <= 0) || (tiles->pd <= 0))
    {
        return;
    }
    for (i = 0u; i <= NDS_NATIVE_WALLPAPER_TILE_COUNT; i++)
    {
        s32 y = ndsRendererWallpaperFirstPixel(tiles->rows[i], tiles->dy,
                                               tiles->pd);

        rows[i] = (y < 0) ? 0 : ((y > SCREEN_HEIGHT) ? SCREEN_HEIGHT : y);
    }
    s_left = (t16)((tiles->dx + 8) >> 4);
    s_right = (t16)((tiles->dx + (tiles->pa * SCREEN_WIDTH) + 8) >> 4);
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    glViewport(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&kProjection);
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor(RGB15(31, 31, 31));
    ndsRendererHardwareSetPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK | POLY_ID(0) |
                                  NDS_RENDERER_WALLPAPER_DEPTH_EQUAL |
                                  NDS_RENDERER_WALLPAPER_FAR_POLYS);
    for (i = 0u; i < NDS_NATIVE_WALLPAPER_TILE_COUNT; i++)
    {
        const s32 y0 = rows[i];
        const s32 y1 = rows[i + 1u];
        const s32 band = (s32)tiles->rows[i] << 8;
        t16 t_top;
        t16 t_bottom;

        if (y0 >= y1)
        {
            continue;
        }
        t_top = (t16)((tiles->dy + (tiles->pd * y0) - band + 8) >> 4);
        t_bottom = (t16)((tiles->dy + (tiles->pd * y1) - band + 8) >> 4);
        GFX_TEX_FORMAT = tiles->params[i];
        glBegin(GL_QUAD);
        glTexCoord2t16(s_left, t_top);
        glVertex3v16((v16)-NDS_RENDERER_WALLPAPER_W,
                     (v16)(NDS_RENDERER_WALLPAPER_W - (128 * y0)),
                     (v16)NDS_RENDERER_WALLPAPER_W);
        glTexCoord2t16(s_left, t_bottom);
        glVertex3v16((v16)-NDS_RENDERER_WALLPAPER_W,
                     (v16)(NDS_RENDERER_WALLPAPER_W - (128 * y1)),
                     (v16)NDS_RENDERER_WALLPAPER_W);
        glTexCoord2t16(s_right, t_bottom);
        glVertex3v16((v16)NDS_RENDERER_WALLPAPER_W,
                     (v16)(NDS_RENDERER_WALLPAPER_W - (128 * y1)),
                     (v16)NDS_RENDERER_WALLPAPER_W);
        glTexCoord2t16(s_right, t_top);
        glVertex3v16((v16)NDS_RENDERER_WALLPAPER_W,
                     (v16)(NDS_RENDERER_WALLPAPER_W - (128 * y0)),
                     (v16)NDS_RENDERER_WALLPAPER_W);
        glEnd();
    }
    sNdsRendererHardwareBoundTextureName = 0;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
}
