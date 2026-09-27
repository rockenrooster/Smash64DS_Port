#ifndef NDS_AUDIO_EXTENT_H
#define NDS_AUDIO_EXTENT_H
#include <stdint.h>

typedef struct NdsAudioExtent {
    uint32_t file_sector;
    uint32_t device_sector;
    uint32_t sector_count;
} NdsAudioExtent;

typedef int (*NdsAudioReadSector)(void *user, uint32_t sector, uint8_t *out);

typedef struct NdsAudioFatVolume {
    uint32_t fat_sector;
    uint32_t fat_sectors;
    uint32_t data_sector;
    uint32_t cluster_count;
    uint32_t cluster_sectors;
    uint32_t entry_bits;
} NdsAudioFatVolume;

/* Boot-only FAT12/16/32 chain extraction. The mounted volume's sector base and
 * the opened ROM's first cluster come from the filesystem, not path guesses. */
int ndsAudioFatOpen(NdsAudioFatVolume *out, NdsAudioReadSector read,
                    void *user, uint32_t volume_sector, uint32_t device_sectors);
int ndsAudioFatExtents(const NdsAudioFatVolume *volume, NdsAudioReadSector read,
                       void *user, uint32_t first_cluster, uint32_t file_bytes,
                       NdsAudioExtent *out, uint32_t capacity, uint32_t *count);

/* Used by ARM7 before retaining a map, then for every bounded media request. */
int ndsAudioExtentsValid(const NdsAudioExtent *map, uint32_t count,
                         uint32_t file_bytes, uint32_t device_sectors);
int ndsAudioExtentResolve(const NdsAudioExtent *map, uint32_t count,
                          uint32_t file_sector, uint32_t wanted_sectors,
                          uint32_t *device_sector, uint32_t *available_sectors);
#endif
