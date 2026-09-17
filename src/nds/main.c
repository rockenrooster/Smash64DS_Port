#include <nds.h>
#include <stdio.h>

#include <nds/nds_platform.h>
#include <nds/nds_os.h>
#include <nds/nds_boot.h>
#include <nds/nds_controller.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_task10_hardware_calibration.h>
#include <nds/nds_video.h>
#include <port/port_probe.h>
#include <port/coroutine.h>
#include <sys/controller.h>

volatile u32 gNdsBootSelfTestResult;
volatile u32 gNdsFrameCounter;

/* P2-2p8 stall budget, the one experiment that prices the data cache itself.
 * The frame is 64.3% memory stall and 560,739 tk/fr of that is data stall, but
 * every layout candidate has been bounded by asking what a BETTER arrangement
 * would save. This asks the opposite and much cheaper question: what is the 4 KB
 * data cache worth RIGHT NOW? Running the same match with it off makes
 * `frame_off - frame_on` exactly that number.
 *
 * It is decisive in both directions. If the working set were reusable, the
 * delta is enormous and layout work has real headroom. If the traffic is
 * compulsory first-touch as measured -- the renderer fetches less than half its
 * own resident data per frame and its lines are used 1.42x on average -- the
 * delta tracks the frame's word count times main-RAM latency, and no
 * rearrangement can find 496,382.
 *
 * ARM946E-S bit 2 of CP15 c1 is the data-cache enable. Clean and invalidate
 * first, or dirty lines never reach memory. Lab only; the shipping default is 0
 * and this costs nothing when compiled out, because the whole body is gone. */
/* ARM, not Thumb: CP15 coprocessor transfers have no Thumb encoding, and this
 * build is -mthumb. Same idiom as ndsFtPosePlay. */
static void __attribute__((noinline, target("arm")))
ndsLabDisableDataCacheIfRequested(void)
{
#if NDS_LAB_NO_DCACHE
    u32 cr;

    DC_FlushAll();
    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(cr));
    cr &= ~(1u << 2);
    __asm__ volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(cr) : "memory");
#endif
}

void syMainLoop(void);

int main(void)
{
    char debug_message[64];
    int os_test;

    ndsLabDisableDataCacheIfRequested();
    ndsPlatformInit();
#if NDS_TASK10_HARDWARE_CALIBRATION
    ndsTask10HardwareCalibrationRun();
#endif
    ndsRelocAssetsInit();
    portCoroutineInitMain();
    os_test = ndsOsSelfTest();
#if NDS_BOOT_DIAG_TEXT
    /* P2-1L (11). The result itself is published below in
     * gNdsBootSelfTestResult and read by every verifier over gdb; this is the
     * human-readable copy on the sub console, which a ROM handed to the owner
     * does not want under its menus. */
    iprintf("OS queues/threads: %s", os_test == 0 ? "PASS\n" : "FAIL ");
    if (os_test != 0) iprintf("%d\n", os_test);
#endif
    sniprintf(debug_message, sizeof(debug_message),
             "SSB64DS: OS SELFTEST %s (%d)\n",
             os_test == 0 ? "PASS" : "FAIL", os_test);
    nocashMessage(debug_message);
    gNdsBootSelfTestResult = (os_test == 0)
        ? 0x50415353u
        : (0xFA110000u | (u32)os_test);

    syMainLoop();
#if NDS_BOOT_DIAG_TEXT
    iprintf("Original boot: %s\n",
            gNdsOriginalBootStage == NDS_BOOT_EXPECTED ? "PASS" : "PARTIAL");
#endif
    ndsVideoBootstrapStart();
    portProbeInit();

    while (1)
    {
#if NDS_R2_MAIN_PRESENT_GUARD
        u32 presented_before;
#endif

        ndsPlatformReadInput();

        ndsOsPostVBlank();
#if NDS_R2_MAIN_PRESENT_GUARD
        presented_before = ndsPlatformTicks();
#endif
        ndsOsRunThreads();
        ndsVideoBootstrapUpdate();
        if (gNdsControllerPollCount != 0 &&
            gSYControllerConnectedNum != 0) {
            syControllerUpdateGlobalData();
        }
        portProbeUpdate();
        /* Only present if the scene loop resumed above did not. A scene that
         * drives its own presentation has already submitted, flushed and waited
         * for VBlank inside its own ndsPlatformEndFrame; repeating it here draws
         * nothing (no geometry is submitted, so the flush is skipped) and still
         * pays one unconditional swiWaitForVBlank.
         *
         * Measured 2026-07-30 on smash64ds-results-lab-hwtri: VS Results ran
         * 2.00 presents per source tic against the battle path's 1.00, with GX
         * submits and flushes both at 1.00 -- so exactly half the presents
         * rendered nothing and existed only to burn a VBlank. */
#if NDS_R2_MAIN_PRESENT_GUARD
        if (ndsPlatformTicks() == presented_before)
#endif
        {
            ndsPlatformBeginFrame();
            portProbeRender();
            ndsPlatformRenderDebugHud();
            ndsPlatformEndFrame();
        }
        gNdsFrameCounter++;
    }
}
