#include <nds.h>

#include <PR/os.h>
#include <PR/sptask.h>
#include <nds/nds_boot.h>
#include <nds/nds_os.h>
#include <nds/nds_platform.h>

#define NDS_OS_EVENT_COUNT 15

typedef struct NDSEventBinding {
    OSMesgQueue *queue;
    OSMesg message;
} NDSEventBinding;

static NDSEventBinding sEvents[NDS_OS_EVENT_COUNT];
static OSMesgQueue *sViQueue;
static OSMesg sViMessage;
static u32 sViRetraceCount = 1;
static u32 sViRetracePhase;
static void *sCurrentFramebuffer;
static void *sNextFramebuffer;

s32 osTvType = OS_TV_NTSC;
OSViMode osViModeNtscLan1;
OSViMode osViModeMpalLan1;

void osSetEventMesg(OSEvent event, OSMesgQueue *queue, OSMesg message)
{
    if (event < NDS_OS_EVENT_COUNT) {
        sEvents[event].queue = queue;
        sEvents[event].message = message;
    }
}

void osViSetEvent(OSMesgQueue *queue, OSMesg message, u32 retrace_count)
{
    sViQueue = queue;
    sViMessage = message;
    sViRetraceCount = retrace_count != 0 ? retrace_count : 1;
    sViRetracePhase = 0;
    gNdsOriginalBootStage |= NDS_BOOT_SCHEDULER_READY;
}

void ndsOsPostVBlank(void)
{
    sCurrentFramebuffer = sNextFramebuffer;
    if (++sViRetracePhase >= sViRetraceCount) {
        sViRetracePhase = 0;
        if (sViQueue != NULL) {
            osSendMesg(sViQueue, sViMessage, OS_MESG_NOBLOCK);
        }
    }
}

void osViSetMode(OSViMode *mode)
{
    (void)mode;
}

void osViBlack(u8 active)
{
    (void)active;
}

void osViSetYScale(f32 scale)
{
    (void)scale;
}

void osViSwapBuffer(void *framebuffer)
{
    sNextFramebuffer = framebuffer;
}

void *osViGetCurrentFramebuffer(void)
{
    return sCurrentFramebuffer;
}

void *osViGetNextFramebuffer(void)
{
    return sNextFramebuffer;
}

void osSpTaskLoad(OSTask *task)
{
    (void)task;
}

void osSpTaskStartGo(OSTask *task)
{
    (void)task;
    if (sEvents[OS_EVENT_SP].queue != NULL) {
        osSendMesg(sEvents[OS_EVENT_SP].queue,
                   sEvents[OS_EVENT_SP].message, OS_MESG_NOBLOCK);
    }
}

void osSpTaskYield(void)
{
}

OSYieldResult osSpTaskYielded(OSTask *task)
{
    (void)task;
    return 0;
}

s32 osDpSetNextBuffer(void *buffer, u64 size)
{
    (void)buffer;
    (void)size;
    if (sEvents[OS_EVENT_DP].queue != NULL) {
        osSendMesg(sEvents[OS_EVENT_DP].queue,
                   sEvents[OS_EVENT_DP].message, OS_MESG_NOBLOCK);
    }
    return 0;
}

u32 osGetCount(void)
{
    return cpuGetTiming();
}

void osInvalDCache(void *address, s32 size)
{
    if (address != NULL && size > 0) DC_InvalidateRange(address, (u32)size);
}

void osWritebackDCache(void *address, s32 size)
{
    if (address != NULL && size > 0) DC_FlushRange(address, (u32)size);
}

void osWritebackDCacheAll(void)
{
    DC_FlushAll();
}

void osInvalICache(void *address, s32 size)
{
    (void)address;
    (void)size;
    IC_InvalidateAll();
}

s32 osAfterPreNMI(void)
{
    return 0;
}
