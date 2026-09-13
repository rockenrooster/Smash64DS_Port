/* Host-only nds/nds_startup.h shadow: the loader only reads the
 * heap-generation key that marks arena residency. */
#ifndef SSB64_NDS_STARTUP_HOST_H
#define SSB64_NDS_STARTUP_HOST_H

#include <PR/ultratypes.h>

extern volatile u32 gNdsTaskmanHeapGeneration;

#endif
