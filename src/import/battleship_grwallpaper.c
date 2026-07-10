#include <PR/gbi.h>
#include <gr/ground.h>
#include <mn/menu.h>
#include <sc/scene.h>
#include <sys/objhelper.h>
#include <sys/taskman.h>
#include <sys/utils.h>
#include <sys/vector.h>

extern GObj *gGMCameraGObj;

void sc1PTrainingModeLoadWallpaper(void);
void sc1PGameBossInitWallpaper(void);

#define gsDPSetCycleType(...) { 0 }
#define gsDPSetFillColor(...) { 0 }
#define gsDPFillRectangle(...) { 0 }

#include "../../decomp/BattleShip-main/decomp/src/gr/grwallpaper.c"
