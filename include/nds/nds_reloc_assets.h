#ifndef SSB64_NDS_RELOC_ASSETS_H
#define SSB64_NDS_RELOC_ASSETS_H

#include <stddef.h>

#include <PR/ultratypes.h>

#define NDS_RELOC_ASSET_INIT_PASS 0x4e465349u /* NFSI */

typedef struct NDSRelocAssetHeader {
    u32 file_id;
    u16 reloc_intern_offset;
    u16 reloc_extern_offset;
    u32 extern_file_ids_num;
    u32 data_size;
} NDSRelocAssetHeader;

struct MObjSub;

void ndsRelocAssetsInit(void);
/* R2-04 E4/E5. Makes the match's animation streams resident. PreloadMatch arms
 * the walk at the battle-start seam; PreloadStep loads one asset per scene
 * update until the list is exhausted. The work is stepped rather than done in
 * one burst because the seam's budget is one BGM packet (~186 ms), not the
 * whole load. Declared unconditionally so this header does not depend on the
 * generated config defines being visible before it; defined only under
 * NDS_R2_ANIM_CACHE, and both call sites are guarded by the same flag. */
void ndsR2AnimCachePreloadMatch(void);
void ndsR2AnimCachePreloadStep(void);
const char *ndsRelocAssetGetPath(u32 asset_id);
s32 ndsRelocAssetReadHeader(u32 asset_id, NDSRelocAssetHeader *out_header);
s32 ndsRelocAssetReadExternFileIDs(u32 asset_id, u32 *out_file_ids,
                                   u32 capacity, u32 *out_count);
s32 ndsRelocAssetLoadData(u32 asset_id, void *dst, size_t dst_capacity,
                           NDSRelocAssetHeader *out_header);
/* Both from one open. Prefer this wherever a caller wants the header and the
 * payload: the two-call form walks the NitroFS directory twice. */
s32 ndsRelocAssetLoadHeaderAndData(u32 asset_id, void *dst,
                                   size_t dst_capacity,
                                   NDSRelocAssetHeader *out_header);
/* A byte range out of a NitroFS payload that is NOT an o2r asset -- no asset
 * table entry, no o2r header, no file id. Slice 1's resident figatree pack is
 * the only such payload the animation path reads, and it is streamed in chunks
 * because the seam's budget is one BGM packet.
 *
 * File I/O stays in this module rather than being re-implemented in
 * reloc_backend_assets.c: that TU has no <stdio.h> and the open/short-read
 * counters that the after-GO zero assertions read live here. */
s32 ndsRelocAssetReadRawRange(const char *path, u32 offset, void *dst,
                              u32 bytes);
/* Size, zero and load from one open. Prefer this over sizing with
 * ndsRelocAssetAllocSize and then loading: that pair walks the NitroFS
 * directory twice for a size the load's own header already carries. `align` is
 * the caller's allocation granularity; *out_alloc_size receives data_size
 * rounded up to it, and dst is zeroed over exactly that region. */
s32 ndsRelocAssetLoadIntoZeroedHeap(u32 asset_id, void *dst, u32 align,
                                    size_t *out_alloc_size,
                                    NDSRelocAssetHeader *out_header);
s32 ndsRelocGetLoadedAssetView(u32 asset_id, const void **out_data,
                               u32 *out_size);
s32 ndsRelocCopyMObjSubForAttachment(struct MObjSub *dst,
                                     const struct MObjSub *src);

extern volatile u32 gNdsRelocAssetInitResult;
extern volatile u32 gNdsRelocAssetHeaderReadCount;
extern volatile u32 gNdsRelocAssetPayloadReadCount;
extern volatile u32 gNdsRelocAssetOpenFailCount;
extern volatile u32 gNdsRelocAssetFormatFailCount;
extern volatile u32 gNdsRelocAssetShortReadCount;

#endif
