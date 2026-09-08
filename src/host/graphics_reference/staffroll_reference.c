/* Preserved paused-campaign software presentation, host-only. */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Software scene composition is host-only
#endif

/* P2-6 step 8 tail. SCStaffroll name/job glyph seam.
 *
 * Source (decomp sc/sccommon/scstaffroll.c) draws every staff name and job
 * glyph from raw 4-bit intensity Image blocks, not Sprite records:
 * scStaffrollInitNameAndJobDisplayLists (:2053-2102) builds one Gfx DL per
 * glyph with gDPLoadTextureBlock_4b from the Image offset (:2085, G_IM_FMT_I,
 * width padded to 16), one textured quad via gSPVertex + gSP2Triangles,
 * attached as DObj children (:1570, :1758) and drawn by
 * scStaffrollJobProcDisplay (:1499) / scStaffrollNameProcDisplay (:1513),
 * which set a PRIMITIVE-tinted XLU combine before gcDrawDObjTreeForGObj.
 * The port SObj path only accepts Sprite records
 * (lbCommonMakeSObjForGObj above, preview shape tests, wallpaper/decode
 * caches), and gcDrawDObjTreeForGObj is a draw recorder on DS, so these
 * quads have no DS expression today.
 *
 * Seam: decode each glyph block ONCE on first draw into a per-scene cache
 * keyed by Image pointer (the source decodes per DL at init, so a per-scene
 * cache matches its cost model), then blit DS halfword pixels into the
 * original-sprite staging preview exactly where the source quad lands,
 * tinted by the display proc's PRIMITIVE colour. No allocation happens on the
 * per-frame path: the pool bump-allocates only while filling, the DObj walk
 * lives in the import TU and is stack-only, and the frame coalescing below
 * reuses one staging Begin per frame. VRAM cost is zero (main-RAM staging,
 * like every other preview arm).
 *
 * Geometry preserved: source verts span [-width,+width] x [-height,+height]
 * around the DObj origin (:2065-2066) with the width x height texels stretched
 * across (:2071-2072), and sibling advance is 2*width (:1643-:1646), so the
 * blit plots a 2w x 2h rect with exact 2x nearest sampling. Tint preserved:
 * PRIM * intensity / 255 per channel (env contributes nothing, same as an
 * SObj with envcolor zero through ndsSpriteLerpPrimEnv). XLU preserved as the
 * preview compositor allows: intensity 0 texels are skipped (transparent),
 * all others land opaque -- the same coverage rule as the other I4 arms.
 * Text, order, kerning, scroll timing, and the recorder call are untouched;
 * those still run in the source procs.
 *
 * I4 byte layout mirrors the generic I4 arm of ndsDrawSObjIntoPreview: one
 * byte per texel pair, `^ 3` word-order swizzle from the O2R loader, high
 * nibble on even columns, rows of padded-width/2 bytes. The glyph loads use
 * NOMIRROR/CLAMP with no TEXSHUF, so no odd-row xor. */

#define NDS_STAFFROLL_GLYPH_SLOTS 64u
/* Sum of width*height over the 56 staged NameAndJob rows is 17,593 bytes
 * (uppers ~9,702 + lowers ~7,314 + punctuation/4 ~577, from
 * dSCStaffrollNameAndJobSpriteInfo); 17,920 leaves 327 bytes of margin. The
 * raw blocks themselves stay resident in the reloc file (offsets
 * 0x0008..0x3258, span 0x3250 = 12,880 bytes); this pool holds the decoded
 * one-byte-per-texel intensities only. */
#define NDS_STAFFROLL_GLYPH_POOL_BYTES 17920u

typedef struct NDSStaffrollGlyphEntry
{
    u32 valid;
    const void *image;
    u32 probe;
    u8 width;
    u8 height;
    u32 pool_offset;
} NDSStaffrollGlyphEntry;

static NDSStaffrollGlyphEntry sNdsStaffrollGlyphs[NDS_STAFFROLL_GLYPH_SLOTS];
/* The pool is carved from the scene's general heap on first use, not from
 * .bss: a static 17,920-byte buffer would be resident in every scene of the
 * game for a screen that runs once per campaign, and RAM is the binding P2
 * constraint. The arena resets with the scene (ndsSceneManagerEnter), so the
 * pointer is dropped by ndsStaffrollGlyphCacheInvalidate, which the import TU
 * calls on a file change, and never freed by hand. */
static u8 *sNdsStaffrollGlyphPool = NULL;
static u32 sNdsStaffrollGlyphPoolUsed;
static u32 sNdsStaffrollPreviewFrame = 0xffffffffu;
static u16 *sNdsStaffrollPreview = NULL;
static u32 sNdsStaffrollPreviewPitch = 0u;

volatile u32 gNdsStaffrollGlyphCacheBuildCount;
volatile u32 gNdsStaffrollGlyphDrawPixelCount;

void ndsStaffrollGlyphCacheInvalidate(void)
{
    u32 i;

    for (i = 0u; i < NDS_STAFFROLL_GLYPH_SLOTS; i++)
    {
        sNdsStaffrollGlyphs[i].valid = 0u;
    }
    sNdsStaffrollGlyphPoolUsed = 0u;
    sNdsStaffrollGlyphPool = NULL; /* arena memory; the scene reset reclaims it */
    sNdsStaffrollPreviewFrame = 0xffffffffu;
    sNdsStaffrollPreview = NULL;
    sNdsStaffrollPreviewPitch = 0u;
}

/* Decode-once keyed by Image pointer. A hit needs pointer equality, matching
 * dimensions, and a 4-byte content probe (a same-address reload with different
 * bytes must remiss). The image pointer is only dereferenced after the caller
 * derived it from the resident file, and the reloc range proof runs before
 * any byte is read, mirroring ndsSObjBuildWallpaperDecodeCache. */
s32 ndsStaffrollGlyphEnsure(const void *image, u32 width, u32 height,
                            u32 *out_slot)
{
    u32 stride;
    u32 row_bytes;
    u32 need;
    u32 i;
    u32 x;
    u32 y;
    const u8 *src;
    u8 *dst;
    u32 probe;
    NDSRelocLoadedFile *loaded;

    if ((image == NULL) || (out_slot == NULL) ||
        (width == 0u) || (height == 0u) ||
        (width > 32u) || (height > 32u))
    {
        return FALSE;
    }
    memcpy(&probe, image, sizeof(probe));
    for (i = 0u; i < NDS_STAFFROLL_GLYPH_SLOTS; i++)
    {
        NDSStaffrollGlyphEntry *entry = &sNdsStaffrollGlyphs[i];

        if ((entry->valid != 0u) && (entry->image == image) &&
            (entry->probe == probe) &&
            ((u32)entry->width == width) &&
            ((u32)entry->height == height))
        {
            *out_slot = i;
            return TRUE;
        }
    }
    stride = ((width + 15u) / 16u) * 16u;
    row_bytes = stride / 2u;
    need = width * height;
    loaded = ndsRelocFindLoadedFileContaining(
        image, (size_t)row_bytes * (size_t)height);
    if (ndsRelocPointerRangeInLoadedFile(
            loaded, image,
            (size_t)row_bytes * (size_t)height) == FALSE)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_STAFFROLL_GLYPH_SLOTS; i++)
    {
        if (sNdsStaffrollGlyphs[i].valid == 0u)
        {
            break;
        }
    }
    if ((i >= NDS_STAFFROLL_GLYPH_SLOTS) ||
        ((sNdsStaffrollGlyphPoolUsed + need) >
         NDS_STAFFROLL_GLYPH_POOL_BYTES))
    {
        return FALSE;
    }
    if (sNdsStaffrollGlyphPool == NULL)
    {
        /* First glyph of the scene: one arena carve, reclaimed by the scene
         * reset. A NULL here means the credits arena is already exhausted,
         * and the glyph is refused rather than written through NULL. */
        sNdsStaffrollGlyphPool =
            (u8 *)syTaskmanMalloc(NDS_STAFFROLL_GLYPH_POOL_BYTES, 16u);
        if (sNdsStaffrollGlyphPool == NULL)
        {
            return FALSE;
        }
        sNdsStaffrollGlyphPoolUsed = 0u;
    }
    src = (const u8 *)image;
    dst = &sNdsStaffrollGlyphPool[sNdsStaffrollGlyphPoolUsed];
    for (y = 0u; y < height; y++)
    {
        for (x = 0u; x < width; x++)
        {
            size_t byte_index = ((size_t)y * (size_t)row_bytes) + (x >> 1);
            u8 packed = src[byte_index ^ 3u];
            u8 nibble = (((x & 1u) == 0u) != FALSE) ?
                (u8)(packed >> 4) : (u8)(packed & 0x0fu);

            dst[((size_t)y * (size_t)width) + x] = (u8)(nibble * 17u);
        }
    }
    sNdsStaffrollGlyphs[i].valid = TRUE;
    sNdsStaffrollGlyphs[i].image = image;
    sNdsStaffrollGlyphs[i].probe = probe;
    sNdsStaffrollGlyphs[i].width = (u8)width;
    sNdsStaffrollGlyphs[i].height = (u8)height;
    sNdsStaffrollGlyphs[i].pool_offset = sNdsStaffrollGlyphPoolUsed;
    sNdsStaffrollGlyphPoolUsed += need;
    gNdsStaffrollGlyphCacheBuildCount++;
    *out_slot = i;
    return TRUE;
}

/* One glyph quad at DObj origin (org_x, org_y): rect
 * [org_x-w, org_x+w) x [org_y-h, org_y+h), 2x nearest sampling, halfword
 * stores with preview clipping. Intensity 0 skips (XLU); otherwise
 * PRIM*intensity/255 via NDS_SPRITE_DIV255 (bound 255*255+127 = 65,152 holds,
 * same proof as ndsSpriteLerpPrimEnv) packed by ndsSpritePackRgb15. */
void ndsStaffrollGlyphBlit(u32 slot, s32 org_x, s32 org_y,
                           u8 prim_r, u8 prim_g, u8 prim_b,
                           u16 *preview, u32 preview_pitch,
                           u32 preview_width, u32 preview_height)
{
    u32 width;
    u32 height;
    const u8 *texels;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 dst_x;
    s32 dst_y;

    if ((slot >= NDS_STAFFROLL_GLYPH_SLOTS) ||
        (sNdsStaffrollGlyphs[slot].valid == 0u) ||
        (preview == NULL) || (preview_pitch == 0u))
    {
        return;
    }
    width = (u32)sNdsStaffrollGlyphs[slot].width;
    height = (u32)sNdsStaffrollGlyphs[slot].height;
    texels = &sNdsStaffrollGlyphPool[sNdsStaffrollGlyphs[slot].pool_offset];
    x0 = org_x - (s32)width;
    x1 = org_x + (s32)width;
    y0 = org_y - (s32)height;
    y1 = org_y + (s32)height;
    for (dst_y = y0; dst_y < y1; dst_y++)
    {
        u32 src_y;

        if ((dst_y < 0) || (dst_y >= (s32)preview_height))
        {
            continue;
        }
        src_y = (u32)(dst_y - y0) >> 1;
        if (src_y >= height)
        {
            src_y = height - 1u;
        }
        for (dst_x = x0; dst_x < x1; dst_x++)
        {
            u32 src_x;
            u8 intensity;
            u8 red;
            u8 green;
            u8 blue;

            if ((dst_x < 0) || (dst_x >= (s32)preview_width))
            {
                continue;
            }
            src_x = (u32)(dst_x - x0) >> 1;
            if (src_x >= width)
            {
                src_x = width - 1u;
            }
            intensity = texels[(src_y * width) + src_x];
            if (intensity == 0u)
            {
                continue;
            }
            red = (u8)NDS_SPRITE_DIV255(
                ((u32)prim_r * (u32)intensity) + 127u);
            green = (u8)NDS_SPRITE_DIV255(
                ((u32)prim_g * (u32)intensity) + 127u);
            blue = (u8)NDS_SPRITE_DIV255(
                ((u32)prim_b * (u32)intensity) + 127u);
            preview[((u32)dst_y * preview_pitch) + (u32)dst_x] =
                ndsSpritePackRgb15(red, green, blue);
            gNdsStaffrollGlyphDrawPixelCount++;
        }
    }
}

/* One staging Begin per frame shared by the job and name display procs (Begin
 * clears, so each proc beginning separately would wipe the other's glyphs).
 * Every call blits; every call commits; only the first call per
 * gNdsFrameCounter begins. Relies on the counter ticking per displayed frame;
 * a stalled counter would smear scrolling glyphs instead of clearing. */
s32 ndsStaffrollFrameBegin(u16 **out_preview, u32 *out_pitch)
{
    if ((out_preview == NULL) || (out_pitch == NULL))
    {
        return FALSE;
    }
    /* One Begin per frame across both menu paths: the scratch is shared, so
     * a second Begin would clear whatever the first path already drew
     * (brackets before glyphs or vice versa). Adopt the open frame staging
     * instead; EndFrame commits it once. */
    if ((sNdsSObjFrameActive != FALSE) &&
        (sNdsSObjFramePreview != NULL) &&
        (sNdsSObjFramePreviewPitch != 0u))
    {
        sNdsStaffrollPreview = sNdsSObjFramePreview;
        sNdsStaffrollPreviewPitch = sNdsSObjFramePreviewPitch;
        sNdsStaffrollPreviewFrame = gNdsFrameCounter;
        *out_preview = sNdsStaffrollPreview;
        *out_pitch = sNdsStaffrollPreviewPitch;
        return TRUE;
    }
    if ((sNdsStaffrollPreviewFrame != gNdsFrameCounter) ||
        (sNdsStaffrollPreview == NULL))
    {
        sNdsStaffrollPreview = ndsPlatformBeginOriginalSpritePreview(
            320u, 240u, 0, 0, &sNdsStaffrollPreviewPitch);
        if ((sNdsStaffrollPreview == NULL) ||
            (sNdsStaffrollPreviewPitch == 0u))
        {
            sNdsStaffrollPreview = NULL;
            sNdsStaffrollPreviewPitch = 0u;
            return FALSE;
        }
        sNdsStaffrollPreviewFrame = gNdsFrameCounter;
    }
    *out_preview = sNdsStaffrollPreview;
    *out_pitch = sNdsStaffrollPreviewPitch;
    return TRUE;
}

void ndsStaffrollFrameCommit(void)
{
    if (sNdsStaffrollPreview != NULL)
    {
        ndsPlatformCommitOriginalSpritePreview();
    }
}
