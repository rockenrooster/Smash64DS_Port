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

/* 2026-10-06: the strong grWallpaperCalcPersp is the integer one in
 * battleship_gmcamera_fixed.c (ndsWallpaperPerspQ). */
#pragma weak grWallpaperCalcPersp

#if NDS_P5
/* P5 (Remix FD.asm fix_): Final Destination in a VS battle keeps its plain
 * wallpaper. The boss wallpaper -- its colour steps, its own camera, the
 * boss player it looks up -- belongs to the 1P game, and Remix skips the
 * call everywhere else. */
static void ndsGrWallpaperBossInit(void)
{
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        sc1PGameBossInitWallpaper();
    }
}
#define sc1PGameBossInitWallpaper ndsGrWallpaperBossInit
#define grWallpaperMakeDecideKind ndsGrWallpaperMakeDecideKindSource
#endif

#include "../../decomp/BattleShip-main/decomp/src/gr/grwallpaper.c"

#if NDS_P5
#include <nds/nds_p5.h>
#undef grWallpaperMakeDecideKind

/* P5 (Remix Stages.asm bg_type): a Remix stage draws the common moving
 * wallpaper -- none of the four sets another type -- where the source's own
 * bound would take its kind past nGRKindBonusStageStart for a bonus board's
 * static one. Training keeps the source's own first arm. */
void grWallpaperMakeDecideKind(void)
{
    if ((gSCManagerSceneData.scene_curr != nSCKind1PTrainingMode) &&
        (ndsP5StageIsRemix((u32)gSCManagerBattleState->gkind) != FALSE))
    {
        grWallpaperMakeCommon();
        return;
    }
    ndsGrWallpaperMakeDecideKindSource();
}
#endif
