/* Host-only sys/taskman.h shadow: only what nds_native_stage_blob.c needs
 * (syTaskmanMalloc + the heap-generation key). The real header drags in
 * PR/gbi.h and the obj system, which have no host build. */
#ifndef SSB64_NDS_SYS_TASKMAN_HOST_H
#define SSB64_NDS_SYS_TASKMAN_HOST_H

#include <stddef.h>

#include <PR/ultratypes.h>

extern void *syTaskmanMalloc(size_t size, u32 align);

#endif
