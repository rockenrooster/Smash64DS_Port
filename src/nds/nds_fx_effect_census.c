/* The effect route census tables: each source effect kind's name and route
 * (original, substitute or skipped) from
 * nds/generated/fx_effect_census.generated.h, with no-op reset and
 * record hooks the effect manager and the item and HUD code call. */

#include <nds/nds_fx_effect_census.h>

#include <string.h>

const char *const gNdsFxEffectNames[NDS_FX_COUNT] = {
    NDS_FX_NAME_ROWS
};

const u8 gNdsFxEffectRoutes[NDS_FX_COUNT] = {
    NDS_FX_ROUTE_ROWS
};

volatile u32 gNdsFxEffectSpawnCount[NDS_FX_COUNT];
volatile u32 gNdsFxEffectOriginalCount[NDS_FX_COUNT];
volatile u32 gNdsFxEffectSubstituteCount[NDS_FX_COUNT];
volatile u32 gNdsFxEffectSkippedCount[NDS_FX_COUNT];

void ndsFxEffectCensusReset(void)
{
}

void ndsFxEffectCensusRecord(u32 id, u32 route)
{
    (void)id;
    (void)route;
}
