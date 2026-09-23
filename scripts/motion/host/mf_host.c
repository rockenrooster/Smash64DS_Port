/* Host harness for scripts/motion/check_mf_pack.py: the runtime MF1 decoder,
 * compiled from the ROM's own source file, exported for ctypes. Nothing here
 * decodes on its own -- every byte comes from src/nds/nds_motion_mf.c. */

#define NDS_MF_HOST 1
#include "../../../src/nds/nds_motion_mf.c"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define MFH_EXPORT __declspec(dllexport)
#else
#define MFH_EXPORT
#endif

static NdsMfTables sMfhTables;
static void *sMfhStorage;
static const u8 *sMfhPack;
static u32 sMfhPackBytes;

/* Parse the header, expand the tables. Returns 0 or a negative error. */
MFH_EXPORT int mfh_init(const u8 *pack, u32 pack_bytes, u32 *storage_bytes)
{
    const NdsMfPackHeader *h = (const NdsMfPackHeader *)pack;
    u32 need;

    if ((pack_bytes < sizeof(NdsMfPackHeader)) || (h->magic != NDS_MF_PACK_MAGIC) ||
        (h->version != NDS_MF_PACK_VERSION) ||
        ((h->tables_off + h->tables_bytes) > pack_bytes))
    {
        return -100;
    }
    need = ndsMfTablesStorageBytes(pack + h->tables_off, h->tables_bytes);
    if (need == 0u)
    {
        return -101;
    }
    free(sMfhStorage);
    sMfhStorage = malloc(need);
    if (sMfhStorage == NULL)
    {
        return -102;
    }
    if (storage_bytes != NULL)
    {
        *storage_bytes = need;
    }
    sMfhPack = pack;
    sMfhPackBytes = pack_bytes;
    return (int)ndsMfExpandTables(pack + h->tables_off, h->tables_bytes, sMfhStorage,
                                  need, &sMfhTables);
}

/* Decode directory entry `index` into out[0..cap). Returns bytes or < 0. */
MFH_EXPORT int mfh_decode(u32 index, u8 *out, u32 cap, u32 *bits_used)
{
    const NdsMfPackHeader *h = (const NdsMfPackHeader *)sMfhPack;
    const NdsMfPackEntry *e;
    u32 start, end;

    if ((sMfhPack == NULL) || (index >= h->dir_count))
    {
        return -110;
    }
    e = (const NdsMfPackEntry *)(sMfhPack + h->dir_off) + index;
    start = h->data_off + e->data_rel;
    end = start + ((e->stream_bits + 7u) >> 3);
    if (end > sMfhPackBytes)
    {
        return -111;
    }
    return (int)ndsMfDecodeClip(&sMfhTables, sMfhPack + start, end - start, out, cap,
                                bits_used);
}

/* Every LUT entry must name the symbol the canonical slow path decodes from
 * the same bits. Returns the number of disagreeing entries. */
MFH_EXPORT int mfh_lut_selfcheck(void)
{
    u32 c, x, bad = 0;

    for (c = 0; c < NDS_MF_CLASS_COUNT; c++)
    {
        for (x = 0; x < NDS_MF_CTX_COUNT; x++)
        {
            const NdsMfTable *t = sMfhTables.table[c][x];
            u32 pfx;

            if (t == NULL)
            {
                continue;
            }
            for (pfx = 0; pfx < (1u << NDS_MF_LUT_BITS); pfx++)
            {
                u32 e = t->lut[pfx];
                u32 L, found = 0, want = 0;

                for (L = 1; L <= NDS_MF_LUT_BITS && !found; L++)
                {
                    u32 code = pfx >> (NDS_MF_LUT_BITS - L);
                    u32 off = code - t->first_code[L];

                    if (off < t->count[L])
                    {
                        found = L;
                        want = ((t->first_index[L] + off) << 4) | L;
                    }
                }
                if ((found ? want : 0u) != e)
                {
                    bad++;
                }
            }
        }
    }
    return (int)bad;
}
