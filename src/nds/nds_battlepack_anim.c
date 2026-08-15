#include <nds/nds_battlepack_anim.h>

/* `used` because a dropped diagnostic global has turned Boundary red here
 * before: `--gc-sections` removed five of them when their writers were compiled
 * out, and the verifier reported "Missing ELF symbol" rather than a flag. */
__attribute__((used)) volatile u32 gNdsBattlePackClips;
__attribute__((used)) volatile u32 gNdsBattlePackBytes;
__attribute__((used)) volatile u32 gNdsBattlePackHits;
__attribute__((used)) volatile u32 gNdsBattlePackMisses;
__attribute__((used)) volatile u32 gNdsBattlePackState;

#if NDS_R2_BATTLEPACK

/* The blob is linked, not loaded. `.incbin` costs one ARM9 image region and
 * zero setup work: no NitroFS walk, no cartridge read, no decompression, no
 * relocation -- which is the point of slice 1. It is `.rodata` rather than an
 * arena allocation because the two pools are separate and only the static one
 * has room for a whole fighter today; see BATTLEPACK_POOL.md.
 *
 * Alignment is 16 so the directory, the slot tables and every script start are
 * 4-byte aligned no matter where the linker places the section. The resolver
 * REFUSES a misaligned result (reloc_backend_assets.c:2889, the 2026-08-02
 * shield-freeze seam), so this is load-bearing, not tidiness. */
__asm__(".section .rodata\n"
        ".balign 16\n"
        ".global gNdsBattlePackBlob\n"
        ".hidden gNdsBattlePackBlob\n"
        "gNdsBattlePackBlob:\n"
        ".incbin \"battlepack_fox.bin\"\n"
        ".balign 4\n"
        ".global gNdsBattlePackBlobEnd\n"
        ".hidden gNdsBattlePackBlobEnd\n"
        "gNdsBattlePackBlobEnd:\n"
        ".previous\n");

extern const u8 gNdsBattlePackBlob[];
extern const u8 gNdsBattlePackBlobEnd[];

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

/* Validated once, not per lookup, and never re-validated: the blob is `const`
 * program image, so a second read cannot disagree with the first. The check is
 * against a BUILD mismatch (a stale asset, a format bump), which is exactly the
 * class `--gc-sections` and generated-data drift have produced here before. */
static const NDSBattlePackHeader *ndsBattlePackHeader(void)
{
    const NDSBattlePackHeader *header =
        (const NDSBattlePackHeader *)(const void *)gNdsBattlePackBlob;
    u32 linked;

    if (gNdsBattlePackState != NDS_BATTLEPACK_STATE_UNCHECKED)
    {
        return (gNdsBattlePackState == NDS_BATTLEPACK_STATE_READY) ? header
                                                                   : NULL;
    }
    linked = (u32)(gNdsBattlePackBlobEnd - gNdsBattlePackBlob);
    if (header->magic != NDS_BATTLEPACK_MAGIC)
    {
        gNdsBattlePackState = NDS_BATTLEPACK_STATE_BAD_MAGIC;
        return NULL;
    }
    if (header->version != NDS_BATTLEPACK_VERSION)
    {
        gNdsBattlePackState = NDS_BATTLEPACK_STATE_BAD_VERSION;
        return NULL;
    }
    if ((header->blob_bytes != linked) || (header->clip_count == 0u) ||
        (header->stream_off >= linked) || (header->dir_off >= linked))
    {
        gNdsBattlePackState = NDS_BATTLEPACK_STATE_BAD_EXTENT;
        return NULL;
    }
    gNdsBattlePackClips = header->clip_count;
    gNdsBattlePackBytes = linked;
    gNdsBattlePackState = NDS_BATTLEPACK_STATE_READY;
    return header;
}

void *ndsBattlePackFindFigatree(u32 asset_id)
{
    const NDSBattlePackHeader *header = ndsBattlePackHeader();
    const NDSBattlePackClip *dir;
    u32 lo;
    u32 hi;

    if (header == NULL)
    {
        return NULL;
    }
    if ((asset_id < header->first_id) || (asset_id > header->last_id))
    {
        return NULL;
    }
    dir = (const NDSBattlePackClip *)(const void *)
        &gNdsBattlePackBlob[header->dir_off];
    lo = 0u;
    hi = header->clip_count;
    while (lo < hi)
    {
        u32 mid = lo + ((hi - lo) >> 1);
        u32 got = dir[mid].asset_id;

        if (got == asset_id)
        {
            return (void *)(uintptr_t)&gNdsBattlePackBlob[dir[mid]
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
    base = (uintptr_t)(const void *)gNdsBattlePackBlob;
    addr = (uintptr_t)ptr;
    if ((addr < base) || (addr > (base + header->blob_bytes)) ||
        (size > (size_t)(header->blob_bytes - (u32)(addr - base))))
    {
        return FALSE;
    }
    if (out_base != NULL)
    {
        *out_base = (const void *)gNdsBattlePackBlob;
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
