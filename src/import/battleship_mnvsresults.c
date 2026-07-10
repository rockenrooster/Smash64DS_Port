/* Compile the original BattleShip VS Results scene translation unit. */
#include <string.h>

#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <lb/transition.h>
#include <mn/menu.h>
#include <nds/nds_startup.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

#include "../../decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.h"

extern sb32 (*dLBCommonFuncMatrixList[])(void);
extern void efManagerInitEffects(void);
extern void *efManagerConfettiMakeEffect(Vec3f *pos,
                                         sb32 is_genlink_mask);
extern u32 sGCCommonsActiveNum;
extern u32 sGCSpritesActiveNum;
extern void ndsFighterManagerRegisterDisplayFighter(GObj *fighter_gobj,
                                                     u32 slot);

#define NDS_VS_RESULTS_PASS 0x56535231u

volatile u32 gNdsVSResultsResult;
volatile u32 gNdsVSResultsMask;
volatile u32 gNdsVSResultsStartCount;
volatile u32 gNdsVSResultsTickCount;
volatile u32 gNdsVSResultsLoadedFileCount;
volatile u32 gNdsVSResultsFighterCount;
volatile u32 gNdsVSResultsGObjCount;
volatile u32 gNdsVSResultsSObjCount;
volatile u32 gNdsVSResultsKind;
volatile u32 gNdsVSResultsCameraProcCount;
volatile u32 gNdsVSResultsFighterDisplayCount;
volatile u32 gNdsVSResultsFighterSubmitCount;
volatile u32 gNdsVSResultsFighterPlace[2];
volatile u32 gNdsVSResultsFighterStatus[2];
volatile s32 gNdsVSResultsFighterMotion[2];

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);

void ndsMNVSResultsManagerFuncUpdate(SYTaskmanSetup *setup);
void ndsMNVSResultsSetupFilesKind(s32 fkind);

#define mnVSResultsStartScene ndsBaseMNVSResultsStartScene
#define scManagerFuncUpdate ndsMNVSResultsManagerFuncUpdate
#define ftManagerSetupFilesAllKind ndsMNVSResultsSetupFilesKind

void ndsBaseMNVSResultsStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c"

#undef mnVSResultsStartScene
#undef scManagerFuncUpdate
#undef ftManagerSetupFilesAllKind

void ndsMNVSResultsManagerFuncUpdate(SYTaskmanSetup *setup)
{
    SYTaskmanSetup ds_setup = *setup;

    ds_setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    ds_setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    scManagerFuncUpdate(&ds_setup);
}

void ndsMNVSResultsSetupFilesKind(s32 fkind)
{
    if ((fkind == nFTKindMario) || (fkind == nFTKindFox))
    {
        ftManagerSetupFilesAllKind(fkind);
    }
}

void ndsMNVSResultsRecordFrame(void)
{
    u32 file_count = 0;
    u32 fighter_count = 0;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sMNVSResultsFiles); i++)
    {
        if (sMNVSResultsFiles[i] != NULL)
        {
            file_count++;
        }
    }
    for (i = 0; i < ARRAY_COUNT(sMNVSResultsFighterGObjs); i++)
    {
        if (sMNVSResultsFighterGObjs[i] != NULL)
        {
            FTStruct *fp = ftGetStruct(sMNVSResultsFighterGObjs[i]);

            fighter_count++;
            /* BattleShip ftdef.h uses fighter DL link 9, which is one of the
             * source Results fighter camera's captured links. */
            if ((fp != NULL) && (fp->dl_link != 9))
            {
                ftParamMoveDLLink(sMNVSResultsFighterGObjs[i], 9);
            }
            if ((i < 2u) && (fp != NULL))
            {
                gNdsVSResultsFighterPlace[i] =
                    (u32)sMNVSResultsPlaces[i];
                gNdsVSResultsFighterStatus[i] = fp->status_id;
                gNdsVSResultsFighterMotion[i] = fp->motion_id;
                ndsFighterManagerRegisterDisplayFighter(
                    sMNVSResultsFighterGObjs[i], i);
            }
        }
    }

    gNdsVSResultsTickCount = (u32)sMNVSResultsTotalTimeTics;
    gNdsVSResultsLoadedFileCount = file_count;
    gNdsVSResultsFighterCount = fighter_count;
    gNdsVSResultsGObjCount = sGCCommonsActiveNum;
    gNdsVSResultsSObjCount = sGCSpritesActiveNum;
    gNdsVSResultsKind = (u32)sMNVSResultsKind;
    if (file_count == ARRAY_COUNT(sMNVSResultsFiles))
    {
        gNdsVSResultsMask |= 1u << 0;
    }
    if ((u32)sMNVSResultsTotalTimeTics >=
        (u32)sMNVSResultsDrawWallpaperTic)
    {
        gNdsVSResultsMask |= 1u << 1;
    }
    if ((u32)sMNVSResultsTotalTimeTics >=
        (u32)sMNVSResultsMakeResultsTic)
    {
        gNdsVSResultsMask |= 1u << 2;
    }
    if (fighter_count >= 2u)
    {
        gNdsVSResultsMask |= 1u << 3;
    }
    if (sGCSpritesActiveNum != 0u)
    {
        gNdsVSResultsMask |= 1u << 4;
    }
    if ((gNdsVSResultsMask & 0x1fu) == 0x1fu)
    {
        gNdsVSResultsResult = NDS_VS_RESULTS_PASS;
    }
}

void mnVSResultsStartScene(void)
{
    gNdsVSResultsResult = 0;
    gNdsVSResultsMask = 0;
    gNdsVSResultsTickCount = 0;
    gNdsVSResultsLoadedFileCount = 0;
    gNdsVSResultsFighterCount = 0;
    gNdsVSResultsGObjCount = 0;
    gNdsVSResultsSObjCount = 0;
    gNdsVSResultsKind = 0;
    gNdsVSResultsCameraProcCount = 0;
    gNdsVSResultsFighterDisplayCount = 0;
    gNdsVSResultsFighterSubmitCount = 0;
    memset((void *)gNdsVSResultsFighterPlace, 0,
           sizeof(gNdsVSResultsFighterPlace));
    memset((void *)gNdsVSResultsFighterStatus, 0,
           sizeof(gNdsVSResultsFighterStatus));
    memset((void *)gNdsVSResultsFighterMotion, 0,
           sizeof(gNdsVSResultsFighterMotion));
    gNdsVSResultsStartCount++;
    ndsBaseMNVSResultsStartScene();
}
