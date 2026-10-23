/* Save data, the DS stand-in for the source's SRAM: one image read at boot
 * and written back by ndsBackupFlush (lbbackup.c), on the flashcart's FAT
 * volume (fat:) or a DSi's SD (sd:), never the read-only nitro: filesystem;
 * and the P7 records' sibling files (open, transact, promote:
 * nds_p7_records.c). See nds_backup.h. */

#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>

#include <nds/nds_backup.h>
#include "nds_scene_harness_config.h"
#include <nds/nds_scene_harness.h>
#include <sc/scene.h>

static u8 sNdsBackupImage[NDS_BACKUP_IMAGE_BYTES] __attribute__((aligned(4)));
static u32 sNdsBackupLoaded;
static u32 sNdsBackupMountTried;
static u32 sNdsBackupPrimaryValid;
static u32 sNdsBackupTempValid;
/* The volume that answered, remembered so a write goes where the read came
 * from. NULL until a path opens. */
static const char *sNdsBackupPath;

__attribute__((used)) volatile u32 gNdsBackupLoadResult = NDS_BACKUP_LOAD_UNTRIED;
__attribute__((used)) volatile u32 gNdsBackupWriteCount;
__attribute__((used)) volatile u32 gNdsBackupWriteFailCount;
__attribute__((used)) volatile u32 gNdsBackupSoundMode;

/* Explicit device prefixes on purpose: nitroFSInit makes `nitro:` the default
 * device, so a bare relative path would resolve into the read-only ROM
 * filesystem. A flashcart mounts through DLDI as `fat:`; a DSi's SD as `sd:`. */
/* Harnesses force unlocks and accumulate scripted records. Exercise the same
 * storage path while keeping that data out of the player's save. */
#define NDS_BACKUP_FILENAME "smash64ds-diagnostic.sav"
static const char *const sNdsBackupPaths[] = {
    "fat:/" NDS_BACKUP_FILENAME,
    "sd:/" NDS_BACKUP_FILENAME,
};
static const char *const sNdsBackupTempPaths[] = {
    "fat:/" NDS_BACKUP_FILENAME ".tmp",
    "sd:/" NDS_BACKUP_FILENAME ".tmp",
};
static const char *const sNdsBackupPreviousPaths[] = {
    "fat:/" NDS_BACKUP_FILENAME ".bak",
    "sd:/" NDS_BACKUP_FILENAME ".bak",
};

/* The source accepts either complete SRAM copy, with signature 666 and its
 * weighted checksum (lbbackup.c:13-63). Check bytes actually read, so a short
 * file cannot acquire a valid tail from a previous recovery candidate. */
static s32 ndsBackupImageValid(size_t bytes)
{
    size_t offsets[] = { 0u, (sizeof(LBBackupData) + 15u) & ~(size_t)15u };
    u32 i;

    for (i = 0u; i < sizeof(offsets) / sizeof(offsets[0]); i++)
    {
        if (bytes >= offsets[i] + sizeof(LBBackupData))
        {
            LBBackupData *copy = (LBBackupData *)&sNdsBackupImage[offsets[i]];

            if ((copy->signature == 666) &&
                (copy->checksum == lbBackupCreateChecksum(copy)))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void ndsBackupMount(void)
{
    if (sNdsBackupMountTried != 0u)
    {
        return;
    }
    sNdsBackupMountTried = 1u;
    /* nitroFSInit already brought libfat up when the ROM runs from a flashcart
     * (fat:, through DLDI) or a DSi's SD (sd:), and that volume is the save's
     * volume too. Mount only when neither answered.
     *
     * fatInitDefault is NOT idempotent: libdvm's dvmInit builds a second
     * cached disc over the same DLDI interface and probes it again. Measured
     * 2026-10-02 at boot: two dvmDiscCacheCreate(16 pages x 8 sectors), the
     * second one this call's -- 65,832 B of libc heap below the taskman arena
     * for the whole run, which every scene's general heap paid, and two
     * independent sector caches over one card. Its result is not trusted on
     * its own: the paths below are what decide whether there is a volume. */
    if ((FindDevice("fat:") >= 0) || (FindDevice("sd:") >= 0))
    {
        return;
    }
    (void)fatInitDefault();
}

void ndsFsLock(void);
void ndsFsUnlock(void);

static void ndsBackupLoadUnlocked(void)
{
    u32 i;

    if (sNdsBackupLoaded != 0u)
    {
        return;
    }
    sNdsBackupLoaded = 1u;
    memset(sNdsBackupImage, 0, sizeof(sNdsBackupImage));
    ndsBackupMount();
    for (i = 0u; i < sizeof(sNdsBackupPaths) / sizeof(sNdsBackupPaths[0]); i++)
    {
        const char *candidates[] = {
            sNdsBackupPaths[i], sNdsBackupPreviousPaths[i], sNdsBackupTempPaths[i]
        };
        u32 candidate;

        for (candidate = 0u; candidate < sizeof(candidates) / sizeof(candidates[0]); candidate++)
        {
            FILE *file = fopen(candidates[candidate], "rb");

            if (file != NULL)
            {
                size_t got;

                memset(sNdsBackupImage, 0, sizeof(sNdsBackupImage));
                got = fread(sNdsBackupImage, 1u, sizeof(sNdsBackupImage), file);
                fclose(file);
                if (sNdsBackupPath == NULL)
                {
                    sNdsBackupPath = sNdsBackupPaths[i];
                }
                if (ndsBackupImageValid(got) != FALSE)
                {
                    sNdsBackupPath = sNdsBackupPaths[i];
                    sNdsBackupPrimaryValid = (candidate == 0u);
                    sNdsBackupTempValid = (candidate == 2u);
                    gNdsBackupLoadResult = NDS_BACKUP_LOAD_FILE;
                    return;
                }
            }
        }
    }
    memset(sNdsBackupImage, 0, sizeof(sNdsBackupImage));
    if (sNdsBackupPath != NULL)
    {
        gNdsBackupLoadResult = NDS_BACKUP_LOAD_ABSENT;
        return;
    }
    /* No file. Tell a volume with no save apart from no volume at all by
     * whether a temporary can be created. Every recovery candidate was checked
     * above before this probe can truncate a temporary from an interrupted save. */
    for (i = 0u; i < sizeof(sNdsBackupTempPaths) / sizeof(sNdsBackupTempPaths[0]); i++)
    {
        FILE *file = fopen(sNdsBackupTempPaths[i], "wb");

        if (file != NULL)
        {
            fclose(file);
            remove(sNdsBackupTempPaths[i]);
            sNdsBackupPath = sNdsBackupPaths[i];
            gNdsBackupLoadResult = NDS_BACKUP_LOAD_ABSENT;
            return;
        }
    }
    gNdsBackupLoadResult = NDS_BACKUP_LOAD_NO_VOLUME;
}

static void ndsBackupLoad(void)
{
    ndsFsLock();
    ndsBackupLoadUnlocked();
    ndsFsUnlock();
}

void ndsBackupSramRead(uintptr_t sram_src, void *ram_dst, size_t size)
{
    ndsBackupLoad();
    if ((ram_dst == NULL) || (sram_src >= NDS_BACKUP_IMAGE_BYTES) ||
        (size > NDS_BACKUP_IMAGE_BYTES - sram_src))
    {
        return;
    }
    memcpy(ram_dst, &sNdsBackupImage[sram_src], size);
}

void ndsBackupSramWrite(void *ram_src, uintptr_t sram_dst, size_t size)
{
    ndsBackupLoad();
    if ((ram_src == NULL) || (sram_dst >= NDS_BACKUP_IMAGE_BYTES) ||
        (size > NDS_BACKUP_IMAGE_BYTES - sram_dst))
    {
        return;
    }
    memcpy(&sNdsBackupImage[sram_dst], ram_src, size);
}

static s32 ndsBackupFlushUnlocked(void)
{
    u32 i;

    ndsBackupLoad();
    for (i = 0u; i < sizeof(sNdsBackupPaths) / sizeof(sNdsBackupPaths[0]); i++)
    {
        FILE *file;
        size_t put;

        /* Keep writes on the volume the save was read from. */
        if ((sNdsBackupPath != NULL) && (sNdsBackupPath != sNdsBackupPaths[i]))
        {
            continue;
        }
        /* A previous first-save promotion may have failed with only the
         * complete temporary left. Promote it before opening that name for
         * another write; a failed retry must leave it recoverable on reboot. */
        if ((sNdsBackupTempValid != 0u) && (sNdsBackupPrimaryValid == 0u))
        {
            remove(sNdsBackupPaths[i]); /* absent or rejected during load */
            if (rename(sNdsBackupTempPaths[i], sNdsBackupPaths[i]) != 0)
            {
                break;
            }
            sNdsBackupPrimaryValid = 1u;
            sNdsBackupTempValid = 0u;
        }
        file = fopen(sNdsBackupTempPaths[i], "wb");
        if (file == NULL)
        {
            continue;
        }
        sNdsBackupTempValid = 0u;
        put = fwrite(sNdsBackupImage, 1u, sizeof(sNdsBackupImage), file);
        if ((fclose(file) != 0) || (put != sizeof(sNdsBackupImage)))
        {
            remove(sNdsBackupTempPaths[i]);
            continue;
        }
        sNdsBackupTempValid = 1u;
        /* FAT rename need not replace an existing target. Keep the last valid
         * canonical file under a recovery name before promoting the new one;
         * never remove that recovery file when it was the only valid input. */
        if (sNdsBackupPrimaryValid != 0u)
        {
            remove(sNdsBackupPreviousPaths[i]);
            if (rename(sNdsBackupPaths[i], sNdsBackupPreviousPaths[i]) != 0)
            {
                break;
            }
            sNdsBackupPrimaryValid = 0u;
        }
        else
        {
            remove(sNdsBackupPaths[i]); /* absent or invalid, recovery remains */
        }
        if (rename(sNdsBackupTempPaths[i], sNdsBackupPaths[i]) != 0)
        {
            break;
        }
        sNdsBackupPath = sNdsBackupPaths[i];
        sNdsBackupPrimaryValid = 1u;
        sNdsBackupTempValid = 0u;
        NDS_DIAG(gNdsBackupWriteCount++);
        return TRUE;
    }
    gNdsBackupWriteFailCount++;
    return FALSE;
}

s32 ndsBackupFlush(void)
{
    s32 result;

    ndsFsLock();
    result = ndsBackupFlushUnlocked();
    ndsFsUnlock();
    return result;
}

/* Siblings: P7 U2's VS records (nds_p7_records.c), named after the save the
 * same way (a harness build's are diagnostic too). */
#define NDS_BACKUP_RECORDS_FILENAME "smash64ds-diagnostic-vs.sav"
#define NDS_BACKUP_1P_RECORDS_FILENAME "smash64ds-diagnostic-1p.sav"
#define NDS_BACKUP_SIBLING_NAMES(volume, name)     { volume name, volume name ".bak", volume name ".tmp" }
/* [sibling][volume]: the VS records (U2), the 1P records (C3). */
static const char *const sNdsBackupSiblingNames[][2][NDS_BACKUP_RECOVERY_NAMES] = {
    { NDS_BACKUP_SIBLING_NAMES("fat:/", NDS_BACKUP_RECORDS_FILENAME),
      NDS_BACKUP_SIBLING_NAMES("sd:/", NDS_BACKUP_RECORDS_FILENAME) },
    { NDS_BACKUP_SIBLING_NAMES("fat:/", NDS_BACKUP_1P_RECORDS_FILENAME),
      NDS_BACKUP_SIBLING_NAMES("sd:/", NDS_BACKUP_1P_RECORDS_FILENAME) },
};
_Static_assert(sizeof(sNdsBackupSiblingNames[0]) / sizeof(sNdsBackupSiblingNames[0][0]) ==
                   sizeof(sNdsBackupPaths) / sizeof(sNdsBackupPaths[0]),
               "one sibling name set per save volume");

/* The sibling's names on the save's volume, or NULL without one. */
static const char *const *ndsBackupSiblingNames(u32 sibling)
{
    u32 i;

    if (sibling >= sizeof(sNdsBackupSiblingNames) / sizeof(sNdsBackupSiblingNames[0]))
    {
        return NULL;
    }
    ndsBackupLoadUnlocked();
    for (i = 0u; i < sizeof(sNdsBackupPaths) / sizeof(sNdsBackupPaths[0]); i++)
    {
        if (sNdsBackupPath == sNdsBackupPaths[i])
        {
            return sNdsBackupSiblingNames[sibling][i];
        }
    }
    return NULL;
}

FILE *ndsBackupOpenSibling(u32 sibling, u32 recovery, const char *mode)
{
    const char *const *names = ndsBackupSiblingNames(sibling);

    if ((names == NULL) || (recovery >= NDS_BACKUP_RECOVERY_NAMES) || (mode == NULL))
    {
        return NULL;
    }
    return fopen(names[recovery], mode);
}

s32 ndsBackupPromoteSibling(u32 sibling)
{
    const char *const *names = ndsBackupSiblingNames(sibling);

    if (names == NULL)
    {
        return FALSE;
    }
    remove(names[0]); /* absent or rejected by the caller */
    return (rename(names[2], names[0]) == 0) ? TRUE : FALSE;
}

s32 ndsBackupTransactSibling(u32 sibling, NDSBackupWriter write, void *ctx, FILE **src)
{
    const char *const *names = ndsBackupSiblingNames(sibling);
    FILE *tmp;
    s32 ok;

    if ((names == NULL) || (write == NULL))
    {
        return FALSE;
    }
    tmp = fopen(names[2], "wb");
    if (tmp == NULL)
    {
        return FALSE;
    }
    ok = write(tmp, ctx);
    if (fclose(tmp) != 0)
    {
        ok = FALSE;
    }
    if ((src != NULL) && (*src != NULL))
    {
        fclose(*src);
        *src = NULL;
    }
    if (ok == FALSE)
    {
        remove(names[2]);
        return FALSE;
    }
    /* The save's promotion: FAT rename need not replace a target, so the
     * canonical file steps aside as .bak first (absent: nothing to keep). */
    remove(names[1]);
    (void)rename(names[0], names[1]);
    return (rename(names[2], names[0]) == 0) ? TRUE : FALSE;
}
