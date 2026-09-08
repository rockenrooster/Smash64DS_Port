/* Retired software fighter preview, for host reference harnesses only. */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Software fighter preview is host-only
#endif

static s32 ndsFighterPreviewLoopClampS32(s32 value, s32 min, s32 max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

static void ndsFighterPreviewLoopPlot(u16 *pixels, u32 pitch, s32 x, s32 y,
                                      u16 color, u32 *count,
                                      u32 *checksum)
{
    if ((pixels == NULL) || (x < 0) || (y < 0) ||
        (x >= (s32)NDS_FIGHTER_PREVIEW_LOOP_WIDTH) ||
        (y >= (s32)NDS_FIGHTER_PREVIEW_LOOP_HEIGHT))
    {
        return;
    }
    pixels[(u32)y * pitch + (u32)x] = color;
    if (count != NULL)
    {
        (*count)++;
    }
    if (checksum != NULL)
    {
        *checksum = (*checksum * 33u) ^ (u32)color ^
            ((u32)x << 16) ^ (u32)y;
    }
}

static void ndsFighterPreviewLoopClear(u16 *pixels, u32 pitch)
{
    u32 x;
    u32 y;
    u16 bg = ndsFighterDLDrawRGB15(6, 8, 14);
    u16 floor = ndsFighterDLDrawRGB15(70, 110, 70);

    if (pixels == NULL)
    {
        return;
    }
    for (y = 0u; y < NDS_FIGHTER_PREVIEW_LOOP_HEIGHT; y++)
    {
        for (x = 0u; x < NDS_FIGHTER_PREVIEW_LOOP_WIDTH; x++)
        {
            pixels[y * pitch + x] = bg;
        }
    }
    for (x = 0u; x < NDS_FIGHTER_PREVIEW_LOOP_WIDTH; x++)
    {
        pixels[(NDS_FIGHTER_PREVIEW_LOOP_HEIGHT - 10u) * pitch + x] = floor;
    }
}

static void ndsFighterPreviewLoopDrawSlot(u32 slot, FTStruct *fp,
                                          u16 *pixels, u32 pitch)
{
    NDSFighterPreviewLoopState *state;
    NDSFighterDLAllDrawCollection collection;
    DObj *root;
    s32 root_delta;
    s32 root_rise;
    s32 root_rise_max;
    s32 screen_x;
    s32 screen_y;
    u32 pixel_count = 0u;
    u32 checksum = 0u;
    u32 i;

    if ((slot >= 2u) || (fp == NULL) || (pixels == NULL))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    ndsFighterCollectAllDObjsWithDL(root, &collection);

    root_delta = ndsFloatToMilliSigned(root->translate.vec.f.x) -
        ((slot == 0u) ? gNdsFighterPreviewLoopP0RootXStartMilli :
            gNdsFighterPreviewLoopP1RootXStartMilli);
    root_rise = ndsFloatToMilliSigned(root->translate.vec.f.y) -
        ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    root_rise_max = ndsFloatToMilliSigned(state->root_y_max) -
        ndsFloatToMilliSigned(state->root_y_start);
    if (root_rise < root_rise_max)
    {
        root_rise = root_rise_max;
    }
    screen_x = state->screen_x_start + (root_delta / 1500);
    screen_y = state->screen_y_floor - (root_rise / 1200);
    screen_x = ndsFighterPreviewLoopClampS32(screen_x, 6, 89);
    screen_y = ndsFighterPreviewLoopClampS32(screen_y, 8, 62);

    if ((state->screen_initialized == 0u) ||
        (screen_y < state->screen_y_min))
    {
        state->screen_y_min = screen_y;
    }
    state->screen_initialized = 1u;
    state->screen_x_final = screen_x;

    for (i = 0u; i < collection.selected_count; i++)
    {
        s32 ox = (s32)(i % 5u) - 2;
        s32 oy = -3 - (s32)((i / 5u) * 3u);
        u16 color = (slot == 0u) ?
            ndsFighterDLDrawRGB15(245, 70 + ((i * 7u) & 31u), 45) :
            ndsFighterDLDrawRGB15(60, 105 + ((i * 5u) & 31u), 245);

        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox,
                                  screen_y + oy, color, &pixel_count,
                                  &checksum);
        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox - 1,
                                  screen_y + oy + 1, color, &pixel_count,
                                  &checksum);
        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox + 1,
                                  screen_y + oy + 1, color, &pixel_count,
                                  &checksum);
    }

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0CandidateCount = collection.total_count;
        gNdsFighterPreviewLoopP0DrawnDObjCount = collection.selected_count;
        gNdsFighterPreviewLoopP0PixelCount += pixel_count;
        gNdsFighterPreviewLoopP0ColorChecksum =
            (gNdsFighterPreviewLoopP0ColorChecksum * 33u) ^ checksum;
        gNdsFighterPreviewLoopP0ScreenXFinal = screen_x;
        gNdsFighterPreviewLoopP0ScreenXDelta =
            screen_x - gNdsFighterPreviewLoopP0ScreenXStart;
        gNdsFighterPreviewLoopP0ScreenYMin = state->screen_y_min;
        gNdsFighterPreviewLoopP0ScreenRise =
            gNdsFighterPreviewLoopP0ScreenYFloor - state->screen_y_min;
    }
    else
    {
        gNdsFighterPreviewLoopP1CandidateCount = collection.total_count;
        gNdsFighterPreviewLoopP1DrawnDObjCount = collection.selected_count;
        gNdsFighterPreviewLoopP1PixelCount += pixel_count;
        gNdsFighterPreviewLoopP1ColorChecksum =
            (gNdsFighterPreviewLoopP1ColorChecksum * 33u) ^ checksum;
        gNdsFighterPreviewLoopP1ScreenXFinal = screen_x;
        gNdsFighterPreviewLoopP1ScreenXDelta =
            screen_x - gNdsFighterPreviewLoopP1ScreenXStart;
        gNdsFighterPreviewLoopP1ScreenYMin = state->screen_y_min;
        gNdsFighterPreviewLoopP1ScreenRise =
            gNdsFighterPreviewLoopP1ScreenYFloor - state->screen_y_min;
    }
}

static void ndsFighterPreviewLoopDrawKeyframe(void)
{
    u32 pitch = 0u;
    u16 *pixels;

    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_PREVIEW_LOOP_WIDTH,
        NDS_FIGHTER_PREVIEW_LOOP_HEIGHT,
        &pitch);
    if (pixels == NULL)
    {
        return;
    }
    if (gNdsFighterPreviewLoopDrawFrameCount == 0u)
    {
        gNdsFighterPreviewLoopPreviewCommitBefore =
            gNdsOriginalDLPreviewCommitCount;
    }
    gNdsFighterPreviewLoopPreviewWidth = NDS_FIGHTER_PREVIEW_LOOP_WIDTH;
    gNdsFighterPreviewLoopPreviewHeight = NDS_FIGHTER_PREVIEW_LOOP_HEIGHT;
    gNdsFighterPreviewLoopPreviewPitch = pitch;
    sNdsFighterPreviewLoopPixels = pixels;
    sNdsFighterPreviewLoopPitch = pitch;
    sNdsFighterPreviewLoopDisplayActive = TRUE;
    ndsFighterPreviewLoopClear(pixels, pitch);
    ftDisplayMainProcDisplay(sNdsFighterStructPool[0].fighter_gobj);
    ftDisplayMainProcDisplay(sNdsFighterStructPool[1].fighter_gobj);
    sNdsFighterPreviewLoopDisplayActive = FALSE;
    sNdsFighterPreviewLoopPixels = NULL;
    sNdsFighterPreviewLoopPitch = 0u;

    gNdsFighterPreviewLoopTotalPixelCount =
        gNdsFighterPreviewLoopP0PixelCount +
        gNdsFighterPreviewLoopP1PixelCount;
    if (gNdsFighterPreviewLoopTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterPreviewLoopPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterPreviewLoopPreviewCommitDelta =
            gNdsFighterPreviewLoopPreviewCommitAfter -
            gNdsFighterPreviewLoopPreviewCommitBefore;
        gNdsFighterPreviewLoopPreviewReady = gNdsOriginalDLPreviewReady;
        gNdsFighterPreviewLoopDrawFrameCount++;
    }
    sNdsFighterPreviewLoopDrawFrameIndex++;
}
