/* Blob-resident stage packets. Contract: include/nds/nds_native_stage_blob.h.
 *
 * Every linked stage packet is static const, so its slab sits in main RAM
 * for the life of the ROM (~500 KB across 41 stages). A blob stage links
 * only its registry row: this loader reads its relocatable blob
 * (nitro:/stages/native_stage_<stage>.bin, written by
 * scripts/stages/generate_nds_native_stage.py --emit-blob) into the scene
 * arena at stage start and fixes up one static mutable NDSNativeStagePacket
 * mirror from base plus blob offsets. The MULTI redirect in
 * nds_native_stage_select.inc already reads through the active packet, so
 * no consumer changes.
 *
 * The slab comes from syTaskmanMalloc, which rewinds on scene exit (see the
 * comment in src/port/reloc_backend_assets.c): residency is keyed on
 * gNdsTaskmanHeapGeneration for the reason the reloc scene-file buffer is,
 * and a rewound arena reads back as NULL so the unresolved-kind decline
 * path fires instead of drawing a recycled slab. mpCollisionInitGroundData
 * loads once per stage entry, so a same-scene double load cannot strand
 * more than one body in the arena.
 *
 * Reads go through the stream primitives (one NitroFS open for the whole
 * blob) the way nds_ui_kit.c loads its pack. */

#include "nds_build_config.h"

#include <sys/taskman.h>

#include <nds/nds_startup.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_native_stage_blob.h>

#define NDS_NATIVE_STAGE_BLOB_GKIND_COUNT 41u
#define NDS_NATIVE_STAGE_BLOB_BODY_CAP_BYTES (32u * 1024u)

__attribute__((used)) volatile u32 gNdsNativeStageBlobLoadCount;
__attribute__((used)) volatile u32 gNdsNativeStageBlobBytesLoaded;
__attribute__((used)) volatile u32 gNdsNativeStageBlobHashMismatchCount;
__attribute__((used)) volatile u32 gNdsNativeStageBlobReadFailCount;
__attribute__((used)) volatile u32 gNdsNativeStageBlobActiveGKind =
    NDS_NATIVE_STAGE_BLOB_GKIND_INVALID;

/* NitroFS path per gkind, decomp gr/grdef.h order. NULL means "no blob":
 * Dream Land stays linked, and PupupuNew/Explain have no packet yet. */
static const char *const sNdsNativeStageBlobPaths[NDS_NATIVE_STAGE_BLOB_GKIND_COUNT] =
{
    "nitro:/stages/native_stage_castle.bin", /* 0 nGRKindCastle */
    "nitro:/stages/native_stage_sector.bin", /* 1 nGRKindSector */
    "nitro:/stages/native_stage_jungle.bin", /* 2 nGRKindJungle */
    "nitro:/stages/native_stage_zebes.bin", /* 3 nGRKindZebes */
    "nitro:/stages/native_stage_hyrule.bin", /* 4 nGRKindHyrule */
    "nitro:/stages/native_stage_yoster.bin", /* 5 nGRKindYoster */
    NULL, /* 6 nGRKindPupupu: linked, never loaded */
    "nitro:/stages/native_stage_yamabuki.bin", /* 7 nGRKindYamabuki */
    "nitro:/stages/native_stage_inishie.bin", /* 8 nGRKindInishie */
    "nitro:/stages/native_stage_pupupusmall.bin", /* 9 nGRKindPupupuSmall */
    NULL, /* 10 nGRKindPupupuNew */
    NULL, /* 11 nGRKindExplain */
    "nitro:/stages/native_stage_yostersmall.bin", /* 12 nGRKindYosterSmall */
    "nitro:/stages/native_stage_metal.bin", /* 13 nGRKindMetal */
    "nitro:/stages/native_stage_zako.bin", /* 14 nGRKindZako */
    "nitro:/stages/native_stage_bonus3.bin", /* 15 nGRKindBonus3 */
    "nitro:/stages/native_stage_last.bin", /* 16 nGRKindLast */
    "nitro:/stages/native_stage_bonus1_mario.bin", /* 17 */
    "nitro:/stages/native_stage_bonus1_fox.bin", /* 18 */
    "nitro:/stages/native_stage_bonus1_donkey.bin", /* 19 */
    "nitro:/stages/native_stage_bonus1_samus.bin", /* 20 */
    "nitro:/stages/native_stage_bonus1_luigi.bin", /* 21 */
    "nitro:/stages/native_stage_bonus1_link.bin", /* 22 */
    "nitro:/stages/native_stage_bonus1_yoshi.bin", /* 23 */
    "nitro:/stages/native_stage_bonus1_captain.bin", /* 24 */
    "nitro:/stages/native_stage_bonus1_kirby.bin", /* 25 */
    "nitro:/stages/native_stage_bonus1_pikachu.bin", /* 26 */
    "nitro:/stages/native_stage_bonus1_purin.bin", /* 27 */
    "nitro:/stages/native_stage_bonus1_ness.bin", /* 28 */
    "nitro:/stages/native_stage_bonus2_mario.bin", /* 29 */
    "nitro:/stages/native_stage_bonus2_fox.bin", /* 30 */
    "nitro:/stages/native_stage_bonus2_donkey.bin", /* 31 */
    "nitro:/stages/native_stage_bonus2_samus.bin", /* 32 */
    "nitro:/stages/native_stage_bonus2_luigi.bin", /* 33 */
    "nitro:/stages/native_stage_bonus2_link.bin", /* 34 */
    "nitro:/stages/native_stage_bonus2_yoshi.bin", /* 35 */
    "nitro:/stages/native_stage_bonus2_captain.bin", /* 36 */
    "nitro:/stages/native_stage_bonus2_kirby.bin", /* 37 */
    "nitro:/stages/native_stage_bonus2_pikachu.bin", /* 38 */
    "nitro:/stages/native_stage_bonus2_purin.bin", /* 39 */
    "nitro:/stages/native_stage_bonus2_ness.bin", /* 40 */
};

static NDSNativeStageBlobPacket sNdsNativeStageBlobPacket;
static u32 sNdsNativeStageBlobLoaded = 0u;
static u32 sNdsNativeStageBlobGeneration = 0u;

static u16 ndsNativeStageBlobReadU16(const u8 *cursor)
{
    return (u16)((u16)cursor[0] | ((u16)cursor[1] << 8));
}

static u32 ndsNativeStageBlobReadU32(const u8 *cursor)
{
    return (u32)cursor[0] | ((u32)cursor[1] << 8) |
        ((u32)cursor[2] << 16) | ((u32)cursor[3] << 24);
}

static u64 ndsNativeStageBlobReadU64(const u8 *cursor)
{
    u64 low = ndsNativeStageBlobReadU32(cursor);
    u64 high = ndsNativeStageBlobReadU32(cursor + 4);

    return low | (high << 32);
}

static u32 ndsNativeStageBlobFnv1a(const u8 *data, u32 bytes)
{
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < bytes; i++)
    {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

const NDSNativeStageBlobPacket *ndsNativeStageBlobPacket(void)
{
    if ((sNdsNativeStageBlobLoaded == 0u) ||
        (sNdsNativeStageBlobGeneration != gNdsTaskmanHeapGeneration))
    {
        return NULL;
    }
    return &sNdsNativeStageBlobPacket;
}

s32 ndsNativeStageBlobLoad(u32 gkind)
{
    /* Table byte sizes, exactly the C structs' (see the generator's
     * _pack_blob_table): index matches the header offset order. */
    static const u8 table_sizes[NDS_NATIVE_STAGE_BLOB_TABLE_COUNT] =
        { 16u, 12u, 12u, 24u, 8u, 16u, 2u, 8u,
          12u, 28u, 12u, 1u, 4u, 64u, 2u, 1u };
    /* Count index per table into the header's 25 u16 counts, or 0xFF when
     * the table length is not count-sized (baked/binding tables). */
    static const u8 table_counts[NDS_NATIVE_STAGE_BLOB_TABLE_COUNT] =
        { 0u, 1u, 2u, 3u, 4u, 11u, 12u, 5u,
          6u, 7u, 8u, 9u, 10u, 18u, 0xFFu, 0xFFu };
    NdsRelocAssetStream stream;
    u8 header[NDS_NATIVE_STAGE_BLOB_HEADER_BYTES];
    const char *path;
    u8 *slab;
    u32 slab_bytes;
    u32 body_bytes;
    u32 fnv;
    u32 counts[25];
    u32 offsets[NDS_NATIVE_STAGE_BLOB_TABLE_COUNT];
    u32 dl_mask;
    u32 i;
    u32 expect_slab;

    gNdsNativeStageBlobLoadCount++;
    gNdsNativeStageBlobActiveGKind = gkind;
    sNdsNativeStageBlobLoaded = 0u;
    if (gkind == NDS_NATIVE_STAGE_BLOB_GKIND_DREAMLAND)
    {
        /* Linked, never loaded. */
        return TRUE;
    }
    if (gkind >= NDS_NATIVE_STAGE_BLOB_GKIND_COUNT)
    {
        gNdsNativeStageBlobReadFailCount++;
        return FALSE;
    }
    path = sNdsNativeStageBlobPaths[gkind];
    if (path == NULL)
    {
        gNdsNativeStageBlobReadFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamOpen(&stream, path) == FALSE)
    {
        gNdsNativeStageBlobReadFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamRead(&stream, 0u, header,
                                NDS_NATIVE_STAGE_BLOB_HEADER_BYTES) == FALSE)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeStageBlobReadFailCount++;
        return FALSE;
    }
    if ((ndsNativeStageBlobReadU32(header) != NDS_NATIVE_STAGE_BLOB_MAGIC) ||
        (ndsNativeStageBlobReadU16(header + 4) != NDS_NATIVE_STAGE_BLOB_ABI) ||
        (ndsNativeStageBlobReadU16(header + 6) !=
         NDS_NATIVE_STAGE_BLOB_HEADER_BYTES))
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeStageBlobHashMismatchCount++;
        return FALSE;
    }
    slab_bytes = ndsNativeStageBlobReadU32(header + 8);
    body_bytes = ndsNativeStageBlobReadU32(header + 12);
    fnv = ndsNativeStageBlobReadU32(header + 16);
    for (i = 0u; i < 25u; i++)
    {
        counts[i] = ndsNativeStageBlobReadU16(header + 20u + i * 2u);
    }
    dl_mask = ndsNativeStageBlobReadU32(header + 90u);
    for (i = 0u; i < NDS_NATIVE_STAGE_BLOB_TABLE_COUNT; i++)
    {
        offsets[i] = ndsNativeStageBlobReadU32(header + 94u + i * 4u);
    }
    if ((slab_bytes == 0u) ||
        (slab_bytes > NDS_NATIVE_STAGE_BLOB_MAX_SLAB_BYTES) ||
        (body_bytes == 0u) ||
        (body_bytes > NDS_NATIVE_STAGE_BLOB_BODY_CAP_BYTES))
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeStageBlobHashMismatchCount++;
        return FALSE;
    }
    /* The slab byte count is recomputed from the table counts, the same sum
     * the generator's slab_bytes() publishes: a truncated or padded body
     * fails here before any pointer is fixed up. */
    expect_slab = counts[0] * 16u + counts[1] * 12u + counts[2] * 12u +
        counts[3] * 24u + counts[4] * 8u + counts[11] * 16u +
        counts[12] * 2u + counts[5] * 8u + counts[6] * 12u +
        counts[7] * 28u + counts[8] * 12u + counts[9] + counts[10] * 4u + 4u;
    if (offsets[14] != NDS_NATIVE_STAGE_BLOB_ABSENT)
    {
        expect_slab += counts[3] * 2u;
    }
    if (offsets[15] != NDS_NATIVE_STAGE_BLOB_ABSENT)
    {
        expect_slab += counts[3];
    }
    if ((expect_slab != slab_bytes) || (dl_mask == 0u) !=
        ((offsets[14] == NDS_NATIVE_STAGE_BLOB_ABSENT) &&
         (offsets[15] == NDS_NATIVE_STAGE_BLOB_ABSENT)))
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeStageBlobHashMismatchCount++;
        return FALSE;
    }
    slab = syTaskmanMalloc((size_t)body_bytes, 0x10u);
    if (slab == NULL)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeStageBlobReadFailCount++;
        return FALSE;
    }
    if (ndsRelocAssetStreamRead(&stream, NDS_NATIVE_STAGE_BLOB_HEADER_BYTES,
                                slab, body_bytes) == FALSE)
    {
        ndsRelocAssetStreamClose(&stream);
        gNdsNativeStageBlobReadFailCount++;
        return FALSE;
    }
    ndsRelocAssetStreamClose(&stream);
    if (ndsNativeStageBlobFnv1a(slab, body_bytes) != fnv)
    {
        gNdsNativeStageBlobHashMismatchCount++;
        return FALSE;
    }
    for (i = 0u; i < NDS_NATIVE_STAGE_BLOB_TABLE_COUNT; i++)
    {
        u32 want;

        if (i == 14u)
        {
            want = (offsets[i] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
                counts[3] * 2u : 0u;
        }
        else if (i == 15u)
        {
            want = (offsets[i] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
                counts[3] : 0u;
        }
        else
        {
            want = counts[table_counts[i]] * table_sizes[i];
        }
        if (offsets[i] == NDS_NATIVE_STAGE_BLOB_ABSENT)
        {
            if (want != 0u)
            {
                gNdsNativeStageBlobHashMismatchCount++;
                return FALSE;
            }
        }
        else if ((offsets[i] & 3u) || (offsets[i] + want > body_bytes))
        {
            gNdsNativeStageBlobHashMismatchCount++;
            return FALSE;
        }
    }
    sNdsNativeStageBlobPacket.assets =
        (offsets[0] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[0]) : NULL;
    sNdsNativeStageBlobPacket.segments =
        (offsets[1] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[1]) : NULL;
    sNdsNativeStageBlobPacket.dobjs =
        (offsets[2] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[2]) : NULL;
    sNdsNativeStageBlobPacket.bindings =
        (offsets[3] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[3]) : NULL;
    sNdsNativeStageBlobPacket.runs =
        (offsets[4] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[4]) : NULL;
    sNdsNativeStageBlobPacket.vertices =
        (offsets[5] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[5]) : NULL;
    sNdsNativeStageBlobPacket.corners =
        (offsets[6] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[6]) : NULL;
    sNdsNativeStageBlobPacket.epochs =
        (offsets[7] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[7]) : NULL;
    sNdsNativeStageBlobPacket.materials =
        (offsets[8] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[8]) : NULL;
    sNdsNativeStageBlobPacket.policies =
        (offsets[9] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[9]) : NULL;
    sNdsNativeStageBlobPacket.state_deltas =
        (offsets[10] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[10]) : NULL;
    sNdsNativeStageBlobPacket.state_sequence =
        (offsets[11] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[11]) : NULL;
    sNdsNativeStageBlobPacket.state_spans =
        (offsets[12] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[12]) : NULL;
#if NDS_TASK51_STAGE_NATIVE
    sNdsNativeStageBlobPacket.baked_world =
        (offsets[13] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const void *)(slab + offsets[13]) : NULL;
#endif
    sNdsNativeStageBlobPacket.asset_count = (u16)counts[0];
    sNdsNativeStageBlobPacket.segment_count = (u16)counts[1];
    sNdsNativeStageBlobPacket.dobj_count = (u16)counts[2];
    sNdsNativeStageBlobPacket.binding_count = (u16)counts[3];
    sNdsNativeStageBlobPacket.run_count = (u16)counts[4];
    sNdsNativeStageBlobPacket.epoch_count = (u16)counts[5];
    sNdsNativeStageBlobPacket.material_event_count = (u16)counts[6];
    sNdsNativeStageBlobPacket.policy_count = (u16)counts[7];
    sNdsNativeStageBlobPacket.state_delta_count = (u16)counts[8];
    sNdsNativeStageBlobPacket.state_sequence_count = (u16)counts[9];
    sNdsNativeStageBlobPacket.state_span_count = (u16)counts[10];
    sNdsNativeStageBlobPacket.dense_vertex_count = (u16)counts[11];
    sNdsNativeStageBlobPacket.corner_count = (u16)counts[12];
    sNdsNativeStageBlobPacket.triangle_count = (u16)counts[13];
    sNdsNativeStageBlobPacket.source_command_count = (u16)counts[14];
    sNdsNativeStageBlobPacket.source_vertex_count = (u16)counts[15];
    sNdsNativeStageBlobPacket.vertex_command_count = (u16)counts[16];
    sNdsNativeStageBlobPacket.triangle_command_count = (u16)counts[17];
    sNdsNativeStageBlobPacket.baked_world_count = (u16)counts[18];
    sNdsNativeStageBlobPacket.raw_triangles = (u16)counts[19];
    sNdsNativeStageBlobPacket.projected_no_z_triangles = (u16)counts[20];
    sNdsNativeStageBlobPacket.projected_range_triangles = (u16)counts[21];
    sNdsNativeStageBlobPacket.cross_run_count = (u16)counts[22];
    sNdsNativeStageBlobPacket.cross_triangle_count = (u16)counts[23];
    sNdsNativeStageBlobPacket.cross_corner_count = (u16)counts[24];
    sNdsNativeStageBlobPacket.rigid_binding_mask =
        ndsNativeStageBlobReadU64(header + 70u);
    sNdsNativeStageBlobPacket.camera_binding_mask =
        ndsNativeStageBlobReadU64(header + 78u);
    sNdsNativeStageBlobPacket.has_generated_segment0 = header[86];
    sNdsNativeStageBlobPacket.gkind = (u8)gkind;
    sNdsNativeStageBlobPacket.reserved[0] = 0u;
    sNdsNativeStageBlobPacket.reserved[1] = 0u;
    sNdsNativeStageBlobPacket.binding_dobjs =
        (offsets[14] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const u16 *)(const void *)(slab + offsets[14]) : NULL;
    sNdsNativeStageBlobPacket.binding_heads =
        (offsets[15] != NDS_NATIVE_STAGE_BLOB_ABSENT) ?
        (const u8 *)(const void *)(slab + offsets[15]) : NULL;
    if ((header[87] != NDS_NATIVE_STAGE_BLOB_GKIND_INVALID) &&
        (header[87] != (u8)gkind))
    {
        gNdsNativeStageBlobHashMismatchCount++;
        return FALSE;
    }
    gNdsNativeStageBlobBytesLoaded =
        NDS_NATIVE_STAGE_BLOB_HEADER_BYTES + body_bytes;
    sNdsNativeStageBlobGeneration = gNdsTaskmanHeapGeneration;
    sNdsNativeStageBlobLoaded = 1u;
    return TRUE;
}
