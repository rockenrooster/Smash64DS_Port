/* P2-7 save data: the DS stand-in for the source's SRAM. See nds_backup.h. */
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>

#include <nds/nds_backup.h>

static u8 sNdsBackupImage[NDS_BACKUP_IMAGE_BYTES];
static u32 sNdsBackupLoaded;
static u32 sNdsBackupMountTried;
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
static const char *const sNdsBackupPaths[] = {
    "fat:/smash64ds.sav",
    "sd:/smash64ds.sav",
};
static const char *const sNdsBackupTempPaths[] = {
    "fat:/smash64ds.sav.tmp",
    "sd:/smash64ds.sav.tmp",
};

static void ndsBackupMount(void)
{
    if (sNdsBackupMountTried != 0u)
    {
        return;
    }
    sNdsBackupMountTried = 1u;
    /* nitroFSInit already brought libfat up when the ROM runs from a card;
     * fatInitDefault is the documented way to make sure, and a volume that is
     * already mounted simply stays mounted. Its result is not trusted on its
     * own: the paths below are what decide whether there is a volume. */
    (void)fatInitDefault();
}

static void ndsBackupLoad(void)
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
        FILE *file = fopen(sNdsBackupPaths[i], "rb");

        if (file != NULL)
        {
            size_t got = fread(sNdsBackupImage, 1u, sizeof(sNdsBackupImage), file);

            fclose(file);
            sNdsBackupPath = sNdsBackupPaths[i];
            gNdsBackupLoadResult = (got != 0u) ? NDS_BACKUP_LOAD_FILE
                                              : NDS_BACKUP_LOAD_ABSENT;
            return;
        }
    }
    /* No file. Tell a volume with no save apart from no volume at all by
     * whether a temporary can be created; the write path probes the same way. */
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

s32 ndsBackupFlush(void)
{
    u32 i;

    ndsBackupLoad();
    for (i = 0u; i < sizeof(sNdsBackupPaths) / sizeof(sNdsBackupPaths[0]); i++)
    {
        FILE *file;
        size_t put;

        /* Prefer the volume the save was read from; fall through to the others
         * only when it has gone away. */
        if ((sNdsBackupPath != NULL) && (sNdsBackupPath != sNdsBackupPaths[i]))
        {
            continue;
        }
        file = fopen(sNdsBackupTempPaths[i], "wb");
        if (file == NULL)
        {
            continue;
        }
        put = fwrite(sNdsBackupImage, 1u, sizeof(sNdsBackupImage), file);
        if ((fclose(file) != 0) || (put != sizeof(sNdsBackupImage)))
        {
            remove(sNdsBackupTempPaths[i]);
            continue;
        }
        /* Replace, not overwrite: the old save survives until the new one is
         * complete on the volume. */
        remove(sNdsBackupPaths[i]);
        if (rename(sNdsBackupTempPaths[i], sNdsBackupPaths[i]) != 0)
        {
            continue;
        }
        sNdsBackupPath = sNdsBackupPaths[i];
        gNdsBackupWriteCount++;
        return TRUE;
    }
    gNdsBackupWriteFailCount++;
    return FALSE;
}
