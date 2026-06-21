#ifndef SSB64_NDS_SYS_MALLOC_H
#define SSB64_NDS_SYS_MALLOC_H

#include <stddef.h>
#include <PR/ultratypes.h>

typedef struct SYMallocRegion {
    u32 id;
    void *start;
    void *end;
    void *ptr;
} SYMallocRegion;

void syMallocInit(SYMallocRegion *region, u32 id, void *start, size_t size);
void *syMallocSet(SYMallocRegion *region, size_t size, u32 alignment);
void syMallocReset(SYMallocRegion *region);

#endif

