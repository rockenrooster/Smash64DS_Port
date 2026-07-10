static void ndsSceneBoundary(void)
{
    gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
    gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
    if (gSCManagerSceneData.scene_curr == nSCKindTitle)
    {
        gNdsOpeningMovieTitleResult = NDS_OPENING_MOVIE_TITLE_PASS;
    }
    gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
    osStopThread(NULL);
}

static NDSRelocLoadedFile *ndsTitleLoadPreviewFile(void)
{
    NDSRelocAssetHeader header;
    NDSRelocLoadedFile *loaded;

    ndsRelocResetLoadedFiles();
    if (ndsRelocAssetReadHeader(NDS_RELOC_ASSET_MN_TITLE, &header) == FALSE)
    {
        return NULL;
    }
    if (header.data_size > NDS_TITLE_FILE_BUFFER_SIZE)
    {
        return NULL;
    }
    if (ndsRelocAssetLoadData(NDS_RELOC_ASSET_MN_TITLE,
                              sNdsTitleFileBuffer,
                              sizeof(sNdsTitleFileBuffer),
                              &header) == FALSE)
    {
        return NULL;
    }

    loaded = ndsRelocRegisterLoadedFile(NDS_RELOC_ASSET_MN_TITLE, 0,
                                        sNdsTitleFileBuffer, &header);
    if (loaded == NULL)
    {
        return NULL;
    }
    if ((ndsRelocApplyWordByteSwap(loaded) == FALSE) ||
        (ndsRelocApplyInternalPointerFixups(loaded) == FALSE))
    {
        return NULL;
    }

    ndsRelocNormalizeTitleSprites(loaded);
    return (gNdsTitleRelocResult == NDS_TITLE_RELOC_PASS) ? loaded : NULL;
}

static void ndsTitleRenderPreview(void)
{
    NDSRelocLoadedFile *loaded = ndsTitleLoadPreviewFile();
    SObj sobjs[ARRAY_COUNT(sNdsTitleSpriteDescs)];
    u16 *preview;
    u32 preview_pitch = 0;
    u32 drew_any = 0;
    u32 i;

    if (loaded == NULL)
    {
        return;
    }

    memset(sobjs, 0, sizeof(sobjs));
    preview = ndsPlatformBeginOriginalSpritePreview(320u, 240u, 0, 0,
                                                    &preview_pitch);
    if ((preview == NULL) || (preview_pitch == 0))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsTitleSpriteDescs); i++)
    {
        const NDSTitleSpriteDesc *desc = &sNdsTitleSpriteDescs[i];
        Sprite *sprite = lbRelocGetFileData(Sprite*, loaded->data,
                                            desc->symbol);

        if (sprite == NULL)
        {
            continue;
        }

        sobjs[i].sprite = *sprite;
        sobjs[i].pos.x = (f32)desc->center_x -
                         ((f32)(u16)sobjs[i].sprite.width * 0.5F);
        sobjs[i].pos.y = (f32)desc->center_y -
                         ((f32)(u16)sobjs[i].sprite.height * 0.5F);
        if ((i + 1u) < ARRAY_COUNT(sNdsTitleSpriteDescs))
        {
            sobjs[i].next = &sobjs[i + 1u];
        }

        if ((sobjs[i].sprite.attr & SP_HIDDEN) == 0)
        {
            gNdsTitleDrawVisibleSObjCount++;
            if (ndsSObjPreviewBasicSupported(&sobjs[i]) != FALSE)
            {
                gNdsTitleDrawRenderableSObjCount++;
            }
            if (ndsDrawSObjIntoPreview(
                    &sobjs[i], 0, preview, preview_pitch, 320u, 240u,
                    (s32)sobjs[i].pos.x, (s32)sobjs[i].pos.y, 0u) != FALSE)
            {
                drew_any++;
            }
        }
    }

    gNdsTitleDrawSObjCount = ARRAY_COUNT(sNdsTitleSpriteDescs);
    if (drew_any != 0)
    {
        ndsPlatformCommitOriginalSpritePreview();
        gNdsTitlePreviewResult = NDS_TITLE_PREVIEW_PASS;
    }
}

#define NDS_SCENE_STUB(name) void name(void) { ndsSceneBoundary(); }

static const NDSOpeningActionPreviewDesc *
ndsOpeningActionPreviewDescForKind(u32 scene_kind)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sNdsOpeningActionPreviewDescs); i++)
    {
        if (sNdsOpeningActionPreviewDescs[i].scene_kind == scene_kind)
        {
            return &sNdsOpeningActionPreviewDescs[i];
        }
    }
    return NULL;
}

static u32 ndsOpeningMovieBridgeMaskForKind(u32 scene_kind)
{
    if ((scene_kind < nSCKindOpeningRun) ||
        (scene_kind > nSCKindOpeningNewcomers))
    {
        return 0;
    }
    return 1u << (scene_kind - nSCKindOpeningRun);
}

static NDSOpeningActionPreviewCache *ndsOpeningActionPreviewFindCache(
    const NDSOpeningActionPreviewDesc *desc)
{
    NDSOpeningActionPreviewCache *empty = NULL;
    u32 i;

    if (desc == NULL)
    {
        return NULL;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsOpeningActionPreviewCaches); i++)
    {
        NDSOpeningActionPreviewCache *cache =
            &sNdsOpeningActionPreviewCaches[i];

        if ((cache->ready != 0) &&
            (cache->asset_id == desc->asset_id) &&
            (cache->offset == desc->offset))
        {
            return cache;
        }
        if ((cache->ready == 0) && (empty == NULL))
        {
            empty = cache;
        }
    }

    if (empty != NULL)
    {
        empty->asset_id = desc->asset_id;
        empty->offset = desc->offset;
        empty->ready = 0;
        empty->pixel_count = 0;
    }
    return empty;
}

static u32 ndsOpeningActionPreviewCountPixels(const u16 *pixels)
{
    u32 i;
    u32 count = 0;

    if (pixels == NULL)
    {
        return 0;
    }

    for (i = 0;
         i < (NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH *
              NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT);
         i++)
    {
        if (pixels[i] != 0)
        {
            count++;
        }
    }
    return count;
}

static s32 ndsOpeningActionPreviewCommitCache(
    const NDSOpeningActionPreviewCache *cache, u32 scene_kind)
{
    u16 *preview;
    u32 preview_pitch = 0;
    u32 row;

    if ((cache == NULL) || (cache->ready == 0))
    {
        return FALSE;
    }

    preview = ndsPlatformBeginOriginalSpritePreview(
        NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH,
        NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT,
        0, 0, &preview_pitch);
    if ((preview == NULL) || (preview_pitch == 0))
    {
        return FALSE;
    }

    for (row = 0; row < NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT; row++)
    {
        memcpy(&preview[row * preview_pitch],
               &cache->pixels[row *
                              NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH],
               NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH *
               sizeof(cache->pixels[0]));
    }

    ndsPlatformCommitOriginalSpritePreview();
    gNdsOpeningMovieActionPreviewResult =
        NDS_OPENING_MOVIE_ACTION_PREVIEW_PASS;
    gNdsOpeningMovieActionPreviewMask |=
        ndsOpeningMovieBridgeMaskForKind(scene_kind);
    gNdsOpeningMovieActionPreviewPixels += cache->pixel_count;
    gNdsOpeningMovieActionPreviewLastKind = scene_kind;
    gNdsOpeningMovieActionPreviewLastWidth = cache->width;
    gNdsOpeningMovieActionPreviewLastHeight = cache->height;
    gNdsOpeningMovieActionPreviewLastFormat = cache->bmfmt;
    gNdsOpeningMovieActionPreviewLastSize = cache->bmsiz;
    return TRUE;
}

static NDSRelocLoadedFile *ndsOpeningActionPreviewLoadFile(
    const NDSOpeningActionPreviewDesc *desc)
{
    NDSRelocAssetHeader header;
    NDSRelocLoadedFile *loaded;

    if (desc == NULL)
    {
        return NULL;
    }

    ndsRelocResetLoadedFiles();
    if (ndsRelocAssetReadHeader(desc->asset_id, &header) == FALSE)
    {
        return NULL;
    }
    if (header.data_size > NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE)
    {
        return NULL;
    }
    if (ndsRelocAssetLoadData(desc->asset_id,
                              sNdsOpeningActionPreviewFileBuffer,
                              sizeof(sNdsOpeningActionPreviewFileBuffer),
                              &header) == FALSE)
    {
        return NULL;
    }

    loaded = ndsRelocRegisterLoadedFile(desc->asset_id, 0,
                                        sNdsOpeningActionPreviewFileBuffer,
                                        &header);
    if (loaded == NULL)
    {
        return NULL;
    }
    if ((ndsRelocApplyWordByteSwap(loaded) == FALSE) ||
        (ndsRelocApplyInternalPointerFixups(loaded) == FALSE))
    {
        return NULL;
    }
    return (ndsRelocNormalizeOpeningActionPreviewSprite(loaded, desc) != FALSE) ?
        loaded : NULL;
}

static s32 ndsOpeningActionPreviewRender(u32 scene_kind)
{
    const NDSOpeningActionPreviewDesc *desc =
        ndsOpeningActionPreviewDescForKind(scene_kind);
    NDSOpeningActionPreviewCache *cache;
    NDSRelocLoadedFile *loaded;
    Sprite *sprite;
    SObj sobj;

    cache = ndsOpeningActionPreviewFindCache(desc);
    if (cache == NULL)
    {
        return FALSE;
    }

    if (cache->ready == 0)
    {
        loaded = ndsOpeningActionPreviewLoadFile(desc);
        if (loaded == NULL)
        {
            return FALSE;
        }

        sprite = lbRelocGetFileData(Sprite*, loaded->data, desc->symbol);
        if (sprite == NULL)
        {
            return FALSE;
        }

        memset(cache->pixels, 0, sizeof(cache->pixels));
        memset(&sobj, 0, sizeof(sobj));
        sobj.sprite = *sprite;
        sobj.pos.x = (f32)desc->x;
        sobj.pos.y = (f32)desc->y;

        if (ndsDrawSObjIntoPreview(
                &sobj, 0, cache->pixels,
                NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH,
                NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH,
                NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT,
                desc->x, desc->y, 0u) == FALSE)
        {
            cache->ready = 0;
            return FALSE;
        }

        cache->width = desc->width;
        cache->height = desc->height;
        cache->bmfmt = desc->bmfmt;
        cache->bmsiz = desc->bmsiz;
        cache->pixel_count =
            ndsOpeningActionPreviewCountPixels(cache->pixels);
        cache->ready = 1;
    }

    if (ndsOpeningActionPreviewCommitCache(cache, scene_kind) == FALSE)
    {
        return FALSE;
    }

    gNdsOpeningMovieActionPreviewCount++;
    return TRUE;
}

static void ndsOpeningActionPreviewHoldFrames(u32 frames)
{
    u32 i;

    for (i = 0; i < frames; i++)
    {
        ndsOpeningMoviePresentFrame();
        gNdsOpeningMovieActionPreviewFrameCount++;
    }
}

static void ndsOpeningMovieBridgeTo(u32 next_scene)
{
    u32 scene_kind = gSCManagerSceneData.scene_curr;

    gNdsOpeningMovieBridgeResult = NDS_OPENING_MOVIE_BRIDGE_PASS;
    gNdsOpeningMovieBridgeMask |=
        ndsOpeningMovieBridgeMaskForKind(scene_kind);
    gNdsOpeningMovieBridgeCount++;
    gNdsOpeningMovieBridgeLastKind = scene_kind;
    gNdsOpeningMovieBridgeLastNextKind = next_scene;

    (void)ndsOpeningActionPreviewRender(scene_kind);
    ndsOpeningActionPreviewHoldFrames(NDS_OPENING_ACTION_PREVIEW_FRAME_HOLD);

    gSCManagerSceneData.scene_prev = scene_kind;
    gSCManagerSceneData.scene_curr = next_scene;
}

NDS_SCENE_STUB(dbBattleStartScene)
NDS_SCENE_STUB(dbCubeStartScene)
NDS_SCENE_STUB(dbFallsStartScene)
NDS_SCENE_STUB(dbMapsStartScene)
NDS_SCENE_STUB(mn1PModeStartScene)
NDS_SCENE_STUB(mnBackupClearStartScene)
NDS_SCENE_STUB(mnCharactersStartScene)
NDS_SCENE_STUB(mnCongraStartScene)
NDS_SCENE_STUB(mnDataStartScene)
NDS_SCENE_STUB(mnMessageStartScene)
NDS_SCENE_STUB(mnModeSelectStartScene)
NDS_SCENE_STUB(mnNoControllerStartScene)
NDS_SCENE_STUB(mnOptionStartScene)
NDS_SCENE_STUB(mnPlayers1PBonusStartScene)
NDS_SCENE_STUB(mnPlayers1PGameContinueStartScene)
NDS_SCENE_STUB(mnPlayers1PGameStartScene)
NDS_SCENE_STUB(mnPlayers1PTrainingStartScene)
NDS_SCENE_STUB(mnScreenAdjustStartScene)
NDS_SCENE_STUB(mnSoundTestStartScene)
NDS_SCENE_STUB(mnUnusedFightersStartScene)
NDS_SCENE_STUB(mnVSItemSwitchStartScene)
NDS_SCENE_STUB(mnVSOptionsStartScene)
NDS_SCENE_STUB(mnVSRecordStartScene)
#if !NDS_IMPORT_BATTLESHIP_VS_RESULTS
NDS_SCENE_STUB(mnVSResultsStartScene)
#endif
NDS_SCENE_STUB(mvEndingStartScene)
NDS_SCENE_STUB(mvUnknownMarioStartScene)
NDS_SCENE_STUB(sc1PBonusStageStartScene)
NDS_SCENE_STUB(sc1PChallengerStartScene)
NDS_SCENE_STUB(sc1PIntroStartScene)
NDS_SCENE_STUB(sc1PManagerUpdateScene)
NDS_SCENE_STUB(sc1PStageClearStartScene)
NDS_SCENE_STUB(sc1PTrainingModeStartScene)
NDS_SCENE_STUB(scAutoDemoStartScene)
NDS_SCENE_STUB(scExplainStartScene)
NDS_SCENE_STUB(scStaffrollStartScene)

void mvOpeningRunStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningCliff);
}

void mvOpeningCliffStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningYamabuki);
}

void mvOpeningYamabukiStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningJungle);
}

void mvOpeningJungleStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningYoster);
}

void mvOpeningYosterStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningSector);
}

void mvOpeningSectorStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningStandoff);
}

void mvOpeningStandoffStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningClash);
}

void mvOpeningClashStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindOpeningNewcomers);
}

void mvOpeningNewcomersStartScene(void)
{
    ndsOpeningMovieBridgeTo(nSCKindTitle);
}
