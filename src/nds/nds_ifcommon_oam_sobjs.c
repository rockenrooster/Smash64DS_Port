/* IFCommon on OAM, the SObjs: battle-screen coordinates, the traffic lamps'
 * specs, the cloud SObjs and the player tags. A slice of nds_ifcommon_oam.c. */

static s32 ndsIFCommonRoundFloatHalfUp(f32 value)
{
    return (value >= 0.0F) ? (s32)(value + 0.5F) :
                             (s32)(value - 0.5F);
}

static s32 ndsIFCommonRoundQ16HalfUp(s32 value)
{
    return (value >= 0) ? ((value + 0x8000) >> 16) :
                          -(((-value) + 0x8000) >> 16);
}

/* The HUD's anchors without soft float (owner 2026-10-05: fixed point only).
 * Render only. A source float (an SObj position or scale the decomp wrote)
 * enters at Q16 through ndsF32ToFixed, integer bit arithmetic; the float
 * products `value * k` and their half-away rounding become one 64-bit
 * product, so an anchor can land one unit from the float product's. */
static s32 __attribute__((noinline, target("arm")))
ndsIFCommonScaleRound(f32 value, u32 k)
{
    const int64_t prod = (int64_t)ndsF32ToFixed(value, 16u) * (int64_t)k;

    return (prod >= 0) ? (s32)((prod + 0x8000) >> 16) :
                         -(s32)(((-prod) + 0x8000) >> 16);
}

/* An unsigned scale (a negative source scale reads 0, as the float
 * conversion's truncation to unsigned did). */
static u32 ndsIFCommonScaleRoundU(f32 value, u32 k)
{
    const s32 v = ndsIFCommonScaleRound(value, k);

    return (v > 0) ? (u32)v : 0u;
}

/* A Q22 value rounded half away from zero to an integer. */
static s32 __attribute__((noinline, target("arm")))
ndsIFCommonRoundQ22HalfUp(int64_t value)
{
    return (value >= 0) ? (s32)((value + (1 << 21)) >> 22) :
                          -(s32)(((-value) + (1 << 21)) >> 22);
}

/* ndsIFCommonBattleScreenX/Y at Q22 (see there): 128 + x * 128 / half_w and
 * 96 - y * 96 / half_h, half = viewport / 2, or the 0.8 fallback. */
static int64_t __attribute__((noinline, target("arm")))
ndsIFCommonBattleScreenXQ22(int64_t projected_x_q22)
{
    const s32 width = gGMCameraStruct.viewport_width;

    if (width > 0)
    {
        return (INT64_C(128) << 22) +
               ndsR2HwMathDivideFast(projected_x_q22 * 256, width);
    }
    return (INT64_C(128) << 22) +
           ndsR2HwMathDivideFast(projected_x_q22 * 4, 5);
}

static int64_t __attribute__((noinline, target("arm")))
ndsIFCommonBattleScreenYQ22(int64_t projected_y_q22)
{
    const s32 height = gGMCameraStruct.viewport_height;

    if (height > 0)
    {
        return (INT64_C(96) << 22) -
               ndsR2HwMathDivideFast(projected_y_q22 * 192, height);
    }
    return (INT64_C(96) << 22) -
           ndsR2HwMathDivideFast(projected_y_q22 * 4, 5);
}

/* gmCameraCheckTargetInBounds at Q22: the half extents are the source's
 * integer quotients. */
static s32 ndsIFCommonInBoundsQ22(int64_t x_q22, int64_t y_q22)
{
    const int64_t half_w = (int64_t)(gGMCameraStruct.viewport_width / 2) << 22;
    const int64_t half_h = (int64_t)(gGMCameraStruct.viewport_height / 2) << 22;

    return ((x_q22 < -half_w) || (x_q22 > half_w) || (y_q22 < -half_h) ||
            (y_q22 > half_h)) ? FALSE : TRUE;
}

/* A battle point the source projects (func_ovl2_800EB924: x, y about the
 * viewport centre, in source pixels) on the DS screen. The battle's 3D fills
 * the screen, so the camera's 300 x 220 viewport maps onto 256 x 192: x
 * scales by 128/150 and y by 96/110, not the 2D layer's uniform 0.8, which is
 * right only for screen-fixed HUD. Anchoring a fighter's tag, an item arrow
 * or a hit spark at 0.8 pulled it toward the centre by up to ~8 px at the
 * screen's edges (owner 10-01: tags "offset around edges of screen"). */
static f32 ndsIFCommonBattleScreenX(f32 projected_x)
{
    f32 half = (f32)gGMCameraStruct.viewport_width * 0.5F;

    return (half > 0.0F) ? (128.0F + (projected_x * (128.0F / half))) :
                           (128.0F + (projected_x * 0.8F));
}

static f32 ndsIFCommonBattleScreenY(f32 projected_y)
{
    f32 half = (f32)gGMCameraStruct.viewport_height * 0.5F;

    return (half > 0.0F) ? (96.0F - (projected_y * (96.0F / half))) :
                           (96.0F - (projected_y * 0.8F));
}

static s32 ndsIFCommonEmitSObj(const SObj *sobj, u32 asset_index)
{
    const NDSIFCommonNativeAsset *asset =
        &sNdsIFCommonAssets[asset_index];
    u32 scale_x_q16;
    u32 scale_y_q16;
    u16 inverse_x;
    u16 inverse_y;
    s32 matrix_index;
    s32 origin_x;
    s32 origin_y;
    u32 tile_index;
    s32 size_double;
    u32 prefiltered;

    if (asset_index >= NDS_IFCOMMON_END_FIRST)
    {
        if (ndsIFCommonEndAssetActive(asset_index) == FALSE)
        {
            return FALSE;
        }
    }
    for (tile_index = 0u; tile_index < asset->tile_count; tile_index++)
    {
        if ((asset->tiles[tile_index].color_format !=
             SpriteColorFormat_Bmp) && (sobj->sprite.alpha != 255u))
        {
            return FALSE;
        }
        if ((asset_index >= NDS_IFCOMMON_END_FIRST) &&
            (asset->tiles[tile_index].gfx == NULL))
        {
            return FALSE;
        }
    }
    /* GO and end bitmaps are already final-DS-resolution: source scalex
     * applies 1:1 locally while the source position still scales by 0.8. */
    prefiltered = ((asset_index <= nNDSIFCommonAssetGoExclaim) ||
                   (asset_index >= NDS_IFCOMMON_END_FIRST)) ?
                      TRUE : FALSE;

    scale_x_q16 = ndsIFCommonScaleRoundU(sobj->sprite.scalex,
        prefiltered ? (1u << 16) : NDS_IFCOMMON_SCREEN_SCALE_Q16);
    scale_y_q16 = ndsIFCommonScaleRoundU(sobj->sprite.scaley,
        prefiltered ? (1u << 16) : NDS_IFCOMMON_SCREEN_SCALE_Q16);
    if ((scale_x_q16 == 0u) || (scale_y_q16 == 0u) ||
        (scale_x_q16 != scale_y_q16))
    {
        return FALSE;
    }
    inverse_x = (u16)(((1u << 24) + (scale_x_q16 / 2u)) /
                      scale_x_q16);
    inverse_y = (u16)(((1u << 24) + (scale_y_q16 / 2u)) /
                      scale_y_q16);
    if (inverse_x != inverse_y)
    {
        return FALSE;
    }
    matrix_index = ndsIFCommonMatrixForScale(inverse_x);
    if (matrix_index < 0)
    {
        return FALSE;
    }
    origin_x = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
        sobj->pos.x, NDS_IFCOMMON_SCREEN_SCALE_Q16));
    origin_y = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
        sobj->pos.y, NDS_IFCOMMON_SCREEN_SCALE_Q16));
    size_double = (scale_x_q16 > (1u << 16)) ? TRUE : FALSE;

    for (tile_index = 0u; tile_index < asset->tile_count; tile_index++)
    {
        const NDSIFCommonNativeTile *tile = &asset->tiles[tile_index];
        const NDSIFCommonTileSpec *spec = &tile->spec;
        s32 local_center_x = (s32)spec->source_x +
            ((s32)spec->cell_width / 2) - (s32)spec->pad_x;
        s32 local_center_y = (s32)spec->source_y +
            ((s32)spec->cell_height / 2) - (s32)spec->pad_y;
        s32 center_x = origin_x + ndsIFCommonRoundQ16HalfUp(
            (s32)((s64)local_center_x * (s64)scale_x_q16));
        s32 center_y = origin_y + ndsIFCommonRoundQ16HalfUp(
            (s32)((s64)local_center_y * (s64)scale_y_q16));
        s32 half_bounds_x = size_double ? spec->cell_width :
                                              (spec->cell_width / 2);
        s32 half_bounds_y = size_double ? spec->cell_height :
                                              (spec->cell_height / 2);
        s32 x = center_x - half_bounds_x;
        s32 y = center_y - half_bounds_y;
        s32 oam_id = sNdsIFCommonNextOamID;
        u32 bitmap_alpha = ndsIFCommonBitmapAlpha(sobj->sprite.alpha);

        oamSet(&oamMain, oam_id, x, y, 0,
               (tile->color_format == SpriteColorFormat_Bmp) ?
                   (int)bitmap_alpha : 0,
               tile->size, (SpriteColorFormat)tile->color_format, tile->gfx,
               matrix_index, size_double, false, false, false, false);
        sNdsIFCommonNextOamID--;
        gNdsIFCommonNativeOamFrameObjectCount++;
    }
    return TRUE;
}

static const NDSIFCommonTrafficSpec *ndsIFCommonTrafficSpecForAsset(
    u32 asset_index)
{
    u32 traffic_index;

    for (traffic_index = 0u;
         traffic_index < NDS_IFCOMMON_TRAFFIC_SPEC_COUNT; traffic_index++)
    {
        if ((u32)sNdsIFCommonTrafficSpecs[traffic_index].asset_index ==
            asset_index)
        {
            return &sNdsIFCommonTrafficSpecs[traffic_index];
        }
    }
    return NULL;
}

static s32 ndsIFCommonTrafficSObjValid(
    const SObj *sobj, const NDSIFCommonTrafficSpec *traffic)
{
    u32 scale_x_q16;
    u32 scale_y_q16;

    if ((sobj == NULL) || (traffic == NULL) ||
        (sNdsIFCommonTrafficTextureName == 0u) ||
        (sobj->sprite.alpha != 255u))
    {
        return FALSE;
    }
    scale_x_q16 = ndsIFCommonScaleRoundU(sobj->sprite.scalex,
        NDS_IFCOMMON_SCREEN_SCALE_Q16);
    scale_y_q16 = ndsIFCommonScaleRoundU(sobj->sprite.scaley,
        NDS_IFCOMMON_SCREEN_SCALE_Q16);
    return ((scale_x_q16 == NDS_IFCOMMON_SCREEN_SCALE_Q16) &&
            (scale_y_q16 == NDS_IFCOMMON_SCREEN_SCALE_Q16)) ? TRUE : FALSE;
}

static s32 ndsIFCommonEmitTrafficSObj(
    const SObj *sobj, const NDSIFCommonTrafficSpec *traffic)
{
    s32 origin_x = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
        sobj->pos.x, NDS_IFCOMMON_SCREEN_SCALE_Q16));
    s32 origin_y = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
        sobj->pos.y, NDS_IFCOMMON_SCREEN_SCALE_Q16));

    if (ndsRendererHardwareDrawIFCommonCloudAtlas(
            sNdsIFCommonTrafficTextureName,
            origin_x * (s32)(1u << 16),
            origin_y * (s32)(1u << 16),
            (s32)((u32)traffic->width << 16),
            (s32)((u32)traffic->height << 16),
            traffic->atlas_x, traffic->atlas_y,
            traffic->width, traffic->height,
            48u + traffic->asset_index) == FALSE)
    {
        return FALSE;
    }
    gNdsIFCommonNativeOamFrameCloudDrawCount++;
    return TRUE;
}

static const NDSIFCommonCloudSpec *ndsIFCommonCloudSpecForAsset(
    u32 asset_index)
{
    u32 cloud_index;

    for (cloud_index = 0u;
         cloud_index < NDS_IFCOMMON_CLOUD_SPEC_COUNT; cloud_index++)
    {
        if ((u32)sNdsIFCommonCloudSpecs[cloud_index].asset_index ==
            asset_index)
        {
            return &sNdsIFCommonCloudSpecs[cloud_index];
        }
    }
    return NULL;
}

static s32 ndsIFCommonCloudSObjValid(
    const SObj *sobj, const NDSIFCommonCloudSpec *cloud)
{
    u32 scale_x_q16;
    u32 scale_y_q16;

    if ((sobj == NULL) || (cloud == NULL))
    {
        return FALSE;
    }
    if ((cloud->atlas_index >= NDS_IFCOMMON_CLOUD_ATLAS_COUNT) ||
        (sNdsIFCommonCloudTextureNames[cloud->atlas_index] == 0u) ||
        (sobj->sprite.alpha != 255u))
    {
        return FALSE;
    }
    scale_x_q16 = ndsIFCommonScaleRoundU(sobj->sprite.scalex,
        NDS_IFCOMMON_SCREEN_SCALE_Q16);
    scale_y_q16 = ndsIFCommonScaleRoundU(sobj->sprite.scaley,
        NDS_IFCOMMON_SCREEN_SCALE_Q16);
    if ((scale_x_q16 == 0u) || (scale_x_q16 != scale_y_q16))
    {
        return FALSE;
    }
    return ((cloud->kind != nNDSIFCommonCloudLight) ||
            (scale_x_q16 == NDS_IFCOMMON_SCREEN_SCALE_Q16)) ? TRUE : FALSE;
}

static s32 ndsIFCommonEmitCloudSObj(
    const SObj *sobj, const NDSIFCommonCloudSpec *cloud)
{
    s32 origin_x_q16 = ndsIFCommonScaleRound(sobj->pos.x,
        NDS_IFCOMMON_SCREEN_SCALE_Q16);
    s32 origin_y_q16 = ndsIFCommonScaleRound(sobj->pos.y,
        NDS_IFCOMMON_SCREEN_SCALE_Q16);
    u32 source_scale_q16 = ndsIFCommonScaleRoundU(sobj->sprite.scalex,
        NDS_IFCOMMON_SCREEN_SCALE_Q16);
    u32 atlas_scale_q16 = (cloud->kind == nNDSIFCommonCloudLight) ?
        (1u << 16) : (source_scale_q16 + 1u) / 2u;
    s32 x_q16 = origin_x_q16 +
        (s32)((u32)cloud->source_x * atlas_scale_q16);
    s32 y_q16 = origin_y_q16 +
        (s32)((u32)cloud->source_y * atlas_scale_q16);
    s32 width_q16 = (s32)((u32)cloud->width * atlas_scale_q16);
    s32 height_q16 = (s32)((u32)cloud->height * atlas_scale_q16);
    u32 poly_id = (cloud->kind == nNDSIFCommonCloudLight) ? 63u : 62u;

    if (ndsRendererHardwareDrawIFCommonCloudAtlas(
            sNdsIFCommonCloudTextureNames[cloud->atlas_index],
            x_q16, y_q16, width_q16, height_q16,
            cloud->atlas_x, cloud->atlas_y,
            cloud->width, cloud->height, poly_id) == FALSE)
    {
        return FALSE;
    }
    gNdsIFCommonNativeOamFrameCloudDrawCount++;
    return TRUE;
}

/* Player tags (1P/2P/3P/4P/CP and the 1P allies' heart, source
 * ifCommonPlayerTagProcDisplay).
 *
 * The six glyphs are IA8 bitmaps 15-21 wide by 24 high living in the
 * IFCommonPlayerTags reloc file, not the GameStatus file this unit prepares,
 * so they cannot ride the hybrid OBJ bake. Each distinct live bitmap is
 * decoded once into its own 4bpp 32x32 OBJ cell; the draw arm then replays
 * the source placement exactly and tints through a 16-entry intensity
 * palette built from that player's source prim colour over black env.
 *
 * Palettes 11-15 hold the five prim ramps. No emitted direct-colour tile
 * reads the main OBJ palettes: GO and the end letters are direct-colour
 * Bmp, and every indexed asset routes to the cloud/traffic texture atlases
 * instead of OAM. Tags live in the fixed tag bank after the spark sheet.
 * Lower HUD (timer/stock/damage) lives on oamSub and is untouched here. */
static void ndsIFCommonResetPlayerTags(void)
{
    memset(sNdsIFCommonPlayerTags, 0, sizeof(sNdsIFCommonPlayerTags));
    sNdsIFCommonPlayerTagCursor = 0u;
    sNdsIFCommonPlayerTagCursorValid = FALSE;
}

static const Bitmap *sNdsIFCommonTaskBitmap;

static void ndsIFCommonResetItemArrow(void)
{
    memset(&sNdsIFCommonItemArrow, 0, sizeof(sNdsIFCommonItemArrow));
    sNdsIFCommonTaskBitmap = NULL;
}

/* IFCommonItem's pickup arrow is source I4 + SP_TRANSPARENT. The retained
 * generic reference treats ordinary I4 as binary coverage: nibble zero is
 * transparent and every nonzero nibble emits the Sprite prim colour. Keep
 * that exact contract in one 16x8 4bpp OBJ cell; the source itself is 9x7. */
static s32 ndsIFCommonReadItemArrowI4(const Sprite *sprite, u32 source_x,
                                     u32 source_y, u8 *intensity)
{
    const Bitmap *bitmap;
    const u8 *pixels;
    u32 width_img;
    u32 height;
    u32 shuffled_x;
    u32 row_bytes;
    u8 packed;

    if ((sprite == NULL) || (intensity == NULL) ||
        (sprite->bitmap == NULL) || (sprite->nbitmaps != 1))
    {
        return FALSE;
    }
    bitmap = sprite->bitmap;
    width_img = (u32)(u16)bitmap->width_img;
    height = (u32)(u16)bitmap->actualHeight;
    if (width_img == 0u)
    {
        width_img = (u32)(u16)bitmap->width;
    }
    if (height == 0u)
    {
        height = (u32)(u16)sprite->height;
    }
    if ((source_x >= (u32)(u16)sprite->width) || (source_y >= height))
    {
        return FALSE;
    }
    shuffled_x = source_x ^ ((source_y & 1u) != 0u ? 8u : 0u);
    if (shuffled_x >= width_img)
    {
        return FALSE;
    }
    row_bytes = (width_img + 1u) / 2u;
    pixels = (const u8 *)bitmap->buf;
    if (pixels == NULL)
    {
        return FALSE;
    }
    packed = pixels[(((size_t)source_y * row_bytes) +
                     (shuffled_x >> 1)) ^ 3u];
    *intensity = ((shuffled_x & 1u) == 0u) ?
        (u8)(packed >> 4) : (u8)(packed & 0x0fu);
    return TRUE;
}

s32 ndsIFCommonNativeOamBakeItemArrow(const Sprite *sprite)
{
    u32 cell[NDS_IFCOMMON_ITEM_BANK_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u16 *gfx;
    u32 *dst;
    u16 *palette;
    u32 word;
    u32 y;

    if ((sprite == NULL) || (sprite->bitmap == NULL) ||
        (sprite->bmfmt != G_IM_FMT_I) ||
        (sprite->bmsiz != G_IM_SIZ_4b) || (sprite->nbitmaps != 1) ||
        ((u32)(u16)sprite->width != NDS_IFCOMMON_ITEM_WIDTH) ||
        ((u32)(u16)sprite->height != NDS_IFCOMMON_ITEM_HEIGHT))
    {
        return FALSE;
    }
    if ((sNdsIFCommonItemArrow.baked != FALSE) &&
        (sNdsIFCommonItemArrow.bitmap == sprite->bitmap) &&
        (sNdsIFCommonItemArrow.runtime_red == (u32)sprite->red) &&
        (sNdsIFCommonItemArrow.runtime_green == (u32)sprite->green) &&
        (sNdsIFCommonItemArrow.runtime_blue == (u32)sprite->blue))
    {
        return TRUE;
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        return FALSE;
    }

    memset(cell, 0, sizeof(cell));
    for (y = 0u; y < NDS_IFCOMMON_ITEM_HEIGHT; y++)
    {
        u32 x;

        for (x = 0u; x < NDS_IFCOMMON_ITEM_WIDTH; x++)
        {
            u8 intensity;
            u32 tile;
            u32 offset;

            if (ndsIFCommonReadItemArrowI4(sprite, x, y, &intensity) == FALSE)
            {
                return FALSE;
            }
            if (intensity == 0u)
            {
                continue;
            }
            tile = (x >> 3);
            offset = (tile * 32u) + ((y & 7u) * 4u) +
                     ((x & 7u) >> 1);
            if ((x & 1u) == 0u)
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) | 1u);
            }
            else
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0x0fu) | 0x10u);
            }
        }
    }

    gfx = (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_ITEM_BANK_BASE);
    dst = (u32 *)(void *)gfx;
    for (word = 0u; word < (u32)(sizeof(cell) / sizeof(cell[0])); word++)
    {
        dst[word] = cell[word];
    }
    palette = &SPRITE_PALETTE[NDS_IFCOMMON_ITEM_PALETTE * 16u];
    palette[0] = 0u;
    for (word = 1u; word < 16u; word++)
    {
        palette[word] = ndsIFCommonPackRgb15(
            sprite->red, sprite->green, sprite->blue);
    }
    sNdsIFCommonItemArrow.bitmap = sprite->bitmap;
    sNdsIFCommonItemArrow.width = NDS_IFCOMMON_ITEM_WIDTH;
    sNdsIFCommonItemArrow.height = NDS_IFCOMMON_ITEM_HEIGHT;
    sNdsIFCommonItemArrow.gfx = gfx;
    sNdsIFCommonItemArrow.runtime_red = sprite->red;
    sNdsIFCommonItemArrow.runtime_green = sprite->green;
    sNdsIFCommonItemArrow.runtime_blue = sprite->blue;
    sNdsIFCommonItemArrow.baked = TRUE;
    return TRUE;
}

/* ndsIFCommonReadI8 without a file range to check against: the bitmap comes
 * from a live SObj whose shape the loader already validated, so only the
 * pointer and the TEXSHUF walk remain. */
static s32 ndsIFCommonReadTagI8(const Sprite *sprite, u32 source_x,
                                u32 source_y, u8 *intensity)
{
    const Bitmap *bitmap;
    u32 out_y = 0u;
    u32 bitmap_index;

    if ((sprite == NULL) || (intensity == NULL) ||
        (sprite->bitmap == NULL))
    {
        return FALSE;
    }
    bitmap = sprite->bitmap;
    for (bitmap_index = 0u;
         (bitmap_index < (u32)(u16)sprite->nbitmaps) &&
         (out_y < (u32)(u16)sprite->height);
         bitmap_index++)
    {
        const Bitmap *current = &bitmap[bitmap_index];
        u32 width = (u32)(u16)current->width;
        u32 width_img = (u32)(u16)current->width_img;
        u32 height = (u32)(u16)current->actualHeight;
        u32 advance = (u32)(u16)sprite->bmheight;
        u32 local_y;
        u32 shuffled_x;
        const u8 *pixels;

        if (width == 0u)
        {
            break;
        }
        if (width_img == 0u)
        {
            width_img = width;
        }
        if (height == 0u)
        {
            height = advance;
        }
        if (advance == 0u)
        {
            advance = height;
        }
        if ((source_y < out_y) || (source_y >= (out_y + height)) ||
            (source_x >= width))
        {
            out_y += advance;
            continue;
        }

        local_y = source_y - out_y;
        shuffled_x = source_x ^ ((local_y & 1u) != 0u ? 4u : 0u);
        pixels = (const u8 *)current->buf;
        if (pixels == NULL)
        {
            return FALSE;
        }
        *intensity = pixels[((size_t)local_y * width_img + shuffled_x) ^
                            3u];
        return TRUE;
    }
    return FALSE;
}

s32 ndsIFCommonNativeOamBakePlayerTag(const Sprite *sprite)
{
    u32 slot;
    u32 width;
    u32 height;
    u32 cell[NDS_IFCOMMON_TAG_CELL_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u16 *gfx;
    u32 *dst;
    u32 word;
    u32 y;

    if ((sprite == NULL) || (sprite->bitmap == NULL) ||
        (sprite->bmfmt != G_IM_FMT_IA) ||
        (sprite->bmsiz != G_IM_SIZ_8b) || (sprite->nbitmaps < 1))
    {
        return FALSE;
    }
    width = (u32)(u16)sprite->width;
    height = (u32)(u16)sprite->height;
    if ((width == 0u) || (width > NDS_IFCOMMON_TAG_CELL) ||
        (height == 0u) || (height > NDS_IFCOMMON_TAG_CELL))
    {
        return FALSE;
    }
    for (slot = 0u; slot < NDS_IFCOMMON_TAG_COUNT; slot++)
    {
        if ((sNdsIFCommonPlayerTags[slot].baked != FALSE) &&
            (sNdsIFCommonPlayerTags[slot].bitmap == sprite->bitmap) &&
            (sNdsIFCommonPlayerTags[slot].width == width) &&
            (sNdsIFCommonPlayerTags[slot].height == height))
        {
            return TRUE;
        }
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        return FALSE;
    }
    for (slot = 0u; slot < NDS_IFCOMMON_TAG_COUNT; slot++)
    {
        if (sNdsIFCommonPlayerTags[slot].baked == FALSE)
        {
            break;
        }
    }
    if (slot >= NDS_IFCOMMON_TAG_COUNT)
    {
        return FALSE;
    }
    if (sNdsIFCommonPlayerTagCursorValid == FALSE)
    {
        sNdsIFCommonPlayerTagCursor = NDS_IFCOMMON_TAG_BANK_BASE;
        sNdsIFCommonPlayerTagCursorValid = TRUE;
    }
    if ((sNdsIFCommonPlayerTagCursor + NDS_IFCOMMON_TAG_CELL_BYTES) >
        NDS_IFCOMMON_OBJ_VRAM_BYTES)
    {
        return FALSE;
    }
    memset(cell, 0, sizeof(cell));
    for (y = 0u; y < height; y++)
    {
        u32 x;

        for (x = 0u; x < width; x++)
        {
            u8 ia;
            u8 index;
            u32 tile = ((y >> 3) * (NDS_IFCOMMON_TAG_CELL >> 3)) +
                       (x >> 3);
            u32 offset = (tile * 32u) + ((y & 7u) * 4u) +
                         ((x & 7u) >> 1);

            if (ndsIFCommonReadTagI8(sprite, x, y, &ia) == FALSE)
            {
                return FALSE;
            }
            /* Source IA combine keeps texels with alpha nibble >= 8 and
             * lerps prim over env by the intensity nibble; index 0 stays
             * transparent. Intensity 0 with kept alpha is opaque black,
             * which shares ramp entry 1 (black) with intensity 1. */
            if ((ia & 0x0fu) < 8u)
            {
                continue;
            }
            index = (u8)(ia >> 4);
            if (index == 0u)
            {
                index = 1u;
            }
            if ((x & 1u) == 0u)
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) |
                                          (index & 0x0fu));
            }
            else
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0x0fu) |
                                          (u8)((index & 0x0fu) << 4));
            }
        }
    }
    /* Stack staging to VRAM with explicit 32-bit stores: DMA cannot source
     * from DTCM stack and VRAM rejects 8-bit writes, same as the hit-spark
     * sheet loader above. */
    gfx = (u16 *)((u8 *)SPRITE_GFX + sNdsIFCommonPlayerTagCursor);
    dst = (u32 *)(void *)gfx;
    for (word = 0u; word < (u32)(sizeof(cell) / sizeof(cell[0])); word++)
    {
        dst[word] = cell[word];
    }
    sNdsIFCommonPlayerTags[slot].bitmap = sprite->bitmap;
    sNdsIFCommonPlayerTags[slot].width = width;
    sNdsIFCommonPlayerTags[slot].height = height;
    sNdsIFCommonPlayerTags[slot].gfx = gfx;
    sNdsIFCommonPlayerTags[slot].baked = TRUE;
    sNdsIFCommonPlayerTagCursor += NDS_IFCOMMON_TAG_CELL_BYTES;
    return TRUE;
}

/* x / 15 for x < 2^16 (checked exhaustively): the palette ramps below divide
 * by 15 per channel per entry, and Thumb code has no UMULL, so GCC called
 * __udivsi3 for each (~55 calls a frame between this file and the HUD). */
static inline u32 ndsIFCommonDiv15(u32 x)
{
    return (x * 34953u) >> 19;
}

static void ndsIFCommonPlayerTagPalette(u32 color_id)
{
    u32 red = (u32)dIFCommonPlayerTagPrimColorsR[color_id];
    u32 green = (u32)dIFCommonPlayerTagPrimColorsG[color_id];
    u32 blue = (u32)dIFCommonPlayerTagPrimColorsB[color_id];
    u16 *palette = &SPRITE_PALETTE[
        (NDS_IFCOMMON_TAG_PALETTE_BASE + color_id) * 16u];
    u32 i;

    palette[0] = 0u;
    for (i = 1u; i < 16u; i++)
    {
        palette[i] = (u16)((1u << 15) |
                           ((ndsIFCommonDiv15(red * i + 7u) >> 3) & 31u) |
                           (((ndsIFCommonDiv15(green * i + 7u) >> 3) & 31u)
                            << 5) |
                           (((ndsIFCommonDiv15(blue * i + 7u) >> 3) & 31u)
                            << 10));
    }
    /* Intensity 0 with kept alpha maps to entry 1 in the bake; the source
     * lerps prim over an all-zero env there, so entry 1 is black, not
     * prim/15 (review, 2026-09-06). Intensity-1 texels share it, one 5-bit
     * step below their true value. */
    palette[1] = (u16)(1u << 15);
}

static s32 ndsIFCommonPlayerTagMiss(u32 reason)
{
    gNdsIFCommonNativeOamFrameFallbackCalls++;
    gNdsIFCommonNativeOamLastFallbackReason = reason;
    return FALSE;
}

static s32 ndsIFCommonPlayerTagRecognized(void)
{
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    return TRUE;
}

/* Source ifCommonPlayerTagProcDisplay:1821-1850 replayed for OAM: TopN joint
 * plus camera_zoom_base, the source projection helper, in-bounds clipping,
 * the wait==1 / eye.z>6000 gate, the hide and bossend flags, SObj centering
 * by sprite width/height. TRUE means handled (drawn, or the source draws
 * nothing this frame); FALSE records a native-render failure. */
static s32 ndsIFCommonEmitPlayerTag(struct GObj *gobj)
{
    SObj *sobj = SObjGetStruct(gobj);
    s32 player;
    SCPlayerData *battle_player;
    FTStruct *fp;
    DObj *topn;
    CObj *cobj;
    int32_t pos_q12[3];
    int64_t projected_x;
    int64_t projected_y;
    s32 screen_x;
    s32 screen_y;
    u32 tag;
    u32 color_id;
    u32 slot;

    if ((sobj == NULL) || (sobj->next != NULL) ||
        (sobj->sprite.bitmap == NULL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    /* SP_TRANSPARENT is the source tag mode (SP_TEXSHUF | SP_TRANSPARENT);
     * anything else, a non-opaque alpha, or a scaled sprite keeps the exact
     * generic path, which owns XLU blending and scaling. */
    if (((sobj->sprite.attr & SP_TRANSPARENT) == 0u) ||
        (sobj->sprite.alpha != 255u) ||
        NDS_FCMP_NE_C(sobj->sprite.scalex, 1.0F) ||
        NDS_FCMP_NE_C(sobj->sprite.scaley, 1.0F))
    {
        return ndsIFCommonPlayerTagMiss(
            nNDSIFCommonFallbackRuntimeColor);
    }
    if ((sobj->sprite.bmfmt != G_IM_FMT_IA) ||
        (sobj->sprite.bmsiz != G_IM_SIZ_8b) || (sobj->sprite.nbitmaps < 1) ||
        ((u32)(u16)sobj->sprite.width == 0u) ||
        ((u32)(u16)sobj->sprite.width > NDS_IFCOMMON_TAG_CELL) ||
        ((u32)(u16)sobj->sprite.height == 0u) ||
        ((u32)(u16)sobj->sprite.height > NDS_IFCOMMON_TAG_CELL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    player = ifGetPlayer(gobj);
    if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (gSCManagerBattleState == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    battle_player = &gSCManagerBattleState->players[player];
    if ((battle_player->pkind == nFTPlayerKindNot) ||
        (battle_player->fighter_gobj == NULL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    tag = (u32)battle_player->tag;
    if (tag > (u32)nIFPlayerTagKindHeart)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackUnknownSprite);
    }
    color_id = (u32)battle_player->color;
    if (color_id >= NDS_IFCOMMON_TAG_PRIM_COUNT)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (((u32)sobj->sprite.red !=
         (u32)dIFCommonPlayerTagPrimColorsR[color_id]) ||
        ((u32)sobj->sprite.green !=
         (u32)dIFCommonPlayerTagPrimColorsG[color_id]) ||
        ((u32)sobj->sprite.blue !=
         (u32)dIFCommonPlayerTagPrimColorsB[color_id]) ||
        (sobj->envcolor.r != 0u) || (sobj->envcolor.g != 0u) ||
        (sobj->envcolor.b != 0u))
    {
        return ndsIFCommonPlayerTagMiss(
            nNDSIFCommonFallbackRuntimeColor);
    }
    fp = ftGetStruct(battle_player->fighter_gobj);
    if (fp == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if ((fp->is_playertag_bossend != FALSE) ||
        (fp->is_playertag_hide != FALSE))
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    if (gGMCameraGObj == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    cobj = CObjGetStruct(gGMCameraGObj);
    if (cobj == NULL)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    if ((fp->playertag_wait != 1) && NDS_FCMP_LE(cobj->vec.eye.z, 6000.0F))
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    if ((fp->attr == NULL) ||
        (fp->joints[nFTPartsJointTopN] == NULL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    topn = fp->joints[nFTPartsJointTopN];
    pos_q12[0] = ndsF32ToFixed(topn->translate.vec.f.x, 12u);
    pos_q12[1] = ndsF32ToFixed(topn->translate.vec.f.y, 12u) +
                 ndsF32ToFixed(fp->attr->camera_zoom_base, 12u);
    pos_q12[2] = ndsF32ToFixed(topn->translate.vec.f.z, 12u);
    ndsProjectToViewportQ22(cobj, gGMCameraMatrix, pos_q12,
                            &projected_x, &projected_y);
    if (ndsIFCommonInBoundsQ22(projected_x, projected_y) == FALSE)
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    /* The source's SObj position: the tag's bottom centre on the projected
     * point. The OAM tag is the sprite's own size, so it is centred on the
     * point's DS position rather than scaled with it. */
    screen_x = ndsIFCommonRoundQ22HalfUp(
        ndsIFCommonBattleScreenXQ22(projected_x) -
        ((int64_t)sobj->sprite.width << 21));
    screen_y = ndsIFCommonRoundQ22HalfUp(
        ndsIFCommonBattleScreenYQ22(projected_y) -
        ((int64_t)sobj->sprite.height << 22));
    if ((screen_x <= -32) || (screen_x >= 256) ||
        (screen_y <= -32) || (screen_y >= 192))
    {
        return ndsIFCommonPlayerTagRecognized();
    }
    if (sNdsIFCommonPrepared == FALSE)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackNotPrepared);
    }
    if ((sobj->sprite.bitmap == NULL) ||
        (sobj->sprite.bmfmt != G_IM_FMT_IA) ||
        (sobj->sprite.bmsiz != G_IM_SIZ_8b) || (sobj->sprite.nbitmaps < 1) ||
        ((u32)(u16)sobj->sprite.width > NDS_IFCOMMON_TAG_CELL) ||
        ((u32)(u16)sobj->sprite.height > NDS_IFCOMMON_TAG_CELL))
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (ndsIFCommonNativeOamBakePlayerTag(&sobj->sprite) == FALSE)
    {
        /* Format was checked above, so a bake miss is slot or VRAM
         * exhaustion. */
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackObjectLimit);
    }
    for (slot = 0u; slot < NDS_IFCOMMON_TAG_COUNT; slot++)
    {
        if ((sNdsIFCommonPlayerTags[slot].baked != FALSE) &&
            (sNdsIFCommonPlayerTags[slot].bitmap == sobj->sprite.bitmap))
        {
            break;
        }
    }
    if (slot >= NDS_IFCOMMON_TAG_COUNT)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (sNdsIFCommonNextOamID < 0)
    {
        return ndsIFCommonPlayerTagMiss(nNDSIFCommonFallbackObjectLimit);
    }
    ndsIFCommonPlayerTagPalette(color_id);
    oamSet(&oamMain, sNdsIFCommonNextOamID, screen_x, screen_y, 0,
           (int)(NDS_IFCOMMON_TAG_PALETTE_BASE + color_id),
           SpriteSize_32x32, SpriteColorFormat_16Color,
           sNdsIFCommonPlayerTags[slot].gfx, -1, false, false, false,
           false, false);
    sNdsIFCommonNextOamID--;
    sNdsIFCommonFrameNeedsCommit = TRUE;
    NDS_DIAG(gNdsIFCommonNativeOamFrameRecognizedCalls++);
    NDS_DIAG(gNdsIFCommonNativeOamFrameDrawCalls++);
    gNdsIFCommonNativeOamFrameObjectCount++;
    return TRUE;
}

static s32 ndsIFCommonItemArrowMiss(u32 reason)
{
    gNdsIFCommonNativeOamFrameFallbackCalls++;
    gNdsIFCommonNativeOamLastFallbackReason = reason;
    return FALSE;
}

static s32 ndsIFCommonItemArrowRecognized(void)
{
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    return TRUE;
}

/* ifCommonItemArrowProcDisplay already owns the source visibility test,
 * projection, and SObj position. Reproduce only lbCommonDrawSObjAttr here:
 * one red I4 source sprite at the battle screen's 0.8 presentation scale. */
static s32 ndsIFCommonEmitItemArrow(struct GObj *gobj)
{
    SObj *sobj = SObjGetStruct(gobj);
    u32 scale_q16 = NDS_IFCOMMON_SCREEN_SCALE_Q16;
    u16 inverse;
    s32 matrix_index;
    s32 origin_x;
    s32 origin_y;
    s32 center_x;
    s32 center_y;
    s32 x;
    s32 y;

    if ((sobj == NULL) || (sobj->next != NULL) ||
        (sobj->sprite.bitmap == NULL))
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
    }
    if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
    {
        return ndsIFCommonItemArrowRecognized();
    }
    if (((sobj->sprite.attr & SP_TRANSPARENT) == 0u) ||
        NDS_FCMP_NE_C(sobj->sprite.scalex, 1.0F) ||
        NDS_FCMP_NE_C(sobj->sprite.scaley, 1.0F) ||
        (sobj->sprite.bmfmt != G_IM_FMT_I) ||
        (sobj->sprite.bmsiz != G_IM_SIZ_4b) ||
        (sobj->sprite.nbitmaps != 1) ||
        ((u32)(u16)sobj->sprite.width != NDS_IFCOMMON_ITEM_WIDTH) ||
        ((u32)(u16)sobj->sprite.height != NDS_IFCOMMON_ITEM_HEIGHT))
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
    }
    if (sobj->sprite.alpha == 0u)
    {
        return ndsIFCommonItemArrowRecognized();
    }
    if (ndsIFCommonNativeOamBakeItemArrow(&sobj->sprite) == FALSE)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackNotPrepared);
    }
    if (sNdsIFCommonNextOamID < 0)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
    }

    inverse = (u16)(((1u << 24) + (scale_q16 / 2u)) / scale_q16);
    matrix_index = ndsIFCommonMatrixForScale(inverse);
    if (matrix_index < 0)
    {
        return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackMatrixLimit);
    }
    /* The source put the arrow's bottom centre on the item's projected point
     * (pos = centre + projected - (width / 2, height)); that point takes the
     * battle 3D's mapping (ndsIFCommonBattleScreenX/Y) and the 0.8-scaled
     * arrow hangs from it. */
    origin_x = ndsIFCommonRoundQ22HalfUp(
        ndsIFCommonBattleScreenXQ22(
            ((int64_t)ndsF32ToFixed(sobj->pos.x, 16u) << 6) +
            ((int64_t)NDS_IFCOMMON_ITEM_WIDTH << 21) -
            ((int64_t)gGMCameraStruct.viewport_center_x << 22)) -
        ((((int64_t)NDS_IFCOMMON_ITEM_WIDTH << 22) * 2) / 5));
    origin_y = ndsIFCommonRoundQ22HalfUp(
        ndsIFCommonBattleScreenYQ22(
            ((int64_t)gGMCameraStruct.viewport_center_y << 22) -
            (((int64_t)ndsF32ToFixed(sobj->pos.y, 16u) << 6) +
             ((int64_t)NDS_IFCOMMON_ITEM_HEIGHT << 22))) -
        ((((int64_t)NDS_IFCOMMON_ITEM_HEIGHT << 22) * 4) / 5));
    center_x = origin_x + ndsIFCommonRoundQ16HalfUp(
        (s32)(NDS_IFCOMMON_ITEM_CELL_WIDTH / 2u) * (s32)scale_q16);
    center_y = origin_y + ndsIFCommonRoundQ16HalfUp(
        (s32)(NDS_IFCOMMON_ITEM_CELL_HEIGHT / 2u) * (s32)scale_q16);
    x = center_x - (s32)(NDS_IFCOMMON_ITEM_CELL_WIDTH / 2u);
    y = center_y - (s32)(NDS_IFCOMMON_ITEM_CELL_HEIGHT / 2u);

    oamSet(&oamMain, sNdsIFCommonNextOamID, x, y, 0,
           NDS_IFCOMMON_ITEM_PALETTE,
           SpriteSize_16x8, SpriteColorFormat_16Color,
           sNdsIFCommonItemArrow.gfx, matrix_index,
           false, false, false, false, false);
    sNdsIFCommonNextOamID--;
    sNdsIFCommonFrameNeedsCommit = TRUE;
    gNdsIFCommonNativeOamFrameObjectCount++;
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
#if NDS_SHIP_TELEMETRY
    ndsIFCommonRecordSemantic(sobj, NDS_IFCOMMON_ASSET_COUNT);
#endif
    return TRUE;
}
