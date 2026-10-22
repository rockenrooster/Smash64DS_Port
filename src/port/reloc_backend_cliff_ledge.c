/* Imported ftmain no longer executes the local duplicate damage callbacks. */
#include <nds/nds_effects.h>
#define NDS_FIGHTER_DASH_RUN_IMPORT_PROCPARAMS_MASK 0xfffdf3ffu
#define NDS_FIGHTER_DASH_RUN_IMPORT_DAMAGE_SETUP_MASK 0xbffffdfdu

#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_EMPTY      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_ENABLED    (1u << 1)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_HURT_SKIP  (1u << 2)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_SHLD_SKIP  (1u << 3)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_GROUP_SKIP (1u << 4)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_RESTORE    (1u << 5)

static sb32 ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled(void)
{
    return FALSE;
}

static sb32 ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
    FTAttackColl *attack_coll, GObj *victim_gobj)
{
    GMHitFlags flags;
    u32 slot;

    if ((attack_coll == NULL) || (victim_gobj == NULL))
    {
        return FALSE;
    }
    flags.is_interact_hurt = FALSE;
    flags.is_interact_shield = FALSE;
    flags.timer_rehit = 0;
    flags.group_id = 7u;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == victim_gobj)
        {
            flags = attack_coll->attack_records[slot].victim_flags;
            break;
        }
    }
    return ((flags.is_interact_hurt == FALSE) &&
            (flags.is_interact_shield == FALSE) &&
            (flags.group_id == 7u)) ? TRUE : FALSE;
}


static void ndsFighterMarioFoxStageGCDrawAllLoopReset(void)
{
    gNdsFighterMarioFoxStageGCDrawAllLoopResult = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopMask = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopCount = 0u;
    gNdsStageGCDrawAllLoopPrepared = 0u;
    gNdsStageGCDrawAllLoopBaseResultSeen = 0u;
    gNdsStageGCDrawAllLoopDrawAllCount = 0u;
    gNdsStageGCDrawAllLoopCameraCallbackCount = 0u;
    gNdsStageGCDrawAllLoopCapturedDisplayCount = 0u;
    gNdsStageGCDrawAllLoopLayerCaptureMask = 0u;
    gNdsStageGCDrawAllLoopMapCaptureMask = 0u;
    gNdsStageGCDrawAllLoopDObjDrawCallbackCount = 0u;
    gNdsStageGCDrawAllLoopDObjDrawKindMask = 0u;
    gNdsStageGCDrawAllLoopLayerDObjMask = 0u;
    gNdsStageGCDrawAllLoopMapDObjMask = 0u;
    gNdsStageGCDrawAllLoopLayerDLReadyMask = 0u;
    gNdsStageGCDrawAllLoopMapDLReadyMask = 0u;
    gNdsStageGCDrawAllLoopLayerMObjMask = 0u;
    gNdsStageGCDrawAllLoopMapMObjMask = 0u;
    gNdsStageGCDrawAllLoopNonStageCaptureCount = 0u;
    gNdsStageGCDrawAllLoopFighterDisplayCallbackCount = 0u;
    gNdsStageGCDrawAllLoopUnexpectedSceneCount = 0u;
    gNdsStageGCDrawAllLoopManualDisplayCallCount = 0u;
    gNdsStageGCDrawAllLoopGObjCountBefore = 0u;
    gNdsStageGCDrawAllLoopGObjCountAfter = 0u;
    gNdsStageGCDrawAllLoopGObjCountDelta = 0u;
    gNdsStageGCDrawAllLoopPreviewCommitDelta = 0u;
    gNdsStageGCDrawAllLoopTotalPixelCount = 0u;
    gNdsStageGCDrawAllLoopCompatMask = 0u;
    gNdsStageGCDrawAllLoopHardwareSubmitCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureFormatMask = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureMaxWidth = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureMaxHeight = 0u;
    gNdsStageGCDrawAllLoopHardwareFighterSubmitCount = 0u;
    gNdsStageGCDrawAllLoopHardwareFighterTriangleCount = 0u;
    gNdsWeaponRendererCaptureCount = 0u;
    gNdsWeaponRendererDObjDrawCount = 0u;
    gNdsWeaponRendererSubmitCount = 0u;
    gNdsWeaponRendererVisibleDrawCount = 0u;
    gNdsWeaponRendererTriangleCount = 0u;
    gNdsWeaponRendererTextureReadyCount = 0u;
    gNdsWeaponRendererTextureRejectCount = 0u;
    gNdsWeaponRendererKindMask = 0u;
    gNdsWeaponRendererCallbackKind = 0u;
    gNdsWeaponRendererNoZCount = 0u;
    gNdsWeaponRendererMovingDrawCount = 0u;
    gNdsWeaponRendererLastXBits = 0u;
    gNdsWeaponRendererLastYBits = 0u;
    gNdsWeaponRendererFireballSubmitCount = 0u;
    gNdsWeaponRendererFireballTriangleCount = 0u;
    gNdsWeaponRendererFireballVisibleDrawCount = 0u;
    gNdsWeaponRendererBlasterSubmitCount = 0u;
    gNdsWeaponRendererBlasterTriangleCount = 0u;
    gNdsWeaponRendererBlasterVisibleDrawCount = 0u;
    gNdsItemRendererCaptureCount = 0u;
    gNdsItemRendererDObjDrawCount = 0u;
    gNdsItemRendererSubmitCount = 0u;
    gNdsItemRendererVisibleDrawCount = 0u;
    gNdsItemRendererTriangleCount = 0u;
    gNdsItemRendererTextureReadyCount = 0u;
    gNdsItemRendererTextureRejectCount = 0u;
    gNdsItemRendererKindMask = 0u;
    gNdsItemRendererRejectedDrawCount = 0u;
    gNdsItemRendererLastHead = 0u;
    gNdsItemRendererLastColorMask = 0u;
    gNdsItemRendererLastEnvColor = 0u;
    gNdsItemRendererLastOtherModeL = 0u;
    gNdsItemRendererLastOtherModeH = 0u;
    gNdsItemRendererAttach52BuildCount = 0u;
    gNdsFoxBlasterQuadDrawCount = 0u;
    gNdsFoxBlasterQuadFallbackCount = 0u;
    gNdsWeaponRendererRejectedDrawCount = 0u;
    gNdsEffectRendererCaptureCount = 0u;
    gNdsEffectRendererDObjDrawCount = 0u;
    gNdsEffectRendererSubmitCount = 0u;
    gNdsEffectRendererSourceModelAdmitCount = 0u;
    gNdsRendererStageDObjNodeCount = 0u;
    gNdsRendererStageDObjDepthOverrunCount = 0u;
    gNdsRendererStageDObjSiblingOverrunCount = 0u;
    gNdsEffectRendererCallbackKindMask = 0u;
    gNdsEffectRendererRejectedKindMask = 0u;
    gNdsEffectRendererDObjFlagsMask = 0u;
    gNdsEffectRendererDObjFieldMask = 0u;
    gNdsEffectRendererTriangleCount = 0u;
    gNdsEffectRendererTextureReadyCount = 0u;
    gNdsEffectRendererTextureRejectCount = 0u;
    gNdsEffectRendererRejectedDrawCount = 0u;
    gNdsEffectRendererLink15DrawCount = 0u;
    gNdsEffectDLBlocker = 0u;
    gNdsEffectDLCommandCount = 0u;
    gNdsEffectDLFirstOpcode = 0u;
    gNdsEffectDLUnsupportedOpcode = 0u;
    gNdsEffectDLVertexCount = 0u;
    gNdsEffectDLTriangleCount = 0u;
    gNdsEffectDLPublishCount = 0u;
    gNdsEffectDLCfgMask = 0u;
    gNdsEffectDLCfgMvT[0] = 0;
    gNdsEffectDLCfgMvT[1] = 0;
    gNdsEffectDLCfgMvT[2] = 0;
    gNdsEffectDLMatrixSeed = 0u;
    gNdsEffectDLMatrixCmd = 0u;
    gNdsEffectDLXformVertexCount = 0u;
    gNdsEffectDLHwVertexCount = 0u;
    gNdsEffectDLHwTriangleCount = 0u;
    gNdsEffectDLVtx0[0] = 0;
    gNdsEffectDLVtx0[1] = 0;
    gNdsEffectDLVtx0[2] = 0;
    gNdsEffectDLVtx0[3] = 0;
    gNdsEffectDLColorMask = 0u;
    gNdsEffectDLPrimColor = 0u;
    gNdsEffectDLEnvColor = 0u;
    gNdsEffectDLOtherModeL = 0u;
    gNdsEffectDLOtherModeValid = 0u;
    gNdsEffectDLSubmitOtherModeIn = 0u;
    gNdsEffectDLSubmitOtherModeOut = 0u;
    gNdsEffectDLSubmitCount = 0u;
    gNdsEffectImpactWaveMakeCount = 0u;
    gNdsEffectImpactWaveMakeNullCount = 0u;
    gNdsEffectImpactWaveLastIndex = -1;
    gNdsEffectImpactWaveLastGObjID = 0u;
    gNdsStageGCDrawAllLoopHardwareCarrySeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryCaptureCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount = 0u;
    sNdsStageGCDrawAllLoopCurrentDisplayLinkID = -1;
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
    sNdsStageGCDrawAllLoopHardwareSubmitCount = 0u;
}

void ndsFighterMarioFoxStageGCDrawAllLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() == FALSE) ||
        (gNdsStageGCDrawAllLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageGCDrawAllLoopReset();
    gNdsStageGCDrawAllLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();
    gNdsStageGCDrawAllLoopPrepared = 1u;
}

s32 ndsFighterMarioFoxLivePreviewUpdateEnabled(void)
{
    return FALSE;
}
