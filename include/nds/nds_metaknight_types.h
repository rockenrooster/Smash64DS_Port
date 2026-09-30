#ifndef NDS_METAKNIGHT_TYPES_H
#define NDS_METAKNIGHT_TYPES_H

#include <ssb_types.h>

/* Pinned EXTRA MetaKnight source AE4 relative to FTStruct.passive_vars ADC.
 * Preserve the source slots without growing the existing passive union. */
typedef struct FTMetaKnightPassiveVars {
    /* Native storage for the source per-player CharEnvColor moveset override.
     * Meta Knight never reads the first two source passive words. */
    u32 cape_environment;
    s32 source_reserved;
    s32 wing_state;
} FTMetaKnightPassiveVars;

/* Source B30 relative to FTStruct.status_vars B18. Both tornado's cooldown
 * and Cape's movement flag use this one slot, which survives ground/air
 * status transitions just as the source status union does. */
typedef struct FTMetaKnightStatusVars {
    s32 source_reserved[6];
    s32 temp2;
} FTMetaKnightStatusVars;

#endif
