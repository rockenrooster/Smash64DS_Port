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

/* gsDPSetCycleType / gsDPSetFillColor / gsDPFillRectangle come from the
 * shared <PR/gbi.h> (real F3DEX2 word pairs); the local zero-stubs hid
 * them and zeroed the 1P wallpaper FILL list. */

#include "../../decomp/BattleShip-main/decomp/src/gr/grwallpaper.c"
