#include <calico.h>
#include <nds/arm9/cache.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dvm.h>
#include <nds/nds_audio_storage.h>

_Static_assert(PxiChannel_User0 == NDS_AUDIO_STORAGE_CHANNEL,
               "A8 storage PXI channel changed");

/* Capture public mount metadata rather than depending on FatFs's private FIL
 * layout. SDK callers are in separate archive objects, so the linker wrapper
 * sees both automatic mounts and the fatMount compatibility entry points. */
extern bool __real_dvmMountVolume(const char *, DvmDisc *, sec_t, const char *);
extern void __real_dvmUnmountVolume(const char *);
static const devoptab_t *sRomDevice;
static DvmDisc *sRomDisc;
static uint32_t sRomVolumeSector;
static void *sRomExtentAllocation;
static NdsAudioStorageMap sRomMap;

static Mutex sStorageMutex;
static Mutex sRomInitMutex;
static NdsAudioStorageRequest sStorageRequest;
static uint8_t sStorageBounce[512] __attribute__((aligned(32)));
static uint32_t sStorageSequence;
static NitroRom sCardRom;
static int sCardRomReady;
static uint32_t sRomBytes;

volatile uint32_t gNdsAudioStorageCardRoute;
volatile uint32_t gNdsAudioStorageRequests;
volatile uint32_t gNdsAudioStorageReads;
volatile uint32_t gNdsAudioStorageBytes;
volatile uint32_t gNdsAudioStorageBounceBytes;
volatile uint32_t gNdsAudioStorageFailures;
volatile uint32_t gNdsAudioStorageMapFailure;
volatile uint32_t gNdsAudioStorageMapDevice;
volatile uint32_t gNdsAudioStorageMapVolumeSector;
volatile uint32_t gNdsAudioStorageMapFirstCluster;
volatile uint32_t gNdsAudioStorageMapExtentCount;
volatile uint32_t gNdsAudioStorageMapAddress;

bool __wrap_dvmMountVolume(const char *name, DvmDisc *disc,
                           sec_t start_sector, const char *fstype)
{
    bool ok = __real_dvmMountVolume(name, disc, start_sector, fstype);
    const char *argv0 = g_envNdsArgvHeader->argv[0];
    if (ok && argv0 && !strcmp(fstype, "vfat"))
    {
        char prefix[34];
        size_t len = strnlen(name, 32);
        memcpy(prefix, name, len);
        prefix[len] = ':'; prefix[len + 1] = 0;
        const devoptab_t *mounted = GetDeviceOpTab(prefix);
        if (mounted && mounted == GetDeviceOpTab(argv0))
        {
            sRomDevice = mounted;
            sRomDisc = disc;
            sRomVolumeSector = start_sector;
        }
    }
    return ok;
}

void __wrap_dvmUnmountVolume(const char *name)
{
    if (sRomDevice && GetDeviceOpTab(name) == sRomDevice)
    {
        sRomDevice = NULL;
        sRomDisc = NULL;
    }
    __real_dvmUnmountVolume(name);
}

void __attribute__((noinline, used, noreturn)) ndsAudioStorageInitHalt(uint32_t reason)
{
    gNdsAudioStorageMapFailure = reason;
    DC_FlushAll();
    for (;;) { __asm__ volatile("" ::: "memory"); }
}

static int ndsAudioStorageFatSector(void *unused, uint32_t sector, uint8_t *out)
{
    (void)unused;
    if (!dvmDiscReadSectors(sRomDisc, sStorageBounce, sector, 1u)) return 0;
    memcpy(out, sStorageBounce, 512u);
    return 1;
}

static uint32_t ndsAudioStorageHeaderWord(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t ndsAudioStorageHeaderSize(const uint8_t *header, uint32_t file_bytes)
{
    /* The libnds argv structure at 0x02fffe70 overlaps the application-header
     * ntr_rom_size field at 0x02fffe80. The original SDK reader never used that
     * overwritten field. Read the immutable file header, and bind its FAT/FNT
     * identity to the still-valid fields in the running application's header. */
    uint32_t bytes = ndsAudioStorageHeaderWord(header + 0x80);
    if (bytes < 512u || bytes > file_bytes ||
        bytes > ndsAudioStorageCardCapacity(g_envAppNdsHeader->device_capacity) ||
        ndsAudioStorageHeaderWord(header + 0x40) != g_envAppNdsHeader->fnt_rom_offset ||
        ndsAudioStorageHeaderWord(header + 0x44) != g_envAppNdsHeader->fnt_size ||
        ndsAudioStorageHeaderWord(header + 0x48) != g_envAppNdsHeader->fat_rom_offset ||
        ndsAudioStorageHeaderWord(header + 0x4c) != g_envAppNdsHeader->fat_size) return 0;
    return bytes;
}

static int ndsAudioStorageBuildMap(const char *path)
{
    struct stat st;
    NdsAudioFatVolume volume;
    uint32_t count, filled, bytes;
    int fd;
    _Static_assert(sizeof(st.st_ino) >= 4, "FAT first-cluster inode is truncated");
    if (!sRomDisc || sRomDevice != GetDeviceOpTab(path) ||
        sRomDisc->sector_shift != 9 || sRomDisc->io_type > BlkDevice_TwlNandAes)
        return 1;
    fd = open(path, O_RDONLY);
    if (fd < 0) return 2;
    int ok = fstat(fd, &st) == 0 && st.st_size >= 512 &&
             (uint64_t)st.st_size <= UINT32_MAX &&
             read(fd, sStorageBounce, sizeof(sStorageBounce)) == sizeof(sStorageBounce);
    close(fd);
    if (!ok || !st.st_ino || (uint32_t)st.st_dev != sRomDisc->io_type) return 3;
    sRomBytes = ndsAudioStorageHeaderSize(sStorageBounce, (uint32_t)st.st_size);
    if (!sRomBytes) return 3;
    if (!ndsAudioFatOpen(&volume, ndsAudioStorageFatSector, NULL,
                        sRomVolumeSector, sRomDisc->num_sectors)) return 4;
    if (!ndsAudioFatExtents(&volume, ndsAudioStorageFatSector, NULL,
                (uint32_t)st.st_ino, sRomBytes, NULL, 0, &count) ||
        count > (UINT32_MAX - 63u) / sizeof(NdsAudioExtent)) return 5;
    bytes = (count * sizeof(NdsAudioExtent) + 31u) & ~31u;
    sRomExtentAllocation = malloc(bytes + 31u);
    if (!sRomExtentAllocation) return 6;
    NdsAudioExtent *extents = (NdsAudioExtent *)
        (((uintptr_t)sRomExtentAllocation + 31u) & ~(uintptr_t)31u);
    if (!ndsAudioFatExtents(&volume, ndsAudioStorageFatSector, NULL,
                (uint32_t)st.st_ino, sRomBytes, extents, count, &filled) ||
        filled != count || !ndsAudioExtentsValid(extents, count,
                    sRomBytes, sRomDisc->num_sectors))
    {
        free(sRomExtentAllocation); sRomExtentAllocation = NULL;
        return 7;
    }
    sRomMap = (NdsAudioStorageMap){
        .abi = NDS_AUDIO_STORAGE_ABI, .device = sRomDisc->io_type,
        .rom_bytes = sRomBytes, .extent_count = count,
        .extents = (uint32_t)(uintptr_t)extents
    };
    DC_FlushRange(extents, bytes);
    gNdsAudioStorageMapDevice = sRomMap.device;
    gNdsAudioStorageMapVolumeSector = sRomVolumeSector;
    gNdsAudioStorageMapFirstCluster = (uint32_t)st.st_ino;
    gNdsAudioStorageMapExtentCount = count;
    gNdsAudioStorageMapAddress = sRomMap.extents;
    return 0;
}

static int ndsAudioStorageCall(uint32_t operation, uint32_t offset,
                               void *destination, uint32_t bytes)
{
    uint32_t reply;
    uint32_t sequence = (sStorageSequence + 1u) & 0xffffu;
    if (sequence == 0u) sequence = 1u;
    sStorageSequence = sequence;
    sStorageRequest = (NdsAudioStorageRequest){
        .abi = NDS_AUDIO_STORAGE_ABI, .operation = operation,
        .sequence = sequence, .offset = offset,
        .destination = (uint32_t)(uintptr_t)destination, .bytes = bytes
    };
    if (bytes != 0u)
    {
        /* Every output passed here owns whole cache lines. Flush before the
         * handoff so a later eviction cannot overwrite ARM7's fresh bytes. */
        DC_FlushRange(destination, bytes);
    }
    DC_FlushRange(&sStorageRequest, sizeof(sStorageRequest));
    gNdsAudioStorageRequests++;
    reply = pxiSendAndReceive((PxiChannel)NDS_AUDIO_STORAGE_CHANNEL,
                             (uint32_t)(uintptr_t)&sStorageRequest >> 5);
    if (bytes != 0u) DC_InvalidateRange(destination, bytes);
    if (reply != ndsAudioStorageReply(sequence, NDS_AUDIO_STORAGE_OK))
    {
        gNdsAudioStorageFailures++;
        return 0;
    }
    return 1;
}

static bool ndsAudioStorageReadCard(void *unused, uint32_t offset,
                                   void *destination, uint32_t bytes)
{
    uint8_t *out = destination;
    uint32_t rom_bytes = sRomBytes;
    int ok = 1;
    (void)unused;
    if ((rom_bytes < sizeof(sStorageRequest)) ||
        (offset > rom_bytes) || (bytes > rom_bytes - offset) ||
        ((destination == NULL) && (bytes != 0u))) return false;
    mutexLock(&sStorageMutex);
    while (bytes != 0u)
    {
        uint32_t part;
        uintptr_t address = (uintptr_t)out;
        if (((address & 31u) == 0u) && (bytes >= 32u) &&
            (address <= UINT32_MAX) &&
            ndsAudioStorageMainRange((uint32_t)address, bytes))
        {
            part = bytes & ~31u;
            if (part > NDS_AUDIO_STORAGE_MAX_READ)
                part = NDS_AUDIO_STORAGE_MAX_READ;
            ok = ndsAudioStorageCall(NDS_AUDIO_STORAGE_READ_CARD,
                                     offset, out, part);
        }
        else
        {
            uint32_t transfer;
            uint32_t read_offset = offset;
            part = bytes < sizeof(sStorageBounce) ? bytes : sizeof(sStorageBounce);
            transfer = (part + 31u) & ~31u;
            /* The final ROM bytes may not fill a cache line. Read a complete
             * preceding line into the owned bounce, then select its suffix. */
            if (transfer > rom_bytes - read_offset)
                read_offset = rom_bytes - transfer;
            ok = ndsAudioStorageCall(NDS_AUDIO_STORAGE_READ_CARD,
                                     read_offset, sStorageBounce, transfer);
            if (ok)
            {
                memcpy(out, sStorageBounce + offset - read_offset, part);
                gNdsAudioStorageBounceBytes += part;
            }
        }
        if (!ok) break;
        gNdsAudioStorageReads++;
        gNdsAudioStorageBytes += part;
        out += part;
        offset += part;
        bytes -= part;
    }
    mutexUnlock(&sStorageMutex);
    return ok != 0;
}

static void ndsAudioStorageCloseCard(void *unused)
{
    (void)unused;
    mutexLock(&sStorageMutex);
    (void)ndsAudioStorageCall(NDS_AUDIO_STORAGE_CLOSE_CARD, 0u, NULL, 0u);
    sCardRomReady = 0;
    free(sRomExtentAllocation);
    sRomExtentAllocation = NULL;
    mutexUnlock(&sStorageMutex);
}

static const NitroRomIface sCardInterface = {
    .read = ndsAudioStorageReadCard,
    .close = ndsAudioStorageCloseCard
};

NitroRom *__wrap_nitroromGetSelf(void)
{
    NitroRom *rom = NULL;
    const char *argv0 = g_envNdsArgvHeader->argv[0];

    mutexLock(&sRomInitMutex);
    if (sCardRomReady)
        rom = &sCardRom;
    else
    {
        NitroRomParams params = {
            .fat_offset = g_envAppNdsHeader->fat_rom_offset,
            .fat_sz = g_envAppNdsHeader->fat_size,
            .fnt_offset = g_envAppNdsHeader->fnt_rom_offset,
            .fnt_sz = g_envAppNdsHeader->fnt_size, .img_offset = 0u
        };
        if (argv0)
        {
            uint32_t error = (uint32_t)ndsAudioStorageBuildMap(argv0);
            if (error) ndsAudioStorageInitHalt(error);
        }
        else if (g_envBootParam->boot_src != EnvBootSrc_Card)
            ndsAudioStorageInitHalt(8u);
        else
        {
            sRomBytes = ndsAudioStorageCardCapacity(g_envAppNdsHeader->device_capacity);
            if (!sRomBytes) ndsAudioStorageInitHalt(8u);
        }
        pxiWaitRemote((PxiChannel)NDS_AUDIO_STORAGE_CHANNEL);
        mutexLock(&sStorageMutex);
        int opened = argv0 ?
            ndsAudioStorageCall(NDS_AUDIO_STORAGE_OPEN_MAP, 0u, &sRomMap, sizeof(sRomMap)) :
            ndsAudioStorageCall(NDS_AUDIO_STORAGE_OPEN_CARD, 0u, NULL, 0u);
        mutexUnlock(&sStorageMutex);
        if (!opened) ndsAudioStorageInitHalt(9u);
        if (opened && nitroromOpen(&sCardRom, &params, &sCardInterface, NULL))
        {
            sCardRomReady = 1;
            gNdsAudioStorageCardRoute = argv0 ? 2u : 1u;
            rom = &sCardRom;
        }
        else if (opened)
        {
            ndsAudioStorageCloseCard(NULL);
            ndsAudioStorageInitHalt(10u);
        }
    }
    mutexUnlock(&sRomInitMutex);
    return rom;
}
