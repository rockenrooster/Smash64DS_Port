/* P2-1c -- the VS shell's 2D UI kit. Contract: include/nds/nds_ui_kit.h.
 * Per-scene VRAM bank ownership: docs/p2/P2-1c-vram-map.md. */

#include "nds_build_config.h"

#if NDS_P2_UI_KIT

#include <nds.h>
#include <string.h>

#include <nds/nds_ui_kit.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_audio_fgm.h>

#include "generated/mn_ui_kit.generated.inc"

#define NDS_UI_KIT_PACK_PATH "nitro:/menus/mn_ui_kit.bin"
/* P2-1h. A SECOND file, deliberately: ndsUiKitEnter reads and hashes the whole
 * OBJ pack on every screen entry, so backdrop art living in it would cost the
 * character select the bytes of a title screen it never shows. */
#define NDS_UI_KIT_SURFACE_PATH "nitro:/menus/mn_surfaces.bin"

/* Bank E on main, bank I on sub. */
#define NDS_UI_KIT_OBJ_BYTES_MAIN (64u * 1024u)
#define NDS_UI_KIT_OBJ_BYTES_SUB (16u * 1024u)

#define NDS_UI_KIT_TEXT_CHUNK_BYTES \
    (NDS_UI_KIT_TEXT_CHUNK_W * NDS_UI_KIT_TEXT_CHUNK_H * (u32)sizeof(u16))
#define NDS_UI_KIT_TEXT_BYTES \
    (NDS_UI_KIT_TEXT_SLOTS * NDS_UI_KIT_TEXT_CHUNKS * \
     NDS_UI_KIT_TEXT_CHUNK_BYTES)

/* The pack is read through a small staging buffer because VRAM drops 8-bit
 * writes and `fread` is a byte path: every image byte lands in main RAM first
 * and reaches VRAM only through dmaCopyWords. 2 KiB keeps the .bss cost near
 * the glyph table's and still holds the image block in eleven reads, all of
 * them at scene load. */
#define NDS_UI_KIT_STAGING_BYTES 2048u

/* Letters sit one row down inside the 8-row cell so the apostrophe's -1 and
 * the period's +4 (mnmaps.c:325/:329) both stay inside it. */
#define NDS_UI_KIT_TEXT_BASELINE 1

_Static_assert(NDS_MN_UI_KIT_GLYPH_CELL_W == 8u &&
                   NDS_MN_UI_KIT_GLYPH_CELL_H == 8u,
               "the glyph cell the blit indexes is 8x8");
_Static_assert(NDS_MN_UI_KIT_GLYPH_COUNT == 29u,
               "mnMapsGetCharacterID maps 29 glyphs");
_Static_assert(NDS_UI_KIT_TEXT_BYTES <= NDS_UI_KIT_OBJ_BYTES_SUB,
               "the text budget must fit bank I so both engines share a layout");
_Static_assert(NDS_UI_KIT_OAM_IDS <= 128u, "OAM has 128 entries an engine");

/* --gc-sections drops a global whose only reader is a probe script; that has
 * reddened Boundary once already on "Missing ELF symbol". */
#define NDS_UI_KIT_PUBLISHED __attribute__((used))

NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitEnterCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitEnterRejectCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitExitCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitEngine = 0xffffffffu;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitPackOpenCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitPackBytesLoaded;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitPackHash;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitPackHashMismatchCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitPackReadFailCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTextComposeCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTextComposeSkipCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTextOverflowCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitCommitCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitCommitIdleCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitVisibleObjectCount;
NDS_UI_KIT_PUBLISHED volatile u32
    gNdsUiKitSfxRequestCount[NDS_UI_KIT_SFX_COUNT];
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSfxLastId;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceOpenCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceBlitCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitFireAtlasBlitCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceBytes;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceHashMismatchCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceReadFailCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceNoLayerCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceLastHash;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceCacheCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceDrawCachedCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceEraseCachedCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitSurfaceTicks;

/* The source's own FGM ids, gm/gmsound.h under REGION_US. Cross-checked
 * against three ids the FGM pack's own comments name (Escape 11, GuardOn 13,
 * GamePause 278) because the enum carries REGION_US conditionals and a naive
 * line count is off by seven through them. */
static const u16 sNdsUiKitSfxIds[NDS_UI_KIT_SFX_COUNT] = {
    164u, /* nSYAudioFGMMenuScroll2     -- mnvsmode.c:1361 cursor move  */
    158u, /* nSYAudioFGMMenuSelect      -- mnmodeselect.c:734 confirm   */
    165u, /* nSYAudioFGMMenuDenied      -- mnplayersvs.c:177 refusal    */
    163u, /* nSYAudioFGMMenuScroll1     -- mnvsmode.c:1449 value change */
    157u  /* nSYAudioFGMTitlePressStart -- mntitle.c:501 confirm        */
};

typedef struct NdsUiKitTextSlot {
    s16 x;
    s16 y;
    u16 width_px;
    u8 chunks;
    u8 visible;
    u32 content_key;
} NdsUiKitTextSlot;

typedef struct NdsUiKitSpriteSlot {
    s16 x;
    s16 y;
    u8 image;
    u8 visible;
    /* P2-1N (5): bitmap-OBJ blend coefficient (1..15; 15 = opaque, the
     * default every plain SetSprite keeps) and the affine double-size flag
     * (the emblem bakes at half resolution and renders 2x). */
    u8 alpha;
    u8 scale2x;
    /* Main-engine OBJ priority, 0 (front) through 3 (back). Plain sprites
     * stay at 0; the title emblem uses 3 so the source wordmark layer draws
     * over it while the title fire remains behind it. */
    u8 priority;
} NdsUiKitSpriteSlot;

static u8 sNdsUiKitGlyphs[NDS_MN_UI_KIT_GLYPH_BLOCK_BYTES];
static u8 sNdsUiKitStaging[NDS_UI_KIT_STAGING_BYTES] __attribute__((aligned(4)));
static NdsUiKitTextSlot sNdsUiKitText[NDS_UI_KIT_TEXT_SLOTS];
static NdsUiKitSpriteSlot sNdsUiKitSprites[NDS_UI_KIT_SPRITE_SLOTS];
static u32 sNdsUiKitImageVram[NDS_MN_UI_KIT_IMAGE_COUNT];
static u32 sNdsUiKitTextVramBase;
static u32 sNdsUiKitTextResident;
static u32 sNdsUiKitActive;
static u32 sNdsUiKitImagesResident;
static u32 sNdsUiKitDirty;

static OamState *ndsUiKitOam(void)
{
    return (gNdsUiKitEngine == NDS_UI_KIT_ENGINE_SUB) ? &oamSub : &oamMain;
}

static u16 *ndsUiKitObjBase(void)
{
    return (gNdsUiKitEngine == NDS_UI_KIT_ENGINE_SUB) ?
        SPRITE_GFX_SUB : SPRITE_GFX;
}

static u16 *ndsUiKitObjAt(u32 offset)
{
    return (u16 *)((u8 *)ndsUiKitObjBase() + offset);
}

static SpriteSize ndsUiKitSpriteSize(u32 width, u32 height)
{
    if ((width == 8u) && (height == 8u)) return SpriteSize_8x8;
    if ((width == 16u) && (height == 16u)) return SpriteSize_16x16;
    if ((width == 32u) && (height == 32u)) return SpriteSize_32x32;
    if ((width == 64u) && (height == 64u)) return SpriteSize_64x64;
    if ((width == 16u) && (height == 8u)) return SpriteSize_16x8;
    if ((width == 32u) && (height == 8u)) return SpriteSize_32x8;
    if ((width == 32u) && (height == 16u)) return SpriteSize_32x16;
    if ((width == 64u) && (height == 32u)) return SpriteSize_64x32;
    if ((width == 8u) && (height == 16u)) return SpriteSize_8x16;
    if ((width == 8u) && (height == 32u)) return SpriteSize_8x32;
    if ((width == 16u) && (height == 32u)) return SpriteSize_16x32;
    if ((width == 32u) && (height == 64u)) return SpriteSize_32x64;
    return (SpriteSize)0;
}

/* --- Source text layout (mn/mnmaps/mnmaps.c). ---------------------------- */

/* mnMapsGetCharacterID, minus the space case: a space advances without a
 * glyph, so the caller separates the two. */
static s32 ndsUiKitGlyphIndex(char c)
{
    switch (c)
    {
    case '\'': return 26;
    case '%': return 27;
    case '.': return 28;
    default: break;
    }
    if ((c < 'A') || (c > 'Z'))
    {
        return -1;
    }
    return (s32)(c - 'A');
}

/* mnMapsGetCharacterSpacing, in whole pixels -- every value it returns is
 * 0.0f or 1.0f. */
static s32 ndsUiKitSpacing(const char *str, s32 index)
{
    char here = str[index];
    char next = str[index + 1];

    switch (here)
    {
    case 'A':
        switch (next)
        {
        case 'F': case 'P': case 'T': case 'V': case 'Y': return 0;
        default: return 1;
        }
    case 'F': case 'P': case 'V': case 'Y':
        switch (next)
        {
        case 'A': case 'T': return 0;
        default: return 1;
        }
    case 'Q': case 'T':
        switch (next)
        {
        case '\'': case '.': return 1;
        default: return 0;
        }
    case '\'': return 1;
    case '.': return 1;
    default:
        return (next == 'T') ? 0 : 1;
    }
}

/* mnMapsMakeString's per-character vertical nudge. */
static s32 ndsUiKitGlyphRow(char c)
{
    if (c == '\'') return NDS_UI_KIT_TEXT_BASELINE - 1;
    if (c == '.') return NDS_UI_KIT_TEXT_BASELINE + 4;
    return NDS_UI_KIT_TEXT_BASELINE;
}

u32 ndsUiKitTextWidth(const char *text)
{
    s32 cursor = 0;
    s32 i;

    if (text == NULL)
    {
        return 0u;
    }
    for (i = 0; text[i] != '\0'; i++)
    {
        char c = text[i];
        s32 glyph;

        if (c == ' ')
        {
            cursor += 4;
            continue;
        }
        if ((c >= '0') && (c <= '9'))
        {
            cursor += (s32)(c - '0');
            continue;
        }
        glyph = ndsUiKitGlyphIndex(c);
        if (glyph < 0)
        {
            continue;
        }
        cursor += (s32)kNdsUiKitGlyphMetrics[glyph].width +
                  ndsUiKitSpacing(text, i);
    }
    return (cursor > 0) ? (u32)cursor : 0u;
}

/* --- Pack load ----------------------------------------------------------- */

static u32 ndsUiKitHashFold(u32 hash, const u8 *bytes, u32 count)
{
    u32 i;

    for (i = 0u; i < count; i++)
    {
        hash ^= bytes[i];
        hash *= 0x01000193u;
    }
    return hash;
}

static s32 ndsUiKitLoadPack(u32 objbytes)
{
    u32 hash = 0x811C9DC5u;
    u32 image;
    u32 cursor;
    /* ONE NitroFS open for the whole pack (P2-1c residual). Every chunk below
     * used to be its own fopen/fseek/fread/fclose -- twelve directory walks
     * for 24 KB, and twenty-three once P2-1d's digit block landed. */
    NdsRelocAssetStream stream;

    gNdsUiKitPackBytesLoaded = 0u;
    gNdsUiKitPackHash = 0u;

    if (ndsRelocAssetStreamOpen(&stream, NDS_UI_KIT_PACK_PATH) == FALSE)
    {
        gNdsUiKitPackReadFailCount++;
        return FALSE;
    }
    gNdsUiKitPackOpenCount++;

    if (ndsRelocAssetStreamRead(&stream, 0u, sNdsUiKitGlyphs,
                                NDS_MN_UI_KIT_GLYPH_BLOCK_BYTES) == FALSE)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsUiKitPackReadFailCount++;
        return FALSE;
    }
    hash = ndsUiKitHashFold(hash, sNdsUiKitGlyphs,
                            NDS_MN_UI_KIT_GLYPH_BLOCK_BYTES);
    gNdsUiKitPackBytesLoaded = NDS_MN_UI_KIT_GLYPH_BLOCK_BYTES;

    /* Images land at the TOP of the engine's OBJ VRAM and the text cells just
     * below them, so the kit grows downward while the battle's OBJ tenant
     * grows upward from zero. */
    cursor = objbytes;
    for (image = 0u; image < NDS_MN_UI_KIT_IMAGE_COUNT; image++)
    {
        const NdsUiKitImageMetric *metric = &kNdsUiKitImageMetrics[image];
        u32 done = 0u;

        sNdsUiKitImageVram[image] = 0xffffffffu;
        if (sNdsUiKitImagesResident == FALSE)
        {
            continue;
        }
        if (metric->bytes > cursor)
        {
            ndsRelocAssetStreamClose(&stream);
            gNdsUiKitPackReadFailCount++;
            return FALSE;
        }
        cursor -= metric->bytes;
        sNdsUiKitImageVram[image] = cursor;
        while (done < metric->bytes)
        {
            u32 slice = metric->bytes - done;

            if (slice > NDS_UI_KIT_STAGING_BYTES)
            {
                slice = NDS_UI_KIT_STAGING_BYTES;
            }
            if (ndsRelocAssetStreamRead(&stream, metric->offset + done,
                                        sNdsUiKitStaging, slice) == FALSE)
            {
                ndsRelocAssetStreamClose(&stream);
                gNdsUiKitPackReadFailCount++;
                return FALSE;
            }
            hash = ndsUiKitHashFold(hash, sNdsUiKitStaging, slice);
            DC_FlushRange(sNdsUiKitStaging, slice);
            dmaCopyWords(3, sNdsUiKitStaging,
                         (u8 *)ndsUiKitObjAt(cursor) + done, slice);
            gNdsUiKitPackBytesLoaded += slice;
            done += slice;
        }
    }

    ndsRelocAssetStreamClose(&stream);

    /* P2-1 closeout: every MAIN-screen menu label now ships as converted
     * source sprite/surface art. Reserving the historical 16 KiB text slab in
     * Bank E therefore bought no reachable behavior and left almost no gap
     * between the menu and battle OBJ tenants. Preserve the text path on the
     * sub engine, where Bank I was explicitly budgeted for it, but reclaim the
     * dead main-engine reservation. */
    if (sNdsUiKitTextResident != FALSE)
    {
        sNdsUiKitTextVramBase = cursor - NDS_UI_KIT_TEXT_BYTES;
        dmaFillHalfWords(0u, ndsUiKitObjAt(sNdsUiKitTextVramBase),
                         NDS_UI_KIT_TEXT_BYTES);
    }
    else
    {
        sNdsUiKitTextVramBase = 0xffffffffu;
    }

    gNdsUiKitPackHash = hash;
    /* The hash covers only the bytes this engine actually loaded, so a
     * text-only sub-engine load cannot be compared against the whole-pack
     * constant.  Checking it on the main engine is what makes a truncated or
     * stale NitroFS file a counted failure rather than a garbled screen. */
    if ((sNdsUiKitImagesResident != FALSE) &&
        (hash != NDS_MN_UI_KIT_PACK_FNV32))
    {
        gNdsUiKitPackHashMismatchCount++;
        return FALSE;
    }
    return TRUE;
}

/* --- Lifetime ------------------------------------------------------------ */

s32 ndsUiKitEnter(u32 engine)
{
    u32 objbytes;

    if (sNdsUiKitActive != FALSE)
    {
        return (gNdsUiKitEngine == engine) ? TRUE : FALSE;
    }
    if (engine > NDS_UI_KIT_ENGINE_SUB)
    {
        gNdsUiKitEnterRejectCount++;
        return FALSE;
    }
    /* The battle's OBJ tenant packs bank E from zero with no cap of its own.
     * Entering over a live one would be a silent overlap, so it is refused
     * and counted instead. */
    if ((engine == NDS_UI_KIT_ENGINE_MAIN) &&
        (ndsIFCommonNativeOamIsPrepared() != FALSE))
    {
        gNdsUiKitEnterRejectCount++;
        return FALSE;
    }

    gNdsUiKitEngine = engine;
    if (engine == NDS_UI_KIT_ENGINE_SUB)
    {
        objbytes = NDS_UI_KIT_OBJ_BYTES_SUB;
        sNdsUiKitImagesResident = FALSE;
        sNdsUiKitTextResident = TRUE;
        vramSetBankI(VRAM_I_SUB_SPRITE);
        oamInit(&oamSub, SpriteMapping_Bmp_1D_128, false);
        REG_DISPCNT_SUB |= DISPLAY_SPR_ACTIVE;
    }
    else
    {
        objbytes = NDS_UI_KIT_OBJ_BYTES_MAIN;
        sNdsUiKitImagesResident = TRUE;
        sNdsUiKitTextResident = FALSE;
        oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
        REG_DISPCNT |= DISPLAY_SPR_ACTIVE;
    }
    /* An OAM entry retains its last state; clearing every id the kit owns is
     * what stops a previous scene's sprite surviving into this one. */
    oamClear(ndsUiKitOam(), 0, NDS_UI_KIT_OAM_IDS);
    memset(sNdsUiKitText, 0, sizeof(sNdsUiKitText));
    memset(sNdsUiKitSprites, 0, sizeof(sNdsUiKitSprites));

    if (ndsUiKitLoadPack(objbytes) == FALSE)
    {
        gNdsUiKitEngine = 0xffffffffu;
        gNdsUiKitEnterRejectCount++;
        return FALSE;
    }

    sNdsUiKitActive = TRUE;
    sNdsUiKitDirty = TRUE;
    gNdsUiKitEnterCount++;
    return TRUE;
}

void ndsUiKitExit(void)
{
    if (sNdsUiKitActive == FALSE)
    {
        return;
    }
    oamClear(ndsUiKitOam(), 0, NDS_UI_KIT_OAM_IDS);
    oamUpdate(ndsUiKitOam());
    memset(sNdsUiKitText, 0, sizeof(sNdsUiKitText));
    memset(sNdsUiKitSprites, 0, sizeof(sNdsUiKitSprites));
    sNdsUiKitActive = FALSE;
    sNdsUiKitDirty = FALSE;
    gNdsUiKitVisibleObjectCount = 0u;
    gNdsUiKitEngine = 0xffffffffu;
    gNdsUiKitExitCount++;
}

/* --- Text ---------------------------------------------------------------- */

static u32 ndsUiKitContentKey(const char *text, u32 rgb)
{
    u32 hash = 0x811C9DC5u ^ rgb;
    u32 i;

    for (i = 0u; (i < 64u) && (text[i] != '\0'); i++)
    {
        hash ^= (u32)(u8)text[i];
        hash *= 0x01000193u;
    }
    /* A key of zero is the "slot has no content" sentinel. */
    return (hash != 0u) ? hash : 1u;
}

s32 ndsUiKitSetText(u32 slot, const char *text, u32 rgb)
{
    NdsUiKitTextSlot *state;
    u16 ramp[16];
    u32 key;
    u32 width;
    u32 chunks;
    u32 base;
    s32 cursor;
    s32 i;
    u32 n;

    if ((sNdsUiKitActive == FALSE) || (sNdsUiKitTextResident == FALSE) ||
        (slot >= NDS_UI_KIT_TEXT_SLOTS) ||
        (text == NULL))
    {
        return FALSE;
    }
    state = &sNdsUiKitText[slot];
    key = ndsUiKitContentKey(text, rgb);
    if (key == state->content_key)
    {
        /* A menu that is not changing must not recompose; this counter is how
         * a per-frame recompose would be caught rather than measured. */
        gNdsUiKitTextComposeSkipCount++;
        state->visible = 1u;
        sNdsUiKitDirty = TRUE;
        return TRUE;
    }

    width = ndsUiKitTextWidth(text);
    if (width > NDS_UI_KIT_TEXT_MAX_PX)
    {
        gNdsUiKitTextOverflowCount++;
        width = NDS_UI_KIT_TEXT_MAX_PX;
    }
    chunks = (width + NDS_UI_KIT_TEXT_CHUNK_W - 1u) / NDS_UI_KIT_TEXT_CHUNK_W;
    if (chunks == 0u)
    {
        chunks = 1u;
    }

    /* The source modulates the whole string by one primitive colour and the
     * glyphs are 4-bit intensity, so sixteen premultiplied entries reproduce
     * every pixel the original could draw with one table lookup each. */
    for (n = 0u; n < 16u; n++)
    {
        u32 red = (((rgb >> 16) & 0xffu) * n) / 15u;
        u32 green = (((rgb >> 8) & 0xffu) * n) / 15u;
        u32 blue = ((rgb & 0xffu) * n) / 15u;

        ramp[n] = (n == 0u) ? 0u :
            (u16)(0x8000u | ((blue >> 3) << 10) | ((green >> 3) << 5) |
                  (red >> 3));
    }

    base = sNdsUiKitTextVramBase +
           (slot * NDS_UI_KIT_TEXT_CHUNKS * NDS_UI_KIT_TEXT_CHUNK_BYTES);
    dmaFillHalfWords(0u, ndsUiKitObjAt(base),
                     NDS_UI_KIT_TEXT_CHUNKS * NDS_UI_KIT_TEXT_CHUNK_BYTES);

    cursor = 0;
    for (i = 0; text[i] != '\0'; i++)
    {
        char c = text[i];
        s32 glyph;
        s32 row;
        u32 gw;
        u32 gh;
        const u8 *pixels;
        u32 gy;

        if (c == ' ')
        {
            cursor += 4;
            continue;
        }
        if ((c >= '0') && (c <= '9'))
        {
            cursor += (s32)(c - '0');
            continue;
        }
        glyph = ndsUiKitGlyphIndex(c);
        if (glyph < 0)
        {
            continue;
        }
        gw = kNdsUiKitGlyphMetrics[glyph].width;
        gh = kNdsUiKitGlyphMetrics[glyph].height;
        row = ndsUiKitGlyphRow(c);
        pixels = &sNdsUiKitGlyphs[(u32)glyph * NDS_MN_UI_KIT_GLYPH_CELL_W *
                                  NDS_MN_UI_KIT_GLYPH_CELL_H];
        for (gy = 0u; gy < gh; gy++)
        {
            s32 dest_y = row + (s32)gy;
            u32 gx;

            if ((dest_y < 0) || (dest_y >= (s32)NDS_UI_KIT_TEXT_CHUNK_H))
            {
                continue;
            }
            for (gx = 0u; gx < gw; gx++)
            {
                s32 dest_x = cursor + (s32)gx;
                u32 chunk;
                u16 *cell;
                u8 intensity;

                if ((dest_x < 0) || (dest_x >= (s32)NDS_UI_KIT_TEXT_MAX_PX))
                {
                    continue;
                }
                intensity = pixels[(gy * NDS_MN_UI_KIT_GLYPH_CELL_W) + gx];
                if (intensity == 0u)
                {
                    continue;
                }
                chunk = (u32)dest_x / NDS_UI_KIT_TEXT_CHUNK_W;
                cell = ndsUiKitObjAt(base +
                                     (chunk * NDS_UI_KIT_TEXT_CHUNK_BYTES));
                /* The pack stores nibble*17, so >>4 recovers the source's
                 * 0..15 intensity exactly. */
                cell[((u32)dest_y * NDS_UI_KIT_TEXT_CHUNK_W) +
                     ((u32)dest_x % NDS_UI_KIT_TEXT_CHUNK_W)] =
                    ramp[intensity >> 4];
            }
        }
        cursor += (s32)gw + ndsUiKitSpacing(text, i);
    }

    state->content_key = key;
    state->width_px = (u16)width;
    state->chunks = (u8)chunks;
    state->visible = 1u;
    gNdsUiKitTextComposeCount++;
    sNdsUiKitDirty = TRUE;
    return TRUE;
}

void ndsUiKitMoveText(u32 slot, s32 x, s32 y)
{
    if ((sNdsUiKitActive == FALSE) || (sNdsUiKitTextResident == FALSE) ||
        (slot >= NDS_UI_KIT_TEXT_SLOTS))
    {
        return;
    }
    sNdsUiKitText[slot].x = (s16)x;
    sNdsUiKitText[slot].y = (s16)(y - NDS_UI_KIT_TEXT_BASELINE);
    sNdsUiKitDirty = TRUE;
}

void ndsUiKitHideText(u32 slot)
{
    if ((sNdsUiKitActive == FALSE) || (sNdsUiKitTextResident == FALSE) ||
        (slot >= NDS_UI_KIT_TEXT_SLOTS))
    {
        return;
    }
    sNdsUiKitText[slot].visible = 0u;
    sNdsUiKitDirty = TRUE;
}

/* --- Images -------------------------------------------------------------- */

s32 ndsUiKitSetSprite(u32 slot, u32 image, s32 x, s32 y)
{
    if ((sNdsUiKitActive == FALSE) || (slot >= NDS_UI_KIT_SPRITE_SLOTS) ||
        (image >= NDS_MN_UI_KIT_IMAGE_COUNT) ||
        (sNdsUiKitImageVram[image] == 0xffffffffu))
    {
        return FALSE;
    }
    sNdsUiKitSprites[slot].image = (u8)image;
    sNdsUiKitSprites[slot].x = (s16)x;
    sNdsUiKitSprites[slot].y = (s16)y;
    sNdsUiKitSprites[slot].visible = 1u;
    sNdsUiKitSprites[slot].alpha = 15u;
    sNdsUiKitSprites[slot].scale2x = 0u;
    sNdsUiKitSprites[slot].priority = 0u;
    sNdsUiKitDirty = TRUE;
    return TRUE;
}

/* P2-1N (5): a bitmap OBJ drawn semi-transparent (the DS blends a bitmap OBJ
 * by its per-OBJ alpha attribute, 1..15) and optionally through affine
 * double-size, for art baked at half resolution. Built for the title emblem:
 * a ~30% flat red wash the 1-bit-alpha BG bake provably cannot carry. */
s32 ndsUiKitSetSpriteBlend(u32 slot, u32 image, s32 x, s32 y, u32 alpha,
                           u32 scale2x, u32 priority)
{
    if (ndsUiKitSetSprite(slot, image, x, y) == FALSE)
    {
        return FALSE;
    }
    if (alpha < 1u)
    {
        alpha = 1u;
    }
    if (alpha > 15u)
    {
        alpha = 15u;
    }
    sNdsUiKitSprites[slot].alpha = (u8)alpha;
    sNdsUiKitSprites[slot].scale2x = (scale2x != 0u) ? 1u : 0u;
    if (priority > 3u)
    {
        priority = 3u;
    }
    sNdsUiKitSprites[slot].priority = (u8)priority;
    sNdsUiKitDirty = TRUE;
    return TRUE;
}

void ndsUiKitMoveSprite(u32 slot, s32 x, s32 y)
{
    if ((sNdsUiKitActive == FALSE) || (slot >= NDS_UI_KIT_SPRITE_SLOTS))
    {
        return;
    }
    sNdsUiKitSprites[slot].x = (s16)x;
    sNdsUiKitSprites[slot].y = (s16)y;
    sNdsUiKitDirty = TRUE;
}

void ndsUiKitHideSprite(u32 slot)
{
    if ((sNdsUiKitActive == FALSE) || (slot >= NDS_UI_KIT_SPRITE_SLOTS))
    {
        return;
    }
    sNdsUiKitSprites[slot].visible = 0u;
    sNdsUiKitDirty = TRUE;
}

/* --- Numbers ------------------------------------------------------------- */

/* mnVSModeMakeNumber's own shape: the ones place is placed first at
 * `right_x - 11`, and each further place steps another 11 px left, so the
 * number grows leftward from a fixed right edge and the label in front of it
 * never moves when the value crosses ten. Negative values are clamped to 0,
 * as the source clamps them (mnvsmode.c:1206). */
static u32 ndsUiKitDigitCount(s32 value)
{
    u32 magnitude = (value > 0) ? (u32)value : 0u;
    u32 digits = 1u;

    while (magnitude >= 10u)
    {
        magnitude /= 10u;
        digits++;
    }
    return digits;
}

u32 ndsUiKitNumberWidth(s32 value)
{
    return ndsUiKitDigitCount(value) * (u32)NDS_UI_KIT_DIGIT_PITCH;
}

u32 ndsUiKitSetNumber(u32 slot, u32 slots_available, s32 value, s32 right_x,
                      s32 y)
{
    u32 magnitude = (value > 0) ? (u32)value : 0u;
    u32 places = ndsUiKitDigitCount(value);
    u32 used = 0u;
    s32 x = right_x;

    if (places > slots_available)
    {
        places = slots_available;
    }
    while (used < places)
    {
        x -= NDS_UI_KIT_DIGIT_PITCH;
        if (ndsUiKitSetSprite(slot + used,
                              NDS_MN_UI_KIT_IMAGE_DIGIT_0 + (magnitude % 10u),
                              x, y) == FALSE)
        {
            break;
        }
        magnitude /= 10u;
        used++;
    }
    return used;
}

/* --- Backdrop surfaces (P2-1h) -------------------------------------------
 *
 * The destination is the main engine's BG2 bitmap -- the surface the menu
 * shell already clears on every screen entry -- so this costs no VRAM bank
 * and no per-frame work. See docs/p2/P2-1c-vram-map.md.
 *
 * Rows travel through the same 2 KiB staging buffer the pack load uses, and
 * for the same hardware reason: VRAM drops 8-bit writes and `fread` is a byte
 * path, so every byte lands in main RAM first. */

static u8 sNdsUiKitSurfaceCache[NDS_MN_UI_KIT_SURFACE_CACHE_BYTES]
    __attribute__((aligned(4)));
static u32 sNdsUiKitSurfaceCached = 0xffffffffu;

_Static_assert(NDS_MN_UI_KIT_SURFACE_MAX_ROW_BYTES <= NDS_UI_KIT_STAGING_BYTES,
               "one surface row must fit the staging buffer");

/* One row into BG2, clipped on all four edges.
 *
 * The clipping is not defensive padding: the title emblem's 4/5 origin is
 * y = -2 and its right edge lands at 259, which is the same overhang the
 * source's own 320-wide frame gives it, so a surface that runs off the screen
 * is the CORRECT result and not a bake error. */
static void ndsUiKitSurfaceRow(const u16 *src, u16 *layer, u32 pitch,
                               u32 layer_w, u32 layer_h,
                               const NdsUiKitSurfaceMetric *metric, s32 sy)
{
    s32 dy = (s32)metric->y + sy;
    s32 dx = (s32)metric->x;
    s32 sx = 0;
    s32 count;
    u16 *dst;

    if ((dy < 0) || (dy >= (s32)layer_h))
    {
        return;
    }
    if (dx < 0)
    {
        sx = -dx;
        dx = 0;
    }
    count = (s32)metric->width - sx;
    if (count > ((s32)layer_w - dx))
    {
        count = (s32)layer_w - dx;
    }
    if (count <= 0)
    {
        return;
    }
    dst = layer + ((u32)dy * pitch) + (u32)dx;
    src += sx;
    if (metric->opaque != 0u)
    {
        /* Nothing to key out, so the whole row is one 16-bit DMA. This is the
         * path the 240x176 collage and the 248x184 title take. */
        dmaCopyHalfWords(3, src, dst, (u32)count * sizeof(u16));
    }
    else
    {
        s32 i;

        for (i = 0; i < count; i++)
        {
            u16 texel = src[i];

            if (texel != 0u)
            {
                dst[i] = texel;
            }
        }
    }
}

static s32 ndsUiKitBlitOneSurface(NdsRelocAssetStream *stream, u32 index,
                                  u16 *layer, u32 pitch, u32 layer_w,
                                  u32 layer_h)
{
    const NdsUiKitSurfaceMetric *metric = &kNdsUiKitSurfaceMetrics[index];
    u32 row_bytes = (u32)metric->width * sizeof(u16);
    u32 rows_per_slice = NDS_UI_KIT_STAGING_BYTES / row_bytes;
    u32 hash = 0x811C9DC5u;
    u32 row = 0u;

    while (row < (u32)metric->height)
    {
        u32 rows = (u32)metric->height - row;
        u32 i;

        if (rows > rows_per_slice)
        {
            rows = rows_per_slice;
        }
        if (ndsRelocAssetStreamRead(stream,
                                    metric->offset + (row * row_bytes),
                                    sNdsUiKitStaging,
                                    rows * row_bytes) == FALSE)
        {
            gNdsUiKitSurfaceReadFailCount++;
            return FALSE;
        }
        hash = ndsUiKitHashFold(hash, sNdsUiKitStaging, rows * row_bytes);
        DC_FlushRange(sNdsUiKitStaging, rows * row_bytes);
        for (i = 0u; i < rows; i++)
        {
            ndsUiKitSurfaceRow(
                (const u16 *)(sNdsUiKitStaging + (i * row_bytes)),
                layer, pitch, layer_w, layer_h, metric, (s32)(row + i));
        }
        gNdsUiKitSurfaceBytes += rows * row_bytes;
        row += rows;
    }

    gNdsUiKitSurfaceLastHash = hash;
    if (hash != metric->fnv32)
    {
        /* Per surface, not per pack: a stale or truncated surface pack is a
         * counted failure here rather than a garbled backdrop nobody can
         * attribute. */
        gNdsUiKitSurfaceHashMismatchCount++;
        return FALSE;
    }
    gNdsUiKitSurfaceBlitCount++;
    return TRUE;
}

static s32 ndsUiKitBlitSurfacesLayer(const u8 *surfaces, u32 count,
                                     sb32 is_foreground)
{
    NdsRelocAssetStream stream;
    u32 start = cpuGetTiming();
    u32 pitch = 0u;
    u32 layer_w = 0u;
    u32 layer_h = 0u;
    u16 *layer;
    u32 i;
    s32 ok = TRUE;

    if ((surfaces == NULL) || (count == 0u))
    {
        return FALSE;
    }
    layer = ndsPlatformGetOriginalSpriteOverlayLayer(is_foreground,
                                                     &pitch, &layer_w,
                                                     &layer_h, NULL);
    if ((layer == NULL) || (pitch == 0u))
    {
        /* The overlay is disabled or unmapped. The screen still runs and still
         * reaches its successor; it just has no backdrop, and this counter is
         * the difference between that and a blit that never fired. */
        gNdsUiKitSurfaceNoLayerCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamOpen(&stream, NDS_UI_KIT_SURFACE_PATH) == FALSE)
    {
        gNdsUiKitSurfaceReadFailCount++;
        return FALSE;
    }
    gNdsUiKitSurfaceOpenCount++;
    for (i = 0u; i < count; i++)
    {
        if (surfaces[i] >= NDS_MN_UI_KIT_SURFACE_COUNT)
        {
            ok = FALSE;
            continue;
        }
        if (ndsUiKitBlitOneSurface(&stream, surfaces[i], layer, pitch, layer_w,
                                   layer_h) == FALSE)
        {
            ok = FALSE;
        }
    }
    ndsRelocAssetStreamClose(&stream);
    gNdsUiKitSurfaceTicks += cpuGetTiming() - start;
    return ok;
}

s32 ndsUiKitBlitSurfaces(const u8 *surfaces, u32 count)
{
    return ndsUiKitBlitSurfacesLayer(surfaces, count, FALSE);
}

s32 ndsUiKitBlitForegroundSurfaces(const u8 *surfaces, u32 count)
{
    return ndsUiKitBlitSurfacesLayer(surfaces, count, TRUE);
}

void ndsUiKitClearForegroundRect(s32 x, s32 y, u32 width, u32 height)
{
    u32 pitch = 0u;
    u32 layer_w = 0u;
    u32 layer_h = 0u;
    u16 *layer = ndsPlatformGetOriginalSpriteOverlayLayer(
        TRUE, &pitch, &layer_w, &layer_h, NULL);
    s32 x0 = x;
    s32 y0 = y;
    s32 x1 = x + (s32)width;
    s32 y1 = y + (s32)height;
    s32 row;

    if ((layer == NULL) || (pitch == 0u) || (width == 0u) || (height == 0u))
    {
        return;
    }
    if (x0 < 0) { x0 = 0; }
    if (y0 < 0) { y0 = 0; }
    if (x1 > (s32)layer_w) { x1 = (s32)layer_w; }
    if (y1 > (s32)layer_h) { y1 = (s32)layer_h; }
    if ((x0 >= x1) || (y0 >= y1))
    {
        return;
    }
    for (row = y0; row < y1; row++)
    {
        dmaFillHalfWords(0, &layer[(u32)row * pitch + (u32)x0],
                         (u32)(x1 - x0) * sizeof(u16));
    }
}

/* P2-1i -- the title's fire atlas. NOT a screen-space backdrop: it is the
 * thirty mnTitleMakeFire states tiled 5x6 into one 255x252 sheet written
 * into the FOREGROUND overlay bitmap (BG3), full-height -- the backdrop
 * path clips every row to the 192-row screen, which is correct for a
 * backdrop and would throw 60 of this sheet's rows away. The BG3 affine
 * then reads one 51x42 cell per frame (ndsPlatformSetTitleFireFrame).
 *
 * The caller must have cleared the layer first (the shell's entry path
 * clears both overlay bitmaps), because this writes only rows 0..251,
 * columns 0..254: column 255 and rows 252..255 stay at the clear's
 * transparent 0, which is also what keeps the affine window from ever
 * sampling garbage at the sheet's edge. */
s32 ndsUiKitBlitFireAtlas(void)
{
    const NdsUiKitSurfaceMetric *metric =
        &kNdsUiKitSurfaceMetrics[NDS_MN_UI_KIT_SURFACE_TITLE_FIRE_ATLAS];
    NdsRelocAssetStream stream;
    u32 start = cpuGetTiming();
    u32 row_bytes = (u32)metric->width * sizeof(u16);
    u32 rows_per_slice = NDS_UI_KIT_STAGING_BYTES / row_bytes;
    u32 hash = 0x811C9DC5u;
    u32 pitch = 0u;
    u32 row = 0u;
    u16 *layer;

    layer = ndsPlatformGetOriginalSpriteOverlayLayer(TRUE, &pitch, NULL, NULL,
                                                     NULL);
    if ((layer == NULL) || (pitch == 0u))
    {
        gNdsUiKitSurfaceNoLayerCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamOpen(&stream, NDS_UI_KIT_SURFACE_PATH) == FALSE)
    {
        gNdsUiKitSurfaceReadFailCount++;
        return FALSE;
    }
    gNdsUiKitSurfaceOpenCount++;
    while (row < (u32)metric->height)
    {
        u32 rows = (u32)metric->height - row;
        u32 i;

        if (rows > rows_per_slice)
        {
            rows = rows_per_slice;
        }
        if (ndsRelocAssetStreamRead(&stream,
                                    metric->offset + (row * row_bytes),
                                    sNdsUiKitStaging,
                                    rows * row_bytes) == FALSE)
        {
            gNdsUiKitSurfaceReadFailCount++;
            ndsRelocAssetStreamClose(&stream);
            return FALSE;
        }
        hash = ndsUiKitHashFold(hash, sNdsUiKitStaging, rows * row_bytes);
        DC_FlushRange(sNdsUiKitStaging, rows * row_bytes);
        for (i = 0u; i < rows; i++)
        {
            /* Whole rows at the layer's pitch, no clip: the sheet is opaque
             * by construction (every atlas texel carries bit 15), so this is
             * the same one-DMA-per-row shape the opaque backdrop rows take. */
            dmaCopyHalfWords(3, sNdsUiKitStaging + (i * row_bytes),
                             layer + ((row + i) * pitch), row_bytes);
        }
        gNdsUiKitSurfaceBytes += rows * row_bytes;
        row += rows;
    }
    ndsRelocAssetStreamClose(&stream);

    gNdsUiKitSurfaceLastHash = hash;
    if (hash != metric->fnv32)
    {
        gNdsUiKitSurfaceHashMismatchCount++;
        return FALSE;
    }
    /* ITS OWN COUNTER, not gNdsUiKitSurfaceBlitCount: the loop verifier's
     * standing invariant is one BACKDROP blit per backdrop-screen entry, and
     * this sheet is not a backdrop. Counting it there made blit=4 against
     * three backdrop entries and reddened the arm. The shared failure counters
     * above stay shared on purpose -- they are asserted zero either way. */
    gNdsUiKitFireAtlasBlitCount++;
    gNdsUiKitSurfaceTicks += cpuGetTiming() - start;
    return TRUE;
}

s32 ndsUiKitCacheSurface(u32 surface)
{
    NdsRelocAssetStream stream;
    const NdsUiKitSurfaceMetric *metric;
    u32 hash = 0x811C9DC5u;

    sNdsUiKitSurfaceCached = 0xffffffffu;
    if (surface >= NDS_MN_UI_KIT_SURFACE_COUNT)
    {
        return FALSE;
    }
    metric = &kNdsUiKitSurfaceMetrics[surface];
    if (metric->bytes > (u32)sizeof(sNdsUiKitSurfaceCache))
    {
        /* The manifest sizes this buffer from the bake's own `cacheable` set,
         * so reaching here means a caller asked to cache a surface the bake
         * did not mark -- refused and counted, never a buffer overrun. */
        return FALSE;
    }
    if (ndsRelocAssetStreamOpen(&stream, NDS_UI_KIT_SURFACE_PATH) == FALSE)
    {
        gNdsUiKitSurfaceReadFailCount++;
        return FALSE;
    }
    gNdsUiKitSurfaceOpenCount++;
    if (ndsRelocAssetStreamRead(&stream, metric->offset,
                                sNdsUiKitSurfaceCache, metric->bytes) == FALSE)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsUiKitSurfaceReadFailCount++;
        return FALSE;
    }
    ndsRelocAssetStreamClose(&stream);
    hash = ndsUiKitHashFold(hash, sNdsUiKitSurfaceCache, metric->bytes);
    gNdsUiKitSurfaceLastHash = hash;
    if (hash != metric->fnv32)
    {
        gNdsUiKitSurfaceHashMismatchCount++;
        return FALSE;
    }
    sNdsUiKitSurfaceCached = surface;
    gNdsUiKitSurfaceCacheCount++;
    return TRUE;
}

/* Both toggles walk the cached surface's own rows, so a blink costs no
 * NitroFS work at all -- one open is more than a whole 60 Hz frame's budget.
 * `field_texel` is the field the bake composited the surface over, which is
 * what makes erasing it a fill rather than a second stored image. */
static void ndsUiKitToggleCachedSurface(u16 field_texel, s32 draw)
{
    const NdsUiKitSurfaceMetric *metric;
    u32 pitch = 0u;
    u32 layer_w = 0u;
    u32 layer_h = 0u;
    u16 *layer;
    u32 row;

    if (sNdsUiKitSurfaceCached >= NDS_MN_UI_KIT_SURFACE_COUNT)
    {
        return;
    }
    layer = ndsPlatformGetOriginalSpriteOverlayLayer(FALSE, &pitch, &layer_w,
                                                     &layer_h, NULL);
    if ((layer == NULL) || (pitch == 0u))
    {
        gNdsUiKitSurfaceNoLayerCount++;
        return;
    }
    metric = &kNdsUiKitSurfaceMetrics[sNdsUiKitSurfaceCached];
    for (row = 0u; row < (u32)metric->height; row++)
    {
        if (draw != FALSE)
        {
            ndsUiKitSurfaceRow(
                (const u16 *)(sNdsUiKitSurfaceCache +
                              (row * (u32)metric->width * sizeof(u16))),
                layer, pitch, layer_w, layer_h, metric, (s32)row);
        }
        else
        {
            s32 dy = (s32)metric->y + (s32)row;
            s32 dx = (s32)metric->x;
            s32 count;
            s32 i;

            if ((dy < 0) || (dy >= (s32)layer_h))
            {
                continue;
            }
            if (dx < 0)
            {
                dx = 0;
            }
            count = (s32)metric->width;
            if (count > ((s32)layer_w - dx))
            {
                count = (s32)layer_w - dx;
            }
            for (i = 0; i < count; i++)
            {
                layer[((u32)dy * pitch) + (u32)dx + (u32)i] = field_texel;
            }
        }
    }
    if (draw != FALSE)
    {
        gNdsUiKitSurfaceDrawCachedCount++;
    }
    else
    {
        gNdsUiKitSurfaceEraseCachedCount++;
    }
}

void ndsUiKitDrawCachedSurface(void)
{
    ndsUiKitToggleCachedSurface(0u, TRUE);
}

/* P2-1N (3): draw a keyed SUB-RECTANGLE of the cached surface at an arbitrary
 * destination, clipped horizontally to [clip_x0, clip_x1). Purpose-built for
 * the CSS slot doors — the cached CSS_DOORS strip holds both card halves and
 * the shutter draws each at its slid position inside the gate box, so a
 * sliding frame costs no NitroFS work for the doors at all. Key texel 0 is
 * skipped exactly as ndsUiKitSurfaceRow's keyed branch skips it. */
s32 ndsUiKitDrawCachedSub(u32 src_x, u32 src_w, s32 dest_x, s32 dest_y,
                          s32 clip_x0, s32 clip_x1)
{
    const NdsUiKitSurfaceMetric *metric;
    u32 pitch = 0u;
    u32 layer_w = 0u;
    u32 layer_h = 0u;
    u16 *layer;
    u32 row;

    if (sNdsUiKitSurfaceCached >= NDS_MN_UI_KIT_SURFACE_COUNT)
    {
        return FALSE;
    }
    layer = ndsPlatformGetOriginalSpriteOverlayLayer(FALSE, &pitch, &layer_w,
                                                     &layer_h, NULL);
    if ((layer == NULL) || (pitch == 0u))
    {
        gNdsUiKitSurfaceNoLayerCount++;
        return FALSE;
    }
    metric = &kNdsUiKitSurfaceMetrics[sNdsUiKitSurfaceCached];
    if ((src_x >= (u32)metric->width) ||
        ((src_x + src_w) > (u32)metric->width))
    {
        return FALSE;
    }
    if (clip_x0 < 0)
    {
        clip_x0 = 0;
    }
    if (clip_x1 > (s32)layer_w)
    {
        clip_x1 = (s32)layer_w;
    }
    for (row = 0u; row < (u32)metric->height; row++)
    {
        const u16 *src = (const u16 *)(sNdsUiKitSurfaceCache +
                                       ((row * (u32)metric->width) +
                                        src_x) * sizeof(u16));
        s32 dy = dest_y + (s32)row;
        s32 dx = dest_x;
        s32 sx = 0;
        s32 count = (s32)src_w;
        s32 i;

        if ((dy < 0) || (dy >= (s32)layer_h))
        {
            continue;
        }
        if (dx < clip_x0)
        {
            sx = clip_x0 - dx;
            dx = clip_x0;
            count -= sx;
        }
        if (count > (clip_x1 - dx))
        {
            count = clip_x1 - dx;
        }
        for (i = 0; i < count; i++)
        {
            u16 texel = src[sx + i];

            if (texel != 0u)
            {
                layer[((u32)dy * pitch) + (u32)dx + (u32)i] = texel;
            }
        }
    }
    gNdsUiKitSurfaceDrawCachedCount++;
    return TRUE;
}

void ndsUiKitEraseCachedSurface(u16 field_texel)
{
    ndsUiKitToggleCachedSurface(field_texel, FALSE);
}

/* --- P2-1k (d): the title's pop animation -------------------------------- */

#include "generated/mn_title_anim.generated.inc"

/* The two generators derive the band independently -- the kit from
 * TITLE_SCREEN's own placements, the decoder from the same function on the same
 * bake -- so a disagreement means one of them ran against a stale tree. Free to
 * check, and the alternative is a mismatch that only shows as art vanishing at
 * one edge. */
_Static_assert(NDS_MN_TITLE_ANIM_TOP == NDS_MN_UI_KIT_TITLE_ANIM_TOP &&
                   NDS_MN_TITLE_ANIM_BOTTOM == NDS_MN_UI_KIT_TITLE_ANIM_BOTTOM,
               "the pose table and the surface bake disagree about the band");
_Static_assert(NDS_MN_TITLE_ANIM_SETTLED_W <= 256u,
               "the settled rectangle must fit the destination row");
_Static_assert(NDS_MN_TITLE_ANIM_MAX_WIDTH <= 256u,
               "a pose is wider than the scaled-row buffer");

/* One destination row, scaled out of a piece's raster before it reaches the
 * layer. It exists for the ROW CACHE: every pose here upscales vertically at
 * some point (the peak frame's pieces run to 3.5x), so consecutive destination
 * rows keep landing on the same source row, and scaling it once instead of per
 * row is the difference between ~10 and ~6 cycles a texel on the frame that
 * decides the cadence. */
static u16 sNdsUiKitTitleAnimLine[256] __attribute__((aligned(4)));

static const u16 *sNdsUiKitTitleAnimPiece[NDS_MN_TITLE_ANIM_PIECES];
static const u16 *sNdsUiKitTitleAnimSettled;
static NdsUiKitTitleAnimRect sNdsUiKitTitleAnimDirty;
static u32 sNdsUiKitTitleAnimReady;

NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimArmCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimLoadFailCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimFrameCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimSettleCount;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimPose;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimBytes32;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimDrawTexels;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimEraseTexels;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimTicks;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimMaxTicks;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimMaxPose;
NDS_UI_KIT_PUBLISHED volatile u32 gNdsUiKitTitleAnimEmptyPoseCount;

u32 ndsUiKitTitleAnimBytes(void)
{
    u32 total = kNdsUiKitSurfaceMetrics[
        NDS_MN_UI_KIT_SURFACE_TITLE_WORDMARK].bytes;
    u32 i;

    for (i = 0u; i < NDS_MN_TITLE_ANIM_PIECES; i++)
    {
        total += kNdsUiKitSurfaceMetrics[kNdsUiKitTitleAnimSurfaces[i]].bytes;
    }
    return total;
}

static const u16 *ndsUiKitTitleAnimRead(NdsRelocAssetStream *stream,
                                        u32 surface, u8 *dst)
{
    const NdsUiKitSurfaceMetric *metric = &kNdsUiKitSurfaceMetrics[surface];
    u32 hash;

    /* Straight into the caller's block, not through sNdsUiKitStaging: that
     * buffer exists because VRAM drops 8-bit writes, and this destination is
     * main RAM. */
    if (ndsRelocAssetStreamRead(stream, metric->offset, dst,
                                metric->bytes) == FALSE)
    {
        gNdsUiKitSurfaceReadFailCount++;
        return NULL;
    }
    hash = ndsUiKitHashFold(0x811C9DC5u, dst, metric->bytes);
    gNdsUiKitSurfaceLastHash = hash;
    if (hash != metric->fnv32)
    {
        gNdsUiKitSurfaceHashMismatchCount++;
        return NULL;
    }
    gNdsUiKitSurfaceBytes += metric->bytes;
    return (const u16 *)dst;
}

s32 ndsUiKitTitleAnimLoad(void *block, u32 bytes)
{
    NdsRelocAssetStream stream;
    u8 *cursor = (u8 *)block;
    u32 need = ndsUiKitTitleAnimBytes();
    u32 i;

    sNdsUiKitTitleAnimReady = 0u;
    if ((block == NULL) || (bytes < need))
    {
        gNdsUiKitTitleAnimLoadFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamOpen(&stream, NDS_UI_KIT_SURFACE_PATH) == FALSE)
    {
        gNdsUiKitSurfaceReadFailCount++;
        gNdsUiKitTitleAnimLoadFailCount++;
        return FALSE;
    }
    gNdsUiKitSurfaceOpenCount++;
    for (i = 0u; i < NDS_MN_TITLE_ANIM_PIECES; i++)
    {
        u32 surface = kNdsUiKitTitleAnimSurfaces[i];

        sNdsUiKitTitleAnimPiece[i] =
            ndsUiKitTitleAnimRead(&stream, surface, cursor);
        if (sNdsUiKitTitleAnimPiece[i] == NULL)
        {
            ndsRelocAssetStreamClose(&stream);
            gNdsUiKitTitleAnimLoadFailCount++;
            return FALSE;
        }
        cursor += kNdsUiKitSurfaceMetrics[surface].bytes;
    }
    sNdsUiKitTitleAnimSettled = ndsUiKitTitleAnimRead(
        &stream, NDS_MN_UI_KIT_SURFACE_TITLE_WORDMARK, cursor);
    ndsRelocAssetStreamClose(&stream);
    if (sNdsUiKitTitleAnimSettled == NULL)
    {
        gNdsUiKitTitleAnimLoadFailCount++;
        return FALSE;
    }
    /* WHAT POSE 1 ERASES. The screen entry has already blitted the whole static
     * title, settled wordmark included, so the first thing the animation owes
     * the screen is the removal of a layout the source has not reached yet. */
    sNdsUiKitTitleAnimDirty.x = (u16)NDS_MN_TITLE_ANIM_SETTLED_X;
    sNdsUiKitTitleAnimDirty.y = (u16)NDS_MN_TITLE_ANIM_SETTLED_Y;
    sNdsUiKitTitleAnimDirty.width = (u16)NDS_MN_TITLE_ANIM_SETTLED_W;
    sNdsUiKitTitleAnimDirty.height = (u16)NDS_MN_TITLE_ANIM_SETTLED_H;
    sNdsUiKitTitleAnimReady = 1u;
    gNdsUiKitTitleAnimBytes32 = need;
    gNdsUiKitTitleAnimPose = 0u;
    gNdsUiKitTitleAnimArmCount++;
    return TRUE;
}

s32 ndsUiKitTitleAnimActive(void)
{
    return (sNdsUiKitTitleAnimReady != 0u) ? TRUE : FALSE;
}

void ndsUiKitTitleAnimEnd(void)
{
    /* The arena block belongs to the scene, and the scene teardown rewinds it.
     * Dropping the pointers is what stops a stale block being drawn out of on
     * the next entry if a load ever fails after a success. */
    sNdsUiKitTitleAnimReady = 0u;
    sNdsUiKitTitleAnimSettled = NULL;
}

/* P2-1L (3). ONE STORE PER DESTINATION TEXEL, PER FRAME -- and that is a
 * correctness requirement here, not a speed choice.
 *
 * THE MEASUREMENT THAT FORCED IT (2026-08-19, free-play ROM
 * `smash64ds-p2-shell-freeplay-hwtri`, presents 28..38): a gdb dump of BG2's
 * bitmap taken at `ndsPlatformEndFrame` is BIT-IDENTICAL to an offline
 * evaluation of the same pose from the same table and the same rasters -- band
 * rows 8..157, zero differing texels at presents 28 and 30 -- while the
 * PHOTOGRAPH of the very same stop shows the black drop-shadow cutout alone,
 * with no letters at all. The blit was never wrong. The panel was showing a
 * state the blit passes through.
 *
 * It passes through it because this layer is BG2's bitmap and the LCD scans it
 * out LIVE, one row every ~2,130 ticks, while the CPU writes it. The old shape
 * was: erase the whole dirty rectangle, then paint five pieces in five
 * top-to-bottom passes, the first of them the unkeyed drop-shadow cutout. Both
 * the beam and each pass sweep downward, so for every band row there is a
 * window in which the beam has already passed the row that the cutout pass
 * reached but the letter passes have not -- that row is displayed BLACK -- and
 * an earlier window, between the erase and the cutout, in which it is displayed
 * EMPTY. "Some poses render invisible/black" is exactly those two windows, and
 * no pose table can avoid them: the animation's own peak frame costs 712,704
 * ticks against a 560,190-tick sweep.
 *
 * So a destination row is now composed WHOLE in `sNdsUiKitTitleAnimLine` --
 * cleared, then the pieces that cover it, in the source's own draw order -- and
 * reaches the layer in a single left-to-right run. Whatever the beam catches is
 * either the previous pose's row or this pose's row; never a half-built one.
 * It is also less work than what it replaces: the union rectangle is stored
 * once instead of an erase plus five overlapping piece rectangles.
 */
static u32 sNdsUiKitTitleAnimSrcRow[NDS_MN_TITLE_ANIM_PIECES];

static u32 ndsUiKitTitleAnimComposeRow(const NdsUiKitTitleAnimPose *frame,
                                       u32 y, u32 x0, u32 width)
{
    u32 sampled = 0u;
    u32 row[NDS_MN_TITLE_ANIM_PIECES];
    u32 same = 1u;
    u32 i;

    /* THE ROW CACHE, KEPT. Every pose upscales somewhere -- the peak frame runs
     * to 3.5x -- so consecutive destination rows keep landing on the same
     * source row of every piece that covers them. When none of the five moved,
     * the composed line is the one already in the buffer and the only work left
     * is the store. This is the same saving the per-piece loop used to take;
     * composing a whole row at a time just moves the test up one level. */
    for (i = 0u; i < NDS_MN_TITLE_ANIM_PIECES; i++)
    {
        const NdsUiKitTitleAnimPose *piece = &frame[i];

        if ((piece->width == 0u) || (y < (u32)piece->y) ||
            (y >= ((u32)piece->y + piece->height)))
        {
            row[i] = 0xffffffffu;
        }
        else
        {
            /* The same walk the pose table is baked against, expressed
             * closed-form because the row loop no longer carries an accumulator
             * per piece: src_y + row * step_y is what `sy += step_y` reaches,
             * exactly, and the largest product a 149-row pose can make is under
             * 2^24. */
            row[i] = (((u32)piece->src_y +
                       ((y - (u32)piece->y) * piece->step_y)) >> 8);
        }
        if (row[i] != sNdsUiKitTitleAnimSrcRow[i])
        {
            same = 0u;
        }
    }
    if (same != 0u)
    {
        return 0u;
    }
    for (i = 0u; i < width; i += 2u)
    {
        *(u32 *)&sNdsUiKitTitleAnimLine[i] = 0u;
    }
    for (i = 0u; i < NDS_MN_TITLE_ANIM_PIECES; i++)
    {
        const NdsUiKitTitleAnimPose *piece = &frame[i];
        const u16 *src;
        u32 base;
        u32 sx;
        u32 j;

        sNdsUiKitTitleAnimSrcRow[i] = row[i];
        if (row[i] == 0xffffffffu)
        {
            continue;
        }
        src = sNdsUiKitTitleAnimPiece[i] +
              (row[i] *
               kNdsUiKitSurfaceMetrics[kNdsUiKitTitleAnimSurfaces[i]].width);
        base = (u32)piece->x - x0;
        sx = piece->src_x;
        if (i == 0u)
        {
            /* THE FIRST PIECE NEEDS NO KEY, and it is an invariant rather than
             * a guess: the line was just cleared, so its own transparent texels
             * write zero over zero. The drop-shadow cutout is the largest of
             * the five and carries roughly a third of the peak frame. */
            for (j = 0u; j < piece->width; j++)
            {
                sNdsUiKitTitleAnimLine[base + j] = src[sx >> 8];
                sx += piece->step_x;
            }
        }
        else
        {
            for (j = 0u; j < piece->width; j++)
            {
                u16 texel = src[sx >> 8];

                sx += piece->step_x;
                if (texel != 0u)
                {
                    sNdsUiKitTitleAnimLine[base + j] = texel;
                }
            }
        }
        sampled += piece->width;
    }
    return sampled;
}

/* The composed run, into the layer. `x0` and `width` are both even and the line
 * buffer is 4-aligned, so every texel pair is one aligned word store; the OR is
 * what lets the caller assert the pose put something on the screen without a
 * second pass over the row. */
static u32 ndsUiKitTitleAnimStoreRow(u16 *layer, u32 pitch, u32 y, u32 x0,
                                     u32 width)
{
    u32 *dst = (u32 *)(layer + (y * pitch) + x0);
    const u32 *src = (const u32 *)sNdsUiKitTitleAnimLine;
    u32 words = width >> 1;
    u32 seen = 0u;
    u32 i;

    for (i = 0u; i < words; i++)
    {
        u32 pair = src[i];

        seen |= pair;
        dst[i] = pair;
    }
    return seen;
}

/* The union of what the previous pose left dirty and what this pose covers,
 * clamped to the band and rounded out to an even column so the store above is
 * word-aligned at both ends. Composing the union in one pass is what folds the
 * separate erase away: a row of it that no piece covers composes to zeros, and
 * zero IS the erase. */
static void ndsUiKitTitleAnimUnion(const NdsUiKitTitleAnimRect *a,
                                   const NdsUiKitTitleAnimRect *b,
                                   u32 *out_x, u32 *out_y,
                                   u32 *out_w, u32 *out_h)
{
    u32 left = 256u;
    u32 top = (u32)NDS_MN_TITLE_ANIM_BOTTOM;
    u32 right = 0u;
    u32 bottom = 0u;
    const NdsUiKitTitleAnimRect *rects[2];
    u32 i;

    rects[0] = a;
    rects[1] = b;
    for (i = 0u; i < 2u; i++)
    {
        const NdsUiKitTitleAnimRect *r = rects[i];

        /* AN EMPTY RECTANGLE IS NOT A RECTANGLE AT THE ORIGIN, and the
         * distinction is load-bearing: `kNdsUiKitTitleAnimRects[0]` is
         * `{0,0,0,0}` because pose 1 shows no label yet, and folding that into
         * a min/max union drags the union's top-left to (0,0) -- which reaches
         * rows 0..7, where the (f2) upper border band lives, and clears it.
         * That is exactly what the first draft of this composer did: the band
         * was gone from the layer at every present, 1,702 texels, while every
         * band row still matched the reference. */
        if ((r->width == 0u) || (r->height == 0u))
        {
            continue;
        }
        if ((u32)r->x < left) { left = (u32)r->x; }
        if ((u32)r->y < top) { top = (u32)r->y; }
        if (((u32)r->x + r->width) > right) { right = (u32)r->x + r->width; }
        if (((u32)r->y + r->height) > bottom)
        {
            bottom = (u32)r->y + r->height;
        }
    }
    left &= ~1u;
    right = (right + 1u) & ~1u;
    if (right > 256u)
    {
        right = 256u;
    }
    /* THE BAND IS THE ANIMATION'S WHOLE WORLD (P2-1k (f2)): rows outside it
     * belong to the two edge-anchored bands, which nothing here may write. The
     * pose table is already clipped to it; clamping here as well is what makes
     * a future table or dirty-rectangle mistake unable to destroy them. */
    if (top < (u32)NDS_MN_TITLE_ANIM_TOP) { top = (u32)NDS_MN_TITLE_ANIM_TOP; }
    if (bottom > (u32)NDS_MN_TITLE_ANIM_BOTTOM)
    {
        bottom = (u32)NDS_MN_TITLE_ANIM_BOTTOM;
    }
    *out_x = left;
    *out_y = top;
    *out_w = (right > left) ? (right - left) : 0u;
    *out_h = (bottom > top) ? (bottom - top) : 0u;
}

/* Pose 51. Same contract as the animated poses -- one whole-row store inside
 * the band -- with the settled composite standing in for the five pieces. The
 * rows ABOVE the band are the exception and stay a keyed write: they hold the
 * (f2) border band, nothing erased them, so there is no partial state for the
 * beam to catch and an unkeyed run would destroy art the animation does not
 * own. */
static u32 ndsUiKitTitleAnimBlitSettled(u16 *layer, u32 pitch,
                                        const NdsUiKitTitleAnimRect *dirty)
{
    const NdsUiKitSurfaceMetric *metric =
        &kNdsUiKitSurfaceMetrics[NDS_MN_UI_KIT_SURFACE_TITLE_WORDMARK];
    NdsUiKitTitleAnimRect rest;
    u32 x0, y0, width, height;
    u32 stored = 0u;
    u32 seen = 0u;
    u32 row;

    for (row = 0u; row < (u32)metric->height; row++)
    {
        const u16 *src;
        u16 *dst;
        u32 i;
        s32 dy = (s32)metric->y + (s32)row;

        if ((dy < 0) || (dy >= NDS_MN_TITLE_ANIM_TOP))
        {
            continue;
        }
        src = sNdsUiKitTitleAnimSettled + (row * metric->width);
        dst = layer + ((u32)dy * pitch) + (u32)metric->x;
        for (i = 0u; i < (u32)metric->width; i++)
        {
            if (src[i] != 0u)
            {
                dst[i] = src[i];
                seen |= src[i];
            }
        }
    }

    {
        s32 rest_top = ((s32)metric->y > NDS_MN_TITLE_ANIM_TOP) ?
                       (s32)metric->y : NDS_MN_TITLE_ANIM_TOP;
        s32 rest_bottom = (s32)metric->y + (s32)metric->height;

        rest.x = (u16)metric->x;
        rest.y = (u16)rest_top;
        rest.width = (u16)metric->width;
        rest.height = (u16)((rest_bottom > rest_top) ?
                            (rest_bottom - rest_top) : 0);
    }
    ndsUiKitTitleAnimUnion(dirty, &rest, &x0, &y0, &width, &height);
    for (row = 0u; row < height; row++)
    {
        u32 y = y0 + row;
        s32 sy = (s32)y - (s32)metric->y;
        u32 i;

        for (i = 0u; i < width; i += 2u)
        {
            *(u32 *)&sNdsUiKitTitleAnimLine[i] = 0u;
        }
        if ((sy >= 0) && (sy < (s32)metric->height))
        {
            const u16 *src = sNdsUiKitTitleAnimSettled +
                             ((u32)sy * metric->width);
            u32 base = (u32)metric->x - x0;

            for (i = 0u; i < (u32)metric->width; i++)
            {
                sNdsUiKitTitleAnimLine[base + i] = src[i];
            }
        }
        seen |= ndsUiKitTitleAnimStoreRow(layer, pitch, y, x0, width);
        stored += width;
    }
    gNdsUiKitTitleAnimDrawTexels += (u32)metric->width * metric->height;
    gNdsUiKitTitleAnimEraseTexels += stored;
    return seen;
}

void ndsUiKitTitleAnimDraw(u32 pose)
{
    u32 start = cpuGetTiming();
    u32 pitch = 0u;
    u32 layer_w = 0u;
    u32 layer_h = 0u;
    u16 *layer;
    u32 i;
    u32 seen = 0u;
    u32 elapsed;

    if (sNdsUiKitTitleAnimReady == 0u)
    {
        return;
    }
    if (pose < 1u)
    {
        pose = 1u;
    }
    if (pose == gNdsUiKitTitleAnimPose)
    {
        return;
    }
    layer = ndsPlatformGetOriginalSpriteOverlayLayer(FALSE, &pitch, &layer_w,
                                                     &layer_h, NULL);
    if ((layer == NULL) || (pitch == 0u))
    {
        gNdsUiKitSurfaceNoLayerCount++;
        return;
    }
    if (pose >= NDS_MN_TITLE_ANIM_SETTLE)
    {
        /* mnTitleSetEndLayout, tic 220. The five SObjs go back to their desc
         * positions at scale 1 and the process ends; here the one composite the
         * bake proves identical to the static title lands, and the animation
         * stops owning the screen. */
        pose = NDS_MN_TITLE_ANIM_SETTLE;
        seen = ndsUiKitTitleAnimBlitSettled(layer, pitch,
                                            &sNdsUiKitTitleAnimDirty);
        sNdsUiKitTitleAnimReady = 0u;
        gNdsUiKitTitleAnimSettleCount++;
    }
    else
    {
        const NdsUiKitTitleAnimPose *frame =
            &kNdsUiKitTitleAnimPoses[(pose - 1u) * NDS_MN_TITLE_ANIM_PIECES];
        u32 x0, y0, width, height;
        u32 sampled = 0u;

        ndsUiKitTitleAnimUnion(&sNdsUiKitTitleAnimDirty,
                               &kNdsUiKitTitleAnimRects[pose - 1u],
                               &x0, &y0, &width, &height);
        /* INVALIDATE THE ROW CACHE PER POSE, not per row: the union's origin
         * and width move between poses, so a line composed for the previous
         * pose's x0 is the wrong line even when every source row matches. */
        for (i = 0u; i < NDS_MN_TITLE_ANIM_PIECES; i++)
        {
            sNdsUiKitTitleAnimSrcRow[i] = 0xfffffffeu;
        }
        for (i = 0u; i < height; i++)
        {
            sampled += ndsUiKitTitleAnimComposeRow(frame, y0 + i, x0, width);
            seen |= ndsUiKitTitleAnimStoreRow(layer, pitch, y0 + i, x0, width);
        }
        gNdsUiKitTitleAnimDrawTexels += sampled;
        gNdsUiKitTitleAnimEraseTexels += width * height;
        sNdsUiKitTitleAnimDirty = kNdsUiKitTitleAnimRects[pose - 1u];
    }
    /* Pose 1's five table entries are all zero-width -- the source has shown no
     * label yet -- so it is the one pose that legitimately stores nothing. Any
     * other is the defect this counter exists to fail on. */
    if ((seen == 0u) && (pose > 1u))
    {
        gNdsUiKitTitleAnimEmptyPoseCount++;
    }
    gNdsUiKitTitleAnimPose = pose;
    gNdsUiKitTitleAnimFrameCount++;
    elapsed = cpuGetTiming() - start;
    gNdsUiKitTitleAnimTicks += elapsed;
    if (elapsed > gNdsUiKitTitleAnimMaxTicks)
    {
        gNdsUiKitTitleAnimMaxTicks = elapsed;
        gNdsUiKitTitleAnimMaxPose = pose;
    }
}

/* --- Audio --------------------------------------------------------------- */

void ndsUiKitSfx(u32 cue)
{
    if (cue >= NDS_UI_KIT_SFX_COUNT)
    {
        return;
    }
    gNdsUiKitSfxRequestCount[cue]++;
    gNdsUiKitSfxLastId = sNdsUiKitSfxIds[cue];
    (void)ndsAudioFgmPlay(sNdsUiKitSfxIds[cue]);
}

/* --- Commit -------------------------------------------------------------- */

void ndsUiKitCommit(void)
{
    OamState *oam;
    u32 visible = 0u;
    u32 id = 0u;
    u32 slot;
    u32 affine_programmed = 0u;

    if (sNdsUiKitActive == FALSE)
    {
        return;
    }
    if (sNdsUiKitDirty == FALSE)
    {
        /* Shadow OAM and the hardware already agree; a menu holding still
         * costs this branch and nothing else. The counter exists so a run
         * whose CommitCount stayed flat is distinguishable from one where the
         * kit was never entered. */
        gNdsUiKitCommitIdleCount++;
        return;
    }
    oam = ndsUiKitOam();

    if (sNdsUiKitTextResident != FALSE)
    {
        for (slot = 0u; slot < NDS_UI_KIT_TEXT_SLOTS; slot++)
        {
            const NdsUiKitTextSlot *state = &sNdsUiKitText[slot];
            u32 base = sNdsUiKitTextVramBase +
                       (slot * NDS_UI_KIT_TEXT_CHUNKS *
                        NDS_UI_KIT_TEXT_CHUNK_BYTES);
            u32 chunk;

            for (chunk = 0u; chunk < NDS_UI_KIT_TEXT_CHUNKS; chunk++, id++)
            {
                s32 show = ((state->visible != 0u) &&
                            (chunk < state->chunks)) ? TRUE : FALSE;

                oamSet(oam, (int)id,
                       (int)state->x +
                           (int)(chunk * NDS_UI_KIT_TEXT_CHUNK_W),
                       (int)state->y, 0, 15,
                       ndsUiKitSpriteSize(NDS_UI_KIT_TEXT_CHUNK_W,
                                          NDS_UI_KIT_TEXT_CHUNK_H),
                       SpriteColorFormat_Bmp,
                       ndsUiKitObjAt(base +
                                     (chunk * NDS_UI_KIT_TEXT_CHUNK_BYTES)),
                       -1, false, (show == FALSE), false, false, false);
                if (show != FALSE)
                {
                    visible++;
                }
            }
        }
    }

    for (slot = 0u; slot < NDS_UI_KIT_SPRITE_SLOTS; slot++, id++)
    {
        const NdsUiKitSpriteSlot *state = &sNdsUiKitSprites[slot];
        const NdsUiKitImageMetric *metric = &kNdsUiKitImageMetrics[state->image];

        if (state->visible == 0u)
        {
            oamSetHidden(oam, (int)id, true);
            continue;
        }
        /* P2-1N (5): a half-resolution sprite renders through affine matrix
         * 0 at 0.5 source-step (= 2x on screen) with double-size coverage;
         * everything else keeps the plain identity path. The matrix is
         * programmed once per commit only when someone needs it. */
        if (state->scale2x != 0u)
        {
            if (affine_programmed == 0u)
            {
                oamRotateScale(oam, 0, 0, 1 << 7, 1 << 7);
                affine_programmed = 1u;
            }
            oamSet(oam, (int)id, (int)state->x, (int)state->y,
                   (int)state->priority,
                   (int)state->alpha,
                   ndsUiKitSpriteSize(metric->cell_w, metric->cell_h),
                   SpriteColorFormat_Bmp,
                   ndsUiKitObjAt(sNdsUiKitImageVram[state->image]),
                   0, true, false, false, false, false);
        }
        else
        {
            oamSet(oam, (int)id, (int)state->x, (int)state->y,
                   (int)state->priority,
                   (int)state->alpha,
                   ndsUiKitSpriteSize(metric->cell_w, metric->cell_h),
                   SpriteColorFormat_Bmp,
                   ndsUiKitObjAt(sNdsUiKitImageVram[state->image]),
                   -1, false, false, false, false, false);
        }
        visible++;
    }

    gNdsUiKitVisibleObjectCount = visible;
    oamUpdate(oam);
    gNdsUiKitCommitCount++;
    sNdsUiKitDirty = FALSE;
}

#endif /* NDS_P2_UI_KIT */
