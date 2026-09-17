#include <stdarg.h>
#include <string.h>

#include <PR/os.h>
#include <PR/rcp.h>
#include <ssb_types.h>
#include <nds/nds_boot.h>
#include <sys/dma.h>
#include <sys/main.h>

volatile u32 gNdsOriginalBootStage;

uintptr_t scmanager_ROM_START;
uintptr_t scmanager_ROM_END;
uintptr_t scmanager_VRAM;
uintptr_t scmanager_TEXT_START;
uintptr_t scmanager_TEXT_END;
uintptr_t scmanager_DATA_START;
uintptr_t scmanager_RODATA_END;
uintptr_t scmanager_BSS_START;
uintptr_t scmanager_BSS_END;

OSPiHandle *gSYDmaRomPiHandle;
static OSPiHandle sRomHandle;

/* ACKNOWLEDGE, THEN FINISH -- do not park forever on a queue nobody feeds.
 *
 * The source creates this as thread 4 and its body blocks on a private queue.
 * That queue is a LOCAL on this thread's own stack and is never published, so
 * no producer exists or can exist; the block was equivalent to returning, but
 * it cost real resources to express:
 *
 *   - the coroutine's stack stayed live. A service thread takes
 *     NDS_OS_SERVICE_STACK_SIZE, 16,384 bytes, malloc'd at osStartThread
 *     (libultra_os.c) -- four arena pages held by a thread that does no mixing,
 *     no sample delivery and no music advancement.
 *   - ndsOsRunThreads resumed it every frame forever, because a blocked thread
 *     is WAITING and the pump resumes WAITING as well as RUNNABLE.
 *
 * Both halves of the contract survive the change: the ready bit is set and the
 * acknowledgement is sent before returning, so anything waiting on boot
 * progress sees exactly what it saw before. Returning ends the coroutine, which
 * marks the thread STOPPED, drops it out of the pump, and lets its 16 KiB go
 * back (see the reclaim at both finish sites in libultra_os.c).
 *
 * Owner review, docs/optimization/OTHR.md candidate O1, 2026-09-17. */
static void ndsBootServiceThread(u32 ready_flag)
{
    gNdsOriginalBootStage |= ready_flag;
    osSendMesg(&gSYMainThreadingMesgQueue, (OSMesg)1, OS_MESG_NOBLOCK);
}

void syAudioThreadMain(void *arg)
{
    (void)arg;
    ndsBootServiceThread(NDS_BOOT_AUDIO_READY);
}

void osInitialize(void)
{
}

void osCreateViManager(OSPri priority)
{
    (void)priority;
}

OSPiHandle *osCartRomInit(void)
{
    return &sRomHandle;
}

void osCreatePiManager(OSPri priority, OSMesgQueue *queue,
                       OSMesg *buffer, s32 count)
{
    (void)priority;
    osCreateMesgQueue(queue, buffer, count);
}

OSPiHandle *syDmaSramPiInit(void)
{
    return &sRomHandle;
}

void syDmaCreateMesgQueue(void)
{
}

void syDmaReadRom(uintptr_t rom_source, void *ram_destination, size_t size)
{
    (void)rom_source;
    memset(ram_destination, 0, size);
}

void syDmaLoadOverlay(SYOverlay *overlay)
{
    (void)overlay;
}

void syDebugStartRmonThread8(void)
{
}

void syDebugPrintf(const char *format, ...)
{
    (void)format;
}

void __osSetWatchLo(u32 value)
{
    (void)value;
}

u32 ndsN64IoRead(u32 address)
{
    if (address == SP_IMEM_START) return 6103u;
    if (address == SP_DMEM_START) return 0xffffffffu;
    return 0;
}
