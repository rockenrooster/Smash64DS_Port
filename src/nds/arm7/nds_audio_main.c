#include <calico.h>
#include <nds/nds_audio_storage.h>
#include <string.h>

_Static_assert(PxiChannel_User0 == NDS_AUDIO_STORAGE_CHANNEL,
               "A8 storage PXI channel changed");

static Thread sStorageThread;
static Mailbox sStorageMailbox;
static uint32_t sStorageMessages[4];
static uint8_t sStorageStack[2048] __attribute__((aligned(8)));
static int sCardOpen;
static uint32_t sRomBytes;
static NdsAudioStorageMap sFileMap;
static uint8_t sSectorBuffer[512] __attribute__((aligned(4)));
extern bool ndsAudioDldiInstall(void);
extern volatile uint32_t gNdsArm7DldiSerialized;

volatile uint32_t gNdsArm7StorageRequests;
volatile uint32_t gNdsArm7StorageReads;
volatile uint32_t gNdsArm7StorageBytes;
volatile uint32_t gNdsArm7StorageFailures;

static bool ndsAudioStorageOpenMap(const NdsAudioStorageMap *map)
{
    if (sCardOpen || map->abi != NDS_AUDIO_STORAGE_ABI || map->device > BlkDevice_TwlNandAes ||
        map->rom_bytes < 512u ||
        map->rom_bytes > ndsAudioStorageCardCapacity(g_envAppNdsHeader->device_capacity) ||
        map->reserved[0] || map->reserved[1] || map->reserved[2] ||
        !map->extent_count || map->extent_count > UINT32_MAX / sizeof(NdsAudioExtent) ||
        (map->extents & 31u) ||
        !ndsAudioStorageMainRange(map->extents, map->extent_count * sizeof(NdsAudioExtent)) ||
        (map->device == BlkDevice_Dldi && !gNdsArm7DldiSerialized)) return false;
    if (!ndsAudioExtentsValid((const NdsAudioExtent *)(uintptr_t)map->extents,
            map->extent_count, map->rom_bytes, blkDevGetSectorCount((BlkDevice)map->device)))
        return false;
    sFileMap = *map;
    sRomBytes = map->rom_bytes;
    return true;
}

static bool ndsAudioStorageReadMap(uint32_t offset, uint8_t *out, uint32_t bytes)
{
    const NdsAudioExtent *map = (const NdsAudioExtent *)(uintptr_t)sFileMap.extents;
    while (bytes)
    {
        uint32_t lba, available, part;
        uint32_t skip = offset & 511u;
        uint32_t sectors = !skip && !((uintptr_t)out & 3u) ? bytes / 512u : 0u;
        if (!ndsAudioExtentResolve(map, sFileMap.extent_count, offset / 512u,
                                    sectors ? sectors : 1u, &lba, &available)) return false;
        if (sectors)
        {
            if (!blkDevReadSectors((BlkDevice)sFileMap.device, out, lba, available)) return false;
            part = available * 512u;
        }
        else
        {
            if (!blkDevReadSectors((BlkDevice)sFileMap.device, sSectorBuffer, lba, 1)) return false;
            part = bytes < 512u - skip ? bytes : 512u - skip;
            memcpy(out, sSectorBuffer + skip, part);
        }
        offset += part; out += part; bytes -= part;
    }
    return true;
}

static int ndsAudioStorageThread(void *unused)
{
    (void)unused;
    for (;;)
    {
        uint32_t message = mailboxRecv(&sStorageMailbox);
        uint32_t address = message << 5;
        uint32_t status = NDS_AUDIO_STORAGE_BAD_REQUEST;
        uint32_t sequence = 0u;

        gNdsArm7StorageRequests++;
        if ((message <= (UINT32_MAX >> 5)) &&
            ndsAudioStorageMainRange(address, sizeof(NdsAudioStorageRequest)))
        {
            const NdsAudioStorageRequest *request =
                (const NdsAudioStorageRequest *)(uintptr_t)address;
            sequence = request->sequence;
            if (ndsAudioStorageValidate(request, address, sRomBytes))
            {
                switch (request->operation)
                {
                case NDS_AUDIO_STORAGE_OPEN_CARD:
                    if (!sCardOpen) sCardOpen = ntrcardOpen();
                    if (sCardOpen)
                        sRomBytes = ndsAudioStorageCardCapacity(g_envAppNdsHeader->device_capacity);
                    if (!sRomBytes) sCardOpen = 0;
                    status = sCardOpen ? NDS_AUDIO_STORAGE_OK :
                                        NDS_AUDIO_STORAGE_IO_ERROR;
                    break;
                case NDS_AUDIO_STORAGE_CLOSE_CARD:
                    if (sCardOpen) ntrcardClose();
                    sCardOpen = 0;
                    sRomBytes = 0u;
                    memset(&sFileMap, 0, sizeof(sFileMap));
                    status = NDS_AUDIO_STORAGE_OK;
                    break;
                case NDS_AUDIO_STORAGE_READ_CARD:
                    if (!sCardOpen && !sFileMap.extent_count)
                        status = NDS_AUDIO_STORAGE_NOT_OPEN;
                    else if (!(sFileMap.extent_count ?
                               ndsAudioStorageReadMap(request->offset,
                                   (void *)(uintptr_t)request->destination, request->bytes) :
                               ntrcardRomRead(-1, request->offset,
                                   (void *)(uintptr_t)request->destination, request->bytes)))
                        status = NDS_AUDIO_STORAGE_IO_ERROR;
                    else
                    {
                        status = NDS_AUDIO_STORAGE_OK;
                        gNdsArm7StorageReads++;
                        gNdsArm7StorageBytes += request->bytes;
                    }
                    break;
                case NDS_AUDIO_STORAGE_OPEN_MAP:
                    status = ndsAudioStorageOpenMap((const NdsAudioStorageMap *)(uintptr_t)
                                request->destination) ? NDS_AUDIO_STORAGE_OK : NDS_AUDIO_STORAGE_BAD_REQUEST;
                    break;
                }
            }
        }
        if (status != NDS_AUDIO_STORAGE_OK) gNdsArm7StorageFailures++;
        pxiReply((PxiChannel)NDS_AUDIO_STORAGE_CHANNEL,
                 ndsAudioStorageReply(sequence, status));
    }
    return 0;
}

int main(void)
{
    if (!ndsAudioDldiInstall())
    {
        gNdsArm7StorageFailures++;
        for (;;) { __asm__ volatile("" ::: "memory"); }
    }
    /* Keep the SDK services used by the game. P2 has no wireless or Maxmod
     * consumer; its audio uses Calico's sound service and the native packs. */
    envReadNvramSettings();
    keypadStartExtServer();
    lcdSetIrqMask(DISPSTAT_IE_ALL, DISPSTAT_IE_VBLANK);
    irqEnable(IRQ_VBLANK);
    rtcInit();
    rtcSyncTime();
    pmInit();
    blkInit();
    touchInit();
    touchStartServer(80, MAIN_THREAD_PRIO);
    soundStartServer(12);
    micStartServer(4);

    mailboxPrepare(&sStorageMailbox, sStorageMessages, 4);
    pxiSetMailbox((PxiChannel)NDS_AUDIO_STORAGE_CHANNEL, &sStorageMailbox);
    threadPrepare(&sStorageThread, ndsAudioStorageThread, NULL,
                  sStorageStack + sizeof(sStorageStack), 24);
    threadStart(&sStorageThread);
    while (pmMainLoop()) threadWaitForVBlank();
    return 0;
}
