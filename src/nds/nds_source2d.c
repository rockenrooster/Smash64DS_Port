/* Native presentation for the source 2D scenes that have no native screen of
 * their own: the 1P intro, stage clear, continue, challenger, message and
 * congratulations scenes (P2-6, 2026-10-01).
 *
 * The software SObj compositor these scenes drew through is host-only under
 * the native-renderer law (src/host/graphics_reference/sprite_reference.c),
 * so on the DS they drew nothing at all: a blue screen and one recorded
 * native failure per sprite per frame (the 1P walk's census, 53 sprites over
 * the first intro and tally: CI4, CI8, RGBA16, RGBA32, I4 and IA8, up to the
 * full 300x220 frame).
 *
 * This is the VS Results tenant's method (nds_results_oam.c) made general,
 * beside it rather than in it so the owner-accepted Results screen cannot
 * move. The source keeps every position, scale, colour and timing; each
 * visible SObj a display callback draws is resampled once to its mapped DS
 * size and presented in source draw order:
 *  - The source frame (10,10)-(310,230) maps onto the 256x192 screen with the
 *    Results scale (256/300, 192/220).
 *  - The first sprite of a frame, when it covers a third of the screen or
 *    more, is the scene's background: it becomes the BG2 bitmap (behind every
 *    OBJ), written once while it stays the same image, size and place.
 *  - Every other sprite becomes OBJ cells in bank E, placed so that a later
 *    sprite covers an earlier one (it takes a lower OAM index). A sprite is
 *    cut into rows of 64/32/16/8 lines, each row at most four cells wide.
 *  - CI4 keeps its own colours as a 16-colour bank (transparent entries map to
 *    index 0); CI8 takes 256-colour cells over its own run of palette entries
 *    (allocated downward from 255), or direct colour when the run does not
 *    fit; RGBA16/RGBA32 are direct-colour bitmap OBJs carrying the sprite's
 *    alpha; I and IA take the source's prim/env ramp as their bank (the
 *    Results rule).
 * Cells are cached by image, size and cut; when bank E, the cell table or the
 * palette is full the cache is wiped at the next frame's start and everything
 * visible is baked again. Anything still not presentable records a native
 * sprite failure, never a silent empty draw. */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <PR/sp.h>
#include <sys/vector.h>

#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_source2d.h>
#include <sc/scene.h>
#include <sys/obj.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

/* The source frame each tenant maps onto the 256x192 screen. Every tenant
 * but one draws in the 320x240 frame and shows (10,10)-(310,230); the staff
 * roll runs at 640x480 (scstaffroll.c's viewports are (20,20)-(620,460)) and
 * its sprites sit in that frame, so they map with half the scale. */
static s32 sNdsS2DSrcOriginX = 10;
static s32 sNdsS2DSrcOriginY = 10;
static u32 sNdsS2DScaleXQ16 = 55924u;
static u32 sNdsS2DScaleYQ16 = 57195u;
#define NDS_S2D_SRC_ORIGIN_X sNdsS2DSrcOriginX
#define NDS_S2D_SRC_ORIGIN_Y sNdsS2DSrcOriginY
#define NDS_S2D_SCALE_X_Q16 sNdsS2DScaleXQ16
#define NDS_S2D_SCALE_Y_Q16 sNdsS2DScaleYQ16
#define NDS_S2D_ALIGN 128u
#define NDS_S2D_VRAM_BYTES (64u * 1024u)
#define NDS_S2D_CELLS 96u
#define NDS_S2D_BANKS 16u
#define NDS_S2D_CI8_TABLES 4u
#define NDS_S2D_MAX_COLS 4u
#define NDS_S2D_PLAN_MEMO 32u
/* A frame's first sprite this large (a third of the screen) is the BG2
 * background. */
#define NDS_S2D_BACKGROUND_AREA (256u * 192u / 3u)

#define NDS_S2D_ENC_NONE 0u
#define NDS_S2D_ENC_RAMP 1u
#define NDS_S2D_ENC_LUT4 2u
#define NDS_S2D_ENC_LUT8 3u
#define NDS_S2D_ENC_BMP 4u

#define NDS_S2D_BANK_RAMP 0u
#define NDS_S2D_BANK_LUT 1u

typedef struct NDSSource2DShape
{
    u8 width;
    u8 height;
    SpriteSize size;
} NDSSource2DShape;

typedef struct NDSSource2DRowPlan
{
    u8 count;
    NDSSource2DShape shape[NDS_S2D_MAX_COLS];
    u16 x[NDS_S2D_MAX_COLS];
} NDSSource2DRowPlan;

typedef struct NDSSource2DSource
{
    const Sprite *sprite;
    const void *file_data;
    size_t file_size;
    const void *lut_data;
    size_t lut_size;
    u32 asset_id;
    u32 bitmap_offset;
    u32 lut_asset_id;
    u32 lut_offset;
} NDSSource2DSource;

typedef struct NDSSource2DCell
{
    u32 asset_id;
    u32 bitmap_offset;
    u32 lut_asset_id;
    u32 lut_offset;
    u16 source_width;
    u16 source_height;
    u16 final_width;
    u16 final_height;
    u16 tile_x;
    u16 tile_y;
    u8 cell_width;
    u8 cell_height;
    u8 bmfmt;
    u8 bmsiz;
    u8 encoding;
    u8 color_format;
    SpriteSize size;
    /* Direct-colour I/IA cells bake the prim/env colours in. */
    u32 tint_prim;
    u32 tint_env;
    u32 modulate;
    u16 *gfx;
} NDSSource2DCell;

typedef struct NDSSource2DBank
{
    u8 kind;
    u8 prim_r;
    u8 prim_g;
    u8 prim_b;
    u8 env_r;
    u8 env_g;
    u8 env_b;
    u8 remap[16];
    u8 opaque;
    u32 modulate;
    u32 lut_asset_id;
    u32 lut_offset;
    u16 colors[16];
} NDSSource2DBank;

/* A CI8 image's run of palette entries: its used opaque LUT entries map onto
 * absolute OBJ palette entries (0 = transparent). */
typedef struct NDSSource2DCI8
{
    u32 lut_asset_id;
    u32 lut_offset;
    u32 image_asset_id;
    u32 image_offset;
    u32 modulate;
    u8 remap[256];
} NDSSource2DCI8;

static const NDSSource2DShape sNdsS2DShapes[] =
{
    /* Wide-first order is the tie-break after fewest cells and least area. */
    { 64u, 32u, SpriteSize_64x32 },
    { 64u, 64u, SpriteSize_64x64 },
    { 32u, 8u, SpriteSize_32x8 },
    { 32u, 16u, SpriteSize_32x16 },
    { 32u, 32u, SpriteSize_32x32 },
    { 32u, 64u, SpriteSize_32x64 },
    { 16u, 8u, SpriteSize_16x8 },
    { 16u, 16u, SpriteSize_16x16 },
    { 16u, 32u, SpriteSize_16x32 },
    { 8u, 8u, SpriteSize_8x8 },
    { 8u, 16u, SpriteSize_8x16 },
    { 8u, 32u, SpriteSize_8x32 }
};

typedef struct NDSSource2DPlanMemo
{
    u32 key;
    NDSSource2DRowPlan plan;
} NDSSource2DPlanMemo;

static NDSSource2DPlanMemo sNdsS2DPlanMemo[NDS_S2D_PLAN_MEMO];
static u32 sNdsS2DPlanMemoNext;
static NDSSource2DCell sNdsS2DCells[NDS_S2D_CELLS];
static NDSSource2DBank sNdsS2DBanks[NDS_S2D_BANKS];
static NDSSource2DCI8 sNdsS2DCI8[NDS_S2D_CI8_TABLES];
static u16 sNdsS2DHighColors[256];
static u32 sNdsS2DScratch[(64u * 64u) / sizeof(u32)];
static u32 sNdsS2DCellCount;
static u32 sNdsS2DBankCount;
static u32 sNdsS2DCI8Count;
static u32 sNdsS2DHighFloor = 256u;
static u32 sNdsS2DVramCursor;
static s32 sNdsS2DNextOamId = 127;
static u32 sNdsS2DFrameSprites;
static u32 sNdsS2DFrameNeedsCommit;
static u32 sNdsS2DPaletteDirty;
static u32 sNdsS2DActive;
static u32 sNdsS2DEnterPending;
static u32 sNdsS2DResetPending;
/* The BG2 background: the large pictures a frame draws before its first OBJ
 * sprite (the 1P intro's sky, then Board the Platforms' picture), composited
 * in draw order. Keys of the layers it holds, bottom first; count 0 = none. */
#define NDS_S2D_BACKGROUND_LAYERS 4u
static u32 sNdsS2DBackgroundKey[NDS_S2D_BACKGROUND_LAYERS][7];
static u32 sNdsS2DBackgroundCount;
static u32 sNdsS2DFrameBackgrounds;
static u32 sNdsS2DFrameObjects;
/* The RGB a no-attribute draw multiplies RGBA/CI texels by: the prim colour
 * its display callback set under a MODULATE combine (0xffffff = none). */
static u32 sNdsS2DModulate = 0xffffffu;

volatile u32 gNdsSource2DEnterCount;
volatile u32 gNdsSource2DExitCount;
volatile u32 gNdsSource2DDrawSObjCount;
volatile u32 gNdsSource2DBakeCellCount;
volatile u32 gNdsSource2DEmitCount;
volatile u32 gNdsSource2DBackgroundWrites;
volatile u32 gNdsSource2DCacheResetCount;
volatile u32 gNdsSource2DVramBytes;
volatile u32 gNdsSource2DVramHigh;
volatile u32 gNdsSource2DFailFormat;
volatile u32 gNdsSource2DFailTile;
volatile u32 gNdsSource2DFailProvenance;
volatile u32 gNdsSource2DFailCells;
volatile u32 gNdsSource2DFailVram;
volatile u32 gNdsSource2DFailBanks;
volatile u32 gNdsSource2DFailOam;
volatile u32 gNdsSource2DFailBackground;

#if NDS_RENDERER_HW_TRIANGLES

static void ndsS2DRecordFailure(const SObj *sobj, u32 reason)
{
    u32 status = 0u;
    u32 root = 0u;

    if (sobj != NULL)
    {
        status = (((u32)sobj->sprite.bmfmt << 16) | (u32)sobj->sprite.bmsiz);
        root = (u32)(uintptr_t)sobj->sprite.bitmap;
    }
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_SPRITE,
                                   (u32)gSCManagerSceneData.scene_curr,
                                   0xffffffffu, status, root, 0u, reason);
}

static s32 ndsS2DRangeValid(const void *base, size_t size, const void *ptr,
                            size_t bytes)
{
    uintptr_t start;
    uintptr_t end;
    uintptr_t range_start;
    uintptr_t range_end;

    if ((base == NULL) || (ptr == NULL))
    {
        return 0;
    }
    start = (uintptr_t)base;
    end = start + size;
    range_start = (uintptr_t)ptr;
    range_end = range_start + bytes;
    if ((end < start) || (range_end < range_start))
    {
        return 0;
    }
    return ((range_start >= start) && (range_end <= end)) ? 1 : 0;
}

static u16 ndsS2DRgb15(u32 red, u32 green, u32 blue)
{
    return (u16)(0x8000u | (u16)(red >> 3) | ((u16)(green >> 3) << 5) |
                 ((u16)(blue >> 3) << 10));
}

static u32 ndsS2DRgba16To8888(u16 color)
{
    u32 red5 = (color >> 11) & 31u;
    u32 green5 = (color >> 6) & 31u;
    u32 blue5 = (color >> 1) & 31u;
    u32 red = (red5 << 3) | (red5 >> 2);
    u32 green = (green5 << 3) | (green5 >> 2);
    u32 blue = (blue5 << 3) | (blue5 >> 2);
    u32 alpha = ((color & 1u) != 0u) ? 255u : 0u;

    return (red << 24) | (green << 16) | (blue << 8) | alpha;
}

static u32 ndsS2DModulateRgba(u32 rgba);

static u16 ndsS2DRgba16ToRgb15(u16 color)
{
    u32 rgba = ndsS2DModulateRgba(ndsS2DRgba16To8888(color));

    return ndsS2DRgb15((rgba >> 24) & 0xffu, (rgba >> 16) & 0xffu,
                       (rgba >> 8) & 0xffu);
}

/* ---- cache and VRAM ---- */

static void ndsS2DResetCache(void)
{
    sNdsS2DCellCount = 0u;
    sNdsS2DBankCount = 0u;
    sNdsS2DCI8Count = 0u;
    sNdsS2DHighFloor = 256u;
    sNdsS2DVramCursor = 0u;
    sNdsS2DPaletteDirty = 1u;
    gNdsSource2DVramBytes = 0u;
}

static u16 *ndsS2DAllocObjBytes(u32 bytes)
{
    u32 cursor = (sNdsS2DVramCursor + (NDS_S2D_ALIGN - 1u)) &
                 ~(NDS_S2D_ALIGN - 1u);
    u16 *gfx;

    if ((bytes > NDS_S2D_VRAM_BYTES) ||
        (cursor > (NDS_S2D_VRAM_BYTES - bytes)))
    {
        gNdsSource2DFailVram++;
        sNdsS2DResetPending = 1u;
        return NULL;
    }
    gfx = (u16 *)((u8 *)SPRITE_GFX + cursor);
    sNdsS2DVramCursor = cursor + bytes;
    gNdsSource2DVramBytes = sNdsS2DVramCursor;
    if (sNdsS2DVramCursor > gNdsSource2DVramHigh)
    {
        gNdsSource2DVramHigh = sNdsS2DVramCursor;
    }
    return gfx;
}

/* ---- source texels ---- */

static s32 ndsS2DResolveSource(const Sprite *sprite, NDSSource2DSource *source)
{
    const void *file_data;
    u32 file_size;
    u32 asset_id;
    u32 bitmap_offset;

    if ((sprite->bitmap == NULL) || (sprite->nbitmaps <= 0) ||
        (ndsRelocGetLoadedPointerProvenance(sprite->bitmap, &asset_id,
                                            &bitmap_offset) == 0) ||
        (ndsRelocGetLoadedAssetView(asset_id, &file_data, &file_size) == 0) ||
        (ndsS2DRangeValid(file_data, file_size, sprite->bitmap,
                          (size_t)(u16)sprite->nbitmaps * sizeof(Bitmap)) == 0))
    {
        gNdsSource2DFailProvenance++;
        return 0;
    }
    memset(source, 0, sizeof(*source));
    source->sprite = sprite;
    source->file_data = file_data;
    source->file_size = (size_t)file_size;
    source->asset_id = asset_id;
    source->bitmap_offset = bitmap_offset;
    source->lut_asset_id = 0xffffffffu;
    source->lut_offset = 0xffffffffu;
    if (sprite->bmfmt == G_IM_FMT_CI)
    {
        u32 entries = (sprite->bmsiz == G_IM_SIZ_4b) ? 16u : 256u;
        u32 lut_asset_id;
        u32 lut_offset;
        const void *lut_data;
        u32 lut_size;

        if ((sprite->LUT == NULL) ||
            (ndsRelocGetLoadedPointerProvenance(sprite->LUT, &lut_asset_id,
                                                &lut_offset) == 0) ||
            (ndsRelocGetLoadedAssetView(lut_asset_id, &lut_data,
                                        &lut_size) == 0) ||
            (ndsS2DRangeValid(lut_data, lut_size, sprite->LUT,
                              entries * sizeof(u16)) == 0))
        {
            gNdsSource2DFailProvenance++;
            return 0;
        }
        source->lut_data = lut_data;
        source->lut_size = (size_t)lut_size;
        source->lut_asset_id = lut_asset_id;
        source->lut_offset = lut_offset;
    }
    return 1;
}

static s32 ndsS2DFindBitmap(const NDSSource2DSource *source, u32 source_x,
                            u32 source_y, const Bitmap **out_bitmap,
                            u32 *out_local_y, u32 *out_width_img,
                            u32 *out_height)
{
    const Sprite *sprite = source->sprite;
    u32 count = (u32)(u16)sprite->nbitmaps;
    u32 advance = (u32)(u16)sprite->bmheight;
    u32 out_y = 0u;
    u32 index;

    /* Strips advance by bmheight: try the strip that position names first. */
    if (advance != 0u)
    {
        index = source_y / advance;
        if (index < count)
        {
            const Bitmap *current = &sprite->bitmap[index];
            u32 width = (u32)(u16)current->width;
            u32 height = (u32)(u16)current->actualHeight;
            u32 local_y = source_y - (index * advance);

            if (height == 0u) height = advance;
            if ((width != 0u) && (local_y < height) && (source_x < width))
            {
                u32 width_img = (u32)(u16)current->width_img;

                *out_bitmap = current;
                *out_local_y = local_y;
                *out_width_img = (width_img != 0u) ? width_img : width;
                *out_height = height;
                return 1;
            }
        }
    }
    for (index = 0u; (index < count) && (out_y < (u32)(u16)sprite->height);
         index++)
    {
        const Bitmap *current = &sprite->bitmap[index];
        u32 width = (u32)(u16)current->width;
        u32 width_img = (u32)(u16)current->width_img;
        u32 height = (u32)(u16)current->actualHeight;
        u32 step = advance;

        if (width == 0u)
        {
            break;
        }
        if (width_img == 0u) width_img = width;
        if (height == 0u) height = step;
        if (step == 0u) step = height;
        if ((source_y >= out_y) && (source_y < (out_y + height)) &&
            (source_x < width))
        {
            *out_bitmap = current;
            *out_local_y = source_y - out_y;
            *out_width_img = width_img;
            *out_height = height;
            return 1;
        }
        out_y += step;
    }
    return 0;
}

/* One stored texel, undoing the loader's word swap and the odd-row swizzle
 * (the host reference's rules, sprite_reference.c:2124-2252).
 *
 * The swizzle is undone on every odd row whatever the SObj's attr says:
 * lbCommonDrawSObjBitmap loads every size with a dxt-0 gDPLoadBlock
 * (lbcommon.c:2316, :2364, :2412, :2460), which never swaps a line, so each
 * SObj bitmap is stored with its odd rows pre-swapped (every staged Sprite
 * record carries SP_TEXSHUF). SP_TEXSHUF itself is only read by libultra's
 * spDraw. The staff roll's SObjs reset attr to a bare SP_TRANSPARENT
 * (scstaffroll.c:1070, :1189, :1921, :1950) and still draw unswizzled on
 * the N64; keyed on the flag, its crosshair, brackets and box text drew with
 * every odd row eight texels out. */
static s32 ndsS2DReadRaw(const NDSSource2DSource *source, u32 source_x,
                         u32 source_y, u32 *out_value)
{
    const Sprite *sprite = source->sprite;
    const Bitmap *bitmap;
    u32 local_y;
    u32 width_img;
    u32 height;
    u32 x = source_x;
    u32 shuffle;
    const u8 *buf;

    if (ndsS2DFindBitmap(source, source_x, source_y, &bitmap, &local_y,
                         &width_img, &height) == 0)
    {
        return 0;
    }
    buf = (const u8 *)bitmap->buf;
    shuffle = ((local_y & 1u) != 0u) ? 1u : 0u;
    switch (sprite->bmsiz)
    {
    case G_IM_SIZ_4b:
    {
        u32 row_bytes = (width_img + 1u) / 2u;
        u8 packed;

        if ((shuffle != 0u) && ((x ^ 8u) < width_img)) x ^= 8u;
        if (ndsS2DRangeValid(source->file_data, source->file_size, buf,
                             (size_t)row_bytes * height) == 0)
        {
            return 0;
        }
        packed = buf[(((size_t)local_y * row_bytes) + (x >> 1)) ^ 3u];
        *out_value = ((x & 1u) == 0u) ? (u32)(packed >> 4) :
                                        (u32)(packed & 0x0fu);
        return 1;
    }
    case G_IM_SIZ_8b:
        if ((shuffle != 0u) && ((x ^ 4u) < width_img)) x ^= 4u;
        if (ndsS2DRangeValid(source->file_data, source->file_size, buf,
                             (size_t)width_img * height) == 0)
        {
            return 0;
        }
        *out_value = buf[(((size_t)local_y * width_img) + x) ^ 3u];
        return 1;
    case G_IM_SIZ_16b:
        if ((shuffle != 0u) && ((x ^ 2u) < width_img)) x ^= 2u;
        if (ndsS2DRangeValid(source->file_data, source->file_size, buf,
                             (size_t)width_img * height * sizeof(u16)) == 0)
        {
            return 0;
        }
        *out_value = ((const u16 *)(const void *)buf)[
            (((size_t)local_y * width_img) + x) ^ 1u];
        return 1;
    case G_IM_SIZ_32b:
        if ((shuffle != 0u) && ((x ^ 2u) < width_img)) x ^= 2u;
        if (ndsS2DRangeValid(source->file_data, source->file_size, buf,
                             (size_t)width_img * height * sizeof(u32)) == 0)
        {
            return 0;
        }
        memcpy(out_value,
               buf + ((((size_t)local_y * width_img) + x) * sizeof(u32)),
               sizeof(u32));
        return 1;
    default:
        return 0;
    }
}

static u32 ndsS2DModulateRgba(u32 rgba)
{
    u32 m = sNdsS2DModulate;
    u32 r;
    u32 g;
    u32 b;

    if (m == 0xffffffu)
    {
        return rgba;
    }
    r = ((((rgba >> 24) & 0xffu) * ((m >> 16) & 0xffu)) + 127u) / 255u;
    g = ((((rgba >> 16) & 0xffu) * ((m >> 8) & 0xffu)) + 127u) / 255u;
    b = ((((rgba >> 8) & 0xffu) * (m & 0xffu)) + 127u) / 255u;
    return (r << 24) | (g << 16) | (b << 8) | (rgba & 0xffu);
}

/* A sprite drawn without SP_TRANSPARENT or SP_CUTOUT is opaque on the N64:
 * the sprite microcode's opaque render mode ignores texel alpha. */
static u32 ndsS2DSpriteOpaque(const Sprite *sprite)
{
    return ((sprite->attr & (SP_TRANSPARENT | SP_CUTOUT)) == 0u) ? 1u : 0u;
}

static u16 ndsS2DLutColor(const NDSSource2DSource *source, u32 index)
{
    return ((const u16 *)source->sprite->LUT)[index ^ 1u];
}

/* A texel as 0xRRGGBBAA. I and IA follow the Results convention: I is white
 * with the intensity as coverage, IA carries both. */
static s32 ndsS2DConvertRGBA(const NDSSource2DSource *source, u32 raw,
                             u32 *out_rgba)
{
    const Sprite *sprite = source->sprite;
    u32 intensity;
    u32 alpha;

    switch (sprite->bmfmt)
    {
    case G_IM_FMT_I:
        alpha = (sprite->bmsiz == G_IM_SIZ_4b) ? (raw * 17u) : (raw & 0xffu);
        *out_rgba = 0xffffff00u | alpha;
        return 1;
    case G_IM_FMT_IA:
        if (sprite->bmsiz == G_IM_SIZ_4b)
        {
            intensity = ((raw >> 1) * 255u + 3u) / 7u;
            alpha = ((raw & 1u) != 0u) ? 255u : 0u;
        }
        else if (sprite->bmsiz == G_IM_SIZ_8b)
        {
            intensity = (raw >> 4) * 17u;
            alpha = (raw & 0x0fu) * 17u;
        }
        else
        {
            intensity = (raw >> 8) & 0xffu;
            alpha = raw & 0xffu;
        }
        if (ndsS2DSpriteOpaque(sprite) != 0u) alpha = 255u;
        *out_rgba = (intensity << 24) | (intensity << 16) | (intensity << 8) |
                    alpha;
        return 1;
    case G_IM_FMT_CI:
        *out_rgba = ndsS2DModulateRgba(
            ndsS2DRgba16To8888(ndsS2DLutColor(source, raw)));
        if (ndsS2DSpriteOpaque(sprite) != 0u) *out_rgba |= 0xffu;
        return 1;
    case G_IM_FMT_RGBA:
        *out_rgba = ndsS2DModulateRgba((sprite->bmsiz == G_IM_SIZ_32b) ? raw :
            ndsS2DRgba16To8888((u16)raw));
        if (ndsS2DSpriteOpaque(sprite) != 0u) *out_rgba |= 0xffu;
        return 1;
    default:
        return 0;
    }
}

static s32 ndsS2DReadRGBA(const NDSSource2DSource *source, u32 x, u32 y,
                          u32 *out_rgba)
{
    u32 raw;

    if (ndsS2DReadRaw(source, x, y, &raw) == 0)
    {
        return 0;
    }
    return ndsS2DConvertRGBA(source, raw, out_rgba);
}

static void ndsS2DBilerp(const u32 taps[4], u32 fx, u32 fy, u8 rgba[4])
{
    u32 weights[4];
    u32 weighted_alpha = 0u;
    u32 tap;
    u32 channel;

    weights[0] = (256u - fx) * (256u - fy);
    weights[1] = fx * (256u - fy);
    weights[2] = (256u - fx) * fy;
    weights[3] = fx * fy;
    for (tap = 0u; tap < 4u; tap++)
    {
        weighted_alpha += (taps[tap] & 0xffu) * weights[tap];
    }
    rgba[3] = (u8)((weighted_alpha + 0x8000u) >> 16);
    if (weighted_alpha == 0u)
    {
        rgba[0] = rgba[1] = rgba[2] = 0u;
        return;
    }
    for (channel = 0u; channel < 3u; channel++)
    {
        u32 shift = 24u - (channel * 8u);
        u32 weighted = 0u;

        for (tap = 0u; tap < 4u; tap++)
        {
            weighted += ((taps[tap] >> shift) & 0xffu) * (taps[tap] & 0xffu) *
                        weights[tap];
        }
        rgba[channel] = (u8)((weighted + (weighted_alpha >> 1)) /
                             weighted_alpha);
    }
}

static s32 ndsS2DSampleFiltered(const NDSSource2DSource *source,
                                u32 step_x_q16, u32 step_y_q16,
                                u32 destination_x, u32 destination_y,
                                u8 rgba[4])
{
    u32 source_width = (u32)(u16)source->sprite->width;
    u32 source_height = (u32)(u16)source->sprite->height;
    s32 sx_q16 = (s32)(step_x_q16 >> 1) - 0x8000 +
                 (s32)(destination_x * step_x_q16);
    s32 sy_q16 = (s32)(step_y_q16 >> 1) - 0x8000 +
                 (s32)(destination_y * step_y_q16);
    u32 sx_q8;
    u32 sy_q8;
    u32 x0;
    u32 y0;
    u32 x1;
    u32 y1;
    u32 taps[4];

    if (sx_q16 < 0) sx_q16 = 0;
    if (sy_q16 < 0) sy_q16 = 0;
    sx_q8 = (u32)sx_q16 >> 8;
    sy_q8 = (u32)sy_q16 >> 8;
    x0 = sx_q8 >> 8;
    y0 = sy_q8 >> 8;
    if (x0 >= source_width) x0 = source_width - 1u;
    if (y0 >= source_height) y0 = source_height - 1u;
    x1 = ((x0 + 1u) < source_width) ? (x0 + 1u) : x0;
    y1 = ((y0 + 1u) < source_height) ? (y0 + 1u) : y0;
    if ((ndsS2DReadRGBA(source, x0, y0, &taps[0]) == 0) ||
        (ndsS2DReadRGBA(source, x1, y0, &taps[1]) == 0) ||
        (ndsS2DReadRGBA(source, x0, y1, &taps[2]) == 0) ||
        (ndsS2DReadRGBA(source, x1, y1, &taps[3]) == 0))
    {
        return 0;
    }
    ndsS2DBilerp(taps, sx_q8 & 0xffu, sy_q8 & 0xffu, rgba);
    return 1;
}

/* Nearest source texel for an indexed bake: the texel centred under the
 * destination pixel. */
static s32 ndsS2DSampleNearestRaw(const NDSSource2DSource *source,
                                  u32 step_x_q16, u32 step_y_q16,
                                  u32 destination_x, u32 destination_y,
                                  u32 *out_raw)
{
    u32 sx = ((destination_x * step_x_q16) + (step_x_q16 >> 1)) >> 16;
    u32 sy = ((destination_y * step_y_q16) + (step_y_q16 >> 1)) >> 16;

    if (sx >= (u32)(u16)source->sprite->width)
    {
        sx = (u32)(u16)source->sprite->width - 1u;
    }
    if (sy >= (u32)(u16)source->sprite->height)
    {
        sy = (u32)(u16)source->sprite->height - 1u;
    }
    return ndsS2DReadRaw(source, sx, sy, out_raw);
}

/* ---- the row sampler (P2-6, 2026-10-02) ----
 *
 * The 1P intro's first draw baked its sprites at ~900 cycles a pixel
 * (profile 1p-intro01-st1: 90M cycles, ~80 VBlanks of black, with the scene
 * already started): each of a filtered pixel's four taps found its strip again
 * (a divide), re-validated the strip's range and switched on the format, and
 * the blend divided three times. The bake now finds each destination row's
 * source strips once and reads the taps straight from them. A tap at or past
 * the row's strip width takes the per-texel reader, so failures and unusual
 * strip layouts are exactly the old path's (ndsS2DFindBitmap returns the same
 * strip for every x below its width). When all four taps are opaque the
 * blend's divides reduce exactly to a shift: the alpha factor cancels,
 * (255 S + 255 * 2^15) / (255 * 2^16) = floor((S + 2^15) / 2^16).
 * gNdsS2DRowSampler 0 is the per-pixel path (same-ROM A/B); 1 with
 * gNdsS2DRowVerify 1 runs both and counts differing pixels. */
volatile u32 gNdsS2DRowSampler __attribute__((used, section(".data"))) = 1u;
volatile u32 gNdsS2DRowVerify __attribute__((used, section(".data")));
volatile u32 gNdsS2DRowVerifyPixels;
volatile u32 gNdsS2DRowVerifyMismatches;

typedef struct NDSSource2DSrcRow
{
    const u8 *buf;      /* NULL: every tap takes the per-texel reader */
    u32 width;          /* the strip's width: taps at or past it fall back */
    u32 width_img;
    u32 local_y;
    u32 shuffle;
    u32 y;              /* the source row */
} NDSSource2DSrcRow;

static void ndsS2DPrepareRow(const NDSSource2DSource *source, u32 source_y,
                             NDSSource2DSrcRow *row)
{
    const Sprite *sprite = source->sprite;
    const Bitmap *bitmap;
    u32 local_y;
    u32 width_img;
    u32 height;
    size_t bytes;

    row->buf = NULL;
    row->width = 0u;
    row->y = source_y;
    if (ndsS2DFindBitmap(source, 0u, source_y, &bitmap, &local_y, &width_img,
                         &height) == 0)
    {
        return;
    }
    switch (sprite->bmsiz)
    {
    case G_IM_SIZ_4b:
        bytes = (size_t)((width_img + 1u) / 2u) * height;
        break;
    case G_IM_SIZ_8b:
        bytes = (size_t)width_img * height;
        break;
    case G_IM_SIZ_16b:
        bytes = (size_t)width_img * height * sizeof(u16);
        break;
    case G_IM_SIZ_32b:
        bytes = (size_t)width_img * height * sizeof(u32);
        break;
    default:
        return;
    }
    if (ndsS2DRangeValid(source->file_data, source->file_size, bitmap->buf,
                         bytes) == 0)
    {
        return;
    }
    row->buf = (const u8 *)bitmap->buf;
    row->width = (u32)(u16)bitmap->width;
    row->width_img = width_img;
    row->local_y = local_y;
    /* Every odd row: see ndsS2DReadRaw. */
    row->shuffle = ((local_y & 1u) != 0u) ? 1u : 0u;
}

/* ndsS2DReadRaw's texel for x below the row's strip width. */
static inline u32 ndsS2DRowRaw(u32 bmsiz, const NDSSource2DSrcRow *row, u32 x)
{
    switch (bmsiz)
    {
    case G_IM_SIZ_4b:
    {
        u32 row_bytes = (row->width_img + 1u) / 2u;
        u8 packed;

        if ((row->shuffle != 0u) && ((x ^ 8u) < row->width_img)) x ^= 8u;
        packed = row->buf[(((size_t)row->local_y * row_bytes) + (x >> 1)) ^ 3u];
        return ((x & 1u) == 0u) ? (u32)(packed >> 4) : (u32)(packed & 0x0fu);
    }
    case G_IM_SIZ_8b:
        if ((row->shuffle != 0u) && ((x ^ 4u) < row->width_img)) x ^= 4u;
        return row->buf[(((size_t)row->local_y * row->width_img) + x) ^ 3u];
    case G_IM_SIZ_16b:
        if ((row->shuffle != 0u) && ((x ^ 2u) < row->width_img)) x ^= 2u;
        return ((const u16 *)(const void *)row->buf)[
            (((size_t)row->local_y * row->width_img) + x) ^ 1u];
    default:
    {
        u32 value;

        if ((row->shuffle != 0u) && ((x ^ 2u) < row->width_img)) x ^= 2u;
        memcpy(&value, row->buf + ((((size_t)row->local_y * row->width_img) +
                                    x) * sizeof(u32)), sizeof(u32));
        return value;
    }
    }
}

static inline s32 ndsS2DRowTapRaw(const NDSSource2DSource *source,
                                  const NDSSource2DSrcRow *row, u32 x,
                                  u32 *out_raw)
{
    if ((row->buf != NULL) && (x < row->width))
    {
        *out_raw = ndsS2DRowRaw(source->sprite->bmsiz, row, x);
        return 1;
    }
    return ndsS2DReadRaw(source, x, row->y, out_raw);
}

static inline s32 ndsS2DRowTapRGBA(const NDSSource2DSource *source,
                                   const NDSSource2DSrcRow *row, u32 x,
                                   u32 *out_rgba)
{
    u32 raw;

    if (ndsS2DRowTapRaw(source, row, x, &raw) == 0)
    {
        return 0;
    }
    return ndsS2DConvertRGBA(source, raw, out_rgba);
}

/* The filtered sample's vertical half for one destination row: the rows its
 * two taps read and the fraction between them (ndsS2DSampleFiltered). */
typedef struct NDSSource2DRowPair
{
    NDSSource2DSrcRow row0;
    NDSSource2DSrcRow row1;
    u32 fy;
} NDSSource2DRowPair;

static void ndsS2DPrepareFilteredRows(const NDSSource2DSource *source,
                                      u32 step_y_q16, u32 destination_y,
                                      NDSSource2DRowPair *pair)
{
    u32 source_height = (u32)(u16)source->sprite->height;
    s32 sy_q16 = (s32)(step_y_q16 >> 1) - 0x8000 +
                 (s32)(destination_y * step_y_q16);
    u32 sy_q8;
    u32 y0;
    u32 y1;

    if (sy_q16 < 0) sy_q16 = 0;
    sy_q8 = (u32)sy_q16 >> 8;
    y0 = sy_q8 >> 8;
    if (y0 >= source_height) y0 = source_height - 1u;
    y1 = ((y0 + 1u) < source_height) ? (y0 + 1u) : y0;
    pair->fy = sy_q8 & 0xffu;
    ndsS2DPrepareRow(source, y0, &pair->row0);
    if (y1 == y0)
    {
        pair->row1 = pair->row0;
    }
    else
    {
        ndsS2DPrepareRow(source, y1, &pair->row1);
    }
}

static s32 ndsS2DSampleFilteredRows(const NDSSource2DSource *source,
                                    const NDSSource2DRowPair *pair,
                                    u32 step_x_q16, u32 destination_x,
                                    u8 rgba[4])
{
    u32 source_width = (u32)(u16)source->sprite->width;
    s32 sx_q16 = (s32)(step_x_q16 >> 1) - 0x8000 +
                 (s32)(destination_x * step_x_q16);
    u32 sx_q8;
    u32 x0;
    u32 x1;
    u32 taps[4];

    if (sx_q16 < 0) sx_q16 = 0;
    sx_q8 = (u32)sx_q16 >> 8;
    x0 = sx_q8 >> 8;
    if (x0 >= source_width) x0 = source_width - 1u;
    x1 = ((x0 + 1u) < source_width) ? (x0 + 1u) : x0;
    if ((ndsS2DRowTapRGBA(source, &pair->row0, x0, &taps[0]) == 0) ||
        (ndsS2DRowTapRGBA(source, &pair->row0, x1, &taps[1]) == 0) ||
        (ndsS2DRowTapRGBA(source, &pair->row1, x0, &taps[2]) == 0) ||
        (ndsS2DRowTapRGBA(source, &pair->row1, x1, &taps[3]) == 0))
    {
        return 0;
    }
    if ((taps[0] & taps[1] & taps[2] & taps[3] & 0xffu) == 0xffu)
    {
        const u32 fx = sx_q8 & 0xffu;
        const u32 fy = pair->fy;
        const u32 w0 = (256u - fx) * (256u - fy);
        const u32 w1 = fx * (256u - fy);
        const u32 w2 = (256u - fx) * fy;
        const u32 w3 = fx * fy;
        u32 channel;

        rgba[3] = 255u;
        for (channel = 0u; channel < 3u; channel++)
        {
            const u32 shift = 24u - (channel * 8u);
            const u32 s = (((taps[0] >> shift) & 0xffu) * w0) +
                          (((taps[1] >> shift) & 0xffu) * w1) +
                          (((taps[2] >> shift) & 0xffu) * w2) +
                          (((taps[3] >> shift) & 0xffu) * w3);

            rgba[channel] = (u8)((s + 0x8000u) >> 16);
        }
        return 1;
    }
    ndsS2DBilerp(taps, sx_q8 & 0xffu, pair->fy, rgba);
    return 1;
}

/* ---- palettes ---- */

static s32 ndsS2DTakeBank(void)
{
    /* Bank n is entries 16n..16n+15; CI8 runs own the entries from the
     * floor up. */
    if ((sNdsS2DBankCount >= NDS_S2D_BANKS) ||
        (((sNdsS2DBankCount + 1u) * 16u) > sNdsS2DHighFloor))
    {
        /* No reset: the caller presents the sprite in direct colour. */
        gNdsSource2DFailBanks++;
        return -1;
    }
    return (s32)sNdsS2DBankCount++;
}

static s32 ndsS2DRampBank(const SObj *sobj)
{
    const Sprite *sprite = &sobj->sprite;
    u8 env_r = 0u;
    u8 env_g = 0u;
    u8 env_b = 0u;
    NDSSource2DBank *bank;
    s32 taken;
    u32 i;

    if (sprite->bmfmt == G_IM_FMT_IA)
    {
        env_r = sobj->envcolor.r;
        env_g = sobj->envcolor.g;
        env_b = sobj->envcolor.b;
    }
    for (i = 0u; i < sNdsS2DBankCount; i++)
    {
        const NDSSource2DBank *b = &sNdsS2DBanks[i];

        if ((b->kind == NDS_S2D_BANK_RAMP) && (b->prim_r == sprite->red) &&
            (b->prim_g == sprite->green) && (b->prim_b == sprite->blue) &&
            (b->env_r == env_r) && (b->env_g == env_g) && (b->env_b == env_b))
        {
            return (s32)i;
        }
    }
    taken = ndsS2DTakeBank();
    if (taken < 0)
    {
        return -1;
    }
    bank = &sNdsS2DBanks[taken];
    memset(bank, 0, sizeof(*bank));
    bank->kind = NDS_S2D_BANK_RAMP;
    bank->prim_r = sprite->red;
    bank->prim_g = sprite->green;
    bank->prim_b = sprite->blue;
    bank->env_r = env_r;
    bank->env_g = env_g;
    bank->env_b = env_b;
    for (i = 1u; i < 16u; i++)
    {
        u32 step = i - 1u;
        u32 inv = 14u - step;

        bank->colors[i] = ndsS2DRgb15(
            ((u32)env_r * inv + (u32)sprite->red * step + 7u) / 14u,
            ((u32)env_g * inv + (u32)sprite->green * step + 7u) / 14u,
            ((u32)env_b * inv + (u32)sprite->blue * step + 7u) / 14u);
    }
    sNdsS2DPaletteDirty = 1u;
    return taken;
}

static u32 ndsS2DColorDistance(u16 a, u16 b)
{
    s32 dr = (s32)((a >> 11) & 31u) - (s32)((b >> 11) & 31u);
    s32 dg = (s32)((a >> 6) & 31u) - (s32)((b >> 6) & 31u);
    s32 db = (s32)((a >> 1) & 31u) - (s32)((b >> 1) & 31u);

    return (u32)((dr * dr) + (dg * dg) + (db * db));
}

/* A CI4 LUT as one 16-colour bank: index 0 transparent, the opaque entries
 * in order, a sixteenth opaque colour sharing its nearest slot. */
static s32 ndsS2DLut4Bank(const NDSSource2DSource *source)
{
    u16 slot_color[16];
    u32 slots = 1u;
    NDSSource2DBank *bank;
    s32 taken;
    u32 i;

    for (i = 0u; i < sNdsS2DBankCount; i++)
    {
        const NDSSource2DBank *b = &sNdsS2DBanks[i];

        if ((b->kind == NDS_S2D_BANK_LUT) &&
            (b->lut_asset_id == source->lut_asset_id) &&
            (b->lut_offset == source->lut_offset) &&
            (b->opaque == (u8)ndsS2DSpriteOpaque(source->sprite)) &&
            (b->modulate == sNdsS2DModulate))
        {
            return (s32)i;
        }
    }
    taken = ndsS2DTakeBank();
    if (taken < 0)
    {
        return -1;
    }
    bank = &sNdsS2DBanks[taken];
    memset(bank, 0, sizeof(*bank));
    bank->kind = NDS_S2D_BANK_LUT;
    bank->lut_asset_id = source->lut_asset_id;
    bank->lut_offset = source->lut_offset;
    bank->opaque = (u8)ndsS2DSpriteOpaque(source->sprite);
    bank->modulate = sNdsS2DModulate;
    for (i = 0u; i < 16u; i++)
    {
        u16 color = ndsS2DLutColor(source, i);
        u32 slot;

        if (bank->opaque != 0u)
        {
            color |= 1u;
        }
        if ((color & 1u) == 0u)
        {
            bank->remap[i] = 0u;
            continue;
        }
        for (slot = 1u; slot < slots; slot++)
        {
            if (slot_color[slot] == color)
            {
                break;
            }
        }
        if (slot == slots)
        {
            if (slots < 16u)
            {
                slot_color[slots] = color;
                bank->colors[slots] = ndsS2DRgba16ToRgb15(color);
                slots++;
            }
            else
            {
                u32 best_distance = 0xffffffffu;
                u32 candidate;

                slot = 1u;
                for (candidate = 1u; candidate < 16u; candidate++)
                {
                    u32 distance =
                        ndsS2DColorDistance(slot_color[candidate], color);

                    if (distance < best_distance)
                    {
                        best_distance = distance;
                        slot = candidate;
                    }
                }
            }
        }
        bank->remap[i] = (u8)slot;
    }
    sNdsS2DPaletteDirty = 1u;
    return taken;
}

/* A CI8 image's used opaque entries as a run of absolute palette entries
 * taken downward from 255. NULL when the run does not fit (the caller falls
 * back to direct colour). */
static const NDSSource2DCI8 *ndsS2DLut8Table(const NDSSource2DSource *source)
{
    const Sprite *sprite = source->sprite;
    NDSSource2DCI8 *table;
    u8 used[256];
    u16 run[256];
    u32 run_count = 0u;
    u32 x;
    u32 y;
    u32 i;

    for (i = 0u; i < sNdsS2DCI8Count; i++)
    {
        table = &sNdsS2DCI8[i];
        if ((table->lut_asset_id == source->lut_asset_id) &&
            (table->lut_offset == source->lut_offset) &&
            (table->image_asset_id == source->asset_id) &&
            (table->image_offset == source->bitmap_offset) &&
            (table->modulate == sNdsS2DModulate))
        {
            return table;
        }
    }
    if (sNdsS2DCI8Count >= NDS_S2D_CI8_TABLES)
    {
        return NULL;
    }
    memset(used, 0, sizeof(used));
    for (y = 0u; y < (u32)(u16)sprite->height; y++)
    {
        NDSSource2DSrcRow scan_row;

        ndsS2DPrepareRow(source, y, &scan_row);
        for (x = 0u; x < (u32)(u16)sprite->width; x++)
        {
            u32 raw;

            if (ndsS2DRowTapRaw(source, &scan_row, x, &raw) != 0)
            {
                used[raw & 0xffu] = 1u;
            }
        }
    }
    table = &sNdsS2DCI8[sNdsS2DCI8Count];
    memset(table, 0, sizeof(*table));
    for (i = 0u; i < 256u; i++)
    {
        u16 color;
        u32 slot;

        if (used[i] == 0u)
        {
            continue;
        }
        color = ndsS2DLutColor(source, i);
        if (ndsS2DSpriteOpaque(sprite) != 0u)
        {
            color |= 1u;
        }
        if ((color & 1u) == 0u)
        {
            continue;
        }
        for (slot = 0u; slot < run_count; slot++)
        {
            if (run[slot] == color)
            {
                break;
            }
        }
        if (slot == run_count)
        {
            run[run_count++] = color;
        }
        /* Entry assigned below, once the run's base is known. */
        table->remap[i] = (u8)(slot + 1u);
    }
    if ((run_count == 0u) ||
        ((sNdsS2DHighFloor - run_count) < ((sNdsS2DBankCount * 16u) + 1u)))
    {
        return NULL;
    }
    sNdsS2DHighFloor -= run_count;
    for (i = 0u; i < 256u; i++)
    {
        if (table->remap[i] != 0u)
        {
            table->remap[i] = (u8)(sNdsS2DHighFloor + table->remap[i] - 1u);
        }
    }
    for (i = 0u; i < run_count; i++)
    {
        sNdsS2DHighColors[sNdsS2DHighFloor + i] = ndsS2DRgba16ToRgb15(run[i]);
    }
    table->lut_asset_id = source->lut_asset_id;
    table->lut_offset = source->lut_offset;
    table->image_asset_id = source->asset_id;
    table->image_offset = source->bitmap_offset;
    table->modulate = sNdsS2DModulate;
    sNdsS2DCI8Count++;
    sNdsS2DPaletteDirty = 1u;
    return table;
}

/* ---- planning ---- */

static u32 ndsS2DMulQ16(u32 lhs_q16, u32 rhs_q16)
{
    u32 whole = lhs_q16 >> 16;
    u32 frac = lhs_q16 & 0xffffu;

    return (whole * rhs_q16) + (((frac * rhs_q16) + 0x8000u) >> 16);
}

static u32 ndsS2DFloatScaleQ16(f32 scale)
{
    if ((scale < 0.0001F) || (scale > 16.0F))
    {
        return 0u;
    }
    return (u32)((scale * 65536.0F) + 0.5F);
}

static s32 ndsS2DFinalDimensions(const Sprite *sprite, u32 *out_width,
                                 u32 *out_height)
{
    u32 scale_x_q16 = 1u << 16;
    u32 scale_y_q16 = 1u << 16;
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;

    if ((width == 0u) || (height == 0u))
    {
        return 0;
    }
    if ((sprite->attr & SP_FASTCOPY) == 0u)
    {
        scale_x_q16 = ndsS2DFloatScaleQ16(sprite->scalex);
        scale_y_q16 = ndsS2DFloatScaleQ16(sprite->scaley);
        if ((scale_x_q16 == 0u) || (scale_y_q16 == 0u))
        {
            return 0;
        }
    }
    *out_width = ((width * ndsS2DMulQ16(scale_x_q16, NDS_S2D_SCALE_X_Q16)) +
                  0x8000u) >> 16;
    *out_height = ((height * ndsS2DMulQ16(scale_y_q16, NDS_S2D_SCALE_Y_Q16)) +
                   0x8000u) >> 16;
    if (*out_width == 0u) *out_width = 1u;
    if (*out_height == 0u) *out_height = 1u;
    return 1;
}

static void ndsS2DSearchPlan(u32 width, u32 height, u32 target, u32 depth,
                             u32 covered, u32 area, NDSSource2DShape chosen[],
                             NDSSource2DRowPlan *best, u32 *best_area)
{
    u32 i;

    if (depth == target)
    {
        if ((covered >= width) && (area < *best_area))
        {
            u32 x = 0u;

            best->count = (u8)target;
            for (i = 0u; i < target; i++)
            {
                best->shape[i] = chosen[i];
                best->x[i] = (u16)x;
                x += chosen[i].width;
            }
            *best_area = area;
        }
        return;
    }
    for (i = 0u; i < (u32)(sizeof(sNdsS2DShapes) / sizeof(sNdsS2DShapes[0]));
         i++)
    {
        const NDSSource2DShape *shape = &sNdsS2DShapes[i];

        if (shape->height < height)
        {
            continue;
        }
        chosen[depth] = *shape;
        ndsS2DSearchPlan(width, height, target, depth + 1u,
                         covered + shape->width,
                         area + ((u32)shape->width * shape->height), chosen,
                         best, best_area);
    }
}

static s32 ndsS2DRowPlan(u32 width, u32 height, NDSSource2DRowPlan *plan)
{
    NDSSource2DShape chosen[NDS_S2D_MAX_COLS];
    u32 key;
    u32 count;
    u32 i;

    if ((width == 0u) || (height == 0u) || (height > 64u) ||
        (width > (64u * NDS_S2D_MAX_COLS)))
    {
        return 0;
    }
    key = (height << 16) | width;
    for (i = 0u; i < NDS_S2D_PLAN_MEMO; i++)
    {
        if (sNdsS2DPlanMemo[i].key == key)
        {
            *plan = sNdsS2DPlanMemo[i].plan;
            return 1;
        }
    }
    memset(plan, 0, sizeof(*plan));
    for (count = 1u; count <= NDS_S2D_MAX_COLS; count++)
    {
        u32 best_area = 0xffffffffu;

        ndsS2DSearchPlan(width, height, count, 0u, 0u, 0u, chosen, plan,
                         &best_area);
        if (plan->count != 0u)
        {
            sNdsS2DPlanMemo[sNdsS2DPlanMemoNext].key = key;
            sNdsS2DPlanMemo[sNdsS2DPlanMemoNext].plan = *plan;
            sNdsS2DPlanMemoNext = (sNdsS2DPlanMemoNext + 1u) &
                                  (NDS_S2D_PLAN_MEMO - 1u);
            return 1;
        }
    }
    return 0;
}

/* The next row of a cut: 64 lines while that many remain, then the largest
 * of 32/16 that fits, the remainder last (its cells round it up to 8/16/32). */
static u32 ndsS2DRowHeight(u32 remaining)
{
    if (remaining >= 64u) return 64u;
    if (remaining >= 32u) return 32u;
    if (remaining >= 16u) return 16u;
    return remaining;
}

/* ---- baking ---- */

static u32 ndsS2DEncodingFor(const Sprite *sprite)
{
    switch (sprite->bmfmt)
    {
    case G_IM_FMT_I:
        return ((sprite->bmsiz == G_IM_SIZ_4b) ||
                (sprite->bmsiz == G_IM_SIZ_8b)) ? NDS_S2D_ENC_RAMP :
                                                  NDS_S2D_ENC_NONE;
    case G_IM_FMT_IA:
        return ((sprite->bmsiz == G_IM_SIZ_4b) ||
                (sprite->bmsiz == G_IM_SIZ_8b) ||
                (sprite->bmsiz == G_IM_SIZ_16b)) ? NDS_S2D_ENC_RAMP :
                                                   NDS_S2D_ENC_NONE;
    case G_IM_FMT_CI:
        return (sprite->bmsiz == G_IM_SIZ_4b) ? NDS_S2D_ENC_LUT4 :
            ((sprite->bmsiz == G_IM_SIZ_8b) ? NDS_S2D_ENC_LUT8 :
                                              NDS_S2D_ENC_NONE);
    case G_IM_FMT_RGBA:
        return ((sprite->bmsiz == G_IM_SIZ_16b) ||
                (sprite->bmsiz == G_IM_SIZ_32b)) ? NDS_S2D_ENC_BMP :
                                                   NDS_S2D_ENC_NONE;
    default:
        return NDS_S2D_ENC_NONE;
    }
}

static void ndsS2DPut4(u8 *scratch, u32 cell_width, u32 x, u32 y, u32 index)
{
    u32 tile = ((y >> 3) * (cell_width >> 3)) + (x >> 3);
    u32 offset = (tile * 32u) + ((y & 7u) * 4u) + ((x & 7u) >> 1);

    if ((x & 1u) == 0u)
    {
        scratch[offset] = (u8)((scratch[offset] & 0xf0u) | index);
    }
    else
    {
        scratch[offset] = (u8)((scratch[offset] & 0x0fu) | (index << 4));
    }
}

static void ndsS2DPut8(u8 *scratch, u32 cell_width, u32 x, u32 y, u32 index)
{
    u32 tile = ((y >> 3) * (cell_width >> 3)) + (x >> 3);

    scratch[(tile * 64u) + ((y & 7u) * 8u) + (x & 7u)] = (u8)index;
}

/* The ramp step of a filtered I/IA texel (the Results rule); 0 = clear. */
static u32 ndsS2DRampIndex(const Sprite *sprite, const u8 rgba[4])
{
    u32 index;

    if (sprite->bmfmt == G_IM_FMT_IA)
    {
        if (rgba[3] < 128u)
        {
            return 0u;
        }
        index = (((u32)rgba[0] + 8u) * 3856u) >> 16;
        if (index == 0u) index = 1u;
    }
    else
    {
        index = (((u32)rgba[3] + 8u) * 3856u) >> 16;
        if ((index == 0u) && (ndsS2DSpriteOpaque(sprite) != 0u)) index = 1u;
    }
    return (index > 15u) ? 15u : index;
}

static s32 ndsS2DWriteCell(NDSSource2DCell *cell,
                           const NDSSource2DSource *source,
                           const u8 *remap)
{
    u32 tinted = ((cell->encoding == NDS_S2D_ENC_BMP) &&
                  ((cell->bmfmt == G_IM_FMT_I) ||
                   (cell->bmfmt == G_IM_FMT_IA))) ? 1u : 0u;
    u32 step_x_q16 = ((u32)cell->source_width << 16) / cell->final_width;
    u32 step_y_q16 = ((u32)cell->source_height << 16) / cell->final_height;
    u8 *scratch = (u8 *)sNdsS2DScratch;
    u32 pixels = (u32)cell->cell_width * cell->cell_height;
    u32 bytes;
    u32 x;
    u32 y;

    bytes = (cell->encoding == NDS_S2D_ENC_BMP) ? (pixels * 2u) :
        ((cell->encoding == NDS_S2D_ENC_LUT8) ? pixels : (pixels / 2u));
    if (cell->encoding == NDS_S2D_ENC_BMP)
    {
        dmaFillHalfWords(0u, cell->gfx, bytes);
    }
    else
    {
        memset(scratch, 0, bytes);
    }
    for (y = 0u; y < cell->cell_height; y++)
    {
        u32 dy = (u32)cell->tile_y + y;
        const u32 rows = (gNdsS2DRowSampler != 0u) ? 1u : 0u;
        const u32 verify = ((rows != 0u) && (gNdsS2DRowVerify != 0u)) ? 1u : 0u;
        NDSSource2DRowPair pair;
        NDSSource2DSrcRow nearest_row;

        if (dy >= cell->final_height)
        {
            break;
        }
        if (rows != 0u)
        {
            if ((cell->encoding == NDS_S2D_ENC_LUT4) ||
                (cell->encoding == NDS_S2D_ENC_LUT8))
            {
                u32 sy = ((dy * step_y_q16) + (step_y_q16 >> 1)) >> 16;

                if (sy >= (u32)(u16)source->sprite->height)
                {
                    sy = (u32)(u16)source->sprite->height - 1u;
                }
                ndsS2DPrepareRow(source, sy, &nearest_row);
            }
            else
            {
                ndsS2DPrepareFilteredRows(source, step_y_q16, dy, &pair);
            }
        }
        for (x = 0u; x < cell->cell_width; x++)
        {
            u32 dx = (u32)cell->tile_x + x;
            u8 rgba[4];

            if (dx >= cell->final_width)
            {
                break;
            }
            if ((cell->encoding == NDS_S2D_ENC_LUT4) ||
                (cell->encoding == NDS_S2D_ENC_LUT8))
            {
                u32 raw;
                u32 index;
                s32 ok;

                if (rows != 0u)
                {
                    u32 sx = ((dx * step_x_q16) + (step_x_q16 >> 1)) >> 16;

                    if (sx >= (u32)(u16)source->sprite->width)
                    {
                        sx = (u32)(u16)source->sprite->width - 1u;
                    }
                    ok = ndsS2DRowTapRaw(source, &nearest_row, sx, &raw);
                    if (verify != 0u)
                    {
                        u32 want = 0u;
                        s32 want_ok = ndsS2DSampleNearestRaw(
                            source, step_x_q16, step_y_q16, dx, dy, &want);

                        gNdsS2DRowVerifyPixels++;
                        if ((want_ok != ok) || ((ok != 0) && (want != raw)))
                        {
                            gNdsS2DRowVerifyMismatches++;
                        }
                    }
                }
                else
                {
                    ok = ndsS2DSampleNearestRaw(source, step_x_q16, step_y_q16,
                                                dx, dy, &raw);
                }
                if (ok == 0)
                {
                    return 0;
                }
                index = remap[raw & ((cell->encoding == NDS_S2D_ENC_LUT4) ?
                                         0x0fu : 0xffu)];
                if (index == 0u)
                {
                    continue;
                }
                if (cell->encoding == NDS_S2D_ENC_LUT4)
                {
                    ndsS2DPut4(scratch, cell->cell_width, x, y, index);
                }
                else
                {
                    ndsS2DPut8(scratch, cell->cell_width, x, y, index);
                }
                continue;
            }
            if (rows != 0u)
            {
                s32 ok = ndsS2DSampleFilteredRows(source, &pair, step_x_q16,
                                                  dx, rgba);

                if (verify != 0u)
                {
                    u8 want[4] = { 0u, 0u, 0u, 0u };
                    s32 want_ok = ndsS2DSampleFiltered(source, step_x_q16,
                                                       step_y_q16, dx, dy,
                                                       want);

                    gNdsS2DRowVerifyPixels++;
                    if ((want_ok != ok) ||
                        ((ok != 0) && (memcmp(want, rgba, sizeof(want)) != 0)))
                    {
                        gNdsS2DRowVerifyMismatches++;
                    }
                }
                if (ok == 0)
                {
                    return 0;
                }
            }
            else if (ndsS2DSampleFiltered(source, step_x_q16, step_y_q16, dx,
                                          dy, rgba) == 0)
            {
                return 0;
            }
            if (cell->encoding == NDS_S2D_ENC_BMP)
            {
                if (rgba[3] >= 128u)
                {
                    if (tinted != 0u)
                    {
                        /* The ramp's colour, unbanked: env to prim by the
                         * intensity (IA) or black to prim by the coverage
                         * (I), as ndsS2DRampBank's steps. */
                        u32 t = (cell->bmfmt == G_IM_FMT_IA) ? rgba[0] : rgba[3];
                        u32 c;

                        for (c = 0u; c < 3u; c++)
                        {
                            u32 shift = 16u - (c * 8u);
                            u32 prim = (cell->tint_prim >> shift) & 0xffu;
                            u32 env = (cell->tint_env >> shift) & 0xffu;

                            rgba[c] = (u8)(((env * (255u - t)) + (prim * t) +
                                            127u) / 255u);
                        }
                    }
                    cell->gfx[(y * cell->cell_width) + x] =
                        ndsS2DRgb15(rgba[0], rgba[1], rgba[2]);
                }
                continue;
            }
            {
                u32 index = ndsS2DRampIndex(source->sprite, rgba);

                if (index != 0u)
                {
                    ndsS2DPut4(scratch, cell->cell_width, x, y, index);
                }
            }
        }
    }
    if (cell->encoding != NDS_S2D_ENC_BMP)
    {
        u32 words = bytes / sizeof(u32);
        u32 i;

        for (i = 0u; i < words; i++)
        {
            ((u32 *)(void *)cell->gfx)[i] = sNdsS2DScratch[i];
        }
    }
    return 1;
}

static NDSSource2DCell *ndsS2DFindOrBakeCell(const NDSSource2DSource *source,
                                             u32 encoding, const u8 *remap,
                                             u32 tint_prim, u32 tint_env,
                                             u32 final_width, u32 final_height,
                                             u32 tile_x, u32 tile_y,
                                             const NDSSource2DShape *shape)
{
    const Sprite *sprite = source->sprite;
    NDSSource2DCell *cell;
    u32 bytes;
    u32 i;

    for (i = 0u; i < sNdsS2DCellCount; i++)
    {
        cell = &sNdsS2DCells[i];
        if ((cell->asset_id == source->asset_id) &&
            (cell->bitmap_offset == source->bitmap_offset) &&
            (cell->lut_asset_id == source->lut_asset_id) &&
            (cell->lut_offset == source->lut_offset) &&
            (cell->source_width == (u16)sprite->width) &&
            (cell->source_height == (u16)sprite->height) &&
            (cell->final_width == final_width) &&
            (cell->final_height == final_height) &&
            (cell->tile_x == tile_x) && (cell->tile_y == tile_y) &&
            (cell->cell_width == shape->width) &&
            (cell->cell_height == shape->height) &&
            (cell->bmfmt == sprite->bmfmt) && (cell->bmsiz == sprite->bmsiz) &&
            (cell->encoding == encoding) && (cell->tint_prim == tint_prim) &&
            (cell->tint_env == tint_env) && (cell->modulate == sNdsS2DModulate))
        {
            return cell;
        }
    }
    if (sNdsS2DCellCount >= NDS_S2D_CELLS)
    {
        gNdsSource2DFailCells++;
        sNdsS2DResetPending = 1u;
        return NULL;
    }
    bytes = (u32)shape->width * shape->height;
    bytes = (encoding == NDS_S2D_ENC_BMP) ? (bytes * 2u) :
        ((encoding == NDS_S2D_ENC_LUT8) ? bytes : (bytes / 2u));
    cell = &sNdsS2DCells[sNdsS2DCellCount];
    memset(cell, 0, sizeof(*cell));
    cell->gfx = ndsS2DAllocObjBytes(bytes);
    if (cell->gfx == NULL)
    {
        return NULL;
    }
    cell->asset_id = source->asset_id;
    cell->bitmap_offset = source->bitmap_offset;
    cell->lut_asset_id = source->lut_asset_id;
    cell->lut_offset = source->lut_offset;
    cell->source_width = (u16)sprite->width;
    cell->source_height = (u16)sprite->height;
    cell->final_width = (u16)final_width;
    cell->final_height = (u16)final_height;
    cell->tile_x = (u16)tile_x;
    cell->tile_y = (u16)tile_y;
    cell->cell_width = shape->width;
    cell->cell_height = shape->height;
    cell->bmfmt = sprite->bmfmt;
    cell->bmsiz = sprite->bmsiz;
    cell->encoding = (u8)encoding;
    cell->color_format = (encoding == NDS_S2D_ENC_BMP) ?
        (u8)SpriteColorFormat_Bmp :
        ((encoding == NDS_S2D_ENC_LUT8) ? (u8)SpriteColorFormat_256Color :
                                          (u8)SpriteColorFormat_16Color);
    cell->size = shape->size;
    cell->tint_prim = tint_prim;
    cell->tint_env = tint_env;
    cell->modulate = sNdsS2DModulate;
    if (ndsS2DWriteCell(cell, source, remap) == 0)
    {
        gNdsSource2DFailProvenance++;
        return NULL;
    }
    sNdsS2DCellCount++;
    gNdsSource2DBakeCellCount++;
    return cell;
}

static s32 ndsS2DRoundFloat(f32 value)
{
    return (value >= 0.0F) ? (s32)(value + 0.5F) : (s32)(value - 0.5F);
}

static s32 ndsS2DMapCoord(s32 source, s32 origin, u32 scale_q16)
{
    s32 product = (source - origin) * (s32)scale_q16;

    return (product >= 0) ? ((product + 0x8000) >> 16) :
                            -(((-product) + 0x8000) >> 16);
}

/* ---- the BG2 background ---- */

static void ndsS2DClearBackground(void)
{
    u16 *layer;
    u32 pitch;
    u32 width;
    u32 height;
    u32 epoch;

    if (sNdsS2DBackgroundCount == 0u)
    {
        return;
    }
    sNdsS2DBackgroundCount = 0u;
    layer = ndsPlatformGetOriginalSpriteOverlayLayer(FALSE, &pitch, &width,
                                                     &height, &epoch);
    if (layer != NULL)
    {
        ndsPlatformHideOriginalSpriteOverlayUntilCommit(FALSE);
        dmaFillHalfWords(0u, layer, 256u * 256u * sizeof(u16));
        (void)ndsPlatformCommitOriginalSpriteFinalLayer(FALSE, 256u * 192u);
    }
}

/* A large sprite drawn before the frame's first OBJ sprite, as the next BG2
 * layer: each layer is written once while its image, size and place, and
 * every layer under it, stay the same. */
static s32 ndsS2DDrawBackground(const SObj *sobj,
                                const NDSSource2DSource *source,
                                u32 final_width, u32 final_height,
                                s32 screen_x, s32 screen_y)
{
    u32 key[7];
    u32 index = sNdsS2DFrameBackgrounds++;
    u16 *layer;
    u32 pitch;
    u32 width;
    u32 height;
    u32 epoch;
    u32 step_x_q16;
    u32 step_y_q16;
    s32 y;

    key[0] = source->asset_id;
    key[1] = source->bitmap_offset;
    key[2] = source->lut_asset_id;
    key[3] = source->lut_offset;
    key[4] = (final_width << 16) | final_height;
    key[5] = ((u32)(screen_x & 0xffff) << 16) | (u32)(screen_y & 0xffff);
    key[6] = sNdsS2DModulate;
    if (index < sNdsS2DBackgroundCount)
    {
        if (memcmp(key, sNdsS2DBackgroundKey[index], sizeof(key)) == 0)
        {
            return 1;
        }
        if (index != 0u)
        {
            /* A changed upper layer over layers that cannot be redrawn from
             * here: the next frame rebuilds the whole stack from the bottom. */
            ndsS2DClearBackground();
            return 1;
        }
        sNdsS2DBackgroundCount = 0u;
    }
    if (index != sNdsS2DBackgroundCount)
    {
        return 1;
    }
    layer = ndsPlatformGetOriginalSpriteOverlayLayer(FALSE, &pitch, &width,
                                                     &height, &epoch);
    if ((layer == NULL) || (pitch < 256u))
    {
        gNdsSource2DFailBackground++;
        return 0;
    }
    ndsPlatformHideOriginalSpriteOverlayUntilCommit(FALSE);
    if (index == 0u)
    {
        dmaFillHalfWords(0u, layer, 256u * 256u * sizeof(u16));
    }
    step_x_q16 = ((u32)(u16)sobj->sprite.width << 16) / final_width;
    step_y_q16 = ((u32)(u16)sobj->sprite.height << 16) / final_height;
    for (y = 0; y < (s32)final_height; y++)
    {
        s32 out_y = screen_y + y;
        u16 *row;
        s32 x;
        const u32 rows = (gNdsS2DRowSampler != 0u) ? 1u : 0u;
        NDSSource2DRowPair pair;

        if ((out_y < 0) || (out_y >= 192))
        {
            continue;
        }
        if (rows != 0u)
        {
            ndsS2DPrepareFilteredRows(source, step_y_q16, (u32)y, &pair);
        }
        row = &layer[(u32)out_y * pitch];
        for (x = 0; x < (s32)final_width; x++)
        {
            s32 out_x = screen_x + x;
            u8 rgba[4];
            s32 ok;

            if ((out_x < 0) || (out_x >= 256))
            {
                continue;
            }
            if (rows != 0u)
            {
                ok = ndsS2DSampleFilteredRows(source, &pair, step_x_q16,
                                              (u32)x, rgba);
                if (gNdsS2DRowVerify != 0u)
                {
                    u8 want[4] = { 0u, 0u, 0u, 0u };
                    s32 want_ok = ndsS2DSampleFiltered(source, step_x_q16,
                                                       step_y_q16, (u32)x,
                                                       (u32)y, want);

                    gNdsS2DRowVerifyPixels++;
                    if ((want_ok != ok) ||
                        ((ok != 0) && (memcmp(want, rgba, sizeof(want)) != 0)))
                    {
                        gNdsS2DRowVerifyMismatches++;
                    }
                }
            }
            else
            {
                ok = ndsS2DSampleFiltered(source, step_x_q16, step_y_q16,
                                          (u32)x, (u32)y, rgba);
            }
            if (ok == 0)
            {
                gNdsSource2DFailBackground++;
                return 0;
            }
            if (rgba[3] >= 128u)
            {
                row[out_x] = ndsS2DRgb15(rgba[0], rgba[1], rgba[2]);
            }
        }
    }
    (void)ndsPlatformCommitOriginalSpriteFinalLayer(FALSE, 256u * 192u);
    (void)ndsPlatformQueueNativeWallpaperAffine(256, 256, 0, 0);
    memcpy(sNdsS2DBackgroundKey[index], key, sizeof(key));
    sNdsS2DBackgroundCount = index + 1u;
    gNdsSource2DBackgroundWrites++;
    return 1;
}

/* One SObj: plan, bake and place every cell, or record why not. */
static void ndsS2DDrawSObj(const SObj *sobj)
{
    const Sprite *sprite = &sobj->sprite;
    NDSSource2DSource source;
    u32 encoding = ndsS2DEncodingFor(sprite);
    const u8 *remap = NULL;
    u32 final_width;
    u32 final_height;
    u32 background_candidate = (sNdsS2DFrameObjects == 0u) ? 1u : 0u;
    s32 bank = 0;
    s32 screen_x;
    s32 screen_y;
    u32 row_y;
    u32 alpha = 15u;
    u32 tint_prim = 0u;
    u32 tint_env = 0u;

    gNdsSource2DDrawSObjCount++;
    sNdsS2DFrameSprites++;
    if ((encoding == NDS_S2D_ENC_NONE) ||
        (ndsS2DFinalDimensions(sprite, &final_width, &final_height) == 0))
    {
        gNdsSource2DFailFormat++;
        ndsS2DRecordFailure(sobj, NDS_NATIVE_FAILURE_BAD_ASSET);
        return;
    }
    if (ndsS2DResolveSource(sprite, &source) == 0)
    {
        ndsS2DRecordFailure(sobj, NDS_NATIVE_FAILURE_BAD_ASSET);
        return;
    }
    screen_x = ndsS2DMapCoord(ndsS2DRoundFloat(sobj->pos.x),
                              NDS_S2D_SRC_ORIGIN_X, NDS_S2D_SCALE_X_Q16);
    screen_y = ndsS2DMapCoord(ndsS2DRoundFloat(sobj->pos.y),
                              NDS_S2D_SRC_ORIGIN_Y, NDS_S2D_SCALE_Y_Q16);
    if (background_candidate != 0u)
    {
        if (((final_width * final_height) >= NDS_S2D_BACKGROUND_AREA) &&
            (sNdsS2DFrameBackgrounds < NDS_S2D_BACKGROUND_LAYERS))
        {
            if (ndsS2DDrawBackground(sobj, &source, final_width, final_height,
                                     screen_x, screen_y) == 0)
            {
                ndsS2DRecordFailure(sobj, NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
            }
            return;
        }
        if (sNdsS2DFrameBackgrounds == 0u)
        {
            ndsS2DClearBackground();
        }
    }
    sNdsS2DFrameObjects++;
    if ((final_width > (64u * NDS_S2D_MAX_COLS)) || (final_height > 256u))
    {
        gNdsSource2DFailTile++;
        ndsS2DRecordFailure(sobj, NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
        return;
    }
    /* Out of palette room, an indexed sprite is presented in direct colour
     * (bitmap OBJ) rather than failed. */
    if (encoding == NDS_S2D_ENC_LUT4)
    {
        bank = ndsS2DLut4Bank(&source);
        if (bank >= 0)
        {
            remap = sNdsS2DBanks[bank].remap;
        }
        else
        {
            encoding = NDS_S2D_ENC_BMP;
            bank = 0;
        }
    }
    else if (encoding == NDS_S2D_ENC_LUT8)
    {
        const NDSSource2DCI8 *table = ndsS2DLut8Table(&source);

        if (table != NULL)
        {
            remap = table->remap;
        }
        else
        {
            encoding = NDS_S2D_ENC_BMP;
        }
    }
    else if (encoding == NDS_S2D_ENC_RAMP)
    {
        bank = ndsS2DRampBank(sobj);
        if (bank < 0)
        {
            encoding = NDS_S2D_ENC_BMP;
            bank = 0;
        }
    }
    if ((encoding == NDS_S2D_ENC_BMP) &&
        ((sprite->bmfmt == G_IM_FMT_I) || (sprite->bmfmt == G_IM_FMT_IA)))
    {
        tint_prim = ((u32)sprite->red << 16) | ((u32)sprite->green << 8) |
                    (u32)sprite->blue;
        if (sprite->bmfmt == G_IM_FMT_IA)
        {
            tint_env = ((u32)sobj->envcolor.r << 16) |
                       ((u32)sobj->envcolor.g << 8) | (u32)sobj->envcolor.b;
        }
    }
    if (encoding == NDS_S2D_ENC_BMP)
    {
        /* libnds: a bitmap OBJ's palette argument is its alpha, 0 hides it
         * (nds/arm9/sprite.h:391); the Results tenant's R01-A lesson. */
        alpha = ((u32)sprite->alpha * 15u + 127u) / 255u;
        if (alpha == 0u)
        {
            return;
        }
    }
    for (row_y = 0u; row_y < final_height;)
    {
        u32 row_height = ndsS2DRowHeight(final_height - row_y);
        NDSSource2DRowPlan plan;
        u32 tile;

        if (((screen_y + (s32)row_y) >= 192) ||
            ((screen_y + (s32)row_y + (s32)row_height) <= 0))
        {
            row_y += row_height;
            continue;
        }
        if (ndsS2DRowPlan(final_width, row_height, &plan) == 0)
        {
            gNdsSource2DFailTile++;
            ndsS2DRecordFailure(sobj, NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
            return;
        }
        for (tile = 0u; tile < plan.count; tile++)
        {
            NDSSource2DCell *cell;
            s32 x = screen_x + (s32)plan.x[tile];

            if ((x >= 256) || ((x + (s32)plan.shape[tile].width) <= 0))
            {
                continue;
            }
            if (sNdsS2DNextOamId < 0)
            {
                gNdsSource2DFailOam++;
                ndsS2DRecordFailure(sobj, NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
                return;
            }
            cell = ndsS2DFindOrBakeCell(&source, encoding, remap, tint_prim,
                                        tint_env, final_width, final_height,
                                        plan.x[tile], row_y,
                                        &plan.shape[tile]);
            if (cell == NULL)
            {
                ndsS2DRecordFailure(sobj, NDS_NATIVE_FAILURE_REJECTED_PROGRAM);
                return;
            }
            oamSet(&oamMain, sNdsS2DNextOamId, x, screen_y + (s32)row_y, 0,
                   (cell->color_format == (u8)SpriteColorFormat_Bmp) ?
                       (int)alpha : bank,
                   cell->size, (SpriteColorFormat)cell->color_format,
                   cell->gfx, -1, false, false, false, false, false);
            sNdsS2DNextOamId--;
            sNdsS2DFrameNeedsCommit = 1u;
            gNdsSource2DEmitCount++;
        }
        row_y += row_height;
    }
}

static s32 ndsS2DSceneIsTenant(u32 scene)
{
    switch (scene)
    {
    case nSCKind1PIntro:
    case nSCKind1PStageClear:
    case nSCKind1PContinue:
    case nSCKind1PChallenger:
    case nSCKindMessage:
    case nSCKindStaffroll:
#if defined(REGION_US)
    case nSCKindCongra:
#endif
#if !NDS_P2_MENU_SHELL
    /* The bonus practice character selects present natively through the 1P
     * select's owner (nds_menu_shell_onep.c), whose baked screen is not
     * bounded by this presenter's 64 KB OBJ bank. Only a shell-off build,
     * which compiles that owner out, still draws their SObjs here. */
    case nSCKind1PBonus1Players:
    case nSCKind1PBonus2Players:
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

static void ndsS2DEnterNow(void)
{
    sNdsS2DEnterPending = 0u;
    ndsS2DResetCache();
    sNdsS2DNextOamId = 127;
    sNdsS2DFrameSprites = 0u;
    sNdsS2DFrameNeedsCommit = 0u;
    sNdsS2DResetPending = 0u;
    sNdsS2DBackgroundCount = 0u;
    sNdsS2DFrameBackgrounds = 0u;
    sNdsS2DFrameObjects = 0u;
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    oamClear(&oamMain, 0, 128);
    oamUpdate(&oamMain);
    sNdsS2DActive = 1u;
    gNdsSource2DEnterCount++;
}

#endif /* NDS_RENDERER_HW_TRIANGLES */

void ndsSource2DEnterForScene(u32 scene)
{
#if NDS_RENDERER_HW_TRIANGLES
    if (ndsS2DSceneIsTenant(scene) == FALSE)
    {
        return;
    }
    if (scene == (u32)nSCKindStaffroll)
    {
        sNdsS2DSrcOriginX = 20;
        sNdsS2DSrcOriginY = 20;
        sNdsS2DScaleXQ16 = 27962u;   /* 256 / 600 */
        sNdsS2DScaleYQ16 = 28597u;   /* 192 / 440 */
    }
    else
    {
        sNdsS2DSrcOriginX = 10;
        sNdsS2DSrcOriginY = 10;
        sNdsS2DScaleXQ16 = 55924u;   /* 256 / 300 */
        sNdsS2DScaleYQ16 = 57195u;   /* 192 / 220 */
    }
    /* Under a held frame the OBJ init would recolour the frame on screen;
     * the Thaw enters (ndsSource2DEnterPending). */
    if (ndsPlatformTransitionHolding() != 0u)
    {
        sNdsS2DEnterPending = 1u;
        return;
    }
    ndsS2DEnterNow();
#else
    (void)scene;
#endif
}

void ndsSource2DEnterPending(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsS2DEnterPending != 0u)
    {
        ndsS2DEnterNow();
    }
#endif
}

void ndsSource2DExit(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    sNdsS2DEnterPending = 0u;
    if (sNdsS2DActive == 0u)
    {
        return;
    }
    oamClear(&oamMain, 0, 128);
    oamUpdate(&oamMain);
    ndsS2DResetCache();
    sNdsS2DBackgroundCount = 0u;
    sNdsS2DActive = 0u;
    gNdsSource2DExitCount++;
#endif
}

s32 ndsSource2DIsActive(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    return ((sNdsS2DActive != 0u) || (sNdsS2DEnterPending != 0u)) ? 1 : 0;
#else
    return 0;
#endif
}

void ndsSource2DBeginFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    s32 first_previous;

    if (sNdsS2DActive == 0u)
    {
        return;
    }
    if (sNdsS2DResetPending != 0u)
    {
        sNdsS2DResetPending = 0u;
        ndsS2DResetCache();
        gNdsSource2DCacheResetCount++;
    }
    /* The previous frame's entries run from its final cursor up to 127. */
    first_previous = sNdsS2DNextOamId + 1;
    if (first_previous < 128)
    {
        oamClear(&oamMain, first_previous, 128 - first_previous);
        sNdsS2DFrameNeedsCommit = 1u;
    }
    sNdsS2DNextOamId = 127;
    sNdsS2DFrameSprites = 0u;
    /* Fewer background layers last frame than BG2 holds: the layers above
     * them left the picture, so the stack is rebuilt from the bottom. A frame
     * that drew nothing at all keeps it (a scene's first frame after Thaw). */
    if ((sNdsS2DFrameBackgrounds < sNdsS2DBackgroundCount) &&
        ((sNdsS2DFrameBackgrounds != 0u) || (sNdsS2DFrameObjects != 0u)))
    {
        ndsS2DClearBackground();
    }
    sNdsS2DFrameBackgrounds = 0u;
    sNdsS2DFrameObjects = 0u;
#endif
}

s32 ndsSource2DDrawGObj(struct GObj *gobj)
{
    return ndsSource2DDrawGObjModulated(gobj, 0xffffffu);
}

s32 ndsSource2DDrawGObjModulated(struct GObj *gobj, u32 modulate_rgb)
{
#if NDS_RENDERER_HW_TRIANGLES
    SObj *sobj;

    if (sNdsS2DActive == 0u)
    {
        /* Entered at the Thaw: nothing to draw into before it. */
        return (sNdsS2DEnterPending != 0u) ? 1 : 0;
    }
    if (gobj == NULL)
    {
        return 1;
    }
    sNdsS2DModulate = modulate_rgb & 0xffffffu;
    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        ndsS2DDrawSObj(sobj);
    }
    sNdsS2DModulate = 0xffffffu;
    return 1;
#else
    (void)gobj;
    (void)modulate_rgb;
    return 0;
#endif
}

void ndsSource2DCommit(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsS2DActive == 0u)
    {
        return;
    }
    if (sNdsS2DPaletteDirty != 0u)
    {
        vu16 *dst = (vu16 *)(void *)SPRITE_PALETTE;
        u32 bank;
        u32 i;

        for (bank = 0u; bank < sNdsS2DBankCount; bank++)
        {
            for (i = 0u; i < 16u; i++)
            {
                dst[(bank * 16u) + i] = sNdsS2DBanks[bank].colors[i];
            }
        }
        for (i = sNdsS2DHighFloor; i < 256u; i++)
        {
            dst[i] = sNdsS2DHighColors[i];
        }
        sNdsS2DPaletteDirty = 0u;
    }
    if (sNdsS2DFrameNeedsCommit != 0u)
    {
        oamUpdate(&oamMain);
        sNdsS2DFrameNeedsCommit = 0u;
    }
#endif
}
