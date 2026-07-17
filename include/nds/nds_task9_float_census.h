#ifndef SSB64_NDS_TASK9_FLOAT_CENSUS_H
#define SSB64_NDS_TASK9_FLOAT_CENSUS_H

#include <PR/ultratypes.h>

#ifndef NDS_TASK9_FLOAT_CENSUS
#define NDS_TASK9_FLOAT_CENSUS 0
#endif

#define NDS_TASK9_FLOAT_ROUTINE_COUNT 35u

#if NDS_TASK9_FLOAT_CENSUS
extern volatile u32 gNdsTask9FloatCensusArmed;
extern volatile u32 gNdsTask9FloatCensusUpdateCount;
extern volatile u32 gNdsTask9FloatCensusCurrent[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCensusLast[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCensusPair[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCensusTotal[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCensusMin[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCensusMax[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCostTicks[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCostCalls[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCostMin[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatCostMax[NDS_TASK9_FLOAT_ROUTINE_COUNT];
extern volatile u32 gNdsTask9FloatTimerReadPairTicks;
extern volatile u32 gNdsTask9FloatTimerReadPairCount;

void ndsTask9FloatCensusBeginUpdate(void);
void ndsTask9FloatCensusEndUpdate(void);
#endif

#endif
