/* Host-only source menu fill compositor; included by reference harnesses. */
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Source menu graphics interpretation is host-only
#endif

/* Same main-RAM/cursor discipline as the effect span scanner
 * (renderer_adapter_stage.c:ndsRendererAdapterDisplayProcSpanValid): heads
 * outside main RAM are residue on targets that never present, never spans. */
static u32 ndsMenuFillSpanValid(const Gfx *cursor, const Gfx *end)
{
    return (((uintptr_t)cursor >= 0x02000000u) &&
            ((uintptr_t)end >= 0x02000000u) &&
            (cursor <= end)) ? TRUE : FALSE;
}

/* RGBA5551 (r[15:11] g[10:6] b[5:1] a[0], mbi.h GPACK_RGBA5551) to opaque
 * DS BGR555. Opaque by RDP FILL semantics, not by the alpha bit. */
static u16 ndsMenuFillPackRgba5551(u16 n64_color)
{
    u16 red = (u16)((n64_color >> 11) & 0x1fu);
    u16 green = (u16)((n64_color >> 6) & 0x1fu);
    u16 blue = (u16)((n64_color >> 1) & 0x1fu);

    return (u16)(0x8000u | red | (green << 5) | (blue << 10));
}

/* Source framebuffer pixels to 320x240 staging pixels. Identity at 320x240
 * (ScreenAdjust, SYVIDEO_SETUP_DEFAULT), halved at 640x480 (credits,
 * dSCStaffrollVideoSetup 640x480; SObj centers at 320,240 prove the space in
 * scStaffrollCheckCursorNameOverlap). Truncation (never floor) past .5 is a
 * sub-pixel delta on odd 640-space bounds, clipped below regardless. */
static s32 ndsMenuFillMapAxis(s32 src, s32 res, s32 stage)
{
    if (res <= 0)
    {
        res = stage;
    }
    /* Magnitudes stay under 1024*320: no 64-bit step needed. */
    return (src * stage) / res;
}

/* The one staging both menu paths share. Prefers the already-open frame
 * layer; adopts the glyph layer when the glyph proc opened first (same
 * shared scratch, so Beginning again would clear its glyphs); opens fresh
 * only when neither is up. */
static u32 ndsMenuFillStagingEnsure(void)
{
    if ((sNdsSObjFramePreview != NULL) &&
        (sNdsSObjFramePreviewPitch != 0u))
    {
        return TRUE;
    }
    if ((sNdsStaffrollPreview != NULL) &&
        (sNdsStaffrollPreviewPitch != 0u) &&
        (sNdsStaffrollPreviewFrame == gNdsFrameCounter))
    {
        sNdsSObjFramePreview = sNdsStaffrollPreview;
        sNdsSObjFramePreviewPitch = sNdsStaffrollPreviewPitch;
        return TRUE;
    }
    ndsSObjPreviewBeginStagingLayer();
    return ((sNdsSObjFramePreview != NULL) &&
            (sNdsSObjFramePreviewPitch != 0u)) ? TRUE : FALSE;
}

/* Inclusive [ulx,lrx]x[uly,lry] in source pixels, opaque stores, staging
 * clipped. Opens staging lazily so a fill-only frame still commits. */
static void ndsMenuFillBlitRect(s32 ulx, s32 uly, s32 lrx, s32 lry,
                                u16 color)
{
    s32 res_w = (s32)gSYVideoResWidth;
    s32 res_h = (s32)gSYVideoResHeight;
    s32 x0 = ndsMenuFillMapAxis(ulx, res_w, 320);
    s32 y0 = ndsMenuFillMapAxis(uly, res_h, 240);
    s32 x1 = ndsMenuFillMapAxis(lrx, res_w, 320);
    s32 y1 = ndsMenuFillMapAxis(lry, res_h, 240);
    s32 dst_x;
    s32 dst_y;
    u16 *preview;
    u32 pitch;

    if (x0 < 0) { x0 = 0; }
    if (y0 < 0) { y0 = 0; }
    if (x1 > 319) { x1 = 319; }
    if (y1 > 239) { y1 = 239; }
    if ((x1 < x0) || (y1 < y0))
    {
        return;
    }
    if (ndsMenuFillStagingEnsure() == FALSE)
    {
        return;
    }
    preview = sNdsSObjFramePreview;
    pitch = sNdsSObjFramePreviewPitch;
    for (dst_y = y0; dst_y <= y1; dst_y++)
    {
        for (dst_x = x0; dst_x <= x1; dst_x++)
        {
            preview[((u32)dst_y * pitch) + (u32)dst_x] = color;
            gNdsMenuFillPixelCount++;
        }
    }
    sNdsSObjFramePreviewDrawCount++;
    gNdsMenuFillRectCount++;
}

/* One DL word folded into the RDP track state, or composited. Opcodes are
 * the F3DEX2 encodings the macros emit (PR/gbi.h): cycle w0 0xe3000a01 with
 * G_CYC_FILL in w1, SETFILLCOLOR 0xf7, SETPRIMCOLOR 0xfa, FILLRECT 0xf6 with
 * lrx/lry in w0 and ulx/uly in w1 (10-bit fields). Zeroed stubs (PipeSync,
 * render mode, segments) decode as opcode 0 and skip. */
static void ndsMenuFillSinkFoldWord(u32 w0, u32 w1)
{
    u32 op = w0 >> 24;

    if ((op == 0xe3u) && (w0 == 0xe3000a01u))
    {
        sNdsMenuFillCycleIsFill = (w1 == (u32)G_CYC_FILL) ? TRUE : FALSE;
    }
    else if (op == (u32)G_SETFILLCOLOR)
    {
        sNdsMenuFillColor = (u16)(w1 & 0xffffu);
    }
    else if (op == 0xfau)
    {
        sNdsMenuFillPrim = w1;
    }
    else if (op == (u32)G_FILLRECT)
    {
        s32 ulx = (s32)((w1 >> 14) & 0x3ffu);
        s32 uly = (s32)((w1 >> 2) & 0x3ffu);
        s32 lrx = (s32)((w0 >> 14) & 0x3ffu);
        s32 lry = (s32)((w0 >> 2) & 0x3ffu);
        u16 color;

        if (sNdsMenuFillCycleIsFill != FALSE)
        {
            color = ndsMenuFillPackRgba5551(sNdsMenuFillColor);
        }
        else
        {
            color = ndsSpritePackRgb15((u8)(sNdsMenuFillPrim >> 24),
                                       (u8)(sNdsMenuFillPrim >> 16),
                                       (u8)(sNdsMenuFillPrim >> 8));
        }
        ndsMenuFillBlitRect(ulx, uly, lrx, lry, color);
    }
}

static void ndsMenuFillSinkDrainRuntime(void)
{
    const Gfx *cursor;
    const Gfx *end;

    if (ndsMenuFillSinkSceneGated() == FALSE)
    {
        return;
    }
    cursor = sNdsMenuFillDrainMark;
    end = gSYTaskmanDLHeads[0];
    if ((sNdsMenuFillDrainMarkValid == FALSE) ||
        (ndsMenuFillSpanValid(cursor, end) == FALSE))
    {
        sNdsMenuFillDrainMark = end;
        sNdsMenuFillDrainMarkValid = TRUE;
        return;
    }
    /* No cap: the span is the bound, and every rect composites on arrival,
     * so no queue can overflow. Spans here are one proc's words. */
    while (cursor < end)
    {
        ndsMenuFillSinkFoldWord(cursor->words.w0, cursor->words.w1);
        cursor++;
    }
    sNdsMenuFillDrainMark = end;
}

#if NDS_P2_1P_GAME
/* The textbox frame never reaches a DL head: it is a static DL on a DObj
 * the recorder does not execute. Coordinates still come from the source
 * words, not a table; the count is the DL's own length
 * (scstaffroll.c:476-487: sync, cycle, rendermode, fillcolor, 4 rects,
 * end = 9). Runs after the runtime drain: the DL sits at display link 9,
 * past the highlight (8). */
extern Gfx dSCStaffrollTextBoxDisplayList[9];

static void ndsMenuFillSinkScanStatic(const Gfx *dl, u32 count)
{
    u32 i;

    if (dl == NULL)
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        ndsMenuFillSinkFoldWord(dl[i].words.w0, dl[i].words.w1);
    }
}
#endif

static void ndsMenuFillSinkEndFrame(void)
{
    if (ndsMenuFillSinkSceneGated() == FALSE)
    {
        return;
    }
    ndsMenuFillSinkDrainRuntime();
#if NDS_P2_1P_GAME
    if (gSCManagerSceneData.scene_curr == nSCKindStaffroll)
    {
        ndsMenuFillSinkScanStatic(dSCStaffrollTextBoxDisplayList, 9u);
    }
#endif
}
