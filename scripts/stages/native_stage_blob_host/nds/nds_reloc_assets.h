/* Host-only nds/nds_reloc_assets.h shadow: the one-open stream contract
 * (include/nds/nds_reloc_assets.h) backed by host files in harness.c. */
#ifndef SSB64_NDS_RELOC_ASSETS_HOST_H
#define SSB64_NDS_RELOC_ASSETS_HOST_H

#include <PR/ultratypes.h>

typedef struct NdsRelocAssetStream {
    void *file; /* FILE*, kept void so callers need no <stdio.h> */
} NdsRelocAssetStream;

s32 ndsRelocAssetStreamOpen(NdsRelocAssetStream *stream, const char *path);
s32 ndsRelocAssetStreamRead(NdsRelocAssetStream *stream, u32 offset, void *dst,
                            u32 bytes);
void ndsRelocAssetStreamClose(NdsRelocAssetStream *stream);

#endif
