/* Bounded original Dream Land ground setup import.
 *
 * This translation unit imports the original common ground display/setup code
 * and the Pupupu stage object setup, then wraps grCommonSetupInitAll so only
 * the verified Pupupu VSBattle boundary enters the original path. Continuous
 * stage update/draw and Whispy runtime behavior remain parked by the taskman
 * boundary; compatibility stubs record if those paths are touched.
 */
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <mn/menu.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

extern s32 gcGetGObjsActiveNum(void);
extern u32 sGCDrawsActiveNum;
extern u32 sGCMaterialsActive;

void gcDrawDObjTreeForGObj(GObj *gobj);
void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
void gcSetupCustomDObjs(GObj *gobj, DObjDesc *dobjdesc, DObj **dobjs,
                        u8 tk1, u8 rk1, u8 sk1);
void gcAddMObjAll(GObj *gobj, MObjSub ***p_mobjsubs);
void gcAddAnimAll(GObj *gobj, AObjEvent32 **anim_joints,
                  AObjEvent32 ***matanim_joints, f32 anim_frame);
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints,
                       f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);
s32 syUtilsRandIntRange(s32 range);

GObj *grCastleMakeGround(void);
GObj *grSectorMakeGround(void);
GObj *grJungleMakeGround(void);
GObj *grZebesMakeGround(void);
GObj *grHyruleMakeGround(void);
GObj *grYosterMakeGround(void);
GObj *grPupupuMakeGround(void);
GObj *grYamabukiMakeGround(void);
GObj *grInishieMakeGround(void);
GObj *grBonus3MakeGround(void);
void sc1PBonusStageInitBonus2(void);
void sc1PBonusStageMakeBonus1Ground(void);

#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRPupupuMapWhispyEyesTransformKindsMObjSub NDS_RELOC_LVALUE(0x0f00u)
#define llGRPupupuMapMapHead NDS_RELOC_LVALUE(0x10f0u)
#define llGRPupupuMapWhispyEyesTransformKindsDObjDesc NDS_RELOC_LVALUE(0x10f0u)
#define llGRPupupuMapWhispyMouthTransformKindsMObjSub NDS_RELOC_LVALUE(0x13b0u)
#define llGRPupupuMapWhispyMouthTransformKindsDObjDesc NDS_RELOC_LVALUE(0x1770u)
#define llGRPupupuMapFlowersBackTransformKindsDObjDesc NDS_RELOC_LVALUE(0x2a80u)
#define llGRPupupuMapFlowersFrontTransformKindsDObjDesc NDS_RELOC_LVALUE(0x31f8u)
#define llGRPupupuMapWhispyEyesLeftTurnAnimJoint NDS_RELOC_LVALUE(0x11a0u)
#define llGRPupupuMapWhispyEyesLeftTurnMatAnimJoint NDS_RELOC_LVALUE(0x11e0u)
#define llGRPupupuMapWhispyEyesLeftBlinkAnimJoint NDS_RELOC_LVALUE(0x12b0u)
#define llGRPupupuMapWhispyEyesRightTurnAnimJoint NDS_RELOC_LVALUE(0x1220u)
#define llGRPupupuMapWhispyEyesRightTurnMatAnimJoint NDS_RELOC_LVALUE(0x1270u)
#define llGRPupupuMapWhispyEyesRightBlinkAnimJoint NDS_RELOC_LVALUE(0x1330u)
#define llGRPupupuMapWhispyMouthLeftStretchAnimJoint NDS_RELOC_LVALUE(0x18b0u)
#define llGRPupupuMapWhispyMouthLeftStretchMatAnimJoint NDS_RELOC_LVALUE(0x1a00u)
#define llGRPupupuMapWhispyMouthLeftTurnAnimJoint NDS_RELOC_LVALUE(0x1be0u)
#define llGRPupupuMapWhispyMouthLeftTurnMatAnimJoint NDS_RELOC_LVALUE(0x1ce0u)
#define llGRPupupuMapWhispyMouthLeftOpenAnimJoint NDS_RELOC_LVALUE(0x1e80u)
#define llGRPupupuMapWhispyMouthLeftOpenMatAnimJoint NDS_RELOC_LVALUE(0x20b0u)
#define llGRPupupuMapWhispyMouthLeftCloseAnimJoint NDS_RELOC_LVALUE(0x2100u)
#define llGRPupupuMapWhispyMouthLeftCloseMatAnimJoint NDS_RELOC_LVALUE(0x22a0u)
#define llGRPupupuMapWhispyMouthRightStretchAnimJoint NDS_RELOC_LVALUE(0x1a40u)
#define llGRPupupuMapWhispyMouthRightStretchMatAnimJoint NDS_RELOC_LVALUE(0x1ba0u)
#define llGRPupupuMapWhispyMouthRightTurnAnimJoint NDS_RELOC_LVALUE(0x1d30u)
#define llGRPupupuMapWhispyMouthRightTurnMatAnimJoint NDS_RELOC_LVALUE(0x1e30u)
#define llGRPupupuMapWhispyMouthRightOpenAnimJoint NDS_RELOC_LVALUE(0x22f0u)
#define llGRPupupuMapWhispyMouthRightOpenMatAnimJoint NDS_RELOC_LVALUE(0x2540u)
#define llGRPupupuMapWhispyMouthRightCloseAnimJoint NDS_RELOC_LVALUE(0x2590u)
#define llGRPupupuMapWhispyMouthRightCloseMatAnimJoint NDS_RELOC_LVALUE(0x2740u)
#define llGRPupupuMapWhispyMouthLeftOpenTexture NDS_RELOC_LVALUE(0x2be0u)
#define llGRPupupuMapWhispyMouthLeftBlowTexture NDS_RELOC_LVALUE(0x2c30u)
#define llGRPupupuMapWhispyMouthLeftCloseTexture NDS_RELOC_LVALUE(0x2c80u)
#define llGRPupupuMapWhispyMouthRightOpenTexture NDS_RELOC_LVALUE(0x2cd0u)
#define llGRPupupuMapWhispyMouthRightBlowTexture NDS_RELOC_LVALUE(0x2d20u)
#define llGRPupupuMapWhispyMouthRightCloseTexture NDS_RELOC_LVALUE(0x2d70u)
#define llGRPupupuMapWhispyEyesLeft0Texture NDS_RELOC_LVALUE(0x33e0u)
#define llGRPupupuMapWhispyEyesLeft1Texture NDS_RELOC_LVALUE(0x3450u)
#define llGRPupupuMapWhispyEyesLeft2Texture NDS_RELOC_LVALUE(0x34b0u)
#define llGRPupupuMapWhispyEyesRight0Texture NDS_RELOC_LVALUE(0x3510u)
#define llGRPupupuMapWhispyEyesRight1Texture NDS_RELOC_LVALUE(0x35c0u)
#define llGRPupupuMapWhispyEyesRight2Texture NDS_RELOC_LVALUE(0x3660u)

#define grCommonSetupInitAll ndsBaseGRCommonSetupInitAll

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grdisplay.c"
#include "../../decomp/BattleShip-main/decomp/src/gr/grmainsetup.c"
#include "../../decomp/BattleShip-main/decomp/src/gr/grcommonsetup.c"
#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c"

#undef grCommonSetupInitAll

static void ndsGRPupupuRecordBefore(void)
{
    gNdsPupupuGroundGObjCountBefore = (u32)gcGetGObjsActiveNum();

    if ((gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->gkind == nGRKindPupupu) &&
        (gMPCollisionGroundData != NULL) &&
        (gNdsSCVSBattleStageGroundDataReady != 0u))
    {
        gNdsPupupuGroundSetupMask |= 1u << 0;
    }
    gNdsPupupuGroundSetupMask |= 1u << 1;
}

static void ndsGRPupupuRecordLayer(GObj *gobj, u32 bit)
{
    DObj *dobj;

    if (gobj == NULL)
    {
        return;
    }

    gNdsPupupuGroundLayerGObjCount++;
    gNdsPupupuGroundLayerGObjMask |= bit;

    dobj = DObjGetStruct(gobj);
    if (dobj != NULL)
    {
        gNdsPupupuGroundLayerDObjMask |= bit;
        if (dobj->mobj != NULL)
        {
            gNdsPupupuGroundLayerMObjMask |= bit;
        }
        if (dobj->aobj != NULL)
        {
            gNdsPupupuGroundLayerAnimMask |= bit;
        }
    }
}

static void ndsGRPupupuRecordMapGObj(GObj *gobj, u32 bit, volatile u32 *out_id)
{
    if (gobj == NULL)
    {
        return;
    }

    gNdsPupupuGroundMapGObjCount++;
    gNdsPupupuGroundMapGObjMask |= bit;
    *out_id = gobj->id;
}

static void ndsGRPupupuRecordAfter(void)
{
    u32 i;

    gNdsPupupuGroundSetupMask |= 1u << 2;

    for (i = 0; i < 4u; i++)
    {
        ndsGRPupupuRecordLayer(gGRCommonLayerGObjs[i], 1u << i);
    }
    if (gNdsPupupuGroundLayerGObjCount != 0u)
    {
        gNdsPupupuGroundSetupMask |= 1u << 3;
        gNdsPupupuGroundDisplayResult = NDS_PUPUPU_GROUND_DISPLAY_PASS;
    }

    gNdsPupupuGroundSetupMask |= 1u << 4;
    gNdsPupupuGroundSetupMask |= 1u << 5;

    if (gGRCommonStruct.pupupu.map_head != NULL)
    {
        gNdsPupupuGroundMapHeadReady = 1;
        gNdsPupupuGroundMapHeadOffset = 0x10f0u;
        gNdsPupupuGroundSetupMask |= 1u << 6;
    }

    ndsGRPupupuRecordMapGObj(gGRCommonStruct.pupupu.map_gobj[0], 1u << 0,
                             &gNdsPupupuGroundWhispyEyesGObjID);
    ndsGRPupupuRecordMapGObj(gGRCommonStruct.pupupu.map_gobj[1], 1u << 1,
                             &gNdsPupupuGroundWhispyMouthGObjID);
    ndsGRPupupuRecordMapGObj(gGRCommonStruct.pupupu.map_gobj[2], 1u << 2,
                             &gNdsPupupuGroundFlowersBackGObjID);
    ndsGRPupupuRecordMapGObj(gGRCommonStruct.pupupu.map_gobj[3], 1u << 3,
                             &gNdsPupupuGroundFlowersFrontGObjID);

    if (gNdsPupupuGroundMapGObjCount == 4u)
    {
        gNdsPupupuGroundSetupMask |= 1u << 7;
    }

    if ((gNdsPupupuGroundLayerDObjMask != 0u) ||
        (gNdsPupupuGroundMapGObjMask != 0u))
    {
        gNdsPupupuGroundSetupMask |= 1u << 8;
    }

    gNdsPupupuGroundParticleBankID =
        (u32)gGRCommonStruct.pupupu.particle_bank_id;
    gNdsPupupuGroundGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsPupupuGroundDObjCountAfter = sGCDrawsActiveNum;
    gNdsPupupuGroundMObjCountAfter = sGCMaterialsActive;
    gNdsPupupuGroundAObjCountAfter =
        (gNdsPupupuGroundLayerAnimMask != 0u) ? 1u : 0u;
    gNdsPupupuGroundProcessAttachCount =
        gNdsPupupuGroundGObjCountAfter -
        gNdsPupupuGroundGObjCountBefore;

    if (gGCCommonLinks[nGCCommonLinkIDGround] != NULL)
    {
        gNdsPupupuGroundRootGObjID =
            gGCCommonLinks[nGCCommonLinkIDGround]->id;
    }

    gNdsPupupuGroundDeferredMask |= 1u << 3;
    gNdsPupupuGroundDeferredMask |= 1u << 4;
    gNdsPupupuGroundGObjResult = NDS_PUPUPU_GROUND_GOBJ_PASS;

    if ((gNdsPupupuGroundSetupMask & 0x3ffu) == 0x3ffu)
    {
        gNdsPupupuGroundSetupResult = NDS_PUPUPU_GROUND_SETUP_PASS;
    }
}

static u32 ndsGRPupupuMapGObjMask(void)
{
    u32 mask = 0;
    u32 i;

    for (i = 0; i < 4u; i++)
    {
        if (gGRCommonStruct.pupupu.map_gobj[i] != NULL)
        {
            mask |= 1u << i;
        }
    }
    return mask;
}

void ndsGRPupupuRunSafeUpdateProbe(void)
{
    u32 mask = 0;

    gNdsPupupuUpdateResult = 0;
    gNdsPupupuUpdateMask = 0;
    gNdsPupupuUpdateTickCount = 0;

    if ((gNdsPupupuGroundSetupResult == NDS_PUPUPU_GROUND_SETUP_PASS) &&
        ((gNdsPupupuGroundSetupMask & 0x3ffu) == 0x3ffu) &&
        (gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->gkind == nGRKindPupupu) &&
        (gMPCollisionGroundData != NULL) &&
        (gGRCommonStruct.pupupu.map_head != NULL) &&
        (ndsGRPupupuMapGObjMask() == 0xfu))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsPupupuUpdateMask = mask;
        return;
    }

    gNdsPupupuUpdateGameStatusBefore =
        (u32)gSCManagerBattleState->game_status;
    gNdsPupupuUpdateMapGObjMaskBefore = ndsGRPupupuMapGObjMask();
    gNdsPupupuUpdateGObjCountBefore = (u32)gcGetGObjsActiveNum();

    gSCManagerBattleState->game_status = nSCBattleGameStatusGo;

    gGRCommonStruct.pupupu.whispy_status =
        nGRPupupuWhispyWindStatusSleep;
    gGRCommonStruct.pupupu.whispy_wind_wait = 2;
    gGRCommonStruct.pupupu.whispy_wind_duration = 0;
    gGRCommonStruct.pupupu.whispy_blink_wait = 3;
    gGRCommonStruct.pupupu.whispy_eyes_status = -1;
    gGRCommonStruct.pupupu.whispy_mouth_status = -1;
    gGRCommonStruct.pupupu.whispy_mouth_texture = -1;
    gGRCommonStruct.pupupu.whispy_eyes_texture = -1;
    gGRCommonStruct.pupupu.flowers_back_status =
        nGRPupupuFlowerStatusDefault;
    gGRCommonStruct.pupupu.flowers_front_status =
        nGRPupupuFlowerStatusDefault;
    gGRCommonStruct.pupupu.flowers_back_wait = 15;
    gGRCommonStruct.pupupu.flowers_front_wait = 22;

    gNdsPupupuUpdateWhispyStatusBefore =
        gGRCommonStruct.pupupu.whispy_status;
    gNdsPupupuUpdateWindWaitBefore =
        gGRCommonStruct.pupupu.whispy_wind_wait;
    gNdsPupupuUpdateBlinkWaitBefore =
        (u32)gGRCommonStruct.pupupu.whispy_blink_wait;
    gNdsPupupuUpdateFlowersBackStatusBefore =
        gGRCommonStruct.pupupu.flowers_back_status;
    gNdsPupupuUpdateFlowersFrontStatusBefore =
        gGRCommonStruct.pupupu.flowers_front_status;

    mask |= 1u << 1;

    grPupupuProcUpdate(gGRCommonStruct.pupupu.map_gobj[0]);
    gNdsPupupuUpdateTickCount++;

    gNdsPupupuUpdateWhispyStatusAfterFirst =
        gGRCommonStruct.pupupu.whispy_status;
    gNdsPupupuUpdateWindWaitAfterFirst =
        gGRCommonStruct.pupupu.whispy_wind_wait;

    if (gGRCommonStruct.pupupu.whispy_status ==
        nGRPupupuWhispyWindStatusWait)
    {
        mask |= 1u << 2;
    }

    grPupupuProcUpdate(gGRCommonStruct.pupupu.map_gobj[0]);
    gNdsPupupuUpdateTickCount++;

    gNdsPupupuUpdateWhispyStatusAfterFinal =
        gGRCommonStruct.pupupu.whispy_status;
    gNdsPupupuUpdateWindWaitAfterFinal =
        gGRCommonStruct.pupupu.whispy_wind_wait;
    gNdsPupupuUpdateBlinkWaitAfterFinal =
        (u32)gGRCommonStruct.pupupu.whispy_blink_wait;
    gNdsPupupuUpdateFlowersBackStatusAfterFinal =
        gGRCommonStruct.pupupu.flowers_back_status;
    gNdsPupupuUpdateFlowersFrontStatusAfterFinal =
        gGRCommonStruct.pupupu.flowers_front_status;
    gNdsPupupuUpdateMapGObjMaskAfter = ndsGRPupupuMapGObjMask();
    gNdsPupupuUpdateGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsPupupuUpdateGameStatusAfter =
        (u32)gSCManagerBattleState->game_status;

    if ((gGRCommonStruct.pupupu.whispy_status ==
         nGRPupupuWhispyWindStatusWait) &&
        (gGRCommonStruct.pupupu.whispy_wind_wait == 1u))
    {
        mask |= 1u << 3;
    }

    if (gNdsPupupuUpdateMapGObjMaskAfter == 0xfu)
    {
        mask |= 1u << 4;
    }

    if ((gNdsPupupuUpdateVelPushCount == 0u) &&
        (gNdsPupupuUpdateQuakeCount == 0u) &&
        (gNdsPupupuUpdateParticleScriptCount == 0u) &&
        (gNdsPupupuUpdateWindFGMCount == 0u) &&
        (gGRCommonStruct.pupupu.flowers_back_status ==
         nGRPupupuFlowerStatusDefault) &&
        (gGRCommonStruct.pupupu.flowers_front_status ==
         nGRPupupuFlowerStatusDefault))
    {
        mask |= 1u << 5;
    }

    if (gNdsPupupuUpdateGObjCountAfter ==
        gNdsPupupuUpdateGObjCountBefore)
    {
        mask |= 1u << 6;
    }

    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 7;
    }

    gNdsPupupuUpdateMask = mask;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsPupupuUpdateResult = NDS_PUPUPU_UPDATE_PASS;
    }
}

void grCommonSetupInitAll(void)
{
    if ((gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->gkind == nGRKindPupupu) &&
        (gMPCollisionGroundData != NULL) &&
        (gNdsSCVSBattleStageGroundDataReady != 0u))
    {
        ndsGRPupupuRecordBefore();
        ndsBaseGRCommonSetupInitAll();
        ndsGRPupupuRecordAfter();
        return;
    }

    ndsGRCompatibilityNonPupupuSetup();
}

static GObj *ndsGRNonPupupuGroundStub(void)
{
    gNdsPupupuGroundNonPupupuStubCallCount++;
    return NULL;
}

GObj *grCastleMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grSectorMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grJungleMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grZebesMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grHyruleMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grYosterMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grYamabukiMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grInishieMakeGround(void) { return ndsGRNonPupupuGroundStub(); }
GObj *grBonus3MakeGround(void) { return ndsGRNonPupupuGroundStub(); }

void sc1PBonusStageInitBonus2(void)
{
    gNdsPupupuGroundNonPupupuStubCallCount++;
}

void sc1PBonusStageMakeBonus1Ground(void)
{
    gNdsPupupuGroundNonPupupuStubCallCount++;
}
