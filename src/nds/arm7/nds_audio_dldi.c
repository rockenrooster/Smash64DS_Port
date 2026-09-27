#include <calico.h>
#include <string.h>

/* The ARM9 linker and nds_arm7_contract.ld reserve this exact sheltered DLDI
 * range. Calico publishes this DISC_INTERFACE to its block server before main.
 * Wrap the public interface so both that server and the A8 audio worker use one
 * priority-inheriting lock. Merely wrapping blkDevReadSectors at link time would
 * miss the calls defined inside the SDK's own block translation unit. */
#define NDS_AUDIO_DLDI_ADDRESS 0x0380b000u
#define NDS_AUDIO_DLDI_BURST_SECTORS 16u
static RMutex sDldiMutex;
static DISC_INTERFACE sDldiOriginal;
volatile uint32_t gNdsArm7DldiSerialized;
volatile uint32_t gNdsArm7DldiReads;
volatile uint32_t gNdsArm7DldiWrites;

static bool ndsDldiStartup(void)
{
    rmutexLock(&sDldiMutex);
    bool ok = sDldiOriginal.startup();
    rmutexUnlock(&sDldiMutex);
    return ok;
}
static bool ndsDldiInserted(void)
{
    rmutexLock(&sDldiMutex);
    bool ok = sDldiOriginal.isInserted();
    rmutexUnlock(&sDldiMutex);
    return ok;
}
static bool ndsDldiClear(void)
{
    rmutexLock(&sDldiMutex);
    bool ok = sDldiOriginal.clearStatus();
    rmutexUnlock(&sDldiMutex);
    return ok;
}
static bool ndsDldiShutdown(void)
{
    rmutexLock(&sDldiMutex);
    bool ok = sDldiOriginal.shutdown();
    rmutexUnlock(&sDldiMutex);
    return ok;
}

static bool ndsDldiRead(sec_t first, sec_t count, void *buffer)
{
    uint8_t *out = buffer;
    if (count && count - 1u > UINT32_MAX - first) return false;
    do
    {
        sec_t part = count < NDS_AUDIO_DLDI_BURST_SECTORS ? count : NDS_AUDIO_DLDI_BURST_SECTORS;
        rmutexLock(&sDldiMutex);
        bool ok = sDldiOriginal.readSectors(first, part, out);
        rmutexUnlock(&sDldiMutex);
        if (!ok) return false;
        gNdsArm7DldiReads++;
        first += part; count -= part; out += part * 512u;
    } while (count);
    return true;
}
static bool ndsDldiWrite(sec_t first, sec_t count, const void *buffer)
{
    const uint8_t *in = buffer;
    if (count && count - 1u > UINT32_MAX - first) return false;
    do
    {
        sec_t part = count < NDS_AUDIO_DLDI_BURST_SECTORS ? count : NDS_AUDIO_DLDI_BURST_SECTORS;
        rmutexLock(&sDldiMutex);
        bool ok = sDldiOriginal.writeSectors(first, part, in);
        rmutexUnlock(&sDldiMutex);
        if (!ok) return false;
        gNdsArm7DldiWrites++;
        first += part; count -= part; in += part * 512u;
    } while (count);
    return true;
}

bool ndsAudioDldiInstall(void)
{
    DldiHeader *header = (DldiHeader *)NDS_AUDIO_DLDI_ADDRESS;
    if (gNdsArm7DldiSerialized) return true;
    if (!(g_envExtraInfo->dldi_features & DLDI_FEATURE_CAN_READ)) return true;
    if (header->magic_num != DLDI_MAGIC_VAL ||
        memcmp(header->magic_str, DLDI_MAGIC_STRING, DLDI_MAGIC_STRING_LEN) ||
        header->dldi_start != NDS_AUDIO_DLDI_ADDRESS ||
        header->driver_sz_log2 > DLDI_SIZE_MAX ||
        header->disc.ioType != g_envExtraInfo->dldi_io_type ||
        !header->disc.startup || !header->disc.isInserted ||
        !header->disc.readSectors || !header->disc.clearStatus || !header->disc.shutdown ||
        ((header->disc.features & DLDI_FEATURE_CAN_WRITE) && !header->disc.writeSectors))
        return false;
    sDldiOriginal = header->disc;
    header->disc.startup = ndsDldiStartup;
    header->disc.isInserted = ndsDldiInserted;
    header->disc.readSectors = ndsDldiRead;
    if (sDldiOriginal.writeSectors) header->disc.writeSectors = ndsDldiWrite;
    header->disc.clearStatus = ndsDldiClear;
    header->disc.shutdown = ndsDldiShutdown;
    gNdsArm7DldiSerialized = 1u;
    return true;
}

extern void __real_armCopyMem32(void *dst, const void *src, size_t size);
void __wrap_armCopyMem32(void *dst, const void *src, size_t size)
{
    __real_armCopyMem32(dst, src, size);
    if (dst != src && (uintptr_t)src == NDS_AUDIO_DLDI_ADDRESS && gNdsArm7DldiSerialized &&
        size >= sizeof(DldiHeader))
    {
        /* Calico's DumpDldi exports a relocatable driver for filesystem setup
         * and chainloading. Export its original interface, never pointers into
         * this application's ARM7 code. Driver code/data remain byte-identical. */
        ((DldiHeader *)dst)->disc = sDldiOriginal;
    }
}
