#include <nds/nds_task39_effect_census.h>

#include <string.h>

const char *const gNdsTask39EffectNames[NDS_TASK39_EFFECT_COUNT] = {
    NDS_TASK39_EFFECT_NAME_ROWS
};

const u8 gNdsTask39EffectRoutes[NDS_TASK39_EFFECT_COUNT] = {
    NDS_TASK39_EFFECT_ROUTE_ROWS
};

volatile u32 gNdsTask39EffectSpawnCount[NDS_TASK39_EFFECT_COUNT];
volatile u32 gNdsTask39EffectOriginalCount[NDS_TASK39_EFFECT_COUNT];
volatile u32 gNdsTask39EffectSubstituteCount[NDS_TASK39_EFFECT_COUNT];
volatile u32 gNdsTask39EffectSkippedCount[NDS_TASK39_EFFECT_COUNT];

void ndsTask39EffectCensusReset(void)
{
}

void ndsTask39EffectCensusRecord(u32 id, u32 route)
{
    (void)id;
    (void)route;
}
