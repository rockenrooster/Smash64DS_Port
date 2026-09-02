#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <string.h>

#include <ft/fighter.h>
#include <nds/nds_battle_hud.h>
#include <nds/nds_startup.h>

#include "generated/battle_hud.generated.inc"

#define NDS_BATTLE_HUD_PLAYERS 4u
#define NDS_BATTLE_HUD_DAMAGE_PALETTE_BASE 0u
#define NDS_BATTLE_HUD_WHITE_PALETTE 4u
#define NDS_BATTLE_HUD_PORTRAIT_PALETTE_BASE 5u
/* THE PORTRAIT AND STOCK BANDS USED TO OVERLAP, AND IT WAS ALREADY LIVE.
 * Portraits once occupied BASE..BASE+NDS_BATTLE_HUD_PORTRAITS-1, uploaded once
 * at prepare. At four portraits (5..8) and stock base 8, player 0's stock
 * palette overwrote the FOURTH portrait's -- Donkey's -- so a Donkey HUD
 * portrait drew in whatever colours player 0's stock icon last needed, and
 * every admitted fighter since took one more of the sixteen sub OBJ palettes:
 * Link, the seventh, pushed the four stock palettes to 12..15 and left NO slot
 * for an eighth portrait, let alone the twelve-fighter roster.
 *
 * The budget is per PLAYER, not per KIND: a match shows at most four
 * portraits, so the portrait band is now BASE..BASE+3 and each player's slot
 * is (re)loaded with its fighter's baked palette when that fighter changes,
 * exactly as the stock palettes already are. Every kind's 4bpp tiles stay
 * resident (tiles do not care which palette an OAM entry names), so the split
 * is damage 0..3, white 4, portraits 5..8, stocks 9..12, and 13..15 are free
 * for the rest of the roster. */
#define NDS_BATTLE_HUD_STOCK_PALETTE_BASE 9u
_Static_assert(NDS_BATTLE_HUD_STOCK_PALETTE_BASE >=
                   (NDS_BATTLE_HUD_PORTRAIT_PALETTE_BASE +
                    NDS_BATTLE_HUD_PLAYERS),
               "HUD stock palettes overlap the portrait band");
_Static_assert((NDS_BATTLE_HUD_STOCK_PALETTE_BASE +
                NDS_BATTLE_HUD_PLAYERS) <= 16u,
               "HUD stock palettes run off the sub OBJ palette");
#define NDS_BATTLE_HUD_MAX_OAM 64u
#define NDS_BATTLE_HUD_DAMAGE_CELL_SIZE 32

/* Source -> DS is the same 4/5 frame scale used by the menu bake. */
#define NDS_BATTLE_HUD_SOURCE_SCALE(v) (((v) * 4 + 2) / 5)
#define NDS_BATTLE_HUD_PLAYER_Y NDS_BATTLE_HUD_SOURCE_SCALE(210)
#define NDS_BATTLE_HUD_STOCK_Y NDS_BATTLE_HUD_SOURCE_SCALE(185)
#define NDS_BATTLE_HUD_PORTRAIT_Y 124

static const s16 sNdsBattleHudPlayerCenterX[NDS_BATTLE_HUD_PLAYERS] = {
    NDS_BATTLE_HUD_SOURCE_SCALE(55),
    NDS_BATTLE_HUD_SOURCE_SCALE(125),
    NDS_BATTLE_HUD_SOURCE_SCALE(195),
    NDS_BATTLE_HUD_SOURCE_SCALE(265)
};
static const s16 sNdsBattleHudTimerCenterX[5] = {
    NDS_BATTLE_HUD_SOURCE_SCALE(232),
    NDS_BATTLE_HUD_SOURCE_SCALE(247),
    NDS_BATTLE_HUD_SOURCE_SCALE(273),
    NDS_BATTLE_HUD_SOURCE_SCALE(288),
    NDS_BATTLE_HUD_SOURCE_SCALE(260)
};

static u16 *sNdsBattleHudDamageGfx[NDS_BATTLE_HUD_DAMAGE_GLYPHS];
static u16 *sNdsBattleHudTimerGfx[NDS_BATTLE_HUD_TIMER_GLYPHS];
static u16 *sNdsBattleHudStockDigitGfx[NDS_BATTLE_HUD_STOCK_DIGIT_GLYPHS];
static u16 *sNdsBattleHudPortraitGfx[NDS_BATTLE_HUD_PORTRAITS];
static u16 *sNdsBattleHudStockGfx[NDS_BATTLE_HUD_STOCK_OWNERS];
/* Which baked portrait palette each player's slot currently holds; 0xff is
 * "none", so the first draw after prepare/clear always uploads. */
static u8 sNdsBattleHudPortraitPaletteOwner[NDS_BATTLE_HUD_PLAYERS];
static u32 sNdsBattleHudPrepared;
static u32 sNdsBattleHudStateHash = 0xffffffffu;

volatile u32 gNdsBattleHudPrepareCount;
volatile u32 gNdsBattleHudRenderCount;
volatile u32 gNdsBattleHudChangeCount;
volatile u32 gNdsBattleHudOamCount;
volatile u32 gNdsBattleHudActiveMask;

static u32 ndsBattleHudMix(u32 hash, u32 value)
{
    hash ^= value;
    return hash * 16777619u;
}

static u32 ndsBattleHudFloatBits(f32 value)
{
    union {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return bits.u;
}

static u32 ndsBattleHudDamage(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0DamageCurrent;
    case 1u: return gNdsIFCommonHUDP1DamageCurrent;
    case 2u: return gNdsIFCommonHUDP2DamageCurrent;
    default: return gNdsIFCommonHUDP3DamageCurrent;
    }
}

static u32 ndsBattleHudStock(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0LowerStock;
    case 1u: return gNdsIFCommonHUDP1LowerStock;
    case 2u: return gNdsIFCommonHUDP2LowerStock;
    default: return gNdsIFCommonHUDP3LowerStock;
    }
}

static u32 ndsBattleHudFkind(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0FighterKind;
    case 1u: return gNdsIFCommonHUDP1FighterKind;
    case 2u: return gNdsIFCommonHUDP2FighterKind;
    default: return gNdsIFCommonHUDP3FighterKind;
    }
}

static u32 ndsBattleHudCostume(u32 player)
{
    switch (player)
    {
    case 0u: return gNdsIFCommonHUDP0Costume;
    case 1u: return gNdsIFCommonHUDP1Costume;
    case 2u: return gNdsIFCommonHUDP2Costume;
    default: return gNdsIFCommonHUDP3Costume;
    }
}

static u32 ndsBattleHudDisplaySeconds(void)
{
    u32 remain = gNdsIFCommonHUDTimeRemain;

    if (remain == 0u)
    {
        return 0u;
    }
    if ((remain != gNdsIFCommonHUDTimerLimit) &&
        (remain <= (0xffffffffu - 59u)))
    {
        /* ifCommonTimerProcDisplay:2391-2395. */
        remain += 59u;
    }
    return remain / 60u;
}

static u32 ndsBattleHudFingerprint(
    const NDSBattleHudDamageState damage_state[NDS_BATTLE_HUD_PLAYERS])
{
    u32 hash = 2166136261u;
    u32 player;

    hash = ndsBattleHudMix(hash, gNdsIFCommonHUDRecordCount != 0u);
    hash = ndsBattleHudMix(hash, gNdsIFCommonHUDActivePlayerMask);
    hash = ndsBattleHudMix(hash, gNdsIFCommonHUDShowDamageMask);
    hash = ndsBattleHudMix(hash, gNdsIFCommonHUDDamageFlashMask);
    hash = ndsBattleHudMix(hash, gNdsIFCommonHUDSingleStockMask);
    hash = ndsBattleHudMix(hash, gNdsIFCommonHUDTimerVisible);
    hash = ndsBattleHudMix(hash, ndsBattleHudDisplaySeconds());
    for (player = 0u; player < NDS_BATTLE_HUD_PLAYERS; player++)
    {
        u32 character;

        hash = ndsBattleHudMix(hash, ndsBattleHudDamage(player));
        hash = ndsBattleHudMix(hash, ndsBattleHudStock(player));
        hash = ndsBattleHudMix(hash, ndsBattleHudFkind(player));
        hash = ndsBattleHudMix(hash, ndsBattleHudCostume(player));
        hash = ndsBattleHudMix(hash,
                               ndsBattleHudFloatBits(damage_state[player].scale));
        hash = ndsBattleHudMix(hash, (u32)damage_state[player].damage);
        hash = ndsBattleHudMix(hash, damage_state[player].color_r);
        hash = ndsBattleHudMix(hash, damage_state[player].color_g);
        hash = ndsBattleHudMix(hash, damage_state[player].color_b);
        hash = ndsBattleHudMix(hash, damage_state[player].color_id);
        hash = ndsBattleHudMix(hash, damage_state[player].is_update_anim);
        hash = ndsBattleHudMix(hash, damage_state[player].char_count);
        hash = ndsBattleHudMix(hash, damage_state[player].visible);
        for (character = 0u; character < NDS_BATTLE_HUD_DAMAGE_CHARS;
             character++)
        {
            hash = ndsBattleHudMix(
                hash, ndsBattleHudFloatBits(
                          damage_state[player].chars[character].pos_x));
            hash = ndsBattleHudMix(
                hash, ndsBattleHudFloatBits(
                          damage_state[player].chars[character].pos_y));
            hash = ndsBattleHudMix(
                hash, damage_state[player].chars[character].image_id);
            hash = ndsBattleHudMix(
                hash, damage_state[player].chars[character].visible);
        }
    }
    return hash;
}

static u16 *ndsBattleHudAlloc(SpriteSize size, const u8 *source, u32 bytes)
{
    u16 *gfx = oamAllocateGfx(&oamSub, size, SpriteColorFormat_16Color);

    if (gfx != NULL)
    {
        dmaCopy(source, gfx, bytes);
    }
    return gfx;
}

static u32 ndsBattleHudPrepare(void)
{
    u32 i;

    if (sNdsBattleHudPrepared != FALSE)
    {
        return TRUE;
    }

    /* Bank H remains the sub BG console.  Bank I is the battle HUD's one and
     * only sub-OBJ tenant.  Reinitializing here is intentional: menu scenes may
     * have owned Bank I earlier, and no pointer into that lifetime is reused. */
    vramSetBankI(VRAM_I_SUB_SPRITE);
    oamInit(&oamSub, SpriteMapping_1D_32, false);

    for (i = 0u; i < NDS_BATTLE_HUD_DAMAGE_GLYPHS; i++)
    {
        sNdsBattleHudDamageGfx[i] = ndsBattleHudAlloc(
            SpriteSize_32x32, kNdsBattleHudDamageGfx[i],
            NDS_BATTLE_HUD_DAMAGE_GFX_BYTES);
        if (sNdsBattleHudDamageGfx[i] == NULL) return FALSE;
    }
    for (i = 0u; i < NDS_BATTLE_HUD_TIMER_GLYPHS; i++)
    {
        sNdsBattleHudTimerGfx[i] = ndsBattleHudAlloc(
            SpriteSize_16x16, kNdsBattleHudTimerGfx[i],
            NDS_BATTLE_HUD_TIMER_GFX_BYTES);
        if (sNdsBattleHudTimerGfx[i] == NULL) return FALSE;
    }
    for (i = 0u; i < NDS_BATTLE_HUD_STOCK_DIGIT_GLYPHS; i++)
    {
        sNdsBattleHudStockDigitGfx[i] = ndsBattleHudAlloc(
            SpriteSize_16x16, kNdsBattleHudStockDigitGfx[i],
            NDS_BATTLE_HUD_STOCK_DIGIT_GFX_BYTES);
        if (sNdsBattleHudStockDigitGfx[i] == NULL) return FALSE;
    }
    for (i = 0u; i < NDS_BATTLE_HUD_PORTRAITS; i++)
    {
        sNdsBattleHudPortraitGfx[i] = ndsBattleHudAlloc(
            SpriteSize_16x16, kNdsBattleHudPortraitGfx[i],
            NDS_BATTLE_HUD_PORTRAIT_GFX_BYTES);
        if (sNdsBattleHudPortraitGfx[i] == NULL) return FALSE;
    }
    for (i = 0u; i < NDS_BATTLE_HUD_STOCK_OWNERS; i++)
    {
        sNdsBattleHudStockGfx[i] = ndsBattleHudAlloc(
            SpriteSize_8x8, kNdsBattleHudStockGfx[i],
            NDS_BATTLE_HUD_STOCK_GFX_BYTES);
        if (sNdsBattleHudStockGfx[i] == NULL) return FALSE;
    }

    /* DS palette RAM drops byte writes.  Keep this hardware-visible copy on
     * the same 16/32-bit-safe DMA path as OBJ graphics instead of relying on
     * whatever access width a generic memcpy happens to choose. */
    dmaCopy(kNdsBattleHudWhitePalette[0],
            &SPRITE_PALETTE_SUB[NDS_BATTLE_HUD_WHITE_PALETTE * 16u],
            16u * sizeof(u16));
    memset(sNdsBattleHudPortraitPaletteOwner, 0xff,
           sizeof(sNdsBattleHudPortraitPaletteOwner));
    oamClear(&oamSub, 0, 128);
    oamUpdate(&oamSub);
    sNdsBattleHudStateHash = 0xffffffffu;
    sNdsBattleHudPrepared = TRUE;
    gNdsBattleHudPrepareCount++;
    return TRUE;
}

static void ndsBattleHudDamagePalette(
    u32 player, const NDSBattleHudDamageState *state)
{
    u32 r = (state != NULL) ? state->color_r : 0xffu;
    u32 g = (state != NULL) ? state->color_g : 0xffu;
    u32 b = (state != NULL) ? state->color_b : 0xffu;
    u32 i;
    u16 *palette = &SPRITE_PALETTE_SUB[
        (NDS_BATTLE_HUD_DAMAGE_PALETTE_BASE + player) * 16u];

    /* The imported source bridge has already evaluated
     * ifCommonPlayerDamageProcDisplay's primitive-colour expression with the
     * source float/truncation rules.  This loop only maps that RGB result onto
     * the DS's 16-entry intensity palette. */
    palette[0] = 0u;
    for (i = 1u; i < 16u; i++)
    {
        u32 ir = (r * i + 7u) / 15u;
        u32 ig = (g * i + 7u) / 15u;
        u32 ib = (b * i + 7u) / 15u;

        palette[i] = RGB15(ir >> 3, ig >> 3, ib >> 3) | BIT(15);
    }
}

static void ndsBattleHudStockPalette(u32 player, u32 fkind, u32 costume)
{
    const u16 *source;

    if (fkind == (u32)nFTKindFox)
    {
        if (costume >= 4u) costume = 0u;
        source = kNdsBattleHudFoxStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindLuigi)
    {
        if (costume >= 4u) costume = 0u;
        source = kNdsBattleHudLuigiStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindDonkey)
    {
        /* BattleShip dFTParamCostumeIDs maps royal colours to 0..3 and team
         * green to costume 4.  DkIcon carries all five source LUTs. */
        if (costume >= 5u) costume = 0u;
        source = kNdsBattleHudDonkeyStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindCaptain)
    {
        /* dFTParamCostumeIDs[nFTKindCaptain] is { {0,4,1,3}, {1,5,2}, 5 } --
         * six distinct indices, and CaptainModel carries six stock LUTs. */
        if (costume >= 6u) costume = 0u;
        source = kNdsBattleHudCaptainStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindSamus)
    {
        /* 217_SamusMain.c exposes five source stock LUTs. */
        if (costume >= 5u) costume = 0u;
        source = kNdsBattleHudSamusStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindLink)
    {
        /* dLinkMain_stock_luts contains exactly four source stock LUTs. */
        if (costume >= 4u) costume = 0u;
        source = kNdsBattleHudLinkStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindPikachu)
    {
        /* 243_PikachuMain.c dPikachuMain_stock_luts names five source LUTs. */
        if (costume >= 5u) costume = 0u;
        source = kNdsBattleHudPikachuStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindYoshi)
    {
        /* 247_YoshiMain.c dYoshiMain_stock_luts names six source LUTs. */
        if (costume >= 6u) costume = 0u;
        source = kNdsBattleHudYoshiStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindNess)
    {
        /* NessMain.c stock_luts names 4 source LUTs. */
        if (costume >= 4u) costume = 0u;
        source = kNdsBattleHudNessStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindPurin)
    {
        /* PurinMain.c stock_luts names 5 source LUTs. */
        if (costume >= 5u) costume = 0u;
        source = kNdsBattleHudPurinStockPalette[costume];
    }
    else if (fkind == (u32)nFTKindKirby)
    {
        /* KirbyMain.c stock_luts names 5 source LUTs. */
        if (costume >= 5u) costume = 0u;
        source = kNdsBattleHudKirbyStockPalette[costume];
    }
    else
    {
        if (costume >= 5u) costume = 0u;
        source = kNdsBattleHudMarioStockPalette[costume];
    }
    dmaCopy(source,
            &SPRITE_PALETTE_SUB[(NDS_BATTLE_HUD_STOCK_PALETTE_BASE + player) *
                                16u],
            16u * sizeof(u16));
}

static void ndsBattleHudSetOam(u32 *next_id, s32 x, s32 y,
                               SpriteSize size, u16 *gfx, u32 palette)
{
    if ((*next_id >= NDS_BATTLE_HUD_MAX_OAM) || (gfx == NULL))
    {
        return;
    }
    oamSet(&oamSub, (int)*next_id, (int)x, (int)y, 0, (int)palette,
           size, SpriteColorFormat_16Color, gfx, -1, false, false, false, false,
           false);
    (*next_id)++;
}

static s32 ndsBattleHudSourceCoord(f32 source)
{
    f32 scaled = source * 0.8F;

    return (scaled >= 0.0F) ? (s32)(scaled + 0.5F) :
                              (s32)(scaled - 0.5F);
}

static void ndsBattleHudSetDamageOam(
    u32 *next_id, u32 player, s32 center_x, s32 center_y, u16 *gfx,
    u32 palette, f32 scale)
{
    u32 scale_q8;
    u32 inverse_q8;
    s32 half_bounds = NDS_BATTLE_HUD_DAMAGE_CELL_SIZE / 2;
    sb32 size_double = FALSE;

    if ((*next_id >= NDS_BATTLE_HUD_MAX_OAM) || (gfx == NULL))
    {
        return;
    }
    if (scale <= 0.0F)
    {
        scale = 1.0F;
    }
    scale_q8 = (u32)(scale * 256.0F + 0.5F);
    if (scale_q8 == 0u)
    {
        scale_q8 = 1u;
    }
    inverse_q8 = (65536u + (scale_q8 / 2u)) / scale_q8;
    if (inverse_q8 > 0x7fffu)
    {
        inverse_q8 = 0x7fffu;
    }

    /* The AOT damage glyph is centred in a transparent 32x32 cell, so the
     * source IFDCharacter position is also the DS affine pivot.  Normal
     * Mario/Fox hit deltas keep the source bounce <=2x and therefore fit the
     * 32x32 nominal bounds.  Preserve the source's unbounded scale for larger
     * deltas by enabling the hardware double-size bounds only when needed. */
    if (scale_q8 > 512u)
    {
        size_double = TRUE;
        half_bounds = NDS_BATTLE_HUD_DAMAGE_CELL_SIZE;
    }
    oamRotateScale(&oamSub, (int)player, 0,
                   (int)inverse_q8, (int)inverse_q8);
    oamSet(&oamSub, (int)*next_id,
           (int)(center_x - half_bounds), (int)(center_y - half_bounds),
           0, (int)palette, SpriteSize_32x32, SpriteColorFormat_16Color,
           gfx, (int)player, size_double, false, false, false, false);
    (*next_id)++;
}

static u32 ndsBattleHudMakeDecimal(u32 value, u8 *digits, u32 max_digits)
{
    u32 divisor = 1u;
    u32 count = 0u;

    while ((value / divisor >= 10u) && (divisor <= 100000000u))
    {
        divisor *= 10u;
    }
    do
    {
        if (count >= max_digits) break;
        digits[count++] = (u8)(value / divisor);
        value %= divisor;
        divisor /= 10u;
    } while (divisor != 0u);
    return count;
}

static void ndsBattleHudDrawDamage(
    u32 player, const NDSBattleHudDamageState *state, u32 *next_id)
{
    u32 i;

    if ((state == NULL) || (state->visible == 0u))
    {
        return;
    }
    for (i = 0u; i < NDS_BATTLE_HUD_DAMAGE_CHARS; i++)
    {
        const NDSBattleHudDamageCharState *character = &state->chars[i];
        u32 glyph = character->image_id;

        if ((character->visible == 0u) ||
            (glyph >= NDS_BATTLE_HUD_DAMAGE_GLYPHS))
        {
            continue;
        }
        ndsBattleHudSetDamageOam(
            next_id, player,
            ndsBattleHudSourceCoord(character->pos_x),
            ndsBattleHudSourceCoord(character->pos_y),
            sNdsBattleHudDamageGfx[glyph],
            NDS_BATTLE_HUD_DAMAGE_PALETTE_BASE + player, state->scale);
    }
}

static void ndsBattleHudDrawStock(u32 player, u32 fkind, u32 *next_id)
{
    u32 stock = ndsBattleHudStock(player);
    u32 owner;
    u32 palette = NDS_BATTLE_HUD_STOCK_PALETTE_BASE + player;
    u32 i;

    if (fkind == (u32)nFTKindMario) owner = 0u;
    else if (fkind == (u32)nFTKindFox) owner = 1u;
    else if (fkind == (u32)nFTKindLuigi) owner = 2u;
    else if (fkind == (u32)nFTKindDonkey) owner = 3u;
    else if (fkind == (u32)nFTKindCaptain) owner = 4u;
    else if (fkind == (u32)nFTKindSamus) owner = 5u;
    else if (fkind == (u32)nFTKindLink) owner = 6u;
    else if (fkind == (u32)nFTKindPikachu) owner = 7u;
    else if (fkind == (u32)nFTKindYoshi) owner = 8u;
    else if (fkind == (u32)nFTKindNess) owner = 9u;
    else if (fkind == (u32)nFTKindPurin) owner = 10u;
    else if (fkind == (u32)nFTKindKirby) owner = 11u;
    else return;

    if (stock == 0x7fu)
    {
        return;
    }
    if (stock <= 6u)
    {
        for (i = 0u; i < stock; i++)
        {
            s32 source_center = (s32)(55 + player * 70u) - 24 + (s32)i * 10;
            s32 x = NDS_BATTLE_HUD_SOURCE_SCALE(source_center) -
                    ((s32)NDS_BATTLE_HUD_STOCK_CONTENT_W / 2);

            ndsBattleHudSetOam(next_id, x, NDS_BATTLE_HUD_STOCK_Y,
                               SpriteSize_8x8, sNdsBattleHudStockGfx[owner],
                               palette);
        }
    }
    else
    {
        u8 digits[3];
        u32 count;
        s32 center = sNdsBattleHudPlayerCenterX[player];
        s32 x;

        if (stock > 999u) stock = 999u;
        count = ndsBattleHudMakeDecimal(stock, digits, 3u);
        /* Source: icon at trunc_pos_x-22, cross at -10.5, then digits +8. */
        x = center + NDS_BATTLE_HUD_SOURCE_SCALE(4 - 22) -
            ((s32)NDS_BATTLE_HUD_STOCK_CONTENT_W / 2);
        ndsBattleHudSetOam(next_id, x, NDS_BATTLE_HUD_STOCK_Y,
                           SpriteSize_8x8, sNdsBattleHudStockGfx[owner], palette);
        x = center + NDS_BATTLE_HUD_SOURCE_SCALE(4 - 10) -
            ((s32)kNdsBattleHudStockDigitMetric[10][0] / 2);
        ndsBattleHudSetOam(next_id, x,
                           NDS_BATTLE_HUD_SOURCE_SCALE(190) -
                               ((s32)kNdsBattleHudStockDigitMetric[10][1] / 2),
                           SpriteSize_16x16, sNdsBattleHudStockDigitGfx[10],
                           NDS_BATTLE_HUD_WHITE_PALETTE);
        for (i = 0u; i < count; i++)
        {
            u32 glyph = digits[i];
            x = center + NDS_BATTLE_HUD_SOURCE_SCALE(4 + (s32)i * 8) -
                ((s32)kNdsBattleHudStockDigitMetric[glyph][0] / 2);
            ndsBattleHudSetOam(next_id, x,
                               NDS_BATTLE_HUD_SOURCE_SCALE(190) -
                                   ((s32)kNdsBattleHudStockDigitMetric[glyph][1] /
                                    2),
                               SpriteSize_16x16,
                               sNdsBattleHudStockDigitGfx[glyph],
                               NDS_BATTLE_HUD_WHITE_PALETTE);
        }
    }
}

static void ndsBattleHudDrawPortrait(u32 player, u32 fkind, u32 *next_id)
{
    u32 owner;

    if (fkind == (u32)nFTKindMario) owner = 0u;
    else if (fkind == (u32)nFTKindFox) owner = 1u;
    else if (fkind == (u32)nFTKindLuigi) owner = 2u;
    else if (fkind == (u32)nFTKindDonkey) owner = 3u;
    else if (fkind == (u32)nFTKindCaptain) owner = 4u;
    else if (fkind == (u32)nFTKindSamus) owner = 5u;
    else if (fkind == (u32)nFTKindLink) owner = 6u;
    else if (fkind == (u32)nFTKindPikachu) owner = 7u;
    else if (fkind == (u32)nFTKindYoshi) owner = 8u;
    else if (fkind == (u32)nFTKindNess) owner = 9u;
    else if (fkind == (u32)nFTKindPurin) owner = 10u;
    else if (fkind == (u32)nFTKindKirby) owner = 11u;
    else return;

    if (sNdsBattleHudPortraitPaletteOwner[player] != (u8)owner)
    {
        /* Same DMA path as the stock palettes: palette RAM drops byte writes. */
        dmaCopy(kNdsBattleHudPortraitPalette[owner],
                &SPRITE_PALETTE_SUB[
                    (NDS_BATTLE_HUD_PORTRAIT_PALETTE_BASE + player) * 16u],
                16u * sizeof(u16));
        sNdsBattleHudPortraitPaletteOwner[player] = (u8)owner;
    }
    ndsBattleHudSetOam(next_id, sNdsBattleHudPlayerCenterX[player] - 8,
                       NDS_BATTLE_HUD_PORTRAIT_Y, SpriteSize_16x16,
                       sNdsBattleHudPortraitGfx[owner],
                       NDS_BATTLE_HUD_PORTRAIT_PALETTE_BASE + player);
}

static void ndsBattleHudDrawTimer(u32 *next_id)
{
    u32 seconds;
    u32 minutes;
    u8 digits[4];
    u32 i;

    if (gNdsIFCommonHUDTimerVisible == 0u)
    {
        return;
    }
    seconds = ndsBattleHudDisplaySeconds();
    minutes = seconds / 60u;
    seconds %= 60u;
    if (minutes > 99u) minutes = 99u;
    digits[0] = (u8)(minutes / 10u);
    digits[1] = (u8)(minutes % 10u);
    digits[2] = (u8)(seconds / 10u);
    digits[3] = (u8)(seconds % 10u);
    for (i = 0u; i < 4u; i++)
    {
        u32 glyph = digits[i];
        s32 x = sNdsBattleHudTimerCenterX[i] -
                ((s32)kNdsBattleHudTimerMetric[glyph][0] / 2);
        s32 y = NDS_BATTLE_HUD_SOURCE_SCALE(30) -
                ((s32)kNdsBattleHudTimerMetric[glyph][1] / 2);

        ndsBattleHudSetOam(next_id, x, y, SpriteSize_16x16,
                           sNdsBattleHudTimerGfx[glyph],
                           NDS_BATTLE_HUD_WHITE_PALETTE);
    }
    ndsBattleHudSetOam(
        next_id,
        sNdsBattleHudTimerCenterX[4] -
            ((s32)kNdsBattleHudTimerMetric[10][0] / 2),
        NDS_BATTLE_HUD_SOURCE_SCALE(30) -
            ((s32)kNdsBattleHudTimerMetric[10][1] / 2),
        SpriteSize_16x16, sNdsBattleHudTimerGfx[10],
        NDS_BATTLE_HUD_WHITE_PALETTE);
}

void ndsBattleHudRender(void)
{
    NDSBattleHudDamageState damage_state[NDS_BATTLE_HUD_PLAYERS];
    u32 hash;
    u32 next_id = 0u;
    u32 player;

    gNdsBattleHudRenderCount++;
    if ((gNdsIFCommonHUDRecordCount == 0u) ||
        (gNdsIFCommonHUDActivePlayerMask == 0u))
    {
        ndsBattleHudClear();
        return;
    }
    if (ndsBattleHudPrepare() == FALSE)
    {
        return;
    }
    for (player = 0u; player < NDS_BATTLE_HUD_PLAYERS; player++)
    {
        if (ndsIFCommonGetBattleHudDamageState(player,
                                               &damage_state[player]) == FALSE)
        {
            memset(&damage_state[player], 0, sizeof(damage_state[player]));
        }
    }
    hash = ndsBattleHudFingerprint(damage_state);
    if (hash == sNdsBattleHudStateHash)
    {
        return;
    }
    sNdsBattleHudStateHash = hash;
    gNdsBattleHudChangeCount++;

    oamClear(&oamSub, 0, 128);
    ndsBattleHudDrawTimer(&next_id);
    for (player = 0u; player < NDS_BATTLE_HUD_PLAYERS; player++)
    {
        u32 bit = 1u << player;
        u32 fkind;
        u32 costume;

        if ((gNdsIFCommonHUDActivePlayerMask & bit) == 0u)
        {
            continue;
        }
        fkind = ndsBattleHudFkind(player);
        costume = ndsBattleHudCostume(player);
        ndsBattleHudDamagePalette(player, &damage_state[player]);
        ndsBattleHudStockPalette(player, fkind, costume);
        ndsBattleHudDrawPortrait(player, fkind, &next_id);
        ndsBattleHudDrawStock(player, fkind, &next_id);
        ndsBattleHudDrawDamage(player, &damage_state[player], &next_id);
    }
    oamUpdate(&oamSub);
    gNdsBattleHudOamCount = next_id;
    gNdsBattleHudActiveMask = gNdsIFCommonHUDActivePlayerMask;
}

void ndsBattleHudClear(void)
{
    if (sNdsBattleHudPrepared != FALSE)
    {
        oamClear(&oamSub, 0, 128);
        oamUpdate(&oamSub);
    }
    /* Force a fresh Bank-I ownership/allocator lifetime on the next battle.
     * This is what makes a future sub-engine menu owner safe too. */
    sNdsBattleHudPrepared = FALSE;
    sNdsBattleHudStateHash = 0xffffffffu;
    memset(sNdsBattleHudPortraitPaletteOwner, 0xff,
           sizeof(sNdsBattleHudPortraitPaletteOwner));
    gNdsBattleHudOamCount = 0u;
    gNdsBattleHudActiveMask = 0u;
}
