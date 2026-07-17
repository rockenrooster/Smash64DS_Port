#ifndef SSB64_NDS_TASK9_STATE_HASH_H
#define SSB64_NDS_TASK9_STATE_HASH_H

#include <PR/ultratypes.h>

#ifndef NDS_TASK9_STATE_HASH
#define NDS_TASK9_STATE_HASH 0
#endif

#define NDS_TASK9_STATE_HASH_MAX_UPDATES 4096u

typedef struct NDSTask9StateHashRecord
{
    u32 hash1;
    u32 hash2;
    u32 bytes;
    u32 records;
    u32 overflow;
} NDSTask9StateHashRecord;

#if NDS_TASK9_STATE_HASH
extern volatile u32 gNdsTask9StateHashArmed;
extern volatile u32 gNdsTask9StateHashCount;
extern volatile u32 gNdsTask9StateHashOverflow;
extern volatile NDSTask9StateHashRecord
    gNdsTask9StateHashes[NDS_TASK9_STATE_HASH_MAX_UPDATES];

void ndsTask9StateHashRecordUpdate(void);
#endif

#endif
