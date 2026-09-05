#ifndef SSB64_NDS_NATIVE_STAGE_BLOB_H
#define SSB64_NDS_NATIVE_STAGE_BLOB_H

/* Relocatable stage packet blobs (P2-4 stage-production packet residency).
 *
 * Every linked stage packet is static const, so its slab sits in main RAM
 * for the life of the ROM (~500 KB across 41 stages). A blob stage instead
 * links only its registry row: scripts/stages/generate_nds_native_stage.py
 * --emit-blob writes one relocatable binary per stage (header + every table
 * laid out exactly as the C structs are, little endian, FNV-1a-32 of the
 * body), staged at nitro:/stages/native_stage_<stage>.bin, and
 * ndsNativeStageBlobLoad reads it into the scene arena at stage start.
 * Dream Land stays linked and byte-identical and never takes this path.
 *
 * Contract: include/nds/nds_native_stage_blob.h.
 */

#include <PR/ultratypes.h>

#define NDS_NATIVE_STAGE_BLOB_MAGIC 0x3142534Eu /* 'NSB1' little endian */
/* ABI 2 (2026-09-05): NDSNativeStageBinding rows are 28 bytes, with u16
 * source_vertex_count and triangle_count. */
#define NDS_NATIVE_STAGE_BLOB_ABI 2u
#define NDS_NATIVE_STAGE_BLOB_HEADER_BYTES 160u
#define NDS_NATIVE_STAGE_BLOB_TABLE_COUNT 16u
#define NDS_NATIVE_STAGE_BLOB_ABSENT 0xFFFFFFFFu
/* Biggest static slab the generator permits; the loader declines anything
 * larger rather than overrunning the scene arena. */
#define NDS_NATIVE_STAGE_BLOB_MAX_SLAB_BYTES (24u * 1024u)

#define NDS_NATIVE_STAGE_BLOB_GKIND_DREAMLAND 6u
#define NDS_NATIVE_STAGE_BLOB_GKIND_INVALID 0xFFu

/* Layout mirror of NDSNativeStagePacket (src/nds/nds_native_stage_select.inc)
 * with opaque table pointers: same pointer order, same scalar order, same
 * sizes. The loader fills one from base plus blob offsets; the resolver
 * casts it to the real packet type. src/nds/nds_native_stage_select.inc
 * proves the two stay field-identical with _Static_asserts, so this header
 * must change in lockstep with that struct. */
typedef struct NDSNativeStageBlobPacket
{
    const void *assets;
    const void *segments;
    const void *dobjs;
    const void *bindings;
    const void *runs;
    const void *vertices;
    const void *corners;
    const void *epochs;
    const void *materials;
    const void *policies;
    const void *state_deltas;
    const void *state_sequence;
    const void *state_spans;
#if NDS_TASK51_STAGE_NATIVE
    const void *baked_world;
#endif
    u16 asset_count;
    u16 segment_count;
    u16 dobj_count;
    u16 binding_count;
    u16 run_count;
    u16 epoch_count;
    u16 material_event_count;
    u16 policy_count;
    u16 state_delta_count;
    u16 state_sequence_count;
    u16 state_span_count;
    u16 dense_vertex_count;
    u16 corner_count;
    u16 triangle_count;
    u16 source_command_count;
    u16 source_vertex_count;
    u16 vertex_command_count;
    u16 triangle_command_count;
    u16 baked_world_count;
    u16 raw_triangles;
    u16 projected_no_z_triangles;
    u16 projected_range_triangles;
    u16 cross_run_count;
    u16 cross_triangle_count;
    u16 cross_corner_count;
    u64 rigid_binding_mask;
    u64 camera_binding_mask;
    u8 has_generated_segment0;
    u8 gkind;
    u8 reserved[2];
    const u16 *binding_dobjs;
    const u8 *binding_heads;
} NDSNativeStageBlobPacket;

/* Load gkind's blob into the scene arena and make it the active packet.
 * Dream Land (linked) is a no-op TRUE. A mismatch or read failure leaves
 * the active packet NULL so the existing unresolved-kind decline path
 * fires. TRUE while a blob packet is resident for gkind. */
s32 ndsNativeStageBlobLoad(u32 gkind);
/* The resident blob packet, or NULL when none is loaded for the current
 * scene (never loaded, load failed, or the arena rewound past it). */
const NDSNativeStageBlobPacket *ndsNativeStageBlobPacket(void);

/* gc-sections drops unreferenced diagnostic globals, hence used. Every one
 * is referenced from the load path. */
extern volatile u32 gNdsNativeStageBlobLoadCount;
extern volatile u32 gNdsNativeStageBlobBytesLoaded;
extern volatile u32 gNdsNativeStageBlobHashMismatchCount;
extern volatile u32 gNdsNativeStageBlobReadFailCount;
extern volatile u32 gNdsNativeStageBlobActiveGKind;

#endif
