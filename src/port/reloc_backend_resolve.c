/* Relocation asset loading, pointer resolution: lent holes, file lookup, the
 * pointer resolver, compact polygon models, file registration and internal
 * pointer fixups, heap-range release. A slice of reloc_backend.c. */

/* Loaded files whose dead bytes the battle lent out as allocation space
 * (ndsRelocLendDeadSpans): an address inside one of their holes belongs to
 * whatever was allocated there, not to the file. A heap generation's: the
 * stage's geometry and the match's content files (holes of file f are
 * [first[f], first[f + 1]), ascending). */
#define NDS_RELOC_HOLE_FILE_MAX 16u
#define NDS_RELOC_HOLE_MAX 64u
static const void *sNdsRelocHoleData[NDS_RELOC_HOLE_FILE_MAX];
static u8 sNdsRelocHoleFirst[NDS_RELOC_HOLE_FILE_MAX + 1u];
static u32 sNdsRelocHoleGeneration;
static u32 sNdsRelocHoleFileCount;
static u32 sNdsRelocHoleLo[NDS_RELOC_HOLE_MAX];
static u32 sNdsRelocHoleHi[NDS_RELOC_HOLE_MAX];

static sb32 ndsRelocRangeTouchesHole(const void *data, uintptr_t offset,
                                     size_t size)
{
    u32 f;
    u32 i;

    if (sNdsRelocHoleGeneration != gNdsTaskmanHeapGeneration)
    {
        return FALSE;
    }
    for (f = 0u; f < sNdsRelocHoleFileCount; f++)
    {
        if (sNdsRelocHoleData[f] != data)
        {
            continue;
        }
        for (i = sNdsRelocHoleFirst[f]; i < sNdsRelocHoleFirst[f + 1u]; i++)
        {
            if (offset + ((size != 0u) ? size : 1u) <= sNdsRelocHoleLo[i])
            {
                break;
            }
            if (offset < sNdsRelocHoleHi[i])
            {
                return TRUE;
            }
        }
        return FALSE;
    }
    return FALSE;
}

static s32 ndsRelocPointerRangeInLoadedFile(const NDSRelocLoadedFile *loaded,
                                             const void *ptr, size_t size)
{
    uintptr_t base;
    uintptr_t addr;

    if ((loaded == NULL) || (loaded->data == NULL) || (ptr == NULL))
    {
        return FALSE;
    }

    base = (uintptr_t)loaded->data;
    addr = (uintptr_t)ptr;
    if ((addr < base) || (addr > (base + loaded->data_size)))
    {
        return FALSE;
    }
    if ((sNdsRelocHoleFileCount != 0u) &&
        (ndsRelocRangeTouchesHole(loaded->data, addr - base, size) != FALSE))
    {
        return FALSE;
    }
    if (loaded->reserved[2] != 0u)
    {
        /* A compact polygon model owns only its backed range: the source
         * offsets around it are other allocations' bytes. */
        uintptr_t offset = addr - base;

        return ((offset >= loaded->compact_lo) &&
                (offset < loaded->compact_hi) &&
                (size <= (size_t)(loaded->compact_hi - offset))) ? TRUE :
                                                                   FALSE;
    }
    return ndsRelocRangeInLoadedFile(loaded, addr - base, size);
}

#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS
/* P2-2p8 T1: compact ground maps. scripts/stages/generate_compact_ground_maps.py
 * rewrites each of the nine VS maps so the stage wallpaper's Sprite and
 * Bitmap[44] table (776 B, 784 padded) ride at the map's tail and
 * MPGroundData.wallpaper points at them. That takes the 158,928-byte wallpaper
 * container out of the map's extern tree: 158,144 B less resident per stage.
 * Nothing read the container's pixels -- the background is the converted BG2
 * image, keyed by the IDENTITY of the Bitmap table -- and every source offset
 * in the map is unchanged, so what differs is confined to the seams below: the
 * stub Sprite's lane normalization (ndsRelocNormalizeCompactGroundMapSprite),
 * the wallpaper's identity (ndsRelocCompactGroundMapWallpaperKey), the size the
 * stage packet pins for the map (ndsRelocCompactGroundMapExpectedSize),
 * Training Mode's wallpaper region and the Dream Land reloc diagnostic in
 * ndsRelocFinalizeLoadedFile.
 *
 * At load, a map IS compact exactly when its own `wallpaper` pointer lands
 * inside its own file, so a source map that NitroFS still holds loads as it
 * always did. The packet size check cannot ask that way: Training Mode
 * rewrites `wallpaper` after load, so it asks the loaded size instead. The P1
 * build compiles none of this: its packets, rows and pins are the source sizes. */
#define NDS_RELOC_COMPACT_GROUND_MAP_GROWTH 784u
#define NDS_RELOC_COMPACT_WALLPAPER_BITMAP 0x269c8u

/* gr/grdef.h order. The wallpaper is the container each map's header named
 * before the rewrite, which is the row identity in kNDSNativeWallpapers
 * (generate_native_wallpapers.py SOURCES); the header is where the map's
 * MPGroundData sits (reloc_data.h llGR*MapMapHeader). The 1P arenas and the
 * 24 boards joined 2026-10-01: each still loaded its 158,928-byte container
 * into every 1P battle, and the Polygon Team's twelve fighters then did not
 * fit. Race to the Finish names no wallpaper container and stays a plain map. */
static const struct {
    u32 map_asset_id;
    u32 wallpaper_asset_id;
    u32 header_offset;
} sNdsRelocCompactGroundMaps[] = {
    { NDS_RELOC_ASSET_GR_CASTLE_MAP, NDS_RELOC_ASSET_MV_OPENING_ROOM_WALLPAPER, 0x14u },
    { NDS_RELOC_ASSET_GR_SECTOR_MAP, NDS_RELOC_ASSET_STAGE_SECTOR, 0x14u },
    { NDS_RELOC_ASSET_GR_JUNGLE_MAP, NDS_RELOC_ASSET_STAGE_JUNGLE, 0x14u },
    { NDS_RELOC_ASSET_GR_ZEBES_MAP, NDS_RELOC_ASSET_STAGE_ZEBES, 0x14u },
    { NDS_RELOC_ASSET_GR_HYRULE_MAP, NDS_RELOC_ASSET_STAGE_CASTLE, 0x14u },
    { NDS_RELOC_ASSET_GR_YOSTER_MAP, NDS_RELOC_ASSET_STAGE_YOSHI, 0x14u },
    { NDS_RELOC_ASSET_GR_PUPUPU_MAP, NDS_RELOC_ASSET_STAGE_DREAM_LAND, 0x14u },
    { NDS_RELOC_ASSET_GR_YAMABUKI_MAP, NDS_RELOC_ASSET_STAGE_POKEMON, 0x14u },
    { NDS_RELOC_ASSET_GR_INISHIE_MAP, NDS_RELOC_ASSET_STAGE_HYRULE_WALLPAPER, 0x14u }
#if NDS_P2_1P_GAME
    ,
    { NDS_RELOC_ASSET_GR_YOSTER_SMALL_MAP, NDS_RELOC_ASSET_STAGE_YOSHI, 0x14u },
    { NDS_RELOC_ASSET_GR_METAL_MAP, NDS_RELOC_ASSET_STAGE_METAL_WALLPAPER, 0x14u },
    { NDS_RELOC_ASSET_GR_ZAKO_MAP, NDS_RELOC_ASSET_STAGE_ZAKO_WALLPAPER, 0x14u },
    { NDS_RELOC_ASSET_GR_LAST_MAP, NDS_RELOC_ASSET_STAGE_LAST_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_MARIO_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_FOX_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_DONKEY_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_SAMUS_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_LUIGI_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_LINK_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_YOSHI_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_CAPTAIN_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_KIRBY_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_PIKACHU_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_PURIN_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS1_NESS_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_MARIO_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_FOX_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_DONKEY_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_SAMUS_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_LUIGI_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_LINK_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_YOSHI_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_CAPTAIN_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_KIRBY_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_PIKACHU_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_PURIN_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u },
    { NDS_RELOC_ASSET_GR_BONUS2_NESS_MAP, NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER, 0x0u }
#endif
#if NDS_P5
#include "nds_p5_native_stages.generated.h"
    /* P5: the Remix stages' maps, compacted the same way
     * (scripts/p5/stage_generate.py stage_nitro_files). */
#define NDS_P5_COMPACT_MAP_ROW(map_file, wallpaper_file, header) \
    , { (map_file), (wallpaper_file), (header) }
    NDS_P5_COMPACT_MAP_ROWS(NDS_P5_COMPACT_MAP_ROW)
#undef NDS_P5_COMPACT_MAP_ROW
#endif
};

/* Compact maps whose stub Sprite was normalized (once per load), and wallpaper
 * draws keyed through a compact map. Both read 0 when NitroFS holds source maps. */
__attribute__((used)) volatile u32 gNdsRelocCompactGroundMapCount;
__attribute__((used)) volatile u32 gNdsRelocCompactGroundMapKeyCount;

/* The scene's compact map, recorded once when its stub Sprite is normalized:
 * the wallpaper key is asked every frame and the stage packet's map size on
 * every revalidation, and both used to scan the loaded-file table to find
 * what this record already says. Valid for the scene generation it was taken
 * in; anything it does not match takes the scanning path below. */
static struct
{
    u32 scene_generation;
    u32 map_asset_id;
    u32 bitmap_offset;
    u32 wallpaper_asset_id;
} sNdsRelocCompactGroundMapActive;

static __attribute__((noinline)) s32 ndsRelocCompactGroundMapIndex(u32 asset_id)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(sNdsRelocCompactGroundMaps); i++)
    {
        if (sNdsRelocCompactGroundMaps[i].map_asset_id == asset_id)
        {
            return (s32)i;
        }
    }
    return -1;
}

/* The Sprite a compact map carries at its tail, or NULL for anything else. */
static __attribute__((noinline)) Sprite *ndsRelocCompactGroundMapSprite(
    const NDSRelocLoadedFile *loaded)
{
    const MPGroundData *ground_data;
    Sprite *sprite;
    s32 index = (loaded != NULL) ?
        ndsRelocCompactGroundMapIndex(loaded->asset_id) : -1;
    u32 header;

    if (index < 0)
    {
        return NULL;
    }
    header = sNdsRelocCompactGroundMaps[index].header_offset;
    if (ndsRelocRangeInLoadedFile(loaded, header, sizeof(MPGroundData)) ==
        FALSE)
    {
        return NULL;
    }
    ground_data = (const MPGroundData *)((const u8 *)loaded->data + header);
    sprite = ground_data->wallpaper;
    return (ndsRelocPointerRangeInLoadedFile(loaded, sprite,
                                             sizeof(Sprite)) != FALSE) ?
        sprite : NULL;
}

/* The size a ground map's packet pins is its SOURCE size; a compact map is that
 * plus its stub. Decided by the size that was loaded rather than by the
 * `wallpaper` pointer, which Training Mode rewrites after load. */
static __attribute__((noinline)) u32 ndsRelocCompactGroundMapExpectedSize(
    u32 asset_id, u32 source_size)
{
    const NDSRelocLoadedFile *loaded;

    if ((sNdsRelocCompactGroundMapActive.map_asset_id == asset_id) &&
        (sNdsRelocCompactGroundMapActive.scene_generation ==
         sNdsRelocSceneGeneration))
    {
        return source_size + NDS_RELOC_COMPACT_GROUND_MAP_GROWTH;
    }
    if (ndsRelocCompactGroundMapIndex(asset_id) < 0)
    {
        return source_size;
    }
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    return ((loaded != NULL) &&
            (loaded->data_size ==
             (source_size + NDS_RELOC_COMPACT_GROUND_MAP_GROWTH))) ?
        (source_size + NDS_RELOC_COMPACT_GROUND_MAP_GROWTH) : source_size;
}

/* The wallpaper's SObj points its Bitmap table at a compact map's stub, so
 * provenance names (map, stub offset). kNDSNativeWallpapers is written against
 * the source container's Bitmap table; translate to that key. FALSE, key
 * untouched, for anything that is not exactly a compact map's stub table. */
static __attribute__((noinline)) s32 ndsRelocCompactGroundMapWallpaperKey(
    u32 *asset_id, u32 *bitmap_offset)
{
    s32 index;
    const NDSRelocLoadedFile *loaded;
    const Sprite *sprite;

    if ((sNdsRelocCompactGroundMapActive.map_asset_id == *asset_id) &&
        (sNdsRelocCompactGroundMapActive.bitmap_offset == *bitmap_offset) &&
        (sNdsRelocCompactGroundMapActive.scene_generation ==
         sNdsRelocSceneGeneration))
    {
        *asset_id = sNdsRelocCompactGroundMapActive.wallpaper_asset_id;
        *bitmap_offset = NDS_RELOC_COMPACT_WALLPAPER_BITMAP;
        gNdsRelocCompactGroundMapKeyCount++;
        return TRUE;
    }
    index = ndsRelocCompactGroundMapIndex(*asset_id);
    if (index < 0)
    {
        return FALSE;
    }
    loaded = ndsRelocFindLoadedFileByAsset(*asset_id);
    sprite = ndsRelocCompactGroundMapSprite(loaded);
    if ((sprite == NULL) ||
        ((const u8 *)sprite->bitmap !=
         ((const u8 *)loaded->data + *bitmap_offset)))
    {
        return FALSE;
    }
    *asset_id = sNdsRelocCompactGroundMaps[index].wallpaper_asset_id;
    *bitmap_offset = NDS_RELOC_COMPACT_WALLPAPER_BITMAP;
    gNdsRelocCompactGroundMapKeyCount++;
    return TRUE;
}

#endif

/* R2-07 cycle 109. The memo is four-way move-to-front, not one-deep.
 *
 * This is 1 of the 30 callers' shared lookup and it sits under the fighter draw
 * path: `ndsRendererAdapterValidateNativeOwnerMaterials` asks it for
 * `palette_image`, `block_image` and `current_image` -- THREE DIFFERENT pointers
 * per material, per fighter, per frame -- and `ndsFighterDrawPlanResolve`,
 * `ndsFighterDLDraw*`, `ndsFighterDLScan*` and `ndsFighterDLExec*` interleave
 * their own streams on top, so a one-entry memo is being asked to serve several
 * interleaved streams.
 *
 * MEASURED, and it corrects the premise this was written on. Over a 60-second
 * both-CPU match: 30,385 calls, way 0 alone **25,434 (83.7%)**, ways 1-3
 * **4,385 more (14.4%)**, only 1,014 reaching the linear scan. So the one-entry
 * memo was already catching most of it, and the total volume is ~15 calls per
 * frame rather than the ~178,000 per match the material seam's call count
 * suggested -- because `ValidateNativeOwnerMaterials` sits behind the owner
 * validate cache, which cycle 98 measured at 3,961/2, so its three searches per
 * material are already elided ~999 times in 1,000.
 *
 * The 4,385 deleted scans are worth on the order of 500 ticks/frame: real,
 * repeatable, free in footprint (binary byte-identical, 160 bytes of headroom),
 * and far below anything this instrument can resolve. Banked under "keep every
 * repeatable correctness-preserving gain", not headlined. Do not brief this as a
 * lever; the counters are kept so the seam never has to be guessed at again.
 *
 * Four ways, checked most-recently-used first, so K=1 behaviour is the w==0 case
 * and nothing about the search ORDER changes for a single stream.
 *
 * Return-value equivalence rests on at most ONE entry being able to match a
 * given pointer, and the non-obvious half of that is the boundary:
 * `ndsRelocPointerRangeInLoadedFile` accepts `addr == base + data_size` (it
 * tests `>`, not `>=`), so two adjacent allocations would both appear to contain
 * a pointer sitting exactly on the seam. They do not, because the inner
 * `ndsRelocRangeInLoadedFile` then rejects `size > data_size - offset`, and at
 * that boundary the remainder is 0 while every caller passes size >= 1. So
 * "first match in scan order" and "the matching way" name the same file. Where
 * the previous code already preferred its one memo over scan order, this prefers
 * one of four the same way -- the hazard class is unchanged, not widened.
 *
 * `ndsRelocPointerRangeInLoadedFile` is two compares and a range check, so a way
 * that misses costs almost nothing; the win is deleting whole scans. */
#define NDS_RELOC_FIND_MEMO_WAYS 4u

volatile u32 gNdsRelocFindMemoHits;
volatile u32 gNdsRelocFindMemoWay0;
volatile u32 gNdsRelocFindMemoScans;
volatile u32 gNdsRelocFindMemoAbsent;

/* File scope, not function-local, so linker/nds_hot_text.ld can name its
 * input section (.data.sNdsRelocFindContainingMemo) and keep it in DTCM. */
static u32 sNdsRelocFindContainingMemo[NDS_RELOC_FIND_MEMO_WAYS] = {
    0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
};

static NDSRelocLoadedFile *ndsRelocFindLoadedFileContaining(const void *ptr,
                                                             size_t size)
{
    u32 *memo = sNdsRelocFindContainingMemo;
    u32 w;
    u32 i;

    for (w = 0u; w < NDS_RELOC_FIND_MEMO_WAYS; w++)
    {
        u32 idx = memo[w];

        if ((idx < sNdsRelocLoadedFileCount) &&
            (ndsRelocPointerRangeInLoadedFile(&sNdsRelocLoadedFiles[idx],
                                              ptr,
                                              size) != FALSE))
        {
            NDS_DIAG(gNdsRelocFindMemoHits++);
            if (w == 0u)
            {
                NDS_DIAG(gNdsRelocFindMemoWay0++);
            }
            else
            {
                /* Move to front, so a stream that keeps hitting way 3 does not
                 * keep paying for three probes. */
                while (w > 0u)
                {
                    memo[w] = memo[w - 1u];
                    w--;
                }
                memo[0] = idx;
            }
            return &sNdsRelocLoadedFiles[idx];
        }
    }

    NDS_DIAG(gNdsRelocFindMemoScans++);
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                             ptr,
                                             size) != FALSE)
        {
            for (w = NDS_RELOC_FIND_MEMO_WAYS - 1u; w > 0u; w--)
            {
                memo[w] = memo[w - 1u];
            }
            memo[0] = i;
            return &sNdsRelocLoadedFiles[i];
        }
    }
    NDS_DIAG(gNdsRelocFindMemoAbsent++);
    return NULL;
}

void *ndsRelocLendDeadSpans(u32 asset_id, u32 expect_size, const u32 *lo,
                            const u32 *hi, u32 count)
{
    const NDSRelocLoadedFile *loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    u32 first;
    u32 i;

    if (sNdsRelocHoleGeneration != gNdsTaskmanHeapGeneration)
    {
        sNdsRelocHoleFileCount = 0u;
        sNdsRelocHoleFirst[0] = 0u;
        sNdsRelocHoleGeneration = gNdsTaskmanHeapGeneration;
    }
    first = sNdsRelocHoleFirst[sNdsRelocHoleFileCount];
    /* This scene's own copy, whole (a compact model owns only part of its
     * range), once. */
    if ((loaded == NULL) || (loaded->data == NULL) ||
        (loaded->data_size != expect_size) || (loaded->reserved[2] != 0u) ||
        (loaded->owner_scene != (u32)gSCManagerSceneData.scene_curr) ||
        (loaded->owner_generation != sNdsRelocSceneGeneration) ||
        (count == 0u) || (sNdsRelocHoleFileCount >= NDS_RELOC_HOLE_FILE_MAX) ||
        (count > NDS_RELOC_HOLE_MAX - first))
    {
        return NULL;
    }
    for (i = 0u; i < sNdsRelocHoleFileCount; i++)
    {
        if (sNdsRelocHoleData[i] == loaded->data)
        {
            return NULL;
        }
    }
    for (i = 0u; i < count; i++)
    {
        if ((lo[i] >= hi[i]) || (hi[i] > loaded->data_size) ||
            (((lo[i] | hi[i]) & 15u) != 0u) ||
            ((((uintptr_t)loaded->data + lo[i]) & 15u) != 0u) ||
            ((i != 0u) && (lo[i] < hi[i - 1u])))
        {
            return NULL;
        }
    }
    for (i = 0u; i < count; i++)
    {
        sNdsRelocHoleLo[first + i] = lo[i];
        sNdsRelocHoleHi[first + i] = hi[i];
    }
    sNdsRelocHoleData[sNdsRelocHoleFileCount] = loaded->data;
    sNdsRelocHoleFileCount++;
    sNdsRelocHoleFirst[sNdsRelocHoleFileCount] = (u8)(first + count);
    /* A memoized containment answer may name this file for an address in a
     * hole the moment it is lent. */
    memset(sNdsRelocFindContainingMemo, 0xff,
           sizeof(sNdsRelocFindContainingMemo));
    return loaded->data;
}

s32 ndsRelocAddressInLentSpan(const void *ptr)
{
    const uintptr_t addr = (uintptr_t)ptr;
    u32 f;
    u32 i;

    if (sNdsRelocHoleGeneration != gNdsTaskmanHeapGeneration)
    {
        return FALSE;
    }
    for (f = 0u; f < sNdsRelocHoleFileCount; f++)
    {
        const uintptr_t base = (uintptr_t)sNdsRelocHoleData[f];

        for (i = sNdsRelocHoleFirst[f]; i < sNdsRelocHoleFirst[f + 1u]; i++)
        {
            if ((addr >= base + sNdsRelocHoleLo[i]) &&
                (addr < base + sNdsRelocHoleHi[i]))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

s32 ndsRelocPointerRangeInLoadedFiles(const void *ptr, size_t size)
{
    return (ndsRelocFindLoadedFileContaining(ptr, size) != NULL) ? TRUE :
                                                                  FALSE;
}

s32 ndsRelocGetLoadedPointerProvenance(const void *ptr, u32 *out_asset_id,
                                       u32 *out_offset)
{
    NDSRelocLoadedFile *loaded;

    if ((ptr == NULL) || (out_asset_id == NULL) || (out_offset == NULL))
    {
        return FALSE;
    }
    *out_asset_id = 0xffffffffu;
    *out_offset = 0xffffffffu;
    loaded = ndsRelocFindLoadedFileContaining(ptr, 1u);
    if (loaded == NULL)
    {
        return FALSE;
    }
    *out_asset_id = loaded->asset_id;
    *out_offset = (u32)((const u8 *)ptr - (const u8 *)loaded->data);
    return TRUE;
}

static size_t ndsRelocStatusNodeDataSize(const LBFileNode *node)
{
    NDSRelocLoadedFile *loaded;
    u32 asset_id;

    if ((node == NULL) || (node->addr == NULL))
    {
        return 0u;
    }

    loaded = ndsRelocFindLoadedFileByData(node->addr);
    if (loaded != NULL)
    {
        return loaded->data_size;
    }

    asset_id = ndsRelocAssetIDForToken((u32)node->id);
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if (loaded != NULL)
    {
        return loaded->data_size;
    }

    loaded = ndsRelocFindLoadedFileByAsset((u32)node->id);
    return (loaded != NULL) ? loaded->data_size : 0u;
}

static s32 ndsRelocPointerRangeInBuffer(const void *base, size_t data_size,
                                        const void *ptr, size_t size)
{
    uintptr_t start;
    uintptr_t addr;

    if ((base == NULL) || (ptr == NULL))
    {
        return FALSE;
    }

    start = (uintptr_t)base;
    addr = (uintptr_t)ptr;
    if ((addr < start) || (addr > (start + data_size)))
    {
        return FALSE;
    }
    return (((addr - start) <= data_size) &&
            (size <= (size_t)(data_size - (addr - start)))) ? TRUE : FALSE;
}

static s32 ndsRelocFindStatusNodeContaining(LBFileNode *nodes, s32 count,
                                            const void *ptr, size_t size,
                                            const void **out_base,
                                            size_t *out_size)
{
    s32 i;

    for (i = 0; i < count; i++)
    {
        size_t data_size = ndsRelocStatusNodeDataSize(&nodes[i]);

        if ((data_size != 0u) &&
            (ndsRelocPointerRangeInBuffer(nodes[i].addr, data_size, ptr,
                                          size) != FALSE))
        {
            if (out_base != NULL)
            {
                *out_base = nodes[i].addr;
            }
            if (out_size != NULL)
            {
                *out_size = data_size;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsRelocFindKnownFileContaining(const void *ptr, size_t size,
                                           const void **out_base,
                                           size_t *out_size)
{
    NDSRelocLoadedFile *loaded;

    /* The pack first, and it is O(1): a resident clip is never registered as a
     * loaded file, so without this the resolver would fall through to the
     * status buffers, find the clip's asset id mapped to the pack pointer, and
     * rebase slot words against a data_size taken from the FILE's header rather
     * than the blob's extent. Answering here is both faster and the only
     * correct bound. */
    if (ndsBattlePackContains(ptr, size, out_base, out_size) != FALSE)
    {
        return TRUE;
    }

    loaded = ndsRelocFindLoadedFileContaining(ptr, size);
    if (loaded != NULL)
    {
        if (out_base != NULL)
        {
            *out_base = loaded->data;
        }
        if (out_size != NULL)
        {
            *out_size = loaded->data_size;
        }
        return TRUE;
    }
    if (ndsRelocFindStatusNodeContaining(sNdsRelocForceStatusBuffer,
                                         sNdsRelocForceStatusBufferCount,
                                         ptr, size, out_base, out_size) != FALSE)
    {
        return TRUE;
    }
    return ndsRelocFindStatusNodeContaining(sNdsRelocStatusBuffer,
                                            sNdsRelocStatusBufferCount,
                                            ptr, size, out_base, out_size);
}

/* Offset counts the file-relative fallback below; Misalign counts the words that
 * fallback would have turned into an address no aligned type can live at, and
 * Value is the last one. Misalign > 0 is the freeze class being refused at its
 * source. Offset > 0 with Misalign 0 says the resolver ran and had nothing to
 * refuse -- which is a different statement from "this never ran", and the two
 * were indistinguishable before. Defined here for the same reason
 * gNdsRelocHeapDeclineCount is: this file's counter block is further down. */
volatile u32 gNdsRelocResolveOffsetCount;
volatile u32 gNdsRelocResolveMisalignCount;
volatile u32 gNdsRelocResolveMisalignValue;

/* THE SHIELD FREEZE, at the seam that manufactures the pointer.
 *
 * This resolver's contract is "give me a pointer to a `size`-byte object of this
 * file, or NULL". It had no alignment requirement, so its offset fallback --
 * `base + raw` for ANY stored word no larger than the file -- happily returned
 * addresses that no aligned type can live at. A figatree slot holding a plain
 * data word rather than a relocated pointer resolves that way: the caller
 * (lbCommonAddFighterPartsFigatree) hands it to gcAddDObjAnimJoint, and
 * gcParseDObjAnimJoint then walks it and never terminates, because a misaligned
 * LDR on ARM9 rotates the word it reads and the opcodes come out as noise. The
 * three captures on 2026-08-02/03 all froze there on 0x23842ea -- an in-file
 * address that is 2 mod 4, which is exactly the shape this function produced and
 * nothing else in the port produces.
 *
 * A misaligned result is therefore not a pointer to refuse to trust; it is not a
 * pointer at all. Return NULL, which every caller already handles -- the joint
 * gets AOBJ_ANIM_NULL and no animation, the same answer a NULL slot already
 * gets -- and count it, because "no misaligned words were seen" and "this code
 * never ran" have to be distinguishable. */
void *ndsRelocResolvePointerFromFileBase(const void *file_base,
                                         const void *ptr,
                                         size_t size)
{
    const void *base = NULL;
    size_t data_size = 0u;
    uintptr_t raw;
    uintptr_t align_mask;
    uintptr_t resolved;
    NDSRelocLoadedFile *relative_loaded;

    if (ptr == NULL)
    {
        return NULL;
    }
    align_mask = (size >= sizeof(u32)) ? (sizeof(u32) - 1u)
                                       : ((size >= sizeof(u16)) ? 1u : 0u);
    {
        /* A pinned cache clip is a stream clip: its slots are offsets, as the
         * registered heap copy's are (ndsRelocMarkLoadedFileRelativeOffsets),
         * and they resolve the same way against the clip's own base. */
        const NDSR2AnimPin *pin = ndsR2AnimPinForPayload(file_base);

        if (pin != NULL)
        {
            raw = (uintptr_t)ptr;
            if ((raw <= pin->size) && (size <= (size_t)(pin->size - raw)))
            {
                NDS_DIAG(gNdsRelocResolveOffsetCount++);
                resolved = (uintptr_t)pin->payload + raw;
                if ((resolved & align_mask) != 0u)
                {
                    gNdsRelocResolveMisalignCount++;
                    gNdsRelocResolveMisalignValue = (u32)resolved;
                    return NULL;
                }
                return (void *)resolved;
            }
        }
    }

    /* PlayersVS' two resident Selected clips are compiled directly from the
     * decomp's AObjEvent16 tables. Their top-level pointer table is copied into
     * the fighter's ordinary figatree heap (the source lifetime), but each
     * entry intentionally remains an absolute pointer to that immutable ROM
     * table. It therefore is neither a loaded reloc-file range nor a file-
     * relative offset, and the generic resolver below must not reject/rebase
     * it. The source parser consumes u16 commands, so two-byte alignment is the
     * actual contract here even though this legacy caller carries an
     * AObjEvent32-typed pointer. Keep this exemption exact to those 50 source
     * table entries; every normal gameplay clip still takes the reloc path. */
    if (ndsBattleShipIsCSSSelectedFigatreeJoint(ptr) != FALSE)
    {
        if (((uintptr_t)ptr & (sizeof(u16) - 1u)) != 0u)
        {
            gNdsRelocResolveMisalignCount++;
            gNdsRelocResolveMisalignValue = (u32)(uintptr_t)ptr;
            return NULL;
        }
        return (void *)ptr;
    }

    /* BPS1 slot words are offsets by construction, exactly like BPA2, but the
     * clip lives in the fighter's ordinary heap rather than one resident blob.
     * The generic absolute-pointer probe is catastrophically wrong for this
     * known format: every small offset misses all loaded ranges, then scans both
     * status buffers before rebasing it. Marking the registered file lets one
     * exact-base lookup answer offset-first and preserves the generic path for
     * every ordinary finalized O2R file. */
    relative_loaded = NULL;
    if ((sNdsRelocRelativeOffsetsMemoBase == file_base) &&
        (sNdsRelocRelativeOffsetsMemo != NULL) &&
        (sNdsRelocRelativeOffsetsMemo->data == file_base) &&
        (sNdsRelocRelativeOffsetsMemo->offsets_are_relative != FALSE))
    {
        relative_loaded = sNdsRelocRelativeOffsetsMemo;
    }
    else
    {
        relative_loaded = ndsRelocFindLoadedFileByData((void *)file_base);
        if ((relative_loaded != NULL) &&
            (relative_loaded->offsets_are_relative != FALSE))
        {
            sNdsRelocRelativeOffsetsMemo = relative_loaded;
            sNdsRelocRelativeOffsetsMemoBase = file_base;
        }
    }
    if ((relative_loaded != NULL) &&
        (relative_loaded->offsets_are_relative != FALSE))
    {
        raw = (uintptr_t)ptr;
        if ((raw <= relative_loaded->data_size) &&
            (size <= (size_t)(relative_loaded->data_size - raw)))
        {
            gNdsRelocResolveOffsetCount++;
            resolved = (uintptr_t)relative_loaded->data + raw;
            if ((resolved & align_mask) != 0u)
            {
                gNdsRelocResolveMisalignCount++;
                gNdsRelocResolveMisalignValue = (u32)resolved;
                return NULL;
            }
            return (u8 *)relative_loaded->data + raw;
        }
    }

    /* THE PACK'S SLOT WORDS ARE OFFSETS BY CONSTRUCTION, SO ASK THAT QUESTION
     * FIRST -- measured, this ordering was 92.4% of the resident pack's whole
     * P95 excess (`artifacts/performance/2026-08-15_battlepack-mechanism/`).
     *
     * The probe below asks "is `ptr` ALREADY an absolute pointer into a known
     * file?". For a clip served from the pack it can never be: the generator
     * emits blob-relative byte offsets, so `ptr` is a small integer. Its MISS
     * path is the expensive one -- `ndsRelocFindKnownFileContaining` falls
     * through the loaded-file scan into `ndsRelocFindStatusNodeContaining` over
     * BOTH status buffers, and every node visited there runs
     * `ndsRelocStatusNodeDataSize` -> `ndsRelocAssetIDForToken`, a ~300-compare
     * chain whose full miss also walks the 143 + 158 Mario/Fox pointer arrays.
     * At ~136 node visits and ~1,003 cycles each that is ~136,000 cycles PER
     * SLOT, and a fighter action change resolves ~18 of them: v3-arm measured
     * `ndsRelocAssetIDForToken` 207.9M cycles and
     * `ndsRelocFindStatusNodeContaining` 113.6M over 641 frames, 99.8%/100.0%
     * of it on the 69 frames that carry an acquisition.
     *
     * Answering from the pack costs one O(1) range test. It cannot change a
     * result: `data_size` is the blob extent (287,904 B) and every DS address is
     * >= 0x02000000, so a word that is genuinely an absolute pointer fails the
     * bound below, falls through, and takes the original path -- where
     * `ndsBattlePackContains` answers it O(1) anyway. Off a pack build this is a
     * compiled-out stub returning FALSE and the function is byte-identical.
     *
     * `#if`'d rather than left to that stub because a cross-TU call is NOT free
     * at flag 0 -- there is no LTO here, so the shipped ROM would grow a call
     * and a branch for a question that can never be answered yes. At flag 0 this
     * file must still compile to the same bytes it does today. */
    if (ndsBattlePackContains(file_base, 1u, &base, &data_size) != FALSE)
    {
        raw = (uintptr_t)ptr;
        if ((raw <= data_size) && (size <= (size_t)(data_size - raw)))
        {
            gNdsRelocResolveOffsetCount++;
            resolved = (uintptr_t)base + raw;
            if ((resolved & align_mask) != 0u)
            {
                gNdsRelocResolveMisalignCount++;
                gNdsRelocResolveMisalignValue = (u32)resolved;
                return NULL;
            }
            return (u8 *)base + raw;
        }
        base = NULL;
        data_size = 0u;
    }

    if (ndsRelocFindKnownFileContaining(ptr, size, NULL, NULL) != FALSE)
    {
        if (((uintptr_t)ptr & align_mask) != 0u)
        {
            gNdsRelocResolveMisalignCount++;
            gNdsRelocResolveMisalignValue = (u32)(uintptr_t)ptr;
            return NULL;
        }
        return (void *)ptr;
    }
    if (ndsRelocFindKnownFileContaining(file_base, 1u, &base, &data_size) ==
        FALSE)
    {
        return NULL;
    }

    raw = (uintptr_t)ptr;
    if ((raw > data_size) || (size > (size_t)(data_size - raw)))
    {
        return NULL;
    }
    gNdsRelocResolveOffsetCount++;
    resolved = (uintptr_t)base + raw;
    if ((resolved & align_mask) != 0u)
    {
        gNdsRelocResolveMisalignCount++;
        gNdsRelocResolveMisalignValue = (u32)resolved;
        return NULL;
    }
    return (u8 *)base + raw;
}

/* A registered file carries fresh big-endian bytes, so any exactly-once
 * record for its asset is stale even when the scene generation did not move
 * (same-scene reload reuses the slot and its generation). */
static void ndsRelocForgetNormalizedWeaponAttrs(u32 asset_id)
{
    u32 i = 0u;

    while (i < sNdsRelocNormalizedWeaponAttrCount)
    {
        if (sNdsRelocNormalizedWeaponAttrs[i].asset_id == asset_id)
        {
            sNdsRelocNormalizedWeaponAttrCount--;
            sNdsRelocNormalizedWeaponAttrs[i] =
                sNdsRelocNormalizedWeaponAttrs[
                    sNdsRelocNormalizedWeaponAttrCount];
            continue;
        }
        i++;
    }
}

/* COMPACT POLYGON MODELS (P7 C1, 2026-10-19). The Fighting Polygon Team loads
 * all eleven polygon models, and ~127 KB of them is N64 geometry the DS never
 * reads: the native owners draw from generated programs keyed by each display
 * list's source offset. scripts/fighters/generate_compact_polygon_models.py
 * stages each model as a 16-byte header ('NCPM', source size, X, cell count
 * << 16 | S), one {G_ENDDL, source offset} identity cell per display list
 * anything names, and the data the DS reads -- the joint tree and any DObj.dls
 * table -- byte for byte from source offset X. The record keeps source
 * offsets: data = storage + S - X and data_size = the source size, so the
 * joint tree's slots and other files' extern targets resolve unchanged, and
 * only [compact_lo, compact_hi) is backed (ndsRelocPointerRangeInLoadedFile).
 * A plain file (NDS_P2_COMPACT_POLYGON_MODELS=0) has no header and loads as
 * before. */
#define NDS_RELOC_COMPACT_MODEL_MAGIC 0x4e43504du
#define NDS_RELOC_COMPACT_MODEL_HEADER 16u

volatile u32 gNdsRelocCompactModelCount;
volatile u32 gNdsRelocCompactModelCellMisses;

/* A freshly read payload, still big-endian: TRUE and the record's geometry
 * when it is a compact polygon model. */
static s32 ndsRelocCompactModelProbe(u32 asset_id, const void *data,
                                     u32 payload_size, u32 *out_source_size,
                                     u32 *out_lo, u32 *out_hi)
{
    const u8 *bytes = data;
    u32 source_size;
    u32 x;
    u32 s;
    u32 cells;

    if ((asset_id < NDS_RELOC_ASSET_NMARIO_MODEL) ||
        (asset_id > NDS_RELOC_ASSET_NNESS_MODEL) || (bytes == NULL) ||
        (payload_size < NDS_RELOC_COMPACT_MODEL_HEADER) ||
        (ndsRelocReadBe32(bytes) != NDS_RELOC_COMPACT_MODEL_MAGIC))
    {
        return FALSE;
    }
    source_size = ndsRelocReadBe32(bytes + 4u);
    x = ndsRelocReadBe32(bytes + 8u);
    s = ndsRelocReadBe32(bytes + 12u) & 0xffffu;
    cells = ndsRelocReadBe32(bytes + 12u) >> 16;
    if ((s < NDS_RELOC_COMPACT_MODEL_HEADER + (cells * 8u)) ||
        (s > payload_size) || (x < s) || ((x & 3u) != 0u) ||
        (((x - s) & 3u) != 0u) ||
        ((x + (payload_size - s)) > source_size) ||
        ((x + (payload_size - s)) > 0xffffu))
    {
        return FALSE;
    }
    *out_source_size = source_size;
    *out_lo = x - s;
    *out_hi = x + (payload_size - s);
    return TRUE;
}

/* The identity cell standing for the display list at `source_offset`, or NULL.
 * Needs the swapped payload. */
static void *ndsRelocCompactModelCell(const NDSRelocLoadedFile *loaded,
                                      u32 source_offset)
{
    const u8 *header = (const u8 *)loaded->data + loaded->compact_lo;
    u8 *cells = (u8 *)loaded->data + loaded->compact_lo +
                NDS_RELOC_COMPACT_MODEL_HEADER;
    u32 lo = 0u;
    u32 hi = ndsRelocReadNative32(header + 12u) >> 16;

    while (lo < hi)
    {
        u32 mid = lo + ((hi - lo) >> 1);
        u32 cell_source = ndsRelocReadNative32(cells + (mid * 8u) + 4u);

        if (source_offset < cell_source)
        {
            hi = mid;
        }
        else if (source_offset > cell_source)
        {
            lo = mid + 1u;
        }
        else
        {
            return cells + (mid * 8u);
        }
    }
    gNdsRelocCompactModelCellMisses++;
    return NULL;
}

/* Where a fixup into `loaded` lands: the source offset, or for a compact
 * model's unbacked display list its cell (NULL when it has none). */
static void *ndsRelocLoadedFileTarget(const NDSRelocLoadedFile *loaded,
                                      u32 offset)
{
    if ((loaded->reserved[2] != 0u) &&
        ((offset < loaded->compact_lo) || (offset >= loaded->compact_hi)))
    {
        return ndsRelocCompactModelCell(loaded, offset);
    }
    return (u8 *)loaded->data + offset;
}

/* The first byte of a record's storage (a compact model's `data` is a
 * virtual base below it). */
static const void *ndsRelocLoadedFileStorage(const NDSRelocLoadedFile *loaded)
{
    return (loaded->reserved[2] != 0u) ?
        (const void *)((const u8 *)loaded->data + loaded->compact_lo) :
        loaded->data;
}

static NDSRelocLoadedFile *ndsRelocRegisterLoadedFileImpl(
    u32 asset_id, u32 bit, void *data, const NDSRelocAssetHeader *header,
    const u16 *known_extern_file_ids, u32 known_extern_count,
    sb32 is_known_extern_table)
{
    NDSRelocLoadedFile *loaded;
    u16 read_ids[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    const u16 *source_ids = known_extern_file_ids;
    u16 *owned_ids = NULL;
    u32 count;

    if ((sNdsRelocPreparedAnim.valid != 0u) &&
        (sNdsRelocPreparedAnim.asset_id == asset_id) &&
        (sNdsRelocPreparedAnim.data == data) &&
        (sNdsRelocPreparedAnim.epoch == sNdsRelocLoadedFilesEpoch) &&
        (sNdsRelocPreparedAnim.count == sNdsRelocLoadedFileCount))
    {
        loaded = (sNdsRelocPreparedAnim.index >= 0) ?
            &sNdsRelocLoadedFiles[sNdsRelocPreparedAnim.index] : NULL;
    }
    else
    {
        loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    }
    sNdsRelocPreparedAnim.valid = 0u;

    if ((loaded == NULL) &&
        (sNdsRelocLoadedFileCount >= NDS_RELOC_LOADED_FILE_CAPACITY))
    {
        gNdsOpeningRoomRelocPointerFixupFailCount++;
        return NULL;
    }
    if ((header == NULL) ||
        (header->extern_file_ids_num > NDS_RELOC_EXTERN_FILE_ID_CAPACITY))
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    count = header->extern_file_ids_num;
    if (count != 0u)
    {
        /* Validate before publishing or overwriting a resident record. The
         * known table can belong to the caller's stack, so it must be copied. */
        if (is_known_extern_table != FALSE)
        {
            if ((source_ids == NULL) || (known_extern_count != count))
            {
                ndsRelocRecordExternalFixupFail(asset_id);
                return NULL;
            }
        }
        else
        {
            u32 read_count = 0u;
            if ((ndsRelocAssetReadExternFileIDs(asset_id, read_ids,
                     NDS_RELOC_EXTERN_FILE_ID_CAPACITY, &read_count) == FALSE) ||
                (read_count != count))
            {
                ndsRelocRecordExternalFixupFail(asset_id);
                return NULL;
            }
            source_ids = read_ids;
        }
        /* A file's dependency count is fixed by its asset header. Reuse that
         * file's allocation on reload; a changed header gets an exact new
         * block. Both blocks have the same lifetime as the scene data. */
        if ((loaded != NULL) && (loaded->extern_count == count))
        {
            owned_ids = loaded->extern_file_ids;
        }
        if (owned_ids == NULL)
        {
            owned_ids = syTaskmanMalloc(count * sizeof(*owned_ids), 4u);
            if (owned_ids == NULL)
            {
                ndsRelocRecordExternalFixupFail(asset_id);
                return NULL;
            }
            /* An allocation can take the anim cache's elastic bytes back,
             * and a pinned clip's rescue then compacts sNdsRelocLoadedFiles
             * (see ndsRelocForceLoadFighterAObj16File): find the record
             * again rather than write through the one looked up before. */
            loaded = ndsRelocFindLoadedFileByAsset(asset_id);
            if ((loaded == NULL) &&
                (sNdsRelocLoadedFileCount >= NDS_RELOC_LOADED_FILE_CAPACITY))
            {
                gNdsOpeningRoomRelocPointerFixupFailCount++;
                return NULL;
            }
        }
        memcpy(owned_ids, source_ids, count * sizeof(*owned_ids));
    }
    if (loaded == NULL)
    {
        loaded = &sNdsRelocLoadedFiles[sNdsRelocLoadedFileCount++];
    }
#if defined(NDS_P2_COMPACT_GROUND_MAPS) && NDS_P2_COMPACT_GROUND_MAPS
    /* A re-registered map is compact again only once its stub is normalized. */
    if (sNdsRelocCompactGroundMapActive.map_asset_id == asset_id)
    {
        sNdsRelocCompactGroundMapActive.map_asset_id = 0u;
    }
#endif

    /* Registration owns every new or replacement stage data pointer. */
    if (ndsRelocIsDreamLandStageAsset(asset_id) != FALSE)
    {
        ndsRelocBumpStageAssetMutation();
    }
    ndsRelocForgetNormalizedWeaponAttrs(asset_id);
    loaded->asset_id = asset_id;
    loaded->bit = bit;
    loaded->data = data;
    sNdsRelocLoadedFilesEpoch++;
    loaded->data_size = header->data_size;
    loaded->owner_scene = (u32)gSCManagerSceneData.scene_curr;
    loaded->owner_generation = sNdsRelocSceneGeneration;
    loaded->reloc_intern_offset = header->reloc_intern_offset;
    loaded->reloc_extern_offset = header->reloc_extern_offset;
    loaded->extern_count = count;
    loaded->extern_file_ids = owned_ids;
    loaded->external_fixup_count = 0;
    loaded->external_fixup_fail_count = 0;
    loaded->internal_fixup_count = 0;
    loaded->internal_fixups_applied = FALSE;
    loaded->external_fixups_applied = FALSE;
    loaded->format_fixups_applied = FALSE;
    loaded->fixups_applying = FALSE;
    loaded->offsets_are_relative = FALSE;
    loaded->reserved[0] = loaded->reserved[1] = loaded->reserved[2] = 0u;
    loaded->compact_lo = loaded->compact_hi = 0u;
    {
        u32 source_size;
        u32 lo;
        u32 hi;

        if (ndsRelocCompactModelProbe(asset_id, data, header->data_size,
                                      &source_size, &lo, &hi) != FALSE)
        {
            loaded->data = (u8 *)data - lo;
            loaded->data_size = source_size;
            loaded->compact_lo = (u16)lo;
            loaded->compact_hi = (u16)hi;
            loaded->reserved[2] = 1u;
            gNdsRelocCompactModelCount++;
        }
    }
    if (sNdsRelocRelativeOffsetsMemo == loaded)
    {
        sNdsRelocRelativeOffsetsMemo = NULL;
        sNdsRelocRelativeOffsetsMemoBase = NULL;
    }
    return loaded;
}

static NDSRelocLoadedFile *ndsRelocRegisterLoadedFile(
    u32 asset_id, u32 bit, void *data, const NDSRelocAssetHeader *header)
{
    return ndsRelocRegisterLoadedFileImpl(asset_id, bit, data, header, NULL, 0u,
                                          FALSE);
}

/* A block the port builds itself (P7's unlock message: its Bitmap row and
 * texels), registered as this scene's loaded file so the sprite presenter --
 * which keys its cells by file provenance (nds_source2d.c) -- draws it. The
 * asset id is outside the ROM's and the P4 files'; nothing in it needs fixing
 * up. */
s32 ndsRelocRegisterSyntheticFile(u32 asset_id, void *data, u32 size)
{
    NDSRelocAssetHeader header;
    NDSRelocLoadedFile *loaded;

    if ((data == NULL) || (size == 0u))
    {
        return FALSE;
    }
    memset(&header, 0, sizeof(header));
    header.data_size = size;
    loaded = ndsRelocRegisterLoadedFile(asset_id, 0u, data, &header);
    if (loaded == NULL)
    {
        return FALSE;
    }
    loaded->internal_fixups_applied = TRUE;
    loaded->external_fixups_applied = TRUE;
    loaded->format_fixups_applied = TRUE;
    return TRUE;
}

static void ndsRelocMarkLoadedFileRelativeOffsets(NDSRelocLoadedFile *loaded)
{
    if (loaded == NULL)
    {
        return;
    }
    loaded->internal_fixups_applied = TRUE;
    loaded->external_fixups_applied = TRUE;
    loaded->format_fixups_applied = TRUE;
    loaded->offsets_are_relative = TRUE;
    sNdsRelocRelativeOffsetsMemo = loaded;
    sNdsRelocRelativeOffsetsMemoBase = loaded->data;
}

static s32 ndsRelocApplyWordByteSwapRange(u32 asset_id, void *data,
                                           u32 byte_offset, u32 byte_count,
                                           sb32 mark_asset)
{
    u32 words;
    u32 i;

    if (data == NULL)
    {
        gNdsOpeningRoomRelocWordSwapFailCount++;
        return FALSE;
    }

    /* The streaming CSS loader applies this same conversion as each aligned
     * payload chunk lands. Keep the per-asset K0 marker at one event, matching
     * the atomic path, while the word counters still account every word. */
    if (mark_asset != FALSE)
    {
        NDS_K0_MARK(gNdsK0AfterGoByteSwaps, asset_id);
    }
    words = byte_count / sizeof(u32);
    for (i = 0; i < words; i++)
    {
        void *word = (u8 *)data + byte_offset + (i * sizeof(u32));

        ndsRelocWriteNative32(word, ndsRelocReadBe32(word));
    }

    if (asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        NDS_DIAG(gNdsStartupLogoRelocWordSwapCount += words);
    }
    else if (ndsRelocIsOpeningRoomAsset(asset_id) != FALSE)
    {
        gNdsOpeningRoomRelocWordSwapCount += words;
    }
    return TRUE;
}

static s32 ndsRelocApplyWordByteSwap(NDSRelocLoadedFile *loaded)
{
    if ((loaded == NULL) || (loaded->data == NULL))
    {
        gNdsOpeningRoomRelocWordSwapFailCount++;
        return FALSE;
    }
    if (loaded->reserved[2] != 0u)
    {
        return ndsRelocApplyWordByteSwapRange(
            loaded->asset_id, loaded->data, loaded->compact_lo,
            (u32)loaded->compact_hi - loaded->compact_lo, TRUE);
    }
    return ndsRelocApplyWordByteSwapRange(loaded->asset_id, loaded->data, 0u,
                                          loaded->data_size, TRUE);
}

/* P2-3f50 telemetry. Which files actually had their internal pointer chain
 * walked, and how long each chain was. A fighter whose parts container is
 * still raw when the game reads it either never reached this pass or its
 * chain does not cover the container, and one ring separates the two.
 * Each entry is (asset_id << 16) | min(fixed_count, 0xffff). */
#define NDS_RELOC_INTERNAL_FIXUP_RING 24u
__attribute__((used)) volatile u32
    gNdsRelocInternalFixupRing[NDS_RELOC_INTERNAL_FIXUP_RING];
__attribute__((used)) volatile u32 gNdsRelocInternalFixupRingCount;

/* P2-2p8 Phase 3 (A7): while non-NULL, the internal fixup walk records each
 * slot it patches (as its word index) so a file can later be copied into a
 * compact image and have every internal pointer re-seated exactly. */
#define NDS_RELOC_IF_COMPACT_MARK 0xe7u
static u16 *sNdsRelocSlotCapture;
static u32 sNdsRelocSlotCaptureActive;
static u32 sNdsRelocSlotCaptureCap;
static u32 sNdsRelocSlotCaptureCount;
static u32 sNdsRelocSlotCaptureOverflow;

/* P4 file slots whose relocation names a target past the file's end. */
__attribute__((used)) volatile u32 gNdsP4RelocOutOfRangeSlots;

static s32 ndsRelocApplyInternalPointerFixups(NDSRelocLoadedFile *loaded)
{
    u16 reloc_intern;
    u32 guard;
    u32 fixed_count = 0;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        gNdsOpeningRoomRelocPointerFixupFailCount++;
        return FALSE;
    }
    if (loaded->internal_fixups_applied != FALSE)
    {
        return TRUE;
    }

    reloc_intern = loaded->reloc_intern_offset;
    guard = (loaded->data_size / sizeof(u32)) + 1u;

    while (reloc_intern != 0xffffu)
    {
        uintptr_t slot_offset = (uintptr_t)reloc_intern * sizeof(u32);
        u32 reloc_word;
        u16 next_reloc;
        u16 target_words;
        uintptr_t target_offset;
        void *slot;
        void *target;

        if ((guard == 0) || ((slot_offset + sizeof(u32)) > loaded->data_size))
        {
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return FALSE;
        }
        guard--;

        slot = (u8 *)loaded->data + slot_offset;
        reloc_word = ndsRelocReadNative32(slot);
        next_reloc = (u16)(reloc_word >> 16);
        target_words = (u16)(reloc_word & 0xffffu);
        target_offset = (uintptr_t)target_words * sizeof(u32);

        if (target_offset >= loaded->data_size)
        {
            /* A Smash Remix file can name a target past its own end: Peach's
             * turnip graphics (0x1418, 0x1260 bytes) has two slots aimed at
             * 0x24C0 and 0x26C8. The N64 relocates without a bound and
             * nothing reads them; failing here failed her whole main file's
             * closure. A P4 file's chain goes on, the slot left NULL. */
            if (ndsP4RelocAssetPath(loaded->asset_id) != NULL)
            {
                gNdsP4RelocOutOfRangeSlots++;
                ndsRelocWriteNativePointer(slot, NULL);
                fixed_count++;
                reloc_intern = next_reloc;
                continue;
            }
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return FALSE;
        }

        target = (u8 *)loaded->data + target_offset;
        ndsRelocWriteNativePointer(slot, target);
        if ((sNdsRelocSlotCaptureActive != FALSE) &&
            (sNdsRelocSlotCapture != NULL))
        {
            if (sNdsRelocSlotCaptureCount < sNdsRelocSlotCaptureCap)
            {
                sNdsRelocSlotCapture[sNdsRelocSlotCaptureCount++] =
                    reloc_intern;
            }
            else
            {
                sNdsRelocSlotCaptureOverflow = TRUE;
            }
        }

        fixed_count++;
        reloc_intern = next_reloc;
    }

    loaded->internal_fixup_count = fixed_count;
    loaded->internal_fixups_applied = TRUE;
    if (gNdsRelocInternalFixupRingCount < NDS_RELOC_INTERNAL_FIXUP_RING)
    {
        gNdsRelocInternalFixupRing[gNdsRelocInternalFixupRingCount] =
            (((u32)loaded->asset_id & 0xffffu) << 16) |
            ((fixed_count > 0xffffu) ? 0xffffu : fixed_count);
    }
    gNdsRelocInternalFixupRingCount++;

    if (loaded->asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        NDS_DIAG(gNdsStartupLogoRelocPointerFixupCount += fixed_count);
    }
    else if (ndsRelocIsOpeningRoomAsset(loaded->asset_id) != FALSE)
    {
        gNdsOpeningRoomRelocPointerFixupCount += fixed_count;
    }
    if (ndsPupupuStageAssetBit(loaded->asset_id) != 0u)
    {
        NDS_DIAG(gNdsStagePupupuInternalFixupCount += fixed_count);
    }
    return TRUE;
}

static s32 ndsRelocIsGeneratedP2FighterAObj32Asset(u32 asset_id)
{
#define NDS_P2_FIGHTER_AOBJ32_TEST(symbol_, id_) \
    if (asset_id == (id_)) return TRUE;
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
#endif
    NDS_P2_SAMUS_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
#if NDS_P2_LINK
    NDS_P2_LINK_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
#endif
    NDS_P2_PIKACHU_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
    NDS_P2_YOSHI_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
    NDS_P2_NESS_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
    NDS_P2_PURIN_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
    NDS_P2_KIRBY_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
#if NDS_P2_1P_GAME
    /* Master Hand's AnimJoint motions (his Appear among them), routed by the
     * 1P rows: treated as AObj16 they skipped the script normalizer, every
     * joint parsed one raw word and stopped, and his Appear never ended --
     * he stayed a ghost nothing could damage (owner 10-01). */
    NDS_1P_BOSS_AOBJ32_ASSET_ROWS(NDS_P2_FIGHTER_AOBJ32_TEST)
#endif
#undef NDS_P2_FIGHTER_AOBJ32_TEST
    return FALSE;
}

static s32 ndsRelocIsFighterAObj32Asset(u32 asset_id)
{
    return ((asset_id == NDS_RELOC_ASSET_MARIO_ANIM_APPEAR1) ||
            (asset_id == NDS_RELOC_ASSET_MARIO_ANIM_APPEAR2) ||
            (asset_id == NDS_RELOC_ASSET_FOX_ANIM_APPEAR) ||
            (asset_id == NDS_RELOC_ASSET_FOX_ANIM_ARWING) ||
            (ndsP4IsFighterAnimEvent32(asset_id) != FALSE) ||
            (ndsRelocIsGeneratedP2FighterAObj32Asset(asset_id) != FALSE)) ?
               TRUE : FALSE;
}

static s32 ndsRelocIsFighterAObj16Asset(u32 asset_id)
{
    return ((ndsRelocIsFighterAnimID(asset_id) != FALSE) &&
            (ndsRelocIsFighterAObj32Asset(asset_id) == FALSE)) ? TRUE : FALSE;
}

s32 ndsRelocPointerIsFighterAObj16(const void *ptr)
{
    NDSRelocLoadedFile *loaded;

    /* PlayersVS' Mario/Fox Selected clips are compiled directly from the
     * BattleShip relocData AObjEvent16 tables (359/372).  The force-load seam
     * copies only their joint-pointer array into figatree_heap, so each joint
     * deliberately remains an absolute pointer to that immutable source table.
     *
     * The generic Event32 admission wrapper asks this predicate before it tries
     * to normalize a script.  Treating these source AObj16 joints as Event32
     * made the shell-driven battle reject the Selected pose (reason 2: not a
     * loaded reloc-file range), even though the pointer resolver immediately
     * below already carries the same exact 50-entry exemption.  Keep the type
     * decision at this one parser boundary: Selected is still the source's u16
     * FIGATREE, just resident in ROM instead of DMA-loaded into N64 RAM. */
    if (ndsBattleShipIsCSSSelectedFigatreeJoint(ptr) != FALSE)
    {
        return TRUE;
    }

    /* THE ADMISSION GATE, and the reason a resident clip has to be named here.
     *
     * `gcAddDObjAnimJoint` (`battleship_sys_objanim.c:1710`) admits a script
     * only if it is NULL, or this says AObj16, or `ndsAObjEvent32NormalizeScript`
     * succeeds on it. A pack clip is deliberately NOT registered as a loaded
     * file -- that registration is one of the things slice 1 phase 5 deletes --
     * so without this branch every packed script would fall through to the
     * AObjEvent32 normalizer, which would either refuse it (a joint silently
     * loses its animation) or write through a `const` pointer.
     *
     * The pack contains AObj16 fighter scripts and nothing else: the generator
     * routes the four AObjEvent32 ids (0x279 0x27a 0x309 0x30a) out by name,
     * never silently. */
    if (ndsBattlePackContains(ptr, sizeof(u16), NULL, NULL) != FALSE)
    {
        return TRUE;
    }

    loaded = ndsRelocFindLoadedFileContaining(ptr, sizeof(u16));
    return ((loaded != NULL) &&
            (ndsRelocIsFighterAObj16Asset(loaded->asset_id) != FALSE)) ?
               TRUE : FALSE;
}

/* Engagement proof, per the standing rule that an optimization which silently
 * never fires is indistinguishable from one that fired and saved nothing.
 * `Resolves` counts the calls to ndsRelocAssetIDForToken this scan actually
 * makes; `Visits` counts the nodes it walks. The ratio IS the cut. */
volatile u32 gNdsR2RelocAliasVisits;
volatile u32 gNdsR2RelocAliasResolves;

static void ndsRelocRemoveStatusNodeAt(LBFileNode *nodes, s32 *count, s32 index)
{
    s32 remaining;

    if ((nodes == NULL) || (count == NULL) || (index < 0) || (index >= *count))
    {
        return;
    }

    remaining = (*count - index) - 1;
    if (remaining > 0)
    {
        memmove(&nodes[index], &nodes[index + 1],
                (size_t)remaining * sizeof(nodes[0]));
    }
    (*count)--;
}

static void ndsRelocRemoveFighterAnimStatusAliases(LBFileNode *nodes,
                                                   s32 *count,
                                                   u32 asset_id,
                                                   void *data)
{
    s32 i = 0;

    if ((nodes == NULL) || (count == NULL) || (data == NULL))
    {
        return;
    }

    while (i < *count)
    {
        /* Slice 45. `ndsRelocAssetIDForToken` was resolved for EVERY node here
         * even though `addr == data` rejects almost all of them, and that
         * resolver is the single largest non-idle symbol in the whole-match
         * tail: +41,731 cyc/frame on 71 of the 80 costliest frames, 11,721,422
         * cycles a match, 28% of it concentrated in that tail. It is a ~550
         * instruction if-chain plus, on a miss, two pointer scans over all
         * 143 + 158 Mario/Fox animation ids (R2-06 E11) -- and a node whose
         * `addr` does not match is always a miss for the purposes of this test,
         * so every one of those walks was discarded.
         *
         * Reordering is exact, not an approximation. `&&` short-circuits, the
         * two operands commute here because `ndsRelocAssetIDForToken` is a pure
         * function of its argument -- `ndsRelocFileID` returns
         * `(u32)(uintptr_t)file_id`, the ADDRESS of a link-time global rather
         * than anything read from it, so the chain has no memory input and no
         * side effect (its body contains no assignment or counter; the same
         * property is what let Task 74 call a cache in front of it "provably
         * safe"). The surviving predicate is unchanged, so the set of removed
         * nodes is identical.
         *
         * This is deliberately NOT Task 74's memo, which is a dead lane: that
         * put three lookup arrays in .main.bss and lost to a chain of
         * branch-predictable link-time immediates that was already resident.
         * Nothing is added here -- the call count goes down. */
        NDS_DIAG(gNdsR2RelocAliasVisits++);
        if (nodes[i].addr == data)
        {
            u32 node_asset_id = ndsRelocAssetIDForToken((u32)nodes[i].id);

            NDS_DIAG(gNdsR2RelocAliasResolves++);
            if ((node_asset_id != asset_id) &&
                (ndsRelocIsFighterAnimID(node_asset_id) != FALSE))
            {
                ndsRelocRemoveStatusNodeAt(nodes, count, i);
                continue;
            }
        }
        i++;
    }
}

/* BattleShip's force loader treats `data` as an animation scratch heap and
 * overwrites it on every action change (lbreloc.c:363-369, ftmain.c:4621-4624).
 * The DS loaded-file table and Event32 normalization ledger are metadata the N64
 * source did not have, so both must retire the OLD occupant before those bytes
 * are replaced.  Do this from the one writer rather than teaching every parser
 * about heap generations.
 *
 * This intentionally covers AObj16 AND the four AObj32 Mario/Fox clips.  The old
 * helper removed only AObj16 aliases, which let an AObj32 loaded-file record and
 * its normalized-command addresses survive reuse of the same fighter heap. */
static void ndsRelocPrepareFighterAnimHeapOverwrite(u32 asset_id, void *data)
{
    u32 i = 0;
    size_t old_bytes = 0u;
    s32 asset_index = -1;

    if (data == NULL)
    {
        return;
    }

    while (i < sNdsRelocLoadedFileCount)
    {
        NDSRelocLoadedFile *loaded = &sNdsRelocLoadedFiles[i];

        /* The first entry of `asset_id`, in the table as it stands after this
         * loop: a removal only ever happens at the cursor, past any match
         * already recorded, so the recorded index never moves. */
        if ((asset_index < 0) && (loaded->asset_id == asset_id))
        {
            asset_index = (s32)i;
        }
        if ((loaded->data == data) &&
            (ndsRelocIsFighterAnimID(loaded->asset_id) != FALSE))
        {
            if (loaded->data_size > old_bytes)
            {
                old_bytes = loaded->data_size;
            }
            if (loaded->asset_id == asset_id)
            {
                i++;
                continue;
            }
            u32 remaining = (sNdsRelocLoadedFileCount - i) - 1u;

            if (remaining > 0u)
            {
                memmove(&sNdsRelocLoadedFiles[i],
                        &sNdsRelocLoadedFiles[i + 1u],
                        (size_t)remaining * sizeof(sNdsRelocLoadedFiles[0]));
            }
            sNdsRelocLoadedFileCount--;
            sNdsRelocLoadedFilesEpoch++;
            continue;
        }
        i++;
    }

    sNdsRelocPreparedAnim.asset_id = asset_id;
    sNdsRelocPreparedAnim.data = data;
    sNdsRelocPreparedAnim.epoch = sNdsRelocLoadedFilesEpoch;
    sNdsRelocPreparedAnim.count = sNdsRelocLoadedFileCount;
    sNdsRelocPreparedAnim.index = asset_index;
    sNdsRelocPreparedAnim.valid = 1u;
    if (old_bytes != 0u)
    {
        ndsAObjEvent32ForgetRange(data, old_bytes);
    }
    ndsRelocRemoveFighterAnimStatusAliases(sNdsRelocStatusBuffer,
                                           &sNdsRelocStatusBufferCount,
                                           asset_id, data);
    ndsRelocRemoveFighterAnimStatusAliases(sNdsRelocForceStatusBuffer,
                                            &sNdsRelocForceStatusBufferCount,
                                            asset_id, data);
}

static s32 ndsRelocPointerInHeapRange(const void *base, size_t size,
                                      const void *ptr)
{
    uintptr_t start;
    uintptr_t end;
    uintptr_t addr;

    if ((base == NULL) || (ptr == NULL) || (size == 0u))
    {
        return FALSE;
    }
    start = (uintptr_t)base;
    end = start + (uintptr_t)size;
    if (end < start)
    {
        return FALSE;
    }
    addr = (uintptr_t)ptr;
    return ((addr >= start) && (addr < end)) ? TRUE : FALSE;
}

void ndsRelocReleaseHeapRange(void *base, size_t size)
{
    u32 i = 0u;
    s32 status_i;

    if ((base == NULL) || (size == 0u))
    {
        return;
    }

    ndsAObjEvent32ForgetRange(base, size);
    while (i < sNdsRelocNormalizedMObjSubCount)
    {
        if (ndsRelocPointerInHeapRange(base, size,
                sNdsRelocNormalizedMObjSubs[i].record) != FALSE)
        {
            sNdsRelocNormalizedMObjSubCount--;
            sNdsRelocNormalizedMObjSubs[i] =
                sNdsRelocNormalizedMObjSubs[sNdsRelocNormalizedMObjSubCount];
            continue;
        }
        i++;
    }

    i = 0u;
    while (i < sNdsRelocLoadedFileCount)
    {
        if (ndsRelocPointerInHeapRange(base, size,
                ndsRelocLoadedFileStorage(&sNdsRelocLoadedFiles[i])) != FALSE)
        {
            u32 remaining;

            ndsRelocForgetNormalizedWeaponAttrs(
                sNdsRelocLoadedFiles[i].asset_id);
            remaining = (sNdsRelocLoadedFileCount - i) - 1u;
            if (remaining != 0u)
            {
                memmove(&sNdsRelocLoadedFiles[i],
                        &sNdsRelocLoadedFiles[i + 1u],
                        (size_t)remaining * sizeof(sNdsRelocLoadedFiles[0]));
            }
            sNdsRelocLoadedFileCount--;
            sNdsRelocLoadedFilesEpoch++;
            continue;
        }
        i++;
    }
    sNdsRelocRelativeOffsetsMemo = NULL;
    sNdsRelocRelativeOffsetsMemoBase = NULL;

    status_i = 0;
    while (status_i < sNdsRelocStatusBufferCount)
    {
        if (ndsRelocPointerInHeapRange(base, size,
                sNdsRelocStatusBuffer[status_i].addr) != FALSE)
        {
            ndsRelocRemoveStatusNodeAt(sNdsRelocStatusBuffer,
                                       &sNdsRelocStatusBufferCount,
                                       status_i);
            continue;
        }
        status_i++;
    }
    status_i = 0;
    while (status_i < sNdsRelocForceStatusBufferCount)
    {
        if (ndsRelocPointerInHeapRange(base, size,
                sNdsRelocForceStatusBuffer[status_i].addr) != FALSE)
        {
            ndsRelocRemoveStatusNodeAt(sNdsRelocForceStatusBuffer,
                                       &sNdsRelocForceStatusBufferCount,
                                       status_i);
            continue;
        }
        status_i++;
    }
}
