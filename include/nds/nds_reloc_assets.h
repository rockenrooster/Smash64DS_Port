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

void ndsRelocAssetsInit(void);
const char *ndsRelocAssetGetPath(u32 file_id);
s32 ndsRelocAssetReadHeader(u32 file_id, NDSRelocAssetHeader *out_header);
s32 ndsRelocAssetLoadData(u32 file_id, void *dst, size_t dst_capacity,
                           NDSRelocAssetHeader *out_header);

extern volatile u32 gNdsRelocAssetInitResult;
extern volatile u32 gNdsRelocAssetHeaderReadCount;
extern volatile u32 gNdsRelocAssetPayloadReadCount;
extern volatile u32 gNdsRelocAssetOpenFailCount;
extern volatile u32 gNdsRelocAssetFormatFailCount;
extern volatile u32 gNdsRelocAssetShortReadCount;

#endif
