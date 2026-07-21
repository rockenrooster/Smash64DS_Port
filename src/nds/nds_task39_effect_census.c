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
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    __asm__ volatile("" : : "r"(gNdsTask39EffectRoutes) : "memory");
    memset((void *)gNdsTask39EffectSpawnCount, 0,
           sizeof(gNdsTask39EffectSpawnCount));
    memset((void *)gNdsTask39EffectOriginalCount, 0,
           sizeof(gNdsTask39EffectOriginalCount));
    memset((void *)gNdsTask39EffectSubstituteCount, 0,
           sizeof(gNdsTask39EffectSubstituteCount));
    memset((void *)gNdsTask39EffectSkippedCount, 0,
           sizeof(gNdsTask39EffectSkippedCount));
#endif
}

void ndsTask39EffectCensusRecord(u32 id, u32 route)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    if (id >= NDS_TASK39_EFFECT_COUNT)
    {
        return;
    }
    gNdsTask39EffectSpawnCount[id]++;
    if (route == NDS_TASK39_EFFECT_ORIGINAL)
    {
        gNdsTask39EffectOriginalCount[id]++;
    }
    else if (route == NDS_TASK39_EFFECT_SUBSTITUTE)
    {
        gNdsTask39EffectSubstituteCount[id]++;
    }
    else
    {
        gNdsTask39EffectSkippedCount[id]++;
    }
#else
    (void)id;
    (void)route;
#endif
}
