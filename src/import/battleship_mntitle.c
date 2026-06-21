/* Compile the original BattleShip Title scene translation unit.
 *
 * The exposed DS entry remains bounded: it starts the original taskman scene
 * and calls original title setup functions through a guarded func_start, then
 * parks before the fire/logo/slash/effect-heavy visual branches.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
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

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
extern s32 gcGetGObjsActiveNum(void);
extern u32 sGCSpritesActiveNum;

extern GObj *sMNTitleTransitionsGObj;
extern GObj *sMNTitleMainGObj;
extern void *sMNTitleFiles[];

extern char dSCManagerBuildDate[];
extern s32 osResetType;

extern void mnTitleFuncUpdate(void);
extern void mnTitleFuncLights(Gfx **dls);
void mnTitleSetPosition(DObj *dobj, SObj *sobj, s32 kind);
void mnTitleShowFire(GObj *gobj);
void mnTitleSetColors(SObj *sobj, s32 kind);
void mnTitleTransitionFromFireLogo(void);
void mnTitleShowGObjLinkID(s32 link_id);

#define mnTitleFuncStart ndsBaseMNTitleFuncStart
#define mnTitleStartScene ndsBaseMNTitleStartScene

void ndsBaseMNTitleFuncStart(void);
void ndsBaseMNTitleStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mncommon/mntitle.c"

#undef mnTitleFuncStart
#undef mnTitleStartScene

void mnTitleFuncStart(void);

static GObj *ndsMNTitleFindGObjByLinkAndID(u8 link_id, u32 id)
{
    GObj *gobj = gGCCommonLinks[link_id];

    while (gobj != NULL)
    {
        if (gobj->id == id)
        {
            return gobj;
        }
        gobj = gobj->link_next;
    }
    return NULL;
}

static u32 ndsMNTitleCountSObjs(GObj *gobj)
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

static void ndsMNTitleMakeLogoFireBounded(void)
{
    s32 before_count = gcGetGObjsActiveNum();
    s32 after_count;
    GObj *logo_fire_gobj;

    efParticleInitAll();
    mnTitleMakeLogoFire();

    after_count = gcGetGObjsActiveNum();
    logo_fire_gobj = ndsMNTitleFindGObjByLinkAndID(4, 15);

    gNdsTitleOriginalLogoFireParticleBank = (u32)sMNTitleParticleBankID;
    if (after_count >= before_count)
    {
        gNdsTitleOriginalLogoFireGObjDelta = (u32)(after_count - before_count);
    }

    if (logo_fire_gobj != NULL)
    {
        gNdsTitleOriginalLogoFireResult = NDS_TITLE_ORIGINAL_LOGO_FIRE_PASS;
        gNdsTitleOriginalLogoFireLinkID = logo_fire_gobj->link_id;
        gNdsTitleOriginalLogoFireDLLinkID = logo_fire_gobj->dl_link_id;
        gNdsTitleOriginalLogoFireCameraMaskLo =
            (u32)(logo_fire_gobj->camera_mask & 0xFFFFFFFFu);

        gNdsTitleOriginalLogoFireMask |= (1u << 0);
        if (logo_fire_gobj->link_id == 4)
        {
            gNdsTitleOriginalLogoFireMask |= (1u << 1);
        }
        if (logo_fire_gobj->dl_link_id == 3)
        {
            gNdsTitleOriginalLogoFireMask |= (1u << 2);
        }
        if ((logo_fire_gobj->camera_mask & COBJ_MASK_DLLINK(0)) != 0)
        {
            gNdsTitleOriginalLogoFireMask |= (1u << 3);
        }
        if (logo_fire_gobj->proc_display == mnTitleLogoFireProcDisplay)
        {
            gNdsTitleOriginalLogoFireMask |= (1u << 4);
        }
    }

    if (gNdsTitleOriginalLogoFireGObjDelta == 1u)
    {
        gNdsTitleOriginalLogoFireMask |= (1u << 5);
    }
}

static void ndsMNTitleMakeFireBounded(void)
{
    u32 before_gobjs = (u32)gcGetGObjsActiveNum();
    u32 before_sobjs = sGCSpritesActiveNum;
    GObj *fire_gobj;
    SObj *first_sobj;
    SObj *next_sobj;

    mnTitleMakeFire();

    fire_gobj = ndsMNTitleFindGObjByLinkAndID(6, 5);
    gNdsTitleOriginalFireGObjDelta =
        (u32)gcGetGObjsActiveNum() - before_gobjs;
    gNdsTitleOriginalFireSObjDelta = sGCSpritesActiveNum - before_sobjs;
    gNdsTitleOriginalFireAlpha = (u32)sMNTitleFireAlpha;

    if (fire_gobj == NULL)
    {
        return;
    }

    first_sobj = SObjGetStruct(fire_gobj);
    next_sobj = (first_sobj != NULL) ? first_sobj->next : NULL;

    gNdsTitleOriginalFireResult = NDS_TITLE_ORIGINAL_FIRE_PASS;
    gNdsTitleOriginalFireGObjFlags = fire_gobj->flags;
    gNdsTitleOriginalFireSObjCount = ndsMNTitleCountSObjs(fire_gobj);

    if ((first_sobj != NULL) && (next_sobj != NULL))
    {
        gNdsTitleOriginalFireFrames =
            (((u32)(u16)first_sobj->user_data.s) << 16) |
            ((u32)(u16)next_sobj->user_data.s);
    }

    gNdsTitleOriginalFireMask |= (1u << 0);
    if (fire_gobj->link_id == 6)
    {
        gNdsTitleOriginalFireMask |= (1u << 1);
    }
    if (fire_gobj->dl_link_id == 0)
    {
        gNdsTitleOriginalFireMask |= (1u << 2);
    }
    if (fire_gobj->proc_display == mnTitleFireProcDisplay)
    {
        gNdsTitleOriginalFireMask |= (1u << 3);
    }
    if (fire_gobj->func_run == mnTitleFireFuncRun)
    {
        gNdsTitleOriginalFireMask |= (1u << 4);
    }
    if ((fire_gobj->gobjproc_head != NULL) &&
        (fire_gobj->gobjproc_head->func_id == mnTitleFireProcUpdate))
    {
        gNdsTitleOriginalFireMask |= (1u << 5);
    }
    if (gNdsTitleOriginalFireSObjCount == 2u)
    {
        gNdsTitleOriginalFireMask |= (1u << 6);
    }
    if (gNdsTitleOriginalFireSObjDelta == 2u)
    {
        gNdsTitleOriginalFireMask |= (1u << 7);
    }
    if (gNdsTitleOriginalFireFrames == 0x000C0000u)
    {
        gNdsTitleOriginalFireMask |= (1u << 8);
    }
    if ((first_sobj != NULL) && (next_sobj != NULL) &&
        ((first_sobj->sprite.attr & SP_TRANSPARENT) != 0) &&
        ((next_sobj->sprite.attr & SP_TRANSPARENT) != 0))
    {
        gNdsTitleOriginalFireMask |= (1u << 9);
    }
    if (((gSCManagerSceneData.scene_prev == nSCKindOpeningNewcomers) &&
         (fire_gobj->flags == GOBJ_FLAG_HIDDEN)) ||
        ((gSCManagerSceneData.scene_prev != nSCKindOpeningNewcomers) &&
         (fire_gobj->flags == GOBJ_FLAG_NONE)))
    {
        gNdsTitleOriginalFireMask |= (1u << 10);
    }
    if (((gSCManagerSceneData.scene_prev == nSCKindOpeningNewcomers) &&
         (sMNTitleFireAlpha == 0)) ||
        ((gSCManagerSceneData.scene_prev != nSCKindOpeningNewcomers) &&
         (sMNTitleFireAlpha == 0xFF)))
    {
        gNdsTitleOriginalFireMask |= (1u << 11);
    }
}

static SYTaskmanSetup ndsMNTitleMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = {
        {
            0,
            mnTitleFuncUpdate,
            gcDrawAll,
            NULL,
            0,
            1,
            2,
            sizeof(Gfx) * 3584,
            sizeof(Gfx) * 1280,
            0,
            0,
            0x1000,
            2,
            0x1000,
            mnTitleFuncLights,
            syControllerFuncRead,
        },
        0,
        sizeof(u64) * 192,
        0,
        0,
        0,
        0,
        sizeof(GObj),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        sizeof(DObj),
        0,
        sizeof(SObj),
        0,
        sizeof(CObj),
        mnTitleFuncStart,
    };

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    return setup;
}

void mnTitleFuncStart(void)
{
    s32 i;

    gNdsTitleOriginalFuncStartResult = NDS_TITLE_ORIGINAL_FUNC_START_PASS;

    for (i = 0; i < ARRAY_COUNT(gSYControllerDevices); i++)
    {
        syControllerStopRumble(i);
    }

    mnTitleLoadFiles();
    if ((sMNTitleFiles[0] != NULL) && (sMNTitleFiles[1] != NULL))
    {
        gNdsTitleOriginalLoadedFileCount = 2;
        gNdsTitleOriginalSetupMask |= (1u << 0);
    }

    mnTitleMakeActors();
    if ((sMNTitleMainGObj != NULL) && (sMNTitleTransitionsGObj != NULL))
    {
        gNdsTitleOriginalSetupMask |= (1u << 1);
        gNdsTitleOriginalMainGObjID = sMNTitleMainGObj->id;
        gNdsTitleOriginalTransitionGObjID = sMNTitleTransitionsGObj->id;
        gNdsTitleOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    }

    ndsMNTitleMakeLogoFireBounded();

    if (mnTitleMakeCameras() == 0)
    {
        gNdsTitleOriginalSetupMask |= (1u << 2);
        gNdsTitleOriginalCameraCount = 4;
    }

    mnTitleInitVars();
    gNdsTitleOriginalSetupMask |= (1u << 3);

    ndsMNTitleMakeFireBounded();

    gNdsTitleOriginalDeferredMask =
        (1u << 1) | /* mnTitleMakeLogo */
        (1u << 2) | /* mnTitleMakeLabels / Press Start */
        (1u << 3) | /* mnTitleMakeSlash */
        (1u << 4);  /* mnTitleMakeLogoFireParticles */
}

void ndsMNTitleRunBoundedUpdates(u32 count)
{
    u32 i;

    if (gSCManagerSceneData.scene_prev != nSCKindOpeningNewcomers)
    {
        gNdsTitleOriginalLayout = (u32)sMNTitleLayout;
        gNdsTitleOriginalTransitionTics = (u32)sMNTitleTransitionTotalTimeTics;
        gNdsTitleOriginalStartActorProcess = (u32)sMNTitleIsStartActorProcess;
        gNdsTitleOriginalProceedScene = (u32)sMNTitleIsProceedScene;
        gNdsTitleOriginalProceedWait = (u32)sMNTitleProceedSceneWait;
        return;
    }

    for (i = 0; i < count; i++)
    {
        mnTitleFuncUpdate();
        gNdsTitleOriginalUpdateCount++;
    }

    gNdsTitleOriginalUpdateResult = NDS_TITLE_ORIGINAL_UPDATE_PASS;
    gNdsTitleOriginalLayout = (u32)sMNTitleLayout;
    gNdsTitleOriginalTransitionTics = (u32)sMNTitleTransitionTotalTimeTics;
    gNdsTitleOriginalStartActorProcess = (u32)sMNTitleIsStartActorProcess;
    gNdsTitleOriginalProceedScene = (u32)sMNTitleIsProceedScene;
    gNdsTitleOriginalProceedWait = (u32)sMNTitleProceedSceneWait;
}

void mnTitleStartScene(void)
{
    dMNTitleVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNTitleVideoSetup);

    if ((!gSCManagerSceneData.is_title_anim_viewed) &&
        (gSCManagerBackupData.boot <= U8_MAX))
    {
        gSCManagerBackupData.boot++;
        lbBackupWrite();
    }

    gNdsTitleOriginalStartResult = NDS_TITLE_ORIGINAL_START_PASS;

    {
        SYTaskmanSetup setup = ndsMNTitleMakeTaskmanSetup();
        syTaskmanStartTask(&setup);
    }
}
