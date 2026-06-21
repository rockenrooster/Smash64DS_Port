/* Compile the original BattleShip VS Mode scene translation unit.
 *
 * The DS entry remains bounded: it enters the original VS setup path through
 * taskman, creates the original menu object graph, records proof, and parks
 * before the input/update loop can transition into players or battle setup.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <gm/gmsound.h>
#include <mn/menu.h>
#include <nds/nds_startup.h>
#include <sc/scene.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
extern s32 gcGetGObjsActiveNum(void);
extern u32 sGCCamerasActiveNum;
extern u32 sGCSpritesActiveNum;

extern void mnVSModeFuncLights(Gfx **dls);
sb32 mnVSModeIsTime(void);
void mnVSModeFuncStart(void);

#define mnVSModeFuncStart ndsBaseMNVSModeFuncStart
#define mnVSModeStartScene ndsBaseMNVSModeStartScene

void ndsBaseMNVSModeFuncStart(void);
void ndsBaseMNVSModeStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsmode.c"

#undef mnVSModeFuncStart
#undef mnVSModeStartScene

static u32 ndsMNVSModeCountSObjs(GObj *gobj)
{
    u32 count = 0;
    SObj *sobj = (gobj != NULL) ? SObjGetStruct(gobj) : NULL;

    while (sobj != NULL)
    {
        count++;
        sobj = sobj->next;
    }
    return count;
}

static SYTaskmanSetup ndsMNVSModeMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = dMNVSModeTaskmanSetup;

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = mnVSModeFuncStart;
    return setup;
}

static void ndsMNVSModeRecordButtonProof(void)
{
    u32 mask = 0;

    if ((sMNVSModeButtonGObjVSStart != NULL) &&
        (ndsMNVSModeCountSObjs(sMNVSModeButtonGObjVSStart) == 4u))
    {
        mask |= (1u << 0);
    }
    if ((sMNVSModeButtonGObjRule != NULL) &&
        (ndsMNVSModeCountSObjs(sMNVSModeButtonGObjRule) == 4u))
    {
        mask |= (1u << 1);
    }
    if ((sMNVSModeButtonGObjTimeStock != NULL) &&
        (ndsMNVSModeCountSObjs(sMNVSModeButtonGObjTimeStock) >= 4u))
    {
        mask |= (1u << 2);
    }
    if ((sMNVSModeButtonGObjVSOptions != NULL) &&
        (ndsMNVSModeCountSObjs(sMNVSModeButtonGObjVSOptions) == 4u))
    {
        mask |= (1u << 3);
    }
    if (sMNVSModeRuleValueGObj != NULL)
    {
        mask |= (1u << 4);
    }
    if (sMNVSModeTimeStockValueGObj != NULL)
    {
        mask |= (1u << 5);
    }

    gNdsVSModeOriginalButtonMask = mask;
}

void mnVSModeFuncStart(void)
{
    GObj *main_gobj;
    LBRelocSetup rl_setup;

    gNdsVSModeOriginalFuncStartResult =
        NDS_VS_MODE_ORIGINAL_FUNC_START_PASS;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNVSModeStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNVSModeStatusBuffer);
    rl_setup.force_status_buffer = NULL;
    rl_setup.force_status_buffer_size = 0;

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesListed(dMNVSModeFileIDs, sMNVSModeFiles);

    if ((sMNVSModeFiles[0] != NULL) && (sMNVSModeFiles[1] != NULL))
    {
        gNdsVSModeOriginalLoadedFileCount = 2;
        gNdsVSModeOriginalRelocResult = NDS_VS_MODE_ORIGINAL_RELOC_PASS;
        gNdsVSModeOriginalSetupMask |= (1u << 0);
    }

    main_gobj = gcMakeGObjSPAfter(0, mnVSModeMain, 0,
                                  GOBJ_PRIORITY_DEFAULT);
    if (main_gobj != NULL)
    {
        gNdsVSModeOriginalMainGObjID = main_gobj->id;
        gNdsVSModeOriginalSetupMask |= (1u << 1);
    }

    gcMakeDefaultCameraGObj(0, GOBJ_PRIORITY_DEFAULT, 100, 0,
                            GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));
    if (sGCCamerasActiveNum >= 1u)
    {
        gNdsVSModeOriginalCameraCount = sGCCamerasActiveNum;
        gNdsVSModeOriginalSetupMask |= (1u << 2);
    }

    mnVSModeFuncStartVars();
    mnVSModeMakeBackgroundViewport();
    mnVSModeMakeMenuNameViewport();
    mnVSModeMakeButtonViewport();
    mnVSModeMakeButtonValuegSYRdpViewport();
    gNdsVSModeOriginalSetupMask |= (1u << 3);

    mnVSModeMakeBackground();
    mnVSModeMakeMenuName();
    mnVSModeMakeVSStartButton();
    mnVSModeMakeRuleButton();
    mnVSModeMakeRuleValue();
    mnVSModeMakeTimeStockButton();
    mnVSModeMakeTimeStockValue();
    mnVSModeMakeVSOptionsButton();
    mnVSModeMakeSubtitle();
    gNdsVSModeOriginalSetupMask |= (1u << 4);

    ndsMNVSModeRecordButtonProof();

    gNdsVSModeOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsVSModeOriginalSObjCount = sGCSpritesActiveNum;
    gNdsVSModeOriginalCursorIndex = (u32)sMNVSModeCursorIndex;
    gNdsVSModeOriginalRule = (u32)sMNVSModeRule;
    gNdsVSModeOriginalTime = (u32)sMNVSModeTime;
    gNdsVSModeOriginalStock = (u32)sMNVSModeStock;
    gNdsVSModeOriginalDeferredMask =
        (1u << 0) | /* mnVSModeMain input/update loop */
        (1u << 1) | /* scene transition to PlayersVS / VSOptions */
        (1u << 2);  /* continuous gcDrawAll sprite rendering */
    gNdsVSModeOriginalSetupResult = NDS_VS_MODE_ORIGINAL_SETUP_PASS;
}

void mnVSModeStartScene(void)
{
    dMNVSModeVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNVSModeVideoSetup);

    gNdsVSModeOriginalStartResult = NDS_VS_MODE_ORIGINAL_START_PASS;

    {
        SYTaskmanSetup setup = ndsMNVSModeMakeTaskmanSetup();
        syTaskmanStartTask(&setup);
    }
}
