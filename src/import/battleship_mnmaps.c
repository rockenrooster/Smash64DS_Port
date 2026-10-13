/* Compile the original BattleShip Maps scene translation unit.
 *
 * The DS entry remains bounded: it runs original stage-select setup through
 * the menu UI graph, proves the Pupupu preview/stage-data path, and can drive
 * one original A-select transition into the existing VSBattle boundary.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
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

#include "nds_build_config.h"

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
extern s32 gcGetGObjsActiveNum(void);
extern u32 sGCCamerasActiveNum;
extern u32 sGCSpritesActiveNum;
extern u32 sGCDrawsActiveNum;
extern u32 sGCMaterialsActive;
extern s32 sSYTaskmanStatus;

extern void mnMapsFuncLights(Gfx **dls);
void mnMapsFuncStart(void);
void mnMapsFuncRun(GObj *gobj);
void mnMapsSetPreviewCameraPosition(CObj *cobj, s32 gkind);

#define mnMapsFuncStart ndsBaseMNMapsFuncStart
#define mnMapsStartScene ndsBaseMNMapsStartScene

void ndsBaseMNMapsFuncStart(void);
void ndsBaseMNMapsStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnmaps/mnmaps.c"

#undef mnMapsFuncStart
#undef mnMapsStartScene

static GObj *sNdsMapsMainGObj;

/* Only the imported StartScene below builds one, and that is compiled out
 * whenever the P2-1f shell owns this scene. */
__attribute__((unused))
static SYTaskmanSetup ndsMNMapsMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = dMNMapsTaskmanSetup;

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = mnMapsFuncStart;
    return setup;
}

static void ndsMNMapsClearControllerState(void)
{
    s32 i;

    gSYControllerConnectedNum = 1;
    for (i = 0; i < MAXCONTROLLERS; i++)
    {
        gSYControllerDeviceStatuses[i] = -1;
        gSYControllerDevices[i].button_tap = 0;
        gSYControllerDevices[i].button_hold = 0;
        gSYControllerDevices[i].button_update = 0;
        gSYControllerDevices[i].button_release = 0;
        gSYControllerDevices[i].stick_range.x = 0;
        gSYControllerDevices[i].stick_range.y = 0;
    }
    gSYControllerDeviceStatuses[0] = 0;
}

void mnMapsFuncStart(void)
{
    GObj *main_gobj;
    LBRelocSetup rl_setup;
    s32 gkind;

    gNdsMapsOriginalFuncStartResult = NDS_MAPS_ORIGINAL_FUNC_START_PASS;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNMapsStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNMapsStatusBuffer);
    rl_setup.force_status_buffer = sMNMapsForceStatusBuffer;
    rl_setup.force_status_buffer_size = ARRAY_COUNT(sMNMapsForceStatusBuffer);

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesListed(dMNMapsFileIDs, sMNMapsFiles);
    if (gNdsMapsOriginalLoadedFileCount == 5u)
    {
        gNdsMapsOriginalSetupMask |= (1u << 0);
    }

    mnMapsAllocModelHeaps();
    if ((sMNMapsModelHeap0 != NULL) && (sMNMapsModelHeap1 != NULL))
    {
        gNdsMapsOriginalSetupMask |= (1u << 1);
    }

    main_gobj = gcMakeGObjSPAfter(0, mnMapsFuncRun, 0,
                                  GOBJ_PRIORITY_DEFAULT);
    sNdsMapsMainGObj = main_gobj;
    if (main_gobj != NULL)
    {
        gNdsMapsOriginalMainGObjID = main_gobj->id;
        gNdsMapsOriginalSetupMask |= (1u << 2);
    }

    gcMakeDefaultCameraGObj(1, GOBJ_PRIORITY_DEFAULT, 100,
                            COBJ_FLAG_ZBUFFER,
                            GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));
    if (sGCCamerasActiveNum >= 1u)
    {
        gNdsMapsOriginalCameraCount = sGCCamerasActiveNum;
        gNdsMapsOriginalSetupMask |= (1u << 3);
    }

    ndsMNMapsClearControllerState();
    mnMapsInitVars();
    gNdsMapsOriginalSetupMask |= (1u << 4);

    mnMapsMakeWallpaperCamera();
    mnMapsMakeLabelsViewport();
    mnMapsMakeIconsCamera();
    mnMapsMakeNameAndEmblemCamera();
    mnMapsMakeCursorCamera();
    mnMapsMakePreviewCamera();
    mnMapsMakePlaqueCamera();
    mnMapsMakePreviewWallpaperCamera();
    gNdsMapsOriginalSetupMask |= (1u << 5);

    mnMapsMakeWallpaper();
    mnMapsMakePlaque();
    mnMapsMakeLabels();
    mnMapsMakeIcons();
    mnMapsMakeNameAndEmblem(sMNMapsCursorSlot);
    mnMapsMakeCursor();
    gNdsMapsOriginalSetupMask |= (1u << 6);

    gkind = mnMapsGetGroundKind(sMNMapsCursorSlot);
    if (gkind == nGRKindPupupu)
    {
        u32 gobj_before = (u32)gcGetGObjsActiveNum();
        u32 sobj_before = sGCSpritesActiveNum;
        u32 dobj_before = sGCDrawsActiveNum;
        u32 mobj_before = sGCMaterialsActive;

        mnMapsMakePreview(gkind);

        gNdsMapsOriginalPreviewResult = NDS_STAGE_PUPUPU_PREVIEW_PASS;
        if (gNdsStagePupupuRelocAssetMask & (1u << 0))
        {
            gNdsMapsOriginalPreviewMask |= (1u << 0);
        }
        if (gNdsStagePupupuGroundDataPtrReady != 0u)
        {
            gNdsMapsOriginalPreviewMask |= (1u << 1);
        }
        if (gNdsStagePupupuWallpaperPtrReady != 0u)
        {
            gNdsMapsOriginalPreviewMask |= (1u << 2);
        }
        gNdsMapsOriginalPreviewMask |= (1u << 3);

        gNdsMapsOriginalPreviewGObjCount =
            (u32)gcGetGObjsActiveNum() - gobj_before;
        gNdsMapsOriginalPreviewWallpaperMade =
            (sGCSpritesActiveNum > sobj_before) ? 1u : 0u;
        gNdsMapsOriginalPreviewDObjCount = sGCDrawsActiveNum - dobj_before;
        gNdsMapsOriginalPreviewMObjCount = sGCMaterialsActive - mobj_before;
        if (gNdsMapsOriginalPreviewGObjCount != 0u)
        {
            gNdsMapsOriginalPreviewMask |= (1u << 4);
        }
        if (gNdsMapsOriginalPreviewDObjCount != 0u)
        {
            gNdsMapsOriginalPreviewModelMade = 1;
            gNdsMapsOriginalPreviewMask |= (1u << 5);
        }
        gNdsMapsOriginalPreviewLayerGObjMask = 0xfu;
        gNdsMapsOriginalPreviewMask |= (1u << 6);
        gNdsMapsOriginalPreviewMask |= (1u << 7);

        gNdsMapsOriginalPreviewDeferred = 0;
        gNdsMapsOriginalDeferredMask &= ~(1u << 0);
        if ((gNdsMapsOriginalPreviewMask & 0xffu) == 0xffu)
        {
            gNdsMapsOriginalSetupMask |= (1u << 7);
        }
    }
    else
    {
        gNdsMapsOriginalPreviewDeferred = 1;
        gNdsMapsOriginalDeferredMask |= (1u << 0);
        gNdsMapsOriginalSetupMask |= (1u << 7);
    }

    gNdsMapsOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsMapsOriginalSObjCount = sGCSpritesActiveNum;
    gNdsMapsOriginalCursorSlot = (u32)sMNMapsCursorSlot;
    gNdsMapsOriginalGroundKind = (u32)gkind;
    gNdsMapsOriginalIsTrainingMode = (u32)sMNMapsIsTrainingMode;
    gNdsMapsOriginalSetupResult = NDS_MAPS_ORIGINAL_SETUP_PASS;
}

static void ndsMNMapsRunMainTick(u32 *updates)
{
    mnMapsFuncRun(sNdsMapsMainGObj);
    (*updates)++;
    gNdsMapsSelectTransitionUpdateCount = *updates;
}

void ndsMNMapsRunSelectVSBattleProbe(void)
{
    u32 updates = 0;
    u32 mask = 0;
    u32 i;

    gNdsMapsSelectTransitionResult = NDS_MAPS_SELECT_TRANSITION_FAIL;
    gNdsMapsSelectTransitionMask = 0;
    gNdsMapsSelectTransitionScenePrevBefore = gSCManagerSceneData.scene_prev;
    gNdsMapsSelectTransitionSceneCurrBefore = gSCManagerSceneData.scene_curr;

    if (((gNdsMapsOriginalSetupMask & 0xffu) == 0xffu) &&
        (sNdsMapsMainGObj != NULL))
    {
        mask |= (1u << 0);
    }
    if ((sMNMapsCursorSlot == 6) &&
        (mnMapsGetGroundKind(sMNMapsCursorSlot) == nGRKindPupupu))
    {
        mask |= (1u << 1);
    }

    ndsMNMapsClearControllerState();
    for (i = 0; i < 9u; i++)
    {
        ndsMNMapsRunMainTick(&updates);
        ndsMNMapsClearControllerState();
    }
    if (sMNMapsTotalTimeTics >= 9)
    {
        mask |= (1u << 2);
    }

    gNdsMapsSelectTransitionInputMask = A_BUTTON;
    gSYControllerDevices[0].button_tap = A_BUTTON;
    gSYControllerDevices[0].button_hold = A_BUTTON;
    ndsMNMapsRunMainTick(&updates);

    if (gNdsMapsSelectTransitionInputMask == A_BUTTON)
    {
        mask |= (1u << 3);
    }
    if (gSCManagerSceneData.gkind == nGRKindPupupu)
    {
        mask |= (1u << 4);
    }
    if ((gSCManagerSceneData.scene_prev == nSCKindMaps) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle))
    {
        mask |= (1u << 5);
    }

    gNdsMapsSelectTransitionTaskmanStatus = (u32)sSYTaskmanStatus;
    if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
    {
        mask |= (1u << 6);
    }

    gcEjectAll();
    NDS_DIAG(gNdsMapsSelectTransitionCleanupCount++);
    mask |= (1u << 7);

    gNdsMapsSelectTransitionSelectedSlot = (u32)sMNMapsCursorSlot;
    gNdsMapsSelectTransitionSelectedGKind =
        (u32)mnMapsGetGroundKind(sMNMapsCursorSlot);
    gNdsMapsSelectTransitionScenePrevFinal = gSCManagerSceneData.scene_prev;
    gNdsMapsSelectTransitionSceneCurrFinal = gSCManagerSceneData.scene_curr;
    gNdsMapsSelectTransitionMask = mask;
    if (mask == 0xffu)
    {
        gNdsMapsSelectTransitionResult = NDS_MAPS_SELECT_TRANSITION_PASS;
    }
}

#if !NDS_P2_MENU_SHELL
/* P2-1f defines the real stage-select scene in src/nds/nds_menu_shell.c. This
 * one runs the ORIGINAL mnMapsFuncStart, which loads five menu files,
 * allocates two model heaps sized to the largest stage map file and builds the
 * whole preview object graph -- the bounded proof path, not a playable
 * screen. Same split P2-1e made for the character select. */
void mnMapsStartScene(void)
{
    dMNMapsVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNMapsVideoSetup);

    gNdsMapsOriginalStartResult = NDS_MAPS_ORIGINAL_START_PASS;

    {
        SYTaskmanSetup setup = ndsMNMapsMakeTaskmanSetup();
        scManagerFuncUpdate(&setup);
    }
}
#endif

#if NDS_P2_MENU_SHELL
/* THE STAGE SELECT'S 3D PREVIEW (owner BUGS.md 2026-10-12: "SSS: missing 3D
 * rendered previews"). mnMapsMakePreview loads the ground's map file, builds
 * its four display layers (mnMapsMakeLayer: the ground descriptors' DObjs,
 * MObjs and first animation frame at a per-stage scale) and draws them
 * through a camera that only bobs (mnmaps.c:1100/:1027/:1336). The shell's
 * stage select keeps its own 2D screen (the baked panel is the source's
 * preview wallpaper) and runs just this part of the scene: the source's
 * preview camera and the four layers, built the way the battle builds them
 * (grDisplayMakeGeometryLayer's procs and display links, no processes, the
 * preview's scale and hidden parts) so the native stage owner draws them
 * from the stage's own packet (ndsRendererAdapterSetStagePreviewLayers). A
 * packet that also names battle-only actors (Zebes' acid, Saffron's gate,
 * Mushroom Kingdom's scale, Dream Land's map objects) gets hidden stand-ins
 * for them (ndsMNMapsPreviewMakeActors).
 *
 * The packet's assets, the map file's tree and the native packet blob load
 * into one block of the screen's heap, reset for each stage the cursor rests
 * on (one stage at a time, where the source keeps two). */
#include <nds/nds_native_stage_blob.h>
#include <nds/nds_reloc_assets.h>
extern void grDisplayDObjSetNoAnimXObj(GObj *ground_gobj, DObjDesc *dobjdesc);

extern GRDisplayDesc dGRDisplayDescs[];
extern volatile u32 gNdsSCVSBattleStageGKind;
extern volatile u32 gNdsNativeStageWarmUploads;
extern u32 cpuGetTiming(void);
extern s32 ndsRendererAdapterNativeStagePreloadAsset(u32 index);
extern size_t ndsRelocExternTreeUnloadedSize(const void *file_id);
extern void ndsRendererHardwareReleaseStageWarmTextures(void);
extern void (*gNdsRendererTexturePrefilter)(u16 *texels, u32 width,
                                            u32 height);
extern volatile u32 gNdsNativeStageWarmUploadLimit;
extern void ndsRendererAdapterSetStagePreviewLayers(void *const *layers);
extern volatile u32 gNdsRendererAdapterStagePreviewArmed;
extern void ndsPlatformSet3DLayerEnabled(s32 is_enabled);
extern void syTaskmanResetGraphicsHeap(void);
extern void func_80004AB0(void);

/* Tics the cursor rests on a stage before its preview loads: a hold that
 * scrolls the row (12 tics a step) never loads the stages it passes. */
#define NDS_MAPS_PREVIEW_DWELL_TICS 6u
#define NDS_MAPS_PREVIEW_GKIND_NONE (-1)
/* The packet blob's body cap (nds_native_stage_blob.c) and the slack the
 * blob header, alignment and a preloaded asset outside the tree may take. */
#define NDS_MAPS_PREVIEW_BLOB_BYTES (48u * 1024u)
/* A stage's first draws each upload textures for about this many ticks
 * (at least one): 0.7 of a VBlank. */
#define NDS_MAPS_PREVIEW_WARM_TICKS 400000u

enum
{
    NDS_MAPS_PREVIEW_STEP_IDLE,
    NDS_MAPS_PREVIEW_STEP_ASSETS, /* the packet's assets, one a tic */
    NDS_MAPS_PREVIEW_STEP_MAP,    /* the map file, its tree, the packet */
    NDS_MAPS_PREVIEW_STEP_MODEL,  /* the layers, the actors, the camera */
    NDS_MAPS_PREVIEW_STEP_WARM    /* drawing until a draw is whole */
};

__attribute__((used)) volatile u32 gNdsMapsPreviewLoads;
__attribute__((used)) volatile u32 gNdsMapsPreviewDraws;
__attribute__((used)) volatile u32 gNdsMapsPreviewNativeDraws;
__attribute__((used)) volatile u32 gNdsMapsPreviewFailStep;
__attribute__((used)) volatile s32 gNdsMapsPreviewShownGkind = -1;
__attribute__((used)) volatile u32 gNdsMapsPreviewBlockBytes;
/* The last load's ticks over its steps, the slowest step, and the slowest
 * draw and the first after a load. */
__attribute__((used)) volatile u32 gNdsMapsPreviewStepTicksMax;
__attribute__((used)) volatile u32 gNdsMapsPreviewLoadTicks;
__attribute__((used)) volatile u32 gNdsMapsPreviewLoadTicksMax;
__attribute__((used)) volatile u32 gNdsMapsPreviewDrawTicksMax;
__attribute__((used)) volatile u32 gNdsMapsPreviewFirstDrawTicks;
/* Of the file ticks, those spent waiting on the ARM7's card reads. */
__attribute__((used)) volatile u32 gNdsMapsPreviewFileWaitTicks;
extern volatile u32 gNdsAudioStorageWaitTicks64;

/* Map-file offsets of the battle-only actors' data, as the ground imports
 * name them (battleship_grzebes_ground.c, battleship_gryamabuki_ground.c,
 * battleship_grinishie_ground.c); Dream Land's are menu symbols. */
#define NDS_MAPS_PREVIEW_ZEBES_ACID_MOBJSUB 0x8c0u     /* llGRZebesMapAcidMObjSub */
#define NDS_MAPS_PREVIEW_ZEBES_ACID_DOBJDESC 0xb08u    /* llGRZebesMapAcidDObjDesc */
#define NDS_MAPS_PREVIEW_YAMABUKI_GATE_DOBJDESC 0x8a0u /* llGRYamabukiMapMapHead */
#define NDS_MAPS_PREVIEW_INISHIE_SCALE_DOBJDESC 0x380u /* llGRInishieMapScaleDObjDesc */
#define NDS_MAPS_PREVIEW_INISHIE_PLATFORM_DL 0x5f0u    /* llGRInishieMapMapHead */
/* renderer_adapter_matrix.c's NDS_RENDERER_ADAPTER_STAGE_CAPTURE_* */
#define NDS_MAPS_PREVIEW_CAPTURE_PUPUPU_MAP 1u
#define NDS_MAPS_PREVIEW_CAPTURE_ZEBES_ACID 2u
#define NDS_MAPS_PREVIEW_CAPTURE_INISHIE_SCALE_TREE 3u
#define NDS_MAPS_PREVIEW_CAPTURE_INISHIE_SCALE_PLATFORM 4u
#define NDS_MAPS_PREVIEW_CAPTURE_YAMABUKI_GATE 5u
#define NDS_MAPS_PREVIEW_ACTOR_MAX 4

extern DObjTransformTypes dGRInishieScaleTransformKinds[];
extern void ndsRendererAdapterSetStagePreviewActor(u32 source, u32 index,
                                                   void *gobj);

static sb32 sNdsMapsPreviewActive;
static GObj *sNdsMapsPreviewActors[NDS_MAPS_PREVIEW_ACTOR_MAX];
static s32 sNdsMapsPreviewShown = NDS_MAPS_PREVIEW_GKIND_NONE;
static s32 sNdsMapsPreviewWanted = NDS_MAPS_PREVIEW_GKIND_NONE;
static u32 sNdsMapsPreviewDwell;
static u32 sNdsMapsPreviewDrawPhase;
static sb32 sNdsMapsPreviewHasModel;
static sb32 sNdsMapsPreviewFreshDraw;
static sb32 sNdsMapsPreviewVisible;
static u32 sNdsMapsPreviewStep;
static u32 sNdsMapsPreviewAsset;
static MPGroundData *sNdsMapsPreviewGroundData;
static u32 sNdsMapsPreviewLoadTicks;
static u32 sNdsMapsPreviewLoadWait;
static GObj *sNdsMapsPreviewCamera;
static GObj *sNdsMapsPreviewLayers[4];
static u8 *sNdsMapsPreviewBlock;
static size_t sNdsMapsPreviewBlockSize;
static SYMallocRegion sNdsMapsPreviewArena;

#if NDS_P5
#include "nds_p5_preview.generated.h"
#include "nds_p5_native_stages.generated.h"
/* P5: the 1P arenas the select offers (docs/P5/New_Stages.md), by the
 * battle's own file infos (mpcollision.c:39-44), which are the headers Remix's
 * stage_file_table names for its preview too. */
static GRFileInfo sNdsMapsPreviewVenueFileInfos[] =
{
    { &llGRMetalMapFileID, &llGRMetalMapMapHeader },
    { &llGRZakoMapFileID,  &llGRZakoMapMapHeader },
    { &llGRLastMapFileID,  &llGRLastMapMapHeader }
};
#endif

/* The map file and header a preview loads: dMNMapsFileInfos for the nine,
 * P5's rows past them, NULL for RANDOM and anything else. */
static GRFileInfo *ndsMNMapsPreviewFileInfo(s32 gkind)
{
    if ((gkind >= 0) && (gkind < (s32)ARRAY_COUNT(dMNMapsFileInfos)))
    {
        return &dMNMapsFileInfos[gkind];
    }
#if NDS_P5
    switch (gkind)
    {
    case nGRKindMetal:
        return &sNdsMapsPreviewVenueFileInfos[0];
    case nGRKindZako:
        return &sNdsMapsPreviewVenueFileInfos[1];
    case nGRKindLast:
        return &sNdsMapsPreviewVenueFileInfos[2];
    /* The Remix stages this build carries: their map file and header
     * (Remix stage_file_table, through scripts/p5/stage_export.py). */
#define NDS_MAPS_P5_REMIX_INFO(g, cname, name, map_file, map_offset) \
    case (g): \
    { \
        static GRFileInfo info = { (intptr_t)(map_file), (intptr_t)(map_offset) }; \
        return &info; \
    }
    NDS_P5_NATIVE_STAGE_ROWS(NDS_MAPS_P5_REMIX_INFO)
#undef NDS_MAPS_P5_REMIX_INFO
    default:
        break;
    }
#endif
    return NULL;
}

/* Remix Stages.asm random_fix_2_: a stage past the nine takes Peach's
 * Castle's camera target, "for consistency" (its position_table sets none). */
static s32 ndsMNMapsPreviewCameraGkind(s32 gkind)
{
    return ((gkind > (s32)nGRKindInishie) && (gkind != 0xDE)) ?
        (s32)nGRKindCastle : gkind;
}

/* mnMapsSetPreviewCameraPosition, then P5's panel offset: the 6x3 grid moves
 * the preview panel down NDS_P5_PREVIEW_DY source rows (p5_stages.py
 * LOWER_DY), and the model moves with it. The camera's viewport is the whole
 * frame, so the model is lifted with the camera instead: eye and target rise
 * together, which carries the target's depth down the screen by the same
 * rows -- at the preview's 32.59-degree fovy and the ~10,200-unit
 * eye-to-target distance, 8 rows of the 220-row viewport are 217 units. */
static void ndsMNMapsPreviewPlaceCamera(CObj *cobj, s32 gkind)
{
    mnMapsSetPreviewCameraPosition(cobj, ndsMNMapsPreviewCameraGkind(gkind));
#if NDS_P5
    cobj->vec.eye.y += NDS_P5_PREVIEW_LIFT;
    cobj->vec.at.y += NDS_P5_PREVIEW_LIFT;
#endif
}

/* mnMapsMakeLayer's model, on grDisplayMakeGeometryLayer's procs and display
 * links (the native stage packet's capture rows) and with no process: the
 * source's preview plays each layer's first animation frame once. */
static GObj *ndsMNMapsPreviewMakeLayer(s32 gkind, MPGroundData *ground_data,
                                       s32 id)
{
    f32 scales[/* */] =
    {
        0.5F, 0.2F, 0.6F,
        0.5F, 0.3F, 0.6F,
        0.5F, 0.4F, 0.2F
    };
    MPGroundDesc *gr_desc = &ground_data->gr_desc[id];
    GObj *gobj;
    f32 scale;

    if ((gkind >= 0) && (gkind < (s32)ARRAY_COUNT(scales)))
    {
        scale = scales[gkind];
    }
    else
    {
        /* Remix set_preview_position_: its zoom_table's scale. */
        scale = 0.5F;
#if NDS_P5
#define NDS_MAPS_PREVIEW_P5_ZOOM(g, zoom) if (gkind == (g)) { scale = (zoom); }
        NDS_P5_PREVIEW_ROWS(NDS_MAPS_PREVIEW_P5_ZOOM)
#undef NDS_MAPS_PREVIEW_P5_ZOOM
#endif
    }
    if (gr_desc->dobjdesc == NULL)
    {
        return NULL;
    }
    gobj = gcMakeGObjSPAfter(nGCCommonKindGroundDisplay, NULL,
                             nGCCommonLinkIDGroundDisplay,
                             GOBJ_PRIORITY_DEFAULT);
    if (gobj == NULL)
    {
        return NULL;
    }
    gcAddGObjDisplay(gobj,
                     (ground_data->layer_mask & (1 << id)) ?
                         dGRDisplayDescs[id].sec_proc_display :
                         dGRDisplayDescs[id].pri_proc_display,
                     dGRDisplayDescs[id].dl_link, GOBJ_PRIORITY_DEFAULT, ~0);
    gcSetupCustomDObjs(gobj, gr_desc->dobjdesc, NULL,
                       nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull,
                       nGCMatrixKindNull);
    if (gr_desc->p_mobjsubs != NULL)
    {
        gcAddMObjAll(gobj, gr_desc->p_mobjsubs);
    }
    if ((gr_desc->anim_joints != NULL) || (gr_desc->p_matanim_joints != NULL))
    {
        gcAddAnimAll(gobj, gr_desc->anim_joints, gr_desc->p_matanim_joints,
                     0.0F);
        gcPlayAnimAll(gobj);
    }
    grDisplayDObjSetNoAnimXObj(gobj, gr_desc->dobjdesc);
    DObjGetStruct(gobj)->scale.vec.f.x = scale;
    DObjGetStruct(gobj)->scale.vec.f.y = scale;
    DObjGetStruct(gobj)->scale.vec.f.z = scale;
    return gobj;
}

/* One battle-only actor's stand-in: its GObj on the battle's proc and link,
 * its DObjs from the map file with the battle's matrix kind, its MObjs, no
 * process and no animation, the root hidden -- mnMapsMakeModel shows the
 * four layers alone, and the owner admits a packet only whole. */
static GObj *ndsMNMapsPreviewMakeActor(u32 slot, u32 source, u32 index,
                                       void (*proc_display)(GObj *),
                                       u8 dl_link)
{
    GObj *gobj;

    if (slot >= NDS_MAPS_PREVIEW_ACTOR_MAX)
    {
        return NULL;
    }
    gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround,
                             GOBJ_PRIORITY_DEFAULT);
    if (gobj == NULL)
    {
        return NULL;
    }
    gcAddGObjDisplay(gobj, proc_display, dl_link, GOBJ_PRIORITY_DEFAULT, ~0);
    sNdsMapsPreviewActors[slot] = gobj;
    ndsRendererAdapterSetStagePreviewActor(source, index, gobj);
    return gobj;
}

static void ndsMNMapsPreviewHideActor(GObj *gobj)
{
    if ((gobj != NULL) && (DObjGetStruct(gobj) != NULL))
    {
        DObjGetStruct(gobj)->flags |= DOBJ_FLAG_HIDDEN;
    }
}

/* The actors a stage's packet captures beside its layers, as
 * grZebesMakeAcid, grYamabukiMakeGate, grInishieMakeScale and
 * grPupupuInitAll build them: from the file the ground data's map_nodes
 * points into, its base found the way each of them finds it. */
static void ndsMNMapsPreviewMakeActors(s32 gkind, MPGroundData *ground_data)
{
    uintptr_t base;
    void *map_head;
    GObj *gobj;
    s32 i;

    switch (gkind)
    {
    case nGRKindZebes:
        base = NDS_MAPS_PREVIEW_ZEBES_ACID_DOBJDESC;
        break;
    case nGRKindYamabuki:
        base = NDS_MAPS_PREVIEW_YAMABUKI_GATE_DOBJDESC;
        break;
    case nGRKindInishie:
        base = NDS_MAPS_PREVIEW_INISHIE_PLATFORM_DL;
        break;
    case nGRKindPupupu:
        base = llGRPupupuMapMapHead;
        break;
    default:
        return;
    }
    if ((ground_data == NULL) || (ground_data->map_nodes == NULL))
    {
        return;
    }
    base = (uintptr_t)ground_data->map_nodes - base;
    map_head = (void *)base;

    switch (gkind)
    {
    case nGRKindZebes:
        gobj = ndsMNMapsPreviewMakeActor(0u,
            NDS_MAPS_PREVIEW_CAPTURE_ZEBES_ACID, 0u,
            gcDrawDObjTreeDLLinksForGObj, 12);
        if (gobj != NULL)
        {
            gcSetupCustomDObjs(gobj,
                (DObjDesc *)(base + NDS_MAPS_PREVIEW_ZEBES_ACID_DOBJDESC),
                NULL, nGCMatrixKindTra, nGCMatrixKindNull, nGCMatrixKindNull);
            gcAddMObjAll(gobj, lbRelocGetFileData(MObjSub ***, map_head,
                (const void *)(uintptr_t)NDS_MAPS_PREVIEW_ZEBES_ACID_MOBJSUB));
            ndsMNMapsPreviewHideActor(gobj);
        }
        break;

    case nGRKindYamabuki:
        gobj = ndsMNMapsPreviewMakeActor(0u,
            NDS_MAPS_PREVIEW_CAPTURE_YAMABUKI_GATE, 0u,
            gcDrawDObjTreeDLLinksForGObj, 6);
        if (gobj != NULL)
        {
            gcSetupCustomDObjs(gobj,
                (DObjDesc *)(base + NDS_MAPS_PREVIEW_YAMABUKI_GATE_DOBJDESC),
                NULL, nGCMatrixKindTraRotRpyR, nGCMatrixKindNull,
                nGCMatrixKindNull);
            ndsMNMapsPreviewHideActor(gobj);
        }
        break;

    case nGRKindInishie:
        gobj = ndsMNMapsPreviewMakeActor(0u,
            NDS_MAPS_PREVIEW_CAPTURE_INISHIE_SCALE_TREE, 0u,
            gcDrawDObjTreeForGObj, 6);
        if (gobj != NULL)
        {
            grModelSetupGroundDObjs(gobj,
                (DObjDesc *)(base + NDS_MAPS_PREVIEW_INISHIE_SCALE_DOBJDESC),
                NULL, dGRInishieScaleTransformKinds);
            ndsMNMapsPreviewHideActor(gobj);
        }
        for (i = 0; i < 2; i++)
        {
            gobj = ndsMNMapsPreviewMakeActor((u32)(1 + i),
                NDS_MAPS_PREVIEW_CAPTURE_INISHIE_SCALE_PLATFORM, (u32)i,
                gcDrawDObjDLHead0, 6);
            if (gobj != NULL)
            {
                DObj *dobj = gcAddDObjForGObj(gobj,
                    (void *)(base + NDS_MAPS_PREVIEW_INISHIE_PLATFORM_DL));

                if (dobj != NULL)
                {
                    gcAddXObjForDObjFixed(dobj, nGCMatrixKindTra, 0);
                }
                ndsMNMapsPreviewHideActor(gobj);
            }
        }
        break;

    case nGRKindPupupu:
    {
        const uintptr_t descs[4] =
        {
            llGRPupupuMapMapHead,
            llGRPupupuMapWhispyMouthTransformKindsDObjDesc,
            llGRPupupuMapFlowersBackTransformKindsDObjDesc,
            llGRPupupuMapFlowersFrontTransformKindsDObjDesc
        };
        const void *mobjsubs[4] =
        {
            &llGRPupupuMapWhispyEyesTransformKindsMObjSub,
            &llGRPupupuMapWhispyMouthTransformKindsMObjSub,
            NULL,
            NULL
        };

        for (i = 0; i < 4; i++)
        {
            gobj = ndsMNMapsPreviewMakeActor((u32)i,
                NDS_MAPS_PREVIEW_CAPTURE_PUPUPU_MAP, (u32)i,
                (i == 3) ? grDisplayLayer3PriProcDisplay :
                           grDisplayLayer0PriProcDisplay,
                (i == 3) ? 16 : 4);
            if (gobj == NULL)
            {
                continue;
            }
            gcSetupCustomDObjs(gobj, (DObjDesc *)(base + descs[i]), NULL,
                               nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull,
                               nGCMatrixKindNull);
            if (mobjsubs[i] != NULL)
            {
                gcAddMObjAll(gobj, lbRelocGetFileData(MObjSub ***, map_head,
                                                      mobjsubs[i]));
            }
            ndsMNMapsPreviewHideActor(gobj);
        }
        break;
    }

    default:
        break;
    }
}

static void ndsMNMapsPreviewDropModel(void)
{
    s32 i;

    ndsRendererAdapterSetStagePreviewLayers(NULL);
    for (i = 0; i < NDS_MAPS_PREVIEW_ACTOR_MAX; i++)
    {
        if (sNdsMapsPreviewActors[i] != NULL)
        {
            gcEjectGObj(sNdsMapsPreviewActors[i]);
            sNdsMapsPreviewActors[i] = NULL;
        }
    }
    for (i = 0; i < 4; i++)
    {
        if (sNdsMapsPreviewLayers[i] != NULL)
        {
            gcEjectGObj(sNdsMapsPreviewLayers[i]);
            sNdsMapsPreviewLayers[i] = NULL;
        }
    }
    sNdsMapsPreviewHasModel = FALSE;
    /* The stage's warm textures (eviction skips them): the next stage needs
     * the VRAM (the owner's playtest lost Zebes, Mushroom Kingdom, Yoshi's
     * Island and Saffron to it). */
    ndsRendererHardwareReleaseStageWarmTextures();
    if (sNdsMapsPreviewBlock != NULL)
    {
        ndsRelocReleaseHeapRange(sNdsMapsPreviewBlock,
                                 sNdsMapsPreviewBlockSize);
        syMallocReset(&sNdsMapsPreviewArena);
    }
}

/* mnMapsMakePreview for one stage, a step a tic so the select never stalls
 * on it (a whole load and its first draw took 8-11M ticks, 15-20 VBlanks,
 * in one tic): the packet's assets one apiece into the block, then the map
 * file and its tree with the packet, then the layers (mnMapsMakeModel's
 * hides included) and the camera's target; the draws after upload the
 * stage's textures a little at a time (gNdsNativeStageWarmUploadLimit) and
 * the model shows from the first whole one. */
static void ndsMNMapsPreviewBegin(s32 gkind)
{
    ndsMNMapsPreviewDropModel();
    sNdsMapsPreviewShown = gkind;
    gNdsMapsPreviewShownGkind = gkind;
    sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_IDLE;
    if ((ndsMNMapsPreviewFileInfo(gkind) == NULL) ||
        (sNdsMapsPreviewBlock == NULL))
    {
        /* RANDOM shows the panel's own art and no model (mnmaps.c:1102). */
        return;
    }
    gNdsMapsPreviewLoads++;
    sNdsMapsPreviewLoadTicks = 0u;
    sNdsMapsPreviewLoadWait = gNdsAudioStorageWaitTicks64;
    /* The packet the owner reads, and the assets it names, are the battle
     * stage kind's (ndsRendererAdapterNativeStageDescriptor). */
    gNdsSCVSBattleStageGKind = (u32)gkind;
    sNdsMapsPreviewAsset = 0u;
    sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_ASSETS;
}

static void ndsMNMapsPreviewFail(u32 step)
{
    gNdsMapsPreviewFailStep = step;
    ndsMNMapsPreviewDropModel();
    sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_IDLE;
}

static void ndsMNMapsPreviewStepLoad(void)
{
    s32 gkind = sNdsMapsPreviewShown;
    SYMallocRegion *previous;
    u32 step_start = cpuGetTiming();
    s32 i;

    switch (sNdsMapsPreviewStep)
    {
    case NDS_MAPS_PREVIEW_STEP_ASSETS:
        previous = ndsTaskmanSwapMallocRegion(&sNdsMapsPreviewArena);
        if (ndsRendererAdapterNativeStagePreloadAsset(sNdsMapsPreviewAsset) !=
            FALSE)
        {
            sNdsMapsPreviewAsset++;
        }
        else
        {
            sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_MAP;
        }
        ndsTaskmanSwapMallocRegion(previous);
        break;

    case NDS_MAPS_PREVIEW_STEP_MAP:
    {
        GRFileInfo *info = ndsMNMapsPreviewFileInfo(gkind);
        size_t bytes;
        void *heap;
        void *file;

        previous = ndsTaskmanSwapMallocRegion(&sNdsMapsPreviewArena);
        /* The packet's assets are loaded: the tree load writes only what
         * they left out (lbRelocGetFileSize sizes the whole tree). */
        bytes = ndsRelocExternTreeUnloadedSize(info->file_id);
        heap = syTaskmanMalloc((bytes != 0u) ? bytes : 0x10u, 0x10);
        file = lbRelocGetExternHeapFile(info->file_id, heap);
        sNdsMapsPreviewGroundData = (file != NULL) ?
            lbRelocGetFileData(MPGroundData*, file, info->offset) : NULL;
        if (sNdsMapsPreviewGroundData != NULL)
        {
            (void)ndsNativeStageBlobLoad((u32)gkind);
            /* The battle pins Dream Land's textures with its static set
             * before its first draw; the preview has no such set, so its
             * first prepares upload on a miss as a loaded packet's do. */
            gNdsNativeStageWarmUploads = 1u;
        }
        ndsTaskmanSwapMallocRegion(previous);
        gNdsMapsPreviewFileWaitTicks =
            (gNdsAudioStorageWaitTicks64 - sNdsMapsPreviewLoadWait) * 64u;
        if (sNdsMapsPreviewGroundData == NULL)
        {
            ndsMNMapsPreviewFail(1u);
            return;
        }
        sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_MODEL;
        break;
    }

    case NDS_MAPS_PREVIEW_STEP_MODEL:
        for (i = 0; i < 4; i++)
        {
            sNdsMapsPreviewLayers[i] = ndsMNMapsPreviewMakeLayer(
                gkind, sNdsMapsPreviewGroundData, i);
        }
        /* mnMapsMakeModel's own hides (mnmaps.c:1048-1060). */
        if ((gkind == nGRKindYamabuki) && (sNdsMapsPreviewLayers[3] != NULL))
        {
            DObjGetChild(DObjGetChild(DObjGetStruct(sNdsMapsPreviewLayers[3])))->flags =
                DOBJ_FLAG_HIDDEN;
        }
        if ((gkind == nGRKindYoster) && (sNdsMapsPreviewLayers[0] != NULL))
        {
            DObj *root_dobj = DObjGetStruct(sNdsMapsPreviewLayers[0]);
            DObj *next_dobj;

            for (next_dobj = root_dobj, i = 1; next_dobj != NULL;
                 next_dobj = lbCommonGetTreeDObjNextFromRoot(next_dobj,
                                                             root_dobj),
                 i++)
            {
                if ((i == 0xF) || (i == 0x11))
                {
                    next_dobj->flags = DOBJ_FLAG_HIDDEN;
                }
            }
        }
        /* After the layers, as the battle makes them: Dream Land's layer-0
         * order (L0, then its three map GObjs) is part of the packet. */
        ndsMNMapsPreviewMakeActors(gkind, sNdsMapsPreviewGroundData);
        ndsRendererAdapterSetStagePreviewLayers(
            (void *const *)sNdsMapsPreviewLayers);
        if (sMNMapsPreviewCObj != NULL)
        {
            ndsMNMapsPreviewPlaceCamera(sMNMapsPreviewCObj, gkind);
        }
        sNdsMapsPreviewHasModel = TRUE;
        sNdsMapsPreviewFreshDraw = TRUE;
        sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_WARM;
        gNdsMapsPreviewFailStep = 0u;
        break;

    default:
        return;
    }
    step_start = cpuGetTiming() - step_start;
    sNdsMapsPreviewLoadTicks += step_start;
    gNdsMapsPreviewLoadTicks = sNdsMapsPreviewLoadTicks;
    if (step_start > gNdsMapsPreviewStepTicksMax)
    {
        gNdsMapsPreviewStepTicksMax = step_start;
    }
    if (sNdsMapsPreviewLoadTicks > gNdsMapsPreviewLoadTicksMax)
    {
        gNdsMapsPreviewLoadTicksMax = sNdsMapsPreviewLoadTicks;
    }
}

void ndsMNMapsPreviewInit(s32 gkind)
{
    LBRelocSetup rl_setup;
    size_t max = 0u;
    size_t size;
    GObj *camera_gobj;
    CObj *cobj;
    s32 i;

    if (sNdsMapsPreviewActive != FALSE)
    {
        return;
    }
    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNMapsStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNMapsStatusBuffer);
    rl_setup.force_status_buffer = sMNMapsForceStatusBuffer;
    rl_setup.force_status_buffer_size = ARRAY_COUNT(sMNMapsForceStatusBuffer);
    lbRelocInitSetup(&rl_setup);

    /* mnMapsAllocModelHeaps' size, one heap. */
    for (i = 0; i < (s32)ARRAY_COUNT(dMNMapsFileInfos); i++)
    {
        size = lbRelocGetFileSize(dMNMapsFileInfos[i].file_id);
        if (max < size)
        {
            max = size;
        }
    }
#if NDS_P5
    for (i = 0; i < (s32)ARRAY_COUNT(sNdsMapsPreviewVenueFileInfos); i++)
    {
        size = lbRelocGetFileSize(sNdsMapsPreviewVenueFileInfos[i].file_id);
        if (max < size)
        {
            max = size;
        }
    }
#define NDS_MAPS_P5_REMIX_SIZE(g, cname, name, map_file, map_offset) \
    size = lbRelocGetFileSize((u32)(map_file)); \
    if (max < size) \
    { \
        max = size; \
    }
    NDS_P5_NATIVE_STAGE_ROWS(NDS_MAPS_P5_REMIX_SIZE)
#undef NDS_MAPS_P5_REMIX_SIZE
#endif
    sNdsMapsPreviewBlockSize = ((max + 0x1fu) & ~(size_t)0x1fu) +
        NDS_MAPS_PREVIEW_BLOB_BYTES;
    sNdsMapsPreviewBlock = syTaskmanMalloc(sNdsMapsPreviewBlockSize, 0x20);
    gNdsMapsPreviewBlockBytes = (sNdsMapsPreviewBlock != NULL) ?
        (u32)sNdsMapsPreviewBlockSize : 0u;
    if (sNdsMapsPreviewBlock != NULL)
    {
        syMallocInit(&sNdsMapsPreviewArena, 0x4d415053u, sNdsMapsPreviewBlock,
                     sNdsMapsPreviewBlockSize);
    }
    else
    {
        gNdsMapsPreviewFailStep = 2u;
    }

    /* mnMapsMakePreviewCamera (mnmaps.c:1336) on the layers' display links,
     * with the select's window fit (ndsMNPlayersVSFitFighterCamera). */
    camera_gobj = gcMakeCameraGObj(1, NULL, 1, GOBJ_PRIORITY_DEFAULT,
                                   func_80017DBC, 65,
                                   COBJ_MASK_DLLINK(4) | COBJ_MASK_DLLINK(6) |
                                   COBJ_MASK_DLLINK(13) | COBJ_MASK_DLLINK(17),
                                   ~0, TRUE, nGCProcessKindThread, NULL, 1,
                                   FALSE);
    sNdsMapsPreviewCamera = camera_gobj;
    if (camera_gobj != NULL)
    {
        cobj = CObjGetStruct(camera_gobj);
        sMNMapsPreviewCObj = cobj;
        syRdpSetViewport(&cobj->viewport, 10.0F, 10.0F, 310.0F, 230.0F);
        cobj->projection.persp.far = 16384.0F;
        cobj->projection.persp.fovy = 32.588150F;
        cobj->projection.persp.aspect = 1.3037037F;
        ndsMNMapsPreviewPlaceCamera(cobj, gkind);
        gcAddGObjProcess(camera_gobj, mnMapsPreviewCameraThreadUpdate,
                         nGCProcessKindThread, 1);
    }
    for (i = 0; i < 4; i++)
    {
        sNdsMapsPreviewLayers[i] = NULL;
    }
    sNdsMapsPreviewHasModel = FALSE;
    sNdsMapsPreviewDrawPhase = 0u;
    /* The scene's light (dMNMapsDisplayList: lighting on, ambient 0x20 and a
     * white light) before each preview draw, as mnMapsFuncStart's setup
     * names it (mnmaps.c:1685); Planet Zebes' lit ground drew black
     * without it. */
    syRdpSetFuncLights(mnMapsFuncLights);
    sNdsMapsPreviewActive = TRUE;
    sNdsMapsPreviewWanted = gkind;
    sNdsMapsPreviewDwell = NDS_MAPS_PREVIEW_DWELL_TICS;
    /* The select's first stage loads whole, behind the scene's entry. */
    ndsMNMapsPreviewBegin(gkind);
    while ((sNdsMapsPreviewStep != NDS_MAPS_PREVIEW_STEP_IDLE) &&
           (sNdsMapsPreviewStep != NDS_MAPS_PREVIEW_STEP_WARM))
    {
        ndsMNMapsPreviewStepLoad();
    }
}

void ndsMNMapsPreviewRequest(s32 gkind)
{
    if (sNdsMapsPreviewActive == FALSE)
    {
        return;
    }
    if (gkind != sNdsMapsPreviewWanted)
    {
        sNdsMapsPreviewWanted = gkind;
        sNdsMapsPreviewDwell = 0u;
    }
}

/* 2026-10-12, owner BUGS.md: "Saffron City looks wrong in SSS, like
 * corrupted geometry". The preview draws a stage at a fraction of its battle
 * size, and the DS samples a texture at its nearest texel where the N64's
 * bilinear tap averaged a neighbourhood: Saffron's window textures broke up
 * into speckle. Each preview upload is low-passed once, in the resolver's
 * scratch before it packs: each texel becomes the mean of its 2x2 block
 * (itself, right, below, below-right; the last row and column clamp), the
 * colour over the block's opaque texels, opaque when at least two are. In
 * place, row by row: a texel's block is read before it is written. The three
 * channels add in one word, 5-bit lanes spread apart (red 0-6, blue 10-16,
 * green 21-27 hold a sum of four). Battle never samples these: the preview
 * releases its uploads when it drops a stage. */
#define NDS_MAPS_PREFILTER_SPREAD(t) (((t) & 0x7c1fu) | (((t) & 0x03e0u) << 16))
#define NDS_MAPS_PREFILTER_PACK(v) \
    ((u16)(0x8000u | ((v) & 0x7c1fu) | (((v) >> 16) & 0x03e0u)))
static void ndsMNMapsPreviewPrefilter(u16 *texels, u32 width, u32 height)
{
    u32 x;
    u32 y;

    if ((width < 2u) || (height < 2u))
    {
        return;
    }
    for (y = 0u; y < height; y++)
    {
        u16 *row = &texels[y * width];
        const u16 *below = ((y + 1u) < height) ? (row + width) : row;

        for (x = 0u; x < width; x++)
        {
            const u32 xr = ((x + 1u) < width) ? (x + 1u) : x;
            const u32 a = row[x];
            const u32 b = row[xr];
            const u32 c = below[x];
            const u32 d = below[xr];

            if ((a & b & c & d & 0x8000u) != 0u)
            {
                const u32 sum = NDS_MAPS_PREFILTER_SPREAD(a) +
                    NDS_MAPS_PREFILTER_SPREAD(b) +
                    NDS_MAPS_PREFILTER_SPREAD(c) +
                    NDS_MAPS_PREFILTER_SPREAD(d) + 0x00400802u;

                row[x] = NDS_MAPS_PREFILTER_PACK(sum >> 2);
            }
            else
            {
                u32 sum = 0u;
                u32 count = 0u;

                if ((a & 0x8000u) != 0u)
                {
                    sum += NDS_MAPS_PREFILTER_SPREAD(a);
                    count++;
                }
                if ((b & 0x8000u) != 0u)
                {
                    sum += NDS_MAPS_PREFILTER_SPREAD(b);
                    count++;
                }
                if ((c & 0x8000u) != 0u)
                {
                    sum += NDS_MAPS_PREFILTER_SPREAD(c);
                    count++;
                }
                if ((d & 0x8000u) != 0u)
                {
                    sum += NDS_MAPS_PREFILTER_SPREAD(d);
                    count++;
                }
                if (count == 2u)
                {
                    row[x] = NDS_MAPS_PREFILTER_PACK((sum + 0x00200401u) >> 1);
                }
                else if (count == 3u)
                {
                    /* (x + 1) * 21846 >> 16: x / 3 rounded, x <= 93. */
                    const u32 red = ((((sum & 0x7fu) + 1u) * 21846u) >> 16);
                    const u32 blue =
                        (((((sum >> 10) & 0x7fu) + 1u) * 21846u) >> 16);
                    const u32 green =
                        (((((sum >> 21) & 0x7fu) + 1u) * 21846u) >> 16);

                    row[x] = (u16)(0x8000u | red | (green << 5) |
                                   (blue << 10));
                }
                else
                {
                    row[x] = (u16)(a & 0x7fffu);
                }
            }
        }
    }
}
#undef NDS_MAPS_PREFILTER_SPREAD
#undef NDS_MAPS_PREFILTER_PACK

/* The 3D layer shows a model only once a draw of it was whole. */
static void ndsMNMapsPreviewShow(sb32 visible)
{
    if (visible != sNdsMapsPreviewVisible)
    {
        sNdsMapsPreviewVisible = visible;
        ndsPlatformSet3DLayerEnabled(visible);
    }
}

void ndsMNMapsPreviewFrame(void)
{
    u32 draw_start;
    sb32 armed;

    if (sNdsMapsPreviewActive == FALSE)
    {
        return;
    }
    /* The renderer's source-frame epoch, as the character select's preview
     * advances it (ndsMNPlayersVSPreviewFrame): per-frame caches key on it,
     * and without it the camera cache kept the first draw's camera for the
     * whole visit -- the camera's bob (mnmaps.c:1319) never reached the
     * screen. */
    gNdsRendererProfileFrameCount++;
    if (sNdsMapsPreviewWanted != sNdsMapsPreviewShown)
    {
        /* The panel already shows the new stage's art: the old model goes
         * at once and the new one loads once the cursor rests. */
        ndsMNMapsPreviewShow(FALSE);
        if (sNdsMapsPreviewDwell < NDS_MAPS_PREVIEW_DWELL_TICS)
        {
            sNdsMapsPreviewDwell++;
        }
        else
        {
            ndsMNMapsPreviewBegin(sNdsMapsPreviewWanted);
        }
    }
    else if ((sNdsMapsPreviewStep != NDS_MAPS_PREVIEW_STEP_IDLE) &&
             (sNdsMapsPreviewStep != NDS_MAPS_PREVIEW_STEP_WARM))
    {
        ndsMNMapsPreviewStepLoad();
    }
    /* The camera's bob is the scene's only process (mnmaps.c:1319). */
    gcRunAll();
    /* 30 Hz like the character select's previews: BG0 retains the last
     * frame between draws. */
    sNdsMapsPreviewDrawPhase ^= 1u;
    if (sNdsMapsPreviewDrawPhase == 0u)
    {
        return;
    }
    if ((sNdsMapsPreviewHasModel == FALSE) || (sNdsMapsPreviewCamera == NULL))
    {
        ndsMNMapsPreviewShow(FALSE);
        return;
    }
    draw_start = cpuGetTiming();
    syTaskmanResetGraphicsHeap();
    func_80004AB0();
    gNdsNativeStageWarmUploadLimit = NDS_MAPS_PREVIEW_WARM_TICKS;
    gNdsRendererTexturePrefilter = ndsMNMapsPreviewPrefilter;
    armed = (ndsRendererAdapterPrepareNativeStageOwner(sNdsMapsPreviewCamera) !=
             FALSE) ? TRUE : FALSE;
    gNdsRendererTexturePrefilter = NULL;
    gNdsNativeStageWarmUploadLimit = 0u;
    if (armed != FALSE)
    {
        /* Only a whole native draw reaches the screen: a prepare that
         * declined (textures still uploading) draws nothing, and the
         * layers' own display procs never draw the preview. */
        gNdsRendererAdapterStagePreviewArmed = 1u;
        gcDrawAll();
        ndsRendererAdapterFinishNativeStageOwner();
        gNdsRendererAdapterStagePreviewArmed = 0u;
        gNdsMapsPreviewNativeDraws++;
        ndsMNMapsPreviewShow(TRUE);
        if (sNdsMapsPreviewStep == NDS_MAPS_PREVIEW_STEP_WARM)
        {
            sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_IDLE;
        }
    }
    gNdsMapsPreviewDraws++;
    draw_start = cpuGetTiming() - draw_start;
    if (draw_start > gNdsMapsPreviewDrawTicksMax)
    {
        gNdsMapsPreviewDrawTicksMax = draw_start;
    }
    if (sNdsMapsPreviewFreshDraw != FALSE)
    {
        gNdsMapsPreviewFirstDrawTicks = draw_start;
        sNdsMapsPreviewFreshDraw = FALSE;
    }
}

void ndsMNMapsPreviewExit(void)
{
    if (sNdsMapsPreviewActive == FALSE)
    {
        return;
    }
    ndsMNMapsPreviewDropModel();
    sNdsMapsPreviewActive = FALSE;
    sNdsMapsPreviewShown = NDS_MAPS_PREVIEW_GKIND_NONE;
    sNdsMapsPreviewWanted = NDS_MAPS_PREVIEW_GKIND_NONE;
    sNdsMapsPreviewCamera = NULL;
    sMNMapsPreviewCObj = NULL;
    sNdsMapsPreviewBlock = NULL;
    gNdsSCVSBattleStageGKind = NDS_NATIVE_STAGE_BLOB_GKIND_INVALID;
    syRdpSetFuncLights(NULL);
    sNdsMapsPreviewStep = NDS_MAPS_PREVIEW_STEP_IDLE;
    sNdsMapsPreviewVisible = FALSE;
    ndsPlatformSet3DLayerEnabled(FALSE);
}
#endif
