#include <ft/fighter.h>
#include <nds/nds_fighter_display.h>

extern f32 lbCommonSin(f32 angle);
extern f32 lbCommonCos(f32 angle);

#undef gSPNumLights
#define gSPNumLights(pkt, count) \
    ndsFighterDisplayContractSetLightCount(count)
#undef gSPLight
#define gSPLight(pkt, light, slot) \
    ndsFighterDisplayContractSetLight((const Light *)(light), (slot))

#include "../../decomp/BattleShip-main/decomp/src/ft/ftdisplaylights.c"
