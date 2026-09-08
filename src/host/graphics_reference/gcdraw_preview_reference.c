#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Software scene preview is host-only
#endif

static void ndsFighterGCDrawAllLoopDrawKeyframe(void)
{
    u32 pitch = 0u;
    u16 *pixels;
    u32 callbacks_before;

    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_GCDRAWALL_LOOP_WIDTH,
        NDS_FIGHTER_GCDRAWALL_LOOP_HEIGHT,
        &pitch);
    if (pixels == NULL)
    {
        return;
    }
    if (gNdsFighterGCDrawAllLoopDrawFrameCount == 0u)
    {
        gNdsFighterGCDrawAllLoopPreviewCommitBefore =
            gNdsOriginalDLPreviewCommitCount;
    }
    sNdsFighterGCDrawAllLoopPixels = pixels;
    sNdsFighterGCDrawAllLoopPitch = pitch;
    sNdsFighterGCDrawAllLoopDisplayActive = TRUE;
    callbacks_before = gNdsFighterGCDrawAllLoopDisplayCallbackCount;

    ndsFighterPreviewLoopClear(pixels, pitch);
    gcDrawAll();
    gNdsFighterGCDrawAllLoopDrawAllCount++;
    if (ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() != FALSE)
    {
        gNdsStageGCDrawAllLoopDrawAllCount++;
    }

    sNdsFighterGCDrawAllLoopDisplayActive = FALSE;
    sNdsFighterGCDrawAllLoopPixels = NULL;
    sNdsFighterGCDrawAllLoopPitch = 0u;

    ndsFighterGCDrawAllLoopCopyFromPreview();
    gNdsFighterGCDrawAllLoopPreviewWidth =
        NDS_FIGHTER_GCDRAWALL_LOOP_WIDTH;
    gNdsFighterGCDrawAllLoopPreviewHeight =
        NDS_FIGHTER_GCDRAWALL_LOOP_HEIGHT;
    gNdsFighterGCDrawAllLoopPreviewPitch = pitch;
    gNdsFighterGCDrawAllLoopTotalPixelCount =
        gNdsFighterGCDrawAllLoopP0PixelCount +
        gNdsFighterGCDrawAllLoopP1PixelCount;

    if ((gNdsFighterGCDrawAllLoopTotalPixelCount > 0u) &&
        (gNdsFighterGCDrawAllLoopDisplayCallbackCount > callbacks_before))
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterGCDrawAllLoopPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterGCDrawAllLoopPreviewCommitDelta =
            gNdsFighterGCDrawAllLoopPreviewCommitAfter -
            gNdsFighterGCDrawAllLoopPreviewCommitBefore;
        gNdsFighterGCDrawAllLoopPreviewReady =
            gNdsOriginalDLPreviewReady;
        gNdsFighterGCDrawAllLoopDrawFrameCount++;
    }
}
