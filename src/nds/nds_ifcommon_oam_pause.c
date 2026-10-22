/* IFCommon on OAM, the menus over the battle: the 1P bonus task icons, the
 * common letters, the pause cells and Training's menu. A slice of
 * nds_ifcommon_oam.c. */

#if NDS_P2_1P_GAME
/* The 1P bonus boards' task row (sc1PBonusStageMakeTargetSprites /
 * MakePlatformSprites, sc1pbonusstage.c): one SObj per remaining target or
 * platform, all the same 16x16 CI4 SC1PStageClear3 sprite, SP_TEXSHUF |
 * SP_TRANSPARENT, at the top left of the game screen. The one live bitmap is
 * decoded once into a 16x16 4bpp cell with its own LUT as palette 9
 * (transparent source entries become index 0); each SObj is then placed like
 * the item arrow, through the screen's 0.8 matrix. */
#define NDS_IFCOMMON_TASK_PALETTE 9u
#define NDS_IFCOMMON_TASK_SIZE 16u

static s32 ndsIFCommonBakeTaskIcon(const Sprite *sprite)
{
    u32 cell[NDS_IFCOMMON_TASK_BANK_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u8 remap[16];
    u16 colors[16];
    const Bitmap *bitmap;
    const u8 *pixels;
    const u16 *lut;
    u32 width_img;
    u32 row_bytes;
    u32 used = 1u;
    u32 *dst;
    u32 i;
    u32 y;

    if ((sprite == NULL) || (sprite->bitmap == NULL) || (sprite->LUT == NULL) ||
        (sprite->bmfmt != G_IM_FMT_CI) || (sprite->bmsiz != G_IM_SIZ_4b) ||
        (sprite->nbitmaps != 1) ||
        ((u32)(u16)sprite->width > NDS_IFCOMMON_TASK_SIZE) ||
        ((u32)(u16)sprite->height > NDS_IFCOMMON_TASK_SIZE))
    {
        return FALSE;
    }
    if (sNdsIFCommonTaskBitmap == sprite->bitmap)
    {
        return TRUE;
    }
    bitmap = sprite->bitmap;
    pixels = (const u8 *)bitmap->buf;
    lut = (const u16 *)sprite->LUT;
    width_img = (u32)(u16)bitmap->width_img;
    if (width_img == 0u)
    {
        width_img = (u32)(u16)bitmap->width;
    }
    if ((pixels == NULL) || (width_img < (u32)(u16)sprite->width))
    {
        return FALSE;
    }
    /* RGBA5551 LUT, halfword-swapped within its words like every converted
     * LUT (entry i at [i ^ 1]); alpha 0 is the cutout. */
    memset(remap, 0, sizeof(remap));
    memset(colors, 0, sizeof(colors));
    for (i = 0u; i < 16u; i++)
    {
        u16 rgba = lut[i ^ 1u];

        if ((rgba & 1u) == 0u)
        {
            continue;
        }
        if (used >= 16u)
        {
            return FALSE;
        }
        remap[i] = (u8)used;
        colors[used] = (u16)(((rgba >> 11) & 0x1fu) |
                             (((rgba >> 6) & 0x1fu) << 5) |
                             (((rgba >> 1) & 0x1fu) << 10));
        used++;
    }
    if (used > 16u)
    {
        return FALSE;
    }
    memset(cell, 0, sizeof(cell));
    row_bytes = (width_img + 1u) / 2u;
    for (y = 0u; y < (u32)(u16)sprite->height; y++)
    {
        u32 x;

        for (x = 0u; x < (u32)(u16)sprite->width; x++)
        {
            /* SP_TEXSHUF: odd rows swap 8-texel halves (the item arrow's
             * reader, ndsIFCommonReadItemArrowI4). */
            u32 shuffled_x = x ^ (((y & 1u) != 0u) ? 8u : 0u);
            u8 packed;
            u8 index;
            u32 offset;

            if (shuffled_x >= width_img)
            {
                return FALSE;
            }
            packed = pixels[((y * row_bytes) + (shuffled_x >> 1)) ^ 3u];
            index = remap[((shuffled_x & 1u) == 0u) ?
                          (packed >> 4) : (packed & 0x0fu)];
            if (index == 0u)
            {
                continue;
            }
            offset = ((y >> 3) * 2u + (x >> 3)) * 32u +
                     ((y & 7u) * 4u) + ((x & 7u) >> 1);
            if ((x & 1u) == 0u)
            {
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) | index);
            }
            else
            {
                cell_bytes[offset] =
                    (u8)((cell_bytes[offset] & 0x0fu) | (index << 4));
            }
        }
    }
    dst = (u32 *)(void *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_TASK_BANK_BASE);
    for (i = 0u; i < (u32)(sizeof(cell) / sizeof(cell[0])); i++)
    {
        dst[i] = cell[i];
    }
    for (i = 0u; i < 16u; i++)
    {
        SPRITE_PALETTE[NDS_IFCOMMON_TASK_PALETTE * 16u + i] = colors[i];
    }
    sNdsIFCommonTaskBitmap = sprite->bitmap;
    return TRUE;
}

static s32 ndsIFCommonIsBonusTaskGObj(struct GObj *gobj)
{
    if ((gSCManagerSceneData.scene_curr != nSCKind1PBonusStage) ||
        (gSCManagerBattleState == NULL) || (gobj == NULL))
    {
        return FALSE;
    }
    return (gobj == ((ndsGrBonusClass(gSCManagerBattleState->gkind) == 2u) ?
                     gGRCommonStruct.bonus2.interface_gobj :
                     gGRCommonStruct.bonus1.interface_gobj)) ? TRUE : FALSE;
}

static s32 ndsIFCommonEmitBonusTasks(struct GObj *gobj)
{
    u32 scale_q16 = NDS_IFCOMMON_SCREEN_SCALE_Q16;
    s32 matrix_index = -1;
    SObj *sobj;

    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        s32 origin_x;
        s32 origin_y;
        s32 x;
        s32 y;

        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        if ((sobj->sprite.scalex != 1.0F) || (sobj->sprite.scaley != 1.0F) ||
            (ndsIFCommonBakeTaskIcon(&sobj->sprite) == FALSE))
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
        }
        if (sNdsIFCommonNextOamID < 0)
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
        }
        if (matrix_index < 0)
        {
            matrix_index = ndsIFCommonMatrixForScale((u16)(
                ((1u << 24) + (scale_q16 / 2u)) / scale_q16));
            if (matrix_index < 0)
            {
                return ndsIFCommonItemArrowMiss(
                    nNDSIFCommonFallbackMatrixLimit);
            }
        }
        origin_x = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
            sobj->pos.x, NDS_IFCOMMON_SCREEN_SCALE_Q16));
        origin_y = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
            sobj->pos.y, NDS_IFCOMMON_SCREEN_SCALE_Q16));
        x = origin_x + ndsIFCommonRoundQ16HalfUp(
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u) * (s32)scale_q16) -
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u);
        y = origin_y + ndsIFCommonRoundQ16HalfUp(
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u) * (s32)scale_q16) -
            (s32)(NDS_IFCOMMON_TASK_SIZE / 2u);
        oamSet(&oamMain, sNdsIFCommonNextOamID, x, y, 0,
               NDS_IFCOMMON_TASK_PALETTE,
               SpriteSize_16x16, SpriteColorFormat_16Color,
               (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_TASK_BANK_BASE),
               matrix_index, false, false, false, false, false);
        sNdsIFCommonNextOamID--;
        sNdsIFCommonFrameNeedsCommit = TRUE;
        gNdsIFCommonNativeOamFrameObjectCount++;
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
    return TRUE;
}
#endif

/* The IFCommonAnnounceCommon messages (ifcommon.c dIFCommonAnnounce*
 * SpriteData: SUDDEN DEATH!, FAILURE, COMPLETE!): IA8 letters with SP_TEXSHUF
 * | SP_TRANSPARENT, prim and env set per message by ifCommonAnnounceSetColors.
 * Each distinct letter is decoded once at the screen's 0.8 grid into a 32x32
 * 4bpp cell in the end bank -- these messages never share the screen with
 * TIME UP or GAME SET, and whichever comes next re-lays the bank -- and the
 * message's ramp (env at intensity 0 to prim at 15, the IA combine) is
 * palette 8. */
#define NDS_IFCOMMON_COMMON_LETTER_MAX 12u
#define NDS_IFCOMMON_COMMON_LETTER_CELL 32u
#define NDS_IFCOMMON_COMMON_LETTER_BYTES 512u
#define NDS_IFCOMMON_COMMON_PALETTE 8u
#define NDS_IFCOMMON_ASSET_ANNOUNCE_COMMON 37u

typedef struct NDSIFCommonCommonLetter
{
    const Bitmap *bitmap;
    u16 *gfx;
} NDSIFCommonCommonLetter;

static NDSIFCommonCommonLetter
    sNdsIFCommonCommonLetters[NDS_IFCOMMON_COMMON_LETTER_MAX];
static u32 sNdsIFCommonCommonLetterCount;
/* The ramp palette 8 holds: the message's prim and env as 0xRRGGBB. */
static u32 sNdsIFCommonCommonPalettePrim;
static u32 sNdsIFCommonCommonPaletteEnv;
static u32 sNdsIFCommonCommonPaletteValid;

/* Owner r71: "Failure" and "Complete" did not draw correctly. The letter
 * cells and palette 8 were cached for the life of the ROM, but both are
 * shared hardware: TIME UP / GAME SET re-lay the end bank under the cached
 * cells, and other scenes' OBJ presenters (source 2D, results) rewrite OBJ
 * VRAM and the palette, so a later message drew from whatever was there --
 * and COMPLETE! plus FAILURE need 13 distinct letters against the 12-cell
 * cache. The cache now lives for one battle scene and one end-bank layout. */
static void ndsIFCommonResetPause(void);

static void ndsIFCommonResetCommonLetters(void)
{
    memset(sNdsIFCommonCommonLetters, 0, sizeof(sNdsIFCommonCommonLetters));
    sNdsIFCommonCommonLetterCount = 0u;
    sNdsIFCommonCommonPaletteValid = FALSE;
    /* The pause cells share the end bank's base. */
    ndsIFCommonResetPause();
}

static s32 ndsIFCommonIsCommonLetter(const SObj *sobj)
{
    u32 asset_id;
    u32 offset;

    return ((sobj->sprite.bitmap != NULL) &&
            (sobj->sprite.bmfmt == G_IM_FMT_IA) &&
            (sobj->sprite.bmsiz == G_IM_SIZ_8b) &&
            (ndsRelocGetLoadedPointerProvenance(sobj->sprite.bitmap,
                                                &asset_id, &offset) != 0) &&
            (asset_id == NDS_IFCOMMON_ASSET_ANNOUNCE_COMMON)) ? TRUE : FALSE;
}

static u16 *ndsIFCommonBakeCommonLetter(const Sprite *sprite)
{
    u32 cell[NDS_IFCOMMON_COMMON_LETTER_BYTES / sizeof(u32)];
    u8 *cell_bytes = (u8 *)cell;
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;
    u32 ds_width;
    u32 ds_height;
    u16 *gfx;
    u32 *dst;
    u32 slot;
    u32 y;

    for (slot = 0u; slot < sNdsIFCommonCommonLetterCount; slot++)
    {
        if (sNdsIFCommonCommonLetters[slot].bitmap == sprite->bitmap)
        {
            return sNdsIFCommonCommonLetters[slot].gfx;
        }
    }
    if (sNdsIFCommonCommonLetterCount >= NDS_IFCOMMON_COMMON_LETTER_MAX)
    {
        return NULL;
    }
    ds_width = (width * 4u + 2u) / 5u;
    ds_height = (height * 4u + 2u) / 5u;
    if ((width == 0u) || (height == 0u) ||
        (ds_width > NDS_IFCOMMON_COMMON_LETTER_CELL) ||
        (ds_height > NDS_IFCOMMON_COMMON_LETTER_CELL))
    {
        return NULL;
    }
    if (sNdsIFCommonCommonLetterCount == 0u)
    {
        /* The end bank is about to hold these letters: TIME UP / GAME SET
         * must be laid out again before either shows, and the pause cells
         * baked again. */
        sNdsIFCommonAnnounceActive = FALSE;
        ndsIFCommonResetPause();
    }
    memset(cell, 0, sizeof(cell));
    for (y = 0u; y < ds_height; y++)
    {
        u32 source_y = ((2u * y + 1u) * height) / (2u * ds_height);
        u32 x;

        for (x = 0u; x < ds_width; x++)
        {
            u32 source_x = ((2u * x + 1u) * width) / (2u * ds_width);
            u32 offset = (((y >> 3) * (NDS_IFCOMMON_COMMON_LETTER_CELL >> 3)) +
                          (x >> 3)) * 32u + ((y & 7u) * 4u) + ((x & 7u) >> 1);
            u8 ia;
            u8 index;

            if (ndsIFCommonReadTagI8(sprite, source_x, source_y, &ia) == FALSE)
            {
                return NULL;
            }
            /* The player tags' IA rule: alpha nibble >= 8 is kept, the
             * intensity nibble picks the ramp entry, 0 stays transparent. */
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
                cell_bytes[offset] = (u8)((cell_bytes[offset] & 0xf0u) | index);
            }
            else
            {
                cell_bytes[offset] =
                    (u8)((cell_bytes[offset] & 0x0fu) | (u8)(index << 4));
            }
        }
    }
    gfx = (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_END_BANK_BASE +
                  sNdsIFCommonCommonLetterCount *
                      NDS_IFCOMMON_COMMON_LETTER_BYTES);
    dst = (u32 *)(void *)gfx;
    for (y = 0u; y < (u32)(sizeof(cell) / sizeof(cell[0])); y++)
    {
        dst[y] = cell[y];
    }
    sNdsIFCommonCommonLetters[sNdsIFCommonCommonLetterCount].bitmap =
        sprite->bitmap;
    sNdsIFCommonCommonLetters[sNdsIFCommonCommonLetterCount].gfx = gfx;
    sNdsIFCommonCommonLetterCount++;
    return gfx;
}

static void ndsIFCommonCommonPalette(const SObj *sobj)
{
    /* Prim and env compared whole: the old single-word key folded them with
     * a mis-parenthesised XOR, and FAILURE's (white over blue) came out as
     * 0xffffffff -- the never-written sentinel -- so its ramp was never
     * written at all. */
    u32 prim = ((u32)sobj->sprite.red << 16) |
               ((u32)sobj->sprite.green << 8) | (u32)sobj->sprite.blue;
    u32 env = ((u32)sobj->envcolor.r << 16) |
              ((u32)sobj->envcolor.g << 8) | (u32)sobj->envcolor.b;
    u16 *palette = &SPRITE_PALETTE[NDS_IFCOMMON_COMMON_PALETTE * 16u];
    u32 i;

    if ((sNdsIFCommonCommonPaletteValid != FALSE) &&
        (prim == sNdsIFCommonCommonPalettePrim) &&
        (env == sNdsIFCommonCommonPaletteEnv))
    {
        return;
    }
    sNdsIFCommonCommonPalettePrim = prim;
    sNdsIFCommonCommonPaletteEnv = env;
    sNdsIFCommonCommonPaletteValid = TRUE;
    palette[0] = 0u;
    for (i = 1u; i < 16u; i++)
    {
        /* Entry i is intensity nibble i (entry 1 also holds intensity 0). */
        u32 t = (i == 1u) ? 0u : i;
        u32 red = sobj->envcolor.r +
            ndsIFCommonDiv15((u32)((s32)sobj->sprite.red -
                                   (s32)sobj->envcolor.r + 255) * t) -
            ndsIFCommonDiv15(255u * t);
        u32 green = sobj->envcolor.g +
            ndsIFCommonDiv15((u32)((s32)sobj->sprite.green -
                                   (s32)sobj->envcolor.g + 255) * t) -
            ndsIFCommonDiv15(255u * t);
        u32 blue = sobj->envcolor.b +
            ndsIFCommonDiv15((u32)((s32)sobj->sprite.blue -
                                   (s32)sobj->envcolor.b + 255) * t) -
            ndsIFCommonDiv15(255u * t);

        palette[i] = ndsIFCommonPackRgb15((u8)red, (u8)green, (u8)blue);
    }
}

static s32 ndsIFCommonEmitCommonLetters(struct GObj *gobj)
{
    SObj *sobj;

    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        u16 *gfx;

        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        if ((ndsIFCommonIsCommonLetter(sobj) == FALSE) ||
            NDS_FCMP_NE_C(sobj->sprite.scalex, 1.0F) ||
            NDS_FCMP_NE_C(sobj->sprite.scaley, 1.0F))
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackBadAsset);
        }
        gfx = ndsIFCommonBakeCommonLetter(&sobj->sprite);
        if (gfx == NULL)
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
        }
        if (sNdsIFCommonNextOamID < 0)
        {
            return ndsIFCommonItemArrowMiss(nNDSIFCommonFallbackObjectLimit);
        }
        ndsIFCommonCommonPalette(sobj);
        oamSet(&oamMain, sNdsIFCommonNextOamID,
               ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
                   sobj->pos.x, NDS_IFCOMMON_SCREEN_SCALE_Q16)),
               ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
                   sobj->pos.y, NDS_IFCOMMON_SCREEN_SCALE_Q16)), 0,
               NDS_IFCOMMON_COMMON_PALETTE,
               SpriteSize_32x32, SpriteColorFormat_16Color, gfx, -1,
               false, false, false, false, false);
        sNdsIFCommonNextOamID--;
        sNdsIFCommonFrameNeedsCommit = TRUE;
        gNdsIFCommonNativeOamFrameObjectCount++;
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
    return TRUE;
}

/* ---- The pause menu (ifcommon.c ifCommonBattlePauseMakeInterface) -------
 * The pausing player's number and the control decals are I4 and IA8 Sprites
 * tinted env-to-prim like the announce letters. They bake once into the end
 * bank as 16-colour cells at the 0.8 DS grid, one ramp palette per prim/env
 * pair in OBJ palettes 2-7 (no battle OBJ presenter uses 0-7); the
 * announcement and the announce letters lay the bank out again before they
 * show, and taking the bank resets this cache. The border frame
 * (ifCommonBattlePauseProcDisplay's five fill rectangles) is 2-pixel runs of
 * white cells. */
#define NDS_IFCOMMON_PAUSE_CELLS 16u
#define NDS_IFCOMMON_PAUSE_PALETTE_FIRST 2u
#define NDS_IFCOMMON_PAUSE_PALETTES 6u
#define NDS_IFCOMMON_PAUSE_CELL_MAX_BYTES 2048u    /* a 64x64 4bpp cell */

typedef struct NDSIFCommonPauseCell
{
    const Bitmap *bitmap;
    u32 prim;
    u32 env;
    u16 *gfx;
    SpriteSize size;
    u8 palette;
} NDSIFCommonPauseCell;

static NDSIFCommonPauseCell sNdsIFCommonPauseCells[NDS_IFCOMMON_PAUSE_CELLS];
static u32 sNdsIFCommonPauseCellCount;
static u32 sNdsIFCommonPauseBankUsed;
static u32 sNdsIFCommonPausePalettePrim[NDS_IFCOMMON_PAUSE_PALETTES];
static u32 sNdsIFCommonPausePaletteEnv[NDS_IFCOMMON_PAUSE_PALETTES];
static u32 sNdsIFCommonPausePaletteCount;
/* The border's white runs: 32x8 / 8x8 with rows 0-1 set, 8x32 / 8x8 with
 * columns 0-1 set (the longest thin OBJ shapes), in the white-over-black
 * ramp's entry 15. */
static u16 *sNdsIFCommonPauseRunH32;
static u16 *sNdsIFCommonPauseRunH8;
static u16 *sNdsIFCommonPauseRunV32;
static u16 *sNdsIFCommonPauseRunV8;
static u8 sNdsIFCommonPauseRunPalette;
static u32 sNdsIFCommonPauseOwnsBank;
static u32 sNdsIFCommonPauseCellBuffer[NDS_IFCOMMON_PAUSE_CELL_MAX_BYTES / 4u];
volatile u32 gNdsIFCommonPauseMisses;

static void ndsIFCommonResetPause(void)
{
    sNdsIFCommonPauseCellCount = 0u;
    sNdsIFCommonPauseBankUsed = 0u;
    sNdsIFCommonPausePaletteCount = 0u;
    sNdsIFCommonPauseRunH32 = NULL;
    sNdsIFCommonPauseRunH8 = NULL;
    sNdsIFCommonPauseRunV32 = NULL;
    sNdsIFCommonPauseRunV8 = NULL;
    sNdsIFCommonPauseOwnsBank = FALSE;
}

/* The first pause cell takes the end bank from the letters and the
 * announcement, which lay it out again before they show. */
static void ndsIFCommonPauseTakeBank(void)
{
    if (sNdsIFCommonPauseOwnsBank == FALSE)
    {
        ndsIFCommonResetCommonLetters();
        sNdsIFCommonAnnounceActive = FALSE;
        sNdsIFCommonPauseOwnsBank = TRUE;
    }
}

/* The end bank from its base, 128-byte granules (SpriteMapping_Bmp_1D_128). */
static u16 *ndsIFCommonPauseAlloc(u32 bytes)
{
    u32 at;

    at = (sNdsIFCommonPauseBankUsed + 127u) & ~127u;
    if ((at + bytes) > NDS_IFCOMMON_END_BANK_BYTES)
    {
        return NULL;
    }
    sNdsIFCommonPauseBankUsed = at + bytes;
    return (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_END_BANK_BASE + at);
}

/* The ramp palette for one prim/env pair (the announce letters' ramp). */
static s32 ndsIFCommonPausePalette(u32 prim, u32 env)
{
    u16 *palette;
    u32 slot;
    u32 i;

    for (slot = 0u; slot < sNdsIFCommonPausePaletteCount; slot++)
    {
        if ((sNdsIFCommonPausePalettePrim[slot] == prim) &&
            (sNdsIFCommonPausePaletteEnv[slot] == env))
        {
            return (s32)(NDS_IFCOMMON_PAUSE_PALETTE_FIRST + slot);
        }
    }
    if (sNdsIFCommonPausePaletteCount >= NDS_IFCOMMON_PAUSE_PALETTES)
    {
        return -1;
    }
    slot = sNdsIFCommonPausePaletteCount++;
    sNdsIFCommonPausePalettePrim[slot] = prim;
    sNdsIFCommonPausePaletteEnv[slot] = env;
    palette = &SPRITE_PALETTE[(NDS_IFCOMMON_PAUSE_PALETTE_FIRST + slot) * 16u];
    palette[0] = 0u;
    for (i = 1u; i < 16u; i++)
    {
        u32 t = (i == 1u) ? 0u : i;
        u32 channel[3];
        u32 c;

        for (c = 0u; c < 3u; c++)
        {
            u32 p = (prim >> (16u - 8u * c)) & 0xffu;
            u32 e = (env >> (16u - 8u * c)) & 0xffu;

            channel[c] = e + ndsIFCommonDiv15((u32)((s32)p - (s32)e + 255) * t) -
                         ndsIFCommonDiv15(255u * t);
        }
        palette[i] = ndsIFCommonPackRgb15((u8)channel[0], (u8)channel[1],
                                          (u8)channel[2]);
    }
    return (s32)(NDS_IFCOMMON_PAUSE_PALETTE_FIRST + slot);
}

/* The smallest legal OBJ shape holding w x h. */
static s32 ndsIFCommonPauseShape(u32 w, u32 h, SpriteSize *size, u32 *cw, u32 *ch)
{
    static const struct
    {
        u8 w;
        u8 h;
        SpriteSize size;
    } kShapes[] = {
        { 8, 8, SpriteSize_8x8 },     { 16, 8, SpriteSize_16x8 },
        { 8, 16, SpriteSize_8x16 },   { 16, 16, SpriteSize_16x16 },
        { 32, 8, SpriteSize_32x8 },   { 8, 32, SpriteSize_8x32 },
        { 32, 16, SpriteSize_32x16 }, { 16, 32, SpriteSize_16x32 },
        { 32, 32, SpriteSize_32x32 }, { 64, 32, SpriteSize_64x32 },
        { 32, 64, SpriteSize_32x64 }, { 64, 64, SpriteSize_64x64 },
    };
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(kShapes); i++)
    {
        if ((w <= kShapes[i].w) && (h <= kShapes[i].h))
        {
            *size = kShapes[i].size;
            *cw = kShapes[i].w;
            *ch = kShapes[i].h;
            return TRUE;
        }
    }
    return FALSE;
}

/* A built cell into OBJ VRAM, a word at a time (VRAM takes no byte stores). */
static u16 *ndsIFCommonPauseUpload(u32 cw, u32 ch)
{
    u32 bytes = (cw * ch) / 2u;
    u16 *gfx = ndsIFCommonPauseAlloc(bytes);
    u32 *dst;
    u32 i;

    if (gfx == NULL)
    {
        return NULL;
    }
    dst = (u32 *)(void *)gfx;
    for (i = 0u; i < bytes / 4u; i++)
    {
        dst[i] = sNdsIFCommonPauseCellBuffer[i];
    }
    return gfx;
}

static void ndsIFCommonPauseSetTexel(u32 cw, u32 x, u32 y, u8 index)
{
    u8 *cell = (u8 *)sNdsIFCommonPauseCellBuffer;
    u32 offset = ((((y >> 3) * (cw >> 3)) + (x >> 3)) * 32u) +
                 ((y & 7u) * 4u) + ((x & 7u) >> 1);

    cell[offset] = ((x & 1u) == 0u) ?
        (u8)((cell[offset] & 0xf0u) | index) :
        (u8)((cell[offset] & 0x0fu) | (u8)(index << 4));
}

static const NDSIFCommonPauseCell *ndsIFCommonBakePauseCell(const SObj *sobj)
{
    const Sprite *sprite = &sobj->sprite;
    u32 prim = ((u32)sprite->red << 16) | ((u32)sprite->green << 8) |
               (u32)sprite->blue;
    u32 env = ((u32)sobj->envcolor.r << 16) | ((u32)sobj->envcolor.g << 8) |
              (u32)sobj->envcolor.b;
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;
    u32 ds_width = (width * 4u + 2u) / 5u;
    u32 ds_height = (height * 4u + 2u) / 5u;
    NDSIFCommonPauseCell *cell;
    SpriteSize size;
    s32 palette;
    u32 cw;
    u32 ch;
    u32 x;
    u32 y;
    u32 i;

    for (i = 0u; i < sNdsIFCommonPauseCellCount; i++)
    {
        cell = &sNdsIFCommonPauseCells[i];
        if ((cell->bitmap == sprite->bitmap) && (cell->prim == prim) &&
            (cell->env == env))
        {
            return cell;
        }
    }
    ndsIFCommonPauseTakeBank();
    if ((sNdsIFCommonPauseCellCount >= NDS_IFCOMMON_PAUSE_CELLS) ||
        (sprite->bitmap == NULL) || (sprite->bitmap->buf == NULL) ||
        (width == 0u) || (height == 0u) ||
        !(((sprite->bmfmt == G_IM_FMT_IA) && (sprite->bmsiz == G_IM_SIZ_8b)) ||
          ((sprite->bmfmt == G_IM_FMT_I) && (sprite->bmsiz == G_IM_SIZ_4b))) ||
        (ndsIFCommonPauseShape(ds_width, ds_height, &size, &cw, &ch) == FALSE))
    {
        return NULL;
    }
    palette = ndsIFCommonPausePalette(prim, env);
    if (palette < 0)
    {
        return NULL;
    }
    memset(sNdsIFCommonPauseCellBuffer, 0, (cw * ch) / 2u);
    for (y = 0u; y < ds_height; y++)
    {
        u32 source_y = ((2u * y + 1u) * height) / (2u * ds_height);

        for (x = 0u; x < ds_width; x++)
        {
            u32 source_x = ((2u * x + 1u) * width) / (2u * ds_width);
            u8 index;

            if (sprite->bmfmt == G_IM_FMT_IA)
            {
                u8 ia;

                /* The announce letters' rule: alpha nibble 8 and up kept,
                 * the intensity nibble picks the ramp entry. */
                if ((ndsIFCommonReadTagI8(sprite, source_x, source_y, &ia) == FALSE) ||
                    ((ia & 0x0fu) < 8u))
                {
                    continue;
                }
                index = (u8)(ia >> 4);
            }
            else
            {
                const Bitmap *bitmap = sprite->bitmap;
                u32 width_img = ((u32)(u16)bitmap->width_img != 0u) ?
                    (u32)(u16)bitmap->width_img : (u32)(u16)bitmap->width;
                u8 intensity;

                /* I4 is its own alpha: half and up kept. */
                if ((ndsIFCommonReadI4(sprite, bitmap->buf,
                                       (size_t)((width_img + 1u) / 2u) * height,
                                       source_x, source_y, &intensity) == FALSE) ||
                    (intensity < 8u))
                {
                    continue;
                }
                index = intensity;
            }
            ndsIFCommonPauseSetTexel(cw, x, y, (index == 0u) ? 1u : index);
        }
    }
    cell = &sNdsIFCommonPauseCells[sNdsIFCommonPauseCellCount];
    cell->gfx = ndsIFCommonPauseUpload(cw, ch);
    if (cell->gfx == NULL)
    {
        return NULL;
    }
    cell->bitmap = sprite->bitmap;
    cell->prim = prim;
    cell->env = env;
    cell->size = size;
    cell->palette = (u8)palette;
    sNdsIFCommonPauseCellCount++;
    return cell;
}

static s32 ndsIFCommonPauseRuns(void)
{
    s32 palette;
    u32 i;

    if (sNdsIFCommonPauseRunH32 != NULL)
    {
        return TRUE;
    }
    ndsIFCommonPauseTakeBank();
    palette = ndsIFCommonPausePalette(0xffffffu, 0u);
    if (palette < 0)
    {
        return FALSE;
    }
    sNdsIFCommonPauseRunPalette = (u8)palette;
    memset(sNdsIFCommonPauseCellBuffer, 0, sizeof(sNdsIFCommonPauseCellBuffer));
    for (i = 0u; i < 32u; i++)
    {
        ndsIFCommonPauseSetTexel(32u, i, 0u, 15u);
        ndsIFCommonPauseSetTexel(32u, i, 1u, 15u);
    }
    sNdsIFCommonPauseRunH32 = ndsIFCommonPauseUpload(32u, 8u);
    memset(sNdsIFCommonPauseCellBuffer, 0, sizeof(sNdsIFCommonPauseCellBuffer));
    for (i = 0u; i < 8u; i++)
    {
        ndsIFCommonPauseSetTexel(8u, i, 0u, 15u);
        ndsIFCommonPauseSetTexel(8u, i, 1u, 15u);
    }
    sNdsIFCommonPauseRunH8 = ndsIFCommonPauseUpload(8u, 8u);
    memset(sNdsIFCommonPauseCellBuffer, 0, sizeof(sNdsIFCommonPauseCellBuffer));
    for (i = 0u; i < 32u; i++)
    {
        ndsIFCommonPauseSetTexel(8u, 0u, i, 15u);
        ndsIFCommonPauseSetTexel(8u, 1u, i, 15u);
    }
    sNdsIFCommonPauseRunV32 = ndsIFCommonPauseUpload(8u, 32u);
    memset(sNdsIFCommonPauseCellBuffer, 0, sizeof(sNdsIFCommonPauseCellBuffer));
    for (i = 0u; i < 8u; i++)
    {
        ndsIFCommonPauseSetTexel(8u, 0u, i, 15u);
        ndsIFCommonPauseSetTexel(8u, 1u, i, 15u);
    }
    sNdsIFCommonPauseRunV8 = ndsIFCommonPauseUpload(8u, 8u);
    return ((sNdsIFCommonPauseRunH32 != NULL) && (sNdsIFCommonPauseRunH8 != NULL) &&
            (sNdsIFCommonPauseRunV32 != NULL) && (sNdsIFCommonPauseRunV8 != NULL)) ?
        TRUE : FALSE;
}

static s32 ndsIFCommonPauseObject(s32 x, s32 y, SpriteSize size, u32 palette,
                                  u16 *gfx)
{
    if (sNdsIFCommonNextOamID < 0)
    {
        return FALSE;
    }
    oamSet(&oamMain, sNdsIFCommonNextOamID, x, y, 0, (int)palette, size,
           SpriteColorFormat_16Color, gfx, -1, false, false, false, false, false);
    sNdsIFCommonNextOamID--;
    gNdsIFCommonNativeOamFrameObjectCount++;
    sNdsIFCommonFrameNeedsCommit = TRUE;
    return TRUE;
}

/* One run of the frame: 32-long pieces, then 8-long pieces, the last one
 * ending flush with the run. */
static s32 ndsIFCommonPauseRun(s32 x0, s32 y0, s32 length, sb32 vertical)
{
    s32 at = 0;

    while ((length - at) >= 32)
    {
        if (ndsIFCommonPauseObject(vertical ? x0 : (x0 + at), vertical ? (y0 + at) : y0,
                                   vertical ? SpriteSize_8x32 : SpriteSize_32x8,
                                   sNdsIFCommonPauseRunPalette,
                                   vertical ? sNdsIFCommonPauseRunV32 :
                                              sNdsIFCommonPauseRunH32) == FALSE)
        {
            return FALSE;
        }
        at += 32;
    }
    while (at < length)
    {
        s32 piece = ((length - at) >= 8) ? at : (length - 8);

        if (piece < 0)
        {
            piece = 0;
        }
        if (ndsIFCommonPauseObject(vertical ? x0 : (x0 + piece), vertical ? (y0 + piece) : y0,
                                   SpriteSize_8x8, sNdsIFCommonPauseRunPalette,
                                   vertical ? sNdsIFCommonPauseRunV8 :
                                              sNdsIFCommonPauseRunH8) == FALSE)
        {
            return FALSE;
        }
        at += 8;
    }
    return TRUE;
}

/* ifCommonBattlePauseProcDisplay's five fill rectangles (ifcommon.c
 * dIFCommonBattlePauseBorderRectangle), corners inclusive as fill mode
 * draws them, at the 0.8 grid. */
void ndsIFCommonNativeOamPauseBorder(void)
{
    static const s16 kRects[5][4] = {
        { 26, 24, 294, 26 }, { 26, 24, 28, 199 }, { 26, 197, 190, 199 },
        { 292, 24, 294, 199 }, { 279, 197, 294, 199 },
    };
    u32 i;

    if (ndsIFCommonPauseRuns() == FALSE)
    {
        gNdsIFCommonPauseMisses++;
        return;
    }
    for (i = 0u; i < ARRAY_COUNT(kRects); i++)
    {
        s32 x0 = (kRects[i][0] * 4 + 2) / 5;
        s32 y0 = (kRects[i][1] * 4 + 2) / 5;
        s32 x1 = ((kRects[i][2] + 1) * 4 + 2) / 5;
        s32 y1 = ((kRects[i][3] + 1) * 4 + 2) / 5;
        sb32 vertical = ((y1 - y0) > (x1 - x0)) ? TRUE : FALSE;

        if (ndsIFCommonPauseRun(x0, y0, vertical ? (y1 - y0) : (x1 - x0),
                                vertical) == FALSE)
        {
            gNdsIFCommonPauseMisses++;
            return;
        }
    }
}

static s32 ndsIFCommonEmitPause(struct GObj *gobj)
{
    SObj *sobj;

    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        const NDSIFCommonPauseCell *cell;

        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        cell = ndsIFCommonBakePauseCell(sobj);
        if ((cell == NULL) ||
            (ndsIFCommonPauseObject(
                 ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
                     sobj->pos.x, NDS_IFCOMMON_SCREEN_SCALE_Q16)),
                 ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
                     sobj->pos.y, NDS_IFCOMMON_SCREEN_SCALE_Q16)),
                 cell->size, cell->palette, cell->gfx) == FALSE))
        {
            gNdsIFCommonPauseMisses++;
            return FALSE;
        }
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
    return TRUE;
}

#if NDS_P2_1P_GAME
/* ---- Training (P7 C4, 2026-10-19) ---------------------------------------
 * sc1ptrainingmode.c's displays and menu: the stat labels and values
 * (DAMAGE, COMBO, ENEMY, SPEED) and, with the menu open, its labels, options,
 * arrows, cursor and the box's four CI4 edges. Text is IA8 or I4 tinted
 * env-to-prim (the pause decals' rule); the edges carry their own LUT. Each
 * distinct sprite and tint bakes once at the 0.8 grid into 16-colour cells,
 * a sprite wider than one 64-pixel OBJ in 64-pixel strips.
 *
 * Training has no countdown, no TIME UP or GAME SET and no common pause (its
 * START is the training menu), so the GO and end banks -- 38,144 B from the
 * OBJ base -- are its own while it runs, and so are the palettes only those
 * would use (the pause's 2-7, the announce letters' 8, the bonus task's 9)
 * with the three no battle presenter writes (0, 1, 15). The cell table
 * lives on the scene heap (one allocation per Training scene, keyed by the
 * heap generation), so no other scene pays for it.
 *
 * The menu's box (sc1PTrainingModeMenuProcDisplay: PRIM 00/64/FF at alpha
 * 0x64 under G_RM_CLD_SURF, (68,47)-(253,198)) and the cursor's red
 * underline are rectangles the Training TU hands over
 * (ndsIFCommonTrainingFillRect): the underline solid 16-colour runs, the box
 * 32x32 bitmap OBJs at alpha 6/16 blended over the 3D layer the way the
 * Results presenter blends (BLDCNT gains BG0 and the backdrop as second
 * targets while the box shows). */
#define NDS_IFCOMMON_TRAINING_CELLS 64u
#define NDS_IFCOMMON_TRAINING_BANK_BYTES \
    (NDS_IFCOMMON_END_BANK_BASE + NDS_IFCOMMON_END_BANK_BYTES)
#define NDS_IFCOMMON_TRAINING_STRIP 64u
#define NDS_IFCOMMON_TRAINING_BOX_CELL 32u

typedef struct NDSIFCommonTrainingCell
{
    const Bitmap *bitmap;
    u32 prim;
    u32 env;
    u16 *gfx;
    u16 size;       /* SpriteSize: shape, size and tile count in 16 bits */
    u8 strip;       /* the 64x64 tile: column low nibble, row high */
    u8 palette;     /* a bitmap object's alpha */
    u8 bitmap_obj;
    u8 pad[3];
} NDSIFCommonTrainingCell;

static NDSIFCommonTrainingCell *sNdsIFCommonTrainingCells;
/* Training's palettes: a ramp per prim/env pair or a CI4 LUT per its
 * colours, in the slots listed; past the last, the nearest ramp serves. */
static const u8 kNdsIFCommonTrainingPaletteSlots[] = {
    2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0u, 1u, 15u
};
#define NDS_IFCOMMON_TRAINING_PALETTES \
    ((u32)(sizeof(kNdsIFCommonTrainingPaletteSlots) / \
           sizeof(kNdsIFCommonTrainingPaletteSlots[0])))
static u32 sNdsIFCommonTrainingPaletteKey[NDS_IFCOMMON_TRAINING_PALETTES][2];
static u32 sNdsIFCommonTrainingPaletteCount;

static u32 sNdsIFCommonTrainingGeneration;
static u32 sNdsIFCommonTrainingCellCount;
static u32 sNdsIFCommonTrainingBankUsed;
static u16 *sNdsIFCommonTrainingBox;
static u16 *sNdsIFCommonTrainingRun[8];
static s8 sNdsIFCommonTrainingRunPalette = -1;
static u32 sNdsIFCommonTrainingBlend;
static u16 sNdsIFCommonTrainingSavedBldCnt;
static u32 sNdsIFCommonTrainingBoxFrame;
volatile u32 gNdsIFCommonTrainingMisses;
volatile u32 gNdsIFCommonTrainingObjects;
/* The last refusal: 1 cells full, 2 format, 3 shape, 4 palette, 5 bank. */
volatile u32 gNdsIFCommonTrainingMissWhy;

/* The training sprite file (reloc file 0x1d, staged as SC1P_TRAINING_SPRITES). */
#define NDS_IFCOMMON_ASSET_TRAINING_SPRITES 0x1du

extern volatile u32 gNdsTaskmanHeapGeneration;
sb32 ndsSyMallocWouldFit(const SYMallocRegion *bp, size_t size, u32 alignment);

/* A new Training scene: the table on its heap, the banks and palettes from
 * empty. FALSE when the heap cannot spare the table. */
static s32 ndsIFCommonTrainingBegin(void)
{
    const size_t bytes =
        NDS_IFCOMMON_TRAINING_CELLS * sizeof(NDSIFCommonTrainingCell);

    if ((sNdsIFCommonTrainingCells != NULL) &&
        (sNdsIFCommonTrainingGeneration == gNdsTaskmanHeapGeneration))
    {
        return TRUE;
    }
    sNdsIFCommonTrainingCells = NULL;
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap, bytes, 0x4u) == FALSE)
    {
        return FALSE;
    }
    sNdsIFCommonTrainingCells = syTaskmanMalloc(bytes, 0x4u);
    sNdsIFCommonTrainingGeneration = gNdsTaskmanHeapGeneration;
    sNdsIFCommonTrainingCellCount = 0u;
    sNdsIFCommonTrainingBankUsed = 0u;
    sNdsIFCommonTrainingBox = NULL;
    memset(sNdsIFCommonTrainingRun, 0, sizeof(sNdsIFCommonTrainingRun));
    sNdsIFCommonTrainingRunPalette = -1;
    /* The banks and palettes change hands: the pause cache, the announce
     * letters and the end messages lay theirs out again before they would
     * show, and the bonus task icon bakes its palette again. */
    ndsIFCommonResetCommonLetters();
    sNdsIFCommonAnnounceActive = FALSE;
    sNdsIFCommonPausePaletteCount = 0u;
    sNdsIFCommonTrainingPaletteCount = 0u;
#if NDS_P2_1P_GAME
    sNdsIFCommonTaskBitmap = NULL;
#endif
    return TRUE;
}

/* GO + end banks from the OBJ base, 128-byte granules. */
static u16 *ndsIFCommonTrainingAlloc(u32 bytes)
{
    u32 at = (sNdsIFCommonTrainingBankUsed + 127u) & ~127u;

    if ((at + bytes) > NDS_IFCOMMON_TRAINING_BANK_BYTES)
    {
        return NULL;
    }
    sNdsIFCommonTrainingBankUsed = at + bytes;
    return (u16 *)((u8 *)SPRITE_GFX + at);
}

static u16 *ndsIFCommonTrainingUpload(u32 bytes)
{
    u16 *gfx = ndsIFCommonTrainingAlloc(bytes);
    u32 *dst;
    u32 i;

    if (gfx == NULL)
    {
        return NULL;
    }
    dst = (u32 *)(void *)gfx;
    for (i = 0u; i < bytes / 4u; i++)
    {
        dst[i] = sNdsIFCommonPauseCellBuffer[i];
    }
    return gfx;
}

static u32 ndsIFCommonTrainingColorDistance(u32 a, u32 b)
{
    u32 d = 0u;
    u32 shift;

    for (shift = 0u; shift < 24u; shift += 8u)
    {
        s32 delta = (s32)((a >> shift) & 0xffu) - (s32)((b >> shift) & 0xffu);

        d += (u32)((delta < 0) ? -delta : delta);
    }
    return d;
}

static s32 ndsIFCommonTrainingPalette(u32 prim, u32 env)
{
    u16 *palette;
    u32 best = 0u;
    u32 best_distance = 0xffffffffu;
    u32 slot;
    u32 i;

    for (slot = 0u; slot < sNdsIFCommonTrainingPaletteCount; slot++)
    {
        if ((sNdsIFCommonTrainingPaletteKey[slot][0] == prim) &&
            (sNdsIFCommonTrainingPaletteKey[slot][1] == env))
        {
            return (s32)kNdsIFCommonTrainingPaletteSlots[slot];
        }
    }
    if (sNdsIFCommonTrainingPaletteCount >= NDS_IFCOMMON_TRAINING_PALETTES)
    {
        for (slot = 0u; slot < sNdsIFCommonTrainingPaletteCount; slot++)
        {
            u32 distance;

            if ((sNdsIFCommonTrainingPaletteKey[slot][0] & 0x80000000u) != 0u)
            {
                continue;
            }
            distance = ndsIFCommonTrainingColorDistance(
                           sNdsIFCommonTrainingPaletteKey[slot][0], prim) +
                       ndsIFCommonTrainingColorDistance(
                           sNdsIFCommonTrainingPaletteKey[slot][1], env);
            if (distance < best_distance)
            {
                best_distance = distance;
                best = slot;
            }
        }
        return (best_distance == 0xffffffffu) ? -1 :
            (s32)kNdsIFCommonTrainingPaletteSlots[best];
    }
    slot = sNdsIFCommonTrainingPaletteCount++;
    sNdsIFCommonTrainingPaletteKey[slot][0] = prim;
    sNdsIFCommonTrainingPaletteKey[slot][1] = env;
    palette = &SPRITE_PALETTE[kNdsIFCommonTrainingPaletteSlots[slot] * 16u];
    palette[0] = 0u;
    for (i = 1u; i < 16u; i++)
    {
        u32 t = (i == 1u) ? 0u : i;
        u32 channel[3];
        u32 c;

        for (c = 0u; c < 3u; c++)
        {
            u32 pc = (prim >> (16u - 8u * c)) & 0xffu;
            u32 ec = (env >> (16u - 8u * c)) & 0xffu;

            channel[c] = ec + ndsIFCommonDiv15((u32)((s32)pc - (s32)ec + 255) * t) -
                         ndsIFCommonDiv15(255u * t);
        }
        palette[i] = ndsIFCommonPackRgb15((u8)channel[0], (u8)channel[1],
                                          (u8)channel[2]);
    }
    return (s32)kNdsIFCommonTrainingPaletteSlots[slot];
}

/* A CI4 sprite's LUT as a palette (alpha 0 is the cutout, entry 0), keyed
 * by its colours. */
static s32 ndsIFCommonTrainingLutPalette(const Sprite *sprite, u8 remap[16])
{
    const u16 *lut = (const u16 *)sprite->LUT;
    u32 key = 2166136261u;
    u16 *palette;
    u32 used = 1u;
    u32 slot;
    u32 i;

    /* Keyed by the LUT's colours: the box's four edges carry four copies. */
    for (i = 0u; i < 16u; i++)
    {
        key = (key ^ lut[i]) * 16777619u;
    }
    key |= 0x80000000u;
    memset(remap, 0, 16u);
    for (slot = 0u; slot < sNdsIFCommonTrainingPaletteCount; slot++)
    {
        if ((sNdsIFCommonTrainingPaletteKey[slot][0] == key) &&
            (sNdsIFCommonTrainingPaletteKey[slot][1] == 0xffffffffu))
        {
            break;
        }
    }
    if (slot == sNdsIFCommonTrainingPaletteCount)
    {
        if (sNdsIFCommonTrainingPaletteCount >= NDS_IFCOMMON_TRAINING_PALETTES)
        {
            return -1;
        }
        sNdsIFCommonTrainingPaletteCount++;
        sNdsIFCommonTrainingPaletteKey[slot][0] = key;
        sNdsIFCommonTrainingPaletteKey[slot][1] = 0xffffffffu;
    }
    palette = &SPRITE_PALETTE[kNdsIFCommonTrainingPaletteSlots[slot] * 16u];
    palette[0] = 0u;
    for (i = 0u; i < 16u; i++)
    {
        /* RGBA5551, halfword-swapped within its words (entry i at [i ^ 1]),
         * as the bonus task icon's LUT. */
        u16 rgba = lut[i ^ 1u];

        if ((rgba & 1u) == 0u)
        {
            continue;
        }
        remap[i] = (u8)used;
        palette[used] = (u16)(((rgba >> 11) & 0x1fu) |
                              (((rgba >> 6) & 0x1fu) << 5) |
                              (((rgba >> 1) & 0x1fu) << 10));
        used++;
        if (used > 16u)
        {
            return -1;
        }
    }
    return (s32)kNdsIFCommonTrainingPaletteSlots[slot];
}

/* One CI4 texel (SP_TEXSHUF: odd rows swap 8-texel halves). */
static s32 ndsIFCommonTrainingReadCI4(const Sprite *sprite, u32 x, u32 y,
                                      u8 *index)
{
    const Bitmap *bitmap = sprite->bitmap;
    u32 width_img = ((u32)(u16)bitmap->width_img != 0u) ?
        (u32)(u16)bitmap->width_img : (u32)(u16)bitmap->width;
    u32 row_bytes = (width_img + 1u) / 2u;
    u32 shuffled_x = x ^ (((y & 1u) != 0u) ? 8u : 0u);
    const u8 *pixels = (const u8 *)bitmap->buf;
    u8 packed;

    if ((pixels == NULL) || (shuffled_x >= width_img) ||
        (y >= (u32)(u16)sprite->height))
    {
        return FALSE;
    }
    packed = pixels[((y * row_bytes) + (shuffled_x >> 1)) ^ 3u];
    *index = ((shuffled_x & 1u) == 0u) ? (u8)(packed >> 4) :
                                         (u8)(packed & 0x0fu);
    return TRUE;
}

/* One 64x64 tile of a sprite at the 0.8 grid (cached by bitmap, tint and
 * tile: column in the low nibble, row in the high). RGBA32 (the cursor's
 * sphere) is a bitmap object, its alpha a cutout at half. */
static const NDSIFCommonTrainingCell *ndsIFCommonTrainingBake(
    const SObj *sobj, u32 tile)
{
    const Sprite *sprite = &sobj->sprite;
    u32 prim = ((u32)sprite->red << 16) | ((u32)sprite->green << 8) |
               (u32)sprite->blue;
    u32 env = ((u32)sobj->envcolor.r << 16) | ((u32)sobj->envcolor.g << 8) |
              (u32)sobj->envcolor.b;
    u32 width = (u32)(u16)sprite->width;
    u32 height = (u32)(u16)sprite->height;
    u32 ds_width = (width * 4u + 2u) / 5u;
    u32 ds_height = (height * 4u + 2u) / 5u;
    u32 x0 = (tile & 0x0fu) * NDS_IFCOMMON_TRAINING_STRIP;
    u32 y0 = (tile >> 4) * NDS_IFCOMMON_TRAINING_STRIP;
    u32 tile_width;
    u32 tile_height;
    NDSIFCommonTrainingCell *cell;
    SpriteSize size;
    u8 remap[16];
    s32 palette = 15;
    u32 is_ci = ((sprite->bmfmt == G_IM_FMT_CI) &&
                 (sprite->bmsiz == G_IM_SIZ_4b)) ? TRUE : FALSE;
    u32 is_rgba = ((sprite->bmfmt == G_IM_FMT_RGBA) &&
                   (sprite->bmsiz == G_IM_SIZ_32b)) ? TRUE : FALSE;
    u32 bytes;
    u32 cw;
    u32 ch;
    u32 x;
    u32 y;
    u32 i;

    for (i = 0u; i < sNdsIFCommonTrainingCellCount; i++)
    {
        cell = &sNdsIFCommonTrainingCells[i];
        if ((cell->bitmap == sprite->bitmap) && (cell->strip == tile) &&
            ((is_ci != FALSE) || (is_rgba != FALSE) ||
             ((cell->prim == prim) && (cell->env == env))))
        {
            return cell;
        }
    }
    if (sNdsIFCommonTrainingCellCount >= NDS_IFCOMMON_TRAINING_CELLS)
    {
        gNdsIFCommonTrainingMissWhy = 1u;
        return NULL;
    }
    if ((sprite->bitmap == NULL) || (sprite->bitmap->buf == NULL) ||
        (width == 0u) || (height == 0u) || (x0 >= ds_width) ||
        (y0 >= ds_height) ||
        !(((sprite->bmfmt == G_IM_FMT_IA) && (sprite->bmsiz == G_IM_SIZ_8b)) ||
          ((sprite->bmfmt == G_IM_FMT_I) && (sprite->bmsiz == G_IM_SIZ_4b)) ||
          ((is_ci != FALSE) && (sprite->LUT != NULL)) || (is_rgba != FALSE)))
    {
        gNdsIFCommonTrainingMissWhy = 2u;
        return NULL;
    }
    tile_width = ds_width - x0;
    if (tile_width > NDS_IFCOMMON_TRAINING_STRIP)
    {
        tile_width = NDS_IFCOMMON_TRAINING_STRIP;
    }
    tile_height = ds_height - y0;
    if (tile_height > NDS_IFCOMMON_TRAINING_STRIP)
    {
        tile_height = NDS_IFCOMMON_TRAINING_STRIP;
    }
    if ((ndsIFCommonPauseShape(tile_width, tile_height, &size, &cw, &ch) == FALSE) ||
        ((is_rgba != FALSE) && ((cw * ch * 2u) > sizeof(sNdsIFCommonPauseCellBuffer))))
    {
        gNdsIFCommonTrainingMissWhy = 3u;
        return NULL;
    }
    if (is_rgba == FALSE)
    {
        palette = (is_ci != FALSE) ? ndsIFCommonTrainingLutPalette(sprite, remap) :
                                     ndsIFCommonTrainingPalette(prim, env);
        if (palette < 0)
        {
            gNdsIFCommonTrainingMissWhy = 4u;
            return NULL;
        }
    }
    bytes = (is_rgba != FALSE) ? (cw * ch * 2u) : ((cw * ch) / 2u);
    memset(sNdsIFCommonPauseCellBuffer, 0, bytes);
    for (y = 0u; y < tile_height; y++)
    {
        u32 source_y = ((2u * (y0 + y) + 1u) * height) / (2u * ds_height);

        for (x = 0u; x < tile_width; x++)
        {
            u32 source_x = ((2u * (x0 + x) + 1u) * width) / (2u * ds_width);
            u8 index;

            if (is_rgba != FALSE)
            {
                const Bitmap *bitmap = sprite->bitmap;
                u32 width_img = ((u32)(u16)bitmap->width_img != 0u) ?
                    (u32)(u16)bitmap->width_img : (u32)(u16)bitmap->width;
                u32 rgba;

                if ((ndsIFCommonReadRgba32(sprite, bitmap->buf,
                                           (size_t)width_img * height * 4u,
                                           source_x, source_y, &rgba) == FALSE) ||
                    ((rgba & 0xffu) < 0x80u))
                {
                    continue;
                }
                ((u16 *)(void *)sNdsIFCommonPauseCellBuffer)[(y * cw) + x] =
                    (u16)(ndsIFCommonPackRgb15((u8)(rgba >> 24), (u8)(rgba >> 16),
                                               (u8)(rgba >> 8)) | 0x8000u);
                continue;
            }
            if (is_ci != FALSE)
            {
                u8 texel;

                if (ndsIFCommonTrainingReadCI4(sprite, source_x, source_y,
                                               &texel) == FALSE)
                {
                    continue;
                }
                index = remap[texel];
                if (index == 0u)
                {
                    continue;
                }
            }
            else if (sprite->bmfmt == G_IM_FMT_IA)
            {
                u8 ia;

                if ((ndsIFCommonReadTagI8(sprite, source_x, source_y, &ia) == FALSE) ||
                    ((ia & 0x0fu) < 8u))
                {
                    continue;
                }
                index = (u8)(ia >> 4);
            }
            else
            {
                const Bitmap *bitmap = sprite->bitmap;
                u32 width_img = ((u32)(u16)bitmap->width_img != 0u) ?
                    (u32)(u16)bitmap->width_img : (u32)(u16)bitmap->width;
                u8 intensity;

                if ((ndsIFCommonReadI4(sprite, bitmap->buf,
                                       (size_t)((width_img + 1u) / 2u) * height,
                                       source_x, source_y, &intensity) == FALSE) ||
                    (intensity < 8u))
                {
                    continue;
                }
                index = intensity;
            }
            ndsIFCommonPauseSetTexel(cw, x, y, (index == 0u) ? 1u : index);
        }
    }
    cell = &sNdsIFCommonTrainingCells[sNdsIFCommonTrainingCellCount];
    cell->gfx = ndsIFCommonTrainingUpload(bytes);
    if (cell->gfx == NULL)
    {
        gNdsIFCommonTrainingMissWhy = 5u;
        return NULL;
    }
    cell->bitmap = sprite->bitmap;
    cell->prim = prim;
    cell->env = env;
    cell->strip = (u8)tile;
    cell->size = (u16)size;
    cell->palette = (u8)palette;
    cell->bitmap_obj = (u8)is_rgba;
    sNdsIFCommonTrainingCellCount++;
    return cell;
}

static s32 ndsIFCommonTrainingObject(s32 x, s32 y, SpriteSize size,
                                     u32 palette, u16 *gfx,
                                     SpriteColorFormat format)
{
    if (sNdsIFCommonNextOamID < 0)
    {
        return FALSE;
    }
    oamSet(&oamMain, sNdsIFCommonNextOamID, x, y, 0, (int)palette, size,
           format, gfx, -1, false, false, false, false, false);
    sNdsIFCommonNextOamID--;
    sNdsIFCommonFrameNeedsCommit = TRUE;
    gNdsIFCommonNativeOamFrameObjectCount++;
    gNdsIFCommonTrainingObjects++;
    return TRUE;
}

/* A Training display or menu GObj: its first visible sprite is in the
 * training sprite file (the magnifier, tags and HUD keep their own paths). */
static s32 ndsIFCommonIsTrainingGObj(struct GObj *gobj)
{
    SObj *sobj;
    u32 asset_id;
    u32 offset;

    if ((gSCManagerSceneData.scene_curr != nSCKind1PTrainingMode) ||
        (gobj == NULL) || (gobj->id != nGCCommonKindInterface))
    {
        return FALSE;
    }
    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        return ((sobj->sprite.bitmap != NULL) &&
                (ndsRelocGetLoadedPointerProvenance(sobj->sprite.bitmap,
                                                    &asset_id, &offset) != 0) &&
                (asset_id == NDS_IFCOMMON_ASSET_TRAINING_SPRITES)) ?
            TRUE : FALSE;
    }
    /* Nothing visible: drawn as nothing. */
    return TRUE;
}

static s32 ndsIFCommonEmitTraining(struct GObj *gobj)
{
    SObj *sobj;

    if (ndsIFCommonTrainingBegin() == FALSE)
    {
        gNdsIFCommonTrainingMisses++;
        return FALSE;
    }
    for (sobj = SObjGetStruct(gobj); sobj != NULL; sobj = sobj->next)
    {
        u32 ds_width;
        u32 ds_height;
        s32 x;
        s32 y;
        u32 tx;
        u32 ty;

        if ((sobj->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        ds_width = ((u32)(u16)sobj->sprite.width * 4u + 2u) / 5u;
        ds_height = ((u32)(u16)sobj->sprite.height * 4u + 2u) / 5u;
        x = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
            sobj->pos.x, NDS_IFCOMMON_SCREEN_SCALE_Q16));
        y = ndsIFCommonRoundQ16HalfUp(ndsIFCommonScaleRound(
            sobj->pos.y, NDS_IFCOMMON_SCREEN_SCALE_Q16));
        for (ty = 0u; (ty * NDS_IFCOMMON_TRAINING_STRIP) < ds_height; ty++)
        {
            for (tx = 0u; (tx * NDS_IFCOMMON_TRAINING_STRIP) < ds_width; tx++)
            {
                const NDSIFCommonTrainingCell *cell =
                    ndsIFCommonTrainingBake(sobj, (ty << 4) | tx);

                if ((cell == NULL) ||
                    (ndsIFCommonTrainingObject(
                         x + (s32)(tx * NDS_IFCOMMON_TRAINING_STRIP),
                         y + (s32)(ty * NDS_IFCOMMON_TRAINING_STRIP),
                         (SpriteSize)cell->size, cell->palette, cell->gfx,
                         (cell->bitmap_obj != 0u) ? SpriteColorFormat_Bmp :
                                                    SpriteColorFormat_16Color) == FALSE))
                {
                    gNdsIFCommonTrainingMisses++;
                    return FALSE;
                }
            }
        }
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    gNdsIFCommonNativeOamFrameDrawCalls++;
    return TRUE;
}

/* A solid run cell of `rows` rows (1-8), 32x8, in the run palette's entry 1:
 * the underline's height in rows, its length in 32- and 8-pixel pieces. */
static u16 *ndsIFCommonTrainingRunCell(u32 rows)
{
    u32 x;
    u32 y;

    if ((rows == 0u) || (rows > 8u))
    {
        return NULL;
    }
    if (sNdsIFCommonTrainingRun[rows - 1u] != NULL)
    {
        return sNdsIFCommonTrainingRun[rows - 1u];
    }
    memset(sNdsIFCommonPauseCellBuffer, 0, 128u);
    for (y = 0u; y < rows; y++)
    {
        for (x = 0u; x < 32u; x++)
        {
            ndsIFCommonPauseSetTexel(32u, x, y, 1u);
        }
    }
    sNdsIFCommonTrainingRun[rows - 1u] = ndsIFCommonTrainingUpload(128u);
    return sNdsIFCommonTrainingRun[rows - 1u];
}

/* The box's one 32x32 bitmap cell, every texel the box colour, opaque bit
 * set (the alpha is the object's). */
static u16 *ndsIFCommonTrainingBoxCell(u16 rgb15)
{
    u32 words = (NDS_IFCOMMON_TRAINING_BOX_CELL * NDS_IFCOMMON_TRAINING_BOX_CELL) / 2u;
    u32 fill = (u32)(rgb15 | 0x8000u) | ((u32)(rgb15 | 0x8000u) << 16);
    u32 *dst;
    u32 i;

    if (sNdsIFCommonTrainingBox != NULL)
    {
        return sNdsIFCommonTrainingBox;
    }
    sNdsIFCommonTrainingBox = ndsIFCommonTrainingAlloc(words * 4u);
    if (sNdsIFCommonTrainingBox == NULL)
    {
        return NULL;
    }
    dst = (u32 *)(void *)sNdsIFCommonTrainingBox;
    for (i = 0u; i < words; i++)
    {
        dst[i] = fill;
    }
    return sNdsIFCommonTrainingBox;
}

void ndsIFCommonTrainingFillRect(s32 ulx, s32 uly, s32 lrx, s32 lry,
                                 u32 rgb, u32 alpha)
{
    s32 x0 = (ulx * 4 + 2) / 5;
    s32 y0 = (uly * 4 + 2) / 5;
    s32 x1 = ((lrx + 1) * 4 + 2) / 5;
    s32 y1 = ((lry + 1) * 4 + 2) / 5;
    u16 rgb15 = ndsIFCommonPackRgb15((u8)(rgb >> 16), (u8)(rgb >> 8),
                                     (u8)rgb);
    s32 x;
    s32 y;

    if ((gSCManagerSceneData.scene_curr != nSCKind1PTrainingMode) ||
        (x1 <= x0) || (y1 <= y0) || (ndsIFCommonTrainingBegin() == FALSE))
    {
        return;
    }
    if (alpha < 0xffu)
    {
        u16 *cell = ndsIFCommonTrainingBoxCell(rgb15);
        u32 alpha15 = (alpha * 15u + 127u) / 255u;

        if (cell == NULL)
        {
            gNdsIFCommonTrainingMisses++;
            return;
        }
        if (sNdsIFCommonTrainingBlend == FALSE)
        {
            sNdsIFCommonTrainingSavedBldCnt = REG_BLDCNT;
            REG_BLDCNT |= BLEND_DST_BG0 | BLEND_DST_BACKDROP;
            sNdsIFCommonTrainingBlend = TRUE;
        }
        sNdsIFCommonTrainingBoxFrame = gNdsRendererProfileFrameCount;
        for (y = y0; y < y1; y += (s32)NDS_IFCOMMON_TRAINING_BOX_CELL)
        {
            for (x = x0; x < x1; x += (s32)NDS_IFCOMMON_TRAINING_BOX_CELL)
            {
                /* Edge pieces overlap the previous one rather than run past
                 * the box: a piece is always a whole 32x32 object. */
                s32 px = ((x + (s32)NDS_IFCOMMON_TRAINING_BOX_CELL) > x1) ?
                    (x1 - (s32)NDS_IFCOMMON_TRAINING_BOX_CELL) : x;
                s32 py = ((y + (s32)NDS_IFCOMMON_TRAINING_BOX_CELL) > y1) ?
                    (y1 - (s32)NDS_IFCOMMON_TRAINING_BOX_CELL) : y;

                if (ndsIFCommonTrainingObject(px, py, SpriteSize_32x32,
                                              alpha15, cell,
                                              SpriteColorFormat_Bmp) == FALSE)
                {
                    gNdsIFCommonTrainingMisses++;
                    return;
                }
            }
        }
        return;
    }
    {
        u32 rows = (u32)(y1 - y0);
        u16 *run;

        if (rows > 8u)
        {
            rows = 8u;
        }
        run = ndsIFCommonTrainingRunCell(rows);
        if (sNdsIFCommonTrainingRunPalette < 0)
        {
            s32 palette = ndsIFCommonTrainingPalette(rgb, rgb);

            if (palette < 0)
            {
                gNdsIFCommonTrainingMisses++;
                return;
            }
            sNdsIFCommonTrainingRunPalette = (s8)palette;
        }
        if (run == NULL)
        {
            gNdsIFCommonTrainingMisses++;
            return;
        }
        for (x = x0; x < x1; )
        {
            SpriteSize size = ((x1 - x) >= 32) ? SpriteSize_32x8 :
                                                 SpriteSize_8x8;
            s32 step = ((x1 - x) >= 32) ? 32 : 8;

            if (ndsIFCommonTrainingObject(x, y0, size,
                                          (u32)sNdsIFCommonTrainingRunPalette,
                                          run, SpriteColorFormat_16Color) == FALSE)
            {
                gNdsIFCommonTrainingMisses++;
                return;
            }
            x += step;
        }
    }
}

/* The box blend's end: when the menu closed (no box last frame) and when
 * the Training scene ends (battleship_sc1ptrainingmode.c). */
void ndsIFCommonTrainingEnd(void)
{
    if (sNdsIFCommonTrainingBlend != FALSE)
    {
        REG_BLDCNT = sNdsIFCommonTrainingSavedBldCnt;
        sNdsIFCommonTrainingBlend = FALSE;
    }
}

static void ndsIFCommonTrainingFrameBegin(void)
{
    if ((sNdsIFCommonTrainingBlend != FALSE) &&
        ((gSCManagerSceneData.scene_curr != nSCKind1PTrainingMode) ||
         ((gNdsRendererProfileFrameCount - sNdsIFCommonTrainingBoxFrame) > 1u)))
    {
        ndsIFCommonTrainingEnd();
    }
}
#endif

s32 ndsIFCommonNativeOamDrawGObj(struct GObj *gobj)
{
    SObj *sobj;
    SObj *scan;
    u32 required_objects = 0u;
    u32 start = NDS_IFCOMMON_TELEMETRY_TICK();
    s32 oam_id_before;
    u32 matrix_count_before;
    u32 object_count_before;
    s32 recognized = FALSE;

    if (gobj == NULL)
    {
        return FALSE;
    }
    sobj = SObjGetStruct(gobj);
    if (sobj == NULL)
    {
        return FALSE;
    }
    if (gobj->proc_display == ifCommonPlayerTagProcDisplay)
    {
        return ndsIFCommonEmitPlayerTag(gobj);
    }
    if (gobj->proc_display == ifCommonItemArrowProcDisplay)
    {
        return ndsIFCommonEmitItemArrow(gobj);
    }
    if ((gobj->id == nGCCommonKindPauseMenu) &&
        (gobj->proc_display == lbCommonDrawSObjAttr))
    {
        return ndsIFCommonEmitPause(gobj);
    }
#if NDS_P2_1P_GAME
    if (ndsIFCommonIsBonusTaskGObj(gobj) != FALSE)
    {
        return ndsIFCommonEmitBonusTasks(gobj);
    }
#endif
    if ((gobj->proc_display == lbCommonDrawSObjAttr) &&
        (ndsIFCommonIsCommonLetter(sobj) != FALSE))
    {
        return ndsIFCommonEmitCommonLetters(gobj);
    }
#if NDS_P2_1P_GAME
    if (ndsIFCommonIsTrainingGObj(gobj) != FALSE)
    {
        return ndsIFCommonEmitTraining(gobj);
    }
#endif

    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        s32 asset_index;
        const NDSIFCommonTrafficSpec *traffic;
        const NDSIFCommonCloudSpec *cloud;

        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        asset_index = ndsIFCommonAssetForSObj(scan);
        if (asset_index == -1)
        {
            if (recognized == FALSE)
            {
                return FALSE;
            }
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackUnknownSprite;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        if (asset_index == -2)
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackRuntimeColor;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        recognized = TRUE;
        traffic = ndsIFCommonTrafficSpecForAsset((u32)asset_index);
        cloud = ndsIFCommonCloudSpecForAsset((u32)asset_index);
        if (((u32)asset_index >= NDS_IFCOMMON_END_FIRST) &&
            (ndsIFCommonEndAssetActive((u32)asset_index) == FALSE))
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackNotPrepared;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        if (((traffic != NULL) &&
             (ndsIFCommonTrafficSObjValid(scan, traffic) == FALSE)) ||
            ((cloud != NULL) &&
             (ndsIFCommonCloudSObjValid(scan, cloud) == FALSE)))
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
        if ((traffic == NULL) && (cloud == NULL))
        {
            required_objects += sNdsIFCommonAssets[asset_index].tile_count;
        }
#if NDS_SHIP_TELEMETRY
        ndsIFCommonRecordSemantic(scan, (u32)asset_index);
#endif
    }
    if (recognized == FALSE)
    {
        return FALSE;
    }
    gNdsIFCommonNativeOamFrameRecognizedCalls++;
    if (sNdsIFCommonPrepared == FALSE)
    {
        gNdsIFCommonNativeOamFrameFallbackCalls++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackNotPrepared;
        NDS_IFCOMMON_TELEMETRY_ADD(
            gNdsIFCommonNativeOamFrameTicks, start);
        return FALSE;
    }
    if (required_objects > (u32)(sNdsIFCommonNextOamID + 1))
    {
        gNdsIFCommonNativeOamFrameFallbackCalls++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackObjectLimit;
        NDS_IFCOMMON_TELEMETRY_ADD(
            gNdsIFCommonNativeOamFrameTicks, start);
        return FALSE;
    }

    oam_id_before = sNdsIFCommonNextOamID;
    matrix_count_before = sNdsIFCommonMatrixCount;
    object_count_before = gNdsIFCommonNativeOamFrameObjectCount;
    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        s32 asset_index;

        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        asset_index = ndsIFCommonAssetForSObj(scan);
        if ((asset_index >= 0) &&
            ((ndsIFCommonTrafficSpecForAsset((u32)asset_index) != NULL) ||
             (ndsIFCommonCloudSpecForAsset((u32)asset_index) != NULL)))
        {
            continue;
        }
        if ((asset_index < 0) ||
            (ndsIFCommonEmitSObj(scan, (u32)asset_index) == FALSE))
        {
            if (sNdsIFCommonNextOamID < oam_id_before)
            {
                oamClear(&oamMain, sNdsIFCommonNextOamID + 1,
                         oam_id_before - sNdsIFCommonNextOamID);
            }
            sNdsIFCommonNextOamID = oam_id_before;
            sNdsIFCommonMatrixCount = matrix_count_before;
            gNdsIFCommonNativeOamFrameObjectCount = object_count_before;
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackMatrixLimit;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
    }
    for (scan = sobj; scan != NULL; scan = scan->next)
    {
        s32 asset_index;
        const NDSIFCommonTrafficSpec *traffic;
        const NDSIFCommonCloudSpec *cloud;

        if ((scan->sprite.attr & SP_HIDDEN) != 0u)
        {
            continue;
        }
        asset_index = ndsIFCommonAssetForSObj(scan);
        traffic = (asset_index >= 0) ?
            ndsIFCommonTrafficSpecForAsset((u32)asset_index) : NULL;
        cloud = (asset_index >= 0) ?
            ndsIFCommonCloudSpecForAsset((u32)asset_index) : NULL;
        if (((traffic != NULL) &&
             (ndsIFCommonEmitTrafficSObj(scan, traffic) == FALSE)) ||
            ((cloud != NULL) &&
             (ndsIFCommonEmitCloudSObj(scan, cloud) == FALSE)))
        {
            gNdsIFCommonNativeOamFrameFallbackCalls++;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            NDS_IFCOMMON_TELEMETRY_ADD(
                gNdsIFCommonNativeOamFrameTicks, start);
            return FALSE;
        }
    }
    gNdsIFCommonNativeOamFrameDrawCalls++;
    if (gNdsIFCommonNativeOamFrameObjectCount != object_count_before)
    {
        sNdsIFCommonFrameNeedsCommit = TRUE;
    }
    NDS_IFCOMMON_TELEMETRY_ADD(gNdsIFCommonNativeOamFrameTicks, start);
    return TRUE;
}

void ndsIFCommonNativeOamCommit(void)
{
    if (sNdsIFCommonFrameNeedsCommit != FALSE)
    {
        u32 start = NDS_IFCOMMON_TELEMETRY_TICK();

        oamUpdate(&oamMain);
#if NDS_SHIP_TELEMETRY
        gNdsIFCommonNativeOamFrameCommitTicks = cpuGetTiming() - start;
#endif
        gNdsIFCommonNativeOamFrameCommitCalls = 1u;
        gNdsIFCommonNativeOamCommitCount++;
    }
    if (gNdsIFCommonNativeOamFrameObjectCount != 0u)
    {
        sNdsIFCommonPreviousLowestOamID = sNdsIFCommonNextOamID + 1;
    }
    else
    {
        sNdsIFCommonPreviousLowestOamID = 128;
    }
    if (sNdsIFCommonFrameNeedsCommit == FALSE)
    {
        gNdsIFCommonNativeOamFrameIdle = 1u;
        gNdsIFCommonNativeOamIdleFrameCount++;
    }
}
