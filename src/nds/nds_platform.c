#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_boot.h>
#include <nds/nds_controller.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_scene.h>
#include <nds/nds_startup.h>
#include <nds/nds_video.h>
#include <sys/controller.h>

#ifndef NDS_RENDERER_HW_TRIANGLES
#define NDS_RENDERER_HW_TRIANGLES 0
#endif

#ifndef NDS_DEBUG_HUD
#define NDS_DEBUG_HUD 1
#endif

extern volatile u32 gNdsBootSelfTestResult;
extern volatile u32 gNdsFrameCounter;

#define NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH 320u
#define NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT 240u
#define NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH 96u
#define NDS_ORIGINAL_DL_PREVIEW_MAX_HEIGHT 72u
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH 72u
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT 54u
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_X 0
#define NDS_ORIGINAL_DL_PREVIEW_DISPLAY_Y 128
#define NDS_ORIGINAL_DL_PREVIEW_BORDER_COLOR RGB15(26, 18, 0)
#define NDS_ORIGINAL_DL_PREVIEW_BG_COLOR RGB15(2, 2, 2)
#define NDS_N64_LOGICAL_WIDTH 320
#define NDS_N64_LOGICAL_HEIGHT 240
#define NDS_TOP_BACKGROUND_COLOR (RGB15(2, 3, 6) | BIT(15))
#define NDS_PERF_SAMPLE_TICKS 60u

#if !NDS_RENDERER_HW_TRIANGLES
static u16 *sFramebuffer;
static u16 *sFramebuffers[2];
static u32 sDrawFramebufferIndex;
#endif
static u32 sTicks;
static u32 sHeldKeys;
volatile u32 gNdsPlatformHeldKeys;
static u32 sPerfSampleReady;
static u32 sPerfLastTick;
static u32 sPerfLastFrameCounter;
static u32 sPerfLastLogicTickCount;
static u32 sPerfLastDLPreviewDrawCount;
static u32 sPerfLastPreviewCommitCount;
static u16 sOriginalSpritePreview[
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT];
static u32 sOriginalSpritePreviewWidth;
static u32 sOriginalSpritePreviewHeight;
static s32 sOriginalSpritePreviewX;
static s32 sOriginalSpritePreviewY;
static u32 sOriginalSpritePreviewReady;
#if NDS_RENDERER_HW_TRIANGLES
static int sOriginalSpriteOverlayBg = -1;
static int sOriginalSpriteOverlayForegroundBg = -1;
static s32 sOriginalSpriteOverlayEnabled;
static s32 sOriginalSpriteOverlayNeedsFlush;
#endif
static u16 sOriginalSpriteDisplayPreview[
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT];
static u32 sOriginalSpriteDisplayPreviewWidth;
static u32 sOriginalSpriteDisplayPreviewHeight;
static u16 sOriginalDLPreview[
    NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH *
    NDS_ORIGINAL_DL_PREVIEW_MAX_HEIGHT];
static u16 sOriginalDLDisplayPreview[
    NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH *
    NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT];
static u32 sOriginalDLPreviewWidth;
static u32 sOriginalDLPreviewHeight;
static u32 sOriginalDLDisplayPreviewWidth;
static u32 sOriginalDLDisplayPreviewHeight;
static u32 sOriginalDLPreviewReady;
static u32 sDebugTextFingerprint = 0xffffffffu;
static u32 sDebugTextReady;

volatile u32 gNdsOriginalSpritePreviewReady;
volatile u32 gNdsOriginalSpritePreviewCommitCount;
volatile u32 gNdsOriginalSpritePreviewDrawCount;
/* These retain the last committed frame while a new scratch layer is built. */
volatile u32 gNdsOriginalSpritePreviewDisplayWidth;
volatile u32 gNdsOriginalSpritePreviewDisplayHeight;
volatile u32 gNdsOriginalDLPreviewReady;
volatile u32 gNdsOriginalDLPreviewWidth;
volatile u32 gNdsOriginalDLPreviewHeight;
volatile u32 gNdsOriginalDLPreviewCommitCount;
volatile u32 gNdsOriginalDLPreviewDrawCount;
volatile u32 gNdsPerfPresentFps;
volatile u32 gNdsPerfLogicFps;
volatile u32 gNdsPerfDLDrawFps;
volatile u32 gNdsPerfPreviewCommitFps;
volatile u32 gNdsPerfPreviewCommitCount;
volatile u32 gNdsPerfSampleCount;
volatile u32 gNdsPerfSampleWindowTicks;
volatile u32 gNdsHardwareRendererSubmittedFrameCount;
volatile u32 gNdsHardwareRendererFlushCount;
volatile u32 gNdsHardwareRendererPolyRamCount;
volatile u32 gNdsHardwareRendererVertexRamCount;
volatile u32 gNdsHardwareRendererStatus;
volatile u32 gNdsHardwareRendererControl;

void ndsPlatformInit(void)
{
    /* libultra time/count are sub-frame hardware counters. Reserve timers 0/1
     * for libnds' 32-bit CPU timing source before original code can sample it. */
    cpuStartTiming(0);

#if NDS_RENDERER_HW_TRIANGLES
    videoSetMode(MODE_5_3D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankE(VRAM_E_TEX_PALETTE);
    glInit();
    glClearColor(2, 3, 6, 31);
    glClearDepth(GL_MAX_DEPTH);
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glViewport(0, 0, 255, 191);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    vramSetBankC(VRAM_C_MAIN_BG_0x06000000);
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    sOriginalSpriteOverlayBg =
        bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    sOriginalSpriteOverlayForegroundBg =
        bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 8, 0);
    bgSetPriority(sOriginalSpriteOverlayForegroundBg, 0);
    bgSetPriority(0, 1);
    bgSetPriority(sOriginalSpriteOverlayBg, 2);
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_DST_BG2;
    REG_BLDALPHA = 16u | (16u << 8);
    dmaFillHalfWords(0, bgGetGfxPtr(sOriginalSpriteOverlayBg),
                     256u * 256u * sizeof(u16));
    dmaFillHalfWords(0, bgGetGfxPtr(sOriginalSpriteOverlayForegroundBg),
                     256u * 256u * sizeof(u16));
#else
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    sFramebuffers[0] = VRAM_A;
    sFramebuffers[1] = VRAM_B;
    sDrawFramebufferIndex = 1;
    sFramebuffer = sFramebuffers[sDrawFramebufferIndex];
    dmaFillHalfWords(NDS_TOP_BACKGROUND_COLOR, sFramebuffers[0],
                     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
    dmaFillHalfWords(NDS_TOP_BACKGROUND_COLOR, sFramebuffers[1],
                     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
#endif

    videoSetModeSub(MODE_0_2D);
    vramSetBankH(VRAM_H_SUB_BG);
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);

#if NDS_DEBUG_HUD
    iprintf("\x1b[?25l");
    iprintf("Smash 64 DS Port\n");
    iprintf("================\n");
    iprintf("melonDS visual debug active\n");
#endif
}

u32 ndsPlatformReadInput(void)
{
    u32 input = 0;
    u32 held;

    scanKeys();
    held = keysHeld();
    sHeldKeys = held;
    gNdsPlatformHeldKeys = held;

    if (held & KEY_LEFT) input |= NDS_INPUT_LEFT;
    if (held & KEY_RIGHT) input |= NDS_INPUT_RIGHT;
    if (held & KEY_UP) input |= NDS_INPUT_UP;
    if (held & KEY_DOWN) input |= NDS_INPUT_DOWN;
    if (held & KEY_A) input |= NDS_INPUT_A;
    if (held & KEY_START) input |= NDS_INPUT_START;
    if (held & KEY_B) input |= NDS_INPUT_B;
    if (held & KEY_X) input |= NDS_INPUT_X;
    if (held & KEY_Y) input |= NDS_INPUT_Y;
    if (held & KEY_L) input |= NDS_INPUT_L;
    if (held & KEY_R) input |= NDS_INPUT_R;

    return input;
}

void ndsPlatformBeginFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    return;
#else
    dmaFillHalfWords(NDS_TOP_BACKGROUND_COLOR, sFramebuffer,
                     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
#endif
}

void ndsPlatformDrawRect(s32 x, s32 y, s32 width, s32 height, u16 color)
{
#if NDS_RENDERER_HW_TRIANGLES
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
#else
    s32 row;
    s32 column;

    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > SCREEN_WIDTH) width = SCREEN_WIDTH - x;
    if (y + height > SCREEN_HEIGHT) height = SCREEN_HEIGHT - y;
    if (width <= 0 || height <= 0) return;

    color |= BIT(15);
    for (row = 0; row < height; row++)
    {
        u16 *dst = sFramebuffer + ((y + row) * SCREEN_WIDTH) + x;
        for (column = 0; column < width; column++)
        {
            dst[column] = color;
        }
    }
#endif
}

u16 *ndsPlatformBeginOriginalSpritePreview(u32 width, u32 height,
                                           s32 n64_x, s32 n64_y,
                                           u32 *out_pitch)
{
    u32 row;

    if ((width == 0) || (height == 0) ||
        (width > NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH) ||
        (height > NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT))
    {
        return NULL;
    }

    sOriginalSpritePreviewWidth = width;
    sOriginalSpritePreviewHeight = height;
    sOriginalSpritePreviewX = n64_x;
    sOriginalSpritePreviewY = n64_y;
    sOriginalSpritePreviewReady = 0;
    sOriginalSpriteDisplayPreviewWidth = 0;
    sOriginalSpriteDisplayPreviewHeight = 0;
    gNdsOriginalSpritePreviewReady = 0;
    for (row = 0; row < height; row++)
    {
        memset(&sOriginalSpritePreview[
                   row * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH],
               0, width * sizeof(sOriginalSpritePreview[0]));
    }

    if (out_pitch != NULL)
    {
        *out_pitch = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH;
    }
    return sOriginalSpritePreview;
}

static void ndsPlatformCopyOriginalSpritePreviewNative(s32 dst_w, s32 dst_h)
{
    s32 y;

    for (y = 0; y < dst_h; y++)
    {
        memcpy(&sOriginalSpriteDisplayPreview[
                   y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH],
               &sOriginalSpritePreview[
                   y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH],
               (size_t)dst_w * sizeof(sOriginalSpriteDisplayPreview[0]));
    }
}

static void ndsPlatformScaleOriginalSpritePreviewNearest(s32 dst_w, s32 dst_h)
{
    u32 step_x;
    u32 step_y;
    u32 src_y_q16;
    s32 y;

    if ((dst_w <= 0) || (dst_h <= 0))
    {
        return;
    }

    step_x = (sOriginalSpritePreviewWidth << 16) / (u32)dst_w;
    step_y = (sOriginalSpritePreviewHeight << 16) / (u32)dst_h;
    src_y_q16 = step_y >> 1;

    for (y = 0; y < dst_h; y++)
    {
        u32 src_y = src_y_q16 >> 16;
        u32 src_x_q16 = step_x >> 1;
        u16 *dst = &sOriginalSpriteDisplayPreview[
            y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH];
        const u16 *src;
        s32 x;

        if (src_y >= sOriginalSpritePreviewHeight)
        {
            src_y = sOriginalSpritePreviewHeight - 1u;
        }
        src = &sOriginalSpritePreview[
            src_y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH];

        for (x = 0; x < dst_w; x++)
        {
            u32 src_x = src_x_q16 >> 16;

            if (src_x >= sOriginalSpritePreviewWidth)
            {
                src_x = sOriginalSpritePreviewWidth - 1u;
            }
            dst[x] = src[src_x];
            src_x_q16 += step_x;
        }
        src_y_q16 += step_y;
    }
}

void ndsPlatformCommitOriginalSpritePreviewLayer(s32 is_foreground)
{
    s32 dst_w;
    s32 dst_h;

    if ((sOriginalSpritePreviewWidth == 0) ||
        (sOriginalSpritePreviewHeight == 0))
    {
        return;
    }

    /*
     * Keep the retained visual diagnostic at native asset resolution. The N64
     * logical position still maps to the DS screen when drawing, but the copy
     * itself stays 128x108 so melonDS captures are useful for texture work.
     */
    dst_w = (s32)sOriginalSpritePreviewWidth;
    dst_h = (s32)sOriginalSpritePreviewHeight;
    if (dst_w <= 0) dst_w = 1;
    if (dst_h <= 0) dst_h = 1;
    if (dst_w > SCREEN_WIDTH)
    {
        dst_w = SCREEN_WIDTH;
    }
    if (dst_h > SCREEN_HEIGHT)
    {
        dst_h = SCREEN_HEIGHT;
    }
    if (dst_w > (s32)NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH)
    {
        dst_w = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH;
    }
    if (dst_h > (s32)NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT)
    {
        dst_h = NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT;
    }

    if ((dst_w == (s32)sOriginalSpritePreviewWidth) &&
        (dst_h == (s32)sOriginalSpritePreviewHeight))
    {
        ndsPlatformCopyOriginalSpritePreviewNative(dst_w, dst_h);
    }
    else
    {
        ndsPlatformScaleOriginalSpritePreviewNearest(dst_w, dst_h);
    }
    sOriginalSpriteDisplayPreviewWidth = (u32)dst_w;
    sOriginalSpriteDisplayPreviewHeight = (u32)dst_h;
    sOriginalSpritePreviewReady = 1;
    gNdsOriginalSpritePreviewReady = 1;
    gNdsOriginalSpritePreviewDisplayWidth = (u32)dst_w;
    gNdsOriginalSpritePreviewDisplayHeight = (u32)dst_h;
    gNdsOriginalSpritePreviewCommitCount++;

#if NDS_RENDERER_HW_TRIANGLES
    if (sOriginalSpriteOverlayEnabled != FALSE)
    {
        int bg = (is_foreground != FALSE) ?
            sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;
        u16 *overlay;
        s32 y;

        if (bg < 0)
        {
            return;
        }
        overlay = (u16 *)bgGetGfxPtr(bg);
        dmaFillHalfWords(0, overlay, 256u * 256u * sizeof(u16));
        for (y = 0; y < dst_h; y++)
        {
            memcpy(&overlay[y * 256],
                   &sOriginalSpriteDisplayPreview[
                       y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH],
                   (size_t)dst_w * sizeof(u16));
        }
    }
#endif
}

void ndsPlatformCommitOriginalSpritePreview(void)
{
    ndsPlatformCommitOriginalSpritePreviewLayer(FALSE);
}

void ndsPlatformClearOriginalSpriteOverlayLayer(s32 is_foreground)
{
#if NDS_RENDERER_HW_TRIANGLES
    int bg = (is_foreground != FALSE) ?
        sOriginalSpriteOverlayForegroundBg : sOriginalSpriteOverlayBg;

    if (bg >= 0)
    {
        dmaFillHalfWords(0, bgGetGfxPtr(bg),
                         256u * 256u * sizeof(u16));
    }
#else
    (void)is_foreground;
#endif
}

void ndsPlatformSetOriginalSpriteOverlayEnabled(s32 is_enabled)
{
#if NDS_RENDERER_HW_TRIANGLES
    if ((is_enabled != FALSE) &&
        (sOriginalSpriteOverlayEnabled == FALSE))
    {
        sOriginalSpriteOverlayNeedsFlush = TRUE;
    }
    sOriginalSpriteOverlayEnabled = is_enabled;
    glClearColor(2, 3, 6, (is_enabled != FALSE) ? 0 : 31);
    if ((is_enabled == FALSE) && (sOriginalSpriteOverlayBg >= 0))
    {
        ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
        ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
    }
#else
    (void)is_enabled;
#endif
}

void ndsPlatformClearOriginalSpritePreview(void)
{
    sOriginalSpritePreviewWidth = 0;
    sOriginalSpritePreviewHeight = 0;
    sOriginalSpritePreviewX = 0;
    sOriginalSpritePreviewY = 0;
    sOriginalSpritePreviewReady = 0;
    sOriginalSpriteDisplayPreviewWidth = 0;
    sOriginalSpriteDisplayPreviewHeight = 0;
    gNdsOriginalSpritePreviewReady = 0;
    memset(sOriginalSpritePreview, 0, sizeof(sOriginalSpritePreview));
    memset(sOriginalSpriteDisplayPreview, 0,
           sizeof(sOriginalSpriteDisplayPreview));
}

u16 *ndsPlatformBeginOriginalDLPreview(u32 width, u32 height, u32 *out_pitch)
{
    if ((width == 0) || (height == 0) ||
        (width > NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH) ||
        (height > NDS_ORIGINAL_DL_PREVIEW_MAX_HEIGHT))
    {
        return NULL;
    }

    sOriginalDLPreviewWidth = width;
    sOriginalDLPreviewHeight = height;
    sOriginalDLDisplayPreviewWidth = 0;
    sOriginalDLDisplayPreviewHeight = 0;
    sOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewWidth = width;
    gNdsOriginalDLPreviewHeight = height;
    memset(sOriginalDLPreview, 0, sizeof(sOriginalDLPreview));

    if (out_pitch != NULL)
    {
        *out_pitch = NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH;
    }
    return sOriginalDLPreview;
}

static void ndsPlatformBuildOriginalDLDisplayPreview(void)
{
    u32 dst_w = sOriginalDLPreviewWidth;
    u32 dst_h = sOriginalDLPreviewHeight;
    u32 y;

    if (dst_w > NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH)
    {
        dst_w = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH;
    }
    if (dst_h > NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT)
    {
        dst_h = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_HEIGHT;
    }

    for (y = 0; y < dst_h; y++)
    {
        u32 src_y = (y * sOriginalDLPreviewHeight) / dst_h;
        u16 *dst = &sOriginalDLDisplayPreview[
            y * NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH];
        u32 x;

        for (x = 0; x < dst_w; x++)
        {
            u32 src_x = (x * sOriginalDLPreviewWidth) / dst_w;
            u16 color = sOriginalDLPreview[
                (src_y * NDS_ORIGINAL_DL_PREVIEW_MAX_WIDTH) + src_x];

            dst[x] = (color != 0) ?
                color :
                (u16)(NDS_ORIGINAL_DL_PREVIEW_BG_COLOR | BIT(15));
        }
    }

    sOriginalDLDisplayPreviewWidth = dst_w;
    sOriginalDLDisplayPreviewHeight = dst_h;
}

void ndsPlatformCommitOriginalDLPreview(void)
{
    if ((sOriginalDLPreviewWidth != 0) && (sOriginalDLPreviewHeight != 0))
    {
        ndsPlatformBuildOriginalDLDisplayPreview();
        sOriginalDLPreviewReady = 1;
        gNdsOriginalDLPreviewReady = 1;
        gNdsOriginalDLPreviewCommitCount++;
    }
}

void ndsPlatformClearOriginalDLPreview(void)
{
    sOriginalDLPreviewWidth = 0;
    sOriginalDLPreviewHeight = 0;
    sOriginalDLDisplayPreviewWidth = 0;
    sOriginalDLDisplayPreviewHeight = 0;
    sOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewReady = 0;
    gNdsOriginalDLPreviewWidth = 0;
    gNdsOriginalDLPreviewHeight = 0;
    memset(sOriginalDLPreview, 0, sizeof(sOriginalDLPreview));
}

#if !NDS_RENDERER_HW_TRIANGLES
static void ndsPlatformDrawOriginalSpritePreview(void)
{
    s32 dst_x;
    s32 dst_y;
    s32 dst_w;
    s32 dst_h;
    s32 y;

    if ((sOriginalSpritePreviewReady == 0) ||
        (sOriginalSpritePreviewWidth == 0) ||
        (sOriginalSpritePreviewHeight == 0))
    {
        return;
    }

    dst_x = (sOriginalSpritePreviewX * SCREEN_WIDTH) / NDS_N64_LOGICAL_WIDTH;
    dst_y = (sOriginalSpritePreviewY * SCREEN_HEIGHT) / NDS_N64_LOGICAL_HEIGHT;
    dst_w = (s32)sOriginalSpriteDisplayPreviewWidth;
    dst_h = (s32)sOriginalSpriteDisplayPreviewHeight;

    if (dst_w <= 0) dst_w = 1;
    if (dst_h <= 0) dst_h = 1;
    if ((dst_x + dst_w) > SCREEN_WIDTH)
    {
        dst_x = SCREEN_WIDTH - dst_w;
    }
    if ((dst_y + dst_h) > SCREEN_HEIGHT)
    {
        dst_y = SCREEN_HEIGHT - dst_h;
    }
    if (dst_x < 0) dst_x = 0;
    if (dst_y < 0) dst_y = 0;

    gNdsOriginalSpritePreviewDrawCount++;
    for (y = 0; y < dst_h; y++)
    {
        s32 screen_y = dst_y + y;
        s32 x;

        if ((screen_y < 0) || (screen_y >= SCREEN_HEIGHT))
        {
            continue;
        }

        for (x = 0; x < dst_w; x++)
        {
            s32 screen_x = dst_x + x;
            u16 color;

            if ((screen_x < 0) || (screen_x >= SCREEN_WIDTH))
            {
                continue;
            }

            color = sOriginalSpriteDisplayPreview[
                (y * NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH) + x];
            if (color != 0)
            {
                sFramebuffer[(screen_y * SCREEN_WIDTH) + screen_x] = color;
            }
        }
    }
}

static void ndsPlatformDrawOriginalDLPreview(void)
{
    const s32 dst_x = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_X;
    const s32 dst_y = NDS_ORIGINAL_DL_PREVIEW_DISPLAY_Y;
    u32 dst_w;
    u32 dst_h;
    s32 y;

    if ((gNdsOriginalDLPreviewReady == 0) ||
        (gNdsOriginalDLPreviewWidth == 0) ||
        (gNdsOriginalDLPreviewHeight == 0) ||
        (sOriginalDLDisplayPreviewWidth == 0) ||
        (sOriginalDLDisplayPreviewHeight == 0))
    {
        return;
    }

    dst_w = sOriginalDLDisplayPreviewWidth;
    dst_h = sOriginalDLDisplayPreviewHeight;

    gNdsOriginalDLPreviewDrawCount++;
    ndsPlatformDrawRect(dst_x - 2, dst_y - 2,
                        (s32)dst_w + 4,
                        (s32)dst_h + 4,
                        NDS_ORIGINAL_DL_PREVIEW_BORDER_COLOR);
    ndsPlatformDrawRect(dst_x - 1, dst_y - 1,
                        (s32)dst_w + 2,
                        (s32)dst_h + 2,
                        NDS_ORIGINAL_DL_PREVIEW_BG_COLOR);

    for (y = 0; y < (s32)dst_h; y++)
    {
        memcpy(&sFramebuffer[((dst_y + y) * SCREEN_WIDTH) + dst_x],
               &sOriginalDLDisplayPreview[
                   (u32)y * NDS_ORIGINAL_DL_PREVIEW_DISPLAY_WIDTH],
               dst_w * sizeof(sOriginalDLDisplayPreview[0]));
    }
}
#endif

static void ndsPlatformPrintDebugLine(u32 row, const char *format, ...)
{
    char line[32];
    va_list args;

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    line[sizeof(line) - 1] = '\0';
    iprintf("\x1b[%lu;0H%-31s", (unsigned long)row, line);
}

static u32 ndsPlatformMixDebugValue(u32 hash, u32 value)
{
    hash ^= value;
    return hash * 16777619u;
}

static u32 ndsPlatformScaleToFps(u32 delta, u32 elapsed_ticks)
{
    if (elapsed_ticks == 0)
    {
        return 0;
    }
    return ((delta * 60u) + (elapsed_ticks / 2u)) / elapsed_ticks;
}

static u32 ndsPlatformOpeningMovieLogicTickCount(void)
{
    return gNdsOpeningRoomTickCount +
           gNdsOpeningPortraitsTickCount +
           gNdsOpeningMarioTickCount +
           gNdsOpeningMovieActionPreviewFrameCount;
}

static u32 ndsPlatformPreviewCommitCount(void)
{
    return gNdsOriginalSpritePreviewCommitCount +
           gNdsOriginalDLPreviewCommitCount;
}

static void ndsPlatformUpdatePerfCounters(void)
{
    u32 now_tick = sTicks;
    u32 logic_tick_count = ndsPlatformOpeningMovieLogicTickCount();
    u32 preview_commit_count = ndsPlatformPreviewCommitCount();
    u32 elapsed_ticks;

    if (sPerfSampleReady == 0)
    {
        sPerfSampleReady = 1;
        sPerfLastTick = now_tick;
        sPerfLastFrameCounter = gNdsFrameCounter;
        sPerfLastLogicTickCount = logic_tick_count;
        sPerfLastDLPreviewDrawCount = gNdsOriginalDLPreviewDrawCount;
        sPerfLastPreviewCommitCount = preview_commit_count;
        return;
    }

    elapsed_ticks = now_tick - sPerfLastTick;
    if (elapsed_ticks < NDS_PERF_SAMPLE_TICKS)
    {
        return;
    }

    gNdsPerfPresentFps = ndsPlatformScaleToFps(
        gNdsFrameCounter - sPerfLastFrameCounter,
        elapsed_ticks);
    gNdsPerfLogicFps = ndsPlatformScaleToFps(
        logic_tick_count - sPerfLastLogicTickCount,
        elapsed_ticks);
    gNdsPerfDLDrawFps = ndsPlatformScaleToFps(
        gNdsOriginalDLPreviewDrawCount - sPerfLastDLPreviewDrawCount,
        elapsed_ticks);
    gNdsPerfPreviewCommitFps = ndsPlatformScaleToFps(
        preview_commit_count - sPerfLastPreviewCommitCount,
        elapsed_ticks);
    gNdsPerfPreviewCommitCount = preview_commit_count;
    gNdsPerfSampleWindowTicks = elapsed_ticks;
    gNdsPerfSampleCount++;

    sPerfLastTick = now_tick;
    sPerfLastFrameCounter = gNdsFrameCounter;
    sPerfLastLogicTickCount = logic_tick_count;
    sPerfLastDLPreviewDrawCount = gNdsOriginalDLPreviewDrawCount;
    sPerfLastPreviewCommitCount = preview_commit_count;
}

static u32 ndsPlatformOpeningHudTickMilestone(void)
{
    u32 tick = gNdsOpeningRoomTickCount;

    if ((gNdsOpeningRoomDrawResult == NDS_OPENING_ROOM_DRAW_PASS) ||
        (tick >= 560u))
    {
        return 560u;
    }
    if ((gNdsOpeningRoomTick500RunResult ==
         NDS_OPENING_ROOM_TICK500_RUN_PASS) ||
        (tick >= 500u))
    {
        return 500u;
    }
    if ((gNdsOpeningRoomTick450RunResult ==
         NDS_OPENING_ROOM_TICK450_RUN_PASS) ||
        (tick >= 450u))
    {
        return 450u;
    }
    if ((gNdsOpeningRoomTick380DeferredResult ==
         NDS_OPENING_ROOM_TICK380_DEFER_PASS) ||
        (tick >= 380u))
    {
        return 380u;
    }
    if ((gNdsOpeningRoomFirstEventRunResult ==
         NDS_OPENING_ROOM_FIRST_EVENT_RUN_PASS) ||
        (tick >= 280u))
    {
        return 280u;
    }
    if (gNdsOpeningRoomFuncStartResult == NDS_OPENING_ROOM_FUNC_START_PASS)
    {
        return 1u;
    }
    return 0u;
}

static u32 ndsPlatformDebugTextFingerprint(void)
{
    u32 hash = 2166136261u;

    hash = ndsPlatformMixDebugValue(hash, gNdsOriginalBootStage);
    hash = ndsPlatformMixDebugValue(hash, gNdsBootSelfTestResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsSceneBoundaryResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsSceneBoundaryKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsTaskmanReturnCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTaskmanBridgeResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTaskmanCleanupResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsRelocAssetInitResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocFileMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocHeaderMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocPayloadMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocWordSwapMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocWordSwapCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomRelocPointerFixupMask);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomRelocPointerFixupCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomFirstEventDeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick380DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick450DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick500DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomTick560DeferredMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomSpotlightCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomScene1CameraCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomScene2CameraCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomLogoCameraCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDeskCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomHazeCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomOutsideCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomSunlightCreateMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomPencilsCreateResult);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomFighterDeferredKind & 0xffu);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewTriangleCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewPixelCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDLPreviewTextureMask);
    hash = ndsPlatformMixDebugValue(hash, ndsPlatformOpeningHudTickMilestone());
    hash = ndsPlatformMixDebugValue(hash, sHeldKeys & 0xfffu);
    hash = ndsPlatformMixDebugValue(hash, gNdsControllerLivePad0Button);
    hash = ndsPlatformMixDebugValue(hash, (u32)gNdsControllerLivePad0StickX);
    hash = ndsPlatformMixDebugValue(hash, (u32)gNdsControllerLivePad0StickY);
    hash = ndsPlatformMixDebugValue(hash, gSYControllerDevices[0].button_hold);
    hash = ndsPlatformMixDebugValue(hash, gSYControllerDevices[0].button_tap);
    hash = ndsPlatformMixDebugValue(hash,
                                    (u32)gSYControllerDevices[0].stick_range.x);
    hash = ndsPlatformMixDebugValue(hash,
                                    (u32)gSYControllerDevices[0].stick_range.y);
    hash = ndsPlatformMixDebugValue(hash,
                                    (u32)gNdsFighterBattlePlayableFinalXMilli);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfSampleCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfPresentFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfLogicFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfDLDrawFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfPreviewCommitFps);
    hash = ndsPlatformMixDebugValue(hash, gNdsPerfPreviewCommitCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomSkipToTitleCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDrawBlocker);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningRoomDrawCameraCallbackCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningRoomDrawDObjCallbackCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieRoomHandoffResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieRoomHandoffTick);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieRoomHandoffScene);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsStartResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsFuncStartResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsUpdateResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsRelocResult);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningPortraitsSpriteNormalizeCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningPortraitsSpriteNormalizeFailCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsTickCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawBlocker);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningPortraitsDrawVisibleSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawWidth);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawHeight);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsDrawPixels);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsNextSceneResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningPortraitsNextSceneKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioTickCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioDrawVisibleSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMarioDrawPixels);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneDispatchMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneDrawMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneDispatchCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneLastKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningNameSceneLastNextKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieBridgeResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieBridgeMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieBridgeCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieActionPreviewResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieActionPreviewMask);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieActionPreviewCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningMovieActionPreviewFrameCount);
    hash = ndsPlatformMixDebugValue(hash,
                                    gNdsOpeningMovieActionPreviewLastKind);
    hash = ndsPlatformMixDebugValue(hash, gNdsOpeningMovieTitleResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleRelocResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitlePreviewResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawResult);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleSpriteNormalizeCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleSpriteNormalizeFailCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawVisibleSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawRenderableSObjCount);
    hash = ndsPlatformMixDebugValue(hash, gNdsTitleDrawPixels);

    return hash;
}

void ndsPlatformRenderDebugHud(void)
{
    u32 debug_text_fingerprint;

#if !NDS_DEBUG_HUD
    return;
#endif
#if !NDS_RENDERER_HW_TRIANGLES
    ndsPlatformDrawOriginalDLPreview();
    ndsPlatformDrawOriginalSpritePreview();
#endif
    ndsPlatformUpdatePerfCounters();

    debug_text_fingerprint = ndsPlatformDebugTextFingerprint();
    if ((sDebugTextReady != 0) &&
        (debug_text_fingerprint == sDebugTextFingerprint))
    {
        return;
    }
    sDebugTextReady = 1;
    sDebugTextFingerprint = debug_text_fingerprint;

    ndsPlatformPrintDebugLine(0, "Smash64DS movie debug");
    ndsPlatformPrintDebugLine(1, "text=change %08lx",
                              (unsigned long)debug_text_fingerprint);
    ndsPlatformPrintDebugLine(2, "boot=%08lx self=%08lx",
                              (unsigned long)gNdsOriginalBootStage,
                              (unsigned long)gNdsBootSelfTestResult);
    ndsPlatformPrintDebugLine(3, "scene=%08lx k=%lu ret=%lu",
                              (unsigned long)gNdsSceneBoundaryResult,
                              (unsigned long)gNdsSceneBoundaryKind,
                              (unsigned long)gNdsTaskmanReturnCount);
    ndsPlatformPrintDebugLine(4, "task=%08lx clean=%08lx",
                              (unsigned long)gNdsTaskmanBridgeResult,
                              (unsigned long)gNdsTaskmanCleanupResult);
    ndsPlatformPrintDebugLine(5, "rel=%08lx nfs=%08lx",
                              (unsigned long)gNdsOpeningRoomRelocResult,
                              (unsigned long)gNdsRelocAssetInitResult);
    ndsPlatformPrintDebugLine(6, "file=%02lx hdr=%02lx pay=%02lx",
                              (unsigned long)gNdsOpeningRoomRelocFileMask,
                              (unsigned long)gNdsOpeningRoomRelocHeaderMask,
                              (unsigned long)gNdsOpeningRoomRelocPayloadMask);
    ndsPlatformPrintDebugLine(7, "fix sw=%02lx/%lu pt=%02lx/%lu",
                              (unsigned long)gNdsOpeningRoomRelocWordSwapMask,
                              (unsigned long)gNdsOpeningRoomRelocWordSwapCount,
                              (unsigned long)gNdsOpeningRoomRelocPointerFixupMask,
                              (unsigned long)gNdsOpeningRoomRelocPointerFixupCount);
    ndsPlatformPrintDebugLine(8, "evt28=%02lx 38=%02lx 45=%02lx",
                              (unsigned long)gNdsOpeningRoomFirstEventDeferredMask,
                              (unsigned long)gNdsOpeningRoomTick380DeferredMask,
                              (unsigned long)gNdsOpeningRoomTick450DeferredMask);
    ndsPlatformPrintDebugLine(9, "evt50=%02lx 56=%02lx sp=%02lx",
                              (unsigned long)gNdsOpeningRoomTick500DeferredMask,
                              (unsigned long)gNdsOpeningRoomTick560DeferredMask,
                              (unsigned long)gNdsOpeningRoomSpotlightCreateMask);
    ndsPlatformPrintDebugLine(10, "cam s1=%03lx s2=%03lx l=%02lx",
                              (unsigned long)gNdsOpeningRoomScene1CameraCreateMask,
                              (unsigned long)gNdsOpeningRoomScene2CameraCreateMask,
                              (unsigned long)gNdsOpeningRoomLogoCameraCreateMask);
    ndsPlatformPrintDebugLine(11, "obj d=%02lx h=%02lx o=%02lx s=%02lx",
                              (unsigned long)gNdsOpeningRoomDeskCreateMask,
                              (unsigned long)gNdsOpeningRoomHazeCreateMask,
                              (unsigned long)gNdsOpeningRoomOutsideCreateMask,
                              (unsigned long)gNdsOpeningRoomSunlightCreateMask);
    ndsPlatformPrintDebugLine(12, "pcl=%08lx fk=%02lx",
                              (unsigned long)gNdsOpeningRoomPencilsCreateResult,
                              (unsigned long)(gNdsOpeningRoomFighterDeferredKind & 0xffu));
    ndsPlatformPrintDebugLine(13, "dlp=%08lx t=%lu p=%lu x=%02lx",
                              (unsigned long)gNdsOpeningRoomDLPreviewResult,
                              (unsigned long)gNdsOpeningRoomDLPreviewTriangleCount,
                              (unsigned long)gNdsOpeningRoomDLPreviewPixelCount,
                              (unsigned long)gNdsOpeningRoomDLPreviewTextureMask);
    ndsPlatformPrintDebugLine(14, "tick~%lu key=%03lx skip=%lu",
                              (unsigned long)ndsPlatformOpeningHudTickMilestone(),
                              (unsigned long)(sHeldKeys & 0xfffu),
                              (unsigned long)gNdsOpeningRoomSkipToTitleCount);
    ndsPlatformPrintDebugLine(15, "draw=%08lx b=%lu c=%lu d=%lu",
                              (unsigned long)gNdsOpeningRoomDrawResult,
                              (unsigned long)gNdsOpeningRoomDrawBlocker,
                              (unsigned long)gNdsOpeningRoomDrawCameraCallbackCount,
                              (unsigned long)gNdsOpeningRoomDrawDObjCallbackCount);
    ndsPlatformPrintDebugLine(16, "mv h=%08lx p=%08lx",
                              (unsigned long)gNdsOpeningMovieRoomHandoffResult,
                              (unsigned long)gNdsOpeningPortraitsStartResult);
    ndsPlatformPrintDebugLine(17, "por t=%lu d=%08lx v=%lu",
                              (unsigned long)gNdsOpeningPortraitsTickCount,
                              (unsigned long)gNdsOpeningPortraitsDrawResult,
                              (unsigned long)gNdsOpeningPortraitsDrawVisibleSObjCount);
    ndsPlatformPrintDebugLine(18, "mario t=%lu v=%lu px=%lu",
                              (unsigned long)gNdsOpeningMarioTickCount,
                              (unsigned long)gNdsOpeningMarioDrawVisibleSObjCount,
                              (unsigned long)gNdsOpeningMarioDrawPixels);
    ndsPlatformPrintDebugLine(19, "name m=%02lx d=%02lx c=%lu",
                              (unsigned long)gNdsOpeningNameSceneDispatchMask,
                              (unsigned long)gNdsOpeningNameSceneDrawMask,
                              (unsigned long)gNdsOpeningNameSceneDispatchCount);
    ndsPlatformPrintDebugLine(20, "fps=%02lu up=%02lu dl=%02lu cv=%02lu",
                              (unsigned long)gNdsPerfPresentFps,
                              (unsigned long)gNdsPerfLogicFps,
                              (unsigned long)gNdsPerfDLDrawFps,
                              (unsigned long)gNdsPerfPreviewCommitFps);
    ndsPlatformPrintDebugLine(21, "ch=%03lx pf=%03lx smp=%02lu win=%02lu",
                              (unsigned long)(gNdsPerfPreviewCommitCount &
                                              0xfffu),
                              (unsigned long)(gNdsOpeningMoviePresentFrameCount &
                                              0xfffu),
                              (unsigned long)(gNdsPerfSampleCount & 0xffu),
                              (unsigned long)(gNdsPerfSampleWindowTicks & 0xffu));
    ndsPlatformPrintDebugLine(22, "inp k=%03lx p=%04lx %ld,%ld",
                              (unsigned long)(sHeldKeys & 0xfffu),
                              (unsigned long)gNdsControllerLivePad0Button,
                              (long)gNdsControllerLivePad0StickX,
                              (long)gNdsControllerLivePad0StickY);
    ndsPlatformPrintDebugLine(23, "sy h=%04x t=%04x %d,%d x=%ld",
                              (unsigned int)gSYControllerDevices[0].button_hold,
                              (unsigned int)gSYControllerDevices[0].button_tap,
                              (int)gSYControllerDevices[0].stick_range.x,
                              (int)gSYControllerDevices[0].stick_range.y,
                              (long)gNdsFighterBattlePlayableFinalXMilli);
}

void ndsPlatformEndFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    u32 submitted = ndsRendererHardwareConsumeSubmittedFrame();

    if ((submitted != 0u) || (sOriginalSpriteOverlayNeedsFlush != FALSE))
    {
        if (submitted != 0u)
        {
            gNdsHardwareRendererSubmittedFrameCount++;
        }
        gNdsHardwareRendererPolyRamCount = GFX_POLYGON_RAM_USAGE;
        gNdsHardwareRendererVertexRamCount = GFX_VERTEX_RAM_USAGE;
        gNdsHardwareRendererStatus = GFX_STATUS;
        gNdsHardwareRendererControl = GFX_CONTROL;
        glFlush(GL_TRANS_MANUALSORT);
        gNdsHardwareRendererFlushCount++;
        sOriginalSpriteOverlayNeedsFlush = FALSE;
    }
    swiWaitForVBlank();
    sTicks++;
#else
    swiWaitForVBlank();
    videoSetMode((sDrawFramebufferIndex == 0) ? MODE_FB0 : MODE_FB1);
    sDrawFramebufferIndex ^= 1u;
    sFramebuffer = sFramebuffers[sDrawFramebufferIndex];
    sTicks++;
#endif
}

u32 ndsPlatformTicks(void)
{
    return sTicks;
}

u32 ndsPlatformHeldKeys(void)
{
    return sHeldKeys;
}
