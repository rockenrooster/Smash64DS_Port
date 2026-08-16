#include <nds/nds_battlepack_anim.h>

/* `used` because a dropped diagnostic global has turned Boundary red here
 * before: `--gc-sections` removed five of them when their writers were compiled
 * out, and the verifier reported "Missing ELF symbol" rather than a flag. */
__attribute__((used)) volatile u32 gNdsBattlePackClips;
__attribute__((used)) volatile u32 gNdsBattlePackBytes;
__attribute__((used)) volatile u32 gNdsBattlePackHits;
__attribute__((used)) volatile u32 gNdsBattlePackMisses;
__attribute__((used)) volatile u32 gNdsBattlePackState;
__attribute__((used)) volatile u32 gNdsBattlePackLoadSteps;
__attribute__((used)) volatile u32 gNdsBattlePackLoadFails;
__attribute__((used)) volatile u32 gNdsBattlePackResidentBytes;
__attribute__((used)) volatile u32 gNdsBattlePackDrops;

#if NDS_R2_BATTLEPACK

/* The blob's runtime base, published by ndsBattlePackAdopt once the residency
 * loader has streamed the whole file into the taskman arena. NULL is the honest
 * "not resident" answer and every entry point returns it as a miss, so a failed
 * or unfinished load degrades to the generic acquisition path rather than
 * handing the parser a partial blob.
 *
 * NOT `const`: the storage is arena, and the arena is reclaimed by a scene
 * rewind. `ndsBattlePackDrop` is the seam that makes that visible. */
static const u8 *sNdsBattlePackBase;

/* THE SLICE-51 FALSIFIER'S SWITCH (lab; `NDS_R2_BATTLEPACK_DISPATCH`, default 1).
 *
 * At 0 the blob is still streamed, validated, adopted and carved out of the
 * arena -- LoadSteps, ResidentBytes, State and Bytes all read exactly as at 1 --
 * but no clip pointer ever leaves this module, so `gNdsBattlePackHits` reads 0
 * against a control that reads 197. That separates the blob's PRESENCE from its
 * DISPATCH, which is the last split the isolation arm could not make.
 *
 * A `volatile` global rather than an `#if` so both arms carry the same
 * instructions at the same addresses and the comparison does not also measure
 * the linker; the cost is one word of `.data` and a load/compare per lookup,
 * paid on both arms. `ndsBattlePackContains` is deliberately NOT gated: with no
 * pack pointer in circulation it answers FALSE anyway, and leaving it live keeps
 * its two call sites identical between the arms. */
__attribute__((used)) volatile u32 gNdsBattlePackDispatch =
    NDS_R2_BATTLEPACK_DISPATCH;

/* generate_battlepack_anim.py `emit_blob`. Every field is a byte offset from
 * the blob base except the two ids, which bound a binary search. */
typedef struct NDSBattlePackHeader
{
    u32 magic;
    u32 version;
    u32 blob_bytes;
    u32 clip_count;
    u32 dir_off;
    u32 table_off;
    u32 stream_off;
    u32 first_id;
    u32 last_id;
    u32 checksum;
} NDSBattlePackHeader;

typedef struct NDSBattlePackClip
{
    u16 asset_id;
    u16 slot_count;
    u32 slot_table_off;
} NDSBattlePackClip;

#define NDS_BATTLEPACK_MAGIC 0x32415042u /* "BPA2", little endian */
#define NDS_BATTLEPACK_VERSION 2u

/* Validated ONCE per residency, at adoption, and never re-validated per lookup:
 * the bytes do not change after the load completes, so a second read cannot
 * disagree with the first. The check is against a BUILD or TRANSFER mismatch (a
 * stale NitroFS payload, a format bump, a short read), which is exactly the
 * class generated-data drift has produced here before -- and the parser's
 * failure mode on a garbage script is a freeze, not a diagnostic. */
static const NDSBattlePackHeader *ndsBattlePackHeader(void)
{
    if ((sNdsBattlePackBase == NULL) ||
        (gNdsBattlePackState != NDS_BATTLEPACK_STATE_READY))
    {
        return NULL;
    }
    return (const NDSBattlePackHeader *)(const void *)sNdsBattlePackBase;
}

s32 ndsBattlePackAdopt(void *base, u32 bytes)
{
    const NDSBattlePackHeader *header =
        (const NDSBattlePackHeader *)(const void *)base;

    ndsBattlePackDrop();
    if ((base == NULL) || (bytes < sizeof(NDSBattlePackHeader)))
    {
        gNdsBattlePackState = NDS_BATTLEPACK_STATE_BAD_EXTENT;
        return FALSE;
    }
    if (header->magic != NDS_BATTLEPACK_MAGIC)
    {
        gNdsBattlePackState = NDS_BATTLEPACK_STATE_BAD_MAGIC;
        return FALSE;
    }
    if (header->version != NDS_BATTLEPACK_VERSION)
    {
        gNdsBattlePackState = NDS_BATTLEPACK_STATE_BAD_VERSION;
        return FALSE;
    }
    /* The blob's own declared length must equal what was actually streamed, and
     * every region it names must land inside it. A short read that still passed
     * magic and version is the failure this catches. */
    if ((header->blob_bytes != bytes) || (header->clip_count == 0u) ||
        (header->dir_off >= bytes) || (header->table_off >= bytes) ||
        (header->stream_off >= bytes) ||
        (header->clip_count >
         ((bytes - header->dir_off) / sizeof(NDSBattlePackClip))))
    {
        gNdsBattlePackState = NDS_BATTLEPACK_STATE_BAD_EXTENT;
        return FALSE;
    }
    sNdsBattlePackBase = (const u8 *)(const void *)base;
    gNdsBattlePackClips = header->clip_count;
    gNdsBattlePackBytes = bytes;
    gNdsBattlePackState = NDS_BATTLEPACK_STATE_READY;
    return TRUE;
}

void ndsBattlePackDrop(void)
{
    if (sNdsBattlePackBase != NULL)
    {
        gNdsBattlePackDrops++;
    }
    sNdsBattlePackBase = NULL;
    gNdsBattlePackClips = 0u;
    gNdsBattlePackBytes = 0u;
    gNdsBattlePackState = NDS_BATTLEPACK_STATE_UNCHECKED;
}

void *ndsBattlePackFindFigatree(u32 asset_id)
{
    const NDSBattlePackHeader *header = ndsBattlePackHeader();
    const NDSBattlePackClip *dir;
    u32 lo;
    u32 hi;

    if ((header == NULL) || (gNdsBattlePackDispatch == 0u))
    {
        return NULL;
    }
    if ((asset_id < header->first_id) || (asset_id > header->last_id))
    {
        return NULL;
    }
    dir = (const NDSBattlePackClip *)(const void *)
        &sNdsBattlePackBase[header->dir_off];
    lo = 0u;
    hi = header->clip_count;
    while (lo < hi)
    {
        u32 mid = lo + ((hi - lo) >> 1);
        u32 got = dir[mid].asset_id;

        if (got == asset_id)
        {
            return (void *)(uintptr_t)&sNdsBattlePackBase[dir[mid]
                                                              .slot_table_off];
        }
        if (got < asset_id)
        {
            lo = mid + 1u;
        }
        else
        {
            hi = mid;
        }
    }
    return NULL;
}

s32 ndsBattlePackAssetIdForSlotTable(const void *slot_table)
{
    const NDSBattlePackHeader *header = ndsBattlePackHeader();
    const NDSBattlePackClip *dir;
    u32 want;
    u32 i;

    if ((header == NULL) || (slot_table == NULL))
    {
        return -1;
    }
    if (((uintptr_t)slot_table < (uintptr_t)(const void *)sNdsBattlePackBase) ||
        ((uintptr_t)slot_table >=
         ((uintptr_t)(const void *)sNdsBattlePackBase + header->blob_bytes)))
    {
        return -1;
    }
    want = (u32)((uintptr_t)slot_table -
                 (uintptr_t)(const void *)sNdsBattlePackBase);
    dir = (const NDSBattlePackClip *)(const void *)
        &sNdsBattlePackBase[header->dir_off];
    for (i = 0u; i < header->clip_count; i++)
    {
        if (dir[i].slot_table_off == want)
        {
            return (s32)dir[i].asset_id;
        }
    }
    return -1;
}

s32 ndsBattlePackContains(const void *ptr, size_t size, const void **out_base,
                          size_t *out_size)
{
    const NDSBattlePackHeader *header;
    uintptr_t base;
    uintptr_t addr;

    if (ptr == NULL)
    {
        return FALSE;
    }
    header = ndsBattlePackHeader();
    if (header == NULL)
    {
        return FALSE;
    }
    base = (uintptr_t)(const void *)sNdsBattlePackBase;
    addr = (uintptr_t)ptr;
    if ((addr < base) || (addr > (base + header->blob_bytes)) ||
        (size > (size_t)(header->blob_bytes - (u32)(addr - base))))
    {
        return FALSE;
    }
    if (out_base != NULL)
    {
        *out_base = (const void *)sNdsBattlePackBase;
    }
    if (out_size != NULL)
    {
        *out_size = (size_t)header->blob_bytes;
    }
    return TRUE;
}

#else /* !NDS_R2_BATTLEPACK */

void *ndsBattlePackFindFigatree(u32 asset_id)
{
    (void)asset_id;
    return NULL;
}

s32 ndsBattlePackAssetIdForSlotTable(const void *slot_table)
{
    (void)slot_table;
    return -1;
}

s32 ndsBattlePackAdopt(void *base, u32 bytes)
{
    (void)base;
    (void)bytes;
    return FALSE;
}

void ndsBattlePackDrop(void)
{
}

s32 ndsBattlePackContains(const void *ptr, size_t size, const void **out_base,
                          size_t *out_size)
{
    (void)ptr;
    (void)size;
    (void)out_base;
    (void)out_size;
    return FALSE;
}

#endif /* NDS_R2_BATTLEPACK */
