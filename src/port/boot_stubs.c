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

static void ndsBootServiceThread(u32 ready_flag)
{
    OSMesgQueue queue;
    OSMesg buffer[1];

    osCreateMesgQueue(&queue, buffer, 1);
    gNdsOriginalBootStage |= ready_flag;
    osSendMesg(&gSYMainThreadingMesgQueue, (OSMesg)1, OS_MESG_NOBLOCK);

    while (TRUE) {
        osRecvMesg(&queue, NULL, OS_MESG_BLOCK);
    }
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
