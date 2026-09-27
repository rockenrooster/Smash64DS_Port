#include <nds/nds_audio_extent.h>
#include <stddef.h>

static uint32_t ndsFat16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}
static uint32_t ndsFat32(const uint8_t *p)
{
    return ndsFat16(p) | (ndsFat16(p + 2) << 16);
}

int ndsAudioFatOpen(NdsAudioFatVolume *out, NdsAudioReadSector read,
                    void *user, uint32_t volume_sector, uint32_t device_sectors)
{
    uint8_t b[512] __attribute__((aligned(32)));
    uint32_t cluster_sectors, reserved, fats, roots, total, fat_sectors;
    uint32_t data_offset, clusters, bits, active = 0;
    uint64_t metadata, volume_end, fat_bytes;
    if (!out || !read || volume_sector >= device_sectors ||
        !read(user, volume_sector, b)) return 0;
    cluster_sectors = b[13];
    reserved = ndsFat16(b + 14);
    fats = b[16];
    roots = ndsFat16(b + 17);
    total = ndsFat16(b + 19);
    if (!total) total = ndsFat32(b + 32);
    fat_sectors = ndsFat16(b + 22);
    if (!fat_sectors) fat_sectors = ndsFat32(b + 36);
    if (ndsFat16(b + 510) != 0xaa55 || ndsFat16(b + 11) != 512 ||
        !cluster_sectors || cluster_sectors > 128 ||
        (cluster_sectors & (cluster_sectors - 1)) || !reserved ||
        !fats || fats > 2 || !fat_sectors || !total) return 0;
    metadata = reserved + (uint64_t)fats * fat_sectors + (roots * 32u + 511u) / 512u;
    volume_end = (uint64_t)volume_sector + total;
    if (metadata >= total || volume_end > device_sectors) return 0;
    data_offset = (uint32_t)metadata;
    clusters = (total - data_offset) / cluster_sectors;
    bits = clusters < 4085 ? 12 : clusters < 65525 ? 16 : 32;
    if (!clusters || clusters > 0x0ffffff3u ||
        ((bits == 32) != (ndsFat16(b + 22) == 0)) ||
        (bits == 32 && roots != 0)) return 0;
    if (bits == 32 && (ndsFat16(b + 40) & 0x80u))
    {
        active = ndsFat16(b + 40) & 15u;
        if (active >= fats) return 0;
    }
    fat_bytes = ((uint64_t)(clusters + 2u) * bits + 7u) / 8u;
    if (fat_bytes > (uint64_t)fat_sectors * 512u) return 0;
    *out = (NdsAudioFatVolume){
        volume_sector + reserved + active * fat_sectors, fat_sectors,
        volume_sector + data_offset, clusters, cluster_sectors, bits
    };
    return 1;
}

typedef struct NdsAudioFatCursor {
    uint8_t data[512] __attribute__((aligned(32)));
    uint32_t sector;
    int valid;
} NdsAudioFatCursor;

static int ndsFatNext(const NdsAudioFatVolume *v, NdsAudioReadSector read,
                       void *user, NdsAudioFatCursor *cache, uint32_t cluster,
                       uint32_t *next)
{
    uint32_t offset = v->entry_bits == 12 ? cluster + cluster / 2u :
                      cluster * (v->entry_bits / 8u);
    uint32_t bytes = v->entry_bits == 32 ? 4u : 2u, value = 0;
    for (uint32_t i = 0; i < bytes; ++i)
    {
        uint32_t sector = v->fat_sector + (offset + i) / 512u;
        if ((offset + i) / 512u >= v->fat_sectors) return 0;
        if (!cache->valid || cache->sector != sector)
        {
            if (!read(user, sector, cache->data)) return 0;
            cache->sector = sector;
            cache->valid = 1;
        }
        value |= (uint32_t)cache->data[(offset + i) % 512u] << (i * 8u);
    }
    if (v->entry_bits == 12) value = (value >> ((cluster & 1u) * 4u)) & 0xfffu;
    if (v->entry_bits == 32) value &= 0x0fffffffu;
    *next = value;
    return 1;
}

int ndsAudioFatExtents(const NdsAudioFatVolume *v, NdsAudioReadSector read,
                       void *user, uint32_t cluster, uint32_t file_bytes,
                       NdsAudioExtent *out, uint32_t capacity, uint32_t *count)
{
    NdsAudioFatCursor cache = { .valid = 0 };
    uint32_t remain = file_bytes / 512u + ((file_bytes % 512u) != 0u);
    uint32_t used = 0, file_sector = 0, previous_end = 0;
    uint32_t cycle_anchor = cluster, cycle_power = 1, cycle_distance = 0;
    if (!v || !read || !count || !file_bytes || !v->cluster_sectors ||
        (v->entry_bits != 12 && v->entry_bits != 16 && v->entry_bits != 32)) return 0;
    *count = 0;
    while (remain)
    {
        uint32_t sectors = remain < v->cluster_sectors ? remain : v->cluster_sectors;
        uint32_t first;
        uint64_t first64;
        uint32_t reserved_cluster = v->entry_bits == 12 ? 0xff0u :
                                    v->entry_bits == 16 ? 0xfff0u : 0x0ffffff0u;
        if (cluster < 2 || cluster >= reserved_cluster || cluster - 2 >= v->cluster_count) return 0;
        first64 = v->data_sector + (uint64_t)(cluster - 2) * v->cluster_sectors;
        if (first64 + sectors > UINT32_MAX) return 0;
        first = (uint32_t)first64;
        if (out)
        {
            for (uint32_t i = 0; i < used; ++i)
            {
                /* Repeated/overlapping clusters are corruption, including a
                 * cycle that happens to end inside the requested file prefix. */
                if (first < out[i].device_sector + out[i].sector_count &&
                    out[i].device_sector < first + sectors) return 0;
            }
        }
        if (used && previous_end == first)
        {
            if (out) out[used - 1].sector_count += sectors;
        }
        else
        {
            if (out)
            {
                if (used >= capacity) return 0;
                out[used] = (NdsAudioExtent){file_sector, first, sectors};
            }
            ++used;
        }
        previous_end = first + sectors;
        file_sector += sectors;
        remain -= sectors;
        if (!remain) break;
        if (!ndsFatNext(v, read, user, &cache, cluster, &cluster)) return 0;
        ++cycle_distance;
        if (cluster == cycle_anchor) return 0;
        if (cycle_distance == cycle_power)
        {
            cycle_anchor = cluster;
            cycle_distance = 0;
            if (cycle_power <= UINT32_MAX / 2u) cycle_power *= 2u;
        }
    }
    *count = used;
    return 1;
}

int ndsAudioExtentsValid(const NdsAudioExtent *map, uint32_t count,
                         uint32_t file_bytes, uint32_t device_sectors)
{
    uint32_t expected = 0;
    uint32_t total = file_bytes / 512u + ((file_bytes % 512u) != 0u);
    if (!map || !count || !file_bytes || count > total) return 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        const NdsAudioExtent *e = map + i;
        if (!e->sector_count || e->file_sector != expected ||
            e->sector_count > total - expected ||
            e->device_sector >= device_sectors ||
            e->sector_count > device_sectors - e->device_sector) return 0;
        expected += e->sector_count;
        for (uint32_t j = 0; j < i; ++j)
            if (e->device_sector < map[j].device_sector + map[j].sector_count &&
                map[j].device_sector < e->device_sector + e->sector_count) return 0;
    }
    return expected == total;
}

int ndsAudioExtentResolve(const NdsAudioExtent *map, uint32_t count,
                          uint32_t file_sector, uint32_t wanted_sectors,
                          uint32_t *device_sector, uint32_t *available_sectors)
{
    uint32_t lo = 0, hi = count;
    if (!map || !count || !wanted_sectors || !device_sector || !available_sectors)
        return 0;
    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo) / 2u;
        const NdsAudioExtent *e = map + mid;
        if (file_sector < e->file_sector) hi = mid;
        else if (file_sector - e->file_sector >= e->sector_count) lo = mid + 1u;
        else
        {
            uint32_t delta = file_sector - e->file_sector;
            uint32_t available = e->sector_count - delta;
            *device_sector = e->device_sector + delta;
            *available_sectors = available < wanted_sectors ? available : wanted_sectors;
            return 1;
        }
    }
    return 0;
}
