#include <nds.h>
#include <stdio.h>

#include <nds/nds_platform.h>
#include <nds/nds_os.h>
#include <nds/nds_boot.h>
#include <nds/nds_controller.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_video.h>
#include <port/port_probe.h>
#include <port/coroutine.h>
#include <sys/controller.h>

volatile u32 gNdsBootSelfTestResult;
volatile u32 gNdsFrameCounter;

void syMainLoop(void);

int main(void)
{
    char debug_message[64];
    int os_test;

    ndsPlatformInit();
    ndsRelocAssetsInit();
    portCoroutineInitMain();
    os_test = ndsOsSelfTest();
    iprintf("OS queues/threads: %s", os_test == 0 ? "PASS\n" : "FAIL ");
    if (os_test != 0) iprintf("%d\n", os_test);
    snprintf(debug_message, sizeof(debug_message),
             "SSB64DS: OS SELFTEST %s (%d)\n",
             os_test == 0 ? "PASS" : "FAIL", os_test);
    nocashMessage(debug_message);
    gNdsBootSelfTestResult = (os_test == 0)
        ? 0x50415353u
        : (0xFA110000u | (u32)os_test);

    syMainLoop();
    iprintf("Original boot: %s\n",
            gNdsOriginalBootStage == NDS_BOOT_EXPECTED ? "PASS" : "PARTIAL");
    ndsVideoBootstrapStart();
    portProbeInit();

    while (1)
    {
        ndsPlatformReadInput();

        ndsOsPostVBlank();
        ndsOsRunThreads();
        ndsVideoBootstrapUpdate();
        if (gNdsControllerPollCount != 0 &&
            gSYControllerConnectedNum != 0) {
            syControllerUpdateGlobalData();
        }
        portProbeUpdate();
        ndsPlatformBeginFrame();
        portProbeRender();
        ndsPlatformRenderDebugHud();
        ndsPlatformEndFrame();
        gNdsFrameCounter++;
    }
}
