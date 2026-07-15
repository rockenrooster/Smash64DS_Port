That’s useful signal: the per-tile bookkeeping landed, but it did not change pixels, so it probably was not the visible root cause by itself. I would stop chasing generic tile state now and make the next slice prove the actual sampled texel, not just “this texture cache entry contains some green somewhere.”

Right now RENDER_TEXTURE is still too weak: ndsRendererProfileTextureSample() increments green/non-white if the bound texture entry contains green/non-white texels anywhere. It does not prove that the submitted S/T coordinates sampled a green/non-white texel. That can let the verifier pass while the floor/tree/fighters are actually sampling white areas or wrong palette regions.

Next Codex plan

Have the agent do these in order:

Add a temporary exact-sampled-texel diagnostic.
Run one debug build with texture cache disabled and exact sampling enabled.
Run one texture-only visual probe.
Only then try the two likely addressing probes: N64 tile mask period and secondary tile-6 load isolation.
Leave dark-blue background alone for now; that is likely wallpaper/SObj composition, not this HW triangle texture path.

Do not start perf/cache yet.

1. Add exact sampled texel proof

In src/nds/nds_renderer.c, near the existing debug macro:

#ifndef NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY
#define NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY 0
#endif

#ifndef NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE
#define NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE 0
#endif

#ifndef NDS_RENDERER_HW_DEBUG_EXACT_SAMPLE_COLOR
#define NDS_RENDERER_HW_DEBUG_EXACT_SAMPLE_COLOR 0
#endif

#if NDS_RENDERER_HW_DEBUG_EXACT_SAMPLE_COLOR && !NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE
#error "Exact texture sample diagnostics require NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE=1 so scratch texels match the active texture."
#endif

Add globals to include/nds/nds_startup.h near the existing texture profile globals:

extern volatile u32 gNdsRendererProfileTextureExactSampleCount;
extern volatile u32 gNdsRendererProfileTextureExactSampleGreenCount;
extern volatile u32 gNdsRendererProfileTextureExactSampleNonWhiteCount;
extern volatile u32 gNdsRendererProfileTextureExactSampleWhiteCount;
extern volatile u32 gNdsRendererProfileTextureExactSampleTransparentCount;
extern volatile u32 gNdsRendererProfileTextureExactSourceGreenButSampleWhiteCount;
extern volatile u32 gNdsRendererProfileTextureExactFirstColor;
extern volatile s32 gNdsRendererProfileTextureExactFirstS;
extern volatile s32 gNdsRendererProfileTextureExactFirstT;
extern volatile u32 gNdsRendererProfileTextureExactFirstWidth;
extern volatile u32 gNdsRendererProfileTextureExactFirstHeight;

Add definitions in src/port/diagnostics.c near the current texture globals:

volatile u32 gNdsRendererProfileTextureExactSampleCount;
volatile u32 gNdsRendererProfileTextureExactSampleGreenCount;
volatile u32 gNdsRendererProfileTextureExactSampleNonWhiteCount;
volatile u32 gNdsRendererProfileTextureExactSampleWhiteCount;
volatile u32 gNdsRendererProfileTextureExactSampleTransparentCount;
volatile u32 gNdsRendererProfileTextureExactSourceGreenButSampleWhiteCount;
volatile u32 gNdsRendererProfileTextureExactFirstColor;
volatile s32 gNdsRendererProfileTextureExactFirstS;
volatile s32 gNdsRendererProfileTextureExactFirstT;
volatile u32 gNdsRendererProfileTextureExactFirstWidth;
volatile u32 gNdsRendererProfileTextureExactFirstHeight;

Reset them wherever the current gNdsRendererProfileTexture* counters are reset in src/port/taskman_seam.c:

gNdsRendererProfileTextureExactSampleCount = 0;
gNdsRendererProfileTextureExactSampleGreenCount = 0;
gNdsRendererProfileTextureExactSampleNonWhiteCount = 0;
gNdsRendererProfileTextureExactSampleWhiteCount = 0;
gNdsRendererProfileTextureExactSampleTransparentCount = 0;
gNdsRendererProfileTextureExactSourceGreenButSampleWhiteCount = 0;
gNdsRendererProfileTextureExactFirstColor = 0;
gNdsRendererProfileTextureExactFirstS = 32767;
gNdsRendererProfileTextureExactFirstT = 32767;
gNdsRendererProfileTextureExactFirstWidth = 0;
gNdsRendererProfileTextureExactFirstHeight = 0;

In ndsRendererHardwareBindTexture(), change the cache lookup to this:

#if NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE
    entry = NULL;
#else
    entry = ndsRendererHardwareFindTexture(&key);
#endif

Then replace ndsRendererProfileTextureSample() with this shape. Keep the existing entry-level fallback in the #else.

static void ndsRendererProfileTextureSample(s16 s, s16 t)
{
    const NDSRendererHardwareTextureCacheEntry *entry =
        sNdsRendererHardwareActiveTextureEntry;
    s32 sample_s;
    s32 sample_t;

    if ((entry == NULL) ||
        (entry->profile_width == 0u) ||
        (entry->profile_height == 0u))
    {
        return;
    }

    sample_s = ndsRendererHardwareTextureWrapCoord(
        ((s32)s) >> 4, entry->profile_width,
        (entry->key.render_tile_cms & NDS_RENDERER_TX_CLAMP) == 0u,
        (entry->key.render_tile_cms & NDS_RENDERER_TX_MIRROR) != 0u);
    sample_t = ndsRendererHardwareTextureWrapCoord(
        ((s32)t) >> 4, entry->profile_height,
        (entry->key.render_tile_cmt & NDS_RENDERER_TX_CLAMP) == 0u,
        (entry->key.render_tile_cmt & NDS_RENDERER_TX_MIRROR) != 0u);

    if (((u32)sample_s >= entry->profile_width) ||
        ((u32)sample_t >= entry->profile_height))
    {
        return;
    }

    gNdsRendererProfileTextureSampleCount++;

#if NDS_RENDERER_HW_DEBUG_EXACT_SAMPLE_COLOR
    {
        u32 index = ((u32)sample_t * entry->profile_width) + (u32)sample_s;
        u16 color = sNdsRendererHardwareTextureScratch[index];

        gNdsRendererProfileTextureExactSampleCount++;

        if (gNdsRendererProfileTextureExactFirstColor == 0u)
        {
            gNdsRendererProfileTextureExactFirstColor = color;
            gNdsRendererProfileTextureExactFirstS = sample_s;
            gNdsRendererProfileTextureExactFirstT = sample_t;
            gNdsRendererProfileTextureExactFirstWidth = entry->profile_width;
            gNdsRendererProfileTextureExactFirstHeight = entry->profile_height;
        }

        if ((color & (1u << 15)) == 0u)
        {
            gNdsRendererProfileTextureExactSampleTransparentCount++;
        }
        else if (ndsRendererTextureColorNonWhite(color) != FALSE)
        {
            gNdsRendererProfileTextureExactSampleNonWhiteCount++;

            if (ndsRendererTextureColorDominantGreen(color) != FALSE)
            {
                gNdsRendererProfileTextureExactSampleGreenCount++;
            }
        }
        else
        {
            gNdsRendererProfileTextureExactSampleWhiteCount++;

            if (entry->green_texels != 0u)
            {
                gNdsRendererProfileTextureExactSourceGreenButSampleWhiteCount++;
            }
        }
    }
#else
    if (entry->nonwhite_texels != 0u)
    {
        gNdsRendererProfileTextureSampleNonWhiteCount++;
    }
    if (entry->green_texels != 0u)
    {
        gNdsRendererProfileTextureSampleGreenCount++;
    }
#endif
}

Add a GDB marker in scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1 near RENDER_TEXTURE:

'printf "RENDER_TEXTURE_EXACT=%u,%u,%u,%u,%u,%u,0x%x,%d,%d,%u,%u\n", gNdsRendererProfileTextureExactSampleCount, gNdsRendererProfileTextureExactSampleGreenCount, gNdsRendererProfileTextureExactSampleNonWhiteCount, gNdsRendererProfileTextureExactSampleWhiteCount, gNdsRendererProfileTextureExactSampleTransparentCount, gNdsRendererProfileTextureExactSourceGreenButSampleWhiteCount, gNdsRendererProfileTextureExactFirstColor, gNdsRendererProfileTextureExactFirstS, gNdsRendererProfileTextureExactFirstT, gNdsRendererProfileTextureExactFirstWidth, gNdsRendererProfileTextureExactFirstHeight'

Run this as a scratch build with:

#define NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE 1
#define NDS_RENDERER_HW_DEBUG_EXACT_SAMPLE_COLOR 1

Do not commit those macros enabled.

Interpretation:

Exact green/non-white high, but screen still white:
    color/combine/material/lighting or DS state issue.

Exact source-green-but-sample-white high:
    UV/addressing/tile mask/shift/origin issue.

Exact samples mostly white:
    texture conversion/source rectangle/palette selection is wrong for the active material.

Exact transparent high:
    source crop or CI palette alpha handling is wrong.
2. Actually run the texture-only visual probe

The macro already exists, but it only helps if someone turns it on. Run one scratch build with:

#define NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY 1
#define NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE 1
#define NDS_RENDERER_HW_DEBUG_EXACT_SAMPLE_COLOR 1

Keep this interpretation strict:

Tree/floor/fighters become recognizable:
    next bug is combine/material/lighting modulation.

Visual stays basically unchanged:
    next bug is texture addressing, TMEM/load tile, palette, or source rectangle.

Platforms remain correct but fighters stay rainbow:
    focus on fighter MObj FRAC/SPLIT/secondary texture state, not global stage texture state.
3. Add N64 tile mask-period probe

Current code records masks/maskt, but the actual DS upload/wrap period still mostly uses the tile rectangle or the power-of-two upload size. N64 wrapping can happen at 1 << masks / 1 << maskt. That can make platforms look fine while floor/fence/fighter materials sample the wrong repeat period.

Add this as a probe in src/nds/nds_renderer.c:

#ifndef NDS_RENDERER_HW_DEBUG_APPLY_TILE_MASK_PERIOD
#define NDS_RENDERER_HW_DEBUG_APPLY_TILE_MASK_PERIOD 0
#endif

Add helper near the texture size helpers:

static u32 ndsRendererHardwareTextureMaskedDimension(u32 dimension,
                                                     u32 mask,
                                                     u32 clamp_flags)
{
    u32 period;

    if ((dimension == 0u) ||
        ((clamp_flags & NDS_RENDERER_TX_CLAMP) != 0u) ||
        (mask == 0u) ||
        (mask >= 12u))
    {
        return dimension;
    }

    period = 1u << mask;

    /*
     * Probe only: when N64 wraps at a smaller mask period than the tile
     * rectangle, upload one period so DS hardware wraps at the same interval.
     */
    if ((period != 0u) && (period < dimension))
    {
        return period;
    }

    return dimension;
}

In ndsRendererHardwareBindTexture(), after width and height have been chosen and before upload_width/upload_height are computed:

#if NDS_RENDERER_HW_DEBUG_APPLY_TILE_MASK_PERIOD
    width = ndsRendererHardwareTextureMaskedDimension(
        width,
        stats->texture_render_tile_masks,
        stats->texture_render_tile_cms);
    height = ndsRendererHardwareTextureMaskedDimension(
        height,
        stats->texture_render_tile_maskt,
        stats->texture_render_tile_cmt);
#endif

Run with:

#define NDS_RENDERER_HW_DEBUG_APPLY_TILE_MASK_PERIOD 1
#define NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY 1
#define NDS_RENDERER_HW_DEBUG_DISABLE_TEXTURE_CACHE 1
#define NDS_RENDERER_HW_DEBUG_EXACT_SAMPLE_COLOR 1

Expected outcomes:

Floor/fence placement changes:
    keep investigating tile mask/shift/addressing. This is a real root.

Fighters become less rainbow:
    fighter textures are also hitting the wrong wrap/mask period.

No visual or exact-sample change:
    mask period is not the current visible blocker.

Do not land the probe as-is unless it improves pixels. If it improves pixels, convert it into a proper N64 tile addressing helper instead of leaving it as a debug macro.

4. Add secondary tile-6 load isolation

Fighter rainbow is suspicious for MOBJ_FLAG_FRAC / MOBJ_FLAG_SPLIT / secondary texture state. The adapter emits secondary loads to tile 6, but the DS path has only one active texture. A tile-6 load must not masquerade as the active render-tile source for tile 0.

Add a probe counter and optional ignore.

Globals in include/nds/nds_startup.h:

extern volatile u32 gNdsRendererProfileTextureSecondaryLoadCount;
extern volatile u32 gNdsRendererProfileTextureIgnoredSecondaryLoadCount;
extern volatile u32 gNdsRendererProfileTextureSecondaryLoadTileMask;

Definitions in src/port/diagnostics.c:

volatile u32 gNdsRendererProfileTextureSecondaryLoadCount;
volatile u32 gNdsRendererProfileTextureIgnoredSecondaryLoadCount;
volatile u32 gNdsRendererProfileTextureSecondaryLoadTileMask;

Reset with the other texture counters:

gNdsRendererProfileTextureSecondaryLoadCount = 0;
gNdsRendererProfileTextureIgnoredSecondaryLoadCount = 0;
gNdsRendererProfileTextureSecondaryLoadTileMask = 0;

In src/nds/nds_renderer.c:

#ifndef NDS_RENDERER_HW_DEBUG_IGNORE_SECONDARY_TEXTURE_LOADS
#define NDS_RENDERER_HW_DEBUG_IGNORE_SECONDARY_TEXTURE_LOADS 0
#endif

static s32 ndsRendererHardwareTextureLoadTileIsPrimary(u32 tile)
{
    /*
     * Normal one-texture paths load through the load tile or directly through
     * the render tile. Tile 6 is used by BattleShip MObj FRAC/SPLIT secondary
     * texture state and must not overwrite the single active DS texture load.
     */
    return ((tile == NDS_RENDERER_LOAD_TILE) ||
            (tile == NDS_RENDERER_RENDER_TILE)) ? TRUE : FALSE;
}

Modify ndsRendererRecordLoadBlock():

static void ndsRendererRecordLoadBlock(NDSRendererStats *stats,
                                       u32 w0, u32 w1)
{
    u32 tile;

    if (stats == NULL)
    {
        return;
    }

    tile = (w1 >> 24) & 0x7u;

    if (ndsRendererHardwareTextureLoadTileIsPrimary(tile) == FALSE)
    {
        gNdsRendererProfileTextureSecondaryLoadCount++;
        gNdsRendererProfileTextureSecondaryLoadTileMask |= 1u << tile;

#if NDS_RENDERER_HW_DEBUG_IGNORE_SECONDARY_TEXTURE_LOADS
        gNdsRendererProfileTextureIgnoredSecondaryLoadCount++;
        return;
#endif
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_kind = NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_tile = tile;
    stats->texture_load_block_uls = (w0 >> 12) & 0x0FFFu;
    stats->texture_load_block_ult = w0 & 0x0FFFu;
    stats->texture_load_block_lrs = (w1 >> 12) & 0x0FFFu;
    stats->texture_load_block_dxt = w1 & 0x0FFFu;
    stats->texture_load_texels = stats->texture_load_block_lrs + 1u;
}

Modify ndsRendererRecordLoadTile() similarly:

tile = (w1 >> 24) & 0x7u;

if (ndsRendererHardwareTextureLoadTileIsPrimary(tile) == FALSE)
{
    gNdsRendererProfileTextureSecondaryLoadCount++;
    gNdsRendererProfileTextureSecondaryLoadTileMask |= 1u << tile;

#if NDS_RENDERER_HW_DEBUG_IGNORE_SECONDARY_TEXTURE_LOADS
    gNdsRendererProfileTextureIgnoredSecondaryLoadCount++;
    return;
#endif
}

stats->texture_load_tile = tile;

Add a marker beside RENDER_TEXTURE_EXACT:

'printf "RENDER_TEXTURE_SECONDARY=%u,%u,%#x\n", gNdsRendererProfileTextureSecondaryLoadCount, gNdsRendererProfileTextureIgnoredSecondaryLoadCount, gNdsRendererProfileTextureSecondaryLoadTileMask'

Run first with ignore off:

#define NDS_RENDERER_HW_DEBUG_IGNORE_SECONDARY_TEXTURE_LOADS 0

Then run with ignore on:

#define NDS_RENDERER_HW_DEBUG_IGNORE_SECONDARY_TEXTURE_LOADS 1

Interpretation:

Secondary load count is high and ignore-on changes fighter rainbow:
    tile-6 FRAC/SPLIT is corrupting the one-texture DS bind view.
    Implement real secondary texture/TMEM handling later, or deliberately skip
    secondary blend state until it is emulated.

Secondary count is high but ignore-on has no visual change:
    tile-6 exists but is not the current blocker.

Secondary count is zero:
    fighter rainbow is not from secondary loads in the sampled frame.
5. Add one stage/fighter bucket probe

The current image mixes stage defects and fighter defects. Have the agent add temporary top-level filters around the HW submit calls, not deep inside the renderer.

Add debug macros somewhere central:

#ifndef NDS_RENDERER_HW_DEBUG_STAGE_ONLY
#define NDS_RENDERER_HW_DEBUG_STAGE_ONLY 0
#endif

#ifndef NDS_RENDERER_HW_DEBUG_FIGHTERS_ONLY
#define NDS_RENDERER_HW_DEBUG_FIGHTERS_ONLY 0
#endif

Then at the stage HW submission site:

#if NDS_RENDERER_HW_DEBUG_FIGHTERS_ONLY
    return;
#endif

And at the fighter HW submission site:

#if NDS_RENDERER_HW_DEBUG_STAGE_ONLY
    return;
#endif

Use this only for screenshots:

Stage-only still has white floor/tree/bushes:
    stage material/TMEM/addressing is independently broken.

Fighters-only still rainbow/broken:
    fighter material or DObj matrix assembly is independently broken.

Fighters-only looks okay but combined scene breaks:
    depth/order/raw-matrix/projected-fallback interaction is covering or slicing them.
6. Keep background out of this slice

The dark-blue background should stay a separate task. Dream Land’s background is likely the wallpaper/SObj/2D composition path, not the stage/fighter HW triangle texture path. Don’t let it block the floor/tree/fighter texture fixes.

A later marker is enough:

volatile u32 gNdsWallpaperSObjSeenCount;
volatile u32 gNdsWallpaperSObjDrawCount;
volatile u32 gNdsWallpaperTextureSeenCount;
Focused commands

Use the same focused loop, not a full sweep:

.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-battle-playable-realtime-harness.ps1 -DelaySeconds 4 -SkipScreenshot

.\scripts\capture-melonds.ps1 `
  -Rom .\smash64ds-battle-playable-canonical-hwtri.nds `
  -Output artifacts\visibility\canonical-hwtri-debug-a.png `
  -SecondOutput artifacts\visibility\canonical-hwtri-debug-b.png `
  -DelaySeconds 30 `
  -SecondDelayMilliseconds 100

.\scripts\assert-melonds-top-visible.ps1 `
  -Image artifacts\visibility\canonical-hwtri-debug-a.png `
  -CompareImage artifacts\visibility\canonical-hwtri-debug-b.png `
  -MinDominantGreenFraction 0.03 `
  -MaxCompareChangedFraction 0.25 `
  -MinDifferentFraction 0.01
Recommended next acceptance

Do not accept another “same screenshot, better counters” step. The next landed renderer fix should visibly change at least one of these:

floor green/white repeat pattern changes toward N64 reference
tree/bush/flower white area gains real texture color
fighter rainbow reduces or parts become coherent
fence moves closer to correct stage position

The immediate goal is not 60fps. The immediate goal is proving whether the remaining visual error is actual sampled texel, tile/TMEM/secondary-load state, or combine/material modulation.