#include <PR/mbi.h>
#include <PR/os.h>
#include <nds/nds_video.h>
#include <sys/video.h>

static OSThread sVideoBootstrapThread;
static sb32 sVideoBootstrapStarted;

volatile u32 gNdsVideoBootstrapResult;

static void ndsVideoBootstrapThread(void *arg)
{
    SYVideoSetup setup = {
        {
            &gSYFramebufferSets[0],
            &gSYFramebufferSets[1],
            &gSYFramebufferSets[2]
        },
        gSYZBuffer,
        320,
        240,
        SYVIDEO_FLAG_DIVOT | SYVIDEO_FLAG_DITHERFILTER |
        SYVIDEO_FLAG_NOGAMMADITHER | 0x800 |
        SYVIDEO_FLAG_NOBLACKOUT | SYVIDEO_FLAG_NOGAMMA |
        SYVIDEO_FLAG_COLORDEPTH16 | SYVIDEO_FLAG_NOSERRATE |
        SYVIDEO_FLAG_ANTIALIAS
    };

    (void)arg;
    syVideoInit(&setup);

    if (gSYVideoResWidth == 320 && gSYVideoResHeight == 240 &&
        gSYVideoColorDepth == G_IM_SIZ_16b &&
        gSYVideoZBuffer == gSYZBuffer) {
        gNdsVideoBootstrapResult = NDS_VIDEO_BOOTSTRAP_PASS;
    } else {
        gNdsVideoBootstrapResult = 0xBAD00001u;
    }
}

void ndsVideoBootstrapStart(void)
{
    if (sVideoBootstrapStarted) return;
    sVideoBootstrapStarted = TRUE;
    osCreateThread(&sVideoBootstrapThread, 91, ndsVideoBootstrapThread,
                   NULL, NULL, 50);
    osStartThread(&sVideoBootstrapThread);
}

void ndsVideoBootstrapUpdate(void)
{
    if (sVideoBootstrapStarted && gNdsVideoBootstrapResult != 0 &&
        sVideoBootstrapThread.port_coroutine != NULL &&
        sVideoBootstrapThread.state == OS_STATE_STOPPED) {
        osDestroyThread(&sVideoBootstrapThread);
    }
}

