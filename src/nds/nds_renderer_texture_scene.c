/* Native renderer, scene textures: texture VRAM as a scene resource and its mip
 * cache, the IFCommon atlases and cloud quads, entry and halo texels, fighter
 * tint tiles, the VRAM census, Hyrule's textures, texture parameters, stage
 * texture sites and the battle static textures. A slice of nds_renderer.c,
 * after nds_renderer_textures_effects.c. */

/* Texture VRAM is a SCENE-owned resource, and this is the one call that says so.
 *
 * R2-07 E3/E4 root-caused the second-entry stage corruption here: libnds
 * allocates texture VRAM inside `glTexImage2D`, nothing in the port owned the
 * lifetime of that allocator across a scene boundary, and a second entry into
 * `nSCKindVSBattle` (Sudden Death, or a START rematch) therefore rebuilt its 24
 * pinned statics into a pool whose occupancy and free-list shape were inherited
 * from the match that had just ended. Measured consequence: `glTexImage2D`
 * refused a 4,096-byte upload with 268,800 contiguous free bytes reported, which
 * failed `PrepareRun` for run 42, rejected the whole native stage owner, and
 * dropped the stage onto the generic renderer every frame -- correct geometry,
 * untextured pond, `STG` 2.76M, 4.2 FPS.
 *
 * Two lesser owners had the same shape and are also settled by resetting rather
 * than by hand: `ndsIFCommonNativeOamDiscardTextures` zeroes the four atlas
 * NAMES without deleting them (so their blocks would leak one entry at a time),
 * and the earlier fix that released the atlases to restore entry one's
 * allocation ORDER only reproduced a layout instead of guaranteeing one.
 *
 * So every entry into the battle scene now starts from an empty allocator, which
 * makes entry N byte-for-byte allocation-equivalent to entry 1 by construction
 * instead of by argument. The caller must release its own software owners first
 * (`ndsIFCommonNativeOamReleaseCloudTextures`) -- `glResetTextures` invalidates
 * every name, so anything still holding one has to have let go.
 *
 * Cost is scene-entry only: one cache teardown plus the re-upload the static and
 * atlas prepares were already doing. It buys the standing law this campaign paid
 * for twice: a resource that survives a scene boundary must be re-derived from
 * something the boundary actually moves, never from pointers, names, sizes, or
 * an allocation order a rewound allocator happens to reproduce. */
volatile u32 gNdsRendererSceneTextureVramResetEnable = 1u;
volatile u32 gNdsRendererSceneTextureVramResetCount;

static void ndsFtrAdmitSceneResetGuard(void);

NDS_RENDERER_COLD void ndsRendererHardwareResetSceneTextureVram(void)
{
    /* The reset frees every texel a held frame's 3D still draws with (the
     * transition hold, nds_platform.c): such a frame ends here. */
    ndsPlatformTransitionThawIf3DShown();
    /* Runtime, not a build flag: the control arm has to be the same binary.
     * This ROM's pacing is cache-placement sensitive and separately linked arms
     * have already confused two comparisons on this row. */
    if (gNdsRendererSceneTextureVramResetEnable == 0u)
    {
        return;
    }
    /* P2-2p8 Phase 1 slice 2b: a previous battle's texture regions still
     * active here means its exit hook was missed; close them first. */
    ndsFtrAdmitSceneResetGuard();
    /* DiscardTextureCache ends the batch, deletes every cache name and the
     * no-texture name, and clears the lookup, refresh queue and active
     * binding. glResetTextures then drops libnds's own texture and palette
     * metadata and rebuilds both block allocators. */
    ndsRendererHardwareDiscardTextureCache();
    /* P8 V5: the wallpaper tiles' block goes with the allocators. */
    ndsNativeWallpaperTilesForget();
    glResetTextures();
#if NDS_VRAM_CENSUS_LIVE
    ndsVramCensusTagReset();   /* slice 2a lab tags: every name is gone */
#endif
    /* Software state that would otherwise reference names glResetTextures has
     * just invalidated. The prepared-run cache only exists at
     * NDS_R2_STAGE_DIRECT; this function is scoped to NDS_RENDERER_HW_TRIANGLES,
     * which is the weaker condition, and the audio-FGM harness is a config that
     * has the second without the first. */
    sNdsNativeStageOwnerExecution.r2_prepared_valid = 0u;
    gNdsRendererSceneTextureVramResetCount++;
}

s32 ndsRendererHardwareUploadSceneMipCache(const u16 *mip0,
                                            const u16 *mip1,
                                            const u16 *mip2)
{
    (void)mip0;
    (void)mip1;
    (void)mip2;
    return FALSE;
}

s32 ndsRendererHardwareDrawSceneMipCache(u32 mip_index,
                                          const s32 *tex_s_q4,
                                          const s32 *tex_t_q4,
                                          u32 columns,
                                          u32 rows)
{
    (void)mip_index;
    (void)tex_s_q4;
    (void)tex_t_q4;
    (void)columns;
    (void)rows;
    return FALSE;
}

static s32 ndsRendererHardwareTextureSizeEnum(u32 size, int *out)
{
    int value;

    if (out == NULL)
    {
        return FALSE;
    }
    switch (size)
    {
    case 8u: value = TEXTURE_SIZE_8; break;
    case 16u: value = TEXTURE_SIZE_16; break;
    case 32u: value = TEXTURE_SIZE_32; break;
    case 64u: value = TEXTURE_SIZE_64; break;
    case 128u: value = TEXTURE_SIZE_128; break;
    case 256u: value = TEXTURE_SIZE_256; break;
    default:
        return FALSE;
    }
    *out = value;
    return TRUE;
}

static s32 ndsRendererHardwarePrepareIFCommonAtlas(
    u32 width, u32 height, u32 texture_format,
    const u16 *palette, u32 palette_entries,
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name,
    u32 color0_transparent)
{
    int size_x;
    int size_y;
    int name = 0;
    u32 bytes;
    u32 param;
    u32 upload_attempts = 0u;
    u8 *pixels = (u8 *)sNdsRendererHardwareTextureScratch;

    if ((fill == NULL) || (texture_name == NULL) ||
        (((texture_format == GL_RGB8_A5) &&
          ((palette == NULL) || (palette_entries == 0u) || (palette_entries > 8u))) ||
         ((texture_format == GL_RGB32_A3) && (palette_entries > 32u)) ||
         ((texture_format == GL_RGB16) && (palette_entries > 16u)) ||
         ((texture_format == GL_RGB256) && (palette_entries > 256u)) ||
         (((texture_format == GL_RGB32_A3) || (texture_format == GL_RGB16) ||
           (texture_format == GL_RGB256)) &&
          ((palette == NULL) || (palette_entries == 0u))) ||
         ((texture_format == GL_RGBA) &&
          ((palette != NULL) || (palette_entries != 0u))) ||
         ((texture_format != GL_RGB8_A5) &&
          (texture_format != GL_RGB32_A3) &&
          (texture_format != GL_RGB16) &&
          (texture_format != GL_RGB256) &&
          (texture_format != GL_RGBA))) ||
        (height == 0u) || (width > (UINT32_MAX / height)) ||
        (ndsRendererHardwareTextureSizeEnum(width, &size_x) == FALSE) ||
        (ndsRendererHardwareTextureSizeEnum(height, &size_y) == FALSE))
    {
        return FALSE;
    }
    /* PAL16 packs two texels per byte; PAL256 and the alpha/index formats
     * use one. Only PAL16/PAL256 use colour-zero transparency; A3I5/A5I3
     * retain their per-texel alpha and may use palette index zero. */
    if (texture_format == GL_RGB16)
    {
        bytes = (width * height) / 2u;
        param = (u32)TEXGEN_TEXCOORD;
        if (color0_transparent != FALSE)
        {
            param |= (u32)GL_TEXTURE_COLOR0_TRANSPARENT;
        }
    }
    else if (texture_format == GL_RGBA)
    {
        bytes = width * height * 2u;
        param = (u32)TEXGEN_TEXCOORD;
    }
    else
    {
        bytes = width * height;
        param = (u32)TEXGEN_TEXCOORD;
    }
    NDS_RENDERER_CAPACITY_HIGH_WATER(
        gNdsRendererTextureScratchFillBytesHighWater, bytes);
    if ((bytes > sizeof(sNdsRendererHardwareTextureScratch)) ||
        (fill(pixels, bytes, user_data) == FALSE))
    {
        return FALSE;
    }
    if ((texture_format == GL_RGB256) && (color0_transparent != FALSE))
    {
        param |= (u32)GL_TEXTURE_COLOR0_TRANSPARENT;
    }
    if (*texture_name != 0u)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(texture_name);
    }
    ndsRendererHardwareEndBatch();
    if (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0)
    {
        return FALSE;
    }
    ndsRendererHardwareBindTextureState(name);
    while (ndsRendererHardwareFencedGlTexImage2D(
               GL_TEXTURE_2D, 0, (int)texture_format, size_x, size_y, 0,
               (int)param, pixels) == 0)
    {
        ndsRendererHardwareFencedGlDeleteTextures(1, &name);
        name = 0;
        upload_attempts++;
        if ((upload_attempts >= NDS_RENDERER_HW_TEXTURE_CACHE_COUNT) ||
            (ndsRendererHardwareEvictTexture(NULL) == FALSE) ||
            (ndsRendererHardwareFencedGlGenTextures(1, &name) == 0))
        {
            /* A REFUSED UPLOAD MUST NOT LEAVE THE TRACKER LYING. We bound a
             * name above with ndsRendererHardwareBindTextureState, which is a
             * raw glBindTexture and does not touch
             * sNdsRendererHardwareBoundTextureName -- and we have just deleted
             * that name. So on this path the hardware is bound to a dead
             * texture while the tracker still claims the caller's previous
             * name, and ndsRendererHardwareBindTextureName ELIDES a rebind
             * when the requested name equals the tracker. The next consumer to
             * ask for that previous name would then draw through the deleted
             * one. Zero both so the next bind is always issued. */
            sNdsRendererHardwareBoundTextureName = 0u;
            sNdsRendererHardwareActiveTextureEntry = NULL;
            return FALSE;
        }
        ndsRendererHardwareBindTextureState(name);
    }
    if (texture_format != GL_RGBA)
    {
        glColorTableEXT(GL_TEXTURE_2D, 0, (int)palette_entries, 0, 0, palette);
    }
    *texture_name = (u32)name;
    sNdsRendererHardwareBoundTextureName = (u32)name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareIFCommonCloudAtlas(
    u32 width, u32 height, const u16 palette[8],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
#if NDS_VRAM_CENSUS_LIVE
    gNdsVramCensusAtlasUser =
        (u32)(uintptr_t)__builtin_return_address(0) & ~1u;
#endif
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB8_A5, palette, 8u,
        fill, user_data, texture_name, FALSE);
}

s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(
    u32 width, u32 height, const u16 palette[32],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
#if NDS_VRAM_CENSUS_LIVE
    gNdsVramCensusAtlasUser =
        (u32)(uintptr_t)__builtin_return_address(0) & ~1u;
#endif
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB32_A3, palette, 32u,
        fill, user_data, texture_name, FALSE);
}

/* Source CI4 with an RGBA5551 TLUT whose entry 0 has alpha 0 -- Mario's
 * fireball is exactly this -- maps onto GL_RGB16 with NO loss: 16 palette
 * entries, 4bpp, and index 0 clear. The two formats above both had to trade
 * index bits against alpha bits; this one does not, so it is the format to
 * reach for whenever the source asset is CI4 and its palette carries the
 * transparency. `fill` writes PACKED nibbles here, two texels per byte, low
 * nibble first -- not one byte per texel as the A5I3/A3I5 fills do. */
NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareIFCommonPal16Atlas(
    u32 width, u32 height, const u16 palette[16],
    NDSRendererTextureFillCallback fill, void *user_data, u32 *texture_name)
{
#if NDS_VRAM_CENSUS_LIVE
    gNdsVramCensusAtlasUser =
        (u32)(uintptr_t)__builtin_return_address(0) & ~1u;
#endif
    return ndsRendererHardwarePrepareIFCommonAtlas(
        width, height, GL_RGB16, palette, 16u,
        fill, user_data, texture_name, TRUE);
}

NDS_RENDERER_COLD static s32 ndsRendererImpactWaveTexelFill(u8 *pixels, u32 bytes,
                                          void *user_data)
{
    (void)user_data;
    if ((pixels == NULL) || (bytes < NDS_RENDERER_IMPACT_WAVE_TEX_BYTES))
    {
        return FALSE;
    }
    memcpy(pixels, sNdsRendererImpactWaveTexels,
           NDS_RENDERER_IMPACT_WAVE_TEX_BYTES);
    return TRUE;
}

typedef struct NDSRebirthHaloTextureFill
{
    const u8 *pixels;
    u32 bytes;
} NDSRebirthHaloTextureFill;

NDS_RENDERER_COLD static s32 ndsRendererRebirthHaloTextureFill(u8 *pixels, u32 bytes,
                                              void *user_data)
{
    const NDSRebirthHaloTextureFill *fill =
        (const NDSRebirthHaloTextureFill *)user_data;

    if ((pixels == NULL) || (fill == NULL) || (fill->pixels == NULL) ||
        (bytes < fill->bytes))
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    memcpy(pixels, fill->pixels, fill->bytes);
    return TRUE;
}

/* The P4 article textures' texels are read only here, at preparation, so
 * they live in NitroFS rather than as .main data every scene's heap would pay
 * for (32 KB for the eighteen contents). One handle per preparation pass;
 * ndsRendererEntryTexelsClose ends it. */
static FILE *sNdsEntryTexelsFile;

static s32 ndsRendererEntryTexelsRead(u32 offset, u8 *out, u32 bytes)
{
    if (sNdsEntryTexelsFile == NULL)
    {
        sNdsEntryTexelsFile = ndsRendererHardwareFencedTextureFopen(
            "nitro:/p4/entry_texels.bin", "rb");
        if (sNdsEntryTexelsFile == NULL)
        {
            return FALSE;
        }
    }
    return ((ndsRendererHardwareFencedTextureFseek(sNdsEntryTexelsFile,
                                                   (long)offset, SEEK_SET) == 0) &&
            (ndsRendererHardwareFencedTextureFread(out, 1u, bytes,
                                                   sNdsEntryTexelsFile) == bytes)) ?
        TRUE : FALSE;
}

static void ndsRendererEntryTexelsClose(void)
{
    if (sNdsEntryTexelsFile != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(sNdsEntryTexelsFile);
        sNdsEntryTexelsFile = NULL;
    }
}

NDS_RENDERER_COLD static s32 ndsRendererEntryEffectTextureFill(u8 *pixels, u32 bytes,
                                              void *user_data)
{
    const NDSEntryEffectTexture *texture =
        (const NDSEntryEffectTexture *)user_data;
    u32 decoded_bytes;

    if ((pixels == NULL) || (texture == NULL) ||
        (texture->texels == NULL))
    {
        return FALSE;
    }
    if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_PAL16)
    {
        decoded_bytes = ((u32)texture->width * texture->height) >> 1;
    }
    else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_A5I3)
    {
        decoded_bytes = (u32)texture->width * texture->height;
    }
    else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_A3I5)
    {
        /* One byte per texel, the same as A5I3; only the bit split differs. */
        decoded_bytes = (u32)texture->width * texture->height;
    }
    else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_RGBA)
    {
        decoded_bytes = (u32)texture->width * texture->height * 2u;
    }
    else
    {
        return FALSE;
    }
    if (bytes < decoded_bytes)
    {
        return FALSE;
    }
    memset(pixels, 0, bytes);
    if (texture->compression == NDS_ENTRY_EFFECT_COMPRESSION_RAW)
    {
        if (texture->texel_bytes != decoded_bytes)
        {
            return FALSE;
        }
        memcpy(pixels, texture->texels, decoded_bytes);
    }
    else if (texture->compression == NDS_ENTRY_EFFECT_COMPRESSION_LZ10)
    {
        /* The generated stream carries the exact DS BIOS LZ10 header.  This
         * fill buffer already exists as the temporary upload destination, so
         * decoding here costs no persistent RAM and no gameplay-frame work. */
        if ((texture->texel_bytes < 4u) ||
            (texture->texels[0] != 0x10u) ||
            ((((u32)texture->texels[1]) |
              ((u32)texture->texels[2] << 8) |
              ((u32)texture->texels[3] << 16)) != decoded_bytes))
        {
            return FALSE;
        }
        swiDecompressLZSSWram(texture->texels, pixels);
    }
    else if (texture->compression == NDS_ENTRY_EFFECT_COMPRESSION_FILE)
    {
        if ((texture->texel_bytes != decoded_bytes) ||
            (ndsRendererEntryTexelsRead(
                 (u32)((uintptr_t)texture->texels - 1u), pixels,
                 decoded_bytes) == FALSE))
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareImpactWaveTextures(void)
{
    u32 variant;

    for (variant = 0u; variant < NDS_RENDERER_IMPACT_WAVE_VARIANT_COUNT;
         variant++)
    {
        if (sNdsRendererImpactWaveTextureName[variant] != 0u)
        {
            continue;
        }
        if (ndsRendererHardwarePrepareIFCommonPal16Atlas(
                NDS_RENDERER_IMPACT_WAVE_TEX_WIDTH,
                NDS_RENDERER_IMPACT_WAVE_TEX_HEIGHT,
                sNdsRendererImpactWavePalettes[variant],
                ndsRendererImpactWaveTexelFill, NULL,
                &sNdsRendererImpactWaveTextureName[variant]) == FALSE)
        {
            return FALSE;
        }
        NDS_DIAG(gNdsImpactWaveNativeTexturePrepareCount++);
    }
    return TRUE;
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareRebirthHaloTextures(void)
{
    static const struct
    {
        const u8 *pixels;
        u32 bytes;
        const u16 *palette;
        u32 width;
        u32 height;
        u32 a5i3;
    } textures[NDS_REBIRTH_HALO_TEXTURE_COUNT] = {
        { sNdsRebirthHaloTexelsA40, sizeof(sNdsRebirthHaloTexelsA40),
          sNdsRebirthHaloPaletteA40,
          NDS_REBIRTH_HALO_A40_WIDTH, NDS_REBIRTH_HALO_A40_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsBC8, sizeof(sNdsRebirthHaloTexelsBC8),
          sNdsRebirthHaloPaletteBC8,
          NDS_REBIRTH_HALO_BC8_WIDTH, NDS_REBIRTH_HALO_BC8_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsDD0, sizeof(sNdsRebirthHaloTexelsDD0),
          sNdsRebirthHaloPaletteDD0,
          NDS_REBIRTH_HALO_DD0_WIDTH, NDS_REBIRTH_HALO_DD0_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsE58, sizeof(sNdsRebirthHaloTexelsE58),
          sNdsRebirthHaloPaletteE58,
          NDS_REBIRTH_HALO_E58_WIDTH, NDS_REBIRTH_HALO_E58_HEIGHT, FALSE },
        { sNdsRebirthHaloTexelsBeam, sizeof(sNdsRebirthHaloTexelsBeam),
          sNdsRebirthHaloPaletteBeam,
          NDS_REBIRTH_HALO_BEAM_WIDTH, NDS_REBIRTH_HALO_BEAM_HEIGHT, TRUE },
    };
    u32 i;

    for (i = 0u; i < NDS_REBIRTH_HALO_TEXTURE_COUNT; i++)
    {
        NDSRebirthHaloTextureFill fill;

        if (sNdsRendererRebirthHaloTextureName[i] != 0u)
        {
            continue;
        }
        fill.pixels = textures[i].pixels;
        fill.bytes = textures[i].bytes;
        if (((textures[i].a5i3 != FALSE) ?
                 ndsRendererHardwarePrepareIFCommonCloudAtlas(
                     textures[i].width, textures[i].height,
                     textures[i].palette,
                     ndsRendererRebirthHaloTextureFill, &fill,
                     &sNdsRendererRebirthHaloTextureName[i]) :
                 ndsRendererHardwarePrepareIFCommonPal16Atlas(
                     textures[i].width, textures[i].height,
                     textures[i].palette,
                     ndsRendererRebirthHaloTextureFill, &fill,
                     &sNdsRendererRebirthHaloTextureName[i])) == FALSE)
        {
            return FALSE;
        }
        NDS_DIAG(gNdsRebirthHaloNativeTexturePrepareCount++);
    }
    return TRUE;
}

void ndsRendererHardwareReleaseIFCommonCloudAtlas(u32 *texture_name)
{
    int name;

    if ((texture_name == NULL) || (*texture_name == 0u))
    {
        return;
    }
    name = (int)*texture_name;
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareFencedGlDeleteTextures(1, &name);
    if (sNdsRendererHardwareBoundTextureName == *texture_name)
    {
        sNdsRendererHardwareBoundTextureName = 0u;
    }
    sNdsRendererHardwareActiveTextureEntry = NULL;
    *texture_name = 0u;
}

/* Edge of a fighter tint tile, in texels. Outside the guard below because the
 * native owner's tint batch reads it to aim its single texcoord. */
#define NDS_R2_FIGHTER_TINT_DIM 8u

/* A SOLID TEXEL, SO A TINTED BODY CAN MULTIPLY AFTER IT SHADES.
 *
 * Fighters draw PRIM x SHADE (then x ENV, which is the stage light colour and
 * white) for an untextured body and TEXEL0 x SHADE for a textured face, and the
 * N64 clamps SHADE to eight bits BEFORE either multiply. The DS geometry engine
 * saturates once, per channel, after the material fold, so a folded prim can
 * only approximate clamp8(l2 + l1*dot) * prim: the shipped diffuse cap reaches
 * prim at dot = 1 where the source sits on prim from dot ~0.5, so a lit body
 * reads darker than the face beside it. Measured on the walk ROM, Kirby
 * (runtime prim 0xFFA4B8): the capped body writes diffuse (12,10,12) / ambient
 * (19,10,11) and draws (25,15,17) at half light against the source's
 * (31,20,23), while the face at the same light draws its full texel. That is
 * the owner's face/body seam.
 *
 * The DS has exactly one post-shade multiply, the texture. So an untextured
 * body whose prim is not white draws textured with an 8x8 tile whose every
 * texel is prim, over the RAW light colours, and the pipeline evaluates
 * clamp31(RGB5(l2) + RGB5(l1)*dot) * prim -- the source's order.
 *
 * WHICH COLOURS: whatever the fighters actually carry at runtime. A costume is
 * a material animation that overwrites the MObjSub prim (Kirby's static prim is
 * green; he draws pink, and yellow in his second costume), and copy hats bring
 * their own. So there is no table to bake: the draw walk LOOKS UP the colour
 * and, on a miss, only queues it and draws the capped fold for that frame; the
 * renderer's frame boundary (ndsRendererHardwareConsumeSubmittedFrame, after
 * the batch closes and before the bind trackers reset) creates queued tiles.
 * Nothing is ever created, evicted or bound from inside a draw walk -- the
 * three routes withdrawn as r42, r43 and r44 all created or bound tiles there.
 * A colour whose upload fails is not retried until the next VRAM reset.
 *
 * 48 slots (1.5 KB of texels, 48 palettes, 768 B of table): four Crash
 * Bandicoots carry more than 16 colours at once, and with 16 the table
 * thrashed -- 344 builds, 328 evictions and 829 full-table misses in 1,200
 * frames, each build or eviction moving the set generation that retires
 * every tinted lean list (682 list materializations against 45 for the
 * original cast; Dream Land WORK P95 2.79M, 2026-10-12). */
#define NDS_R2_FIGHTER_TINT_SLOTS 48u
#define NDS_R2_FIGHTER_TINT_QUEUE 8u
#define NDS_R2_FIGHTER_TINT_BYTES \
    ((NDS_R2_FIGHTER_TINT_DIM * NDS_R2_FIGHTER_TINT_DIM) / 2u)

typedef struct NDSR2FighterTint
{
    u32 rgb;       /* 24-bit RRGGBB this tile carries */
    u32 name;
    /* P2-2p8 Phase 1 slice 1: the two words a packet records when it binds
     * this tile (glGetTexParameter / PLTT_BASE), captured at creation. The
     * PLTT base is below 0x3000 (0xFFFF: none). */
    u32 teximage;
    u16 pltt;
    u16 last_used; /* low half of sNdsRendererHardwareFrameSerial at the last
                      hit: only differences of a few frames are read */
} NDSR2FighterTint;

static NDSR2FighterTint sNdsR2FighterTints[NDS_R2_FIGHTER_TINT_SLOTS];
static u32 sNdsR2FighterTintCount;
static u32 sNdsR2FighterTintQueue[NDS_R2_FIGHTER_TINT_QUEUE];
static u32 sNdsR2FighterTintQueueCount;
static u32 sNdsR2FighterTintFailed[NDS_R2_FIGHTER_TINT_QUEUE];
static u32 sNdsR2FighterTintFailedCount;
/* Names die with glResetTextures; the set is stamped with the VRAM reset
 * generation it was built in and forgotten (not deleted) when that moves. */
static u32 sNdsR2FighterTintGeneration;
volatile u32 gNdsR2FighterTintBuilds;
volatile u32 gNdsR2FighterTintHits;
volatile u32 gNdsR2FighterTintMisses;
volatile u32 gNdsR2FighterTintFails;
volatile u32 gNdsR2FighterTintQueueFull;
volatile u32 gNdsR2FighterTintTableFull;
volatile u32 gNdsR2FighterTintEvictions;
/* Bumped whenever a tile is created or deleted; a lean list remembers the
 * generation its tint binds were resolved under (tint_set_generation). A list
 * made before its colour's tile existed carries fold words, and one made while
 * a tile existed binds it by name; either is wrong the moment the set changes.
 * Measured on r49's first cut: the first battle frame was captured before the
 * tile was built and drew the capped fold for the rest of the match. */
volatile u32 gNdsR2FighterTintSetGeneration;

static s32 ndsR2FighterTintFill(u8 *pixels, u32 bytes, void *user_data)
{
    (void)user_data;
    if ((pixels == NULL) || (bytes < NDS_R2_FIGHTER_TINT_BYTES))
    {
        return FALSE;
    }
    /* 0x11: both 4-bit texels of the byte select palette index 1 (index 0 is
     * transparent under PrepareIFCommonPal16Atlas's colour-0 rule). */
    memset(pixels, 0x11, (size_t)bytes);
    return TRUE;
}

static void ndsR2FighterTintSyncGeneration(void)
{
    u32 generation = gNdsRendererSceneTextureVramResetCount + 1u;

    if (sNdsR2FighterTintGeneration != generation)
    {
        sNdsR2FighterTintCount = 0u;
        sNdsR2FighterTintQueueCount = 0u;
        sNdsR2FighterTintFailedCount = 0u;
        sNdsR2FighterTintGeneration = generation;
        gNdsR2FighterTintSetGeneration++;
    }
}

/* The GL name of a resident tile whose every texel is `material_color`, or 0.
 * Callable from the fighter draw walk: it allocates, uploads, evicts and binds
 * nothing. A miss is queued for the frame boundary. */
static u32 ndsRendererR2FighterTintLookup(u32 material_color)
{
    u32 rgb = (material_color >> 8) & 0x00ffffffu;
    u32 i;

    ndsR2FighterTintSyncGeneration();
    for (i = 0u; i < sNdsR2FighterTintCount; i++)
    {
        if (sNdsR2FighterTints[i].rgb == rgb)
        {
            NDS_DIAG(gNdsR2FighterTintHits++);
            sNdsR2FighterTints[i].last_used =
                (u16)sNdsRendererHardwareFrameSerial;
            return sNdsR2FighterTints[i].name;
        }
    }
    NDS_DIAG(gNdsR2FighterTintMisses++);
    for (i = 0u; i < sNdsR2FighterTintFailedCount; i++)
    {
        if (sNdsR2FighterTintFailed[i] == rgb)
        {
            return 0u;
        }
    }
    for (i = 0u; i < sNdsR2FighterTintQueueCount; i++)
    {
        if (sNdsR2FighterTintQueue[i] == rgb)
        {
            return 0u;
        }
    }
    if (sNdsR2FighterTintQueueCount < NDS_R2_FIGHTER_TINT_QUEUE)
    {
        sNdsR2FighterTintQueue[sNdsR2FighterTintQueueCount++] = rgb;
    }
    else
    {
        NDS_DIAG(gNdsR2FighterTintQueueFull++);
    }
    return 0u;
}

/* Frame boundary only: create the tiles the last frame asked for. */
static void ndsRendererHardwareServiceFighterTintTiles(void)
{
    ndsR2FighterTintSyncGeneration();
    /* Not under a held frame (nds_platform.c, the transition hold): its
     * uploads could land on texels that frame still draws with. The queue
     * waits for the first frame after the Thaw. */
    if (ndsPlatformTransitionHolding() != 0u)
    {
        return;
    }
    while (sNdsR2FighterTintQueueCount != 0u)
    {
        u32 rgb = sNdsR2FighterTintQueue[--sNdsR2FighterTintQueueCount];
        u16 palette[16];
        u32 name = 0u;
        u32 i;
        s32 resident = FALSE;

        for (i = 0u; i < sNdsR2FighterTintCount; i++)
        {
            if (sNdsR2FighterTints[i].rgb == rgb)
            {
                resident = TRUE;
                break;
            }
        }
        if (resident != FALSE)
        {
            continue;
        }
        if (sNdsR2FighterTintCount >= NDS_R2_FIGHTER_TINT_SLOTS)
        {
            /* Evict the least recently used tile the frame just drawn did not
             * touch: the character select cycles through far more colours than
             * it shows at once, and a table that only fills would leave every
             * later preview on the fold. Deleting here is safe -- no walk is
             * running -- and the set generation below retires any packet that
             * recorded a bind of it. */
            u32 victim = NDS_R2_FIGHTER_TINT_SLOTS;

            /* Two frames, not one: the previous frame's polygon list may still
             * be rasterising while this one is submitted, and the tile created
             * just below could land in the block a deleted one freed. */
            for (i = 0u; i < sNdsR2FighterTintCount; i++)
            {
                if (((u16)((u16)sNdsRendererHardwareFrameSerial -
                           sNdsR2FighterTints[i].last_used) >= 2u) &&
                    ((victim == NDS_R2_FIGHTER_TINT_SLOTS) ||
                     ((s16)(sNdsR2FighterTints[i].last_used -
                            sNdsR2FighterTints[victim].last_used) < 0)))
                {
                    victim = i;
                }
            }
            if (victim == NDS_R2_FIGHTER_TINT_SLOTS)
            {
                NDS_DIAG(gNdsR2FighterTintTableFull++);
                continue;
            }
            ndsRendererHardwareReleaseIFCommonCloudAtlas(
                &sNdsR2FighterTints[victim].name);
            sNdsR2FighterTints[victim] =
                sNdsR2FighterTints[sNdsR2FighterTintCount - 1u];
            sNdsR2FighterTintCount--;
            NDS_DIAG(gNdsR2FighterTintEvictions++);
            gNdsR2FighterTintSetGeneration++;
        }
        memset(palette, 0, sizeof(palette));
        palette[1] = (u16)RGB15(((rgb >> 16) & 0xffu) >> 3,
                                ((rgb >> 8) & 0xffu) >> 3,
                                (rgb & 0xffu) >> 3);
        if (ndsRendererHardwarePrepareIFCommonPal16Atlas(
                NDS_R2_FIGHTER_TINT_DIM, NDS_R2_FIGHTER_TINT_DIM, palette,
                ndsR2FighterTintFill, NULL, &name) == FALSE)
        {
            NDS_DIAG(gNdsR2FighterTintFails++);
            if (sNdsR2FighterTintFailedCount < NDS_R2_FIGHTER_TINT_QUEUE)
            {
                sNdsR2FighterTintFailed[sNdsR2FighterTintFailedCount++] = rgb;
            }
            continue;
        }
        sNdsR2FighterTints[sNdsR2FighterTintCount].rgb = rgb;
        sNdsR2FighterTints[sNdsR2FighterTintCount].name = name;
        sNdsR2FighterTints[sNdsR2FighterTintCount].last_used =
            (u16)sNdsRendererHardwareFrameSerial;
        {
            /* P2-2p8 Phase 1 slice 1: capture exactly the words a lean list
             * binds for this tile. The prepare above returns with the tile
             * bound and its palette set (libnds's active texture), so the
             * two getters return them here -- reads only, no GX write. */
            int palette_format = -1;

            sNdsR2FighterTints[sNdsR2FighterTintCount].teximage =
                (u32)glGetTexParameter();
            glGetColorTableParameterEXT(
                GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT, &palette_format);
            sNdsR2FighterTints[sNdsR2FighterTintCount].pltt =
                ((palette_format >= 0) && (palette_format < 0xFFFF)) ?
                    (u16)palette_format : 0xFFFFu;
        }
        sNdsR2FighterTintCount++;
        NDS_DIAG(gNdsR2FighterTintBuilds++);
        gNdsR2FighterTintSetGeneration++;
    }
}

/* P2-2p8 Phase 1 slice 1: the resident tile for a colour, as the words a
 * packet bind carries. `touch` refreshes its LRU stamp (a lean draw uses it
 * without going through ndsRendererR2FighterTintLookup). */
s32 ndsFtrLeanTintTileWords(u32 rgb, u32 touch, u32 *teximage, u32 *pltt)
{
    u32 i;

    ndsR2FighterTintSyncGeneration();
    for (i = 0u; i < sNdsR2FighterTintCount; i++)
    {
        if (sNdsR2FighterTints[i].rgb == rgb)
        {
            if (touch != 0u)
            {
                sNdsR2FighterTints[i].last_used =
                    (u16)sNdsRendererHardwareFrameSerial;
            }
            *teximage = sNdsR2FighterTints[i].teximage;
            *pltt = (sNdsR2FighterTints[i].pltt == 0xFFFFu) ? 0xffffffffu :
                (u32)sNdsR2FighterTints[i].pltt;
            return TRUE;
        }
    }
    return FALSE;
}

/* P2-2p8 Phase 1 slice 7 (owner playtest, 2026-09-24): THE TILE SET OUTLIVES
 * THE BATTLE, AND A MID-MATCH TILE LIVES IN BANK D.
 *
 * Under admission 2 the admission locks A+B at its first frame end, so every
 * tile the match builds after that -- the first frame's own colours included
 * -- is allocated in D, and the battle exit hands D back to BG3. The set is
 * forgotten only at the next VRAM reset (a battle entry), so VS Results and
 * the character select after it bound those tiles by their texture-slot-3
 * words once D was a cleared BG bank: every tinted part (Mario's cap and
 * overalls, Fox's trousers) drew from nothing, in both renderer routes, until
 * the next battle. The battle exit calls this with D already locked, so libnds
 * cannot hand the released texels out again before the remap: every tile
 * whose texels lie in [lcd_lo, lcd_hi) is released, its colour queued so the
 * next frame boundary rebuilds it in A+B, and the set generation moved so
 * every packet and lean list that bound one re-derives its words. Battle exit
 * only, so size-optimized like the admission's own cold code. */
static u32 __attribute__((cold, optimize("Os")))
ndsR2FighterTintReleaseRange(u32 lcd_lo, u32 lcd_hi)
{
    u32 released = 0u;
    u32 i = 0u;

    ndsR2FighterTintSyncGeneration();
    while (i < sNdsR2FighterTintCount)
    {
        u32 lcd = 0x06800000u +
            ((sNdsR2FighterTints[i].teximage & 0xFFFFu) << 3);

        if ((lcd < lcd_lo) || (lcd >= lcd_hi))
        {
            i++;
            continue;
        }
        if (sNdsR2FighterTintQueueCount < NDS_R2_FIGHTER_TINT_QUEUE)
        {
            sNdsR2FighterTintQueue[sNdsR2FighterTintQueueCount++] =
                sNdsR2FighterTints[i].rgb;
        }
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsR2FighterTints[i].name);
        sNdsR2FighterTints[i] =
            sNdsR2FighterTints[sNdsR2FighterTintCount - 1u];
        sNdsR2FighterTintCount--;
        released++;
    }
    if (released != 0u)
    {
        gNdsR2FighterTintSetGeneration++;
    }
    return released;
}

/* Fighter entry props are required only while VSBattle presents the source
 * match intro.  Their converted texture names are direct GL residents rather
 * than cache entries, so ordinary LRU eviction can never reclaim them after
 * the entry GObjs disappear.  The generator marks only textures referenced
 * exclusively by entry roots; anything shared with a gameplay root (Link's
 * Spin effect is the important case) is deliberately retained. */
volatile u32 gNdsEntryEffectStartupTextureReleaseCount;
volatile u32 gNdsEntryEffectStartupTextureReleaseBytes;
/* "Startup only" is the generator's claim about the ENTRY ROOT LIST, and one
 * stage breaks it: Sector Z's Arwing is a ground object that draws Fox's entry
 * Arwing list for the whole match. Its nineteen textures were retired at GO
 * with the rest, so the fly-by arrived to texture name 0 and the owner refused
 * it every frame (REJECTED_PROGRAM root 0x1FA0, 2,584 in 900 frames). The
 * caller says when that stage is live; nothing else keeps its textures. */
volatile u32 gNdsEntryEffectKeepFoxArwingTextures;

NDS_RENDERER_COLD static s32 ndsRendererEntryTextureIsFoxArwing(u32 texture_slot)
{
    u32 root_index;

    for (root_index = NDS_ENTRY_EFFECT_FOX_ROOT_FIRST;
         root_index < NDS_ENTRY_EFFECT_DONKEY_ROOT_FIRST; root_index++)
    {
        const NDSEntryEffectRoot *root = &sNdsEntryEffectRoots[root_index];
        u32 g;

        for (g = 0u; g < (u32)root->group_count; g++)
        {
            if (sNdsEntryEffectGroups[(u32)root->first_group + g].texture_slot ==
                texture_slot)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* Once a match, two frames into GO: compact, not fast. */
NDS_RENDERER_COLD void __attribute__((cold, optimize("Os")))
ndsRendererHardwareReleaseEntryStartupTextures(void)
{
    u32 i;

    for (i = 0u; i < NDS_ENTRY_EFFECT_TEXTURE_COUNT; i++)
    {
        const NDSEntryEffectTexture *texture;
        u32 bytes;

        if ((sNdsEntryEffectTextureStartupOnly[i] == 0u) ||
            (sNdsRendererEntryEffectTextureName[i] == 0u))
        {
            continue;
        }
        if ((gNdsEntryEffectKeepFoxArwingTextures != 0u) &&
            (ndsRendererEntryTextureIsFoxArwing(i) != FALSE))
        {
            continue;
        }
        texture = &sNdsEntryEffectTextures[i];
        bytes = (u32)texture->width * (u32)texture->height;
        if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_PAL16)
        {
            bytes >>= 1;
        }
        else if (texture->ds_format == NDS_ENTRY_EFFECT_TEXTURE_RGBA)
        {
            bytes <<= 1;
        }
        else if ((texture->ds_format != NDS_ENTRY_EFFECT_TEXTURE_A5I3) &&
                 (texture->ds_format != NDS_ENTRY_EFFECT_TEXTURE_A3I5))
        {
            /* Generated startup-only textures are already closed over the
             * supported DS formats. Fail closed rather than accounting a
             * future format with the wrong byte width. */
            continue;
        }
        ndsRendererHardwareReleaseIFCommonCloudAtlas(
            &sNdsRendererEntryEffectTextureName[i]);
        gNdsEntryEffectStartupTextureReleaseCount++;
        gNdsEntryEffectStartupTextureReleaseBytes += bytes;
    }
}

#if NDS_VRAM_CENSUS_LIVE
/* ---------------------------------------------------------------------------
 * P2-2p8 Phase 1 slice 2a (lab only): texture and palette VRAM census.
 *
 * The ground truth is libnds's own bookkeeping: glGlobalData.texturePtrs
 * (one gl_texture_data per name: TEXIMAGE_PARAM, palette index) and the two
 * s_vramBlock allocators (the memory-order block list: free = indexOut 0).
 * Each live name is attributed to one bucket: texture-cache entries by their
 * static/warm flags or the fighter draw slot / profile owner of their last
 * upload (PRE tag), entry-effect / tint / halo+shield names by membership,
 * every other name by the site that created it (gNdsVramCensusSitePc; the
 * host maps PCs to functions). Reads only; nothing is bound or written.
 * ------------------------------------------------------------------------- */
enum
{
    nNdsVcCacheStage = 0,   /* cache entry, stage owner (dynamic) */
    nNdsVcCacheStatic,      /* cache entry with a static record / owner mask */
    nNdsVcCacheWarm,        /* cache entry kept by a stage blob (stage_warm) */
    nNdsVcCacheSlot0,       /* cache entry uploaded inside slot 0's draw */
    nNdsVcCacheSlot1,
    nNdsVcCacheSlot2,
    nNdsVcCacheSlot3,
    nNdsVcCacheOther,       /* cache entry, any other owner */
    nNdsVcEntryStartup,     /* entry-effect texture, startup-only */
    nNdsVcEntryKept,        /* entry-effect texture kept after GO */
    nNdsVcTint,             /* fighter tint tiles */
    nNdsVcHaloShield,       /* rebirth halo + entry shield textures */
    nNdsVcSiteBase          /* + creating site index; last = untagged/full */
};
#define NDS_VRAM_CENSUS_BUCKETS \
    ((u32)nNdsVcSiteBase + NDS_VRAM_CENSUS_SITE_MAX + 1u)
#define NDS_VRAM_CENSUS_KEYS 48u
#define NDS_VRAM_CENSUS_EPISODE_FIRST 798u
#define NDS_VRAM_CENSUS_EPISODE_END 1044u

typedef struct NDSVramCensusSnap
{
    u32 frame;
    u32 tex_names;          /* names holding texels */
    u32 tex_bytes;          /* sum of TEXIMAGE_PARAM texel sizes */
    u32 tex_size_field;     /* sum of libnds texSize (unit check) */
    u32 tex_free;           /* texture allocator, memory-order walk */
    u32 tex_free_blocks;
    u32 tex_largest_free;
    u32 tex_alloc_blocks;
    u32 tex_alloc_bytes;
    u32 tex_start;
    u32 tex_end;
    u32 tex_usable;         /* bytes of banks mapped as texture slots */
    u32 tex_free_usable;    /* free bytes inside those banks */
    u32 tex_largest_usable; /* largest contiguous free run inside them */
    u32 tex_free_runs_usable;
    u32 pal_names;          /* distinct palettes referenced by live names */
    u32 pal_bytes;          /* sum of their palSize */
    u32 pal_free;
    u32 pal_free_blocks;
    u32 pal_largest_free;
    u32 pal_alloc_blocks;
    u32 pal_alloc_bytes;
    u32 pal_start;
    u32 pal_end;
    u32 pal_usable;
    u32 pal_free_usable;
    u32 pal_largest_usable;
    u32 pal_free_runs_usable;
    u32 dispcnt;
    u32 vramcnt_abcd;
    u32 vramcnt_efg;
    u32 bg2_opaque;         /* explicit captures only, else 0 */
    u32 bg3_opaque;
    u32 last_request;       /* gNdsVramCensusLastRequest at capture */
    u32 bytes[NDS_VRAM_CENSUS_BUCKETS];
    u32 count[NDS_VRAM_CENSUS_BUCKETS];
    u32 pal[NDS_VRAM_CENSUS_BUCKETS];
    /* Slice 2b: the part of tex_bytes / bytes[] whose texels live in bank D
     * (LCD 0x06860000-0x0687ffff, texture slot 3 while a battle holds it). */
    u32 tex_bytes_d;
    u32 bytes_d[NDS_VRAM_CENSUS_BUCKETS];
} NDSVramCensusSnap;

typedef struct NDSVramCensus
{
    u32 frames;
    u32 captures;
    u32 walk_guard_hits;
    u32 name_limit;
    NDSVramCensusSnap live;     /* every enabled frame end */
    NDSVramCensusSnap go;       /* first frame end at GO */
    NDSVramCensusSnap burst;    /* first fighter texture reject */
    NDSVramCensusSnap episode;  /* min largest-free frame in [798, 1044) */
    NDSVramCensusSnap after;    /* min largest-free frame from 1044 */
    u32 peak_bytes[NDS_VRAM_CENSUS_BUCKETS];
    u32 peak_count[NDS_VRAM_CENSUS_BUCKETS];
    u32 peak_tex_bytes;
    u32 peak_pal_bytes;
    u32 min_largest_free;
    u32 min_largest_free_frame;
    u32 bg_frame[2];            /* two mid-match overlay pixel counts */
    u32 bg2_opaque[2];
    u32 bg3_opaque[2];
    /* Fighter texture keys, one record per distinct upload inside a draw:
     * [0] image asset << 20 | offset, [1] tlut asset << 20 | offset
     * (0xffffffff none, 0xfffffffe unresolved), [2] N64 tile fmt | siz << 3
     * | DS fmt << 5 | DS s-size << 8 | t-size << 11 | palette code << 14
     * (0 none, 1 <= 4, 2 <= 16, 3 <= 256 entries) | repeat/flip << 16
     * | uploads << 20. */
    u32 key_count[4];
    u32 key_overflow[4];
    u32 key_uploads[4];
    u32 key_uploads_after_go[4];
    u32 key_prov_fail[4];
    u32 key[4][NDS_VRAM_CENSUS_KEYS][3];
} NDSVramCensus;

NDSVramCensus gNdsVramCensus __attribute__((used, aligned(32)));

/* Slice 2b (lab): what the admission did, and every fighter-draw upload after
 * it finished -- a key the admitted set does not hold (the gate counter). */
typedef struct NDSFtrAdmitLab
{
    u32 runs;
    u32 word;
    u32 frame;
    u32 regions;            /* bank D is a texture bank, A+B locked */
    u32 bg3_not_empty;      /* word 2 refused: the battle uses BG3 */
    u32 lock_before;
    u32 lock_after;
    u32 records;
    u32 skipped_costume;
    u32 skipped_hat;
    u32 applied;            /* records the cache resolved */
    u32 fails;
    u32 asset_fails;
    u32 entries_admitted;   /* distinct cache entries marked admitted */
    u32 bytes_admitted;
    u32 bytes_in_d;
    u32 bytes_in_ab;
    u32 per_fighter_applied[4];
    u32 per_fighter_bytes[4];
    u32 locked;             /* A+B locked: D is the only allocating region */
    u32 lock_frame;
    u32 free_before;        /* usable texture free / largest run before D */
    u32 largest_before;
    u32 free_after;         /* after the admission, D included */
    u32 largest_after;
    u32 exit_safety;        /* regions closed by the scene-reset guard */
    u32 bases;              /* loaded fighter files the adapter handed over */
    u32 pruned;             /* records whose span the compact pack omitted */
    u32 absent;             /* records whose file is not loaded this battle */
    u32 libc_skipped;       /* records left on-demand at the libc floor */
    u32 slot_skipped;       /* records left on-demand at the slot reserve */
    u32 libc_top_min;       /* lowest libc top chunk seen by the admission */
    u32 read_ticks;         /* NitroFS open + index + chunk reads */
    u32 apply_ticks;        /* replay + resolve/convert/upload */
    u32 reject_mask;        /* NDS_RENDERER_HW_TEXREJECT_* of refused records */
    u32 reject_first[6];    /* reason, index, render settile w0/w1, tile size
                             * w0/w1 of the first refused record */
    u32 ticks;
    u32 exit_runs;
    u32 exit_released;
    u32 exit_orphans;
    u32 exit_tints;         /* slice 7: fighter tint tiles released from D */
    u32 done;
    u32 outside[4];         /* fighter-draw uploads after admission, per slot */
    u32 outside_count;
    u32 outside_first[8][3];
    /* Per slot, the first outside upload's whole cache key, and the key of an
     * admitted entry with the same image (state 1), or state 2 when no
     * admitted entry has that image at all. */
    u32 diag_state[4];
    u32 diag_same_image[4];
    u32 diag_outside_key[4][59];
    u32 diag_admit_key[4][59];
} NDSFtrAdmitLab;

NDSFtrAdmitLab gNdsFtrAdmitLab __attribute__((used, aligned(32)));
volatile u32 gNdsFtrOutsideLr[8][4] __attribute__((used));
#define NDS_FTR_ADMIT_LAB(stmt) do { stmt; } while (0)
static u8 sNdsVramCensusBucketOf[NDS_VRAM_CENSUS_NAME_MAX];
static u32 sNdsVramCensusPalSeen[NDS_VRAM_CENSUS_NAME_MAX / 32u];

/* P8 V4: the bank map's rows -- every live texture name at the last capture:
 * its texel address (the LCD alias 0x06800000.. of banks A-D), bytes,
 * TEXIMAGE_PARAM format and size fields, owner bucket and the bytes of the
 * palette it is the first to name. scripts/tools/vram_map.ps1 reads them at
 * the census frame. */
typedef struct NDSVramCensusMapRow
{
    u32 addr;
    u32 bytes;
    u16 name;
    u8 bucket;
    u8 format;
    u16 pal_bytes;
    u16 size;
} NDSVramCensusMapRow;
__attribute__((used)) NDSVramCensusMapRow gNdsVramCensusMap[NDS_VRAM_CENSUS_NAME_MAX];
__attribute__((used)) volatile u32 gNdsVramCensusMapCount;
/* And libnds's texture allocator blocks in address order: VRAM held without a
 * texture name (the fighter admission's carve regions) is only here. */
#define NDS_VRAM_CENSUS_BLOCK_MAX 512u
typedef struct NDSVramCensusBlockRow
{
    u32 addr;
    u32 bytes;
    u32 used;
} NDSVramCensusBlockRow;
__attribute__((used)) NDSVramCensusBlockRow gNdsVramCensusBlockMap[NDS_VRAM_CENSUS_BLOCK_MAX];
__attribute__((used)) volatile u32 gNdsVramCensusBlockCount;

u32 ndsPlatformVramCensusOverlayOpaque(u32 layer);

static u32 ndsVramCensusParamsBytes(u32 params)
{
    static const u8 bits_per_texel[8] = { 0u, 8u, 2u, 4u, 8u, 2u, 8u, 16u };
    u32 width = 8u << ((params >> 20) & 7u);
    u32 height = 8u << ((params >> 23) & 7u);

    return (width * height * (u32)bits_per_texel[(params >> 26) & 7u]) / 8u;
}

/* The usable part of an allocator: the LCD ranges of the banks whose
 * VRAMCNT mode matches (3 = texture slot for A-D, texture palette for
 * E-G), merged when adjacent. libnds's allocators span A-D / E-G whatever
 * the banks are mapped as; glTexImage2D skips banks in another mode. */
static u32 ndsVramCensusSpans(u32 palettes, u32 *span_start, u32 *span_end)
{
    static const u32 kTexBase[4] = { 0x06800000u, 0x06820000u, 0x06840000u,
                                     0x06860000u };
    static const u32 kPalBase[3] = { 0x06880000u, 0x06890000u, 0x06894000u };
    static const u32 kPalSize[3] = { 0x10000u, 0x4000u, 0x4000u };
    const volatile u8 *cnt = (const volatile u8 *)0x04000240u;
    u32 spans = 0u;
    u32 i;

    for (i = 0u; i < (palettes ? 3u : 4u); i++)
    {
        u32 mode = palettes ? cnt[4u + i] : cnt[i];
        u32 base = palettes ? kPalBase[i] : kTexBase[i];
        u32 size = palettes ? kPalSize[i] : 0x20000u;

        if (((mode & 0x80u) == 0u) || ((mode & 7u) != 3u))
        {
            continue;
        }
        if ((spans != 0u) && (span_end[spans - 1u] == base))
        {
            span_end[spans - 1u] = base + size;
        }
        else
        {
            span_start[spans] = base;
            span_end[spans] = base + size;
            spans++;
        }
    }
    return spans;
}

/* out: 0 free, 1 free blocks, 2 largest free, 3 alloc blocks, 4 alloc
 * bytes, 5 start, 6 end, 7 usable bytes, 8 usable free, 9 usable largest
 * contiguous free run, 10 usable free runs. */
static void ndsVramCensusBlocks(const s_vramBlock *block, u32 palettes,
                                u32 *out)
{
    const s_SingleBlock *b;
    u32 span_start[4];
    u32 span_end[4];
    u32 spans;
    u32 run_end = 0u;
    u32 run = 0u;
    u32 guard = 0u;
    u32 i;

    for (i = 0u; i < 11u; i++)
    {
        out[i] = 0u;
    }
    if (block == NULL)
    {
        return;
    }
    out[5] = (u32)(uintptr_t)block->startAddr;
    out[6] = (u32)(uintptr_t)block->endAddr;
    spans = ndsVramCensusSpans(palettes, span_start, span_end);
    for (i = 0u; i < spans; i++)
    {
        out[7] += span_end[i] - span_start[i];
    }
    if (palettes == 0u)
    {
        gNdsVramCensusBlockCount = 0u;
    }
    for (b = block->firstBlock; (b != NULL) && (guard < 4096u);
         b = b->node[1], guard++)
    {
        u32 lo = (u32)(uintptr_t)b->AddrSet;
        u32 hi = lo + b->blockSize;

        if ((palettes == 0u) &&
            (gNdsVramCensusBlockCount < NDS_VRAM_CENSUS_BLOCK_MAX))
        {
            NDSVramCensusBlockRow *row =
                &gNdsVramCensusBlockMap[gNdsVramCensusBlockCount++];

            row->addr = lo;
            row->bytes = b->blockSize;
            row->used = (b->indexOut != 0u) ? 1u : 0u;
        }

        if (b->indexOut == 0u)
        {
            out[0] += b->blockSize;
            out[1]++;
            if (b->blockSize > out[2])
            {
                out[2] = b->blockSize;
            }
            for (i = 0u; i < spans; i++)
            {
                u32 a = (lo > span_start[i]) ? lo : span_start[i];
                u32 z = (hi < span_end[i]) ? hi : span_end[i];

                if (a >= z)
                {
                    continue;
                }
                out[8] += z - a;
                /* Adjacent free blocks inside one span form one run. */
                if ((run != 0u) && (run_end == a))
                {
                    run += z - a;
                }
                else
                {
                    run = z - a;
                    out[10]++;
                }
                run_end = z;
                if (run > out[9])
                {
                    out[9] = run;
                }
            }
        }
        else
        {
            out[3]++;
            out[4] += b->blockSize;
            run = 0u;
        }
    }
    if (guard >= 4096u)
    {
        gNdsVramCensus.walk_guard_hits++;
    }
}

static u32 ndsVramCensusCacheBucket(
    const NDSRendererHardwareTextureCacheEntry *entry, u32 tag)
{
    u32 slot_plus1 = (tag >> 5) & 7u;

    if ((entry->static_record_plus1 != 0u) || (entry->static_owner_mask != 0u))
    {
        return nNdsVcCacheStatic;
    }
    if (entry->stage_warm != 0u)
    {
        return nNdsVcCacheWarm;
    }
    if ((slot_plus1 >= 1u) && (slot_plus1 <= 4u))
    {
        return (u32)nNdsVcCacheSlot0 + slot_plus1 - 1u;
    }
    if (((tag >> 8) & 31u) == (u32)NDS_RENDERER_PROFILE_OWNER_STAGE)
    {
        return nNdsVcCacheStage;
    }
    return nNdsVcCacheOther;
}

static void ndsVramCensusMark(u32 name, u32 bucket)
{
    if ((name != 0u) && (name < NDS_VRAM_CENSUS_NAME_MAX))
    {
        sNdsVramCensusBucketOf[name] = (u8)bucket;
    }
}

static void ndsVramCensusCapture(NDSVramCensusSnap *snap, u32 pixels)
{
    const DynamicArray *textures = &glGlobalData.texturePtrs;
    const DynamicArray *palettes = &glGlobalData.palettePtrs;
    u32 blocks[11];
    u32 limit;
    u32 name;
    u32 i;

    memset(snap, 0, sizeof(*snap));
    snap->frame = gNdsRendererProfileFrameCount;
    snap->last_request = gNdsVramCensusLastRequest;
    snap->dispcnt = REG_DISPCNT;
    snap->vramcnt_abcd = *(volatile u32 *)0x04000240u;
    snap->vramcnt_efg = *(volatile u32 *)0x04000244u;

    memset(sNdsVramCensusBucketOf, 0xff, sizeof(sNdsVramCensusBucketOf));
    memset(sNdsVramCensusPalSeen, 0, sizeof(sNdsVramCensusPalSeen));
    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        const NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];
        u32 entry_name = (u32)entry->name;

        if ((entry_name != 0u) && (entry_name < NDS_VRAM_CENSUS_NAME_MAX))
        {
            ndsVramCensusMark(entry_name, ndsVramCensusCacheBucket(
                entry, sNdsVramCensusTag[entry_name]));
        }
    }
    for (i = 0u; i < NDS_ENTRY_EFFECT_TEXTURE_COUNT; i++)
    {
        ndsVramCensusMark(sNdsRendererEntryEffectTextureName[i],
                          (sNdsEntryEffectTextureStartupOnly[i] != 0u) ?
                              nNdsVcEntryStartup : nNdsVcEntryKept);
    }
    for (i = 0u; i < sNdsR2FighterTintCount; i++)
    {
        ndsVramCensusMark(sNdsR2FighterTints[i].name, nNdsVcTint);
    }
    for (i = 0u; i < NDS_REBIRTH_HALO_TEXTURE_COUNT; i++)
    {
        ndsVramCensusMark(sNdsRendererRebirthHaloTextureName[i],
                          nNdsVcHaloShield);
    }
    for (i = 0u; i < 5u; i++)
    {
        ndsVramCensusMark(sNdsEntryShieldTextureName[i], nNdsVcHaloShield);
    }

    limit = textures->cur_size;
    if (limit > NDS_VRAM_CENSUS_NAME_MAX)
    {
        limit = NDS_VRAM_CENSUS_NAME_MAX;
    }
    gNdsVramCensus.name_limit = textures->cur_size;
    gNdsVramCensusMapCount = 0u;
    for (name = 1u; name < limit; name++)
    {
        const gl_texture_data *tex =
            (const gl_texture_data *)textures->data[name];
        u32 bucket;
        u32 bytes;

        if (tex == NULL)
        {
            continue;
        }
        bucket = sNdsVramCensusBucketOf[name];
        if (bucket == 0xffu)
        {
            u32 tag = sNdsVramCensusTag[name];
            u32 site = tag & 31u;

            bucket = (((tag & 0x4000u) == 0u) ||
                      (site >= NDS_VRAM_CENSUS_SITE_MAX)) ?
                ((u32)nNdsVcSiteBase + NDS_VRAM_CENSUS_SITE_MAX) :
                ((u32)nNdsVcSiteBase + site);
        }
        bytes = (tex->vramAddr != NULL) ?
            ndsVramCensusParamsBytes(tex->texFormat) : 0u;
        if (bytes != 0u)
        {
            u32 addr = (u32)(uintptr_t)tex->vramAddr;
            NDSVramCensusMapRow *row =
                &gNdsVramCensusMap[gNdsVramCensusMapCount++];

            row->addr = addr;
            row->bytes = bytes;
            row->name = (u16)name;
            row->bucket = (u8)bucket;
            row->format = (u8)((tex->texFormat >> 26) & 7u);
            row->size = (u16)((tex->texFormat >> 20) & 0x3fu);
            row->pal_bytes = 0u;
            snap->tex_names++;
            snap->tex_bytes += bytes;
            snap->tex_size_field += tex->texSize;
            snap->bytes[bucket] += bytes;
            snap->count[bucket]++;
            if ((addr >= 0x06860000u) && (addr < 0x06880000u))
            {
                snap->tex_bytes_d += bytes;
                snap->bytes_d[bucket] += bytes;
            }
        }
        if ((tex->palIndex > 0) &&
            ((u32)tex->palIndex < palettes->cur_size) &&
            ((u32)tex->palIndex < NDS_VRAM_CENSUS_NAME_MAX) &&
            ((sNdsVramCensusPalSeen[(u32)tex->palIndex >> 5] &
              (1u << ((u32)tex->palIndex & 31u))) == 0u))
        {
            const gl_palette_data *pal =
                (const gl_palette_data *)palettes->data[tex->palIndex];

            sNdsVramCensusPalSeen[(u32)tex->palIndex >> 5] |=
                1u << ((u32)tex->palIndex & 31u);
            if (pal != NULL)
            {
                snap->pal_names++;
                snap->pal_bytes += pal->palSize;
                snap->pal[bucket] += pal->palSize;
                if ((bytes != 0u) && (gNdsVramCensusMapCount != 0u))
                {
                    gNdsVramCensusMap[gNdsVramCensusMapCount - 1u].pal_bytes =
                        (u16)pal->palSize;
                }
            }
        }
    }
    ndsVramCensusBlocks(glGlobalData.vramBlocks[0], 0u, blocks);
    snap->tex_usable = blocks[7];
    snap->tex_free_usable = blocks[8];
    snap->tex_largest_usable = blocks[9];
    snap->tex_free_runs_usable = blocks[10];
    snap->tex_free = blocks[0];
    snap->tex_free_blocks = blocks[1];
    snap->tex_largest_free = blocks[2];
    snap->tex_alloc_blocks = blocks[3];
    snap->tex_alloc_bytes = blocks[4];
    snap->tex_start = blocks[5];
    snap->tex_end = blocks[6];
    ndsVramCensusBlocks(glGlobalData.vramBlocks[1], 1u, blocks);
    snap->pal_usable = blocks[7];
    snap->pal_free_usable = blocks[8];
    snap->pal_largest_usable = blocks[9];
    snap->pal_free_runs_usable = blocks[10];
    snap->pal_free = blocks[0];
    snap->pal_free_blocks = blocks[1];
    snap->pal_largest_free = blocks[2];
    snap->pal_alloc_blocks = blocks[3];
    snap->pal_alloc_bytes = blocks[4];
    snap->pal_start = blocks[5];
    snap->pal_end = blocks[6];
    if (pixels != 0u)
    {
        snap->bg2_opaque = ndsPlatformVramCensusOverlayOpaque(0u);
        snap->bg3_opaque = ndsPlatformVramCensusOverlayOpaque(1u);
    }
    gNdsVramCensus.captures++;
}

void ndsVramCensusCaptureBurst(void)
{
    if ((gNdsVramCensusEnable == 0u) || (gNdsVramCensus.burst.frame != 0u))
    {
        return;
    }
    ndsVramCensusCapture(&gNdsVramCensus.burst, 1u);
    DC_FlushRange(&gNdsVramCensus, sizeof(gNdsVramCensus));
}

static void ndsFtrCarveLabPublish(void);

void ndsVramCensusFrame(void)
{
    NDSVramCensus *census = &gNdsVramCensus;
    NDSVramCensusSnap *live = &census->live;
    u32 frame;
    u32 i;

    /* Slice 2c lab blocks: the sampler reads memory, not the D-cache. */
    ndsFtrCarveLabPublish();
    if (gNdsVramCensusEnable == 0u)
    {
        return;
    }
    ndsVramCensusCapture(live, 0u);
    frame = live->frame;
    census->frames++;
    for (i = 0u; i < NDS_VRAM_CENSUS_BUCKETS; i++)
    {
        if (live->bytes[i] > census->peak_bytes[i])
        {
            census->peak_bytes[i] = live->bytes[i];
        }
        if (live->count[i] > census->peak_count[i])
        {
            census->peak_count[i] = live->count[i];
        }
    }
    if (live->tex_bytes > census->peak_tex_bytes)
    {
        census->peak_tex_bytes = live->tex_bytes;
    }
    if (live->pal_bytes > census->peak_pal_bytes)
    {
        census->peak_pal_bytes = live->pal_bytes;
    }
    if ((census->min_largest_free_frame == 0u) ||
        (live->tex_largest_usable < census->min_largest_free))
    {
        census->min_largest_free = live->tex_largest_usable;
        census->min_largest_free_frame = frame;
    }
    if ((census->go.frame == 0u) && (gNdsFtrLean.go_frame != 0u))
    {
        ndsVramCensusCapture(&census->go, 1u);
    }
    if ((frame >= NDS_VRAM_CENSUS_EPISODE_FIRST) &&
        (frame < NDS_VRAM_CENSUS_EPISODE_END) &&
        ((census->episode.frame == 0u) ||
         (live->tex_largest_usable < census->episode.tex_largest_usable)))
    {
        census->episode = *live;
    }
    if ((frame >= NDS_VRAM_CENSUS_EPISODE_END) &&
        ((census->after.frame == 0u) ||
         (live->tex_largest_usable < census->after.tex_largest_usable)))
    {
        census->after = *live;
    }
    for (i = 0u; i < 2u; i++)
    {
        static const u32 kBgFrame[2] = { 900u, 1500u };

        if ((census->bg_frame[i] == 0u) && (frame >= kBgFrame[i]))
        {
            census->bg_frame[i] = frame;
            census->bg2_opaque[i] = ndsPlatformVramCensusOverlayOpaque(0u);
            census->bg3_opaque[i] = ndsPlatformVramCensusOverlayOpaque(1u);
        }
    }
    DC_FlushRange(census, sizeof(*census));
}

/* One distinct fighter texture per record; the host checks every image and
 * TLUT against the fighter's enumerated closure. */
static void ndsVramCensusNoteCacheKey(
    const NDSRendererHardwareTextureCacheEntry *entry,
    const NDSRendererHardwareTextureKey *key, u32 palette_entries)
{
    NDSVramCensus *census = &gNdsVramCensus;
    u32 slot_plus1 = gNdsVramCensusDrawSlotPlus1;
    u32 slot;
    u32 asset = 0u;
    u32 offset = 0u;
    u32 w0 = 0xffffffffu;
    u32 w1 = 0xffffffffu;
    u32 w2;
    u32 pal_code;
    u32 i;

    /* Always on in a census build (not gated by gNdsVramCensusEnable): the
     * first fighter draws happen before the sampler's first poke. */
    if ((slot_plus1 == 0u) || (slot_plus1 > 4u))
    {
        return;
    }
    slot = slot_plus1 - 1u;
    census->key_uploads[slot]++;
    if (gNdsFtrAdmitLab.done != 0u)
    {
        gNdsFtrAdmitLab.outside[slot]++;
    }
    if (gNdsFtrLean.go_frame != 0u)
    {
        census->key_uploads_after_go[slot]++;
    }
    if (ndsRelocGetLoadedPointerProvenance(
            (const void *)(uintptr_t)key->image, &asset, &offset) != FALSE)
    {
        w0 = ((asset & 0xfffu) << 20) | (offset & 0xfffffu);
    }
    else
    {
        census->key_prov_fail[slot]++;
    }
    if (key->tlut_image != 0u)
    {
        if (ndsRelocGetLoadedPointerProvenance(
                (const void *)(uintptr_t)key->tlut_image, &asset,
                &offset) != FALSE)
        {
            w1 = ((asset & 0xfffu) << 20) | (offset & 0xfffffu);
        }
        else
        {
            w1 = 0xfffffffeu;
            census->key_prov_fail[slot]++;
        }
    }
    pal_code = (palette_entries == 0u) ? 0u :
        (palette_entries <= 4u) ? 1u : (palette_entries <= 16u) ? 2u : 3u;
    w2 = (key->format & 7u) | ((key->size & 3u) << 3) |
        (((entry->params >> 26) & 7u) << 5) |
        (((entry->params >> 20) & 7u) << 8) |
        (((entry->params >> 23) & 7u) << 11) | (pal_code << 14) |
        (((entry->params >> 16) & 0xfu) << 16);
    if ((gNdsFtrAdmitLab.done != 0u) && (gNdsFtrAdmitLab.outside_count < 8u))
    {
        /* Slice 2c (lab): who uploaded it -- the return address of the
         * resolve path's caller (this note is inlined into it) -- and the
         * native-failure count at that moment. */
        gNdsFtrOutsideLr[gNdsFtrAdmitLab.outside_count][0] =
            (u32)(uintptr_t)__builtin_return_address(0);
        gNdsFtrOutsideLr[gNdsFtrAdmitLab.outside_count][1] =
            gNdsRendererNativeFailure.count;
        gNdsFtrOutsideLr[gNdsFtrAdmitLab.outside_count][2] =
            gNdsRendererProfileFrameCount;
        gNdsFtrOutsideLr[gNdsFtrAdmitLab.outside_count][3] =
            key->image_format | (key->image_size << 8) |
            (key->image_width << 16);
        gNdsFtrAdmitLab.outside_first[gNdsFtrAdmitLab.outside_count][0] =
            (slot << 28) | (w0 & 0x0fffffffu);
        gNdsFtrAdmitLab.outside_first[gNdsFtrAdmitLab.outside_count][1] = w1;
        gNdsFtrAdmitLab.outside_first[gNdsFtrAdmitLab.outside_count][2] = w2;
        gNdsFtrAdmitLab.outside_count++;
    }
    for (i = 0u; i < census->key_count[slot]; i++)
    {
        u32 *record = census->key[slot][i];

        if ((record[0] == w0) && (record[1] == w1) &&
            ((record[2] & 0xfffffu) == w2))
        {
            if ((record[2] >> 20) != 0xfffu)
            {
                record[2] += 0x100000u;
            }
            return;
        }
    }
    if (census->key_count[slot] >= NDS_VRAM_CENSUS_KEYS)
    {
        census->key_overflow[slot]++;
        return;
    }
    census->key[slot][census->key_count[slot]][0] = w0;
    census->key[slot][census->key_count[slot]][1] = w1;
    census->key[slot][census->key_count[slot]][2] = w2 | 0x100000u;
    census->key_count[slot]++;
}
#endif

#if NDS_P2_STAGE_HYRULE
typedef struct NDSHyruleTextureFill
{
    FILE *file;
    u32 offset;
} NDSHyruleTextureFill;

NDS_RENDERER_COLD static s32 ndsHyruleTextureFill(u8 *pixels, u32 bytes, void *user)
{
    const NDSHyruleTextureFill *fill = user;

    return ((fill->offset <= NDS_HYRULE_NATIVE_ASSET_BYTES) &&
        (bytes <= NDS_HYRULE_NATIVE_ASSET_BYTES - fill->offset) &&
        (ndsRendererHardwareFencedTextureFseek(fill->file, fill->offset, SEEK_SET) == 0) &&
        (ndsRendererHardwareFencedTextureFread(pixels, 1, bytes, fill->file) == bytes));
}

NDS_RENDERER_COLD void ndsRendererHardwareReleaseHyruleTextures(void)
{
    u32 i;

    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    /* Delete references before their shared palette owner. Idempotent after
     * scene exit, cache reset or a partially completed texture preparation. */
    for (i = NDS_HYRULE_NATIVE_FRAME_COUNT; i != 0u; i--)
    {
        ndsRendererHardwareReleaseIFCommonCloudAtlas(&sNdsHyruleNativeNames[i - 1u]);
    }
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareHyruleTextures(void)
{
    FILE *file = NULL;
    u16 palette[256];
    u32 texture_id;
    u32 index = 0u;

    if (sNdsHyruleNativeNames[NDS_HYRULE_NATIVE_FRAME_COUNT - 1u] != 0u)
    {
        return TRUE;
    }
    NDS_DIAG(gNdsHyruleNativeTexturePrepareCount++);
    file = ndsRendererHardwareFencedTextureFopen(NDS_HYRULE_NATIVE_ASSET_PATH, "rb");
    if ((file == NULL) ||
        (ndsRendererHardwareFencedTextureFseek(file, 0, SEEK_END) != 0) ||
        (ndsRendererHardwareFencedTextureFtell(file) != NDS_HYRULE_NATIVE_ASSET_BYTES))
    {
        goto fail;
    }
    for (texture_id = 0u; texture_id < NDS_HYRULE_NATIVE_TEXTURE_COUNT; texture_id++)
    {
        const NDSHyruleNativeTexture *texture = &gNdsHyruleNativeTextures[texture_id];
        u32 frame;
        u32 first_name = index;
        u32 format;
        u32 transparent;

        switch (texture->ds_format)
        {
        case NDS_PARTICLE_FORMAT_PAL16: format = GL_RGB16; transparent = TRUE; break;
        case NDS_PARTICLE_FORMAT_A5I3: format = GL_RGB8_A5; transparent = FALSE; break;
        case NDS_PARTICLE_FORMAT_PAL256: format = GL_RGB256; transparent = TRUE; break;
        default: goto fail;
        }
        if ((texture->palette_entries == 0u) || (texture->palette_entries > 256u) ||
            (texture->frames == 0u) || (texture->frames > NDS_HYRULE_NATIVE_FRAME_COUNT - index) ||
            (texture->palette_offset > NDS_HYRULE_NATIVE_ASSET_BYTES) ||
            (texture->palette_entries * 2u > NDS_HYRULE_NATIVE_ASSET_BYTES - texture->palette_offset) ||
            (ndsRendererHardwareFencedTextureFseek(file, texture->palette_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(palette, 2, texture->palette_entries, file) != texture->palette_entries))
        {
            goto fail;
        }
        for (frame = 0u; frame < texture->frames; frame++, index++)
        {
            NDSHyruleTextureFill fill = { file,
                texture->data_offset + frame * texture->frame_bytes };
            int palette_width = 0;

            if (ndsRendererHardwarePrepareIFCommonAtlas(texture->width, texture->height,
                    format, palette, texture->palette_entries, ndsHyruleTextureFill,
                    &fill, &sNdsHyruleNativeNames[index], transparent) == FALSE)
            {
                goto fail;
            }
            if (frame != 0u)
            {
                /* All five source frames use the same 254-entry palette.
                 * libnds owns its reference count and frees the replaced copy. */
                ndsRendererHardwareBindTextureState((int)sNdsHyruleNativeNames[index]);
                glAssignColorTable(GL_TEXTURE_2D, (int)sNdsHyruleNativeNames[first_name]);
            }
            glGetColorTableParameterEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT,
                                       &palette_width);
            if (palette_width < texture->palette_entries)
            {
                goto fail;
            }
        }
    }
    if (index != NDS_HYRULE_NATIVE_FRAME_COUNT) { goto fail; }
    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        goto fail;
    }
    return TRUE;

fail:
    if (file != NULL) { (void)ndsRendererHardwareFencedTextureFclose(file); }
    ndsRendererHardwareReleaseHyruleTextures();
    NDS_DIAG(gNdsHyruleNativeTextureFailCount++);
    return FALSE;
}

u32 ndsRendererHardwareHyruleTextureName(u32 texture, u32 frame)
{
    u32 index = 0u;
    u32 i;

    if ((texture >= NDS_HYRULE_NATIVE_TEXTURE_COUNT) ||
        (frame >= gNdsHyruleNativeTextures[texture].frames))
    {
        return 0u;
    }
    for (i = 0u; i < texture; i++) { index += gNdsHyruleNativeTextures[i].frames; }
    return sNdsHyruleNativeNames[index + frame];
}
#endif

static v16 ndsRendererHardwareIFCommonScreenX(s32 pixel_q16)
{
    s64 scaled = (s64)pixel_q16 * 32;
    s64 rounded = (scaled >= 0) ? (scaled + 0x8000) >> 16 :
                                     -(((-scaled) + 0x8000) >> 16);

    rounded -= 4096;
    if (rounded < -32768)
    {
        rounded = -32768;
    }
    else if (rounded > 32767)
    {
        rounded = 32767;
    }
    return (v16)rounded;
}

static v16 ndsRendererHardwareIFCommonScreenY(s32 pixel_q16)
{
    s64 scaled = (s64)pixel_q16 * 128;
    const s64 denominator = 3 * 65536;
    s64 rounded = (scaled >= 0) ?
        (scaled + (denominator / 2)) / denominator :
        -(((-scaled) + (denominator / 2)) / denominator);

    rounded = 4096 - rounded;
    if (rounded < -32768)
    {
        rounded = -32768;
    }
    else if (rounded > 32767)
    {
        rounded = 32767;
    }
    return (v16)rounded;
}

s32 ndsRendererHardwareDrawIFCommonCloudAtlas(
    u32 texture_name, s32 x_q16, s32 y_q16,
    s32 width_q16, s32 height_q16,
    u32 texture_x, u32 texture_y, u32 texture_width,
    u32 texture_height, u32 poly_id)
{
    NDSRendererIFCommonCloudDraw *draw;

    if ((texture_name == 0u) || (width_q16 <= 0) || (height_q16 <= 0) ||
        (texture_width == 0u) || (texture_height == 0u) ||
        (texture_x + texture_width > 256u) ||
        (texture_y + texture_height > 128u) || (poly_id > 63u) ||
        (sNdsRendererIFCommonCloudQueueCount >=
         NDS_RENDERER_IFCOMMON_CLOUD_QUEUE_COUNT))
    {
        return FALSE;
    }
    draw = &sNdsRendererIFCommonCloudQueue[
        sNdsRendererIFCommonCloudQueueCount++];
    draw->texture_name = texture_name;
    draw->x_q16 = x_q16;
    draw->y_q16 = y_q16;
    draw->width_q16 = width_q16;
    draw->height_q16 = height_q16;
    draw->texture_x = texture_x;
    draw->texture_y = texture_y;
    draw->texture_width = texture_width;
    draw->texture_height = texture_height;
    draw->poly_id = poly_id;
    gNdsRendererIFCommonCloudQueuedCount++;
    return TRUE;
}

static void ndsRendererHardwareEmitIFCommonClouds(void)
{
    u32 draw_index;

    if (sNdsRendererIFCommonCloudQueueCount == 0u)
    {
        return;
    }

    /* Emit IFCommon traffic, Contour, and Light quads at the final renderer
     * boundary in the order queued by the source pass. */
    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glColor(RGB15(31, 31, 31));
    for (draw_index = 0u;
         draw_index < sNdsRendererIFCommonCloudQueueCount; draw_index++)
    {
        const NDSRendererIFCommonCloudDraw *draw =
            &sNdsRendererIFCommonCloudQueue[draw_index];
        v16 left = ndsRendererHardwareIFCommonScreenX(draw->x_q16);
        v16 right = ndsRendererHardwareIFCommonScreenX(
            draw->x_q16 + draw->width_q16);
        v16 top = ndsRendererHardwareIFCommonScreenY(draw->y_q16);
        v16 bottom = ndsRendererHardwareIFCommonScreenY(
            draw->y_q16 + draw->height_q16);
        /* Half a texel in (owner, 2026-10-06: the GO lamp's 12x9 lettering
         * read "&8" on the DS): the DS starts each pixel's texture
         * coordinate at the pixel's edge, so a 1:1 quad sampled every texel
         * exactly on its boundary, and the edge walk's rounding (the Y map is
         * 42.67 vertex units a pixel) flipped alternate rows a texel left or
         * right. Sampling at texel centres is the same 1:1 image, stable. */
        t16 tex_left = (t16)((draw->texture_x << 4) + 8);
        t16 tex_right =
            (t16)(((draw->texture_x + draw->texture_width) << 4) + 8);
        t16 tex_top = (t16)((draw->texture_y << 4) + 8);
        t16 tex_bottom =
            (t16)(((draw->texture_y + draw->texture_height) << 4) + 8);
        /* Opaque A3 texels update depth on DS. Give each later source SObj a
         * one-step-nearer depth so frame, lamps, contour, and Light compose in
         * painter order instead of the first quad masking every successor. */
        v16 depth = (v16)(-4080 - (s32)draw_index);

        ndsRendererHardwareBindTextureState((int)draw->texture_name);
        ndsRendererHardwareSetPolyFmt(
            POLY_CULL_NONE | POLY_ALPHA(31) | POLY_ID(draw->poly_id));
        glBegin(GL_TRIANGLE);
#define NDS_IFCOMMON_CLOUD_VERTEX(s, t, x, y) do { \
    glTexCoord2t16((s), (t)); \
    glVertex3v16((x), (y), depth); \
} while (0)
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_top, left, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_bottom, left, bottom);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_top, right, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_top, right, top);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_left, tex_bottom, left, bottom);
        NDS_IFCOMMON_CLOUD_VERTEX(tex_right, tex_bottom, right, bottom);
#undef NDS_IFCOMMON_CLOUD_VERTEX
        gNdsRendererIFCommonCloudEmittedCount++;
    }
    sNdsRendererHardwareBoundTextureName =
        sNdsRendererIFCommonCloudQueue[
            sNdsRendererIFCommonCloudQueueCount - 1u].texture_name;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    sNdsRendererHardwareSubmitted = TRUE;
    sNdsRendererIFCommonCloudQueueCount = 0u;
}

static u32 ndsRendererHardwareTextureNextPow2(u32 value)
{
    u32 out = 8u;

    while ((out < value) && (out < NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        out <<= 1;
    }
    return out;
}

/* The RDP clamps a G_TX_CLAMP axis at the tile edge (SETTILESIZE lrs/lrt);
 * the DS clamps at the power-of-two upload edge. A tile narrower than its
 * upload in a clamped axis therefore needs its edge texel replicated across
 * the padding, or a coordinate just past the tile edge reads the zeroed
 * padding, which the PAL16 packer publishes as transparent colour 0.
 * Pikachu's ears are a 12x1 CI4 ramp uploaded as 16x8 and sampled at
 * t = 1.125 texels after the bilinear offset: source row 0 on the RDP, the
 * transparent padding row here, so both ears vanished. Wrapped axes are left
 * as they were (zero padding). */
static void ndsRendererHardwareReplicateClampPadding(
    u16 *texels, u32 width, u32 height,
    u32 upload_width, u32 upload_height, u32 cms, u32 cmt)
{
    u32 x;
    u32 y;

    if ((texels == NULL) || (width == 0u) || (height == 0u) ||
        (width > upload_width) || (height > upload_height))
    {
        return;
    }
    if ((width < upload_width) && ((cms & NDS_RENDERER_TX_CLAMP) != 0u))
    {
        for (y = 0u; y < height; y++)
        {
            u16 *row = &texels[y * upload_width];
            u16 edge = row[width - 1u];

            for (x = width; x < upload_width; x++)
            {
                row[x] = edge;
            }
        }
    }
    if ((height < upload_height) && ((cmt & NDS_RENDERER_TX_CLAMP) != 0u))
    {
        const u16 *edge_row = &texels[(height - 1u) * upload_width];

        for (y = height; y < upload_height; y++)
        {
            memcpy(&texels[y * upload_width], edge_row,
                   upload_width * sizeof(u16));
        }
    }
}

static s32 ndsRendererHardwareTextureMaskedClampNeedsWrap(
    u32 mode, u32 mask, u32 upload_extent, u32 tile_extent)
{
    u32 mask_extent;
    u32 sampler_extent;

    /* RDP mask repeat/mirror can occur before the logical tile clamp edge. */
    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (upload_extent == 0u) || (tile_extent == 0u))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    if ((upload_extent != mask_extent) || (tile_extent <= mask_extent))
    {
        return FALSE;
    }
    sampler_extent = upload_extent;
    if ((mode & NDS_RENDERER_TX_MIRROR) != 0u)
    {
        sampler_extent <<= 1;
    }
    return (((mode & NDS_RENDERER_TX_MIRROR) != 0u) ||
            (sampler_extent != tile_extent)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareTextureMaterializesMaskedClamp(
    u32 mode, u32 mask, u32 source_extent, u32 tile_extent)
{
    u32 mask_extent;

    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) || (mask == 0u) ||
        (mask >= 31u) || (source_extent == 0u) ||
        (tile_extent > NDS_RENDERER_HW_TEXTURE_MAX_WIDTH))
    {
        return FALSE;
    }
    mask_extent = 1u << mask;
    return ((tile_extent > mask_extent) &&
            (source_extent >= mask_extent) &&
            (source_extent <= tile_extent)) ? TRUE : FALSE;
}

/* A WRAPPING AXIS IS SAMPLED ONE MASK PERIOD WIDE, WHATEVER WAS LOADED.
 *
 * With G_TX_WRAP the RDP masks the coordinate and never clamps it, so only
 * the first 1 << mask texels of each TMEM line or column are ever read. The
 * extent the callers derive comes from the tile window or, when the window
 * outruns the load, from the TMEM line stride, and either can be wider than
 * that period. Peach's Castle's steep roof (StageCastleFile2 DL 0x1698) is a
 * CI4 tile at line 1 -- sixteen texels a row -- with masks 3 over an 80-texel
 * window: texels 8-15 of every row are index 0, whose LUT entry has alpha 0.
 * Uploading the whole line made the DS repeat every sixteen and sample that
 * padding, which read as a near-white field while texel alpha was forced
 * opaque and as a transparent roof once it was honoured (r40).
 *
 * Upload one period and the repeating sampler wraps where the RDP does. A
 * mirrored axis is the same period with GL_TEXTURE_FLIP. Eight is the smallest
 * DS texture edge, so shorter periods keep the old path. */
static u32 ndsRendererHardwareTextureWrapPeriodExtent(
    u32 mode, u32 mask, u32 extent)
{
    if (((mode & NDS_RENDERER_TX_CLAMP) == 0u) &&
        (mask >= 3u) && (mask < 31u) && ((1u << mask) < extent))
    {
        return 1u << mask;
    }
    return extent;
}

static u32 ndsRendererHardwareTextureMaskedAddress(
    u32 coord, u32 mode, u32 mask)
{
    u32 mask_extent = 1u << mask;
    u32 period = coord >> mask;
    u32 local = coord & (mask_extent - 1u);

    if (((mode & NDS_RENDERER_TX_MIRROR) != 0u) &&
        ((period & 1u) != 0u))
    {
        local = mask_extent - 1u - local;
    }
    return local;
}

static u32 ndsRendererHardwareTextureParams(
    const NDSRendererStats *stats,
    const NDSRendererTileState *render_tile,
    u32 upload_width,
    u32 upload_height)
{
    u32 params;
    s32 wrap_s;
    s32 wrap_t;

    if (render_tile == NULL)
    {
        return TEXGEN_OFF;
    }

    params = (ndsRendererHardwareUseTextureMatrix(stats) != FALSE) ?
        TEXGEN_TEXCOORD : TEXGEN_OFF;
    wrap_s = ((render_tile->cms & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cms, render_tile->masks, upload_width,
            render_tile->width);
    wrap_t = ((render_tile->cmt & NDS_RENDERER_TX_CLAMP) == 0u) ||
        ndsRendererHardwareTextureMaskedClampNeedsWrap(
            render_tile->cmt, render_tile->maskt, upload_height,
            render_tile->height);
    if (wrap_s != FALSE)
    {
        params |= GL_TEXTURE_WRAP_S;
    }
    if ((wrap_s != FALSE) &&
        ((render_tile->cms & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_S;
    }
    if (wrap_t != FALSE)
    {
        params |= GL_TEXTURE_WRAP_T;
    }
    if ((wrap_t != FALSE) &&
        ((render_tile->cmt & NDS_RENDERER_TX_MIRROR) != 0u))
    {
        params |= GL_TEXTURE_FLIP_T;
    }
    return params;
}

static u32 ndsRendererHardwareMergeTextureParams(u32 params)
{
    u32 current = (u32)glGetTexParameter();

    current &= ~NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    current |= params & NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK;
    return current;
}

/* Main RAM again (2026-09-28): the whole-match gate census ranks this and
 * ndsRendererHardwareBindTextureName at the bottom of ITCM rent (~1.6K and
 * ~2.9K cycles/byte), and the pose engine's whole-frame clock step needed
 * their 284 bytes. */
static void __attribute__((noinline)) ndsRendererHardwareApplyTextureParams(u32 params)
{
    if (((sNdsRendererGXStateShadow.valid_mask &
          NDS_RENDERER_GX_STATE_TEXTURE_PARAMS) != 0u) &&
        (sNdsRendererGXStateShadow.texture_params == params))
    {
        return;
    }
    glTexParameter(GL_TEXTURE_2D, (int)params);
    sNdsRendererGXStateShadow.texture_params = params;
    sNdsRendererGXStateShadow.valid_mask |=
        NDS_RENDERER_GX_STATE_TEXTURE_PARAMS;
}

#define NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT 128u

typedef struct NDSRendererStageTextureSite
{
    const Gfx *site;
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 state_hash1;
    u32 state_hash2;
    u32 entry_generation;
    u32 format;
    u32 size;
    u32 width;
    u32 height;
    u32 uses_texel1;
    u32 prim_lod_fraction;
} NDSRendererStageTextureSite;

static NDSRendererStageTextureSite
    sNdsRendererStageTextureSites[NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT];
static u32 sNdsRendererStageTextureSiteNext;

_Static_assert(
    sizeof(sNdsRendererStageTextureSites) <= (12u * 1024u),
    "stage texture-site plans must stay below 12 KiB");

static void __attribute__((noinline)) ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name);

static u32 ndsRendererStageTextureSiteSlot(const Gfx *site)
{
    u32 value = (u32)(uintptr_t)site;

    value ^= value >> 11;
    value ^= value >> 19;
    return value & (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
}

static NDSRendererStageTextureSite *ndsRendererStageTextureSiteFind(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats)
{
    u32 slot;
    u32 probe;

    if ((state == NULL) || (stats == NULL) ||
        (state->source_command_site == NULL))
    {
        return NULL;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *plan =
            &sNdsRendererStageTextureSites[slot];

        if (plan->site == NULL)
        {
            return NULL;
        }
        if ((plan->site == state->source_command_site) &&
            (plan->state_hash1 == stats->texture_source_hash1) &&
            (plan->state_hash2 == stats->texture_source_hash2))
        {
            return plan;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    return NULL;
}

static s32 ndsRendererStageTextureSiteTryBind(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    const NDSRendererConfig *config)
{
    NDSRendererStageTextureSite *plan;
    NDSRendererHardwareTextureCacheEntry *entry;

    if (sNdsRendererStageTextureSitesEnabled == 0u)
    {
        return FALSE;
    }
    gNdsG1SiteConsults++;
    plan = ndsRendererStageTextureSiteFind(state, stats);
    if (plan == NULL)
    {
        return FALSE;
    }
    entry = plan->entry;
    if ((entry == NULL) || (entry->ready == 0u) ||
        (entry->key_generation != plan->entry_generation) ||
        ((plan->uses_texel1 != 0u) &&
         (plan->prim_lod_fraction != stats->prim_lod_fraction)))
    {
        return FALSE;
    }

    entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
    if (entry->pinned != 0u)
    {
        ndsRendererHardwareRecordBattleStaticTextureHit(entry);
    }
    if (sNdsRendererHardwareActiveTextureEntry != entry)
    {
        ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
        ndsRendererHardwareApplyTextureParams(entry->params);
        sNdsRendererHardwareActiveTextureEntry = entry;
    }
    gNdsG1SiteHits++;
    stats->hardware_texture_ready_count++;
    stats->hardware_texture_format = plan->format;
    stats->hardware_texture_width = plan->width;
    stats->hardware_texture_height = plan->height;
    return TRUE;
}

static void ndsRendererStageTextureSiteRemember(
    const NDSRendererTraversalState *state,
    const NDSRendererStats *stats,
    NDSRendererHardwareTextureCacheEntry *entry,
    u32 format,
    u32 size,
    u32 width,
    u32 height)
{
    NDSRendererStageTextureSite *plan = NULL;
    NDSRendererStageTextureSite *empty = NULL;
    u32 slot;
    u32 probe;

    if ((sNdsRendererStageTextureSitesEnabled == 0u) ||
        (state == NULL) || (state->source_command_site == NULL) ||
        (stats == NULL) || (entry == NULL) || (entry->ready == 0u))
    {
        return;
    }
    slot = ndsRendererStageTextureSiteSlot(state->source_command_site);
    for (probe = 0u; probe < NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT; probe++)
    {
        NDSRendererStageTextureSite *candidate =
            &sNdsRendererStageTextureSites[slot];

        if (candidate->site == state->source_command_site)
        {
            plan = candidate;
            break;
        }
        if ((empty == NULL) && (candidate->site == NULL))
        {
            empty = candidate;
        }
        slot = (slot + 1u) &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u);
    }
    if (plan == NULL)
    {
        plan = empty;
        if (plan != NULL)
        {
            gNdsG1SiteOccupancy++;
        }
    }
    if (plan == NULL)
    {
        plan = &sNdsRendererStageTextureSites[
            sNdsRendererStageTextureSiteNext++ &
            (NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT - 1u)];
        /* Capacity thrash: the table is full and this slot belongs to a
         * DIFFERENT site, so it is being taken from it. Near zero after warm-up
         * means the working set fits in 128 entries; a large count means this
         * memo is repeating the refuted one's failure with a different key. */
        if ((plan->site != NULL) &&
            (plan->site != state->source_command_site))
        {
            gNdsG1SiteOverwrites++;
        }
    }
    gNdsG1SiteRemembers++;
    plan->site = state->source_command_site;
    plan->entry = entry;
    plan->state_hash1 = stats->texture_source_hash1;
    plan->state_hash2 = stats->texture_source_hash2;
    plan->entry_generation = entry->key_generation;
    plan->format = format;
    plan->size = size;
    plan->width = width;
    plan->height = height;
    plan->uses_texel1 = (ndsRendererHardwareEntryKeyWord(
                             entry,
                             NDS_RENDERER_HW_TEXTURE_KEY_WORD(texel1_image)) !=
                         0u) ? TRUE : FALSE;
    plan->prim_lod_fraction = ndsRendererHardwareEntryKeyWord(
        entry, NDS_RENDERER_HW_TEXTURE_KEY_WORD(prim_lod_fraction));
}

static void ndsRendererTextureSourceHashCommand(
    NDSRendererStats *stats,
    u32 w0,
    u32 w1)
{
    u32 hash1;
    u32 hash2;

    if ((stats == NULL) ||
        (sNdsRendererRuntimeOwner != NDS_RENDERER_PROFILE_OWNER_STAGE))
    {
        return;
    }

    hash1 = stats->texture_source_hash1;
    hash1 ^= w0 ^ ((w1 << 16) | (w1 >> 16));
    hash1 *= 16777619u;
    stats->texture_source_hash1 = hash1;

    hash2 = stats->texture_source_hash2 + w1 + 0x9e3779b9u;
    hash2 = (hash2 << 7) | (hash2 >> 25);
    stats->texture_source_hash2 = hash2 ^ w0 ^ 0x85ebca6bu;
}

static u32 ndsRendererHardwareAlphaStateKey(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return 0u;
    }
    return (stats->othermode_l & NDS_RENDERER_ALPHA_COMPARE_MASK) |
           ((stats->blend_color & 0xffu) << 8);
}

static u32 ndsRendererHardwareFogStateKey(const NDSRendererStats *stats)
{
    if ((stats == NULL) ||
        (stats->fog_status == 0u) ||
        (stats->fog_max <= stats->fog_min))
    {
        return 0u;
    }
    return ((u32)stats->fog_min & 0x3ffu) |
           (((u32)stats->fog_max & 0x3ffu) << 10) |
           ((stats->fog_color & 0xfffu) << 20);
}

static void __attribute__((noinline)) ndsRendererHardwareBindTextureName(
    NDSRendererStats *stats,
    u32 texture_name)
{
    if (texture_name == 0u)
    {
        return;
    }
    if (sNdsRendererHardwareBoundTextureName != texture_name)
    {
        ndsRendererHardwareEndBatch();
        ndsRendererHardwareBindTextureState((int)texture_name);
        sNdsRendererHardwareBoundTextureName = texture_name;
        ndsRendererProfileRecordTextureBind();
        if (stats != NULL)
        {
            stats->hardware_texture_bind_count++;
        }
    }
}

/* Bind a GL name the texture cache does not own -- a fighter tint tile --
 * without leaving the cache's bookkeeping stale.
 *
 * The cache elides a bind when sNdsRendererHardwareActiveTextureEntry already
 * equals the entry it wants (ResolveResidentTexture, the stage-site plan, the
 * run-texture memo). A bare ndsRendererHardwareBindTextureName for a foreign
 * name moves the bound-name tracker but not that pointer, so the next run
 * asking for the previously active entry would skip its bind and its params
 * and sample the foreign texels. The impact-wave, rebirth-halo and entry
 * effect owners clear the pointer inline beside each of their binds; this is
 * that pair, named, so check-fighter-draw-global-texture-state.py can admit it
 * as a seam of the fighter walk. The params shadow needs nothing:
 * BindTextureState invalidates it. */
static inline void ndsRendererHardwareBindForeignTextureName(
    NDSRendererStats *stats,
    u32 texture_name)
{
    ndsRendererHardwareBindTextureName(stats, texture_name);
    sNdsRendererHardwareActiveTextureEntry = NULL;
}

NDS_RENDERER_COLD static void ndsRendererHardwareReleaseBattleStaticTextureEntries(void)
{
    u32 i;

    for (i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i++)
    {
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[i];

        if (entry->pinned != 0u)
        {
            (void)ndsRendererHardwareReleaseTexture(entry);
        }
    }
}

/* Materialise a generated static key against the CURRENT reloc bases.
 *
 * Generated records deliberately store source asset IDs + offsets rather than
 * taskman-heap addresses.  The initial preload used to be the only place that
 * converted those offsets to pointers, which quietly made a pinned texture's
 * identity depend on the reloc file never moving afterward. Dream Land does
 * replace stage files during scene construction, however: the converted DS
 * payload remains perfectly valid in VRAM while the same source O2R asset is
 * reloaded at a new heap address. Keep pointer construction in one helper so
 * initial preload and mutation refresh obey exactly the same bounds contract. */
static const void *ndsRendererHardwareBattleStaticPointer(
    const void *base, u32 bytes, u32 source_offset)
{
    const void *pointer;
    uintptr_t address;

#if (NDS_P2_1P_GAME || NDS_P2_MENU_SHELL) || NDS_P2_COMPACT_BATTLE_FIGHTERS
    pointer = ndsRelocNativeAssetAddress(base, source_offset);
#else
    if ((source_offset >= bytes) ||
        ((uintptr_t)base > (uintptr_t)(0xffffffffu - source_offset)))
    {
        return NULL;
    }
    pointer = (const u8 *)base + source_offset;
#endif
    address = (uintptr_t)pointer;
    /* This builds identity only; upload reads the offline DS payload. Validate
     * the mapped byte in the current physical view, not a source offset against
     * compact byte length. The reloc mapper rejects omitted source spans. */
    return ((pointer != NULL) && (address <= 0xffffffffu) &&
            (address >= (uintptr_t)base) &&
            ((address - (uintptr_t)base) < bytes)) ? pointer : NULL;
}

NDS_RENDERER_COLD static s32 ndsRendererHardwareBuildBattleStaticTextureKey(
    const NDSBattlePlayableStaticTextureRecord *record,
    NDSRendererHardwareTextureKey *key)
{
    const void *image_base;
    const void *tlut_base;
    const void *image;
    const void *tlut = NULL;
    const void *texel1 = NULL;
    u32 image_size;
    u32 tlut_size = 0u;
    u32 texel1_offset;

    if ((record == NULL) || (key == NULL) ||
        (ndsRelocGetLoadedAssetView(
             record->image_asset_id, &image_base, &image_size) == FALSE) ||
        (record->key_words[
             NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD] !=
         record->image_offset) ||
        (record->key_words[
             NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD] !=
         record->tlut_offset))
    {
        return FALSE;
    }
    image = ndsRendererHardwareBattleStaticPointer(
        image_base, image_size, record->image_offset);
    if (image == NULL)
    {
        return FALSE;
    }
    tlut_base = NULL;
    if (record->tlut_asset_id != 0u)
    {
        if (ndsRelocGetLoadedAssetView(
                record->tlut_asset_id, &tlut_base, &tlut_size) == FALSE)
        {
            return FALSE;
        }
        tlut = ndsRendererHardwareBattleStaticPointer(
            tlut_base, tlut_size, record->tlut_offset);
        if (tlut == NULL)
        {
            return FALSE;
        }
    }
    else if (record->tlut_offset != 0u)
    {
        return FALSE;
    }
    texel1_offset = record->key_words[
        NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD];
    if (texel1_offset != 0u)
    {
        texel1 = ndsRendererHardwareBattleStaticPointer(
            image_base, image_size, texel1_offset);
        if (texel1 == NULL)
        {
            return FALSE;
        }
    }

    memcpy(key, record->key_words, sizeof(*key));
    key->image = (u32)(uintptr_t)image;
    key->tlut_image = (u32)(uintptr_t)tlut;
    key->texel1_image = (u32)(uintptr_t)texel1;
    return ((key->width == record->logical_width) &&
            (key->height == record->logical_height)) ? TRUE : FALSE;
}

NDS_RENDERER_COLD s32 ndsRendererHardwareRefreshBattleStaticTexturePointers(void)
{
    u32 record_index;
    u32 refreshed = 0u;

    if (sNdsRendererBattleStaticTexturePrepared == 0u)
    {
        return TRUE;
    }
    for (record_index = 0u;
         record_index < ndsBattlePlayableStaticTextureKeyCount();
         record_index++)
    {
        const NDSBattlePlayableStaticTextureRecord *record =
            ndsBattlePlayableStaticTextureRecordAt(record_index);
        NDSRendererHardwareTextureCacheEntry *entry =
            &sNdsRendererHardwareTextureCache[record_index];
        NDSRendererHardwareTextureKey key;
        u32 slot = ndsRendererHardwareEntrySlot(entry);
        u32 *pointers;
        u32 key_hash;

        if (record == NULL)
        {
            return FALSE;
        }
        /* Same applicability rule as the prepare above, and it has to be the
         * same or this function contradicts it: a record whose fighter is not
         * in this match was never uploaded, so its slot is not ready and its
         * key cannot be built. Walking it as if it were resident is what made
         * the whole stage admission decline with reject reason 3 after the
         * prepare itself started succeeding. */
        {
            const void *applicable_base;
            u32 applicable_size;

            if ((ndsRelocGetLoadedAssetView(record->image_asset_id,
                                            &applicable_base,
                                            &applicable_size) == FALSE) ||
                ((record->tlut_asset_id != 0u) &&
                 (ndsRelocGetLoadedAssetView(record->tlut_asset_id,
                                             &applicable_base,
                                             &applicable_size) == FALSE)))
            {
                continue;
            }
        }
        if ((slot != record_index) ||
            (entry->ready == 0u) || (entry->pinned == 0u) ||
            (entry->static_record_plus1 != record_index + 1u) ||
            (ndsRendererHardwareBuildBattleStaticTextureKey(record, &key) ==
             FALSE))
        {
            return FALSE;
        }
        pointers = sNdsRendererHardwareStaticKeyPointers[slot];
        if ((pointers[0] == key.image) &&
            (pointers[1] == key.tlut_image) &&
            (pointers[2] == key.texel1_image))
        {
            continue;
        }

        /* Remove under the OLD pointer-derived hash before changing identity. */
        ndsRendererHardwareTextureLookupRemove(entry);
        key_hash = ndsRendererHardwareTextureKeyHash(&key);
        ndsRendererHardwareEntrySetStaticKey(entry, &key);
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
        entry->key_hash = key_hash;
        ndsRendererHardwareTextureLookupInsert(entry);
        refreshed++;
    }
    if (refreshed != 0u)
    {
        /* Any caller retaining the old entry identity must revalidate through
         * key_generation; the active bind has the same rule but clearing this
         * pointer also prevents a stale same-entry fast path in this frame. */
        sNdsRendererHardwareActiveTextureEntry = NULL;
        NDS_DIAG(gNdsRendererBattleStaticTextureRefreshCount++);
        NDS_DIAG(gNdsRendererBattleStaticTextureRefreshedEntryCount += refreshed);
    }
    return TRUE;
}

NDS_RENDERER_COLD s32 ndsRendererHardwarePrepareBattleStaticTextures(void)
{
    FILE *file = NULL;
    long payload_size;
    u32 record_index;
    /* Payload bytes belonging to records this match cannot key, so the
     * extent check below compares against what was APPLICABLE rather than
     * against the pack's full declared total. */
    u32 skipped_payload_bytes = 0u;

    if (gNdsRendererBattleStaticTextureEnabled == 0u)
    {
        return FALSE;
    }
    if (sNdsRendererBattleStaticTexturePrepared != 0u)
    {
        return TRUE;
    }

    gNdsRendererBattleStaticTexturePrepareCount++;
    gNdsRendererBattleStaticTexturePrepareFailCount = 0u;
    gNdsRendererBattleStaticTexturePreparedCount = 0u;
    gNdsRendererBattleStaticTexturePreparedBytes = 0u;
    gNdsRendererBattleStaticTextureArmCount = 0u;
    gNdsRendererBattleStaticTexturePinnedHitCount = 0u;
    gNdsRendererBattleStaticTextureSeenMask = 0u;
    gNdsRendererBattleStaticTextureSeenMaskHi = 0u;
    gNdsRendererBattleStaticTextureOwnerMask = 0u;
    gNdsRendererBattleStaticTextureViolationCount = 0u;
    gNdsRendererBattleStaticTextureSkippedCount = 0u;
    gNdsRendererBattleStaticTextureFailStep = 0u;
    gNdsRendererBattleStaticTextureTeardownCount = 0u;
    gNdsRendererBattleStaticTextureFirstAddress = 0u;
    gNdsRendererBattleStaticTextureEndAddress = 0u;
    gNdsRendererBattleStaticTextureAllocationSpanBytes = 0u;
    gNdsRendererBattleStaticTextureBankMask = 0u;
    /* Reset with the family they belong to, so a per-scene reading is a per-
     * scene reading. The first-reject latch in particular is only meaningful
     * against the scene it was armed in. */
    gNdsRendererStageOwnerRejectCount = 0u;
    gNdsRendererStageOwnerLastRejectReason = 0u;
    gNdsRendererStageOwnerFirstRejectReason = 0u;
    gNdsRendererStageOwnerAbortCount = 0u;
    gNdsRendererStageOwnerPostArmRejectCount = 0u;
    gNdsRendererStaticTexturePreparedNow = 0u;
    sNdsRendererBattleStaticTextureArmed = FALSE;

    /* A battle owns the cache from this point forward. Starting empty makes
     * the generated residency and its exact VRAM span deterministic. */
    ndsRendererHardwareDiscardTextureCache();
    file = ndsRendererHardwareFencedTextureFopen(
        NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_PAYLOAD_PATH, "rb");
    if (file == NULL)
    {
        gNdsRendererBattleStaticTextureFailStep = 1u;
        goto fail;
    }
    if ((ndsRendererHardwareFencedTextureFseek(file, 0, SEEK_END) != 0) ||
        ((payload_size = ndsRendererHardwareFencedTextureFtell(file)) < 0) ||
        ((u32)payload_size !=
         ndsBattlePlayableStaticTexturePayloadBytes()) ||
        (ndsRendererHardwareFencedTextureFseek(file, 0, SEEK_SET) != 0))
    {
        gNdsRendererBattleStaticTextureFailStep = 2u;
        goto fail;
    }
    /* ONE read for every palette, before the record loop. See the note beside
     * sNdsRendererStaticTexturePaletteBlock: doing this per record put 22 extra
     * NitroFS seeks and reads inside the scene load. */
    {
        u32 palette_block_bytes =
            ndsBattlePlayableStaticTexturePaletteBlockBytes();

        if ((palette_block_bytes >
             sizeof(sNdsRendererStaticTexturePaletteBlock)) ||
            (ndsRendererHardwareFencedTextureFseek(
                 file,
                 (long)ndsBattlePlayableStaticTexturePaletteBlockOffset(),
                 SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererStaticTexturePaletteBlock, 1,
                 palette_block_bytes, file) != palette_block_bytes))
        {
            gNdsRendererBattleStaticTextureFailStep = 3u;
            goto fail;
        }
    }

    for (record_index = 0u;
         record_index < ndsBattlePlayableStaticTextureKeyCount();
         record_index++)
    {
        const NDSBattlePlayableStaticTextureRecord *record =
            ndsBattlePlayableStaticTextureRecordAt(record_index);
        NDSRendererHardwareTextureKey key;
        NDSRendererHardwareTextureCacheEntry *entry;
        u32 key_hash;
        int size_x;
        int size_y;

        if (record == NULL)
        {
            gNdsRendererBattleStaticTextureFailStep = 12u;
            goto fail;
        }
        /* A RECORD FOR A FIGHTER THIS MATCH DOES NOT HAVE IS NOT AN ERROR.
         * The corpus is the P1 pin set, so it carries Fox's images as well
         * as Mario's and the stage's, and the key builder resolves each one
         * through ndsRelocGetLoadedAssetView. In a Mario mirror there is no
         * Fox asset, the key build returned FALSE, and the whole pin set
         * aborted -- so Dream Land's own textures were never pinned either,
         * the stage's texture resolve declined every run, and the match
         * recorded 20,160 native stage failures in 480 presents while the
         * Mario/Fox pair recorded none. Skip the inapplicable record and
         * count it; every other decline below still fails closed. */
        {
            const void *applicable_base;
            u32 applicable_size;

            if ((ndsRelocGetLoadedAssetView(record->image_asset_id,
                                            &applicable_base,
                                            &applicable_size) == FALSE) ||
                ((record->tlut_asset_id != 0u) &&
                 (ndsRelocGetLoadedAssetView(record->tlut_asset_id,
                                             &applicable_base,
                                             &applicable_size) == FALSE)))
            {
                gNdsRendererBattleStaticTextureSkippedCount++;
                skipped_payload_bytes += record->payload_bytes;
                continue;
            }
        }
        NDS_RENDERER_CAPACITY_HIGH_WATER(
            gNdsRendererTextureScratchStaticPayloadBytesHighWater,
            record->payload_bytes);
        if ((record->reserved != 0u) ||
            (record->payload_bytes == 0u) ||
            (record->payload_bytes >
             sizeof(sNdsRendererHardwareTextureScratch)) ||
            (record->payload_offset >
             ndsBattlePlayableStaticTexturePayloadBytes()) ||
            (record->payload_bytes >
             ndsBattlePlayableStaticTexturePayloadBytes() -
                 record->payload_offset) ||
            (record->payload_bytes !=
             ndsRendererStaticTextureSpanBytes(record)) ||
            (ndsRendererStaticTextureSpanBytes(record) == 0u) ||
            (ndsRendererHardwareTextureSizeEnum(
                 record->upload_width, &size_x) == FALSE) ||
            (ndsRendererHardwareTextureSizeEnum(
                 record->upload_height, &size_y) == FALSE) ||
            (ndsRendererHardwareBuildBattleStaticTextureKey(record, &key) ==
             FALSE))
        {
            gNdsRendererBattleStaticTextureFailStep = 4u;
            goto fail;
        }

        key_hash = ndsRendererHardwareTextureKeyHash(&key);
        if (ndsRendererHardwareFindTexture(&key, key_hash) != NULL)
        {
            gNdsRendererBattleStaticTextureFailStep = 5u;
            goto fail;
        }
        if ((ndsRendererHardwareFencedTextureFseek(
                 file, (long)record->payload_offset, SEEK_SET) != 0) ||
            (ndsRendererHardwareFencedTextureFread(
                 sNdsRendererHardwareTextureScratch, 1,
                 record->payload_bytes, file) != record->payload_bytes))
        {
            gNdsRendererBattleStaticTextureFailStep = 6u;
            goto fail;
        }
        if ((record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16) ||
            (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL256))
        {
            u32 palette_bytes = (u32)record->palette_entries * sizeof(u16);

            u32 block_index;

            if ((record->palette_entries == 0u) ||
                (record->palette_entries >
                 (u16)(sizeof(sNdsRendererStaticTexturePalette) /
                       sizeof(sNdsRendererStaticTexturePalette[0]))) ||
                (record->palette_offset <
                 ndsBattlePlayableStaticTexturePaletteBlockOffset()))
            {
                gNdsRendererBattleStaticTextureFailStep = 7u;
                goto fail;
            }
            block_index = record->palette_offset -
                ndsBattlePlayableStaticTexturePaletteBlockOffset();
            if ((block_index >
                 ndsBattlePlayableStaticTexturePaletteBlockBytes()) ||
                (palette_bytes >
                 ndsBattlePlayableStaticTexturePaletteBlockBytes() -
                     block_index))
            {
                gNdsRendererBattleStaticTextureFailStep = 8u;
                goto fail;
            }
            memcpy(sNdsRendererStaticTexturePalette,
                   &sNdsRendererStaticTexturePaletteBlock[block_index],
                   palette_bytes);
        }

        /* THE SLOT IS THE RECORD INDEX. That identity is what lets a static
         * entry drop its resident key: reading the other 56 words back out of
         * ROM needs the record, and reading the three pointer words needs
         * sNdsRendererHardwareStaticKeyPointers, and both are addressed by this
         * one number. It also makes the corpus immune to the runtime allocator,
         * which no longer touches slots below STATIC_COUNT at all. */
        if (record_index >= NDS_RENDERER_HW_TEXTURE_STATIC_COUNT)
        {
            gNdsRendererBattleStaticTextureFailStep = 9u;
            goto fail;
        }
        entry = &sNdsRendererHardwareTextureCache[record_index];
        if ((entry->ready != 0u) || (entry->name != 0))
        {
            entry = ndsRendererHardwareReleaseTexture(entry);
        }
        ndsRendererHardwareRecordBattleTextureFence(
            NDS_RENDERER_BATTLE_TEXTURE_FENCE_ALLOC);
        if ((entry->name == 0) &&
            (ndsRendererHardwareFencedGlGenTextures(
                 1, &entry->name) == 0))
        {
            gNdsRendererBattleStaticTextureFailStep = 10u;
            goto fail;
        }
        ndsRendererHardwareBindTextureName(NULL, (u32)entry->name);
        {
            /* Colour 0 is the transparent one ONLY when the generator put a
             * transparent colour there; an image with no transparent texel
             * uses all sixteen entries and index 0 is real art. Reading the
             * palette rather than carrying a flag keeps the two in step. */
            int params = TEXGEN_TEXCOORD;
            GL_TEXTURE_TYPE_ENUM type = GL_RGBA;

            if ((record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16) ||
                (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL256))
            {
                type = (record->ds_format ==
                        NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16) ?
                    GL_RGB16 : GL_RGB256;
                if (sNdsRendererStaticTexturePalette[0] == 0u)
                {
                    params |= GL_TEXTURE_COLOR0_TRANSPARENT;
                }
            }
            if (ndsRendererHardwareFencedGlTexImage2D(
                    GL_TEXTURE_2D, 0, type, size_x, size_y, 0,
                    params,
                    sNdsRendererHardwareTextureScratch) == 0)
            {
                (void)ndsRendererHardwareReleaseTexture(entry);
                gNdsRendererBattleStaticTextureFailStep = 11u;
                goto fail;
            }
            if ((record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16) ||
                (record->ds_format == NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL256))
            {
                glColorTableEXT(GL_TEXTURE_2D, 0,
                                (int)record->palette_entries, 0, 0,
                                sNdsRendererStaticTexturePalette);
            }
        }
        {
            uintptr_t first = (uintptr_t)glGetTexturePointer(entry->name);
            uintptr_t end = first + record->payload_bytes;

            if ((first == 0u) || (end <= first) ||
                (end > (uintptr_t)0xffffffffu))
            {
                (void)ndsRendererHardwareReleaseTexture(entry);
                gNdsRendererBattleStaticTextureFailStep = 13u;
                goto fail;
            }
            if ((gNdsRendererBattleStaticTextureFirstAddress == 0u) ||
                (first < gNdsRendererBattleStaticTextureFirstAddress))
            {
                gNdsRendererBattleStaticTextureFirstAddress = (u32)first;
            }
            if (end > gNdsRendererBattleStaticTextureEndAddress)
            {
                gNdsRendererBattleStaticTextureEndAddress = (u32)end;
            }
            if ((first < (uintptr_t)VRAM_B) &&
                (end > (uintptr_t)VRAM_A))
            {
                gNdsRendererBattleStaticTextureBankMask |= 1u << 0;
            }
            if ((first < (uintptr_t)VRAM_C) &&
                (end > (uintptr_t)VRAM_B))
            {
                gNdsRendererBattleStaticTextureBankMask |= 1u << 1;
            }
        }

        ndsRendererHardwareEntrySetStaticKey(entry, &key);
        sNdsRendererHardwareTextureKeyGeneration++;
        if (sNdsRendererHardwareTextureKeyGeneration == 0u)
        {
            sNdsRendererHardwareTextureKeyGeneration++;
        }
        entry->key_generation = sNdsRendererHardwareTextureKeyGeneration;
        entry->key_hash = key_hash;
        entry->params = (u32)glGetTexParameter();
        entry->last_used_frame = 0u;
        entry->pinned = TRUE;
        entry->static_record_plus1 = record_index + 1u;
        entry->static_owner_mask = record->owner_mask;
        entry->ready = TRUE;
        ndsRendererHardwareTextureLookupInsert(entry);
        gNdsRendererBattleStaticTexturePreparedCount++;
        gNdsRendererBattleStaticTexturePreparedBytes +=
            record->payload_bytes;
    }

    if (ndsRendererHardwareFencedTextureFclose(file) != 0)
    {
        file = NULL;
        gNdsRendererBattleStaticTextureFailStep = 14u;
        goto fail;
    }
    file = NULL;
    /* THE EXPECTED BANK MASK IS DERIVED, AND PINNING IT AT 3 COST A CYCLE.
     *
     * The span starts at VRAM_A and runs preparedBytes, so which banks it
     * covers is arithmetic: always A, and B only once it passes A's 128 KiB.
     * The literal 3 was true of one corpus size and read as a correctness
     * check. On 2026-08-03 the corpus was repacked to DS paletted -- lossless,
     * same pixels, 136,192 -> 61,696 bytes -- and the whole residency lifecycle
     * failed with actual=(212,1,1,1,0,0): the prepare reached here with a
     * perfectly good 24-texture span that no longer straddled the boundary, and
     * this line rejected it. The renderer then fell back to ordinary texture
     * resolution, which still LOOKS right and still runs at speed, so the
     * failure was invisible outside the verifier.
     *
     * That mis-attribution also poisoned the atlas measurement it was running
     * beside: a stage whose static textures never became resident renders
     * untextured, which is exactly the symptom the 32,768-byte sheet was
     * blamed for. Derive the mask, and a size change stops being a failure. */
    {
        /* Applicable, not declared. Every record this match could key was
         * uploaded contiguously from VRAM_A, so the span is still exact --
         * it is simply shorter by the records whose fighter is absent. */
        u32 applicable_bytes =
            ndsBattlePlayableStaticTexturePreparedBytes() -
            skipped_payload_bytes;
        u32 applicable_count =
            ndsBattlePlayableStaticTextureKeyCount() -
            gNdsRendererBattleStaticTextureSkippedCount;
        u32 expected_bank_mask =
            (applicable_bytes > ((u32)VRAM_B - (u32)VRAM_A)) ? 3u : 1u;

        if ((gNdsRendererBattleStaticTexturePreparedCount !=
             applicable_count) ||
            (gNdsRendererBattleStaticTexturePreparedBytes !=
             applicable_bytes) ||
            ((gNdsRendererBattleStaticTextureAllocationSpanBytes =
              gNdsRendererBattleStaticTextureEndAddress -
              gNdsRendererBattleStaticTextureFirstAddress) !=
             applicable_bytes) ||
            (gNdsRendererBattleStaticTextureFirstAddress != (u32)VRAM_A) ||
            (gNdsRendererBattleStaticTextureEndAddress !=
             ((u32)VRAM_A + applicable_bytes)) ||
            (gNdsRendererBattleStaticTextureBankMask != expected_bank_mask))
        {
            gNdsRendererBattleStaticTextureFailStep = 15u;
            goto fail;
        }
    }
    sNdsRendererBattleStaticTexturePrepared = TRUE;
    gNdsRendererStaticTexturePreparedNow = 1u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    return TRUE;

fail:
    if (file != NULL)
    {
        (void)ndsRendererHardwareFencedTextureFclose(file);
    }
    ndsRendererHardwareReleaseBattleStaticTextureEntries();
    sNdsRendererBattleStaticTexturePrepared = FALSE;
    sNdsRendererBattleStaticTextureArmed = FALSE;
    gNdsRendererBattleStaticTexturePreparedCount = 0u;
    gNdsRendererBattleStaticTexturePreparedBytes = 0u;
    gNdsRendererBattleStaticTextureFirstAddress = 0u;
    gNdsRendererBattleStaticTextureEndAddress = 0u;
    gNdsRendererBattleStaticTextureAllocationSpanBytes = 0u;
    gNdsRendererBattleStaticTextureBankMask = 0u;
    gNdsRendererBattleStaticTexturePrepareFailCount++;
    return FALSE;
}
