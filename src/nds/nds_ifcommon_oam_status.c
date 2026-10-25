/* IFCommon on OAM, the game status: the countdown and end assets' metadata,
 * the effects arena, preparing the native OAM, the announcements, the end
 * bake and the asset behind each SObj. A slice of nds_ifcommon_oam.c, which
 * includes it at its end. */

/* Indexed entries own no OBJ bytes: every one of them renders through the
 * retained traffic/cloud GX atlases. This binds sprite/bitmap/size metadata
 * for all 25 assets so traffic/cloud/GO/end matching keeps working, without
 * allocating or baking indexed OBJ cells. End letters additionally check
 * the exact source RGBA32 extents. */
static s32 ndsIFCommonBindGameStatusMetadata(const void *file_data,
                                             size_t file_size)
{
    u32 asset_index;
    u32 letter;

    memset(sNdsIFCommonAssets, 0, sizeof(sNdsIFCommonAssets));
    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        const NDSIFCommonAssetSpec *spec =
            &sNdsIFCommonAssetSpecs[asset_index];
        NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];
        const Sprite *sprite = (const Sprite *)((const u8 *)file_data +
                                                spec->offset);

        if ((spec->offset + sizeof(*sprite) > file_size) ||
            (ndsIFCommonRangeValid(file_data, file_size, sprite->bitmap,
                                   (size_t)(u16)sprite->nbitmaps *
                                       sizeof(Bitmap)) == FALSE))
        {
            return FALSE;
        }
        asset->sprite = sprite;
        asset->bitmap = sprite->bitmap;
        asset->width = (u32)(u16)sprite->width;
        asset->height = (u32)(u16)sprite->height;
        asset->tile_count = spec->tile_count;
        asset->runtime_red = spec->red;
        asset->runtime_green = spec->green;
        asset->runtime_blue = spec->blue;
        asset->runtime_env_red = spec->env_red;
        asset->runtime_env_green = spec->env_green;
        asset->runtime_env_blue = spec->env_blue;
    }
    for (letter = 0u; letter < 9u; letter++)
    {
        const NDSIFCommonNativeAsset *asset =
            &sNdsIFCommonAssets[NDS_IFCOMMON_END_FIRST + letter];

        if ((asset->width != sNdsIFCommonEndLetters[letter].source_width) ||
            (asset->height != sNdsIFCommonEndLetters[letter].source_height) ||
            (asset->sprite->bmfmt != G_IM_FMT_RGBA) ||
            (asset->sprite->bmsiz != G_IM_SIZ_32b))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* Direct-only tile bake into a fixed VRAM base. GO keeps its existing
 * prefiltered pixels; end letters use the same 0.8 RGBA prefilter at the
 * final DS grid. No palette, no indexed path.
 *
 * `dest` is where the pixels go: OBJ VRAM itself, or (P2-2p8 A7) a RAM image
 * of an OBJ bank whose first halfword stands for VRAM offset `origin`, laid
 * out byte-for-byte as the VRAM bake would lay it out. `set_tiles` records
 * the tile layout the draw path reads; `fill` writes the pixels. The VRAM
 * bake is (SPRITE_GFX, 0, TRUE, TRUE) and stays exactly what it was. */
static s32 ndsIFCommonBakeDirectAssetTo(u32 asset_index,
                                        const void *file_data,
                                        size_t file_size, u32 vram_base,
                                        u16 *dest, u32 origin,
                                        u32 set_tiles, u32 fill)
{
    const NDSIFCommonAssetSpec *spec =
        &sNdsIFCommonAssetSpecs[asset_index];
    NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];
    const Sprite *sprite = asset->sprite;
    u32 to_vram = (dest == (u16 *)SPRITE_GFX) ? TRUE : FALSE;
    u32 tile_index;
    u32 cursor = vram_base;

    if ((sprite == NULL) || (spec->tile_count > NDS_IFCOMMON_MAX_TILES) ||
        (vram_base < origin))
    {
        return FALSE;
    }
    for (tile_index = 0u; tile_index < spec->tile_count; tile_index++)
    {
        const NDSIFCommonTileSpec *tile_spec = &spec->tiles[tile_index];
        NDSIFCommonNativeTile *tile = &asset->tiles[tile_index];
        u32 bytes = (u32)tile_spec->cell_width *
                    (u32)tile_spec->cell_height * sizeof(u16);
        u32 y;

        if (((tile_spec->source_x + tile_spec->content_width) >
             asset->width) ||
            ((tile_spec->source_y + tile_spec->content_height) >
             asset->height) ||
            ((tile_spec->pad_x + tile_spec->content_width) >
             tile_spec->cell_width) ||
            ((tile_spec->pad_y + tile_spec->content_height) >
             tile_spec->cell_height) ||
            ((cursor + bytes) > NDS_IFCOMMON_OBJ_VRAM_BYTES))
        {
            return FALSE;
        }
        if ((u32)ndsIFCommonSpriteSize(tile_spec->cell_width,
                                       tile_spec->cell_height) == 0u)
        {
            return FALSE;
        }
        if (set_tiles != FALSE)
        {
            tile->gfx = (u16 *)((u8 *)SPRITE_GFX + cursor);
            tile->size = ndsIFCommonSpriteSize(tile_spec->cell_width,
                                               tile_spec->cell_height);
            tile->color_format = SpriteColorFormat_Bmp;
            tile->spec = *tile_spec;
        }
        if (fill != FALSE)
        {
            u16 *pixels = (u16 *)((u8 *)dest + (cursor - origin));

            if (to_vram != FALSE)
            {
                dmaFillHalfWords(0u, pixels, bytes);
            }
            else
            {
                memset(pixels, 0, bytes);
            }
            for (y = 0u; y < tile_spec->content_height; y++)
            {
                u32 x;

                for (x = 0u; x < tile_spec->content_width; x++)
                {
                    u16 color = ndsIFCommonDecodePrefilteredGoPixel(
                            sprite, file_data, file_size,
                            (u32)tile_spec->source_x + x,
                            (u32)tile_spec->source_y + y);

                    pixels[(((u32)tile_spec->pad_y + y) *
                                tile_spec->cell_width) +
                           tile_spec->pad_x + x] = color;
                }
            }
        }
        cursor += bytes;
        gNdsIFCommonNativeOamPrepareTiles++;
    }
    gNdsIFCommonNativeOamPrepareAssets++;
    return TRUE;
}

static s32 ndsIFCommonBakeDirectAsset(u32 asset_index, const void *file_data,
                                      size_t file_size, u32 vram_base)
{
    return ndsIFCommonBakeDirectAssetTo(asset_index, file_data, file_size,
                                        vram_base, (u16 *)SPRITE_GFX, 0u,
                                        TRUE, TRUE);
}

static s32 ndsIFCommonEndAssetActive(u32 asset_index)
{
    const NDSIFCommonEndSlot *slots;
    u32 slot;

    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonAnnounceActive == FALSE) ||
        (asset_index < NDS_IFCOMMON_END_FIRST) ||
        (asset_index >= NDS_IFCOMMON_ASSET_COUNT))
    {
        return FALSE;
    }
    slots = (sNdsIFCommonAnnounceGameSet != FALSE) ?
        sNdsIFCommonGameSetSlots : sNdsIFCommonTimeUpSlots;
    for (slot = 0u; slot < 6u; slot++)
    {
        if (slots[slot].asset_index == asset_index)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void ndsFxEffectsEngage(u32 mask)
{
    gNdsFxEngagementMask |= mask;
}

static s32 ndsFxEffectsArenaValid(void)
{
    if ((gNdsFxArenaBootSize == 0u) &&
        (gNdsTaskmanArenaChosenSize != 0u))
    {
        gNdsFxArenaBootSize = gNdsTaskmanArenaChosenSize;
    }
    if ((gNdsFxArenaBootSize == 0u) ||
        (gNdsTaskmanArenaChosenSize != gNdsFxArenaBootSize))
    {
        gNdsFxArenaRejectCount = 1u;
        return FALSE;
    }
    return TRUE;
}

/* THIS WHOLE SPRITE HIT-SPARK PATH IS UNREACHABLE IN THE SHIPPING ROM, and the
 * clamp below is what proved it the expensive way.
 *
 * Its only external entry is the weak `efManagerDamageNormalLightMakeEffect` /
 * `...HeavyMakeEffect` in reloc_backend_compat_shims.c, and
 * `battleship_efmanager.c` provides STRONG definitions of both names under
 * NDS_R2_SOURCE_EFFECTS_PARTICLE, which defaults to 1 and is not overridden by
 * any target. The linker takes the strong ones. Disassembly of the published
 * ROM, 2026-08-03:
 *
 *   020941c8 <efManagerDamageNormalLightMakeEffect>:
 *     bl 209224c <ndsBaseEFManagerDamageNormalLightMakeEffect>
 *
 * -- straight to the source implementation, never here. `ndsFxHitSparkSpawn`
 * is reached only from this file's own heavy-spark death, which nothing starts,
 * so gNdsFxHitSparkSpawnCount is structurally 0.
 *
 * BUGS.md's *"orange ball ... looks too big"* row was answered here with the 2.2
 * ceiling below and the owner re-filed it unchanged, because the effect the
 * owner sees is drawn by the source particle path. The scale there is
 * source-exact and was re-derived twice: efmanager.c:2175 ramps light to 4.9x
 * at 40 damage and the heavy maker (efmanager.c:2197) sets no scale at all --
 * both exactly what the port does. What was wrong is the IMAGE: a 32x32 spark
 * box-averaged into a 16x16 atlas cell reads as a soft round blob, and
 * magnified 4.9x that is an orange ball. The cell is source resolution now.
 *
 * Left in place rather than deleted because NDS_TASK39_FX_SPRITES gates more
 * than this one path and a deletion is a separate, wider change. Do not tune
 * this constant again -- nothing reads it. */
#define NDS_FX_HIT_SPARK_SCALE_MAX 2.2F

volatile u32 gNdsFxHitSparkScaleClampCount;

void ndsFxHitSparkSpawn(const Vec3f *pos, s32 player, s32 size,
                            sb32 is_static, sb32 is_heavy)
{
    u32 index;
    NDSFxHitSpark *spark = NULL;

    if ((pos == NULL) || (sNdsFxHitSparkGfx == NULL) ||
        (ndsFxEffectsArenaValid() == FALSE))
    {
        NDS_DIAG(gNdsFxHitSparkDropCount++);
        return;
    }
    for (index = 0u; index < NDS_FX_HIT_SPARK_CAPACITY; index++)
    {
        if (sNdsFxHitSparks[index].active == FALSE)
        {
            spark = &sNdsFxHitSparks[index];
            break;
        }
    }
    if (spark == NULL)
    {
        NDS_DIAG(gNdsFxHitSparkDropCount++);
        return;
    }

    *spark = (NDSFxHitSpark){0};
    spark->pos = *pos;
    spark->player = (u8)((u32)player & 3u);
    spark->size = (s16)size;
    spark->is_heavy = (is_heavy != FALSE) ? TRUE : FALSE;
    spark->active = TRUE;
    if (spark->is_heavy != FALSE)
    {
        spark->scale = 1.0F;
    }
    else
    {
        f32 angle;
        f32 velocity = (is_static != FALSE) ? 0.0F :
            ((syUtilsRandFloat() * 38.0F) + 12.0F);

        angle = syUtilsRandFloat() * 6.28318530718F;
        spark->vel.x = __cosf(angle) * velocity;
        spark->vel.y = __sinf(angle) * velocity;
        spark->scale = (size < 10) ?
            (((10 - size) * -0.05F) + 1.0F) :
            (((size - 10) * 0.13F) + 1.0F);
        /* THE ORANGE BALL. `size` is damage and clamps at 40 upstream, so this
         * ramp reaches (40-10)*0.13 + 1.0 = 4.9x -- nearly five times the
         * sprite -- on a 256x192 screen. That is the owner's *"orange ball
         * visual effect that looks too big"* on side-A hits: a forward tilt is
         * normal-element and light, so it lands here, and HEAVY_ENV tints
         * player 1's spark RED (generate_fx_hit_sparks.py:30), which is the
         * orange.
         *
         * Note the heavy branch above is a flat 1.0, so before this the LIGHT
         * spark could draw five times the size of the HEAVY one -- the ramp was
         * never bounded, only the damage feeding it was.
         *
         * There is no source number to derive a ceiling from: the N64 draws
         * particles here, not sprites, so the sprite's size is a port choice
         * and the owner is its oracle. 2.2 keeps a big hit clearly bigger than
         * a small one while staying near the heavy spark's own scale. */
        if (spark->scale > NDS_FX_HIT_SPARK_SCALE_MAX)
        {
            spark->scale = NDS_FX_HIT_SPARK_SCALE_MAX;
            NDS_DIAG(gNdsFxHitSparkScaleClampCount++);
        }
    }
    gNdsFxHitSparkSpawnCount++;
    ndsFxEffectsEngage(NDS_FX_ENGAGED_SPRITES);
}

void ndsFxEffectsUpdate(void)
{
    u32 index;

    if (ndsFxEffectsArenaValid() == FALSE)
    {
        return;
    }
    for (index = 0u; index < NDS_FX_HIT_SPARK_CAPACITY; index++)
    {
        NDSFxHitSpark *spark = &sNdsFxHitSparks[index];
        u32 lifetime;

        if (spark->active == FALSE)
        {
            continue;
        }
        spark->pos.x += spark->vel.x;
        spark->pos.y += spark->vel.y;
        spark->age++;
        NDS_DIAG(gNdsFxHitSparkUpdateCount++);
        lifetime = (spark->is_heavy != FALSE) ?
            NDS_FX_HIT_SPARK_HEAVY_LIFETIME :
            NDS_FX_HIT_SPARK_LIGHT_LIFETIME;
        if (spark->age >= lifetime)
        {
            if (spark->is_heavy != FALSE)
            {
                Vec3f pos = spark->pos;
                s32 player = spark->player;
                s32 size = spark->size;

                spark->active = FALSE;
                ndsFxEffectCensusRecord(
                    NDS_FX_EF_MANAGER_DAMAGE_NORMAL_LIGHT_MAKE_EFFECT,
                    NDS_FX_SUBSTITUTE);
                ndsFxHitSparkSpawn(&pos, player, size, FALSE, FALSE);
            }
            else
            {
                spark->active = FALSE;
            }
        }
    }
}

/* Stream the hit-spark sheet from NitroFS straight into OBJ VRAM.
 *
 * It used to be a linked 22,528-byte array DMA'd in one shot. That array was
 * five whole steps of the boot-time taskman arena search (4,096 each), spent on
 * bytes that are read exactly once and then live in VRAM for the rest of the
 * run -- and it is what left NDS_R2_PARTICLE_RUNTIME=1 with no arena to boot
 * into. The generator was already writing the identical payload to
 * assets/effects/, so this reads that instead.
 *
 * One frame cell at a time, through a stack buffer, with explicit 32-bit
 * stores. Two DS-specific reasons, both of which have bitten this project:
 * DMA cannot source from DTCM, where libnds puts the stack, so dmaCopyWords
 * from a local is silently wrong; and VRAM rejects 8-bit writes, so a memcpy
 * whose tail degrades to byte stores would corrupt the sheet rather than fail.
 * A u32 loop is immune to both. 44 cells at load time costs nothing. */
void ndsFsLock(void);
void ndsFsUnlock(void);

static s32 ndsFxPrepareHitSparksUnlocked(u32 *vram_cursor)
{
    u32 bytes = NDS_FX_HIT_SPARK_ASSET_BYTES;
    u32 cell[NDS_FX_HIT_SPARK_CELL_BYTES / sizeof(u32)];
    FILE *file;
    u32 offset;

    _Static_assert(NDS_FX_HIT_SPARK_ASSET_BYTES ==
                       NDS_FX_HIT_SPARK_CELL_BYTES *
                           NDS_FX_HIT_SPARK_FRAME_COUNT,
                   "Task 39 hit-spark payload size drifted");
    *vram_cursor = (*vram_cursor +
                    (NDS_IFCOMMON_OBJ_GFX_ALIGNMENT - 1u)) &
                   ~(NDS_IFCOMMON_OBJ_GFX_ALIGNMENT - 1u);
    if ((*vram_cursor + bytes) > NDS_IFCOMMON_OBJ_VRAM_BYTES)
    {
        return FALSE;
    }
    file = fopen(NDS_FX_HIT_SPARK_ASSET_PATH, "rb");
    if (file == NULL)
    {
        gNdsFxObjVramBytes = 0u;
        return FALSE;
    }
    sNdsFxHitSparkGfx =
        (u16 *)((u8 *)SPRITE_GFX + *vram_cursor);
    for (offset = 0u; offset < bytes;
         offset += NDS_FX_HIT_SPARK_CELL_BYTES)
    {
        u32 *dst = (u32 *)(void *)((u8 *)sNdsFxHitSparkGfx + offset);
        u32 word;

        if (fread(cell, 1u, sizeof(cell), file) != sizeof(cell))
        {
            fclose(file);
            sNdsFxHitSparkGfx = NULL;
            gNdsFxObjVramBytes = 0u;
            return FALSE;
        }
        for (word = 0u; word < (u32)(sizeof(cell) / sizeof(cell[0])); word++)
        {
            dst[word] = cell[word];
        }
    }
    fclose(file);
    *vram_cursor += bytes;
    gNdsFxObjVramBytes = bytes;
    gNdsFxObjVramRemaining =
        NDS_IFCOMMON_OBJ_VRAM_BYTES - *vram_cursor;
    return TRUE;
}

static s32 ndsFxPrepareHitSparks(u32 *vram_cursor)
{
    s32 result;

    ndsFsLock();
    result = ndsFxPrepareHitSparksUnlocked(vram_cursor);
    ndsFsUnlock();
    return result;
}

/* Drop the retained texture NAMES when the VRAM behind them is dropped.
 *
 * These names were cleared in exactly one place -- ndsIFCommonNativeOamInit,
 * which runs once at boot (nds_platform.c:328) -- while the allocations they
 * refer to are released on every scene change, by the
 * ndsRendererHardwareDiscardBattleStaticTextures call inside the scene-cache
 * eviction (reloc_backend_assets.c). A boot-scoped cache guarding a
 * scene-scoped resource.
 *
 * The consequence is silent and specific: ndsIFCommonNativeOamPrepareClouds
 * early-returns TRUE as soon as both cloud names and the traffic name are
 * non-zero, so after a scene reload it re-uploads NOTHING and the OAM path
 * keeps emitting handles into VRAM that has since been reused. Sprites drawn
 * from freed texture names is what the owner's rematch screenshot shows.
 *
 * This is the same shape as sNdsRendererBattleStaticTexturePrepared, which is
 * already cleared by ndsRendererHardwareDiscardTextureCache -- that latch
 * survived the bug because someone wired its invalidation; this one was
 * missed. */
s32 ndsIFCommonNativeOamIsPrepared(void)
{
    return (sNdsIFCommonPrepared != FALSE) ? TRUE : FALSE;
}

void ndsIFCommonNativeOamDiscardTextures(void)
{
    memset(sNdsIFCommonCloudTextureNames, 0,
           sizeof(sNdsIFCommonCloudTextureNames));
    sNdsIFCommonTrafficTextureName = 0u;

    /* The prepare latch, for the same reason and one level up. Its guard reads
     * `sNdsIFCommonPrepared && sNdsIFCommonPreparedFile == file_data`, which
     * LOOKS self-invalidating -- it compares the asset pointer, so a different
     * file re-prepares -- and that is exactly why it survived review. Pointer
     * identity is not identity across a rewound allocator: the reloc heap is
     * rewound between scenes (docs/BUGS.md records AdapterCount 2 as the proof),
     * so the second entry loads the same asset into the same slot, gets the same
     * address, matches the guard and returns TRUE without re-preparing -- after
     * ndsRendererHardwareDiscardBattleStaticTextures has already released the
     * cloud and traffic atlases this latch claims are resident.
     *
     * Clearing it here makes the cache scene-scoped, which is the scope it
     * always needed; within a scene the pointer compare is still valid because
     * addresses are stable there. */
    sNdsIFCommonPrepared = FALSE;
    sNdsIFCommonPreparedFile = NULL;
    sNdsIFCommonPreparedFileSize = 0u;
    /* Scene reset returns GO ownership and drops the end phase with it; the
     * next scene re-bakes GO and the source-selected ending from scratch. */
    sNdsIFCommonAnnounceActive = FALSE;
    sNdsIFCommonAnnounceGameSet = NDS_IFCOMMON_ANNOUNCE_TIME_UP;

    /* And the Task 39 hit-spark sheet, which is not a texture name at all but a
     * raw VRAM address -- (u8 *)SPRITE_GFX + vram_cursor, handed out by the same
     * OBJ VRAM packing the prepare above redoes from scratch. Both draw sites
     * guard on `!= NULL` only, so a stale pointer is indistinguishable from a
     * live one and the sparks keep sampling whatever now occupies that offset. */
    memset(sNdsFxHitSparks, 0, sizeof(sNdsFxHitSparks));
    sNdsFxHitSparkGfx = NULL;
    ndsIFCommonResetPlayerTags();
    ndsIFCommonResetItemArrow();

    gNdsIFCommonNativeOamTextureDiscardCount++;
}

/* R2-07 E2. The cloud and traffic atlases ONLY -- not the prepare latch, and
 * not the hit-spark OBJ VRAM. ndsIFCommonNativeOamDiscardTextures above is the
 * scene-teardown form and clears everything; this is the re-entry form, and the
 * difference matters because ndsIFCommonNativeOamPrepareClouds needs
 * sNdsIFCommonPrepared and sNdsIFCommonPreparedFile to still hold in order to
 * rebuild the atlases from the asset it already has.
 *
 * Why a second entry needs it: texture VRAM is allocated by libnds inside
 * glTexImage2D, and these three names are the only textures that survive
 * ndsRendererHardwareDiscardTextureCache -- PrepareClouds early-returns while
 * all three are non-zero. So entry one allocates statics first and atlases
 * after, while entry two frees and re-uploads the 24 statics AROUND atlas
 * blocks already sitting in the middle of the pool. Same bytes, different
 * layout, and the dynamic stage textures that fit on entry one no longer fit:
 * measured as TEXREJECT bit 12 (TEXIMAGE) on the second entry against a mask of
 * 0 on the first, which fails PrepareRun for run 42 and rejects the whole
 * native stage owner.
 *
 * Releasing them here restores entry one's allocation ORDER, which is what the
 * allocator actually depends on. */
void ndsIFCommonNativeOamReleaseCloudTextures(void)
{
    ndsIFCommonReleaseCloudAtlases();
    ndsIFCommonReleaseTrafficAtlas();
    gNdsIFCommonNativeOamCloudReleaseCount++;
}

void ndsIFCommonNativeOamInit(void)
{
    /* Traffic conversion and all four IFCommon atlases happen before
     * gameplay; the draw path only emits retained hardware handles. */
    gNdsIFCommonNativeOamPreparePaletteBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureCount = 0u;
    gNdsIFCommonNativeOamPrepareCloudFailureStage = 0u;
    memset((void *)gNdsIFCommonNativeOamPrepareCloudNonzeroTexels, 0,
           sizeof(gNdsIFCommonNativeOamPrepareCloudNonzeroTexels));
    gNdsIFCommonNativeOamHotConvertCount = 0u;
    gNdsIFCommonNativeOamRuntimeUploadBytes = 0u;
    memset(sNdsIFCommonCloudTextureNames, 0,
           sizeof(sNdsIFCommonCloudTextureNames));
    sNdsIFCommonTrafficTextureName = 0u;
    sNdsIFCommonPreparedFile = NULL;
    sNdsIFCommonPreparedFileSize = 0u;
    sNdsIFCommonAnnounceActive = FALSE;
    sNdsIFCommonAnnounceGameSet = NDS_IFCOMMON_ANNOUNCE_TIME_UP;
    memset(sNdsFxHitSparks, 0, sizeof(sNdsFxHitSparks));
    sNdsFxHitSparkGfx = NULL;
    ndsIFCommonResetPlayerTags();
    ndsIFCommonResetCommonLetters();
    gNdsFxSpawnTicks = 0u;
    gNdsFxUpdateTicks = 0u;
    gNdsFxDrawTicks = 0u;
    gNdsFxFrameTicks = 0u;
    gNdsFxMaxFrameTicks = 0u;
    gNdsFxEngagementMask = 0u;
    gNdsFxHitSparkSpawnCount = 0u;
    gNdsFxHitSparkUpdateCount = 0u;
    gNdsFxHitSparkDrawCount = 0u;
    gNdsFxHitSparkDropCount = 0u;
    gNdsFxFlashDrawCount = 0u;
    gNdsFxArenaRejectCount = 0u;
    gNdsFxArenaBootSize = gNdsTaskmanArenaChosenSize;
    gNdsFxObjVramBytes = 0u;
    gNdsFxObjVramRemaining = NDS_IFCOMMON_OBJ_VRAM_BYTES;
    /* The battle's first OBJ write: a held frame's OBJs (the 1P intro's,
     * nds_source2d.c) still read bank E and the OAM, so the hold ends here
     * (ndsPlatformTransitionThaw), before the init and the atlas bakes. */
    ndsPlatformTransitionThaw();
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    oamClear(&oamMain, 0, 128);
    oamUpdate(&oamMain);
}

/* Fixed GO slot bases inside the GO bank. G 7168 + O 7168 + ! 3072. */
static const u32 sNdsIFCommonGoSlotBases[3] = { 0u, 7168u, 14336u };

s32 ndsIFCommonNativeOamPrepareGameStatus(void *file_data,
                                           size_t file_size)
{
    u32 start;
    u32 vram_cursor;
    u32 go_index;

    _Static_assert(NDS_IFCOMMON_GO_BANK_BYTES == 17408u,
                   "GO bank size drifted");
    _Static_assert(NDS_IFCOMMON_END_BANK_BYTES == 20736u,
                   "end bank size drifted");
    _Static_assert(NDS_IFCOMMON_USED_BYTES <= NDS_IFCOMMON_OBJ_VRAM_BYTES,
                   "fixed banks exceed OBJ VRAM");

    if ((file_data == NULL) ||
        (file_size < NDS_IFCOMMON_GAME_STATUS_SIZE))
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        return FALSE;
    }
    if ((sNdsIFCommonPrepared != FALSE) &&
        (sNdsIFCommonPreparedFile == file_data))
    {
        return TRUE;
    }
    /* This bake runs while the battle's files load -- before the OAM init
     * below it, which used to be the hold's only end on this path. The held
     * frame (the 1P intro's banners, the SSS) still reads bank E, so baking
     * the GO, spark and end tiles into it showed them through the held
     * sprites (owner 2026-10-01: intro banners full of blue blocks). End the
     * hold first; the cover stands until the battle presents. */
    ndsPlatformTransitionThaw();

    start = cpuGetTiming();
    gNdsIFCommonNativeOamPrepareCount++;
    gNdsIFCommonNativeOamPrepareAssets = 0u;
    gNdsIFCommonNativeOamPrepareTiles = 0u;
    gNdsIFCommonNativeOamPrepareBytes = 0u;
    gNdsIFCommonNativeOamPreparePaletteBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureCount = 0u;
    gNdsIFCommonNativeOamPrepareCloudFailureStage = 0u;
    memset((void *)gNdsIFCommonNativeOamPrepareCloudNonzeroTexels, 0,
           sizeof(gNdsIFCommonNativeOamPrepareCloudNonzeroTexels));
    gNdsIFCommonNativeOamPrepareProfileFrame =
        gNdsRendererProfileFrameCount;
    ndsIFCommonReleaseCloudAtlases();
    ndsIFCommonReleaseTrafficAtlas();
    ndsIFCommonResetPlayerTags();
    ndsIFCommonResetItemArrow();
    ndsIFCommonResetCommonLetters();
    sNdsIFCommonPrepared = FALSE;
    sNdsIFCommonPreparedFile = NULL;
    sNdsIFCommonPreparedFileSize = 0u;
    sNdsIFCommonAnnounceActive = FALSE;
    sNdsIFCommonAnnounceGameSet = NDS_IFCOMMON_ANNOUNCE_TIME_UP;
    /* A new load is the full file again; the previous scene's compact image
     * and baked streams died with its heap. */
    sNdsIFCommonCompact = FALSE;
    sNdsIFCommonEndBaked[0] = NULL;
    sNdsIFCommonEndBaked[1] = NULL;
    sNdsIFCommonEndBakedWords[0] = 0u;
    sNdsIFCommonEndBakedWords[1] = 0u;

    if (ndsIFCommonBindGameStatusMetadata(file_data, file_size) == FALSE)
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackBadAsset;
        return FALSE;
    }
    for (go_index = 0u; go_index < 3u; go_index++)
    {
        if (ndsIFCommonBakeDirectAsset(go_index, file_data, file_size,
                                       NDS_IFCOMMON_GO_BANK_BASE +
                                           sNdsIFCommonGoSlotBases[
                                               go_index]) == FALSE)
        {
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
    }
    vram_cursor = NDS_IFCOMMON_SPARK_BANK_BASE;
    if (ndsFxPrepareHitSparks(&vram_cursor) == FALSE)
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackBadAsset;
        return FALSE;
    }

    sNdsIFCommonPrepared = TRUE;
    sNdsIFCommonPreparedFile = file_data;
    sNdsIFCommonPreparedFileSize = file_size;
    gNdsIFCommonNativeOamPrepareBytes = NDS_IFCOMMON_USED_BYTES;
    gNdsIFCommonNativeOamPrepareTicks = cpuGetTiming() - start;
    gNdsIFCommonNativeOamPrepareSuccessCount++;
    return TRUE;
}

/* Bake the source-selected ending into the end bank. The source SObj
 * creation seam calls this before adding the first end-message glyph, so
 * GO can still be live: GO bytes are never touched here. TIME UP (0) and
 * GAME SET (1) are source-exclusive per round and share the bank. Repeat
 * calls with the active phase upload nothing. A partial failure leaves the
 * phase inactive so a retry re-bakes cleanly without falsely reading ready. */
s32 ndsIFCommonNativeOamPrepareAnnouncement(u32 game_set)
{
    const NDSIFCommonEndSlot *slots;
    u32 start;
    u32 slot;

    if (game_set > NDS_IFCOMMON_ANNOUNCE_GAME_SET)
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        return FALSE;
    }
    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonPreparedFile == NULL) ||
        ((sNdsIFCommonCompact == FALSE) &&
         (sNdsIFCommonPreparedFileSize < NDS_IFCOMMON_GAME_STATUS_SIZE)))
    {
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackNotPrepared;
        return FALSE;
    }
    if ((sNdsIFCommonAnnounceActive != FALSE) &&
        (sNdsIFCommonAnnounceGameSet == game_set))
    {
        return TRUE;
    }
    start = cpuGetTiming();
    /* TIME UP / GAME SET take the end bank the common letters live in. */
    ndsIFCommonResetCommonLetters();
    slots = (game_set != FALSE) ? sNdsIFCommonGameSetSlots :
                                  sNdsIFCommonTimeUpSlots;
    if (sNdsIFCommonCompact != FALSE)
    {
        /* The letters' pixels are not resident: lay the tiles out exactly as
         * the bake below would, then expand the pre-baked bank image. */
        const u16 *stream = sNdsIFCommonEndBaked[game_set];
        u32 words = sNdsIFCommonEndBakedWords[game_set];
        u16 *out = (u16 *)((u8 *)SPRITE_GFX + NDS_IFCOMMON_END_BANK_BASE);
        u32 out_words = NDS_IFCOMMON_END_BANK_BYTES / sizeof(u16);
        u32 at = 0u;
        u32 i = 0u;

        for (slot = 0u; slot < 6u; slot++)
        {
            if ((stream == NULL) ||
                (ndsIFCommonBakeDirectAssetTo(
                     slots[slot].asset_index, sNdsIFCommonPreparedFile,
                     sNdsIFCommonPreparedFileSize,
                     NDS_IFCOMMON_END_BANK_BASE + slots[slot].bank_offset,
                     (u16 *)SPRITE_GFX, 0u, TRUE, FALSE) == FALSE))
            {
                stream = NULL;
                break;
            }
        }
        while ((stream != NULL) && (i < words) && (at < out_words))
        {
            u32 control = stream[i++];
            u32 count = control & 0x7fffu;

            if ((count == 0u) || ((at + count) > out_words))
            {
                stream = NULL;
                break;
            }
            if ((control & 0x8000u) != 0u)
            {
                if ((i + count) > words)
                {
                    stream = NULL;
                    break;
                }
                while (count-- != 0u)
                {
                    out[at++] = stream[i++];
                }
            }
            else
            {
                while (count-- != 0u)
                {
                    out[at++] = 0u;
                }
            }
        }
        if ((stream == NULL) || (at != out_words) || (i != words))
        {
            for (slot = 0u; slot < 6u; slot++)
            {
                NDSIFCommonNativeAsset *dead =
                    &sNdsIFCommonAssets[slots[slot].asset_index];
                u32 dead_tile;

                for (dead_tile = 0u; dead_tile < NDS_IFCOMMON_MAX_TILES;
                     dead_tile++)
                {
                    dead->tiles[dead_tile].gfx = NULL;
                }
            }
            sNdsIFCommonAnnounceActive = FALSE;
            NDS_DIAG(gNdsIFCommonEndBakedFailCount++);
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
        NDS_DIAG(gNdsIFCommonEndBakedDecodeCount++);
        sNdsIFCommonAnnounceActive = TRUE;
        sNdsIFCommonAnnounceGameSet = game_set;
        gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
        gNdsIFCommonNativeOamPrepareSuccessCount++;
        return TRUE;
    }
    for (slot = 0u; slot < 6u; slot++)
    {
        u32 asset_index = slots[slot].asset_index;
        NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];
        u32 tile;

        for (tile = 0u; tile < NDS_IFCOMMON_MAX_TILES; tile++)
        {
            asset->tiles[tile].gfx = NULL;
        }
        if (ndsIFCommonBakeDirectAsset(
                asset_index, sNdsIFCommonPreparedFile,
                sNdsIFCommonPreparedFileSize,
                NDS_IFCOMMON_END_BANK_BASE + slots[slot].bank_offset) ==
            FALSE)
        {
            u32 clear;

            for (clear = 0u; clear <= slot; clear++)
            {
                NDSIFCommonNativeAsset *dead =
                    &sNdsIFCommonAssets[slots[clear].asset_index];
                u32 dead_tile;

                for (dead_tile = 0u; dead_tile < NDS_IFCOMMON_MAX_TILES;
                     dead_tile++)
                {
                    dead->tiles[dead_tile].gfx = NULL;
                }
            }
            sNdsIFCommonAnnounceActive = FALSE;
            gNdsIFCommonNativeOamPrepareFailCount++;
            gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
            gNdsIFCommonNativeOamLastFallbackReason =
                nNDSIFCommonFallbackBadAsset;
            return FALSE;
        }
    }
    sNdsIFCommonAnnounceActive = TRUE;
    sNdsIFCommonAnnounceGameSet = game_set;
    gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
    gNdsIFCommonNativeOamPrepareSuccessCount++;
    return TRUE;
}

s32 ndsIFCommonNativeOamPrepareClouds(void)
{
    u32 start;

    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonPreparedFile == NULL) ||
        ((sNdsIFCommonCompact == FALSE) &&
         (sNdsIFCommonPreparedFileSize < NDS_IFCOMMON_GAME_STATUS_SIZE)))
    {
        gNdsIFCommonNativeOamPrepareCloudFailureStage = 1u;
        gNdsIFCommonNativeOamPrepareFailCount++;
        return FALSE;
    }
    if ((sNdsIFCommonCloudTextureNames[0] != 0u) &&
        (sNdsIFCommonCloudTextureNames[1] != 0u) &&
        (sNdsIFCommonTrafficTextureName != 0u))
    {
        return TRUE;
    }

    start = cpuGetTiming();
    ndsIFCommonReleaseCloudAtlases();
    ndsIFCommonReleaseTrafficAtlas();
    gNdsIFCommonNativeOamPrepareCloudTextureBytes = 0u;
    gNdsIFCommonNativeOamPrepareCloudTextureCount = 0u;
    gNdsIFCommonNativeOamPrepareCloudFailureStage = 0u;
    memset((void *)gNdsIFCommonNativeOamPrepareCloudNonzeroTexels, 0,
           sizeof(gNdsIFCommonNativeOamPrepareCloudNonzeroTexels));
    if ((ndsIFCommonPrepareCloudAtlases(
             sNdsIFCommonPreparedFile,
             sNdsIFCommonPreparedFileSize) == FALSE) ||
        (ndsIFCommonPrepareTrafficAtlas(
            sNdsIFCommonPreparedFile,
            sNdsIFCommonPreparedFileSize) == FALSE))
    {
        ndsIFCommonReleaseCloudAtlases();
        ndsIFCommonReleaseTrafficAtlas();
        gNdsIFCommonNativeOamPrepareFailCount++;
        gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
        gNdsIFCommonNativeOamLastFallbackReason =
            nNDSIFCommonFallbackBadAsset;
        return FALSE;
    }
    gNdsIFCommonNativeOamPrepareTicks += cpuGetTiming() - start;
    return TRUE;
}

extern void *syTaskmanMalloc(size_t size, u32 align);

static u32 ndsIFCommonPayloadBytes(const Sprite *sprite, const Bitmap *bitmap)
{
    u32 texels = (u32)(u16)bitmap->width_img * (u32)(u16)bitmap->actualHeight;

    switch (sprite->bmsiz)
    {
    case G_IM_SIZ_4b: return (texels + 1u) / 2u;
    case G_IM_SIZ_8b: return texels;
    case G_IM_SIZ_16b: return texels * 2u;
    case G_IM_SIZ_32b: return texels * 4u;
    default: return 0u;
    }
}

/* Ranges (offset from the prepared file, bytes) the compact image may drop:
 * every bitmap payload of an asset with OBJ tiles -- the GO and end letters,
 * whose pixels are baked here and never read again. Returns 0 (keep the
 * whole file) if any range is out of the file, exceeds `max`, or shares a
 * byte with a payload whose asset keeps its pixels (lamps, rod, frame: the
 * cloud and traffic atlases read them after this point). */
u32 ndsIFCommonNativeOamLetterPayloads(u32 *offsets, u32 *bytes, u32 max)
{
    const u8 *base = (const u8 *)sNdsIFCommonPreparedFile;
    size_t size = sNdsIFCommonPreparedFileSize;
    u32 count = 0u;
    u32 pass;

    if ((sNdsIFCommonPrepared == FALSE) || (base == NULL) ||
        (sNdsIFCommonCompact != FALSE))
    {
        return 0u;
    }
    for (pass = 0u; pass < 2u; pass++)
    {
        u32 asset_index;

        for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
             asset_index++)
        {
            const Sprite *sprite = sNdsIFCommonAssets[asset_index].sprite;
            u32 letter =
                (sNdsIFCommonAssetSpecs[asset_index].tile_count != 0u) ?
                    TRUE : FALSE;
            u32 b;

            if (sprite == NULL)
            {
                return 0u;
            }
            if ((pass == 0u) != (letter != FALSE))
            {
                continue;
            }
            for (b = 0u; b < (u32)(u16)sprite->nbitmaps; b++)
            {
                const Bitmap *bitmap = &sprite->bitmap[b];
                u32 payload = ndsIFCommonPayloadBytes(sprite, bitmap);
                uintptr_t at = (uintptr_t)bitmap->buf - (uintptr_t)base;
                u32 i;

                if ((bitmap->buf == NULL) || (payload == 0u))
                {
                    continue;
                }
                if ((at >= size) || (payload > (size - at)))
                {
                    return 0u;
                }
                if (pass == 0u)
                {
                    for (i = 0u; i < count; i++)
                    {
                        if ((offsets[i] == (u32)at) && (bytes[i] == payload))
                        {
                            break;
                        }
                    }
                    if (i < count)
                    {
                        continue;
                    }
                    if (count >= max)
                    {
                        return 0u;
                    }
                    offsets[count] = (u32)at;
                    bytes[count] = payload;
                    count++;
                    continue;
                }
                for (i = 0u; i < count; i++)
                {
                    if (((u32)at < (offsets[i] + bytes[i])) &&
                        (offsets[i] < ((u32)at + payload)))
                    {
                        return 0u;
                    }
                }
            }
        }
    }
    return count;
}

static u32 ndsIFCommonRleEncode(const u16 *in, u32 n, u16 *out, u32 cap)
{
    u32 i = 0u;
    u32 o = 0u;

    while (i < n)
    {
        if (in[i] == 0u)
        {
            u32 run = 0u;

            while (((i + run) < n) && (in[i + run] == 0u) && (run < 0x7fffu))
            {
                run++;
            }
            if (o >= cap)
            {
                return 0u;
            }
            out[o++] = (u16)run;
            i += run;
        }
        else
        {
            u32 start = i;
            u32 literal = 0u;

            /* A lone zero between texels stays literal: a zero run costs a
             * control word either way. */
            while ((i < n) && (literal < 0x7fffu))
            {
                if ((in[i] == 0u) && (((i + 1u) >= n) || (in[i + 1u] == 0u)))
                {
                    break;
                }
                i++;
                literal++;
            }
            if ((o + 1u + literal) > cap)
            {
                return 0u;
            }
            out[o++] = (u16)(0x8000u | literal);
            memcpy(&out[o], &in[start], literal * sizeof(u16));
            o += literal;
        }
    }
    return o;
}

/* Where an end stream is kept (2026-10-18). Bank H is the sub screen's BG0
 * console (nds_platform.c consoleInit: 4bpp font tiles from its char base,
 * one 2 KB map at its map base), and the bytes between the font's last tile
 * and the map are drawn by nothing: 22,528 B, which hold both streams
 * (22,104 B, every one of them a match's heap before). VRAM takes no byte
 * stores, so the caller copies halfwords; the decode reads halfwords. A
 * stream that does not fit takes the scene heap, as it did. */
#define NDS_IFCOMMON_END_SPARE_BANK_BASE 0x06200000u
#define NDS_IFCOMMON_END_SPARE_BANK_BYTES 0x8000u
__attribute__((used)) volatile u32 gNdsIFCommonEndBakedSpareBytes;

static u16 *ndsIFCommonEndBakeStore(u32 words, u32 *spare_used)
{
    PrintConsole *console = consoleSelect(NULL);
    u32 bytes = ((words * (u32)sizeof(u16)) + 3u) & ~3u;
    uintptr_t lo;
    uintptr_t hi;
    uintptr_t at;

    consoleSelect(console);
    if ((console != NULL) && (console->consoleInitialised != false) &&
        (console->bgLayer == 0u) && ((REG_BG0CNT_SUB & BIT(7)) == 0u) &&
        (console->fontBgGfx != NULL) && (console->fontBgMap != NULL))
    {
        /* A 4bpp tile is 32 bytes. */
        lo = (uintptr_t)console->fontBgGfx +
             (((uintptr_t)console->fontCharOffset +
               (uintptr_t)console->font.numChars) * 32u);
        hi = (uintptr_t)console->fontBgMap;
        at = ((lo + 3u) & ~(uintptr_t)3u) + *spare_used;
        if ((lo >= NDS_IFCOMMON_END_SPARE_BANK_BASE) && (hi > lo) &&
            (hi <= (NDS_IFCOMMON_END_SPARE_BANK_BASE +
                    NDS_IFCOMMON_END_SPARE_BANK_BYTES)) &&
            ((at + bytes) <= hi))
        {
            *spare_used += bytes;
            gNdsIFCommonEndBakedSpareBytes += bytes;
            return (u16 *)at;
        }
    }
    return (u16 *)syTaskmanMalloc(bytes, 4u);
}

/* Bake TIME UP (0) and GAME SET (1) into RAM images of the end bank, exactly
 * as ndsIFCommonNativeOamPrepareAnnouncement would bake them into OBJ VRAM,
 * and keep them run-length coded (ndsIFCommonEndBakeStore). `scratch` holds
 * one bank image; `code` holds the worst-case stream. Must run while the
 * full file is still the prepared file. */
s32 ndsIFCommonNativeOamBakeEndVariants(u16 *scratch, u16 *code,
                                        u32 code_capacity_words)
{
    u32 game_set;
    u32 spare_used = 0u;

    if ((sNdsIFCommonPrepared == FALSE) || (sNdsIFCommonCompact != FALSE) ||
        (scratch == NULL) || (code == NULL))
    {
        return FALSE;
    }
    gNdsIFCommonEndBakedBytes = 0u;
    for (game_set = 0u; game_set < 2u; game_set++)
    {
        const NDSIFCommonEndSlot *slots =
            (game_set != 0u) ? sNdsIFCommonGameSetSlots :
                               sNdsIFCommonTimeUpSlots;
        u32 slot;
        u32 words;
        u16 *resident;

        memset(scratch, 0, NDS_IFCOMMON_END_BANK_BYTES);
        for (slot = 0u; slot < 6u; slot++)
        {
            if (ndsIFCommonBakeDirectAssetTo(
                    slots[slot].asset_index, sNdsIFCommonPreparedFile,
                    sNdsIFCommonPreparedFileSize,
                    NDS_IFCOMMON_END_BANK_BASE + slots[slot].bank_offset,
                    scratch, NDS_IFCOMMON_END_BANK_BASE, FALSE, TRUE) ==
                FALSE)
            {
                NDS_DIAG(gNdsIFCommonEndBakedFailCount++);
                return FALSE;
            }
        }
        words = ndsIFCommonRleEncode(
            scratch, NDS_IFCOMMON_END_BANK_BYTES / sizeof(u16), code,
            code_capacity_words);
        resident = (words != 0u) ?
            ndsIFCommonEndBakeStore(words, &spare_used) : NULL;
        if (resident == NULL)
        {
            NDS_DIAG(gNdsIFCommonEndBakedFailCount++);
            return FALSE;
        }
        /* Halfword stores (VRAM drops byte stores): volatile keeps the
         * compiler from turning the loop into memcpy. */
        for (slot = 0u; slot < words; slot++)
        {
            ((volatile u16 *)resident)[slot] = code[slot];
        }
        sNdsIFCommonEndBaked[game_set] = resident;
        sNdsIFCommonEndBakedWords[game_set] = words;
        NDS_DIAG(gNdsIFCommonEndBakedBytes += words * sizeof(u16));
    }
    return TRUE;
}

/* The loader copied the file into its compact image: move every retained
 * pointer into it. `map` turns a source offset (and a byte count that must
 * stay contiguous) into a compact offset. All-or-nothing. */
s32 ndsIFCommonNativeOamIsPreparedFile(const void *file_data)
{
    return ((sNdsIFCommonPrepared != FALSE) &&
            (sNdsIFCommonPreparedFile == file_data)) ? TRUE : FALSE;
}

s32 ndsIFCommonNativeOamRebaseGameStatus(
    const void *old_base, size_t old_size, void *new_base, size_t new_size,
    s32 (*map)(u32 source_offset, u32 bytes, u32 *out_offset), u32 compact)
{
    const Sprite *sprites[NDS_IFCOMMON_ASSET_COUNT];
    u32 asset_index;

    if ((sNdsIFCommonPrepared == FALSE) ||
        (sNdsIFCommonPreparedFile != old_base) || (map == NULL) ||
        (new_base == NULL))
    {
        return FALSE;
    }
    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        const Sprite *sprite = sNdsIFCommonAssets[asset_index].sprite;
        uintptr_t at = (uintptr_t)sprite - (uintptr_t)old_base;
        u32 mapped;

        if ((sprite == NULL) || (at >= old_size) ||
            (map((u32)at, sizeof(Sprite), &mapped) == FALSE) ||
            ((mapped + sizeof(Sprite)) > new_size))
        {
            return FALSE;
        }
        sprites[asset_index] =
            (const Sprite *)((const u8 *)new_base + mapped);
    }
    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        NDSIFCommonNativeAsset *asset = &sNdsIFCommonAssets[asset_index];

        asset->sprite = sprites[asset_index];
        /* The loader already re-seated the Sprite's own bitmap pointer. */
        asset->bitmap = sprites[asset_index]->bitmap;
    }
    sNdsIFCommonPreparedFile = new_base;
    sNdsIFCommonPreparedFileSize = new_size;
    sNdsIFCommonCompact = (compact != FALSE) ? TRUE : FALSE;
    return TRUE;
}

_Static_assert(NDS_IFCOMMON_END_BANK_IMAGE_BYTES ==
                   NDS_IFCOMMON_END_BANK_BYTES,
               "the loader's end-bank scratch must match the bank");

#if NDS_1P_GAME
static void ndsIFCommonTrainingFrameBegin(void);
#endif

void ndsIFCommonNativeOamBeginFrame(void)
{
#if NDS_1P_GAME
    ndsIFCommonTrainingFrameBegin();
#endif
#if NDS_SHIP_TELEMETRY
    gNdsIFCommonNativeOamFrameBeginTicks = 0u;
    gNdsIFCommonNativeOamFrameCommitTicks = 0u;
#endif
    gNdsIFCommonNativeOamFrameCommitCalls = 0u;
    gNdsIFCommonNativeOamFrameClearedObjects = 0u;
    gNdsIFCommonNativeOamFrameIdle = 0u;
    sNdsIFCommonFrameNeedsCommit = FALSE;
    if (sNdsIFCommonPreviousLowestOamID < 128)
    {
        u32 start = NDS_IFCOMMON_TELEMETRY_TICK();

        gNdsIFCommonNativeOamFrameClearedObjects =
            (u32)(128 - sNdsIFCommonPreviousLowestOamID);
        oamClear(&oamMain, sNdsIFCommonPreviousLowestOamID,
                 (int)gNdsIFCommonNativeOamFrameClearedObjects);
        NDS_IFCOMMON_TELEMETRY_ADD(
            gNdsIFCommonNativeOamFrameBeginTicks, start);
        sNdsIFCommonFrameNeedsCommit = TRUE;
    }
    sNdsIFCommonNextOamID = 127;
    sNdsIFCommonMatrixCount = 0u;
#if NDS_SHIP_TELEMETRY
    gNdsIFCommonNativeOamFrameTicks = 0u;
#endif
    gNdsIFCommonNativeOamFrameRecognizedCalls = 0u;
    gNdsIFCommonNativeOamFrameDrawCalls = 0u;
    gNdsIFCommonNativeOamFrameFallbackCalls = 0u;
#if NDS_SHIP_TELEMETRY
    gNdsIFCommonNativeOamFrameSObjCount = 0u;
    gNdsIFCommonNativeOamFrameSemanticHash = NDS_IFCOMMON_HASH_SEED;
#endif
    gNdsIFCommonNativeOamFrameObjectCount = 0u;
    gNdsIFCommonNativeOamFrameCloudDrawCount = 0u;
    NDS_DIAG(gNdsIFCommonNativeOamLastFallbackReason =
        nNDSIFCommonFallbackNone);
    ndsFxHitSparksDraw();
}

static s32 ndsIFCommonAssetForSObj(const SObj *sobj)
{
    u32 asset_index;

    for (asset_index = 0u; asset_index < NDS_IFCOMMON_ASSET_COUNT;
         asset_index++)
    {
        const NDSIFCommonNativeAsset *asset =
            &sNdsIFCommonAssets[asset_index];

        if ((sobj->sprite.bitmap == asset->bitmap) &&
            ((u32)(u16)sobj->sprite.width == asset->width) &&
            ((u32)(u16)sobj->sprite.height == asset->height))
        {
            if ((asset_index == nNDSIFCommonAssetShadowInitial) ||
                (asset_index == nNDSIFCommonAssetShadowGo))
            {
                if (((u32)sobj->sprite.red != asset->runtime_red) ||
                    ((u32)sobj->sprite.green != asset->runtime_green) ||
                    ((u32)sobj->sprite.blue != asset->runtime_blue) ||
                    ((u32)sobj->envcolor.r != asset->runtime_env_red) ||
                    ((u32)sobj->envcolor.g != asset->runtime_env_green) ||
                    ((u32)sobj->envcolor.b != asset->runtime_env_blue))
                {
                    continue;
                }
            }
            else if ((asset_index >= nNDSIFCommonAssetRedDim) &&
                     (((u32)sobj->sprite.red != asset->runtime_red) ||
                      ((u32)sobj->sprite.green != asset->runtime_green) ||
                      ((u32)sobj->sprite.blue != asset->runtime_blue)))
            {
                return -2;
            }
            return (s32)asset_index;
        }
    }
    return -1;
}

static s32 ndsIFCommonMatrixForScale(u16 inverse)
{
    u32 matrix_index;

    for (matrix_index = 0u; matrix_index < sNdsIFCommonMatrixCount;
         matrix_index++)
    {
        if (sNdsIFCommonMatrixInverse[matrix_index] == inverse)
        {
            return (s32)matrix_index;
        }
    }
    if (sNdsIFCommonMatrixCount >= 32u)
    {
        return -1;
    }
    matrix_index = sNdsIFCommonMatrixCount++;
    sNdsIFCommonMatrixInverse[matrix_index] = inverse;
    oamRotateScale(&oamMain, (int)matrix_index, 0, inverse, inverse);
    return (s32)matrix_index;
}

static void ndsFxHitSparksDraw(void)
{
    u32 index;

    if ((sNdsFxHitSparkGfx == NULL) || (gGMCameraGObj == NULL) ||
        (CObjGetStruct(gGMCameraGObj) == NULL) ||
        (ndsFxEffectsArenaValid() == FALSE))
    {
        return;
    }
    for (index = 0u; index < NDS_FX_HIT_SPARK_CAPACITY; index++)
    {
        NDSFxHitSpark *spark = &sNdsFxHitSparks[index];
        Vec3f pos;
        f32 projected_x;
        f32 projected_y;
        f32 scale;
        u32 scale_q16;
        u32 frame;
        u16 inverse;
        s32 matrix_index;
        s32 center_x;
        s32 center_y;
        s32 size_double;

        if (spark->active == FALSE)
        {
            continue;
        }
        if (sNdsIFCommonNextOamID < 0)
        {
            NDS_DIAG(gNdsFxHitSparkDropCount++);
            break;
        }
        pos = spark->pos;
        func_ovl2_800EB924(CObjGetStruct(gGMCameraGObj),
                           gGMCameraMatrix, &pos,
                           &projected_x, &projected_y);
        center_x = ndsIFCommonRoundFloatHalfUp(
            ndsIFCommonBattleScreenX(projected_x));
        center_y = ndsIFCommonRoundFloatHalfUp(
            ndsIFCommonBattleScreenY(projected_y));
        if ((center_x < -16) || (center_x > 272) ||
            (center_y < -16) || (center_y > 208))
        {
            continue;
        }

        scale = spark->scale * NDS_FX_HIT_SPARK_SCREEN_SCALE;
        scale_q16 = (u32)((scale * 65536.0F) + 0.5F);
        if (scale_q16 == 0u)
        {
            continue;
        }
        inverse = (u16)(((1u << 24) + (scale_q16 / 2u)) / scale_q16);
        matrix_index = ndsIFCommonMatrixForScale(inverse);
        if (matrix_index < 0)
        {
            NDS_DIAG(gNdsFxHitSparkDropCount++);
            continue;
        }
        if (spark->is_heavy != FALSE)
        {
            frame = NDS_FX_HIT_SPARK_HEAVY_OFFSET +
                ((u32)spark->player * NDS_FX_HIT_SPARK_HEAVY_FRAMES) +
                (((u32)spark->age * NDS_FX_HIT_SPARK_HEAVY_FRAMES) /
                 NDS_FX_HIT_SPARK_HEAVY_LIFETIME);
        }
        else
        {
            frame = ((u32)spark->player *
                     NDS_FX_HIT_SPARK_LIGHT_FRAMES) +
                (((u32)spark->age * NDS_FX_HIT_SPARK_LIGHT_FRAMES) /
                 NDS_FX_HIT_SPARK_LIGHT_LIFETIME);
        }
        size_double = (scale_q16 > (1u << 16)) ? TRUE : FALSE;
        oamSet(&oamMain, sNdsIFCommonNextOamID,
               center_x - (size_double ? 16 : 8),
               center_y - (size_double ? 16 : 8),
               0, 15, SpriteSize_16x16, SpriteColorFormat_Bmp,
               sNdsFxHitSparkGfx +
                   (frame * (NDS_FX_HIT_SPARK_CELL_BYTES / 2u)),
               matrix_index, size_double, false, false, false, false);
        sNdsIFCommonNextOamID--;
        sNdsIFCommonFrameNeedsCommit = TRUE;
        gNdsIFCommonNativeOamFrameObjectCount++;
        NDS_DIAG(gNdsFxHitSparkDrawCount++);
    }
}

#if NDS_SHIP_TELEMETRY
static void ndsIFCommonRecordSemantic(const SObj *sobj, u32 asset_index)
{
    u32 hash = gNdsIFCommonNativeOamFrameSemanticHash;

    hash = ndsIFCommonHashMix(hash, asset_index);
    hash = ndsIFCommonHashMix(hash, ndsIFCommonFloatBits(sobj->pos.x));
    hash = ndsIFCommonHashMix(hash, ndsIFCommonFloatBits(sobj->pos.y));
    hash = ndsIFCommonHashMix(hash,
                             ndsIFCommonFloatBits(sobj->sprite.scalex));
    hash = ndsIFCommonHashMix(hash,
                             ndsIFCommonFloatBits(sobj->sprite.scaley));
    hash = ndsIFCommonHashMix(
        hash, ((u32)sobj->sprite.red << 24) |
                  ((u32)sobj->sprite.green << 16) |
                  ((u32)sobj->sprite.blue << 8) | sobj->sprite.alpha);
    hash = ndsIFCommonHashMix(
        hash, ((u32)sobj->envcolor.r << 24) |
                  ((u32)sobj->envcolor.g << 16) |
                  ((u32)sobj->envcolor.b << 8) | sobj->envcolor.a);
    hash = ndsIFCommonHashMix(hash, sobj->sprite.attr);
    gNdsIFCommonNativeOamFrameSemanticHash = hash;
    gNdsIFCommonNativeOamFrameSObjCount++;
}
#endif

static u32 ndsIFCommonBitmapAlpha(u8 source_alpha)
{
    if (source_alpha == 0u)
    {
        return 0u;
    }
    return (((u32)source_alpha * 15u) + 127u) / 255u;
}
