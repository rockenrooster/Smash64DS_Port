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

#define ftDisplayLightsDrawReflect ndsBaseFtDisplayLightsDrawReflect
#include "../../decomp/BattleShip-main/decomp/src/ft/ftdisplaylights.c"
#undef ftDisplayLightsDrawReflect

/* 2026-10-05 (owner: fixed point only): the reflection light is a pure
 * function of the two angles -- four degree-to-radian divides, two sines, two
 * cosines and the s8 direction a call -- and every fighter draw asks it with
 * the stage's same two angles. The last answer is kept by the angles' bits;
 * a repeat writes the same three direction bytes the source computes, into
 * the same graphics-heap Light, through the same two light commands. */
static u32 sNdsReflectAngleX;
static u32 sNdsReflectAngleY;
static u32 sNdsReflectValid;
static s8 sNdsReflectDir[3];

void ftDisplayLightsDrawReflect(Gfx **display_list, f32 light_angle_x,
                                f32 light_angle_y)
{
    Light *light = (Light *)gSYTaskmanGraphicsHeap.ptr;
    u32 angle_x;
    u32 angle_y;

    __builtin_memcpy(&angle_x, &light_angle_x, sizeof(angle_x));
    __builtin_memcpy(&angle_y, &light_angle_y, sizeof(angle_y));
    if ((sNdsReflectValid == 0u) || (angle_x != sNdsReflectAngleX) ||
        (angle_y != sNdsReflectAngleY))
    {
        ndsBaseFtDisplayLightsDrawReflect(display_list, light_angle_x,
                                          light_angle_y);
        sNdsReflectDir[0] = light->l.dir[0];
        sNdsReflectDir[1] = light->l.dir[1];
        sNdsReflectDir[2] = light->l.dir[2];
        sNdsReflectAngleX = angle_x;
        sNdsReflectAngleY = angle_y;
        sNdsReflectValid = 1u;
        return;
    }
    light->l.dir[0] = sNdsReflectDir[0];
    light->l.dir[1] = sNdsReflectDir[1];
    light->l.dir[2] = sNdsReflectDir[2];

    gSPNumLights(display_list[0]++, 1);
    gSPLight(display_list[0]++, gSYTaskmanGraphicsHeap.ptr, 1);

    gSYTaskmanGraphicsHeap.ptr = (Light *)gSYTaskmanGraphicsHeap.ptr + 1;
}
