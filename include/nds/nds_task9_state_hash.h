#ifndef SSB64_NDS_TASK9_STATE_HASH_H
#define SSB64_NDS_TASK9_STATE_HASH_H

#include <PR/ultratypes.h>




#define NDS_TASK9_STATE_HASH_MAX_UPDATES 4096u

typedef struct NDSTask9StateHashRecord
{
    u32 hash1;
    u32 hash2;
    u32 bytes;
    u32 records;
    u32 overflow;
} NDSTask9StateHashRecord;


#endif
