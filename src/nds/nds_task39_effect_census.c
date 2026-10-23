/* The effect route census tables: each source effect kind's name and route
 * (original, substitute or skipped) from
 * nds/generated/task39_effect_census.generated.h, with no-op reset and
 * record hooks the effect manager and the item and HUD code call. */

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
