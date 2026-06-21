/* Compile the original BattleShip scene manager translation unit. */
#include <nds/nds_scene_harness.h>

#define scManagerRunLoop ndsBaseSCManagerRunLoop

#include "../../decomp/BattleShip-main/decomp/src/sc/scmanager.c"

#undef scManagerRunLoop

void ndsBaseSCManagerRunLoop(sb32 arg);

void scManagerRunLoop(sb32 arg)
{
    ndsDevSceneHarnessApply();
    ndsBaseSCManagerRunLoop(arg);
}
